/*
 * dev_usage.c - Compute device usage percentage.
 *
 *	Description:
 *	    Computes usage for the specified device.
 *
 *	On entry:
 *	    dev		= Device entry.
 *
 *	Returns:
 *	    The usage percentage.  Zero if the device's capacity is not
 *	    defined.
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

#pragma ident "$Revision: 1.12 $"


#include <sys/types.h>

#include "sam/types.h"
#include "aml/device.h"


int dev_usage(
	dev_ent_t *dev)
{
	int usage;

	usage = 0;
	if (dev->state == DEV_ON && dev->status.b.ready &&
	    dev->capacity != 0)  {
		usage = (int)((double)100 *
		    (((double)dev->capacity - (double)dev->space)
		    / (double)dev->capacity) + 0.5);
	}
	return (usage);
}
