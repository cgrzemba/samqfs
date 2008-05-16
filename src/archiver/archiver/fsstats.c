/*
 * fsstats.c - print file system statistics.
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

#pragma ident "$Revision: 1.31 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <memory.h>
#include <stdio.h>

/* POSIX headers. */
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <sys/param.h>

/* SAM-FS headers. */
#include "sam/mount.h"
#include "sam/lib.h"

/* Local headers. */
#include "archiver.h"
#include "dir_inode.h"

/* Structures. */

struct type_stats {
	struct stats all;
	struct stats archdone;
	struct stats copies[MAX_ARCHIVE];
	struct stats offline;
};

/* Private data. */

/* File statistics */
static struct {
	struct type_stats dir;
	struct type_stats lnk;
	struct type_stats reg;
	struct type_stats rem;
	struct type_stats seg;
	struct type_stats total;

	int	BlkNumof;
	int	ChrNumof;
	int	DataSegments;
	int	FifoNumof;
	int	SockNumof;
	int	UnknownNumof;
} stats;

/* Private functions. */
static void countEntry(struct sam_perm_inode *dinode);
static void countType(struct sam_disk_inode *dinode, struct type_stats *p);
static void printStats(void);
static void printType(struct type_stats *p, char *name);


/*
 * Scan file system .inodes file and print statistics found.
 */
void
PrintFsStats(void)
{
	union sam_di_ino *inodesBuffer;
	fsize_t	inodesSize;
	size_t	inodesBufferSize;
	ino_t	inodeNumber;
	int	bytesReturned;
	int	inodesFree;

	inodesBufferSize = INO_BLK_SIZE * INO_BLK_FACTOR;
	SamMalloc(inodesBuffer, inodesBufferSize);
	memset(&stats, 0, sizeof (stats));
	inodesFree = 0;
	inodesSize = 0;

	/*
	 * Read the .inodes file.
	 */
	inodeNumber = 0;
	while ((bytesReturned = read(FsFd, inodesBuffer, inodesBufferSize)) >
	    0) {
		union sam_di_ino *inodeInBuffer;

		/*
		 * Step through the buffer.  Each active inode will match the
		 * increasing inode number and have a matching version number.
		 */
		inodeInBuffer = inodesBuffer;
		while (bytesReturned > 0) {
			struct sam_perm_inode *pinode;
			struct sam_disk_inode *dinode;

			dinode = &inodeInBuffer->inode.di;
			pinode = (struct sam_perm_inode *)dinode;
			inodeInBuffer++;
			inodeNumber++;
			bytesReturned -= sizeof (union sam_di_ino);

			if (inodeNumber == SAM_INO_INO) {
				inodesSize = dinode->rm.size;
				countEntry(pinode);
				continue;
			}

			/*
			 * Ignore non-file inodes & priviledged inodes.
			 */
			if (dinode->mode == 0 ||
			    S_ISEXT(dinode->mode) ||
			    dinode->id.ino != inodeNumber ||
			    !(SAM_CHECK_INODE_VERSION(dinode->version))) {
				if (!SAM_PRIVILEGE_INO(dinode->version,
				    dinode->id.ino)) {
					inodesFree++;
				}
				continue;
			}
			countEntry(pinode);
		}
	}
	SamFree(inodesBuffer);
	printf("%s: %s (%s bytes)", GetCustMsg(4622), CountToA(inodeNumber),
	    StrFromFsize(inodesSize, 1, NULL, 0));
	printf("  %s: %s\n\n", GetCustMsg(4623), CountToA(inodesFree));
	printf("\n");
	printStats();
	printf("\n");
}


/* Private functions. */


/*
 * Count each entry by type.
 */
static void
countEntry(
	struct sam_perm_inode *pinode)
{
	struct sam_disk_inode *dinode = (struct sam_disk_inode *)pinode;

	/* Ignore the inode extension types */
	if (S_ISEXT(dinode->mode)) {
		return;
	}
	if (S_ISSEGS(dinode)) {
		stats.DataSegments++;
		/*
		 * Ignore data segments.
		 * They will be counted when we do the index segment.
		 */
		return;
	}

	if (S_ISSEGI(dinode)) {
		static struct sam_ioctl_idseginfo ss;
		struct sam_perm_inode dsPinode;

		dinode->status.b.archdone = TRUE;
		dinode->status.b.offline = TRUE;
		dinode->arch_status = 0xf;
		dinode->rm.size = 0;
		ss.size = 0;
		while (GetSegmentInode("", pinode, &dsPinode, &ss) != -1) {
			struct sam_disk_inode *dsDinode =
			    (struct sam_disk_inode *)&dsPinode;

			dinode->rm.size +=
			    dsDinode->rm.size;
			dinode->arch_status &=
			    dsDinode->arch_status;
			dinode->status.b.archdone &=
			    dsDinode->status.b.archdone;
			dinode->status.b.offline &=
			    dsDinode->status.b.offline;
		}
		/*
		 * This now looks like a regular file inode.
		 */
		countType(dinode, &stats.seg);
		countType(dinode, &stats.total);
		return;
	}

	switch (dinode->mode & S_IFMT) {
	case S_IFREG:
		countType(dinode, &stats.reg);
		break;

	case S_IFDIR:
		countType(dinode, &stats.dir);
		break;

	case S_IFLNK:
		countType(dinode, &stats.lnk);
		break;

	case S_IFREQ:
		/* Removable media size is for the media */
		dinode->rm.size = ((dinode->psize.rmfile + SAM_BLK - 1) >>
		    SAM_SHIFT) << SAM_SHIFT;
		countType(dinode, &stats.rem);
		break;

	case S_IFSOCK:
		stats.SockNumof++;
		break;

	case S_IFIFO:
		stats.FifoNumof++;
		break;

	case S_IFCHR:
		stats.ChrNumof++;
		break;

	case S_IFBLK:
		stats.BlkNumof++;
		break;

	default:
		stats.UnknownNumof++;
		break;
	}
	countType(dinode, &stats.total);
}


/*
 * Count entry by type.
 */
static void
countType(
	struct sam_disk_inode *dinode,
	struct type_stats *p)
{
	p->all.numof++;
	p->all.size += dinode->rm.size;
	if (dinode->status.b.offline) {
		p->offline.numof++;
		p->offline.size += dinode->rm.size;
	}
	if (dinode->status.b.archdone) {
		p->archdone.numof++;
		p->archdone.size += dinode->rm.size;
	}
	if (dinode->arch_status & 1) {
		p->copies[0].numof++;
		p->copies[0].size += dinode->rm.size;
	}
	if (dinode->arch_status & 2) {
		p->copies[1].numof++;
		p->copies[1].size += dinode->rm.size;
	}
	if (dinode->arch_status & 4) {
		p->copies[2].numof++;
		p->copies[2].size += dinode->rm.size;
	}
	if (dinode->arch_status & 8) {
		p->copies[3].numof++;
		p->copies[3].size += dinode->rm.size;
	}
}


/*
 * Print out statistics found.
 */
static void
printStats(void)
{
	printf("%-15s     Count  Percent   Bytes  Percent"
	    "                Bytes\n", "File type");
	printType(&stats.total, "All");
	printType(&stats.reg, "Regular");
	printType(&stats.seg, "Segmented");
	printType(&stats.dir, "Directories");
	printType(&stats.lnk, "Symbolic links");
	printType(&stats.rem, "Removable media");

	printf("\n");
	if (stats.DataSegments != 0) {
		printf("%-15s %9s\n", "DataSegments",
		    CountToA(stats.DataSegments));
	}
	if (stats.SockNumof != 0) {
		printf("%-15s %9s\n", "Sockets",
		    CountToA(stats.SockNumof));
	}
	if (stats.FifoNumof != 0) {
		printf("%-15s %9s\n", "Fifos",
		    CountToA(stats.FifoNumof));
	}
	if (stats.ChrNumof != 0) {
		printf("%-15s %9s\n", "Char special",
		    CountToA(stats.ChrNumof));
	}
	if (stats.BlkNumof != 0) {
		printf("%-15s %9s\n", "Block special",
		    CountToA(stats.BlkNumof));
	}
	if (stats.UnknownNumof != 0) {
		printf("%-15s %9s\n", "Unknown",
		    CountToA(stats.UnknownNumof));
	}
}


/*
 * Print statistics from type_stats.
 */
static void
printType(
	struct type_stats *p,
	char *name)
{
	int copy;

	if (p->all.numof == 0) {
		return;
	}
	printf("\n%-15s %9s", name, CountToA(p->all.numof));
	printf("  %6.2f%% %7s  %6.2f%%  %19lld\n",
	    p->all.numof * 100.0 / stats.total.all.numof,
	    StrFromFsize(p->all.size, 1, NULL, 0),
	    p->all.size * 100.0 / stats.total.all.size,
	    p->all.size);
	printf("    %-11s %9s", "offline", CountToA(p->offline.numof));
	if (p->offline.numof != 0) {
		printf("  %6.2f%% %7s  %6.2f%%  %19lld",
		    p->offline.numof * 100.0 / stats.total.all.numof,
		    StrFromFsize(p->offline.size, 1, NULL, 0),
		    p->offline.size * 100.0 / stats.total.all.size,
		    p->offline.size);
	}
	printf("\n");

	printf("    %-11s %9s", "archdone", CountToA(p->archdone.numof));
	if (p->archdone.numof != 0) {
		printf("  %6.2f%% %7s  %6.2f%%  %19lld",
		    p->archdone.numof * 100.0 / stats.total.all.numof,
		    StrFromFsize(p->archdone.size, 1, NULL, 0),
		    p->archdone.size * 100.0 / stats.total.all.size,
		    p->archdone.size);
	}
	printf("\n");

	for (copy = 0; copy < MAX_ARCHIVE; copy++) {
		printf("    copy%-7d %9s", copy + 1,
		    CountToA(p->copies[copy].numof));
		if (p->copies[copy].numof != 0) {
			printf("  %6.2f%% %7s  %6.2f%%  %19lld",
			    p->copies[copy].numof * 100.0 /
			    stats.total.all.numof,
			    StrFromFsize(p->copies[copy].size, 1, NULL, 0),
			    p->copies[copy].size * 100.0 / stats.total.all.size,
			    p->copies[copy].size);
		}
		printf("\n");
	}
}
