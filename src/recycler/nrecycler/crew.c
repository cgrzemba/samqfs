/*
 * crew.c - work crew. Each crew performs an operation on its own data.
 * Threads in a work crew may all perform the same operation, or each
 * a separate operation, but they always proceed independently.
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
#pragma ident "$Revision: 1.5 $"

static char *_SrcFile = __FILE__;

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/acl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/bitmap.h>

#include "recycler_c.h"
#include "recycler_threads.h"

#include <fcntl.h>
#include <dirent.h>

#include "sam/types.h"
#include "aml/shm.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "aml/device.h"
#include "aml/catlib.h"
#include "sam/names.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "sam/fs/ino.h"
#include "sam/fs/ino_ext.h"
#include "aml/diskvols.h"
#include "aml/sam_rft.h"
#include "aml/id_to_path.h"
#include "sam/custmsg.h"
#include "sam/exit.h"
/* #include "../../../src/fs/include/bswap.h" */
#include "../../../src/fs/cmd/dump-restore/csd.h"

#include "recycler.h"

extern boolean_t InfiniteLoop;

static void *crewWorker(void *arg);

/*
 * Create a work crew.
 */
int
CrewCreate(
	Crew_t *crew,
	int crew_size)
{
	int i;
	int ret;

	(void) memset(crew, 0, sizeof (Crew_t));
	crew->cr_size = (crew_size > CREW_MAXSIZE) ? CREW_MAXSIZE : crew_size;
	crew->cr_count = 0;
	crew->cr_first = NULL;
	crew->cr_last = NULL;

	/*
	 * Initialize synchronization objects
	 */
	PthreadMutexInit(&crew->cr_mutex, NULL);
	PthreadCondInit(&crew->cr_done, NULL);
	PthreadCondInit(&crew->cr_go, NULL);

	/*
	 * Create the worker threads.
	 */
	for (i = 0; i < crew->cr_size; i++) {
		crew->cr_workers[i].wo_crew = crew;
		ret = pthread_create(&crew->cr_workers[i].wo_thread,
		    NULL, crewWorker, (void*)&crew->cr_workers[i]);
		if (ret != 0) {
			Trace(TR_MISC,
			    "Error: pthread_create failed, errno = %d", errno);
			return (-1);
		}
	}

	return (0);
}

void
CrewCleanup(
	/* LINTED argument unused in function */
	Crew_t *crew)
{
}

static void *
crewWorker(
	void *arg)
{
	Worker_t *mine;
	Crew_t *crew;
	WorkItem_t *work;
	int rv;

	mine = (Worker_t *)arg;
	crew = mine->wo_crew;

	/*
	 * Wait for something to be put on the work queue.
	 */
	PthreadMutexLock(&crew->cr_mutex);
	while (crew->cr_count == 0) {
		rv = pthread_cond_wait(&crew->cr_go, &crew->cr_mutex);
		if (rv != 0) {
			Trace(TR_MISC,
			    "Error: pthread_cond_wait failed %d", errno);
			abort();
		}
	}
	PthreadMutexUnlock(&crew->cr_mutex);

	while (InfiniteLoop) {
		/*
		 * Wait for work.  If crew->cr_first is NULL, there's no work.
		 * If crew->cr_count goes to zero, we're done.
		 */
		PthreadMutexLock(&crew->cr_mutex);

		while (crew->cr_first == NULL) {
			rv = pthread_cond_wait(&crew->cr_go, &crew->cr_mutex);
			if (rv != 0) {
				Trace(TR_MISC,
				    "Error: pthread_cond_wait failed %d",
				    errno);
				abort();
			}
		}

#if 0
		Trace(TR_SAMDEV, "Crew woke: 0x%x", (int)crew->cr_first);
#endif

		/*
		 * Remove and process a work item.
		 */
		work = crew->cr_first;
		crew->cr_first = work->wi_next;
		if (crew->cr_first == NULL) {
			crew->cr_last = NULL;
		}

#if 0
		Trace(TR_SAMDEV,
		    "Crew thread took: 0x%x first: 0x%x last: 0x%x",
		    (int)work, (int)crew->cr_first, (int)crew->cr_last);
#endif

		PthreadMutexUnlock(&crew->cr_mutex);

		work->wi_func(work->wi_arg);

		/*
		 * Decrement count of outstanding work items.  Wake waiters
		 * if the crew is now idle.
		 */
		PthreadMutexLock(&crew->cr_mutex);
		crew->cr_count--;

#if 0
		Trace(TR_SAMDEV, "Crew thread work 0x%x completed", (int)work);
#endif

		if (crew->cr_count <= 0) {
			rv = pthread_cond_broadcast(&crew->cr_done);
			if (rv != 0) {
				Trace(TR_MISC,
				    "Error: pthread_cond_broadcast failed %d",
				    errno);
				abort();
			}
		}
		PthreadMutexUnlock(&crew->cr_mutex);
	}
	return (NULL);
}
