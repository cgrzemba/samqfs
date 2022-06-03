/*
 * load.c
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

#pragma ident "$Revision: 1.22 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <libgen.h>
#include <string.h>
#include <errno.h>

#define	DEC_INIT

#include "sam/types.h"
#include "sam/param.h"
#include "aml/types.h"
#include "aml/proto.h"
#include "pub/sam_errno.h"
#include "aml/samapi.h"
#include "aml/catalog.h"
#include "sam/lib.h"
#include "aml/catlib.h"
#include "sam/custmsg.h"
#include "sam/lint.h"
#include "aml/shm.h"

/* globals */
shm_alloc_t              master_shm, preview_shm;

static void Usage(void);

int
main(int argc, char **argv)
{
	int	wait_time = 0;
	int	eq = -1;
	int	err = 0;
	int	volarg;
	char	c;
	struct	VolId    vid;
	struct	CatalogEntry ced;
	struct	CatalogEntry *ce = &ced;

	program_name = basename(argv[0]);
	CustmsgInit(0, NULL);

	if (CatalogInit(program_name) == -1) {
		fprintf(stderr, "%s\n", GetCustMsg(2364));
				/*  Catalog initialization failed! */
		exit(EXIT_FAILURE);
	}

	if (argc < 2 || argc > 4) {	/* catch the obvious */
		Usage();
		/* NOTREACHED */
	}

	/*
	 * Process arguments.
	 */
	while ((c = getopt(argc, argv, "w")) != EOF) {
		switch (c) {
		case 'w':
			wait_time = 120;
			break;

		default:
			Usage();
		}
	}

	volarg = optind;

	/*
	 * Convert specifier to volume identifier.
	 */
	if (StrToVolId(argv[volarg], &vid) != 0) {
		error(EXIT_FAILURE, errno,
		    catgets(catfd, SET, 18207,
		    "Volume specification error %s:"),
		    argv[volarg]);
	}

	/*
	 * Use CatalogGetEntry() to validate the specifier.
	 * At this point vid.ViEq is the catalog eq ordinal.
	 */
	if ((ce = CatalogGetEntry(&vid, &ced)) == NULL) {
		error(EXIT_FAILURE, errno,
		    catgets(catfd, SET, 18207,
		    "Volume specification error %s:"),
		    argv[volarg]);
	}

	/* get optional unavail'ed drive equipment number */

	if ((argc - optind) > 1) {
		/* eq is the ordinal of the unavailable drive */
		if ((eq = atoi(argv[++optind])) < 0) {
			error(EXIT_FAILURE, errno, catgets(catfd, SET, 13003,
			    "Invalid equipment ordinal (%s)"), argv[optind]);
		}
	}
	else
	{
		/* eq is the ordinal of the library */
		eq = ce->CeEq;
	}

	err = sam_load((ushort_t)eq, ce->CeVsn, ce->CeMtype, ce->CeSlot,
	    ce->CePart, wait_time);

	if (err) {
		fprintf(stderr,
		    catgets(catfd, SET, 13204,
		    "%s error: %s\n"), program_name, sam_errno(errno));
	}

	return (err);
}

static void
Usage()
{
	fprintf(stderr,
	    catgets(catfd, SET, 13001,
	    "Usage: %s %s\n"),
	    program_name, " [-w] eq:slot[:part] | media.vsn [deq]");
	exit(1);
	/* NOTREACHED */
}
