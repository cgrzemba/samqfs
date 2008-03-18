/*
 * config.h
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

#if !defined(CONFIG_H)
#define	CONFIG_H

#pragma ident "$Revision: 1.19 $"

#include <limits.h>

#define	STAGERD_PROGRAM_NAME	"sam-stagerd"
#define	COPY_PROGRAM_NAME	"sam-stagerd_copy"
#define	COMMAND_FILE_NAME	"stager.cmd"

#define	RM_DIR ".stage"		/* removable media directory name */
#define	DIR_MODE 0777		/* create mode for stager's directories */
#define	FILE_MODE 0664		/* create mode for stager's rm files */

/*
 * Optional trace message elements.
 */
#define	OPTIONAL_TRACE_MASK	(1 << TR_date) | (1 << TR_module)

/*
 * Stage file request list file name.  The request list
 * is memory mapped to the following file name.
 */
#define	SHARED_FILENAME		"sharedinfo"
#define	STAGE_REQS_FILENAME	"stagereqs"
#define	STAGE_REQ_EXTENTNAME	"stagereq_extents"
#define	STAGE_DONE_FILENAME	"stagedone"
#define	COPY_PROCS_FILENAME	"copyprocs"
#define	COPY_FILE_FILENAME	"copyfile"
#define	STREAMS_DIRNAME		"streams"
#define	FILESYSTEM_FILENAME	"filesystem"
#define	DISK_VOLUMES_FILENAME	"diskvolumes"

/*
 * Stage file request list allocation size.  If more
 * space is required in the request list, this is the number
 * of entries added to the list.
 */
#define	STAGE_REQUESTS_CHUNKSIZE 100

#define	STAGE_EXTENTS_CHUNKSIZE 10

/*
 * Composition list allocation size.  If more space is
 * required in the composition list, this is the number of
 * entries added to the list.
 */
#define	COMPOSE_LIST_CHUNKSIZE 100

/*
 * Scheduler thread timeout value in seconds.  Scheduler will
 * wait on the condition variable (incoming stage requests) for this
 * timeout value.  If timeout is reached, the scheduler thread will
 * wakeup and look to schedule existing requests on newly freed drives.
 */
#define	SCHEDULER_TIMEOUT_SECS	2

/*
 * Check active stage file requests timeout value in seconds.
 */
#define	REQUEST_TIMEOUT_SECS	2

/*
 * Copy process timeout value in seconds.  Copy proc will wait
 * on the condition variable (copy requests) for this timeout value.
 * If timeout is reached, the copy process will exit.
 */
#define	COPYPROC_TIMEOUT_SECS	60*5

/*
 * Number of times max active stages is exceeded before logging
 * a message.  Counter is reset after the message is logged.
 */
#define	MAX_ACTIVE_EXCEEDED_MSG	100

/*
 * Will check for configuration changes whenever this timeout
 * value is reached.
 */
#define	CONFIG_CHECK_TIMEOUT_SECS	10

/*
 * Max number of copy errors in a stream before rejecting it.
 */
#define	MAX_COPY_STREAM_ERRORS	10

#endif /* CONFIG_H */
