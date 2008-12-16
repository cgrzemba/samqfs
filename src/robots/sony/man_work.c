/*
 * manage_list - manage the primary worklist.
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

#include <thread.h>
#include <synch.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/shm.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "aml/trace.h"
#include "sam/nl_samfs.h"
#include "sony.h"
#include "aml/proto.h"
#include "sam/lib.h"

#pragma ident "$Revision: 1.23 $"


/* function prototypes */

void post_shutdown(library_t *);

/* some globals */

extern char *fifo_path;
extern shm_alloc_t master_shm, preview_shm;
extern thread_t threads[SONY_MAIN_THREADS];

void *
manage_list(void *vlibrary)
{
	int			exit_status = 0, old_count;
	char			*ent_pnt = "manage_list";
	ushort_t		delayed;
	time_t			now, short_delay, auto_check;
	robo_event_t		*current, *next;
	library_t		*library = (library_t *)vlibrary;

	mutex_lock(&library->mutex);    /* wait for initialization */
	mutex_unlock(&library->mutex);

	short_delay = 0;
	old_count = 0;
	delayed = 0;
	auto_check = (time(&now) + 5);

	for (;;) {
		mutex_lock(&library->list_mutex);

		/*
		 * See if there in anything to do.  We will wait if the
		 * active count is 0 or its equal to the same value it had
		 * when we last woke up and there is a delayed request.
		 */
		if (library->active_count == 0 ||
		    ((old_count == library->active_count) && delayed)) {

			timestruc_t	wait_time;

			wait_time.tv_sec = time(&now) + library->un->delay;
			wait_time.tv_nsec = 0;
			if ((auto_check >= now) &&
			    (auto_check <  wait_time.tv_sec))

				wait_time.tv_sec = auto_check;

			if (delayed && (short_delay < wait_time.tv_sec))
				wait_time.tv_sec = short_delay;

			if (wait_time.tv_sec > now) {
				cond_timedwait(&library->list_condit,
				    &library->list_mutex, &wait_time);
				if (library->chk_req) {
					library->chk_req = FALSE;
					if (library->un->state == DEV_ON)
						/*
						 * Force a check
						 */
						auto_check = 0;
				}
			}
		}

		/*
		 * Get the current time
		 */
		time(&now);
		if (auto_check <= now) {
			mutex_unlock(&library->list_mutex);
			(void) check_requests(library);
			auto_check = now + library->un->delay;
			continue;
		}

		/*
		 * If there is something on the list . . .
		 */
		if ((old_count = library->active_count) == 0) {
			mutex_unlock(&library->list_mutex);
			continue;
		}

		short_delay = 0;
		delayed = FALSE;
		current = library->first;
		mutex_unlock(&library->list_mutex);

		do {
			mutex_lock(&library->list_mutex);
			/*
			 * If delayed and the time has not expired,
			 * go on tothe next
			 */
			next = current->next;
			if ((current->status.b.delayed) &&
			    (current->timeout > now)) {

				if (short_delay == 0)
					short_delay = current->timeout;
				else if (current->timeout < short_delay)
					short_delay = current->timeout;
				current = next;
				/*
				 * Need to know there are delayed requests
				 */
				delayed = TRUE;

				mutex_unlock(&library->list_mutex);
				continue;
			}

			if (current == library->first)
				library->first = unlink_list(current);
			else
				(void) unlink_list(current);

			current->next = NULL;
			ETRACE((LOG_NOTICE, "LbEv c %#x n %#x (%d)\n", current,
			    library->first, library->active_count));
			library->active_count--;
			library->un->active = library->active_count;
			mutex_unlock(&library->list_mutex);

			/*
			 * Entry is off the list and ready to process
			 */
			switch (current->type) {
			case EVENT_TYPE_INTERNAL:

				switch (current->request.internal.command) {

				case ROBOT_INTRL_AUDIT_SLOT:
					if (start_audit(library, current,
					    current->request.internal.slot)) {
						/*
						 * Unable to find resources,
						 * delay the request and try
						 * later
						 */
						current->status.b.delayed
						    = TRUE;
						current->timeout = now + 10;
						delayed = TRUE;
						add_to_end(library, current);
					}
					current = next;
					break;

				default:
					sam_syslog(LOG_ERR,
					    "%s:Bad internal event: %s:%d\n",
					    ent_pnt, __FILE__, __LINE__);

					break;
				}

				break;

			case EVENT_TYPE_MESS:
				if (current->request.message.magic
				    != MESSAGE_MAGIC) {
					sam_syslog(LOG_ERR,
					    "%s: Bad magic %#x.", ent_pnt,
					    current->request.message.magic);
					current->completion = EAGAIN;
					disp_of_event(library, current, EBADF);
					current = next;
					continue;
				}
				if (library->un->state >= DEV_OFF &&
				    (current->request.message.command >
				    ACCEPT_DOWN)) {

					current->completion = EAGAIN;
					disp_of_event(library, current, EAGAIN);
					current = next;
					continue;
				}

			switch (current->request.message.command) {

				case MESS_CMD_SHUTDOWN:
					if (DBG_LVL(SAM_DBG_DEBUG))
						sam_syslog(LOG_DEBUG,
						    "received"
						    " shutdown:%s:%d.\n",
						    __FILE__, __LINE__);
					post_shutdown(library);
					threads[SONY_WORK_THREAD]
					    = (thread_t)-1;
					thr_exit(&exit_status);
					break;

				case MESS_CMD_STATE:
					/*
					 * state_request will put the event
					 * back on the free list when
					 * the command is done.
					 */
					state_request(library, current);
					current = next;
					break;

				case MESS_CMD_TAPEALERT:
					/*
					 * tapealert_request will put the
					 * event back on the
					 * free list when the command is done.
					 */
					tapealert_solicit(library, current);
					current = next;
					break;

				case MESS_CMD_SEF:
					/*
					 * sef_request will put the event
					 * back on the free list when the
					 * command is done.
					 */
					sef_solicit(library, current);
					current = next;
					break;

				case MESS_CMD_LABEL:
					if (label_request(library, current)) {
						/*
						 * Unable to find resources,
						 * delay the request, try later.
						 */
						current->status.b.delayed
						    = TRUE;
						current->timeout = now + 10;
						delayed = TRUE;
						add_to_end(library, current);
					}
					current = next;
					break;

				case MESS_CMD_MOUNT:
					/*
					 * mount_request will take care of
					 * putting the event back on free list
					 */
					if (mount_request(library, current)) {
						/*
						 * Unable to find resources,
						 * delay request and try later.
						 */
						current->status.b.delayed
						    = TRUE;
						current->timeout = now + 10;
						delayed = TRUE;
						add_to_end(library, current);
					}
					current = next;
					break;

				case MESS_CMD_LOAD_UNAVAIL:
					load_unavail_request(library, current);
					current = next;
					break;

				case MESS_CMD_AUDIT:
				if (start_audit(library, current, current->
				    request.message.param.audit_request.slot)) {
						current->status.b.delayed
						    = TRUE;
						current->timeout = now + 10;
						delayed = TRUE;
						add_to_end(library, current);
					}
					current = next;
					break;

				case MESS_CMD_PREVIEW:
					(void) check_requests(library);
					time(&now);
					auto_check = now + library->un->delay;
					disp_of_event(library, current, 0);
					current = next;
					break;

				case MESS_CMD_UNLOAD:
					/*
					 * unload_request will put the event
					 * back on the free list when
					 * the command is done.
					 * unload_request will add the request
					 * to the drive's worklist.
					 */
					unload_request(library, current);
					current = next;
					break;

				case MESS_CMD_TODO:
					todo_request(library, current);
					current = next;
					break;

				case MESS_CMD_ADD:
					add_to_cat_req(library, current);
					current = next;
					break;

				case MESS_CMD_EXPORT:
					/*
					 * export_request will add the request
					 * to the
					 * mailbox worklist.
					 */
					export_media(library, current);
					current = next;
					break;

				case MESS_CMD_ACK:
					/*
					 * A no-op. Dispose of event.
					 */
					disp_of_event(library, current, 0);
					current = next;
					break;

				default:
					sam_syslog(LOG_ERR,
					    "%s: Unknown robot command %d.",
					    ent_pnt,
					    current->request.message.command);

					disp_of_event(library, current, 0);
					current = next;
					break;
				}

			break;

			default:
				sam_syslog(LOG_ERR,
				    "%s: Unknown event type %d.\n",
				    ent_pnt, current->type);
				disp_of_event(library, current, EBADF);
				current = next;
				break;
			}
			break;
		} while (current != NULL);
	}
}


/*
 * post_shutdown - send a shutdown request to all elements
 */
void
post_shutdown(library_t *library)
{
	int		active;
	time_t		start_time;
	robo_event_t	shut_down, *event;
	drive_state_t	*drive;
	xport_state_t	*transport = library->transports;

	shut_down.type = EVENT_TYPE_INTERNAL;
	shut_down.request.internal.command = ROBOT_INTRL_SHUTDOWN;
	(void) time(&start_time);

	for (drive = library->drive; drive != NULL; drive = drive->next) {
		event = malloc_wait(sizeof (robo_event_t), 5, 0);
		*event = shut_down;
		mutex_lock(&drive->list_mutex);
		add_active_list(drive->first, event);
		drive->active_count++;
		mutex_unlock(&drive->list_mutex);
		cond_signal(&drive->list_condit);
	}

	sleep(1);
	active = TRUE;

	while ((time(NULL) < (start_time + 10)) && active) {
		active = FALSE;
		for (drive = library->drive; drive != NULL; drive = drive->next)
			active = active || (drive->thread < 0);
	}

	event = malloc_wait(sizeof (robo_event_t), 5, 0);
	*event = shut_down;
	mutex_lock(&transport->list_mutex);
	add_active_list(transport->first, event);
	transport->active_count++;
	mutex_unlock(&transport->list_mutex);

	sleep(1);

	while ((time(NULL) < (start_time + 10)) && (transport->thread < 0))
		sleep(1);
}
