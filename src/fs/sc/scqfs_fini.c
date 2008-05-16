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
 * scqfs_fini.c - FINI method for SUNW.qfs
 *
 * Called when the Metadata Server resource (of type SUNW.qfs)
 * is deleted from the system.  Unmounts the specified mountpoint.
 * This happens on ALL nodes of the Cluster where the QFS resource
 * can failover (i.e. the Nodelist of the Resource Group which
 * contains the Metadata server resource).
 */

#pragma ident "$Revision: 1.15 $"

#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <rgm/libdsdev.h>

#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"

#include "scqfs.h"

static int UmountFS(int argc, char **argv, struct FsInfo *fp);


int
main(int argc, char *argv[])
{
	struct RgInfo  rg;
	struct FsInfo *fi;
	int rc = 0;

	program_name = basename(argv[0]);
	CustmsgInit(0, NULL);

	if (GetRgInfo(argc, argv, &rg) < 0) {
		return (1);
	}

	if ((fi = GetFileSystemInfo(&rg)) == NULL) {
		return (1);
	}

	RelRgInfo(&rg);

	rc = RevDepForkProcs(argc, argv, fi, UmountFS);

	FreeFileSystemInfo(fi);

	/* Return Success/failure status */
	return (rc);
}


static int
UmountFS(int argc,
		char **argv,
		struct FsInfo *fp)
{
	struct RgInfo rg;

	if (!fp->fi_fs || ! fp->fi_mntpt) {
		return (1);
	}
	if (GetRgInfo(argc, argv, &rg) < 0) {
		return (1);
	}

	if (GetMdsInfo(&rg, fp) < 0) {
		return (1);
	}

	if (validate_mounted(fp->fi_mntpt, B_FALSE)) {
		if (umount_qfs(&rg, fp) != 0) {
			/* 22686 */
			scds_syslog(LOG_ERR,
			    "Error attempting to unmount %s.", fp->fi_fs);
			return (1);
		}
	}
	return (0);
}
