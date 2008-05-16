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

#pragma ident "$Revision: 1.20 $"

#include "sam/types.h"

/*
 * Stager's shared data definition.  This structure defines common data
 * that is used among all processes.  There is one copy of this structure
 * and its located in a memory mapped file.
 */
#define	STAGER_SHARED_MAGIC	012322267146
#define	STAGER_SHARED_VERSION	60814	/* YMMDD */

typedef struct SharedInfo {
	uint32_t magic;			/* magic number */
	uint32_t version;		/* version number */
	long hostId;			/* hostid where sam-stagerd started */
	pid_t parentPid;		/* pid of stager's parent daemon */
	upath_t	stageReqsFile;		/* file name for stage requests */
	upath_t	stageReqExtents;	/* filename for stage request extents */
	upath_t	stageDoneFile;		/* file name for stage completitions */
	upath_t	copyprocsFile;		/* file name for copy procs data */
	upath_t	streamsDir;		/* directory for stream data */
	int	max_active;		/* max number of active stages */
	int	num_copyprocs;		/* number of copy processes/threads */
	int	num_filesystems;	/* number of filesystems */
	int	logEvents;		/* stage log events */
	upath_t	fileSystemFile;		/* file name for filesystem data */
	upath_t	diskVolumesFile;	/* file name for disk volumes */
	upath_t	coresDir;		/* directory for core files */
} SharedInfo_t;

extern SharedInfo_t *SharedInfo;

#endif /* STAGER_SHARED_H */
