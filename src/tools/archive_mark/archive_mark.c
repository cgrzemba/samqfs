/*
 *    archive_mark.c - Mark file as archived.
 *
 *    Archive_mark marks the specified file as archived.
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

#pragma ident "$Revision: 1.18 $"


/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stropts.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


/* POSIX headers. */
#include <sys/fcntl.h>

/* SAM-FS headers. */
#include "pub/lib.h"
#include "pub/stat.h"
#include "pub/rminfo.h"

#include "sam/types.h"
#include "aml/device.h"
#include "sam/fioctl.h"
#include "sam/uioctl.h"
#include "sam/resource.h"
#include "sam/fs/ino.h"
#include "sam/devnm.h"
#include "sam/fs/ino.h"
#include "sam/fs/ino_ext.h"
#include "sam/lib.h"


char *program_name;			/* Program name: used by error() */


void
main(int argc, char *argv[])
{
	int copy;			/* Archive copy */
	int verbose;			/* Verbose flag */
	long offset;			/* File offset */
	char *file;			/* File name */
	char *archive_file;		/* Archive file name */
	struct sam_stat fstat;		/* Stat reply for file */
	struct stat rstat;		/* Stat reply for resource file */
	int err = 0;
	int i;
	char *p;
	struct sam_ioctl_setarch setarch;
	struct sam_rminfo buf;
	struct sam_rminfo *rmp = &buf;
	int dt;
	int fd;

	if (argc < 4) {
		printf("Usage:\t%s\n",
		    "archive_mark -l1..4 [-v] [-offset n] resource_file file");
		exit(-1);
	}
	program_name = "archive_mark";

	copy = -1;			/* Default parameters */
	verbose = FALSE;
	offset = 0;

	while (--argc > 0) {
		if (**++argv == '-') {
			if (strncmp(*argv, "-c", 2) == 0 &&
			    strlen(*argv) == 3) {
				i = *(*argv + 2) - '0' - 1;
				if (i >= 0 && i < MAX_ARCHIVE) {
					copy = i;
					continue;
				} else {
					error(0, 0,
					    "Unknown archive copy (%c)",
					    *(*argv + 2));
				}
				exit(1);
			}
			if (strcmp(*argv, "-v") == 0) {
				verbose = TRUE;
				continue;
			}
			if (strcmp(*argv, "-offset") == 0) {
				offset = atol(*++argv);
				argc--;
				continue;
			}
			error(1, 0, "Unrecognized argument (%s)", *argv);
		}
		break;
	}

	if (copy < 0)
		error(1, 0, "Archive copy not specified.");
	if (argc < 1)
		error(1, 0, "Resource/file not specified.");
	if (argc < 2)
		error(1, 0, "File not specified.");

	archive_file = *argv;
	file = *++argv;

	if (sam_stat(file, &fstat, sizeof (fstat)) < 0)
		error(1, errno, "sam_stat: %s", file);

	if (stat(archive_file, &rstat) < 0)
		error(1, errno, "stat: %s", archive_file);

	if (!S_ISREQ(rstat.st_mode))
		error(1, errno, "%s: Not a resource file.", archive_file);

	if (sam_readrminfo(archive_file, &buf, sizeof (struct sam_rminfo)) < 0)
		error(1, errno, "sam_readrminfo: %s", archive_file);

	if (verbose)
		fprintf(stderr, "archive_mark: copy=%d %s %s\n",
		    copy + 1, file, archive_file);

	dt = media_to_device(rmp->media);
	if (verbose) {
		if ((dt & DT_CLASS_MASK) == DT_OPTICAL) {
			fprintf(stderr, "archive_mark: %s %.8x %s %s\n",
			    rmp->media,
			    rmp->version,
			    rmp->section[0].vsn,
			    rmp->file_id);
		} else {		/* Tape */
			fprintf(stderr, "archive_mark: %x %d %s\n",
			    rmp->media,
			    rmp->section[0].vsn);
		}
	}
	if ((dt & DT_CLASS_MASK) == DT_OPTICAL) {
		i = strcmp(rmp->group_id, SAM_ARCHIVE_GROUP);
		if (i != 0) {
			error(0, 0, "Removable media file must have group "
			    "id of (%s) to be used for archive.",
			    SAM_ARCHIVE_GROUP);
			err++;
		}
		i = strcmp(rmp->owner_id, SAM_ARCHIVE_OWNER);
		if (i != 0) {
			error(0, 0, "Removable media file must have owner "
			    "id of (%s) to be used for archive.",
			    SAM_ARCHIVE_OWNER);
			err++;
		}
		if (err)
			error(1, 0, "Removable media file not compatible "
			    "for archive.", 0);
	}

	setarch.id.ino = fstat.st_ino;
	setarch.id.gen = fstat.gen;
	setarch.flags = 0;
	setarch.media = dt;
	setarch.copy = copy;
	setarch.access_time.tv_sec = fstat.st_atime;
	setarch.modify_time.tv_sec = fstat.st_mtime;
	setarch.ar.n_vsns = rmp->n_vsns;
	setarch.ar.version = rmp->version;
	setarch.ar.position = rmp->section[0].position;
	setarch.ar.file_offset = 0;	/* At the beginning of the file */
	memcpy(&setarch.ar.vsn, rmp->section[0].vsn,
	    sizeof (vsn_t));
	setarch.vp.ptr = (struct sam_vsn_section *)malloc
	    (sizeof (struct sam_vsn_section) * rmp->n_vsns);
	memcpy(setarch.vp.ptr, &rmp->section[0],
	    sizeof (rmp->section[0]) * rmp->n_vsns);
	if ((fd = open(file, O_RDWR)) < 0) {
		error(1, errno, "open(%s).", file);
	}
	if (ioctl(fd, F_SETARCH, &setarch) < 0) {
		error(1, errno, "ioctl(F_SETARCH).", 0);
	}
	exit(0);
}
