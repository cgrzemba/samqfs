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
 * scqfs_stop.c - Stop method for SUNW.qfs
 * No work is done here this is just a
 * place holder. We leave the Metadata server
 * running on this node. When Start is launched
 * on some other node, we takeover the metadata
 * server from that other node.
 */

#pragma ident "$Revision: 1.9 $"

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

	program_name = basename(argv[0]);
	CustmsgInit(0, NULL);

	/* Process arguments passed by RGM and initialize syslog */
	if (GetRgInfo(argc, argv, &rg) < 0) {
		return (1);
	}

	RelRgInfo(&rg);

	/* Return Success */
	return (0);
}
