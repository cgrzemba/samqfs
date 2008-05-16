/*
 * fileprops.h - File properties definitions.
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

#if !defined(FILEPROPS_H)
#define	FILEPROPS_H

#pragma ident "$Revision: 1.34 $"

/* Macros. */
#define	FP_MAGIC 0062010		/* File properties magic number */
#define	FP_VERSION 51122		/* File properties version (YMMDD) */

#define	MAX_AR (2 * MAX_ARCHIVE)	/* New and rearchive ArchReq-s */

struct FileProps {
	MappedFile_t Fp;
	int	FpVersion;	/* File version */
	uname_t	FpFsname;	/* File system name */
	int	FpCount;	/* Number of file properties descriptors */

	struct FilePropsEntry {
		uint32_t FpFlags;
		int	FpBaseAsn;	/* Archive Set for file properties */
		int	FpAsn[MAX_AR];	/* Archive Set number for each copy */
		uint_t	FpArchAge[MAX_ARCHIVE];   /* Age to archive file */
		uint_t	FpUnarchAge[MAX_ARCHIVE]; /* Age to unarchive file */

		/* Search criteria. */
		upath_t	FpPath;		/* Path */
		char	FpName[256];	/* Name of file to match */
		int	FpPathSize;	/* Size of path */
		int	FpAccess;	/* Access time */
		time_t	FpAfter;	/* Created or modified after time */
		gid_t	FpGid;		/* Group id */
		uid_t	FpUid;		/* User id */
		fsize_t	FpMaxsize;	/* Maximum size */
		fsize_t	FpMinsize;	/* Minimum size */

		/* Attribute setting. */
		uint32_t FpMask;	/* Mask to use */
		int	FpPartial;	/* Partial size in kilobytes */
		uint32_t FpStatus;	/* Attributes to set */

		/* Archive copy controls. */
		/* The following three elements are in arch_status format */
		/* One bit for each copy */
		uchar_t	FpCopiesNorel;	/* Copies before auto release allowed */
		uchar_t FpCopiesRel;	/* Release file after copy is made */
		uchar_t	FpCopiesReq;	/* Copies required to be made */
		uchar_t	FpCopiesUnarch;	/* Copies to be unarchived */

		char	*FpRegexp;	/* Compiled regular expression for */
					/* FpName */
	} FpEntry[1];
};

/* Definitions for FpFlags. */
#define	FP_default	0x00000001	/* Default data archiving */
#define	FP_metadata	0x00000002	/* Filesystem metadata */
#define	FP_noarch	0x00000004	/* Don't archive these files */
#define	FP_nftv		0x00100000	/* No file time validation */

#define	FP_props	0x000fff00
#define	FP_access	0x00000100	/* -access */
#define	FP_after	0x00000200	/* -after */
#define	FP_gid		0x00000400	/* -group */
#define	FP_maxsize	0x00000800	/* -maxsize */
#define	FP_minsize	0x00001000	/* -minsize */
#define	FP_name		0x00002000	/* -name */
#define	FP_uid		0x00004000	/* -uid */

/* Used by arfind. */
#define	FP_add		0x80000000	/* FileProps added in reconfiguration */
#define	FP_change	0x40000000	/* FileProps changed in */
					/* reconfiguration */

#define	FpArfind FpRegexp		/* Start of arfind usage */
#define	FpLineno FpRegexp		/* Line number used by */
					/* archiver/readcmd.c */

#if	defined(NEED_FILEPROPS_NAMES)
#include "sam/setfield.h"
#endif /* defined(NEED_FILEPROPS_NAMES) */

#endif /* FILEPROPS_H */
