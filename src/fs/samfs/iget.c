/*
 * ----- sam/iget.c - Process the iget/iput functions.
 *
 *	Processes the SAMFS inode get and inode put functions.
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

#pragma ident "$Revision: 1.222 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/t_lock.h>
#include <sys/sysmacros.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/cred.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/stat.h>
#include <sys/mode.h>
#include <sys/fs_subr.h>
#include <sys/proc.h>
#include <sys/disp.h>
#include <sys/file.h>
#include <sys/ddi.h>

/* ----- SAMFS Includes */

#include "sam/fioctl.h"
#include "sam/samaio.h"

#include "inode.h"
#include "mount.h"
#include "ino_ext.h"
#include "macros.h"
#include "ioblk.h"
#include "extern.h"
#include "debug.h"
#include "syslogerr.h"
#include "trace.h"
#include "qfs_log.h"
#include "objnode.h"

extern int sam_mask_signals;

#ifdef METADATA_SERVER
extern struct vnodeops *samfs_vnodeopsp;
#endif
extern struct vnodeops *samfs_client_vnodeopsp;

static void sam_clear_incore_inode(sam_node_t *ip);
static void sam_release_ino(sam_node_t *ip);
static sam_node_t *sam_freeino(void);
static int sam_verify_ino(sam_id_t *fp, sam_iget_t flag, sam_node_t *ip,
	boolean_t write_lock);
static int sam_inode_constructor(void *buf, void *not_used, int notused);
static void sam_inode_destructor(void *buf, void *not_used);
static int sam_read_disk_ino(sam_mount_t *mp, sam_id_t *fp, buf_t **bpp,
	struct sam_perm_inode **pip);


/*
 * ----- sam_get_ino - Get/lock an inode.
 *	Given a file id (i-number/generation number), find it and
 *  return it held. First check the inode active hash cache.
 *  If there, return the inode locked. If not there either get a
 *  free inode if the inode limit has been reached or allocate a
 *  new inode if the inode limit has not been reached.
 *  flag is set:
 *   IG_NEW		- get new inode.
 *   IG_EXISTS	- get existing inode for given id, id must match.
 *   IG_DNLCDIR	- get existing inode for given id.ino, id.ino must match,
 *		  but not id.gen because it will always be passed in as zero.
 *   IG_STALE	- get existing inode, or if stale, get stale inode, id.ino
 *		  must match, but generation may be less than id.gen.
 *   IG_SET	- create a new inode and initialize it using the record passed
 *		  in via ipp.
 */

int				/* ERRNO if error, 0 if successful--ipp set. */
sam_get_ino(
	struct vfs *vfsp,	/* vfs pointer for this instance of mount. */
	sam_iget_t flag,	/* get new, existing, dir cache, or stale ino */
	sam_id_t *fp,		/* file i-number & generation number. */
	sam_node_t **ipp)	/* pointer to pointer to inode (returned). */
{
	int error;
	sam_mount_t *mp;
	vnode_t *vp;
	sam_node_t *ip = NULL;
	sam_node_t *rip;
	sam_ino_record_t *irec;
	buf_t *bp;
	struct sam_perm_inode *permip;
	vtype_t	type;
	kmutex_t *ihp;
	uint_t	sv_count;

	irec = (flag == IG_SET) ? ((sam_ino_record_t *)(void *)*ipp) : NULL;
	*ipp = NULL;

	mp = (sam_mount_t *)(void *)vfsp->vfs_data;
	TRACE(T_SAM_FIND, (vfs_t *)vfsp, fp->ino, fp->gen,
	    SAM_IHASH(fp->ino, vfsp->vfs_dev));

	if (fp->ino == 0) {
		return (ENOENT);
	}
	if (mp->mt.fi_status & FS_UMOUNT_IN_PROGRESS) {
		TRACE(T_SAM_UMNT_IGET, NULL, mp->mt.fi_status,
		    (sam_tr_t)mp, EBUSY);
		return (EBUSY);
	}

	/* Check to see if the inode is in the hash chain. */
	error = sam_check_cache(fp, mp, flag, NULL, &rip);
	if (error || (rip != NULL)) {
		if (rip) {
			*ipp = rip;
			vp = SAM_ITOV(rip);
			if (flag == IG_SET) {
				RW_LOCK_OS(&rip->inode_rwl, RW_WRITER);
				error = sam_reset_client_ino(rip, 0, irec);
				RW_UNLOCK_OS(&rip->inode_rwl, RW_WRITER);
				if (error) {
					*ipp = NULL;
				}
			} else {
				TRACE(T_SAM_CACHE_1, vp, rip->di.id.ino,
				    rip->di.id.gen,
				    vp->v_count);
			}
		}
		if (error) {
			TRACE(T_SAM_CACHE_E1, NULL, fp->ino, fp->gen, error);
		}
		return (error);
	}

	/*
	 * If the number of inodes is over the limit, get a free inode.
	 * If no free inodes, signal reclaim thread to free inodes from the
	 * dnlc cache which then allows the incore inode to be reused.
	 * If free inode returned, inode WRITERS lock set.
	 */
	if (samgt.inocount > samgt.ninodes) {
		ip = sam_freeino();
	}
	if (ip == NULL) {
		ip = (sam_node_t *)kmem_cache_alloc(samgt.inode_cache,
		    KM_SLEEP);
		ip->mp = NULL;
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		ip->flags.bits = 0;
		ip->flags.b.busy = 1;	/* Inode is active (SAM_BUSY) */
		vp = SAM_ITOV(ip);
		vp->v_count = 1;	/* init ref count for this vnode */
		mutex_enter(&samgt.ifreelock);
		samgt.inocount++;
		mutex_exit(&samgt.ifreelock);
		ip->chain.hash.forw   = ip->chain.hash.back   = ip;
		ip->chain.free.forw   = ip->chain.free.back   = ip;
	}
	if (ip->mp != mp) {
		if (ip->mp != NULL) {
			SAM_VFS_RELE(ip->mp);
		}
		ip->mp = mp;
		SAM_VFS_HOLD(mp);
	}
	ip->dev = vfsp->vfs_dev;
	ip->di.id.ino = 0;
	sam_clear_incore_inode(ip);
	if ((mp->mt.fi_type == DT_META_OBJ_TGT_SET) && (!ip->objnode)) {
		/* 'mat' FS and this inode does not have an object node */
		sam_objnode_alloc(ip);
	}

	/*
	 * Verify inode was not created in the hash chain since last check.
	 * NOTE, put in hash chain if not found.
	 */
	error = sam_check_cache(fp, mp, flag, ip, &rip);
	if (error || (rip != NULL)) {
		if (rip) {
			*ipp = rip;
			vp = SAM_ITOV(rip);
			if (flag == IG_SET) {
				RW_LOCK_OS(&rip->inode_rwl, RW_WRITER);
				error = sam_reset_client_ino(rip, 0, irec);
				RW_UNLOCK_OS(&rip->inode_rwl, RW_WRITER);
				if (error) {
					*ipp = NULL;
				}
			} else {
				TRACE(T_SAM_CACHE_2, vp, rip->di.id.ino,
				    rip->di.id.gen,
				    vp->v_count);
			}
		}
		if (error) {
			TRACE(T_SAM_CACHE_E2, NULL, fp->ino, fp->gen, error);
		}
		return (error);
	}

	/*
	 * Reinitialize / recycle the vnode first then set fields
	 * interesting to us such as the FS ops, etc.
	 *
	 * As of Solaris 10 some fields that we're interested in are no
	 * longer directly accessible. Consequently, we have to call vn_reinit
	 * to initialize them. vn_reinit clears the vnode completely. If we're
	 * reusing a vnode we may not want to trash its existing reference
	 * count so we'll save it and reassign it after the reinit call.
	 * This avoids some funky ref count problems that can occur.
	 */
	vp = SAM_ITOV(ip);

	mutex_enter(&(vp)->v_lock);
	sv_count = vp->v_count;
	vn_reinit(vp);
	vp->v_count = sv_count;
	mutex_exit(&(vp)->v_lock);
	vp->v_vfsp = vfsp;
	if (SAM_IS_SHARED_FS(mp)) {
		vn_setops(vp, samfs_client_vnodeopsp);
	} else {
#ifdef METADATA_SERVER
		vn_setops(vp, samfs_vnodeopsp);
#else
		vn_setops(vp, samfs_client_vnodeopsp);
#endif
	}

	if (mp->mt.fi_type == DT_META_OBJ_TGT_SET) {
		sam_objnode_init(ip);
	}

	ASSERT(vp->v_data == (caddr_t)ip);
	if (flag == IG_SET) {
		ip->di.mode = 0;
		if ((error = sam_reset_client_ino(ip, 0, irec))) {
			goto out;
		}
	} else {
		if ((error = sam_read_disk_ino(mp, fp, &bp, &permip))) {
			goto out;
		}

		ip->di = permip->di;	/* Copy disk image to incore inode */
		if (ip->di.version == SAM_INODE_VERS_2) {
			ip->di2 = permip->di2;
		} else {
			bzero((char *)&ip->di2,
			    sizeof (struct sam_disk_inode_part2));
		}
		brelse(bp);
	}

	type = IFTOVT(S_ISREQ(ip->di.mode) ? S_IFREG : ip->di.mode);
	vp->v_type = type;
	if (type == VBLK || type == VCHR) {		/* if a special */
		ip->rdev = expldev((dev32_t)ip->di.psize.rdev);
		vp->v_rdev = ip->rdev;
	}

	if (fp->ino == SAM_ROOT_INO) {
		vp->v_flag = VROOT;
	} else if (SAM_PRIVILEGE_INO(ip->di.version, ip->di.id.ino)) {
		vp->v_flag = 0;
	} else {
#ifdef VMODSORT
		vp->v_flag = (S_ISREG(ip->di.mode)) ? VMODSORT : 0;
#else
		vp->v_flag = 0;
#endif
	}

	if ((error = sam_verify_ino(fp, flag, ip, TRUE))) {
		goto out;
	}

	if (SAM_IS_OBJECT_FILE(ip)) {
		if ((error = sam_osd_create_obj_layout(ip))) {
			goto out;
		}
	} else {
		ASSERT(ip->olp == NULL);
	}

	/* Backwards support for residency time prior to 3.3.0-28 */
	if (ip->di.residence_time == 0) {
		ip->di.residence_time = ip->di.creation_time;
		ip->flags.b.changed = 1;
	}

	/* Backwards support prior to 3.2, 3.3.1, etc. */
	/*
	 * Clear previous settings. Stageall and partial not allowed with
	 * direct.
	 */
	if (ip->di.status.b.direct) {
		ip->di.status.b.stage_all = 0;
	}

	if (ip->di.status.b.bof_online) {
		if (ip->di.psize.partial == 0) {
			/*
			 * Backwards support for partial attribute prior to 3.2.
			 */
			if (ip->di.status.b.on_large) {
				ip->di.psize.partial = 16;
			} else {
				ip->di.psize.partial = 8;
			}
			if (ip->di.status.b.offline) {
				if (ip->di.extent[0] == 0) {
					ip->di.status.b.pextents = 0;
				} else {
					ip->di.status.b.pextents = 1;
				}
			}
		} else {
			/*
			 * Check for a 3.3.1 condition where the pextents flag
			 * was set, yet no data blocks existed.
			 */
			if (S_ISREG(ip->di.mode)) {
				if ((ip->di.status.b.pextents) &&
				    (ip->di.blocks == 0) &&
				    (ip->di.rm.size > 0)) {
					ip->di.status.b.pextents = 0;
				}
			}
		}
	}

	/*
	 * Only a regular file can be a segmented file, thus if a file is
	 * not a regular file, then explicitly clear the segmented file
	 * and segmented inode bit flags for the file.
	 */

	if (!S_ISREG(ip->di.mode)) {
		ip->di.status.b.seg_file = 0;
		ip->di.status.b.seg_ino  = 0;
	}

	/*
	 * Regular and directory files must be on disk.
	 */

	if (S_ISREG(ip->di.mode) || S_ISDIR(ip->di.mode)) {
		ip->di.rm.media = DT_DISK;
	}

	/* End of backward support */

	sam_set_size(ip);
	ip->cl_allocsz = ip->size;
	if (ip->cl_allocsz < (ip->di.blocks * SAM_BLK)) {
		ip->cl_allocsz = (ip->di.blocks * SAM_BLK);
	}

	/*
	 * Set directio if mount directio or file directio set.
	 */
	sam_reset_default_options(ip);
	ip->flags.b.abr = 0;
	if (S_ISREQ(ip->di.mode)) {
		ip->flags.bits |= SAM_DIRECTIO;
		if (ip->mp->mt.fi_version == SAMFS_SBLKV2) {
			ip->di.status.b.remedia = 0;
		}
	}
	ip->flags.b.stage_n = ip->di.status.b.direct;
	ip->flags.b.stage_all = ip->di.status.b.stage_all;
	/*
	 * Save the disk inode's byte offset for possible QFS logging.
	 */
	if ((TRANS_ISTRANS(ip->mp)) ||
	    SAM_PRIVILEGE_INO(ip->di.version, ip->di.id.ino)) {
		sam_node_t *dip;
		offset_t offset;
		sam_ioblk_t ioblk;
		offset_t size;

		if (ip->di.id.ino != SAM_INO_INO) {
			dip = ip->mp->mi.m_inodir;
		} else {
			dip = ip;
		}
		offset = (offset_t)SAM_ITOD(ip->di.id.ino);
		size = SAM_ISIZE;

		if (dip != ip) {
			RW_LOCK_OS(&dip->inode_rwl, RW_READER);
		}
		error = sam_map_block(dip, offset, size, SAM_READ, &ioblk,
		    CRED());
		if (dip != ip) {
			RW_UNLOCK_OS(&dip->inode_rwl, RW_READER);
		}
		if (!error) {
			ip->doff = ldbtob(fsbtodb(ip->mp, ioblk.blkno))
			    + ioblk.pboff;
			ip->dord = ioblk.ord;
		} else {
			ip->doff = 0;
			ip->dord = 0;
		}
	}
	TRANS_MATA_IGET(ip->mp, ip);

	*ipp = ip;
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	error = 0;
	TRACE(T_SAM_FIND1, vp, ip->di.id.ino, ip->di.id.gen, vp->v_count);
	return (error);
out:
	TRACE(T_SAM_FIND3, SAM_ITOV(ip), ip->di.mode, ip->di.id.ino,
	    ip->di.id.gen);
	if (error == EBADR) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		*ipp = ip;
		TRACE(T_SAM_FIND4, SAM_ITOV(ip), error, fp->ino, fp->gen);
		return (error);
	}
	/*
	 * We created an inode placeholder but then couldn't read the inode.
	 * Mark it as stale so active threads don't try to use it; remove it
	 * from the hash chain so new threads don't find it at all.
	 */
	ip->flags.b.stale = 1;
	ihp = &samgt.ihashlock[SAM_IHASH(ip->di.id.ino, ip->dev)];
	mutex_enter(ihp);
	if (ip->flags.b.hash) {
		SAM_DESTROY_OBJ_LAYOUT(ip);
		SAM_UNHASH_INO(ip);
	}
	mutex_exit(ihp);
	TRACE(T_SAM_FIND5, SAM_ITOV(ip), error, fp->ino, fp->gen);
	sam_release_ino(ip);
	return (error);
}


/*
 * ----- sam_clear_incore_inode - Clear incore flags.
 */

static void
sam_clear_incore_inode(sam_node_t *ip)
{
	int i;

	time_t curr_time;

	curr_time = SAM_SECOND();

	/* XXX - eventually only initialize client/server fields if shared? */

	/*
	 * Client fields.
	 */
	ip->cl_stage_size = 0;
	ip->cl_allocsz = 0;
	ip->cl_pend_allocsz = 0;
	ip->cl_alloc_unit = ip->mp->mt.fi_minallocsz;
	ip->cl_leases = 0;
	ip->cl_saved_leases = 0;
	ip->cl_short_leases = 0;
	ip->cl_extend_leases = 0;
	ip->cl_hold_blocks = 0;
	ip->cl_flags = 0;
	ip->cl_locks = 0;
	ip->cl_flock = NULL;
	ip->cl_attr_seqno = 0;
	ip->cl_ino_seqno = 0;
	ip->cl_ino_gen = 0;
	ip->cl_srvr_ord = 0;
	for (i = 0; i < MAX_EXPIRING_LEASES; i++) {
		ip->cl_leaseused[i] = 0;
	}
	ip->cl_lease_chain.forw = ip->cl_lease_chain.back = NULL;
	ip->cl_lease_chain.node = ip;

	/*
	 * Server fields.
	 */
	ip->size_owner = 0;
	ip->write_owner = 0;
	ip->sr_write_seqno = 0;
	ip->sr_sync = 0;
	ip->sr_leases = NULL;
	ip->blkd_frlock = NULL;
	ip->rm_pid = 0;
	ip->stage_pid = 0;
	for (i = 0; i < MAX_ARCHIVE; i++) {
		ip->arch_pid[i] = 0;
	}
	ip->arch_count = 0;


	ip->updtime = curr_time;

	ip->stage_thr = 0;
	ip->copy = 0;
	ip->daemon_busy = 0;
	ip->rdev = 0;
	ip->seqno = 0;
	ip->rm_err = 0;
	ip->io_count = 0;
	ip->mm_pages = 0;
	ip->wmm_pages = 0;
	ip->pending_mmappers = 0;
	ip->flush_len = 0;
	ip->space = 0;
	ip->ra_off = 0;
	ip->flush_off = 0;
	ip->koffset = 0;
	ip->page_off = 0;
	ip->real_stage_off = 0;
	ip->real_stage_len = 0;
	ip->lbase = 0;
	ip->rmio_buf = 0;
	ip->klength = 0;
	ip->segment_ip = NULL;
	ip->segment_ord = 0;
	ip->stage_n_count = 0;
	ip->rw_count = 0;
	ip->cnt_writes = 0;
	ip->wr_errno = 0;
	ip->no_opens = 0;
	ip->rm_wait = 0;
	ip->accstm_pnd = 0;
	ip->nfs_ios = 0;
	ip->obj_ios = 0;
	ip->rd_consec = 0;
	ip->wr_consec = 0;
	ip->wr_thresh = 0;
	ip->wr_fini = 0;
	ip->seg_held = 0;
	ip->getino_waiters = 0;
	ip->waiting_getino = FALSE;
	ip->stage_err = 0;
	ip->hash_flags = 0;
	ip->aclp = NULL;
	ip->listiop = NULL;
	ip->flags.bits &= (SAM_BUSY|SAM_FREE|SAM_HASH);
	ip->mt_handle = NULL;
	bzero(&ip->bquota[0], sizeof (ip->bquota));
	ip->stage_n_off = 0;
	ip->stage_off = 0;
	ip->stage_len = 0;
	ip->stage_size = 0;
	ip->maxalloc = 0;
	ip->dir_offset = 0;
	ip->cl_closing = 0;
	ip->last_unmap = 0;
	ip->zero_end = 0;
	sam_clear_map_cache(ip);
	ASSERT(vn_has_cached_data(SAM_ITOV(ip)) == 0);
	SAM_DESTROY_OBJ_LAYOUT(ip);
}


/*
 * ----- sam_find_ino - Get/lock an inode.
 *	Given a file id (i-number/generation number), find it and
 *  return it locked. If segment inode and segment_ip not set,
 *  find parent and lock parent until sam_rele_ino.
 */

int				/* ERRNO if error, 0 if successful--ipp set. */
sam_find_ino(
	struct vfs *vfsp,	/* vfs pointer for this instance of mount. */
	sam_iget_t flag,	/* flag - get new, existing, or stale inode */
	sam_id_t *fp,		/* file i-number & generation number. */
	sam_node_t **ipp)	/* pointer to pointer to inode (returned). */
{
	sam_node_t *ip, *pip;
	int error;

	if ((error = sam_get_ino(vfsp, flag, fp, &ip))) {
		return (error);
	}
	if (S_ISSEGS(&ip->di)) {
		if ((error = sam_get_ino(vfsp, flag,
		    &ip->di.parent_id, &pip))) {
			VN_RELE(SAM_ITOV(ip));
			return (error);
		}
		ip->segment_ip = pip;
		ip->seg_held++;
	}
	*ipp = ip;
	return (0);
}


/*
 * ----- sam_rele_ino - Unlock an inode and if segment unlock parent.
 *	Given a file id (i-number/generation number), unlock it.
 *  If segment parent inode held in sam_find_ino, unlock parent.
 */

void
sam_rele_ino(sam_node_t *ip)
{
	sam_node_t *segment_ip = NULL;

	if (S_ISSEGS(&ip->di) && ip->seg_held) {
		ip->seg_held--;
		segment_ip = ip->segment_ip;
	}
	VN_RELE(SAM_ITOV(ip));
	if (segment_ip) {
		VN_RELE(SAM_ITOV(segment_ip));
	}
}


/*
 * ----- sam_verify_ino - Verify the inode in cache exists or
 *	does not exist based on flag.  Do not verify the generation
 *	number if the inode was found in the directory cache (IG_DNLCDIR)
 *	because it will always be passed in as zero.
 */

static int			/* ERRNO, else 0 if inode pointer returned */
sam_verify_ino(
	sam_id_t *fp,		/* file i-number & generation number. */
	sam_iget_t flag,	/* flag - get new, existing, or stale inode */
	sam_node_t *ip,		/* pointer to inode. */
	boolean_t write_lock)	/* TRUE if inode WRITERS lock held */
{
	if (flag == IG_NEW) {	/* If creating a new inode */
		sam_quota_inode_fini(ip);
		ip->di.id.ino = fp->ino;
		if (++ip->di.id.gen == 0) {
			ip->di.id.gen++;
		}
		sam_clear_incore_inode(ip);
	} else {			/* If looking up an existing inode */
		if ((ip->di.mode == 0) || (ip->di.id.ino != fp->ino) ||
		    ((flag != IG_DNLCDIR) && (ip->di.id.gen != fp->gen)) ||
		    (ip->flags.b.purge_pend)) {
#ifdef METADATA_SERVER
			if (SAM_IS_SHARED_READER(ip->mp)) {
				ASSERT(flag != IG_DNLCDIR);
				(void) sam_refresh_shared_reader_ino(ip,
				    write_lock, CRED());
				if ((ip->di.mode) &&
				    (ip->di.id.ino == fp->ino) &&
				    (ip->di.id.gen == fp->gen)) {
					return (0);
				}
			}
#endif
			if (flag == IG_STALE) {
				if ((ip->di.id.ino == fp->ino) &&
				    (ip->di.id.gen != fp->gen)) {
					return (EBADR);
				}
			}
			return (ENOENT);
		}
	}
	return (0);
}


/*
 * ----- sam_verify_root - Verify the inode in cache exists
 *	and seems to be a root of a sam file system.
 */

int				/* 0 if valid */
sam_verify_root(
	sam_node_t *ip,		/* pointer to inode. */
	sam_mount_t *mp)	/* pointer to mount table. */
{
	int	error = 1;	/* assume some error */
	if (S_ISDIR(ip->di.mode)) {
		if (ip->di.id.ino != SAM_ROOT_INO) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: root inode not SAM_ROOT_INO",
			    mp->mt.fi_name);
		}
		if (ip->di.parent_id.ino != ip->di.id.ino ||
		    ip->di.parent_id.gen != ip->di.id.gen) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: root inode parent not self",
			    mp->mt.fi_name);
		}
		/* let 'em off with a warning */
		error = 0;
	} else {
		cmn_err(CE_WARN, "SAM-QFS: %s: root inode is not a directory",
		    mp->mt.fi_name);
		error = ENOTDIR;
	}
	return (error);
}


/*
 * ----- sam_check_cache - Get inode if it is in cache (hash chain).
 *	Search active ino hash chain for inode in the SAM filesystem.
 *	If found, return ip with vnode held. If not found, return NULL.
 *	NOTE, if vip set, put in hash chain if not found. Otherwise if
 *	found, release vip since the ip has already entered the hash chain.
 */

int				/* ERRNO, else 0 if inode pointer returned */
sam_check_cache(
	sam_id_t *fp,		/* file i-number & generation number. */
	sam_mount_t *mp,	/* pointer to mount table. */
	sam_iget_t flag,	/* flag - get new, existing, or stale inode */
	sam_node_t *vip,	/* pointer to hash lock. */
	sam_node_t **rip)	/* NULL if not found, pointer if found. */
{
	sam_ihead_t *hip;
	sam_node_t *ip;
	vnode_t *vp;
	int ihash;
	kmutex_t *ihp;
	boolean_t write_lock;
	krw_t rw_type;
	int error;

	*rip = NULL;
	if (fp->ino == SAM_INO_INO && mp->mi.m_inodir) {	/* If .inodes */
		VN_HOLD(SAM_ITOV(mp->mi.m_inodir));
		*rip = mp->mi.m_inodir;
		return (0);
	}
	ihash = SAM_IHASH(fp->ino, mp->mi.m_vfsp->vfs_dev);
	ihp = &samgt.ihashlock[ihash];	/* Set hash lock */
	mutex_enter(ihp);

	hip = &samgt.ihashhead[ihash];
	ASSERT(hip != NULL);

#ifdef DEBUG
	{
		/*
		 * Verify that a dev/inode only lives on its chain once.
		 * There is a performance cost to this check, but it should
		 * be small, as the hash buckets are sized to be small.
		 */
		sam_node_t *dip;
		int match_found = 0, match_stale = 0, match_client_dup = 0;

		ip = (sam_node_t *)hip->forw;
		while (ip != (sam_node_t *)(void *)hip) {
			if ((fp->ino == ip->di.id.ino) && (mp == ip->mp)) {
				dip = ip;
				match_found++;
				if ((ip->flags.bits & SAM_STALE) != 0) {
					match_stale++;
				}
				if (fp->gen != ip->di.id.gen) {
					if (SAM_IS_SHARED_CLIENT(mp) &&
					    (flag != IG_NEW)) {
						match_client_dup++;
					}
				}
			}
			/*
			 * Also check each cache entry for an undamaged
			 * in-memory pad area. The hope here is to isolate when
			 * the pad is destroyed.
			 */
			SAM_VERIFY_INCORE_PAD(ip);
			ip = ip->chain.hash.forw;
		}
		if ((match_found - (match_stale + match_client_dup)) > 1) {
			/*
			 * If this is a client, also panic the metadata server.
			 */
			if (SAM_IS_SHARED_FS(dip->mp) &&
			    !SAM_IS_SHARED_SERVER(dip->mp)) {
				(void) sam_proc_block(dip->mp,
				    dip->mp->mi.m_inodir,
				    BLOCK_panic, SHARE_nowait, NULL);
				delay(hz);
			}
			cmn_err(CE_PANIC,
			    "SAM-QFS: %s: sam_check_cache: duplicate "
			    "dev/inum found ip=%p ino.gen=%d.%d "
			    "duplicates found: %d stale dups found: %d "
			    "client dups found: %d",
			    dip->mp->mt.fi_name, (void *)dip, dip->di.id.ino,
			    dip->di.id.gen, match_found, match_stale,
			    match_client_dup);
		}
	}
#endif /* DEBUG */

	ip = (sam_node_t *)hip->forw;
	while (ip != (sam_node_t *)(void *)hip) {
		if ((fp->ino == ip->di.id.ino) && (mp == ip->mp) &&
		    ((ip->flags.bits & SAM_STALE) == 0)) {
			if (fp->gen != ip->di.id.gen) {
				/*
				 * The following check was changed to
				 * accommodate multiple instances in a shared
				 * client hash chain of an inode with differing
				 * gen numbers. It really a workaround, and the
				 * original problem should never occur.
				 */
				if (SAM_IS_SHARED_CLIENT(mp) &&
				    (flag != IG_NEW)) {
					/*
					 * Skip shared client inodes with an old
					 * gen number.  These old inodes need to
					 * stay hashed until the owner puts them
					 * on the free list.
					 */
					ip = ip->chain.hash.forw;
					continue;
				}
			}

			vp = SAM_ITOV(ip);
			if (error = sam_hold_vnode(ip, flag)) {
				if (vip != NULL) {
					/* Release newly acquired ino */
					sam_release_ino(vip);
				}
				mutex_exit(ihp);
				return (error);
			}

			if (flag == IG_NEW) {	/* If creating a new inode */
				rw_type = RW_WRITER;
				write_lock = TRUE;
			} else {
				rw_type = RW_READER;
				write_lock = FALSE;
			}
			if (rw_tryenter(&ip->inode_rwl, rw_type) == 0) {
				/*
				 * Could not get inode lock, block until lock
				 * gotten.
				 */
				mutex_exit(ihp);
				TRACE(T_SAM_CACHE_INO1, SAM_ITOV(ip),
				    ip->di.id.ino,
				    (SAM_ITOV(ip))->v_count, ip->flags.bits);
				RW_LOCK_OS(&ip->inode_rwl, rw_type);
				mutex_enter(ihp);
			}
			/*
			 * Under lock, verify inode still exists for this mount,
			 * the inode is still in the hash chain, and it's not
			 * stale.
			 */
			if ((fp->ino == ip->di.id.ino) && (mp == ip->mp) &&
			    ((ip->flags.bits & (SAM_HASH|SAM_STALE)) ==
			    SAM_HASH)) {
				if (vip != NULL) {
					/* Release newly acquired ino */
					sam_release_ino(vip);
				}
				mutex_exit(ihp);
				if ((error = sam_verify_ino(fp, flag,
				    ip, write_lock))) {
					RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
					if (error == EBADR) {
						*rip = ip;
					} else {
						VN_RELE(vp);
					}
					return (error);
				}
				/* If currently inactive */
				if (ip->flags.b.free) {
					if (!write_lock) {
						if (rw_tryupgrade(
						    &ip->inode_rwl) == 0) {
							/*
							 * Could not get writers
							 * lock, block until
							 * gotten.  Verify inode
							 * again since lock has
							 * been lifted.
							 */
							RW_UNLOCK_OS(
							    &ip->inode_rwl,
							    rw_type);
							TRACE(T_SAM_CACHE_INO2,
							    SAM_ITOV(ip),
							    ip->di.id.ino,
							    (SAM_ITOV(
							    ip))->v_count,
							    ip->flags.bits);
							rw_type = RW_WRITER;
							RW_LOCK_OS(
							    &ip->inode_rwl,
							    rw_type);
							if ((error =
							    sam_verify_ino(fp,
							    flag, ip, TRUE))) {

					RW_UNLOCK_OS(
					    &ip->inode_rwl,
					    rw_type);
								if (error ==
								    EBADR) {
									*rip =
									    ip;
								} else {
									VN_RELE(
									    vp);
								}
								return (error);
							}
						}
						write_lock = TRUE;
					}
					mutex_enter(&samgt.ifreelock);
					sam_remove_freelist(ip);
					mutex_exit(&samgt.ifreelock);
				}
				if (ip->flags.b.busy == 0) {
					if (!write_lock) {
						mutex_enter(&ip->fl_mutex);
					}
					/* Inode now active (SAM_BUSY) */
					ip->flags.b.busy = 1;
					if (!write_lock) {
						mutex_exit(&ip->fl_mutex);
					}
				}
				RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
				*rip = ip;
				return (0);

			} else {
				/*
				 * Inode changed, release it, and start at top
				 * of hash.
				 */
				if ((ip->flags.bits &
				    (SAM_HASH|SAM_STALE|SAM_FREE)) ==
				    (SAM_HASH|SAM_STALE|SAM_FREE)) {
					/*
					 * If it's stale and still hashed
					 * unhash it if it's on the freelist
					 */
					SAM_DESTROY_OBJ_LAYOUT(ip);
					SAM_UNHASH_INO(ip);
				}
				mutex_exit(ihp);
				RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
				VN_RELE(vp);
				mutex_enter(ihp);
				ip = hip->forw;
				continue;
			}
		}
		ip = ip->chain.hash.forw;
	}
	if (vip != NULL) {
		/* Put inode on head of the inode hashlist. */
		vip->di.id.ino = fp->ino; /* Set id to avoid cache conflicts. */
		vip->di.id.gen = fp->gen;
		SAM_HASH_INO(hip, vip);
	}
	mutex_exit(ihp);
	return (0);
}


/*
 * ----- sam_release_ino - Release inode.
 *	Release newly acquired inode. inode_rwl WRITER lock is set.
 *	If inode is not busy, put it on the freelist. Release WRITER lock.
 */

static void
sam_release_ino(sam_node_t *ip)
{
	vnode_t *vp;

	ASSERT(RW_WRITE_HELD(&ip->inode_rwl));

	vp = SAM_ITOV(ip);
	mutex_enter(&vp->v_lock);
	if (--vp->v_count >= 1) {
		mutex_exit(&vp->v_lock);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		return;
	}
	mutex_exit(&vp->v_lock);
	if (ip->flags.b.free) {
		cmn_err(SAMFS_DEBUG_PANIC,
		    "SAM-QFS: %s: sam_release_ino: releasing a free inode"
		    " ip=%x, ino=%d, bits=%x",
		    ip->mp->mt.fi_name, (uint_t)ip,
		    ip->di.id.ino, ip->flags.bits);
	}
	if (vn_has_cached_data(vp) == 0) {
		sam_put_freelist_head(ip);
	} else {
		sam_put_freelist_tail(ip);
	}
	ip->flags.b.busy = 0;		/* Clear SAM_BUSY; Put on free list */
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
}


/*
 * ----- sam_put_freelist_head - Put inode on top of the freelist.
 *	Put inode on head of the inode freelist.
 */

void
sam_put_freelist_head(sam_node_t *ip)
{
	mutex_enter(&samgt.ifreelock);
	samgt.ifreehead.free.forw->chain.free.back = ip;
	ip->chain.free.back = (sam_node_t *)(void *)&samgt.ifreehead;
	ip->chain.free.forw = samgt.ifreehead.free.forw;
	samgt.ifreehead.free.forw = ip;
	ip->flags.b.free = 1;
	samgt.inofree++;
	SAM_VERIFY_INCORE_PAD(ip);
	mutex_exit(&samgt.ifreelock);

}

/*
 * ----- sam_put_freelist_tail - Put inode at the bottom of the freelist.
 *	Put inode at the bottom of the inode freelist.
 */

void
sam_put_freelist_tail(sam_node_t *ip)
{
	mutex_enter(&samgt.ifreelock);
	samgt.ifreehead.free.back->chain.free.forw = ip;
	ip->chain.free.forw = (sam_node_t *)(void *)&samgt.ifreehead;
	ip->chain.free.back = samgt.ifreehead.free.back;
	samgt.ifreehead.free.back = ip;
	ip->flags.b.free = 1;
	samgt.inofree++;
	SAM_VERIFY_INCORE_PAD(ip);
	mutex_exit(&samgt.ifreelock);

}


/*
 * ----- sam_remove_freelist - Remove inode from the freelist.
 *	Remove inode from the inode freelist.
 *	NOTE. samgt.ifreelock is set on entry.
 */

void
sam_remove_freelist(sam_node_t *ip)
{
	ip->chain.free.back->chain.free.forw = ip->chain.free.forw;
	ip->chain.free.forw->chain.free.back = ip->chain.free.back;
	ip->chain.free.forw = ip;
	ip->chain.free.back = ip;
	ip->flags.b.free = 0;
	if (--samgt.inofree < 0) {
		samgt.inofree = 0;
	}
}


/*
 * ----- sam_read_disk_ino - Read inode and return buffer pointer.
 * If shared client, try for 1 second to get inode. If server is not
 * responding, return with ETIME errno. For main path, just call sam_read_ino.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_read_disk_ino(
	sam_mount_t *mp,	/* pointer to the mount table. */
	sam_id_t *fp,		/* file i-number & generation number. */
	buf_t **ibp,		/* .inodes file buffer (returned). */
	struct sam_perm_inode **permip) /* inode data (returned). */
{
	buf_t *bp;
	int error;

	if (SAM_IS_SHARED_CLIENT(mp)) {
		if ((error = sam_get_client_inode(mp, fp, SHARE_wait_one,
				sizeof (struct sam_perm_inode), &bp)) == 0) {
			*ibp = bp;
			*permip =
			    (struct sam_perm_inode *)(void *)bp->b_un.b_addr;

			/*
			 * Stale inode buffer for shared client.
			 */
			bp->b_flags |= B_STALE | B_AGE;
		}
		if (SAM_IS_SHARED_CLIENT(mp)) {
			return (error);
		}
		if (!error) {
			TRACE(T_SAM_FAILOVER_WIN, NULL, 1, fp->ino, 0);
			brelse(bp);
		}
	}
	return (sam_read_ino(mp, fp->ino, ibp, permip));
}


/*
 * ----- sam_freeino - Delete/Get an inode from the freelist.
 *	If the freelist is not empty, return the head freelist inode for
 *	reuse if it has no pages or if the number of allocated inodes is
 *	over the limit.
 */

static sam_node_t *
sam_freeino(void)
{
	sam_node_t *ip;
	sam_node_t *nip;
	vnode_t *vp;
	kmutex_t *ihp;
	int count;
	int error;


start:
	mutex_enter(&samgt.ifreelock);
	count = samgt.inofree;		/* Starting free count */
	ip = (sam_node_t *)samgt.ifreehead.free.forw;
	while ((ip != (sam_node_t *)(void *)&samgt.ifreehead) &&
	    (--count >= 0)) {
		vp = SAM_ITOV(ip);
		nip = ip->chain.free.forw;
		if (rw_tryenter(&ip->inode_rwl, RW_WRITER) == 0) {
			ip = nip;
			continue;
		}
		VN_HOLD(vp);	/* Make inode busy */
		ip->flags.b.busy = 1;	/* Inode is active (SAM_BUSY) */
		if (ip->flags.b.free == 0) {
			cmn_err(SAMFS_DEBUG_PANIC,
			    "SAM-QFS: %s: sam_freeino: inode is not free"
			    " ip=%x, ino=%d.%d, bits=%x",
			    ip->mp->mt.fi_name, (uint_t)ip,
			    ip->di.id.ino, ip->di.id.gen,
			    ip->flags.bits);
		}
		sam_remove_freelist(ip);
		mutex_exit(&samgt.ifreelock);

		/*
		 * If regular offline stage_n file, release stage_n blocks.
		 */
		if (ip->di.status.b.direct) {
			sam_free_stage_n_blocks(ip);
		}

		/*
		 * Flush and invalidate pages associated with this free inode.
		 */
		error = sam_flush_pages(ip, B_INVAL);

		ihp = &samgt.ihashlock[SAM_IHASH(ip->di.id.ino, ip->dev)];
		mutex_enter(ihp);

		mutex_enter(&vp->v_lock);
		mutex_enter(&ip->write_mutex);
		if (vn_has_cached_data(vp) || (vp->v_count > 1) ||
		    ip->cnt_writes ||
		    ip->wr_fini || ip->wr_thresh) {
			int no_rele;

			/*
			 * We're here because either the inode is 'dented' with
			 * pages still attached (likely from an earlier error,
			 * or because somebody is in the process of reclaiming
			 * it (vcount > 1)
			 */
			mutex_exit(&ip->write_mutex);
			no_rele = error && (samgt.inocount > samgt.ninodes);
			if (no_rele) {
				/*
				 * Want to keep this out of the
				 * VN_RELE()/VOP_INACTIVE() path since we're
				 * relatively certain that we've got a failing
				 * device nearby and there could be a slew of
				 * other similar inodes on the free list and to
				 * try and recurse through them until hitting a
				 * good one might blow our stack.
				 *
				 * We've now
				 * removed this guy from the freelist and he's
				 * still on the hash list.  Do a logical VN_RELE
				 * to release our hold and move on.
				 */
				vp->v_count--;
			}
			mutex_exit(&vp->v_lock);
			mutex_exit(ihp);
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			if (!no_rele) {
				VN_RELE(vp);
			}
			goto start;
		}
		mutex_exit(&ip->write_mutex);
		mutex_exit(&vp->v_lock);

		/*
		 * Remove from old hash, will probably be put in different hash
		 * chain
		 */
		if (ip->flags.b.hash) {
			SAM_DESTROY_OBJ_LAYOUT(ip);
			SAM_UNHASH_INO(ip);
		}
		mutex_exit(ihp);
		return (ip);
	}
	mutex_exit(&samgt.ifreelock);
	return (NULL);
}


/*
 * ----- sam_unhash_ino - unhash inode from incore inode cache.
 */

void
sam_unhash_ino(sam_node_t *ip)
{
	kmutex_t *ihp;

	ihp = &samgt.ihashlock[SAM_IHASH(ip->di.id.ino, ip->dev)];
	mutex_enter(ihp);
	if (ip->flags.b.hash) {
		SAM_DESTROY_OBJ_LAYOUT(ip);
		SAM_UNHASH_INO(ip);
	}
	mutex_exit(ihp);
}


/*
 * ----- sam_reset_default_options - reset to default mount options.
 * Reset directio to the setfa -D or mount forcedirectio setting.
 * Reset qwrite to the qwrite mount setting.
 * NOTE: inode_rwl WRITERS lock set on entry and exit.
 */

void
sam_reset_default_options(sam_node_t *ip)
{
	if (ip->di.id.ino == SAM_INO_INO) {
		return;
	}
	if (S_ISREG(ip->di.mode)) {
		int directio_flag;

		directio_flag =
		    (ip->di.status.b.directio ||
		    (ip->mp->mt.fi_config & MT_DIRECTIO)) ?
		    DIRECTIO_ON : DIRECTIO_OFF;
		sam_set_directio(ip, directio_flag);
		ip->flags.b.qwrite = (ip->mp->mt.fi_config & MT_QWRITE) ?
		    1 : 0;
	}
}


/*
 * ----- sam_set_directio - set directio
 *
 * NOTE: inode_rwl WRITERS lock set on entry and exit.
 */

void				/* ERRNO, else 0 if successful. */
sam_set_directio(
	sam_node_t *ip,		/* Pointer to inode. */
	int directio_flag)	/* DIRECTIO_ON or DIRECTIO_OFF */
{
	ASSERT(RW_WRITE_HELD(&ip->inode_rwl));

	if (S_ISREG(ip->di.mode)) {
		if (ip->flags.b.directio != directio_flag) {
			ip->flags.b.directio = directio_flag;
			if (directio_flag == DIRECTIO_ON) {
				/*
				 * Flush and invalidate pages to set directio.
				 * Cannot flush pages now if memory mapped.
				 */
				if (!ip->mm_pages) {
					sam_flush_pages(ip, B_INVAL);
				}
			}
		}
	}
}


/*
 * Set ABR on this file.  This sets a flag that passes the B_ABRWRITE
 * b_flag to the underlying device driver, which disables DRL (dirty
 * region logging), speeding mirror writes for those apps that can
 * manage mirror consistency themselves).  E.g., Oracle.
 *
 * This request comes from the MDC by way of a server directed action.
 */
void			/* ERRNO if an error occured, 0 if successful. */
sam_set_abr(sam_node_t *ip)
{
	ASSERT(RW_WRITE_HELD(&ip->inode_rwl) ||
	    (RW_READ_HELD(&ip->inode_rwl) && MUTEX_HELD(&ip->fl_mutex)));

	TRACE(T_SAM_ABR_SET, SAM_ITOP(ip), ip->di.id.ino,
	    0, (int)ip->flags.b.abr);
	ip->flags.b.abr = 1;
}


/*
 * ----- sam_hold_vnode -
 * Increment existing vnode count and hold if it is not purging.
 * If this is a new inode, it may be still syncing to disk. It
 * is OK to reassign it.
 */

int				/* ERRNO if vnode purging, 0 if successful. */
sam_hold_vnode(
	sam_node_t *ip,		/* Pointer to inode. */
	sam_iget_t flag)	/* flag - get new, existing, or stale inode */
{
	vnode_t *vp = SAM_ITOV(ip);
	int error = 0;

	mutex_enter(&(vp)->v_lock);
	if ((flag != IG_NEW) && (ip->flags.b.purge_pend)) {
		error = ENOENT;
	} else {
		(vp)->v_count++;
	}
	mutex_exit(&(vp)->v_lock);
	return (error);
}


/*
 * ----- sam_set_size -
 * Set the real size based on offline/online. Set partial if
 * partial extents exist.
 */

void
sam_set_size(sam_node_t *ip)
{
	if (ip->di.status.b.offline == 0) {
		ip->size = ip->di.rm.size;
		if (S_ISSEGI(&ip->di)) {
			ip->size = ip->di.rm.info.dk.seg.fsize;
		}
	} else if (!ip->flags.b.staging) {
		ip->size = 0;
		if (ip->di.status.b.bof_online) {
			if (ip->di.status.b.pextents) {	/* Partial online */
				ip->size = sam_partial_size(ip);
			}
		}
	}
}


/*
 * ----- sam_destroy_ino - deletes an inode from the SAM-QFS inode cache.
 * Removes from the free list, if present.
 * Decrements the inode count.
 */

void
sam_destroy_ino(sam_node_t *ip, boolean_t locked)
{
	sam_mount_t *mp = ip->mp;

	ASSERT((ip->sr_leases == NULL) && (ip->cl_lease_chain.forw == NULL));

	if (ip->flags.b.free) {
		if (locked == 0) {
			mutex_enter(&samgt.ifreelock);
		}
		sam_remove_freelist(ip);
		if (locked == 0) {
			mutex_exit(&samgt.ifreelock);
		}
	}
	if (--samgt.inocount < 0) {
		samgt.inocount = 0;
	}

	if (mp->mt.fi_type == DT_META_OBJ_TGT_SET) {
		sam_objnode_free(ip);
	}

	kmem_cache_free(samgt.inode_cache, (void *)ip);

	SAM_VFS_RELE(mp);
}


/*
 * ----- sam_create_ino_cache - creates the SAM-QFS inode cache.
 * Called from sam_init_ino in init.c
 */

void
sam_create_ino_cache(void)
{
	samgt.inode_cache = kmem_cache_create("sam_inode_cache",
	    sizeof (sam_node_t), 0,
	    sam_inode_constructor,
	    sam_inode_destructor,
	    NULL /* sam_inode_reclaim */,
	    NULL, NULL, 0);
}

/*
 * ----- sam_delete_ino_cache - deletes the SAM-QFS inode cache.
 * Destroys all free inodes and deletes the sam inode cache.
 * Called from sam_clean_ino in init.c
 */

void
sam_delete_ino_cache(void)
{
	sam_node_t *ip;
	sam_node_t *nip;

	mutex_enter(&samgt.ifreelock);
	ip = (sam_node_t *)samgt.ifreehead.free.forw;
	while (ip != (sam_node_t *)(void *)&samgt.ifreehead) {
		nip = ip->chain.free.forw;
		ip->flags.bits = 0;		/* ip->flags.b.free = 0 */
		if (--samgt.inofree < 0) {
			samgt.inofree = 0;
		}
#ifndef _NoOSD_
		sam_osd_destroy_obj_layout(ip, 0);
#endif
		sam_destroy_ino(ip, TRUE);
		ip = nip;
	}
	mutex_exit(&samgt.ifreelock);
	kmem_cache_destroy(samgt.inode_cache);
	samgt.inode_cache = NULL;
}

/*
 * ----- sam_inode_constructor - initializes a new SAM-QFS inode.
 * Only called from kmem_cache_alloc().
 */

/* ARGSUSED1 */
static int
sam_inode_constructor(
	void *buf,			/* inode to be constructed */
	void *not_used,
	int notused)
{
	sam_node_t *ip = (sam_node_t *)buf;
	vnode_t *vp;

	bzero(ip, sizeof (sam_node_t));
	rw_init(&ip->inode_rwl, NULL, RW_DEFAULT, NULL);
	ip->flags.b.busy = 1;	/* Inode is active (SAM_BUSY) */
	rw_init(&ip->data_rwl, NULL, RW_DEFAULT, NULL);
	sam_mutex_init(&ip->daemon_mutex, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&ip->daemon_cv, NULL, CV_DEFAULT, NULL);
	sam_mutex_init(&ip->rm_mutex, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&ip->rm_cv, NULL, CV_DEFAULT, NULL);
	sam_mutex_init(&ip->write_mutex, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&ip->write_cv, NULL, CV_DEFAULT, NULL);
	sam_mutex_init(&ip->listio_mutex, NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&ip->fl_mutex, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&ip->ilease_cv, NULL, CV_DEFAULT, NULL);
	sam_mutex_init(&ip->iom.map_mutex, NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&ip->ilease_mutex, NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&ip->blkd_frlock_mutex, NULL, MUTEX_DEFAULT, NULL);
	dnlc_dir_init(&ip->i_danchor);
	sam_mutex_init(&ip->i_indelmap_mutex, NULL, MUTEX_DEFAULT, NULL);

	/*
	 * Vnode fields. Solaris 10 introduced the concept of an opaque
	 * vnode. That is, some of its members are no longer directly
	 * visible to consumers of the data structure. FS inodes can
	 * no longer embed a vnode within it's inode structure as we did
	 * in previous versions. So, we must use the appropriate interfaces
	 * for allocating/freeing vnodes and accessing the private
	 * structure members that we are interested in.
	 */

	vp = vn_alloc(KM_SLEEP);
	vp->v_data = (caddr_t)ip;
	ip->vnode = vp;

#ifdef METADATA_SERVER
	vn_setops(vp, samfs_vnodeopsp);
#else
	vn_setops(vp, samfs_client_vnodeopsp);
#endif

	/*
	 * Client fields.
	 */
	sam_mutex_init(&ip->cl_apmutex, NULL, MUTEX_DEFAULT, NULL);

	/*
	 * Initialize the pads now. I'll use the
	 * same pattern as the kernel does for
	 * redzone given that we're using them for
	 * a similar purpose.
	 */

	ip->pad0 = PAD0_CONTENTS;
	ip->pad1 = PAD1_CONTENTS;

	return (0);
}

/*
 * ----- sam_inode_destructor - clears a deleted SAM-QFS inode.
 * Only called from kmem_cache_free().
 */

/* ARGSUSED1 */
static void
sam_inode_destructor(void *buf, void *not_used)
{
	sam_node_t *ip = (sam_node_t *)buf;
	vnode_t *vp;

	rw_destroy(&ip->inode_rwl);
	rw_destroy(&ip->data_rwl);
	mutex_destroy(&ip->daemon_mutex);
	cv_destroy(&ip->daemon_cv);
	mutex_destroy(&ip->rm_mutex);
	cv_destroy(&ip->rm_cv);
	mutex_destroy(&ip->write_mutex);
	cv_destroy(&ip->write_cv);
	mutex_destroy(&ip->listio_mutex);
	mutex_destroy(&ip->fl_mutex);
	cv_destroy(&ip->ilease_cv);
	mutex_destroy(&ip->iom.map_mutex);
	mutex_destroy(&ip->ilease_mutex);
	mutex_destroy(&ip->blkd_frlock_mutex);
	mutex_destroy(&ip->i_indelmap_mutex);

	dnlc_dir_fini(&ip->i_danchor);

	vp = SAM_ITOV(ip);
	vn_free(vp);

	mutex_destroy(&ip->cl_apmutex);
}


/*
 * ----- sam_wait_space - wait until disk space is available.
 * No space (ENOSPC), if not a shared client, or not waiting on reclaim thread,
 *    issue "File system full" message,
 * if file is on QFS meta device, return ENOSPC and issue META ENOSPC message.
 * if shared QFS (no SAM) or standalone FS data device, return ENOSPC
 * if NFS and not standalone, return EINPROGRESS
 * otherwise hold thread until space is available.
 */

int				/* ERRNO if error occured, 0 if successful. */
sam_wait_space(
	sam_node_t *ip,		/* The inode table entry pointer. */
	int error)		/* SAM ENOSPC error */
{
	char *err_str = "waiting";
	int	ret_error = 0;
	enum syslog_space_actions action = E_WAITING;
	clock_t last_alloc_time;
	boolean_t block_ran;
	sam_mount_t *mp;	/* The mount table pointer. */

	mp = ip->mp;
	mutex_enter(&mp->mi.m_block.mutex);
	last_alloc_time = mp->mi.m_blkth_alloc;
	mutex_exit(&mp->mi.m_block.mutex);

	if (ip->di.status.b.meta || IS_SAM_META_ENOSPC(error)) {
		/* if meta inode, META ENOSPC */
		ret_error = ENOSPC;
		err_str = "META ENOSPC";
		action = E_NOSPC;

	} else if ((SAM_IS_SHARED_CLIENT(mp) && !(SAM_IS_SHARED_SAM_FS(mp))) ||
	    (!(SAM_IS_SHARED_CLIENT(mp)) && SAM_IS_STANDALONE(mp))) {
		ret_error = ENOSPC;
		err_str = "ENOSPC";
		action = E_NOSPC;

	} else if (SAM_THREAD_IS_NFS()) { /* If nfs access and not standalone */
		/*
		 * Set T_WOULDBLOCK so server does not send a reply to the
		 * client.  The client will timeout and resend the request.
		 */
		curthread->t_flag |= T_WOULDBLOCK;
		ret_error = EINPROGRESS;
		err_str = "EINPROGRESS";
		action = E_INPROGRESS;
	}

	/*
	 * Issue "full" message at most once every 5 minutes.
	 * No message issued if MDS is waiting on the reclaim thread.
	 */
	error = ret_error;
	TRACE(T_SAM_WAIT_FSPACE1, mp->mi.m_vfsp, (sam_tr_t)mp, error,
	    mp->mi.m_fsfullmsg);
	mp->mi.m_fsfull = ddi_get_lbolt();
	if (mp->mi.m_next == NULL) {
		if ((ddi_get_lbolt() - mp->mi.m_fsfullmsg) >
		    (5 * 60 * drv_usectohz(1000000))) {
			mp->mi.m_fsfullmsg = mp->mi.m_fsfull;
			cmn_err(CE_NOTE,
			"SAM-QFS: %s: sam_wait_space: File system full - %s",
			    mp->mt.fi_name, err_str);
			sam_syslog(SAM_VTOI(mp->mi.m_vn_root),
			    E_NOSPACE, 0, (int)action);
		}
	}

	if (error == 0) {
		mutex_enter(&mp->mi.m_block.mutex);
		block_ran = (last_alloc_time < mp->mi.m_blkth_alloc);
		mutex_exit(&mp->mi.m_block.mutex);
		if (!block_ran) {
			int ret = 0;

			/*
			 * If the block thread has not allocated since we
			 * started, wait until the block thread has run.
			 */
			mutex_enter(&mp->ms.m_waitwr_mutex);
			mp->mi.m_wait_write++;
			if (sam_mask_signals) {
				ret = sam_cv_wait_sig(&mp->ms.m_waitwr_cv,
				    &mp->ms.m_waitwr_mutex);
			} else {
				ret = cv_wait_sig(&mp->ms.m_waitwr_cv,
				    &mp->ms.m_waitwr_mutex);
			}
			if (ret == 0) {
				error = EINTR;
			}
			mp->mi.m_wait_write--;
			if (mp->mt.fi_status & FS_FAILOVER) {
				mutex_exit(&mp->ms.m_waitwr_mutex);
				error = sam_stop_inode(ip);
			} else {
				mutex_exit(&mp->ms.m_waitwr_mutex);
			}
		}
	}
	TRACE(T_SAM_WAIT_FSPACE2, mp->mi.m_vfsp, (sam_tr_t)mp, error,
	    mp->mi.m_fsfullmsg);
	return (error);
}


/*
 * ----- sam_enospc - flag an ENOSPC error
 *	sam_enospc returns an ENOSPC error, unless the inode is
 *	a metadata inode, and the file system is a QFS file system.
 *	If metadata inode and QFS file system, return a "special"
 *	ENOSPC, which won't wait for space.
 */

int
sam_enospc(sam_node_t *ip)
{
	if ((ip->di.status.b.meta != 0) && (ip->mp->mt.mm_count > 0)) {
		return (ENOSPC + SAM_META_ENOSPC_BIAS);
	} else {
		return (ENOSPC);
	}
}


/*
 * ----- sam_get_symlink - Retrieve a SAM-QFS symbolic link name.
 * Return the symbolic link name from inode extension(s).
 *
 * Caller must have the inode read lock held.  This lock is not released.
 */

int				/* ERRNO if error, 0 if successful */
sam_get_symlink(
	sam_node_t *bip,	/* Pointer to base inode */
	struct uio *uiop)	/* User I/O vector array. */
{
	int error = 0;
	buf_t *bp;
	sam_id_t eid;
	struct sam_inode_ext *eip;
	int ino_chars;
	int n_chars;

	ASSERT(RW_READ_HELD(&bip->inode_rwl));

	/*
	 * At 4.2, the symlink is stored in the direct extents if <= 96 chars.
	 */
	n_chars = bip->di.psize.symlink;
	if (n_chars <= SAM_SYMLINK_SIZE) {
		if (bip->di.extent[0] || n_chars == 0) {
			if ((n_chars > 0) && bip->di.extent[0]) {
				error = uiomove((caddr_t)&bip->di.extent[0],
				    n_chars, UIO_READ,
				    uiop);
			}
			TRACE(T_SAM_GET_SLN_EXT, SAM_ITOV(bip),
			    bip->di.id.ino, n_chars,
			    error);
			return (error);
		}
	}

	n_chars = 0;
	eid = bip->di.ext_id;
	while (eid.ino) {
		if (error = sam_read_ino(bip->mp, eid.ino, &bp,
				(struct sam_perm_inode **)&eip)) {
			break;
		}
		if (EXT_HDR_ERR(eip, eid, bip)) {
			sam_req_ifsck(bip->mp, -1,
			    "sam_get_symlink: EXT_HDR", &bip->di.id);
			error = ENOCSI;
			brelse(bp);
			break;
		}
		if (S_ISSLN(eip->hdr.mode)) {
			ino_chars = eip->ext.sln.n_chars;
			if ((ino_chars == 0) ||
			    (ino_chars > MAX_SLN_CHARS_IN_INO) ||
			    ((n_chars + ino_chars) > bip->di.psize.symlink)) {
				sam_req_ifsck(bip->mp, -1,
				    "sam_get_symlink: bad size", &bip->di.id);
				error = ENOCSI;
				brelse(bp);
				break;
			}
			if (error = uiomove((caddr_t)&eip->ext.sln.chars[0],
			    ino_chars, UIO_READ, uiop)) {
				brelse(bp);
				break;
			}
			n_chars += ino_chars;
		}
		eid = eip->hdr.next_id;
		brelse(bp);
	}
	if (error == 0) {
		if (n_chars != bip->di.psize.symlink) {
			sam_req_ifsck(bip->mp, -1,
			    "sam_get_symlink: wrong size", &bip->di.id);
			error = ENOCSI;
		}
	}
	TRACE(T_SAM_GET_SLN_EXT, SAM_ITOV(bip), bip->di.id.ino, n_chars, error);
	return (error);
}


/*
 * ----- sam_get_special_file - Retrieve a SAM-QFS special file.
 */

int				/* ERRNO if error, 0 if successful */
sam_get_special_file(
	sam_node_t *ip,		/* Pointer to inode */
	vnode_t **vpp,		/* Returned special vnode */
	cred_t *credp)
{
	int error = 0;
	vnode_t *vp = SAM_ITOV(ip);

	/*
	 * RM_CHAR_DEV_FILE means treat file like a char device for aio.
	 * Attach the samaio character device.
	 * Set vnode char type and rdev for aio pseudo device.
	 * The specfs will call VOP_GETATTR() for AT_SIZE so lock can be
	 * safely held.
	 */
	*vpp = NULL;
	if ((ip->di.rm.ui.flags & RM_CHAR_DEV_FILE) && (vp->v_type != VCHR) &&
	    (samgt.samaio_vp != NULL)) {
		struct samaio_ioctl aio;
		int minor;

		aio.vp = vp;
		aio.fs_eq = ip->mp->mt.fi_eq;
		aio.ino = ip->di.id.ino;
		aio.gen = ip->di.id.gen;
		error = cdev_ioctl(samgt.samaio_vp->v_rdev,
		    SAMAIO_ATTACH_DEVICE,
		    (intptr_t)&aio, ip->di.mode, CRED(), &minor);
		if (error == 0) {
			vp->v_rdev = makedevice(getmajor(
			    samgt.samaio_vp->v_rdev), minor);
			vp->v_type = VCHR;
		} else {
			vp->v_type = VREG;
			vp->v_rdev = 0;
			VN_RELE(vp);
			return (error);
		}
	}
	if (SAM_ISVDEV(vp->v_type)) {	/* if a special */
		struct vnode *sp_vp;

		sp_vp = specvp(vp, vp->v_rdev, vp->v_type, credp);
		if (sp_vp == NULL) {
			error = ENOSYS;
		}
		VN_RELE(vp);
		if (error == 0) {
			*vpp = sp_vp;
			TRACE(T_SAM_LOOKUP_SP, vp,
			    (sam_tr_t)*vpp, ip->di.id.ino,
			    sp_vp->v_rdev);
		}
	}
	return (error);
}

/*
 * ----- sam_ilock_two - Grab two inode_rwl locks while avoiding deadlock
 *	by using inode numbers to impose ordering.  If the inodes are the same,
 *  we only get the lock once.
 */
void
sam_ilock_two(sam_node_t *ip1, sam_node_t *ip2, krw_t rw)
{
	if (ip1 == ip2) {
		RW_LOCK_OS(&ip1->inode_rwl, rw);
	} else {
		if (ip1->di.id.ino < ip2->di.id.ino) {
			RW_LOCK_OS(&ip1->inode_rwl, rw);
			RW_LOCK_OS(&ip2->inode_rwl, rw);
		} else {
			RW_LOCK_OS(&ip2->inode_rwl, rw);
			RW_LOCK_OS(&ip1->inode_rwl, rw);
		}
	}
}

/*
 * ----- sam_iunlock_two - Release two inode_rwl locks in the reverse order
 *	from which they were obtained.  If the inodes are the same, only release
 *  the lock once.
 */
/* ARGSUSED2 */
void
sam_iunlock_two(sam_node_t *ip1, sam_node_t *ip2, krw_t rw)
{
	if (ip1 == ip2) {
		RW_UNLOCK_OS(&ip1->inode_rwl, rw);
	} else {
		if (ip1->di.id.ino < ip2->di.id.ino) {
			RW_UNLOCK_OS(&ip2->inode_rwl, rw);
			RW_UNLOCK_OS(&ip1->inode_rwl, rw);
		} else {
			RW_UNLOCK_OS(&ip1->inode_rwl, rw);
			RW_UNLOCK_OS(&ip2->inode_rwl, rw);
		}
	}
}

/*
 * ----- sam_dlock_two - Grab two data_rwl locks while avoiding deadlock
 *	by using inode numbers to impose ordering.  If the inodes are the same,
 *  we only get the lock once.
 */
void
sam_dlock_two(sam_node_t *ip1, sam_node_t *ip2, krw_t rw)
{
	if (ip1 == ip2) {
		sam_rwdlock_ino(ip1, rw, 0);
	} else {
		if (ip1->di.id.ino < ip2->di.id.ino) {
			sam_rwdlock_ino(ip1, rw, 0);
			sam_rwdlock_ino(ip2, rw, 0);
		} else {
			sam_rwdlock_ino(ip2, rw, 0);
			sam_rwdlock_ino(ip1, rw, 0);
		}
	}
}

/*
 * ----- sam_dunlock_two - Release two data_rwl locks in the reverse order
 *	from which they were obtained.  If the inodes are the same, only release
 *  the lock once.
 */
/* ARGSUSED2 */
void
sam_dunlock_two(sam_node_t *ip1, sam_node_t *ip2, krw_t rw)
{
	if (ip1 == ip2) {
		RW_UNLOCK_OS(&ip1->data_rwl, rw);
	} else {
		if (ip1->di.id.ino < ip2->di.id.ino) {
			RW_UNLOCK_OS(&ip2->data_rwl, rw);
			RW_UNLOCK_OS(&ip1->data_rwl, rw);
		} else {
			RW_UNLOCK_OS(&ip1->data_rwl, rw);
			RW_UNLOCK_OS(&ip2->data_rwl, rw);
		}
	}
}

#ifdef	DEBUG

/*
 * ----- sam_itov - convert inode ptr to vnode ptr.
 * Used in Debug only to verify the in-memory inode pad areas.
 */
vnode_t *
sam_itov(sam_node_t *ip)
{
	SAM_VERIFY_INCORE_PAD(ip);
	return (Sam_ITOV(ip));
}

/*
 * ----- sam_vtoi - convert vnode ptr to inode ptr.
 * Used in Debug only to verify the in-memory inode pad areas.
 */
sam_node_t *
sam_vtoi(vnode_t *vp)
{
	sam_node_t *ip;

	ip = Sam_VTOI(vp);
	SAM_VERIFY_INCORE_PAD(ip);
	return (ip);
}
#endif	/* DEBUG */
