/*
 * ----- sam/vfsops.c - Process the vfs_ops functions.
 *
 * Processes the vfs_ops functions supported on the SAM File System.
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

#pragma ident "$Revision: 1.165 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/flock.h>
#include <sys/debug.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/ksynch.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/pathname.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/ddi.h>
#include <sys/cmn_err.h>
#include <sys/file.h>
#include <sys/disp.h>
#include <sys/mount.h>
#include <sys/mntent.h>
#include <sys/fs_subr.h>
#include <sys/panic.h>
#include <vm/seg_map.h>
#include <sys/policy.h>

/* ----- SAMFS Includes */

#include "sam/types.h"
#include "sam/mount.h"
#include "sam/samevent.h"

#ifdef METADATA_SERVER
#include "license/license.h"
#endif	/* METADATA_SERVER */

#include "samfs.h"
#include "inode.h"
#include "mount.h"
#include "global.h"
#include "dirent.h"
#include "debug.h"
#include "extern.h"
#include "arfind.h"
#include "trace.h"
#include "qfs_log.h"
#include "objctl.h"
extern int ncsize;

extern int sam_freeze_ino(sam_mount_t *mp, sam_node_t *ip, int force_freeze);
static int sam_clients_mounted(sam_mount_t *mp);
static int sam_open_vfs_operation(sam_mount_t *mp, sam_fid_t *sam_fidp);

/*
 * ----- Module Loading/Unloading and Autoconfiguration declarations
 * vfsops contains the filsystem entry points.
 * Operations supported on virtual file system.
 */

vfsops_t *samfs_vfsopsp;


/*
 * ----- samfs_mount - Mount the SAM filesystem.
 * Mounts the SAM filesystem.
 */

int				/* ERRNO if error, 0 if successful. */
samfs_mount(
	vfs_t *vfsp,		/* VFS pointer for SAMFS. */
	vnode_t *vp,		/* Vnode pointer for mount point. */
	struct mounta *pp,	/* Pointer to mount parameters. */
	cred_t *credp)		/* Credentials pointer */
{
	int i, fs_count, npart;
	int filemode;
	int error = 0, err_line = 0;
	boolean_t device_open_err = FALSE;
	sam_id_t id;
	uname_t fsname;
	pathname_t pn, *pnp = NULL;
	sam_node_t *rip;
	sam_mount_t *mp = NULL;
	sam_mount_info_t *mip = NULL;
	struct sam_fs_part *cur_fsp;
	struct samdent *fs_dent;
	int needtrans = 0;
	char fsclean;

	TRACE(T_SAM_MOUNT, NULL, (sam_tr_t)vfsp, (sam_tr_t)vp, 0);
	/*
	 * Check to ensure init process finished.
	 */
	if (samgt.fstype == 0) {
		cmn_err(CE_WARN, "SAM-QFS: init incomplete, mount failed");
		return (EFAULT);
	}

	mutex_enter(&samgt.global_mutex);
	samgt.num_fs_mounting++;
	mutex_exit(&samgt.global_mutex);

	bzero((void *)fsname, sizeof (uname_t));

	if (secpolicy_fs_mount(credp, vp, vfsp)) {
		err_line = __LINE__;
		error = EPERM;		/* Check for superuser */
		goto done;
	}

	/*
	 * Copyin the mount arguments.
	 */
	if ((pp->dataptr == NULL) || (pp->datalen <= 0) ||
	    (pp->datalen > sizeof (sam_mount_info_t))) {
		cmn_err(CE_WARN, "SAM-QFS: Mount arguments invalid <%p/%x>",
		    (void *)pp->dataptr, pp->datalen);
		err_line = __LINE__;
		error = EINVAL;
		goto done;
	}
	mip = (sam_mount_info_t *)kmem_zalloc(sizeof (sam_mount_info_t),
	    KM_SLEEP);
	if (copyin(pp->dataptr, (char *)mip, pp->datalen)) {
		err_line = __LINE__;
		error = EFAULT;
		goto done;
	}

	bcopy(mip->params.fi_name, fsname, sizeof (uname_t));
	cmn_err(CE_NOTE, "SAM-QFS: %s: Initiated mount filesystem", fsname);

	if (vp->v_type != VDIR) {
		err_line = __LINE__;
		error = ENOTDIR;	/* Verify is a directory */
		goto done;
	}

	if (pp->flags & MS_REMOUNT) {
		cmn_err(CE_WARN, "SAM-QFS: Remount not supported");
		err_line = __LINE__;
		error = EINVAL;		/* Remount not supported */
		goto done;
	}

	mutex_enter(&vp->v_lock);	/* Mount point already mounted? */
	if (!(pp->flags & MS_OVERLAY) &&
	    ((vp->v_count != 1) || (vp->v_flag & VROOT))) {
		mutex_exit(&vp->v_lock);
		err_line = __LINE__;
		error = EBUSY;
		goto done;
	}
	mutex_exit(&vp->v_lock);

#ifdef METADATA_SERVER
	/*
	 * Check licensing - if shared but not licensed, disallow the mount.
	 * Don't check licensing on client-only host.
	 */
	if ((mip->params.fi_config1 & MC_SHARED_FS) &&
	    !samgt.license.license.lic_u.b.shared_fs) {
		err_line = __LINE__;
		error = ENOTSUP;
		goto done;
	}

	/*
	 * Check licensing - if worm_capable but not licensed, disallow the
	 * mount.
	 */
	if ((mip->params.fi_config & MT_ALLWORM_OPTS) &&
	    !samgt.license.license.lic_u.b.WORM_fs) {
		err_line = __LINE__;
		error = ENOTSUP;
		goto done;
	}
#endif	/* METADATA_SERVER */

	/*
	 * Set the mount table for this mount.
	 */
	if ((fs_count = mip->params.fs_count) <= 0) {
		cmn_err(CE_WARN,
		    "SAM-QFS: Mount parameters invalid, fs_count = %x",
		    fs_count);
		err_line = __LINE__;
		error = EINVAL;
		goto done;
	}
	mutex_enter(&samgt.global_mutex);
	for (mp = samgt.mp_list; mp != NULL; mp = mp->ms.m_mp_next) {
		if (strncmp(mp->mt.fi_name, mip->params.fi_name,
		    sizeof (mp->mt.fi_name)) == 0) {
			if (fs_count != mp->mt.fs_count) {
				mutex_exit(&samgt.global_mutex);
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: Mount: device "
				    "count %d does not match mcf %d",
				    mip->params.fi_name, fs_count,
				    mp->mt.fs_count);
				mp = NULL;
				err_line = __LINE__;
				error = ECANCELED;
				goto done;
			}
			break;
		}
	}
	if (mp == NULL) {
		mutex_exit(&samgt.global_mutex);
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: Cannot find block special device",
		    mip->params.fi_name);
		err_line = __LINE__;
		error = ECANCELED;
		goto done;
	}

	/*
	 * Is this file system already mounted?  (Or are we trying already?)
	 * Or are we unmounting?
	 */

	mutex_enter(&mp->ms.m_waitwr_mutex);
	if (mp->mt.fi_status & (FS_MOUNTED | FS_MOUNTING |
	    FS_UMOUNT_IN_PROGRESS)) {
		mutex_exit(&mp->ms.m_waitwr_mutex);
		mutex_exit(&samgt.global_mutex);
		mp = NULL;
		err_line = __LINE__;
		error = EBUSY;
		goto done;
	}

	/*
	 * Clear the mount instance mount parameters.
	 */
	mip->params.fi_status = mp->mt.fi_status &
	    (FS_FSSHARED|FS_SRVR_BYTEREV);
	strncpy(mip->params.fi_server, mp->mt.fi_server,
	    sizeof (mip->params.fi_server));
	bzero((char *)&mp->mi,
	    (sizeof (sam_mt_instance_t) +
	    (sizeof (struct samdent) * (fs_count-1))));
	mp->mt = mip->params;
	mp->mt.fi_status |= FS_MOUNTING;
	mutex_exit(&mp->ms.m_waitwr_mutex);

	mp->ms.m_involuntary = 0;
	mp->ms.m_sr_ino_seqno = 1;
	mp->ms.m_sr_ino_gen = lbolt;

	for (i = 0; i < SAM_MAX_DAU; i++) {
		int j;

		for (j = 0; j < SAM_NO_FB_ARRAY; j++) {
			sam_mutex_init(&mp->mi.m_fb_pool[i].array[j].fb_mutex,
			    NULL, MUTEX_DEFAULT, NULL);
		}
	}
	sam_mutex_init(&mp->mi.m_sblk_mutex, NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&mp->mi.m_lease_mutex, NULL, MUTEX_DEFAULT, NULL);
	mp->mi.m_sr_lease_chain.forw =
	    (sam_lease_ino_t *)(void *)&mp->mi.m_sr_lease_chain;
	mp->mi.m_sr_lease_chain.back =
	    (sam_lease_ino_t *)(void *)&mp->mi.m_sr_lease_chain;
	mp->mi.m_cl_lease_chain.forw = &mp->mi.m_cl_lease_chain;
	mp->mi.m_cl_lease_chain.back = &mp->mi.m_cl_lease_chain;
	mp->mi.m_cl_lease_chain.node = NULL;
	mp->mi.m_vfsp = vfsp;		/* Mounted */
	vfsp->vfs_data = (caddr_t)mp;
	mp->mi.m_cl_leasenext = mp->mi.m_cl_leasecnt = 0;
	mp->mi.m_sr_leasenext = mp->mi.m_sr_leasecnt = 0;

	/*
	 * Read-Only if so desired
	 */
	if (pp->flags & MS_RDONLY) {
		vfsp->vfs_flag |= VFS_RDONLY;
		vfs_setmntopt(vfsp, MNTOPT_RO, NULL, 0);
	}

	if (pp->flags & MS_NOSUID) {
		vfs_setmntopt(vfsp, MNTOPT_NOSUID, NULL, 0);
	}

	mp->mi.m_fsact_buf = NULL;
	mp->ms.m_fsev_buf = NULL;
	if (!SAM_IS_STANDALONE(mp)) {
		sam_arfind_init(mp);
		if (mp->mt.fi_config1 & MC_SAM_DB) {
			(void) sam_event_init(mp, 0);
		}
	}
	mutex_exit(&samgt.global_mutex);

	/*
	 * Get directory pathname.
	 */
	if ((error = pn_get(pp->dir, UIO_USERSPACE, &pn))) {
		err_line = __LINE__;
		goto done;
	}
	pnp = &pn;

	/*
	 * Move mount device into into the mount table. Then open
	 * the block special devices for this filesystem.
	 */
	for (i = 0, cur_fsp = &mip->part[0], fs_dent = &mp->mi.m_fs[0];
	    i < mp->mt.fs_count; i++, cur_fsp++, fs_dent++) {
		fs_dent->part = *cur_fsp;
	}
	filemode = (mp->mi.m_vfsp->vfs_flag & VFS_RDONLY) ?
	    FREAD : FREAD | FWRITE;
	if ((error = sam_getdev(mp, 0, filemode, &npart, credp))) {
		TRACE(T_SAM_MNT_ERR, vfsp, 12, (sam_tr_t)vp, error);
		err_line = __LINE__;
		if (error != ENXIO || npart < 1) {
			goto done;
		}
		device_open_err = TRUE;
	}

	/*
	 * Read superblock and set up fields, rewrite if not read only.
	 * Note: The device_open_err flag notes that an error occurred
	 *	in sam_getdev(), but at least one superblock should be
	 *	available. We check the super block to determine if a >1TB
	 *	LUN was found, and a device error was returned due to those
	 *	limitations. In this case, sam_mount_fs() should return
	 *	EFBIG, and we should give a more meaningful error message.
	 */
	if ((error = sam_mount_fs(mp))) {
		TRACE(T_SAM_MNT_ERR, vfsp, 13, (sam_tr_t)vp, error);
		err_line = __LINE__;
		if (error == EFBIG) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s:"
			    " Attempted to mount a filesystem "
			    "with > 1TB partition",
			    mp->mt.fi_name);
		}
		goto done;
	} else if (device_open_err) {
		error = ENXIO;
		err_line = __LINE__;
		goto done;
	}

	sam_tracing = mp->mt.fi_config & MT_TRACE;

	/*
	 * Configure the device IDs associated with the FS.  These are
	 * returned particularly in sam_getattr_vn() to NFS, where they
	 * have some consequences.
	 */
	if (mp->mt.fi_config & MT_CDEVID) {
		vfsp->vfs_dev = makedevice(samgt.samioc_major, mp->mt.fi_eq);
	} else {
		vfsp->vfs_dev = mp->mi.m_fs[0].dev;
	}

	/*
	 * Configure the FS IDs associated with the FS.  These are returned
	 * in various places, particularly sam_getattr_vn() and statvfs().
	 * Historically, the equivalent settings are:
	 *
	 * SAM-QFS		vfsp->vfs_dev		vfsp->vfs_fsid
	 *
	 *	4.0, 4.1, 4.2:
	 *	  gfsid*	m_fs[0].dev		sbp->info.sb.fs_id
	 *	  nogfsid	m_fs[0].dev	cmpldev(vfs_dev) + samfs.fstype
	 *
	 *	4.3, 4.4, 4.5:
	 *	  gfsid*	m_fs[0].dev		sbp->info.sb.fs_id
	 *	  nogfsid	(samioc_major, fi_eq)	(vfs_dev, samfs.fstype)
	 *
	 *  4.6:
	 *	  gfsid*				sbp->info.sb.fs_id
	 *	  nogfsid				(vfs_dev, samfs.fstype)
	 *	  cdevid*		(samioc_major, fi_eq)
	 *	  nocdevid		m_fs[0].dev
	 *
	 *			'*' indicates default value
	 */
	if (mp->mt.fi_config & MT_GFSID) {
		vfsp->vfs_fsid = mp->mi.m_sbp->info.sb.fs_id;
	} else {
		vfs_make_fsid(&vfsp->vfs_fsid, vfsp->vfs_dev, samgt.fstype);
	}

	vfsp->vfs_flag |= VFS_NOTRUNC;
	vfsp->vfs_bsize = SAM_DEV_BSIZE;
	vfsp->vfs_fstype = samgt.fstype;
	vfsp->vfs_bcount = 0;

	/*
	 * OSD FS specific Initialization
	 */
	mp->mi.m_osdt_lun = ~0ULL;	/* 0 is a valid LUN */
	mp->mi.m_osdfs_root = NULL;
	mp->mi.m_osdfs_part = NULL;

	/*
	 * Get .inodes, root, & .blocks (hidden) inodes.
	 */
	for (i = SAM_INO_INO; i <= SAM_BLK_INO; i++) {
		id.ino = i;
		id.gen = i;
		if ((error = sam_get_ino(vfsp, IG_EXISTS, &id, &rip))) {
			TRACE(T_SAM_MNT_ERRID, vfsp, 14, i, error);
			(void) sam_umount_ino(vfsp, TRUE);
			err_line = __LINE__;
			goto done;
		}
		switch (i) {
		case SAM_INO_INO:
			mp->mi.m_inodir = rip;
			mp->mi.m_inodir->flags.bits |= SAM_DIRECTIO;
			break;
		case SAM_ROOT_INO:
			if ((error = sam_verify_root(rip, mp))) {
				err_line = __LINE__;
				goto done;
			}
			mp->mi.m_vn_root = SAM_ITOV(rip);
			break;
		case SAM_BLK_INO:
			mp->mi.m_inoblk = rip;
			break;
		}
	}

	if (mp->mt.fi_type == DT_META_OBJ_TGT_SET) {
		/*
		 * Get the default Root Object and default Partition 0 inodes.
		 */
		id.ino = SAM_OBJ_ROOT_INO;
		id.gen = SAM_OBJ_ROOT_INO;
		if ((error = sam_get_ino(vfsp, IG_EXISTS, &id, &rip))) {
			TRACE(T_SAM_MNT_ERRID, vfsp, 14, id.ino, error);
			(void) sam_umount_ino(vfsp, TRUE);
			err_line = __LINE__;
			goto done;
		} else {
			mp->mi.m_osdfs_root = rip;
		}

		id.ino = SAM_OBJ_PAR0_INO;
		id.gen = SAM_OBJ_PAR0_INO;
		if ((error = sam_get_ino(vfsp, IG_EXISTS, &id, &rip))) {
			TRACE(T_SAM_MNT_ERRID, vfsp, 14, id.ino, error);
			(void) sam_umount_ino(vfsp, TRUE);
			err_line = __LINE__;
			goto done;
		} else {
			mp->mi.m_osdfs_part = rip;
		}
	}

#ifdef METADATA_SERVER
	/*
	 * The .inodes file i-node contains the number of the next supposedly
	 * free i-node.  If that i-node is at the end of the list, then it is
	 * probable that ialloc() found a bad free i-node and forced the
	 * next free i-node number to the end of the i-node list, with the
	 * reasonable expectation that the .inodes file will be dynamically
	 * extended to accomodate the new free i-node.  The cost is simply
	 * some number of lost i-nodes that will be recovered during the next
	 * filesystem check.
	 *
	 * One problem is that, when i-nodes are pre-allocated, if ialloc()
	 * forces the next free i-node number to the end of the i-node list,
	 * it effectively eliminates all available i-nodes from the system,
	 * because the pre-allocated i-node list is not intended to be
	 * expandable.
	 *
	 * Checking (and re-building) the free list at mount time is a temporary
	 * approach to patch things, until we find the cause of i-node free
	 * list corruption.
	 *
	 * One tricky part of this approach is that mkfs has never created
	 * a free list chain for pre-allocated i-nodes.  It just allocates
	 * a sufficient number of blocks to .inodes, which holds un-initialized
	 * (un-chained) pre-allocated i-nodes.  So, it's impossible to tell
	 * the difference between a bad pre-allocated i-node free-list chain
	 * and a free-list that contains an uninitialized pre-allocated
	 * i-node.  All we do know is that the i-node # 9 (not a user
	 * available i-node) is the first free i-node in a new mkfs'ed
	 * filesystem.  We could key off of that assumption to avoid
	 * re-building the whole freelist at first mount time.  But that
	 * involves risk, because a freelist with first i-node #9 may
	 * really be corrupted.  Also, that wouldn't prevent re-building
	 * the freelist on any subsequent mount, because the freelist may
	 * point to un-initialized i-nodes.  So, we might as well do here
	 * what mkfs SHOULD have historically done - build an intact
	 * pre-allocated i-node freelist.
	 */

	/* Validate free list if not mounting read-only */
	if ((pp->flags & MS_RDONLY) == 0 && !SAM_IS_SHARED_CLIENT(mp)) {
		/* Validate .inodes free list (first 64 i-nodes only) */
		if ((error =
		    sam_inode_freelist_check(mp, SAM_ILIST_PREALLOC,
		    64)) != 0) {
			int result;

			if ((error = sam_inode_freelist_build(mp, -1,
			    &result)) != 0) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: Repair i-node freelist failed"
				    " - RUN FILESYSTEM CHECK",
				    mp->mt.fi_name);
				return (error);
			}
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: Updated i-node freelist "
			    "with %d i-nodes",
			    mp->mt.fi_name, result);
		}
	}
#endif /* METADATA_SERVER */

	/*
	 * Start up block allocator thread and inode allocator threads.
	 */
	if ((error = sam_start_threads(mp))) {
		err_line = __LINE__;
		goto done;
	}

	if (LQFS_CAPABLE(mp)) {
		/*
		 * Chicken and egg problem. The superblock may have deltas
		 * in the log.  So after the log is scanned we reread the
		 * superblock. We guarantee that the fields needed to
		 * scan the log will not be in the log.
		 */

		/*
		 * LUFS comments say that we should trust the value of
		 * qfs_clean only when fs_state == FSOKAY - fs_time ,
		 * where FSOKAY is the constant value 0x7c269d38....
		 * For QFS, implicitly trust qfs_clean for now.
		 */
		if (LQFS_GET_LOGBNO(mp) &&
		    (LQFS_GET_FS_CLEAN(mp) == FSLOG)) {
			error = lqfs_snarf(mp, mp,
			    (vfsp->vfs_flag & VFS_RDONLY));
			if (error) {
				/*
				 * Allow a ro mount to continue even if the
				 * log cannot be processed - yet.
				 */
				if (!(vfsp->vfs_flag & VFS_RDONLY)) {
					cmn_err(CE_WARN, "Error accessing QFS "
					"log for %s; Please run fsck(1M)", pnp);
					goto done;
				}
			}
			/*
			 * LUFS works with the superblock in a buffer to control
			 * when the superblock is written to disk.
			 * lqfs_snarf() may have updated the contents of the
			 * superblock.  LUFS re-reads it here.  LQFS references
			 * the superblock through mp->mi.m_sbp, and uses
			 * sam_sbwrite() to write it to disk.
			 */
		}
		if (vfsp->vfs_flag & VFS_RDONLY) {
			fsclean = LQFS_GET_FS_CLEAN(mp);
			if ((fsclean == FSCLEAN) ||
			    (fsclean == FSSTABLE) ||
			    (fsclean == FSLOG)) {
				LQFS_SET_FS_CLEAN(mp, FSSTABLE);
			} else {
				/*
				 * The filesystem is either active, bad, or
				 * legacy (pre-journal).  If active, the mount
				 * would have already failed.  If bad, fsclean
				 * is non-zero.  If legacy, don't do anything
				 * special.
				 */
				if (fsclean != 0) {
					LQFS_SET_FS_CLEAN(mp, FSBAD);
				}
			}
		} else {
			TRANS_DOMATAMAP(mp);

			if ((TRANS_ISERROR(mp)) ||
			    ((LQFS_GET_FS_CLEAN(mp) == FSLOG) &&
			    !TRANS_ISTRANS(mp))) {
				LQFS_SET_LOGP(mp, NULL);
				LQFS_SET_DOMATAMAP(mp, 0);
				error = ENOSPC;
				goto done;
			}

			fsclean = LQFS_GET_FS_CLEAN(mp);
			if (fsclean == FSCLEAN ||
			    fsclean == FSSTABLE ||
			    fsclean == FSLOG) {
				LQFS_SET_FS_CLEAN(mp, FSSTABLE);
			} else {
				/*
				 * The filesystem is either active, bad, or
				 * legacy (pre-journal).  If active, the mount
				 * would have already failed.  If bad, fsclean
				 * is non-zero.  If legacy, don't do anything
				 * special.
				 */
				if (fsclean != 0) {
					error = ENOSPC;
					goto done;
				}
			}

			fsclean = LQFS_GET_FS_CLEAN(mp);
			if (fsclean == FSSTABLE && TRANS_ISTRANS(mp)) {
				LQFS_SET_FS_CLEAN(mp, FSLOG);
			}
		}
		TRANS_MATA_MOUNT(mp);
		needtrans = 1;

		if (TRANS_ISTRANS(mp)) {
			/* Mark the fs as unrolled */
			LQFS_SET_FS_ROLLED(mp, FS_NEED_ROLL);

			if ((pp->flags & MS_RDONLY) == 0) {
				TRANS_SBWRITE(mp, TOP_MOUNT);
			}
		}
	}

	pn_free(&pn);

	/*
	 * Notify sam-fsd of successful mount.
	 */
	{
		struct sam_fsd_cmd cmd;

		cmd.cmd = FSD_mount;
		bcopy(mp->mt.fi_name, cmd.args.mount.fs_name,
		    sizeof (cmd.args.mount.fs_name));
		cmd.args.mount.init = mp->mi.m_sbp->info.sb.init;
		cmd.args.mount.start_samdb = FALSE;
		if (mp->mt.fi_config1 & MC_SAM_DB) {
			cmd.args.mount.start_samdb = TRUE;
		}
		(void) sam_send_scd_cmd(SCD_fsd, &cmd, sizeof (cmd));
	}

	/*
	 * Delay reporting of initial watermark state until sam-amld has had
	 * a chance to initialize the mount params from above.
	 */
	if (!SAM_IS_STANDALONE(mp)) {
		sam_report_initial_watermark(mp);
	}

	error = 0;
done:
	mutex_enter(&samgt.global_mutex);
	if (mip != NULL) {
		kmem_free((void *)mip, sizeof (sam_mount_info_t));
	}
	if (error == 0) {
		mutex_enter(&mp->ms.m_waitwr_mutex);
		mp->mt.fi_status |= FS_MOUNTED;
		samgt.num_fs_mounted++;
		mp->mt.fi_status &= ~FS_MOUNTING;
		mutex_exit(&mp->ms.m_waitwr_mutex);
		sam_send_shared_mount(mp);
		/*
		 * Initialize quota-related data structures.  Note that this
		 * is required for all potential metadata servers for failover.
		 * We do this here so that the client NAME_lookup can complete.
		 */
		sam_quota_init(mp);
		TRACE(T_SAM_MNT_RET, vfsp, (sam_tr_t)mp,
		    (sam_tr_t)mp->mi.m_vn_root, 0);
		cmn_err(CE_NOTE,
		    "SAM-QFS: %s: Completed mount filesystem", fsname);
		if (mp->mt.fi_type == DT_META_OBJ_TGT_SET) {
			sam_objctl_create(mp);
		}
	} else {
		if (mp) {
			if (needtrans) {
				TRANS_MATA_UMOUNT(mp);
			}
			sam_stop_threads(mp);
			if (TRANS_ISTRANS(mp)) {
				lqfs_unsnarf(mp);
			}
		}
		sam_cleanup_mount(mp, pnp, credp);
		if (mp) {
			mutex_enter(&mp->ms.m_waitwr_mutex);
			mp->mt.fi_status &= ~(FS_MOUNTED|FS_MOUNTING);
			mutex_exit(&mp->ms.m_waitwr_mutex);
			*mp->mt.fi_mnt_point = '\0';
			sam_send_shared_mount(mp);
		}
		TRACE(T_SAM_MNT_ERRLN, NULL, err_line, error, 0);
		cmn_err(CE_NOTE,
		    "SAM-QFS: %s: Aborted mount filesystem", fsname);
	}
	samgt.num_fs_mounting--;
	mutex_exit(&samgt.global_mutex);
	return (error);
}


/*
 * ----- samfs_umount - Unmount the SAM filesystem.
 * Unmounts the SAM filesystem. Close the block special devices.
 */

/* ARGSUSED1 */
int				/* ERRNO if error, 0 if successful. */
samfs_umount(
	vfs_t *vfsp,		/* Vfs pointer for SAMFS. */
	int fflag,		/* Forced umount flag */
	cred_t *credp)		/* Credentials pointer */
{
	sam_mount_t *mp;
	struct sam_fsd_cmd cmd;
	uname_t fsname;
	int version;
	int e, error = 0;

	mp = (sam_mount_t *)(void *)vfsp->vfs_data;
	TRACE(T_SAM_UMOUNT, vfsp, (sam_tr_t)mp, (sam_tr_t)mp->mi.m_vn_root,
	    mp->mi.m_vn_root->v_count);

	if (secpolicy_fs_unmount(credp, vfsp)) {
		TRACE(T_SAM_UMNT_RET, vfsp, 0, (sam_tr_t)mp, EPERM);
		return (EPERM);
	}

	mutex_enter(&mp->ms.m_waitwr_mutex);

	/*
	 * If file system is not mounted, or is being mounted, or is already
	 * being unmounted, return busy. If file system is failing over, return
	 * busy.
	 */
	if (!(mp->mt.fi_status & FS_MOUNTED) ||
	    (mp->mt.fi_status & (FS_MOUNTING | FS_UMOUNT_IN_PROGRESS))) {
		mutex_exit(&mp->ms.m_waitwr_mutex);
		TRACE(T_SAM_UMNT_RET, vfsp, 10, (sam_tr_t)mp, EBUSY);
		return (EBUSY);
	} else if (mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING)) {
		mutex_exit(&mp->ms.m_waitwr_mutex);
		cmn_err(CE_NOTE,
	"SAM-QFS: %s: Umount ignored - failover in progress (%x)",
		    mp->mt.fi_name, mp->mt.fi_status);
		return (EBUSY);
	} else if (SAM_IS_SHARED_SERVER(mp) &&
	    !(fflag & MS_FORCE) &&
	    sam_clients_mounted(mp)) {
		mutex_exit(&mp->ms.m_waitwr_mutex);
		TRACE(T_SAM_UMNT_RET, vfsp, 11, (sam_tr_t)mp, EBUSY);
		return (EBUSY);
	} else if ((mp->mt.fi_status & FS_OSDT_MOUNTED) &&
	    !(fflag & MS_FORCE)) {
		mutex_exit(&mp->ms.m_waitwr_mutex);
		cmn_err(CE_NOTE,
		"SAM-QFS: %s: Umount ignored - OSD Target Service is still "
		"active.", mp->mt.fi_name);
		return (EBUSY);
	}

	TRACE(T_SAM_UMNT_RET, vfsp, 1, (sam_tr_t)mp, 0);
	version = mp->mt.fi_version;
	bzero((void *)fsname, sizeof (uname_t));
	bcopy(mp->mt.fi_name, fsname, sizeof (uname_t));
	cmn_err(CE_NOTE, "SAM-QFS: %s: Initiated unmount filesystem: vers %d",
	    fsname, version);

	if (!SAM_IS_STANDALONE(mp)) {
		/*
		 * Notify sam-fsd of attempt to umount, to stop releaser.
		 */
		cmd.cmd = FSD_preumount;
		bcopy(mp->mt.fi_name, cmd.args.umount.fs_name,
		    sizeof (cmd.args.umount.fs_name));
		cmd.args.umount.umounted = 0;
		(void) sam_send_scd_cmd(SCD_fsd, &cmd, sizeof (cmd));

		sam_arfind_umount(mp); /* Tell arfind we're trying to unmount */
	}

	if (mp->mt.fi_type == DT_META_OBJ_TGT_SET) {
		sam_objctl_destroy(mp);
	}

	/*
	 * Hold on to the FS to avoid a premature free.
	 */
	SAM_VFS_HOLD(mp);

	mp->mt.fi_status |= FS_UMOUNT_IN_PROGRESS;
	mutex_exit(&mp->ms.m_waitwr_mutex);

	if (!SAM_IS_STANDALONE(mp)) {
		int i;

		/*
		 * Wait a while for releaser to stop.
		 */
		for (i = 0; i < 100; i++) {
			if ((mp->mt.fi_status & FS_RELEASING) == 0) {
				break;
			}
			delay(hz/4);
		}
	}

	TRACE(T_SAM_UMNT_RET, vfsp, 2, (sam_tr_t)mp, 0);
	if ((error = sam_unmount_fs(mp, fflag, SAM_UNMOUNT_FS)) != 0) {
		if ((fflag & MS_FORCE) == 0) {
			goto busy;
		}
	}

	/*
	 * Clean up any quota stuff
	 */
	sam_quota_fini(mp);
	TRACE(T_SAM_UMNT_RET, vfsp, 17, (sam_tr_t)mp, 0);

	if (LQFS_CAPABLE(mp)) {
		if (!(mp->mt.fi_mflag & MS_RDONLY)) {
			int error;

			if (TRANS_ISTRANS(mp)) {
				ml_unit_t	*ul = LQFS_GET_LOGP(mp);
				mt_map_t	*mtm = ul->un_deltamap;

				if (ul->un_flags & LDL_NOROLL) {
					ASSERT(mtm->mtm_nme == 0);
				} else {
					/*
					 * Might need to check if T_DONTBLOCK
					 * is already set here for LQFS
					 */
					curthread->t_flag |= T_DONTBLOCK;
					TRANS_BEGIN_SYNC(mp, TOP_COMMIT_FLUSH,
					    TOP_COMMIT_SIZE, error);
					if (!error) {
						TRANS_END_SYNC(mp, error,
						    TOP_COMMIT_FLUSH,
						    TOP_COMMIT_SIZE);
					}
					curthread->t_flag &= ~T_DONTBLOCK;
					logmap_roll_dev(mp->mi.m_log);
				}
			}
			bflush(vfsp->vfs_dev);
			LQFS_SET_FS_ROLLED(mp, FS_ALL_ROLLED);

			TRANS_SBUPDATE(mp, vfsp, TOP_SBUPDATE_UNMOUNT);

			/*
			 * push this last transaction
			 */
			curthread->t_flag |= T_DONTBLOCK;
			TRANS_BEGIN_SYNC(mp, TOP_COMMIT_UNMOUNT,
			    TOP_COMMIT_SIZE, error);
			if (!error) {
				TRANS_END_SYNC(mp, error, TOP_COMMIT_UNMOUNT,
				    TOP_COMMIT_SIZE);
			}
			curthread->t_flag &= ~T_DONTBLOCK;
		}

		TRANS_MATA_UMOUNT(mp);
		lqfs_unsnarf(mp);	/* Release the in-memory LQFS structs */
	}

	/*
	 * Notify fsalog daemon of successful umount
	 */
	if (!SAM_IS_STANDALONE(mp)) {
		if (mp->ms.m_fsev_buf) {
			sam_event_umount(mp);
		}
	}

	/*
	 * Unhash last 3 inodes and inactivate them.
	 */
	if ((e = sam_umount_ino(vfsp, fflag)) != 0) {
		TRACE(T_SAM_UMNT_RET, vfsp, 7, (sam_tr_t)mp, e);
		if ((fflag & MS_FORCE) == 0) {
			goto busy;
		}
	}
	if (e) {
		error = e;
	}
	TRACE(T_SAM_UMNT_RET, vfsp, 8, (sam_tr_t)mp, error);

	/*
	 * Notify sam-fsd of successful umount.
	 */
	cmd.cmd = FSD_umount;
	bcopy(mp->mt.fi_name, cmd.args.umount.fs_name,
	    sizeof (cmd.args.umount.fs_name));
	cmd.args.umount.umounted = 1;
	(void) sam_send_scd_cmd(SCD_fsd, &cmd, sizeof (cmd));

	/*
	 * Finish cleaning up arfind; close our backing store devices.
	 */
	if (!SAM_IS_STANDALONE(mp)) {
		sam_arfind_fini(mp);
		if (mp->ms.m_fsev_buf) {
			sam_event_fini(mp);
		}
	}

	/*
	 * Flush and close devices.
	 */
	sam_cleanup_mount(mp, NULL, credp);

	TRACE(T_SAM_UMNT_RET, vfsp, 9, (sam_tr_t)mp, error);

	mutex_enter(&samgt.global_mutex);

	if (error && (fflag & MS_FORCE)) {
		sam_mount_t *nmp;		/* new mount info structure */
		uint32_t sv_status = mp->mt.fi_status;

		/*
		 * Umount wasn't clean.  We can't free up the mount structure;
		 * we must label it stale and clean it up if/when the last
		 * reference to it is freed.  After marking it stale, put it
		 * on the stale list and create a fresh one, so that the FS
		 * can be mounted again.
		 */
		sam_stale_mount_info(mp);

		/*
		 * Make sure that the share daemon didn't sneak out and back in
		 * on us, which would have dropped its VFS_HOLD in favor of a
		 * deferred hold.  sam_find_filesystem() and find_mount_point()
		 * both lock on mp->ms.m_waitwr_mutex, but don't check for
		 * unmounted/unmounting flags.  The FS is staled now, so either
		 * of these will now find the new FS rather than the old.
		 */
		if (SAM_IS_SHARED_FS(mp)) {
			SAM_SHARE_DAEMON_CNVT_FS(mp);
		}

		TRACE(T_SAM_UMNT_RET, vfsp, 101, (sam_tr_t)mp, error);
		vfsp->vfs_flag |= VFS_UNMOUNTED;

		nmp = sam_dup_mount_info(mp);
		/*
		 * Don't need m_waitwr_lock -- still holding global lock
		 */
		nmp->mt = nmp->orig_mt;
		nmp->mt.fi_status = sv_status & FS_FSSHARED;
		samgt.num_fs_mounted--;

		/*
		 * Append ':umnt-f' to FS name, so that any console messages
		 * arriving re: the unmounted filesystem are distinguishable
		 * from the active FS (if any).
		 */
		strlcat(&mp->mt.fi_name[0], ":umnt-f", sizeof (mp->mt.fi_name));
		TRACE(T_SAM_UMNT_RET, vfsp, 102, (sam_tr_t)mp, error);
		mp->mt.fi_status &= ~(FS_MOUNTED | FS_UMOUNT_IN_PROGRESS);
		cv_broadcast(&mp->ms.m_waitwr_cv);
		mutex_exit(&samgt.global_mutex);
		TRACE(T_SAM_UMNT_RET, vfsp, 103, (sam_tr_t)mp, error);

		/*
		 * Update the server with our status.  This should probably
		 * go away, esp. if/when we provide a more direct mechanism
		 * for allowing the kernel to tell sam-fsd that it should
		 * start a sam-sharefsd daemon for the FS.
		 */
		if (SAM_IS_SHARED_FS(mp) && !SAM_IS_SHARED_SERVER(mp)) {
			sam_send_shared_mount(mp);
			delay(hz);
		}
		TRACE(T_SAM_UMNT_RET, vfsp, 104, (sam_tr_t)mp, error);

		if (SAM_IS_SHARED_FS(mp)) {
			/*
			 * The FS's mount structure has been respun; and the
			 * existing share daemon is tied to the old one.  Tell
			 * sam-fsd to kill/abandon the old share daemon.  Use
			 * the original FS name.
			 */
			cmd.cmd = FSD_stalefs;
			bcopy(nmp->mt.fi_name, cmd.args.umount.fs_name,
			    sizeof (cmd.args.umount.fs_name));
			cmd.args.umount.umounted = 1;
			(void) sam_send_scd_cmd(SCD_fsd, &cmd, sizeof (cmd));
		}
		cmn_err(CE_NOTE,
		    "SAM-QFS: %s: Completed unmount filesystem: vers %d",
		    fsname, version);
	} else {
		*mp->mt.fi_mnt_point = '\0';

		mp->orig_mt.fi_status =
		    mp->mt.fi_status & (FS_FSSHARED|FS_SRVR_BYTEREV);
		strncpy(mp->orig_mt.fi_server, mp->mt.fi_server,
		    sizeof (mp->orig_mt.fi_server));

		mp->mt = mp->orig_mt; /* Restore original parameter settings */

		mutex_enter(&mp->ms.m_waitwr_mutex);
		mp->mt.fi_status &= ~(FS_MOUNTED | FS_UMOUNT_IN_PROGRESS);
		mutex_exit(&mp->ms.m_waitwr_mutex);

		samgt.num_fs_mounted--;

		mutex_exit(&samgt.global_mutex);

		sam_send_shared_mount(mp);
		cmn_err(CE_NOTE,
		    "SAM-QFS: %s: Completed unmount filesystem: vers %d",
		    fsname, version);
	}
	SAM_VFS_RELE(mp);
	return (0);

busy:
	if (sam_start_threads(mp)) {
		vfsp->vfs_flag |= VFS_RDONLY;
		mp->mt.fi_mflag |= MS_RDONLY;
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: Cannot create FS threads, "
		    "switching to read-only",
		    mp->mt.fi_name);
	}
	sam_quota_init(mp);
	/*
	 * Notify sam-fsd of unsuccessful umount.
	 */
	cmd.cmd = FSD_umount;
	bcopy(mp->mt.fi_name, cmd.args.umount.fs_name,
	    sizeof (cmd.args.umount.fs_name));
	cmd.args.umount.umounted = 0;
	(void) sam_send_scd_cmd(SCD_fsd, &cmd, sizeof (cmd));
	cmn_err(CE_WARN, "SAM-QFS: %s: Aborted unmount filesystem: vers %d",
	    fsname, version);
	mutex_enter(&mp->ms.m_waitwr_mutex);
	mp->mt.fi_status &= ~FS_UMOUNT_IN_PROGRESS;
	cv_broadcast(&mp->ms.m_waitwr_cv);
	mutex_exit(&mp->ms.m_waitwr_mutex);
	SAM_VFS_RELE(mp);
	return (EBUSY);
}


/*
 * ----- samfs_root - Return root vnode.
 * Return root vnode for this instance of mount.
 */

int				/* ERRNO if error, 0 if successful. */
samfs_root(
	vfs_t *vfsp,		/* Vfs pointer for SAMFS. */
	vnode_t **vpp)		/* Pointer pointer to root vnode (returned) */
{
	sam_mount_t *mp;

	if ((mp = (sam_mount_t *)(void *)vfsp->vfs_data) != NULL) {
		VN_HOLD(mp->mi.m_vn_root);
		*vpp = mp->mi.m_vn_root;
	} else {
		*vpp = NULL;
	}
	TRACE(T_SAM_ROOT, vfsp, (sam_tr_t)*vpp, (sam_tr_t)(*vpp)->v_count, 0);
	return (0);
}


/*
 * ----- samfs_statvfs - Return file system status.
 * Return sblk status for this instance of SAMFS mount.
 */

int				/* ERRNO if error, 0 if successful. */
samfs_statvfs(
	vfs_t *vfsp,		/* Vfs pointer for SAMFS. */
	sam_statvfs_t *sp)	/* Statvfs buffer pointer (status returned) */
{
	sam_mount_t *mp;
	struct sam_sblk *sblk = NULL;
	int error = 0;

	if (vfsp->vfs_flag & VFS_UNMOUNTED) {
		/*
		 * Shouldn't happen.
		 */
		TRACE(T_SAM_STATVFS, NULL, (sam_tr_t)vfsp, __LINE__, EIO);
		return (EIO);
	}
	TRACE(T_SAM_STATVFS, vfsp, (sam_tr_t)MAXNAMLEN, 0, 0);
	if (((mp = (sam_mount_t *)(void *)vfsp->vfs_data) != NULL) &&
	    ((sblk = mp->mi.m_sbp) != NULL)) {
		int free_blks;

		sam_open_operation_nb(mp);
		if (SAM_IS_SHARED_CLIENT(mp)) {
			(void) sam_proc_block_vfsstat(mp, SHARE_wait_one);
		} else if (SAM_IS_SHARED_READER(mp)) {
			sam_update_filsys(mp, 0);
		}
		if (sblk->info.sb.magic != SAM_MAGIC_V1 &&
		    sblk->info.sb.magic != SAM_MAGIC_V2 &&
		    sblk->info.sb.magic != SAM_MAGIC_V2A) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: Superblock magic invalid <%x>",
			    mp->mt.fi_name, sblk->info.sb.magic);
			error = EINVAL;
			goto out;
		}
		bzero((void *)sp, sizeof (sam_statvfs_t));

		sp->f_bsize = LG_BLK(mp, DD);	/* file system block size */
		sp->f_frsize = SAM_DEV_BSIZE;	/* fragment size */
		sp->f_blocks = sblk->info.sb.capacity;	/* total # of blocks */
		/*
		 * Compute dynamic free blocks, if not mounted rdonly nor shared
		 * client.
		 */
		free_blks = 0;
		if (!((mp->mt.fi_mflag & MS_RDONLY) ||
		    (SAM_IS_SHARED_CLIENT(mp)))) {
			int i;

			/*
			 * Add free blocks on the block list.
			 */
			for (i = 0; i < mp->mt.fs_count; i++) {
				struct samdent *dp;
				int bt;

				if (sblk->eq[i].fs.type == DT_META) {
					continue;
				}
				dp = &mp->mi.m_fs[i];
				if (dp->skip_ord) {
					continue;
				}
				for (bt = 0; bt < SAM_MAX_DAU; bt++) {
					int blocks;
					struct sam_block *block;

					if ((block = dp->block[bt]) == NULL) {
						continue;
					}
					blocks = BLOCK_COUNT(block->fill,
					    block->out, block->limit)
					    * sblk->info.sb.dau_blks[bt];
					if (sblk->eq[i].fs.num_group > 1) {
						blocks *=
						    sblk->eq[i].fs.num_group;
					}
					free_blks += blocks;
				}
			}
			/*
			 * Add free small blocks on the release block list.
			 */
			if (!mp->mt.mm_count) {
				free_blks += mp->mi.m_inoblk_blocks;
			}
			/*
			 * Add reserved large block in .block file.
			 */
			if (mp->mi.m_blk_bn &&
			    (mp->mi.m_inoblk->di.status.b.meta == DD)) {
				free_blks += LG_DEV_BLOCK(mp, DD);
			}
		}
		/* tot # of free blks */
		sp->f_bfree = sblk->info.sb.space + free_blks;
		/* blks avail */
		sp->f_bavail = sblk->info.sb.space + free_blks;

		sp->f_files = -1;	/* total # of file nodes (inodes) */
		sp->f_ffree = -1;	/* total # of free file nodes */
		sp->f_favail = -1;	/* inodes avail to non-superuser */
		if (sblk->info.sb.mm_count) {
			offset_t files;
			files = (sblk->info.sb.mm_capacity <<
			    SAM_DEV_BSHIFT) >> SAM_ISHIFT;
			/* total # of file nodes (inodes) */
			sp->f_files = (uint_t)files;
			files = (sblk->info.sb.mm_space <<
			    SAM_DEV_BSHIFT) >> SAM_ISHIFT;
			sp->f_ffree = (uint_t)files;
			sp->f_favail = sp->f_ffree;
		}
		/*
		 * sp->f_fsid:  dev32_t, even tho' it's an 8-byte type.
		 * If the 'nogfsid' mount option is set, we pick either
		 * the makedevice(samioc_major, fi_eq) or mp->mi.m_fs[0].dev
		 * for it, according to the value fo the cdevid/nocdevid
		 * mount flag.  If 'gfsid' is set, then it's the 4-byte
		 * seconds-since-1970 FS time-stamp from the superblock.
		 * Panics will happen if any of the upper four bytes are
		 * non-zero (in Solaris when cmpldev(~) fails).
		 *
		 * NFS cares deeply about this value.  It should not
		 * be changed willy-nilly; admins should not change
		 * [no]gfsid/[no]cdevid w/o good cause.  If the default
		 * values of these mount options are someday changed,
		 * the VSW_VOLATILEDEV flag should probably also be
		 * set in the samfs_vfsdef struct (>= Solaris 10U2).
		 *
		 * We allow the different values here to accommodate
		 * SunCluster/HA NFS, which must return one value
		 * for this across multiple hosts.  Although the major
		 * for sd/ssd devices is guaranteed, minors are not.
		 * Also, volume devices (Veritas, and other 3rd party
		 * devices) may have major/minor that vary from host
		 * to host.  samioc.major/fi_eq provide a semi-reasonable
		 * mechanism to avoid problems with non-constant dev_ts
		 * on different hosts.  It may still require editing
		 * /etc/name_to_major to give samioc a common value
		 * on all hosts, and doing configuration reboots.
		 * To use samioc.major/fi_eq, mount (on all hosts)
		 * with 'nogfsid' set (and 'cdevid' set (default)).
		 */
		sp->f_fsid = (unsigned long)vfsp->vfs_fsid.val[0];

		/* target fs type name */
		strcpy(sp->f_basetype, vfssw[vfsp->vfs_fstype].vsw_name);
		/* bit-mask of flags */
		sp->f_flag = vf_to_stf(vfsp->vfs_flag);
		sp->f_namemax = MAXNAMLEN;	/* maximum file name length */
out:
		SAM_CLOSE_OPERATION(mp, error);
		return (error);
	}
	cmn_err(CE_WARN, "SAM-QFS: Superblock ptrs invalid <%p/%p>",
	    (void *)mp, (void *)sblk);
	return (EINVAL);
}


/*
 * ----- samfs_sync - Update the file system.
 * Sync the SAM file system prior to umount if vfsp != NULL;
 * otherwise sync all SAM-QFS filesystems.
 * flag  = 0             Commands -- SYNC, UMOUNT or MOUNT.
 *         1 SYNC_ATTR   Sync cached attributes only (periodic).
 *         2 SYNC_CLOSE  Sync due to shutdown.
 */

/* ARGSUSED2 */
int				/* ERRNO if error, 0 if successful. */
samfs_sync(
	vfs_t *vfsp,		/* Vfs pointer for SAMFS. */
	short flag,		/* Sync flag, see description */
	cred_t *credp)		/* Credentials pointer */
{
	sam_mount_t *mp;
	int	count_em;
	int	error = 0;

	/* check to ensure init process finished */
	if (samgt.fstype == 0) {
		cmn_err(CE_WARN, "SAM-QFS: init incomplete, sync failed");
		return (EFAULT);
	}

	/* stop tracing if we're panicing. */
	if (panic_quiesce) {
		TRACE(T_SAM_SYNC, vfsp, (sam_tr_t)flag, 0, 0);
		sam_tracing = 0;
	}

	mutex_enter(&samgt.global_mutex);
	samgt.num_fs_syncing++;
	mutex_exit(&samgt.global_mutex);

	if (vfsp == NULL) {
		vfs_t *last_sam_vfsp;

		last_sam_vfsp = NULL;
		count_em = 1;
		for (mp = samgt.mp_list; mp != NULL; mp = mp->ms.m_mp_next) {
			if (!(mp->mt.fi_status & FS_MOUNTED)) {
				continue;
			}
			vfsp = (vfs_t *)mp->mi.m_vfsp;
			if ((vfsp->vfs_fstype == samgt.fstype) &&
			    (vfs_lock(vfsp) == 0)) {
				TRACE(T_SAM_SYNC, vfsp, flag, count_em,
				    mp->mt.fi_status);
				sam_open_operation_nb(mp);
				mutex_enter(&mp->ms.m_waitwr_mutex);
				vfs_unlock(vfsp);
				if (mp->mi.m_fs_syncing == 0) {
					mp->mi.m_fs_syncing = 1;
					mutex_exit(&mp->ms.m_waitwr_mutex);

					mutex_enter(&mp->ms.m_synclock);
					sam_update_filsys(mp, flag);
					mutex_exit(&mp->ms.m_synclock);

					mutex_enter(&mp->ms.m_waitwr_mutex);
					mp->mi.m_fs_syncing = 0;
				}
				mutex_exit(&mp->ms.m_waitwr_mutex);
				if (TRANS_ISTRANS(mp)) {
					/*
					 * LQFS: commit any outstanding async
					 * transactions
					 */
					curthread->t_flag |= T_DONTBLOCK;
					TRANS_BEGIN_SYNC(mp, TOP_COMMIT_UPDATE,
					    TOP_COMMIT_SIZE, error);
					if (!error) {
						TRANS_END_SYNC(mp, error,
						    TOP_COMMIT_UPDATE,
						    TOP_COMMIT_SIZE);
					}
					curthread->t_flag &= ~T_DONTBLOCK;
				}
				SAM_CLOSE_OPERATION(mp, error);
				TRACE(T_SAM_SYNC_RET, vfsp, flag,
				    count_em, mp->mt.fi_status);
				count_em++;
			}
		}
		vfsp = last_sam_vfsp;
	} else {
		if ((vfsp->vfs_fstype == samgt.fstype) &&
		    (vfs_lock(vfsp) == 0)) {
			mp = (sam_mount_t *)(void *)vfsp->vfs_data;
			TRACE(T_SAM_SYNC, vfsp, flag, 0, mp->mt.fi_status);
			sam_open_operation_nb(mp);
			mutex_enter(&mp->ms.m_waitwr_mutex);
			vfs_unlock(vfsp);
			if (mp->mi.m_fs_syncing == 0) {
				mp->mi.m_fs_syncing = 1;
				mutex_enter(&mp->ms.m_synclock);
				mutex_exit(&mp->ms.m_waitwr_mutex);
				sam_update_filsys(mp, flag);
				mutex_exit(&mp->ms.m_synclock);
				mutex_enter(&mp->ms.m_waitwr_mutex);
				mp->mi.m_fs_syncing = 0;
			}
			mutex_exit(&mp->ms.m_waitwr_mutex);
			if (TRANS_ISTRANS(mp)) {
				/*
				 * LQFS: commit any outstanding async
				 * transactions
				 */
				curthread->t_flag |= T_DONTBLOCK;
				TRANS_BEGIN_SYNC(mp, TOP_COMMIT_UPDATE,
				    TOP_COMMIT_SIZE, error);
				if (!error) {
					TRANS_END_SYNC(mp, error,
					    TOP_COMMIT_UPDATE,
					    TOP_COMMIT_SIZE);
				}
				curthread->t_flag &= ~T_DONTBLOCK;
			}
			SAM_CLOSE_OPERATION(mp, error);
			TRACE(T_SAM_SYNC_RET, vfsp, flag, 0, mp->mt.fi_status);
		}
	}
	mutex_enter(&samgt.global_mutex);
	if (--samgt.num_fs_syncing < 0) {
		samgt.num_fs_syncing = 0;
	}
	mutex_exit(&samgt.global_mutex);
	return (0);
}


/*
 * ----- samfs_vget - Return root vnode.
 * Return locked vnode for this file identifier. Generation
 * number must match.
 */

/* ARGSUSED2 */
int				/* ERRNO if error, 0 if successful. */
samfs_vget(
	vfs_t *vfsp,		/* VFS pointer for SAMFS. */
	vnode_t **vpp,		/* Pointer pointer to root vnode (returned) */
	fid_t *fidp)		/* Pointer to file identifier. */
{
	sam_fid_t *sam_fidp;
	sam_mount_t *mp;
	sam_node_t *ip;
	int error;

again:
	if (vfsp->vfs_flag & VFS_UNMOUNTED) {
		/*
		 * Don't use vfsp as first arg in TRACE -- sam mount structure
		 * may be going/may have gone away; sam_trace dereferences it.
		 */
		TRACE(T_SAM_VGET_UMNTD, NULL, (sam_tr_t)vfsp, __LINE__, EIO);
		*vpp = NULL;
		return (EIO);
	}
	mp = (sam_mount_t *)(void *)vfsp->vfs_data;
	sam_fidp = (sam_fid_t *)fidp;

	/*
	 * Trap for the special objctl Root Node.  This Root Node is used
	 * to emulate the Object namespace in qfs.
	 */
	if ((sam_fidp->un._fid.id.ino == SAM_OBJ_OBJCTL_INO) &&
	    (sam_fidp->un._fid.id.gen == SAM_OBJ_OBJCTL_INO)) {
		*vpp = mp->mi.m_vn_objctl;
		VN_HOLD(*vpp);
		return (0);
	}

	/*
	 * Need to hard freeze NFS threads in sam_open_vfs_operation.
	 * Unfortunately the call to VFS_VGET in nfs_fhtovp does not
	 * propagate EAGAIN to the caller resulting in a failed NFS operation
	 * rather than a retry.
	 * One possible negative side effect is for soft mounted
	 * file systems on the NFS client, these may get "connection timed out"
	 * errors if the freeze lasts too long.
	 */
	error = sam_open_vfs_operation(mp, sam_fidp);

	if (error) {
		return (error);
	}

	error = sam_stale_hash_ino(&sam_fidp->un._fid.id, mp, &ip);

	if (error == EAGAIN) {
		curthread->t_flag &= ~T_WOULDBLOCK;
		SAM_CLOSE_VFS_OPERATION(mp, error);
		goto again;
	}

	if (error != 0 || (ip == NULL)) {
		for (;;) {
			if ((error = sam_get_ino(vfsp, IG_EXISTS,
			    &sam_fidp->un._fid.id,
			    &ip))) {
				if (error == ETIME) {
					continue;
				} else if (error == EAGAIN) {
					curthread->t_flag &= ~T_WOULDBLOCK;
					SAM_CLOSE_VFS_OPERATION(mp, error);
					goto again;
				}
				*vpp = (vnode_t *)NULL;
				TRACE(T_SAM_VGET_ESRCH, vfsp,
				    sam_fidp->un._fid.id.ino,
				    sam_fidp->un._fid.id.gen, error);
				goto done;
			}
			break;
		}
	}
	*vpp = SAM_ITOV(ip);
	TRACE(T_SAM_VGET_RET, vfsp, sam_fidp->un._fid.id.ino,
	    sam_fidp->un._fid.id.gen, (sam_tr_t)*vpp);
done:
	SAM_CLOSE_VFS_OPERATION(mp, error);
	return (error);
}


/*
 *	----	samfs_freevfs
 *
 * Free the SAM-QFS mount structure.  Called some time after a
 * forced unmount, when we were unable to free the FS's resources
 * at unmount time (usually due to outstanding open vnodes).  The
 * mount structure associated with this vfsp should be on the
 * samgt.mp_stale list, and should/must have no other outstanding
 * references remaining.
 */
void
samfs_freevfs(vfs_t *vfsp)
{
	sam_mount_t *mp = (sam_mount_t *)(void *)vfsp->vfs_data;

	TRACE(T_SAM_VFS_FREE, vfsp, (sam_tr_t)mp, vfsp->vfs_flag,
	    mp->mt.fi_status);
	ASSERT(mp);
	mp->mi.m_vfsp = NULL;
	if ((vfsp->vfs_flag & VFS_UNMOUNTED) &&
	    mp && (mp->mt.fi_status & FS_MOUNTED) == 0) {
		dcmn_err((CE_NOTE,
		    "VFS_FREE: count=%d flag=%x\n",
		    vfsp->vfs_count, vfsp->vfs_flag));
		sam_destroy_stale_mount((sam_mount_t *)(void *)vfsp->vfs_data);
		return;
	}
}


/*
 * ---- sam_clients_mounted
 *
 * Return the number of mounted clients that this shared FS has.
 * The local host must be the server.  Does not count server.
 */
static int
sam_clients_mounted(sam_mount_t *mp)
{
	int n, ord;
	client_entry_t *clp;

	if (!SAM_IS_SHARED_FS(mp) || !SAM_IS_SHARED_SERVER(mp) ||
	    mp->ms.m_clienti == NULL) {
		return (0);
	}

	n = 0;
	for (ord = 1; ord <= mp->ms.m_max_clients; ord++) {
		clp = sam_get_client_entry(mp, ord, 0);
		if ((clp != NULL) &&
		    (ord != mp->ms.m_client_ord) &&
		    (clp->cl_status & FS_MOUNTED) &&
		    ((lbolt - clp->cl_msg_time) / hz < 3 * SAM_MIN_DELAY)) {
			/*
			 * Host isn't server , is mounted, and sent a message
			 * recently.
			 */
			n++;
		}
	}
	return (n);
}

/*
 * ---- sam_open_vfs_operation
 *
 * Set the task specific data for a VFS operation.
 */
static int
sam_open_vfs_operation(sam_mount_t *mp, sam_fid_t *sam_fidp)
{
	sam_operation_t ep;
	int error = 0;
	int force_freeze = 0;

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

		if (SAM_THREAD_IS_NFS()) {
			/*
			 * Hard freeze NFS threads.
			 * See comments in samfs_vget.
			 */
			force_freeze = 1;
		}

		TRACE(T_SAM_VGET_FREEZE, mp, sam_fidp->un._fid.id.ino,
		    sam_fidp->un._fid.id.gen, 0);
		error = sam_freeze_ino(mp, NULL, force_freeze);
		TRACE(T_SAM_VGET_UNFREEZE, mp, sam_fidp->un._fid.id.ino,
		    sam_fidp->un._fid.id.gen, error);

		if (error != 0) {
			return (error);
		}
	}
	/*
	 * This needs to be non-blocking
	 * since there are paths that acquire
	 * an inode write lock.
	 */
	ep.val.flags = SAM_FRZI_NOBLOCK;
	ep.val.depth = 1;
	ep.val.module = mp->ms.m_tsd_key;
	SAM_INC_OPERATION(mp);
	tsd_set(mp->ms.m_tsd_key, ep.ptr);
	return (0);
}
