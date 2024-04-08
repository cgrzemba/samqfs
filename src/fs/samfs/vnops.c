/*
 * ----- sam/vnops.c - Process the vnode functions.
 *
 * Processes the vnode functions supported on the SAM File System.
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

#pragma ident "$Revision: 1.162 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/dirent.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/fs_subr.h>
#include <sys/flock.h>
#include <sys/systm.h>
#include <sys/ioccom.h>
#include <sys/vmsystm.h>
#include <sys/file.h>
#include <sys/rwlock.h>
#include <sys/share.h>

#include <nfs/nfs.h>

#include <vm/as.h>
#include <vm/seg_vn.h>

#include <sys/policy.h>

#if defined(SOL_511_ABOVE)
#include <sys/vfs_opreg.h>
#endif

/* ----- SAMFS Includes */

#include "sam/param.h"
#include "sam/types.h"
#include "sam/fioctl.h"
#include "sam/samaio.h"
#include "sam/samevent.h"

#include "samfs.h"
#include "ino.h"
#include "macros.h"
#include "inode.h"
#include "mount.h"
#include "arfind.h"
#include "rwio.h"
#include "extern.h"
#include "debug.h"
#include "trace.h"
#include "qfs_log.h"
#include "objctl.h"


/*
 * ----	sam_EIO
 *
 * Solaris 10 and above use a flexible method for creating
 * vnode operations tables and don't require fully matched
 * arg lists, allowing the use of sam_EIO for essentially
 * all unused vnode functions.
 *
 * Return EIO.  Used for vnops entries where the underlying
 * VP is stale due to forced unmount of the underlying FS.
 */
int
sam_EIO(void)
{
	return (EIO);
}


/* ----- Vnode operations supported on the samfs file system. */

struct vnodeops *samfs_vnodeopsp;

const fs_operation_def_t samfs_vnodeops_template[] = {
	VOPNAME_OPEN, sam_open_vn,	/* will not be blocked by lockfs */
	VOPNAME_CLOSE, sam_close_vn,	/* will not be blocked by lockfs */
	VOPNAME_READ, sam_read_vn,
	VOPNAME_WRITE, sam_write_vn,
	VOPNAME_IOCTL, sam_ioctl_vn,
#if (0)
	VOPNAME_SETFL, ufs_setfl_vn,
#endif
	VOPNAME_GETATTR, sam_getattr_vn,
	VOPNAME_SETATTR, sam_setattr_vn,
	VOPNAME_ACCESS, sam_access_vn,
	VOPNAME_LOOKUP, sam_lookup_vn,
	VOPNAME_CREATE, sam_create_vn,
	VOPNAME_REMOVE, sam_remove_vn,
	VOPNAME_LINK, sam_link_vn,
	VOPNAME_RENAME, sam_rename_vn,
	VOPNAME_MKDIR, sam_mkdir_vn,
	VOPNAME_RMDIR, sam_rmdir_vn,
	VOPNAME_READDIR, sam_readdir_vn,
	VOPNAME_SYMLINK, sam_symlink_vn,
	VOPNAME_READLINK, sam_readlink_vn,
	VOPNAME_FSYNC, sam_fsync_vn,
	VOPNAME_INACTIVE, (fs_generic_func_p) sam_inactive_vn, /* not blocked */
	VOPNAME_FID, sam_fid_vn,
	VOPNAME_RWLOCK, (fs_generic_func_p) sam_rwlock_vn, /* not blocked */
	VOPNAME_RWUNLOCK, (fs_generic_func_p) sam_rwunlock_vn, /* not blocked */
	VOPNAME_SEEK, sam_seek_vn,
#if (0)
	VOPNAME_CMP, ufs_cmp_vn,
#endif
	VOPNAME_FRLOCK, sam_frlock_vn,
	VOPNAME_SPACE, sam_space_vn,
#if (0)
	VOPNAME_REALVP, ufs_realvp_vn,
#endif
	VOPNAME_GETPAGE, sam_getpage_vn,
	VOPNAME_PUTPAGE, sam_putpage_vn,
	VOPNAME_MAP, (fs_generic_func_p) sam_map_vn,
	VOPNAME_ADDMAP, (fs_generic_func_p) sam_addmap_vn, /* not blocked */
	VOPNAME_DELMAP, sam_delmap_vn,	/* will not be blocked by lockfs */
	VOPNAME_POLL, (fs_generic_func_p) fs_poll,	/* not blocked */
#if (0)
	VOPNAME_DUMP, ufs_dump_vn,
#endif
	VOPNAME_PATHCONF, sam_pathconf_vn,
#if (0)
	VOPNAME_PAGEIO, ufs_pageio_vn,
	VOPNAME_DUMPCTL, ufs_dumpctl_vn,
	VOPNAME_DISPOSE, ufs_dispose_vn,
#endif
	VOPNAME_GETSECATTR, sam_getsecattr_vn,
	VOPNAME_SETSECATTR, sam_setsecattr_vn,
#if (0)
	VOPNAME_SHRLOCK, ufs_shrlock_vn,
#endif
	VOPNAME_VNEVENT, fs_vnevent_support,
	NULL, NULL
};


struct vnodeops *samfs_vnode_staleopsp;

const fs_operation_def_t samfs_vnode_staleops_template[] = {
	VOPNAME_OPEN, sam_EIO,
	VOPNAME_CLOSE, sam_close_stale_vn,
	VOPNAME_READ, sam_EIO,
	VOPNAME_WRITE, sam_EIO,
	VOPNAME_IOCTL, sam_EIO,
#if (0)
	VOPNAME_SETFL, ufs_setfl_vn,
#endif
	VOPNAME_GETATTR, sam_EIO,
	VOPNAME_SETATTR, sam_EIO,
	VOPNAME_ACCESS, sam_EIO,
	VOPNAME_LOOKUP, sam_EIO,
	VOPNAME_CREATE, sam_EIO,
	VOPNAME_REMOVE, sam_EIO,
	VOPNAME_LINK, sam_EIO,
	VOPNAME_RENAME, sam_EIO,
	VOPNAME_MKDIR, sam_EIO,
	VOPNAME_RMDIR, sam_EIO,
	VOPNAME_READDIR, sam_EIO,
	VOPNAME_SYMLINK, sam_EIO,
	VOPNAME_READLINK, sam_EIO,
	VOPNAME_FSYNC, sam_EIO,
	VOPNAME_INACTIVE, (fs_generic_func_p) sam_inactive_stale_vn,
	VOPNAME_FID, sam_EIO,
	VOPNAME_RWLOCK, (fs_generic_func_p) sam_rwlock_vn,
	VOPNAME_RWUNLOCK, (fs_generic_func_p) sam_rwunlock_vn,
	VOPNAME_SEEK, sam_EIO,
#if (0)
	VOPNAME_CMP, ufs_cmp_vn,
#endif
	VOPNAME_FRLOCK, sam_EIO,
	VOPNAME_SPACE, sam_EIO,
#if (0)
	VOPNAME_REALVP, ufs_realvp_vn,
#endif
	VOPNAME_GETPAGE, sam_EIO,
	VOPNAME_PUTPAGE, sam_putpage_vn,
	VOPNAME_MAP, sam_EIO,
	VOPNAME_ADDMAP, sam_EIO,
	VOPNAME_DELMAP, sam_delmap_stale_vn,
	VOPNAME_POLL, (fs_generic_func_p) fs_poll,
#if (0)
	VOPNAME_DUMP, ufs_dump_vn,
#endif
	VOPNAME_PATHCONF, sam_EIO,
#if (0)
	VOPNAME_PAGEIO, ufs_pageio_vn,
	VOPNAME_DUMPCTL, ufs_dumpctl_vn,
	VOPNAME_DISPOSE, ufs_dispose_vn,
#endif
	VOPNAME_GETSECATTR, sam_EIO,
	VOPNAME_SETSECATTR, sam_EIO,
#if (0)
	VOPNAME_SHRLOCK, ufs_shrlock_vn,
	VOPNAME_VNEVENT, fs_vnevent_support,
#endif
	NULL, NULL
};


/*
 * ----- sam_open_vn - Open a SAM-FS file.
 * Opens a SAM-FS file.
 */

/* ARGSUSED3 */
int				/* ERRNO if error, 0 if successful. */
sam_open_vn(
#if defined(SOL_511_ABOVE)
	vnode_t **vpp,		/* pointer pointer to vnode */
	int filemode,		/* open file mode */
	cred_t *credp,		/* credentials pointer */
	caller_context_t *ct	/* caller context pointer */
#else
	vnode_t **vpp,		/* pointer pointer to vnode */
	int filemode,		/* open file mode */
	cred_t *credp		/* credentials pointer */
#endif
)
{
	sam_node_t *ip;
	int error = 0;
	vnode_t *vp = *vpp;
	proc_t *p = ttoproc(curthread);

	ip = SAM_VTOI(vp);
	TRACE(T_SAM_OPEN, vp, ip->di.id.ino, filemode, ip->di.status.bits);

	RW_LOCK_OS(&ip->inode_rwl, RW_READER);

	if (ip->di.id.ino == SAM_INO_INO) {
		error = EPERM;
	} else if (ip->di.status.b.damaged) {
		error = ENODATA;		/* best fit errno */
	} else if (S_ISREQ(ip->di.mode)) {	/* If removable media file */
		if (!rw_tryupgrade(&ip->inode_rwl)) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
		error = sam_open_rm(ip, filemode, credp);
		rw_downgrade(&ip->inode_rwl);
	} else if (SAM_IS_SHARED_READER(ip->mp) &&
	    (ip->di.id.ino != SAM_INO_INO)) {
		if ((error =
		    sam_refresh_shared_reader_ino(ip, FALSE, credp)) == 0) {
			if (ip->di.status.b.offline) {
				error = EROFS;
			}
		}
	}
	if (error) {
		goto out;
	}

	/*
	 * The default mode disallows archiving while the file is opened for
	 * write. If the attribute, concurrent_archive (archive -c) is set,
	 * the file is archived while opened for write. Used by databases.
	 */
	if ((ip->di.status.b.concurrent_archive == 0) && (error == 0) &&
	    (filemode & (FCREAT | FTRUNC | FWRITE))) {
		mutex_enter(&ip->fl_mutex);
		ip->flags.b.write_mode = 1;	/* File opened for write */
		mutex_exit(&ip->fl_mutex);
	}

	/*
	 * If exec'ed program, don't increment count because there is no close.
	 */
	if (!(p->p_proc_flag & P_PR_EXEC)) {
		mutex_enter(&ip->fl_mutex);
		ip->no_opens++;
		mutex_exit(&ip->fl_mutex);
	}

out:
	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	TRACE(T_SAM_OPEN_RET, vp, (sam_tr_t)ip->di.rm.size,
	    ip->flags.bits, error);
	return (error);
}


/*
 * ----- sam_setattr_vn - Set attributes call.
 * Set attributes for a SAM-FS file.
 */

/* ARGSUSED4 */
int				/* ERRNO if error, 0 if successful. */
sam_setattr_vn(
	vnode_t *vp,		/* pointer to vnode. */
	vattr_t *vap,		/* vattr pointer. */
	int flags,		/* flags. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct)	/* caller context pointer. */
{
	sam_node_t *ip;
	int error;
	int issync;
	int trans_size;
	int dotrans = 0;

	TRACE(T_SAM_SETATTR, vp, (sam_tr_t)vap, flags, vap->va_mask);
	ip = SAM_VTOI(vp);

	/*
	 * Start LQFS transaction
	 */
	trans_size = (int)TOP_SETATTR_SIZE(ip);
	TRANS_BEGIN_CSYNC(ip->mp, issync, TOP_SETATTR, trans_size);
	dotrans++;
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);

	error = sam_setattr_ino(ip, vap, flags, credp);

	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	if (dotrans) {
		int terr = 0;

		TRANS_END_CSYNC(ip->mp, terr, issync, TOP_SETATTR, trans_size);
		if (error == 0) {
			error = terr;
		}
	}

	TRACE(T_SAM_SETATTR_RET, vp, (sam_tr_t)vap, error, 0);
	return (error);
}


/*
 * ----- sam_lookup_vn - Lookup a SAM-FS file.
 * Given a parent directory and pathname component, lookup
 * and return the vnode found. If found, vnode is "held".
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
sam_lookup_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *pvp,		/* Pointer to parent directory vnode */
	char *cp,		/* Pointer to the component name. */
	vnode_t **vpp,		/* Pointer pointer to the vnode (returned). */
	pathname_t *pnp,	/* Pointer to the pathname. */
	int flags,		/* Flags. */
	vnode_t *rdir,		/* Pointer to root directory vnode. */
	cred_t *credp,		/* Credentials pointer. */
	caller_context_t *ct,	/* Caller context pointer. */
	int *defp,		/* Returned per-dirent flags. */
	struct pathname *rpnp	/* Returned case-preserved name in directory. */
#else
	vnode_t *pvp,		/* Pointer to parent directory vnode */
	char *cp,		/* Pointer to the component name. */
	vnode_t **vpp,		/* Pointer pointer to the vnode (returned). */
	pathname_t *pnp,	/* Pointer to the pathname. */
	int flags,		/* Flags. */
	vnode_t *rdir,		/* Pointer to root directory vnode. */
	cred_t *credp		/* Credentials pointer. */
#endif
)
{
	int error;
	sam_node_t *pip;
	sam_node_t *ip;

	TRACES(T_SAM_LOOKUP, pvp, cp);

	if (flags & LOOKUP_XATTR) {
		return (sam_lookup_xattr(pvp, vpp, flags, credp));
	}

	pip = SAM_VTOI(pvp);

	/*
	 * If we are at root AND we are implementing objects on the Storage
	 * Node AND the component name is '.objects', we give it the special
	 * Gpfs vnode that we use to display objects on the Storage Node.
	 *
	 * This is a special trap to enable us to provide "pathname" for
	 * objects implemented by SAMQFS.
	 *
	 */
	if (pip->mp->mt.fi_type == DT_META_OBJ_TGT_SET) {
		if ((pvp == pip->mp->mi.m_vn_root) &&
		    pip->mp->mi.m_vn_objctl && (strcmp(cp, OBJCTL_NAME) == 0)) {
			VN_HOLD(pip->mp->mi.m_vn_objctl);
			*vpp = pip->mp->mi.m_vn_objctl;
			return (0);
		}
	}

	if ((error = sam_lookup_name(pip, cp, &ip, NULL, credp)) == 0) {
		*vpp = SAM_ITOV(ip);
		if (((ip->di.rm.ui.flags & RM_CHAR_DEV_FILE) &&
		    S_ISREG(ip->di.mode)) ||
		    SAM_ISVDEV((*vpp)->v_type)) {

			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			/*
			 * Repeat the test under lock.
			 * Don't need the overhead of always grabbing
			 * inode_rwl, RW_WRITER for a rare condition.
			 *
			 */
			if (((ip->di.rm.ui.flags & RM_CHAR_DEV_FILE) &&
			    S_ISREG(ip->di.mode)) ||
			    SAM_ISVDEV((*vpp)->v_type)) {
				vnode_t *vp;

				if ((error =
				    sam_get_special_file(ip, &vp,
				    credp)) == 0) {
					*vpp = vp;
				} else {
					*vpp = NULL;
					TRACE(T_SAM_LOOKUP_ERR, pvp,
					    pip->di.id.ino, error, 0);
					RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
					return (error);
				}
			}
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
		TRACE(T_SAM_LOOKUP_RET, pvp, (sam_tr_t)*vpp, ip->di.id.ino,
		    ip->di.nlink);
	} else {
		*vpp = NULL;
		TRACE(T_SAM_LOOKUP_ERR, pvp, pip->di.id.ino, error, 0);
	}
	return (error);
}


/*
 * ----- sam_create_vn - Create a SAM-FS file.
 * Check permissions and if parent directory is valid,
 * then create entry in the directory and set appropriate modes.
 * If created, the vnode is "held". The parent is "unheld".
 */

/* ARGSUSED8 */
int				/* ERRNO if error, 0 if successful. */
sam_create_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *pvp,		/* pointer to parent directory vnode. */
	char *cp,		/* pointer to the component name to create. */
	vattr_t *vap,		/* vattr ptr for type & mode information. */
	vcexcl_t ex,		/* exclusive create flag. */
	int mode,		/* file mode information. */
	vnode_t **vpp,		/* pointer pointer to returned vnode. */
	cred_t *credp,		/* credentials pointer. */
	int filemode,		/* open file mode. */
	caller_context_t *ct,	/* caller context pointer. */
	vsecattr_t *vsap	/* security attributes pointer. */
#else
	vnode_t *pvp,		/* pointer to parent directory vnode. */
	char *cp,		/* pointer to the component name to create. */
	vattr_t *vap,		/* vattr ptr for type & mode information. */
	vcexcl_t ex,		/* exclusive create flag. */
	int mode,		/* file mode information. */
	vnode_t **vpp,		/* pointer pointer to returned vnode. */
	cred_t *credp,		/* credentials pointer. */
	int filemode		/* open file mode. */
#endif
)
{
	int error;
	sam_node_t *pip;

	TRACES(T_SAM_CREATE, pvp, cp);
	/*
	 * Cannot set sticky bit unless superuser.
	 */
	if ((vap->va_mode & VSVTX) && secpolicy_vnode_stky_modify(credp) != 0) {
		vap->va_mode &= ~VSVTX;
	}
	pip = SAM_VTOI(pvp);
	error = sam_create_ino(pip, cp, vap, ex, mode, vpp, credp, filemode);
	if (error == 0) {
		sam_node_t *ip;

		ip = SAM_VTOI(*vpp);
		if (((ip->di.rm.ui.flags & RM_CHAR_DEV_FILE) &&
		    S_ISREG(ip->di.mode)) ||
		    SAM_ISVDEV((*vpp)->v_type)) {
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			/*
			 * Repeat the test under lock.
			 * Don't need the overhead of always grabbing
			 * inode_rwl, RW_WRITER for a rare condition.
			 */
			if (((ip->di.rm.ui.flags & RM_CHAR_DEV_FILE) &&
			    S_ISREG(ip->di.mode)) ||
			    SAM_ISVDEV((*vpp)->v_type)) {
				vnode_t *vp;

				if ((error =
				    sam_get_special_file(ip, &vp,
				    credp)) == 0) {
					*vpp = vp;
				} else {
					*vpp = NULL;
					TRACE(T_SAM_CREATE_ERR,
					    pvp, pip->di.id.ino, error, 0);
					RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
					return (error);
				}
			}
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
		TRACE(T_SAM_CREATE_RET, pvp, (sam_tr_t)*vpp, ip->di.id.ino,
		    ip->di.nlink);
	} else {
		*vpp = NULL;
		TRACE(T_SAM_CREATE_ERR, pvp, pip->di.id.ino, error, 0);
	}
	return (error);
}


/*
 * ----- sam_remove_vn - Remove a SAM-FS file.
 * Remove a SAM-FS file.
 */

/* ARGSUSED3 */
int				/* ERRNO if error, 0 if successful. */
sam_remove_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *pvp,		/* Pointer to parent directory vnode. */
	char *cp,		/* Pointer to the component name to remove. */
	cred_t *credp,		/* Credentials pointer. */
	caller_context_t *ct,	/* Caller context pointer. */
	int flag		/* File flags. */
#else
	vnode_t *pvp,		/* Pointer to parent directory vnode. */
	char *cp,		/* Pointer to the component name to remove. */
	cred_t *credp		/* Credentials pointer. */
#endif
)
{
	int error;
	sam_node_t *pip;
	sam_node_t *ip;
	struct sam_name name;
	ino_t ino = 0;
	vnode_t *rmvp = NULL;
#ifdef LQFS_TODO_LOCKFS
	struct ulockfs *ulp;
#endif /* LQFS_TODO_LOCKFS */
	int trans_size;
	int recursive;
	int issync;

	TRACES(T_SAM_REMOVE, pvp, cp);
	pip = SAM_VTOI(pvp);
#ifdef LQFS_TODO_LOCKFS
	error = ufs_lockfs_begin(pip->mp, &ulp, ULOCKFS_REMOVE_MASK);
	if (error) {
		goto out;
	}

	if (ulp) {
#endif /* LQFS_TODO_LOCKFS */
		trans_size = (int)TOP_REMOVE_SIZE(pip);
		TRANS_BEGIN_CSYNC(pip->mp, issync, TOP_REMOVE, trans_size);
#ifdef LQFS_TODO_LOCKFS
	}
#endif /* LQFS_TODO_LOCKFS */

	/*
	 * sam_free_xattr_tree calls VOP_REMOVE to clean up orphaned xattrs. It
	 * already holds the directory data_rwl. We will already be in the
	 * middle of a transaction, so we don't want to start a new one.
	 */
	recursive = (RW_OWNER_OS(&pip->data_rwl) == curthread);

	if (!recursive) {
		RW_LOCK_OS(&pip->data_rwl, RW_WRITER);
	}

	name.operation = SAM_REMOVE;
	name.client_ord = 0;
	if ((error = sam_lookup_name(pip, cp, &ip, &name, credp)) == 0) {
		ino = ip->di.id.ino;

		/* Entry exists, remove it. */
		error = sam_remove_name(pip, cp, ip, &name, credp);
		rmvp = SAM_ITOV(ip);
	}
	if (!recursive) {
		RW_UNLOCK_OS(&pip->data_rwl, RW_WRITER);
	}
#ifdef LQFS_TODO_LOCKFS
	if (ulp) {
#endif /* LQFS_TODO_LOCKFS */
		TRANS_END_CSYNC(pip->mp, error, issync, TOP_REMOVE, trans_size);
#ifdef LQFS_TODO_LOCKFS
		ufs_lockfs_end(ulp);
	}
#endif /* LQFS_TODO_LOCKFS */

	if (rmvp != NULL) {
		if (error == 0) {
			VNEVENT_REMOVE_OS(rmvp, pvp, cp, NULL);
		}
		VN_RELE(rmvp);
	}

#ifdef LQFS_TODO_LOCKFS
out:
#endif /* LQFS_TODO_LOCKFS */
	TRACE(T_SAM_REMOVE_RET, pvp, pip->di.id.ino, ino, error);
	return (error);
}


/*
 * ----- sam_link_vn - Hard link a SAM-FS file or directory..
 * Add a hard link to an existing file or directory..
 */

/* ARGSUSED4 */
int				/* ERRNO if error, 0 if successful. */
sam_link_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *pvp,		/* parent directory where link is targeted. */
	vnode_t *vp,		/* existing vnode. */
	char *cp,		/* component name for the link. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct,	/* caller context pointer. */
	int flag		/* file flags. */
#else
	vnode_t *pvp,		/* parent directory where link is targeted. */
	vnode_t *vp,		/* existing vnode. */
	char *cp,		/* component name for the link. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	int error = 0;
	sam_node_t *pip;
	sam_node_t *ip;
	sam_node_t *sip;
	vnode_t *realvp;
	struct sam_name name;
	ino_t ino = 0;
#ifdef LQFS_TODO_LOCKFS
	struct ulockfs *ulp;
#endif /* LQFS_TODO_LOCKFS */
	int issync;
	int trans_size;
	int terr = 0;

	pip = SAM_VTOI(pvp);
	if (VOP_REALVP_OS(vp, &realvp, NULL) == 0) {
		vp = realvp;
	}
	sip = SAM_VTOI(vp);
	TRACES(T_SAM_LINK, pvp, cp);
	if (!pip->di.status.b.worm_rdonly && sip->di.status.b.worm_rdonly) {
		return (EROFS);
	}
	if ((vp->v_type == VDIR) &&
	    (secpolicy_fs_linkdir(credp, vp->v_vfsp))) {
		return (EPERM);
	}
lookup_name:
#ifdef LQFS_TODO_LOCKFS
	error = ufs_lockfs_begin(pip->mp, &ulp, ULOCKFS_LINK_MASK);
	if (error) {
		goto out;
	}

	if (ulp) {
#endif /* LQFS_TODO_LOCKFS */
		trans_size = (int)TOP_LINK_SIZE(pip);
		TRANS_BEGIN_CSYNC(pip->mp, issync, TOP_LINK, trans_size);
#ifdef LQFS_TODO_LOCKFS
	}
#endif /* LQFS_TODO_LOCKFS */

	RW_LOCK_OS(&pip->data_rwl, RW_WRITER);
	name.operation = SAM_LINK;
	if ((error = sam_lookup_name(pip, cp, &ip, &name, credp)) == ENOENT) {
		/* Entry does not exist, link it. */
		ip = SAM_VTOI(vp);
		ino = ip->di.id.ino;
		if (ip->di.nlink >= MAXLINK) {
			error = EMLINK;
		} else if (ip->di.nlink == 0) {
			error = ENOENT;
		} else {
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			error = sam_create_name(pip, cp,
			    &ip, &name, NULL, credp);
			if ((error != 0) && IS_SAM_ENOSPC(error)) {
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				RW_UNLOCK_OS(&pip->data_rwl, RW_WRITER);
#ifdef LQFS_TODO_LOCKFS
				if (ulp) {
#endif /* LQFS_TODO_LOCKFS */
					TRANS_END_CSYNC(pip->mp, terr, issync,
					    TOP_LINK, trans_size);
#ifdef LQFS_TODO_LOCKFS
					ufs_lockfs_end(ulp);
				}
#endif /* LQFS_TODO_LOCKFS */
				error = sam_wait_space(pip, error);
				if (error == 0) {
					error = terr;
				}
				if (error) {
					return (error);
				}
				goto lookup_name;
			}
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
	} else if (error == 0) { /* Entry already exists, fail the link. */
		VN_RELE(SAM_ITOV(ip));
		error = EEXIST;
	}
	RW_UNLOCK_OS(&pip->data_rwl, RW_WRITER);

#ifdef LQFS_TODO_LOCKFS
	if (ulp) {
#endif /* LQFS_TODO_LOCKFS */
		TRANS_END_CSYNC(pip->mp, terr, issync, TOP_LINK, trans_size);
		if (error == 0) {
			error = terr;
		}
#ifdef LQFS_TODO_LOCKFS
		ufs_lockfs_end(ulp);
	}
	if (error == 0) {
		VNEVENT_LINK_OS(vp, NULL);
	}
out:
#endif /* LQFS_TODO_LOCKFS */

	TRACE(T_SAM_LINK_RET, pvp, (sam_tr_t)vp, ino, error);
	return (error);
}


/*
 * ----- sam_rename_vn - Rename a SAM-FS file.
 * Rename a SAM-FS file: Verify the old file exists. Remove the new file
 * if it exists; hard link the new file to the old file; remove the old file.
 */

/* ARGSUSED5 */
int				/* ERRNO if error, 0 if successful. */
sam_rename_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *opvp,		/* Pointer to the old file parent vnode. */
	char *onm,		/* Pointer to old file name. */
	vnode_t *npvp,		/* Pointer to the new file parent vnode. */
	char *nnm,		/* Pointer to new file name. */
	cred_t *credp,		/* Credentials pointer. */
	caller_context_t *ct,	/* Caller context pointer. */
	int flag		/* File flags. */
#else
	vnode_t *opvp,		/* Pointer to the old file parent vnode. */
	char *onm,		/* Pointer to old file name. */
	vnode_t *npvp,		/* Pointer to the new file parent vnode. */
	char *nnm,		/* Pointer to new file name. */
	cred_t *credp		/* Credentials pointer. */
#endif
)
{
	vnode_t *realvp;
	int error;
	int pass = 0;

	TRACES(T_SAM_RENAME1, opvp, onm);
	if (VOP_REALVP_OS(npvp, &realvp, NULL) == 0) {
		npvp = realvp;
	}
	TRACES(T_SAM_RENAME2, npvp, nnm);
retry:
	error = sam_rename_inode(SAM_VTOI(opvp), onm, SAM_VTOI(npvp), nnm,
	    NULL, credp);
	if (error == EDEADLK) {
		if (pass >= 10) {
			pass += 4;
		} else {
			pass++;
		}
		delay(pass);
		goto retry;
	}
	return (error);
}


/*
 * ----- sam_mkdir_vn - Make a SAM-FS directory.
 * Make a directory in this file system.
 */

/* ARGSUSED5 */
int				/* ERRNO if error, 0 if successful. */
sam_mkdir_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *pvp,		/* pointer to parent directory vnode. */
	char *cp,		/* component directory name to create. */
	vattr_t *vap,		/* vnode attributes pointer. */
	vnode_t **vpp,		/* pointer pointer to returned vnode. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct,	/* caller context pointer. */
	int flag,		/* file flags. */
	vsecattr_t *vsap	/* security attributes pointer. */
#else
	vnode_t *pvp,		/* pointer to parent directory vnode. */
	char *cp,		/* component directory name to create. */
	vattr_t *vap,		/* vnode attributes pointer. */
	vnode_t **vpp,		/* pointer pointer to returned vnode. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	int error;
	sam_node_t *pip;
	sam_node_t *ip;
	struct sam_name name;
#ifdef LQFS_TODO_LOCKFS
	struct ulockfs *ulp;
#endif /* LQFS_TODO_LOCKFS */
	int issync;
	int trans_size;
	int terr = 0;

	TRACES(T_SAM_MKDIR, pvp, cp);
	/* Only superuser can set sticky bit */
	if ((vap->va_mode & VSVTX) && secpolicy_vnode_stky_modify(credp) != 0) {
		vap->va_mode &= ~VSVTX;
	}
	pip = SAM_VTOI(pvp);

	/*
	 * Can't make directory in xattr hidden dir.
	 */
	if (S_ISATTRDIR(pip->di.mode)) {
		return (EINVAL);
	}

lookup_name:
#ifdef LQFS_TODO_LOCKFS
	error = ufs_lockfs_begin(pip->mp, &ulp, ULOCKFS_MKDIR_MASK);
	if (error) {
		goto out;
	}

	if (ulp) {
#endif /* LQFS_TODO_LOCKFS */
		trans_size = (int)TOP_MKDIR_SIZE(pip);
		TRANS_BEGIN_CSYNC(pip->mp, issync, TOP_MKDIR, trans_size);
#ifdef LQFS_TODO_LOCKFS
	}
#endif /* LQFS_TODO_LOCKFS */

	RW_LOCK_OS(&pip->data_rwl, RW_WRITER);
	name.operation = SAM_MKDIR;
	if ((error = sam_lookup_name(pip, cp, &ip, &name, credp)) == ENOENT) {
		if (((error = sam_create_name(pip, cp, &ip,
		    &name, vap, credp)) != 0) &&
		    IS_SAM_ENOSPC(error)) {
			RW_UNLOCK_OS(&pip->data_rwl, RW_WRITER);
#ifdef LQFS_TODO_LOCKFS
			if (ulp) {
#endif /* LQFS_TODO_LOCKFS */
				TRANS_END_CSYNC(pip->mp, terr, issync,
				    TOP_MKDIR, trans_size);
#ifdef LQFS_TODO_LOCKFS
				ufs_lockfs_end(ulp);
			}
#endif /* LQFS_TODO_LOCKFS */
			error = sam_wait_space(pip, error);
			if (error == 0) {
				error = terr;
			}
			if (error)
				return (error);
			goto lookup_name;
		}
		RW_UNLOCK_OS(&pip->data_rwl, RW_WRITER);

	} else if (error == 0) {	/* If entry already exists. */
		RW_UNLOCK_OS(&pip->data_rwl, RW_WRITER);
		error = EEXIST;
		VN_RELE(SAM_ITOV(ip));	/* Decrement vnode count */
	} else {
		RW_UNLOCK_OS(&pip->data_rwl, RW_WRITER);
	}
#ifdef LQFS_TODO_LOCKFS
	if (ulp) {
#endif /* LQFS_TODO_LOCKFS */
		TRANS_END_CSYNC(pip->mp, terr, issync, TOP_MKDIR, trans_size);
		if (error == 0) {
			error = terr;
		}
#ifdef LQFS_TODO_LOCKFS
		ufs_lockfs_end(ulp);
	}
out:
#endif /* LQFS_TODO_LOCKFS */
	if (error == 0) {
		*vpp = SAM_ITOV(ip);
		TRACE(T_SAM_MKDIR_RET,
		    pvp, (sam_tr_t)* vpp, ip->di.id.ino, error);
	} else {
		*vpp = NULL;
		TRACE(T_SAM_MKDIR_RET, pvp, (sam_tr_t)* vpp, 0, error);
	}
	return (error);
}


/*
 * ----- sam_rmdir_vn - Remove a SAM-FS directory.
 * Remove a directory in this file system.
 */

/* ARGSUSED2 */
int				/* ERRNO if error, 0 if successful. */
sam_rmdir_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *pvp,		/* pointer to parent directory vnode. */
	char *cp,		/* pointer to the component name to remove. */
	vnode_t *cdir,		/* pointer to vnode. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct,	/* caller context pointer. */
	int flag		/* file flags. */
#else
	vnode_t *pvp,		/* pointer to parent directory vnode. */
	char *cp,		/* pointer to the component name to remove. */
	vnode_t *cdir,		/* pointer to vnode. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	int error;
	sam_node_t *pip;
	sam_node_t *ip;
	struct sam_name name;
	ino_t ino = 0;
	vnode_t *rmvp = NULL;
#ifdef LQFS_TODO_LOCKFS
	struct ulockfs *ulp;
#endif /* LQFS_TODO_LOCKFS */
	int issync;
	int terr = 0;

	TRACES(T_SAM_RMDIR, pvp, cp);
	pip = SAM_VTOI(pvp);
#ifdef LQFS_TODO_LOCKFS
	error = ufs_lockfs_begin(pip->mp, &ulp, ULOCKFS_RMDIR_MASK);
	if (error) {
		goto out2;
	}

	if (ulp) {
#endif /* LQFS_TODO_LOCKFS */
		TRANS_BEGIN_CSYNC(pip->mp, issync, TOP_RMDIR, TOP_RMDIR_SIZE);
#ifdef LQFS_TODO_LOCKFS
	}
#endif /* LQFS_TODO_LOCKFS */

	RW_LOCK_OS(&pip->data_rwl, RW_WRITER);
	name.operation = SAM_RMDIR;
	name.client_ord = 0;
	ip = NULL;
	if ((error = sam_lookup_name(pip, cp, &ip, &name, credp)) == 0) {
		/*
		 * Entry exists.  Check the locking order.
		 */
		if (pip->di.id.ino > ip->di.id.ino) {
			sam_node_t *ip2;

			RW_UNLOCK_OS(&pip->data_rwl, RW_WRITER);
go_fish:
			sam_dlock_two(pip, ip, RW_WRITER);
			bzero(&name, sizeof (name));
			name.operation = SAM_RMDIR;
			if ((error = sam_lookup_name(pip, cp,
			    &ip2, &name, credp))) {
				/*
				 * Entry was removed while the lock
				 * was released.
				 */
				goto lost_it;
			}
			/*
			 * If this name gives us a new inode then we
			 * have to start over.
			 */
			if ((ip->di.id.ino != ip2->di.id.ino) ||
			    (ip->di.id.gen != ip2->di.id.gen)) {
				sam_dunlock_two(pip, ip, RW_WRITER);
				VN_RELE(SAM_ITOV(ip));
				ip = ip2;
				goto go_fish;
			}
			VN_RELE(SAM_ITOV(ip2));
		} else {
			RW_LOCK_OS(&ip->data_rwl, RW_WRITER);
		}
		ino = ip->di.id.ino;

		error = sam_remove_name(pip, cp, ip, &name, credp);
		if (error == 0) {
			rmvp = SAM_ITOV(ip);
		}
	}
lost_it:
	sam_dunlock_two(pip, ip ? ip : pip, RW_WRITER);

#ifdef LQFS_TODO_LOCKFS
	if (ulp) {
#endif /* LQFS_TODO_LOCKFS */
		TRANS_END_CSYNC(pip->mp, terr, issync, TOP_RMDIR,
		    TOP_RMDIR_SIZE);
		if (error == 0) {
			error = terr;
		}
#ifdef LQFS_TODO_LOCKFS
		ufs_lockfs_end(ulp);
	}
#endif /* LQFS_TODO_LOCKFS */

	if (rmvp) {
		VNEVENT_RMDIR_OS(rmvp, pvp, cp, NULL);
	}
	if (ip) {
		VN_RELE(SAM_ITOV(ip));
	}

#ifdef LQFS_TODO_LOCKFS
out2:
#endif /* LQFS_TODO_LOCKFS */

	TRACE(T_SAM_RMDIR_RET, pvp, pip->di.id.ino, ino, error);
	return (error);
}


/*
 * ----- sam_symlink_vn - Make a SAM-FS symbolic link.
 *	Make a symlink in this file system.
 */

/* ARGSUSED5 */
int				/* ERRNO if error, 0 if successful. */
sam_symlink_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *pvp,		/* pointer to parent directory vnode. */
	char *cp,		/* pointer to the name of the symbolic link. */
	vattr_t *vap,		/* vattr pointer for the symbolic link. */
	char *tnm,		/* pointer to the symbolic link pathname. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct,	/* caller context pointer. */
	int flag		/* file flags. */
#else
	vnode_t *pvp,		/* pointer to parent directory vnode. */
	char *cp,		/* pointer to the name of the symbolic link. */
	vattr_t *vap,		/* vattr pointer for the symbolic link. */
	char *tnm,		/* pointer to the symbolic link pathname. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	int error;
	sam_node_t *pip;
	sam_node_t *ip;
	struct sam_name name;	/* If no entry, offset is returned here */
	ino_t ino = 0;
	int	version;
#ifdef LQFS_TODO_LOCKFS
	struct ulockfs *ulp;
#endif /* LQFS_TODO_LOCKFS */
	int issync;
	int trans_size;
	int terr = 0;

	TRACES(T_SAM_SYMLINK, pvp, cp);
	pip = SAM_VTOI(pvp);
	version = pip->di.version;
lookup_name:
#ifdef LQFS_TODO_LOCKFS
	error = ufs_lockfs_begin(pip->mp, &ulp, ULOCKFS_SYMLINK_MASK);
	if (error) {
		goto out;
	}

	if (ulp) {
#endif /* LQFS_TODO_LOCKFS */
		trans_size = (int)TOP_SYMLINK_SIZE(pip);
		TRANS_BEGIN_CSYNC(pip->mp, issync, TOP_SYMLINK, trans_size);
#ifdef LQFS_TODO_LOCKFS
	}
#endif /* LQFS_TODO_LOCKFS */
	RW_LOCK_OS(&pip->data_rwl, RW_WRITER);
	name.operation = SAM_CREATE;
	if ((error = sam_lookup_name(pip, cp, &ip, &name, credp)) == ENOENT) {
		if (((error = sam_create_name(pip, cp, &ip, &name, vap, credp))
		    != 0) && IS_SAM_ENOSPC(error)) {
			RW_UNLOCK_OS(&pip->data_rwl, RW_WRITER);
#ifdef LQFS_TODO_LOCKFS
			if (ulp) {
#endif /* LQFS_TODO_LOCKFS */
				TRANS_END_CSYNC(pip->mp, terr, issync,
				    TOP_SYMLINK, trans_size);
#ifdef LQFS_TODO_LOCKFS
				ufs_lockfs_end(ulp);
			}
#endif /* LQFS_TODO_LOCKFS */
			error = sam_wait_space(pip, error);
			if (error == 0) {
				error = terr;
			}
			if (error) {
				return (error);
			}
			goto lookup_name;
		}
		if (error == 0) {
			ino = ip->di.id.ino;

			/*
			 * If inode. ext. symlinks
			 */
			if (version >= SAM_INODE_VERS_2) {
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				error = sam_set_symlink(pip, ip, tnm,
				    strlen(tnm), credp);
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			} else if (version == SAM_INODE_VERS_1) {
				error = sam_set_old_symlink(ip, tnm,
				    strlen(tnm), credp);
			} else {
				sam_req_ifsck(ip->mp, -1,
				    "sam_symlink_vn: version", &ip->di.id);
				error = ENOCSI;
			}

			/*
			 * If we get an error while creating the symbolic
			 * link, remove the entry which we created in the
			 * directory.  We need to do a lookup to ensure that
			 * we're working with the right slot in the directory,
			 * since sam_create_name won't update the slot
			 * information.  This ought to be cleaned up sometime.
			 */

			if (error != 0) {
				sam_node_t *xip;

				name.operation = SAM_REMOVE;
				name.client_ord = 0;
				if (sam_lookup_name(pip, cp, &xip,
				    &name, credp) == 0) {
					ASSERT(ip == xip);
					(void) sam_remove_name(pip, cp, ip,
					    &name, credp);
					VN_RELE(SAM_ITOV(ip));
				}
			}
			VN_RELE(SAM_ITOV(ip));	/* Decrement v_count */
		}
	} else if (error == 0) {	/* If entry already exists. */
		error = EEXIST;
		VN_RELE(SAM_ITOV(ip));	/* Decrement count */
	}
	RW_UNLOCK_OS(&pip->data_rwl, RW_WRITER);
#ifdef LQFS_TODO_LOCKFS
	if (ulp) {
#endif /* LQFS_TODO_LOCKFS */
		TRANS_END_CSYNC(pip->mp, terr, issync, TOP_SYMLINK, trans_size);
		if (error == 0) {
			error = terr;
		}
#ifdef LQFS_TODO_LOCKFS
		ufs_lockfs_end(ulp);
	}
out:
#endif /* LQFS_TODO_LOCKFS */

	TRACE(T_SAM_SYMLINK_RET, pvp, ino, error, 0);
	return (error);
}


/*
 * ----- sam_frlock_vn - File and record locking.
 * File and record locking.
 */

/* ARGSUSED7 */
int					/* ERRNO, else 0 if successful. */
sam_frlock_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,			/* Pointer to vnode. */
	int cmd,			/* Command. */
	sam_flock_t *flp,		/* Pointer to flock */
	int filemode,			/* filemode flags, see file.h */
	offset_t offset,		/* offset. */
	struct flk_callback *fcb,	/* extra callback param */
	cred_t *credp,			/* credentials pointer. */
	caller_context_t *ct		/* caller context pointer. */
#else
	vnode_t *vp,			/* Pointer to vnode. */
	int cmd,			/* Command. */
	sam_flock_t *flp,		/* Pointer to flock */
	int filemode,			/* filemode flags, see file.h */
	offset_t offset,		/* offset. */
	struct flk_callback *fcb,	/* extra callback param */
	cred_t *credp			/* credentials pointer. */
#endif
)
{
	int error;
	sam_node_t *ip;

	ip = SAM_VTOI(vp);
	TRACE(T_SAM_FRLOCK, vp, cmd, filemode, offset);
	if (ip->mm_pages && MANDLOCK(vp, ip->di.mode)) {
		return (EAGAIN);
	}
	error = FS_FRLOCK_OS(vp, cmd, flp, filemode, offset, fcb, credp, NULL);
	TRACE(T_SAM_FRLOCK_RET, vp, cmd, filemode, error);
	return (error);
}


/*
 * ----- sam_space_vn - Truncate/expand storage space.
 * For cmd F_FREESP, truncate/expand space.
 *  l_whence indicates where the relative offset is measured:
 *	  0 - the start of the file.
 *	  1 - the current position.
 *	  2 - the end of the file.
 *  l_start is the offset from the position specified in l_whence.
 *  l_len is the size to be freed. l_len = 0 frees up to the end of the file.
 */

/* ARGSUSED6 */
int				/* ERRNO if error occured, 0 if successful. */
sam_space_vn(
	vnode_t *vp,		/* vnode entry */
	int cmd,		/* command */
	sam_flock_t *flp,	/* Pointer to flock */
	int filemode,		/* filemode flags, see file.h */
	offset_t offset,	/* offset */
	cred_t *credp,		/* credentials */
	caller_context_t *ct)	/* caller context pointer. */
{
	int error = 0;
	sam_node_t *ip;

	TRACE(T_SAM_SPACE, vp, cmd, flp->l_start, offset);
	if (cmd == F_FREESP) {
		if ((error = convoff(vp, flp, 0, (sam_offset_t)offset)) == 0) {
			ip = SAM_VTOI(vp);
			if (S_ISREQ(ip->di.mode)) {
				/*
				 * Cannot truncate a removeable media file
				 */
				error = EINVAL;
			} else if (SAM_PRIVILEGE_INO(ip->di.version,
			    ip->di.id.ino)) {
				/*
				 * Cannot truncate priviledged inodes
				 */
				error = EPERM;
			} else {
				offset_t segment_size;
				/*
				 * If there are any dirty pages flush them
				 * before growing a segmented file. This is
				 * particularly useful if we're transitioning
				 * the first inode to an index.
				 */
				segment_size = SAM_SEGSIZE(
				    ip->di.rm.info.dk.seg_size);
				if (ip->di.status.b.segment &&
				    vn_has_cached_data(SAM_ITOV(ip)) &&
				    (flp->l_start > segment_size) &&
				    (ip->size <= segment_size)) {
					(void) VOP_PUTPAGE_OS(SAM_ITOV(ip), 0,
					    0, B_INVAL, credp, NULL);
					mutex_enter(&ip->write_mutex);
					ip->klength = ip->koffset = 0;
					mutex_exit(&ip->write_mutex);
				}
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				if ((error = sam_space_ino(ip, flp,
				    filemode)) == 0) {
					if (TRANS_ISTRANS(ip->mp)) {
						/*
						 * TRANS_ITRUNC starts a new
						 * transaction, so we need to
						 * give-up the inode r/w lock.
						 */
						RW_UNLOCK_OS(&ip->inode_rwl,
						    RW_WRITER);
						error = TRANS_ITRUNC(ip,
						    (u_offset_t)flp->l_start,
						    STALE_ARCHIVE, credp);
						RW_LOCK_OS(&ip->inode_rwl,
						    RW_WRITER);
					} else {
						error = sam_clear_file(ip,
						    (offset_t)flp->l_start,
						    STALE_ARCHIVE, credp);
					}
				}
				if (ip->size < ip->zero_end) {
					ip->zero_end = 0;
				}
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			}
		}
	} else {
		error = EINVAL;
	}
	TRACE(T_SAM_SPACE_RET, vp, error, 0, 0);
	return (error);
}


/*
 * ----- sam_addmap_vn - Increment count of memory mapped pages.
 * Increment count of memory mapped pages.
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
sam_addmap_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* pointer to vnode. */
	offset_t offset,	/* file offset. */
	struct as *asp,		/* pointer to address segment. */
	caddr_t a,		/* pointer offset. */
	sam_size_t length,	/* file length provided to mmap. */
	uchar_t prot,		/* requested protection from <sys/mman.h>. */
	uchar_t maxprot,	/* max requested prot from <sys/mman.h>. */
	uint_t flags,		/* flags. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* pointer to vnode. */
	offset_t offset,	/* file offset. */
	struct as *asp,		/* pointer to address segment. */
	caddr_t a,		/* pointer offset. */
	sam_size_t length,	/* file length provided to mmap. */
	uchar_t prot,		/* requested protection from <sys/mman.h>. */
	uchar_t maxprot,	/* max requested prot from <sys/mman.h>. */
	uint_t flags,		/* flags. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	int error = 0;
	int pages;
	sam_node_t *ip;

	TRACE(T_SAM_ADDMAP, vp, (sam_tr_t)offset, length, prot);
	ip = SAM_VTOI(vp);

	if (vp->v_flag & VNOMAP) {	/* If file cannot be memory mapped */
		error = ENOSYS;
	} else {
		boolean_t is_write;

		is_write = (prot & PROT_WRITE) &&
		    ((flags & MAP_TYPE) == MAP_SHARED);

		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		mutex_enter(&ip->fl_mutex);
		pages = btopr(length);
		ip->mm_pages += pages;
		if (is_write) {
			ip->wmm_pages += pages;
		}
		mutex_exit(&ip->fl_mutex);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	}

	TRACE(T_SAM_ADDMAP_RET, vp, pages, ip->mm_pages, error);
	return (error);
}


/*
 * ----- sam_delmap_vn - Decrement count of memory mapped pages.
 * Decrement count of memory mapped pages.
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
sam_delmap_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* Pointer to vnode. */
	offset_t offset,	/* File offset. */
	struct as *asp,		/* Pointer to address segment. */
	caddr_t a,		/* Pointer offset. */
	sam_size_t length,	/* File length provided to mmap. */
	uint_t prot,		/* Requested access from <sys/mman.h>. */
	uint_t maxprot,		/* Max requested access from <sys/mman.h> */
	uint_t flags,		/* Flags. */
	cred_t *credp,		/* Credentials pointer. */
	caller_context_t *ct	/* Caller context pointer. */
#else
	vnode_t *vp,		/* Pointer to vnode. */
	offset_t offset,	/* File offset. */
	struct as *asp,		/* Pointer to address segment. */
	caddr_t a,		/* Pointer offset. */
	sam_size_t length,	/* File length provided to mmap. */
	uint_t prot,		/* Requested access from <sys/mman.h>. */
	uint_t maxprot,		/* Max requested access from <sys/mman.h> */
	uint_t flags,		/* Flags. */
	cred_t *credp		/* Credentials pointer. */
#endif
)
{
	int error = 0;
	int pages;
	sam_node_t *ip;

	TRACE(T_SAM_DELMAP, vp, (sam_tr_t)offset, length, prot);
	ip = SAM_VTOI(vp);

	if (vp->v_flag & VNOMAP) {	/* If file cannot be memory mapped */
		error = ENOSYS;
	} else {
		boolean_t is_write;

		is_write = (prot & PROT_WRITE) &&
		    ((flags & MAP_TYPE) == MAP_SHARED);

		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		mutex_enter(&ip->fl_mutex);
		pages = btopr(length);
		ip->mm_pages -= pages;
		ASSERT(ip->mm_pages >= 0);
		if (ip->mm_pages < 0) {
			ip->mm_pages = 0;
		}
		if (is_write) {
			ip->wmm_pages -= pages;
		}
		mutex_exit(&ip->fl_mutex);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	}

	TRACE(T_SAM_DELMAP_RET, vp, pages, ip->mm_pages, error);
	return (error);
}


/*
 * ----- sam_getsecattr_vn - get security attributes for vnode.
 * Retrieve security attributes from access control list.
 */

/* ARGSUSED3 */
int
sam_getsecattr_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* Pointer to vnode. */
	vsecattr_t *vsap,	/* Security attributes pointer. */
	int flag,		/* Flags. */
	cred_t *credp,		/* Credentials pointer. */
	caller_context_t *ct	/* Caller context pointer. */
#else
	vnode_t *vp,		/* Pointer to vnode. */
	vsecattr_t *vsap,	/* Security attributes pointer. */
	int flag,		/* Flags. */
	cred_t *credp		/* Credentials pointer. */
#endif
)
{
	int error = 0;
	sam_node_t *ip;

	ip = SAM_VTOI(vp);
	vsap->vsa_mask &= (VSA_ACL | VSA_ACLCNT | VSA_DFACL | VSA_DFACLCNT);
	if (vsap->vsa_mask != 0) {
		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		error = sam_acl_get_vsecattr(ip, vsap);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	}

	DTRACE_PROBE2(vsap, vnode_t *, vp , vsecattr_t *, vsap);
	TRACE(T_SAM_GETSECATTR, vp, (sam_tr_t)vsap, flag, error);
	return (error);
}


/*
 * ----- sam_setsecattr_vn - set security attributes for vnode.
 * Set security attributes in inode extensions(s).
 */

/* ARGSUSED4 */
int
sam_setsecattr_vn(
#if defined(SOL_511_ABOVE)
	struct vnode *vp,	/* Pointer to vnode. */
	vsecattr_t *vsap,	/* Security attributes pointer. */
	int flag,		/* Flags. */
	struct cred *credp,	/* Credentials pointer. */
	caller_context_t *ct	/* Caller context pointer. */
#else
	struct vnode *vp,	/* Pointer to vnode. */
	vsecattr_t *vsap,	/* Security attributes pointer. */
	int flag,		/* Flags. */
	struct cred *credp	/* Credentials pointer. */
#endif
)
{
	int error = 0;
	sam_node_t *ip;
#ifdef LQFS_TODO_LOCKFS
	struct ulockfs *ulp;
#endif /* LQFS_TODO_LOCKFS */
	int trans_size;

	ip = SAM_VTOI(vp);

	/*
	 * NAS compatibility.  If the WORM bit is set we can not
	 * allow the acl info to be updated.  Also, this routine is called
	 * as a normal part of handling the mode change for the WORM
	 * trigger (chmod 4000) via another VOP, so we can't return EPERM.
	 * Just return without changing the acl.
	 */
	if (ip->di.status.b.worm_rdonly) {
		goto out;
	}
	/* Only super-user or file owner can set access control list */
	if (secpolicy_vnode_setdac(credp, ip->di.uid)) {
		error = EPERM;
		goto out;
	}
	if (ip->di.id.ino == SAM_INO_INO) {
		error = EINVAL;
		goto out;
	}
	vsap->vsa_mask &= (VSA_ACL | VSA_DFACL);
	if ((vsap->vsa_mask == 0) ||
	    ((vsap->vsa_aclentp == NULL) &&
	    (vsap->vsa_dfaclentp == NULL))) {
		error = EINVAL;
		goto out;
	}

	/*
	 * Make sure default access control list only on directories.
	 */
	if ((vsap->vsa_dfaclcnt > 0) && !(S_ISDIR(ip->di.mode))) {
		error = EINVAL;
		goto out;
	}

#ifdef LQFS_TODO_LOCKFS
	error = qfs_lockfs_begin(ip->mp, &ulp, ULOCKFS_SETATTR_MASK);
	if (error) {
		goto out;
	}

	if (ulp) {
#endif /* LQFS_TODO_LOCKFS */
		trans_size = TOP_SETSECATTR_SIZE(ip);
		TRANS_BEGIN_ASYNC(ip->mp, TOP_SETSECATTR, trans_size);
#ifdef LQFS_TODO_LOCKFS
	}
#endif /* LQFS_TODO_LOCKFS */

	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	error = sam_acl_set_vsecattr(ip, vsap);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

#ifdef LQFS_TODO_LOCKFS
	if (ulp) {
#endif /* LQFS_TODO_LOCKFS */
		TRANS_END_ASYNC(ip->mp, TOP_SETSECATTR, trans_size);
#ifdef LQFS_TODO_LOCKFS
		ufs_lockfs_end(ulp);
	}
#endif /* LQFS_TODO_LOCKFS */

out:
	TRACE(T_SAM_SETSECATTR, vp, (sam_tr_t)vsap, flag, error);
	return (error);
}


/*
 * ----- sam_delmap_stale_vn
 *
 * Delete page mappings attached to a stale vnode.  Should be
 * OK as is.  If we delete the last pages, the kernel should
 * notice, and eventually call inactive if/when appropriate.
 *
 */
/* ARGSUSED */
int
sam_delmap_stale_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,
	offset_t offset,
	struct as *asp,
	caddr_t a,
	sam_size_t length,
	uint_t prot,
	uint_t maxprot,
	uint_t flags,
	cred_t *credp,
	caller_context_t *ct
#else
	vnode_t *vp,
	offset_t offset,
	struct as *asp,
	caddr_t a,
	sam_size_t length,
	uint_t prot,
	uint_t maxprot,
	uint_t flags,
	cred_t *credp
#endif
)
{
	int error = 0;
	int pages;
	sam_node_t *ip;

	TRACE(T_SAM_DELMAP, vp, (sam_tr_t)offset, length, prot);
	ip = SAM_VTOI(vp);

	if (vp->v_flag & VNOMAP) {	/* If file cannot be memory mapped */
		error = ENOSYS;
	} else {
		boolean_t is_write;

		is_write = (prot & PROT_WRITE) &&
		    ((flags & MAP_TYPE) == MAP_SHARED);

		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		mutex_enter(&ip->fl_mutex);
		pages = btopr(length);
		ip->mm_pages -= pages;
		if (is_write) {
			ip->wmm_pages -= pages;
		}
		mutex_exit(&ip->fl_mutex);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	}

	TRACE(T_SAM_DELMAP_RET, vp, pages, ip->mm_pages, error);
	return (error);
}
