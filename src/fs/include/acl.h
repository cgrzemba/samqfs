/*
 *	acl.h - SAM-FS file system access control list
 *
 *	Defines the structure of SAM-FS access control list.
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

#ifndef	_SAM_FS_ACL_H
#define	_SAM_FS_ACL_H

#ifdef sun
#pragma ident "$Revision: 1.17 $"
#endif


/*
 * Check that the file type is one that accepts ACLs
 */
#define	CHECK_ACL_ALLOWED(MODE)	   (((MODE) == S_IFDIR) || \
				((MODE) == S_IFREG) || \
				((MODE) == S_IFIFO) || \
				((MODE) == S_IFCHR) || \
				((MODE) == S_IFBLK) || \
				((MODE) == S_IFREQ))
/*
 * Conversion macros for incore acl to vnode secattr form.
 */
#define	ACL_VSEC(TYPE, NUM, FLD, PTR) \
	{							\
		int i;						\
		sam_acl_t *x_entp = (FLD);			\
		for (i = 0; i < (NUM); i++) {			\
			ASSERT(x_entp != NULL);			\
			ASSERT(x_entp->a_type == (TYPE));	\
			(PTR)->a_type = (TYPE);			\
			(PTR)->a_id   = x_entp->a_id;		\
			(PTR)->a_perm = x_entp->a_perm;		\
			(PTR)++;				\
			x_entp++;				\
		}						\
	}

#define	ACL_VSEC_CLASS(TYPE, FLD, PTR) \
	if ((FLD).is_def) {					\
		(PTR)->a_type = (TYPE);				\
		(PTR)->a_id   = 0;				\
		(PTR)->a_perm = (FLD).bits;			\
		(PTR)++;					\
	}

#define	MODE_CHECK(mode, perm) (((mode) & (perm)) != (mode))

/*
 * Flags for sam_check_acl
 */
#define	ACL_CHECK		0x01
#define	DEF_ACL_CHECK	0x02

/*
 * Get ACL group permissions if the mask is not present, and the ACL
 * group permission intersected with the mask if the mask is present
 */
#define	MASK2MODE(ACL)						\
	((ACL)->acl.mask.is_def ?				\
		((((ACL)->acl.mask.bits &			\
			(ACL)->acl.group->a_perm) & 07) << 3) :	\
		(((ACL)->acl.group->a_perm & 07) << 3))

/*
 * In core access control list entry, the same like aclent_t form sys/acl.h for UFS ACL, ZFS ACL ace_t is 2Byte longer!
 */
typedef struct sam_acl {
	int			a_type;		/* the type of ACL entry */
	uid_t			a_id;		/* the entry in -uid or gid */
	ushort_t		a_perm;		/* the permission field */
} sam_acl_t;

/*
 * In core access control list mask
 */
typedef struct sam_aclmask {
	short			is_def;		/* Is mask defined? */
	mode_t			bits;		/* Permission mask */
} sam_aclmask_t;

/*
 * In core access control list table
 */
typedef struct sam_acltab {
	int			cnt;		/* number of ACL entries */
	sam_acl_t		*entp;		/* 1st access cntrl list ent */

	sam_acl_t		*owner;		/* owner object */
	sam_acl_t		*group;		/* group object */
	sam_acl_t		*other;		/* other object */
	int			nusers;		/* count of users */
	sam_acl_t		*users;		/* sublist of users */
	int			ngroups;	/* count of groups */
	sam_acl_t		*groups;	/* sublist of groups */
	sam_aclmask_t		mask;		/* mask */
} sam_acltab_t;

/*
 * In core access control list
 */
typedef struct sam_ic_acl {
	sam_id_t	id;		/* Id/generation of owning inode */
	int		flags;		/* See below */
	int		size;		/* Alloc'd size of incore ACL struct */

	sam_acltab_t	acl;		/* regular ACLs */
	sam_acltab_t	dfacl;		/* default ACLs */

	sam_acl_t	ent[1];		/* regular & default ACLs list */
} sam_ic_acl_t;


/* Flags field values */
#define	ACL_MODIFIED	0x01	/* In core ACL struct modified */


#endif /* _SAM_FS_ACL_H */
