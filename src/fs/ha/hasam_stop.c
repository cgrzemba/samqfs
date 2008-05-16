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

/*
 * hasam_stop.c - Stop method for SUNW.hasam
 *
 * This method will offline the available drives, stops archiver &
 * stager, stops samd.
 */

#include <strings.h>
#include <libgen.h>

#include <stdlib.h>
#include <unistd.h>
#include <rgm/libdsdev.h>

#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"

#include "hasam.h"

static int hasamStopWorker(int, char **, struct FsInfo_4hasam *);

int
main(int argc, char *argv[])
{

	struct RgInfo_4hasam  rg;
	struct FsInfo_4hasam *fi;
	int rc;

	/*
	 * Set debug to print messages to stderr
	 */
	hasam_debug = 0;

	/*
	 * Process arguments passed by RGM and initialize syslog.
	 */
	program_name = basename(argv[0]);
	CustmsgInit(0, NULL);

	if (GetRgInfo_4hasam(argc, argv, &rg) < 0) {
		return (1);
	}

	/*
	 * Allocate space for the FsInfo array and clear it.
	 */
	if ((fi = GetFileSystemInfo_4hasam(&rg)) == NULL) {
		return (1);
	}

	/*
	 * Close the open ds/cluster/rg handles we have open.
	 */
	RelRgInfo_4hasam(&rg);

	/*
	 * We've got the FS names, their mount points, now
	 * we need to fork/thread off procs for each FS.
	 */
	rc = ForkProcs_4hasam(argc, argv, fi, hasamStopWorker);

	/* Offline tape drives */
	stopAccess2Tapes();

	FreeFileSystemInfo_4hasam(fi);
	return (rc);
}

/*
 * hasamStopWorker
 *
 * Called for each filesystem in the resource group to stop archiving
 * and staging activities. Then offline tape drives and stop sam-amld,
 * archiver & stager daemons.
 */
static int
hasamStopWorker(
	int argc,
	char **argv,
	struct FsInfo_4hasam *fp)
{
	boolean_t archiver_stopped;
	boolean_t stager_stopped;

	if (!fp->fi_fs || !fp->fi_mntpt) {
		return (1);
	}

	if (check_sam_daemon("sam-archiverd")) {
		/* Stop archiver first */
		archiver_stopped = stop_archiver();
		if (archiver_stopped == B_FALSE) {
			dprintf(stderr, "Error stopping archiver !\n");
		}
	}

	return (0);
}
