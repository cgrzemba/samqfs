/*
 * scandirs.c - Scan directories.
 *
 * Scan a filesystem by traversing directory trees.
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

#pragma ident "$Revision: 1.63 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* Feature test switches. */
/* FILE_TRACE	If defined, turn on DEBUG traces for files processed */
/* SCAN_TRACE	If defined, turn on DEBUG traces for scanning */
#if defined(DEBUG)
#define	SCAN_TRACE
#endif

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
#include <unistd.h>
#include <sys/stat.h>

/* SAM-FS headers. */
#include "pub/stat.h"

/* Local headers. */
#include "arfind.h"
#include "dir_inode.h"

/* Private data. */
static char *baseName;		/* Position in path buffer of base name */
static struct PathBuffer pathBuf, *pb = &pathBuf;

/*
 * Directory entry stack.
 * Directory entries for the directories found in the directory being scanned
 * are stacked and then removed for the recursive directory scan.
 */
static char *dirEnts = NULL;	/* Unprocessed directory directory entries */
static int dirEntsSize = 0;	/* Current size of dirEnts */

/* Inode stat information. */
static struct sam_perm_inode pinode;
static struct sam_disk_inode *dinode = (struct sam_disk_inode *)&pinode;

static int maxDirEntsSize = 0;	/* Maximum size of dirEnts */

/* Private functions. */
static void addDirent(sam_dirent_t *dp);
static int scanDir(sam_dirent_t *dp, struct ScanListEntry *se);
static boolean_t isNoarchDir(char *dirPath);


/*
 * Scan directory tree.
 * Examine the directory tree specified in the scanlist entry.
 */
void
ScanDirs(
	struct ScanListEntry *se)
{
	static struct sam_dirent dp;

	if (se->SeId.ino == SAM_ROOT_INO) {
		struct ScanListEntry seAdd;

		/*
		 * Root directory.
		 * Check the root inode.
		 */
		pb->PbEnd = pb->PbPath;
		*pb->PbPath = '\0';
		if (GetPinode(se->SeId, &pinode) != 0) {
			return;
		}
		memset(&seAdd, 0, sizeof (seAdd));
		seAdd.SeFlags = se->SeFlags & SE_request;
		seAdd.SeTime = TIME32_MAX;
		EXAM_MODE(dinode) = EXAM_DIR;
		(void) CheckFile(pb, &pinode, &seAdd);
		if (seAdd.SeTime != TIME32_MAX) {
			ScanfsAddEntry(&seAdd);
		}
	}

	/*
	 * Reset directory stack.
	 */
	dirEntsSize = 0;
	IdToPathId(se->SeId, pb);
	Trace(TR_FILES, "Scan %s", ScanPathToMsg(pb->PbPath));
#if defined(SCAN_TRACE)
	Trace(TR_DEBUG, "Scanning %s %d.%d", ScanPathToMsg(pb->PbPath),
	    se->SeId.ino, se->SeId.gen);
#endif /* defined(SCAN_TRACE) */
	baseName = pb->PbEnd;
	dp.d_id = se->SeId;

	if (scanDir(&dp, se) != 0) {
		/*
		 * Interrupted.
		 */
#if defined(SCAN_TRACE)
		Trace(TR_DEBUG, "Interrupted - %s", ScanPathToMsg(pb->PbPath));
#endif /* defined(SCAN_TRACE) */
	}
#if defined(SCAN_TRACE)
	Trace(TR_DEBUG, "Scan finished");
#endif /* defined(SCAN_TRACE) */
}


/*
 * Initialize module.
 */
void
ScanDirsInit(void)
{
	int	l;

	/*
	 * Construct path for file action.
	 * Place string terminator at beginning of buffer.
	 */
	pb->PbPath = pb->PbBuf;
	l = strlen(MntPoint);
	strncpy(pb->PbPath, MntPoint, sizeof (pb->PbBuf)-2);
	pb->PbPath += l;
	*pb->PbPath++ = '/';
}


/* Private functions. */


/*
 * Recursively scan a directory.
 * Save each directory entry, and process at the end.
 * Skip directories until previous sub-directory reached.
 * RETURN: 1 if interrupted, 0 if normal exit.
 */
static int
scanDir(
	sam_dirent_t *dpScan,	/* Directory to scan */
	struct ScanListEntry *se)
{
	static struct DirRead dr;
	struct ScanListEntry seAdd;
	sam_dirent_t *dp;
	size_t	dirEntsMark;
	size_t	next;
	boolean_t rootDir;
	char	*baseNameMark;

	if (Exec != ES_run && !(se->SeFlags & SE_request)) {
		return (1);
	}

	/*
	 * Begin the directory read.
	 */
	if (OpenDir(dpScan->d_id, dinode, &dr) < 0) {
		return (0);
	}

	if ((se->SeFlags & SE_noarch) && dinode->status.b.noarch) {
		Trace(TR_DEBUG, "Skipping -n %s%s", pb->PbPath, dpScan->d_name);
		(void) close(dr.DrFd);
		return (0);
	}

	/*
	 * Extend the full path.
	 */
	baseNameMark = baseName;
	rootDir = (dpScan->d_id.ino == SAM_ROOT_INO);
	if (!rootDir) {
		memmove(baseName, dpScan->d_name, dpScan->d_namlen);
		baseName += dpScan->d_namlen;
		*baseName = '\0';
		if (!(State->AfFlags & ASF_archivemeta) &&
		    (se->SeFlags & SE_noarch) &&
		    isNoarchDir(pb->PbPath)) {
			Trace(TR_DEBUG, "Skipping noarch %s", pb->PbPath);
			baseName = baseNameMark;
			(void) close(dr.DrFd);
			return (0);
		}
#if defined(FILE_TRACE)
		Trace(TR_DEBUG, "Scanning %s", pb->PbPath);
#endif /* defined(FILE_TRACE) */
		/* Scanning %s */
		PostOprMsg(4357, pb->PbPath);
		*baseName++ = '/';
	} else {
		PostOprMsg(4357, ".");
	}

	/*
	 * Mark the directory entry stack.
	 * Initialize worklist entry.
	 */
	dirEntsMark = dirEntsSize;
	memset(&seAdd, 0, sizeof (seAdd));
	seAdd.SeFlags = se->SeFlags & SE_request;
	seAdd.SeId = dpScan->d_id;
	seAdd.SeTime = TIME32_MAX;
	while ((dp = GetDirent(&dr)) != NULL) {
		char	*name;

		name = (char *)dp->d_name;
		/* ignore dot and dot-dot */
		if (*name == '.' &&
		    (*(name+1) == '\0' ||
		    (*(name+1) == '.' && *(name+2) == '\0'))) {
			continue;
		}
		if (rootDir && *name == '.') {
			/*
			 * In the root directory.
			 * Ignore named priviledged directories and files.
			 */
			if (strcmp(name, ".archive") == 0 ||
			    strcmp(name, ".rft") == 0 ||
			    strcmp(name, ".stage") == 0 ||
			    strcmp(name, ".inodes") == 0) {
#if defined(FILE_TRACE)
				Trace(TR_DEBUG, "Ignoring %s", name);
#endif /* defined(FILE_TRACE) */
				continue;
			}
		}
		if (S_ISDIR(dp->d_fmt)) {
			if (se->SeFlags & SE_subdir) {
				addDirent(dp);
			}
			if (!(State->AfFlags & ASF_archivemeta)) {
				continue;
			}
		}

		/*
		 * Append name to end of directory path.
		 */
		if ((Ptrdiff(baseName, pb->PbPath) + dp->d_namlen) >=
		    MAXPATHLEN) {
			/*
			 * Get a copy of the inode.
			 * Only send message and set archdone if
			 * archdone not set.
			 */
			if (GetPinode(dp->d_id, &pinode) != 0) {
				if (FsFd < 0) {
					break;
				}
			} else if (!dinode->status.b.archdone) {
				Trace(TR_DEBUGERR,
				    "inode %d: pathname too long",
				    dp->d_id.ino);
				SetArchdone(pb->PbPath, dinode);
			}
			continue;
		}
		memmove(baseName, name, dp->d_namlen + 1);
		pb->PbEnd = baseName + dp->d_namlen;
		*pb->PbEnd = '\0';

		/*
		 * Get a copy of the inode.
		 */
		if (GetPinode(dp->d_id, &pinode) != 0) {
			if (FsFd < 0) {
				break;
			}
			continue;
		}
		EXAM_MODE(dinode) = EXAM_DIR;
#if defined(FILE_TRACE)
		Trace(TR_DEBUG, "Checking %d.%d %s",
		    dp->d_id.ino, dp->d_id.gen, pb->PbPath);
#endif /* defined(FILE_TRACE) */
		CheckFile(pb, &pinode, &seAdd);
		if (se->SeFlags & SE_stats) {
			FsstatsCountFile(&pinode, pb->PbPath);
		}
	}

	if (errno != 0) {
		Trace(TR_DEBUGERR, "readdir(%s)", pb->PbPath);
	}
	if (close(dr.DrFd) < 0) {
		Trace(TR_DEBUGERR, "closedir(%s)", pb->PbPath);
	}
	if (FsFd < 0) {
		goto out;
	}

	/*
	 * Process all the directories found in this directory.
	 */
	for (next = dirEntsMark; next < dirEntsSize; /* Increment inside */) {
		dp = (sam_dirent_t *)(void *)(dirEnts + next);
		next += SAM_DIRSIZ(dp);
		if (scanDir(dp, se) != 0) {
			return (1);
		}
	}
	if (seAdd.SeTime != TIME32_MAX) {
		/*
		 * Future work found.
		 * Add directory to worklist.
		 * Avoid scanning the directory too soon.
		 */
		seAdd.SeTime = max(seAdd.SeTime, time(NULL) + EPSILON_TIME);
		if (rootDir) {
			ScanfsAddEntry(&seAdd);
		} else {
			*(baseName - 1)  = '\0';
			ScanfsAddEntry(&seAdd);
		}
	}

	/*
	 * Remove this directory from the path.
	 */
out:
	dirEntsSize = dirEntsMark;
	baseName = baseNameMark;
	*baseName = '\0';
	return (FsFd < 0);
}



/*
 * Add a directory entry to the stack
 */
static void
addDirent(
	sam_dirent_t *dp)
{
	static int dirEntsAlloc = 0;
	int	len;

	len = SAM_DIRSIZ(dp);
	if (dirEntsSize + len >= dirEntsAlloc) {
		/*
		 * No room in table for entry.  Reallocate space for enlarged
		 * stack.
		 */
		dirEntsAlloc += 4096;
		SamRealloc(dirEnts, dirEntsAlloc);
	}
	memmove(&dirEnts[dirEntsSize], dp, len);
	dirEntsSize += len;
	if (dirEntsSize > maxDirEntsSize) {
		maxDirEntsSize = dirEntsSize;
	}
}


/*
 * Determine if a directory is no_archive.
 */
boolean_t
isNoarchDir(
	char *dirPath)
{
	struct FilePropsEntry *fp;
	int	i;

	for (i = 1; i < FileProps->FpCount; i++) {
		/*
		 * Compare the directory path to the file properties path.
		 * An empty file properties path will match all directories.
		 */
		fp = &FileProps->FpEntry[i];
		if (fp->FpPathSize != 0) {
			char	*pp, *dp;

			dp = dirPath;
			if (dp[fp->FpPathSize] != '\0' &&
			    dp[fp->FpPathSize] != '/') {
				continue;
			}
			pp = fp->FpPath;
			while (*pp != '\0' && *pp == *dp) {
				pp++;
				dp++;
			}
			if ((*pp != '\0' && *pp != '/') ||
			    (*dp != '\0' && *dp != '/')) {
				continue;
			}

			/*
			 * Directory matches first part of path
			 * of files to be archived.
			 */
			if (fp->FpFlags & FP_props) {
				/*
				 * Have to examine files in directory.
				 */
				return (FALSE);
			}
			break;
		} else if (!(fp->FpFlags & FP_props)) {
			break;
		}
	}
	if (i < FileProps->FpCount && (fp->FpFlags & FP_noarch)) {
		return (TRUE);
	}
	return (FALSE);
}
