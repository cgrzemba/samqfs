/*
 * archreq.c - compose archive requests.
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

#pragma ident "$Revision: 1.67 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <grp.h>

/* SAM-FS headers. */
#include "sam/custmsg.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/sam_trace.h"
#include "sam/fs/ino.h"

/* Local headers. */
#include "common.h"
#include "utility.h"
#define	NEED_ARCHSET_NAMES
#include "archset.h"
#define	NEED_ARCHREQ_NAMES
#define	NEED_ARCHREQ_MSG_SENT
#include "archreq.h"

/* Private functions. */
static void printFileList(FILE *st, struct ArchReq *ar, int cpi);
static void printFileInfo(FILE *st, struct FileInfo *fi, boolean_t first);
static void printVolumeList(FILE *st, struct ArchReq *ar);


/*
 * Check to see if a customer message has already been sent.
 * RETURN: TRUE if message sent.
 */
boolean_t
ArchReqCustMsgSent(
	struct ArchReq *ar,
	int msgNum)
{
	int	flags;
	int	i;

	/*
	 * Check to see if message was sent.
	 */
	for (i = 0, flags = 1; msgSentTable[i] != 0; i++) {
		if (msgSentTable[i] == msgNum) {
			if (flags & ar->ArMsgSent) {
				/* Message sent */
				return (TRUE);
			}
			ar->ArMsgSent |= flags;
			break;
		}
		flags <<= 1;
	}
	/*
	 * Message not in table, or not sent.
	 */
	return (FALSE);
}


/*
 * Return the file name for an ArchReq.
 * The file name is returned in a user's buffer.
 */
void
ArchReqFileName(
	struct ArchReq *ar,
	char *fname)
{
	snprintf(fname, ARCHREQ_FNAME_SIZE,
	    ARCHIVER_DIR"/%s/"ARCHREQ_DIR"/%s.%d",
	    ar->ArFsname, ar->ArAsname, ar->ArSeqnum);
}


/*
 * Increase the size of an ArchReq.
 * RETURN: The larger ArchReq.
 */
struct ArchReq *
ArchReqGrow(
	struct ArchReq *ar)	/* ArchReq to grow */
{
	char	arname[ARCHREQ_NAME_SIZE];
	int	incr;

	/*
	 * No room for file entry.
	 * Double existing allocation until ARCH_INCR reached.
	 * Then add ARCH_INCR each time.
	 */
	if (ar->Ar.MfLen < ARCHREQ_INCR) {
		incr = ar->Ar.MfLen;
	} else {
		incr = ARCHREQ_INCR;
	}
	(void) ArchReqName(ar, arname);
	ar = (struct ArchReq *)MapFileGrow(ar, incr);
	if (ar == NULL) {
		LibFatal(MapFileGrow, arname);
	}
	Trace(TR_ALLOC, "Grow %s: %d, %u", arname, incr, ar->Ar.MfLen);
	return (ar);
}


/*
 * Enter message in ArchReq.
 */
void
ArchReqMsg(
	const char *srcFile,
	const int srcLine,
	struct ArchReq *ar,
	int msgNum,
	...)
{
	va_list	args;
	char	arname[ARCHREQ_NAME_SIZE];
	char	*fmt;

	/*
	 * Look up message in message catalog.
	 */
	va_start(args, msgNum);
	if (msgNum != 0) {
		fmt = GetCustMsg(msgNum);
	} else {
		fmt = va_arg(args, char *);
	}

	/*
	 * Do the message formatting.
	 */
	vsnprintf(ar->ArCpi[0].CiOprmsg, sizeof (ar->ArCpi[0].CiOprmsg),
	    fmt, args);
	va_end(args);
	_Trace(TR_oprmsg, srcFile, srcLine,
	    "%s - %s", ArchReqName(ar, arname), ar->ArCpi[0].CiOprmsg);
}


/*
 * Return the name of an ArchReq.
 * The name of the ArchReq is returned in a user's buffer.
 * The format is:  fsname.ArchiveSetName.SequenceNumber
 */
char *
ArchReqName(
	struct ArchReq *ar,
	char *arname)
{
	snprintf(arname, ARCHREQ_NAME_SIZE, "%s.%s.%d",
	    ar->ArFsname, ar->ArAsname, ar->ArSeqnum);
	return (arname);
}


/*
 * Print the ArchReq information to a file.
 */
void
ArchReqPrint(
	FILE *st,		/* File to print to */
	struct ArchReq *ar,	/* ArchReq to print */
	boolean_t showFiles)	/* TRUE to print individual file information */
{
	char	arname[ARCHREQ_NAME_SIZE];
	char	timestr[ISO_STR_FROM_TIME_BUF_SIZE];
	int	n;

	(void) TimeToIsoStr(ar->ArTime, timestr);
	n = ar->ArState;
	if (n < 0 || n > ARS_MAX) {
		n = 0;
	}

	/*
	 * List common information.
	 * Name, state, creation time [drives].
	 */
	fprintf(st, "%s %s %s", ArchReqName(ar, arname),
	    StateNames[n], timestr);
	if (ar->ArDrives > 1) {
		fprintf(st, " drives:%d", ar->ArDrives);
	}
	n = (ar->ArFiles != 0) ? ar->ArFiles : ar->ArCount;
	fprintf(st, "\n    files:%d space:%s", n,
	    StrFromFsize(ar->ArSpace, 3, NULL, 0));
	fprintf(st, " flags:");
	if (ar->ArStatus & ARF_rearch) {
		fprintf(st, " rearch");
	}
	for (n = 0; n <= NUMOF(ArFlagNames); n++) {
		if (ar->ArFlags & (1 << n)) {
			fprintf(st, " %s", ArFlagNames[n]);
		}
	}
	fprintf(st, "\n");

	if (ar->ArState == ARS_create) {
		/*
		 * Creating or composing an ArchReq.
		 */
		fprintf(st, "    %s\n", ar->ArCpi[0].CiOprmsg);
		if (showFiles) {
			printFileList(st, ar, -2);
		}

	} else if (ar->ArState == ARS_schedule) {
		boolean_t volsAssigned;

		/*
		 * ArchReq in schedule queue.
		 */
		if (ar->ArMinSpace != FSIZE_MAX) {
			fprintf(st, " (min: %s)", StrFromFsize(ar->ArMinSpace,
			    3, NULL, 0));
		}
		fprintf(st, "    priority: ");
		if (ar->ArPriority <= PR_MIN) {
			fprintf(st, "--");
		} else {
			fprintf(st, PR_FMT, ar->ArPriority);
		}
		fprintf(st, " ");
		if (ar->ArSchedPriority == PR_MIN) {
			fprintf(st, "--");
		} else {
			fprintf(st, PR_FMT, ar->ArSchedPriority);
		}
		if (ar->ArFlags & AR_offline) {
			fprintf(st, "  offline VSNs: %d", ar->ArStageVols);
		}
		fprintf(st, "\n    %s\n", ar->ArCpi[0].CiOprmsg);

		/*
		 * Show each copy instance.
		 */
		volsAssigned = FALSE;
		for (n = 0; n < ar->ArDrivesUsed; n++) {
			struct ArcopyInstance *ci;

			ci = &ar->ArCpi[n];
			fprintf(st, "    ");
			if (ar->ArDrivesUsed != 0) {
				fprintf(st, "Drive %d ", n + 1);
			}
			if (*ci->CiVsn != '\0') {
				volsAssigned = TRUE;
				fprintf(st, "Vol: %s.%s", ci->CiMtype,
				    ci->CiVsn);
			}
			if (*ci->CiOwner != '\0') {
				fprintf(st, "  owner:%s", ci->CiOwner);
			}
			fprintf(st, "\n");
			fprintf(st, "      Files: %d, bytes: %s", ci->CiFiles,
			    StrFromFsize(ci->CiSpace, 3, NULL, 0));
			if (ci->CiMinSpace != FSIZE_MAX) {
				fprintf(st, " (min: %s)",
				    StrFromFsize(ci->CiMinSpace, 3, NULL, 0));
			}
			fprintf(st, "\n");
			if (showFiles) {
				printFileList(st, ar, n);
			}
		}
		if (showFiles) {
			if (!volsAssigned) {
				printFileList(st, ar, -2);
			} else {
				/*
				 * Show unselected files.
				 */
				printFileList(st, ar, -1);
			}
		} else if (ar->ArFlags & AR_offline) {
			printVolumeList(st, ar);
		}

	} else if (ar->ArState == ARS_archive) {
		boolean_t first;

		/*
		 * Archiving.
		 * Show each copy instance.
		 */
		first = TRUE;
		for (n = 0; n < ar->ArDrivesUsed; n++) {
			struct ArcopyInstance *ci;

			ci = &ar->ArCpi[n];
			if (*ci->CiVsn == '\0') {
				continue;
			}
			if (!first) {
				fprintf(st, "\n");
			}
			first = FALSE;
			if (ci->CiArcopyNum < 0) {
				fprintf(st, "    Drive %d  Vol: %s.%s",
				    n + 1, ci->CiMtype, ci->CiVsn);
				if (*ci->CiOwner != '\0') {
					fprintf(st, "  owner:%s", ci->CiOwner);
				}
				fprintf(st, "\n");
				fprintf(st, "    Files: %d, bytes: %s",
				    ci->CiFiles,
				    StrFromFsize(ci->CiSpace, 3, NULL, 0));
				if (ci->CiMinSpace != FSIZE_MAX) {
					fprintf(st, " (min: %s)",
					    StrFromFsize(ci->CiMinSpace, 3,
					    NULL, 0));
				}
				fprintf(st, "\n");
			} else {
				fprintf(st, "    rm%d  Vol: %s.%s", ci->CiRmFn,
				    ci->CiMtype, ci->CiVsn);
				if (*ci->CiOwner != '\0') {
					fprintf(st, "  owner:%s", ci->CiOwner);
				}
				fprintf(st, "\n");
				fprintf(st, "    Archives: %d  files: %d (%d),"
				    " bytes: %s", ci->CiArchives,
				    ci->CiFilesWritten, ci->CiFiles,
				    StrFromFsize(ci->CiBytesWritten, 3,
				    NULL, 0));
				fprintf(st, " (%s)",
				    StrFromFsize(ci->CiSpace, 3, NULL, 0));
				fprintf(st, "\n");
				fprintf(st, "    %s\n", ci->CiOprmsg);
			}
			if (showFiles) {
				printFileList(st, ar, n);
			}
		}
	}

#if defined(AR_DEBUG)
	fprintf(st, "    count: %d, size:%d, alloc:%d\n",
	    ar->ArCount, ar->ArSize, ar->Ar.MfLen);
#endif /* defined(AR_DEBUG) */
#if defined(lint)
	fprintf(st, "%s %s %s %s %s %s %s %s %s\n",
	    Joins[0].EeName, OfflineCopies[0].EeName, Timeouts[0].EeName,
	    Reserves[0].RsName, Rsorts[0].EeName, Sorts[0].EeName,
	    CiFlagNames[0], DivideNames[0], FiFlagNames[0]);
#endif /* defined(lint) */
}


/*
 * Trace an ArchReq.
 */
void
ArchReqTrace(
	struct ArchReq *ar,
	boolean_t files)
{
	FILE	*st;

	st = TraceOpen();
	if (st != NULL) {
		ArchReqPrint(st, ar, files | (*TraceFlags & (1 << TR_files)));
	}
	TraceClose(-1);
}


/*
 * Check for valid ArchReq.
 */
boolean_t
ArchReqValid(
	struct ArchReq *ar)
{
	struct MappedFile *mf;
	struct stat st;
	char	arname[ARCHREQ_NAME_SIZE];
	char	fname[ARCHREQ_FNAME_SIZE];

	ArchReqFileName(ar, fname);
	if (stat(fname, &st) != 0) {
		Trace(TR_MISC, "Archreq invalid (%s) removed",
		    ArchReqName(ar, arname));
		return (FALSE);
	}
	mf = (struct MappedFile *)(void *)ar;
	if (mf->MfMagic != ARCHREQ_MAGIC) {
		Trace(TR_MISC, "Archreq invalid (%s) bad magic",
		    ArchReqName(ar, arname));
		return (FALSE);
	}
	if (st.st_size < sizeof (struct MappedFile) ||
	    st.st_size != mf->MfLen) {
		Trace(TR_MISC, "Archreq invalid (%s) bad size",
		    ArchReqName(ar, arname));
		return (FALSE);
	}
	if (ar->ArVersion != ARCHREQ_VERSION) {
		Trace(TR_MISC, "Archreq invalid (%s) version mismatch."
		    " found: %d expected: %d",
		    ArchReqName(ar, arname), ar->ArVersion, ARCHREQ_VERSION);
		return (FALSE);
	}
	if (ar->Ar.MfValid == 0) {
		Trace(TR_MISC, "Archreq invalid (%s) invalidated",
		    ArchReqName(ar, arname));
		return (FALSE);
	}
	if (ar->ArState < ARS_create || ar->ArState > ARS_done ||
	    *ar->ArAsname == '\0' || *ar->ArFsname == '\0' ||
	    ar->ArTime == 0 || ar->ArCount <= 0 || ar->ArDrives <= 0) {
		Trace(TR_MISC, "Archreq invalid (%s) bad values",
		    ArchReqName(ar, arname));
		return (FALSE);
	}
	return (TRUE);
}


/*
 * Print file information.
 */
static void
printFileInfo(
	FILE *st,
	struct FileInfo *fi,
	boolean_t first)
{
	static boolean_t totEnd;
	static fsize_t totSpace;
	static int archiveFile;
	static int totFiles;

	if (st == NULL) {
		archiveFile = 1;
		totEnd = TRUE;
		totFiles = 0;
		totSpace = 0;
		return;
	}

	/*
	 * Detect new archive file (tarball).
	 */
	if (fi == NULL || first) {
		if (archiveFile != 1) {
			fprintf(st, "       Total:  files %d, space %s\n",
			    totFiles, StrFromFsize(totSpace, 3, NULL, 0));
		}
		if (fi == NULL) {
			return;
		}
		totEnd = TRUE;
	}
	if (totEnd) {
		totEnd = FALSE;
		if (fi->FiCpi >= 0) {
			fprintf(st, "    Archive file %d:\n", archiveFile++);
		} else {
			fprintf(st, "    Not selected:\n");
		}
		totFiles = 0;
		totSpace = 0;
	}
	if (!(fi->FiFlags & FI_join)) {
		totFiles++;
		totSpace += fi->FiSpace;
	}

#if defined(AR_DEBUG)
	fprintf(st, " c:%2d ", fi->FiCpi);
#else /* defined(AR_DEBUG) */
	fprintf(st, "    ");
#endif /* defined(AR_DEBUG) */
	fprintf(st, "type:%c ino:%4d s:%x/f:%x space:%s time:%ld priority:",
	    fi->FiType,
	    fi->FiId.ino,
	    fi->FiStatus,
	    fi->FiFlags,
	    StrFromFsize(fi->FiSpace, 3, NULL, 0),
	    fi->FiModtime);
	if (fi->FiPriority == PR_MIN) {
		fprintf(st, "--");
	} else {
		fprintf(st, PR_FMT, fi->FiPriority);
	}
	fprintf(st, "\n        %s\n", fi->FiName);

	if (fi->FiStatus & FIF_offline) {
		struct OfflineInfo *oi;

		oi = LOC_OFFLINE_INFO(fi);
		fprintf(st, "         copy:%d position: %llx.%x %s.%s\n",
		    fi->FiStageCopy,
		    oi->OiPosition,
		    oi->OiOffset,
		    sam_mediatoa(oi->OiMedia),
		    oi->OiVsn);
	}
}


/*
 * Print files.
 */
static void
printFileList(
	FILE *st,
	struct ArchReq *ar,
	int cpi)	/* Copy index:  -2 show all; -1 show not selected */
{
	char	*fic;
	int	count;
	int	*fii;
	int	i;

	if (ar->ArCount == 0) {
		return;
	}
	fic = FIRST_FILE_INFO_ADDR(ar);
	if (ar->ArFileIndex != 0) {
		fii = LOC_FILE_INDEX(ar);
	} else {
		fii = NULL;
	}
	if (ar->ArState == ARS_create) {
		count = ar->ArCount;
	} else {
		printFileInfo(NULL, NULL, FALSE);
		count = ar->ArSelFiles;
	}
	for (i = 0; i < count; i++) {
		struct FileInfo *fi;

		if (fii != NULL) {
			if (fii != LOC_FILE_INDEX(ar)) {
				break;
			}
			fi = LOC_FILE_INFO(i);
		} else {
			fi = (struct FileInfo *)(void *)fic;
			fic += FI_SIZE(fi);
		}

		if (cpi == -2 || fi->FiCpi == cpi) {
			if (!(fi->FiFlags & FI_join)) {
				printFileInfo(st, fi, fi->FiFlags & FI_first);
			} else {
				int	end;
				int	j;

				printFileInfo(st, fi, TRUE);
				end = fi->FiJoinStart + fi->FiJoinCount;
				for (j = fi->FiJoinStart; j < end; j++) {
					fi = LOC_FILE_INFO(j);
					printFileInfo(st, fi, FALSE);
				}
			}
		}
	}
	if (ar->ArState != ARS_create) {
		printFileInfo(st, NULL, FALSE);
	}
}


/*
 * Print offline volume list.
 */
static void
printVolumeList(
	FILE *st,
	struct ArchReq *ar)
{
	char	*firstVsn;
	int	*fii;
	int	i;

	fii = LOC_FILE_INDEX(ar);
	fprintf(st, "  Stage volumes:\n");
	firstVsn = "";
	for (i = 0; i < ar->ArSelFiles; i++) {
		struct FileInfo *fi;

		fi = LOC_FILE_INFO(i);
		if (fi->FiStatus & FIF_offline) {
			struct OfflineInfo *oi;

			oi = LOC_OFFLINE_INFO(fi);
			if (strcmp(oi->OiVsn, firstVsn) != 0) {
				fprintf(st, "   %s.%s\n",
				    sam_mediatoa(oi->OiMedia), oi->OiVsn);
				firstVsn = oi->OiVsn;
			}
		}
	}
}
