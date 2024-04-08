/*
 *  unload.c
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
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>

#define	DEC_INIT

#include "sam/types.h"
#include "sam/param.h"
#include "sam/custmsg.h"
#include "aml/shm.h"
#include "aml/device.h"
#include "aml/fifo.h"
#include "aml/proto.h"
#include "sam/lib.h"

int
main(int argc, char **argv)
{
	int   shmid, fifo_fd;
	int   wait = FALSE;
	void  *memory;
	char  fifo_file[MAXPATHLEN];

	dev_ent_t		*device;
	dev_ptr_tbl_t	*dev_ptr_tbl;
	shm_ptr_tbl_t	*shm_ptr_tbl;
	sam_cmd_fifo_t	cmd_block;
	char *prg_name = (char *)basename(argv[0]);

	CustmsgInit(0, NULL);
#if !defined(DEBUG)
	if (geteuid() != 0) {
		fprintf(stderr,
		    catgets(catfd, SET, 100,
		    "%s may be run only by super-user.\n"),
		    prg_name);
		exit(1);
	}
#endif

	if (argc < 2 || argc > 3) {
		fprintf(stderr, catgets(catfd, SET, 13001, "Usage: %s %s\n"),
		    prg_name, "[-w] eq");
		exit(1);
	}

	argv++;
	argc--;

	if (argc == 2) {
		if (strcmp(*argv, "-w") == 0) {
			wait = TRUE;
		} else {
			fprintf(stderr,
			    catgets(catfd, SET, 13001, "Usage: %s %s\n"),
			    prg_name, "[-w] eq");
			exit(1);
		}
		argv++;
	}

	memset(&cmd_block, 0, sizeof (cmd_block));

	if ((shmid = shmget(SHM_MASTER_KEY, 0, 0)) < 0) {
		fprintf(stderr,
		    catgets(catfd, SET, 272, "%s: SAM_FS is not running.\n"),
		    prg_name);
		exit(1);
	}

	if ((memory = shmat(shmid, (void *)NULL, 0444)) == (void *)-1) {
		fprintf(stderr,
		    catgets(catfd, SET, 211,
		    "%s: Cannot attach shared memory segment: %s\n"),
		    prg_name, strerror(errno));
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
		    catgets(catfd, SET, 2314,
		    "%s: %d is not a valid equipment ordinal.\n"),
		    prg_name, cmd_block.eq);
		exit(1);
	}

	if (dev_ptr_tbl->d_ent[cmd_block.eq] == 0) {
		fprintf(stderr,
		    catgets(catfd, SET, 2314,
		    "%s: %d is not a valid equipment ordinal.\n"),
		    prg_name, cmd_block.eq);
		exit(1);
	}

	device = (dev_ent_t *)
	    ((char *)memory + (int)dev_ptr_tbl->d_ent[cmd_block.eq]);

	if (!(IS_OPTICAL(device) || IS_TAPE(device) || IS_ROBOT(device))) {
		fprintf(stderr,
		    catgets(catfd, SET, 2313,
		    "%s: %d is not a removable media device.\n"),
		    prg_name, cmd_block.eq);
		exit(1);
	}

	if (device->state >= DEV_UNAVAIL && IS_TAPE(device)) {
		int open_fd;

		if ((open_fd = open(device->name, O_RDONLY)) < 0 &&
		    errno == EBUSY) {
			if (wait) {
				while ((open_fd =
				    open(device->name, O_RDONLY)) < 0 &&
				    errno == EBUSY)
					sleep(5);
			} else {
				fprintf(stderr,
				    catgets(catfd, SET, 2317,
				    "%s: Device %d is open by another process."
				    "\n"),
				    prg_name, cmd_block.eq);
				exit(1);
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

	if (!device->status.b.ready) {
		fprintf(stderr, catgets(catfd, SET, 2782,
		    "unload: %d is not loaded.\n"),
		    cmd_block.eq);
		exit(1);
	}

	cmd_block.magic = CMD_FIFO_MAGIC;
	cmd_block.slot = ROBOT_NO_SLOT;
	cmd_block.cmd = CMD_FIFO_UNLOAD;
	if ((fifo_fd = open(fifo_file, O_WRONLY)) < 0) {
		perror(catgets(catfd, SET, 1113,
		    "Unable to open command fifo:"));
		exit(1);
	}

	write(fifo_fd, &cmd_block, sizeof (sam_cmd_fifo_t));
	close(fifo_fd);
	if (wait) {
		if (IS_ROBOT(device)) {
			while (device->state != DEV_OFF)
				sleep(5);
		} else {
			while (device->status.b.ready ||
			    device->status.b.unload)
				sleep(5);
			while (device->open_count)
				sleep(1);
		}
	}
	return (0);
}
