/*
 * chk_drives.c - library routines for searching the drives for various
 * conditions.
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#pragma ident "$Revision: 1.32 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>

#include "aml/device.h"
#include "aml/dev_log.h"
#include "aml/robots.h"
#include "sam/nl_samfs.h"
#include "../generic/generic.h"
#include "aml/proto.h"

#pragma weak is_flip_requested


/*
 * find_idle_drive - find an idle drive
 *
 * returns -
 * 	pointer to drive_state_t
 *	NULL if no idle drives
 *
 * drive->mutex and drive->um->mutex are locked on return.
 *
 */
drive_state_t *
find_idle_drive(
	library_t *library)
{

	char 			*l_mess = library->un->dis_mes[DIS_MES_NORM];
	drive_state_t 	*drive, start_drive, *best_drive = NULL;

	/*
	 * Start looking for an empty drive at library->index.
	 * This is to even out the drive selection. If SAM always
	 * started the search for an empty drive at the first drive,
	 * the first few drives in the list would be chosen
	 * far more than drives at the end of the list.
	 *
	 */
	if ((drive = library->index->next) == NULL) {
		drive = library->drive;
	}

	/*
	 * First look for an empty drive. If none found,
	 * then look for an idle drive.
	 */

	/*
	 * Drives in ACSLS controlled libraries have their own search routine.
	 */
	if (library->un->equ_type == DT_STKAPI) {
		drive_state_t	*new_drive = NULL;

		new_drive = (drive_state_t *)find_empty_drive(drive);
		if (new_drive != NULL) {
			/*
			 * find_empty_drive returns with drive->mutex
			 * and drive->un->mutex locked.
			 */
			return (new_drive);
		}

	} else {
		for (;;) {
			if (!mutex_trylock(&drive->mutex)) {
				if (!drive->status.b.full &&
				    !drive->active_count &&
				    !mutex_trylock(&drive->un->mutex)) {

					if (!(drive->un->status.bits &
					    (DVST_REQUESTED|DVST_CLEANING))&&
					    (drive->un->state < DEV_IDLE)) {

						mutex_lock(&library->mutex);
						library->index = drive;
#if defined(DEBUG)
						sprintf(l_mess, "assign drive"
						    " %d to next task",
						    drive->un->eq);
#endif
						mutex_unlock(&library->mutex);
						return (drive);
					}
					mutex_unlock(&drive->un->mutex);
				}
				mutex_unlock(&drive->mutex);
			}

			if (drive == library->index)
				/* have we looked at all drives */
				break;

			if ((drive = drive->next) == NULL)
				drive = library->drive;
		}
	}


	/*
	 * We did not find an empty drive.
	 * Now look for the best drive that is not busy.
	 * Start at the first drive since we are looking for the least
	 * recently used and must search them all.
	 */
	for (drive = library->drive; drive != NULL; drive = drive->next) {
		/*
		 * If a shared drive is idle, then this library
		 * has control of it so include it in the selection list.
		 */
		if (drive->un == NULL) {
			continue;
		}

		if (drive->un->flags & DVFG_SHARED && !drive->status.b.full) {
			continue;
		}

		if (!mutex_trylock(&drive->mutex)) {
			if (drive->active_count == 0 &&
			    !mutex_trylock(&drive->un->mutex)) {

				if (!(drive->un->status.bits &
				    (DVST_REQUESTED | DVST_CLEANING)) &&
				    drive_is_idle(drive) &&
				    !is_requested(library, drive) &&
				    !(is_flip_requested == NULL ?
				    FALSE :
				    is_flip_requested(library, drive))) {

					if (best_drive) {
						if (drive->un->mtime <
						    best_drive->un->mtime) {
							/* Release old_best */
							/* mutexes */
							mutex_unlock(
							    &best_drive->
							    un->mutex);
							mutex_unlock(
							    &best_drive->mutex);
							best_drive = drive;
							continue;
							/* Still holding */
							/* drive mutexes */
						}
					} else {
						best_drive = drive;
						continue;
						/* Still holding drive */
						/* mutexes */
					}
				}
				mutex_unlock(&drive->un->mutex);
			}
			mutex_unlock(&drive->mutex);
		}
	}


	/* Do not update the index as it is just for empty drives */
#if defined(DEBUG)
	if (best_drive != NULL) {
		sprintf(l_mess, "assign drive %d to next task",
		    best_drive->un->eq);
	}
#endif
	return (best_drive);
}

/*
 *	drive_is_idle - Is the drive idle
 *
 * exit -
 *	  0 - if not idle
 *	  1 - if idle
 *		to be idle, the drive must meet the following:
 *			  active count is zero,
 *			  status is online,
 *			  cartridge in drive at least required time,
 *			  not open
 * Mutex aquired before entering.
 *
 */
int
drive_is_idle(
	drive_state_t *drive)
{
	dev_ent_t 	*un;
	int 		dummy = 0, local_open;

	un = drive->un;
	if (!drive->status.b.offline && un->active == 0 &&
	    un->open_count == 0 &&
	    (un->mtime <= time(NULL)) && (un->state < DEV_IDLE)) {
		switch (un->type & DT_CLASS_MASK) {
		case DT_OPTICAL:
			/* must have an open device for optical */
			if ((local_open = open_unit(un, un->name, 1)) < 0) {
				/* should always be able to open optical */
				DevLog(DL_SYSERR(4019), un->name);
				DownDevice(un, SAM_STATE_CHANGE);
				drive->un->status.bits = 0;
				return (0);
			}
			dummy = set_driver_idle(drive, local_open);

			if (dummy == 1) {
				/* if only one open, that's us */
				close_unit(un, &local_open);
				return (1);	/* return idle */
			}
			(void) close_unit(un, &local_open);
			if ((local_open = open_unit(un, un->name, 1)) < 0) {
				DevLog(DL_SYSERR(4020), un->name);
				DownDevice(un, SAM_STATE_CHANGE);
				drive->un->status.bits = 0;
				dummy = -1;
			} else
				dummy = set_driver_idle(drive, local_open);

			if (dummy == 1) {	/* if only one open */
				close_unit(un, &local_open);
				return (1);	/* return idle */
			}
			if (dummy >= 0)
				clear_driver_idle(drive, local_open);
			close_unit(un, &local_open);
			return (0);	/* return busy */

		case DT_TAPE:
			return (1);	/* tape is idle */

		default:
			return (0);	/* return busy */
		}
	}
	return (0);			/* retrun busy */
}


/*
 *	is_requested - is the volume mounted on a drive in the preview
 *
 *	exit -
 *		 1 - if requested
 *		 0 - if not or library in audit
 */
int
is_requested(
	library_t *library,
	drive_state_t *drive)
{
	/*
	 * Stranger media in a drive will not be marked as labeled
	 * and the mount requests won't show up in preview. Therefore,
	 * an idle drive with stranger media loaded will look as if it
	 * is not requested. This creates a race condition between
	 * migkit requesting the volume in the drive and SAM selecting
	 * the drive for unloading. So, just return "requested" for any
	 * drive with stranger media loaded.
	 */
	if (drive->un->status.bits & DVST_STRANGE) {
		return (1);
	}

	if (library->un->status.b.audit || !drive->un->status.b.ready ||
	    !drive->un->status.b.labeled)
		return (0);

	return (check_for_vsn(drive->un->vsn, drive->un->type));
}
