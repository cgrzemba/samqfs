/*
 *	media_move.c - routines to issue requests to the transport
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

static char *_SrcFile = __FILE__;

#include <thread.h>
#include <synch.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
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
#include "stk.h"

#pragma ident "$Revision: 1.46 $"

/*	Function prototypes */
void LogApiStatus(char *entry_ptr, int error);

extern shm_alloc_t master_shm, preview_shm;

/*
 * For stk, a helper is used. This helper will make the acsapi request
 * to isolate the stk robot from the rpc calls since they are not thread
 * safe.  The completion code returned containes the success or failure of
 * the helper.	This would normally be the exit code of the helper, or
 * of the fork call.  If non zero, it is the exit code or error + STATUS_LAST.
 */

/*
 *	force_media - unload a drive.
 * exit -
 *	  0 - ok
 *	  1 - not ok
 *
 */
int
force_media(
	library_t *library,	/* pointer to the library */
	drive_state_t *drive,	/* drive */
	int *stk_error)
{
	int 	retry, err;
	char 	*ent_pnt = "force_media";
	xport_state_t 	*transport;
	robo_event_t 	*force, *tmp;
	stk_information_t *stk_info;
	retry = 1;

retry:

	stk_info = malloc_wait(sizeof (stk_information_t), 2, 0);
	memset(stk_info, 0, sizeof (stk_information_t));
	stk_info->drive_id = drive->drive_id;

	if ((drive->un->slot != ROBOT_NO_SLOT) &&
	    (library->hasam_running) &&
	    (strlen((char *)drive->bar_code) != 0)) {

		memcpy((void *)&stk_info->vol_id.external_label[0],
		    (void *)&drive->bar_code, EXTERNAL_LABEL_SIZE);
	}

	/* Build transport thread request */
	force = get_free_event(library);
	(void) memset(force, 0, sizeof (robo_event_t));

	force->request.internal.command = ROBOT_INTRL_FORCE_MEDIA;
	force->request.internal.address = (void *)stk_info;

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

	/* Wait for the transport to do the unload */
	while (force->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&force->condit, &force->mutex);
	mutex_unlock(&force->mutex);

	*stk_error = err = force->completion;
	if (err == STATUS_SUCCESS) {
		stk_resp_acs_t *acs_resp =
		    (stk_resp_acs_t *)&
		    force->request.message.param.start_of_request;
		ACS_DISMOUNT_RESPONSE *dr = &acs_resp->data.dismount;


		/* Check the api_status before proceeding. */
		if (acs_resp->hdr.api_status != STATUS_SUCCESS) {
			LogApiStatus(ent_pnt, acs_resp->hdr.api_status);
			goto out;
		}
		/* Get acs_response status */
		*stk_error = err = acs_resp->hdr.acs_status;
		if (err == STATUS_SUCCESS) {
			/* Get status of the dismount command */
			*stk_error = err = dr->dismount_status;
			if (err != STATUS_SUCCESS) {
				sam_syslog(LOG_INFO,
				    "%s:%d,%d,%d,%d:returned:%s", ent_pnt,
				    DRIVE_LOC(drive->drive_id),
				    sam_acs_status(err));
				if (err == STATUS_LSM_OFFLINE) {
					down_library(library, SAM_STATE_CHANGE);
				}
				if (err == STATUS_DRIVE_IN_USE && retry > 0) {
					sam_syslog(LOG_INFO,
					    "%s: retrying dismount,"
					    " STATUS_DRIVE_IN_USE",
					    ent_pnt);
					retry--;
					free(stk_info);
					mutex_destroy(&force->mutex);
					cond_destroy(&force->condit);
					free(force);
					sleep(10);
					goto retry;
				}
			}
		} else {
			sam_syslog(LOG_INFO,
			    "%s:%d,%d,%d,%d acs response status:%s",
			    ent_pnt, DRIVE_LOC(drive->drive_id),
			    sam_acs_status(err));
		}
	} else {
		if (err > STATUS_LAST) {
			err = err - STATUS_LAST;
			sam_syslog(LOG_INFO, "helper-%s(errno):%m.", ent_pnt);
		} else {
			sam_syslog(LOG_INFO, "helper-%s:status:%s",
			    ent_pnt, sam_acs_status(err));
		}
	}

out:

	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "Return from %s (%#x).", ent_pnt, err);

	free(stk_info);
	mutex_destroy(&force->mutex);
	cond_destroy(&force->condit);
	free(force);
	return (err);
}


/*
 *	dismount_media - unload a volser
 * exit -
 *	  0 - ok
 *	  1 - not ok
 *
 */
int
dismount_media(
	library_t *library,	/* pointer to the library */
	drive_state_t *drive,	/* drive */
	int *stk_error)
{
	int 	retry, err;
	char 	*ent_pnt = "dismount_media";
	xport_state_t 	*transport;
	robo_event_t 	*dismount, *tmp;
	stk_information_t 	*stk_info;

	retry = 1;

retry:

	stk_info = malloc_wait(sizeof (stk_information_t), 2, 0);
	memset(stk_info, 0, sizeof (stk_information_t));
	stk_info->drive_id = drive->drive_id;
	memcpy((void *)&stk_info->vol_id.external_label[0],
	    (void *)&drive->bar_code, EXTERNAL_LABEL_SIZE);


	/* Build transport thread request */
	dismount = get_free_event(library);
	(void) memset(dismount, 0, sizeof (robo_event_t));

	dismount->request.internal.command = ROBOT_INTRL_DISMOUNT_MEDIA;
	dismount->request.internal.address = (void *)stk_info;

	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG, "%s: from %s.", ent_pnt, drive->un->name);

	dismount->type = EVENT_TYPE_INTERNAL;
	dismount->status.bits = REST_SIGNAL;
	dismount->completion = REQUEST_NOT_COMPLETE;

	transport = library->transports;
	mutex_lock(&dismount->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0) {
		transport->first = dismount;
	} else {
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
	*stk_error = err = dismount->completion;
	if (err == STATUS_SUCCESS) {
		stk_resp_acs_t *acs_resp =
		    (stk_resp_acs_t *)&
		    dismount->request.message.param.start_of_request;
		ACS_DISMOUNT_RESPONSE *dr = &acs_resp->data.dismount;

		/* Check the api_status before proceeding. */
		if (acs_resp->hdr.api_status != STATUS_SUCCESS) {
			LogApiStatus(ent_pnt, acs_resp->hdr.api_status);
			goto out;
		}
		/* Get acs_response status */
		*stk_error = err = acs_resp->hdr.acs_status;
		if (err == STATUS_SUCCESS) {
			/* Get status of the dismount command */
			*stk_error = err = dr->dismount_status;
			if (err != STATUS_SUCCESS) {
				sam_syslog(LOG_INFO,
				    "%s:%d,%d,%d,%d:returned:%s", ent_pnt,
				    DRIVE_LOC(drive->drive_id),
				    sam_acs_status(err));

				if (err == STATUS_LSM_OFFLINE) {
					down_library(library, SAM_STATE_CHANGE);
				}

				if (err == STATUS_DRIVE_IN_USE && retry > 0) {
					sam_syslog(LOG_INFO,
					    "%s: retrying dismount,"
					    " STATUS_DRIVE_IN_USE",
					    ent_pnt);
					retry--;
					free(stk_info);
					mutex_destroy(&dismount->mutex);
					cond_destroy(&dismount->condit);
					free(dismount);
					sleep(10);
					goto retry;
				}
			}
		} else {
			sam_syslog(LOG_INFO,
			    "%s:%d,%d,%d,%d acs response status:%s",
			    ent_pnt, DRIVE_LOC(drive->drive_id),
			    sam_acs_status(err));
		}
	} else {
		if (err > STATUS_LAST) {
			err = err - STATUS_LAST;
			sam_syslog(LOG_INFO, "helper-%s(errno):%m.", ent_pnt);
		} else {
			sam_syslog(LOG_INFO,
			    "helper-%s status:%s", ent_pnt,
			    sam_acs_status(err));
		}
	}

out:

	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "Return from %s (%#x).", ent_pnt, err);

	free(stk_info);
	mutex_destroy(&dismount->mutex);
	cond_destroy(&dismount->condit);
	free(dismount);
	return (err);
}


/*
 *	load_media - load media into a drive
 * exit -
 *	 0 - ok
 *	 1 - not ok
 */
int
load_media(
	library_t *library,
	drive_state_t *drive,
	struct CatalogEntry *ce,
	int *stk_error)
{
	int 		err;
	char 		*ent_pnt = "load_media";
	xport_state_t	*transport;
	robo_event_t 	*load, *tmp;
	stk_information_t 	*stk_info;

	stk_info = malloc_wait(sizeof (stk_information_t), 2, 0);
	memset(stk_info, 0, sizeof (stk_information_t));
	stk_info->drive_id = drive->drive_id;
	memcpy((void *)&stk_info->vol_id.external_label[0],
	    ce->CeBarCode, EXTERNAL_LABEL_SIZE);
	memcpy((void *)&drive->bar_code, ce->CeBarCode, EXTERNAL_LABEL_SIZE);

	/* Build transport thread request */

	load = get_free_event(library);
	(void) memset(load, 0, sizeof (robo_event_t));
	load->request.internal.command = ROBOT_INTRL_LOAD_MEDIA;
	load->request.internal.address = (void *)stk_info;

	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG, "%s: from %s to %s.", ent_pnt,
		    stk_info->vol_id.external_label, drive->un->name);

	load->type = EVENT_TYPE_INTERNAL;
	load->status.bits = REST_SIGNAL;
	load->completion = REQUEST_NOT_COMPLETE;

	transport = library->transports;
	mutex_lock(&load->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0) {
		transport->first = load;
	} else {
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

	*stk_error = err = load->completion;
	if (err == STATUS_SUCCESS) {
		stk_resp_acs_t *acs_resp =
		    (stk_resp_acs_t *)&
		    load->request.message.param.start_of_request;
		ACS_MOUNT_RESPONSE *mr = &acs_resp->data.mount;


		/* Check the api_status before proceeding. */
		if (acs_resp->hdr.api_status != STATUS_SUCCESS) {
			LogApiStatus(ent_pnt, acs_resp->hdr.api_status);
			goto out;
		}

		/* Get acs_response status */
		*stk_error = err = acs_resp->hdr.acs_status;
		if (err == STATUS_SUCCESS) {
			/* Get status of the mount command */
			*stk_error = err = mr->mount_status;
			if (err != STATUS_SUCCESS) {
				sam_syslog(LOG_INFO,
				    "%s:%d,%d,%d,%d:returned:%s", ent_pnt,
				    DRIVE_LOC(drive->drive_id),
				    sam_acs_status(err));
				if (err == STATUS_LSM_OFFLINE) {
					down_library(library, SAM_STATE_CHANGE);
				}
			} else {
				drive->un->dt.tp.default_capacity
				    = ce->CeCapacity;
			}
		} else {
			sam_syslog(LOG_INFO,
			    "%s:%d,%d,%d,%d acs response status:%s",
			    ent_pnt, DRIVE_LOC(drive->drive_id),
			    sam_acs_status(err));
		}
	} else {
		if (err > STATUS_LAST) {
			err = err - STATUS_LAST;
			sam_syslog(LOG_INFO, "helper-%s(errno):%m.", ent_pnt);
		} else {
			sam_syslog(LOG_INFO, "helper-%s status:%s",
			    ent_pnt, sam_acs_status(err));
		}
	}

out:

	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "Return from %s (%#x).", ent_pnt, err);

	free(stk_info);
	mutex_destroy(&load->mutex);
	cond_destroy(&load->condit);
	free(load);
	return (err);
}


/*
 * onestep_eject - eject volume to cap
 * exit -
 *	0 - ok
 *	1 - not ok
 */
int
onestep_eject(
	library_t *library,
	struct CatalogEntry *ce,
	int *stk_error)
{
	int 	err;
	VOLID 	*vol_id;
	char 	*ent_pnt = "onestep_eject";
	xport_state_t 	*transport;
	robo_event_t 	*eject, *tmp;

	vol_id = malloc_wait(sizeof (VOLID), 2, 0);
	strncpy((void *)vol_id, ce->CeBarCode, sizeof (VOLID));

	/* Build transport thread request */
	eject = get_free_event(library);
	(void) memset(eject, 0, sizeof (robo_event_t));
	eject->request.internal.command = ROBOT_INTRL_EJECT_MEDIA;
	eject->request.internal.address = (void *)vol_id;

	if (DBG_LVL(SAM_DBG_TMOVE)) {
		sam_syslog(LOG_DEBUG, "%s: %s.", ent_pnt, vol_id);
	}
	eject->type = EVENT_TYPE_INTERNAL;
	eject->status.bits = REST_SIGNAL;
	eject->completion = REQUEST_NOT_COMPLETE;

	transport = library->transports;
	mutex_lock(&eject->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0) {
		transport->first = eject;
	} else {
		LISTEND(transport, tmp);
		append_list(tmp, eject);
	}

	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);

	while (eject->completion == REQUEST_NOT_COMPLETE) {
		cond_wait(&eject->condit, &eject->mutex);
	}

	mutex_unlock(&eject->mutex);

	if ((err = eject->completion) == STATUS_SUCCESS) {
		stk_resp_acs_t *acs_resp =
		    (stk_resp_acs_t *)&
		    eject->request.message.param.start_of_request;
		SAM_ACS_EJECT_RESPONSE *ej = &acs_resp->data.eject;

		/* First check the api_status */
		if (acs_resp->hdr.api_status != STATUS_SUCCESS) {
			LogApiStatus(ent_pnt, acs_resp->hdr.api_status);
			goto out;
		}
		/* Now get the acs_response status */
		*stk_error = err = acs_resp->hdr.acs_status;
		if (err == STATUS_SUCCESS) {

			/* Now get the status of the eject */
			*stk_error = err = ej->eject_status;
			if (err == STATUS_SUCCESS) {

				/* Now get the status of the volume */
				*stk_error = err = ej->vol_status[1];
				if (err == STATUS_SUCCESS) {
					struct VolId vid;

					memset(&vid, 0, sizeof (struct VolId));
					CatalogExport(CatalogVolIdFromCe(
					    ce, &vid));
				} else {
					sam_syslog(LOG_INFO, "%s returned:%s",
					    ent_pnt,
					    sam_acs_status(err));
					if (err == STATUS_LSM_OFFLINE) {
						down_library(library,
						    SAM_STATE_CHANGE);
					}
				}
			} else {
				sam_syslog(LOG_INFO, "%s status:%s", ent_pnt,
				    sam_acs_status(err));
			}
		} else {
			sam_syslog(LOG_INFO, "%s response status:%s", ent_pnt,
			    sam_acs_status(err));
		}
	} else {
		if (err > STATUS_LAST) {
			err = err - STATUS_LAST;
			sam_syslog(LOG_INFO,
			    "helper-%s status(errno):%m.", ent_pnt);
		} else {
			sam_syslog(LOG_INFO, "helper-%s status:%s", ent_pnt,
			    sam_acs_status(err));
		}
	}

out:
	if (DBG_LVL(SAM_DBG_DEBUG)) {
		sam_syslog(LOG_DEBUG, "Return from %s (%#x).", ent_pnt, err);
	}
	mutex_destroy(&eject->mutex);
	cond_destroy(&eject->condit);
	free(eject);
	free(vol_id);
	return (err);
}


/*
 *	view_media - view a database entry
 *
 * exit -
 *	 0 - ok
 *	 1 - not ok
 */
int
view_media(
	library_t *library,		/* library pointer */
	struct CatalogEntry *ce,	/* catalog entry */
	int *stk_error)
{
	int 	err;
	char 	*ent_pnt = "view_media";
	xport_state_t 	*transport;
	robo_event_t 	*view, *tmp;
	VOLID 	*vol_id;

	vol_id = malloc_wait(sizeof (VOLID), 2, 0);
	strncpy((void *)vol_id, ce->CeBarCode, sizeof (VOLID));

	/* Build transport thread request */
	view = get_free_event(library);
	(void) memset(view, 0, sizeof (robo_event_t));
	view->request.internal.command = ROBOT_INTRL_VIEW_DATABASE;
	view->request.internal.address = (void *)vol_id;

	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG, "%s: %s.", ent_pnt, vol_id);

	view->type = EVENT_TYPE_INTERNAL;
	view->status.bits = REST_SIGNAL;
	view->completion = REQUEST_NOT_COMPLETE;

	transport = library->transports;
	mutex_lock(&view->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0) {
		transport->first = view;
	} else {
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

	if ((err = view->completion) == STATUS_SUCCESS) {
		stk_resp_acs_t *acs_resp =
		    (stk_resp_acs_t *)
		    &view->request.message.param.start_of_request;
		SAM_ACS_QUERY_VOL_RESPONSE *qr = &acs_resp->data.query;

		/* Check the api_status before proceeding. */
		if (acs_resp->hdr.api_status != STATUS_SUCCESS) {
			LogApiStatus(ent_pnt, acs_resp->hdr.api_status);
			goto out;
		}
		/* Get the acs_response status */
		*stk_error = err = acs_resp->hdr.acs_status;
		if (err == STATUS_SUCCESS) {
			/* Now get the status of the query command	*/
			*stk_error = err = qr->query_vol_status;
			if (err == STATUS_SUCCESS) {
				/* Dig out the status of the volume itself */
				*stk_error = err = qr->vol_status[0].status;
				if (err == STATUS_VOLUME_HOME ||
				    err == STATUS_VOLUME_IN_DRIVE ||
				    err == STATUS_VOLUME_IN_TRANSIT) {
					int m = qr->vol_status[0].media_type;

					err = 0;
					if (DBG_LVL(SAM_DBG_DEBUG))
						sam_syslog(LOG_INFO,
						    "%s:media type %#x.",
						    ent_pnt, m);

					/*
					 * There is a special case for the 3480/
					 * 3490/3490E.
					 * ACS returns a type of zero for all
					 * three. Look to see if the temporary
					 * bit is set in the catalog. If
					 * so, this will override the setting
					 * of the value from the lookup table
					 * and the assumption will be made that
					 * the capacity has been set by the
					 * operator.
					 */
					if ((m == 0) &&
					    (ce->CeStatus &
					    CES_capacity_set)) {
						if (DBG_LVL(SAM_DBG_DEBUG))
							sam_syslog(LOG_DEBUG,
							"%s:capacity from"
							" catalog %#x",
							    ent_pnt,
							    ce->CeCapacity);
					} else if (m < 0) {
						/*
						 * Set capacity to 0 if error
						 * occurs and warn user.
						 */
						(void) CatalogSetFieldByLoc(
						    ce->CeEq, ce->CeSlot,
						    ce->CePart,
						    CEF_Capacity, 0, 0);
						switch (m) {
						case -1:
							sam_syslog(LOG_INFO,
							"%s:Media type based on"
							" user preferences(%d)",
							    ent_pnt, m);
						case -2:
							sam_syslog(LOG_INFO,
							"%s:Media type ignored"
							" for this request(%d)",
							    ent_pnt, m);
						case -3:
							sam_syslog(LOG_INFO,
							"%s:Unknown media type"
							" for this request(%d)",
							    ent_pnt, m);
						}
					} else if (m < library->
					    media_capacity.count &&
					    *(library->
					    media_capacity.capacity
					    + m)) {
						/*
						 * Only use capacity from  stk
						 * index if there is no
						 * capacity set at all.
						 */
						if (ce->CeCapacity == 0) {
			(void) CatalogSetFieldByLoc(ce->CeEq, ce->CeSlot,
			    ce->CePart, CEF_Capacity,
			    *(library-> media_capacity.capacity + m), 0);
							/*
							 * Set space as this
							 * must be newly
							 * imported media.
							 */
			(void) CatalogSetFieldByLoc(ce->CeEq, ce->CeSlot,
			    ce->CePart, CEF_Space,
			    *(library->media_capacity.capacity + m), 0);
						}
						if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG,
			    "%s:setting capacity from table %#x", ent_pnt,
			    *(library->media_capacity.capacity + m));
					} else {
						(void) CatalogSetFieldByLoc(
						    ce->CeEq, ce->CeSlot,
						    ce->CePart,
						    CEF_Capacity, 0, 0);
						sam_syslog(LOG_INFO,
						    "%s:Media type (%d)"
						    " not defined in"
						    " media index",
						    ent_pnt, m);
					}
				} else {
					sam_syslog(LOG_INFO, "%s returned:%s",
					    ent_pnt, sam_acs_status(err));
					if (err == STATUS_LSM_OFFLINE) {
						down_library(library,
						    SAM_STATE_CHANGE);
					}
				}
			} else {
				sam_syslog(LOG_INFO, "%s status:%s", ent_pnt,
				    sam_acs_status(err));
			}
		} else {
			sam_syslog(LOG_INFO, "%s response status:%s", ent_pnt,
			    sam_acs_status(err));
		}
	} else {
		if (err > STATUS_LAST) {
			err = err - STATUS_LAST;
			sam_syslog(LOG_INFO,
			    "helper-%s status(errno):%m.", ent_pnt);
		} else {
			sam_syslog(LOG_INFO, "helper-%s status:%s", ent_pnt,
			    sam_acs_status(err));
		}
	}

out:

	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "Return from %s (%#x).", ent_pnt, err);

	mutex_destroy(&view->mutex);
	cond_destroy(&view->condit);
	free(view);
	free(vol_id);
	return (err);
}


/*
 *	get_media - get the media mounted.
 *
 * entry   -
 *		   drive->mutex should be held.	 This mutex
 *		   can be released during processing, but will be held
 *		   on return.
 *
 * returns -
 *		   0 - ok (RET_GET_MEDIA_SUCCESS)
 *		   1 - some sort of error, dispose of event
 *	(RET_GET_MEDIA_DISPOSE)
 *		   2 - event was requeued.
 *	(RET_GET_MEDIA_REQUEUED)
 *		   -1 - same as 1 except that the ACS library returned an error.
 *	(RET_GET_MEDIA_DOWN_DRIVE)
 *	  -2 - dispose, error, return error
 *	(RET_GET_MEDIA_RET_ERROR)
 *
 *		   in all cases, dev_ent activity count will be incremented.
 */
int
get_media(
	library_t *library,
	drive_state_t *drive,
	robo_event_t *event,	/* the event (can be NULL) */
	struct CatalogEntry *ce)
{
	int 		vol_status = 0, stk_error;
	char 		*d_mess = drive->un->dis_mes[DIS_MES_NORM];
	char 		*ent_pnt = "get_media";
	dev_ent_t 	*un = drive->un;
	int 		status = 0;

	if (ce == NULL) {
		sam_syslog(LOG_DEBUG, "load failed.");
		sprintf(d_mess, "mount failed.");
		return (RET_GET_MEDIA_DISPOSE);
	}

	mutex_lock(&un->mutex);
	un->active++;
	mutex_unlock(&un->mutex);

	/* Is the media already mounted */
	if (drive->status.b.full && (un->slot == ce->CeSlot)) {
		if (DBG_LVL(SAM_DBG_TMOVE))
			sam_syslog(LOG_DEBUG, "%s:(%d)%s:%d.",
			    ent_pnt, un->eq, __FILE__, __LINE__);
	} else {
		/*
		 * Make sure the source storage element has media in it.
		 * If the element is empty, external requests
		 * will be put back on the library's work list with the hope
		 * that it will be picked up later. Internal
		 * requests will return an error to the caller. If the in_use
		 * flag is not set, then the slot is
		 * "really" empty and the request will be disposed of.
		 */
		if (DBG_LVL(SAM_DBG_TMOVE))
			sam_syslog(LOG_DEBUG, "%s:(%d)%s:%d.", ent_pnt,
			    un->eq, __FILE__, __LINE__);

		/*
		 * Query ACSLS to find the volume if the slot is not occupied.
		 * If the volume is in the slot, reset the occupy bit.
		 */
		if (!(ce->CeStatus & CES_occupied)) {
			(void) view_media(library, ce, &vol_status);
			sam_syslog(LOG_DEBUG,
			    "query volume status: %s, %s:(%d)%s:%d.",
			    sam_acs_status(vol_status), ent_pnt, un->eq,
			    __FILE__, __LINE__);
			switch (vol_status) {

			case STATUS_VOLUME_HOME:
				status |= CES_occupied;
				(void) CatalogSetFieldByLoc(library->un->eq,
				    ce->CeSlot, 0, CEF_Status, status, 0);
				break;

			case STATUS_VOLUME_IN_DRIVE:
					sam_syslog(LOG_DEBUG,
					"%s: Volume in drive (%d) type %d.",
					    ent_pnt, un->eq, event->type);
			default:
				if (event != NULL &&
				    event->type != EVENT_TYPE_INTERNAL &&
				    !event->status.b.dont_reque) {

					add_to_end(library, event);
					return (RET_GET_MEDIA_REQUEUED);
				} else {
					sam_syslog(LOG_DEBUG,
					    "%s: (%d) type %d.",
					    ent_pnt, un->eq,
					    event->type);
					return (RET_GET_MEDIA_DISPOSE);
				}
				break;
			}
		}

		if (!(ce->CeStatus & CES_occupied)) {
			if ((ce->CeStatus & CES_inuse) && event != NULL &&
			    event->type != EVENT_TYPE_INTERNAL &&
			    !event->status.b.dont_reque) {

				event->next = NULL;

				/* Do not dispose of event */
				add_to_end(library, event);
				return (RET_GET_MEDIA_REQUEUED);
			}

			if (DBG_LVL(SAM_DBG_TMOVE))
				sam_syslog(LOG_DEBUG, "%s:(%d)%s:%d.",
				    ent_pnt, un->eq,
				    __FILE__, __LINE__);
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
			if (*un->i.ViMtype != '\0')
				un->i.ViFlags |= VI_mtype;
			memmove(un->i.ViVsn, un->vsn, sizeof (un->i.ViVsn));
			if (*un->i.ViVsn != '\0')
				un->i.ViFlags |= VI_vsn;
			un->i.ViEq = un->fseq;
			un->i.ViSlot = un->slot;
			un->i.ViPart = 0;
			if (un->i.ViSlot != ROBOT_NO_SLOT)
				un->i.ViFlags |= VI_part;

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
			if (dismount_media(library, drive, &stk_error) &&
			    stk_error != STATUS_IPC_FAILURE) {
				if (DBG_LVL(SAM_DBG_TMOVE))
					sam_syslog(LOG_DEBUG,
					"%s:(%d)%s:%d.", ent_pnt, un->eq,
					    __FILE__, __LINE__);

				/* indicate drive down */
				return (RET_GET_MEDIA_DOWN_DRIVE);
			}
			/*
			 * If failure and ipc failure, look to see if the media
			 * was put away, if so, then proceed as if it worked.
			 */
			if (stk_error == STATUS_IPC_FAILURE) {
				int stk_error2;
				struct CatalogEntry old_ced;
				struct CatalogEntry *old_ce = &old_ced;

				old_ce = CatalogGetEntry(&un->i, &old_ced);

				if (old_ce != NULL) {
					view_media(library,
					    old_ce, &stk_error2);
					if (stk_error2 != STATUS_VOLUME_HOME &&
					    stk_error2 !=
					    STATUS_VOLUME_IN_TRANSIT) {
						sam_syslog(LOG_DEBUG,
						"query volume status:"
						" %s, %s:(%d)%s:%d.",
						    sam_acs_status(vol_status),
						    ent_pnt,
						    un->eq, __FILE__, __LINE__);

						return
						    (RET_GET_MEDIA_DOWN_DRIVE);
					}
				}
			}

			CatalogVolumeUnloaded(&un->i, "");
			memset(&un->vsn, 0, sizeof (vsn_t));

			mutex_lock(&un->mutex);
			un->slot = ROBOT_NO_SLOT;
			un->mid = un->flip_mid = ROBOT_NO_SLOT;
			un->label_time = 0;
			mutex_unlock(&un->mutex);
			drive->status.b.full = FALSE;
			drive->status.b.bar_code = FALSE;
		}
		sprintf(d_mess, "mount %s", ce->CeVsn);
		if (load_media(library, drive, ce, &stk_error)) {
			/*
			 * Process error and return status to caller.
			 * Requeue the event if not internal otherwise
			 * return status to caller.
			 */
			int err;
			int ret = RET_GET_MEDIA_DOWN_DRIVE;
			int drive_status, drive_state;
			struct VolId vid;

			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG, "load failed.");
			if (DBG_LVL(SAM_DBG_TMOVE))
				sam_syslog(LOG_DEBUG,
				"%s:(%d)%s:%d.", ent_pnt, un->eq,
				    __FILE__, __LINE__);

			sprintf(d_mess, "mount of %s failed.", ce->CeVsn);

			/*
			 * Attempt some sort of recovery:
			 * Check state of  volume itself, if home or in transit,
			 * set occupied bit in catalog and requeue the request.
			 */
			vol_status = 0;
			(void) view_media(library, ce, &vol_status);

			/*
			 * Trace result of the query volume for easy debugging.
			 */
			sam_syslog(LOG_DEBUG,
			    "query volume status: %s, %s:(%d)%s:%d.",
			    sam_acs_status(vol_status), ent_pnt,
			    un->eq, __FILE__, __LINE__);

			switch (vol_status) {

			case STATUS_VOLUME_HOME:
			case STATUS_VOLUME_IN_TRANSIT:
				status |= CES_occupied;
				(void) CatalogSetFieldByLoc(
				    library->un->eq, ce->CeSlot, 0,
				    CEF_Status, status, 0);

				/* If internal event, then don't requeue it */
				if (event == NULL ||
				    event->type == EVENT_TYPE_INTERNAL) {

					ret = RET_GET_MEDIA_DISPOSE;
				} else {
					event->next = NULL;
					add_to_end(library, event);
					ret = RET_GET_MEDIA_REQUEUED;
				}
				break;

			case STATUS_VOLUME_IN_DRIVE:
				ret = RET_GET_MEDIA_RET_ERROR;
				break;

			case STATUS_VOLUME_NOT_IN_LIBRARY:
			case STATUS_VOLUME_ABSENT:
				status |= CES_occupied;
				(void) CatalogSetFieldByLoc(library->un->eq,
				    ce->CeSlot, 0, CEF_Status, status, 0);
				memset(&vid, 0, sizeof (struct VolId));
				vid.ViEq = library->un->eq;
				vid.ViSlot = ce->CeSlot;
				vid.ViFlags = VI_cart;
				status = CatalogExport(&vid);
				ret = RET_GET_MEDIA_RET_ERROR;
				break;

			default:
				break;
			}

			/*
			 * The load failed so need to chck condition of drive.
			 * If not online and available, down the drive.
			 */
			err = query_drive(library, drive,
			    &drive_status, &drive_state);
			if (err == STATUS_SUCCESS &&
			    drive_state == STATE_ONLINE) {

				switch (drive_status) {
				case STATUS_DRIVE_AVAILABLE:
					break;

				case STATUS_DRIVE_IN_USE:
					/*
					 * Keep trying the drives in a round
					 * robin fashion.
					 */
					ret = RET_GET_MEDIA_RET_ERROR;
					DevLog(DL_ALL(8001), un->eq);
					break;

				default:
					if (ret > 0) {
						ret = -ret;
					}
					break;
				}
			} else {
				if (ret > 0)
					ret = -ret;
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
LogApiStatus(
	char *ent_ptr,
	int err)
{
	if (err > STATUS_LAST) {
		err = err - STATUS_LAST;
		sam_syslog(LOG_INFO,
		    "helper failed on %s request, error(%#x).",
		    ent_ptr, err);
	} else {
		sam_syslog(LOG_INFO,
		    "helper failed on %s request, error(%#x).",
		    ent_ptr, err);
	}
}
