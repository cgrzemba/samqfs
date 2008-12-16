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
 * scqfs_validate.c - validate method for SUNW.qfs
 *
 * Check to make sure that the cluster node is set up correctly
 * correctly for MetaDataServer failover.
 */

#pragma ident "$Revision: 1.8 $"

#include <stdio.h>
#include <libgen.h>
#include <rgm/libdsdev.h>

#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"

#include "scqfs.h"

int
main(int argc, char *argv[])
{
	struct RgInfo  rg;
	struct FsInfo *fi;
	int rc;


	program_name = basename(argv[0]);
	CustmsgInit(0, NULL);

	/* Override to default behavior - validate prints messages to stderr */
	doprint = 1;

	if (GetRgInfo(argc, argv, &rg) < 0) {
		return (1);
	}

	if ((fi = GetFileSystemInfo(&rg)) == NULL) {
		return (1);
	}

	rc = svc_validate(&rg, fi);

	RelRgInfo(&rg);

	FreeFileSystemInfo(fi);

	return (rc);
}
