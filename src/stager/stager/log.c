/*
 * log.c - collect file staging events and write to log file
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

#pragma ident "$Revision: 1.28 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
struct tm *localtime_r(const time_t *clock, struct tm *res);
#include <unistd.h>
#include <pthread.h>

/* SAM-FS headers. */
#include "sam/lib.h"
#include "sam/custmsg.h"
#include "aml/stager.h"

/* Local headers. */
#include "stager_lib.h"
#include "file_defs.h"
#include "stage_reqs.h"
#include "log.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

#define	MAXLINE 1024+256

/*	Public data. */
LogInfo_t LogFile = { NULL };

/*	Private data. */
static char logBuffer[MAXLINE];
static pthread_mutex_t logFileMutex = PTHREAD_MUTEX_INITIALIZER;

static void writeLog(const char *type, const char *fmt, ...);
static void writeLogFile(const char *type, const char *fmt, va_list args);

/*
 * Open stager's log file.
 */
void
OpenLogFile(
	char *name)
{
	if (LogFile.file != NULL) {
		fclose(LogFile.file);
		LogFile.file = NULL;
	}

	if (name != NULL && *name != '\0') {
		LogFile.file = fopen64(name, "a");
		if (LogFile.file == NULL) {
			SendCustMsg(HERE, 19002, name);
		}
	}
}

/*
 * Check if log file was removed or truncated.
 * If yes, start a new one.
 */
void
CheckLogFile(
	char *name)
{
	struct stat buf;

	if (name != NULL && *name != '\0') {
		int rc;

		rc = stat(name, &buf);
		if ((rc == -1 && (errno == ENOENT || errno == ENOTDIR)) ||
		    (rc >= 0 && buf.st_size == 0)) {
			/*
			 * Log file not found or truncated.  Start a new one.
			 */
			(void) pthread_mutex_lock(&logFileMutex);
			OpenLogFile(name);
			(void) pthread_mutex_unlock(&logFileMutex);
		}
	}
}

/*
 * Write staging event to log file.
 */
void
LogIt(
	LogType_t type,
	FileInfo_t *file)
{
	int copy;
	char *typeString;
	char pathBuffer[PATHBUF_SIZE];

	if (GET_FLAG(file->flags, FI_DUPLICATE)) {
		return;
	}

	if (LogFile.file != NULL && IsLogEventEnabled(type)) {

		switch (type) {
			case LOG_STAGE_START:
				typeString = "S";
				break;
			case LOG_STAGE_DONE:
				typeString = "F";
				break;
			case LOG_STAGE_CANCEL:
				typeString = "C";
				break;
			case LOG_STAGE_ERROR:
				typeString = "E";
				break;
			default:
				typeString = "?";
		}

		copy = file->copy;
		GetFileName(file, &pathBuffer[0], PATHBUF_SIZE, NULL);

		writeLog(typeString,
		    "%s %s %llx.%llx %ld.%d %lld %s %d %s %s %s %d %c",
		    sam_mediatoa(file->ar[copy].media),
		    file->ar[copy].section.vsn,
		    file->ar[copy].section.position,
		    file->ar[copy].section.offset,
		    file->id.ino, file->id.gen, file->len,
		    pathBuffer, copy + 1,
		    getuser(file->owner),
		    getgroup(file->group),
		    getuser(file->user),
		    file->eq,
		    GET_FLAG(file->flags, FI_DATA_VERIFY) ? 'V' : '-');
	}
}

/*
 * Write start log entry for specified file.  Used by migration toolkit.
 */
void
LogStageStart(
	FileInfo_t *file)
{
	if (file != NULL) {
		LogIt(LOG_STAGE_START, file);
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
	static char *tdformat = " %Y/%m/%d %H:%M:%S ";

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
