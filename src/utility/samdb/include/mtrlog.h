/*
 * ----	mtrlog.h - Type definitions for mtrlog.
 *
 *	Description:
 *	    Structure and proto type definitions for routines used
 *	    by mtrlog.
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

#define	logtime()	logtime2()

#define	FILE_LOG(a, b)	 fprintf(MTR_file, "%s: %s %s\n", logtime2(), a, b)
#define	FILE_LOGL(a, b, c) fprintf(MTR_file, "%s: %s %s [%s]\n", \
		logtime2(), a, b, c)
#define	FILE_LOGC(a, b, c) if (c) fprintf(MTR_file, "%s: %s %s\n", \
		logtime2(), a, b)


void init_mtrlog(FILE *log, FILE *err);
void mtrlog(int, int, char *, ...);
char *logfile(char *, char *);
char *logtime1();
char *logtime2();
