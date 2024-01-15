/*
 * ----- block.c - Process the disk block functions.
 *
 * Process the SAM-QFS allocate disk block functions. Keep the circular
 * block buffer full.
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

#pragma ident "$Revision: 1.136 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/t_lock.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/cred.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/callb.h>
#include <sys/mount.h>
#include <sys/sysmacros.h>

/* ----- SAMFS Includes */

#include "sam/param.h"
#include "sam/types.h"

#include "macros.h"
#include "sblk.h"
#include "inode.h"
#include "mount.h"
#include "arfind.h"
#include "debug.h"
#include "extern.h"
#include "trace.h"
#include "kstats.h"
#include "qfs_log.h"
#include "behavior_tags.h"
#include "syslogerr.h"

static void sam_block_done(buf_t *bp);
static boolean_t sam_get_sm_blklist(sam_mount_t *mp);
static int sam_get_blklist(sam_mount_t *mp, int ord);
static int sam_get_disk_blks(sam_mount_t *mp, int ord);
static int sam_set_block(sam_mount_t *mp, int ord, buf_t *bp, int off,
		int rblk, int *bt, int *modified, sam_bn_t *next_dau);
static void sam_fill_block_pool(sam_mount_t *mp, struct sam_block *block,
		int nblocks, sam_daddr_t bn);
static void sam_write_map(sam_mount_t *mp, struct sam_block *block, buf_t *bp,
		int ord, int modblocks, sam_bn_t next_dau);
static void sam_set_sblk(sam_mount_t *mp, struct sam_sblk *sblk, int ord,
		int modified, sam_bn_t next_dau);
static void sam_block_thread_exit(sam_mount_t *mp, callb_cpr_t *cprinfo);
static void sam_process_prealloc_req(sam_mount_t *mp, struct sam_sblk *sblk,
		sam_prealloc_t *pap, boolean_t check_system);
static int sam_get_prealloc_daus(sam_mount_t *mp, struct sam_sblk *sblk,
		sam_bn_t blk, int off, sam_bn_t *next_dau, sam_prealloc_t *pap,
		boolean_t check_system);
static offset_t sam_count_block(buf_t *bp);
static void sam_reset_fs_space(sam_mount_t *mp);
static void sam_reset_eq_space(sam_mount_t *mp, int ord);
static void sam_drain_free_list(sam_mount_t *mp, struct samdent *dp);
static void sam_change_state(sam_mount_t *mp, struct samdent *dp, int ord);
static void sam_wait_free_blocks(struct sam_mount *mp);
static void sam_set_lun_state(struct sam_mount *mp, int ord, uchar_t state);
static int sam_grow_fs(sam_mount_t *mp, struct samdent *dp, int ord);
static void sam_delete_blocklist(struct sam_block **blockp);
static void sam_init_blocklist(sam_mount_t *mp, uchar_t ord);
static int sam_write_label(sam_mount_t *mp, struct samdent *dp, int ord);
static int sam_remove_from_allocation_links(sam_mount_t *mp, int ord);

int sam_update_the_sblks(sam_mount_t *mp);

extern int sam_bfmap(sam_caller_t caller, struct sam_sblk *sblk, int ord,
	dev_t meta_dev, char *dcp, char *cptr, int bits);
extern int sam_cablk(sam_caller_t caller, struct sam_sblk *sblk, void *vp,
	int ord, int bits, int mbits, uint_t dd_kblocks, uint_t mm_kblocks,
	int *lenp);
extern int sam_check_osd_daus(sam_mount_t *mp);

#pragma rarely_called(sam_block_thread_exit, sam_process_prealloc_req)
#pragma does_not_return(sam_block_thread_exit)

/*
 * ----- sam_block_thread - async thread that gets available blocks.
 *
 *  This async thread makes sure the block buffers are full.
 *
 * The block pools are maintained as a circular queue, with three pointers
 * (in, out, and fill) and a limit.
 *
 * The queue has three areas defined by these pointers (wrapping around at
 * limit):
 *
 *   [out, in) => blocks available for allocation.
 *   [in, fill) => blocks being allocated.
 *   [fill, out) => unused.
 *
 * Allocation takes two passes.  In the first pass, block candidates are
 * identified by reading the bitmap, and the updated bitmap is scheduled
 * to be written to disk.  These candidates reside in the [in, fill) range
 * of the queue.  In the second pass, the candidates are made available
 * for allocation by being moved to the [out, in) range of the queue.
 * This guarantees that blocks are not allocated before the bitmap has
 * been updated on disk, so that we will be consistent in the case of
 * disk failures or system crashes.
 *
 */

void
sam_block_thread(sam_mount_t *mp)
{
	callb_cpr_t cprinfo;

	/*
	 * Setup the CPR callback for suspend/resume.
	 */
	CALLB_CPR_INIT(&cprinfo, &mp->mi.m_block.put_mutex, callb_generic_cpr,
	    "sam_block_thread");

	mutex_enter(&mp->mi.m_block.mutex);
	mp->mi.m_block.state = SAM_THR_EXIST;
	/* Thread started, signal mnt thread */
	cv_signal(&mp->mi.m_block.get_cv);

	for (;;) {
		boolean_t blocks_allocated;
		boolean_t io_outstanding;
		int number_buffers, buf_busy;
		int ord;
		struct sam_prealloc *next;

		/*
		 * Exit thread if umount in progress.
		 */
		if (mp->mi.m_block.flag == SAM_THR_UMOUNT) {
			mutex_exit(&mp->mi.m_block.mutex);
			(void) sam_block_thread_exit(mp, &cprinfo);
		}

		SAM_COUNT64(thread, block_count);

		blocks_allocated = FALSE;
		io_outstanding = FALSE;

		/*
		 * If I/O completed with no error, update block "in" pointer.
		 */
		for (ord = 0; ord < mp->mt.fs_count; ord++) {
			struct samdent *dp;

			dp = &mp->mi.m_fs[ord];
			if (dp->skip_ord || is_osd_group(dp->part.pt_type)) {
				continue;
			}
			if (dp->busy) {		/* I/O in progress */
				io_outstanding = TRUE;
				continue;
			}
			if (dp->modified) { /* I/O completed with no error */
				int modified = dp->modified;
				sam_bn_t next_dau = dp->next_dau;
				int bt;

				/* Commit blocks */
				for (bt = 0; bt < SAM_MAX_DAU; bt++) {
					struct sam_block *block;

					if ((block = dp->block[bt]) == NULL) {
						continue;
					}
					block->in = block->fill;
					blocks_allocated = TRUE;
				}
				dp->modified = 0;
				mutex_exit(&mp->mi.m_block.mutex);
				sam_set_sblk(mp, mp->mi.m_sbp, ord,
				    modified, next_dau);
				mutex_enter(&mp->mi.m_block.mutex);

				/* Wake up anyone waiting for blocks */
				/* If threads waiting for blocks */
				if (mp->mi.m_block.wait) {
					DTRACE_PROBE1(cv__broadcast__bl__wait, sam_mount_t, mp);
					cv_broadcast(&mp->mi.m_block.get_cv);
				}
			}
		}
		mutex_exit(&mp->mi.m_block.mutex);

		/*
		 * Fill the buffers in the circular buffer.
		 */
		if ((mp->mt.fi_config1 & MC_SMALL_DAUS)) {
			/*
			 * If any buffer busy, cannot get .blocks
			 * small blocks
			 */
			if (!io_outstanding) {
				/*
				 * Get all the small blocks from the
				 * .blocks hidden file
				 */
				if (sam_get_sm_blklist(mp)) {
					blocks_allocated = TRUE;
				}
			}
		} else {
			/*
			 * Ok to fill any ordinal that is not busy if no
			 * small
			 */
			io_outstanding = FALSE;
		}
		if (!io_outstanding) {
			for (ord = 0; ord < mp->mt.fs_count; ord++) {
				struct samdent *dp;

				/*
				 * Get blocks from the bit maps if bit map
				 * is not empty.
				 */
				dp = &mp->mi.m_fs[ord];
				if (dp->skip_ord) {
					continue;
				}

				if (dp->command != DK_CMD_null) {
					sam_change_state(mp, dp, ord);
					if (dp->part.pt_state != DEV_ON) {
						continue;
					}
				}

				if (dp->map_empty ||
				    is_osd_group(dp->part.pt_type)) {
					continue;
				}
				if (sam_get_blklist(mp, ord)) {
					blocks_allocated = TRUE;
				}
			}
		}

		/*
		 * Process any preallocation requests.
		 */
		mutex_enter(&mp->mi.m_block.mutex);
		while ((next = mp->mi.m_prealloc) != NULL) {
			/* Update start of list */
			mp->mi.m_prealloc = next->next;
			mutex_exit(&mp->mi.m_block.mutex);
			next->next = 0;
			if (mp->mi.m_fs[next->ord].part.pt_state == DEV_ON) {
				sam_process_prealloc_req(mp, mp->mi.m_sbp,
				    next, TRUE);
			} else if (mp->mi.m_fs[next->ord].part.pt_state ==
			    DEV_NOALLOC) {
				next->error = ENOSPC;
			} else {
				next->error = ENODEV;
			}

			/*
			 * Signal threads waiting on preallocation
			 */
			mutex_enter(&mp->mi.m_block.mutex);
			next->wait = 0;
			DTRACE_PROBE1(cv__broadcast__prealloc, sam_mount_t, mp);
			cv_broadcast(&mp->mi.m_block.get_cv);
			blocks_allocated = TRUE;
		}

		/*
		 * Check for all buffers at least half_full, empty, or
		 * busy with I/O.
		 */
		number_buffers = 0;
		buf_busy = 0;
		for (ord = 0; ord < mp->mt.fs_count; ord++) {
			struct samdent *dp;
			int bt;

			dp = &mp->mi.m_fs[ord];
			if (dp->skip_ord || dp->map_empty ||
			    is_osd_group(dp->part.pt_type)) {
				continue;
			}
			for (bt = 0; bt < SAM_MAX_DAU; bt++) {
				struct sam_block *block;
				int block_count;

				if ((block = dp->block[bt]) == NULL) {
					continue;
				}
				number_buffers++;
				block_count = BLOCK_COUNT(block->fill,
				    block->out, block->limit);
				DTRACE_PROBE2(blocks, int, block_count,
				    struct sam_block, block);
				if ((block_count >= (block->limit >> 1)) ||
				    dp->busy ||
				    (block_count == 0)) {
					/*
					 * Half full, I/O in progress, or
					 * no space
					 */
					buf_busy++;
				}
			}
		}

		/*
		 * Wake up any thread waiting to allocate blocks.
		 *
		 * We do this
		 * on each pass, whether or not we have allocated blocks,
		 * because a thread can sleep on space even when there are
		 * blocks in the pools (new blocks can be added to the pools
		 * between when we check for space and when we sleep).
		 */
		mp->mi.m_blkth_ran = ddi_get_lbolt();
		if (blocks_allocated) {
			mp->mi.m_blkth_alloc = mp->mi.m_blkth_ran;
			/* If threads waiting for blocks */
			if (mp->mi.m_block.wait) {
				DTRACE_PROBE1(cv__broadcast__wait, sam_mount_t, mp);
				cv_broadcast(&mp->mi.m_block.get_cv);
			}
			if (SAM_IS_SHARED_WRITER(mp)) {
				mutex_exit(&mp->mi.m_block.mutex);
				(void) sam_update_sblk(mp, 0, 0, TRUE);
				mutex_enter(&mp->mi.m_block.mutex);
			}
		}
		/*
		 * Wake threads up waiting because of ENOSPC
		 */
		if (mp->mi.m_wait_write) {
			mutex_enter(&mp->ms.m_waitwr_mutex);
			if (mp->mi.m_wait_write) {
				DTRACE_PROBE1(cv__broadcast__wait__enospc, sam_mount_t, mp);
				cv_broadcast(&mp->ms.m_waitwr_cv);
			}
			mutex_exit(&mp->ms.m_waitwr_mutex);
		}

		/*
		 * Wait if not umounting and if all buffers are at least
		 * half_full/empty/busy.
		 * Check i/o completion and don't wait if i/o is completed.
		 * Prior to waiting, notify CPR that this thread is suspended.
		 * After waking up, notify CPR that this thread has resumed.
		 */
		if ((mp->mi.m_block.flag != SAM_THR_UMOUNT) &&
		    ((number_buffers == buf_busy) || io_outstanding)) {
			boolean_t nowait = FALSE;

			for (ord = 0; ord < mp->mt.fs_count; ord++) {
				struct samdent *dp;

				dp = &mp->mi.m_fs[ord];
				if (dp->skip_ord || dp->busy ||
				    is_osd_group(dp->part.pt_type)) {
					continue;
				}
				if (dp->modified) {
					nowait = TRUE;
					break;
				}
			}
			if (nowait) {
				continue;
			}

			mutex_enter(&mp->mi.m_block.put_mutex);
			mp->mi.m_block.put_wait = 1;
			mutex_exit(&mp->mi.m_block.mutex);

			CALLB_CPR_SAFE_BEGIN(&cprinfo);

			cv_wait(&mp->mi.m_block.put_cv,
			    &mp->mi.m_block.put_mutex);

			CALLB_CPR_SAFE_END(&cprinfo,
			    &mp->mi.m_block.put_mutex);

			mp->mi.m_block.put_wait = 0;
			mutex_exit(&mp->mi.m_block.put_mutex);
			mutex_enter(&mp->mi.m_block.mutex);
		}
	}
}


/*
 * ----- sam_block_thread_exit - async thread that terminates.
 *
 *  Drain the buffers out of the free list and kill this thread.
 */

static void
sam_block_thread_exit(sam_mount_t *mp, callb_cpr_t *cprinfo)
{
	int ord;
	struct samdent *dp;

	/*
	 * Wait for all I/O to complete.
	 */
	mutex_enter(&mp->mi.m_block.mutex);
	for (ord = 0; ord < mp->mt.fs_count; ord++) {
		dp = &mp->mi.m_fs[ord];
		if (dp->skip_ord) {
			continue;
		}
		while (dp->busy) {		/* I/O in progress */
			mutex_enter(&mp->mi.m_block.put_mutex);
			mp->mi.m_block.put_wait = 1;
			mutex_exit(&mp->mi.m_block.mutex);
			cv_wait(&mp->mi.m_block.put_cv,
			    &mp->mi.m_block.put_mutex);
			mp->mi.m_block.put_wait = 0;
			mutex_exit(&mp->mi.m_block.put_mutex);
			mutex_enter(&mp->mi.m_block.mutex);
		}
		if (dp->modified) {
			int modified = dp->modified;
			sam_bn_t next_dau = dp->next_dau;
			int bt;

			for (bt = 0; bt < SAM_MAX_DAU; bt++) {
				struct sam_block *block;

				if ((block = dp->block[bt]) == NULL) {
					continue;
				}
				block->in = block->fill;
			}
			dp->modified = 0;
			mutex_exit(&mp->mi.m_block.mutex);
			sam_set_sblk(mp, mp->mi.m_sbp, ord, modified,
			    next_dau);
			mutex_enter(&mp->mi.m_block.mutex);
		}
	}
	mutex_exit(&mp->mi.m_block.mutex);

	/*
	 * Drain the buffers out of the free list.
	 */
	for (ord = 0; ord < mp->mt.fs_count; ord++) {
		dp = &mp->mi.m_fs[ord];
		if (dp->skip_ord) {
			continue;
		}
		sam_drain_free_list(mp, dp);
	}

	mutex_enter(&mp->mi.m_block.mutex);
	mp->mi.m_block.state = SAM_THR_DEAD;	/* change states */

	/*
	 * Signal anyone waiting. Threads should check state and exit
	 * accordingly.
	 */
	cv_broadcast(&mp->mi.m_block.get_cv);
	mutex_exit(&mp->mi.m_block.mutex);

	/*
	 * Cleanup CPR on exit. This also releases block.put_mutex.
	 */
	mutex_enter(&mp->mi.m_block.put_mutex);
	CALLB_CPR_EXIT(cprinfo);

	thread_exit();
}


/*
 * ----- sam_drain_free_list - Empty specified block list.
 *
 *  sam_drain_free_list returns to the reservation map (via sam_free_block)
 *	all entries on both block lists for the specified device. This routine
 *	is used during unmount and noallocation actions.
 */

static void
sam_drain_free_list(
	sam_mount_t *mp,		/* Pointer to the mount table. */
	struct samdent *dp)		/* mount point device entry */
{
	struct sam_block *block;	/* Pointer to the block list. */
	int bt;						/* block type */

	/*
	 * Drain the buffers out of the free list.
	 */
	if (dp->skip_ord) {
		return;
	}
	/*
	 * For any blocks on this equipment.
	 */
	for (bt = 0; bt < SAM_MAX_DAU; bt++) {
		if ((block = dp->block[bt]) == NULL) {
			continue;
		}
		mutex_enter(&block->md_mutex);
		/*
		 * Iterate through the block list, and free 'em.
		 */
		while (block->out != block->in) {
			sam_free_block(mp, bt, block->bn[block->out],
			    block->ord);
			block->bn[block->out] = 0;
			if (++block->out >= block->limit) {
				block->out = 0;
			}
		}
		mutex_exit(&block->md_mutex);
	}
}


/*
 * ----- sam_process_prealloc_req - process preallocation request.
 *
 * Process preallocation request for a sequential list of blocks.
 */

static void
sam_process_prealloc_req(
	sam_mount_t *mp,
	struct sam_sblk *sblk,	/* Pointer to the superblock */
	sam_prealloc_t *pap,	/* Pointer to the preallocation request. */
	boolean_t check_system)	/* Check block <= system */
{
	int ord;
	sam_bn_t blk;		/* Current bit map block for this unit */
	int off;
	sam_bn_t next_dau;
	boolean_t lock_set = FALSE;

	pap->first_bn = 0;
	ord = pap->ord;
	pap->count0 = pap->count;
	pap->error = 0;

	if (mutex_owner(&mp->mi.m_fs[ord].eq_mutex) != curthread) {
		mutex_enter(&mp->mi.m_fs[ord].eq_mutex);
		lock_set = TRUE;
	}

	blk = sblk->eq[ord].fs.allocmap;
	off = 0;
	next_dau = 0;
	pap->set = 0;
	pap->error = sam_get_prealloc_daus(mp, sblk, blk, off, &next_dau, pap,
	    check_system);
	if (pap->error == 0) {
		pap->first_bn = 0;
		pap->count = pap->count0;
		off = pap->first_dau;
		off = off >> NBBYSHIFT;
		blk = (sblk->eq[ord].fs.allocmap + (off >> SAM_DEV_BSHIFT));
		off = off & SAM_LOG_WMASK;
		pap->set = 1;
		pap->error = sam_get_prealloc_daus(mp, sblk, blk, off,
		    &next_dau, pap, check_system);
		if (pap->error == 0) {
			/* Don't change next_dau */
			next_dau = sblk->eq[ord].fs.dau_next;
			sam_set_sblk(mp, sblk, ord, pap->count0, next_dau);
		}
	}
	if (lock_set) {
		mutex_exit(&mp->mi.m_fs[ord].eq_mutex);
	}
}


/*
 * ----- sam_get_prealloc_daus - get daus from current block.
 *
 * Returns large daus for direct (preallocated) requests.
 */

static int			/* 0 = done, ENOSPC = keep going */
sam_get_prealloc_daus(
	sam_mount_t *mp,	/* Pointer to the mount table. */
	struct sam_sblk *sblk,	/* Pointer to the superblock */
	sam_bn_t blk,		/* Current bit map block for this ord */
	int off,		/* Offset in the current buffer */
	sam_bn_t *next_dau,	/* Next dau to be used */
	sam_prealloc_t *pap,	/* Pointer to preallocation parameter block */
	boolean_t check_system)	/* Check block <= system */
{
	int dt;
	int ord;
	int count;		/* Number of words left */
	int bit;
	uint_t *wptr, mask;
	sam_bn_t bn;
	buf_t *bp;
	struct buf *tbp, *buflist = NULL;
	int num;
	int am_setting;
	int first_bn_set;
	int blk0;	/* Starting bit map block for this ord */
	offset_t ndau;

	am_setting = pap->set;
	first_bn_set = 0;
	while (pap->count > 0) {
		/*
		 * Start searching a new block.
		 */
		dt = pap->dt;
		ord = pap->ord;
		blk0 = sblk->eq[ord].fs.allocmap;
		if (SAM_BREAD(mp, mp->mi.m_fs[sblk->eq[ord].fs.mm_ord].dev,
		    blk, SAM_DEV_BSIZE, &bp)) {
			pap->error = EIO;
			break;
		}
		bp->av_back = NULL;
		if (buflist != NULL) {
			bp->av_forw = buflist;
			bp->av_forw->av_back = bp;
		}
		buflist = bp;

		num = blk - blk0 + 1;
		wptr = (uint_t *)(void *)(bp->b_un.b_addr + off);
		/* 32 bit words left in buf */
		count = (SAM_DEV_BSIZE - off) >> NBPWSHIFT;
		for (; count > 0; wptr++, count--) {
			/* if no free blocks (this 32) */
			if (!*wptr) {
				continue;
			}
			for (bit = 32, mask = 0x80000000; bit > 0;
			    bit--, mask >>= 1) {
				if (*wptr & mask) {
					/* a fresh start */
					if (first_bn_set == 0) {
						*next_dau = (num <<
						    SAM_BIT_SHIFT) -
						    (count << NBWDSHIFT) +
						    (32 - bit);
						ndau = (uint_t)*next_dau;
						if (DIF_LG_SHIFT(mp, dt)) {
							ndau = ndau <<
							    DIF_LG_SHIFT(mp,
							    dt);
						} else {
							ndau = (ndau *
							    LG_BLK(mp, dt)) >>
							    SAM_DEV_BSHIFT;
						}
						bn = (uint_t)ndau;
						if ((check_system &&
						    (ndau <
						    sblk->eq[ord].fs.system)) ||
						    ndau >
						    sblk->eq[ord].fs.capacity) {
							cmn_err(CE_WARN,
							    "SAM-QFS: %s: "
							    "sam_get_daus:"
							    " bn=0x%llx, "
							    "ord=%d, "
							    "capacity=0x%llx "
							    "KB",
							    mp->mt.fi_name,
							    ndau, ord,
							    sblk->eq[
							    ord].fs.capacity);
							sam_req_fsck(mp, ord,
							    "sam_get_daus: "
							    "bad block");
							continue;
						}
						pap->first_bn = bn;
						first_bn_set = 1;
						pap->first_dau = *next_dau;
					}
					if (bit == 32 && pap->count >= 32 &&
					    *wptr == 0xffffffff) {
						if (am_setting) {
							/*
							 * mark 32 DAUs as
							 * allocated
							 */
							*wptr = 0;
							*next_dau =
							    *next_dau + 32;
						}
						pap->count -= 32;
						bit = 0;
					} else {
						if (am_setting) {
							*wptr &= ~mask;
							*next_dau =
							    *next_dau + 1;
						}
						pap->count--;
					}
					/* if allocation complete */
					if (pap->count <= 0) {
						goto completed;
					}
				} else {
					pap->first_bn = 0; /* Restart search */
					first_bn_set = 0;
					pap->count = pap->count0;
					if (buflist) {
						tbp = buflist;
						ASSERT(tbp == bp);
						while ((tbp = bp->av_forw) !=
						    NULL) {
							bp->av_forw =
							    tbp->av_forw;
							tbp->av_forw =
							    tbp->av_back = NULL;
							brelse(tbp);
						}
					}
				}
			}
		}
		/* end of buffer processing */
		if (first_bn_set == 0) { /* return it, if not a candidate */
			ASSERT(buflist == bp);
			buflist = bp->av_forw = bp->av_back = NULL;
			brelse(bp);
		}
		if ((++blk) >= (blk0 + sblk->eq[ord].fs.l_allocmap)) {
			pap->error = ENOSPC;
			break;
		}
		off = 0;
	}

	/*
	 * Release and write (if needed) all the buffers.
	 */
completed:
	if (pap->error == 0) {
		ASSERT(buflist != NULL);
		/*
		 * If setting, pop buffers from the "end" of the list, since
		 * that the list is in reverse block order.
		 */
		if (am_setting) {
			int error, not_done = 1;

			for (bp = buflist; bp->av_forw != NULL;
			    bp = bp->av_forw) {
				;
			}
			while (not_done) {
				if ((tbp = bp->av_back) == NULL) {
					not_done = 0;
					buflist = NULL;
				} else
					tbp->av_forw = NULL;

				bp->av_back = bp->av_forw = NULL;
				if ((error = SAM_BWRITE(mp, bp)) != 0) {
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: I/O error writing "
					    "map blocks <%d>",
					    mp->mt.fi_name, error);
					pap->error = EIO;
					break;
				}
				bp = tbp;
			}
		}
	}
	/* If any buffers remain, release 'em. */
	while ((bp = buflist) != NULL) {
		buflist = bp->av_forw;
		bp->av_back = bp->av_forw = NULL;
		brelse(bp);
	}
	return (pap->error);
}


/*
 * ----- sam_get_sm_blklist - allocate small block list.
 *
 * Build list of free blocks in buffer. Read .blocks hidden
 * file and fill block buffer.
 */

static boolean_t
sam_get_sm_blklist(sam_mount_t *mp)
{
	sam_node_t *ip;		/* Inode for .blocks hidden file */
	struct sam_block *block;	/* Pointer to the block table. */
	offset_t offset = 0;
	buf_t *bp = NULL;
	struct sam_inoblk *smp;
	sam_ioblk_t ioblk;
	sam_daddr_t bnx;
	int i, j, in;
	boolean_t modified = FALSE;
	int half_full;
	int bn_shift = mp->mi.m_bn_shift;

	half_full = 0;
	for (i = 0; i < mp->mt.fs_count; i++) {
		if ((block = mp->mi.m_fs[i].block[SM]) == NULL) {
			half_full++;
			continue;
		}
		block->fill = block->in;
		block->block_count = BLOCK_COUNT(block->in, block->out,
		    block->limit);
		if (block->block_count >= (block->limit >> 1)) {
			half_full++;	/* If half full */
		}
	}
	if (half_full == mp->mt.fs_count)
		return (0);
	ip = mp->mi.m_inoblk;
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	while (offset < ip->di.rm.size) {
		if (sam_map_block(ip, offset, (offset_t)SM_BLK(mp, MM),
		    SAM_READ, &ioblk, CRED())) {
			goto out;
		}
		ioblk.blkno += ioblk.pboff >> SAM_DEV_BSHIFT;
		if ((SAM_BREAD(mp, mp->mi.m_fs[ioblk.ord].dev, ioblk.blkno,
		    (long)SM_BLK(mp, MM), &bp))) {
			goto out;
		}
		for (i = 0, smp = (struct sam_inoblk *)(void *)bp->b_un.b_addr;
		    i < SM_BLK(mp, MM);
				i += sizeof (struct sam_inoblk), smp++) {
			if (smp->bn == 0xffffffff) {	/* End of list */
				goto out;
			}
			if (smp->bn && smp->free) {	/* if nonempty entry */
				if (smp->ord >= mp->mt.fs_count) {
					block = NULL;
				} else {
					block = mp->mi.m_fs[
						smp->ord].block[SM];
				}
				if (block == NULL) {
					if (mp->mi.m_fs[
						smp->ord].part.pt_state !=
						DEV_NOALLOC) {
						cmn_err(CE_WARN,
						"SAM-QFS: %s: "
						"sam_get_sm_blklist, "
						"bn=0x%x, bad ord=%d",
							mp->mt.fi_name,
							smp->bn, smp->ord);
						sam_req_fsck(mp, -1,
							"sam_get_sm_blklist: "
							"bad block ordinal");
						smp->free = 0;
						smp->bn = 0;
						smp->ord = 0;
						modified = TRUE;
					}
					continue;
				}

				bnx = smp->bn;
				bnx <<= bn_shift;


	/* N.B. Bad indentation here to meet cstyle requirements */
	for (j = 0; j < SM_BLKCNT(mp, block->dt); j++) {
		if (smp->free & (1 << j)) {
			if ((bnx < mp->mi.m_sbp->eq[smp->ord].fs.system) ||
			    (bnx > mp->mi.m_sbp->eq[smp->ord].fs.capacity)) {
				cmn_err(CE_WARN,
			"SAM-QFS: %s: sam_get_sm_blklist, bn=0x%llx, ord=%d",
				    mp->mt.fi_name, bnx, smp->ord);
				sam_req_fsck(mp, smp->ord,
				    "sam_get_sm_blklist: bad block");
				smp->free = 0;
				smp->bn = 0;
				smp->ord = 0;
				modified = TRUE;
				break;
			}
			in = (block->fill + 1 < block->limit) ?
					block->fill + 1 : 0;
			if (in != block->out) {
				block->bn[block->fill] = bnx >> bn_shift;
				block->fill = in;
				smp->free &= ~(1 << j);
				if ((mp->mi.m_inoblk_blocks -=
					SM_DEV_BLOCK(mp, block->dt)) < 0) {
					mp->mi.m_inoblk_blocks = 0;
				}
				modified = TRUE;
			}
		}
		bnx += SM_DEV_BLOCK(mp, block->dt);
	}


				if (smp->free == 0) {
					smp->bn = 0;
					smp->ord = 0;
				}
			}
		}
		offset += SM_BLK(mp, MM);
		if (modified) {
			/* If no error, return blocks */
			if (SAM_BWRITE(mp, bp) == 0) {
				for (i = 0; i < mp->mt.fs_count; i++) {
					if ((block =
					    mp->mi.m_fs[i].block[SM]) ==
					    NULL) {
						continue;
					}
					/* Return these blocks */
					block->in = block->fill;
					block->block_count =
					    BLOCK_COUNT(block->in,
					    block->out, block->limit);
				}
			}
		} else {
			brelse(bp);
		}
		bp = NULL;
	}
out:
	if (modified) {
		TRANS_INODE(ip->mp, ip);
		sam_mark_ino(ip, (SAM_UPDATED | SAM_CHANGED));
		if (bp != NULL) {
			/* If no error, return blocks */
			if (SAM_BWRITE(mp, bp) == 0) {
				for (i = 0; i < mp->mt.fs_count; i++) {
					if ((block =
					    mp->mi.m_fs[i].block[SM]) ==
					    NULL) {
						continue;
					}
					/* Return these blocks */
					block->in = block->fill;
					block->block_count =
					    BLOCK_COUNT(block->in,
					    block->out, block->limit);
				}
			}
		}
	} else {
		if (bp != NULL)  brelse(bp);
	}
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	return (modified);
}


/*
 * ----- sam_get_blklist - allocate block list.
 *
 * Build list of free blocks in buffer. Get
 * blocks from the bit maps.
 */

static int
sam_get_blklist(
	sam_mount_t *mp,	/* Pointer to the mount table. */
	int ord)		/* Current ordinal */
{
	struct samdent *dp;	/* Pointer to device entry in mount table */

	/*
	 * If busy, cannot get anymore blocks
	 */
	dp = &mp->mi.m_fs[ord];
	if (dp->busy || dp->modified)  {
		return (0);		/* Ordinal is busy with I/O */
	}
	return (sam_get_disk_blks(mp, ord));
}


/*
 * ----- sam_fill_block_pool - put block in circular block queue.
 */

static void
sam_fill_block_pool(
	sam_mount_t *mp,		/* Pointer to the mount table. */
	struct sam_block *block,	/* Pointer to the block table. */
	int nblocks,
	sam_daddr_t bn)
{
	int num;
	int in;
	int bn_shift = block->mp->mi.m_bn_shift;

	for (num = 0; num < nblocks; num++) {
		in = block->fill+1 < block->limit ? block->fill+1 : 0;
		if (in != block->out) {
			block->bn[block->fill] = bn >> bn_shift;
			block->fill = in;
		} else {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: BLOCK WRAP, fill=%d, bc=%d, out=%d",
			    mp->mt.fi_name, block->fill,
			    BLOCK_COUNT(block->fill, block->out, block->limit),
			    block->out);
			sam_req_fsck(block->mp, block->ord,
			    "sam_fill_block_pool: wrap");
		}
		bn += SM_DEV_BLOCK(block->mp, block->dt);
	}
}


/*
 * ----- sam_get_disk_blks - allocate block list.
 *
 * Build list of free blocks in buffer. Allocate forever forward and
 * wrap around once.
 */

static int
sam_get_disk_blks(sam_mount_t *mp, int ord)
{
	struct sam_sblk *sblk;
	struct sam_block *block;	/* Pointer to the block table. */
	int bt;
	sam_bn_t blk;		/* Current bit map block for this unit */
	sam_bn_t blk0;		/* Starting bit map block for this unit */
	int nblks;		/* Number of blocks left before wrap or done */
	int offset, off;
	int num, wrap;
	int modified;
	sam_bn_t next_dau;
	buf_t *bp;
	int daus_allocated = 0;

	sblk = mp->mi.m_sbp;
	/*
	 * Initialize fill and save initial block count, proceed to fill
	 * pool if it is less than half full.
	 */
	for (bt = 0; bt < SAM_MAX_DAU; bt++) {
		if ((block = mp->mi.m_fs[ord].block[bt]) == NULL) {
			continue;
		}
		block->fill = block->in;
		block->block_count = BLOCK_COUNT(block->in, block->out,
		    block->limit);
		if (block->block_count >= (block->limit >> 1)) {
			continue;
		}

		mutex_enter(&mp->mi.m_fs[ord].eq_mutex);
		next_dau = sblk->eq[ord].fs.dau_next;
		offset = next_dau >> NBBYSHIFT;
		blk0 = sblk->eq[ord].fs.allocmap;
		blk = (blk0 + (offset >> SAM_DEV_BSHIFT));
		off = offset & SAM_LOG_WMASK;
		/* num = number of bytes from beginning of current block */
		num = ((sblk->eq[ord].fs.dau_size + (NBBY - 1)) >> NBBYSHIFT) -
		    ((blk - blk0) << SAM_DEV_BSHIFT);
		nblks = (num + (SAM_DEV_BSIZE - 1)) >> SAM_DEV_BSHIFT;

		wrap = 0;
		modified = 0;
		for (;;) {
			if (SAM_BREAD(mp,
			    mp->mi.m_fs[sblk->eq[ord].fs.mm_ord].dev,
			    blk, SAM_DEV_BSIZE, &bp)) {
				break;
			}
			num = blk - blk0 + 1;
			if ((sam_set_block(mp, ord, bp, off, num, &bt,
			    &modified, &next_dau))) {
				break;
			}
			daus_allocated += modified;
			blk++;
			off = 0;
			/* If more blocks before wrap */
			if (--nblks > 0) {
				continue;
			}
			if (wrap) {
				mp->mi.m_fs[ord].map_empty = 1;
				break;		/* Map is empty, done */
			}
			offset = 0;
			/* num = (dau_next + 7) / 32 */
			num = ((sblk->eq[ord].fs.dau_next +
			    (NBBY - 1)) >> NBBYSHIFT);
			nblks = (num + (SAM_DEV_BSIZE - 1)) >> SAM_DEV_BSHIFT;
			blk = blk0;
			wrap = 1;
		}
		mutex_exit(&mp->mi.m_fs[ord].eq_mutex);
	}
	return (daus_allocated);
}


/*
 * ----- sam_set_block - get daus from current bit map block.
 *
 * Return daus to disk ordinal for small and large daus.
 * If any blocks returned and both buffers are full,
 * last I/O is async.
 */

static int			/* 1 = done, 0 = keep going */
sam_set_block(
	sam_mount_t *mp,	/* Pointer to the mount table. */
	int ord,		/* Current ordinal */
	buf_t *bp,
	int off,		/* Offset in the current buffer */
	int rblk,		/* Relative block offset */
	int *bt,		/* Current block type, SM/LG */
	int *modified,		/* Number of dau acquired */
	sam_bn_t *next_dau)	/* Next dau to be used */
{
	struct sam_block *block;	/* Pointer to the block table. */
	int count;			/* Number of words left */
	int bit;
	uint_t *wptr, mask;
	sam_daddr_t bn;
	int nblocks;
	int block_count;
	int type, dt;
	int modblocks;
	offset_t ndau;
	struct sam_sblk *sblk;

	sblk = mp->mi.m_sbp;

	modblocks = *modified;
	type = *bt;
	block = mp->mi.m_fs[ord].block[type];
	dt = block->dt;
	nblocks = (type == SM) ? LG_BLOCK(mp, dt) : 1;
	wptr = (uint_t *)(void *)(bp->b_un.b_addr + off);
	count = (SAM_DEV_BSIZE - off) >> NBPWSHIFT;
	for (; count > 0; wptr++, count--) {
		if (!*wptr)  continue;
		for (bit = 32, mask = 0x80000000; bit > 0; bit--, mask >>= 1) {
			block_count = BLOCK_COUNT(block->fill, block->out,
			    block->limit);
			if ((block_count + nblocks) >= block->limit) {
				if (type != SM) {		/* If done */
					/* If no blocks returned */
					if (modblocks == 0) {
						brelse(bp);
						return (1);
					}
					sam_write_map(mp, block, bp, ord,
					    modblocks, *next_dau);
					return (1);
				}
				*bt = ++type;
				nblocks = 1;
				block = mp->mi.m_fs[ord].block[type];
				dt = block->dt;
				block_count = BLOCK_COUNT(block->fill,
				    block->out, block->limit);
				if ((block_count + nblocks) >= block->limit) {
					sam_write_map(mp, block, bp, ord,
					    modblocks, *next_dau);
					return (1);
				}
			}
			if (*wptr & mask) {
				*wptr &= ~mask;
				modblocks++;
				*modified = modblocks;
				*next_dau = (rblk << SAM_BIT_SHIFT) -
				    (count << NBWDSHIFT) +
				    (32 - bit);
				ndau = (uint_t)*next_dau;
				if (DIF_LG_SHIFT(mp, dt)) {
					ndau = ndau << DIF_LG_SHIFT(mp, dt);
				} else {
					ndau = (ndau * LG_BLK(mp, dt)) >>
					    SAM_DEV_BSHIFT;
				}
				bn = (sam_daddr_t)ndau;
				if (ndau > sblk->eq[ord].fs.capacity) {
					cmn_err(CE_WARN, "SAM-QFS: %s: "
					    "sam_set_block,"
					    " bn=0x%llx, ord=%d, "
					    "capacity=0x%llxKB",
					    mp->mt.fi_name, ndau, ord,
					    sblk->eq[ord].fs.capacity);
					sam_req_fsck(mp, ord,
					    "sam_set_block: bad block");
					continue;
				}
				sam_fill_block_pool(mp, block, nblocks, bn);
			}
		}
	}
	if (modblocks) {	/* Depleted block, return to get next one */
		if (SAM_BWRITE(mp, bp)) {
			return (EIO);
		}
		sam_set_sblk(mp, sblk, ord, modblocks, *next_dau);

		/*
		 * Must update circular buffers for both SM and LG blocks
		 * since we may have switched on full SM buffer above.
		 */
		/* Commit blocks */
		for (type = 0; type < SAM_MAX_DAU; type++) {
			if ((block = mp->mi.m_fs[ord].block[type]) == NULL) {
				continue;
			}
			block->in = block->fill;
		}
		*modified = 0;				/* Clear modified */
	} else {
		brelse(bp);
	}
	return (0);
}


/*
 * ----- sam_write_map - update bit maps.
 *
 *  Issue async I/O to clear allocated blocks.
 */

static void
sam_write_map(
	sam_mount_t *mp,		/* Mount table pointer */
	struct sam_block *block,	/* Pointer to the block table. */
	buf_t *bp,			/* The buffer pointer */
	int ord,			/* Device ordinal */
	int modified,			/* Number of modified bits */
	sam_bn_t next_dau)		/* Next dau to be used */
{
	mutex_enter(&mp->mi.m_block.mutex);
	mp->mi.m_fs[ord].busy = 1;
	mp->mi.m_fs[ord].modified = modified;
	mp->mi.m_fs[ord].next_dau = next_dau;
	mutex_exit(&mp->mi.m_block.mutex);

	bp->b_vp = (vnode_t *)(void *)block;
	bp->b_flags &= ~(B_READ|B_DONE);
	bp->b_flags |= B_ASYNC|B_WRITE;
	bp->b_iodone = (int (*) ())sam_block_done;
	bp->b_file = NULL;
	bp->b_offset = -1;
	if ((bp->b_flags & B_READ) == 0) {
		LQFS_MSG(CE_WARN, "sam_write_map(): bdev_strategy writing "
		    "mof 0x%x edev %ld nb %d\n", bp->b_blkno * 512,
		    bp->b_edev, bp->b_bcount);
	} else {
		LQFS_MSG(CE_WARN, "sam_write_map(): bdev_strategy reading "
		    "mof 0x%x edev %ld nb %d\n", bp->b_blkno * 512,
		    bp->b_edev, bp->b_bcount);
	}
	bdev_strategy(bp);
}


/*
 * ----- sam_set_sblk - update superblock fields
 *
 *  Update superblock fields after I/O done.
 */

static void
sam_set_sblk(
	sam_mount_t *mp,	/* Superblock table pointer */
	struct sam_sblk *sblk,	/* Pointer to the superblock */
	int ord,		/* Device ordinal */
	int modified,		/* Number of modified bits */
	sam_bn_t next_dau)	/* Next dau to be used */
{
	offset_t dau_blks, meta_blks;
	fs_wmstate_e wmstate;
	int out_of_blocks = 0;

	mutex_enter(&mp->mi.m_sblk_mutex);
	sblk->eq[ord].fs.dau_next = next_dau;
	if (sblk->eq[ord].fs.num_group > 1) {
		modified *= sblk->eq[ord].fs.num_group;
	}
	if (sblk->eq[ord].fs.type == DT_META) {
		meta_blks = sblk->info.sb.mm_blks[LG];
		if ((sblk->eq[ord].fs.space -= (meta_blks * modified)) < 0) {
			sblk->eq[ord].fs.space = 0;
		}
		if ((sblk->info.sb.mm_space -= (meta_blks * modified)) < 0) {
			sblk->info.sb.mm_space = 0;
		}
	} else {
		dau_blks = sblk->info.sb.dau_blks[LG];
		if ((sblk->eq[ord].fs.space -= (dau_blks * modified)) < 0) {
			sblk->eq[ord].fs.space = 0;
		}
		if ((sblk->info.sb.space -= (dau_blks * modified)) < 0) {
			sblk->info.sb.space = 0;
			out_of_blocks++;
		}
	}

	/* Determine watermark state */
	wmstate = FS_LHWM;			/* assume in the middle */
	if (sblk->info.sb.space < mp->mi.m_high_blk_count || out_of_blocks)
		wmstate = FS_HWM;
	else if (sblk->info.sb.space > mp->mi.m_low_blk_count)
		wmstate = FS_LWM;
	if (mp->mi.m_xmsg_state != wmstate) {
		sam_check_wmstate(mp, wmstate);
	}
	mutex_exit(&mp->mi.m_sblk_mutex);

	/*
	 * Call releaser and notify arfind if over the high threshold.
	 */
	if (sblk->info.sb.space < mp->mi.m_high_blk_count) {
		sam_start_releaser(mp);
		sam_arfind_hwm(mp);
	}
}


/*
 * ----- sam_block_done - process I/O completion.
 */

void
sam_block_done(buf_t *bp)
{
	struct sam_block *block;
	sam_mount_t *mp;
	int ord;

	block = (struct sam_block *)bp->b_vp;
	bp->b_vp = NULL;
	ord = block->ord;
	mp = block->mp;
	bp->b_iodone = NULL;
	mutex_enter(&mp->mi.m_block.mutex);
	mp->mi.m_fs[ord].busy = 0;
	/* If error, do not include these blocks for allocating. */
	if ((bp->b_flags & B_ERROR)) {
		mp->mi.m_fs[ord].modified = 0;
	}
	SAM_KICK_BLOCK(mp);
	mutex_exit(&mp->mi.m_block.mutex);
	iodone(bp);
}


/*
 * ----- sam_start_releaser - start up the releaser
 *
 * Start up the releaser because the high threshold has been met.
 */

void
sam_start_releaser(sam_mount_t *mp)
{
	struct sam_fsd_cmd cmd;
	int error;

	mutex_enter(&mp->ms.m_waitwr_mutex);
	if (SAM_IS_SHARED_CLIENT(mp) || SAM_IS_STANDALONE(mp) ||
	    (mp->mi.m_release_time &&
	    (mp->mi.m_release_time > SAM_SECOND())) ||
	    mp->mt.fi_status & FS_RELEASING ||
	    !(mp->mt.fi_status & FS_MOUNTED) ||
	    (mp->mt.fi_status & FS_UMOUNT_IN_PROGRESS)) {
		mutex_exit(&mp->ms.m_waitwr_mutex);
		return;
	}

	/*
	 * Show releaser running.
	 * Send command to sam-fsd to start the releaser.
	 */
	cmd.cmd = FSD_releaser;
	bcopy(mp->mt.fi_name, cmd.args.releaser.fs_name,
	    sizeof (cmd.args.releaser.fs_name));
	if ((error = sam_send_scd_cmd(SCD_fsd, &cmd, sizeof (cmd))) == 0) {
		mp->mt.fi_status |= FS_RELEASING;
	}

	TRACE(T_SAM_FIFO_REL, mp->mi.m_vfsp, mp->mt.fi_high,
	    mp->mi.m_high_blk_count, error);
	mutex_exit(&mp->ms.m_waitwr_mutex);
}


/*
 * ----- sam_reset_fs_space - check & reset space on this filesystem.
 *
 * Verify total equipment space in superblock matches filesystem space.
 */

static void
sam_reset_fs_space(sam_mount_t *mp)
{
	struct sam_sblk *sblk;
	offset_t space, mm_space;
	int i;

	mutex_enter(&mp->mi.m_sblk_mutex);
	sblk = mp->mi.m_sbp;
	for (space = 0, mm_space = 0, i = 0; i < sblk->info.sb.fs_count; i++) {
		if (sblk->eq[i].fs.type == DT_META) {
			mm_space += sblk->eq[i].fs.space;
		} else {
			space += sblk->eq[i].fs.space;
		}
	}
	if (sblk->info.sb.space != space) {
		cmn_err(CE_NOTE,
		    "SAM-QFS: %s: reset space from 0x%llx KB to 0x%llx KB",
		    mp->mt.fi_name, sblk->info.sb.space, space);
		sblk->info.sb.space = space;
	}
	if (sblk->info.sb.mm_count) {
		if (sblk->info.sb.mm_space != mm_space) {
			cmn_err(CE_NOTE,
			    "SAM-QFS: %s: reset meta space from 0x%llx KB "
			    "to 0x%llx KB",
			    mp->mt.fi_name, sblk->info.sb.mm_space, mm_space);
			sblk->info.sb.mm_space = mm_space;
		}
	}
	mutex_exit(&mp->mi.m_sblk_mutex);
}


/*
 * ----- sam_reset_eq_space - check & reset space on the equipment.
 *
 * Count free blocks on the specified ord.
 */

static void
sam_reset_eq_space(
	sam_mount_t *mp,	/* Pointer to the mount table. */
	int ord)		/* Current ordinal */
{
	struct sam_sblk *sblk;
	sam_bn_t blk;	/* Current bit map block for this unit */
	int nblks;		/* Number of blocks left before wrap or done */
	buf_t *bp;
	offset_t free;
	offset_t dau_blks;


	mutex_enter(&mp->mi.m_sblk_mutex);
	sblk = mp->mi.m_sbp;
	if (sblk->eq[ord].fs.type == DT_META) {
		dau_blks = sblk->info.sb.mm_blks[LG];
	} else {
		dau_blks = sblk->info.sb.dau_blks[LG];
	}
	if (sblk->eq[ord].fs.num_group > 1) {
		dau_blks *= sblk->eq[ord].fs.num_group;
	}
	blk = sblk->eq[ord].fs.allocmap;
	nblks = sblk->eq[ord].fs.l_allocmap;

	/* Count free bits in maps */
	free = 0;
	for (;;) {
		if (SAM_BREAD(mp, mp->mi.m_fs[sblk->eq[ord].fs.mm_ord].dev,
		    blk, SAM_DEV_BSIZE, &bp)) {
			break;
		}
		free += sam_count_block(bp);
		brelse(bp);
		blk++;
		if (--nblks > 0) {
			continue;	/* If more blocks before wrap */
		}
		break;
	}

	if (sblk->eq[ord].fs.space != (free * dau_blks)) {
		dcmn_err((CE_NOTE,
		    "SAM-QFS: %s: reset ord=%d"
		    " partition space from 0x%llx KB to 0x%llx KB"
		    " for capacity 0x%llx KB",
		    mp->mt.fi_name, ord, sblk->eq[ord].fs.space,
		    free * dau_blks, sblk->eq[ord].fs.capacity));
		ASSERT((free * dau_blks) <= sblk->eq[ord].fs.capacity);
		sblk->eq[ord].fs.space = free * dau_blks;
	}

	mutex_exit(&mp->mi.m_sblk_mutex);
}


/*
 * ----- sam_count_block - count free blocks.
 *
 * Count free blocks in this buffer.
 */

static offset_t			/* 1 = done, 0 = keep going */
sam_count_block(buf_t *bp)
{
	int count;		/* Number of words left */
	int bit;
	uint_t *wptr, mask;
	offset_t free;

	free = 0;
	wptr = (uint_t *)(void *)bp->b_un.b_addr;
	count = SAM_DEV_BSIZE >> NBPWSHIFT;
	for (; count > 0; wptr++, count--) {
		if (!*wptr)  continue;
		for (bit = 32, mask = 0x80000000; bit > 0; bit--, mask >>= 1) {
			if (*wptr & mask) {
				free = free + 1;
			}
		}
	}
	return (free);
}


/*
 * ----- sam_check_wmstate - check file system watermark state.
 *
 * Called when the "new" state appears to be different
 * than the previous state. This routine throttles message
 * changes to sam-amld to at most every 15 seconds.
 */
void
sam_check_wmstate(
	sam_mount_t *mp,	/* The mount table pointer. */
	fs_wmstate_e state)	/* Current state */
{
	fs_wmstate_e old_state, new_state;
	int send_state = 0;

	/*
	 * If standalone, there's no sam-amld to talk to.
	 */
	if (SAM_IS_STANDALONE(mp)) {
		return;
	}

	mutex_enter(&mp->ms.m_xmsg_lock);
	/* Send a message at most every FS_WMSTATE_MIN_SECS seconds */
	if ((mp->mi.m_xmsg_time + FS_WMSTATE_MIN_SECS) < SAM_SECOND()) {
		old_state = mp->mi.m_xmsg_state;

		switch (state) {

		/*
		 * If now above hi or below low, send
		 * unless no change
		 */
		case FS_LWM:
		case FS_HWM:
			if (old_state != state) {
				new_state = state;
				send_state++;
			}
			break;

		/*
		 * In the middle, we need to determine
		 * whether hi or low before this.
		 */
		case FS_LHWM:
		case FS_HLWM:
			if (old_state == FS_HWM) {
				new_state = FS_HLWM;
				send_state++;
			} else if (old_state == FS_LWM) {
				new_state = FS_LHWM;
				send_state++;
			}
			break;
		}
		if (send_state)
			sam_report_fs_watermark(mp, (int)new_state);
	}
	mutex_exit(&mp->ms.m_xmsg_lock);
}

/*
 * ----- sam_report_fs_watermark - send fs_watermark_state.
 *
 * Unconditionally report the watermark state to sam-fsd
 * via the sam-fsd command interface. Since xmsg_state and xmsg_time
 * fields are manipulated in the mount table, release_lock
 * should be set on entry and exit.
 */

void
sam_report_fs_watermark(
	sam_mount_t *mp,	/* The mount table pointer. */
	int state)
{
	struct sam_fsd_cmd	cmd;
	int			old_state;

	old_state = mp->mi.m_xmsg_state;
	mp->mi.m_xmsg_state = state;
	mp->mi.m_xmsg_time = SAM_SECOND();

	/*
	 * build a watermark message
	 * Note, this code, added by CR 6869388,
	 * removes a call to prv_fswm_priority which was
	 * previously done in fifo_fs.c. See CR 6876577.
	 */
	cmd.cmd = FSD_syslog;
	cmd.args.syslog.slerror = E_FSWM;
	cmd.args.syslog.eq = mp->mt.fi_eq;
	cmd.args.syslog.param = state;
	bcopy(mp->mt.fi_mnt_point, cmd.args.syslog.mnt_point,
	    sizeof (cmd.args.syslog.mnt_point));
	(void) sam_send_scd_cmd(SCD_fsd, &cmd, sizeof (cmd));

	TRACE(T_SAM_FIFO_FSWM,
	    mp->mi.m_vfsp, (sam_tr_t)mp->mt.fi_eq, state, old_state);

	/*
	 * Notify arfind if going up above high water mark.
	 */
	if ((state == FS_HWM) && (mp->mt.fi_config & MT_HWM_ARCHIVE)) {
		sam_arfind_hwm(mp);
	}
}


/*
 * ----- sam_init_block - initialize the block thread.
 *
 * Check to see if this mount is capable of allocating blocks.
 * If so, start block thread.
 */

int					/* ERRNO if error, 0 if successful. */
sam_init_block(sam_mount_t *mp)
{
	int ord;
	struct samdent *dp;
	struct sam_sblk *sblk;	/* Pointer to the superblock */
	int error = 0;

	mutex_enter(&mp->mi.m_block.mutex);
	if (mp->mt.fi_mflag & MS_RDONLY || mp->mt.fi_state == DEV_NOALLOC ||
	    mp->mt.fi_state == DEV_UNAVAIL) {
		goto out;
	}
	if (SAM_IS_SHARED_CLIENT(mp) ||
	    (mp->mi.m_block.state == SAM_THR_EXIST)) {
		goto out;
	}

	/*
	 * Reset space in superblock for equipments and filesystem.
	 */
	sblk = mp->mi.m_sbp;
	for (ord = 0; ord < sblk->info.sb.fs_count; ord++) {
		dp = &mp->mi.m_fs[ord];
		if (dp->skip_ord || is_osd_group(dp->part.pt_type)) {
			continue;
		}
		sam_reset_eq_space(mp, ord);
	}
	sam_reset_fs_space(mp);

	for (ord = 0; ord < sblk->info.sb.fs_count; ord++) {
		dp = &mp->mi.m_fs[ord];
		if (dp->skip_ord || dp->part.pt_state == DEV_NOALLOC ||
		    dp->part.pt_state == DEV_UNAVAIL ||
		    is_osd_group(dp->part.pt_type)) {
			continue;
		}
		sam_init_blocklist(mp, ord);
	}
	/*
	 * Initialize the block thread for this filesystem mount.
	 */
	error = sam_init_thread(&mp->mi.m_block, sam_block_thread, mp);

out:
	mutex_exit(&mp->mi.m_block.mutex);
	return (error);
}


/*
 * ----- sam_kill_block - kill the block thread.
 *
 * Free block pools.
 */

void
sam_kill_block(sam_mount_t *mp)
{
	int bt, ord;

	sam_kill_thread(&mp->mi.m_block);
	for (bt = 0; bt < SAM_MAX_DAU; bt++) {
		for (ord = 0; ord < mp->mt.fs_count; ord++) {
			if (mp->mi.m_fs[ord].block[bt] != NULL) {
				sam_delete_blocklist(
				    &mp->mi.m_fs[ord].block[bt]);
			}
		}
	}
}

/*
 * ----- sam_delete_blocklist - delete specified block list
 *
 * Free memory space used for a block list, and dereference
 * the block list pointer.
 */

static void
sam_delete_blocklist(struct sam_block **blockp)
{
	if (*blockp != NULL) {
		kmem_free((void *) *blockp, sizeof (struct sam_block) +
		    ((*blockp)->limit - 1) * sizeof (sam_bn_t));
	}
	*blockp = NULL;
}

/*
 * ----- sam_init_blocklist - initialize the block pools
 *
 * Allocate and initialize the block pools for a specified
 * partition. A small and large block pool is initialized
 * for mm and md devices. The size of each block pool is
 * adjusted for number of partitions in the file system.
 */

static void
sam_init_blocklist(
	sam_mount_t *mp,	/* The pointer to the mount table */
	uchar_t ord)		/* Partition ordinal */
{
	int bt, limit;
	struct sam_block *block;
	struct samdent *dp;

	limit = L_SAM_BLK;
	if (mp->mt.fs_count > 2) limit = limit >> 1;
	if (mp->mt.fs_count > 4) limit = limit >> 1;
	bt = LG;
	dp = &mp->mi.m_fs[ord];
	if (dp->part.pt_type == DT_DATA) {
		bt = SM;
	}
	if (dp->part.pt_type == DT_META) {
		if (SM_BLK(mp, MM) != LG_BLK(mp, MM)) {
			bt = SM;
		}
	}
	for (;;) {
		if (mp->mi.m_fs[ord].block[bt] == NULL) {
			/*
			 * Allocate and set up circular queues for the block
			 * pools.
			 */
			block = (struct sam_block *)kmem_zalloc(
			    sizeof (struct sam_block) +
			    ((limit-1) * sizeof (sam_bn_t)),
			    KM_SLEEP);
			mp->mi.m_fs[ord].block[bt] = block;
			block->mp = mp;
			block->ord = (short)ord;
			block->dt = (dp->part.pt_type == DT_META) ? MM : DD;
			block->limit = limit;
			sam_mutex_init(&block->md_mutex, NULL,
			    MUTEX_DEFAULT, NULL);
		}
		if (bt == SM) {
			mp->mt.fi_config1 |= MC_SMALL_DAUS;
			bt = LG;
			continue;
		}
		break;
	}

	/*
	 * Wake threads up waiting because of ENOSPC
	 */
	if (mp->mi.m_wait_write) {
		mutex_enter(&mp->ms.m_waitwr_mutex);
		if (mp->mi.m_wait_write) {
			cv_broadcast(&mp->ms.m_waitwr_cv);
		}
		mutex_exit(&mp->ms.m_waitwr_mutex);
	}
}

/*
 * ----- sam_change_state - effect the transition due to a change in state
 *
 * sam_change_state is called by the block thread to make the changes
 * effective for the commands;
 * 1. Add     - Add eq to mounted file system
 * 2. Remove  - Remove eq1 from mounted file system; if eq2, move eq1 to eq2
 * 3. Release - Release files on eq and mark offline
 * 4. Noalloc - Release all blocks on any block pool(s) assigned to this group,
 *    and delete the block pool. Do not allow any more allocates.
 * 5. Alloc   - Initialize the block pool(s) and allow allocation
 */
static void
sam_change_state(
	sam_mount_t *mp,	/* Pointer to the mount table. */
	struct samdent *dp,	/* Pointer to device entry in mount table */
	int ord)		/* Current ordinal */
{
	struct sam_sblk *sblk;	/* Pointer to the superblock */
	boolean_t sblk_modified = FALSE;
	int rtnerr = 0;

	/*
	 * If busy, cannot change state
	 */
	dp = &mp->mi.m_fs[ord];
	TRACE(T_SAM_CHG_STATE, mp, ord, dp->part.pt_state, dp->num_group);
	if (dp->busy || dp->modified)  {
		TRACE(T_SAM_CHG_STATE_ERR, mp, ord, dp->part.pt_state, 1);
		return;		/* Ordinal is busy with I/O */
	}
	sblk = mp->mi.m_sbp;

	switch (dp->command) {

	case DK_CMD_remove:
	case DK_CMD_release:
	case DK_CMD_noalloc:
		sam_drain_free_list(mp, dp);
		dp->skip_ord = 1;
		dp->part.pt_state = DEV_NOALLOC;
		sblk_modified = TRUE;
		break;

	case DK_CMD_add:
		/*
		 * Add is only supported for sblk version 2A.
		 */
		if ((rtnerr = sam_grow_fs(mp, dp, ord))) {
			break;
		}

		/*
		 * Standalone, fallthrough to alloc command. Shared, wait
		 * until the clients have updated their mcf files. Admin
		 * must type alloc command.
		 */
		if (SAM_IS_SHARED_FS(mp)) {
			sblk_modified = TRUE;
			break;
		}

		/* LINTED [fallthrough on case statement] */
	case DK_CMD_alloc: {
		int i;

		if (SAM_IS_SHARED_FS(mp)) {
			client_entry_t *clp;

			for (i = 1; i <= mp->ms.m_max_clients; i++) {
				clp = sam_get_client_entry(mp, i, 0);
				if (clp == NULL ||
				    clp->cl_sh.sh_fp == NULL ||
				    !(clp->cl_status & FS_MOUNTED) ||
				    (clp->cl_status & FS_SERVER) ||
				    (clp->cl_flags & SAM_CLIENT_NOT_RESP)) {
					continue;
				}

				if (!sam_client_has_tag(clp,
				    QFS_TAG_VFSSTAT_V2)) {
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: Client (%d) %s "
					    "package must be updated to "
					    "support online grow",
					    mp->mt.fi_name, ord, clp->hname);
					rtnerr = 6;
					break;
				}

				if ((clp->cl_fsgen != mp->mi.m_sblk_fsgen) ||
				    (clp->fs_count != sblk->info.sb.fs_count) ||
				    (clp->mm_count != sblk->info.sb.mm_count)) {
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: Client (%d) %s has "
					    "not updated mcf (fs_cnt %d/%d "
					    "mm_cnt %d/%d fsgen %x/%x)",
					    mp->mt.fi_name, ord, clp->hname,
					    sblk->info.sb.fs_count,
					    clp->fs_count,
					    sblk->info.sb.mm_count,
					    clp->mm_count, mp->mi.m_sblk_fsgen,
					    clp->cl_fsgen);
					rtnerr = 7;
					break;
				}
			}
			if (rtnerr) {
				break;
			}
		}

		/*
		 * Build the allocation link for this new LUN.
		 * Initialize the block pool for this new LUN;
		 */
		if (dp->alloc_link == 0) {
			if ((sam_build_allocation_links(mp, sblk, ord))) {
				rtnerr = 8;
				break;
			}
		}
		mutex_enter(&mp->mi.m_block.mutex);
		if (dp->block[LG] == NULL) {
			sam_init_blocklist(mp, ord);
		}
		dp->skip_ord = 0;
		dp->part.pt_state = DEV_ON;
		mutex_exit(&mp->mi.m_block.mutex);
		if (SAM_IS_SHARED_FS(mp)) {
			sam_wake_sharedaemon(mp, EINTR);
		}
		sblk_modified = TRUE;
		}
		break;

	case DK_CMD_off: {
		offset_t space;
		struct sam_sbord *sop;

		sblk = mp->mi.m_sbp;
		sop = &sblk->eq[ord].fs;
		sam_wait_free_blocks(mp);

		space = sop->space + (sop->system * sop->num_group);
		if (space != sop->capacity) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: cannot OFF ord=%d space 0x%llx KB"
			    " is not equal to capacity 0x%llx KB",
			    mp->mt.fi_name, ord, space, sop->capacity);
			rtnerr = 9;
		} else {
			/*
			 * Release space for bit map if after system.  This
			 * occurs when a data device is added with online grow.
			 */
			if (sop->allocmap >= sblk->eq[sop->mm_ord].fs.system) {
				sam_rel_blks_t *rlp;

				rlp = (sam_rel_blks_t *)kmem_zalloc(
				    sizeof (sam_rel_blks_t), KM_SLEEP);
				rlp->status.b.direct_map = 1;
				if (sblk->eq[sop->mm_ord].fs.type == DT_META) {
					rlp->status.b.meta = 1;
				}
				rlp->e.extent[0] = (sop->allocmap >>
				    mp->mi.m_bn_shift);
				rlp->e.extent_ord[0] = sop->mm_ord;
				rlp->e.extent[1] = sop->l_allocmap;
				mutex_enter(&mp->mi.m_inode.mutex);
				rlp->next = mp->mi.m_next;
				mp->mi.m_next = rlp;
				mutex_exit(&mp->mi.m_inode.mutex);
				mutex_enter(&mp->mi.m_inode.put_mutex);
				if (mp->mi.m_inode.put_wait) {
					cv_signal(&mp->mi.m_inode.put_cv);
				}
				mutex_exit(&mp->mi.m_inode.put_mutex);
				sam_wait_free_blocks(mp);
			}
			sam_remove_from_allocation_links(mp, ord);
			dp->part.pt_state = DEV_OFF;
			mutex_enter(&mp->mi.m_sblk_mutex);
			sop->state = DEV_OFF;
			sblk->info.sb.capacity -= sop->capacity;
			sop->capacity = 0;
			sblk->info.sb.space -= sop->space;
			sop->space = 0;
			sop->system = 0;
			sop->dau_next = 0;
			sop->allocmap = 0;
			sop->l_allocmap = 0;
			mutex_exit(&mp->mi.m_sblk_mutex);
			sblk_modified = TRUE;
		}
		mutex_enter(&mp->ms.m_waitwr_mutex);
		mp->mt.fi_status &= ~FS_SHRINKING;
		mutex_exit(&mp->ms.m_waitwr_mutex);
		break;
		}

	default:
		rtnerr = 10;
		break;
	}

	/*
	 * If the superblock changed, set the ordinal state for all
	 * members in the group. Then update all the superblocks.
	 */
	sblk = mp->mi.m_sbp;
	if (sblk_modified && (ord < sblk->info.sb.fs_count)) {
		mutex_enter(&mp->mi.m_sblk_mutex);
		sam_set_lun_state(mp, ord, dp->part.pt_state);
		if ((sam_update_the_sblks(mp)) != 0) { 	/* If error */
			cmn_err(CE_WARN, "SAM-QFS: %s: Error changing state on"
			    " eq %d lun %s, could not write sblk",
			    mp->mt.fi_name, dp->part.pt_eq, dp->part.pt_name);
		}
		mutex_exit(&mp->mi.m_sblk_mutex);
		TRACE(T_SAM_CHG_STATE_OK, mp, ord, dp->part.pt_state,
		    dp->num_group);
	} else {
		if (rtnerr) {
			TRACE(T_SAM_CHG_STATE_ERR, mp, ord, dp->part.pt_state,
			    rtnerr);
		} else {
			TRACE(T_SAM_NO_CHG_STATE, mp, ord, dp->part.pt_state,
			    dp->num_group);
		}
	}
	dp->command = DK_CMD_null;
}


/*
 * ----- sam_wait_free_blocks - Wait until all blocks are in free map.
 * Wait until free block list has been put in the free block
 * pool (sam_fb_pool_t). Call sam_update_filsys to make sure
 * the free block list has been merged into the free block maps.
 */
static void
sam_wait_free_blocks(
	struct sam_mount *mp)
{
	sam_wait_release_blk_list(mp);
	mutex_enter(&mp->ms.m_synclock);
	sam_update_filsys(mp, 0);
	mutex_exit(&mp->ms.m_synclock);
}


/*
 * ----- sam_set_lun_state - Set state for lun or striped group
 */
static void
sam_set_lun_state(
	struct sam_mount *mp,
	int ord,
	uchar_t state)
{
	struct samdent *dp;
	struct sam_sbord *sop;
	int num_group;
	int i;

	dp = &mp->mi.m_fs[ord];
	sop = &mp->mi.m_sbp->eq[ord].fs;
	num_group = dp->num_group;
	for (i = 0; i < num_group; i++, dp++, sop++) {
		sop->state = state;
		dp->part.pt_state = state;
	}
}


/*
 * ----- sam_growfs_next_meta - get next available meta device
 */
static int
sam_growfs_next_meta(
	int cur_ord,		/* current ordinal */
	struct sam_sblk *sblk) 	/* pointer to superblock	*/
{
	int			next_ord = cur_ord;
	int			last_ord = sblk->info.sb.fs_count - 1;
	struct sam_sbord	*mop;

	next_ord = next_ord + 1 > last_ord ? 0 : next_ord + 1;

	mop = &sblk->eq[next_ord].fs;

	/*
	 * Loop will terminate if cur_ord is the only valid device and
	 * return cur_ord as the device
	 */
	while ((mop->type != DT_META) || (mop->state != DEV_ON)) {
		/*
		 * If looped back to starting ordinal, bail
		 */
		if (next_ord == sblk->info.sb.mm_ord) {
			break;
		}
		next_ord = next_ord + 1 > last_ord ? 0 : next_ord + 1;
		mop = &sblk->eq[next_ord].fs;
	}

	return (next_ord);
}

/*
 * ----- sam_grow_fs - Add this eq to the file system
 * Build the bit maps and add this partition to the file system
 */
static int
sam_grow_fs(
	sam_mount_t *mp,	/* Pointer to the mount table. */
	struct samdent *dp,	/* Pointer to device entry in mount table */
	int ord)		/* Current ordinal */
{
	struct sam_sblk *sblk;		/* Pointer to the superblock */
	struct sam_sbord *sop, *dsop;	/* Sblk entry for this ordinal */
	sam_sblk_t *old_sblk;		/* Pointer to copy of original sblk */
	int old_sblk_size;		/* Size of original sblk */
	int new_sblk_size;		/* Size of new sblk */
	char *dcp;
	offset_t blocks;
	sblk_args_t args;
	int mm_ord;
	struct samdent *meta_dp;	/* Meta entry in mount table */
	int dt;
	sam_prealloc_t pa;
	int len;
	struct samdent *ddp;
	dtype_t		type;		/* Device type */
	int i;
	int error;

	sblk = mp->mi.m_sbp;

	/*
	 * Currently cannot add object device ("oXXX").
	 */
	type = dp->part.pt_type;
	if (is_osd_group(type)) {
		cmn_err(CE_WARN, "SAM-QFS: %s: Cannot add object eq %d "
		    "lun %s",
		    mp->mt.fi_name, dp->part.pt_eq, dp->part.pt_name);
		return (22);
	}

#if defined(SOL_511_ABOVE) && !defined(_NoOSD_)
	/*
	 * Check for validity of object pool group.  All members of an
	 * object pool must have the same DAU.
	 */
	if (sam_check_osd_daus(mp)) {
		return (23);
	}
#endif

	/*
	 * Check for validity of stripe group. Can only grow first
	 * member of the stripe group.
	 */
	if (is_stripe_group(type)) {
		for (i = 0; i < ord; i++) {
			if (type != mp->mi.m_fs[i].part.pt_type) {
				continue;
			}
			cmn_err(CE_WARN, "SAM-QFS: %s: Error adding eq %d "
			    "lun %s,"
			    " eq is not first member of the stripe group",
			    mp->mt.fi_name, dp->part.pt_eq, dp->part.pt_name);
			return (24);
		}
	}

	/*
	 * Set num_group. It has been cleared on mount.
	 */
	dp->num_group = 1;
	ddp = &mp->mi.m_fs[ord+1];
	for (i = ord+1; i < mp->mt.fs_count; i++, ddp++) {
		if (!is_stripe_group(ddp->part.pt_type)) {
			break;
		}
		if (type != ddp->part.pt_type) {
			break;
		}
		dp->num_group++;
	}

	/*
	 * Cannot add meta devices to a ms file system.
	 */
	dt = (type == DT_META) ? MM : DD;
	if ((mp->mt.mm_count == 0) && (dt == MM)) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: Cannot add meta eq %d lun %s to ms "
		    "file system",
		    mp->mt.fi_name, dp->part.pt_eq, dp->part.pt_name);
		return (25);
	}

	/*
	 * Verify pt_size is set.
	 */
	if (dp->part.pt_size == 0) {
		cmn_err(CE_WARN, "SAM-QFS: %s: Error adding "
		    "eq %d lun %s no size configured, do 'samd config'",
		    mp->mt.fi_name, dp->part.pt_eq, dp->part.pt_name);
		return (21);
	}

	/*
	 * If ma file system and data device, put the map on next meta device.
	 * If ma file system and meta device, put maps on this device.
	 * If ms file system, put maps on this device.
	 * Compute blocks = # of logical blocks (in units of 1024 bytes)
	 */
	blocks = dp->part.pt_size >> SAM2SUN_BSHIFT;
	if (mp->mt.mm_count && (type != DT_META)) {
		mm_ord = sblk->info.sb.mm_ord;	/* Next meta device */
		do {
			/*
			 * Preallocate a sequential number of blocks for the
			 * maps on mm_ord.
			 */
			pa.length = blocks / LG_DEV_BLOCK(mp, dt);
			pa.count = (howmany(pa.length, (SAM_DEV_BSIZE * NBBY)) +
			    LG_DEV_BLOCK(mp, MM) - 1) /
			    LG_DEV_BLOCK(mp, MM);
			pa.dt = MM;
			pa.error = 0;

			/*
			 * If successful, mm_ord is the current mm device
			 */
			pa.ord = (ushort_t)mm_ord;
			sam_process_prealloc_req(mp, sblk, &pa, TRUE);
			if (!pa.error) {
				break;
			}
			mm_ord = sam_growfs_next_meta(mm_ord, sblk);

		} while (mm_ord != sblk->info.sb.mm_ord);

		if (pa.error) {
			cmn_err(CE_WARN, "SAM-QFS: %s: Error adding "
			    "eq %d lun %s maps on eq %d, error = %d",
			    mp->mt.fi_name, dp->part.pt_eq,
			    dp->part.pt_name,
			    sblk->eq[mm_ord].fs.eq, pa.error);
			return (26);
		}
	} else {
		mm_ord = ord;
		pa.first_bn = SUPERBLK + LG_DEV_BLOCK(mp, dt);
		pa.first_bn = ((pa.first_bn + LG_DEV_BLOCK(mp, dt) - 1) /
		    LG_DEV_BLOCK(mp, dt)) * LG_DEV_BLOCK(mp, dt);
	}

	mutex_enter(&mp->ms.m_waitwr_mutex);
	if (mp->mt.fi_status & FS_RECONFIG) {
		mutex_exit(&mp->ms.m_waitwr_mutex);
		cmn_err(CE_WARN, "SAM-QFS: %s: Error adding eq %d lun %s, "
		    "config in progress", mp->mt.fi_name,
		    dp->part.pt_eq, dp->part.pt_name);
		return (27);
	}
	mp->mt.fi_status |= FS_RECONFIG;
	mutex_exit(&mp->ms.m_waitwr_mutex);

	/*
	 * Get the new sblk. Set the new size, but do not set the
	 * new device count until the free map has been built and cleared.
	 */
	mutex_enter(&mp->mi.m_sblk_mutex);

	/*
	 * Note that mi.m_sblk_size is rounded up to DEV_BSIZE.
	 */
	old_sblk_size = mp->mi.m_sblk_size;
	new_sblk_size = L_SBINFO + (mp->mt.fs_count * L_SBORD);
	new_sblk_size = roundup(new_sblk_size, DEV_BSIZE);

	/*
	 * Make a copy of the original superblock
	 */
	old_sblk = (sam_sblk_t *)kmem_zalloc(sizeof (*old_sblk), KM_SLEEP);
	bcopy((char *)sblk, (char *)old_sblk, sizeof (*old_sblk));
	sblk->info.sb.sblk_size = new_sblk_size;

	/*
	 * set global mm_ord to the mm ordinal used for this new device.
	 * sam_init_sblk_dev will advance the global mm_ord to the next
	 * mm device if necessary.
	 */
	sblk->info.sb.mm_ord = (ushort_t)mm_ord;

	/*
	 * Set state to OFF for the selected ordinal. The new ordinal may be
	 * within the superblock ordinals or after the last superblock ordinal.
	 * Set num_group for possible new striped group.
	 */
	sop = &sblk->eq[ord].fs;
	sop->ord = (ushort_t)ord;
	sop->eq = dp->part.pt_eq;
	sop->state = DEV_OFF;
	sop->type = dp->part.pt_type;
	sop->num_group = dp->num_group;

	/*
	 * Initialize the new device entry in the superblock.
	 */
	sop->dau_next = 0;		/* used to set system; in blocks */
	sop->num_group = dp->num_group;
	sop->mm_ord = (ushort_t)mm_ord;
	if (ord != mm_ord) {
		sblk->eq[mm_ord].fs.dau_next = pa.first_bn;
	}
	args.ord =  ord;
	args.type = type;
	args.blocks = blocks / LG_DEV_BLOCK(mp, dt);
	args.kblocks[DD] = mp->mi.m_dau[DD].kblocks[LG];
	args.kblocks[MM] = mp->mi.m_dau[MM].kblocks[LG];
	sam_init_sblk_dev(sblk, &args);

	sop->system = ((sop->dau_next + LG_DEV_BLOCK(mp, dt) - 1) /
	    LG_DEV_BLOCK(mp, dt)) * LG_DEV_BLOCK(mp, dt);

	/*
	 * reset dau_next to 0. Prealloc will have marked the system area
	 * as already allocated causing dau_next to advance to the first
	 * allocatable dau.
	 */
	sop->dau_next = 0;

	mutex_exit(&mp->mi.m_sblk_mutex);

	/*
	 * Build the free map.
	 */
	mutex_enter(&dp->eq_mutex);
	dcp = (char *)kmem_zalloc(SAM_DEV_BSIZE, KM_SLEEP);
	meta_dp = &mp->mi.m_fs[mm_ord];
	error = sam_bfmap(SAMFS_CALLER, sblk, ord, meta_dp->dev, dcp, NULL, 1);
	kmem_free(dcp, SAM_DEV_BSIZE);
	if (error) {
		mutex_exit(&dp->eq_mutex);
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: I/O error writing map blocks "
		    "for eq %d on %s",
		    mp->mt.fi_name, dp->part.pt_eq,
		    mp->mi.m_fs[mm_ord].part.pt_name);
		goto done;
	}

	/*
	 * Clear allocated blocks in the map.
	 */
	error = sam_cablk(SAMFS_CALLER, sblk, (void *)mp, ord, 1, 1,
	    LG_DEV_BLOCK(mp, DD), LG_DEV_BLOCK(mp, MM), &len);
	if (error) {
		mutex_exit(&dp->eq_mutex);
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: Error %d clearing blocks in bitmap "
		    "system len %x computed len %x eq %d lun %s",
		    mp->mt.fi_name, error, sop->system, len,
		    dp->part.pt_eq, dp->part.pt_name);
		goto done;
	}

	/*
	 * Write label block if adding a data device.
	 */
	if ((type != DT_META) && SAM_IS_SHARED_FS(mp)) {
		if ((error = sam_write_label(mp, dp, ord))) {
			mutex_exit(&dp->eq_mutex);
			cmn_err(CE_WARN, "SAM-QFS: %s: Error writing label on "
			    "eq %d lun %s",
			    mp->mt.fi_name, dp->part.pt_eq, dp->part.pt_name);
			goto done;
		}
	}
	mutex_exit(&dp->eq_mutex);

	/*
	 * Increment the file system generation number.
	 * Set the correct device count now. Update capacity and space count.
	 * Set DEV_UNAVAIL state in the mount table and the superblock.
	 */
	mutex_enter(&mp->mi.m_sblk_mutex);
	sblk->info.sb.fsgen++;
	mp->mi.m_sblk_fsgen = sblk->info.sb.fsgen;
	sblk->info.sb.fs_count = mp->mt.fs_count;
	if (dt == MM) {
		sblk->info.sb.mm_count++;
		sblk->info.sb.mm_capacity += sop->capacity;
		sblk->info.sb.mm_space += sop->space;
	} else {
		sblk->info.sb.da_count++;
		sblk->info.sb.capacity += sop->capacity;
		sblk->info.sb.space += sop->space;
	}
	dp->part.pt_capacity = sop->capacity * 1024;
	dp->part.pt_space = sop->space * 1024;

	for (i = 0, ddp = dp, dsop = sop; i < dp->num_group;
	    i++, ddp++, dsop++) {
		ddp->skip_ord = 1;	/* Skip until block pool initialized */
		dsop->type = sop->type;
		dsop->eq = ddp->part.pt_eq;
		ddp->command = DK_CMD_null;
		dsop->mm_ord = sop->mm_ord;
		dsop->system = sop->system;
		dsop->state = DEV_UNAVAIL;
		ddp->part.pt_state = DEV_UNAVAIL;
	}

	/*
	 * Set superblock for OFF devices.
	 */
	ddp = &mp->mi.m_fs[0];
	for (i = 0; i < mp->mt.fs_count; i++, ddp++) {
		if (ddp->part.pt_state == DEV_OFF) {
			sblk->eq[i].fs.state = DEV_OFF;
			sblk->eq[i].fs.eq = ddp->part.pt_eq;
			sblk->eq[i].fs.type = ddp->part.pt_type;
			sblk->eq[i].fs.num_group = ddp->num_group;
		}
	}

	/*
	 * Write the new sblk on all the ords (LIFO). If error, restore
	 * old sblk. If error writing sblks, currently the new allocation
	 * map is not removed. This means this file system will need to do
	 * a fsck. XXX Fix this.
	 */
	mp->mi.m_sblk_size = new_sblk_size;
	if ((error = sam_update_the_sblks(mp)) != 0) { 	/* If error */
		cmn_err(CE_WARN, "SAM-QFS: %s: Error adding eq %d lun %s,"
		    " could not write sblk",
		    mp->mt.fi_name, dp->part.pt_eq, dp->part.pt_name);
		/*
		 * If error, restore original superblock image and block size
		 */
		bcopy((char *)old_sblk, (char *)sblk, sizeof (*sblk));
		mp->mi.m_sblk_size = old_sblk_size;
		if ((error = sam_update_the_sblks(mp)) != 0) { /* If error */
			error = 0;
			sam_req_fsck(mp, ord,
			    "sam_grow_fs: ALERT: cannot restore old sblk");
		} else {
			error = EIO;
			sam_req_fsck(mp, ord,
			    "sam_grow_fs: restored old sblk; reclaim blocks");
		}
	}
	mutex_exit(&mp->mi.m_sblk_mutex);

done:
	if (error) {
		mutex_enter(&mp->mi.m_sblk_mutex);
		bcopy((char *)old_sblk, (char *)sblk, sizeof (*sblk));
		mp->mi.m_sblk_size = old_sblk_size;
		mutex_exit(&mp->mi.m_sblk_mutex);
		kmem_free(old_sblk, sizeof (*old_sblk));
		mutex_enter(&mp->ms.m_waitwr_mutex);
		mp->mt.fi_status &= ~FS_RECONFIG;
		mutex_exit(&mp->ms.m_waitwr_mutex);
		return (29);
	}
	kmem_free(old_sblk, sizeof (*old_sblk));
	mutex_enter(&mp->ms.m_waitwr_mutex);
	mp->mt.fi_status &= ~FS_RECONFIG;
	mutex_exit(&mp->ms.m_waitwr_mutex);
	return (0);
}


/*
 * ----- sam_write_label - Write the shared label block to the new
 * data device. Loop until the label can be read from the first data
 * device that is ON/NOALLOC/UNAVAIL.
 */
static int
sam_write_label(
	sam_mount_t *mp,	/* Pointer to the mount table. */
	struct samdent *dp,	/* Device entry of the new added ordinal */
	int ord)		/* Ordinal where label is to be written */
{
	int i;
	struct samdent *ddp;
	int dt;
	int error = 0;

	for (i = 0; i < mp->mt.fs_count; i++) {
		if (i == ord) {
			continue;
		}
		ddp = &mp->mi.m_fs[i];
		dt = (ddp->part.pt_type == DT_META) ? MM : DD;
		if (dt != DD) {
			continue;
		}
		if (ddp->part.pt_state != DEV_ON &&
		    ddp->part.pt_state != DEV_NOALLOC &&
		    ddp->part.pt_state != DEV_UNAVAIL) {
			continue;
		}
		if (is_osd_group(ddp->part.pt_type)) {
			char *buf = NULL;

			buf = kmem_alloc(L_LABEL, KM_SLEEP);
			error = sam_issue_object_io(mp, ddp->oh, FREAD,
			    SAM_OBJ_LBLK_ID, UIO_SYSSPACE, buf, 0, L_LABEL);
			if (error == 0) {
				error = sam_issue_object_io(mp, dp->oh, FWRITE,
				    SAM_OBJ_LBLK_ID, UIO_SYSSPACE, buf,
				    0, L_LABEL);
			}
			kmem_free(buf, L_LABEL);
		} else {
			buf_t *bp, *obp;

			error = sam_bread(mp, ddp->dev, LABELBLK, L_LABEL, &bp);
			if (error == 0) {
				obp = getblk(dp->dev,
				    (LABELBLK << SAM2SUN_BSHIFT), L_LABEL);
				bcopy((void *)bp->b_un.b_addr, obp->b_un.b_addr,
				    L_LABEL);
				bp->b_flags |= B_STALE | B_AGE;
				brelse(bp);
				error = sam_bwrite2(mp, obp);
				obp->b_flags |= B_STALE | B_AGE;
				brelse(obp);
			}
		}
		if (error) {
			continue;
		}
		break;

	}
	return (error);
}


/*
 * ---- sam_fs_clear_maps - Clear free disk blocks in map.
 */

int
sam_fs_clear_map(
	void *vmp,		/* Pointer to mount table */
	struct sam_sblk *sblk,	/* Pointer to the superblock */
	int ord,		/* Disk ordinal to clear maps. */
	int len)		/* Number of allocation units */
{
	sam_mount_t *mp = (sam_mount_t *)(void *)vmp;
	sam_prealloc_t pa;

	pa.length = len;
	pa.count = len;
	pa.dt = MM;
	pa.ord = (ushort_t)ord;
	sam_process_prealloc_req(mp, sblk, &pa, FALSE);
	if (pa.error) {
		cmn_err(CE_WARN, "SAM-QFS: %s: Error %d clearing blks on "
		    "eq %d lun %s",
		    mp->mt.fi_name, pa.error, mp->mi.m_fs[ord].part.pt_eq,
		    mp->mi.m_fs[ord].part.pt_name);
	}
	return (pa.error);
}

/*
 * ----- sam_update_the_sblks - update all super blocks.
 * sam_update_the_sblks synchronously updates all the super blocks
 * of a file system.
 *
 */

int				/* > 0 if error, 0 if successful */
sam_update_the_sblks(struct sam_mount *mp)
{
	int ord;
	struct sam_sblk *sblk;

	sblk = mp->mi.m_sbp;
	for (ord = (sblk->info.sb.fs_count - 1); ord >= 0; ord--) {
		if (sblk->eq[ord].fs.state == DEV_DOWN) {
			continue;
		}
		if (sblk->eq[ord].fs.state == DEV_OFF &&
		    !mp->mi.m_fs[ord].opened) {
			continue;
		}
		if (ord == 0) {
			if (!sam_update_sblk(mp, ord, 1, TRUE)) {
				return (1);
			}
		}
		if (!sam_update_sblk(mp, ord, 0, TRUE)) {
			return (1);
		}
	}
	return (0);
}


/*
 * ----- sam_remove_from_allocation_links -
 *
 * Remove the specified ordinal from the allocation links.
 */

static int
sam_remove_from_allocation_links(
	sam_mount_t *mp,
	int ord)
{
	struct samdent *dp;
	int disk_type;
	int dk_max;
	int prev_ord = -1;
	int cur_ord;

	dp = &mp->mi.m_fs[ord];

	if (dp->part.pt_type == DT_META) {
		disk_type = MM;
	} else {
		/*
		 * Includes is_osd_group(dp->part.pt_type).
		 */
		disk_type = DD;
	}

	cur_ord = mp->mi.m_dk_start[disk_type];
	for (dk_max = mp->mi.m_dk_max[disk_type];
	    dk_max > 0; dk_max--) {

		if (cur_ord == ord) {
			if (prev_ord == -1) {
				mp->mi.m_dk_start[disk_type] =
				    mp->mi.m_fs[ord].next_ord;
			} else {
				mp->mi.m_fs[prev_ord].next_ord =
				    mp->mi.m_fs[ord].next_ord;
			}
			mp->mi.m_fs[ord].next_ord = 0;
			mp->mi.m_dk_max[disk_type]--;
			dp->alloc_link = 0;	/* Not in allocation list */
			dp->skip_ord = 1;	/* Don't allocate on this ord */
			break;
		}
		prev_ord = cur_ord;
		cur_ord = mp->mi.m_fs[cur_ord].next_ord;
		if (cur_ord == 0) {
			break;
		}
	}

	return (0);
}
