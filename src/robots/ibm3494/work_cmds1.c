/*
 *	work_cmds1.c - command processors for the work list
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
#include <synch.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/tapes.h"
#include "aml/shm.h"
#include "aml/robots.h"
#include "aml/trace.h"
#include "sam/defaults.h"
#include "sam/lib.h"
#include "aml/proto.h"
#include "ibm3494.h"
#include "aml/catlib.h"
#include "aml/catalog.h"

#pragma ident "$Revision: 1.33 $"

static char *_SrcFile = __FILE__;

/* globals */
extern shm_alloc_t master_shm, preview_shm;


/*
 *	add_to_cat_req - add an entry to the catalog
 *
 */
void
add_to_cat_req(
	library_t *library,
	robo_event_t *event)
{
	int 		err;
	uint32_t	status;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	struct VolId vid;
	IBM_query_info_t *query_data = NULL;
	addcat_request_t *request =
	    &event->request.message.param.addcat_request;
	ushort_t vol_cat;

	request->media = library->un->media;

	/* Check for duplicate before proceeding */
	ce = CatalogGetCeByBarCode(library->un->eq,
	    sam_mediatoa(library->un->media), request->vsn, &ced);
	if (ce != NULL) {
		sam_syslog(LOG_INFO, "%s duplicate volume, import fails",
		    request->vsn);
		disp_of_event(library, event, 0);
		return;
	}

	if (!library->options.b.shared) {
		sam_syslog(LOG_INFO, "Add media through the I/O ports.");
		disp_of_event(library, event, 0);
		return;
	}

	if ((err = view_media(library, request->vsn, (void *)&query_data))
	    != MC_REQ_OK) {
		sam_syslog(LOG_WARNING, "add to catalog failed(%d):%#x.",
		    library->un->eq, err);
		disp_of_event(library, event, 0);
		return;
	} else if (library->un->media == DT_3592 &&
	    (! IS_3592_MEDIA(query_data->data.expand_vol_data.vol_attr1))) {
		sam_syslog(LOG_WARNING,
		    "add to catalog failed(%d): %s is not a 3592",
		    library->un->eq, request->vsn);
		disp_of_event(library, event, 0);
		return;
	} else if (library->un->media == DT_3590 &&
	    (! IS_3590_MEDIA(query_data->data.expand_vol_data.vol_attr1))) {
		sam_syslog(LOG_WARNING,
		    "add to catalog failed(%d): %s is not a 3590",
		    library->un->eq, request->vsn);
		disp_of_event(library, event, 0);
		return;
	}

	status = 0;
	status |= (CES_inuse | CES_occupied | CES_bar_code);
	vid.ViEq = library->un->eq;
	vid.ViSlot = ROBOT_NO_SLOT;
	memmove(vid.ViMtype, sam_mediatoa(request->media),
	    sizeof (vid.ViMtype));

	ce = CatalogGetCeByBarCode(library->un->eq, vid.ViMtype,
	    request->vsn, &ced);

	if (ce != NULL) {
		sam_syslog(LOG_INFO, "Import errored, duplicate vsn");
		disp_of_event(library, event, 0);
		return;
	}
	if (CatalogSlotInit(&vid, status, (library->status.b.two_sided) ?
	    2 : 0, request->vsn, "")) {
		sam_syslog(LOG_INFO, "Import errored, not enough slots");
		disp_of_event(library, event, 0);
		return;
	}
	ce = CatalogGetCeByBarCode(library->un->eq, vid.ViMtype,
	    request->vsn, &ced);

	memcpy(&vol_cat, &query_data->data.expand_vol_data.cat_assigned[0],
	    sizeof (vol_cat));
	if (vol_cat != INSERT_CATEGORY && vol_cat != library->sam_category) {
		sam_syslog(LOG_INFO, "%s is not insert or %#x category.",
		    ce->CeVsn, library->sam_category);
	} else {
		set_media_category(library, ce->CeVsn, 0,
		    library->sam_category);
		/*
		 * If there is a catalog entry,
		 * and the entry's capacity is zero, and
		 * not set to zero by the user,
		 * set the capacity to default.
		 */
		if (ce &&
		    !(ce->CeStatus & CES_capacity_set) && !(ce->CeCapacity)) {
			(void) CatalogSetFieldByLoc(library->un->eq,
			    ce->CeSlot, 0, CEF_Capacity,
			    DEFLT_CAPC(library->un->media),
			    0);
			(void) CatalogSetFieldByLoc(library->un->eq,
			    ce->CeSlot, 0, CEF_Space,
			    DEFLT_CAPC(library->un->media),
			    0);
		}
	}

	if (request->flags & CMD_ADD_VSN_AUDIT) {
		robo_event_t *new_event;
		robot_internal_t *request;

		status = CES_needs_audit;
		CatalogSetFieldByLoc(ce->CeEq, ce->CeSlot, ce->CePart,
		    CEF_Status, status, 0);
		new_event = (robo_event_t *)
		    malloc_wait(sizeof (robo_event_t), 5, 0);
		request = &new_event->request.internal;
		(void) memset(new_event, 0, sizeof (robo_event_t));
		new_event->type = EVENT_TYPE_INTERNAL;
		new_event->status.bits = REST_FREEMEM;
		request->command = ROBOT_INTRL_AUDIT_SLOT;
		request->slot = ce->CeSlot;
		request->part = ce->CePart;
		request->flags.b.audit_eod = TRUE;
		add_to_end(library, new_event);
	} else if (request->flags & CMD_ADD_VSN_STRANGE) {
		status &= ~(CES_labeled | CES_needs_audit);
		status |= CES_non_sam;
		CatalogSetFieldByLoc(ce->CeEq, ce->CeSlot, ce->CePart,
		    CEF_Status, status, 0);
	}

	if (query_data != NULL)
		free(query_data);
	disp_of_event(library, event, 0);
}


/*
 *	audit a slot
 *
 * returns -
 *	0 - on success(request assigned to drive or audit started
 * !0 - No free drives.
 *
 * Note: both search_drives and find_idle_drive return with the mutex locked
 * for the drive found.
 */
int
start_audit(
	library_t *library,
	robo_event_t *event,
	const int slot)
{
	uchar_t		audit_eod = 0;
	robo_event_t 	canned_event, *new_event, *end;
	robot_internal_t *intrl;
	drive_state_t 	*drive;
	struct CatalogEntry ced;
	struct CatalogEntry *ce;

	if (event->type == EVENT_TYPE_INTERNAL)
		audit_eod = event->request.internal.flags.b.audit_eod;
	else
		audit_eod =
		    (event->request.message.param.audit_request.flags &
		    AUDIT_FIND_EOD) != 0;
	(void) memset(&canned_event, 0, sizeof (canned_event));
	intrl = &canned_event.request.internal;

	canned_event.type = EVENT_TYPE_INTERNAL;
	canned_event.status.bits = REST_FREEMEM;
	intrl->flags.b.audit_eod = audit_eod;
	intrl->command = ROBOT_INTRL_START_AUDIT;

	/* see if its the whole robot */
	if (slot == ROBOT_NO_SLOT) {
		sam_syslog(LOG_ERR, "Audit not supported on IBM3494.");
		disp_of_event(library, event, 0);
		return (0);
	}

	ce = CatalogGetCeByLoc(library->eq, slot, 0, &ced);

	/* Audit a specific element */
	if (ce == NULL) {
		sam_syslog(LOG_INFO,
		    "Audit request for slot outside of device (%d).",
		    library->eq);
		disp_of_event(library, event, EFAULT);
		return (0);
	}

	if (!(ce->CeStatus & CES_inuse)) {
		sam_syslog(LOG_INFO, "Audit request for empty slot-%d (%d).",
		    ce->CeSlot, library->eq);
		disp_of_event(library, event, EFAULT);
		return (0);
	}

	if (ce->CeStatus & CES_non_sam) {
		sam_syslog(LOG_INFO,
		    "Audit request for not SAM-FS media slot-%d.", ce->CeSlot);
		disp_of_event(library, event, EFAULT);
		return (0);
	}

	/* Search drives and find_idle_drive return with the un->mutex set */
	if ((drive = search_drives(library, ce->CeSlot)) == NULL)
		drive = find_idle_drive(library);

	if (drive == NULL)
		return (1);		/* no free drives */

	drive->un->status.b.requested = TRUE;
	mutex_unlock(&drive->un->mutex);

	/* add the request to the drives worklist */
	intrl->slot = slot;
	new_event = malloc_wait(sizeof (robo_event_t), 5, 0);
	*new_event = canned_event;
	mutex_lock(&drive->list_mutex);
	if (drive->active_count) {	/* append to the end of the list */
		LISTEND(drive, end);
		ETRACE((LOG_NOTICE, "ApDr(%d) %#x-%#x(%d) %s:%d.",
		    drive->un->eq, end, event,
		    drive->active_count, __FILE__, __LINE__));
		append_list(end, new_event);
	} else {
		ETRACE((LOG_NOTICE, "FrDr(%d) %#x(%d) %s:%d.",
		    drive->un->eq, event, drive->active_count,
		    __FILE__, __LINE__));
		drive->first = new_event;
	}

	drive->active_count++;

	mutex_unlock(&drive->mutex);	/* release lock from search_drives */
	/* or find_idle_drive */
	cond_signal(&drive->list_condit);
	mutex_unlock(&drive->list_mutex);

	disp_of_event(library, event, 0);
	return (0);			/* return success */
}


/*
 *	export_media - move media to the eject area.
 *
 */
void
export_media(
	library_t *library,
	robo_event_t *event)
{
	int 	err = 0;
	int	slot;
	char 	*message;
	drive_state_t *drive = NULL;
	export_request_t *request =
	    &event->request.message.param.export_request;
	struct VolId	vid;
	struct CatalogEntry ced;
	struct CatalogEntry *ce;

	/* find the media to export */

	switch (request->flags & EXPORT_FLAG_MASK) {
	case EXPORT_BY_VSN:

		memset(&vid, 0, sizeof (struct VolId));
		memmove(vid.ViMtype, sam_mediatoa(request->media),
		    sizeof (vid.ViMtype));
		memmove(vid.ViVsn, request->vsn, sizeof (vid.ViVsn));
		vid.ViFlags = VI_logical;
		ce = CatalogGetEntry(&vid, &ced);

		if (ce == NULL) {
			err = ENOENT;
			message = "export media - vsn not found";
			goto err;
		}
		break;

	case EXPORT_BY_SLOT:

			slot = request->slot;
		memset(&vid, 0, sizeof (struct VolId));
		vid.ViEq = request->eq;
		vid.ViSlot = request->slot;
		vid.ViFlags = VI_cart;
		ce = CatalogGetEntry(&vid, &ced);

		if (ce == NULL) {
			err = EFAULT;
			message = "export media - slot number out of range.";
			goto err;
		}
		break;

	case EXPORT_BY_EQ:

		for (drive = library->drive; drive != NULL;
		    drive = drive->next) {
			if (drive->un->eq == request->eq)
				break;
		}

		if (drive == NULL) {
			err = ENOENT;
			message = "export media - equipment not found";
			goto err;
		}

		if (!drive->status.b.full) {
			err = ENOENT;
			message = "export media - equipment not full";
			goto err;
		}

		if (drive->status.b.full && drive->un->slot != ROBOT_NO_SLOT)
			slot = drive->un->slot;
		else
			slot = ROBOT_NO_SLOT;

		mutex_lock(&drive->mutex);
		mutex_lock(&drive->un->mutex);
		drive->un->status.bits |= DVST_WAIT_IDLE;
		while (!drive_is_idle(drive)) {
			mutex_unlock(&drive->un->mutex);
			mutex_unlock(&drive->mutex);
			sleep(5);
			mutex_lock(&drive->mutex);
			mutex_lock(&drive->un->mutex);
		}

		drive->un->status.bits &= ~DVST_WAIT_IDLE;
		mutex_unlock(&drive->un->mutex);
		if (drive->un->status.b.ready)
			(void) spin_drive(drive, SPINDOWN, NOEJECT);
		break;

	default:
		err = EFAULT;
		message = "export_media: bad request";
		goto err;

	}

	/*
	 * If not a drive export, check to make sure the slot is occupied,
	 * if not, look over the drives and find the media
	 */
	if (drive == NULL && ce != NULL) {

		if (!(ce->CeStatus & CES_inuse)) {
			err = ENOENT;
			message = "export media - slot number not occupied.";
			goto err;
		}

		if (!(ce->CeStatus & CES_occupied)) {
			for (drive = library->drive; drive != NULL;
			    drive = drive->next) {
				mutex_lock(&drive->mutex);
				if (drive->status.b.full &&
				    drive->un->slot == slot)
					break;
				mutex_unlock(&drive->mutex);
			}

			if (drive == NULL)
				sam_syslog(LOG_INFO,
				    "Export media: Unable to find media for"
				    " slot %d.",
				    ce->CeSlot);
			else {

				mutex_lock(&drive->un->mutex);
				if (drive->un->active != 0) {
					mutex_unlock(&drive->un->mutex);
					mutex_unlock(&drive->mutex);
					err = EBUSY;
					message =
					    "export media - media in drive and"
					    " drive is busy.";
					goto err;
				}
				drive->un->status.b.requested = TRUE;
				mutex_unlock(&drive->un->mutex);

				if (drive->un->status.b.ready)
					(void) spin_drive(drive, SPINDOWN,
					    NOEJECT);
			}
		}
	}

	if (drive != NULL) {
		if (err = dismount_media(library, drive)) {
			disp_of_event(library, event, 1);
			return;
		}
		mutex_lock(&drive->un->mutex);
		drive->status.b.full = FALSE;
		drive->status.b.bar_code = FALSE;
		drive->un->status.bits =
		    (drive->un->status.bits & DVST_CLEANING);
		drive->un->status.b.present = 1;
		if (drive->open_fd >= 0)
			close_unit(drive->un, &drive->open_fd);
		drive->un->slot = ROBOT_NO_SLOT;
		drive->un->mid = drive->un->flip_mid = ROBOT_NO_SLOT;
		mutex_unlock(&drive->un->mutex);
		mutex_unlock(&drive->mutex);
	}

	err = set_media_category(library, ce->CeBarCode, 0, EJECT_CATEGORY);
		if (err == MC_REQ_OK) {
			CatalogExport(&vid);
		}

	disp_of_event(library, event, err);
	return;

err:
	sam_syslog(LOG_INFO, message);
	disp_of_event(library, event, err);
	return;

}
