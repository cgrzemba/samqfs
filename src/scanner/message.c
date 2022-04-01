/*
 * message.c - process incomming messages.
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

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <thread.h>
#include <synch.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/mtio.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "sam/types.h"
#include "sam/param.h"
#include "sam/lib.h"
#include "aml/device.h"
#include "aml/shm.h"
#include "sam/defaults.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"
#include "aml/tapealert.h"
#include "aml/sef.h"
#include "sam/lib.h"

#define	REQUEST_NOT_COMPLETE  -100

/* function prototypes */

void scanner_message(message_request_t *);
void *todo_request(void *);
void *state_change(void *);
void start_thread(message_request_t *, void *(*) (void *));
void *tapealert_action(void *vmessage);
void *sef_action(void *vmessage);

/* some globals */
extern shm_alloc_t master_shm, preview_shm;

/*
 * monitor_msg - thread routine to monitor messages.
 */
void *
monitor_msg(void *vscan_tid)
{
	thread_t *scan_tid = (thread_t *)(vscan_tid);
	sigset_t signal_set;
	shm_ptr_tbl_t  *shm_ptr_tbl;
	message_request_t *message;

	(void) sigfillset(&signal_set);	/* Block all signals */
	(void) thr_sigsetmask(SIG_SETMASK, &signal_set, (sigset_t *)NULL);

	(void) sigemptyset(&signal_set);
	(void) sigaddset(&signal_set, SIGEMT);	/* Allow SIGEMT */
	(void) thr_sigsetmask(SIG_UNBLOCK, &signal_set, NULL);

	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
	message = (message_request_t *)SHM_REF_ADDR(shm_ptr_tbl->scan_mess);

	/* Main loop */
	mutex_lock(&message->mutex);
	for (;;) {
		while (message->mtype == MESS_MT_VOID)
			cond_wait(&message->cond_r, &message->mutex);

		scanner_message(message);
		message->mtype = MESS_MT_VOID;
		message->message.exit_id.pid = 0;
		cond_signal(&message->cond_i);	/* and wake up anyone waiting */
	}
}

void
scanner_message(message_request_t *request)
{
	switch (request->message.command) {

		case MESS_CMD_TODO:
		/*
		 * todo_request will put the event back on the free list when
		 * the command is done.
		 */
		(void) start_thread(request, todo_request);
		break;

	case MESS_CMD_STATE:
		(void) start_thread(request, state_change);
		break;

	case MESS_CMD_LABEL:
		(void) start_thread(request, wt_labels);
		break;

	case MESS_CMD_TAPEALERT:
		(void) start_thread(request, tapealert_action);
		break;

	case MESS_CMD_SEF:
		(void) start_thread(request, sef_action);
		break;

	default:
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG,
			    "scanner_message:Unknown message: %#x.",
			    request->message.command);
		break;
	}
}

/*
 * state_change - device is changing states.
 * called as a thread.
 *
 */
void *
state_change(void *vmessage)
{
	char *ent_pnt = "state_change";
	dev_ent_t *un;
	dev_ptr_tbl_t *dev_ptr_tbl;
	sam_defaults_t *defaults;
	state_change_t *request;
	message_request_t *message = (message_request_t *)vmessage;
	int error = 0;

	request = &message->message.param.state_change;

	dev_ptr_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->dev_table);

	defaults = GetDefaults();

	if (request->eq > (equ_t)dev_ptr_tbl->max_devices ||
	    dev_ptr_tbl->d_ent[request->eq] == 0) {
		sam_syslog(LOG_ERR,
		    catgets(catfd, SET, 20001, "%s: No such device (%d)."),
		    ent_pnt, request->eq);
		write_client_exit_string(&message->message.exit_id,
		    ENOENT, NULL);
		free(vmessage);
		thr_exit((void *) NULL);
	}
	un = (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->d_ent[request->eq]);
	mutex_lock(&un->mutex);
	switch (request->state) {	/* what state are we moving to */
	case DEV_ON:		/* switching to */
		switch (request->old_state) {
		case DEV_IDLE:
			/* FALLTHROUGH */
		case DEV_UNAVAIL:
			/* FALLTHROUGH */
		case DEV_OFF:
			/* FALLTHROUGH */
		case DEV_DOWN:
			OnDevice(un);
			if (IS_TAPE(un))
				ChangeMode(un->name, SAM_TAPE_MODE);
			ident_dev(un, -1);
			break;

		default:
			sam_syslog(LOG_WARNING,
			    catgets(catfd, SET, 20002,
			    "Invalid state change(%d)."), un->eq);
			error = EINVAL;
			break;
		}
		break;

	case DEV_RO:		/* switching to */
		break;

	case DEV_IDLE:		/* switching to */
		if (request->old_state == DEV_ON) {
			un->state = DEV_IDLE;
			if (un->active != 0)
				break;
		} else {
			sam_syslog(LOG_WARNING,
			    catgets(catfd, SET, 20002,
			    "Invalid state change(%d)."), un->eq);
			error = EINVAL;
			break;
		}
		/* FALLTHROUGH */

	case DEV_UNAVAIL:	/* switching to */
		if (request->old_state == DEV_DOWN) {
			sam_syslog(LOG_WARNING,
			    catgets(catfd, SET, 20002,
			    "Invalid state change(%d)."), un->eq);
			error = EINVAL;
			break;
		}
		/* FALLTHROUGH */

	case DEV_OFF:		/* switching to */
		if (un->active == 0) {
			int fd = -1;	/* need an open file descriptor */

			if (IS_TAPE(un))
				ChangeMode(un->name, defaults->tapemode);
			if ((fd = open(un->name, (O_RDWR | O_NDELAY))) < 0)
				if (IS_TAPE(un) && (errno == EACCES))
					fd = open(un->name,
					    (O_RDONLY | O_NDELAY));

			/*
			 * If no active i/o and changing state to OFF or
			 * IDLE, the device is switched off.
			 */
			if (request->state == DEV_OFF ||
			    request->state == DEV_IDLE)
				OffDevice(un, USER_STATE_CHANGE);
			else
				un->state = DEV_UNAVAIL;

			if (fd >= 0) {
				un->status.bits = DVST_PRESENT;
				(void) memset(un->vsn, ' ', 32);
				mutex_lock(&un->io_mutex);
				TAPEALERT(fd, un);
				(void) scsi_cmd(fd, un, SCMD_DOORLOCK, 0,
				    UNLOCK);
				if (IS_TAPE(un))
					un->dt.tp.position = 0;
				else if (request->old_state < DEV_UNAVAIL)
					notify_fs_invalid_cache(un);

				TAPEALERT(fd, un);
				(void) scsi_cmd(fd, un, SCMD_START_STOP, 0,
				    SPINDOWN, EJECT_MEDIA);
				TAPEALERT(fd, un);
				mutex_unlock(&un->io_mutex);
				close(fd);
				DEC_OPEN(un);
			} else {
				un->status.bits = DVST_PRESENT;
				(void) memset(un->vsn, ' ', 32);
			}
		} else
			un->state = DEV_IDLE;
		break;

	case DEV_DOWN:
		DownDevice(un, USER_STATE_CHANGE);
		break;

	default:
		sam_syslog(LOG_WARNING, catgets(catfd, SET, 20002,
		    "Invalid state change(%d)."), un->eq);
		error = EINVAL;
	}

	write_client_exit_string(&message->message.exit_id, error, NULL);

	mutex_unlock(&un->mutex);
	free(vmessage);
	thr_exit((void *) NULL);
}

/*
 * tapealert_action - perform device tapealert action.
 *
 * called as a thread.
 */
void *
tapealert_action(void *vmessage)
{
	char *ent_pnt = "state_change";
	dev_ent_t *un;
	dev_ptr_tbl_t *dev_ptr_tbl;
	message_request_t *message = (message_request_t *)vmessage;
	tapealert_request_t *request;

	request = &message->message.param.tapealert_request;

	dev_ptr_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->dev_table);

	if (request->eq > (equ_t)dev_ptr_tbl->max_devices ||
	    dev_ptr_tbl->d_ent[request->eq] == 0) {
		sam_syslog(LOG_ERR,
		    catgets(catfd, SET, 20001, "%s: No such device (%d)."),
		    ent_pnt, request->eq);
		free(vmessage);
		thr_exit((void *) NULL);
	}
	un = (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->d_ent[request->eq]);
	if (un == NULL) {
		free(vmessage);
		thr_exit((void *) NULL);
	}
	mutex_lock(&un->mutex);
	/* what state are we moving to */
	if (request->flags & TAPEALERT_ENABLED) {
		un->tapealert |= TAPEALERT_ENABLED;
		get_supports_tapealert(un, -1);
	} else {
		un->tapealert &= ~TAPEALERT_ENABLED;
	}
	mutex_unlock(&un->mutex);
	free(vmessage);
	thr_exit((void *) NULL);
}

/*
 * sef_action - perform device sef action.
 *
 * called as a thread.
 */
void *
sef_action(void *vmessage)
{
	char *ent_pnt = "state_change";
	dev_ent_t *un;
	dev_ptr_tbl_t *dev_ptr_tbl;
	message_request_t *message = (message_request_t *)vmessage;
	sef_request_t  *request;

	request = &message->message.param.sef_request;

	dev_ptr_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->dev_table);

	if (request->eq > (equ_t)dev_ptr_tbl->max_devices ||
	    dev_ptr_tbl->d_ent[request->eq] == 0) {
		sam_syslog(LOG_ERR,
		    catgets(catfd, SET, 20001, "%s: No such device (%d)."),
		    ent_pnt, request->eq);
		free(vmessage);
		thr_exit((void *) NULL);
	}
	un = (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->d_ent[request->eq]);
	if (un == NULL) {
		free(vmessage);
		thr_exit((void *) NULL);
	}
	mutex_lock(&un->mutex);
	/* what state are we moving to */
	if (request->flags & SEF_ENABLED) {
		un->sef_sample.state |= SEF_ENABLED;
		get_supports_sef(un, -1);
	} else {
		un->sef_sample.state &= ~SEF_ENABLED;
	}
	un->sef_sample.interval = request->interval;
	mutex_unlock(&un->mutex);
	free(vmessage);
	thr_exit((void *) NULL);
}

/*
 * todo - add request to the current stage/bufferio process
 *
 * Note:  The active count should be incremented BEFORE placing the
 * request in the scanner's message area.  This is to prevent the
 * scanner from doing something to the device between making the request
 * and the scanner picking up the request.  Runs as a thread.
 */
void *
todo_request(void *vmessage)
{
	char *ent_pnt = "todo_request";
	dev_ent_t *un;
	dev_ptr_tbl_t *dev_ptr_tbl;
	message_request_t *message = (message_request_t *)vmessage;
	todo_request_t *request = &message->message.param.todo_request;

	dev_ptr_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->dev_table);

	if (request->eq < 0 || request->eq > (equ_t)dev_ptr_tbl->max_devices ||
	    dev_ptr_tbl->d_ent[request->eq] == 0) {
		sam_syslog(LOG_ERR, catgets(catfd, SET, 20001,
		    "%s: No such device (%d)."), ent_pnt, request->eq);
		free(vmessage);
		thr_exit((void *) NULL);
	}
	un = (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->d_ent[request->eq]);

	switch (request->sub_cmd) {
	case TODO_ADD:
#if defined(DEBUG)
		sam_syslog(LOG_DEBUG, "todo_request add mount %#x", request->callback);
#endif
		{
			if (request->callback == CB_POSITION_RMEDIA) {
				mutex_lock(&un->io_mutex);
				position_rmedia(un);
				mutex_unlock(&un->io_mutex);
				DEC_ACTIVE(un);
				break;
			} else if (request->callback == CB_NOTIFY_FS_LOAD) {
				/*
				 * notify_fs_mount requires the mutex and
				 * increments the active count, so do so and
				 * then decrement the active count upon
				 * return since the todo requested
				 * incremented it before posting the request.
				 */
				mutex_lock(&un->mutex);
				notify_fs_mount(&request->handle,
				    &request->resource, un, 0);
				DEC_ACTIVE(un);
				mutex_unlock(&un->mutex);
			} else if (request->callback == CB_NOTIFY_TP_LOAD) {
				/*
				 * notify_tp_mount requires the mutex and
				 * increments the active count, so do so and
				 * then decrement the active count upon
				 * return since the todo requested
				 * incremented it before posting the request.
				 */
				mutex_lock(&un->mutex);
				notify_tp_mount(&request->handle, un, 0);
				DEC_ACTIVE(un);
				mutex_unlock(&un->mutex);
			} else {
				mutex_lock(&un->mutex);
				DEC_ACTIVE(un);
				mutex_unlock(&un->mutex);
				notify_fs_mount(&request->handle,
				    &request->resource, NULL, ENXIO);
			}
		}

		break;

		/* Cancel stage or mount requests */
	case TODO_CANCEL:
		switch (request->callback) {

		case CB_NOTIFY_FS_LOAD:
#if defined(DEBUG)
			sam_syslog(LOG_DEBUG, "todo cancel mount.");
#endif
			break;

		default:
			sam_syslog(LOG_WARNING,
			    "%s:Unknown cancel request %d.", ent_pnt,
			    request->callback);
			break;
		}

		break;

	case TODO_UNLOAD:
		unload_media(un, request);
		break;
	}

	free(vmessage);
	thr_exit((void *) NULL);
}

/*
 * start_thread.
 *
 * start a function on a thread, passing a copy of the message.
 * Called thread is responsible for freeing the malloced memory.
 *
 */
void
start_thread(message_request_t *command, void *func(void *))
{
	message_request_t *new_req;

	new_req = (message_request_t *)malloc_wait(sizeof (message_request_t),
	    2, 0);
	memcpy(new_req, command, sizeof (message_request_t));

	if (thr_create(NULL, MD_THR_STK, func, (void *) new_req, THR_DETACHED,
	    NULL)) {
		char *errmes = error_handler(errno);
		Dl_info	 sym_info;

		memset(&sym_info, 0, sizeof (Dl_info));
		if (dladdr((void *) func, &sym_info) &&
		    sym_info.dli_sname != NULL)
			sam_syslog(LOG_INFO, "Unable to create thread %s:%s.",
			    sym_info.dli_sname, errmes);
		else
			sam_syslog(LOG_INFO, "Unable to create thread %s:%s.",
			    "unknown", errmes);
		free(new_req);
	}
}
