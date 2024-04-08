/*
 *   clri.c - clear an inode (zap it to all zeroes)
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
 *
 */

#pragma ident "$Revision: 1.15 $"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#define DEC_INIT
#include "sam/uioctl.h"
#include "sam/lib.h"
#include <errno.h>
#include <string.h>

int sam_fd;

int issue_ioctl(int inum);

void
usage()
{
	fprintf(stderr, "Usage: %s: %s [-F samfs] [-V] mount-point ino\n",
	    program_name, program_name);
}

int
main(int argc, char **argv)
{
	char *c;
	char *mnt_point;
	extern char *optarg;
	extern int optind;
	int Fflg = 0;
	int Vflg = 0;
	int errs = 0;
	int g;
	int inum;

	program_name = argv[0];

	while ((g = getopt(argc, argv, "F:V")) != EOF) {
		switch (g) {
		case 'F':
			if (Fflg) {
				errs = 1;
			} else {
				Fflg = 1;
				if (strcmp(optarg, "samfs")) {
					usage();
					exit(1);
				}
			}
			break;
		case 'V':
			if (Vflg) {
				errs = 1;
			} else {
				Vflg = 1;
			}
			break;
		default:
			errs = 1;
			break;
		}
	}

	if (errs) {
		usage();
		exit(1);
	}

	if (Vflg) {
		fprintf(stderr, "%s version 1.0\n", program_name);
		exit(0);
	}

	if (argc - optind + 1 != 3) {
		usage();
		exit(1);
	}

	mnt_point = argv[optind];

	inum = strtol(argv[optind+1], &c, 0);
	if (*c != '\0') {
		fprintf(stderr, "%s: cannot convert %s to integer\n",
		    program_name,
		    argv[optind+1]);
		exit(1);
	}

	sam_fd = open(mnt_point, O_RDONLY);
	if (sam_fd < 0) {
		fprintf(stderr, "%s: cannot open %s for reading\n",
		    program_name, mnt_point);
		exit(1);
	}

	return (issue_ioctl(inum));
}

int
issue_ioctl(int inum)
{
	struct sam_ioctl_inode ctl;
	int retval;

	ctl.ino = inum;

	retval = ioctl(sam_fd, F_ZAPINO, &ctl);
	if (retval < 0) {
		fprintf(stderr, "%s: ioctl(F_ZAPINO) returns %d, %s\n",
		    program_name, retval, strerror(errno));
		return (2);
	}
	return (0);
}
