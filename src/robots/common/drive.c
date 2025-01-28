/*
 *	drive.c - thread that watches over drives
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

#pragma ident "$Revision: 1.60 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <thread.h>
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
#include "sam/custmsg.h"
#include "sam/param.h"
#include "aml/device.h"
#include "sam/lib.h"
#include "aml/dev_log.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "aml/labels.h"
#include "sam/defaults.h"
#include "sam/nl_samfs.h"
#include "../generic/generic.h"
#include "aml/trace.h"
#include "aml/odlabels.h"
#include "aml/tplabels.h"
#include "aml/proto.h"
#include "driver/samst_def.h"
#include "drive.h"
#include "aml/catalog.h"
#include "aml/catlib.h"


/*	some globals */
extern pid_t mypid;
extern shm_alloc_t master_shm, preview_shm;

/* DEV_ENT - given an equipment ordinal, return the dev_ent */
#define	DEV_ENT(a) ((dev_ent_t *)SHM_REF_ADDR(((dev_ptr_tbl_t *)SHM_REF_ADDR( \
	((shm_ptr_tbl_t *)master_shm.shared_memory)->dev_table))->d_ent[(a)]))

void *
drive_thread(
	void *vdrive)
{
	int 		exit_status = 0;
	char 		*d_mess;
	char 		*idle_mess, *empty_mess;
	uchar_t 	timeout = FALSE, chk_req;
	time_t 		last_active, now;
	dev_ent_t 	*un;
	timestruc_t 	wait_time;
	robo_event_t 	*event;
	drive_state_t 	*drive = (drive_state_t *)vdrive;
	state_change_t 	*dummy_change;
	sam_defaults_t 	*defaults;

	mutex_lock(&drive->mutex);	/* wait for go */
	mutex_unlock(&drive->mutex);

	idle_mess = catgets(catfd, SET, 9205, "idle");
	empty_mess = catgets(catfd, SET, 9206, "empty");
	defaults = GetDefaults();
	un = drive->un;
	d_mess = un->dis_mes[DIS_MES_NORM];
	dummy_change = (state_change_t *)malloc_wait(
	    sizeof (state_change_t), 2, 0);
	memset(dummy_change, 0, sizeof (state_change_t));
	dummy_change->eq = un->eq;
	dummy_change->state = DEV_OFF;
	dummy_change->old_state = DEV_IDLE;

	chk_req = FALSE;
	for (;;) {
		/* If nothing on our list, then send library a wakeup. */

		if (drive->active_count == 0 && !timeout) {
			if (un->state == DEV_IDLE)
				drive_state_change(drive, dummy_change);

			mutex_lock(&drive->library->list_mutex);
			drive->library->chk_req = chk_req;
			chk_req = FALSE;
			cond_signal(&drive->library->list_condit);
			mutex_unlock(&drive->library->list_mutex);
		}
		wait_time.tv_nsec = 0;
		mutex_lock(&drive->list_mutex);
		if ((un->tapeclean & TAPECLEAN_AUTOCLEAN) &&
		    un->status.b.cleaning && !drive->status.b.cln_inprog) {
			robo_event_t *new_event;

			new_event = (robo_event_t *)malloc_wait(
			    sizeof (robo_event_t), 5, 0);
			(void) memset(new_event, 0, sizeof (robo_event_t));
			new_event->request.message.param.clean_request.eq =
			    un->eq;
			new_event->request.message.command = MESS_CMD_CLEAN;
			/* LINTED constant truncated by assignment */
			new_event->request.message.magic = MESSAGE_MAGIC;
			new_event->type = EVENT_TYPE_MESS;
			new_event->status.bits = (REST_FREEMEM | REST_DELAYED);
			new_event->timeout = time(NULL);
			drive->status.b.cln_inprog = TRUE;
			add_to_end(drive->library, new_event);
		}
		if (drive->active_count == 0) {
			wait_time.tv_sec = time(NULL) + 10;
			if (cond_timedwait(&drive->list_condit,
			    &drive->list_mutex, &wait_time) == ETIME && TRUE)
				timeout = TRUE;
			else
				timeout = FALSE;
		}
		time(&now);
		/* Check for inactive drive */
		if (drive->active_count == 0) {
			mutex_unlock(&drive->list_mutex);
			/*
			 * Quickly check without mutexs. During stage and audit
			 * the mutex is set and the timeout will happen
			 * incorrectly.
			 */
			if (un->open_count || un->active || un->status.b.audit)
				last_active = now;

			if (drive->status.b.full && un->state < DEV_IDLE &&
			    !mutex_trylock(&drive->mutex)) {
				if (!mutex_trylock(&un->mutex)) {

		/* N.B. Bad indentation here to meet cstyle requirements */

		if (un->open_count || un->active ||
		    (un->status.bits & (DVST_REQUESTED | DVST_AUDIT)))
			last_active = now;
		else {
			preview_tbl_t *preview_tbl = &((shm_preview_tbl_t *)
			    (preview_shm.shared_memory))->preview_table;

			if (((defaults->idle_unload && timeout &&
			    (now - last_active) >= defaults->idle_unload)) ||
			    ((defaults->shared_unload &&
			    (drive->un->flags & DVFG_SHARED) &&
			    (now - last_active) >= defaults->shared_unload))) {
				un->status.bits |= DVST_REQUESTED;
				/* force dismount now */
				un->mtime = 0;
				mutex_unlock(&un->mutex);
				clear_drive(drive);
				chk_req = TRUE;
				*d_mess = '\0';
				mutex_lock(&un->mutex);
				un->status.bits &= ~DVST_REQUESTED;
				if (drive->open_fd >= 0) {
					DevLog(DL_DETAIL
					    (4001), drive->open_fd,
					    un->open_count);
					close_unit(un, &drive->open_fd);
				}
			} else if (preview_tbl->ptbl_count != 0) {
				chk_req = TRUE;
				timeout = FALSE;
			}
			memccpy(d_mess, drive->status.b.full ?
			    idle_mess : empty_mess, '\0', DIS_MES_LEN);
		}
		mutex_unlock(&un->mutex);

				}
				mutex_unlock(&drive->mutex);
			} else if (!drive->status.b.full &&
			    !un->status.b.requested)
				memccpy(d_mess, empty_mess, '\0', DIS_MES_LEN);
			continue;
		}

		/* pick up the event and act on it */
		last_active = now;
		if ((event = drive->first) == NULL) {
			DevLog(DL_DETAIL(4002), drive->active_count);
			if (drive->active_count)
				drive->active_count--;
			mutex_unlock(&drive->list_mutex);
			continue;
		}
		drive->first = event->next;
		drive->active_count--;
		mutex_unlock(&drive->list_mutex);
		event->next = NULL;

		ETRACE((LOG_NOTICE, "EvDr(%d) %#x(%#x) - ", un->eq,
		    event, (event->type == EVENT_TYPE_MESS) ?
		    event->request.message.command :
		    event->request.internal.command));

		switch (event->type) {
		case EVENT_TYPE_INTERNAL:
			switch (event->request.internal.command) {
			case ROBOT_INTRL_START_AUDIT:
				/* FALLTHROUGH */
			case ROBOT_INTRL_AUDIT_SLOT:
				audit(drive, event->request.internal.slot,
				    event->request.internal.flags.b.audit_eod);
				last_active = time(&now);
				disp_of_event(drive->library, event, 0);
				break;

			case ROBOT_INTRL_INIT:
				init_drive(drive);
				last_active = now;
				disp_of_event(drive->library, event, 0);
				break;

			case ROBOT_INTRL_SHUTDOWN:
				drive->thread = (thread_t)- 1;
				thr_exit(&exit_status);
				break;

			case ROBOT_INTRL_MOUNT:
				/*
				 * mount will call disp_of_event.
				 * Since internal mounts are only generated due
				 * to preview activity, the preview entry will
				 * have its busy bit cleared and the event will
				 * be disposed of.
				 */
				mount(drive, event);
				break;

			default:
				break;
			}
			break;

		case EVENT_TYPE_MESS:
			if (event->request.message.magic != MESSAGE_MAGIC)
				break;

			switch (event->request.message.command) {
			case MESS_CMD_SHUTDOWN:
				drive->thread = (thread_t)- 1;
				thr_exit(&exit_status);
				break;

			case MESS_CMD_STATE:
				drive_state_change(drive,
				    &event->request.message.param.state_change);

				disp_of_event(drive->library, event, 0);
				break;

			case MESS_CMD_TAPEALERT:
				drive_tapealert_solicit(drive,
				    &event->request.message.param.
				    tapealert_request);
				disp_of_event(drive->library, event, 0);
				break;

			case MESS_CMD_SEF:
				drive_sef_solicit(drive,
				    &event->request.message.param.sef_request);
				disp_of_event(drive->library, event, 0);
				break;

			case MESS_CMD_UNLOAD:
				mutex_lock(&drive->mutex);
				un->status.bits |= DVST_REQUESTED;
				(void) clear_drive(drive);
				un->status.bits &= ~DVST_REQUESTED;
				if (drive->open_fd >= 0) {
					(void) close(drive->open_fd);
					mutex_lock(&un->mutex);
					DEC_OPEN(un);
					DevLog(DL_DETAIL(4003),
					    drive->open_fd, un->open_count);
					(void) clear_un_fields(un);
					un->slot = ROBOT_NO_SLOT;
					un->mid = ROBOT_NO_SLOT;
					un->label_time = 0;
					mutex_unlock(&un->mutex);
					drive->open_fd = -1;
				} else {
					mutex_lock(&un->mutex);
					(void) clear_un_fields(un);
					un->slot = ROBOT_NO_SLOT;
					un->mid = ROBOT_NO_SLOT;
					un->label_time = 0;
					mutex_unlock(&un->mutex);
				}
				mutex_unlock(&drive->mutex);
				chk_req = TRUE;
				disp_of_event(drive->library, event, 0);
				break;

			case MESS_CMD_LABEL:
				/*
				 * label_slot will call disp_of_event or place
				 * the event back on the library list.
				 */
				label_slot(drive, event);
				break;

			case MESS_CMD_MOUNT:
				/*
				 * mount will call disp_of_event or place the
				 * event back on the library list.
				 */
				mount(drive, event);
				break;

			case MESS_CMD_LOAD_UNAVAIL:
				/*
				 * mount_unavail will call disp_of_event or
				 * place the event back on the library list.
				 */
				load_unavail(drive, event);
				break;

			case MESS_CMD_CLEAN:
				/*
				 * If drive is busy for stage or etc.
				 * delay the event and try later.
				 * clean will call disp_of_event.
				 */
				mutex_lock(&un->mutex);
				if (un->active > 0) {
					event->status.b.delayed = TRUE;
					event->timeout = time(NULL) + 10;
					add_to_end(drive->library, event);
					mutex_unlock(&un->mutex);
				} else {
					mutex_unlock(&un->mutex);
					clean(drive, event);
				}
				break;

			case MESS_CMD_TODO:
				/*
				 * todo will call disp_of_event or place the
				 * event back on the library list.
				 */
				todo(drive, event);
				break;

			default:
				break;
			}
			break;

		default:
			DevLog(DL_ERR(4004), event->type);
			break;
		}
	}
}


/*
 *	mount - mount the requested media.
 * The requested bits in the device status structure should have been
 * set before posting the mount request.
 *
 */
void
mount(
	drive_state_t *drive,
	robo_event_t *event)
{
	char 		*ent_pnt = "mount";
	int 		err, rc;
	int		partition;
	int		non_label_status;
	char 		*d_mess = drive->un->dis_mes[DIS_MES_NORM];
	vsn_t 		vsn;
	uint_t 		slot;
	media_t 	media;
	dev_ent_t 	*un = drive->un;
	sam_defaults_t 	*defaults;
	mount_request_t *request;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;

	vsn[0] = '\0';
	defaults = GetDefaults();

	if (event->type == EVENT_TYPE_INTERNAL) {
		slot = event->request.internal.slot;
		partition =  event->request.internal.part;
	} else if (event->type == EVENT_TYPE_MESS) {
		request = &event->request.message.param.mount_request;
		slot = request->slot;
		partition = request->part;
		media = request->media;
		memcpy(vsn, request->vsn, sizeof (vsn_t));
	} else {
		/*
		 * This is a condition that should never happen.
		 * Issue message in English. Will leave any preview entry
		 * busy but since the request in not valid, we can't determine
		 * if there is a preview entry.
		 */
		DevLog(DL_ERR(4005), event->type);
		drive->new_slot = ROBOT_NO_SLOT;
		disp_of_event(drive->library, event, EINVAL);
		mutex_lock(&un->mutex);
		un->status.bits &= ~DVST_REQUESTED;
		mutex_unlock(&un->mutex);
		return;
	}

	if (slot == ROBOT_NO_SLOT || un->state > DEV_IDLE) {
		if (slot == ROBOT_NO_SLOT) {
			DevLog(DL_ERR(4006), slot,
			    (vsn[0] == '\0') ? "NONE" : vsn);
		} else {
			DevLog(DL_ERR(4025), un->eq);
		}
		/*
		 * If this mount request came from the preview table,
		 * then me must clear the busy flag.
		 */
		clear_busy_bit(event, drive, slot);
		/* Clear reservation for the drive */
		mutex_lock(&un->mutex);
		un->status.bits &= ~DVST_REQUESTED;
		mutex_unlock(&un->mutex);
		disp_of_event(drive->library, event, 0);
		drive->new_slot = ROBOT_NO_SLOT;
		return;
	}

	if (!(un->status.bits & DVST_REQUESTED)) {
		DevLog(DL_DEBUG(4007));
	}

	if (vsn[0] == '\0') {
		DevLog(DL_DETAIL(4008), slot, partition, "NONE",
		    event->request.internal.address);
		ce = CatalogGetCeByLoc(
		    drive->library->un->eq, slot, partition, &ced);
	} else {
		DevLog(DL_DETAIL(4008), slot, partition, vsn,
		    event->request.internal.address);
		ce = CatalogGetCeByMedia(sam_mediatoa(media), vsn, &ced);
	}

	if (ce == NULL) {
		DevLog(DL_ERR(4006), slot, (vsn[0] == '\0') ? "NONE" : vsn);
		clear_busy_bit(event, drive, slot);
		mutex_lock(&un->mutex);
		un->status.bits &= ~DVST_REQUESTED;
		mutex_unlock(&un->mutex);
		disp_of_event(drive->library, event, 0);
		drive->new_slot = ROBOT_NO_SLOT;
		return;
	}

	if (!(ce->CeStatus & CES_inuse)) {
		DevLog(DL_ERR(4009), ce->CeSlot, ce->CeVsn);
		clear_busy_bit(event, drive, ce->CeSlot);
		mutex_lock(&un->mutex);
		un->status.bits &= ~DVST_REQUESTED;
		mutex_unlock(&un->mutex);
		disp_of_event(drive->library, event, 0);
		drive->new_slot = ROBOT_NO_SLOT;
		return;
	}

	mutex_lock(&drive->mutex);
	if ((err = get_media(drive->library, drive, event, ce)) !=
	    RET_GET_MEDIA_SUCCESS) {
		/* Only log this message if the error is fatal */
		if (IS_GET_MEDIA_FATAL_ERROR(err)) {
			DevLog(DL_ERR(4010), err, ce->CeSlot, ce->CeVsn);
		}
		memset(&un->vsn, 0, sizeof (vsn_t));
		if (err == RET_GET_MEDIA_RET_ERROR_BAD_MEDIA ||
		    err == RET_GET_MEDIA_RET_ERROR) {
			/*
			 * Let the caller know the request failed
			 * and dispose of the event after clearing
			 * the busy bit.
			 */
			if (event->type == EVENT_TYPE_INTERNAL &&
			    event->request.internal.address != NULL) {
				remove_preview_ent((preview_t *)(
				    event->request.internal.address),
				    NULL, PREVIEW_NOTIFY, EIO);
			}
		}
		clear_busy_bit(event, drive, ce->CeSlot);
		clear_driver_idle(drive, drive->open_fd);
		mutex_lock(&un->mutex);
		close_unit(un, &drive->open_fd);
		DEC_ACTIVE(un);
		un->status.bits &= ~DVST_REQUESTED;
		if (err == RET_GET_MEDIA_DOWN_DRIVE) {
			DevLog(DL_ERR(4011));
			SendCustMsg(HERE, 9360, ce->CeVsn, drive->un->eq);
			DownDevice(un, SAM_STATE_CHANGE);
		}
		if (IS_GET_MEDIA_FATAL_ERROR(err)) {
			err = -err;
		}
		mutex_unlock(&un->mutex);
		drive->new_slot = ROBOT_NO_SLOT;
		mutex_unlock(&drive->mutex);
		if (err == RET_GET_MEDIA_DISPOSE ||
		    err == RET_GET_MEDIA_RET_ERROR) {
			disp_of_event(drive->library, event, EIO);
		}
		return;
	}

	/*
	 * get_media retuns with an open fd
	 * and the acitve count incremented.
	 */
	DevLog(DL_DETAIL(4012), ce->CeSlot, ce->CeVsn);

	drive->new_slot = ROBOT_NO_SLOT;
	mutex_unlock(&drive->mutex);
	mutex_lock(&un->mutex);
	if (ce->CeStatus & CES_non_sam)
		un->status.bits |= DVST_STRANGE;

	un->status.bits |= DVST_SCANNING;
	mutex_unlock(&un->mutex);

	/*	If NO_MEDIA - close, etc and return */
	/* spin_drive() prints error - downs drive or media */
	if ((rc = spin_drive(drive, SPINUP, NOEJECT))) {
		/* On media errors - we need to remove the preview entry */
		if (rc == BAD_MEDIA) {
			/* Let the caller know that it failed */
			remove_preview_ent(
			    (preview_t *)(event->request.internal.address),
			    NULL, PREVIEW_NOTIFY, EIO);
		} else {
			/* clean up preview */
			clear_busy_bit(event, drive, ce->CeSlot);
		}
		mutex_lock(&drive->mutex);
		clear_driver_idle(drive, drive->open_fd);
		mutex_lock(&un->mutex);
		DEC_ACTIVE(un);
		un->status.bits &= ~(DVST_SCANNING | DVST_REQUESTED);
		mutex_unlock(&un->mutex);
		clear_drive(drive); /* put media away - requires drive->mutex */
		mutex_lock(&un->mutex);
		close_unit(un, &drive->open_fd);	/* requires un->mutex */
		mutex_unlock(&drive->mutex);
		mutex_unlock(&un->mutex);
		if (rc != DOWN_EQU)
			disp_of_event(drive->library, event, EIO);
		return;
	}
	if (!(un->status.bits & DVST_STRANGE)) {
		/* scan_a_device expects that the VSN is filled in */
		mutex_lock(&un->mutex);
		memcpy(un->vsn, ce->CeVsn, sizeof (vsn_t));
		un->vsn[sizeof (vsn_t) - 1] = (char) 0;
		un->i.ViPart = ce->CePart;
		mutex_unlock(&un->mutex);
		scan_a_device(un, drive->open_fd);
		mutex_lock(&un->mutex);
		if (IS_TAPE(un)) {	/* get the old info for tape */
			un->space = (ce->CeSpace <= un->capacity) ?
			    ce->CeSpace : un->capacity;
			if (un->space == 0)
				un->status.bits |= DVST_STOR_FULL;
			un->status.b.read_only |=
			    (ce->CeStatus & CES_read_only);
			DevLog(DL_DETAIL(4013), ce->CeSlot, un->vsn);
		}
		UpdateCatalog(un, 0, CatalogVolumeLoaded);
	} else {
		mutex_lock(&un->mutex);
		memcpy(un->vsn, ce->CeVsn, sizeof (vsn_t));
		un->vsn[sizeof (vsn_t) - 1] = (char) 0;
		un->status.b.read_only |= (ce->CeStatus & CES_read_only);
		un->status.bits &= ~(DVST_SCANNING | DVST_LABELED);
		un->status.bits |= (DVST_READY | DVST_PRESENT);
		DevLog(DL_DETAIL(4014), ce->CeSlot, un->vsn);
	}

	clear_driver_idle(drive, drive->open_fd);
	DEC_ACTIVE(un);
	close_unit(un, &drive->open_fd);

	/*
	 * If the cleaning bit went on, clear the drive,
	 * clear preview busy, dispose of event and return.
	 */
	if (un->status.bits & DVST_CLEANING) {
		mutex_unlock(&un->mutex);
		mutex_lock(&drive->mutex);
		clear_drive(drive);
		mutex_unlock(&drive->mutex);
		mutex_lock(&un->mutex);
		/* clear drive will leave unit open */
		close_unit(un, &drive->open_fd);
		clear_busy_bit(event, drive, ce->CeSlot);
		un->status.bits &= ~DVST_REQUESTED;
		mutex_unlock(&un->mutex);
		disp_of_event(drive->library, event, 0);
		return;
	}
	/*
	 * device mutex (un->mutex) is held, requested bit is set,
	 * active count is zero, open count is zero.
	 */
	if (event->type == EVENT_TYPE_INTERNAL &&
	    event->request.internal.address != NULL) {
		preview_t *preview =
		    (preview_t *)(event->request.internal.address);

		/* Make sure its the same preview entry */
		mutex_lock(&preview->p_mutex);
		if (preview->sequence != event->request.internal.sequence) {
			/*
			 * The preview entry that started this was replaced
			 * (deleted and a new one added). Just log the
			 * message and check to see if someone else needs this
			 * media.
			 */
			DevLog(DL_DETAIL(4015), preview->sequence,
			    event->request.internal.sequence);

			mutex_unlock(&preview->p_mutex);
			un->status.bits &= ~DVST_REQUESTED;
			if (!(un->status.bits & DVST_CLEANING))
				check_preview(un);
			mutex_unlock(&un->mutex);
			disp_of_event(drive->library, event, 0);
			return;
		}
		if (preview->busy != TRUE)
#ifdef DEBUG
		/*
		 * Replaced setting busy bit when it's already been set with
		 * assertion. Should never happened or we have problem with
		 * preview synchronization.
		 */
		{
			sam_syslog(LOG_CRIT,
			    "Preview busy bit not set while mounting,"
			    " aborting\n");
			abort();
		}
#else
		{
			sam_syslog(LOG_ERR,
			    "Preview busy bit not set while mounting\n");
			preview->busy = TRUE;
		}
#endif

		/*
		 * If it is unlabeled, not write protected, has valid barcode,
		 * is sam_media, a write request, ptoc_fwa == 0, not cleaning,
		 * and the barcode option is selected, write a label on it.
		 */
		non_label_status = (CES_read_only | CES_unavail |
		    CES_writeprotect | CES_non_sam | CES_bad_media);
		if ((ce->m.CePtocFwa == 0) &&
		    (ce->CeStatus & CES_bar_code) &&
		    (preview->write && un->status.b.ready) &&
		    (defaults->flags & DF_LABEL_BARCODE) &&
		    !(ce->CeStatus & non_label_status) &&
		    !(un->status.bits & (DVST_READ_ONLY | DVST_WRITE_PROTECT |
		    DVST_LABELED | DVST_CLEANING |
		    DVST_BAD_MEDIA | DVST_STRANGE))) {
			int		tmp;
			uint_t 	flags = 0;
			char 	lb_vsn[LEN_OPTIC_VSN];
			char 	*MES_9208 = catgets(catfd, SET, 9208,
			    "writing label %s on %s");
			char 	*mess;
			label_req_t lb_req;

			/* Don't hold the preview mutex over the labeling */
			mutex_unlock(&preview->p_mutex);

			if (is_tape(sam_atomedia(ce->CeMtype))) {
				tmp = LEN_TAPE_VSN;
			} else {
				tmp = LEN_OPTIC_VSN;
			}

			vsn_from_barcode(
			    &lb_vsn[0], ce->CeBarCode, defaults, tmp);

			/* One last check before labeling: Verify the label */
			/* time is 0 */
			if (ce->CeLabelTime != 0) {
				snprintf(d_mess, DIS_MES_LEN, catgets(catfd,
				    SET, 9323,
				    "Cannot label: %s"),
				    "apparent unlabeled tape has active"
				    " label_time");
				sam_syslog(LOG_INFO, catgets(catfd, SET, 9323,
				    "Cannot label: %s"),
				    "apparent unlabeled tape has active"
				    " label_time");
				sam_syslog(LOG_INFO, catgets(catfd, SET, 9223,
				    "%s(%d): Cannot label: VSN %s is flagged"
				    " as bad media."),
				    ent_pnt, drive->un->eq, lb_vsn);
				goto LabelFail;
			}

			memmove(un->i.ViMtype, sam_mediatoa(drive->un->type),
			    sizeof (un->i.ViMtype));
			memmove(un->i.ViVsn, &lb_vsn, sizeof (un->i.ViVsn));
			if (CatalogLabelVolume(&un->i, (char *)lb_vsn) == -1)
				goto LabelFail;
			DevLog(DL_DETAIL(4016), slot, lb_vsn, ce->CeBarCode);
			mess = (char *)malloc_wait(strlen(MES_9208) +
			    strlen(lb_vsn) + strlen((char *)ce->CeBarCode) +
			    15, 4, 0);
			sprintf(mess, MES_9208, lb_vsn, ce->CeBarCode);
			memccpy(d_mess, mess, '\0', DIS_MES_LEN);
			free(mess);

			lb_req.eq = un->eq;
			lb_req.slot = ce->CeSlot;
			lb_req.part = ce->CePart;
			lb_req.flags = flags;
			if (IS_TAPE(un)) {
				lb_req.block_size = un->dt.tp.default_blocksize;
			}
			lb_req.vsn = lb_vsn;
			lb_req.info = NULL;
			drive->open_fd = open_unit(un, un->name, 2);
			un->status.bits |= DVST_OPENED;
			INC_ACTIVE(un);
			un->status.bits |= DVST_REQUESTED;
			mutex_unlock(&un->mutex);

			if (IS_TAPE(un)) {
				mutex_lock(&un->io_mutex);
				err = write_tape_labels(
				    &(drive->open_fd), un, &lb_req);
				mutex_unlock(&un->io_mutex);
			} else {
				flags |= LABEL_ERASE;
				lb_req.flags = flags;
				mutex_lock(&un->io_mutex);
				err = write_labels(drive->open_fd, un, &lb_req);
				mutex_unlock(&un->io_mutex);
			}

			if (err) {
				mutex_lock(&un->mutex);
				if (err != VOLSAFE_LABEL_ERROR) {
					un->status.bits |= DVST_BAD_MEDIA;
					(void) CatalogSetFieldByLoc(
					    drive->library->un->eq, ce->CeSlot,
					    ce->CePart, CEF_Status,
					    CES_bad_media, 0);
				}
				un->status.bits &= ~DVST_REQUESTED;
				mutex_unlock(&un->mutex);
				/* Call CatalogLabelFailed to remove "dummy" */
				/* entry. */
				CatalogLabelFailed(&un->i, (char *)lb_vsn);
			} else {
				mutex_lock(&un->mutex);
				un->status.bits &= ~DVST_REQUESTED;
				un->status.bits |= DVST_SCANNING;
				un->space = un->capacity;
				mutex_unlock(&un->mutex);
				scan_a_device(un, drive->open_fd);
				UpdateCatalog(un, 0, CatalogLabelComplete);
			}

			/* Leave the labeling section with the mutex held */
			mutex_lock(&un->mutex);

			close_unit(un, &drive->open_fd);
			DEC_ACTIVE(un);

LabelFail:
			/* re-acquire the preview mutex */
			mutex_lock(&preview->p_mutex);
		}

		/* Was the request canceled */
		if (preview->p_error) {
			mutex_unlock(&preview->p_mutex);
			remove_preview_ent(
			    preview, NULL, PREVIEW_NOTIFY, ECANCELED);
			un->status.bits &= ~DVST_REQUESTED;
			if (!(un->status.bits & DVST_CLEANING))
				check_preview(un);
			mutex_unlock(&un->mutex);
			disp_of_event(drive->library, event, 0);
			return;
		}

		/*
		 * if its a write request and the media is read-only or
		 * not sam or write-protected, bail out with access error
		 */
		if (preview->write && (un->status.bits &
		    (DVST_READ_ONLY | DVST_WRITE_PROTECT | DVST_STRANGE))) {
			mutex_unlock(&preview->p_mutex);
			remove_preview_ent(
			    preview, NULL, PREVIEW_NOTIFY, EACCES);
			un->status.bits &= ~DVST_REQUESTED;
			if (!(un->status.bits & DVST_CLEANING))
				check_preview(un);
			mutex_unlock(&un->mutex);
			disp_of_event(drive->library, event, 0);
			return;
		}

		/* Check for labeled (or not sam) and correct vsn */
		if (!(un->status.bits & (DVST_LABELED | DVST_STRANGE)) ||
		    strcmp(un->vsn, preview->resource.archive.vsn) != 0) {
			/* If it is not labeled or not sam, flag as */
			/* bad media. */
			if (!(un->status.bits & (DVST_LABELED | DVST_STRANGE)))
				un->status.bits |= DVST_BAD_MEDIA;

			un->status.bits &= ~DVST_REQUESTED;
			if (un->status.bits & DVST_BAD_MEDIA) {

				/* Tell the requester that this is bad media */
				(void) CatalogSetFieldByLoc(
				    drive->library->un->eq, ce->CeSlot,
				    ce->CePart, CEF_Status, CES_bad_media, 0);
				un->label_time = 0;

				/*
				 * Include the media barcode in the report
				 * if known/available.
				 */
				if (ce->CeStatus & CES_bar_code) {
					if (slot != ROBOT_NO_SLOT) {
						DevLog(DL_ERR(4017),
						    slot, ce->CeBarCode);
					} else {
						DevLog(DL_ERR(4023),
						    ce->CeBarCode);
					}
				} else {
					if (slot != ROBOT_NO_SLOT) {
						DevLog(DL_ERR(4018), slot);
					} else {
						DevLog(DL_ERR(4024));
					}
				}

				mutex_unlock(&preview->p_mutex);
				remove_preview_ent(
				    preview, NULL, PREVIEW_NOTIFY, ENOSPC);
			} else {
				/*
				 * Wrong vsn, clear busy for this preview and
				 * see if someone wants it.
				 */
				preview->robot_equ = 0;
				preview->slot = ROBOT_NO_SLOT;
				preview->busy = FALSE;
				mutex_unlock(&preview->p_mutex);
				if (!(un->status.bits & DVST_CLEANING))
					check_preview(un);
			}
		} else {
			/* Everything looks good, assign the media to the */
			/* request. */
			mutex_unlock(&preview->p_mutex);
			un->mtime = time(NULL) + un->delay;
			remove_preview_ent(preview, un, PREVIEW_NOTIFY, 0);
		}
	}

	/* if media is unlabeled set space = capacity if capacity is good */
	if (!(un->status.bits & (DVST_LABELED | DVST_STRANGE))) {
		un->space = un->capacity;
	}

	un->status.bits &= ~DVST_REQUESTED;

	/*
	 * Increment device active and open for migration toolkit.  These fields
	 * will be decremented when the toolkit code releases the device
	 * (sam_mig_release_device).
	 */
	if ((event->type == EVENT_TYPE_MESS) &&
	    (request->flags & CMD_MOUNT_S_MIGKIT)) {
		INC_ACTIVE(un);
		INC_OPEN(un);
	}

	mutex_unlock(&un->mutex);
	disp_of_event(drive->library, event, 0);
}


/*
 *	todo - Process request for media already mounted on this device.
 *
 * Note:  The active count should be incremented BEFORE makeing this
 * request. This is to prevent the robot from doing something to the
 * device bewteen making the request and the robot picking up the request.
 */
void
todo(
	drive_state_t *drive,
	robo_event_t *event)
{
	char 			*ent_pnt = "todo";
	dev_ent_t 		*un = drive->un;
	todo_request_t 	*request;

	request = &event->request.message.param.todo_request;
	switch (request->sub_cmd) {
	case TODO_ADD:
#if defined(DEBUG)
		sam_syslog(LOG_DEBUG, "todo add mount %#x", request->callback);
#endif
		{
			if (request->callback == CB_POSITION_RMEDIA) {
				mutex_lock(&un->io_mutex);
				position_rmedia(un);
				mutex_unlock(&un->io_mutex);
				DEC_ACTIVE(un);
				break;
			} else if (request->callback == CB_NOTIFY_FS_LOAD) {
				/*
				 * notify_fs_mount increments the active count,
				 * so decrement the active count upon
				 * return since the todo requested incremented
				 * it before posting the request.
				 */
				mutex_lock(&un->mutex);
				notify_fs_mount(&request->handle,
				    &request->resource, un, 0);
				DEC_ACTIVE(un);
				mutex_unlock(&un->mutex);
			} else if (request->callback == CB_NOTIFY_TP_LOAD) {
				/*
				 * notify_tp_mount increments the active count,
				 * so decrement the active count upon
				 * return since the todo requested incremented
				 * it before posting the request.
				 */
				mutex_lock(&un->mutex);
				notify_tp_mount(&request->handle, un, 0);
				DEC_ACTIVE(un);
				mutex_unlock(&un->mutex);
			} else {
				mutex_lock(&un->mutex);
				DEC_ACTIVE(un);
				mutex_unlock(&un->mutex);
				notify_fs_mount(&request->handle,
				    &request->resource, NULL, ENXIO);
			}
		}

		break;

	case TODO_CANCEL:
		switch (request->callback) {

		case CB_NOTIFY_FS_LOAD:
			break;

		default:
			sam_syslog(LOG_INFO, "%s: Unknown cancel request %d.",
			    ent_pnt, request->callback);
			break;
		}
		break;

	case TODO_UNLOAD:
		unload_media(un, request);
		break;
	}

	disp_of_event(drive->library, event, 0);
}
