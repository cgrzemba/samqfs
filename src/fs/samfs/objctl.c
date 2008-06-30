/*
 * ---- osdfs_ctldir.c - Object File System namespace emulation routines.
 *
 * Object QFS control directory (a.k.a. ".objects")
 *
 * This is the 'objects' directory that provides the capability to display
 * objects in the partition.  The objects are built using the GFS primitives,
 * as the hierarchy does not exist in Object QFS.
 *
 *			 samfs1
 *			    |
 *			.objects
 *		            |
 *       Object_id .. Object_id Object_id .. Object_id
 *
 * This Object QFS control directory using GFS will allow a user to access
 * Objects from User Land, as if the objects has name and hierarchical.  This
 * allows us to use standard tools like 'ls' to display objects meta data and
 * attributes.
 *
 * Since object space is flat, an ls of /[mountpoint]/.objects is efectively
 * a 'find *' operation on the whole Object QFS File System.  This can be
 * very expensive.  It is much better to get the 'object name' from the MDS
 * and access that object on the target e.g.
 * "ls -al /[mountpoint]/.objects/[object_name]
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

#pragma ident "$Revision: 1.1 $"

#include <unistd.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/dirent.h>
#include <sys/fcntl.h>
#include <sys/sysmacros.h>
#include <sys/vnode.h>
#include <sys/fs_subr.h>
#include <sys/flock.h>
#include <sys/systm.h>
#include <sys/ioccom.h>
#include <sys/vmsystm.h>
#include <sys/file.h>
#include <sys/rwlock.h>
#include <sys/dnlc.h>
#include <sys/share.h>
#include <sys/policy.h>
#include "gfs.h"
#include <sys/stat.h>
#include <sys/mount.h>

#if defined(SOL_511_ABOVE)
#include <sys/vfs_opreg.h>
#endif

#include <nfs/nfs.h>
#include <nfs/lm.h>

#include <vm/as.h>
#include <vm/seg_vn.h>
#include <vm/seg_map.h>
#include <vm/pvn.h>

/* ----- SAMFS Includes */

#include "sys/cred.h"
#include "sam/param.h"
#include "sam/types.h"
#include "sam/samaio.h"

#include "samfs.h"
#include "ino.h"
#include "macros.h"
#include "inode.h"
#include "mount.h"
#include "rwio.h"
#include "ioblk.h"
#include "extern.h"
#include "debug.h"
#include "trace.h"
#include "ino_ext.h"
#include "listio.h"
#include "objctl.h"
#include "qfs_log.h"

vnodeops_t *objctl_ops_root;
static const fs_operation_def_t objctl_tops_root[];

static gfs_opsvec_t objctl_opsvec[] = {
	{ ".objects", objctl_tops_root, &objctl_ops_root },
	{ NULL }
};

typedef struct objctl_node {
	gfs_dir_t   qc_gfs_private;
	uint64_t    qc_id;
	timestruc_t qc_cmtime;  /* ctime and mtime, always the same */
} objctl_node_t;

/*
 * samidtoa - Converts a SAMFS inode id into it's hex string representation.
 *	sam_id consist of an Inode-Number and Generation Number.
 *	This routine converts the sam_id to [inode-number][generation-number]
 *		Example:
 *			inode-number = 0x40 and generation-number = 3
 *			0000004000000003 is the converted string representation
 *			of the sam_id.
 *
 *	Since Objects do not have names, and all pathnames are required in
 *	Posix Interfaces(some), being able to represent Objects by using it's
 *	string representation enables programs to manipulate objects directly
 *	on the Storage Node.
 */
/* ARGSUSED */
static void
samidtoa(sam_id_t *id, char *id_str)
{
	uint32_t ino;
	uint32_t gen;
	char *temp = id_str;
	int i, count;

	ino = id->ino;
	gen = id->gen;

	/*
	 * Format the inode-number and generation-number.
	 *	Example:
	 *		0000112200003344 = ino 0x1122 and gen 0x3344
	 *
	 */
	for (count = 7, i = 0; i < 8; i++, count--) {
		sprintf((char *)&temp[i], "%x",
		    (uchar_t)(0xf & (ino >> (count*4))));
	}
	for (count = 7, i = 8; i < 16; i++, count--) {
		sprintf((char *)&temp[i], "%x",
		    (uchar_t)(0xf & (gen >> (count*4))));
	}
}

/*
 * sam_objctl_init() - Initialize the various GFS pieces we'll need to create
 * and manipulate ".objects" namespace. This is called from the samfs init
 * routine during module load time, and initializes the vnode ops vectors
 * that we'll be using to support POSIX type calls to Objects on the
 * Storage node Target.
 */
/* ARGSUSED */
void
sam_objctl_init(void)
{
	VERIFY(gfs_make_opsvec(objctl_opsvec) == 0);
}

/*
 * sam_objctl_fini() - Called when samfs module unloads.
 */
/* ARGSUSED */
void
sam_objctl_fini(void)
{
	/*
	 * Remove vfsctl vnode ops
	 */
	if (objctl_ops_root)
		vn_freevnodeops(objctl_ops_root);

	objctl_ops_root = NULL;
}

/*
 * sam_objctl_readdir_callback: We get in here when someone is trying to do a
 * lookup on Parent "[mountpoint]/.objects".
 *
 * Since object name space is flat .. an ls on [mountpoint]/.objects will
 * show you every single object in the file system.  This is not a very
 * good way of finding your objects.  First get your object identification
 * from the Meta Data Server, and then only do:
 * ls [mountpoint]/.objects/[object-id]
 */
/* ARGSUSED */
static int
sam_objctl_readdir_callback(vnode_t *vp, void *dirent, int *eofp,
    offset_t *offp, offset_t *nextp, void *data, int flags)
{
	char objectname[MAXNAMELEN];
	struct dirent64 *dp = (struct dirent64 *)dirent;
	sam_ino_t ino;
	struct sam_mount *mp;
	struct vfs *vfsp;
	struct sam_node *userobj;
	sam_id_t	*object_id;
	sam_ihead_t *hip;
	int ihash;
	kmutex_t *ihp;
	struct sam_perm_inode *pip;
	buf_t *bp;
	int32_t max_inum;
	int32_t found;
	int error;

	vfsp = vp->v_vfsp;
	mp = vfsp->vfs_data;
	ino = (sam_ino_t)*offp;
	if (ino == 0)
		ino = SAM_MIN_USER_INO; /* System objects are not shown */

	/*
	 * Need to terminate if it is greater the .inode EOF
	 */
	max_inum = ((mp->mi.m_inodir->di.rm.size +
	    (mp->mi.m_inodir->di.status.b.direct_map ?
	    0 : mp->mt.fi_rd_ino_buf_size)) >> SAM_ISHIFT);
	if (ino >= max_inum) { /* We are done. */
		*eofp = 1;
		return (0);
	}

	/*
	 * Find it in cache.
	 * If not in cache - we need to read it from disk.
	 */
	found = 0;
	ihash = SAM_IHASH(ino, mp->mi.m_vfsp->vfs_dev);
	ihp = &samgt.ihashlock[ihash];  /* Set hash lock */
	mutex_enter(ihp);
	hip = &samgt.ihashhead[ihash];
	ASSERT(hip != NULL);
	userobj = (sam_node_t *)hip->forw;
	while (userobj != (sam_node_t *)(void *)hip) {
		if ((ino == userobj->di.id.ino) && (mp == userobj->mp)) {
			/*
			 * Found inode.
			 */
			if (userobj->flags.bits & SAM_STALE) {
				break;
			} else {
				/*
				 * We should verify that it is an object?
				 */
				object_id = &userobj->di.id;
				found++;
				break;
			}
		}

	} /* while (userobj != (sam_node_t *)(void *)hip) */
	mutex_exit(ihp);

	while (ino <= max_inum && !found) {
		/*
		 * Actually, this loop looks really dumb - one would assume
		 * that once you get a block of inodes, perhaps you should
		 * loop through all the inodes in the block before calling
		 * sam_read_ino() for 1st inode on the next block.
		 *
		 * This is lifted code .. so, there must be a good reason.
		 */
		if ((error = sam_read_ino(mp, ino, &bp, &pip)) != 0) {
			/*
			 * Can't read i-node. I/O error?  error is errno (>0).
			 */
			cmn_err(CE_WARN,
			    "sam_objctl_readdir_callback: %s: "
			    "sam_inodes_count: "
			    "i-node %d is unreadable error %d",
			    mp->mt.fi_name, ino, error);
			break;
		}
		if (pip->di.mode == 0) {
			/* Free i-node - bump count and loop again. */
			ino++;
			brelse(bp);
			continue;
		}

		/*
		 * The correct object type is not set yet.
		 * We may also have regualr files in this File System.
		 * Do the correct check when we implement the correct
		 * object type.  For now, this test is a placeholder.
		 */
		if (1) {
			/*
			 * Found a valid object.
			 */
			found++;
			object_id = &pip->di.id;
			brelse(bp);
			break;
		} else {
			ino++;
			brelse(bp);
		}
	} /* while (ino <= max_inum && !found) */

	if (found) {
		/*
		 * Found valid object.
		 */
		bzero((char *)objectname, MAXNAMELEN);
		samidtoa(object_id, (char *)objectname);
		(void) strcpy(dp->d_name, objectname);
		dp->d_ino = ino;
		*nextp = ++ino;
	} else {
		/*
		 * None left ..
		 */
		*eofp = 1;
	}

	return (0);
}

/*
 * sam_objctl_create() - Create the pseudo '.objects' directory that implements
 * the namespace for objects on the Storage Node.  This directory
 * is cached as part of the VFS structure.  This result in a hold on the
 * vfs_t.  The code in samfs_umount() therefore checks against a vfs_count
 * of 2 instead of 1.  This reference is removed when the ctldir is
 * destroyed in the unmount.
 *
 * Note that this only exist in memory no actual disk version exist.
 *
 * This routine is called in samfs_mount().
 */
/* ARGSUSED */
void
sam_objctl_create(struct sam_mount *mp)
{
	vnode_t *vp;
	objctl_node_t *qcp;

	ASSERT(mp->mi.m_vn_objctl == NULL);

	vp = gfs_root_create(sizeof (objctl_node_t), mp->mi.m_vfsp,
	    objctl_ops_root, OBJCTL_INO_ROOT, NULL,
	    NULL, MAXNAMELEN, sam_objctl_readdir_callback, NULL);
	if (!vp) {
		/*
		 * Allow the mount to proceed.
		 */
		cmn_err(CE_WARN,
		    "objctl_create: Unable to create root gfs node\n");
		return;
	}

	qcp = vp->v_data;
	qcp->qc_id = OBJCTL_INO_ROOT;
	gethrestime(&qcp->qc_cmtime);

	/*
	 * We're only faking the fact that we have a root of a filesystem for
	 * the sake of the GFS interfaces.  Undo the flag manipulation it did
	 * for us.
	 */
	vp->v_flag &= ~(VROOT | VNOCACHE | VNOMAP | VNOSWAP | VNOMOUNT);

	cmn_err(CE_WARN, "objctl_create: vn 0s%llx vn_count = %d\n",
	    vp, vp->v_count);
	mp->mi.m_vn_objctl = vp;
}

/*
 * sam_objctl_destroy - Destroy the '.objects' directory.  Only called when the
 * filesystem is unmounted. There might still be more references if we
 * were force unmounted, but only new qfs_inactive() calls can occur
 * and they don't reference .objects
 */
/* ARGSUSED */
void
sam_objctl_destroy(struct sam_mount *mp)
{

	/*
	 * Check and make sure that the '.objects' directory was actually
	 * created.
	 */
	if (mp->mi.m_vn_objctl) {
		cmn_err(CE_WARN,
		    "objctl_create: vn 0s%llx vn_count = %d\n",
		    mp->mi.m_vn_objctl, (mp->mi.m_vn_objctl)->v_count);
		VN_RELE(mp->mi.m_vn_objctl);
	}
	mp->mi.m_vn_objctl = NULL;
}

/*
 * objctl_common_open - Open the '.objects' pseudo directory.
 */
/* ARGSUSED */
static int
sam_objctl_common_open(vnode_t **vpp, int flags, cred_t *cr)
{
	/*
	 * For now we do not support opening an object for writing on the
	 * Storage Node.
	 */
	if (flags & FWRITE)
		return (EACCES);

	return (0);
}

/*
 * objctl_common_close - Close the '.objects' pseudo directory.
 */
/* ARGSUSED */
static int
sam_objctl_common_close(vnode_t *vpp, int flags, int count, offset_t off,
    cred_t *cr)
{
	return (0);
}

/*
 * objctl_common_access - Disallow writes on the pseudo '.objects'
 * directory.
 */
/* ARGSUSED */
static int
sam_objctl_common_access(vnode_t *vp, int mode, int flags, cred_t *cr)
{
	if (mode & VWRITE)
		return (EACCES);

	return (0);
}

/*
 * objctl_common_getattr - Get attributes of the pseudo '.objects' directory.
 */
/* ARGSUSED */
static void
sam_objctl_common_getattr(vnode_t *vp, vattr_t *vap)
{
	objctl_node_t	*qcp = vp->v_data;
	timestruc_t	now;

	vap->va_uid = 0;
	vap->va_gid = 0;
	vap->va_rdev = 0;
	/*
	 * We are a purly virtual object, so we have no
	 * blocksize or allocated blocks.
	 */
	vap->va_blksize = 0;
	vap->va_nblocks = 0;
	vap->va_seq = 0;
	vap->va_fsid = vp->v_vfsp->vfs_dev;
	vap->va_mode = S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP |
	    S_IROTH | S_IXOTH;
	vap->va_type = VDIR;
	/*
	 * We live in the now (for atime).
	 */
	gethrestime(&now);
	vap->va_atime = now;
	vap->va_mtime = vap->va_ctime = qcp->qc_cmtime;
}

/*
 * objctl_common_fid - Return file identifier.
 *
 *	Return file identifier given vnode. Used by nfs.
 */
/* ARGSUSED */
static int
sam_objctl_common_fid(vnode_t *vp, fid_t *fidp)
{
	sam_fid_t *sam_fidp = (sam_fid_t *)fidp;


	if (fidp->fid_len < (sizeof (sam_fid_t) - sizeof (ushort_t))) {
		fidp->fid_len = sizeof (sam_fid_t) - sizeof (ushort_t);
		return (ENOSPC);
	}
	bzero((char *)sam_fidp, sizeof (sam_fid_t));
	sam_fidp->fid_len = sizeof (sam_fid_t) - sizeof (ushort_t);
	sam_fidp->un._fid.id.ino = SAM_OBJ_OBJCTL_INO;
	sam_fidp->un._fid.id.gen = SAM_OBJ_OBJCTL_INO;
	printf("objctl_common_fid: Called for control node\n");
	return (0);
}

/*
 * objctl_common_remove - Remove an object.
 *
 * Remove an object from the Object QFS
 */
/* ARGSUSED */
static int			/* ERRNO if error, 0 if successful. */
sam_objctl_common_remove(
	vnode_t *pvp,		/* Pointer to parent directory vnode. */
	char *cp,		/* Pointer to the component name to remove. */
	cred_t *credp)		/* credentials pointer. */
{

	struct sam_mount *mp = pvp->v_vfsp->vfs_data;
	sam_node_t *ip = NULL;
	sam_id_t sam_id;
	vnode_t *vp = NULL;
	uint64_t id;
	vfs_t *vfsp;
	int error = 0;

	vfsp = mp->mi.m_vfsp;
	if (strlen(cp) != 16) {
		return (ENOENT);
	}

	/*
	 * Translate the name into a sam_id.
	 */
	error = ddi_strtoul(cp, NULL, 16, (unsigned long *)&id);
	sam_id.ino = (id >> 32) & 0xffffffff;
	sam_id.gen = id & 0xffffffff;

	/*
	 * Get the vnode and free the vnode.
	 */
	error = sam_get_ino(vfsp, IG_EXISTS, &sam_id, &ip);
	if (!ip || error) {
		return (error);
	}
	vp = SAM_ITOV(ip);

	/*
	 * Objects are not parents nor does it has children.  Basically, we
	 * just need to free this particular inode.
	 *
	 * This delete has been initiated the Storage Node.  The assumption
	 * here is that - irrespective, just get rid of the inode.
	 */
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	ip->di.nlink = 0;
	TRANS_INODE(ip->mp, ip);
	sam_mark_ino(ip, SAM_CHANGED);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

	/* vnevent_remove(vp); */
	VN_RELE(vp)
	return (ENOENT);
}

/*
 * Get root directory attributes.
 */
/* ARGSUSED */
static int
sam_objctl_root_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cr)
{

	vap->va_nodeid = OBJCTL_INO_ROOT;
	vap->va_nlink = 1;
	vap->va_size = 1;

	sam_objctl_common_getattr(vp, vap);
	return (0);
}

/*
 * Special case the handling of "..".
 */
/* ARGSUSED */
int
sam_objctl_root_lookup(vnode_t *dvp, char *nm, vnode_t **vpp, pathname_t *pnp,
    int flags, vnode_t *rdir, cred_t *cr)
{
	uint64_t id;
	struct sam_id sam_id;
	struct sam_node *userobj;
	int err;

	if (strcmp(nm, "..") == 0) {
		err = VFS_ROOT(dvp->v_vfsp, vpp);
		return (err);
	}

	/*
	 * Special case '.'.
	 */
	if (*nm == '\0' || strcmp(nm, ".") == 0) {
		VN_HOLD(dvp);
		*vpp = dvp;
		return (0);
	}

	/*
	 * The component name is a hex ascii representation of the object number
	 * Example: 11223344aabbccdd
	 * It has to be exactly 16 characters.
	 */
	if (strlen(nm) != 16) {
		printf("objctl_root_lookup: Component name %s not found\n",
		    nm);
		return (ENOENT);
	}

	/*
	 * Translate the name into a sam_id.
	 */
	err = ddi_strtoul(nm, NULL, 16, (unsigned long *)&id);
	sam_id.ino = (id >> 32) & 0xffffffff;
	sam_id.gen = id & 0xffffffff;

	/*
	 * Get the inode.
	 */
	err = sam_get_ino(dvp->v_vfsp, IG_EXISTS, &sam_id, &userobj);
	if (err) {
		return (ENOENT);
	}

	*vpp = SAM_ITOV(userobj);
	return (0);

}

/*
 * These are the vnop operations that are supported for the special directory
 * .objects.  Objects prior(mountpoint) and beyond this pseudo directory uses
 * the SAMQFS vnodeops.  We may create more vnodeops e.g. VOP_CREATE if the
 * arises.
 */
static const fs_operation_def_t objctl_tops_root[] = {
	VOPNAME_OPEN, sam_objctl_common_open,
	VOPNAME_CLOSE, sam_objctl_common_close,
	VOPNAME_GETATTR, sam_objctl_root_getattr,
	VOPNAME_ACCESS,	sam_objctl_common_access,
	VOPNAME_READDIR, gfs_vop_readdir,
	VOPNAME_LOOKUP, sam_objctl_root_lookup,
	VOPNAME_INACTIVE, (fs_generic_func_p)gfs_vop_inactive,
	VOPNAME_FID, sam_objctl_common_fid,
	VOPNAME_REMOVE, sam_objctl_common_remove,
	NULL, NULL
};
