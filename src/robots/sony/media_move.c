/*
 * media_move.c - routines to issue requests to the transport
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

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <thread.h>
#include <synch.h>
#include <string.h>
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
#include "aml/dev_log.h"
#include "sam/param.h"
#include "aml/shm.h"
#include "aml/robots.h"
#include "sam/lib.h"
#include "aml/catlib.h"
#include "aml/catalog.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"
#include "sony.h"

#pragma ident "$Revision: 1.28 $"

/* Function prototypes */
void LogApiStatus(char *entry_ptr, int error);

extern shm_alloc_t master_shm, preview_shm;
extern char *sam_sony_status(int status);

/*
 * For the sony, a helper is used.  This helper will make the api request
 * to isolate the sony robot from the rpc calls since they are not thread
 * safe.  The completion code returned containes the success or failure of
 * the helper.  This would normally be the exit code of the helper, or
 * of the fork call.  If non zero, it is the exit code or error + STATUS_LAST.
 */

/*
 * force_media - unload a drive.
 *
 * exit -
 *    0 - ok
 *    1 - not ok
 *
 */
int
force_media(library_t *library, drive_state_t *drive, int *sony_error)
{
	int			err;
	char			*ent_pnt = "force_media";
	xport_state_t		*transport;
	robo_event_t		*force, *tmp;
	sony_information_t	*sony_info;

	sony_info = malloc_wait(sizeof (sony_information_t), 2, 0);
	memset(sony_info, 0, sizeof (sony_information_t));
	sony_info->BinNoChar = malloc_wait(PSCDRIVENAMELEN, 2, 0);
	memset(sony_info->BinNoChar, 0, sizeof (PSCDRIVENAMELEN));
	sony_info->PscCassInfo = NULL;
	sony_info->PscBinInfo = drive->PscBinInfo;
	strcpy(sony_info->BinNoChar, drive->BinNoChar);

	/*
	 * Build the transport thread request
	 */
	force = get_free_event(library);
	(void) memset(force, 0, sizeof (robo_event_t));

	force->request.internal.command = ROBOT_INTRL_FORCE_MEDIA;
	force->request.internal.address = (void *)sony_info;

	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG, "%s: from %s.", ent_pnt, drive->un->name);

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

	/*
	 * Wait for the trasnprot to do the unload
	 */
	while (force->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&force->condit, &force->mutex);
	mutex_unlock(&force->mutex);

	*sony_error = err = force->completion;
	if (err == PSCERR_NO_ERROR) {
		sony_resp_api_t *response = (sony_resp_api_t *)
		    &force->request.message.param.start_of_request;
		if (response->api_status != PSCERR_NO_ERROR) {
			err = response->api_status;
			LogApiStatus(ent_pnt, response->api_status);
			goto out;
		}
	} else
	sam_syslog(LOG_INFO, "helper-%s:status:%s", ent_pnt,
	    sam_sony_status(err));
out:

	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "Return from %s (%#x).", ent_pnt, err);
	free(sony_info);
	mutex_destroy(&force->mutex);
	cond_destroy(&force->condit);
	free(force);
	return (err);
}


/*
 * dismount_media - unload a volser
 * exit -
 *    0 - ok
 *    1 - not ok
 */
int
dismount_media(library_t *library, drive_state_t *drive, int *sony_error)
{
	int			err;
	char			*ent_pnt = "dismount_media";
	xport_state_t		*transport;
	robo_event_t		*dismount, *tmp;
	sony_information_t	*sony_info;

	sony_info = malloc_wait(sizeof (sony_information_t), 2, 0);
	memset(sony_info, 0, sizeof (sony_information_t));
	sony_info->BinNoChar = malloc_wait(PSCDRIVENAMELEN, 2, 0);
	memset(sony_info->BinNoChar, 0, sizeof (PSCDRIVENAMELEN));
	sony_info->PscBinInfo = drive->PscBinInfo;
	sony_info->PscCassInfo = NULL;
	strcpy(sony_info->BinNoChar, drive->BinNoChar);

	/*
	 * Build the transport thread request
	 */
	dismount = get_free_event(library);
	(void) memset(dismount, 0, sizeof (robo_event_t));
	dismount->request.internal.command = ROBOT_INTRL_DISMOUNT_MEDIA;
	dismount->request.internal.address = (void *)sony_info;
	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG, "%s: from %s.", ent_pnt, drive->un->name);

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
	/*
	 * Wait for the transport to do the unload
	 */
	while (dismount->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&dismount->condit, &dismount->mutex);
	mutex_unlock(&dismount->mutex);
	/*
	 * Check the dismount request/helper status
	 */
	*sony_error = err = dismount->completion;
	if (err == PSCERR_NO_ERROR) {
		sony_resp_api_t *response = (sony_resp_api_t *)
		    &dismount->request.message.param.start_of_request;
		if (response->api_status != PSCERR_NO_ERROR) {
			err = response->api_status;
			LogApiStatus(ent_pnt, response->api_status);
			goto out;
		}
	} else
		sam_syslog(LOG_INFO, "helper-%s status:%s", ent_pnt,
		    sam_sony_status(err));
out:

	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "Return from %s (%#x).", ent_pnt, err);
	free(sony_info);
	mutex_destroy(&dismount->mutex);
	cond_destroy(&dismount->condit);
	free(dismount);
	return (err);
}


/*
 * load_media - load media into a drive
 * exit -
 *   0 - ok
 *   1 - not ok
 */
int
load_media(library_t *library, drive_state_t *drive, struct CatalogEntry *ce,
	int *sony_error)
{
	int		err;
	char		*ent_pnt = "load_media";
	xport_state_t	*transport;
	robo_event_t	*load, *tmp;
	sony_information_t	*sony_info;

	sony_info = malloc_wait(sizeof (sony_information_t), 2, 0);
	memset(sony_info, 0, sizeof (sony_information_t));
	sony_info->PscCassInfo = malloc_wait(sizeof (PscCassInfo_t), 2, 0);
	memset(sony_info->PscCassInfo, 0, sizeof (PscCassInfo_t));
	sony_info->BinNoChar = malloc_wait(PSCDRIVENAMELEN, 2, 0);
	memset(sony_info->BinNoChar, 0, sizeof (PSCDRIVENAMELEN));
	sony_info->PscBinInfo = drive->PscBinInfo;
	memcpy(sony_info->PscCassInfo->szCassetteId, ce->CeBarCode,
	    PSCCASSIDLEN);
	strcpy(sony_info->BinNoChar, drive->BinNoChar);
	/*
	 * Build the transport request
	 */
	load = get_free_event(library);
	(void) memset(load, 0, sizeof (robo_event_t));
	load->request.internal.command = ROBOT_INTRL_LOAD_MEDIA;
	load->request.internal.address = (void *)sony_info;

	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG, "%s: from %s to %s(%s).", ent_pnt,
		    sony_info->PscCassInfo->szCassetteId, drive->un->name,
		    sony_info->BinNoChar);

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
	/*
	 * Wait for the transport to do the unload
	 */
	while (load->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&load->condit, &load->mutex);
	mutex_unlock(&load->mutex);

	*sony_error = err = load->completion;
	if (err == PSCERR_NO_ERROR) {
		/*
		 * Check the status of the request
		 */
		sony_resp_api_t *response =
		    (sony_resp_api_t *)
		    &load->request.message.param.start_of_request;
		if (response->api_status != PSCERR_NO_ERROR &&
		    response->api_status != PSCERR_DRIVE_BUSY) {
			/*
			 * The PscMoveCassette is always returning "drive busy"
			 * but actually sucessfully loads the tape. Until I can
			 * figure out why, just ignore this return code.
			 */
			err = response->api_status;
			LogApiStatus(ent_pnt, response->api_status);
			goto out;
		}
	} else
		sam_syslog(LOG_INFO, "helper-%s status:%s",
		    ent_pnt, sam_sony_status(err));

out:

	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "Return from %s (%#x).", ent_pnt, err);
	free(sony_info->PscCassInfo);
	free(sony_info);
	mutex_destroy(&load->mutex);
	cond_destroy(&load->condit);
	free(load);
	return (err);
}


/*
 * view_media - view a database entry
 * exit -
 *   0 - ok
 *   1 - not ok
 */
int
view_media(library_t *library, struct CatalogEntry *ce, int *sony_error)
{
	int		err;
	char		*ent_pnt = "view_media";
	xport_state_t	*transport;
	robo_event_t	*view, *tmp;
	sony_information_t	*sony_info;

	sony_info = malloc_wait(sizeof (sony_information_t), 2, 0);
	memset(sony_info, 0, sizeof (sony_information_t));
	sony_info->PscCassInfo = malloc_wait(sizeof (PscCassInfo_t), 2, 0);
	memset(sony_info->PscCassInfo, 0, sizeof (PscCassInfo_t));
	memcpy(sony_info->PscCassInfo->szCassetteId,
	    ce->CeBarCode, PSCCASSIDLEN);
	/*
	 * Build the transport thread
	 */
	view = get_free_event(library);
	(void) memset(view, 0, sizeof (robo_event_t));
	view->request.internal.command = ROBOT_INTRL_VIEW_DATABASE;
	view->request.internal.address = (void *)sony_info;

	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG, "%s: %s.", ent_pnt,
		    sony_info->PscCassInfo->szCassetteId);
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
	/*
	 * Wait for the transport to do the request
	 */
	while (view->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&view->condit, &view->mutex);
	mutex_unlock(&view->mutex);

	if ((err = view->completion) == PSCERR_NO_ERROR) {
		sony_resp_api_t *response =
		    (sony_resp_api_t *)
		    &view->request.message.param.start_of_request;
		if (response->api_status != PSCERR_NO_ERROR) {
			err = response->api_status;
			LogApiStatus(ent_pnt, response->api_status);
			goto out;
		}
	} else
		sam_syslog(LOG_INFO, "helper-%s status:%s",
		    ent_pnt, sam_sony_status(err));

out:

	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "Return from %s (%#x).", ent_pnt, err);
	mutex_destroy(&view->mutex);
	cond_destroy(&view->condit);
	free(view);
	return (err);
}


/*
 * get_media - get the media mounted.
 * entry   -
 *         drive->mutex should be held.  This mutex
 *         can be released during processing, but will be held
 *         on return.
 * returns -
 *		0 - ok ( RET_GET_MEDIA_SUCCESS )
 *		1 - some sort of error, dispose of event
 *			Note: The catalog mutex is held on this condition.
 *			( RET_GET_MEDIA_DISPOSE )
 *		2 - event was requeued.
 *			( RET_GET_MEDIA_REQUEUED )
 *		-1- same as 1 cept that the ACS library returned an error.
 *			Note: The catalog mutex is held on this condition.
 *			( RET_GET_MEDIA_DOWN_DRIVE )
 *		-2- dispose, error, return error
 *			( RET_GET_MEDIA_RET_ERROR )
 *		In all cases, dev_ent activity count will be incremented.
 */
int
get_media(library_t *library, drive_state_t *drive, robo_event_t *event,
    struct CatalogEntry *ce)
{
	int 		sony_error, status = 0;
	char 		*d_mess = drive->un->dis_mes[DIS_MES_NORM];
	char 		*ent_pnt = "get_media";
	dev_ent_t	*un = drive->un;

	mutex_lock(&un->mutex);
	un->active++;
	mutex_unlock(&un->mutex);

	sony_error = 0;

	/*
	 * Check if the media is already mounted
	 */
	if (drive->status.b.full && (un->slot == ce->CeSlot)) {
		if (DBG_LVL(SAM_DBG_TMOVE))
			sam_syslog(LOG_DEBUG, "%s:(%d)%s:%d.", ent_pnt, un->eq,
			    __FILE__, __LINE__);
	} else {
		/*
		 * Make sure the source storage element has media in it.
		 * If the element is empty, then the request is put back on
		 * the library's work list - if the request is not internal.
		 * If the request is internal then an error is returned to
		 * the caller. If the in_use flag is not set, then the slot is
		 * empty and the request will be disposed of.
		 */
		if (DBG_LVL(SAM_DBG_TMOVE))
			sam_syslog(LOG_DEBUG, "%s:(%d)%s:%d.", ent_pnt, un->eq,
			    __FILE__, __LINE__);
		if (!(ce->CeStatus & CES_occupied)) {
			/*
			 * Check if this should be put back on the
			 * library's list
			 */
			if ((ce->CeStatus & CES_inuse) && event != NULL &&
			    event->type != EVENT_TYPE_INTERNAL &&
			    !event->status.b.dont_reque) {

				event->next = NULL;
				add_to_end(library, event);
				/* Do not dispose of event. */
				return (RET_GET_MEDIA_REQUEUED);
			}
			/*
			 * Note: the catalog mutex is held on this return
			 * This will allow the caller to clean up flags for
			 * entry being processed.
			 */
			if (DBG_LVL(SAM_DBG_TMOVE))
				sam_syslog(LOG_DEBUG, "%s:(%d)%s:%d.", ent_pnt,
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
			sprintf(d_mess, "dismounting %s", un->vsn);
			if (dismount_media(library, drive, &sony_error)) {
				if (DBG_LVL(SAM_DBG_TMOVE))
					sam_syslog(LOG_DEBUG, "%s:(%d)%s:%d.",
					    ent_pnt, un->eq,
					    __FILE__, __LINE__);
				return (RET_GET_MEDIA_DOWN_DRIVE);
			}
			/*
			 * Clean up the old entry
			 */
			if (*un->i.ViMtype != '\0') un->i.ViFlags |= VI_mtype;
			if (*un->i.ViVsn != '\0') un->i.ViFlags |= VI_vsn;
			CatalogVolumeUnloaded(&un->i, "");

			mutex_lock(&un->mutex);
			un->slot = ROBOT_NO_SLOT;
			un->mid = un->flip_mid = ROBOT_NO_SLOT;
			un->label_time = 0;
			mutex_unlock(&un->mutex);
			/* Clear the drive information */
			drive->status.b.full = FALSE;
			drive->status.b.bar_code = FALSE;
		}
		sprintf(d_mess, "mount %s", ce->CeVsn);
		if (load_media(library, drive, ce, &sony_error)) {
			/*
			 * Process error and return status to caller.
			 * Requeue the event if not internal otherwise
			 * return status to caller.
			 * Must exit with catalog mutex held if abs(exit) == 1.
			 */
			int	ret = RET_GET_MEDIA_DOWN_DRIVE;
			struct VolId vid;

			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG, "load failed.");
			if (DBG_LVL(SAM_DBG_TMOVE))
				sam_syslog(LOG_DEBUG, "%s:(%d)%s:%d.", ent_pnt,
				    un->eq, __FILE__, __LINE__);
			sprintf(d_mess, "mount of %s failed.", ce->CeVsn);
			/*
			 * Attempt recovery.
			 * Check the state of the volume itself.
			 */
			if (sony_error == PSCERR_NO_SUCH_CASSETTE) {
				status |= CES_occupied;
				(void) CatalogSetFieldByLoc(library->un->eq,
				    ce->CeSlot, 0, CEF_Status, status, 0);
				memset(&vid, 0, sizeof (struct VolId));
				vid.ViEq = library->un->eq;
				vid.ViSlot = ce->CeSlot;
				vid.ViFlags = VI_cart;
				status = CatalogExport(&vid);
				ret = RET_GET_MEDIA_RET_ERROR;
			} else {
				status |= CES_occupied;
				(void) CatalogSetFieldByLoc(library->un->eq,
				    ce->CeSlot, 0, CEF_Status, status, 0);
				/* Do not requeue an internal event */
				if (event == NULL ||
				    event->type == EVENT_TYPE_INTERNAL)
					ret = RET_GET_MEDIA_DISPOSE;
				else {
					event->next = NULL;
					add_to_end(library, event);
					ret = RET_GET_MEDIA_REQUEUED;
				}
			}
			return (ret);
		}
	}
	mutex_lock(&un->mutex);
	drive->status.b.full = TRUE;
	drive->status.b.valid = TRUE;
	un->slot = ce->CeSlot;
	un->mid = ce->CeMid;
	un->status.b.labeled = FALSE;
	un->status.b.ready = FALSE;

	memmove(un->vsn, ce->CeVsn, sizeof (un->vsn));
	if (ce->CeStatus & CES_bar_code) {
		drive->status.b.bar_code = TRUE;
		memcpy(drive->bar_code, ce->CeBarCode, BARCODE_LEN);
	} else {
		memset(drive->bar_code, 0, BARCODE_LEN);
	}

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


/*
 * Log the api_status
 */
void
LogApiStatus(char *ent_ptr, int err)
{
	sam_syslog(LOG_INFO, "helper failed on %s request, error(%#x).",
	    ent_ptr, err);
}
