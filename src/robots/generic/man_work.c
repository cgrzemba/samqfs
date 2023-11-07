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

#pragma ident "$Revision: 1.30 $"

static char *_SrcFile = __FILE__;

#include <thread.h>
#include <synch.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/dev_log.h"
#include "aml/shm.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "aml/trace.h"
#include "sam/nl_samfs.h"
#include "generic.h"
#include "aml/proto.h"
#include "sam/lib.h"

/*	function prototypes */
void post_shutdown(library_t *);
void start_thread_cmd(library_t *, robo_event_t *, void *(*) (void *));

/*	some globals */
extern shm_alloc_t master_shm, preview_shm;
extern thread_t threads[GENERIC_MAIN_THREADS];
extern int clean_request(library_t *library, robo_event_t *event);


void *
manage_list(
	void *vlibrary)
{
	dev_ent_t 	*un;
	int 		old_count;
	char 		*ent_pnt = "manage_list";
	time_t 		now, short_delay, auto_check;
	ushort_t	delayed;
	library_t 	*library = (library_t *)vlibrary;
	timestruc_t 	wait_time;
	robo_event_t 	*current, *next;

	un = library->un;
	mutex_lock(&library->mutex);	/* wait for initialization */
	mutex_unlock(&library->mutex);

	old_count = 0;
	delayed = 0;
	short_delay = 0;

	auto_check = (time(&now) + 5);

	for (;;) {
		mutex_lock(&library->list_mutex);

		/*
		 * See if there in anything to do
		 * wait if the active count is 0, or its equal to the same
		 * value when last woke up and there is a delayed request.
		 */
		if (library->active_count == 0 ||
		    ((old_count == library->active_count) && delayed)) {
			wait_time.tv_sec = time(&now) + library->un->delay;
			wait_time.tv_nsec = 0;
			if ((auto_check >= now) &&
			    (auto_check < wait_time.tv_sec))
				wait_time.tv_sec = auto_check;

			if (delayed && (short_delay < wait_time.tv_sec))
				wait_time.tv_sec = short_delay;

			if (wait_time.tv_sec > now) {
				cond_timedwait(&library->list_condit,
				    &library->list_mutex, &wait_time);
				if (library->chk_req) {
					library->chk_req = FALSE;
					if (library->un->state == DEV_ON)
						auto_check = 0;
				}
			}
		}
		time(&now);		/* get the current time */
		if (auto_check <= now) {
			mutex_unlock(&library->list_mutex);
			(void) check_requests(library);
			auto_check = now + library->un->delay;
			continue;
		}

		/* if there is something on the list */
		if ((old_count = library->active_count) == 0) {
			mutex_unlock(&library->list_mutex);
			continue;
		}
		short_delay = 0;
		delayed = FALSE;
		current = library->first;
		if (current == NULL) {
			DevLog(DL_ERR(5082));
			library->un->active = library->active_count = 0;
			mutex_unlock(&library->list_mutex);
			continue;
		}
		mutex_unlock(&library->list_mutex);

		do {
			mutex_lock(&library->list_mutex);

			next = current->next;
			if ((current->status.b.delayed) &&
			    (current->timeout > now)) {
				if (short_delay == 0)
					short_delay = current->timeout;
				else if (current->timeout < short_delay)
					short_delay = current->timeout;
				current = next;
				delayed = TRUE;	/* flag delayed requests */
				mutex_unlock(&library->list_mutex);
				continue;
			}
			ETRACE((LOG_NOTICE, "LbEv c %#x f %#x n %#x (%d)",
			    current, library->first, current->next,
			    library->active_count));
			if (current == library->first) {
				ETRACE((LOG_NOTICE, "LbEv c=f %#x (%d)",
				    current, library->active_count));
				library->first = unlink_list(current);
			} else {
				ETRACE((LOG_NOTICE,
				    "LbEv c!=f c %#x f %#x (%d)", current,
				    library->first, library->active_count));
				(void) unlink_list(current);
			}

			library->un->active = --library->active_count;

			if (library->active_count != 0 &&
			    library->first == NULL)
				ETRACE((LOG_NOTICE, "LbEL (%d)",
				    library->active_count));
			if (library->active_count == 0 &&
			    library->first != NULL)
				ETRACE((LOG_NOTICE, "LbEG (%d)",
				    library->first));

			mutex_unlock(&library->list_mutex);

			/* entry is off the list and ready to process */
			ETRACE((LOG_NOTICE, "LbCurr %#x type %d", current, current->type ));
			switch (current->type) {
			case EVENT_TYPE_INTERNAL:
				switch (current->request.internal.command) {
				case ROBOT_INTRL_AUDIT_SLOT:
					if (start_audit(library, current,
					    current->request.internal.slot)) {
						/*
						 * Unable to find resources,
						 * delay request and try later.
						 */
						current->status.b.delayed =
						    TRUE;
						current->timeout = now + 10;
						delayed = TRUE;
						if (!short_delay)
							short_delay =
							    current->timeout;
						add_to_end(library, current);
					}
					current = next;
					break;

				default:
					sam_syslog(LOG_INFO,
					    "%s: Bad internal event: %s:%d",
					    ent_pnt, __FILE__, __LINE__);
					break;
				}
				break;

			case EVENT_TYPE_MESS:	/* came off the message queue */
				ETRACE((LOG_NOTICE, "LbCurr %#x mess type %d", current, current->request.message.command));
				if (current->request.message.magic !=
				    MESSAGE_MAGIC) {
					sam_syslog(LOG_INFO,
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
					post_shutdown(library);
					threads[GENERIC_WORK_THREAD] =
					    (thread_t)- 1;
					thr_exit((void *)NULL);
					break;

				case MESS_CMD_STATE:
					/*
					 * state_request puts event back on the
					 * free list when the command is done.
					 */
					state_request(library, current);
					current = next;
					break;

				case MESS_CMD_TAPEALERT:
					/*
					 * tapealert_request puts event back on
					 * free list when the command is done.
					 */
					tapealert_solicit(library, current);
					current = next;
					break;

				case MESS_CMD_SEF:
					/*
					 * sef_request puts event back on
					 * free list when the command is done.
					 */
					sef_solicit(library, current);
					current = next;
					break;

				case MESS_CMD_LABEL:
					if (label_request(library, current)) {
						/*
						 * unable to find resources,
						 * delay request and try later.
						 */
						current->status.b.delayed =
						    TRUE;
						current->timeout = now + 10;
						if (!short_delay)
							short_delay =
							    current->timeout;
						delayed = TRUE;
						add_to_end(library, current);
					}
					current = next;
					break;

				case MESS_CMD_MOUNT:
					/*
					 * mount_request put
					 * the event back on the free list when
					 * the command is done or if it is bad.
					 */
					if (mount_request(library, current)) {
						/*
						 * unable to find resources,
						 * delay request and try later.
						 */
						current->status.b.delayed =
						    TRUE;
						current->timeout = now + 10;
						if (!short_delay)
							short_delay =
							    current->timeout;
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
					if (start_audit(library, current,
		    				current->request.message.param.audit_request.slot)) {
						/*
						 * unable to find resources,
						 * delay request and try later.
						 */
						current->status.b.delayed =
						    TRUE;
						current->timeout = now + 10;
						if (!short_delay)
							short_delay =
							    current->timeout;
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
					 * unload_request puts the event back on
					 * free list when command is done.
					 *
					 * Unload request adds the request
					 * to the drives worklist.
					 */
					unload_request(library, current);
					current = next;
					break;

				case MESS_CMD_CLEAN:
					/*
					 * clean_request puts event back on
					 * free list when the command is done.
					 *
					 * clean_request adds the request
					 * to the drives worklist.
					 */
					clean_request(library, current);
					current = next;
					break;

				case MESS_CMD_IMPORT:
					/*
					 * import_request puts event back on
					 * free list when the command is done.
					 *
					 * import_request adds request to the
					 * mail-box worklist.
					 */
					if (IS_GENERIC_API(library->un->type)) {
				sam_syslog(LOG_ERR,
				    "%s: Unknown robot command %d.", ent_pnt,
				    current->request.message.command);
						disp_of_event(library, current,
						    EBADF);
					} else {
						import_request(library,
						    current);
					}
					current = next;
					break;

				case MESS_CMD_ADD:
					/*
					 * add_to_cat_req puts event back on the
					 * free list when the command is done.
					 */
					{
						int local_status = 0;

						if (IS_GENERIC_API(
						    library->un->type)) {
							add_to_cat_req(library,
							    current);
						} else {
					sam_syslog(LOG_ERR,
					    "%s: Unknown robot command %d.",
					    ent_pnt,
					    current->request.message.command);
							local_status = EBADF;
						}
						disp_of_event(library, current,
						    local_status);
					}
					current = next;
					break;

				case MESS_CMD_EXPORT:
					/*
					 * export_request puts event back on the
					 * free list when the command is done.
					 *
					 * export_request will add request to
					 * mail-box worklist.
					 */
					if (IS_GENERIC_API(library->un->type))
						export_media((void *)library,
						    current, library->un->type);
					else
						export_request(library,
						    current);
					current = next;
					break;

				case MESS_CMD_TODO:
					/*
					 * todo_request puts event back on the
					 * free list when the command is done.
					 */
					todo_request(library, current);
					current = next;
					break;

				case MESS_CMD_MOVE:
					/*
					 * move_request puts event back on the
					 * free list when the command is done.
					 * This request runs as a thread.
					 */
					if (IS_GENERIC_API(library->un->type)) {
						sam_syslog(LOG_ERR,
				    		"%s: Unknown robot command %d.", ent_pnt,
				    		current->request.message.command);
						disp_of_event(library, current, EBADF);
					} else {
						start_thread_cmd(library,
						    current, move_request);
					}
					current = next;
					break;

				case MESS_CMD_ACK:
					/* A no-op. Dispose of event. */
					disp_of_event(library, current, 0);
					current = next;
					break;

				default:
					sam_syslog(LOG_INFO,
					    "%s: Unknown robot command %d.",
					    ent_pnt,
					    current->request.message.command);
					disp_of_event(library, current, EBADF);
					current = next;
					break;
				}

				break;

			default:
				sam_syslog(LOG_INFO,
				    "%s: Unknown event type %d.", ent_pnt,
				    current->type);
				disp_of_event(library, current, EBADF);
				current = next;
				break;
			}
		} while (current != NULL);
	}
}


/*
 *	post_shutdown - send a shutdown request to all elements
 */
void
post_shutdown(
	library_t *library)
{
	int 		active;
	time_t 		start_time;
	drive_state_t 	*drive;
	xport_state_t 	*transport = library->transports;
	iport_state_t 	*import = library->import;
	robo_event_t 	*event;

	(void) time(&start_time);

	for (drive = library->drive; drive != NULL; drive = drive->next) {
		event = (robo_event_t *)
		    malloc_wait(sizeof (robo_event_t), 5, 0);
		memset(event, 0, sizeof (robo_event_t));
		event->type = EVENT_TYPE_INTERNAL;
		event->status.bits = REST_FREEMEM;
		event->request.internal.command = ROBOT_INTRL_SHUTDOWN;
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

	event = (robo_event_t *)malloc_wait(sizeof (robo_event_t), 5, 0);
	memset(event, 0, sizeof (robo_event_t));
	event->type = EVENT_TYPE_INTERNAL;
	event->status.bits = REST_FREEMEM;
	event->request.internal.command = ROBOT_INTRL_SHUTDOWN;
	mutex_lock(&transport->list_mutex);
	add_active_list(transport->first, event);
	transport->active_count++;
	mutex_unlock(&transport->list_mutex);

	event = (robo_event_t *)malloc_wait(sizeof (robo_event_t), 5, 0);
	memset(event, 0, sizeof (robo_event_t));
	event->type = EVENT_TYPE_INTERNAL;
	event->status.bits = REST_FREEMEM;
	event->request.internal.command = ROBOT_INTRL_SHUTDOWN;
	mutex_lock(&import->list_mutex);
	add_active_list(import->first, event);
	import->active_count++;
	mutex_unlock(&import->list_mutex);

	thr_yield();
	sleep(1);

	while ((time(NULL) < (start_time + 10)) &&
	    (transport->thread < 0 && import->thread < 0))
		sleep(1);
}


/*
 *	start_thread_cmd.
 *
 * start a command on a thread, passing the event and library.
 * Called thread is responsible for freeing the malloced memory.
 *
 */
void
start_thread_cmd(
	library_t *library,
	robo_event_t *event,
	void *func(void *))
{
	char *ent_pnt = "start_thread_cmd";
	robot_threaded_cmd *cmd;

	cmd = malloc_wait(sizeof (robot_threaded_cmd), 2, 0);
	cmd->library = library;
	cmd->event = event;
	if (thr_create(NULL, MD_THR_STK, func, (void *)cmd,
	    THR_DETACHED, NULL)) {
		char *errmes = error_handler(errno);
		Dl_info sym_info;

		memset(&sym_info, 0, sizeof (Dl_info));
		if (dladdr((void *)func, &sym_info) &&
		    sym_info.dli_sname != NULL)
			sam_syslog(LOG_INFO,
			    "%s: Unable to create thread %s:%s.", ent_pnt,
			    sym_info.dli_sname, errmes);
		else
			sam_syslog(LOG_INFO,
			    "%s: Unable to create thread unknown:%s.",
			    ent_pnt, errmes);

		free(cmd);
	}
}
