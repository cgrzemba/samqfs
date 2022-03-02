/*
 * scaninodes.c - Scan inodes.
 *
 * Scan a filesystem by reading the .inodes file.
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

#pragma ident "$Revision: 1.56 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

/* SAM-FS headers. */
#include "sam/param.h"

/* Local headers. */
#include "arfind.h"
#include "dir_inode.h"

#if defined(DEBUG)
#define	FILE_TRACE
#define	SCAN_TRACE
#endif

/* Private data. */
static pthread_cond_t scanPause = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t scanPauseMutex = PTHREAD_MUTEX_INITIALIZER;
static boolean_t pauseScan = FALSE;

/* Private functions. */
static ino_t displayProgress(ino_t inodeNumber, struct sam_disk_inode *dinode);


/*
 * Scan .inodes file.
 */
void
ScanInodes(
	struct ScanListEntry *se)
{
	static union sam_di_ino *inodesBuffer = NULL;
	static size_t inodesBufferSize;
	struct ScanListEntry seAdd;
	ino_t	inodeNumber;
	ino_t	nextInodeNumber;
	offset_t offset;
	int	bytesReturned;

	if (inodesBuffer == NULL) {
		inodesBufferSize = INO_BLK_SIZE * INO_BLK_FACTOR;
		SamMalloc(inodesBuffer, inodesBufferSize);
	}
	inodeNumber = 0;
	offset = 0;
	if (llseek(FsFd, offset, SEEK_SET) < 0) {
		Trace(TR_ERR, "llseek(.inodes, %lld)", offset);
		return;
	}

#if defined(SCAN_TRACE)
	Trace(TR_DEBUG, "Scanning inodes %d", se->SeFlags & SE_back);
#endif /* defined(SCAN_TRACE) */
	nextInodeNumber = inodeNumber;
	memset(&seAdd, 0, sizeof (seAdd));
	seAdd.SeFlags = se->SeFlags & SE_request;

	/* WHILE-READING-INODES */
	while ((bytesReturned =
	    read(FsFd, inodesBuffer, inodesBufferSize)) > 0) {
		union sam_di_ino *inodeInBuffer;
		sam_time_t timeNow;

		timeNow = time(NULL);

		/*
		 * Step through the buffer.  Each active inode will match the
		 * increasing inode number and have a matching version number.
		 */
		inodeInBuffer = inodesBuffer;

		/* WHILE-CHECKING-BUFFER */
		while (bytesReturned > 0 && FsFd > 0) {
			static struct PathBuffer pathBuf, *pb = &pathBuf;
			struct sam_perm_inode *pinode;
			struct sam_disk_inode *dinode;

			dinode = &inodeInBuffer->inode.di;
			pinode = (struct sam_perm_inode *)dinode;
			inodeInBuffer++;
			inodeNumber++;
			bytesReturned -= sizeof (union sam_di_ino);

			/*
			 * Show scanning progress.
			 */
			if (inodeNumber >= nextInodeNumber) {
				nextInodeNumber =
				    displayProgress(inodeNumber, dinode);
			}

			/*
			 * Ignore non-file inodes.
			 */
			if (dinode->mode == 0 ||
			    S_ISEXT(dinode->mode) ||
			    dinode->id.ino != inodeNumber ||
			    !(SAM_CHECK_INODE_VERSION(dinode->version))) {
				continue;
			}
			if ((se->SeFlags & SE_stats) &&
			    (se->SeFlags & SE_request)) {
				FsstatsCountFile(pinode, "");
			}

			/*
			 * Active inode.
			 * If archdone is not set, check the file.
			 */
			if (!dinode->status.b.archdone) {
				EXAM_MODE(dinode) = EXAM_INODE;
				TIME_NOW(dinode) = timeNow;
				CheckInode(pb, pinode, &seAdd);
			}

			if (se->SeFlags & SE_stats && !SAM_PRIVILEGE_INO(
			    dinode->version, dinode->id.ino)) {
				/*
				 * Count the file.
				 */
#if defined(FILE_TRACE)
				if (dinode->status.b.archdone) {
					IdToPath(dinode, pb);
				}
#endif /* defined(FILE_TRACE) */
				FsstatsCountFile(pinode, pb->PbPath);
			}
		} /* WHILE-CHECKING-BUFFER */

		if (Exec != ES_run && !(se->SeFlags & SE_request)) {
#if defined(SCAN_TRACE)
		Trace(TR_DEBUG, "Interrupted");
#endif /* defined(SCAN_TRACE) */
			break;
		}

		PthreadMutexLock(&scanPauseMutex);
#if defined(SCAN_TRACE)
		if ((se->SeFlags & SE_back) && pauseScan) {
			Trace(TR_MISC, "Scan paused");
		}
#endif /* defined(SCAN_TRACE) */

		while ((se->SeFlags & SE_back) && pauseScan) {
			/* Inode scan paused for file system activity. */
			PostOprMsg(4365);

			ThreadsCondTimedWait(&scanPause, &scanPauseMutex,
			    time(NULL) + (4 * 60));

			ClearOprMsg();

#if defined(SCAN_TRACE)
			if ((se->SeFlags & SE_back) && pauseScan == FALSE) {
				Trace(TR_MISC, "Scan restarted");
			}
#endif /* defined(SCAN_TRACE) */
		}

		PthreadMutexUnlock(&scanPauseMutex);

	} /* WHILE-READING-INODES */

#if defined(SCAN_TRACE)
	Trace(TR_MISC, "Scan finished");
#endif /* defined(SCAN_TRACE) */

}


/*
 * Check an inode.
 */
void
CheckInode(
	struct PathBuffer *pb,
	struct sam_perm_inode *pinode,
	struct ScanListEntry *se)
{
	struct sam_disk_inode *dinode = (struct sam_disk_inode *)pinode;
	sam_id_t sId;			/* Segmented file id */

#if defined(FILE_TRACE)
	Trace(TR_DEBUG, "Check inode: %d.%d", dinode->id.ino, dinode->id.gen);
#endif /* defined(FILE_TRACE) */

	/*
	 * Check for priviledged inodes.
	 */
	if (SAM_PRIVILEGE_INO(dinode->version, dinode->id.ino)) {
		/*
		 * Archive only the root directory, and lost+found.
		 */
		if (dinode->id.ino != SAM_ROOT_INO &&
		    dinode->id.ino != SAM_LOSTFOUND_INO) {
			SetArchdone(".priv-inode", dinode);
			return;
		}
	}

	/*
	 * If the file is marked no archive or is damaged it can be skipped.
	 */
	if (dinode->status.b.noarch || dinode->status.b.damaged) {
		return;
	}

	if (!S_ISSEGS(dinode)) {
		IdToPath(dinode, pb);
	} else {
		static pthread_mutex_t enter = PTHREAD_MUTEX_INITIALIZER;
		static struct sam_perm_inode ipinode;
		static struct sam_disk_inode *idinode =
		    (struct sam_disk_inode *)&ipinode;

		/*
		 * Data segment inode.
		 * Get index segment inode to get file name and file size.
		 * The file size in the data segment is the size of the data
		 * segment.  So we put the file size in a place in the inode
		 * that is not needed for archiving (see arfind.h).
		 */
		PthreadMutexLock(&enter);
		if (GetPinode(dinode->parent_id, &ipinode) != 0) {
			PthreadMutexUnlock(&enter);
			Trace(TR_ERR, "stat(%d.%d)failed",
			    dinode->parent_id.ino, dinode->parent_id.gen);
			return;
		}
		IdToPath(idinode, pb);
		*SEGFILE_SIZE(dinode) = idinode->rm.size;
		sId = idinode->parent_id;
		PthreadMutexUnlock(&enter);
	}
	if (pb->PbPath == pb->PbEnd) {
		SetArchdone("*NO-PATH*", dinode);
		return;
	}
	if (*pb->PbPath == '.') {
		/*
		 * Files in priviledged directories.
		 */
		if (strncmp(pb->PbPath, ".archive/", 9) == 0 ||
		    strncmp(pb->PbPath, ".rft/", 5) == 0 ||
		    strncmp(pb->PbPath, ".stage/", 7) == 0) {
			SetArchdone(pb->PbPath, dinode);
			return;
		}
	}

	se->SeCopies = 0;
	se->SeTime = TIME32_MAX;
	CheckFile(pb, pinode, se);
	if (se->SeFlags & SE_stats) {
		FsstatsCountFile(pinode, pb->PbPath);
	}
	if (se->SeTime != TIME32_MAX) {
		if (!S_ISSEGS(dinode)) {
			se->SeId = dinode->parent_id;
		} else {
			se->SeId = sId;
		}
		ScanfsAddEntry(se);
	}
}


/*
 * Pause background scan.
 */
void
ScanInodesPauseScan(
	boolean_t pause)
{
	PthreadMutexLock(&scanPauseMutex);
	pauseScan = pause;
	PthreadCondSignal(&scanPause);
	PthreadMutexUnlock(&scanPauseMutex);
}


/* Private functions. */

/*
 * Progress message display.
 * Generate a bar graph in the operator message.
 * The bar graph shows progress in 5% increments.
 * RETURN: Next inode number of interest.
 */
static ino_t
displayProgress(
	ino_t inodeNumber,	/* Number of the inode read */
	struct sam_disk_inode *dinode)
{
	static int fivePct;
	static ino_t maxInodeNumber = 0;
	ino_t	nextInodeNumber;
	char	barGraph[23];	/* Room for 20 marks */
	int	barGraphIndex;

	/*
	 * Set maximum number of inodes and the next inode increment.
	 * Reset the message.
	 */
	if (inodeNumber == SAM_INO_INO) {
		int	inodeCount;

		inodeCount = dinode->rm.size / SAM_ISIZE;
		fivePct = inodeCount / 20;
		maxInodeNumber = 20 * fivePct;
	}
	if (maxInodeNumber == 0) {
		return ((ino_t)LONG_MAX);
	}
	barGraphIndex = 0;
	barGraph[barGraphIndex++] = '|';

	/*
	 * The number of inodes increased since the scan began.
	 */
	if (inodeNumber > maxInodeNumber) {
		inodeNumber = maxInodeNumber;
	}

	/*
	 * Fill in the bar graph.
	 */
	for (nextInodeNumber = fivePct;
	    nextInodeNumber < inodeNumber;
	    nextInodeNumber += fivePct) {
		barGraph[barGraphIndex++] = '*';
	}

	/*
	 * If the bar graph is filled, avoid further entries.
	 */
	if (nextInodeNumber >= maxInodeNumber) {
		nextInodeNumber = LONG_MAX;
		maxInodeNumber = 0;
	}
	while (barGraphIndex < 20) {
		barGraph[barGraphIndex++] = ' ';
	}
	barGraph[barGraphIndex++] = '|';
	barGraph[barGraphIndex] = '\0';
	/* Scanning inodes %s */
	PostOprMsg(4303, barGraph);
	return (nextInodeNumber);
}
