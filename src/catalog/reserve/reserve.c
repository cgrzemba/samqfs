/*
 * reserve - Reserve a volume for archiving.
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

#pragma ident "$Revision: 1.16 $"

/* ANSI C headers. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* SAM-FS headers. */
#define DEC_INIT
#include "sam/custmsg.h"
#include "sam/exit.h"
#include "sam/lib.h"
#include "aml/samapi.h"
#include "sam/lint.h"
#include "aml/shm.h"

/* globals */
shm_alloc_t              master_shm, preview_shm;

/* Private functions. */
static void MsgFunc(int code, char *msg);

int
main(
	int argc,	/* Number of arguments */
	char *argv[])	/* Argument pointer list */
{
	time_t rtime = 0;

	program_name = "reserve";
	if (argc < 3 || argc > 4) {
		fprintf(stderr, GetCustMsg(13001), program_name,
		    "mediatype.vsn asname/owner/fsname [time]\n");
			exit(EXIT_USAGE);
	}
	if (argc > 3) {
		rtime = StrToTime(argv[3]);
		if (rtime == -1) {
			fprintf(stderr, "Cannot convert time %s\n", argv[3]);
			exit(EXIT_FAILURE);
		}
	}
	if (SamReserveVolume(argv[1], rtime, argv[2], MsgFunc) == -1) {
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}


/*
 * Error message function.
 */
static void
MsgFunc(
	int code,
	char *msg)
{
	fprintf(stderr, "%s: %s:%d\n", program_name, msg, code);
}
