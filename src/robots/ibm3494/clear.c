/*
 * clear.c - routines for clearing drives.
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

#pragma ident "$Revision: 1.28 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

#include <thread.h>
#include <synch.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/device.h"
#include "sam/lib.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/robots.h"
#include "ibm3494.h"
#include "aml/proto.h"

/* some globals */
extern shm_alloc_t master_shm, preview_shm;


/*
 * clear_drive - clear any media in the drive.
 *
 * drive->mutex held on entry.
 * Note: The drive->mutex may be released and re-acquired.
 */
int
clear_drive(
	drive_state_t *drive)
{
	char		*mess = drive->un->dis_mes[DIS_MES_NORM];
	char		*c_mess = drive->un->dis_mes[DIS_MES_CRIT];
	dev_ent_t	*un = drive->un;
	boolean_t	catalog_volume_unloaded;

	if (un->state > DEV_OFF)
		return (0);

	if (!drive->status.b.full)
		return (0);

	catalog_volume_unloaded = FALSE;

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
		while (!drive_is_idle(drive) && (un->state < DEV_OFF)) {
			mutex_unlock(&un->mutex);
			mutex_unlock(&drive->mutex);
			sleep(5);
			mutex_lock(&drive->mutex);
			mutex_lock(&un->mutex);
		}
	}

	un->status.bits = (un->status.bits & ~DVST_WAIT_IDLE) |
	    DVST_UNLOAD | (un->status.bits & DVST_CLEANING);
	mutex_unlock(&un->mutex);

	if (un->slot != ROBOT_NO_SLOT)
		sprintf(mess, "Unloading %s.", un->vsn);
	else
		memccpy(mess, "Unloading.", '\0', DIS_MES_LEN);

	/*
	 * Save off what information we know about this volume
	 * before we spin down the drive and clear out the un.
	 */
	memmove(un->i.ViMtype, sam_mediatoa(un->type), sizeof (un->i.ViMtype));
	memmove(un->i.ViVsn, un->vsn, sizeof (un->i.ViVsn));
	un->i.ViEq = un->fseq;
	un->i.ViSlot = un->slot;
	un->i.ViPart = 0;

	(void) spin_drive(drive, SPINDOWN, NOEJECT);

	mutex_lock(&un->mutex);
	un->status.bits &= ~DVST_UNLOAD;
	un->status.b.opened = (un->open_count != 0);
	mutex_unlock(&un->mutex);

	if (un->i.ViSlot == ROBOT_NO_SLOT) {
		memccpy(mess, "Waiting for media changer.", '\0', DIS_MES_LEN);
		if (force_media(drive->library, drive)) {
			/* Unable to clear drive */
			memccpy(c_mess, "Media changer reported error.",
			    '\0', DIS_MES_LEN);
			sam_syslog(LOG_CRIT, "Unable to clear drive %d.",
			    un->eq);
			clear_driver_idle(drive, drive->open_fd);
			down_drive(drive, SAM_STATE_CHANGE);
			return (-1);
		} else {
			if (CatalogVolumeUnloaded(&un->i,
			    (char *)&drive->bar_code) != -1) {
				memset(&un->vsn, 0, sizeof (vsn_t));
				un->slot = ROBOT_NO_SLOT;
			}
		}
	} else {
		sprintf(mess, "Waiting for media changer to dismount %s.",
		    un->i.ViVsn);
		if (dismount_media(drive->library, drive)) {
			/* Unable to clear drive */
			memccpy(c_mess, "Media changer reported error.",
			    '\0', DIS_MES_LEN);
			sam_syslog(LOG_CRIT, "Unable to clear drive %d.",
			    un->eq);
			clear_driver_idle(drive, drive->open_fd);
			down_drive(drive, SAM_STATE_CHANGE);
			return (-1);
		}

		if (*un->i.ViMtype != '\0') un->i.ViFlags |= VI_mtype;
		if (*un->i.ViVsn != '\0') un->i.ViFlags |= VI_vsn;
		CatalogVolumeUnloaded(&un->i, (char *)&drive->bar_code);
		catalog_volume_unloaded = TRUE;
	}

	mutex_lock(&un->mutex);
	if (catalog_volume_unloaded) {
		memset(&un->vsn, 0, sizeof (vsn_t));
		un->slot = ROBOT_NO_SLOT;
		un->mid = ROBOT_NO_SLOT;
		un->flip_mid = ROBOT_NO_SLOT;
	}
	un->status.bits = DVST_PRESENT | (un->status.bits & DVST_CLEANING);
	un->label_time = 0;
	mutex_unlock(&un->mutex);

	*mess = '\0';
	drive->status.b.full = FALSE;
	drive->status.b.bar_code = FALSE;
	memset((void *)&drive->bar_code, 0, BARCODE_LEN);
	clear_driver_idle(drive, drive->open_fd);
	return (0);
}
