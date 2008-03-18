/*
 * ----- getdents_linux.c - Process the SAM-QFS readdir function.
 *
 *	Processes the standard readdir call and the ioctl getdents
 *	(F_GETDENTS) extended read directory call.
 *
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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
#pragma ident "$Revision: 1.16 $"
#endif

#include "sam/osversion.h"


/* ----- UNIX Includes */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/unistd.h>

#include <linux/fs.h>

#include <asm/system.h>
#include <asm/uaccess.h>


/* ----- SAMFS Includes */

#include "sam/param.h"
#include "sam/types.h"

#include "ino.h"
#include "inode.h"
#include "mount.h"
#include "dirent.h"
#include "debug.h"
#include "clextern.h"


/*
 * NOTE: We should also use fs init time, but it's not quickly available.
 * This needs to be re-worked for real later anyway, but it works okay for now.
 */

/*
 * ----- fbread -
 * Read file block at offset.  Return data in new fbuf of size bsize.
 * Here because it is used only for directory info retrieval.
 */
int
fbread(sam_node_t *ip, offset_t offset, int bsize, struct fbuf **fbpp)
{
	sam_block_fgetbuf_t fgetbuf;
	struct fbuf *fbp = NULL;
	int num_pages = 1;
	int error = 0;
	int p_offset[2];
	int copy_len[2];
	int start_offset;
	char *dest;
	int i;
	char *fbread_buf = NULL;

	if (bsize > PAGESIZE) {
		error = EINVAL;
		goto done;
	}

	fbread_buf = kmem_zalloc(PAGESIZE, KM_SLEEP);
	if (fbread_buf == NULL) {
		error = ENOMEM;
		goto done;
	}

	p_offset[0] = offset & ~(PAGESIZE - 1);			/* blk 0 addr */
	p_offset[1] = (offset + bsize - 1) & ~(PAGESIZE - 1);	/* blk 1 addr */
	start_offset = offset & (PAGESIZE - 1);		/* blk 0 start offset */
	copy_len[0] = PAGESIZE - start_offset;		/* blk 1 copy length */
	if (copy_len[0] > bsize) {
		copy_len[0] = bsize;
	}
	copy_len[1] = bsize - copy_len[0];		/* blk 2 copy length */

	if (p_offset[0] != p_offset[1]) {
		num_pages++;
	}

	/* Allocate new fbuf */
	fbp = (struct fbuf *) kmem_zalloc(sizeof (struct fbuf) + bsize,
	    KM_SLEEP);
	if (fbp == NULL) {
		error = ENOMEM;
		goto done;
	}
	fbp->fb_addr = (char *)fbp + sizeof (struct fbuf);
	fbp->fb_bsize = bsize;

	fbp->fb_count = 1;
	dest = fbp->fb_addr;

	/* Retrieve up to 2 blocks. */
	for (i = 0; i < num_pages; i++) {

		/* Initialize fgetbuf for send to server */
		fgetbuf.offset = p_offset[i];
		fgetbuf.len = PAGESIZE;
		fgetbuf.bp = (int64_t)fbread_buf;
		fgetbuf.ino = ip->di.id.ino;

		error = sam_proc_block(ip->mp, ip, BLOCK_fgetbuf,
		    SHARE_wait, &fgetbuf);

		if (error) {
			kmem_free(fbp, sizeof (struct fbuf) + bsize);
			fbp = NULL;
			goto done;
		}

		bcopy((fbread_buf + start_offset), dest, copy_len[i]);
		start_offset = 0;	/* Copy from start of block 2 next */
		dest += copy_len[i];
	}

done:
	if (fbread_buf) {
		kmem_free(fbread_buf, PAGESIZE);
	}
	*fbpp = fbp;    /* Save pointer to fbuf */
	return (error);
}


/*
 * ----- sam_getdents - Return a SAM directory.
 *	Read the directory and return the entries.
 *	Inode data_rwl RW_READER lock is set on entry.
 */

int				/* ERRNO if error, 0 if successful. */
sam_getdents(
	sam_node_t *ip,		/* directory inode to be returned. */
	cred_t *credp,		/* credentials pointer */
	filldir_t filldir,	/* copyout function */
	void *d,		/* getdents callback buffer */
	struct file *fp,	/* File pointer */
	int *ret_count)		/* Returned count */
{
	struct sam_dirent *dp;	/* Sam dirent entry pointer */
	struct fbuf *fbp;
	int error;
	int in;
	ino_t ino;
	offset_t f_offset;
	int dt_flag;
	int fbsize;
	int bsize = 0;

	error = 0;
	fbp = NULL;
	*ret_count = 0;

	f_offset = fp->f_pos;	/* Return directory entries at offset. */

	if (f_offset >= ip->di.rm.size) {	/* Offset must be in bounds */
		return (0);
	}
	if (f_offset & (NBPW-1)) {	/* Offset wust be on word boundary */
		return (EINVAL);
	}

	if (ip->di.status.b.offline || ip->flags.b.stage_pages) {
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (ip->di.status.b.offline || ip->flags.b.stage_pages) {
			error = sam_clear_ino(ip, ip->di.rm.size,
			    MAKE_ONLINE, credp);
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			if (error) {
				goto done;
			}
		} else {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
	}

	while (f_offset < ip->di.rm.size) {

		fbsize = SAM_FBSIZE(f_offset);	/* # bytes to read from block */
		bsize = fbsize;
		if (fbsize > DIR_BLK) {		/* Limit size to max 4096 */
			bsize = fbsize - DIR_BLK;
		}

		if ((error = fbread(ip, f_offset, bsize, &fbp)) != 0) {
			goto done;
		}

		in = 0;
		while (in < bsize) {
			dp = (struct sam_dirent *)(void *)(fbp->fb_addr + in);

			/* If empty/validation entry */
			if (dp->d_fmt <= SAM_DIR_VERSION) {
				if (((int)dp->d_reclen <= 0) ||
				    ((int)dp->d_reclen > bsize) ||
				    ((int)dp->d_reclen & (NBPW-1))) {
					/* Ill-formed directory entry */
					error = EINVAL;
					goto done;
				}
				in += dp->d_reclen;
				f_offset += dp->d_reclen;
				continue;
			}
			ino = dp->d_id.ino;
			if ((ino == 0) ||
			    (dp->d_namlen == 0) ||
			    (dp->d_namlen > dp->d_reclen)) {
				/* Directory inode entry invalid */
				error = EBADSLT;
				goto done;
			}

			if ((strncmp(dp->d_name, ".", dp->d_namlen) == 0) ||
			    (strncmp(dp->d_name, "..", dp->d_namlen) == 0)) {
				dt_flag = DT_DIR;
			} else {
				dt_flag = DT_UNKNOWN;
			}
			if ((error = filldir(d, dp->d_name, dp->d_namlen,
			    f_offset, ino, dt_flag)) < 0) {
				goto done;	/* User buffer exhausted? */
			}

			/* Following code duplicated in lookup.c */
			if (((int)dp->d_reclen <= 0) ||
			    ((int)dp->d_reclen > bsize) ||
			    ((int)dp->d_reclen & (NBPW-1))) {
				error = EINVAL;		/* invalid entry */
				goto done;
			}
			in += dp->d_reclen;
			f_offset += dp->d_reclen;
			*ret_count += 1;
		}

		if (fbp) {
			fbrelease(fbp);
		}
		fbp = NULL;
	}
done:
	if (fbp) {
		fbrelease(fbp);
	}

	fp->f_pos = f_offset;
	return (error);
}


/*
 * ----- sam_check_zerodirblk -
 * At 3.3.1-25 through 3.3.1-34 and 3.5.0-xx through 3.5.0-xy, a problem
 * in create code could have caused a zero block at the end of a directory,
 * without a validation record. sam_check_zerodirblk checks for this
 * condition.
 */

int				/* 1 returned, if a zero directory block */
sam_check_zerodirblk(sam_node_t *pip, char *cp, sam_u_offset_t offset)
{
	ulong_t *word;
	int i;

	if (offset == (pip->di.rm.size - DIR_BLK)) {
		word = (ulong_t *)(void *)cp;
		for (i = 0; i < (DIR_BLK/sizeof (ulong_t)); i++, word++) {
			if (*word != 0)
				return (0);
		}
		return (1);
	}
	return (0);
}
