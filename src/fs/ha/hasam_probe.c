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
 * hasam_probe.c - Probe for SAM Server
 * Periodically checks to see if the node the program is run on has
 * essential daemons running, the catalog is available and QFS
 * filesystem is mounted under a timeout.
 */

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/sysmacros.h>

#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"

#include <rgm/libdsdev.h>

#include "hasam.h"


int
main(int argc, char *argv[])
{
	struct RgInfo_4hasam  rg;
	struct FsInfo_4hasam *fi;

	int		probe_interval, retry_interval;
	int		probe_result;
	int		probe_failure_1, probe_failure_2, probe_failure_3;

	long		dt;
	hrtime_t	ht1, ht2;

	boolean_t ismounted;

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
	 * Get retry, probe, and timeout intervals
	 */
	retry_interval = scds_get_rs_retry_interval(rg.rg_scdsh);
	probe_interval = scds_get_rs_thorough_probe_interval(rg.rg_scdsh);

	/*
	 * Probe failure levels
	 *
	 * probe_failure_2 assumes at least 3 probes will run within
	 * the USER DEFINED retry_interval
	 */
	probe_failure_1 = howmany(SCDS_PROBE_COMPLETE_FAILURE,
	    (retry_interval / probe_interval));
	probe_failure_2 = howmany(SCDS_PROBE_COMPLETE_FAILURE, 3);
	probe_failure_3 = SCDS_PROBE_COMPLETE_FAILURE;

	/*
	 * Since the weighting for probe_failure_1 is the smallest weighting
	 * that can fit in the retry_interval window, if probe_failure_2 is
	 * less than this, then probe_failure_2 would never cause a failover.
	 *
	 * Therefore, to correct the problem set probe_failure_2 equal to
	 * probe_failure_1.
	 *
	 * In effect this means that if 3 probes will not fit in the
	 * retry_interval, then set it to match whatever was computed
	 * for probe_failure_1.
	 */
	probe_failure_2 = MAX(probe_failure_1, probe_failure_2);

	for (;;) {
		probe_result = 0;

		/* Start time */
		ht1 = gethrtime();

		if (!probe_hasam_config()) {
			probe_result += probe_failure_3;
		}

		/* Filesystems check */
		ismounted = check_all_fs_mounted(fi);
		if (ismounted == B_FALSE) {
			probe_result = probe_failure_3;
			break;
		}

		/* Stop time */
		ht2 = gethrtime();

		/* Convert to milliseconds */
		dt = (long)((ht2 - ht1) / 1e6);

		/* Compute failure history and take action if needed */
		(void) scds_fm_action(rg.rg_scdsh, probe_result, dt);

		/* sleep for probe_interval */
		(void) scds_fm_sleep(rg.rg_scdsh, probe_interval);
	}

	/*
	 * Close the open ds/cluster/rg handles we have open.
	 */
	RelRgInfo_4hasam(&rg);
	FreeFileSystemInfo_4hasam(fi);

	/* Detected filesystem unmounted condition */
	if (ismounted == B_FALSE) {
		return (1);
	}
	return (0);
}
