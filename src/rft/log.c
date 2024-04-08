/*
 * log.c - collect rft events and write to log file
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

#pragma ident "$Revision: 1.12 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
struct tm *localtime_r(const time_t *clock, struct tm *res);
#include <unistd.h>
#include <pthread.h>

/* SAM-FS headers. */
#include "sam/lib.h"
#include "sam/custmsg.h"
#include "sam/sam_trace.h"
#include "aml/sam_rft.h"

/* Local headers. */
#include "rft_defs.h"
#include "log.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

#define	MAXLINE 1024+256

/* Public data. */
LogInfo_t LogFile = { NULL };

/* Private data. */
static char logBuffer[MAXLINE];
static pthread_mutex_t logFileMutex = PTHREAD_MUTEX_INITIALIZER;

static void writeLog(const char *type, const char *fmt, ...);
static void writeLogFile(const char *type, const char *fmt, va_list args);

/*
 * Open rft daemon's log file.
 */
void
OpenLogFile(
	char *name)
{
	if (name != NULL && *name != '\0') {
		LogFile.file = fopen64(name, "a");
		if (LogFile.file == NULL) {
			SendCustMsg(HERE, 22002, name);
		}
	}
}

/*
 * Write rft event to log file.
 */
void
LogIt(
	Crew_t *crew)
{
	char *typeString;
	uint64_t nbytes;
	char *hostname;
	char *filename;

	if (IS_LOG_ENABLED()) {

		ASSERT(crew != NULL);

		if (crew->nbytes_sent > 0) {
			typeString = "S";
			nbytes = crew->nbytes_sent;
		} else {
			typeString = "R";
			nbytes = crew->nbytes_received;
		}

		ASSERT(crew->cli != NULL);

		hostname = IS_NULL(crew->cli->hostname);
		filename = IS_NULL(crew->filename);

		writeLog(typeString, "%s %llu %s", hostname, nbytes, filename);
	}
}

/*
 * Write log.
 */
static void
writeLog(
	const char *type,
	const char *fmt,
	...)
{
	va_list args;
	va_start(args, fmt);

	writeLogFile(type, fmt, args);
	va_end(args);
}

/*
 * Write log helper.
 */
static void
writeLogFile(
	const char *type,
	const char *fmt,
	va_list ap)
{
	struct tm tm;
	time_t now;
	char *msg;
	static char *tdformat = " %m/%d %H:%M:%S ";

	(void) pthread_mutex_lock(&logFileMutex);
	msg = logBuffer;
	strcpy(msg, type);
	msg += strlen(msg);

	now = time(NULL);
	strftime(msg, sizeof (logBuffer)-1, tdformat,
	    localtime_r(&now, &tm));
	msg += strlen(msg);

	if (fmt != NULL) {
		vsprintf(msg, fmt, ap);
		msg += strlen(msg);
	}

	*msg++ = '\n';
	*msg = '\0';
	fputs(logBuffer, LogFile.file);
	(void) fflush(LogFile.file);
	(void) pthread_mutex_unlock(&logFileMutex);
}
