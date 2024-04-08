/*
 * clear.c - routines for clearing elements.
 *
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
#include "sony.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/catalog.h"
#include "aml/catlib.h"

#pragma ident "$Revision: 1.26 $"

static char    *_SrcFile = __FILE__;

/* function prototypes */


/* globals */

extern pid_t    mypid;
extern char    *fifo_path;
extern shm_alloc_t master_shm, preview_shm;


/*
 * clear_drive - clear any media in the drive. drive->mutex held on entry.
 * Note: The drive->mutex may be released and re-acquired.
 */
int
clear_drive(drive_state_t *drive)
{
	int	sony_error;
	char	*d_mess = drive->un->dis_mes[DIS_MES_NORM];
	char	*ent_pnt = "clear_drive";
	char	*MES_9006 =
	    catgets(catfd, SET, 9006, "can not clear drive, move failed");
	char	*MES_9007 = catgets(catfd, SET, 9007, "%s(%d): Move failed.");
	dev_ent_t	*un = drive->un;

	if (un->state > DEV_OFF)
		return (0);

	if (!drive->status.b.full)
		return (0);

	/*
	 * drive_is_idle does not consider a drive less than DEV_IDLE to be
	 * idle, so it must be checked here.
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

	DTB(drive->bar_code, BARCODE_LEN);

	memmove(un->i.ViMtype, sam_mediatoa(un->type), sizeof (un->i.ViMtype));
	memmove(un->i.ViVsn, un->vsn, sizeof (un->i.ViVsn));
	un->i.ViEq = un->fseq;
	un->i.ViSlot = un->slot;
	un->i.ViPart = 0;

	(void) spin_drive(drive, SPINDOWN, NOEJECT);

	mutex_lock(&un->mutex);
	un->status.bits = (DVST_PRESENT | DVST_REQUESTED) |
	    (un->status.bits & DVST_CLEANING);
	un->status.b.opened = (un->open_count != 0);
	mutex_unlock(&un->mutex);

	memccpy(d_mess, catgets(catfd, SET, 9009, "waiting for media changer"),
	    '\0', DIS_MES_LEN);
	if (un->i.ViSlot == ROBOT_NO_SLOT) {
		if (force_media(drive->library, drive, &sony_error)) {
			/*
			 * Unable to clear drive
			 */
			memccpy(d_mess, MES_9006, '\0', DIS_MES_LEN);
			sam_syslog(LOG_ERR, MES_9007, ent_pnt, un->eq);
			clear_driver_idle(drive, drive->open_fd);
			down_drive(drive, SAM_STATE_CHANGE);
			return (-1);
		}
	} else {
		if (dismount_media(drive->library, drive, &sony_error)) {
			/*
			 * Unable to clear drive
			 */
			memccpy(d_mess, MES_9006, '\0', DIS_MES_LEN);
			sam_syslog(LOG_ERR, MES_9007, ent_pnt, un->eq);
			clear_driver_idle(drive, drive->open_fd);
			down_drive(drive, SAM_STATE_CHANGE);
			return (-1);
		}
	}

	if (un->i.ViSlot != ROBOT_NO_SLOT)
		un->i.ViFlags = VI_cart;
	if (*un->i.ViMtype != '\0')
		un->i.ViFlags |= VI_mtype;
	if (*un->i.ViVsn != '\0')
		un->i.ViFlags |= VI_vsn;
	CatalogVolumeUnloaded(&un->i, (char *)&drive->bar_code);

	mutex_lock(&un->mutex);
	memset(&un->vsn, 0, sizeof (vsn_t));
	un->slot = ROBOT_NO_SLOT;
	un->mid = ROBOT_NO_SLOT;
	un->flip_mid = ROBOT_NO_SLOT;
	un->status.bits &= ~DVST_REQUESTED;
	un->label_time = 0;
	mutex_unlock(&un->mutex);
	drive->status.b.full = FALSE;
	drive->status.b.bar_code = FALSE;
	memset((void *) &drive->bar_code, 0, BARCODE_LEN);
	*d_mess = '\0';
	clear_driver_idle(drive, drive->open_fd);
	return (0);
}
