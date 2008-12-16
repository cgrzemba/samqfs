/*
 * sblk_show.h - Show Superblock Information
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

#ifndef _SAM_SBLK_SHOW_H
#define	_SAM_SBLK_SHOW_H

#ifdef sun
#pragma ident "$Revision: 1.14 $"
#endif

/* Unrecognized superblock version */
#define	SAMFS_SBLK_UNKNOWN_STR  "unknown"

/* Filesystem name definitions */
#define	FSNAME_UNKNOWN		"unknown"
#define	FSNAME_SAM_QFS_SBV1	"sam-qfs-sbv1"
#define	FSNAME_SAM_QFS_SBV2	"sam-qfs-sbv2"
#define	FSNAME_SAM_QFS_SBV2A	"sam-qfs-sbv2A"

/* Unrecognized superblock name */
#define	SAMFS_SB_NAME_UNKNOWN_STR	""

/* Unrecognized FSCK status */
#define	SB_UNKNOWN_STR		"unknown"

/* Declare Superblock Display Library Functions */

int
sam_sbinfo_format(struct sam_sbinfo *sbip, sam_format_buf_t *bufp);

int
sam_sbord_format(struct sam_sbord *sbop, sam_format_buf_t *bufp);

int
sam_inodes_format(struct sam_perm_inode *pip, sam_format_buf_t *bufp);

int
sam_host_table_format(struct sam_host_table *htp, sam_format_buf_t *bufp);

int
sam_sbversion_to_str(unsigned long sbversion, char *strp, int len);

int
sam_sbname_to_str(char name[], int len_from, char *strp, int len_to);

int
sam_fstype_to_str(unsigned long fstype, char *strp, int len);

int
sam_fsstate_to_str(unsigned long fsstate, char *strp, int len);

int
sam_sstate_to_str(unsigned long sstate, char *strp, int len);

int
sam_devtype_to_str(unsigned long devtype, char *strp, int len);
#endif /* _SAM_SBLK_SHOW_H */
