/*
 * stage_reqs.c - support for mapping in stage requests from daemon
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

#pragma ident "$Revision: 1.18 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/param.h>

/* Solaris headers. */
#include <sys/mman.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/shm.h"
#include "pub/stat.h"
#include "sam/syscall.h"
#include "sam/uioctl.h"
#include "sam/custmsg.h"
#include "sam/exit.h"

/* Local headers. */
#include "stager_lib.h"
#include "stager_config.h"
#include "stager_threads.h"
#include "stage_reqs.h"
#include "stage_done.h"
#include "copy_defs.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/*
 * This structure contains all stage requests in progress.
 * The request list is implemented as an index file that is mapped to
 * the process's address space.  The request list is created in the
 *  stager daemon, written to a file and memory mapped into our
 *  address space.
 */
static struct {
	FileInfo_t	*data;
} stageReqs = { NULL };

static StageDone_t *stageDone = NULL;

extern CopyInstance_t *Context;

/*
 * Map in stage request file.
 */
void
MapInRequestList()
{
	stageReqs.data =
	    (FileInfo_t *)MapInFile(SharedInfo->stageReqsFile, O_RDWR, NULL);
	if (stageReqs.data == NULL) {
		FatalSyscallError(EXIT_NORESTART, HERE, "MapInFile",
		    SharedInfo->stageReqsFile);
	}

	stageDone = (StageDone_t *)MapInFile(SharedInfo->stageDoneFile, O_RDWR,
	    NULL);
	if (stageDone == NULL) {
		FatalSyscallError(EXIT_NORESTART, HERE, "MapInFile",
		    SharedInfo->stageDoneFile);
	}
}

/*
 * Get pointer to stage file information for a specific
 * request identifier.
 */
FileInfo_t *
GetFile(
	int id)
{
	FileInfo_t *file = NULL;
	if (id >= 0) {
		file = &stageReqs.data[id];
	}
	return (file);
}

/*
 * Mark file staging as done and add to stage done list
 * so request list space can be reclaimed.
 */
void
SetStageDone(
	FileInfo_t *file)
{
	int id;
	FileInfo_t *last;

	THREAD_LOCK(file);
	SET_FLAG(file->flags, FI_DONE);

	Trace(TR_DEBUG, "SetStageDone: ino: %d.%d fseq: %d",
	    file->id.ino, file->id.gen, file->fseq);

	if (getppid() == 1) {
		/*
		 * Parent has exited.
		 * No need to add to the stageDone list.
		 */
		SET_FLAG(file->flags, FI_ORPHAN);
		if (GET_FLAG(file->flags, FI_MULTIVOL)) {
			CLEAR_FLAG(file->flags, FI_DCACHE);
			SET_FLAG(file->flags, FI_CANCEL);
		}
		file->context = Context->pid;
		THREAD_UNLOCK(file);
		return;
	}
	THREAD_UNLOCK(file);

	id = file->sort;
	PthreadMutexLock(&stageDone->mutex);
	if (stageDone->first == -1) {
		stageDone->first = stageDone->last = id;
	} else {
		last = GetFile(stageDone->last);
		last->next = id;
		stageDone->last = id;
	}
	file->next = -1;
	PthreadCondSignal(&stageDone->cond);
	PthreadMutexUnlock(&stageDone->mutex);
}
