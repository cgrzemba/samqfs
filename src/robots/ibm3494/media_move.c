/*
 *	media_move.c - routines to issue requests to the library.
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include "sam/types.h"
#include "aml/device.h"
#include "sam/param.h"
#include "aml/shm.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/robots.h"
#include "sam/lib.h"
#include "aml/proto.h"
#include "ibm3494.h"

#pragma ident "$Revision: 1.28 $"

static char *_SrcFile = __FILE__;

/* some globals */
extern shm_alloc_t master_shm, preview_shm;


/*
 *	force_media - unload a drive. Issued as a delayed request.
 *
 */
req_comp_t
force_media(
	library_t *library,
	drive_state_t *drive)
{
	req_comp_t 	err;
	ibm_req_info_t 	*ibm_info;
	robo_event_t 	*force, *tmp;
	xport_state_t 	*transport;

	ibm_info = (ibm_req_info_t *)malloc_wait(sizeof (ibm_req_info_t), 2, 0);
	memset(ibm_info, 0, sizeof (ibm_req_info_t));
	ibm_info->drive_id = drive->drive_id;

	/* Build transport thread request */

	force = malloc_wait(sizeof (robo_event_t), 5, 0);
	(void) memset(force, 0, sizeof (robo_event_t));

	force->request.internal.command = ROBOT_INTRL_FORCE_MEDIA;
	force->request.internal.address = (void *)ibm_info;

	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG, "force_media: from %s.", drive->un->name);

	force->type = EVENT_TYPE_INTERNAL;
	force->status.bits = REST_SIGNAL;
	force->completion = REQUEST_NOT_COMPLETE;

	transport = library->transports;
	mutex_lock(&force->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0)
		transport->first = force;
	else {
		LISTEND(transport, tmp);
		append_list(tmp, force);
	}
	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);

	/* Wait for the transport to do the unload */
	while (force->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&force->condit, &force->mutex);
	mutex_unlock(&force->mutex);

	err = (req_comp_t)force->completion;
	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG,
		    "Return from transport force (%#x).", force->completion);

	free(ibm_info);
	mutex_destroy(&force->mutex);
	free(force);
	return (err);
}


/*
 *	dismount_media - unload a volser
 *
 */
req_comp_t
dismount_media(
	library_t *library,
	drive_state_t *drive)
{
	req_comp_t 	err;
	ibm_req_info_t 	*ibm_info;
	xport_state_t 	*transport;
	robo_event_t 	*dismount, *tmp;

	ibm_info = malloc_wait(sizeof (ibm_req_info_t), 2, 0);
	memset(ibm_info, 0, sizeof (ibm_req_info_t));
	ibm_info->drive_id = drive->drive_id;
	sprintf((void *)&ibm_info->volser, "%-8.8s", drive->bar_code);

	/* Build transport thread request */

	dismount = malloc_wait(sizeof (robo_event_t), 5, 0);
	(void) memset(dismount, 0, sizeof (robo_event_t));

	dismount->request.internal.command = ROBOT_INTRL_DISMOUNT_MEDIA;
	dismount->request.internal.address = (void *)ibm_info;

	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG, "dismount_media: from %s.",
		    drive->un->name);

	dismount->type = EVENT_TYPE_INTERNAL;
	dismount->status.bits = REST_SIGNAL;
	dismount->completion = REQUEST_NOT_COMPLETE;

	transport = library->transports;
	mutex_lock(&dismount->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0)
		transport->first = dismount;
	else {
		LISTEND(transport, tmp);
		append_list(tmp, dismount);
	}
	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);

	/* Wait for the transport to do the unload */
	while (dismount->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&dismount->condit, &dismount->mutex);
	mutex_unlock(&dismount->mutex);

	/* Check the dismount request/helper status */
	err = (req_comp_t)dismount->completion;
	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG, "Return from transport dismount (%#x).",
		    dismount->completion);
	free(ibm_info);
	mutex_destroy(&dismount->mutex);
	free(dismount);
	return (err);
}


/*
 *	load_media - load media into a drive
 *
 */
req_comp_t
load_media(
	library_t *library,
	drive_state_t *drive,
	struct CatalogEntry *ce,
	ushort_t category)
{
	req_comp_t 	err;
	xport_state_t 	*transport;
	robo_event_t 	*load, *tmp;
	ibm_req_info_t 	*ibm_info;

	ibm_info = malloc_wait(sizeof (ibm_req_info_t), 2, 0);
	memset(ibm_info, 0, sizeof (ibm_req_info_t));
	ibm_info->drive_id = drive->drive_id;
	ibm_info->src_cat = category;

	if (ce != NULL)
		memcpy((void *)&drive->bar_code, ce->CeBarCode, 8);
	else {
		memset((void *)&drive->bar_code, 0, 8);
		memset((void *)&drive->bar_code, ' ', 6);
	}

	sprintf((void *)&ibm_info->volser, "%-8.8s", drive->bar_code);

	/* Build transport thread request */
	load = malloc_wait(sizeof (robo_event_t), 5, 0);
	(void) memset(load, 0, sizeof (robo_event_t));
	load->request.internal.command = ROBOT_INTRL_LOAD_MEDIA;
	load->request.internal.address = (void *)ibm_info;

	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG,
		    "load_media(%d): from %s to %s.", LIBEQ, drive->bar_code,
		    drive->un->name);

	load->type = EVENT_TYPE_INTERNAL;
	load->status.bits = REST_SIGNAL;
	load->completion = REQUEST_NOT_COMPLETE;

	transport = library->transports;
	mutex_lock(&load->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0)
		transport->first = load;
	else {
		LISTEND(transport, tmp);
		append_list(tmp, load);
	}
	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);

	/* Wait for the transport to do the unload */
	while (load->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&load->condit, &load->mutex);
	mutex_unlock(&load->mutex);

	err = (req_comp_t)load->completion;
	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG,
		    "Return from transport load (%#x).", load->completion);

	free(ibm_info);
	mutex_destroy(&load->mutex);
	free(load);
	return (err);
}


/*
 *	view_media - view a database entry
 */
req_comp_t
view_media(
	library_t *library,
	char *vsn,
	void **ret_data)
{
	req_comp_t 	err;
	ibm_req_info_t 	*ibm_info;
	xport_state_t 	*transport;
	robo_event_t 	*view, *tmp;

	ibm_info = malloc_wait(sizeof (ibm_req_info_t), 2, 0);
	memset(ibm_info, 0, sizeof (ibm_req_info_t));
	ibm_info->sub_cmd = MT_QEVD;	/* view a single data base entry */
	sprintf((void *)&ibm_info->volser, "%-8.8s", vsn);

	/* Build transport thread request */
	view = malloc_wait(sizeof (robo_event_t), 5, 0);
	(void) memset(view, 0, sizeof (robo_event_t));
	view->request.internal.command = ROBOT_INTRL_VIEW_DATABASE;
	view->request.internal.address = (void *)ibm_info;

	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG, "view_media: %s.", vsn);

	view->type = EVENT_TYPE_INTERNAL;
	view->status.bits = REST_SIGNAL;
	view->completion = REQUEST_NOT_COMPLETE;

	transport = library->transports;

	mutex_lock(&view->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0)
		transport->first = view;
	else {
		LISTEND(transport, tmp);
		append_list(tmp, view);
	}
	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);

	/* Wait for the transport to do the request */
	while (view->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&view->condit, &view->mutex);
	mutex_unlock(&view->mutex);

	err = (req_comp_t)view->completion;
	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG, "Return from view (%#x).",
		    view->completion);

	if (!err) {
		*ret_data = malloc_wait(sizeof (IBM_query_info_t), 2, 0);
		memcpy(*ret_data, ibm_info->ret_data,
		    sizeof (IBM_query_info_t));
	} else
		*ret_data = NULL;

	free(ibm_info);
	mutex_destroy(&view->mutex);
	free(view);
	return (err);
}


/*
 *	view_media_category - return media in the given category.
 */
req_comp_t
view_media_category(
	library_t *library,
	int seqno,		/* starting-1 sequence number */
	void **ret_data,	/* return data */
	ushort_t category)
{
	req_comp_t	err;
	ibm_req_info_t	*ibm_info;
	xport_state_t	*transport;
	robo_event_t	*view, *tmp;

	ibm_info = malloc_wait(sizeof (ibm_req_info_t), 2, 0);
	memset(ibm_info, 0, sizeof (ibm_req_info_t));
	ibm_info->sub_cmd = MT_QCID;	/* view all entries of categroy */
	ibm_info->src_cat = category;
	ibm_info->seqno = seqno;
	memset(&ibm_info->volser[0], ' ', 8);

	/* Build transport thread request */
	view = malloc_wait(sizeof (robo_event_t), 5, 0);
	(void) memset(view, 0, sizeof (robo_event_t));
	view->request.internal.command = ROBOT_INTRL_VIEW_DATABASE;
	view->request.internal.address = (void *)ibm_info;

	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG, "view_media_category: %#x.", category);

	view->type = EVENT_TYPE_INTERNAL;
	view->status.bits = REST_SIGNAL;
	view->completion = REQUEST_NOT_COMPLETE;
	transport = library->transports;

	mutex_lock(&view->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0)
		transport->first = view;
	else {
		LISTEND(transport, tmp);
		append_list(tmp, view);
	}
	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);

	/* Wait for the transport to do the request */
	while (view->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&view->condit, &view->mutex);
	mutex_unlock(&view->mutex);

	err = (req_comp_t)view->completion;

	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG, "Return from view_media_category (%#x).",
		    view->completion);
	if (!err) {
		*ret_data = malloc_wait(sizeof (IBM_query_info_t), 2, 0);
		memcpy(*ret_data, ibm_info->ret_data,
		    sizeof (IBM_query_info_t));
	} else
		*ret_data = NULL;

	free(ibm_info);
	mutex_destroy(&view->mutex);
	free(view);
	return (err);
}


/*
 *	query_library - send the supplied query to the library.
 */
req_comp_t
query_library(
	library_t *library,
	int seqno,		/* starting-1 sequence number */
	int sub_cmd,		/* what query */
	void **ret_data,	/* return data */
	ushort_t category)
{
	req_comp_t 	err;
	ibm_req_info_t 	*ibm_info;
	xport_state_t 	*transport;
	robo_event_t 	*view, *tmp;

	ibm_info = malloc_wait(sizeof (ibm_req_info_t), 2, 0);
	memset(ibm_info, 0, sizeof (ibm_req_info_t));
	ibm_info->sub_cmd = sub_cmd;
	ibm_info->src_cat = category;
	ibm_info->seqno = seqno;
	memset(&ibm_info->volser[0], ' ', 8);

	/* Build transport thread request */
	view = malloc_wait(sizeof (robo_event_t), 5, 0);
	(void) memset(view, 0, sizeof (robo_event_t));
	view->request.internal.command = ROBOT_INTRL_QUERY_LIBRARY;
	view->request.internal.address = (void *)ibm_info;

	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG, "query_library: %#x.", sub_cmd);

	view->type = EVENT_TYPE_INTERNAL;
	view->status.bits = REST_SIGNAL;
	view->completion = REQUEST_NOT_COMPLETE;
	transport = library->transports;

	mutex_lock(&view->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0)
		transport->first = view;
	else {
		LISTEND(transport, tmp);
		append_list(tmp, view);
	}
	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);

	/* Wait for the transport to do the request */
	while (view->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&view->condit, &view->mutex);
	mutex_unlock(&view->mutex);

	err = (req_comp_t)view->completion;
	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG, "Return from query_library (%#x).",
		    view->completion);
	if (!err) {
		*ret_data = malloc_wait(sizeof (IBM_query_info_t), 2, 0);
		memcpy(*ret_data, ibm_info->ret_data,
		    sizeof (IBM_query_info_t));
	} else
		*ret_data = NULL;

	free(ibm_info);
	mutex_destroy(&view->mutex);
	free(view);
	return (err);
}


/*
 *	set_media_category - change the category of media.
 */
req_comp_t
set_media_category(
	library_t *library,
	char *volser,
	ushort_t src_cat,	/* source category */
	ushort_t targ_cat)
{
	req_comp_t	err;
	ibm_req_info_t 	*ibm_info;
	xport_state_t 	*transport;
	robo_event_t 	*set, *tmp;

	ibm_info = malloc_wait(sizeof (ibm_req_info_t), 2, 0);
	memset(ibm_info, 0, sizeof (ibm_req_info_t));
	ibm_info->targ_cat = targ_cat;
	ibm_info->src_cat = src_cat;
	sprintf((void *)&ibm_info->volser, "%-8.8s", volser);

	/* Build transport thread request */
	set = malloc_wait(sizeof (robo_event_t), 5, 0);
	(void) memset(set, 0, sizeof (robo_event_t));
	set->request.internal.command = ROBOT_INTRL_SET_CATEGORY;
	set->request.internal.address = (void *)ibm_info;

	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG,
		    "set_media_category: %s %#x->%#x.", volser, src_cat,
		    targ_cat);

	set->type = EVENT_TYPE_INTERNAL;
	set->status.bits = REST_SIGNAL;
	set->completion = REQUEST_NOT_COMPLETE;
	transport = library->transports;

	mutex_lock(&set->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0)
		transport->first = set;
	else {
		LISTEND(transport, tmp);
		append_list(tmp, set);
	}
	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);

	/* Wait for the transport to do the request */
	while (set->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&set->condit, &set->mutex);
	mutex_unlock(&set->mutex);

	err = (req_comp_t)set->completion;
	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG, "Return from set (%#x).",
		    set->completion);

	free(ibm_info);
	mutex_destroy(&set->mutex);
	free(set);
	return (err);
}


/*
 *	get_media - get the media mounted.
 *
 * entry   -
 *	 drive->mutex should be held.  This mutex
 *	 can be released during processing, but will be held
 *	 on return.
 *
 * returns -
 *	 0 - ok
 *	 1 - some sort of error, dispose of event
 *		Note: The catalog mutex is held on this condition.
 *	 2 - event was requeued.
 *	 -1 - same as 1 cept that the drive should be downed
 *
 *	 in all cases, dev_ent activity count will be incremented.
 */
int
get_media(
	library_t *library,
	drive_state_t *drive,
	robo_event_t *event,		/* the event (can be NULL) */
	struct CatalogEntry *ce)	/* catalog entry to be loaded */
{
	char *d_mess = drive->un->dis_mes[DIS_MES_NORM];
	dev_ent_t *un = drive->un;
	int status = 0;

	mutex_lock(&un->mutex);
	INC_ACTIVE(un);
	mutex_unlock(&un->mutex);

	/* is the media is already mounted */
	if (drive->status.b.full && (un->slot == ce->CeSlot)) {
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG, "get_media:(%d)%s:%d.",
			    un->eq, __FILE__, __LINE__);
	} else {			/* get the media loaded */
		/*
		 * Make sure the source storage element has media in it.
		 * If the element is empty, external requests
		 * will be put back on the library's work list with the
		 * hope that it will be picked up later. Internal
		 * requests will return an error to the caller. If the in_use
		 * flag is not set, then the slot is
		 * "really" empty and the request will be disposed of.
		 */
		if (DBG_LVL(SAM_DBG_TMOVE))
			sam_syslog(LOG_DEBUG, "get_media:(%d)%s:%d.",
			    un->eq, __FILE__, __LINE__);

		if (!(ce->CeStatus & CES_occupied)) {
			/* Should this be put back on the library's list? */
			if ((ce->CeStatus & CES_inuse) && event != NULL &&
			    event->type != EVENT_TYPE_INTERNAL &&
			    !event->status.b.dont_reque) {
				event->next = NULL;
				add_to_end(library, event);
				/* Do not dispose of event */
				return (RET_GET_MEDIA_REQUEUED);
			}

			if (DBG_LVL(SAM_DBG_TMOVE))
				sam_syslog(LOG_DEBUG, "get_media:(%d)%s:%d.",
				    un->eq, __FILE__, __LINE__);
			return (RET_GET_MEDIA_DISPOSE);

		} else {
			status &= ~CES_occupied;
			(void) CatalogSetFieldByLoc(library->un->eq, ce->CeSlot,
			    0, CEF_Status, status, CES_occupied);
		}

		if (drive->status.b.full) {

			mutex_lock(&un->mutex);
			un->status.bits |= DVST_UNLOAD;
			mutex_unlock(&un->mutex);

			/*
			 * Save off what information we know about this volume
			 * before we spin down the drive and clear out the un.
			 */
			memmove(un->i.ViMtype, sam_mediatoa(un->type),
			    sizeof (un->i.ViMtype));
			memmove(un->i.ViVsn, un->vsn, sizeof (un->i.ViVsn));
			un->i.ViEq = un->fseq;
			un->i.ViSlot = un->slot;
			un->i.ViPart = 0;

			if (un->status.b.ready) {
				(void) spin_drive(drive, SPINDOWN, NOEJECT);
				sprintf(d_mess, "unloading %s", un->vsn);
			}

			mutex_lock(&un->mutex);
			close_unit(un, &drive->open_fd);
			un->status.bits = (DVST_REQUESTED | DVST_PRESENT) |
			    (un->status.bits & DVST_CLEANING);
			clear_un_fields(un);
			mutex_unlock(&un->mutex);

#if defined(USEDISMOUNT)
			sprintf(d_mess, "dismounting %s", un->vsn);
			if (dismount_media(library, drive) != MC_REQ_OK) {
				if (DBG_LVL(SAM_DBG_TMOVE))
					sam_syslog(LOG_DEBUG,
					    "get_media:(%d)%s:%d.",
					    un->eq, __FILE__, __LINE__);
				return (RET_GET_MEDIA_DOWN_DRIVE);
			}
#endif					/* defined(USEDISMOUNT) */


			/* clean up the old entries */
			if (*un->i.ViMtype != '\0')
				un->i.ViFlags |= VI_mtype;
			if (*un->i.ViVsn != '\0')
				un->i.ViFlags |= VI_vsn;
			CatalogVolumeUnloaded(&un->i, "");

			mutex_lock(&un->mutex);
			un->slot = ROBOT_NO_SLOT;
			un->mid = un->flip_mid = ROBOT_NO_SLOT;
			un->label_time = 0;
			mutex_unlock(&un->mutex);

			/* clear the drive information */
			drive->status.b.full = FALSE;
			drive->status.b.bar_code = FALSE;
		}
#if !defined(USEDISMOUNT)
		if (*un->i.ViVsn != '\0')
			sprintf(d_mess, "dismount %s/mount %s",
			    un->i.ViVsn, un->vsn);
		else
			sprintf(d_mess, "mount %s", ce->CeVsn);
#else
		sprintf(d_mess, "mount %s", ce->CeVsn);
#endif
		if (load_media(library, drive, ce, 0) != MC_REQ_OK) {
			/*
			 * Process error and return status to caller.
			 * Requeue the event if not internal otherwise
			 * return status to caller.
			 */
			req_comp_t err;
			int ret = RET_GET_MEDIA_DISPOSE;
			IBM_query_info_t *info;
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG, "load of %s failed.",
				    ce->CeVsn);

			sprintf(d_mess, "mount of %s failed.", ce->CeVsn);
			if (DBG_LVL(SAM_DBG_TMOVE))
				sam_syslog(LOG_DEBUG, "get_media:(%d)%s:%d.",
				    un->eq, __FILE__, __LINE__);

			/*
			 * Attempt some sort of recovery:
			 * Check the state of the volume itself
			 */
			err = view_media(library, ce->CeBarCode, (void *)&info);
			if (err != MC_REQ_OK) {

				status |= CES_occupied;
				(void) CatalogSetFieldByLoc(library->un->eq,
				    ce->CeSlot, 0, CEF_Status, status, 0);

				/* If internal event, then dont requeue it */
				if (event == NULL ||
				    event->type == EVENT_TYPE_INTERNAL)
					ret = RET_GET_MEDIA_DISPOSE;
			} else if (info != NULL) {
				ushort_t vol_status, category;

				memcpy(&vol_status, &info->
				    data.expand_vol_data.volume_status[0],
				    sizeof (vol_status));
				memcpy(&category,
				    &info->data.expand_vol_data.cat_assigned[0],
				    sizeof (category));
				if (vol_status & MT_VLI)
					sam_syslog(LOG_INFO, "get_media(%d):"
					    " %s Present but inaccessible.",
					    LIBEQ, ce->CeVsn);
				if (vol_status & (MT_VM | MT_VQM | MT_VPM |
				    MT_VQD | MT_VPD))
					sam_syslog(LOG_INFO, "get_media(%d):"
					    " %s, Mounting/Dismounting "
					    " or queued.", LIBEQ, ce->CeVsn);
				if (vol_status & (MT_VQE | MT_VPE))
					sam_syslog(LOG_INFO, "get_media(%d):"
					    " %s, Ejecting.", LIBEQ,
					    ce->CeVsn);
				if (vol_status & MT_VMIS)
					sam_syslog(LOG_INFO, "get_media(%d):"
					    " %s, Misplaced.", LIBEQ,
					    ce->CeVsn);
				if (vol_status & MT_VUU)
					sam_syslog(LOG_INFO, "get_media(%d):"
					    " %s, Unreadable label.", LIBEQ,
					    ce->CeVsn);
				if (vol_status & MT_VMM)
					sam_syslog(LOG_INFO, "get_media(%d):"
					    " %s, Used during manual mode.",
					    LIBEQ, ce->CeVsn);
				if (vol_status & MT_VME)
					sam_syslog(LOG_INFO, "get_media(%d):"
					    " %s, Manually Ejected.",
					    LIBEQ, ce->CeVsn);
				if (category == MAN_EJECTED_CAT)
					set_media_category(library, ce->CeVsn,
					    MAN_EJECTED_CAT,
					    PURGE_VOL_CATEGORY);
				if (vol_status & (MT_VLI | MT_VQE | MT_VPE |
				    MT_VMIS | MT_VUU | MT_VME)) {
					ret = RET_GET_MEDIA_DISPOSE;
					sam_syslog(LOG_INFO,
					    "rec_cat(%d): %s, ce->CeSlot %d:"
					    " removed from catalog",
					    LIBEQ, ce->CeVsn, ce->CeSlot);
				}
				free(info);
			}
			return (ret);
		}
	}

	mutex_lock(&un->mutex);
	drive->status.b.full = TRUE;
	drive->status.b.valid = TRUE;
	memmove(un->vsn, ce->CeVsn, sizeof (un->vsn));
	un->slot = ce->CeSlot;
	un->mid = ce->CeMid;
	un->flip_mid = ROBOT_NO_SLOT;
	un->status.b.labeled = FALSE;
	un->status.b.ready = FALSE;
	drive->status.b.bar_code = TRUE;
	memcpy(drive->bar_code, ce->CeBarCode, BARCODE_LEN + 1);
	un->space = ce->CeSpace;
	switch (un->type & DT_CLASS_MASK) {
	case DT_OPTICAL:
		un->dt.od.ptoc_fwa = ce->m.CePtocFwa;
		break;

	case DT_TAPE:
		un->dt.tp.position = ce->m.CeLastPos;
		break;
	}
	mutex_unlock(&un->mutex);

	return (RET_GET_MEDIA_SUCCESS);
}
