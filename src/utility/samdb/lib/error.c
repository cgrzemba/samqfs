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
#pragma ident "$Revision: 1.1 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sam/custmsg.h>
#include <sam/sam_trace.h>


#define	MAX_MSGLEN 1024

extern char *program_name;

/*
 * init_trace - initializes sam_trace.h depending on whether
 * caller is a Daemon or command line process.
 */
void
init_trace(int is_daemon, int trace_id)
{
	CustmsgInit(is_daemon, NULL);

	/* Set up tracing.  */
	if (is_daemon) {
		/* Provide file descriptors for the standard files.  */
		(void) close(STDIN_FILENO);
		if (open("/dev/null", O_RDONLY) != STDIN_FILENO) {
			LibFatal(open, "stdin /dev/null");
		}
		(void) close(STDOUT_FILENO);
		if (open("/dev/null", O_RDWR) != STDOUT_FILENO) {
			LibFatal(open, "stdout /dev/null");
		}
		(void) close(STDERR_FILENO);
		if (open("/dev/null", O_RDWR) != STDERR_FILENO) {
			LibFatal(open, "stderr /dev/null");
		}

		TraceInit(program_name, trace_id | TR_MPLOCK | TR_MPMASTER);
	} else {
		/* Redirect trace messages to stdout/stderr.  */
		TraceInit(NULL, TI_none);
		*TraceFlags = (1 << TR_date) - 1;
		*TraceFlags &= ~(1 << TR_alloc);
		*TraceFlags &= ~(1 << TR_dbfile);
		*TraceFlags &= ~(1 << TR_files);
		*TraceFlags &= ~(1 << TR_oprmsg);
		*TraceFlags &= ~(1 << TR_sig);
	}
}

/*
 * ----	error - Print error message.
 *
 *	Description:
 *	    Error prints an error message to standard error.  Error
 *	    is designed to be compatable with the GNU error routine.
 *	    A message of the following format is written:
 *
 *		program name:prefix message:error message
 *
 *	On entry:
 *	    status = Exit status.  If non-zero exit(2) will be called.
 *	    errno  = Error number.  If non-zero the corresponding error
 *		     message will be printed.
 *	    message= Error message prefix text if not NULL.
 *	    args   = Arguments supplied to printf when printing the
 *		     prefix message.
 */
void error(
	int status,		/* Exit status */
	/* LINTED */
	int errno,		/* Error number */
	char *message,	/* Prefix message */
	...)			/* Arguments for prefix message */
{
	va_list args;
	char msg[MAX_MSGLEN];

	if (message != NULL) {
		va_start(args, message);
		vsnprintf(msg, MAX_MSGLEN, message, args);
		va_end(args);
		Trace(TR_ERR, msg);
	}


	if (status) {
		exit(status);
	}
}
