/*
 *  import2.c - second part of import.c
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

#pragma ident "$Revision: 1.51 $"

/* Using __FILE__ makes duplicate strings */
static char    *_SrcFile = __FILE__;

#include <stdio.h>
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

#include "driver/samst_def.h"
#include "sam/types.h"
#include "aml/external_data.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/dev_log.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "aml/tapes.h"
#include "aml/trace.h"
#include "sam/defaults.h"
#include "sam/nl_samfs.h"
#include "aml/proto.h"

#include "generic.h"
#include "element.h"

#include "sam/lib.h"
#include "aml/catlib.h"
#include "aml/catalog.h"
#include "aml/tapealert.h"

/*	function prototypes */
int		wait_for_iport(iport_state_t *, int, int);
int		lock_and_status_adic(library_t *);
iport_state_t  *find_empty_export(library_t *);
void	    clear_import(library_t *, iport_state_t *);
void	    init_import(iport_state_t *);
void	    start_slot_audit(library_t *, uint_t, uint_t);
void	    process_multi_import(iport_state_t *, uint_t, uint_t, media_t);
void	    process_d360_import(iport_state_t *, uint_t, uint_t, media_t);
void	    process_452_export(library_t *);
void	    wait_452_import(library_t *);
void	    unlock_adic(library_t *);

/*	Some local structs and typedefs */
/* used to link a bunch of events together */
typedef struct linked_events {
	robo_event_t    event;
	struct linked_events *next;
}		linked_events_t;

/*	some globals */
extern shm_alloc_t master_shm, preview_shm;


/*
 *	start_slot_audit - schedule an audit for the specified slot
 */
void
start_slot_audit(
		library_t *library,
		uint_t slot,
		uint_t audit_eod)
{
	robo_event_t   *new_event;
	robot_internal_t *request;

	new_event = get_free_event(library);
	request = &new_event->request.internal;
	(void) memset(new_event, 0, sizeof (robo_event_t));
	new_event->type = EVENT_TYPE_INTERNAL;
	new_event->status.bits = REST_FREEMEM;
	request->command = ROBOT_INTRL_AUDIT_SLOT;
	request->slot = slot;
	if (audit_eod)
		request->flags.b.audit_eod = TRUE;
	add_to_end(library, new_event);
}


/*
 *	process_d360_import - keep taking media till full or no more.
 *
 */
void
process_d360_import(
		    iport_state_t *iport,
		    uint_t fslot,	/* slot for first media */
		    uint_t flags,
		    media_t media)
{
	dev_ent_t	*un;
	int		status = 0;
	char	   *l_mess = iport->library->un->dis_mes[DIS_MES_NORM];
	uint_t	  slot = fslot, element;
	uint_t	  not_sam;
	uchar_t	*buffer;
	library_t	*library = iport->library;
	move_flags_t    move_flags;
	storage_element_t *storage_descrip;
	element_status_page_t *status_page;
	storage_element_ext_t *extension;

	un = library->un;
	not_sam = ((flags & CMD_IMPORT_STRANGE) != 0);
	if (wait_for_iport(iport, FULL, (60 * 5))) {
		return;
	}
	buffer = (uchar_t *)malloc_wait(library->ele_dest_len * 3, 2, 0);
	status_page = (element_status_page_t *)
	    (buffer + sizeof (element_status_data_t));
	storage_descrip = (storage_element_t *)
	    ((char *)status_page + sizeof (element_status_page_t));
	extension = (storage_element_ext_t *)
	    ((char *)storage_descrip + sizeof (storage_element_t));

	for (;;) {		/* main import loop */
		int		err;

		move_flags.bits = 0;
		move_flags.b.noerror = FALSE;
		element = ELEMENT_ADDRESS(library, slot);
		if (update_element_status(library, IMPORT_EXPORT_ELEMENT) ||
		    !iport->status.b.full)
			break;

		if ((err = move_media(library, 0, iport->element, element, 0,
		    move_flags))) {
			char	   *MES_9058 = catgets(catfd, SET, 9058,
			    "import failed, check import/export hopper");

			memccpy(l_mess, MES_9058, '\0', DIS_MES_LEN);
			DevLog(DL_ERR(5109));
			break;
		}
		memset(buffer, 0, library->ele_dest_len);
		if (read_element_status(library, STORAGE_ELEMENT,
		    element, 1, buffer,
		    library->ele_dest_len * 3) <= 0) {
			uint16_t	ele_addr, ele_dest_len;
			struct VolId    vid;
			struct CatalogEntry ced;
			struct CatalogEntry *ce = &ced;
			uint32_t	status;

			BE16toH(&status_page->ele_dest_len, &ele_dest_len);
			BE16toH(&storage_descrip->ele_addr, &ele_addr);

			memset(&vid, 0, sizeof (struct VolId));
			status = CES_inuse | CES_occupied;
			vid.ViFlags = VI_cart;
			vid.ViEq = library->un->eq;
			vid.ViSlot = SLOT_NUMBER(library, ele_addr);
			memmove(vid.ViMtype, sam_mediatoa(library->un->media),
			    sizeof (vid.ViMtype));

			if (status_page->PVol && extension != NULL) {
				dtb(&(extension->PVolTag[0]), BARCODE_LEN);
				if (status_page->AVol && extension != NULL)
					dtb(&(extension->AVolTag[0]),
					    BARCODE_LEN);
				if (is_barcode(extension->PVolTag)) {
					status |= CES_bar_code;
					(void) CatalogSlotInit(&vid, status,
					    (library->status.b.two_sided)
					    ? 2 : 0,
					    (char *)extension->PVolTag,
					    (char *)extension->AVolTag);
				} else {
					(void) CatalogSlotInit(&vid, status,
					    (library->status.b.two_sided)
					    ? 2 : 0, "", "");
				}
				if ((ce = CatalogGetEntry(&vid, &ced)) !=
				    NULL) {
					if ((!(ce->CeStatus &
					    CES_capacity_set)) &&
					    (ce->CeCapacity == 0)) {
						(void) CatalogSetFieldByLoc(
						    library->un->eq,
						    vid.ViSlot, vid.ViPart,
						    CEF_Capacity,
						    DEFLT_CAPC(
						    library->un->media), 0);
						(void) CatalogSetFieldByLoc(
						    library->un->eq,
						    vid.ViSlot, vid.ViPart,
						    CEF_Space,
						    DEFLT_CAPC(
						    library->un->media), 0);
					}
				}
			} else {
				(void) CatalogSlotInit(&vid, status,
				    (library->status.b.two_sided)
				    ? 2 : 0, "", "");
			}
		}
		if (not_sam) {
			status = CES_non_sam;
			(void) CatalogSetFieldByLoc(library->un->eq, slot, 0,
			    CEF_Status, status, CES_non_sam);
		}
		slot = FindFreeSlot(library);

		if (slot == ROBOT_NO_SLOT) {
			mutex_lock(&library->un->mutex);
			library->un->status.b.stor_full = TRUE;
			mutex_unlock(&library->un->mutex);
			return;
		}
	}			/* end of import loop */

	if (buffer != NULL)
		free(buffer);

}


/*
 *	process_452_export - prepare the 452 for export by unloading any
 * media from drives that belong in the import/export area and marking
 * all slots in that area unavailable.
 */
void
process_452_export(
		library_t *library)
{
	dev_ent_t	*un;
	int		search_drives, start_slot, end_slot;
	uint_t	  slot, flags = HIST_UPDATE_INFO;
	drive_state_t  *drive;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;

	un = library->un;
	if (library->un->dt.rb.status.b.export_unavail)
		flags |= HIST_SET_UNAVAIL;

	start_slot = library->range.storage_count;
	/* add for the import/export door slots */
	end_slot = start_slot + library->range.ie_count;
	do {
		search_drives = 0;

		for (slot = start_slot; slot < end_slot; slot++) {
			/*
			 * Set each slot unavail unless the media is in a
			 * drive
			 */
			ce = CatalogGetCeByLoc(library->un->eq, slot, 0, &ced);
			if (ce = NULL)
				continue;
			if (ce->CeStatus & CES_inuse)
				if (ce->CeStatus & CES_occupied) {
					CatalogSetFieldByLoc(un->eq, slot, 0,
					    CEF_Status, 0, 0xffff);
					CatalogSetFieldByLoc(un->eq, slot, 0,
					    CEF_Status,
					    CES_export_slot, 0);
					CatalogSetFieldByLoc(un->eq, slot, 0,
					    CEF_Status,
					    CES_unavail, 0);
				} else
					search_drives++;
			else {
				CatalogSetFieldByLoc(un->eq, slot, 0,
				    CEF_Status, 0, 0xffff);
				CatalogSetFieldByLoc(un->eq, slot, 0,
				    CEF_Status,
				    CES_export_slot, 0);
				CatalogSetFieldByLoc(un->eq, slot, 0,
				    CEF_Status,
				    CES_unavail, 0);
			}
		}
/*	end of for loop */

		if (search_drives) {
			char	   *mess = catgets(catfd, SET, 9061,
			    "unloading drives for export");
			robo_event_t   *event;
			linked_events_t *first_event = NULL, *next_event = NULL;

			memccpy(library->un->dis_mes[DIS_MES_NORM], mess,
			    '\0', DIS_MES_LEN);
			DevLog(DL_DETAIL(5072));

			/*
			 * Start an event on each drive to unload the media
			 * if the media lives in the import/export area.
			 */
			for (drive = library->drive; drive != NULL;
			    drive = drive->next) {
				mutex_lock(&drive->un->mutex);
				if (drive->status.b.full &&
				    !drive->un->status.b.requested &&
				    (drive->media_element >=
				    library->range.ie_lower &&
				    drive->media_element <=
				    library->range.ie_upper)) {
					drive->un->status.b.requested = TRUE;
					mutex_unlock(&drive->un->mutex);
					if (first_event == NULL)
					first_event = next_event =
					    (linked_events_t *)malloc_wait(
					    sizeof (linked_events_t), 2, 0);
					else {
					next_event->next =
					    (linked_events_t *)malloc_wait(
					    sizeof (linked_events_t), 2, 0);
					next_event = next_event->next;
					}
					memset(next_event, 0,
					    sizeof (linked_events_t));
					event = &next_event->event;
					event->status.b.sig_cond = TRUE;
					event->type = EVENT_TYPE_MESS;
					event->request.message.magic =
					    MESSAGE_MAGIC;
					event->request.message.command =
					    MESS_CMD_UNLOAD;
					event->completion =
					    REQUEST_NOT_COMPLETE;
					mutex_lock(&drive->list_mutex);
					if (drive->active_count) {
						robo_event_t   *end;
						/*
						 * append to the end of the
						 * list
						 */
						LISTEND(drive, end);
					ETRACE((LOG_NOTICE,
					    "ApDr(%d) %#x-%#x(%d) %s:%d.",
					    drive->un->eq, end, event,
					    drive->active_count,
					    __FILE__, __LINE__));
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
				} else
					mutex_unlock(&drive->un->mutex);
			}

			next_event = first_event;
			/* wait for all drives to unload */
			while (next_event != NULL) {
				linked_events_t *last_event;

				mutex_lock(&next_event->event.mutex);
				while (next_event->event.completion ==
				    REQUEST_NOT_COMPLETE)
					cond_wait(&next_event->event.condit,
					    &next_event->event.mutex);
				mutex_unlock(&next_event->event.mutex);
				mutex_destroy(&next_event->event.mutex);
				cond_destroy(&next_event->event.condit);
				last_event = next_event;
				next_event = next_event->next;
				free(last_event);
			}
		}
	} while (search_drives > 0);
}


/*
 *	wait_452_import - wait for the 452 import door to be closed
 *
 */
void
wait_452_import(
		library_t *library)
{
	ushort_t	slot, start_slot, end_slot;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;

	start_slot = library->range.storage_count;
	/* add for the import/export door slots */
	end_slot = start_slot + library->range.ie_count;
	mutex_unlock(&library->un->mutex);
	library->un->status.b.attention = TRUE;
	library->un->status.b.i_e_port = TRUE;
	LOCK_MAILBOX(library);
	mutex_unlock(&library->un->mutex);

	for (slot = start_slot; slot < end_slot; slot++) {
		ce = CatalogGetCeByLoc(library->un->eq, slot, 0, &ced);
		if (ce == NULL)
			continue;
		CatalogSetFieldByLoc(library->un->eq, slot, 0,
		    CEF_Status, 0, 0xffff);
		CatalogSetFieldByLoc(library->un->eq, slot, 0,
		    CEF_Status, CES_unavail, 0);
	}
	do {
		/*
		 * History is taken care of in update_element_status for the
		 * 452
		 */
		(void) update_element_status(library, IMPORT_EXPORT_ELEMENT);
		if (ce->CeStatus & CES_unavail)
			sleep(30);	/* not shut yet */
		else {
			mutex_unlock(&library->un->mutex);
			library->un->status.b.attention = FALSE;
			library->un->status.b.i_e_port = FALSE;
			mutex_unlock(&library->un->mutex);
			return;
		}
	} while (TRUE);
}


/*
 *	init_import - initialize an import(/export) drawer.
 *
 */
void
init_import(
	    iport_state_t *iport)
{
	dev_ent_t	*un;
	iport_state_t  *tmp;

	/*
	 * Clear (empty) the import element. The IBM 3584 is not cleared
	 * because it can be partitioned and clearing may steal another
	 * system's cartridges STK97xx can have multiple I/E areas
	 */
	un = iport->library->un;
	if (iport->library->un->type != DT_ACL452 &&
	    iport->library->un->type != DT_ADIC448 &&
	    iport->library->un->type != DT_ATLP3000 &&
	    iport->library->un->type != DT_IBM3584 &&
	    iport->library->un->type != DT_SL3000 &&
	    iport->library->un->type != DT_SONYDMS &&
	    !(iport->library->un->type == DT_STK97XX &&
	    iport->library->range.ie_count > 1)) {
		for (tmp = iport->library->import; tmp != NULL;
		    tmp = tmp->next) {
			clear_import(iport->library, tmp);
		}
	}
	mutex_lock(&iport->library->un->mutex);

	/*
	 * Determine which elements are for importing and which are for
	 * exporting
	 */
	iport->library->im_ele = iport->library->ex_ele = NULL;
	switch (iport->library->un->type) {
	case DT_ACL2640:
		for (tmp = iport; tmp != NULL; tmp = tmp->next) {
			/*
			 * if import enabled and not a passthrough then this
			 * is the import element
			 */
			if (tmp->status.b.inenab &&
			    ((tmp->element & 0xff) != 0x42))
				iport->library->im_ele = tmp;

			/*
			 * if export enabled and not a passthrough then this
			 * is the export element
			 */
			if (tmp->status.b.exenab &&
			    ((tmp->element & 0xff) != 0x42))
				iport->library->ex_ele = tmp;
		}
		break;

		/*
		 * Always start with the first import/export on the ADIC,
		 * stk97xx and acl452
		 */
	case DT_SONYDMS:
		/* Fallthrough */
	case DT_STK97XX:
		/* Fallthrough */
	case DT_STKLXX:
		/* Fallthrough */
	case DT_ADIC100:
		/* Fallthrough */
	case DT_ADIC448:
		/* Fallthrough */
	case DT_ADIC1000:
		/* Fallthrough */
	case DT_EXBX80:
		/* Fallthrough */
	case DT_IBM3584:
		/* Fallthrough */
	case DT_HP_C7200:
		/* Fallthrough */
	case DT_ATLP3000:
		/* Fallthrough */
	case DT_ACL452:
		/* Fallthrough */
	case DT_PLASMON_G:
		/* Fallthrough */
	case DT_QUANTUMC4:
		/* Fallthrough */
	case DT_HPSLXX:
		/* Fallthrough */
	case DT_FJNMXX:
		/* Fallthrough */
	case DT_SL3000:
		/* Fallthrough */
	case DT_ODI_NEO:
		iport->library->im_ele = iport;
		iport->library->ex_ele = iport;
		break;

	default:
		for (tmp = iport; tmp != NULL; tmp = tmp->next) {
			if (tmp->status.b.inenab)
				iport->library->im_ele = tmp;
			if (tmp->status.b.exenab)
				iport->library->ex_ele = tmp;
		}
		break;
	}

	if (iport->library->im_ele == NULL || iport->library->ex_ele == NULL) {
		DevLog(DL_ALL(5100));
	}
	/*
	 * Set up mailbox to prevent operator from inserting media without
	 * our knoweldge.
	 */
	(void) LOCK_MAILBOX(iport->library);

	/* if its a 452 and the door is open, then set up for close */
	mutex_unlock(&iport->library->un->mutex);
	if (iport->library->un->type == DT_ACL452 && !iport->status.b.access)
		wait_452_import(iport->library);

}


/*
 *	schedule_export - put an export request in the libraries message
 * queue.  The library thread will pick it up and put it on the
 * event list for the transport(which is currently busy executing
 * this code).
 */
void
schedule_export(
		library_t *library,
		uint_t slot)
{
	dev_ent_t	*un = library->un;
	message_request_t *message;
	export_request_t *request;

	message = (message_request_t *)SHM_REF_ADDR(un->dt.rb.message);
	request = &message->message.param.export_request;

	mutex_lock(&message->mutex);
	while (message->mtype != MESS_MT_VOID)
		cond_wait(&message->cond_i, &message->mutex);

	(void) memset(&message->message, 0, sizeof (sam_message_t));
	/* LINTED constant truncated by assignment */
	message->message.magic = MESSAGE_MAGIC;
	message->message.command = MESS_CMD_EXPORT;
	request->eq = un->eq;
	request->flags = EXPORT_BY_SLOT;
	request->slot = slot;
	message->mtype = MESS_MT_NORMAL;
	cond_signal(&message->cond_r);
	mutex_unlock(&message->mutex);
}


/*
 *	process_multi_import - scan through the a media changers import/export
 * elements looking for full
 *
 */
void
process_multi_import(
			iport_state_t *iport,
			uint_t fslot,	/* slot for first media */
			uint_t flags,
			media_t media)
{
	dev_ent_t	*un;
	int		first_element = TRUE;
	uint_t	  slot = fslot, element;
	uint_t	  not_sam;
	dtype_t	 equ_type;
	library_t	*library = iport->library;
	move_flags_t    move_flags;
	iport_state_t  *next_iport = iport;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	int		media_imported = 0;
	char	   *l_mess;

	un = library->un;
	not_sam = ((flags & CMD_IMPORT_STRANGE) != 0);
	equ_type = library->un->equ_type;
	l_mess = library->un->dis_mes[DIS_MES_NORM];

	memccpy(l_mess, catgets(catfd, SET, 9053, "place media in import area"),
	    '\0', DIS_MES_LEN);

	do {
		/*
		 * Wait for operator to import media before we lock the door.
		 */

		if (equ_type == DT_PLASMON_G) {
			sleep(60);
		}

		/*
		 * The adic cannot hold the door locked for longer than 2
		 * moves so it must be locked and unlocked around each move.
		 * Also, since it must be unlocked, then there is a chance
		 * that the operator did something, so status_element_range
		 * must be issued for each pass while the door is locked.
		 */
		if (equ_type == DT_ADIC448) {
			if (lock_and_status_adic(library)) {
				return;
			}
		} else {
			/*
			 *  On the Plasmon-G, we have to make sure
			 *  the magazine, if any, is locked.
			 */
			if (equ_type == DT_PLASMON_G) {
				LOCK_MAILBOX(library);
			}
			status_element_range(library,
			    IMPORT_EXPORT_ELEMENT,
			    library->range.ie_lower,
			    library->range.ie_count);
		}

		for (next_iport = iport; next_iport != NULL &&
		    slot != ROBOT_NO_SLOT;
		    next_iport = next_iport->next) {

			/*
			 * Must be full and put there by the operator and
			 * accessable The Sony and P3000 don't support this
			 * feature The ADIC 100 supports this feature except
			 * on the first slot, we have to ignore it on all
			 * slots.
			 */
			if (next_iport->status.b.full &&
			    (((equ_type == DT_SONYDMS) ||
			    (equ_type == DT_ATLP3000) ||
			    (equ_type == DT_ADIC100) ||
			    (equ_type == DT_ODI_NEO) ||
			    (equ_type == DT_IBM3584) ||
			    (equ_type == DT_EXBX80) ||
			    (equ_type == DT_QUANTUMC4) ||
			    (equ_type == DT_HPSLXX) ||
			    (equ_type == DT_FJNMXX) ||
			    (equ_type == DT_SL3000) ||
			    (equ_type == DT_STKLXX &&
			    strncmp((char *)un->product_id,
			    "SL500", 5) == 0)) ||
			    next_iport->status.b.impexp) &&
			    next_iport->status.b.access) {
				media_imported = 1;
				if (!first_element && equ_type == DT_ADIC448 &&
				    lock_and_status_adic(library)) {
					return;
				}
				first_element = FALSE;
				move_flags.bits = 0;
				move_flags.b.noerror = TRUE;
				element = ELEMENT_ADDRESS(library, slot);

				if (move_media(library, 0, next_iport->element,
				    element, 0,
				    move_flags)) {
					/*
					 * clear the reservations in the
					 * catalog
					 */
					if (equ_type == DT_ADIC448)
						unlock_adic(library);
					return;
				}
				if (equ_type == DT_ADIC448)
					unlock_adic(library);

				/* get the bar_code and stuff */
				memccpy(l_mess, catgets(catfd, SET, 9008,
				    "read barcode"),
				    '\0', DIS_MES_LEN);
				if (status_element_range(library,
				    STORAGE_ELEMENT, element, 1)) {
					int		status = 0;

					DevLog(DL_ERR(5073));
				memccpy(l_mess, catgets(catfd, SET, 9059,
				    "import halted, check log"),
				    '\0', DIS_MES_LEN);

					/* Schedule an export for this media */
					status |= (CES_occupied & CES_inuse &
					    CES_bad_media);
					CatalogSetFieldByLoc(un->eq, slot, 0,
					    CEF_Status, status, 0);
					schedule_export(library, slot);
					/* break import loop */
					break;
				}
				ce = CatalogGetCeByLoc(un->eq, slot, 0, &ced);
				if (ce == NULL)
					break;

				if (not_sam) {
					CatalogSetFieldByLoc(un->eq, slot, 0,
					    CEF_Status,
					    CES_non_sam, 0);
				} else {
					char *MES_9063 = catgets(
					    catfd, SET, 9063, "imported media"
					    " (slot %d) has barcode %s");
					char *mess = (char *)
					    malloc_wait(strlen(MES_9063) + 10 +
					    BARCODE_LEN + 1, 2, 0);

					sprintf(mess, MES_9063, slot,
					    ce->CeBarCode);
					memccpy(l_mess, mess, '\0',
					    DIS_MES_LEN);
					DevLog(DL_DETAIL(5102),
					    slot, ce->CeBarCode);
					free(mess);
				}



				/*
				 * If the entry's capacity is zero, and not
				 * set to zero by the user, set the capacity
				 * to default.
				 */
				ce = CatalogGetCeByLoc(library->un->eq,
				    slot, 0, &ced);
				if (ce != NULL) {
					if ((!(ce->CeStatus &
					    CES_capacity_set)) &&
					    (ce->CeCapacity == 0)) {
						(void) CatalogSetFieldByLoc(
						    library->un->eq,
						    slot, 0,
						    CEF_Capacity,
						    DEFLT_CAPC(library->un->
						    media), 0);
						(void) CatalogSetFieldByLoc(
						    library->un->eq,
						    slot, 0,
						    CEF_Space,
						    DEFLT_CAPC(library->un->
						    media), 0);
					}
				}
				slot = FindFreeSlot(library);

				if (slot == ROBOT_NO_SLOT) {
					memccpy(l_mess,
					    catgets(catfd, SET, 9059,
					    "import halted, check log"),
					    '\0', DIS_MES_LEN);
					mutex_lock(&library->un->mutex);
					library->un->status.b.stor_full = TRUE;
					mutex_unlock(&library->un->mutex);
					return;
				}
			}
		}

		if (equ_type == DT_ADIC448)
			unlock_adic(library);
		if (equ_type == DT_PLASMON_G) {
			UNLOCK_MAILBOX(library);
		}
	} while (equ_type == DT_PLASMON_G && media_imported == 0);
}

/*
 *	find_empty_export - find an empty export element
 */
iport_state_t  *
find_empty_export(
		library_t *library)
{
	iport_state_t  *next_iport;

	if (library->un->equ_type == DT_ADIC448) {
		if (lock_and_status_adic(library))
			return (NULL);
	} else
		status_element_range(library, IMPORT_EXPORT_ELEMENT,
		    library->range.ie_lower, library->range.ie_count);

	next_iport = library->import;
	for (; next_iport != NULL; next_iport = next_iport->next) {
		if (!next_iport->status.b.full)
			break;
	}
	return (next_iport);
}


/*
 *	lock_and_status_adic - lock the door and status the import/export
 * elements.
 *
 * return 0 if ok
 *		  not 0 if failure
 */
int
lock_and_status_adic(
			library_t *library)
{
	dev_ent_t	*un;
	int		retry = 4;
	char	   *l_mess = library->un->dis_mes[DIS_MES_NORM];
	char	   *lc_mess = library->un->dis_mes[DIS_MES_CRIT];
	sam_extended_sense_t *sense = (sam_extended_sense_t *)
	    SHM_REF_ADDR(library->un->sense);
	int		err;

	un = library->un;
	sleep(3);
	mutex_lock(&library->un->io_mutex);
	while (retry--) {
		memset(sense, 0, sizeof (sam_extended_sense_t *));
		TAPEALERT(library->open_fd, library->un);
		err = scsi_cmd(library->open_fd, library->un,
		    SCMD_DOORLOCK, 0, LOCK);
		TAPEALERT(library->open_fd, library->un);
		if (err != 0) {
			if (sense->es_key == KEY_UNIT_ATTENTION &&
			    sense->es_add_code == 0x28 &&
			    sense->es_qual_code == 0x01)
				continue;

			if (sense->es_key == KEY_NOT_READY &&
			    sense->es_add_code == 0x04 &&
			    sense->es_qual_code == 0x82) {
				memccpy(lc_mess, catgets(catfd, SET, 9168,
				    "close import/export door"),
				    '\0', DIS_MES_LEN);
				sleep(30);
				*lc_mess = '\0';
				continue;
			} else {
				DevLog(DL_ERR(5114));
				memccpy(l_mess, catgets(catfd, SET, 9169,
				    "unable to lock import/export door"),
				    '\0', DIS_MES_LEN);
				mutex_unlock(&library->un->io_mutex);
				DevLogSense(un);
				DevLogCdb(un);
				return (-1);
			}
		} else
			break;
	}

	mutex_unlock(&library->un->io_mutex);
	if (retry == 0)
		return (-1);

	if (status_element_range(library, IMPORT_EXPORT_ELEMENT,
	    library->range.ie_lower, library->range.ie_count)) {
		unlock_adic(library);
		return (-1);
	}
	return (0);
}


/*
 *	unlock_adic - unlock the import/export door
 */
void
unlock_adic(
	    library_t *library)
{
	mutex_lock(&library->un->io_mutex);
	TAPEALERT(library->open_fd, library->un);
	scsi_cmd(library->open_fd, library->un, SCMD_DOORLOCK, 0, UNLOCK);
	TAPEALERT(library->open_fd, library->un);
	mutex_unlock(&library->un->io_mutex);
}
