/*
 *	clear.c - routines for clearing elements.
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

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/robots.h"
#include "sam/nl_samfs.h"
#include "stk.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/catalog.h"
#include "aml/catlib.h"

#pragma ident "$Revision: 1.26 $"

/* Using __FILE__ makes duplicate strings */
static char *_SrcFile = __FILE__;

/* Some globals */
extern pid_t mypid;
extern char *fifo_path;
extern shm_alloc_t master_shm, preview_shm;

/*
 * clear_drive - Clear any media in the drive.
 *
 * drive->mutex held on entry.
 * Note: The drive->mutex may be released and re-acquired.
 */
int
clear_drive(drive_state_t *drive)
{
	int	stk_error;
	char	*d_mess = drive->un->dis_mes[DIS_MES_NORM];
	char	*ent_pnt = "clear_drive";
	char	*MES_9006 = catgets(catfd, SET, 9006,
	    "can not clear drive, move failed");
	char	*MES_9007 = catgets(catfd, SET, 9007,
	    "%s(%d): Move failed.");
	dev_ent_t		*un = drive->un;
	struct CatalogEntry 	ced;
	struct CatalogEntry 	*ce = &ced;

	if (un->state > DEV_OFF)
		return (0);

	if (!drive->status.b.full)
		return (0);

	/*
	 * drive_is_idle does not consider a drive less than DEV_IDLE
	 * to be idle, so it must be checked here.
	 */
	mutex_lock(&un->mutex);
	un->status.bits |= (DVST_WAIT_IDLE | DVST_REQUESTED);
	if (un->state == DEV_IDLE || un->state == DEV_UNAVAIL) {
		while (un->active != 0) {
			mutex_unlock(&drive->mutex);
			mutex_unlock(&un->mutex);
			sleep(5);
			mutex_lock(&drive->mutex);
			mutex_lock(&un->mutex);
		}
	} else {
		while (!drive_is_idle(drive) &&
		    (un->state < DEV_OFF)) {

			mutex_unlock(&un->mutex);
			mutex_unlock(&drive->mutex);
			sleep(5);
			mutex_lock(&drive->mutex);
			mutex_lock(&un->mutex);
		}
	}

	un->status.bits =
	    (un->status.bits & ~DVST_WAIT_IDLE) | DVST_UNLOAD;
	mutex_unlock(&un->mutex);

	memmove(un->i.ViMtype, sam_mediatoa(un->type),
	    sizeof (un->i.ViMtype));
	memmove(un->i.ViVsn, un->vsn, sizeof (un->i.ViVsn));

	un->i.ViEq = un->fseq;
	if (un->slot == ROBOT_NO_SLOT) {
		/*
		 * At initialization time, un->slot is -1
		 * so get the slot from the Mtype.Vsn
		 */
		ce = CatalogGetCeByMedia(un->i.ViMtype,
		    un->i.ViVsn, &ced);
		if (ce != NULL) {
			un->i.ViSlot = ce->CeSlot;
		} else {
			un->i.ViSlot = un->slot;
		}
	} else {
		un->i.ViSlot = un->slot;
	}
	un->i.ViPart = 0;

	(void) spin_drive(drive, SPINDOWN, NOEJECT);
	mutex_lock(&un->mutex);
	un->status.bits = (DVST_PRESENT | DVST_REQUESTED) |
	    (un->status.bits & DVST_CLEANING);
	un->status.b.opened = (un->open_count != 0);
	mutex_unlock(&un->mutex);

	memccpy(d_mess, catgets(catfd, SET, 9009,
	    "waiting for media changer"), '\0', DIS_MES_LEN);

	if ((un->i.ViSlot == ROBOT_NO_SLOT) ||
	    (drive->library->hasam_running)) {

		if (force_media(drive->library, drive, &stk_error)) {
			/* Unable to clear drive */
			memccpy(d_mess, MES_9006, '\0', DIS_MES_LEN);
			sam_syslog(LOG_ERR, MES_9007, ent_pnt, un->eq);
			clear_driver_idle(drive, drive->open_fd);
			down_drive(drive, SAM_STATE_CHANGE);
			return (-1);
		} else {
			if (CatalogVolumeUnloaded(
			    &un->i, (char *)&drive->bar_code) != -1) {

				memset(&un->vsn, 0, sizeof (vsn_t));
				un->slot = ROBOT_NO_SLOT;
			}
		}
	} else {
		if (dismount_media(drive->library, drive, &stk_error) &&
		    stk_error != STATUS_IPC_FAILURE) {
			/* Unable to clear drive */
			memccpy(d_mess, MES_9006, '\0', DIS_MES_LEN);
			sam_syslog(LOG_ERR, MES_9007, ent_pnt, un->eq);
			clear_driver_idle(drive, drive->open_fd);
			down_drive(drive, SAM_STATE_CHANGE);
			return (-1);
		}
		/*
		 * If failure and ipc failure, look to see if the media
		 * was put away, if so, then proceed as if it
		 * worked.
		 */
		if (stk_error == STATUS_IPC_FAILURE) {
			int stk_error2;
			struct CatalogEntry ced;
			struct CatalogEntry *ce = &ced;

			un->i.ViFlags = VI_cart;
			ce = CatalogGetEntry(&un->i, &ced);

			view_media(drive->library, ce, &stk_error2);
			if (stk_error2 != STATUS_VOLUME_HOME &&
			    stk_error2 != STATUS_VOLUME_IN_TRANSIT) {

				memccpy(d_mess, MES_9006, '\0', DIS_MES_LEN);
				sam_syslog(LOG_ERR, MES_9007, ent_pnt, un->eq);
				clear_driver_idle(drive, drive->open_fd);
				down_drive(drive, SAM_STATE_CHANGE);
				return (-1);
			}
		}
		if (*un->i.ViMtype != '\0')
			un->i.ViFlags |= VI_mtype;
		if (*un->i.ViVsn != '\0')
			un->i.ViFlags |= VI_vsn;

		if (CatalogVolumeUnloaded(
		    &un->i, (char *)&drive->bar_code) != -1) {

			memset(&un->vsn, 0, sizeof (vsn_t));
			un->slot = ROBOT_NO_SLOT;
		}
	}

	mutex_lock(&un->mutex);
	un->status.bits &= ~DVST_REQUESTED;
	un->label_time = 0;
	mutex_unlock(&un->mutex);

	drive->status.b.full = FALSE;
	drive->status.b.bar_code = FALSE;
	memset((void *)&drive->bar_code, 0, BARCODE_LEN);
	*d_mess = '\0';
	clear_driver_idle(drive, drive->open_fd);
	return (0);
}
