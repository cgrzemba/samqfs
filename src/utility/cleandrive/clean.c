/*
 * clean.c
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

#pragma ident "$Revision: 1.17 $"


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>

#define	MAIN

#include "sam/types.h"
#include "sam/param.h"
#include "aml/shm.h"
#include "aml/device.h"
#include "aml/fifo.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"


int
main(int argc, char **argv)
{
	int	shmid, fifo_fd;
	void	*memory;
	char	fifo_file[MAXPATHLEN];

	dev_ent_t			*device;
	dev_ptr_tbl_t		*dev_ptr_tbl;
	shm_ptr_tbl_t		*shm_ptr_tbl;
	sam_cmd_fifo_t	cmd_block;

	CustmsgInit(0, NULL);
#if !defined(DEBUG)
	if (geteuid() != 0) {
		fprintf(stderr, catgets(catfd, SET, 100,
		    "%s may be run only by super-user.\n"), basename(argv[0]));
		exit(1);
	}
#endif
	if (argc != 2) {
		fprintf(stderr, catgets(catfd, SET, 13001, "Usage: %s %s\n"),
		    "cleandrive", "eq");
		exit(1);
	}

	argv++;

	memset(&cmd_block, 0, sizeof (cmd_block));

	if ((shmid = shmget(SHM_MASTER_KEY, 0, 0)) < 0) {
		fprintf(stderr,
		    catgets(catfd, SET, 2244, "SAM_FS is not running.\n"));
		exit(1);
	}

	if ((memory = shmat(shmid, (void *)NULL, 0444)) == (void *)-1) {
		fprintf(stderr,
		    catgets(catfd, SET, 568,
		    "Cannot attach shared memory segment\n"));
		exit(1);
	}

	shm_ptr_tbl = (shm_ptr_tbl_t *)memory;
	sprintf(fifo_file, "%s" "/" CMD_FIFO_NAME,
	    ((char *)memory + shm_ptr_tbl->fifo_path));

	dev_ptr_tbl =
	    (dev_ptr_tbl_t *)((char *)memory + shm_ptr_tbl->dev_table);
	cmd_block.eq = atoi(*argv);

	if (cmd_block.eq <= 0 || cmd_block.eq > dev_ptr_tbl->max_devices) {
		fprintf(stderr,
		    catgets(catfd, SET, 80,
		    "%d is not a valid equipment ordinal.\n"),
		    cmd_block.eq);
		exit(1);
	}

	if (dev_ptr_tbl->d_ent[cmd_block.eq] == 0) {
		fprintf(stderr,
		    catgets(catfd, SET, 80,
		    "%d is not a valid equipment ordinal.\n"),
		    cmd_block.eq);
		exit(1);
	}

	device = (dev_ent_t *)
	    ((char *)memory + (int)dev_ptr_tbl->d_ent[cmd_block.eq]);

	if (!IS_TAPE(device)) {
		fprintf(stderr,
		    catgets(catfd, SET, 81,
		    "%d is not tape device.\n"), cmd_block.eq);
		exit(1);
	}
	if (device->fseq != 0) {
		device = (dev_ent_t *)
		    ((char *)memory + (int)dev_ptr_tbl->d_ent[device->fseq]);
		if (device->equ_type == DT_IBMATL) {
			fprintf(stderr,
			    catgets(catfd, SET, 2479,
			    "The IBM3494 only supports automatic "
			    "drive cleaning.\n"));
			exit(1);
		}
	}

	cmd_block.magic = CMD_FIFO_MAGIC;
	cmd_block.slot = ROBOT_NO_SLOT;
	cmd_block.cmd = CMD_FIFO_CLEAN;
	if ((fifo_fd = open(fifo_file, O_WRONLY)) < 0) {
		perror(catgets(catfd, SET, 1113,
		    "Unable to open command fifo:"));
		exit(1);
	}

	write(fifo_fd, &cmd_block, sizeof (sam_cmd_fifo_t));

	close(fifo_fd);
	return (0);
}
