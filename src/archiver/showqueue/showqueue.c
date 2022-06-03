/*
 * showqueue.c - display the content of an archive queue file.
 *
 * showqueue reads the files named in the argument list and prints the
 * queue file information.
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

#pragma ident "$Revision: 1.64 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

/* Solaris headers. */
#include <sys/param.h>

/* SAM-FS headers. */
#define DEC_INIT
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/exit.h"
#include "sam/mount.h"
#include "sam/lib.h"

/* Local headers. */
#define	NEED_ARCHREQ_NAMES
#define	NEED_ARFIND_EVENT_NAMES
#define	NEED_EXAM_METHOD_NAMES
#define	DEC_INIT
#include "common.h"
#include "archset.h"
#include "archreq.h"
#include "device.h"	/* To statisfy device.c requirements in libarch */
#include "dir_inode.h"
#include "utility.h"
#include "../arfind/examlist.h"
#include "../arfind/scanlist.h"

/* Private data. */
static boolean_t debug = FALSE;
static boolean_t showArchReqs = TRUE;
static boolean_t showExamlist = TRUE;
static boolean_t showScanlist = TRUE;
static boolean_t showStateFile = TRUE;
static boolean_t useCwd = FALSE;
static boolean_t verbose = FALSE;
static upath_t arfindDir = "./";
static char ts[ISO_STR_FROM_TIME_BUF_SIZE];

/* Private functions. */
static int cmp_slTime(const void *p1, const void *p2);
static void printArchReq(char *arname);
static int printArchReqs(void);
static void printDebugArchReq(struct ArchReq *ar, char *arname);
static void printExamlist(void);
static void printScanlist(void);
static void printStateFile(void);
static void showFileSystem(char *fsname);

/* Public functions. */
extern char *IdToPathId(sam_id_t id);
extern void IdToPathInit(void);


int
main(
	int argc,	/* Number of arguments */
	char *argv[])	/* Argument pointer list */
{
	extern int optind;
	boolean_t asSet = FALSE;
	boolean_t follow = FALSE;
	char	*queueFile = NULL;
	int	c;
	int	errflag = 0;
	int	optindSave;

	program_name = "showqueue";
	while ((c = getopt(argc, argv, "acdefq:sv")) != EOF) {
		switch (c) {
		case 'a':
			if (!asSet) {
				showExamlist = FALSE;
				showScanlist = FALSE;
				showStateFile = B_FALSE;
			}
			asSet = TRUE;
			showArchReqs = TRUE;
			break;
		case 'c':
			useCwd = TRUE;
			break;
		case 'd':
			debug = TRUE;
			break;
		case 'e':
			if (!asSet) {
				showArchReqs = FALSE;
				showScanlist = FALSE;
			}
			asSet = TRUE;
			showExamlist = TRUE;
			break;
		case 'f':
			follow = TRUE;
			break;
		case 'q':
			queueFile = optarg;
			break;
		case 's':
			if (!asSet) {
				showArchReqs = FALSE;
				showExamlist = FALSE;
			}
			asSet = TRUE;
			showScanlist = TRUE;
			break;
		case 'v':
			verbose = TRUE;
			break;
		case '?':
		default:
			errflag++;
			break;
		}
	}
	if (queueFile != NULL && follow && optind != argc) {
		errflag++;
	}
	if (errflag > 0) {
		fprintf(stderr, GetCustMsg(13001), "showqueue",
		    "[-a] [-d] [-f] [-s] [-v] [file_system [archreq ...] ]");
		fprintf(stderr, GetCustMsg(13001), "showqueue",
		    "[-d] [-v] -q archreq");
		exit(EXIT_USAGE);
	}

	optindSave = optind;
	IdToPathInit();
	FsFd = -1;

repeat:
	optind = optindSave;

	(void) ArchSetAttach(O_RDONLY);
	if (queueFile != NULL) {
		printArchReq(queueFile);
		return (EXIT_SUCCESS);
	}

	if (useCwd) {
		snprintf(ScrPath, sizeof (ScrPath), "%s"ARFIND_STATE,
		    arfindDir);
		printStateFile();
		if (showExamlist) {
			printExamlist();
		}
		if (showScanlist) {
			printScanlist();
		}
		if (showArchReqs) {
			(void) printArchReqs();
		}
		return (EXIT_SUCCESS);
	}

	if (optind == argc) {
		/*
		 * Do all filesystems.
		 */
		struct sam_fs_status *fsarray;
		int	i;
		int	numFilesystems;

		if ((numFilesystems = GetFsStatus(&fsarray)) == -1) {
			/* GetFsStatus() failed */
			error(EXIT_FAILURE, errno, GetCustMsg(4800));
		}

		if (numFilesystems == 0) {
			/* No samfs filesystems */
			error(EXIT_FAILURE, errno, GetCustMsg(4801));
		}
		for (i = 0; i < numFilesystems; i++) {
			showFileSystem(fsarray[i].fs_name);
		}
	} else {
		char *fsname;

		fsname = argv[optind++];
		if (optind == argc) {
			/*
			 * Do this one filesystem.
			 */
			showFileSystem(fsname);
		} else {
			/* Archive requests */
			printf(GetCustMsg(4803));
			printf("\n");
			snprintf(arfindDir, sizeof (arfindDir),
			    ARCHIVER_DIR"/%s/", fsname);
			while (optind < argc) {
				printArchReq(argv[optind++]);
			}
		}
	}

	if (follow) {
		sleep(5);
		goto repeat;
	}
	return (EXIT_SUCCESS);
}


/*
 * Compare scanlist entry times ascending.
 */
static int
cmp_slTime(
	const void *p1,
	const void *p2)
{
	struct ScanListEntry **se1 = (struct ScanListEntry **)p1;
	struct ScanListEntry **se2 = (struct ScanListEntry **)p2;

	if ((*se1)->SeTime > (*se2)->SeTime) {
		return (1);
	}
	if ((*se1)->SeTime < (*se2)->SeTime) {
		return (-1);
	}
	return (0);
}


/*
 * Print debug information for a queue file.
 */
static void
printDebugArchReq(
	struct ArchReq *ar,
	char *arname)
{
	int	i;

#define	PRI1(a, b)  printf("%-10.10s%15d", a, b);
#define	PRI2(a, b)  printf("          %-15.10s%15d\n", a, b);
#define	PRX1(a, b)  printf("%-10.10s%15x", a, b);
#define	PRX2(a, b)  printf("          %-15.10s%15x\n", a, b);
#define	PRLL1(a, b) printf("%-10.10s%15lld", a, b);
#define	PRLL2(a, b) printf("          %-15.10s%15lld\n", a, b);
#define	PRS1(a, b)  printf("%-10.10s%15s", a, b);
#define	PRS2(a, b)  printf("          %-15.10s%15s\n", a, b);
	i = ar->ArState;
	if (i < 0 || i > ARS_done) {
		i = 0;
	}

	printf("[ ArchReq %s ]\n", arname);
	PRS1("Fsname:",		ar->ArFsname);
	PRI2("Version:",	ar->ArVersion);
	PRS1("Asname:",		ar->ArAsname);
	PRS2("State:",		StateNames[i]);
	PRI1("Seqnum:",		ar->ArSeqnum);
	PRX2("Flags:",		ar->ArFlags);
	PRI1("Count:",		ar->ArCount);
	PRI2("Size:",		ar->ArSize);
	PRI1("Drives:",		ar->ArDrives);
	PRX2("Status:",		ar->ArStatus);

	PRI1("Files:",		ar->ArFiles);
	PRLL2("Space:",		ar->ArSpace);
	PRI1("SelFiles:",	ar->ArSelFiles);
	PRLL2("SelSpace:",	ar->ArSelSpace);
	PRI1("DrivesUsed:",	ar->ArDrivesUsed);
	PRLL2("MinSpace:",	ar->ArMinSpace);
	PRX1("MsgSent:",	ar->ArMsgSent);
	PRI1("StageVols:",	ar->ArStageVols);
	printf("\n");

	if (ar->ArState == ARS_archive) {
		printf("[ ArcopyInstance ] for # drives\n");
		for (i = 0; i < ar->ArDrives; i++) {
			printf("ArCpi[%d] ----------------------\n", i);
			PRI1("Pid:",		(int)ar->ArCpi[i].CiPid);
			PRI2("ArcopyNum:",	ar->ArCpi[i].CiArcopyNum);
			PRLL1("MinSpace:",	ar->ArCpi[i].CiMinSpace);
			PRLL1("Space:",		ar->ArCpi[i].CiSpace);
			PRLL2("BytesWritten:",	ar->ArCpi[i].CiBytesWritten);
			PRI1("Files:",		ar->ArCpi[i].CiFiles);
			PRI2("FilesWritten:",	ar->ArCpi[i].CiFilesWritten);
			PRI1("Archives:",	ar->ArCpi[i].CiArchives);
			PRI2("LibDev:",		ar->ArCpi[i].CiLibDev);
			PRI1("Aln:",		ar->ArCpi[i].CiAln);
			PRI2("Slot:",		ar->ArCpi[i].CiSlot);
			PRX1("Flags:",		ar->ArCpi[i].CiFlags);
			PRS2("Owner:",		ar->ArCpi[i].CiOwner);
			PRS1("Mtype:",		ar->ArCpi[i].CiMtype);
			PRS2("Vsn:",		ar->ArCpi[i].CiVsn);
			PRI1("RmFn:",		ar->ArCpi[i].CiRmFn);
			printf("\n");
			printf("CiOprmsg: %s\n",	ar->ArCpi[i].CiOprmsg);
		}
	}

	if (verbose && ar->ArCount != 0) {
		char	*fic;
		int	i;

		/*
		 * Step through the files and list them.
		 */
		fic = FIRST_FILE_INFO_ADDR(ar);
		for (i = 0; i < ar->ArCount; i++) {
			struct FileInfo *fi;

			fi = (struct FileInfo *)(void *)fic;
			fic += FI_SIZE(fi);
			printf(" c:%2d type:%c ino:%4d s:%x/f:%x"
			    " space:%s time:%ld priority:",
			    fi->FiCpi,
			    fi->FiType,
			    fi->FiId.ino,
			    fi->FiStatus,
			    fi->FiFlags,
			    StrFromFsize(fi->FiSpace, 3, NULL, 0),
			    fi->FiModtime);
			if (fi->FiPriority == PR_MIN) {
				printf("--");
			} else {
				printf(PR_FMT, fi->FiPriority);
			}
			printf("\n        %s\n", fi->FiName);

			if (fi->FiStatus & FIF_offline) {
				struct OfflineInfo *oi;

				oi = LOC_OFFLINE_INFO(fi);
				printf("         copy:%d"
				    " position: %llx.%x %s.%s\n",
				    fi->FiStageCopy,
				    oi->OiPosition,
				    oi->OiOffset,
				    sam_mediatoa(oi->OiMedia),
				    oi->OiVsn);
			}
		}
	}

#if defined(lint)
	printf("%s %s %s %s", ArFlagNames[0], CiFlagNames[0], DivideNames[0],
	    FiFlagNames[0]);
#endif /* defined(lint) */
}


/*
 * Print a single queue file.
 */
static void
printArchReq(
	char *arname)
{
	struct ArchReq *ar;

	ar = ArMapFileAttach(arname, ARCHREQ_MAGIC, O_RDONLY);
	if (ar == NULL) {
		/* Cannot access queue file %s %s */
		error(0, errno, GetCustMsg(4802), "", arname);
		return;
	}
	if (debug) {
		printDebugArchReq(ar, arname);
	} else {
		if (!ArchReqValid(ar)) {
			/* Invalid ArchReq %s */
			error(0, 0, GetCustMsg(4810), arname);
		} else {
			ArchReqPrint(stdout, ar, verbose);
		}
	}
	(void) ArMapFileDetach(ar);
	printf("\n");
}


/*
 * Print all the queue files.
 */
static int
printArchReqs(void)
{
	boolean_t first;
	DIR		*dirp;
	struct dirent *dirent;
	char	*p;

	snprintf(ScrPath, sizeof (ScrPath), "%s"ARCHREQ_DIR, arfindDir);
	p = ScrPath + strlen(ScrPath);
	if ((dirp = opendir(ScrPath)) == NULL) {
		/* Cannot opendir %s */
		error(EXIT_FAILURE, errno, GetCustMsg(622), ScrPath);
	}
	first = TRUE;
	while ((dirent = readdir(dirp)) != NULL) {
		if (*dirent->d_name != '.') {
			if (first) {
				first = FALSE;
				/* Archive requests */
				printf(GetCustMsg(4803));
				printf("\n");
			}
			snprintf(p, sizeof (ScrPath) - Ptrdiff(p, ScrPath),
			    "/%s", dirent->d_name);
			printArchReq(ScrPath);
		}
	}
	if (closedir(dirp) == -1) {
		/* Cannot closedir %s */
		error(EXIT_FAILURE, errno, GetCustMsg(623), ScrPath);
	}
	return (first);
}
/*
 * Print examlist.
 */
static void
printExamlist(void)
{
	struct ExamList *examList;
	int	i;

	printf("Exam list:");
	snprintf(ScrPath, sizeof (ScrPath), "%s"EXAMLIST, arfindDir);
	examList = ArMapFileAttach(ScrPath, EXAMLIST_MAGIC, O_RDONLY);
	if (examList == NULL) {
		printf(" none\n");
		return;
	}
	if (examList->ElCount != 0) {
		if (debug) {
			printf(" count %d size %d free %d\n",
			    examList->ElCount, examList->ElSize,
			    examList->ElFree);
		} else {
			printf(" %d entries\n",
			    examList->ElCount - examList->ElFree);
		}

		if (verbose) {
			for (i = 0; i < examList->ElCount; i++) {
				struct ExamListEntry *xe;

				xe = &examList->ElEntry[i];
				if (!(xe->XeFlags & XE_free)) {
					printf(" %s",
					    TimeToIsoStr(xe->XeTime, ts));
					if (FsFd != -1) {
						char	*path;

						path = IdToPathId(xe->XeId);
						printf(" %s",
						    ScanPathToMsg(path));
					} else {
						printf(" %d.%d",
						    xe->XeId.ino, xe->XeId.gen);
					}
					printf("\n");
				} else if (debug) {
					printf(" %3d %04x", i, xe->XeFlags);
					printf(" %d.%d %3d\n",
					    xe->XeId.ino, xe->XeId.gen,
					    (int)xe->XeTime);
				}
			}
		}
	} else {
		printf(" empty\n");
	}
	printf("\n");
	(void) ArMapFileDetach(examList);
}



/*
 * Print scanlist.
 */
static void
printScanlist(void)
{
	static struct ScanListEntry **seList = NULL;
	static size_t alloc = 0;
	struct ScanList *scanList;
	struct ScanListEntry *se;
	size_t  size;
	int		i;
	int	seListCount;

	/* Scan list */
	printf("%s", GetCustMsg(4811));
	snprintf(ScrPath, sizeof (ScrPath), "%s"SCANLIST, arfindDir);
	scanList = ArMapFileAttach(ScrPath, SCANLIST_MAGIC, O_RDONLY);
	if (scanList == NULL || scanList->SlVersion != SCANLIST_VERSION) {
		/* none */
		printf(": %s\n", GetCustMsg(4812));
		goto out;
	}
	/* Examine */
	printf("  %s: %s\n", GetCustMsg(4634),
	    ExamMethodNames[scanList->SlExamine]);
	if (scanList->SlCount == 0) {
		/* empty */
		printf(": %s\n", GetCustMsg(4813));
		goto out;
	}

	/*
	 * Sort scanlist by time.
	 */
	size = scanList->SlCount * sizeof (struct ScanListEntry *);
	if (size > alloc) {
		alloc = size;
		SamRealloc(seList, alloc);
	}
	seListCount = 0;
	if (scanList->SlCount > 1) {
		se = &scanList->SlEntry[0];
		for (i = 0; i < scanList->SlCount; i++) {
			if (!(se->SeFlags & SE_free)) {
				seList[seListCount++] = se;
			}
			se++;
		}
	}

	if (seListCount <= 1) {
		seListCount = 1;
		seList[0] = &scanList->SlEntry[0];
	} else {
		qsort(seList, seListCount, sizeof (struct ScanListEntry *),
		    cmp_slTime);
	}

	for (i = 0; i < seListCount; i++) {
		struct ScanListEntry *se;

		se = seList[i];
		if (se->SeFlags & SE_scanning) {
			/* Scanning */
			printf("    %-19s", GetCustMsg(4814));
		} else {
			printf("%3d %s", i, TimeToIsoStr(se->SeTime, ts));
		}
		if (scanList->SlExamine >= EM_noscan) {
			char	*asname;

			if (se->SeFlags & SE_back) {
				asname = "background";
			} else if (ArchSetTable != NULL) {
				asname = ArchSetTable[se->SeAsn].AsName;
			} else {
				asname = "??";
			}
			printf(" %-16s", asname);
			if (se->SeCopies != 0) {
				printf(" %c%c%c%c",
				    (se->SeCopies & 0x1) ? '1' : '_',
				    (se->SeCopies & 0x2) ? '2' : '_',
				    (se->SeCopies & 0x4) ? '3' : '_',
				    (se->SeCopies & 0x8) ? '4' : '_');
			} else {
				printf(" ----");
			}
		}
		if (se->SeFlags & SE_inodes) {
			printf(" inodes");
		} else if (FsFd != -1) {
			char	*path;

			path = IdToPathId(se->SeId);
			printf(" %s", ScanPathToMsg(path));
		} else {
			printf(" %d", se->SeId.ino);
		}
		printf("\n");
	}

out:
	if (scanList != NULL) {
		(void) ArMapFileDetach(scanList);
	}
}


/*
 * Print content of state file.
 */
static void
printStateFile(void)
{
	struct ArfindState *af;
	int	i;

	snprintf(ScrPath, sizeof (ScrPath), "%s"ARFIND_STATE, arfindDir);
	af = ArMapFileAttach(ScrPath, AF_MAGIC, O_RDONLY);
	if (af == NULL) {
		return;
	}
	printf("Files waiting to start: %10s\n",
	    CountToA((uint64_t)af->AfFilesCreate));
	printf("Files being scheduled:  %10s\n",
	    CountToA((uint64_t)af->AfFilesSchedule));
	printf("Files archiving:        %10s\n",
	    CountToA((uint64_t)af->AfFilesArchive));
	printf("Events processed:       %10s\n",
	    CountToA((uint64_t)af->AfFsactEvents));
	for (i = 1; i < AE_MAX + 1; i++) {
		printf("    %-10s %10s\n", fileEventNames[i],
		    CountToA((uint64_t)af->AfEvents[i]));
	}
	if (debug) {
		printf("Id2path: %10s\n",
		    CountToA((uint64_t)af->AfId2path));
		printf("    cached:  %10s\n",
		    CountToA((uint64_t)af->AfId2pathCached));
		printf("    idstat:  %10s\n",
		    CountToA((uint64_t)af->AfId2pathIdstat));
		printf("    readdir: %10s\n",
		    CountToA((uint64_t)af->AfId2pathReaddir));
	}
	(void) ArMapFileDetach(af);
}


/*
 * Print all queue files for a filesystem.
 * Enter all previous queue files into the queue.
 */
static void
showFileSystem(
	char *fsname)
{
	struct sam_fs_info fi;
	char	*msg;

	if (GetFsInfo(fsname, &fi) == -1) {
		/* GetFsInfo(%s) failed */
		error(EXIT_FAILURE, errno, GetCustMsg(4804), fsname);
	}
	/* Filesystem %s: */
	printf(GetCustMsg(4805), fsname);
	printf("  ");
	msg = NULL;
	if (!(fi.fi_config & MT_SAM_ENABLED)) {
		/* Filesystem defined "archive = off".  Cannot archive. */
		msg = GetCustMsg(4338);

	} else if (!(fi.fi_config & MT_ARCHIVE_SCAN)) {
		/* noarscan enabled */
		msg = GetCustMsg(4362);

	} else if (fi.fi_status & FS_CLIENT) {
		/* Shared file system client.  Cannot archive. */
		msg = GetCustMsg(4351);

	} else if (fi.fi_config & MT_SHARED_READER) {
		/* Shared reader file system.  Cannot archive. */
		msg = GetCustMsg(4350);

	} else if (!(fi.fi_status & FS_MOUNTED) ||
	    (fi.fi_status & FS_UMOUNT_IN_PROGRESS)) {
		/* not mounted */
		msg = GetCustMsg(4807);
	}

	if (msg != NULL) {
		printf("\n  %s\n", msg);
		return;
	}

	printf("\n");
	MntPoint = fi.fi_mnt_point;
	FsFd = OpenInodesFile(MntPoint);
	snprintf(arfindDir, sizeof (arfindDir), ARCHIVER_DIR"/%s/", fsname);
	if (showStateFile == B_TRUE) {
		printStateFile();
	}
	if (showExamlist) {
		printExamlist();
	}
	if (showScanlist) {
		printScanlist();
	}
	if (FsFd >= 0) {
		(void) close(FsFd);
	}

	if (!showArchReqs) {
		return;
	}
	if (printArchReqs() && (fi.fi_status & FS_MOUNTED)) {
		/* No archive requests */
		printf(GetCustMsg(4808));
		printf("\n");
	}
}
