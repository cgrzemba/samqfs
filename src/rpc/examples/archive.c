/*
 * archive.c - example program using archive.
 *
 * Use archive() to set archive attributes on a SAM-FS file.
 * This program may be compiled for use locally or as an rpc client.
 *
 */

/*
 *    SUN_disclaimer_begin
 *
 *  Copyright(c) 2003 Sun Microsystems, Inc.
 *  All rights reserved.
 *
 *  This file is a product of Sun Microsystems, Inc. and is provided for
 *  unrestricted use provided that this header is included on all media
 *  and as a part of the software program in whole or part.  Users may
 *  copy, modify or distribute this file at will.
 *
 *  This file is provided with no support and without any obligation on
 *  the part of Sun Microsystems, Inc. to assist in its use, correction,
 *  modification or enhancement.
 *
 *  THIS FILE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING
 *  THE WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 *  SUN MICROSYSTEMS INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 *  INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS FILE
 *  OR ANY PART THEREOF.
 *
 *  IN NO EVENT WILL SUN MICROSYSTEMS, INC. BE LIABLE FOR ANY LOST REVENUE
 *  OR PROFITS OR OTHER SPECIAL, INDIRECT AND CONSEQUENTIAL DAMAGES, EVEN
 *  IF THEY HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 *
 *  Sun Microsystems, Inc.
 *
 *    SUN_disclaimer_end
 */


#pragma ident "$Revision: 1.7 $"

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
