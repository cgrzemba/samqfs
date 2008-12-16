/*
 * ----- sam/bio.c - Buffer cache functions.
 *
 *  Buffer cache processing functions.
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

#pragma ident "$Revision: 1.64 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/fbuf.h>
#include <sys/var.h>
#include <sys/rwlock.h>
#include <sys/cmn_err.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <vm/seg_map.h>

/* ----- SAMFS Includes */

#include "sam/param.h"
#include "sam/types.h"

#include "inode.h"
#include "mount.h"
#include "ino.h"
#include "ioblk.h"
#include "extern.h"
#include "share.h"
#include "debug.h"
#include "trace.h"
#include "qfs_log.h"


extern struct hbuf	*hbuf;			/* Hash buckets */

#define	bio_bhash(dev, bn)	(hash2ints((dev), (int)(bn)) & v.v_hmask)

static int sam_get_client_block(sam_mount_t *mp, sam_node_t *ip,
	enum BLOCK_operation op, enum SHARE_flag wait_flag,
	void *args, buf_t *bp);

void (*bio_lqfs_strategy)(void *, buf_t *);


/*
 * ----- sam_blkstale - Ensure that a specified block is staled.
 *	This is used by the shared file system to stale indirect blocks.
 */

void
sam_blkstale(
	dev_t dev,		/* Buffer device */
	daddr_t blkno)		/* Buffer 512 byte block number */
{
	struct buf *bp, *dp;
	struct hbuf *hp;
	uint_t index;
	kmutex_t *hmp;

	index = bio_bhash(dev, blkno);
	hp = &hbuf[index];
	dp = (struct buf *)(void *)hp;
	hmp = &hp->b_lock;

	/*
	 * Identify the buffer in the cache belonging to this device
	 * and blkno (if any). Stale it. Do not use sem locks because
	 * of possible deadlocks. Not necessary.
	 */
	mutex_enter(hmp);
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {
		if (bp->b_blkno == blkno && bp->b_edev == dev) {
			bp->b_flags |= B_STALE | B_AGE;
			mutex_exit(hmp);
			return;
		}
	}
	mutex_exit(hmp);
}


/*
 * ----- sam_bread -
 * Issue sync. read and check for an error. If error, release buffer.
 * If logging, use bio_lqfs_strategy() instead of bread(), with
 * bread_common() code ported from Solaris to bypass the assumption
 * that UFS is the only logging filesystem.
 */

int				/* EIO if error, 0 if successful. */
sam_bread(
	struct sam_mount *mp,	/* Pointer to mount info for QFS logging */
	dev_t dev,		/* Buffer device */
	sam_daddr_t blkno,	/* Buffer 1024 byte block number */
	long bsize,		/* Buffer byte count */
	buf_t **bpp)		/* Returned buffer header ptr if no error */
{
	buf_t *bp;
	daddr_t blk;

	blk = (daddr_t)blkno;
	ASSERT(blk == blkno);
	if (mp && TRANS_ISTRANS(mp) && bio_lqfs_strategy) {
		klwp_t *lwp = ttolwp(curthread);

		bp = getblk_common(NULL, dev, (blk << SAM2SUN_BSHIFT),
		    bsize, /* errflg */ 1);
		if ((bp->b_flags & B_DONE) == 0) {
			bp->b_flags |= B_READ;
			ASSERT(bp->b_bcount == bsize);
			(*bio_lqfs_strategy)(TRANS_ISTRANS(mp), bp);
			if (lwp != NULL) {
				lwp->lwp_ru.inblock++;
			}
			(void) biowait(bp);
		}
	} else {
		bp = bread(dev, (blk << SAM2SUN_BSHIFT), bsize);
	}
	if ((bp->b_flags & B_ERROR) == 0) {
		*bpp = bp;
		return (0);
	}
	cmn_err(CE_WARN,
	    "SAM-QFS: sam_bread: bp=%p, blkno=%llx, dev=%lx, count=%ld, "
	    "error=%d",
	    (void *)bp, blkno, dev, bsize, geterror(bp));
	TRACE(T_SAM_BREAD_ERR, NULL, geterror(bp), dev, blkno<<SAM2SUN_BSHIFT);
	brelse(bp);
	*bpp = NULL;
	return (EIO);
}


/*
 * ----- sam_bread_db -
 * Issue sync. read and check for an error. If error, release buffer.
 * If logging, use bio_lqfs_strategy() instead of bread(), with
 * bread_common() code ported from Solaris to bypass the assumption
 * that UFS is the only logging filesystem.
 */

int				/* EIO if error, 0 if successful. */
sam_bread_db(
	struct sam_mount *mp,	/* Pointer to mount info for QFS logging */
	dev_t dev,		/* Buffer device */
	daddr_t blkno,		/* Buffer 512 byte block number */
	long bsize,		/* Buffer byte count */
	buf_t **bpp)		/* Returned buf header pointer if no error */
{
	buf_t *bp;

	if (mp && TRANS_ISTRANS(mp) && bio_lqfs_strategy) {
		klwp_t *lwp = ttolwp(curthread);

		bp = getblk_common(NULL, dev, blkno,
		    bsize, /* errflg */ 1);
		if ((bp->b_flags & B_DONE) == 0) {
			bp->b_flags |= B_READ;
			ASSERT(bp->b_bcount == bsize);
			(*bio_lqfs_strategy)(TRANS_ISTRANS(mp), bp);
			if (lwp != NULL) {
				lwp->lwp_ru.inblock++;
			}
			(void) biowait(bp);
		}
	} else {
		bp = bread(dev, blkno, bsize);
	}
	if ((bp->b_flags & B_ERROR) == 0) {
		*bpp = bp;
		return (0);
	}
	cmn_err(CE_WARN,
	    "SAM-QFS: sam_bread_db: bp=%p, blkno=%llx, dev=%lx, count=%ld, "
	    "error=%d",
	    (void *)bp, blkno, dev, bsize, geterror(bp));
	TRACE(T_SAM_BREAD_ERR, NULL, geterror(bp), dev, blkno);
	brelse(bp);
	*bpp = NULL;
	return (EIO);
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
	buf_t **bpp)			/* Returned buffer */
{
	int error;
	sam_block_sblk_t getsblk;
	buf_t *bp;

	bp = ngeteblk(bsize);
	getsblk.bsize = bsize;
	getsblk.addr = (uint64_t)bp->b_un.b_addr;

	if ((error = sam_get_client_block(mp, NULL, BLOCK_getsblk, wait_flag,
	    &getsblk, bp))) {
		TRACE(T_SAM_BREAD_ERR, NULL, geterror(bp), 0, SUPERBLK);
		brelse(bp);
		bp = NULL;
	}

	*bpp = bp;
	return (error);
}


/*
 * ----- sam_get_client_inode -
 * Issue command OTW to get inode information (512 bytes).
 * Cannot issue this command if file system is locked.
 */

int				/* EIO if error, 0 if successful. */
sam_get_client_inode(
	sam_mount_t *mp,	/* Mount table pointer */
	sam_id_t *fp,		/* file i-number & generation number. */
	enum SHARE_flag wait_flag,
	long bsize,		/* Buffer byte count */
	buf_t **bpp)		/* Returned buffer header, unless error */
{
	buf_t *bp;
	int error;
	sam_block_getino_t getino;

start:
	bp = getblk(mp->mi.m_vfsp->vfs_dev, ((daddr_t)0x80000000 | fp->ino),
	    bsize);
	if ((bp->b_flags & B_DONE) && !(bp->b_flags & B_STALE)) {
		*bpp = bp;
		if (bp->b_bcount != bsize) {
			cmn_err(CE_NOTE,
		"SAM-QFS: %s: sam_get_client_inode: bp=%p blk=%x "
		"size=%lld bsize=%lld",
			    mp->mt.fi_name, (void *)bp, bp->b_blkno,
			    (long long)bp->b_bcount, (long long)bsize);
		}
		error = 0;
	} else {
		getino.id = *fp;
		getino.bsize = bsize;
		getino.addr = (uint64_t)bp->b_un.b_addr;
		error = sam_get_client_block(mp, NULL, BLOCK_getino, wait_flag,
		    &getino, bp);
	}
	if (error) {
		TRACE(T_SAM_BREAD_ERR, NULL, geterror(bp), 0,
		    ((daddr_t)0x80000000 | fp->ino));
		brelse(bp);
		*bpp = NULL;
		/*
		 * Freeze all threads except set/get .hosts file call.
		 */
		if (fp->ino != SAM_HOST_INO) {
			if (error == ENOTCONN || error == EPIPE) {
				if ((error =
				    sam_stop_inode(mp->mi.m_inodir)) == 0) {
					goto start;
				}
			}
		}
	} else {
		*bpp = bp;
	}
	return (error);
}


/*
 * ----- sam_get_client_block -
 * Issue command OTW to get a buffer.
 */

static int			/* error if not successful, 0 if successful. */
sam_get_client_block(
	sam_mount_t *mp,	/* Mount table pointer */
	sam_node_t *ip,		/* Inode pointer */
	enum BLOCK_operation op,
	enum SHARE_flag wait_flag,
	void *args,		/* Arguments for this op */
	buf_t *bp)		/* Buffer header pointer */
{
	int error;

	bp->b_flags = B_BUSY | B_READ | (bp->b_flags & B_NOCACHE);
	error = sam_proc_block(mp, ip, op, wait_flag, args);
	bp->b_flags &= ~(B_STALE | B_AGE | B_DELWRI | B_RETRYWRI);
	if (error == 0) {
		bp->b_resid = bp->b_bcount;
		bp->b_flags |= B_DONE;
	} else {
		bp->b_flags |= (B_ERROR | B_DONE | B_STALE);
		bp->b_error = error;
	}
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
	long bsize,		/* buf byte count on local machine or server */
	buf_t **bpp)		/* Returned buffer header, unless error */
{
	sam_mount_t *mp = ip->mp;
	buf_t *bp;
	sam_daddr_t block;
	daddr_t blk;
	int error;

	block = (blkno << SAM2SUN_BSHIFT);
	blk = (daddr_t)block;
	ASSERT(blk == block);

	if (SAM_IS_SHARED_CLIENT(mp)) {
		sam_block_getbuf_t getbuf;

		bp = getblk(mp->mi.m_fs[ord].dev, blk, bsize);
		if ((bp->b_flags & B_DONE) && !(bp->b_flags & B_STALE)) {
			*bpp = bp;
			if (bp->b_bcount != bsize) {
				cmn_err(CE_NOTE,
				"SAM-QFS: %s: sam_sbread: bp=%p blk=%x "
				"size=%lld bsize=%lld",
				    mp->mt.fi_name, (void *)bp, bp->b_blkno,
				    (long long)bp->b_bcount, (long long)bsize);
			}
			error = 0;
		} else {
			getbuf.ord = ord;
			getbuf.blkno = (int32_t)blkno;
			getbuf.bsize = bsize;
			getbuf.addr = (uint64_t)bp->b_un.b_addr;
			error = sam_get_client_block(mp, ip, BLOCK_getbuf,
			    SHARE_wait, &getbuf, bp);
		}
		if (error == 0) {
			*bpp = bp;
			return (0);
		}
		if (SAM_IS_SHARED_SERVER(mp)) {
			brelse(bp);
		} else {
			TRACE(T_SAM_CL_BREAD_ERR, SAM_ITOV(ip),
			    geterror(bp), ord, blkno);
			brelse(bp);
			*bpp = NULL;
			return (error);
		}
	}

	/*
	 * If not a shared file system, or the QFS shared metadata server.
	 */
#if 0
	bp = bread(mp->mi.m_fs[ord].dev, blk, bsize);
	if (bp && ((bp->b_flags & B_ERROR) == 0)) {
		*bpp = bp;
		return (0);
	}
	error = EIO;
	TRACE(T_SAM_BREAD_ERR, SAM_ITOV(ip), geterror(bp), ord, blkno);
	brelse(bp);
	*bpp = NULL;
#else
	error = sam_bread_db(mp, mp->mi.m_fs[ord].dev, blk, bsize, &bp);
	if (error == 0) {
		*bpp = bp;
	} else {
		*bpp = NULL;
	}
#endif /* 0 */
	return (error);
}


/*
 * ----- sam_bwrite -
 * Issue sync.  Write and check for an error.  Release buffer.
 * If logging, use bio_lqfs_strategy() instead of bwrite2(),
 * with bwrite_common() code ported from Solaris to bypass
 * the assumption that UFS is the only logging filesystem.
 */

int				/* EIO if error, 0 if successful. */
sam_bwrite(
	struct sam_mount *mp,	/* Pointer to mount info for QFS logging */
	buf_t *bp)		/* Pointer to buffer header */
{
	int error;

	if (mp && TRANS_ISTRANS(mp) && bio_lqfs_strategy != NULL) {
		klwp_t *lwp = ttolwp(curthread);

		ASSERT(SEMA_HELD(&bp->b_sem));
		bp->b_flags &= ~(B_READ|B_DONE|B_ERROR|B_DELWRI);
		if (lwp != NULL)
			lwp->lwp_ru.oublock++;
		(*bio_lqfs_strategy)(TRANS_ISTRANS(mp), bp);
		(void) biowait(bp);
	} else {
		bwrite2(bp);
	}
	if ((bp->b_flags & B_ERROR) == 0) {
		error = 0;
	} else {
		error = EIO;
	}
	brelse(bp);
	return (error);
}


/*
 * ----- sam_bwrite_noforcewait_dorelease -
 * Issue sync.  Write and check for an error.  Don't wait, release buffer.
 * If logging, use bio_lqfs_strategy() instead of bwrite(),
 * with bwrite_common() code ported from Solaris to bypass
 * the assumption that UFS is the only logging filesystem.
 */

int				/* EIO if error, 0 if successful. */
sam_bwrite_noforcewait_dorelease(
	struct sam_mount *mp,	/* Pointer to mount info for QFS logging */
	buf_t *bp)		/* Pointer to buffer header */
{
	int error;

	if (mp && TRANS_ISTRANS(mp) && bio_lqfs_strategy != NULL) {
		register int do_wait;
		int flag;
		klwp_t *lwp = ttolwp(curthread);

		ASSERT(SEMA_HELD(&bp->b_sem));
		flag = bp->b_flags;
		bp->b_flags &= ~(B_READ|B_DONE|B_ERROR|B_DELWRI);
		if (lwp != NULL)
			lwp->lwp_ru.oublock++;
		do_wait = ((flag & B_ASYNC) == 0);
		(*bio_lqfs_strategy)(TRANS_ISTRANS(mp), bp);
		if (do_wait) {
			(void) biowait(bp);
			brelse(bp);
		}
	} else {
		bwrite(bp);
	}
	if ((bp->b_flags & B_ERROR) == 0) {
		error = 0;
	} else {
		error = EIO;
	}
	return (error);
}


/*
 * ----- sam_bwrite2 -
 * Issue sync.  Write and check for an error.  Do not release buffer.
 * If logging, use bio_lqfs_strategy() instead of bwrite2(),
 * with bwrite_common() code ported from Solaris to bypass
 * the assumption that UFS is the only logging filesystem.
 */

int				/* EIO if error, 0 if successful. */
sam_bwrite2(
	struct sam_mount *mp,	/* Pointer to mount info for QFS logging */
	buf_t *bp)		/* Pointer to buffer header */
{
	int error;

	if (mp && TRANS_ISTRANS(mp) && bio_lqfs_strategy != NULL) {
		klwp_t *lwp = ttolwp(curthread);

		ASSERT(SEMA_HELD(&bp->b_sem));
		bp->b_flags &= ~(B_READ|B_DONE|B_ERROR|B_DELWRI);
		if (lwp != NULL)
			lwp->lwp_ru.oublock++;
		(*bio_lqfs_strategy)(TRANS_ISTRANS(mp), bp);
		(void) biowait(bp);
	} else {
		bwrite2(bp);
	}
	if ((bp->b_flags & B_ERROR) == 0) {
		error = 0;
	} else {
		error = EIO;
	}
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
	int kptr[],			/* ary of extents that end the file */
	int ik,				/* kptr index */
	uint32_t *extent_bn,		/* mass storage extent block number */
	uchar_t *extent_ord,		/* mass storage extent ordinal */
	int level,			/* level of indirection */
	int *set)			/* Set for last indirect block */
{
	struct sam_mount *mp;
	int i;
	buf_t *bp;
	sam_indirect_extent_t *iep;
	sam_daddr_t bn;
	uint32_t *ie_addr;
	uchar_t *ie_ord;
	int error = 0;

	mp = ip->mp;
	bn = *extent_bn;
	bn <<= mp->mi.m_bn_shift;
	if ((error =
	    sam_sbread(ip, *extent_ord, bn, SAM_INDBLKSZ, &bp)) == 0) {
		iep = (sam_indirect_extent_t *)(void *)bp->b_un.b_addr;

		/* check for invalid indirect block */
		if (iep->ieno != (level+1)) {
			error = ENOCSI;
		} else if ((iep->id.ino != ip->di.id.ino) ||
		    (iep->id.gen != ip->di.id.gen)) {
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
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: flush_indirect block:"
				    " ino=%d.%d.%d ib=%d.%d.%d bn=0x%x ord=%d",
				    ip->mp->mt.fi_name, ip->di.id.ino,
				    ip->di.id.gen, level+1,
				    iep->id.ino, iep->id.gen, iep->ieno,
				    *extent_bn, *extent_ord);
				sam_req_ifsck(ip->mp, *extent_ord,
				    "sam_flush_indirect_block: gen",
				    &ip->di.id);

			} else {
				dcmn_err((CE_WARN,
				    "SAM-QFS %s: Client flush_indirect block:"
				    " ino=%d.%d.%d ib=%d.%d.%d bn=0x%x ord=%d",
				    ip->mp->mt.fi_name, ip->di.id.ino,
				    ip->di.id.gen, level+1,
				    iep->id.ino, iep->id.gen, iep->ieno,
				    *extent_bn, *extent_ord));
			}
			brelse(bp);
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
				/*
				 * Flush/Stale large extent block.
				 */
				bn = *ie_addr;
				bn <<= (mp->mi.m_bn_shift + SAM2SUN_BSHIFT);
				if (bc_flag == SAM_STALE_BC) {
					sam_blkstale(
					    mp->mi.m_fs[*ie_ord].dev, bn);
				} else {
					blkflush(mp->mi.m_fs[*ie_ord].dev, bn);
				}
			}
		}
		brelse(bp);
		/*
		 * Flush/Stale large extent block.
		 */
		bn = *extent_bn;
		bn <<= (mp->mi.m_bn_shift + SAM2SUN_BSHIFT);
		if (bc_flag == SAM_STALE_BC) {
			sam_blkstale(mp->mi.m_fs[*extent_ord].dev, bn);
		} else {
			blkflush(mp->mi.m_fs[*extent_ord].dev, bn);
		}
	} else {
		if (!SAM_IS_SHARED_CLIENT(mp)) {
			cmn_err(CE_WARN,
		"SAM-QFS: %s: Indirect block I/O error: ino=%d.%d bn=0x%x "
		"ord=%d",
			    mp->mt.fi_name, ip->di.id.ino, ip->di.id.gen,
			    *extent_bn, *extent_ord);
			sam_req_ifsck(mp, *extent_ord,
			    "sam_flush_indirect_block: gen2", &ip->di.id);
		}
	}
	return (error);
}
