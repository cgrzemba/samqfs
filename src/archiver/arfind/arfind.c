/*
 * arfind.c - Arfind main program.
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

#pragma ident "$Revision: 1.93 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/param.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/times.h>

/* Solaris headers. */
#include <sys/vfs.h>

/* SAM-FS headers. */
#include "sam/exit.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/signals.h"
#include "sam/syscall.h"
#include "sam/fs/sblk.h"

/* Local headers. */
#define	DEC_INIT
#include "arfind.h"
#undef	DEC_INIT

/* Private data. */

/* Signals control. */
static void sigAlarm(int sig);
static void sigFatal(int sig);
static void sigHup(int sig);
static void sigTerm(int sig);

static struct SignalFunc signalFunc[] = {
	{ SIGALRM, sigAlarm },
	{ SIGHUP, sigHup },
	{ SIGINT, sigTerm },
	{ SIGTERM, sigTerm },
	{ 0 }
};

static struct SignalsArg signalsArg = { signalFunc, sigFatal, "." };

/*
 * Threads control.
 * Table of threads to start and stop.
 */
static struct Threads threads[] = {
	{ "Signals" },
	{ "Archive", Archive },
	{ "Examinodes", ExamInodes },
	{ "FsAct", },
	{ "FsExamine", FsExamine },
	{ "Message", Message },
	{ "Scanfs", Scanfs },
	{ NULL }
};

static clock_t startRtime;
static int stopSignal = 0;

/* Private functions. */
static void arfindInit(char *argv[]);
static void atExit(void);
static void attachFileProps(void);
static void reconfig(ReconfigControl_t ctrl);


int
main(
	int argc,
	char *argv[])
{
	if (strcmp(GetParentName(), SAM_ARCHIVER) != 0) {
#if !defined(TEST_W_DBX)
		fprintf(stderr, "sam-arfind may be started only from %s\n",
		    SAM_ARCHIVER);
		exit(EXIT_FAILURE);
	}
#else /* !defined(TEST_W_DBX) */
		Daemon = FALSE;
	}
	while (Daemon) {
		sleep(10000);
	}
#endif /* !defined(TEST_W_DBX) */

	if (argc < 2) {
		fprintf(stderr, "Usage: %s filesystem\n", argv[0]);
		exit(EXIT_USAGE);
	}

	/*
	 * Perform initialization.
	 */
	Exec = ES_init;
	snprintf(signalsArg.SaCoreDir, sizeof (signalsArg.SaCoreDir),
	    SAM_ARCHIVERD_HOME"/%s", argv[1]);
	threads[0].TtThread = Signals(&signalsArg);
	arfindInit(argv);
	ThreadsStart(threads, reconfig);
	if (Exec < ES_umount) {
		ChangeState(State->AfExec);
		alarm(1);	/* Start the timer */
		FsAct();
	}
	ThreadsStop();
	if (stopSignal != 0) {
		Trace(TR_MISC, "Stopped by %s", StrSignal(stopSignal));
	}
	if (State->AfNormalExit) {
		State->AfExitTime = time(NULL);
	}
	return (EXIT_SUCCESS);
}


/*
 * Change the archiving state.
 */
void
ChangeState(
	ExecState_t newState)
{
	/*
	 * Only change the state for running if not trying to unmount.
	 */
	if (newState != Exec && (Exec < ES_umount || newState >= ES_term ||
	    (newState & ES_force))) {
		newState &= ~ES_force;
		Trace(TR_MISC, "State changed from %s to %s",
		    ExecStateToStr(Exec), ExecStateToStr(newState));
		Exec = newState;
	}
}


/*
 * Trace times.
 */
void
TraceTimes(void)
{
	struct tms tms;
	clock_t	rtime;
	double	real;
	double	sys;
	double	user;
	int	min;

	rtime = times(&tms);
	real = (double)(rtime - startRtime) / (double)CLK_TCK;
	sys = (double)tms.tms_stime / (double)CLK_TCK;
	user = (double)tms.tms_utime / (double)CLK_TCK;
	Trace(TR_MISC, "real %d  user %d  sys %d",
	    (int)real, (int)user, (int)sys);
	min = (int)(real / (double)60);
	Trace(TR_MISC, "real   %d:%3.1f", min, real - (double)(min * 60));
	min = (int)(user / (double)60);
	Trace(TR_MISC, "user   %d:%3.1f", min, user - (double)(min * 60));
	min = (int)(sys / (double)60);
	Trace(TR_MISC, "sys    %d:%3.1f", min, sys - (double)(min * 60));
}


/* Private functions. */


/*
 * Initialize program.
 */
static void
arfindInit(
	char *argv[])
{
	static struct sam_fs_info fi;
	sam_fssbinfo_arg_t sbinfo_arg;
	sam_sbinfo_t sbInfo;
	struct tms tms;
	char *fname;
	char ts[ISO_STR_FROM_TIME_BUF_SIZE];
	int i;

	/*
	 * Set name for trace messages.
	 */
	FsName = argv[1];
	snprintf(ScrPath, sizeof (ScrPath), "af-%s", FsName);
	SamStrdup(program_name, ScrPath);
	CustmsgInit(Daemon, NotifyRequest);
	if (Daemon) {
		TraceInit(program_name, TI_archiver | TR_MPLOCK);
	} else {
		/*
		 * Allow trace messages to go to stdout/stderr.
		 */
		TraceInit(NULL, TI_none);
		*TraceFlags = (1 << TR_module) | ((1 << TR_date) - 1);
		*TraceFlags &= ~((1 << TR_alloc) | (1 << TR_oprmsg));
	}
#if defined(AR_DEBUG)
	*TraceFlags |= 1 << TR_ardebug;
#endif /* defined(AR_DEBUG) */
	Trace(TR_MISC, "Start: %s", FsName);
	atexit(atExit);

	/*
	 * Move to execution directory.
	 */
	if (chdir(FsName) == -1) {
		LibFatal(chdir, FsName);
	}

	/*
	 * Attach archiver files.
	 */
	fname = ARCHIVER_DIR"/"ARCHIVER_STATE;
	if ((AdState = ArMapFileAttach(fname, AD_MAGIC, O_RDWR)) == NULL) {
		LibFatal(ArMapFileAttach, fname);
	}

	/*
	 * Attach the state file.
	 */
	if ((State = ArMapFileAttach(ARFIND_STATE, AF_MAGIC, O_RDWR)) == NULL) {
		LibFatal(ArMapFileAttach, ARFIND_STATE);
	}
	AfState = State;
	State->AfPid = getpid();
	OpenOprMsg(State->AfOprMsg);
	/* Initializing */
	PostOprMsg(4327);

	/*
	 * Set mount point.
	 */
	if (GetFsInfo(FsName, &fi) == -1) {
		LibFatal(GetFsInfo, FsName);
	}
	if (!(fi.fi_status & FS_MOUNTED) ||
	    (fi.fi_status & FS_UMOUNT_IN_PROGRESS)) {
		SendCustMsg(HERE, 4009, FsName);
		exit(EXIT_NORESTART);
	}
	MntPoint = fi.fi_mnt_point;

	/*
	 * Get superblock info.
	 */
#if defined(TEST_W_DBX)
	State->AfSbInit = 0;
#endif /* defined(TEST_W_DBX) */
	sbinfo_arg.sbinfo.ptr = &sbInfo;
	sbinfo_arg.fseq = fi.fi_eq;
	if (sam_syscall(SC_fssbinfo, &sbinfo_arg, sizeof (sbinfo_arg)) < 0) {
		LibFatal(SC_fssbinfo, FsName);
	}
	Recover = TRUE;
	if (State->AfSbInit != sbInfo.init) {
		/*
		 * File system remade since last execution.
		 */
		State->AfSbInit = sbInfo.init;
		State->AfSeqNum = 0;
		memset(&State->AfStats, 0, sizeof (State->AfStats));
		memset(&State->AfStatsScan, 0, sizeof (State->AfStatsScan));
		Recover = FALSE;
	} else {
		if (!State->AfNormalExit) {
			/*
			 * Last execution was not stopped normally.
			 */
			Recover = FALSE;
			Trace(TR_MISC,
			    "Started after abnormal termination -"
			    " full scan begun.");
		} else if (sbInfo.time > State->AfExitTime) {
			/*
			 * Super block changed after last exit.
			 */
			Recover = FALSE;
			Trace(TR_MISC,
			    "File system modified since last execution -"
			    " full scan begun.");
			Trace(TR_DEBUG, "%d seconds",
			    sbInfo.time - State->AfExitTime);
		}
	}
	Trace(TR_MISC, "%s initialized on %s", FsName,
	    TimeToIsoStr(State->AfSbInit, ts));

	/*
	 * Initialize state file.
	 */
	State->AfNormalExit = FALSE;
	State->AfStatsScan.total.numof = -1;
	State->AfStartTime = time(NULL);
	State->AfGetdents = 0;
	State->AfIdstat = 0;
	State->AfOpendir = 0;

	/* Clear Id2Path counters */
	State->AfId2path = 0;
	State->AfId2pathCached = 0;
	State->AfId2pathIdstat = 0;
	State->AfId2pathReaddir = 0;

	/* Clear event counters */
	State->AfFsactEvents = 0;
	State->AfFsactCalls = 0;
	State->AfFsactBufs = 0;
	for (i = 1; i < AE_MAX; i++) {
		State->AfEvents[i] = 0;
	}

	startRtime = times(&tms);

	attachFileProps();

	/*
	 * Initialize non-thread modules.
	 */
	IdToPathInit();
	ScanDirsInit();
}


/*
 * Clean up everything on an exit().
 */
static void
atExit(void)
{
	if (LogStream != NULL) {
		(void) fclose(LogStream);
		LogStream = NULL;
	}
	if (State != NULL) {
		State->AfPid = 0;
		ClearOprMsg();
	}
}


/*
 * Attach file properties.
 */
void
attachFileProps(void)
{
	static struct FileProps fpEmpty;
	struct ArchSetFileHdr *oldArchSetFile;
	struct ArchSet *oldArchSetTable;
	struct FileProps *oldFileProps;
	struct FilePropsEntry *fp;
	int	deletes;
	int	fpNum;
	int	fpoNum;

	if (FileProps != NULL && FileProps->Fp.MfValid != 0 &&
	    ArchSetFile != NULL && ArchSetFile->As.MfValid != 0) {
		/*
		 * FileProps and ArchiveSets not changed.
		 */
		return;
	}
	if (FileProps == &fpEmpty) {
		FileProps = NULL;
	}

	/*
	 * Check the file properties file.
	 */
	oldFileProps = NULL;
	oldArchSetFile = NULL;
	if (ArchSetMap != NULL) {
		SamFree(ArchSetMap);
		ArchSetMap = NULL;
	}
	if (FileProps == NULL || FileProps->Fp.MfValid == 0) {
		oldFileProps = FileProps;
		if ((FileProps =
		    ArMapFileAttach(FILE_PROPS, FP_MAGIC, O_RDWR)) == NULL) {
			Trace(TR_DEBUG, "FileProps not available");
		} else if (FileProps->FpVersion != FP_VERSION) {
			Trace(TR_DEBUG, "FileProps version mismatch."
			    " Is: %d, should be: %d",
			    FileProps->FpVersion, FP_VERSION);
			(void) ArMapFileDetach(FileProps);
			FileProps = NULL;
		}
		if (FileProps == NULL) {
			FileProps = &fpEmpty;
			FileProps->FpCount = 1;
			ChangeState(ES_init);
			return;
		}
		ClassifyInit();
	}

	/*
	 * Check the Archive Sets file.
	 */
	if (ArchSetFile == NULL || ArchSetFile->As.MfValid == 0) {
		int	oldArchSetNumof;

		oldArchSetFile = ArchSetFile;
		oldArchSetTable = ArchSetTable;
		oldArchSetNumof = ArchSetNumof;
		ArchSetFile = NULL;
		if (ArchSetAttach(O_RDONLY) == NULL) {
			Trace(TR_DEBUG, "ArchSetFile not available");
			ArchSetFile = oldArchSetFile;
			ChangeState(ES_init);
			return;
		}
		if (oldArchSetFile != NULL) {
			struct ArchSet *asOld;
			int	asNum;
			int	i;

			/*
			 * Archive set file changed.
			 * Make remapping table.
			 */
			Trace(TR_ARDEBUG, "oldArchSetFile %d", oldArchSetNumof);
			asNum = max(oldArchSetNumof, ArchSetNumof);
			SamMalloc(ArchSetMap, asNum * sizeof (int));
			memset(ArchSetMap, 0, asNum * sizeof (int));
			for (i = 0, asOld = oldArchSetTable;
			    i < oldArchSetNumof; i++, asOld++) {
				struct ArchSet *as;
				int	j;

				ArchSetMap[i] = -1;
				for (j = 0, as = ArchSetTable; j < ArchSetNumof;
				    j++, as++) {
					if (strcmp(asOld->AsName,
					    as->AsName) == 0) {
						ArchSetMap[i] = j;
						break;
					}
				}
				Trace(TR_ARDEBUG, "%d %s %d %d",
				    i, asOld->AsName,
				    ArchSetMap[i], as - ArchSetTable);
			}
		}
	}

	if (oldFileProps == NULL) {
		goto out;
	}

	/*
	 * Check to see what has changed.
	 * Look up all old entries in new FileProps.
	 */
	Trace(TR_MISC, "Reconfigure.");
	Trace(TR_ARDEBUG, "oldFileProps %d", oldFileProps->FpCount);
	for (fpNum = 0; fpNum < FileProps->FpCount; fpNum++) {
		FileProps->FpEntry[fpNum].FpFlags |= FP_add;
	}
	deletes = 0;
	for (fpoNum = 0; fpoNum < oldFileProps->FpCount; fpoNum++) {
		struct FilePropsEntry *fpo;
		boolean_t removed;
		char	*asname;

		fpo = &oldFileProps->FpEntry[fpoNum];

		/*
		 * Free allocated storage.
		 */
		if (fpo->FpRegexp != NULL) {
			free(fpo->FpRegexp);
			fpo->FpRegexp = NULL;
		}

		/*
		 * Look up old entry in new FileProps.
		 */
		if (oldArchSetFile != NULL) {
			asname = oldArchSetTable[fpo->FpBaseAsn].AsName;
		} else {
			asname = ArchSetTable[fpo->FpBaseAsn].AsName;
		}
		removed = TRUE;
		for (fpNum = 0; fpNum < FileProps->FpCount; fpNum++) {
			int	copy;

			fp = &FileProps->FpEntry[fpNum];
	/* N.B. Bad indentation to meet cstyle requirements */
	if (strcmp(ArchSetTable[fp->FpBaseAsn].AsName, asname) != 0 ||
	    strcmp(fp->FpPath, fpo->FpPath) != 0 ||
	    (fp->FpFlags & FP_props) != (fpo->FpFlags & FP_props) ||
	    ((fp->FpFlags & FP_name) && strcmp(fp->FpName, fpo->FpName) != 0) ||
	    ((fp->FpFlags & FP_uid) && fp->FpUid != fpo->FpUid) ||
	    ((fp->FpFlags & FP_gid) && fp->FpGid != fpo->FpGid) ||
	    ((fp->FpFlags & FP_minsize) && fp->FpMinsize != fpo->FpMinsize) ||
	    ((fp->FpFlags & FP_maxsize) && fp->FpMaxsize != fpo->FpMaxsize)) {
				/*
				 * Different entries.
				 */
				continue;
			}

			/*
			 * Found entry in new FileProps.
			 * Compare attributes.
			 */
			removed = FALSE;
			fp->FpFlags &= ~(FP_add | FP_change);
			if (fp->FpStatus != fpo->FpStatus ||
			    fp->FpMask != fpo->FpMask) {
				/*
				 * Some attribute changed.
				 * A new scan is needed.
				 */
				fp->FpFlags |= FP_change;
			}

			/*
			 * Check copies.
			 */
			for (copy = 0; copy < MAX_ARCHIVE; copy++) {
				int	copyBit;

				copyBit = 1 << copy;
				if (fp->FpCopiesReq != fpo->FpCopiesReq) {
					/*
					 * Some copy flags are changed.
					 * A new scan is needed.
					 */
					fp->FpFlags |= FP_change;
				}

				/*
				 * Check archiving requirements.
				 */
				if (fp->FpCopiesReq & copyBit) {
					if (!(fpo->FpCopiesReq & copyBit)) {
						Trace(TR_MISC,
						    "%s copy %d added",
						    asname, copy + 1);
						fp->FpFlags |= FP_change;
					} else if (fp->FpArchAge[copy] !=
					    fpo->FpArchAge[copy]) {
						Trace(TR_MISC,
						    "%s copy %d archive age"
						    " changed",
						    asname, copy + 1);
						fp->FpFlags |= FP_change;
					}
				} else if (fpo->FpCopiesReq & copyBit) {
					Trace(TR_MISC,
					    "%s copy %d removed",
					    asname, copy + 1);
					fp->FpFlags |= FP_change;
				}

				/*
				 * Check unarchiving requirements.
				 */
				if (fp->FpCopiesUnarch & copyBit) {
					if (!(fpo->FpCopiesUnarch & copyBit)) {
						Trace(TR_MISC,
						    "%s copy %d unarchive"
						    " added",
						    asname, copy + 1);
						fp->FpFlags |= FP_change;
					} else if (fp->FpUnarchAge[copy] !=
					    fpo->FpUnarchAge[copy]) {
						Trace(TR_MISC,
						    "%s copy %d unarchive age"
						    " changed",
						    asname, copy + 1);
						fp->FpFlags |= FP_change;
					}
				} else if (fpo->FpCopiesUnarch & copyBit) {
					Trace(TR_MISC,
					    "%s copy %d unarchive removed",
					    asname, copy + 1);
					fp->FpFlags |= FP_change;
				}
			}
		}

		if (removed) {
			Trace(TR_MISC, "%s removed", asname);
		}
	}

	/*
	 * Set up scan for any new or changed file properties.
	 */
	for (fpNum = 0; fpNum < FileProps->FpCount; fpNum++) {
		fp = &FileProps->FpEntry[fpNum];
		if (fp->FpFlags & FP_add) {
			Trace(TR_MISC, "%s added",
			    ArchSetTable[fp->FpBaseAsn].AsName);
		}
		if (fp->FpFlags & FP_change) {
			Trace(TR_MISC, "%s changed",
			    ArchSetTable[fp->FpBaseAsn].AsName);
		}
		if (fp->FpFlags & (FP_add | FP_change)) {
			int	copy;

			fp->FpFlags &= ~(FP_add | FP_change);

			/*
			 * Find first copy required.
			 */
			for (copy = 0; copy < MAX_ARCHIVE; copy++) {
				if (fp->FpAsn != 0) {
					if (State->AfExamine >= EM_noscan) {
						struct ScanListEntry se;

						/*
						 * Add a scan entry.
						 */
						memset(&se, 0, sizeof (se));
						if (PathToId(fp->FpPath,
						    &se.SeId) >= 0) {
							se.SeAsn =
							    fp->FpBaseAsn;
							se.SeTime = time(NULL);
							ScanfsAddEntry(&se);
						}
					}
					fp->FpFlags &= ~(FP_add | FP_change);
				}
			}
		}
	}
	if (deletes != 0) {
		/*
		 * Notify the archiver daemon to cause the scheduler to run.
		 * The scheduler will notice the deleted ArchReqs.
		 */
		(void) ArchiverCatalogChange();
	}

out:
	if (oldArchSetFile != NULL) {
		(void) ArMapFileDetach(oldArchSetFile);
	}
	if (oldFileProps != NULL) {
		(void) ArMapFileDetach(oldFileProps);
	}
}


/*
 * Reconfigure.
 */
static void
reconfig(
	/*ARGSUSED0*/
	ReconfigControl_t ctrl)
{
	ChangeState(State->AfExec);
#if !defined(TEST_W_DBX)
	TraceReconfig();
#endif /* !defined(TEST_W_DBX) */
	attachFileProps();
	ArchiveReconfig();
	ScanfsReconfig();
	if (ArchSetMap != NULL) {
		SamFree(ArchSetMap);
		ArchSetMap = NULL;
	}
}


/*
 * SIGALRM received.
 */
static void
/*ARGSUSED0*/
sigAlarm(
	int sig)
{
	if (getppid() == 1) {
		static boolean_t first = TRUE;

		if (first) {
			first = FALSE;
			/* Parent process died */
			PostOprMsg(4046);
			SendCustMsg(HERE, 4046);
		}
		ChangeState(ES_term);
		ThreadsWakeup();
	}
	if (Exec == ES_fs_wait) {
		/* Waiting for :arrun fs.%s */
		PostOprMsg(4353, FsName);
	} else if (Exec == ES_wait) {
		/* Waiting for :arrun %s */
		PostOprMsg(4352, "");
	}
	alarm(AdState->AdInterval);
}


/*
 * Fatal signal received.
 */
static void
/*ARGSUSED0*/
sigFatal(
	int sig)
{
	ScanfsTrace();
#if defined(DEBUG)
	TraceRefs();
	ArchiveTrace();
#endif /* defined(DEBUG) */
}


/*
 * SIGHUP received.
 */
static void
/*ARGSUSED0*/
sigHup(
	int sig)
{
	ThreadsReconfigSync(RC_reconfig);
}


/*
 * Termination signal.
 */
static void
/*ARGSUSED0*/
sigTerm(
	int sig)
{
	ChangeState(ES_term);
	stopSignal = sig;
	ThreadsWakeup();
	State->AfNormalExit = TRUE;
}
