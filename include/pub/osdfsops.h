/*
 * ----- osdfsops - Object file system functions.
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

#ifndef _OSDFSOPS_H
#define	_OSDFSOPS_H

/*
 * Object File System level operation vector.
 */
typedef struct osdfsops osdfsops_t;

/*
 * The osdfs structure works like the vfs_t.
 * osdfs_op should not be used directly.  Accessor functions are provided.
 */
typedef struct osdfs {
	osdfsops_t	*osdfs_op;	/* operations on OSDFS */
	void		*osdfs_data;	/* private data - ptr to SAMQFS mp */
	uint64_t	osdfs_lun;	/* Logical Unit Number */
} osdfs_t;

/*
 * OSDFS_OPS defines all the osdfs operations.  It is used to define
 * the osdfsops structure (below).
 */
#define	OSDFS_OPS							\
	int	(*osdfs_verify)(osdfs_t *, cred_t *);			\
	int	(*osdfs_lunattach)(osdfs_t *, char *, cred_t *);	\
	int	(*osdfs_lundetach)(osdfs_t *, int, cred_t *);		\
	int	(*osdfs_oget)(osdfs_t *, uint64_t, uint64_t,		\
				objnode_t **);				\
	int	(*osdfs_paroget)(osdfs_t *, uint64_t,			\
				objnode_t **);				\
	int	(*osdfs_sync)(osdfs_t *, short, cred_t *);		\
	int	(*osdfs_get_rootobj)(osdfs_t *, objnode_t **, cred_t *)	\
	/* NB: No ";" */

/*
 * Operations supported by Object Storage Device file system.
 */
struct osdfsops {
	OSDFS_OPS;	/* Signature of all osdfs operations (osdfsops) */
};

#define	OSDFS_VERIFY(osdfsp, cr)		\
	(*(osdfsp)->osdfs_op->osdfs_verify)(osdfsp, cr);

#define	OSDFS_LUNATTACH(osdfsp, mntpnt, cr)	\
	(*(osdfsp)->osdfs_op->osdfs_lunattach)(osdfsp, mntpnt, cr);

#define	OSDFS_LUNDETACH(osdfsp, flag, cr)	\
	(*(osdfsp)->osdfs_op->osdfs_lundetach)(osdfsp, flag, cr);

#define	OSDFS_OGET(osdfsp, parid, objid, objpp)	\
	(*(osdfsp)->osdfs_op->osdfs_oget)(osdfsp, parid, objid, objpp);

#define	OSDFS_PAROGET(osdfsp, parid, objpp)	\
	(*(osdfsp)->osdfs_op->osdfs_paroget)(osdfsp, parid, objpp);

#define	OSDFS_SYNC(osdfsp, flag, cr)		\
	(*(osdfsp)->osdfs_op->osdfs_sync)(osdfsp, flag, cr);

#define	OSDFS_GET_ROOTOBJ(osdfsp, objpp, cr)	\
	(*(osdfsp)->osdfs_op->osdfs_get_rootobj)(osdfsp, objpp, cr);

#endif /* _OSDFSOPS_H */
