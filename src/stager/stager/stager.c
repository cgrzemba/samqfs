/*
 * stager.c - stager daemon's main program
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

#pragma ident "$Revision: 1.72 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <pthread.h>
#include <time.h>
struct tm *localtime_r(const time_t *clock, struct tm *res);

/* Socket headers. */
#include <sys/socket.h>
#include <sys/un.h>

/* Solaris headers. */
#include <sys/shm.h>
#include <sys/syscall.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/shm.h"
#include "sam/nl_samfs.h"
#include "sam/names.h"
#include "aml/samlive.h"
#include "sam/defaults.h"
#include "aml/diskvols.h"
#include "sam/syscall.h"
#include "sam/custmsg.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "sam/exit.h"
#include "sam/lib.h"
#include "aml/stager.h"
#include "aml/remote.h"
#include "sam/signals.h"
#include "aml/stager_defs.h"
#if defined(lint)
#include "sam/lint.h"
#undef kill
#endif /* defined(lint) */

/* Local headers. */
#include "stager_config.h"
#include "stager_lib.h"
#include "stager_threads.h"
#include "stager_shared.h"
#include "copy_defs.h"
#include "rmedia.h"
#include "stream.h"
#include "stage_done.h"
#include "file_defs.h"

#include "stager.h"
#include "stage_reqs.h"
#include "schedule.h"
#include "thread.h"
#include "thirdparty.h"

static upath_t fullpath;	/* temporary space for building names */
static void initStagerd();
static void createShared();
static void createSharedMapFile();
static void removeSharedMapFile();
static void makeStagerDirs();
static int createFilesystemReader();
static void *filesystemReader();
static int createScheduler();
static int createMigrator();
static void createState();
static void removeStateMapFile();
static void setStagerPid(pid_t pid);
static void cleanupAndExit(int error);
static void cancelThreads();
static void readCmdFile();
static int initConfigCheck();
static void *configCheck(void *arg);
static void reconfig();

static void fatalSignalCleanup(int signum);
static void sigChld(int signum);
static void sigHup(int signum);
static void sigTerm(int signum);
static void sigUsr1(int signum);
static void requeueRequests();

/*
 * External stage request list.
 */
extern CopyInstanceList_t *CopyInstanceList;
extern StageReqs_t StageReqs;
extern int OrphanProcs;
extern StageDoneInfo_t *stageDone;

static time_t cmdFileTime;
static char cmdFileName[] = SAM_CONFIG_PATH"/"COMMAND_FILE_NAME;
static boolean_t infiniteLoop = B_TRUE;

static struct SignalFunc signalFunc[] = {
	{ SIGCHLD, sigChld },
	{ SIGHUP,  sigHup },
	{ SIGINT,  sigTerm },
	{ SIGTERM, sigTerm },
	{ SIGUSR1, sigUsr1 },
	{ 0 }
};
static struct SignalsArg signalsArg = {
	signalFunc, fatalSignalCleanup, SAM_STAGERD_HOME
};

/*
 * Filesystem reader thread id.
 */
static pthread_t fsReaderThreadId;

/*
 * Configuration check thread id.
 */
static pthread_t configCheckThreadId;

/*
 * Stager initialization in progress.  Used to prevent reconfiguration
 * before daemon initalization has completed.
 */
static boolean_t initInProgress = B_TRUE;

static pthread_mutex_t reconfigMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t copyInstanceMutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * SAM-FS shared memory segment.
 */
shm_alloc_t 	master_shm, preview_shm;
shm_ptr_tbl_t	*shm_ptr_tbl = NULL;

/*
 * SAM-FS defaults shared memory segment.
 */
sam_defaults_t	*Defaults;

/*
 * Set to TRUE if not started from sam-amld.
 */
boolean_t 		DebugMode = FALSE;

/*
 * Shutdown stager daemon.
 */
int ShutdownStager = 0;

/*
 * Stager start mode.
 */
enum StartMode StartMode = SM_cold;

/*
 * Idle stager daemon.
 */
boolean_t IdleStager = B_TRUE;

/*
 * Message catalog descriptor.
 */
nl_catd catfd = NULL;

/*
 * Program name.
 */
char *program_name = NULL;

/*
 * Data shared with copy procs.
 */
SharedInfo_t *SharedInfo = NULL;

/*
 * Stager's state information.
 */
StagerStateInfo_t *State = NULL;

/*
 * Path to stager's work directory
 */
char *WorkDir = NULL;

/*
 * Path to stager's copy executable.
 */
char *CopyExecPath = NULL;

/*
 * Set of signals to block.  Only the thread that calls
 * sigwait() will receive the signals.
 */
sigset_t signalSet;

int
main(
	/* LINTED argument unused in function */
	int argc,
	char *argv[])
{
	sigset_t set;

	program_name = argv[0];
	CustmsgInit(1, NULL);

	sprintf(fullpath, "%s/%s", SAM_VARIABLE_PATH, STAGER_DIRNAME);
	SamStrdup(WorkDir, fullpath);

	CustmsgInit(1, NULL);

	Defaults = GetDefaults();

	/*
	 * Check to see if not started from sam-fsd.
	 */
	if (strcmp(GetParentName(), SAM_FSD) != 0) {
		DebugMode = TRUE;
	}

	/*
	 * Change directory.  This keeps the core file separate.
	 */
	if (chdir(WorkDir) == -1) {
		FatalSyscallError(EXIT_NORESTART, HERE, "chdir", WorkDir);
	}

	/*
	 * Initialize tracing.
	 */
	TraceInit("stager", TI_stager | TR_MPLOCK | TR_MPMASTER);
	Trace(TR_PROC, "Stager daemon started");

	/*
	 * Set process signal handling.
	 */
	(void) Signals(&signalsArg);

	/*
	 * Block all signals.
	 */
	(void) sigfillset(&set);
	if (pthread_sigmask(SIG_BLOCK, &set, NULL) == -1) {
		LibFatal(sigprocmask, "block signals");
	}

	/*
	 * Create and initialize stager's shared data space.
	 */
	createShared();

	/*
	 * Make media parameters table.  NOTE: this table
	 * must be created before reading the command file.
	 */
	MakeMediaParamsTable();

	/*
	 * Read command file.
	 */
	readCmdFile();

	if (DebugMode == TRUE) {
		return (EXIT_SUCCESS);
	}

	/*
	 * Create stager's shared data.
	 */
	createSharedMapFile();

	Trace(TR_PROC, "Started after %s",
	    StartMode == SM_cold ? "clean shutdown" :
	    StartMode == SM_failover ? "failover" :
	    StartMode == SM_restart ? "abnormal termination" :
	    "unknown state");

	/*
	 * Initialize daemon.
	 */
	initStagerd();

	/*
	 * Check active copy processes if restarting.
	 */
	if (StartMode == SM_restart) {
		CheckCopyProcs();
	}

	/*
	 * Create thread to schedule stage requests.
	 */
	if (createScheduler()) {
		SendCustMsg(HERE, 19009);
		cleanupAndExit(EXIT_FAILURE);
	}

	/*
	 * Create thread to schedule migration toolkit requests.
	 */
	if (FoundThirdParty()) {
		if (createMigrator()) {
			SendCustMsg(HERE, 19009);
			cleanupAndExit(EXIT_FAILURE);
		}
	}

	/*
	 * Create thread to receive stage file requests
	 * from filesystem.
	 */
	if (createFilesystemReader()) {
		SendCustMsg(HERE, 19009);
		cleanupAndExit(EXIT_FAILURE);
	}

	/*
	 * Requeue pending stage requests on the recovered request list
	 * if restarting.
	 */
	if (StartMode == SM_restart) {
		if (StageReqs.entries > 0) {
			requeueRequests();
		}
		/*
		 * Force scheduler thread to check orphan procs if copyprocList
		 * has recovered.
		 */
		if (CopyInstanceList != NULL) {
			OrphanProcs = -1;
		}
	}

	/*
	 * Let fs to know stagerd is running.
	 */
	setStagerPid(SharedInfo->si_parentPid);

	/*
	 * Allow filesystemReader thread to receive a request from FS.
	 */
	IdleStager = B_FALSE;

	/*
	 * Initialization complete.  Clear flag and allow reconfig requests.
	 */
	initInProgress = B_FALSE;

	/*
	 * The filesystemReader thread is receiving stage file
	 * requests from the filesystem.  This loop will check for
	 * completed stage file requests and handle staging errors.
	 */
	while (ShutdownStager == 0) {
		int err;
		int id;

		/*
		 * Check status of active stage file requests.
		 */
		err = CheckRequests(&id);
		if (err == CHECK_REQUEST_SUCCESS) continue;

		/*
		 * Found multivolume request which is ready to
		 * stage the next VSN or error retry attempt.
		 * Send request to scheduler.
		 */
		if (err == CHECK_REQUEST_SCHED) {
			(void) SendToScheduler(id);
		}
	}

	/*
	 * If failover is about to begin, set -1.
	 */
	setStagerPid(ShutdownStager == SIGUSR1 ? -1 : 0);

	/*
	 * Shutdown copy procs and cancel threads.
	 */
	ShutdownCopy(ShutdownStager);
	cancelThreads();

	/*
	 * Remove stager's shared data files.
	 */
	RemoveStageReqsMapFile();
	RemoveStageDoneMapFile();
	RemoveCopyProcMapFile();
	RemoveFileSystemMapFile();

	removeStateMapFile();

	if (ShutdownStager == SIGUSR1) {
		SharedInfo->si_parentPid = -1;
		Trace(TR_PROC, "Stager daemon exiting for failover");
	} else {
		removeSharedMapFile();	/* remove stager's shared data last */
		Trace(TR_PROC, "Stager daemon exiting");
	}

	SendCustMsg(HERE, 19003, ShutdownStager);

	return (EXIT_SUCCESS);
}

void
ReconfigLock(void)
{
	PthreadMutexLock(&reconfigMutex);
}

void
ReconfigUnlock(void)
{
	PthreadMutexUnlock(&reconfigMutex);
}

void
CopyInstanceLock(void)
{
	PthreadMutexLock(&copyInstanceMutex);
}

void
CopyInstanceUnlock(void)
{
	PthreadMutexUnlock(&copyInstanceMutex);
}

/*
 * Initialize data, tables, ie. most everything needed by daemon.
 */
static void
initStagerd(void)
{
	int fatal_error;

	/*
	 * Initialize filesystems.
	 */
	fatal_error = InitFilesys();
	if (fatal_error) {
		cleanupAndExit(fatal_error);
	}

	/*
	 * Initialize request list.
	 */
	fatal_error = InitRequestList();
	if (fatal_error) {
		cleanupAndExit(fatal_error);
	}

	/*
	 * Initialize messages.
	 */
	fatal_error = InitMessages();
	if (fatal_error) {
		cleanupAndExit(fatal_error);
	}

	/*
	 * Initialize devices.
	 */
	fatal_error = InitDevices();
	if (fatal_error) {
		cleanupAndExit(fatal_error);
	}

	/*
	 * Initialize removable media.
	 */
	fatal_error = InitMedia();
	if (fatal_error) {
		cleanupAndExit(fatal_error);
	}

	/*
	 * Create removable media files.
	 */
	CreateRmFiles();

	/*
	 * Make stager directories.
	 */
	makeStagerDirs();

	/*
	 * Create, initialize and map in stager's state information.
	 * This state information is used externally.
	 */
	createState();

	OpenLogFile(GetCfgLogFile());	/* open log file */

	sprintf(fullpath, "%s/%s", SAM_EXECUTE_PATH, COPY_PROGRAM_NAME);
	SamStrdup(CopyExecPath, fullpath);
	ASSERT(CopyExecPath != NULL);

	/*
	 * Initialize stage request and completition lists.
	 */
	InitStageDoneList();

	/*
	 * Initialize configuration check.
	 */
	fatal_error = initConfigCheck();
	if (fatal_error) {
		cleanupAndExit(fatal_error);
	}

/*	InitStats();	*/	/* initialize statistics */
}

/*
 * Create stager's shared data memory map file.
 */
static void
createSharedMapFile(void)
{
	int ret;
	struct stat buf;
	size_t len;
	SharedInfo_t *shared;

	sprintf(fullpath, "%s/%s/%s",
	    SAM_VARIABLE_PATH, STAGER_DIRNAME, SHARED_FILENAME);

	/*
	 * Determine start mode if SharedInfo file exists.
	 */
	if (stat(fullpath, &buf) == 0) {
		shared = (SharedInfo_t *)MapInFile(fullpath, O_RDONLY, &len);
		if (shared != NULL) {
			if (len == sizeof (SharedInfo_t) &&
			    shared->si_magic == SHARED_INFO_MAGIC &&
			    shared->si_version == SHARED_INFO_VERSION) {
				Trace(TR_MISC, "Old daemon pid: %ld",
				    shared->si_parentPid);
				if (shared->si_parentPid < 0) {
					StartMode = SM_failover;
				} else if (shared->si_parentPid > 0) {
					int rv;
					struct stat sb;

					/*
					 * If another sam-stagerd is running,
					 * sam-fsd gets restarted and trying
					 * to start sam-stagerd.
					 * Otherwise, sam-stagerd gets restarted
					 * after involuntary failover or
					 * abnormal termination.
					 */
					SetErrno = 0;
					rv = FindProc(STAGERD_PROGRAM_NAME, "");
					if (rv > 0) {
						Trace(TR_ERR,
						    "Another stagerd %ld is "
						    "already running.",
						    shared->si_parentPid);
						UnMapFile(shared, len);
						cleanupAndExit(EXIT_FAILURE);
					}
					/*
					 * If starting on different host and
					 * HASAM running file exists, daemon
					 * is restarted after involuntary
					 * failover. Otherwise, restarted
					 * after abnormal termination.
					 */
					Trace(TR_MISC,
					    "Old/New hostid: 0x%lx/0x%lx",
					    shared->si_hostId,
					    SharedInfo->si_hostId);

					if ((shared->si_hostId !=
					    SharedInfo->si_hostId) &&
					    (stat(HASAM_RUN_FILE, &sb) == 0)) {
						StartMode = SM_failover;
					} else {
						StartMode = SM_restart;
					}
				}
			}
			UnMapFile(shared, len);
		}
		(void) unlink(fullpath);
	}

	ret = WriteMapFile(fullpath, (void *)SharedInfo, sizeof (SharedInfo_t));
	if (ret != 0) {
		FatalSyscallError(EXIT_NORESTART, HERE,
		    "WriteMapFile", fullpath);
	}
	SamFree(SharedInfo);
	SharedInfo = (SharedInfo_t *)MapInFile(fullpath, O_RDWR, NULL);
}

/*
 * Remove stager's shared data memory mapped file.
 */
static void
removeSharedMapFile(void)
{
	sprintf(fullpath, "%s/%s/%s",
	    SAM_VARIABLE_PATH, STAGER_DIRNAME, SHARED_FILENAME);
	RemoveMapFile(fullpath, (void *)SharedInfo, sizeof (SharedInfo_t));
}

/*
 * Make stager directories.
 */
static void
makeStagerDirs(void)
{
	/*
	 * Make directory for memory mapped files.
	 */
	int i;
	int num_drives;

	/*
	 * Make directory for streams.
	 */
	MakeDirectory(SharedInfo->si_streamsDir);

	/*
	 * Make directories for copy procs.
	 */
	num_drives = GetNumAllDrives();
	for (i = 0; i < num_drives; i++) {
		sprintf(fullpath, "%s/rm%d", WorkDir, i);
		MakeDirectory(fullpath);
	}
}

/*
 * Create scheduler thread.  The scheduler thread is responsible
 * for scheduling stage file requests on available drives.
 */
static int
createScheduler(void)
{
	int rval;
	extern SchedComm_t SchedComm;

	PthreadMutexInit(&SchedComm.mutex, NULL);
	PthreadCondInit(&SchedComm.avail, NULL);

	SchedComm.first = SchedComm.last = NULL;

	rval = pthread_create(&SchedComm.tid, NULL, Scheduler,
	    (void *)&SchedComm);

	return (rval);
}

/*
 * Create migration toolkit thread.  The migrator thread is responsible
 * for issuing stage file requests for third party media.
 */
static int
createMigrator(void)
{
	int rval;
	extern SchedComm_t MigComm;

	/*
	 * Could be a reconfiguration request.  Check if thread
	 * is already running.
	 */
	if (MigComm.tid != 0) {
		return (0);
	}

	rval = pthread_create(&MigComm.tid, NULL, Migrator, (void *)&MigComm);

	return (rval);
}

/*
 * Create filesystem reader thread.  This thread will
 * receive stage file requests from the filesystem.
 */
static int
createFilesystemReader(void)
{
	int rval;

	rval = pthread_create(&fsReaderThreadId, NULL, filesystemReader, NULL);

	return (rval);
}

/*
 * Filesystem reader thread.  This thread simply receives stage file
 * requests from the filesystem, adds request to list of active
 * stages and sends it to the scheduler.
 */
static void*
filesystemReader(void)
{
	static int maxActiveExceeded = 0;
	int syserr;
	int status;
	int id;
	sam_stage_arg_t arg;
	sam_stage_request_t request;

	Trace(TR_DEBUG, "File system reader started");

	(void) memset(&request, 0, sizeof (request));
	(void) memset(&arg, 0, sizeof (arg));
	arg.stager_cmd = STAGER_getrequest;
	arg.p.request.ptr = &request;

	while (infiniteLoop) {

		if (IdleStager == B_FALSE) {
			syserr = sam_syscall(SC_stager, &arg, sizeof (arg));

			if (syserr < 0) {
				if (errno == EINTR) continue;
				break;
			}

			if (IdleStager == B_TRUE) {
				ErrorRequest(&request, ECANCELED);
				continue;
			}

			/*
			 * Ignore stage requests if shutdown has requested
			 * for a voluntary failover.
			 */
			if (ShutdownStager == SIGUSR1) {
				continue;
			}
		} else {
			sleep(5);
			continue;
		}

		Trace(TR_FILES, "Received file inode: %d.%d fseq: %d",
		    request.id.ino, request.id.gen, request.fseq);

		/*
		 * Check if request to cancel stage.
		 */
		if (request.flags & STAGE_CANCEL) {
			CancelRequest(&request);

		} else {

			status = REQUEST_LIST_FULL;
			/*
			 * Add to request list.  If okay, send it to
			 * scheduler.
			 */
			while (status == REQUEST_LIST_FULL) {
				id = AddFile(&request, &status);

				if (status == REQUEST_READY) {
					ASSERT(id >= 0);
					(void) SendToScheduler(id);
				} else if (status == REQUEST_LIST_FULL) {
					/*
					 * Request list full.  Wait for more
					 * space to become available.  Another
					 * thread is checking for files that
					 * have finished staging and is freeing
					 * up request space.
					 */
					maxActiveExceeded++;
					if (maxActiveExceeded >
					    MAX_ACTIVE_EXCEEDED_MSG) {
						Trace(TR_DEBUG, "Max active "
						    "stages exceeded");
						SendCustMsg(HERE, 19010);
						maxActiveExceeded = 0;
					}
					sleep(2);
				}
			}
		}
	}

	Trace(TR_DEBUG, "File system reader completed");
	return (NULL);
}

/*
 * Set stagerd's pid in the sam_global_tbl_t.
 */
static void
setStagerPid(
	pid_t pid)
{
	sam_stage_arg_t arg;

	(void) memset(&arg, 0, sizeof (arg));
	arg.stager_cmd = STAGER_setpid;
	arg.p.pid = pid;

	(void *)sam_syscall(SC_stager, &arg, sizeof (arg));
}

/*
 * Cleanup and exit.
 */
static void
cleanupAndExit(
	int error)
{
	exit(error);
}

/*
 * Create and initialize stager's shared data space.
 */
static void
createShared(void)
{
	SamMalloc(SharedInfo, sizeof (SharedInfo_t));
	(void) memset(SharedInfo, 0, sizeof (SharedInfo_t));

	SharedInfo->si_magic = SHARED_INFO_MAGIC;
	SharedInfo->si_version = SHARED_INFO_VERSION;
	SharedInfo->si_hostId = gethostid();
	SharedInfo->si_parentPid = getpid();

	(void) sprintf(SharedInfo->si_stageReqsFile, "%s/%s",
	    WorkDir, STAGE_REQS_FILENAME);

	(void) sprintf(SharedInfo->si_stageReqExtents, "%s/%s",
	    WorkDir, STAGE_REQ_EXTENTNAME);

	(void) sprintf(SharedInfo->si_stageDoneFile, "%s/%s",
	    WorkDir, STAGE_DONE_FILENAME);

	(void) sprintf(SharedInfo->si_copyInstancesFile, "%s/%s",
	    WorkDir, COPY_PROCS_FILENAME);

	(void) sprintf(SharedInfo->si_streamsDir, "%s/%s",
	    WorkDir, STREAMS_DIRNAME);

	(void) sprintf(SharedInfo->si_fileSystemFile, "%s/%s",
	    WorkDir, FILESYSTEM_FILENAME);

	(void) sprintf(SharedInfo->si_diskVolumesFile, "%s/%s",
	    WorkDir, DISK_VOLUMES_FILENAME);

	(void) sprintf(SharedInfo->si_coresDir, "%s", WorkDir);
}

/*
 * Create, initialize and map in stager's state information.
 */
static void
createState(void)
{
	int ret;

	SamMalloc(State, sizeof (StagerStateInfo_t));
	(void) memset(State, 0, sizeof (StagerStateInfo_t));

	State->pid = SharedInfo->si_parentPid;
	(void) strcpy(State->logFile, GetCfgLogFile());
	(void) strcpy(State->streamsDir, SharedInfo->si_streamsDir);
	(void) strcpy(State->stageReqsFile, SharedInfo->si_stageReqsFile);

	sprintf(fullpath, "%s/%s/%s",
	    SAM_VARIABLE_PATH, STAGER_DIRNAME, STAGER_STATE_FILENAME);
	ret = WriteMapFile(fullpath, (void *)State, sizeof (StagerStateInfo_t));
	if (ret != 0) {
		FatalSyscallError(EXIT_NORESTART, HERE,
		    "WriteMapFile", fullpath);
	}
	SamFree(State);

	State = (StagerStateInfo_t *)MapInFile(fullpath, O_RDWR, NULL);
	ASSERT(State != NULL);
}

/*
 * Remove stager's state shared data memory mapped file.
 */
static void
removeStateMapFile(void)
{
	sprintf(fullpath, "%s/%s/%s",
	    SAM_VARIABLE_PATH, STAGER_DIRNAME, STAGER_STATE_FILENAME);
	RemoveMapFile(fullpath, (void *)State, sizeof (StagerStateInfo_t));
}

/*
 * Part of shutdown, cancel any thread that may be working with
 * memory mapped files and should be canceled before the files
 * are removed.
 */
static void
cancelThreads(void)
{
	extern SchedComm_t SchedComm;
	extern pthread_t ReceiveMsgsThreadId;

	pthread_cancel(configCheckThreadId);
	pthread_cancel(fsReaderThreadId);
	pthread_cancel(ReceiveMsgsThreadId);
	pthread_cancel(SchedComm.tid);
}

/*
 * Initialize thread to check for external configuration changes.
 */
static int
initConfigCheck(void)
{
	int fatal_error;

	fatal_error = pthread_create(&configCheckThreadId, NULL,
	    configCheck, NULL);

	return (fatal_error);
}

/*
 * Thread to check for external configuration changes.
 */
#if defined(lint)
#undef sleep
#endif
static void *
configCheck(
	/* LINTED argument unused in function */
	void *arg)
{
	unsigned int interval;

	while (ShutdownStager == 0) {

		/*
		 * Check if log file was removed or truncated.
		 */
		CheckLogFile(GetCfgLogFile());

		interval = CONFIG_CHECK_TIMEOUT_SECS;

		while (ShutdownStager == 0 && interval != 0) {
			unsigned int remain;

			remain = sleep(interval);
			if (interval != 0) {
				interval = remain;
			}
		}
	}

	return (NULL);
}

/*
 * Read stager command file.
 */
static void
readCmdFile(void)
{
	struct stat buf;

	if (stat(cmdFileName, &buf) == -1) {
		/*
		 * Command file not found.
		 */
		cmdFileTime = 0;

	} else {
		cmdFileTime = buf.st_mtim.tv_sec;
	}
	ReadCmds();

	SharedInfo->si_logEvents = GetCfgLogEvents();
}

/*
 * Reconfigure request.
 */
static void
reconfig(void)
{
	int fatal_error;
	struct stat buf;

	/*
	 * Wait until daemon initialization has completed before
	 * attempting a reconfiguration.
	 */
	while (initInProgress == B_TRUE) {
		(void) sleep(5);
	}

	TraceReconfig();
	Trace(TR_MISC, "Reconfigure request received");
	if (stat(cmdFileName, &buf) == -1) {
		/*
		 * Command file not found.
		 */
		buf.st_mtim.tv_sec = 0;
	}

	ReconfigLock();

	if (buf.st_mtim.tv_sec != cmdFileTime) {

		Trace(TR_MISC, "Rereading command file");

		/*
		 * Re-init media parameters table.
		 */
		MakeMediaParamsTable();

		readCmdFile();

		/*
		 * Reconfigure log file.
		 */
		OpenLogFile(GetCfgLogFile());
		(void) strcpy(State->logFile, GetCfgLogFile());
	}

	/*
	 * Reconfigure site defaults.
	 */
	Defaults = GetDefaults();

	/*
	 * Reconfigure devices.
	 */
	fatal_error = InitDevices();
	if (fatal_error) {
		cleanupAndExit(fatal_error);
	}

	/*
	 * Reconfigure filesystems.
	 */
	fatal_error = InitFilesys();
	if (fatal_error) {
		cleanupAndExit(fatal_error);
	}

	/*
	 * Reconfigure removable media.
	 */
	fatal_error = InitMedia();
	if (fatal_error) {
		cleanupAndExit(fatal_error);
	}

	/*
	 * Reconfigure catalog.
	 */
	ReconfigCatalog();

	CreateRmFiles();

	/*
	 * Request reconfigure the copy instance table if exist.
	 */

	CopyInstanceLock();

	if (CopyInstanceList != NULL) {
		CopyInstanceList->cl_reconfig = B_TRUE;
	}

	CopyInstanceUnlock();

	if (FoundThirdParty()) {
		if (createMigrator()) {
			SendCustMsg(HERE, 19009);
			cleanupAndExit(EXIT_FAILURE);
		}
	}

	ReconfigUnlock();
}

/*
 * Fatal signal cleanup.
 */
static void
fatalSignalCleanup(
	/* LINTED argument unused in function */
	int signum)
{
	ShutdownCopy(SIGTERM);
	cancelThreads();

	TraceStageReqs(TR_ERR);
	TraceWorkQueue(TR_ERR);

	ShutdownWork();
}

static void
sigHup(
	/* LINTED argument unused in function */
	int signum)
{
	reconfig();
	SendSig2Copy(SIGHUP);
}

static void
sigTerm(
	/* LINTED argument unused in function */
	int signum)
{
	ShutdownStager = SIGTERM;
}

static void
sigUsr1(
	int signum)
{

	ShutdownStager = signum;

	if (stageDone != NULL) {
		PthreadMutexLock(&stageDone->sd_mutex);
		PthreadCondSignal(&stageDone->sd_cond);
		PthreadMutexUnlock(&stageDone->sd_mutex);
	}
}

static void
sigChld(
	int signum)
{
	CopyProcExit(signum);
}

/*
 * Requeue requests.
 */
static void
requeueRequests(void)
{
	int id;
	int next;
	boolean_t nocopyproc = B_TRUE;
	FileInfo_t *fi;

	/*
	 * Save copyprocList's status, recovered or not.
	 * copyprocList will be allocated after first SendToScheduler() call
	 * if it's not recovered or allocated yet.
	 */
	if (CopyInstanceList != NULL) {
		nocopyproc = B_FALSE;
	}

	Trace(TR_MISC, "requeueRequests: entries: %d nocopyproc: %d",
	    StageReqs.entries, nocopyproc);

	SeparateMultiVolReq();
	id = StageReqs.requeue;
	while (id >= 0) {

		fi = GetFile(id);
		ASSERT(fi != NULL);

		next = fi->next;
		if (GET_FLAG(fi->flags, FI_ORPHAN) == 0) {
			if (GET_FLAG(fi->flags,
			    (FI_DCACHE|FI_DONE|FI_ACTIVE))) {
				Trace(TR_MISC, "Delete pending request: %d.%d "
				    "flags: %04x",
				    fi->id.ino, fi->id.gen, fi->flags);
				DeleteRequest(id);
			} else if (IsFileSystemMounted(fi->fseq) == B_FALSE) {
				Trace(TR_MISC, "Delete request: %d.%d "
				    "flags: %04x fseq: %d not mounted",
				    fi->id.ino, fi->id.gen, fi->flags,
				    fi->fseq);
				DeleteRequest(id);
			} else {
				pthread_mutexattr_t mattr;

				PthreadMutexattrInit(&mattr);
				PthreadMutexattrSetpshared(&mattr,
				    PTHREAD_PROCESS_SHARED);
				PthreadMutexInit(&fi->mutex, &mattr);
				PthreadMutexattrDestroy(&mattr);

				Trace(TR_MISC, "Requeue request: %d.%d "
				    "flags:%04x context:%d",
				    fi->id.ino, fi->id.gen, fi->flags,
				    (int)fi->context);

				(void) SendToScheduler(id);
			}
		} else if (nocopyproc == B_TRUE) {
			/*
			 * Request is orphan and copyprocList not recovered.
			 * Need to delete request here since no way to know when
			 * stage gets done. Copy process sets FI_ORPHAN.
			 */
			Trace(TR_MISC, "Delete orphan request: %d.%d "
			    "flags: %04x context: %d",
			    fi->id.ino, fi->id.gen, fi->flags,
			    (int)fi->context);
			DeleteRequest(id);
		} else {
			int i;

			for (i = 0; i < CopyInstanceList->cl_entries; i++) {
				CopyInstanceInfo_t *ci;

				ci = &CopyInstanceList->cl_data[i];
				if (ci->ci_pid == fi->context) {
					/*
					 * Orphan copy proc is still staging
					 * this file. Leave this as staging.
					 */
					Trace(TR_MISC, "Orphan request: %d.%d "
					    "is still active"
					    " flags: %04x context: %d",
					    fi->id.ino, fi->id.gen,
					    fi->flags, (int)fi->context);
					fi->next = -1;
					break;
				}
			}
			if (i >= CopyInstanceList->cl_entries) {
				Trace(TR_MISC, "Delete orphan request: %d.%d "
				    "flags: %04x context: %d",
				    fi->id.ino, fi->id.gen, fi->flags,
				    (int)fi->context);
				DeleteRequest(id);
			}
		}
		id = next;
	}
	if (State != NULL) {
		State->reqEntries = StageReqs.entries;
		State->reqAlloc = StageReqs.alloc;
	}
}
