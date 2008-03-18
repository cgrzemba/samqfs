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

/*
 * hasam_validate.c - validate method for SUNW.hasam
 *
 * Check to make sure that the cluster node is set up
 * correctly for SAM failover.
 */

#include <stdio.h>
#include <libgen.h>
#include <rgm/libdsdev.h>

#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"

#include "hasam.h"

int
main(int argc, char *argv[])
{
	struct RgInfo_4hasam  rg;
	struct FsInfo_4hasam *fi;

	/*
	 * Override to default behavior - validate prints messages to stderr
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
	FreeFileSystemInfo_4hasam(fi);
	return (0);
}
