/*
 * ----- sam/rename.c - Process the directory rename functions.
 *
 *	Processes the SAMFS inode directory rename function.
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

#pragma ident "$Revision: 1.68 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/t_lock.h>
#include <sys/sysmacros.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/cred.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/stat.h>
#include <sys/mode.h>
#include <sys/vfs.h>
#include <sys/fbuf.h>
#include <sys/policy.h>

/* ----- SAMFS Includes */

#include "sam/types.h"
#include "sam/samevent.h"
#include "inode.h"
#include "mount.h"
#include "ioblk.h"
#include "dirent.h"
#include "extern.h"
#include "arfind.h"
#include "debug.h"
#include "trace.h"
#include "kstats.h"
#include "qfs_log.h"


int sam_rename_error_line;

static int sam_stickytest(struct sam_node *pip, struct sam_node *ip,
		boolean_t ip_locked, cred_t *credp);
static int sam_check_path(struct sam_node *opip, sam_id_t id,
		struct sam_node *npip);
static int sam_change_dotdot(struct sam_node *, struct sam_node *);


/*
 * ----- sam_rename_inode - rename an entry in the directory.
 *	Given an old directory inode and component name, a new directory inode
 *	and component name, rename the old to the new. Verify the old file
 *	exists. Remove the new file if it exists; hard link the new file to
 *	the old file; remove the old file/directory. If no error, return
 *	inode pointer and inode "held" (v_count incremented). It is caller's
 *      responsibility to release the vnode of the returned inode if the
 *      caller is interested in that inode (output parmeter is not NULL) and
 *	no error occured during the function execution.
 * 	In case of an error, the corresponding vnode is released.
 *
 */

int				/* ERRNO if error, 0 if successful. */
sam_rename_inode(
	struct sam_node *opip,	/* Pointer to the old file parent inode. */
	char *onm,		/* Pointer to old file name. */
	struct sam_node *npip,	/* Pointer to the new file parent inode. */
	char *nnm,		/* Pointer to new file name. */
	sam_node_t **ipp,	/* new inode of the renamed file (returned). */
	cred_t *credp)		/* credentials pointer. */
{
	ino_t ino = 0;
	sam_mount_t *mp = opip->mp;
	struct sam_node *oip = NULL;
	struct sam_node *nip = NULL;
	struct sam_node *enip = NULL;
	struct sam_name oname;
	struct sam_name nname;
	boolean_t source_is_a_dir;
	boolean_t target_is_a_dir = FALSE;
	boolean_t parents_are_different = FALSE;
	struct vnode *vp;
	sam_id_t oid, poid;
	sam_time_t otime;
	int error_line = 0;
	int error;
	int new_error;
#ifdef LQFS_TODO_LOCKFS
	struct ulockfs *ulp;
#endif /* LQFS_TODO_LOCKFS */
	int issync;
	int trans_size;
	int terr = 0;

	/*
	 *	Hold old & new directory RW_WRITER data_rwl locks throughout
	 *	rename.
	 *
	 *	Note: This implies that all calls to sam_lookup_name() have a
	 *		non-NULL sam_name argument.
	 */
	if (opip != npip) {
		parents_are_different = TRUE;
	}
	oid.ino = 0;

	/*
	 * Start an LQFS rename transaction
	 */
#ifdef LQFS_TODO_LOCKFS
	if (ulp) {
#endif /* LQFS_TODO_LOCKFS */
		trans_size = (int)TOP_RENAME_SIZE(opip);
		TRANS_BEGIN_CSYNC(mp, issync, TOP_RENAME, trans_size);
#ifdef LQFS_TODO_LOCKFS
	}
#endif /* LQFS_TODO_LOCKFS */

	/*
	 * sam_dlock_two handles the case where the inodes are the same.
	 */
	sam_dlock_two(opip, npip, RW_WRITER);

	/*
	 * Check that the old file exists and has the right permissions.
	 * Treat it like a create, except we expect to find the name.
	 * We don't use the results of oname structure this time, but it
	 * has to be there.
	 */
	oname.operation = SAM_CREATE;
	if ((error = sam_lookup_name(opip, onm, &oip, &oname, credp))) {
		error_line = __LINE__;
		/* Entry does not exist or error. */
		goto out15;
	}
	/*
	 * Old or New directory must not be old file.
	 */
	if ((opip == oip) || (npip == oip)) {
		error = EINVAL;
		error_line = __LINE__;
		goto out15;
	}
	/*
	 */
	if (SAM_PRIVILEGE_INO(oip->di.version, oip->di.id.ino)) {
		error = EPERM;	/* Cannot rename privileged inos */
		error_line = __LINE__;
		goto out15;
	}
	/*
	 * Cannot rename same file.  Do not return an error.
	 */
	if ((opip == npip) && (strcmp(onm, nnm) == 0)) {
		goto out15;
	}

	/*
	 * If WORM bit set, can't rename the file.
	 */
	source_is_a_dir = S_ISDIR(oip->di.mode);
	if (oip->di.status.b.worm_rdonly) {
		if (!source_is_a_dir) {
			error = EROFS;
			error_line = __LINE__;
			goto out15;
		} else if (sam_empty_dir(oip) != 0) {
			/*
			 * Block rename of a non-empty directory.
			 */
			error = EROFS;
			error_line = __LINE__;
			goto out15;
		}
	}

	/*
	 * Check if the new file exists.  Returns the found entry, else the
	 * slot where this component name could fit and ENOENT, else an error.
	 */
	nname.operation = SAM_RENAME_LOOKUPNEW;
	if ((new_error = sam_lookup_name(npip, nnm, &nip,
	    &nname, credp)) == 0) {
		/*
		 * Cannot rename hard link for the same inode.  Do not return an
		 * error.
		 */
		if (oip->di.id.ino == nip->di.id.ino) {
			VN_RELE(SAM_ITOV(nip));
			goto out15;
		}
		/*
		 * If the new directory is sticky, the user must own the
		 * directory, be superuser or own the file.
		 */
		if ((npip->di.mode & S_ISVTX) &&
		    (error = sam_stickytest(npip, nip, FALSE, credp))) {
			VN_RELE(SAM_ITOV(nip));
			error_line = __LINE__;
			goto out15;
		}
	}
	RW_LOCK_OS(&oip->inode_rwl, RW_WRITER);

	/*
	 * User must have write access to the old parent directory or if the
	 * directory is sticky, the user must own the directory, be superuser,
	 * own the old file, or else have write permission to the old file.
	 */
	if ((error = sam_access_ino(opip, S_IWRITE, FALSE, credp))) {
		if ((error = sam_stickytest(opip, oip, TRUE, credp))) {
			error_line = __LINE__;
			goto out1;
		}
	}
	/*
	 * Cannot rename '.' and '..'
	 */
	if (IS_DOT_OR_DOTDOT(onm)) {
		error_line = __LINE__;
		error = EINVAL;
		goto out1;
	}

	/*
	 * If renaming a directory, verify it is not in new directory path.
	 */
	if (source_is_a_dir && (oip != npip)) {
		if ((error = sam_check_path(opip, oip->di.id, npip))) {
			error_line = __LINE__;
			goto out1;
		}
		if (parents_are_different &&
		    (error = sam_access_ino(oip, S_IWRITE, TRUE, credp))) {
			error_line = __LINE__;
			goto out1;
		}
	}

	/*
	 * Remove the new file if it exists.
	 */
	error = new_error;
	if (error == 0) {
		/*
		 * At this point nip is set and v_count has been incremented.
		 */
		vp = SAM_ITOV(nip);
		if (parents_are_different) {
			if ((error = sam_access_ino(npip,
			    S_IWRITE, FALSE, credp))) {
				if ((error = sam_stickytest(npip, nip,
				    FALSE, credp))) {
					error_line = __LINE__;
					goto out1;
				}
			}
		}
		target_is_a_dir = S_ISDIR(nip->di.mode);
		if (target_is_a_dir) {
			/* check if target is a mount point */
			if (vn_ismntpt(vp)) {
				error_line = __LINE__;
				error = EBUSY;
			/* check if source also a directory */
			} else if (!source_is_a_dir) {
				error_line = __LINE__;
				error = EISDIR;
			/* check if target directory empty */
			} else if (sam_empty_dir(nip)) {
				error_line = __LINE__;
				error = EEXIST;
			} else if (parents_are_different &&
			    (error = sam_access_ino(nip, S_IWRITE,
			    FALSE, credp))) {
				error_line = __LINE__;
			}
		} else if (source_is_a_dir) {
			error = ENOTDIR;
			error_line = __LINE__;
		}
		if (error) {
			goto out1;
		}

		if (target_is_a_dir) {
			nname.operation = SAM_RMDIR;
			if (RW_TRYENTER_OS(&nip->data_rwl, RW_WRITER) == 0) {
				error = EDEADLK;
				error_line = __LINE__;
				goto out1;
			}
		} else {
			nname.operation = SAM_REMOVE;
		}
		/*
		 * New entry exists, remove it.
		 * This will decrement v_count but leave nip set.
		 */
		error = sam_remove_name(npip, nnm, nip, &nname, kcred);
		if (target_is_a_dir) {
			RW_UNLOCK_OS(&nip->data_rwl, RW_WRITER);
		}
		/*
		 * Don't release the inode of the removed file,
		 * save it for VNEVENT_RENAME_DEST_OS().
		 */
		enip = nip;
		nip = NULL;
		if (error) {
			error_line = __LINE__;
			goto out14;
		}
		/*
		 * Set nname so new entry will be added without extending
		 * the directory.
		 */
		nname.operation = SAM_FORCE_LOOKUP;
		if ((error = sam_lookup_name(npip, nnm, &nip, &nname, credp)) !=
		    ENOENT) {
			error_line = __LINE__;
			goto out1;
		}
	} else if (error != ENOENT) {
		/*
		 * At this point nip is null and v_count has not been
		 * incremented.
		 */
		error_line = __LINE__;
		goto out14;
	}
	oid = oip->di.id;
	poid = oip->di.parent_id;
	otime = opip->di.modify_time.tv_sec;

	/*
	 * Link the new file to the existing old file.
	 */
	nname.operation = SAM_RENAME_LINK;
	if ((error = sam_create_name(npip, nnm,
	    &oip, &nname, NULL, credp)) == 0) {
		boolean_t oip_unlocked = FALSE;

		ino = oip->di.id.ino;
		if (source_is_a_dir) {
			if (parents_are_different) {
				/* If directory, change ".." */
				oip->di.parent_id = npip->di.id;
				RW_UNLOCK_OS(&oip->inode_rwl, RW_WRITER);
				oip_unlocked = TRUE;
				error = sam_change_dotdot(oip, npip);
				if (error)
					error_line = __LINE__;
				RW_LOCK_OS(&opip->inode_rwl, RW_WRITER);
				opip->di.nlink--;
				TRANS_INODE(mp, opip);
				RW_UNLOCK_OS(&opip->inode_rwl, RW_WRITER);
				RW_LOCK_OS(&npip->inode_rwl, RW_WRITER);
				npip->di.nlink++;
				TRANS_INODE(mp, npip);
				RW_UNLOCK_OS(&npip->inode_rwl, RW_WRITER);
			}
		}
		if (!oip_unlocked) {
			RW_UNLOCK_OS(&oip->inode_rwl, RW_WRITER);
		}
		if (SAM_SYNC_META(mp)) {
			sam_ilock_two(npip, oip, RW_WRITER);
			sam_sync_meta(npip, oip, credp);
			sam_iunlock_two(npip, oip, RW_WRITER);
		}

		/* Remove the old file, do not decrement count. */
		/*
		 * Lookup old file again because directory structure may have
		 * changed
		 */
		oname.operation = SAM_REMOVE;
		if ((error = sam_lookup_name(opip, onm, &oip, &oname, credp))) {
			error_line = __LINE__;
			/* Entry does not exist or error. */
			goto out15;
		}
		if (SAM_IS_SHARED_SERVER(mp)) {
			sam_notify_arg_t *notify;

			/*
			 * Shared server must notify all clients to stale dnlc.
			 */
			SAM_COUNT64(shared_server, notify_dnlc);
			notify = kmem_zalloc(sizeof (*notify), KM_SLEEP);
			strcpy(notify->p.dnlc.component, onm);
			sam_proc_notify(opip, NOTIFY_dnlc, 0, notify);
			if (source_is_a_dir) {
				strcpy(notify->p.dnlc.component, "..");
				sam_proc_notify(oip, NOTIFY_dnlc, 0, notify);
			}
			kmem_free(notify, sizeof (*notify));
		}
		VN_RELE(SAM_ITOV(oip));		/* VN_RELE for second lookup */
		oname.operation = SAM_RENAME_LINK;
		error = sam_remove_name(opip, onm, oip, &oname, kcred);
		if (error)
			error_line = __LINE__;
		/* Check possible new archive set */
		oip->di.status.b.archdone = 0;
		/*
		 * Notify arfind and event daemon of file/directory rename.
		 */
		sam_send_to_arfind(oip, AE_rename, 0);
		if (mp->ms.m_fsev_buf == NULL) {
			goto out15;
		}

		/*
		 * If parents are different and event logging on, send 1st of
		 * 2 events for rename. 1 = old parent, 2 = new parent.
		 */
		if (parents_are_different) {
			sam_disk_inode_t di;

			di.id = oid;
			di.parent_id = poid;
			sam_send_event(mp, &di, ev_rename, 1, otime);
			sam_send_event(mp, &oip->di, ev_rename, 2,
			    npip->di.modify_time.tv_sec);
		} else {
			sam_send_event(mp, &oip->di, ev_rename, 0,
			    npip->di.modify_time.tv_sec);
		}
		goto out15;
	} else {
		/*
		 * At this point nip may be null or set, but v_count has already
		 * been dealt with accordingly.
		 */
		error_line = __LINE__;
		goto out14;
	}

out1:
	if (nip) {
		VN_RELE(SAM_ITOV(nip));
	}

out14:
	RW_UNLOCK_OS(&oip->inode_rwl, RW_WRITER);

out15:
	sam_dunlock_two(npip, opip, RW_WRITER);
#ifdef LQFS_TODO_LOCKFS
	if (ulp) {
#endif /* LQFS_TODO_LOCKFS */
		TRANS_END_CSYNC(mp, terr, issync, TOP_RENAME, trans_size);
		if (error == 0) {
			error = terr;
		}
#ifdef LQFS_TODO_LOCKFS
		ufs_lockfs_end(ulp);
	}
#endif /* LQFS_TODO_LOCKFS */

	if (error == 0) {
		if (enip != NULL) {
			VNEVENT_RENAME_DEST_OS(SAM_ITOV(enip), SAM_ITOV(npip),
			    nnm, NULL);
		}
		if (parents_are_different) {
			VNEVENT_RENAME_DEST_DIR_OS(SAM_ITOV(npip), NULL);
		}
		if (oip != NULL) {
			VNEVENT_RENAME_SRC_OS(SAM_ITOV(oip), SAM_ITOV(opip),
			    onm, NULL);
		}
	}
	if (enip != NULL) {
		VN_RELE(SAM_ITOV(enip));
	}
	if (oip != NULL && (ipp == NULL || error)) {
		VN_RELE(SAM_ITOV(oip));
	}

	if (error) {
		sam_rename_error_line = error_line;
		error_line = (error_line << 16) | error;
	} else {
		if (ipp != NULL) {
			*ipp = oip;
		}
		error_line = 0;
	}
	TRACE(T_SAM_RENAME_RET, SAM_ITOV(npip), (sam_tr_t)oip, ino, error_line);
	return (error);
}


/*
 * ----- sam_check_path - check directory path
 *	Verify the old ino is not in the path of the new directory.
 *	If it is present, orphans would reset after the rename.
 *	Compare the ".." entries back up to root with the old ino number.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_check_path(
	struct sam_node *opip,	/* pointer to the old file parent inode. */
	sam_id_t oid,		/* old file id (ino/gen). */
	struct sam_node *npip)	/* pointer to the new file parent inode. */
{
	int error;
	struct sam_node *ip;
	struct fbuf *fbp;
	struct sam_empty_dir *dp;
	sam_id_t parent;

	/*
	 * If in the same directory, do not bother.
	 */
	if (opip == npip)
		return (0);
	if (npip->di.id.ino == oid.ino && npip->di.id.gen == oid.gen) {
		return (EINVAL);
	}
	if (oid.ino == SAM_ROOT_INO) {
		return (0);
	}
	ip = npip;
	for (;;) {
		error = fbread(SAM_ITOV(ip), (sam_u_offset_t)0,
		    DIR_BLK, S_OTHER, &fbp);
		if (error) {
			return (error);
		}
		dp = (struct sam_empty_dir *)(void *)fbp->fb_addr;
		if ((S_ISDIR(ip->di.mode) == 0) || (ip->di.nlink == 0) ||
		    (ip->di.rm.size < sizeof (struct sam_empty_dir)) ||
		    (dp->dotdot.d_namlen != 2) ||
		    (dp->dotdot.d_name[0] != '.') ||
		    (dp->dotdot.d_name[1] != '.')) {
			fbrelse(fbp, S_OTHER);
			error = ENOTDIR;
			break;
		}
		parent = dp->dotdot.d_id;
		fbrelse(fbp, S_OTHER);
		if (oid.ino == parent.ino && oid.gen == parent.gen) {
			/* Error if old file is in the new parent path */
			error = EINVAL;
			break;
		}
		if (parent.ino == SAM_ROOT_INO) {
			error = 0;
			break;
		}
		if ((ip != npip) && (ip != opip)) {
			RW_UNLOCK_CURRENT_OS(&ip->data_rwl);
			VN_RELE(SAM_ITOV(ip));	/* Decrement v_count */
		}
		if (parent.ino == npip->di.id.ino &&
		    parent.gen == npip->di.id.gen) {
			ip = npip;
		} else if (parent.ino == opip->di.id.ino &&
		    parent.gen == opip->di.id.gen) {
			ip = opip;
		} else {
			vnode_t *vp;

			vp = SAM_ITOV(npip);
			if ((error = sam_get_ino(vp->v_vfsp, IG_EXISTS,
			    &parent, &ip))) {
				ip = NULL;
				break;
			}
			if (RW_TRYENTER_OS(&ip->data_rwl, RW_READER) == 0) {
				error = EDEADLK;
				VN_RELE(SAM_ITOV(ip));
				ip = NULL;
				break;
			}
		}
	}
	if (ip && (ip != npip) && (ip != opip)) {
		RW_UNLOCK_CURRENT_OS(&ip->data_rwl);
		VN_RELE(SAM_ITOV(ip));	/* Decrement v_count */
	}
	return (error);
}


/*
 * ----- sam_change_dotdot - change the dotdot ino in the directory entry.
 *	Change the dotdot ino in the directory entry.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_change_dotdot(
	struct sam_node *ip,	/* pointer to the renamed inode. */
	struct sam_node *npip)	/* pointer to the new file parent inode. */
{
	int error;
	struct fbuf *fbp;
	struct sam_empty_dir *dp;

	error = fbread(SAM_ITOV(ip), (sam_u_offset_t)0, DIR_BLK, S_WRITE, &fbp);
	if (error) {
		return (error);
	}
	dp = (struct sam_empty_dir *)(void *)fbp->fb_addr;
	if ((ip->di.nlink == 0) ||
	    (ip->di.rm.size < sizeof (struct sam_empty_dir))||
	    (dp->dotdot.d_namlen != 2) || (dp->dotdot.d_name[0] != '.') ||
	    (dp->dotdot.d_name[1] != '.')) {
		error = ENOTDIR;
		fbrelse(fbp, S_WRITE);
	} else {
		dp->dotdot.d_id = npip->di.id;
		if (TRANS_ISTRANS(ip->mp)) {
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			TRANS_DIR(ip, (sam_u_offset_t)0);
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			fbwrite(fbp);
		} else if (SAM_SYNC_META(ip->mp)) {
			fbwrite(fbp);
		} else {
			fbdwrite(fbp);
		}
	}
	return (error);
}

/*
 * ----- sam_stickytest - unlink or rename of a file in a sticky directory
 *			is allowed when one of the following is true:
 *				1) are superuser
 *				2) own the file,
 *				3) own the directory,
 *				4) have write permission to the file
 */
static int
sam_stickytest(
	struct sam_node *pip,	/* Pointer to the file parent inode. */
	struct sam_node *ip,	/* Pointer to the file inode. */
	boolean_t ip_locked,	/* Do we have ip->inode_rwl locked? */
	cred_t *credp)		/* credentials pointer. */
{
	int error = 0;
	uid_t	id;

	if (pip->di.mode & S_ISVTX) {		/* must be sticky, to test */
		id = crgetuid(credp);
		if ((id != pip->di.uid) &&
		    (id != ip->di.uid) &&
		    secpolicy_vnode_remove(credp)) {
			error = sam_access_ino(ip, S_IWRITE, ip_locked, credp);
		}
	} else {
		error = EACCES;
	}
	return (error);
}
