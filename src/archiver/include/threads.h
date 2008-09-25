/*
 * threads.h - Threads control definitions.
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

#ifndef THREADS_H
#define	THREADS_H

#pragma ident "$Revision: 1.21 $"

/* Function macros. */

/* pthread error checking. */

#define	PthreadCondInit(a, b) \
{ errno = pthread_cond_init(a, b); if (errno != 0) \
	LibFatal(pthread_cond_init, #a); }

#define	PthreadCondSignal(a) \
{ errno = pthread_cond_signal(a); if (errno != 0) \
	LibFatal(pthread_cond_signal, #a); }

#define	PthreadCondWait(a, b) \
{ errno = pthread_cond_wait(a, b); if (errno != 0) \
	LibFatal(pthread_cond_wait, #a); }

#define	PthreadMutexInit(a, b) \
{ errno = pthread_mutex_init(a, b); if (errno != 0) \
	LibFatal(pthread_mutex_init, #a); }

#define	PthreadMutexLock(a) \
{ errno = pthread_mutex_lock(a); if (errno != 0) \
	LibFatal(pthread_mutex_lock, #a); }

#define	PthreadMutexUnlock(a) \
{ errno = pthread_mutex_unlock(a); if (errno != 0) \
	LibFatal(pthread_mutex_unlock, #a); }

#if 0 /* To trace mutex lock/unlock, move these after definition lines above */
Trace(TR_ARDEBUG, "%d Lock %s", (int)thr_self(), #a); \
Trace(TR_ARDEBUG, "%d Unlock %s", (int)thr_self(), #a); \
Leave this line in...  It terminates the line above.
#endif

/*
 * Reconfiguration synchronization control codes.
 * Threads call ThreadsReconfigSync() to prevent reconfiguration when they
 * are working.
 * When not working, threads call ThreadsReconfigSync() to allow
 * reconfiguration.
 * When reconfiguration can take place, the 'reconfig' function set by
 * ThreadsStart() is called to perform reconfiguration.
 */
typedef enum { RC_none,
	RC_allow,		/* Allow reconfiguration */
	RC_intReconfig,		/* Internal reconfigure */
	RC_reconfig,		/* Reconfigure */
	RC_wait,		/* Wait to reconfigure */
	RC_max
} ReconfigControl_t;

/*
 * Threads control.
 * Thread table entry for threads to start and stop.
 * Last entry has TtName = NULL;
 */
struct Threads {
	char	*TtName;		/* Name of thread */
	void	*(*TtFunc)(void *);	/* Thread function to start */
	void	(*TtStop)(void);	/* Function to stop thread */
	void	(*TtWakeup)(void);	/* Function to wakeup thread */
	pthread_t TtThread;
};

/* Functions. */
void ThreadsExit(void);
void ThreadsInitWait(void(*stop)(void), void(*wakeup)(void));
void ThreadsReconfigSync(ReconfigControl_t ctrl);
void ThreadsSleep(int seconds);
void ThreadsStart(struct Threads *table,
		void(*reconfig)(ReconfigControl_t ctrl));
void ThreadsStop(void);
void ThreadsCondTimedWait(pthread_cond_t *cond, pthread_mutex_t *mutex,
		time_t waitTime);
void ThreadsDiskVols(boolean_t lockon);
void ThreadsWakeup(void);

#endif /* THREADS_H */
