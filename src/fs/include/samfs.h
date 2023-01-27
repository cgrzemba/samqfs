/*
 *	samfs.h - vfs and vnode prototypes for the SAM file system.
 *
 *	Defines the functions prototypes for the vfs and vnode
 *	operation of the SAM-QFS filesystem.
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

#ifndef	_SAMFS_H
#define	_SAMFS_H

#ifdef	linux
#ifdef	__KERNEL__
#include "linux/types.h"
#else
#include <sys/types.h>
#endif	/* __KERNEL__ */
#else	/* linux */

#include	<sys/types.h>
#include	<sys/vfs.h>
#include	<sys/vnode.h>

#endif	/* linux */


/* ----- samfs file system type. */

#if !defined(SAM_INIT)
extern int samfs_fstype;
#endif


/* ----- samfs fsid.  */

#define MODULE_VERSION MOD_VERSION
#define	SAMFS_NAME_VERSION	"SAM-QFS Storage Archiving Mgmt v" MODULE_VERSION


/* ----- SAM-FS vfs function prototypes.  */

int samfs_mount(vfs_t *vfsp, vnode_t *vp, struct mounta *pp, cred_t *credp);
int samfs_umount(vfs_t *vfsp, int, cred_t *credp);
int samfs_root(vfs_t *vfsp, vnode_t **vpp);
int samfs_statvfs(vfs_t *vfsp, sam_statvfs_t *sp);
int samfs_sync(vfs_t *vfsp, short flag, cred_t *credp);
int samfs_vget(vfs_t *vfsp, vnode_t **vpp, struct fid *fidp);
void samfs_freevfs(vfs_t *vfsp);


#if defined(SOL_511_ABOVE)
/*
 * ----- Solaris 11 SAM-FS vnode function prototypes.
 */
int sam_open_vn(vnode_t **vpp, int, cred_t *credp, caller_context_t *ct);
int sam_close_vn(vnode_t *vp, int filemode, int count, offset_t offset,
	cred_t *credp, caller_context_t *ct);
int sam_read_vn(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *credp,
	caller_context_t *ct);
int sam_write_vn(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *credp,
	caller_context_t *ct);
int sam_ioctl_vn(vnode_t *vp, int cmp, sam_intptr_t arg, int flag,
	cred_t *credp, int *rvp, caller_context_t *ct);
int sam_getattr_vn(vnode_t *vp, struct vattr *vap, int flag, cred_t *credp,
	caller_context_t *ct);
int sam_setattr_vn(vnode_t *vp, struct vattr *vap, int flag, cred_t *credp,
	caller_context_t *ct);
int sam_access_vn(vnode_t *vp, int mode, int flag, cred_t *credp,
	caller_context_t *ct);
int sam_lookup_vn(vnode_t *vp, char *cp, vnode_t **vpp, struct pathname *pnp,
	int flag, vnode_t *rdir, cred_t *credp, caller_context_t *ct,
	int *defp, struct pathname *rpnp);
int sam_create_vn(vnode_t *vp, char *cp, struct vattr *vap, vcexcl_t ex,
	int mode, vnode_t **vpp, cred_t *credp, int filemode,
	caller_context_t *ct, vsecattr_t *vsap);
int sam_remove_vn(vnode_t *pvp, char *cp, cred_t *credp,
	caller_context_t *ct, int flag);
int sam_link_vn(vnode_t *pvp, vnode_t *vp, char *cp, cred_t *credp,
	caller_context_t *ct, int flag);
int sam_rename_vn(vnode_t *opvp, char *omn, vnode_t *npvp, char *nnm,
	cred_t *credp, caller_context_t *ct, int flag);
int sam_mkdir_vn(vnode_t *pvp, char *cp, struct vattr *vap, vnode_t **vpp,
	cred_t *credp, caller_context_t *ct, int flag, vsecattr_t *vsap);
int sam_rmdir_vn(vnode_t *pvp, char *cp, vnode_t *cdir, cred_t *credp,
	caller_context_t *ct, int flag);
int sam_readdir_vn(vnode_t *vp, struct uio *uiop, cred_t *credp, int *eofp,
	caller_context_t *ct, int flag);
int sam_symlink_vn(vnode_t *pvp, char *cp, struct vattr *vap, char *tnm,
	cred_t *credp, caller_context_t *ct, int flag);
int sam_readlink_vn(vnode_t *vp, struct uio *uiop, cred_t *credp,
	caller_context_t *ct);
int sam_fsync_vn(vnode_t *vp, int filemode, cred_t *credp,
	caller_context_t *ct);
void sam_inactive_vn(vnode_t *vp, cred_t *credp, caller_context_t *ct);
int sam_fid_vn(vnode_t *vp, struct fid *fidp, caller_context_t *ct);
void sam_rwlock_vn(vnode_t *vp, int w, caller_context_t *ct);
void sam_rwunlock_vn(vnode_t *vp, int w, caller_context_t *ct);
int sam_seek_vn(vnode_t *vp, offset_t ooff, offset_t *noffp,
	caller_context_t *ct);
int sam_frlock_vn(vnode_t *vp, int cmd, sam_flock_t *flp, int flag,
	offset_t offset, flk_callback_t *fcp, cred_t *credp,
	caller_context_t *ct);
int sam_space_vn(vnode_t *vp, int cmd, sam_flock_t *flp, int flag,
	offset_t offset, cred_t *credp, caller_context_t *ct);
int sam_getpage_vn(vnode_t *vp, offset_t offset, sam_size_t length,
	uint_t *protp, struct page **pglist, sam_size_t plsize,
	struct seg *segp, caddr_t addr, enum seg_rw rw, cred_t *credp,
	caller_context_t *ct);
int sam_putpage_vn(vnode_t *vp, offset_t offset, sam_size_t length, int flags,
	cred_t *credp, caller_context_t *ct);
int sam_map_vn(vnode_t *vp, offset_t offset, struct as *asp, caddr_t *addrpp,
	sam_size_t length, uchar_t prot, uchar_t maxprot, uint_t flags,
	cred_t *credp, caller_context_t *ct);
int sam_addmap_vn(vnode_t *vp, offset_t offset, struct as *asp, caddr_t a,
	sam_size_t length, uchar_t prot, uchar_t maxprot, uint_t flags,
	cred_t *credp, caller_context_t *ct);
int sam_delmap_vn(vnode_t *vp, offset_t offset, struct as *asp, caddr_t a,
	sam_size_t length, uint_t prot, uint_t maxprot, uint_t flags,
	cred_t *credp, caller_context_t *ct);
int sam_pathconf_vn(vnode_t *vp, int cmd, ulong_t *valp,
	cred_t *credp, caller_context_t *ct);
int sam_getsecattr_vn(vnode_t *vp, vsecattr_t *vsap, int flag,
	cred_t *credp, caller_context_t *ct);
int sam_setsecattr_vn(vnode_t *vp, vsecattr_t *vsap, int flag,
	cred_t *credp, caller_context_t *ct);

#else
/*
 * ----- Solaris 10 SAM-FS vnode function prototypes.
 */
int sam_open_vn(vnode_t **vpp, int, cred_t *credp);
int sam_close_vn(vnode_t *vp, int filemode, int count, offset_t offset,
	cred_t *credp);
int sam_read_vn(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *credp,
	caller_context_t *ct);
int sam_write_vn(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *credp,
	caller_context_t *ct);
int sam_ioctl_vn(vnode_t *vp, int cmp, sam_intptr_t arg, int flag,
	cred_t *credp, int *rvp);
int sam_getattr_vn(vnode_t *vp, struct vattr *vap, int flag, cred_t *credp);
int sam_setattr_vn(vnode_t *vp, struct vattr *vap, int flag, cred_t *credp,
	caller_context_t *ct);
int sam_access_vn(vnode_t *vp, int mode, int flag, cred_t *credp);
int sam_lookup_vn(vnode_t *vp, char *cp, vnode_t **vpp, struct pathname *pnp,
	int flag, vnode_t *rdir, cred_t *credp);
int sam_create_vn(vnode_t *vp, char *cp, struct vattr *vap, vcexcl_t ex,
	int mode, vnode_t **vpp, cred_t *credp, int filemode);
int sam_remove_vn(vnode_t *pvp, char *cp, cred_t *credp);
int sam_link_vn(vnode_t *pvp, vnode_t *vp, char *cp, cred_t *credp);
int sam_rename_vn(vnode_t *opvp, char *omn, vnode_t *npvp, char *nnm,
	cred_t *credp);
int sam_mkdir_vn(vnode_t *pvp, char *cp, struct vattr *vap, vnode_t **vpp,
	cred_t *credp);
int sam_rmdir_vn(vnode_t *pvp, char *cp, vnode_t *cdir, cred_t *credp);
int sam_readdir_vn(vnode_t *vp, struct uio *uiop, cred_t *credp, int *eofp);
int sam_symlink_vn(vnode_t *pvp, char *cp, struct vattr *vap, char *tnm,
	cred_t *credp);
int sam_readlink_vn(vnode_t *vp, struct uio *uiop, cred_t *credp);
int sam_fsync_vn(vnode_t *vp, int filemode, cred_t *credp);
void sam_inactive_vn(vnode_t *vp, cred_t *credp);
int sam_fid_vn(vnode_t *vp, struct fid *fidp);
void sam_rwlock_vn(vnode_t *vp, int w, caller_context_t *ct);
void sam_rwunlock_vn(vnode_t *vp, int w, caller_context_t *ct);
int sam_seek_vn(vnode_t *vp, offset_t ooff, offset_t *noffp);
int sam_frlock_vn(vnode_t *vp, int cmd, sam_flock_t *flp, int flag,
	offset_t offset, flk_callback_t *fcp, cred_t *credp);
int sam_space_vn(vnode_t *vp, int cmd, sam_flock_t *flp, int flag,
	offset_t offset, cred_t *credp, caller_context_t *ct);
int sam_getpage_vn(vnode_t *vp, offset_t offset, sam_size_t length,
	uint_t *protp, struct page **pglist, sam_size_t plsize,
	struct seg *segp, caddr_t addr, enum seg_rw rw, cred_t *credp);
int sam_putpage_vn(vnode_t *vp, offset_t offset, sam_size_t length, int flags,
	cred_t *credp);
int sam_map_vn(vnode_t *vp, offset_t offset, struct as *asp, caddr_t *addrpp,
	sam_size_t length, uchar_t prot, uchar_t maxprot, uint_t flags,
	cred_t *credp);
int sam_addmap_vn(vnode_t *vp, offset_t offset, struct as *asp, caddr_t a,
	sam_size_t length, uchar_t prot, uchar_t maxprot, uint_t flags,
	cred_t *credp);
int sam_delmap_vn(vnode_t *vp, offset_t offset, struct as *asp, caddr_t a,
	sam_size_t length, uint_t prot, uint_t maxprot, uint_t flags,
	cred_t *credp);
int sam_pathconf_vn(vnode_t *vp, int cmd, ulong_t *valp,
	cred_t *credp);
int sam_getsecattr_vn(vnode_t *vp, vsecattr_t *vsap, int flag,
	cred_t *credp);
int sam_setsecattr_vn(vnode_t *vp, vsecattr_t *vsap, int flag,
	cred_t *credp);

#endif	/* SAM-FS vnode function prototypes */


/*
 * Function prototypes for VNOPS for forcibly unmounted vnodes.
 * Most return EIO; a few allow freeing/deallocating of vnode
 * and/or vnode resources.
 *
 * Solaris 10 and above have a constructor function that allows
 * most of these declarations to be avoided.
 */
#if defined(SOL_511_ABOVE)
/*
 * ----- Solaris 11 SAM-FS stale vnode function prototypes.
 */
int sam_close_stale_vn(vnode_t *vp, int filemode, int count, offset_t offset,
	cred_t *credp, caller_context_t *ct);
void sam_inactive_stale_vn(vnode_t *vp, cred_t *credp, caller_context_t *ct);
int sam_delmap_stale_vn(vnode_t *vp, offset_t offset, struct as *asp, caddr_t a,
	sam_size_t length, uint_t prot, uint_t maxprot, uint_t flags,
	cred_t *credp, caller_context_t *ct);

#else
/*
 * ----- Solaris 10 SAM-FS stale vnode function prototypes.
 */
int sam_close_stale_vn(vnode_t *vp, int filemode, int count, offset_t offset,
	cred_t *credp);
void sam_inactive_stale_vn(vnode_t *vp, cred_t *credp);
int sam_delmap_stale_vn(vnode_t *vp, offset_t offset, struct as *asp, caddr_t a,
	sam_size_t length, uint_t prot, uint_t maxprot, uint_t flags,
	cred_t *credp);

#endif	/* SAM-FS stale vnode function prototypes */

#endif	/* _SAMFS_H */
