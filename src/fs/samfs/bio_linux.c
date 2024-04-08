/*
 * ----- bio_linux.c - linux block I/O functions
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
 * or https://illumos.org/license/CDDL.
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
#pragma ident "$Revision: 1.22 $"
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
#include <linux/blkdev.h>

#include <asm/param.h>
#include <asm/uaccess.h>
#include <asm/system.h>

/*
 * ----- SAMFS Includes
 */

#include "sam/param.h"
#include "sam/types.h"

#include "inode.h"
#include "mount.h"
#include "ino.h"
#include "clextern.h"
#include "share.h"
#include "debug.h"
#include "trace.h"


int sam_get_client_block(sam_mount_t *mp, sam_node_t *ip,
	enum BLOCK_operation op, enum SHARE_flag wait_flag,
	void *args, char *buf);

/*
 * ----- sam_blkstale - Ensure that a specified block is staled.
 *  This is used by the shared file system to stale indirect blocks.
 */

void
sam_blkstale(
	dev_t dev,		/* Buffer device */
	daddr_t blkno)		/* Buffer 512 byte block number */
{
/*
 * We need to figure out how to do this for Linux
 */
}


/*
 * ----- sam_bread -
 * Issue sync. read and check for an error. If error, release buffer.
 */

int				/* EIO if error, 0 if successful. */
sam_bread(
	struct samdent *dp,	/* contains Linux file pointer */
	sam_daddr_t blkno,	/* Buffer 1024 byte block number */
	long bsize,		/* Buffer byte count */
	char *buf)
{
	int error = 0;
	struct file *fp = NULL;
	loff_t offset;
	ssize_t ret;
	mm_segment_t save_state;

	fp = dp->fp;

	if (!dp->opened) {
		return (EINVAL);
	}

	/*
	 * Convert block offset to bytes
	 */
	offset = blkno * SAM_DEV_BSIZE;

	save_state = get_fs();
	set_fs(KERNEL_DS);

	ret = fp->f_op->read(fp, buf, (size_t)bsize, &offset);

	set_fs(save_state);

	if (ret < 0) {
		error = -ret;
		cmn_err(CE_WARN,
		    "SAM-QFS: sam_bread: dev=%x, blkno=%llx, count=%ld, "
		    "error=%d", dp->dev, blkno, bsize, error);
		TRACE(T_SAM_BREAD_ERR, NULL, error, dp->dev, blkno);
	}
	return (error);
}


/*
 * ----- sam_get_client_sblk -
 * Issue command OTW to get superblock information.
 * Read it into a newly created buffer.
 */

int					/* EIO if error, 0 if successful. */
sam_get_client_sblk(
	sam_mount_t *mp,		/* Mount table pointer */
	enum SHARE_flag wait_flag,
	long bsize,			/* Buffer byte count */
	char *buf)			/* Returned buffer */
{
	int error;
	sam_block_sblk_t getsblk;

	getsblk.bsize = bsize;
	getsblk.addr = 0;
	memcpy((void *)&getsblk.addr, (void *)&buf, sizeof (char *));

	if ((error = sam_get_client_block(mp, NULL, BLOCK_getsblk, wait_flag,
	    &getsblk, buf))) {
		TRACE(T_SAM_BREAD_ERR, NULL, error, 0, SUPERBLK);
	}

	return (error);
}


/*
 * ----- sam_get_client_block -
 * Issue command OTW to get a buffer.
 */

int				/* error, else 0 if successful. */
sam_get_client_block(
	sam_mount_t *mp,	/* Mount table pointer */
	sam_node_t *ip,		/* Inode pointer */
	enum BLOCK_operation op,
	enum SHARE_flag wait_flag,
	void *args,		/* Arguments for this op */
	char *buf)		/* Buffer header pointer */
{
	int error;

	error = sam_proc_block(mp, ip, op, wait_flag, args);

	return (error);
}


/*
 * ----- sam_sbread -
 * Read into the buffer cache. blkno is a 512 sector number.
 * Issue OTW for a shared client, otherwise, issue bread to the disk.
 * Issue sync. read and check for an error. If error, release buffer.
 * ENOTCONN/EPIPE returned during failover. If now server,
 * issue bread.
 */

int				/* EIO if error, 0 if successful. */
sam_sbread(
	sam_node_t *ip,		/* Inode table pointer */
	int ord,		/* Device ordinal */
	sam_daddr_t blkno,	/* Buffer 1024 byte block number */
	long bsize,		/* byte cnt on local machine or server */
	char *bufp)		/* buffer */
{
	sam_mount_t *mp = ip->mp;
	int error;
	sam_block_getbuf_t getbuf;
	sam_iecache_t *iecachep;

	/*
	 * Grab iecache mutex.  Use cached block if available.
	 * NOTE: This scheme is temporary until we can develop a more
	 * general solution for Linux.  This quick solution facilitates
	 * large file testing on Linux.
	 */
	mutex_enter(&ip->cl_iec_mutex);
	if ((iecachep = sam_iecache_find(ip, ord, blkno, bsize)) != NULL) {
		memcpy(bufp, iecachep->bufp, bsize);
		mutex_exit(&ip->cl_iec_mutex);	/* Done. */
		error = 0;
	} else {
		/*
		 * Request indirect extent from MDS.  Don't need to hold
		 * mutex in the interim.
		 */
		mutex_exit(&ip->cl_iec_mutex);
		getbuf.ord = ord;
		getbuf.blkno = blkno;
		getbuf.bsize = bsize;
		getbuf.addr = 0;
		memcpy((void *)&getbuf.addr, (void *)&bufp, sizeof (char *));
		error = sam_get_client_block(mp, NULL, BLOCK_getbuf,
		    SHARE_wait, &getbuf, NULL);

		/* If successful, update cache with this block */
		if (error == 0) {
			(void) sam_iecache_update(ip, ord, blkno, bsize, bufp);
		}
	}

	TRACE(T_SAM_CL_BREAD_ERR, SAM_SITOLI(ip), error, ord, blkno);
	return (error);
}


/*
 * ----- sam_flush_indirect_block - Free indirect extents.
 * Recursively call this routine until all indirect blocks are released.
 */

int					/* ERRNO if error, 0 if successful. */
sam_flush_indirect_block(
	sam_node_t *ip,			/* Inode pointer */
	sam_bufcache_flag_t bc_flag,	/* Flush or stale buffer cache */
	int kptr[],			/* ary of extents that end the file. */
	int ik,				/* kptr index */
	uint32_t *extent_bn,		/* mass storage extent block number */
	uchar_t *extent_ord,		/* mass storage extent ordinal */
	int level,			/* level of indirection */
	int *set)			/* Set for last indirect block */
{
	struct sam_mount *mp;
	int i;
	sam_indirect_extent_t *iep;
	sam_daddr_t bn;
	uint32_t *ie_addr;
	uchar_t *ie_ord;
	int error = 0;
	char *bp;

	mp = ip->mp;
	bn = *extent_bn;
	bn <<= mp->mi.m_bn_shift;
	if ((bp = kmem_alloc(SAM_INDBLKSZ, KM_SLEEP)) == NULL) {
		return (ENOMEM);
	}
	if ((error = sam_sbread(ip, *extent_ord, bn, SAM_INDBLKSZ, bp)) == 0) {
		iep = (sam_indirect_extent_t *)bp;
		/* check for invalid indirect block */
		if (iep->ieno != (level+1)) {
			sam_req_fsck(mp, -1,
			    "sam_flush_indirect_block: level");
			error = ENOCSI;
		} else if ((iep->id.ino != ip->di.id.ino) ||
		    (iep->id.gen != ip->di.id.gen)) {
			sam_req_fsck(mp, -1,
			    "sam_flush_indirect_block: ino/gen");
			error = ENOCSI;
			if (S_ISSEGS(&ip->di) &&
			    (ip->di.rm.info.dk.seg.ord == 0)) {
				if ((iep->id.ino == ip->di.parent_id.ino) &&
				    (iep->id.gen == ip->di.parent_id.gen)) {
					error = 0;
				}
			}
		}
		if (error) {	/* Invalid indirect block */
			if (!SAM_IS_SHARED_CLIENT(ip->mp)) {
				sam_req_fsck(ip->mp, *extent_ord,
				    "sam_flush_indirect_block: gen");
				cmn_err(CE_NOTE,
"SAM-QFS update.c indirect block: fs=%d ino=%d.%d.%d ib=%d.%d.%d bn=0x%x "
"ord=%d",
				    ip->mp->mt.fi_eq, ip->di.id.ino,
				    ip->di.id.gen, level+1,
				    iep->id.ino, iep->id.gen, iep->ieno,
				    *extent_bn, *extent_ord);
			} else {
				cmn_err(CE_NOTE,
				    "SAM-QFS Client indirect block: fs=%d "
				    "ino=%d.%d.%d ib=%d.%d.%d bn=0x%x ord=%d",
				    ip->mp->mt.fi_eq, ip->di.id.ino,
				    ip->di.id.gen, level+1,
				    iep->id.ino, iep->id.gen, iep->ieno,
				    *extent_bn, *extent_ord);
			}
			kmem_free(bp, SAM_INDBLKSZ);
			return (ENOCSI);
		}
		for (i = (DEXT - 1); i >= 0; i--) {
			if (*set && i < kptr[ik]) {
				error = ECANCELED;
				break;
			}
			ie_addr = &iep->extent[i];
			if (*ie_addr == 0) {	/* If empty */
				continue;
			}
			ie_ord = &iep->extent_ord[i];
			if (level) {
				if (i <= kptr[ik]) {
					*set = 1;
				}
				error = sam_flush_indirect_block(ip, bc_flag,
				    kptr, (ik + 1),
				    ie_addr, ie_ord, (level - 1), set);
			} else {
				bn = *ie_addr;
				bn <<= mp->mi.m_bn_shift;
				sam_iecache_ent_clear(ip, *ie_ord, bn);
			}
		}

		bn = *extent_bn;
		bn <<= mp->mi.m_bn_shift;
		sam_iecache_ent_clear(ip, *extent_ord, bn);
	} else {
		if (!SAM_IS_SHARED_CLIENT(mp)) {
			sam_req_fsck(mp, *extent_ord,
			    "sam_flush_indirect_block: gen2");
			cmn_err(CE_NOTE,
			"SAM-QFS Indirect block I/O error: fs=%d ino=%d.%d "
			"bn=0x%x ord=%d",
			    mp->mt.fi_eq, ip->di.id.ino, ip->di.id.gen,
			    *extent_bn, *extent_ord);
		}
	}
	kmem_free(bp, SAM_INDBLKSZ);
	return (error);
}
