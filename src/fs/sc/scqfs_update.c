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
 * scqfs_update.c - Update method for SUNW.qfs
 * Called whenever any properties are changed.
 * Restart the fault monitor as all the properties
 * which are dynamically updatable are fault monitor
 * properties.
 */

#pragma ident "$Revision: 1.7 $"

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

	if (GetRgInfo(argc, argv, &rg) < 0) {
		return (1);
	}

	/* There is only one fault monitor, index 0 */
	rc = scds_pmf_restart_fm(rg.rg_scdsh, 0);

	RelRgInfo(&rg);

	return (rc);
}
