/*
 *	alterfile.c - Alter the content of a file.
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

#pragma ident "$Revision: 1.16 $"


/* Feature test switches. */
	/* None. */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <libgen.h>
#include <sys/mman.h>

/* SAM-FS headers. */
#define DEC_INIT
#include <sam/lib.h>

/* Local headers. */
	/* None. */

/* Macros. */
	/* None. */

/* Types. */
	/* None. */

/* Structures. */
	/* None. */

/* Private data. */
	/* None. */

/* Private functions. */
	/* None. */

/* Public data. */
	/* None. */

/* Function macros. */
	/* None. */

/* Signal catching functions. */
	/* None. */

int
main(int argc, char *argv[])
{
	extern char *program_name;
	extern char *optarg;
	extern int optind;
	uchar_t value = 0;
	offset_t offset = -1;
	char c;
	int errflag = 0;
	int xor = 0;

	program_name = basename(argv[0]);

	while ((c = getopt(argc, argv, "o:v:x:")) != EOF) {
		switch (c) {
		case 'o':
			offset = strtol(optarg, NULL, 0);
			break;
		case 'v':
			value = strtol(optarg, NULL, 0);
			break;
		case 'x':
			value = strtol(optarg, NULL, 0);
			xor = 1;
			break;
		case '?':
		default:
			errflag++;
			break;
		}
	}

	if ((argc - optind) < 1) {
		fprintf(stderr,
		"Usage: %s [-ooffset] [-vvalue] [-xvalue] file ...\n",
		    program_name);
		exit(2);
	}

	while (optind < argc) {
		struct stat64 st;
		uchar_t *data;
		char *fname;
		int fd;
		offset_t bn;

		/*
		 * Access the file.
		 */
		fname = argv[optind++];
		if ((fd = open(fname, O_RDWR | O_LARGEFILE, 0777)) < 0) {
			error(1, 1, "Cannot open %s", fname);
		}
		if (fstat64(fd, &st) != 0) {
			error(1, 1, "Cannot stat %s", fname);
		}
		data = (uchar_t *)mmap(NULL, st.st_size,
		    (PROT_READ | PROT_WRITE),
		    MAP_SHARED, fd, 0);
		if (data == MAP_FAILED)  error(1, 1, "Cannot mmap %s", fname);

		/*
		 * Change the data.
		 */
		if (offset == -1)  bn = (double)st.st_size * drand48();
		else  bn = (offset < st.st_size) ? offset : st.st_size -1;
		if (!xor)  data[bn] = value;
		else  data[bn] ^= value;

		if (munmap((char *)data, st.st_size) == -1) {
			error(1, 1, "Cannot munmap %s", fname);
		}
		close(fd);
	}
	return (0);
}
