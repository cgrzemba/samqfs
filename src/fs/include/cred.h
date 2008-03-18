/*
 *	cred.h - SAM-QFS file system access to cred structure
 *
 *	Provides wrappers for cred access in pre-Solaris 10 systems.
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

#ifndef	_SAM_FS_CRED_H
#define	_SAM_FS_CRED_H

#ifdef sun
#pragma ident "$Revision: 1.12 $"
#endif


#ifndef SOL_510_ABOVE
/*
 * macros for cred access - Solaris 9.
 */
#define	crgetref(credp)			(credp->cr_ref)
#define	crgetuid(credp)			(credp->cr_uid)
#define	crgetruid(credp)		(credp->cr_ruid)
#define	crgetsuid(credp)		(credp->cr_suid)
#define	crgetgid(credp)			(credp->cr_gid)
#define	crgetrgid(credp)		(credp->cr_rgid)
#define	crgetsgid(credp)		(credp->cr_sgid)
#define	crgetngroups(credp)		(credp->cr_ngroups)
#define	crgetgroups(credp)		(credp->cr_groups)
/*
 * macros for security policies - Solaris 9.
 */
#define	secpolicy_fs_config(credp, vfsp)	(suser(credp) ? 0 : EPERM)
#define	secpolicy_fs_mount(credp, mvp, vfsp)	(suser(credp) ? 0 : EPERM)
#define	secpolicy_fs_unmount(credp, vfsp)	(suser(credp) ? 0 : EPERM)
#define	secpolicy_fs_linkdir(credp, vfsp)	(suser(credp) ? 0 : EPERM)
#define	secpolicy_vnode_stky_modify(credp)	crgetuid(credp)
#define	secpolicy_fs_quota(credp, vfsp)		(suser(credp) ? 0 : EPERM)
#define	secpolicy_vnode_create_gid(credp)	(suser(credp) ? 0 : EPERM)
#define	secpolicy_vnode_owner(credp, uid) \
		(((credp->cr_uid != uid) && !suser(credp)) ? EPERM : 0)
#define	secpolicy_vnode_remove(credp)		(suser(credp) ? 0 : EPERM)
#define	secpolicy_vnode_setdac(credp, uid) \
		(((credp->cr_uid != uid) && !suser(credp)) ? EPERM : 0)
#define	secpolicy_vnode_setids_setgids(credp, gid) \
		(suser(credp) ? 0 : EPERM)
#define	secpolicy_vnode_setid_retain(credp, su) \
		(suser(credp) ? 0 : EPERM)
#endif

#endif /* _SAM_FS_CRED_H */
