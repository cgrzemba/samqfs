/*
 * archstatus.c - print archive status for files in a file system.
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

#pragma ident "$Revision: 1.5 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* Feature test switches. */
/* FILE_TRACE   If defined, turn on DEBUG traces for files processed */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

/* SAM-FS headers. */
#include "pub/stat.h"
#include "sam/lib.h"

/* Local headers. */
#include "archiver.h"
#include "dir_inode.h"
#include "../arfind/arfind.h"


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

/* Counts */
static int dirDepth;		/* Directory depth. */
static int maxDepth = 0;	/* Maximum directory depth */
static int maxDirEntsSize = 0;	/* Maximum size of dirEnts */

/* Private functions. */
static void addDirent(sam_dirent_t *dp);
static void printArchiveStatus(char *filePath, struct sam_perm_inode *pinode);
static int scanDir(sam_dirent_t *dp);


/*
 * Print archive status.
 */
void
PrintArchiveStatus(void)
{
	static struct sam_dirent dp;
	int	l;

	ClassifyInit();
	pb->PbPath = pb->PbBuf;
	l = strlen(MntPoint);
	strncpy(pb->PbPath, MntPoint, sizeof (pb->PbBuf)-2);
	pb->PbPath += l;
	*pb->PbPath++ = '/';
	pb->PbEnd = pb->PbPath;
	*pb->PbPath = '\0';
	baseName = pb->PbEnd;
	dp.d_id.ino = SAM_ROOT_INO;
	dp.d_id.gen = 2;
	(void) scanDir(&dp);
}


/* Private functions. */


/*
 * Print archive detail for entry.
 */
static void
printArchiveStatus(
	char *filePath,
	struct sam_perm_inode *pinode)
{
	struct sam_disk_inode *dinode = (struct sam_disk_inode *)pinode;
	struct ArchSet *as;
	struct FilePropsEntry *fp;
	sam_time_t t;
	time_t	accessRef;
	time_t	archRef;
	time_t	modifyRef;
	char	ft;
	char	*p;
	int	copy;
	int	copyBit;

	if (S_ISLNK(dinode->mode)) {
		ft = 'l';
	} else if (S_ISREQ(dinode->mode)) {
		ft = 'R';
	} else if (S_ISSEGI(dinode)) {
		ft = 'I';
	} else if (S_ISDIR(dinode->mode)) {
		ft = 'd';
	} else if (S_ISSEGS(dinode)) {
		ft = 'S';
	} else if (S_ISREG(dinode->mode)) {
		ft = 'f';
	} else if (S_ISBLK(dinode->mode)) {
		ft = 'b';
	} else {
		ft = '?';
	}
	printf("%c \"", ft);
	for (p = filePath; *p != 0; p++) {
		if (*p == '"') {
			(void) putchar('\\');
			(void) putchar('"');
		} else if (*p == '\\') {
			(void) putchar('\\');
			(void) putchar('\\');
		} else {
			(void) putchar(*p);
		}
	}
	printf("\" ");

	printf("%d.%d ", dinode->id.ino, dinode->id.gen);
	fp = ClassifyFile(filePath, pinode, &t);
	if (fp == NULL || (fp->FpFlags & FP_noarch)) {
		printf("- - - - -\n");
		return;
	}
	as = &ArchSetTable[fp->FpBaseAsn];
	printf("%s ", as->AsName);
	/*
	 * Set access and modification reference times.
	 */
	modifyRef = dinode->modify_time.tv_sec;
	archRef = modifyRef;
	accessRef = dinode->access_time.tv_sec;

	for (copy = 0, copyBit = 1; copy < MAX_ARCHIVE; copy++, copyBit <<= 1) {
		static char ts[ISO_STR_FROM_TIME_BUF_SIZE];
		static char tmFmt[] = "%Y-%m-%dT%T";
		static struct tm tm;
		time_t	tv;

		if (copy != 0) {
			printf(" ");
		}
		if (!(fp->FpCopiesReq & copyBit)) {
			printf("-");
			continue;
		}
		if (dinode->arch_status & copyBit) {
			printf("%s.%s", sam_mediatoa(dinode->media[copy]),
			    pinode->ar.image[copy].vsn);
		} else {
			if (dinode->ar_flags[copy] & (AR_arch_i | AR_rearch)) {
				tv = TIME_NOW(dinode);
			} else {
				tv = archRef + fp->FpArchAge[copy];
			}
			strftime(ts, sizeof (ts), tmFmt, localtime_r(&tv, &tm));
			printf("%s", ts);
		}
		if (fp->FpCopiesUnarch & copyBit) {
			as = &ArchSetTable[fp->FpAsn[copy]];
			if (as->AsEflags & AE_unarchage) {
				tv = modifyRef + fp->FpUnarchAge[copy];
			} else {
				tv = accessRef + fp->FpUnarchAge[copy];
			}
			strftime(ts, sizeof (ts), tmFmt, localtime_r(&tv, &tm));
			printf("/%s", ts);
		}
	}
	printf("\n");
}


/*
 * Recursively scan a directory.
 * Save each directory entry, and process at the end.
 * Return: 1 if interrupted, 0 if normal exit.
 */
static int
scanDir(
	sam_dirent_t *dpScan)	/* Directory to scan */
{
	static struct DirRead dr;
	sam_dirent_t *dp;
	size_t	dirEntsMark;
	size_t	next;
	boolean_t rootDir;
	char	*baseNameMark;

	baseNameMark = baseName;
	dr.DrFd = 0;

	/*
	 * Begin the directory read.
	 */
	if (OpenDir(dpScan->d_id, dinode, &dr) < 0) {
		return (0);
	}

	/*
	 * Extend the full path.
	 */
	rootDir = (dpScan->d_id.ino == SAM_ROOT_INO);
	if (!rootDir) {
		memmove(baseName, dpScan->d_name, dpScan->d_namlen);
		baseName += dpScan->d_namlen;
		*baseName++ = '/';
	}

	/*
	 * Mark the directory entry stack.
	 * Initialize worklist entry.
	 */
	dirEntsMark = dirEntsSize;
	dirDepth++;
	if (dirDepth > maxDepth) {
		maxDepth = dirDepth;
	}

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
				printf("Ignoring %s\n", name);
#endif /* defined(FILE_TRACE) */
				continue;
			}
		}
		if (S_ISDIR(dp->d_fmt)) {
			addDirent(dp);
		}

		/*
		 * Append name to end of directory path.
		 */
		if ((Ptrdiff(baseName, pb->PbPath) + dp->d_namlen) >=
		    MAXPATHLEN) {
#if defined(FILE_TRACE)
			printf("inode %d: pathname too long\n", dp->d_id.ino);
#endif /* defined(FILE_TRACE) */
			continue;
		}
		memmove(baseName, name, dp->d_namlen + 1);
		pb->PbEnd = baseName + dp->d_namlen;
		*pb->PbEnd = '\0';

		/*
		 * Get a copy of the inode.
		 */
		if (GetPinode(dp->d_id, &pinode) != 0) {
			continue;
		}
		EXAM_MODE(dinode) = EXAM_DIR;
#if defined(FILE_TRACE)
		printf("Checking %d.%d %s\n", dp->d_id.ino, dp->d_id.gen,
		    pb->PbPath);
#endif /* defined(FILE_TRACE) */
		printArchiveStatus(pb->PbPath, &pinode);
	}

	if (errno != 0) {
		fprintf(stderr, "readdir(%s)\n", pb->PbPath);
	}
	if (close(dr.DrFd) < 0) {
		fprintf(stderr, "closedir(%s)\n", pb->PbPath);
	}

	/*
	 * Process all the directories found in this directory.
	 */
	for (next = dirEntsMark; next < dirEntsSize; /* Increment inside */) {
		dp = (sam_dirent_t *)(void *)(dirEnts + next);
		next += SAM_DIRSIZ(dp);
		if (scanDir(dp) != 0) {
			return (1);
		}
	}
	dirEntsSize = dirEntsMark;

	/*
	 * Remove this directory from the path.
	 */
	baseName = baseNameMark;
	*baseName = '\0';
	dirDepth--;
	return (0);
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
