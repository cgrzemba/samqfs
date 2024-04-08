/*
 * filesys.c - provide access to mounted filesystem definitions
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

#pragma ident "$Revision: 1.19 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>

/* Solaris headers. */

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/shm.h"
#include "sam/nl_samfs.h"
#include "pub/stat.h"
#include "pub/lib.h"
#include "sam/syscall.h"
#include "sam/mount.h"
#include "sam/exit.h"
#include "sam/custmsg.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "sam/lib.h"
#include "aml/stager_defs.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Local headers. */
#include "stager_lib.h"
#include "stager_config.h"
#include "stager_shared.h"

/*
 *	File system table.
 */
static struct sam_fs_status *fileSystem = NULL;

static void updateFileSystems();

/*
 * Initialize filesystem table.  Map in filesystems.
 */
void
MapInFileSystem(void)
{
	fileSystem = (struct sam_fs_status *)MapInFile(
	    SharedInfo->si_fileSystemFile, O_RDWR, NULL);
	if (fileSystem == NULL) {
		FatalSyscallError(EXIT_NORESTART, HERE, "MapInFile",
		    SharedInfo->si_fileSystemFile);
	}
}

/*
 * Get mount point name for specified filesystem equipment number.
 */
char *
GetMountPointName(
	equ_t eq)
{
	int i;
	char *mount_point_name = NULL;

	for (i = 0; i < SharedInfo->si_numFilesys; i++) {

		if (fileSystem[i].fs_eq == eq) {

			/*
			 * Filesystem is unmounted, update table.
			 */
			if (IS_FILESYSTEM_UMOUNTED(fileSystem[i].fs_status)) {
				updateFileSystems();
			}

			if (IS_FILESYSTEM_MOUNTED(fileSystem[i].fs_status)) {
				/*
				 * If don't already have it, get mount
				 * point name.
				 */
				if (fileSystem[i].fs_mnt_point[0] == '\0') {
					struct sam_fs_info fi;
					if (GetFsInfo(fileSystem[i].fs_name,
					    &fi) == -1) {
						WarnSyscallError(HERE,
						    "GetFsInfo",
						    fileSystem[i].fs_name);
					}
					strcpy(fileSystem[i].fs_mnt_point,
					    fi.fi_mnt_point);
				}
				mount_point_name = fileSystem[i].fs_mnt_point;
			}
			break;
		}
	}
	return (mount_point_name);
}

/*
 * Get file system name for specified file system equipment number.
 */
char *
GetFsName(
	equ_t eq)
{
	int i;
	char *fs_name = NULL;

	for (i = 0; i < SharedInfo->si_numFilesys; i++) {
		if (fileSystem[i].fs_eq == eq) {
			fs_name = fileSystem[i].fs_name;
			break;
		}
	}
	return (fs_name);
}

/*
 * Update file system table.
 */
static void
updateFileSystems(void)
{
	int i;
	int num;
	struct sam_fs_status *fsarray;

	num = GetFsStatus(&fsarray);
	ASSERT(num == SharedInfo->si_numFilesys);

	for (i = 0; i < num; i++) {
		fileSystem[i] = *(fsarray + i);
	}
	free(fsarray);
}
