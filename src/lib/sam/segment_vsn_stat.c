/*
 * segment_vsn_stat.c - Get multi-volume information for a data segment of a
 *			SAMFS file archive copy.
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

#pragma ident "$Revision: 1.11 $"

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

#ifndef MAIN
int
sam_segment_vsn_stat(const char *path, int copy, int segment_index,
	struct sam_section *buf, size_t bufsize)
{
	struct sam_segment_vsn_stat_arg arg;

	arg.path.ptr = (char *)path;
	arg.buf.ptr = buf;
	arg.bufsize = bufsize;
	arg.copy = copy;
	arg.segment = segment_index;
	return (sam_syscall(SC_segment_vsn_stat, &arg, sizeof (arg)));
}
#endif

#ifdef MAIN
int
main(int argc, char **argv)
{
	int			copy;
	int			segment;
	int			num_segs;
	int			num_vsns;
	int			error;
	int			sec;
	char			*pathname;
	struct sam_stat		file_stat_buf;
	struct sam_stat		*data_seg_stat_buf;
	struct sam_section	vsns[MAX_VOLUMES];

	if (argc != 4) {
		fprintf(stderr,
		    "Usage: %s file_path copy_number segment_number.\n",
		    argv[0]);
		exit(-70);
	}

	pathname	= argv[1];
	copy		= atoi(argv[2]);
	segment		= atoi(argv[3]);

	if (copy < 0 || copy > 3) {
		fprintf(stderr,
		    "Error: copy_number must be greater than or equal"
		    " to 0 and less than or equal to 3.\nUsage: %s "
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

	if (!SS_ISSEGMENT_F(file_stat_buf.attr)) {
		fprintf(stderr,
		    "Error: the file %s is not segmented, required"
		    " call to\n       sam_segment_stat will fail.\n\n"
		    "       Will not be able to call "
		    "sam_segment_vsn_stat.\n", pathname);
		num_segs = 1;
	} else {
		num_segs = file_stat_buf.st_size /
		    (file_stat_buf.segment_size * 1048576);
		if (file_stat_buf.st_size >
		    num_segs * file_stat_buf.segment_size * 1048576) {
			num_segs++;
		}

		fprintf(stderr,
		    "Detail: %s is a segmented file it is %lld bytes and\n"
		    "        has %d segments of size %dMb.\n", pathname,
		    (long long)file_stat_buf.st_size,
		    (int)num_segs,
		    (int)file_stat_buf.segment_size);
	}

	data_seg_stat_buf = (struct sam_stat *)malloc(num_segs*
	    sizeof (struct sam_stat));

	if (data_seg_stat_buf ==  (struct sam_stat *)NULL) {
		fprintf(stderr,
		    "Error: failed to allocate an array of %d sam_stat "
		    "structures for getting information on the data "
		    "segments of %s.\n", num_segs, pathname);
		exit(-100);
	}

	(void) memset((void *)data_seg_stat_buf, (char)0,
	    num_segs * sizeof (struct sam_stat));

	error = sam_segment_stat(pathname, data_seg_stat_buf,
	    num_segs * sizeof (struct sam_stat));

	if (error) {
		fprintf(stderr,
		    "Error: sam_segment_stat of file %s with %d segments "
		    "failed.\n       %d was returned and errno was set "
		    "to %d.", pathname, num_segs, error, errno);
		free(data_seg_stat_buf);

		exit(-110);
	}

	if (segment >= num_segs) {
		fprintf(stderr,
		    "Error: Can not perform sam_segment_vsn_stat on "
		    "segment %d of file %s.\n       This file has %d "
		    "segments.\n", segment, pathname,
		    num_segs);
		free(data_seg_stat_buf);

		exit(-120);
	}

	num_vsns = (int)data_seg_stat_buf[segment].copy[copy].n_vsns;

	if (num_vsns > MAX_VOLUMES) {
		fprintf(stderr,
		    "Warning: copy number %d of segment %d of file %s "
		    "uses %d volumes\n         according to "
		    "sam_segment_stat, but this number exceeds %d,\n"
		    "         the maximum number of vsns allowed for one "
		    "copy.\n",
		    copy, segment, pathname, num_vsns, (int)MAX_VOLUMES);
		num_vsns = MAX_VOLUMES;
	} else if (num_vsns < 0) {
		fprintf(stderr,
		    "Warning: sam_segment_stat of segment %d of file %s\n"
		    "          revealed that copy %d has sections on %d "
		    "volumes,\n          but this number is negative, a "
		    "file can not use a\n          negative number of "
		    "vsns.\n", segment, pathname, copy, num_vsns);
	} else if (num_vsns == 0) {
		fprintf(stderr,
		    "Warning: sam_segment_stat of segment %d of file %s "
		    "revealed that\n         copy %d currently utilizes no"
		    " vsns.  Call to\n         sam_segment_vsn_stat will "
		    "fail.\n", segment, pathname, copy);
	} else if (num_vsns == 1) {
		fprintf(stderr,
		    "Warning: sam_segment_stat of segment %d of file %s "
		    "revealed that\n         copy %d uses only one vsn.  "
		    "Call to\n         sam_segment_vsn_stat will fail.\n",
		    segment, pathname, copy);
	} else {
		fprintf(stderr,
		    "Detail: Call to sam_segment_stat indicates that copy"
		    " %d of\n        segment %d of file %s uses %d vsns."
		    "\n", copy, segment, pathname, num_vsns);
	}

	memset((void *)vsns, (char) 0, sizeof (struct sam_section) *
	    MAX_VOLUMES);

	error = sam_segment_vsn_stat(pathname, copy, segment, vsns,
	    MAX_VOLUMES * sizeof (struct sam_section));

	if (error) {
		fprintf(stderr,
		    "Error: Call to sam_segment_vsn_stat(%s, %d, %d, ...)"
		    " failed, call returned %d.\n",
		    pathname, copy, segment, error);
		fprintf(stderr, "       errno set to %d.\n", errno);
		free(data_seg_stat_buf);

		exit(error);
	} else {
		fprintf(stderr,
		    "Detail: Call to sam_segment_vsn_stat(%s, %d, %d, ...)"
		    " returned and no error was indicated.\n",
		    pathname, copy, segment);
	}

	fprintf(stdout, "%s: segment %d: copy: %d\n", pathname, segment, copy);

	for (sec = 0; sec < num_vsns; sec++) {
		fprintf(stdout,
		    "    section %d: length: %10lld, position: %10llx,\n"
		    "                 offset: %-4llx, vsn: %s\n",
		    sec, vsns[sec].length, vsns[sec].position,
		    ((vsns[sec].offset)/512), vsns[sec].vsn);
	}

	free(data_seg_stat_buf);

	return (0);
}
#endif
