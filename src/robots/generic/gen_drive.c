/*
 * gen_drive.c - generic specific drive support
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

#pragma ident "$Revision: 1.58 $"

/* Using __FILE__ makes duplicate strings */
static char    *_SrcFile = __FILE__;

#include <stdio.h>
#include <synch.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/param.h"
#include "sam/lib.h"
#include "aml/device.h"
#include "aml/dev_log.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/odlabels.h"
#include "aml/tplabels.h"
#include "aml/historian.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "sam/defaults.h"
#include "generic.h"

#if SAM_OPEN_SOURCE
#include "derrno.h"
#endif

#include "aml/trace.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"
#include "../common/drive.h"
#include "driver/samst_def.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/tapes.h"

/* some globals */

extern pid_t    mypid;
extern shm_alloc_t master_shm, preview_shm;
extern int
generic_get_media(library_t *library, drive_state_t *drive,
		robo_event_t *event, struct CatalogEntry *ce);

/* local function prototypes */
void	    clean_3570(drive_state_t *, robo_event_t *, struct CatalogEntry *);

/*
 * audit - start auditing
 *
 *
 */
void
audit(
	drive_state_t *drive,	/* drive state pointer */
	const uint_t slot,		/* slot to audit */
	const int audit_eod)
{				/* flag to find eod during audit */
	int		part, err;
	uint_t	   myslot = 0;
	dev_ent_t	*un;
	sam_defaults_t *defaults;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	int		skip_audit_eod = 0;

	defaults = GetDefaults();

	SANITY_CHECK(drive != (drive_state_t *)0);
	SANITY_CHECK(drive->library != (library_t *)0);
	SANITY_CHECK(drive->library->un != (dev_ent_t *)0);
	SANITY_CHECK(drive->library->un != drive->un);
	un = drive->un;

	if ((slot == ROBOT_NO_SLOT_L) &&
	    IS_GENERIC_API(drive->library->un->type)) {
		DevLog(DL_ERR(6004));
		return;
	}
	mutex_lock(&drive->mutex);

	if (drive->status.b.full) {
		mutex_lock(&un->mutex);
		un->status.b.requested = TRUE;
		mutex_unlock(&un->mutex);
		if (clear_drive(drive)) {
			mutex_lock(&un->mutex);
			un->status.b.requested = TRUE;
			mutex_unlock(&un->mutex);
			mutex_unlock(&drive->mutex);
			return;
		}
		if (drive->open_fd >= 0) {
			mutex_lock(&un->mutex);
			close_unit(un, &drive->open_fd);
			DEC_OPEN(un);
			mutex_unlock(&un->mutex);
		}
	}
	mutex_unlock(&drive->mutex);

	mutex_lock(&un->mutex);
	un->status.b.requested = TRUE;
	un->status.b.labeled = FALSE;
	un->status.b.ready = FALSE;
	mutex_unlock(&un->mutex);

	if (slot == ROBOT_NO_SLOT_L) {
		mutex_lock(&drive->library->mutex);
		drive->library->countdown--;
		drive->library->drives_auditing++;
		mutex_unlock(&drive->library->mutex);

		/*
		 * ok not to lock here wait for all drives to clear
		 */
		while (drive->library->countdown > 0)
			sleep(4);
	}
	for (;;) {

		mutex_lock(&drive->mutex);
		if (slot == ROBOT_NO_SLOT_L) {
			/* get the next slot number (s) */
			mutex_lock(&drive->library->mutex);
			myslot = drive->library->audit_index;
			if (myslot <= drive->library->range.storage_count) {
				drive->library->audit_index++;
				mutex_unlock(&drive->library->mutex);
			} else {
				/* No more slots to audit */
				mutex_unlock(&drive->library->mutex);
				mutex_lock(&drive->library->un->mutex);
				drive->library->un->status.b.mounted = TRUE;
				drive->library->un->status.b.audit = FALSE;
				drive->library->un->status.b.ready = TRUE;
				mutex_unlock(&drive->library->un->mutex);
				if (drive->status.b.full) {
					clear_drive(drive);
					if (drive->open_fd >= 0)
						mutex_lock(&un->mutex);
					close_unit(un, &drive->open_fd);
					mutex_unlock(&un->mutex);
				}
				mutex_lock(&un->mutex);
				un->status.b.requested = FALSE;
				mutex_unlock(&un->mutex);
				mutex_unlock(&drive->mutex);
				return;
			}
		} else {
			/* get specific slot */
			myslot = slot;
		}

		/*
		 * Should we audit this media? (is occupied, not cleaning and
		 * is a sam tape)
		 */
		if (drive->library->status.b.two_sided) {
			part = 1;
		} else {
			part = 0;
		}
		ce = CatalogGetCeByLoc(drive->library->un->eq,
			    myslot, part, &ced);
		if (ce == NULL ||
		    (!(ce->CeStatus & CES_occupied)) ||
		    (ce->CeStatus & CES_cleaning) ||
		    (ce->CeStatus & CES_non_sam)) {

			mutex_unlock(&drive->mutex);
			if (slot != ROBOT_NO_SLOT_L) {	/* only one slot */
				mutex_lock(&un->mutex);
				un->status.b.requested = FALSE;
				mutex_unlock(&un->mutex);
				return;
			}
			continue;
		}
		/*
		 * The following lines of code get a tape mounted, or if
		 * two-sided media, mounts the "A" side.
		 */
		err = get_media(drive->library, drive, NULL, ce);

		if (err) {
			mutex_lock(&un->mutex);
			un->status.b.requested = FALSE;
			DEC_ACTIVE(un);
			mutex_unlock(&un->mutex);
			mutex_unlock(&drive->mutex);
			return;
		}
		mutex_lock(&un->mutex);
		un->status.b.scanning = TRUE;
		mutex_unlock(&un->mutex);
		if (spin_drive(drive, SPINUP, NOEJECT)) {

			mutex_lock(&drive->un->mutex);
			drive->un->status.b.scanning &= ~DVST_SCANNING;
			drive->un->status.bits &= ~DVST_REQUESTED;
			mutex_unlock(&drive->un->mutex);

			if (un->state > DEV_ON) {
				clear_drive(drive);
				mutex_lock(&un->mutex);
				clear_driver_idle(drive, drive->open_fd);
				DEC_ACTIVE(un);
				close_unit(un, &drive->open_fd);
				mutex_unlock(&un->mutex);
				mutex_unlock(&drive->mutex);
			} else {
				mutex_lock(&un->mutex);
				clear_driver_idle(drive, drive->open_fd);
				DEC_ACTIVE(un);
				close_unit(un, &drive->open_fd);
				mutex_unlock(&un->mutex);
				mutex_unlock(&drive->mutex);
			}
			SendCustMsg(HERE, 9348);
			DevLog(DL_ERR(5218));
			return;
		}
		un->status.bits |= DVST_AUDIT;

		un->mid = ce->CeMid;
		un->status.b.labeled = FALSE;
		un->i.ViPart = ce->CePart;
		scan_a_device(un, drive->open_fd);
		if (drive->status.b.bar_code) {
			(void) CatalogSetStringByLoc(drive->library->un->eq,
			ce->CeSlot, ce->CePart,
			CEF_BarCode, (char *)drive->bar_code);
		}
		/*
		 * If the cleaning light came on while scanning, leave the
		 * audit bit set and unload the drive.
		 */
		if (un->status.bits & DVST_CLEANING) {
			mutex_lock(&un->mutex);
			un->mtime = 0;
			DEC_ACTIVE(un);
			close_unit(un, &drive->open_fd);
			un->status.b.requested = FALSE;
			mutex_unlock(&un->mutex);
			clear_drive(drive);
			mutex_unlock(&drive->mutex);
			return;
		} else {
			un->status.bits &= ~DVST_AUDIT;
		}

		mutex_lock(&un->mutex);
		/*
		 * This next check keeps us from auditing media that is not
		 * really labeled (label lie). I'm not sure why the un->mutex
		 * is held for this.
		 */
		if (!un->status.b.labeled &&
		    (ce->CeStatus & CES_bar_code) &&
		    (defaults->flags & DF_LABEL_BARCODE)) {
			int tmp;

			if (IS_TAPE(un)) {
				tmp = LEN_TAPE_VSN;
			} else {
				tmp = LEN_OPTIC_VSN;
			}
			vsn_from_barcode(un->vsn, ce->CeBarCode, defaults, tmp);
			un->status.b.labeled = TRUE;
			un->space = un->capacity;
			skip_audit_eod = 1;
		}
		if (IS_TAPE(un)) {
			if (un->status.b.labeled &&
			    audit_eod && !skip_audit_eod) {
				DevLog(DL_DETAIL(5074), un->vsn);
				mutex_unlock(&un->mutex);
				mutex_lock(&un->io_mutex);
				tape_append(drive->open_fd, un, NULL);
				mutex_unlock(&un->io_mutex);
				mutex_lock(&un->mutex);
			} else {
				if (!un->status.b.labeled) {
					un->space = un->capacity;
				} else {
					un->space = ce->CeSpace;
				}
			}
		}
		DevLog(DL_DETAIL(3215), un->capacity,  un->space);
		UpdateCatalog(un, 0, CatalogVolumeLoaded);

		/*
		 * Now do the "B" side if this is optical media.
		 * flip_and_scan calls CatalogVolumeLoaded so it is not done
		 * here.
		 */
		if (drive->library->status.b.two_sided && (ce->CePart == 1)) {
			mutex_unlock(&un->mutex);
			if (flip_and_scan(ce->CePart, drive)) {
				clear_drive(drive);
				mutex_lock(&un->mutex);
				un->status.b.requested = FALSE;
				clear_driver_idle(drive, drive->open_fd);
				close_unit(un, &drive->open_fd);
				DEC_ACTIVE(un);
				mutex_unlock(&un->mutex);
				mutex_unlock(&drive->mutex);
				return;
			}
			mutex_unlock(&drive->mutex);
		} else {
			mutex_unlock(&un->mutex);
			mutex_unlock(&drive->mutex);
		}

		mutex_lock(&un->mutex);
		close_unit(un, &drive->open_fd);
		DEC_ACTIVE(un);
		un->status.b.requested = TRUE;
		mutex_unlock(&un->mutex);

		if (slot != ROBOT_NO_SLOT_L) {	/* only one slot */
			mutex_lock(&un->mutex);
			un->status.b.requested = FALSE;
			mutex_unlock(&un->mutex);
			mutex_lock(&drive->library->un->mutex);
			drive->library->un->status.b.mounted = TRUE;
			mutex_unlock(&drive->library->un->mutex);
			return;
		}
	}
}


/*
 * search_drives - search drives looking for a drive with a storage element
 *
 *
 * exit -
 *   drive_state_t *to drive with the element(mutex locked)
 *   drive_state_t *NULL if not found.
 *
 */
drive_state_t  *
search_drives(
		library_t *library,
		uint_t element)
{
	drive_state_t  *drive;
	dev_ent_t	*un;
	int		valid;
	uint_t	   check_element;

	SANITY_CHECK(library != (library_t *)0);

	for (drive = library->drive; drive != NULL; drive = drive->next) {
		if (drive->un == NULL)
			continue;

		un = drive->un;

		check_element = drive->media_element;
		valid = drive->status.b.valid;
		/*
		 * source address valid and matches element
		 */
		if (valid && (check_element == element)) {
			mutex_lock(&drive->mutex);
			mutex_lock(&un->mutex);
			/* source address valid and matched */
			if (valid && (check_element == element))
				return (drive);
			mutex_unlock(&un->mutex);
			mutex_unlock(&drive->mutex);
		}
	}
	return (NULL);
}


/*
 *  is_flip_requested - is the flip side of the volume requested
 *
 *  exit -
 *	1 - if requested
 *	0 - if not or library in audit
 */
int
is_flip_requested(
		library_t *library,
		drive_state_t *drive)
{
	int		slot;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;

	SANITY_CHECK(library != (library_t *)0);

	if (!library->status.b.two_sided ||	/* not two sided */
	    library->un->status.b.audit ||	/* if audit, don't even look */
	    !drive->status.b.valid)	/* can't tell, just say no */
		return (0);

	slot = SLOT_NUMBER(library, drive->media_element);

	if (drive->un->i.ViPart == 1) {
		ce = CatalogGetCeByLoc(library->un->eq, slot, 2, &ced);
	} else {
		ce = CatalogGetCeByLoc(library->un->eq, slot, 1, &ced);
	}

	if (ce == NULL)
		return (0);

	if (ce->CeStatus & CES_labeled) {
		/*
		 * Checks if this vsn is already in the preview queue.
		 */
		return (check_for_vsn(ce->CeVsn, sam_atomedia(ce->CeMtype)));
	}
	return (0);
}


/*
 * clean - clean the drive.
 *
 */
void
clean(
	drive_state_t *drive,
	robo_event_t *event)
{
	dev_ent_t	*un;
	int		err, retry;
	uint32_t	access_count, status = 0;
	char	   *d_mess;
	char	   *l_mess;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	library_t	*library;
	move_flags_t    move_flags;

	SANITY_CHECK(drive != (drive_state_t *)0);
	un = drive->un;
	SANITY_CHECK(un != (dev_ent_t *)0);
	library = (library_t *)drive->library;
	SANITY_CHECK(library != (library_t *)0);
	d_mess = drive->un->dis_mes[DIS_MES_NORM];
	l_mess = library->un->dis_mes[DIS_MES_NORM];
	mutex_lock(&drive->mutex);
	if (clear_drive(drive)) {
		mutex_lock(&drive->un->mutex);
		drive->un->status.bits |= DVST_CLEANING;
		mutex_unlock(&drive->un->mutex);
		drive->status.b.cln_inprog = FALSE;
		mutex_unlock(&drive->mutex);
		disp_of_event(library, event, ENOENT);
		return;
	}
	mutex_lock(&drive->un->mutex);
	drive->un->status.bits |= (DVST_REQUESTED | DVST_CLEANING);
	if (drive->un->open_count) {
		clear_driver_idle(drive, drive->open_fd);
		close_unit(drive->un, &drive->open_fd);
		DEC_OPEN(un);
	}
	mutex_unlock(&drive->un->mutex);
	mutex_unlock(&drive->mutex);

	DevLog(DL_ALL(5075));

	memccpy(d_mess, catgets(catfd, SET, 9025, "needs cleaning"),
	    '\0', DIS_MES_LEN);

	ce = CatalogGetCleaningVolume(library->un->eq, &ced);

	if (ce == NULL) {
		memccpy(l_mess,
		    catgets(catfd, SET, 9026,
	    "no cleaning cartridge available"),
		    '\0', DIS_MES_LEN);
		DevLog(DL_ERR(5141));
		SendCustMsg(HERE, 9347);
		mutex_lock(&drive->mutex);
		drive->status.b.cln_inprog = FALSE;
		down_drive(drive, SAM_STATE_CHANGE);
		mutex_lock(&drive->un->mutex);
		drive->un->status.bits &= ~DVST_REQUESTED;
		mutex_unlock(&drive->un->mutex);
		mutex_unlock(&drive->mutex);
		disp_of_event(library, event, EAGAIN);
		return;
	} else {

		status &= ~CES_occupied;
		(void) CatalogSetFieldByLoc(ce->CeEq, ce->CeSlot, ce->CePart,
		    CEF_Status, status, CES_occupied);


	}


	if (library->un->equ_type == DT_3570C) {
		clean_3570(drive, event, ce);
		return;
	}
	mutex_lock(&drive->mutex);

	if (IS_GENERIC_API(library->un->type)) {
		int		local_retry, d_errno, last_derrno = -1;
		api_errs_t	ret;
		char	   *tag = "load on clean";

		local_retry = 3;
		ret = API_ERR_TR;

		while (local_retry > 0) {
			if (aci_load_media(library, drive, ce, &d_errno) == 0)
				break;
			else {
				/* Error return on api call */
				if (d_errno == 0) {
					/*
					 * if call did not happen - error
					 * return but no error
					 */
					local_retry = -1;
					d_errno = EAMUCOMM;
				} else if ((last_derrno == -1) ||
				    (last_derrno != d_errno)) {
					/* Save error if repeated */
					last_derrno = d_errno;
					if (api_valid_error(library->un->type,
					    d_errno, library->un)) {
				/* Indentation for cstyle */
				if (library->un->slot != ROBOT_NO_SLOT_L) {
					DevLog(DL_DEBUG(6001),
					    library->un->slot, tag, d_errno,
					    d_errno, api_return_message(
					    library->un->type, d_errno));
				} else {
					DevLog(DL_DEBUG(6043), tag, d_errno,
					    d_errno, api_return_message(
					    library->un->type, d_errno));
				}

				local_retry = api_return_retry(
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
			DevLog(DL_ERR(6036), ce->CeBarCode);
			memccpy(drive->un->dis_mes[DIS_MES_CRIT],
			    catgets(catfd, SET, 9029,
			"unable to load cleaning cartridge, move failed"),
			    '\0', DIS_MES_LEN);

			if (local_retry == -1) {
				/* The call didn't happen */
				DevLog(DL_ERR(6040), tag);
			} else if (local_retry == 0) {
				/* retries exceeded */
				DevLog(DL_ERR(6039), tag);
			} else {
				if (api_valid_error(drive->library->un->type,
				    d_errno, drive->library->un)) {
					if (drive->library->un->slot !=
					    ROBOT_NO_SLOT_L) {
						DevLog(DL_ERR(6001),
						    drive->library->un->slot,
						    tag, d_errno, d_errno,
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

			if (ret == API_ERR_DD)
				down_drive(drive, SAM_STATE_CHANGE);
			else if (ret == API_ERR_DM)
				set_bad_media(un);
			else if (ret == API_ERR_DL)
				down_library(library, SAM_STATE_CHANGE);

			drive->status.b.cln_inprog = FALSE;
			mutex_lock(&drive->un->mutex);
			drive->un->status.bits &= ~(DVST_CLEANING |
			    DVST_REQUESTED);
			mutex_unlock(&drive->un->mutex);
			mutex_unlock(&drive->mutex);
			disp_of_event(library, event, EIO);
			return;
		}
	} else {
		move_flags.bits = 0;
		memccpy(d_mess,
		    catgets(catfd, SET, 9009, "waiting for media changer"),
		    '\0', DIS_MES_LEN);
		/*
		 * SPECTRA LOGIC NOTE:  In 3.3.1, the invert argument on this
		 * move was set to one.  This apparently was done for the
		 * spectra-logic robot which overloaded the invert argument
		 * to be the clean argument (See scsi_command case
		 * SCMD_MOVE_MEDIUM).  Unfortunately, the Qualstar robot,
		 * which is mapped to the spectra-logic, implements the
		 * mailbox control for the same bit that spectra-logic uses
		 * as clean.  Confused? Yep.
		 *
		 * Now to add to the confusion. Somewhere in the catalog
		 * rewrite, for reasons lost in the mists of time, the invert
		 * argument was set to zero.  This is inadverentately half
		 * the fix for snap 4966.  It simplifies things, ignoring any
		 * "special" cleaning logic and treating the cleaning tape as
		 * an ordinary load/unload. It seems to work.
		 *
		 * See #ifdef UNKNOWN_SPECTRA_LOGIC below for the other half of
		 * the fix.
		 */
		if (move_media(library, 0, ELEMENT_ADDRESS(library, ce->CeSlot),
		    drive->element, 0, move_flags)) {
			memccpy(drive->un->dis_mes[DIS_MES_CRIT],
			    catgets(catfd, SET, 9029,
			"unable to load cleaning cartridge, move failed"),
			    '\0', DIS_MES_LEN);

			DevLog(DL_ERR(5143));
			down_drive(drive, SAM_STATE_CHANGE);
			drive->status.b.cln_inprog = FALSE;
			mutex_unlock(&drive->mutex);
			disp_of_event(library, event, EIO);
			return;
		}
	}

	mutex_unlock(&drive->mutex);

	/*
	 * Log successful mount of cleaning tape
	 */
	DevLog(DL_ALL(10042), drive->un->eq);
	tapeclean_media(drive->un);

	/*
	 * move_media does not set up the un, so UpdateCatalog can't be
	 * called from here. Using generic_get_media instead of move_media
	 * leaves the drive hung up.
	 */
	status &= ~CES_occupied;
	(void) CatalogSetFieldByLoc(ce->CeEq, ce->CeSlot, ce->CePart,
	    CEF_Status, status, CES_occupied);
	access_count = ce->CeAccess;
	access_count--;
	(void) CatalogSetFieldByLoc(ce->CeEq, ce->CeSlot, ce->CePart,
	    CEF_Access, access_count, 0);
	(void) CatalogSetFieldByLoc(ce->CeEq, ce->CeSlot, ce->CePart,
	    CEF_MountTime, time(NULL), 0);

	retry = 7;
	err = 0;

	if (IS_GENERIC_API(library->un->equ_type)) {
		char	   *tag = "unload for clean";
		int		local_retry, d_errno, last_derrno;
		api_errs_t	ret;
		do {
			memccpy(d_mess,
			    catgets(catfd, SET, 9030,
			    "waiting for cleaning cycle"),
			    '\0', DIS_MES_LEN);
			sleep(3 * 60);	/* wait 3 minutes */
			tapeclean_media(drive->un);
			mutex_lock(&drive->mutex);
			memccpy(d_mess,
			    catgets(catfd, SET, 9031,
			    "attempt to unload cleaning cartridge"),
			    '\0', DIS_MES_LEN);

			local_retry = 3;
			ret = API_ERR_TR;
			last_derrno = -1;

			while (local_retry > 0) {
				/*
				 * vsn is not set, use aci_force_media()
				 * instead of aci_dismount_media()
				 */
				if (aci_force_media(library, drive,
				    &d_errno) == 0)
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
						if (api_valid_error(
						    drive->library->un->type,
						    d_errno,
						    drive->library->un)) {

						if (drive->library->un->slot !=
						    ROBOT_NO_SLOT_L) {
						DevLog(DL_DEBUG(6001),
						    drive->library->un->slot,
						    tag, d_errno, d_errno,
						    api_return_message(
						    library->un->type,
						    d_errno));
							} else {
						DevLog(DL_DEBUG(6043),
						    tag, d_errno, d_errno,
						    api_return_message(
						    library->un->type,
						    d_errno));
						}

						local_retry = api_return_retry(
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
				DevLog(DL_ERR(6033), ce->CeBarCode);

				if (local_retry == -1) {
					/* The call didn't happen */
					DevLog(DL_ERR(6040), tag);
				} else if (local_retry == 0) {
					/* retries exceeded */
					DevLog(DL_ERR(6039), tag);
				} else {
					if (api_valid_error(
					    drive->library->un->type,
					    d_errno, drive->library->un)) {
						if (drive->library->un->slot !=
						    ROBOT_NO_SLOT_L) {
						DevLog(DL_ERR(6001),
						    drive->library->un->slot,
						    tag, d_errno, d_errno,
						    api_return_message(
						    library->un->type,
						    d_errno));
						} else {
						DevLog(DL_ERR(6043),
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
			}
			mutex_unlock(&drive->mutex);
		} while (err != 0 && retry-- != 0);
	} else {
		/*
		 * SPECTRA LOGIC NOTE:  Due to the removal of "special"
		 * cleaning code, we must unload the cleaning tape for all
		 * robot types.
		 *
		 * See the previous SPECTRA LOGIC NOTE:
		 */
#ifdef UNKNOWN_SPECTRA_LOGIC
		if (library->un->equ_type != DT_SPECLOG)
#endif
			do {
				memccpy(d_mess,
				    catgets(catfd, SET, 9030,
				    "wait for cleaning cycle"),
				    '\0', DIS_MES_LEN);
				sleep(3 * 60);	/* wait 3 minutes */
				tapeclean_media(drive->un);
				mutex_lock(&drive->mutex);
				DevLog(DL_DETAIL(5077));
				memccpy(d_mess,
				    catgets(catfd, SET, 9031,
				    "attempt to unload cleaning cartridge"),
				    '\0', DIS_MES_LEN);
				err = move_media(library, 0, drive->element,
				    ELEMENT_ADDRESS(library, ce->CeSlot),
				    0, move_flags);
				if (err) {
					DevLog(DL_ERR(5078), retry);
				} else {
					DevLog(DL_DETAIL(5079));
				}
				mutex_unlock(&drive->mutex);
			} while (err != 0 && retry-- != 0);
	}

	tapeclean_media(drive->un);

	if (err != 0) {
		DevLog(DL_ERR(5080));
		memccpy(drive->un->dis_mes[DIS_MES_CRIT],
		    catgets(catfd, SET, 9032,
		    "unable to unload cleaning cartridge"),
		    '\0', DIS_MES_LEN);
		mutex_lock(&drive->mutex);
		drive->status.b.cln_inprog = FALSE;
		down_drive(drive, SAM_STATE_CHANGE);
		mutex_unlock(&drive->mutex);
		disp_of_event(library, event, EIO);
		return;
	}
	status = CES_occupied;
	if (drive->un->status.b.bad_media) {
		/* cleaning media marked in catalog as bad */
		status |= CES_bad_media;

		/* reset bad media flag */
		drive->un->status.b.bad_media = 0;
	}
	(void) CatalogSetFieldByLoc(library->un->eq, ce->CeSlot, 0,
	    CEF_Status, status, 0);

	DevLog(DL_ALL(5334), access_count);

	if ((status & CES_bad_media) == 0) {
		memccpy(d_mess,
		    catgets(catfd, SET, 9034, "drive has been cleaned"),
		    '\0', DIS_MES_LEN);
	}
	mutex_lock(&drive->mutex);
	drive->status.b.cln_inprog = FALSE;
	mutex_lock(&drive->un->mutex);
	if ((status & CES_bad_media) == 0) {
		/* drive was cleaned */
		drive->un->status.bits &= ~(DVST_CLEANING | DVST_REQUESTED);
	} else {
		/* drive was not cleaned, it needs to still be cleaned */
		drive->un->status.bits &= ~(DVST_REQUESTED);
	}
	mutex_unlock(&drive->un->mutex);
	mutex_unlock(&drive->mutex);
	if (ce->CeAccess == 0 || (status & CES_bad_media)) {
		char	   *MES_9035 = catgets(catfd, SET, 9035,
		    "cleaning cartridge in slot %d has expired");

		char *mess = (char *)malloc_wait(
		    strlen(MES_9035) + 15, 2, 0);
		sprintf(mess, MES_9035, ce->CeSlot);
		memccpy(l_mess, mess, '\0', DIS_MES_LEN);
		free(mess);
		switch (library->un->type) {
		case DT_METD28:
		case DT_DLT2700:
		case DT_GRAUACI:
			DevLog(DL_ERR(5144), ce->CeSlot);
			break;

		default:
			schedule_export(library, ce->CeSlot);
			DevLog(DL_ERR(5145), ce->CeSlot);
			break;
		}
	} else if (tapeclean_drive(drive->un)) {
		memccpy(l_mess,
		    catgets(catfd, SET, 2983, "clean failed"),
		    '\0', DIS_MES_LEN);
		DevLog(DL_ERR(5364));
		mutex_lock(&drive->mutex);
		down_drive(drive, SAM_STATE_CHANGE);
		mutex_unlock(&drive->mutex);
		disp_of_event(library, event, EIO);
		return;
	}
	disp_of_event(library, event, 0);
}

/*
 * clean_3570 - attempt to load cleaning tape into 3570.
 */
void
clean_3570(
	drive_state_t *drive,
	robo_event_t *event,
	struct CatalogEntry *ce)
{
	int		retry;
	char	   *dev_name;
	char	   *d_mess = drive->un->dis_mes[DIS_MES_NORM];
	dev_ent_t	*un = drive->un;
	library_t	*library = drive->library;
	move_flags_t    move_flags;

	mutex_lock(&drive->mutex);
	move_flags.bits = 0;
	/*
	 * The 3570 does not return from the move until the cleaning cycle
	 * has completed.
	 */
	memccpy(d_mess, catgets(catfd, SET, 9030, "wait for cleaning cycle"),
	    '\0', DIS_MES_LEN);
	if (generic_get_media(library, drive, event, ce)) {
		memccpy(drive->un->dis_mes[DIS_MES_CRIT],
		    catgets(catfd, SET, 9029,
		    "unable to load cleaning cartridge, move failed"),
		    '\0', DIS_MES_LEN);

		DevLog(DL_ERR(5145), ce->CeSlot);
		down_drive(drive, SAM_STATE_CHANGE);
		drive->status.b.cln_inprog = FALSE;
		mutex_unlock(&drive->mutex);
		disp_of_event(library, event, EIO);
		return;
	}
	mutex_unlock(&drive->mutex);
	sleep(4);
	dev_name = samst_devname(un);
	mutex_lock(&un->mutex);
	drive->open_fd = open_unit(un, dev_name, 10);
	mutex_unlock(&un->mutex);
	free(dev_name);

	un->i.ViEq = un->fseq;
	un->i.ViSlot = un->slot;
	un->i.ViPart = 0;
	un->i.ViFlags = VI_cart;
	UpdateCatalog(drive->un, 0, CatalogVolumeLoaded);

	/* Wait for cleaning to finish */

	retry = 60;
	while (retry--) {
		sam_extended_sense_t *sense = (sam_extended_sense_t *)
		    SHM_REF_ADDR(un->sense);

		mutex_lock(&un->io_mutex);
		memset(sense, 0, sizeof (sam_extended_sense_t));
		if (scsi_cmd(drive->open_fd, un, SCMD_TEST_UNIT_READY, 20) ||
		    sense->es_key != 0) {
			/* If cleaning in progress */
			if (sense->es_key == 0x02 &&
			    sense->es_add_code == 0x30 &&
			    sense->es_qual_code == 0x03) {
				mutex_unlock(&un->io_mutex);
				sleep(30);
				continue;
			}
			if (sense->es_key == 0x06 &&
			    sense->es_add_code == 0x82 &&
			    sense->es_qual_code == 0x83)
				break;

			mutex_unlock(&un->io_mutex);
			sprintf(d_mess, "sense %x, %x, %x", sense->es_key,
			    sense->es_add_code, sense->es_qual_code);
			sleep(10);
		}
	}
	if (retry <= 0)
		DevLog(DL_ERR(5216));

	memccpy(d_mess, catgets(catfd, SET, 9034, "drive has been cleaned"),
	    '\0', DIS_MES_LEN);
	mutex_unlock(&un->io_mutex);
	mutex_lock(&un->mutex);
	close_unit(un, &drive->open_fd);
	mutex_unlock(&un->mutex);
	mutex_lock(&drive->mutex);
	move_flags.bits = 0;
	memccpy(d_mess,
	    catgets(catfd, SET, 9009, "waiting for media changer"),
	    '\0', DIS_MES_LEN);
	if (move_media(library, 0, drive->element, 0xff, 1, move_flags)) {
		memccpy(drive->un->dis_mes[DIS_MES_CRIT],
		    catgets(catfd, SET, 9032,
		    "unable to unload cleaning cartridge"),
		    '\0', DIS_MES_LEN);
		DevLog(DL_ERR(5147));
		drive->status.b.cln_inprog = FALSE;
		down_drive(drive, SAM_STATE_CHANGE);
		mutex_unlock(&drive->mutex);
		disp_of_event(library, event, EIO);
		return;
	}
	if (CatalogVolumeUnloaded(&un->i, "") == -1) {
		DevLog(DL_SYSERR(5336), ce->CeSlot);
	}
	drive->status.b.cln_inprog = FALSE;
	mutex_lock(&drive->un->mutex);
	drive->un->status.bits &= ~(DVST_CLEANING | DVST_REQUESTED);
	un->label_time = 0;
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
	 * At this time this is a place holder only until passthru is
	 * supported.
	 */
	return (TRUE);
}

drive_state_t  *
find_empty_drive(
		drive_state_t *drive)
{
	/*
	 * Place holder
	 */
	return (NULL);
}
