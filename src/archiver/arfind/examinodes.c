/*
 * examinodes.c - Examine inodes.
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
 * or https://illumos.org/license/CDDL.
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

#pragma ident "$Revision: 1.18 $"

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
#include <stdlib.h>

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
#define EXAMLIST_INCR   16384   /* Slots per file grow */
#define EXAMLIST_START  16384
#define IX_NONE         UINT_MAX
#define IX_BATCH        32     /* max Entries taken per lock round */
#define IX_HASH_START   16384   /* Power of 2 */

/* Private data. */
static struct ExamList *examList;

#define	IDLIST "idlist_exam"
#define	IDLIST_TMP "idlist_tmp"
/* #define	IDLIST_MAGIC 0110414112324 */
#define	IDLIST_MAGIC 0414112324
#define	IDLIST_INCR 1000
#define	ID_LOC(ir) ((void *)((char *)examList + *(ir)))

static pthread_cond_t examWait = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t examListMutex = PTHREAD_MUTEX_INITIALIZER;

static char ts[ISO_STR_FROM_TIME_BUF_SIZE];
static time_t deferTime;	/* Time to defer examinations */

/* private functions */
static void initExaminodes();
static struct ExamList *initExamList(char *name);
static void addExamList(sam_id_t id, int event, sam_time_t xeTime,
    char *caller);
static void wakeup(void);

/*
 * Volatile index over the persistent examList slots (rebuilt on start).
 *   heap[]     min-heap of slot numbers ordered by xeLess()
 *   hpos[]     slot -> position in heap, IX_NONE if slot not in heap
 *   freeSlot[] stack of reusable slots
 *   hash[]     open addressing, ino -> slot + 1 (0 = empty)
 */
static struct ExamIdx {
        uint_t  *heap;
        uint_t  hcount;
        uint_t  *hpos;
        uint_t  *freeSlot;
        uint_t  nfree;
        uint_t  cap;            /* capacity of heap/hpos/freeSlot */
        uint_t  *hash;
        uint_t  hsize;
        uint_t  hused;
        uint_t  hshift;         /* 32 - log2(hsize) */
} ix;

struct ExamBatch {
        sam_id_t id;
        int     event;
        int     flags;
};

/* Order: earliest time first, ino as tie-breaker. Add priority here later. */
static int
xeLess(
        uint_t a,
        uint_t b)
{
        struct ExamListEntry *x = &examList->ElEntry[a];
        struct ExamListEntry *y = &examList->ElEntry[b];

        if (x->XeTime != y->XeTime) {
                return (x->XeTime < y->XeTime);
        }
        return (x->XeId.ino < y->XeId.ino);
}

/* ---- Heap ---- */

static void
heapUp(
        uint_t k)
{
        uint_t slot = ix.heap[k];

        while (k > 0) {
                uint_t p = (k - 1) / 2;

                if (!xeLess(slot, ix.heap[p])) {
                        break;
                }
                ix.heap[k] = ix.heap[p];
                ix.hpos[ix.heap[k]] = k;
                k = p;
        }
        ix.heap[k] = slot;
        ix.hpos[slot] = k;
}

static void
heapDown(
        uint_t k)
{
        uint_t slot = ix.heap[k];

        for (;;) {
                uint_t c = 2 * k + 1;

                if (c >= ix.hcount) {
                        break;
                }
                if (c + 1 < ix.hcount &&
                    xeLess(ix.heap[c + 1], ix.heap[c])) {
                        c++;
                }
                if (!xeLess(ix.heap[c], slot)) {
                        break;
                }
                ix.heap[k] = ix.heap[c];
                ix.hpos[ix.heap[k]] = k;
                k = c;
        }
        ix.heap[k] = slot;
        ix.hpos[slot] = k;
}

/* Restore heap order after the key of slot changed. */
static void
heapFix(
        uint_t slot)
{
        heapUp(ix.hpos[slot]);
        heapDown(ix.hpos[slot]);
}

static void
heapInsert(
        uint_t slot)
{
        ix.heap[ix.hcount] = slot;
        ix.hpos[slot] = ix.hcount;
        ix.hcount++;
        heapUp(ix.hcount - 1);
}

static void
heapRemove(
        uint_t slot)
{
        uint_t k = ix.hpos[slot];
        uint_t last = ix.heap[--ix.hcount];

        ix.hpos[slot] = IX_NONE;
        if (last != slot) {
                ix.heap[k] = last;
                ix.hpos[last] = k;
                heapUp(k);
                heapDown(ix.hpos[last]);
        }
}

/* ---- Hash (ino -> slot), linear probing, backward-shift delete ---- */

static uint_t
hashHome(
        sam_ino_t ino)
{
        return (((uint_t)ino * 2654435761U) >> ix.hshift);
}

static void
hashPut(
        uint_t slot)
{
        uint_t mask = ix.hsize - 1;
        uint_t i = hashHome(examList->ElEntry[slot].XeId.ino);

        while (ix.hash[i] != 0) {
                i = (i + 1) & mask;
        }
        ix.hash[i] = slot + 1;
        ix.hused++;
}

static void
hashGrow(void)
{
        uint_t *old = ix.hash;
        uint_t oldSize = ix.hsize;
        uint_t bits = 0;
        uint_t i;

        ix.hsize = (oldSize == 0) ? IX_HASH_START : oldSize * 2;
        while ((1U << bits) < ix.hsize) {
                bits++;
        }
        ix.hshift = 32 - bits;
        SamMalloc(ix.hash, ix.hsize * sizeof (uint_t));
        memset(ix.hash, 0, ix.hsize * sizeof (uint_t));
        ix.hused = 0;
        for (i = 0; i < oldSize; i++) {
                if (old[i] != 0) {
                        hashPut(old[i] - 1);
                }
        }
        if (old != NULL) {
                SamFree(old);
        }
}

static void
hashInsert(
        uint_t slot)
{
        if ((ix.hused + 1) * 2 > ix.hsize) {    /* keep load <= 50% */
                hashGrow();
        }
        hashPut(slot);
}

static uint_t
hashFind(
        sam_ino_t ino)
{
        uint_t mask = ix.hsize - 1;
        uint_t i = hashHome(ino);

	if (ix.hash == NULL)
		return (IX_NONE);

        while (ix.hash[i] != 0) {
                uint_t slot = ix.hash[i] - 1;

                if (examList->ElEntry[slot].XeId.ino == ino) {
                        return (slot);
                }
                i = (i + 1) & mask;
        }
        return (IX_NONE);
}

static void
hashDelete(
        sam_ino_t ino)
{
        uint_t mask = ix.hsize - 1;
        uint_t i = hashHome(ino);
        uint_t j;

        while (ix.hash[i] != 0 &&
            examList->ElEntry[ix.hash[i] - 1].XeId.ino != ino) {
                i = (i + 1) & mask;
        }
        if (ix.hash[i] == 0) {
                return;                         /* not present */
        }
        /* Shift following cluster members back to fill the hole. */
        j = i;
        for (;;) {
                uint_t k;

                j = (j + 1) & mask;
                if (ix.hash[j] == 0) {
                        break;
                }
                k = hashHome(examList->ElEntry[ix.hash[j] - 1].XeId.ino);
                if (i <= j ? (i < k && k <= j) : (i < k || k <= j)) {
                        continue;               /* k lies in (i, j]: stays */
                }
                ix.hash[i] = ix.hash[j];
                i = j;
        }
        ix.hash[i] = 0;
        ix.hused--;
}

/* ---- Slots ---- */

/* Make heap/hpos/freeSlot hold at least n slots. */
static void
ixReserve(
        uint_t n)
{
        uint_t newcap, i;
        uint_t *p;

        if (n <= ix.cap) {
                return;
        }
        newcap = (ix.cap != 0) ? ix.cap : IX_HASH_START;
        while (newcap < n) {
                newcap *= 2;
        }
        if ((p = realloc(ix.heap, newcap * sizeof (uint_t))) == NULL) {
                LibFatal(realloc, "ExamIdx heap");
        }
        ix.heap = p;
        if ((p = realloc(ix.hpos, newcap * sizeof (uint_t))) == NULL) {
                LibFatal(realloc, "ExamIdx hpos");
        }
        ix.hpos = p;
        if ((p = realloc(ix.freeSlot, newcap * sizeof (uint_t))) == NULL) {
                LibFatal(realloc, "ExamIdx freeSlot");
        }
        ix.freeSlot = p;
        for (i = ix.cap; i < newcap; i++) {
                ix.hpos[i] = IX_NONE;
        }
        ix.cap = newcap;
}

/* Get a slot: reuse a free one or append to the mapped file. */
static uint_t
allocSlot(void)
{
        uint_t slot;

        if (ix.nfree > 0) {
                slot = ix.freeSlot[--ix.nfree];
                examList->ElFree--;
                return (slot);
        }
        while (examList->ElSize + sizeof (struct ExamListEntry) >
            examList->El.MfLen) {
                examList = MapFileGrow(examList,
                    EXAMLIST_INCR * sizeof (struct ExamListEntry));
                if (examList == NULL) {
                        LibFatal(MapFileGrow, EXAMLIST);
                }
#if defined(EXAM_TRACE)
                Trace(TR_MISC, "Examlist grow count: %d len: %u",
                    examList->ElCount, examList->El.MfLen);
#endif /* defined(EXAM_TRACE) */
        }
        ixReserve(examList->ElCount + 1);
        slot = examList->ElCount++;
        examList->ElSize += sizeof (struct ExamListEntry);
        ix.hpos[slot] = IX_NONE;
        return (slot);
}

/* Take a slot out of heap and hash and put it on the free stack. */
static void
removeSlot(
        uint_t slot)
{
        struct ExamListEntry *xe = &examList->ElEntry[slot];

        heapRemove(slot);
        hashDelete(xe->XeId.ino);
        xe->XeFlags |= XE_free;
        xe->XeTime = TIME32_MAX;
        ix.freeSlot[ix.nfree++] = slot;
        examList->ElFree++;
}

/*
 * Take up to max due entries (time <= limit) out of the list.
 * Called with examListMutex held. Returns number of entries.
 */
static int
examPopDue(
    time_t limit,
    struct ExamBatch *b,
    int max)
{
	int n = 0;

	while (n < max && ix.hcount > 0) {
		uint_t slot = ix.heap[0];
		struct ExamListEntry *xe = &examList->ElEntry[slot];

		if (xe->XeTime > limit) {
			break;
		}

#if defined(FILE_TRACE)
		Trace(TR_DEBUG, "examPopDue: ino %d event %s xetime %d limit %d diff %d",
		    xe->XeId.ino, fileEventNames[xe->XeEvent], xe->XeTime, limit, limit - xe->XeTime);
#endif
		b[n].id = xe->XeId;
		b[n].event = xe->XeEvent;
		b[n].flags = xe->XeFlags;
		removeSlot(slot);
		n++;
	}
	return (n);
}

/*
 * Build hash, heap and free stack from the slots of the (recovered or
 * freshly created) examList. Called once at startup.
 */
static void
ixBuild(void)
{
	uint_t i;

	if (ix.hash == NULL) {
		hashGrow();
	}
	ixReserve(examList->ElCount + 1);
	ix.hcount = 0;
	ix.nfree = 0;

	for (i = 0; i < examList->ElCount; i++) {
		struct ExamListEntry *xe = &examList->ElEntry[i];
		uint_t s;

		ix.hpos[i] = IX_NONE;
		if (xe->XeFlags & XE_free) {
			ix.freeSlot[ix.nfree++] = i;
			continue;
		}
		s = hashFind(xe->XeId.ino);
		if (s != IX_NONE) {
			/* Duplicate ino in file: keep one, earliest time. */
			struct ExamListEntry *d = &examList->ElEntry[s];

			if (xe->XeTime < d->XeTime) {
				d->XeTime = xe->XeTime;
				heapFix(s);
			}
			xe->XeFlags |= XE_free;
			xe->XeTime = TIME32_MAX;
			ix.freeSlot[ix.nfree++] = i;
			continue;
		}
		hashInsert(i);
		heapInsert(i);
	}
	examList->ElFree = ix.nfree;
}

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

	static struct ExamBatch batch[IX_BATCH];

	initExaminodes();
	memset(&seAdd, 0, sizeof (seAdd));
	PthreadMutexLock(&examListMutex);
	deferTime = time(NULL) + State->AfBackGndInterval;
	PthreadMutexUnlock(&examListMutex);
	ThreadsInitWait(wakeup, wakeup);

	/*
	 * Process examlist entries as long as we are archiving.
	 */
	while (Exec < ES_term) {
		time_t  timeNow;
		int     nb, b;

		PthreadMutexLock(&examListMutex);
		timeNow = time(NULL);
		if (Exec != ES_run || FsFd == -1 || ix.hcount == 0 ||
		    examList->ElEntry[ix.heap[0]].XeTime > timeNow) {
			time_t until = timeNow + EPSILON_TIME;

			if (Exec == ES_run && ix.hcount != 0 &&
			    examList->ElEntry[ix.heap[0]].XeTime < until) {
				until = examList->ElEntry[ix.heap[0]].XeTime;
			}
#if defined(FILE_TRACE)
			Trace(TR_DEBUG, "ExamInodes: wait until %d %s",
			    until, TimeToIsoStr(until, NULL));
#endif /* defined(FILE_TRACE) */
			ThreadsCondTimedWait(&examWait, &examListMutex, until);
			PthreadMutexUnlock(&examListMutex);
			continue;
		}
#if defined(FILE_TRACE)
		Trace(TR_DEBUG, "ExamInodes: ix.hcount %d, entry time %d, timeNow+ %d, delta %d",
		    ix.hcount, examList->ElEntry[ix.heap[0]].XeTime, timeNow + EPSILON_TIME, (timeNow + EPSILON_TIME)-examList->ElEntry[ix.heap[0]].XeTime);
#endif /* defined(FILE_TRACE) */
		PthreadMutexUnlock(&examListMutex);

		ThreadsReconfigSync(RC_wait);

		/*
		 * Lock the list while finding entries to examine.
		 */
		PthreadMutexLock(&examListMutex);
		timeNow = time(NULL);
		deferTime = timeNow + State->AfBackGndInterval;
		nb = examPopDue(timeNow, batch, IX_BATCH);
                PthreadMutexUnlock(&examListMutex);

		/*
		 * Pause a background scan.
		 */
                if (nb > 0) {
                        ScanInodesPauseScan(TRUE);
                }
#if defined(FILE_TRACE)
		Trace(TR_DEBUG, "ExamInodes: process %d events, ix.hcount %d, defer time %d", nb, ix.hcount, deferTime);
#endif /* defined(FILE_TRACE) */
                for (b = 0; b < nb && Exec < ES_term; b++) {
                        sam_id_t id = batch[b].id;
                        int event = batch[b].event;
                        int flags = batch[b].flags;

                        if (event != AE_remove && GetPinode(id, &pinode) != 0) {
                                /* stat failed: trace as before */
#if defined(FILE_TRACE)
				Trace(TR_DEBUG, "ExamInodes: %d.%d stat failed",
				    id.ino, id.gen);
#endif /* defined(FILE_TRACE) */
                                continue;
                        }
                        /* archdone check, EXAM_MODE, CheckInode,
                           directory handling: unchanged, a plain
                           'continue' replaces the lock/unlock juggling */
#if defined(FILE_TRACE)
			Trace(TR_DEBUG,
			    "ExamInodes: %d.%d event: '%s' flags: %04x",
			    id.ino, id.gen,
			    fileEventNames[event], flags);
#endif /* defined(FILE_TRACE) */

			/* Done if archdone already set. */
			if (dinode->status.b.archdone) {
				if (event != AE_rename &&
				    event != AE_archive) {
					continue;
				}
			}

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
					seAdd.SeFlags = SE_subdir;
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
                if (nb > 0) {
                        ScanInodesPauseScan(FALSE);
                }
		ThreadsReconfigSync(RC_allow);
	}

	Trace(TR_ARDEBUG, "ExamInodes: exiting");
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
        uint_t  slot;

        PthreadMutexLock(&examListMutex);
        slot = hashFind(id.ino);
        if (slot != IX_NONE) {
#if defined(FILE_TRACE)
                struct ExamListEntry *xe = &examList->ElEntry[slot];

                Trace(TR_DEBUG, "Examlist remove inode: %d.%d event: '%s'",
                    xe->XeId.ino, xe->XeId.gen, fileEventNames[xe->XeEvent]);
#endif /* defined(FILE_TRACE) */
                removeSlot(slot);
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
	TraceClose(INT_MAX);
}


/* Private functions. */

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
			Trace(TR_MISC, "Found examlist");
#endif /* defined(EXAM_TRACE) */
			if (examList->ElVersion != EXAMLIST_VERSION) {
				Trace(TR_DEBUG,
				    "examlist version mismatch. Is: %d,"
				    " should be: %d",
				    examList->ElVersion, EXAMLIST_VERSION);
				(void) ArMapFileDetach(examList);
				examList = NULL;
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
	Trace(TR_MISC, "Create ExamList: %s %u", name, el->El.MfLen);
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
        uint_t  slot;
        struct ExamListEntry *xe;

        if (xeTime >= deferTime) {
#if defined(FILE_TRACE)
                Trace(TR_DEBUG,
                    "Examlist defer inode: %d.%d event: '%s' caller: '%s'",
                    id.ino, id.gen, fileEventNames[event], caller);
#endif /* defined(FILE_TRACE) */
                return;
        }

        slot = hashFind(id.ino);
        if (slot != IX_NONE) {
                xe = &examList->ElEntry[slot];
                if (id.gen < xe->XeId.gen) {
                        return;         /* stale generation: ignore */
                }
                xe->XeEvent = (ushort_t)event;  /* merge rule? see notes */
                if (id.gen > xe->XeId.gen) {
                        xe->XeId.gen = id.gen;
                        xe->XeTime = xeTime;
                        heapFix(slot);
#if defined(FILE_TRACE)
			Trace(TR_DEBUG, "addExamlist: update inode: %d.%d "
			    "event: '%s' caller: '%s' gen: %d time: %s",
			    id.ino, id.gen, fileEventNames[event], caller, id.gen, TimeToIsoStr(xeTime, NULL));
#endif /* defined(FILE_TRACE) */
                } else if (xeTime < xe->XeTime) {
                        xe->XeTime = xeTime;
                        heapUp(ix.hpos[slot]);
#if defined(FILE_TRACE)
			Trace(TR_DEBUG, "addExamlist: update inode: %d.%d "
			    "event: '%s' caller: '%s' time: %s",
			    id.ino, id.gen, fileEventNames[event], caller, TimeToIsoStr(xeTime, NULL));
#endif /* defined(FILE_TRACE) */
                }
        } else {
                slot = allocSlot();
                xe = &examList->ElEntry[slot];
                xe->XeId = id;
                xe->XeTime = xeTime;
                xe->XeEvent = (ushort_t)event;
                xe->XeFlags = 0;
                hashInsert(slot);
                heapInsert(slot);
#if defined(FILE_TRACE)
		Trace(TR_DEBUG,
		    "addExamlist: new inode: %d.%d event: '%s' caller: '%s' time: %s",
		    id.ino, id.gen, fileEventNames[event], caller, TimeToIsoStr(xeTime, NULL));
#endif /* defined(FILE_TRACE) */
        }

        /* New head of the heap: the consumer may have to wake earlier. */
        if (ix.heap[0] == slot) {
                PthreadCondSignal(&examWait);
        }
}

static void
wakeup(void)
{
        PthreadMutexLock(&examListMutex);
        PthreadCondSignal(&examWait);
        PthreadMutexUnlock(&examListMutex);
}
