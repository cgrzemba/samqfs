/*
 * ----- balloc.c - Process the disk block functions.
 *
 * Process the SAM-QFS allocate/free disk block functions.
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

#pragma ident "$Revision: 1.87 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/t_lock.h>
#include <sys/sysmacros.h>
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
#include <sys/ddi.h>
#include <sys/stat.h>

/* ----- SAMFS Includes */

#include "sam/param.h"
#include "sam/types.h"

#include "macros.h"
#include "sblk.h"
#include "inode.h"
#include "mount.h"
#include "quota.h"
#include "debug.h"
#include "extern.h"
#include "indirect.h"
#include "trace.h"
#include "extern.h"
#include "qfs_log.h"


/*
 * ----- sam_alloc_block - allocate block.
 *
 * Get block from small, large, or meta buffer pool (circular buffer).
 * NOTE: Block returned in extent form, that is, not corrected for
 *	ext_bshift
 * If empty signal thread to fill it.
 */

int				/* ERRNO, else 0 if successful. */
sam_alloc_block(
	sam_node_t *ip,		/* Pointer to the inode. */
	int bt,			/* The block type: small (SM), large (LG). */
	sam_bn_t *bn,		/* Pointer to disk block number (returned). */
	int *ord)		/* Pointer to disk ordinal (returned). */
{
	struct sam_block *block;	/* Pointer to the block table. */
	int out, block_count;
	int first_unit, unit;
	sam_mount_t *mp = ip->mp;
	int error = 0;
	int stripe;
	int pass = 0;
	boolean_t skipping_ord;


	/*
	 * Remember the first unit - so we know when we've cycled.
	 */
	if (ip->di.unit >= mp->mt.fs_count) {
		if ((error = sam_set_unit(ip->mp, &ip->di))) {
			return (error);
		}
	}
	first_unit = unit = ip->di.unit;

	for (;;) {
		if (mp->mi.m_fs[unit].part.pt_state == DEV_NOALLOC ||
		    mp->mi.m_fs[unit].part.pt_state == DEV_UNAVAIL ||
		    ((block = mp->mi.m_fs[unit].block[bt]) == NULL)) {
			skipping_ord = TRUE;
			block = NULL;
			goto skip_this_unit;
		} else {
			skipping_ord = FALSE;
		}

		/*
		 * Get a block from the buffer pool.
		 */
		mutex_enter(&block->md_mutex);
		out = block->out;

		/*
		 * Check if buffer pool is empty.
		 * Note that this block exits via a return only
		 * (i.e. no 'break')
		 */
		while (out != block->in) {
			sam_daddr_t bnx;

			*bn = bnx = block->bn[out];
			*ord = unit;
			block->bn[out] = 0;
			if (++out >= block->limit) {
				out = 0;
			}
			block->out = out;
			bnx <<= mp->mi.m_bn_shift;
			if (bnx < mp->mi.m_fs[*ord].system) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: allocate bad block < "
				    "system: bn=0x%llx ord=%d sys=%x\n",
				    mp->mt.fi_name, bnx, *ord,
				    mp->mi.m_fs[*ord].system);
				sam_req_ifsck(mp, unit,
				    "sam_alloc_block: bad block", &ip->di.id);
				*bn = 0;
				continue;
			}

			/*
			 * Buffer pool less than half full (count blocks
			 * not yet committed),
			 * and map is NOT empty, signal block thread, but
			 * don't wait.
			 */

			block_count = BLOCK_COUNT(block->fill, block->out,
			    block->limit);
			DTRACE_PROBE2(blocks, int, block_count,
			    struct sam_block, block);
			mutex_exit(&block->md_mutex);
			if ((block_count < (block->limit >> 1)) &&
			    !mp->mi.m_fs[unit].map_empty) {
				DTRACE_PROBE1(sam__kick__block, sam_mount_t, mp);
				SAM_KICK_BLOCK(mp);
			}

			/*
			 * Increment to the next unit if stride is GE stripe.
			 */
			stripe = (ip->di.status.b.stripe_width &&
			    !S_ISDIR(ip->di.mode)) ?
			    ip->di.stripe : mp->mt.fi_stripe[block->dt];
			if (stripe) {
				ip->di.stride++;
				if (ip->di.stride >= stripe) {
					ip->di.stride = 0;
					if (mp->mi.m_fs[unit].next_ord) {
						ip->di.unit =
						    mp->mi.m_fs[unit].next_ord;
					} else if (mp->mi.m_dk_max[
					    ip->di.status.b.meta] > 0) {
						ip->di.unit =
						    mp->mi.m_dk_start[
						    ip->di.status.b.meta];
					}
				}
			}
			return (0);
		}
		mutex_exit(&block->md_mutex);

		/*
		 * Buffer pool is empty. If unit is empty, go to the next unit.
		 * If all units are empty and there are no reclaims pending and
		 * not on a shared QFS client, return ENOSPC.
		 * Otherwise, signal the block thread and wait until signaled.
		 * Wake up if no signal received after (at most) 2 second.
		 */
		mp->mi.m_no_blocks++;

skip_this_unit:
		/*
		 * skipping_ord protects against NULL block pointer.
		 * device can go to noalloc state after skipping_ord
		 * is set to false, so also check skip_ord.
		 */
		while ((skipping_ord || mp->mi.m_fs[unit].skip_ord ||
		    (BLOCK_COUNT(block->in, block->out, block->limit) == 0)) &&
		    (mp->mi.m_block.state == SAM_THR_EXIST)) {
			clock_t t_out;

			/*
			 * If this unit is empty or skipped, move to the
			 * next and
			 * head back to top of loop.  Cycle through all
			 * units until all empty.
			 */
			if (skipping_ord || mp->mi.m_fs[unit].skip_ord ||
			    mp->mi.m_fs[unit].map_empty) {
				if (ip->di.blocks &&
				    (mp->mt.fi_config1 &
				    MC_MISMATCHED_GROUPS)) {
					return (SAM_ENOSPC(ip));
				}
				if (mp->mi.m_fs[unit].next_ord) {
					unit = mp->mi.m_fs[unit].next_ord;
				} else if (mp->mi.m_dk_max[
				    ip->di.status.b.meta] > 0) {
					unit = mp->mi.m_dk_start[
					    ip->di.status.b.meta];
				}
				ip->di.unit = (uchar_t)unit;
				if (unit == first_unit && pass++ > 1) {
					return (SAM_ENOSPC(ip));
				}
				break;	/* try next unit (this one's empty) */
			}

			/*
			 * Wait for block thread to fill buffer pool.
			 *
			 * Note: m_block.mutex must be acquired before
			 * m_block.put_mutex
			 * to avoid block thread to complete his work
			 * before we reach cv_timedwait.
			 */
			DTRACE_PROBE1(sam__kick__block__skip, sam_mount_t, mp);
			mutex_enter(&mp->mi.m_block.mutex);
			SAM_KICK_BLOCK(mp);
			t_out = ddi_get_lbolt() + hz + hz;	/* 2 second timeout */
			mp->mi.m_block.wait++;
			(void) cv_timedwait(&mp->mi.m_block.get_cv,
			    &mp->mi.m_block.mutex,
			    t_out);

			/*
			 * Note we don't check to see if the wait timed
			 * out - the check at
			 * the top of the loop will catch that (we will
			 * just wait some
			 * more if no blocks are ready)
			 */
			mp->mi.m_block.wait--;
			mutex_exit(&mp->mi.m_block.mutex);
		}
	}
}


/*
 * ----- sam_request_preallocated - request and wait for preallocation.
 */

int				/* ERRNO if error occurred, 0 if successful. */
sam_prealloc_blocks(sam_mount_t *mp, sam_prealloc_t *pap)
{
	sam_prealloc_t *map;

	/*
	 * Queue preallocation request, signal block thread and wait for
	 * the preallocation to complete.
	 */
	pap->next = NULL;
	pap->wait = 1;

	mutex_enter(&mp->mi.m_block.mutex);
	map = mp->mi.m_prealloc;
	if (map == NULL) {
		mp->mi.m_prealloc = pap;
	} else {
		while (map->next != NULL) {
			map = map->next;
		}
		map->next = pap;
	}

	SAM_KICK_BLOCK(mp);			/* Signal block thread */

	while (pap->wait == 1) {
		mp->mi.m_block.wait++;
		cv_wait(&mp->mi.m_block.get_cv, &mp->mi.m_block.mutex);
		mp->mi.m_block.wait--;
	}
	mutex_exit(&mp->mi.m_block.mutex);
	return (pap->error);
}


/*
 * ----- sam_copy_to_large - move data from small blocks to large.
 *	sam_copy_to_large move a files' data from small blocks to large.
 *	If the file is not at the "limit" of small blocks offsets, the
 *	file is not moved. Also, if an error occurs during the transfer,
 *	the file also not converted.
 */
void
sam_copy_to_large(sam_node_t *ip, int size)
{
	int expand_size;
	int expand_blocks;
	int error;
	sam_bn_t bn;
	int ord;
	int cur_ext;
	int nextra_allocated = 0;
	sam_u_offset_t cur_off, off_this_extent;
	ssize_t size_this_extent;
	struct fbuf *fbp;
	struct buf *bp;
	sam_bn_t old_extent[NSDEXT];
	uchar_t old_ord[NSDEXT];

	ASSERT(!SAM_IS_OBJECT_FILE(ip));

	if (TRANS_ISTRANS(ip->mp)) {
		return;
	}

	if (ip->di.status.b.on_large || ip->di.status.b.direct_map)
		return;
	/*
	 *  Verify that all small block extents are full, and that all
	 *  other in-inode extents are empty.
	 */
	for (cur_ext = 0; cur_ext < NSDEXT; cur_ext++) {
		if (ip->di.extent[cur_ext] == 0)
			return;
		old_extent[cur_ext] = ip->di.extent[cur_ext];
		old_ord[cur_ext] = ip->di.extent_ord[cur_ext];
	}
	for (cur_ext = NSDEXT; cur_ext < NOEXT; cur_ext++)
		if (ip->di.extent[cur_ext] != 0)
			return;

	cur_ext = NSDEXT;
	expand_size = ip->di.rm.size + size;
	expand_blocks = 0;
	while (expand_size >= 0) {
		error = sam_alloc_block(ip, LG, &bn, &ord);
		if (error)
			goto back_it_out;
		expand_size -= LG_BLK(ip->mp, ip->di.status.b.meta);
		ip->di.extent[cur_ext] = bn;
		ip->di.extent_ord[cur_ext] = (uchar_t)ord;
		ip->di.blocks += LG_BLOCK(ip->mp, ip->di.status.b.meta);
		expand_blocks += LG_BLOCK(ip->mp, ip->di.status.b.meta);
		nextra_allocated++;
		cur_ext++;
	}
	/*
	 *	Adjust quotas for added blocks.  Discount large blocks
	 *	replacing small blocks for which we have already accounted.
	 */
	expand_blocks -= NSDEXT * SM_BLOCK(ip->mp, ip->di.status.b.meta);
	(void) sam_quota_balloc(ip->mp, ip,
	    D2QBLKS(expand_blocks), D2QBLKS(expand_blocks), CRED());

	ip->flags.b.changed = 1;

	/*
	 * Copy from the old pages into the newly allocated blocks.
	 */
	cur_ext = NSDEXT;
	cur_off = 0;
	expand_size = ip->di.rm.size;
	size_this_extent = LG_BLK(ip->mp, ip->di.status.b.meta);
	off_this_extent = 0;
	if ((bp = sam_get_buf_header()) == NULL)
		goto back_it_out;
	fbp = NULL;
	while (expand_size > 0) {
		if ((error = fbread(SAM_ITOV(ip), cur_off, MAXBSIZE,
		    S_OTHER, &fbp)))
			break;
		/*
		 * Set up buffer for write.
		 */
		bp->b_edev = ip->mp->mi.m_fs[ip->di.extent_ord[cur_ext]].dev;
		bp->b_dev = cmpdev(bp->b_edev);
		bp->b_lblkno = ip->di.extent[cur_ext];
		bp->b_lblkno <<= (SAM2SUN_BSHIFT + ip->mp->mi.m_bn_shift);
		bp->b_lblkno += (off_this_extent / DEV_BSIZE);
		bp->b_un.b_addr = (caddr_t)fbp->fb_addr;
		bp->b_flags = B_PHYS | B_BUSY | B_WRITE;
		bp->b_bcount = bp->b_bufsize = fbp->fb_count;
		LQFS_MSG(CE_WARN, "sam_copy_to_large(): moving small blocks "
		    "to large block for inode %d\n", ip->di.id.ino);
		if ((error = SAM_BWRITE2(ip->mp, bp)) != 0)
			break;
		cur_off += fbp->fb_count;
		expand_size -= fbp->fb_count;
		size_this_extent -= fbp->fb_count;
		if ((long)size_this_extent <= 0) {
			cur_ext ++;
			size_this_extent = LG_BLK(ip->mp,
			    ip->di.status.b.meta);
			off_this_extent = 0;
		} else {
			off_this_extent += fbp->fb_count;
		}
		fbrelse(fbp, S_OTHER);
		fbp = NULL;
	}
	sam_free_buf_header((sam_uintptr_t *)bp);
	if (fbp != NULL) {
		fbrelse(fbp, S_OTHER);
	}
	if (expand_size <= 0) {
		/*
		 * Move the new "large" extents into the inode, and clear the
		 * rest. Also, add the "on_large" flag, and adjust the blocks.
		 */
		for (cur_ext = 0; cur_ext < NSDEXT; cur_ext++) {
			ip->di.extent[cur_ext] = ip->di.extent[NSDEXT +
			    cur_ext];
			ip->di.extent_ord[cur_ext] =
			    ip->di.extent_ord[NSDEXT + cur_ext];
			ip->di.extent[NSDEXT + cur_ext] = 0;
			ip->di.extent_ord[NSDEXT + cur_ext] = 0;
		}
		ip->di.blocks -= (SM_BLOCK(ip->mp,
		    ip->di.status.b.meta) * NSDEXT);
		ip->di.status.b.on_large = 1;
		ip->flags.b.changed = 1;

		/*
		 *  Finally, free the old small blocks from the directory.
		 */
		for (cur_ext = 0; cur_ext < NSDEXT; cur_ext++)
			sam_free_block(ip->mp, SM, old_extent[cur_ext],
			    old_ord[cur_ext]);
		return;
	}

	/*
	 * Some error has occurred. Remove the recently allocated blocks
	 * (except the first one). Retain the first one, since we wouldn't
	 * be expanding unless the file/directory was expanding anyway.
	 */
back_it_out:
	cur_ext = NSDEXT + 1;
	while (nextra_allocated > 1) {
		sam_free_block(ip->mp, LG, ip->di.extent[cur_ext],
		    ip->di.extent_ord[cur_ext]);
		ip->di.blocks -= LG_BLOCK(ip->mp, ip->di.status.b.meta);
		ip->di.extent[cur_ext] = 0;
		ip->di.extent_ord[cur_ext] = 0;
		cur_ext++;
		nextra_allocated--;
	}
}


/*
 * ----- sam_allocate_block - Allocate blocks.
 *
 * If the block is not allocated and is not sparse, allocate
 * the block.
 */

int
sam_allocate_block(
	sam_node_t *ip,		/* Pointer to the inode. */
	offset_t offset,	/* Logical byte offset in file (longlong_t) */
	sam_map_t flag,
	struct sam_ioblk *iop,	/* Ioblk array. */
	struct map_params *mapp,
	cred_t *credp)
{
	sam_mount_t *mp;
	offset_t boff = iop->blk_off;
	uint32_t *xbnp = mapp->bnp;
	uchar_t *xeip = mapp->eip;
	int blk_count;			/* Count of blocks in the pt_seg */
	sam_bn_t ext_bn = 0;
	int i;
	int len;
	int error = 0;

	ASSERT(!SAM_IS_OBJECT_FILE(ip));

	mp = ip->mp;
	if (iop->imap.bt != SM) {
		blk_count = 1;	/* Extents are large */
	} else {
		blk_count = 2;	/* Extents are small, may allocate 2 blocks */

		/*
		 * Check to see if the block is at the beginning of the page.
		 * If no, back down to the block at the beginning of the page.
		 */
		if (boff != (offset & ~(MAXBSIZE - 1))) {
			boff = offset & ~(MAXBSIZE -1);
			mapp->bnp--;
			mapp->eip--;
		} else if (!ip->mm_pages) {	/* Don't extend unless mmap */
			blk_count = 1;
		}
	}

	/*
	 * Allocate file data block. For 4k DAU, allocate 2 blocks if
	 * mmap pages.
	 * Update inode blocks (di.blocks is in units of 4K);
	 */
	len = 0;
	for (i = 0; i < blk_count; i++) {
		if (*mapp->bnp == 0) {
			int ord;
			int tot_allocsz;
			int allocsz =
			    mp->mi.m_dau[iop->imap.dt].blocks[
			    iop->imap.bt]*iop->num_group;

			/*
			 * Don't increase total blocks in use if this
			 * is a stage.
			 */

			tot_allocsz = ip->flags.b.staging ? 0 : allocsz;
			error = sam_quota_balloc(mp, ip,
			    D2QBLKS(allocsz), D2QBLKS(tot_allocsz), credp);
			if (error) {
				return (error);
			}
			if ((ip->di.id.ino == SAM_BLK_INO)) {
				if (!mp->mi.m_blk_bn) {
					return (ENOSPC);
				}
				/*
				 * Use reserved block for .blocks
				 * file. Avoid deadlock.
				 */
				ord = mp->mi.m_blk_ord;
				ext_bn = mp->mi.m_blk_bn;
				mp->mi.m_blk_bn = 0;
			} else if ((error = sam_alloc_block(ip, iop->imap.bt,
			    &ext_bn, &ord))) {
				sam_quota_bret(mp, ip, D2QBLKS(allocsz),
				    D2QBLKS(tot_allocsz));
				return (error);
			}
			ip->flags.b.changed = 1;
			*mapp->bnp = ext_bn;
			*mapp->eip = (uchar_t)ord;
			mapp->modified = 1;
			len += mapp->dau_size;
			/*
			 * Increment file blocks in units of 4096 bytes.
			 */
			if (!(S_ISSEGI(&ip->di))) {
				ip->di.blocks += allocsz;
			}
			if (S_ISSEGS(&ip->di)) {
				ip->segment_ip->di.blocks += allocsz;
			}
		} else {
			if (i == 0) {
				boff += mapp->dau_size;
			}
		}
		mapp->bnp++;
		mapp->eip++;
	}
	mapp->bnp = xbnp;		/* Restore original block and ord */
	mapp->eip = xeip;

	/*
	 * If DAU allocated, clear pages (entire DAU) if NOT appending.
	 */
	if ((flag == SAM_WRITE_MMAP) || (flag == SAM_FORCEFAULT) ||
	    (flag == SAM_WRITE) || (flag == SAM_WRITE_SPARSE) ||
	    (flag == SAM_ALLOC_ZERO)) {
		sam_offset_t bend = boff + len;
		struct fbuf *fbp;

		if (flag == SAM_WRITE && bend > ip->size) {
			if (boff >= ip->size) {
				return (0);
			}
			bend = ip->size;
		}
		TRACE(T_SAM_DKMAP3, SAM_ITOV(ip), ext_bn, boff, bend);
		len = mp->mi.m_dau[iop->imap.dt].seg[iop->imap.bt];
		if (iop->imap.bt == SM) {

			/*
			 * Check to see if the page is not in memory.  It
			 * can happen
			 * if the page is bigger than the block (2 4K DAUs
			 * in 8k page)
			 */
			if ((error = sam_map_fault(ip, boff, len, mapp))) {
				return (error);
			}
		}
		SAM_SET_LEASEFLG(ip);
		while (boff < bend) {
			fbzero(SAM_ITOV(ip), boff, len, &fbp);
			fbrelse(fbp, S_WRITE);
			boff += len;
		}
		SAM_CLEAR_LEASEFLG(ip);
	}
	return (0);
}
