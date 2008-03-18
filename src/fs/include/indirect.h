/*
 *	indirect.h - SAM-QFS file system indirect block definitions.
 *
 *	Defines the structure of indirect block for the SAM file system.
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

#ifndef	_SAM_FS_INDIRECT_H
#define	_SAM_FS_INDIRECT_H

#ifdef sun
#pragma ident "$Revision: 1.12 $"
#endif

#ifdef	linux
#include <linux/types.h>
#else	/* linux */
#include <sys/types.h>
#endif	/* linux */

#include "sam/types.h"
#include "sam/param.h"

struct sam_get_extent_args {
	sam_dau_t *dau;		/* Pointer to DAU table for this ordinal */
	int num_group;		/* Number of elements in the stripe group */
				/* for this ord */
	int sblk_vers;		/* Superblock version */
	ino_st_t status;	/* Ino status */
	offset_t offset;	/* Logical byte offset in file (longlong_t) */
};

struct sam_get_extent_rval {
	int *de;			/* Direct extent-- returned */
	int *dtype;			/* Device data or meta -- returned */
	int *btype;			/* Small/Large extent-- returned */
	int *ileft;			/* Number of extents left-- returned */
};

/*
 * ----- Indirect block function prototypes.
 */

int  sam_get_extent(struct sam_get_extent_args *arg, int kptr[],
					struct sam_get_extent_rval *rval);

#endif	/*  _SAM_FS_INDIRECT_H */
