/*
 * control.c - Arfind daemon control module.
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

#pragma ident "$Revision: 1.33 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <limits.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* POSIX headers. */
#include <unistd.h>
#include <sys/times.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "pub/stat.h"
#include "sam/lib.h"

/* Local headers. */
#include "arfind.h"

/* Processing functions. */
static void dirRmarchreq(void);
static void dirScan(void);
static void dirTrace(void);

static struct Controls {
	char	*CtIdent;
	void	(*CtFunc)(void);
} controls[] = {
	{ "rmarchreq", dirRmarchreq },
	{ "scan", dirScan },
	{ "trace", dirTrace },
	{ NULL }
};
static char *dirName;		/* Directive name */
static char errMsg[CTMSG_SIZE];	/* Buffer for error message return */
static char ident[128];
static char *value;
static jmp_buf errReturn;	/* Error message return */

/* Private functions. */
static void err(int msgNum, ...);
static void traceFileProps(void);


/*
 * Arfind control.
 */
char *
Control(
	char *identArg,
	char *valueArg)
{
	struct Controls *table;

	/*
	 * Isolate first identifier and look it up.
	 */
	strncpy(ident, identArg, sizeof (ident));
	value = valueArg;
	dirName = strtok(ident, ".");
	for (table = controls; table->CtIdent != NULL; table++) {
		if (strcmp(dirName, table->CtIdent) == 0) {
			break;
		}
	}

	/*
	 * Process the identifier.
	 */
	if (setjmp(errReturn) == 0) {
		if (table->CtIdent == NULL) {
			/* Unknown directive '%s' */
			err(CustMsg(4900), dirName, value);
		} else {
			table->CtFunc();
			return ("");
		}
	}
	return (errMsg);
}


/* Control functions. */



/*
 * Remove ArchReq(s).
 */
static void
dirRmarchreq(void)
{
	char	*arname;
	int	msgnum;

	arname = value;
	msgnum = ArchiveRmArchReq(arname);
	if (msgnum != 0) {
		err(CustMsg(msgnum), arname);
	}
}


/*
 * Scan a directory.
 */
static void
dirScan(void)
{
	struct ScanListEntry se;
	char	*path;
	int	interval;

	memset(&se, 0, sizeof (se));
	se.SeFlags = SE_request | SE_subdir;
	path = strtok(NULL, "");
	if (path == NULL || strcmp(path, ".") == 0) {
		se.SeId.ino = 0;
		se.SeFlags |= SE_back | SE_full;
	} else if (strcmp(path, ".inodes") == 0) {
		se.SeId.ino = 0;
		se.SeFlags |= SE_inodes | SE_back | SE_full;
	} else {
		struct sam_stat sb;

		/*
		 * Check the directory given.
		 */
		snprintf(ScrPath, sizeof (ScrPath), "%s/%s", MntPoint, path);
		if (sam_stat(ScrPath, &sb, sizeof (sb)) == -1) {
			err(CustMsg(616), ScrPath);
		}
		if (!SS_ISSAMFS(sb.attr) || !S_ISDIR(sb.st_mode)) {
			err(CustMsg(4904), path);
		}
		se.SeId.ino = sb.st_ino;
		se.SeId.gen = sb.gen;
	}
	if (*value != '\0') {
		if (StrToInterval(value, &interval) == -1) {
			/* Invalid '%s' value ('%s') */
			err(CustMsg(14102), "scan", value);
		}
	} else {
		interval = 0;
	}
	se.SeTime = time(NULL) + interval;
	ScanfsAddEntry(&se);
}


/*
 * Trace arfind tables.
 */
static void
dirTrace(void)
{
	TraceTimes();
	Trace(TR_MISC, "  idstat: %d  opendir: %d  getdents: %d",
	    State->AfIdstat, State->AfOpendir, State->AfGetdents);
	Trace(TR_MISC, "  id2path: %d  cached: %d idstat: %d  readdir: %d",
	    State->AfId2path, State->AfId2pathCached,
	    State->AfId2pathIdstat, State->AfId2pathReaddir);

	traceFileProps();
	FsActTrace();
	ExamInodesTrace();
	ScanfsTrace();
	ArchiveTrace();
	IdToPathTrace();
	MapFileTrace();
#if defined(TRACEREFS)
	TraceRefs();
#endif /* defined(TRACEREFS) */
}


/* Private functions. */


/*
 * Error return.
 * Compose message in response buffer.
 */
static void
err(
	int msgNum,
	...)
{
	va_list args;
	char *fmt;

	va_start(args, msgNum);
	if (msgNum != 0) {
		fmt = GetCustMsg(msgNum);
	} else {
		fmt = va_arg(args, char *);
	}
	va_end(args);
	vsnprintf(errMsg, sizeof (errMsg), fmt, args);
	longjmp(errReturn, 1);
}


/*
 * Trace file properties.
 */
static void
traceFileProps(void)
{
	FILE	*st;
	int	i;

	Trace(TR_MISC, "File properties:");
	if ((st = TraceOpen()) == NULL) {
		return;
	}
	for (i = 0; i < FileProps->FpCount; i++) {
		struct FilePropsEntry *fp;
		int	an;

		fp = &FileProps->FpEntry[i];
		fprintf(st, "%s ", ArchSetTable[fp->FpBaseAsn].AsName);
		if (!(fp->FpFlags & FP_metadata)) {
			fprintf(st, "  path: ");
			if (*fp->FpPath != '\0') {
				fprintf(st, "%s", fp->FpPath);
			} else {
				fprintf(st, ".");
			}
		} else {
			fprintf(st, "  Metadata");
		}
		fprintf(st, "\n");

		if (fp->FpFlags & FP_noarch) {
			continue;
		}
		/*
		 * Print out copy information.
		 */
		for (an = 0; an < MAX_AR; an++) {
			int	copy;
			int	copyBit;

			copy = an;
			if (copy >= MAX_ARCHIVE) {
				copy -= MAX_ARCHIVE;
			}
			copyBit = 1 << copy;
			if (!(fp->FpCopiesReq & copyBit)) {
				continue;
			}
			if (an < MAX_ARCHIVE) {
				fprintf(st, "  %d %s:  ", copy + 1,
				    ArchSetTable[fp->FpAsn[an]].AsName);
				if (fp->FpCopiesRel & copyBit) {
					fprintf(st, " Release");
				}
				if (fp->FpCopiesNorel & copyBit) {
					fprintf(st, " Norelease");
				}
				fprintf(st, " arch_age: %s",
				    StrFromInterval(fp->FpArchAge[copy],
				    NULL, 0));
				if (fp->FpCopiesUnarch & copyBit) {
					fprintf(st, " unarch_age: %s",
					    StrFromInterval(
					    fp->FpUnarchAge[copy], NULL, 0));
				}
			} else {
				fprintf(st, "  %dR %s", copy + 1,
				    ArchSetTable[fp->FpAsn[an]].AsName);
			}
			fprintf(st, "\n");
		}
	}
	fprintf(st, "\n");
	TraceClose(-1);
}
