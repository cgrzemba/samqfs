/*
 * error.c - Print error message.
 *
 *	Description:
 *	    Error prints an error message to standard error.  Error
 *	    is designed to be compatible with the GNU error routine.
 *	    A message of the following format is written:
 *
 *		program name:prefix message:error message
 *
 *	On entry:
 *	    status = Exit status.  If non-zero exit(2) will be called.
 *	    errno  = Error number.  If non-zero the corresponding error
 *		    message will be printed.
 *	    message = Error message prefix text if not NULL.
 *	    args   = Arguments supplied to printf when printing the
 *		    prefix message.
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

#pragma ident "$Revision: 1.19 $"


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include "sam/types.h"
#include "sam/lib.h"

extern char *program_name;	/* Program name	*/

void
error(
	int status,
	int errnum,		/* errno from calling routine */
	char *fmt,
	...)
{
	char	msg_buf[4096];
	char	*p, *pe;

	p = msg_buf;
	pe = p + sizeof (msg_buf)-2;
	if (program_name != NULL) {
		snprintf(p, Ptrdiff(pe, p), "%s: ", program_name);
		p += strlen(p);
	}

	if (fmt != NULL) {
		va_list	args;

		va_start(args, fmt);
		vsnprintf(p, Ptrdiff(pe, p), fmt, args);
		va_end(args);
		p += strlen(p);
	}

	if (errnum) {
		snprintf(p, Ptrdiff(pe, p), ": ");
		p += strlen(p);
		(void) StrFromErrno(errnum, p, Ptrdiff(pe, p));
		p += strlen(p);
	}
	*p++ = '\n';
	*p = '\0';
	fputs(msg_buf, stderr);
	fflush(stderr);
	if (status != 0) {
		exit(status);
	}
}
