/*
 * sam_unload.c - Contains API for sam_unload() function
 *
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.19 $"


#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stropts.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mtio.h>
#include <sys/stat.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/fifo.h"
#include "aml/proto.h"
#include "aml/sam_utils.h"
#include "aml/samapi.h"
#include "pub/sam_errno.h"



/*
 * sam_unload() - API function to unload
 *
 * Input parameters --
 *	eq_ord		Equipment ordinal
 *	wait_response	Wait for command response if nonzero
 *			-1	Wait forever
 *			> zero	Wait this many seconds for response
 *
 * Output parameters --
 *	errno		Set to error number if error encountered
 *
 * Return value --
 *	0		If successful in unloading the media
 *	-1		If error encountered,
 *			'errno' set to error number
 *
 */

int
sam_unload(
	ushort_t eq_ord,	/* Equipment ordinal */
	int wait_response)	/* Set nonzero to wait for command response */
{
	char	*fifo_path;		/* Path to FIFO pipe, */
					/* from shared memory */
	int	forever = FALSE;	/* Set TRUE if wait_response is -1 */
	int	wait = wait_response;	/* Duration of wait defined by caller */

	dev_ent_t	*device;
	operator_t	operator;	/* Data on operator */
	sam_cmd_fifo_t	cmd_block;


	/*
	 * If wait for response equals -1,
	 * set flag to indicate wait forever
	 */

	if (-1 == wait_response) {
		forever = TRUE;
	}

	/*
	 * Get device entry and FIFO path from master shared memory segment
	 */

	if (sam_get_dev(eq_ord, &device, &fifo_path, &operator) < 0) {
		return (-1);
	}

	if (!(IS_OPTICAL(device) || IS_TAPE(device) || IS_ROBOT(device))) {
		errno = ER_NOT_REMOV_MEDIA_DEVICE;
		return (-1);
	}

	/*
	 * If robot device and operator not privileged, return with error
	 */

	if (IS_ROBOT(device) && !SAM_OPER_SLOT(operator)) {
		errno = ER_OPERATOR_NOT_PRIV;
		return (-1);
	}

	/*
	 * If tape device is not available for SAM-FS,
	 * check device availability
	 */

	if (device->state >= DEV_UNAVAIL && IS_TAPE(device)) {
		int open_fd;

		if (((open_fd = open(device->name, O_RDONLY)) < 0) &&
		    (errno == EBUSY)) {
			if (forever || (wait > 0)) {
				while (forever || (wait-- > 0)) {
				/* N.B. Bad indentation to meet cstyle */
				/* requirements */
				if (((open_fd =
				    open(device->name, O_RDONLY)) < 0) &&
				    (errno == EBUSY)) {
					sleep(1);
				}
				else
				{
					break;
				}
				}
			}
			else
			{
				errno = ER_DEVICE_USE_BY_ANOTHER;
				return (-1);
			}
		}

		if (open_fd >= 0) {
			struct  mtop  tape_op;

			tape_op.mt_op = MTOFFL;
			tape_op.mt_count = 0;
			(void) ioctl(open_fd, MTIOCTOP, &tape_op);
			close(open_fd);
		}
		else
		{
			errno = ER_NO_DEVICE_FOUND;
			return (-1);
		}
	}

	if (!device->status.b.ready) {
		errno = ER_DEVICE_NOT_READY;
		return (-1);
	}

	/*
	 * Ready and send command message for unload of the device
	 */

	memset(&cmd_block, 0, sizeof (cmd_block));
	cmd_block.magic = CMD_FIFO_MAGIC;
	cmd_block.slot = ROBOT_NO_SLOT;
	cmd_block.cmd = CMD_FIFO_UNLOAD;
	cmd_block.eq = eq_ord;

	if (sam_send_cmd(&cmd_block, wait_response, fifo_path) < 0) {
		return (-1);
	}

	return (0);
}
