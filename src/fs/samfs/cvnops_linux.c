/*
 * ----- cvnops_linux.c -
 *
 * Processes the common vnode functions supported on the QFS File System.
 *
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at pkg/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#ifdef sun
#pragma ident "$Revision: 1.38 $"
#endif

#include "sam/osversion.h"

/*
 * ----- UNIX Includes
 */

#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/file.h>
#include <linux/uio.h>
#include <linux/dirent.h>
#include <linux/proc_fs.h>
#include <linux/limits.h>
#include <linux/net.h>
#include <linux/socket.h>
#include <linux/skbuff.h>
#include <linux/version.h>
#include <net/sock.h>

#include <asm/param.h>
#include <asm/uaccess.h>


/*
 * ----- SAMFS Includes
 */

#include "sam/param.h"
#include "sam/types.h"
#include "sam/fioctl.h"

#include "samfs_linux.h"
#include "ino.h"
#include "macros.h"
#include "inode.h"
#include "mount.h"
#include "rwio.h"
#include "clextern.h"

#include "debug.h"
#include "trace.h"
#include "kstats_linux.h"


int sam_get_sparse_blocks(sam_node_t *ip, enum SPARSE_type flag, uio_t *uiop,
					int ioflag, cred_t *credp);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
ssize_t samqfs_generic_file_read(struct file *file,
				const char __user *buf,
				size_t count, loff_t *ppos);
ssize_t samqfs_generic_file_write(struct file *file,
				const char __user *buf,
				size_t count, loff_t *ppos);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
#define	samqfs_generic_file_read(file, buf, count, ppos) \
	generic_file_read(file, buf, count, ppos)

#define	samqfs_generic_file_write(file, buf, count, ppos) \
	rfs_generic_file_write(file, buf, count, ppos)
#endif


/*
 * ----- sam_read_vn -
 * Read a SAM-QFS file.
 */

/* ARGSUSED2 */
ssize_t					/* ERRNO if error, 0 if successful. */
sam_read_vn(struct file *fp, char *buf, ssize_t count, loff_t *ppos)
{
	int error = 0;
	ssize_t rval;
	struct dentry *de = fp->f_dentry;
	struct inode *li = de->d_inode;
	sam_node_t *ip, *bip;
	loff_t offset;
	int stage_n_set = 0;
	uio_t uio;
	sam_cred_t sam_cred;
	cred_t *credp;
	int restoreodirect = 0;
	size_t ccount, rval_total = 0;
	int doingodirect = 0;
	char *cbuf;
	krw_t lockstate = RW_READER;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	if (fp->f_flags & O_DIRECT) {
		lockstate = RW_WRITER;
	}
#endif

	ip = SAM_LITOSI(li);
	bip = ip;

	offset = *ppos;

	TRACE(T_SAM_READ, li, ip->di.id.ino, (sam_tr_t)offset, count);

	RW_LOCK_OS(&ip->inode_rwl, lockstate);

	if (ip->di.status.b.damaged) {
		rval_total = -ENODATA;		/* best fit errno */
		goto out;
	}

	if (S_ISREQ(ip->di.mode)) {
		rval_total = -ENOTSUP;
		goto out;
	}

	if (count == 0) {
		rval_total = 0;
		goto out;
	}

	/*
	 * If offline, return when enough of the file is online for the request.
	 */
	if (ip->di.status.b.offline) {

		uio.uio_loffset = *ppos;
		uio.uio_resid = count;

		sam_set_cred(NULL, &sam_cred);
		credp = (cred_t *)&sam_cred;

		if ((error = sam_read_proc_offline(ip, bip, &uio, credp,
		    &stage_n_set))) {
			rval_total = -error;
			goto out;
		}
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	if (fp->f_ra.ra_pages < 128) {
		fp->f_ra.ra_pages = 128;
	}
#endif

	doingodirect = fp->f_flags & O_DIRECT;

	if (doingodirect &&
	    ((rfs_i_size_read(li) - offset) < SAM_DEV_BSIZE_LINUX)) {
		/*
		 * Linux won't read less than a blocksize when O_DIRECT
		 * is set, even at the end of the file. This means data
		 * at the end of a file beyond the last SAM_DEV_BSIZE_LINUX
		 * offset can't be read with O_DIRECT set.
		 * If the current offset is less than SAM_DEV_BSIZE_LINUX
		 * from the end of the file, clear the O_DIRECT flag so the data
		 * can be read.
		 */
		fp->f_flags &= ~O_DIRECT;
		restoreodirect = 1;
		doingodirect = 0;
	}

	ccount = count;
	cbuf = buf;

again:

	rval = samqfs_generic_file_read(fp, cbuf, ccount, ppos);

	if ((rval < ccount) && (rval > 0) && doingodirect) {
		/*
		 * Linux can't cross a device boundary
		 * which can happen during direct IO.
		 * This results in an incomplete IO operation.
		 * Continue the operation at the new offset.
		 */
		cbuf += rval;
		ccount -= rval;
		rval_total += rval;

		goto again;

	} else {

		if (rval < 0) {
			/*
			 * Error from generic_file_read
			 */
			rval_total = rval;

		} else {
			/*
			 * Bytes read
			 */
			rval_total += rval;
		}
	}

	if (restoreodirect) {
		/*
		 * Since we are doing direct IO
		 * restore the O_DIRECT flag and
		 * remove the page(s) just read.
		 */
		fp->f_flags |= O_DIRECT;
		if (rval_total > 0) {
			truncate_inode_pages(li->i_mapping, offset);
		}
	}

	if (rval >= 0 && !(ip->mp->mt.fi_mflag & MS_RDONLY)) {
		mutex_enter(&ip->fl_mutex);
		sam_mark_ino(ip, (SAM_ACCESSED));
		mutex_exit(&ip->fl_mutex);
	}

out:
	RW_UNLOCK_OS(&ip->inode_rwl, lockstate);
	TRACE(T_SAM_READ_RET, li, ip->di.id.ino, 0,
	    rval_total < 0 ? -rval_total : 0);
	return (rval_total);
}


/*
 * ----- sam_write_vn -
 * Write a SAM-QFS file.
 */

int				/* ERRNO if error, 0 if successful. */
sam_write_vn(struct file *fp, char *buf, size_t count, loff_t *ppos)
{
	int error = 0;
	size_t rval = -ENOSYS;
	struct dentry *de = fp->f_dentry;
	struct inode *li = de->d_inode;
	sam_node_t *ip;
	loff_t offset;
	int ioflag = fp->f_flags;
	sam_u_offset_t allocsz;
	uio_t uio;
	sam_cred_t sam_cred;
	cred_t *credp;
	size_t ccount, rval_total = 0;
	int doingodirect = 0;
	char *cbuf;
	offset_t prev_filesize;

	ip = SAM_LITOSI(li);

	offset = *ppos;
	if (fp->f_flags & O_APPEND) {
		/*
		 * *ppos is zero if O_APPEND is set.
		 */
		offset = rfs_i_size_read(li);
	}

	doingodirect = ioflag & O_DIRECT;

	allocsz = count;

	TRACE(T_SAM_WRITE, li, ip->di.id.ino, (sam_tr_t)offset, (uint_t)count);

	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);

	if (ip->di.rm.info.dk.allocahead) {
		if ((offset + count) > ip->cl_allocsz) {
			allocsz = (ip->di.rm.info.dk.allocahead *
			    SAM_ONE_MEGABYTE);
		}
	}

	if (S_ISREQ(ip->di.mode)) {
		rval_total = -ENOTSUP;
		goto out;
	}

	if (count == 0) {
		rval_total = 0;
		goto out;
	}

	/*
	 * If offline or stage pages exist for the inode, clear inode
	 * file/segment for writing by making file/segment online.
	 */
	sam_set_cred(NULL, &sam_cred);
	credp = (cred_t *)&sam_cred;
	if (ip->di.status.b.offline || ip->flags.b.stage_pages) {
		RW_UNLOCK_OS(&ip->data_rwl, RW_WRITER);
		error = sam_clear_ino(ip, ip->di.rm.size, MAKE_ONLINE, credp);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		RW_LOCK_OS(&ip->data_rwl, RW_WRITER);
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (error) {
			rval_total = -error;
			goto out;
		}
	}

	prev_filesize = ip->di.rm.size;

	while (((error = sam_map_block(ip, offset, (offset_t)allocsz,
	    SAM_ALLOC_BLOCK, NULL, credp)) != 0) &&
	    IS_SAM_ENOSPC(error)) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		error = sam_wait_space(ip, error);
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (error) {
			break;
		}
	}

	uio.uio_loffset = offset;
	uio.uio_resid = count;

	if ((error == ELNRNG) && S_ISREG(ip->di.mode)) {
		sam_set_cred(NULL, &sam_cred);
		credp = (cred_t *)&sam_cred;

		error = sam_get_sparse_blocks(ip, SPARSE_zeroall,
		    &uio, ioflag, credp);
	}
	if (error) {
		rval_total = -error;
		goto out;
	}

	if (!doingodirect && ((uio.uio_loffset + uio.uio_resid) >
	    prev_filesize)) {
		/*
		 * Zero from size to the end of the dau that
		 * contains prev_filesize, zero the dau that contains
		 * uio.uio_loffset and zero the dau that contains
		 * uio.uio_loffset + uio.uio_resid.
		 */
		extern int sam_clear_append_after_map(sam_node_t *ip,
		    offset_t size,
		    offset_t offset, offset_t count);

		error = sam_clear_append_after_map(ip, prev_filesize,
		    uio.uio_loffset, uio.uio_resid);
		if (error) {
			rval_total = -error;
			goto out;
		}
	}

	ccount = count;
	cbuf = buf;

again:

	while (((rval = samqfs_generic_file_write(fp,
	    cbuf, ccount, ppos)) < 0) &&
	    IS_SAM_ENOSPC(-rval)) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		RW_UNLOCK_CURRENT_OS(&ip->data_rwl);
		error = sam_wait_space(ip, rval);
		RW_LOCK_OS(&ip->data_rwl, RW_WRITER);
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (error) {
			rval_total = -error;
			goto done;
		}
	}

	if ((rval < ccount) && (rval > 0) && doingodirect) {
		/*
		 * Linux can't cross a device boundary
		 * which can happen during direct IO.
		 * This results in an incomplete IO operation.
		 * Continue the operation at the new offset.
		 */
		cbuf += rval;
		ccount -= rval;
		rval_total += rval;

		goto again;

	} else {

		if (rval < 0) {
			/*
			 * Error from generic_file_write
			 */
			rval_total = rval;

		} else {
			/*
			 * Bytes written
			 */
			rval_total += rval;
		}
	}

	if (doingodirect && ((offset + count) > prev_filesize)) {
		/*
		 * Zero from prev_filesize to offset.
		 */
		void sam_clear_append_post_write(sam_node_t *ip, offset_t size,
		    offset_t offset, offset_t count);

		ccount = count;
		if ((rval_total > 0) && (rval_total < count)) {
			ccount = rval_total;
		}

		sam_clear_append_post_write(ip, prev_filesize,
		    offset, ccount);
	}

done:
	if (ip->flags.b.staging == 0) {
		if (rfs_i_size_read(li) > ip->di.rm.size) {
			ip->size = ip->di.rm.size = rfs_i_size_read(li);
		}
	}

	if (rval_total > 0) {
		sam_mark_ino(ip, (SAM_UPDATED | SAM_CHANGED));
	}
out:
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	TRACE(T_SAM_WRITE_RET, li, ip->di.id.ino, count,
	    rval_total < 0 ? -rval_total : 0);
	return (rval_total);
}


/*
 * ----- samqfs_access_vn -
 * Get access permissions for a SAM-QFS file.
 */

static int			/* ERRNO if error, 0 if successful. */
__samqfs_access_vn(struct inode *li, int mask)
{
	sam_node_t *ip;
	int error;
	int mode;
	sam_cred_t sam_cred;
	cred_t *credp;

	TRACE(T_SAM_ACCESS, li, mask, 0, 0);

	sam_set_cred(NULL, &sam_cred);
	credp = (cred_t *)&sam_cred;

	ip = SAM_LITOSI(li);

	/*
	 * mask is "other" position permission bits
	 * sam_access_ino wants "owner" position bits for mode
	 */
	mode = mask << 6;
	error = sam_access_ino(ip, mode, FALSE, credp);

	TRACE(T_SAM_ACCESS_RET, li, ip->di.mode, error, ip->di.id.ino);
	if (error) {
		return (-error);
	} else {
		return (0);
	}
}


#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
int				/* ERRNO if error, 0 if successful. */
samqfs_access_vn(struct inode *li, int mask)
{
	return (__samqfs_access_vn(li, mask));
}
#endif


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
int				/* ERRNO if error, 0 if successful. */
samqfs_access_vn(struct inode *li, int mask, struct nameidata *nd)
{
	return (__samqfs_access_vn(li, mask));
}
#endif


/*
 * ----- sam_readdir_vn - Return a SAM-QFS directory.
 * Read the directory and return the entries in the
 * file system independent format. NOTE, inode data RWLOCK held.
 */
int				/* ERRNO if error, 0 if successful. */
sam_readdir_vn(
	struct inode *li,	/* directory inode to be returned */
	cred_t *credp,		/* credentials pointer */
	filldir_t filldir,	/* Copyout function */
	void *d,		/* getdents callback buffer */
	struct file *fp,	/* file pointer */
	int *ret_count)		/* Returned count */
{
	sam_node_t *ip;
	int error;

	ip = SAM_LITOSI(li);

	/*
	 * Quit if we're at EOF or link count is zero (posix).
	 */
	if ((fp->f_pos >= ip->di.rm.size) || ((int)ip->di.nlink <= 0)) {
		return (0);
	}
	if ((error = sam_getdents(ip, credp, filldir, d, fp, ret_count)) == 0) {
		if (!(ip->mp->mt.fi_mflag & MS_RDONLY)) {
			RW_LOCK_OS(&ip->inode_rwl, RW_READER);
			mutex_enter(&ip->fl_mutex);
			sam_mark_ino(ip, (SAM_ACCESSED));
			mutex_exit(&ip->fl_mutex);
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		}
		if (SAM_THREAD_IS_NFS()) {
			ip->nfs_ios++;
		}
	}

	return (error);
}


/*
 * ----- sam_readlink_vn - Read a SAM-QFS symbolic link.
 * Return the symbolic link name.
 */
int
sam_readlink_vn(
	struct inode *li,	/* pointer to i-node. */
	uio_t *uiop,		/* User I/O vector array. */
	cred_t *credp)		/* credentials pointer. */
{
	int r_val = 0;
	sam_node_t *ip;

	ip = SAM_LITOSI(li);

	TRACE(T_SAM_READLINK, li, ip->di.id.ino, (sam_tr_t)uiop->uio_loffset,
	    (sam_tr_t)uiop->uio_resid);

	if (ip->di.version >= SAM_INODE_VERS_2) {
		if ((ip->di.ext_attrs & ext_sln) == 0) {
			dcmn_err((CE_WARN, "SAM-QFS: %s: symlink has "
			    "zero ext_sln flag",
			    ip->mp->mt.fi_name));
		}
		ASSERT(ip->di.ext_attrs & ext_sln);

		/* Retrieve symlink name from inode extension */
		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		r_val = sam_get_symlink(ip, uiop);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	}

	TRACE(T_SAM_READLINK_RET, li, ip->di.id.ino,
	    (sam_tr_t)uiop->uio_loffset, (sam_tr_t)uiop->uio_resid);

	return (r_val);
}
