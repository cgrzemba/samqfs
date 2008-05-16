/*
 * work_cmds1.c - command processors for the work list
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
#include "sam/nl_samfs.h"
#include "sony.h"
#include "aml/catlib.h"
#include "aml/catalog.h"

#pragma ident "$Revision: 1.30 $"

static char *_SrcFile = __FILE__;

/* globals */

extern shm_alloc_t master_shm, preview_shm;
extern char *sam_sony_status(int status);


/*
 * add_to_cat_req - add an entry to the catalog
 */
void
add_to_cat_req(library_t *library, robo_event_t *event)
{
	int		err, sony_error;
	char		*ent_pnt = "add_to_cat_req";
	uint32_t	status;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	struct VolId	vid;
	addcat_request_t *request =
	    &event->request.message.param.addcat_request;

	request->media = library->un->media;

	/* Check for duplicate before proceeding */
	ce = CatalogGetCeByBarCode(library->un->eq,
	    sam_mediatoa(library->un->media), request->vsn, &ced);
	if (ce != NULL) {
		sam_syslog(LOG_INFO, "%s: %s duplicate volume, import fails",
		    ent_pnt, request->vsn);
		disp_of_event(library, event, 0);
		return;
	}

	status = 0;
	status |= (CES_inuse | CES_occupied | CES_bar_code);
	vid.ViEq = library->un->eq;
	vid.ViSlot = ROBOT_NO_SLOT;
	memmove(vid.ViMtype, sam_mediatoa(request->media),
	    sizeof (vid.ViMtype));
	(void) CatalogSlotInit(&vid, status,
	    (library->status.b.two_sided) ? 2 : 0, request->vsn, "");

	ce = CatalogGetCeByBarCode(library->un->eq, vid.ViMtype,
	    request->vsn, &ced);
	if (ce == NULL) {
		sam_syslog(LOG_INFO, "%s: %s not found.",
		    ent_pnt, request->vsn);
		disp_of_event(library, event, 0);
		return;
	}

	if (((err = view_media(library, ce, &sony_error)) != 0)) {
		sam_syslog(LOG_INFO, "%s: view_media: failed:%s.", ent_pnt,
		    sam_sony_status(sony_error));
		memset(&vid, 0, sizeof (struct VolId));
		CatalogExport(CatalogVolIdFromCe(ce, &vid));
		goto out;
	}

	vid.ViSlot = ce->CeSlot;
	vid.ViPart = 0;

	/*
	 * If the entry's capacity is zero, and not set to zero
	 * by the user, set the capacity to default.
	 */
	if ((!(ce->CeStatus & CES_capacity_set)) &&
	    (ce->CeCapacity == 0)) {
		(void) CatalogSetFieldByLoc(library->un->eq, vid.ViSlot,
		    vid.ViPart, CEF_Capacity,
		    DEFLT_CAPC(library->un->media), 0);
		(void) CatalogSetFieldByLoc(library->un->eq, vid.ViSlot,
		    vid.ViPart, CEF_Space,
		    DEFLT_CAPC(library->un->media), 0);
	}

	if (request->flags & CMD_ADD_VSN_AUDIT) {
		robo_event_t		*new_event;
		robot_internal_t	*request;

		status |= CES_needs_audit;
		(void) CatalogSetFieldByLoc(library->un->eq, vid.ViSlot,
		    vid.ViPart, CEF_Status, status, 0);
		new_event = get_free_event(library);
		request = &new_event->request.internal;
		(void) memset(new_event, 0, sizeof (robo_event_t));
		new_event->type = EVENT_TYPE_INTERNAL;
		new_event->status.bits = REST_FREEMEM;
		request->command = ROBOT_INTRL_AUDIT_SLOT;
		request->slot = ce->CeMid;
		request->flags.b.audit_eod = TRUE;
		add_to_end(library, new_event);
	} else if (request->flags & CMD_ADD_VSN_STRANGE) {
		status &= ~(CES_labeled | CES_needs_audit);
		status |= CES_non_sam;
		(void) CatalogSetFieldByLoc(library->un->eq, vid.ViSlot,
		    vid.ViPart, CEF_Status, status, 0);
	}
out:
	disp_of_event(library, event, 0);
}


/*
 * audit a slot
 * returns -
 *  0 - on success (request assigned to drive or audit started
 * !0 - No free drives.
 * Note: both search_drives and find_idle_drive return with the mutex locked
 * for the drive found.
 */
int
start_audit(library_t *library, robo_event_t *event, const int slot)
{
	char			*ent_pnt = "start_audit";
	uchar_t			audit_eod = 0;
	robo_event_t		canned_event, *new_event, *end;
	robot_internal_t	*intrl;
	drive_state_t		*drive;
	struct CatalogHdr *ch;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;

	if (event->type == EVENT_TYPE_INTERNAL)
		audit_eod = event->request.internal.flags.b.audit_eod;
	else
		audit_eod = (event->request.message.param.audit_request.flags
		    & AUDIT_FIND_EOD) != 0;
	(void) memset(&canned_event, 0, sizeof (canned_event));
	intrl = &canned_event.request.internal;

	canned_event.type = EVENT_TYPE_INTERNAL;
	canned_event.status.bits = REST_FREEMEM;
	intrl->flags.b.audit_eod = audit_eod;
	intrl->command = ROBOT_INTRL_START_AUDIT;

	/*
	 * Check if this audit is for the entire library
	 */
	if (slot == (unsigned)ROBOT_NO_SLOT) {
		sam_syslog(LOG_INFO, catgets
		    (catfd, SET, 9458, "Audit not supported on sony."));
		disp_of_event(library, event, 0);
		return (0);
	}

	ce = CatalogGetCeByLoc(library->un->eq, slot, 0, &ced);

	/*
	 * Audit a specific element
	 */
	if (slot == (unsigned)ROBOT_NO_SLOT) {
		sam_syslog(LOG_INFO, catgets(catfd, SET, 9090,
		    "%s: Slot (%d) is out of range."), ent_pnt, slot);
		disp_of_event(library, event, EFAULT);
		return (0);
	}

	ch = CatalogGetHeader(library->eq);
	library->audit_tab_len = ch->ChNumofEntries;

	/*
	 * Check if slot exists.
	 */
	if ((ce == NULL) || (slot > library->audit_tab_len)) {
		sam_syslog(LOG_INFO, catgets(catfd, SET, 9090,
		    "%s: Slot (%d) is out of range."), ent_pnt, slot);
		disp_of_event(library, event, EFAULT);
		return (0);
	}

	if (!(ce->CeStatus & CES_inuse)) {
		sam_syslog(LOG_INFO, catgets(catfd, SET, 9096,
		    "%s: Slot (%d) is not occupied."), ent_pnt, slot);
		disp_of_event(library, event, EFAULT);
		return (0);
	}

	if (ce->CeStatus & CES_non_sam) {
		sam_syslog(LOG_INFO, "%s: Slot(%d) is not SAM-FS media.",
		    ent_pnt, slot);
		disp_of_event(library, event, EFAULT);
		return (0);
	}

	/*
	 * Search drives to find an idle drive. find_idle_drive returns with
	 * un->mutex set.
	 */
	if ((drive = search_drives(library, slot)) == NULL)
		drive = find_idle_drive(library);
	if (drive == NULL)
		return (1);  /* no free drives */

	drive->un->status.b.requested = TRUE;
	mutex_unlock(&drive->un->mutex);

	/*
	 * Add the request to the drive's worklist
	 */
	intrl->slot = slot;
	new_event = get_free_event(library);
	*new_event = canned_event;
	mutex_lock(&drive->list_mutex);
	if (drive->active_count) {
		/* Append to the end of the list */
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

	/*
	 * Release the lock from search_drives or find_idle_drive
	 */
	drive->active_count++;
	mutex_unlock(&drive->mutex);
	cond_signal(&drive->list_condit);
	mutex_unlock(&drive->list_mutex);
	disp_of_event(library, event, 0);
	return (0);
}


/*
 * export_media - Remove an entry from the catalog
 */
void
export_media(library_t *library, robo_event_t *event)
{
	int			err = 0, sony_error;
	uint_t			slot;
	char			*ent_pnt = "export_media";
	struct VolId		vid;
	struct CatalogEntry	ced;
	struct CatalogEntry	*ce = &ced;
	drive_state_t		*drive = NULL;
	export_request_t *request =
	    &event->request.message.param.export_request;

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
				sam_syslog(LOG_INFO, catgets(catfd, SET, 9164,
				    "%s: vsn (%s) not found."),
				    ent_pnt, request->vsn);
				goto err;
			}
			break;

		case EXPORT_BY_SLOT:
			memset(&vid, 0, sizeof (struct VolId));
			vid.ViEq = request->eq;
			vid.ViSlot = request->slot;
			vid.ViFlags = VI_cart;
			ce = CatalogGetEntry(&vid, &ced);
			if (ce == NULL) {
				err = EFAULT;
				sam_syslog(LOG_INFO, catgets(catfd, SET, 9090,
				    "%s: Slot (%d) is out of range."),
				    ent_pnt, request->slot);
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
				sam_syslog(LOG_INFO, catgets(catfd, SET, 9165,
				    "%s: Device(%d) not found."),
				    ent_pnt, request->eq);
				goto err;
			}
			if (!drive->status.b.full) {
				err = ENOENT;
				sam_syslog(LOG_INFO, catgets(catfd, SET, 9166,
				    "%s: Device(%d) is empty."),
				    ent_pnt, request->eq);
				goto err;
			}
			if (drive->status.b.full &&
			    drive->un->slot != ROBOT_NO_SLOT)
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
			sam_syslog(LOG_INFO, "%s: Bad request", ent_pnt);
			goto err;
	}

	/*
	 * If not a drive export, make sure the slot is occupied.
	 * If not, look over the drives and find the media.
	 */
	if (drive == NULL && slot != ROBOT_NO_SLOT) {
		if (!(ce->CeStatus & CES_inuse)) {
			err = ENOENT;
			sam_syslog(LOG_INFO, catgets(catfd, SET, 9096,
			    "%s: Slot (%d) is not occupied."),
			    ent_pnt, ce->CeSlot);
			goto err;
		}
		if (!(ce->CeStatus & CES_occupied)) {
			for (drive = library->drive; drive != NULL;
			    drive = drive->next) {

				mutex_lock(&drive->mutex);
				if (drive->status.b.full &&
				    drive->un->slot == ce->CeSlot)
					break;
				mutex_unlock(&drive->mutex);
			}
			if (drive == NULL) {
				err = ENOENT;
				sam_syslog(LOG_INFO,
				    "%s: Unable to find media for slot %d.",
				    ent_pnt, ce->CeSlot);
				goto err;
			} else {
				mutex_lock(&drive->un->mutex);
				if (drive->un->active != 0) {
					mutex_unlock(&drive->un->mutex);
					mutex_unlock(&drive->mutex);
					err = EBUSY;
					sam_syslog(LOG_INFO,
					    catgets(catfd, SET, 9167,
					    "%s: Media from slot %d is"
					    " mounted and drive(%d) is"
					    " busy."), ent_pnt, ce->CeSlot,
					    drive->un->eq);
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
		if ((err = dismount_media(library, drive, &sony_error))) {
			disp_of_event(library, event, 1);
			return;
		}
		mutex_lock(&drive->un->mutex);
		drive->status.b.full = FALSE;
		drive->status.b.bar_code = FALSE;
		drive->un->slot = ROBOT_NO_SLOT;
		drive->un->mid = drive->un->flip_mid = ROBOT_NO_SLOT;
		drive->un->status.bits = 0;
		drive->un->status.b.present = 1;
		if (drive->open_fd >= 0)
			close_unit(drive->un, &drive->open_fd);
		mutex_unlock(&drive->un->mutex);
		mutex_unlock(&drive->mutex);
	}

	CatalogExport(&vid);

	disp_of_event(library, event, err);
	return;

err:
	disp_of_event(library, event, err);
}
