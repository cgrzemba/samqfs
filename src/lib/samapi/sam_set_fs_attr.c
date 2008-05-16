/*
 * sam_set_fs_attr.c - set file set attribute values
 *
 *	sam_set_fs_thresh() sets the maximum and minimum thresholds for release
 *	sam_set_fs_contig() sets the maximum contiguous space in the file set
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

#pragma ident "$Revision: 1.19 $"


#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <stdio.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "sam/lib.h"
#include "aml/fifo.h"
#include "aml/proto.h"
#include "aml/sam_utils.h"
#include "aml/samapi.h"
#include "pub/sam_errno.h"


/*
 *	sam_set_fs_contig() - API function to set the maximum contiguous space
 *
 *	Input parameters --
 *		eq_ord		Equipment ordinal
 *		max_contig	Maximum contiguous blocks in file set
 *		wait_response	Wait for command response if nonzero
 *				-1	Wait forever
 *				> zero	Wait this many seconds for response
 *
 *	Output parameters --
 *		errno		Set to error number if error encountered
 *
 *	Return value --
 *		0		If successful in set_fs_threshing the media
 *		-1		If error encountered,
 *				'errno' set to error number
 */

int
sam_set_fs_contig(
	ushort_t eq_ord,	/* Equipment ordinal */
	int type,		/* CT_readahead or CT_writebehind */
	int contig,		/* Maximum readahead/writebehind kilobytes */
	/* LINTED argument unused in function */
	int wait_response)	/* Nonzero value to wait for response */
{
	char *fifo_path;	/* Path to FIFO pipe, from shared memory */

	dev_ent_t *device;
	operator_t operator;	/* Data on operator */
	char value[20];


	/*
	 *	Get device entry and FIFO path from master shared memory segment
	 */

	if (sam_get_dev(eq_ord, &device, &fifo_path, &operator) < 0) {
		return (-1);
	}

	/*
	 *	If operator does not have 'root' authority, return with error
	 */

	if (!SAM_ROOT_LEVEL(operator)) {
		errno = ER_OPERATOR_NOT_PRIV;
		return (-1);
	}

	/*
	 *	If device is not a disk family set device, return with error
	 */

	if (!IS_FS(device)) {
		errno = ER_DEVICE_NOT_CORRECT_TYPE;
		return (-1);
	}
	sprintf(value, "%d", contig);
	if (type == CNTG_readahead) {
		(void) SetFsParam(device->set, "readahead", value);
	} else {
		(void) SetFsParam(device->set, "writebehind", value);
	}
	return (0);
}


/*
 *	sam_set_fs_thresh() - API function to set the high and low thresholds
 *
 *	Input parameters --
 *		eq_ord		Equipment ordinal
 *		min_threshold	Minimum threshold to stop release for file set
 *		max_threshold	Maximum threshold to start release for file set
 *		wait_response	Wait for command response if nonzero
 *				-1	Wait forever
 *				> zero	Wait this many seconds for response
 *
 *	Output parameters --
 *		errno		Set to error number if error encountered
 *
 *	Return value --
 *		0		If successful in set_fs_threshing the media
 *		-1		If error encountered,
 *				'errno' set to error number
 */

int
sam_set_fs_thresh(
	ushort_t eq_ord,	/* Equipment ordinal */
	int min_threshold,	/* Minimum threshold to stop release */
	int max_threshold,	/* Maximum threshold to start release */
	/* LINTED argument unused in function */
	int wait_response)	/* Nonzero value to wait for response */
{
	char *fifo_path;	/* Path to FIFO pipe, from shared memory */

	dev_ent_t *device;
	operator_t operator;	/* Data on operator */
	char value[20];


	/*
	 *	Get device entry and FIFO path from master shared memory segment
	 */

	if (sam_get_dev(eq_ord, &device, &fifo_path, &operator) < 0) {
		return (-1);
	}

	if (!IS_FS(device)) {
		errno = ER_DEVICE_NOT_CORRECT_TYPE;
		return (-1);
	}
	sprintf(value, "%d", min_threshold);
	(void) SetFsParam(device->set, "low", value);
	sprintf(value, "%d", max_threshold);
	(void) SetFsParam(device->set, "high", value);
	return (0);
}
