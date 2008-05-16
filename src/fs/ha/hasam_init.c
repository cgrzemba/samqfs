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
 * hasam_init.c - BOOT/INIT method for SUNW.hasam
 *
 * Called when the SAM Server resource (of type SUNW.hasam) is
 * created for the very first time.
 * This happens on ALL nodes of the Cluster where the SAM resource
 * can fail over.
 */

#include <strings.h>
#include <unistd.h>

#include <libgen.h>
#include <rgm/libdsdev.h>

#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"

#include "hasam.h"

static int hasamInitWorker(int, char **, struct FsInfo_4hasam *);

int
main(
	int argc,
	char *argv[])
{
	struct RgInfo_4hasam	rg;
	struct FsInfo_4hasam	*fi;
	int				rc;

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
	 * Do all the basic checks required for HA-SAM.
	 */
	if (!GetSamInfo()) {
		return (1);
	}

	/*
	 * Check if sam-fsd daemon is running
	 */
	if (!is_sam_fsd()) {
		return (1);
	}

	/*
	 * Close the open ds/cluster/rg handles we have open.
	 */
	RelRgInfo_4hasam(&rg);

	/*
	 * We've got the FS names & their mount points. Now
	 * we need to fork/thread off procs for each FS.
	 * Each thread needs to check if FS is mounted and mount
	 * if if not.
	 */
	rc = ForkProcs_4hasam(argc, argv, fi, hasamInitWorker);

	FreeFileSystemInfo_4hasam(fi);
	return (rc);
}


/*
 * Worker thread for init to make sure the QFS is mounted, if not
 * attempt to mount it about 2 times and then give up.
 *
 * Return: success = 0 [or] failure = 1
 */
static int
hasamInitWorker(
	int argc,
	char **argv,
	struct FsInfo_4hasam *fp)
{
	int i, rc;
	boolean_t mnt_status;

	if (!fp->fi_fs || !fp->fi_mntpt) {
		return (1);
	}

	for (i = 0; i < 2; i++) {
		/* Check if the given QFS is mounted */
		mnt_status = check_fs_mounted(fp->fi_fs);
		if (mnt_status == B_FALSE) {
			/* FS not mounted, attempt to mount the QFS */
			if ((rc = mount_fs_4hasam(fp->fi_fs)) != 0) {
				/* 22685 */
				scds_syslog(LOG_ERR,
				    "%s: Error mounting %s.",
				    fp->fi_fs, fp->fi_mntpt);
			} else {
				sleep(3);
			}
		} else {
			break;
		}
	}

	if (mnt_status == B_FALSE) {
		dprintf(stderr, "Error FS not mounted %s !\n", fp->fi_fs);
		return (1);
	}

	return (rc == 0 ? 0 : 1);
}
