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
 * hasam_fini.c - FINI method for SUNW.hasam
 *
 * Called when the resource (of type SUNW.hasam) is deleted from the system.
 * This happens on ALL nodes of the Cluster where the SAM resource
 * can fail over.
 */

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>
#include <rgm/libdsdev.h>

#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"

#include "hasam.h"

static int hasamFiniWorker(void);


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
	 * Stop sam-amld, archiver & stager daemons if running
	 */
	rc = hasamFiniWorker();

	FreeFileSystemInfo_4hasam(fi);

	/* Return Success/failure status */
	return (rc);
}


/*
 * Worker for fini. Ensure archiver & stager daemons are
 * NOT running: sam-amld, sam-archiverd, sam-stagerd.
 * The stop method should have taken care of stopping the
 * tape drives.
 */
static int
hasamFiniWorker()
{
	boolean_t amld_status;
	boolean_t archiver_status;
	boolean_t stager_status;
	boolean_t stageall_status;

	/* Check if archiver & stager daemons are still running */
	archiver_status = check_sam_daemon("sam-archiverd");
	stager_status = check_sam_daemon("sam-stagerd");
	stageall_status = check_sam_daemon("sam-stagealld");

	dprintf(stderr, "Arch=%d, Stg=%d, Stgall=%d\n",
	    archiver_status, stager_status, stageall_status);

	if ((archiver_status) || (stager_status) || (stageall_status)) {
		/* This step is supposed to stop archiver & stager daemons */
		samd_hastop();
		sleep(samd_timer);
	}

	/* Check if sam-amld daemon is running, if yes stop it */
	amld_status = check_sam_daemon("sam-amld");
	if (amld_status) {
		samd_stop();
	}

	/*
	 * Can't do much if the daemons don't stop, so return success
	 * and the SamQfs administrator should rectify the problem
	 */
	return (0);
}
