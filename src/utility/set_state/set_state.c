/*
 *	set_state.c
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

#pragma ident "$Revision: 1.22 $"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#define	DEC_INIT

#include "sam/types.h"
#include "sam/param.h"
#include "pub/devstat.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "sam/custmsg.h"
#include "sam/devnm.h"
#include "aml/samapi.h"
#include "pub/sam_errno.h"
#include "aml/shm.h"

/* globals */
shm_alloc_t              master_shm, preview_shm;

static void Usage();

int
main(int argc, char **argv)
{
	int eq, err, i;
	char *state;
	dstate_t new_state;

	CustmsgInit(0, NULL);

	if (argc < 3 || argc > 4) {	/* catch the obvious */
		Usage();
		exit(1);
	}
	argv++;
	if (argc == 4) {
		/* Ignore -w option. */
		if (strcmp(*argv, "-w") != 0) {
			Usage();
			exit(1);
		}
		argv++;
	}
	state = *argv;

	for (i = 0; dev_state[i] != NULL; i++) {
		if (strcasecmp(state, dev_state[i]) == 0)
			break;
	}

	if (strcasecmp(state, "DOWN") == 0) {
		fprintf(stderr, catgets(catfd, SET, 2322,
		    "set_state: Cannot down device, can only off device.\n"));
		exit(1);
	}
	if (dev_state[i] == NULL) {
		fprintf(stderr,
		    catgets(catfd, SET, 2321, "set_state: Unknown state %s.\n"),
		    state);
		exit(1);
	}
	new_state = (dstate_t)i;

	argv++;
	eq = atoi(*argv);

	err = sam_set_state((ushort_t)eq, new_state, 120);
	if (err) {
		fprintf(stderr,
		    catgets(catfd, SET, 13206,
		    "set_state error: %s\n"), sam_errno(errno));
	}
	return (err);

}

static void
Usage()
{
	fprintf(stderr,
	    catgets(catfd, SET, 13202, "Usage: set_state [-w] state eq\n"));
}
