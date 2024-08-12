/*
 * scanfs.c - Scan the file system.
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

#pragma ident "$Revision: 1.107 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* Feature test switches. */
#if defined(DEBUG)
#define	SCAN_TRACE   /* If defined, turn on DEBUG traces for scanning */
#endif

/* ANSI C headers. */
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

/* POSIX headers. */
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

/* SAM-FS headers. */
#include "sam/mount.h"
#include "sam/lib.h"

/* Local headers. */
#include "arfind.h"
#include "dir_inode.h"

/* Macros. */
#define	SCANLIST_INCR 2000
#define	SCANLIST_START 2000

/* Private data. */

static struct ScanList *scanList;	/* Scan list */

/*
 * idList -  List of the location of all inode ids in the scanList.
 * Each entry in the scanList is referenced by an entry in the idList.
 * An idList entry is the offset in the scanList of a sam_id_t struct.
 *
 * The idList section is sorted by inode number.  It is searched using a binary
 * search.
 */

static struct IdList {
	MappedFile_t Il;
	size_t	IlSize;		/* Size of all entries */
	int	IlCount;	/* Number of entries */
	uint_t IlEntry[1];
} *idList;

#define	IDLIST "idList_scan"
#define	IDLIST_MAGIC 0414112324
#define	IDLIST_INCR 1000
#undef	ID_LOC
#define	ID_LOC(ir) ((void *)((char *)scanList + *ir))

static pthread_cond_t scanWait = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t scanWaitMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t scanListMutex = PTHREAD_MUTEX_INITIALIZER;

static struct ScanListEntry fullScan;	/* Full scan */
static struct ScanListEntry seScan;	/* The scan being performed */

static struct PathBuffer pathBuf, *pb = &pathBuf;
static time_t timeNow;
static char ts[ISO_STR_FROM_TIME_BUF_SIZE];

/* Private functions. */
static time_t backGndScanTime(void);
static struct ScanListEntry *findScan(void);
static uint_t *idListAdd(sam_id_t id);
static void idListCreate(void);
static uint_t *idListLookup(sam_id_t id);
static void initScanfs(void);
static void wakeup(void);


/*
 * Scan file system.
 */
void *
Scanfs(
	void *arg)
{
	initScanfs();
	ThreadsInitWait(wakeup, wakeup);

	timeNow = time(NULL);
	if (!Recover && State->AfExamine >= EM_noscan) {
		/*
		 * We are not recovering work.
		 * Start a full directory scan.
		 */
		fullScan.SeTime = timeNow;
		ScanfsAddEntry(&fullScan);
	}

	/*
	 * Process scanlist entries as long as we are archiving.
	 */
	PthreadMutexLock(&scanListMutex);
	(void) findScan();
	PthreadMutexUnlock(&scanListMutex);
	while (Exec < ES_term) {
		struct ScanListEntry *se;
		time_t	waitTime;
		int numOfEntries;
		int freeEntries;

		timeNow = time(NULL);
		waitTime = seScan.SeTime;

		PthreadMutexLock(&scanListMutex);
		numOfEntries = scanList->SlCount;
		freeEntries = scanList->SlFree;
		PthreadMutexUnlock(&scanListMutex);

		if (Exec != ES_run || FsFd == -1) {

			/*
			 * Wakeup alarm to set proper message.
			 */
			(void) kill(getpid(), SIGALRM);
			waitTime = timeNow + LONG_TIME;
		} else if ((numOfEntries - freeEntries) > 1 ||
		    State->AfExamine < EM_noscan) {
			char	ts[ISO_STR_FROM_TIME_BUF_SIZE];
			char	*path;

			if (seScan.SeFlags & SE_inodes) {
				path = "inodes";
			} else {
				IdToPathId(seScan.SeId, pb);
				path = ScanPathToMsg(pb->PbPath);
			}
			(void) TimeToIsoStr(seScan.SeTime, ts);
#if defined(SCAN_TRACE)
			Trace(TR_DEBUG, "Waiting until %s to scan %s",
			    ts, path);
#endif /* defined(SCAN_TRACE) */
			/* Waiting until %s to scan %s */
			PostOprMsg(4356, ts, path);
		} else {
			/* Monitoring file system activity. */
			PostOprMsg(4363);
		}
		PthreadMutexLock(&scanWaitMutex);
		ThreadsCondTimedWait(&scanWait, &scanWaitMutex, waitTime);
		PthreadMutexUnlock(&scanWaitMutex);
		if (FsFd == -1) {
			continue;
		}

		/*
		 * Check the scan list again.
		 * There might be a newly scheduled scan.
		 */
		ThreadsReconfigSync(RC_wait);
		PthreadMutexLock(&scanListMutex);
		se = findScan();
		timeNow = time(NULL);
		if (seScan.SeTime <= timeNow &&
		    (Exec == ES_run || (seScan.SeFlags & SE_request))) {
			int	i;

			i = se - &scanList->SlEntry[0];
			se->SeFlags |= SE_scanning;
			/*
			 * Only the first entry is a permanently scheduled scan.
			 * Free the others.
			 */
			if (i > 0) {
				se->SeFlags |= SE_free;
				se->SeTime = TIME32_MAX;
			}

			PthreadMutexUnlock(&scanListMutex);

			/*
			 * Do the selected scan.
			 */
			if (seScan.SeFlags & SE_stats) {
				FsstatsStart();
			}
			if (seScan.SeFlags & SE_inodes) {
				ScanInodes(&seScan);
			} else {
				ScanDirs(&seScan);
			}
			if (seScan.SeFlags & SE_stats) {
				FsstatsEnd();
			}

			PthreadMutexLock(&scanListMutex);
			timeNow = time(NULL);
			se->SeFlags &= ~SE_scanning;

			/*
			 * Only the permanent scan (i.e. SlEntry[0])
			 * is rescheduled again.
			 */
			if (State->AfExamine < EM_noscan) {
				/*
				 * Scanning mode, full scan.
				 * Start archiving.
				 */
				ArchiveStartRequests();

				if (i == 0) {
					se = &scanList->SlEntry[0];
					se->SeTime = timeNow +
					    State->AfInterval;

					if (State->AfExamine == EM_scan) {
						/*
						 * All but the first scan
						 * is an inodes scan.
						 */
						se->SeId.ino = 0;
						se->SeFlags |= SE_inodes;
					}
				}
			} else {
				if (i == 0) {
					scanList->SlEntry[0].SeTime =
					    backGndScanTime();
				}
			}

			/*
			 * Get a new entry to process.
			 */
			se = findScan();
		}
		PthreadMutexUnlock(&scanListMutex);
		ThreadsReconfigSync(RC_allow);
	}

	Trace(TR_ARDEBUG, "Scanfs() exiting");
	ThreadsExit();
/* NOTREACHED */
	return (arg);
}


/*
 * Add entry to scanlist.
 */
void
ScanfsAddEntry(
	struct ScanListEntry *sep)
{
	struct ScanListEntry *se;
	uint_t *ir;

#if defined(SCAN_TRACE)
	Trace(TR_DEBUG, "ScanfsAddEntry(%d.%d, %s)",
	    sep->SeId.ino, sep->SeId.gen, TimeToIsoStr(sep->SeTime, ts));
#endif /* defined(SCAN_TRACE) */
	if (sep->SeId.ino == 0 && !(sep->SeFlags & SE_inodes)) {
		sep->SeId.ino = SAM_ROOT_INO;
		sep->SeId.gen = 2;
	}

	PthreadMutexLock(&scanListMutex);
	ir = idListAdd(sep->SeId);
	if (*ir != 0) {
		se = (struct ScanListEntry *)ID_LOC(ir);

		/*
		 * Scanlist entry found.
		 * Set the earliest time.
		 */
		se->SeFlags &= ~SE_free;
		se->SeCopies |= sep->SeCopies;
		se->SeFlags |= sep->SeFlags & SE_subdir;
		se->SeTime = min(se->SeTime, sep->SeTime);
	} else {

		/*
		 * Need a new entry.
		 */
		while (scanList->SlSize + sizeof (struct ScanListEntry) >
		    scanList->Sl.MfLen) {
			scanList = MapFileGrow(scanList,
			    SCANLIST_INCR * sizeof (struct ScanListEntry));
			if (scanList == NULL) {
				LibFatal(MapFileGrow, SCANLIST);
			}
#if defined(SCAN_TRACE)
			Trace(TR_DEBUG, "Grow %s: %d",
			    SCANLIST, scanList->SlCount);
#endif /* defined(SCAN_TRACE) */
		}
		se = &scanList->SlEntry[scanList->SlCount];
		scanList->SlCount++;
		scanList->SlSize += sizeof (struct ScanListEntry);
		State->AfScanlist[0] = scanList->SlCount;
		*se = *sep;
		*ir = Ptrdiff(se, scanList);
#if defined(SCAN_TRACE)
		Trace(TR_DEBUG, " New (%d.%d) %s", se->SeId.ino, se->SeId.gen,
		    TimeToIsoStr(se->SeTime, ts));
#endif /* defined(SCAN_TRACE) */
	}

	if (se->SeTime < seScan.SeTime) {
		wakeup();
	}
	PthreadMutexUnlock(&scanListMutex);
}


/*
 * Set full scan.
 */
void
ScanfsFullScan(void)
{
	if (State->AfExamine >= EM_noscan) {
		struct ScanListEntry *se;

		/*
		 * Start background scan.
		 */
		PthreadMutexLock(&scanListMutex);
		se = &scanList->SlEntry[0];
		if (!(se->SeFlags & SE_scanning)) {
			se->SeTime = time(NULL);
			Trace(TR_MISC,
			    "File system activity buffer overrun -"
			    " full scan begun.");
			wakeup();
		}
		PthreadMutexUnlock(&scanListMutex);
	}
}


/*
 * Reconfigure module.
 */
void
ScanfsReconfig(void)
{
	PthreadMutexLock(&scanListMutex);
	if (scanList != NULL && scanList->SlExamine == State->AfExamine) {
		if (ArchSetMap != NULL) {
			int	i;

			/*
			 * Update scanlist Archive Set references.
			 */
			for (i = 0; i < scanList->SlCount; i++) {
				struct ScanListEntry *se;
				int	asn;

				se = &scanList->SlEntry[i];
				asn = ArchSetMap[se->SeAsn];
				if (asn != -1) {
					se->SeAsn = asn;
				} else {
					se->SeAsn = 0;
					se->SeTime = time(NULL);
				}
			}
		}
		goto out;
	}

	idListCreate();
	if (!Recover || State->AfExamine < EM_noscan) {
		scanList = NULL;
	} else {
		scanList = ArMapFileAttach(SCANLIST, SCANLIST_MAGIC, O_RDWR);
		if (scanList != NULL) {
#if defined(SCAN_TRACE)
			Trace(TR_DEBUG, "Found scanlist");
#endif /* defined(SCAN_TRACE) */
			if (scanList->SlVersion != SCANLIST_VERSION) {
				Trace(TR_DEBUG,
				    "scanlist version mismatch."
				    " Is: %d, should be: %d",
				    scanList->SlVersion, SCANLIST_VERSION);
				(void) ArMapFileDetach(scanList);
				scanList = NULL;
			} else if (scanList->SlCount != 0) {
				int	i;

				/*
				 * Add entries to the idList.
				 */
				for (i = 1; i < scanList->SlCount; i++) {
					struct ScanListEntry *se;
					uint_t	*ir;

					se = &scanList->SlEntry[i];
					ir = idListAdd(se->SeId);
					*ir = Ptrdiff(se, scanList);
				}
			}
		}
	}
	if (scanList == NULL) {
		struct ScanListEntry *se;

		/*
		 * Initialize an empty ScanList.
		 */
		scanList = MapFileCreate(SCANLIST, SCANLIST_MAGIC,
		    SCANLIST_START * sizeof (struct ScanListEntry));
		if (scanList == NULL) {
			LibFatal(create, SCANLIST);
		}
		scanList->SlCount = 0;
		scanList->SlSize = sizeof (struct ScanList) -
		    sizeof (struct ScanListEntry);
		scanList->SlVersion = SCANLIST_VERSION;
		scanList->SlFree = 0;
		scanList->Sl.MfValid  = 1;
#if defined(SCAN_TRACE)
		Trace(TR_DEBUG, "Create ScanList: %u", scanList->Sl.MfLen);
#endif /* defined(SCAN_TRACE) */

		/*
		 * Add the first entry.
		 * If we're not using one of the scan methods, the first entry
		 * the background scan.
		 * Otherwise, the first entry is the selected scan.
		 */
		se = &scanList->SlEntry[0];
		memmove(se, &fullScan, sizeof (struct ScanListEntry));
		se->SeTime = time(NULL);
		if (State->AfExamine == EM_scan) {
			se->SeFlags |= SE_stats;
		} else if (State->AfExamine == EM_scandirs) {
			se->SeFlags |= SE_noarch | SE_stats;
		} else if (State->AfExamine == EM_scaninodes) {
			se->SeFlags |= SE_inodes | SE_stats;
		} else {
			se->SeFlags |= SE_inodes | SE_back;
		}
		scanList->SlCount++;
		scanList->SlSize += sizeof (struct ScanListEntry);
	}
	scanList->SlExamine = State->AfExamine;

out:
	State->AfScanlist[0] = scanList->SlCount;
	State->AfScanlist[1] = scanList->SlCount - scanList->SlFree;
	if (State->AfExamine >= EM_noscan) {
		scanList->SlEntry[0].SeTime = backGndScanTime();
	}
	PthreadMutexUnlock(&scanListMutex);
	wakeup();
}


/*
 * Remove and inode from the scan list.
 */
void
ScanfsRmInode(
	sam_id_t id)
{
	uint_t	*ir;

	PthreadMutexLock(&scanListMutex);
	ir = idListLookup(id);
	if (ir != NULL && *ir != 0) {
		struct ScanListEntry *se;

		se = (struct ScanListEntry *)ID_LOC(ir);
		if (!(se->SeFlags |= SE_free)) {
			se->SeFlags |= SE_free;
			scanList->SlFree++;
			State->AfScanlist[1] =
			    scanList->SlCount - scanList->SlFree;
#if defined(SCAN_TRACE)
			Trace(TR_DEBUG, "ScanfsRmInode(%d.%d)",
			    se->SeId.ino, se->SeId.gen);
#endif /* defined(SCAN_TRACE) */
		}
	}
	PthreadMutexUnlock(&scanListMutex);
}


/*
 * Start new scan.
 */
void
ScanfsStartScan(void)
{
	if (State->AfExamine < EM_noscan) {
		/*
		 * Change first entry to start now.
		 */
		PthreadMutexLock(&scanListMutex);
		scanList->SlEntry[0].SeTime = time(NULL);
		PthreadMutexUnlock(&scanListMutex);
	}
	wakeup();
}


/*
 * Trace the scanlist.
 */
void
ScanfsTrace(void)
{
	FILE	*st;

	st = TraceOpen();
	if (st == NULL) {
		return;
	}
	fprintf(st, "Scanfs - scanlist: count %d size %d free %d\n",
	    scanList->SlCount, scanList->SlSize, scanList->SlFree);
#if defined(SCAN_TRACE)
	PthreadMutexLock(&scanListMutex);
	if (scanList->SlCount != 0) {
		int	i;

		for (i = 0; i < scanList->SlCount; i++) {
			struct ScanListEntry *se;

			se = &scanList->SlEntry[i];
			fprintf(st, " %3d  ", i);
			if (se->SeFlags & SE_free) {
				fprintf(st, " %d.%d %3d", se->SeId.ino,
				    se->SeId.gen, (int)se->SeTime);
			} else {
				char	flags[] = "xxxx";
				ushort_t f;

				f = se->SeFlags;
				if (f & SE_scanning) {
					fprintf(st, "    %-19s", "Scanning");
				} else {
					fprintf(st, "%s",
					    TimeToIsoStr(se->SeTime, ts));
				}

				flags[0] = (f & SE_back) ? 'b' :
				    ((f & SE_request) ? 'r' : '-');
				flags[1] = (f & SE_full) ? 'f' : '-';
				flags[2] = (f & SE_noarch) ? 'n' : '-';
				flags[3] = (f & SE_free) ? 'f' : '-';
				fprintf(st, " %s", flags);

				flags[0] = (se->SeCopies & 0x1) ? '1' : '_';
				flags[1] = (se->SeCopies & 0x2) ? '2' : '_';
				flags[2] = (se->SeCopies & 0x4) ? '3' : '_';
				flags[3] = (se->SeCopies & 0x8) ? '4' : '_';
				fprintf(st, " %s", flags);

				fprintf(st, " %d", se->SeId.ino);
			}
			fprintf(st, "\n");
		}
	}
	PthreadMutexUnlock(&scanListMutex);
#endif /* defined(SCAN_TRACE) */
	fprintf(st, "\n");
	TraceClose(INT_MAX);
}


/* Private functions. */


/*
 * Find the next scan to be performed.
 */
static struct ScanListEntry *
findScan(void)
{
	struct ScanListEntry *nextScan;
	int	free;
	int	i;

#if defined(SCAN_TRACE)
	Trace(TR_DEBUG, "Find scan count: %d", scanList->SlCount);
#endif /* defined(SCAN_TRACE) */
	nextScan = &scanList->SlEntry[0];
	free = 0;
	for (i = 1; i < scanList->SlCount; i++) {
		struct ScanListEntry *se;

		se = &scanList->SlEntry[i];
		if (!(se->SeFlags & SE_free)) {
			se->SeFlags &= ~SE_scanning;
			if (se->SeTime < nextScan->SeTime) {
				nextScan = se;
			}
		} else {
			free++;
		}
	}
	memmove(&seScan, nextScan, sizeof (seScan));
	scanList->SlFree = free;
	State->AfScanlist[1] = scanList->SlCount - scanList->SlFree;
	return (nextScan);
}


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
#if defined(ARCH_TRACE)
		Trace(TR_DEBUG, "Grow %s: %d", IDLIST, idList->IlCount);
#endif /* defined(ARCH_TRACE) */
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
	idList =
	    MapFileCreate(IDLIST, IDLIST_MAGIC, IDLIST_INCR * sizeof (uint_t));
	idList->Il.MfValid = 1;
	idList->IlCount = 0;
	idList->IlSize = sizeof (struct IdList);
}


/*
 * Lookup file in idList.
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
initScanfs(void)
{
	pb->PbPath = pb->PbBuf;
	if (State->AfExamine != EM_scaninodes) {
		fullScan.SeId.ino = SAM_ROOT_INO;
		fullScan.SeId.gen = 2;
		fullScan.SeFlags = SE_back | SE_full | SE_noarch | SE_subdir;
	} else {
		fullScan.SeId.ino = 0;
		fullScan.SeFlags |= SE_back | SE_inodes | SE_stats;
	}
	scanList = NULL;
	ScanfsReconfig();
}


/*
 * Determine background scan time.
 */
static time_t
backGndScanTime(void)
{
	time_t	scanTime;

	timeNow = time(NULL);
	if (State->AfBackGndInterval % (24 * 60 * 60) != 0) {
		/*
		 * Not a multiple of a day.
		 */
		scanTime = timeNow + State->AfBackGndInterval;
	} else {
		struct tm tm, tms;
		time_t	lastScan;
		int	hour;
		int	min;

		hour = State->AfBackGndTime >> 8;
		min = State->AfBackGndTime & 0xff;
		(void) localtime_r(&timeNow, &tm);
		lastScan = scanList->SlEntry[0].SeTime;
		(void) localtime_r(&lastScan, &tms);
		if (tms.tm_hour == hour && tms.tm_min == min) {
			/*
			 * Last scan was at the requested time.
			 */
			scanTime = lastScan;
			if (lastScan < timeNow) {
				scanTime += State->AfBackGndInterval;
			}
		} else {
			/*
			 * Start the scan at the next requested time.
			 * Use start of present day as reference.
			 */
			lastScan = timeNow -
			    (tm.tm_hour * 3600) - (tm.tm_min * 60) - tm.tm_sec;
			scanTime = lastScan + hour * 3600 + min * 60;
			if (scanTime < timeNow) {
				scanTime += State->AfBackGndInterval;
			}
		}
	}
	return (scanTime);
}


/*
 * Wakeup the Scanfs() thread.
 */
static void
wakeup(void)
{
	PthreadMutexLock(&scanWaitMutex);
	PthreadCondSignal(&scanWait);
	PthreadMutexUnlock(&scanWaitMutex);
}
