/*
 * export.c
 *
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

#pragma ident "$Revision: 1.20 $"

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Solaris headers. */
#include <libgen.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/exit.h"
#include "sam/lib.h"
#include "sam/lint.h"
#include "aml/samapi.h"

/* Private functions. */
static void MsgFunc(int code, char *msg);

/* Public data. */
char *program_name;		/* Program name: used by error */

#define	USAGE_STRING "[-f] [eq:slot | media.vsn]"


int
main(int argc, char **argv)
{
	int one_step = 0;
	int WaitResponse = 0;

	program_name = basename(argv[0]);
	argc --;
	argv ++;

	if (argc > 2 || argc < 1) {
		fprintf(stderr, GetCustMsg(13001), program_name, USAGE_STRING);
		exit(EXIT_USAGE);
	}

	if (argc > 0 && (strcmp(*argv, "-f") == 0)) {
		one_step = TRUE;
		argc --;
		argv ++;
	} else if (argc > 0 && **argv == '-') {
		fprintf(stderr, GetCustMsg(13001), program_name, USAGE_STRING);
		exit(EXIT_USAGE);
	}

	if (argc < 1) {
		fprintf(stderr, GetCustMsg(13001), program_name, USAGE_STRING);
		exit(EXIT_USAGE);
	}

	if (SamExportCartridge(*argv, WaitResponse, one_step, MsgFunc) == -1) {
		error(EXIT_FAILURE, errno, "SamExportCartridge failed");
	}
	return (EXIT_SUCCESS);
}


/*
 * Error message function.
 */
static void
MsgFunc(int code, char *msg)
{
	error(code, 0, msg);
}
