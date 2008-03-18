/*
 * log.c - collect recycling events and write to log file.
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
#pragma ident "$Revision: 1.5 $"

static char *_SrcFile = __FILE__;

#include "recycler_c.h"

#include "sam/custmsg.h"
#include "sam/sam_trace.h"
#include "sam/sam_malloc.h"

static struct {
	boolean_t	init;		/* only allow one initialization */
	char		*path;		/* log file path name */
	FILE		*fp;		/* file pointer */
} logData = {B_FALSE, NULL, NULL};

int
LogOpen(
	char *path)
{
	FILE *fp;
	int retval;

	if (logData.init == B_TRUE)
		return (-1);

	if (path != NULL) {
		fp = fopen(path, "a");
		if (fp == NULL) {
			Trace(TR_MISC, "Error: '%s' open failed, errno: %d",
			    path, errno);
		}
	}

	if (fp == NULL) {
		retval = -1;
	} else {
		logData.init = B_TRUE;
		logData.fp = fp;

		SamMalloc(logData.path, strlen(path) + 1);
		(void) strncpy(logData.path, path, strlen(path) + 1);

		retval = 0;
	}

	return (retval);
}

void
Log(
	int msgnum,
	...)
{
	va_list ap;

	char buf[1025];
	char *format;


	va_start(ap, msgnum);
	if (msgnum != 0) {
		format = GetCustMsg(msgnum);
	} else {
		format = va_arg(ap, char *);
	}
	va_end(ap);
	(void) vsprintf(buf, format, ap);

	Trace(TR_MISC, "[LOG] %s", buf);

	if (logData.init == B_TRUE && logData.fp != NULL) {
		(void) fprintf(logData.fp, "%s\n", buf);
		(void) fflush(logData.fp);
	}
}

void
LogClose(void)
{
	if (logData.fp != NULL) {
		(void) fclose(logData.fp);
	}
	if (logData.path != NULL) {
		SamFree(logData.path);
	}
	(void) memset(&logData, 0, sizeof (logData));
}
