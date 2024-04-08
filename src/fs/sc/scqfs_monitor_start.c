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
 * or https://illumos.org/license/CDDL.
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
 * scqfs_monitor_start.c - Monitor_start method for SUNW.qfs
 * Starts up /opt/SUNWsamfs/sc/bin/scqfs_probe under PMF
 */

#pragma ident "$Revision: 1.8 $"

#include <libgen.h>
#include <rgm/libdsdev.h>

#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"

#include "scqfs.h"

int
main(int argc, char *argv[])
{
	struct RgInfo rg;
	int	rc;

	program_name = basename(argv[0]);
	CustmsgInit(0, NULL);

	/* Process arguments passed by RGM and initialize syslog */
	if (GetRgInfo(argc, argv, &rg) < 0) {
		return (1);
	}

	rc = mon_start(&rg);

	RelRgInfo(&rg);

	return (rc);
}
