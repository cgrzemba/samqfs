/*
 * sblk_mgmt.h - Definitions for superblock management functions
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

#ifndef SAM_SBLK_MGMT_H
#define	SAM_SBLK_MGMT_H

#ifdef sun
#pragma ident "$Revision: 1.15 $"
#endif

/* Flags for device open restrictions. */
#define	DEV_FLAGS_NONE	0x00
#define	DEV_FLAGS_BLK	0x01	/* Must be block device */
#define	DEV_FLAGS_CHR	0x02	/* Must be character device */

/* Filesystem type definitions */
#define	FSTYPE_UNKNOWN		0	/* Unknown filesystem type */
#define	FSTYPE_SAM_QFS_SBV1	1	/* SAM-QFS w/ Superblock v1 */
#define	FSTYPE_SAM_QFS_SBV2	2	/* SAM-QFS w/ Superblock v2 */
#define	FSTYPE_SAM_QFS_SBV2A	3	/* SAM-QFS w/ Superblock v2A */

/* Declarations for superblock management library functions. */

int
sam_dev_fd_get(char *devp, unsigned long flags, unsigned long mode, int *fdp);

int
sam_fd_block_get(int fd, unsigned long byte_offset, void *bp, int len);

int
sam_fd_sb_get(int fd, struct sam_sblk *sbp);

int
sam_dev_sb_get(char *devp, struct sam_sblk *sbp, int *fdp);

int
sam_sb_version_get(struct sam_sblk *sbp, unsigned long *version);

int
sam_sb_name_get(struct sam_sblk *sbp, char *strp, int len);

int
sam_sb_fstype_get(struct sam_sblk *sbp, unsigned long *ftp);

int
sam_sb_devtype_get(struct sam_sblk *sbp, unsigned long *devtype);

int
sam_inodes_get(int fd, struct sam_perm_inode *pip);

#endif /* SAM_SBLK_MGMT_H */
