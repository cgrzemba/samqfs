/*
 * ----- sam/remove.c - Process the directory remove functions.
 *
 * Processes the SAMFS inode directory functions.
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

#pragma ident "$Revision: 1.92 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/t_lock.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/cred.h>
#include <sys/vnode.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/dnlc.h>
#include <sys/fbuf.h>
#include <sys/user.h>
#include <sys/cmn_err.h>		/* for debug only */
#include <sys/policy.h>
#include <sys/sysmacros.h>

/* ----- SAMFS Includes */

#include "sam/types.h"
#include "sam/samevent.h"

#include "inode.h"
#include "mount.h"
#include "dirent.h"
#include "extern.h"
#include "arfind.h"
#include "debug.h"
#include "trace.h"
#include "ino_ext.h"
#include "kstats.h"
#include "qfs_log.h"

struct sam_fbuf_params {
	uint32_t	fbp_offset;		/* Original offset */
	uint32_t	fbp_reloff;		/* Offset within fbuf */
	uint32_t	fbp_off;		/* Offset of first page byte */
	uint32_t	fbp_psize;		/* Page size */
	uint32_t	fbp_size;		/* Page size */
};

static void sam_resolve_fbuf(uint32_t offset, struct sam_fbuf_params *fbprp);
static void sam_compress_dirblk(sam_node_t *dip, struct sam_dirent *dp,
						sam_offset_t blk_off);


/*
 * ----- sam_remove_name - Remove an entry in the directory.
 *	Given a parent directory inode and component name, remove a
 *	file/directory. NOTE. Parent data_rwl lock set on entry.
 */

int				/* ERRNO if error, 0 if successful. */
sam_remove_name(
	sam_node_t *pip,	/* parent directory inode. */
	char *cp,		/* component name to be removed. */
	sam_node_t *ip,		/* inode to be removed. */
	struct sam_name *namep,	/* name that holds the slot to be removed. */
	cred_t *credp)		/* credentials. */
{
	struct sam_dirent *dp;
	int error;
	struct fbuf *fbp;
	uid_t	id;
	sam_u_offset_t offset;
	ushort_t slot_size;
	enum sam_op operation;
	vnode_t *vp;
	struct sam_fbuf_params fbuf_params;
	timespec_t system_time;
	dcret_t reply;
	boolean_t write_lock = FALSE;
	ushort_t d_namehash;

	fbp = NULL;

	/*
	 * Cannot remove '.' and '..'
	 */
	vp = SAM_ITOV(ip);
	if (cp[0] == '.') {
		if (cp[1] == '\0') {
			error = EINVAL;
			goto out1;
		} else if ((cp[1] == '.') && (cp[2] == '\0')) {
			error = EEXIST;
			goto out1;
		}
	}
	if (S_ISDIR(pip->di.mode) == 0) { /* Parent must be a directory. */
		error = ENOTDIR;
		goto out1;
	}
	if (SAM_PRIVILEGE_INO(ip->di.version, ip->di.id.ino)) {
		error = EPERM;	/* Cannot remove privileged inos */
		goto out1;
	}
	if (ip->di.status.b.worm_rdonly && S_ISDIR(ip->di.mode) == 0) {
		boolean_t   is_priv =
		    (secpolicy_fs_config(credp, ip->mp->mi.m_vfsp) == 0);
		/*
		 * If a WORM file and the retention time has not expired
		 * return EROFS.    Allow deletes IFF the lite bit is set
		 * and we have appropriate privileges.
		 */
		if (!(ip->mp->mt.fi_config & MT_LITE_WORM) || !is_priv) {

			SAM_HRESTIME(&system_time);

			if (ip->di.version >= SAM_INODE_VERS_2) {
				if (((ip->di2.rperiod_start_time/60 +
				    ip->di2.rperiod_duration) >
				    system_time.tv_sec/60) ||
				    (ip->di2.rperiod_duration == 0)) {
					error = EROFS;
					goto out1;
				}
			} else {
				error = EROFS;
				goto out1;
			}
		}
	}
	/*
	 * Execute & Write access required for remove
	 */
	if ((error = sam_access_ino(pip, (S_IEXEC | S_IWRITE), FALSE, credp))) {
		goto out1;
	}

	/*
	 * If the directory is sticky, the user must own the directory, be
	 * superuser, own the file, or else have write permission to the file.
	 * sam_access_ino() makes the check for superuser or equivalent.
	 */
	RW_LOCK_OS(&ip->inode_rwl, RW_READER);
	id = crgetuid(credp);
	if ((pip->di.mode & S_ISVTX) &&
	    (id != pip->di.uid) && (id != ip->di.uid) &&
	    (error = sam_access_ino(ip, S_IWRITE, TRUE, credp))) {
		goto out;
	}
	operation = namep->operation;
	if (operation == SAM_RMDIR) {	/* Rmdir(2) remove */
		if ((pip == ip) || (vp == SAM_ITOV(pip))) {
			error = EINVAL;
			goto out;
		} else if (S_ISDIR(ip->di.mode) == 0) {
			error = ENOTDIR; /* Not a directory */
			goto out;
		}
	} else if (operation == SAM_REMOVE) {	/* Unlink(2) remove */
		if (vp->v_type == VDIR &&
		    secpolicy_fs_linkdir(credp, vp->v_vfsp)) {
			error = EPERM;
			goto out;
		}
	}

	RW_UPGRADE_OS(&ip->inode_rwl);
	write_lock = TRUE;

	/*
	 * If ip has extended attributes, remove the DNLC entry for
	 * the unnamed directory.  This will allow the vnode to become
	 * inactive, at which point we will clean up the orphaned
	 * EA tree.
	 */
	if (SAM_INODE_HAS_XATTR(ip)) {
		dnlc_remove(vp, XATTR_DIR_NAME);
	}

	if (operation == SAM_RMDIR) {
		/*
		 * Check for current directory
		 */
		if (vp == PTOU(curproc)->u_cdir) {
			TRACE(T_SAM_VCOUNT, vp, __LINE__, vp->v_count, 0);
			error = EINVAL;
			goto out;
		}
		if (sam_empty_dir(ip) != 0) {
			if (ip->di.status.b.worm_rdonly) {
				error = EROFS;
			} else {
				error = EEXIST;
			}
			goto out;
		}
		/*
		 * We now know that the directory is empty, so the link count
		 * should be 2.  Adjust or accept a bad count for now.
		 */
		if (ip->di.nlink != 2) {
			TRACE(T_SAM_NLNKCNT, vp, __LINE__,
			    ip->di.nlink, (sam_tr_t)ip);
			ASSERT(ip->di.nlink >= 2);	/* for debug systems */
			if (ip->di.nlink < 2)
				ip->di.nlink = 2;
		}
	}

	/*
	 * Read the page of memory associated with this directory entry.
	 * Remove the entry by setting the fmt=0 and clearing the entry
	 * except for the reclen.
	 */
	offset = namep->data.entry.offset;
	sam_resolve_fbuf(offset, &fbuf_params);
	error = fbread(SAM_ITOV(pip), fbuf_params.fbp_off, fbuf_params.fbp_size,
	    S_WRITE, &fbp);
	if (error) {
		goto out;
	}
	error = sam_validate_dir_blk(pip, fbuf_params.fbp_off, DIR_BLK, &fbp);
	if (error) {
		goto out;
	}

	dp = (struct sam_dirent *)((sam_uintptr_t)fbp->fb_addr +
	    (sam_uintptr_t)fbuf_params.fbp_reloff);
	dp->d_fmt = 0;		/* Empty entry */
	d_namehash = dp->d_namehash;

	slot_size = dp->d_reclen - SAM_DIRSIZ(dp);
	bzero((caddr_t)&dp->d_id, (dp->d_reclen - 4));

	/*
	 * Remove from the DNLC cache.
	 */
	dnlc_remove(SAM_ITOV(pip), cp);

	/*
	 * Remove from the directory cache.
	 */
	reply = dnlc_dir_rem_entry(&pip->i_danchor, cp, NULL);
	if (S_ISDIR(ip->di.mode)) {
		TRACE(T_SAM_EDNLC_REM_D, SAM_ITOV(pip), reply, 0, 0);
	} else {
		TRACE(T_SAM_EDNLC_REM_E, SAM_ITOV(pip), reply, 0, 0);
	}

	if (slot_size >= SAM_DIRLEN_MIN) {
		uint64_t handle;
		/*
		 * If the entry had associated extra space, remove it from
		 * the directory cache.
		 */
		handle = SAM_LEN_TO_H(slot_size, offset);
		reply = dnlc_dir_rem_space_by_handle(&pip->i_danchor, handle);
		TRACE(T_SAM_EDNLC_REM_H, SAM_ITOV(pip), offset,
		    slot_size, reply);
	}

	dp = (struct sam_dirent *)(sam_uintptr_t)fbp->fb_addr;
	sam_compress_dirblk(pip, dp, (sam_offset_t)fbuf_params.fbp_off);
	RW_LOCK_OS(&pip->inode_rwl, RW_READER);
	mutex_enter(&pip->fl_mutex);
	if ((operation == SAM_RMDIR) && (pip->di.nlink > 0)) {
		pip->di.nlink--;
	}
	TRANS_INODE(pip->mp, pip);
	sam_mark_ino(pip, (SAM_UPDATED | SAM_CHANGED));
	mutex_exit(&pip->fl_mutex);
	RW_UNLOCK_OS(&pip->inode_rwl, RW_READER);

	if (ip->di.nlink > 0) {
		ip->di.nlink--;
		if (S_ISDIR(ip->di.mode)) {
			dnlc_remove(vp, "..");
			if (operation == SAM_RMDIR) {
				if (ip->di.nlink > 0) {
					ip->di.nlink--;
				}
			}
		} else if (ip->di.nlink > 0) {
			if (ip->di.version >= SAM_INODE_VERS_2 &&
			    (operation != SAM_RENAME_LINK ||
			    (ip->di.ext_attrs & ext_hlp))) {
				(void) sam_get_hardlink_parent(ip, pip);
			}
		}
		if (SAM_IS_SHARED_SERVER(pip->mp)) {
			sam_notify_arg_t *notify;

			/*
			 * Shared server must notify all clients to stale DNLC.
			 */
			SAM_COUNT64(shared_server, notify_dnlc);
			notify = kmem_zalloc(sizeof (*notify), KM_SLEEP);
			strcpy(notify->p.dnlc.component, cp);
			sam_proc_notify(pip, NOTIFY_dnlc, 0, notify,
			    namep->client_ord);
			kmem_free(notify, sizeof (*notify));
		}
	}
	TRANS_INODE(ip->mp, ip);
	sam_mark_ino(ip, SAM_CHANGED);
	/*
	 * LQFS: Need inode lock around TRANS_DIR.  There might be
	 * a broader issue here for QFS in general.
	 */
	if (TRANS_ISTRANS(pip->mp)) {
		RW_LOCK_OS(&pip->inode_rwl, RW_WRITER);
		TRANS_DIR(pip, fbuf_params.fbp_off);
		RW_UNLOCK_OS(&pip->inode_rwl, RW_WRITER);
		fbwrite(fbp);
	} else if (SAM_SYNC_META(pip->mp)) {
		if (rw_tryenter(&pip->inode_rwl, RW_WRITER) == 0) {
			fbwrite(fbp);
			RW_LOCK_OS(&pip->inode_rwl, RW_WRITER);
			error = sam_update_inode(pip, SAM_SYNC_ONE, FALSE);
			RW_UNLOCK_OS(&pip->inode_rwl, RW_WRITER);
		} else {
			fbrelse(fbp, S_OTHER);
			sam_sync_meta(pip, NULL, credp);
			RW_UNLOCK_OS(&pip->inode_rwl, RW_WRITER);
		}
	} else {
		fbdwrite(fbp);
	}
	fbp = NULL;

	/*
	 * Notify arfind and event daemon of removal.
	 */
	sam_send_to_arfind(ip, AE_remove, 0);
	if (ip->mp->ms.m_fsev_buf) {
		if (operation == SAM_RENAME_LINK) {
			sam_disk_inode_t di;
			di.id = ip->di.id;
			di.parent_id = pip->di.id;
			sam_send_event(ip->mp, &di, ev_rename, 1, d_namehash,
			    ip->di.change_time.tv_sec);
		} else {
			sam_send_event(ip->mp, &ip->di, ev_remove, ip->di.nlink,
			    d_namehash, ip->di.change_time.tv_sec);
		}
	}

out:
	if (fbp != NULL) {
		fbrelse(fbp, S_OTHER);
	}
	if (write_lock) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	} else {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	}
out1:
	return (error);
}


/*
 * ----- sam_empty_dir - Verify directory is empty
 *	Given a parent directory inode, check if directory is empty.
 *	NOTE. Parent data_rwl lock set on entry.
 */

int			/* ERRNO if not empty, 0 if directory is empty. */
sam_empty_dir(sam_node_t *ip)
{
	int size;
	int i;
	struct sam_dirent *dp;
	struct fbuf *fbp = NULL;
	int error = 0;
	sam_u_offset_t offset;
	int fbsize;
	ino_t ino;
	offset_t inodir_size;

	inodir_size = ip->mp->mi.m_inodir->di.rm.size;
	size = ip->di.rm.size;
	offset = 0;

	if (ip->di.nlink != 2) {
		error = ENOTEMPTY;
		goto done;
	}

	while (size > 0) {
		int bsize;

		fbsize = SAM_FBSIZE(offset);
		bsize = fbsize;
		if (fbsize > DIR_BLK)  bsize = fbsize - DIR_BLK;
		error = fbread(SAM_ITOV(ip), offset, bsize, S_OTHER, &fbp);
		if (error) {
			return (error);
		}
		/* Validate directory block */
		if ((error = sam_validate_dir_blk(ip, offset,
		    bsize, &fbp)) != 0) {
			/* Not directory data, allow it to be removed. */
			error = 0;
			goto done;
		}
		dp = (struct sam_dirent *)(void *)fbp->fb_addr;
		i = 0;
		while (i < bsize) {
			if (dp->d_fmt > SAM_DIR_VERSION) { /* If full entry */
				ino = dp->d_id.ino;
				if ((ino == 0) ||
				    (SAM_ITOD(ino) > inodir_size) ||
				    (dp->d_reclen == 0)) {
					/*
					 * Invalid entry, allow directory
					 * removal
					 */
					error = 0;
					goto done;
				}
				if (!(((dp->d_namlen == 1) &&
				    (dp->d_name[0] == '.')) ||
				    ((dp->d_namlen == 2) &&
				    (dp->d_name[0] == '.') &&
				    (dp->d_name[1] == '.')))) {
					error = ENOTEMPTY;
					goto done;
				}
			}
			i += dp->d_reclen;
			dp = (struct sam_dirent *)(void *)(fbp->fb_addr + i);
		}
		offset += bsize;
		size -= bsize;
		fbrelse(fbp, S_OTHER);
		fbp = NULL;
	}
done:
	if (fbp != NULL) {
		fbrelse(fbp, S_OTHER);
	}
	return (error);
}


/*
 * ----- sam_get_hardlink_parent - Restore a previous parent of
 * a hardlinked inode.
 *
 * Change parent id of hardlinked inode if it's being removed
 * from a directory that matches the current parent.  The new
 * parent id will be one of the previous parent ids saved in
 * the inode extension.  If it's not a match, locate and remove
 * the directory id from an HLP inode extension.
 *
 * There are some shenanigans here to deal with past problems.
 * Specifically, we now support multiple HLP extension inodes
 * to deal with large numbers of hard links.  However, this has
 * not always been supported, and fsck can't allocate new HLP
 * extensions to fix them up properly, although it can repopulate
 * existing ones.  So we make sure to leave slots for fsck to
 * populate if we have reason to believe that we need them.
 *
 */
int				/* ERRNO if error, 0 if successful */
sam_get_hardlink_parent(
	sam_node_t *bip,	/* Pointer to base inode */
	sam_node_t *pip)	/* Pointer to directory inode for remove */
{
	int found = 0, error = 0;
	int n_ids;
	buf_t *ebp = NULL;
	sam_id_t eid;
	struct sam_inode_ext *eip = NULL;

	if ((bip->di.ext_attrs & ext_hlp) == 0) {
		/*
		 * We were asked to remove an HLP parent, and one doesn't
		 * exist.  We want to notify the authorities, and we also
		 * want to ALLOCATE an HLP header if nlinks > 1 so that
		 * fsck will perhaps fix things up.
		 */
		sam_req_ifsck(bip->mp, -1,
		    "sam_get_hardlink_parent: ext_hlp = 0", &bip->di.id);
		if (bip->di.nlink > 1) {
			error = sam_alloc_inode_ext(bip, S_IFHLP, 1,
			    &bip->di.ext_id);
			if (!error) {
				eid = bip->di.ext_id;
				error = sam_read_ino(bip->mp, eid.ino, &ebp,
				    (struct sam_perm_inode **)&eip);
				if (!error) {
					eip->ext.hlp.creation_time =
					    SAM_SECOND();
					eip->ext.hlp.change_time = SAM_SECOND();
					eip->ext.hlp.ids[0] = bip->di.parent_id;
					eip->ext.hlp.n_ids = 1;
					if (TRANS_ISTRANS(bip->mp)) {
						TRANS_WRITE_DISK_INODE(bip->mp,
						    ebp, eip, eid);
					} else {
			(void) sam_bwrite_noforcewait_dorelease(bip->mp, ebp);
					}
					bip->di.ext_attrs |= ext_hlp;
					TRANS_INODE(bip->mp, bip);
					sam_mark_ino(bip, SAM_CHANGED);
				}
			}
		} else {
			sam_free_inode_ext(bip, S_IFHLP,
			    SAM_ALL_COPIES, &bip->di.ext_id);
		}
		return (ENOCSI);
	}

	eid = bip->di.ext_id;
	while (eid.ino) {
		/*
		 * Find hardlink parent inode extension.
		 */
		if (error = sam_read_ino(bip->mp, eid.ino, &ebp,
					(struct sam_perm_inode **)&eip)) {
			break;
		}
		if (EXT_HDR_ERR(eip, eid, bip)) {
			error = ENOCSI;
			brelse(ebp);
			ebp = NULL;
			sam_req_ifsck(bip->mp, -1,
			    "sam_get_hardlink_parent: EXT_HDR", &bip->di.id);
			break;
		}
		if (S_ISHLP(eip->hdr.mode) &&
		    (n_ids = eip->ext.hlp.n_ids) != 0) {
			if (n_ids < 0 || n_ids > MAX_HLP_IDS_IN_INO) {
				error = ENOCSI;
				brelse(ebp);
				ebp = NULL;
				sam_req_ifsck(bip->mp, -1,
				    "sam_get_hardlink_parent: n_ids",
				    &bip->di.id);
				break;
			}
			if (bip->di.parent_id.ino == pip->di.id.ino &&
			    bip->di.parent_id.gen == pip->di.id.gen) {
				/*
				 * Change parent id if it matches remove path
				 * directory Copy last HLP id to parent and
				 * remove it from HLP.
				 */
				--n_ids;
				bip->di.parent_id = eip->ext.hlp.ids[n_ids];
				eip->ext.hlp.ids[n_ids].ino =
				    eip->ext.hlp.ids[n_ids].gen = 0;
				found = 1;
				break;
			} else {
				int i;
				/*
				 * Find id of the remove path directory, copy
				 * the last HLP id into its slot, and remove the
				 * last HLP id.  If not found, look for it in a
				 * subsequent extent.
				 */
				for (i = 0; i < n_ids; i++) {
					if (eip->ext.hlp.ids[i].ino ==
					    pip->di.id.ino &&
					    eip->ext.hlp.ids[i].gen ==
					    pip->di.id.gen) {
						--n_ids;
						eip->ext.hlp.ids[i] =
						    eip->ext.hlp.ids[n_ids];
						eip->ext.hlp.ids[n_ids].ino =
						    eip->ext.hlp.ids[
						    n_ids].gen = 0;
						found = 1;
						break;
					}
				}
				if (found) {
					break;
				}
			}

		}
		eid = eip->hdr.next_id;
		brelse(ebp);
		ebp = NULL;
		eip = NULL;
	}

	if (error) {
		if (ebp != NULL) {
			brelse(ebp);
		}
		TRACE(T_SAM_GET_HLP_EXT, SAM_ITOV(bip),
		    (int)bip->di.id.ino, found, error);
		return (error);
	}

	if (found) {
		/*
		 * Update the extension inode that we just pulled
		 * the (new) old parent from.
		 */
		eip->ext.hlp.change_time = SAM_SECOND();
		eip->ext.hlp.n_ids = n_ids;

		if (TRANS_ISTRANS(bip->mp)) {
			TRANS_WRITE_DISK_INODE(bip->mp, ebp, eip, eid);
		} else if (SAM_SYNC_META(bip->mp)) {
			error = sam_write_ino_sector(bip->mp, ebp, eid.ino);
		} else {
			(void) sam_bwrite_noforcewait_dorelease(bip->mp, ebp);
		}
		ebp = NULL;
		TRANS_INODE(bip->mp, bip);
		sam_mark_ino(bip, SAM_CHANGED);
	}

	if (ebp != NULL) {
		brelse(ebp);
		ebp = NULL;
	}

	if (bip->di.nlink == 1) {
		/*
		 * Free HLP extents.
		 */
		sam_free_inode_ext(bip, S_IFHLP, SAM_ALL_COPIES,
		    &bip->di.ext_id);
		bip->di.ext_attrs &= ~ext_hlp;
	}

	TRACE(T_SAM_GET_HLP_EXT, SAM_ITOV(bip), (int)bip->di.id.ino,
	    found, error);

	if (error) {
		return (error);
	}

	if (!found) {
		dcmn_err((CE_NOTE,
		    "SAM-QFS FS %s: Parent ID not found in HLP extension; "
		    "inode = %d.%d; HLP extension = %d.%d",
		    bip->mp->mt.fi_name,
		    bip->di.id.ino, bip->di.id.gen,
		    eid.ino, eid.gen));
		sam_req_ifsck(bip->mp, -1,
		    "sam_get_hardlink_parent: Parent ID not found",
		    &bip->di.id);
		error = ENOCSI;
	}
	return (error);
}


/*
 * ----- sam_resolve_fbuf - Given an offset within a file, calculate
 *	fbuf parameters. An fbuf must start on a page boundary (although
 *  the supplied offset can be anywhere in the page) and the length
 *	cannot exceed the page.
 */
static void
sam_resolve_fbuf(uint32_t offset, struct sam_fbuf_params *fbprp)
{
	fbprp->fbp_offset = offset;
	fbprp->fbp_reloff = offset & (DIR_BLK - 1);
	fbprp->fbp_off = offset - fbprp->fbp_reloff;
	fbprp->fbp_size = fbprp->fbp_psize = DIR_BLK;
}


/*
 * ----- sam_compress_dirblk - combine adjacent space areas
 * within a directory block. Remove individual directory cache
 * space entries, and replace with the "combined" ones.
 */

static void
sam_compress_dirblk(sam_node_t *dip, struct sam_dirent *dp,
    sam_offset_t blkoff)
{
	sam_uintptr_t reclen, offset = 0;
	struct sam_dirent *next_dp, *space_dp = NULL;
	const int dirblk_end = DIR_BLK - sizeof (struct sam_dirval);
	boolean_t space_combined = FALSE;
	boolean_t use_ednlc;
	dcanchor_t *dcap = &dip->i_danchor;
	uint64_t handle, space_offset;
	int orig_space_len = 0;
	dcret_t reply;
	vnode_t *dvp = SAM_ITOV(dip);

	if (dcap->dca_dircache != NULL) {
		use_ednlc = TRUE;
	} else {
		use_ednlc = FALSE;
	}
	space_offset = 0;
	do {
		next_dp = (struct sam_dirent *)((sam_uintptr_t)dp + offset);
		reclen = next_dp->d_reclen;
		if (next_dp->d_fmt) {			/* Is a name entry */
			if (space_dp && space_combined && use_ednlc) {
				/*
				 * Delete the original space entry.
				 */
				ASSERT(orig_space_len != 0);
				handle = SAM_LEN_TO_H(orig_space_len,
				    space_offset);
				reply = dnlc_dir_rem_space_by_handle(dcap,
				    handle);
				TRACE(T_SAM_EDNLC_REM_H, dvp, space_offset,
				    orig_space_len, reply);

				/*
				 * Add the compressed space
				 */
				handle = SAM_LEN_TO_H(space_dp->d_reclen,
				    space_offset);
				reply = dnlc_dir_add_space(dcap,
				    (uint_t)space_dp->d_reclen,
				    handle);
				if (reply != DOK) {
					use_ednlc = FALSE;
					TRACE(T_SAM_EDNLC_ERR, dvp, 0,
					    (uint_t)space_dp->d_reclen, reply);
				} else {
					TRACE(T_SAM_EDNLC_ADD_S, dvp,
					    (sam_tr_t)SAM_H_TO_OFF(handle),
					    (sam_tr_t)SAM_H_TO_LEN(handle),
					    reply);
				}
			}
			space_dp = NULL;
			space_offset = 0;
		} else {			/* Is a space entry */
			if (space_dp == NULL) {
				space_dp = next_dp;
				space_combined = FALSE;
				space_offset = (uint64_t)(blkoff + offset);
				orig_space_len = space_dp->d_reclen;
			} else {
				space_dp->d_reclen += reclen;
				space_combined = TRUE;
				if (use_ednlc) {
					/*
					 * delete the entry being combined
					 */
					handle = SAM_LEN_TO_H(reclen,
					    (blkoff + offset));
					reply = dnlc_dir_rem_space_by_handle(
					    dcap, handle);
					TRACE(T_SAM_EDNLC_REM_H, dvp,
					    (blkoff + offset),
					    reclen, reply);
				}
			}
		}
		offset += reclen;
	} while (reclen && (offset < dirblk_end));
	if (space_dp && space_combined && use_ednlc) {
		/*
		 * Delete the original space entry.
		 */
		ASSERT(orig_space_len != 0);
		handle = SAM_LEN_TO_H(orig_space_len, space_offset);
		reply = dnlc_dir_rem_space_by_handle(dcap, handle);
		TRACE(T_SAM_EDNLC_REM_H, dvp, space_offset,
		    orig_space_len, reply);

		/*
		 * Add the compressed space
		 */
		handle = SAM_LEN_TO_H(space_dp->d_reclen, space_offset);
		reply = dnlc_dir_add_space(dcap, (uint_t)space_dp->d_reclen,
		    handle);
		if (reply != DOK) {
			TRACE(T_SAM_EDNLC_ERR, dvp, 0,
			    (uint_t)space_dp->d_reclen, reply);
		} else {
			TRACE(T_SAM_EDNLC_ADD_S, dvp,
			    (sam_tr_t)SAM_H_TO_OFF(handle),
			    (uint_t)space_dp->d_reclen, reply);
		}
	}
}
