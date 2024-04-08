/*
 * ----- segment.c - Process the segment access disk functions.
 *
 * Process the SAM-QFS segment access file.
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

#pragma ident "$Revision: 1.79 $"

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
#include <sys/stat.h>
#include <sys/cred.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/fbuf.h>
#include <sys/thread.h>
#include <sys/proc.h>

/* ----- SAMFS Includes */

#include "sam/param.h"
#include "sam/types.h"
#include "sam/syscall.h"
#include "sam/samevent.h"

#include "macros.h"
#include "sblk.h"
#include "inode.h"
#include "ino_ext.h"
#include "mount.h"
#include "arfind.h"
#include "debug.h"
#include "validation.h"
#include "rwio.h"
#include "segment.h"
#include "extern.h"
#include "trace.h"
#include "qfs_log.h"


static void sam_get_index_offset(int segment_ord, offset_t *offset, int *in);
static void sam_set_segment_flags(sam_node_t *bip, sam_node_t *ip);
static int sam_alloc_segment_ino(sam_node_t *bip, int segment_ord,
	sam_node_t ** ipp);
static int sam_segment_issue_callback(sam_node_t *bip,
	enum CALLBACK_type type, sam_callback_segment_t *callback);
static int
sam_make_segment_callback(sam_node_t *bip, sam_node_t *ip,
	enum CALLBACK_type type, sam_callback_segment_t *callback,
	boolean_t write_lock);
static int sam_check_current_segment(sam_node_t *bip, int ordinal,
	sam_node_t ** ipp);

extern void sam_rwlock_common(vnode_t *vp, int w);


/*
 * ----- sam_setup_segment - setup the segment access file for I/O.
 *
 *  Given the index parent inode, stage it if offline.
 *  Called at the beginning of r/w I/O.
 */

int					/* ERRNO if error, 0 if successful. */
sam_setup_segment(
	sam_node_t *bip,		/* Index segment inode pointer */
	uio_t *uiop,			/* user I/O vector array. */
	enum uio_rw rw,
	segment_descriptor_t *seg,
	cred_t *credp)			/* credentials pointer */
{
	int error = 0;

	/*
	 * Segmented files are not supported on shared SAM-QFS.
	 * Since a file system can be upgraded to shared, check and
	 * return an error, if segment on shared.
	 */
	if (SAM_IS_SHARED_FS(bip->mp)) {
		return (ENOTSUP);
	}

	/*
	 * Stage segment file if segment index is offline.
	 */
	if (bip->di.status.b.offline) {
		ASSERT((rw == UIO_WRITE) ? RW_WRITE_HELD(&bip->inode_rwl) : 1);
		error = sam_proc_offline(bip, (offset_t)0,
		    (offset_t)bip->di.rm.info.dk.seg.fsize, NULL, CRED(), NULL);
	}

	/*
	 * Save offset and length in segment descriptor.
	 */
	seg->offset = uiop->uio_loffset;
	seg->resid = uiop->uio_resid;

	/*
	 * If segment index does not exist, acquire RW_WRITER data
	 * lock and wait for all I/O to complete. Then flush and
	 * invalidate pages. Clear write map cache info.
	 * Convert to index seg_file and first segment inode (seg_ino).
	 */
	if (!S_ISSEGI(&bip->di)) {
		sam_node_t *ip;
		int wr_data_lock = 0;

		if (rw == UIO_READ) {
			cmn_err(SAMFS_DEBUG_PANIC,
			    "SAM-QFS: %s: "
			    "no seg_file and read: bip=%p, ino=%d, "
			    "segment_ord=%d\n",
			    bip->mp->mt.fi_name, (void *)bip,
			    bip->di.id.ino, 0);
			if (rw_tryupgrade(&bip->inode_rwl) == 0) {
				RW_UNLOCK_OS(&bip->inode_rwl, RW_READER);
				RW_LOCK_OS(&bip->inode_rwl, RW_WRITER);
			}
		}
		RW_UNLOCK_OS(&bip->inode_rwl, RW_WRITER);
		if (!RW_WRITE_HELD(&bip->data_rwl)) {
			if (rw_tryupgrade(&bip->data_rwl) == 0) {
				RW_UNLOCK_OS(&bip->data_rwl, RW_READER);
				RW_LOCK_OS(&bip->data_rwl, RW_WRITER);
			}
			wr_data_lock = 1;
		}

		mutex_enter(&bip->write_mutex);
		bip->wr_fini++;
		while (bip->cnt_writes) {
			cv_wait(&bip->write_cv, &bip->write_mutex);
		}
		if (--bip->wr_fini < 0) {
			bip->wr_fini = 0;
		}
		mutex_exit(&bip->write_mutex);

		RW_LOCK_OS(&bip->inode_rwl, RW_WRITER);
		if (vn_has_cached_data(SAM_ITOV(bip))) {
			(void) VOP_PUTPAGE_OS(SAM_ITOV(bip), 0, 0, B_INVAL,
			    credp, NULL);
		}

		bip->klength = bip->koffset = 0;
		sam_clear_map_cache(bip);

		if (!S_ISSEGI(&bip->di)) {
			if ((error = sam_get_segment_ino(bip, 0, &ip))) {
				return (error);
			}
			bip->segment_ip = ip;
			bip->segment_ord = 0;
		}
		if (rw == UIO_READ) {
			rw_downgrade(&bip->inode_rwl);
		}
		if (wr_data_lock) {
			rw_downgrade(&bip->data_rwl);
		}
	}
	return (error);
}


/*
 * ----- sam_get_segment - get the segment access file
 *
 *  Given the index parent inode, get the segment access inode.
 *  Called at the beginning of r/w I/O.
 *  NOTE. Segment index inode_rwl lock set on entry. Cleared on exit.
 *  Reacquired in sam_release_segment.
 */

int					/* ERRNO if error, 0 if successful. */
sam_get_segment(
	sam_node_t *bip,		/* Index segment inode pointer */
	uio_t *uiop,			/* user I/O vector array. */
	enum uio_rw	rw,
	segment_descriptor_t *seg,
	sam_node_t **ipp,
	cred_t *credp)
{
	sam_node_t *ip;
	offset_t segment_size;
	int error;
	vnode_t *vip;
	krw_t rw_type;
	int last_segord;

	/*
	 * Type of hold on bip->inode_rwl on entry.
	 */
	rw_type = rw == UIO_READ ? RW_READER : RW_WRITER;

	/*
	 * Get segment inode based on offset.
	 */
	segment_size = SAM_SEGSIZE(bip->di.rm.info.dk.seg_size);
	seg->ordinal = uiop->uio_loffset / segment_size;
	last_segord = bip->di.rm.size / segment_size;
	if ((error = sam_check_current_segment(bip, seg->ordinal, &ip))) {
		return (error);
	}
	if (ip == NULL) {
		if (rw == UIO_READ) {
			if ((rw_tryupgrade(&bip->inode_rwl) == 0)) {
				RW_UNLOCK_OS(&bip->inode_rwl, RW_READER);
				RW_LOCK_OS(&bip->inode_rwl, RW_WRITER);
			}
			/* Backwards support for read */
			if (seg->ordinal < last_segord) {
				if ((error = sam_get_segment_ino(bip,
				    seg->ordinal, &ip))) {
					goto errout;
				}
				if (ip->di.rm.size != segment_size) {
					/*
					 * Segment is sparse, truncate to
					 * segment_size.
					 */
					RW_UNLOCK_OS(&bip->inode_rwl,
					    RW_WRITER);
					RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
					ip->segment_ip = bip;
					error = sam_clear_ino(ip, segment_size,
					    STALE_ARCHIVE, credp);
					RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
					RW_LOCK_OS(&bip->inode_rwl, RW_WRITER);
					if (error) {
						SAM_SEGRELE(SAM_ITOV(ip));
						goto errout;
					}
				}
			}
		} else {	/* UIO_WRITE */
			if (last_segord < seg->ordinal) {
				offset_t size;

				if ((error = sam_check_current_segment(
				    bip, last_segord, &ip))) {
					return (error);
				}
				if (ip != NULL) {
					size = ip->di.rm.size;
				} else {
					if ((error = sam_get_segment_ino(
					    bip, last_segord, &ip))) {
						return (error);
					}
					size = ip->di.rm.size;
					SAM_SEGRELE(SAM_ITOV(ip));
				}
				ip = NULL;
				if (size != segment_size) {
					/*
					 * Segment is sparse, truncate all
					 * segments between ip->di.rm.size and
					 * uiop->uio_loffset to segment size.
					 */
					if ((error = sam_clear_file(bip,
					    (offset_t)uiop->uio_loffset,
					    STALE_ARCHIVE, credp))) {
						return (error);
					}
				}
			}
		}
		if (ip == NULL) {
			if ((error = sam_get_segment_ino(bip, seg->ordinal,
			    &ip))) {
				goto errout;
			}
		}
		if (bip->segment_ip)  {
			SAM_SEGRELE(SAM_ITOV(bip->segment_ip));
		}
		bip->segment_ip = ip;
		bip->segment_ord = seg->ordinal;
		if (rw == UIO_READ) {
			rw_downgrade(&bip->inode_rwl);
		}
	}
	vip = SAM_ITOV(ip);
	VN_HOLD(vip);		/* Hold during I/O */
	TRACE(T_SAM_GETSEGMENT, SAM_ITOV(ip), ip->di.id.ino, seg->ordinal,
	    vip->v_count);
	RW_UNLOCK_OS(&bip->inode_rwl, rw_type);
	sam_rwlock_common(SAM_ITOV(ip), rw == UIO_WRITE ? 1 : 0);
	RW_LOCK_OS(&ip->inode_rwl, rw_type);

	/*
	 * Update uio info to stay within segment. Set seg_file parent ip.
	 */
	uiop->uio_loffset = uiop->uio_loffset % segment_size;
	if ((uiop->uio_loffset + uiop->uio_resid) > segment_size) {
		uiop->uio_resid = segment_size - uiop->uio_loffset;
	}
	seg->ord_resid = uiop->uio_resid;
	ip->segment_ip = bip;
	*ipp = ip;
	return (0);

errout:
	if (rw == UIO_READ) {
		rw_downgrade(&bip->inode_rwl);
	}
	return (error);
}


/*
 * ----- sam_check_current_segment - Get and check current segment held in bip.
 *
 *  Given the index parent inode, check if index parent inode is holding
 *  a segment ordinal == ordinal. If true, validate the segment ordinal
 *  then return ip. If not true, return NULL.
 */

static int		/* ERRNO if error, else 0 if inode pointer returned */
sam_check_current_segment(
	sam_node_t *bip,	/* Index segment inode pointer */
	int ordinal,		/* Segment ordinal */
	sam_node_t **ipp)	/* Pointer if valid */
{
	sam_node_t *ip = NULL;

	if ((bip->segment_ord == ordinal) && bip->segment_ip) {
		ip = bip->segment_ip;
		if (ip->di.rm.info.dk.seg.ord != bip->segment_ord) {
			dcmn_err((CE_NOTE,
			    "SAM-QFS: %s: sam_check_current_segment: ip=%p, "
			    "ino=%d, segment_ord=%d\n",
			    ip->mp->mt.fi_name, (void *)ip, ip->di.id.ino,
			    ordinal));
			sam_req_ifsck(ip->mp, ip->di.rm.info.dk.seg.ord,
			    "sam_check_current_segment: ord", &ip->di.id);
			sam_req_ifsck(ip->mp, bip->segment_ord,
			    "sam_check_current_segment: ord2", &ip->di.id);
			return (ECHRNG);
		}
	}
	*ipp = ip;
	return (0);
}


/*
 * ----- sam_release_segment - release the segment access file
 *
 *  Given the index parent inode, release the segment access inode.
 *  Update segment index file size;
 *  Called at the end of r/w I/O.
 */

void				/* ERRNO if error, 0 if successful. */
sam_release_segment(
	sam_node_t *bip,	/* Index segment inode pointer */
	uio_t *uiop,		/* user I/O vector array. */
	enum uio_rw	rw,
	segment_descriptor_t *seg,
	sam_node_t *ip)
{
	vnode_t *vip;
	uint_t xfer;

	RW_UNLOCK_CURRENT_OS(&ip->inode_rwl);
	RW_UNLOCK_CURRENT_OS(&ip->data_rwl);
	RW_LOCK_OS(&bip->inode_rwl, RW_WRITER);
	if (rw == UIO_WRITE) {
		offset_t size;

		size = (seg->ordinal) *
		    SAM_SEGSIZE(bip->di.rm.info.dk.seg_size);
		size += ip->di.rm.size;
		if (size > bip->di.rm.size) {
			bip->di.rm.size = size;
			bip->flags.b.updated = 1;
		}
	}

	/*
	 * Update uio info to reflect entire file.
	 */
	xfer = seg->ord_resid - uiop->uio_resid;
	uiop->uio_loffset = seg->offset += xfer;
	uiop->uio_resid = seg->resid -= xfer;

	vip = SAM_ITOV(ip);
	SAM_SEGRELE(vip);		/* Free after I/O done */
	TRACE(T_SAM_RELSEGMENT, SAM_ITOV(ip), ip->di.id.ino, seg->ordinal,
	    vip->v_count);
}


/*
 * ----- sam_free_segment_ino - remove segment data inode from index.
 *
 *  Given the index parent inode, remove a segment data inode
 *	number from the index block.
 *  Note: index inode_rwl WRITER lock set on entry and exit.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_free_segment_ino(
	sam_node_t *bip,	/* Index segment inode pointer */
	int segment_ord,
	sam_id_t *id)
{
	offset_t offset;
	int in;
	struct buf *bp;
	sam_ioblk_t ioblk;
	struct sam_val *svp;
	sam_id_t *pid;
	int error = 0;

	ASSERT(S_ISSEGI(&bip->di));

	/*
	 * Get offset which is the beginning of the segment block. in
	 * is the relative offset in the segment block.
	 */
	sam_get_index_offset(segment_ord, &offset, &in);

	ASSERT(offset < bip->di.rm.info.dk.seg.fsize);

	/*
	 * Read and validate the segment index block. Clear the id based
	 * on the segment ord.
	 */
	if ((error = sam_map_block(bip,
	    offset, SAM_SEG_BLK, SAM_READ, &ioblk, CRED()))) {
		return (error);
	}
	if (ioblk.count == 0) {		/* Append to index segment file */
		error = ENOSPC;
		return (error);
	}
	ioblk.blkno += (ioblk.pboff & ~(SAM_SEG_BLK - 1)) >> SAM_DEV_BSHIFT;
	error = SAM_BREAD(bip->mp, bip->mp->mi.m_fs[ioblk.ord].dev,
	    ioblk.blkno, SAM_SEG_BLK, &bp);
	if (error) {
		return (error);
	}
	/*
	 * Get the validation record and make sure it's for this index inode.
	 */
	svp = (struct sam_val *)(void *)
	    (bp->b_un.b_addr + SAM_SEG_BLK - sizeof (struct sam_val));
	if ((svp->v_id.ino != bip->di.id.ino) ||
	    (svp->v_id.gen != bip->di.id.gen) ||
	    (svp->v_version != SAM_VAL_VERSION)) {
		brelse(bp);
		error = EDOM;
		return (error);
	}
	pid = (sam_id_t *)(void *)(bp->b_un.b_addr + in);

	if ((id->ino != pid->ino) || (id->gen != pid->gen)) {
		brelse(bp);
		error = EDOM;
		return (error);
	}
	pid->ino = 0;
	pid->gen = 0;
	bip->flags.b.updated = 1;
	svp->v_time = SAM_SECOND();
	svp->v_relsize = in - sizeof (sam_id_t);
	bdwrite(bp);				/* Write id to index segment */
	return (0);
}


/*
 * ----- sam_get_segment_ino - get segment data inode.
 *
 *  Given the index parent inode, get a segment access inode.
 *  Allocate an inode if not present.
 *  Returns with a inode with activity count incremented.
 *  Note: index inode_rwl WRITER lock set on entry and exit.
 */

int				/* ERRNO if error, 0 if successful. */
sam_get_segment_ino(
	sam_node_t *bip,	/* Index segment inode pointer */
	int segment_ord,
	sam_node_t **ipp)
{
	offset_t offset;
	int in;
	struct buf *bp;
	sam_ioblk_t ioblk;
	struct sam_val *svp;
	sam_id_t *pid;
	sam_node_t *ip;
	int error;

	ip = NULL;

	/*
	 * If this is the first segment. Allocate first segment inode and
	 * move segment index inode to it.
	 */
	if (!S_ISSEGI(&bip->di)) {
		if (segment_ord != 0) {
			dcmn_err((CE_NOTE,
			    "SAM-QFS: %s: "
			    "sam_get_segment_ino: bip=%p, ino=%d, "
			    "segment_ord=%d\n",
			    bip->mp->mt.fi_name, (void *)bip,
			    bip->di.id.ino, segment_ord));
			sam_req_ifsck(bip->mp, segment_ord,
			    "sam_get_segment_ino: bad ord", &bip->di.id);
			return (ECHRNG);
		}
		if ((error = sam_alloc_segment_ino(bip, segment_ord, &ip))) {
			return (error);
		}
	}

	/*
	 * Get offset which is the beginning of the segment block. in
	 * is the relative offset in the segment block. If offset is GT fsize,
	 * this is a new segment. Clear and set the validation record.
	 */
	sam_get_index_offset(segment_ord, &offset, &in);
	while (offset >= bip->di.rm.info.dk.seg.fsize) {
		offset = bip->di.rm.info.dk.seg.fsize;
		if ((error = sam_map_block(bip,
		    offset, SAM_SEG_BLK, SAM_WRITE_BLOCK, &ioblk, CRED()))) {
			return (error);
		}
		bip->flags.b.updated = 1;
		ioblk.blkno += (ioblk.pboff & ~(SAM_SEG_BLK - 1)) >>
		    SAM_DEV_BSHIFT;
		bp = getblk(bip->mp->mi.m_fs[ioblk.ord].dev,
		    (ioblk.blkno<<SAM2SUN_BSHIFT), SAM_SEG_BLK);
		clrbuf(bp);
		svp = (struct sam_val *)(void *)
		    (bp->b_un.b_addr + SAM_SEG_BLK - sizeof (struct sam_val));
		svp->v_version = SAM_VAL_VERSION;
		svp->v_size = offset + SAM_SEG_BLK;
		svp->v_id = bip->di.id;
		error = SAM_BWRITE(bip->mp, bp);
		if (error) {
			return (error);
		}
		bip->di.rm.info.dk.seg.fsize = offset + SAM_SEG_BLK;
		sam_get_index_offset(segment_ord, &offset, &in);
	}

	/*
	 * Read and validate the segment index block. Get the id based
	 * on the segment ord. If zero, allocate a new inode. If an inode
	 * already exists, get it and verify the parent is the index.
	 */
	if ((error = sam_map_block(bip,
	    offset, SAM_SEG_BLK, SAM_READ, &ioblk, CRED()))) {
		goto out;
	}
	if (ioblk.count == 0) {		/* Append to index segment file */
		error = ENOSPC;
		goto out;
	}
	ioblk.blkno += (ioblk.pboff & ~(SAM_SEG_BLK - 1)) >> SAM_DEV_BSHIFT;
	error = SAM_BREAD(bip->mp, bip->mp->mi.m_fs[ioblk.ord].dev,
	    ioblk.blkno, SAM_SEG_BLK, &bp);
	if (error) {
		return (error);
	}
	svp = (struct sam_val *)(void *)
	    (bp->b_un.b_addr + SAM_SEG_BLK - sizeof (struct sam_val));
	if ((svp->v_id.ino != bip->di.id.ino) ||
	    (svp->v_id.gen != bip->di.id.gen) ||
	    (svp->v_version != SAM_VAL_VERSION)) {
		brelse(bp);
		error = EDOM;
		goto out;
	}
	pid = (sam_id_t *)(void *)(bp->b_un.b_addr + in);
	if (pid->ino == 0) {
		if (ip == NULL) {
			if ((error = sam_alloc_segment_ino(bip,
			    segment_ord, &ip))) {
				brelse(bp);
				goto out;
			}
		}
		pid->ino = ip->di.id.ino;
		pid->gen = ip->di.id.gen;
		bip->flags.b.updated = 1;
		svp->v_time = SAM_SECOND();
		svp->v_relsize = in + sizeof (sam_id_t);
		bdwrite(bp);			/* Write id to index segment */
	} else {
		sam_id_t id = *pid;

		brelse(bp);
		if ((error = sam_get_ino(SAM_ITOV(bip)->v_vfsp,
		    IG_EXISTS, &id, &ip))) {
			return (error);
		}
		if ((ip->di.parent_id.ino != bip->di.id.ino) ||
		    (ip->di.parent_id.gen != bip->di.id.gen)) {
			error = EDOM;
			goto out;
		}
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		sam_set_segment_flags(bip, ip);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	}
	*ipp = ip;
	return (0);
out:
	if (ip) {
		SAM_SEGRELE(SAM_ITOV(ip));
	}
	return (error);
}


/*
 * ----- sam_set_segment_flags - set the index inode flags into the segment.
 *
 * The incore flags from the index segment inode must be set in the segment
 * inode when it acquired.
 * NOTE: ip inode_rwl WRITERS lock set on entry and exit.
 */

static void			/* ERRNO if error, 0 if successful. */
sam_set_segment_flags(
	sam_node_t *bip,	/* Index segment inode pointer */
	sam_node_t *ip)		/* Segment inode pointer */
{
	sam_set_directio(ip, bip->flags.b.directio);
	ip->flags.b.qwrite = bip->flags.b.qwrite;
	ip->flags.b.stage_n = bip->flags.b.stage_n;
	ip->flags.b.stage_all = bip->flags.b.stage_all;
}


/*
 * ----- sam_alloc_segment_ino - allocate segment data inode.
 *
 *  Given the index parent inode, allocate a segment access inode.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_alloc_segment_ino(
	sam_node_t *bip,	/* Index segment inode pointer */
	int segment_ord,
	sam_node_t **ipp)
{
	sam_id_t id;
	sam_node_t *ip;
	struct sam_disk_inode *dp, *pdp;
	int i;
	int error;

	if ((error = sam_alloc_inode(bip->mp, S_IFREG, &id.ino))) {
		return (error);
	}
	id.gen = 0;
	if (error = sam_get_ino(SAM_ITOV(bip)->v_vfsp, IG_NEW, &id, &ip)) {
		return (error);
	}
	TRANS_INODE(bip->mp, bip);
	sam_mark_ino(bip, (SAM_UPDATED | SAM_CHANGED));

	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);

	/*
	 * Move index inode into newly created segment inode.
	 */
	id.ino = ip->di.id.ino;
	id.gen = ip->di.id.gen;

	pdp = &bip->di;
	dp = &ip->di;
	*dp = *pdp;

	ip->di.id.ino = id.ino;			/* Restore correct id */
	ip->di.id.gen = id.gen;
	ip->di.parent_id = bip->di.id;	/* Set index segment parent id */

	ip->di.status.b.seg_file = 0;	/* Clear index segment flag */
	ip->di.status.b.seg_ino = 1;	/* Segment inode */
	ip->di.status.b.meta = 0;		/* Segment inode is file data */
	ip->di.nlink = 1;				/* Reset link count */
	sam_set_segment_flags(bip, ip);

	/*
	 * For all except the first segment, clear extents. For the first
	 * segment, the index extents are moved into the segment inode.
	 * Then the index extents are cleared.
	 * Blocks for the entire file are incremented in the index segment.
	 */
	if (S_ISSEGI(&bip->di)) {
		for (i = (NOEXT - 1); i >= 0; i--) {
			ip->di.extent[i] =  0;
			ip->di.extent_ord[i] = 0;
		}
		ip->di.blocks = 0;
		ip->di.lextent = 0;
		ip->di.status.b.on_large = 1;
		ip->di.status.b.archdone = 0;
		ip->di.arch_status = 0;
		ip->di.rm.size = 0;
		if ((error = sam_set_unit(ip->mp, &(ip->di)))) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			return (error);
		}
	} else {
		bip->di.status.b.seg_file = 1;
		bip->di.status.b.on_large = 1;
		ip->size = bip->size;
		for (i = (NOEXT - 1); i >= 0; i--) {
			bip->di.extent[i] =  0;
			bip->di.extent_ord[i] = 0;
		}
		bip->di.lextent = 0;
		if (bip->mp->mt.mm_count) {
			/* Segment ino data on meta device */
			bip->di.status.b.meta = 1;
		}
		if ((error = sam_set_unit(bip->mp, &(bip->di)))) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			return (error);
		}
	}
	/*
	 * Extension inodes:
	 *   types ext_sln, ext_rfa, ext_hlp, ext_acl should be kept with
	 *		the index inode and cleared from the data segment.
	 *		the first 2 shouldn't be there anyway.
	 *
	 *   type  ext_mva should be cleared from the index inode.
	 */
	ip->di.ext_id.ino = ip->di.ext_id.gen = 0;
	ip->di.ext_attrs = 0;
	ip->di.status.b.acl = 0;
	sam_free_inode_ext(bip, S_IFMVA, SAM_ALL_COPIES, &bip->di.ext_id);
	bip->di.ext_attrs &= ~ext_mva;

	/*
	 * Notify arfind & event daemon of segment index inode change.
	 * Stale copy 1, and note the modification time in copy 1 creation time.
	 */
	if (!(bip->di.ar_flags[0] & AR_stale)) {
		struct sam_perm_inode *permip;
		buf_t *bp;

		if ((error = sam_read_ino(bip->mp, bip->di.id.ino,
		    &bp, &permip)) == 0) {
			permip->ar.image[0].creation_time = SAM_SECOND();
			bip->di.ar_flags[0] |= AR_stale;
			bip->di.ar_flags[0] &= ~AR_verified;
			if (TRANS_ISTRANS(bip->mp)) {
				TRANS_WRITE_DISK_INODE(bip->mp, bp, permip,
				    bip->di.id);
			} else {
				bdwrite(bp);
			}
		}
		sam_send_to_arfind(bip, AE_modify, 0);
		if (bip->mp->ms.m_fsev_buf) {
			sam_send_event(bip->mp, &bip->di, ev_modify, 0, 0,
			    bip->di.modify_time.tv_sec);
		}
	}

	/*
	 * Notify arfind & event daemon of data segment change.
	 */
	sam_send_to_arfind(ip, AE_modify, 0);
	if (ip->mp->ms.m_fsev_buf) {
		sam_send_event(ip->mp, &ip->di, ev_modify, 0, 0,
		    ip->di.modify_time.tv_sec);
	}

	ip->di.rm.info.dk.seg.ord = segment_ord; /* Save inode ordinal */
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	*ipp = ip;
	return (0);
}


/*
 * ----- sam_read_segment_info - Return segment inode data.
 *
 *  Given the index parent inode, return segment inode data.
 */

int					/* ERRNO if error, 0 if successful. */
sam_read_segment_info(
	sam_node_t *bip,		/* Index segment inode pointer */
	offset_t offset,
	int size,
	char *buf)
{
	struct buf *bp;
	sam_ioblk_t ioblk;
	struct sam_val *svp;
	int error = 0;

	RW_LOCK_OS(&bip->inode_rwl, RW_READER);
	/*
	 * Stage segment file if segment data is offline.
	 */
	if (bip->di.status.b.offline) {
		if ((error = sam_proc_offline(bip, (offset_t)0,
		    (offset_t)bip->di.rm.info.dk.seg.fsize, NULL,
		    CRED(), NULL))) {
			RW_UNLOCK_OS(&bip->inode_rwl, RW_READER);
			return (error);
		}
	}
	while ((size > 0) && (offset < bip->di.rm.info.dk.seg.fsize)) {
		int out;

		if ((error = sam_map_block(bip,
		    offset, SAM_SEG_BLK, SAM_READ, &ioblk, CRED()))) {
			break;
		}
		if (ioblk.count == 0) {	/* Append to index segment file */
			error = ENOSPC;
			break;
		}
		ioblk.blkno += (ioblk.pboff & ~(SAM_SEG_BLK - 1)) >>
		    SAM_DEV_BSHIFT;
		error = SAM_BREAD(bip->mp, bip->mp->mi.m_fs[ioblk.ord].dev,
		    ioblk.blkno, SAM_SEG_BLK, &bp);
		if (error) {
			break;
		}
		svp = (struct sam_val *)(void *)
		    (bp->b_un.b_addr + SAM_SEG_BLK - sizeof (struct sam_val));
		if ((svp->v_id.ino != bip->di.id.ino) ||
		    (svp->v_id.gen != bip->di.id.gen) ||
		    (svp->v_version != SAM_VAL_VERSION)) {
			brelse(bp);
			error = EDOM;
			break;
		}
		out = MIN(size, SAM_SEG_BLK);
		if ((copyout((char *)bp->b_un.b_addr, (char *)buf, out))) {
			brelse(bp);
			error = EFAULT;
			break;
		} else {
			size -= out;
			buf += out;
			offset += out;
		}
		brelse(bp);
	}
	RW_UNLOCK_OS(&bip->inode_rwl, RW_READER);
	return (error);
}


/*
 * ----- sam_get_index_offset - get index block based on segment ord.
 */

static void
sam_get_index_offset(
	int segment_ord,	/* Segment ordinal */
	offset_t *offset,	/* Segment beginning block offset */
	int *in)		/* Relative offset in segment block */
{
	/*
	 * We could replace the while loop with arithmetic. The tradeoff
	 * is the expense of the division if not in the first index block.
	 *
	 *		if (segment_ord < SAM_MAX_SEG_ORD) {
	 *			*offset = 0;
	 *			*in = segment_ord * sizeof(sam_id_t));
	 *		} else {
	 *			*offset = (segment_ord / SAM_MAX_SEG_ORD) *
	 *				SAM_SEG_BLK;
	 *			*in = (segment_ord % SAM_MAX_SEG_ORD) *
	 *				sizeof(sam_id_t);
	 *		}
	 */

	*offset = 0;
	while (segment_ord >= SAM_MAX_SEG_ORD) {
		segment_ord -= SAM_MAX_SEG_ORD;
		*offset += SAM_SEG_BLK;
	}
	*in = (segment_ord * sizeof (sam_id_t));
}


/*
 * ----- sam_rele_index - Inactivate the segment file index.
 *
 * If segment is inactive and index held, release index.
 */

void
sam_rele_index(sam_node_t *ip)
{
	if (S_ISSEGS(&ip->di) && ip->seg_held && ip->segment_ip) {
		VN_RELE(SAM_ITOV(ip->segment_ip));
		ip->seg_held--;
	}
}


/*
 * ----- sam_flush_segment_index - Update segment index on disk.
 *
 *  Given the index parent inode, sync. write index to disk.
 */

int					/* ERRNO if error, 0 if successful. */
sam_flush_segment_index(sam_node_t *bip)
{
	struct buf *bp;
	int offset;		/* Segment block offset */
	sam_ioblk_t ioblk;
	struct sam_val *svp;
	int error = 0;

	offset = 0;
	while ((offset < bip->di.rm.info.dk.seg.fsize)) {
		if ((error = sam_map_block(bip,
		    offset, SAM_SEG_BLK, SAM_READ, &ioblk, CRED()))) {
			break;
		}
		if (ioblk.count == 0) {	/* Append to index segment file */
			error = ENOSPC;
			break;
		}
		ioblk.blkno += (ioblk.pboff & ~(SAM_SEG_BLK - 1)) >>
		    SAM_DEV_BSHIFT;
		error = SAM_BREAD(bip->mp, bip->mp->mi.m_fs[ioblk.ord].dev,
		    ioblk.blkno, SAM_SEG_BLK, &bp);
		if (error) {
			break;
		}
		svp = (struct sam_val *)(void *)
		    (bp->b_un.b_addr + SAM_SEG_BLK - sizeof (struct sam_val));
		if ((svp->v_id.ino != bip->di.id.ino) ||
		    (svp->v_id.gen != bip->di.id.gen) ||
		    (svp->v_version != SAM_VAL_VERSION)) {
			brelse(bp);
			error = EDOM;
			break;
		}
		offset += SAM_SEG_BLK;
		(void) sam_bwrite_noforcewait_dorelease(bip->mp, bp);
	}
	return (error);
}


/*
 * ----- sam_callback_segment - process the callback for each segment.
 *
 *  Issue callback for each of the data segment inodes.
 *  Given the index parent inode, call the func or system call for
 *  all segment access inode(s).
 */

int					/* ERRNO if error, 0 if successful. */
sam_callback_segment(
	sam_node_t *bip,		/* Index segment inode */
	enum CALLBACK_type type,	/* CALLBACK type */
	sam_callback_segment_t *callback,
	boolean_t write_lock)		/* TRUE if WRITERS lock, else FALSE */
{
	int in;
	int segment_ord;
	int start_ord;
	offset_t segment_size;
	offset_t offset, fileoff;
	sam_node_t *ip;
	struct buf *bp;
	sam_ioblk_t ioblk;
	struct sam_val *svp;
	enum sam_clear_ino_type cflag = callback->p.clear.cflag;
	int error = 0, save_error = 0;

	/*
	 * Stage segment file if segment data is offline.
	 */
	if (bip->di.status.b.offline) {
		error = sam_proc_offline(bip, (offset_t)0,
		    (offset_t)bip->di.rm.info.dk.seg.fsize, NULL, CRED(), NULL);
		if (error) {
			return (error);
		}
	}

	/*
	 * Get segment inodes based on offset and segment_ord.
	 */
	segment_size = SAM_SEGSIZE(bip->di.rm.info.dk.seg_size);

	segment_ord = 0;
	start_ord = 0;
	offset = 0;
	fileoff = 0;
	in = 0;
	if (type == CALLBACK_clear) {
		switch (cflag) {
		case MAKE_ONLINE:
			break;
		case PURGE_FILE:
			break;
		case STALE_ARCHIVE:
			ASSERT(segment_size > 0);
			fileoff = callback->p.clear.length;
			segment_ord = fileoff / segment_size;
			start_ord = fileoff / segment_size;
			sam_get_index_offset(segment_ord, &offset, &in);
			break;
		default:
			return (EINVAL);
		}
	}
	while ((fileoff < bip->di.rm.size) &&
	    (offset < bip->di.rm.info.dk.seg.fsize)) {
		if ((error = sam_map_block(bip,
		    offset, SAM_SEG_BLK, SAM_READ, &ioblk, CRED()))) {
			return (error);
		}
		if (ioblk.count == 0) {	/* Append to index segment file */
			return (ENOSPC);
		}
		ioblk.blkno += (ioblk.pboff & ~(SAM_SEG_BLK - 1)) >>
		    SAM_DEV_BSHIFT;
		bp = NULL;
		while (fileoff < bip->di.rm.size) {
			sam_id_t *pid;

			if (bp == NULL) {
				error = SAM_BREAD(bip->mp,
				    bip->mp->mi.m_fs[ioblk.ord].dev,
				    ioblk.blkno, SAM_SEG_BLK, &bp);
				if (error) {
					return (error);
				}
				svp = (struct sam_val *)(void *)
				    (bp->b_un.b_addr + SAM_SEG_BLK -
				    sizeof (struct sam_val));
				if ((svp->v_id.ino != bip->di.id.ino) ||
				    (svp->v_id.gen != bip->di.id.gen) ||
				    (svp->v_version != SAM_VAL_VERSION)) {
					brelse(bp);
					return (EDOM);
				}
			}
			pid = (sam_id_t *)(void *)(bp->b_un.b_addr + in);
			if (pid->ino != 0) {
				error = sam_get_ino(SAM_ITOV(bip)->v_vfsp,
				    IG_EXISTS, pid, &ip);
				if (error == 0) {
					brelse(bp);
					bp = NULL;



	/* N.B. Bad indentation here to meet cstyle requirements */
	if ((type == CALLBACK_clear) &&
	    (cflag == STALE_ARCHIVE)) {
		if (start_ord == segment_ord) {
			/*
			 * first segment to
			 * truncate down
			 */
			callback->p.clear.length = fileoff % segment_size;
		} else {
			callback->p.clear.length = 0;
		}
		if (callback->p.clear.length ==
		    0) {
			callback->p.clear.cflag = PURGE_FILE;
		}
		if (callback->p.clear.cflag ==
		    PURGE_FILE) {
			error = sam_free_segment_ino(bip,
			    segment_ord,
			    &ip->di.id);
		}
	}



					error = sam_make_segment_callback(bip,
					    ip, type,
					    callback, write_lock);
				}
			}
			segment_ord++;
			sam_get_index_offset(segment_ord, &offset, &in);
			fileoff = segment_ord * segment_size;
			if (in == 0) {	/* Advance to the next SEG block */
				break;
			}
			if (error) {
				if (save_error == 0)  save_error = error;
			}
		}
		if (bp) {
			brelse(bp);
			bp = NULL;
		}
	}
	/*
	 *  STALE_ARCHIVE, truncate extend case.
	 */
	if ((fileoff > bip->di.rm.size) && (type == CALLBACK_clear) &&
	    (cflag == STALE_ARCHIVE)) {
		int last_segord;
		offset_t new_length;

		new_length = callback->p.clear.length;
		last_segord = new_length / segment_size;
		fileoff = bip->di.rm.size;
		start_ord = bip->di.rm.size / segment_size;
		segment_ord = start_ord;
		sam_get_index_offset(start_ord, &offset, &in);

		while (fileoff < new_length) {
			if (offset >= bip->di.rm.info.dk.seg.fsize) {
				if (error = sam_get_segment_ino(bip,
				    segment_ord, &ip)) {
					return (error);
				}
				SAM_SEGRELE(SAM_ITOV(ip));
			}
			if ((error = sam_map_block(bip, offset,
			    SAM_SEG_BLK, SAM_READ,
			    &ioblk, CRED()))) {
				return (error);
			}
			if (ioblk.count == 0) {
				return (ENOSPC);
			}
			ioblk.blkno += (ioblk.pboff & ~(SAM_SEG_BLK - 1)) >>
			    SAM_DEV_BSHIFT;
			bp = NULL;

			while (fileoff < new_length) {
				sam_id_t *pid;

				if (bp == NULL) {
					error = SAM_BREAD(bip->mp,
					    bip->mp->mi.m_fs[ioblk.ord].dev,
					    ioblk.blkno, SAM_SEG_BLK, &bp);
					if (error) {
						return (error);
					}
					svp = (struct sam_val *)
					    (void *)(bp->b_un.b_addr +
					    SAM_SEG_BLK -
					    sizeof (struct sam_val));
					if ((svp->v_id.ino != bip->di.id.ino) ||
					    (svp->v_id.gen != bip->di.id.gen) ||
					    (svp->v_version !=
					    SAM_VAL_VERSION)) {
						brelse(bp);
						bp = NULL;
						return (EDOM);
					}
				}
				pid = (sam_id_t *)(void *)(bp->b_un.b_addr +
				    in);
				if (pid->ino != 0) {
					error = sam_get_ino(bip->vnode->v_vfsp,
					    IG_EXISTS, pid, &ip);
				} else {
					error = sam_alloc_segment_ino(bip,
					    segment_ord, &ip);
					if (error == 0) {
						pid->ino = ip->di.id.ino;
						pid->gen = ip->di.id.gen;
						bip->flags.b.updated = 1;
						svp->v_time = SAM_SECOND();
						svp->v_relsize = in +
						    sizeof (sam_id_t);
						/* Write id to index segment */
						bdwrite(bp);
					} else {
						brelse(bp);
					}
					bp = NULL;
				}
				if (error == 0) {
					if (bp) {
						brelse(bp);
						bp = NULL;
					}
					if (segment_ord == last_segord) {
						callback->p.clear.length =
						    new_length % segment_size;
					} else {
						callback->p.clear.length =
						    segment_size;
					}
					error = sam_make_segment_callback(bip,
					    ip, type,
					    callback, write_lock);
				} else {
					SAM_SEGRELE(SAM_ITOV(ip));
				}
				segment_ord++;
				sam_get_index_offset(segment_ord, &offset, &in);
				fileoff = segment_ord * segment_size;
				if (in == 0) {
					/* Advance to the next SEG block */
					break;
				}
				if ((error) && (save_error == 0)) {
					save_error = error;
				}
			}   /* end while */
			if (bp) {
				brelse(bp);
				bp = NULL;
			}
		}   /* end while */
	}
	return (save_error);
}


/*
 * ----- sam_segment_issue_callback - issue function or system call.
 *
 * Call the func or system call for the given segment.
 */

static int					/* ERRNO, else 0 if success */
sam_segment_issue_callback(
	sam_node_t *ip,				/* Data segment inode */
	enum CALLBACK_type type,		/* Type of callback */
	sam_callback_segment_t *callback)
{
	vnode_t *vp = SAM_ITOV(ip);
	int error = 0;

	switch (type) {

	case CALLBACK_clear: {
		sam_callback_clear_t *clear = &callback->p.clear;
		offset_t length = clear->length;
		enum sam_clear_ino_type cflag = clear->cflag;
		cred_t *credp = clear->credp;

		switch (cflag) {
		case MAKE_ONLINE:
			if (ip->flags.b.staging) {
				error = sam_clear_ino(ip, length, cflag, credp);
			}
			SAM_SEGRELE(vp);
			break;

		case STALE_ARCHIVE:
			error = sam_clear_ino(ip, length, cflag, credp);
			SAM_SEGRELE(vp);
			break;

		case PURGE_FILE:
			mutex_enter(&ip->fl_mutex);
			ip->flags.b.purge_pend = 1;
			mutex_exit(&ip->fl_mutex);
			error = sam_clear_ino(ip, length, cflag, credp);
			/*
			 * Notify arfind and event daemon of removal.
			 */
			sam_send_to_arfind(ip, AE_remove, 0);
			if (ip->mp->ms.m_fsev_buf) {
				sam_send_event(ip->mp, &ip->di, ev_remove, 0, 0,
				    ip->di.change_time.tv_sec);
			}
			sam_return_this_ino(ip, 1);
			if (vp->v_count != 0) {
				dcmn_err((CE_NOTE,
				    "SAM-QFS: %s: PURGE seg: ip=%p, "
				    "ino=%d, vp=%p, cnt=%d\n",
				    ip->mp->mt.fi_name, (void *)ip,
				    ip->di.id.ino, (void *)vp, vp->v_count));
			}
			break;

		default:
			SAM_SEGRELE(vp);
			break;
		}
		}
		break;

	case CALLBACK_syscall: {
		sam_callback_syscall_t *syscall = &callback->p.syscall;

		error = (*(syscall->func))(vp, syscall->cmd, syscall->args,
		    syscall->credp);
		SAM_SEGRELE(vp);
		}
		break;

	case CALLBACK_inactivate: {
		sam_callback_inactivate_t *inact = &callback->p.inactivate;

		if ((error = sam_inactivate_ino(vp, inact->flag)) == 0) {
			sam_return_this_ino(ip, 0);
		} else {
			SAM_SEGRELE(vp);
		}
		}
		break;

	case CALLBACK_wait_rm: {
		sam_callback_wait_rm_t *wait_rm = &callback->p.wait_rm;

		error = sam_wait_archive(ip, wait_rm->archive_w);
		SAM_SEGRELE(vp);
		}
		break;

	case CALLBACK_stat_vsn: {
		sam_callback_sam_vsn_stat_t	*vsn_stat_callback;
		int				copy;
		int				buf_size;
		void				*buf;

		vsn_stat_callback	= &callback->p.sam_vsn_stat;
		copy			= vsn_stat_callback->copy;
		buf_size		= vsn_stat_callback->bufsize;
		buf			= vsn_stat_callback->buf;

		error = sam_vsn_stat_inode_operation(ip, copy, buf_size, buf);

		SAM_SEGRELE(vp);
		}
		break;

	default:
		SAM_SEGRELE(vp);
		break;
	}
	return (error);
}


/*
 * ----- sam_segment_make_segment_callback - Sets up locks, calls
 *
 *	sam_segment_issue_callback to issue the call back for the segment,
 *	releases the locks, returns the result.
 */

static int				/* ERRNO if error, 0 if successful. */
sam_make_segment_callback(
	sam_node_t *bip,		/* Index segment inode */
	sam_node_t *ip,			/* data segment inode */
	enum CALLBACK_type type,	/* CALLBACK type */
	sam_callback_segment_t *callback,
	boolean_t write_lock)		/* TRUE if WRITERS lock, else FALSE */
{
	int error;

	if (bip->di.id.ino == ip->di.parent_id.ino &&
	    bip->di.id.gen == ip->di.parent_id.gen) {
		ip->segment_ip = bip;

		if (write_lock) {
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			RW_UNLOCK_OS(&bip->inode_rwl, RW_WRITER);
		} else {
			RW_LOCK_OS(&ip->inode_rwl, RW_READER);
			RW_UNLOCK_OS(&bip->inode_rwl, RW_READER);
		}

		error = sam_segment_issue_callback(ip, type, callback);


		if (write_lock) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			RW_LOCK_OS(&bip->inode_rwl, RW_WRITER);
		} else {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			RW_LOCK_OS(&bip->inode_rwl, RW_READER);
		}
	} else {
		error = EDOM;
		SAM_SEGRELE(SAM_ITOV(ip));
	}

	return (error);
}


/*
 * ----- sam_callback_one_segment - Performs sam_callback_segment operation
 *	but just for the one specified segment.
 */
int					/* ERRNO if error, 0 if successful. */
sam_callback_one_segment(
	sam_node_t *bip,		/* Index segment inode */
	int segment_ord,		/* segment number */
	enum CALLBACK_type type,	/* CALLBACK type */
	sam_callback_segment_t *callback,
	boolean_t write_lock)		/* TRUE if WRITERS lock, else FALSE */
{
	sam_node_t	*ip;
	int			error;
	uint64_t	segment_size;
	uint64_t	offset;

	segment_size = (uint64_t)SAM_SEGSIZE(bip->di.rm.info.dk.seg_size);

	offset = segment_size * (uint64_t)segment_ord;

	if (offset >= bip->di.rm.size) {
		/* Segment does not exist */
		return (ENOENT);
	}

	error = sam_get_segment_ino(bip, segment_ord, &ip);

	if (error) {
		return (error);
	} else if (ip->segment_ip == (sam_node_t *)NULL) {
		ip->segment_ip = bip;
	}

	ASSERT(ip->segment_ip == bip);

	error = sam_make_segment_callback(bip, ip, type, callback, write_lock);

	return (error);
}
