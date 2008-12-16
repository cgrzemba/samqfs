/*
 * thread.c - stager's pthread api
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

#pragma ident "$Revision: 1.16 $"

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <pthread.h>

/* Solaris headers. */
#include <sys/shm.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/shm.h"
#include "aml/tar.h"
#include "pub/stat.h"
#include "sam/fioctl.h"
#include "pub/rminfo.h"
#include "aml/stager_defs.h"

/* Local headers. */
#include "stager_lib.h"
#include "stager_threads.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

void
ThreadSemaInit(
	ThreadSema_t *s,
	int value)
{
	pthread_mutex_init(&(s->mutex), NULL);
	pthread_cond_init(&(s->cv), NULL);
	s->count = value;
}

void
ThreadSemaWait(
	ThreadSema_t *s)
{
	pthread_mutex_lock(&(s->mutex));
	while (s->count == 0) pthread_cond_wait(&s->cv, &(s->mutex));
	s->count--;
	pthread_mutex_unlock(&(s->mutex));
}

void
ThreadSemaPost(
	ThreadSema_t *s)
{
	pthread_mutex_lock(&(s->mutex));
	s->count++;
	pthread_mutex_unlock(&(s->mutex));
	pthread_cond_signal(&s->cv);
}

void
ThreadStateInit(
	ThreadState_t *s)
{
	pthread_mutex_init(&(s->mutex), NULL);
	pthread_cond_init(&(s->cv), NULL);
	s->state = 0;
}

void
ThreadStateWait(
	ThreadState_t *s)
{
	pthread_mutex_lock(&(s->mutex));
	while (s->state == 0) pthread_cond_wait(&s->cv, &(s->mutex));
	s->state = 0;
	pthread_mutex_unlock(&(s->mutex));
}

void
ThreadStatePost(
	ThreadState_t *s)
{
	pthread_mutex_lock(&(s->mutex));
	s->state = 1;
	pthread_mutex_unlock(&(s->mutex));
	pthread_cond_signal(&s->cv);
}
