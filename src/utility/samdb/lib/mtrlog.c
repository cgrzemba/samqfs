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

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "shadow.h"
#include "mtrlog.h"

extern char *program_name; /* Program name: used by error */

extern char *sys_errlist[]; /* Error message text */
extern int sys_nerr; /* Maximum defined error number */

extern int SAMDB_Verbose; /* Verbose flag */
static FILE *MTR_log; /* Log file */
static FILE *MTR_err; /* Error log file */

/*
 * ---- setMTRLogFile - Sets the files used for MTR logging
 */
void
init_mtrlog(FILE *log, FILE *err)
{
	MTR_log = log;
	MTR_err = err;
}

/*
 * ----	mtrlog - Write error message to log files.
 *
 *	Description:
 *	    Mtrlog writes an error message to the specified log files.
 *	    Mtrlog is designed to be compatable with the GNU error
 *	    routine.  A message of the following format is written:
 *
 *		time:prefix message:error message
 *
 *	On entry:
 *		where  = Log file routing.
 *			0 = log file only.
 *			1 = log and errlog files.
 *			2 = log and errlog files, and stderr.
 *		errnum  = Error number.  If non-zero the corresponding error
 *		     message will be printed.
 *	    message= Error message prefix text if not NULL.
 *	    args   = Arguments supplied to printf when printing the
 *		     prefix message.
 */
void
mtrlog(
	int where,	/* Exit status */
	int errnum,	/* Error number */
	char *message,	/* Prefix message */
	...) 		/* Arguments for prefix message	*/
{
	va_list args;
	int debug;

	debug = SAMDB_Verbose || (where > 1);

	fprintf(MTR_log, "%s: ", logtime());
	if (where) {
		fprintf(MTR_err, "%s: ", logtime());
	}

	if (message != NULL) {
		va_start(args, message);
		vfprintf(MTR_log, message, args);
		if (where) {
			vfprintf(MTR_err, message, args);
		}
		if (debug) {
			vfprintf(stderr, message, args);
		}
		va_end(args);
	}

	if (errnum) {
		if (errnum > 0 && errnum < sys_nerr) {
			fprintf(MTR_log, ": %s", sys_errlist[errnum]);
			if (where) {
				fprintf(MTR_err, ": %s", sys_errlist[errnum]);
			}
			if (debug) {
				fprintf(stderr, ": %s", sys_errlist[errnum]);
			}
		} else {
			fprintf(MTR_log, ": Unknown error %d", errnum);
			if (where) {
				fprintf(MTR_err, ": Unknown error %d", errnum);
			}
			if (debug) {
				fprintf(stderr, ": Unknown error %d", errnum);
			}
		}
	}

	fprintf(MTR_log, "\n");
	fflush(MTR_log);
	if (where) {
		fprintf(MTR_err, "\n");
		fflush(MTR_err);
	}
	if (debug) {
		fprintf(stderr, "\n");
		fflush(stderr);
	}
}
