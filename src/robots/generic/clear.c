/* ------------------------------------------------------------------ */
/*	clear.c - routines for clearing elements */
/* ------------------------------------------------------------------ */

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

#pragma ident "$Revision: 1.41 $"

/* Using __FILE__ makes duplicate strings */
static char    *_SrcFile = __FILE__;

#include <thread.h>
#include <synch.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/syslog.h>
#include <sys/shm.h>

#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/dev_log.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "sam/defaults.h"
#include "aml/robots.h"
#include "sam/nl_samfs.h"
#include "generic.h"
#include "element.h"
#include "aml/external_data.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/tapealert.h"


/*	function prototypes */


/*	some globals */
extern pid_t    mypid;
extern shm_alloc_t master_shm, preview_shm;


/*
 *	clear_drive - clear any media in the drive.
 *
 * drive->mutex held on entry.
 * Note: The drive->mutex may be released and re-acquired.
 */
int
clear_drive(drive)
	drive_state_t  *drive;
{
	char	   *d_mess;
	dev_ent_t	*un;
	int		good_addr;
	int		mid;
	int		slot;
	move_flags_t    move_flags;
	vsn_t	   held_vsn;

	good_addr = FALSE;
	mid = ROBOT_NO_SLOT;

	SANITY_CHECK(drive != (drive_state_t *)0);
	un = drive->un;
	SANITY_CHECK(un != (dev_ent_t *)0);
	d_mess = drive->un->dis_mes[DIS_MES_NORM];
	SANITY_CHECK(drive->library != (struct library_s *)0);
	SANITY_CHECK(drive->library->un != (dev_ent_t *)0);

	if (un->state > DEV_OFF) {
		return (0);
	}
	if (!drive->status.b.full) {
		return (0);
	}
	un->i.ViPart = 0;

	mutex_lock(&un->mutex);
	un->status.bits |= (DVST_WAIT_IDLE | DVST_REQUESTED);
	memcpy(&held_vsn, &un->vsn, sizeof (vsn_t));

	if (un->state == DEV_IDLE || un->state == DEV_UNAVAIL) {
		while (un->active != 0) {
			mutex_unlock(&drive->mutex);
			mutex_unlock(&un->mutex);

			sleep(5);

			mutex_lock(&drive->mutex);
			mutex_lock(&un->mutex);
		}

	} else {
		while (!drive_is_idle(drive) && (un->state < DEV_OFF)) {
			mutex_unlock(&un->mutex);
			mutex_unlock(&drive->mutex);

			sleep(5);

			mutex_lock(&drive->mutex);
			mutex_lock(&un->mutex);
		}
	}

	un->status.bits &= ~DVST_WAIT_IDLE;
	un->status.bits |= DVST_UNLOAD;

	/* Do we know the catalog location */
	if (IS_GENERIC_API(drive->library->un->type)) {
#if !defined(SAM_OPEN_SOURCE)
		api_errs_t	ret;
		char	   *tag;
		int		d_errno;
		int		err;
		int		last_derrno;
		int		local_retry;

		tag = "force";
		last_derrno = -1;

		mutex_unlock(&un->mutex);

		un->i.ViFlags = 0;
		memmove(un->i.ViMtype, sam_mediatoa(un->type),
				    sizeof (un->i.ViMtype));
		memmove(un->i.ViVsn, un->vsn, sizeof (un->i.ViVsn));
		un->i.ViEq = un->fseq;
		un->i.ViSlot = un->slot;

		if (un->i.ViPart != 0) {
			un->i.ViFlags |= VI_part;
		}
		(void) spin_drive(drive, SPINDOWN, NOEJECT);

		mutex_lock(&un->mutex);
		un->status.bits &= ~DVST_UNLOAD;
		mid = un->mid;
		mutex_unlock(&un->mutex);

		local_retry = 3;
		ret = API_ERR_TR;
		memccpy(d_mess, catgets(catfd, SET, 9009,
		    "waiting for media changer"),
			'\0', DIS_MES_LEN);

		while (local_retry > 0) {

			if (drive->aci_drive_entry->volser[0] != '\0') {
				err = aci_dismount_media(drive->library,
					    drive, &d_errno);
			} else {
				err = aci_force_media(drive->library,
					    drive, &d_errno);
			}

			if (err == 0) {
				break;
			} else {
				/* Error return on api call */
				if (d_errno == 0) {
					/*
					 * If call did not happen - error
					 * return but no error
					 */
					local_retry = -1;
					d_errno = EAMUCOMM;
				} else if (d_errno == EDEVEMPTY) {
					/* drive was empty - done */
					local_retry = -2;
					d_errno = EOK;
				} else if ((last_derrno == -1) ||
					    (last_derrno != d_errno)) {
					/* Save error if repeated */
			/* bad indentation due to cstyle requirements */
					last_derrno = d_errno;
					if (api_valid_error(
					    drive->library->un->type,
						d_errno, drive->library->un)) {
						if (un->slot != ROBOT_NO_SLOT) {
						DevLog(DL_DEBUG(6001),
							un->slot, tag,
							d_errno, d_errno,
							api_return_message(
						    drive->library->un->type,
							d_errno));
						} else {
						DevLog(DL_DEBUG(6043), tag,
							d_errno, d_errno,
							api_return_message(
						    drive->library->un->type,
							d_errno));
						}

					local_retry = api_return_retry(
					drive->library->un->type, d_errno);
					ret = api_return_degree(
					drive->library->un->type, d_errno);
					} else {
						local_retry = -2;
					}
				}
				if (local_retry > 0) {
					/* delay before retrying */
					local_retry--;
					if (local_retry > 0) {
					sleep(api_return_sleep(
					drive->library->un->type, d_errno));
					}
				}
			}
		}

		if (d_errno != EOK) {

			if (un->slot != ROBOT_NO_SLOT) {
				DevLog(DL_ERR(6003), un->slot);
			} else {
				DevLog(DL_ERR(6044));
			}

			memccpy(d_mess,
				catgets(catfd, SET, 9006,
				    "can not clear drive, move failed"),
				'\0', DIS_MES_LEN);

			if (local_retry == -1) {
				/* The call didn't happen */
				DevLog(DL_ERR(6040), tag);
			} else if (local_retry == 0) {
				/* retries exceeded */
				DevLog(DL_ERR(6039), tag);
			}
			if (api_valid_error(drive->library->un->type,
			    d_errno, drive->library->un)) {
				if (un->slot != ROBOT_NO_SLOT) {
				DevLog(DL_ERR(6001), un->slot, tag,
				    d_errno, d_errno, api_return_message(
				    drive->library->un->type, d_errno));
				} else {
				DevLog(DL_ERR(6043), tag, d_errno,
				    d_errno, api_return_message(
				    drive->library->un->type, d_errno));
				}
			}
			mutex_lock(&un->mutex);
			un->status.bits &= ~DVST_REQUESTED;
			mutex_unlock(&un->mutex);

			if (ret == API_ERR_DD) {
				down_drive(drive, SAM_STATE_CHANGE);
				return (-1);
			} else if (ret == API_ERR_DL) {
				down_library(drive->library, SAM_STATE_CHANGE);
			}
			return (1);
		}
		if (un->i.ViMtype[0] != '\0') {
			un->i.ViFlags |= VI_mtype;
		}
		if (un->i.ViVsn[0] != '\0') {
			un->i.ViFlags |= VI_vsn;
		}
		CatalogVolumeUnloaded(&un->i, (char *)drive->bar_code);

		mutex_lock(&un->mutex);
		memset(&un->vsn, 0, sizeof (vsn_t));
		un->slot = ROBOT_NO_SLOT;
		un->mid = ROBOT_NO_SLOT;
		un->status.bits = ((un->status.bits & DVST_CLEANING) |
							    DVST_PRESENT);
		un->status.bits &= ~DVST_REQUESTED;
		un->label_time = 0;
		mutex_unlock(&un->mutex);

		*d_mess = '\0';

		drive->status.b.full = FALSE;
		drive->status.b.valid = FALSE;

		/* clear the barcode information since this drive was cleared */

		drive->status.b.bar_code = FALSE;
		drive->aci_drive_entry->volser[0] = '\0';
		clear_driver_idle(drive, drive->open_fd);

		if (un->slot != ROBOT_NO_SLOT) {
			DevLog(DL_ALL(3236), held_vsn, un->slot);
		} else {
			DevLog(DL_ALL(3248), held_vsn);
		}

#endif
		return (0);

	} else {		/* Not a GRAU */

		if (un->slot != (unsigned)ROBOT_NO_SLOT) {
			slot = un->slot;
		} else {
			slot = ROBOT_NO_SLOT;
		}

		if (!drive->status.b.valid) {
			drive->media_element = ELEMENT_ADDRESS(
							drive->library, slot);
			drive->status.b.valid = TRUE;
		}
		/* Do we know where to put this media yet? */
		if (!good_addr && (good_addr = (drive->status.b.valid &&
			IS_STORAGE(drive->library, drive->media_element)))) {
			struct CatalogEntry ced;
			struct CatalogEntry *ce = &ced;

			/*
			 * The entry pointer in the dev_ent must not be
			 * valid. Attempt to find the entry by slot number.
			 * since the valid bit is set, the element and invert
			 * flags are good. Determine the slot number from the
			 * element address
			 */
			un->slot = SLOT_NUMBER(drive->library,
						drive->media_element);

			if (drive->library->status.b.two_sided) {
				if (drive->status.b.d_st_invert) {
					un->i.ViPart = 2;
				} else {
					un->i.ViPart = 1;
				}
			}
			/* The storage must not be occupied */
			ce = CatalogGetCeByLoc(drive->library->un->eq,
				    un->slot, un->i.ViPart, &ced);
			if ((ce == NULL) || (ce->CeStatus & CES_occupied))
				good_addr = FALSE;
		}
		mutex_unlock(&un->mutex);

		/*
		 * To be a good address at this time, the valid bit must have
		 * been on, the address is storage and that storage address
		 * is not occupied and available. If this is not the case,
		 * find a spot to put the media.
		 */
		if (!good_addr) {
			struct CatalogEntry ced;
			struct CatalogEntry *ce = &ced;

			(void) spin_drive(drive, SPINDOWN, NOEJECT);

			mutex_lock(&un->mutex);
			un->status.bits = ((un->status.bits & DVST_CLEANING) |
								DVST_PRESENT |
							    DVST_REQUESTED);
			mutex_unlock(&un->mutex);

			drive->status.b.d_st_invert = FALSE;

			slot = ROBOT_NO_SLOT;

			if (drive->status.b.bar_code) {
				ce = CatalogGetCeByBarCode(
						    drive->library->un->eq,
							sam_mediatoa(un->type),
					    (char *)drive->bar_code, &ced);
				if (ce != NULL) {
					slot = ce->CeSlot;
				}
			} else {
				if (&held_vsn[0] != '\0') {
					ce = CatalogGetCeByMedia(
							sam_mediatoa(un->type),
							held_vsn, &ced);
					if (ce != NULL) {
						slot = ce->CeSlot;
					}
				}
			}

			if (slot != ROBOT_NO_SLOT) {

				mutex_lock(&un->mutex);
				un->slot = slot;
				mutex_unlock(&un->mutex);

				drive->media_element = ELEMENT_ADDRESS(
							drive->library, slot);
				drive->status.b.valid = TRUE;

			} else { /* slot == ROBOT_NO_SLOT */

				if ((slot = FindFreeSlot(drive->library)) ==
							ROBOT_NO_SLOT) {
				char *dc_mess = un->dis_mes[DIS_MES_CRIT];

					memccpy(dc_mess,
						GetCustMsg(9003),
						'\0', DIS_MES_LEN);
					DevLog(DL_ERR(5134));
					SendCustMsg(HERE, 9003);
					SendCustMsg(HERE, 9330);
					down_drive(drive, SAM_STATE_CHANGE);

					mutex_lock(&un->mutex);
					un->status.bits &= ~DVST_REQUESTED;
					mutex_unlock(&un->mutex);

					clear_driver_idle(drive,
							drive->open_fd);

					return (-1);
				}
				mutex_lock(&un->mutex);

				un->mid = mid;
				un->slot = slot;
				un->flip_mid = ROBOT_NO_SLOT;

				mutex_unlock(&un->mutex);

				drive->media_element = ELEMENT_ADDRESS(
							drive->library, slot);
				drive->status.b.valid = TRUE;
			}

			drive->media_element = ELEMENT_ADDRESS(
						drive->library, un->slot);
			move_flags.bits = 0;
			memccpy(d_mess,
				catgets(catfd, SET, 9009,
					"waiting for media changer"),
				'\0', DIS_MES_LEN);

			if (move_media(drive->library, 0, drive->element,
					drive->media_element, 0, move_flags)) {
				char	   *dc_mess = un->dis_mes[DIS_MES_CRIT];

				if (un->slot != ROBOT_NO_SLOT) {
					/*
					 * Log error message include slot
					 * number for diagnostic purposes.
					 */
					DevLog(DL_ERR(5001), un->slot);
				} else {
					/*
					 * Either device does not identify
					 * slots by slot number (device is
					 * manual) or slot number is unknown.
					 * Log error message, omit slot
					 * number.
					 */
					DevLog(DL_ERR(5357));
				}

				memccpy(dc_mess,
					catgets(catfd, SET, 9006,
					"can not clear drive, move failed"),
					'\0', DIS_MES_LEN);

				mutex_lock(&un->mutex);
				un->status.bits &= ~DVST_REQUESTED;
				mutex_unlock(&un->mutex);

				clear_driver_idle(drive, drive->open_fd);
				down_drive(drive, SAM_STATE_CHANGE);

				return (-1);
			} else {
				/*
				 * Need to read the barcode if spectralogics
				 * or its an adic and there is no barcode
				 * reported for the drive.
				 */
				struct VolId    vid;
				uint32_t	CatStatus;

				memset(&vid, 0, sizeof (vid));

				vid.ViFlags = VI_cart;
				vid.ViEq = drive->library->un->eq;
				vid.ViSlot = slot;
				vid.ViPart = 0;

				memmove(vid.ViMtype,
				sam_mediatoa(drive->library->un->media),
					sizeof (vid.ViMtype));
		/* bad indentation due to cstyle requirements */

		if (drive->library->un->equ_type == DT_SPECLOG ||
		    (drive->library->un->equ_type == DT_ADIC448 &&
			!drive->status.b.bar_code)) {
			memccpy(drive->library->un->dis_mes[DIS_MES_NORM],
				catgets(catfd, SET, 9008, "reading barcode"),
				'\0', DIS_MES_LEN);
			status_element_range(drive->library, STORAGE_ELEMENT,
				drive->media_element, 1);
			}
				CatStatus = CES_inuse | CES_occupied;

				if (drive->status.b.bar_code) {
					CatStatus |= CES_bar_code;

				(void) CatalogSlotInit(&vid, CatStatus,
				(drive->library->status.b.two_sided) ? 2 : 0,
				(char *)drive->bar_code, "");
				} else {
					(void) CatalogSlotInit(&vid, CatStatus,
					(drive->library->status.b.two_sided) ?
					2 : 0, "", "");
				}

				drive->status.b.full = FALSE;
				drive->status.b.valid = FALSE;
				drive->status.b.bar_code = FALSE;

				memset(drive->bar_code, 0, BARCODE_LEN);
				memset(&un->vsn, 0, sizeof (vsn_t));

				mutex_lock(&un->mutex);

				un->status.bits &= ~DVST_REQUESTED;
				un->flip_mid = ROBOT_NO_SLOT;
				un->mid = un->flip_mid;
				un->slot = ROBOT_NO_SLOT;
				un->status.bits = (DVST_PRESENT |
					(un->status.bits & DVST_CLEANING));

				mutex_unlock(&un->mutex);

				clear_driver_idle(drive, drive->open_fd);

				DevLog(DL_ALL(3248), held_vsn);

				return (0);
			}
		}
		/* address is good, just put it away */
		un->i.ViFlags = VI_cart;
		memmove(un->i.ViMtype, sam_mediatoa(un->type),
				sizeof (un->i.ViMtype));
		if (*un->i.ViMtype != '\0') {
			un->i.ViFlags |= VI_mtype;
		}
		memmove(un->i.ViVsn, un->vsn, sizeof (un->i.ViVsn));
		if (*un->i.ViVsn != '\0') {
			un->i.ViFlags |= VI_vsn;
		}
		un->i.ViEq = un->fseq;
		un->i.ViSlot = un->slot;
		if (un->i.ViPart != 0) {
			un->i.ViFlags |= VI_part;
		}
		(void) spin_drive(drive, SPINDOWN, NOEJECT);

		mutex_lock(&un->mutex);
		un->status.bits = (DVST_PRESENT | DVST_REQUESTED |
					(un->status.bits & DVST_CLEANING));
		clear_driver_idle(drive, drive->open_fd);
		mutex_unlock(&un->mutex);

		move_flags.bits = 0;
		memccpy(d_mess, catgets(catfd, SET, 9009,
			"waiting for media changer"),
			'\0', DIS_MES_LEN);

		if (move_media(drive->library, 0, drive->element,
				drive->media_element,
				drive->status.b.d_st_invert, move_flags)) {
			char	   *dc_mess = un->dis_mes[DIS_MES_CRIT];

			/*
			 * Report error message, include slot number if used,
			 * known, and available, otherwise, omit slot number
			 * from error message.
			 */
			if (slot != ROBOT_NO_SLOT) {
				DevLog(DL_ERR(5126), slot);
			} else {
				DevLog(DL_ERR(5358));
			}

			memccpy(dc_mess,
				catgets(catfd, SET, 9006,
				"can not clear drive, move failed"),
				'\0', DIS_MES_LEN);

			mutex_lock(&un->mutex);
			un->status.bits &= ~DVST_REQUESTED;
			mutex_unlock(&un->mutex);

			down_drive(drive, SAM_STATE_CHANGE);

			return (-1);
		}
		memccpy(d_mess,
			catgets(catfd, SET, 9010, "drive cleared"),
			'\0', DIS_MES_LEN);

		CatalogVolumeUnloaded(&un->i, (char *)drive->bar_code);

		mutex_lock(&un->mutex);
		un->flip_mid = ROBOT_NO_SLOT;
		un->mid = un->flip_mid;
		un->slot = ROBOT_NO_SLOT;
		un->status.bits &= ~DVST_REQUESTED;
		un->label_time = 0;
		memset(&un->vsn, 0, sizeof (vsn_t));
		mutex_unlock(&un->mutex);

		drive->status.b.full = FALSE;
		drive->status.b.valid = FALSE;
		drive->status.b.bar_code = FALSE;
		memset(drive->bar_code, 0, BARCODE_LEN);

		if (slot != ROBOT_NO_SLOT) {
			DevLog(DL_ALL(3236), held_vsn, slot);
		} else {
			DevLog(DL_ALL(3248), held_vsn);
		}

		return (0);
	}
}


/*
 *	clear_transport - clear media in transport
 *
 * Clear transport should only be called during initialization or
 * error recovery.
 *
 * exit -
 *	  0 - ok
 *	  !0 - failure
 *
 */
int
    clear_transport(library_t *library,
    xport_state_t *transport) {
	dev_ent_t	*un;
	int		retry, err = 0;
	char	   *l_mess;
	int		slot, status = 0;
	int		added_more_time = FALSE;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	sam_extended_sense_t *sense;


	/*
	 * If source address is valid and points to empty storage, return it
	 * to the storage.
	 */
	SANITY_CHECK(library != (library_t *)0);
	sense = (sam_extended_sense_t *)SHM_REF_ADDR(library->un->sense);
	SANITY_CHECK(transport != (xport_state_t *)0);

	if (!transport->status.b.full)
		return (0);

	SANITY_CHECK(library != (library_t *)0);
	SANITY_CHECK(library->un != (dev_ent_t *)0);
	l_mess = library->un->dis_mes[DIS_MES_NORM];

	un = library->un;
	DevLog(DL_DETAIL(5002), transport->element);
	memccpy(l_mess, catgets(catfd, SET, 9011, "clearing transport"),
		'\0', DIS_MES_LEN);

	if ((un->type == DT_METD360) && (transport->element == 0x3)) {
		iport_state_t  *import = library->import;
		for (; import != NULL && !import->status.b.exenab;
			import = import->next) {
		}
		if (import != NULL) {
			mutex_lock(&library->un->io_mutex);
			retry = 2;
			do {
				TAPEALERT(library->open_fd, library->un);
				memset(sense, 0, sizeof (sam_extended_sense_t));
				if (scsi_cmd(library->open_fd, library->un,
				SCMD_MOVE_MEDIUM, 0, 0, transport->element,
				import->element, 0) < 0) {
				TAPEALERT_SKEY(library->open_fd, library->un);
				GENERIC_SCSI_ERROR_PROCESSING(library->un,
					    library->scsi_err_tab, 0, err,
					    added_more_time, retry,
					/* DOWN_EQ code */
					down_library(library, SAM_STATE_CHANGE);
					mutex_unlock(&library->un->io_mutex);
					return (-1);
				    /* MACRO for cstyle */,
					/* ILLREQ code */
					mutex_unlock(&library->un->io_mutex);
					return (-1);
					/* MACRO for cstyle */,
					    /* MACRO for cstyle */;
			/* MACRO for cstyle */)
				} else
					break;
			} while (--retry > 0);
			TAPEALERT(library->open_fd, library->un);
			mutex_unlock(&library->un->io_mutex);
			if (retry <= 0) {
				DevLog(DL_ERR(5210));
				return (-1);
			}
			return (0);
		}
	}
	slot = SLOT_NUMBER(library, transport->media_element);
	ce = CatalogGetCeByLoc(library->un->eq, slot, 0, &ced);
	if (ce == NULL) {
		DevLog(DL_ERR(5210));
		return (-1);
	}
	if (transport->status.b.valid &&
	    IS_STORAGE(library, transport->media_element) &&
	    !(ce->CeStatus & CES_occupied)) {
		mutex_lock(&library->un->io_mutex);
		retry = 2;
		do {
			TAPEALERT(library->open_fd, library->un);
			memset(sense, 0, sizeof (sam_extended_sense_t));
			if (scsi_cmd(library->open_fd, library->un,
				SCMD_MOVE_MEDIUM, 0,
				transport->element, transport->element,
				transport->media_element,
				transport->status.b.invert) < 0) {
				TAPEALERT_SKEY(library->open_fd, library->un);
				GENERIC_SCSI_ERROR_PROCESSING(library->un,
				    library->scsi_err_tab, 0, err,
							added_more_time, retry,
				/* DOWN_EQ code */
				    down_library(library, SAM_STATE_CHANGE);
				mutex_unlock(&library->un->io_mutex);
				return (-1);
				/* MACRO for cstyle */,
				/* ILLREQ code */
				    mutex_unlock(&library->un->io_mutex);
				return (-1);
				/* MACRO for cstyle */,
				    /* MACRO for cstyle */;
		/* MACRO for cstyle */)
			} else
				break;
		} while (--retry > 0);
		TAPEALERT(library->open_fd, library->un);

		mutex_unlock(&library->un->io_mutex);
		if (retry > 0) {
			transport->status.b.full = FALSE;
			transport->status.b.valid = FALSE;
			return (0);
		} else {
			DevLog(DL_ERR(5211));
			return (-1);
		}
	}
	/* Put it in an empty slot and set the audit needed bit */
	if ((slot = FindFreeSlot(library)) == ROBOT_NO_SLOT) {
		char	   *lc_mess = library->un->dis_mes[DIS_MES_CRIT];

		memccpy(lc_mess, GetCustMsg(9012), '\0', DIS_MES_LEN);
		SendCustMsg(HERE, 9012);
		DevLog(DL_ERR(5135), transport->element);
		SendCustMsg(HERE, 9331, transport->element);
		return (-1);
	}
	mutex_lock(&library->un->io_mutex);
	added_more_time = FALSE;
	retry = 2;
	do {
		TAPEALERT(library->open_fd, library->un);
		memset(sense, 0, sizeof (sam_extended_sense_t));
		if (scsi_cmd(library->open_fd, library->un, SCMD_MOVE_MEDIUM, 0,
				transport->element, transport->element,
				ELEMENT_ADDRESS(library, slot), 0) < 0) {
			TAPEALERT_SKEY(library->open_fd, library->un);
			GENERIC_SCSI_ERROR_PROCESSING(library->un,
			    library->scsi_err_tab, 0, err,
							added_more_time, retry,
			/* DOWN_EQ code */
				    down_library(library, SAM_STATE_CHANGE);
			mutex_unlock(&library->un->io_mutex);
			return (-1);
			/* MACRO for cstyle */,
			/* ILLREQ code */
			    mutex_unlock(&library->un->io_mutex);
			return (-1);
			/* MACRO for cstyle */,
			    /* MACRO for cstyle */;
	/* MACRO for cstyle */)
		} else
			break;
	} while (--retry > 0);
	TAPEALERT(library->open_fd, library->un);

	mutex_unlock(&library->un->io_mutex);

	if (retry <= 0) {
		DevLog(DL_ERR(5212));
		memccpy(l_mess, catgets(catfd, SET, 9306,
			"transport not cleared"),
			'\0', DIS_MES_LEN);
		return (-1);
	}
	status &= ~CES_needs_audit;
	(void) CatalogSetFieldByLoc(library->un->eq, slot, 0,
				    CEF_Status, status, CES_needs_audit);

	transport->status.b.full = FALSE;
	transport->status.b.valid = FALSE;
	memccpy(l_mess, catgets(catfd, SET, 9013, "transport cleared"),
		'\0', DIS_MES_LEN);
	return (0);
}


/*
 *	clear_import - clear the import/export(mailbox)
 *
 */
void
clear_import(
		library_t *library,
		iport_state_t *iport)
{
	dev_ent_t	*un;
	int		good_addr;
	char		*l_mess;
	uint_t		element, slot;
	move_flags_t    move_flags;
	uint32_t	status;
	uchar_t		*buffer = NULL;
	storage_element_t	*storage_descrip;
	element_status_page_t	*status_page;
	storage_element_ext_t	*extension;
	struct CatalogEntry	ced;
	struct CatalogEntry	*ce = &ced;
	struct VolId    vid;

	SANITY_CHECK(library != (library_t *)0);
	SANITY_CHECK(iport != (iport_state_t *)0);
	SANITY_CHECK(library->un != (dev_ent_t *)0);

	un = library->un;
	l_mess = un->dis_mes[DIS_MES_NORM];

	if (!iport->status.b.full)
		return;

	if (un->type == DT_METD360)
		return;

	memccpy(l_mess, catgets(catfd, SET, 9014,
	    "clear import/export element"),
	    '\0', DIS_MES_LEN);
	if ((good_addr = (iport->status.b.valid &&
	    IS_STORAGE(library, iport->media_element))) != 0) {
		DevLog(DL_DETAIL(5003), iport->media_element);
		slot = SLOT_NUMBER(library, iport->media_element);
	} else {
		DevLog(DL_ERR(5004), iport->media_element);
	}

	/*
	 * If the address does not look good or the slot at that address is
	 * not inuse or its occupied or auditing, find a place to put this
	 * media.
	 */
	ce = CatalogGetCeByLoc(library->un->eq, slot, 0, &ced);

	if ((un->status.b.audit) || !good_addr ||
	    (slot == (unsigned)ROBOT_NO_SLOT) ||
	    (ce == NULL) ||
	    !(ce->CeStatus & CES_inuse) ||
	    (ce->CeStatus & CES_occupied)) {
		DevLog(DL_DETAIL(5005));

		if ((slot = FindFreeSlot(library)) == ROBOT_NO_SLOT) {
		char *lc_mess = library->un->dis_mes[DIS_MES_CRIT];

			memccpy(lc_mess, GetCustMsg(9015), '\0', DIS_MES_LEN);
			SendCustMsg(HERE, 9015);
			DevLog(DL_ERR(5136), iport->element);
			SendCustMsg(HERE, 9332, iport->element);
			return;
		}

		iport->media_element = ELEMENT_ADDRESS(library, slot);
		iport->status.b.invert = FALSE;
	}

	element = ELEMENT_ADDRESS(library, slot);
	move_flags.bits = 0;
	if (move_media(library, 0, iport->element, iport->media_element, 0,
	    move_flags)) {
		char	   *lc_mess = un->dis_mes[DIS_MES_CRIT];

		/* Unable to clear import */
		DevLog(DL_ERR(5128), iport->element);
		memccpy(lc_mess,
		    catgets(catfd, SET, 9016,
		    "can not clear import/export element, move failed."),
		    '\0', DIS_MES_LEN);
		return;
	}

	buffer = malloc_wait(library->ele_dest_len * 3, 2, 0);
	status_page = (element_status_page_t *)
	    (buffer + sizeof (element_status_data_t));
	storage_descrip = (storage_element_t *)
	    ((char *)status_page + sizeof (element_status_page_t));
	extension = (storage_element_ext_t *)
	    ((char *)storage_descrip + sizeof (storage_element_t));

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

	iport->status.b.full = FALSE;
	iport->status.b.valid = FALSE;
	DevLog(DL_DETAIL(5138), iport->element);
	memccpy(l_mess, catgets(catfd, SET, 9017,
	    "import/export element cleared"),
	    '\0', DIS_MES_LEN);
}
