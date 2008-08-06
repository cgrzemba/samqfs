/*
 * ---- osdfsops.c - Object File System operations.
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

#ifdef sun
#pragma ident "$Revision: 1.2 $"
#endif

#include "sam/osversion.h"

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

#include "pub/objnops.h"
#include "pub/osdfsops.h"

/* ----- SAMFS Includes */

#include "sys/cred.h"
#include "sam/types.h"
#include "sam/mount.h"

#ifdef METADATA_SERVER
#include "license/license.h"
#endif	/* METADATA_SERVER */

#include "sys/cred.h"
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
#include "osdfs.h"

/*
 * ----- sam_osdfs_verify - Verify the handle in the osdfs.
 * This routine will be used by the ioctl() layer to verify that the handle
 * is still valid.
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
sam_osdfs_verify(
	osdfs_t	*osdfsp,	/* osdfs_t pointer for SAMFS. */
	cred_t	*credp)		/* Credentials pointer */
{

	sam_mount_t *mp = (sam_mount_t *)osdfsp->osdfs_data;
	vfs_t *vfsp;
	int error = 0;

	if (!mp) {
		error = EINVAL;
		cmn_err(CE_WARN, "sam_osdfs_verify: Null mp\n");
		return (error);
	}

	vfsp = mp->mi.m_vfsp;
	if (!vfsp) {
		error = EINVAL;
		cmn_err(CE_WARN, "sam_osdfs_verify: Null Handle\n");
		return (error);
	}

	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EPERM;
		cmn_err(CE_WARN, "sam_osdfs_verify: Failed secpolicy\n");
		return (error);
	}

	return (error);
}

/*
 * ----- sam_osdfs_lunattach - Attach a mounted SAM-QFS file system to a
 * Logical Unit Number exported by the OSD LUN Driver on the Storage Node.
 *
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
sam_osdfs_lunattach(
	osdfs_t *osdfsp,	/* osdfs_t pointer for SAMFS. */
	char *fsname,		/* FS name */
	cred_t *credp)		/* Credentials pointer */
{

	sam_mount_t *mp = NULL;
	int error = 0;

	/*
	 * Associate a Lun to a file system.
	 */
	mutex_enter(&samgt.global_mutex);
	for (mp = samgt.mp_list; mp != NULL; mp = mp->ms.m_mp_next) {
		if ((mp->mt.fi_status & FS_MOUNTED) &&
		    !(mp->mt.fi_status & FS_UMOUNT_IN_PROGRESS)) {
			if (strncmp(mp->mt.fi_name, fsname,
			    sizeof (mp->mt.fi_name)) == 0) {
				break;
			}
		}
	}
	if (mp == NULL) {
		mutex_exit(&samgt.global_mutex);
		return (ENODEV);
	}

	if (osdfsp->osdfs_lun == ~0) {
		/* Invalid LU Number. */
		cmn_err(CE_WARN,
		    "sam_osdfs_lunattach: Invalid LUN 0x%llx\n",
		    osdfsp->osdfs_lun);
		mutex_exit(&samgt.global_mutex);
		return (EINVAL);
	}

	mp->mi.m_osdt_lun = osdfsp->osdfs_lun;
	osdfsp->osdfs_data = (void *)mp;
	mutex_exit(&samgt.global_mutex);
	return (error);

}

/*
 * ----- sam_osdfs_lundetach - Dettach a mounted SAM-QFS file system to a
 * Logical Unit Number exported by the OSD LUN Driver on the OSD Target host.
 *
 * After this operation, the file system is not accessible to the OSD LUN
 * Driver on the OSD Target.
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
sam_osdfs_lundetach(
	osdfs_t *osdfsp,	/* osdfs_t pointer for SAMFS. */
	int fflag,		/* Forced umount flag */
	cred_t *credp)		/* Credentials pointer */
{
	sam_mount_t *mp = NULL;
	int error = 0;

	/*
	 * Disconnect a LUN to a file system.
	 */
	mutex_enter(&samgt.global_mutex);
	for (mp = samgt.mp_list; mp != NULL; mp = mp->ms.m_mp_next) {
		if (osdfsp->osdfs_data == (void *)mp) {
			if (!(mp->mt.fi_status & FS_MOUNTED)) {
				mutex_exit(&samgt.global_mutex);
				cmn_err(CE_WARN, " Lun Detach FS not mounted "
				    "not found\n");
				return (ENOENT);
			}
			break;
		}
	}
	if (mp == NULL) {
		mutex_exit(&samgt.global_mutex);
		cmn_err(CE_WARN, " Lun Detach FS not found\n");
		error = ENODEV;
		return (error);
	}

	osdfsp->osdfs_data = NULL;
	osdfsp->osdfs_lun = ~0ULL;
	mp->mi.m_osdt_lun = ~0ULL;
	mutex_exit(&samgt.global_mutex);
	return (error);
}

/* ARGSUSED */
int
sam_osdfs_get_rootobj(osdfs_t *osdfsp, objnode_t **robjpp, cred_t *credp)
{
	sam_mount_t *mp;
	int error = 0;

	mp = (sam_mount_t *)osdfsp->osdfs_data;
	if (!mp || (mp->mi.m_osdt_lun == ~0ULL)) {
		error = EINVAL;
		cmn_err(CE_WARN,
		    "sam_osdfs_get_rootobj: Not Attached 0x%llx\n", mp);
		return (error);
	}

	VN_HOLD(SAM_ITOV(mp->mi.m_osdfs_root));
	*robjpp = mp->mi.m_osdfs_root->objnode;

	return (error);
}

/*
 * ----- sam_osdfs_oget - Returns the object node pointer for object id.
 * The object id may be a Collection or a User Object.
 *
 * The Object is returned HELD.  The Caller is expected to call sam_obj_rele()
 * to release the hold on the object.
 */

/* ARGSUSED */
int
sam_osdfs_oget(osdfs_t *osdfsp, uint64_t partition_id,
    uint64_t objid, objnode_t **objpp)

{
	sam_node_t *ip = NULL;
	sam_mount_t *mp;
	int error = 0;

	if (!partition_id) {
		error = EINVAL;
		cmn_err(CE_WARN, "sam_osdfs_oget: NULL Partition Id\n");
		return (error);
	}

	if (!osdfsp) {
		error = EINVAL;
		cmn_err(CE_WARN, "sam_osdfs_oget: NULL vfsp\n");
		return (error);
	}

	mp = (sam_mount_t *)osdfsp->osdfs_data;
	if (!mp || (mp->mi.m_osdt_lun == ~0ULL)) {
		error = EINVAL;
		cmn_err(CE_WARN, "sam_osdfs_oget: Not attached 0x%llx\n", mp);
		return (error);
	}

	error = sam_get_ino(mp->mi.m_vfsp, IG_EXISTS, (sam_id_t *)&objid,
		    &ip);
	if (!ip || error) {
		cmn_err(CE_WARN, "sam_osdfs_oget: Id get 0x%llx error %d\n",
		    objid, error);
		return (error);
	} else {
		/*
		 * We have the object.  Return the vnode pointer as handle.
		 * The vnode is held.
		 */
		*objpp = ip->objnode;
	}

	return (error);
}

/*
 * ----- sam_osdfs_paroget - Returns the Partition Object handle given the
 * partition id.
 */

/* ARGSUSED */
int
sam_osdfs_paroget(osdfs_t *osdfsp, uint64_t partition_id, objnode_t **objpp)
{
	sam_node_t *parip;
	sam_mount_t *mp;
	int error = 0;

	if (!partition_id) {
		error = EINVAL;
		cmn_err(CE_WARN, "sam_osdfs_paroget: Null partition Id\n");
		return (error);
	}

	mp = (sam_mount_t *)osdfsp->osdfs_data;
	if (!mp || (mp->mi.m_osdt_lun == ~0ULL)) {
		error = EINVAL;
		cmn_err(CE_WARN,
		    "sam_osdfs_paroget: Not attached 0x%llx\n", mp);
		return (error);
	}


	if (partition_id == SAM_OBJ_PAR_ID) {
		/* Special Default Partition for SAMQFS */
		parip = mp->mi.m_osdfs_part;
		VN_HOLD(SAM_ITOV(parip));
	} else {
		/* Go get the partition object */
		error = sam_get_ino(mp->mi.m_vfsp, IG_EXISTS,
		    (sam_id_t *)&partition_id, &parip);
		if (!error) {
			cmn_err(CE_WARN,
			    "sam_osdfs_paroget: Get Id error %d\n", error);
		}
	}

	*objpp = parip->objnode;
	return (error);

}

/*
 * ----- sam_osdfs_sync - Flush the Object File System.
 */

/* ARGSUSED */
int
sam_osdfs_sync(osdfs_t *osdfsp, short flag, cred_t *credp)
{
	sam_mount_t *mp;
	int error = 0;

	mp = (sam_mount_t *)osdfsp->osdfs_data;
	if (!mp || (mp->mi.m_osdt_lun == ~0ULL)) {
		error = EINVAL;
		cmn_err(CE_WARN,
		    "sam_osdfs_sync: Not attached 0x%llx\n", mp);
		return (error);
	}

	error = VFS_SYNC(mp->mi.m_vfsp, flag, credp);
	return (error);

}

struct osdfsops osdfsops = {
	sam_osdfs_verify,	/* osdfs_verify */
	sam_osdfs_lunattach,	/* osdfs_lunattach */
	sam_osdfs_lundetach,	/* osdfs_lundetach */
	sam_osdfs_oget,		/* osdfs_oget */
	sam_osdfs_paroget,	/* osdfs_paroget */
	sam_osdfs_sync,		/* osdfs_sync */
	sam_osdfs_get_rootobj	/* osdfs_get_rootobj */
};

osdfsops_t *osdfsopsp = &osdfsops;

/*
 * ----- osdfs_ioctlregister - Register an ioctl connection to the OSD.
 * The OSD SAM-QFS File System must already be attached by the LU Driver.
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
osdfs_ioctl_register(
	osdfs_t *osdfsp,	/* osdfs_t pointer for SAMFS. */
	char *fsname,		/* FS name */
	cred_t *credp)		/* Credentials pointer */
{

	sam_mount_t *mp = NULL;
	int error = 0;

	mutex_enter(&samgt.global_mutex);
	for (mp = samgt.mp_list; mp != NULL; mp = mp->ms.m_mp_next) {
		if ((mp->mt.fi_status & FS_MOUNTED) &&
		    !(mp->mt.fi_status & FS_UMOUNT_IN_PROGRESS)) {
			if (strncmp(mp->mt.fi_name, fsname,
			    sizeof (mp->mt.fi_name)) == 0) {
				break;
			}
		}
	}
	if (mp == NULL) {
		mutex_exit(&samgt.global_mutex);
		return (ENODEV);
	}

	if (mp->mi.m_osdt_lun == ~0ULL) {
		mutex_exit(&samgt.global_mutex);
		return (ENODEV);
	}

	osdfsp->osdfs_data = (void *)mp;
	mutex_exit(&samgt.global_mutex);
	return (error);

}

/*
 * ----- osdfs_ioctl_unregister - Unregister and ioctl connection to the OSD
 * device.
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
osdfs_ioctl_unregister(
	osdfs_t *osdfsp,	/* osdfs_t pointer for SAMFS. */
	int fflag,		/* Forced umount flag */
	cred_t *credp)		/* Credentials pointer */
{
	int error = 0;

	/*
	 * For now just clear the handle.
	 */
	osdfsp->osdfs_data = NULL;
	return (error);
}
