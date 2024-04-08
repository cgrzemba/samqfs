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
#include <stdarg.h>
#include <synch.h>
#include <stdio.h>
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
#include "ibm3494.h"
#include "aml/proto.h"
#include "sam/lib.h"

#pragma ident "$Revision: 1.21 $"

/*	function prototypes */
void init_transport(xport_state_t *);
void force(library_t *, robo_event_t *);
void dismount(library_t *, robo_event_t *);
void view(library_t *, robo_event_t *);
void query_drv(library_t *, robo_event_t *);
void query_lib(library_t *, robo_event_t *);
void load(library_t *, robo_event_t *);
void set_category(library_t *, robo_event_t *);

/* some globals */
char *l_mess, *lc_mess;
extern shm_alloc_t master_shm, preview_shm;


/*
 *	Main thread.  Sits on the message queue and waits for something to do.
 *
 * The transport thread for the ibm will issue a delayed request for
 * requests supporting delayed requests.  Otherwise will issue the request
 * and wait for response.
 */
void *
transport_thread(
	void *vxport)
{
	robo_event_t 	*event;
	xport_state_t 	*transport = (xport_state_t *)vxport;
	struct sigaction sig_action;
	sigset_t signal_set, full_block_set;

	sigfillset(&full_block_set);
	sigemptyset(&signal_set);	/* signals to except. */
	sigaddset(&signal_set, SIGCHLD);

	mutex_lock(&transport->mutex);	/* wait for go */
	mutex_unlock(&transport->mutex);
	l_mess = transport->library->un->dis_mes[DIS_MES_NORM];
	lc_mess = transport->library->un->dis_mes[DIS_MES_CRIT];
	thr_sigsetmask(SIG_SETMASK, &full_block_set, NULL);
	memset(&sig_action, 0, sizeof (struct sigaction));
	(void) sigemptyset(&sig_action.sa_mask);
	sig_action.sa_flags = SA_RESTART;
	sig_action.sa_handler = SIG_DFL;
	(void) sigaction(SIGCHLD, &sig_action, NULL);
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
		ETRACE((LOG_NOTICE, "EvTr %#x(%#x) - \n",
		    event, (event->type == EVENT_TYPE_MESS) ?
		    event->request.message.command :
		    event->request.internal.command));
		event->next = NULL;

		/* Everyone must take care of disposing of the event */
		switch (event->type) {
		case EVENT_TYPE_INTERNAL:
			switch (event->request.internal.command) {
			case ROBOT_INTRL_LOAD_MEDIA:
				if (transport->library->un->state <= DEV_IDLE) {
					load(transport->library, event);
				} else {
					disp_of_event(transport->library,
					    event, EINVAL);
				}
				break;

			case ROBOT_INTRL_FORCE_MEDIA:
				force(transport->library, event);
				break;

			case ROBOT_INTRL_DISMOUNT_MEDIA:
				dismount(transport->library, event);
				break;

			case ROBOT_INTRL_INIT:
				init_transport(transport);
				disp_of_event(transport->library, event, 0);
				break;

			case ROBOT_INTRL_VIEW_DATABASE:
				view(transport->library, event);
				break;

			case ROBOT_INTRL_QUERY_DRIVE:
				query_drv(transport->library, event);
				break;

			case ROBOT_INTRL_QUERY_LIBRARY:
				query_lib(transport->library, event);
				break;

			case ROBOT_INTRL_SET_CATEGORY:
				set_category(transport->library, event);
				break;

			case ROBOT_INTRL_SHUTDOWN:
				transport->thread = (thread_t)- 1;
				thr_exit((void *)NULL);
				break;

			default:
				disp_of_event(transport->library, event,
				    EINVAL);
				break;
			}
			break;

		case EVENT_TYPE_MESS:
			if (event->request.message.magic != MESSAGE_MAGIC) {
				if (DBG_LVL(SAM_DBG_DEBUG))
					sam_syslog(LOG_DEBUG,
					    "xpt_thr:bad magic: %s:%d.\n",
					    __FILE__, __LINE__);

				disp_of_event(transport->library, event,
				    EINVAL);
				break;
			}
			switch (event->request.message.command) {
			default:
				if (DBG_LVL(SAM_DBG_DEBUG))
					sam_syslog(LOG_DEBUG,
					    "xpt_thr:msq_bad: %s:%d.\n",
					    __FILE__, __LINE__);
				disp_of_event(transport->library, event,
				    EINVAL);
				break;
			}

		default:
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "xpt_thr:event_bad: %s:%d.\n",
				    __FILE__, __LINE__);
			disp_of_event(transport->library, event, EINVAL);
			break;
		}
	}
}


void
init_transport(
	xport_state_t *transport)
{
	cond_signal(&transport->condit); /* signal done */
}


/*
 *	load - load volser into drive
 *
 */
void
load(
	library_t *library,
	robo_event_t *event)
{
	IBM_mount_t 		mount_req;
	delay_list_ent_t 	*dly_ent;
	ibm_req_info_t 		*ibm_info =
	    (ibm_req_info_t *)event->request.internal.address;

	dly_ent =
	    (delay_list_ent_t *)malloc_wait(sizeof (delay_list_ent_t), 2, 0);
	memset(dly_ent, 0, sizeof (delay_list_ent_t));
	dly_ent->event = event;
	memset(&mount_req, 0, sizeof (IBM_mount_t));
	mount_req.device = ibm_info->drive_id;
	mount_req.target_cat = ibm_info->targ_cat;
	mount_req.source_cat = ibm_info->src_cat;
	memcpy(mount_req.volser, ibm_info->volser, 8);

	mutex_lock(&library->dlist_mutex);
	{
		char *c = &ibm_info->volser[0];
		sprintf(l_mess, "Issue load for %c%c%c%c%c%c to device %#8.8x",
		    c[0], c[1], c[2], c[3], c[4], c[5], ibm_info->drive_id);
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG, "load(%d) %#x, %c%c%c%c%c%c.",
			    LIBEQ, ibm_info->drive_id,
			    c[0], c[1], c[2], c[3], c[4], c[5]);
	}

	if (ioctl_ibmatl(library->open_fd, MTIOCLM, &mount_req) == -1) {
		ushort_t cc = mount_req.mtlmret.cc;
		char dmmy[DIS_MES_LEN * 2];

		sprintf(dmmy, "load cmd failed(%x): %s", cc,
		    (cc > HIGH_CC) ? "Undefined" : cc_codes[cc], cc);
		memccpy(l_mess, dmmy, '\0', DIS_MES_LEN);
		sam_syslog(LOG_INFO, "load(%d): (MTIOCLM): %m", LIBEQ);
		if (errno != ENOMEM && errno != EFAULT)
			sam_syslog(LOG_INFO, "load(%d): (MTIOCLM): %s(%#x)",
			    LIBEQ,
			    (cc > HIGH_CC) ? "Undefined" : cc_codes[cc], cc);
		free(dly_ent);
		mutex_unlock(&library->dlist_mutex);
		disp_of_event(library, event, MC_REQ_TR);
	} else {
		/* The delay processing thread will dispose of the event */
		dly_ent->req_id = mount_req.mtlmret.req_id;
		if ((dly_ent->next = library->delay_list) != NULL)
			library->delay_list->last = dly_ent;
		library->delay_list = dly_ent;
		mutex_unlock(&library->dlist_mutex);
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG, "load(%d) %#x, req_id = %#x.",
			    LIBEQ, ibm_info->drive_id,
			    mount_req.mtlmret.req_id);
	}
}


/*
 *	set_category - set the category of specified volume to whatever.
 *
 */
void
set_category(
	library_t *library,
	robo_event_t *event)
{
	int			delayed_req = FALSE;
	IBM_set_category_t 	set_req;
	delay_list_ent_t 	*dly_ent;
	ibm_req_info_t 		*ibm_info =
	    (ibm_req_info_t *)event->request.internal.address;


	memset(&set_req, 0, sizeof (IBM_set_category_t));
	set_req.target_cat = ibm_info->targ_cat;
	set_req.source_cat = ibm_info->src_cat;
	/* Eject process is done as a delayed request */
	if (ibm_info->targ_cat == EJECT_CATEGORY ||
	    ibm_info->targ_cat == B_EJECT_CATEGORY) {
		dly_ent = (delay_list_ent_t *)
		    malloc_wait(sizeof (delay_list_ent_t), 2, 0);
		memset(dly_ent, 0, sizeof (delay_list_ent_t));
		dly_ent->event = event;
		delayed_req = TRUE;
	} else
		set_req.wait_flg = 1;	/* indicate no delay */

	memcpy(set_req.volser, ibm_info->volser, 8);
	mutex_lock(&library->dlist_mutex);

	{
		char *c = &ibm_info->volser[0];
		sprintf(l_mess, "set category %#x on %c%c%c%c%c%c",
		    ibm_info->targ_cat, c[0], c[1], c[2], c[3], c[4], c[5]);

		if (!delayed_req)
			mutex_unlock(&library->dlist_mutex);
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG,
			    "set_category(%d) %#x, %c%c%c%c%c%c.", LIBEQ,
			    ibm_info->targ_cat,
			    c[0], c[1], c[2], c[3], c[4], c[5]);
	}

	if (ioctl_ibmatl(library->open_fd, MTIOCLSVC, &set_req) == -1) {
		ushort_t cc = set_req.mtlsvcret.cc;
		char dmmy[DIS_MES_LEN * 2];

		sprintf(dmmy, "set category failed(%x): %s", cc,
		    (cc > HIGH_CC) ? "Undefined" : cc_codes[cc], cc);
		memccpy(l_mess, dmmy, '\0', DIS_MES_LEN);

		sam_syslog(LOG_INFO, "set_category(%d): (MTIOCLSVC): %m",
		    LIBEQ);
		if (errno != ENOMEM && errno != EFAULT)
			sam_syslog(LOG_INFO,
			    "set_category(%d): (MTIOCLSVC): %s(%#x).", LIBEQ,
			    (cc > HIGH_CC) ? "Undefined" : cc_codes[cc], cc);
		if (delayed_req) {
			free(dly_ent);
			mutex_unlock(&library->dlist_mutex);
		}
		disp_of_event(library, event, MC_REQ_TR);
	} else if (delayed_req) {
		/* The delay processing thread will dispose of the event */
		dly_ent->req_id = set_req.mtlsvcret.req_id;
		if ((dly_ent->next = library->delay_list) != NULL)
			library->delay_list->last = dly_ent;
		library->delay_list = dly_ent;
		mutex_unlock(&library->dlist_mutex);

		if (DBG_LVL(SAM_DBG_DEBUG)) {
			char *c = &ibm_info->volser[0];

			sam_syslog(LOG_DEBUG,
			    "set_category(%d): %c%c%c%c%c%c, %#x, id = %#x.",
			    LIBEQ, c[0], c[1], c[2], c[3], c[4], c[5],
			    ibm_info->targ_cat, set_req.mtlsvcret.req_id);
		}
	} else
		disp_of_event(library, event, MC_REQ_OK);
}


/*
 *	force - force unload a drive
 *
 */
void
force(
	library_t *library,
	robo_event_t *event)
{
	IBM_dismount_t 		dismount_req;
	delay_list_ent_t 	*dly_ent;
	ibm_req_info_t 		*ibm_info =
	    (ibm_req_info_t *)event->request.internal.address;


	dly_ent = (delay_list_ent_t *)
	    malloc_wait(sizeof (delay_list_ent_t), 2, 0);
	memset(dly_ent, 0, sizeof (delay_list_ent_t));
	dly_ent->event = event;
	memset(&dismount_req, 0, sizeof (IBM_dismount_t));
	dismount_req.device = ibm_info->drive_id;
	dismount_req.target_cat = ibm_info->targ_cat;
	memset(dismount_req.volser, ' ', 8);

	mutex_lock(&library->dlist_mutex);
	sprintf(l_mess, "force unload device %#8.8x", ibm_info->drive_id);
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "force(%d) %#x.",
		    LIBEQ, ibm_info->drive_id);

	if (ioctl_ibmatl(library->open_fd, MTIOCLDM, &dismount_req) == -1) {
		ushort_t cc = dismount_req.mtldret.cc;
		char dmmy[DIS_MES_LEN * 2];

		sprintf(dmmy, "force unload cmd failed(%x): %s", cc,
		    (cc > HIGH_CC) ? "Undefined" : cc_codes[cc], cc);
		memccpy(l_mess, dmmy, '\0', DIS_MES_LEN);

		sam_syslog(LOG_INFO, "force(%d): (MTIOCLDM): %m", LIBEQ);
		if (errno != ENOMEM && errno != EFAULT)
			sam_syslog(LOG_INFO, "force(%d): (MTIOCLDM): %s(%#x)",
			    LIBEQ,
			    (cc > HIGH_CC) ? "Undefined" : cc_codes[cc], cc);
		free(dly_ent);
		mutex_unlock(&library->dlist_mutex);
		disp_of_event(library, event, MC_REQ_TR);
	} else {
		/* The delay processing thread will dispose of the event */
		dly_ent->req_id = dismount_req.mtldret.req_id;
		if ((dly_ent->next = library->delay_list) != NULL)
			library->delay_list->last = dly_ent;
		library->delay_list = dly_ent;
		mutex_unlock(&library->dlist_mutex);
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG, "force(%d) %#x, req_id = %#x.",
			    LIBEQ, ibm_info->drive_id,
			    dismount_req.mtldret.req_id);

	}
}


/*
 *	dismount - dismount volser
 *
 */
void
dismount(
	library_t *library,
	robo_event_t *event)
{
	IBM_dismount_t 		dismount_req;
	delay_list_ent_t 	*dly_ent;
	ibm_req_info_t 		*ibm_info =
	    (ibm_req_info_t *)event->request.internal.address;


	dly_ent = (delay_list_ent_t *)
	    malloc_wait(sizeof (delay_list_ent_t), 2, 0);
	memset(dly_ent, 0, sizeof (delay_list_ent_t));
	dly_ent->event = event;
	memset(&dismount_req, 0, sizeof (IBM_dismount_t));
	dismount_req.device = ibm_info->drive_id;
	dismount_req.target_cat = ibm_info->targ_cat;
	memcpy(dismount_req.volser, ibm_info->volser, 8);

	mutex_lock(&library->dlist_mutex);
	{
		char *c = &ibm_info->volser[0];
		sprintf(l_mess, "dismount %c%c%c%c%c%c from device %#8.8x",
		    c[0], c[1], c[2], c[3], c[4], c[5], ibm_info->drive_id);
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG, "dismount(%d) %#x, %c%c%c%c%c%c.",
			    LIBEQ, ibm_info->drive_id,
			    c[0], c[1], c[2], c[3], c[4], c[5]);
	}

	if (ioctl_ibmatl(library->open_fd, MTIOCLDM, &dismount_req) == -1) {
		ushort_t cc = dismount_req.mtldret.cc;

		char dmmy[DIS_MES_LEN * 2];

		sprintf(dmmy, "dismount cmd failed(%x): %s", cc,
		    (cc > HIGH_CC) ? "Undefined" : cc_codes[cc], cc);
		memccpy(l_mess, dmmy, '\0', DIS_MES_LEN);
		sam_syslog(LOG_INFO, "dismount(%d): (MTIOCLDM): %m", LIBEQ);
		if (errno != ENOMEM && errno != EFAULT)
			sam_syslog(LOG_INFO,
			    "dismount(%d): (MTIOCLDM): %s(%#x)", LIBEQ,
			    (cc > HIGH_CC) ? "Undefined" : cc_codes[cc], cc);
		free(dly_ent);
		mutex_unlock(&library->dlist_mutex);
		disp_of_event(library, event, MC_REQ_TR);
	} else {
		/* The delay processing thread will dispose of the event */
		dly_ent->req_id = dismount_req.mtldret.req_id;
		if ((dly_ent->next = library->delay_list) != NULL)
			library->delay_list->last = dly_ent;
		library->delay_list = dly_ent;
		mutex_unlock(&library->dlist_mutex);
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG, "dismount(%d) %#x, req_id = %#x.",
			    LIBEQ, ibm_info->drive_id,
			    dismount_req.mtldret.req_id);
	}
}


/*
 *	view - view library stuff.
 * Use library query to return information about things.
 * ibm_info specifies what information.
 */
void
view(
	library_t *library,
	robo_event_t *event)
{
	IBM_query_t 	query_req;
	ibm_req_info_t 	*ibm_info =
	    (ibm_req_info_t *)event->request.internal.address;

	memset(&query_req, 0, sizeof (IBM_query_t));
	memcpy(query_req.volser, ibm_info->volser, 8);
	query_req.device = ibm_info->drive_id;
	query_req.sub_cmd = ibm_info->sub_cmd;
	query_req.cat_seqno = ibm_info->seqno;
	query_req.source_cat = ibm_info->src_cat;

	{
		char *c = &ibm_info->volser[0];

		mutex_lock(&library->dlist_mutex);
		switch (ibm_info->sub_cmd) {
		case MT_QCID:
			sprintf(l_mess, "Issue query category %#x",
			    ibm_info->src_cat);
			break;

		case MT_QLD:
			memccpy(l_mess, "Issue query library",
			    '\0', DIS_MES_LEN);
			break;

		case MT_QEVD:
			sprintf(l_mess, "Issue query volume %c%c%c%c%c%c",
			    c[0], c[1], c[2], c[3], c[4], c[5]);
			break;

		case MT_QDD:
			sprintf(l_mess, "Issue query device %#8.8x",
			    ibm_info->drive_id);
			break;

		default:
			sam_syslog(LOG_DEBUG,
			    "Issue query sub_cmd = %#x ", ibm_info->sub_cmd);
			break;
		}

		mutex_unlock(&library->dlist_mutex);

		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG,
			    "view(%d) %c%c%c%c%c%c, sub_cmd = %#x ", LIBEQ,
			    c[0], c[1], c[2], c[3], c[4], c[5],
			    ibm_info->sub_cmd);
	}

	if (ioctl_ibmatl(library->open_fd, MTIOCLQ, &query_req) == -1) {
		ushort_t cc = query_req.mtlqret.cc;
		char dmmy[DIS_MES_LEN * 2];

		sprintf(dmmy, "query cmd %#x failed(%x): %s",
		    ibm_info->sub_cmd, cc,
		    (cc > HIGH_CC) ? "Undefined" : cc_codes[cc], cc);
		memccpy(l_mess, dmmy, '\0', DIS_MES_LEN);
		sam_syslog(LOG_INFO, "view(%d): (MTIOCLQ): %m", LIBEQ);
		if (errno != ENOMEM && errno != EFAULT)
			sam_syslog(LOG_INFO, "view(%d): (MTIOCLQ): %s(%#x)",
			    LIBEQ,
			    (cc > HIGH_CC) ? "Undefined" : cc_codes[cc], cc);
		disp_of_event(library, event, MC_REQ_FL);
	} else {
		ushort_t cc = query_req.mtlqret.cc;

		if (cc)
			sam_syslog(LOG_INFO, "view(%d): ??: (MTIOCLQ): %s(%#x)",
			    LIBEQ,
			    (cc > HIGH_CC) ? "Undefined" : cc_codes[cc], cc);
		if (query_req.mtlqret.info.info_type != 0) {
			ibm_info->ret_data =
			    malloc_wait(sizeof (IBM_query_info_t), 2, 0);
			memcpy(ibm_info->ret_data, &query_req.mtlqret.info,
			    sizeof (IBM_query_info_t));
		} else
			sam_syslog(LOG_INFO,
			    "view(%d): (MTIOCLQ): No information.", LIBEQ);
		disp_of_event(library, event, MC_REQ_OK);
	}
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "view(%d): returning.", LIBEQ);

}


/*
 *	query_lib - send a query to the library
 * Use library query to return information about the library.
 * ibm_info specifies what information.
 */
void
query_lib(
	library_t *library,
	robo_event_t *event)
{
	IBM_query_t 	query_req;
	ibm_req_info_t 	*ibm_info =
	    (ibm_req_info_t *)event->request.internal.address;

	memset(&query_req, 0, sizeof (IBM_query_t));
	query_req.sub_cmd = ibm_info->sub_cmd;
	query_req.cat_seqno = ibm_info->seqno;
	query_req.source_cat = ibm_info->src_cat;

	mutex_lock(&library->dlist_mutex);
	sprintf(l_mess, "Issue query lib sub_cmd %#x ", ibm_info->sub_cmd);
	mutex_unlock(&library->dlist_mutex);
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "query_lib(%d) cmd = %#x ",
		    LIBEQ, ibm_info->sub_cmd);

	if (ioctl_ibmatl(library->open_fd, MTIOCLQ, &query_req) == -1) {
		ushort_t cc = query_req.mtlqret.cc;
		char dmmy[DIS_MES_LEN * 2];

		sprintf(dmmy, "query lib cmd failed(%x): %s", cc,
		    (cc > HIGH_CC) ? "Undefined" : cc_codes[cc], cc);
		memccpy(l_mess, dmmy, '\0', DIS_MES_LEN);

		sam_syslog(LOG_INFO, "query_lib(%d): (MTIOCLQ): %m", LIBEQ);
		if (errno != ENOMEM && errno != EFAULT)
			sam_syslog(LOG_INFO,
			    "query_lib(%d): (MTIOCLQ): %s(%#x)", LIBEQ,
			    (cc > HIGH_CC) ? "Undefined" : cc_codes[cc], cc);
		disp_of_event(library, event, MC_REQ_FL);
	} else {
		ushort_t cc = query_req.mtlqret.cc;

		if (cc)
			sam_syslog(LOG_INFO,
			    "query_lib(%d): ??: (MTIOCLQ): %s(%#x)", LIBEQ,
			    (cc > HIGH_CC) ? "Undefined" : cc_codes[cc], cc);
		if (query_req.mtlqret.info.info_type != 0) {
			ibm_info->ret_data =
			    malloc_wait(sizeof (IBM_query_info_t), 2, 0);
			memcpy(ibm_info->ret_data, &query_req.mtlqret.info,
			    sizeof (IBM_query_info_t));
		} else
			sam_syslog(LOG_INFO,
			    "query_lib(%d): (MTIOCLQ): No information.", LIBEQ);
		disp_of_event(library, event, MC_REQ_OK);
	}
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "query_lib(%d): returning.", LIBEQ);
}

/*
 * query_drv - query a drive
 *
 */
void
query_drv(
	library_t *library,
	robo_event_t *event)
{
	IBM_query_t 	query_req;
	ibm_req_info_t 	*ibm_info =
	    (ibm_req_info_t *)event->request.internal.address;

	memset(&query_req, 0, sizeof (IBM_query_t));
	query_req.device = ibm_info->drive_id;
	query_req.sub_cmd = MT_QDD;

	mutex_lock(&library->dlist_mutex);
	sprintf(l_mess, "Issue query drive device %#8.8x", ibm_info->drive_id);
	mutex_unlock(&library->dlist_mutex);
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "query_drv(%d) %#x.", LIBEQ,
		    ibm_info->drive_id);

	if (ioctl_ibmatl(library->open_fd, MTIOCLQ, &query_req) == -1) {
		ushort_t cc = query_req.mtlqret.cc;
		char dmmy[DIS_MES_LEN * 2];

		sprintf(dmmy, "query drive cmd failed(%x): %s", cc,
		    (cc > HIGH_CC) ? "Undefined" : cc_codes[cc], cc);
		memccpy(l_mess, dmmy, '\0', DIS_MES_LEN);

		sam_syslog(LOG_INFO, "query_drv(%d): (MTIOCLQ): %m", LIBEQ);
		if (errno != ENOMEM && errno != EFAULT)
			sam_syslog(LOG_INFO,
			    "query_drv(%d): (MTIOCLQ): %s(%#x)", LIBEQ,
			    (cc > HIGH_CC) ? "Undefined" : cc_codes[cc], cc);
		disp_of_event(library, event, MC_REQ_FL);
	} else {
		ushort_t cc = query_req.mtlqret.cc;

		if (cc)
			sam_syslog(LOG_INFO,
			    "query_drv(%d): ??: (MTIOCLQ): %s(%#x)", LIBEQ,
			    (cc > HIGH_CC) ? "Undefined" : cc_codes[cc], cc);
		if (query_req.mtlqret.info.info_type != 0) {
			ibm_info->ret_data =
			    malloc_wait(sizeof (IBM_query_info_t), 2, 0);
			memcpy(ibm_info->ret_data, &query_req.mtlqret.info,
			    sizeof (IBM_query_info_t));
		} else
			sam_syslog(LOG_INFO,
			    "query_drv(%d): (MTIOCLQ): No information.", LIBEQ);
		disp_of_event(library, event, MC_REQ_OK);
	}
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "query_drv(%d) returning.", LIBEQ);

}


#if defined(THREADSDONTWORK)
	/* VARARGS4 */
int
start_request(
	char *cmd,
	int sequ,
	equ_t eq,
	robo_event_t *event,
	int cnt, ...)
{
	int 	fd, i, chld_stat;
	pid_t 	pid, got_pid;
	char 	**args, **args2, *path;
	char 	chr_shmid[12], chr_seq[12], chr_event[12], chr_equ[12];

	va_list aps;

	path = malloc_wait(MAXPATHLEN + 1, 2, 0);
	sprintf(chr_shmid, "%d", master_shm.shmid);
	sprintf(chr_seq, "%#x", sequ);
	sprintf(chr_event, "%#x", event);
	sprintf(chr_equ, "%d", eq);
	args = (char **)malloc((cnt + 6) * sizeof (char *));
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
	sprintf(path, "%s/ibm3494_helper", SAM_EXECUTE_PATH);
#if defined(DEBUG)
	sam_syslog(LOG_DEBUG, "start helper event address %#x", event);
	sam_syslog(LOG_DEBUG, "start helper %s, %s, %s, %s, %s",
	    args[0], args[1], args[2], args[3], args[4]);
#endif
	/* Set non-standard files to close on exec. */
	for (fd = STDERR_FILENO + 1; fd < OPEN_MAX; fd++) {
		(void) fcntl(fd, F_SETFD, FD_CLOEXEC);
	}
	if ((pid = fork1()) == 0) {	/* we are the child */
		/*
		 * Clear the special group id so that the file
		 * system does not get confused about any stage that might
		 * be in progress when we close the file.
		 */
		setgid(0);

		execv(path, args);
		_exit(STATUS_LAST + errno);
	}
	free(path);
	free(args);
	if (pid < 0) {
		sam_syslog(LOG_INFO,
		    "Unable to fork ibm3494 helper process:%m.");
		return (errno + STATUS_LAST);
	}
	if ((got_pid = waitpid(pid, &chld_stat, 0)) != pid) {
		sam_syslog(LOG_INFO,
		    "Waitpid: Waiting for %d, got $d with %#x.",
		    pid, got_pid, chld_stat);
		return (EINVAL + STATUS_LAST);
	}
	i = 0;

	if (WIFEXITED(chld_stat))
		i = WEXITSTATUS(chld_stat);
	else if (WIFSIGNALED(chld_stat)) {
		sam_syslog(LOG_INFO, "ibm3494 helper(%d) received signal %d.",
		    pid, WTERMSIG(chld_stat));
		i = EINTR + STATUS_LAST;
	}
	return (i);
}
#endif					/* THREADSDONTWORK */
