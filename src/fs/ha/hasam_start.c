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

/*
 * hasam_start.c - Start method for SUNW.hasam
 *
 * This method starts samd and the tape drives that are essential
 * for SAM operation. It also makes sure the samfsinfo command is
 * successful for the given QFS filesystem.
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

static int hasamStartWorker(int, char **, struct FsInfo_4hasam *);
static int checkAccess2Tapes();

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
	 * We've got the FS names, their mount points, and
	 * the list of nodes that can serve.  Now
	 * we need to fork/thread off procs for each FS.
	 */
	rc = ForkProcs_4hasam(argc, argv, fi, hasamStartWorker);

	/* Create a run file hasam_running */
	make_run_file();

	/*
	 * Enable access to all tape drives for use by SAM
	 */
	if (checkAccess2Tapes() != 0) {
		delete_run_file();
		return (1);
	}

	FreeFileSystemInfo_4hasam(fi);
	return (rc);
}

/*
 * Called for each filesystem in the resource group.
 *
 * Return: success = 0 [or] failure = 1
 */
static int
hasamStartWorker(
	int argc,
	char **argv,
	struct FsInfo_4hasam *fp)
{
	int i, rc = 0;
	boolean_t samfsinfo_status;
	boolean_t mnt_status;
	boolean_t samd_status;

	if (!fp->fi_fs || !fp->fi_mntpt) {
		return (1);
	}

	return (0);
}

/*
 * Enable access to all tape drives by running a samd start,
 * turn ON tape drives and start archiver.
 */
static int
checkAccess2Tapes()
{
	struct media_info *mout;
	boolean_t daemon_status;
	boolean_t drives_online;
	boolean_t catalog_status;
	boolean_t archiver_started;
	boolean_t samd_status;
	int rc = 1;
	int i;

	/*
	 * Do all the basic checks required for HA-SAM.
	 */
	if (!GetSamInfo()) {
		return (1);
	}

	/* Run samd config command */
	samd_status = samd_config();
	if (!samd_status) {
		samd_status = samd_config();
		if (!samd_status) {
			return (1);
		}
	}
	sleep(samd_timer);

	/* Start sam-amld daemon */
	samd_status = samd_start();
	sleep(samd_timer);
	if (!samd_status) {
		samd_status = samd_start();
		sleep(samd_timer);
		if (!samd_status) {
			return (1);
		}
	}

	/*
	 * Check if atleast one tape library with drive is
	 * configured in mcf, if not there is a possibility
	 * no library is configured and the archiving is
	 * limited to disk archives.
	 * So don't return error if no library is detected.
	 */
	if (!tape_lib_drv_avail) {
		/*
		 * Check if sam-fsd daemon is running
		 */
		if (!is_sam_fsd()) {
			return (1);
		}
		return (0);
	}

	/*
	 * Tape library is configured in mcf,
	 * so validate essential SAM daemons
	 *
	 * In case of network attached libraries, starting
	 * of daemons sam-amld and sam-catserverd may take
	 * a bit longer. So give some extra time and retry
	 * for 3 times before returning error from check for
	 * essential daemons.
	 */
	for (i = 0; i < 3; i++) {
		daemon_status = verify_essential_sam_daemons();
		if (daemon_status) {
			break;
		} else {
			if (i >= 2) {
				return (1);
			}
			if (!check_sam_daemon("sam-amld")) {
				samd_start();
			}
		}
		sleep(samd_timer);
	}

	/*
	 * Since sam-amld is running, we must be able to
	 * get library related information. So collect library
	 * and drive information configured for SAM
	 */
	if ((mout = collect_media_info()) == NULL) {
		dprintf(stderr, "Tape library may not be configured\n");
		rc = 1;
		goto finished;
	}

	/* Make sure catalog of all configured libraries are available */
	catalog_status = check_catalog(mout);

	/* Turn ON all the drives configured for SAM */
	drives_online = make_drives_online(mout);
	sleep(drive_timer);

	/* Start archiver */
	archiver_started = start_archiver();

	if (daemon_status && catalog_status && drives_online &&
	    archiver_started) {
		dprintf(stderr, "Daemons %d, Cat %d, Drv %d, arch %d\n",
		    daemon_status, catalog_status,
		    drives_online, archiver_started);
		rc = 0;
	}

finished:
	free_media_info(mout);
	return (rc);
}
