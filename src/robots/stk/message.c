/*
 *	message.c - process incomming messages.
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

#include <thread.h>
#include <synch.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/shm.h"
#include "aml/message.h"
#include "aml/trace.h"
#include "aml/robots.h"
#include "sam/nl_samfs.h"
#include "stk.h"
#include "aml/proto.h"
#include "sam/lib.h"

#pragma ident "$Revision: 1.20 $"

/*	function prototypes */
void *inc_free(void *);
static void sig_catch();
void *stk_acs_response(void *);

/*	some globals */
extern thread_t threads[STK_MAIN_THREADS];
extern char *fifo_path;
extern shm_alloc_t master_shm, preview_shm;

/*	for handoff to stk_acs_response thread */
static mutex_t stk_acs_mutex = DEFAULTMUTEX;
static cond_t stk_acs_cond = DEFAULTCV;
static robo_event_t *stk_acs_event_head = NULL, *stk_acs_event_last = NULL;


/*	signal catcher to exit thread */
static void
sig_catch()
{
	thr_exit(NULL);
}


/*
 *	monitor_msg - thread routine to monitor messages.
 */
void *
monitor_msg(
	void *vlibrary)
{
	int 		exit_status = 0;
	sigset_t 	signal_set;
	library_t 	*library = (library_t *)vlibrary;
	robo_event_t 		*current_event;
	struct sigaction 	sig_action;
	message_request_t 	*message, shutdown;
	enum sam_mess_type 	mtype;

	/* dummy up a shutdown message */
	(void) memset(&shutdown, 0, sizeof (message_request_t));
	(void) memset(&sig_action, 0, sizeof (struct sigaction));

	shutdown.mtype = MESS_MT_SHUTDOWN;
	/* LINTED constant truncated by assignment */
	shutdown.message.magic = MESSAGE_MAGIC;
	shutdown.message.command = MESS_CMD_SHUTDOWN;

	/*
	 * Should have been called with all signals blocked,
	 * now let sigemt be delivered and just exit when it is
	 */
	sig_action.sa_handler = sig_catch;
	sig_action.sa_flags = 0;
	(void) sigemptyset(&signal_set);
	(void) sigaddset(&signal_set, SIGEMT);
	(void) sigaction(SIGEMT, &sig_action, (struct sigaction *)NULL);
	(void) thr_sigsetmask(SIG_UNBLOCK, &signal_set, NULL);

	mutex_lock(&library->mutex);	/* wait for initialize */
	mutex_unlock(&library->mutex);

	message = (message_request_t *)SHM_REF_ADDR(library->un->dt.rb.message);
	if (thr_create(NULL, MD_THR_STK, stk_acs_response, (void *)library,
	    (THR_BOUND | THR_NEW_LWP | THR_DETACHED), NULL)) {
		sam_syslog(LOG_CRIT,
		    "Unable to start stk_acs_response thread: %m.");
		thr_exit(NULL);
	}

	/* Main loop */
	for (;;) {
		current_event = get_free_event(library);

		/*
		 * Zeroing the struct has the effect of initializing
		 * the mutex and the condition to USYNC_THREAD, just
		 * what we want
		 */
		(void) memset(current_event, 0, sizeof (robo_event_t));
		current_event->status.bits = REST_FREEMEM;

		/* Wait for a message */
		mutex_lock(&message->mutex);
		while (message->mtype == MESS_MT_VOID)
			cond_wait(&message->cond_r, &message->mutex);

		/* Copy the request into the event */
		current_event->request.message = message->message;
		mtype = message->mtype;	/* capture message type */

		message->mtype = MESS_MT_VOID;	/* release the message area */
		message->message.exit_id.pid = 0;
		cond_signal(&message->cond_i);	/* and wake up anyone waiting */
		mutex_unlock(&message->mutex);

		if (mtype == MESS_MT_APIHELP) {
			current_event->next = NULL;
			mutex_lock(&stk_acs_mutex);
			/*
			 * If the list is NULL, this will be the only
			 * entry on the list.  Set the head and last to current
			 */
			if (stk_acs_event_head == NULL) {
				stk_acs_event_head =
				    stk_acs_event_last = current_event;
				cond_signal(&stk_acs_cond);
			} else {
				/*
				 * If the head is not null, last points to the
				 * last entry on the list. Point last
				 * next to the current then set last = current
				 */
				stk_acs_event_last->next = current_event;
				stk_acs_event_last = current_event;
			}

			mutex_unlock(&stk_acs_mutex);
		} else {
			current_event->type = EVENT_TYPE_MESS;

			/*
			 * Put the event on the list and
			 * wake up the event handler
			 */
			add_to_end(library, current_event);
			if (message->mtype == MESS_MT_SHUTDOWN) {
				if (DBG_LVL(SAM_DBG_DEBUG))
					sam_syslog(LOG_DEBUG,
					"shutdown request:%s:%d.",
					    __FILE__, __LINE__);
				threads[STK_MSG_THREAD] = (thread_t)-1;
				thr_exit(&exit_status);
				/* NOTREACHED */
				return (NULL);
			}
		}
	}
}


/*
 *	Get a free event and check to see if more are needed.
 */
robo_event_t *
get_free_event(
	library_t *library)
{
	robo_event_t *ret;
	char *ent_pnt = "get_free_event";

	mutex_lock(&library->free_mutex);

	if (library->free_count < 20 && !library->inc_free_running) {
		sigset_t signal_set;
		(void) sigemptyset(&signal_set);
		(void) sigaddset(&signal_set, SIGEMT);
		library->inc_free_running++;
		thr_sigsetmask(SIG_BLOCK, &signal_set, NULL);
		thr_create(NULL, MD_THR_STK, &inc_free, (void *)library,
		    (THR_DETACHED | THR_BOUND), NULL);
		thr_sigsetmask(SIG_UNBLOCK, &signal_set, NULL);
		thr_yield();
	}

	while (library->free_count <= 0) {
		mutex_unlock(&library->free_mutex);
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG,
			    "%s: Waiting for free event.", ent_pnt);
		sleep(2);
		mutex_lock(&library->free_mutex);
	}

	ret = library->free;
	ETRACE((LOG_NOTICE, "EV:LfGf: %#x.", ret));
	library->free_count--;
	library->free = ret->next;
	mutex_unlock(&library->free_mutex);
	return (ret);
}


/*
 *	inc_free - Get a new batch of events for the library free list.
 */
void *
inc_free(
	void *vlibrary)
{
	robo_event_t *new, *end;
	library_t *library = (library_t *)vlibrary;

	new = init_list(ROBO_EVENT_CHUNK);
	mutex_lock(&library->free_mutex);
	if (library->free == NULL)
		library->free = new;
	else {
		end = library->free;
		while (end->next != NULL) {
			end = end->next;
		}
		end->next = new;
		new->previous = end;
	}
	library->free_count += ROBO_EVENT_CHUNK;
	library->inc_free_running = 0;
	mutex_unlock(&library->free_mutex);
	thr_exit((void *)NULL);
}


/*
 *	get response from helper and pass on to waiting thread.
 */
void *
stk_acs_response(
	void *vlibrary)
{
	robo_event_t *event;
	library_t *library = (library_t *)vlibrary;

	/* Wait for an event then pull it off the list */
	while (TRUE) {
		stk_resp_acs_t *acs_resp;

		mutex_lock(&stk_acs_mutex);
		while (stk_acs_event_head == NULL)
			cond_wait(&stk_acs_cond, &stk_acs_mutex);

		/*
		 * This event is the current head.
		 * Set the head to the next entry.
		 */
		event = stk_acs_event_head;
		stk_acs_event_head = event->next;
		mutex_unlock(&stk_acs_mutex);
		event->next = NULL;
		acs_resp =
		    (stk_resp_acs_t *)&
		    event->request.message.param.start_of_request;
		if (acs_resp->hdr.event != NULL) {
			robo_event_t *old_event = acs_resp->hdr.event;

			if (DBG_LVL(SAM_DBG_TMESG))
				sam_syslog(LOG_DEBUG,
				    "MR:stk_acs_resp: %#x", old_event);

			/*
			 * get the response data copied over to
			 * the original event
			 */
			memcpy((void *)&old_event->
			    request.message.param.start_of_request,
			    (void *)acs_resp, sizeof (stk_resp_acs_t));

			/* signal the requester */
			disp_of_event(library, old_event, 0);
		}
		event->status.bits = REST_FREEMEM;
		/* handle message event */
		disp_of_event(library, event, 0);
	}
}
