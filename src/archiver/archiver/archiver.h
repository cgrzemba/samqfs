/*
 * archiver.h - Archiver program definitions.
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

#ifndef ARCHIVER_H
#define	ARCHIVER_H

#pragma ident "$Revision: 1.41 $"

/* Local headers. */
#include "common.h"
#include "utility.h"

/* Macros. */

#define	ALLSETS_MAX ((2 * MAX_ARCHIVE) + 1)	/* allsets all + two per copy */

/* List options */
#define	LO_none	0x00
#define	LO_arch	0x01		/* List detailed file archive information */
#define	LO_conf	0x02		/* List configuration information */
#define	LO_fs	0x04		/* List file system statistics */
#define	LO_line	0x08		/* List input lines */
#define	LO_vsn	0x10		/* List VSNs */
#define	LO_all	0x1f

/* Public data. */

/* File system table */
DCL struct FileSys {
	int	count;
	struct	FileSysEntry {
		uname_t	FsName;		/* Name of filesystem */
		ushort_t FsFlags;	/* File system flags */
		int	FsBackGndInterval;	/* Background scan interval */
		int	FsBackGndTime;	/* Background scan time of day */
		int	FsDirCacheSize;	/* Directory cache size */
		int	FsInterval;	/* Scan interval */
		upath_t	FsLogFile;	/* Archive log file */
		int	FsMaxpartial;	/* Max Partial size in kilobytes */
		ExamMethod_t FsExamine;	/* File system examination method */
	} entry[1];
} *FileSysTable IVAL(NULL);

#define	FS_archivemeta	0x0001	/* Archive meta data */
#define	FS_noarchive	0x0002	/* No archiving for this file system */
#define	FS_noarfind	0x0004	/* Don't run sam-arfind (noarscan) */
#define	FS_scanlist	0x0008	/* Perform scanlist consolidation */
#define	FS_setarchdone	0x0010	/* Set archdone state while scanning files */
#define	FS_share_client	0x0020	/* shared client */
#define	FS_share_reader	0x0040	/* shared reader */
#define	FS_wait		0x0080	/* Filesystem set to wait */

DCL int ListOptions IVAL(LO_none);

DCL struct FileProps **FilesysProps IVAL(NULL);
DCL boolean_t FirstRead IVAL(TRUE);
DCL boolean_t Wait IVAL(FALSE);

/* For scanning the filesystem and using arfind modules. */
DCL struct ArfindState *State IVAL(NULL);	/* arfind state */
DCL struct FileProps *FileProps;

/* Functions. */
void PrintArchiveStatus(void);
void PrintInfo(char *fsname);
void PrintFsStats(void);
void ReadCmds(char *fname);

#endif /* ARCHIVER_H */
