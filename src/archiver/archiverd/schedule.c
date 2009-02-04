/*
 * schedule.c - Schedule archives.
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

#pragma ident "$Revision: 1.138 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <signal.h>
#include <stddef.h>
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

/* SAM-FS headers. */
#include "pub/rminfo.h"
#include "sam/types.h"
#include "sam/exit.h"
#include "aml/fifo.h"
#include "sam/lib.h"
#include "aml/catalog.h"
#include "aml/catlib.h"

/* Local headers. */
#include "archiverd.h"
#include "device.h"
#include "volume.h"

/* Macros. */
#define	VOL_INCR 17

/* Wait flags - entries in the wait queue. */
enum {
	WF_acnors = 0x01,	/* sam-arcopy norestart error */
	WF_dkIdle = 0x02,
	WF_rmIdle = 0x04,
	WF_schedule = 0x08,
	WF_stageVol = 0x10,
	WF_max
};

/* Types. */

/* Returns from findResources() */
typedef enum {
	FS_busy = 1,		/* Resources busy */
	FS_noarch,		/* Files in ArchReq not archivable now */
	FS_start,		/* Start an archive copy */
	FS_volumes,		/* No volumes available */
	FS_max
} FRstatus_t;

/* Private data. */

/*
 * Scheduling locks and queues.
 * Main interlock.
 * Used to control access scheduling action.
 * All static variables in this module are protected by the main interlock.
 */
static pthread_mutex_t scheduleMutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t schedWait = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t schedWaitMutex = PTHREAD_MUTEX_INITIALIZER;
static int schedWaitCount = 0;	/* Count of scheduler runs */

/* The queues. */
static struct Queue archQ =	/* ArchReq-s that are being archived */
		{ "archive" };
static struct Queue schedQ =	/* ArchReq-s waiting to be archived */
		{ "schedule" };
static struct Queue waitQ =	/* ArchReq-s that are waiting to be started */
		{ "wait" };

/*
 * Available volumes.
 * This is a list of the volumes available for assigning to an ArchReq.
 */
static struct VolAvail {
	int	alloc;		/* Number of entries allocated */
	int	count;		/* Number of entries that are valid */
	int	toUse;		/* Number of volumes to use */
	struct VolInfo entry[1];
} *volsAvail = NULL;

/*
 * Overflow volumes.
 * This is a list of the volumes that have overflowed.
 */
static struct VolOverflow {
	int	alloc;			/* Number of entries allocated */
	struct VolOverflowEntry {
		mtype_t	VoMtype;	/* Volume media type */
		vsn_t	VoVsn;		/* Volume label */
		int	VoArcopyNum;	/* arcopy number */
	} entry[1];
} *volsOverflow = NULL;

static ExecState_t dkState = ES_run;
static ExecState_t rmState = ES_run;
static boolean_t amldStart = FALSE;
static boolean_t requeueCheck = FALSE;
static char *msgFname;		/* Resource message file name */
static int waitFlags = 0;	/* Entries in the wait queue */
static int dkDrivesAllow;	/* Disk archive allowed to use */
static int hcDrivesAllow;	/* Honeycomb archive allowed to use */
static int rmDrivesAllow;	/* Removable media drives allowed to use */
static int msgNum;		/* Resource message number */

#if defined(TEST_W_DBX)
static boolean_t runCopies = TRUE;
#endif /* defined(TEST_W_DBX) */

/* Private functions. */
static int archiveFiles(void);
static void arcopyComplete(char *argv1, int stat);
static struct ArchReq *cartridgeBusy(struct VolInfo *vi);
static void checkQueueTime(void);
static int cmp_aDkVolspace(const void *p1, const void *p2);
static int cmp_dDkVolspace(const void *p1, const void *p2);
static int cmp_aVolspace(const void *p1, const void *p2);
static int cmp_dVolspace(const void *p1, const void *p2);
static int drivesAvailable(void);
static struct QueueEntry *findArchReq(char *ariname, int *cpi);
static FRstatus_t findDkVolumes(struct ArchReq *ar, struct ArchSet *as,
		int drivesToUse);
static FRstatus_t findCpiWork(struct ArchReq *arDrive, struct ArchSet *as);
static FRstatus_t findOverflowVolume(struct ArchReq *arVolReq,
		struct ArchSet *as);
static FRstatus_t findResources(struct ArchReq *ar, struct ArchSet *as);
static FRstatus_t findRmVolume(struct ArchReq *ar, struct ArchSet *as,
		fsize_t spaceRequired, int cpi);
static fsize_t getOvflmin(struct ArchSet *as, char *mtype);
static void initSchedule(void);
static boolean_t isArchLibDriveFree(int aln);
static void moreCpiWork(struct ArchReq *ar, int cpi);
static void requeueEntries(void);
static int startArcopy(struct ArchReq *ar, int cpi);
static void stopArcopys(struct ArchReq *ar, ExecControl_t ctrl);
static void traceDriveStatus(void);
static boolean_t waitArchReq(struct ArchReq *ar);
static void waitMessage(void);
static void wakeup(void);


/*
 * Schedule archiving.
 */
void
Schedule(void)
{
	initSchedule();
	ThreadsInitWait(wakeup, wakeup);

	while (AdState->AdExec <= ES_cmd_errors) {
		ThreadsSleep(1);
	}
	while (AdState->AdExec < ES_term) {
		static int waiting = 0;

		PthreadMutexLock(&schedWaitMutex);
		while (schedWaitCount <= 0) {
			time_t	scheduleTime;
			char ts[ISO_STR_FROM_TIME_BUF_SIZE];

			scheduleTime = time(NULL) + WAIT_TIME;
			if (AdState->AdExec == ES_wait) {
				/* Waiting for %s */
				PostOprMsg(4300, ":arrun");
				scheduleTime = time(NULL) + LONG_TIME;
			} else if (waiting != 0) {
				/* Waiting for resources. */
				PostOprMsg(4313);
			} else if (archQ.QuHead.QeFwd != &archQ.QuHead) {
				/* Archiving files */
				PostOprMsg(4328);
			} else if (schedQ.QuHead.QeFwd == &schedQ.QuHead) {
				/* Idle */
				PostOprMsg(4336);
			}
			waiting = 0;
			waitMessage();

			/*
			 * Wait for entries in the schedule queue.
			 */
			Trace(TR_ARDEBUG, "Waiting until %s",
			    TimeToIsoStr(scheduleTime, ts));
			ThreadsCondTimedWait(&schedWait, &schedWaitMutex,
			    scheduleTime);
			schedWaitCount++;	/* Execute if timed out */
		}
		schedWaitCount = 0;
		PthreadMutexUnlock(&schedWaitMutex);
		if (amldStart) {
			amldStart = FALSE;
			ThreadsReconfigSync(RC_intReconfig);
		}
		ThreadsReconfigSync(RC_wait);
		DeviceCheck();
		VolumeCheck();
		if (requeueCheck) {
			requeueCheck = FALSE;
			waitFlags = 0;
			requeueEntries();
		}

		if (AdState->AdExec == ES_run) {
			if (schedQ.QuHead.QeFwd != &schedQ.QuHead) {
				waiting = archiveFiles();
			}
		}
		checkQueueTime();
		ThreadsReconfigSync(RC_allow);
	}

	Trace(TR_ARDEBUG, "Schedule() exiting");
}


/*
 * Dequeue an archive request.
 */
boolean_t
ScheduleDequeue(
	char *dequeueArname)
{
	struct QueueEntry *qe;
	boolean_t dequeued;

	Trace(TR_ARDEBUG, "ScheduleDequeue(%s)", dequeueArname);
	PthreadMutexLock(&scheduleMutex);
	dequeued = TRUE;

	/*
	 * Search wait queue.  Remove if found.
	 */
	for (qe = waitQ.QuHead.QeFwd; qe != &waitQ.QuHead; qe = qe->QeFwd) {
		if (strcmp(qe->QeArname, dequeueArname) == 0) {
			QueueRemove(qe);
			waitMessage();
			goto out;
		}
	}

	/*
	 * Search schedule queue.  Remove if found.
	 */
	for (qe = schedQ.QuHead.QeFwd; qe != &schedQ.QuHead; qe = qe->QeFwd) {
		if (strcmp(qe->QeArname, dequeueArname) == 0) {
			Trace(TR_MISC, "schedQ Dequeue(%s 0x%x)",
			    dequeueArname, (int)qe);
			QueueRemove(qe);
			goto out;
		}
	}
	dequeued = FALSE;

	/*
	 * Search archive queue.  Stop arcopy-s if found.
	 */
	for (qe = archQ.QuHead.QeFwd; qe != &archQ.QuHead; qe = qe->QeFwd) {
		struct ArchReq *ar;

		ar = qe->QeAr;
		if (strcmp(qe->QeArname, dequeueArname) == 0 &&
		    (ar->ArStatus & ARF_rmreq)) {
			stopArcopys(ar, EC_stop);
			goto out;
		}
	}

out:
	PthreadMutexUnlock(&scheduleMutex);
	if (dequeued) {
		ComposeEnqueue(qe);
	}
	return (dequeued);
}


/*
 * Add an archive request to queue.
 */
void
ScheduleEnqueue(
	struct QueueEntry *qe)
{
	struct ArchReq *ar;

	ar = qe->QeAr;
	ar->ArSchedPriority = ar->ArPriority;
	memset(&ar->ArCpi[0], 0, ar->ArDrives * sizeof (struct ArcopyInstance));
	PthreadMutexLock(&scheduleMutex);
	if (waitArchReq(ar)) {
		QueueAdd(qe, &waitQ);
		waitMessage();
	} else {
		Trace(TR_QUEUE, "SchedQ enqueue (%s)", qe->QeArname);
		QueueAdd(qe, &schedQ);
		wakeup();
	}
	PthreadMutexUnlock(&scheduleMutex);
}


/*
 * Return volume status.
 */
boolean_t		/* TRUE if volume is in use. */
ScheduleGetVolStatus(
	mtype_t mtype,
	vsn_t vsn)
{
	struct QueueEntry *qe;
	int	i;
	int	response;

	PthreadMutexLock(&scheduleMutex);
	response = 0;
	for (qe = archQ.QuHead.QeFwd; qe != &archQ.QuHead; qe = qe->QeFwd) {
		struct ArchReq *ar;
		int	i;

		ar = qe->QeAr;
		for (i = 0; i < ar->ArDrives; i++) {
			struct ArcopyInstance *ci;

			ci = &ar->ArCpi[i];
			if (strcmp(ci->CiMtype, mtype) == 0 &&
			    strcmp(ci->CiVsn, vsn) == 0) {
				response = 1;
				goto found;
			}
		}
	}

	/*
	 * Check overflow volumes.
	 */
	for (i = 0; i < volsOverflow->alloc; i++) {
		struct VolOverflowEntry *vo;

		vo = &volsOverflow->entry[i];
		if (strcmp(vo->VoMtype, mtype) == 0 &&
		    strcmp(vo->VoVsn, vsn) == 0) {
			response = 1;
			goto found;
		}
	}

found:
	PthreadMutexUnlock(&scheduleMutex);
	return (response);
}


/*
 * Get a volume for an archive request.
 * Provide another volume for volume overflow if allowed.
 */
int
ScheduleRequestVolume(
	char *ariname,		/* ArchReq instance */
	fsize_t fileSize)	/* Size of file being archived */
{
	struct ArchSet *as;
	struct ArchReq *ar;
	struct ArchReq *arReq;
	struct ArcopyInstance *ci;
	struct QueueEntry *qe;
	struct VolOverflowEntry *vo;
	fsize_t ovflmin;
	int	cpi;
	int	i;
	int	retval;

	/*
	 * Find ArchReq and copy instance.
	 */
	retval = -1;
	PthreadMutexLock(&scheduleMutex);
	qe = findArchReq(ariname, &cpi);
	if (qe == NULL) {
		goto out;
	}

	ar = qe->QeAr;
	ci = &ar->ArCpi[cpi];
	as = FindArchSet(ar->ArAsname);
	if (as == NULL) {
		goto out;
	}
	ovflmin = getOvflmin(as, ci->CiMtype);
	if (ovflmin == 0 || fileSize < ovflmin) {
		/*
		 * Volume overflow not allowed.
		 */
		goto out;
	}

	if (as->AsDbgFlags & ASDBG_tstovfl) {
		/*
		 * Continue on same volume.
		 */
		retval = 0;
	}

	/*
	 * Save overflow volume.
	 * We need to be able to show the volume in use to the recycler.
	 */
	for (i = 0; i < volsOverflow->alloc; i++) {
		vo = &volsOverflow->entry[i];
		if (*vo->VoVsn == '\0') {
			break;
		}
	}
	if (i >= volsOverflow->alloc) {
		size_t  size;

		volsOverflow->alloc += VOL_INCR;
		size = sizeof (struct VolOverflow) +
		    ((volsOverflow->alloc - 1) *
		    sizeof (struct VolOverflowEntry));
		SamRealloc(volsOverflow, size);
		vo = &volsOverflow->entry[i];
		memset(vo, 0, VOL_INCR * sizeof (struct VolOverflowEntry));
	}
	vo->VoArcopyNum = ci->CiArcopyNum;
	memmove(vo->VoMtype, ci->CiMtype, sizeof (vo->VoMtype));
	memmove(vo->VoVsn, ci->CiVsn, sizeof (vo->VoVsn));

	/*
	 * Make drive appear not in use.
	 */
	ci->CiAln = -1;

	/*
	 * Make a copy of the ArchReq.
	 */
	SamMalloc(arReq, sizeof (struct ArchReq));
	memmove(arReq, ar, sizeof (struct ArchReq));
	arReq->ArState = ARS_volRequest;
	arReq->ArDrives = 1;
	arReq->ArFiles = 0;
	arReq->ArFileIndex = 0;
	arReq->ArCount = 0;
	memset(&arReq->ArCpi[0], 0, sizeof (struct ArcopyInstance));
	arReq->ArCpi[0].CiPid = ci->CiPid;

	/*
	 * Enter the ArchReq in the schedule queue.
	 */
	qe = QueueGetFree();
	qe->QeAr = arReq;
	strncpy(qe->QeArname, "volRequest", sizeof (qe->QeArname) - 1);
	Trace(TR_QUEUE, "schedQ add(%s)", "volRequest");
	QueueAdd(qe, &schedQ);
	wakeup();
	retval = 0;

out:
	PthreadMutexUnlock(&scheduleMutex);
	return (retval);
}


/*
 * Run the Schedule() thread.
 */
void
ScheduleRun(
	char *msg)
{
	requeueCheck = TRUE;
	Trace(TR_ARDEBUG, "ScheduleRun(%s)", msg);
	wakeup();
}


/*
 * Set disk archiving state.
 * EC_run allows archiving.
 * EC_idle stops copies after an archive file is complete.
 * EC_stop stops copy immediately.
 */
void
ScheduleSetDkState(
	ExecControl_t ctrl)
{
	struct QueueEntry *qe;

	PthreadMutexLock(&scheduleMutex);
	if (ctrl != EC_run) {
		dkState = ES_wait;

		/*
		 * Stop arcopy-s that are active on disk archives.
		 */
		for (qe = archQ.QuHead.QeFwd; qe != &archQ.QuHead;
		    qe = qe->QeFwd) {
			struct ArchReq *ar;

			ar = qe->QeAr;
			if (ar->ArFlags & AR_diskArchReq) {
				stopArcopys(ar, ctrl);
			}
		}
	} else {
		dkState = ES_run;
	}
	PthreadMutexUnlock(&scheduleMutex);
}


/*
 * Set filesystem archiving state.
 * EC_run allows archiving.
 * EC_idle stops copies after an archive file is complete.
 * EC_stop stops copy immediately.
 */
void
ScheduleSetFsState(
	struct FileSysEntry *fs,
	ExecControl_t ctrl)
{
	struct QueueEntry *qe, *qeNext;

	FileSysStatus(fs);
	PthreadMutexLock(&scheduleMutex);
	if (ctrl != EC_run) {
		/*
		 * Stop arcopy-s that are active on this filesystem.
		 */
		for (qe = archQ.QuHead.QeFwd; qe != &archQ.QuHead;
		    qe = qe->QeFwd) {
			struct ArchReq *ar;

			ar = qe->QeAr;
			if (strcmp(ar->ArFsname, fs->FsName) == 0) {
				stopArcopys(ar, ctrl);
			}
		}
		/*
		 * Search schedule queue.  Move to wait queue if found.
		 */
		for (qe = schedQ.QuHead.QeFwd; qe != &schedQ.QuHead;
		    qe = qeNext) {
			struct ArchReq *ar;

			qeNext = qe->QeFwd;
			ar = qe->QeAr;
			if (strcmp(ar->ArFsname, fs->FsName) == 0 &&
			    waitArchReq(ar)) {
				QueueRemove(qe);
				QueueAdd(qe, &waitQ);
				waitMessage();
			}
		}
	}
	PthreadMutexUnlock(&scheduleMutex);
}


/*
 * Set removable media archiving state.
 * EC_run allows archiving.
 * EC_idle stops copies after an archive file is complete.
 * EC_stop stops copy immediately.
 */
void
ScheduleSetRmState(
	ExecControl_t ctrl)
{
	struct QueueEntry *qe;

	PthreadMutexLock(&scheduleMutex);
	if (ctrl == EC_amld_start) {
		amldStart = TRUE;
	} else if (ctrl != EC_run) {
		/*
		 * sam-amld stopping does not change the state.
		 * The lack of removable media will prevent archiving.
		 */
		if (ctrl != EC_amld_stop) {
			rmState = ES_wait;
		} else {
			ctrl = EC_stop;
		}

		/*
		 * Stop arcopy-s that are active on removable media.
		 */
		for (qe = archQ.QuHead.QeFwd; qe != &archQ.QuHead;
		    qe = qe->QeFwd) {
			struct ArchReq *ar;

			ar = qe->QeAr;
			if (!(ar->ArFlags & AR_diskArchReq)) {
				stopArcopys(ar, ctrl);
			}
		}
	} else {
		rmState = ES_run;
	}
	PthreadMutexUnlock(&scheduleMutex);
}


/*
 * Trace scheduler tables.
 */
void
ScheduleTrace(void)
{
	PthreadMutexLock(&scheduleMutex);
	traceDriveStatus();
	QueueTrace(HERE, &schedQ);
	QueueTrace(HERE, &archQ);
	QueueTrace(HERE, &waitQ);
	PthreadMutexUnlock(&scheduleMutex);
}


/* Private functions. */


/*
 * Archive files.
 * RETURN: Non-zero if waiting for resources.
 */
static int
archiveFiles(void)
{
	struct QueueEntry *qe, *qeNext;
	FRstatus_t status;
	int	waiting;

	if (drivesAvailable() == 0) {
		/* No drives available */
		PostOprMsg(4332);
		Trace(TR_QUEUE, "No drives available.");
		for (qe = schedQ.QuHead.QeFwd; qe != &schedQ.QuHead;
		    qe = qe->QeFwd) {
			ArchReqMsg(HERE, qe->QeAr, 4332);
		}
		return (1);
	}

	/* Scheduling archives */
	PostOprMsg(4304);
	Trace(TR_MISC, "Scheduling archives");

	/*
	 * Make an assignment of resources so that scheduling priority can be
	 * set.
	 */
	PthreadMutexLock(&scheduleMutex);
	if (*TraceFlags & (1 << TR_queue)) {
		QueueTrace(HERE, &schedQ);
	}
	waiting = 0;
	for (qe = schedQ.QuHead.QeFwd; qe != &schedQ.QuHead; qe = qeNext) {
		struct ArchReq *ar;
		struct ArchSet *as;

		qeNext = qe->QeFwd;
		ar = qe->QeAr;
		as = FindArchSet(ar->ArAsname);
		if (as == NULL) {
			Trace(TR_MISC, "Invalid archive set for ArchReq %s",
			    qe->QeArname);
			ar->Ar.MfValid = 0;
		}
		if (ar->ArState == ARS_volRequest) {
			if (as == NULL ||
			    findOverflowVolume(ar, as) == FS_start) {
				/*
				 * Dequeue entry and discard ArchReq.
				 */
				QueueRemove(qe);
				QueueFree(qe);
				SamFree(ar);
			}
			continue;
		}

		if (ar->ArState <= ARS_done && !ArchReqValid(ar)) {
			/*
			 * Dequeue and return invalid ArchReq to compose.
			 */
			ar->ArFlags |= AR_unqueue;
			QueueRemove(qe);
			ComposeEnqueue(qe);
			continue;
		}

		/*
		 * Verify that stage volumes are available for all
		 * selected files.
		 */
		if (ar->ArSelSpace == 0 ||
		    (ar->ArFlags & AR_offline &&
		    ComposeNonStageFiles(ar) == ar->ArSelFiles)) {
			/*
			 * All of the selected files are or were non-stagable.
			 * Add ArchReq to wait queue.
			 */
			QueueRemove(qe);
			QueueAdd(qe, &waitQ);
			waitMessage();
			waiting++;
			continue;
		}

		/*
		 * Move entry to archive queue.
		 * The resources assigned while scheduling will appear to be
		 * in use to lower priority entries in the queue.
		 * If scheduling is not successful, the entry will be returned
		 * to the schedule queue.
		 */
		QueueRemove(qe);
		memset(&ar->ArCpi[0], 0,
		    ar->ArDrives * sizeof (struct ArcopyInstance));
		QueueAdd(qe, &archQ);
		status = findResources(ar, as);
		if (status == FS_start) {
			double	priority;

			/*
			 * Set scheduling priority.
			 */
			priority = ar->ArPriority;
			if (volsAvail->entry[0].VfFlags & VF_loaded) {
				Trace(TR_QUEUE,
				    "  ArchReq %s: archive volume loaded",
				    qe->QeArname);
				priority += as->AsPrArch_ld;
			}
			if (ar->ArFlags & AR_offline) {
				Trace(TR_QUEUE,
				    "  ArchReq %s: offline files",
				    qe->QeArname);
				priority += as->AsPrOffline;
			}
			if (volsAvail->toUse > 1) {
				priority += as->AsPrArch_ovfl;
			}
			if (priority > PR_MAX) {
				priority = PR_MAX;
			}
			if (priority < PR_MIN) {
				priority = PR_MIN;
			}
			if (priority != ar->ArSchedPriority) {
				/*
				 * Priority has changed, requeue entry.
				 */
				ar->ArSchedPriority = priority;
				QueueRemove(qe);
				QueueAdd(qe, &archQ);
			}
		} else {
			/*
			 * Cannot schedule entry.
			 */
			QueueRemove(qe);
			if (ar->ArState == ARS_cpiMore) {
				/*
				 * Discard a copy instance work entry.
				 */
				QueueFree(qe);
				SamFree(ar);
				continue;
			}

			ar->ArDrivesUsed = 0;
			if (status == FS_noarch) {
				/*
				 * Cannot archive this entry - move it
				 * to the wait queue.
				 */
				QueueAdd(qe, &waitQ);
			} else {
				/*
				 * Move it back to the schedule queue.
				 */
				Trace(TR_QUEUE,
				    "SchedQ move (%s)", qe->QeArname);
				QueueAdd(qe, &schedQ);
			}
			waiting++;
		}
	}

	if (archQ.QuHead.QeFwd != &archQ.QuHead) {
		int	started;

		/*
		 * Start arcopy-s for ArchReq-s in the archive queue.
		 */
		started = 0;
		for (qe = archQ.QuHead.QeFwd; qe != &archQ.QuHead;
		    qe = qeNext) {
			struct ArchReq *ar;
			int	cpi;
			int	cpiStarted;

			qeNext = qe->QeFwd;
			ar = qe->QeAr;
			if (ar->ArState == ARS_archive) {
				continue;
			}
			if (ar->ArState == ARS_cpiMore) {
				/* Get the real guy */
				cpi = ar->ArCount;
				ar =
				    (struct ArchReq *)(void *)(ar->ArFileIndex);
#if defined(AR_DEBUG)
				ArchReqTrace(ar, *TraceFlags & (1 << TR_files));
#endif /* defined(AR_DEBUG) */
				if (startArcopy(ar, cpi) == 0) {
					started++;
				}
				ar = qe->QeAr;
				QueueRemove(qe);
				QueueFree(qe);
				SamFree(ar);
				continue;
			}
#if defined(AR_DEBUG)
			ArchReqTrace(ar, *TraceFlags & (1 << TR_files));
#endif /* defined(AR_DEBUG) */

			for (cpi = 0, cpiStarted = 0; cpi < ar->ArDrives;
			    cpi++) {
				if (*ar->ArCpi[cpi].CiVsn != '\0' &&
				    startArcopy(ar, cpi) == 0) {
					cpiStarted++;
					started++;
				}
			}
			if (cpiStarted == 0) {
				/*
				 * Cannot start any copies in the ArchReq,
				 * move it back to the schedule queue.
				 */
				/* scheduler error */
				ArchReqMsg(HERE, ar, 4361);
				ar->ArState = ARS_schedule;
				waitFlags |= WF_schedule;
				ar->ArFlags |= AR_schederr;
				QueueRemove(qe);
				Trace(TR_QUEUE,
				    "schedQ archiveFiles err(%s)",
				    qe->QeArname);
				QueueAdd(qe, &schedQ);
			} else {
				ar->ArMsgSent = 0;
			}
		}
		if (started > 0 && (*TraceFlags & (1 << TR_queue))) {
			QueueTrace(HERE, &archQ);
		}

		/*
		 * Check all archive queue entries.
		 */
		for (qe = archQ.QuHead.QeFwd; qe != &archQ.QuHead;
		    qe = qeNext) {
			struct ArchReq *ar;
			int	cpi;

			qeNext = qe->QeFwd;

			ar = qe->QeAr;
			if (!ArchReqValid(ar)) {
				/*
				 * Dequeue and return invalid ArchReq
				 * to compose.
				 */
				ar->ArFlags |= AR_unqueue;
				QueueRemove(qe);
				ComposeEnqueue(qe);
				continue;
			}

			/*
			 * Check copy instances.
			 */
			for (cpi = 0; cpi < ar->ArDrives; cpi++) {
				if (*ar->ArCpi[cpi].CiVsn != '\0') {
					break;
				}
			}
			if (cpi == ar->ArDrives) {
				/*
				 * No arcopy-s active.
				 * Return ArchReq to Compose().
				 */
				Trace(TR_QUEUE, "Orphan ArchReq %s",
				    qe->QeArname);
				QueueRemove(qe);
				ComposeEnqueue(qe);
			}
		}
	}

	PthreadMutexUnlock(&scheduleMutex);
	return (waiting);
}


/*
 * Process completed arcopy.
 */
static void
arcopyComplete(
	char *argv1,
	int stat)	/* Child status */
{
	struct QueueEntry *qe;
	struct ArchReq *ar;
	struct ArcopyInstance *ci;
	int	cpi;
	int	i;

	PthreadMutexLock(&scheduleMutex);
	qe = findArchReq(argv1, &cpi);
	if (qe == NULL) {
		PthreadMutexUnlock(&scheduleMutex);
		Trace(TR_ERR, "arcopy(%s) %d not in archive queue",
		    argv1, stat);
		return;
	}
	ar = qe->QeAr;
	ci = &ar->ArCpi[cpi];
	*ci->CiVsn = '\0';
	*AdState->AdArchReq[ci->CiArcopyNum] = '\0';

	if (!(ci->CiFlags & CI_diskInstance)) {
		struct ArchDriveEntry *ad;
		struct ArchLibEntry *al;

		al = &ArchLibTable->entry[ci->CiAln];
		ad = &al->AlDriveTable[ci->CiRmFn - al->AlRmFn];
		ad->AdFlags &= ~AD_busy;

		/*
		 * Remove any overflow volumes.
		 */
		for (i = 0; i < volsOverflow->alloc; i++) {
			struct VolOverflowEntry *vo;

			vo = &volsOverflow->entry[i];
			if (*vo->VoVsn != '\0' &&
			    vo->VoArcopyNum == ci->CiArcopyNum) {
				memset(vo, 0, sizeof (struct VolOverflowEntry));
			}
		}
	}

	Trace(TR_MISC, "%s[%d](%s) finished.", AC_PROG, (int)ci->CiPid, argv1);
	ci->CiPid = 0;

	/* Copy finished */
	snprintf(ci->CiOprmsg, sizeof (ci->CiOprmsg), GetCustMsg(4343));

	/*
	 * Check all arcopy-s in this ArchReq.
	 */
	for (i = 0; i < ar->ArDrives; i++) {
		if (*ar->ArCpi[i].CiVsn != '\0') {
			/*
			 * An arcopy is busy.
			 * If we are doing removable media archiving, try
			 * to find more work for the completed copy instance.
			 */
			if (!(ar->ArStatus & ARF_rmreq) &&
			    !(ar->ArFlags & AR_diskArchReq) &&
			    (ci->CiFlags & CI_more)) {
				*ci->CiVsn = ' ';  /* Make the instance busy */
				moreCpiWork(ar, cpi);
			}
			PthreadMutexUnlock(&scheduleMutex);
			return;
		}
	}

	/*
	 * Remove from archive queue.
	 * Return the ArchReq to Compose().
	 */
	ar = qe->QeAr;
	QueueRemove(qe);
	if ((stat >> 8) == EXIT_NORESTART) {
		ar->ArFlags |= AR_acnors;
	}
	PthreadMutexUnlock(&scheduleMutex);
	ComposeEnqueue(qe);
	wakeup();
}


/*
 * Determine if a cartridge is busy.
 * Scan active arcopy-s.
 * Check for a matching library and element address.
 * RETURN: ArchReq using cartridge
 */
static struct ArchReq *
cartridgeBusy(
	struct VolInfo *vi)
{
	struct QueueEntry *qe;

	for (qe = archQ.QuHead.QeFwd; qe != &archQ.QuHead; qe = qe->QeFwd) {
		struct ArchReq *ar;
		int	i;

		ar = qe->QeAr;
		for (i = 0; i < ar->ArDrives; i++) {
			if (*ar->ArCpi[i].CiVsn != '\0' &&
			    ar->ArCpi[i].CiAln == vi->VfAln &&
			    ar->ArCpi[i].CiSlot == vi->VfSlot) {
				return (ar);
			}
		}
	}
	return (NULL);
}


/*
 * Check queue time.
 */
static void
checkQueueTime(void)
{
	struct QueueEntry *qe;
	struct ArchReq *ar;
	struct ArchSet *as;
	time_t	now;

	now = time(NULL);
	PthreadMutexLock(&scheduleMutex);
	for (qe = schedQ.QuHead.QeFwd; qe != &schedQ.QuHead; qe = qe->QeFwd) {
		ar = qe->QeAr;
		as = FindArchSet(ar->ArAsname);
		if (as != NULL && ar->ArTimeQueued + as->AsQueueTime < now &&
		    !ArchReqCustMsgSent(ar, 4024)) {
			SendCustMsg(HERE, 4024, qe->QeArname,
			    ar->ArCpi[0].CiOprmsg);
		}
	}
	for (qe = waitQ.QuHead.QeFwd; qe != &waitQ.QuHead; qe = qe->QeFwd) {
		ar = qe->QeAr;
		as = FindArchSet(ar->ArAsname);
		if (as != NULL && ar->ArTimeQueued + as->AsQueueTime >= now &&
		    !ArchReqCustMsgSent(ar, 4024)) {
			SendCustMsg(HERE, 4024, qe->QeArname,
			    ar->ArCpi[0].CiOprmsg);
		}
	}
	PthreadMutexUnlock(&scheduleMutex);
}


/*
 * Compare dk volume space for ascending sort.
 * Maintain catalog order for equal space.
 */
static int
cmp_aDkVolspace(
	const void *p1,
	const void *p2)
{
	struct VolInfo *v1 = (struct VolInfo *)p1;
	struct VolInfo *v2 = (struct VolInfo *)p2;
	double	f1, f2;

	f1 = (double)v1->VfSpace / (double)v1->VfCapacity;
	f2 = (double)v2->VfSpace / (double)v2->VfCapacity;
	if (f1 > f2) {
		return (1);
	}
	if (f1 < f2) {
		return (-1);
	}
	return (0);
}


/*
 * Compare dk volume space for descending sort.
 * Maintain catalog order for equal space.
 */
static int
cmp_dDkVolspace(
	const void *p1,
	const void *p2)
{
	struct VolInfo *v1 = (struct VolInfo *)p1;
	struct VolInfo *v2 = (struct VolInfo *)p2;
	double	f1, f2;

	f1 = (double)v1->VfSpace / (double)v1->VfCapacity;
	f2 = (double)v2->VfSpace / (double)v2->VfCapacity;
	if (f1 < f2) {
		return (1);
	}
	if (f1 > f2) {
		return (-1);
	}
	return (0);
}


/*
 * Compare volume space for ascending sort.
 * Maintain catalog order for equal space.
 */
static int
cmp_aVolspace(
	const void *p1,
	const void *p2)
{
	struct VolInfo *v1 = (struct VolInfo *)p1;
	struct VolInfo *v2 = (struct VolInfo *)p2;

	if ((v1->VfFlags & VF_busy) && !(v2->VfFlags & VF_busy)) {
		return (1);
	}
	if ((v2->VfFlags & VF_busy) && !(v1->VfFlags & VF_busy)) {
		return (-1);
	}
	if (v1->VfSpace > v2->VfSpace) {
		return (1);
	}
	if (v1->VfSpace < v2->VfSpace) {
		return (-1);
	}

	if (v1->VfAln > v2->VfAln) {
		return (1);
	}
	if (v1->VfAln < v2->VfAln) {
		return (-1);
	}

	if (v1->VfSlot > v2->VfSlot) {
		return (1);
	}
	if (v1->VfSlot < v2->VfSlot) {
		return (-1);
	}
	return (0);
}


/*
 * Compare volume space for descending sort.
 * Maintain catalog order for equal space.
 */
static int
cmp_dVolspace(
	const void *p1,
	const void *p2)
{
	struct VolInfo *v1 = (struct VolInfo *)p1;
	struct VolInfo *v2 = (struct VolInfo *)p2;

	if ((v1->VfFlags & VF_busy) && !(v2->VfFlags & VF_busy)) {
		return (1);
	}
	if ((v2->VfFlags & VF_busy) && !(v1->VfFlags & VF_busy)) {
		return (-1);
	}
	if (v1->VfSpace < v2->VfSpace) {
		return (1);
	}
	if (v1->VfSpace > v2->VfSpace) {
		return (-1);
	}

	if (v1->VfAln > v2->VfAln) {
		return (1);
	}
	if (v1->VfAln < v2->VfAln) {
		return (-1);
	}

	if (v1->VfSlot > v2->VfSlot) {
		return (1);
	}
	if (v1->VfSlot < v2->VfSlot) {
		return (-1);
	}
	return (0);
}


/*
 * Check for drives available.
 */
static int
drivesAvailable(void)
{
	int	aln;
	int	drivesAvail;

	if (*TraceFlags & (1 << TR_queue)) {
		traceDriveStatus();
	}

	dkDrivesAllow = 0;
	hcDrivesAllow = 0;
	rmDrivesAllow = 0;
	drivesAvail = 0;
	for (aln = 0; aln < ArchLibTable->count; aln++) {
		struct ArchLibEntry *al;

		al = &ArchLibTable->entry[aln];
#if !defined(USE_HISTORIAN)
		if (al->AlFlags & AL_historian) {
			continue;
		}
#endif /* !defined(USE_HISTORIAN) */
		if (al->AlFlags & AL_disk) {
			dkDrivesAllow += al->AlDrivesAllow;
		} else if (al->AlFlags & AL_honeycomb) {
			hcDrivesAllow += al->AlDrivesAllow;
		} else {
			rmDrivesAllow += al->AlDrivesAllow;
		}
		drivesAvail += al->AlDrivesAvail;
	}
	return (drivesAvail);
}


/*
 * Find ArchReq in queue.
 */
static struct QueueEntry *
findArchReq(
	char *ariname,
	int *cpi)
{
	struct QueueEntry *qe;
	struct ArchReq *ar;
	char	*asname;
	char	*pc, *ps;
	int	seqnum;

	pc = strrchr(ariname, '.');
	*pc = '\0';
	*cpi = atoi(pc + 1);

	/* Separate file system and Archive Set */
	asname = strchr(ariname, '.');
	*asname++ = '\0';

	/* Separate Archive Set and sequence number */
	ps = strrchr(asname, '.');
	*ps = '\0';
	seqnum = atoi(ps + 1);

	for (qe = archQ.QuHead.QeFwd; qe != &archQ.QuHead; qe = qe->QeFwd) {
		ar = qe->QeAr;
		if (strcmp(ar->ArFsname, ariname) == 0 &&
		    strcmp(ar->ArAsname, asname) == 0 &&
		    ar->ArSeqnum == seqnum) {
			break;
		}
	}
	*pc = *(asname - 1) = *ps = '.';
	if (qe == &archQ.QuHead) {
		return (NULL);
	}
	return (qe);
}


/*
 * Find disk volume(s) for an archive request.
 */
static FRstatus_t
findDkVolumes(
	struct ArchReq *ar,
	struct ArchSet *as,
	int drivesToUse)
{
	struct DiskVolsDictionary *diskVols;
	struct VolInfo *vi;
	fsize_t spaceAvail;
	char	arname[ARCHREQ_NAME_SIZE];
	int	i;
	int	volsTried;

	Trace(TR_QUEUE, "ArchReq %s: assigning volumes, space:%s",
	    ArchReqName(ar, arname),
	    StrFromFsize(ar->ArSelSpace, 1, NULL, 0));

	ThreadsDiskVols(B_TRUE);
	diskVols = DiskVolsNewHandle(program_name, DISKVOLS_VSN_DICT, 0);
	if (diskVols == NULL) {
		goto out;
	}

	volsTried = 0;
	volsAvail->count = 0;
	spaceAvail = 0;

	/*
	 * Find all volumes for the number of drives to use.
	 */
	for (;;) {
		if (volsAvail->count >= volsAvail->alloc) {
			size_t size;

			volsAvail->alloc += VOL_INCR;
			size = sizeof (struct VolAvail) +
			    ((volsAvail->alloc - 1) * sizeof (struct VolInfo));
			SamRealloc(volsAvail, size);
		}
		vi = &volsAvail->entry[volsAvail->count];

		if (GetDkArchiveVol(as, diskVols, volsTried, vi) == -1) {
			break;
		}
		if (vi->VfSpace >= ar->ArMinSpace) {
			spaceAvail += vi->VfSpace;
			volsAvail->count++;
		}
		volsTried++;
	}

	if (volsAvail->count == 0) {
		goto out;
	}

	/*
	 * Sort the available volumes.
	 * If we are not filling VSNs, bigger volumes first.
	 */
	if (volsAvail->count > 1) {
		if (!(as->AsEflags & AE_fillvsns)) {
			qsort(volsAvail->entry, volsAvail->count,
			    sizeof (struct VolInfo), cmp_dDkVolspace);
		} else {
			/*
			 * -fillvsns uses the volume with the least space
			 * for all drives.
			 */
			qsort(volsAvail->entry, volsAvail->count,
			    sizeof (struct VolInfo), cmp_aDkVolspace);
			volsAvail->count = 1;
		}
	}
	volsAvail->count = min(volsAvail->count, drivesToUse);
	spaceAvail = 0;
	for (i = 0; i < volsAvail->count; i++) {
		vi = &volsAvail->entry[i];
		spaceAvail += vi->VfSpace;
	}
	if (ar->ArMinSpace < spaceAvail) {
		(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
		ThreadsDiskVols(B_FALSE);
		return (FS_start);
	}

out:
	/* No volumes available */
	ArchReqMsg(HERE, ar, 4333);
	/* No volumes available for Archive Set %s */
	if (!ArchReqCustMsgSent(ar, 4010)) {
		SendCustMsg(HERE, 4010, as->AsName);
	}
	(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
	ThreadsDiskVols(B_FALSE);
	return (FS_volumes);
}


/*
 * Find more work for drive.
 */
static FRstatus_t
findCpiWork(
	struct ArchReq *arDrive,
	struct ArchSet *as)
{
	struct ArcopyInstance *ci;
	struct ArchReq *ar;
	FRstatus_t status;
	int	cpi;

	/*
	 * Select the files for copy instance.
	 * Find and assign resources for copy instance.
	 */
	/* Get the real guy */
	cpi = arDrive->ArCount;
	ar = (struct ArchReq *)(void *)(arDrive->ArFileIndex);
	ComposeForCpi(ar, as, cpi);
	msgNum = 0;
	msgFname = "";

	ci = &ar->ArCpi[cpi];
	if (ci->CiFiles != 0) {
		struct VolInfo *vi;

		status = findRmVolume(ar, as, ci->CiSpace, cpi);
		if (status == FS_start) {
			/*
			 * Enter resources found.
			 */
			vi = &volsAvail->entry[0];
			ci->CiAln = vi->VfAln;
			ci->CiSlot = vi->VfSlot;
			strncpy(ci->CiMtype, vi->VfMtype, sizeof (ci->CiMtype));
			strncpy(ci->CiVsn, vi->VfVsn, sizeof (ci->CiVsn));
			ci->CiVolSpace = vi->VfSpace;
		}
	} else {
		status = FS_noarch;
	}
	return (status);
}


/*
 * Find overflow volume.
 */
static FRstatus_t
findOverflowVolume(
	struct ArchReq *arVolReq,	/* ArchReq to process */
	struct ArchSet *as)
{
	struct ArchDriveEntry *ad;
	struct ArchLibEntry *al;
	struct ArchReq *ar;
	struct ArcopyInstance *ci;
	struct QueueEntry *qe;
	struct VolInfo *vi;
	FRstatus_t status;
	pid_t	pid;
	int	cpi;

	/*
	 * Get the ArchReq and copy instance of the original request.
	 */
	pid = arVolReq->ArCpi[0].CiPid;
	for (qe = archQ.QuHead.QeFwd; qe != &archQ.QuHead; qe = qe->QeFwd) {
		ar = qe->QeAr;
		for (cpi = 0; cpi < ar->ArDrives; cpi++) {
			ci = &ar->ArCpi[cpi];
			if (ci->CiPid == pid) {
				goto found;
			}
		}
	}

	/*
	 * Not found.
	 * Returning FS_Start will cause this ArchReq to be dequeued.
	 */
	Trace(TR_QUEUE, "ar_copy not found %d", (int)arVolReq->ArCpi[0].CiPid);
	return (FS_start);

found:
	Trace(TR_ARDEBUG, "findRmVolume(%lld %lld)", ci->CiSpace,
	    ci->CiBytesWritten);
	arVolReq->ArCpi[0] = *ci;
	status = findRmVolume(arVolReq, as, ci->CiSpace - ci->CiBytesWritten,
	    0);
	if (status != FS_start) {
		if (msgNum != 0 && !ArchReqCustMsgSent(ar, msgNum)) {
			SendCustMsg(HERE, msgNum, as->AsName, msgFname);
		}
		return (status);
	}

	/*
	 * Return volume information to arcopy.
	 */
	vi = &volsAvail->entry[0];
	memmove(ci->CiMtype, vi->VfMtype, sizeof (ci->CiMtype));
	memmove(ci->CiVsn, vi->VfVsn, sizeof (ci->CiVsn));
	ci->CiSlot = vi->VfSlot;
	al = &ArchLibTable->entry[vi->VfAln];
	ci->CiLibDev = SHM_GET_OFFS(al->AlDevent);
	ci->CiAln = vi->VfAln;
	ci->CiFlags &= ~CI_volreq;
	ar->ArMsgSent = 0;
	ad = &al->AlDriveTable[ci->CiRmFn - al->AlRmFn];
	ad->AdFlags |= AD_busy;
	strncpy(ad->AdVi.VfMtype, ci->CiMtype, sizeof (ad->AdVi.VfMtype)-1);
	strncpy(ad->AdVi.VfVsn, ci->CiVsn, sizeof (ad->AdVi.VfVsn)-1);
	return (FS_start);
}


/*
 * Find resources for an archive request.
 */
static FRstatus_t
findResources(
	struct ArchReq *ar,	/* ArchReq to process */
	struct ArchSet *as)	/* Archive set */
{
	struct QueueEntry *qe;
	FRstatus_t status;
	int	asDrives;
	int	cpi;
	int	drivesToUse;
	int	drivesUsed;

	if (ar->ArState == ARS_cpiMore) {
		return (findCpiWork(ar, as));
	}

	/*
	 * Set drives available to use.
	 * Start with number of drives allowed by archiving class.
	 * (disk/removable media)
	 */
	if (ar->ArFlags & AR_disk) {
		drivesToUse = dkDrivesAllow;
	} else if (ar->ArFlags & AR_honeycomb) {
		drivesToUse = hcDrivesAllow;
	} else {
		drivesToUse = rmDrivesAllow;
	}

	/*
	 * Set number of drives allowed for the Archive Set.
	 * The selected space for all the ArchReqs being scheduled
	 * or archiving must be at least 'drivemin'.
	 */
	asDrives = (as->AsFlags & AS_drives) ? as->AsDrives : 1;
	if (asDrives > ar->ArDrives) {
		asDrives = ar->ArDrives;
	}
	if (asDrives > 1 && (as->AsFlags & AS_drivemin)) {
		fsize_t	selSpace;

		/*
		 * Compute selected space in queue for this Archive Set.
		 */
		selSpace = 0;
		for (qe = schedQ.QuHead.QeFwd; qe != &schedQ.QuHead;
		    qe = qe->QeFwd) {
			if (strcmp(qe->QeAr->ArAsname, as->AsName) == 0) {
				selSpace += qe->QeAr->ArSelSpace;
			}
		}
		for (qe = archQ.QuHead.QeFwd; qe != &archQ.QuHead;
		    qe = qe->QeFwd) {
			if (strcmp(qe->QeAr->ArAsname, as->AsName) == 0) {
				selSpace += qe->QeAr->ArSelSpace;
			}
		}
		for (qe = waitQ.QuHead.QeFwd; qe != &waitQ.QuHead;
		    qe = qe->QeFwd) {
			if (strcmp(qe->QeAr->ArAsname, as->AsName) == 0) {
				selSpace += qe->QeAr->ArSelSpace;
			}
		}
		if (selSpace < as->AsDrivemin) {
			/*
			 * Allow only 1 drive for the Archive Set.
			 */
			asDrives = 1;
		}
	}

	/*
	 * Account for drives in use by ArchReqs for this Archive Set.
	 * Account for drives being used by all ArchReqs of the same
	 * archiving class.
	 */
	for (qe = archQ.QuHead.QeFwd; qe != &archQ.QuHead; qe = qe->QeFwd) {
		if ((ar->ArFlags & AR_diskArchReq) !=
		    (qe->QeAr->ArFlags & AR_diskArchReq)) {
			continue;
		}
		for (cpi = 0; cpi < qe->QeAr->ArDrives; cpi++) {
			if (*qe->QeAr->ArCpi[cpi].CiVsn != '\0') {
				if (strcmp(qe->QeAr->ArAsname, as->AsName) ==
				    0) {
					asDrives--;
					if (asDrives <= 0) {
						/* Archive set drives busy */
						ArchReqMsg(HERE, ar, 4329);
						return (FS_busy);
					}
				}
				drivesToUse--;
				if (drivesToUse <= 0) {
					/* All drives in use */
					ArchReqMsg(HERE, ar, 4347);
					return (FS_busy);
				}
			}
		}
	}

	/*
	 * Restrict drives to use to the ArchReq specification.
	 * And the file count.
	 */
	if (ar->ArDrives < drivesToUse) {
		drivesToUse = ar->ArDrives;
	}
	if (ar->ArSelFiles < drivesToUse) {
		drivesToUse = ar->ArSelFiles;
	}
	if (ar->ArStageVols != 0 && ar->ArStageVols < drivesToUse) {
		drivesToUse = ar->ArStageVols;
	}

	if (drivesToUse > 1) {
		fsize_t	minSize;
		int64_t	count;

		/*
		 * Decide drive count based on drivemin (or archmax)
		 * and space required.
		 */
		if (as->AsFlags & AS_drivemin) {
			minSize = as->AsDrivemin;
		} else {
			minSize = GetArchmax(as, NULL);
		}
		if (minSize > 0) {
			count = ar->ArSelSpace / minSize;
			if (count < drivesToUse) {
				drivesToUse = (int)count;
			}
		}
		if (drivesToUse <= 0) {
			drivesToUse = 1;
		}
	}
	drivesUsed = 0;

	/*
	 * Assign resources for the drives available.
	 * Assign the maximum number of drives possible to the ArchReq.
	 * We may not have enough non-busy volumes, or the drives may not
	 * be in the library holding the volumes to use.
	 */
	status = FS_busy;
	if (ar->ArFlags & AR_diskArchReq) {
		int	vii;

		/*
		 * Disk archiving.
		 */
		status = findDkVolumes(ar, as, drivesToUse);
		if (status != FS_start) {
			return (status);
		}

		/*
		 * Select the files for each copy instance.
		 * Assign the disk volume properties to the copies.
		 */
		ComposeDrives(ar, as, drivesToUse);
		for (cpi = 0, vii = 0; cpi < drivesToUse; cpi++) {
			struct ArcopyInstance *ci;
			struct VolInfo *vi;

			ci = &ar->ArCpi[cpi];
			if (vii >= volsAvail->count) {
				vii = 0;
			}
			vi = &volsAvail->entry[vii++];
			if (ci->CiFiles != 0) {
				if (ar->ArFlags & AR_disk) {
					ci->CiFlags |= CI_disk;
				} else {
					ci->CiFlags |= CI_honeycomb;
				}
				ci->CiAln = vi->VfAln;
				ci->CiSlot = vi->VfSlot;
				strncpy(ci->CiMtype, vi->VfMtype,
				    sizeof (ci->CiMtype));
				strncpy(ci->CiVsn, vi->VfVsn,
				    sizeof (ci->CiVsn));
				ci->CiVolSpace = ci->CiSpace;
				drivesUsed++;
			}
		}


	} else {
		/*
		 * Removable media archiving.
		 * Try to assign the maximum number of drives with resources
		 * available.
		 */
		while (drivesToUse > 0) {

			/*
			 * Select the files for each copy instance.
			 * Find and assign resources for each copy
			 * instance that has files to archive.
			 */
			ComposeDrives(ar, as, drivesToUse);
			msgNum = 0;
			msgFname = "";
			drivesUsed = 0;
			for (cpi = 0; cpi < drivesToUse; cpi++) {
				struct ArcopyInstance *ci;

				ci = &ar->ArCpi[cpi];
				if (ci->CiFiles != 0) {
					struct VolInfo *vi;

					status = findRmVolume(ar, as,
					    ci->CiSpace, cpi);
					if (status != FS_start) {
						/*
						 * Some resource not available.
						 */
						break;
					}

					/*
					 * Enter resources found.
					 */
					vi = &volsAvail->entry[0];
					ci->CiAln = vi->VfAln;
					ci->CiSlot = vi->VfSlot;
					strncpy(ci->CiMtype, vi->VfMtype,
					    sizeof (ci->CiMtype));
					strncpy(ci->CiVsn, vi->VfVsn,
					    sizeof (ci->CiVsn));
					ci->CiVolSpace = vi->VfSpace;
					drivesUsed++;
				}
			}
			if (drivesUsed >= drivesToUse) {

				/*
				 * Resources found for all drives that
				 * we can use.
				 */
				break;
			}

			/*
			 * Try to use what seems available.
			 */
			drivesToUse = drivesUsed;
		}
	}

	if (drivesUsed == 0) {
		/*
		 * No resources found.
		 */
		if (msgNum != 0 && !ArchReqCustMsgSent(ar, msgNum)) {
			SendCustMsg(HERE, msgNum, as->AsName, msgFname);
		}
		return (status);
	}

	/*
	 * Found resources for all copy instances.
	 */
	if (drivesUsed != 1) {
		char arname[ARCHREQ_NAME_SIZE];

		(void) ArchReqName(ar, arname);
		Trace(TR_MISC, "%s divided for %d drives", arname, drivesUsed);
	}
	return (FS_start);
}


/*
 * Find removable media volume(s) for an archive request.
 */
static FRstatus_t
findRmVolume(
	struct ArchReq *ar,	/* ArchReq to process */
	struct ArchSet *as,
	fsize_t spaceRequired,
	int cpi)		/* Arcopy instance working on */
{
	struct VolInfo *vi;
	struct ArchReq *arb;
	struct ArcopyInstance *ci;
	struct FileInfo *fi;
	boolean_t fillvsns;
	boolean_t noDrives;
	fsize_t ovflmin;
	char	arname[ARCHREQ_NAME_SIZE];
	int	drivesInuse;
	int	*fii;
	int	i;
	int	volsTried;

	/*
	 * Collect volumes for assigning to the ArchReq.
	 */
	Trace(TR_QUEUE, "ArchReq %s: assigning volumes, space:%s",
	    ArchReqName(ar, arname),
	    StrFromFsize(spaceRequired, 1, NULL, 0));
	volsAvail->count = 0;
	ci = &ar->ArCpi[cpi];
	drivesInuse = 0;
	msgNum = 0;
	msgFname = "";
	noDrives = TRUE;
	volsTried = 0;
	for (;;) {
		if (volsAvail->count >= volsAvail->alloc) {
			size_t	size;

			ASSERT(volsAvail->count == volsAvail->alloc);
			volsAvail->alloc += VOL_INCR;
			size = sizeof (struct VolAvail) +
			    ((volsAvail->alloc - 1) * sizeof (struct VolInfo));
			SamRealloc(volsAvail, size);
		}
		vi = &volsAvail->entry[volsAvail->count];

		if (GetRmArchiveVol(as, volsTried, ci->CiOwner,
		    ar->ArFsname, vi) == -1) {
			break;
		}
		if (volsTried == 0) {
			fillvsns = (as->AsEflags & AE_fillvsns) ? TRUE : FALSE;
			ovflmin = getOvflmin(as, vi->VfMtype);
			if (ovflmin != 0 && ci->CiMinSpace > ovflmin) {
				fillvsns = FALSE;
			}
		}
		volsTried++;

		/*
		 * Check if volume has less free space then the minimum
		 * required for fillvsns. If so, mark as full and continue.
		 */
		if (fillvsns && vi->VfSpace < as->AsFillvsnsmin) {
			/* Logical VolId to update catalog. */
			struct VolId vid;
			vid.ViFlags = VI_logical;
			strncpy(vid.ViMtype, vi->VfMtype, sizeof (vid.ViMtype));
			strncpy(vid.ViVsn, vi->VfVsn, sizeof (vid.ViVsn));

			Trace(TR_QUEUE, "ArchReq %s: Setting %s.%s full,"
			    " free space (%llu) less than fillvsns (%llu)",
			    ArchReqName(ar, arname),
			    vi->VfMtype,
			    vi->VfVsn,
			    vi->VfSpace,
			    as->AsFillvsnsmin);

			/* Mark volume as full */
			if (CatalogSetField(&vid, CEF_Status,
			    CES_archfull, CES_archfull) == -1) {
				Trace(TR_ERR, "Catalog set full failed: %s.%s",
				    vid.ViMtype,
				    vid.ViVsn);
			}

			continue;
		}
		/*
		 * Skip volume if no drives are available for the
		 * library holding the volume.
		 */
		if (ArchLibTable->entry[vi->VfAln].AlDrivesAvail == 0) {
			continue;
		}
		noDrives = FALSE;

		/*
		 * Skip volume if no drives are free for the library holding
		 * the volume.
		 */
		if (!isArchLibDriveFree(vi->VfAln)) {
			drivesInuse++;
			continue;
		}
		if (vi->VfSpace < ci->CiMinSpace && ovflmin == 0) {
			/*
			 * Volume has too little space to be useful.
			 * Volume will not hold smallest file, and
			 * volume overflow is not allowed.
			 */
			continue;
		}

		volsAvail->count++;

		/*
		 * Note and skip busy volumes that are not reserved by the
		 * archive set.  If a busy volume is reserved to the archive
		 * set, we will try to use it later.
		 */
		if ((arb = cartridgeBusy(vi)) != NULL) {
			vi->VfFlags |= VF_busy;
			if (arb != ar) {
				/*
				 * Not busy for this ArchReq.
				 */
				if (vi->VfFlags & VF_reserved) {
					/* Reserved volume busy */
					ArchReqMsg(HERE, ar, 4330);
					return (FS_busy);
				}
			}
		}

		if (fillvsns) {
			/*
			 * Archive Set requires volumes to be filled.
			 * Volumes will be checked later.
			 */
			continue;
		}
		if (!(vi->VfFlags & VF_busy) && vi->VfSpace >= spaceRequired) {
			/*
			 * First non-busy volume with enough space.
			 * Use this volume.
			 */
			volsAvail->toUse = 1;
			volsAvail->entry[0] = *vi;
			return (FS_start);
		}
	}

	if (volsAvail->count == 0) {
		/*
		 * No volumes available to use -
		 */
		if (volsTried != 0) {
			/*
			 * Some volumes found.
			 * Return busy if drives in use.
			 */
			if (drivesInuse != 0) {
				/* Drives busy */
				ArchReqMsg(HERE, ar, 4335);
				return (FS_busy);
			}
			if (noDrives) {
				/* No drives available */
				ArchReqMsg(HERE, ar, 4332);
				return (FS_busy);
			}
		}

		/*
		 * Otherwise, there are no volumes to hold this ArchReq.
		 */
		/* No volumes available for Archive Set %s */
		msgNum = 4010;
		/* No volumes available */
		ArchReqMsg(HERE, ar, 4333);
		return (FS_volumes);
	}

	/*
	 * No non-busy volume was found that can hold the complete ArchReq.
	 * Sort the available volumes, busy ones last.
	 * If we are not filling VSNs, bigger volumes first.
	 */
	if (volsAvail->count > 1) {
		if (!fillvsns) {
			qsort(volsAvail->entry, volsAvail->count,
			    sizeof (struct VolInfo), cmp_dVolspace);
		} else {
			qsort(volsAvail->entry, volsAvail->count,
			    sizeof (struct VolInfo), cmp_aVolspace);
		}
	}
	if (volsAvail->entry[0].VfFlags & VF_busy) {
		/*
		 * If the first volume is busy, that implies all volumes
		 * are busy.
		 */
		/* Available volume busy */
		ArchReqMsg(HERE, ar, 4331);
		return (FS_busy);
	}

	/*
	 * Find one that will hold at least the minimum size.
	 */
	for (i = 0; i < volsAvail->count; i++) {
		vi = &volsAvail->entry[i];
		Trace(TR_QUEUE, "  %d: %s.%s space:%s %s",
		    i, vi->VfMtype, vi->VfVsn,
		    StrFromFsize(vi->VfSpace, 1, NULL, 0),
		    (vi->VfFlags & VF_busy) ? "busy" : "");
		if (vi->VfSpace > ci->CiMinSpace) {
			/*
			 * First volume with enough space for the minumum
			 * required.  Use this volume.
			 */
			volsAvail->toUse = 1;
			volsAvail->entry[0] = *vi;
			return (FS_start);
		}
	}

	/*
	 * Use the first file for a file name.
	 * This may not be the correct file.
	 */
	fii = LOC_FILE_INDEX(ar);
	fi = LOC_FILE_INFO(0);
	msgFname = fi->FiName;
	if (ovflmin != 0) {
		fsize_t	availSpace;

		/*
		 * Volume overflow allowed.
		 * Check to see if we can do the job.
		 */
		availSpace = 0;
		volsAvail->toUse = 0;
		for (i = 0; i < volsAvail->count; i++) {
			vi = &volsAvail->entry[i];
			volsAvail->toUse++;
			if (volsAvail->toUse > MAX_VOLUMES) {
				break;
			}
			availSpace += vi->VfSpace;
			if (availSpace >= spaceRequired) {
				if (ar->ArState == ARS_volRequest) {
					return (FS_start);
				}
				break;
			}
		}
		ComposeSelectFit(ar, cpi, availSpace, ovflmin);
		if (ci->CiSpace > 0) {
			return (FS_start);
		}

		/*
		 * No space available for Archive Set %s -
		 * File %s too large for volume overflow on remaining volumes
		 */
		msgNum = 4013;
		/* File too large for volume overflow on remaining volumes */
		ArchReqMsg(HERE, ar, 4344, msgFname);
	} else {
		/*
		 * Volume overflow not allowed.
		 * Issue file too large message.
		 */
		if (!(fi->FiFlags & FI_join)) {
			/*
			 * No space available for Archive Set %s -
			 * File %s too large for any remaining volume
			 */
			msgNum = 4014;
			/* File too large for any remaining volume */
			ArchReqMsg(HERE, ar, 4345, msgFname);
		} else {
			/*
			 * No space available for Archive Set %s -
			 * Joined files too large for any remaining volume
			 */
			msgNum = 4015;
			/* Joined files too large for any remaining volume */
			ArchReqMsg(HERE, ar, 4346);
		}
	}
	return (FS_noarch);
}


/*
 * Get ovflmin for an Archive Set.
 */
static fsize_t		/* 0 if volume overflow not allowed */
getOvflmin(
	struct ArchSet *as,
	char *mtype)
{
	fsize_t ovflmin;

	if (as->AsFlags & AS_ovflmin) {
		ovflmin = as->AsOvflmin;
	} else {
		struct MediaParamsEntry *mp;

		/*
		 * Get ovflmin based on media type.
		 */
		mp = MediaParamsGetEntry(mtype);
		if (mp != NULL && (mp->MpFlags & MP_volovfl)) {
			ovflmin = mp->MpOvflmin;
		} else {
			return (0);
		}
	}
	if (ovflmin == 0) {
		ovflmin = 1;
	}
	return (ovflmin);
}


/*
 * Initialize module.
 */
static void
initSchedule(void)
{
	QueueInit(&archQ);
	QueueInit(&waitQ);
	QueueInit(&schedQ);
	SamMalloc(volsAvail, sizeof (struct VolAvail));
	memset(volsAvail, 0, sizeof (struct VolAvail));
	volsAvail->alloc = 1;
	SamMalloc(volsOverflow, sizeof (struct VolOverflow));
	memset(volsOverflow, 0, sizeof (struct VolOverflow));
	volsOverflow->alloc = 1;
}


/*
 * Determine if a drive is available in the library.
 * Count drives being used in this library.
 */
static boolean_t		/* TRUE if a drive is available */
isArchLibDriveFree(
	int aln)		/* Archive library to check */
{
	struct QueueEntry *qe;
	int	drivesAllow;

	drivesAllow = ArchLibTable->entry[aln].AlDrivesAllow;
	if (drivesAllow > ArchLibTable->entry[aln].AlDrivesAvail) {
		drivesAllow = ArchLibTable->entry[aln].AlDrivesAvail;
	}
	for (qe = archQ.QuHead.QeFwd; qe != &archQ.QuHead; qe = qe->QeFwd) {
		int	i;

		for (i = 0; i < qe->QeAr->ArDrives; i++) {
			if (*qe->QeAr->ArCpi[i].CiVsn != '\0' &&
			    qe->QeAr->ArCpi[i].CiAln == aln) {
				drivesAllow--;
				if (drivesAllow <= 0) {
					return (FALSE);
				}
			}
		}
	}
	return (TRUE);
}


/*
 * Assign more work to a copy instance.
 */
static void
moreCpiWork(
	struct ArchReq *ar,
	int cpi)
{
	struct ArchReq *arReq;
	struct QueueEntry *qe;
	char arname[ARCHREQ_NAME_SIZE];

	(void) ArchReqName(ar, arname);
	Trace(TR_QUEUE, "moreCpiWork(%s, %d)", arname, cpi);

	/*
	 * Make a copy of the ArchReq.
	 */
	SamMalloc(arReq, sizeof (struct ArchReq));
	memmove(arReq, ar, sizeof (struct ArchReq));
	arReq->ArState = ARS_cpiMore;
	arReq->ArDrives = 1;
	arReq->ArFiles = 0;
	arReq->ArFileIndex = 0;
	arReq->ArFlags = 0;
	arReq->ArCount = 0;
	memset(&arReq->ArCpi[0], 0, sizeof (struct ArcopyInstance));
	/* Save the real guy */
	arReq->ArCount = cpi;
	arReq->ArFileIndex = (int)ar;

	/*
	 * Enter the ArchReq in the schedule queue.
	 */
	qe = QueueGetFree();
	qe->QeAr = arReq;
	strncpy(qe->QeArname, "moreCpiWork", sizeof (qe->QeArname) - 1);
	Trace(TR_QUEUE, "schedQ more cpi work(%s)", qe->QeArname);
	QueueAdd(qe, &schedQ);
	wakeup();
}


/*
 * Requeue schedule queue entries.
 * Move all wait queue entries to Compose.
 */
static void
requeueEntries(void)
{
	struct QueueEntry *qe, *qeNext;

	PthreadMutexLock(&scheduleMutex);

	for (qe = waitQ.QuHead.QeFwd; qe != &waitQ.QuHead; qe = qeNext) {
		qeNext = qe->QeFwd;
		QueueRemove(qe);
		ComposeEnqueue(qe);
	}

	PthreadMutexUnlock(&scheduleMutex);
}


/*
 * Start an archive copy.
 * RETURN: -1 if start failed.
 */
static int
startArcopy(
	struct ArchReq *ar,	/* ArchReq to assign to an arcopy */
	int cpi)		/* Copy instance */
{
	struct ArchDriveEntry *ad;
	struct ArchSet *as;
	struct ArcopyInstance *ci;
	char	*argv[MAX_START_PROCESS_ARGS];
	char	ariname[ARCHREQ_FNAME_SIZE];
	int	i;

	as = FindArchSet(ar->ArAsname);
	if (as == NULL) {
		return (-1);
	}
	ci = &ar->ArCpi[cpi];
	ad = NULL;

	/*
	 * Find an unused arcopy number.
	 */
	for (i = 0; i < AdState->AdCount; i++) {
		if (*AdState->AdArchReq[i] == '\0') {
			break;
		}
	}
	ci->CiArcopyNum = i;

	/*
	 * Enter ArchReq variables.
	 */
	ar->ArState = ARS_archive;
	if (!(ci->CiFlags & CI_diskInstance)) {
		struct ArchLibEntry *al;

		al = &ArchLibTable->entry[ci->CiAln];
		ci->CiLibDev = SHM_GET_OFFS(al->AlDevent);
		for (i = 0; i < al->AlDrivesNumof; i++) {
			ad = &al->AlDriveTable[i];
			if ((ad->AdFlags & AD_avail) &&
			    !(ad->AdFlags & AD_busy)) {
				break;
			}
		}
		if (i >= al->AlDrivesNumof) {
			return (-1);
		}
		ci->CiRmFn = al->AlRmFn + i;
		if (al->AlFlags & AL_sim) {
			ci->CiFlags |= CI_sim;
		}
	}

	/*
	 * Make tarballs.
	 */
	ComposeMakeTarballs(ar, cpi, GetArchmax(as, ci->CiMtype),
	    getOvflmin(as, ci->CiMtype), ci->CiVolSpace);

	/*
	 * Prepare arcopy arguments.
	 */
	argv[0] = AC_PROG;
	snprintf(ariname, sizeof (ariname), "%s.%s.%d.%d", ar->ArFsname,
	    ar->ArAsname, ar->ArSeqnum, cpi);
	argv[1] = ariname;
	argv[2] = NULL;

#if defined(TEST_W_DBX)
	if (!runCopies) {
		Trace(TR_ARDEBUG, "Would start %s", argv[1]);
		ArchReqTrace(ar, TRUE);
		ar->ArState = ARS_schedule;
		return (0);
	}
	ArchReqTrace(ar, TRUE);
#endif /* defined(TEST_W_DBX) */
	if (StartProcess(argv, arcopyComplete) == -1) {
		return (-1);
	}

	if (ad != NULL) {
		ad->AdFlags |= AD_busy;
		strncpy(ad->AdVi.VfMtype, ci->CiMtype,
		    sizeof (ad->AdVi.VfMtype)-1);
		strncpy(ad->AdVi.VfVsn, ci->CiVsn,
		    sizeof (ad->AdVi.VfVsn)-1);
	}
	strncpy(AdState->AdArchReq[ci->CiArcopyNum], ariname,
	    sizeof (AdState->AdArchReq[0]));
	return (0);
}


/*
 * Stop all arcopy-s for an ArchReq.
 */
static void
stopArcopys(
	struct ArchReq *ar,
	ExecControl_t ctrl)
{
	int	i;

	for (i = 0; i < ar->ArDrives; i++) {
		struct ArcopyInstance *ci;

		ci = &ar->ArCpi[i];
		if (*ci->CiVsn != '\0') {
			if (ctrl == EC_idle) {
				ci->CiFlags |= CI_idled;
			} else {
				if (ci->CiPid != 0) {
					kill(ci->CiPid, SIGTERM);
				}
			}
		}
	}
}


/*
 * Trace drive status.
 */
static void
traceDriveStatus(void)
{
	FILE	*st;
	int	aln;

	Trace(TR_MISC, "Drive status:");
	if ((st = TraceOpen()) == NULL) {
		return;
	}
	for (aln = 0; aln < ArchLibTable->count; aln++) {
		struct ArchLibEntry *al;
		int	adn;

		al = &ArchLibTable->entry[aln];
#if !defined(USE_HISTORIAN)
		if (al->AlFlags & AL_historian) {
			continue;
		}
#endif /* !defined(USE_HISTORIAN) */

		/*
		 * Display the drive status.
		 */
		fprintf(st, " %3d %-10s Drives:%d Available:%d Allowed:%d\n",
		    aln, al->AlName,
		    al->AlDrivesNumof, al->AlDrivesAvail, al->AlDrivesAllow);
		if (al->AlFlags & (AL_disk | AL_honeycomb)) {
			continue;
		}
		for (adn = 0; adn < al->AlDrivesNumof; adn++) {
			struct ArchDriveEntry *ad;

			ad = &al->AlDriveTable[adn];
			fprintf(st, "     ");
			if (!(al->AlFlags & AL_manual)) {
				fprintf(st, "%-10s", ad->AdName);
			} else {
				fprintf(st, "          ");
			}
			fprintf(st, "     Vol:");
			PrintVolInfo(st, &ad->AdVi);
		}
	}
	TraceClose(-1);
}


/*
 * Check for suspended archiving.
 */
static boolean_t	/* TRUE if wait needed */
waitArchReq(
	struct ArchReq *ar)
{
	int	i;

	*ar->ArCpi[0].CiOprmsg = '\0';

	/*
	 * Check for file system conditions that would idle or stop archiving.
	 */
	for (i = 0; i < FileSysTable->count; i++) {
		struct FileSysEntry *fs;

		fs = &FileSysTable->entry[i];
		if (strcmp(fs->FsName, ar->ArFsname) == 0) {
			if (!(fs->FsFlags & FS_mounted)) {
				/* Waiting for %s */
				ArchReqMsg(HERE, ar, 4300, "mount");
			} else if (fs->FsFlags & FS_umount) {
				/* Waiting for %s */
				ArchReqMsg(HERE, ar, 4300, "umount");
			} else if (fs->FsAfState->AfExec == ES_wait) {
				/* Waiting for :arrun %s */
				ArchReqMsg(HERE, ar, 4352, "");
			} else if (fs->FsAfState->AfExec == ES_fs_wait) {
				/* Waiting for :arrun fs.%s */
				ArchReqMsg(HERE, ar, 4353, fs->FsName);
			}
			break;
		}
	}
	if (i >= FileSysTable->count) {
		/* Waiting for %s */
		ArchReqMsg(HERE, ar, 4300, ar->ArFsname);
	}
	if (*ar->ArCpi[0].CiOprmsg != '\0') {
		return (TRUE);
	}

	/*
	 * Check disk or removable media for idled or stopped archiving.
	 * Check a non-stagable ArchReq for a change in stagability.
	 */
	if (dkState != ES_run && (ar->ArFlags & AR_diskArchReq)) {
		/* Waiting for :arrun %s */
		ArchReqMsg(HERE, ar, 4352, "dk");
	} else if (rmState != ES_run && !(ar->ArFlags & AR_disk)) {
		/* Waiting for :arrun %s */
		ArchReqMsg(HERE, ar, 4352, "rm");
	} else if (ar->ArFlags & AR_acnors) {
		waitFlags |= WF_acnors;
		/* sam-arcopy fatal error - see sam-log */
		ArchReqMsg(HERE, ar, 4358);
	} else if (ar->ArFlags & AR_nonstage) {
		if (ComposeNonStageFiles(ar) != ar->ArSelFiles) {
			/*
			 * Some files are now stagable.
			 */
			ar->ArFlags &= ~AR_nonstage;
			*ar->ArCpi[0].CiOprmsg = '\0';
		} else {
			waitFlags |= WF_stageVol;
		}
	}
	return (*ar->ArCpi[0].CiOprmsg != '\0');
}


/*
 * Write wait message.
 */
static void
waitMessage(void)
{
	static struct {		/* Message table */
		int flag;	/* Wait flag for message (0 is end of table) */
		int	msgNum;	/* Catalog message number if non-zero */
		char *msg;	/* Message text for no message number */
	} *wm, waitMsg[] = {
		{ WF_acnors, 4359, "sam-arcopy error" },
		{ WF_dkIdle, 0, ":arrun dk" },
		{ WF_rmIdle, 0, ":arrun rm" },
		{ WF_schedule, 4361, "schedule error" },
		{ WF_stageVol, 4360, "stage volume" },
		{ 0 }
	};
	char	*p;

	if (AdState->AdExec != ES_run) {
		*AdState->AdOprMsg2 = '\0';
		return;
	}
	if (waitQ.QuHead.QeFwd == &waitQ.QuHead) {
		waitFlags = 0;
	}
	waitFlags |= (dkState != ES_run) ? WF_dkIdle : 0;
	waitFlags |= (rmState != ES_run) ? WF_rmIdle : 0;
	p = AdState->AdOprMsg2;
	if (waitFlags != 0) {
		char	*pe;
		char	*sep = "";

		pe = p + sizeof (AdState->AdOprMsg2);
		/* Waiting for %s */
		snprintf(p, Ptrdiff(pe, p), GetCustMsg(4300), "");
		p += strlen(p);
		for (wm = &waitMsg[0]; wm->flag != 0; wm++) {
			if (waitFlags & wm->flag) {
				char	*msg;

				if (wm->msgNum != 0) {
					msg = GetCustMsg(wm->msgNum);
				} else {
					msg = wm->msg;
				}
				snprintf(p, Ptrdiff(pe, p), "%s%s", sep, msg);
				p += strlen(p);
				sep = ", ";
			}
		}
	}
	*p = '\0';
}


/*
 * Wakeup the Schedule() thread.
 */
void
wakeup(void)
{
	PthreadMutexLock(&schedWaitMutex);
	schedWaitCount++;
	PthreadCondSignal(&schedWait);
	PthreadMutexUnlock(&schedWaitMutex);
}


#if defined(TEST_W_DBX)

/*
 * Scheduler test.
 */
void
SchedulerTest(void)
{
	runCopies = FALSE;
}

#endif /* defined(TEST_W_DBX) */
