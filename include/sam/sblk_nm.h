/*
 * sblk_nm.h - Superblock Value <-> String Conversion Tables
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

#ifndef _SAM_SBLK_NM_H
#define	_SAM_SBLK_NM_H

#ifdef sun
#pragma ident "$Revision: 1.14 $"
#endif

/* Superblock version definitions for display */
#define	SAMFS_SBLKV1_STR		"sbv1"
#define	SAMFS_SBLKV2_STR		"sbv2"

/* FSCK status for display */
#define	SB_FSCK_ALL_STR			"fsck-all"
#define	SB_FSCK_SP_STR			"fsck-slice"
#define	SB_FSCK_GEN_STR			"fsck-general"
#define	SB_FSCK_NONE_STR		"clean"

/* Number <-> string conversion tables */

typedef struct {
	char *nm;		/* Character string */
	unsigned long val;	/* Number */
} sb_str_num_t;

/* Superblock version */
sb_str_num_t	_sb_version[] = {
	{SAMFS_SBLKV1_STR, SAMFS_SBLKV1},
	{SAMFS_SBLKV2_STR, SAMFS_SBLKV2},
	{"", 0}
};

/* FSCK status */
sb_str_num_t	_fsck_status[] = {
	{SB_FSCK_ALL_STR, SB_FSCK_ALL},
	{SB_FSCK_SP_STR, SB_FSCK_SP},
	{SB_FSCK_GEN_STR, SB_FSCK_GEN},
	{SB_FSCK_NONE_STR, SB_FSCK_NONE},
	{"", 0}
};

#endif /* _SAM_SBLK_NM_H */
