/*
 * old_resource.h - SAM-FS removable media resource definitions.
 *
 *	Contains structures and definitions for old removable media
 *	files. Version 2 and version 2 dump records are defined.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#ifndef _SAM_OLD_RESOURCE_H
#define	_SAM_OLD_RESOURCE_H

#pragma ident "$Revision: 1.9 $"

#include "sam/types.h"

#define	SAM_OLD_RESOURCE_SIZE(n) \
			(sizeof (sam_resource_file_t)+((n)-1)*sizeof (vsn_t))

#define	SAM_OLD_ARCHIVE_GROUP	"sam_archive"
#define	SAM_OLD_ARCHIVE_OWNER	"sam_archive"


/* Version 2 */
/* Removable media information record. */

typedef struct	sam_old_arch_rminfo {
	offset_t	size;			/* Size of file */
	media_t		media;			/* Media type */
#ifndef BYTE_SWAP
	ushort_t
		valid		: 1,	/* Data is valid if 1 */
		incomplete_file : 1,	/* File is incomplete */
		bof_written	: 1,	/* BOF has been written */
		file_written	: 1,	/* File has been written */
		mmap_io		: 1,	/* Optical access via mmap */
		fifo_io		: 1, /* Tape should be accessed through FIFO */
		partial_pdu	: 1,	/* User wrote a partial pdu */
		process_eox	: 1, /* Wr end of section label&volume swap */
		nolinger	: 1, /* Stage request issued with no linger */
		archive		: 1,	/* Resource file is archive image */
		stage_n		: 1;	/* Stage never -- ignore tar header */
#else	/* BYTE_SWAP */
	ushort_t	flags;
#endif	/* BYTE_SWAP */
	uint_t		file_offset;	/* File starting byte offset */
	uint_t		mau;		/* Media allocation unit */
	uint_t		position;	/* Position of file */
} sam_old_arch_rminfo_t;


/* Removable media archive record. */

typedef struct sam_old_archive {
	sam_old_arch_rminfo_t rm_info;	/* Removable media ino info */
	sam_time_t	creation_time;	/* Creation time */
	short		n_vsns;		/* Number of VSNs for entry */
	vsn_t		vsn;		/* Volume serial number */
	char		pad[2];
#ifndef BYTE_SWAP
	union {					/* Media class */
		struct {			/* Optical archive entry: */
			int32_t	label_pda;	/* File label phys disk addr */
			int32_t	version;	/* Version number */
			char	file_id[32];	/* Recorded file name */
		} od;
	} mc;
#else	/* BYTE_SWAP */
	int32_t	label_pda;		/* File label physical disk addr */
	int32_t	version;		/* Version number */
	char	file_id[32];		/* Recorded file name */
#endif	/* BYTE_SWAP */
} sam_old_archive_t;


/* Removable media resource record. */

typedef struct sam_old_resource {
	short		revision;	/* Revision control number */
	char		access;		/* Open access control */
	char		protect;	/* Write protect flag */
	char		pad1[4];
	sam_old_archive_t archive;	/* Archive record */
	vsn_t		next_vsn;	/* Next VSN */
	vsn_t		prev_vsn;	/* Previous VSN */
	uint32_t	required_size;	/* Required size in 1024b blocks */
	char		pad2[4];
#ifndef BYTE_SWAP
	union {				/* Media class */
		struct {		/* Optical resource entry: */
		char	group_id[32];	/* Group identifier */
		char	owner_id[32];	/* Owner identifier, blank=def */
		char	info[160];	/* User information */
		} od;
	} mc;
#else	/* BYTE_SWAP */
	char	group_id[32];		/* Group identifier */
	char	owner_id[32];		/* Owner identifier, blank=def */
	char	info[160];		/* User information */
#endif	/* BYTE_SWAP */
} sam_old_resource_t;


/* Removable media resource file structure. */

typedef struct sam_old_resource_file {
	sam_old_resource_t	resource;
	int			cur_ord;	/* Current VSN ordinal */
	int			n_vsns;		/* Num VSNs in VSN list */
	vsn_t			vsn_list[1];	/* VSN list */
} sam_old_resource_file_t;


/* Version 3 */
/*
 * rminfo.h - SAM-FS removable media file access definitions.
 *
 * Defines the SAM-FS removable media file access structure and functions.
 *
 */

#define	OLD_MAX_VSNS 8

typedef struct sam_old_section {
	char	vsn[32];	/* VSN */
	u_longlong_t position;	/* Position on this volume */
	u_longlong_t offset;	/* Section offset of file on this volume */
	u_longlong_t length;	/* Section length of file on this volume */
} sam_old_section_t;

typedef struct sam_old_rminfo {
	ushort_t flags;			/* Access flags */
	char	media[4];		/* Media type */
	uint32_t	creation_time;	/* Time file created */
	uint_t	block_size;		/* Media block size */
	u_longlong_t position;		/* Current position on the media */
	u_longlong_t required_size;	/* Required size on a request */

	/* For optical media */
	char	file_id[32];	/* Recorded file name */
	int	version;	/* Version number */
	char	owner_id[32];	/* Owner identifier */
	char	group_id[32];	/* Group identifier */
	char	info[160];	/* User information */

	/* For all media. */
	short	n_vsns;		/* Number of VSNs available */
	short	c_vsn;		/* Current VSN */
	sam_old_section_t section[OLD_MAX_VSNS];	/* VSNs information */
} sam_old_rminfo_t;

#endif	/* _SAM_OLD_RESOURCE_H */
