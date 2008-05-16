/*
 *	ibm349d_drive.c - ibm3494_drive unique routines.
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

static char *_SrcFile = __FILE__;

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
#include "sam/lib.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/odlabels.h"
#include "aml/tplabels.h"
#include "aml/historian.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "sam/defaults.h"
#include "ibm3494.h"
#include "aml/trace.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/proto.h"
#include "../common/drive.h"
#include "driver/samst_def.h"

#pragma ident "$Revision: 1.34 $"

/*	some globals */
extern shm_alloc_t master_shm, preview_shm;


/*
 *	audit - start auditing
 *
 *
 */
void
audit(
	drive_state_t *drive,
	const uint_t slot,
	const int audit_eod_flag)
{
	int 		err, audit_eod = audit_eod_flag;
	dev_ent_t 	*un = drive->un;
	library_t 	*library = drive->library;
	robo_event_t 	robo_event;
	sam_defaults_t 	*defaults;
	struct VolId	vid;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;

	defaults = GetDefaults();

	if (slot == ROBOT_NO_SLOT) {
		sam_syslog(LOG_INFO, "Audit not supported on ibm349x.");
		return;
	}

	memset(&vid, 0, sizeof (struct VolId));
	vid.ViPart = 0;
	vid.ViSlot = slot;
	vid.ViEq = drive->library->un->eq;
	vid.ViFlags = VI_cart;
	ce = CatalogGetEntry(&vid, &ced);
	if (ce == NULL)	{
		sam_syslog(LOG_INFO, "Audit not supported on ibm349x.");
		return;
	}

	/* Set up dummy event for get_media, force no_requeue */
	memset(&robo_event, 0, sizeof (robo_event_t));
	robo_event.status.b.dont_reque = TRUE;

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
			un->status.b.opened = (un->open_count != 0);
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
		if ((err = get_media(library, drive, &robo_event, ce))
		    != RET_GET_MEDIA_SUCCESS) {
			mutex_lock(&un->mutex);
			un->status.b.requested = FALSE;
			if (IS_GET_MEDIA_FATAL_ERROR(err)) {
				if (err == RET_GET_MEDIA_DOWN_DRIVE) {
					un->state = DEV_DOWN;
					sam_syslog(LOG_INFO, "Setting device"
					    " (%d) down, due to error.",
					    un->eq);
					err = -err;
				}
			}
			DEC_ACTIVE(un);
			mutex_unlock(&un->mutex);	/* if error */
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
		if (!(ce->CeStatus & CES_labeled) &&
		    (ce->CeStatus & CES_bar_code) &&
		    (defaults->flags & DF_LABEL_BARCODE) && IS_TAPE(un)) {
			vsn_from_barcode(un->vsn, ce->CeBarCode, defaults, 6);
			un->status.b.labeled = TRUE;
			un->space = un->capacity;
		}
		if (audit_eod && un->status.b.labeled) {
			sam_syslog(LOG_INFO, "Finding EOD on (%d:%s).",
			    un->eq, un->vsn);
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
	mutex_lock(&library->un->mutex);
	library->un->status.b.mounted = TRUE;
	mutex_unlock(&library->un->mutex);
}


/*
 *	query_drive - get information for a drive.
 *
 */
req_comp_t
query_drive(
	library_t *library,
	drive_state_t *drive,
	int *drive_status,
	int *drive_state)
{
	req_comp_t 	err;
	ibm_req_info_t 	*ibm_info;
	robo_event_t 	*query, *tmp;
	xport_state_t 	*transport;

	ibm_info = malloc_wait(sizeof (ibm_req_info_t), 2, 0);
	memset(ibm_info, 0, sizeof (ibm_req_info_t));
	ibm_info->drive_id = drive->drive_id;

	/* Build transport thread request */

	query = malloc_wait(sizeof (robo_event_t), 5, 0);
	(void) memset(query, 0, sizeof (robo_event_t));
	query->request.internal.command = ROBOT_INTRL_QUERY_DRIVE;
	query->request.internal.address = (void *)ibm_info;

	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "query_drive: %s.", drive->un->name);

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

	/* Wait for the transport to do the query */
	while (query->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&query->condit, &query->mutex);
	mutex_unlock(&query->mutex);

	err = (req_comp_t)query->completion;
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG,
		    "Return from query_drive (%#x).", query->completion);

	if (err == MC_REQ_OK) {
		IBM_query_info_t *q_info = ibm_info->ret_data;

		if (q_info == NULL)
			sam_syslog(LOG_INFO,
			    "Query drive failed to return data");
		else {
			char dev_state = q_info->data.dev_data.dev_state;
			ushort_t category;

			memcpy(&category, &q_info->data.dev_data.mnted_cat[0],
			    sizeof (category));
			if (dev_state & MT_INSTALLED &&
			    dev_state & MT_AVAILBLE) {
				if ((dev_state & MT_VOL_LOADED ||
				    q_info->data.dev_data.volser[0] != ' ') &&
				    category != CLEAN_CATEGORY) {
					mutex_lock(&drive->mutex);
					drive->status.b.full = TRUE;
					mutex_unlock(&drive->mutex);
					memmove(drive->un->i.ViVsn, (void *)
					    &q_info->data.dev_data.volser,
					    sizeof (drive->un->i.ViVsn));
					memmove(drive->un->i.ViMtype,
					    sam_mediatoa(drive->un->equ_type),
					    sizeof (drive->un->i.ViMtype));
					drive->un->i.ViFlags = VI_logical;
				}
			} else {
				sam_syslog(LOG_INFO,
				    "Library reports device %#x(%s) as not %s.",
				    drive->drive_id, drive->un->name,
				    (dev_state & MT_INSTALLED) ? "availble" :
				    "installed");
				err = MC_REQ_DD;
			}
			*drive_state = q_info->data.dev_data.dev_state;
		}
	}
	if (ibm_info->ret_data != NULL)
		free(ibm_info->ret_data);

	mutex_destroy(&query->mutex);
	free(query);
	free(ibm_info);
	return (err);
}


/*
 *	search_drives - search drives looking for a drive with slot
 *
 *
 * exit -
 *	 drive_state_t *to drive with the slot(mutex locked)
 *	 drive_state_t *NULL if not found.
 *
 */
drive_state_t *
search_drives(
	library_t *library,
	uint_t slot)
{
	drive_state_t *drive;

	for (drive = library->drive; drive != NULL; drive = drive->next) {
		if (drive->un == NULL)
			continue;

		if (drive->status.b.full &&	/* source full */
		    (drive->un->slot == slot)) {	/* matches slot */
			mutex_lock(&drive->mutex);
			mutex_lock(&drive->un->mutex);
			if (drive->status.b.full &&	/* source full */
			    (drive->un->slot == slot))
				return (drive);	/* matched */

			mutex_unlock(&drive->un->mutex);
			mutex_unlock(&drive->mutex);
		}
	}
	return (NULL);
}


/*
 *	clean - clean the drive.
 *
 */
void
clean(
	drive_state_t *drive,
	robo_event_t *event)
{
	library_t 	*library = drive->library;

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
	drive->status.b.cln_inprog = FALSE;
	mutex_lock(&drive->un->mutex);
	drive->un->status.bits &= ~(DVST_CLEANING | DVST_REQUESTED);
	mutex_unlock(&drive->un->mutex);
	mutex_unlock(&drive->mutex);
	disp_of_event(library, event, 0);
}


boolean_t
drive_is_local(
	library_t *library,
	drive_state_t *drive)
{
	/*
	 * At this time this is a place holder only until
	 * passthru is supported by ibm3494
	 */
	return (TRUE);
}

drive_state_t *
find_empty_drive(
	drive_state_t *drive)
{
	/*
	 * Place holder.
	 */
	return (NULL);
}
