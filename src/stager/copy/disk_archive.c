/*
 * disk_archive.c - provide access to samfs disk archiving
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

#pragma ident "$Revision: 1.12 $"

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <sys/shm.h>

/* Solaris headers. */

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/shm.h"
#include "sam/sam_malloc.h"
#include "sam/archiver.h"

/* Local headers. */
#include "stager_lib.h"
#include "stager_shared.h"
#include "disk_archive.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */


/* Disk VSN table. */
static struct {
	size_t			alloc;
	DiskVolumeInfo_t	*data;
} diskVolumeTable = { 0, NULL };

/*
 * Map in disk volumes.
 */
void
MapInDiskVolumes(void)
{
	diskVolumeTable.data = (DiskVolumeInfo_t *)MapInFile(
	    SharedInfo->diskVolumesFile, O_RDWR, &diskVolumeTable.alloc);
}

/*
 * Find a disk volume table entry.
 */
DiskVolumeInfo_t *
FindDiskVolume(
	char *label)
{
	DiskVolumeInfo_t *vol;
	DiskVolumeInfo_t *found = NULL;
	size_t offset;

	for (offset = 0; offset < diskVolumeTable.alloc;
	    offset += STRUCT_RND(sizeof (DiskVolumeInfo_t) + vol->path_len)) {
/* LINTED pointer cast may result in improper alignment */
		vol = (DiskVolumeInfo_t *)((char *)diskVolumeTable.data +
		    offset);
		if (strcmp(label, vol->label) == 0) {
			found = vol;
			break;
		}
	}
	return (found);
}
