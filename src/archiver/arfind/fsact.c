/*
 * fsact.c - filesystem activity processing.
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

#pragma ident "$Revision: 1.60 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* Feature test switches. */
/* FILE_TRACE	If defined, turn on DEBUG traces for files processed */

/* ANSI C headers. */
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

/* POSIX headers. */
#include <sys/param.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

/* SAM-FS headers. */
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/syscall.h"
#include "sam/uioctl.h"

/* Local headers. */
#define	ARFIND_PRIVATE
#define	NEED_ARFIND_EVENT_NAMES
#include "arfind.h"
#undef	ARFIND_PRIVATE
#undef	NEED_ARFIND_EVENT_NAMES
#include "dir_inode.h"

/* Event buffer. */
typedef struct Buffer_s {
	struct Buffer_s *BfFwd;		/* Forward link */
	struct Buffer_s *BfBak;		/* Backward link */
	int	BfCount;
	struct arfind_event *BfEvents;
} Buffer_t;

static Buffer_t freeHead;
static Buffer_t fullHead;

static int bufferCount;

/* Private data. */
static pthread_mutex_t bufferMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t examineWait = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t examineWaitMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t fsActThread;

/* Private functions. */
static void addBuffer(Buffer_t *bf, Buffer_t *bfHead);
static void freeBuffer(Buffer_t *bf);
static void obsoleteFsExamine(void *arg);
static Buffer_t *getFreeBuffer(void);
static void initFsAct(void);
static void removeBuffer(Buffer_t *bf);
static void wakeupFsAct(void);
static void wakeupFsExamine(void);


/*
 * Respond to filesystem action.
 */
void
FsAct(void)
{
	Buffer_t *bf;
	struct sam_arfind_arg args;
	time_t	restartTime;

	initFsAct();
	memset(&args, 0, sizeof (args));
	ThreadsInitWait(wakeupFsAct, wakeupFsAct);

	fsActThread = pthread_self();
	strncpy(args.AfFsname, FsName, sizeof (args.AfFsname));
	args.AfWait			= 5;
	args.AfMaxCount		= ARFIND_EVENT_MAX;

	bf = getFreeBuffer();

	/* To quiet lint. */
	restartTime = time(NULL) + UMOUNT_DELAY;

	while (Exec < ES_term) {
		if (Exec == ES_umount) {
			struct sam_fs_info fi;
			char	ts[ISO_STR_FROM_TIME_BUF_SIZE];

			/*
			 * Filesystem has started to unmount.  Wait for it
			 * to complete, or the restart time has been reached.
			 */
			/* Waiting until %s for fs unmount */
			PostOprMsg(4340, TimeToIsoStr(restartTime, ts));
			ThreadsSleep(5);
			if (GetFsInfo(FsName, &fi) == -1 ||
			    !(fi.fi_status & FS_MOUNTED) ||
			    (fi.fi_status & FS_UMOUNT_IN_PROGRESS)) {
				ChangeState(ES_term);
				ThreadsWakeup();
				State->AfNormalExit = TRUE;
				goto out;
			}
			if (restartTime > time(NULL)) {
				continue;
			}
			/*
			 * Resume archiving.
			 * Notify sam-archiverd.
			 */
			ChangeState(State->AfExec | ES_force);
			ThreadsWakeup();
			(void) ArchiverFsMount(FsName);
		}
		if (FsFd < 0 && Exec != ES_umount) {
			if ((FsFd = OpenInodesFile(MntPoint)) < 0) {
				Trace(TR_ERR, "open(%s)", MntPoint);
			}
			ThreadsWakeup();
		}

		State->AfFsactCalls++;

		args.AfCount = 0;
		args.AfBuffer.ptr = bf->BfEvents;
		if (sam_syscall(SC_arfind, &args, sizeof (args)) != 0) {
			if (errno != ETIME && errno != EINTR) {
				Trace(TR_ERR, "sam_syscall(SC_arfind) failed");
			}
		}
		if (args.AfUmount != 0) {
			Trace(TR_ARDEBUG, "Unmount received");
			ChangeState(ES_umount);
			if (FsFd > 0) {
				(void) close(FsFd);
				FsFd = -1;
			}
			ThreadsWakeup();
			restartTime = time(NULL) + UMOUNT_DELAY;
			ThreadsSleep(1);
			continue;
		}

		if (args.AfCount == 0) {
			continue;
		}

		Trace(TR_DEBUG, "Event buffer 0x%x act count: %d overflow: %d",
		    (long)bf->BfEvents, args.AfCount, args.AfOverflow);

		if (args.AfOverflow != 0) {
			ScanfsFullScan();
		}
		State->AfFsactEvents += args.AfCount;

		bf->BfCount = args.AfCount;
		addBuffer(bf, &fullHead);

		/*
		 * Run the FsExamine() thread.
		 */
		PthreadMutexLock(&examineWaitMutex);
		PthreadCondSignal(&examineWait);
		PthreadMutexUnlock(&examineWaitMutex);

		bf = getFreeBuffer();
	}

out:
	if (FsFd > 0) {
		(void) close(FsFd);
		FsFd = -1;
	}
	Trace(TR_ARDEBUG, "FsAct() %d event buffers", bufferCount);
}


/*
 * Trace module.
 */
void
FsActTrace(void)
{
	FILE	*st;
	int	i;

	if ((st = TraceOpen()) == NULL) {
		return;
	}
	fprintf(st, "Fsact - calls: %d, buffers: %d, events: %d\n",
	    State->AfFsactCalls, State->AfFsactBufs, State->AfFsactEvents);
	for (i = 1; i < AE_MAX; i++) {
		fprintf(st, "   %8s:  %d\n", fileEventNames[i],
		    State->AfEvents[i]);
	}

	fprintf(st, "\n");
	TraceClose(-1);
}


/*
 * Thread - Examine files.
 */
void *
FsExamine(
	void *arg)
{
	int	examined;

	ThreadsInitWait(wakeupFsExamine, wakeupFsExamine);

	if (State->AfExamine == EM_noscan_obsolete) {
		obsoleteFsExamine(arg);
		goto out;
	}

	examined = 0;
	while (Exec < ES_term) {
		Buffer_t *bf;
		int	i;
		time_t	timeNow;

		/*
		 * Wait for entries in the list.
		 */
		ThreadsReconfigSync(RC_allow);
		PthreadMutexLock(&examineWaitMutex);
		while (Exec < ES_term && fullHead.BfFwd == &fullHead) {
			PthreadCondWait(&examineWait, &examineWaitMutex);
		}
		PthreadMutexUnlock(&examineWaitMutex);
		if (Exec >= ES_term) {
			break;
		}
		ThreadsReconfigSync(RC_wait);

		bf = fullHead.BfFwd;

		Trace(TR_DEBUG, "Event buffer 0x%x exam count: %d",
		    (long)bf->BfEvents, bf->BfCount);
		removeBuffer(bf);

		/*
		 * Process the file action notifications.
		 */
		examined += bf->BfCount;
		timeNow = time(NULL);
		for (i = 0; i < bf->BfCount; i++) {
			struct arfind_event *ev;
			int	event;

			ev = &bf->BfEvents[i];
			event = ev->AeEvent;
			if (event >= AE_none && event < AE_MAX) {
				State->AfEvents[event]++;
				Trace(TR_DEBUG, "Event inode: %d.%d "
				    "event: '%s'",
				    ev->AeId.ino, ev->AeId.gen,
				    fileEventNames[event]);
				if (event == AE_hwm) {
					Trace(TR_MISC, "%d  High water mark",
					    i);
					if (Exec == ES_run) {
						ScanfsStartScan();
						ArchiveStartRequests();
					}
				} else {
					if (event == AE_remove ||
					    event == AE_rename) {
						ExamInodesRmInode(ev->AeId);
						ArchiveRmInode(ev->AeId);
						IdToPathRmInode(ev->AeId);
						ScanfsRmInode(ev->AeId);
						if (event == AE_remove) {
							continue;
						}
					}
					ExamInodesAddEntry(ev->AeId, event,
					    timeNow, "fsact");
				}
			} else {
				Trace(TR_DEBUG,
				    "Invalid event: %d inode: %d.%d",
				    ev->AeEvent, ev->AeId.ino, ev->AeId.gen);
			}
		}
		freeBuffer(bf);
	}

	Trace(TR_ARDEBUG, "FsExamine() %d files examined", examined);

out:
	ThreadsExit();
	/*NOTREACHED*/
	return (arg);
}


/*
 * Add buffer to end of list.
 */
static void
addBuffer(
	Buffer_t *bf,
	Buffer_t *bfHead)
{
	PthreadMutexLock(&bufferMutex);
	bf->BfFwd = bfHead;
	bf->BfBak = bfHead->BfBak;
	bfHead->BfBak->BfFwd = bf;
	bfHead->BfBak = bf;
	PthreadMutexUnlock(&bufferMutex);
}


/*
 * Free a buffer.
 * Add the buffer to the 'free' list.
 */
static void
freeBuffer(
	Buffer_t *bf)
{
	Trace(TR_DEBUG, "Event buffer 0x%x free",
	    (long)bf->BfEvents);

	addBuffer(bf, &freeHead);
}


/*
 * Get an unused buffer.
 * If an buffer is on the free list, return it.
 * Otherwise, allocate a new buffer.
 */
static Buffer_t *
getFreeBuffer(void)
{
	Buffer_t *bf;

	PthreadMutexLock(&bufferMutex);
	if (freeHead.BfFwd != &freeHead) {
		bf = freeHead.BfFwd;
		bf->BfBak->BfFwd = bf->BfFwd;
		bf->BfFwd->BfBak = bf->BfBak;
		PthreadMutexUnlock(&bufferMutex);
	} else {
		PthreadMutexUnlock(&bufferMutex);
		SamMalloc(bf, sizeof (Buffer_t));
		memset(bf, 0, sizeof (Buffer_t));
		SamMalloc(bf->BfEvents,
		    ARFIND_EVENT_MAX * sizeof (struct arfind_event));
		bufferCount++;
		State->AfFsactBufs++;
		Trace(TR_DEBUG, "Event buffer 0x%x alloc count: %d",
		    (long)bf->BfEvents, bufferCount);
	}
	return (bf);
}


/*
 * Initialize module.
 */
static void
initFsAct(void)
{
	memset(&freeHead, 0, sizeof (freeHead));
	freeHead.BfFwd = freeHead.BfBak = &freeHead;
	memset(&fullHead, 0, sizeof (fullHead));
	fullHead.BfFwd = fullHead.BfBak = &fullHead;
	bufferCount = 0;
}


/*
 * Remove buffer from the list.
 */
static void
removeBuffer(
	Buffer_t *bf)
{
	PthreadMutexLock(&bufferMutex);
	bf->BfBak->BfFwd = bf->BfFwd;
	bf->BfFwd->BfBak = bf->BfBak;
	PthreadMutexUnlock(&bufferMutex);
}


/*
 * Wakeup FsAct().
 */
static void
wakeupFsAct(void)
{
	(void) pthread_kill(fsActThread, SIGUSR1);
}


/*
 * Wakeup FsExamine().
 */
static void
wakeupFsExamine(void)
{
	PthreadMutexLock(&examineWaitMutex);
	PthreadCondSignal(&examineWait);
	PthreadMutexUnlock(&examineWaitMutex);
}

/*
 * Thread - Examine files.  Obsolete.
 * This 4.x no_scan method was replaced in 5.0
 * and should be removed.
 */
static void
obsoleteFsExamine(
	/* LINTED argument unused in function */
	void *arg)
{
	/* Inode stat information. */
	static struct sam_perm_inode pinode;
	static struct sam_disk_inode *dinode = (struct sam_disk_inode *)&pinode;
	static struct PathBuffer pathBuf, *pb = &pathBuf;
	struct ScanListEntry seAdd;
	int		examined;

	memset(&seAdd, 0, sizeof (seAdd));
	examined = 0;

	while (Exec < ES_term) {
		Buffer_t *bf;
		int		i;
		int		removes;

		/*
		 * Wait for entries in the list.
		 */
		ThreadsReconfigSync(RC_allow);
		PthreadMutexLock(&examineWaitMutex);
		while (Exec < ES_term && fullHead.BfFwd == &fullHead) {
			PthreadCondWait(&examineWait, &examineWaitMutex);
		}
		PthreadMutexUnlock(&examineWaitMutex);
		if (Exec >= ES_term) {
			break;
		}
		ThreadsReconfigSync(RC_wait);

		bf = fullHead.BfFwd;
		removeBuffer(bf);

		/*
		 * Process the file action notifications.
		 */
		examined += bf->BfCount;
		removes = 0;
		Trace(TR_ARDEBUG, "Examine %d events %d",
		    bf->BfCount, examined);
		for (i = 0; i < bf->BfCount; i++) {
			struct arfind_event *ev;
			int event;

			ev = &bf->BfEvents[i];
			event = ev->AeEvent;
			if (event >= AE_none && event < AE_MAX) {
				Trace(TR_FILES, "inode %d.%d %s",
				    ev->AeId.ino, ev->AeId.gen,
				    fileEventNames[event]);
				State->AfEvents[event]++;
			} else {
				Trace(TR_FILES, "inode %d.%d %d",
				    ev->AeId.ino, ev->AeId.gen, event);
			}

			switch (event) {

			case AE_rename:
				IdToPathRmInode(ev->AeId);
				ArchiveRmInode(ev->AeId);
				removes++;
				/*FALLTHROUGH*/

			case AE_archive:
			case AE_change:
			case AE_close:
			case AE_create:
			case AE_modify:
			case AE_rearchive:
			case AE_unarchive: {
				if (State->AfExamine < EM_noscan &&
				    event != AE_rename) {
					break;
				}

				/*
				 * Get a copy of the inode.
				 */
				if (GetPinode(ev->AeId, &pinode) != 0) {
#if defined(AR_DEBUG)
					Trace(TR_ERR, "%s stat(%d.%d) failed",
					    fileEventNames[event],
					    ev->AeId.ino, ev->AeId.gen);
#endif /* defined(AR_DEBUG) */
					break;
				}

				/* Done if archdone already set. */
				if (dinode->status.b.archdone) {
					if (event != AE_rename &&
					    event != AE_archive) {
						break;
					}
				}

				EXAM_MODE(dinode) = (short)event;
				CheckInode(pb, &pinode, &seAdd);
				if (event == AE_archive &&
				    pb->PbPath != pb->PbEnd &&
				    S_ISDIR(dinode->mode)) {
					/*
					 * Archive a directory.
					 * Start archiving for it.
					 */
					ArchiveStartDir(pb->PbPath);
				} else if (event == AE_rename &&
				    pb->PbPath != pb->PbEnd &&
				    S_ISDIR(dinode->mode)) {
					/*
					 * A renamed directory.
					 * Enter a scan for it in case it
					 * fits new file properties.
					 */
					seAdd.SeId = dinode->id;
					seAdd.SeCopies = 0;
					seAdd.SeAsn = 0;
					seAdd.SeFlags = SE_subdir;
					seAdd.SeTime = TIME_NOW(dinode);

					ScanfsAddEntry(&seAdd);
				}
				}
				break;

			case AE_hwm:
				Trace(TR_MISC, "%d  High water mark", i);
				if (Exec == ES_run) {
					ScanfsStartScan();
					ArchiveStartRequests();
				}
				break;

			case AE_remove:
				IdToPathRmInode(ev->AeId);
				ArchiveRmInode(ev->AeId);
				removes++;
				break;

			default:
				Trace(TR_ERR, "Invalid event %d", event);
				break;
			}
		}
		freeBuffer(bf);
		if (removes > 0) {
			ArchiveRun();
		}
	}

	Trace(TR_ARDEBUG, "FsExamine() %d files examined", examined);
}
