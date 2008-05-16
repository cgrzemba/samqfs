/*
 * sam_clear_request.c - Clear the specified slot from the mount requests
 *
 *	sam_clear_request() function clears the specified slot or entry from
 *	the removable media mount requests
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


#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <unistd.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "sam/defaults.h"
#include "aml/fifo.h"
#include "sam/lib.h"
#include "aml/proto.h"
#include "aml/sam_utils.h"
#include "aml/shm.h"

/* API headers. */
#include "aml/samapi.h"
#include "pub/sam_errno.h"




/*
 *	sam_clear_request() - API function to clear a removable media mount
 *			      request
 *
 *	Input parameters --
 *		slot		Slot number
 *		wait_response	Wait for command response if nonzero
 *				-1	Wait forever
 *				> zero	Wait this many seconds
 *					for response
 *
 *	Output parameters --
 *		errno		Set to error number if error encountered
 *
 *	Return value --
 *		0		If successful in moving the media
 *		-1		If error encountered,
 *				'errno' set to error number
 *
 */

int
sam_clear_request(
	uint_t	 slot,		/* Slot number */
	int wait_response)	/* Set nonzero to wait for response */
{
	char	*fifo_path;	/* Path to FIFO pipe, from shared memory */
	int	cmd = CMD_FIFO_DELETE_P;

	operator_t	operator;	/* Operator data */
	sam_cmd_fifo_t	cmd_block;
	shm_alloc_t	master_shm;	/* Master shared memory structure */


	/*
	 *	Access device table in shared memory segment.
	 */

	if ((master_shm.shared_memory =
	    (shm_ptr_tbl_t *)sam_mastershm_attach(0, SHM_RDONLY)) ==
	    (void *) -1) {
		return (-1);
	}


	/*
	 *	Check if operator has privilege for label
	 */

	SET_SAM_OPER_LEVEL(operator);

	if (!SAM_OPER_CLEAR(operator)) {
		shmdt((char *)master_shm.shared_memory);
		errno = ER_OPERATOR_NOT_PRIV;
		return (-1);
	}

	fifo_path = (char *)SHM_REF_ADDR(((shm_ptr_tbl_t *)
	    master_shm.shared_memory)->fifo_path);

	/*
	 * Clear command block
	 */

	memset(&cmd_block, 0, sizeof (cmd_block));

	/*
	 *	Format up commmand to sam-amld
	 */

	cmd_block.magic = CMD_FIFO_MAGIC;
	cmd_block.slot = slot;
	cmd_block.cmd = cmd;

	/*
	 *	Send commmand to sam-amld
	 */

	if (sam_send_cmd(&cmd_block, wait_response, fifo_path) < 0) {
		shmdt((char *)master_shm.shared_memory);
		return (-1);
	}

	shmdt((char *)master_shm.shared_memory);
	return (0);
}
