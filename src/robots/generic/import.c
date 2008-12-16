/*
 *  import.c - thread that watches over a import/export element
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

#pragma ident "$Revision: 1.67 $"

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
#include "sam/custmsg.h"
#include "aml/external_data.h"
#include "sam/lib.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/dev_log.h"
#include "aml/scsi.h"
#include "aml/tapes.h"
#include "aml/shm.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "aml/trace.h"
#include "sam/defaults.h"
#include "sam/nl_samfs.h"
#include "aml/proto.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/tapealert.h"

#include "generic.h"
#include "element.h"

/*	function prototypes */
int		wait_for_iport(iport_state_t *, int, int);
static int
move_empty_tray(library_t *library, iport_state_t *iport,
		uint_t empty_tray, const int direction);
static int	wait_for_plasmond_iport(iport_state_t *, int, int);
iport_state_t  *find_empty_export(library_t *);
void	    init_import(iport_state_t *);
static void	import_media(iport_state_t *, robo_event_t *);
void	    export_media(void *, robo_event_t *, uint_t type);
void	    generic_export_media(iport_state_t *, robo_event_t *);
void	    api_export_media(library_t *, robo_event_t *);
void	    start_slot_audit(library_t *, uint_t, uint_t);
void	    process_multi_import(iport_state_t *, uint_t, uint_t, uint_t);
void	    process_d360_import(iport_state_t *, uint_t, uint_t, media_t);
void	    process_generic_import(iport_state_t *, uint_t, uint_t, media_t);
void	    process_452_export(library_t *);
void	    wait_452_import(library_t *);
void	    unlock_adic(library_t *);

/*	some globals */
extern shm_alloc_t master_shm, preview_shm;


void	   *
import_thread(
		void *viport)
{
	dev_ent_t	*un;
	int		exit_status = 0;
	robo_event_t   *event;
	iport_state_t  *iport = (iport_state_t *)viport;
	library_t	*library = iport->library;

	un = library->un;
	mutex_lock(&iport->mutex);	/* wait for go */
	mutex_unlock(&iport->mutex);

	for (;;) {
		mutex_lock(&iport->list_mutex);
		if (iport->active_count == 0)
			cond_wait(&iport->list_condit, &iport->list_mutex);

		if (iport->active_count == 0) {	/* check to make sure */
			mutex_unlock(&iport->list_mutex);
			continue;
		}
		event = iport->first;
		iport->first = unlink_list(event);
		iport->active_count--;
		mutex_unlock(&iport->list_mutex);

		switch (event->type) {
		case EVENT_TYPE_INTERNAL:
			switch (event->request.internal.command) {

			case ROBOT_INTRL_INIT:
				init_import(iport);
				disp_of_event(library, event, 0);
				break;

			case ROBOT_INTRL_SHUTDOWN:
				mutex_lock(&library->un->mutex);
				mutex_lock(&library->un->io_mutex);
				/*
				 * We are shuting down, attempt to allow
				 * media removal from the robot.
				 */
				TAPEALERT(library->open_fd, library->un);
				if (library->un->type == DT_PLASMON_D) {
					/*
					 * In Plasmon DVD-RAM library, unlock
					 * the I/E drawer and door.
					 */
					scsi_cmd(library->open_fd,
					    library->un, SCMD_DOORLOCK,
					    0, UNLOCK, 1);	/* drawer */
					scsi_cmd(library->open_fd,
					    library->un, SCMD_DOORLOCK,
					    0, UNLOCK, 0);	/* door */
				} else {
					UNLOCK_MAILBOX(library);
				}
				TAPEALERT(library->open_fd, library->un);
				mutex_unlock(&library->un->io_mutex);
				mutex_unlock(&library->un->mutex);

				iport->thread = (thread_t)- 1;
				thr_exit(&exit_status);
				break;

			default:
				DevLog(DL_DEBUG(5148),
				    event->request.internal.command);
				break;
			}
			break;

		case EVENT_TYPE_MESS:
			if (event->request.message.magic != MESSAGE_MAGIC) {
				DevLog(DL_DEBUG(5149));
				break;
			}
			switch (event->request.message.command) {
			case MESS_CMD_IMPORT:
				if (IS_GENERIC_API(library->un->type)) {
					DevLog(DL_DEBUG(5150),
					    event->request.message.command);
				} else {
					import_media(library->im_ele, event);
				}
				break;

			case MESS_CMD_EXPORT:
				export_media((void *) library->ex_ele,
				    event, library->un->type);
				break;

			default:
				DevLog(DL_DEBUG(5150),
				    event->request.message.command);
				break;
			}
			break;

		default:
			DevLog(DL_DEBUG(5151), event->type);
			break;
		}
	}
}


/*
 *	import_media - bring media into the library
 *
 */
static void
import_media(
		iport_state_t *iport,
		robo_event_t *event)
{
	dev_ent_t	*un;
	int		err, status = 0;
	char	   *l_mess;
	uint_t	  slot, element;
	library_t	*library;
	media_t	 media;

	if (iport == NULL) {
		sam_syslog(LOG_WARNING, "No import mailbox defined");
		return;
	}
	library = iport->library;
	un = library->un;
	l_mess = library->un->dis_mes[DIS_MES_NORM];

	switch (library->un->type) {
	case DT_DLT2700:
		memccpy(l_mess, catgets(catfd, SET, 9038,
		    "import/export not supported"),
		    '\0', DIS_MES_LEN);
		DevLog(DL_ERR(5153));
		return;
	}

	if (library->un->type == DT_ACL452) {
		wait_452_import(library);
		return;
	}
	slot = FindFreeSlot(library);

	if (slot == (unsigned)ROBOT_NO_SLOT) {
		memccpy(l_mess,
		    catgets(catfd, SET, 9040,
		    "no empty storage slots for import"),
		    '\0', DIS_MES_LEN);
		DevLog(DL_ERR(5154));
		SendCustMsg(HERE, 9333);
		mutex_lock(&library->un->mutex);
		library->un->status.bits |= DVST_STOR_FULL;
		mutex_unlock(&library->un->mutex);
		err = ENOSPC;
	} else {
		uint_t	  not_sam = FALSE, audit_eod = FALSE;
		uint_t	  flags;

		/* If it came from the outside world, pick up the flags */
		if (event->type == EVENT_TYPE_MESS) {
		flags = event->request.message.param.import_request.flags;
		media = event->request.message.param.import_request.media;
			if (media == 0) {
				media = library->un->media;
			}
			audit_eod = ((flags & CMD_IMPORT_AUDIT) != 0);

			/* If not sam, then there is no audit to be done */
			if ((not_sam = ((flags & CMD_IMPORT_STRANGE) != 0))) {
				flags &= ~CMD_IMPORT_AUDIT;
				if (audit_eod)
					DevLog(DL_ERR(5155));
				audit_eod = FALSE;
			}
		}
		if (audit_eod)
			DevLog(DL_LABEL(5156));

		if (not_sam) {
			status = CES_non_sam;
			(void) CatalogSetFieldByLoc(library->un->eq,
			    slot, 0, CEF_Status,
			    status, 0);
		}
		element = ELEMENT_ADDRESS(library, slot);
		if (!(IS_GENERIC_API(library->un->type))) {

			/*
			 * Position the mailbox and allow the operator to
			 * insert media
			 */
			mutex_lock(&library->un->mutex);
			library->un->status.bits |= (DVST_ATTENTION |
			    DVST_I_E_PORT);

			/* Allow media import from mailbox. */
			UNLOCK_MAILBOX(library);

			/*
			 * Not done if Plasmon DVD-RAM library.	 After the
			 * I/E drawer is unlocked, move an empty tray to
			 * drawer.
			 */
			if (library->un->type == DT_PLASMON_D) {
				move_empty_tray(library, iport,
				    element, ROTATE_IN);
			}
			mutex_unlock(&library->un->mutex);

			/* wait for media to be inserted */
			switch (library->un->type) {
			case DT_METD360:
				process_d360_import(iport, slot, flags, media);
				break;

			case DT_SONYDMS:
				/* FALLTHROUGH */
			case DT_ADIC448:
				/* FALLTHROUGH */
			case DT_ODI_NEO:
				/* FALLTHROUGH */
			case DT_ATLP3000:
				/* FALLTHROUGH */
			case DT_QUANTUMC4:
				/* FALLTHROUGH */
			case DT_HPSLXX:
				/* FALLTHROUGH */
			case DT_PLASMON_G:
				process_multi_import(iport, slot, flags, media);
				break;
			case DT_ADIC100:
				/* FALLTHROUGH */
			case DT_ADIC1000:
				/* FALLTHROUGH */
			case DT_EXBX80:
				/* FALLTHROUGH */
			case DT_STKLXX:
				/* FALLTHROUGH */
			case DT_SL3000:
				/* FALLTHROUGH */
			case DT_IBM3584:
				/* FALLTHROUGH */
			case DT_STK97XX:
				/* FALLTHROUGH */
			case DT_FJNMXX:
				/* FALLTHROUGH */
			case DT_SLPYTHON:
				/* attempt to lock the import/export door */
				mutex_lock(&library->un->io_mutex);
				TAPEALERT(library->open_fd, library->un);
				scsi_cmd(library->open_fd, library->un,
				    SCMD_DOORLOCK, 0, LOCK);
				TAPEALERT(library->open_fd, library->un);
				mutex_unlock(&library->un->io_mutex);
				process_multi_import(iport, slot, flags, media);
				/* attempt to unlock the import/export door */
				mutex_lock(&library->un->io_mutex);
				TAPEALERT(library->open_fd, library->un);
				scsi_cmd(library->open_fd, library->un,
				    SCMD_DOORLOCK, 0, UNLOCK);
				TAPEALERT(library->open_fd, library->un);
				mutex_unlock(&library->un->io_mutex);
				break;
			default:
				process_generic_import(iport, slot,
				    flags, media);
			}
		}
	}

	mutex_lock(&library->un->mutex);
	LOCK_MAILBOX(library);
	library->un->status.bits &= ~(DVST_ATTENTION | DVST_I_E_PORT);
	library->un->status.bits |= DVST_MOUNTED;
	mutex_unlock(&library->un->mutex);
	disp_of_event(library, event, err);
}


/*
 *	For Plasmon DVD-RAM library.  Either move an empty tray
 * to the I/E drawer (importing) or move an empty tray from
 * drawer to the library (exporting).
 */
static int
move_empty_tray(
		library_t *library,
		iport_state_t *iport,
		uint_t empty_tray,
		const int direction)
{
	dev_ent_t	*un;
	int		err, retry;
	int		added_more_time = FALSE;
	sam_extended_sense_t *sense;

	sense = (sam_extended_sense_t *)SHM_REF_ADDR(library->un->sense);
	un = library->un;
	mutex_lock(&un->io_mutex);

	retry = 3;
	do {
		TAPEALERT(library->open_fd, library->un);
		memset(sense, 0, sizeof (sam_extended_sense_t));
		if (direction == ROTATE_IN) {
			/* Move tray from library to I/E drawer. */
			DevLog(DL_DETAIL(5342), empty_tray, iport->element);
			err = scsi_cmd(library->open_fd, un,
			    SCMD_MOVE_MEDIUM, 0, 0,
			    empty_tray, iport->element, 0);
		} else {
			/* Move tray from I/E drawer to library. */
			DevLog(DL_DETAIL(5342), iport->element, empty_tray);
			err = scsi_cmd(library->open_fd, un,
			    SCMD_MOVE_MEDIUM, 0, 0,
			    iport->element, empty_tray, 0);
		}
		TAPEALERT(library->open_fd, library->un);

		if (err) {
			GENERIC_SCSI_ERROR_PROCESSING(un,
			    library->scsi_err_tab, 0,
			    err, added_more_time, retry,
			/* DOWN_EQU case statement. */
			    down_library(library,
			    SAM_STATE_CHANGE);
			mutex_unlock(&un->io_mutex);
			return (-1);
			/* MACRO for cstyle */,
			/* ILLREQ case statement. */
			    mutex_unlock(&un->io_mutex);
			return (-1);
			/* MACRO for cstyle */,
			/* Any more code to execute. */
			    /* MACRO for cstyle */;
	/* MACRO for cstyle */)
		} else
			break;	/* success, empty tray moved to drawer */
	} while (--retry > 0);

	/* If move failed, log error. */
	if (retry <= 0)
		DevLog(DL_ERR(5343));
	else
		DevLog(DL_DETAIL(5344));

	mutex_unlock(&un->io_mutex);
	return (0);
}


/*
 *	export_media - bring media out of the library
 *
 */
void
export_media(
		void *element,
		robo_event_t *event,
		uint_t type)
{
	if (IS_GENERIC_API(type)) {
		api_export_media((library_t *)element, event);
	} else {
		generic_export_media((iport_state_t *)element, event);
	}
}


/*
 * Export media for a generic SCSI library.
 */
void
generic_export_media(
			iport_state_t *ex_ele,
			robo_event_t *event)
{
	dev_ent_t	*un;
	int		err = 0;
	char	   *l_mess;
	uint_t	  element;
	int		slot;
	library_t	*library;
	move_flags_t    move_flags;
	drive_state_t  *drive = NULL;
	iport_state_t  *iport;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	struct VolId    vid;
	export_request_t *request =
	    &event->request.message.param.export_request;

	if (ex_ele == NULL) {
		sam_syslog(LOG_WARNING, "No export mailbox defined");
		return;
	}
	slot = request->slot;

	library = ex_ele->library;
	un = library->un;
	l_mess = library->un->dis_mes[DIS_MES_NORM];

	switch (library->un->type) {
	case DT_DLT2700:
		memccpy(l_mess, catgets(catfd, SET, 9038,
		    "import/export not supported"),
		    '\0', DIS_MES_LEN);
		DevLog(DL_ERR(5158));
		return;

	case DT_ACL452:
		process_452_export(library);
		mutex_lock(&library->un->mutex);
		library->un->status.b.attention = TRUE;
		library->un->status.b.i_e_port = TRUE;
		UNLOCK_MAILBOX(library);
		mutex_unlock(&library->un->mutex);
		return;

	case DT_ACL2640:
		/* FALLTHROUGH */
	case DT_ATL1500:
		/* FALLTHROUGH */
	case DT_METD360:
		iport = ex_ele;
		mutex_lock(&iport->mutex);
		break;

		/* These one should never get here */
	case DT_METD28:
		iport = NULL;
		break;

	case DT_ATLP3000:
		/* FALLTHROUGH */
	case DT_SONYDMS:
		/* FALLTHROUGH */
	case DT_ADIC448:
		/* FALLTHROUGH */
	case DT_ODI_NEO:
		/* FALLTHROUGH */
	case DT_ADIC100:
		/* FALLTHROUGH */
	case DT_ADIC1000:
		/* FALLTHROUGH */
	case DT_EXBX80:
		/* FALLTHROUGH */
	case DT_STKLXX:
		/* FALLTHROUGH */
	case DT_IBM3584:
		/* FALLTHROUGH */
	case DT_QUANTUMC4:
		/* FALLTHROUGH */
	case DT_HPSLXX:
		/* FALLTHROUGH */
	case DT_STK97XX:
		/* FALLTHROUGH */
	case DT_FJNMXX:
		/* FALLTHROUGH */
	case DT_SL3000:
		/* FALLTHROUGH */
	case DT_SLPYTHON:
		/*
		 * Media changers must support init_element_range command
		 * (0xe7) to use find_empty_export.
		 *
		 * Actually, DT_QUANTUMC4, DT_HP_C7200, DT_FJNMXX and
		 * DT_SL3000 do not support the E7. However, there is
		 * special code in process_multi_import to handle these.
		 */
		if ((iport = find_empty_export(library)))
			mutex_lock(&iport->mutex);
		break;

	case DT_HP_C7200:
		if (library->range.ie_count > 0) {
			if ((iport = find_empty_export(library)))
				mutex_lock(&iport->mutex);
		} else {
			memccpy(l_mess, catgets(catfd, SET, 9038,
			    "import/export not supported"),
			    '\0', DIS_MES_LEN);
			DevLog(DL_ERR(5158));
			return;
		}
		break;

	default:
		iport = ex_ele;
		mutex_lock(&iport->mutex);
		break;
	}

	if (iport == NULL) {
		memccpy(l_mess,
		    catgets(catfd, SET, 9042, "no empty export elements"),
		    '\0', DIS_MES_LEN);

		DevLog(DL_ERR(5159));
		return;
	}
	/*
	 * If this is an adic, the the import/export door is locked at this
	 * point, be sure it gets unlocked before exiting.
	 */

	/* find the media to export */

	switch (request->flags & EXPORT_FLAG_MASK) {

	case EXPORT_BY_VSN:
		ce = CatalogGetCeByMedia(sam_mediatoa(request->media),
		    request->vsn, &ced);
		if (ce == NULL) {
			err = ENOENT;
			DevLog(DL_ERR(5165));
			goto err;
		}
		element = ELEMENT_ADDRESS(library, slot);
		break;

	case EXPORT_BY_SLOT:
		ce = CatalogGetCeByLoc(library->un->eq, slot, 0, &ced);
		element = ELEMENT_ADDRESS(library, slot);
		if ((!IS_STORAGE(library, element)) || (ce == NULL)) {
			err = EFAULT;
			DevLog(DL_ERR(5166), slot);
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

			DevLog(DL_ERR(5167), request->eq);
			goto err;
		}
		element = drive->element;
		if (!drive->status.b.full) {
			err = ENOENT;
			DevLog(DL_ERR(5168), request->eq);
			goto err;
		}
		if (drive->status.b.valid &&
		    IS_STORAGE(library, drive->element)) {
			element = drive->element;
			slot = SLOT_NUMBER(library, drive->element);
		} else {
			slot = (unsigned)ROBOT_NO_SLOT;
		}

		mutex_lock(&drive->mutex);
		mutex_lock(&drive->un->mutex);
		drive->un->status.bits |= (DVST_WAIT_IDLE | DVST_REQUESTED);
		while (!drive_is_idle(drive)) {
			mutex_unlock(&drive->un->mutex);
			mutex_unlock(&drive->mutex);
			sleep(5);
			mutex_lock(&drive->mutex);
			mutex_lock(&drive->un->mutex);
		}

		drive->un->status.bits &= ~DVST_WAIT_IDLE;
		if (drive->un->status.b.ready) {
			mutex_unlock(&drive->un->mutex);
			(void) spin_drive(drive, SPINDOWN, NOEJECT);
		} else
			mutex_unlock(&drive->un->mutex);
		break;

	default:
		err = EFAULT;
		DevLog(DL_ERR(5169), request->flags & EXPORT_FLAG_MASK);
		goto err;
	}

	/*
	 * Element and slot should now be set. If not a drive export, check
	 * to make sure the slot is occupied, if not, look over the drives
	 * and find the media.
	 */
	if (ce == NULL) {
		/*
		 * Try one more time to get the catalog entry. Could have
		 * been an export by eq.
		 */
		ce = CatalogGetCeByLoc(library->un->eq, slot, 0, &ced);
		if (ce == NULL) {
			err = ENOENT;
			DevLog(DL_ERR(5170), slot);
			goto err;
		}
	}
	memset(&vid, 0, sizeof (struct VolId));
	memmove(vid.ViMtype, ce->CeMtype, sizeof (vid.ViMtype));
	memmove(vid.ViVsn, ce->CeVsn, sizeof (vid.ViVsn));
	vid.ViEq = ce->CeEq;
	vid.ViSlot = ce->CeSlot;

	if (drive == NULL && slot != (unsigned)ROBOT_NO_SLOT) {
		if (!(ce->CeStatus & CES_inuse)) {
			err = ENOENT;
			DevLog(DL_ERR(5170), slot);
			goto err;
		}
		if (!(ce->CeStatus & CES_occupied)) {
			for (drive = library->drive; drive != NULL;
			    drive = drive->next) {
				mutex_lock(&drive->mutex);
				if (drive->status.b.full &&
				    drive->status.b.valid &&
				    element == drive->media_element)
					break;
				mutex_unlock(&drive->mutex);
			}

			if (drive == NULL) {
				err = ENOENT;
				DevLog(DL_ERR(5171));
				goto err;
			}
			mutex_lock(&drive->un->mutex);
			if (drive->un->active != 0) {
				mutex_unlock(&drive->un->mutex);
				mutex_unlock(&drive->mutex);
				err = EBUSY;
				DevLog(DL_ERR(5172));
				goto err;
			}
			drive->un->status.bits |= (DVST_REQUESTED |
			    DVST_UNLOAD);
			if (drive->un->status.b.ready) {
				mutex_unlock(&drive->un->mutex);
				(void) spin_drive(drive, SPINDOWN, NOEJECT);
			} else
				mutex_unlock(&drive->un->mutex);

			element = drive->element;
		}
	}
	switch (library->un->type) {
	case DT_HPLIBS:
		TAPEALERT(library->open_fd, library->un);
		scsi_cmd(library->open_fd, library->un,
		    SCMD_DOORLOCK, 0, UNLOCK);
		TAPEALERT(library->open_fd, library->un);
		break;

	case DT_CYGNET:
		LOCK_MAILBOX(library);
		break;

	default:
		UNLOCK_MAILBOX(library);
		break;
	}

	move_flags.bits = 0;
	if (library->un->type == DT_METD360) {
		move_flags.b.noerror = TRUE;
	}
	err = move_media(library, 0, element, iport->element, 0, move_flags);
	if (err) {
		char	   *lc_mess = library->un->dis_mes[DIS_MES_CRIT];

		if (err == D_ELE_FULL) {
			memccpy(lc_mess, catgets(catfd, SET, 9051,
			    "export hopper full"),
			    '\0', DIS_MES_LEN);
			DevLog(DL_ERR(5160));
			if (drive != NULL && slot != ROBOT_NO_SLOT) {
				if (move_media(library, 0, element,
				    drive->media_element, 0,
				    move_flags)) {
				memccpy(
				    drive->un->dis_mes[DIS_MES_CRIT],
				    catgets(catfd, SET, 9006,
				    "unable to clear drive, move failed"),
				    '\0', DIS_MES_LEN);
					DevLog(DL_ERR(5161));
					SendCustMsg(HERE, 9345);
					down_drive(drive, SAM_STATE_CHANGE);
					mutex_unlock(&drive->mutex);
				}
			}
			mutex_lock(&library->un->mutex);
			library->un->status.b.attention = FALSE;
			library->un->status.b.i_e_port = FALSE;
			if (library->un->equ_type == DT_ADIC448)
				unlock_adic(library);
			else
				LOCK_MAILBOX(library);

			mutex_unlock(&library->un->mutex);
			mutex_unlock(&iport->mutex);
			sleep(10);
			*lc_mess = '\0';
			disp_of_event(library, event, 1);
			return;
		}
		disp_of_event(library, event, err);
		DevLog(DL_ERR(5162));
		mutex_unlock(&iport->mutex);
		return;
	}
	/* If exporting from a drive, clear the drive state. */
	if (drive != NULL) {
		slot = ROBOT_NO_SLOT;
		drive->status.b.full = FALSE;
		if (drive->status.b.valid)
			slot = SLOT_NUMBER(library, drive->media_element);
		drive->status.b.valid = FALSE;
		drive->status.b.bar_code = FALSE;
		mutex_lock(&drive->un->mutex);
		drive->un->status.bits = DVST_PRESENT |
		    (drive->un->status.bits & DVST_CLEANING);
		if (drive->open_fd >= 0)
			close_unit(drive->un, &drive->open_fd);
		mutex_unlock(&drive->un->mutex);
		mutex_unlock(&drive->mutex);
	}
	vid.ViFlags = VI_cart;
	CatalogExport(&vid);

	/* Find the ce again, this time in the historian. */
	ce = CatalogGetCeByMedia(vid.ViMtype, vid.ViVsn, &ced);
	if (ce == NULL)
		goto err;

	if (library->un->equ_type == DT_ADIC448)
		unlock_adic(library);
	else if ((library->un->type != DT_STK97XX) &&
	    (library->un->type != DT_STKLXX) &&
	    (library->un->type != DT_ODI_NEO) &&
	    (library->un->type != DT_QUANTUMC4) &&
	    (library->un->type != DT_SONYDMS) &&
	    (library->un->type != DT_ADIC100) &&
	    (library->un->type != DT_ADIC1000) &&
	    (library->un->type != DT_EXBX80) &&
	    (library->un->type != DT_IBM3584) &&
	    (library->un->type != DT_HPSLXX) &&
	    (library->un->type != DT_FJNMXX) &&
	    (library->un->type != DT_SL3000) &&
	    (library->un->type != DT_SLPYTHON) &&
	    (library->un->type != DT_ATLP3000)) {
		mutex_lock(&library->un->mutex);
		library->un->status.bits |= (DVST_ATTENTION | DVST_I_E_PORT);
		UNLOCK_MAILBOX(library);
		mutex_unlock(&library->un->mutex);

		/* Wait forever for mail slot to empty. */
		if (library->un->type == DT_PLASMON_D ||
		    library->un->type == DT_PLASMON_G) {
			/* Special case for Plasmon DVD-RAM library. */
			(void) wait_for_plasmond_iport(iport, EMPTY, 0);
		} else {
			(void) wait_for_iport(iport, EMPTY, 0);
		}

		mutex_lock(&library->un->mutex);
		library->un->status.bits &= ~(DVST_ATTENTION | DVST_I_E_PORT);
		LOCK_MAILBOX(library);

		/*
		 * Not done if Plasmon DVD-RAM library.	 The I/E drawer is
		 * locked. Move the empty tray in I/E drawer back to library.
		 */
		if (library->un->type == DT_PLASMON_D) {
			if (drive != NULL) {
				/*
				 * Exported from drive.	 Find correct library
				 * element to move empty tray back to.
				 */
				move_empty_tray(library, iport,
				    ELEMENT_ADDRESS(library, slot),
				    ROTATE_OUT);
			} else {
				move_empty_tray(library, iport,
				    element, ROTATE_OUT);
			}
		}
	}
	library->un->status.bits &= ~DVST_STOR_FULL;
	library->un->status.bits |= DVST_MOUNTED;
	mutex_unlock(&library->un->mutex);
	mutex_unlock(&iport->mutex);
	disp_of_event(library, event, err);
	return;

err:

	if (library->un->equ_type == DT_ADIC448) {
		unlock_adic(library);
	}
	mutex_unlock(&iport->mutex);
	disp_of_event(library, event, err);
}


/*
 *	wait_for_iport - wait for media to be placed into or taken out of the
 * mail box.
 */
int
wait_for_iport(
		iport_state_t *import,	/* the element */
		int what,	/* empty or full */
		int wtime)
{
	char	   *lc_mess = import->library->un->dis_mes[DIS_MES_CRIT];
	time_t	  when = time(NULL);
	library_t	*library = import->library;

	when += ((wtime > 2) ? wtime : 2);	/* At lease two seconds */

	if (what == FULL) {	/* wait for media to arrive */
		while (when > time(NULL)) {
			memccpy(lc_mess,
			    catgets(catfd, SET, 9053,
			    "place media in import area"),
			    '\0', DIS_MES_LEN);
			sleep(2);

			mutex_lock(&library->mutex);
			mutex_lock(&library->un->mutex);
			update_element_status(library, IMPORT_EXPORT_ELEMENT);
			mutex_unlock(&library->un->mutex);
			mutex_unlock(&library->mutex);

			/* Only check media changers that set the access bit. */
			if (import->status.b.full) {
				switch (library->un->equ_type) {
				case DT_ADIC100:
					/* FALLTHROUGH */
				case DT_ADIC448:
					/* FALLTHROUGH */
				case DT_STK97XX:
					/* FALLTHROUGH */
				case DT_ACL452:
					/* FALLTHROUGH */
				case DT_3570C:
					/* FALLTHROUGH */
				case DT_HPLIBS:
				case DT_PLASMON_D:
				case DT_PLASMON_G:
					{
						if (import->status.b.access) {
							*lc_mess = '\0';
							return (0);
						}
					}
					break;

				default:
					*lc_mess = '\0';
					return (0);
				}
			}
		}
		*lc_mess = '\0';
		return (1);
	}
	for (;;) {
		memccpy(lc_mess,
		    catgets(catfd, SET, 9054, "remove media from export area"),
		    '\0', DIS_MES_LEN);
		mutex_lock(&library->mutex);
		mutex_lock(&library->un->mutex);
		update_element_status(library, IMPORT_EXPORT_ELEMENT);
		mutex_unlock(&library->un->mutex);
		mutex_unlock(&library->mutex);
		/*
		 * Return if media removed from export area or if this is a
		 * Plasmon DVD-RAM library and I/E drawer was closed with
		 * media still in it.
		 */
		if (!import->status.b.full ||
		    ((library->un->equ_type == DT_PLASMON_D ||
		    library->un->type == DT_PLASMON_G) &&
		    import->status.b.open == 0)) {
			*lc_mess = '\0';
			return (0);
		}
		sleep(2);
	}
}


/*
 * Special case for Plasmon DVD-RAM library I/E drawer.
 *
 * If import, wait until the drawer is closed.	After drawer
 * is closed check if there is media in the tray.
 *
 * If export, wait until drawer is closed and media is removed.
 * Reopen drawer if media was not removed.
 *
 * Returns: 0 if media found in I/E drawer.
 *			1 if no media found in I/E drawer.
 */
static int
wait_for_plasmond_iport(
			iport_state_t *import,	/* I/E element */
			int what,
			int wait_time	/* time to wait */
)
{
	int		no_media;
	int		drawer_open;
	int		err;
	int		media_present;

	library_t	*library = import->library;
	no_media = 0;

	if (what == FULL) {
		/* Import.	Wait until I/E drawer is closed. */
		drawer_open = 1;
		while (drawer_open) {
			no_media = wait_for_iport(import, what, wait_time);
			/*
			 * Open status flag will be zero when drawer is
			 * closed.
			 */
			drawer_open = import->status.b.open;
		}
	} else {
		/*
		 * Export. Wait until I/E drawer is closed AND no media is
		 * present in the drawer.
		 */
		/* Export doesn't return until NO media in drawer */
		no_media = 1;

		media_present = 1;
		while (media_present) {
			(void) wait_for_iport(import, what, wait_time);
			media_present = import->status.b.full;
			if (media_present && library->un->equ_type ==
			    DT_PLASMON_G) {
				UNLOCK_MAILBOX(library);
				mutex_lock(&library->un->io_mutex);
				scsi_cmd(library->open_fd, library->un,
				    SCMD_OPEN_CLOSE_MAILSLOT, 0,
				    PLASMON_G_OPEN);
				mutex_unlock(&library->un->io_mutex);
			} else if (media_present) {
				/*
				 * Drawer closed but media detected.  Reopen
				 * drawer.
				 */
				/* rotate_mailbox(library, ROTATE_OUT); */
				mutex_lock(&library->un->io_mutex);
				err = scsi_cmd(library->open_fd,
				    library->un, 0xd, 0, 1);
				mutex_unlock(&library->un->io_mutex);
			}
		}
	}
	return (no_media);
}


void
process_generic_import(
			iport_state_t *iport,
			uint_t fslot,	/* slot for first media */
			uint_t flags,
			media_t media)
{
	dev_ent_t	*un;
	int		waittime;
	uint_t	  audit_eod, not_sam;
	char	   *l_mess;
	uint_t	  slot = fslot, element;
	uchar_t	*buffer = NULL;
	library_t	*library;
	move_flags_t    move_flags;
	storage_element_t *storage_descrip;
	element_status_page_t *status_page;
	storage_element_ext_t *extension;
	struct CatalogEntry ced;
	struct CatalogEntry ced2;
	struct CatalogEntry *ce = &ced;
	struct CatalogEntry *ce2 = &ced2;
	int		no_media, status = 0;

	audit_eod = ((flags & CMD_IMPORT_AUDIT) != 0);
	not_sam = ((flags & CMD_IMPORT_STRANGE) != 0);

	library = iport->library;
	un = library->un;

	l_mess = library->un->dis_mes[DIS_MES_NORM];
	buffer = malloc_wait(library->ele_dest_len * 3, 2, 0);
	status_page = (element_status_page_t *)
	    (buffer + sizeof (element_status_data_t));
	storage_descrip = (storage_element_t *)
	    ((char *)status_page + sizeof (element_status_page_t));
	extension = (storage_element_ext_t *)
	    ((char *)storage_descrip + sizeof (storage_element_t));

	for (;;) {
		waittime = IMPORT_WAIT_TIME;
		if (library->un->type == DT_ACL2640) {
			int		retry;
			sam_extended_sense_t *sense = (sam_extended_sense_t *)
			    SHM_REF_ADDR(library->un->sense);

			waittime = IMPORT_WAIT_TIME;
			mutex_lock(&library->un->io_mutex);

			/* Clear any check conditions with TUR */
			for (retry = 15; retry > 0; retry--) {
				if (scsi_cmd(library->open_fd, library->un,
				    SCMD_TEST_UNIT_READY, 20) < 0) {
					if (sense->es_key != 0) {
						sleep(2);
						continue;
					}
				}
				break;
			}
			if (retry <= 0) {
				/* Timeout - give up */
				mutex_unlock(&library->un->io_mutex);
				free(buffer);
				return;
			}
			memccpy(l_mess,
			    catgets(catfd, SET, 9053,
			    "place media in import area"),
			    '\0', DIS_MES_LEN);

			DevLog(DL_DETAIL(5173));
			scsi_cmd(library->open_fd, library->un,
			    SCMD_READY_INPORT, 90, iport->element);

			/*
			 * Wait 2 secs, TUR waiting for Import/Export Element
			 * Accessed)
			 */
			for (retry = 15; retry > 0; retry--) {
				DevLog(DL_DEBUG(5189));
				sleep(2);
				if (scsi_cmd(library->open_fd, library->un,
				    SCMD_TEST_UNIT_READY, 20) < 0) {
					/*
					 * 06,28,01 -- the inport door was
					 * closed
					 */
					DevLog(DL_DETAIL(5190), sense->es_key,
					    sense->es_add_code,
					    sense->es_qual_code);
					if ((sense->es_key ==
					    KEY_UNIT_ATTENTION) &&
					    (sense->es_add_code == 0x28) &&
					    (sense->es_qual_code == 0x01))
						break;
				} else if ((sense->es_key == KEY_NOT_READY) &&
				    (sense->es_qual_code ==
				    ERR_ASCQ_BECOMMING)) {
					/* if becoming ready, try again */
					DevLog(DL_DETAIL(5191), sense->es_key,
					    sense->es_add_code,
					    sense->es_qual_code);
					retry++;
					continue;
				}
				DevLog(DL_DETAIL(5192), sense->es_key,
				    sense->es_add_code,
				    sense->es_qual_code);
			}
			if (retry <= 0) {
				/* Timeout - give up - door was never closed */
				memccpy(l_mess,
				    catgets(catfd, SET, 9255,
				    "import complete"),
				    '\0', DIS_MES_LEN);
				mutex_unlock(&library->un->io_mutex);
				free(buffer);
				return;
			}
			/* Clear any check conditions with TUR */
			DevLog(DL_DEBUG(5193));
			for (retry = 15; retry > 0; retry--) {
				if (scsi_cmd(library->open_fd, library->un,
				    SCMD_TEST_UNIT_READY, 20) < 0) {
					DevLog(DL_DEBUG(5194), sense->es_key,
					    sense->es_add_code,
					    sense->es_qual_code);
					if (sense->es_key != 0) {
						sleep(2);
						continue;
					}
				}
				break;
			}
			mutex_unlock(&library->un->io_mutex);
			if (retry <= 0) {
				/* Timeout - give up */
				memccpy(l_mess,
				    catgets(catfd, SET, 9255,
				    "import complete"),
				    '\0', DIS_MES_LEN);
				free(buffer);
				return;
			}
		}
		DevLog(DL_DEBUG(5195));

		/* Special wait for Plasmon DVD-RAM library. */
		if (library->un->type == DT_PLASMON_D ||
		    library->un->type == DT_PLASMON_G) {
			no_media = wait_for_plasmond_iport(iport,
			    FULL, waittime);

			if (no_media) {
				/*
				 * If I/E drawer closed and no media in it,
				 * ove the empty tray in E/E drawer back to
				 * library.
				 */
				element = ELEMENT_ADDRESS(library, slot);
				if (iport->status.b.tray)
					move_empty_tray(library, iport,
					    element, ROTATE_OUT);
				if (buffer != NULL)
					free(buffer);
				return;
			}
		} else {
			if (library->un->type != DT_SONYCSM) {
				if (wait_for_iport(iport, FULL, waittime)) {
					if (library->un->type == DT_ACL2640) {
				memccpy(l_mess, catgets(catfd, SET, 9255,
				    "import complete"),
				    '\0', DIS_MES_LEN);
					}
					if (buffer != NULL)
						free(buffer);
					return;
				}
			}
		}

		if (slot == ROBOT_NO_SLOT) {
			slot = FindFreeSlot(library);
			if (slot == ROBOT_NO_SLOT) {
				DevLog(DL_ERR(5176));
				SendCustMsg(HERE, 9334);
				mutex_lock(&library->un->mutex);
				library->un->status.bits |= DVST_STOR_FULL;
				mutex_unlock(&library->un->mutex);
				if (buffer != NULL) {
					free(buffer);
				}
				return;
			}
		}
		move_flags.bits = 0;
		element = ELEMENT_ADDRESS(library, slot);
		if (DBG_LVL(SAM_DBG_DEBUG))
			sprintf(l_mess, "importing media to slot %d", slot);

		if (move_media(library, 0, iport->element,
		    element, 0, move_flags)) {
			free(buffer);
			return;
		}
		memset(buffer, 0, library->ele_dest_len);
		if (read_element_status(library, STORAGE_ELEMENT,
		    element, 1, buffer,
		    library->ele_dest_len * 3) > 0) {
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
				/*
				 * If there is a catalog entry, and the
				 * entry's capacity is zero, and not set to
				 * zero by the user, set the capacity to
				 * default.
				 */
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
		/* If not_sam clear the audit and labeled bit */
		if (not_sam) {
			status = CES_non_sam;
			(void) CatalogSetFieldByLoc(library->un->eq, slot, 0,
			    CEF_Status, status, 0);
		}
		/*
		 * Check if should issue a slot audit for the  newly imported
		 * media.
		 */
		if (library->status.b.two_sided) {
			ce = CatalogGetCeByLoc(library->un->eq, slot, 1, &ced);
			ce2 = CatalogGetCeByLoc(library->un->eq,
			    slot, 2, &ced2);
			if ((ce != NULL) && (ce2 != NULL)) {
				if ((ce->CeStatus & CES_needs_audit) ||
				    (ce2->CeStatus & CES_needs_audit) ||
				    (audit_eod)) {
					start_slot_audit(library,
					    ce->CeSlot, audit_eod);
				}
			}
		} else {
			ce = CatalogGetCeByLoc(library->un->eq, slot, 0, &ced);
			if ((ce != NULL) &&
			    ((ce->CeStatus & CES_needs_audit) ||
			    (audit_eod)) &&
			    (!(ce->CeStatus & CES_cleaning))) {
				start_slot_audit(library, ce->CeSlot,
				    audit_eod);
			}
		}
		slot = ROBOT_NO_SLOT;

		if (library->un->type == DT_SONYCSM) {
			if (wait_for_iport(iport, FULL, waittime)) {
				if (buffer != NULL)
					free(buffer);
				return;
			}
		}
	}
}
