/*
 * ----- inode.c - Process the sam inode functions.
 *
 * Processes the inode functions supported on the SAM File
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

#pragma ident "$Revision: 1.158 $"

#include "sam/osversion.h"

#define	_SYSCALL32		1
#define	_SYSCALL32_IMPL	1

/* ----- UNIX Includes */

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
#include <sys/mode.h>
#include <vm/pvn.h>
#include <nfs/nfs.h>
#include <sys/policy.h>

/* ----- SAMFS Includes */

#include "sam/types.h"
#include "sam/fioctl.h"
#include "sam/samevent.h"

#include "inode.h"
#include "mount.h"
#include "rwio.h"
#include "extern.h"
#include "arfind.h"
#include "debug.h"
#include "macros.h"
#include "trace.h"
#include "acl.h"
#include "qfs_log.h"

/*
 * Some common mask bits.
 */
#define	SUID_SET(mode) ((mode) & S_ISUID)
#define	RMASK (S_IRUSR | S_IRGRP | S_IROTH)
#define	WMASK (S_IWUSR | S_IWGRP | S_IWOTH)
#define	XMASK (S_IXUSR | S_IXGRP | S_IXOTH)
#define	WXMASK (WMASK | XMASK)
#define	RWXALLMASK (S_IRWXU | S_IRWXG | S_IRWXO)
#define	GRP_SVTXMASK (S_ISGID | S_ISVTX)

static inline int sam_inode_validate(ino_t ino, struct sam_perm_inode *pip);

kmutex_t	qfs_scan_lock;	/* Prevent multiple qfs_scan_inodes() */

/*
 * ----- sam_create_ino - Create a SAM-QFS file.
 * Check permissions and if parent directory is valid,
 * then create entry in the directory and set appropriate modes.
 * If created, the vnode is "held". The parent is "unheld".
 */

/* ARGSUSED7 */
int				/* ERRNO if error, 0 if successful. */
sam_create_ino(
	sam_node_t *pip,	/* pointer to parent directory inode. */
	char *cp,		/* pointer to the component name to create. */
	vattr_t *vap,		/* vattr ptr for type & mode information. */
	vcexcl_t ex,		/* exclusive create flag. */
	int mode,		/* file mode information. */
	vnode_t **vpp,		/* pointer pointer to returned vnode. */
	cred_t *credp,		/* credentials pointer. */
	int filemode)		/* open file mode */
{
	int error = 0;
	sam_node_t *ip;
	struct sam_name name;	/* If no entry, slot info is returned here */
	int trans_size;
	int issync;
	int truncflag = 0;
	int terr = 0;
#ifdef LQFS_TODO_LOCKFS
	struct ulockfs *ulp;
#endif /* LQFS_TODO_LOCKFS */

	/*
	 * Cannot set sticky bit unless superuser.
	 */
	if ((vap->va_mode & VSVTX) && secpolicy_vnode_stky_modify(credp)) {
		vap->va_mode &= ~VSVTX;
	}

lookup_name:

#ifdef LQFS_TODO_LOCKFS
	error = qfs_lockfs_begin(pip->mp, &ulp, ULOCKFS_CREATE_MASK);
	if (error) {
		return (error);
	}

	if (ulp) {
#endif /* LQFS_TODO_LOCKFS */
		/* Start LQFS create transaction */
		trans_size = (int)TOP_CREATE_SIZE(pip);
		TRANS_BEGIN_CSYNC(pip->mp, issync, TOP_CREATE, trans_size);
#ifdef LQFS_TODO_LOCKFS
	}
#endif /* LQFS_TODO_LOCKFS */

	RW_LOCK_OS(&pip->data_rwl, RW_WRITER);
	name.operation = SAM_CREATE;
	if ((error = sam_lookup_name(pip, cp, &ip, &name, credp)) == ENOENT) {
		if (((error = sam_create_name(pip, cp, &ip, &name,
		    vap, credp)) != 0) &&
		    IS_SAM_ENOSPC(error)) {
			RW_UNLOCK_OS(&pip->data_rwl, RW_WRITER);
			/*
			 * Temporarily end LQFS create transaction
			 */
#ifdef LQFS_TODO_LOCKFS
			if (ulp) {
#endif /* LQFS_TODO_LOCKFS */
				TRANS_END_CSYNC(pip->mp, terr, issync,
				    TOP_CREATE, trans_size);
#ifdef LQFS_TODO_LOCKFS
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
		RW_UNLOCK_OS(&pip->data_rwl, RW_WRITER);

	} else if (error == 0) {	/* If entry already exists. */
		RW_UNLOCK_OS(&pip->data_rwl, RW_WRITER);
		error = EEXIST;

		if (ex == NONEXCL) {	/* If non-exclusive create */
			if ((S_ISDIR(ip->di.mode) ||
			    S_ISATTRDIR(ip->di.mode)) &&
			    (mode & S_IWRITE)) {
				/* Cannot create over an existing dir. */
				error = EISDIR;
			} else if (SAM_PRIVILEGE_INO(ip->di.version,
			    ip->di.id.ino)) {
				/* Cannot create over privileged inodes */
				error = EPERM;
			} else if (mode) {	/* Check mode if set */
				error = sam_access_ino(ip, mode, FALSE, credp);
			} else {
				error = 0;
			}
			if ((error == 0) && S_ISREG(ip->di.mode) &&
			    (vap->va_mask & AT_SIZE) && (vap->va_size == 0)) {
				/*
				 * If logging, do the truncate after the
				 * LQFS create transaction is logged.
				 */
				if (TRANS_ISTRANS(ip->mp)) {
					truncflag++;
				} else {
					RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
					error = sam_clear_file(ip, 0,
					    STALE_ARCHIVE, credp);
					RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				}
				if (error == 0) {
					VNEVENT_CREATE_OS(SAM_ITOV(ip), NULL);
				}
			}
		}
		/*
		 * Cannot do the following as it caused a stale of
		 * offline copies.
		 */
#if	0
		if ((error == 0) && ((mode & O_CREAT) == 0)) {
			TRANS_INODE(ip->mp, ip);
			sam_mark_ino(ip, SAM_UPDATED|SAM_CHANGED);
		}
#endif
		if (error) {
			VN_RELE(SAM_ITOV(ip));	/* Decrement v_count if error */
		}
	} else {
		RW_UNLOCK_OS(&pip->data_rwl, RW_WRITER);
	}
#ifdef LQFS_TODO_LOCKFS
	if (ulp) {
#endif /* LQFS_TODO_LOCKFS */
		TRANS_END_CSYNC(pip->mp, terr, issync, TOP_CREATE, trans_size);
		/*
		 * If we haven't had a more interesting failure
		 * already, then anything that might've happened
		 * here should be reported.
		 */
		if (error == 0) {
			error = terr;
		}
#ifdef LQFS_TODO_LOCKFS
	}
#endif /* LQFS_TODO_LOCKFS */

	if (!error && truncflag) {
		(void) TRANS_ITRUNC(ip, (u_offset_t)0, STALE_ARCHIVE, credp);
	}

#ifdef LQFS_TODO_LOCKFS
	if (ulp) {
		qfs_lockfs_end(ulp);
	}
#endif /* LQFS_TODO_LOCKFS */

	if (error == 0) {
		*vpp = SAM_ITOV(ip);
		TRACE(T_SAM_CREATE_RET, SAM_ITOV(pip), (sam_tr_t)* vpp,
		    ip->di.id.ino, error);
	}
	return (error);
}


/*
 * ----- sam_chk_worm - Check a WORM'd SAM-QFS file.
 * Enforce NAS checking for WORM'd files. Check for execute
 * permission or if the sticky or setgid bit are set.  Don't allow
 * the SUID bit to be cleared unless the read bits are being set
 * or cleared or the retention period is being extended.
 */

static int
sam_chk_worm(sam_mode_t mode, uint_t mask, sam_node_t *ip)
{
	/*
	 * For NAS compatibility the following are true:
	 *  - An attempt to set the setgid bit or sticky bit will fail
	 *		returning EPERM.
	 *  - An attempt to enable write or execute permission will fail
	 *		returning EPERM.
	 */
	if (!S_ISDIR(ip->di.mode)) {
		if ((mask == AT_MODE) &&
		    (mode & (GRP_SVTXMASK | WXMASK))) {
			return (EPERM);
		}
		/*
		 * For NAS compatibility the following is true:
		 *  - Any attempt to clear the setuid bit will fail returning
		 *  EPERM.  The following operations are exceptions to this
		 *  rule:
		 *		- Read access bits can be set and cleared.
		 *		  The mask must be set to AT_MODE and the
		 *		  mode bits must be read only.
		 *		- The retention period can be extended via
		 *			updating the access time. Note, only the
		 *			access time may be updated.
		 */

		/*
		 * Prevent all changes except mode, access time, and UID/GID.
		 * The mod time flag is included here as the "touch" utility
		 * passes it when a request is made to update the access
		 * time.  It's harmless as mod time is not allowed to
		 * change (see setattr below).
		 */
		if (mask & ~(AT_MODE|AT_ATIME|AT_MTIME|AT_UID|AT_GID)) {
			return (EPERM);
		}

		if (mask & AT_MODE) {
			/*
			 * Something other than the read mode bits are
			 * being changed.
			 */
			if (mode & WXMASK) {
				return (EPERM);
			}

			/*
			 * A mode change which would reset the SUID bit.
			 */
			if (!SUID_SET(mode) && SUID_SET(ip->di.mode)) {
				return (EPERM);
			}
		}
	}
	return (0);
}


/*
 * ----- sam_worm_trigger - Set the WORM bit on a SAM-QFS file.
 * The NAS WORM trigger (chmod 4000) 53XX mode or remove write
 * perimission for Compatibility mode.
 */

static int
sam_worm_trigger(sam_node_t *ip, sam_mode_t oldmode, timespec_t system_time)
{
	struct sam_sbinfo *sblk = &ip->mp->mi.m_sbp->info.sb;
	boolean_t compat_mode = 0;
	vnode_t *vp;
	sam_node_t *pip;
	sam_time_t parent_def_retention = -1;
	sam_mount_t	*mp = ip->mp;

	vp = SAM_ITOV(ip);

	/*
	 * In 4.6 there are two modes of WORM trigger operation.
	 * One is compatible with the 53xx SUN NAS series. This mode
	 * uses the SUID bit by itself.  If the setuid is to be set
	 * and no execute bits were set on a file, then the WORM
	 * bit should be set.  In addition, the new access mode will
	 * be the old access mode OR'ed with any read bits and the
	 * setuid bit.   The second mode is called compatibility mode.
	 * This mode uses the transition from a writeable mode as the
	 * trigger.
	 */
	if (sam_check_worm_capable(ip, TRUE) == 0) {

		ASSERT(ip->di.version >= SAM_INODE_VERS_2);

		compat_mode = ((ip->mp->mt.fi_config & MT_ALLEMUL) != 0);

		/*
		 * If any execute bit is set and compat mode is not set,
		 * (eg 53xx SUN NAS mode) reset the mode bits and
		 * return an error.
		 */
		if (!compat_mode &&
		    (oldmode & XMASK) && (vp->v_type != VDIR)) {
			ip->di.mode = oldmode;
			return (EACCES);
		}

		/*
		 * We don't want to set the SUID bit on directories
		 * (53XX NAS mode).  We do want to set the retention
		 * period on directories.  This period will be used
		 * to set the default retention period on a file in
		 * the directory.
		 */
		if (vp->v_type != VDIR) {
			/*
			 * The flag in the superblock needs to be modified
			 * indicating WORM is active in this volume.  This is
			 * done to protect the volume from being destroyed
			 * (eg sammkfs).  Do this only once unless we're
			 * upgrading from a lite mode.  If a WORM option
			 * was set with a previous trigger, do not update
			 * the superblock again.  If we're upgrading from a
			 * lite mode the volume's superblock needs to be
			 * updated to reflect the stricter mode.  The mount
			 * code verifies and doesn't allow multiple WORM
			 * options to be set.
			 */
			if (SBLK_UPDATE(sblk, ip) &&
			    (sblk->opt_mask_ver == SBLK_OPT_VER1)) {
				boolean_t	update_sblk = 0;

				if (WORM_MT_OPT(ip->mp) && !SBLK_WORM(sblk)) {
					sblk->opt_mask |= SBLK_OPTV1_WORM;
					sblk->opt_mask &= ~SBLK_OPTV1_WORM_LITE;
					update_sblk = 1;
				}

				if (EMUL_MT_OPT(mp) && !SBLK_WORM_EMUL(sblk)) {
					sblk->opt_mask |= SBLK_OPTV1_WORM_EMUL;
					sblk->opt_mask &= ~SBLK_OPTV1_EMUL_LITE;
					update_sblk = 1;
				}

				if (WORM_LITE_MT_OPT(mp) &&
				    !(SBLK_WORM_LITE(sblk) &&
				    WORM_LITE_MT_OPT(mp))) {
					sblk->opt_mask |= SBLK_OPTV1_WORM_LITE;
					update_sblk = 1;
				}

				if (EMUL_LITE_MT_OPT(mp) &&
				    !(SBLK_EMUL_LITE(sblk) &&
				    EMUL_LITE_MT_OPT(mp))) {
					sblk->opt_mask |= SBLK_OPTV1_EMUL_LITE;
					update_sblk = 1;
				}

				if (update_sblk) {
					sblk->opt_mask |=
					    SBLK_OPTV1_CONV_WORMV2;
					(void) sam_update_all_sblks(mp);
				}
			}
		} else {
			/*
			 * Don't set the SUID bit on directories.
			 */
			ip->di.mode &= ~S_ISUID;
			ip->di.mode |= oldmode;
		}

		/*
		 * If this is the first time setting the WORM
		 * bit and the retention time period has not been
		 * specified the default retention period will
		 * be used.  The retention period is stored as a
		 * number of minutes in the rperiod_duration field.
		 * Else, pickup the access time as it contains
		 * the desired retention period.
		 */
		if (!ip->di.status.b.worm_rdonly) {
			if (ip->di.access_time.tv_sec <= system_time.tv_sec) {
				/*
				 * Check to see if the parent directory has a
				 * default retention.  If so, pass it to the
				 * child.
				 */
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				if (sam_get_ino(ip->mp->mi.m_vfsp, IG_EXISTS,
				    &ip->di.parent_id, &pip) == 0) {
					RW_LOCK_OS(&pip->inode_rwl, RW_READER);
					if (pip->di.status.b.worm_rdonly) {
						ip->di2.rperiod_duration =
						    parent_def_retention =
						    pip->di2.rperiod_duration;
					}
					RW_UNLOCK_OS(&pip->inode_rwl,
					    RW_READER);
				}
				VN_RELE(SAM_ITOV(pip));
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);

				/*
				 * If we have't assigned a retention
				 * set the period to the system default.
				 */
				if (parent_def_retention == -1) {
					ip->di2.rperiod_duration =
					    ip->mp->mt.fi_def_retention;
				}
			} else if (ip->di.access_time.tv_sec == INT_MAX) {
				/*
				 * The access time was set to its largest
				 * value. The user wants permanent retention.
				 */
				ip->di2.rperiod_duration = 0;
			} else {
				/*
				 * The access time was advanced. The user
				 * has set the retention period to some
				 * point in the future.
				 */
				ip->di2.rperiod_duration = 1 +
				    (ip->di.access_time.tv_sec -
				    system_time.tv_sec)/60;
			}
			ip->di2.rperiod_start_time =
			    system_time.tv_sec;
			ip->di2.p2flags |= P2FLAGS_WORM_V2;
			TRANS_INODE(ip->mp, ip);
			sam_mark_ino(ip, SAM_CHANGED);
		}
		ip->di.status.b.worm_rdonly = 1;
	}
	return (0);
}


/*
 * ----- sam_set_rperiod - Set the retention period on a
 * SAM-QFS file. Set or extend the retention period per
 * the NAS interface. INT_MAX means permanent retention
 * and is stored as 0. Ensure the period is valid (not
 * regressing in time).
 */

static int
sam_set_rperiod(
	sam_node_t *ip,		/* pointer to inode. */
	vattr_t *vap,		/* vattr pointer. */
	boolean_t priv_lite)	/* WORM lite, privileged user */
{
	int error = 0;

	ASSERT(ip->di.version >= SAM_INODE_VERS_2);

	if (priv_lite) {
		if (vap->va_atime.tv_sec == INT_MAX) {
			ip->di2.rperiod_duration = 0;
		} else {
			ip->di2.rperiod_duration = 1 +
			    (vap->va_atime.tv_sec -
			    ip->di2.rperiod_start_time)/60;
		}
	} else if (ip->di2.rperiod_duration == 0) {
		error = EINVAL;
	} else if (vap->va_atime.tv_sec == INT_MAX) {
		ip->di2.rperiod_duration = 0;
	} else if ((ip->di2.rperiod_duration != 0) &&
	    (vap->va_atime.tv_sec >
	    (ip->di2.rperiod_start_time +
	    (ip->di2.rperiod_duration * 60)))) {
		ip->di2.rperiod_duration = 1 +
		    (vap->va_atime.tv_sec -
		    ip->di2.rperiod_start_time)/60;
	} else {
		error = EINVAL;
	}
	return (error);
}


/*
 * ----- sam_setattr_ino - Set attributes call.
 * Set attributes for a SAM-QFS file.
 */

int				/* ERRNO if error, 0 if successful. */
sam_setattr_ino(
	sam_node_t *ip,		/* pointer to inode. */
	vattr_t *vap,		/* vattr pointer. */
	int flags,		/* flags. */
	cred_t *credp)		/* credentials pointer. */
{
	uint_t mask;
	int error = 0;
	vnode_t *vp;
	sam_mode_t oldmode, mode;
	timespec_t  system_time;
	vattr_t oldva;

	oldva.va_mode = ip->di.mode;
	oldva.va_uid = ip->di.uid;
	oldva.va_gid = ip->di.gid;

	vp = SAM_ITOV(ip);
	if (vap->va_mask & AT_NOSET) {
		return (EINVAL);
	}
	mode = vap->va_mode & ~S_IFMT;
	SAM_HRESTIME(&system_time);

	/*
	 * Enforce the "read only" portion of WORM files.
	 */
	if (ip->di.status.b.worm_rdonly && !S_ISDIR(ip->di.mode)) {
		error = sam_chk_worm(mode, vap->va_mask, ip);
		if (error) {
			return (error);
		}
	}

	/*
	 * Generic setattr security policy check.
	 */
	if (error = secpolicy_vnode_setattr(credp, vp, vap,
	    &oldva, flags, sam_access_ino_ul, ip)) {
		return (error);
	}

	mask = vap->va_mask;

	if (mask & AT_SIZE) {		/* -----Change size */
		if (error == 0) {
			/* Can only truncate a regular file */
			if (S_ISREQ(ip->di.mode)) {
				error = EINVAL;
				goto out;
			} else if (SAM_PRIVILEGE_INO(ip->di.version,
			    ip->di.id.ino)) {
				error = EPERM;	/* Can't trunc priv'ed inodes */
				goto out;
			}
			if (S_ISSEGI(&ip->di) && (vap->va_size != 0)) {
				/*
				 * If file is segment access and not truncating
				 * to zero--fix.
				 */
				error = EINVAL;
				goto out;
			}
			/*
			 * Might need to do TRANS_ITRUNC here for LQFS....
			 */
			if ((error = sam_clear_ino(ip, (offset_t)vap->va_size,
			    STALE_ARCHIVE, credp))) {
				goto out;
			}
		}
	}

	if (mask & AT_MODE) {				/* -----Change mode */
		/* Cannot change .inodes file */
		if (ip->di.id.ino == SAM_INO_INO) {
			error = EPERM;
			goto out;
		}
		oldmode = ip->di.mode;
		ip->di.mode &= S_IFMT;
		ip->di.mode |= vap->va_mode & ~S_IFMT;
		if (ip->di.status.b.worm_rdonly) {
			if (!S_ISDIR(ip->di.mode)) {
				ip->di.mode &= ~WMASK;
			}
			if (oldmode & S_ISUID) {
				ip->di.mode |= S_ISUID;
			}
		}

		/*
		 * In 4.6 there are two modes of WORM trigger operation.
		 * One is compatible with the 53xx SUN NAS series.  This
		 * mode uses the SUID bit by itself.  The second mode is
		 * called compatibility mode.  This mode uses the transition
		 * from a writeable mode as the trigger. Note, copying a
		 * read-only file to a WORM capable volume does *NOT*
		 * initiate the WORM trigger in this mode.
		 */
		if (samgt.license.license.lic_u.b.WORM_fs &&
		    (ip->di.version >= SAM_INODE_VERS_2) &&
		    (((vap->va_mode == S_ISUID) &&
		    (ip->mp->mt.fi_config & MT_ALLWORM)) ||
		    ((ip->mp->mt.fi_config & MT_ALLEMUL) &&
		    (((oldmode & RWXALLMASK) == RWXALLMASK) ||
		    ((vap->va_mode != S_ISUID) &&
		    (oldmode & WMASK) && !(ip->di.mode & WMASK)))))) {
			error = sam_worm_trigger(ip, oldmode, system_time);
			if (error) {
				ip->di.mode = oldmode;
				goto out;
			} else if ((ip->mp->mt.fi_config & MT_ALLEMUL) &&
			    ((oldmode & RWXALLMASK) == RWXALLMASK)) {
				ip->di.mode = oldmode;
			} else if ((vap->va_mode == S_ISUID) &&
			    (!S_ISDIR(ip->di.mode))) {
				if (ip->mp->mt.fi_config & MT_ALLEMUL) {
					ip->di.mode = oldmode &
					    (S_IFMT | RMASK);
				} else {
					ip->di.mode = S_ISUID |
					    (oldmode & (S_IFMT | RMASK));
				}
			}
		}
		TRANS_INODE(ip->mp, ip);
		if (S_ISATTRDIR(oldmode)) {
			ip->di.mode |= S_IFATTRDIR;
		}
		sam_mark_ino(ip, SAM_CHANGED);
	}

	if (mask & (AT_UID | AT_GID)) {		/* -----Change uid/gid */
		int ouid, ogid;

		if (vap->va_mask & AT_MODE) {
			ip->di.mode = (ip->di.mode & S_IFMT) |
			    (vap->va_mode & ~S_IFMT);
		}
		/*
		 * To change file ownership, a process must have
		 * privilege if:
		 *
		 * If it is not the owner of the file, or
		 * if doing restricted chown semantics and
		 * either changing the ownership to someone else or
		 * changing the group to a group that we are not
		 * currently in.
		 */
		if (crgetuid(credp) != ip->di.uid ||
		    (rstchown &&
		    (((mask & AT_UID) && vap->va_uid != ip->di.uid) ||
		    ((mask & AT_GID) && !groupmember(vap->va_gid, credp))))) {
			error = secpolicy_vnode_owner(credp, vap->va_uid);
			if (error) {
				goto out;
			}
		}

		ouid = ip->di.uid;
		ogid = ip->di.gid;
		if (error = sam_quota_chown(ip->mp, ip,
		    (mask&AT_UID) ? vap->va_uid : ouid,
		    (mask&AT_GID) ? vap->va_gid : ogid, credp)) {
			goto out;
		}
		if (mask & AT_UID)  ip->di.uid = vap->va_uid;
		if (mask & AT_GID)  ip->di.gid = vap->va_gid;
		ip->di.status.b.archdone = 0;
		TRANS_INODE(ip->mp, ip);
		sam_mark_ino(ip, SAM_CHANGED);
		/*
		 * Notify arfind and event daemon of setattr.
		 */
		sam_send_to_arfind(ip, AE_change, 0);
		if (ip->mp->ms.m_fsev_buf) {
			sam_send_event(ip->mp, &ip->di, ev_change, 0, 0,
			    ip->di.change_time.tv_sec);
		}
	}

	if (mask & (AT_ATIME | AT_MTIME)) {	/* -----Modify times */
		/*
		 * Synchronously flush pages so dates do not get changed after
		 * utime.  If staging, finish stage and then flush pages.
		 */
		if (ip->flags.b.staging) {
			/*
			 * Might need to do TRANS_ITRUNC or similar here
			 * for LQFS
			 */
			if ((error = sam_clear_file(ip, ip->di.rm.size,
			    MAKE_ONLINE, credp))) {
				goto out;
			}
		}
		sam_flush_pages(ip, 0);
		if (mask & AT_ATIME) {
			/*
			 * The access time field is used by WORM operations to
			 * store the retention timestamp.  This is intercepted
			 * here and stored in the inode's retention period
			 * time fields if either the field hasn't been set or
			 * the provided value exceeds what is currently there
			 * (i.e. we're extending the period).
			 */
			error = sam_check_worm_capable(ip, TRUE);
			if (!error) {
				boolean_t   lite_mode =
				    ((ip->mp->mt.fi_config &
				    MT_LITE_WORM) != 0);
				boolean_t   is_priv =
				    (secpolicy_fs_config(credp,
				    ip->mp->mi.m_vfsp) == 0);

				if (S_ISREG(ip->di.mode) && !WORM(ip)) {
					/*
					 * Regular file in WORM capable
					 * directory.  Set access time per the
					 * request.
					 */
					ip->di.access_time.tv_sec =
					    vap->va_atime.tv_sec;
					ip->di.access_time.tv_nsec =
					    vap->va_atime.tv_nsec;
					ip->flags.b.accessed = 1;
				} else if (WORM(ip) &&
				    (ip->di.version >= SAM_INODE_VERS_2)) {
					boolean_t	extend_period;
					/*
					 * Extend the retention period if so
					 * requested. If lite mode and a
					 * privileged user or a directory allow
					 * the retention period to be shortened.
					 */

					if (vap->va_atime.tv_sec >
					    ip->di2.rperiod_start_time +
					    ip->di2.rperiod_duration * 60) {
						extend_period = 1;
					} else {
						extend_period = 0;
					}

					if (S_ISREG(ip->di.mode) &&
					    extend_period) {
						error = sam_set_rperiod(ip,
						    vap, is_priv &&
						    lite_mode);
					} else if (S_ISDIR(ip->di.mode) ||
					    (S_ISREG(ip->di.mode) &&
					    is_priv && lite_mode)) {
						/*
						 * If the requested time would
						 * result in a non- negative
						 * retention period, set the
						 * period to the difference of
						 * the request and current time.
						 * A negative retention period
						 * is not allowed.
						 */
						if (vap->va_atime.tv_sec >
						    system_time.tv_sec) {


			if (vap->va_atime.tv_sec == INT_MAX) {
				ip->di2.rperiod_duration = 0;
			} else {
				ip->di2.rperiod_duration = 1 +
				    (vap->va_atime.tv_sec -
				    system_time.tv_sec)/60;
			}


						} else {
							error = EINVAL;
						}
					}
					if (error) {
						goto out;
					}
				} else {
					/*
					 * Shouldn't get here, invalid request.
					 */
					error = EINVAL;
					goto out;
				}
				TRANS_INODE(ip->mp, ip);
				sam_mark_ino(ip, SAM_CHANGED);
			} else {
				error = 0;
				ip->di.access_time.tv_sec =
				    vap->va_atime.tv_sec;
				ip->di.access_time.tv_nsec =
				    vap->va_atime.tv_nsec;
				ip->flags.b.accessed = 1;
				TRANS_INODE(ip->mp, ip);
			}
		}
		if (mask & AT_MTIME) {
			if (!ip->di.status.b.worm_rdonly) {
				ip->di.modify_time.tv_sec =
				    vap->va_mtime.tv_sec;
				ip->di.modify_time.tv_nsec =
				    vap->va_mtime.tv_nsec;
				ip->di.change_time.tv_sec = system_time.tv_sec;
				ip->di.change_time.tv_nsec =
				    system_time.tv_nsec;
				ip->flags.b.updated = 1;
				/* Modify time has been set */
				ip->flags.b.dirty = 1;
			}
			TRANS_INODE(ip->mp, ip);
		}
	}

	/*
	 * Check for and apply ACL info, if present.
	 */
	if (ip->di.status.b.acl && !ip->di.status.b.worm_rdonly) {
		if (SAM_IS_SHARED_FS(ip->mp) && SAM_IS_SHARED_SERVER(ip->mp)) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			sam_callout_acl(ip, ip->mp->ms.m_client_ord);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
		if (error = sam_acl_setattr(ip, vap)) {
			goto out;
		}
	}
	if (ip->mp->mt.fi_config & MT_SHARED_WRITER) {
		if ((error == 0) &&
		    (ip->flags.bits & (SAM_ACCESSED|SAM_UPDATED|SAM_CHANGED))) {
			(void) sam_update_inode(ip, SAM_SYNC_ONE, FALSE);
		}
	}
out:

	return (error);
}


/*
 * ----- sam_refresh_shared_reader_ino -
 * Validate shared_reader inode when last update time + fi_invalid
 * is greater than current time. If an fi_invalid is set > 0 (mount option,
 * "invalid"), stale information is permitted.  If validating, read
 * the disk copy. If the inode has been modified since the last check,
 * update inode, reset size, and invalidate pages.
 */

int
sam_refresh_shared_reader_ino(
	sam_node_t *ip,			/* Pointer to the inode */
	boolean_t writelock,		/* Inode WRITER lock held, */
					/*   otherwise READER lock held. */
	cred_t *credp)			/* credentials. */
{
	sam_id_t id;
	struct sam_perm_inode *permip;
	buf_t *bp;
	int refresh = 0;
	int error;

	if ((ip->updtime + ip->mp->mt.fi_invalid) > SAM_SECOND()) {
		return (0);
	}

	if (!writelock) {
		/*
		 * Acquire inode lock before buffer lock. Recheck the update
		 * time.
		 */
		if (!rw_tryupgrade(&ip->inode_rwl)) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			if ((ip->updtime + ip->mp->mt.fi_invalid) >
			    SAM_SECOND()) {
				error = 0;
				goto out;
			}
		}
	}
	id = ip->di.id;
	if ((error = sam_read_ino(ip->mp, id.ino, &bp, &permip))) {
		goto out;
	}
	if ((permip->di.mode != 0) && (permip->di.id.ino == ip->di.id.ino) &&
	    (permip->di.id.gen == ip->di.id.gen)) {
		if ((permip->di.modify_time.tv_sec !=
		    ip->di.modify_time.tv_sec) ||
		    (permip->di.modify_time.tv_nsec !=
		    ip->di.modify_time.tv_nsec)||
		    (permip->di.change_time.tv_sec !=
		    ip->di.change_time.tv_sec) ||
		    (permip->di.change_time.tv_nsec !=
		    ip->di.change_time.tv_nsec)||
		    (permip->di.residence_time != ip->di.residence_time) ||
		    (permip->di.rm.size != ip->di.rm.size) ||
		    (permip->di.mode != ip->di.mode)) {
			refresh = 1;
		} else {
			ip->di.uid = permip->di.uid;
			ip->di.gid = permip->di.gid;
		}
	} else {
		refresh = 1;
		error = ENOENT;		/* This inode has been removed */
	}
	if (refresh) {
		vnode_t *vp = SAM_ITOV(ip);

		/*
		 * If a refresh is needed on a directory inode,
		 * invalidate associated dnlc entries.
		 */
		if (S_ISDIR(ip->di.mode)) {
			sam_invalidate_dnlc(vp);
		}

		/*
		 * Move shared_writer's inode copy into inode. Set size
		 * and invalidate pages. Set shared_reader update time.
		 */
		ip->di = permip->di; /* Move disk ino to incore ino */
		ip->di2 = permip->di2;
		brelse(bp);
		vp->v_type = IFTOVT(S_ISREQ(ip->di.mode) ?
		    S_IFREG : ip->di.mode);
		sam_set_size(ip);
		(void) VOP_PUTPAGE_OS(vp, 0, 0, B_INVAL, credp, NULL);
		if (ip->di.status.b.acl) {
			(void) sam_acl_inactive(ip);
			error = sam_get_acl(ip, &ip->aclp);
		}
		ip->updtime = SAM_SECOND();
	} else {
		ip->updtime = SAM_SECOND();
		brelse(bp);
	}

out:
	if (!writelock) {
		rw_downgrade(&ip->inode_rwl);
	}
	return (error);
}


/*
 * ----- sam_trunc_excess_blocks - truncate excess blocks.
 * Called when the file is closed or inactivated.
 * Do not truncate if the file is object or was preallocated.
 */

int
sam_trunc_excess_blocks(sam_node_t *ip)
{
	int dau_size;
	int num_group;
	int error = 0;

	if (SAM_IS_OBJECT_FILE(ip)) {
		return (0);
	}
	ip->cl_hold_blocks = 0;
	if (ip->di.status.b.direct_map == 0) {
		dau_size = LG_BLK(ip->mp, ip->di.status.b.meta);
		num_group = ip->mp->mi.m_fs[ip->di.unit].num_group;
		if (num_group > 1) {
			dau_size *= num_group;
		}
		if (ip->cl_allocsz >= (ip->size + dau_size)) {
			offset_t length;

			if (ip->di.status.b.offline) {
				length = ip->di.rm.size;
			} else {
				length = ip->size;
			}
			if ((error = sam_proc_truncate(ip, length, SAM_REDUCE,
			    CRED())) == 0) {
				sam_set_size(ip);
				ip->cl_allocsz = ip->size;
			}
		}
	}
	sam_clear_map_cache(ip);
	return (error);
}


/*
 * ----- sam_read_ino - Read inode and return buffer pointer.
 *	Read .inodes file block and return buffer pointer.
 */

int					/* ERRNO if error, 0 if successful. */
sam_read_ino(
	sam_mount_t *mp,		/* pointer to the mount table. */
	sam_ino_t ino,			/* i-number. */
	buf_t **ibp,			/* .inodes file buffer (returned). */
	struct sam_perm_inode **permip) /* out: inode data. */
{
	sam_node_t *dip;
	offset_t offset;
	sam_ioblk_t ioblk;
	sam_daddr_t bn;
	int error = 0;
	buf_t *bp;
	offset_t size;

	/*
	 * For shared_reader or shared reader client, buffer cache
	 * inode is 512 bytes. Regular filesystem caches inodes in 16k buffer.
	 */
	if (SAM_IS_SHARED_CLIENT(mp)) {
		sam_id_t id;

		id.ino = ino;
		id.gen = 0;
		if ((error = sam_get_client_inode(mp, &id, SHARE_wait,
		    SAM_ISIZE,
		    &bp)) == 0) {
			*ibp = bp;
			*permip =
			    (struct sam_perm_inode *)(void *)SAM_ITOO(ino,
			    SAM_ISIZE, bp);

			/*
			 * Stale inode buffer for shared client.
			 */
			bp->b_flags |= B_STALE | B_AGE;
		}
		if (SAM_IS_SHARED_CLIENT(mp)) {
			return (error);
		}
		if (!error) {
			TRACE(T_SAM_FAILOVER_WIN, NULL, 2, id.ino, 0);
			brelse(bp);
		}
	}

	/*
	 * Read rd_ino_buf_size bytes for multi-reader and standard file system.
	 */
	size = mp->mt.fi_rd_ino_buf_size;

	dip = mp->mi.m_inodir;
	if (ino != SAM_INO_INO) {
		if (dip == NULL) {
			return (ENXIO);
		}

		/*
		 * Read inode block from directory .inodes file.
		 */
		offset = (offset_t)SAM_ITOD(ino);
		if (ino == 0) {
			return (ENOENT);
		}

		if (SAM_IS_SHARED_READER(mp)) {
			if (offset >= dip->di.rm.size) {
				struct sam_disk_inode *dp;

				/*
				 * Reread the .inode inode from disk and replace
				 * it.
				 */
				bn = dip->di.extent[0];
				bn <<= mp->mi.m_bn_shift;
				RW_LOCK_OS(&dip->inode_rwl, RW_READER);
				error = SAM_BREAD(mp,
				    mp->mi.m_fs[dip->di.extent_ord[0]].dev,
				    bn, size, &bp);
				if (error) {
					RW_UNLOCK_OS(&dip->inode_rwl,
					    RW_READER);
					return (error);
				}
				dp = SAM_ITOO(dip->di.id.ino, size, bp);
				*(&dip->di) = *dp;
				dip->size = dip->di.rm.size;
				bp->b_flags |= B_STALE | B_AGE;
				brelse(bp);
				RW_UNLOCK_OS(&dip->inode_rwl, RW_READER);
			}
			if (offset >= dip->di.rm.size) {
				return (ENOENT);
			}
		} else {
			if (offset > (dip->di.rm.size +
			    mp->mt.fi_rd_ino_buf_size)) {
				return (ENOENT);
			}
		}

		/*
		 * If inode is within .inodes size, read the inode.
		 * If inode is outside .inodes size, extend the .inodes file.
		 */
		if (offset < dip->di.rm.size) {
			/* i-number within existing .inodes */
			RW_LOCK_OS(&dip->inode_rwl, RW_READER);
			error = sam_map_block(dip, offset, size,
			    SAM_READ, &ioblk, CRED());
			if (error) {
				if (IS_SAM_ENOSPC(error)) {
					error = ENOENT;
				}
				RW_UNLOCK_OS(&dip->inode_rwl, RW_READER);
				return (error);
			}
			if (ioblk.count == 0) {
				/* Append to inode directory file */
				RW_UNLOCK_OS(&dip->inode_rwl, RW_READER);
				return (ENOENT);
			}
			ioblk.blkno += (ioblk.pboff & ~(size - 1)) >>
			    SAM_DEV_BSHIFT;
			error = SAM_BREAD(mp, mp->mi.m_fs[ioblk.ord].dev,
			    ioblk.blkno, size, &bp);
			RW_UNLOCK_OS(&dip->inode_rwl, RW_READER);

		} else {
			/*
			 * i-number exceeds existing .inodes, append a large DAU
			 */
			offset_t maxsize;
			struct buf *first_bp;

			if (SAM_IS_CLIENT_OR_READER(mp)) {
				cmn_err(CE_PANIC,
				    "SAM-QFS: %s: read_ino ino=%d",
				    mp->mt.fi_name, ino);
			}
			maxsize = INO_ALLOC_SIZE;
			first_bp = NULL;
			while (maxsize > 0) {
				RW_LOCK_OS(&dip->inode_rwl, RW_WRITER);
				error = sam_map_block(dip, offset,
				    size, SAM_WRITE_BLOCK,
				    &ioblk, CRED());
				RW_UNLOCK_OS(&dip->inode_rwl, RW_WRITER);
				if (error) {
					if (first_bp) {
						error = 0;
						break;
					}
					return (error);
				}
				ioblk.blkno += (ioblk.pboff &
				    ~(size - 1)) >>SAM_DEV_BSHIFT;
				bp = getblk(mp->mi.m_fs[ioblk.ord].dev,
				    ioblk.blkno << SAM2SUN_BSHIFT, size);
				clrbuf(bp);
				error = SAM_BWRITE2(mp, bp);
				if (error) {
					dip->di.rm.size -= size;
					bp->b_flags |= B_STALE | B_AGE;
					brelse(bp);
					break;
				}
				offset += size;
				maxsize -= size;
				if (first_bp == NULL) {
					first_bp = bp;
				} else {
					bp->b_flags |= B_AGE;
					/* Release all buffers but the first */
					brelse(bp);
				}
			}
			if (error == 0) {
				RW_LOCK_OS(&dip->inode_rwl, RW_WRITER);
				TRANS_INODE(dip->mp, dip);
				sam_mark_ino(dip, (SAM_UPDATED | SAM_CHANGED));
				if ((TRANS_ISTRANS(dip->mp) == 0) &&
				    (SAM_SYNC_META(mp))) {
					(void) sam_update_inode(dip,
					    SAM_SYNC_ONE, FALSE);
				}
				RW_UNLOCK_OS(&dip->inode_rwl, RW_WRITER);
				bp = first_bp;
			}
		}

	} else {
		/*
		 * First inode read, get directory .inodes file start from sblk.
		 */
		error = SAM_BREAD(mp, mp->mi.m_fs[mp->mi.m_dk_start[MM]].dev,
		    mp->mi.m_sbp->info.sb.inodes, size, &bp);
		if (error == 0) {
			struct sam_disk_inode *dp;

			dp = SAM_ITOO(ino, size, bp);
			bn = dp->extent[0];
			bn <<= mp->mi.m_bn_shift;
			if (dp->id.ino != SAM_INO_INO ||
			    dp->id.gen != SAM_INO_INO ||
			    (bn != mp->mi.m_sbp->info.sb.inodes)) {
				cmn_err(CE_WARN,
"SAM-QFS: %s: block %llx on eq %d is invalid, %d.%d, extent=%x, rt=%x, st=%x",
				    mp->mt.fi_name,
				    mp->mi.m_sbp->info.sb.inodes,
				    mp->mi.m_fs[
				    mp->mi.m_dk_start[MM]].part.pt_eq,
				    dp->id.ino, dp->id.gen, dp->extent[0],
				    dp->residence_time, dp->status.bits);
				brelse(bp);
				return (EINVAL);
			}
		}
	}
	if (error == 0) {
		*ibp = bp;
		*permip = (struct sam_perm_inode *)(void *)SAM_ITOO(
		    ino, size, bp);

		/*
		 * Stale inode buffer for shared_reader and shared client.
		 */
		if (SAM_IS_CLIENT_OR_READER(mp)) {
			bp->b_flags |= B_STALE | B_AGE;
		}
	}
	return (error);
}


/*
 * ----- sam_lockfs -
 * Lock this file system or unlock this file system.
 */

int			/* ERRNO if an error occured, 0 if successful. */
sam_lockfs(sam_mount_t *mp, struct lockfs *lockp, int flag)
{
	TRACE(T_SAM_LOCKFS, mp, lockp->lf_lock, flag, mp->mt.fi_status);
#if SAM_LOCKFS
	mutex_enter(&mp->ms.m_waitwr_mutex);
	if (LOCKFS_IS_ULOCK(lockp)) {
		mp->mt.fi_status &= ~FS_LOCKFS;
		cv_broadcast(&mp->ms.m_waitwr_cv);
	} else if (LOCKFS_IS_WLOCK(lockp)) {
		mp->mt.fi_status |= FS_LOCK_WRITE;
	} else if (LOCKFS_IS_NLOCK(lockp)) {
		mp->mt.fi_status |= FS_LOCK_NAME;
	} else if (LOCKFS_IS_DLOCK(lockp)) {
		mp->mt.fi_status |= FS_LOCK_RM_NAME;
	} else if (LOCKFS_IS_HLOCK(lockp)) {
		mp->mt.fi_status |= FS_LOCK_HARD;
	}
	mutex_exit(&mp->ms.m_waitwr_mutex);
	return (0);
#else /* SAM_LOCKFS */
	return (EINVAL);
#endif /* SAM_LOCKFS */
}


/*
 * ----- sam_lockfs_status -
 * Get lockfs status for this file system.
 */

int				/* ERRNO, else 0 if successful. */
sam_lockfs_status(sam_mount_t *mp, struct lockfs *lockp, int flag)
{
	TRACE(T_SAM_LOCKFS_ST, mp, lockp->lf_lock, flag, mp->mt.fi_status);
	mutex_enter(&mp->ms.m_waitwr_mutex);
	lockp->lf_lock = 0;
	if (mp->mt.fi_status & FS_LOCK_WRITE) {
		lockp->lf_lock = LOCKFS_WLOCK;
	} else if (mp->mt.fi_status & FS_LOCK_NAME) {
		lockp->lf_lock = LOCKFS_NLOCK;
	} else if (mp->mt.fi_status & FS_LOCK_RM_NAME) {
		lockp->lf_lock = LOCKFS_DLOCK;
	} else if (mp->mt.fi_status & FS_LOCK_HARD) {
		lockp->lf_lock = LOCKFS_HLOCK;
	}
	mutex_exit(&mp->ms.m_waitwr_mutex);
	return (0);
}


/* ARGSUSED4 */
int				/* ERRNO if error, 0 if successful. */
sam_proc_lockfs(
	sam_node_t *ip,		/* Pointer to inode. */
	int cmd,		/* Command number. */
	int *arg,		/* Pointer to arguments. */
	int flag,		/* Datamodel */
	int *rvp,		/* Returned value pointer */
	cred_t *credp)		/* Credentials pointer.	*/
{
	uint64_t *lp;
	struct lockfs lockfs;
	int error = 0;

	if (secpolicy_fs_config(credp, ip->mp->mi.m_vfsp)) {
		return (EPERM);
	}
	lp = (uint64_t *)(void *)arg;
	if ((flag & DATAMODEL_MASK) == DATAMODEL_LP64) {
		if (copyin((caddr_t)*lp, &lockfs, sizeof (struct lockfs))) {
			return (EFAULT);
		}
	} else {
		struct lockfs32 lockfs32;

		if (copyin((caddr_t)*lp, &lockfs32, sizeof (struct lockfs32))) {
			return (EFAULT);
		}
		lockfs.lf_lock = (ulong_t)lockfs32.lf_lock;
		lockfs.lf_flags = (ulong_t)lockfs32.lf_flags;
		lockfs.lf_key = (ulong_t)lockfs32.lf_key;
		lockfs.lf_comlen = (ulong_t)lockfs32.lf_comlen;
		lockfs.lf_comment = (caddr_t)lockfs32.lf_comment;
	}
	if (cmd == C_FIOLFSS) {
		error = sam_lockfs_status(ip->mp, &lockfs, 0);
	} else {
		error = sam_lockfs(ip->mp, &lockfs, 0);
	}
	if (error == 0) {
		if ((flag & DATAMODEL_MASK) == DATAMODEL_LP64) {
			(void) copyout(&lockfs, (caddr_t)*lp,
			    sizeof (struct lockfs));
		} else {
			struct lockfs32 lockfs32;
			lockfs32.lf_lock = (uint32_t)lockfs.lf_lock;
			lockfs32.lf_flags = (uint32_t)lockfs.lf_flags;
			lockfs32.lf_key = (uint32_t)lockfs.lf_key;
			lockfs32.lf_comlen = (uint32_t)lockfs.lf_comlen;
			lockfs32.lf_comment = (uint32_t)lockfs.lf_comment;
			(void) copyout(&lockfs32, (caddr_t)*lp,
			    sizeof (struct lockfs32));
		}
	}
	return (error);
}


/*
 * ----- sam_inodes_count - Count the free user i-nodes in the first 'count'
 *	and last 'count' entries of .inodes.  Return the free count in
 *	'result'.  If error, return errno.  Otherwise, return 0.
 *	NOTE: Assumes that m_inode_mutex is held.
 */
int
sam_inodes_count(
	sam_mount_t *mp,	/* Mount table pointer */
	int count,		/* Half the # of .inodes i-nodes to validate */
	int32_t *result)	/* Returned count of free i-nodes */
{
	buf_t *bp;
	sam_node_t *dip;
	struct sam_perm_inode *pip;
	int32_t min_user_inum;
	int32_t max_inum;
	int32_t bottom_inodes_end;
	int32_t top_inodes_start;
	int32_t cur_ino = SAM_INO_INO;
	int32_t cnt = 0;
	int error = 0;

	if ((mp == NULL) || (result == NULL)) {
		return (EINVAL);
	}

	dip = mp->mi.m_inodir;

	min_user_inum = mp->mi.m_min_usr_inum;	/* Min user i-node number */
	max_inum = ((dip->di.rm.size + (dip->di.status.b.direct_map ?
	    0 : mp->mt.fi_rd_ino_buf_size)) >> SAM_ISHIFT);
	bottom_inodes_end = min_user_inum + count - 1;
	if (bottom_inodes_end > max_inum) {
		bottom_inodes_end = max_inum;
	}
	top_inodes_start = max_inum - count + 1;
	if (top_inodes_start <= bottom_inodes_end) {
		top_inodes_start = bottom_inodes_end + 1;
	}

	cur_ino = min_user_inum;
	while (cur_ino <= max_inum) {
		if ((error = sam_read_ino(mp, cur_ino, &bp, &pip)) != 0) {
			/*
			 * Can't read i-node. I/O error?  error is errno (>0).
			 */
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: sam_inodes_count: "
			    "i-node %d is unreadable",
			    mp->mt.fi_name, cur_ino);
			break;
		}
		if (pip->di.mode == 0) {
			/* Free i-node - bump count. */
			cnt++;
		}
		brelse(bp);

		if (cur_ino != bottom_inodes_end) {
			cur_ino++;
		} else {
			cur_ino = top_inodes_start;
		}
	}

	*result = cnt;

	return (error);
}


/*
 * ----- sam_inode_freelist_walk - Walks the i-node free list and validates
 *	i-nodes, up to the maximum of 'count' free i-nodes.  If 'count'
 *	is 0, then only the .inodes i-node is validated.  If count is < 0,
 *	then all free list i-nodes are validated.
 *
 * If only valid i-nodes are found:
 *	Returns 0, and 'result' contains the count of free list i-nodes
 *	that were checked.  This value is useful to determine the count
 *	of free i-nodes, especially when i-nodes have been pre-allocated.
 *	If 'result'	is zero, then only the .inodes i-node was checked,
 *	and the free list is empty.
 *
 * If the next in-range i-node cannot be read (to see if it's allocated):
 *	Returns errno (> 0) to indicate the reason for the read failure, and
 *	'result' contains the i-node number of the i-node that can't be read.
 *
 * If an invalid i-node is found:
 *	Returns < 0, and 'result' contains the i-node number of the i-node
 *	that refers to an invalid i-node.
 */
int
sam_inode_freelist_walk(
	sam_mount_t *mp,	/* Mount table pointer */
	int count,		/* Number of free list i-nodes to validate */
	int32_t *result)	/* Result of completed check */
{
	buf_t *bp;
	sam_node_t *dip;
	struct sam_perm_inode *pip;
	int32_t min_user_inum;
	int32_t max_inum;
	int32_t next_ino;
	int32_t cur_ino = SAM_INO_INO;
	int32_t cnt = 0;
	int error = 0;

	if ((mp == NULL) || (result == NULL)) {
		return (EINVAL);
	}

	dip = mp->mi.m_inodir;

	mutex_enter(&mp->ms.m_inode_mutex);

	min_user_inum = mp->mi.m_min_usr_inum;	/* Min user i-node number */
	max_inum = ((dip->di.rm.size + (dip->di.status.b.direct_map ?
	    0 : mp->mt.fi_rd_ino_buf_size)) >> SAM_ISHIFT);

	/*
	 * Check starts with .inodes inode.  Ends when:
	 *	- Current i-node is at the end of the free list (next is 0), or
	 *	- Reach requested count of validated free list i-nodes, or
	 *	- Current i-node refers to an out-of-range i-node (return < 0),
	 *	  or
	 *	- Current i-node refers to an allocated i-node (return < 0), or
	 *	- Can't read the next free list i-node (return errno)
	 */
	next_ino = dip->di.free_ino;
	for (;;) {
		if (next_ino == 0) {	/* Reached end of free list */
			break;
		}

		/*
		 * Legacy pre-allocated inode may refer to one past the end
		 * if .inodes space, and indicates the end of the freelist.
		 */
		if ((dip->di.status.b.direct_map) &&
		    (next_ino == (max_inum + 1))) {
			break;
		}

		/*
		 * Check if current i-node refers to an out-of-range i-node
		 * number.
		 */
		if ((next_ino < min_user_inum) || (next_ino > max_inum)) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: sam_inode_freelist_walk: "
			    "i-node %d refers to out-of-range ino %d",
			    mp->mt.fi_name, cur_ino, next_ino);
			error = -1;
			break;
		}

		/* Check if current i-node refers to an allocated i-node */
		if ((error = sam_read_ino(mp, next_ino, &bp, &pip)) != 0) {
			/*
			 * next free list i-node is in-range, but we can't read
			 * it.  I/O error?  error is errno (>0).
			 */
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: sam_inode_freelist_walk: "
			    "i-node %d refers to unreadable ino %d",
			    mp->mt.fi_name, cur_ino, next_ino);
			cur_ino = next_ino;
			break;
		}
		if (pip->di.mode != 0) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: sam_inode_freelist_walk: "
			    "i-node %d refers to allocated ino %d",
			    mp->mt.fi_name, cur_ino, next_ino);
			brelse(bp);
			error = -1;
			break;
		}

		if (cnt == count) {	/* Reached requested count */
			brelse(bp);
			break;
		}

		cur_ino = next_ino;	/* Go validate the next i-node */
		next_ino = pip->di.free_ino;
		brelse(bp);
		cnt++;
	}

	if (error == 0) {
		/*
		 * In the case of a pre-allocated i-node FS whose freelist
		 * has never been initialized, the freelist will have fewer
		 * i-nodes than are actually available.  A samfsck will fix
		 * the freelist.  4.2.c depends on it.  Attempt to detect
		 * the condition within the first 'count' and last 'count'
		 * i-nodes in .inodes (the ordering has changed over time).
		 * If detected, return < 0 to force building the pre-allocated
		 * i-node freelist.
		 */
		if (dip->di.status.b.direct_map != 0) {
			int32_t free_icount;

			error = sam_inodes_count(mp, count, &free_icount);
			if ((error == 0) && (free_icount > cnt)) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_inode_freelist_walk: "
				    "Need to initialize pre-allocated "
				    "i-node freelist",
				    mp->mt.fi_name);
				mutex_exit(&mp->ms.m_inode_mutex);
				return (-2);
			}
		}
		*result = cnt;
	} else {
		*result = cur_ino;
	}

	mutex_exit(&mp->ms.m_inode_mutex);

	return (error);
}


/*
 * ----- sam_inode_free - Free inode identified by i-node number 'ino', and
 *	permanent inode pointer 'pip'.  Returns errno to indicate the return
 *	error status.
 */
int						/* Return errno */
sam_inode_free(
	sam_mount_t *mp,			/* Mount table pointer */
	ino_t ino,				/* I-node number */
	struct sam_perm_inode *pip)		/* Perm i-node */
{
	int32_t gen;
	int error = 0;
	sam_node_t *dip;

	if ((mp == NULL) || (pip == NULL)) {
		return (EINVAL);
	}

	dip = mp->mi.m_inodir;			/* .inodes i-node */

	/* Initialize i-node contents */
	gen = pip->di.id.gen;		/* Increment from original gen */
	gen++;
	bzero((char *)pip, sizeof (struct sam_perm_inode));
	pip->di.id.ino = ino;
	pip->di.id.gen = gen;

	/* Insert non-reserved i-node into i-node free list */
	if (ino >= mp->mi.m_min_usr_inum) {
		pip->di.free_ino = dip->di.free_ino;
		dip->di.free_ino = ino;
		dip->flags.b.changed = 1;
	}

	return (error);
}

/*
 * ----- sam_inode_validate - Validate the i-node identified by i-node
 *	number 'ino', and the permanent inode pointer 'pip'.  Returns
 *	errno > 0 to indicate the return error status, or -1 to indicate
 *	a bad i-node.
 */
static inline int
sam_inode_validate(
	ino_t ino,				/* I-node number */
	struct sam_perm_inode *pip)		/* permanent i-node */
{
	int error = -1;
	int32_t pip_ino;
	int pip_mode;
	int32_t pip_size;

	if (pip == NULL) {
		return (EINVAL);
	}

	pip_ino = pip->di.id.ino;
	pip_mode = pip->di.mode;
	pip_size = pip->di.rm.size;

	if ((pip_mode == 0) || (pip_ino == 0) || (pip_ino != ino)) {
		return (error);
	}

	if ((pip_ino != SAM_ROOT_INO) && (pip_ino == pip->di.parent_id.ino) &&
	    (pip->di.blocks == 0)) {
		return (error);
	}

	if (pip_size < 0) {
		return (error);
	}

	if (S_ISDIR(pip_mode) || (S_ISSEGI(&pip->di))) {
		if (pip->di.status.b.damaged) {
			return (error);
		}
		if (S_ISDIR(pip_mode) && (pip_size == 0)) {
			return (error);
		}
		if (S_ISSEGI(&pip->di) && (pip->di.rm.info.dk.seg.fsize == 0)) {
			return (error);
		}
	}
	return (0);
}

/*
 * ----- sam_inode_freelist_build - Build i-node free list
 *	Builds i-node free list from existing free and bad inodes.  The
 *	number of i-nodes in the list depends on the supplied 'count'
 *	value.  If 'count' is < 0, then the final list is comprised of
 *	all currently free and bad i-nodes.  Otherwise, the maximum
 *	size of the list is 'count' i-nodes.
 *	Returns errno to indicate the return status.  If errno is 0,
 *	'result' contains the number of i-nodes in the new free list.
 */
int
sam_inode_freelist_build(
	sam_mount_t *mp,	/* Mount table pointer */
	int32_t count,		/* Number of bad i-nodes to free (<0 is all) */
	int32_t *result)	/* Free list returned i-node count */
{
	ino_t ino;
	sam_node_t *dip;
	buf_t *bp;
	struct sam_perm_inode *pip;
	int error = 0;
	int32_t freed_count = 0;
	int build_time;
	int32_t min_ino;

	if (mp == NULL) {
		return (EINVAL);
	}

	dip = mp->mi.m_inodir;

	mutex_enter(&mp->ms.m_inode_mutex);

	/* Truncate current list */
	dip->di.free_ino = 0;
	*result = 0;

	/*
	 * I-nodes are inserted to head of freelist.  Build the free list
	 * starting with highest numbered i-node to generate a free-list
	 * whose head i-node is the lowest numbered.
	 */
	ino = SAM_DTOI(dip->di.rm.size) - 1;
	if (count < 1) {
		min_ino = mp->mi.m_min_usr_inum;
	}
	build_time = ((ino - min_ino) + 1)/4608; /* Can process ~4608/sec */

	if (build_time >= 60) {
		cmn_err(CE_WARN, "SAM-QFS: %s: sam_inode_freelist_build: "
		    "Repairing i-node freelist - please wait ~%d minute(s)",
		    mp->mt.fi_name, ((build_time+30)/60));
	}

	while ((count < 0) || (freed_count < count)) {
		if (ino < min_ino) {	/* Reached end */
			break;
		}

		/* Read this i-node */
		if ((error = sam_read_ino(mp, ino, &bp, &pip)) != 0) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: sam_inode_freelist_build: "
			    "Can't read inode %ld",
			    mp->mt.fi_name, ino);
			break;
		}

		/* Validate i-node */
		error = sam_inode_validate(ino, pip);

		if (error == 0) {		/* looks good */
			brelse(bp);
		} else if (error > 0) { /* errno from sam_inode_validate() */
			brelse(bp);
			break;
		} else {			/* Free bad i-node */
			if ((error = sam_inode_free(mp, ino, pip)) != 0) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_inode_freelist_build: "
				    "Can't free i-node %ld",
				    mp->mt.fi_name, ino);
				brelse(bp);
				break;
			}
			if (TRANS_ISTRANS(mp)) {
				TRANS_WRITE_DISK_INODE(mp, bp, pip, pip->di.id);
			} else {
				bdwrite(bp);
			}
			freed_count++;
		}

		ino--;		/* Go process next inode number */
	}

	/* Sync the .inodes inode */
	if ((error = sam_update_inode(dip, SAM_SYNC_ONE, FALSE)) != 0) {
		cmn_err(CE_WARN, "SAM-QFS: %s: sam_inode_freelist_build: "
		    "Couldn't sync .inodes i-node after freelist build",
		    mp->mt.fi_name);
	}

	mutex_exit(&mp->ms.m_inode_mutex);

	*result = freed_count;
	return (error);
}

/*
 * ----- sam_inodes_freelist_check - Check the i-node free list for
 *	the filesystem of the specified mount info.  'itype' indicates
 *	the required type of i-node list on which to perform this
 *	check - SAM_ILIST_PREALLOC indicates pre-allocated i-nodes,
 *	and SAM_ILIST_DEFAULT indicates the default allocation scheme.
 *	'count' indicates the number if free list i-nodes to check.
 *	If 'count' is < 0, then all free list i-nodes are checked.
 *	Returns errno (>= 0) to indicate the return error status, or
 *	returns < 0 to indicate that a bad i-node condition was found.
 */
int
sam_inode_freelist_check(
	sam_mount_t *mp,	/* Mount table pointer */
	int itype,		/* SAM_ILIST_PREALLOC or SAM_ILIST_DEFAULT */
	int32_t count)		/* Number of i-nodes to check (< 0 is ALL) */
{
	sam_node_t *dip;
	int32_t result;
	int error = 0;
	boolean_t prealloc;

	if (mp == NULL) {
		return (EINVAL);
	}

	dip = mp->mi.m_inodir;
	prealloc = (dip->di.status.b.direct_map != 0);

	/* Check pre-alloc'ed i-node list? */
	if ((itype == SAM_ILIST_PREALLOC) && (prealloc == B_FALSE)) {
		return (0);
	}

	error = sam_inode_freelist_walk(mp, count, &result);

	/* Log i-node info if pre-allocated and useful */
	if ((error == 0) && (prealloc == B_TRUE)) {
		if ((count < 0) && (result < 64)) {
			/* Warn if checked ALL, but found < 64 free */
			cmn_err(CE_NOTE,
			    "SAM-QFS: %s: sam_inode_freelist_check: "
			    "%d i-nodes remaining on free list",
			    mp->mt.fi_name, result);
		} else if (result < count) {
			/* Warn if found fewer free than the number checked */
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: sam_inode_freelist_check: "
			    "%d i-nodes remaining on free list",
			    mp->mt.fi_name, result);
		}

		return (0);
	}

	return (error);
}


/*
 * ----- qfs_flush_inode - Flush inode pages.
 */
/* ARGSUSED */
int
qfs_flush_inode(sam_node_t *ip, void *arg)
{
	int saverror = 0;

	return (saverror);
}


/*
 * ----- qfs_scan_inodes - Scan inodes and apply func() to each one.
 */
/* ARGSUSED */
int
qfs_scan_inodes(
	int rwtry,
	int (*func)(sam_node_t *, void *),
	void *arg,
	sam_mount_t *mp)
{
	int saverror = 0;

	return (saverror);
}


/*
 *	----	sam_stale_inode
 *
 * The FS associated with this inode is being forcibly unmounted.
 * Reset the vnode's vop table pointer to the FS's associated stale
 * vnode ops table, so that only close/unmap/... (operations that
 * release resources) do anything other than return EIO.
 */
void
sam_stale_inode(sam_node_t *ip)
{
	extern struct vnodeops *samfs_vnode_staleopsp;
	vnode_t *vp = SAM_ITOV(ip);

	TRACE(T_SAM_STALE_INO, vp, ip->di.id.ino, vp->v_count, ip->flags.bits);
	if (SAM_IS_SHARED_FS(ip->mp)) {
		vn_setops(vp, samfs_vnode_staleopsp);
	} else {
#ifdef METADATA_SERVER
		vn_setops(vp, samfs_vnode_staleopsp);
#else
		extern struct vnodeops *samfs_client_vnode_staleopsp;

		vn_setops(vp, samfs_client_vnode_staleopsp);
#endif
	}
}
