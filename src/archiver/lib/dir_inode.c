/*
 * dir_inode.c - Archiver directory and inode reading functions.
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

#pragma ident "$Revision: 1.9 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <sys/vfs.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "pub/stat.h"
#include "sam/fioctl.h"
#include "sam/uioctl.h"
#include "sam/fs/dirent.h"
#include "sam/fs/ino.h"
#include "sam/fs/sblk.h"
#include "sam/fs/validation.h"

/* Local headers. */
#include "common.h"
#include "dir_inode.h"


/*
 * Get disk inode.
 */
int
GetDinode(
	sam_id_t id,
	struct sam_disk_inode *dinode)
{
	struct sam_ioctl_idstat idstatArgs;

	idstatArgs.id = id;
	idstatArgs.size = sizeof (struct sam_disk_inode);
	idstatArgs.dp.ptr = dinode;
	if (ioctl(FsFd, F_IDSTAT, &idstatArgs) < 0) {
		if (errno == EBADF) {
			errno = 0;
		} else if (errno != ENOENT) {
			Trace(TR_DEBUGERR, "idstat(%d.%d)", id.ino, id.gen);
		}
		return (-1);
	}
	TIME_NOW(dinode) = idstatArgs.time;
	if (AfState != NULL) {
		AfState->AfIdstat++;
	}
	return (0);
}


/*
 * Get directory entry.
 * RETURN: Next directory entry.
 */
struct sam_dirent *
GetDirent(
	struct DirRead *dr)
{
	struct sam_dirent *dp;

	do {
		int	size;

		if (dr->DrCount <= 0) {
			errno = 0;
			if (dr->DrGetdentsArgs.eof) {
				return (NULL);
			}
			if (AfState != NULL) {
				AfState->AfGetdents++;
			}
			dr->DrCount =
			    ioctl(dr->DrFd, F_GETDENTS, &dr->DrGetdentsArgs);
			if (dr->DrCount <= 0) {
				return (NULL);
			}
			dr->DrDp = (char *)(void *)dr->DrGetdentsArgs.dir.ptr;
		}
		dp = (struct sam_dirent *)(void *)dr->DrDp;
		size = SAM_DIRSIZ(dp);
		dr->DrDp += size;
		dr->DrCount -= size;
	} while (dp->d_fmt == 0);
	return (dp);
}


/*
 * Get permanent inode.
 */
int
GetPinode(
	sam_id_t id,
	struct sam_perm_inode *pinode)
{
	struct sam_ioctl_idstat idstatArgs;

	idstatArgs.id = id;
	idstatArgs.size = sizeof (struct sam_perm_inode);
	idstatArgs.dp.ptr = pinode;
	if (ioctl(FsFd, F_IDSTAT, &idstatArgs) < 0) {
		if (errno == EBADF) {
			errno = 0;
		} else if (errno != ENOENT) {
			Trace(TR_DEBUGERR, "idstat(%d.%d)", id.ino, id.gen);
		}
		return (-1);
	}
	TIME_NOW((struct sam_disk_inode *)pinode) = idstatArgs.time;
	if (AfState != NULL) {
		AfState->AfIdstat++;
	}
	return (0);
}


/*
 * Get the next data segment inode.
 * RETURN: segment number; -1 if no more segments.
 */
int
GetSegmentInode(
	char *filePath,
	struct sam_perm_inode *pinode,	/* For the segment index */
	struct sam_perm_inode *dsPinode, /* Data segment inode returned */
	struct sam_ioctl_idseginfo *ss)	/* For first call, ss->size == 0 */
{
#define	segOrd id.gen	/* used after first call */
#define	segInd id.ino

	if (ss->size == 0) {
		struct sam_disk_inode *dinode = (struct sam_disk_inode *)pinode;

		ss->size = dinode->rm.info.dk.seg.fsize;
		if (ss->size == 0) {
			return (-1);
		}
		SamRealloc(ss->buf.ptr, ss->size);
		memset(ss->buf.ptr, 0, ss->size);
		ss->id = dinode->id;
		ss->offset = 0;
		if (ioctl(FsFd, F_IDSEGINFO, ss) < 0) {
			Trace(TR_ERR, "ioctl:F_IDSEGMENT(%d.%d, %s)",
			    dinode->id.ino, dinode->id.gen, filePath);
			return (-1);
		}
		ss->segInd = 0;
		ss->segOrd = 0;
	}

	while (ss->offset < ss->size) {
		if (ss->segInd >= SAM_MAX_SEG_ORD) {
			ss->segInd = 0;
			ss->segOrd += SAM_MAX_SEG_ORD;
			ss->offset += SAM_SEG_BLK;
		} else {
			sam_id_t *pid;
			int	segNum;

			segNum = ss->segOrd + ss->segInd;
			pid = ((sam_id_t *)(void *)((ss->buf.ptr) + ss->offset))
			    + ss->segInd;
			ss->segInd++;
			if (pid->ino != 0) {
				if (GetPinode(*pid, dsPinode) == 0) {
					return (segNum);
				}
				Trace(TR_ERR, "stat segment(%d.%d.S%d, %s)",
				    pid->ino, pid->gen, segNum, filePath);
			}
		}
	}
	return (-1);
#undef segOrd
#undef segInd
}


/*
 * Open directory.
 */
int
OpenDir(
	sam_id_t id,
	struct sam_disk_inode *dinode,
	struct DirRead *dr)
{
	struct sam_ioctl_idopen idopenArgs;

	if (dr->DrGetdentsArgs.dir.ptr == NULL) {
		dr->DrGetdentsArgs.size = DIRBUF_SIZE;
		SamMalloc(dr->DrGetdentsArgs.dir.ptr, DIRBUF_SIZE);
	}
	idopenArgs.id = id;
	idopenArgs.mtime = 0;
	idopenArgs.copy = 0;
	idopenArgs.flags = 0;
	idopenArgs.dp.ptr = dinode;
	if ((dr->DrFd = ioctl(FsFd, F_IDOPENDIR, &idopenArgs)) == -1) {
		Trace(TR_DEBUGERR, "opendir(%d.%d)", id.ino, id.gen);
		return (-1);
	}
	if (dinode != NULL) {
		TIME_NOW(dinode) = idopenArgs.mtime;
	}
	dr->DrGetdentsArgs.offset = 0;
	dr->DrGetdentsArgs.eof = FALSE;
	dr->DrCount = 0;
	if (AfState != NULL) {
		AfState->AfOpendir++;
	}
	return (0);
}
