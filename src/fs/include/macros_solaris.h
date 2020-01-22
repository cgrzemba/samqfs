/*
 *	macros_solaris.h - SAM-QFS macros.
 *
 *	Description:
 *		define SAM-QFS macros for Solaris.
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

#ifndef	_SAM_MACROS_SOLARIS_H
#define	_SAM_MACROS_SOLARIS_H

#ifdef sun
#pragma ident "$Revision: 1.36 $"
#endif

#include <sys/types.h>

/*
 * ----- read/write locks
 */

#define	RW_LOCK_INIT_OS(rwl, a2, a3, a4)	rw_init(rwl, a2, a3, a4)
#define	RW_LOCK_DESTROY_OS(rwl)			rw_destroy(rwl)

#define	RW_LOCK_OS(rwl, t)		rw_enter(rwl, t)
#define	RW_UNLOCK_OS(rwl, t)		rw_exit(rwl)
#define	RW_UNLOCK_CURRENT_OS(rwl)	rw_exit(rwl)
#define	RW_OWNER_OS(rwl)		rw_owner(rwl)
#define	RW_TRYUPGRADE_OS(rwl)		rw_tryupgrade(rwl)
#define	RW_DOWNGRADE_OS(rwl)		rw_downgrade(rwl)
#define	RW_WRITE_HELD_OS(rwl)		rw_write_held(rwl)
#define	RW_TRYENTER_OS(rwl, t)		rw_tryenter(rwl, t)


/*
 * ----- Macros to upgrade READERS lock to WRITERS lock.
 */
#define	RW_UPGRADE_OS(rwl)	if (!RW_TRYUPGRADE_OS(rwl)) {	\
		RW_UNLOCK_OS(rwl, RW_READER);			\
		RW_LOCK_OS(rwl, RW_WRITER);			\
	}


/*
 * ----- General interface to current time in seconds.
 */

#define	SAM_SECOND() gethrestime_sec()

/*
 * ----- General interface to hrestime (timespec_t).
 */
#define	SAM_HRESTIME(a) gethrestime((a))


/*
 * ----- Check if the calling thread is one of the sam_sharefs_threads
 */
#define	IS_SHAREFS_THREAD_OS \
	((uintptr_t)curthread->t_startpc == (uintptr_t)sam_sharefs_thread)


/*
 * ----- General test if caller is NFS.
 */

#define		SAM_THREAD_IS_NFS() \
	(curthread->t_flag & T_DONTPEND)

/*
 * ----- General test if caller is NFS and nonfssync is set.
 */
#define		SAM_FORCE_NFS_ASYNC(mp) \
	(SAM_THREAD_IS_NFS() && (mp->mt.fi_config & MT_NFSASYNC))


/*
 * ----- Test for unshared samfs vnode.
 * Checks both vfsp type mount flags for single mount only. Returns true if
 * NOT a single mounted (unshared) samfs file system.
 */

#define	SAM_VNODE_IS_NOT_SINGLE_SAMFS(vp) \
	(((vp)->v_vfsp->vfs_fstype != samgt.fstype) ||	\
		(((sam_mount_t *)(void *)		\
		((vp)->v_vfsp->vfs_data))->mt.fi_config1 & MC_SHARED_FS))


/*
 * ----- General test for samfs vnode.
 * Checks vfsp type. Returns true if NOT a samfs file system.
 */

#define	SAM_VNODE_IS_NOTSAMFS(vp) \
	(((vp)->v_vfsp == NULL) || !((vp)->v_vfsp->vfs_fstype == samgt.fstype))


/*
 * ----- Set EJUKEBOX NFS error correct return.
 */

#define	SAM_NFS_JUKEBOX_ERROR(error)


/*
 * ----- Increment/decrement FS mount point reference
 * counts.  (One reference per in-core FS inode.)
 */
#define	SAM_VFS_HOLD(mp)		VFS_HOLD((mp)->mi.m_vfsp)
#define	SAM_VFS_RELE(mp)		VFS_RELE((mp)->mi.m_vfsp)

/*
 * Track the number of rdsock threads in the FS.  We keep a
 * VFS hold on the FS whenever the count is non-zero.
 */
#define	SAM_SHARE_DAEMON_HOLD_FS(mp) sam_share_daemon_hold_fs(mp)
#define	SAM_SHARE_DAEMON_RELE_FS(mp) sam_share_daemon_rele_fs(mp)
#define	SAM_SHARE_DAEMON_CNVT_FS(mp) sam_share_daemon_cnvt_fs(mp)

#define	SAM_VP_IS_STALE(vp) ((vp)->v_vfsp->vfs_flag & VFS_UNMOUNTED)


/*
 * ----- Lease requirement flag operations.
 * This is used to flag threads that get into the client getpage
 * path that do not need READ or WRITE lease.
 */
#define	SAM_GET_LEASEFLG(ip)					\
	tsd_get(ip->mp->ms.m_tsd_leasekey)

#define	SAM_SET_LEASEFLG(ip)					\
	(void) tsd_set(ip->mp->ms.m_tsd_leasekey, ip)

#define	SAM_CLEAR_LEASEFLG(ip)					\
	(void) tsd_set(ip->mp->ms.m_tsd_leasekey, NULL)

/*
 * ----- Decrement operation activity count.
 */
#define	SAM_INC_OPERATION(mp) {				\
	atomic_add_32(&(mp)->ms.m_cl_active_ops, 1);	\
	ASSERT((mp)->ms.m_cl_active_ops > 0);		\
}

#define	SAM_DEC_OPERATION(mp) {				\
	ASSERT((mp)->ms.m_cl_active_ops > 0);		\
	atomic_add_32(&(mp)->ms.m_cl_active_ops, -1);	\
}

#ifdef DEBUG
#define	SAM_CLOSE_OPERATION(mp, error) {		\
	sam_operation_t ep; 				\
	int r;						\
	ep.ptr = tsd_get((mp)->ms.m_tsd_key);		\
	ASSERT(ep.ptr);					\
	ASSERT(ep.val.depth > 0);			\
	ASSERT(ep.val.module == (mp)->ms.m_tsd_key);	\
	ASSERT((mp)->ms.m_cl_active_ops > 0);		\
	if (--ep.val.depth == 0) {			\
		SAM_DEC_OPERATION(mp);			\
		ep.ptr = NULL;				\
	}						\
	r = tsd_set((mp)->ms.m_tsd_key, ep.ptr);	\
	ASSERT(r == 0);					\
	SAM_NFS_JUKEBOX_ERROR(error);			\
}
#define	SAM_CLOSE_VFS_OPERATION(mp, error) {		\
	sam_operation_t ep;				\
	int r;						\
	ep.ptr = tsd_get((mp)->ms.m_tsd_key);		\
	ASSERT(ep.ptr);					\
	ASSERT(ep.val.depth > 0);			\
	ASSERT(ep.val.module == (mp)->ms.m_tsd_key);	\
	if (--ep.val.depth == 0) {			\
		SAM_DEC_OPERATION(mp);			\
		ep.ptr = NULL;				\
	}						\
	r = tsd_set((mp)->ms.m_tsd_key, ep.ptr);	\
	ASSERT(r == 0);					\
}
#else	/* DEBUG */
#define	SAM_CLOSE_OPERATION(mp, error) {		\
	sam_operation_t ep;				\
	ep.ptr = tsd_get((mp)->ms.m_tsd_key);		\
	if (--ep.val.depth == 0) {			\
		SAM_DEC_OPERATION(mp);			\
		ep.ptr = NULL;				\
	}						\
	tsd_set((mp)->ms.m_tsd_key, ep.ptr);		\
	SAM_NFS_JUKEBOX_ERROR(error);			\
}
#define	SAM_CLOSE_VFS_OPERATION(mp, error) {		\
	sam_operation_t ep;				\
	ep.ptr = tsd_get((mp)->ms.m_tsd_key);		\
	if (ep.ptr) {					\
		if (--ep.val.depth == 0) {		\
			SAM_DEC_OPERATION(mp);		\
			ep.ptr = NULL;			\
		}					\
		(void) tsd_set((mp)->ms.m_tsd_key, NULL);	\
	} else {						\
		cmn_err(CE_WARN, "SAM-QFS: %s: NULL VFS tsd",	\
			(mp)->mt.fi_name);			\
	}							\
}
#endif	/* DEBUG */

#define	THREAD_KILL_OS(t)	psignal(ttoproc(t), SIGKILL);

/*
 * ----- Macros to get address of indirect extent
 */
#define	ie_ptr(mapp)	(sam_indirect_extent_t *)(void *)mapp->bp->b_un.b_addr


/*
 * ----- Trace macros for vnode
 */
#define	SAM_ITOP(ip)	SAM_ITOV(ip)


/*
 * ----- Current process id
 */
#define	SAM_CUR_PID		(curproc->p_pid)


/*
 * ----- Hold and Release inode macros.
 */
#define	VN_HOLD_OS(ip)		VN_HOLD(SAM_ITOV(ip))
#define	VN_RELE_OS(ip)		VN_RELE(SAM_ITOV(ip))

#define	MOD_INC_USE_COUNT
#define	MOD_DEC_USE_COUNT


/*
 * ----- Macros to use instead of ifdefs in places of common code between OSes
 */

#define	SAM_SET_ABR(a)		sam_set_abr(a)

/*
 * 64-bit kernel mutexes must be 8-byte aligned for atomicity.
 */
#ifdef _LP64
#define	sam_mutex_init(sem, a2, a3, a4) {				\
	ASSERT(!((uint64_t)sem & 0x7));					\
	mutex_init(sem, a2, a3, a4);					\
}
#else  /* _LP64 */
#define	sam_mutex_init(sem, a2, a3, a4) {				\
	mutex_init(sem, a2, a3, a4);					\
}
#endif /* _LP64 */

#define	local_to_solaris_errno(x)	(x)
#define	solaris_to_local_errno(x)	(x)

#if defined(SOL_511_ABOVE)
/*
 * ----- Solaris 11 vnode operations.
 */
#define	VOP_OPEN_OS(vpp, mode, cr, ct) \
	VOP_OPEN(vpp, mode, cr, ct)
#define	VOP_CLOSE_OS(vp, f, c, o, cr, ct) \
	VOP_CLOSE(vp, f, c, o, cr, ct)
#define	VOP_READ_OS(vp, uiop, iof, cr, ct) \
	VOP_READ(vp, uiop, iof, cr, ct)
#define	VOP_WRITE_OS(vp, uiop, iof, cr, ct) \
	VOP_WRITE(vp, uiop, iof, cr, ct)
#define	VOP_IOCTL_OS(vp, cmd, a, f, cr, rvp, ct) \
	VOP_IOCTL(vp, cmd, a, f, cr, rvp, ct)
#define	VOP_GETATTR_OS(vp, vap, f, cr, ct) \
	VOP_GETATTR(vp, vap, f, cr, ct)
#define	VOP_RWLOCK_OS(vp, w, ct) \
	VOP_RWLOCK(vp, w, ct)
#define	VOP_RWUNLOCK_OS(vp, w, ct) \
	VOP_RWUNLOCK(vp, w, ct)
#define	VOP_REALVP_OS(vp1, vp2, ct) \
	VOP_REALVP(vp1, vp2, ct)
#define	VOP_PUTPAGE_OS(vp, of, sz, fl, cr, ct) \
	VOP_PUTPAGE(vp, of, sz, fl, cr, ct)
/*
 * ----- Solaris 11 vnevent notifications.
 */
#define	VNEVENT_REMOVE_OS(rmvp, pvp, cp, ct) \
	vnevent_remove(rmvp, pvp, cp, ct)
#define	VNEVENT_RMDIR_OS(rmvp, pvp, cp, ct) \
	vnevent_rmdir(rmvp, pvp, cp, ct)
#define	VNEVENT_RENAME_SRC_OS(ovp, opvp, onm, ct) \
	vnevent_rename_src(ovp, opvp, onm, ct)
#define	VNEVENT_RENAME_DEST_OS(nvp, npvp, nnm, ct) \
	vnevent_rename_dest(nvp, npvp, nnm, ct)
#if defined(_INOTIFY)
#define	VNEVENT_RENAME_DEST_DIR_OS(vp, ct) \
	vnevent_rename_dest_dir(vp, vp, "", ct)
#else
#define	VNEVENT_RENAME_DEST_DIR_OS(vp, ct) \
	vnevent_rename_dest_dir(vp, ct)
#endif
#define	VNEVENT_CREATE_OS(vp, ct) \
	vnevent_create(vp, ct)
#define	VNEVENT_LINK_OS(vp, ct) \
	vnevent_link(vp, ct)
/*
 * No need to implement vnevent_mountedover() since it is
 * called from vfs.c domount(), which calls samfs_mount().
 */
#if (0)
#define	VNEVENT_MOUNTEDOVER_OS(vp, ct) \
	vnevent_mountedover(vp, ct)
#endif
/*
 * ----- Solaris 11 filesystem utilities.
 */
#define	FS_FRLOCK_OS(vp, cmd, flp, filemode, offset, fcb, credp, ct) \
	fs_frlock(vp, cmd, flp, filemode, offset, fcb, credp, ct)
#define	FS_PATHCONF_OS(vp, cmd_a, valp, credp, ct) \
	fs_pathconf(vp, cmd_a, valp, credp, ct)

#else
/*
 * ----- Solaris 10 vnode operations.
 */
#define	VOP_OPEN_OS(vpp, mode, cr, ct) \
	VOP_OPEN(vpp, mode, cr)
#define	VOP_CLOSE_OS(vp, f, c, o, cr, ct) \
	VOP_CLOSE(vp, f, c, o, cr)
#define	VOP_READ_OS(vp, uiop, iof, cr, ct) \
	VOP_READ(vp, uiop, iof, cr, ct)
#define	VOP_WRITE_OS(vp, uiop, iof, cr, ct) \
	VOP_WRITE(vp, uiop, iof, cr, ct)
#define	VOP_IOCTL_OS(vp, cmd, a, f, cr, rvp, ct) \
	VOP_IOCTL(vp, cmd, a, f, cr, rvp)
#define	VOP_GETATTR_OS(vp, vap, f, cr, ct) \
	VOP_GETATTR(vp, vap, f, cr)
#define	VOP_RWLOCK_OS(vp, w, ct) \
	VOP_RWLOCK(vp, w, ct)
#define	VOP_RWUNLOCK_OS(vp, w, ct) \
	VOP_RWUNLOCK(vp, w, ct)
#define	VOP_REALVP_OS(vp1, vp2, ct) \
	VOP_REALVP(vp1, vp2)
#define	VOP_PUTPAGE_OS(vp, of, sz, fl, cr, ct) \
	VOP_PUTPAGE(vp, of, sz, fl, cr)
/*
 * ----- Solaris 10 vnevent notifications.
 */
#define	VNEVENT_REMOVE_OS(rmvp, pvp, cp, ct) \
	vnevent_remove(rmvp)
#define	VNEVENT_RMDIR_OS(rmvp, pvp, cp, ct) \
	vnevent_rmdir(rmvp)
#define	VNEVENT_RENAME_SRC_OS(ovp, opvp, onm, ct) \
	vnevent_rename_src(ovp)
#define	VNEVENT_RENAME_DEST_OS(nvp, npvp, nnm, ct) \
	vnevent_rename_dest(nvp)
#define	VNEVENT_RENAME_DEST_DIR_OS(vp, ct) \
	do { } while (0)
#define	VNEVENT_CREATE_OS(vp, ct) \
	do { } while (0)
#define	VNEVENT_LINK_OS(vp, ct) \
	do { } while (0)
/*
 * ----- Solaris 10 filesystem utilities.
 */
#define	FS_FRLOCK_OS(vp, cmd, flp, filemode, offset, fcb, credp, ct) \
	fs_frlock(vp, cmd, flp, filemode, offset, fcb, credp)
#define	FS_PATHCONF_OS(vp, cmd_a, valp, credp, ct) \
	fs_pathconf(vp, cmd_a, valp, credp)

#endif	/* vnode operations */

#endif /* _SAM_MACROS_SOLARIS_H */
