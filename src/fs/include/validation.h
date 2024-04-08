/*
 *	validation.h - SAM-QFS file system validation record.
 *
 *	This record is written when the file is updated and verified
 *  when the file is used.
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
 * or https://illumos.org/license/CDDL.
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

#ifndef	_SAM_FS_VALIDATION_H
#define	_SAM_FS_VALIDATION_H

#ifdef sun
#pragma ident "$Revision: 1.16 $"
#endif

#define	SAM_VAL_VERSION	(2)			/* SAM-FS validation version */

/*
 *  SAM file system validation record.
 */

typedef struct sam_val {
	ushort_t	v_version;	/* Version number */
	ushort_t	v_relsize;	/* Relative valid data size in block */
	sam_time_t	v_time;		/* Time record was last modified */
	offset_t	v_size;		/* Size of the file */
	sam_id_t	v_id;		/* Owner Id - i-number/generation */
} sam_val_t;

/*
 * Segment validation parameters.
 * v_size is the size of the index in multiples of blocks (SAM_SEG_BLK).
 * v_relsize is the relative size of valid data in the block.
 * For example, if 1 segment, v_size = 4096, v_relsize = 8;
 */

#define	SAM_SEG_BLK	(4096)		/* Size of segment index block */
#define	SAM_SEG_VAL_ORD	((sizeof (sam_val_t) + sizeof (sam_id_t) - 1) / \
			sizeof (sam_id_t))
#define	SAM_MAX_SEG_ORD ((SAM_SEG_BLK / sizeof (sam_id_t)) - SAM_SEG_VAL_ORD)


#endif	/* _SAM_FS_VALIDATION_H */
