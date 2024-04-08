/*
 *	scsi_support - support routines for various scsi things.
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
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/mman.h>
#include <sys/scsi/generic/sense.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "sam/nl_samfs.h"
#include "../generic/generic.h"
#include "aml/proto.h"

#pragma ident "$Revision: 1.17 $"

/*	Globals */
extern shm_alloc_t master_shm, preview_shm;


/*
 * spin_drive - spinup/down a drive and handle any errors on the operation.
 *
 *	entry -
 *		 drive->un->status.b.requested set
 *
 */
int
spin_drive(
	drive_state_t *drive,
	const int updown,		/* SPINUP or SPINDOWN */
	const int eject) 		/* EJECT_MEDIA or NOEJECT */
{
	int ret;

	ret = spin_unit(drive->un, drive->samst_name, &drive->open_fd, updown,
	    eject);

	if (ret == (int)DOWN_EQU)
		down_drive(drive, SAM_STATE_CHANGE);
	else if (ret == BAD_MEDIA)
		set_bad_media(drive->un);
	return (ret);
}
