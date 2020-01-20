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

#pragma ident "$Revision: 1.49 $"

static char *_SrcFile = __FILE__;

#include <stdio.h>
#include <thread.h>
#include <synch.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/dev_log.h"
#include "aml/shm.h"
#include "aml/robots.h"
#include "aml/trace.h"
#include "sam/defaults.h"
#include "sam/nl_samfs.h"
#include "generic.h"

#if SAM_OPEN_SOURCE
#include "derrno.h"
#endif

#include "sam/lib.h"
#include "aml/proto.h"
#include "aml/catalog.h"
#include "aml/catlib.h"

/*	Function prototypes */
void *unload_lib(void *);

/*	globals */
extern shm_alloc_t master_shm, preview_shm;
extern void set_operator_panel(library_t *library, const int lock);


/*
 * move_request - move media between two storage slots.
 * called as a thread.
 */
void *
move_request(
	void *vcmd)
{
	struct VolId	vid;
	uint_t		source, dest;
	uchar_t		just_flip = FALSE;
	struct CatalogEntry ce1d;
	struct CatalogEntry ce2d;
	struct CatalogEntry *ce1 = &ce1d, *ce2 = &ce2d;
	dev_ent_t 	*un;
	move_flags_t	flags;
	library_t 	*library = ((robot_threaded_cmd *)vcmd)->library;
	robo_event_t	*event = ((robot_threaded_cmd *)vcmd)->event;
	cmdmove_request_t *request =
	    &(event->request.message.param.cmdmove_request);

	un = library->un;
	if (library->un->equ_type == DT_DLT2700) {
		DevLog(DL_ERR(5094));
		disp_of_event(library, event, 0);
		thr_exit(NULL);
	}
	source = ELEMENT_ADDRESS(library, request->slot);
	dest = ELEMENT_ADDRESS(library, request->d_slot);
	if (source == dest) {
		if (library->status.b.two_sided) {
			just_flip = TRUE;
		} else {
			DevLog(DL_ERR(5096), source);
			disp_of_event(library, event, 0);
			thr_exit(NULL);
		}
	}
	ce1 = CatalogGetCeByLoc(library->un->eq, request->slot, 0, &ce1d);
	ce2 = CatalogGetCeByLoc(library->un->eq, request->d_slot, 0, &ce2d);
	if ((ce1 == NULL) || (!(ce1->CeStatus & CES_occupied))) {
		DevLog(DL_ERR(5097), request->slot);
		disp_of_event(library, event, 0);
		thr_exit(NULL);
	}
	if ((ce2 != NULL) &&
	    ((ce2->CeStatus & CES_occupied) || (ce2->CeStatus & CES_inuse))) {

		if (!just_flip) {
			DevLog(DL_ERR(5098), request->d_slot);
			disp_of_event(library, event, 0);
			thr_exit(NULL);
		}
	}
	if (ce1->CeStatus & CES_unavail) {

		if (ce1->CeStatus & CES_unavail)
			DevLog(DL_ERR(5199), request->slot);
		else
			DevLog(DL_ERR(5200), request->d_slot);

		disp_of_event(library, event, 0);
		thr_exit(NULL);
	}
	if (source == (unsigned)ROBOT_NO_SLOT ||
	    dest == (unsigned)ROBOT_NO_SLOT) {
		DevLog(DL_ERR(5088), source, dest);
		disp_of_event(library, event, 0);
		thr_exit(NULL);
	}

	flags.bits = 0;

	if (move_media(library, 0, source, dest, just_flip, flags)) {
		disp_of_event(library, event, 0);
		thr_exit(NULL);
	}

	/* Update catalog. */
	memset(&vid, 0, sizeof (struct VolId));
	vid.ViEq = un->eq;
	vid.ViSlot = request->slot;
	if (just_flip) {
		/*
		 * Use 3 as a temporary partition.
		 * We have to be able to find both sides.
		 */
		vid.ViFlags = VI_onepart;
		vid.ViPart = 1;
		CatalogSetField(&vid, CEF_Partition, 3, 0);
		vid.ViPart = 2;
		CatalogSetField(&vid, CEF_Partition, 1, 0);
		vid.ViPart = 3;
		CatalogSetField(&vid, CEF_Partition, 2, 0);
	} else {
		vid.ViFlags = VI_cart;
		vid.ViPart = 0;
		CatalogMoveSlot(&vid, request->d_slot);
	}

	disp_of_event(library, event, 0);
	thr_exit(NULL);
}


/*
 * start a full audit or audit a slot
 *
 * returns -
 *	0 - on success (request assigned to drive or audit started
 *      !0 - No free drives.
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
	uchar_t 	audit_eod = 0;
	dev_ent_t 	*un;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	robo_event_t	*new_event;
	drive_state_t	*drive;

	un = library->un;
	if (event->type == EVENT_TYPE_INTERNAL)
		audit_eod = event->request.internal.flags.b.audit_eod;
	else
		audit_eod =
		    (event->request.message.param.audit_request.flags &
		    AUDIT_FIND_EOD) != 0;

	/* Audit of entire library or just single slot? */
	if (IS_GENERIC_API(library->un->type) &&
	    (slot == (unsigned)ROBOT_NO_SLOT)) {
		sam_syslog(LOG_INFO,
		    catgets(catfd, SET, 9199, "Audit not supported on GRAU."));
		disp_of_event(library, event, 0);
		return (0);
	} else {
		/* Auditing entire library */
		if (slot == (unsigned)ROBOT_NO_SLOT) {

			if (library->un->status.b.audit) {
				/* already auditing this library */
				disp_of_event(library, event, 0);
				return (0);
			}
			mutex_lock(&library->un->mutex);
			library->un->status.b.ready = FALSE;
			library->un->status.b.audit = TRUE;
			mutex_unlock(&library->un->mutex);

			(void) CatalogSetAudit(library->eq);

			/* Send each drive a request to start auditing */
			mutex_lock(&library->mutex);
			library->audit_index = 0;
			library->countdown = library->drives_auditing = 0;

			for (drive = library->drive; drive != NULL;
			    drive = drive->next) {

				if ((drive->thread < 0) ||
				    (drive->un->state >= DEV_IDLE)) {
					DevLog(DL_DETAIL(5089));
					continue;
				}
				library->countdown++;
				new_event = get_free_event(library);
				(void) memset(new_event, 0,
				    sizeof (robo_event_t));
				new_event->request.internal.command =
				    ROBOT_INTRL_START_AUDIT;
				new_event->request.internal.slot =
				    ROBOT_NO_SLOT;
				new_event->type = EVENT_TYPE_INTERNAL;
				new_event->status.bits = REST_FREEMEM;
				new_event->request.internal.flags.b.audit_eod =
				    audit_eod;
				mutex_lock(&drive->list_mutex);
				if (drive->active_count) {
					robo_event_t *end;

					LISTEND(drive, end);
					append_list(end, new_event);
				} else
					drive->first = new_event;

				drive->active_count++;
				cond_signal(&drive->list_condit);
				mutex_unlock(&drive->list_mutex);
			}

			mutex_unlock(&library->mutex);
			disp_of_event(library, event, 0);
			return (0);
		}
	}

	/* Auditing a single slot */
	ce = CatalogGetCeByLoc(library->un->eq, slot, 0, &ced);
	if (ce == NULL) {
		disp_of_event(library, event, EFAULT);
		return (0);
	}
	if (!(ce->CeStatus & CES_inuse)) {
		DevLog(DL_ERR(5091), slot);
		disp_of_event(library, event, EFAULT);
		return (0);
	}
	if (ce->CeStatus & CES_non_sam) {
		DevLog(DL_ERR(5092), slot);
		disp_of_event(library, event, EFAULT);
		return (0);
	}

	(void) CatalogSetFieldByLoc(ce->CeEq, ce->CeSlot, ce->CePart,
	    CEF_Status, CES_needs_audit, 0);
	if (library->status.b.two_sided) {
		(void) CatalogSetFieldByLoc(ce->CeEq, ce->CeSlot,
		    ((ce->CePart == 1) ? 2:1), CEF_Status, CES_needs_audit, 0);
	}

	/*
	 * search_drives returns with drive->un->mutex and
	 * drive->mutex both locked if a drive is found. If drive is
	 * NULL, no mutex is held.
	 */
	if ((drive =
	    search_drives(library, ELEMENT_ADDRESS(library, slot))) == NULL)
		drive = find_idle_drive(library);

	if (drive == NULL) {
		return (1);
	}
	drive->un->status.b.requested = TRUE;
	mutex_unlock(&drive->un->mutex);

	/* Add the request to the drive's worklist */
	new_event = get_free_event(library);
	(void) memset(new_event, 0, sizeof (robo_event_t));
	new_event->request.internal.slot = slot;
	new_event->type = EVENT_TYPE_INTERNAL;
	new_event->status.bits = REST_FREEMEM;
	new_event->request.internal.command = ROBOT_INTRL_START_AUDIT;
	new_event->request.internal.flags.b.audit_eod = audit_eod;

	mutex_lock(&drive->list_mutex);
	if (drive->active_count) {
		robo_event_t *end;

		LISTEND(drive, end);
		ETRACE((LOG_NOTICE, "ApDr(%d) %#x-%#x(%d) %s:%d.",
		    drive->un->eq, end, event,
		    drive->active_count, __FILE__, __LINE__));
		append_list(end, new_event);
	} else {
		ETRACE((LOG_NOTICE, "FrDr(%d) %#x(%d) %s:%d.",
		    drive->un->eq, event,
		    drive->active_count, __FILE__, __LINE__));
		drive->first = new_event;
	}

	drive->active_count++;
	mutex_unlock(&drive->mutex);

	cond_signal(&drive->list_condit);
	mutex_unlock(&drive->list_mutex);
	disp_of_event(library, event, 0);
	return (0);
}


/*
 * import_request - stick the import event on the import threads
 * work list.
 */
void
import_request(
	library_t *library,
	robo_event_t *event)
{
	robo_event_t 	*end;
	iport_state_t 	*import = library->import;

	mutex_lock(&import->list_mutex);

	if (import->active_count) {
		/* append to end of list */
		LISTEND(import, end);
		ETRACE((LOG_NOTICE, "ApIm %#x-%#x(%d) %s:%d.", end, event,
		    import->active_count, __FILE__, __LINE__));
		(void) append_list(end, event);
	} else {
		ETRACE((LOG_NOTICE, "FrIm %#x(%d) %s:%d.", event,
		    import->active_count, __FILE__, __LINE__));
		import->first = event;
	}

	import->active_count++;
	cond_signal(&import->list_condit);
	mutex_unlock(&import->list_mutex);
}


/*
 * export_request - place the export event on the import threads work list
 */
void
export_request(
	library_t *library,
	robo_event_t *event)
{
	robo_event_t 	*end;
	iport_state_t 	*import = library->import;

	set_media_default(&(event->request.message.param.export_request.media));
	mutex_lock(&import->list_mutex);

	if (import->active_count) {
		/* append to the end of the list */
		LISTEND(import, end);
		ETRACE((LOG_NOTICE, "ApIm %#x-%#x(%d) %s:%d.", end, event,
		    import->active_count, __FILE__, __LINE__));
		(void) append_list(end, event);
	} else {
		ETRACE((LOG_NOTICE, "ApIm %#x-%#x(%d) %s:%d.", end, event,
		    import->active_count, __FILE__, __LINE__));
		import->first = event;
	}

	import->active_count++;
	cond_signal(&import->list_condit);
	mutex_unlock(&import->list_mutex);
}


/*
 * unload_lib - Thread routine to set up a library for media change.
 * An unload to the library means
 * the operator wants to change the media.	Idle the robot,
 * unload the drives then off the robot.  Push all media to the
 * historian then unlock the "mailbox".
 * The library state change when the operator on's the library
 * will take care of everything else.
 */
typedef struct linked_events {
	robo_event_t event;
	struct linked_events *next;
} linked_events_t;


void *
unload_lib(
	void *vlibrary)
{
	library_t 	*library = (library_t *)vlibrary;
	dev_ent_t 	*un;
	drive_state_t 	*drive;
	robo_event_t 	*event;
	linked_events_t *first_event = NULL, *next_event = NULL;

	un = library->un;
	for (drive = library->drive; drive != NULL; drive = drive->next) {
		mutex_lock(&drive->mutex);
		if (drive->status.b.full) {
			mutex_lock(&drive->un->mutex);
			drive->un->status.b.requested = TRUE;
			mutex_unlock(&drive->un->mutex);
			if (first_event == NULL) {
				first_event = next_event =
				    (linked_events_t *)malloc_wait
				    (sizeof (linked_events_t), 2, 0);
			} else {
				next_event->next = (linked_events_t *)
				    malloc_wait(sizeof (linked_events_t), 2, 0);
				next_event = next_event->next;
			}

			memset(next_event, 0, sizeof (linked_events_t));
			event = &next_event->event;
			event->status.b.sig_cond = TRUE;
			event->type = EVENT_TYPE_MESS;
			event->request.message.magic = MESSAGE_MAGIC;
			event->request.message.command = MESS_CMD_UNLOAD;
			event->completion = REQUEST_NOT_COMPLETE;
			mutex_lock(&drive->list_mutex);

			if (drive->active_count) {
				robo_event_t *end;

				/* append to the end of the list */
				LISTEND(drive, end);
				ETRACE((LOG_NOTICE,
				    "ApDr(%d) %#x-%#x(%d) %s:%d.",
				    drive->un->eq, end, event,
				    drive->active_count, __FILE__, __LINE__));
				(void) append_list(end, event);
			} else {
				ETRACE((LOG_NOTICE, "FrDr(%d) %#x(%d) %s:%d.",
				    drive->un->eq, event, drive->active_count,
				    __FILE__, __LINE__));
				drive->first = event;
			}

			drive->active_count++;
			cond_signal(&drive->list_condit);
			mutex_unlock(&drive->list_mutex);
		}
		mutex_unlock(&drive->mutex);
	}

	next_event = first_event;
	/* Wait for all drives to unload */
	while (next_event != NULL) {
		linked_events_t *last_event;

		mutex_lock(&next_event->event.mutex);
		while (next_event->event.completion == REQUEST_NOT_COMPLETE)
			cond_wait(&next_event->event.condit,
			    &next_event->event.mutex);
		mutex_unlock(&next_event->event.mutex);
		mutex_destroy(&next_event->event.mutex);
		cond_destroy(&next_event->event.condit);
		last_event = next_event;
		next_event = next_event->next;
		free(last_event);
	}

	CatalogLibraryExport(library->un->fseq);

	mutex_lock(&library->un->mutex);
	OffDevice(un, SAM_STATE_CHANGE);
	library->un->status.b.requested = FALSE;
	library->un->status.b.i_e_port = TRUE;
	library->un->status.b.attention = TRUE;
	UNLOCK_MAILBOX(library);
	set_operator_panel(library, UNLOCK);
	mutex_unlock(&library->un->mutex);
	thr_exit((void *)NULL);
}


/*
 *
 *	 GRAU
 *
 */


/*
 *	add_to_cat_req - add an entry to the catalog
 *
 */
void
add_to_cat_req(
	library_t *library,
	robo_event_t *event)
{
#if !defined(SAM_OPEN_SOURCE)
    /* function for ADIC libraries */
	int		local_retry, d_errno, last_derrno = -1;
	uint32_t	status;
	char		*l_mess = library->un->dis_mes[DIS_MES_NORM];
	char		*ent_pnt = "add_to_cat_req";
	char		*tag = "view";
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	struct VolId	vid;
	char		other_side_vsn[ACI_VOLSER_LEN];
	sam_defaults_t	*defaults;
	addcat_request_t *request =
	    &event->request.message.param.addcat_request;
	dev_ent_t	*un = library->un;
	api_errs_t	ret;

	defaults = GetDefaults();

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

	memset(&vid, 0, sizeof (struct VolId));
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
	vid.ViSlot = ce->CeSlot;
	vid.ViPart = ce->CePart;

	if ((!(ce->CeStatus & CES_capacity_set)) && (ce->CeCapacity == 0)) {
		(void) CatalogSetFieldByLoc(library->un->eq, vid.ViSlot,
		    vid.ViPart, CEF_Capacity,
		    DEFLT_CAPC(library->un->media), 0);
		(void) CatalogSetFieldByLoc(library->un->eq,
		    vid.ViSlot, vid.ViPart,
		    CEF_Space, DEFLT_CAPC(library->un->media), 0);
	}
#if defined(DEBUG)
	sprintf(l_mess, "view %s", request->vsn);
#endif

	local_retry = 3;
	ret = API_ERR_TR;

	while (local_retry > 0) {
		if (aci_view_media(library, ce, (int *)NULL, &d_errno) == 0)
			break;
		else {
			/* Error return on api call */
			if (d_errno == 0) {
				/*
				 * if call did not happen -
				 * error return but no error
				 */
				local_retry = -1;
				d_errno = EAMUCOMM;
			} else if ((last_derrno == -1) ||
			    (last_derrno != d_errno)) {
				/* Save error if repeated */
				last_derrno = d_errno;
				if (api_valid_error(library->un->type,
				    d_errno, library->un)) {

					if (library->un->slot !=
					    ROBOT_NO_SLOT) {
						DevLog(DL_DEBUG(6001),
						    library->un->slot, tag,
						    d_errno, d_errno,
						    api_return_message(
						    library->un->type,
						    d_errno));
					} else {
						DevLog(DL_DEBUG(6043), tag,
						    d_errno, d_errno,
						    api_return_message(
						    library->un->type,
						    d_errno));
					}

					local_retry =
					    api_return_retry(library->un->type,
					    d_errno);
					ret =
					    api_return_degree(library->un->type,
					    d_errno);
				} else {
					local_retry = -2;
				}
			}
			if (local_retry > 0) {
				/* delay before retrying */
				local_retry--;
				if (local_retry > 0)
					sleep(api_return_sleep(
					    library->un->type, d_errno));
			}
		}
	}
	if (d_errno != EOK) {

		memmove(vid.ViVsn, ce->CeVsn, sizeof (vid.ViVsn));
		vid.ViFlags = VI_logical;
		CatalogExport(&vid);

		DevLog(DL_ERR(6041), request->vsn);

		if (local_retry == -1) {
			/* The call didn't happen */
			DevLog(DL_ERR(6040), tag);
		} else if (local_retry == 0) {
			/* retries exceeded */
			DevLog(DL_ERR(6039), tag);
		} else {
			if (api_valid_error(library->un->type, d_errno,
			    library->un)) {
				if (library->un->slot != ROBOT_NO_SLOT) {
					DevLog(DL_ERR(6001), library->un->slot,
					    tag, d_errno, d_errno,
					    api_return_message(
					    library->un->type, d_errno));
				} else {
					DevLog(DL_ERR(6043), tag, d_errno,
					    d_errno, api_return_message(
					    library->un->type, d_errno));
				}
			}
		}

		if (ret == API_ERR_DM) {
			set_bad_media(un);
		} else if (ret == API_ERR_DL) {
			down_library(library, SAM_STATE_CHANGE);
		}
		return;
	} else {
		if (library->status.b.two_sided) {

			memset(other_side_vsn, 0, ACI_VOLSER_LEN);
			if (aci_getside(library, request->vsn,
			    (char *)other_side_vsn, &d_errno)) {
				CatalogExport(&vid);
			} else {
				vid.ViSlot = ce->CeSlot;
				vid.ViPart = 2;
				vid.ViFlags = VI_cart;
				/*
				 * Fill in the Catalog Entry with all the info
				 * The status bits have already been set
				 * correctly when Side A was imported.
				 */
				(void) CatalogSetStringByLoc(library->un->eq,
				    vid.ViSlot, vid.ViPart, CEF_BarCode,
				    other_side_vsn);
				if (defaults->flags & DF_LABEL_BARCODE) {
					int volser_size;
					vsn_t tmp_vsn;

					volser_size = ACI_VOLSER_LEN;
					vsn_from_barcode(tmp_vsn,
					    other_side_vsn, defaults,
					    volser_size);
					(void) CatalogSetStringByLoc(
					    library->un->eq, vid.ViSlot,
					    vid.ViPart, CEF_Vsn, tmp_vsn);
					status &= ~CES_needs_audit;
					(void) CatalogSetFieldByLoc(
					    library->un->eq, vid.ViSlot,
					    vid.ViPart, CEF_Status, status,
					    CES_needs_audit);
					status = CES_labeled;
					(void) CatalogSetFieldByLoc(
					    library->un->eq, vid.ViSlot,
					    vid.ViPart, CEF_Status, status, 0);
					ce = CatalogGetCeByLoc(library->un->eq,
					    vid.ViSlot, vid.ViPart, &ced);
					if ((ce != NULL) &&
					    (ce->CeCapacity == 0)) {
						(void) CatalogSetFieldByLoc(
						    library->un->eq, vid.ViSlot,
						    vid.ViPart, CEF_Capacity,
						    DEFLT_CAPC(
						    library->un->media), 0);
						(void) CatalogSetFieldByLoc(
						    library->un->eq, vid.ViSlot,
						    vid.ViPart, CEF_Space,
						    ce->CeCapacity, 0);
					}
				}
			}
		}
	}


	if (request->flags & CMD_ADD_VSN_AUDIT &&
	    !(ce->CeStatus & CES_cleaning)) {
		robo_event_t 	*new_event;
		robot_internal_t *request;

		status |= CES_needs_audit;
		(void) CatalogSetFieldByLoc(library->un->eq, vid.ViSlot,
		    vid.ViPart, CEF_Status, status, 0);
		new_event = get_free_event(library);
		request = &new_event->request.internal;
		(void) memset(new_event, 0, sizeof (robo_event_t));
		new_event->type = EVENT_TYPE_INTERNAL;
		new_event->status.bits = REST_FREEMEM;
		request->command = ROBOT_INTRL_AUDIT_SLOT;
		request->slot = ce->CeSlot;
		request->flags.b.audit_eod = TRUE;
		add_to_end(library, new_event);
	} else if (request->flags & CMD_ADD_VSN_STRANGE &&
	    !(ce->CeStatus & CES_cleaning)) {
		status &= ~(CES_labeled | CES_needs_audit);
		status |= CES_non_sam;
		(void) CatalogSetFieldByLoc(library->un->eq, vid.ViSlot,
		    vid.ViPart, CEF_Status, status, 0);
	}
#else
    sam_syslog(LOG_WARNING, "unimplemented ADIC code called");
#endif
}


/*
 *	api_export_media - bring media out of the library
 *
 */
void
api_export_media(
	library_t *library,
	robo_event_t *event)
{
#if !defined(SAM_OPEN_SOURCE)
	int 	err = 0;
	char 	*ent_pnt = "api_export_media";
	uint_t 	slot;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	drive_state_t 	*drive = NULL;
	export_request_t *request =
	    &event->request.message.param.export_request;
	dev_ent_t *un = library->un;


	/* find the media to export */
	slot = request->slot;
	switch (request->flags & EXPORT_FLAG_MASK) {
	case EXPORT_BY_VSN:
		if (slot == (unsigned)ROBOT_NO_SLOT) {
			err = ENOENT;
			sam_syslog(LOG_INFO, catgets(catfd, SET, 9164,
			    "%s: vsn (%s) not found."), ent_pnt, request->vsn);
			goto err;
		}
		break;

	case EXPORT_BY_SLOT:
		if (slot == (unsigned)ROBOT_NO_SLOT) {
			err = EFAULT;
			sam_syslog(LOG_INFO,
			    catgets(catfd, SET, 9090,
			    "%s: Slot (%d) is out of range."),
			    ent_pnt, slot);
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
			sam_syslog(LOG_INFO,
			    catgets(catfd, SET, 9165,
			    "%s: Device(%d) not found."), ent_pnt, request->eq);
			goto err;
		}
		if (!drive->status.b.full) {
			err = ENOENT;
			sam_syslog(LOG_INFO,
			    catgets(catfd, SET, 9166,
			    "%s: Device(%d) is empty."), ent_pnt, request->eq);
			goto err;
		}
		if (drive->status.b.full && drive->un->slot != ROBOT_NO_SLOT)
			slot = drive->un->slot;
		else
			slot = (unsigned)ROBOT_NO_SLOT;

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
		sam_syslog(LOG_INFO, "%s: Bad request %d", ent_pnt,
		    request->flags);
		goto err;
	}
	/* slot should now be set */

	/*
	 * If not a drive export, check to make sure the slot is occupied,
	 * if not, look over the drives and find the media
	 */
	ce = CatalogGetCeByLoc(library->un->eq, slot, 0, &ced);
	if (ce == NULL)
		goto err;

	if (drive == NULL && slot != (unsigned)ROBOT_NO_SLOT) {
		if (!(ce->CeStatus & CES_inuse)) {
			err = ENOENT;
			sam_syslog(LOG_INFO,
			    catgets(catfd, SET, 9096,
			    "%s: Slot (%d) is not occupied."),
			    ent_pnt, ce->CeSlot);
			goto err;
		}
		if (!(ce->CeStatus & CES_occupied)) {
			for (drive = library->drive; drive != NULL;
			    drive = drive->next) {
				mutex_lock(&drive->mutex);
				if (drive->status.b.full && drive->un->slot
				    == slot)
					break;
				mutex_unlock(&drive->mutex);
			}

			if (drive == NULL) {
				err = ENOENT;
				sam_syslog(LOG_INFO,
				    "%s: Unable to find media for slot %d.",
				    ent_pnt, slot);
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
					    " mounted and drive(%d) is busy."),
					    ent_pnt, slot, drive->un->eq);
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
		int local_retry, d_errno, last_derrno = -1;
		api_errs_t ret;
		char *tag = "dismount";

		local_retry = 3;
		ret = API_ERR_TR;

		while (local_retry > 0) {
			if (aci_dismount_media(library, drive, &d_errno) == 0)
				break;
			else {
				/* Error return on api call */
				if (d_errno == 0) {
					/*
					 * if call did not happen -
					 * error return but no error
					 */
					local_retry = -1;
					d_errno = EAMUCOMM;
				} else if ((last_derrno == -1) ||
				    (last_derrno != d_errno)) {
					/* Save error if repeated */
					last_derrno = d_errno;
					if (api_valid_error(library->un->type,
					    d_errno, library->un)) {
						if (slot !=
						    (unsigned)ROBOT_NO_SLOT) {
							DevLog(DL_DEBUG(6001),
							    slot, tag, d_errno,
							    d_errno,
							    api_return_message(
							    library->un->type,
							    d_errno));
						} else {
							DevLog(DL_DEBUG(6043),
							    slot, tag, d_errno,
							    d_errno,
							    api_return_message(
							    library->un->type,
							    d_errno));
						}

						local_retry =
						    api_return_retry(
						    library->un->type, d_errno);
						ret = api_return_degree(
						    library->un->type, d_errno);
					} else {
						local_retry = -2;
					}
				}
				if (local_retry > 0) {
					/* delay before retrying */
					local_retry--;
					if (local_retry > 0)
						sleep(api_return_sleep(
						    library->un->type,
						    d_errno));
				}
			}
		}

		if (d_errno != EOK) {
			if (local_retry == -1) {
				/* The call didn't happen */
				DevLog(DL_ERR(6040), tag);
			} else if (local_retry == 0) {
				/* retries exceeded */
				DevLog(DL_ERR(6039), tag);
			} else {
				if (api_valid_error(library->un->type,
				    d_errno, library->un)) {
					if (slot != (unsigned)ROBOT_NO_SLOT) {
						DevLog(DL_ERR(6001), slot, tag,
						    d_errno, d_errno,
						    api_return_message(
						    library->un->type,
						    d_errno));
					} else {
						DevLog(DL_ERR(6043), tag,
						    d_errno, d_errno,
						    api_return_message(
						    library->un->type,
						    d_errno));

					}
				}
			}

			if (ret == API_ERR_DL)
				down_library(library, SAM_STATE_CHANGE);
			else if (ret == API_ERR_DD)
				down_drive(drive, SAM_STATE_CHANGE);

			disp_of_event(library, event, 1);
			return;
		}
		mutex_lock(&drive->un->mutex);
		/* clear the drive information */
		drive->status.b.full = FALSE;
		drive->status.b.bar_code = FALSE;
		drive->aci_drive_entry->volser[0] = '\0';

		drive->un->status.bits =
		    (drive->un->status.bits & DVST_CLEANING);
		drive->un->status.b.present = 1;
		if (drive->open_fd >= 0)
			close_unit(drive->un, &drive->open_fd);

		mutex_unlock(&drive->un->mutex);
		mutex_unlock(&drive->mutex);
	}
	if (slot != ROBOT_NO_SLOT) {
		struct VolId vid;

		memset(&vid, 0, sizeof (struct VolId));
		vid.ViEq = request->eq;
		vid.ViSlot = slot;
		vid.ViFlags = VI_cart;
		CatalogExport(&vid);
	}
	disp_of_event(library, event, err);
	return;


err:
	disp_of_event(library, event, err);

#else
    sam_syslog(LOG_WARNING, "unimplemented ADIC code called");
#endif
}
