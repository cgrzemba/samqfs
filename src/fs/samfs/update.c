/*
 * ----- update.c - Process the vfs sync function.
 *
 *	Processes the vfs sync function for a SAM filesystem.
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

#pragma ident "$Revision: 1.141 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/stat.h>

/* ----- SAMFS Includes */

#include "macros.h"
#include "inode.h"
#include "mount.h"
#include "ino_ext.h"
#include "extern.h"
#include "arfind.h"
#include "debug.h"
#include "trace.h"
#include "qfs_log.h"

int sam_message = 0;

static int sam_sync_inodes(sam_mount_t *, int flag);
static int sam_flush_inode_ext(sam_node_t *ip);
static int sam_merge_blocks(sam_mount_t *mp, int flag);

/*
 * ---- sam_update_filsys - Write file system.
 *	Write out timestamp in superblock on primary device.
 *	Merge released blocks into DAU maps.
 *	Merge release inodes into the .inomap file.
 *	Update any changed/modified inode entries.
 *	Write dirty buffers.
 *	Write out timestamp in superblock copy on primary device.
 *  flag parameter definition:
 *  flag  = 0             Commands -- SYNC, UMOUNT or MOUNT.
 *          1 SYNC_ATTR   Sync cached attributes only (periodic).
 *          2 SYNC_CLOSE  Sync due to shutdown.
 */

int					/* ERRNO if error, 0 if successful. */
sam_update_filsys(
	sam_mount_t *mp,		/* The mount table pointer. */
	int flag)			/* Sync flag */
{
	struct sam_sblk *sblk;
	struct samdent *dp;
	uint_t sblk_size;
	buf_t *bp;
	int i;
	int error;
	int sblk_no;
	offset_t space;
	boolean_t force_update;

	/*
	 * For shared client in a shared file system, update superblock
	 * for df. For shared client, sync pages, but notify server of
	 * inode changes. If umounting, wait forever for server response.
	 * Otherwise, do not wait if server is not responding because
	 * fsflush cannot block.
	 *
	 * If failing over, defer page flushing. May have to get map from
	 * server and this can cause deadlock while server is failing over.
	 */
	if (SAM_IS_SHARED_CLIENT(mp)) {
		enum SHARE_flag wait_flag;

		if (mp->mt.fi_status & FS_FAILOVER) {
			return (0);
		}
		if (flag & SYNC_CLOSE) {
			wait_flag = SHARE_nowait;
			flag = SYNC_CLOSE;
		} else {
			wait_flag = SHARE_wait_one;
			flag = 0;
		}
		if ((error = sam_update_shared_filsys(mp,
		    wait_flag, flag)) == 0) {
			(void) sam_sync_inodes(mp, flag);
		}
		return (error);
	}

	sblk = mp->mi.m_sbp;
	sblk_size = mp->mi.m_sblk_size;

	/*
	 * For shared_reader, update superblock for df.
	 * No sync for read-only filesystem.
	 */
	if (mp->mt.fi_mflag & MS_RDONLY) {
		error = 0;
		if (SAM_IS_SHARED_READER(mp)) {
			error = SAM_BREAD(mp, mp->mi.m_fs[0].dev, SUPERBLK,
			    sblk_size, &bp);
			if (error == 0) {
				bcopy((void *)bp->b_un.b_addr, sblk, sblk_size);
				bp->b_flags |= B_STALE | B_AGE;
				brelse(bp);
			} else {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: SBLK read failed: "
				    "bn=0x%x, dev=%lx",
				    mp->mt.fi_name, mp->mi.m_sblk_offset[0],
				    mp->mi.m_fs[0].dev);
			}
		}
		return (error);	/* No sync if read-only */
	}

	/*
	 * Get capacity and space for OSD LUNs in this file system.
	 */
	if (SAM_IS_OBJECT_FS(mp)) {
		space = 0;
		for (i = 0; i < sblk->info.sb.fs_count; i++) {
			dp = &mp->mi.m_fs[i];
			if (is_osd_group(dp->part.pt_type)) {
				error = sam_get_osd_fs_attr(mp, dp->oh,
				    &dp->part);
				if (error == 0) {
					sblk->eq[i].fs.capacity =
					    dp->part.pt_capacity;
					sblk->eq[i].fs.space =
					    dp->part.pt_space;
					space += sblk->eq[i].fs.space;
				} else {
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: Eq %d Error %d: "
					    "cannot get superblock capacity "
					    "and space", mp->mt.fi_name,
					    dp->part.pt_eq, error);
				}
			}
		}
		sblk->info.sb.space = space;
	}

	/*
	 * Update timestamp. Write primary superblock if not SYNC_ATTR.
	 */
	sblk->info.sb.time = SAM_SECOND();
	if (!(flag & SYNC_ATTR)) {
		/*
		 * If we're not already within an LQFS transaction, start one.
		 */
		if (curthread->t_flag & T_DONTBLOCK) {
			(void) sam_update_sblk(mp, 0, 0, TRUE);
		} else {
			if (!panicstr || (TRANS_ISTRANS(mp) == 0)) {
				curthread->t_flag |= T_DONTBLOCK;
				TRANS_BEGIN_ASYNC(mp, TOP_SBUPDATE_UPDATE,
				    TOP_SBUPDATE_SIZE);
				(void) sam_update_sblk(mp, 0, 0, TRUE);
				TRANS_END_ASYNC(mp, TOP_SBUPDATE_UPDATE,
				    TOP_SBUPDATE_SIZE);
				curthread->t_flag &= ~T_DONTBLOCK;
			}
		}
	}

	mutex_enter(&mp->ms.m_inode_mutex);
	(void) sam_update_inode(mp->mi.m_inodir, SAM_SYNC_ONE, FALSE);
	mutex_exit(&mp->ms.m_inode_mutex);

	(void) sam_sync_inodes(mp, flag);	/* sync all changed inodes */

	mutex_enter(&mp->ms.m_inode_mutex);
	(void) sam_update_inode(mp->mi.m_inodir, SAM_SYNC_ONE, FALSE);
	mutex_exit(&mp->ms.m_inode_mutex);

	for (i = 0; i < mp->mt.fs_count; i++) {
		(void) bflush(mp->mi.m_fs[i].dev); /* flush changed buffers */
	}

	if ((sam_merge_blocks(mp, flag)) != 0) {
		/*
		 * if released blocks emptied
		 */
		for (i = 0; i < mp->mt.fs_count; i++) {
			/* flush changed buffers */
			(void) bflush(mp->mi.m_fs[i].dev);
		}
	}

	/*
	 * Write primary superblock if SYNC_ATTR. Otherwise, write backup
	 * superblock (since we already wrote the primary superblock above.)
	 */
	if (flag & SYNC_ATTR) {
		sblk_no = 0;
		force_update = FALSE;
	} else {
		sblk_no = 1;
		force_update = TRUE;
	}

	/*
	 * If we're not already within an LQFS transaction, start one.
	 */
	if (curthread->t_flag & T_DONTBLOCK) {
		(void) sam_update_sblk(mp, 0, sblk_no, force_update);
	} else {
		if (!panicstr || (TRANS_ISTRANS(mp) == 0)) {
			curthread->t_flag |= T_DONTBLOCK;
			TRANS_BEGIN_ASYNC(mp, TOP_SBUPDATE_UPDATE,
			    TOP_SBUPDATE_SIZE);
			(void) sam_update_sblk(mp, 0, sblk_no, force_update);
			TRANS_END_ASYNC(mp, TOP_SBUPDATE_UPDATE,
			    TOP_SBUPDATE_SIZE);
			curthread->t_flag &= ~T_DONTBLOCK;
		}
	}

	/*
	 * Start releaser and notify arfind if over the high water mark.
	 */
	if ((!(flag & SYNC_CLOSE)) &&
	    (sblk->info.sb.space < mp->mi.m_high_blk_count)) {
		(void) sam_start_releaser(mp);
		sam_arfind_hwm(mp);
	}
	return (0);
}


/*
 * ---- sam_sync_inodes - Write inode file.
 * Check all inodes in hash chain that are busy.
 * If inode is accessed, changed, or modified, write pages and update
 * incore inode.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_sync_inodes(
	sam_mount_t *mp,	/* The mount table pointer. */
	int flag)		/* Sync flag */
{
	sam_node_t *ip;
	sam_node_t *nip;
	vnode_t *vp;
	int ino;
	sam_ihead_t *hip;
	int i;
	int syncflg;
	boolean_t sync_attr;

	syncflg = ((flag & SYNC_CLOSE) ||
	    (mp->mt.fi_status & FS_FAILOVER)) ? 0 : B_ASYNC;
	hip = (sam_ihead_t *)&samgt.ihashhead[0];

	/*
	 * If SYNC_ATTR set and deferred access time is enabled (fi_atime == 0),
	 * set sync_attr flag for sam_update_inode.
	 */
	if ((flag & SYNC_ATTR) && (mp->mt.fi_atime == 0)) {
		sync_attr = TRUE;
	} else {
		sync_attr = FALSE;
	}

	for (i = 0; i < samgt.nhino; i++, hip++) {
		sam_node_t *prev_ip;

		mutex_enter(&samgt.ihashlock[i]);
		prev_ip = NULL;
		for (ip = (sam_node_t *)(void *)hip->forw;
		    ip != (sam_node_t *)(void *)hip; ip = nip) {
			vp = SAM_ITOV(ip);
			nip = ip->chain.hash.forw;
			/* If not under this mount point or .inodes? */
			if (ip->mp != mp) {
				continue;
			}
			if (ip->di.id.ino == SAM_INO_INO) {
				continue;
			}
			if (!rw_tryenter(&ip->inode_rwl, RW_WRITER)) {
				continue;
			}
			ino = ip->di.id.ino;
			if (ip->flags.b.free || (ino == 0) ||
			    (SAM_ITOD(ino) > mp->mi.m_inodir->di.rm.size)) {
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				continue;
			}

			/*
			 * Lift the hash mutex during the page sync and inode
			 * sync.  Do not sync pages staging because partially
			 * staged pages are locked and cannot be unlocked until
			 * written.  Do not sync pages if stage_n.
			 */
			VN_HOLD(vp);	/* Keep inode busy to hold in hash */
			mutex_exit(&samgt.ihashlock[i]);
			if (prev_ip != NULL) {
				if (S_ISSEGS(&prev_ip->di)) {
					SAM_SEGRELE(SAM_ITOV(prev_ip));
				} else {
					VN_RELE(SAM_ITOV(prev_ip));
				}
				prev_ip = NULL;
			}
			if ((!(flag & SYNC_ATTR)) &&
			    (vn_has_cached_data(vp) != 0) &&
			    !ip->flags.b.staging &&
			    !ip->flags.b.stage_n &&
			    (ip->flags.bits &
			    (SAM_ACCESSED|SAM_UPDATED|SAM_CHANGED|SAM_DIRTY))) {
				(void) VOP_PUTPAGE_OS(vp, 0, 0, syncflg,
				    CRED(), NULL);
			}
			(void) sam_update_inode(ip, SAM_SYNC_ALL, sync_attr);
			mutex_enter(&samgt.ihashlock[i]);
			nip = ip->chain.hash.forw;
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			prev_ip = ip;
		}
		mutex_exit(&samgt.ihashlock[i]);
		if (prev_ip != NULL) {
			if (S_ISSEGS(&prev_ip->di)) {
				SAM_SEGRELE(SAM_ITOV(prev_ip));
			} else {
				VN_RELE(SAM_ITOV(prev_ip));
			}
		}
	}
	return (0);
}


/*
 * ---- sam_update_inode - Write inode.
 *  If inode accessed/modified/changed, update buffer with incore inode.
 *  Delay write or sync write buffer based on sync type.
 */

int				/* errno if err, 0 if ok. */
sam_update_inode(
	sam_node_t *ip,		/* The inode table pointer. */
	enum sam_sync_type st,	/* SAM_SYNC_ALL: sync all, delayed write */
				/* SAM_SYNC_ONE: sync one inode */
				/* SAM_SYNC_PURGE: sync of one purged inode */
	boolean_t sync_attr)	/* Attribute-only-sync flag */
{
	int ino;
	uint_t ino_flags, inotm_flags;
	struct sam_perm_inode *permip;
	int error = 0;
	buf_t *bp;
	sam_mount_t *mp;

	mp = ip->mp;
	ino = ip->di.id.ino;

	/*
	 * Sync any incore access control list to inode extension(s).
	 */
	if (ip->aclp && (ip->aclp->flags & ACL_MODIFIED)) {
		(void) sam_acl_flush(ip);
	}

	/*
	 * If inode accessed/modified/changed, sync.
	 */
	ino_flags = ip->flags.bits;
	inotm_flags = ino_flags & SAM_UPDATE_FLAGS;
	if (inotm_flags || ip->accstm_pnd) {
		/*
		 * Rather than update the access time for every
		 * NFS/CLIENT read operation (it gets expensive), we check
		 * the following:
		 * 1) Is this the "normal" sync interval, then update
		 * 2) Is this NFS/CLIENT and are we only updating ACCESS time,
		 *	  defer the update.
		 */
		if (st != SAM_SYNC_ALL &&
		    (SAM_THREAD_IS_NFS() || SAM_IS_SHARED_CLIENT(mp)) &&
		    (mp->mt.fi_status & FS_FAILOVER) == 0 &&
		    inotm_flags == SAM_ACCESSED) {
			ip->accstm_pnd++;
			ip->flags.bits &= ~SAM_UPDATE_FLAGS;
			return (0);
		}

		/*
		 * If sync_attr flag and only access flag set for cached inode
		 * update, defer the inode access update until the inode is
		 * inactivated, the file system is unmounted, or ~1 minute
		 * has elapsed since the inode was accessed. Note, sync inode
		 * so the releaser has accurate inode data if SAM and the space
		 * used is above the low water mark.
		 */
		if (sync_attr && inotm_flags == SAM_ACCESSED &&
		    (!SAM_IS_STANDALONE(mp) &&
		    (mp->mi.m_sbp->info.sb.space > mp->mi.m_low_blk_count))) {
			if ((ip->di.access_time.tv_sec + 60) > SAM_SECOND()) {
				return (0);
			}
		}

		TRACE(T_SAM_UPDATEFS, SAM_ITOV(ip),
		    (SAM_ITOV(ip))->v_count, ino,
		    ip->flags.bits);
		if (SAM_IS_SHARED_CLIENT(mp)) {
			if ((error = sam_update_shared_ino(ip,
			    st, TRUE)) == 0) {
				ip->flags.bits &=
				    ~(SAM_UPDATE_FLAGS|SAM_VALID_MTIME);
				ip->accstm_pnd = 0;
			}
			return (error);
		}

		/*
		 * If purging inode, acquire inode allocation mutex prior
		 * to reading inode. Otherwise, this inode can be allocated
		 * prior to the disk copy update.  If not purging inode,
		 * update allocated block count for object files.
		 */
		if (st == SAM_SYNC_PURGE) {
			mutex_enter(&mp->ms.m_inode_mutex);
			sam_free_inode(mp, &ip->di);
		} else if (SAM_IS_OBJECT_FILE(ip) &&
		    (ip->flags.bits & SAM_UPDATED)) {
			sam_osd_update_blocks(ip, 0);
		}
		if ((error = sam_read_ino(mp, ino, &bp, &permip)) != 0) {
			TRACE(T_SAM_UPDATE1, SAM_ITOV(ip), ino, error, 0);
			if (st == SAM_SYNC_PURGE) {
				mutex_exit(&mp->ms.m_inode_mutex);
			}
			return (error);
		}
		ip->flags.bits &= ~(SAM_UPDATE_FLAGS|SAM_VALID_MTIME);
		ip->cl_flags &= ~(SAM_UPDATE_FLAGS|SAM_VALID_MTIME);
		ip->accstm_pnd = 0;
		permip->di = ip->di;
		if (ip->di.version == SAM_INODE_VERS_2) {
			permip->di2 = ip->di2;
		}
		if (st == SAM_SYNC_PURGE) {
			mutex_exit(&mp->ms.m_inode_mutex);
		}

		if (TRANS_ISTRANS(ip->mp)) {
			/*
			 * Pass only a sector size buffer containing
			 * the inode, otherwise when the buffer is copied
			 * into a cached roll buffer then too much memory
			 * gets consumed if 8KB inode buffers are passed.
			 */

			/*
			 * LQFS kludge for know:  If reserved inode, get doff.
			 */
			if (ip->doff == 0) {
				sam_node_t *dip;
				offset_t offset;
				sam_ioblk_t ioblk;
				offset_t size;

				dip = ip->mp->mi.m_inodir;
				offset = (offset_t)SAM_ITOD(ip->di.id.ino);
				size = SAM_ISIZE;

				/*
				 * Might want to check if inode lock is already
				 * held here for LQFS ?
				 */
				RW_LOCK_OS(&dip->inode_rwl, RW_READER);
				error = sam_map_block(dip, offset, size,
				    SAM_READ, &ioblk, CRED());
				RW_UNLOCK_OS(&dip->inode_rwl, RW_READER);
				if (!error) {
					/* Save as disk byte offset */
					ip->doff = ldbtob(fsbtodb(ip->mp,
					    ioblk.blkno)) + ioblk.pboff;
					ip->dord = ioblk.ord;
				} else {
					ip->doff = 0;
					ip->dord = 0;
				}
			}
			TRANS_LOG(ip->mp, (caddr_t)permip,
			    ip->doff, ip->dord,
			    sizeof (struct sam_perm_inode),
			    (caddr_t)P2ALIGN((uintptr_t)permip, DEV_BSIZE),
			    DEV_BSIZE);
			/*
			 * Might need to flush segments and extents here for
			 * LQFS?
			 */
			brelse(bp);
		} else if (st == SAM_SYNC_ALL) {
			bdwrite(bp);
		} else {
			if ((error = sam_write_ino_sector(mp, bp, ino))) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: cannot sync inode %d.%d",
				    mp->mt.fi_name, ino, ip->di.id.gen);
				return (error);
			}

			/*
			 * Flush inode extents, segment index, and indirect
			 * blocks if file modified or changed.
			 */
			if (ino_flags & (SAM_UPDATED | SAM_CHANGED)) {
				if ((error == 0) && ip->di.ext_id.ino) {
					error = sam_flush_inode_ext(ip);
				}
				if ((error == 0) && S_ISSEGI(&ip->di)) {
					error = sam_flush_segment_index(ip);
				}
				if (error == 0) {
					error = sam_flush_extents(ip);
				}
				return (error);
			}
		}
	}
	return (error);
}


/*
 * ---- sam_write_ino_sector - Sync write the inode sector.
 * Get buffer header and use buffer info to sync. write 512 byte inode.
 */

int					/* errno if err, 0 if ok. */
sam_write_ino_sector(
	sam_mount_t *mp,		/* Pointer to the mount table */
	buf_t *bp,			/* The buffer pointer. */
	sam_ino_t ino)			/* The i-number */
{
	buf_t *tbp;
	sam_ino_t base_ino;
	int error = 0;

	if ((tbp = sam_get_buf_header()) == NULL) {
		if (SAM_BWRITE(mp, bp)) {
			return (EIO);
		}
	} else {

		tbp->b_proc = bp->b_proc;
		tbp->b_edev = bp->b_edev;
		tbp->b_dev = bp->b_dev;

		base_ino = ((ino-1) &
		    ~((mp->mt.fi_wr_ino_buf_size >> SAM_ISHIFT) - 1)) + 1;
		tbp->b_un.b_addr = (char *)SAM_ITOO(base_ino, bp->b_bcount, bp);
		tbp->b_lblkno = ((SAM_ITOD(base_ino) & (bp->b_bcount-1)) /
		    DEV_BSIZE) + bp->b_lblkno;
		tbp->b_bcount = mp->mt.fi_wr_ino_buf_size;
		tbp->b_bufsize = tbp->b_bcount;
		tbp->b_flags = B_PHYS | B_BUSY | B_WRITE;
		tbp->b_file = SAM_ITOP(mp->mi.m_inodir);
		tbp->b_offset = (base_ino - 1) * SAM_ISIZE;
		if ((tbp->b_flags & B_READ) == 0) {
			LQFS_MSG(CE_WARN, "sam_write_ino_sector(): "
			    "bdev_strategy writing mof 0x%x edev %ld "
			    "nb %d\n", tbp->b_blkno * 512, tbp->b_edev,
			    tbp->b_bcount);
		} else {
			LQFS_MSG(CE_WARN, "sam_write_ino_sector(): "
			    "bdev_strategy reading mof 0x%x edev %ld "
			    "nb %d\n", tbp->b_blkno * 512, tbp->b_edev,
			    tbp->b_bcount);
		}
		(void) bdev_strategy(tbp);
		error = biowait(tbp);
		sam_free_buf_header((sam_uintptr_t *)tbp);
		brelse(bp);
	}
	return (error);
}


/*
 * ----- sam_flush_inode_ext - flush inode extensions.
 * Flush all inode extensions for this inode.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_flush_inode_ext(sam_node_t *ip)
{
	buf_t *bp;
	struct sam_inode_ext *eip;
	sam_id_t eid;
	int error = 0;

	eid = ip->di.ext_id;
	while (eid.ino) {
		if (error = sam_read_ino(ip->mp, eid.ino, &bp,
					(struct sam_perm_inode **)&eip)) {
			return (error);
		}
		if (EXT_HDR_ERR(eip, eid, ip)) {	/* Validate header */
			sam_req_ifsck(ip->mp, -1,
			    "sam_flush_inode_ext: EXT_HDR", &ip->di.id);
			brelse(bp);
			return (ENOCSI);
		}
		eid = eip->hdr.next_id;
		if (TRANS_ISTRANS(ip->mp)) {
			sam_ioblk_t ioblk;

			RW_LOCK_OS(&ip->mp->mi.m_inodir->inode_rwl, RW_READER);
			error = sam_map_block(ip->mp->mi.m_inodir,
			    (offset_t)SAM_ITOD(eid.ino), SAM_ISIZE,
			    SAM_READ, &ioblk, CRED());
			RW_UNLOCK_OS(&ip->mp->mi.m_inodir->inode_rwl,
			    RW_READER);
			if (!error) {
				offset_t doff;

				doff = ldbtob(fsbtodb(ip->mp, ioblk.blkno))
				    + ioblk.pboff;
				TRANS_EXT_INODE(ip->mp, eid, doff, ioblk.ord);
			} else {
				error = 0;
			}
			brelse(bp);
		} else if ((error = sam_write_ino_sector(ip->mp, bp,
		    eip->hdr.id.ino))) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: cannot sync inode ext %d.%d",
			    ip->mp->mt.fi_name, eip->hdr.id.ino,
			    eip->hdr.id.gen);
			break;
		}
	}
	return (error);
}


/*
 * ----- sam_flush_extents - Flush indirect extents.
 * Recusively call this routine until all indirect blocks are flushed.
 */

int					/* ERRNO if error, 0 if successful. */
sam_flush_extents(sam_node_t *ip)
{
	int i;
	int kptr[NIEXT + 1];
	int error = 0;

	if (ip->di.status.b.direct_map || S_ISLNK(ip->di.mode)) {
		return (0);
	}

	if (ip->di.extent[NDEXT] != 0) {
		daddr_t blkno;

		blkno = ip->di.extent[NDEXT];
		blkno <<= (ip->mp->mi.m_bn_shift + SAM2SUN_BSHIFT);
		blkflush(ip->mp->mi.m_fs[ip->di.extent_ord[NDEXT]].dev, blkno);
	}
	for (i = 0; i < (NIEXT+1); i++) {
		kptr[i] = -1;
	}
	for (i = (NOEXT - 1); i > NDEXT; i--) {
		int set;
		int kptr_index;

		if (ip->di.extent[i] == 0) {
			continue;
		}
		set = 0;
		kptr_index = i - NDEXT;
		error = sam_flush_indirect_block(ip, SAM_FLUSH_BC,
		    kptr, kptr_index,
		    &ip->di.extent[i], &ip->di.extent_ord[i],
		    (i - (NDEXT + 1)), &set);
		if (error)  break;
	}
	return (error);
}


/*
 * ---- sam_merge_blocks - process the released blocks array list.
 * sam_merge_blocks is called during file system update, or
 * when the released block list is full. The list is merged.
 * The .blocks inode is flushed, if any blocks were merged.
 * The count of blocks merged is returned.
 */

static int			/* Set if any blocks merged */
sam_merge_blocks(
	sam_mount_t *mp,	/* The mount table pointer. */
	int flag)		/* The sync flag */
{
	sam_node_t *ip;
	int bt;
	int active;
	int i;
	int j;
	sam_fb_pool_t *fbp;
	sam_freeblk_t *fap;
	offset_t space, mm_space;
	int blocks_merged = 0;
	int inoblk_lock	= 0;

	ip = mp->mi.m_inoblk;
	for (bt = 0; bt < SAM_MAX_DAU; bt++) {
		fbp = &mp->mi.m_fb_pool[bt];
		active = fbp->active;	/* Currently filling this array */
		for (j = 0; j < SAM_NO_FB_ARRAY; j++) {
			active = (active + 1) & 1;	/* Free this array */
			fbp->active = (active + 1) & 1;
			fap = &fbp->array[active];
			mutex_enter(&fap->fb_mutex);
			if (fap->fb_count == 0) {
				mutex_exit(&fap->fb_mutex);
				continue;
			}
			for (i = 0; i < fap->fb_count; i++) {
				if (bt == SM) {
					if (inoblk_lock == 0) {
						RW_LOCK_OS(&ip->inode_rwl,
						    RW_WRITER);
						inoblk_lock = 1;
					}
					(void) sam_merge_sm_blks(ip,
					    fap->fb_bn[i],
					    fap->fb_ord[i]);
				} else {
					(void) sam_free_lg_block(mp,
					    fap->fb_bn[i],
					    fap->fb_ord[i]);
				}
				blocks_merged++;
			}
			fap->fb_count = 0;
			mutex_exit(&fap->fb_mutex);
		}
	}
	if (inoblk_lock == 1) {
		TRANS_INODE(mp, ip);
		ip->flags.b.updated = 1;
		(void) sam_update_inode(ip, SAM_SYNC_ONE, FALSE);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	}

	if (blocks_merged) {
		struct sam_sblk *sblk;	/* The superblock pointer. */

		mutex_enter(&mp->mi.m_sblk_mutex);
		sblk = mp->mi.m_sbp;
		for (space = 0, mm_space = 0, i = 0;
		    i < sblk->info.sb.fs_count; i++) {
			if (sblk->eq[i].fs.type == DT_META) {
				mm_space += sblk->eq[i].fs.space;
			} else {
				space += sblk->eq[i].fs.space;
			}
		}
		sblk->info.sb.mm_space = mm_space;
		sblk->info.sb.space = space;
		mutex_exit(&mp->mi.m_sblk_mutex);
	}

	if (flag & SYNC_CLOSE) {
		if (mp->mi.m_blk_bn != 0) {
			sam_free_lg_block(mp, mp->mi.m_blk_bn,
			    mp->mi.m_blk_ord);
			mp->mi.m_blk_bn = 0;
		}
	}
	return (blocks_merged);
}


/*
 * ---- sam_free_lg_block - Process a large release block.
 *	Update large block in bit maps.
 */

int					/* ERRNO if error, 0 if successful. */
sam_free_lg_block(sam_mount_t *mp, sam_bn_t ext, int ord)
{
	struct sam_sblk *sblk;
	offset_t bit;
	uint_t *wptr;
	uint_t offset;
	sam_bn_t blk;
	sam_daddr_t bn;
	buf_t *bp;
	offset_t blocks;
	offset_t capacity, *space_ptr;
	fs_wmstate_e wmstate;
	int dt;

	sblk = mp->mi.m_sbp;
	dt = mp->mi.m_fs[ord].part.pt_type == DT_META ? MM : DD;
	bn = ext;
	bn <<= mp->mi.m_bn_shift;
	bit = bn;
	if (DIF_LG_SHIFT(mp, dt)) {
		bit >>= DIF_LG_SHIFT(mp, dt);
	} else {
		bit /= LG_DEV_BLOCK(mp, dt);
	}
	offset = bit >> NBBYSHIFT;
	mutex_enter(&mp->mi.m_fs[ord].eq_mutex);

	blk = (sam_bn_t)(sblk->eq[ord].fs.allocmap +
	    (offset >> SAM_DEV_BSHIFT));
	if ((SAM_BREAD(mp, mp->mi.m_fs[sblk->eq[ord].fs.mm_ord].dev, blk,
	    SAM_DEV_BSIZE, &bp))) {
		mutex_exit(&mp->mi.m_fs[ord].eq_mutex);
		return (EIO);
	}
	wptr = (uint_t *)(void *)(bp->b_un.b_addr + (offset & SAM_LOG_WMASK));
	bit &= 0x1f;
	if ((bn < sblk->eq[ord].fs.system) ||
	    (sblk->eq[ord].fs.system == 0) ||
	    (bn > sblk->eq[ord].fs.capacity)) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: free_lg_block: bn=0x%llx, ord=%d",
		    mp->mt.fi_name, (long long)bn, ord);
		brelse(bp);
		mutex_exit(&mp->mi.m_fs[ord].eq_mutex);
	} else {
		if (sam_getbit(*wptr, (31 - bit)) == 1) {
			sam_message++;
			if (sam_message < 10) {
				cmn_err(CE_NOTE,
				    "SAM-QFS: %s: Free Block already "
				    "free: bn=0x%llx, ord=%d",
				    mp->mt.fi_name, (long long)bn, ord);
			}
		}
		if ((mp->mi.m_blk_bn == 0) &&
		    (dt == mp->mi.m_inoblk->di.status.b.meta)) {
			/*
			 * Keep in reserve 1 large meta block for the .blocks
			 * file.
			 */
			mp->mi.m_blk_bn = ext;
			mp->mi.m_blk_ord = ord;
			brelse(bp);
			mutex_exit(&mp->mi.m_fs[ord].eq_mutex);
		} else {
			*wptr = sam_setbit(*wptr, (31 - bit));
			bdwrite(bp);
			mutex_exit(&mp->mi.m_fs[ord].eq_mutex);

			mutex_enter(&mp->mi.m_sblk_mutex);
			if (dt == DD) {
				blocks = sblk->info.sb.dau_blks[LG];
				capacity = sblk->info.sb.capacity;
				space_ptr = &sblk->info.sb.space;
			} else {
				blocks = sblk->info.sb.mm_blks[LG];
				capacity = sblk->info.sb.mm_capacity;
				space_ptr = &sblk->info.sb.mm_space;
			}
			if (sblk->eq[ord].fs.num_group > 1) {
				blocks *= sblk->eq[ord].fs.num_group;
			}
			if ((sblk->eq[ord].fs.space += blocks) >
			    sblk->eq[ord].fs.capacity) {
				sblk->eq[ord].fs.space =
				    sblk->eq[ord].fs.capacity;
			}
			if ((*space_ptr += blocks) > capacity) {
				*space_ptr = capacity;
			}
			mutex_exit(&mp->mi.m_sblk_mutex);

			/*
			 * Added a block back to the maps.  Clear map_empty
			 * flag.  Always wake threads up waiting because of
			 * ENOSPC.  Note: these threads will signal the block
			 * thread.
			 */
			mp->mi.m_fs[ord].map_empty = 0;
			/*
			 * Wake up threads waiting because of ENOSPC
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
		 * Determine watermark state
		 */
		wmstate = FS_LHWM;		/* assume in the middle */
		if (sblk->info.sb.space < mp->mi.m_high_blk_count)
			wmstate = FS_HWM;
		else if (sblk->info.sb.space > mp->mi.m_low_blk_count)
			wmstate = FS_LWM;
		if (mp->mi.m_xmsg_state != wmstate) {
			sam_check_wmstate(mp, wmstate);
		}
	}
	return (0);
}


/*
 * ---- sam_merge_sm_blks - Process a small release block.
 *	Put the small block in the .blocks hidden file.
 *  3 cases:  empty ... full  ... end
 *            full  ... empty ... end
 *            full  ... ...   ... end
 *  Compress end for case 2.
 */

int				/* ERRNO if error, 0 if successful. */
sam_merge_sm_blks(sam_node_t *ip, sam_bn_t ext, int ord)
{
	offset_t offset;
	offset_t size;
	int i, dt;
	int error;
	buf_t *bp;
	sam_mount_t *mp;
	struct sam_inoblk *smp;
	struct sam_inoblk *esmp;
	sam_ioblk_t ioblk;
	daddr_t bn;
	sam_daddr_t bnx;
	int foundempty;
	offset_t empty;		/* Empty place for new entry */
	offset_t full;		/* Last full entry */
	offset_t end;		/* End of list (0xffffffff) */

	mp = ip->mp;
	full = 0;
	end = 0;
	foundempty = 0;
	offset = 0;
	bn = ext;
	bn <<= mp->mi.m_bn_shift;

	/*
	 * Verify that the block number we're releasing is valid.
	 */

	if ((ord >= mp->mt.fs_count) ||
	    (bn < mp->mi.m_sbp->eq[ord].fs.system) ||
	    (!mp->mi.m_sbp->eq[ord].fs.system) ||
	    (bn > mp->mi.m_sbp->eq[ord].fs.capacity)) {
		cmn_err(SAMFS_CE_DEBUG_PANIC,
		    "SAM-QFS: %s: sam_merge_sm_blks: bn=0x%llx, ord=%d",
		    mp->mt.fi_name, (long long)bn, ord);
		return (ERANGE);
	}

	while (offset < ip->di.rm.size) {
		if ((error = sam_map_block(ip, offset,
		    (offset_t)SM_BLK(mp, MM), SAM_READ, &ioblk, CRED()))) {
			return (error);
		}
		ioblk.blkno += ioblk.pboff >> SAM_DEV_BSHIFT;
		error = SAM_BREAD(mp, mp->mi.m_fs[ioblk.ord].dev,
		    ioblk.blkno, (long)SM_BLK(mp, MM), &bp);
		if (error) {
			return (error);
		}
		smp = (struct sam_inoblk *)(void *)bp->b_un.b_addr;
		for (i = 0; i < SM_BLK(mp, MM);
			i += sizeof (struct sam_inoblk)) {
			if (smp->bn == 0xffffffff) {	/* End of list */
				end = offset;
				goto out;
			} else if (smp->bn != 0) {		/* Full entry */
				full = offset;
				dt = (mp->mi.m_fs[ord].part.pt_type ==
					DT_META) ? MM : DD;
				bnx = smp->bn;
				bnx <<= mp->mi.m_bn_shift;
				if ((bnx == (bn & ~SM_BMASK(mp, dt))) &&
				    (smp->ord == ord)) {
					smp->free |= 1 << ((bn &
							    SM_BMASK(mp, dt)) >>
					    DIF_SM_SHIFT(mp, dt));
					if (smp->free == SM_BITS(mp, dt)) {
						if (sam_free_lg_block(mp,
							smp->bn, ord) == 0) {
							smp->bn = 0;
							smp->ord = 0;
							smp->free = 0;


	/* N.B. Bad indentation here to meet cstyle requirements */
	if ((mp->mi.m_inoblk_blocks -=
	    LG_DEV_BLOCK(mp, dt)) < 0) {
		mp->mi.m_inoblk_blocks = 0;
	}


						}
					}
					bdwrite(bp);
					return (0);
				}
			} else if (foundempty == 0) {	/* Empty entry */
				foundempty = 1;
				empty = offset;
				esmp = smp;
			}
			smp++;
			offset += sizeof (struct sam_inoblk);
		}
		brelse(bp);
	}
	/*
	 * No end of list (0xffffffff) found (could happen if error extending).
	 */
	size = ip->di.rm.size;
	if ((error = sam_map_block(ip, offset, (offset_t)SM_BLK(mp, MM),
	    SAM_WRITE_BLOCK, &ioblk, CRED()))) {
		return (error);
	}
	ioblk.blkno += ioblk.pboff >> SAM_DEV_BSHIFT;
	bp = getblk(mp->mi.m_fs[ioblk.ord].dev, (ioblk.blkno<<SAM2SUN_BSHIFT),
	    SM_BLK(mp, MM));
	clrbuf(bp);
	smp = (struct sam_inoblk *)(void *)bp->b_un.b_addr;
	smp->bn = 0xffffffff;
	if (SAM_BWRITE(mp, bp)) {
		ip->di.rm.size = size;
	}
	ip->flags.b.changed = 1;
	return (EIO);
out:
	dt = MM;
	if (foundempty) {				/* Fill in empty slot */

		if ((empty & ~SM_MASK(mp, dt)) != (end & ~SM_MASK(mp, dt))) {
			brelse(bp);		/* empty in previous block */
			if ((error = sam_map_block(ip,
			    (offset_t)empty,
			    (offset_t)sizeof (struct sam_inoblk),
			    SAM_READ, &ioblk, CRED()))) {
				return (error);
			}
			ioblk.blkno += ioblk.pboff >> SAM_DEV_BSHIFT;
			ioblk.blkno &= ~(SM_DEV_BLOCK(mp, dt) - 1);
			error = SAM_BREAD(mp, mp->mi.m_fs[ioblk.ord].dev,
			    ioblk.blkno, SM_BLK(mp, dt), &bp);
			if (error) {
				return (error);
			}
			smp = (struct sam_inoblk *)(void *)(bp->b_un.b_addr +
			    (ioblk.pboff & SM_MASK(mp, dt)));
		} else {
			smp = esmp;
		}
		/* Record free small dau block number */
		/* Record free small dau device ordinal */
		smp->ord = (ushort_t)ord;
		dt = (mp->mi.m_fs[ord].part.pt_type == DT_META) ? MM : DD;
		smp->bn = (bn & ~SM_BMASK(mp, dt)) >> mp->mi.m_bn_shift;
		/* Place within large dau */
		smp->free = 1 << ((bn & SM_BMASK(mp, dt)) >>
		    DIF_SM_SHIFT(mp, dt));
		mp->mi.m_inoblk_blocks += SM_DEV_BLOCK(mp, dt);
		if (empty < full) {
			bdwrite(bp);
			return (0);
		}
		full = empty;	/* New empty is last full entry */
	} else {
		/* No empty, write over 0xffffffff */
		/* Record free small dau block number */
		/* Record free small dau device ordinal */
		smp->ord = (ushort_t)ord;
		dt = (mp->mi.m_fs[ord].part.pt_type == DT_META) ? MM : DD;
		smp->bn = (bn & ~SM_BMASK(mp, dt)) >> mp->mi.m_bn_shift;
		/* Place within large dau */
		smp->free = 1 << ((bn & SM_BMASK(mp, dt)) >>
		    DIF_SM_SHIFT(mp, dt));
		mp->mi.m_inoblk_blocks += SM_DEV_BLOCK(mp, dt);
		full = end;
	}
	full += sizeof (struct sam_inoblk);		/* Next entry */
	if ((full & SM_MASK(mp, dt)) == 0) {	/* If at beginning of block */
		bdwrite(bp);
		if ((error = sam_map_block(ip, (offset_t)full,
		    (offset_t)SM_BLK(mp, MM),
		    SAM_WRITE_BLOCK, &ioblk, CRED()))) {
			return (error);
		}
		ioblk.blkno += ioblk.pboff >> SAM_DEV_BSHIFT;
		bp = getblk(mp->mi.m_fs[ioblk.ord].dev,
		    (ioblk.blkno<<SAM2SUN_BSHIFT),
		    SM_BLK(mp, MM));
		clrbuf(bp);
		smp = (struct sam_inoblk *)(void *)bp->b_un.b_addr;
		ip->flags.b.changed = 1;
	} else {
		smp++;
	}
	smp->bn = 0xffffffff;
	smp->ord = 0;
	smp->free = 0;
	bdwrite(bp);

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

	return (0);
}


/*
 * Request that this filesystem be noted for an fs check.
 * Set status bits in the root slice's superblock, and
 * push them out to disk.
 *
 * Requests may specify that a particular slice be checked,
 * or just that the filesystem be checked (slice == -1).
 * Each slice contains a pair of status bits indicating
 * that either a non-specific or specific request was made
 * to fsck this slice.
 *
 * These bits are cleared by fsck.
 */
void
sam_req_ifsck(sam_mount_t *mp, int slice, char *msg, sam_id_t *idp)
{
	int i;
	struct sam_sblk *sblk;

	if (mp == NULL) {
		return;
	}
	if (mp->mt.fi_mflag & MS_RDONLY) {
		return;
	}
	if (SAM_IS_CLIENT_OR_READER(mp)) {
		return;
	}

	mutex_enter(&mp->ms.m_waitwr_mutex);
	sblk = mp->mi.m_sbp;
	if (slice == -1) {
		/*
		 * Set request for filesystem, unknown/all slice(s).
		 */
		for (i = 0; i < sblk->info.sb.fs_count; i++) {
			if ((sblk->eq[i].fs.fsck_stat & SB_FSCK_GEN) == 0) {
				sblk->eq[i].fs.fsck_stat |= SB_FSCK_GEN;
			}
		}
		if ((sblk->info.sb.state & SB_FSCK_GEN) == 0) {
			if (idp) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: inode %d.%d fsck "
				    "requested: %s",
				    mp->mt.fi_name, idp->ino, idp->gen, msg);
			} else {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: fsck requested: %s",
				    mp->mt.fi_name, msg);
			}
			sblk->info.sb.state |= SB_FSCK_GEN;
		}
	} else {
		/*
		 * Set request for a known/specific FS slice
		 */
		if (slice >= 0 && slice < sblk->info.sb.fs_count) {
			if ((sblk->eq[slice].fs.fsck_stat & SB_FSCK_SP) == 0) {
				if (idp) {
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: inode %d.%d "
					    "fsck requested for eq %d: %s",
					    mp->mt.fi_name,
					    idp->ino, idp->gen,
					    mp->mi.m_fs[slice].part.pt_eq, msg);
				} else {
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: fsck requested "
					    "for eq %d: %s",
					    mp->mt.fi_name,
					    mp->mi.m_fs[slice].part.pt_eq, msg);
				}
				sblk->eq[slice].fs.fsck_stat |= SB_FSCK_SP;
			}
		}
		if ((sblk->info.sb.state & SB_FSCK_SP) == 0) {
			cmn_err(CE_WARN, "SAM-QFS: %s: fsck requested: %s",
			    mp->mt.fi_name, msg);
			sblk->info.sb.state |= SB_FSCK_SP;
		}
	}
	mutex_exit(&mp->ms.m_waitwr_mutex);

#if DEBUG
	/*
	 * If DEBUG, Panic previous server and this host.
	 */
	{
		int prev_srvr = 0;

		prev_srvr = mp->ms.m_prev_srvr_ord;
		if (prev_srvr != 0) {
			(void) sam_proc_notify(mp->mi.m_inodir,
			    NOTIFY_panic, prev_srvr,
			    NULL);
			delay(hz);
		}
		if (idp) {
			cmn_err(CE_PANIC,
			    "SAM-QFS: %s: sam_req_ifsck %s ino=%d.%d",
			    mp->mt.fi_name, msg, idp->ino, idp->gen);
		} else {
			cmn_err(CE_PANIC, "SAM-QFS: %s: sam_req_fsck %s",
			    mp->mt.fi_name, msg);
		}
	}
#endif /* DEBUG */
}


void
sam_req_fsck(
	sam_mount_t *mp,
	int slice,
	char *msg)
{
	sam_req_ifsck(mp, slice, msg, NULL);
}


/*
 * ----- sam_sync_meta -
 * Async. flush directory pages to disk. While the I/O is in
 * progress, flush directory inode. Then flush the child inode if one.
 * NOTE: requires parent inode write lock set.
 * NOTE: requires child inode write lock set (if one).
 */

int
sam_sync_meta(
	sam_node_t *pip,		/* Parent inode pointer */
	sam_node_t *cip,		/* Child inode pointer, if one */
	cred_t *credp)
{
	vnode_t *vp = SAM_ITOV(pip);
	int error;
	int flag = B_ASYNC;

	/*
	 * A shared-reader does directory block reads when it recognizes
	 * that the directory inode has changed.   The shared-writer needs to
	 * use SYNC I/O on the directory blocks to make sure those blocks are
	 * valid before it updates the directory inode.
	 */
	if (SAM_IS_SHARED_WRITER(pip->mp)) {
		flag = 0;
	}
	if (VOP_PUTPAGE_OS(vp, 0, 0, flag, credp, NULL)) {
		TRACE(T_SAM_FLUSHERR4, vp, pip->di.id.ino, vp->v_count,
		    pip->flags.bits);
	}
	ASSERT(RW_OWNER_OS(&pip->inode_rwl) == curthread);
	error = sam_update_inode(pip, SAM_SYNC_ONE, FALSE);
	if (cip) {
		ASSERT(RW_OWNER_OS(&cip->inode_rwl) == curthread);
		error = sam_update_inode(cip, SAM_SYNC_ONE, FALSE);
	}

	/*
	 * Wait until directory I/O has finished.
	 */
	if (pip) {
		mutex_enter(&pip->write_mutex);
		while (pip->cnt_writes) {
			pip->wr_fini++;
			cv_wait(&pip->write_cv, &pip->write_mutex);
			if (--pip->wr_fini < 0) {
				pip->wr_fini = 0;
			}
		}
		mutex_exit(&pip->write_mutex);
	}
	return (error);
}

/*
 * ----- sam_update_sblk - update super block.
 * sam_update_sblk synchronously updates the specified super block.
 *   sblk_no = 0 is the primary superblock; sblk_no = 1 is the backup superblock
 *   force_update TRUE always writes the superblock.
 *   force_update FALSE writes the superblock only if the space or state was
 *     changed..
 * Note: Only ordinal 0 can update a backup super block.
 *
 */

boolean_t
sam_update_sblk(struct sam_mount *mp, uchar_t ord, int sblk_no,
    boolean_t force_update)
{
	struct sam_sblk *sblk;
	struct buf *bp;
	uint_t sblk_size;
	boolean_t ret = TRUE;

	sblk = mp->mi.m_sbp;
	if (force_update || (mp->mi.m_prev_space != sblk->info.sb.space) ||
	    (mp->mi.m_prev_mm_space != sblk->info.sb.mm_space) ||
	    (mp->mi.m_prev_state != sblk->info.sb.state)) {
		sblk_size = mp->mi.m_sblk_size;
		ASSERT(ord < mp->mt.fs_count);
		ASSERT(((ord == 0) ? 2 : 1) > sblk_no);
		bp = getblk(mp->mi.m_fs[ord].dev,
		    (mp->mi.m_sblk_offset[sblk_no] << SAM2SUN_BSHIFT),
		    sblk_size);
		if (bp->b_bufsize != sblk_size) {
			bp->b_flags |= B_STALE | B_AGE;
			brelse(bp);
			bp = getblk(mp->mi.m_fs[ord].dev,
			    (mp->mi.m_sblk_offset[sblk_no] << SAM2SUN_BSHIFT),
			    sblk_size);
		}
		if ((bp->b_flags & B_ERROR) || (bp->b_bufsize != sblk_size)) {
			bp->b_flags |= B_STALE | B_AGE;
			brelse(bp);
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: SBLK getbuf failed: bn=0x%x, dev=%lx",
			    mp->mt.fi_name, mp->mi.m_sblk_offset[sblk_no],
			    mp->mi.m_fs[ord].dev);
			ret = FALSE;
		} else {
			if (ord != 0) {
				sblk->info.sb.ord = ord;
			}
			bcopy(sblk, (void *)bp->b_un.b_addr, sblk_size);
			sblk->info.sb.ord = 0;
			/*
			 * LQFS: delta the whole superblock
			 */
			TRANS_DELTA(mp,
			    ldbtob(fsbtodb(mp,
			    mp->mi.m_sblk_offset[sblk_no])),
			    0, sblk_size, DT_SB, NULL, 0);
			if (SAM_BWRITE(mp, bp) != 0) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: SBLK Update failed: "
				    "bn=0x%x, dev=%lx",
				    mp->mt.fi_name,
				    mp->mi.m_sblk_offset[sblk_no],
				    mp->mi.m_fs[ord].dev);
				ret = FALSE;
			} else {
				mp->mi.m_prev_state = sblk->info.sb.state;
				mp->mi.m_prev_space = sblk->info.sb.space;
				mp->mi.m_prev_mm_space = sblk->info.sb.mm_space;
			}
		}
	}
	return (ret);
}

/*
 * ----- sam_update_all_sblks - update all super blocks.
 * sam_update_all_sblks synchronously updates all the super blocks
 * of a file system.
 *
 */

boolean_t
sam_update_all_sblks(struct sam_mount *mp)
{
	int ord;
	boolean_t ret = TRUE;
	struct sam_sblk *sblk;

	sblk = mp->mi.m_sbp;
	sam_update_filsys(mp, 0);		/* Takes care of ord 0 */
	for (ord = 1; ord < sblk->info.sb.fs_count; ord++) {
		if (sblk->eq[ord].fs.state == DEV_OFF ||
		    sblk->eq[ord].fs.state == DEV_DOWN) {
			continue;
		}
		if (!sam_update_sblk(mp, ord, 0, TRUE)) {
			ret = FALSE;
		}
	}
	return (ret);
}
