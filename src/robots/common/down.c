/*
 * down.c - routines for downing elements.
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

#pragma ident "$Revision: 1.21 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <thread.h>
#include <synch.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "sam/types.h"
#include "sam/param.h"
#include "sam/lib.h"
#include "aml/device.h"
#include "aml/dev_log.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "sam/nl_samfs.h"
#include "../generic/generic.h"
#include "aml/proto.h"
#include "drive.h"

/* some globals */
extern shm_alloc_t master_shm, preview_shm;


/*
 * down_library - shut down the library.
 */
/* LINTED argument unused in function */
void
down_library(
	library_t *library,
	int source)
{
	DownDevice(library->un, source);
}

/*
 * down_drive - shut down the use of a drive.
 *
 * entry -
 *	drive - drive_state_t
 */
void
down_drive(
	drive_state_t *drive,
	int source)
{
	if (source != USER_STATE_CHANGE) {
		mutex_lock(&drive->list_mutex);
		mutex_lock(&drive->library->list_mutex);
		drive->status.b.offline = TRUE;

		mutex_lock(&drive->un->mutex);
		DownDevice(drive->un, source);
		drive->un->status.bits = 0;
		mutex_unlock(&drive->un->mutex);

		/* Move the list to the library */
		move_list(drive, drive->library);
		mutex_unlock(&drive->library->list_mutex);
		mutex_unlock(&drive->list_mutex);
	} else {
		sam_syslog(LOG_WARNING, "Request to down device from user :"
		    " Not supported.");
	}
}

/*
 * off_drive - shut off the use of a drive.
 *
 * entry -
 *	drive - drive_state_t
 */
void
off_drive(
	drive_state_t *drive,
	int source)
{
	mutex_lock(&drive->list_mutex);
	mutex_lock(&drive->library->list_mutex);
	drive->status.b.offline = TRUE;

	mutex_lock(&drive->un->mutex);
	OffDevice(drive->un, source);
	drive->un->status.bits = 0;
	mutex_unlock(&drive->un->mutex);

	/* Move the list to the library */
	move_list(drive, drive->library);
	mutex_unlock(&drive->library->list_mutex);
	mutex_unlock(&drive->list_mutex);
}
