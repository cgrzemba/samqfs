/*
 * ----- uioctl.c - Process the sam ioctl utility functions.
 *
 *	Processes the ioctl utility functions supported on the SAM File
 *	System. Called by sam_vnode function.
 *
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#pragma ident "$Revision: 1.132 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/file.h>
#include <sys/flock.h>
#include <sys/dkio.h>
#include <sys/policy.h>

/* ----- SAMFS Includes */

#include "sam/types.h"
#include "sam/uioctl.h"
#include "sam/resource.h"
#include "pub/sam_errno.h"
#include "pub/rminfo.h"

#include "samfs.h"
#include "inode.h"
#include "mount.h"
#include "ino_ext.h"
#include "ioblk.h"
#include "extern.h"
#include "arfind.h"
#include "trace.h"
#include "debug.h"
#include "kstats.h"
#include "qfs_log.h"

static int sam_set_archive(sam_node_t *ip, struct sam_ioctl_setarch *pp,
		cred_t *credp);

static int sam_get_rm_info_file(sam_node_t *bip, sam_resource_file_t *rfp);

static int sam_get_devices(uname_t fs_name, char *buffer);

/*
 * ----- sam_ioctl_util_cmd - ioctl archiver utility command.
 *
 * Called when the archiver/csd issues an ioctl utility commandi "u" for the
 * file "/mountpoint/.ioctl, /mountpoint/.inodes, etc. file".
 */

int			/* ERRNO if an error occurred, 0 if successful. */
sam_ioctl_util_cmd(
	sam_node_t *ioctl_ip,	/* Pointer to "/mountpoint/file" inode. */
	int cmd,		/* Command number. */
	int *arg,		/* Pointer to arguments. */
	int *rvp,		/* Returned value pointer */
	cred_t *credp)		/* Credentials pointer. */
{
	sam_mount_t *mp;
	int	error	= 0;

	mp  = ioctl_ip->mp;
	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		return (EPERM);
	}
	switch (cmd) {

	case C_SETARCH: {		/* Mark file as archived. */
		struct sam_ioctl_setarch *pp;
		sam_node_t *ip;

		pp = (struct sam_ioctl_setarch *)(void *)arg;
		if (SAM_IS_CLIENT_OR_READER(mp)) {
			error = EINVAL;
		} else {
			if ((error = sam_find_ino(mp->mi.m_vfsp,
			    IG_EXISTS, &pp->id, &ip)) == 0) {
				RW_LOCK_OS(&ip->data_rwl, RW_WRITER);
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				error = sam_set_archive(ip, pp, credp);
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				RW_UNLOCK_OS(&ip->data_rwl, RW_WRITER);
				sam_rele_ino(ip);
			}
		}
		}
		break;

	case C_LISTSETARCH: {
		struct sam_ioctl_listsetarch *pp;

		pp = (struct sam_ioctl_listsetarch *)(void *)arg;
		if (SAM_IS_CLIENT_OR_READER(mp)) {
			error = EINVAL;
		} else {
			struct sam_ioctl_setarch *sap;
			int i;

			sap = (struct sam_ioctl_setarch *)pp->lsa_list.p32;
			if (curproc->p_model != DATAMODEL_ILP32) {
			sap = (struct sam_ioctl_setarch *)pp->lsa_list.p64;
			}
			error = 0;
			for (i = 0; i < pp->lsa_count; i++, sap++) {
				struct sam_ioctl_setarch sa;
				sam_node_t *ip;

				if (copyin((caddr_t)sap, (caddr_t)&sa,
				    sizeof (sa))) {
					error = EFAULT;
					break;
				}
				if ((error = sam_find_ino(mp->mi.m_vfsp,
				    IG_EXISTS, &sa.id, &ip)) == 0) {
					RW_LOCK_OS(&ip->data_rwl, RW_WRITER);
					RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
					error = sam_set_archive(ip, &sa, credp);
					RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
					RW_UNLOCK_OS(&ip->data_rwl, RW_WRITER);
					sam_rele_ino(ip);
				}
				if (error != 0) {
					sa.flags |= SA_error;
					sa.error = error;
					if (copyout((caddr_t)&sa,
					    (caddr_t)sap, sizeof (sa))) {
						error = EFAULT;
						break;
					}
					error = 0;
				}
			}
		}
		}
		break;

	case C_IDSTAT: {		/* Stat inode */
		buf_t *bp = NULL;
		struct sam_perm_inode *permip;
		struct sam_perm_inode *upp;
		struct sam_ioctl_idstat *idstat;
		sam_node_t *ip;
		void *dp;

		idstat = (struct sam_ioctl_idstat *)(void *)arg;
		if (idstat->size != sizeof (struct sam_disk_inode) &&
		    idstat->size !=
				sizeof (struct sam_perm_inode)) {
			error = EINVAL;
			break;
		}
		dp = (void *)idstat->dp.p32;
		if (curproc->p_model != DATAMODEL_ILP32) {
			dp = (void *)idstat->dp.p64;
		}
		upp = (struct sam_perm_inode *)dp;
		if (error = sam_check_cache(&idstat->id, mp, IG_EXISTS,
		    NULL, &ip)) {
			break;
		}
		if (ip != NULL) {
			vnode_t *vp = SAM_ITOV(ip);

			if (copyout((char *)&ip->di, (char *)upp,
					sizeof (struct sam_disk_inode))) {
				error = EFAULT;
			} else if (vn_ismntpt(vp) != NULL) {
				/*
				 * Don't allow idstat to cross mount points.
				 * Intentionally make this test after copying
				 * the "mounted on" disk inode to user space
				 * (for samfsdump).
				 */
				error = EXDEV;
			}
			if (error || idstat->size ==
			    sizeof (struct sam_disk_inode)) {
				/*
				 * Caller only wants the disk inode.
				 * (or some error happened)
				 */
				VN_RELE(vp);
				break;
			}
			if (ip->flags.bits &
			    (SAM_UPDATED|SAM_CHANGED|SAM_DIRTY)) {
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				if (sam_update_inode(ip, SAM_SYNC_ONE, FALSE)) {
					error = EIO;
				}
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			}
			VN_RELE(vp);
			if (error) {
				break;
			}
		}
		if ((error = sam_read_ino(mp, idstat->id.ino, &bp,
		    &permip)) == 0) {
			if ((permip->di.mode == 0) ||
			    (permip->di.id.ino != idstat->id.ino) ||
			    (permip->di.id.gen != idstat->id.gen)) {
				/*
				 * Inode not found or removed.
				 */
				brelse(bp);
				error = ENOENT;
				break;
			}
			if (ip == NULL) {	/* If need to copy disk inode */
				if (copyout((char *)permip, (char *)upp,
					sizeof (struct sam_disk_inode))) {
					error = EFAULT;
					brelse(bp);
					break;
				}
			}
			if (idstat->size == sizeof (struct sam_disk_inode)) {
				brelse(bp);
				break;
			}
			/*
			 * Return csum, and ar sections of the perm_inode.
			 * Also copyout part 2 of the inode to get the
			 * WORM retention period information.
			 */
			if (permip->di.version >= SAM_INODE_VERS_2) {
				/* Current version */
				if (copyout((char *)&permip->csum,
				    (char *)&upp->csum,
				    sizeof (csum_t) +
					sizeof (struct sam_arch_inode))) {
					error = EFAULT;
					brelse(bp);
					break;
				}
				if (copyout((char *)&permip->di2,
				    (char *)&upp->di2,
				    sizeof (struct sam_disk_inode_part2))) {
					error = EFAULT;
					brelse(bp);
					break;
				}
				/*
				 * Return csum, ar and aid sections of the
				 * perm_inode.
				 */
			} else if (permip->di.version == SAM_INODE_VERS_1) {
				/*
				 * Prev inode version (1)
				 */
				if (copyout((char *)&permip->csum,
				    (char *)&upp->csum,
				    sizeof (csum_t) +
				    sizeof (struct sam_arch_inode) +
				    (sizeof (sam_id_t)*MAX_ARCHIVE))) {
					error = EFAULT;
					brelse(bp);
					break;
				}
			}
			brelse(bp);
		}
		idstat->time = SAM_SECOND();
		}
		break;


	case C_IDOPEN:			/* Inode file open */
	case C_IDOPENDIR:		/* Inode directory open */
		/* Fall through to common code. */
	case C_IDOPENARCH: {		/* Inode open for arcopy */
		struct sam_ioctl_idopen *idopen;
		sam_node_t *ip;
		void *dp;

		if (SAM_IS_SHARED_FS(mp) && !SAM_IS_SHARED_SERVER(mp)) {
			error = ENOTSUP;
			break;
		}
		idopen = (struct sam_ioctl_idopen *)(void *)arg;
		if (error = sam_find_ino(mp->mi.m_vfsp, IG_EXISTS,
		    &idopen->id, &ip)) {
			break;
		}
		if (cmd == C_IDOPENARCH) {
			/*
			 * File must not be recently modified, online or direct
			 * access, not damaged, no archive copy made (unless a
			 * rearchive), and not locked.  If modify time is in the
			 * future, no error is returned and file is archived
			 * anyway.
			 */
			if ((ip->di.modify_time.tv_sec <= SAM_SECOND()) &&
			    (ip->di.modify_time.tv_sec > idopen->mtime)) {
				error = EARCHMD;
			} else if (ip->di.status.b.offline &&
			    !(idopen->flags & IDO_offline_direct)) {
				error = ER_FILE_IS_OFFLINE;
			} else if (ip->flags.b.write_mode ||
			    (ip->wmm_pages != 0)) {
				error = EQ_FILE_BUSY;
			} else if (ip->di.status.b.damaged) {
				error = EQ_FILE_DAMAGED;
			} else if (ip->di.arch_status & (1 << idopen->copy) &&
			    !(ip->di.ar_flags[idopen->copy] & AR_rearch)) {
				error = ENOARCH;
			} else if (MANDLOCK(SAM_ITOV(ip), ip->di.mode)) {
				sam_rwlock_vn(SAM_ITOV(ip), 0, NULL);
				if (chklock(SAM_ITOV(ip), FREAD,
				    (sam_offset_t)0, (int)0,
				    FNDELAY, NULL)) {
					error = EQ_FILE_LOCKED;
				}
				sam_rwunlock_vn(SAM_ITOV(ip), 0, NULL);
			}
		}

		/*
		 * The archiver buffer lock optimization was
		 * removed as unneeded.
		 */

		dp = (void *)idopen->dp.p32;
		if (curproc->p_model != DATAMODEL_ILP32) {
			dp = (void *)idopen->dp.p64;
		}
		if (dp != NULL) {
			if (copyout((caddr_t)&ip->di, (caddr_t)dp,
					sizeof (struct sam_disk_inode))) {
				error = EFAULT;
			}
		}
		if (error == 0 && (error =
		    sam_get_fd(SAM_ITOV(ip), rvp)) == 0) {
			vnode_t *vp = SAM_ITOV(ip);

			/*
			 * Can't use VOP_OPEN() or call fop_open() for the
			 * following; sam_open_vn() forbids the opening of
			 * SAM_INO_INO.
			 */
			if (vp->v_type == VREG) {
				atomic_add_32(&vp->v_rdcnt, 1);
				atomic_add_32(&vp->v_wrcnt, 1);
			}
			ip->no_opens++;
		}

		/*
		 * Set archiverd pid and set stage never for direct access.
		 * Track the archiverd pid and the number of arcopy processes
		 * running.
		 *
		 * Later when all arcopies complete we will reset the staging
		 * properties back to their current settings.
		 *
		 * Also, the arch_pid id is used to detect that the
		 * arcopy process is
		 * accessing a file and avoid changing the access time
		 * when a file
		 * is opened by arcopy in read mode for archiving.
		 */

		if (error == 0) {
			if (cmd == C_IDOPENARCH) {
				int i;

				for (i = 0; i < MAX_ARCHIVE; i++) {
					if (ip->arch_pid[i]) {
						if (ip->arch_pid[i] ==
						    curproc->p_ppid) {
							break;
						}
					} else {
						ip->arch_pid[i] =
						    curproc->p_ppid;
						ip->arch_count++;
						break;
					}
				}

				if (idopen->flags & IDO_offline_direct) {
					RW_LOCK_OS(&ip->inode_rwl, RW_READER);
					mutex_enter(&ip->fl_mutex);
					ip->flags.b.arch_direct = 1;
					ip->flags.b.stage_n = 1;
					ip->flags.b.stage_all = 0;
					mutex_exit(&ip->fl_mutex);
					RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
				}
			}
			if (idopen->flags & IDO_direct_io) {

				/*
				 * sam_set_directio() doesn't normally require
				 * data_rwl, but here we are acquiring it to
				 * to coordinate with a thread coming through
				 * sendfile.
				 */
				RW_LOCK_OS(&ip->data_rwl, RW_WRITER);
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				sam_set_directio(ip, DIRECTIO_ON);
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				RW_UNLOCK_OS(&ip->data_rwl, RW_WRITER);
			}
			VN_RELE(SAM_ITOV(ip));
			idopen->mtime = SAM_SECOND();	/* For C_IDOPENDIR */
		} else {
			sam_rele_ino(ip);
		}
		}
		break;


	case C_IDTIME: {			/* Update the file times */
		struct sam_ioctl_idtime *pp;
		sam_node_t *ip;

		pp = (struct sam_ioctl_idtime *)(void *)arg;
		error = sam_find_ino(mp->mi.m_vfsp, IG_EXISTS, &pp->id, &ip);
		if (error == 0) {
			/*
			 * If the file has the WORM bit set do not
			 * update file times. We want to retain the
			 * existing retention period, duration, etc.
			 */
			if (!ip->di.status.b.worm_rdonly) {
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				ip->flags.bits |= (SAM_UPDATED | SAM_CHANGED);
				ip->di.access_time.tv_sec = pp->atime;
				ip->di.access_time.tv_nsec = 0;
				ip->di.modify_time.tv_sec = pp->mtime;
				ip->di.modify_time.tv_nsec = 0;
				ip->di.creation_time = pp->xtime;
				ip->di.attribute_time = pp->ytime;
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			}
			sam_rele_ino(ip);
		}
		}
		break;

	case C_IDSCF: {		/* set/clear the per archive-copy flags */
		struct sam_ioctl_idscf *pp;
		sam_node_t *ip;

		pp = (struct sam_ioctl_idscf *)(void *)arg;
		if (pp->copy >= MAX_ARCHIVE) {
			return (EBADSLT);		/* Illegal copy */
		}
		error = sam_find_ino(mp->mi.m_vfsp, IG_EXISTS, &pp->id, &ip);
		if (error == 0) {
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			if ((ip->di.arch_status & (1 << pp->copy)) ||
			    (ip->di.ar_flags[pp->copy]) & AR_stale) {
				ip->di.ar_flags[pp->copy] = ((~pp->c_flags)
				    & ip->di.ar_flags[pp->copy]) |
				    (pp->c_flags & pp->flags);
				ip->di.status.b.archdone = 0;
				ip->flags.b.changed = 1;
				sam_send_to_arfind(ip, AE_rearchive,
				    pp->copy + 1);
			} else {
				error = EINVAL;
			}
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			sam_rele_ino(ip);
		}
		}
		break;

	case C_ARCHFLAGS: {	/* set/clear the archdone & archnodrop */
		struct sam_ioctl_archflags *pp;
		sam_node_t *ip;

		pp = (struct sam_ioctl_archflags *)(void *)arg;
		error = sam_find_ino(mp->mi.m_vfsp, IG_EXISTS, &pp->id, &ip);
		if (error == 0) {
			int mask;
			int	n;

			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			ip->di.status.b.archnodrop = (pp->archnodrop != 0) ?
			    1 : 0;

			/*
			 * Set/clear copies required.
			 * Allow 'archdone' to be set only if all copies are OK.
			 */
			for (n = 0, mask = 1; n < MAX_ARCHIVE;
			    n++, mask += mask) {
				if (mask & pp->copies_req) {
					ip->di.ar_flags[n] |= AR_required;
				} else {
					ip->di.ar_flags[n] &= ~AR_required;
				}
				if ((ip->di.ar_flags[n] & AR_required) &&
				    (ip->di.ar_flags[n] &
				    (AR_inconsistent | AR_damaged | AR_arch_i |
				    AR_rearch | AR_stale))) {
					pp->archdone = 0;
				}
			}

			ip->di.status.b.archdone = (pp->archdone != 0) ? 1 : 0;
			ip->flags.b.changed = 1;

			/*
			 * If thread waiting on archiving, wake it up now.
			 */
			if (ip->flags.bits & SAM_ARCHIVE_W) {
				mutex_enter(&ip->rm_mutex);
				if (ip->rm_wait) {
					cv_broadcast(&ip->rm_cv);
				}
				mutex_exit(&ip->rm_mutex);
			}
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			sam_rele_ino(ip);
		}
		}
		break;

	case C_IDRESOURCE: {	/* return the removable media information */
		struct sam_ioctl_idresource *pp;
		void *rp;
		sam_node_t *ip;
		buf_t *bp;

		pp = (struct sam_ioctl_idresource *)(void *)arg;
		rp = (void *)pp->rp.p32;
		if (curproc->p_model != DATAMODEL_ILP32) {
			rp = (void *)pp->rp.p64;
		}
		error = sam_get_ino(mp->mi.m_vfsp, IG_EXISTS, &pp->id, &ip);
		if (error == 0) {
			RW_LOCK_OS(&ip->inode_rwl, RW_READER);
			if (pp->size < ip->di.psize.rmfile) {
				error = EINVAL;
			} else {
				if (ip->di.ext_attrs & ext_rfa) {

					/*
					 * Resource data stored as inode
					 * extensions
					 */
					if (ip->di.ext_id.ino == 0) {
						/* No ino exts is bad */
						sam_req_ifsck(mp, -1,
						    "sam_ioctl_util_cmd: "
						    "eid.ino = 0", &ip->di.id);
						error = ENOCSI;
					} else {
						error = sam_get_rm_info_file(ip,
						    (sam_resource_file_t *)rp);
					}

				} else {
					int rm_size;
					sam_resource_file_t *rfp;

					/*
					 * Resource data stored as meta data
					 * block
					 */
					rm_size = SAM_RM_SIZE(
					    ip->di.psize.rmfile);
					ASSERT(mp->mi.m_bn_shift == 0);
					if ((error = SAM_BREAD(mp,
					    mp->mi.m_fs[
					    ip->di.extent_ord[0]].dev,
					    ip->di.extent[0],
					    rm_size, &bp)) == 0) {

						rfp = (sam_resource_file_t *)
						    (void *)bp->b_un.b_addr;
						if (copyout((caddr_t)rfp,
						    (caddr_t)rp,
						    ip->di.psize.rmfile)) {
							error = EFAULT;
						}
						brelse(bp);
					}
				}
			}
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			VN_RELE(SAM_ITOV(ip));
		}
		}
		break;

	case C_IDSTAGE: {		/* stage the file */
		struct sam_ioctl_idstage *pp;
		sam_node_t *ip;
		offset_t length;

		pp = (struct sam_ioctl_idstage *)(void *)arg;
		error = sam_find_ino(mp->mi.m_vfsp, IG_EXISTS, &pp->id, &ip);
		if (error == 0) {
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			if (ip->di.status.b.offline) {
				if (pp->copy != 0) {
					/*
					 * Use only this copy.
					 * Error if no copy.
					 */
					pp->copy--;
					if ((ip->di.arch_status &
					    (1 << pp->copy)) == 0) {
						error = EC_NO_ARCHIVE_COPY;
					}
				}
				if (error == 0) {
					ip->copy = pp->copy;
					if (pp->flags & IS_wait) {
						length = ip->di.rm.size;
					} else {
						length = (offset_t)0;
					}
					ip->flags.b.stage_n = 0;
					ip->flags.b.stage_all = 0;
					ip->di.status.b.archnodrop = 1;
					/*
					 * Return ENOSPC if no disk space.
					 */
					ip->flags.b.nowaitspc = 1;
					if (SAM_IS_SHARED_FS(ip->mp)) {

stage_inode:
						sam_open_operation_nb(ip->mp);
						error = sam_proc_offline(ip,
						    (offset_t)0, length,
						    NULL, credp, NULL);
						if (error == EREMCHG) {
							SAM_CLOSE_OPERATION(
							    ip->mp, error);
							RW_UNLOCK_OS(
							    &ip->inode_rwl,
							    RW_WRITER);


	/* N.B. Bad indentation here to meet cstyle requirements */
	if ((error =
	    sam_open_operation(ip)) == 0) {
		RW_LOCK_OS(
		    &ip->inode_rwl,
		    RW_WRITER);
		SAM_CLOSE_OPERATION(
		    ip->mp,
		    error);
		goto stage_inode;
	}



							RW_LOCK_OS(
							    &ip->inode_rwl,
							    RW_WRITER);
						} else {
							SAM_CLOSE_OPERATION(
							    ip->mp, error);
						}
					} else {
						error = sam_proc_offline(ip,
						    (offset_t)0, length,
						    NULL, credp, NULL);
					}
					ip->flags.b.nowaitspc = 0;
					/*
					 * Cannot reset stage_n and stage_all if
					 * staging. They will be reset in
					 * sam_close_stage after stage
					 * completes.
					 */
					if (!ip->flags.b.staging) {
						ip->flags.b.stage_n =
						    ip->di.status.b.direct;
						ip->flags.b.stage_all =
						    ip->di.status.b.stage_all;
					}
				}
			}
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			sam_rele_ino(ip);
		}
		}
		break;

	case C_IDSEGINFO: {	/* Return index segment inode information */
		struct sam_ioctl_idseginfo *pp;
		sam_node_t *ip;
		void *bp;

		pp = (struct sam_ioctl_idseginfo *)(void *)arg;
		bp = (void *)pp->buf.p32;
		if (curproc->p_model != DATAMODEL_ILP32) {
			bp = (void *)pp->buf.p64;
		}
		if ((error = sam_get_ino(mp->mi.m_vfsp,
		    IG_EXISTS, &pp->id, &ip)) == 0) {
			error = sam_read_segment_info(ip, pp->offset,
			    pp->size, bp);
			VN_RELE(SAM_ITOV(ip));
		}
		}
		break;

	case C_IDMVA: {		/* Return multivolume archive inode info */
		struct sam_ioctl_idmva *idmva;
		sam_node_t *ip;
		struct sam_vsn_section *vsnp;
		void *vp;
		buf_t *bp = NULL;
		int copy;
		int size = 0;

		idmva = (struct sam_ioctl_idmva *)(void *)arg;
		if (idmva->id.ino == 0) {
			error = EINVAL;
			break;
		}
		vp = (void *)idmva->buf.p32;
		if (curproc->p_model != DATAMODEL_ILP32) {
			vp = (void *)idmva->buf.p64;
		}
		vsnp = (struct sam_vsn_section *)vp;
		if (vsnp == NULL) {
			error = EINVAL;
			break;
		}

		/* Return vsn sections, if they exist. */
		if (mp->mi.m_sblk_version == SAMFS_SBLKV2) {
			/* Current version */
			if ((error = sam_get_ino(mp->mi.m_vfsp, IG_EXISTS,
			    &idmva->id, &ip)) == 0) {
				if ((ip->di.ext_attrs & ext_mva) == 0) {
					error = EINVAL;
				} else {
					int t_size = idmva->size;

					RW_LOCK_OS(&ip->inode_rwl, RW_READER);
					/* Retrieve vsns in copy order. */
					for (copy = 0; copy < MAX_ARCHIVE;
					    copy++) {
						if (ip->di.arch_status &
						    (1<<copy)) {
							if (error =
							    sam_get_multivolume(
							    ip, &vsnp,
							    copy, &t_size)) {
								break;
							}
						}
					}
					if ((error == 0) && (t_size > 0)) {
						error = EDOM;
					}
					RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
				}
				VN_RELE(SAM_ITOV(ip));
			}
		} else if (mp->mi.m_sblk_version == SAMFS_SBLKV1) {
			/* Previous vers */
			for (copy = 0; copy < MAX_ARCHIVE; copy++) {
				if (idmva->aid[copy].ino) {
					sam_id_t eid;
					struct sam_inode_ext *eip;
					int ino_vsns;
					int ino_size;

					eid = idmva->aid[copy];
					while (eid.ino) {
						if (error = sam_read_ino(mp,
						    eid.ino, &bp,
						    (struct sam_perm_inode **)
						    &eip)) {
							break;
						}
						if ((eip->hdr.version !=
						    SAM_INODE_VERS_1) ||
						    (eip->hdr.id.ino !=
						    eid.ino) ||
							/*
							 * Backward
							 * compatibility code to
							 * allow 350/33x gen'ed
							 * extension inodes to
							 * pass the header test.
							 * 350/33x set the "gen"
							 * portion of the
							 * "next_id" field to 0.
							 */
						    ((eip->hdr.id.gen !=
						    eid.gen) &&
						    (eid.gen != 0)) ||
						    (eip->hdr.file_id.ino !=
						    idmva->id.ino) ||
						    (eip->hdr.file_id.gen !=
						    idmva->id.gen) ||
						    ((eip->hdr.mode &
						    S_IFEXT) != S_IFMVA)) {
							/*
							 * We can get here if
							 * the base inode is
							 * deleted while chasing
							 * down the extensions.
							 * Don't flag FS for
							 * fsck, but do leave
							 * note for debugging.
							 */
	/* N.B. Bad indentation here to meet cstyle requirements */
			dcmn_err((CE_WARN,
			    "Extension inode "
			    "%d.%d: base "
			    "%d.%d: parent "
			    "id mismatch "
			    "(%d.%d)",
			    eid.ino, eid.gen,
			    idmva->id.ino,
			    idmva->id.gen,
			    eip->hdr.file_id.ino, eip->hdr.file_id.gen));
							error = ENOCSI;
							brelse(bp);
							break;
						}
						ino_vsns = eip->ext.mv1.n_vsns;
			ino_size = ino_vsns * sizeof (struct sam_vsn_section);
						if ((size + ino_size) >
						    idmva->size) {
							error = EOVERFLOW;
							brelse(bp);
							break;
						}
						if (copyout(
						    (caddr_t)&eip->ext.mv1.vsns,
						    (caddr_t)vsnp, ino_size)) {
							error = EFAULT;
							brelse(bp);
							break;
						}
						vsnp += ino_vsns;
						size += ino_size;

						eid = eip->hdr.next_id;
						brelse(bp);
					}
					if (error)  break;
				}
			}
			if ((error == 0) && (size < idmva->size)) {
				error = EDOM;
			}
		}
		}
		break;

#ifdef DEBUG
	case C_PANIC: {			/* Panic machine */
		struct sam_ioctl_panic *pp;

		pp = (struct sam_ioctl_panic *)(void *)arg;
		if (SAM_IS_SHARED_FS(mp) && !SAM_IS_SHARED_SERVER(mp)) {
			(void) sam_proc_block(mp, mp->mi.m_inodir, BLOCK_panic,
			    SHARE_nowait, NULL);
			delay(hz);
		}
		cmn_err(CE_PANIC,
		    "SAM-QFS: %s: APPLICATION panic: status=%x, "
		    "dev=%llx, ino=%lld",
		    mp->mt.fi_name, mp->mt.fi_status, pp->param1, pp->param2);
		}
		break;
#endif /* DEBUG */

	default:
		error = ENOTTY;
		break;
	}
	return (error);
}


#ifdef DKIOCGETVOLCAP

#define		MAX_DMR_CHUNK		(256*1024)
#define		SVM_MODULE		"md"

/*
 * We have an inode and a read request.  The request is to read
 * a particular side of a mirror that presumably comprises the
 * storage under the inode.  Map the offset into the inode, and
 * then issue the I/O request to the underlying volume (assuming
 * it is capable of handling the read mirror side request).
 *
 * This routine emulates/simulates the DKIOCDMR ioctl().
 */
static int
sam_dmr_io(
	sam_node_t *ip,			/* inode */
	struct vol_directed_rd *rdp,	/* I/O request descriptor (updated) */
	int flags,			/* copyin/out flags */
	cred_t *credp)
{
	int error, xerr;
	char *kbuf;
	offset_t klen, rdlen;
	sam_ioblk_t ioblk;
	struct vol_directed_rd *v;

	if (rdp->vdr_nbytes == 0) {
		return (0);
	}

	if (rdp->vdr_offset % DEV_BSIZE) {
		return (EINVAL);
	}

	v = kmem_zalloc(sizeof (*v), KM_SLEEP);

	klen = MIN(rdp->vdr_nbytes, MAX_DMR_CHUNK);
	kbuf = kmem_alloc(klen, KM_SLEEP);

	RW_LOCK_OS(&ip->data_rwl, RW_READER);
	RW_LOCK_OS(&ip->inode_rwl, RW_READER);

	/*
	 * Map the file offset to its block number for the underlying
	 * Volume Manager, which is expecting a device logical block
	 * number.
	 */
	rdlen = klen;
	error = sam_map_block(ip, rdp->vdr_offset, rdlen,
	    SAM_READ, &ioblk, credp);
	if (error) {
		goto out;
	}

	v->vdr_flags = rdp->vdr_flags;
	v->vdr_offset = ((daddr_t)ioblk.blkno) << SAM_DEV_BSHIFT;
	if (ioblk.pboff) {
		v->vdr_offset += ioblk.pboff;
		ioblk.contig -= ioblk.pboff;
	}
	v->vdr_nbytes = (size32_t)MIN(rdlen, ioblk.contig);
	v->vdr_data = (void *)kbuf;
	v->vdr_side = rdp->vdr_side;

	TRACE(T_SAM_IOCTL_DMR, SAM_ITOV(ip),
	    rdp->vdr_offset, rdp->vdr_nbytes, rdp->vdr_side);
	xerr = 0;
	error = cdev_ioctl(ip->mp->mi.m_fs[ioblk.ord].dev, DKIOCDMR,
	    (intptr_t)v, FKIOCTL | FREAD, credp, &xerr);
	if (error != 0) {
		goto out;
	}
	if (xerr != 0) {
		error = xerr;
		goto out;
	}

	rdp->vdr_flags = v->vdr_flags;
	if (v->vdr_bytesread != rdp->vdr_nbytes) {
		rdp->vdr_flags |= DKV_DMR_SHORT;
	}
	rdp->vdr_bytesread = v->vdr_bytesread;
	rdp->vdr_side = v->vdr_side;
	bcopy((char *)v->vdr_side_name, (char *)rdp->vdr_side_name,
	    sizeof (rdp->vdr_side_name));
	if (ddi_copyout(kbuf, rdp->vdr_data, v->vdr_bytesread, flags)) {
		error = EFAULT;
	}

out:
	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	RW_UNLOCK_OS(&ip->data_rwl, RW_READER);
	if (error) {
		TRACE(T_SAM_IOCTL_DMR_ERR, SAM_ITOV(ip),
		    rdp->vdr_offset, rdp->vdr_side, error);
	}
	kmem_free(v, sizeof (*v));
	kmem_free(kbuf, klen);
	return (error);
}


/*
 * ----- sam_ioctl_dkio_cmd
 *
 * Emulate Solaris Disk Suite's ioctl()s in support of Oracle RAC.
 * This requires emulation of the DKIOCINFO, DKIOCGETVOLCAP,
 * DKIOCSETVOLCAP, and DKIOCDMR calls.  The first is used by Oracle
 * RAC to determine if the underlying volume might be a mirror
 * (type md), the second to determine if ABR (application based
 * recovery) and DMR (directed mirror reads) are supported, the
 * 3rd to enable ABR operation, and the last to request a read
 * from a particular mirror side.
 *
 * In our case, we set these flags per SAM-AIO configured file.
 * If ABR is set via the DKIOCSETVOLCAP ioctl(), then writes
 * issued to the associated, underlying storage will have their
 * direct I/O writes flagged with B_ABRWRITE, to indicate that
 * the write should not set the DRL bit associated with that
 * chunk of storage.  This potentially speeds performance, at
 * the expense of the application, which must, if a host crashes
 * while a write is outstanding, re-write the potentially
 * written blocks to ensure mirror consistency (typically
 * after reading some or all sides of the mirror to determine
 * whether or not the write partially or fully succeeded).
 *
 * We also emulate the DKIOCINFO call.  Oracle RAC, our
 * biggest application, makes this call to the underlying
 * storage, and will only attempt ABR operation if the
 * underlying volume reports back that it's an SDS md volume.
 *
 * XXX It may do this for VxVM volumes too.  ???
 */
int				/* ERRNO, else 0 if successful. */
sam_ioctl_dkio_cmd(
	sam_node_t *ip,		/* Pointer to "/mountpoint/file" inode */
	int cmd,		/* Command number */
	sam_intptr_t arg,	/* Pointer to arguments */
	int flags,		/* ioctl memcp flags */
	cred_t *credp)		/* Credentials pointer */
{
	sam_mount_t *mp = ip->mp;
	int error = 0;
	union {
		struct dk_cinfo dci;
		struct volcap vci;
		struct vol_directed_rd vdr;
	} *ic;
#ifdef _MULTI_DATAMODEL
	struct vol_directed_rd32 *vdr32 = NULL;
#endif

	ic = kmem_zalloc(sizeof (*ic), KM_SLEEP);
	switch (cmd) {
	case DKIOCINFO:				/* Get disk info (emulate) */
		strncpy(&ic->dci.dki_cname[0], SAM_FSTYPE,
		    sizeof (ic->dci.dki_cname));
		/* may need to change */
		ic->dci.dki_ctype = (ushort_t)samgt.fstype;
		ic->dci.dki_flags = 0x0;
		ic->dci.dki_cnum = mp->mt.fi_eq; /* controller number */
		ic->dci.dki_addr = 0;		/* controller address */
		ic->dci.dki_space = 0;		/* controller bus type */
		ic->dci.dki_prio = 0;		/* interrupt priority */
		ic->dci.dki_vec = 0;		/* interrupt vector */
		strncpy(&ic->dci.dki_dname[0], SVM_MODULE,
		    sizeof (ic->dci.dki_dname));
		ic->dci.dki_unit = ip->di.id.ino;
		ic->dci.dki_partition = 0;
		ic->dci.dki_slave = 0;
		ic->dci.dki_maxtransfer =
		    (1 << (8*sizeof (ic->dci.dki_maxtransfer) - 1)) - 1;
		error = ddi_copyout((void *)&ic->dci,
		    (void *)arg, sizeof (ic->dci), flags);
		break;

	case DKIOCGETVOLCAP: {		/* Get Volume capabilities (ABR/DMR) */
		ic->vci.vc_info = ((mp->mt.fi_config & MT_ABR_DATA) ?
		    DKV_ABR_CAP : 0) |
		    ((mp->mt.fi_config & MT_DMR_DATA) ?
		    DKV_DMR_CAP : 0);
		ic->vci.vc_set = (ip->flags.b.abr ? DKV_ABR_CAP : 0);
		error = ddi_copyout((void *)&ic->vci,
		    (void *)arg, sizeof (ic->vci), flags);
		}
		break;

	case DKIOCSETVOLCAP:		/* Set Volume capabilities (ABR/DMR) */
		if ((flags & FWRITE) == 0) {
			error = EPERM;
			break;
		}
		ddi_copyin((void *)arg, (void *)&ic->vci,
		    sizeof (ic->vci), flags);
		TRACE(T_SAM_ABR_IOCTL, SAM_ITOP(ip),
		    ip->di.id.ino, ic->vci.vc_set, ip->flags.b.abr);
		if (ic->vci.vc_set & DKV_ABR_CAP) {
			int setabr = 0;

			RW_LOCK_OS(&ip->inode_rwl, RW_READER);
			mutex_enter(&ip->fl_mutex);

			if (!ip->flags.b.abr) {
				if (mp->mt.fi_config & MT_ABR_DATA) {
					sam_set_abr(ip);
					setabr = 1;
				} else {
					error = EINVAL;
				}
			}

			mutex_exit(&ip->fl_mutex);

			if (!error && setabr && SAM_IS_SHARED_FS(ip->mp)) {
				if (SAM_IS_SHARED_SERVER(ip->mp)) {
					sam_callout_abr(ip,
					    mp->ms.m_client_ord);
					RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
				} else {
					/*
					 * Notify server
					 */
					RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
					error = sam_proc_inode(ip,
					    INODE_setabr, NULL, CRED());
				}
			} else {
				RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			}
		} else {
			RW_LOCK_OS(&ip->inode_rwl, RW_READER);
			mutex_enter(&ip->fl_mutex);

			ip->flags.b.abr = 0;

			mutex_exit(&ip->fl_mutex);
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		}
		if (error) {
			break;
		}
		if (ic->vci.vc_set & DKV_DMR_CAP) {
			/*
			 * Return an error if a request has been made to set DMR
			 * and the FS doesn't support it.  Otherwise ignore it.
			 */
			if ((mp->mt.fi_config & MT_DMR_DATA) == 0) {
				error = EINVAL;
				break;
			}
		}
		error = ddi_copyout((void *)&ic->vci, (void *)arg,
		    sizeof (ic->vci), flags);
		break;

	case DKIOCDMR:			/* directed mirror read request */
#ifdef _MULTI_DATAMODEL
		vdr32 = kmem_zalloc(sizeof (*vdr32), KM_SLEEP);
		if ((flags & DATAMODEL_MASK) == DATAMODEL_ILP32) {
			ddi_copyin((void *)arg, (void *)vdr32,
			    sizeof (*vdr32), flags);

			ic->vdr.vdr_flags = vdr32->vdr_flags;
			ic->vdr.vdr_offset = vdr32->vdr_offset;
			ic->vdr.vdr_nbytes = vdr32->vdr_nbytes;
			ic->vdr.vdr_data = (void *)(uintptr_t)vdr32->vdr_data;
			ic->vdr.vdr_side = vdr32->vdr_side;
		} else
#endif
		{
			if (ddi_copyin((void *)arg, (void *)&ic->vdr,
			    sizeof (ic->vdr), flags)) {
				error = EFAULT;
				break;
			}
		}
		error = sam_dmr_io(ip, &ic->vdr, flags, credp);
		if (error) {
			break;
		}
#ifdef _MULTI_DATAMODEL
		if ((flags & DATAMODEL_MASK) == DATAMODEL_ILP32) {
			vdr32->vdr_flags = ic->vdr.vdr_flags;
			vdr32->vdr_offset = ic->vdr.vdr_offset;
			vdr32->vdr_bytesread = ic->vdr.vdr_bytesread;
			vdr32->vdr_side =  ic->vdr.vdr_side;
			bcopy((char *)&ic->vdr.vdr_side_name,
			    (char *)&vdr32->vdr_side_name,
			    sizeof (vdr32->vdr_side_name));
			if (ddi_copyout((void *)vdr32, (void *)arg,
			    sizeof (*vdr32), flags)) {
				error = EFAULT;
			}
		} else
#endif
		{
			if (ddi_copyout((void *)&ic->vdr, (void *)arg,
			    sizeof (ic->vdr), flags)) {
				error = EFAULT;
			}
		}
		break;

	default:
		error = ENOTTY;
		break;
	}
	kmem_free(ic, sizeof (*ic));
#ifdef _MULTI_DATAMODEL
	if (vdr32) {
		kmem_free(vdr32, sizeof (*vdr32));
	}
#endif
	return (error);
}

#endif	/* DKIOCGETVOLCAP */


/*
 * ----- sam_set_archive - mark file as archived.
 *	Mark file as archived. Copy and update the archive record
 *	into the inode.
 */

static int				/* ERRNO, else 0 if successful. */
sam_set_archive(
	sam_node_t *ip,			/* Pointer to inode. */
	struct sam_ioctl_setarch *pp,	/* Ioctl arguments. */
	cred_t *credp)			/* Credentials pointer. */
{
	struct sam_vsn_section *vsnp;
	struct sam_perm_inode *permip;
	buf_t *bp;
	uint_t copy;
	int error = 0;
	boolean_t release = FALSE;
	boolean_t inconsistent = FALSE;

	ASSERT(RW_OWNER_OS(&ip->data_rwl) == curthread);

	if (ip->mp->mt.fi_mflag & MS_RDONLY) {
		return (EROFS);
	}

	copy = pp->copy;
	vsnp = (void *)pp->vp.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		vsnp = (void *)pp->vp.p64;
	}

	if (copy >= MAX_ARCHIVE) {		/* Illegal copy number. */
		return (EBADSLT);
	}

	if (ip->di.id.ino == SAM_INO_INO) {	/* Illegal inode number. */
		return (EINVAL);
	}

	/*
	 * Check if file changed, Can't archive copy if file modified unless
	 * inconsistent attribute was set.
	 */
	if (pp->modify_time.tv_sec != ip->di.modify_time.tv_sec) {
		if (!ip->di.status.b.inconsistent_archive) {
			SAM_COUNT64(sam, noarch_incon);
			return (EARCHMD);
		}
		inconsistent = TRUE;
	}

	/*
	 * Check if file is already archived, Can't archive if AR_rearch or
	 * AR_inconsistent not set.
	 */
	if ((ip->di.arch_status & (1 << copy)) &&
	    (!(ip->di.ar_flags[copy] & (AR_rearch|AR_inconsistent)))) {
		SAM_COUNT64(sam, noarch_already);
		return (ENOARCH);
	}

	/*
	 * Check if regular file checksum attributes changed while archiving.
	 */
	if (S_ISREG(ip->di.mode) && ip->di.status.b.cs_gen &&
	    (!(pp->flags & SA_csummed))) {
		return (EINVAL);
	}

	/*
	 * Check if position is nonzero.
	 */
	if ((pp->ar.position == 0) &&
	    (!(pp->ar.arch_flags & SAR_diskarch))) {
		return (EINVAL);
	}

	if (ip->di.version >= SAM_INODE_VERS_2) {	/* Version 2 or 3 */
		if ((pp->ar.n_vsns > 1) || (ip->di.ext_attrs & ext_mva)) {
			if (ip->di.ext_attrs & ext_mva) {
				/*
				 * Free multivolume inodes from possible prev
				 * arch copy.
				 */
				sam_free_inode_ext(ip, S_IFMVA, copy,
				    &ip->di.ext_id);
			}
			if (pp->ar.n_vsns > 1) {
				if (error = sam_set_multivolume(ip, &vsnp, copy,
				    pp->ar.n_vsns, &ip->di.ext_id)) {
					return (error);
				}
				ip->di.ext_attrs |= ext_mva;
			}
		}
	}
	if ((error = sam_read_ino(ip->mp, ip->di.id.ino, &bp, &permip)) == 0) {
		if (ip->di.version == SAM_INODE_VERS_1) {
			sam_id_t id;
			sam_perm_inode_v1_t *permip_v1 =
			    (sam_perm_inode_v1_t *)permip;

			/*
			 * Previous inode version (1)
			 */
			id = permip_v1->aid[copy];
			if ((pp->ar.n_vsns > 1) || id.ino) {
				/*
				 * Process multivolume inode
				 */
				brelse(bp);
				if (id.ino) {
					/*
					 * Free multivolume inodes from previous
					 * archive copy.
					 */
					sam_free_inode_ext(ip, S_IFMVA,
					    copy, &id);
				}
				if (pp->ar.n_vsns > 1) {
					if (error = sam_set_multivolume(ip,
					    &vsnp, copy,
					    pp->ar.n_vsns, &id)) {
						return (error);
					}
				} else {
					id.ino = 0;
					id.gen = 0;
				}
				if (error = sam_read_ino(ip->mp,
				    ip->di.id.ino, &bp, &permip)) {
					return (error);
				}
				permip_v1->aid[copy] = id;
			}
		}
		ip->di.media[copy] = permip->di.media[copy] = pp->media;
		if (!inconsistent) {
			ip->di.arch_status = permip->di.arch_status |=
			    (1 << copy);
		}
		ip->di.ar_flags[copy] = permip->di.ar_flags[copy] &=
		    AR_required;
		if (ip->di.status.b.cs_gen && (pp->flags & SA_csummed)) {
			permip->csum = pp->csum;
			ip->di.status.b.cs_val = 1;
			permip->di.status.b.cs_val = 1;
		}
		permip->ar.image[copy] = pp->ar;
		if (permip->ar.image[copy].creation_time == 0) {
			permip->ar.image[copy].creation_time = SAM_SECOND();
		}
		if (inconsistent) {
			SAM_COUNT64(sam, archived_incon);
			ip->di.ar_flags[copy] |= AR_inconsistent;
		}

		SAM_COUNT64(sam, archived);

		/*
		 * If all required copies are done, and no copies need further
		 * work set archive done.
		 */
		if ((ip->di.arch_status & pp->sa_copies_req) ==
		    pp->sa_copies_req) {
			boolean_t archdone;
			int mask;
			int	n;

			archdone = TRUE;
			for (n = 0, mask = 1; n < MAX_ARCHIVE;
			    n++, mask += mask) {
				if ((mask & pp->sa_copies_req) &&
				    (ip->di.ar_flags[n] &
				    (AR_inconsistent | AR_damaged |
				    AR_arch_i |
				    AR_rearch | AR_stale))) {
					archdone = FALSE;
				}
			}
			ip->di.status.b.archdone =
			    permip->di.status.b.archdone = archdone;
		}

		if (S_ISREG(ip->di.mode) && !S_ISSEGI(&ip->di)) {
			/*
			 * Set archnodrop if all copies before release are not
			 * done.  If all copies have been made and the file
			 * should be released, release it.
			 */
			if ((ip->di.arch_status & pp->sa_copies_norel) !=
			    pp->sa_copies_norel) {
				ip->di.status.b.archnodrop =
				    permip->di.status.b.archnodrop = 1;
			} else {
				ip->di.status.b.archnodrop =
				    permip->di.status.b.archnodrop = 0;
			}
			if (!ip->di.status.b.nodrop) {
				/*
				 * Releasing a file is allowed.
				 */
				if (ip->di.status.b.release &&
				    ip->di.status.b.archdone) {
					release = TRUE;
				} else {
					int copy_bits;

					/*
					 * Release after all required copies
					 * made.
					 */
					copy_bits = pp->sa_copies_rel &
					    pp->sa_copies_norel;
					if (copy_bits != 0 &&
					    ((ip->di.arch_status &
					    copy_bits) == copy_bits)) {
						release = TRUE;
					}

					/*
					 * Required release after this copy
					 * mode.
					 */
					copy_bits = pp->sa_copies_rel &
					    ~pp->sa_copies_norel;
					if (copy_bits & 1 << copy) {
						release = TRUE;
					}
				}
			}
		}

		ip->flags.b.changed = 1;
		bdwrite(bp);
	}
	/*
	 * Build a stage request and send it to the stager for
	 * files requiring data verification.
	 */
	if (!error &&
	    !S_ISDIR(ip->di.mode) &&
	    (ip->di.rm.ui.flags & RM_DATA_VERIFY)) {

		int i;
		int prealloc;
		int req_ext_cnt;
		sam_stage_request_t *req;
		sam_stage_request_t *req_ext;

		req = kmem_alloc(sizeof (*req), KM_SLEEP);
		if (req == NULL) {
			error = ENOMEM;
		} else {
			error = sam_build_stagerd_req(ip, copy, req,
			    &req_ext_cnt,
			    &req_ext, &prealloc, credp);
			if (!error) {
				req->arcopy[copy].flags |= STAGE_COPY_VERIFY;
				req->copy = copy;
				req->len = ip->di.rm.size;
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				TRACES(T_SAM_FIFO_STAGEV, SAM_ITOV(ip),
				    (char *)req->arcopy[0].section[0].vsn);
				error = sam_send_stage_cmd(ip->mp, SCD_stager,
				    req, sizeof (*req));
				/*
				 * Send any request extension, used for
				 * multivolume.
				 */
				for (i = 0; i < req_ext_cnt; i++) {
					if (error == 0) {
						req_ext[i].arcopy[copy].flags |=
						    STAGE_COPY_VERIFY;
						error = sam_send_stage_cmd(
						    ip->mp,
						    SCD_stager, &req_ext[i],
						    sizeof (*req));
					}
				}
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			}
		}
		if (req_ext_cnt > 0) {
			kmem_free((void *)req_ext,
			    (req_ext_cnt * sizeof (sam_stage_request_t)));
		}
		kmem_free(req, sizeof (*req));
	}
	if (release) {
		SAM_COUNT64(sam, archived_rele);
		error = sam_drop_ino(ip, credp);
	}
	/*
	 * If thread waiting on archiving, wake it up now if at least 1
	 * archive copy.
	 */
	if ((ip->flags.bits & SAM_ARCHIVE_W) && ip->di.arch_status) {
		mutex_enter(&ip->rm_mutex);
		if (ip->rm_wait) {
			cv_broadcast(&ip->rm_cv);
		}
		mutex_exit(&ip->rm_mutex);
	}
	TRACE(T_SAM_ARCHIVE_MARK, SAM_ITOV(ip), pp->media, (pp->copy+1), error);
	return (error);
}


/*
 * ----- sam_set_multivolume - set the multivolume inode.
 *
 * Allocate inode(s) for multivolume information;
 * Build multivolume inode(s) and write to disk.
 */

int					/* ERRNO, else 0 if successful. */
sam_set_multivolume(
	sam_node_t *bip,		/* Pointer to base inode. */
	struct sam_vsn_section **vsnpp,	/* vsn section array */
	int copy,			/* Copy number, 0..(MAX_ARCHIVE-1) */
	int t_vsns,			/* Num vsns in vsn section array */
	sam_id_t *fid)			/* out: Ptr to 1st inode extension */
{
	int error = 0;
	int vsns;
	int n_exts;
	buf_t *bp;
	sam_id_t eid;
	struct sam_inode_ext *eip;
	int ino_vsns;

	vsns = t_vsns;
	if (vsns == 0) {
		error = EINVAL;
	} else {
		n_exts = howmany(vsns, MAX_VSNS_IN_INO);
		if ((error = sam_alloc_inode_ext(bip, S_IFMVA,
		    n_exts, fid)) == 0) {
			eid = *fid;
			while (eid.ino && (vsns > 0)) {
				if (error = sam_read_ino(bip->mp, eid.ino, &bp,
					(struct sam_perm_inode **)&eip)) {
					break;
				}
				if (EXT_HDR_ERR(eip, eid, bip) ||
				    EXT_MODE_ERR(eip, S_IFMVA)) {
					sam_req_ifsck(bip->mp, -1,
					    "sam_set_multivolume: "
					    "EXT_HDR/EXT_MODE", &bip->di.id);
					error = ENOCSI;
					brelse(bp);
					break;
				}
				if (bip->di.version >= SAM_INODE_VERS_2) {
					/*
					 * Current inode version (2)
					 */
					eip->ext.mva.creation_time =
					    SAM_SECOND();
					eip->ext.mva.change_time =
					    eip->ext.mva.creation_time;
					eip->ext.mva.copy = copy;
					eip->ext.mva.t_vsns = t_vsns;
					ino_vsns = MIN(vsns, MAX_VSNS_IN_INO);
					eip->ext.mva.n_vsns = ino_vsns;
					if (copyin((caddr_t)*vsnpp,
					    (caddr_t)
					    &eip->ext.mva.vsns.section[0],
					    sizeof (struct sam_vsn_section) *
					    ino_vsns)) {
						error = EFAULT;
						brelse(bp);
						break;
					}
				} else if (bip->di.version ==
				    SAM_INODE_VERS_1) {
					/*
					 * Previous inode version (1)
					 */
					eip->ext.mv1.copy = copy;
					ino_vsns = MIN(vsns, MAX_VSNS_IN_INO);
					eip->ext.mv1.n_vsns = ino_vsns;
					if (copyin((caddr_t)*vsnpp,
					    (caddr_t)
					    &eip->ext.mv1.vsns.section[0],
					    sizeof (struct sam_vsn_section) *
					    ino_vsns)) {
						error = EFAULT;
						brelse(bp);
						break;
					}
				}
				*vsnpp += ino_vsns;
				vsns -= ino_vsns;

				eid = eip->hdr.next_id;
				bdwrite(bp);
			}
			if (error) {
				sam_free_inode_ext(bip, S_IFMVA, copy, fid);
			}
		}
	}

	TRACE(T_SAM_SET_MVA_EXT, SAM_ITOV(bip), copy, t_vsns, error);
	return (error);
}


/*
 * ----- sam_get_rm_info_file - get removable media info for rmedia inode.
 *
 * Scan inode extensions for removable media info and copy to user.
 */

static int				/* ERRNO, else 0 if successful. */
sam_get_rm_info_file(
	sam_node_t *bip,		/* Pointer to base inode. */
	sam_resource_file_t *rfp)	/* user removable media structure */
{
	int error = 0;
	int n_vsns = 0;
	int vsns = 0;
	int size = 0;
	sam_id_t eid;
	buf_t *bp;
	struct sam_inode_ext *eip;
	int ino_size;
	int ino_vsns;

	eid = bip->di.ext_id;
	while (eid.ino && (size < bip->di.psize.rmfile)) {
		if (error = sam_read_ino(bip->mp, eid.ino, &bp,
				(struct sam_perm_inode **)&eip)) {
			break;
		}
		if (EXT_HDR_ERR(eip, eid, bip)) {
			sam_req_ifsck(bip->mp, -1,
			    "sam_get_rm_info_file: EXT_HDR", &bip->di.id);
			brelse(bp);
			error = ENOCSI;
			break;
		}
		if (S_ISRFA(eip->hdr.mode)) {
			if (EXT_1ST_ORD(eip)) {
				struct sam_rfa_inode *rfa = &eip->ext.rfa;

				/*
				 * Resource info and vsn[0] in first inode ext
				 */
				n_vsns = rfa->info.n_vsns;
				if ((n_vsns == 0) || (n_vsns > MAX_VOLUMES)) {
					error = EDOM;
				} else {
					ino_size = sizeof (sam_resource_file_t);
					if (ino_size > bip->di.psize.rmfile) {
						ino_size = bip->di.psize.rmfile;
						error = EOVERFLOW;
					}
					if (copyout((caddr_t)&rfa->info,
					    (caddr_t)rfp, ino_size)) {
						error = EFAULT;
					}
				}
				if (error) {
					brelse(bp);
					break;
				}
				vsns = 1;
			} else {
				struct sam_rfv_inode *rfv = &eip->ext.rfv;

				/*
				 * Rest of vsn list is in following inode exts
				 */
				ino_vsns = rfv->n_vsns;
				if ((rfv->ord != vsns) ||
				    (ino_vsns == 0) ||
				    (ino_vsns > MAX_VSNS_IN_INO)) {
					error = EDOM;
				} else {
					ino_size =
					    sizeof (struct sam_vsn_section) *
					    ino_vsns;
					if ((size + ino_size) >
					    bip->di.psize.rmfile) {
						ino_size =
						    bip->di.psize.rmfile - size;
						error = EOVERFLOW;
					}
					if (ino_size > 0) {
						if (copyout(
						    (caddr_t)
						    &rfv->vsns.section[0],
						    (caddr_t)rfp, ino_size)) {
							error = EFAULT;
						}
					}
				}
				if (error) {
					brelse(bp);
					break;
				}
				vsns += ino_vsns;
			}
			rfp = (sam_resource_file_t *)(void *)
			    ((caddr_t)rfp + ino_size);
			size += ino_size;
		}
		eid = eip->hdr.next_id;
		brelse(bp);
	}
	if (error == 0) {
		if ((size != bip->di.psize.rmfile) || (vsns != n_vsns)) {
			sam_req_ifsck(bip->mp, -1,
			    "sam_get_rm_info_file: n_vsns", &bip->di.id);
			error = ENOCSI;
		}
	}

	TRACE(T_SAM_GET_RFA_EXT, SAM_ITOV(bip), size, vsns, error);
	return (error);
}


/*
 * ----- sam_ioctl_oper_cmd - ioctl operator utility commands.
 *	Called when the operator issues an ioctl utility command "U" for the
 *	file "/mountpoint/.ioctl, /mountpoint/.inodes, etc. file".
 */

/* ARGSUSED3 */
int				/* ERRNO, else 0 if successful. */
sam_ioctl_oper_cmd(
	sam_node_t *ioctl_ip,	/* Pointer to "/mountpoint/file" inode. */
	int cmd,		/* Command number. */
	int *arg,		/* Pointer to arguments. */
	int *rvp,		/* Returned value pointer */
	cred_t *credp)		/* Credentials pointer. */
{
	sam_mount_t *mp;
	int	error	= 0;

	mp  = ioctl_ip->mp;
	switch (cmd) {

	case C_SBLK: {				/* Read superblock table */
		struct sam_ioctl_sblk *sb;
		void *sbp;

		sb = (struct sam_ioctl_sblk *)(void *)arg;
		sbp = (void *)sb->sbp.p32;
		if (curproc->p_model != DATAMODEL_ILP32) {
			sbp = (void *)sb->sbp.p64;
		}
		if (copyout((char *)mp->mi.m_sbp, (char *)sbp,
		    mp->mi.m_sbp->info.sb.sblk_size)) {
			error = EFAULT;
		}
		}
		break;

	case C_SBINFO: {		/* Read superblock information */
		struct sam_ioctl_sbinfo *sb;
		void *sbinfo;

		sb = (struct sam_ioctl_sbinfo *)(void *)arg;
		sbinfo = (void *)sb->sbinfo.p32;
		if (curproc->p_model != DATAMODEL_ILP32) {
			sbinfo = (void *)sb->sbinfo.p64;
		}
		if (copyout((char *)mp->mi.m_sbp, (char *)sbinfo,
				sizeof (struct sam_sbinfo))) {
			error = EFAULT;
		}
		}
		break;

	case C_MOUNT: {				/* Read mount table */
		struct sam_ioctl_mount *mmp;
		void *mount;

		mmp = (struct sam_ioctl_mount *)(void *)arg;
		mount = (void *)mmp->mount.p32;
		if (curproc->p_model != DATAMODEL_ILP32) {
			mount = (void *)mmp->mount.p64;
		}
		if (copyout((char *)mp, (char *)mount, sizeof (sam_mount_t) +
		    ((mp->mt.fs_count - 1) *
				sizeof (struct samdent)))) {
			error = EFAULT;
		}
		}
		break;

	case C_ZAPINO:			/* Zero inode */
		if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
			return (EPERM);
		}
		/*FALLTHRU*/
	case C_RDINO: {					/* Read inode */
		struct sam_ioctl_inode *iip;
		sam_id_t id;
		buf_t *bp;
		sam_node_t *ip;
		struct sam_perm_inode *permip;

		iip = (struct sam_ioctl_inode *)(void *)arg;
		if (SAM_ITOD(iip->ino) > mp->mi.m_inodir->di.rm.size) {
			error = ENOENT;
			break;
		}
		id.ino = iip->ino;
		iip->extent_factor = 1 << mp->mi.m_sbp->info.sb.ext_bshift;
		if ((error = sam_read_ino(mp, id.ino, &bp, &permip)) == 0) {
			void *ptr;

			ptr = (void *)iip->pip.p32;
			if (curproc->p_model != DATAMODEL_ILP32) {
				ptr = (void *)iip->pip.p64;
			}
			if (cmd == C_RDINO) {
				if (ptr != NULL) {
					if (copyout((char *)permip,
					    (char *)ptr,
					    sizeof (struct sam_perm_inode))) {
						error = EFAULT;
					}
					iip->mode = 1;
				}
			}
			id.gen = permip->di.id.gen;
			if (cmd == C_ZAPINO) {
				bzero((char *)permip,
				    sizeof (struct sam_perm_inode));
				bdwrite(bp);
			} else {
				brelse(bp);
			}

			/*
			 * Check to see if inode is in hash chain.
			 */
			error = sam_check_cache(&id, mp, IG_EXISTS, NULL, &ip);
			if (error == 0 && ip != NULL) {
				ptr = (void *)iip->ip.p32;
				if (curproc->p_model != DATAMODEL_ILP32) {
					ptr = (void *)iip->ip.p64;
				}
				RW_LOCK_OS(&ip->inode_rwl, RW_READER);
				if (cmd == C_RDINO) {
					if (ptr != NULL) {
						if (copyout((char *)ip,
						    (char *)ptr,
						    sizeof (struct sam_node))) {
							error = EFAULT;
						}
					}
				} else if (cmd == C_ZAPINO) {
					bzero((char *)&ip->di,
					    sizeof (struct sam_disk_inode));
					ip->di.id = id;
				}
				RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
				VN_RELE(SAM_ITOV(ip));
				iip->mode = 0;
			}
		}
		}
		break;

	case C_DEV: {
		uname_t fs_name;
		char *buffer;
		struct sam_ioctl_dev *devbuf;
		void *outbuf;

		devbuf = (struct sam_ioctl_dev *)(void *)arg;
		outbuf = (void *)devbuf->buf.ptr;

		/*
		 * Get the file system name.
		 */
		bcopy((char *)(void *)arg, fs_name, sizeof (uname_t));

		buffer = kmem_alloc(sizeof (upath_t) *
		    MAX_DEVS_SAMQFS, KM_SLEEP);
		if (buffer == NULL) {
			error = ENOMEM;
			break;
		} else {
			error = sam_get_devices(fs_name, buffer);
		}

		if (error == 0) {
			if (copyout((char *)buffer, (char *)(void *)outbuf,
			    (sizeof (upath_t) * MAX_DEVS_SAMQFS))) {
					error = EFAULT;
			}
		}

		kmem_free(buffer, (sizeof (upath_t) * MAX_DEVS_SAMQFS));
		}
		break;

	default:
		error = ENOTTY;
		break;
	}
	return (error);
}


/*
 * ----- sam_get_fd - get a file descriptor
 *	Called to return a file description for the stager or archiver IDOPEN
 *  ioctl.
 */

int					/* ERRNO if error, 0 if ok */
sam_get_fd(vnode_t *vp, int *rvp)
{
	file_t *fp;
	int	fd;
	int	error;

	VN_HOLD(vp);
	if ((error = falloc(NULL, (FREAD|FWRITE|FOFFMAX), &fp, &fd)) == 0) {
		fp->f_vnode = vp;
		mutex_exit(&fp->f_tlock);
		setf(fd, fp);	/* Fill in file slot */
		*rvp = fd;
	} else {
		VN_RELE(vp);
	}
	return (error);
}

/*
 * Find the devices associated with the given filesystem and
 * copy them into the provided buffer.
 */
static int
sam_get_devices(uname_t fs_name, char *buffer)
{
	sam_mount_t *mp;
	int error, i;

	/*
	 * Find requested mount point.
	 */
	mutex_enter(&samgt.global_mutex);
	for (mp = samgt.mp_list; mp != NULL; mp = mp->ms.m_mp_next) {
		if (strcmp(mp->mt.fi_name, fs_name) == 0) {
			break;
		}
	}
	if (mp != NULL) {

		/*
		 * Copy partitions into buffer.
		 */
		error = 0;
		for (i = 0; i < mp->mt.fs_count && i < MAX_DEVS_SAMQFS; i++) {
			mutex_enter(&mp->ms.m_waitwr_mutex);
			if ((mp->mt.fi_status & FS_MOUNTED) &&
			    !(mp->mt.fi_status &
			    (FS_MOUNTING | FS_UMOUNT_IN_PROGRESS)) &&
			    (mp->mi.m_sbp != NULL)) {
				bcopy(mp->mi.m_fs[i].part.pt_name,
				    &buffer[i*sizeof (upath_t)],
				    sizeof (upath_t));
			} else {
				error =  ENOENT;
				mutex_exit(&mp->ms.m_waitwr_mutex);
				break;
			}
			mutex_exit(&mp->ms.m_waitwr_mutex);
		}
	} else {
		error = ENOENT;
	}
	mutex_exit(&samgt.global_mutex);
	return (error);
}
