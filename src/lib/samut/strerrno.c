/*
 * strerrno.c - get error message string.
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#pragma ident "$Revision: 1.18 $"

/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* SAM-FS headers. */
#include "sam/nl_samfs.h"
#include "pub/sam_errno.h"
#include "sam/custmsg.h"
#include "sam/lib.h"
#include "sam/lint.h"

/*
 * Pre Solaris 2.6.
 * snprintf() is not defined.  Use vsprintf() and count on the user of snprintf
 * to provide a big enough buffer.
 */



/*
 * Convert an error number to a string.
 * Process sam error numbers and an unknown error number.
 * Return message to user provided buffer.
 */
char *
StrFromErrno(
	int errno_arg,		/* Error number to convert */
	char *buf,		/* Buffer for string */
	int buf_size)		/* Size of buffer */
{
	static char our_buf[STR_FROM_ERRNO_BUF_SIZE];
	char *p;
	int msgnum;

	if (buf == NULL) {
		buf = our_buf;
		buf_size = sizeof (our_buf);
	}
	if (buf_size < 2) {
		if (buf_size >= 1) {
			*buf = '\0';
		}
		return (buf);
	}

	msgnum = 0;
	if (SAM_ERRNO <= errno_arg && errno_arg < SAM_MAX_ERRNO) {
		/*
		 * A sam error number.
		 * Look up the error in our message catalog.
		 */
		msgnum = (errno_arg - SAM_ERRNO) + ERRNO_CATALOG;
	} else if (0 <= errno_arg && errno_arg < 256) {
		/*
		 * Solaris strerrno() returns "Unknown error"
		 * if the error is not known.
		 */
		if ((p = strerror(errno_arg)) != NULL) {
			memccpy(buf, p, '\0', buf_size - 1);
			buf[buf_size - 1] = '\0';
			return (buf);
		}
	}
	if (catfd == NULL) {
		CustmsgInit(0, NULL);
	}

	if (msgnum != 0) {
		p = catgets(catfd, SET, msgnum,
		    "Error number %d, SAM-FS message %d");
	} else {
		/*
		 * Our "Unknown error number %d"
		 */
		p = catgets(catfd, SET, 16999, "Error number %d");
	}
	snprintf(buf, buf_size, p, errno_arg, msgnum);
	return (buf);
}


#if defined(TEST)


int
main(
	int argc,
	char *argv[])
{
	int		err_no;

	for (err_no = -1; err_no <= 255; err_no++) {
		printf("%5d %s\n", err_no, StrFromErrno(err_no, NULL, 0));
	}

	for (err_no = SAM_ERRNO - 1; err_no <= SAM_MAX_ERRNO; err_no++) {
		printf("%5d %s\n", err_no, StrFromErrno(err_no, NULL, 0));
	}
	return (0);
}

#endif /* defined(TEST) */
