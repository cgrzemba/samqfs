/*
 * samstat.c - example program using sam_stat.
 *
 * Use sam_stat() to obtain information about a SAM-FS file.
 * This program may be used as a client.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <fcntl.h>

/* SAM-FS headers. */
#include "lib.h"
#include "stat.h"
#include "rminfo.h"
#if defined(REMOTE)
#include "samrpc.h"
#endif /* defined(REMOTE) */


int
main(
	int argc,	/* Number of arguments */
	char *argv[])	/* Argument pointer list */
{
#if defined(REMOTE)
	char *rpchost = NULL;
#endif /* defined(REMOTE) */
	int errors = 0;
	int c;

	/*
	 * Process arguments.
	 */
	while ((c = getopt(argc, argv, "h:")) != EOF) {
		switch (c) {

#if defined(REMOTE)
		case 'h':
			rpchost = strdup(optarg);
			break;
#endif /* defined(REMOTE) */

		case '?':
		default:
			errors++;
			break;
		}
	}

	if (optind  == argc || errors != 0) {
		(void) fprintf(stderr, "Usage:  %s [-h samhost] filename ...\n",
		    argv[0]);
		exit(2);
	}


#if defined(REMOTE)
	if (sam_initrpc(rpchost) < 0) {
		perror("sam_initrpc");
		exit(1);
	}
#endif /* defined(REMOTE) */

	/*
	 * Do sam_stat() for all the files.
	 */
	while (optind < argc) {
		struct sam_stat sb;
		char *fname;
		int n;

		fname = argv[optind++];
		if (sam_stat(fname, &sb, sizeof (struct sam_stat)) == -1) {
			perror("sam_stat");
			exit(1);
		}

		/*
		 * Print out the POSIX stat() information.
		 */
		(void) printf("\nsam_stat of file:  %s\n", fname);
		(void) printf("mode=0%lo, ino=%ld, dev=%ld, nlink=%ld\n",
		    sb.st_mode, sb.st_ino, sb.st_dev, sb.st_nlink);
		(void) printf("uid=%ld, gid=%ld\n",
		    sb.st_uid, sb.st_gid);
		(void) printf("atime=%ld, mtime=%ld, ctime=%ld\n",
		    sb.st_atime, sb.st_mtime, sb.st_ctime);
		if (!SS_ISSAMFS(sb.attr)) {
			(void) printf("\n");
			return (0);
		}

		/*
		 * Print out the SAM-FS information.
		 */
		(void) printf("attr:  %05x %s\n", sb.attr,
		    sam_attrtoa(sb.attr, NULL));
		(void) printf("\n");

		/*
		 * Print out the information for each of the archive copies.
		 */
		(void) printf("copy  flags  vsns   posn.offset      "
		    "media  vsn\n");
		for (n = 0; n < MAX_ARCHIVE; n++) {
			if (!(sb.copy[n].flags & CF_ARCHIVED)) continue;
			(void) printf("%d     %5x  %4d   0x%llx.%lx      "
			    "%-5s  %s\n",
			    n + 1, sb.copy[n].flags,
			    sb.copy[n].n_vsns,
			    sb.copy[n].position, sb.copy[n].offset,
			    sb.copy[n].media, sb.copy[n].vsn);
		}
	}
#if defined(REMOTE)
	sam_closerpc();
#endif /* defined(REMOTE) */
	return (0);
}
