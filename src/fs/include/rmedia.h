/*
 *	rmedia.h - SAM-QFS file system disk rmedia definitions.
 *
 *	Defines the structure of disk rmedia for the SAM file system.
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

#ifndef	_SAM_FS_RMEDIA_H
#define	_SAM_FS_RMEDIA_H

#ifdef sun
#pragma ident "$Revision: 1.24 $"
#endif

#ifdef linux
#ifdef __KERNEL__
#include	<linux/types.h>
#else
#include	<sys/types.h>
#endif /* __KERNEL */

#else /* linux */
#include	<sys/types.h>
#endif /* linux */

/*
 * Inherit mask for removable media flag (rm.ui.flags)
 * RM_CHAR_DEV_FILE and RM_DATA_VERIFY are inheritable
 */
#define	RM_INHERIT_MASK (RM_DATA_VERIFY | RM_CHAR_DEV_FILE)

/*
 * ----- Inode removable media, segment status, and aio attr flag.
 *
 * sam_rminfo_t in resource.h must match.  Eventually, make
 * resource.h use sam_rm_t.
 */

#define	RM_VALID		0x8000 /* Data is valid */
#define	RM_STRANGER		0x4000 /* Stranger tape removable media file */
#define	RM_BOF_WRITTEN		0x2000 /* BOF has been written */
#define	RM_FILE_WRITTEN		0x1000 /* File has been written */

#define	RM_PROCESS_WTM		0x0800 /* Only write a tape mark */
#define	RM_BLOCK_IO		0x0400 /* Optical/Tape access via direct I/O */
#define	RM_PARTIAL_PDU		0x0200 /* User wrote a partial pdu */
#define	RM_PROCESS_EOX		0x0100 /* Write an end of section label */

#define	RM_UNUSED1		0x0080
#define	RM_UNUSED2		0x0040
#define	RM_DATA_VERIFY		0x0020 /* Data verification required */
#define	RM_VOLUMES		0x0010 /* More than 1 volume specified */

#define	RM_BUFFERED_IO		0x0008 /* Optical/Tape access via buf'd I/O */
#define	RM_OBJECT_FILE		0x0004 /* Object file */
#define	RM_CHAR_DEV_FILE	0x0002 /* File mapped to char device for aio */
#define	RM_FILEMARK		0x0001 /* Filemark encountered */

#ifndef BYTE_SWAP

typedef union rm_status {
	ushort_t	flags;		/* Removable Media File status flags */
	ushort_t	extent_flags;	/* Extent flags for direct extents */
} rm_status_t;

typedef struct  {
	offset_t	size;	/* Size of file */
	media_t		media;	/* Media type */
	rm_status_t	ui;	/* Extent flags; Removable Media flag bits */
	union {
		struct rminfo {
			uint_t	file_offset;	/* File starting byte offset */
			uint_t	mau;		/* Media allocation unit */
			uint_t	position;	/* Position of file */
		} rm;
		struct seginfo {
			uint_t	allocahead;	/* Allocahead size */
			uint_t	seg_size;	/* Segment size in megabytes */
			union {
				int fsize; /* Seg idx size, seg_file only */
				uint_t ord;	/* Seg ord - seg_ino only */
			} seg;
		} dk;
	} info;
} sam_rm_t;

#else

/*
 * Kluged version to keep the byte-swapping logic
 * going in spite of the union (allowable here
 * only because the union's fields are all the
 * same sizes in the different unions).
 */
typedef struct sam_rm {
	offset_t	size;			/* Size of file */
	media_t		media;			/* Media type */
	ushort_t	flags;			/* bit fields */
	/*
	 * fields below are 'unioned' in the actual
	 * structure.  Happily, they're all the same
	 * sizes in the different unions, and so can
	 * be byte swapped without further knowledge
	 * of the particular internals.
	 */
	uint_t	file_offset;		/* File starting byte offset */
	uint_t	mau;				/* Media allocation unit */
	uint_t	position;			/* Position of file */
} sam_rm_t;

#endif

#endif /* _SAM_FS_RMEDIA_H */
