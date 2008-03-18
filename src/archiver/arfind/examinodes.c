/*
 * examinodes.c - Examine inodes.
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

#pragma ident "$Revision: 1.11 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* Feature test switches. */
/* EXAM_TRACE	If defined, turn on DEBUG traces for module */
/* FILE_TRACE   If defined, turn on DEBUG traces for files processed */

#if defined(DEBUG)
#define	EXAM_TRACE
#define	FILE_TRACE
#endif

/* ANSI C headers. */
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

/* POSIX headers. */
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <sys/vfs.h>

/* SAM-FS headers. */
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/uioctl.h"
#include "sam/fs/ino.h"
#include "sam/fs/sblk.h"

/* Local headers. */
#define	NEED_ARFIND_EVENT_NAMES
#include "arfind.h"
#undef NEED_ARFIND_EVENT_NAMES
#include "dir_inode.h"

/* Macros. */
#define	EXAMLIST_INCR 1000000
#define	EXAMLIST_START 10000

/* Private data. */

static struct ExamList *examList;

/*
 * idList -  List of the location of all inode ids in the examList.
 * Each entry in the examList is referenced by an entry in the idList.
 * An idList entry is the offset in the examList of a sam_id_t struct.
 *
 * The idList section is sorted by inode number.  It is searched using
 * a binary search.
 */

static struct IdList {
	MappedFile_t Il;
	size_t	IlSize;		/* Size of all entries */
	int	IlCount;	/* Number of entries */
	uint_t IlEntry[1];
} *idList;

#define	IDLIST "idlist_exam"
#define	IDLIST_MAGIC 0110414112324
#define	IDLIST_INCR 1000
#define	ID_LOC(ir) ((void *)((char *)examList + *ir))

static pthread_cond_t examWait = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t examWaitMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t examListMutex = PTHREAD_MUTEX_INITIALIZER;

static char ts[ISO_STR_FROM_TIME_BUF_SIZE];
static time_t deferTime;	/* Time to defer examinations */

/* Private functions. */
static uint_t *idListAdd(sam_id_t id);
static void idListCreate(void);
static uint_t *idListLookup(sam_id_t id);
static void initExaminodes();
static struct ExamList *initExamList(char *name);
static void addExamList(sam_id_t id, int event, sam_time_t xeTime,
    char *caller);
static void pruneExamList(void);
static void wakeup(void);


/*
 * Thread - Examine inodes list.
 */
void *
ExamInodes(
	void *arg)
{
	static struct sam_perm_inode pinode;
	static struct sam_disk_inode *dinode = (struct sam_disk_inode *)&pinode;
	static struct PathBuffer pathBuf, *pb = &pathBuf;
	struct ScanListEntry seAdd;

	initExaminodes();
	memset(&seAdd, 0, sizeof (seAdd));
	deferTime = time(NULL) + State->AfBackGndInterval;
	ThreadsInitWait(wakeup, wakeup);

	/*
	 * Process examlist entries as long as we are archiving.
	 */
	while (Exec < ES_term) {
		time_t	timeNow;
		boolean_t noActiveMsg = TRUE;
		boolean_t backgroundPaused;
		int	active;
		int	elFree;
		int	i;

		timeNow = time(NULL);
		PthreadMutexLock(&examWaitMutex);
		ThreadsCondTimedWait(&examWait, &examWaitMutex,
		    timeNow + EPSILON_TIME);
		PthreadMutexUnlock(&examWaitMutex);
		if (Exec != ES_run || FsFd == -1) {
			continue;
		}
		ThreadsReconfigSync(RC_wait);
		timeNow = time(NULL);
		deferTime = timeNow + State->AfBackGndInterval;

#if defined(EXAM_TRACE)
		if (examList->ElCount != examList->ElFree) {
			Trace(TR_DEBUG, "Examlist search start "
			    "count: %d size: %d free: %d",
			    examList->ElCount, examList->ElSize,
			    examList->ElFree);
		}
#endif /* defined(EXAM_TRACE) */

		/*
		 * Lock the list while finding entries to examine.
		 */
		PthreadMutexLock(&examListMutex);
		active = 0;
		elFree = 0;
		backgroundPaused = FALSE;

		for (i = 0; i < examList->ElCount; i++) {
			struct ExamListEntry *xe;
			sam_id_t id;
			int	event;
			int	flags;

			xe = &examList->ElEntry[i];
			if (xe->XeFlags & XE_free) {
				elFree++;
				continue;
			}
			if (xe->XeTime > timeNow + EPSILON_TIME) {
				active++;
				continue;
			}

			id = xe->XeId;
			event = xe->XeEvent;
			flags = xe->XeFlags;
			xe->XeFlags |= XE_free;
			xe->XeTime = TIME_MAX;
			PthreadMutexUnlock(&examListMutex);

			/*
			 * Pause a background scan.
			 */
			if (backgroundPaused == FALSE) {
				backgroundPaused = TRUE;
				ScanInodesPauseScan(TRUE);
			}

			/*
			 * Get a copy of the inode.
			 */
			if (event != AE_remove && GetPinode(id, &pinode) != 0) {
#if defined(FILE_TRACE)
				Trace(TR_DEBUG, "Exam inode: %d.%d stat failed",
				    id.ino, id.gen);
#endif /* defined(FILE_TRACE) */
			} else {
#if defined(FILE_TRACE)
				Trace(TR_DEBUG,
				    "Exam inode: %d.%d event: '%s' flags: %04x",
				    id.ino, id.gen,
				    fileEventNames[event], flags);
#endif /* defined(FILE_TRACE) */

				EXAM_MODE(dinode) = (ushort_t)event;
				CheckInode(pb, &pinode, &seAdd);

				if (S_ISDIR(dinode->mode)) {
					if (event == AE_rename) {
						/*
						 * A renamed directory.
						 * Enter a scan for it in case
						 * it fits new file properties.
						 */
						seAdd.SeId = (dinode->id);
						seAdd.SeCopies = 0;
						seAdd.SeAsn = 0;
						seAdd.SeTime = TIME_NOW(dinode);
						ScanfsAddEntry(&seAdd);
					} else if (event == AE_archive &&
					    pb->PbPath != pb->PbEnd) {
						/*
						 * Archive a directory.
						 * Start archiving for it.
						 */
						ArchiveStartDir(pb->PbPath);
					}
				}
			}
			PthreadMutexLock(&examListMutex);
			noActiveMsg = FALSE;
		}

		if (backgroundPaused == TRUE) {
			ScanInodesPauseScan(FALSE);
			backgroundPaused = FALSE;
		}

		examList->ElFree = elFree;
		if (examList->ElFree >= EXAMLIST_INCR) {
			pruneExamList();
		} else {
			PthreadMutexUnlock(&examListMutex);
		}

		ThreadsReconfigSync(RC_allow);
		if (active == 0 && !noActiveMsg) {
			noActiveMsg = TRUE;
#if defined(EXAM_TRACE)
			Trace(TR_DEBUG, "Examlist no activity");
#endif /* defined(EXAM_TRACE) */
		}

		if (active != 0) {
			noActiveMsg = FALSE;
		}
	}

	Trace(TR_ARDEBUG, "ExamInodes() exiting");
	return (arg);
}


/*
 * Add entry to examlist.
 */
void
ExamInodesAddEntry(
	sam_id_t id,
	int event,
	sam_time_t xeTime,
	char *caller)
{
	PthreadMutexLock(&examListMutex);
	addExamList(id, event, xeTime, caller);
	PthreadMutexUnlock(&examListMutex);
}


/*
 * Remove an inode from the examine list.
 */
void
ExamInodesRmInode(
	sam_id_t id)
{
	uint_t	*ir;

	PthreadMutexLock(&examListMutex);
	ir = idListLookup(id);
	if (ir != NULL && *ir != 0) {
		struct ExamListEntry *xe;

		xe = (struct ExamListEntry *)ID_LOC(ir);
		xe->XeFlags |= XE_free;
		xe->XeTime = TIME_MAX;
#if defined(FILE_TRACE)
		Trace(TR_DEBUG, "Examlist remove inode: %d.%d event: '%s'",
		    xe->XeId.ino, xe->XeId.gen,
		    fileEventNames[xe->XeEvent]);
#endif /* defined(FILE_TRACE) */
	}
	PthreadMutexUnlock(&examListMutex);
}


/*
 * Trace the examine list.
 */
void
ExamInodesTrace(void)
{
	FILE    *st;

	if ((st = TraceOpen()) == NULL) {
		return;
	}
	PthreadMutexLock(&examListMutex);
	fprintf(st, "Examlist debug count: %d size: %d free: %d\n",
	    examList->ElCount, examList->ElSize, examList->ElFree);

	if (examList->ElCount != 0) {
		int	i;

		for (i = 0; i < examList->ElCount; i++) {
			struct ExamListEntry *xe;

			xe = &examList->ElEntry[i];
			if (!(xe->XeFlags & XE_free)) {
				fprintf(st, " %3d %04x %d.%d %s\n",
				    i, xe->XeFlags,
				    xe->XeId.ino, xe->XeId.gen,
				    TimeToIsoStr(xe->XeTime, ts));
#if defined(EXAM_TRACE)
			} else {
				fprintf(st, " %3d %04x %d.%d %3d\n",
				    i, xe->XeFlags,
				    xe->XeId.ino, xe->XeId.gen,
				    (int)xe->XeTime);
#endif /* defined(EXAM_TRACE) */
			}
		}
	}
	PthreadMutexUnlock(&examListMutex);
	fprintf(st, "\n");
	TraceClose(-1);
}


/* Private functions. */


/*
 * Add an entry to the idList.
 */
static uint_t *
idListAdd(
	sam_id_t id)
{
	uint_t *ir, *irEnd;
	int	i;
	int	new;
	int	l, u;

	/*
	 * Binary search list.
	 */
	l = 0;
	u = idList->IlCount;
	new = 0;
	while (u > l) {
		sam_ino_t ino;
		sam_id_t *ip;

		i = (l + u) >> 1;	/* Find midpoint */
		ir = &idList->IlEntry[i];
		ip = ID_LOC(ir);
		ino = ip->ino;
		if (id.ino == ino) {
			return (ir);
		}
		if (id.ino < ino) {
			u = i;
			new = u;
		} else {
			/* id.ino > ino */
			l = i + 1;
			new = l;
		}
	}

	/*
	 * Need a new entry.
	 */
	while (idList->IlSize + sizeof (uint_t) > idList->Il.MfLen) {
		idList = MapFileGrow(idList, IDLIST_INCR * sizeof (uint_t));
		if (idList == NULL) {
			LibFatal(MapFileGrow, IDLIST);
		}
#if defined(EXAM_TRACE)
		Trace(TR_DEBUG, "Grow %s: %d", IDLIST, idList->IlCount);
#endif /* defined(EXAM_TRACE) */
	}

	/*
	 * Set location where entry belongs in sorted order.
	 * Move remainder of list up.
	 */
	ir = &idList->IlEntry[new];
	irEnd = &idList->IlEntry[idList->IlCount];
	if (Ptrdiff(irEnd, ir) > 0) {
		memmove(ir+1, ir, Ptrdiff(irEnd, ir));
	}
	idList->IlCount++;
	idList->IlSize += sizeof (uint_t);
	*ir = 0;
	return (ir);
}


/*
 * Create the empty idList.
 */
static void
idListCreate(void)
{
	idList = MapFileCreate(IDLIST, IDLIST_MAGIC,
	    IDLIST_INCR * sizeof (uint_t));
	idList->Il.MfValid = 1;
	idList->IlCount = 0;
	idList->IlSize = sizeof (struct IdList);
}


/*
 * Lookup file in the idList.
 */
static uint_t *
idListLookup(
	sam_id_t id)
{
	uint_t	*ir;
	int	i;
	int	l, u;

	/*
	 * Binary search list.
	 */
	l = 0;
	u = idList->IlCount;
	while (u > l) {
		sam_ino_t ino;
		sam_id_t *ip;

		i = (l + u) >> 1;	/* Find midpoint */
		ir = &idList->IlEntry[i];
		ip = ID_LOC(ir);
		ino = ip->ino;
		if (id.ino == ino) {
			return (ir);
		}
		if (id.ino < ino) {
			u = i;
		} else {
			/* id.ino > ino */
			l = i + 1;
		}
	}
	return (NULL);
}


/*
 * Initialize module.
 */
static void
initExaminodes(void)
{
	if (!Recover || State->AfExamine < EM_noscan) {
		examList = NULL;
	} else {
		examList = ArMapFileAttach(EXAMLIST, EXAMLIST_MAGIC, O_RDWR);
		if (examList != NULL) {
#if defined(EXAM_TRACE)
			Trace(TR_DEBUG, "Found examlist");
#endif /* defined(EXAM_TRACE) */
			if (examList->ElVersion != EXAMLIST_VERSION) {
				Trace(TR_DEBUG,
				    "examlist version mismatch. Is: %d,"
				    " should be: %d",
				    examList->ElVersion, EXAMLIST_VERSION);
				(void) ArMapFileDetach(examList);
				examList = NULL;
			} else {
				int	i;

				idListCreate();
				for (i = 0; i < examList->ElCount; i++) {
					struct ExamListEntry *xe;
					uint_t	*ir;

					xe = &examList->ElEntry[i];
					ir = idListAdd(xe->XeId);
					*ir = Ptrdiff(xe, examList);
				}
			}
		}
	}
	if (examList == NULL) {
		examList = initExamList(EXAMLIST);
	}
}


/*
 * Initialize an empty ExamList.
 */
static struct ExamList *
initExamList(
	char *name)
{
	struct ExamList *el;

	idListCreate();
	el = MapFileCreate(name, EXAMLIST_MAGIC,
	    EXAMLIST_START * sizeof (struct ExamListEntry));
	if (el == NULL) {
		LibFatal(create, name);
	}
	el->ElCount = 0;
	el->ElSize = sizeof (struct ExamList) - sizeof (struct ExamListEntry);
	el->ElVersion = EXAMLIST_VERSION;
	el->ElFree = 0;
	el->El.MfValid  = 1;
#if defined(EXAM_TRACE)
	Trace(TR_DEBUG, "Create ExamList: %s %u", name, el->El.MfLen);
#endif /* defined(EXAM_TRACE) */
	return (el);
}


/*
 * Add entry to the examine list.
 * Exam list mutex locked on entry by caller.
 */
static void
addExamList(
	sam_id_t id,
	int event,
	sam_time_t xeTime,
	char *caller)
{
	uint_t	*ir;
	struct ExamListEntry *xe;

	if (xeTime >= deferTime) {
#if defined(FILE_TRACE)
		Trace(TR_DEBUG,
		    "Examlist defer inode: %d.%d event: '%s' caller: '%s'",
		    id.ino, id.gen, fileEventNames[event], caller);
#endif /* defined(FILE_TRACE) */
		return;
	}

	ir = idListAdd(id);
	if (*ir != 0) {
		xe = (struct ExamListEntry *)ID_LOC(ir);

		/*
		 * Examlist entry found.
		 * Set the earliest time.
		 */
		xe->XeEvent = (ushort_t)event;
		xe->XeFlags &= ~XE_free;
		if (xe->XeId.gen == id.gen) {
			xe->XeTime = min(xe->XeTime, xeTime);

			Trace(TR_DEBUG, "Examlist update inode: %d.%d "
			    "event: '%s' caller: '%s'",
			    xe->XeId.ino, xe->XeId.gen,
			    fileEventNames[event], caller);

		} else {
#if defined(FILE_TRACE)
			Trace(TR_DEBUG, "Examlist changed inode: %d.%d "
			    "event: '%s' caller: '%s' old gen: %d",
			    id.ino, id.gen,
			    fileEventNames[event], caller, xe->XeId.gen);
#endif /* defined(FILE_TRACE) */
			if (id.gen > xe->XeId.gen) {
				xe->XeId.gen = id.gen;
				xe->XeTime = xeTime;
			}
		}
	} else {

		/*
		 * Need a new entry.
		 */
		while (examList->ElSize + sizeof (struct ExamListEntry) >
		    examList->El.MfLen) {
			examList = MapFileGrow(examList,
			    EXAMLIST_INCR * sizeof (struct ExamListEntry));
			if (examList == NULL) {
				LibFatal(MapFileGrow, EXAMLIST);
			}
#if defined(EXAM_TRACE)
			Trace(TR_DEBUG, "Examlist grow count: %d len: %d",
			    examList->ElCount, examList->El.MfLen);
#endif /* defined(EXAM_TRACE) */
		}
		xe = &examList->ElEntry[examList->ElCount];
		examList->ElCount++;
		examList->ElSize += sizeof (struct ExamListEntry);
#if defined(FILE_TRACE)
		Trace(TR_DEBUG,
		    "Examlist new inode: %d.%d event: '%s' caller: '%s'",
		    id.ino, id.gen, fileEventNames[event], caller);
#endif /* defined(FILE_TRACE) */
		xe->XeId = id;
		xe->XeTime = xeTime;
		xe->XeEvent = (ushort_t)event;
		xe->XeFlags = 0;
		*ir = Ptrdiff(xe, examList);
	}
}


/*
 * Prune the examine list.
 * examListMutex locked on entry.  Unlocked on exit.
 */
static void
pruneExamList(void)
{
	struct ExamList *el, *elTmp;
	struct ExamListEntry *xe;
	int	i;

	if (examList->ElCount == 0 || examList->ElFree == 0) {
		PthreadMutexUnlock(&examListMutex);
		return;
	}

	/*
	 * Create a new examlist.
	 */
	elTmp = initExamList("ExamTmp");
	if (elTmp == NULL) {
		LibFatal(create, "ExamTmp");
	}

	el = examList;
	examList = elTmp;
	if (MapFileRename(examList, EXAMLIST) == -1) {
		LibFatal(MapFileRename, EXAMLIST);
	}

	Trace(TR_MISC, "Examlist prune count: %d free: %d",
	    el->ElCount, el->ElFree);

	/*
	 * Add the non-free entries to the new list.
	 */
	for (i = 0; i < el->ElCount; i++) {
		xe = &el->ElEntry[i];
		if (!(xe->XeFlags & XE_free)) {
			addExamList(xe->XeId, xe->XeTime, xe->XeEvent,
			    "pruneExamList");
		}
	}
	(void) ArMapFileDetach(el);
	PthreadMutexUnlock(&examListMutex);
}


/*
 * Wakeup the ExamInodes() thread.
 */
static void
wakeup(void)
{
	PthreadMutexLock(&examWaitMutex);
	PthreadCondSignal(&examWait);
	PthreadMutexUnlock(&examWaitMutex);
}
