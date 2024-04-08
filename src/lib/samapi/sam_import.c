/*
 * sam_import.c - import removable media API
 *
 *	sam_import() causes the media specified to be input into the removable
 *	media device.
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

#pragma ident "$Revision: 1.18 $"


#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/fifo.h"
#include "sam/lib.h"
#include "aml/proto.h"
#include "sam/devnm.h"
#include "aml/samapi.h"
#include "aml/sam_utils.h"
#include "pub/devstat.h"
#include "pub/sam_errno.h"




/*
 *	sam_import() - API function to import media
 *
 *	Input parameters --
 *		eq_ord		Equipment ordinal
 *		vsn		Volume serial number to import or(char *) NULL
 *		media_type	Type of media to import(Historian)
 *				or(char *) NULL
 *		audit_eod	Flag to indicate that audit to EOD performed
 *		wait_response	Wait for command response if nonzero
 *				-1	Wait forever
 *				> zero	Wait this many seconds for response
 *
 *	Output parameters --
 *		errno		Set to error number if error encountered
 *
 *	Return value --
 *		0		If successful in exporting the media
 *		-1		If error encountered,
 *				'errno' set to error number
 *
 */

int
sam_import(
	ushort_t eq_ord,	/* Equipment ordinal */
	char	*vsn,		/* Volume Serial Number */
	char	*media_nm,	/* Media nmemonic string */
	int	audit_eod,	/* Flag for audit to EOD performed */
	int	wait_response)	/* Set nonzero value to wait for response */
{
	char *fifo_path;	/* Path to FIFO pipe, from shared memory */
	int media = 0;
	dev_ent_t *device;
	operator_t operator;	/* Data on operator */
	sam_cmd_fifo_t cmd_block;


	/*
	 * Convert media nmemonic to device type if needed
	 */

	if ((char *)NULL != media_nm) {
		if ((media = media_to_device(media_nm)) == -1) {
			errno = ER_INVALID_MEDIA_TYPE;
			return (-1);
		}
	}

	/*
	 * Get device entry and FIFO path from master shared memory segment
	 */

	if (sam_get_dev(eq_ord, &device, &fifo_path, &operator) < 0) {
		return (-1);
	}

	/*
	 * If user cannot perform movement of media within a library, return
	 */

	if (!SAM_OPER_SLOT(operator)) {
		errno = ER_OPERATOR_NOT_PRIV;
		return (-1);
	}

	/*
	 * If device is not ready for import, return
	 */

	if (!device->status.b.ready) {
		errno = ER_DEVICE_NOT_READY;
		return (-1);
	}

	/*
	 * If device is not a robot, return
	 */

	if (!IS_ROBOT(device)) {
		errno = ER_ROBOT_DEVICE_REQUIRED;
		return (-1);
	}

	/*
	 * If device is not the Historian and media type specified, return
	 */

	if ((device->equ_type != DT_HISTORIAN) && (media != 0)) {
		errno = ER_HISTORIAN_MEDIA_ONLY;
		return (-1);
	}

	/*
	 * If device is the Historian and audit to end of data specified, return
	 */

	if ((device->equ_type == DT_HISTORIAN) && audit_eod) {
		errno = ER_AUDIT_EOD_NOT_HISTORIAN;
		return (-1);
	}

	/*
	 * If device is the Historian and no media nor VSN specified, return
	 */

	if (device->equ_type == DT_HISTORIAN) {
		if (media == 0) {
			errno = ER_MEDIA_FOR_HISTORIAN;
			return (-1);
		}

		if ((char *)NULL == vsn) {
			errno = ER_VSN_BARCODE_REQUIRED;
			return (-1);
		}

		cmd_block.cmd = CMD_FIFO_ADD_VSN;
		cmd_block.media = media;
	}
	else
	{
		/*
		 * If device is a robot and no VSN is specified for a GRAU,
		 * STK, or IBM library, return with error
		 */

		if (((char *)NULL == vsn) && ((DT_GRAUACI == device->type) ||
		    (DT_STKAPI == device->type) ||
		    (DT_IBMATL == device->type) ||
		    (DT_SONYPSC == device->type))) {
			errno = ER_VSN_BARCODE_REQUIRED;
			return (-1);
		}
		cmd_block.cmd = CMD_FIFO_IMPORT;

		if (audit_eod) {
			cmd_block.flags |= CMD_IMPORT_AUDIT;
		}
	}

	cmd_block.magic = CMD_FIFO_MAGIC;
	cmd_block.slot = ROBOT_NO_SLOT;
	cmd_block.eq = eq_ord;

	if ((char *)NULL != vsn) {
		strncpy(cmd_block.vsn, vsn, sizeof (vsn_t));
	}

	if (sam_send_cmd(&cmd_block, wait_response, fifo_path) < 0) {
		return (-1);
	}

	return (0);
}
