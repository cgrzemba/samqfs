/*
 * stager_threads.h - stager daemon thead control definitions.
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

#ifndef STAGER_THREADS_H
#define	STAGER_THREADS_H

#pragma ident "$Revision: 1.9 $"

/* Pthread functions macros with error checking. */
#define	PthreadMutexLock(a) \
{ int _rval_; _rval_ = pthread_mutex_lock(a); if (_rval_ != 0) \
	LibFatal(pthread_mutex_lock, #a); }

#define	PthreadMutexUnlock(a) \
{ int _rval_; _rval_ = pthread_mutex_unlock(a); if (_rval_ != 0) \
	LibFatal(pthread_mutex_unlock, #a); }

#define	PthreadCondWait(a, b) \
{ int _rval_; _rval_ = pthread_cond_wait(a, b); if (_rval_ != 0) \
	LibFatal(pthread_cond_wait, #a); }

/* Macro doesn't work well if caller needs to determine if timed out was hit */
#define	PthreadCondTimedwait(a, b, c) \
{ int _rval_; _rval_ = pthread_cond_timedwait(a, b, c); \
	if (_rval_ != 0 && _rval_ != ETIMEDOUT) \
	LibFatal(pthread_cond_wait, #a); }

#define	PthreadCondSignal(a) \
{ int _rval_; _rval_ = pthread_cond_signal(a); if (_rval_ != 0) \
	LibFatal(pthread_cond_signal, #a); }

#define	PthreadMutexattrInit(a) \
{ int _rval_; _rval_ = pthread_mutexattr_init(a); if (_rval_ != 0) \
	LibFatal(pthread_mutexattr_init, #a); }

#define	PthreadMutexattrDestroy(a) \
{ int _rval_; _rval_ = pthread_mutexattr_destroy(a); if (_rval_ != 0) \
	LibFatal(pthread_mutexattr_destroy, #a); }

#define	PthreadCondattrInit(a) \
{ int _rval_; _rval_ = pthread_condattr_init(a); if (_rval_ != 0) \
	LibFatal(pthread_condattr_init, #a); }

#define	PthreadCondattrDestroy(a) \
{ int _rval_; _rval_ = pthread_condattr_destroy(a); if (_rval_ != 0) \
	LibFatal(pthread_condattr_destroy, #a); }

#define	PthreadMutexattrSetpshared(a, b) \
{ int _rval_; _rval_ = pthread_mutexattr_setpshared(a, b); if (_rval_ != 0) \
	LibFatal(pthread_mutexattr_setpshared, #a); }

#define	PthreadCondattrSetpshared(a, b) \
{ int _rval_; _rval_ = pthread_condattr_setpshared(a, b); if (_rval_ != 0) \
	LibFatal(pthread_condattr_setpshared, #a); }

#define	PthreadMutexInit(a, b) \
{ int _rval_; _rval_ = pthread_mutex_init(a, b); if (_rval_ != 0) \
	LibFatal(pthread_mutex_init, #a); }

#define	PthreadCondInit(a, b) \
{ int _rval_; _rval_ = pthread_cond_init(a, b); if (_rval_ != 0) \
	LibFatal(pthread_cond_init, #a); }

#define	PthreadCancel(a) \
{ int _rval_; _rval_ = pthread_cancel(a); if (_rval_ != 0) \
	LibFatal(pthread_cancel, #a); }

/* Structures. */
typedef struct ThreadSema {
	pthread_mutex_t	mutex;		/* protect access to count */
	pthread_cond_t	cv;		/* signals change to count */
	int		count;		/* protected count */
} ThreadSema_t;

typedef struct ThreadState {
	pthread_mutex_t	mutex;		/* protect access to state */
	pthread_cond_t	cv;		/* signals change to state */
	int		state;		/* protected state */
} ThreadState_t;

/* Structure representing communication to another thread. */
typedef struct ThreadComm {
	pthread_mutex_t	mutex;		/* protect access to data */
	pthread_cond_t	avail;		/* data available */
	int		data_ready;	/* data present */
	void		*first;
	void		*last;
	pthread_t	tid;		/* thread id */
} ThreadComm_t;

/* Define prototypes in threads_impl.c */
void ThreadSemaInit(ThreadSema_t *s, int value);
void ThreadSemaWait(ThreadSema_t *s);
void ThreadSemaPost(ThreadSema_t *s);

void ThreadStateInit(ThreadState_t *s);
void ThreadStateWait(ThreadState_t *s);
void ThreadStatePost(ThreadState_t *s);

#endif /* STAGER_THREADS_H */
