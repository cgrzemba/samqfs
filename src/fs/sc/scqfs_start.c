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
 * scqfs_start.c - Start method for SUNW.qfs
 *
 * This is just a place holder.  This method only checks to see
 * that the RG's FSes are mounted; most of the real startup work
 * is done in scqfs_prenet_start.
 */

#pragma ident "$Revision: 1.11 $"

#include <strings.h>
#include <libgen.h>

#include <stdlib.h>
#include <rgm/libdsdev.h>

#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"

#include "scqfs.h"

static int MasterFS(int, char **, struct FsInfo *);


int
main(int argc, char *argv[])
{
	struct RgInfo  rg;
	struct FsInfo *fi;
	int rc = 0;


	program_name = basename(argv[0]);
	CustmsgInit(0, NULL);

	/* Process arguments passed by RGM and initialize syslog */
	if (GetRgInfo(argc, argv, &rg) < 0) {
		return (1);
	}

	if ((fi = GetFileSystemInfo(&rg)) == NULL) {
		return (1);
	}

	RelRgInfo(&rg);

	rc = ForkProcs(argc, argv, fi, MasterFS);

	FreeFileSystemInfo(fi);

	return (rc);
}


/*
 * MasterFS
 *
 * Called for each filesystem in the resource group.
 * Verify that this host is the MDS for the FS, and that
 * we can stat(2) <mntpath>/.inodes.
 */
static int
MasterFS(
	int argc,
	char **argv,
	struct FsInfo *fp)
{
	int l, rc = 0;
	char *master = NULL;
	struct RgInfo rg;
	char cmd[2048];

	if (!fp->fi_fs || !fp->fi_mntpt) {
		return (1);
	}

	if (GetRgInfo(argc, argv, &rg) < 0) {
		return (1);
	}

	if (get_serverinfo(fp->fi_fs, NULL, &master, NULL) < 0 ||
	    master == NULL) {
		return (1);		/* error logged by get_serverinfo */
	}

	if (strcasecmp(master, rg.rg_nodename)) {
			/* 22670 */
			scds_syslog(LOG_NOTICE,
			    "%s: This node is not the metadata server. "
			    "Current metadata server is %s.",
			    fp->fi_fs, master);
			return (1);
	}

	l = snprintf(cmd, sizeof (cmd), "/bin/ls -l %s/.inodes", fp->fi_mntpt);
	if (l >= sizeof (cmd)) {
		scds_syslog(LOG_ERR,
		    "%s: Mount pathname too long", fp->fi_fs);
		return (1);
	}
	rc = run_system(LOG_ERR, cmd);

	free(master);
	RelRgInfo(&rg);

	return (rc);
}
