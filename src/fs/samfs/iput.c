/*
 * ----- sam/iput.c - Process the sam inactivate functions.
 *
 * Processes the inode inactivate functions supported on the SAM File
 * System. Called by sam_vnode functions.
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
#pragma ident "$Revision: 1.141 $"
#endif

#include "sam/osversion.h"

/* ----- UNIX Includes */

#ifdef sun
#include <sys/types.h>
#include <sys/types32.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/ksynch.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/lockfs.h>
#include <sys/sunddi.h>
#include <vm/pvn.h>
#include <nfs/nfs.h>
#include <sys/policy.h>
#endif /* sun */

#ifdef linux
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
#include <linux/net.h>
#include <linux/socket.h>
#include <linux/skbuff.h>
#include <net/sock.h>

#include <asm/param.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 0))
#include <linux/writeback.h>
#endif
#endif /* linux */

/* ----- SAMFS Includes */

#include "sam/types.h"
#ifdef sun
#include "sam/samaio.h"
#include "sam/samevent.h"
#endif /* sun */

#include "inode.h"
#include "mount.h"
#include "segment.h"
#include "rwio.h"
#ifdef sun
#include "extern.h"
#endif /* sun */
#ifdef linux
#include "clextern.h"
#endif /* linux */
#include "arfind.h"
#include "debug.h"
#include "macros.h"
#include "trace.h"
#include "acl.h"
#include "sam_thread.h"
#ifdef sun
#include "kstats.h"
#endif /* sun */
#ifdef linux
#include "kstats_linux.h"
#include "samfs_linux.h"
#endif /* linux */


#ifdef sun
static void sam_free_incore_inode(sam_node_t *ip);
#endif
int sam_freeze_ino(sam_mount_t *mp, sam_node_t *ip, int force_freeze);

#ifdef linux
extern char *statfs_thread_name;
#endif /* linux */


#ifdef sun
/*
 * ---- sam_access_ino_ul
 *
 * Dummy version of sam_access_ino.  Needed for security functional
 * secpolicy_vnode_setattr, which requires an "unlocked" version of
 * sam_access_ino().
 */
int
sam_access_ino_ul(void *vip, int mode, cred_t *credp)
{
	sam_node_t *ip = (sam_node_t *)vip;

	return (sam_access_ino(ip, mode, TRUE, credp));
}


/*
 * ----- sam_access_ino - Get access permissions.
 * Given an inode & mode of access, verify permissions for caller.
 * Shift mode from owner to group to other for check.
 *
 * Caller must indicate whether the inode lock is held.  It may be
 * acquired in read mode, if not.
 */

int				/* ERRNO if error, 0 if successful. */
sam_access_ino(
	sam_node_t *ip,		/* pointer to inode. */
	int mode,		/* mode of access to be verified */
	boolean_t locked,	/* is ip->inode_rwl held by caller? */
	cred_t *credp)		/* credentials pointer. */
{
	int shift = 0;

	ASSERT(!locked || RW_LOCK_HELD(&ip->inode_rwl));

	/*
	 * If requesting write access, and read only filesystem or WORM file
	 * return error.
	 */
	if (mode & S_IWRITE) {
		if (ip->mp->mt.fi_mflag & MS_RDONLY) {
			return (EROFS);
		}
		if (ip->di.status.b.worm_rdonly && !S_ISDIR(ip->di.mode)) {
			return (EROFS);
		}
	}

	if (!locked) {
		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
	}

	/* Use ACL, if present, to check access. */
	if (ip->di.status.b.acl) {
		int error;

		error = sam_acl_access(ip, mode, credp);
		if (!locked) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		}
		return (error);
	}

	if (!locked) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	}

	if (crgetuid(credp) != ip->di.uid) {
		shift += 3;
		if (!groupmember((uid_t)ip->di.gid, credp)) {
			shift += 3;
		}
	}
	mode &= ~(ip->di.mode << shift);
	if (mode == 0) {
		return (0);
	}
	return (secpolicy_vnode_access(credp, SAM_ITOV(ip), ip->di.uid, mode));
}
#endif /* sun */

#ifdef linux
int				/* ERRNO if error, 0 if successful. */
sam_access_ino(
	sam_node_t *ip,		/* pointer to inode. */
	int mode,		/* mode of access to be verified, */
				/*   owner position. */
	boolean_t locked,	/* is ip->inode_rwl held by caller? */
	cred_t *credp)		/* credentials pointer. */
{

	ASSERT(!locked);

	/*
	 * If requesting write access, and read only filesystem or WORM file
	 * return error.
	 */
	if (mode & S_IWRITE) {
		if (ip->mp->mt.fi_mflag & MS_RDONLY) {
			return (EROFS);
		}
		if (ip->di.status.b.worm_rdonly && !S_ISDIR(ip->di.mode)) {
			return (EROFS);
		}
	}
	if (crgetuid(credp) == 0) {
		return (0);		/* all accesses allowed for root */
	}

	if (crgetuid(credp) != ip->di.uid) {
		/*
		 * caller not owner, check group
		 */
		mode >>= 3;
		if (!in_group_p(ip->di.gid)) {
			/*
			 * caller not in group, check other
			 */
			mode >>= 3;
		}
	}
	if ((ip->di.mode & mode) == mode) {
		return (0);
	}
	return (EACCES);
}
#endif /* linux */

#ifdef sun
/*
 * ----- sam_inactive_ino - Inode is inactive, deallocate it.
 * Inactivate the vnode--decrement the v_count. If still active
 * exit. Check if file has been removed (nlink == 0).
 *
 * NOTE: inode_rwl lock must not be held by the caller.
 */

void
sam_inactive_ino(
	sam_node_t *ip,		/* pointer to inode. */
	cred_t *credp)		/* credentials pointer. */
{
	vnode_t *vp;
	ino_t ino;
	sam_mount_t *mp;

	/*
	 * If incore inode count exceeds limit, free one.  Cannot free the
	 * inode that is being inactivated.
	 */
	if (samgt.inocount > samgt.ninodes) {
		sam_free_incore_inode(ip);
	}

	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	ino = ip->di.id.ino;
	vp = SAM_ITOV(ip);
	mp = ip->mp;

	/*
	 * Check if file is now active, could happen between last v_lock.
	 * Also if file is free and not hashed or stale, treat like a
	 * client inactivate.
	 */
	mutex_enter(&vp->v_lock);
	if (vp->v_count > 1 ||
	    (ip->flags.bits & (SAM_FREE|SAM_HASH|SAM_STALE)) == SAM_FREE) {
		vp->v_count--;
		mutex_exit(&vp->v_lock);
		TRACE(T_SAM_INACTIVE_RET1, vp, vp->v_count,
		    ip->flags.bits, ino);
		if (ip->flags.bits & SAM_FREE) {
			if (ip->di.status.b.direct) {
				sam_free_stage_n_blocks(ip);
			}
			if (SAM_IS_SHARED_SERVER(ip->mp) &&
			    (ip->sr_leases == NULL) &&
			    ((ip->flags.bits & SAM_NORECLAIM) == 0) &&
			    !(ip->mp->mt.fi_status &
			    (FS_FROZEN|FS_RESYNCING))) {
				(void) sam_trunc_excess_blocks(ip);
			}
		}
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		return;
	}

	/*
	 * Defer the inactivate for inodes accessed via NFS.  Defer the
	 * inactivate for all inodes that are staging (local and NFS) to
	 * prevent duplicate stage requests. Defer inodes in the mat OSN
	 * file system if inode is busy or I/O operations have occurred since
	 * the last inactivate.
	 */
	if ((ip->di.nlink > 0) && (ino != 0) &&
	    !(ip->flags.bits & SAM_STALE) &&
	    !(ip->mp->mt.fi_status & (FS_UMOUNT_IN_PROGRESS|FS_LOCK_HARD|
	    FS_FAILOVER)) &&
	    !(vp->v_vfsp->vfs_flag & VFS_UNMOUNTED) &&
	    !S_ISSEGS(&ip->di)) {

		if (((ip->nfs_ios != 0) && SAM_THREAD_IS_NFS()) ||
		    (ip->flags.bits & SAM_STAGING) ||
		    ((mp->mt.fi_type == DT_META_OBJ_TGT_SET) && ip->objnode &&
		    (ip->objnode->obj_busy || ip->obj_ios))) {

			struct sam_inv_inos *rip, *ripn;

			TRACE(T_SAM_INACTIVE_RET2, vp, vp->v_count,
			    ip->flags.bits, ino);
			mutex_exit(&vp->v_lock);
			rip = (struct sam_inv_inos *)kmem_zalloc(
			    sizeof (struct sam_inv_inos), KM_SLEEP);
			rip->ip = ip;
			rip->id = ip->di.id;
			rip->entry_time = lbolt;
			/*
			 * Clear mat object I/O ops at delayed inactivate time.
			 */
			ip->obj_ios = 0;
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

			mutex_enter(&mp->mi.m_inode.mutex);
			ripn = mp->mi.m_invalp;
			mp->mi.m_invalp = rip;
			mp->mi.m_inval_count++;
			rip->next = ripn;
			mutex_exit(&mp->mi.m_inode.mutex);
			sam_taskq_add(sam_inactivate_inodes, mp, NULL,
			    SAM_INACTIVE_DEFER_SECS * hz);
			return;
		}
	}

	mutex_exit(&vp->v_lock);

	if (ino != 0 && !(ip->flags.bits & SAM_STALE)) {
		if (S_ISSEGI(&ip->di) && ip->segment_ip) {
			/*
			 * If active segment inode, release it.
			 */
			SAM_SEGRELE(SAM_ITOV(ip->segment_ip));
			ip->segment_ip = NULL;
			ip->segment_ord = 0;
		}

		if (ip->aclp) {	/* Inactivate access control list struct. */
			(void) sam_acl_inactive(ip);
		}

		if (vp->v_type == VDIR) {
			/* if directory, purge associated DNLC */
			dnlc_dir_purge(&ip->i_danchor);
			ip->ednlc_ft = 0;
			TRACE(T_SAM_EDNLC_PURGE, vp, ino, 4, 0);
		}
	}

	if ((mp->mt.fi_mflag & MS_RDONLY) ||
	    SAM_IS_SHARED_CLIENT(ip->mp) ||
	    (ino == 0) || (ip->flags.bits & SAM_STALE)) {
		/*
		 * Read only filesystem, just unhash. Zero i-number, or stale,
		 * means we are inactivating a newly allocated inode structure
		 * which has not yet been fully initialized.
		 */
		sam_return_this_ino(ip, 1);
		TRACE(T_SAM_INACTIVE_RET3, vp, vp->v_count,
		    ip->flags.bits, ino);

	/*
	 * The inode flag SAM_NORECLAIM is set when the server does a force
	 * expiration of leases for failover or a unmount.
	 * XXX - Need to handle expiration of leases for a unmount with clients
	 * still mounted.
	 */
	} else if (((int)ip->di.nlink <= 0) &&
	    (((ip->flags.bits & SAM_NORECLAIM) == 0) ||
	    !(ip->mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING)))) {
		/*
		 * Purge file--release file space, archived inodes, and free
		 * inode.
		 */
		ip->rdev = 0;
		(void) sam_clear_file(ip, 0, MAKE_ONLINE, credp);
		/*
		 * Check again if inode has has been acquired since v_lock was
		 * lifted.
		 */
		mutex_enter(&vp->v_lock);
		if (vp->v_count > 1) {
			vp->v_count--;
			mutex_exit(&vp->v_lock);
			TRACE(T_SAM_INACTIVE_RET4, vp, vp->v_count,
			    ip->flags.bits, ino);
			if (ip->di.status.b.direct) {
				sam_free_stage_n_blocks(ip);
			}
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			return;
		}
		ip->flags.bits |= SAM_PURGE_PEND;
		mutex_exit(&vp->v_lock);
		(void) sam_clear_file(ip, 0, PURGE_FILE, credp);

		/*
		 * The last free inode is the first to be assigned. It is
		 * possible this inode has been reassigned. Do not unhash it if
		 * reassigned.
		 */
		sam_return_this_ino(ip, 1);
		TRACE(T_SAM_INACTIVE_RET5, vp, vp->v_count,
		    ip->flags.bits, ino);

	} else {
		int flag = (mp->mt.fi_status & FS_FAILOVER) ? 0 : B_ASYNC;
		/*
		 * File is inactive -- sync pages & inode, then put on freelist.
		 */
		if (S_ISSEGI(&ip->di)) {
			sam_callback_segment_t callback;

			callback.p.inactivate.flag = flag;
			(void) sam_callback_segment(ip, CALLBACK_inactivate,
			    &callback, TRUE);
		}
		(void) sam_inactivate_ino(SAM_ITOV(ip), flag);
		sam_return_this_ino(ip, 0);
		TRACE(T_SAM_INACTIVE_RET6, vp, vp->v_count,
		    ip->flags.bits, ino);
	}
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
}


/*
 * ----- sam_inactive_stale_ino
 *
 * Inode is being inactivated, and the associated FS has been
 * forcibly unmounted.  Deallocate the inode, and if it's the
 * last outstanding inode, free up the mount table and vfs table.
 */
/* ARGSUSED1 */
void
sam_inactive_stale_ino(sam_node_t *ip, cred_t *credp)
{
	vnode_t *vp = SAM_ITOV(ip);
	sam_mount_t *mp = ip->mp;

	/*
	 * Hold up until any pending unmount completes.
	 */
	sam_await_umount_complete(ip);

	/*
	 * Don't let mp/vfsp get away from us yet
	 */
	SAM_VFS_HOLD(mp);

	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	if (ip->aclp) {
		/* Free in-core access control list */
		(void) sam_free_acl(ip);
	}
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

	/*
	 * It is possible to have pages for this vnode if there was an error
	 * flushing them. The stale vnops calls sam_putpage_vn.
	 */
	mutex_enter(&vp->v_lock);
	ASSERT(!ip->flags.b.hash);
	ASSERT(!ip->flags.b.free);
	TRACE(T_SAM_INACTIVE_RET0, vp, vp->v_count,
	    ip->flags.bits, ip->di.id.ino);
	if (vp->v_count-- > 1) {
		/*
		 * Shouldn't happen.  It's inactive...
		 */
		mutex_exit(&vp->v_lock);
		ASSERT(!ip->flags.b.free);
	} else {
		mutex_exit(&vp->v_lock);
		ASSERT(!rw_write_held(&ip->data_rwl));
		sam_osd_destroy_obj_layout(ip, 0);
		sam_destroy_ino(ip, FALSE);	/* calls inode destructor */
	}

	/* let mp/vfsp go if this was the last outstanding inode */
	SAM_VFS_RELE(mp);
}


/*
 * ----- sam_return_this_ino -
 * Decrement vnode count now. If file has become active, return.
 * Otherwise, if shared reader, remove inode. No inode cache is
 * retained for shared reader. If not shared reader, put
 * this inode on the freelist.
 * NOTE: ihashlock must be acquired before vp->v_lock, and must
 *		 not be held across calls to flush pages (which will
 *		 release and reacquire the inode writer lock).
 */

void
sam_return_this_ino(
	sam_node_t *ip,		/* pointer to inode. */
	int purge_flag)		/* Set if purging file or reader or client */
{
	kmutex_t *ihp;
	vnode_t *vp = SAM_ITOV(ip);

	mutex_enter(&vp->v_lock);

	/*
	 * Flush and invalidate pages.
	 * Note, for regular files, putpage does not flush during
	 * freezing/frozen, to prevent a deadlock if indirect blocks
	 * must be retrieved from the server. Buffer cache is invalidated
	 * at the end of the frozen state.
	 */
	if (vn_has_cached_data(vp) && (vp->v_count <= 1)) {
		mutex_exit(&vp->v_lock);
		if (ip->flags.bits & SAM_PURGE_PEND) {
			(void) pvn_vplist_dirty(vp, 0,
			    sam_putapage, (B_INVAL | B_TRUNC), CRED());
		} else {
			int flags;
			int error;

			/*
			 * Flush and invalidate pages associated with this free
			 * inode.  Truncate shared client file system directory
			 * pages & stale inode pages because they may be stale.
			 * Call VOP_PUTPAGE directly so writer inode lock is
			 * not dropped & inode updates cannot occur.
			 */
			flags = B_INVAL;
			if ((SAM_IS_SHARED_CLIENT(ip->mp) &&
			    S_ISDIR(ip->di.mode)) ||
			    (ip->flags.bits & SAM_STALE)) {
				flags = B_INVAL|B_TRUNC;
			}
			ASSERT(rw_owner(&ip->inode_rwl) == curthread);
			error = VOP_PUTPAGE_OS(vp, 0, 0, flags, CRED(), NULL);
			if (error) {
				cmn_err(CE_WARN,
"SAM-QFS: %s: cannot flush pages err=%d: ip=%p, %d:%d.%d, count %d, pages=%p",
				    ip->mp->mt.fi_name, error, (void *)ip,
				    ip->mp->mi.m_fs[0].part.pt_eq,
				    ip->di.id.ino, ip->di.id.gen, vp->v_count,
				    (void *)vp->v_pages);
			}
		}
		if (SAM_IS_SHARED_CLIENT(ip->mp) &&
		    !(ip->mp->mt.fi_status & (FS_FREEZING | FS_FROZEN))) {
			(void) sam_stale_indirect_blocks(ip, 0);
		}
		mutex_enter(&vp->v_lock);
	} else {
		if (SAM_IS_SHARED_CLIENT(ip->mp) &&
		    !(ip->mp->mt.fi_status & (FS_FREEZING | FS_FROZEN))) {
			(void) sam_stale_indirect_blocks(ip, 0);
		}
	}

	/*
	 * Release v_lock because page flushes can cause the VM to acquire it.
	 */
	mutex_exit(&vp->v_lock);

	/*
	 * If client, changed or modified, and not directory, not frozen,
	 * and not stale or reclaimed, NFS or nlink zero, remove leases.
	 * This will cause the inode to be inactivated on the server when
	 * all the leases are removed.
	 */
	if (SAM_IS_SHARED_CLIENT(ip->mp) &&
	    (ip->flags.bits & SAM_UPDATE_FLAGS) && !S_ISDIR(ip->di.mode) &&
	    !(ip->mp->mt.fi_status & FS_FROZEN) &&
	    ((ip->flags.bits & (SAM_NORECLAIM|SAM_STALE)) == 0) &&
	    (SAM_THREAD_IS_NFS() || (ip->di.nlink == 0))) {
		(void) sam_proc_rm_lease(ip, CL_CLOSE, RW_WRITER);
	}

	/*
	 * Acquire hash lock so that we can remove the inode from the hash list
	 * if appropriate.  Note that proper lock ordering is to acquire the
	 * inode lock first (we have it), then the hash lock, then the v_lock.
	 */
	ihp = &samgt.ihashlock[SAM_IHASH(ip->di.id.ino, ip->dev)];
	mutex_enter(ihp);
	mutex_enter(&vp->v_lock);

#ifdef DEBUG
	{
		sam_node_t *tip, *dip;
		sam_id_t id;
		sam_ihead_t *hip;
		int match_found = 0, match_stale = 0, match_client_dup = 0;

		/*
		 * Verify that a dev/inode only lives on its chain once.
		 * There is a performance cost to this check, but it should
		 * be small, as the hash buckets are sized to be small.
		 */
		hip = &samgt.ihashhead[SAM_IHASH(ip->di.id.ino, ip->dev)];
		ASSERT(hip != NULL);
		id = ip->di.id;

		tip = (sam_node_t *)hip->forw;
		while (tip != (sam_node_t *)(void *)hip) {
			/*
			 * Also check each cache entry for an undamaged
			 * in-memory pad area. The hope here is to isolate when
			 * the pad is destroyed.
			 */
			SAM_VERIFY_INCORE_PAD(tip);
			if ((id.ino == tip->di.id.ino) && (ip->mp == tip->mp)) {
				dip = tip;
				match_found++;
				if ((tip->flags.bits & SAM_STALE) != 0) {
					match_stale++;
				}
				if (id.gen != tip->di.id.gen) {
					if (SAM_IS_SHARED_CLIENT(ip->mp)) {
						match_client_dup++;
					}
				}
			}
			tip = tip->chain.hash.forw;
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
			    "SAM-QFS: %s: sam_return_this_ino: duplicate "
			    "dev/inum found ip=%p ino.gen=%d.%d "
			    "duplicates found: %d stale dups found: %d "
			    "client dups found: %d",
			    dip->mp->mt.fi_name, (void *)dip, dip->di.id.ino,
			    dip->di.id.gen, match_found, match_stale,
			    match_client_dup);
		}
	}
#endif /* DEBUG */

	/*
	 * If purge_flag, check to see if free inode has been reassigned. If
	 * reassigned, do not unhash it because it is waiting on the inode
	 * WRITERS lock. Clear purge_pend to indicate inode has completed
	 * purging.
	 */
	if (purge_flag) {
		if (vp->v_count <= 1) {
			if (ip->flags.bits & SAM_HASH) {
				SAM_DESTROY_OBJ_LAYOUT(ip);
				SAM_UNHASH_INO(ip);
			}
		}
		ip->flags.bits &= ~SAM_PURGE_PEND;
	}

	vp->v_count--;
	if ((int)vp->v_count >= 1) {
		mutex_exit(&vp->v_lock);
		mutex_exit(ihp);
		return;
	}

	/*
	 * If samaio character file, detach it now. This removes the character
	 * device file.
	 */
	if ((ip->di.rm.ui.flags & RM_CHAR_DEV_FILE) && (vp->v_type == VCHR)) {
		sam_detach_aiofile(vp);
	}

	sam_quota_inode_fini(ip);

	/*
	 * If shared client or shared reader, remove from hash and invalidate
	 * inode.
	 */
	if (SAM_IS_CLIENT_OR_READER(ip->mp) ||
	    (ip->mp->mt.fi_status & (FS_FREEZING|FS_FROZEN))) {

		/*
		 * If no pages, remove inode from the hash and put in freelist.
		 * If pages, keep in hash chain and set inactivate.
		 * At resync time, the inode will be unhashed and freed after
		 * its pages are sync'ed.
		 */
		if (vn_has_cached_data(vp) == 0) {
			if (ip->flags.bits & SAM_HASH) {
				SAM_DESTROY_OBJ_LAYOUT(ip);
				SAM_UNHASH_INO(ip);
			}
		} else {
			/*
			 * Set inactivate to indicate to the reclaim thread to
			 * finish the inactivate. Note, these inodes are in the
			 * free chain with SAM_INACTIVATE mask set.
			 */
			dcmn_err((CE_NOTE,
	"SAM-QFS: %s: return_this_ino & pages: ip=%p, %d.%d, "
	"count %d, pages=%p",
			    ip->mp->mt.fi_name, (void *)ip, ip->di.id.ino,
			    ip->di.id.gen,
			    vp->v_count, (void *)vp->v_pages));
			vp->v_count++;
			ip->flags.bits |= SAM_INACTIVATE;
		}
	}
	mutex_exit(ihp);

	/*
	 * Do not hold hash lock while modifying free list. Can cause deadlock.
	 */
	if (!(ip->flags.bits & SAM_FREE)) {
		if (vn_has_cached_data(vp) || ip->cnt_writes || ip->wr_fini ||
		    ip->wr_thresh) {
			sam_put_freelist_tail(ip);
		} else {
			sam_put_freelist_head(ip);
		}
		ip->flags.bits &= ~SAM_BUSY;
	}
	mutex_exit(&vp->v_lock);
}


/*
 * ----- sam_detach_aiofile - detach SAM-QFS AIO driver.
 */

void
sam_detach_aiofile(vnode_t *vp)
{
	sam_node_t *ip;	/* pointer to inode. */
	struct samaio_ioctl aio;
	int minor;
	int error;

	ip = SAM_VTOI(vp);
	aio.vp = vp;
	aio.fs_eq = ip->mp->mt.fi_eq;
	aio.ino = ip->di.id.ino;
	aio.gen = ip->di.id.gen;
	error = cdev_ioctl(vp->v_rdev, SAMAIO_DETACH_DEVICE,
	    (intptr_t)&aio, ip->di.mode, CRED(), &minor);
	if (error == 0) {
		vp->v_type = VREG;
		vp->v_rdev = 0;
	} else {
		cmn_err(CE_WARN,
"SAM-QFS: %s: sam_detach_aiofile: char dev error=%d: ip=%p, %d.%d, rdev %lx",
		    ip->mp->mt.fi_name, error, (void *)ip,
		    ip->di.id.ino, ip->di.id.gen, vp->v_rdev);
	}
}


/*
 * ----- sam_inactivate_ino - Flush all pages and sync ino.
 *	If shared server, truncate blocks down to size.
 *	Switch to default I/O type and default qwrite setting.
 *	Flush and sync this inactivated inode.
 *	NOTE. inode_rwl lock set on entry and exit.
 */

int
sam_inactivate_ino(vnode_t *vp, int flag)
{
	sam_node_t *ip = SAM_VTOI(vp);

	if (ip->di.status.b.direct) {
		sam_free_stage_n_blocks(ip);
	}
	if (ip->listiop) {
		sam_remove_listio(ip);
	}
	if (SAM_IS_SHARED_SERVER(ip->mp) && (ip->sr_leases == NULL) &&
	    ((ip->flags.bits & SAM_NORECLAIM) == 0) &&
	    !(ip->mp->mt.fi_status & FS_FROZEN)) {
		(void) sam_trunc_excess_blocks(ip);
	}
	sam_reset_default_options(ip);
	sam_flush_pages(ip, flag);
	(void) sam_update_inode(ip, SAM_SYNC_ONE, FALSE);
	return (0);
}


/*
 * ----- sam_free_stage_n_blocks - free stage_n blocks in last window.
 */

void
sam_free_stage_n_blocks(sam_node_t *ip)
{
	/*
	 * If stage_n, offline and not staging, free stage_n window blocks
	 */
#ifdef METADATA_SERVER
	if (S_ISREG(ip->di.mode) && ip->di.status.b.offline &&
	    !(ip->flags.bits & SAM_STAGING)) {
		if (ip->di.blocks != 0) {
			(void) sam_drop_ino(ip, CRED());
		}
		ip->real_stage_off = 0;
		ip->real_stage_len = 0;
		ip->stage_off = 0;
		ip->stage_len = 0;
		if (ip->st_buf) {
			atomic_add_64(&ip->mp->mi.m_umem_size,
			    -ip->st_umem_len);
			ddi_umem_free(ip->st_cookie);
			ip->st_buf = NULL;
			ip->st_umem_len = 0;
		}
	}
#endif
}
#endif /* sun */


/*
 * ----- sam_cv_wait_sig -  issue cv_wait_sig.
 * Save the signals. If not kernel thread, mask out all signals except SIGHUP,
 * SIGQUIT, SIGINT, and SIGTERM. Restore signals after returning.
 */

int				/* 1 if signal received, 0 if interrupted. */
sam_cv_wait_sig(kcondvar_t *cvp, kmutex_t *kmp)
{
	int error;
#ifdef sun
	k_sigset_t savesigmask;
	int mask_signals = 0;

	if (ttolwp(curthread) != NULL) {	/* If not kernel thread */
		sigintr(&savesigmask, 1);
		mask_signals = 1;
	}
#endif /* sun */

	error = cv_wait_sig(cvp, kmp);

#ifdef sun
	if (mask_signals) {
		sigunintr(&savesigmask);
	}
#endif /* sun */
	return (error);
}


/*
 * ----- sam_cv_wait1sec_sig -  issue cv_timedwait_sig.
 * Save the signals. If not kernel thread, mask out all signals except SIGHUP,
 * SIGQUIT, SIGINT, and SIGTERM. Restore signals after returning.
 */

int			/* 1 if signaled, 0 if interrupted, -1 if timed out */
sam_cv_wait1sec_sig(kcondvar_t *cvp, kmutex_t *kmp)
{
	int ret;
#ifdef sun
	k_sigset_t savesigmask;
	int mask_signals = 0;

	if (ttolwp(curthread) != NULL) {	/* If not kernel thread */
		sigintr(&savesigmask, 1);
		mask_signals = 1;
	}
#endif /* sun */

	ret = cv_timedwait_sig(cvp, kmp, lbolt + hz);

#ifdef sun
	if (mask_signals) {
		sigunintr(&savesigmask);
	}
#endif /* sun */
	return (ret);
}


/*
 * ----- sam_check_sig -  check if signal received.
 * Save the signals. If not kernel thread, mask out all signals except SIGHUP,
 * SIGQUIT, SIGINT, and SIGTERM. Restore signals after returning.
 */

#ifdef sun
int				/* 1 if signal received, 0 if none */
sam_check_sig()
{
	int ret = 0;
	k_sigset_t savesigmask;

	if (ttolwp(curthread) != NULL) {	/* If not kernel thread */
		sigintr(&savesigmask, 1);
		ret = ISSIG(curthread, JUSTLOOKING);
		sigunintr(&savesigmask);
	}
	return (ret);
}


/*
 * ----- sam_free_incore_inode - Free memory for incore inode.
 * If the number of incore inodes is over the limit, free space
 * from the top of the freelist.
 */

static void
sam_free_incore_inode(
	sam_node_t *iip)	/* pointer to inode which cannot be freed. */
{
	sam_node_t *ip, *nip;
	vnode_t *vp;
	int flags;
	kmutex_t *ihp;
	int ihash;
	int ret_noaction;
	int error;
	ino_t ino;

	mutex_enter(&samgt.ifreelock);
	ip = (sam_node_t *)samgt.ifreehead.free.forw;

	/* Skip the inactive inode */
	ret_noaction = 0;
	if ((ip == (sam_node_t *)(void *)&samgt.ifreehead) || (ip == iip) ||
	    (ip->di.id.ino <= SAM_BLK_INO) ||
	    (ip->mp->mt.fi_status & FS_FAILOVER)) {
		ret_noaction = 1;
	} else {
		/* skip if caller owns this hash mutex */
		ihash = SAM_IHASH(ip->di.id.ino, ip->dev);
		ihp = &samgt.ihashlock[ihash];
		ino = ip->di.id.ino;
		if (mutex_owned(ihp)) {
			ret_noaction = 1;
		}
	}

	mutex_exit(&samgt.ifreelock);
	if (ret_noaction) {		/* if skipping */
		return;
	}
	mutex_enter(ihp);
	mutex_enter(&samgt.ifreelock);
	nip = (sam_node_t *)samgt.ifreehead.free.forw;

	/* If enough inodes or previous head inode no longer free */
	if ((samgt.inocount <= samgt.ninodes) || (ip != nip) ||
	    (ino != nip->di.id.ino)) {
		mutex_exit(&samgt.ifreelock);
		mutex_exit(ihp);
		return;
	}
	vp = SAM_ITOV(ip);
	if (RW_TRYENTER_OS(&ip->inode_rwl, RW_WRITER) == 0) {
		mutex_exit(&samgt.ifreelock);
		mutex_exit(ihp);
		return;
	}

	VN_HOLD(vp);			/* Make inode busy to hold in hash */
	ip->flags.bits |= SAM_BUSY;	/* Inode is active (SAM_BUSY) */
	sam_remove_freelist(ip);
	mutex_exit(&samgt.ifreelock);
	mutex_exit(ihp);

	/*
	 * Flush and invalidate pages associated with this free inode.
	 * Truncate shared client file system directory pages; may be stale.
	 */
	flags = B_INVAL;
	if (SAM_IS_SHARED_CLIENT(ip->mp) && S_ISDIR(ip->di.mode)) {
		flags = B_INVAL|B_TRUNC;
	}
	error = sam_flush_pages(ip, flags);

	mutex_enter(ihp);
	mutex_enter(&vp->v_lock);
	mutex_enter(&ip->write_mutex);
	if (vn_has_cached_data(vp) || (vp->v_count > 1) || ip->cnt_writes) {
		int no_rele;

		/*
		 * We're here because either the inode is 'dented' with pages
		 * still attached (likely from an earlier error, or because
		 * somebody is in the process of reclaiming it (vcount > 1)
		 */
		TRACE(T_SAM_KPINCORE, NULL, ip->di.id.ino,
		    vp->v_count, ip->flags.bits);
		mutex_exit(&ip->write_mutex);
		no_rele = error && (samgt.inocount > samgt.ninodes);
		if (no_rele) {
			/*
			 * Want to keep this out of the VN_RELE()/VOP_INACTIVE()
			 * path since we're relatively certain that we've got a
			 * failing device nearby and there could be a slew of
			 * other similar inodes on the free list and to try and
			 * recurse through them until hitting a good one might
			 * blow our stack.
			 *
			 * Since this guy is off the free
			 * list but still on the hash list, do the logical
			 * VN_RELE to release our hold and move on.
			 */
			vp->v_count--;
		}
		mutex_exit(&vp->v_lock);
		mutex_exit(ihp);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (!no_rele) {
			VN_RELE(vp);
		}
		return;
	}
	if (ip->flags.bits & SAM_HASH) {
		SAM_DESTROY_OBJ_LAYOUT(ip);
		SAM_UNHASH_INO(ip);
	}
	TRACE(T_SAM_RMINCORE, NULL, ip->di.id.ino, vp->v_count, ip->flags.bits);
	mutex_exit(&ip->write_mutex);
	mutex_exit(&vp->v_lock);
	mutex_exit(ihp);
	if (vp->v_count > 1) {
		cmn_err(CE_PANIC,
		"SAM-QFS: %s: sam_free_incore_inode: Removing "
		"inode %d.%d, vcount %d",
		    ip->mp->mt.fi_name, ip->di.id.ino, ip->di.id.gen,
		    vp->v_count);
	}
	ASSERT(vn_has_cached_data(vp) == 0);
	SAM_DESTROY_OBJ_LAYOUT(ip);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	sam_destroy_ino(ip, FALSE);
}
#endif /* sun */


/*
 * ----- sam_mark_ino -  Update the inode times based on flag.
 * Since we only keep 32-bit of time in the on-disk inode, if
 * the file system is still alive beyond 2038, the inode times
 * will just stick at the last setting.
 */

void
sam_mark_ino(
	sam_node_t *ip,		/* pointer to the inode entry. */
	uint_t flags)		/* SAM_ACCESSED, SAM_UPDATED, SAM_CHANGED. */
{
	/*
	 * Must have unique time in microseconds for nfs.
	 */
	typedef union {
		struct timeval32 tv;
		uint64_t whole;
	} timeval32_u;
	static timeval32_u sam_time = {{0, 0}};
	timeval32_u tmp_time;
#ifdef sun
	timespec_t system_time;
#endif /* sun */
#ifdef linux
	struct timeval system_time;
#endif /* linux */
	sam_timestruc_t inode_time;
	long inode_usec;

	/*
	 * Do not change access time if the process is arcopy or noatime is set.
	 * Note, the SAM_ACCESSED bit is not set in ip->flags (change in
	 * behavior) so the inode will not be updated if it has not been
	 * changed/modified.
	 */
	if (flags & SAM_ACCESSED) {
		if (ip->mp->mt.fi_atime == -1) {	/* If no atime update */
			if ((flags & (SAM_CHANGED | SAM_UPDATED)) == 0) {
				return;
			}
		}
#ifdef sun
		if (ip->arch_count) {
			int i;

			for (i = 0; i < MAX_ARCHIVE; i++) {
				if (ip->arch_pid[i] == 0) {
					continue;
				}
				if (ip->arch_pid[i] == curproc->p_ppid) {
					flags &= ~SAM_ACCESSED;
					if ((flags &
					    (SAM_CHANGED | SAM_UPDATED)) == 0) {
						return;
					}
					break;
				}
			}
		}
#endif /* sun */
	}
	SAM_HRESTIME(&system_time);

	/*
	 * Convert to microseconds.
	 */
#ifdef sun
	inode_usec = system_time.tv_nsec / 1000;
#endif /* sun */
#ifdef linux
	inode_usec = system_time.tv_usec;
#endif /* linux */
	/*
	 * Use the 64-bit variant of the union of two 32-bit timevals.
	 * For machines of smaller word size (< 64),
	 * a second in time may be lost.
	 */
	tmp_time.whole = sam_time.whole;

	/*
	 * Start by adjusting sam_time to correspond to changes in the
	 * system_time.  We need to handle time moving forward (the normal case)
	 * as well as time moving backward.
	 */
	if (((system_time.tv_sec > tmp_time.tv.tv_sec) ||
	    ((system_time.tv_sec == tmp_time.tv.tv_sec) &&
	    (inode_usec > tmp_time.tv.tv_usec))) ||
	    ((system_time.tv_sec < tmp_time.tv.tv_sec) ||
	    ((system_time.tv_sec == tmp_time.tv.tv_sec) &&
	    (inode_usec < tmp_time.tv.tv_usec)))) {
		if (system_time.tv_sec < INT32_MAX) {
			tmp_time.tv.tv_sec = system_time.tv_sec;
			tmp_time.tv.tv_usec = inode_usec;
			sam_time.whole = tmp_time.whole;
		}
	} else {
		if (tmp_time.tv.tv_sec < INT32_MAX) {
			tmp_time.tv.tv_usec++;
			/*
			 * Check for microsecond overflow.
			 */
			if (tmp_time.tv.tv_usec >= MICROSEC) {
				tmp_time.tv.tv_sec++;
				tmp_time.tv.tv_usec -= MICROSEC;
			}
			sam_time.whole = tmp_time.whole;
		}
	}
	inode_time.tv_sec = tmp_time.tv.tv_sec;
	inode_time.tv_nsec = tmp_time.tv.tv_usec * 1000;

	ip->flags.bits |= flags;
	ip->cl_flags |= (flags & (SAM_ACCESSED|SAM_UPDATED|SAM_CHANGED));

	/*
	 * Update the times in the inode if the flags mask is set. If
	 * the inode is a data segment, also update the index inode.
	 */
	if (flags & SAM_ACCESSED) {
		ip->di.access_time = inode_time;
		if (S_ISSEGS(&ip->di)) {
			ip->segment_ip->di.access_time = inode_time;
			ip->segment_ip->flags.bits |= SAM_ACCESSED;
		}
	}
	if (flags & SAM_CHANGED) {
		ip->di.change_time = inode_time;
		if (S_ISSEGS(&ip->di)) {
			ip->segment_ip->di.change_time = inode_time;
			ip->segment_ip->flags.bits |= SAM_CHANGED;
		}
	}
	if (!(flags & SAM_UPDATED)) {
		return;
	}

	/*
	 * Modify time has been set and is valid.
	 */
	ip->di.modify_time = inode_time;
	ip->flags.bits |= (SAM_DIRTY | SAM_VALID_MTIME);

	if (S_ISSEGS(&ip->di)) {
		/*
		 * Update modify time in index inode.
		 */
		ip->segment_ip->di.modify_time = inode_time;
		ip->segment_ip->di.status.b.archdone = 0;
		ip->segment_ip->flags.bits |= SAM_UPDATED;
	}

	/*
	 * Remove archive copy status. Mark copies stale.
	 * Notify arfind & event daemon of changed file (or data segment).
	 * If the WORM bit is enabled we can't do this as we
	 * will stale archive copies and if the file is off-line
	 * will inadvertently damage the file.  Further, if the
	 * file is WORM'd its data can not change so its
	 * archive copies shouldn't be stale as a result of
	 * applying the trigger.
	 */
	if (ip->di.arch_status && !ip->di.status.b.worm_rdonly) {
		int	mask;
		int	n;

		for (n = 0, mask = 1; n < MAX_ARCHIVE; n++, mask += mask) {
			if (ip->di.arch_status & mask) {
				ip->di.ar_flags[n] |= AR_stale;
				ip->di.ar_flags[n] &= ~AR_verified;
			}
		}
		ip->di.arch_status = 0;
		ip->di.status.b.cs_val = 0;
		ip->di.status.b.archdone = 0;
		sam_send_to_arfind(ip, AE_modify, 0);
		sam_send_event(ip->mp, &ip->di, ev_modify, 0, 0,
		    ip->di.modify_time.tv_sec);
	}
}


/*
 * -----	sam_freeze_ino -
 * Freeze this inode until the lockfs/failover has completed.
 */

int				/* ERRNO, else 0 if successful. */
sam_freeze_ino(sam_mount_t *mp, sam_node_t *ip, int force_freeze)
{
	int error = 0;

	if (ip) {
		TRACE(T_SAM_FREEZE_INO, SAM_ITOP(ip), ip->di.id.ino,
		    ip->di.id.gen, ip->flags.bits);
	}

	/*
	 * If the caller is fsflush, don't freeze. Cannot hold up fsflush.
	 */
	if (sam_is_fsflush()) {
		return (ETIME);
	}

	mutex_enter(&mp->ms.m_waitwr_mutex);
	/*
	 * Test status again under lock so as to avoid race condition
	 * on clearing the flag(s).  Clearing these flags is done under
	 * lock (mp->ms.m_waitwr_mutex) in sam_failover_done() and
	 * sam_srvr_responding().
	 */
	if ((mp->mt.fi_status & (FS_LOCK_HARD | FS_UMOUNT_IN_PROGRESS)) == 0) {
		mutex_exit(&mp->ms.m_waitwr_mutex);
		return (0);
	}
	mp->mi.m_wait_frozen++;
	if (!SAM_THREAD_IS_NFS() || force_freeze) {
		/* if not NFS access or force_freeze is set */
		if (sam_cv_wait_sig(&mp->ms.m_waitwr_cv,
		    &mp->ms.m_waitwr_mutex) == 0) {
			error = EINTR;
		}
	} else {
		if (sam_cv_wait1sec_sig(&mp->ms.m_waitwr_cv,
		    &mp->ms.m_waitwr_mutex) == 0) {
			error = EINTR;
		} else {
#ifdef sun
			curthread->t_flag |= T_WOULDBLOCK;
#endif /* sun */
			error = EAGAIN;
		}
	}
	if (--mp->mi.m_wait_frozen < 0) {
		mp->mi.m_wait_frozen = 0;
	}
	mutex_exit(&mp->ms.m_waitwr_mutex);

	if (ip) {
		TRACE(T_SAM_UNFREEZE_INO, SAM_ITOP(ip), ip->di.id.ino,
		    ip->di.id.gen, ip->flags.bits);
	}
	return (error);
}


/*
 * If there's an unmount or a LOCKFS pending, wait until it finishes.
 */
void
sam_await_umount_complete(sam_node_t *ip)
{
	sam_mount_t *mp = ip->mp;

	while (mp->mt.fi_status & FS_UMOUNT_IN_PROGRESS) {
		(void) sam_freeze_ino(mp, ip, 0);
	}
}


#ifdef sun
/*
 *	----	sam_open_operation
 *
 * This version uses the thread-specific-data functions (tsd_*
 * aka HARPY) that Solaris provides to track potentially re-entrant
 * samfs module functions.
 */
int
sam_open_operation(sam_node_t *ip)
{
	sam_mount_t *mp = ip->mp;
	sam_operation_t ep;
	int error;

	ep.ptr = tsd_get(mp->ms.m_tsd_key);
	if (ep.ptr) {
		ASSERT(ep.val.depth > 0);
		ASSERT(mp->ms.m_cl_active_ops > 0);
		ep.val.depth++;
		tsd_set(mp->ms.m_tsd_key, ep.ptr);
		return (0);
	}
	while (mp->mt.fi_status & (FS_LOCK_HARD | FS_UMOUNT_IN_PROGRESS)) {
		if (curthread->t_flag & T_DONTBLOCK) {
			curthread->t_flag |= T_WOULDBLOCK;
			return (EAGAIN);
		}
		if (IS_SHAREFS_THREAD_OS) { /* Do not freeze shared fs thread */
			break;
		}
		if ((error = sam_freeze_ino(mp, ip, 0)) != 0) {
			return (error);
		}
	}

	/*
	 * Did pending unmount complete?
	 */
	if (!(mp->mt.fi_status & (FS_MOUNTED | FS_MOUNTING))) {
		return (EIO);
	}
	ep.val.depth = 1;
	ep.val.module = mp->ms.m_tsd_key;
	ep.val.flags = 0;
	SAM_INC_OPERATION(mp);
	tsd_set(mp->ms.m_tsd_key, ep.ptr);
	return (0);
}


/*
 *	----	sam_open_ino_operation
 *
 * This version assumes there's an ino lock held, and releases
 * it/reacquires it if there's sleeping to be done.
 */
int
sam_open_ino_operation(sam_node_t *ip, krw_t lock_type)
{
	sam_mount_t *mp = ip->mp;
	sam_operation_t ep;
	int error;

	ep.ptr = tsd_get(mp->ms.m_tsd_key);
	if (ep.ptr != NULL) {
		ASSERT(ep.val.depth > 0);
		ASSERT(mp->ms.m_cl_active_ops > 0);
		ep.val.depth++;
		tsd_set(mp->ms.m_tsd_key, ep.ptr);
		return (0);
	}
	while (mp->mt.fi_status & (FS_LOCK_HARD | FS_UMOUNT_IN_PROGRESS)) {
		if (curthread->t_flag & T_DONTBLOCK) {
			curthread->t_flag |= T_WOULDBLOCK;
			return (EAGAIN);
		}
		if (IS_SHAREFS_THREAD_OS) { /* Do not freeze shared fs thread */
			break;
		}
		RW_UNLOCK_OS(&ip->inode_rwl, lock_type);
		if ((error = sam_freeze_ino(mp, ip, 0)) != 0) {
			RW_LOCK_OS(&ip->inode_rwl, lock_type);
			return (error);
		}
		RW_LOCK_OS(&ip->inode_rwl, lock_type);
	}

	/*
	 * Did pending unmount complete?
	 */
	if (!(mp->mt.fi_status & (FS_MOUNTED | FS_MOUNTING))) {
		return (EIO);
	}
	ep.val.depth = 1;
	ep.val.module = mp->ms.m_tsd_key;
	ep.val.flags = 0;
	SAM_INC_OPERATION(mp);
	tsd_set(mp->ms.m_tsd_key, ep.ptr);
	return (0);
}


/*
 * -----    sam_idle_operation -
 * Check for any pending failovers/umounts
 */
int
sam_idle_operation(sam_node_t *ip)
{
	sam_mount_t *mp = ip->mp;
	sam_operation_t etsdhp;
	int error;

	if (!(mp->mt.fi_status & (FS_MOUNTED | FS_MOUNTING))) {
		return (EIO);
	}

	etsdhp.ptr = tsd_get(mp->ms.m_tsd_key);

	if ((etsdhp.ptr == NULL) ||
	    (etsdhp.val.flags & SAM_FRZI_NOBLOCK)) {
		/*
		 * Don't block.  Either thread has no context (worker thread?
		 * ioctl?), thread is set non-blocking.
		 */
		if (mp->mt.fi_status & (FS_LOCK_HARD | FS_UMOUNT_IN_PROGRESS)) {
			if (curthread->t_flag & T_DONTBLOCK) {
				curthread->t_flag |= T_WOULDBLOCK;
				return (EAGAIN);
			}
		}
		if (!(mp->mt.fi_status & (FS_MOUNTED | FS_MOUNTING))) {
			return (EIO);
		}
		return (0);
	}

	SAM_DEC_OPERATION(mp);
	while (mp->mt.fi_status & (FS_LOCK_HARD | FS_UMOUNT_IN_PROGRESS)) {
		if ((error = sam_freeze_ino(mp, ip, 1)) != 0) {
			SAM_INC_OPERATION(mp);
			return (error);
		}
	}
	SAM_INC_OPERATION(mp);

	if (!(mp->mt.fi_status & (FS_MOUNTED | FS_MOUNTING))) {
		return (EIO);
	}
	return (0);
}


/*
 * -----    sam_idle_ino_operation -
 *
 * Check for any pending failovers/umounts; if found, then
 * release/re-obtain the inode lock for the duration.  This
 * could be combined w/ sam_idle_operation above, making
 * this a wrapper for it.
 */
int
sam_idle_ino_operation(sam_node_t *ip, krw_t lock_type)
{
	sam_mount_t *mp = ip->mp;
	sam_operation_t etsdhp;
	int error;

	if (!(mp->mt.fi_status & (FS_MOUNTED | FS_MOUNTING))) {
		return (EIO);
	}

	etsdhp.ptr = tsd_get(mp->ms.m_tsd_key);
	if (etsdhp.ptr == NULL || (etsdhp.val.flags & SAM_FRZI_NOBLOCK)) {
		/*
		 * Don't block.  Either thread has no context (worker thread?
		 * ioctl?) or thread is set non-blocking.
		 */
		if (mp->mt.fi_status & (FS_LOCK_HARD | FS_UMOUNT_IN_PROGRESS)) {
			if (curthread->t_flag & T_DONTBLOCK) {
				curthread->t_flag |= T_WOULDBLOCK;
				return (EAGAIN);
			}
		}
		if (!(mp->mt.fi_status & (FS_MOUNTED | FS_MOUNTING))) {
			return (EIO);
		}
		return (0);
	}

	SAM_DEC_OPERATION(mp);
	while (mp->mt.fi_status & (FS_LOCK_HARD | FS_UMOUNT_IN_PROGRESS)) {
		RW_UNLOCK_OS(&ip->inode_rwl, lock_type);
		if ((error = sam_freeze_ino(mp, ip, 0)) != 0) {
			RW_LOCK_OS(&ip->inode_rwl, lock_type);
			SAM_INC_OPERATION(mp);
			return (error);
		}
		RW_LOCK_OS(&ip->inode_rwl, lock_type);
	}
	SAM_INC_OPERATION(mp);

	if (!(mp->mt.fi_status & (FS_MOUNTED | FS_MOUNTING))) {
		return (EIO);
	}
	return (0);
}


/*
 * -----    sam_open_operation_nb_unless_idle -
 *
 * If this is the thread's initial entry into the filesystem, it checks
 * if the filesystem is supposed to be idle. If the filesystem is intended
 * to be idle, then this returns EAGAIN.
 * Otherwise, it calls sam_open_operation_nb() marking the start of operation.
 * Note, this will never block.
 */
int
sam_open_operation_nb_unless_idle(sam_node_t *ip)
{
	sam_operation_t ep;
	sam_mount_t *mp = ip->mp;


	ep.ptr = tsd_get(mp->ms.m_tsd_key);

	if (ep.ptr == NULL) {
		if (mp->mt.fi_status & (FS_LOCK_HARD | FS_UMOUNT_IN_PROGRESS)) {
			return (EAGAIN);
		}
	}

	sam_open_operation_nb(mp);

	return (0);
}
#endif	/* sun */


#ifdef linux
/*
 * -----	sam_open_operation
 *
 * Increment the operation activity count.
 */
int
sam_open_operation(sam_node_t *ip)
{
	sam_mount_t *mp = ip->mp;
	int error;

	while (mp->mt.fi_status & (FS_LOCK_HARD | FS_UMOUNT_IN_PROGRESS)) {
		if ((error = sam_freeze_ino(mp, ip, 0)) != 0) {
			return (error);
		}
	}

	/*
	 * Did pending unmount complete?
	 */
	if (!(mp->mt.fi_status & (FS_MOUNTED | FS_MOUNTING))) {
		return (EIO);
	}
	SAM_INC_OPERATION(mp);
	return (0);
}


/*
 * -----    sam_idle_operation -
 * Check for any pending failovers/umounts
 */
int
sam_idle_operation(sam_node_t *ip)
{
	int error;

	SAM_DEC_OPERATION(ip->mp);
	if ((error = sam_open_operation(ip)) == 0) {
		SAM_INC_OPERATION(ip->mp);
	}
	return (error);
}
#endif	/* linux */


/*
 *	----	sam_open_operation_nb
 *
 * Similar to sam_open_operation(), but doesn't block or return error.
 * Accepts a mount pointer, rather than an inode pointer; we only need
 * the inode pointer if we want to block.
 */
#ifdef sun
void
sam_open_operation_nb(sam_mount_t *mp)
{
	sam_operation_t ep;

	ep.ptr = tsd_get(mp->ms.m_tsd_key);
	if (ep.ptr) {
		/*
		 * For now, assume attributes of original caller to
		 * sam_open_operation, rather than overriding them (by setting
		 * SAM_FRZI_NOBLOCK).
		 */
		ASSERT(ep.val.depth > 0);
		ASSERT(mp->ms.m_cl_active_ops > 0);
		ep.val.depth++;
		tsd_set(mp->ms.m_tsd_key, ep.ptr);
	} else {
		ep.val.depth = 1;
		ep.val.flags = SAM_FRZI_NOBLOCK;
		ep.val.module = mp->ms.m_tsd_key;
		SAM_INC_OPERATION(mp);
		tsd_set(mp->ms.m_tsd_key, ep.ptr);
	}
}


void
sam_open_operation_rwl(sam_node_t *ip)
{
	sam_mount_t *mp = ip->mp;
	sam_operation_t ep;

	ASSERT(S_ISDIR(ip->di.mode));

	ep.ptr = tsd_get(mp->ms.m_tsd_key);

	if (ep.ptr) {
		/*
		 * This operation must be made non-blocking since
		 * it will be acquiring a lock.
		 */
		ASSERT(ep.val.depth > 0);
		ASSERT(mp->ms.m_cl_active_ops > 0);
		ep.val.depth++;
		ep.val.flags |= (SAM_FRZI_NOBLOCK | SAM_FRZI_DRWLOCK);
		tsd_set(mp->ms.m_tsd_key, ep.ptr);
		return;
	}

	/*
	 * Freeze this thread here before allowing it to
	 * continue non-blocking holding a lock (SAM_FRZI_DRWLOCK).
	 */
	while (mp->mt.fi_status & (FS_LOCK_HARD | FS_UMOUNT_IN_PROGRESS)) {
		/*
		 * Do not freeze share fs thread or fsflush.
		 */
		if (IS_SHAREFS_THREAD_OS || sam_is_fsflush()) {
			break;
		}
		(void) sam_freeze_ino(mp, ip, 1);
	}

	ep.val.depth = 1;
	ep.val.flags = SAM_FRZI_NOBLOCK | SAM_FRZI_DRWLOCK;
	ep.val.module = mp->ms.m_tsd_key;
	SAM_INC_OPERATION(mp);
	tsd_set(mp->ms.m_tsd_key, ep.ptr);
}


/*
 *	----	sam_unset_operation_nb
 *
 * Removes the SAM_FRZI_NOBLOCK flag from the current operation.
 */
void
sam_unset_operation_nb(sam_mount_t *mp)
{
	sam_operation_t ep;

	ep.ptr = tsd_get(mp->ms.m_tsd_key);
	if (ep.ptr) {
		ASSERT(ep.val.depth > 0);
		ASSERT(mp->ms.m_cl_active_ops > 0);
		ep.val.flags &= ~SAM_FRZI_NOBLOCK;
		tsd_set(mp->ms.m_tsd_key, ep.ptr);
	}
}


/*
 * -----    sam_open_mutex_operation -
 *
 * Used instead of mutex_enter() directly.
 * Allows a thread to sleep waiting for the mutex
 * and not hold up failover.
 */
void
sam_open_mutex_operation(sam_node_t *ip, kmutex_t *mup)
{
	sam_mount_t *mp = ip->mp;

	if (SAM_IS_SHARED_CLIENT(mp)) {
#ifdef DEBUG
		sam_operation_t ep;
		ep.ptr = tsd_get(mp->ms.m_tsd_key);
		ASSERT(ep.ptr != NULL);
#endif

		if (!mutex_tryenter(mup)) {
			/*
			 * Can't get the mutex so drop m_cl_active_ops
			 * while we wait, this prevents failover from hanging
			 * should failover occur while we wait.
			 */
			SAM_DEC_OPERATION(mp);
			mutex_enter(mup);

			/*
			 * Before incrementing m_cl_active_ops
			 * check for failover. Failover may already
			 * be proceeding with m_cl_active_ops of zero.
			 */
			while (mp->mt.fi_status &
			    (FS_LOCK_HARD | FS_UMOUNT_IN_PROGRESS)) {
				/*
				 * If failover started while we acquired
				 * the mutex freeze here.
				 */
				(void) sam_freeze_ino(mp, ip, 1);
			}
			SAM_INC_OPERATION(mp);
		}

	} else {

		mutex_enter(mup);
	}
}
#endif	/* sun */


#ifdef linux
void
sam_open_operation_nb(sam_mount_t *mp)
{
	SAM_INC_OPERATION(mp);
}
#endif	/* linux */


/*
 * -----	sam_is_fsflush -
 * Return true if this thread is fsflush.
 */

boolean_t			/* TRUE if fsflush, FALSE if not fsflush */
sam_is_fsflush()
{
#ifdef sun
	if (curproc == proc_fsflush) {
		return (TRUE);
	}
	if (tsd_get(samgt.tsd_fsflush_key)) {
		return (TRUE);
	}
#endif /* sun */
#ifdef linux
	int slen;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	if (current_is_pdflush()) {
		return (TRUE);
	}
#endif /* KERNEL VERSION >= 2.6.0 */

	slen = strlen(current->comm);
	if (slen != strlen(statfs_thread_name)) {
		return (FALSE);
	}
	if (strncmp(current->comm, statfs_thread_name, slen) == 0) {
		return (TRUE);
	}
#endif /* linux */
	return (FALSE);
}
