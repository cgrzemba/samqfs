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
 * scqfs_probe.c - Probe for QFS Metadata Server
 * Periodically checks to see if the node the program is run on is the
 * metadata server and if so, does a "ls -l $mountpoint/.inodes" under
 * a timeout to verify the filesystem is mounted and accessible.
 */

#pragma ident "$Revision: 1.13 $"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/sysmacros.h>

#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"

#include <rgm/libdsdev.h>

#include "scqfs.h"


int
main(int argc, char *argv[])
{
	struct RgInfo  rg;
	struct FsInfo *fi;
	scha_err_t err;
	int		i, rc;
	int		fscount;

	int		timeout, probe_interval, retry_interval;
	int		probe_result;
	int		probe_failure_1, probe_failure_2, probe_failure_3;

	char	**cmds;

	long		dt;
	hrtime_t	ht1, ht2;


	program_name = basename(argv[0]);
	CustmsgInit(0, NULL);

	if (GetRgInfo(argc, argv, &rg) < 0) {
		return (1);
	}

	if ((fi = GetFileSystemInfo(&rg)) == NULL) {
		return (1);
	}

	/*
	 * Get retry, probe, and timeout intervals
	 */
	retry_interval = scds_get_rs_retry_interval(rg.rg_scdsh);
	probe_interval = scds_get_rs_thorough_probe_interval(rg.rg_scdsh);
	timeout = scds_get_ext_probe_timeout(rg.rg_scdsh);

	fscount = rg.rg_mntpts->array_cnt;
	cmds = (char **)malloc(fscount * sizeof (char *));
	if (cmds == NULL) {
		/* 22602 */
		scds_syslog(LOG_ERR, "Out of Memory");
		return (1);
	}

	for (i = 0; i < fscount; i++) {
		int l;
		char cmd[MAXPATHLEN+64];

		l = snprintf(cmd, sizeof (cmd),
		    "/bin/ls -l %s/.inodes", fi[i].fi_mntpt);
		if (l >= sizeof (cmd)) {
			scds_syslog(LOG_ERR,
			    "%s: mount pathname too long", fi[i].fi_fs);
			return (1);
		}
		cmds[i] = strdup(cmd);
	}

	/* Suppress output of ls */
	closefds();

	/*
	 * Probe failure levels
	 *
	 * probe_failure_2 assumes at least 3 probes will run within
	 * the USER DEFINED retry_interval
	 */
	probe_failure_1 =
	    howmany(SCDS_PROBE_COMPLETE_FAILURE,
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

		/* Filesystems check */
		for (i = 0; i < fscount; i++) {
			char *master;

			rc = 0;
			master = NULL;
			/*
			 * If we can't get a metadata server, fail over
			 * the resource - something is seriously wrong.
			 */
			if (get_serverinfo(fi[i].fi_fs, NULL,
			    &master, NULL) < 0) {
				probe_result += probe_failure_3;
				break; /* Errors logged in get_serverinfo() */
			}

			/*
			 * Validate master is this node
			 *
			 * Use probe_failure_1 weighting, this allows the most
			 * failures to occur before a retry_interval expires
			 * and triggers a failover.
			 *
			 * This error is cumulative with the checks that follow.
			 */
			if (master == NULL) {
				/* 22649 */
				scds_syslog(LOG_WARNING,
				    "%s: Metadata server not found.",
				    fi[i].fi_fs);
				probe_result = probe_failure_3;
				break;
			}

			if (strcasecmp(master, rg.rg_nodename)) {
				/* 22666 */
				scds_syslog(LOG_WARNING,
				    "%s: This node is not the metadata server. "
				    "Current metadata server: %s.",
				    fi[i].fi_fs, master);
				free(master);
				probe_result = probe_failure_3;
				break;
			}

			/* Free memory allocated by get_serverinfo() */
			free(master);

			/*
			 * Invoke ls command and weight errors.
			 *
			 * A timeout of the ls command weights at
			 * probe_failure_2, allowing 3 or less of these
			 * errors to occur before failing the probe.
			 *
			 * A cluster detected error of the ls command
			 * (specified in the cluster docs?) uses
			 * probe_failure_1, allowing as many of these
			 * failures as can fit in a retry_interval
			 *
			 * A failure of the ls command uses probe_failure_3
			 * which triggers an immediate failover.
			 *
			 * Note: The filesystem loop is broken below
			 * if the ls fails with an error code.
			 */
			err = scds_timerun(rg.rg_scdsh, cmds[i],
			    timeout, SIGKILL, &rc);

			if (err == SCHA_ERR_TIMEOUT) {
				/* 22667 */
				scds_syslog(LOG_WARNING,
				    "%s: Cluster timed out probe command.",
				    fi[i].fi_fs);
				probe_result += probe_failure_2;
			} else if (err != SCHA_ERR_NOERR) {
				/* 22668 */
				scds_syslog(LOG_WARNING,
				    "%s: Cluster detected probe failure: %s.",
				    fi[i].fi_fs, scds_error_string(err));
				probe_result += probe_failure_1;
			} else if (rc != 0) {
				/* 22669 */
				scds_syslog(LOG_ERR, "%s: Failure accessing "
				    "filesystem. Command: %s, Error: %s",
				    fi[i].fi_fs, cmds[i], strerror(rc));
				probe_result = probe_failure_3;
				break;
			}
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
}
