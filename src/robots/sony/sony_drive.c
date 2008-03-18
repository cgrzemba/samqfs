/*
 * sony_drive.c - sony drive unique routines.
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

#pragma ident "$Revision: 1.32 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <synch.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/dev_log.h"
#include "sam/lib.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/odlabels.h"
#include "aml/tplabels.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "sam/defaults.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"
#include "sony.h"
#include "aml/trace.h"
#include "aml/proto.h"
#include "../common/drive.h"
#include "driver/samst_def.h"

/* some globals */

extern shm_alloc_t master_shm, preview_shm;
extern char *sam_sony_status(int status);


/*
 * audit - start auditing
 */
void
audit(drive_state_t *drive, const uint_t slot, const int audit_eod)
{
	int		err;
	char		*ent_pnt = "audit";
	dev_ent_t	*un = drive->un;
	robo_event_t	robo_event;
	sam_defaults_t	*defaults;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;

	defaults = GetDefaults();
	if (slot == ROBOT_NO_SLOT) {
		sam_syslog(LOG_ERR, catgets(catfd, SET, 9458,
		    "Audit not supported on sony."));
		return;
	}

	/*
	 * Set up dummy event for get_media, force no_requeue
	 */
	memset(&robo_event, 0, sizeof (robo_event_t));
	robo_event.status.b.dont_reque = TRUE;

	ce = CatalogGetCeByLoc(drive->library->un->eq, slot, 0, &ced);
	if (ce == NULL) {
		sam_syslog(LOG_INFO, catgets(catfd, SET, 9090,
		    "%s: Slot (%d) is out of range."), ent_pnt, slot);
		return;
	}

	mutex_lock(&drive->mutex);
	if (drive->status.b.full) {
		un->status.b.requested = TRUE;
		if (clear_drive(drive)) {
			mutex_unlock(&drive->mutex);
			return;
		}
		if (drive->open_fd >= 0) {
			mutex_lock(&un->mutex);
			close_unit(un, &drive->open_fd);
			mutex_unlock(&un->mutex);
		}
	}
	mutex_unlock(&drive->mutex);
	mutex_lock(&un->mutex);
	un->status.b.requested = TRUE;
	un->status.b.labeled = FALSE;
	un->status.b.ready = FALSE;
	mutex_unlock(&un->mutex);

	mutex_lock(&drive->mutex);
	if ((ce->CeStatus & CES_occupied) && !(ce->CeStatus & CES_non_sam)) {
		if ((err = get_media(drive->library, drive, &robo_event, ce))
		    != RET_GET_MEDIA_SUCCESS) {

			mutex_lock(&un->mutex);
			un->status.b.requested = FALSE;
			if (IS_GET_MEDIA_FATAL_ERROR(err)) {
				if (err == RET_GET_MEDIA_DOWN_DRIVE) {
					un->state = DEV_DOWN;
					sam_syslog(LOG_INFO,
					    catgets(catfd, SET, 9459,
					    "%s:Setting device (%d) down"
					    " due to PSC error."),
					    ent_pnt, un->eq);
					err = -err;
				}
			}
			DEC_ACTIVE(un);
			mutex_unlock(&un->mutex);
			mutex_unlock(&drive->mutex);
			return;
		}
		mutex_lock(&un->mutex);
		un->status.b.scanning = TRUE;
		mutex_unlock(&un->mutex);
		while (spin_drive(drive, SPINUP, NOEJECT)) {
			if (un->state > DEV_ON) {
				mutex_lock(&un->mutex);
				un->status.b.scanning = FALSE;
				mutex_unlock(&un->mutex);
				return;
			}
			sleep(2);
		}
		un->i.ViPart = ce->CePart;
		scan_a_device(un, drive->open_fd);
		mutex_lock(&un->mutex);
		if (!un->status.b.labeled && (ce->CeStatus & CES_bar_code) &&
		    (defaults->flags & DF_LABEL_BARCODE) && IS_TAPE(un)) {

			vsn_from_barcode(un->vsn, ce->CeBarCode, defaults, 6);
			un->status.b.labeled = TRUE;
			un->space = un->capacity;
		}
		if (audit_eod && un->status.b.labeled) {
			mutex_unlock(&un->mutex);
			mutex_lock(&un->io_mutex);
			tape_append(drive->open_fd, un, NULL);
			mutex_unlock(&un->io_mutex);
			mutex_lock(&un->mutex);
		}
		UpdateCatalog(un, 0, CatalogVolumeLoaded);
		close_unit(un, &drive->open_fd);
		DEC_ACTIVE(un);
		un->status.b.requested = TRUE;
		mutex_unlock(&un->mutex);
	}
	mutex_unlock(&drive->mutex);
	mutex_lock(&un->mutex);
	un->status.b.requested = FALSE;
	mutex_unlock(&un->mutex);
	mutex_lock(&drive->library->un->mutex);
	drive->library->un->status.b.mounted = TRUE;
	mutex_unlock(&drive->library->un->mutex);
}


/*
 * query_drive - get information for a drive.
 * exit -
 *   status of the query.  If > PSCERR_NO_ERROR, then helper returned error.
 */
int
query_drive(library_t *library, drive_state_t *drive, int *drive_status,
	int *drive_state)
{
	int			err;
	sony_information_t	*sony_info;
	robo_event_t		*query, *tmp;
	xport_state_t		*transport;

	sony_info = malloc_wait(sizeof (sony_information_t), 2, 0);
	memset(sony_info, 0, sizeof (sony_information_t));
	sony_info->PscBinInfo = drive->PscBinInfo;
	sony_info->BinNoChar  = drive->BinNoChar;
	/*
	 * Build the transport thread request
	 */
	query = get_free_event(library);
	(void) memset(query, 0, sizeof (robo_event_t));
	query->request.internal.command = ROBOT_INTRL_QUERY_DRIVE;
	query->request.internal.address = (void *)sony_info;
	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG,
		    "sony_query_drive: %s.", sony_info->BinNoChar);

	query->type = EVENT_TYPE_INTERNAL;
	query->status.bits = REST_SIGNAL;
	query->completion = REQUEST_NOT_COMPLETE;
	transport = library->transports;
	mutex_lock(&query->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0)
		transport->first = query;
	else {
		LISTEND(transport, tmp);
		append_list(tmp, query);
	}
	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);
	/*
	 * Wait for the transport to complete the query command
	 */
	while (query->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&query->condit, &query->mutex);
	mutex_unlock(&query->mutex);

	err = query->completion;
	if (err == PSCERR_NO_ERROR) {
		sony_resp_api_t *response = (sony_resp_api_t *)
		    &query->request.message.param.start_of_request;
		/*
		 * Get the API response status
		 */
		err = response->api_status;
		if (err == PSCERR_NO_ERROR) {
			err = *drive_status = response->data.BinInfo.ulStatus;
			*drive_state = 0;
			if (DBG_LVL(SAM_DBG_DEBUG)) {
				sam_syslog(LOG_DEBUG, "query drive (%d)",
				    response->data.BinInfo.wBinNo);
				sam_syslog(LOG_DEBUG, "query status (%08x)",
				    response->data.BinInfo.ulStatus);
			}

			if (strlen((void *)
			    &response->data.BinInfo.szCassetteId) != 0) {

				if (DBG_LVL(SAM_DBG_DEBUG))

			sam_syslog(LOG_DEBUG, "query drive (%d) has volume %s",
			    response->data.BinInfo.wBinNo,
			    response->data.BinInfo.szCassetteId);
				dtb((uchar_t *)
				    response->data.BinInfo.szCassetteId,
				    PSCCASSIDLEN);
				memcpy(drive->un->vsn,
				    response->data.BinInfo.szCassetteId,
				    sizeof (drive->un->i.ViVsn));
				memcpy(drive->bar_code,
				    response->data.BinInfo.szCassetteId,
				    PSCCASSIDLEN);
				memmove(drive->un->i.ViVsn,
				    response->data.BinInfo.szCassetteId,
				    sizeof (drive->un->i.ViVsn));
				memmove(drive->un->i.ViMtype,
				    sam_mediatoa(drive->un->equ_type),
				    sizeof (drive->un->i.ViMtype));
				drive->un->i.ViFlags = VI_logical;
			} else if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "query drive (%d) has no volume",
				    response->data.BinInfo.wBinNo);
		} else
			sam_syslog(LOG_ERR, "query drive response status:%s",
			    sam_sony_status(err));
	} else
		sam_syslog(LOG_INFO, "helper-query_drive status:%s",
		    sam_sony_status(err));
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "Return from query_drive (%#x).", err);
	mutex_destroy(&query->mutex);
	cond_destroy(&query->condit);
	free(query);
	free(sony_info);
	return (err);
}


/*
 * search_drives - search drives looking for a drive with slot
 * exit -
 *   drive_state_t * to drive with the slot (mutex locked)
 *   drive_state_t *NULL if not found.
 */
drive_state_t *
search_drives(library_t *library, uint_t slot)
{
	drive_state_t	*drive;

	for (drive = library->drive; drive != NULL; drive = drive->next) {
		if (drive->un == NULL)
			continue;
		if (drive->status.b.full && (drive->un->slot == slot)) {
			mutex_lock(&drive->mutex);
			mutex_lock(&drive->un->mutex);
			if (drive->status.b.full && (drive->un->slot == slot))
				return (drive);
			mutex_unlock(&drive->un->mutex);
			mutex_unlock(&drive->mutex);
		}
	}
	return (NULL);
}


/*
 * clean - clean the drive.
 */
void
clean(drive_state_t *drive, robo_event_t *event)
{
	int		err, retry, sony_error;
	dev_ent_t	*un = drive->un;
	library_t	*library = drive->library;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;

	mutex_lock(&drive->mutex);
	if (clear_drive(drive)) {
		mutex_lock(&drive->un->mutex);
		drive->un->status.bits |= DVST_CLEANING;
		mutex_unlock(&drive->un->mutex);
		mutex_unlock(&drive->mutex);
		disp_of_event(library, event, EAGAIN);
		return;
	}

	mutex_lock(&drive->un->mutex);
	drive->un->status.bits |= (DVST_REQUESTED | DVST_CLEANING);
	if (drive->un->open_count) {
		clear_driver_idle(drive, drive->open_fd);
		close_unit(drive->un, &drive->open_fd);
		DEC_OPEN(drive->un);
	}

	mutex_unlock(&drive->un->mutex);
	mutex_unlock(&drive->mutex);
	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG, "clean:(%d).", drive->un->eq);

	ce = CatalogGetCleaningVolume(library->un->eq, &ced);

	if (ce == NULL) {
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG,
			    "clean:(%d)-No cleaning cartridge found.",
			    drive->un->eq);

		mutex_lock(&drive->mutex);
		down_drive(drive, SAM_STATE_CHANGE);
		mutex_lock(&drive->un->mutex);
		drive->un->status.bits &= ~DVST_REQUESTED;
		mutex_unlock(&drive->un->mutex);
		disp_of_event(library, event, EAGAIN);
		mutex_unlock(&drive->mutex);
		return;
	}

	if (!(ce->CeStatus & CES_occupied)) {
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG, "clean:(%d)No cartridge in slot.",
			    drive->un->eq);
		mutex_lock(&drive->un->mutex);
		drive->un->status.bits &= ~DVST_REQUESTED;
		mutex_unlock(&drive->un->mutex);
		disp_of_event(library, event, ENOENT);
		return;
	}

	mutex_lock(&drive->mutex);

	if (load_media(library, drive, ce, &sony_error)) {
		sam_syslog(LOG_INFO,
		    "Unable to load cleaning cartridge(%d) slot %d.",
		    library->un->eq, ce->CeSlot);
		down_drive(drive, SAM_STATE_CHANGE);
		mutex_unlock(&drive->mutex);
		disp_of_event(library, event, EIO);
		return;
	}
	mutex_unlock(&drive->mutex);

	/*
	 * Log successful mount of cleaning tape
	 */
	DevLog(DL_ALL(10042), drive->un->eq);

	retry = 2;
	do {
		mutex_lock(&drive->mutex);
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG,
			    "Attempt to unload cleaning cartridge.");
		err = dismount_media(library, drive, &sony_error);
		if (DBG_LVL(SAM_DBG_DEBUG)) {
			if (err)
				sam_syslog(LOG_DEBUG,
				    "Unload cleaning failed - retrying.");
			else
				sam_syslog(LOG_DEBUG,
				    "Unload cleaning succeeded.");
		}
		mutex_unlock(&drive->mutex);
	} while (err != 0 && retry-- != 0);

	if (err != 0) {
		sam_syslog(LOG_INFO,
		    "Unable to unload cleaning cartridge(%d) slot %d.",
		    library->un->eq, ce->CeSlot);
		mutex_lock(&drive->mutex);
		drive->status.b.cln_inprog = FALSE;
		down_drive(drive, SAM_STATE_CHANGE);
		mutex_unlock(&drive->mutex);
		disp_of_event(library, event, EIO);
		return;
	}
	mutex_lock(&drive->mutex);
	drive->status.b.cln_inprog = FALSE;
	mutex_lock(&drive->un->mutex);
	drive->un->status.bits &= ~(DVST_CLEANING | DVST_REQUESTED);
	mutex_unlock(&drive->un->mutex);
	mutex_unlock(&drive->mutex);
	disp_of_event(library, event, 0);
}


boolean_t
drive_is_local(library_t *library, drive_state_t *drive)
{
	/*
	 * At this time this is a place holder only
	 * until passthru is supported by sony psc
	 */
	return (TRUE);
}

drive_state_t *
find_empty_drive(drive_state_t *drive)
{
	/*
	 * Place holder
	 */
	return (NULL);
}
