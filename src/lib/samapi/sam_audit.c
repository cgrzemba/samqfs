/*
 * sam_audit.c - audit removable media API
 *
 *	sam_audit() causes the device specified to be audited
 *
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

#pragma ident "$Revision: 1.20 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "sam/types.h"
#include "sam/defaults.h"
#include "aml/device.h"
#include "aml/fifo.h"
#include "sam/param.h"
#include "aml/proto.h"
#include "aml/sam_utils.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/samapi.h"
#include "pub/sam_errno.h"


/*
 *	sam_audit() - API function to audit media, including entire robot
 *
 *	Input parameters --
 *		eq_ord		Equipment ordinal
 *		ea		Element address in robot to be audited or
 *				ROBOT_NO_SLOT
 *		wait_response	Wait for command response if nonzero
 *				-1	Wait forever
 *				> zero	Wait this many seconds for response
 *
 *	Output parameters --
 *		errno		Set to error number if error encountered
 *
 *	Return value --
 *		0		If successful in auditing the media
 *		-1		If error encountered,
 *				'errno' set to error number
 *
 */

int
sam_audit(
	ushort_t eq_ord,	/*	Equipment ordinal */
	uint_t	 ea,		/*	Element address */
	int wait_response)	/*	Nonzero value to wait for response */
{
	char *fifo_path;	/*	Path to FIFO pipe, from shared memory */
	dev_ent_t *device;
	operator_t operator;	/*	Data on operator */
	sam_cmd_fifo_t	cmd_block;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;

	if (CatalogInit("SamApi") == -1) {
		errno = ER_UNABLE_TO_INIT_CATALOG;
		/* Unable to initialize catalog. */
		return (-1);
	}

	/*
	 * Get device entry and FIFO path from master shared memory segment
	 */

	if (sam_get_dev(eq_ord, &device, &fifo_path, &operator) < 0) {
		return (-1);
	}

	if ((!IS_ROBOT(device)) || (device->state > DEV_IDLE)) {
		errno = ER_NO_AUDIT;	/* "Cannot audit device" */
		return (-1);
	}

	/*
	 * Clear command block
	 */

	memset(&cmd_block, 0, sizeof (cmd_block));


	/*
	 *	If a ea is specified, verify that it is valid
	 */

	if ((uint_t)ROBOT_NO_SLOT != ea) {
		ce = CatalogGetCeByLoc(eq_ord, ea, 0, &ced);
		if (ce == NULL) {
			errno = ER_NOT_VALID_SLOT_NUMBER;
			return (-1);
		}
		if (!(ce->CeStatus & CES_inuse)) {
			errno = ER_SLOT_NOT_OCCUPIED; /* Slot is not occupied */
			return (-1);
		}
	}
	else
	{
		/*
		 *	Check if operator permitted to do a full audit
		 */

		if (!SAM_OPER_FULLAUDIT(operator)) {
			errno = ER_OPERATOR_NOT_PRIV;
			return (-1);
		}
	}

	/*
	 *	Put command block values into command
	 */

	cmd_block.magic = CMD_FIFO_MAGIC;
	cmd_block.cmd = CMD_FIFO_AUDIT;
	cmd_block.slot = ea;
	cmd_block.eq = eq_ord;
	cmd_block.part = 0;

	/*
	 *	Send command and wait for response if requested
	 */

	if (sam_send_cmd(&cmd_block, wait_response, fifo_path) < 0) {
		return (-1);
	}

	return (0);
}
