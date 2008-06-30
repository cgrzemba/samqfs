/*
 * ----- objorphan.h - Object Orphan List definition.
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

#ifndef _OBJORPHAN_H
#define	_OBJORPHAN_H

#define	DEFAULT_OSDT_ORPHANS	1024		/* Number Orphans slot */
#define	ORPHAN_LIST_VERSION	0x2D4F5250414E2d	/* -ORPHAN- */

struct obj_orphan_list_hdr {
	uint64_t	ool_version;
	kmutex_t	ool_kmutex;		/* Mutex lock */
	objnode_t	*ool_objp;		/* Object Node of Orphan file */
	uint64_t	ool_freelist;		/* First free Index */
	uint64_t	ool_active_count;	/* Number of Orphans */
	uint64_t	ool_list_count;		/* Number entries in list */
	uint64_t	*ool_listp;		/* Pointer to list */
} obj_orphan_list_hdr_t;

extern int sam_obj_orphan_init(sam_mount_t *mp, void **handle);
extern int sam_obj_orphan_close(void *handle);

#endif /* _OBJORPHAN_H */
