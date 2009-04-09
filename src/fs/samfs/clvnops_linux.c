/*
 * ----- clvnops_linux.c - Process the Linux inode functions
 *
 * Processes the inode functions supported on the SAM-QFS File System.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#ifdef sun
#pragma ident "$Revision: 1.108 $"
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
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
#include <linux/iobuf.h>
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#include <linux/buffer_head.h>
#include <linux/writeback.h>
#include <linux/mpage.h>
#endif


#include <asm/param.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#include <asm/statfs.h>
#include <linux/namei.h>
#endif


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
#include "clextern.h"
#include "debug.h"
#include "trace.h"
#include "rwio.h"
#include "kstats_linux.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))

typedef dev_t qfs_mknod_dev_t;
int samqfs_get_block_reader(struct inode *li, sector_t iblk,
			    struct buffer_head *bh, int flag);
int samqfs_get_block_writer(struct inode *li, sector_t iblk,
			    struct buffer_head *bh, int flag);

int samqfs_get_blocks_reader(struct inode *li, sector_t iblk,
				unsigned long maxblocks,
				struct buffer_head *bh, int flag);
int samqfs_get_blocks_writer(struct inode *li, sector_t iblk,
				unsigned long maxblocks,
				struct buffer_head *bh, int flag);

#if defined(__x86_64) && \
	(LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 5))
#define	QFS_VM_WRITE	(VM_WRITE)
#else
#define	QFS_VM_WRITE	(VM_WRITE|VM_MAYWRITE)
#endif

/*
 * Figure out how many args are used in the 'flush' operation
 * of struct file_operations.
 *
 * FOP_FLUSHARGS == 1: Use one-arg form.
 * FOP_FLUSHARGS == 2: Use two-arg form.
 */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 16))
/* SLES9 and RHEL4 */
#define	FOP_FLUSHARGS 1
#endif

#if SUSE_LINUX && (LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 16))
/* SLES10 */
#if defined(SLES10FCS) || defined(SLES10SP1)
#define	FOP_FLUSHARGS 1
#else
/* SLES10/SP2 and later */
#define	FOP_FLUSHARGS 2
#endif
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18))
#define	FOP_FLUSHARGS 2
#endif

#endif /* 2.6 */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))

typedef int qfs_mknod_dev_t;
int samqfs_get_block_reader(struct inode *li, long iblk,
			    struct buffer_head *bh, int flag);
int samqfs_get_block_writer(struct inode *li, long iblk,
			    struct buffer_head *bh, int flag);

#define	QFS_VM_WRITE	(VM_WRITE|VM_MAYWRITE)

#endif /* 2.4 */

struct vm_operations_struct samqfs_vm_ops;

/*
 * ----- samqfs_client_open_vn -
 *	Open a SAM-QFS shared file.
 */

int				/* ERRNO if error, 0 if successful. */
samqfs_client_open_vn(
	struct inode *li,	/* pointer to inode */
	struct file *fp)	/* file pointer */
{
	int error = 0;
	uint32_t filemode;
	sam_node_t *ip = SAM_LITOSI(li);
	sam_cred_t sam_cred;
	cred_t *credp;

	if (S_ISSEGI(&ip->di)) {
		/* linux client doesn't support segmented files */
		return (-ENOTSUP);
	}

	filemode = fp->f_flags;
	TRACE(T_SAM_CL_OPEN, li, ip->di.id.ino, filemode, ip->di.status.bits);

	if ((error = sam_open_operation(ip)) != 0) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (-error);
	}
	RW_LOCK_OS(&ip->inode_rwl, RW_READER);

	if (ip->di.id.ino == SAM_INO_INO) {
		error = EPERM;
		goto out_err_unlock;
	}
	if (ip->di.status.b.damaged) {
		error = ENODATA;
		goto out_err_unlock;
	}
	if (S_ISREQ(ip->di.mode)) {
		/* Linux client can't be metadata server (yet) */
		error = ENOTSUP;
		goto out_err_unlock;
	}

	ip->zero_end = 0;

	/*
	 * The default mode disallows archiving while the file is opened for
	 * write. If the attribute, concurrent_archive (archive -c) is set,
	 * the file is archived while opened for write. Used by databases.
	 */
	if ((ip->di.status.b.concurrent_archive == 0) &&
	    (filemode & (O_CREAT | O_TRUNC | O_WRONLY | O_RDWR))) {
		mutex_enter(&ip->fl_mutex);
		ip->flags.b.write_mode = 1;	/* File opened for write */
		mutex_exit(&ip->fl_mutex);
	}

	sam_set_cred(NULL, &sam_cred);
	credp = (cred_t *)&sam_cred;

	/*
	 * If the file is not currently opened (by any process on this host),
	 * request an open lease.  This is used by the server to track which
	 * files are in use by clients.
	 *
	 * If the file mode indicates that the file is being opened for read,
	 * the server will issue a read lease in addition to the open lease,
	 * under the assumption that the file will soon be read.
	 */
	if (S_ISREG(ip->di.mode)) {
		/* Check mandatory locking */
		if ((ip->di.mode & S_ISGID) && !(ip->di.mode & S_IXGRP)) {
			error = EACCES;
			goto out_err_unlock;
		}
		if (ip->no_opens == 0) {
			sam_lease_data_t data;

			bzero(&data, sizeof (data));
			data.ltype = LTYPE_open;
			data.alloc_unit = ip->cl_alloc_unit;
			data.filemode = filemode;
			data.shflags.b.directio = ip->flags.b.directio;
			data.shflags.b.abr = ip->flags.b.abr;
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			error = sam_proc_get_lease(ip, &data, NULL, NULL,
			    SHARE_wait, credp);
			RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		}
	}

	mutex_enter(&ip->fl_mutex);
	if (error == 0) {
		ip->no_opens++;
	} else if ((ip->no_opens == 0) && (ip->pending_mmappers == 0) &&
	    (ip->last_unmap == 0) && (ip->mm_pages == 0) &&
	    S_ISREG(ip->di.mode)) {
		/*
		 * If error on open and no mmap pages, update
		 * inode on the server and cancel lease. LEASE_remove waits
		 * for the response.
		 * This allows the allocated pages to be released.
		 * Set last_unmap to prevent a lease race with new mmappers.
		 */
		ip->last_unmap = 1;
		mutex_exit(&ip->fl_mutex);

		error = sam_proc_rm_lease(ip, CL_CLOSE, RW_READER);

		mutex_enter(&ip->fl_mutex);
		ASSERT(ip->pending_mmappers == 0);
		ASSERT(ip->mm_pages == 0);
		ASSERT(ip->last_unmap == 1);
		ip->last_unmap = 0;
	}
	mutex_exit(&ip->fl_mutex);

out_err_unlock:
	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	TRACE(T_SAM_CL_OPEN_RET, li, (sam_tr_t)ip->di.rm.size, ip->no_opens,
	    error);
	SAM_CLOSE_OPERATION(ip->mp, error);
	return (error);
}


/*
 * ----- samqfs_client_close_vn -
 *	Close a SAM-QFS shared file.
 */

int				/* ERRNO if error, 0 if successful. */
samqfs_client_close_vn(
	struct inode *li,	/* pointer to Inode. */
	struct file *fp)	/* file pointer. */
{
	int error = 0;
	uint32_t filemode;
	sam_node_t *ip = SAM_LITOSI(li);
	sam_cred_t sam_cred;
	cred_t *credp;
	boolean_t last_close = FALSE;
	boolean_t last_unmap = FALSE;

	TRACE(T_SAM_CL_CLOSE, li, ip->di.id.ino, ip->cl_leases, ip->flags.bits);

	if ((error = sam_open_operation(ip)) != 0) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (-error);
	}

	filemode = fp->f_flags;
	sam_set_cred(NULL, &sam_cred);
	credp = (cred_t *)&sam_cred;

	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);

	if (--ip->no_opens < 0) {
		ip->no_opens = 0;
	}
	if ((ip->no_opens == 0) && (ip->cl_closing == 0) &&
	    (ip->pending_mmappers == 0)) {
		ip->cl_closing = 1;
		last_close = TRUE;
	}

	/*
	 * On last reference close for a regular file, sync pages on the client.
	 */
	if (last_close) {
		/*
		 * Flush and invalidate any dirty data buffers.
		 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
		error = rfs_write_inode_now(SAM_SITOLI(ip), 0);
		invalidate_inode_pages(SAM_SITOLI(ip)->i_mapping);
#else
		error = fsync_inode_data_buffers(SAM_SITOLI(ip));
		invalidate_inode_pages(SAM_SITOLI(ip));
#endif
	}

	/*
	 * Prevent a lease race with any new mmappers.
	 */
	if (last_close && (ip->pending_mmappers == 0) &&
	    (ip->last_unmap == 0) && (ip->mm_pages == 0)) {
		ip->last_unmap = 1;
		last_unmap = TRUE;
	}

	/*
	 * On last close and no mmap pages, update inode
	 * on the server and cancel lease. LEASE_remove waits for the response.
	 */
	if (last_close && last_unmap && S_ISREG(ip->di.mode)) {
		error = sam_proc_rm_lease(ip, CL_CLOSE, RW_WRITER);
		if (error == 0) {
			ip->flags.b.staging = 0;
		}
	}
	if (last_close) {
		ip->cl_closing = 0;
	}
	if (last_unmap) {
		ASSERT(ip->pending_mmappers == 0);
		ASSERT(ip->mm_pages == 0);
		ASSERT(ip->last_unmap == 1);
		ip->last_unmap = 0;
	}

	TRACE(T_SAM_CL_CLOSE_RET, li, ip->cl_leases, ip->di.rm.size, error);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	SAM_CLOSE_OPERATION(ip->mp, error);
	return (error);
}


/*
 * ----- samqfs_client_read_vn -
 *	Read a SAM-QFS file.
 */

ssize_t
samqfs_client_read_vn(struct file *fp, char *buf, size_t count, loff_t *ppos)
{
	struct dentry *de = fp->f_dentry;
	struct inode *li = de->d_inode;
	sam_node_t *ip;
	boolean_t using_lease = FALSE;
	sam_cred_t sam_cred;
	cred_t *credp;
	int error = 0;
	ssize_t res = 0;

	ip = SAM_LITOSI(li);
	TRACE(T_SAM_CL_READ, li, ip->di.id.ino, *ppos, 0);
	SAM_DEBUG_COUNT64(nreads);

	/*
	 * Solaris acquires this data_rwl via a call to VOP_RWLOCK().
	 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	if (fp->f_flags & O_DIRECT)
		rfs_lock_inode(li);
#endif
	RW_LOCK_OS(&ip->data_rwl, RW_READER);

	if (MANDLOCK_OS(ip, ip->di.mode)) {
		error = -EACCES;
		goto out;
	}

	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		error = -error;
		goto out;
	}

	sam_set_cred(NULL, &sam_cred);
	credp = (cred_t *)&sam_cred;

	/*
	 * For a regular file, get a read lease.
	 * The lease will be cancelled at close or it will expire automatically.
	 */
	if (S_ISREG(ip->di.mode)) {
		sam_lease_data_t data;

		bzero(&data, sizeof (data));

		mutex_enter(&ip->ilease_mutex);
		ip->cl_leaseused[LTYPE_read]++;
		mutex_exit(&ip->ilease_mutex);
		using_lease = TRUE;

		data.ltype = LTYPE_read;
		data.lflag = 0;
		data.sparse = SPARSE_none;
		data.offset = *ppos;
		data.resid = 0;
		data.alloc_unit = ip->cl_alloc_unit;
		data.filemode = FREAD;
		data.cmd = 0;
		data.shflags.b.directio = ip->flags.b.directio;
		data.shflags.b.abr = ip->flags.b.abr;
		error = sam_proc_get_lease(ip, &data, NULL, NULL, SHARE_wait,
		    credp);
	}

	if (error == 0) {
		res = sam_read_vn(fp, buf, count, ppos);
		/*
		 * res >= 0 is a bytes read count
		 * res < 0 is a -ERRNO
		 */
		if (res < 0) {
			error = -res;
		}
	}
	if (using_lease) {
		mutex_enter(&ip->ilease_mutex);
		ip->cl_leaseused[LTYPE_read]--;
		if ((ip->cl_leaseused[LTYPE_read] == 0) &&
		    (ip->cl_leasetime[LTYPE_read] <= lbolt)) {
			sam_sched_expire_client_leases(ip->mp, 0, FALSE);
		}
		mutex_exit(&ip->ilease_mutex);
	}

	SAM_CLOSE_OPERATION(ip->mp, error);
out:
	RW_UNLOCK_OS(&ip->data_rwl, RW_READER);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	if (fp->f_flags & O_DIRECT)
		rfs_unlock_inode(li);
#endif
	if (error == 0) {
		return (res);
	} else {
		return (error);
	}
}


/*
 * ----- samqfs_client_write_vn - Write a SAM-FS file.
 *	Write a SAM-QFS file.
 */

static ssize_t
samqfs_client_write_vn(struct file *fp, const char *buf, size_t count,
    loff_t *ppos)
{
	struct dentry *de = fp->f_dentry;
	struct inode *li = de->d_inode;
	sam_node_t *ip;
	int error = 0;
	loff_t offset;
	boolean_t using_lease = FALSE;
	boolean_t appending = FALSE;
	sam_cred_t sam_cred;
	cred_t *credp;
	ssize_t res = 0;

	ip = SAM_LITOSI(li);
	offset = *ppos;
	if (fp->f_flags & O_APPEND) {
		/*
		 * *ppos is zero if O_APPEND is set.
		 */
		offset = rfs_i_size_read(li);
	}

	TRACE(T_SAM_CL_WRITE, li, ip->di.id.ino, (sam_tr_t)offset,
	    (uint_t)count);
	SAM_DEBUG_COUNT64(nwrites);

	/*
	 * Solaris acquires this data_rwl via a call to VOP_RWLOCK().
	 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	rfs_lock_inode(li);
#else
	if (!(fp->f_flags & O_DIRECT)) {
		rfs_lock_inode(li);
	}
#endif
	RW_LOCK_OS(&ip->data_rwl, RW_WRITER);

	if (SAM_PRIVILEGE_INO(ip->di.version, ip->di.id.ino)) {
		error = -EPERM;
		goto out;
	}
	if (count == 0) {
		goto out;
	}
	if (MANDLOCK_OS(ip, ip->di.mode)) {
		error = -EACCES;
		goto out;
	}

	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		error = -error;
		goto out;
	}

	sam_set_cred(NULL, &sam_cred);
	credp = (cred_t *)&sam_cred;

	/*
	 * For a regular file, get a write or write append lease.
	 * The lease will be cancelled at close or it will expire automatically.
	 */
	if (S_ISREG(ip->di.mode)) {
		sam_lease_data_t data;

		bzero(&data, sizeof (data));
		data.lflag = 0;
		data.sparse = SPARSE_none;
		data.offset = offset;
		data.resid = count;
		data.alloc_unit = ip->cl_alloc_unit;
		data.filemode = FWRITE;
		data.cmd = 0;
		data.shflags.b.directio = ip->flags.b.directio;
		data.shflags.b.abr = ip->flags.b.abr;
		if ((fp->f_flags & O_APPEND) ||
		    ((offset + count) > ip->di.rm.size)) {
			appending = TRUE;
			data.ltype = LTYPE_append;
		} else {
			appending = FALSE;
			data.ltype = LTYPE_write;
		}

		/*
		 * Prevent existing or new lease from being relinquished until
		 * this write is complete.
		 * Note: We'll hold onto the lease even if we enter
		 * sam_wait_space.
		 * In this case the server may forcibly expire it.
		 * That's OK, we
		 * will reacquire the lease when space is available.
		 */
		mutex_enter(&ip->ilease_mutex);
		ip->cl_leaseused[LTYPE_write]++;
		if (appending) {
			ip->cl_leaseused[LTYPE_append]++;
		}
		mutex_exit(&ip->ilease_mutex);
		using_lease = TRUE;

		error = sam_proc_get_lease(ip, &data, NULL, NULL, SHARE_wait,
		    credp);
		/* XXX - Do we need to check again for writing past EOF? */
	}
	if (error == 0) {
		res = sam_write_vn(fp, (char *)buf, count, ppos);
		/*
		 * res >= 0 is a bytes written count
		 * res < 0 is a -ERRNO
		 */
		if (res < 0) {
			error = -res;
		}
	}
	if (using_lease) {
		mutex_enter(&ip->ilease_mutex);
		ip->cl_leaseused[LTYPE_write]--;
		if (appending) {
			ip->cl_leaseused[LTYPE_append]--;
		}
		if (((ip->cl_leaseused[LTYPE_write] == 0) &&
		    (ip->cl_leasetime[LTYPE_write] <= lbolt)) ||
		    (appending &&
		    (ip->cl_leaseused[LTYPE_append] == 0) &&
		    (ip->cl_leasetime[LTYPE_append] <= lbolt))) {
			sam_sched_expire_client_leases(ip->mp, 0, FALSE);
		}
		mutex_exit(&ip->ilease_mutex);
	}

	SAM_CLOSE_OPERATION(ip->mp, error);
out:
	RW_UNLOCK_OS(&ip->data_rwl, RW_WRITER);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	rfs_unlock_inode(li);
#else
	if (!(fp->f_flags & O_DIRECT)) {
		rfs_unlock_inode(li);
	}
#endif
	if (error == 0) {
		return (res);
	} else {
		return (error);
	}
}

static struct page *
samqfs_client_nopage(struct vm_area_struct *area, unsigned long address,
    qfs_nopage_arg3_t arg3)
{
	struct inode *li = area->vm_file->f_dentry->d_inode;
	sam_node_t *ip = SAM_LITOSI(li);
	struct page *page;
	enum LEASE_type ltype;
	int64_t offset = 0;
	int64_t length = 0;

	ASSERT(ip->last_unmap == 0);

	if ((area->vm_flags & (VM_MAYSHARE|VM_SHARED)) &&
	    (area->vm_flags & QFS_VM_WRITE)) {
		ltype = LTYPE_write;
	} else {
		ltype = LTYPE_read;
	}
	TRACE(T_SAM_CL_GETPAGE, li, (sam_tr_t)offset, length,
	    ltype == LTYPE_read ? 1 : 2);

	mutex_enter(&ip->ilease_mutex);
	ip->cl_leaseused[ltype]++;

	if (!(ip->cl_leases & (1 << ltype))) {
		sam_lease_data_t data;
		int error;

		mutex_exit(&ip->ilease_mutex);
		bzero(&data, sizeof (data));

		data.ltype = ltype;
		data.lflag = 0;
		data.sparse = SPARSE_none;
		data.offset = offset;
		data.resid = length;
		data.alloc_unit = ip->cl_alloc_unit;
		data.cmd = 0;
		data.filemode = (ltype == LTYPE_read ? FREAD : FWRITE);
		data.shflags.b.directio = 0;

		if ((error = sam_proc_get_lease(ip, &data, NULL, NULL,
		    SHARE_wait, CRED()))) {
			mutex_enter(&ip->ilease_mutex);
			ip->cl_leaseused[ltype]--;
			mutex_exit(&ip->ilease_mutex);
			TRACE(T_SAM_CL_GETPG_RET, li, error, 0, 0)
			return (NULL);
		}
	}
	mutex_exit(&ip->ilease_mutex);

	RW_LOCK_OS(&ip->inode_rwl, RW_READER);
	page = filemap_nopage(area, address, arg3);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);

	SAM_DECREMENT_LEASEUSED(ip, ltype);
	TRACE(T_SAM_CL_GETPG_RET, li, 0, 0, 0);
	return (page);
}


/*
 * ----- samqfs_client_delmap_vn -
 * Delete a file mmaped region
 */
void
samqfs_client_delmap_vn(struct vm_area_struct *vma)
{
	struct dentry *de;
	struct inode *li;
	struct file *fp;
	sam_node_t *ip;
	boolean_t is_write;
	krw_t rw_type = RW_READER;
	unsigned long length;
	int error = 0;
	int pages;
	offset_t offset;
	unsigned long prot;
	unsigned long flags;
	unsigned long last_unmapper = 0;

	/*
	 * Get the file pointer and inode
	 */
	fp = vma->vm_file;
	de = fp->f_dentry;
	li = de->d_inode;

	ip = SAM_LITOSI(li);

	prot	= vma->vm_page_prot.pgprot;
	flags	= vma->vm_flags;
	length	= vma->vm_end - vma->vm_start;
	offset	= vma->vm_pgoff << PAGE_SHIFT;
	pages	= length >> PAGE_SHIFT;

	TRACE(T_SAM_CL_DELMAP, li, (sam_tr_t)offset, length, prot);

	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return;
	}

	is_write = (prot & QFS_VM_WRITE) &&
	    (flags & (VM_SHARED|VM_MAYSHARE));

reenter:
	RW_LOCK_OS(&ip->inode_rwl, rw_type);
	mutex_enter(&ip->fl_mutex);
	if (ip->pending_mmappers) {
		mutex_exit(&ip->fl_mutex);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		delay(hz);
		goto reenter;
	}

	ip->mm_pages -= pages;
	if (ip->mm_pages < 0) {
		ip->mm_pages = 0;
	}
	if ((ip->mm_pages == 0) && (ip->pending_mmappers == 0) &&
	    (ip->last_unmap == 0)) {
		ip->last_unmap = 1;
		last_unmapper = 1;
	}
	if (is_write) {
		ip->wmm_pages -= pages;
	}
	mutex_exit(&ip->fl_mutex);

	if (last_unmapper) {
		/*
		 * On last delete map, sync pages on the client.
		 */
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		rw_type = RW_WRITER;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
		error = rfs_write_inode_now(li, 0);
#else
		error = filemap_fdatasync(li->i_mapping);
		if (error == 0) {
			error = filemap_fdatawait(li->i_mapping);
		}
#endif

		/*
		 * If the file is not open, we were the last reference, so close
		 * the inode.  Otherwise, just relinquish the lease.
		 */
		if (ip->no_opens == 0) {
			error = sam_proc_rm_lease(ip, CL_CLOSE, rw_type);
			if (error == 0) {
				/* XXX what is this for? */
				ip->flags.b.staging = 0;
			}
			ASSERT(ip->mm_pages == 0);
			ASSERT(ip->pending_mmappers == 0);
			ASSERT(ip->last_unmap == 1);
			ip->last_unmap = 0;
		} else {
			RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
			sam_client_remove_leases(ip, CL_MMAP, 0);
			(void) sam_proc_relinquish_lease(ip, CL_MMAP,
			    FALSE, ip->cl_leasegen);
			RW_LOCK_OS(&ip->inode_rwl, RW_READER);
			mutex_enter(&ip->fl_mutex);
			ASSERT(ip->mm_pages == 0);
			ASSERT(ip->pending_mmappers == 0);
			ASSERT(ip->last_unmap == 1);
			ip->last_unmap = 0;
			mutex_exit(&ip->fl_mutex);
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			goto out;
		}
	}
	RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
out:
	TRACE(T_SAM_CL_DELMAP_RET, li, pages, ip->mm_pages, error);

	SAM_CLOSE_OPERATION(ip->mp, error);
}


static int
samqfs_client_addmap_vn(struct file *fp, struct vm_area_struct *vma)
{
	struct dentry *de = fp->f_dentry;
	struct inode *li = de->d_inode;
	sam_node_t *ip;
	int error;
	int pages;
	boolean_t is_write;
	unsigned long length;
	offset_t offset;
	unsigned long prot;
	unsigned long flags;
	cred_t *credp;
	sam_cred_t sam_cred;
	sam_lease_data_t *data;
	offset_t orig_resid;
	offset_t prev_size;
	uint32_t ltype;


	ip = SAM_LITOSI(li);

	prev_size = ip->size;
	prot = vma->vm_page_prot.pgprot;
	flags = vma->vm_flags;

	length = vma->vm_end - vma->vm_start;
	pages = length >> PAGE_SHIFT;
	offset = vma->vm_pgoff << PAGE_SHIFT;

	TRACE(T_SAM_CL_ADDMAP, li, (sam_tr_t)offset, length, prot);
	/*
	 * Get memory-mapping lease based on requested page permissions
	 * and sharing type.  (Private mappings are always read-only
	 * from the file system's perspective; pages which are written
	 * become anonymous.)
	 */
	is_write = (prot & QFS_VM_WRITE) &&
	    (flags & (VM_SHARED|VM_MAYSHARE));

	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (-error);
	}

	data = kmem_zalloc(sizeof (sam_lease_data_t), KM_SLEEP);
	if (data == NULL) {
		error = -ENOMEM;
		goto outerr;
	}

reenter:
	RW_LOCK_OS(&ip->inode_rwl, RW_READER);
	mutex_enter(&ip->fl_mutex);
	if (ip->last_unmap) {
		ASSERT(ip->pending_mmappers == 0);
		ASSERT(ip->mm_pages == 0);
		mutex_exit(&ip->fl_mutex);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		delay(hz);
		goto reenter;
	}

	/*
	 * Serialize mmappers.
	 */
	if (ip->pending_mmappers || ip->cl_closing) {
		mutex_exit(&ip->fl_mutex);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		delay(hz);
		goto reenter;
	}

	/*
	 * We set pending_mmappers as a bridge between the point where
	 * we get the CL_MMAP lease and where we set mm_pages.  This stops
	 * anyone from releasing our lease via CL_CLOSE before
	 * we can set mm_pages.
	 */
	ip->pending_mmappers++;
	mutex_exit(&ip->fl_mutex);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);

	sam_set_cred(NULL, &sam_cred);
	credp = (cred_t *)&sam_cred;

	data->lflag = 0;
	data->offset = offset;
	data->resid = length;
	orig_resid = length;
	data->alloc_unit = ip->cl_alloc_unit;
	data->shflags.b.directio = ip->flags.b.directio;
	data->shflags.b.abr = ip->flags.b.abr;
	data->ltype = LTYPE_mmap;
	if (is_write) {
		ltype = LTYPE_write;
		/*
		 * Force blocks to be allocated and zeroed
		 */
		data->sparse = SPARSE_zeroall;
		data->filemode = FWRITE;
	} else {
		ltype = LTYPE_read;
		data->sparse = SPARSE_none;
		data->filemode = FREAD;
	}
	mutex_enter(&ip->ilease_mutex);
	ip->cl_leaseused[ltype]++;
	mutex_exit(&ip->ilease_mutex);

	for (;;) {
		error = sam_get_zerodaus(ip, data, credp);
		if (error == 0) {
			if (is_write) {
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);

				error = sam_process_zerodaus(ip, data, credp);

				if (error == 0) {
					orig_resid -= data->resid;
					if (orig_resid <= 0) {
						if (offset > prev_size) {
							/*
							 * Zero the DAUs between
							 * size and offset.
							 */
							sam_zero_sparse_blocks(
							    ip,
							    prev_size, offset);
						}
						break;
					}
					data->offset += data->resid;
					data->resid = orig_resid;
					RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
					continue;
				}
				if (error) {
					RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
					if (error == ELNRNG) {
						cmn_err(CE_WARN,
				"SAM-QFS: %s: mmap ELNRNG error: ino %d.%d, "
				"off %llx len %llx",
						    ip->mp->mt.fi_name,
						    ip->di.id.ino,
						    ip->di.id.gen, offset,
						    (long long)length);
						SAMFS_PANIC_INO(
						    ip->mp->mt.fi_name,
						    "SAM-QFS: mmap ELNRNG",
						    ip->di.id.ino);
					}
				}
			} else {
				RW_LOCK_OS(&ip->inode_rwl, RW_READER);
				mutex_enter(&ip->fl_mutex);
			}
		}
		break;
	}
	SAM_DECREMENT_LEASEUSED(ip, ltype);

	if (error == 0) {
		ip->mm_pages += pages;
		ip->pending_mmappers--;
		if (is_write) {
			ip->wmm_pages += pages;
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		} else {
			mutex_exit(&ip->fl_mutex);
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		}
	} else {
		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		mutex_enter(&ip->fl_mutex);
		ip->pending_mmappers--;
		mutex_exit(&ip->fl_mutex);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	}
	kmem_free(data, sizeof (sam_lease_data_t));

	if (error == 0) {

		error = generic_file_mmap(fp, vma);

		if (error < 0) {
			error = -error;
		}

		/*
		 * Replace vm_ops with one that has close defined.
		 */
		vma->vm_ops = &samqfs_vm_ops;
	}

outerr:
	TRACE(T_SAM_CL_ADDMAP_RET, li, pages, ip->mm_pages, error);

	SAM_CLOSE_OPERATION(ip->mp, error);
	return (error);
}


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
static int
samqfs_client_flush(struct file *fp)
{
	struct inode *li = fp->f_dentry->d_inode;
	int error = 0;

	error = fsync_inode_data_buffers(li);
	if (error > 0) {
		error = -error;
	}
	return (error);
}
#endif


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
static int
__samqfs_client_flush(struct file *fp)
{
	struct inode *li = fp->f_dentry->d_inode;
	sam_node_t *ip = SAM_LITOSI(li);
	int error = 0;

	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	error = rfs_write_inode_now(li, 0);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

	if (error > 0) {
		error = -error;
	}
	return (error);
}

#if (FOP_FLUSHARGS == 1)
static int
samqfs_client_flush(struct file *fp)
{
	return (__samqfs_client_flush(fp));
}
#endif

#if (FOP_FLUSHARGS == 2)
static int
samqfs_client_flush(struct file *fp, fl_owner_t id)
{
	return (__samqfs_client_flush(fp));
}
#endif
#endif /* >= 2.6.0 */


/*
 * ----- samqfs_client_fsync_vn - Flush this inode to disk.
 * Synchronize the inode's in-memory state on disk.
 */
int
samqfs_client_fsync_vn(struct file *fp, struct dentry *de, int ds)
{
	struct inode *li = de->d_inode;
	sam_node_t *ip = SAM_LITOSI(li);
	int error;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	error = rfs_write_inode_now(li, 0);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
#else
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	error = fsync_inode_data_buffers(li);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
#endif

	if (!(li->i_state & I_DIRTY)) {
		return (error);
	}

	if (ds && !(li->i_state & I_DIRTY_DATASYNC)) {
		return (error);
	}

	error = sam_update_shared_ino(ip, SAM_SYNC_ALL, FALSE);

	if (error > 0) {
		return (-error);
	} else {
		return (error);
	}
}


/*
 * ----- samqfs_client_frlock_vn - File and record locking.
 *  Ask the server host to lock the file or record.
 */

int
samqfs_client_frlock_vn(struct file *fp, int cmd, struct file_lock *lflp)
{
	struct inode *li = fp->f_dentry->d_inode;
	loff_t offset = fp->f_pos;
	mode_t filemode = fp->f_mode;
	sam_flock_t	flock;
	sam_cred_t sam_cred;
	cred_t *credp;
	sam_node_t *ip;
	int error = 0;

	TRACE(T_SAM_CL_FRLOCK, li, cmd, lflp->fl_type, 0);
	ip = SAM_LITOSI(li);

	if (MANDLOCK_OS(ip, ip->di.mode)) {
		return (-EACCES);
	}

	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (-error);
	}

	sam_set_cred(NULL, &sam_cred);
	credp = (cred_t *)&sam_cred;

	flock.l_type = lflp->fl_type;
	flock.l_whence = 0;
	flock.l_start = lflp->fl_start;
	if (lflp->fl_end == OFFSET_MAX) {
		flock.l_len = 0;
	} else {
		flock.l_len = lflp->fl_end - lflp->fl_start + 1;
	}
	flock.l_pid = lflp->fl_pid;
	if ((lflp->fl_start == 0) && (lflp->fl_end == OFFSET_MAX) &&
	    (lflp->fl_type == F_UNLCK)) {
		cmd = 0;
		/*
		 * cmd = 0 for QFS means that all locks on this file owned
		 * by this process will be released
		 */
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 9))
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	if ((lflp->fl_type == F_UNLCK) &&
	    ((cmd == F_SETLK) || (cmd == F_SETLKW) || (cmd == 0))) {
		if (lflp->fl_flags & FL_FLOCK) {
			error = flock_lock_file_wait(fp, lflp);
		} else {
			error = posix_lock_file_wait(fp, lflp);
		}
		if (error) {
			printk(KERN_WARNING "sam-qfs: kernel locking and "
			    "filesystem locking out of sync "
			    "(unlock), err = %d\n",
			    error);
			TRACE(T_SAM_CL_FRLOCK, li, cmd, lflp->fl_type, error);
		}
	}
	if (!error) {
		error = sam_client_frlock_ino(ip, cmd, &flock, filemode,
		    offset, NULL, credp);
	}
	if ((lflp->fl_type != F_UNLCK) &&
	    ((cmd == F_SETLK) || (cmd == F_SETLKW))) {
		if (lflp->fl_flags & FL_FLOCK) {
			error = flock_lock_file_wait(fp, lflp);
		} else {
			error = posix_lock_file_wait(fp, lflp);
		}
		if (error) {
			printk(KERN_WARNING "sam-qfs: kernel locking and "
			    "filesystem locking out of sync (lock), err = %d\n",
			    error);
			TRACE(T_SAM_CL_FRLOCK, li, cmd, lflp->fl_type, error);
		}
	}
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
#else
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	error = sam_client_frlock_ino(ip, cmd, &flock, filemode,
	    offset, NULL, credp);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
#endif

	if (cmd == F_GETLK) {
		lflp->fl_type = flock.l_type;
		lflp->fl_start = flock.l_start;
		lflp->fl_end = flock.l_start + flock.l_len - 1;
		lflp->fl_pid = flock.l_pid;
	}

	SAM_CLOSE_OPERATION(ip->mp, error);
	return (error);
}


/*
 * ----- samqfs_client_getpage_vn -
 * Get a page of a SAM-QFS file.
 */
static int
samqfs_client_getpage_vn(struct file *fp, struct page *page)
{
	int res;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
	res = rfs_block_read_full_page(page, samqfs_get_block_reader);
#else
	res = mpage_readpage(page, samqfs_get_block_reader);
#endif

	return (res);
}


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18))
static int
samqfs_client_sync_page(struct page *page)
{
	return (block_sync_page(page));
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18))
static void
samqfs_client_sync_page(
struct page *page)
{
	block_sync_page(page);
}
#endif


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))

/*
 * ----- samqfs_client_putpage_vn -
 * Write a page to a SAM-QFS file.
 */
static int
samqfs_client_putpage_vn(struct page *page, struct writeback_control *wbc)
{
	int res = 0;
	int need_unlock = 0;
	sam_node_t *ip = SAM_LITOSI(page->mapping->host);

	/*
	 * Here we're generally called with the inode_rwl as RW_WRITER.
	 * However, we can get into this code without holding any locks in
	 * the following cases:
	 * 2.6 kernel: msync_interval again, and any pressure on the page
	 * cache can eventually end up in a ->writepage.
	 */
	if (!RW_WRITE_HELD_OS(&ip->inode_rwl)) {
		if (wbc->sync_mode != WB_SYNC_NONE) {
			/*
			 * Called for sync--we are required to start I/O
			 * against the page.
			 */
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		} else {
			if (!RW_TRYENTER_OS(&ip->inode_rwl, RW_WRITER)) {
				goto out_busy;
			}
		}
		need_unlock = 1;
	}

	res = block_write_full_page(page, samqfs_get_block_writer, wbc);

	if (need_unlock) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	}

	return (res);

out_busy:
	redirty_page_for_writepage(wbc, page);
	unlock_page(page);
	return (0);
}

static int
samqfs_client_putpages_vn(struct address_space *mapping,
    struct writeback_control *wbc)
{
	int res = 0;
	int need_unlock = 0;
	sam_node_t *ip = SAM_LITOSI(mapping->host);

	if (!RW_WRITE_HELD_OS(&ip->inode_rwl)) {
		if (ip->mp->mt.fi_status & FS_UMOUNT_IN_PROGRESS) {
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		} else if (!RW_TRYENTER_OS(&ip->inode_rwl, RW_WRITER)) {
			goto out_busy;
		}
		need_unlock = 1;
	}

	res = mpage_writepages(mapping, wbc, NULL);

	if (need_unlock) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	}
	return (res);

out_busy:
	wbc->pages_skipped++;	/* indicate lack of progress */
	return (0);
}

#endif /* LINUX_VERSION_CODE >= 2.6.0 */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))

/*
 * ----- samqfs_client_putpage_vn -
 * Write a page to a SAM-QFS file.
 */
static int
samqfs_client_putpage_vn(struct page *page)
{
	int res = 0;
	int need_unlock = 0;
	sam_node_t *ip = SAM_LITOSI(page->mapping->host);

	/*
	 * Here we're generally called with the inode_rwl as RW_WRITER.
	 * However, we can get into this code without holding any locks in
	 * the following cases:
	 * 2.4 kernel: msync_interval and launder_page can call ->writepage
	 * directly and as an initial entry point to QFS.
	 */
	if (!RW_WRITE_HELD_OS(&ip->inode_rwl)) {
		if (!RW_TRYENTER_OS(&ip->inode_rwl, RW_WRITER)) {
			goto out_busy;
		}
		need_unlock = 1;
	}

	res = block_write_full_page(page, samqfs_get_block_writer);

	if (need_unlock) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	}

	return (res);

out_busy:
	SetPageDirty(page);
	unlock_page(page);
	return (0);
}

#endif /* LINUX_VERSION_CODE < 2.6.0 */

static int
samqfs_commit_write(struct file *fp, struct page *page, unsigned from,
    unsigned to)
{
	int error;

	error = generic_commit_write(fp, page, from, to);

	return (error);
}


static int
samqfs_prepare_write(struct file *fp, struct page *page, unsigned from,
    unsigned to)
{
	int error;

	error = block_prepare_write(page, from, to, samqfs_get_block_writer);

	return (error);
}


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18))
ssize_t
samqfs_direct_IO(int rw, struct kiocb *iocb, const struct iovec *iov,
	loff_t offset, unsigned long ns)
{
	struct file *fp = iocb->ki_filp;
	struct inode *li = fp->f_mapping->host;
	ssize_t ret;

	if (rw == READ) {
		ret = blockdev_direct_IO_no_locking(rw, iocb, li,
		    li->i_sb->s_bdev, iov, offset, ns,
		    samqfs_get_block_reader, NULL);
	} else {
		ret = blockdev_direct_IO(rw, iocb, li,
		    li->i_sb->s_bdev, iov, offset, ns,
		    samqfs_get_block_writer, NULL);
	}
	return (ret);
}
#endif /* >= 2.6.18 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)) && \
	(LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18))
ssize_t
samqfs_direct_IO(int rw, struct kiocb *iocb, const struct iovec *iov,
    loff_t offset, unsigned long ns)
{
	struct file *fp = iocb->ki_filp;
	struct inode *li = fp->f_mapping->host;
	ssize_t ret;

	if (rw == READ) {
		ret = blockdev_direct_IO_no_locking(rw, iocb, li,
		    li->i_sb->s_bdev, iov, offset, ns,
		    samqfs_get_blocks_reader, NULL);
	} else {
		ret = blockdev_direct_IO(rw, iocb, li,
		    li->i_sb->s_bdev, iov, offset, ns,
		    samqfs_get_blocks_writer, NULL);
	}
	return (ret);
}
#endif /* >= 2.6.0  &&  < 2.6.18 */


#if defined(RHE_LINUX) && (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
static int
samqfs_direct_IO(int rw, struct file *fp, struct kiobuf *kiobuf,
    unsigned long bnr, int bsize, int ssize, int soffset)
{
	struct inode *li = fp->f_dentry->d_inode->i_mapping->host;
	int ret;

	if (rw == READ) {
		ret = rfs_generic_direct_sector_IO(rw, li, kiobuf, bnr,
		    bsize, ssize,
		    soffset, samqfs_get_block_reader, 1);
	} else {
		ret = rfs_generic_direct_sector_IO(rw, li, kiobuf, bnr,
		    bsize, ssize,
		    soffset, samqfs_get_block_writer, 1);
	}
	return (ret);
}
#endif /* RHE && < 2.6.0 */

static int			/* Symlink length, or -ERRNO if error */
samqfs_client_readlink_vn(struct dentry *de, char *buf, int len)
{
	int error = 0;
	int r_val = 0;
	uio_t uio;
	iovec_t iov;
	sam_cred_t sam_cred;
	cred_t *credp = (cred_t *)&sam_cred;
	struct inode *li = de->d_inode;
	sam_node_t *ip = SAM_LITOSI(li);

	TRACE(T_SAM_CL_READLINK, NULL, ip->di.id.ino, 0, 0);

	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (-error);
	}

	sam_set_cred(NULL, &sam_cred);
	if (!SAM_IS_SHARED_SERVER(ip->mp)) {
		if ((error = sam_refresh_client_ino(ip,
		    ip->mp->mt.fi_meta_timeo, credp)) != 0) {
			goto out;
		}
	}

	/*
	 * We use structs uio and vio only to hold buffer & len info.
	 * Linux doesn't manage those structs as with regular I/O.
	 */
	iov.iov_base = buf;
	iov.iov_len = len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_loffset = 0;
	uio.uio_resid = len;
	uio.uio_segflg = UIO_USERSPACE;

	r_val = sam_readlink_vn(li, &uio, credp);
	if (r_val > 0) {		/* r_val is ERRNO, convert to be < 0 */
		error = -r_val;
	} else {			/* r_val is length, conv. to be >= 0 */
		r_val = -r_val;
	}

out:
	SAM_CLOSE_OPERATION(ip->mp, error);
	if (error == 0) {
		return (r_val);
	}
	return (error);
}


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 8))
static int
samqfs_nd_set_link_err(struct nameidata *nd, int err)
{
	ASSERT(err < 0);
	return (err);
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 8))
static int
samqfs_nd_set_link_err(struct nameidata *nd, int err)
{
	ASSERT(err < 0);
	nd_set_link(nd, ERR_PTR(err));
	return (0);
}
#endif

static int
__samqfs_client_follow_link(struct dentry *de, struct nameidata *nd)
{
	int error = 0;
	int r_val;
	uio_t uio;
	iovec_t iov;
	int len = 1024;			/* What is max sl_name length? */
	char *sl_name = NULL;
	sam_cred_t sam_cred;
	cred_t *credp = (cred_t *)&sam_cred;
	struct inode *li = de->d_inode;
	sam_node_t *ip = SAM_LITOSI(li);

	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		error = samqfs_nd_set_link_err(nd, -error);
		return (error);
	}

	sam_set_cred(NULL, &sam_cred);
	if (!SAM_IS_SHARED_SERVER(ip->mp)) {
		if ((error = sam_refresh_client_ino(ip,
		    ip->mp->mt.fi_meta_timeo, credp)) != 0) {
			error = samqfs_nd_set_link_err(nd, -error);
			goto out;
		}
	}

	sl_name = kmem_zalloc(len * sizeof (*sl_name), KM_SLEEP);
	if (sl_name == NULL) {
		error = samqfs_nd_set_link_err(nd, -ENOMEM);
		goto out;
	}

	/*
	 * We use structs uio and vio only to hold buffer & len info.
	 * Linux doesn't manage those structs as with regular I/O.
	 */
	iov.iov_base = sl_name;
	iov.iov_len = len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_loffset = 0;
	uio.uio_resid = len;
	uio.uio_segflg = UIO_SYSSPACE;

	r_val = sam_readlink_vn(li, &uio, credp);
	if (r_val > 0) {		/* r_val is ERRNO, convert to be < 0 */
		error = samqfs_nd_set_link_err(nd, -r_val);
		kmem_free(sl_name, 0);
	} else {			/* r_val is length, conv. to be >= 0 */
		r_val = -r_val;

		sl_name[r_val] = '\0';
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 8))
		nd_set_link(nd, sl_name);
		error = 0;
#else
		error = vfs_follow_link(nd, sl_name);
		kmem_free(sl_name, 0);
#endif
	}

out:
	SAM_CLOSE_OPERATION(ip->mp, error);
	return (error);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 13)
static int
samqfs_client_follow_link(struct dentry *de, struct nameidata *nd)
{
	return (__samqfs_client_follow_link(de, nd));
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 13)
static void *
samqfs_client_follow_link(struct dentry *de, struct nameidata *nd)
{
	(void) __samqfs_client_follow_link(de, nd);
	return (NULL);
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 8)) && \
	(LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 13))
static void
samqfs_client_put_link(struct dentry *de, struct nameidata *nd)
{
	char *s = nd_get_link(nd);
	if (!IS_ERR(s)) {
		kmem_free(s, 0);
	}
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 13)
static void
samqfs_client_put_link(struct dentry *de, struct nameidata *nd, void *p)
{
	char *s = nd_get_link(nd);
	if (!IS_ERR(s))
		kmem_free(s, 0);
}
#endif


/*
 * ----- samqfs_client_readdir_vn -
 *	Readdir of a SAM-QFS directory.
 */

int
samqfs_client_readdir_vn(struct file *fp, void *d, filldir_t filldir)
{
	struct dentry *lde;
	struct inode *li;
	sam_node_t *ip;
	cred_t *credp;
	sam_cred_t sam_cred;
	int ret_count = 0;

	int error = -ENOSYS;

	lde = fp->f_dentry;
	li = lde->d_inode;
	ip = SAM_LITOSI(li);

	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (-error);
	}

	sam_set_cred(NULL, &sam_cred);
	credp = (cred_t *)&sam_cred;

	if (!SAM_IS_SHARED_SERVER(ip->mp)) {
		error = sam_refresh_client_ino(ip,
		    ip->mp->mt.fi_meta_timeo, credp);
		if (error) {
			goto out;
		}
	}

	if ((error = sam_readdir_vn(li, credp, filldir, d, fp, &ret_count)) ==
	    0) {
		ip->nfs_ios++;
	} else if (error == EDOM) {
		sam_directed_actions(ip,
		    (SR_INVAL_PAGES | SR_STALE_INDIRECT), 0,
		    credp);
		if ((error = sam_readdir_vn(li, credp, filldir, d, fp,
		    &ret_count)) == 0) {
			ip->nfs_ios++;
		}
	}

out:
	SAM_CLOSE_OPERATION(ip->mp, error);

	/*
	 * The error code should possibly be returned here instead of 0.
	 * A check for the special case of the error return from filldir
	 * in sam_getdents would need to be added as it needs a 0 to be
	 * returned here so that the vfs layer can check the error code
	 * in its data block to see if it needs to do further action,
	 * like additional reads because the original buffer was too small
	 * to contain the requested data.
	 */
	return (0);
}


static int			/* ERRNO if error, 0 if successful. */
__samqfs_client_create_vn(struct inode *dir, struct dentry *de, int mode)
{
	sam_node_t *pip;
	struct inode *li = NULL;
	sam_name_create_t create;
	sam_ino_record_t *nrec;
	sam_ino_t inumber = 0;
	int error = 0;

	TRACES(T_SAM_CL_CREATE, dir, de->d_name.name);
	pip = SAM_LITOSI(dir);

	/*
	 * Shared file system does not support pipes and block/char. special
	 * files. Note, the major/minor number may not be the same on all hosts.
	 */
	if (S_ISBLK(mode) || S_ISCHR(mode) || S_ISFIFO(mode)) {
		return (-ENOTSUP);
	}

	if ((error = sam_open_operation(pip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (-error);
	}

	/*
	 * Cannot set sticky bit unless superuser.
	 */
	if (current->fsuid != 0) {
		mode &= ~S_ISVTX;
	}

	/*
	 * Ask the server if file exists. If not, create it on the server.
	 * Never truncate the file's existing blocks on create. May avoid an
	 * expensive allocate append.
	 */

	memset((char *)&create, 0, sizeof (create));
	create.ex = 0;
	create.mode = mode;
	create.flag = 0;
	create.vattr.va_mask = (QFS_AT_MODE|QFS_AT_UID|QFS_AT_GID|QFS_AT_TYPE);
	create.vattr.va_mode = mode;
	create.vattr.va_uid = current->fsuid;
	if (dir->i_mode & S_ISGID) {
		create.vattr.va_gid = dir->i_gid;
	} else {
		create.vattr.va_gid = current->fsgid;
	}
	create.vattr.va_type = VREG;

	nrec = kmem_alloc(sizeof (*nrec), KM_SLEEP);
	error = sam_proc_name(pip, NAME_create, &create, sizeof (create),
	    &de->d_name, NULL, NULL, nrec);

	if (error == 0) {
		sam_node_t *ip;

		error = sam_get_client_ino(nrec, pip, &de->d_name, &ip, NULL);

		if (error == 0) {
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			sam_clear_map_cache(ip);
			(void) sam_stale_indirect_blocks(ip, 0);
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

			li = SAM_SITOLI(ip);
			d_instantiate(de, li);
			inumber = ip->di.id.ino;
		}
	}
	kmem_free(nrec, sizeof (*nrec));

	if (error) {
		rfs_d_drop(de);
	}
	TRACE(T_SAM_CL_CREATE_RET, dir, (sam_tr_t)li, inumber, error);
	SAM_CLOSE_OPERATION(pip->mp, error);
	return (error);
}


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
int				/* ERRNO if error, 0 if successful. */
samqfs_client_create_vn(struct inode *dir, struct dentry *de, int mode)
{
	return (__samqfs_client_create_vn(dir, de, mode));
}
#endif


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
int				/* ERRNO if error, 0 if successful. */
samqfs_client_create_vn(struct inode *dir, struct dentry *de, int mode,
    struct nameidata *ndp)
{
	return (__samqfs_client_create_vn(dir, de, mode));
}
#endif


/*
 * ----- samqfs_client_lookup_vn - Lookup a SAM-QFS file.
 * Given a parent directory and pathname component, lookup
 * and return the vnode found. If found, vnode is "held".
 */

static struct dentry *
__samqfs_client_lookup_vn(struct inode *dir, struct dentry *de)
{
	int error;
	sam_node_t *pip;
	sam_node_t *ip;
	struct inode *li;
	struct dentry *ret;

	TRACES(T_SAM_CL_LOOKUP, dir, de->d_name.name);

	if (de->d_name.len >= MAXNAMELEN) {
		return (ERR_PTR(-ENAMETOOLONG));
	}

	pip = SAM_LITOSI(dir);

	if ((error = sam_open_operation(pip))) {
		return (ERR_PTR(-error));
	}

	error = sam_client_lookup_name(pip,
	    &de->d_name, pip->mp->mt.fi_meta_timeo, 0, &ip, NULL);
	if (error == 0) {
		ASSERT(ip);
		li = SAM_SITOLI(ip);
		TRACE(T_SAM_CL_LOOKUP_RET, dir, (sam_tr_t)li, ip->di.id.ino,
		    ip->di.nlink);
	} else {
		TRACE(T_SAM_CL_LOOKUP_ERR, dir, pip->di.id.ino, error, 0);
		if (error != ENOENT) {
			goto out_err;
		}
		error = 0;
		li = NULL;
	}

	de->d_op = &samqfs_dentry_ops;
	ret = rfs_d_splice_alias(li, de);

	SAM_CLOSE_OPERATION(pip->mp, error);
	return (ret);

out_err:
	SAM_CLOSE_OPERATION(pip->mp, error);
	return (ERR_PTR(error));
}


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
struct dentry *
samqfs_client_lookup_vn(struct inode *dir, struct dentry *de,
    struct nameidata *ndp)
{
	return (__samqfs_client_lookup_vn(dir, de));
}
#endif


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
struct dentry *
samqfs_client_lookup_vn(struct inode *dir, struct dentry *de)
{
	return (__samqfs_client_lookup_vn(dir, de));
}
#endif


/*
 * ----- samqfs_client_link_vn -
 *	Add a hard link to an existing shared file.
 */

int			/* ERRNO if error, 0 if successful. */
samqfs_client_link_vn(struct dentry *olddep, struct inode *pli,
    struct dentry *dep)
{
	sam_name_link_t *linkp;
	sam_ino_record_t *nrec;
	struct inode *li = olddep->d_inode;
	sam_node_t *pip;
	sam_node_t *ip;
	sam_ino_t inumber = 0;
	int error = 0;

	if (S_ISDIR(li->i_mode)) {
		return (-EPERM);
	}

	pip = SAM_LITOSI(pli);
	if ((error = sam_open_operation(pip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (-error);
	}

	ip = SAM_LITOSI(li);

	linkp = kmem_alloc(sizeof (*linkp), KM_SLEEP);
	linkp->id = ip->di.id;
	inumber = ip->di.id.ino;

	nrec = kmem_alloc(sizeof (*nrec), KM_SLEEP);

	error = sam_proc_name(pip, NAME_link, linkp, sizeof (*linkp),
	    &dep->d_name, NULL, NULL, nrec);
	if (error == 0) {
		li->i_nlink++;
	}
	rfs_d_drop(dep);

	kmem_free(nrec, sizeof (*nrec));
	kmem_free(linkp, sizeof (*linkp));

	TRACE(T_SAM_CL_LINK_RET, pli, (sam_tr_t)ip, inumber, error);
	SAM_CLOSE_OPERATION(pip->mp, error);
	return (error);
}


/*
 * ----- samqfs_client_remove_vn -
 *	Remove a shared SAM-QFS file.
 */

int				/* ERRNO if error, 0 if successful. */
samqfs_client_remove_vn(
	struct inode *dir,	/* Pointer to parent directory inode. */
	struct dentry *de)	/* Pointer to the dentry to remove. */
{
	sam_node_t *pip;
	sam_node_t *ip;
	sam_name_remove_t remove;
	int error = 0;
	sam_ino_record_t *nrec;

	TRACES(T_SAM_CL_REMOVE, dir, de->d_name.name);

	pip = SAM_LITOSI(dir);
	if ((error = sam_open_operation(pip))) {
		return (-error);
	}

	ASSERT(de->d_inode);
	ip = SAM_LITOSI(de->d_inode);

	/*
	 * Ask server to remove the file.
	 * Lift lock and resolve conflicts on the server.
	 * Remove this reference from the name cache.
	 */
	remove.id = ip->di.id;

	nrec = kmem_alloc(sizeof (*nrec), KM_SLEEP);

	error = sam_proc_name(pip, NAME_remove,
	    &remove, sizeof (remove), &de->d_name,
	    NULL, NULL, nrec);
	if (error == 0) {
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		error = sam_reset_client_ino(ip, 0, nrec);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	}
	kmem_free(nrec, sizeof (*nrec));

	TRACE(T_SAM_CL_REMOVE_RET, dir, pip->di.id.ino, remove.id.ino, error);
	SAM_CLOSE_OPERATION(pip->mp, error);
	return (error);
}


/*
 * ----- samqfs_client_symlink_vn -
 *	Create symlink with the contents specified by 'sname'.
 */
int
samqfs_client_symlink_vn(struct inode *lpip, struct dentry *dep,
    const char *sname)
{
	sam_name_symlink_t symlink;
	sam_ino_record_t *irp;
	sam_node_t *pip;
	int error;

	TRACES(T_SAM_CL_SYMLINK, lpip, sname);

	if (strlen(sname) + 1 > QFS_MAXPATHLEN) {
		return (-ENAMETOOLONG);
	}

	pip = SAM_LITOSI(lpip);
	if ((error = sam_open_operation(pip))) {
		return (-error);
	}

	memset((char *)&symlink, 0, sizeof (symlink));
	symlink.vattr.va_mask = (QFS_AT_MODE|QFS_AT_UID|QFS_AT_GID|QFS_AT_TYPE);
	symlink.vattr.va_mode = 0777;
	symlink.vattr.va_uid = current->fsuid;
	symlink.vattr.va_gid = current->fsgid;
	symlink.vattr.va_type = VLNK;
	symlink.comp_size = dep->d_name.len;
	symlink.path_size = strlen(sname);

	irp = kmem_alloc(sizeof (*irp), KM_SLEEP);
	error = sam_proc_name(pip, NAME_symlink, &symlink, sizeof (symlink),
	    &dep->d_name, (char *)sname, NULL, irp);

	if (error == 0) {
		sam_node_t *ip;
		struct inode *li;

		error = sam_get_client_ino(irp, pip, &dep->d_name, &ip, NULL);

		if (error == 0) {
			li = SAM_SITOLI(ip);
			d_instantiate(dep, li);
		}
	}
	kmem_free(irp, sizeof (*irp));
	SAM_CLOSE_OPERATION(pip->mp, error);
	TRACE(T_SAM_CL_SYMLN_RET, lpip, 0, error, 0);
	return (error);
}


int
samqfs_client_mknod_vn(struct inode *dir, struct dentry *de, int mode,
    qfs_mknod_dev_t rdev)
{
	int error = -ENOSYS;

	return (error);
}



/*
 * ----- samqfs_client_rename_vn -
 *	Rename a SAM-QFS shared file: Verify the old file exists. Remove
 *	the new file if it exists; hard link the new file to the old file;
 *	remove the old file.
 */

int
samqfs_client_rename_vn(struct inode *odirp, struct dentry *odep,
    struct inode *ndirp, struct dentry *ndep)
{
	sam_name_rename_t rename;
	sam_ino_record_t *irp;
	sam_node_t *opip;
	sam_node_t *npip;
	int error;

	opip = SAM_LITOSI(odirp);
	npip = SAM_LITOSI(ndirp);

	TRACES(T_SAM_CL_RENAME1, odirp, odep->d_name.name);
	TRACES(T_SAM_CL_RENAME1, ndirp, ndep->d_name.name);

	if ((error = sam_open_operation(opip))) {
		return (-error);
	}

	memset((char *)&rename, 0, sizeof (rename));
	rename.osize = odep->d_name.len;
	rename.nsize = ndep->d_name.len;
	rename.new_parent_id = npip->di.id;

	irp = kmem_alloc(sizeof (*irp), KM_SLEEP);
	error = sam_proc_name(opip, NAME_rename, &rename, sizeof (rename),
	    &odep->d_name, (char *)ndep->d_name.name, NULL, irp);

	rfs_d_drop(odep);
	rfs_d_drop(ndep);
	kmem_free(irp, sizeof (*irp));

	TRACE(T_SAM_CL_RENAME_RET, ndirp, (sam_tr_t)opip, 0, error);
	SAM_CLOSE_OPERATION(opip->mp, error);
	return (error);
}


/*
 * ----- samqfs_client_mkdir_vn -
 *	Make a shared SAM_QFS directory in this file system.
 */

int
samqfs_client_mkdir_vn(struct inode *dir, struct dentry *de, int mode)
{
	sam_node_t *pip;
	sam_node_t *ip;
	struct inode *li = NULL;
	sam_name_mkdir_t mkdir;
	sam_ino_record_t *nrec;
	int error = 0;

	TRACES(T_SAM_CL_MKDIR, dir, de->d_name.name);
	pip = SAM_LITOSI(dir);
	if ((error = sam_open_operation(pip))) {
		goto out;
	}

	if (current->fsuid != 0) {	/* Only superuser can set sticky bit */
		mode &= ~S_ISVTX;
	}

	/*
	 * Ask the server if directory exists. If not, create it on the server.
	 */
	memset((char *)&mkdir, 0, sizeof (mkdir));
	mkdir.vattr.va_mask = (QFS_AT_MODE|QFS_AT_UID|QFS_AT_GID|QFS_AT_TYPE);
	mkdir.vattr.va_uid = current->fsuid;
	if (dir->i_mode & S_ISGID) {
		mkdir.vattr.va_gid = dir->i_gid;
		mode |= S_ISGID;
	} else {
		mkdir.vattr.va_gid = current->fsgid;
	}
	mkdir.vattr.va_mode = mode;
	mkdir.vattr.va_type = VDIR;

	nrec = kmem_alloc(sizeof (*nrec), KM_SLEEP);
	error = sam_proc_name(pip, NAME_mkdir,
	    &mkdir, sizeof (mkdir), &de->d_name,
	    NULL, NULL, nrec);
	if (error == 0) {
		error = sam_get_client_ino(nrec, pip, &de->d_name, &ip, NULL);
		if (error == 0) {
			li = SAM_SITOLI(ip);
			TRACE(T_SAM_CL_MKDIR_RET, dir, (sam_tr_t)li,
			    ip->di.id.ino, ip->di.rm.size);
			d_instantiate(de, li);
		}

	}
	kmem_free(nrec, sizeof (*nrec));

out:
	if (error) {
		rfs_d_drop(de);
	}
	TRACE(T_SAM_CL_MKDIR_ERR, dir, 0, 0, error);
	SAM_CLOSE_OPERATION(pip->mp, error);
	return (error);
}


/*
 * ----- samqfs_client_rmdir_vn -
 *	Remove a shared SAM-QFS directory from this file system.
 */

int
samqfs_client_rmdir_vn(struct inode *dir, struct dentry *de)
{
	sam_node_t *pip;
	sam_node_t *ip;
	sam_name_rmdir_t rmdir;
	int error = -ENOSYS;
	sam_ino_record_t *nrec;

	TRACES(T_SAM_CL_RMDIR, dir, de->d_name.name);

	pip = SAM_LITOSI(dir);
	if ((error = sam_open_operation(pip))) {
		return (-error);
	}

	ip = SAM_LITOSI(de->d_inode);

	/*
	 * Ask server to remove the directory.
	 * Lift lock and resolve conflicts on the server.
	 * Remove this reference from the name cache.
	 */

	rmdir.id = ip->di.id;

	nrec = kmem_alloc(sizeof (*nrec), KM_SLEEP);
	error = sam_proc_name(pip, NAME_rmdir,
	    &rmdir, sizeof (rmdir), &de->d_name, NULL, NULL, nrec);

	if (error == 0) {
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		error = sam_reset_client_ino(ip, 0, nrec);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

		de->d_inode->i_nlink = 0;
	} else if (error == EEXIST) {
		/*
		 * EEXIST is returned from the MDS when
		 * the directory is not empty.
		 */
		error = ENOTEMPTY;
	}
	kmem_free(nrec, sizeof (*nrec));

	TRACE(T_SAM_CL_RMDIR_RET, dir, pip->di.id.ino, rmdir.id.ino, error);
	SAM_CLOSE_OPERATION(pip->mp, error);
	return (error);
}


int
samqfs_client_setattr_vn(struct dentry *dep, struct iattr *iap)
{
	int error = 0;
	sam_node_t *ip;
	sam_inode_setattr_t setattr;
	sam_cred_t sam_cred;
	cred_t *credp;
	struct inode *li = dep->d_inode;

	ip = SAM_LITOSI(li);

	if ((error = sam_open_operation(ip))) {
		return (-error);
	}

	sam_set_cred(NULL, &sam_cred);
	credp = (cred_t *)&sam_cred;

	setattr.flags = 0;
	error = qfs_iattr_to_v32(iap, &setattr.vattr);

	if (iap->ia_valid & ATTR_SIZE) {
		sam_lease_data_t data;

		bzero(&data, sizeof (data));

		RW_LOCK_OS(&ip->data_rwl, RW_WRITER);
		mutex_enter(&ip->cl_apmutex);
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);

		sam_flush_pages(ip, B_INVAL);

		vmtruncate(li, iap->ia_size);

		data.ltype = LTYPE_truncate;
		data.lflag = SAM_TRUNCATE;
		data.sparse = SPARSE_none;
		data.offset = 0;
		data.resid = iap->ia_size;
		data.alloc_unit = ip->cl_alloc_unit;
		data.filemode = O_TRUNC;
		data.cmd = 0;
		data.shflags.b.directio = ip->flags.b.directio;
		data.shflags.b.abr = ip->flags.b.abr;

		error = sam_truncate_shared_ino(ip, &data, credp);

		/*
		 * Since we don't know the previous value of zero_end, we
		 * need to reset it to BOF.
		 */
		if (ip->size < ip->zero_end) {
			ip->zero_end = 0;
		}

		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		mutex_exit(&ip->cl_apmutex);
		RW_UNLOCK_OS(&ip->data_rwl, RW_WRITER);

	} else {

		error = sam_proc_inode(ip, INODE_setattr, &setattr, credp);

	}

	TRACE(T_SAM_CL_SETATT_RET, dep->d_inode,
	    ip->di.rm.size, ip->di.mode, error);
	SAM_CLOSE_OPERATION(ip->mp, error);
	return (error);
}


static int
__samqfs_client_getattr(struct dentry *de)
{
	cred_t *credp;
	sam_node_t *ip;
	int error = 0;
	int meta_timeo;
	struct inode *li;

	li = de->d_inode;
	ip = SAM_LITOSI(li);

	if ((error = sam_open_operation(ip))) {
		return (-error);
	}
	if (ip->di.id.ino != SAM_INO_INO) {
		credp = CRED();

		if ((ip->cl_flags & (SAM_UPDATED | SAM_CHANGED)) &&
		    !(ip->cl_leases & SAM_DATA_MODIFYING_LEASES)) {
			/*
			 * Force an inode update if updated or changed
			 * and not holding any SAM_DATA_MODIFYING_LEASES.
			 */
			meta_timeo = 0;

		} else {
			/*
			 * Update inode on configured timeout interval.
			 */
			meta_timeo = ip->mp->mt.fi_meta_timeo;
		}

		error = sam_refresh_client_ino(ip,
		    ip->mp->mt.fi_meta_timeo, credp);
	}
	SAM_CLOSE_OPERATION(ip->mp, error);
	return (error);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
int
samqfs_client_revalidate(struct dentry *de)
{
	return (__samqfs_client_getattr(de));
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
int
samqfs_client_getattr(struct vfsmount *mnt, struct dentry *de,
    struct kstat *stat)
{
	sam_node_t *ip = SAM_LITOSI(de->d_inode);
	int error;

	error = __samqfs_client_getattr(de);
	if (error == 0) {
		generic_fillattr(de->d_inode, stat);
		stat->blksize = LG_BLK(ip->mp, DD);
	}
	return (error);
}
#endif

static int
__samqfs_lookup_revalidate(struct dentry *de)
{
	cred_t *credp;
	sam_node_t *ip;
	int error, res = 1;

	if (de->d_inode == NULL) {
		return (0);
	}
	ip = SAM_LITOSI(de->d_inode);

	if ((error = sam_open_operation(ip))) {
		return (0);
	}
	if (ip->di.id.ino != SAM_INO_INO) {
		if (!ip->flags.b.purge_pend) {
			credp = CRED();
			error = sam_refresh_client_ino(ip,
			    ip->mp->mt.fi_meta_timeo, credp);
			if (error > 0) {
				res = 0;
			}

		} else {
			res = 0;
		}
	}
	SAM_CLOSE_OPERATION(ip->mp, error);
	return (res);
}


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
static int
samqfs_lookup_revalidate(struct dentry *de, int flags)
{
	return (__samqfs_lookup_revalidate(de));
}
#endif


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
static int
samqfs_lookup_revalidate(struct dentry *de, struct nameidata *ndp)
{
	return (__samqfs_lookup_revalidate(de));
}
#endif


static void
samqfs_root_dentry_iput(struct dentry *de, struct inode *li)
{
	sam_node_t *ip = SAM_LITOSI(li);
	sam_mount_t *mp = NULL;

	mp = ip->mp;
	mutex_enter(&mp->ms.m_waitwr_mutex);

	mp->mt.fi_status |= FS_UMOUNT_IN_PROGRESS;

	if (ip->di.id.ino != SAM_ROOT_INO) {

		mp->mt.fi_status &= ~FS_UMOUNT_IN_PROGRESS;
		mutex_exit(&mp->ms.m_waitwr_mutex);

		cmn_err(CE_WARN,
		    "SAM-QFS root_dentry_iput bad ino %d\n", ip->di.id.ino);
		iput(li);
		return;
	}

	mp->mi.m_vn_root = NULL;

	mutex_exit(&mp->ms.m_waitwr_mutex);

	/*
	 * iput the root inode
	 */
	iput(li);
}

/*
 * ----- File operations for files supported on the SAM-QFS file system.
 */

struct file_operations samqfs_file_ops = {
	llseek:		generic_file_llseek,
	read:		samqfs_client_read_vn,
	write:		samqfs_client_write_vn,
	mmap:		samqfs_client_addmap_vn,
	open:		samqfs_client_open_vn,
	flush:		samqfs_client_flush,
	release:	samqfs_client_close_vn,
	fsync:		samqfs_client_fsync_vn,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 9))
	flock:		samqfs_client_frlock_vn,
#endif
	lock:		samqfs_client_frlock_vn
};


/*
 * ----- Inode operations for files supported on the SAM-QFS file system.
 */
struct inode_operations samqfs_file_inode_ops = {
	permission:	samqfs_access_vn,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	getattr:	samqfs_client_getattr,
#else
	revalidate:	samqfs_client_revalidate,
#endif
	setattr:	samqfs_client_setattr_vn
};


/*
 * ----- Address space operations for files
 *	supported by the SAM-QFS file system.
 */
struct address_space_operations samqfs_file_aops = {
	writepage:		samqfs_client_putpage_vn,
	readpage:		samqfs_client_getpage_vn,
	sync_page:		samqfs_client_sync_page,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	writepages:		samqfs_client_putpages_vn,
#endif
	prepare_write:	samqfs_prepare_write,
	commit_write:	samqfs_commit_write,
#if defined(RHE_LINUX) && (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 8))
	direct_sector_IO: samqfs_direct_IO
#else
	direct_IO:		samqfs_direct_IO
#endif
};


struct inode_operations samqfs_symlink_inode_ops = {
	readlink:		samqfs_client_readlink_vn,
	follow_link:		samqfs_client_follow_link,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 8))
	put_link:		samqfs_client_put_link,
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	getattr:		samqfs_client_getattr,
#else
	revalidate:		samqfs_client_revalidate,
#endif
	setattr:		samqfs_client_setattr_vn
};


/*
 * ----- File operations for directories supported by the SAM-QFS file system.
 */
struct file_operations samqfs_dir_ops = {
	read:		generic_read_dir,
	readdir:	samqfs_client_readdir_vn,
	open:		samqfs_client_open_vn,
	release:	samqfs_client_close_vn,
};


/*
 * ----- Inode operations for directories supported by the SAM-QFS file system.
 */

struct inode_operations samqfs_dir_inode_ops = {
	create:		samqfs_client_create_vn,
	lookup:		samqfs_client_lookup_vn,
	link:		samqfs_client_link_vn,
	unlink:		samqfs_client_remove_vn,
	symlink:	samqfs_client_symlink_vn,
	mkdir:		samqfs_client_mkdir_vn,
	rmdir:		samqfs_client_rmdir_vn,
	mknod:		samqfs_client_mknod_vn,
	rename:		samqfs_client_rename_vn,
	permission:	samqfs_access_vn,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	getattr:	samqfs_client_getattr,
#else
	revalidate:	samqfs_client_revalidate,
#endif
	setattr:	samqfs_client_setattr_vn,
};


struct dentry_operations samqfs_dentry_ops = {
	d_revalidate:	samqfs_lookup_revalidate,
};


/*
 * For file system root inodes
 */
struct dentry_operations samqfs_root_dentry_ops = {
	d_revalidate:	samqfs_lookup_revalidate,
	d_iput:			samqfs_root_dentry_iput,
};


/*
 * VM ops for mmap
 */
struct vm_operations_struct samqfs_vm_ops = {
	nopage:	samqfs_client_nopage,
	close:	samqfs_client_delmap_vn
};
