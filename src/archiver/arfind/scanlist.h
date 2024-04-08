/*
 * scanlist.h - Arfind scanlist definitions.
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

#ifndef SCANLIST_H
#define	SCANLIST_H

#pragma ident "$Revision: 1.22 $"

/* Macros. */
#define	SCANLIST_MAGIC 0122072714	/* Scanlist file magic number */
#define	SCANLIST_VERSION 61211		/* Scanlist file version (YMMDD) */

/* The scan list. */
/* A memory mapped file. */
struct ScanList {
	MappedFile_t Sl;
	int	SlVersion;		/* Version */
	size_t	SlSize;			/* Size of all entries */
	int	SlCount;		/* Number of entries */
	int	SlFree;			/* Number of freed entries */
	ExamMethod_t SlExamine;		/* For showqueue */
	struct ScanListEntry {
		sam_id_t SeId;		/* Inode number */
		sam_time_t SeTime;	/* Time to process entry */
		int	 SeAsn;		/* Archive Set number */
		ushort_t SeCopies;  /* Archive copy bits expected during scan */
		ushort_t SeFlags;
	} SlEntry[1];
};

/* Flags. */
#define	SE_back		0x0001		/* Background scan */
#define	SE_free		0x0002		/* Free entry */
#define	SE_full		0x0004		/* Full scan */
#define	SE_inodes	0x0008		/* Inodes scan */
#define	SE_noarch	0x0010		/* Stop on 'noarchive' directory */
#define	SE_request	0x0020		/* Scan requested */
#define	SE_scanning	0x0040		/* Scannning this entry */
#define	SE_stats	0x0080		/* Produce statistics */
#define	SE_subdir	0x0100		/* Scan subdirectories */

#endif /* SCANLIST_H */
