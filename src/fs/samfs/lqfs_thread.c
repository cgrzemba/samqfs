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
#pragma ident	"$Revision: 1.8 $"
#endif

#include <sys/systm.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/sysmacros.h>
#include <sys/debug.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/cmn_err.h>
#define	_LQFS_INFRASTRUCTURE
#include <lqfs_common.h>
#undef _LQFS_INFRASTRUCTURE
#ifdef LUFS
#include <sys/fssnap_if.h>
#include <sys/fs/qfs_inode.h>
#include <sys/fs/qfs_filio.h>
#include <sys/fs/qfs_log.h>
#include <sys/fs/qfs_bio.h>
#endif /* LUFS */
#include <sys/inttypes.h>
#include <sys/callb.h>
#include <sys/tnf_probe.h>

/*
 * Kernel threads for logging
 * Currently only one for rolling the log (one per log).
 */

#define	LQFS_DEFAULT_NUM_ROLL_BUFS 16
#define	LQFS_DEFAULT_MIN_ROLL_BUFS 4
#define	LQFS_DEFAULT_MAX_ROLL_BUFS 64

/*
 * Macros
 */
#define	logmap_need_roll(logmap) ((logmap)->mtm_nme > logmap_maxnme)
#define	ldl_empty(ul) ((ul)->un_head_lof == (ul)->un_tail_lof)

/*
 * Tunables
 */
uint32_t lqfs_num_roll_bufs = LQFS_DEFAULT_NUM_ROLL_BUFS;
uint32_t lqfs_min_roll_bufs = LQFS_DEFAULT_MIN_ROLL_BUFS;
uint32_t lqfs_max_roll_bufs = LQFS_DEFAULT_MAX_ROLL_BUFS;
long logmap_maxnme = 1536;
int trans_roll_tics = 0;
uint64_t trans_roll_new_delta = 0;
uint64_t lrr_wait = 0;
/*
 * Key for thread specific data for the roll thread to
 * bypass snapshot throttling
 */
uint_t bypass_snapshot_throttle_key;

/*
 * externs
 */
extern kmutex_t		ml_scan;
extern kcondvar_t	ml_scan_cv;
extern int		maxphys;

static void
trans_roll_wait(mt_map_t *logmap, callb_cpr_t *cprinfop)
{
	mutex_enter(&logmap->mtm_mutex);
	logmap->mtm_ref = 0;
	if (logmap->mtm_flags & MTM_FORCE_ROLL) {
		cv_broadcast(&logmap->mtm_from_roll_cv);
	}
	logmap->mtm_flags &= ~(MTM_FORCE_ROLL | MTM_ROLLING);
	CALLB_CPR_SAFE_BEGIN(cprinfop);
	(void) cv_timedwait(&logmap->mtm_to_roll_cv, &logmap->mtm_mutex,
	    ddi_get_lbolt() + trans_roll_tics);
	CALLB_CPR_SAFE_END(cprinfop, &logmap->mtm_mutex);
	logmap->mtm_flags |= MTM_ROLLING;
	mutex_exit(&logmap->mtm_mutex);
}

/*
 * returns the number of 8K buffers to use for rolling the log
 */
static uint32_t
log_roll_buffers()
{
	/*
	 * sanity validate the tunable lqfs_num_roll_bufs
	 */
	if (lqfs_num_roll_bufs < lqfs_min_roll_bufs) {
		return (lqfs_min_roll_bufs);
	}
	if (lqfs_num_roll_bufs > lqfs_max_roll_bufs) {
		return (lqfs_max_roll_bufs);
	}
	return (lqfs_num_roll_bufs);
}

/*
 * Find something to roll, then if we don't have cached roll buffers
 * covering all the deltas in that MAPBLOCK then read the master
 * and overlay the deltas.
 * returns;
 * 	0 if sucessful
 *	1 on finding nothing to roll
 *	2 on error
 */
int
log_roll_read(ml_unit_t *ul, rollbuf_t *rbs, int nmblk, caddr_t roll_bufs,
    int *retnbuf)
{
	offset_t	mof;
	offset_t	mof_orig;
	uchar_t		ord;
	buf_t		*bp;
	rollbuf_t	*rbp;
	mt_map_t	*logmap = ul->un_logmap;
	daddr_t		mblkno;
	dev_t		medev;
	int		i;
	int		error;
	int		nbuf;
	qfsvfs_t	*qfsvfsp = ul->un_qfsvfs;

	/*
	 * Make sure there is really something to roll
	 */
	mof = 0;
	ord = 0;
	if (!logmap_next_roll(logmap, &mof, &ord)) {
		return (1);
	}

	/*
	 * build some master blocks + deltas to roll forward
	 */
	rw_enter(&logmap->mtm_rwlock, RW_READER);
	nbuf = 0;
	do {
		mof_orig = mof;
		LQFS_MSG(CE_WARN, "log_roll_read(): Found logmap entry to roll "
		    "mof 0x%x ord %d\n", mof, ord);
		mof = mof & (offset_t)MAPBLOCKMASK;
		mblkno = lbtodb(mof);
		medev = qfsvfsp->mi.m_fs[ord].dev;
		LQFS_MSG(CE_WARN, "log_roll_read(): Adjusted mof to align with "
		    "buffer mof 0x%x (blkno %ld) edev %ld nb 8K\n", mof,
		    mblkno, medev);

		/*
		 * Check for the case of a new delta to a set up buffer
		 */
		for (i = 0, rbp = rbs; i < nbuf; ++i, ++rbp) {
			LQFS_MSG(CE_WARN, "log_roll_read(): Checking if logmap "
			    "entry mof 0x%x (blkno %ld) edev %ld "
			    "starts within the same 8K block as rollbuf entry "
			    "mof 0x%x (blkno %ld) edev %ld (8K blk mof 0x%x)\n",
			    mof_orig, lbtodb(mof_orig), medev,
			    ldbtob(rbp->rb_bh.b_blkno),
			    rbp->rb_bh.b_blkno, rbp->rb_bh.b_edev,
			    ldbtob(rbp->rb_bh.b_blkno) &
			    (offset_t)MAPBLOCKMASK);
#ifdef LUFS
			if ((P2ALIGN(rbp->rb_bh.b_blkno,
			    MAPBLOCKSIZE / DEV_BSIZE) == mblkno) &&
			    (rbp->rb_bh.b_edev == medev)) {
#else
			if (((ldbtob(rbp->rb_bh.b_blkno) &
			    (offset_t)MAPBLOCKMASK) == mblkno) &&
			    (rbp->rb_bh.b_edev == medev)) {
#endif /* LUFS */
				TNF_PROBE_0(trans_roll_new_delta, "lqfs",
				    /* CSTYLED */);
				trans_roll_new_delta++;
				/* Flush out the current set of buffers */
				goto flush_bufs;
			}
		}

		/*
		 * Work out what to roll next. If it isn't cached then read
		 * it asynchronously from the master.
		 */
		bp = &rbp->rb_bh;
		bp->b_blkno = mblkno;
		bp->b_edev = medev;
		bp->b_dev = cmpdev(bp->b_edev);
		bp->b_flags = B_READ;
		bp->b_un.b_addr = roll_bufs + (nbuf << MAPBLOCKSHIFT);
		bp->b_bufsize = MAPBLOCKSIZE;
		LQFS_MSG(CE_WARN, "log_roll_read(): Initializing new roll "
		    "buffer. Reading mof 0x%x edev %ld nb %d\n",
		    ldbtob(mblkno), medev, bp->b_bufsize);
		if (top_read_roll(rbp, ul)) {
			/* logmap deltas were in use */
			if (nbuf == 0) {
				/*
				 * On first buffer wait for the logmap user
				 * to finish by grabbing the logmap lock
				 * exclusively rather than spinning
				 */
				rw_exit(&logmap->mtm_rwlock);
				lrr_wait++;
				rw_enter(&logmap->mtm_rwlock, RW_WRITER);
				rw_exit(&logmap->mtm_rwlock);
				return (1);
			}
			/* we have at least one buffer - flush it */
			goto flush_bufs;
		}
		if ((bp->b_flags & B_INVAL) == 0) {
			nbuf++;
		}
		mof_orig += MAPBLOCKSIZE;
		mof = mof_orig;
	} while ((nbuf < nmblk) && logmap_next_roll(logmap, &mof, &ord));

	/*
	 * If there was nothing to roll cycle back
	 */
	if (nbuf == 0) {
		rw_exit(&logmap->mtm_rwlock);
		return (1);
	}

flush_bufs:
	/*
	 * For each buffer, if it isn't cached then wait for the read to
	 * finish and overlay the deltas.
	 */
	for (error = 0, i = 0, rbp = rbs; i < nbuf; ++i, ++rbp) {
		if (!rbp->rb_crb) {
			bp = &rbp->rb_bh;
			LQFS_MSG(CE_WARN, "log_roll_read(): Sync-reading data "
			    "(no CRB) from log mof 0x%x ord %d nb %d\n",
			    ldbtob(bp->b_blkno) & (offset_t)MAPBLOCKMASK, ord,
			    MAPBLOCKSIZE);
			if (trans_not_wait(bp)) {
				ldl_seterror(ul,
				    "Error reading master during qfs log roll");
				error = 1;
			}

			/*
			 * sync read the data from the log
			 */
			ord = lqfs_find_ord(qfsvfsp, bp);
			if (ldl_read(ul, bp->b_un.b_addr,
			    ldbtob(bp->b_blkno) & (offset_t)MAPBLOCKMASK, ord,
			    MAPBLOCKSIZE, rbp->rb_age)) {
				error = 1;
			}
		}

		/*
		 * reset the age bit in the age list
		 */
		LQFS_MSG(CE_WARN, "log_roll_read(): Calling "
		    "logmap_list_put_roll().\n");
		logmap_list_put_roll(logmap, rbp->rb_age);

		if (ul->un_flags & LDL_ERROR) {
			error = 1;
		}
	}
	rw_exit(&logmap->mtm_rwlock);
	if (error) {
		return (2);
	}
	*retnbuf = nbuf;
	return (0);
}

/*
 * Write out a cached roll buffer
 */
void
log_roll_write_crb(qfsvfs_t *qfsvfsp, rollbuf_t *rbp)
{
	crb_t *crb = rbp->rb_crb;
	buf_t *bp = &rbp->rb_bh;

	bp->b_blkno = lbtodb(crb->c_mof);
	bp->b_edev = qfsvfsp->mi.m_fs[crb->c_ord].dev;
	bp->b_dev = cmpdev(bp->b_edev);
	bp->b_un.b_addr = crb->c_buf;
	bp->b_bcount = crb->c_nb;
	bp->b_bufsize = crb->c_nb;
	ASSERT((crb->c_nb & DEV_BMASK) == 0);
	bp->b_flags = B_WRITE;
	logstats.ls_rwrites.value.ui64++;

	/* if snapshots are enabled, call it */
#ifdef LQFS_TODO_SNAPSHOT
	if (qfsvfsp->vfs_snapshot) {
		fssnap_strategy(&qfsvfsp->vfs_snapshot, bp);
	} else {
#endif /* LQFS_TODO_SNAPSHOT */
		if ((bp->b_flags & B_READ) == 0) {
			LQFS_MSG(CE_WARN, "log_roll_write_crb(): bdev_strategy "
			    "writing mof 0x%x edev %ld nb %d\n",
			    bp->b_blkno * 512, bp->b_edev, bp->b_bcount);
		} else {
			LQFS_MSG(CE_WARN, "log_roll_write_crb(): bdev_strategy "
			    "reading mof 0x%x edev %ld nb %d\n",
			    bp->b_blkno * 512, bp->b_edev, bp->b_bcount);
		}
		(void) bdev_strategy(bp);
#ifdef LQFS_TODO_SNAPSHOT
	}
#endif /* LQFS_TODO_SNAPSHOT */
}

/*
 * Write out a set of non cached roll buffers
 */
/* ARGSUSED0 */
void
log_roll_write_bufs(qfsvfs_t *qfsvfsp, rollbuf_t *rbp)
{
	buf_t		*bp = &rbp->rb_bh;
	buf_t		*bp2;
	rbsecmap_t	secmap = rbp->rb_secmap;
	int		j, k;

	ASSERT(secmap);
	ASSERT((bp->b_flags & B_INVAL) == 0);

	LQFS_MSG(CE_WARN, "log_roll_write(): Entered with 8K bp mof 0x%x "
	    "(blkno %ld)\n", bp->b_blkno*512, bp->b_blkno);
	do { /* for each contiguous block of sectors */
		/* find start of next sector to write */
		for (j = 0; j < 16; ++j) {
			if (secmap & UINT16_C(1))
				break;
			secmap >>= 1;
		}
		bp->b_un.b_addr += (j << DEV_BSHIFT);
		bp->b_blkno += j;

		/* calculate number of sectors */
		secmap >>= 1;
		j++;
		for (k = 1; j < 16; ++j) {
			if ((secmap & UINT16_C(1)) == 0)
				break;
			secmap >>= 1;
			k++;
		}
		bp->b_bcount = k << DEV_BSHIFT;
		bp->b_flags = B_WRITE;
		logstats.ls_rwrites.value.ui64++;

		/* if snapshots are enabled, call it */
#ifdef LQFS_TODO_SNAPSHOT
		if (qfsvfsp->vfs_snapshot) {
			fssnap_strategy(&qfsvfsp->vfs_snapshot, bp);
		} else {
#endif /* LQFS_TODO_SNAPSHOT */
			if ((bp->b_flags & B_READ) == 0) {
				LQFS_MSG(CE_WARN, "log_roll_write_buf(): "
				    "bdev_strategy writing mof 0x%x "
				    "edev %ld nb %d\n", bp->b_blkno * 512,
				    bp->b_edev, bp->b_bcount);
			} else {
				LQFS_MSG(CE_WARN, "log_roll_write_buf(): "
				    "bdev_strategy reading mof 0x%x "
				    "edev %ld nb %d\n", bp->b_blkno * 512,
				    bp->b_edev, bp->b_bcount);
			}
			(void) bdev_strategy(bp);
#ifdef LQFS_TODO_SNAPSHOT
		}
#endif /* LQFS_TODO_SNAPSHOT */
		if (secmap) {
			/*
			 * Allocate another buf_t to handle
			 * the next write in this MAPBLOCK
			 * Chain them via b_list.
			 */
			bp2 = kmem_alloc(sizeof (buf_t), KM_SLEEP);
			bp->b_list = bp2;
			bioinit(bp2);
			bp2->b_iodone = trans_not_done;
			bp2->b_bufsize = MAPBLOCKSIZE;
			bp2->b_edev = bp->b_edev;
			bp2->b_un.b_addr =
			    bp->b_un.b_addr + bp->b_bcount;
			bp2->b_blkno = bp->b_blkno + k;
			bp = bp2;
		}
	} while (secmap);
}

/*
 * Asynchronously roll the deltas, using the sector map
 * in each rollbuf_t.
 */
int
log_roll_write(ml_unit_t *ul, rollbuf_t *rbs, int nbuf)
{

	qfsvfs_t	*qfsvfsp = ul->un_qfsvfs;
	rollbuf_t	*rbp;
	buf_t		*bp, *bp2;
	rollbuf_t	*head, *prev, *rbp2;

	/*
	 * Order the buffers by blkno
	 */
	ASSERT(nbuf > 0);
#ifdef lint
	prev = rbs;
#endif
	for (head = rbs, rbp = rbs + 1; rbp < rbs + nbuf; rbp++) {
		for (rbp2 = head; rbp2; prev = rbp2, rbp2 = rbp2->rb_next) {
			if (rbp->rb_bh.b_blkno < rbp2->rb_bh.b_blkno) {
				if (rbp2 == head) {
					rbp->rb_next = head;
					head = rbp;
				} else {
					prev->rb_next = rbp;
					rbp->rb_next = rbp2;
				}
				break;
			}
		}
		if (rbp2 == NULL) {
			prev->rb_next = rbp;
			rbp->rb_next = NULL;
		}
	}

	/*
	 * issue the in-order writes
	 */
	for (rbp = head; rbp; rbp = rbp2) {
		if (rbp->rb_crb) {
			log_roll_write_crb(qfsvfsp, rbp);
		} else {
			log_roll_write_bufs(qfsvfsp, rbp);
		}
		/* null out the rb_next link for next set of rolling */
		rbp2 = rbp->rb_next;
		rbp->rb_next = NULL;
	}

	/*
	 * wait for all the writes to finish
	 */
	for (rbp = rbs; rbp < rbs + nbuf; rbp++) {
		bp = &rbp->rb_bh;
		if (trans_not_wait(bp)) {
			ldl_seterror(ul,
			    "Error writing master during qfs log roll");
		}

		/*
		 * Now wait for all the "cloned" buffer writes (if any)
		 * and free those headers
		 */
		bp2 = bp->b_list;
		bp->b_list = NULL;
		while (bp2) {
			if (trans_not_wait(bp2)) {
				ldl_seterror(ul,
				    "Error writing master during qfs log roll");
			}
			bp = bp2;
			bp2 = bp2->b_list;
			kmem_free(bp, sizeof (buf_t));
		}
	}

	if (ul->un_flags & LDL_ERROR) {
		return (1);
	}
	return (0);
}

void
trans_roll(ml_unit_t *ul)
{
	callb_cpr_t	cprinfo;
	mt_map_t	*logmap = ul->un_logmap;
	rollbuf_t	*rbs;
	rollbuf_t	*rbp;
	buf_t		*bp;
	caddr_t		roll_bufs;
	uint32_t	nmblk;
	int		i;
	int		doingforceroll;
	int		nbuf;
	uchar_t		ord;
	qfsvfs_t	*qfsvfsp = ul->un_qfsvfs;

	CALLB_CPR_INIT(&cprinfo, &logmap->mtm_mutex, callb_generic_cpr,
	    "trans_roll");

	/*
	 * We do not want the roll thread's writes to be
	 * throttled by the snapshot.
	 * If they are throttled then we can have a deadlock
	 * between the roll thread and the snapshot taskq thread:
	 * roll thread wants the throttling semaphore and
	 * the snapshot taskq thread cannot release the semaphore
	 * because it is writing to the log and the log is full.
	 */

	(void) tsd_set(bypass_snapshot_throttle_key, (void *)1);

	/*
	 * setup some roll parameters
	 */
	if (trans_roll_tics == 0) {
		trans_roll_tics = 5 * hz;
	}
	nmblk = log_roll_buffers();

	/*
	 * allocate the buffers and buffer headers
	 */
	roll_bufs = kmem_alloc(nmblk * MAPBLOCKSIZE, KM_SLEEP);
	rbs = kmem_alloc(nmblk * sizeof (rollbuf_t), KM_SLEEP);

	/*
	 * initialize the buffer headers
	 */
	for (i = 0, rbp = rbs; i < nmblk; ++i, ++rbp) {
		rbp->rb_next = NULL;
		bp = &rbp->rb_bh;
		bioinit(bp);
		bp->b_edev = ul->un_dev;
		bp->b_iodone = trans_not_done;
		bp->b_bufsize = MAPBLOCKSIZE;
	}

	doingforceroll = 0;

again:
	/*
	 * LOOP FOREVER
	 */

	/*
	 * exit on demand
	 */
	mutex_enter(&logmap->mtm_mutex);
	if ((ul->un_flags & LDL_ERROR) || (logmap->mtm_flags & MTM_ROLL_EXIT)) {
		kmem_free(rbs, nmblk * sizeof (rollbuf_t));
		kmem_free(roll_bufs, nmblk * MAPBLOCKSIZE);
		logmap->mtm_flags &= ~(MTM_FORCE_ROLL | MTM_ROLL_RUNNING |
		    MTM_ROLL_EXIT | MTM_ROLLING);
		cv_broadcast(&logmap->mtm_from_roll_cv);
		CALLB_CPR_EXIT(&cprinfo);
		thread_exit();
		/* NOTREACHED */
	}

	/*
	 * MT_SCAN debug mode
	 *	don't roll except in FORCEROLL situations
	 */
	if (logmap->mtm_debug & MT_SCAN)
		if ((logmap->mtm_flags & MTM_FORCE_ROLL) == 0) {
			mutex_exit(&logmap->mtm_mutex);
			trans_roll_wait(logmap, &cprinfo);
			goto again;
		}
	ASSERT(logmap->mtm_trimlof == 0);

	/*
	 * If we've finished a force roll cycle then wakeup any
	 * waiters.
	 */
	if (doingforceroll) {
		doingforceroll = 0;
		logmap->mtm_flags &= ~MTM_FORCE_ROLL;
		mutex_exit(&logmap->mtm_mutex);
		cv_broadcast(&logmap->mtm_from_roll_cv);
	} else {
		mutex_exit(&logmap->mtm_mutex);
	}

	/*
	 * If someone wants us to roll something; then do it
	 */
	if (logmap->mtm_flags & MTM_FORCE_ROLL) {
		doingforceroll = 1;
		LQFS_MSG(CE_WARN, "trans_roll(): Someone wants a forced "
		    "roll.\n");
		goto rollsomething;
	}

	/*
	 * Log is busy, check if logmap is getting full.
	 */
	if (logmap_need_roll(logmap)) {
		LQFS_MSG(CE_WARN, "trans_roll(): Log is busy - logmap getting "
		    "full.\n");
		goto rollsomething;
	}

	/*
	 * Check if the log is idle and is not empty
	 */
	if (!logmap->mtm_ref && !ldl_empty(ul)) {
		LQFS_MSG(CE_WARN, "trans_roll(): Log is idle - not empty.\n");
		goto rollsomething;
	}

	/*
	 * Log is busy, check if its getting full
	 */
	if (ldl_need_roll(ul)) {
		LQFS_MSG(CE_WARN, "trans_roll(): Log is busy - getting "
		    "full.\n");
		goto rollsomething;
	}

	/*
	 * nothing to do; wait a bit and then start over
	 */
	trans_roll_wait(logmap, &cprinfo);
	goto again;

	/*
	 * ROLL SOMETHING
	 */

rollsomething:
	/*
	 * Use the cached roll buffers, or read the master
	 * and overlay the deltas
	 */
	switch (log_roll_read(ul, rbs, nmblk, roll_bufs, &nbuf)) {
	case 1: trans_roll_wait(logmap, &cprinfo);
		/* FALLTHROUGH */
	case 2: goto again;
	/* default case is success */
	}

	/*
	 * Asynchronously write out the deltas
	 */
	LQFS_MSG(CE_WARN, "trans_roll(): Calling log_roll_write\n");
	if (log_roll_write(ul, rbs, nbuf)) {
		goto again;
	}

	/*
	 * free up the deltas in the logmap
	 */
	LQFS_MSG(CE_WARN, "trans_roll(): Freeing logmap deltas\n");
	for (i = 0, rbp = rbs; i < nbuf; ++i, ++rbp) {
		bp = &rbp->rb_bh;
		ord = lqfs_find_ord(qfsvfsp, bp);
		logmap_remove_roll(logmap,
		    ldbtob(bp->b_blkno) & (offset_t)MAPBLOCKMASK, ord,
		    MAPBLOCKSIZE);
	}

	/*
	 * free up log space; if possible
	 */
	logmap_sethead(logmap, ul);

	/*
	 * LOOP
	 */
	goto again;
}
