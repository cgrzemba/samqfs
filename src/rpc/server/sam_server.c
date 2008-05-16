/*
 *  sam_server.c
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

#pragma ident "$Revision: 1.16 $"

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include "pub/stat.h"
#include "pub/samrpc.h"
#include "pub/lib.h"

sam_st *
samstat_1(
	statcmd *argp,
	/* LINTED argument unused in function */
	struct svc_req *rqstp)
{
	static sam_st statbuf;
	int ret;

	ret = sam_stat(argp->filename, (samstat_t *)& statbuf.s, argp->size);
	if (ret < 0) {
		statbuf.result = errno;
	} else {
		statbuf.result = 0;
	}

	return (&statbuf);
}

sam_st *
samlstat_1(
	statcmd *argp,
	/* LINTED argument unused in function */
	struct svc_req *rqstp)
{
	static sam_st statbuf;
	int ret;

	ret = sam_lstat(argp->filename, (samstat_t *)& statbuf.s, argp->size);
	if (ret < 0) {
		statbuf.result = errno;
	} else {
		statbuf.result = 0;
	}
	return (&statbuf);
}

int *
samarchive_1(
	filecmd * argp,
	/* LINTED argument unused in function */
	struct svc_req *rqstp)
{
	static int result;

	result = sam_archive(argp->filename, argp->options);
	if (result < 0) {
		result = errno;
	}
	return (&result);
}

int *
samsetfa_1(
	filecmd * argp,
	/* LINTED argument unused in function */
	struct svc_req *rqstp)
{
	static int result;

	result = sam_setfa(argp->filename, argp->options);
	if (result < 0) {
		result = errno;
	}
	return (&result);
}

int *
samsegment_1(
	filecmd * argp,
	/* LINTED argument unused in function */
	struct svc_req *rqstp)
{
	static int result;

	result = sam_segment(argp->filename, argp->options);
	if (result < 0) {
		result = errno;
	}
	return (&result);
}

int *
samrelease_1(
	filecmd * argp,
	/* LINTED argument unused in function */
	struct svc_req *rqstp)
{
	static int result;

	result = sam_release(argp->filename, argp->options);
	if (result < 0) {
		result = errno;
	}
	return (&result);
}

int *
samstage_1(
	filecmd * argp,
	/* LINTED argument unused in function */
	struct svc_req *rqstp)
{
	static int result;

	result = sam_stage(argp->filename, argp->options);
	if (result < 0) {
		result = errno;
	}
	return (&result);
}
