/*
 * file_defs.h - stage file defInitions
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

#ifndef FILE_DEFS_H
#define	FILE_DEFS_H

#pragma ident "$Revision: 1.31 $"

#define	IF_COPY_DISKARCH(f, copy)  \
	((f->ar[copy].flags & STAGE_COPY_DISKARCH) != 0)

/*
 *  File status flags.
 */
enum {
	FI_ACTIVE =		1 << 0,	/* staging is active */
	FI_DONE	=		1 << 1,	/* staging has completed */
	FI_NO_TARHDR =		1 << 2,	/* no tar header to verify */
	FI_MULTIVOL =		1 << 3,	/* multivolume stage */
	FI_CANCEL =		1 << 4,	/* cancel stage */
	FI_RETRY =		1 << 5,	/* retry stage */
	FI_DCACHE =		1 << 6,	/* disk cache file open */
	FI_POSITIONING =	1 << 7,	/* positioning removable media */
	FI_OPENING =		1 << 8,	/* stage syscall response in progress */
	FI_EXTENDED =		1 << 9,	/* extended request for multivolume */
	FI_USE_CSUM =		1 << 10, /* use checksum value for file */
	FI_WRITE_ERROR =	1 << 11, /* swrite error to disk cache */
	FI_DUPLICATE =		1 << 12, /* duplicate stage */
	FI_TAR_ERROR =		1 << 13, /* tar header is invalid */
	FI_EXTENSION =		1 << 14, /* multivolume extension block */
	FI_STAGE_NEVER =	1 << 15, /* stage never attribute */
	FI_STAGE_PARTIAL =	1 << 16, /* partial stage attribute */
	FI_DCACHE_CLOSE	=	1 << 17, /* close dcache */
	FI_NO_RETRY =		1 << 18, /* not retry stage */
	FI_ORPHAN =		1 << 19, /* active in orphaned copy proc */
	FI_DATA_VERIFY =	1 << 20, /* data verification */
	FI_PAX_TARHDR =		1 << 21	 /* copy made with pax tar header */
};

/* Structures. */

/*
 * File extent header information.
 */
typedef struct FileExtentHdrInfo {
	uint32_t	fh_magic;	/* magic number */
	uint32_t	fh_version;	/* version number */
	time_t		fh_create;
	int		fh_alloc;
	int		fh_entries;
} FileExtentHdrInfo_t;

#define	FILE_EXTENT_MAGIC	05041501
#define	FILE_EXTENT_VERSION	80601	/* YMMDD */

/*
 * File extent information.
 * Extention to file information structure.
 */
typedef struct FileExtentInfo {
	sam_id_t	fe_id;		/* file identification */
	int		fe_extOrd;	/* extension ordinal */
	equ_t		fe_fseq;	/* filesystem equipment ordinal */
	int		fe_count;	/* active count */
	/*
	 * To save memory, the FileInfo structure contains only one
	 * vsn record.  If a multivolume stage request, all
	 * volume sections for each archive copy are saved here.
	 */
	sam_stage_copy_t fe_ar[MAX_ARCHIVE];

} FileExtentInfo_t;

#endif /* FILE_DEFS_H */
