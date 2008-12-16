/*
 * ---- dis_archiver.c - Display archiver status.
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

#pragma ident "$Revision: 1.48 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Solaris headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

/* SAM-FS headers. */
#define	NEED_EXAM_METHOD_NAMES
#define	NEED_ARFIND_EVENT_NAMES
#include "aml/archiver.h"
#include "aml/archset.h"
#include "aml/archreq.h"
#include "sam/custmsg.h"
#include "sam/mount.h"
#include "sam/lib.h"

#include "sam/sam_trace.h"
#include "sam/nl_samfs.h"

/* Local headers. */
#include "samu.h"

/* Private functions. */
static void disArcopy(int acn, boolean detail);
static void disArfind(struct sam_fs_status *fs, boolean detail);
static void attachArchiverdStateFile(void);
static char *countToA(longlong_t v);

/* Private data. */
static struct ArchiverdState *adState = NULL;
static struct CopyProgress {
	fsize_t	size;
	int	char_index;
} *copyProgress = NULL;			/* Copy progress for each arcopy */
static uname_t fileSystemName = "";
static int fileSystemNum = 0;		/* for the a display */
static int fileSystemCount = 0;
static int adCount = 0;
static int arcopyNum = 0;


/*
 *	Operator archiver display.
 */
void
DisArchiver(
void)
{
	struct sam_fs_status *fsarray;
	boolean detail;
	boolean ismore;
	int		arfStartline;
	int		i;

	attachArchiverdStateFile();
	if ((adState->AdLastAlarm + (2 * ALARM_TIME)) < time(NULL)) {
		/* Archiver daemon is not running. */
		Mvprintw(ln++, 0, "%s", GetCustMsg(7369));
		return;
	}

	/*
	 * Display archiver status.
	 */
	Mvprintw(ln++, 0, "%s:  %s", SAM_ARCHIVER, adState->AdOprMsg);
	Mvprintw(ln++, 0, "%s", adState->AdOprMsg2);
	ln += 2;
	arfStartline = ln;

	/*
	 * Display the arfind information.
	 * If a specific filesystem is requested, display it in detail.
	 */
	detail = fileSystemName[0] != '\0';
	ismore = FALSE;
	if ((fileSystemCount = GetFsStatus(&fsarray)) == -1) {
		Error("GetFsStatus failed");
	}
	for (i = fileSystemNum; i < fileSystemCount; i++) {
		if (ln < ((LINES - arfStartline - 4)/2 + arfStartline)) {
			disArfind(&fsarray[i], detail);
		} else if (!detail) {
			ismore = TRUE;
		}
	}
	free(fsarray);
	if (ismore) {
		/* "     more" */
		Mvprintw(ln++, 0, GetCustMsg(2));
	}

	/*
	 * Display the arcopy information.
	 */
	ln++;
	if (adCount < adState->AdCount) {
		size_t	size;

		adCount = adState->AdCount;
		size = adCount * sizeof (struct CopyProgress);
		copyProgress =
		    (struct CopyProgress *)realloc(copyProgress, size);
		if (NULL == copyProgress) {
			Error("cannot malloc copy count %d", adCount);
		}
		memset(copyProgress, 0, size);
	}
	for (i = arcopyNum; i < adState->AdCount; i++) {
		if (*adState->AdArchReq[i] != '\0') {
			disArcopy(i, detail);
		}
		if ((ln > LINES - 3) && ((i + 1) < adState->AdCount)) {
			/* "     more" */
			Mvprintw(ln++, 0, GetCustMsg(2));
			break;
		}
	}
}


/*
 * Display file system status.
 */
static void
disArfind(
struct sam_fs_status *fs,
boolean detail)
{
	struct ArfindState *af;

	if (detail && strcmp(fileSystemName, fs->fs_name) != 0) {
		return;
	}
	Mvprintw(ln++, 0, "%s:  %s", AF_PROG, fs->fs_name);
	af = ArfindAttach(fs->fs_name, O_RDONLY);
	if (af == NULL) {
		/* " not archiving" */
		Mvprintw(ln++, 0, GetCustMsg(1812));
		ln++;
		return;
	}
	if (*fs->fs_mnt_point == '\0') {
		Mvprintw(ln++, 0, "%s", af->AfOprMsg);
		ln++;
		return;
	} else {
		/* " mounted at %s" */
		Printw(GetCustMsg(1705), fs->fs_mnt_point);
	}

	Mvprintw(ln++, 0, "Files waiting to start %10s",
	    countToA(af->AfFilesCreate));
	Printw("  schedule %10s", countToA(af->AfFilesSchedule));
	Printw("  archiving %10s", countToA(af->AfFilesArchive));

	Mvprintw(ln++, 0, "%s", af->AfOprMsg);
	if (detail && af->AfExamine != EM_none) {
		int		i;

		/* Examine */
		Mvprintw(ln++, 0, "%s: %s", GetCustMsg(4634),
		    ExamMethodNames[af->AfExamine]);
		/* Interval */
		Printw("  %s: %s", GetCustMsg(4633),
		    StrFromInterval(af->AfInterval, NULL, 0));
		Mvprintw(ln++, 0, "%s: %s", GetCustMsg(4616), af->AfLogFile);

		Mvprintw(ln++, 0, "events    %12s",
		    countToA(af->AfFsactEvents));
		Printw("  syscalls %12s", countToA(af->AfFsactCalls));
		Printw("  buffers  %12s", countToA(af->AfFsactBufs));
		for (i = 1; i < AE_MAX; i++) {
			Mvprintw(ln++, 0, " %-9s%12s", fileEventNames[i],
			    countToA(af->AfEvents[i]));
		}

		Mvprintw(ln++, 0, "idstat    %12s", countToA(af->AfIdstat));
		Printw("  opendir  %12s", countToA(af->AfOpendir));
		Printw("  getdents %12s", countToA(af->AfGetdents));
		Mvprintw(ln++, 0, "id2path   %12s", countToA(af->AfId2path));
		Printw("  cached   %12s", countToA(af->AfId2pathCached));
		Printw("  idstat   %12s", countToA(af->AfId2pathIdstat));
		Printw("  readdir  %12s", countToA(af->AfId2pathReaddir));

		Mvprintw(ln++, 0, "scanlist  %12s",
		    countToA(af->AfScanlist[1]));
	}

	if (detail && af->AfExamine != EM_noscan && af->AfExamine != EM_none) {
		int copy;

		/* "regular files" */
		Mvprintw(ln++, 0, "%-16s", GetCustMsg(7364));
		Printw("  %12s", countToA(af->AfStats.regular.numof));
		Printw("  %9s", (af->AfStats.regular.numof == 0) ? "" :
		    StrFromFsize(af->AfStats.regular.size, 3, NULL, 0));
		if (af->AfStatsScan.total.numof != -1) {
			Printw("  %14s",
			    countToA(af->AfStatsScan.regular.numof));
			Printw("  %9s",
			    (af->AfStatsScan.regular.numof == 0) ? "" :
			    StrFromFsize(af->AfStatsScan.regular.size,
			    3, NULL, 0));
		} else {
			Printw("  %16lld", af->AfStatsScan.regular.size);
		}

		/* "offline files" */
		Mvprintw(ln++, 0, "%-16s", GetCustMsg(7365));
		Printw("  %12s", countToA(af->AfStats.offline.numof));
		Printw("  %9s", (af->AfStats.offline.numof == 0) ? "" :
		    StrFromFsize(af->AfStats.offline.size, 3, NULL, 0));
		if (af->AfStatsScan.total.numof != -1) {
			Printw("  %14s",
			    countToA(af->AfStatsScan.offline.numof));
			Printw("  %9s",
			    (af->AfStatsScan.offline.numof == 0) ? "" :
			    StrFromFsize(af->AfStatsScan.offline.size,
			    3, NULL, 0));
		} else {
			Printw("  %16lld", af->AfStatsScan.offline.size);
		}

		/* "archdone files" */
		Mvprintw(ln++, 0, "%-16s", GetCustMsg(7366));
		Printw("  %12s", countToA(af->AfStats.archdone.numof));
		Printw("  %9s", (af->AfStats.archdone.numof == 0) ? "" :
		    StrFromFsize(af->AfStats.archdone.size, 3, NULL, 0));
		if (af->AfStatsScan.total.numof != -1) {
			Printw("  %14s",
			    countToA(af->AfStatsScan.archdone.numof));
			Printw("  %9s",
			    (af->AfStatsScan.archdone.numof == 0) ? "" :
			    StrFromFsize(af->AfStatsScan.archdone.size,
			    3, NULL, 0));
		} else {
			Printw("  %16lld", af->AfStatsScan.archdone.size);
		}

		for (copy = 0; copy < MAX_ARCHIVE; copy++) {
			char	copyStr[12];

			/* "copy" */
			snprintf(copyStr, sizeof (copyStr), "%s%d",
			    GetCustMsg(7300),
			    copy + 1);
			Mvprintw(ln++, 0, "%-16s", copyStr);
			Printw("  %12s",
			    countToA(af->AfStats.copies[copy].numof));
			Printw("  %9s",
			    (af->AfStats.copies[copy].numof == 0) ? "" :
			    StrFromFsize(af->AfStats.copies[copy].size,
			    3, NULL, 0));
			if (af->AfStatsScan.total.numof != -1) {
				Printw("  %14s",
				    countToA(
				    af->AfStatsScan.copies[copy].numof));
				Printw("  %9s",
				    (af->
				    AfStatsScan.copies[copy].numof == 0) ? "" :
				    StrFromFsize(af->
				    AfStatsScan.copies[copy].size,
				    3, NULL, 0));
			} else {
				Printw("  %16lld",
				    af->AfStatsScan.copies[copy].size);
			}
		}

		/* "directories" */
		Mvprintw(ln++, 0, "%-16s", GetCustMsg(7367));
		Printw("  %12s", countToA(af->AfStats.dirs.numof));
		Printw("  %9s", (af->AfStats.dirs.numof == 0) ? "" :
		    StrFromFsize(af->AfStats.dirs.size, 3, NULL, 0));
		if (af->AfStatsScan.total.numof != -1) {
			Printw("  %14s", countToA(af->AfStatsScan.dirs.numof));
			Printw("  %9s", (af->AfStatsScan.dirs.numof == 0) ? "" :
			    StrFromFsize(af->AfStatsScan.dirs.size,
			    3, NULL, 0));
		} else {
			Printw("  %16lld", af->AfStatsScan.dirs.size);
		}

		/* "total" */
		Mvprintw(ln++, 0, "%-16s", GetCustMsg(7368));
		Printw("  %12s", countToA(af->AfStats.total.numof));
		Printw("  %9s", (af->AfStats.total.numof == 0) ? "" :
		    StrFromFsize(af->AfStats.total.size, 3, NULL, 0));
		if (af->AfStatsScan.total.numof != -1) {
			Printw("  %14s",
			    countToA(af->AfStatsScan.total.numof));
			Printw("  %9s",
			    (af->AfStatsScan.total.numof == 0) ? "" :
			    StrFromFsize(af->AfStatsScan.total.size,
			    3, NULL, 0));
		} else {
			Printw("  %16lld", af->AfStatsScan.total.size);
		}
	} else if (detail && af->AfExamine == EM_noscan) {
		ln++;
		/* No statistics available if examine = noscan */
		Mvprintw(ln++, 0, "%s", GetCustMsg(7402));
		/* Use "archiver -f [-n filesystem]" to get statistics. */
		Mvprintw(ln++, 0, "%s", GetCustMsg(7403));
	}
	(void) MapFileDetach(af);
	ln++;
}


/*
 * Display arcopy status.
 */
static void
disArcopy(
int acn,
boolean detail)
{
	static char *progressCh = { "|/-\\" };
	static upath_t arname;
	static uname_t	fsname;
	struct ArchReq *ar;
	struct ArcopyInstance *ci;
	char	*p;
	int		cpi;
	int		l;

	/*
	 * Decode AdArchReq entry.
	 * File system name.
	 */
	p = strchr(adState->AdArchReq[acn], '.');
	if (p == NULL) {
		return;
	}
	l = Ptrdiff(p, adState->AdArchReq[acn]);
	memmove(fsname, adState->AdArchReq[acn], l);
	fsname[l] = '\0';
	if (detail && strcmp(fileSystemName, fsname) != 0) {
		return;
	}

	/*
	 * ArchReq name and arcopy instance.
	 */
	strncpy(arname, p + 1, sizeof (arname));
	p = strrchr(arname, '.');
	*p++ = '\0';
	cpi = atoi(p);

	ar = ArchReqAttach(fsname, arname, O_RDONLY);
	if (ar == NULL) {
		return;
	}
	ci = &ar->ArCpi[cpi];

	Mvprintw(ln++, 0, "%s:  %s.%s %s.%s", AC_PROG, fsname, arname,
	    ci->CiMtype, ci->CiVsn);
	Mvprintw(ln++, 0, "%s", ci->CiOprmsg);
	if (detail) {
		/* "Archives: %d  files: %d (%d)  bytes: %9s" */
		Mvprintw(ln++, 0, GetCustMsg(7353), ci->CiArchives,
		    ci->CiFilesWritten, ci->CiFiles,
		    StrFromFsize(ci->CiBytesWritten, 3, NULL, 0));
		Printw(" (%s) %c", StrFromFsize(ci->CiSpace, 3, NULL, 0),
		    progressCh[copyProgress[acn].char_index]);
		if (copyProgress[acn].size != ci->CiBytesWritten) {
			copyProgress[acn].char_index =
			    (copyProgress[acn].char_index + 1) % 4;
			copyProgress[acn].size = ci->CiBytesWritten;
		}
		Mvprintw(ln++, 0, "Priority:%-.6G", ar->ArSchedPriority);
	}
	ln++;
	(void) MapFileDetach(ar);
}


/*
 * Initialization for archiver display.
 * a [filesystem]
 */
boolean
InitArchiver(
void)
{
	struct sam_fs_status *fsarray;
	int		i;

	attachArchiverdStateFile();
	if (0 == Argc) {
		return (TRUE);
	}
	if (Argc < 2) {
		*fileSystemName = '\0';
		return (TRUE);
	}

	/*
	 * Look up filesystem name.
	 */
	if ((fileSystemCount = GetFsStatus(&fsarray)) == -1) {
		Error("GetFsStatus failed");
	}
	for (i = 0; i < fileSystemCount; i++) {
		if (strcmp(Argv[1], fsarray[i].fs_name) == 0) {
			break;
		}
	}
	free(fsarray);
	if (i < fileSystemCount) {
		strcpy(fileSystemName, Argv[1]);
		fileSystemNum = i;
	} else {
		/* "File system %s not available." */
		Error(GetCustMsg(1160), Argv[1]);
		/* NOTREACHED */
	}
	return (TRUE);
}


/*
 * Keyboard processor for archiver.
 */
boolean
KeyArchiver(
char key)
{
	struct sam_fs_status *fsarray;
	boolean fwd = TRUE;

	switch (key) {

	case KEY_full_bwd:
		fwd = FALSE;

	/*FALLTHROUGH*/
	case KEY_full_fwd:
		if ((fileSystemCount = GetFsStatus(&fsarray)) == -1) {
			Error("GetFsStatus failed");
		}
		if (fwd) {
			fileSystemNum++;
			if (fileSystemNum >= fileSystemCount) {
				fileSystemNum = 0;
			}
		} else {
			fileSystemNum--;
			if (fileSystemNum < 0) {
				fileSystemNum = fileSystemCount + 1;
			}
		}
		if ('\0' == *fileSystemName) {
			free(fsarray);
			break;
		}
		strcpy(fileSystemName, fsarray[fileSystemNum].fs_name);
		free(fsarray);
		break;

	case KEY_half_fwd:
		if (++arcopyNum >= adState->AdCount) {
			arcopyNum = 0;
		}
		break;

	case KEY_half_bwd:
		if (--arcopyNum < 0) {
			arcopyNum = 0;
		}
		break;

	default:
		return (FALSE);
	}
	return (TRUE);
}


/*
 * Attach archiver daemon state file.
 */
static void
attachArchiverdStateFile(
void)
{
	static size_t oldLen = 0;
	upath_t fname;

	if (adState != NULL &&
	    (adState->Ad.MfValid == 0 || adState->Ad.MfLen != oldLen)) {
		(void) MapFileDetach(adState);
		adState = NULL;
	}
	if (NULL != adState) {
		return;
	}

	if (NULL != ArchiverDir) {
		snprintf(fname, sizeof (fname), "%s/ARCHIVER_STAT",
		    ArchiverDir);
		adState = MapFileAttach(fname, AD_MAGIC, O_RDONLY);
		if (adState == NULL) {
			LibFatal(MapFileAttach, fname);
		}
	} else {
		adState = MapFileAttach(ARCHIVER_DIR"/"ARCHIVER_STATE, AD_MAGIC,
		    O_RDONLY);
		if (adState == NULL) {
			/* "Access denied." */
			Error(GetCustMsg(331));
		}
	}

	if (AD_VERSION != adState->AdVersion) {
		int		version;

		version = adState->AdVersion;
		(void) MapFileDetach(adState);
		adState = NULL;
		errno = 0;
		/*
		 * "Archiver shared memory version mismatch.
		 * Is: %d, should be: %d"
		 */
		Error(GetCustMsg(440), version, AD_VERSION);
	}
	oldLen = adState->Ad.MfLen;
}


/*
 *	Return pointer to comma separated size conversion.
 * 	Returns the start of the string.
 */
static char *
countToA(
longlong_t v)	/* Value to convert. */
{
	static	char buf[32];
	char	*p;
	int		NumofDigits;

	p = buf + sizeof (buf) - 1;
	*p-- = '\0';
	NumofDigits = 0;
	/* Generate digits in reverse order. */
	do {
		if (NumofDigits++ >= 3) {
			*p-- = ',';
			NumofDigits = 1;
		}
		*p-- = v % 10 + '0';
	} while ((v /= 10) > 0);
	return (p+1);
}
