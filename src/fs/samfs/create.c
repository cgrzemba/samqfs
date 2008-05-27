/*
 * ----- sam/create.c - Process the directory create functions.
 *
 *	Processes the SAMFS inode directory functions.
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

#pragma ident "$Revision: 1.149 $"

#include "sam/osversion.h"

/*
 * ----- UNIX Includes
 */

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
#include <sys/stat.h>
#include <sys/mode.h>
#include <sys/vfs.h>
#include <sys/dnlc.h>
#include <sys/fbuf.h>
#include <sys/mount.h>
#include <sys/policy.h>

/*
 * ----- SAMFS Includes
 */
#include "sam/types.h"
#include "sam/samevent.h"

#include "inode.h"
#include "mount.h"
#include "rwio.h"
#include "dirent.h"
#include "extern.h"
#include "arfind.h"
#include "debug.h"
#include "quota.h"
#include "trace.h"
#include "ino_ext.h"
#include "kstats.h"
#include "pub/sam_errno.h"
#include "qfs_log.h"

static const struct sam_empty_dir empty_directory_template = {
	{S_IFDIR, sizeof (struct sam_dirent), 0, 0, 0, 1, '.', '\0', '\0'},
	{S_IFDIR, 0, 0, 0, 0, 2, '.', '.', '\0'}
};

extern ushort_t sam_dir_gennamehash(int nl, char *np);
static int sam_make_dir(sam_node_t *pip, sam_node_t *ip, cred_t *credp);
static int sam_make_dirslot(sam_node_t *ip, struct sam_name *namep,
							struct fbuf **fbpp);
static int sam_make_ino(sam_node_t *pip, vattr_t *vap, sam_node_t **ipp,
						cred_t *credp);
static int sam_set_hardlink_parent(sam_node_t *bip);
static void sam_enter_dnlc(sam_node_t *pip, sam_node_t *ip, char *cp,
				struct sam_name *namep, ushort_t slot_size);
static void sam_enter_dir_dnlc(sam_node_t *pip, sam_ino_t ino,
			uint32_t slot_offset, char *cp, ushort_t slot_size);

extern int sam_fbzero(sam_node_t *ip, offset_t offset, int tlen,
			sam_ioblk_t *iop, map_params_t *mapp,
			struct fbuf **rfbp);

/*
 * ----- sam_create_name - create an entry in the directory.
 *	Given a parent directory inode and component name, create a
 *	file/directory. If no error, return inode pointer and inode
 *	"held" (v_count incremented).
 */

int				/* ERRNO if error, 0 if successful. */
sam_create_name(
	sam_node_t *pip,	/* Pointer to parent directory inode. */
	char *cp,		/* Pointer to component name. */
	sam_node_t **ipp,	/* Pointer pointer to inode (returned). */
	struct sam_name *namep,	/* Pointer to sam_dirent that holds the slot */
	vattr_t *vap,		/* Pointer to attributes */
	cred_t *credp)		/* Credentials pointer. */
{
	ushort_t reclen;
	sam_u_offset_t offset;
	enum sam_op operation;
	sam_node_t *ip;
	int error;
	boolean_t write_lock = FALSE;
	struct fbuf *fbp;
	struct sam_dirent *dp;
	ushort_t dirsize, slot_size;

	/*
	 * Write access required to create in directory
	 */
	fbp = NULL;
	if ((error = sam_access_ino(pip, S_IWRITE, FALSE, credp))) {
		return (error);
	}
	if (SAM_DIRLEN(cp) > DIR_BLK) {		/* Validate length of record */
		return (EINVAL);
	}
	if (pip->di.nlink == 0) {
		return (ENOENT);
	}

	/*
	 * Create a new directory slot if there is no space for the
	 * new directory entry.
	 */
	if ((namep->type != SAM_BIG_SLOT) && (namep->type != SAM_EMPTY_SLOT)) {
		RW_LOCK_OS(&pip->inode_rwl, RW_WRITER);
		write_lock = TRUE;
		if ((error = sam_make_dirslot(pip, namep, &fbp)) != 0) {
			TRANS_INODE(pip->mp, pip);
			RW_UNLOCK_OS(&pip->inode_rwl, RW_WRITER);
			return (error);
		}
	} else {
		RW_LOCK_OS(&pip->inode_rwl, RW_READER);
	}

	/*
	 * For create, make new inode unless creating a hard link.
	 */
	operation = namep->operation;
	if ((operation != SAM_LINK) && (operation != SAM_RENAME_LINK)) {
		if ((error = sam_make_ino(pip, vap, &ip, credp))) {
			/*
			 * We may have created a new directory slot.
			 * Purge the directory cache to be safe.
			 */
			TRACE(T_SAM_EDNLC_ERR, SAM_ITOV(pip), 0, 0, error);
			dnlc_dir_purge(&pip->i_danchor);
			if (fbp != NULL) {
				fbrelse(fbp, S_OTHER);
			}
			if (write_lock) {
				RW_UNLOCK_OS(&pip->inode_rwl, RW_WRITER);
			} else {
				RW_UNLOCK_OS(&pip->inode_rwl, RW_READER);
			}
			return (error);
		}
		/*
		 *
		 * For directory, create a new empty directory.
		 * ip->inode_rwl RW_WRITER lock set in sam_make_ino()
		 */
		if (vap->va_type == VDIR) {
			if ((error = sam_make_dir(pip, ip, credp))) {
				goto out;
			}
		}
	} else {			/* For link, use existing inode */
		ip = *ipp;
		if (!S_ISDIR(ip->di.mode)) {
			/*
			 * Save link to existing parent before switching.
			 * But if we're doing a SAM_RENAME_LINK and there's
			 * no extension inode, then just replace the parent
			 * link with the new parent.
			 */
			if (ip->di.version >= SAM_INODE_VERS_2 &&
			    (operation != SAM_RENAME_LINK ||
			    (ip->di.ext_attrs & ext_hlp))) {
				if ((error = sam_set_hardlink_parent(ip)) !=
				    0) {
					goto out;
				}
			}
			ip->di.parent_id = pip->di.id;

			/*
			 * Capture access control list if it's being inherited
			 * from the existing parent before switching.
			 */
			if ((ip->di.status.b.acl || ip->di.status.b.dfacl) &&
			    !ip->aclp &&
			    !(ip->di.ext_attrs & ext_acl)) {
				if (error = sam_acl_inherit(ip, pip,
				    &ip->aclp)) {
					if (ip->di.version >=
					    SAM_INODE_VERS_2 &&
					    (operation != SAM_RENAME_LINK ||
					    (ip->di.ext_attrs & ext_hlp))) {
						(void) sam_get_hardlink_parent(
						    pip, ip);
					}
					goto out;
				}
				/* Save to ext(s). */
				ip->aclp->flags |= ACL_MODIFIED;
			}
		}
	}
	ASSERT(namep->type == SAM_EMPTY_SLOT || namep->type == SAM_BIG_SLOT);
	reclen = namep->data.slot.reclen;
	offset = namep->data.slot.offset;
	if (fbp == NULL) {
		int fbsize, bsize;

		fbsize = SAM_FBSIZE(offset);
		bsize = fbsize;
		if (fbsize > DIR_BLK) {
			bsize = fbsize - DIR_BLK;
		}
		/*
		 * parent directory inode can be the same as target inode when
		 * linking directory "."  Don't lock inode twice.
		 */
		if (pip == ip) {
			ASSERT(RW_OWNER_OS(&ip->inode_rwl) == curthread);
		} else {
			if (!rw_tryupgrade(&pip->inode_rwl)) {
				RW_UNLOCK_OS(&pip->inode_rwl, RW_READER);
				RW_LOCK_OS(&pip->inode_rwl, RW_WRITER);
			}
			write_lock = TRUE;
		}
		error = fbread(SAM_ITOV(pip), offset, bsize, S_WRITE, &fbp);
		if (error) {
			goto out;
		}
	}
	dp = (struct sam_dirent *)(void *)fbp->fb_addr;
	if (namep->type == SAM_BIG_SLOT) {

		/*
		 * Divide the big slot into the "old" entry part (first),
		 * (Remember, the "old" part is a valid directory entry)
		 * and the new entry part (second).
		 */
		dirsize = SAM_DIRSIZ(dp);
		reclen -= dirsize;
		dp->d_reclen = dirsize;
		dp = (struct sam_dirent *)(void *)((char *)dp + dirsize);
		namep->data.slot.offset += dirsize;
		namep->data.slot.reclen -= dirsize;
	}
	dp->d_id = ip->di.id;
	dp->d_fmt = ip->di.mode & S_IFMT;
	dp->d_namlen = strlen(cp);
	dp->d_reclen = reclen;
	strcpy((char *)dp->d_name, cp);
	dp->d_namehash = sam_dir_gennamehash(dp->d_namlen, cp);
	if (S_ISDIR(ip->di.mode)) {
		if ((operation != SAM_LINK) && (operation != SAM_RENAME_LINK)) {
			pip->di.nlink++;
		}
	}
	sam_mark_ino(pip, (SAM_UPDATED | SAM_CHANGED));
	/*
	 * Notify arfind of file/directory creation. This triggers the
	 * archiving of files that are created on the shared clients.
	 */
	sam_send_to_arfind(ip, AE_create, 0);

	/*
	 * Notify event daemon of file/directory creation.
	 */
	if (operation != SAM_RENAME_LINK) {
		sam_send_event(ip, ev_create, 0);
	}

	/*
	 * Add to the DNLC cache and directory cache.
	 */
	slot_size = dp->d_reclen - SAM_DIRSIZ(dp);
	sam_enter_dnlc(pip, ip, cp, namep, slot_size);

	if ((operation == SAM_LINK) || (operation == SAM_RENAME_LINK)) {
		ip->di.nlink++;
		if (!ip->di.status.b.worm_rdonly) {
			sam_mark_ino(ip, SAM_CHANGED);
		}
	} else {
		sam_mark_ino(ip, (SAM_ACCESSED | SAM_UPDATED | SAM_CHANGED));
	}
	*ipp = ip;

	/*
	 * If sync_meta == 1, sync parent, child and directory pages.
	 * Do not sync symlink inode now because it will be done after
	 * the symlink has been created.
	 */
	TRANS_DIR(pip, namep->data.slot.offset);
	if (TRANS_ISTRANS(pip->mp)) {
		fbwrite(fbp);
	} else if (SAM_SYNC_META(pip->mp) && (operation != SAM_SYMLINK)) {
		fbrelse(fbp, S_OTHER);
		/* sync will be done by sam_rename_vn */
		if (operation != SAM_RENAME_LINK) {
			sam_sync_meta(pip, ip, credp);
		}
	} else {
		fbdwrite(fbp);
	}
	TRANS_INODE(ip->mp, ip);
	TRANS_INODE(pip->mp, pip);
	if ((operation != SAM_LINK) && (operation != SAM_RENAME_LINK)) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	}
	if (write_lock) {
		RW_UNLOCK_OS(&pip->inode_rwl, RW_WRITER);
	} else {
		RW_UNLOCK_OS(&pip->inode_rwl, RW_READER);
	}
	return (0);

out:
	/*
	 * We may have created a new directory slot.
	 * Purge the directory cache to be safe.
	 */
	TRACE(T_SAM_EDNLC_ERR, SAM_ITOV(pip), 0, 1, error);
	dnlc_dir_purge(&pip->i_danchor);
	if (fbp != NULL) {
		fbrelse(fbp, S_OTHER);
	}
	TRANS_INODE(pip->mp, pip);
	if (write_lock) {
		RW_UNLOCK_OS(&pip->inode_rwl, RW_WRITER);
	} else {
		RW_UNLOCK_OS(&pip->inode_rwl, RW_READER);
	}
	if ((operation != SAM_LINK) && (operation != SAM_RENAME_LINK)) {
		if (S_ISDIR(ip->di.mode)) {		/* If directory */
			--ip->di.nlink;
		}
		/* Number of references to file */
		if ((int)--ip->di.nlink < 0) {
			ip->di.nlink = 0;
		}
		TRANS_INODE(ip->mp, ip);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		VN_RELE(SAM_ITOV(ip));	/* Decrement v_count */
	}
	return (error);
}


/*
 * ----- sam_restore_name - restore an inode and its name.
 *	Given a parent directory inode and component name, create a
 *	file/directory. If no error, return inode pointer and inode
 *	"held" (v_count incremented).
 */

int					/* ERRNO if error, 0 if successful. */
sam_restore_name(
	sam_node_t *pip,		/* parent directory inode. */
	char *cp,			/* Pointer to component name. */
	struct sam_name *namep,		/* sam_dirent that holds the slot */
	struct sam_perm_inode *permp,	/* Pointer to sam permanent inode */
	sam_id_t *ridp,			/* Returned id pointer */
	cred_t *credp)			/* Credentials pointer. */
{
	ushort_t reclen;
	sam_u_offset_t offset;
	uint32_t slot_offset;
	ushort_t slot_size;
	buf_t *bp;
	int error, i, inode_found;
	struct fbuf *fbp;
	struct sam_dirent *dirp;
	sam_mount_t *mp;
	sam_id_t id;
	struct sam_perm_inode *permip;

	/*
	 * Write access required to create in directory
	 */
	fbp = NULL;
	ridp->ino = ridp->gen = 0;
	if ((error = sam_access_ino(pip, S_IWRITE, FALSE, credp))) {
		return (error);
	}
	if (SAM_DIRLEN(cp) > DIR_BLK) {		/* Validate length of record */
		return (EINVAL);
	}
	if (pip->di.nlink == 0) {
		return (ENOENT);
	}

	/*
	 * Create a new directory slot if there is no space for the
	 * new directory entry.
	 */
	RW_LOCK_OS(&pip->inode_rwl, RW_WRITER);
	if ((namep->type != SAM_BIG_SLOT) && (namep->type != SAM_EMPTY_SLOT)) {
		if ((error = sam_make_dirslot(pip, namep, &fbp)) != 0) {
			TRANS_INODE(pip->mp, pip);
			RW_UNLOCK_OS(&pip->inode_rwl, RW_WRITER);
			return (error);
		}
	}

	/*
	 * Allocate an on-disk inode for the restore. Types can be taken from
	 * the "passed in" prototype.
	 */
	mp = pip->mp;
	do {
		sam_node_t *ip;

		if (mp->mt.fi_status & FS_UMOUNT_IN_PROGRESS) {
			TRACE(T_SAM_UMNT_IGET, NULL, mp->mt.fi_status,
			    (sam_tr_t)mp,
			    EBUSY);
			error = EBUSY;
			goto out;
		}
		inode_found = 0;
		if ((error = sam_alloc_inode(mp, permp->di.mode, &id.ino))) {
			goto out;
		}
		if ((error = sam_check_cache(&id, mp, IG_NEW, NULL, &ip))) {
			SAMFS_PANIC_INO(mp->mt.fi_name, "inode allocation",
			    id.ino);
			goto out;
		}
		if (ip != NULL) {
			VN_RELE(SAM_ITOV(ip));
			inode_found = 1;
			ASSERT(ip->di.free_ino == 0);
		}
	} while (inode_found);
	if ((error = sam_read_ino(mp, id.ino, &bp, &permip))) {
		goto out;
	}

	/*
	 * Replace fields in the on-disk inode.
	 * If you make a change here, you probably need to make it in
	 * sammkfs -r and fioctl.c (IDRESTORE) also.
	 */
	id = permip->di.id;
	if (++id.gen == 0) {
		id.gen++;
	}
	*ridp = id;
	bcopy((void *)permp, (void *)permip, sizeof (struct sam_perm_inode));
	permip->di.version = pip->di.version;
	permip->di.id = id;
	permip->di.parent_id = pip->di.id;
	permip->di.ext_id.ino = permip->di.ext_id.gen = 0;
	permip->di.ext_attrs = 0;
	permip->di.status.b.archdone = 0;
	permip->di.blocks = 0;
	if (S_ISREG(permip->di.mode) && !S_ISSEGI(&permip->di)) {
		permip->di.status.b.on_large =
		    (permp->di.rm.size > SM_OFF(mp, DD)) ? 1 : 0;
	}
	if (SAM_IS_OBJECT_FS(mp) && S_ISREG(permip->di.mode)) {
		if ((error = sam_create_object_id(mp, &permip->di))) {
			brelse(bp);
			goto out;
		}
	} else {
		for (i = (NOEXT - 1); i >= 0; i--) {
			permip->di.extent[i] = 0;
			permip->di.extent_ord[i] = 0;
		}
	}
	if (permip->di.rm.size != 0) {	/* if space in the original inode */
		permip->di.status.b.offline = 1;
		permip->di.status.b.pextents = 0;
	}
	(void) sam_quota_balloc_perm(mp, permip,
	    0, S2QBLKS(permip->di.rm.size), CRED());
	(void) sam_quota_falloc(mp, permip->di.uid,
	    permip->di.gid, permip->di.admin_id, CRED());
	sam_set_unit(mp, &(permip->di));

	/*
	 * Write the directory entry.
	 */
	reclen = namep->data.slot.reclen;
	offset = namep->data.slot.offset;
	slot_offset = namep->data.slot.offset;
	if (fbp == NULL) {
		int fbsize, bsize;

		fbsize = SAM_FBSIZE(offset);
		bsize = fbsize;
		if (fbsize > DIR_BLK) {
			bsize = fbsize - DIR_BLK;
		}
		error = fbread(SAM_ITOV(pip), offset, bsize, S_WRITE, &fbp);
		if (error) {
			brelse(bp);
			goto out;
		}
	}
	dirp = (struct sam_dirent *)(void *)fbp->fb_addr;
	if (namep->type == SAM_BIG_SLOT) {
		ushort_t dirsiz;

		dirsiz = SAM_DIRSIZ(dirp);
		reclen -= dirsiz;
		dirp->d_reclen = dirsiz;
		slot_offset += (uint32_t)dirsiz;
		dirp = (struct sam_dirent *)
		    (void *)((char *)dirp + SAM_DIRSIZ(dirp));
	}
	dirp->d_id = permip->di.id;
	dirp->d_fmt = permip->di.mode & S_IFMT;
	dirp->d_namlen = strlen(cp);
	dirp->d_reclen = reclen;
	strcpy((char *)dirp->d_name, cp);
	dirp->d_namehash = sam_dir_gennamehash(dirp->d_namlen, cp);

	/*
	 * Do not add this to the DNLC cache because we don't want
	 * to replace existing entries with names that are unlikely
	 * to be used.  However, we must remove any negative DNLC
	 * entries that may exist.
	 */
	dnlc_remove(SAM_ITOV(pip), cp);

	/*
	 * Add to the directory cache.
	 */
	slot_size = dirp->d_reclen - SAM_DIRSIZ(dirp);
	sam_enter_dir_dnlc(pip, id.ino, slot_offset, cp, slot_size);

	TRANS_DIR(pip, slot_offset);
	if (TRANS_ISTRANS(pip->mp)) {
		(void) sam_bwrite_noforcewait_dorelease(pip->mp, bp);
		fbwrite(fbp);
	} else if (pip->mp->mt.fi_config & MT_SHARED_WRITER) {
		(void) sam_bwrite_noforcewait_dorelease(pip->mp, bp);
		fbwrite(fbp);
	} else {
		bdwrite(bp);
		fbdwrite(fbp);
	}
	TRANS_INODE(pip->mp, pip);
	RW_UNLOCK_OS(&pip->inode_rwl, RW_WRITER);
	return (0);

out:
	/*
	 * We may have created a new directory slot.
	 * Purge the directory cache to be safe.
	 */
	TRACE(T_SAM_EDNLC_ERR, SAM_ITOV(pip), 0, 2, error);
	dnlc_dir_purge(&pip->i_danchor);
	if (fbp != NULL) {
		fbrelse(fbp, S_OTHER);
	}
	TRANS_INODE(pip->mp, pip);
	RW_UNLOCK_OS(&pip->inode_rwl, RW_WRITER);
	return (error);
}


/*
 * ----- sam_make_ino - make an ino entry.
 *	Given a parent directory inode and new file attributes,
 *	create a new inode and fill in the inode information.
 *	Inode read/write lock set on return with no error.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_make_ino(
	sam_node_t *pip,	/* Pointer to parent directory inode */
	vattr_t	*vap,		/* Pointer to attributes. */
	sam_node_t **ipp,	/* Pointer pointer to inode (returned). */
	cred_t *credp)		/* Credentials pointer. */
{
	sam_mount_t *mp;
	int	error = 0;
	sam_id_t id;
	sam_node_t *ip;
	vnode_t *vp;
	struct vfs *vfsp;
	mode_t mode;
	uid_t nuid;
	gid_t ngid;
	int type, naid;

	*ipp = NULL;
	mp = pip->mp;
	type = vap->va_type;
	nuid = crgetuid(credp);
	/*
	 * To determine the group-id of a newly created file or directory:
	 *  1) if the gid is set in the attributes, use it if
	 *		a) process is superuser, or
	 *		b) is the same gid as the parent directory, or
	 *		c) specified gid is a groupmember.
	 *			- or -
	 *  2) If directory set-gid bit is set, use the gid as the parent
	 *	   directory
	 *			- or -
	 *  3) use the process's gid.
	 */
	ngid = crgetgid(credp);
	if ((vap->va_mask & AT_GID) &&
	    ((vap->va_gid == pip->di.gid) ||
	    groupmember(vap->va_gid, credp) ||
	    (secpolicy_vnode_create_gid(credp) == 0))) {
		ngid = vap->va_gid;
	} else if ((pip->di.mode & S_ISGID)) {	/* If set gid on execution */
		ngid = pip->di.gid;
	}
	naid = pip->di.admin_id;
	if ((error = sam_quota_falloc(mp, nuid, ngid, naid, credp))) {
		return (error);
	}
	mode = MAKEIMODE(type, vap->va_mode);
	if ((error = sam_alloc_inode(mp, mode, &id.ino))) {
		return (error);
	}
	/*
	 * VN_HOLD if found in sam_get_ino.
	 */
	id.gen = 0;
	vfsp = mp->mi.m_vfsp;
	if ((error = sam_get_ino(vfsp, IG_NEW, &id, &ip))) {
		if (error != EBUSY) {
			SAMFS_PANIC_INO(mp->mt.fi_name,
			    "Invalid inode allocation", id.ino);
		}
		return (error);
	}
	if (ip->di.free_ino != 0) {
		dcmn_err((CE_WARN,
		    "SAM-QFS: %s: Free inode %d.%d pointer is not zero = %d",
		    mp->mt.fi_name, ip->di.id.ino, ip->di.id.gen,
		    ip->di.free_ino));
		ip->di.free_ino = 0;
	}

	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	*ipp = ip;
	vp = SAM_ITOV(ip);
	vp->v_type = type;		/* Vnode type (for create) */
	if (type == VDIR) {
		ip->di.nlink = 2;	/* Number of references for new dir */
	} else {
		ip->di.nlink = 1;	/* Number of references for new file */
	}
	ip->di.uid = nuid;
	ip->di.gid = ngid;
	ip->di.admin_id = naid;
	ip->di.mode = mode;

	/*
	 * Propagate the set-GID bit if creating a directory.  Otherwise,
	 * clear the set-GID bit unless the process is superuser or member of
	 * the same group.
	 */
	if (type == VDIR && (pip->di.mode & S_ISGID)) {
		ip->di.mode |= S_ISGID;
	} else if ((ip->di.mode & S_ISGID) &&
	    !groupmember(ip->di.gid, credp) &&
	    secpolicy_vnode_setids_setgids(credp, ip->di.gid)) {
		ip->di.mode &= ~S_ISGID;
	}
	ip->di.version = pip->di.version;		/* Use parent version */
	ip->di.creation_time = SAM_SECOND();
	ip->di.attribute_time = ip->di.creation_time;
	ip->di.residence_time = ip->di.creation_time;

	ip->di.parent_id = pip->di.id;
	ip->di.rm.size = 0;		/* File size in bytes */
	ip->size = 0;			/* Current file size in bytes */
	ip->di.blocks = 0;		/* Current file size in blocks */
	ip->dev	= vfsp->vfs_dev;
	if (type == VBLK || type == VCHR) {
		ip->rdev = vap->va_rdev;
		if (cmpldev((dev32_t *)&ip->di.psize.rdev, ip->rdev) == 0) {
			error = EOVERFLOW;
			goto make_cleanup;
		}
	} else {
		ip->rdev = 0;
	}
	vp->v_rdev = ip->rdev;

	/*
	 * Propagate -
	 * cs_gen, cs_use,
	 * stage_all, noarch, bof_online,
	 * direct, nodrop, release
	 * checksum algorithm, segment,
	 * stripe, default ACLs
	 * setfa -q (aio)
	 */
	if (type == VDIR) {
		/*
		 * Propagate worm_rdonly and retention period to directories.
		 */
		ip->di.status.bits = pip->di.status.bits & SAM_DIRINHERIT_MASK;
		if (pip->di.status.b.worm_rdonly &&
		    (pip->di.version >= SAM_INODE_VERS_2)) {
			ip->di2.rperiod_duration = pip->di2.rperiod_duration;
			ip->di2.rperiod_start_time =
			    pip->di2.rperiod_start_time;
		}
	} else {
		ip->di.status.bits = pip->di.status.bits & SAM_INHERIT_MASK;
	}
	/*
	 * Propagate inheritable rmedia flags
	 */
	ip->di.rm.ui.flags = pip->di.rm.ui.flags & RM_INHERIT_MASK;

	if (ip->di.status.b.bof_online) {
		ip->di.psize.partial = pip->di.psize.partial;
		ip->di.status.b.pextents = 1;
	}
	ip->di.cs_algo = pip->di.cs_algo;
	ip->di.stripe = pip->di.stripe;
	ip->di.stripe_group = pip->di.stripe_group;
	ip->flags.b.stage_n = ip->di.status.b.direct;
	ip->flags.b.stage_all = ip->di.status.b.stage_all;

	sam_reset_default_options(ip);

	if (ip->di.status.b.segment) {
		/*
		 * Segment is only allowed with regular files and
		 * directories.
		 */
		if (S_ISREG(ip->di.mode) || S_ISDIR(ip->di.mode)) {
			ip->di.rm.info.dk.seg_size =
			    pip->di.rm.info.dk.seg_size;
			ip->di.stage_ahead = pip->di.stage_ahead;
		} else {
			ip->di.status.b.segment = 0;
		}
	}

	if (pip->di.status.b.dfacl) {
		error = sam_acl_create(ip, pip);
		if (error) {
			goto make_cleanup;
		}
	}

	sam_set_unit(ip->mp, &ip->di);
	if (SAM_IS_OBJECT_FS(mp) && S_ISREG(ip->di.mode)) {
		if ((error = sam_create_object_id(mp, &ip->di))) {
			goto make_cleanup;
		}
	}
	return (0);

	/*
	 * Clean up after a failed inode allocation.
	 * Return it to the file system.
	 */

make_cleanup:
	*ipp = NULL;
	ip->di.nlink = 0;
	TRANS_INODE(ip->mp, ip);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	sam_inactive_ino(ip, CRED());
	return (error);
}


/*
 * ----- sam_set_unit - set the unit.
 */

void
sam_set_unit(
	sam_mount_t *mp,			/* Mount table pointer. */
	struct sam_disk_inode *di)		/* Disk inode pointer. */
{
	int i, ord, oldord, mask;
	mode_t	mode = di->mode;
	boolean_t striped = FALSE;

	/*
	 * Set the slice based on round robin and then set the next ordinal.
	 */
	if (S_ISDIR(mode) && mp->mt.mm_count) {
		di->status.b.meta = 1;		/* Directories on meta device */
	}
	i = di->status.b.meta;	/* Device type: data (DD) or meta (MM) */
	di->stride = 0;		/* Reset stride to 0 */
	mask = 0;
	if (i == DD) {
		if (SAM_IS_OBJECT_FS(mp)) {
			di->status.b.stripe_group = 1;
			mask = DT_TARGET_GROUP;
		} else if (di->status.b.stripe_group) {
			mask = DT_STRIPE_GROUP;
		}
	}
	if (mask) {
		for (ord = 0; ord < mp->mt.fs_count; ord++) {
			if ((di->stripe_group|mask) ==
			    mp->mi.m_fs[ord].part.pt_type) {
				di->unit = (uchar_t)ord;
				striped = TRUE;
				break;
			}
		}
		if (ord == mp->mt.fs_count) {		/* Should not happen */
			di->status.b.stripe_group = 0;
			di->stripe_group = 0;
		}
	}
	if (!striped)  {
		ord = oldord = mp->mi.m_unit[i];	/* Round robin files */
		while (mp->mi.m_sbp->eq[ord].fs.space == 0) {
			if (mp->mi.m_fs[ord].next_ord) {
				ord = mp->mi.m_fs[ord].next_ord;
			} else {
				ord = mp->mi.m_dk_start[i];
			}
			if (ord == oldord) {
				break;		/* give up, out of space */
			}
		}
		di->unit = (uchar_t)ord;
		if (mp->mi.m_fs[ord].next_ord) {
			mp->mi.m_unit[i] = mp->mi.m_fs[ord].next_ord;
		} else {
			mp->mi.m_unit[i] = mp->mi.m_dk_start[i];
		}
	}
	if (mp->mi.m_fs[ord].part.pt_type != DT_DATA) {
		di->status.b.on_large = 1;
	}

	if (S_ISDIR(mode) && mp->mt.mm_count) {
		/* If SM daus on meta device */
		if (SM_BLK(mp, MM) != LG_BLK(mp, LG)) {
			di->status.b.on_large = 0;
		}
	}
}


/*
 * ----- sam_make_dir - make an empty directory.
 *	Allocate a block and fill in an empty directory entry.
 */
/*ARGSUSED2*/
static int			/* ERRNO if error, 0 if successful. */
sam_make_dir(
	sam_node_t *pip,	/* Pointer to parent inode. */
	sam_node_t *ip,		/* Pointer to inode. */
	cred_t *credp)		/* Credentials pointer. */
{
	struct fbuf *fbp;
	int	error;
	struct sam_empty_dir *ep;

	if (ip->mp->mt.mm_count > 1) {
		/*
		 * If multiple metadata partitions, assign the first
		 * directory ordinal based on the current .inodes file
		 * "unit".
		 */
		ip->di.unit = ip->mp->mi.m_inodir->di.unit;
	}
	if ((error = sam_make_dirslot(ip, NULL, &fbp)) != 0) {
		return (error);
	}
	ep = (struct sam_empty_dir *)(void *)fbp->fb_addr;
	*ep = empty_directory_template;
	ep->dot.d_id = ip->di.id;
	ep->dotdot.d_id = pip->di.id;
	ep->dotdot.d_reclen = DIR_BLK - (sizeof (struct sam_dirent) +
	    sizeof (struct sam_dirval));
	/*
	 * Always sync. the empty directory.
	 */
	TRANS_DIR(ip, (sam_u_offset_t)0);
	fbwrite(fbp);
	return (0);
}


/*
 * ----- sam_make_dirslot - make a directory slot for a new name.
 *   Given a directory inode, create a slot big enough to hold the
 *   new directory entry. The offset and length of the new directory
 *   slot is in the sam_name structure. The block is initialized,
 *   the validation records filled in, and fbp is returned as non-null
 *   to indicate the new buffer.
 *
 *   If directory is currently expanding beyond the small extents, allocate
 *   enough blocks to hold the small extent data + a new dirslot (superblock
 *   version v2 and above).
 *
 *   Caller must have the inode write lock held, and this lock is not
 *   released.
 */

static int
sam_make_dirslot(
	sam_node_t *ip,			/* Pointer to directory inode. */
	struct sam_name *namep,		/* Pointer to sam_dirent */
	struct fbuf **fbpp)		/* Pointer to new directory buffer */
{
	ushort_t reclen;
	sam_u_offset_t offset;
	int error;
	int fbsize, bsize;
	struct sam_dirval *dvp;
	struct sam_dirent *dp;
	struct fbuf *fbufp;		/* Pointer to new directory buffer */

	*fbpp = NULL;
	ASSERT(RW_WRITE_HELD(&ip->inode_rwl));
	offset = ip->di.rm.size;
	if ((ip->di.version > SAM_INODE_VERS_1) &&
	    (ip->di.status.b.on_large == 0)) {
		int dt;

		dt = ip->di.status.b.meta;
		if (offset == SM_OFF(ip->mp, dt)) {
			sam_copy_to_large(ip, DIR_BLK);
		}
	}
	fbsize = SAM_FBSIZE(offset);
	bsize = fbsize;
	if (fbsize > DIR_BLK) {
		bsize = fbsize - DIR_BLK;
	}
	if ((error = sam_map_block(ip, (offset_t)offset, (offset_t)bsize,
	    SAM_WRITE, NULL, CRED()))) {
		return (error);
	}

	/*
	 * Zero the new space and get an fbuf pointer
	 * to the incore page.
	 */
	error = sam_fbzero(ip, offset, bsize, NULL, NULL, &fbufp);

	if (error) {
		ip->size -= bsize;
		return (error);
	}
	ip->di.rm.size = ip->size;
	dp = (struct sam_dirent *)(void *)fbufp->fb_addr;
	reclen = bsize - sizeof (struct sam_dirval);
	dp->d_reclen = reclen;
	dvp = (struct sam_dirval *)(void *)((char *)dp + reclen);
	dvp->d_version = SAM_DIR_VERSION;
	dvp->d_reclen = sizeof (struct sam_dirval);
	dvp->d_id = ip->di.id;
	dvp->d_time = SAM_SECOND();
	/*
	 *	Write out the empty directory block. This is needed
	 *	to prevent a error in further create code from having
	 *	the block allocated, size set and no data written.
	 */
	TRANS_DIR(ip, offset);
	if ((TRANS_ISTRANS(ip->mp)) || (SAM_SYNC_META(ip->mp))) {
		fbwrite(fbufp);
	} else {
		fbdwrite(fbufp);
	}
	error = fbread(SAM_ITOV(ip), offset, bsize, S_WRITE, &fbufp);
	if (error) {
		/*
		 * We have created a new directory slot.
		 * Purge the directory cache to be safe.
		 */
		TRACE(T_SAM_EDNLC_ERR, SAM_ITOV(ip), 0, 3, error);
		dnlc_dir_purge(&ip->i_danchor);
		return (error);
	}
	*fbpp = fbufp;
	if (namep != NULL) {
		namep->data.slot.reclen = reclen;
		namep->data.slot.offset = offset;
		namep->type = SAM_EMPTY_SLOT;
	}
	return (0);
}


/*
 * ----- sam_set_symlink - Store a SAM-QFS symbolic link name.
 * Store the symbolic link name in multiple inode extensions, if needed.
 *
 * Caller must have the inode write lock held.  This lock is not released.
 */

int				/* ERRNO if error, 0 if successful */
sam_set_symlink(
	sam_node_t *pip,	/* Pointer to parent inode */
	sam_node_t *bip,	/* Pointer to symlink inode */
	char *sln,		/* Pointer to the symbolic link name. */
	int n_chars,		/* Number of chars in symbolic link name */
	cred_t *credp)		/* Credentials pointer. */
{
	int error = 0;
	int chars;
	int n_exts;
	buf_t *bp;
	sam_id_t eid;
	sam_ino_t ino;
	struct sam_inode_ext *eip;
	char *stsln;
	int ino_chars;

	ASSERT(RW_WRITE_HELD(&bip->inode_rwl));
	ASSERT(bip->di.version >= SAM_INODE_VERS_2);

	stsln = sln;
	if (n_chars) {			/* Any characters */
		n_exts = howmany(n_chars, MAX_SLN_CHARS_IN_INO);
	} else {
		n_exts = 1;		/* Allocate to hold the null string */
	}
	if ((error = sam_alloc_inode_ext(bip, S_IFSLN, n_exts,
	    &bip->di.ext_id)) == 0) {
		eid = bip->di.ext_id;
		chars = n_chars;
		do {
			ino = eid.ino;
			if (error = sam_read_ino(bip->mp, ino, &bp,
					(struct sam_perm_inode **)&eip)) {
				break;
			}
			if (EXT_HDR_ERR(eip, eid, bip)) {
				sam_req_ifsck(bip->mp, -1,
				    "sam_set_symlink: EXT_HDR", &bip->di.id);
				error = ENOCSI;
				brelse(bp);
				break;
			}
			if (S_ISSLN(eip->hdr.mode)) {
				eip->ext.sln.creation_time = SAM_SECOND();
				eip->ext.sln.change_time =
				    eip->ext.sln.creation_time;
				ino_chars = MIN(chars, MAX_SLN_CHARS_IN_INO);
				eip->ext.sln.n_chars = ino_chars;
				if (ino_chars > 0) {
					bcopy(sln,
					    (char *)&eip->ext.sln.chars[0],
					    ino_chars);
					sln += ino_chars;
					chars -= ino_chars;
				}

				eid = eip->hdr.next_id;
				if (TRANS_ISTRANS(bip->mp)) {
					sam_ioblk_t ioblk;

					RW_LOCK_OS(
					    &bip->mp->mi.m_inodir->inode_rwl,
					    RW_READER);
					error = sam_map_block(
					    bip->mp->mi.m_inodir,
					    (offset_t)SAM_ITOD(
					    eip->hdr.id.ino),
					    SAM_ISIZE, SAM_READ, &ioblk,
					    CRED());
					RW_UNLOCK_OS(
					    &bip->mp->mi.m_inodir->inode_rwl,
					    RW_READER);
					if (!error) {
						offset_t doff;

						doff = ldbtob(
						    fsbtodb(bip->mp,
						    ioblk.blkno)) + ioblk.pboff;
						TRANS_EXT_INODE(bip->mp,
						    eip->hdr.id, doff,
						    ioblk.ord);
					} else {
						error = 0;
					}
					brelse(bp);
				} else if (SAM_SYNC_META(bip->mp)) {
					error = sam_write_ino_sector(bip->mp,
					    bp, ino);
				} else {
					bdwrite(bp);
				}
			} else {
				brelse(bp);
				break;
			}
		} while (eid.ino && chars > 0);

		if (error) {
			sam_free_inode_ext(bip, S_IFSLN, SAM_ALL_COPIES,
			    &bip->di.ext_id);
		} else {
			if (n_chars <= SAM_SYMLINK_SIZE) {
				char *ichar = (char *)&bip->di.extent[0];

				bcopy(stsln, ichar, n_chars);
			}
			bip->di.ext_attrs |= ext_sln;
			bip->di.psize.symlink = n_chars;
			TRANS_INODE(bip->mp, bip);
			sam_mark_ino(bip, SAM_CHANGED);
			bip->flags.b.updated = 1;
			if ((TRANS_ISTRANS(bip->mp)) ||
			    (SAM_SYNC_META(bip->mp))) {
				if (pip) {
					RW_LOCK_OS(&pip->inode_rwl, RW_WRITER);
					sam_sync_meta(pip, bip, credp);
					RW_UNLOCK_OS(&pip->inode_rwl,
					    RW_WRITER);
				} else {
					sam_update_inode(bip, SAM_SYNC_ONE,
					    FALSE);
				}
			}
		}
	}
	TRACE(T_SAM_SET_SLN_EXT, SAM_ITOV(bip), bip->di.id.ino, n_chars, error);
	return (error);
}


/*
 * ----- sam_set_old_symlink - Store a SAM-QFS symbolic link name.
 * Store the symbolic link name in data blocks, if needed.
 * Used for SAM-QFS file systems using 3.5.0 and earlier format.
 */

int				/* ERRNO if error, 0 if successful */
sam_set_old_symlink(
	sam_node_t *ip,		/* Pointer to base inode */
	char *sln,		/* Pointer to the symbolic link name. */
	int n_chars,		/* Number of chars in symbolic link name */
	cred_t *credp)
{
	int error = 0;
	vnode_t *vp;

	vp = SAM_ITOV(ip);
	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)sln, n_chars, (off_t)0,
	    UIO_SYSSPACE, 0, n_chars, credp, NULL);
	(void) VOP_PUTPAGE_OS(vp, 0, 0, B_ASYNC, credp, NULL);
	return (error);
}


/*
 * ----- sam_get_old_symlink - Read a SAM-QFS symbolic link name.
 * Read the symbolic link name from data blocks.
 * Used for SAM-QFS file systems using 3.5.0 and earlier format.
 */

int				/* ERRNO if error, 0 if successful */
sam_get_old_symlink(
	sam_node_t *ip,		/* Pointer to inode */
	struct uio *uiop,	/* User I/O vector array. */
	cred_t *credp)
{
	int	error;

	/*
	 * Symlink name was stored as metadata block
	 */
	if (ip->di.status.b.offline) {
		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		if (ip->di.status.b.offline) {
			error = sam_proc_offline(ip, (offset_t)0,
			    ip->di.rm.size, NULL,
			    credp, NULL);
		} else {
			error = 0;
		}
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		if (error) {
			return (error);
		}
	}
	RW_LOCK_OS(&ip->data_rwl, RW_READER);
	RW_LOCK_OS(&ip->inode_rwl, RW_READER);

	/*
	 * Retrieve symlink name from metadata block.
	 */
	if ((error = sam_read_io(SAM_ITOV(ip), uiop, 0)) == 0) {
		if (!(ip->mp->mt.fi_mflag & MS_RDONLY)) {
			TRANS_INODE(ip->mp, ip);
			sam_mark_ino(ip, (SAM_ACCESSED));
		}
	}
	RW_UNLOCK_OS(&ip->data_rwl, RW_READER);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	return (error);
}


/*
 * ----- sam_set_hardlink_parent - Save existing parent of hardlinked inode.
 * Save existing parent of hardlinked inode in inode extension.  New parent
 * will be set by caller.
 *
 * Caller must have the inode write lock held.  This lock is not released.
 */

static int			/* ERRNO if error, 0 if successful */
sam_set_hardlink_parent(sam_node_t *bip)
{
	int error = 0, done = 0;
	int i_changed = 0, e_changed = 0;
	buf_t *bp;
	sam_id_t eid;
	struct sam_inode_ext *eip;

	ASSERT(RW_WRITE_HELD(&bip->inode_rwl));

	if (bip->di.ext_attrs & ext_hlp) {
		/*
		 * HLP extension inode(s) exist; if there's
		 * an empty HLP slot find it and use it.
		 */
		eid = bip->di.ext_id;
		while (eid.ino && !done && !error) {
			if (error = sam_read_ino(bip->mp, eid.ino, &bp,
					(struct sam_perm_inode **)&eip)) {
				done = 1;
				break;
			}

			if (EXT_HDR_ERR(eip, eid, bip)) {
				sam_req_ifsck(bip->mp, -1,
				    "sam_set_hardlink_parent: EXT_HDR",
				    &bip->di.id);
				error = ENOCSI;
				brelse(bp);
				done = 1;
				break;
			}

			if (S_ISHLP(eip->hdr.mode)) {
				int n_ids = eip->ext.hlp.n_ids;

				/*
				 * Check the number of parents purportedly
				 * present.
				 */
				if (n_ids < 0) {
					dcmn_err((CE_WARN,
					    "SAM-QFS: %s: bad count in "
					    "HLP extension (%d) [1..%d];"
					    " inode %d.%d, HLP extension %d.%d",
					    bip->mp->mt.fi_name, n_ids,
					    MAX_HLP_IDS_IN_INO,
					    bip->di.id.ino, bip->di.id.gen,
					    eid.ino, eid.gen));
					brelse(bp);
					sam_req_ifsck(bip->mp, -1,
					    "sam_set_hardlink_parent: "
					    "n_ids < 0", &bip->di.id);
					error = ENOCSI;
					break;
				}
				if (n_ids < MAX_HLP_IDS_IN_INO) {
					/*
					 * Copacetic -- found an empty slot.
					 * Tack parent id
					 * to end of hardlink parents list.
					 */
					eip->ext.hlp.ids[n_ids] =
					    bip->di.parent_id;
					eip->ext.hlp.change_time = SAM_SECOND();
					eip->ext.hlp.n_ids = ++n_ids;
					e_changed = 1;
					done = 1;
					break;
				}
				if (n_ids > MAX_HLP_IDS_IN_INO) {
					dcmn_err((CE_WARN,
					"SAM-QFS: %s: parent links (%d) "
					    "overflowed HLP extension;"
					    " inode %d.%d, HLP extension %d.%d",
					    bip->mp->mt.fi_name, n_ids,
					    bip->di.id.ino,
					    bip->di.id.gen, eid.ino, eid.gen));
					brelse(bp);
					sam_req_ifsck(bip->mp, -1,
					    "sam_set_hardlink_parent: "
					    "n_ids > MAX_HLP_IDS_IN_INO",
					    &bip->di.id);
					error = ENOCSI;
					break;
				}
			}
			eid = eip->hdr.next_id;
			brelse(bp);
		}
		if (e_changed) {
			if (TRANS_ISTRANS(bip->mp)) {
				sam_ioblk_t ioblk;

				RW_LOCK_OS(&bip->mp->mi.m_inodir->inode_rwl,
				    RW_READER);
				error = sam_map_block(bip->mp->mi.m_inodir,
				    (offset_t)SAM_ITOD(eid.ino), SAM_ISIZE,
				    SAM_READ, &ioblk, CRED());
				RW_UNLOCK_OS(&bip->mp->mi.m_inodir->inode_rwl,
				    RW_READER);
				if (!error) {
					offset_t doff;

					doff = ldbtob(fsbtodb(bip->mp,
					    ioblk.blkno)) + ioblk.pboff;
					TRANS_EXT_INODE(bip->mp, eid, doff,
					    ioblk.ord);
				} else {
					error = 0;
				}
				brelse(bp);
			} else if (SAM_SYNC_META(bip->mp)) {
				error = sam_write_ino_sector(bip->mp, bp,
				    eid.ino);
			} else {
				(void) sam_bwrite_noforcewait_dorelease(
				    bip->mp, bp);
			}
		}
		if (done || error) {
			TRACE(T_SAM_SET_HLP_EXT, SAM_ITOV(bip),
			    (int)bip->di.id.ino, eid.ino, error);
			return (error);
		}
	}

	if (!done) {
		sam_id_t org_ext = bip->di.ext_id;

		/*
		 * Either the file didn't have any HLP extensions, or there
		 * wasn't an empty slot in it/them.  Allocate a new HLP
		 * extension and save the parent link info into it.
		 */
		if ((error = sam_alloc_inode_ext(bip, S_IFHLP, 1,
		    &bip->di.ext_id))) {
			return (error);
		}
		bip->di.ext_attrs |= ext_hlp;
		i_changed++;

		eid = bip->di.ext_id;
		if ((error = sam_read_ino(bip->mp, eid.ino, &bp,
				(struct sam_perm_inode **)&eip)) == 0) {
			eip->ext.hlp.creation_time = SAM_SECOND();
			eip->ext.hlp.change_time = SAM_SECOND();
			eip->ext.hlp.ids[0] = bip->di.parent_id;
			eip->ext.hlp.n_ids = 1;

			if (TRANS_ISTRANS(bip->mp)) {
				sam_ioblk_t ioblk;

				RW_LOCK_OS(&bip->mp->mi.m_inodir->inode_rwl,
					RW_READER);
				error = sam_map_block(bip->mp->mi.m_inodir,
					(offset_t)SAM_ITOD(eid.ino),
					SAM_ISIZE, SAM_READ, &ioblk, CRED());
				RW_UNLOCK_OS(&bip->mp->mi.m_inodir->inode_rwl,
						RW_READER);
				if (!error) {
					offset_t doff;

					doff = ldbtob(fsbtodb(bip->mp,
						ioblk.blkno)) + ioblk.pboff;
					TRANS_EXT_INODE(bip->mp, eid, doff,
							ioblk.ord);
				} else {
					error = 0;
				}
				brelse(bp);
			} else if (SAM_SYNC_META(bip->mp)) {
				error = sam_write_ino_sector(bip->mp, bp,
								eid.ino);
			} else {
				(void) sam_bwrite_noforcewait_dorelease(
							bip->mp, bp);
			}
		} else {
			/*
			 * If error, restore original extension pointer here,
			 * and throw new extension away.
			 */
			bip->di.ext_id = org_ext;
			i_changed = 0;
		}
	}

	if (i_changed) {
		/*
		 * Note this so our caller flushes the base inode.
		 */
		if (!bip->di.status.b.worm_rdonly) {
			TRANS_INODE(bip->mp, bip);
			sam_mark_ino(bip, SAM_CHANGED);
		}
	}

	TRACE(T_SAM_SET_HLP_EXT, SAM_ITOV(bip),
	    (int)bip->di.id.ino, (int)eid.ino, error);
	return (error);
}


/*
 * ----- sam_check_worm_capable - test worm capability.
 *	If the file system is worm_capable and the inode being tested
 *	is a directory, return no error (0).
 *	If the file system is worm_capable and the inode being tested
 *	is not a directory, check either the first parent or all parents
 *	(depending on all_flag) and return no error if all specified are
 *	worm-capable.
 *	In all other cases, return failure code.
 */
int				/* return error number, if not worm capable */
sam_check_worm_capable(sam_node_t *ip, boolean_t all_flag)
{
	struct vfs *vfsp;
	sam_node_t *pip;
	vnode_t *vp, *pvp;
	boolean_t worm_capable, is_root_vn;
	boolean_t add_id;
	boolean_t initialize_list;
	sam_id_t id, ppid, pid, ext_id;
	struct sam_inode_ext *eip;
	struct buf *bp;
	sam_mount_t *mp;
	nlink_t nlinks, checked_size;
	sam_id_t ids_checked[MAX_HLP_IDS_IN_INO];
	int n_ids;
	int i, j;
	int start_chk_index = 0;
	int n_ids_index = 0;

	vp = SAM_ITOV(ip);

	ASSERT(rw_iswriter(&ip->inode_rwl));
	if (ip->mp->mt.fi_config & MT_ALLWORM_OPTS) {
		id = ip->di.id;
		vfsp = vp->v_vfsp;
		ppid = pid = ip->di.parent_id;
		nlinks = ip->di.nlink;
		if (ip->di.ext_attrs & ext_hlp) {
			ext_id = ip->di.ext_id;
		} else {
			ext_id.ino = 0;
		}
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (sam_get_ino(vfsp, IG_EXISTS, &pid, &pip) != 0) {
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			return (ER_NO_PARENT);
		}
		RW_LOCK_OS(&pip->inode_rwl, RW_READER);
		worm_capable = (pip->di.status.b.worm_rdonly == 1);
		pvp = SAM_ITOV(pip);
		is_root_vn = (pvp == ip->mp->mi.m_vn_root);
		ids_checked[0] = pip->di.id;
		RW_UNLOCK_OS(&pip->inode_rwl, RW_READER);
		VN_RELE(SAM_ITOV(pip));
		if (vp->v_type == VDIR) {
			/*
			 * Need to check to ensure parent is
			 * worm capable or the root of the tree.
			 */
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			if (worm_capable || is_root_vn) {
				return (0);
			} else {
				return (ER_PARENT_NOT_WORM);
			}
		}
		if (!worm_capable) {
			/*
			 * parent not worm_capable
			 */
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			return (ER_PARENT_NOT_WORM);
		}
		checked_size = 1;
		initialize_list = FALSE;

		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		mp = ip->mp;
		while (all_flag && ext_id.ino != 0) {
			/*
			 * Check all hard link parents.
			 * Any failure is a failure.
			 */

			if (sam_read_ino(mp, ext_id.ino, &bp,
					(struct sam_perm_inode **)&eip) != 0) {
				return (ER_NO_PARENT);
			}
			if (EXT_HDR_ERR(eip, ext_id, ip)) {
				sam_req_ifsck(mp, -1,
				    "sam_set_hardlink_parent: EXT_HDR",
				    &ip->di.id);
				brelse(bp);
				return (ENOCSI);
			}
			if (!S_ISHLP(eip->hdr.mode)) {
				n_ids_index = 0;
				ext_id = eip->hdr.next_id;
				brelse(bp);
				continue;
			}
			/*
			 * Collect the "like" parents into a list.
			 */
			n_ids = eip->ext.hlp.n_ids;
			if (checked_size >= MAX_HLP_IDS_IN_INO) {
				checked_size = MAX_HLP_IDS_IN_INO -
				    (n_ids - n_ids_index);
				if (checked_size > 0) {
					checked_size = 0;
				}
			} else if (initialize_list) {
				checked_size = 0;
			}
			for (i = n_ids_index; i < n_ids; i++) {
				add_id = TRUE;
				pid = eip->ext.hlp.ids[i];
				n_ids_index = i;
				for (j = 0; j < checked_size; j++) {
					if (ids_checked[j].ino == pid.ino &&
					    ids_checked[j].gen == pid.gen) {
						add_id = FALSE;
						break;
					}
				}
				if (add_id) {
					ids_checked[checked_size++] = pid;
				}
				if (checked_size >= MAX_HLP_IDS_IN_INO) {
					n_ids_index++;
					break;
				}
			}
			/*
			 * If at end of this hlp, set up for the next.
			 */
			if (n_ids == n_ids_index+1) {
				n_ids_index = 0;
				ext_id = eip->hdr.next_id;
				initialize_list = TRUE;
			} else {
				initialize_list = FALSE;
			}

			brelse(bp);

			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			for (i = start_chk_index; i < checked_size; i++) {
				pid = ids_checked[i];
				if (sam_get_ino(vfsp, IG_EXISTS, &pid,
				    &pip) != 0) {
					RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
					return (ER_NO_PARENT);
				}
				RW_LOCK_OS(&pip->inode_rwl, RW_READER);
				worm_capable =
				    (pip->di.status.b.worm_rdonly == 1);
				RW_UNLOCK_OS(&pip->inode_rwl, RW_READER);
				VN_RELE(SAM_ITOV(pip));
				if (!worm_capable) {
					RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
					return (ER_PARENT_NOT_WORM);
				}
			}
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
		/*
		 * Verify that things didn't change while unlocked.
		 */
		if (id.ino == ip->di.id.ino && id.gen == ip->di.id.gen &&
		    ppid.ino == ip->di.parent_id.ino &&
		    ppid.gen == ip->di.parent_id.gen &&
		    nlinks == ip->di.nlink) {
			return (0);
		} else {
			return (EINVAL);
		}
	} else {
		return (ER_MOUNT_NOT_WORM);
	}
}


/*
 * ----- sam_enter_dnlc -
 * Enter a name into both the DNLC cache and the directory cache.
 */
static void
sam_enter_dnlc(
	sam_node_t *pip,		/* Parent directory inode */
	sam_node_t *ip,			/* New entry inode */
	char *cp,			/* New name */
	struct sam_name *namep,		/* Name information block */
	ushort_t slot_size)		/* Space remaining in this dirent */
{
	vnode_t *pvp, *vp;

	/*
	 * Add to the DNLC cache.
	 */
	pvp = SAM_ITOV(pip);
	vp = SAM_ITOV(ip);
	dnlc_update(pvp, cp, vp);
	/*
	 * Add to the directory cache.
	 */
	sam_enter_dir_dnlc(pip, ip->di.id.ino, namep->data.slot.offset, cp,
	    slot_size);
}


/*
 * ----- sam_enter_dir_dnlc -
 * Enter a name into the directory cache only.
 */
static void
sam_enter_dir_dnlc(
	sam_node_t *pip,		/* Parent directory inode */
	sam_ino_t ino,			/* inode number */
	uint32_t slot_offset,		/* Name offset */
	char *cp,			/* New name */
	ushort_t slot_size)		/* Space remaining in this dirent */
{
	uint64_t	handle;
	dcret_t		reply;
	vnode_t		*pvp = SAM_ITOV(pip);

	/*
	 * Add an entry to the directory cache.
	 * DNOCACHE will be returned if no directory cache exists.
	 */
	handle = SAM_DIR_OFF_TO_H(ino, slot_offset);
	reply = dnlc_dir_add_entry(&pip->i_danchor, cp, handle);
	if (reply == DTOOBIG) {
		/*
		 * The cache is too large, purge it.
		 */
		TRACE(T_SAM_EDNLC_ERR, pvp, ino, strlen(cp), reply);
		dnlc_dir_purge(&pip->i_danchor);
		pip->ednlc_ft = SAM_SECOND();
		SAM_COUNT64(dnlc, ednlc_purges);
		SAM_COUNT64(dnlc, ednlc_too_big);
		TRACE(T_SAM_EDNLC_PURGE, pvp, pip->di.id.ino, 2, 0);
	} else {
		TRACE(T_SAM_EDNLC_ADD_E, pvp, (sam_tr_t)SAM_H_TO_OFF(handle),
		    (sam_tr_t)SAM_H_TO_INO(handle), reply);
		if (slot_size >= SAM_DIRLEN_MIN) {
			/*
			 * Add slot space to the directory cache.
			 */
			handle = SAM_LEN_TO_H(slot_size, slot_offset);
			reply = dnlc_dir_add_space(&pip->i_danchor,
			    slot_size, handle);
			if (reply == DTOOBIG) {
				/*
				 * The cache is too large, purge it.
				 */
				TRACE(T_SAM_EDNLC_ERR, pvp, ino, slot_size,
				    reply);
				dnlc_dir_purge(&pip->i_danchor);
				pip->ednlc_ft = SAM_SECOND();
				SAM_COUNT64(dnlc, ednlc_purges);
				SAM_COUNT64(dnlc, ednlc_too_big);
				TRACE(T_SAM_EDNLC_PURGE, pvp,
				    pip->di.id.ino, 3, 0);
			} else {
				TRACE(T_SAM_EDNLC_ADD_S, pvp,
				    (sam_tr_t)SAM_H_TO_OFF(handle),
				    (sam_tr_t)SAM_H_TO_LEN(handle), reply);
			}
		}
	}
}
