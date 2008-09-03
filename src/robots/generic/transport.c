/*
 *	transport.c - thread that watches over a transport element
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
#pragma ident "$Revision: 1.39 $"

static char *_SrcFile = __FILE__;

#include <stdio.h>
#include <thread.h>
#include <synch.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/signal.h>
#include <stdarg.h>

#include "driver/samst_def.h"
#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/dev_log.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "aml/trace.h"
#include "sam/nl_samfs.h"
#include "generic.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/tapealert.h"
#include "element.h"

/*	function prototypes */
int element_type(library_t *, uint_t element);
int exchange(library_t *, robo_event_t *);
int move(library_t *, robo_event_t *);
char *element_string(int);
void init_transport(xport_state_t *);
void rezero_unit(library_t *);

/*	GRAU Functions */
void *api_load_command(void *);
void *api_force_command(void *);
void *api_dismount_command(void *);
void *api_view_command(void *);
void *api_drive_access_command(void *);
void *api_query_drive_command(void *);
void *api_getsideinfo_command(void *);
void api_start_request(library_t *, char *, int, robo_event_t *, int, ...);

/*	some globals */
extern shm_alloc_t master_shm, preview_shm;


/*
 *	Main thread.  Sits on the message queue and waits for something to do.
 */
void *
transport_thread(
	void *vxport)
{
	int 		exit_status = 0, err;
	robo_event_t 	*event;
	xport_state_t 	*transport = (xport_state_t *)vxport;
	int 		is_api = IS_GENERIC_API(transport->library->un->type);
	dev_ent_t 	*un = transport->library->un;

	mutex_lock(&transport->mutex);	/* wait for go */
	mutex_unlock(&transport->mutex);

	for (;;) {
		mutex_lock(&transport->list_mutex);
		if (transport->active_count == 0)
			cond_wait(&transport->list_condit,
			    &transport->list_mutex);

		if (transport->active_count == 0) {	/* check to make sure */
			mutex_unlock(&transport->list_mutex);
			continue;
		}
		event = transport->first;
		transport->first = unlink_list(event);
		transport->active_count--;
		mutex_unlock(&transport->list_mutex);
		ETRACE((LOG_NOTICE, "EvTr %#x(%#x) -",
		    event, (event->type == EVENT_TYPE_MESS) ?
		    event->request.message.command :
		    event->request.internal.command));
		err = 0;

		switch (event->type) {
		case EVENT_TYPE_INTERNAL:
			switch (event->request.internal.command) {
			case ROBOT_INTRL_MOVE_MEDIA:
				if (is_api == TRUE) {
					err = EINVAL;
					break;
				} else {
					if (un->state <= DEV_IDLE) {
						err = move(transport->library,
						    event);
					} else {
						err = EINVAL;
					}
				}
				break;

			case ROBOT_INTRL_EXCH_MEDIA:
				if (is_api == TRUE) {
					err = EINVAL;
					break;
				} else {
					if (un->state <= DEV_IDLE) {
						err = exchange(
						    transport->library, event);
					} else {
						err = EINVAL;
					}
				}
				break;

			case ROBOT_INTRL_INIT:
				init_transport(transport);
				if (is_api == TRUE) {
					disp_of_event(transport->library,
					    event, 0);
				}
				break;

			case ROBOT_INTRL_SHUTDOWN:
				transport->thread = (thread_t)- 1;
				thr_exit(&exit_status);
				break;

			case ROBOT_INTRL_LOAD_MEDIA:
				if (is_api == FALSE) {
					err = EINVAL;
					break;
				}
				event->next = (robo_event_t *)
				    transport->library;
				err = thr_create(NULL, MD_THR_STK,
				    api_load_command,
				    (void *)event, THR_DETACHED, NULL);
				if (err)
					DevLog(DL_ERR(6038),
					    event->request.internal.command,
					    err);
				break;

			case ROBOT_INTRL_FORCE_MEDIA:
				if (is_api == FALSE) {
					err = EINVAL;
					break;
				}
				event->next = (robo_event_t *)
				    transport->library;
				err = thr_create(NULL, MD_THR_STK,
				    api_force_command,
				    (void *)event, THR_DETACHED, NULL);
				if (err)
					DevLog(DL_ERR(6038),
					    event->request.internal.command,
					    err);
				break;

			case ROBOT_INTRL_DISMOUNT_MEDIA:
				if (is_api == FALSE) {
					err = EINVAL;
					break;
				}
				event->next = (robo_event_t *)
				    transport->library;
				err = thr_create(NULL, MD_THR_STK,
				    api_dismount_command,
				    (void *)event, THR_DETACHED, NULL);
				if (err)
					DevLog(DL_ERR(6038),
					    event->request.internal.command,
					    err);
				break;

			case ROBOT_INTRL_VIEW_DATABASE:
				if (is_api == FALSE) {
					err = EINVAL;
					break;
				}
				event->next = (robo_event_t *)
				    transport->library;
				err = thr_create(NULL, MD_THR_STK,
				    api_view_command,
				    (void *)event, THR_DETACHED, NULL);
				if (err)
					DevLog(DL_ERR(6038),
					    event->request.internal.command,
					    err);
				break;

			case ROBOT_INTRL_DRIVE_ACCESS:
				if (is_api == FALSE) {
					err = EINVAL;
					break;
				}
				event->next = (robo_event_t *)
				    transport->library;
				err = thr_create(NULL, MD_THR_STK,
				    api_drive_access_command,
				    (void *)event, THR_DETACHED, NULL);
				if (err)
					DevLog(DL_ERR(6038),
					    event->request.internal.command,
					    err);
				break;

			case ROBOT_INTRL_QUERY_DRIVE:
				if (is_api == FALSE) {
					err = EINVAL;
					break;
				}
				event->next = (robo_event_t *)
				    transport->library;
				err = thr_create(NULL, MD_THR_STK,
				    api_query_drive_command,
				    (void *)event, THR_DETACHED, NULL);
				if (err)
					DevLog(DL_ERR(6038),
					    event->request.internal.command,
					    err);
				break;

			case ROBOT_INTRL_GET_SIDE_INFO:
				if (is_api == FALSE) {
					err = EINVAL;
					break;
				}
				event->next = (robo_event_t *)
				    transport->library;
				err = thr_create(NULL, MD_THR_STK,
				    api_getsideinfo_command,
				    (void *)event, THR_DETACHED, NULL);
				if (err)
					DevLog(DL_ERR(6038),
					    event->request.internal.command,
					    err);
				break;

			default:
				err = EINVAL;
				break;
			}
			break;

		case EVENT_TYPE_MESS:
			if (event->request.message.magic != MESSAGE_MAGIC) {
				if (DBG_LVL(SAM_DBG_DEBUG))
					sam_syslog(LOG_DEBUG,
					    "xpt_thr:bad magic: %s:%d.",
					    __FILE__, __LINE__);
				break;
			}
			switch (event->request.message.command) {
			default:
				if (DBG_LVL(SAM_DBG_DEBUG))
					sam_syslog(LOG_DEBUG,
					    "xpt_thr:msq_bad: %s:%d.",
					    __FILE__, __LINE__);
				err = EINVAL;
				break;
			}

		default:
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "xpt_thr:event_bad: %s:%d.",
				    __FILE__, __LINE__);
			err = EINVAL;
			break;
		}
		if (is_api == FALSE) {
			disp_of_event(transport->library, event, err);
		} else if (err) {
			/* call disp_of_event only if an error on grau */
			if (err < 0)
				err = errno;
			disp_of_event(transport->library, event, err);
		}
	}
}


void
init_transport(xport_state_t *transport)
{
	int clear = 0;
	xport_state_t 	*current = transport;

	if (!(IS_GENERIC_API(transport->library->un->type))) {
		for (; current != (xport_state_t *)0; current = current->next)
			if (clear_transport(current->library, current))
				clear++;

		if (clear) {
			dev_ent_t *un;
			char *lc_mess;

			SANITY_CHECK(transport != (xport_state_t *)0);
			SANITY_CHECK(transport->library != (library_t *)0);
			SANITY_CHECK(transport->library->un != (dev_ent_t *)0);
			un = transport->library->un;
			lc_mess = un->dis_mes[DIS_MES_CRIT];
			memccpy(lc_mess,
			    catgets(catfd, SET, 9077,
			    "needs operator attention"),
			    '\0', DIS_MES_LEN);
			DevLog(DL_ERR(5152));
			DownDevice(un, SAM_STATE_CHANGE);
			exit(1);	/* this kill all threads */
		}
	}
	cond_signal(&transport->condit); /* signal done */
}


/*
 *	find_element
 *
 * Given a drive list and an element, return it's drive entry
 *
 */
drive_state_t *
find_element(
	drive_state_t *drive,
	uint_t element)
{
	while (drive != (drive_state_t *)0) {
		if (drive->element == element)
			return (drive);
		drive = drive->next;
	}
	return ((drive_state_t *)0);
}

void
move_drive_error(
	library_t *library,
	uint_t source,
	uint_t dest,
	int *err)
{
	/* Down the drive(s), not the robot */
	int		downed_one = 0;
	drive_state_t	*drive_to_down;
	dev_ent_t	*un = library->un;
	uchar_t		buf[library->ele_dest_len +
	    sizeof (element_status_data_t) + sizeof (element_status_page_t) +
	    50];
	storage_element_t *desc;

	if (drive_to_down = find_element(library->drive, source)) {
		/* The source is a drive */
		down_drive(drive_to_down, SAM_STATE_CHANGE);
		downed_one++;
	} else {
		/*
		 * The source is not a drive. Read the storage element of the
		 * source. If full then set error to RECOVERED_MEDIA_MOVE. This
		 * tells the requester to set the CES_occupied status bit in the
		 * catalog entry.
		 */
		mutex_unlock(&library->un->io_mutex);
		if (read_element_status(library, STORAGE_ELEMENT, source,
		    1, buf, sizeof (buf)) > 0) {
			desc = (storage_element_t *)(buf +
			    sizeof (element_status_data_t) +
			    sizeof (element_status_page_t));
			if (desc->full)
				*err = RECOVERED_MEDIA_MOVE;
		}
		mutex_lock(&library->un->io_mutex);
	}

	if (drive_to_down = find_element(library->drive, dest)) {
		down_drive(drive_to_down, SAM_STATE_CHANGE);
		downed_one++;
	}
	if (downed_one == 0)
		DevLog(DL_ERR(5333), source, dest, 0);
}

/*
 *	move - move from one element to another
 *
 *	entry -
 *	   library - library_t *
 *	   event - robo_event_t *
 */
int
move(
	library_t *library,
	robo_event_t *event)
{
	dev_ent_t 	*un;
	int 		err = -1, retry, timeout;
	char 		*l_mess = library->un->dis_mes[DIS_MES_NORM];
	char 		*MES_9079 =
	    catgets(catfd, SET, 9079, "move from %s to %s %s");
	char 		*mess, *src_mess, *des_mess, *i_mess;
	robot_internal_t 	*cmd = &event->request.internal;
	int 		added_more_time = FALSE;
	sam_extended_sense_t *sense = (sam_extended_sense_t *)
	    SHM_REF_ADDR(library->un->sense);
	int		movmed_err;

	un = library->un;
	src_mess = element_string(element_type(library, cmd->source));
	des_mess = element_string(element_type(library, cmd->destination1));
	if (cmd->flags.b.invert1)
		i_mess = catgets(catfd, SET, 9084, "invert");
	else
		i_mess = "";

	mess = (char *)malloc_wait(strlen(MES_9079) + strlen(src_mess) +
	    strlen(des_mess) + strlen(i_mess) + 10, 4, 0);
	sprintf(mess, MES_9079, src_mess, des_mess, i_mess);
	memccpy(l_mess, mess, '\0', DIS_MES_LEN);
	free(mess);
	DevLog(DL_DETAIL(5057),
	    cmd->source, cmd->flags.b.invert1 ? "invert" : "asis",
	    cmd->destination1);

	/*
	 * A programming note from DocStore states that you should
	 * allow up to 4 minutes for a move.  This is to allow
	 * for recovery and retry by the robot.	 If other robots need
	 * different time outs, this is the place to put um.
	 */
	switch (library->un->type) {
	case DT_DLT2700:
		timeout = 300;
		break;

	case DT_DOCSTOR:
		timeout = 4 * 60;
		break;

	case DT_METD28:
		/* FALLTHROUGH */
	case DT_METD360:
		/* FALLTHROUGH */
	case DT_SPECLOG:
		/* FALLTHROUGH */
	case DT_ATL1500:
		/* FALLTHROUGH */
	case DT_ODI_NEO:
		/* FALLTHROUGH */
	case DT_QUANTUMC4:
		/* FALLTHROUGH */
	case DT_STK97XX:
		/* FALLTHROUGH */
	case DT_FJNMXX:
		/* FALLTHROUGH */
	case DT_SL3000:
		/* FALLTHROUGH */
	case DT_SLPYTHON:
		timeout = 10 * 60;
		break;

	default:
		timeout = 5 * 60;
		break;
	}

	mutex_lock(&library->un->io_mutex);
	retry = 2;
	do {
		time_t start;

		TAPEALERT(library->open_fd, library->un);
		memset(sense, 0, sizeof (sam_extended_sense_t));
		start = time(NULL);
		movmed_err = scsi_cmd(library->open_fd, library->un,
		    SCMD_MOVE_MEDIUM, timeout,
		    cmd->transport, cmd->source, cmd->destination1,
		    (cmd->flags.b.invert1 ? 1 : 0));
		TAPEALERT(library->open_fd, library->un);
		if (movmed_err < 0) {
			DevLog(DL_TIME(5177), cmd->source,
			    cmd->flags.b.invert1 ? "invert" : "asis",
			    cmd->destination1, time(NULL) - start);

			GENERIC_SCSI_ERROR_PROCESSING(library->un,
			    library->scsi_err_tab, 0,
			    err, added_more_time, retry,
			    /* code for DOWN_EQU */
			    if (!cmd->flags.b.noerror) {
				err = DOWN_EQU;
				if (sense->es_add_code == 0x40 &&
				    sense->es_qual_code == 0x02) {
					move_drive_error(library, cmd->source,
					    cmd->destination1, &err);
				} else {
					down_library(library, SAM_STATE_CHANGE);
				}
				retry = 1;
				/* MACRO for cstyle */}
				break;
				/* MACRO for cstyle */,
				/* code for ILLREQ */
				    retry = 0;
				break;
			/* MACRO for cstyle */,
			    case CLR_TRANS_RET:
			    mutex_unlock(&library->un->io_mutex);
			if (update_element_status(library, TRANSPORT_ELEMENT)) {
				err = DOWN_EQU;
				down_library(library, SAM_STATE_CHANGE);
				retry = 1;
			} else {
				xport_state_t *xports = library->transports;

				if (clear_transport(library, xports) ||
				    ((xports->next != NULL) &&
				    clear_transport(library, xports->next))) {
					err = DOWN_EQU;
					down_library(library, SAM_STATE_CHANGE);
					retry = 1;
				} else
					err = RECOVERED_MEDIA_MOVE;
			}
			mutex_lock(&library->un->io_mutex);
			break;

		case INCOMPATIBLE_MEDIA:
			err = INCOMPATIBLE_MEDIA_MOVE;
			retry = 1;
			break;

		case REZERO:
			rezero_unit(library);
			if (library->un->state == DEV_DOWN)
				retry = 1;
			continue;

		case DOWN_SUB_EQU:
			/* Down the drive(s), not the robot */
			if (!cmd->flags.b.noerror) {
				move_drive_error(library, cmd->source,
				    cmd->destination1, &err);
				retry = 1;
			}
			break;

		case S_ELE_EMPTY:
			/* FALLTHROUGH */
		case D_ELE_FULL:
			/* FALLTHROUGH */
		case NO_MEDIA:
			if ((library->un->type == DT_ACL2640) &&
			    (sense->es_key == 0xb) &&
			    (sense->es_add_code == 0x8c) &&
			    (sense->es_qual_code == 0x7)) {
				memccpy(l_mess, "media incorrectly loaded",
				    '\0', DIS_MES_LEN);
			}
			retry = 1;	/* force while exit */
			break;
			/* MACRO for cstyle */);
		} else {
			DevLog(DL_TIME(5178), cmd->source,
			    cmd->flags.b.invert1 ? "invert" : "asis",
			    cmd->destination1, time(NULL) - start);
			err = 0;
			break;
		}
	} while (--retry > 0);
	if (retry <= 0) {
		DevLog(DL_ERR(5207), cmd->source,
		    cmd->flags.b.invert1 ? "invert" : "asis",
		    cmd->destination1);
		DevLogCdb(un);
		DevLogSense(un);
	}
	/* Metrum claims that a move after an error will clear the alarm */
	if (err != 0 && cmd->flags.b.noerror && library->un->type == DT_METD360)
		rezero_unit(library);

	mutex_unlock(&library->un->io_mutex);
	DevLog(DL_DETAIL(5179), cmd->source,
	    cmd->flags.b.invert1 ? "invert" : "asis", cmd->destination1);
	if (memcmp(l_mess, catgets(catfd, SET, 9079, "move from"), 9) == 0)
		if (!err)
			memccpy(l_mess, catgets(catfd, SET, 9087,
			    "move complete"), '\0', DIS_MES_LEN);
		else
			memccpy(l_mess, catgets(catfd, SET, 9307,
			    "move failed"), '\0', DIS_MES_LEN);
	return (err);
}


/*
 *	exchange - move from one element to another to another
 *
 *	entry -
 *	   library - library_t *
 *	   event - robo_event_t *
 */
int
exchange(
	library_t *library,
	robo_event_t *event)
{
	dev_ent_t 	*un;
	int 		err = 0, retry;
	int 		src_type, dest1_type, dest2_type;
	char 		xcap_byte = (char)0;
	char 		*cap_byte = &xcap_byte;
	char 		*MES_9086 = catgets(catfd, SET, 9086,
	    "exchange from %s to %s %s to %s %s");
	char 		*mess, *src_mess, *des1_mess, *i1_mess, *des2_mess,
	    *i2_mess;
	char 		*l_mess = library->un->dis_mes[DIS_MES_NORM];
	robot_internal_t 	*cmd = &event->request.internal;
	sam_extended_sense_t *sense = (sam_extended_sense_t *)
	    SHM_REF_ADDR(library->un->sense);
	int		exchg_err;

	un = library->un;
	src_mess = element_string(element_type(library, cmd->source));
	des1_mess = element_string(element_type(library, cmd->destination1));
	des2_mess = element_string(element_type(library, cmd->destination2));
	if (cmd->flags.b.invert1)
		i1_mess = catgets(catfd, SET, 9084, "invert");
	else
		i1_mess = "";

	if (cmd->flags.b.invert2)
		i2_mess = catgets(catfd, SET, 9084, "invert");
	else
		i2_mess = "";

	mess = (char *)malloc_wait(strlen(MES_9086) + strlen(src_mess) +
	    strlen(des1_mess) + strlen(des2_mess) +
	    strlen(i1_mess) + strlen(i2_mess) + 10, 4, 0);

	sprintf(mess, MES_9086, src_mess, des1_mess, i1_mess,
	    des2_mess, i2_mess);
	memccpy(l_mess, mess, '\0', DIS_MES_LEN);
	free(mess);

	DevLog(DL_DETAIL(5056),
	    cmd->source, cmd->flags.b.invert1 ? "invert" : "asis",
	    cmd->destination1, cmd->destination2,
	    cmd->flags.b.invert2 ? "invert" : "asis");

	src_type = element_type(library, cmd->source);
	dest1_type = element_type(library, cmd->destination1);
	dest2_type = element_type(library, cmd->destination2);
	if (!(IS_GENERIC_API(library->un->type)))
		cap_byte = (char *)(((char *)library->page1f) + 12 +
		    dest2_type);

	/*
	 * Does this library support an exchange from src -> dest1 -> dest2?
	 * Scsi spec says that dest2 has to be the same type as src for
	 * an exchange
	 */
	if ((src_type == dest2_type) &&
	    (*cap_byte & (1 << (dest1_type - 1))) &&
	    (un->type != DT_QUAL82xx)) {
		int added_more_time = FALSE;
		int timeout;

		/*
		 * A programming note from DosStore states that you should allow
		 * up to 4 minutes for a move.  This is
		 * to allow for recovery and retry by the robot.
		 * If other robots need different time outs,
		 * this is the place to put um.
		 */
		switch (library->un->type) {
		case DT_DOCSTOR:
			timeout = 4 * 60;
			break;

		case DT_METD28:
			/* FALLTHROUGH */
		case DT_METD360:
			timeout = 10 * 60;
			break;

		default:
			timeout = 2 * 60;
			break;
		}

		mutex_lock(&library->un->io_mutex);
		retry = 2;
		do {
			time_t start;

			TAPEALERT(library->open_fd, library->un);
			memset(sense, 0, sizeof (sam_extended_sense_t));
			start = time(NULL);
			exchg_err = scsi_cmd(library->open_fd,
			    library->un, SCMD_EXCHANGE_MEDIUM,
			    timeout, cmd->transport, cmd->source,
			    cmd->destination1, cmd->destination2,
			    (cmd->flags.b.invert1 ? 1 : 0),
			    (cmd->flags.b.invert2 ? 1 : 0));
			TAPEALERT(library->open_fd, library->un);
			if (exchg_err < 0) {
				DevLog(DL_TIME(5180), cmd->source,
				    cmd->flags.b.invert1 ? "invert" : "asis",
				    cmd->destination1, cmd->destination2,
				    cmd->flags.b.invert2 ? "invert" : "asis",
				    time(NULL) - start);

				GENERIC_SCSI_ERROR_PROCESSING(library->un,
				    library->scsi_err_tab, 0,
				    err, added_more_time, retry,
						/* code for DOWN_EQU */
				    if (!cmd->flags.b.noerror) {
					down_library(library, SAM_STATE_CHANGE);
					retry = 1;
					/* MACRO for cstyle */}
					break;
					/* MACRO for cstyle */,
					/* code for ILLREQ */
					    break;
					/* MACRO for cstyle */,
					    case CLR_TRANS_RET:
					    mutex_unlock(
					    &library->un->io_mutex);
				if (update_element_status(library,
				    TRANSPORT_ELEMENT)) {
					err = DOWN_EQU;
					retry = 1;
				} else {
					xport_state_t *xports =
					    library->transports;
					if (clear_transport(library, xports) ||
					    ((xports->next != NULL) &&
					    clear_transport(library,
					    xports->next))) {
						err = DOWN_EQU;
						down_library(library,
						    SAM_STATE_CHANGE);
						retry = 1;
					} else
						err = RECOVERED_MEDIA_MOVE;
				}
				mutex_lock(&library->un->io_mutex);
				break;

			case REZERO:
				rezero_unit(library);
				if (library->un->state == DEV_DOWN)
					retry = 1;
				continue;


			case DOWN_SUB_EQU:
				if (!cmd->flags.b.noerror) {
					int downed_one = 0;
					drive_state_t *drive_to_down =
					    find_element(library->drive,
					    cmd->source);

					if (drive_to_down !=
					    (drive_state_t *)0) {
						down_drive(drive_to_down,
						    SAM_STATE_CHANGE);
						downed_one++;
					}
					drive_to_down =
					    find_element(library->drive,
					    cmd->destination1);
					if (drive_to_down !=
					    (drive_state_t *)0) {
						down_drive(drive_to_down,
						    SAM_STATE_CHANGE);
						downed_one++;
					}
					drive_to_down =
					    find_element(library->drive,
					    cmd->destination2);
					if (drive_to_down !=
					    (drive_state_t *)0) {
						down_drive(drive_to_down,
						    SAM_STATE_CHANGE);
						downed_one++;
					}
					if (downed_one == 0)
						DevLog(DL_ERR(5333),
						    cmd->source,
						    cmd->destination1,
						    cmd->destination2);
					retry = 1;
				}
				break;

		case S_ELE_EMPTY:
		case D_ELE_FULL:
		case NO_MEDIA:
				retry = 1;
				break;
		/* MACRO for cstyle */)
			} else {
				DevLog(DL_TIME(5198), cmd->source,
				    cmd->flags.b.invert1 ? "invert" : "asis",
				    cmd->destination1, cmd->destination2,
				    cmd->flags.b.invert2 ? "invert" : "asis",
				    time(NULL) - start);
				err = 0;
				break;
			}
		} while (--retry > 0);

		if (retry <= 0) {
			DevLog(DL_ERR(5208), cmd->source,
			    cmd->flags.b.invert1 ? "invert" : "asis",
			    cmd->destination1, cmd->destination2,
			    cmd->flags.b.invert2 ? "invert" : "asis");
			DevLogCdb(un);
			DevLogSense(un);
		}
		if (err != 0 && cmd->flags.b.noerror &&
		    library->un->type == DT_METD360)
			rezero_unit(library);

		mutex_unlock(&library->un->io_mutex);
	} else {
		robo_event_t move_event;
		robot_internal_t *move_cmd;

		move_cmd = &move_event.request.internal;
		memset(&move_event, 0, sizeof (robo_event_t));

		/*
		 * Must do the exchange as a sequence of two moves.
		 * Do dest1 -> dest2 first
		 */
		move_cmd->transport = 0; /* use default transport */
		move_cmd->source = cmd->destination1;
		move_cmd->destination1 = cmd->destination2;
		move_cmd->flags.b.invert1 = cmd->flags.b.invert2;
		move_cmd->flags.b.noerror = cmd->flags.b.noerror;
		err = move(library, &move_event);

		if (!err) {
			/* use default transport */
			move_cmd->transport = 0;
			move_cmd->source = cmd->source;
			move_cmd->destination1 = cmd->destination1;
			move_cmd->flags.b.invert1 = cmd->flags.b.invert1;
			move_cmd->flags.b.noerror = cmd->flags.b.noerror;
			err = move(library, &move_event);
			if (err) {
				/*
				 * The second move failed. If there is a failure
				 * in exchange() SAM expects things to be as
				 * they were when we started.
				 * So, undo "Do dest1 -> dest2 first".
				 * NOTE: we are ignoring any error on this move
				 * and hoping for the best; the error from the
				 * second move has to be preserved for return
				 * to the caller.
				 */
				move_cmd->transport = 0;
				move_cmd->source = cmd->destination2;
				move_cmd->destination1 = cmd->destination1;
				move_cmd->flags.b.invert1 =
				    cmd->flags.b.invert2;
				move_cmd->flags.b.noerror =
				    cmd->flags.b.noerror;
				move(library, &move_event);
			}
		}
	}

	if (memcmp(l_mess, catgets(catfd, SET, 9086, "exchange from"), 13) == 0)
		memccpy(l_mess, catgets(catfd, SET, 9088, "exchange complete"),
		    '\0', DIS_MES_LEN);
	return (err);
}


/*
 *	element_type - return type of the element address
 *
 * entry -
 *		library - library_t *
 *		element - element to determine
 *
 * return -
 *		element type(1 - 4)
 *		0 - if unknown element type
 */
int
element_type(
	library_t *library,
	uint_t element)
{

	/* order of checks based on most lookups being for drives or storage */
	if (library->range.drives_count &&
	    (element >= library->range.drives_lower) &&
	    (element <= library->range.drives_upper))
		return (DATA_TRANSFER_ELEMENT);

	if (library->range.storage_count &&
	    (element >= library->range.storage_lower) &&
	    (element <= library->range.storage_upper))
		return (STORAGE_ELEMENT);

	if (library->range.ie_count &&
	    (element >= library->range.ie_lower) &&
	    (element <= library->range.ie_upper))
		return (IMPORT_EXPORT_ELEMENT);

	if (library->range.transport_count &&
	    (element >= library->range.transport_lower) &&
	    (element <= library->range.transport_upper))
		return (TRANSPORT_ELEMENT);

	return (0);
}


/*
 *	rezero_unit - issue rezero command to library.
 * io_mutex held on entry.
 */
void
rezero_unit(
	library_t *library)
{
	int 	retry, err, cmd_err;
	int 	added_more_time = FALSE;
	dev_ent_t *un = library->un;	/* Needed by DL_ERR */

	sam_extended_sense_t *sense = (sam_extended_sense_t *)
	    SHM_REF_ADDR(library->un->sense);


	retry = 2;
	do {
		TAPEALERT(library->open_fd, library->un);
		memset(sense, 0, sizeof (sam_extended_sense_t));
		switch (un->type) {
		case DT_DOCSTOR:
			cmd_err = scsi_cmd(library->open_fd, un,
			    SCMD_REZERO_UNIT, 0);
			break;

		default:
			cmd_err = scsi_cmd(library->open_fd, un,
			    SCMD_POSITION_TO_ELEMENT, 0, 0,
			    library->range.storage_upper, 0);
			break;
		}

		if (cmd_err < 0) {
			TAPEALERT_SKEY(library->open_fd, library->un);
			GENERIC_SCSI_ERROR_PROCESSING(un,
			    library->scsi_err_tab, 0,
			    err, added_more_time, retry,
					/* code for DOWN_EQU */
			    down_library(library, SAM_STATE_CHANGE);
					return;
					/* MACRO for cstyle */,
					/* code for ILLREQ */
					    return;
					/* MACRO for cstyle */,
					/* More codes */
					    /* MACRO for cstyle */;
					/* MACRO for cstyle */)
		} else {
			err = 0;
			break;
		}
	} while (--retry > 0);
	if (retry <= 0) {
		DevLog(DL_ERR(5209));
		DevLogCdb(un);
		DevLogSense(un);
	}
}


/*
 *	element_string - given an element type return a displayable string
 * for that type.
 */
char *
element_string(
	int element_type)
{
	switch (element_type) {
		case STORAGE_ELEMENT:
		return (catgets(catfd, SET, 9080, "storage"));

	case DATA_TRANSFER_ELEMENT:
		return (catgets(catfd, SET, 9081, "drive"));

	case IMPORT_EXPORT_ELEMENT:
		return (catgets(catfd, SET, 9082, "import/export"));

	case TRANSPORT_ELEMENT:
		return (catgets(catfd, SET, 9083, "transport"));

	default:
		return (catgets(catfd, SET, 9085, "unknown"));
	}
}


/*
 *	  GRAU
 */


/*	some globals */
extern char *debug_device_name;		/* From init.c */


/*
 *	api_load_command - load volser into drive
 *
 */
void *
api_load_command(
	void *vevent)
{
#if !defined(SAM_OPEN_SOURCE)
	int 		sequ_no;
	robo_event_t 	*event = vevent;
	library_t	*library = (library_t *)event->next;
	char 		*l_mess = library->un->dis_mes[DIS_MES_NORM];
	char 		*ent_pnt = "api_load_command";
	char 		media[10];

	aci_information_t *aci_info =
	    (aci_information_t *)(event->request.internal.address);

	mutex_lock(&library->transports->mutex);
	sequ_no = ++(library->transports->sequence);
	mutex_unlock(&library->transports->mutex);
	sprintf(l_mess, "%s -> %s", aci_info->aci_vol_desc->volser,
	    aci_info->aci_drive_entry->drive_name);
	memset(media, 0, sizeof (media));
	sprintf(media, "%d", aci_info->aci_vol_desc->type);
	if (DBG_LVL(SAM_DBG_LOAD))
		sam_syslog(LOG_DEBUG, "%s: Sequence %d: %s into %s.",
		    ent_pnt, sequ_no,
		    aci_info->aci_vol_desc->volser,
		    aci_info->aci_drive_entry->drive_name);
	api_start_request(library, "mount", sequ_no, event, 3,
	    aci_info->aci_vol_desc->volser, media,
	    aci_info->aci_drive_entry->drive_name);
#endif
	thr_exit(NULL);
}


/*
 *	api_force_command - force unload a drive
 *
 */
void *
api_force_command(
	void *vevent)
{
#if !defined(SAM_OPEN_SOURCE)
	int 		sequ_no;
	robo_event_t 	*event = vevent;
	library_t	*library = (library_t *)event->next;
	char 		*l_mess = library->un->dis_mes[DIS_MES_NORM];
	char 		*ent_pnt = "api_force_command";
	aci_information_t *aci_info =
	    (aci_information_t *)(event->request.internal.address);

	mutex_lock(&library->transports->mutex);
	sequ_no = ++(library->transports->sequence);
	mutex_unlock(&library->transports->mutex);
	sprintf(l_mess, "force %s", aci_info->aci_drive_entry->drive_name);
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "%s: Sequence %d: %s.", ent_pnt,
		    sequ_no, aci_info->aci_drive_entry->drive_name);

	api_start_request(library, "force", sequ_no, event, 1,
	    aci_info->aci_drive_entry->drive_name);
#endif
	thr_exit(NULL);
}


/*
 *	api_drive_access_command - send access information to the AMU
 */
void *
api_drive_access_command(
	void *vevent)
{
#if !defined(SAM_OPEN_SOURCE)
	int 		sequ_no;
	robo_event_t *event = vevent;
	library_t 	*library = (library_t *)event->next;
	char 		*ent_pnt = "api_drive_access_command";
	aci_information_t *aci_info =
	    (aci_information_t *)(event->request.internal.address);

	if (DBG_LVL(SAM_DBG_EVENT))
		sam_syslog(LOG_DEBUG, "EV: TRE: drive access: %#x:%#x.",
		    event, aci_info);
	mutex_lock(&library->transports->mutex);
	sequ_no = ++(library->transports->sequence);
	mutex_unlock(&library->transports->mutex);
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "%s: Sequence %d: %s.", ent_pnt, sequ_no,
		    aci_info->aci_drive_entry->drive_name);
	api_start_request(library, "driveaccess", sequ_no, event, 2,
	    library->api_client, aci_info->aci_drive_entry->drive_name);
#endif
	thr_exit(NULL);
}


/*
 *	api_query_drive_command - send access information to the AMU
 */
void *
api_query_drive_command(
	void *vevent)
{
#if !defined(SAM_OPEN_SOURCE)
	int 		sequ_no;
	robo_event_t	*event = vevent;
	library_t 	*library = (library_t *)event->next;
	char 		*ent_pnt = "api_query_drive_command";
	aci_information_t *aci_info =
	    (aci_information_t *)(event->request.internal.address);

	if (DBG_LVL(SAM_DBG_EVENT))
		sam_syslog(LOG_DEBUG, "EV: TRE: query drive: %#x:%#x.",
		    event, aci_info);
	mutex_lock(&library->transports->mutex);
	sequ_no = ++(library->transports->sequence);
	mutex_unlock(&library->transports->mutex);
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "%s: Sequence %d: %s.", ent_pnt, sequ_no,
		    aci_info->aci_drive_entry->drive_name);
	api_start_request(library, "querydrive", sequ_no, event, 2,
	    library->api_client, aci_info->aci_drive_entry->drive_name);
#endif
	thr_exit(NULL);
}


/*
 *	api_dismount_command - dismount volser
 */
void *
api_dismount_command(
	void *vevent)
{
#if !defined(SAM_OPEN_SOURCE)
	int 		sequ_no;
	robo_event_t	*event = vevent;
	library_t 	*library = (library_t *)event->next;
	char 		*l_mess = library->un->dis_mes[DIS_MES_NORM];
	char 		*ent_pnt = "api_dismount_command";
	char 		media[10];
	aci_information_t *aci_info =
	    (aci_information_t *)(event->request.internal.address);

	mutex_lock(&library->transports->mutex);
	sequ_no = ++(library->transports->sequence);
	mutex_unlock(&library->transports->mutex);
	sprintf(l_mess, "%s <- %s", aci_info->aci_drive_entry->volser,
	    aci_info->aci_drive_entry->drive_name);
	memset(media, 0, sizeof (media));
	sprintf(media, "%d", aci_info->aci_drive_entry->type);

	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "%s: Sequence %d; %s from %s.",
		    ent_pnt, sequ_no,
		    aci_info->aci_drive_entry->volser,
		    aci_info->aci_drive_entry->drive_name);

	api_start_request(library, "dismount", sequ_no, event, 2,
	    aci_info->aci_drive_entry->volser, media);
#endif
	thr_exit(NULL);
}


/*
 *	api_view_command - view database entry
 */
void *
api_view_command(
	void *vevent)
{
#if !defined(SAM_OPEN_SOURCE)
	int 		sequ_no;
	robo_event_t	*event = vevent;
	library_t 	*library = (library_t *)event->next;
	char 		*ent_pnt = "api_view_command";
	char 		vol_ser[ACI_VOLSER_LEN];
	char 		media[10];
	aci_vol_desc_t *volume_info =
	    (aci_vol_desc_t *)(event->request.internal.address);

	mutex_lock(&library->transports->mutex);
	sequ_no = ++(library->transports->sequence);
	mutex_unlock(&library->transports->mutex);
	memset(media, 0, sizeof (media));
	sprintf(media, "%d", volume_info->type);

	strcpy(vol_ser, volume_info->volser);
	memset(media, 0, sizeof (media));
	sprintf(media, "%d", volume_info->type);
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "%s: Sequence: %d: %s.", ent_pnt,
		    sequ_no, volume_info->volser);

	api_start_request(library, "view", sequ_no, event, 3,
	    volume_info->volser, media, library->api_client);
#endif
	thr_exit(NULL);
}


void *
api_getsideinfo_command(
	void *vevent)
{
#if !defined(SAM_OPEN_SOURCE)
	int 		sequ_no;
	robo_event_t	*event = vevent;
	library_t 	*library = (library_t *)event->next;
	char 		*l_mess = library->un->dis_mes[DIS_MES_NORM];
	char 		*ent_pnt = "api_getsideinfo_command";
	aci_vol_desc_t	*volume_info =
	    (aci_vol_desc_t *)(event->request.internal.address);

	mutex_lock(&library->transports->mutex);
	sequ_no = ++(library->transports->sequence);
	mutex_unlock(&library->transports->mutex);
	sprintf(l_mess, "%s ", volume_info->volser);

	if (DBG_LVL(SAM_DBG_DEBUG))
		syslog(LOG_DEBUG, "%s: Sequence: %d: %s.", ent_pnt, sequ_no,
		    volume_info->volser);

	api_start_request(library, "getside", sequ_no, event, 2,
	    library->api_client, volume_info->volser);
#endif
	thr_exit(NULL);
}


	/* VARARGS4 */
void
api_start_request(
	library_t *library,
	char *cmd,
	int sequ,
	robo_event_t *event,
	int cnt, ...)
{
	int 	i;
	char 	*ent_pnt = "api_start_request";
	equ_t 	eq = library->eq;
	char 	**args, **args2, *messg;
	char 	chr_shmid[12], chr_seq[12], chr_event[12], chr_equ[12];
	api_priv_mess_t *priv_message = library->help_msg;

	va_list aps;

	messg = malloc_wait(512, 2, 0);
	sprintf(chr_shmid, "%d", master_shm.shmid);
	sprintf(chr_seq, "%d", sequ);
	sprintf(chr_event, "%#x", event);
	sprintf(chr_equ, "%d", eq);
	args = (char **)malloc_wait((cnt + 7) * sizeof (char *), 2, 0);
	args2 = args;

	*args2++ = cmd;
	*args2++ = debug_device_name;
	*args2++ = chr_shmid;
	*args2++ = chr_seq;
	*args2++ = chr_equ;
	*args2++ = chr_event;
	va_start(aps, cnt);
	for (i = 1; i <= cnt; i++)
		*args2++ = va_arg(aps, char *);

	va_end(aps);
	*args2 = NULL;
	memset(messg, '\0', 512);

	sprintf(messg, "%s/%s", SAM_EXECUTE_PATH, library->api_helper_name);
	for (args2 = args; *args2 != NULL; args2++)
		sprintf(&messg[strlen(messg)], ", %s", *args2);

	if (DBG_LVL(SAM_DBG_DEBUG)) {
		if (DBG_LVL_EQ(SAM_DBG_EVENT | SAM_DBG_TMESG))
			sam_syslog(LOG_DEBUG, "%s: %s: Sequence %d:%s.",
			    ent_pnt, args[0], sequ, messg);
		else
			sam_syslog(LOG_DEBUG, "%s: %s: Sequence %d.",
			    ent_pnt, args[0], sequ);
	}
	mutex_lock(&priv_message->mutex);
	while (priv_message->mtype != API_PRIV_VOID)
		cond_wait(&priv_message->cond_i, &priv_message->mutex);

	memccpy(priv_message->message, messg, '\0', 512);
	priv_message->mtype = API_PRIV_NORMAL;

	cond_signal(&priv_message->cond_r);
	mutex_unlock(&priv_message->mutex);

	free(messg);
	free(args);
}
