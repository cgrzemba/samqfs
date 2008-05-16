/*
 *	drive1.c - support for thread that watches over an optical drive.
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

#pragma ident "$Revision: 1.48 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
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
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/device.h"
#include "sam/lib.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/sef.h"
#include "aml/odlabels.h"
#include "aml/tplabels.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "sam/defaults.h"
#include "sam/nl_samfs.h"
#include "../generic/generic.h"
#include "aml/trace.h"
#include "aml/proto.h"
#include "aml/tapealert.h"
#include "driver/samst_def.h"
#include "drive.h"

/*	some globals */
extern pid_t mypid;
extern shm_alloc_t master_shm, preview_shm;
extern void off_drive(drive_state_t *drive, int source);


/*
 *	label_slot - write labels on the media that belongs in slot.
 *
 * This routine updates the slot occupied flag in the catalog before the
 * exchange/move is done.  Since a failure of the exchange/move can leave
 * the occupied state in doubt anyway, this should not be a problem.  To
 * wait until the exchange/move was done, would require locking the
 * catalog table during the exchange/move process, which is a long time.
 */
void
label_slot(
	drive_state_t *drive,
	robo_event_t *event)
{
	int		err;
	char	*l_mess = drive->library->un->dis_mes[DIS_MES_NORM];
	char	*ent_pnt = "label_slot";
	library_t		*library;
	label_request_t *label_request;
	label_req_t		lb_req;
	struct CatalogEntry ced;
	struct CatalogEntry *ce;
	properties_t prop;
	properties_t volsafe;

	library = drive->library;
	label_request = &event->request.message.param.label_request;
	ce = CatalogGetCeByLoc(library->un->eq, label_request->slot,
	    label_request->part, &ced);

	/*
	 * If slot empty, unavail, write proteced, read only
	 * or not SAM media, do not attempt to label.
	 */
	if ((ce == NULL) ||
	    (!(ce->CeStatus & CES_inuse)) ||
	    (ce->CeStatus & CES_unavail) ||
	    (ce->CeStatus & CES_writeprotect) ||
	    (ce->CeStatus & CES_read_only) ||
	    (ce->CeStatus & CES_non_sam)) {

		char *MES_9212 = catgets(catfd, SET, 9212,
		    "Slot %d is write protected, unavailable, read only, not"
		    " present, or not SAM media");

		char *mess = (char *)malloc_wait(strlen(MES_9212) + 10, 4, 0);
		sprintf(mess, MES_9212, label_request->slot);
		sam_syslog(LOG_INFO,
		    catgets(catfd, SET, 9211,
		    "%s: Slot %d is write protected, unavailable, read only,"
		    " not present, or not SAM media."),
		    ent_pnt, label_request->slot);
		memccpy(l_mess, mess, '\0', DIS_MES_LEN);
		mutex_lock(&drive->un->mutex);
		drive->un->status.b.requested = FALSE;
		mutex_unlock(&drive->un->mutex);
		write_event_exit(event, EXIT_FAILED, mess);
		free(mess);
		disp_of_event(drive->library, event, ENOENT);
		return;
	}

	if (ce->CeStatus & CES_needs_audit) {
		char *MES_9214 =
		    catgets(catfd, SET, 9214,
		    "Cannot label media in slot %d - waiting for audit");

		char *mess = (char *)malloc_wait(strlen(MES_9214) + 10, 4, 0);

		sprintf(mess, MES_9214, ce->CeSlot);
		memccpy(l_mess, mess, '\0', DIS_MES_LEN);
		sam_syslog(LOG_INFO,
		    catgets(catfd, SET, 9213,
		"%s: Cannot label media in slot %d: Waiting for audit."),
		    ent_pnt, ce->CeSlot);
		mutex_lock(&drive->un->mutex);
		drive->un->status.bits &= ~DVST_REQUESTED;
		mutex_unlock(&drive->un->mutex);
		write_event_exit(event, EXIT_FAILED, mess);
		free(mess);
		disp_of_event(drive->library, event, ENOENT);
		return;
	}

	if (ce->CeStatus & CES_cleaning) {
		char *mess =

		    catgets(catfd, SET, 9216,
		    "cannot label cleaning cartridge.");
		sam_syslog(LOG_INFO,
		    catgets(catfd, SET, 9215,
		    "%s: Cannot label cleaning cartridge in slot %d"),
		    ent_pnt, ce->CeSlot);
		memccpy(l_mess, mess, '\0', DIS_MES_LEN);
		mutex_lock(&drive->un->mutex);
		drive->un->status.bits &= ~DVST_REQUESTED;
		mutex_unlock(&drive->un->mutex);
		write_event_exit(event, EXIT_FAILED, mess);
		disp_of_event(drive->library, event, ENOENT);
		return;
	}

	lb_req.vsn = label_request->vsn;

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

	if (err = get_media(library, drive, event, ce)) {
		clear_driver_idle(drive, drive->open_fd);
		mutex_lock(&drive->un->mutex);
		close_unit(drive->un, &drive->open_fd);
		DEC_ACTIVE(drive->un);
		drive->un->status.bits &= ~DVST_REQUESTED;
		if (IS_GET_MEDIA_FATAL_ERROR(err)) {
			if (err == RET_GET_MEDIA_DOWN_DRIVE) {
				DownDevice(drive->un, SAM_STATE_CHANGE);
			}
			err = -err;
		}
		mutex_unlock(&drive->un->mutex);
		mutex_unlock(&drive->mutex);
		if (err == RET_GET_MEDIA_DISPOSE) {
			disp_of_event(drive->library, event, EIO);
		}
		return;
	}

	mutex_unlock(&drive->mutex);
	mutex_lock(&drive->un->mutex);
	drive->un->status.bits |= DVST_SCANNING;
	drive->un->status.bits &= ~DVST_LABELED;
	mutex_unlock(&drive->un->mutex);
	if (spin_drive(drive, SPINUP, NOEJECT)) {
		/* spin_drive() prints error - downs drive or media */
		if (drive->un->state > DEV_ON) {
			mutex_lock(&drive->mutex);
			clear_drive(drive);
			clear_driver_idle(drive, drive->open_fd);
			DEC_ACTIVE(drive->un);
			close_unit(drive->un, &drive->open_fd);
			drive->un->status.bits &=
			    ~(DVST_REQUESTED | DVST_SCANNING);
			mutex_unlock(&drive->un->mutex);
			mutex_unlock(&drive->mutex);
			return;
		}
		return;
	}

	/*
	 * Zap the un->vsn field so old information is not used.
	 * e.g., if labeling new optical media, un->vsn could be
	 * filled in with the A side label and the B side is blank.
	 */
	memset(drive->un->vsn, 0, sizeof (vsn_t));
	drive->un->i.ViPart = ce->CePart;
	scan_a_device(drive->un, drive->open_fd);
	UpdateCatalog(drive->un, 0, CatalogVolumeLoaded);
	mutex_lock(&drive->un->mutex);

	if (drive->un->status.bits & DVST_READ_ONLY) {
		char *mess;

		if (drive->status.b.bar_code) {
			char *MES_9218 = catgets(catfd, SET, 9218,
			    "unable to label media from slot %d,"
			    " barcode %s - readonly");
			mess = (char *)malloc_wait(strlen(MES_9218) +
			    strlen((char *)drive->bar_code) + 10, 4, 0);

			sprintf(mess, MES_9218,
			    drive->un->slot, drive->bar_code);

			SendCustMsg(HERE, 9217, ent_pnt, drive->un->eq,
			    drive->un->slot, drive->bar_code);

		} else {
			char *MES_9220 =
			    catgets(catfd, SET, 9220,
			    "unable to label media from slot %d - readonly");
			mess = (char *)malloc_wait(strlen(MES_9220) + 10, 4, 0);

			sprintf(mess, MES_9220, drive->un->eq, drive->un->slot);
			SendCustMsg(HERE, 9219, ent_pnt, drive->un->eq,
			    drive->un->slot);
		}

		memccpy(l_mess, mess, '\0', DIS_MES_LEN);
		close_unit(drive->un, &drive->open_fd);
		DEC_ACTIVE(drive->un);
		clear_driver_idle(drive, drive->open_fd);
		drive->un->status.bits &= ~(DVST_REQUESTED | DVST_SCANNING);
		mutex_unlock(&drive->un->mutex);
		write_event_exit(event, EXIT_FAILED, mess);
		free(mess);
		disp_of_event(drive->library, event, EACCES);
		return;
	}

	if ((drive->un->status.bits & DVST_READY) &&
	    !(drive->un->status.bits & DVST_CLEANING)) {
		/* If the media is labeled, then the relabel bit must be set */
		if ((drive->un->status.bits & DVST_LABELED) &&
		    !(label_request->flags & LABEL_RELABEL)) {
			char *MES_9222 = catgets(catfd, SET, 9222,
			    "media from slot %d is already labeled");
			char *mess = (char *)malloc_wait(
			    strlen(MES_9222) + 10, 4, 0);

			sprintf(mess, MES_9222, drive->un->slot);
			memccpy(l_mess, mess, '\0', DIS_MES_LEN);
			sam_syslog(LOG_INFO,
			    catgets(catfd, SET, 9221,
			"%s(%d): Media from slot %d is already labeled."),
			    ent_pnt, drive->un->eq, drive->un->slot);
			close_unit(drive->un, &drive->open_fd);
			DEC_ACTIVE(drive->un);
			clear_driver_idle(drive, drive->open_fd);
			drive->un->status.bits &=
			    ~(DVST_REQUESTED | DVST_SCANNING);
			mutex_unlock(&drive->un->mutex);
			write_event_exit(event, EXIT_FAILED, mess);
			free(mess);
			disp_of_event(drive->library, event, EACCES);
			return;
		}

		/* If worm media, then it can not be relabeled. */
		if ((drive->un->status.bits & DVST_LABELED) &&
		    (label_request->flags & LABEL_RELABEL) &&
		    (get_media_type(drive->un) == MEDIA_WORM)) {
			char *mess = (char *)malloc_wait(
			    strlen(GetCustMsg(9305))+1, 4, 0);

			strcpy(mess, GetCustMsg(9305));
			memccpy(l_mess, mess, '\0', DIS_MES_LEN);
			sam_syslog(LOG_INFO, mess);
			SendCustMsg(HERE, 9305);
			close_unit(drive->un, &drive->open_fd);
			DEC_ACTIVE(drive->un);
			clear_driver_idle(drive, drive->open_fd);
			drive->un->status.bits &=
			    ~(DVST_REQUESTED | DVST_SCANNING);
			mutex_unlock(&drive->un->mutex);
			write_event_exit(event, EXIT_FAILED, mess);
			free(mess);
			disp_of_event(drive->library, event, EACCES);
			return;
		}

		if ((*label_request->old_vsn != '\0') &&
		    (strcmp(drive->un->vsn, label_request->old_vsn) != 0)) {
			char *MES_9309 = catgets(catfd, SET, 9309,
			    "media VSN %s at slot %d not same as old VSN %s");
			char *mess = (char *)malloc_wait(
			    strlen(MES_9309) + 70, 4, 0);

			sprintf(mess, MES_9309,
			    drive->un->vsn, drive->un->slot,
			    label_request->old_vsn);
			memccpy(l_mess, mess, '\0', DIS_MES_LEN);
			sam_syslog(LOG_INFO, catgets(catfd, SET, 9310,
			    "s(%d): Volume VSN %s at slot %d not same as old"
			    " VSN %s."),
			    ent_pnt, drive->un->eq, drive->un->vsn,
			    drive->un->slot, label_request->old_vsn);

			close_unit(drive->un, &drive->open_fd);
			DEC_ACTIVE(drive->un);
			clear_driver_idle(drive, drive->open_fd);
			drive->un->status.bits &=
			    ~(DVST_REQUESTED | DVST_SCANNING);
			mutex_unlock(&drive->un->mutex);
			write_event_exit(event, EXIT_FAILED, mess);
			free(mess);
			disp_of_event(drive->library, event, EACCES);
			return;
		}

		if (drive->un->status.bits & DVST_BAD_MEDIA) {
			drive->un->status.bits &= ~DVST_BAD_MEDIA;
			(void) CatalogSetFieldByLoc(ce->CeEq, ce->CeSlot,
			    ce->CePart, CEF_Status, CES_bad_media, 0);
		}

		drive->un->i.ViSlot = ce->CeSlot;
		drive->un->i.ViPart = ce->CePart;
		drive->un->i.ViEq = ce->CeEq;
		memmove(drive->un->i.ViMtype, sam_mediatoa(drive->un->type),
		    sizeof (drive->un->i.ViMtype));
		memmove(drive->un->i.ViVsn, drive->un->vsn,
		    sizeof (drive->un->i.ViVsn));

		if (CatalogLabelVolume(&drive->un->i, lb_req.vsn) == -1) {
			char buf[STR_FROM_ERRNO_BUF_SIZE];

			snprintf(l_mess, DIS_MES_LEN, catgets(catfd, SET, 9323,
			    "Cannot label: %s"),
			    StrFromErrno(errno, buf, sizeof (buf)));
			sam_syslog(LOG_INFO,
			    catgets(catfd, SET, 9223, "%s(%d): %s"),
			    ent_pnt, drive->un->eq, l_mess);
			mutex_lock(&drive->un->mutex);
			drive->un->status.bits &= ~DVST_REQUESTED;
			mutex_unlock(&drive->un->mutex);
			write_event_exit(event, EXIT_FAILED, l_mess);
			disp_of_event(drive->library, event, ENOENT);
			return;
		}

		mutex_unlock(&drive->un->mutex);

		err = 0;
		lb_req.eq = drive->un->eq;
		lb_req.slot = ce->CeSlot;
		lb_req.flags = label_request->flags;
		lb_req.block_size = label_request->block_size;
		lb_req.info = label_request->info;
		lb_req.part = label_request->partition;
		/* Set the correct partition in the un */
		drive->un->i.ViPart = ce->CePart;

		switch (drive->un->type & DT_CLASS_MASK) {
		case DT_OPTICAL:
			mutex_lock(&drive->un->io_mutex);
			err = write_labels(drive->open_fd, drive->un, &lb_req);
			mutex_unlock(&drive->un->io_mutex);
			break;

		case DT_TAPE:
			mutex_lock(&drive->un->io_mutex);
			err = write_tape_labels(
			    &(drive->open_fd), drive->un, &lb_req);
			mutex_unlock(&drive->un->io_mutex);
			/* used volsafe relabel failure message */
			prop = drive->un->dt.tp.properties;
			volsafe = PROPERTY_VOLSAFE|PROPERTY_VOLSAFE_PERM_LABEL;
			if (err == 0 && (prop & volsafe) == volsafe) {
				memccpy(l_mess, GetCustMsg(9359), '\0',
				    DIS_MES_LEN);
			}
			break;

		default:
			sam_syslog(LOG_ERR,
			    "%s(%d): Unknown device type (%#x).", ent_pnt,
			    drive->un->eq, drive->un->type);
			err = -1;
			break;
		}

		drive->un->i.ViPart = ce->CePart;
		scan_a_device(drive->un, drive->open_fd);
		mutex_lock(&drive->un->mutex);
		drive->un->status.bits &= ~DVST_OPENED;

		if (err) {
			if (err != VOLSAFE_LABEL_ERROR) {
				drive->un->status.bits |= DVST_BAD_MEDIA;
			}
			CatalogLabelFailed(&drive->un->i, label_request->vsn);
		} else {
			UpdateCatalog(drive->un, 0, CatalogLabelComplete);
		}

		DEC_ACTIVE(drive->un);
		clear_driver_idle(drive, drive->open_fd);
		/*
		 * take away most of the mount time given by the scan
		 * The check_preview will increment it if an entry found.
		 */
		close_unit(drive->un, &drive->open_fd);
		drive->un->mtime -= (drive->un->delay - 2);
		drive->un->status.bits &= ~(DVST_REQUESTED | DVST_SCANNING);

		if (drive->un->status.bits & DVST_LABELED)
			check_preview(drive->un);
		mutex_unlock(&drive->un->mutex);
		mutex_lock(&library->mutex);
		library->un->status.bits |= DVST_MOUNTED;
		mutex_unlock(&library->mutex);
		write_event_exit(event, err, NULL);
		disp_of_event(drive->library, event, 0);

		return;
	}

	DEC_ACTIVE(drive->un);
	clear_driver_idle(drive, drive->open_fd);
	close_unit(drive->un, &drive->open_fd);
	drive->un->status.bits &= ~(DVST_REQUESTED | DVST_SCANNING);
	mutex_unlock(&drive->un->mutex);
	/* must've failed if we got to here */
	disp_of_event(drive->library, event, EXIT_FAILED);
}


/*
 *	move_list - move MESS type entries back to the main worklist.
 * Used when OFFing a drive.
 *
 * entry -
 *	   drive  - drive_state_t * (with list mutex locked)
 *	   library - library_t * (with list mutex locked)
 */
void
move_list(
	drive_state_t *drive,
	library_t *library)
{
	robo_event_t *drive_list = drive->first, *move_element;
	robo_event_t *library_list;

	if (drive_list == NULL)
		return;

	if (library->first == NULL)
		library_list = NULL;
	else {
		/* point library_list to the last element on the list */
		LISTEND(library, library_list);
	}

	while (drive_list != NULL) {
		move_element = drive_list;
		drive_list = drive_list->next;
		unlink_list(move_element);
		if (move_element->type == EVENT_TYPE_MESS) {
			if (library_list == NULL) {
				/* library list is empty */
				library_list = library->first = move_element;
			} else {
				library_list = append_list(
				    library_list, move_element);
			}
			library->active_count++;
		} else
			disp_of_event(library, move_element, EIO);
	}
	drive->first = NULL;
	drive->active_count = 0;
}


void
generate_audit(library_t *library)
{
}

void
drive_state_change(
	drive_state_t *drive,
	state_change_t *state_change)
{
	int i;
	char *d_mess = drive->un->dis_mes[DIS_MES_NORM];
	char *dc_mess = drive->un->dis_mes[DIS_MES_CRIT];
	char *ent_pnt = "drive_state_change";
	sam_defaults_t *defaults;
	struct DeviceParams *DeviceParams;

	defaults = GetDefaults();

	if (state_change->state == state_change->old_state)
		return;

	DeviceParams = (struct DeviceParams *)
	    (void *)((char *)defaults + sizeof (sam_defaults_t));

	mutex_lock(&drive->mutex);
	switch (state_change->state) {
	case DEV_ON: {			/* switching to */
		switch (state_change->old_state) {
		case DEV_IDLE:	/* going from to on */
			/* FALLTHROUGH */
		case DEV_UNAVAIL:	/* going from to on */
			/* FALLTHROUGH */
		case DEV_OFF:	/* going from to on */
			/* FALLTHROUGH */
			/* clear critical message */
			*dc_mess = '\0';
			mutex_lock(&drive->un->mutex);
#if defined(DEBUG)
			memccpy(d_mess, "attempting to on device",
			    '\0', DIS_MES_LEN);
#endif
			drive->un->state = DEV_ON;
			drive->status.b.offline = FALSE;
			if (IS_TAPE(drive->un))
				ChangeMode(drive->un->name, SAM_TAPE_MODE);
			if (drive->open_fd >= 0)
				close(drive->open_fd);
			if (IS_OPTICAL(drive->un) && (drive->open_fd =
			    open_unit(drive->un, drive->un->name, 3)) >= 0) {
				mutex_lock(&drive->un->io_mutex);
				scsi_reset(drive->open_fd, drive->un);
				mutex_unlock(&drive->un->io_mutex);
				clear_driver_idle(drive, drive->open_fd);
				close_unit(drive->un, &drive->open_fd);
			}
			drive->un->open_count = 0;
			drive->un->status.bits = DVST_REQUESTED;
			ident_dev(drive->un, -1);
			mutex_unlock(&drive->un->mutex);
			/* if ident_dev worked */
			if (drive->un->state == DEV_ON) {
				/* init_drive grabs the mutex */
				mutex_unlock(&drive->mutex);
				init_drive(drive);
				mutex_lock(&drive->un->mutex);
				drive->un->active = 0;
				drive->un->open_count = 0;
				drive->un->status.bits |= DVST_PRESENT;
				drive->un->status.bits &= ~DVST_REQUESTED;
				/* Make any defaults.conf changes not */
				/* made by sam-init. */
				for (i = 0; i < DeviceParams->count; i++) {
					struct DpEntry *dp;

					dp = DeviceParams->DpTable + i;
					if (drive->un->type != dp->DpType) {
						continue;
					}
					if (drive->un->delay == 0) {
						drive->un->delay =
						    dp->DpDelay_time;
					}
					if (drive->un->unload_delay == 0) {
						drive->un->unload_delay
						    = dp->DpUnload_time;
					}
					if (IS_TAPE(drive->un) &&
					    drive->un->dt.tp.
					    default_blocksize == 0) {
						drive->un->dt.tp.
						    default_blocksize =
						    dp->DpBlock_size;
					}
					if (IS_TAPE(drive->un) &&
					    drive->un->dt.tp.
					    position_timeout == 0) {
						drive->un->dt.tp.
						    position_timeout =
						    dp->DpPosition_timeout;
					}
					break;
				}
				mutex_unlock(&drive->un->mutex);
				return;
			}
			break;

		default:	/* going from on to */
			sam_syslog(LOG_ERR,
			    catgets(catfd, SET, 9225,
			    "%s(%d): Invalid state change."),
			    ent_pnt, drive->un->eq);
			break;
		}
		break;
	}

	case DEV_RO:			/* switching to */
		break;

	case DEV_IDLE:			/* switching to */
		{
			if (state_change->old_state == DEV_ON) {
#if defined(DEBUG)
				memccpy(d_mess, "setting device idle",
				    '\0', DIS_MES_LEN);
#endif
				mutex_lock(&drive->un->mutex);
				drive->un->state = DEV_IDLE;
				if (drive->un->active != 0) {
					mutex_unlock(&drive->un->mutex);
					break;
				}
			} else {
				sam_syslog(LOG_ERR,
				    catgets(catfd, SET, 9225,
				    "%s(%d): Invalid state change."),
				    ent_pnt, drive->un->eq);
				break;
			}

			mutex_unlock(&drive->un->mutex);
		}
		/* If the active count is 0, then we can */
		/* FALLTHROUGH */
	case DEV_OFF:			/* switching to */
		{
			int old_open_cnt;
			old_open_cnt = drive->un->open_count;
			clear_drive(drive);
			/* See if clear_drive caused an open */
			if (drive->un->open_count != old_open_cnt) {
				mutex_lock(&drive->un->mutex);
				close_unit(drive->un, &drive->open_fd);
				mutex_unlock(&drive->un->mutex);
			}
			if (IS_TAPE(drive->un))
				ChangeMode(drive->un->name, defaults->tapemode);
			off_drive(drive, USER_STATE_CHANGE);
			if (state_change->state == DEV_UNAVAIL) {
				mutex_lock(&drive->un->mutex);
				drive->un->state = DEV_UNAVAIL;
				mutex_unlock(&drive->un->mutex);
			}
			break;
		}

	case DEV_UNAVAIL:		/* switching to */
		{
			if (state_change->old_state != DEV_DOWN) {
				clear_drive(drive);
				/* See if clear_drive caused an open */
				if (drive->un->open_count) {
					mutex_lock(&drive->un->mutex);
					close_unit(drive->un, &drive->open_fd);
					drive->un->open_count = 0;
					drive->un->state = DEV_UNAVAIL;
					mutex_unlock(&drive->un->mutex);
				} else {
					mutex_lock(&drive->un->mutex);
					drive->un->state = DEV_UNAVAIL;
					mutex_unlock(&drive->un->mutex);
				}
				if (IS_TAPE(drive->un)) {
					ChangeMode(drive->un->name,
					    defaults->tapemode);
				}
			} else {
				sam_syslog(LOG_ERR,
				    catgets(catfd, SET, 9225,
				    "%s(%d): Invalid state change."),
				    ent_pnt, drive->un->eq);
			}
			break;
		}

	case DEV_DOWN:			/* switching to */
		down_drive(drive, USER_STATE_CHANGE);
		mutex_lock(&drive->un->mutex);
		drive->un->state = DEV_DOWN;
		mutex_unlock(&drive->un->mutex);
		break;

	default:
		sam_syslog(LOG_INFO,
		    "%s(%d): Unknown state(%#x) in state change.",
		    ent_pnt, drive->un->eq, state_change->state);
	}

	mutex_unlock(&drive->mutex);
	mutex_lock(&drive->un->mutex);
	drive->un->status.bits &= ~DVST_REQUESTED;
	mutex_unlock(&drive->un->mutex);
}

/*
 *	drive_tapealert_solicit - perform device tapealert request.
 */
void
drive_tapealert_solicit(
	drive_state_t *drive,
	tapealert_request_t *tapealert_request)
{
	dev_ent_t *un = drive->un;

	mutex_lock(&un->mutex);
	if (tapealert_request->flags & TAPEALERT_ENABLED) {
		un->tapealert |= TAPEALERT_ENABLED;
		get_supports_tapealert(un, -1);
	} else {
		un->tapealert &= ~TAPEALERT_ENABLED;
	}
	mutex_unlock(&un->mutex);
}

/*
 *	drive_sef_solicit - perform device sef request.
 */
void
drive_sef_solicit(
	drive_state_t *drive,
	sef_request_t *sef_request)
{
	dev_ent_t *un = drive->un;

	mutex_lock(&un->mutex);
	if (sef_request->flags & SEF_ENABLED) {
		un->sef_sample.state |= SEF_ENABLED;
		get_supports_sef(un, -1);
	} else {
		un->sef_sample.state &= ~SEF_ENABLED;
	}
	un->sef_sample.interval = sef_request->interval;
	mutex_unlock(&un->mutex);
}

/*
 *	load_unavail - mount the requested media.
 * The requested bits in the device status structure should have been
 * set before posting the load_unavail request.
 *
 */
void
load_unavail(
	drive_state_t *drive,
	robo_event_t *event)
{
	int err;
	char *ent_pnt = "load_unavail";
	char *vsn = NULL;
	uint_t slot;
	load_u_request_t *request;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;

	if (drive->un->state != DEV_UNAVAIL) {
		sam_syslog(LOG_INFO,
		    catgets(catfd, SET, 9226, "%s(%d): Incorrect state."),
		    ent_pnt, drive->un->eq);
		disp_of_event(drive->library, event, EINVAL);
		return;
	}

	if (event->type == EVENT_TYPE_MESS) {
		request = &event->request.message.param.load_u_request;
		vsn = (char *)&request->vsn;
	} else {
		sam_syslog(LOG_INFO,
		    "%s(%d): Unknown load_unavail request (%#x).", ent_pnt,
		    drive->un->eq, event->type);
		drive->new_slot = ROBOT_NO_SLOT;
		mutex_lock(&drive->un->mutex);
		drive->un->status.bits &= ~DVST_REQUESTED;
		mutex_unlock(&drive->un->mutex);
		disp_of_event(drive->library, event, EINVAL);
		return;
	}
	slot = request->slot;

	if (DBG_LVL(SAM_DBG_LOAD)) {
		sam_syslog(LOG_DEBUG, "%s(%d): enter: requested is %s set:",
		    ent_pnt, drive->un->eq,
		    drive->un->status.b.requested ? "" : "NOT");
		sam_syslog(LOG_DEBUG, "%s(%d): %#x, %#x, %s.", ent_pnt,
		    drive->un->eq, slot, event->request.internal.address,
		    (vsn == NULL) ? "NONE" : vsn);

	}
	if (slot == ROBOT_NO_SLOT) {
		if (DBG_LVL(SAM_DBG_LOAD))
			sam_syslog(LOG_DEBUG, "%s(%d):%s:%d.",
			    ent_pnt, drive->un->eq, __FILE__, __LINE__);
		/* Clear reservation for the drive */

		mutex_lock(&drive->un->mutex);
		drive->un->status.bits &= ~DVST_REQUESTED;
		mutex_unlock(&drive->un->mutex);
		disp_of_event(drive->library, event, 0);
		drive->new_slot = ROBOT_NO_SLOT;
		return;
	}

	if (DBG_LVL(SAM_DBG_LOAD))
		sam_syslog(LOG_DEBUG, "%s(%d):(%#x)%s:%d.", ent_pnt,
		    drive->un->eq, slot,  __FILE__, __LINE__);
	ce = CatalogGetCeByLoc(
	    drive->library->un->eq, slot, request->part, &ced);

	if ((ce == NULL) || (!(ce->CeStatus & CES_inuse))) {
		if (DBG_LVL(SAM_DBG_LOAD))
			sam_syslog(LOG_DEBUG, "%s(%d):%s:%d.",
			    ent_pnt, drive->un->eq, __FILE__, __LINE__);
		mutex_lock(&drive->un->mutex);
		drive->un->status.bits &= ~DVST_REQUESTED;
		mutex_unlock(&drive->un->mutex);
		disp_of_event(drive->library, event, 0);
		drive->new_slot = ROBOT_NO_SLOT;
		return;
	}

	mutex_lock(&drive->mutex);

	if (err = get_media(drive->library, drive, event, ce)) {
		if (DBG_LVL(SAM_DBG_LOAD))
			sam_syslog(LOG_DEBUG, "%s(%d):%s:%d.",
			    ent_pnt, drive->un->eq, __FILE__, __LINE__);

		clear_driver_idle(drive, drive->open_fd);
		mutex_lock(&drive->un->mutex);
		close_unit(drive->un, &drive->open_fd);
		DEC_ACTIVE(drive->un);
		drive->un->status.bits &= ~DVST_REQUESTED;
		if (IS_GET_MEDIA_FATAL_ERROR(err)) {
			if (err == RET_GET_MEDIA_DOWN_DRIVE) {
				DownDevice(drive->un, SAM_STATE_CHANGE);
			}
			err = -err;
		}
		mutex_unlock(&drive->un->mutex);
		drive->new_slot = ROBOT_NO_SLOT;
		mutex_unlock(&drive->mutex);
		if (err == RET_GET_MEDIA_DISPOSE) {
			if (DBG_LVL(SAM_DBG_LOAD))
				sam_syslog(LOG_DEBUG, "%s(%d):%s:%d.",
				    ent_pnt, drive->un->eq, __FILE__, __LINE__);
			/* get_media leaves with the mutex on the catalog */
			/* held. */
			disp_of_event(drive->library, event, EIO);
		}
		if (DBG_LVL(SAM_DBG_LOAD))
			sam_syslog(LOG_DEBUG, "%s(%d):%s:%d.",
			    ent_pnt, drive->un->eq, __FILE__, __LINE__);
		return;
	}

	if (DBG_LVL(SAM_DBG_LOAD))
		sam_syslog(LOG_DEBUG, "%s(%d): slot %d: mounted.",
		    ent_pnt, drive->un->eq, slot);

	drive->new_slot = ROBOT_NO_SLOT;
	mutex_unlock(&drive->mutex);
	mutex_lock(&drive->un->mutex);
	drive->un->status.bits |= DVST_SCANNING;
	mutex_unlock(&drive->un->mutex);
	if (spin_drive(drive, SPINUP, NOEJECT)) {
		/* spin_drive() prints error - downs drive or media */
		if ((drive->un->state > DEV_ON) &&
		    (drive->un->state != DEV_UNAVAIL)) {
			mutex_lock(&drive->mutex);
			clear_drive(drive);	/* put media away */
			clear_driver_idle(drive, drive->open_fd);
			mutex_lock(&drive->un->mutex);
			DEC_ACTIVE(drive->un);
			close_unit(drive->un, &drive->open_fd);
			drive->un->status.bits &=
			    ~(DVST_REQUESTED | DVST_SCANNING);
			mutex_unlock(&drive->un->mutex);
			mutex_unlock(&drive->mutex);
			return;
		}
		return;
	}

	/*
	 * Although it is normally called in the following logic,
	 * don't call CatalogVolumeLoaded here because the
	 * volume has not been scanned and important values in the
	 * catalog, like space remaining, will be zeroed.
	 */
	mutex_lock(&drive->un->mutex);
	clear_driver_idle(drive, drive->open_fd);
	memcpy(drive->un->vsn, ce->CeVsn, sizeof (vsn_t));
	DEC_ACTIVE(drive->un);
	close_unit(drive->un, &drive->open_fd);
	drive->un->status.bits = (DVST_READY | DVST_PRESENT);
	drive->un->status.b.strange = (ce->CeStatus & CES_non_sam);
	mutex_unlock(&drive->un->mutex);
	disp_of_event(drive->library, event, 0);
}
