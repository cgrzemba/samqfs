/*
 * samrpc.c - RPC library calls for SamFS clients
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

#pragma ident "$Revision: 1.16 $"

#define	SAM_LIB 1

#ifndef __linux__
#include <rpc/rpcent.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include "stat.h"
#include "samrpc.h"


/*
 * Select the RPC client protocol based on the type of platform.
 */

#ifndef __linux__
#define RPC_PROTOCOL "netpath"
#else
#define RPC_PROTOCOL "udp"
#endif


/*
 * Default timeout value for RPC calls in seconds.
 *
 * The same value is in ../src/rpc/client/samfs_clnt.c.
 */

#define RPC_TIMEOUT 25

/* Global data */
CLIENT *clnt;

static char sampath[PATH_MAX];
static int samlocal = 0;

char *
sam_filename(
	char *fn)
{
	/* If it's already a complete path, keep it. */
	if (fn[0] == '/') {
		return (fn);
	}
	if (fn[0] == '\0') {
		return (NULL);
	}

	if (samlocal) {
		/*
		 * Construct a complete path relative to the current working
		 * directory.
		 */
		if ((fn[0] == '~') || (fn[0] == '$')) {
			return (NULL);
		}
		if ((getcwd(sampath, PATH_MAX)) == NULL) {
			return (NULL);
		}
		(void) strcat(sampath, "/");
		(void) strcat(sampath, fn);
		return (sampath);
	} else {
		return (NULL);
	}
}

/*
 * Initialize the RPC connection.
 */

int
sam_initrpc(
	char *rpchost)
{
	struct rpcent *rpce;
	char *hostname;

	/* Get rpcent for program number */
	rpce = getrpcbyname(PROGNAME);
	if (!rpce) {
		/* Try getting the rpcent using the default program number */
		rpce = getrpcbynumber(SamFS);
		if ((!rpce) || (strcmp(PROGNAME, rpce->r_name))) {
			errno = EADDRNOTAVAIL;
			return (-1);
		}
	}
	/* Get name of sam host */
	if (rpchost) {
		hostname = rpchost;
	} else {
		if (!(hostname = getenv("SAMHOST"))) {
			hostname = SAMRPC_HOST;
		}
	}
	if (strcmp(hostname, "localhost") == 0) {
		samlocal = 1;
	}

	clnt = clnt_create(hostname, (ulong_t)rpce->r_number, SAMVERS,
	   RPC_PROTOCOL); 
	if (clnt == (CLIENT *)NULL) {
		clnt_pcreateerror(hostname);
		return (-1);
	}
	auth_destroy(clnt->cl_auth);
	clnt->cl_auth = authunix_create_default();
	return (0);
}


/*
 * Archive the file and/or set archive attributes.
 */

int
sam_archive(
	char *filename,
	char *options)
{
	filecmd samarchive_arg;
	int *result;

	if (clnt == (CLIENT *)NULL) {
		errno = EDESTADDRREQ;
		return (-1);
	}
	samarchive_arg.filename = sam_filename(filename);
	if (samarchive_arg.filename == NULL) {
		errno = EINVAL;
		return (-1);
	}
	samarchive_arg.options = options;
	result = samarchive_1(&samarchive_arg, clnt);
	if (result == (int *)NULL) {
		clnt_perror(clnt, "sam_archive failed");
		return (-1);
	}
	return (0);

}


/*
 * Set attributes on a file or directory
 */

int
sam_setfa(
	char *filename,
	char *options)
{
	filecmd samsetfa_arg;
	int *result;

	if (clnt == (CLIENT *)NULL) {
		errno = EDESTADDRREQ;
		return (-1);
	}
	samsetfa_arg.filename = sam_filename(filename);
	if (samsetfa_arg.filename == NULL) {
		errno = EINVAL;
		return (-1);
	}
	samsetfa_arg.options = options;
	result = samsetfa_1(&samsetfa_arg, clnt);
	if (result == (int *)NULL) {
		clnt_perror(clnt, "sam_setfa failed");
		return (-1);
	}
	if (*result) {
		errno = *result;
		return (-1);
	} else {
		return (0);
	}

}


/*
 * Set segment attributes on a file or directory
 */

int
sam_segment(
	char *filename,
	char *options)
{
	filecmd samsegment_arg;
	int *result;

	if (clnt == (CLIENT *)NULL) {
		errno = EDESTADDRREQ;
		return (-1);
	}
	samsegment_arg.filename = sam_filename(filename);
	if (samsegment_arg.filename == NULL) {
		errno = EINVAL;
		return (-1);
	}
	samsegment_arg.options = options;
	result = samsegment_1(&samsegment_arg, clnt);
	if (result == (int *)NULL) {
		clnt_perror(clnt, "sam_segment failed");
		return (-1);
	}
	if (*result) {
		errno = *result;
		return (-1);
	} else {
		return (0);
	}

}


/*
 * Release disk space and/or set release attributes.
 */

int
sam_release(
	char *filename,
	char *options)
{
	filecmd samrelease_arg;
	int *result;

	if (clnt == (CLIENT *)NULL) {
		errno = EDESTADDRREQ;
		return (-1);
	}
	samrelease_arg.filename = sam_filename(filename);
	if (samrelease_arg.filename == NULL) {
		errno = EINVAL;
		return (-1);
	}
	samrelease_arg.options = options;
	result = samrelease_1(&samrelease_arg, clnt);
	if (result == (int *)NULL) {
		clnt_perror(clnt, "sam_release failed");
		return (-1);
	}
	if (*result) {
		errno = *result;
		return (-1);
	} else {
		return (0);
	}
}


/*
 * Copy the off-line file to disk and/or set staging attributes.
 */

int
sam_stage(
	char *filename,
	char *options)
{
	filecmd samstage_arg;
	int *result;

	if (clnt == (CLIENT *)NULL) {
		errno = EDESTADDRREQ;
		return (-1);
	}
	samstage_arg.filename = sam_filename(filename);
	if (samstage_arg.filename == NULL) {
		errno = EINVAL;
		return (-1);
	}
	samstage_arg.options = options;
	result = samstage_1(&samstage_arg, clnt);
	if (result == (int *)NULL) {
		clnt_perror(clnt, "sam_stage failed");
		return (-1);
	}
	if (*result) {
		errno = *result;
		return (-1);
	} else {
		return (0);
	}
}


/*
 * Obtain file attributes about the file.
 * Read, write or execute permission of the named file is not required,
 * but all directories in the path name must be searchable.
 */

int
sam_stat(
	const char *filename,
	struct sam_stat *statbuf,
	size_t bufsize)
{
	sam_st *result_1;
	statcmd samstat_1_arg;
	int size;

	if (clnt == (CLIENT *)NULL) {
		errno = EDESTADDRREQ;
		return (-1);
	}
	samstat_1_arg.filename = sam_filename((char *)filename);
	if (samstat_1_arg.filename == NULL) {
		errno = EINVAL;
		return (-1);
	}
	samstat_1_arg.size = bufsize;
	result_1 = samstat_1(&samstat_1_arg, clnt);
	if (result_1 == (sam_st *)NULL) {
		clnt_perror(clnt, "sam_stat failed");
		return (-1);
	}
	if (result_1->result) {
		errno = result_1->result;
		return (-1);
	}
	/*
	 * Copy the stat structure returned to the user's specified location.
	 */
	size = (bufsize <= sizeof (struct sam_stat)) ?
	    bufsize : sizeof (struct sam_stat);
	(void) memcpy(statbuf, &result_1->s, size);
	return (0);
}


/*
 * Obtain file attributes similar to sam_stat, except when the named file
 * is a symbolic link.  In this case, sam_lstat returns information about
 * the link.
 */

int
sam_lstat(
	const char *filename,
	struct sam_stat *statbuf,
	size_t bufsize)
{
	sam_st *result_1;
	statcmd samlstat_1_arg;
	int size;

	if (clnt == (CLIENT *)NULL) {
		errno = EDESTADDRREQ;
		return (-1);
	}
	samlstat_1_arg.filename = sam_filename((char *)filename);
	if (samlstat_1_arg.filename == NULL) {
		errno = EINVAL;
		return (-1);
	}
	samlstat_1_arg.size = bufsize;
	result_1 = samlstat_1(&samlstat_1_arg, clnt);
	if (result_1 == (sam_st *)NULL) {
		clnt_perror(clnt, "sam_lstat failed");
		return (-1);
	}
	if (result_1->result) {
		errno = result_1->result;
		return (-1);
	}
	/*
	 * Copy the stat structure returned to the user's specified location.
	 */
	size = (bufsize <= sizeof (struct sam_stat)) ?
	    bufsize : sizeof (struct sam_stat);
	(void) memcpy(statbuf, &result_1->s, size);
	return (0);
}


/*
 * Close the rpc connection.
 */

int
sam_closerpc()
{
	if (clnt == (CLIENT *)NULL) {
		return (0);
	}
	auth_destroy(clnt->cl_auth);
	clnt_destroy(clnt);
	return (0);
}
