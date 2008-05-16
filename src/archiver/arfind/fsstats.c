/*
 * fsstats.c - manage file system statistics.
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.24 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

/* SAM-FS headers. */
#include "sam/mount.h"

/* Local headers. */
#include "arfind.h"
#include "dir_inode.h"


/*
 * Count each file by type.
 * Called from the file system scanners.
 */
void
FsstatsCountFile(
	struct sam_perm_inode *pinode,
	char *filePath)
{
	struct sam_disk_inode *dinode = (struct sam_disk_inode *)(void *)pinode;

	if (S_ISSEGS(dinode)) {
		return;
	}

#if defined(FILE_TRACE)
	if (filePath == NULL) {
		filePath = "*NO-PATH*";
	}
	Trace(TR_DEBUG, "Counting %s (%d.%d)", filePath, dinode->id.ino,
	    dinode->id.gen);
#endif /* defined(FILE_TRACE) */

	if (S_ISSEGI(dinode)) {
		static struct sam_ioctl_idseginfo ss;
		struct sam_perm_inode dsPinode;
		int	sn;

		/*
		 * Use the index inode to cumulate the information.
		 */
		dinode->status.b.archdone = TRUE;
		dinode->status.b.offline = TRUE;
		dinode->arch_status = 0xf;
		dinode->rm.size = 0;
		ss.size = 0;
		while ((sn =
		    GetSegmentInode("", pinode, &dsPinode, &ss)) != -1) {
			struct sam_disk_inode *dsDinode =
			    (struct sam_disk_inode *)&dsPinode;

#if defined(FILE_TRACE)
			Trace(TR_DEBUG, "  %d (%d.%d)", sn, dinode->id.ino,
			    dinode->id.gen);
#endif /* defined(FILE_TRACE) */
			dinode->rm.size += dsDinode->rm.size;
			dinode->arch_status &= dsDinode->arch_status;
			dinode->status.b.archdone &=
			    dsDinode->status.b.archdone;
			dinode->status.b.offline &= dsDinode->status.b.offline;
		}
		/*
		 * This now looks like a regular file inode.
		 */
	}

	switch (dinode->mode & S_IFMT) {
	case S_IFREG:
		State->AfStatsScan.total.numof++;
		State->AfStatsScan.total.size += dinode->rm.size;
		State->AfStatsScan.regular.numof++;
		State->AfStatsScan.regular.size += dinode->rm.size;
		if (dinode->status.b.offline) {
			State->AfStatsScan.offline.numof++;
			State->AfStatsScan.offline.size += dinode->rm.size;
		}
		if (dinode->status.b.archdone) {
			State->AfStatsScan.archdone.numof++;
			State->AfStatsScan.archdone.size += dinode->rm.size;
		}
		if (dinode->arch_status & 1) {
			State->AfStatsScan.copies[0].numof++;
			State->AfStatsScan.copies[0].size += dinode->rm.size;
		}
		if (dinode->arch_status & 2) {
			State->AfStatsScan.copies[1].numof++;
			State->AfStatsScan.copies[1].size += dinode->rm.size;
		}
		if (dinode->arch_status & 4) {
			State->AfStatsScan.copies[2].numof++;
			State->AfStatsScan.copies[2].size += dinode->rm.size;
		}
		if (dinode->arch_status & 8) {
			State->AfStatsScan.copies[3].numof++;
			State->AfStatsScan.copies[3].size += dinode->rm.size;
		}
		break;

	case S_IFDIR:
		State->AfStatsScan.total.numof++;
		State->AfStatsScan.total.size += dinode->rm.size;
		State->AfStatsScan.dirs.numof++;
		State->AfStatsScan.dirs.size += dinode->rm.size;
		break;

	case S_IFLNK:
	case S_IFREQ:
	case S_IFSOCK:
	case S_IFIFO:
	case S_IFCHR:
	case S_IFBLK:
	default:
		break;
	}
}


/*
 * Reset statistics.
 */
void
FsstatsStart(void)
{
	memset(&State->AfStatsScan, 0, sizeof (State->AfStatsScan));
}


/*
 * Update statistics.
 */
void
FsstatsEnd(void)
{
	if (State->AfStatsScan.total.numof != -1) {
		State->AfStats = State->AfStatsScan;
		State->AfStatsScan.total.numof = -1;
	}
}
