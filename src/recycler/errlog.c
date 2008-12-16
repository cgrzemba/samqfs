/*
 * errlog.c
 *  function which allow selection of any (or all) of three diferent
 *	destinations for the messages.  Selection is done via the first
 *	argument.  The three destinations are:
 *
 *		TO_TTY		stderr
 *		TO_SYS		system logfile (syslog())
 *		TO_FILE		recycler's logfile (if a logfile = xxx line
 *				were specified in the recycler.cmd file)
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

#pragma ident "$Revision: 1.18 $"

#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>
#include "recycler.h"

#include "sam/custmsg.h"
#include "sam/lib.h"
#include "sam/lint.h"

#define	MAXLINE 1024+256

void
emit(
	int where,
	int priority,
	int msgno,
	...)
{
	va_list	ap;

	char	buf[MAXLINE];
	char	*format;

	va_start(ap, msgno);
	if (msgno != 0) {
		format = GetCustMsg(msgno);
	} else {
		format = va_arg(ap, char *);
	}
	va_end(ap);
	vsprintf(buf, format, ap);

	if (where & TO_TTY) {
		fprintf(stderr, "%s\n", buf);
	}

	if (where & TO_SYS) {
		sam_syslog(priority, "%s", buf);
	}

	if (where & TO_FILE && log != NULL) {
		printf("%s\n", buf);
	}
}
