/*
 * archiverd.c - Archiver daemon program.
 *
 * archiverd is the parent program of the SUN SAM-FS file archiving facility.
 * archiverd is started by sam-fsd.
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

#pragma ident "$Revision: 1.142 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

/* SAM-FS headers. */
#if defined(AR_DEBUG)
#define	TRACE_CONTROL
#endif /* defined(AR_DEBUG) */

#define        DEC_INIT
#include "sam/types.h"
#include "sam/defaults.h"
#include "sam/exit.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/param.h"
#include "sam/signals.h"
#include "sam/syscall.h"

/* Local headers. */
#define	DEC_INIT
#include "archiverd.h"
#include "device.h"
#include "volume.h"
#undef	DEC_INIT

#if defined(lint)
#undef kill
#undef sigemptyset
#undef sigfillset
#undef sigaddset
#undef sigdelset
#undef sigprocmask
#endif /* defined(lint) */

/* Private data. */

/*
 * Child process table.
 * Record of active child processes.
 */
static struct ChildProc {
	int	count;
	struct ChildProcEntry {
		uname_t CpName;				/* process name */
		char	CpArgv1[ARCHREQ_FNAME_SIZE];	/* First argument */
		void	(*CpFunc)(char *argv1, int stat);
		pid_t	CpPid;
		int	CpNoSIGCHLD;
	} entry[1];
} *childProcTable = NULL;

static pthread_mutex_t childProcMutex = PTHREAD_MUTEX_INITIALIZER;

/* Signals control. */
static void processSigChld(int sig);
static void sigAlarm(int sig);
static void sigFatal(int sig);
static void sigHup(int sig);
static void sigTerm(int sig);

static struct SignalFunc signalFunc[] = {
	{ SIGALRM, sigAlarm },
	{ SIGCHLD, processSigChld },
	{ SIGHUP, sigHup },
	{ SIGINT, sigTerm },
	{ SIGPIPE, NULL },
	{ SIGTERM, sigTerm },
	{ 0 }
};

static struct SignalsArg signalsArg = {
	signalFunc, sigFatal, SAM_ARCHIVERD_HOME
};

/*
 * Threads control.
 * Table of threads to start and stop.
 */
static struct Threads threads[] = {
	{ "Signals" },
	{ "Compose", Compose },
	{ "Message", Message },
	{ "Schedule" },
	{ NULL }
};

static boolean_t cmdFileValid = FALSE;	/* archiver.cmd file is valid */
static boolean_t cmdFirst = TRUE;	/* First time reading command file */
static char cmdFileName[] = ARCHIVER_CMD; /* Archiver commands file name */
static int stopSignal = 0;	/* What termination signal was received */

/* Private functions. */
static boolean_t activeProcesses(void);
static void archiverdInit(void);
static void arfindComplete(char *argv1, int stat);
static void atExit(void);
static void attachStateFile(void);
static void checkFs(void);
static void checkStateFile(void);
static void makeArchDir(void);
static void makeArchReqDir(char *fsname);
static void makeArchiveFiles(char *mntPoint);
static void makeFileSys(void);
static void makeStateFile(void);
static void processSendSignal(int sig, char *pname, char *argv1);
static void readCmdFile(void);
static void readCmdFileComplete(char *argv1, int stat);
static void reconfig(ReconfigControl_t ctrl);
static void resetArfindState(struct FileSysEntry *fs);
static void rmFsFiles(struct FileSysEntry *fs, boolean_t state);
static void rmWorkInProgress(void);


int
/*ARGSUSED0*/
main(
	int argc,
	char *argv[])
{
	int	i;

	/*
	 * Check initiator.
	 */
	if (strcmp(GetParentName(), SAM_FSD) != 0) {
#if !defined(TEST_W_DBX)
		/* sam-archiverd may be started only by sam-fsd */
		fprintf(stderr, "%s\n", GetCustMsg(4045));
		exit(EXIT_FAILURE);
	}
#else /* !defined(TEST_W_DBX) */
		Daemon = FALSE;
		if (argc > 1) {
			SchedulerTest();
		}
	}
	while (Daemon) {
		sleep(10000);
	}
#endif /* !defined(TEST_W_DBX) */

	/*
	 * Perform initialization.
	 */
	archiverdInit();
	ThreadsReconfigSync(RC_wait);
	threads[0].TtThread = Signals(&signalsArg);
	atexit(atExit);
	ThreadsStart(threads, reconfig);

	/*
	 * Read archiver command file, and wait for completion.
	 */
	alarm(1);	/* Start the alarm sequence */
	checkFs();
	readCmdFile();
	ThreadsReconfigSync(RC_allow);
	Schedule();

	/*
	 * Stop archiving on all mounted filesystems.
	 */
	for (i = 0; i < FileSysTable->count; i++) {
		struct FileSysEntry *fs;

		fs = &FileSysTable->entry[i];
		FileSysUmount(fs);
	}
	processSendSignal(SIGTERM, AF_PROG, NULL);
	processSendSignal(stopSignal, AC_PROG, NULL);

	/*
	 * Wait for children to exit.
	 */
	/* Waiting for archiving to complete */
	PostOprMsg(4016);
	while (activeProcesses()) {
		ThreadsSleep(1);
	}
	ThreadsStop();
	if (AdState->AdExec == ES_term) {
		ClearOprMsg();
		if (stopSignal != 0) {
			/* Stopped by signal */
			SendCustMsg(HERE, 4003, StrSignal(stopSignal));
		}
		return (EXIT_SUCCESS);
	}

	/*
	 * Restart.
	 * Just exec() ourselves.
	 */
#if defined(AR_DEBUG)
	/*
	 * Remove the trace file.
	 */
	{
		struct TraceCtlBin *tb;
		struct TraceCtlEntry *te;

		tb = ArMapFileAttach(TRACECTL_BIN, TRACECTL_MAGIC, O_RDWR);
		if (tb != NULL) {
			te = &tb->entry[TI_archiver];
			(void) unlink(te->TrFname);
		} else {
			Trace(TR_ARDEBUG, "Delete %s file failed",
			    traceIdNames[TI_archiver]);
		}
	}
#endif /* defined(AR_DEBUG) */

	if (AdState->AdExec == ES_restart) {
		rmWorkInProgress();
	}
	/* Restarting. */
	PostOprMsg(4007);
	SendCustMsg(HERE, 4007);
	ThreadsSleep(2);	/* Let them see the message. */
	AdState->AdPid = 0;
	for (i = STDERR_FILENO + 1; i < OPEN_MAX; i++) {
		(void) fcntl(i, F_SETFD, FD_CLOEXEC);
	}
	snprintf(ScrPath, sizeof (ScrPath), "%s/%s", SAM_EXECUTE_PATH, argv[0]);
	execv(ScrPath, argv);

	/*
	 * execv() failed.
	 */
	ErrorExitStatus = EXIT_NORESTART;
	LibFatal(exec, ScrPath);
	/*NOTREACHED*/
}


/*
 * Change execution state.
 * Avoid changing state while stopping.
 */
boolean_t				/* TRUE if state changed */
ChangeState(
	ExecState_t newState)
{
	static pthread_mutex_t execMutex = PTHREAD_MUTEX_INITIALIZER;
	boolean_t	status;

	PthreadMutexLock(&execMutex);
	if (AdState->AdExec < ES_term) {
		status = TRUE;
		if (AdState->AdExec != newState) {
			Trace(TR_MISC, "State changed from %s to %s",
			    ExecStateToStr(AdState->AdExec),
			    ExecStateToStr(newState));
			AdState->AdExec = newState;
		}
	} else {
		status = FALSE;
	}
	PthreadMutexUnlock(&execMutex);
	ThreadsWakeup();
	ClearOprMsg();
	return (status);
}


/*
 * Trace child process table.
 */
void
ChildTrace(void)
{
	FILE	*st;
	int	i;

	Trace(TR_MISC, "Child process status:");
	if ((st = TraceOpen()) == NULL) {
		return;
	}
	PthreadMutexLock(&childProcMutex);
	for (i = 0; i < childProcTable->count; i++) {
		struct ChildProcEntry *cp;

		cp = &childProcTable->entry[i];
		if (*cp->CpName != '\0') {
			fprintf(st, "%3d %-10s %-10s %d %d\n",
			    i, cp->CpName, cp->CpArgv1,
			    (int)cp->CpPid, cp->CpNoSIGCHLD);
		}
	}
	PthreadMutexUnlock(&childProcMutex);
	TraceClose(-1);
}


/*
 * File system mounted.
 */
void
FileSysMount(
	struct FileSysEntry *fs)
{
	FileSysStatus(fs);
	if (fs->FsFlags & FS_mounted) {
		fs->FsFlags &= ~FS_umount;
		makeArchiveFiles(fs->FsMntPoint);
		makeArchReqDir(fs->FsName);
		if (fs->FsAfState->AfPid != 0) {
			/*
			 * Notify the arfind.
			 */
			(void) kill(fs->FsAfState->AfPid, SIGALRM);
		}
		if (fs->FsAfState->AfExec == ES_run) {
			ScheduleSetFsState(fs, EC_run);
			ScheduleRun("FileSysMount");
		}
	}
}


/*
 * Update file system status.
 */
void
FileSysStatus(
	struct FileSysEntry *fs)
{
	static pthread_mutex_t fsStatMutex = PTHREAD_MUTEX_INITIALIZER;
	struct sam_fs_info fi;

	PthreadMutexLock(&fsStatMutex);
	if (GetFsInfo(fs->FsName, &fi) == -1) {
		Trace(TR_ERR, "GetFsInfo(%s) failed", fs->FsName);
		goto out;
	}
	fs->FsFlags &= FS_umount;
	if ((fi.fi_status & FS_MOUNTED) &&
	    !(fi.fi_status & FS_UMOUNT_IN_PROGRESS)) {
		fs->FsFlags |= FS_mounted;
		strncpy(fs->FsMntPoint, fi.fi_mnt_point,
		    sizeof (fs->FsMntPoint)-1);
	} else {
		*fs->FsMntPoint = '\0';
	}

	if (!(fi.fi_config & MT_SAM_ENABLED)) {
		fs->FsFlags |= FS_noarchive;
	}
	if (!(fi.fi_config & MT_ARCHIVE_SCAN)) {
		fs->FsFlags |= FS_noarfind;
	}

	if (fi.fi_config & MT_SHARED_READER) {
		fs->FsFlags |= FS_share_reader;
	}
	if (fi.fi_status & FS_CLIENT) {
		fs->FsFlags |= FS_share_client;
	}
	if (fs->FsFlags &
	    (FS_share_client | FS_share_reader | FS_noarchive | FS_noarfind)) {
		/*
		 * Not archiving this file system.
		 * Remove all but state file.
		 */
		rmFsFiles(fs, FALSE);
	}

out:
	PthreadMutexUnlock(&fsStatMutex);
}


/*
 * File system unmount.
 */
void
FileSysUmount(
	struct FileSysEntry *fs)
{
	FileSysStatus(fs);
	fs->FsFlags |= FS_umount;
	ScheduleSetFsState(fs, EC_stop);
	ScheduleRun("FileSysUmount");
}


/*
 * Start a child process.
 * The process is entered into the child table.
 * RETURN: -1 if error.
 */
int
StartProcess(
	char *argv_a[],
	void (*func)(char *argv1, int stat))
{
	upath_t args[MAX_START_PROCESS_ARGS + 1];
	upath_t path;
	sigset_t set, oset;
	struct ChildProcEntry *cpNew;
	char	*argv[MAX_START_PROCESS_ARGS + 1];
	int	i;

	for (i = 0; i < MAX_START_PROCESS_ARGS; i++) {
		argv[i] = args[i];
		if (argv_a[i] != NULL) {
			strncpy(argv[i], argv_a[i], sizeof (upath_t)-1);
		} else {
			argv[i] = NULL;
			break;
		}
	}
	argv[i] = NULL;

	/*
	 * Find empty entry.
	 * Avoid starting process if already running.
	 */
	cpNew = NULL;
	PthreadMutexLock(&childProcMutex);
	for (i = 0; i < childProcTable->count; i++) {
		struct ChildProcEntry *cp;

		cp = &childProcTable->entry[i];
		if (*cp->CpName == '\0') {
			cpNew = cp;
		} else if (strcmp(cp->CpName, argv[0]) == 0 &&
		    strcmp(cp->CpArgv1, argv[1]) == 0) {
			/*
			 * Already have one of these.
			 */
			PthreadMutexUnlock(&childProcMutex);
			Trace(TR_ARDEBUG, "%s(%s) already running",
			    argv[0], argv[1]);
			return (-1);
		}
	}
	if (cpNew == NULL) {	/* Increase childProc->entry */
		/*
		 * Small trick here - childProcTable is always
		 * 'one entry ahead'.
		 */
		SamRealloc(childProcTable, sizeof (struct ChildProc) +
		    childProcTable->count * sizeof (struct ChildProcEntry));
		cpNew = &childProcTable->entry[childProcTable->count];
		childProcTable->count++;
	}

	/*
	 * Make table entry.
	 */
	memset(cpNew, 0, sizeof (struct ChildProcEntry));
	strncpy(cpNew->CpName, argv[0], sizeof (cpNew->CpName));
	strncpy(cpNew->CpArgv1, argv[1], sizeof (cpNew->CpArgv1));
	cpNew->CpFunc = func;
	cpNew->CpNoSIGCHLD = 0;

	/*
	 * Set non-standard files to close on exec.
	 */
	for (i = STDERR_FILENO + 1; i < OPEN_MAX; i++) {
		(void) fcntl(i, F_SETFD, FD_CLOEXEC);
	}
	snprintf(path, sizeof (path), "%s/%s", SAM_EXECUTE_PATH, cpNew->CpName);

	/*
	 * Start process.
	 * Block all signals for new process.
	 */
	if (sigfillset(&set) == -1) {
		LibFatal(sigfillset, "");
	}
	if (sigprocmask(SIG_SETMASK, &set, &oset) == -1) {
		LibFatal(sigprocmask, "SIG_SETMASK");
	}
	if ((cpNew->CpPid = fork1()) < 0) {
		cpNew->CpPid = 0;
		*cpNew->CpName = '\0';
		PthreadMutexUnlock(&childProcMutex);
		Trace(TR_ERR, "fork1(%s) failed", argv[0]);
		return (-1);
	}

	if (cpNew->CpPid == 0) {
		/* Child. */
		execv(path, argv);

		/*
		 * execv() failed.
		 */
		TracePid = getpid();
		program_name = argv[0];
		ErrorExitStatus = EXIT_NORESTART;
		LibFatal(exec, path);
		/*NOTREACHED*/
	}

	/* Parent. */
	if (sigprocmask(SIG_SETMASK, &oset, NULL) == -1) {
		LibFatal(sigprocmask, "SIG_SETMASK");
	}
	Trace(TR_PROC, "Started %s[%d] %s", cpNew->CpName, (int)cpNew->CpPid,
	    cpNew->CpArgv1);
	PthreadMutexUnlock(&childProcMutex);
	return (0);
}



/* Private functions. */



/*
 * Count active processes.
 */
boolean_t				/* TRUE processes active */
activeProcesses(void)
{
	int	active;
	int	i;

	active = 0;
	PthreadMutexLock(&childProcMutex);
	for (i = 0; i < childProcTable->count; i++) {
		if (childProcTable->entry[i].CpPid != 0) {
			active++;
		}
	}
	PthreadMutexUnlock(&childProcMutex);
	return (active != 0);
}


/*
 * Perform completion tasks.
 * "Release" state file.
 */
static void
atExit(void)
{
	if (AdState != NULL) {
		AdState->AdPid = 0;
		/* Not active */
		PostOprMsg(4337);
	}
}


/*
 * Initialize program.
 */
static void
archiverdInit(void)
{
	sam_defaults_t *df;
	sigset_t set;

	program_name = SAM_ARCHIVER;
	CustmsgInit(Daemon, NotifyRequest);

	/*
	 * Block all signals.
	 */
	if (sigfillset(&set) == -1) {
		LibFatal(sigfillset, "");
	}
	if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
		LibFatal(sigprocmask, "block signals");
	}

	/*
	 * Set up tracing.
	 */
	if (Daemon) {
		/*
		 * Provide file descriptors for the standard files.
		 */
		(void) close(STDIN_FILENO);
		if (open("/dev/null", O_RDONLY) != STDIN_FILENO) {
			LibFatal(open, "stdin /dev/null");
		}
		(void) close(STDOUT_FILENO);
		if (open("/dev/null", O_RDWR) != STDOUT_FILENO) {
			LibFatal(open, "stdout /dev/null");
		}
		(void) close(STDERR_FILENO);
		if (open("/dev/null", O_RDWR) != STDERR_FILENO) {
			LibFatal(open, "stderr /dev/null");
		}

		TraceInit(program_name, TI_archiver | TR_MPLOCK | TR_MPMASTER);
	} else {
		/*
		 * Allow trace messages to go to stdout/stderr.
		 */
		TraceInit(NULL, TI_none);
		*TraceFlags = (1 << TR_module) | ((1 << TR_date) - 1);
		*TraceFlags &= ~(1 << TR_alloc);
		*TraceFlags &= ~(1 << TR_dbfile);
		*TraceFlags &= ~(1 << TR_files);
		*TraceFlags &= ~(1 << TR_oprmsg);
		*TraceFlags &= ~(1 << TR_sig);
	}
#if defined(AR_DEBUG)
	*TraceFlags |= 1 << TR_ardebug;
#endif /* defined(AR_DEBUG) */

	/*
	 * All archiver files are relative to ARCHIVER_DIR.
	 * Also, archiverd's and archiver's core files will be found here.
	 */
	if (chdir(ARCHIVER_DIR) == -1) {
		LibFatal(chdir, ARCHIVER_DIR);
	}

	/*
	 * Set group id for all created directories and files.
	 */
	df = GetDefaults();
	if (setgid(df->operator.gid) == -1) {
		Trace(TR_ERR, "setgid(%ld)", df->operator.gid);
	}
	(void) umask(0027);	/* Allow only root writes */

	attachStateFile();

	/*
	 * Make the basic tables.
	 */
	makeFileSys();
	DeviceConfig(AP_archiverd);
	VolumeConfig();
	if (AdState->AdExec == ES_none) {
		makeStateFile();
	}
	makeArchDir();
	AdState->AdPid = getpid();
	OpenOprMsg(AdState->AdOprMsg);
	/* Initializing */
	PostOprMsg(4327);

	/*
	 * Allocate process table and child status queue.
	 */
	SamMalloc(childProcTable, sizeof (struct ChildProc));
	memset(childProcTable, 0, sizeof (struct ChildProc));
	childProcTable->count = 1;

	/*
	 * Discard an unusable archive set file.
	 */
	ArchSetFile = ArMapFileAttach(ARCHIVE_SETS, ARCHSETS_MAGIC, O_RDWR);
	if (ArchSetFile == NULL || ArchSetFile->AsVersion != ARCHSETS_VERSION) {
		Trace(TR_MISC, "Unusable archive set file: %s",
		    StrFromErrno(errno, NULL, 0));
		(void) unlink(ARCHIVE_SETS);
		if (ArchSetFile != NULL) {
			ArchSetFile->As.MfValid = 0;
		}
	}
	if (ArchSetFile != NULL) {
		(void) ArMapFileDetach(ArchSetFile);
	}
	ArchSetFile = NULL;
}


/*
 * Arfind completed.
 */
static void
arfindComplete(
	char *argv1,
	int stat)
{
	int	i;

	/*
	 * Find arfind.
	 */
	for (i = 0; i < FileSysTable->count; i++) {
		struct FileSysEntry *fs;

		fs = &FileSysTable->entry[i];
		if (strcmp(fs->FsName, argv1) == 0) {
			if (((stat >> 8) & 0xff) == EXIT_NORESTART) {
				fs->FsFlags |= FS_norestart;
			}
			*fs->FsAfState->AfOprMsg = '\0';
			fs->FsAfState->AfPid = 0;
			break;
		}
	}
	checkFs();
}


/*
 * Attach the archiver state file.
 * Make it if missing or obsolete.
 */
static void
attachStateFile(void)
{
	ExecState_t prevExec;
	struct stat buf;
	char	*msg;

	if ((AdState = ArMapFileAttach(ARCHIVER_STATE, AD_MAGIC, O_RDWR)) !=
	    NULL) {
		if (AdState->AdVersion == AD_VERSION) {
			if (Daemon && AdState->AdPid != 0) {
				/*
				 * Maybe another archiverd is running.
				 * Wake it up and then test it with
				 * kill(pid, 0).
				 */
				(void) kill(AdState->AdPid, SIGALRM);
				sleep(2);
				errno = 0;
				if (kill(AdState->AdPid, 0) == 0 ||
				    (errno != 0 && errno != ESRCH &&
				    errno != ECONNREFUSED)) {
					/* Another sam-archiverd is running */
					SendCustMsg(HERE, 4008);
					exit(EXIT_FAILURE);
				}
			}
			prevExec = AdState->AdExec;
			AdState->AdExec = ES_init;
		} else {
			/*
			 * Discard invalid state file.
			 */
			Trace(TR_MISC, "Invalid state file");
			(void) unlink(ARCHIVER_STATE);
			AdState->Ad.MfValid = 0;
			if (ArMapFileDetach(AdState) == -1) {
				LibFatal(ArMapFileDetach, ARCHIVER_STATE);
			}
			AdState = NULL;
			prevExec = ES_none;
		}
	}

	switch (prevExec) {

	case ES_rerun:
		msg = "soft restart";
		break;

	case ES_restart:
		msg = "restart";
		if (AdState != NULL) {
			(void) unlink(ARCHIVER_STATE);
			AdState->Ad.MfValid = 0;
			if (ArMapFileDetach(AdState) == -1) {
				LibFatal(ArMapFileDetach, ARCHIVER_STATE);
			}
			AdState = NULL;
		}
		(void) unlink(ARCHIVE_SETS);
		break;

	case ES_none:
	case ES_term:
		msg = "started";
		break;

	default:
		msg = "started after possible crash";
		break;
	}
	if (AdState == NULL) {
		/* Empty archiver "state file" */
		static struct ArchiverdState adState;

		AdState = &adState;
		memset(AdState, 0, sizeof (struct ArchiverdState));
		strncpy(AdState->AdNotifyFile, SAM_SCRIPT_PATH"/"NOTIFY,
		    sizeof (AdState->AdNotifyFile)-1);
		prevExec = ES_none;
	} else {
		memset(AdState->AdArchReq, 0,
		    AdState->AdCount * sizeof (AdState->AdArchReq));
	}
	Trace(TR_MISC, "Archiver daemon %s", msg);

	/*
	 * Check archiver.cmd file.
	 */
	if (stat(cmdFileName, &buf) != 0) {
		/* File doesn't exist */
		cmdFileValid = FALSE;
	} else if (buf.st_mtime != AdState->AdCmdfile) {
		Trace(TR_MISC,
		    "archiver.cmd file changed since last execution");
		cmdFileValid = FALSE;
	}
}


/*
 * Check filesystems.
 */
void
checkFs(void)
{
	int	i;

	if (AdState->AdExec >= ES_term) {
		return;
	}
	ThreadsReconfigSync(RC_wait);
	for (i = 0; i < FileSysTable->count; i++) {
		struct FileSysEntry *fs;

		fs = &FileSysTable->entry[i];
		if (fs->FsAfState->AfPid != 0) {
			/*
			 * Arfind running.
			 */
			continue;
		}

		/*
		 * Arfind not running.
		 * Post messages for:
		 * file system is not mounted,
		 * the file system is mounted "no archive(nosam)",
		 * "shared client", * "no arfind(noarscan) or "shared reader",
		 * or arfind restart has been disabled.
		 */
		FileSysStatus(fs);
		if (fs->FsFlags & FS_noarchive) {
			/* fs defined "archive = off".  Cannot archive. */
			snprintf(fs->FsAfState->AfOprMsg,
			    sizeof (fs->FsAfState->AfOprMsg),
			    GetCustMsg(4338));

		} else if (fs->FsFlags & FS_noarfind) {
			/* noarscan enabled */
			snprintf(fs->FsAfState->AfOprMsg,
			    sizeof (fs->FsAfState->AfOprMsg),
			    GetCustMsg(4362));

		} else if (fs->FsFlags & FS_share_client) {
			/* Shared file system client.  Cannot archive. */
			snprintf(fs->FsAfState->AfOprMsg,
			    sizeof (fs->FsAfState->AfOprMsg),
			    GetCustMsg(4351));

		} else if (fs->FsFlags & FS_share_reader) {
			/* Shared reader file system.  Cannot archive. */
			snprintf(fs->FsAfState->AfOprMsg,
			    sizeof (fs->FsAfState->AfOprMsg),
			    GetCustMsg(4350));

		} else if (fs->FsFlags & FS_norestart) {
			/* Archiving disabled - see sam log */
			snprintf(fs->FsAfState->AfOprMsg,
			    sizeof (fs->FsAfState->AfOprMsg),
			    GetCustMsg(4339));

		} else if (!(fs->FsFlags & FS_mounted)) {
			/* Waiting for mount */
			snprintf(fs->FsAfState->AfOprMsg,
			    sizeof (fs->FsAfState->AfOprMsg),
			    GetCustMsg(4300), "mount");

		} else {
			static char *argv[] = { AF_PROG, "", NULL };
			struct stat sb;

			/*
			 * arfind not running.
			 * Start arfind.
			 */
			if (!(fs->FsFlags & FS_mounted)) {
				Trace(TR_QUEUE,
				    "Tried to start arfind %s", fs->FsName);
				continue;
			}
			makeArchiveFiles(fs->FsMntPoint);

			*fs->FsAfState->AfOprMsg = '\0';
			fs->FsAfState->AfExec = AdState->AdExec;
			argv[1] = fs->FsName;
			snprintf(ScrPath, sizeof (ScrPath),
			    "%s/"FILE_PROPS, fs->FsName);
			if (!cmdFileValid) {
				(void) unlink(ScrPath);
			}
			(void) StartProcess(argv, arfindComplete);

			/*
			 * Assure that the file properties file exists.
			 */
			if (cmdFileValid && stat(ScrPath, &sb) != 0) {
				Trace(TR_DEBUG, "%s missing", ScrPath);
				readCmdFile();
			}
		}
	}
	ThreadsReconfigSync(RC_allow);
}


/*
 * Check state file.
 */
static void
checkStateFile(void)
{
	upath_t ae;
	int	fd;
	int	i;

	if (AdState->AdCount >= ArchLibTable->AlDriveCount) {
		return;
	}

	/*
	 * Increase size of state file to hold additional ArchReq path entries.
	 * This is only used by samu.
	 */
	fd = open(ARCHIVER_STATE, O_WRONLY | O_APPEND);
	if (fd < 0) {
		LibFatal(open-CREAT, ARCHIVER_STATE);
	}
	memset(ae, 0, sizeof (ae));
	for (i = AdState->AdCount; i < ArchLibTable->AlDriveCount; i++) {
		if (write(fd, ae, sizeof (ae)) != sizeof (ae)) {
			LibFatal(write, ARCHIVER_STATE);
		}
		AdState->Ad.MfLen += sizeof (ae);
	}
	AdState->AdCount = ArchLibTable->AlDriveCount;
	if (close(fd) == -1) {
		LibFatal(close, ARCHIVER_STATE);
	}
	(void) ArMapFileDetach(AdState);
	if ((AdState = ArMapFileAttach(ARCHIVER_STATE, AD_MAGIC, O_RDWR)) ==
	    NULL) {
		LibFatal(ArMapFileAttach, ARCHIVER_STATE);
	}
	makeArchDir();
	OpenOprMsg(AdState->AdOprMsg);
	Trace(TR_ARDEBUG, "Increased state file %d drives", AdState->AdCount);
}


/*
 * Make the archive directories.
 */
static void
makeArchDir(void)
{
	int	i;

	/*
	 * Make directories for arcopy.
	 */
	for (i = 0; i < ArchLibTable->AlDriveCount; i++) {
		snprintf(ScrPath, sizeof (ScrPath), "arcopy%d", i);
		MakeDir(ScrPath);
	}
}


/*
 * If doesn't exist, make the archive request directory.
 */
static void
makeArchReqDir(
	char *fsname)
{
	struct stat sb;
	int retries;

	snprintf(ScrPath, sizeof (ScrPath), "%s/%s/%s", ARCHIVER_DIR,
	    fsname, ARCHREQ_DIR);

	for (retries = 0; /* Terminated inside */; retries++) {
		if (lstat(ScrPath, &sb) != 0) {
			/*
			 * Directory doesn't exist.
			 */
			if (retries >= 1) {
				return;
			}
			if (mkdir(ScrPath, 0700) != 0) {
				Trace(TR_ERR, "mkdir(%s)", ScrPath);
				return;
			}
			Trace(TR_MISC, "mkdir(%s)", ScrPath);
			break;
		} else if (!S_ISDIR(sb.st_mode)) {
			/*
			 * Path exists but not a directory.
			 */
			(void) unlink(ScrPath);
		} else {
			/*
			 * Directory exists.
			 */
			break;
		}
	}
}


/*
 * Make archive files.
 * The .archive directory is assured to exist in the root of the file system.
 * A removable media file is made for each possible archive removable media
 * drive.
 */
static void
makeArchiveFiles(
	char *mntPoint)
{
	struct stat sb;
	int	retries;
	int	i;

	/*
	 * Assure that the directory exists.
	 * Delete files by the target directory name.
	 */
	snprintf(ScrPath, sizeof (ScrPath), "%s/%s", mntPoint, RM_FILES_DIR);
	for (retries = 0; /* Terminated inside */; retries++) {
		if (lstat(ScrPath, &sb) != 0) {
			/*
			 * Directory doesn't exist.
			 */
			if (retries >= 1) {
				return;
			}
			if (mkdir(ScrPath, 0700) != 0) {
				Trace(TR_ERR, "mkdir(%s)", ScrPath);
				return;
			}
			Trace(TR_MISC, "mkdir(%s)", ScrPath);
			break;
		} else if (!S_ISDIR(sb.st_mode)) {
			(void) unlink(ScrPath);
		} else {
			sam_defaults_t *df;

			(void) chmod(ScrPath, 0700);
			df = GetDefaults();
			(void) chown(ScrPath, 0, df->operator.gid);
			break;
		}
	}

	/*
	 * Verify that the removable media files exist.
	 */
	for (i = 0; i < ArchLibTable->AlRmDrives; i++) {
		snprintf(ScrPath, sizeof (ScrPath), "%s/%s/%s%d", mntPoint,
		    RM_FILES_DIR, RM_FILES_NAME, i);
		errno = 0;
		if (lstat(ScrPath, &sb) != 0 || !S_ISREG(sb.st_mode)) {
			int fd;

			/*
			 * stat() error or not a regular file.
			 */
			if (errno == EOVERFLOW) {
				/*
				 * Size of file too big for stat().
				 */
				continue;
			}

			/*
			 * Create the file.
			 */
			(void) unlink(ScrPath);
			if ((fd = open(ScrPath, O_CREAT | O_EXCL, 0600)) < 0) {
				Trace(TR_ERR, "open(%s)", ScrPath);
				continue;
			}
			(void) close(fd);
			Trace(TR_ARDEBUG, "%s created", ScrPath);
		}
	}
}


/*
 * Make file system table.
 */
static void
makeFileSys(void)
{
	struct FileSys *oldFileSysTable;
	struct FileSysEntry *fs;
	struct sam_fs_status *fsarray;
	size_t	size;
	int	fsCount;
	int	i;

	if ((fsCount = GetFsStatus(&fsarray)) == -1) {
		LibFatal(GetFsStatus, "");
	}
	if (fsCount == 0) {
		SendCustMsg(HERE, 4006);
		exit(EXIT_NORESTART);
	}
	oldFileSysTable = FileSysTable;

	/*
	 * Check for filesystem change.
	 */
	if (FileSysTable != NULL && fsCount == FileSysTable->count) {
		for (i = 0; i < FileSysTable->count; i++) {
			fs = &FileSysTable->entry[i];
			if (strcmp(fs->FsName, fsarray[i].fs_name) != 0) {
				break;
			}
		}
		if (i == FileSysTable->count) {
			/*
			 * No change.
			 */
			free(fsarray);
			return;
		}
	}

	/*
	 * No previous file system table or some change in file systems.
	 * Make file system table.
	 */
	size = sizeof (struct FileSys) +
	    ((fsCount - 1) * sizeof (struct FileSysEntry));
	SamMalloc(FileSysTable, size);
	memset(FileSysTable, 0, size);
	FileSysTable->count = fsCount;

	/*
	 * Enter file system names.
	 */
	for (i = 0; i < FileSysTable->count; i++) {
		fs = &FileSysTable->entry[i];
		strncpy(fs->FsName, fsarray[i].fs_name, sizeof (fs->FsName)-1);
		if (oldFileSysTable != NULL) {
			int	j;

			/*
			 * Copy content of old FileSysTable.
			 */
			for (j = 0; j < oldFileSysTable->count; j++) {
				struct FileSysEntry *fso;

				fso = &oldFileSysTable->entry[j];
				if (strcmp(fso->FsName, fs->FsName) == 0) {
					memcpy(fs, fso,
					    sizeof (struct FileSysEntry));
					fso->FsAfState = NULL;
					break;
				}
			}
			if (j == oldFileSysTable->count) {
				/*
				 * Added a file system.
				 */
				resetArfindState(fs);
				Trace(TR_MISC, "Add    fs %s", fs->FsName);
			} else {
				Trace(TR_MISC, "Same   fs %s", fs->FsName);
			}
		} else {
			resetArfindState(fs);
		}
	}
	free(fsarray);
	if (oldFileSysTable != NULL) {
		/*
		 * Any file systems left were unconfigured.
		 */
		for (i = 0; i < oldFileSysTable->count; i++) {
			fs = &oldFileSysTable->entry[i];
			if (fs->FsAfState != NULL) {
				rmFsFiles(fs, TRUE);
				Trace(TR_MISC, "Delete fs %s", fs->FsName);
			}
		}
		SamFree(oldFileSysTable);
		Trace(TR_MISC, "File system reconfigured");
	}
	for (i = 0; i < FileSysTable->count; i++) {
		fs = &FileSysTable->entry[i];
		FileSysStatus(fs);
	}
#if defined(TEST_W_DBX)
	rmWorkInProgress();
#endif /* defined(TEST_W_DBX) */
}


/*
 * Make the archiver daemon state file.
 */
static void
makeStateFile(void)
{
	size_t	size;

	/*
	 * Initialize data.
	 */
	if (ArchLibTable == NULL) {
		LibFatal(create, ARCHIVER_STATE);
        }
	size = sizeof (struct ArchiverdState) +
	    ((ArchLibTable->AlDriveCount - 1) * sizeof (AdState->AdArchReq));
	AdState = MapFileCreate(ARCHIVER_STATE, AD_MAGIC, size);
	if (AdState == NULL) {
		LibFatal(create, ARCHIVER_STATE);
	}
	AdState->AdVersion	= AD_VERSION;
	AdState->AdPid		= getpid();
	AdState->AdExec		= ES_init;
	AdState->AdCount	= ArchLibTable->AlDriveCount;
	AdState->AdLastAlarm = time(NULL);
	strncpy(AdState->AdNotifyFile, SAM_SCRIPT_PATH"/"NOTIFY,
	    sizeof (AdState->AdNotifyFile)-1);
	AdState->Ad.MfValid	= 1;
	Trace(TR_ARDEBUG, "State file created %d drives", AdState->AdCount);
}



/*
 * Send a signal to process.
 */
static void
processSendSignal(
	int sig,
	char *pname,	/* Process name.  If NULL, all processes. */
	char *argv1)	/* Argument 1.  If NULL, all arguments. */
{
	struct ChildProcEntry *cp;
	int	i;

	PthreadMutexLock(&childProcMutex);
	for (i = 0; i < childProcTable->count; i++) {
		cp = &childProcTable->entry[i];
		if (cp->CpPid != 0 && strcmp(cp->CpName, AA_PROG) != 0 &&
		    (pname == NULL || strcmp(pname, cp->CpName) == 0) &&
		    (argv1 == NULL || strcmp(argv1, cp->CpArgv1) == 0)) {
			(void) kill(cp->CpPid, sig);
			Trace(TR_MISC, "Signaled %s[%d](%s): %s", cp->CpName,
			    (int)cp->CpPid, cp->CpArgv1, strsignal(sig));
		}
	}
	PthreadMutexUnlock(&childProcMutex);
}


/*
 * Process a SIGCHLD.
 */
static void
/*ARGSUSED0*/
processSigChld(
	int sig)
{
	pid_t	pid;
	int	stat;

	/*
	 * Process all exited child processes that are waiting.
	 */
	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
		struct ChildProcEntry *cp;
		void	(*func)(char *argv1, int stat);
		upath_t	argv1;
		int	exitStatus;
		int	sigNum;
		int	i;

		ASSERT(pid != 0);
		if (pid == 0) {
			continue;
		}

		/*
		 * Look up child.
		 */
		PthreadMutexLock(&childProcMutex);
		for (i = 0; i < childProcTable->count; i++) {
			cp = &childProcTable->entry[i];
			if (cp->CpPid == pid) {
				break;
			}
		}
		if (i >= childProcTable->count) {
			/*
			 * This is not a known child.
			 */
			PthreadMutexUnlock(&childProcMutex);
			Trace(TR_PROC, "Unknown child - pid %d, stat %04x",
			    (int)pid, stat);
			continue;
		}

		/*
		 * Note if the completion status was not good.
		 */
		exitStatus = (stat >> 8) & 0xf;
		sigNum = stat & 0xff;
		if (exitStatus != 0 ||
		    (sigNum != 0 && (sigNum != SIGTERM && sigNum != SIGINT))) {
			SendCustMsg(HERE, 4027, cp->CpName, pid, stat & 0xffff);
		} else {
			Trace(TR_ARDEBUG, "%s[%d](%s) exited",
			    cp->CpName, (int)cp->CpPid, cp->CpArgv1);
		}

#if defined(NO_SIGCHLD_FOR_ARCOPY)
		if (strcmp(cp->CpName, AC_PROG) == 0) {
			PthreadMutexUnlock(&childProcMutex);
			continue;
		}
#endif /* defined(NO_SIGCHLD_FOR_ARCOPY) */

		func = cp->CpFunc;
		memmove(argv1, cp->CpArgv1, sizeof (argv1));
		cp->CpPid = 0;
		*cp->CpName = '\0';
		PthreadMutexUnlock(&childProcMutex);

		/*
		 * Process completion.
		 */
		func(argv1, stat);
	}
}


/*
 * Read archiver command file.
 */
static void
readCmdFile(void)
{
	static char *argv[] = { AA_PROG, "-l", NULL };
	struct stat buf;

	/*
	 * Record command file time.
	 */
	if (stat(cmdFileName, &buf) != 0) {
		/* File doesn't exist */
		buf.st_mtime = 0;
	}
	if (!cmdFirst && buf.st_mtime != AdState->AdCmdfile) {
		/* archiver command file changed, rereading */
		SendCustMsg(HERE, 4012);
	}
	AdState->AdCmdfile = buf.st_mtime;

	cmdFileValid = FALSE;
	if (!cmdFirst || !Daemon) {
			/*
			 * Tell archiver that it's not the first read.
			 */
			*argv[1] = '\0';
	}
	(void) StartProcess(argv, readCmdFileComplete);
}


/*
 * Command file read.
 * Called when archiver exits.
 */
static void
readCmdFileComplete(
	char *argv1,
	int stat)
{
	int	exitStatus;
	int	i;

	exitStatus = stat >> 8;
	if (Daemon) {
		if ((stat & 0xff) != 0) {
			Trace(TR_MISC, "archiver(%s) failed", argv1);
			return;
		}
		if (exitStatus != EXIT_SUCCESS && exitStatus != EXIT_WAIT) {
			if (!ChangeState(ES_cmd_errors)) {
				return;
			}
			/*
			 * Errors in archiver commands.
			 * No archiving will be done.
			 */
			SendCustMsg(HERE, 4023);
			PostOprMsg(4023);
			return;
		}
	} else {
		if ((stat & 0xff) != 0 ||
		    (exitStatus != EXIT_SUCCESS && exitStatus != EXIT_WAIT)) {
			fprintf(stderr, "archiver(%s) failed\n", argv1);
			exit(EXIT_FAILURE);
		}
		exitStatus = EXIT_SUCCESS;
	}

	/*
	 * No problems reading command file.
	 */
	cmdFileValid = TRUE;
	if (ArchSetAttach(O_RDWR) == NULL) {
		LibFatal(ArMapFileAttach,  ARCHIVER_DIR"/"ARCHIVE_SETS);
	}
	ArchLibTable->AlDriveCount = 0;
	for (i = 0; i < ArchLibTable->count; i++) {
		ArchLibTable->AlDriveCount +=
		    ArchLibTable->entry[i].AlDrivesNumof;
	}
	checkStateFile();

	if (cmdFirst) {
		cmdFirst = FALSE;
		if (exitStatus == EXIT_WAIT) {
			if (!ChangeState(ES_wait)) {
				return;
			}
			/* Waiting for %s */
			PostOprMsg(4300, ":arrun");
		} else {
			if (!ChangeState(ES_run)) {
				return;
			}
			ClearOprMsg();
		}
		ReadReservedVsns();
	} else {
		if (AdState->AdExec == ES_cmd_errors) {
			(void) ChangeState(ES_run);
		}
	}
	if (AdState->AdExec == ES_run) {
		ScheduleRun("readCmdFileComplete");
	}
	(void) kill(AdState->AdPid, SIGALRM);
	processSendSignal(SIGHUP, NULL, NULL);
}


/*
 * Perform reconfiguration.
 */
static void
reconfig(
	ReconfigControl_t ctrl)
{
	if (ctrl == RC_reconfig) {
		TraceReconfig();
		makeFileSys();
	}
	DeviceConfig(AP_archiverd);
	VolumeConfig();
	if (ctrl == RC_reconfig) {
		checkStateFile();
		readCmdFile();
	}
}


/*
 * Reset arfind state.
 * Assure that the directories exist.
 * Make the state file if missing or obsolete.
 */
static void
resetArfindState(
	struct FileSysEntry *fs)
{
	struct ArfindState *afState;
	size_t	size;

	MakeDir(fs->FsName);
	snprintf(ScrPath, sizeof (ScrPath), "%s/"ARCHREQ_DIR, fs->FsName);
	MakeDir(ScrPath);
	snprintf(ScrPath, sizeof (ScrPath), "%s/"ARFIND_STATE, fs->FsName);
	if ((afState = ArMapFileAttach(ScrPath, AF_MAGIC, O_RDWR)) != NULL) {
		if (afState->AfVersion == AF_VERSION) {

			/*
			 * No change to the existing state file.
			 */
			if (afState->AfPid != 0) {
				/*
				 * Stop an arfind.
				 */
				(void) kill(afState->AfPid, SIGTERM);
				ThreadsSleep(1);
			}
			afState->AfPid	= 0;
			/* Not active */
			strncpy(afState->AfOprMsg, GetCustMsg(4337),
			    sizeof (afState->AfOprMsg));
			fs->FsAfState = afState;
			return;
		}

		/*
		 * Invalidate state file.
		 */
		(void) unlink(ScrPath);
		afState->Af.MfValid = 0;
		if (ArMapFileDetach(afState) == -1) {
			LibFatal(ArMapFileDetach, ScrPath);
		}
	}

	/*
	 * Initialize data.
	 */
	size = sizeof (struct ArfindState);
	afState = MapFileCreate(ScrPath, AF_MAGIC, size);
	if (afState == NULL) {
		LibFatal(create, ScrPath);
	}
	afState->AfVersion	= AF_VERSION;
	afState->AfExamine	= EM_noscan;
	afState->AfExec		= ES_init;
	afState->AfBackGndInterval = BACKGROUND_SCAN_INTERVAL;
	afState->AfBackGndTime = BACKGROUND_SCAN_TIME;
	afState->Af.MfValid = 1;
	fs->FsAfState = afState;
	Trace(TR_MISC, "%s created", ScrPath);
}


/*
 * Remove file system work files.
 */
static void
rmFsFiles(
	struct FileSysEntry *fs,
	boolean_t state)
{
	DIR		*dirp;

	/*
	 * Invalidate and remove state file.
	 */
	if (state) {
		/*
		 * Invalidate state file.
		 */
		snprintf(ScrPath, sizeof (ScrPath),
		    "%s/"ARFIND_STATE, fs->FsName);
		(void) unlink(ScrPath);
		if (fs->FsAfState != NULL) {
			fs->FsAfState->Af.MfValid = 0;
			if (ArMapFileDetach(fs->FsAfState) == -1) {
				LibFatal(ArMapFileDetach, ScrPath);
			}
			fs->FsAfState = NULL;
		}
	}

	/*
	 * Remove the file properties, and the scanlist.
	 */
	snprintf(ScrPath, sizeof (ScrPath), "%s/"FILE_PROPS, fs->FsName);
	(void) unlink(ScrPath);
	snprintf(ScrPath, sizeof (ScrPath), "%s/"SCANLIST, fs->FsName);
	(void) unlink(ScrPath);

	/*
	 * Remove ArchReq directory entries.
	 */
	snprintf(ScrPath, sizeof (ScrPath), "%s/"ARCHREQ_DIR, fs->FsName);
	if ((dirp = opendir(ScrPath)) != NULL) {
		struct dirent *dirent;

		while ((dirent = readdir(dirp)) != NULL) {
			if (*dirent->d_name != '.') {
				snprintf(ScrPath, sizeof (ScrPath),
				    "%s/"ARCHREQ_DIR"/%s",
				    fs->FsName, dirent->d_name);
				(void) unlink(ScrPath);
			}
		}
		(void) closedir(dirp);
	}
	snprintf(ScrPath, sizeof (ScrPath), "%s/"ARCHREQ_DIR, fs->FsName);
	(void) rmdir(ScrPath);
}


/*
 * Remove work in progress.
 */
static void
rmWorkInProgress(void)
{
	int	i;

	for (i = 0; i < FileSysTable->count; i++) {
		rmFsFiles(&FileSysTable->entry[i], TRUE);
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
	static time_t lastInterval = 0;
	int	i;

	AdState->AdLastAlarm = time(NULL);
	alarm(ALARM_TIME);
	checkFs();
	if (AdState->AdLastAlarm > lastInterval + AdState->AdInterval) {
		lastInterval = AdState->AdLastAlarm;
		ScheduleRun("interval");
	}

	/*
	 * Check child processes.
	 */
retry:
	PthreadMutexLock(&childProcMutex);
	for (i = 0; i < childProcTable->count; i++) {
		struct ChildProcEntry *cp;
		upath_t	argv1;
		void	(*func)(char *argv1, int stat);

		cp = &childProcTable->entry[i];
		if (*cp->CpName == '\0' || cp->CpPid == 0) {
			continue;
		}
		if (kill(cp->CpPid, 0) != 0) {
			if (errno != ESRCH && errno != ECONNREFUSED) {
				Trace(TR_MISC, "kill(0, %d) failed - %s %s",
				    (int)cp->CpPid, cp->CpName, cp->CpArgv1);
				continue;
			}
		} else {
			uname_t pname;

			/*
			 * Check the process name.
			 * Just after the fork, the name may be "sam-archiverd".
			 */
			(void) GetProcName(cp->CpPid, pname, sizeof (pname));
			if (strcmp(pname, cp->CpName) == 0 ||
			    strcmp(pname, program_name) == 0) {
				continue;
			}
		}
		cp->CpNoSIGCHLD++;
		if (cp->CpNoSIGCHLD <= 1) {
			continue;
		}

		/*
		 * Process does not exist.
		 * Process completion.
		 */
		Trace(TR_MISC, "%s[%d](%s) died (no SIGCHLD)", cp->CpName,
		    (int)cp->CpPid, cp->CpArgv1);
		func = cp->CpFunc;
		memmove(argv1, cp->CpArgv1, sizeof (argv1));
		cp->CpPid = 0;
		*cp->CpName = '\0';
		PthreadMutexUnlock(&childProcMutex);
		func(argv1, 0);
		goto retry;
	}
	PthreadMutexUnlock(&childProcMutex);
}


/*
 * Fatal signal received.
 */
static void
/*ARGSUSED0*/
sigFatal(
	int sig)
{
	ScheduleTrace();
	ComposeTrace();
	QueueTrace(HERE, NULL);
#if defined(DEBUG)
	TraceRefs();
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
	stopSignal = sig;
	(void) ChangeState(ES_term);
}
