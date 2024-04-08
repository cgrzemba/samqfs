/*
 * sam_set_state.c
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

#pragma ident "$Revision: 1.20 $"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <stropts.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/ipc.h>
#include <sys/stat.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/fifo.h"
#include "aml/proto.h"
#include "sam/devnm.h"
#include "aml/samapi.h"
#include "aml/sam_utils.h"
#include "pub/devstat.h"
#include "pub/sam_errno.h"



/*
 *	sam_set_state() - API function to set device state
 *
 *	Input parameters --
 *		eq_ord		Equipment ordinal
 *		new_state	Enumeration value from devstat.h
 *		wait_response	Wait for command response if nonzero
 *				-1	Wait forever
 *				> zero	Wait this many seconds for response
 *
 *	Output parameters --
 *		errno		Set to error number if error encountered
 *
 *	Return value --
 *		0		If successful in moveing the media
 *		-1		If error encountered,
 *				'errno' set to error number
 *
 */
int
sam_set_state(
	ushort_t eq_ord,	/* Equipment ordinal */
	dstate_t new_state,	/* New state for device */
	int wait_response)
{
	char *fifo_path;		/* FIFO pipe path */
	dev_ent_t *device;
	int forever = FALSE;		/* Set TRUE if wait_response is -1 */
	int wait = wait_response;	/* Wait time for busy */
	operator_t operator;		/* Data on operator */
	sam_cmd_fifo_t	cmd_block;


	/*
	 *	Verify valid new state is specified
	 */

	if ((new_state < DEV_ON) ||
	    (new_state == DEV_DOWN) ||
	    (new_state > DEV_DOWN)) {
		errno = ER_INVALID_STATE_SPECIFIED;
		return (-1);
	}

	/*
	 *	If wait for response equals -1, set flag to indicate
	 *	wait forever
	 */

	if (-1 == wait_response) {
		forever = TRUE;
	}

	/*
	 *	Get device entry and FIFO path from master shared memory segment
	 */

	if (sam_get_dev(eq_ord, &device, &fifo_path, &operator) < 0) {
		return (-1);
	}

	/*
	 *	If NOT ( (turning device ON and an operator class user)
	 *	or user has privilege to change device states ), return
	 */

	if (!((new_state == DEV_ON && SAM_OPER_LEVEL(operator)) ||
	    SAM_OPER_STATE(operator))) {
		errno = ER_OPERATOR_NOT_PRIV;
		return (-1);
	}

	/*
	 *	If device is not a removable media device, return with error
	 */

	if (!(IS_OPTICAL(device) || IS_TAPE(device) || IS_ROBOT(device))) {
		errno = ER_NOT_REMOV_MEDIA_DEVICE;
		return (-1);
	}

	/*
	 *	If device is DOWN and the new state is not OFF,
	 *	return with error
	 */

	if ((device->state == DEV_DOWN) && (new_state != DEV_OFF)) {
		errno = ER_DEVICE_DOWN_NEW_STATE;
		return (-1);
	}

	if ((device->state >= DEV_UNAVAIL) && (IS_TAPE(device))) {
		int open_fd;

		if (((open_fd = open(device->name, O_RDONLY)) < 0) &&
		    (errno == EBUSY)) {
			if (forever || (wait > 0)) {
				while (forever || (wait-- > 0)) {
				/* N.B. Bad indentation here to meet cstyle */
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

				if (!(forever || (wait >= 0))) {
					errno = EBUSY;
					return (-1);
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
	}

	memset(&cmd_block, 0, sizeof (cmd_block));

	if (device->state != new_state) {
		cmd_block.eq = eq_ord;
		cmd_block.state = new_state;
		cmd_block.magic = CMD_FIFO_MAGIC;
		cmd_block.cmd = CMD_FIFO_SET_STATE;

		if (sam_send_cmd(&cmd_block, wait_response, fifo_path) < 0) {
			return (-1);
		}
	}

	return (0);
}
