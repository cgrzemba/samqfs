/*
 * transport.c - thread that watches over a transport element
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
#include <stdarg.h>
#include <synch.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "driver/samst_def.h"
#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/mode_sense.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "aml/trace.h"
#include "sam/nl_samfs.h"
#include "sony.h"
#include "aml/proto.h"
#include "sam/lib.h"

#pragma ident "$Revision: 1.20 $"


/* function prototypes */

void init_transport(xport_state_t *);
void *force(void *);
void *dismount(void *);
void *view(void *);
void *query_drv(void *);
void *load(void *);
void start_request(library_t *, char *, int, robo_event_t *, int, ...);

/* some globals */

extern shm_alloc_t master_shm, preview_shm;

/*
 * Main thread.  Sits on the message queue and waits for something to do.
 *
 * The transport thread for the sony will start a thread for each request
 * directed at the sony.  This thread will fork the sony_helper process
 * and then wait for child termination.  The helper will report its
 * status back through the robots message queue.  If an error occurs
 * during the fork, the thread will dispose of the event with an error.
 *
 * The helper is started with the following fixed arguments:
 * command  -  The command
 * sequence -  The sequence number
 * eq       -  The equipment number of the robot
 * event    -  Address of the event (for return in the response only).
 */

void *
transport_thread(void *vxport)
{
	int			err;
	robo_event_t		*event;
	xport_state_t 		*transport = (xport_state_t *)vxport;
	struct sigaction	sig_action;
	sigset_t		signal_set, full_block_set;

	sigfillset(&full_block_set);
	sigemptyset(&signal_set);	/* signals to except. */
	sigaddset(&signal_set, SIGCHLD);

	mutex_lock(&transport->mutex);	/* wait for go  */
	mutex_unlock(&transport->mutex);

	thr_sigsetmask(SIG_SETMASK, &full_block_set, NULL);
	memset(&sig_action, 0, sizeof (struct sigaction));
	(void) sigemptyset(&sig_action.sa_mask);
	sig_action.sa_flags = SA_RESTART;
	sig_action.sa_handler = SIG_DFL;
	(void) sigaction(SIGCHLD, &sig_action, NULL);

	for (;;) {
		mutex_lock(&transport->list_mutex);

		/*
		 * Wait for an event
		 */
		while (transport->active_count == 0)
			cond_wait(&transport->list_condit,
			    &transport->list_mutex);

		event = transport->first;
		transport->first = unlink_list(event);
		transport->active_count--;
		mutex_unlock(&transport->list_mutex);
		ETRACE((LOG_NOTICE, "EvTr %#x(%#x)",
		    event, (event->type == EVENT_TYPE_MESS) ?
		    event->request.message.command :
		    event->request.internal.command));
		event->next = NULL;
		err = 0;

		switch (event->type) {

			case EVENT_TYPE_INTERNAL:

				switch (event->request.internal.command) {
					case ROBOT_INTRL_LOAD_MEDIA:
						if (transport->library->
						    un->state <= DEV_IDLE) {
							event->next =
							    (robo_event_t *)
							    transport->
							    library;
							err = thr_create(NULL,
							    MD_THR_STK,
							    load,
							    (void *)event,
							    THR_DETACHED,
							    NULL);
						} else {
							err = EINVAL;
						}
						break;

					case ROBOT_INTRL_FORCE_MEDIA:
						event->next = (robo_event_t *)
						    transport->library;
						err = thr_create(NULL,
						    MD_THR_STK, force,
						    (void *)event,
						    THR_DETACHED, NULL);
						break;

					case ROBOT_INTRL_DISMOUNT_MEDIA:
						event->next = (robo_event_t *)
						    transport->library;
						err = thr_create(NULL,
						    MD_THR_STK, dismount,
						    (void *)event,
						    THR_DETACHED, NULL);
						break;

					case ROBOT_INTRL_INIT:
						init_transport(transport);
						break;

					case ROBOT_INTRL_VIEW_DATABASE:
						event->next = (robo_event_t *)
						    transport->library;
						err = thr_create(NULL,
						    MD_THR_STK, view,
						    (void *)event,
						    THR_DETACHED, NULL);
						break;

					case ROBOT_INTRL_QUERY_DRIVE:
						event->next = (robo_event_t *)
						    transport->library;
						err = thr_create(NULL,
						    MD_THR_STK, query_drv,
						    (void *)event,
						    THR_DETACHED, NULL);
						break;

					case ROBOT_INTRL_SHUTDOWN:
						transport->thread =
						    (thread_t)-1;
						thr_exit((void *)NULL);
						break;

					default:
						err = EINVAL;
						break;
				}
				break;

			case EVENT_TYPE_MESS:
				if (event->request.message.magic
				    != MESSAGE_MAGIC) {

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
							"xpt_thr:msq_bad:"
							" %s:%d.",
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

		/*
		 * Only dispose of event in error cases
		 */
		if (err) {
			if (err < 0) err = errno;
			disp_of_event(transport->library, event, err);
		}

	}
}

void
init_transport(xport_state_t *transport)
{
	cond_signal(&transport->condit);    /* signal done */
}


/*
 * load - load volser into drive
 */
void *
load(void  *vevent)
{
	int		i;
	int		sequ_no;
	char		CassetteId[PSCCASSIDLEN+1];
	robo_event_t	*event = vevent;
	library_t	*library = (library_t *)event->next;
	char  		mess[DIS_MES_LEN * 2];

	sony_information_t *sony_info =
	    (sony_information_t *)event->request.internal.address;

	/*
	 * Spaces are allowed in Sony barcodes. The helper code
	 * doesn't handle this well. Subsititue _ for the spaces
	 * in order to pass the correct number of parameters.
	 */
	strcpy(CassetteId, sony_info->PscCassInfo->szCassetteId);
	for (i = 0; i <= PSCCASSIDLEN; i++) {
		if (CassetteId[i] == ' ') CassetteId[i] = '_';
	}

	mutex_lock(&library->transports->mutex);
	sequ_no = ++(library->transports->sequence);
	mutex_unlock(&library->transports->mutex);
	sprintf(mess, "mount(%d) volume %s, drive %s", sequ_no,
	    CassetteId, sony_info->BinNoChar);
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "%s.", mess);
	memccpy(library->un->dis_mes[DIS_MES_NORM], mess, '\0', DIS_MES_LEN);

	start_request(library, "mount", sequ_no, event, 4,
	    library->sony_servername, library->sony_userid,
	    CassetteId, sony_info->BinNoChar);

	thr_exit(NULL);
}


/*
 * force - force unload a drive
 */
void *
force(void  *vevent)
{
	int		sequ_no;
	robo_event_t	*event = vevent;
	library_t	*library = (library_t *)event->next;
	char		mess[DIS_MES_LEN * 2];

	sony_information_t *sony_info =
	    (sony_information_t *)event->request.internal.address;

	mutex_lock(&library->transports->mutex);
	sequ_no = ++(library->transports->sequence);
	mutex_unlock(&library->transports->mutex);
	sprintf(mess, "sony_force(%d) %s", sequ_no, sony_info->BinNoChar);
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "%s.", mess);
	memccpy(library->un->dis_mes[DIS_MES_NORM], mess, '\0', DIS_MES_LEN);

	start_request(library, "force", sequ_no, event, 3,
	    library->sony_servername, library->sony_userid,
	    sony_info->BinNoChar);

	thr_exit(NULL);
}


/*
 * dismount - dismount volser
 */
void *
dismount(void *vevent)
{
	int		sequ_no;
	robo_event_t	*event = vevent;
	library_t	*library = (library_t *)event->next;
	char		mess[DIS_MES_LEN * 2];

	sony_information_t *sony_info =
	    (sony_information_t *)event->request.internal.address;

	mutex_lock(&library->transports->mutex);
	sequ_no = ++(library->transports->sequence);
	mutex_unlock(&library->transports->mutex);
	sprintf(mess, "dismount(%d) Drive Bin Number %s, ", sequ_no,
	    sony_info->BinNoChar);
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "%s.", mess);
	memccpy(library->un->dis_mes[DIS_MES_NORM], mess, '\0', DIS_MES_LEN);

	start_request(library, "dismount", sequ_no, event, 3,
	    library->sony_servername, library->sony_userid,
	    sony_info->BinNoChar);

	thr_exit(NULL);
}


/*
 * view - view database entry
 */
void *
view(void *vevent)
{
	int		i;
	int		sequ_no;
	robo_event_t	*event = vevent;
	library_t	*library = (library_t *)event->next;
	char		mess[DIS_MES_LEN * 2];
	char		CassetteId[PSCCASSIDLEN+1];

	sony_information_t *sony_info =
	    (sony_information_t *)(event->request.internal.address);

	/*
	 * Spaces are allowed in Sony barcodes. The helper code
	 * doesn't handle this well. Subsititue _ for the spaces
	 * in order to pass the correct number of parameters.
	 */
	strcpy(CassetteId, sony_info->PscCassInfo->szCassetteId);
	for (i = 0; i <= PSCCASSIDLEN; i++) {
		if (CassetteId[i] == ' ') CassetteId[i] = '_';
	}

	mutex_lock(&library->transports->mutex);
	sequ_no = ++(library->transports->sequence);
	mutex_unlock(&library->transports->mutex);
	sprintf(mess, "view(%d) volser %s", sequ_no, CassetteId);
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "%s.", mess);
	memccpy(library->un->dis_mes[DIS_MES_NORM], mess, '\0', DIS_MES_LEN);

	start_request(library, "view", sequ_no, event, 3,
	    library->sony_servername, library->sony_userid, CassetteId);
	thr_exit(NULL);
}


/*
 * query_drv - query a drive
 */
void *
query_drv(void *vevent)
{
	int		sequ_no;
	robo_event_t	*event = vevent;
	library_t	*library = (library_t *)event->next;
	char		mess[DIS_MES_LEN * 2];

	sony_information_t *sony_info =
	    (sony_information_t *)event->request.internal.address;

	mutex_lock(&library->transports->mutex);
	sequ_no = ++(library->transports->sequence);
	mutex_unlock(&library->transports->mutex);
	sprintf(mess, "sony_query_drv(%d), drive(%s).", sequ_no,
	    sony_info->BinNoChar);
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "%s.", mess);
	memccpy(library->un->dis_mes[DIS_MES_NORM], mess, '\0', DIS_MES_LEN);

	start_request(library, "query_drv", sequ_no, event, 3,
	    library->sony_servername, library->sony_userid,
	    sony_info->BinNoChar);

	thr_exit(NULL);
}


	/* VARARGS4 */
void
start_request(library_t *library, char *cmd, int sequ, robo_event_t *event,
    int cnt, ...)
{
	int	i;
	equ_t	eq = library->eq;
	char 	**args, **args2, *messg;
	char 	chr_shmid[12], chr_seq[12], chr_event[12], chr_equ[12];
	va_list aps;
	sony_priv_mess_t *priv_message = library->help_msg;

	messg = malloc_wait(512, 2, 0);
	sprintf(chr_shmid, "%d", master_shm.shmid);
	sprintf(chr_seq, "%#x", sequ);
	sprintf(chr_event, "%#x", event);
	sprintf(chr_equ, "%d", eq);
	args = (char **)malloc((cnt+6) * sizeof (char *));
	args2 = args;

	*args2++ = cmd;
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

	sprintf(messg, "%s/sam-sony_helper", SAM_EXECUTE_PATH);
	for (args2 = args; *args2 != NULL; args2++)
		sprintf(&messg[strlen(messg)], ", %s", *args2);

	if (DBG_LVL(SAM_DBG_DEBUG)) {
		if (DBG_LVL_EQ(SAM_DBG_EVENT | SAM_DBG_TMESG))
			sam_syslog(LOG_DEBUG, "start helper:%s(%d)%s.",
			    args[0], sequ, messg);
		else
			sam_syslog(LOG_DEBUG, "start helper:%s(%d).",
			    args[0], sequ);
	}

	mutex_lock(&priv_message->mutex);
	while (priv_message->mtype != SONY_PRIV_VOID)
		cond_wait(&priv_message->cond_i, &priv_message->mutex);

	memccpy(priv_message->message, messg, '\0', 512);
	priv_message->mtype = SONY_PRIV_NORMAL;
	cond_signal(&priv_message->cond_r);
	mutex_unlock(&priv_message->mutex);

	free(messg);
	free(args);
}
