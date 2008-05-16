/*
 * sam/resource.h - SAM-FS removable media resource definitions.
 *
 *  Contains structures and definitions for archive files and
 *  resource files.
 *
 *  sam_resource_file_t:
 *      The resource file record is built by the request
 *      command and stored in the first extent of the resource file.
 *      It describes all the vsns for the removable media file.
 *  sam_resource_t:
 *      The resource record describes the current instance of the
 *      volume serial number and file on it.
 *  sam_archive_t:
 *      The resource record contains the archive record. For an archive
 *      file, there are a possible 4 archive records, 2 per inode.
 *      The archive record contains all the information necessary to
 *      retrive the archived image.
 *  sam_rminfo_t:
 *      When an archived file or resource file is opened, the removable
 *      media info record is moved into the inode and used by the
 *      file system to access the removable media data.
 *  sam_stage_request_t
 *      The filesystem responds to the stager system call by posting
 *      this request structure.  The request contains the inode
 *      information describing all four possible copies.
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

#ifndef _SAM_RESOURCE_H
#define	_SAM_RESOURCE_H

#ifdef sun
#pragma ident "$Revision: 1.29 $"
#endif

#include "sam/types.h"
#include "sam/param.h"

#define	SAM_RESOURCE_SIZE(n) (sizeof (sam_resource_file_t) + ((n) - 1) * \
	sizeof (struct sam_section))

#define	SAM_RM_SIZE(n) ((n + SAM_BLK - 1) >> SAM_SHIFT) << SAM_SHIFT

#define	SAM_RESOURCE_REVISION	1	/* Release 3.1.1 and above revision */
#define	SAM_ARCHIVE_GROUP	"sam_archive"
#define	SAM_ARCHIVE_OWNER	"sam_archive"

/* Begin: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack(4)
#endif

/* ----- Removable media multi-volume information record. */

typedef struct sam_vsn_section { /* For each archive copy volume section */
	char		vsn[32]; /* VSN */
	u_longlong_t	length;	/* Section length of file on this volume */
	u_longlong_t	position; /* Position of archive file for section */
	u_longlong_t	offset;	/* Location of copy section in archive file */
} sam_vsn_section_t;

/* ----- Removable media information record. */

typedef struct sam_arch_rminfo {
	offset_t	size;		/* Size of file */
	media_t		media;		/* Media type */
#ifndef BYTE_SWAP
	ushort_t
#if defined(_BIT_FIELDS_HTOL)
		valid		: 1,	/* Data is valid if 1 */
		stranger	: 1,	/* Stranger tape removable media file */
		bof_written	: 1,	/* BOF has been written */
		file_written	: 1,	/* File has been written */

		process_wtm	: 1,	/* Only write a tape mark */
		block_io	: 1,	/* Optical/Tape access */
					/* via direct I/O */
		partial_pdu	: 1,	/* User wrote a partial pdu */
		process_eox	: 1,	/* Write an end of section label */

		unused1		: 1,
		unused2		: 1,
		unused3		: 1,
		volumes		: 1,	/* More than 1 volume specified */

		buffered_io	: 1,	/* Optical/Tape access via */
					/* buffered I/O */
		unused4		: 1,
		unused5		: 1,
		filemark	: 1;	/* Filemark encountered */
#else /* defined(_BIT_FIELDS_HTOL) */
		filemark	: 1,	/* Filemark encountered */
		unused5		: 1,
		unused4		: 1,
		buffered_io	: 1,	/* Optical/Tape access via */
					/* buffered I/O */

		volumes		: 1,	/* More than 1 volume specified */
		unused3		: 1,
		unused2		: 1,
		unused1		: 1,

		process_eox	: 1,	/* Write an end of section label */
		partial_pdu	: 1,	/* User wrote a partial pdu */
		block_io	: 1,	/* Optical/Tape access via */
					/* direct I/O */
		process_wtm	: 1,	/* Only write a tape mark */

		file_written	: 1,	/* File has been written */
		bof_written	: 1,	/* BOF has been written */
		stranger	: 1,	/* Stranger tape removable */
					/* media file */
		valid		: 1;	/* Data is valid if 1 */
#endif /* defined(_BIT_FIELDS_HTOL) */
#else	/* BYTE_SWAP */
	ushort_t	flags;
#endif	/* BYTE_SWAP */
	uint_t		file_offset;	/* File starting byte offset */
	uint_t		mau;		/* Media allocation unit */
	uint_t		position;	/* Position of file */
} sam_arch_rminfo_t;


/* ----- Removable media archive record. */

typedef struct sam_archive {
	sam_arch_rminfo_t rm_info;	/* Removable media ino info */
	sam_time_t	creation_time;	/* Creation time */
	short		n_vsns;		/* Number of VSNs for entry */
	vsn_t		vsn;		/* Volume serial number */
	char		pad1[2];
#ifndef BYTE_SWAP
	union {				/* Media class */
		struct {		/* Optical archive entry: */
			int32_t label_pda;	/* File label physical */
						/* disk addr */
			int32_t version;	/* Version number */
			char	file_id[32];	/* Recorded file name */
		} od;
	} mc;
#else	/* BYTE_SWAP */
	/*
	 * fields below are 'unioned' in the actual
	 * structure.
	 */
	int32_t		label_pda;	/*  File label physical disk addr */
	int32_t		version;	/*  Version number */
	char		file_id[32];	/*  Recorded file name */
#endif	/* BYTE_SWAP */
} sam_archive_t;

/* End: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack()
#endif


/* ----- Removable media resource record. */

typedef struct sam_resource {
	short		revision;	/* Revision control number */
	char		access;		/* Open access control */
	char		protect;	/* Write protect flag */
	char		pad1[4];
	sam_archive_t	archive;	/* Archive record */
	vsn_t		next_vsn;	/* Next VSN */
	vsn_t		prev_vsn;	/* Previous VSN */
	int32_t		required_size;	/* Required size in 1024b blocks */
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
	/*
	 * fields below are 'unioned' in the actual
	 * structure.  They don't need byte-swapping.
	 */
	char		group_id[32];	/* Group identifier */
	char		owner_id[32];	/* Owner identifier, blank=def */
	char		info[160];	/* User information */
#endif	/* BYTE_SWAP */
} sam_resource_t;


/* ----- Removable media resource file structure. */

typedef struct sam_resource_file {
	sam_resource_t	resource;
	int		cur_ord;	/* Current VSN ordinal */
	int		n_vsns;		/* Number of VSNs in VSN list */
	struct sam_vsn_section	section[1];
} sam_resource_file_t;


/* Begin: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack(4)
#endif


/* ----- Stage request record. */

/*
 * Maximum number of volume sections for each archive copy that are
 * returned in a single stage request.  More volume sections are
 * handled by returning multiple requests.
 */
#define	SAM_MAX_VSN_SECTIONS	8

typedef struct sam_stage_copy {
	media_t		media;		/* Media type */
	int		n_vsns;		/* Num of VSNs in use for copy */
	ushort_t	flags;		/* Copy flags */
	short		ext_ord;	/* Extended multivolume request ord */
	struct sam_vsn_section	section[SAM_MAX_VSN_SECTIONS];
} sam_stage_copy_t;

#define	STAGE_COPY_ARCHIVED	0x0001	/* Copy made */
#define	STAGE_COPY_STALE	0x0002	/* Copy is stale */
#define	STAGE_COPY_DAMAGED	0x0004	/* Copy is damaged */
#define	STAGE_COPY_HDR0		0x0008	/* Position/offset point to tar hdr */
#define	STAGE_COPY_DISKARCH	0x0010	/* Copy is disk archive */
#define	STAGE_COPY_VERIFY	0x0020	/* Copy needs verification */

/*
 * Persistent data returned by stager daemon in filesystem response
 * to a stage.  This is not the only data returned by the response.
 * This structure defines the data that is not used by the
 * daemon and only sent in the stage request so it can be returned
 * to the filesystem in the response.
 */
typedef struct sam_stage_filesys {
	offset_t		stage_off;
	uchar_t			wait;		/* Stage wait flag */
} sam_stage_filesys_t;

typedef struct sam_stage_request {
	sam_id_t		id;		/* File identification */
	equ_t			fseq;		/* Family set equipment num */
	ushort_t		flags;		/* Staging flags */
	int			copy;		/* Stage from this arch copy */
	u_longlong_t		len;		/* Length of staging request */
	u_longlong_t		offset;		/* Offset of staging request */
	uchar_t			cs_algo;	/* Checksum algorithm */
	pid_t			pid;		/* Pid of requestor */
	uid_t			user;		/* Uid of requestor */
	uid_t			owner;		/* Owner's user id */
	gid_t			group;		/* Owner's group id */
	sam_stage_filesys_t	filesys;	/* Persistent filesystem */
						/* data */
	sam_stage_copy_t	arcopy[MAX_ARCHIVE];	/* Archive copy */
							/* information */
} sam_stage_request_t;

#define	STAGE_CANCEL	0x0001		/* Cancel stage request */
#define	STAGE_COPY	0x0002		/* User requested copy to stage from */
#define	STAGE_EXTENDED	0x0004		/* Extended request for multivolume */
#define	STAGE_CSVALID	0x0008		/* Valid checksum */
#define	STAGE_CSUSE	0x0010		/* Use checksum value */
#define	STAGE_NEVER	0x0020		/* Stage never request */
#define	STAGE_PARTIAL	0x0040		/* Partial stage request */

/* End: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack()
#endif

#endif  /* _SAM_RESOURCE_H */
