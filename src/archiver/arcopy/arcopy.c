/*
 * arcopy.c - Arcopy main program.
 *
 * arcopy copies files from a supplied ArchReq file to removable media in
 * standard tar format.  When the files are successfully written, the
 * file status is updated with the archive information.
 *
 * Usage: sam_arcopy rmedia_file fsName ArchReqName MediaType.VSN
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


#pragma ident "$Revision: 1.113 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

/* Solaris headers. */
#include <sys/param.h>

/* SAM-FS headers. */
#include "pub/rminfo.h"
#include "sam/types.h"
#include "sam/param.h"
#include "sam/exit.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/signals.h"
#include "sam/uioctl.h"
#include "sam/fs/ino.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/diskvols.h"
#include "aml/shm.h"

/* Local headers. */
#include "threads.h"
#define	DEC_INIT
#include "arcopy.h"
#undef	DEC_INIT

#if defined(lint)
#undef sigemptyset
#undef sigfillset
#undef sigaddset
#undef sigdelset
#undef sigprocmask
#endif /* defined(lint) */

/* Macros. */
#define	STAGE_WAIT TRUE
#define	STAGE_NOWAIT FALSE

/* globals */
shm_alloc_t              preview_shm;

/* Private data. */

/* Signals control. */
static void sigAlarm(int sig);
static void sigHup(int sig);
static void sigInt(int sig);
static void sigTerm(int sig);

static struct SignalFunc signalFunc[] = {
	{ SIGALRM, sigAlarm },
	{ SIGHUP, sigHup },
	{ SIGINT, sigInt },
	{ SIGPIPE, sigTerm },
	{ SIGTERM, sigTerm },
	{ 0 }
};

static struct SignalsArg signalsArg = { signalFunc, NULL, "." };

/*
 * Threads control.
 * Table of threads to start and stop.
 */
static struct Threads threads[] = {
	{ "Signals" },
	{ "Read" },
	{ "Setarchive", SetArchive },
	{ "Write", WriteBuffer },
	{ NULL }
};

/*
 * Timeouts.
 */
static struct {
	boolean_t on;
	int	value;
} timeouts[TO_max];
static pthread_mutex_t timeoutMutex = PTHREAD_MUTEX_INITIALIZER;

static enum OfflineCopyMethods olcm;
static upath_t fsName;			/* Filesystem name */
static char *stopMessage = "";
static int stopSignal = 0;

/* Private functions. */
static boolean_t archiveIdled(void);
static void arcopyInit(char *argv[]);
static void arcopyCleanup();
static void getArchReq(char *arname, int cpi);
static void reconfig(ReconfigControl_t ctrl);
static void setIoMethod(int fn);
static int stageFiles(int firstFile);
static void stageOneFile(boolean_t wait);
static void waitForOnline(int firstFile, int nextFileToStage);


int
main(
	int argc,
	char *argv[])
{
	int	nextFileToStage;
	int	fn;

	if (strcmp(GetParentName(), SAM_ARCHIVER) != 0) {
#if !defined(AR_DEBUG)
		fprintf(stderr, "sam-arcopy may be started only from %s\n",
		    SAM_ARCHIVER);
		exit(EXIT_FAILURE);
#else
		Daemon = FALSE;
#endif /* defined(AR_DEBUG) */
	}
	if (argc < 2) {
		fprintf(stderr,
		    "usage: sam-arcopy fsName.ArchReqName.cpi\n");
		exit(EXIT_USAGE);
	}
	snprintf(signalsArg.SaCoreDir, sizeof (signalsArg.SaCoreDir),
	    SAM_ARCHIVERD_HOME"/%s", argv[1]);
	threads[0].TtThread = Signals(&signalsArg);
	arcopyInit(argv);

	Exec = ES_run;
	ThreadsStart(threads, reconfig);
	alarm(10);
	setIoMethod(0);
	ThreadsInitWait(NULL, NULL);

	/*
	 * Stage files before starting.
	 */
	nextFileToStage = 0;
	if (olcm == OC_all || olcm == OC_ahead) {
		nextFileToStage = stageFiles(0);
	}
	if (olcm == OC_all) {
		waitForOnline(0, nextFileToStage);
	}

	/*
	 * Process all files.
	 */
	fn = 0;

	while (!archiveIdled() && fn < FilesNumof) {
		int	firstFile;

		Instance->CiArchives++;
		FilesTable[fn].AfFlags &= ~AF_first;
		firstFile = fn;

		ClearOprMsg();
		setIoMethod(fn);
		if (olcm != OC_direct && olcm != OC_all)  {

			/*
			 * Stage the next batch of files.
			 */
			if (olcm != OC_ahead) {
				nextFileToStage = stageFiles(nextFileToStage);
			}

			/*
			 * Wait for all files to be copied to become online.
			 */
			waitForOnline(fn, nextFileToStage);

			if (olcm == OC_ahead) {
				nextFileToStage = stageFiles(nextFileToStage);
			}
		}

		/*
		 * Copy all files contained in the Archive File (tarball).
		 */
		while (fn < FilesNumof) {
			if (FilesTable[fn].AfFlags & AF_first) {
				break;
			}
			File = &FilesTable[fn];
			fn++;
			if (File->AfFlags & AF_error ||
			    (File->f->FiStatus & FIF_remove)) {
				continue;
			}
			if (olcm != OC_direct && (File->AfFlags & AF_offline)) {
				continue;
			}
			CopyFile();
		}
		EndArchiveFile(firstFile);
		SetArchiveRun(fn);
	}

	SetArchiveRun(FilesNumof);
	Exec = ES_idle;
	if (fn < FilesNumof) {
		Trace(TR_MISC, "Archiving terminated before completion: %s",
		    stopMessage);
	}

	ThreadsStop();
	if (stopSignal != 0) {
		Trace(TR_MISC, "Stopped by %s", StrSignal(stopSignal));
	}

	arcopyCleanup();

	Trace(TR_MISC, "%d file%s archived in %d archive file%s.",
	    FilesArchived, (FilesArchived != 1) ? "s" : "",
	    Instance->CiArchives, (Instance->CiArchives != 1) ? "s" : "");
#if defined(AR_DEBUG)
	if (*TraceFlags & (1 << TR_alloc)) {
		TraceRefs();
	}
#endif /* defined(AR_DEBUG) */
	return (EXIT_SUCCESS);
}


/*
 * Clear timeout.
 * Preserves 'errno'.
 */
void
ClearTimeout(
	enum Timeouts to)
{
	int	saveErrno;

	saveErrno = errno;
	PthreadMutexLock(&timeoutMutex);
	if (to > TO_none && to < TO_max) {
		timeouts[to].on = FALSE;
	}
	PthreadMutexUnlock(&timeoutMutex);
	errno = saveErrno;
}


/*
 * Set timeout.
 */
void
SetTimeout(
	enum Timeouts to)
{
	PthreadMutexLock(&timeoutMutex);
	switch (to) {
	case TO_read:
		timeouts[to].value = MediaParams->MpReadTimeout;
		break;
	case TO_request:
		timeouts[to].value = MediaParams->MpRequestTimeout;
		break;
	case TO_stage:
		timeouts[to].value = MediaParams->MpStageTimeout;
		break;
	case TO_write:
		timeouts[to].value = WriteTimeout;
		break;
	default:
		goto out;
	}
	timeouts[to].on = (timeouts[to].value > 0) ? TRUE : FALSE;

out:
	PthreadMutexUnlock(&timeoutMutex);
#if defined(AR_DEBUG)
	if (to == TO_read && strcmp(File->f->FiName, "Timeout/read") == 0) {
		Trace(TR_DEBUG, "Check read timeout");
		ThreadsSleep(timeouts[TO_read].value + 1);
	}
	if (to == TO_request && strcmp(File->f->FiName, "Timeout/request") ==
	    0) {
		Trace(TR_DEBUG, "Check request timeout");
		ThreadsSleep(timeouts[TO_request].value + 1);
	}
	if (to == TO_stage && strcmp(File->f->FiName, "Timeout/stage") == 0) {
		Trace(TR_DEBUG, "Check stage timeout");
		ThreadsSleep(timeouts[TO_stage].value + 1);
	}
	if (to == TO_write && strcmp(File->f->FiName, "Timeout/write") == 0) {
		Trace(TR_DEBUG, "Check write timeout");
		ThreadsSleep(timeouts[TO_write].value + 1);
	}
#endif /* defined(AR_DEBUG) */
}

/* Private functions. */


/*
 * Check library, drive, and volume for usability.
 */
static boolean_t
archiveIdled(void)
{
	struct VolInfo vfd;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	struct dev_ent *dev;
	struct VolId vid;

	if (Exec != ES_run) {
		return (TRUE);
	}
	if (Instance->CiFlags & CI_idled) {
		Exec = ES_idle;
		stopMessage = "idled";
		return (TRUE);
	}
	if (!ArchReqValid(ArchReq)) {
		Exec = ES_term;
		stopMessage = "archreq invalid";
		return (TRUE);
	}

	if (Instance->CiFlags & (CI_diskInstance | CI_sim)) {
		return (FALSE);
	}

	/*
	 * Check the library.
	 */
	dev = (struct dev_ent *)SHM_REF_ADDR(Instance->CiLibDev);
	if (dev->state != DEV_ON ||
	    !dev->status.b.ready ||
	    dev->status.b.audit) {
		Exec = ES_term;
		stopMessage = "library not ready";
		return (TRUE);
	}

	if (VolsTable->entry[VolCur].VlEq == 0) {
		/*
		 * No drive active.
		 */
		return (FALSE);
	}

	/*
	 * Check drive and media status.
	 */
	if (!RemoteArchive.enabled) {
		for (dev =
		    (struct dev_ent *)SHM_REF_ADDR(shm_ptr_tbl->first_dev);
		    dev != NULL;
		    dev = (struct dev_ent *)SHM_REF_ADDR(dev->next)) {
			if (dev->eq == VolsTable->entry[VolCur].VlEq) {
				break;
			}
		}
		vfd.VfFlags = 0;
		GetDriveVolInfo(dev, &vfd);
		if (vfd.VfFlags & VF_unusable) {
			Exec = ES_term;
			stopMessage = "drive not available";
			return (TRUE);
		}
	}

	/*
	 * Check catalog entry for volume.
	 */
	vid.ViFlags = VI_logical;
	strncpy(vid.ViMtype, Instance->CiMtype, sizeof (vid.ViMtype));
	strncpy(vid.ViVsn, Instance->CiVsn, sizeof (vid.ViVsn));
	ce = CatalogGetEntry(&vid, &ced);
	if (ce == NULL || (ce->CeStatus & RM_VOL_UNUSABLE_STATUS)) {
		Exec = ES_term;
		stopMessage = "volume not usable";
		return (TRUE);
	}

	return (FALSE);
}


/*
 * Initialize program.
 * Arguments: rmedia_file fsName ArchReqName MediaType.VSN
 */
static void
arcopyInit(
	char *argv[])
{
	static struct sam_fs_info fi;
	sam_defaults_t *defaults;
	struct ArfindState *af_state;
	size_t	size;
	char	*p;
	char	*arname;
	int	cpi;

	/*
	 * Set name for trace and log messages.
	 */
	ErrorExitStatus = EXIT_NORESTART;
	program_name = AC_PROG;
	CustmsgInit(Daemon, NotifyRequest);

	if (Daemon) {
		TraceInit(program_name, TI_archiver | TR_MPLOCK);
	} else {
		/*
		 * Allow trace messages to go to stdout/stderr.
		 */
		TraceInit(NULL, TI_none);
		*TraceFlags = (1 << TR_module) | ((1 << TR_date) - 1);
	}
#if defined(AR_DEBUG)
	*TraceFlags |= 1 << TR_ardebug;
#endif /* defined(AR_DEBUG) */
	Trace(TR_MISC, "Start: %s", argv[1]);

	/*
	 * Attach archiver files.
	 */
	strncpy(ArName, argv[1], sizeof (ArName));
	p = strrchr(argv[1], '.');
	*p++ = '\0';
	cpi = atoi(p);
	p = strchr(argv[1], '.');
	*p++ = '\0';
	strncpy(fsName, argv[1], sizeof (fsName) - 1);
	arname = p;
	AdState = ArMapFileAttach(ARCHIVER_DIR"/"ARCHIVER_STATE,
	    AD_MAGIC, O_RDONLY);
	if (AdState == NULL) {
		LibFatal(ArMapFileAttach, "archiver state");
	}

	/*
	 * Set mount point.
	 */
	if (GetFsInfo(fsName, &fi) == -1) {
		LibFatal(GetFsInfo, fsName);
	}
	if (!(fi.fi_status & FS_MOUNTED) ||
	    (fi.fi_status & FS_UMOUNT_IN_PROGRESS)) {
		SendCustMsg(HERE, 4009, fsName);
		exit(EXIT_NORESTART);
	}
	MntPoint = fi.fi_mnt_point;

	DefaultHeader = NULL;

	if (!(defaults = GetDefaults())) {
		LibFatal(GetDefaults, fsName);
	}
	/*
	 * default to using legacy format if both flags are set, which would
	 * be unexpected, to say the least.  Ditto if neither flag is set
	 */
	if (defaults->flags & DF_LEGACY_ARCH_FORMAT) {
		if (defaults->flags & DF_PAX_ARCH_FORMAT) {
			Trace(TR_DEBUG, "both flags set, using legacy");
		}

		Trace(TR_DEBUG, "arcopy using legacy format");
		ZeroOffset = 0;
		ArchiveFormat = LEGACY_FORMAT;
	} else if (defaults->flags & DF_PAX_ARCH_FORMAT) {
		Trace(TR_DEBUG, "arcopy using pax format");
		ZeroOffset = 1;
		ArchiveFormat = PAX_FORMAT;
	} else {
		Trace(TR_DEBUG, "neither flag set, using legacy");
		ZeroOffset = 0;
		ArchiveFormat = LEGACY_FORMAT;
	}

	/*
	 * Get the ArchReq.
	 * Use the information from it to get the mount point, and archive
	 * copy number.
	 */
	getArchReq(arname, cpi);

	/*
	 * Move to execution directory.
	 */
	snprintf(ScrPath, sizeof (ScrPath), "arcopy%d", Instance->CiArcopyNum);
	MakeDir(ScrPath);
	if (chdir(ScrPath) == -1) {
		LibFatal(chdir, ScrPath);
	}
	if (!(Instance->CiFlags & CI_diskInstance)) {
		/*
		 * Not doing a disk archive.
		 * We need access to removable media.
		 */
		if (ShmatSamfs(O_RDONLY) == NULL) {
			LibFatal(ShmatSamfs, "");
		}
		if (VolumeCatalog() == -1) {
			LibFatal(CatalogInit, "");
		}
	}

	if (ArchSetAttach(O_RDONLY) == NULL) {
		LibFatal(ArMapFileAttach,  ARCHIVER_DIR"/"ARCHIVE_SETS);
	}
	ArchiveSet = FindArchSet(ArchReq->ArAsname);
	if (ArchiveSet == NULL) {
		LibFatal(FindArchSet, ArchReq->ArAsname);
	}
	ArchiveCopy = AS_COPY(ArchiveSet->AsName);
	ArchiveMedia = sam_atomedia(Instance->CiMtype);

	/*
	 * Open ioctl file.
	 */
	if ((FsFd = open(MntPoint, O_RDONLY)) <= 0) {
		LibFatal(open, MntPoint);
	}

	/*
	 * Find log file from arfind's state file.
	 */
	snprintf(ScrPath, sizeof (ScrPath), "../%s/"ARFIND_STATE,
	    ArchReq->ArFsname);
	af_state = ArMapFileAttach(ScrPath, AF_MAGIC, O_RDONLY);
	if (af_state == NULL) {
		LibFatal(ArMapFileAttach, ScrPath);
	}
	LogName = af_state->AfLogFile;

	/*
	 * Enter volume in list.
	 */
	size = sizeof (struct Vols);
	SamMalloc(VolsTable, size);
	memset(VolsTable, 0, size);
	VolsTable->count = 1;
	strncpy(VolsTable->entry[0].Vi.VfMtype, Instance->CiMtype,
	    sizeof (VolsTable->entry[0].Vi.VfMtype));
	strncpy(VolsTable->entry[0].Vi.VfVsn, Instance->CiVsn,
	    sizeof (VolsTable->entry[0].Vi.VfVsn));

	/*
	 * Initialize modules.
	 */
	memset(&RemoteArchive, 0, sizeof (RemoteArchive));
	if (Instance->CiFlags & CI_disk) {
		DkInit();
#if !defined(_NoOSD_)
	} else if (Instance->CiFlags & CI_honeycomb) {
		HcInit();

#if defined(lint)
		/* keep lint happy */
		(void) HcReadFromFile(NULL, NULL, 0);
		(void) HcCreateSeqnumMeta(NULL, NULL, 0);
#endif
#endif
	} else {
		RmInit();
	}
}

void
arcopyCleanup()
{
	if (DefaultHeader) {
		ph_destroy_hdr(DefaultHeader);
		DefaultHeader = NULL;
	}
}

/*
 * Get the ArchReq.
 * Access the queue file and make the file info table entry
 * for each file to archive.
 */
static void
getArchReq(
	char *arname,
	int cpi)
{
	struct ArchReq *ar;
	size_t	size;
	int	*fii;
	int	i;

	ar = ArchReqAttach(fsName, arname, O_RDWR);
	if (ar == NULL) {
		LibFatal(ArchReqAttach, arname);
	}
	if (!ArchReqValid(ar)) {
		LibFatal(ArchReqValid, arname);
	}
	Instance = &ar->ArCpi[cpi];
	Instance->CiPid = getpid();
	Instance->CiArchives = 0;
	Instance->CiFilesWritten = 0;
	Instance->CiBytesWritten = 0;
	*Instance->CiOprmsg = '\0';
	OpenOprMsg(Instance->CiOprmsg);
	size = ar->ArFiles * sizeof (struct ArchiveFile);
	SamMalloc(FilesTable, size);
	memset(FilesTable, 0, size);

	/*
	 * Select files matching our instance.
	 */
	FilesNumof = 0;
	fii = LOC_FILE_INDEX(ar);
	for (i = 0; i < ar->ArSelFiles; i++) {
		struct FileInfo *fi;

		fi = LOC_FILE_INFO(i);
		if (fi->FiCpi != cpi || FI_IGNORE) {
			continue;
		}
		if (!(fi->FiFlags & FI_join)) {
			FilesTable[FilesNumof++].f = fi;
		} else {
			int	end;
			int	j;

			/*
			 * Get all joined files.
			 */
			end = fi->FiJoinStart + fi->FiJoinCount;
			for (j = fi->FiJoinStart; j < end; j++) {
				fi = LOC_FILE_INFO(j);
				FilesTable[FilesNumof++].f = fi;
			}
		}
	}

	if (FilesNumof == 0) {
		errno = 0;
		Trace(TR_MISC, "No files to archive");
		exit(EXIT_FAILURE);
	}

	/*
	 * Set archival flags.
	 */
	for (i = 0; i < FilesNumof; i++) {
		struct ArchiveFile *af;
		struct FileInfo *fi;

		af = &FilesTable[i];
		fi = af->f;
		af->AfFlags |= (fi->FiFlags & FI_first) ? AF_first : 0;
		af->AfFlags |= (fi->FiFlags & FI_offline) ? AF_offline : 0;
		af->AfFlags |=
		    (Instance->CiFlags & CI_honeycomb) ? AF_notarhdr : 0;
	}
	Instance->CiFiles = FilesNumof;
	ArchReq = ar;
}


/*
 * Reconfiguration.
 */
void
reconfig(
	/*ARGSUSED0*/
	ReconfigControl_t ctrl)
{
	TraceReconfig();
}


/*
 * Set I/O and offline copy methods.
 */
static void
setIoMethod(
	int fn)		/* First file of batch to stage */
{
	media_t	media;

	olcm = ArchiveSet->AsOlcm;
	media = sam_atomedia(VolsTable->entry[VolCur].Vi.VfMtype);
	if ((ArchiveSet->AsEflags & AE_tapenonstop) &&
	    (media & DT_CLASS_MASK) == DT_TAPE) {
		TapeNonStop = TRUE;
	} else {
		TapeNonStop = FALSE;
	}

	/*
	 * Check first file.
	 */
	if (FilesTable[fn].AfFlags & AF_offline) {
		struct OfflineInfo *oi;

		oi = LOC_OFFLINE_INFO(FilesTable[fn].f);
		if (oi->OiMedia == media &&
		    strcmp(oi->OiVsn, VolsTable->entry[VolCur].Vi.VfVsn) == 0) {

			/*
			 * Copying from the same volume.
			 * Cannot use non-stop or direct copy.
			 */
			if (TapeNonStop) {
				TapeNonStop = FALSE;
				Trace(TR_MISC, "I/O to same volume %s.%s:"
				    " tapenonstop ignored",
				    VolsTable->entry[VolCur].Vi.VfMtype,
				    VolsTable->entry[VolCur].Vi.VfVsn);
			}
			if (olcm == OC_direct) {
				olcm = OC_none;
				Trace(TR_MISC, "I/O to same volume %s.%s:"
				    " copy direct ignored",
				    VolsTable->entry[VolCur].Vi.VfMtype,
				    VolsTable->entry[VolCur].Vi.VfVsn);
			}
		}
		DirectIo = (olcm == OC_direct);
	} else {
		DirectIo = FALSE;
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
	int	i;

	if (getppid() == 1) {
		static boolean_t first = TRUE;

		if (first) {
			first = FALSE;
			/* Parent process died */
			PostOprMsg(4046);
			SendCustMsg(HERE, 4046);
		}
		Exec = ES_idle;
	}
	PthreadMutexLock(&timeoutMutex);
	for (i = 1; i < TO_max; i++) {
		if (timeouts[i].on) {
			timeouts[i].value -= ALARM_INTERVAL;
			if (timeouts[i].value <= 0) {
				char	*name;

				switch (i) {
				case TO_read:
					name = "read";
					break;
				case TO_request:
					name = "request";
					break;
				case TO_stage:
					name = "stage";
					break;
				case TO_write:
					name = "write";
					break;
				default:
					name = "??";
					break;
				}
				Trace(TR_ERR, "%s timeout", name);
				exit(EXIT_FAILURE);
			}
		}
	}
	PthreadMutexUnlock(&timeoutMutex);
	alarm(ALARM_INTERVAL);
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
 * Idle signal.
 */
static void
/*ARGSUSED0*/
sigInt(
	int sig)
{
	Exec = ES_idle;
	stopMessage = "idled";
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
	Trace(TR_MISC, "Stopped by %s", StrSignal(stopSignal));
	Trace(TR_MISC, "%d file%s archived in %d archive file%s.",
	    FilesArchived, (FilesArchived != 1) ? "s" : "",
	    Instance->CiArchives, (Instance->CiArchives != 1) ? "s" : "");
	exit(EXIT_SUCCESS);
}


/*
 * Stage files.
 * All files in the selection list that are marked offline are staged in
 * with no wait.
 * RETURN: Next file to stage.
 */
static int
stageFiles(
	int firstFile)
{
	char	*firstVsn;
	int	fn;
	int	nextFile;

	/*
	 * Find next file.
	 */
	for (nextFile = firstFile; nextFile < FilesNumof; nextFile++) {
		File = &FilesTable[nextFile];
		if (olcm != OC_all && (File->AfFlags & AF_first) &&
		    nextFile != firstFile) {
			break;
		}
	}

	/*
	 * Issue all stages at once.
	 */
	firstVsn = "";
	for (fn = firstFile; fn < nextFile; fn++) {
		File = &FilesTable[fn];
		if (File->AfFlags & AF_offline &&
		    !(File->f->FiStatus & FIF_remove)) {
			struct OfflineInfo *oi;

			/*
			 * Display the files and the volume from
			 * which we're staging.
			 */
			oi = LOC_OFFLINE_INFO(File->f);
			if (strcmp(firstVsn, oi->OiVsn) != 0) {
				firstVsn = oi->OiVsn;
				Trace(TR_MISC, "Staging from %s.%s",
				    sam_mediatoa(oi->OiMedia), oi->OiVsn);
			}
			/* Staging - file %d (%d) from %s.%s  %s/%s */
			PostOprMsg(4348, fn, nextFile,
			    sam_mediatoa(oi->OiMedia), oi->OiVsn,
			    MntPoint, File->f->FiName);

			stageOneFile(STAGE_NOWAIT);
		}
	}
	return (nextFile);
}


/*
 * Stage a file.
 */
static void
stageOneFile(
	boolean_t wait)	/* TRUE if must wait */
{
	struct sam_ioctl_idstage sii;
	int	ret;

	if (File->f->FiFlags & FI_stagesim) {
		File->AfFlags &= ~AF_offline;
		File->AfFlags |= AF_release;
		return;
	}
	sii.id = File->f->FiId;
	sii.copy = File->f->FiStageCopy;
	if (wait) {
		sii.flags = IS_wait;
	} else {
		sii.flags = IS_none;
	}
	SetTimeout(TO_stage);
	ret = ioctl(FsFd, F_IDSTAGE, &sii);
	ClearTimeout(TO_stage);
	if (!wait) {
		return;
	}

	if (ret == 0) {
		File->AfFlags &= ~AF_offline;
		File->AfFlags |= AF_release;
	} else {
		File->AfFlags &= ~AF_offline;
		File->AfFlags |= AF_error;
		Trace(TR_ERR, "stage(%d, %s/%s) failed", sii.copy,
		    MntPoint, File->f->FiName);
	}
}


/*
 * Wait for a batch of offline files to become on-line.
 */
static void
waitForOnline(
	int firstFile,
	int nextFileToStage)
{
	int	fn;

	for (fn = firstFile; fn < nextFileToStage; fn++) {
		File = &FilesTable[fn];
		if (File->AfFlags & AF_offline &&
		    !(File->f->FiStatus & FIF_remove)) {
			struct OfflineInfo *oi;

			/*
			 * Display the files and the volume from
			 * which we're staging.
			 */
			oi = LOC_OFFLINE_INFO(File->f);
			/* Staging - file %d of %d from %s.%s  %s/%s */
			PostOprMsg(4348, fn, FilesNumof,
			    sam_mediatoa(oi->OiMedia), oi->OiVsn,
			    MntPoint, File->f->FiName);

			stageOneFile(STAGE_WAIT);
		}
	}
}
