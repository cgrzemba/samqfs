/*
 * samadm servicetag command
 */

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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#include "samadm.h"
#include <sam/names.h>

#pragma ident "$Revision: 1.3 $"


int
servicetag_cmd(
	int		argc,		/* Operand arg count */
	char		*argv[],	/* Operand arg vector pointer */
	cmdOptions_t	*options,	/* Pointer to parsed option flags */
	void		*callData)	/* Void data (unused) from main */
{
	if (strcmp(argv[0], "add") == 0) {
		if (system(SAM_UTILITY_PATH "/samservicetag add") < 0) {
			perror("samadm: could not execute samservicetag");
			return (1);
		}
	} else if (strcmp(argv[0], "delete") == 0) {
		if (system(SAM_UTILITY_PATH "/samservicetag delete") < 0) {
			perror("samadm: could not execute samservicetag");
			return (1);
		}
	} else {
		fprintf(stderr, "%s: %s: invalid option(%s)\n",
		    cmd_name, subcmd_name, argv[0]);
		return (1);
	}
	return (0);
}
