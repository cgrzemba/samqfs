/*
 * common.c - code common to all robots.
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

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

#include <thread.h>
#include <synch.h>
#include <stdio.h>
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
#include "sam/defaults.h"
#include "aml/robots.h"
#include "librobots.h"
#include "aml/proto.h"

#pragma ident "$Revision: 1.13 $"


/*
 * clear_un_fields - clears specific fields in the un prior to removing media.
 * Mutex on un MUST be locked.
 */

void
clear_un_fields(dev_ent_t *un)
{
	un->space = un->capacity = 0;

	switch (un->type & DT_CLASS_MASK) {

		case DT_OPTICAL:
			un->dt.od.ptoc_fwa = 0;
			break;

		case DT_TAPE:
			un->dt.tp.position = 0;
			break;
	}
}
