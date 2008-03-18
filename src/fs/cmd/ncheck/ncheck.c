/*
 *  samncheck.c - "convert inode numbers to pathnames" command
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

#pragma ident "$Revision: 1.17 $"

/* ANSI C headers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/param.h>

/* SAM-FS headers. */
#include "pub/stat.h"
#define	MAIN
#include "sam/custmsg.h"
#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "aml/id_to_path.h"
#include "sam/uioctl.h"
#include "sam/types.h"

#include "ino.h"


int
main(int argc, char **argv)
{
	char *fsname;
	int arg_index;
	int	fd;

	CustmsgInit(0, NULL);
	if (getuid() != 0 && geteuid() != 0) {
		/* You must be root to run %s\n */
		fprintf(stderr, GetCustMsg(13244), argv[0]);
		exit(EXIT_FAILURE);
	}

	if (argc < 3) {
		/* Usage: %s mount-point inode-number...\n */
		fprintf(stderr, GetCustMsg(13239), argv[0]);
		exit(EXIT_FAILURE);
	}
	fsname = argv[1];
	fd = open(fsname, O_RDONLY);
	if (fd < 0) {
		/* Couldn't open '%s'.\n */
		error(0, errno, GetCustMsg(13222), fsname);
		exit(EXIT_FAILURE);
	}

	sync(); sync();   /* make .inodes a little more current */

	for (arg_index = 2; arg_index < argc; arg_index++) {
		struct sam_perm_inode FileInode;
		static struct sam_ioctl_inode InodeArgs;
		sam_id_t id;
		char *name = NULL;

		(void) sscanf(argv[arg_index], "%u", &id.ino);
		id.gen = 0;
		InodeArgs.ino = id.ino;
		InodeArgs.mode = 0;
		InodeArgs.ip.ptr = NULL;
		InodeArgs.pip.ptr = &FileInode;
		if (ioctl(fd, F_RDINO, &InodeArgs) < 0) {
			/* unable to read inode (via ioctl) %d */
			error(0, errno, GetCustMsg(13245), InodeArgs.ino);
			continue;
		}

		if (FileInode.di.id.ino == 0 || FileInode.di.mode == 0) {
			/* Inode is free, inode %d.\n */
			fprintf(stderr, GetCustMsg(13240), id.ino);
			continue;
		}
		name = id_to_path(fsname, FileInode.di.id);

		if (name) {
			struct sam_stat ss;

			if (sam_lstat(name, &ss,
					sizeof (struct sam_stat)) < 0) {
				printf("%u\t%s\n", id.ino, name);
			} else {
				printf("%u.%ld\t%s\n", id.ino, ss.gen, name);
			}
		} else {
			/* No name returned from id_to_path, inode %d.\n */
			fprintf(stderr, GetCustMsg(13241), id.ino);
		}
	}
	return (EXIT_SUCCESS);
}
