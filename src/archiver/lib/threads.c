/*
 * threads.c - Thread control functions.
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

#pragma ident "$Revision: 1.24 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/sam_trace.h"

/* Local headers. */
#include "common.h"
#include "threads.h"

#if defined(lint)
#undef sigemptyset
#undef sigfillset
#undef sigaddset
#undef sigdelset
#endif /* defined(lint) */

/* Private Data. */
static struct Threads *threadsTable;
static void(*reconfigFunc)(ReconfigControl_t ctrl);
static pthread_mutex_t reconfigMutex = PTHREAD_MUTEX_INITIALIZER;
static uint64_t threadUse = 1;	/* Initialized to prevent reconfiguration */
static int startCount;
static pthread_mutex_t diskvolsMutex = PTHREAD_MUTEX_INITIALIZER;

#define	MAX_THREADS ((sizeof (threadUse) * 8) - 1)

/* Private functions. */
static void catchSIGUSR1(int sig);

/*
 * Perform condition timed wait.
 */
void
ThreadsCondTimedWait(
	pthread_cond_t *cond,
	pthread_mutex_t *mutex,
	time_t waitTime)
{
	struct timespec abstime;
	int	seconds;

	abstime.tv_sec = time(NULL);
	seconds = waitTime - abstime.tv_sec;
	if (seconds > SCAN_INTERVAL_MAX) {
		seconds = SCAN_INTERVAL_MAX;
	}
	abstime.tv_sec += seconds;
	abstime.tv_nsec = 0;
	errno = pthread_cond_timedwait(cond, mutex, &abstime);
	if (errno != 0 && errno != ETIMEDOUT) {
		LibFatal(pthread_cond_wait, "ThreadsCondTimedWait");
	}
	if (errno == ETIMEDOUT && seconds == SCAN_INTERVAL_MAX) {
		errno = 0;
	}
}


/*
 * Clear active thread.
 */
void
ThreadsExit(void)
{
	struct Threads *th;

	/*
	 * Find thread exiting.
	 */
	for (th = threadsTable; th->TtName != NULL; th++) {
		if (th->TtThread == pthread_self()) {
			Trace(TR_ARDEBUG, "Exited %s[%d]", th->TtName,
			    th->TtThread);
			th->TtThread = 0;
			pthread_exit(NULL);
		}
	}
	Trace(TR_ARDEBUG, "ThreadsExit %d not found", pthread_self());
}


/*
 * Wait for threads to complete initialization.
 * All started threads must call this when initialization is complete.
 */
void
ThreadsInitWait(
	void(*stop)(void),
	void(*wakeup)(void))
{
	static pthread_mutex_t initMutex = PTHREAD_MUTEX_INITIALIZER;
	static pthread_cond_t initCond = PTHREAD_COND_INITIALIZER;
	static int initCount = 0;
	struct Threads *th;

	PthreadMutexLock(&initMutex);

	/*
	 * Find thread and enter stop and wakeup functions.
	 */
	for (th = threadsTable; th->TtName != NULL; th++) {
		if (th->TtThread == pthread_self()) {
			th->TtStop = stop;
			th->TtWakeup = wakeup;
			break;
		}
	}
	Trace(TR_ARDEBUG, "ThreadsWaitInit[%d] %s", pthread_self(), th->TtName);
	initCount++;
	if (initCount >= startCount) {
		(void) pthread_cond_broadcast(&initCond);
	} else while (initCount < startCount) {
		errno = pthread_cond_wait(&initCond, &initMutex);
		if (errno == EINTR) {
			Trace(TR_ARDEBUG, "ThreadsWaitInit[%d] interrupted",
			    pthread_self());
		} else if (errno != 0 && errno != ETIMEDOUT) {
			LibFatal(pthread_cond_wait, "ThreadsWaitInit");
		}
	}
	PthreadMutexUnlock(&initMutex);
	Trace(TR_ARDEBUG, "ThreadsWaitInit go [%d] %s",
	    pthread_self(), th->TtName);
}


/*
 * Synchronize reconfiguration.
 * Each thread is represented as a bit in 'threadUse'.  The bit is set when
 * the thread wants to prevent reconfiguration while working (RC_wait).
 * The bit is clear when the thread can allow a reconfiguration (RC_allow).
 * When reconfiguration is required (RC_reconfig or RC_intReconfig), the
 * reconfiguration can take place if all bits are clear.
 * The thread performing the 'RC_reconf' or 'RC_intReconfig' does the
 * reconfiguration if no bits are set.  Otherwise, the last thread
 * to clear its bit does the reconfiguration.
 */
void
ThreadsReconfigSync(
	ReconfigControl_t ctrl)
{
	static boolean_t reconfig = FALSE;
	ReconfigControl_t saveCtrl;

	PthreadMutexLock(&reconfigMutex);
	switch (ctrl) {

	case RC_intReconfig:
	case RC_reconfig:
		saveCtrl = ctrl;
		reconfig = TRUE;
		/*FALLTHROUGH*/

	case RC_allow:
		threadUse &= ~(1 << pthread_self());
		if (threadUse == 0 && reconfig) {
			reconfig = FALSE;
			Trace(TR_MISC, "Reconfiguration");
			reconfigFunc(saveCtrl);
		}
		break;

	case RC_wait:
		threadUse |= (1 << pthread_self());
		break;

	default:
		Trace(TR_ERR, "Strange reconfigControl %d", ctrl);
		break;
	}
	PthreadMutexUnlock(&reconfigMutex);
}


/*
 * Put thread to sleep.
 */
void
ThreadsSleep(
	int seconds)
{
	pthread_cond_t wait = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t waitMutex = PTHREAD_MUTEX_INITIALIZER;
	struct timespec abstime;

	PthreadMutexLock(&waitMutex);
	abstime.tv_sec = time(NULL) + seconds;
	abstime.tv_nsec = 0;
	errno = pthread_cond_timedwait(&wait, &waitMutex, &abstime);
	if (errno == EINTR) {
		Trace(TR_ARDEBUG, "ThreadsSleep[%d] interrupted",
		    pthread_self());
	} else if (errno != 0 && errno != ETIMEDOUT) {
		LibFatal(pthread_cond_wait, "ThreadsSleep");
	}
	PthreadMutexUnlock(&waitMutex);
}


/*
 * Start threads.
 */
void
ThreadsStart(
	struct Threads *table,
	void(*reconfig)(ReconfigControl_t ctrl))
{
	struct Threads *th;
	struct sigaction act;
	sigset_t oset;

	threadsTable = table;
	reconfigFunc = reconfig;

	/*
	 * Block all signals except SIGUSR1 for the threads that we will start.
	 * SIGUSR1 can be sent to the threads to interrupt them out of system
	 * calls.
	 */
	memset(&act, 0, sizeof (act));
	act.sa_handler = catchSIGUSR1;
	if (sigemptyset(&act.sa_mask) == -1) {
		LibFatal(sigemptyset, "");
	}
	if (sigaction(SIGUSR1, &act, NULL) == -1) {
		LibFatal(sigaction, "SIGUSR1");
	}

	if (sigfillset(&act.sa_mask) == -1) {
		LibFatal(sigfillset, "");
	}
	if (sigdelset(&act.sa_mask, SIGUSR1) == -1) {
		LibFatal(sigdelset, "");
	}
	errno = pthread_sigmask(SIG_SETMASK, &act.sa_mask, &oset);
	if (errno != 0) {
		LibFatal(pthread_sigmask, "block all");
	}

	/*
	 * Count requested threads.
	 */
	startCount = 0;
	for (th = threadsTable; th->TtName != NULL; th++) {
		if (th->TtThread == 0) {
			startCount++;
		}
	}

	/*
	 * Start requested threads.
	 */
	for (th = threadsTable; th->TtName != NULL; th++) {
		if (th->TtThread != 0) {
			continue;
		}
		if (th->TtFunc == NULL) {
			/*
			 * The calling thread has no function defined.
			 * Enter calling thread in table.
			 */
			th->TtThread = pthread_self();
			continue;
		}
		errno = pthread_create(&th->TtThread, NULL, *th->TtFunc, th);
		if (errno != 0) {
			LibFatal(pthread_create, th->TtName);
		}
		if (th->TtThread > MAX_THREADS) {
			LibFatal(pthread_create, "thread > MAX_THREADS");
		}
		Trace(TR_ARDEBUG, "Started %s[%d]", th->TtName, th->TtThread);
	}

	/*
	 * Restore calling thread signals.
	 */
	errno = pthread_sigmask(SIG_SETMASK, &oset, NULL);
	if (errno != 0) {
		LibFatal(pthread_sigmask, "reset");
	}
	PthreadMutexLock(&reconfigMutex);
	threadUse &= ~1;
	PthreadMutexUnlock(&reconfigMutex);
}


/*
 * Stop threads.
 * Call the thread's stop function.
 * If no stop function, cancel the thread.
 */
void
ThreadsStop(void)
{
	struct Threads *th;

	/*
	 * Stop the threads.
	 */
	for (th = threadsTable; th->TtName != NULL; th++) {
		if (th->TtThread != 0 && th->TtFunc != NULL &&
		    th->TtThread != pthread_self()) {
			if (th->TtStop != NULL) {
				th->TtStop();
			} else {
				errno = pthread_kill(th->TtThread, SIGUSR1);
				if (errno != 0 && errno != ESRCH) {
					Trace(TR_ERR, "pthread_kill(%s) failed",
					    th->TtName);
				}
			}
		}
	}

	/*
	 * Wait for threads to complete their work.
	 */
	for (th = threadsTable; th->TtName != NULL; th++) {
		if (th->TtThread != 0 && th->TtFunc != NULL &&
		    th->TtThread != pthread_self()) {
			void	*statusp;

			errno = pthread_join(th->TtThread, &statusp);
			if (errno != 0) {
				Trace(TR_ERR,
				    "pthread_join(th->TtName) failed");
			}
			Trace(TR_ARDEBUG, "Stopped %s[%d]",
			    th->TtName, th->TtThread);
		}
	}
}


/*
 * Wakeup threads.
 * Call the thread's wakeup function.
 */
void
ThreadsWakeup(void)
{
	struct Threads *th;

	for (th = threadsTable; th->TtName != NULL; th++) {
		if (th->TtThread != 0 && th->TtThread != pthread_self()) {
			if (th->TtWakeup != NULL) {
				Trace(TR_ARDEBUG, "Wakeup %s[%d]",
				    th->TtName, th->TtThread);
				th->TtWakeup();
			}
		}
	}
}

/*
 * Thread locking for diskvols database access.
 */
void
ThreadsDiskVols(
boolean_t lockon)
{
	if (lockon == B_TRUE) {
		PthreadMutexLock(&diskvolsMutex);
	} else {
		PthreadMutexUnlock(&diskvolsMutex);
	}
}



/* Private functions. */

/*
 * Catch SIGUSR1.
 */
static void
catchSIGUSR1(
	int sig)
{
#if defined(AR_DEBUG) || defined(lint)
	TraceSignal(sig);
#endif /* defined(AR_DEBUG) || defined(lint) */
}
