/*
 *	sgethost.c -- library interfaces for shared FS operations
 *	related to failover.
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

#pragma ident "$Revision: 1.17 $"

/* Feature test switches. */
	/* None. */

/* ANSI C headers. */
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>

/* Solaris headers. */
#include <errno.h>

/* SAM-FS headers. */
#include <sam/types.h>
#include <sam/param.h>
#include <sam/syscall.h>
#include <sam/lib.h>

/* Local headers. */
	/* None. */

/* Macros. */
	/* None. */

/* Types. */
	/* None. */

/* Structures. */
	/* None. */

/* Private data. */
	/* None. */

/* Private functions. */
	/* None. */

/* Public data. */
	/* None. */

/* Function macros. */
	/* None. */

int
sam_gethost(const char *fs, int len, char *tab)
{
	int err;
	struct sam_sgethosts_arg arg;

	memset(&arg, 0, sizeof (arg));

	strncpy((char *)&arg.fsname, fs, sizeof (arg.fsname));
	arg.size = len;
	arg.hosts.ptr = tab;

	err = sam_syscall(SC_gethosts, &arg, sizeof (arg));
	return (err);
}


int
sam_sethost(const char *fs, int newserver, int len, const char *tab)
{
	int err;
	struct sam_sgethosts_arg arg;

	memset(&arg, 0, sizeof (arg));

	strncpy((char *)&arg.fsname, fs, sizeof (arg.fsname));
	arg.newserver = newserver;
	arg.size = len;
	arg.hosts.ptr = tab;

	err = sam_syscall(SC_sethosts, &arg, sizeof (arg));
	return (err);
}


int
sam_shareops(const char *fs, int op, int host)
{
	int err;
	struct sam_shareops_arg arg;

	memset(&arg, 0, sizeof (arg));

	strncpy((char *)&arg.fsname, fs, sizeof (arg.fsname));
	arg.op = op;
	arg.host = host;

	err = sam_syscall(SC_shareops, &arg, sizeof (arg));
	return (err);
}
