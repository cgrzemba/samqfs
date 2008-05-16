/*
 * fs.c - read and accumulate archive copy data for file systems.
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
#pragma ident "$Revision: 1.8 $"

static char *_SrcFile = __FILE__;

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/bitmap.h>

#include "recycler_c.h"
#include "recycler_threads.h"

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#include "pub/rminfo.h"
#include "sam/types.h"
#include "aml/shm.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "aml/device.h"
#include "aml/catlib.h"
#include "aml/diskvols.h"
#include "sam/devnm.h"
#include "sam/resource.h"
#include "sam/names.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "sam/fs/ino.h"
#include "sam/fs/ino_ext.h"
#include "aml/id_to_path.h"

#include "recycler.h"

/* Global error which stops recycler from completing. */
extern boolean_t CannotRecycle;


/* Global table for archive media */
extern MediaTable_t ArchMedia;

static char *errmsg1 = "Error reading samfs inode file";

static int inodeAccumulate(union sam_di_ino *inode, int copy,
	MediaTable_t *vsn_table, MediaTable_t *datTable);
/* LINTED static unused */
static void timeToString(time_t time, char *buf, size_t buflen);

/*
 * Initialize file system information.
 */
struct sam_fs_info *
FsInit(
	int *numFs)
{
	int i;
	int count;
	int rval;
	size_t size;
	struct sam_fs_status *fs;
	struct sam_fs_status *fsarray;
	struct sam_fs_info *fsp;
	struct sam_fs_info *firstFs;
	boolean_t umountedFs;

	firstFs = NULL;
	*numFs = 0;

	count = GetFsStatus(&fsarray);
	if (count == -1) {
		Trace(TR_MISC, "Error: GetFsStatus failed");
		return (NULL);
	}

	/*
	 * Get the mount parameters and devices for each file system.
	 */
	size = count * sizeof (struct sam_fs_info);
	SamMalloc(firstFs, size);
	(void) memset(firstFs, 0, size);

	for (i = 0, fsp = firstFs, fs = fsarray; i < count; i++, fs++) {
		rval = GetFsInfo(fs->fs_name, fsp);
		if (rval == -1) {
			Trace(TR_MISC, "Error: GetFsInfo failed");
			return (NULL);
		}
		/*
		 * If this system is not the metadata server/writer, ignore it.
		 */
		if ((fsp->fi_config & MT_SHARED_READER) ||
		    (fsp->fi_status & FS_CLIENT)) {
			Trace(TR_MISC, "Ignore file system '%s'", fsp->fi_name);
		} else {
			fsp++;
		}
	}

	free(fsarray);
	count = fsp - firstFs;

	if (count == 0) {
		SamFree(firstFs);
		firstFs = NULL;
		Trace(TR_MISC, "Error: no file systems found");
	}

	/*
	 * All inode writable file systems must be mounted.
	 */
	umountedFs = B_FALSE;
	for (i = 0, fsp = firstFs; i < count; i++, fsp++) {
		if ((fsp->fi_status & FS_MOUNTED) == B_FALSE) {
			/* 'File system not mounted' */
			Log(20403, fsp->fi_name);
			umountedFs = B_TRUE;
		}
	}

	if (umountedFs == B_TRUE) {
		SamFree(firstFs);
		firstFs = NULL;
		count = 0;
	}

	*numFs = count;
	return (firstFs);
}

void
FsScan(
	Crew_t *crew,
	struct sam_fs_info *firstFs,
	int numFs,
	int pass,
	void *(*worker)(void *arg))
{
	int i;
	struct sam_fs_info *fsp;
	int status;
	WorkItem_t *request_arr;
	ScanArgs_t *scanarg_arr;

	fsp = firstFs;

	SamMalloc(request_arr, numFs * sizeof (WorkItem_t));
	(void) memset(request_arr, 0, numFs * sizeof (WorkItem_t));

	SamMalloc(scanarg_arr, numFs * sizeof (ScanArgs_t));
	(void) memset(scanarg_arr, 0, numFs * sizeof (ScanArgs_t));

	for (i = 0; i < numFs; i++) {
		WorkItem_t *request;
		ScanArgs_t *arg;

		arg = &scanarg_arr[i];
		request = &request_arr[i];

		arg->data = (void *)fsp;
		arg->pass = pass;

		request->wi_func = worker;
		request->wi_arg = (void *)arg;

		PthreadMutexLock(&crew->cr_mutex);
		if (crew->cr_first == NULL) {
			crew->cr_first = request;
			crew->cr_last = request;
		} else {
			crew->cr_last->wi_next = request;
			crew->cr_last = request;
		}
		crew->cr_count++;

		status = pthread_cond_signal(&crew->cr_go);
		if (status != 0) {
			Trace(TR_MISC, "Error: pthread_cond_signal failed %d",
			    errno);
			abort();
		}
		PthreadMutexUnlock(&crew->cr_mutex);

		fsp++;
	}

	/*
	 * Wait for workers to finish.
	 */
	while (crew->cr_count > 0) {
		PthreadCondWait(&crew->cr_done, &crew->cr_mutex);
	}
	PthreadMutexUnlock(&crew->cr_mutex);

	SamFree(request_arr);
	SamFree(scanarg_arr);
}


/*
 * Scan selected file system and accumulate VSN information.
 */
void *
FsAccumulate(
	void *arg)
{
	int fd;
	int ngot;
	int ninodes;
	int inode_i;
	int expected_ino;
	union sam_di_ino inodes[
	    (INO_BLK_FACTOR * INO_BLK_SIZE / sizeof (union sam_di_ino)) + 2];
	struct sam_fs_info *fsp;
	ScanArgs_t *scan_arg;
	int pass;
	int num_inodes;
	int tot_inodes;
	time_t start_time;
	time_t scan_time;
	char *mnt_point;
	char *fsname;

	scan_arg = (ScanArgs_t *)arg;

	fsp = scan_arg->data;
	pass = scan_arg->pass;
	start_time = time(NULL);

	expected_ino = 0;
	num_inodes = 0;
	tot_inodes = 0;

	fsname = fsp->fi_name;
	mnt_point = fsp->fi_mnt_point;

	fd = OpenInodesFile(mnt_point);
	if (fd < 0) {
		Trace(TR_MISC, "%s '%s', cannot open .inodes file, errno= %d",
		    errmsg1, mnt_point, errno);
		CANNOT_RECYCLE();
		return (NULL);
	}

	Trace(TR_MISC, "[%s] Start accumulating samfs inodes '%s'",
	    fsname, mnt_point);

	while ((ngot = read(fd, &inodes, INO_BLK_FACTOR * INO_BLK_SIZE)) > 0) {

		ninodes = ngot / sizeof (union sam_di_ino);

		for (inode_i = 0; inode_i < ninodes; inode_i++) {
			struct sam_disk_inode *dp;

			dp = &inodes[inode_i].inode.di;
			expected_ino++;

			/*
			 * Check for inodes we can skip.
			 */
			if (dp->id.ino != expected_ino)
				continue;

			if (dp->mode == 0)		/* unallocated */
				continue;

			if (dp->arch_status == 0)	/* not archived */
				continue;

			if (S_ISEXT(dp->mode))		/* inode extension */
				continue;

			tot_inodes++;
			num_inodes++;

			if (num_inodes > INODES_IN_PROGRESS) {
				Trace(TR_MISC,
				    "[%s] Accumulating inodes, %d done",
				    fsname, tot_inodes);
				num_inodes = 0;
			}

			/*
			 * If this is a request file, then flag all VSNs
			 * as active.
			 */
			if (pass == 1 && S_ISREQ(dp->mode)) {
				int idx;
				int rval;
				struct sam_rminfo rb;
				char *name;


				name = id_to_path(mnt_point, dp->id);
				/* handleRequestFile(mnt_point, dp->id); */
				rval = sam_readrminfo(name, &rb, sizeof (rb));
				if (rval < 0) {
					Trace(TR_MISC,
					    "%s '%s', cannot find pathname, "
					    "inode: %d.%d",
					    errmsg1, mnt_point, dp->id.ino,
					    dp->id.gen);

					CANNOT_RECYCLE();
					continue;
				}

				for (idx = 0; idx < rb.n_vsns; idx++) {
					MediaEntry_t *me;

					Trace(TR_MISC,
					    "Request file: %s vsn: '%s'",
					    name, rb.section[idx].vsn);

					me = MediaFind(&ArchMedia,
					    sam_atomedia(rb.media),
					    rb.section[idx].vsn);

					/*
					 * N.B. Bad indentation to meet cstyle
					 * requirements.
					 */
					if (me != NULL) {
					PthreadMutexLock(&me->me_mutex);
					me->me_files++;
					PthreadMutexUnlock(&me->me_mutex);

					} else {
						CANNOT_RECYCLE();
					}
				}

			} else {

				if (FsInodeHandle(&inodes[inode_i], NULL,
				    pass) == -1) {
					CANNOT_RECYCLE();
				}
			}
		}
	}
	(void) close(fd);

	scan_time = time(NULL) - start_time;
	Trace(TR_MISC,
	    "[%s] End samfs inode scan '%s', %d inodes accumulated in %ld secs",
	    fsname, mnt_point, tot_inodes, scan_time);

	return (NULL);
}

int
FsInodeHandle(
	union sam_di_ino *inode,
	MediaTable_t *datTable,
	int pass)
{
	int rval;
	int copy;
	int media_type;
	sam_disk_inode_t *dp;

	rval = 0;
	dp = &inode->inode.di;

	/*
	 * Check for inodes we can skip.
	 */
	if (dp->mode == 0) {		/* unallocated */
		return (0);
	}

	if (dp->arch_status == 0) {	/* not archived */
		return (0);
	}

	if (S_ISEXT(dp->mode)) {	/* inode extension */
		return (0);
	}

	/*
	 * Does the archive status flag look okay.
	 */
	if ((dp->arch_status & 0xf) != dp->arch_status) {
		Trace(TR_MISC, "%s '%s', bad arch_status 0x%x inode: %d.%d",
		    errmsg1, "???", dp->arch_status, dp->id.ino, dp->id.gen);
		return (-1);
	}

	/*
	 * Process each archive copy.
	 */
	for (copy = 0; copy < MAX_ARCHIVE; copy++) {
		/*
		 * If not first pass, skip removable media.
		 * Already accumulated it.
		 */
		if (pass != 1 &&
		    (dp->media[copy] & DT_CLASS_MASK) != DT_DISK) {
			continue;
		}

		/*
		 * If the archive copy isn't valid, skip it.
		 */
		if ((dp->arch_status & (1 << copy)) == 0) {
			continue;
		}

		/*
		 * Skip the archive copy if its on third-party media.
		 */
		if ((dp->media[copy] & DT_CLASS_MASK) == DT_THIRD_PARTY) {
			continue;
		}

		/*
		 * Check if valid media type.  Note that each copy has only one
		 * media type, even though it may have multiple VSNS.  Each VSN
		 * will be on the same media type.
		 */
		for (media_type = 0; dev_nm2mt[media_type].nm != NULL;
		    media_type++) {
			if (dp->media[copy] == dev_nm2mt[media_type].dt) {
				break;
			}
		}

		if (dev_nm2mt[media_type].nm == NULL) {
			Trace(TR_MISC, "Error: unknown archive media 0x%x "
			    "inode: %d.%d copy: %d",
			    dp->arch_status, dp->id.ino, dp->id.gen, copy);
			rval = -1;
			continue;
		}

		if (inode->inode.ar.image[copy].n_vsns <= 0) {
			Trace(TR_MISC,
			    "Error: invalid n_vsns field %d "
			    "inode: %d.%d copy: %d",
			    inode->inode.ar.image[copy].n_vsns,
			    dp->id.ino, dp->id.gen, copy);
			rval = -1;
			continue;

		} else if (inode->inode.ar.image[copy].n_vsns == 1) {

			rval = inodeAccumulate(inode, copy, &ArchMedia,
			    datTable);

		} else {
			rval = -1;
		}
	}
	return (rval);
}

void
FsCleanup(
	/* LINTED argument unused in function */
	struct sam_fs_info *firstFs,
	/* LINTED argument unused in function */
	int numFs)
{
}

/*
 * Accumulate into media entry the archive copy information for an inode.
 */
static int
inodeAccumulate(
	union sam_di_ino *inode,
	int copy,
	MediaTable_t *mediaTable,
	MediaTable_t *datTable)
{
	DiskVolumeSeqnum_t seqnum;
	DiskVolumeSeqnum_t max;
	int idx;
	MediaEntry_t *arch;
	MediaEntry_t *dat;

	arch = MediaFind(mediaTable, inode->inode.di.media[copy],
	    inode->inode.ar.image[copy].vsn);

	if (arch == NULL) {
		Trace(TR_MISC, "Error: failed to find media (archive) %s.%s",
		    sam_mediatoa(inode->inode.di.media[copy]),
		    inode->inode.ar.image[copy].vsn);
		return (-1);
	}

	dat = NULL;
	if (datTable != NULL) {
		dat = MediaFind(datTable, inode->inode.di.media[copy],
		    inode->inode.ar.image[copy].vsn);

		if (dat == NULL) {
			Trace(TR_MISC,
			    "Error: failed to find media (dat) %s.%s",
			    sam_mediatoa(inode->inode.di.media[copy]),
			    inode->inode.ar.image[copy].vsn);
			return (-1);
		}
	}

	Trace(TR_SAMDEV, "[%s.%s] Accumulate inode: %d.%d copy: %d pos: 0x%x",
	    sam_mediatoa(inode->inode.di.media[copy]),
	    inode->inode.ar.image[copy].vsn,
	    inode->inode.di.id.ino, inode->inode.di.id.gen, copy + 1,
	    inode->inode.ar.image[copy].position);

	max = mediaTable->mt_mapmin + mediaTable->mt_mapchunk - 1;
	PthreadMutexLock(&arch->me_mutex);

	if ((inode->inode.di.media[copy] == DT_DISK) &&
	    (arch->me_bitmap != NULL)) {
		/*
		 * Disk archive media.
		 */
		seqnum = inode->inode.ar.image[copy].position;

		if (seqnum >= mediaTable->mt_mapmin && seqnum <= max) {
			idx = seqnum - mediaTable->mt_mapmin;

			arch->me_files++;
			BT_SET(arch->me_bitmap, idx);

			if (dat != NULL) {
				PthreadMutexLock(&dat->me_mutex);
				dat->me_files++;
				BT_SET(dat->me_bitmap, seqnum);
				PthreadMutexUnlock(&dat->me_mutex);
			}
		}
	} else {
		/*
		 * Removable media.  Skip if media was relabeled since this
		 * copy was created.
		 */
		if (arch->me_label <=
		    inode->inode.ar.image[copy].creation_time) {
			arch->me_files++;
			if (dat != NULL) {
				PthreadMutexLock(&dat->me_mutex);
				dat->me_files++;
				PthreadMutexUnlock(&dat->me_mutex);
			}
		}
	}
	PthreadMutexUnlock(&arch->me_mutex);

	return (0);
}

static void
timeToString(
	time_t time,
	char *buf,
	size_t buflen)
{
	static char *tdformat = "%H:%M:%S";
	struct tm tm;
	time_t val;

	val = time;
	(void) strftime(buf, buflen, tdformat, localtime_r(&val, &tm));
}
