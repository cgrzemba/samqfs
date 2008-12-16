/*
 * This program scans the .inodes file on a SAM-QFS file system and counts
 * how many of the inodes in the file are free.  This can be used as part
 * of a metadata space monitoring regimen.  Note that it is still important
 * to monitor metadata space which has not been allocated to .inodes, since
 * additional space is required for directories and indirect blocks.
 *
 * Usage: qfsicount [-v] fsname
 *
 *   The '-v' option displays counts for each type of inode.  Without -v,
 *   all non-free inode types will be displayed in aggregate.
 *
 *   The fsname should be a SAM file system name or a (full) mount point.
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

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/mnttab.h>
#include <sys/vfs.h>

#include "sam/mount.h"
#include "sam/syscall.h"
#include "sam/uioctl.h"

#include "sam/fs/ino.h"
#include "sam/fs/sblk.h"

#define	INO_KINDS 18	/* 16 "real" types + "reserved" + "bad version" */
#define	INO_PER_PASS 1024

static union sam_di_ino inobuf[INO_PER_PASS];

static const char *inonames[INO_KINDS] = {
	"free", "fifo", "character", "$03", "directory", "$05", "block", "$07",
	"regular", "$09", "symlink", "$0B", "socket", "$0D", "rmedia",
	"extension",
	"reserved", "badvers"
};

static void usage(char *name);
static int open_inode_file(const char *fsname);
static int scan_inode_file(int inode_fd, int stats[INO_KINDS]);

int
main(int argc, char *argv[])
{
	boolean_t verbose;
	char *fsname;
	int inode_fd;
	int err;
	int stats[INO_KINDS];

	/* Parse arguments, this is ugly but.... */

	if (argc == 2) {
		verbose = FALSE;
		fsname = argv[1];
	} else if (argc == 3) {
		if (strcmp(argv[1], "-v") != 0) {
			usage(argv[0]);
		}
		verbose = TRUE;
		fsname = argv[2];
	} else {
		usage(argv[0]);
	}

	inode_fd = open_inode_file(fsname);
	if (inode_fd < 0) {
		fprintf(stderr, "%s: Could not open .inodes file for %s.\n",
		    argv[0], fsname);
		exit(2);
	}

	err = scan_inode_file(inode_fd, stats);
	if (err != 0) {
		fprintf(stderr, "%s: Error %d reading .inodes file\n",
		    argv[0], err);
		exit(3);
	}

	(void) close(inode_fd);

	if (verbose) {
		int i;

		printf("Filesystem %s\n", fsname);
		for (i = 0; i < INO_KINDS; i++) {
			if ((i == 0) || (stats[i] != 0)) {
				printf("%-10s %d\n", inonames[i], stats[i]);
			}
		}
	} else {
		int i, used;

		for (i = 1, used = 0; i < INO_KINDS; i++) {
			used += stats[i];
		}

		printf("Filesystem\tAllocated\tFree\n");
		printf("%-10s\t%-9d\t%d\n", fsname, used, stats[0]);
	}

	return (0);
}

static void
usage(char *name)
{
	fprintf(stderr, "Usage: %s [-v] fsname\n", name);
	exit(1);
}

static int
open_inode_file(const char *fsname)
{
	FILE *mnt_fp;
	struct mnttab mnt_entry;
	struct sam_ioctl_idopen arg;
	char *mountpoint;
	int mountpoint_fd;
	int inode_fd;

	/* If given a SAM file system name, convert to a mount point. */

	if (fsname[0] != '/') {
		mnt_fp = fopen(MNTTAB, "r");
		if (mnt_fp == NULL) {
			return (-1);
		}

		mountpoint = NULL;

		while (getmntent(mnt_fp, &mnt_entry) == 0) {
			if (strcmp(mnt_entry.mnt_special, fsname) == 0) {
				mountpoint = mnt_entry.mnt_mountp;
				break;
			}
		}

		(void) fclose(mnt_fp);

		if (mountpoint == NULL) {
			return (-1);
		}
	} else {
		mountpoint = (char *)fsname;
	}

	/* Open the .inodes file by using an ioctl on the mount point. */

	mountpoint_fd = open(mountpoint, O_RDONLY);
	if (mountpoint_fd < 0) {
		return (-1);
	}

	bzero(&arg, sizeof (arg));
	arg.id.ino = SAM_INO_INO;
	inode_fd = ioctl(mountpoint_fd, F_IDOPEN, &arg);

	(void) close(mountpoint_fd);

	return (inode_fd);
}

static int
scan_inode_file(int inode_fd, int stats[INO_KINDS])
{
	int i, bytes, inodes, offset;

	for (i = 0; i < INO_KINDS; i++) {
		stats[i] = 0;
	}

	offset = 0;
	while ((bytes = read(inode_fd, &inobuf, sizeof (inobuf))) > 0) {
		inodes = bytes / sizeof (union sam_di_ino);
		for (i = 0; i < inodes; i++) {
			int vers, kind;

			vers = inobuf[i].inode.di.version;
			kind = (inobuf[i].inode.di.mode >> 12) & 0x0F;

			if ((vers != 1) && (vers != 2)) {
				if ((vers != 0) || (kind != 0)) {
					kind = 17;	/* bad version */
				}
			}

			if ((kind == 0) && ((offset + i) < SAM_MIN_USER_INO)) {
				kind = 16;			/* reserved */
			}

			stats[kind]++;
		}
		offset += inodes;
	}

	return (bytes);
}
