/*
 * ----- fioctl.c - Process the sam ioctl file functions.
 *
 *	Processes the ioctl "f" commands on the SAM-QFS File System.
 *	Called when a vnode ioctl "f" command is issued on an opened file.
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

#pragma ident "$Revision: 1.95 $"

#include <sam/osversion.h>

#define	_SYSCALL32		1
#define	_SYSCALL32_IMPL	1

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/dirent.h>
#include <sys/file.h>
#include <sys/lockfs.h>

#include <vm/page.h>

/* ----- SAMFS Includes */

#include "sam/param.h"
#include "sam/types.h"
#include "sam/fioctl.h"
#include "sam/resource.h"
#include "pub/rminfo.h"

#include "ino.h"
#include "inode.h"
#include "mount.h"
#include "ino_ext.h"
#include "rwio.h"
#include "extern.h"
#include "quota.h"
#include "syslogerr.h"
#include "trace.h"

#define	TAR_RECORDSIZE	512

static int sam_restore_a_file(sam_node_t *pip,
	struct sam_ioctl_samrestore *resp, cred_t *credp);
static int sam_restore_inode(sam_node_t *ip, struct sam_ioctl_idrestore *pp,
	struct sam_perm_inode *inode, cred_t *credp);
static int sam_set_rm_info_file(sam_node_t *bip, sam_resource_file_t *rfp,
	int n_vsns);
static int sam_set_old_rm_info_file(sam_node_t *bip, sam_resource_file_t *rfp,
	int n_vsns);
static int sam_stage_write(sam_node_t *ip, sam_ioctl_swrite_t *swp,
	int	*rvp, cred_t *credp);


/*
 * ----- sam_ioctl_sam_cmd - ioctl file command.
 *
 *	Called when the user issues an file "f" ioctl command for
 *	an opened file on the SAM-FS file system.
 */

/* ARGSUSED3 */
int				/* ERRNO if error, 0 if successful. */
sam_ioctl_sam_cmd(
	sam_node_t *ip,		/* Pointer to inode. */
	int	cmd,		/* Command number. */
	int	*arg,		/* Pointer to arguments. */
	int flag,		/* Datamodel */
	int	*rvp,		/* Returned value pointer */
	cred_t *credp)		/* Credentials pointer.	*/
{
	int	error = 0;
	vnode_t	*vp;

	switch (cmd) {

	case C_GETDENTS: {		/* Read full SAM-FS directory */
		sam_ioctl_getdents_t *gdp;
		uio_t uio;
		iovec_t iov;
		void *dir;

		gdp = (sam_ioctl_getdents_t *)(void *)arg;
		if (gdp->size < sizeof (struct sam_dirent)) {
			error = EINVAL;
			break;
		}
		vp = SAM_ITOV(ip);
		if (vp->v_type != VDIR) {
			error = ENOTDIR;
			break;
		}
		dir = (void *)gdp->dir.p32;
		if (curproc->p_model != DATAMODEL_ILP32) {
			dir = (void *)gdp->dir.p64;
		}
		iov.iov_base = (char *)dir;
		iov.iov_len	= gdp->size;
		uio.uio_iov	= (iovec_t *)&iov;
		uio.uio_iovcnt = 1;
		uio.uio_loffset = gdp->offset;
		uio.uio_segflg = UIO_USERSPACE;
		uio.uio_resid = gdp->size;
		uio.uio_fmode = 0;
		RW_LOCK_OS(&ip->data_rwl, RW_READER);
		error = sam_getdents(ip, &uio, credp, &gdp->eof, FMT_SAM);
		RW_UNLOCK_OS(&ip->data_rwl, RW_READER);

		if (error == 0) {
			*rvp = gdp->size - uio.uio_resid;
			gdp->offset = uio.uio_loffset;
		}
		}
		break;

	case C_SWRITE: {	/* Stage write using daemon I/O */
		sam_ioctl_swrite_t	*swp;

		swp = (sam_ioctl_swrite_t *)(void *)arg;
		error = sam_stage_write(ip, swp, rvp, credp);
		}
		break;

	case C_STSIZE: {	/* Update stage file size for mmap stage */
		sam_ioctl_stsize_t	*ssp;

		if ((ip->stage_pid > 0) || (ip->stage_pid != SAM_CUR_PID)) {
			error = EPERM;
			break;
		}
		ssp = (sam_ioctl_stsize_t *)(void *)arg;
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		ip->size = ssp->size;
		ip->stage_size = ssp->size & MAXBMASK;
		mutex_enter(&ip->rm_mutex);
		if (ip->rm_wait) {
			cv_broadcast(&ip->rm_cv);
		}
		mutex_exit(&ip->rm_mutex);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
		break;

	case C_IDRESTORE: {		/* Restore inode */
		struct sam_ioctl_idrestore *pp;
		struct sam_perm_inode inode;
		void *dp;

		pp = (struct sam_ioctl_idrestore *)(void *)arg;
		dp = (void *)pp->dp.p32;
		if (curproc->p_model != DATAMODEL_ILP32) {
			dp = (void *)pp->dp.p64;
		}
		if (copyin((caddr_t)dp, (caddr_t)&inode,
					sizeof (struct sam_perm_inode))) {
			return (EFAULT);
		}
		if (!(SAM_CHECK_INODE_VERSION(inode.di.version))) {
			return (EINVAL);
		}
		if (SAM_IS_SHARED_CLIENT(ip->mp)) {
			return (ENOTSUP);
		}
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (S_ISSEGI(&ip->di) && S_ISSEGS(&inode.di)) {
			sam_node_t *sip;

			if ((error = sam_get_segment_ino(ip,
			    inode.di.rm.info.dk.seg.ord, &sip)) == 0) {
				RW_LOCK_OS(&sip->inode_rwl, RW_WRITER);
				error = sam_restore_inode(sip, pp, &inode,
				    credp);
				RW_UNLOCK_OS(&sip->inode_rwl, RW_WRITER);
				VN_RELE(SAM_ITOV(sip));
			}
		} else {
			error = sam_restore_inode(ip, pp, &inode, credp);
		}
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
		break;

	case C_UNLOAD: {	/* Unload removable media file */
		sam_ioctl_rmunload_t *pp;

		pp = (sam_ioctl_rmunload_t *)(void *)arg;
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (ip->rm_pid && (ip->rm_pid == curproc->p_pid)) {
			if (ip->rdev) {
				error = sam_unload_rm(ip, FWRITE,
				    (pp->flags & UNLOAD_EOX) != 0,
				    (pp->flags & UNLOAD_WTM) != 0, credp);
			} else  {
				while (ip->flags.b.unloading) {
					if ((error = sam_wait_rm(ip,
					    SAM_RM_OPENED))) {
						break;
					}
				}
				if (error == 0)  error = ip->rm_err;
			}
		} else {
			error = EPERM;
		}
		pp->position = ip->di.rm.info.rm.position;
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
		break;

	case C_GETRMINFO: {		/* Get removable media information */
		sam_ioctl_getrminfo_t *pp;
		void *buf;

		pp = (sam_ioctl_getrminfo_t *)(void *)arg;
		buf = (void *)pp->buf.p32;
		if (curproc->p_model != DATAMODEL_ILP32) {
			buf = (void *)pp->buf.p64;
		}
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (ip->rm_pid && (ip->rm_pid == curproc->p_pid)) {
			error = sam_readrm(ip, (char *)buf, pp->bufsize);
		} else {
			error = EPERM;
		}
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
		break;

	case C_RMPOSITION: {	/* Position removable media file */
		sam_ioctl_rmposition_t *pp;

		pp = (sam_ioctl_rmposition_t *)(void *)arg;
		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		if (ip->rm_pid && (ip->rm_pid == curproc->p_pid)) {
			if (ip->rdev) {
				error = sam_position_rm(ip, pp->setpos, credp);
			} else {
				while (ip->flags.b.positioning) {
					if ((error = sam_wait_rm(ip,
					    SAM_RM_OPENED))) {
						break;
					}
				}
				if (error == 0) error = ip->rm_err;
			}
		} else {
			error = EPERM;
		}
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		}
		break;

	case C_SAMRESTORE: {			/* Read inode */
		struct sam_ioctl_samrestore *resp;

		resp = (struct sam_ioctl_samrestore *)(void *)arg;
		vp = SAM_ITOV(ip);
		if (vp->v_type != VDIR) {
			error = ENOTDIR;
		} else if (SAM_IS_SHARED_CLIENT(ip->mp)) {
			error = ENOTSUP;
		} else {
			error = sam_restore_a_file(ip, resp, credp);
		}
		}
		break;

#ifdef	DEBUG
	case C_FLUSHINVAL:			/* Flush and invalidate pages */
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		error = sam_flush_pages(ip, B_INVAL);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		break;
#endif	/* DEBUG */

	default:
		error = ENOTTY;
		break;

	}
	return (error);
}


/*
 * ----- sam_restore_a_file - samfsrestore a file.
 *
 * Called by samfsrestore to restore a file to the specified directory.
 * The named file may need to be removed and then replaced, if specified.
 * This allows samfsrestore to restore on-disk contents without needing
 * to open the file.
 */

static int					/* ERRNO, else 0 success. */
sam_restore_a_file(
	sam_node_t *pip,			/* directory inode */
	struct sam_ioctl_samrestore *resp,	/* Pointer to arguments */
	cred_t *credp)				/* user credentials */
{
	struct sam_perm_inode *perm_ino;
	struct sam_name name;
	int error;
	int copy, multi_vol;
	struct vnode *pvp;
	sam_id_t id;
	sam_node_t *ip;
	void *ptr;
	struct buf *bp;
	struct full_dirent {
		struct sam_dirent dirnt;
		char rest_of_name[MAXNAMLEN];
	} *dirent;
	struct sam_dirent *dirp;
	struct sam_vsn_section *vsnp;
	char *name_str;

	/*
	 * Allocate large buffers via kmem to avoid stack overflow.
	 */
	perm_ino = kmem_alloc(sizeof (*perm_ino), KM_SLEEP);
	dirent = kmem_alloc(sizeof (*dirent), KM_SLEEP);
	dirp = &dirent->dirnt;
	name_str = (char *)dirp->d_name;

	if (resp == NULL) {
		error = EFAULT;
		goto out;
	}

	/*
	 * Get pointer to name.
	 */
	ptr = (void *)resp->name.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		ptr = (void *)resp->name.p64;
	}
	if (ptr == NULL) {
		error = EFAULT;
		goto out;
	}
	if (copyin((caddr_t)ptr, (caddr_t)dirent,
		sizeof (struct sam_dirent)) != 0) {
		error = EFAULT;
		goto out;
	}
	pvp = SAM_ITOV(pip);
	if (dirp->d_namlen > 4) {
		if (error = copyin((caddr_t)ptr, (caddr_t)dirent,
		    sizeof (struct sam_dirent) + MAXNAMLEN + 1 -
		    sizeof (dirent->dirnt.d_name))) {
			error = EFAULT;
			goto out;
		}
	}
	/*
	 * Note that this check must be made AFTER the final copyin, above.
	 * A malicious user process could set up a race between ioctl() and
	 * a thread which is changing d_namlen, so a check after the first
	 * copyin is not sufficient; d_namlen will be overwritten by the
	 * second copyin.
	 */
	if (dirp->d_namlen > MAXNAMLEN) {
		error = ENAMETOOLONG;
		goto out;
	}
	name_str[dirp->d_namlen] = '\0';

	TRACES(T_SAM_RESTOR_FILE, pvp, name_str);
	/*
	 * Copy in perm. inode.
	 */
	ptr = (void *)resp->dp.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		ptr = (void *)resp->dp.p64;
	}
	if (ptr == NULL) {
		error = EFAULT;
		goto out;
	}
	error = copyin((caddr_t)ptr, (caddr_t)perm_ino,
	    sizeof (struct sam_perm_inode));
	if (error) {
		error = EFAULT;
		goto out;
	}

	/*
	 * Determine if multivolume inode extension(s) exist.
	 */
	multi_vol = 0;
	if (perm_ino->di.version >= SAM_INODE_VERS_2) { /* Vers 2 or 3 */
		if (perm_ino->di.ext_attrs & ext_mva) {
			multi_vol++;
		}
	} else if (perm_ino->di.version == SAM_INODE_VERS_1) {
		/* Prev version */
		sam_perm_inode_v1_t *perm_ino_v1 =
		    (sam_perm_inode_v1_t *)perm_ino;
		for (copy = 0; copy < MAX_ARCHIVE; copy++) {
			if ((perm_ino->di.arch_status & (1<<copy)) &&
			    (perm_ino_v1->aid[copy].ino)) {
				multi_vol++;
			}
		}
	} else {
		error = ENOTSUP;
		goto out;
	}

	/*
	 * Check the directory for the name. If not found, restore this name
	 * and return the ID. If found, return EEXIST.
	 */
	RW_LOCK_OS(&pip->data_rwl, RW_WRITER);
	name.operation = SAM_RESTORE;
	error = sam_lookup_name(pip, name_str, &ip, &name, credp);
	if (error == ENOENT) {			/* file not found, make one. */
		if ((error = sam_restore_name(pip, name_str, &name, perm_ino,
		    &id, credp)) == 0) {
			if ((copyout((caddr_t)&id,
			    (caddr_t)&((sam_perm_inode_t *)ptr)->di.id,
			    sizeof (sam_id_t)))) {
				error = EFAULT;
			}
		}
	} else if (error == 0) {
		error = EEXIST;
		VN_RELE(SAM_ITOV(ip));
	}

	if (error == 0) {
		/*
		 * Symbolic link.
		 */
		if (S_ISLNK(perm_ino->di.mode)) {
			error = sam_get_ino(pvp->v_vfsp, IG_EXISTS, &id, &ip);
			if (error == 0) {
				char *link;
				size_t llen;

				/*
				 * Call dnlc_update() to increment the v_count
				 * to avoid the sync may forced by VN_RELE()
				 * below.  Need to replace by VN_HOLD() later.
				 */
				dnlc_update(pvp, name_str, SAM_ITOV(ip));
				link = kmem_alloc(MAXPATHLEN+1, KM_SLEEP);

				ptr = (void *)resp->lp.p32;
				if (curproc->p_model != DATAMODEL_ILP32) {
					ptr = (void *)resp->lp.p64;
				}
				if (copyinstr((caddr_t)ptr, (caddr_t)link,
				    (MAXPATHLEN+1),
				    &llen)) {
					error = EFAULT;
				} else {
					llen--;	/* don't count null byte */
					if (pip->di.version >=
					    SAM_INODE_VERS_2) {
						RW_LOCK_OS(&ip->inode_rwl,
						    RW_WRITER);
						error = sam_set_symlink(NULL,
						    ip, link, llen, credp);
						RW_UNLOCK_OS(&ip->inode_rwl,
						    RW_WRITER);
					} else if (pip->di.version ==
					    SAM_INODE_VERS_1) {
						error = sam_set_old_symlink(
						    ip, link, llen, credp);
					} else {
						error = ENOCSI;
					}
				}
				kmem_free(link, MAXPATHLEN+1);
				VN_RELE(SAM_ITOV(ip));	/* Decrement v_count */
			}
		} else if (S_ISREG(perm_ino->di.mode) &&
		    perm_ino->di.status.b.worm_rdonly &&
		    (perm_ino->di.version >= SAM_INODE_VERSION) &&
		    (perm_ino->di2.p2flags & P2FLAGS_WORM_V2)) {
			/*
			 * Check to see if we need to update the superblock
			 * for worm'd regular file.
			 */
			struct sam_sbinfo *sblk = &pip->mp->mi.m_sbp->info.sb;
			if ((sblk->opt_mask & SBLK_OPTV1_CONV_WORMV2) == 0) {
				sblk->opt_mask |= SBLK_OPTV1_CONV_WORMV2;
				if (!sam_update_all_sblks(pip->mp)) {
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: WORM can't update"
					    " superblock via a samfsrestore"
					    " operation",
					    pip->mp->mt.fi_name);
				}
			}
		}
	}
	if (error == 0 && multi_vol) {
		error = sam_get_ino(pvp->v_vfsp, IG_EXISTS, &id, &ip);
		if (error == 0) {
			/*
			 * Call dnlc_update() to increment the v_count to avoid
			 * the sync may forced by VN_RELE() below.
			 * Need to replace by VN_HOLD() later.
			 */
			dnlc_update(pvp, name_str, SAM_ITOV(ip));
		}
	}
	RW_UNLOCK_OS(&pip->data_rwl, RW_WRITER);

	if (error == 0 && multi_vol) {
		struct sam_perm_inode *permip;

		/* Process the multivolume inode extension(s) */
		ptr = (void *)resp->vp.p32;
		if (curproc->p_model != DATAMODEL_ILP32) {
			ptr = (void *)resp->vp.p64;
		}
		vsnp = (struct sam_vsn_section *)(void *)ptr;
		if (error = sam_read_ino(ip->mp, ip->di.id.ino, &bp, &permip)) {
			VN_RELE(SAM_ITOV(ip));
			goto out;
		}
		for (copy = 0; copy < MAX_ARCHIVE; copy++) {
			sam_id_t id;
			int n_vsns;

			if ((permip->di.arch_status & (1<<copy)) == 0) {
				continue;
			}
			n_vsns = permip->ar.image[copy].n_vsns;
			if (n_vsns <= 1) {
				continue;
			}
			if (permip->di.version >= SAM_INODE_VERS_2) {
				bdwrite(bp);
				id = ip->di.ext_id;
				if (error = sam_set_multivolume(ip, &vsnp,
				    copy, n_vsns, &id)) {
					VN_RELE(SAM_ITOV(ip));
					goto out;
				}
				if (error = sam_read_ino(ip->mp,
				    ip->di.id.ino, &bp, &permip)) {
					VN_RELE(SAM_ITOV(ip));
					goto out;
				}
				permip->di.ext_id = ip->di.ext_id = id;
				permip->di.ext_attrs = ip->di.ext_attrs |=
				    ext_mva;

			} else if (permip->di.version == SAM_INODE_VERS_1) {
				sam_perm_inode_v1_t *permip_v1 =
				    (sam_perm_inode_v1_t *)permip;
				/*
				 * Previous inode version (1)
				 */
				bdwrite(bp);
				id.ino = id.gen = 0;
				if (error = sam_set_multivolume(ip, &vsnp,
				    copy, n_vsns, &id)) {
					VN_RELE(SAM_ITOV(ip));
					goto out;
				}
				if (error = sam_read_ino(ip->mp,
				    ip->di.id.ino, &bp, &permip)) {
					VN_RELE(SAM_ITOV(ip));
					goto out;
				}
				permip_v1->aid[copy] = id;
			}
		}
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		ip->flags.b.changed = 1;
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		bdwrite(bp);
		VN_RELE(SAM_ITOV(ip));
	}
out:
	kmem_free(perm_ino, sizeof (*perm_ino));
	kmem_free(dirent, sizeof (*dirent));

	return (error);
}


/*
 * ----- sam_restore_inode - samfsrestore inode.
 *
 * Called by samfsrestore to restore information in an existing inode.
 * Note: inode_rwl WRITER lock set on entry and exit.
 */

static int				/* ERRNO, else 0 if successful. */
sam_restore_inode(
	sam_node_t *ip,			/* Pointer to inode. */
	struct sam_ioctl_idrestore *pp,	/* Pointer to arguments */
	struct sam_perm_inode *inode,	/* Pointer to permanent inode */
	cred_t *credp)			/* User credentials */
{
	buf_t *bp;
	struct sam_perm_inode *permip;
	struct sam_disk_inode *dip, *pdip;
	struct sam_disk_inode_part2 *dip2, *pdip2;
	int copy;
	int error;
	int i;
	sam_id_t oid, poid;
	sam_time_t otime;
	offset_t old_size;
	int old_on_large;
	void *ptr;
	struct sam_vsn_section *vsnp;
	int n_vsns;
	sam_mount_t	*mp;
	sam_resource_file_t *rfp;
	int update_all_sblks = 0;
	struct sam_sbinfo *sblk = &ip->mp->mi.m_sbp->info.sb;

	/*
	 *	Remove quota counts for current inode.
	 */
	mp = ip->mp;
	sam_quota_bret(mp, ip, D2QBLKS(ip->di.blocks), TOTBLKS(ip));
	sam_quota_fret(mp, ip);
	sam_quota_inode_fini(ip);

	old_size = ip->di.rm.size;
	if (S_ISREG(inode->di.mode) &&
	    ip->di.rm.size > 0 &&
	    ip->di.rm.size <= inode->di.rm.size) {
		old_on_large = ip->di.status.b.on_large;
	}

	/*
	 * Replace fields in the argument inode from the incore inode
	 * Save old inode.gen for possible restore event.
	 */
	oid = inode->di.id;
	poid = inode->di.parent_id;
	otime = inode->di.modify_time.tv_sec;
	inode->di.version = ip->di.version;
	inode->di.id = ip->di.id;
	inode->di.parent_id = ip->di.parent_id;
	/*
	 * Free any pre-existing ACLs or ACLs applied as
	 * default ACLs from the parent to preserve the
	 * ACL settings as dumped.
	 */
	if (ip->di.ext_attrs & ext_acl) {
		sam_free_inode_ext(ip, S_IFACL, SAM_ALL_COPIES, &ip->di.ext_id);
		ip->di.ext_attrs &= ~ext_acl;
	}
	inode->di.ext_id.ino = 0;
	inode->di.ext_id.gen = 0;
	inode->di.ext_attrs = 0;
	inode->di.status.b.acl = 0;
	inode->di.status.b.dfacl = 0;
	inode->di.nlink = ip->di.nlink;
	inode->di.status.b.on_large = ip->di.status.b.on_large;
	if (S_ISREG(inode->di.mode) && inode->di.rm.size > SM_OFF(ip->mp, DD)) {
		inode->di.status.b.on_large = 1;
	}
	inode->di.status.b.meta = ip->di.status.b.meta;
	inode->di.stripe = ip->di.stripe;
	inode->di.status.b.stripe_group = ip->di.status.b.stripe_group;
	inode->di.stripe_group = ip->di.stripe_group;
	if (ip->di.blocks > 0) {
		inode->di.unit = ip->di.unit;
		inode->di.stride = ip->di.stride;
	} else {
		sam_set_unit(ip->mp, &inode->di);
	}

	/*
	 * Symbolic link.
	 */
	if (S_ISLNK(inode->di.mode)) {
		char *link;
		size_t llen;

		link = kmem_alloc(MAXPATHLEN+1, KM_SLEEP);
		error = 0;

		ptr = (void *)pp->lp.p32;
		if (curproc->p_model != DATAMODEL_ILP32) {
			ptr = (void *)pp->lp.p64;
		}
		if (copyinstr((caddr_t)ptr, (caddr_t)link,
		    (MAXPATHLEN+1), &llen)) {
			error = EFAULT;
		} else {
			llen--;		/* don't count null byte */
			if (ip->di.version >= SAM_INODE_VERS_2) {

				/* Symlink data stored as inode extensions */
				error = sam_set_symlink(NULL, ip,
				    (char *)link, llen, credp);
				if (error == 0) {
					inode->di.ext_id = ip->di.ext_id;
					inode->di.ext_attrs = ip->di.ext_attrs;
					inode->di.psize.symlink =
					    ip->di.psize.symlink = llen;
					inode->di.rm.size = ip->di.rm.size = 0;
					inode->di.unit = 0;
				}
			} else if (ip->di.version == SAM_INODE_VERS_1) {

				/* Symlink data stored as data blocks */
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				error = sam_set_old_symlink(ip, (char *)link,
				    llen, credp);
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				if (error == 0) {
					inode->di.rm.size = ip->di.rm.size =
					    llen;
					inode->di.psize.symlink =
					    ip->di.psize.symlink = 0;
				}
			}
			SAM_ITOV(ip)->v_type = VLNK; /* set vnode as a link */
		}
		kmem_free(link, MAXPATHLEN+1);
		if (error) {
			return (error);
		}
	}

	/*
	 * Removable media (request) file.
	 */
	if (S_ISREQ(inode->di.mode)) {
		ptr = (void *)pp->rp.p32;
		if (curproc->p_model != DATAMODEL_ILP32) {
			ptr = (void *)pp->rp.p64;
		}
		rfp = (sam_resource_file_t *)ptr;
		if (copyin((caddr_t)&rfp->n_vsns,
		    (caddr_t)&n_vsns, sizeof (int))) {
			return (EFAULT);
		}
		if ((n_vsns == 0) || (n_vsns > MAX_VOLUMES)) {
			return (EINVAL);
		}

		if (ip->di.version >= SAM_INODE_VERS_2) { /* Vers 2 or 3 */

			/* Removable media info stored as inode extensions */
			if (error = sam_set_rm_info_file(ip, rfp, n_vsns)) {
				return (error);
			}
			inode->di.ext_id = ip->di.ext_id;
			inode->di.ext_attrs = ip->di.ext_attrs;
			inode->di.unit = 0;
		} else if (ip->di.version == SAM_INODE_VERS_1) {
			/* Prev version */

			/* Removable media info stored as data blocks */
			error = sam_set_old_rm_info_file(ip, rfp, n_vsns);
		}
		inode->di.psize.rmfile =
		    ip->di.psize.rmfile = SAM_RESOURCE_SIZE(n_vsns);
	}

	/* Pick up fields that might have changed */
	inode->di.blocks = ip->di.blocks;
	for (i = (NOEXT - 1); i >= 0; i--) {
		inode->di.extent[i] = ip->di.extent[i];
		inode->di.extent_ord[i] = ip->di.extent_ord[i];
	}

	if (error = sam_read_ino(ip->mp, ip->di.id.ino, &bp, &permip)) {
		return (error);
	}

	/* Replace the permanent inode from the argument inode */
	if (S_ISDIR(inode->di.mode) || S_ISSEGI(&inode->di)) {
		ptr = (void *)pp->dp.p32;
		if (curproc->p_model != DATAMODEL_ILP32) {
			ptr = (void *)pp->dp.p64;
		}
		if ((copyout((caddr_t)&inode->di.id, (caddr_t)
		    &((struct sam_perm_inode *)ptr)->di.id,
		    sizeof (sam_id_t)))) {
			brelse(bp);
			return (EFAULT);
		}
		if (S_ISDIR(inode->di.mode)) {
			inode->di.rm.size = ip->di.rm.size;
		}

		if (S_ISSEGI(&inode->di)) {
			/* Segment data inodes may have been restored. */
			inode->di.rm.info.dk.seg.fsize =
			    ip->di.rm.info.dk.seg.fsize;
		}

		/*
		 * Do not restore archive copies for directories or segment
		 * index.
		 */
		inode->di.arch_status = 0;

		dip = &permip->di;
		pdip = &inode->di;
		*dip = *pdip;
		/*
		 * If we're restoring a WORM'd directory and the
		 * version is at least 2, copy the retention period
		 * start time and duration from the recovered
		 * (argument) inode.  Set the WORM V2 conversion
		 * flag so we do this only once.
		 */
		if (inode->di.status.b.worm_rdonly &&
		    S_ISDIR(inode->di.mode) &&
		    (inode->di.version >= SAM_INODE_VERS_2) &&
		    (permip->di.version >= SAM_INODE_VERS_2) &&
		    ((permip->di2.p2flags & P2FLAGS_WORM_V2) == 0)) {
			permip->di2.rperiod_start_time =
			    inode->di2.rperiod_start_time;
			permip->di2.rperiod_duration =
			    inode->di2.rperiod_duration;
			permip->di2.p2flags |= P2FLAGS_WORM_V2;
		}
	} else {
		/*
		 * Copy the entire recovered (argument) inode
		 * when dealing with a file.  This includes
		 * part 2 of the disk inode so we don't have
		 * to explicitly copy WORM information.  We
		 * still need to set the conversion flag and
		 * because it's a file set a flag to indicate
		 * the superblock may need to be updated.
		 */
		*permip = *inode;
		if (inode->di.status.b.worm_rdonly &&
		    (inode->di.version >= SAM_INODE_VERS_2) &&
		    (permip->di.version >= SAM_INODE_VERS_2) &&
		    ((permip->di2.p2flags & P2FLAGS_WORM_V2) == 0)) {
			permip->di2.p2flags |= P2FLAGS_WORM_V2;
			update_all_sblks = 1;
		}
	}

	permip->di.status.b.archdone = 0;
	permip->di.status.b.pextents = 0;	/* No partial extents */

	if ((S_ISREG(inode->di.mode) && !S_ISSEGI(&inode->di)) &&
	    (permip->di.rm.size != 0)) {
		if (permip->di.status.b.damaged) {
			permip->di.status.b.offline = 1;
		} else if (permip->di.rm.size != old_size) {
			/*
			 * file data at least partly offline, maybe partial
			 * restored.
			 */
			permip->di.status.b.offline = 1;
			if (old_size > 0 && old_size == (offset_t)
			    (permip->di.psize.partial * SAM_DEV_BSIZE)) {
				permip->di.status.b.on_large = old_on_large;
				permip->di.rm.media = DT_DISK;
				permip->di.status.b.pextents = 1;
			}
		} else {
			/*
			 *	all file data has been restored.
			 */
			permip->di.status.b.offline = 0;
			permip->di.status.b.on_large = old_on_large;
			permip->di.rm.media = DT_DISK;
			permip->di.status.b.pextents = 1;
		}
	}

	/* Replace the incore inode from the permanent inode */
	dip = &ip->di;
	pdip = &permip->di;
	*dip = *pdip;

	/* Replace the incore inode from the permanent inode part 2 */
	if ((ip->di.version >= SAM_INODE_VERS_2) &&
	    (permip->di.version >= SAM_INODE_VERS_2)) {
		dip2 = &ip->di2;
		pdip2 = &permip->di2;
		*dip2 = *pdip2;
	}

	/* Process the multivolume inode extension(s) */
	ptr = (void *)pp->vp.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		ptr = (void *)pp->vp.p64;
	}
	vsnp = (struct sam_vsn_section *)ptr;
	for (copy = 0; copy < MAX_ARCHIVE; copy++) {
		sam_id_t id;

		if ((ip->di.arch_status & (1<<copy)) == 0) {
			continue;
		}
		n_vsns = permip->ar.image[copy].n_vsns;
		if (n_vsns <= 1) {
			continue;
		}
		if (ip->di.version >= SAM_INODE_VERS_2) { /* Vers 2 or 3 */

			/*
			 * Store mva info as inode extensions linked thru ext_id
			 * field
			 */
			bdwrite(bp);
			id = ip->di.ext_id;
			if (error = sam_set_multivolume(ip, &vsnp, copy,
			    n_vsns, &id)) {
				return (error);
			}
			ip->di.ext_id = id;
			ip->di.ext_attrs |= ext_mva;
			if (error = sam_read_ino(ip->mp, ip->di.id.ino,
			    &bp, &permip)) {
				return (error);
			}
			permip->di.ext_id = id;
			permip->di.ext_attrs |= ext_mva;

		} else if (ip->di.version == SAM_INODE_VERS_1) {
			/* Prev version */

			/*
			 * Store mva info as old format ids in the inode aid
			 * table
			 */
			bdwrite(bp);
			id.ino = id.gen = 0;
			if (error = sam_set_multivolume(ip, &vsnp, copy,
			    n_vsns, &id)) {
				return (error);
			}
			if (error = sam_read_ino(ip->mp, ip->di.id.ino,
			    &bp, &permip)) {
				return (error);
			}
			((sam_perm_inode_v1_t *)permip)->aid[copy] = id;
		}
	}
	ip->flags.b.changed = 1;

	/*
	 * Send restore event with old inode entry and new inode entry
	 */
	if (mp->ms.m_fsev_buf) {
		struct sam_disk_inode di;

		di.id = oid;
		di.parent_id = poid;
		sam_send_event(mp, &di, ev_restore, 1, otime);
		sam_send_event(mp, &ip->di, ev_restore, 2,
		    ip->di.modify_time.tv_sec);
	}
	bdwrite(bp);

	/*
	 *	Restore quota counts for inode.
	 */
	(void) sam_quota_balloc(mp, ip, D2QBLKS(ip->di.blocks),
	    TOTBLKS(ip), credp);
	(void) sam_quota_falloc(mp, ip->di.uid, ip->di.gid,
	    ip->di.admin_id, credp);

	/*
	 *  Update superblocks if WORM file restored.
	 */
	if ((update_all_sblks) &&
	    ((sblk->opt_mask & SBLK_OPTV1_CONV_WORMV2) == 0)) {

		sblk->opt_mask |= SBLK_OPTV1_CONV_WORMV2;
		if (!sam_update_all_sblks(ip->mp)) {
			cmn_err(CE_WARN, "SAM-QFS: %s: WORM can't update"
			    " superblock via a samfsrestore operation",
			    ip->mp->mt.fi_name);
		}
	}

	/*
	 * If shared file system, cancel lease for files that were opened
	 * regular, but were changed to a symlink or removable media file.
	 */
	if (SAM_IS_SHARED_FS(ip->mp) &&
	    (S_ISREQ(inode->di.mode) || S_ISLNK(inode->di.mode))) {
		error = sam_proc_rm_lease(ip, CL_CLOSE, RW_WRITER);
		if (error == 0) {
			ip->flags.b.staging = 0;
			sam_client_remove_all_leases(ip);
		}
	}
	return (error);
}


/*
 * ----- sam_set_rm_info_file - save the removable media info for rmedia inode.
 *
 * Save the user removable media info in multiple inode extensions, if needed.
 */

static int				/* ERRNO, else 0 if successful. */
sam_set_rm_info_file(
	sam_node_t *bip,		/* Pointer to base inode. */
	sam_resource_file_t *rfp,	/* user removable media structure */
	int n_vsns)			/* Number of vsns in removable */
					/*   media structure. */
{
	int error = 0;
	int vsns;
	int size;
	int n_exts;
	buf_t *bp;
	sam_id_t eid;
	struct sam_inode_ext *eip;
	int ino_size;
	int ino_vsns;


	/* Allocate enough inode extensions to hold the removable media info */
	n_exts = 1 + howmany(n_vsns - 1, MAX_VSNS_IN_INO);
	if ((error = sam_alloc_inode_ext(bip, S_IFRFA, n_exts,
						&bip->di.ext_id)) == 0) {
		vsns = size = 0;

		eid = bip->di.ext_id;
		while (eid.ino && (size < SAM_RESOURCE_SIZE(n_vsns))) {
			if (error = sam_read_ino(bip->mp, eid.ino, &bp,
					(struct sam_perm_inode **)&eip)) {
				break;
			}
			if (EXT_HDR_ERR(eip, eid, bip)) {
				sam_req_ifsck(bip->mp, -1,
					"sam_set_rm_info_file: EXT_HDR",
					&bip->di.id);
				brelse(bp);
				error = ENOCSI;
				break;
			}
			if (S_ISRFA(eip->hdr.mode)) {
				if (EXT_1ST_ORD(eip)) {
					struct sam_rfa_inode *rfa;

					/*
					 * Resource info and vsn[0] go in first
					 * inode ext
					 */
					rfa = &eip->ext.rfa;
					ino_size  =
						sizeof (sam_resource_file_t);
					if (copyin((caddr_t)rfp,
						(caddr_t)&rfa->info,
						ino_size)) {
						brelse(bp);
						error = EFAULT;
						break;
					}
					vsns = 1;
				} else {
					struct sam_rfv_inode *rfv;

					/*
					 * Rest of vsn list goes in following
					 * inode exts
					 */
					rfv = &eip->ext.rfv;
					ino_vsns = MIN((n_vsns - vsns),
							MAX_VSNS_IN_INO);
					/* ord of 1st vsn in ext */
					rfv->ord = vsns;
					rfv->n_vsns = ino_vsns;
					ino_size =
					    sizeof (struct sam_vsn_section) *
					    ino_vsns;
					if (ino_size > 0) {
						if (copyin((caddr_t)rfp,
						(caddr_t)&rfv->vsns.section[
							0],
						    ino_size)) {
							brelse(bp);
							error = EFAULT;
							break;
						}
					}
					vsns += ino_vsns;
				}
				rfp = (sam_resource_file_t *)(void *)
						((caddr_t)rfp + ino_size);
				size += ino_size;

				eid = eip->hdr.next_id;
				bdwrite(bp);
			} else {
				brelse(bp);
				break;
			}
		}
		if (error) {
			sam_free_inode_ext(bip, S_IFRFA, SAM_ALL_COPIES,
							&bip->di.ext_id);
		} else {
			if ((size != SAM_RESOURCE_SIZE(n_vsns)) ||
			    (vsns != n_vsns)) {
				sam_req_ifsck(bip->mp, -1,
					"sam_set_rm_info_file: n_vsns",
					&bip->di.id);
				error = ENOCSI;
			} else {
				bip->di.ext_attrs |= ext_rfa;
				bip->di.psize.rmfile = size;
			}
		}
	}

	TRACE(T_SAM_SET_RFA_EXT, SAM_ITOV(bip), size, vsns, error);
	return (error);
}

/*
 * ----- sam_set_old_rm_info_file - Store removable media info.
 *
 * Store the removable media info in data blocks, if needed.
 * Used for SAM-FS file systems using 3.5.0 and earlier format.
 */

static int				/* ERRNO if error, 0 if successful */
sam_set_old_rm_info_file(
	sam_node_t *bip,		/* Pointer to base inode. */
	sam_resource_file_t *rfp,	/* user removable media structure */
	int n_vsns)			/* Number of vsns in removable */
					/*   media structure. */
{
	int error = 0;
	int rm_size;
	daddr_t blkno;
	buf_t *bp;

	rm_size = SAM_RESOURCE_SIZE(n_vsns);
	while (((error = sam_map_block(bip, (offset_t)0, (offset_t)rm_size,
		SAM_WRITE_BLOCK, NULL, CRED())) != 0) && IS_SAM_ENOSPC(error)) {
		RW_UNLOCK_OS(&bip->inode_rwl, RW_WRITER);
		error = sam_wait_space(bip, error);
		RW_LOCK_OS(&bip->inode_rwl, RW_WRITER);
		if (error)
			break;
	}
	if (error == 0) {
		blkno = bip->di.extent[0];
		blkno <<= (bip->mp->mi.m_bn_shift + SAM2SUN_BSHIFT);
		bp = getblk(bip->mp->mi.m_fs[bip->di.extent_ord[0]].dev, blkno,
					rm_size);
		clrbuf(bp);
		if (copyin((caddr_t)rfp, bp->b_un.b_addr, rm_size)) {
			brelse(bp);
			return (EFAULT);
		}
		bdwrite(bp);
	}

	return (error);
}


/*
 * ----- sam_stage_write - stager daemon writes to disk.
 */

static int				/* ERRNO, else 0 if successful. */
sam_stage_write(
	sam_node_t *ip,			/* Pointer to inode. */
	sam_ioctl_swrite_t *swp,	/* Swrite parameter pointer */
	int	*rvp,			/* Returned value pointer */
	cred_t *credp)			/* Credentials pointer.	*/
{
	uio_t uio;
	iovec_t	iov;
	vnode_t	*vp;
	int	count;
	offset_t st_length;
	int error = 0;
	void *buf;

	buf = (void *)swp->buf.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		buf = (void *)swp->buf.p64;
	}
	count = swp->nbyte;
	vp = SAM_ITOV(ip);
	iov.iov_base = (char *)buf;
	iov.iov_len = count;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_loffset = swp->offset + ip->stage_off;
	uio.uio_segflg = UIO_USERSPACE;
	uio.uio_resid = count;
	uio.uio_fmode = FWRITE;
	uio.uio_llimit = SAM_MAXOFFSET_T;	/* ip->di.rm.size? */

	/*
	 * Don't need the data lock even though writing the file because
	 * the stage thread is always extending the file and no writes
	 * are allowed during the stage. Readers always read behind.
	 */

	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	if ((ip->stage_pid <= 0) || (ip->stage_pid != SAM_CUR_PID)) {
		error = ECANCELED;
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		return (error);
	}
	st_length = ip->stage_off + ip->stage_len;
	if ((uio.uio_loffset + count) < st_length) {
		if (count & (TAR_RECORDSIZE - 1)) {
			ip->stage_err = ECOMM;
			sam_syslog(ip, E_BAD_STAGE, ip->stage_err, count);
		}
	}
	if (!ip->flags.b.staging) {
		ip->stage_err = ECANCELED;
	}
	if (ip->stage_err) {
		error = ip->stage_err;	/* If staging operation cancelled */
	} else if ((uio.uio_loffset + count) > st_length) {
		error = ESPIPE;	/* off + length > filesize, no data written */
	} else {
		boolean_t write_data;
		/*
		 * If partial extents are online, throw away stage data until
		 * reaching partial_size. The user will read the data from the
		 * disk, thus avoiding the stage page locked across stage
		 * writes.
		 */
		write_data = TRUE;
		if (ip->di.status.b.pextents) {
			offset_t partial_size = sam_partial_size(ip);

			if ((uio.uio_loffset + count) <= partial_size) {
				write_data = FALSE;
			} else if (uio.uio_loffset < partial_size) {
				offset_t skip_count =
				    (partial_size - uio.uio_loffset);

				iov.iov_len -= skip_count;
				uio.uio_resid -= skip_count;
				iov.iov_base += skip_count;
				uio.uio_loffset += skip_count;
			}
		}
		if (write_data) {
			int stage_directio = ip->flags.b.stage_directio;

			if (ip->flags.b.stage_n && ip->st_buf) {
				error = sam_stage_n_write_io(vp, &uio);
			} else if (ip->flags.b.stage_directio ||
			    ip->flags.b.directio) {
				if (stage_directio == 0) {
					/*
					 * Switch to directio. Flush and
					 * invalidate pages.
					 */
					sam_flush_pages(ip, B_INVAL);
					ip->flags.b.stage_directio = 1;
				}
				/*
				 * The stage buf lock mechanism was removed as
				 * unneeded.
				 */
				error = sam_dk_direct_io(ip, UIO_WRITE, 0,
				    &uio, credp);
			} else {
				error = sam_stage_write_io(vp, &uio);
			}
			ip->stage_size = (ip->size - MAXBSIZE);
			if (ip->size < MAXBSIZE) {
				ip->stage_size = ip->size;
			}
			ip->stage_size = ip->stage_size & ~(SAM_STAGE_SIZE - 1);
			ip->flags.b.changed = 1;
			count -= uio.uio_resid;
			if (error == EINTR && count != 0) {
				error = 0;
			}

			/*
			 * If shared, notify clients when stage size increased
			 * by 2MB.
			 */
			if (SAM_IS_SHARED_FS(ip->mp) && (ip->stage_size >=
			    (ip->cl_stage_size + SAM_CL_STAGE_SIZE))) {
				ip->cl_stage_size = ip->stage_size;
				sam_notify_staging_clients(ip);
			}
		}
	}
	*rvp = count;		/* Count of bytes transferred if no error */
	mutex_enter(&ip->rm_mutex);
	if (ip->rm_wait && (samgt.stagerd_pid >= 0)) {
		cv_broadcast(&ip->rm_cv);
	}
	mutex_exit(&ip->rm_mutex);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	return (error);
}
