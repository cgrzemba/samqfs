/*
 *	csd.h -  Control structure dump record definitions.
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.7 $"

#ifndef	SAM_CSD_H
#define	SAM_CSD_H

#include "aml/tar.h"

#define	CSD_MAGIC	0x63647378	/* Dump identifier word "cdsx"	*/
					/* was supposed to be "csdx", but */
					/* has been "cdsx", so won't change */
					/* it. */
#define	CSD_MAGIC_RE	0x78736463	/* Reverse-endian dump identifier */
#define	CSD_VERS_2	2		/* version number 2	*/
#define	CSD_VERS_3	3		/* version number 3 */
#define	CSD_VERS_4	4		/* version number 4 */
#define	CSD_VERS_5	5		/* version number 5 */
#define	CSD_VERS	CSD_VERS_4

/* ----	csd header record - Dump header record.			*/
/* ---- Any valid version should begin with this header	*/

typedef struct csd_header {
	int magic;
	int version;
	sam_time_t time;
} csd_header_t;

typedef struct csd_header csd_hdr_t;

/* ----	csd header record (extended) - Dump header record.	*/
/* ---- Any version greater than CSD_VERS_4 should begin	*/
/* ---- with this header.					*/

typedef struct csd_header_extended {
	csd_header_t	csd_header;
	int32_t			csd_header_flags;
	longlong_t		csd_header_pad[10];
	int				csd_header_magic;
	char			pad[4];
} csd_header_extended_t;

typedef	struct	csd_header_extended	csd_hdrx_t;

/*
 *	csd header flags definitions.
 */
#define	CSD_H_NOFILEDATA	0x00001	/* dump contains no */
					/* embedded file data. */
#define	CSD_H_FILEDATA		0x00002	/* dump may contain */
					/* embedded file data. */
#define	CSD_H_VALID		0x00002	/* Restore supports */
					/* these flags. */

typedef struct csd_filehdr {		/* csd version 5 and greater */
	int32_t	magic;			/* "csdf" */
	int32_t	flags;			/* file flags */
	int32_t	namelen;		/* name length (name follows) */
} csd_filehdr_t;

#define	CSD_MAX_EXCLUDED	10	/* maximum excluded dirs */
#define	CSD_MAX_INCLUDED	10	/* maximum included files */

typedef struct csd_filehdr csd_fhdr_t;

#define	CSD_FMAGIC	0x63736466		/* file header "csdf"	*/
#define	CSD_FH_DATA	0x00001			/* dump contains data	*/
#define	CSD_FH_PAD	0x00002			/* file header is a pad	*/
						/* "namelen" bytes long	*/

#define	CSD_DEFAULT_BUFSZ	0x40000		/* default buffer size */
#define	CSD_MIN_BUFSZ		0x40000
#define	CSD_MAX_BUFSZ		0xa00000

/*
 * Modified during 1997/01 to handle files with archive copies that
 * overflow volumes, as well as to clean up the restore interface.
 * Currently, the request command and the filesystem does not
 * support more than 8 VSNS. Dump format 4 is not tied to this
 * limitation.
 *
 * dump format:
 * version 4			version 3		version 2
 *
 * For directories, regular file, symlinks, & removable media file.
 * int namelen			int namelen		int namelen
 * char name[namelen]		char name[namelen]	char name[namelen]
 * struct sam_perm_inode	struct sam_perm_inode	struct sam_perm_inode
 *		 optional fields:
 *
 * version 5
 * For directories, regular file, symlinks, & removable media file.
 * int32_t magic
 * int32_t flags
 * int32_t namelen
 * char name[namelen]
 * struct sam_perm_inode
 *		optional fields:
 *
 * For a regular file with embedded data (version 5 and greater),
 * One or more tar-like headers, followed by file data.
 *
 * For a regular file that has archive copies where n_vsns > 1.
 * arcopy copy: struct sam_section[0]
 *	...
 * struct sam_section[n_vsns[copy]]
 * arcopy copy: struct sam_section[0]
 *	...
 * struct sam_section[n_vsns[copy]]
 *
 * For a segment file, repeat until all segment inodes:
 * struct sam_perm_inode
 * For a segment inode that has archive copies where n_vsns > 1.
 * arcopy copy: struct sam_section[0]
 *	...
 * struct sam_section[n_vsns[copy]]
 * arcopy copy: struct sam_section[0]
 *	...
 * struct sam_section[n_vsns[copy]]
 *
 * For a symlink
 * int linklen			int linklen		int linklen
 * char link[namelen]		char link[namelen]	char link[namelen]
 *
 * For a removable media file
 * sam_resource_file_t		old_sam_rminfo_t	old_sam_resource_file_t
 */


/*
 *	A note on dump version (contained in the dump header at the beginning
 *	of a non-headerless csd dump file):
 *
 *	Versions 0 and 1 were IDS format.
 *	Version 2 is Solaris format with sam_old_resource_file information
 *	Version 3 is Solaris format with sam_old_rminfo information
 *	Version 4 is Solaris format with sam_resource_file information and
 *	volume overflow handling.
 *	Version 5 is a new Solaris format with embedded data (csd info
 *	followed by a tar header, then file data).
 */

#endif	/* SAM_CSD_H */
