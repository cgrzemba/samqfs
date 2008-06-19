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

#pragma ident "$Revision: 1.30 $"

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
#include "aml/stager.h"
#include "aml/stager_defs.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Local headers. */
#include "stager_lib.h"
#include "stager_config.h"
#include "stager_shared.h"
#include "rmedia.h"

#include "stager.h"

/*
 * File system table.
 */
static struct {
	int			entries;
	struct sam_fs_status	*data;
} fileSystemTable = { 0, NULL };

static void makeRmFiles(char *mount_name);
static void createFileSystemMapFile(char *file_name);

/*
 * Initialize filesystem table.  Count filesystems and build table
 * for use by stager.
 */
int
InitFilesys(void)
{
	int i, j;
	size_t size;
	struct sam_fs_status *fsarray;
	int num;

	/*
	 * Get count of configured filesystems.
	 */
	num = GetFsStatus(&fsarray);
	if (num < 0) {
		FatalSyscallError(EXIT_NORESTART, HERE, "GetFsStatus", "");
	}

	/*
	 * We are not considering shared filesystems for which we
	 * are the shared clients.
	 */
	i = 0;
	while (i < num) {
		if (IS_SHARED_CLIENT(fsarray[i].fs_status)) {
			for (j = i; j < (num - 1); j++)
				fsarray[j] = fsarray[j+1];
			num--;
		} else {
			i++;
		}
	}

	fileSystemTable.entries = num;

	if (fileSystemTable.entries == 0) {
		/*
		 * No configured filesystems.
		 */
		SendCustMsg(HERE, 19004);
		return (EXIT_NORESTART);
	}

	/*
	 * Allocate filesystems table.
	 */
	if (fileSystemTable.data != NULL) {
		RemoveFileSystemMapFile();
		fileSystemTable.data = NULL;
	}
	size = fileSystemTable.entries * sizeof (struct sam_fs_status);
	SamMalloc(fileSystemTable.data, size);
	(void) memset(fileSystemTable.data, 0, size);

	for (i = 0; i < fileSystemTable.entries; i++) {
		fileSystemTable.data[i] = *(fsarray + i);
	}

	free(fsarray);		/* free memory allocated in GetFsStatus() */
	createFileSystemMapFile(SharedInfo->si_fileSystemFile);
	fileSystemTable.data =
	    (struct sam_fs_status *)MapInFile(SharedInfo->si_fileSystemFile,
	    O_RDWR, NULL);
	SharedInfo->si_numFilesys = fileSystemTable.entries;

	Trace(TR_DEBUG, "Filesystems initialized");
	return (0);
}

/*
 * Create removable media files in mounted filesystems.
 */
void
CreateRmFiles(void)
{
	int i;
	char *mount_name;
	struct sam_fs_status *fs;
	int count = 0;

	if (fileSystemTable.data != NULL) {
		for (i = 0; i < fileSystemTable.entries; i++) {
			fs = &fileSystemTable.data[i];

			if (IS_FILESYSTEM_MOUNTED(fs->fs_status)) {
				mount_name = fs->fs_mnt_point;
				if (*mount_name == NULL) {
					continue;
				}
				count++;
				makeRmFiles(mount_name);
			}
		}
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
	struct sam_fs_status *fs;
	char *mount_point_name = NULL;

	ReconfigLock();		/* wait on reconfig */

	for (i = 0; i < fileSystemTable.entries; i++) {
		fs = &fileSystemTable.data[i];

		if (fs->fs_eq == eq) {

			if (IS_FILESYSTEM_MOUNTED(fs->fs_status)) {
				mount_point_name = fs->fs_mnt_point;

			} else {
				struct sam_fs_info fi;
				if (GetFsInfo(fs->fs_name, &fi) == -1) {
					WarnSyscallError(HERE, "GetFsInfo",
					    fs->fs_name);
				}
				if (IS_FILESYSTEM_MOUNTED(fi.fi_status)) {
					SET_FLAG(fs->fs_status, FS_MOUNTED);
					strcpy(fs->fs_mnt_point,
					    fi.fi_mnt_point);
					mount_point_name = fs->fs_mnt_point;
				}
			}
			break;
		}
	}
	ReconfigUnlock();		/* allow reconfig */

	return (mount_point_name);
}

/*
 * Part of shutdown, unlink filesystem data's memory mapped file.
 */
void
RemoveFileSystemMapFile(void)
{
	size_t size;

	size = fileSystemTable.entries * sizeof (struct sam_fs_status);
	RemoveMapFile(SharedInfo->si_fileSystemFile,
	    (void *)fileSystemTable.data, size);
}

/*
 * Is filesystem mounted.
 */
boolean_t
IsFileSystemMounted(
	equ_t eq)
{
	int i;
	struct sam_fs_status *fs;
	boolean_t mounted = B_FALSE;

	ReconfigLock();		/* wait on reconfig */

	for (i = 0; i < fileSystemTable.entries; i++) {
		fs = &fileSystemTable.data[i];

		if (fs->fs_eq == eq) {
			if (GET_FLAG(fs->fs_status, FS_MOUNTED)) {
				mounted = B_TRUE;

			} else {
				struct sam_fs_info fi;
				if (GetFsInfo(fs->fs_name, &fi) == -1) {
					WarnSyscallError(HERE, "GetFsInfo",
					    fs->fs_name);
				}
				if (fi.fi_status & FS_MOUNTED) {
					SET_FLAG(fs->fs_status, FS_MOUNTED);
					mounted = B_TRUE;
				}
			}
			break;
		}
	}
	ReconfigUnlock();		/* allow reconfig */

	return (mounted);
}

/*
 *	Mount filesystem.
 */
void
MountFileSystem(
	char *name)
{
	int i;
	struct sam_fs_status *fs;

	ReconfigLock();		/* wait on reconfig */

	for (i = 0; i < fileSystemTable.entries; i++) {
		fs = &fileSystemTable.data[i];

		if (fs->fs_name == NULL) {
			continue;
		}
		if (strcmp(fs->fs_name, name) == 0) {
			struct sam_fs_info fi;

			Trace(TR_DEBUG, "Filesystem '%s' mounted", name);
			SET_FLAG(fs->fs_status, FS_MOUNTED);

			/*
			 * It may have changed, get mount point name.
			 */
			if (GetFsInfo(fs->fs_name, &fi) == -1) {
				WarnSyscallError(HERE, "GetFsInfo",
				    fs->fs_name);
			}
			strcpy(fs->fs_mnt_point, fi.fi_mnt_point);
			makeRmFiles(fs->fs_mnt_point);
			break;
		}
	}
	ReconfigUnlock();		/* allow reconfig */
}

/*
 * Unmount filesystem.
 */
void
UmountFileSystem(
	char *name)
{
	int i;
	struct sam_fs_status *fs;

	ReconfigLock();		/* wait on reconfig */

	for (i = 0; i < fileSystemTable.entries; i++) {
		fs = &fileSystemTable.data[i];

		if (strcmp(fs->fs_name, name) == 0) {
			CLEAR_FLAG(fs->fs_status, FS_MOUNTED);
			Trace(TR_DEBUG, "Filesystem '%s' unmounted", name);
			break;
		}
	}
	ReconfigUnlock();		/* allow reconfig */

	return;

}

/*
 * Create memory mapped file for filesystem data.
 */
static void
createFileSystemMapFile(
	char *file_name)
{
	int ret;

	ret = WriteMapFile(file_name, (void *)fileSystemTable.data,
	    fileSystemTable.entries * sizeof (struct sam_fs_status));
	if (ret != 0) {
		FatalSyscallError(EXIT_NORESTART, HERE,
		    "WriteMapFile", file_name);
	}
	SamFree(fileSystemTable.data);
}


/*
 * Make removable media files.  The .stager directory is created
 * in the root of the filesystem.  A removable media file is
 * created for each possible drive.
 */
static void
makeRmFiles(
	char *mount_name)
{
	upath_t fullpath;
	struct stat sb;
	int rc;
	char *filename;
	int i;
	int numdrives;

	sprintf(fullpath, "%s/%s", mount_name, RM_DIR);

	/*
	 * Check if directory doesn't exist or something is there
	 * but its not a directory.
	 */
	rc = stat(fullpath, &sb);
	if (rc != 0 || S_ISDIR(sb.st_mode) == 0) {
		if (rc == 0) {
			(void) unlink(fullpath); /* remove existing file */
		}

		if (mkdir(fullpath, DIR_MODE) != 0) {
			FatalSyscallError(EXIT_NORESTART, HERE,
			    "mkdir", fullpath);
		}
	}

	/*
	 * Check if directory created successfully.
	 */
	if (sam_archive(fullpath, "n") < 0) {
		WarnSyscallError(HERE, "sam_archive", fullpath);
	}

	filename = fullpath + strlen(fullpath);

	/*
	 * Get total number of drives for all libraries.
	 */
	numdrives = GetNumAllDrives();

	for (i = 0; i < numdrives; i++) {
		int fd;
		sprintf(filename, "/rm%d", i);

		if (stat(fullpath, &sb) != 0) {
			fd = open(fullpath, O_CREAT | O_TRUNC | SAM_O_LARGEFILE,
			    FILE_MODE);
			if (fd < 0) {
				WarnSyscallError(HERE, "open", fullpath);
				SendCustMsg(HERE, 19023, fullpath);
			} else {
				(void) close(fd);
			}
		}
	}
}
