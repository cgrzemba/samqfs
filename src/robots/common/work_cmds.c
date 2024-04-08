/*
 *	work_cmds.c - command processors for the work list
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
#pragma ident "$Revision: 1.36 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */
#include <stdio.h>
#include <thread.h>
#include <synch.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <syslog.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/shm.h"
#include "aml/robots.h"
#include "aml/trace.h"
#include "sam/custmsg.h"
#include "sam/defaults.h"
#include "sam/nl_samfs.h"
#include "../generic/generic.h"
#include "aml/proto.h"
#include "aml/tapealert.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "sam/lib.h"

#pragma weak is_flip_requested
#pragma weak unload_lib			/* Unique to generic */

/* Function prototypes */
int entry_found(preview_t *, struct CatalogEntry *, void *, uint_t, int);
void *unload_lib(void *);

/* globals */
extern shm_alloc_t master_shm, preview_shm;


/*
 * find a drive to label the requested media
 *
 * returns -
 *	 0 - on success(request assigned to drive)
 *	!0 - No free drives.
 *
 * Note: both search_drives and find_idle_drive return with the mutex locked
 * for the drive found.
 */
int
label_request(
	library_t *library,
	robo_event_t *event)
{
	int		tmp;
	char 		*ent_pnt = "label_request";
	char 		*l_mess = library->un->dis_mes[DIS_MES_NORM];
	uint_t 		element;
	robo_event_t 	*end;
	drive_state_t 	*drive;
	sam_defaults_t 	*defaults;
	label_request_t *request = &event->request.message.param.label_request;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	struct CatalogEntry ced_dup;
	struct CatalogEntry *ce_dup = &ced_dup;

	defaults = GetDefaults();

	if (request->slot != ROBOT_NO_SLOT) {
		ce = CatalogGetCeByLoc(library->un->eq, request->slot,
		    request->part, &ced);
	} else {
		ce = CatalogGetCeByMedia((char *)sam_mediatoa(request->media),
		    request->vsn, &ced);
	}

	/* if slot not inuse or its busy, can't label it */
	if ((ce == NULL) || (!(ce->CeStatus & CES_inuse)) ||
	    (ce->CeStatus & CES_unavail) || (ce->CeStatus & CES_needs_audit)) {
		char *mess =
		    catgets(catfd, SET, 9228, "Slot is empty or media is busy");

		sam_syslog(LOG_INFO, catgets(catfd, SET, 9227,
		    "%s: Label request for slot %d: Slot is empty or busy."),
		    ent_pnt, request->slot);
		write_event_exit(event, EXIT_FAILED, mess);
		memccpy(l_mess, mess, '\0', DIS_MES_LEN);
		disp_of_event(library, event, EFAULT);
		return (0);
	}

	if ((request->flags & LABEL_BARCODE) == LABEL_BARCODE) {
		if ((!(ce->CeStatus & CES_bar_code)) ||
		    (ce->CeStatus & CES_cleaning)) {
			char *mess;

			if (ce->CeStatus & CES_cleaning) {
				mess = catgets(catfd, SET, 9232,
				    "Slot contains a cleaning cartridge");
				sam_syslog(LOG_INFO,
				    catgets(catfd, SET, 9236,
				    "%s: Slot %d is a cleaning cartridge."),
				    ent_pnt, ce->CeSlot);
			} else {
				mess = catgets(catfd, SET, 9231,
				    "media has no barcode");
				sam_syslog(LOG_INFO,
				    catgets(catfd, SET, 9229,
				    "%s: Barcode label request:"
				    " slot %d has no barcode."),
				    ent_pnt, ce->CeBarCode);
			}

			write_event_exit(event, EXIT_FAILED, mess);
			memccpy(l_mess, mess, '\0', DIS_MES_LEN);
			disp_of_event(library, event, ENOENT);
			return (0);
		}

		/*
		 * build the vsn from the barcode, only get the
		 * correct amount based on the media type in the slot
		 */
		if (is_tape(sam_atomedia(ce->CeMtype)))
			tmp = LEN_TAPE_VSN;
		else
			tmp = LEN_OPTIC_VSN;

		vsn_from_barcode(request->vsn, ce->CeBarCode, defaults, tmp);
	}

	set_media_default(&(request->media));

	/* Make sure this vsn is not in the robot already */
	ce_dup = CatalogGetCeByMedia((char *)sam_mediatoa(request->media),
	    request->vsn, &ced_dup);

	/*
	 * Check to be sure this is really a duplicate and not just
	 * the same vsn. Could be a relabel of the same vsn or a
	 * -new label with label lie in effect.
	 */
	if ((ce_dup != NULL) && (ce_dup->CeMid != ce->CeMid)) {
		char *MES_9234 = catgets(catfd, SET, 9234,
		    "request would duplicate vsn at slot %d");
		char *mess = (char *)malloc_wait(strlen(MES_9234) + 10, 4, 0);
		sprintf(mess, MES_9234, ce_dup->CeSlot);

		/* generate a sysevent and write to sam_log */
		SendCustMsg(HERE, 9233, request->vsn, ce_dup->CeVsn,
		    ce_dup->CeSlot);

		write_event_exit(event, EXIT_FAILED, mess);
		memccpy(l_mess, mess, '\0', DIS_MES_LEN);
		free(mess);
		disp_of_event(library, event, EEXIST);
		return (0);
	}

	/* check to see if this media is already in a drive */
	/* search_drives and find_idle_drive will exit with the mutex set */
	element = ELEMENT_ADDRESS(library, ce->CeSlot);
	if ((drive = search_drives(library, element)) == NULL) {
		if (!(ce->CeStatus & CES_occupied)) {
			return (1);
		}

		/*
		 * still a window here, another drive could be mounting
		 * this media but the code in drive1.c will take
		 * care of the case when the slot is not occupied during media
		 * load try to find an idle drive. find_idle exits with mutex
		 * set
		 */
		memmove(library->un->i.ViVsn, ce->CeVsn,
		    sizeof (library->un->i.ViVsn));
		if ((drive = find_idle_drive(library)) == NULL)
			return (1);	/* cant find idle drive */
	} else {
		/* Make sure the drive is idle */
		if (drive->un->status.b.requested || drive->un->active ||
		    drive->un->open_count || drive->un->state >= DEV_IDLE) {
			mutex_unlock(&drive->un->mutex); /* release the mutex */
			mutex_unlock(&drive->mutex);	/* release the mutex */
			return (1);
		}
	}

	drive->un->status.b.shared_reqd = FALSE;
	drive->un->status.b.requested = TRUE;	/* flag it */
	mutex_unlock(&drive->un->mutex); /* release the mutex */

	/* put this event of the worklist for the drive */

	event->next = NULL;
	event->previous = NULL;
	mutex_lock(&drive->list_mutex);

	if (drive->active_count) {
		LISTEND(drive, end);
		append_list(end, event);
	} else {
		drive->first = event;
	}

	drive->active_count++;
	mutex_unlock(&drive->mutex);
	cond_signal(&drive->list_condit);
	mutex_unlock(&drive->list_mutex);
	return (0);			/* return success */
}


/*
 * check_requests - check preview for something in this library
 *
 * returns -
 *	 0 - on success(request assigned to drive or nothing to do)
 *		if success the event is now on the work list of the drive chosen
 *		if nothing to do, the event in on the free list or free'd
 *	!0 - No free drives.
 */
int
check_requests(
	library_t *library)
{
	preview_t 		*preview;
	drive_state_t 	*drive;
	struct CatalogEntry ced;

	/* Library isn't ON so don't bother checking the drives */
	if (library->un->state <= DEV_ON) {
		/*
		 * Look over the drives to see if any requested
		 * media is already mounted.
		 */
		for (drive = library->drive; drive != NULL;
		    drive = drive->next) {
			if (drive->un == NULL)
				continue;
			if (drive->un->status.b.labeled &&
			    drive->un->state < DEV_IDLE) {
				if (!mutex_trylock(&drive->mutex)) {
					if (!mutex_trylock(&drive->un->mutex)) {
						mutex_unlock(&drive->mutex);
						/*
						 * Make sure the unit is still
						 * labeled and not requested
						 */
						if (drive->
						    un->status.b.labeled &&
						    !drive->un->
						    status.b.requested)
							check_preview(
							    drive->un);
						mutex_unlock(&drive->un->mutex);
					} else
						mutex_unlock(&drive->mutex);
				}
			}
		}

		/* Look for an idle device with the flip side requested. */
		if (library->status.b.two_sided)
			for (drive = library->drive; drive != NULL;
			    drive = drive->next) {

		/* N.B. Bad indentation here to meet cstyle requirements */

		if (drive->un == NULL)
			continue;
		if (drive->un->status.b.ready && drive->un->state < DEV_IDLE &&
		    !mutex_trylock(&drive->mutex)) {
			mutex_lock(&drive->un->mutex);
			if ((drive->active_count == 0) &&
			    (drive->un->active == 0) &&
			    (drive->un->open_count == 0) &&
			    (drive->un->mtime < time(NULL)) &&
			    !drive->un->status.b.requested) {
				robo_event_t *event;

				if (!is_flip_requested(library, drive)) {
					mutex_unlock(&drive->un->mutex);
					mutex_unlock(&drive->mutex);
					continue;
				}
				drive->un->status.b.requested = TRUE;
				mutex_unlock(&drive->un->mutex);
				event = malloc_wait(
				    sizeof (robo_event_t), 2, 0);
				(void) memset(event, 0, sizeof (robo_event_t));
				event->type = EVENT_TYPE_MESS;
				event->request.message.magic = MESSAGE_MAGIC;
				event->request.message.command =
				    MESS_CMD_UNLOAD;
				event->completion = REQUEST_NOT_COMPLETE;
				event->status.bits = (REST_DONTREQ |
				    REST_FREEMEM);
				mutex_lock(&drive->list_mutex);
				if (drive->active_count) {
					robo_event_t *end;
					/* append to the end of the list */
					LISTEND(drive, end);
					ETRACE((LOG_NOTICE,
					    "ApDr(%d) %#x-%#x(%d) %s:%d.",
					    drive->un->eq, end, event,
					    drive->active_count, __FILE__,
					    __LINE__));
					(void) append_list(end, event);
				} else {
					ETRACE((LOG_NOTICE,
					    "FrDr(%d) %#x(%d) %s:%d.",
					    drive->un->eq, event,
					    drive->active_count,
					    __FILE__, __LINE__));
					drive->first = event;
				}

				drive->active_count++;
				mutex_unlock(&drive->mutex);
				cond_signal(&drive->list_condit);
				mutex_unlock(&drive->list_mutex);
			} else {
				mutex_unlock(&drive->un->mutex);
				mutex_unlock(&drive->mutex);
			}
		}
			}
	}
	/*
	 * loop attempting to mount all requested media
	 * until there are no requests or all drives are busy.
	 */
	mutex_lock(&library->mutex);
	library->preview_count = 0;
	mutex_unlock(&library->mutex);

	while ((preview = scan_preview(library->un->eq, &ced, PREVIEW_SET_FOUND,
	    (void *)library, entry_found)) != NULL) {
		drive = NULL;

		/* Must do this after scan to be sure that the robot->equ */
		/* is set */
		if (library->un->state > DEV_ON) {
			sam_syslog(LOG_DEBUG,
			    "Remove preview entries for off'd robot (%d)\n",
			    library->un->eq);
			if (thr_create(NULL, DF_THR_STK, remove_stale_preview,
			    NULL, (THR_DETACHED | THR_BOUND), NULL) &&
			    thr_create(NULL, DF_THR_STK, remove_stale_preview,
			    NULL, THR_DETACHED, NULL)) {
				sam_syslog(LOG_INFO,
				    "Unable to create thread"
				    " remove_stale_preview:%m.");
			}
			return (1);
		}

		/* find_idle_drive exits with the mutex set */
		if (ced.CeSlot != ROBOT_NO_SLOT) {
			memmove(library->un->i.ViVsn, ced.CeVsn,
			    sizeof (library->un->i.ViVsn));
		} else {
			memmove(library->un->i.ViVsn, " ",
			    sizeof (library->un->i.ViVsn));
		}

		if (drive || (drive = find_idle_drive(library)) != NULL) {
			robo_event_t *event;

			if (drive->un->status.b.requested ||
			    drive->un->active ||
			    drive->un->open_count ||
			    drive->un->state >= DEV_IDLE) {

				/* release the mutex */
				mutex_unlock(&drive->un->mutex);
				/* release the mutex */
				mutex_unlock(&drive->mutex);
				mutex_lock(&preview->p_mutex);
				if (preview->p_error) {
					mutex_unlock(&preview->p_mutex);
					remove_preview_ent(preview, NULL,
					    PREVIEW_NOTHING, 0);
				} else {
					preview->busy = FALSE;
					mutex_unlock(&preview->p_mutex);
				}
				return (1);
			}
			drive->un->status.b.shared_reqd = FALSE;
			drive->un->status.b.requested = TRUE;
			mutex_unlock(&drive->un->mutex);
			drive->new_slot = ced.CeSlot;
			event = malloc_wait(sizeof (robo_event_t), 2, 0);
			(void) memset(event, 0, sizeof (robo_event_t));
			event->type = EVENT_TYPE_INTERNAL;
			event->status.bits = (REST_DONTREQ | REST_FREEMEM);
			event->request.internal.command = ROBOT_INTRL_MOUNT;
			event->request.internal.slot = ced.CeSlot;
			event->request.internal.part = ced.CePart;
			event->request.internal.address = preview;
			event->request.internal.sequence = preview->sequence;
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
			mutex_unlock(&drive->mutex);
			cond_signal(&drive->list_condit);
			mutex_unlock(&drive->list_mutex);
		} else {
			/* No drives, clear the busy bit. */
			mutex_lock(&preview->p_mutex);
			if (preview->p_error) {
				mutex_unlock(&preview->p_mutex);
				remove_preview_ent(
				    preview, NULL, PREVIEW_NOTHING, 0);
			} else {
				preview->busy = FALSE;
				mutex_unlock(&preview->p_mutex);
			}
			return (1);
		}
	}
	return (0);
}


/*
 * mount_request - find a drive to mount the requested media.
 *
 * returns -
 *	 0 - on success(request assigned to drive or no vsn found)
 *	!0 - No free drives.
 *
 * Note: both search_drives and find_idle_drive return with the mutex locked
 * for the drive found.
 */
int
mount_request(
	library_t *library,
	robo_event_t *event)
{
	char 	*l_mess = library->un->dis_mes[DIS_MES_LEN];
	char 	*ent_pnt = "mount_request";
	mount_request_t *request;
	drive_state_t *drive;
	struct CatalogEntry ced;
	struct CatalogEntry *ce;

	request = &event->request.message.param.mount_request;

	set_media_default(&request->media);

	/* If there is no slot, then use vsn to get the slot number */
	if (request->slot == ROBOT_NO_SLOT) {
		ce = CatalogGetCeByMedia((char *)sam_mediatoa(request->media),
		    request->vsn, &ced);
	} else {
		ce = CatalogGetCeByLoc(library->un->eq, request->slot,
		    request->part, &ced);
	}

	if (ce == NULL) {
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_INFO, "%s: %s not found.",
			    ent_pnt, request->vsn);
		memccpy(l_mess, catgets(catfd, SET, 9044, "vsn not found"),
		    '\0', DIS_MES_LEN);
		disp_of_event(library, event, ENOENT);
		return (0);
	}

	if ((!(ce->CeStatus & CES_inuse)) || (ce->CeStatus & CES_cleaning)) {
		char *mess;

		if (ce->CeStatus & CES_cleaning) {
			mess =
			    catgets(catfd, SET, 9232,
			    "Slot contains a cleaning cartridge");
			sam_syslog(LOG_INFO,
			    catgets(catfd, SET, 9236,
			    "%s: Slot %d is a cleaning cartridge."),
			    ent_pnt, ce->CeSlot);
		} else {
			mess = catgets(catfd, SET, 9228,
			    "Slot is empty or media is busy");
			sam_syslog(LOG_INFO, catgets(catfd, SET, 9235,
			    "%s: Slot %d is empty."), ent_pnt, ce->CeSlot);
		}

		memccpy(l_mess, mess, '\0', DIS_MES_LEN);
		disp_of_event(library, event, ENOENT);
		return (0);
	}

	/*
	 * check to see if this media is already in a drive.
	 * search_drives exits with the mutex set
	 */
	drive = search_drives(library, ELEMENT_ADDRESS(library, ce->CeSlot));

	if (drive != NULL && drive->un->state >= DEV_IDLE) {
		mutex_unlock(&drive->un->mutex);
		mutex_unlock(&drive->mutex);	/* release mutexs */
		return (1);		/* indicate no resources */
	}

	/* if in a drive and the right side is up */
	if ((drive != NULL) &&
	    (IS_FLIPPED(library, ce->CeSlot) == drive->status.b.d_st_invert)) {

		/*
		 * Increment device active and open for migration toolkit.
		 * These fields will be decremented when the toolkit code
		 * releases the device (sam_mig_release_device).
		 */
		if ((event->type == EVENT_TYPE_MESS) &&
		    (request->flags & CMD_MOUNT_S_MIGKIT)) {
			INC_ACTIVE(drive->un);
			INC_OPEN(drive->un);
		}

		mutex_unlock(&drive->un->mutex);
		mutex_unlock(&drive->mutex);	/* release mutex */
		disp_of_event(library, event, 0);
		return (0);		/* media already mounted */
	}

	if (drive == NULL) {
		if (!(ce->CeStatus & CES_occupied)) {
			return (1);
		}
		memmove(library->un->i.ViVsn, ce->CeVsn,
		    sizeof (library->un->i.ViVsn));
		if ((drive = find_idle_drive(library)) == NULL)
			return (1);	/* no free drives */
	}
	drive->un->status.b.shared_reqd = FALSE;
	drive->un->status.b.requested = TRUE;
	mutex_unlock(&drive->un->mutex);

	/* put this event on the worklist for the drive */

	event->next = NULL;
	event->previous = NULL;

	mutex_lock(&drive->list_mutex);
	if (drive->active_count) {
		robo_event_t *end;

		LISTEND(drive, end);
		ETRACE((LOG_NOTICE, "ApDr(%d) %#x-%#x(%d) %s:%d.",
		    drive->un->eq, end, event, drive->active_count,
		    __FILE__, __LINE__));
		append_list(end, event);
	} else {
		ETRACE((LOG_NOTICE, "FrDr(%d) %#x(%d) %s:%d.",
		    drive->un->eq, event, drive->active_count,
		    __FILE__, __LINE__));
		drive->first = event;
	}

	drive->active_count++;
	mutex_unlock(&drive->mutex);	/* release drive mutex */
	cond_signal(&drive->list_condit);
	mutex_unlock(&drive->list_mutex);
	return (0);
}


/*
 * load_unavail_request - load the media on the specified "unavail" drive.
 *
 */
void
load_unavail_request(
	library_t *library,
	robo_event_t *event)
{
	char 	*l_mess = library->un->dis_mes[DIS_MES_NORM];
	char 	*ent_pnt = "load_unavail_request";
	load_u_request_t *request;
	drive_state_t *drive;
	struct CatalogEntry ced;
	struct CatalogEntry *ce;

	request = &event->request.message.param.load_u_request;

	set_media_default(&request->media);

	/*
	 * Always get catalog entry by media.vsn because the eq
	 * in this case is the eq of the drive.
	 */
	ce = CatalogGetCeByLoc(library->un->eq, request->slot,
	    request->part, &ced);

	if (ce == NULL) {
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_INFO, "%s: %s not found.",
			    ent_pnt, request->vsn);
		memccpy(l_mess, catgets(catfd, SET, 9044, "vsn not found"),
		    '\0', DIS_MES_LEN);
		disp_of_event(library, event, ENOENT);
		return;
	}

	if ((!(ce->CeStatus & CES_inuse)) || (ce->CeStatus & CES_cleaning)) {
		char *mess;

		if (ce->CeStatus & CES_cleaning) {
			mess = catgets(catfd, SET, 9232,
			    "Slot contains a cleaning cartridge");
			sam_syslog(LOG_INFO, catgets(catfd, SET, 9236,
			    "%s: Slot %d is a cleaning cartridge."),
			    ent_pnt, ce->CeSlot);
		} else {
			mess = catgets(catfd, SET, 9228,
			    "Slot is empty or media is busy");
			sam_syslog(LOG_INFO, catgets(catfd, SET, 9235,
			    "%s: Slot %d is empty."), ent_pnt, ce->CeSlot);
		}

		memccpy(l_mess, mess, '\0', DIS_MES_LEN);
		disp_of_event(library, event, ENOENT);
		return;
	}

	/*
	 * check to see if this media is already in a drive.
	 * search_drives exits with the mutex set
	 */
	drive = search_drives(library, ELEMENT_ADDRESS(library, ce->CeSlot));
	if (drive != NULL) {

		mutex_unlock(&drive->un->mutex);
		mutex_unlock(&drive->mutex);

		sam_syslog(LOG_INFO, catgets(catfd, SET, 9230,
		    "%s: %s is already mounted on %d."),
		    ent_pnt, ce->CeVsn, drive->un->eq);
		memccpy(l_mess, catgets(catfd, SET, 9237,
		    "media is already mounted"), '\0', DIS_MES_LEN);
		disp_of_event(library, event, 0);
		return;			/* media already mounted */
	}

	if (!(ce->CeStatus & CES_occupied)) {
		char *mess =
		    catgets(catfd, SET, 9228, "Slot is empty or media is busy");
		sam_syslog(LOG_INFO, catgets(catfd, SET, 9235,
		    "%s: Slot %d is empty."), ent_pnt, ce->CeSlot);
		memccpy(l_mess, mess, '\0', DIS_MES_LEN);
		disp_of_event(library, event, 0);
		return;
	}

	for (drive = library->drive; drive != NULL; drive = drive->next)
		if (drive->un->eq == request->eq)
			break;

	if (drive == NULL) {
		sam_syslog(LOG_INFO, catgets(catfd, SET, 9238,
		    "%s: Cannot find device %d."), ent_pnt, request->eq);
		memccpy(l_mess, catgets(catfd, SET, 9239, "cannot find device"),
		    '\0', DIS_MES_LEN);
		disp_of_event(library, event, EFAULT);
		return;
	}

	mutex_lock(&drive->mutex);
	mutex_lock(&drive->un->mutex);
	if (drive->un->state != DEV_UNAVAIL) {
		sam_syslog(LOG_INFO, catgets(catfd, SET, 9240,
		    "%s: Device %d: State must be set to unavail."),
		    ent_pnt, request->eq);

		memccpy(l_mess,
		    catgets(catfd, SET, 9241, "state must be set to unavail"),
		    '\0', DIS_MES_LEN);
		mutex_unlock(&drive->un->mutex);
		mutex_unlock(&drive->mutex);
		disp_of_event(library, event, EFAULT);
		return;
	}

	drive->un->status.b.requested = TRUE;
	mutex_unlock(&drive->un->mutex);

	/* put this event on the worklist for the drive */
	event->next = NULL;
	event->previous = NULL;

	mutex_lock(&drive->list_mutex);
	if (drive->active_count) {
		robo_event_t *end;

		LISTEND(drive, end);
		ETRACE((LOG_NOTICE, "ApDr(%d) %#x-%#x(%d) %s:%d.",
		    drive->un->eq, end, event, drive->active_count,
		    __FILE__, __LINE__));
		append_list(end, event);
	} else {
		ETRACE((LOG_NOTICE, "FrDr(%d) %#x(%d) %s:%d.",
		    drive->un->eq, event, drive->active_count,
		    __FILE__, __LINE__));
		drive->first = event;
	}

	drive->active_count++;
	mutex_unlock(&drive->mutex);	/* release drive mutex */
	cond_signal(&drive->list_condit);
	mutex_unlock(&drive->list_mutex);
}


/*
 * give a drive a file system todo request
 */
int
todo_request(
	library_t *library,
	robo_event_t *event)
{
	todo_request_t *request;
	drive_state_t *drive;

	request = &event->request.message.param.todo_request;

	for (drive = library->drive; drive != NULL; drive = drive->next)
		if (drive->un->eq == request->eq)
			break;

	if (drive == NULL) {
		ETRACE((LOG_ERR, "NfDr(%d) %d.",
		    drive->un->eq, request->eq,
		    __FILE__, __LINE__));
		send_fs_error(&request->handle, EINVAL);
		disp_of_event(library, event, ENOENT);
		return (0);
	}

	/* put this event on the worklist for the drive */
	event->next = NULL;
	event->previous = NULL;

	mutex_lock(&drive->list_mutex);
	if (drive->active_count) {
		robo_event_t *end;

		LISTEND(drive, end);
		ETRACE((LOG_NOTICE, "ApDr(%d) %#x-%#x(%d) %s:%d.",
		    drive->un->eq, end, event, drive->active_count,
		    __FILE__, __LINE__));
		append_list(end, event);
	} else {
		ETRACE((LOG_NOTICE, "FrDr(%d) %#x(%d) %s:%d.",
		    drive->un->eq, event, drive->active_count,
		    __FILE__, __LINE__));
		drive->first = event;
	}

	drive->active_count++;
	mutex_unlock(&drive->mutex);	/* release drive mutex */
	cond_signal(&drive->list_condit);
	mutex_unlock(&drive->list_mutex);
	return (0);			/* return success */
}


/*
 * unload_request - stick the event on the drive thread work list.
 */
void
unload_request(
	library_t *library,
	robo_event_t *event)
{
	char 	*l_mess = library->un->dis_mes[DIS_MES_NORM];
	char 	*ent_pnt = "unload_request";
	unload_request_t *request =
	    &event->request.message.param.unload_request;
	drive_state_t *drive = NULL;

	if ((request->eq == library->un->eq) &&
	    (request->slot == ROBOT_NO_SLOT)) {
		dstate_t lib_state;

		if (unload_lib == NULL) {
			memccpy(l_mess,
			    catgets(catfd, SET, 9243,
			    "unload library not supported"), '\0', DIS_MES_LEN);
			sam_syslog(LOG_INFO, catgets(catfd, SET, 9242,
			    "%s: Unload library not supported."), ent_pnt);
			disp_of_event(library, event, ENOENT);
			return;
		}
		lib_state = library->un->state;
		mutex_lock(&library->un->mutex);
		library->un->state = DEV_IDLE;
		library->un->status.b.requested = TRUE;
		mutex_unlock(&library->un->mutex);

		if (thr_create(NULL, DF_THR_STK, &unload_lib, (void *)library,
		    THR_DETACHED, NULL)) {
			sam_syslog(LOG_ERR,
			    "Unable to create thread unload_lib.");
			mutex_lock(&library->un->mutex);
			library->un->state = lib_state;
			mutex_unlock(&library->un->mutex);
		}
		return;
	} else if (request->slot != ROBOT_NO_SLOT) {
		/* see if the slot is in one of the drives */
		ushort_t media_element =
		    ELEMENT_ADDRESS(library, request->slot);

		if (IS_STORAGE(library, media_element))
			for (drive = library->drive; drive != NULL;
			    drive = drive->next) {
				if (drive->status.b.full &&
				    drive->status.b.valid &&
				    ELEMENT_ADDRESS(library, drive->un->slot) ==
				    media_element)
					break;
			}

		if (drive == NULL) {
			sam_syslog(LOG_INFO, catgets(catfd, SET, 9244,
			    "%s: Slot not found in drive."), ent_pnt);
			memccpy(l_mess, catgets(catfd, SET, 9245,
			    "Slot not found in drive"), '\0', DIS_MES_LEN);
			disp_of_event(library, event, ENOENT);
			return;
		}
	}
	/* find the drive */
	if (drive == NULL) {
		for (drive = library->drive; drive != NULL;
		    drive = drive->next) {
			if (drive->un->eq == request->eq)
				break;
		}
	}
	if (drive == NULL) {
		sam_syslog(LOG_INFO, catgets(catfd, SET, 9238,
		    "%s: Cannot find device %d."), ent_pnt, request->eq);
		memccpy(l_mess, catgets(catfd, SET, 9239, "cannot find device"),
		    '\0', DIS_MES_LEN);
		disp_of_event(library, event, ENOENT);
		return;
	}
	mutex_lock(&drive->list_mutex);
	if (drive->active_count) {
		robo_event_t *end;

		/* append to the end of the list */
		LISTEND(drive, end);
		ETRACE((LOG_NOTICE, "ApIm %#x-%#x(%d) %s:%d.", end, event,
		    drive->active_count, __FILE__, __LINE__));
		(void) append_list(end, event);
	} else {
		ETRACE((LOG_NOTICE, "FrIm %#x(%d) %s:%d.",
		    event, drive->active_count, __FILE__, __LINE__));
		drive->first = event;
	}

	drive->active_count++;
	cond_signal(&drive->list_condit);
	mutex_unlock(&drive->list_mutex);
}

/*
 * entry_found - function for scan_preview.
 *
 * passed to scan_preivew as the function to call when a preview entry
 * is found whos vsn/media is in this media changers' catalog.
 */
int
entry_found(
	preview_t *preview,		/* Preview entry */
	struct CatalogEntry *ce,	/* catalog entry */
	void *args,			/* any args */
	uint_t slot,			/* slot number */
	int flags)
{
	switch (flags) {
		case 0:
		{
			ushort_t flip_side;
			ushort_t element;
			library_t *library = (library_t *)args;
			drive_state_t *drive;

			/* If not this robot */
			if (preview->robot_equ != 0 &&
			    preview->robot_equ != library->un->eq)
				return (1);

			mutex_lock(&library->mutex);
			library->preview_count++;
			mutex_unlock(&library->mutex);

			element = ELEMENT_ADDRESS(library, slot);
			flip_side = FALSE;

			/*
			 * Search the drives looking for a drive with this media
			 * (either side already mounted or in the
			 * process of mounting it.
			 */
			for (drive = library->drive; drive != NULL;
			    drive = drive->next) {
				if (drive->un == NULL)
					continue;

				if (drive->un->state >= DEV_IDLE)
					continue;

				/* is there media mounted and is it this */
				/* media */
				if ((drive->status.b.valid &&
				    (ELEMENT_ADDRESS(library, drive->un->slot)
				    == element)) && drive->un->status.b.ready &&
				    !drive->un->status.b.requested) {
					flip_side = TRUE;
					break;
				}
				if ((drive->new_slot != ROBOT_NO_SLOT) &&
				    ((ELEMENT_ADDRESS(library, drive->new_slot)
				    == element) && (slot != drive->new_slot))) {
					flip_side = TRUE;
					break;
				}
			}

			mutex_lock(&preview->p_mutex);
			preview->robot_equ = library->un->eq;
			preview->slot = ce->CeSlot;
			preview->element = element;
			preview->flip_side = flip_side;
			mutex_unlock(&preview->p_mutex);
			break;
		}

	case 1:
		break;

	default:
		break;
	}
	return (0);
}


/*
 * give a drive a clean request
 */
int
clean_request(
	library_t *library,
	robo_event_t *event)
{
	char 	*l_mess = library->un->dis_mes[DIS_MES_NORM];
	robo_event_t *end;
	drive_state_t *drive;
	clean_request_t *request;

	request = &event->request.message.param.clean_request;

	for (drive = library->drive; drive != NULL; drive = drive->next) {
		if (drive->un == NULL)
			continue;

		if (drive->un->eq == request->eq)
			break;
	}

	if ((drive == NULL) || (drive->un == NULL)) {
		disp_of_event(library, event, ENOENT);
		return (0);
	}
	if (drive->un->status.b.cleaning && !event->status.b.delayed) {
#if defined(DEBUG)
		sprintf(l_mess, "duplicate request to clean drive %d",
		    drive->un->eq);
#endif
		disp_of_event(library, event, 0);
		return (0);
	}

	/* put this event on the worklist for the drive */
	event->next = NULL;
	event->previous = NULL;

	mutex_lock(&drive->list_mutex);
	if (drive->active_count) {
		LISTEND(drive, end);
		ETRACE((LOG_NOTICE, "ApDr(%d) %#x-%#x(%d) %s:%d.",
		    drive->un->eq, end, event, drive->active_count,
		    __FILE__, __LINE__));
		append_list(end, event);
	} else {
		ETRACE((LOG_NOTICE, "FrDr(%d) %#x(%d) %s:%d.",
		    drive->un->eq, event, drive->active_count,
		    __FILE__, __LINE__));
		drive->first = event;
	}

	drive->active_count++;
	mutex_unlock(&drive->mutex);	/* release drive mutex */
	cond_signal(&drive->list_condit);
	mutex_unlock(&drive->list_mutex);
	return (0);
}


/*
 * state_request - device changing states
 */
void
state_request(
	library_t *library,
	robo_event_t *event)
{
	char 		*l_mess = library->un->dis_mes[DIS_MES_NORM];
	char 		*ent_pnt = "state_request";
	robo_event_t *end;
	drive_state_t *drive;
	state_change_t *request = &event->request.message.param.state_change;

	if (request->eq != library->un->eq) {
		/* find the drive */
		for (drive = library->drive; drive != NULL;
		    drive = drive->next) {
			if (drive->un == NULL)
				continue;

			if (drive->un->eq == request->eq)
				break;
		}

		if ((drive == NULL) || (drive->un == NULL)) {
			sam_syslog(LOG_INFO,
			    catgets(catfd, SET, 9238,
			    "%s: Cannot find device %d."),
			    ent_pnt, request->eq);
			memccpy(l_mess, catgets(catfd, SET, 9239,
			    "cannot find device"), '\0', DIS_MES_LEN);
			disp_of_event(library, event, ENOENT);
			return;
		}
		mutex_lock(&drive->list_mutex);
		if (drive->active_count) {
			/* append to the end of the list */
			LISTEND(drive, end);
			ETRACE((LOG_NOTICE, "ApIm %#x-%#x(%d) %s:%d.",
			    end, event, drive->active_count,
			    __FILE__, __LINE__));
			(void) append_list(end, event);
		} else {
			ETRACE((LOG_NOTICE, "FrIm %#x(%d) %s:%d.",
			    event, drive->active_count, __FILE__, __LINE__));
			drive->first = event;
		}

		drive->active_count++;
		cond_signal(&drive->list_condit);
		mutex_unlock(&drive->list_mutex);
	} else {
		if (request->old_state >= DEV_OFF &&
		    request->state < DEV_IDLE) {
			if (request->state == DEV_ON)
				OnDevice(library->un);
			else
				library->un->state = request->state;
			re_init_library(library);
		} else if (request->state == DEV_OFF) {
			OffDevice(library->un, USER_STATE_CHANGE);
		} else if (request->state == DEV_DOWN) {
			DownDevice(library->un, USER_STATE_CHANGE);
		} else
			library->un->state = request->state;
		disp_of_event(library, event, 0);
	}

}

/*
 * tapealert_solicit - device tapealert request
 */
void
tapealert_solicit(
	library_t *library,
	robo_event_t *event)
{
	char 		*l_mess = library->un->dis_mes[DIS_MES_NORM];
	char 		*ent_pnt = "tapealert_request";
	robo_event_t 	*end;
	drive_state_t 	*drive;
	tapealert_request_t *request;

	request = &event->request.message.param.tapealert_request;

	if (request->eq != library->un->eq) {
		/* find the drive */
		for (drive = library->drive; drive != NULL;
		    drive = drive->next) {
			if (drive->un == NULL)
				continue;

			if (drive->un->eq == request->eq)
				break;
		}

		if ((drive == NULL) || (drive->un == NULL)) {
			sam_syslog(LOG_INFO,
			    catgets(catfd, SET, 9238,
			    "%s: Cannot find device %d."),
			    ent_pnt, request->eq);
			memccpy(l_mess, catgets(catfd, SET, 9239,
			    "cannot find device"), '\0', DIS_MES_LEN);
			disp_of_event(library, event, ENOENT);
			return;
		}
		mutex_lock(&drive->list_mutex);
		if (drive->active_count) {
			/* append to the end of the list */
			LISTEND(drive, end);
			ETRACE((LOG_NOTICE, "ApIm %#x-%#x(%d) %s:%d.",
			    end, event, drive->active_count,
			    __FILE__, __LINE__));
			(void) append_list(end, event);
		} else {
			ETRACE((LOG_NOTICE, "FrIm %#x(%d) %s:%d.",
			    event, drive->active_count, __FILE__, __LINE__));
			drive->first = event;
		}

		drive->active_count++;
		cond_signal(&drive->list_condit);
		mutex_unlock(&drive->list_mutex);
	} else {
		if (request->flags & TAPEALERT_ENABLED) {
			library->un->tapealert |= TAPEALERT_ENABLED;
			get_supports_tapealert(library->un, -1);
		} else {
			library->un->tapealert &= ~TAPEALERT_ENABLED;
		}
		disp_of_event(library, event, 0);
	}

}

/*
 * sef_solicit - device sef request
 */
void
sef_solicit(
	library_t *library,
	robo_event_t *event)
{
	char 		*l_mess = library->un->dis_mes[DIS_MES_NORM];
	char 		*ent_pnt = "sef_request";
	robo_event_t 	*end;
	drive_state_t 	*drive;
	sef_request_t   *request;

	request = &event->request.message.param.sef_request;

	if (request->eq != library->un->eq) {
		/* find the drive */
		for (drive = library->drive; drive != NULL;
		    drive = drive->next) {
			if (drive->un == NULL)
				continue;

			if (drive->un->eq == request->eq)
				break;
		}

		if ((drive == NULL) || (drive->un == NULL)) {
			sam_syslog(LOG_INFO,
			    catgets(catfd, SET, 9238,
			    "%s: Cannot find device %d."),
			    ent_pnt, request->eq);
			memccpy(l_mess, catgets(catfd, SET, 9239,
			    "cannot find device"), '\0', DIS_MES_LEN);
			disp_of_event(library, event, ENOENT);
			return;
		}
		mutex_lock(&drive->list_mutex);
		if (drive->active_count) {
			/* append to the end of the list */
			LISTEND(drive, end);
			ETRACE((LOG_NOTICE, "ApIm %#x-%#x(%d) %s:%d.",
			    end, event, drive->active_count,
			    __FILE__, __LINE__));
			(void) append_list(end, event);
		} else {
			ETRACE((LOG_NOTICE, "FrIm %#x(%d) %s:%d.",
			    event, drive->active_count, __FILE__, __LINE__));
			drive->first = event;
		}

		drive->active_count++;
		cond_signal(&drive->list_condit);
		mutex_unlock(&drive->list_mutex);
	}
}
