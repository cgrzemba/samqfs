/*
 * sam_utils.c - Utility functions used in API command processing
 *
 *	is_ansi_tp_label() - Function to verify tape label VSN is correct
 *	sam_send_cmd() - Function to send command on FIFO pipe and wait
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

#pragma ident "$Revision: 1.17 $"


/* Feature test switches. */
	/* None. */

/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/param.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

/* Solaris headers. */
#include <libgen.h>
#include <sys/ipc.h>
#include <sys/shm.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/defaults.h"
#include "aml/exit_fifo.h"
#include "aml/fifo.h"
#include "sam/lib.h"
#include "aml/shm.h"

/* SAM-FS API headers */
#include "pub/sam_errno.h"

/* Private data. */
static shm_ptr_tbl_t *shm_mstr = (shm_ptr_tbl_t *)NULL;
				/* Master shared memory address */

/*
 *	Validate ANSI tape label field.
 *	Check that a string is correct for an ANSI tape label field.
 *	Returns 1 if string is valid, 0 if not
 */

int
is_ansi_tp_label(
	char *s,	/* string to be validated */
	size_t size)	/* size of field */
{
	char c;

	if (strlen(s) > size)
		return (0);
	while ((c = *s++) != '\0') {
		if (isupper(c))
			continue;
		if (isdigit(c))
			continue;
		if (strchr("!\"%&'()*+,-./:;<=>?_", c) == NULL)
			return (0);
	}
	return (1);
}





/*
 *	sam_get_dev() - Internal function to get device from shared memory area
 *
 *	Input parameters --
 *		eq_ord		Equipment ordinal to get device entry
 *
 *	Output parameters --
 *		dev		Device entry for equipment number
 *		fifo_path	FIFO command path
 *		errno		Set to error number if error encountered
 *
 *	Return value --
 *		0		If successful in getting the device address
 *		-1		If error encountered,
 *				'errno' set to error number
 *
 */

int
sam_get_dev(
	ushort_t	eq_ord,		/* Equipment ordinal */
	dev_ent_t	**dev,		/* Device entry */
	char		**fifo_path,	/* FIFO command path */
	operator_t	*operator)	/* Data on operator */
{
	dev_ptr_tbl_t *dev_tbl;		/* Device table address */
	int max_devices;		/* Maximum no. of devices */

	/*
	 *	Access device table in shared memory segment.
	 */

	if ((shm_ptr_tbl_t *)NULL == shm_mstr) {
		if ((shm_mstr =
		    (shm_ptr_tbl_t *)sam_mastershm_attach(0, SHM_RDONLY)) ==
		    (void *) -1) {
			return (-1);
		}
	}

	/* LINTED pointer cast may result in improper alignment */
	dev_tbl = (dev_ptr_tbl_t *)SHM_ADDRESS(shm_mstr, shm_mstr->dev_table);
	max_devices = dev_tbl->max_devices;

	if ((0 == (int)eq_ord) || ((int)eq_ord > max_devices) ||
	    ((dev_ent_t *)NULL == dev_tbl->d_ent[eq_ord])) {
		errno = ER_NO_EQUIP_ORDINAL;
		return (-1);
	}

	/* LINTED pointer cast may result in improper alignment */
	*dev = (dev_ent_t *)SHM_ADDRESS(shm_mstr, dev_tbl->d_ent[eq_ord]);
	*fifo_path = (char *)SHM_ADDRESS(shm_mstr, shm_mstr->fifo_path);

	*operator = GetDefaults()->operator;
	operator->gid = geteuid() == 0 ?
	    SAM_ROOT : (getegid() == operator->gid ?
	    SAM_OPER : SAM_USER);

	return (0);
}


/*
 *	sam_send_cmd() - Internal function to send command on FIFO pipe and wait
 *			 for response if requested
 *
 *	Input parameters --
 *		cmd_block	Command block to be sent on FIFO pipe
 *		wait_resp	If nonzero,
 *				wait for response on return FIFO pipe
 *				-1	Wait forever
 *				> zero	Wait this many seconds for response
 *		fifo_path	Path defining FIFO pipe from
 *				MASTER shared memory
 *
 *	Output parameters --
 *		errno		Set to error number if error encountered
 *
 *	Return value --
 *		0		If successful in sending command on FIFO pipe
 *		-1		If error encountered,
 *				'errno' set to error number
 *
 */

int
sam_send_cmd(
	sam_cmd_fifo_t *cmd_block,	/* Command block address */
	int wait_resp,			/* If nonzero, */
					/* wait for response on FIFO */
	char *fifo_path)
{
	char cmd_fifo[MAXPATHLEN];	/* Command FIFO file path/name */
	int fifo_fd;			/* Command FIFO descriptor */

	if (wait_resp) {
		set_exit_id(0, &(cmd_block->exit_id));
		if (create_exit_FIFO(&(cmd_block->exit_id)) < 0) {
			errno = ER_NO_RESPONSE_FIFO;
					/* Cannot create response FIFO */
			return (-1);
		}
	}
	else
	{
		cmd_block->exit_id.pid = 0;
	}

	/*	Verify path is not too long for destination */

	if ((strlen(fifo_path) + strlen(CMD_FIFO_NAME) + 3) > MAXPATHLEN) {
		errno = ER_FIFO_PATH_LENGTH;
		return (-1);
	}

	strcpy(cmd_fifo, fifo_path);
	strcat(cmd_fifo, "/");
	strcat(cmd_fifo, CMD_FIFO_NAME);

	if ((fifo_fd = open(cmd_fifo, O_WRONLY)) < 0) {
		return (-1);
	}

	if (write(fifo_fd, cmd_block, sizeof (sam_cmd_fifo_t)) < 0) {
		return (-1);
	}

	close(fifo_fd);

	if (wait_resp) {
		char resp_string[256];
		int completion = -1;

		if (read_server_exit_string(&(cmd_block->exit_id), &completion,
		    resp_string, 255, wait_resp) < 0) {
			int tmp = errno;
			(void) unlink_exit_FIFO(&(cmd_block->exit_id));
			errno = tmp;
			return (-1); /* "Could not retrieve command response" */
		}

		(void) unlink_exit_FIFO(&(cmd_block->exit_id));

		/*
		 *	Check if completion response string received
		 */

		if (completion < 0) {
			errno = ER_FIFO_COMMAND_RESPONSE;
			return (-1);
		}
		if (completion > 0) {
			errno = completion;
			return (-1);	/* pass errno back */
		}
	}

	return (0);			/* Return success */
}
