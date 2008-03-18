/*
 * vsn_stat.c - Get multi-volume information for a SAMFS file archive copy.
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

#pragma ident "$Revision: 1.16 $"


/* Feature test switches. */
	/* None. */

/* ANSI C headers. */
	/* None. */
#ifdef MAIN
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "pub/rminfo.h"
#endif

/* POSIX headers. */
#include <sys/types.h>

/* Solaris headers. */
	/* None. */

/* SAM-FS headers. */
#include "sam/syscall.h"
#include "sam/lib.h"
#include "pub/stat.h"

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
sam_vsn_stat(const char *path, int copy, struct sam_section *buf,
	size_t bufsize)
{
	struct sam_vsn_stat_arg arg;

	arg.path.ptr = (char *)path;
	arg.buf.ptr = buf;
	arg.bufsize = bufsize;
	arg.copy = copy;
	return (sam_syscall(SC_vsn_stat, &arg, sizeof (arg)));
}

#ifdef MAIN
int
main(int argc, char **argv)
{
	int			copy;
	int			num_vsns;
	int			error;
	int			sec;
	char			*pathname;
	struct sam_stat		file_stat_buf;
	struct sam_section	vsns[MAX_VOLUMES];

	if (argc != 3) {
		fprintf(stderr, "Usage: %s file_path copy_number.\n", argv[0]);
		exit(-70);
	}

	pathname	= argv[1];
	copy		= atoi(argv[2]);

	if (copy < 0 || copy > 3) {
		fprintf(stderr,
		    "Error: copy_number must be greater than or equal "
		    "to 0 and less than or equal to 3.\nUsage: %s "
		    "file_path copy_number.\n", argv[0]);
		exit(-80);
	}

	(void) memset((void *) &file_stat_buf, 0, sizeof (struct sam_stat));

	error = sam_stat(pathname, &file_stat_buf, sizeof (struct sam_stat));

	if (error) {
		fprintf(stderr,
		    "Error failed to sam_stat the file, %s.\n", pathname);
		exit(-90);
	}

	num_vsns = (int)file_stat_buf.copy[copy].n_vsns;

	if (num_vsns > MAX_VOLUMES) {
		fprintf(stderr,
		    "Warning: copy number %d of file %s uses %d volumes "
		    "according to sam_stat,\n         but this number "
		    "exceeds %d, the maximum number of vsns allowed\n"
		    "         for one copy.\n",
		    copy, pathname, num_vsns, (int)MAX_VOLUMES);
		num_vsns = MAX_VOLUMES;
	} else if (num_vsns < 0) {
		fprintf(stderr,
		    "Warning: sam_stat of file %s revealed that copy "
		    "%d has sections on %d volumes.\n         but this"
		    " number is negative, a file can not use a negative\n"
		    "         number of vsns.\n",
		    pathname, copy, num_vsns);
	} else if (num_vsns == 0) {
		fprintf(stderr,
		    "Warning: sam_stat of file %s revealed that copy %d "
		    "currently utilizes no vsns.\n         Call to "
		    "sam_vsn_stat will fail.\n", pathname, copy);
	} else if (num_vsns == 1) {
		fprintf(stderr,
		    "Warning: sam_stat of file %s revealed that copy %d "
		    "uses only one vsn.\n         Call to sam_vsn_stat "
		    "will fail.\n", pathname, copy);
	} else {
		fprintf(stderr,
		    "Detail: Call to sam_stat indicates that copy %d of "
		    "file %s uses %d vsns.\n", copy, pathname, num_vsns);
	}

	memset((void *)vsns, (char)0,
	    sizeof (struct sam_section) * MAX_VOLUMES);

	error = sam_vsn_stat(pathname, copy, vsns,
	    MAX_VOLUMES * sizeof (struct sam_section));

	if (error) {
		fprintf(stderr,
		    "Error: Call to sam_vsn_stat(%s, %d, ...) failed, "
		    "call returned %d.\n", pathname, copy, error);
		fprintf(stderr, "       errno set to %d.\n", errno);
		exit(error);
	} else {
		fprintf(stderr,
		    "Detail: Call to sam_vsn_stat(%s, %d, ...) returned "
		    "and no error was indicated.\n", pathname, copy);
	}

	fprintf(stdout, "%s: copy: %d\n", pathname, copy);

	for (sec = 0; sec < num_vsns; sec++) {
		fprintf(stdout,
		    "    section %d: length: %10lld, position: %10llx,\n"
		    "                 offset: %-4llx, vsn: %s\n",
		    sec, vsns[sec].length, vsns[sec].position,
		    ((vsns[sec].offset)/512), vsns[sec].vsn);
	}

	return (0);
}
#endif
