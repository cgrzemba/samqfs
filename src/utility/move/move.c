/*
 * move.c
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.14 $"

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/* Solaris headers. */
#include <libgen.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/exit.h"
#include "sam/lib.h"
#include "aml/samapi.h"
#include "sam/lint.h"

/* Private functions. */
static void MsgFunc(int code, char *msg);

/* Public data. */
char *program_name;	/* Program name: used by error */


int
main(int argc, char **argv)
{
	char	*endptr;
	int	dest_slot;

	program_name = basename(argv[0]);
	if (argc != 3) {
		fprintf(stderr, GetCustMsg(13001),
		    program_name, "eq:slot slot");
		fprintf(stderr, "       %s mediatype.vsn slot\n",
		    program_name);
		exit(EXIT_USAGE);
	}

	/*
	 * Set destination slot.
	 */
	dest_slot = strtol(argv[2], &endptr, 10);
	if (endptr == argv[2] || *endptr != '\0') {
		error(EXIT_FAILURE, 0, GetCustMsg(13630));
	}

	if (SamMoveCartridge(argv[1], dest_slot, 0, MsgFunc) == -1) {
		error(EXIT_FAILURE, errno, "SamMoveCartridge failed");
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
