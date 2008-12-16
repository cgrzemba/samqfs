/*
 * queue.c - queue functions.
 * Create and manage the archiver queues.
 * Queues are link lists of ArchReq-s ordered by the scheduling priority.
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

#pragma ident "$Revision: 1.19 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

/* POSIX headers. */
#include <pthread.h>

/* SAM-FS headers. */
#include "sam/types.h"

/* Local headers. */
#include "archiverd.h"

/* Private data. */
static pthread_mutex_t QueueMutex = PTHREAD_MUTEX_INITIALIZER;
static struct ArchReq arNull;
static struct QueueEntry *qeAll = NULL;
static struct Queue freeQ = { "free" };


/*
 * Add entry to a queue.
 * Entry is added in priority order.
 */
void
QueueAdd(
	struct QueueEntry *qe,
	struct Queue *qu)
{
	struct QueueEntry *qet;

	PthreadMutexLock(&QueueMutex);
	for (qet = qu->QuHead.QeFwd;
	    qet->QeAr->ArSchedPriority > qe->QeAr->ArSchedPriority ||
	    (qet->QeAr->ArSchedPriority == qe->QeAr->ArSchedPriority &&
	    qet->QeAr->ArTime < qe->QeAr->ArTime);
	    qet = qet->QeFwd) {
		;
	}
	qe->QeFwd = qet;
	qe->QeBak = qet->QeBak;
	qet->QeBak->QeFwd = qe;
	qet->QeBak = qe;
	PthreadMutexUnlock(&QueueMutex);
}


/*
 * Free a queue entry.
 * Add the queue entry to the 'free' queue.
 */
void
QueueFree(
	struct QueueEntry *qe)
{
	qe->QeAr = &arNull;
	strncpy(qe->QeArname, "null", sizeof (qe->QeArname) - 1);
	QueueAdd(qe, &freeQ);
}


/*
 * Get an unused queue entry.
 * If an entry is on the free queue, return it.
 * Otherwise, allocate a new entry.
 */
struct QueueEntry *
QueueGetFree(void)
{
	struct QueueEntry *qe;

	PthreadMutexLock(&QueueMutex);
	if (freeQ.QuHead.QeFwd != &freeQ.QuHead) {
		qe = freeQ.QuHead.QeFwd;
		qe->QeBak->QeFwd = qe->QeFwd;
		qe->QeFwd->QeBak = qe->QeBak;
	} else {
		SamMalloc(qe, sizeof (struct QueueEntry));
		qe->QeAr = &arNull;
		strncpy(qe->QeArname, "null", sizeof (qe->QeArname) - 1);
		qe->QeNext = qeAll;
		qeAll = qe;
	}
	PthreadMutexUnlock(&QueueMutex);
	return (qe);
}


/*
 * Search queues for ArchReq.
 */
boolean_t
QueueHasArchReq(
	char *arname)
{
	struct QueueEntry *qe;

	PthreadMutexLock(&QueueMutex);
	for (qe = qeAll; qe != NULL; qe = qe->QeNext) {
		if (strcmp(qe->QeArname, arname) == 0) {
			break;
		}
	}
	PthreadMutexUnlock(&QueueMutex);
	return (qe != NULL);
}


/*
 * Initialize a queue.
 * Point the queue head to itself using an 'empty' ArchReq.
 * Set the minimum priority and name the queue.
 */
void
QueueInit(
	struct Queue *qu)
{
	struct QueueEntry *qe;

	qe = &qu->QuHead;
	qe->QeFwd = qe;
	qe->QeBak = qe;
	qe->QeAr = &arNull;
	strncpy(qe->QeArname, "head", sizeof (qe->QeArname) - 1);
	if (arNull.ArSchedPriority == 0) {
		arNull.ArSchedPriority = -FLT_MAX;
		QueueInit(&freeQ);
	}
}


/*
 * Remove entry from a queue.
 */
void
QueueRemove(
	struct QueueEntry *qe)
{
	PthreadMutexLock(&QueueMutex);
	qe->QeBak->QeFwd = qe->QeFwd;
	qe->QeFwd->QeBak = qe->QeBak;
	PthreadMutexUnlock(&QueueMutex);
}


/*
 * Trace a queue.
 */
void
QueueTrace(
	const char *srcFile,
	const int srcLine,
	struct Queue *qu)    /* Queue to trace.  If NULL, trace free queue. */
{
	struct QueueEntry *qe;
	FILE	*st;
	char	*msg;
	int	i;

	PthreadMutexLock(&QueueMutex);
	st = NULL;
	if (qu == NULL) {
		qu = &freeQ;
		i = 0;
		for (qe = qu->QuHead.QeFwd; qe != &qu->QuHead; qe = qe->QeFwd) {
			i++;
		}
		_Trace(TR_misc, srcFile, srcLine, "free queue: %d entries", i);
#if defined(DEBUG)
		if ((st = TraceOpen()) != NULL) {
			i = 0;
			for (qe = qeAll; qe != NULL; qe = qe->QeNext) {
				fprintf(st, "%d %s %p\n", i++,
				    qe->QeArname, (void *)qe->QeAr);
			}
		}
#endif /* defined(DEBUG) */
		goto out;
	}

	qe = qu->QuHead.QeFwd;
	if (qe == &qu->QuHead) {
		msg = "empty";
	} else {
		msg = "";
	}
	_Trace(TR_misc, srcFile, srcLine, "%s queue: %s", qu->QuName, msg);
	if (qe != &qu->QuHead && (st = TraceOpen()) != NULL) {
		i = 0;
		while (qe != &qu->QuHead) {
			fprintf(st, "%3d %s %s ", i++,
			    qe->QeArname, qe->QeAr->ArFsname);
			ArchReqPrint(st, qe->QeAr,
			    *TraceFlags & (1 << TR_files));
			qe = qe->QeFwd;
		}
	}

out:
	PthreadMutexUnlock(&QueueMutex);
	if (st != NULL) {
		TraceClose(-1);
	}
}
