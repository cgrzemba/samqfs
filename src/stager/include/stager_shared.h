/*
 * shared.h - shared defInitions
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

#if !defined(STAGER_SHARED_H)
#define	STAGER_SHARED_H

#pragma ident "$Revision: 1.21 $"

/*
 * Stager's shared data definition.  This structure defines common data
 * that is used among all processes.  There is one copy of this structure
 * and its located in a memory mapped file.
 */
#define	SHARED_INFO_MAGIC	012322267146
#define	SHARED_INFO_VERSION	60814	/* YMMDD */

typedef struct SharedInfo {
	uint32_t si_magic;		/* magic number */
	uint32_t si_version;		/* version number */
	long si_hostId;			/* hostid where sam-stagerd started */
	pid_t si_parentPid;		/* pid of stager's parent daemon */
	/* Paths to memory mapped work files. */
	upath_t	si_stageReqsFile;	/* stage requests file */
	upath_t	si_stageReqExtents;	/* stage request extents file */
	upath_t	si_stageDoneFile;	/* stage completitions file */
	upath_t	si_copyInstancesFile;	/* copy instances data file */
	upath_t	si_streamsDir;		/* stream data directory */
	int	si_max_active;		/* max number of active stages */
	int	si_numCopyInstanceInfos;	/* number of copy processes */
	int	si_numFilesys;		/* number of filesystems */
	int	si_logEvents;		/* stage log events */
	upath_t	si_fileSystemFile;	/* file name for filesystem data */
	upath_t	si_diskVolumesFile;	/* file name for disk volumes */
	upath_t	si_coresDir;		/* directory for core files */
} SharedInfo_t;

extern SharedInfo_t *SharedInfo;

#endif /* STAGER_SHARED_H */
