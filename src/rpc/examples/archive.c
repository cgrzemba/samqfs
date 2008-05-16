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
 * archive.c - example program using archive.
 *
 * Use archive() to set archive attributes on a SAM-FS file.
 * This program may be compiled for use locally or as an rpc client.
 *
 */

#pragma ident "$Revision: 1.8 $"

/* Feature test switches. */
/* REMOTE Use rpc for access. */

/* ANSI C headers. */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/fcntl.h>

/* SAM-FS headers. */
#include "lib.h"
#include "stat.h"
#include "samrpc.h"

/* Private data. */
static struct sam_stat sb;

static char fullpath[MAXPATHLEN + 4];	/* Current full path name */
static char opns[16], *opn = opns;	/* File operations */
static int exit_status = 0;		/* Exit status */

/* Options. */
static int Default = FALSE;
static int n_opt = FALSE;


int
main(
	int argc,	/* Number of arguments */
	char *argv[])	/* Argument pointer list */
{
	extern int optind;
	int c;
#if defined(REMOTE)
	char *opts = "h:dn";
	char *rpchost = NULL;
#else /* defined(REMOTE) */
	char *opts = "dn";
#endif /* defined(REMOTE) */

	/*
	 * Process arguments.
	 */
	while ((c = getopt(argc, argv, opts)) != EOF) {
		switch (c) {
		case 'd':
			Default = TRUE;
			break;

		case 'n':
			n_opt = TRUE;
			break;

#if defined(REMOTE)
		case 'h':
			rpchost = strdup(optarg);
			break;
#endif /* defined(REMOTE) */

		case '?':
		default:
			exit_status++;
		}
	}

	if (optind == argc)
		exit_status++;	/* No file name */

	if (exit_status != 0) {
		(void) fprintf(stderr,
		    "Usage: %s [-h samhost] [-d] [-n] filename", argv[0]);
		exit(2);
	}
#if defined(REMOTE)
	if (sam_initrpc(rpchost) < 0) {
		(void) fprintf(stderr, "sam_initrpc failed, errno %d.\n",
		    errno);
		exit(2);
	}
#endif /* defined(REMOTE) */

	/*
	 * Set up action.
	 */
	(void) memset(opns, 0, sizeof (opns));
	if (Default)
		*opn++ = 'd';
	if (n_opt)
		*opn++ = 'n';
	if (!Default && !n_opt)
		*opn++ = 'i';

	/*
	 * Process all file names.
	 */
	while (optind < argc) {
		char    *name = argv[optind++];

		(void) strncpy(fullpath, name, sizeof (fullpath) - 2);
		if (sam_lstat(fullpath, &sb, sizeof (struct sam_stat)) < 0) {
			perror("sam_lstat");
			continue;
		}
		if (!SS_ISSAMFS(sb.attr)) {
			perror("Not a SAM-FS file");
			continue;
		}
		if (sam_archive(name, opns) < 0) {
			perror("sam_archive");
			continue;
		}
	}
#if defined(REMOTE)
	sam_closerpc();
#endif /* defined(REMOTE) */
	return (exit_status);
}
