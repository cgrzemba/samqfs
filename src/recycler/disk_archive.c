/*
 * disk_archive.c - recycling support for samfs disk archives
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

#pragma ident "$Revision: 1.46 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* POSIX headers. */
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <sys/mnttab.h>
#include <assert.h>

/* Solaris headers. */
#include <sys/mman.h>
#include <syslog.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/diskvols.h"
#include "sam/nl_samfs.h"
#include "aml/sam_rft.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/custmsg.h"
#include "sam/uioctl.h"
#include "aml/tar.h"
#include "sam/sam_trace.h"
#include <sam/fs/bswap.h>

/* Honeycomb headers. */
#include "hc.h"
#include "hcclient.h"

/* Local headers. */
#include "recycler.h"

typedef struct dirHandle {
	SamrftImpl_t	*rft;
	union {
		DIR	*ptr;
		int	idx;
	} dirp;
} dirHandle_t;

static struct DiskVolumeInfo *diskVolume = NULL;
static SamrftImpl_t *rft = NULL;
static hc_session_t *session = NULL;
static upath_t fullpath;
static int numRecycledFiles = 0;
static boolean_t ignoreRecycling;
static boolean_t traceDebug = B_FALSE;
static boolean_t traceMisc = B_FALSE;
static union sam_di_ino
	inodes[(INO_BLK_FACTOR * INO_BLK_SIZE / sizeof (union sam_di_ino)) + 2];

/*
 * Maximum number of sequence numbers per filesystem to sort
 * in one pass.  A larger number will use more memory but results
 * in fewer filesystem scans.
 */
#define	SEQNUM_BLK_FACTOR	10000
#define	ACTIVE_LIST_CHUNKSIZE	100

typedef struct {
	struct sam_fs_info *fsp;	/* file system */
	sam_id_t	id;		/* inode */
	uint_t		copy;		/* archive copy */
	u_longlong_t	offset;		/* location of copy in archive file */
} File_t;

typedef struct seqEntry {
	boolean_t has_active_files;	/* TRUE if disk archive file in use */
	boolean_t drain_candidate;	/* candidate for rearchiving */
					/*    active files */
	long long size_active_files;	/* amount of space in use */
	long long tarfile_size;		/* size of archive tarfile */
	/*
	 */
	size_t	alloc_active_list;	/* size of allocated list */
	size_t	num_active_list;	/* number of active files in list */
	File_t	*active_list;
} seqEntry_t;

static struct {
	int64_t		entries;	/* number of entries per filesystem */
	size_t		alloc;		/* total size of allocated space per */
					/*    filesystem */
	seqEntry_t	*data;
} seqNumbers = { SEQNUM_BLK_FACTOR, 0, NULL };

static void setDiskArchiveStats(VSN_TABLE *vsn);
static void insertDiskVol(ROBOT_TABLE *robot, vsn_t vsn);
static void recycle(VSN_TABLE *vsn);
static void recycleSeqNumbers(ROBOT_TABLE *robot, VSN_TABLE *vsn);
static void accumulateSeqNumbers(VSN_TABLE *vsn, struct sam_fs_info *fsp,
	seqEntry_t *buffer, DiskVolumeSeqnum_t seqnum);
static long long removeSeqNumbers(DiskVolumeSeqnum_t seqnum, VSN_TABLE *vsn,
	SeqNumsInUse_t *seqNumsInUse);
static long long selectDrainCandidates(DiskVolumeSeqnum_t seqnum,
	long long dataquantity, boolean_t limitquantity,
	int obs, char *volName);
static void drainSeqNumbers(DiskVolumeSeqnum_t seqnum, char *volName);
static long long emptyTarFile(DiskVolumeSeqnum_t seqnum, char *volName);
static long long statTarFile(DiskVolumeSeqnum_t seqnum);
static long long deleteObject(DiskVolumeSeqnum_t seqnum, char *volName);
static int deleteMetadataRecord(hc_oid *oid);
static void unlinkFile(char *file_name);
static char *genSeqNumberFileName(DiskVolumeSeqnum_t seqnum);
static char *gobbleName(char *path, char d_name[1]);
static DiskVolumeSeqnum_t getDiskVolumeSeqnum(char *volName,
	DiskVolumeInfo_t *dv);
static int updateSpaceUsed(char *volName, long long removedSpace);
static void addActiveList(seqEntry_t *buf, struct sam_fs_info *fsp,
	union sam_di_ino *inode, int copy, u_longlong_t offset);
static void initActiveList(seqEntry_t *buf);
static void rearchiveCopy(char *fsname, sam_id_t id, int copy);
static void clearDiskVolsFlag(char *volname, int flag);

/*
 * Assign disk volume to robot (archive set) table.
 */
void
AssignDiskVol(
	int robot_index,
	VSN_TABLE *vsn)
{
	int i;
	DiskVolsDictionary_t *diskvols;
	ROBOT_TABLE *robot;
	boolean_t already_inserted;
	char *volname;

	robot = &ROBOT_table[robot_index];

	if (robot == NULL || vsn == NULL) {
		return;
	}

	diskVolume = NULL;
	diskvols = DiskVolsGetHandle(DISKVOLS_VSN_DICT);

	if (diskvols == NULL) {
		return;
	}

	/*
	 * Check to make sure disk volume exists in dictionary.
	 */
	(void) diskvols->Get(diskvols, vsn->vsn, &diskVolume);
	if (diskVolume == NULL) {
		DiskVolsRelHandle(DISKVOLS_VSN_DICT);
		return;
	}

	/*
	 * Check to make sure disk volume doesn't already exist
	 * in this robot's list of disk volumes.
	 */
	already_inserted = B_FALSE;
	for (i = 0; i < robot->diskvols.entries; i++) {
		volname = (char *)&robot->diskvols.key[i];
		if (strcmp(vsn->vsn, volname) == 0) {
			already_inserted = B_TRUE;
			break;
		}
	}

	if (already_inserted == B_FALSE) {
		char *host;
		DiskVolumeSeqnum_t maxSeqnum = -1;

		host = DiskVolsGetHostname(diskVolume);
		rft = SamrftConnect(host);
		if (rft == NULL) {
			if (host == NULL) {
				host = "";
			}
			Trace(TR_MISC, "Sam rft connection to '%s' failed",
			    host);
			DiskVolsRelHandle(DISKVOLS_VSN_DICT);
			return;
		}

		/* Set space and capacity for disk volume. */
		setDiskArchiveStats(vsn);

		/* Set is_recycling flag for disk volume if 'c' flag set. */
		if (diskVolume->DvFlags & DV_recycle) {
			vsn->is_recycling = 1;
		}

		/*
		 * Get disk volume's sequence number.  Accumulate in use
		 * sequence numbers for disk volume across all filesystems.
		 */
		maxSeqnum = getDiskVolumeSeqnum(vsn->vsn, diskVolume);
		if (maxSeqnum == -1) {
			Trace(TR_MISC, "Failed to get disk "
			    "volume seqnum for %s", vsn->vsn);
		}

		vsn->maxSeqnum = maxSeqnum;

		/*
		 * Insert volume in robot (archive set) table.
		 */
		insertDiskVol(robot, vsn->vsn);

		vsn->disk_archsets++;

		/*
		 * Volume may belong to multiple archive sets.
		 * Pick parameters from the first one.
		 */
		if (vsn->robot == NULL) {
			vsn->robot = robot;
			/*
			 * Set dev so VSN table can be sorted by archset.
			 */
			vsn->dev = (dev_ent_t *)robot;
		}

		SamrftDisconnect(rft);
		rft = NULL;
	}
	DiskVolsRelHandle(DISKVOLS_VSN_DICT);
}


/*
 * Find and recycle any disk archive vsn which are marked for recycling.
 * Always log remove actions as if recycling is occurring.
 */
void
RecycleDiskArchives(void)
{
	int vsn_i;

	if (cannot_recycle) {
		return;
	}

	if (*TraceFlags & (1 << TR_debug)) {
		traceDebug = B_TRUE;
	}

	if (*TraceFlags & (1 << TR_misc)) {
		traceMisc = B_TRUE;
	}

	for (vsn_i = 0; vsn_i < table_used; vsn_i++) {
		VSN_TABLE *vsn_entry = &vsn_table[vsn_permute[vsn_i]];

		/*
		 * If it's not disk media, skip it.
		 */
		if (IS_DISK_MEDIA(vsn_entry->media) == FALSE) {
			continue;
		}

		ignoreRecycling = B_FALSE;

		/*
		 * Disk media, check if it needs recycling.  Recycle
		 * this disk volume if is_recycling, 'c', flag is set.
		 */
		if (vsn_entry->needs_recycling == FALSE &&
		    vsn_entry->is_recycling == FALSE) {

			ignoreRecycling = B_TRUE;
		}

		recycle(vsn_entry);
	}
}

/*
 * Set file system information, space and capacity, for
 * a disk volume.
 */
static void
setDiskArchiveStats(
	VSN_TABLE *vsn)
{
	int rval;
	boolean_t ready;

	if (vsn == NULL || diskVolume == NULL) {
		return;
	}

	ready = DiskVolsIsAvail(vsn->vsn, diskVolume, B_FALSE, DVA_recycler);
	if (ready == B_TRUE) {
		vsn->blocksize = 1024;
		rval = DiskVolsGetSpaceUsed(vsn->vsn, diskVolume, rft,
		    &vsn->written);
		if (rval != 0) {
			Trace(TR_ERR, "'%s' Get space used failed, err= %d",
			    vsn->vsn, rval);
		}
		Trace(TR_MISC, "[%s] Get space used: %lld (%s)",
		    vsn->vsn, vsn->written,
		    StrFromFsize(vsn->written, 3, NULL, 0));
	}
}

/*
 * Insert volume in robot (archive set) table.
 */
static void
insertDiskVol(
	ROBOT_TABLE *robot,
	vsn_t vsn)
{
	DiskVolumeInfo_t *insert;
	int index;
	int size;
	int old_alloc;

	ASSERT(robot->diskvols.data != NULL);

	if (robot->diskvols.entries >= robot->diskvols.alloc) {
		vsn_t *key;
		DiskVolumeInfo_t **data;

		old_alloc = robot->diskvols.alloc;
		robot->diskvols.alloc += DICT_CHUNKSIZE;

		/*
		 * Increase size of disk volume name space.
		 */
		size = robot->diskvols.alloc * sizeof (vsn_t);
		SamRealloc(robot->diskvols.key, size);
		key = robot->diskvols.key + old_alloc;
		memset(key, 0, DICT_CHUNKSIZE * sizeof (vsn_t));


		/*
		 * Increase size of disk volume dictionary element space.
		 */
		size = robot->diskvols.alloc * sizeof (DiskVolumeInfo_t *);
		SamRealloc(robot->diskvols.data, size);
		data = robot->diskvols.data + old_alloc;
		memset(data, 0, DICT_CHUNKSIZE * sizeof (DiskVolumeInfo_t *));
	}

	SamMalloc(insert, sizeof (DiskVolumeInfo_t));
	(void) memset(insert, 0, sizeof (DiskVolumeInfo_t));
	memcpy(insert, diskVolume, sizeof (DiskVolumeInfo_t));

	index = robot->diskvols.entries;
	memcpy(&robot->diskvols.key[index], vsn, sizeof (vsn_t));
	robot->diskvols.data[index] = insert;
	robot->diskvols.entries++;
}

/*
 * Recycle vsn.
 */
static void
recycle(
	VSN_TABLE *vsn)
{
	char *host;
	ROBOT_TABLE *robot = NULL;
	struct DiskVolsDictionary *diskvols;

	/*
	 * Set archive set.
	 */
	robot = vsn->robot;

	if (robot == NULL) {
		Trace(TR_ERR,
		    "Could not find disk archive set for volume '%s'",
		    vsn->vsn);
		return;
	}

	Trace(TR_MISC,
	    "[%s] Recycling disk volume minobs: %d%% dataquantity: %s (%d)",
	    vsn->vsn, robot->obs,
	    StrFromFsize(robot->dataquantity, 3, NULL, 0),
	    robot->limit_quantity);

	Trace(TR_MISC, "[%s] Needs recycling: %d ignore: %d candidate: %d",
	    vsn->vsn, vsn->needs_recycling, ignoreRecycling, vsn->candidate);

	/* Recycling disk archive set */
	cemit(TO_FILE, 0, 20308, robot->name, vsn->vsn);

	if (IS_DISK_HONEYCOMB(vsn->media)) {
		/*
		 * Metadata records for the following list of OIDs
		 * will be deleted.
		 */
		cemit(TO_FILE, 0, 20354);
	} else {
		/*
		 * The following list of files and directories will be removed.
		 */
		cemit(TO_FILE, 0, 20355);
	}

	if (ignoreRecycling) {
		cemit(TO_FILE, 0, 20309);
	}

	diskVolume = NULL;
	diskvols = DiskVolsGetHandle(DISKVOLS_VSN_DICT);
	if (diskvols != NULL) {
		(void) diskvols->Get(diskvols,
		    (char *)&vsn->vsn[0], &diskVolume);
	}
	if (diskVolume == NULL) {
		Trace(TR_ERR, "Could not find disk volume '%s'",
		    (char *)&vsn->vsn[0]);
		goto out;
	}

	if (IS_DISK_HONEYCOMB(vsn->media)) {
		hcerr_t hcerr;

		hcerr = hc_init(malloc, free, realloc);
		if (hcerr != HCERR_OK) {
			SendCustMsg(HERE, 4057, hcerr, hc_decode_hcerr(hcerr));
			goto out;
		}

		hcerr = hc_session_create_ez(diskVolume->DvAddr,
		    diskVolume->DvPort, &session);
		if (hcerr != HCERR_OK) {
			SendCustMsg(HERE, 4057, hcerr, hc_decode_hcerr(hcerr));
			goto out;
		}
	}

	host = DiskVolsGetHostname(diskVolume);
	rft = SamrftConnect(host);
	if (rft == NULL) {
		if (host == NULL) {
			host = "";
		}
		Trace(TR_ERR, "Sam rft connection to '%s' failed", host);
		goto out;
	}

	numRecycledFiles = 0;

	/*
	 * Recycle sequence numbers.  Any sequence number not
	 * being used are mapped to files which can be removed from the
	 * disk archive volume.
	 */
	recycleSeqNumbers(robot, vsn);

	if (rft != NULL) {
		SamrftDisconnect(rft);
		rft = NULL;
	}

	/*
	 * Terminate and free honeycomb's global session.
	 */
	if (session != NULL) {
		hc_session_free(session);
		hc_cleanup();
		session = NULL;
	}

	if (diskVolume->DvFlags & DV_remote) {
		cemit(TO_FILE, 0, 20302, numRecycledFiles, vsn->vsn,
		    diskVolume->DvHost, diskVolume->DvPath);
	} else {
		cemit(TO_FILE, 0, 20303, numRecycledFiles, vsn->vsn,
		    diskVolume->DvPath);
	}

	if (robot->mail && ignoreRecycling == B_FALSE) {
		char *fmt;
		char *subject;
		char *mailaddr;
		FILE *mailfile;

		subject = catgets(catfd, SET, 20257, "Message number 20257");
		mailaddr = robot->mailaddress;

		mailfile = Mopen(mailaddr);
		if (mailfile != NULL) {
			if (diskVolume->DvFlags & DV_remote) {
				fmt = catgets(catfd, SET, 20302,
				    "Message number 20302");
				fprintf(mailfile, fmt, numRecycledFiles,
				    vsn->vsn, diskVolume->DvHost,
				    diskVolume->DvPath);
				fprintf(mailfile, "\n");
			} else {
				fmt = catgets(catfd, SET, 20303,
				    "Message number 20303");
				fprintf(mailfile, fmt, numRecycledFiles,
				    vsn->vsn, diskVolume->DvPath);
				fprintf(mailfile, "\n");
			}
			Msend(&mailfile, mailaddr, subject);
		}
	}
out:
	DiskVolsRelHandle(DISKVOLS_VSN_DICT);
}

/*
 * Recycle sequence numbers.  Any sequence number not
 * being used are mapped to files which can be removed from the
 * disk archive volume.
 */
static void
recycleSeqNumbers(
	ROBOT_TABLE *robot,
	VSN_TABLE *vsn)
{
	int i;
	DiskVolumeSeqnum_t curSeqnum;
	seqEntry_t *buffer;
	struct sam_fs_info *fsp;
	long long drained_space;
	long long removed_space;
	long long dataquantity = robot->dataquantity;
	boolean_t limitquantity = robot->limit_quantity;
	int obs = robot->obs;
	DiskVolumeSeqnum_t maxSeqnum;
	SeqNumsInUse_t *seqNumsInUse = NULL;

	maxSeqnum = vsn->maxSeqnum;
	if (maxSeqnum == -1) {
		return;
	}

	seqNumbers.entries = SEQNUM_BLK_FACTOR;
	seqNumbers.alloc = 0;
	seqNumbers.data = NULL;

	if (maxSeqnum + 1 < seqNumbers.entries) {
		seqNumbers.entries = maxSeqnum + 1;
	}

	seqNumbers.alloc = sizeof (seqEntry_t) * seqNumbers.entries;
	SamMalloc(seqNumbers.data, seqNumbers.alloc);

	curSeqnum = 0;
	buffer = seqNumbers.data;
	while (curSeqnum <= maxSeqnum && dataquantity > 0) {
		memset(buffer, 0, seqNumbers.alloc);
		for (i = 0, fsp = first_fs; i < num_fs; i++, fsp++) {
			accumulateSeqNumbers(vsn, fsp, buffer, curSeqnum);
			seqNumsInUse = GetSeqNumsInUse(vsn->vsn, fsp->fi_name,
			    seqNumsInUse);
		}

		/*
		 * Recycle sequence numbers.  Two passes.
		 *
		 * Remove sequence numbers.  A disk archive file associated
		 * with a sequence number that is not being used can be
		 * removed from the disk archive volume.
		 *
		 * Drain sequence numbers. For any sequence number containing
		 * active files, check if files should be rearchived.
		 */

		removed_space = removeSeqNumbers(curSeqnum, vsn, seqNumsInUse);

		if (ignoreRecycling == B_FALSE && removed_space > 0) {
			int rval;

			rval = updateSpaceUsed(vsn->vsn, removed_space);
			if (rval != 0) {
				Trace(TR_ERR,
				    "Error: failed to update space "
				    "used in disk volume seqnum.");
				SamFree(seqNumsInUse);
				return;
			}

			if (diskVolume->DvFlags & DV_archfull) {
				clearDiskVolsFlag(vsn->vsn, DV_archfull);
			}

			if (diskVolume->DvFlags & DV_recycle) {
				clearDiskVolsFlag(vsn->vsn, DV_recycle);
			}
		}

		if (IS_DISK_FILESYS(vsn->media)) {
			drained_space = selectDrainCandidates(curSeqnum,
			    dataquantity, limitquantity, obs, vsn->vsn);
			if (drained_space > 0) {
				drainSeqNumbers(curSeqnum, vsn->vsn);
				if (limitquantity == TRUE) {
					dataquantity -= drained_space;
				}
			}
		}

		/*
		 * Free the active list used for current loop iteration.
		 */
		for (i = 0; i < seqNumbers.entries; i++) {
			if (buffer[i].active_list != NULL) {
				SamFree(buffer[i].active_list);
			}
		}

		if (dataquantity <= 0) {
			Trace(TR_MISC, "[%s] Data quantity limit reached",
			    vsn->vsn);
			break;
		}

		curSeqnum += seqNumbers.entries;

		if (curSeqnum + seqNumbers.entries > maxSeqnum) {
			seqNumbers.entries = maxSeqnum - curSeqnum + 1;
		}
	}

	/*
	 * Free workspace used.
	 */
	SamFree(seqNumsInUse);
	SamFree(seqNumbers.data);
}

/*
 * Accumulate disk volume sequence numbers for specified volume.
 */
static void
accumulateSeqNumbers(
	VSN_TABLE *vsn,
	struct sam_fs_info *fsp,
	seqEntry_t *buffer,
	DiskVolumeSeqnum_t min)
{
	int fd;
	int samfd;
	int ngot;
	int ninodes;
	int inode_i;
	int copy;
	int idx;
	union sam_di_ino *inode;
	char *fs_name;
	media_t media;
	DiskVolumeSeqnum_t max;
	DiskVolumeSeqnum_t seqnum;

	fs_name = fsp->fi_mnt_point;
	max = min + seqNumbers.entries - 1;

	fd = OpenInodesFile(fs_name);
	if (fd < 0) {
		return;
	}
	samfd = open(fs_name, O_RDONLY);
	if (samfd < 0) {
		return;
	}

	Trace(TR_MISC, "[%s] Accumulate sequences: '%s' range: %llx-%llx",
	    vsn->vsn, fs_name, min, max);

	while ((ngot = read(fd, &inodes, INO_BLK_FACTOR * INO_BLK_SIZE)) > 0) {
		ninodes = ngot / sizeof (union sam_di_ino);

		for (inode_i = 0; inode_i < ninodes; inode_i++) {
			sam_disk_inode_t *dp;
			sam_arch_inode_t *ar;

			inode = &inodes[inode_i];
			dp = &inode->inode.di;
			ar = &inode->inode.ar;

			for (copy = 0; copy < MAX_ARCHIVE; copy++) {
				if ((dp->arch_status & (1 << copy)) == 0)
					continue;

				if (IS_DISK_MEDIA(dp->media[copy]) == FALSE)
					continue;

				if (strcmp(ar->image[copy].vsn, vsn->vsn) != 0)
					continue;

				seqnum = ar->image[copy].position;
				if (seqnum < min || seqnum > max)
					continue;

				media = dp->media[copy];

				/*
				 * Since id_to_path() call is very expensive,
				 * make sure that the call is required first.
				 */
		/* N.B. Bad indentation to meet cstyle requirements. */
				if (traceDebug == B_TRUE) {
				if (IS_DISK_HONEYCOMB(media)) {
					Trace(TR_DEBUG,
					    "[%s] Active file: '%s' "
					    "meta: '%s:%llx'",
					    vsn->vsn,
					    id_to_path(fs_name, dp->id),
					    vsn->vsn, seqnum);
				} else {
					Trace(TR_DEBUG,
					    "[%s] Active file: '%s' "
					    "tar: '%s'",
					    vsn->vsn,
					    id_to_path(fs_name, dp->id),
					    genSeqNumberFileName(seqnum));
				}
				}

				idx = seqnum - min;
				buffer[idx].has_active_files = B_TRUE;
				buffer[idx].size_active_files +=
				    inode->inode.di.rm.size + TAR_RECORDSIZE;

				/*
				 * Add entry to active file list for this
				 * sequence number.
				 */
				addActiveList(&buffer[idx], fsp, inode, copy,
				    ar->image[copy].file_offset);

			/* N.B. Bad indentation to meet cstyle requirements. */
			if (IS_DISK_FILESYS(media) &&
			    buffer[idx].tarfile_size == 0) {

			buffer[idx].tarfile_size =
			    statTarFile(seqnum);

			/*
			 * If tarball not found and check
			 * expired flag is set, log expired
			 * archive image and set the cannot
			 * recycle flag.
			 */
			if (buffer[idx].tarfile_size == -1 &&
			    check_expired) {
				char *pathname;
				char msgbuf[1024];
				char *name;

				pathname = id_to_path(fs_name,
				    inode->inode.di.id);
				emit(TO_ALL, LOG_ERR, 20218, pathname);

				/* Send sysevent to generate SNMP trap */
				sprintf(msgbuf, GetCustMsg(20218), pathname);
				(void) PostEvent(RY_CLASS, "ExpiredArch", 20218,
				    LOG_ERR, msgbuf,
				    NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);

				(void) DiskVolsGenFileName(seqnum,
				    fullpath, sizeof (fullpath));
				name = gobbleName(diskVolume->DvPath, fullpath);

				emit(TO_ALL, LOG_ERR, 20221, name, copy + 1);

				cannot_recycle = TRUE;
			}
			}
		vsn->has_active_files = TRUE;
		vsn->active_files++;
		}
		}
	}
	(void) close(fd);
	(void) close(samfd);
}

/*
 * Remove sequence numbers.  A disk archive file associated
 * with a sequence number that is not being used can be
 * removed from the disk archive volume.
 */
static long long
removeSeqNumbers(
	DiskVolumeSeqnum_t seqnum,
	VSN_TABLE *vsn,
	SeqNumsInUse_t *seqNumsInUse
)
{
	int i;
	long long removed_space = 0;
	char *volName;

	volName = vsn->vsn;

	Trace(TR_MISC, "[%s] Select remove candidates: %llx-%llx",
	    volName, seqnum, seqnum + seqNumbers.entries - 1);

	for (i = 0; i < seqNumbers.entries; i++) {
		if (seqNumbers.data[i].has_active_files == B_FALSE &&
		    !IsSeqNumInUse(i + seqnum, seqNumsInUse)) {
			if (IS_DISK_HONEYCOMB(vsn->media)) {
				removed_space +=
				    deleteObject(i + seqnum, volName);
			} else {
				removed_space +=
				    emptyTarFile(i + seqnum, volName);
			}
		}
	}

	Trace(TR_MISC, "[%s] Recycled space during remove: %s",
	    volName, StrFromFsize(removed_space, 1, NULL, 0));

	return (removed_space);
}

static long long
selectDrainCandidates(
	DiskVolumeSeqnum_t seqnum,
	long long dataquantity,		/* limits amount of data to rearchive */
	boolean_t limitquantity,
	int obs,
	char *volName)
{
	int i;
	int percent_junk;
	long long active;
	long long written;
	long num_candidates = 0;
	long long drained_space = 0;

	Trace(TR_MISC, "[%s] Select drain candidates: %llx-%llx",
	    volName, seqnum, seqnum + seqNumbers.entries - 1);

	i = 0;
	while (i < seqNumbers.entries && dataquantity > 0) {

		if (seqNumbers.data[i].has_active_files == B_FALSE) {
			/*
			 * No active files in tarball.
			 * Continue to next sequence number.
			 */
			i++;
			continue;
		}

		/*
		 * Size of selected tarball.
		 */
		written = seqNumbers.data[i].tarfile_size;

		if (written == 0) {
			int j;
			char *fsname;
			sam_id_t id;
			int copy;
			char *pathname;
			int numFiles;
			File_t *file;

			/*
			 * Tarball is empty, skipping it.
			 * It was suppose to contain the following
			 * list of files.
			 */
			emit(TO_FILE, 0, 20324, volName,
			    genSeqNumberFileName(i + seqnum));

			numFiles = seqNumbers.data[i].num_active_list;
			for (j = 0; numFiles; j++) {
				file = &seqNumbers.data[i].active_list[j];

				fsname = file->fsp->fi_mnt_point;
				id = file->id;
				copy = file->copy;
				pathname = id_to_path(fsname, id);
				emit(TO_FILE, 0, 20325, copy + 1, pathname);
			}
			i++;
			continue;	/* Next sequence number */
		}

		/*
		 * Found drain candidate.
		 */

		/*
		 * Amount of data which would need to be rearchived so as
		 * clear the tarball of useful data.
		 */
		active = seqNumbers.data[i].size_active_files;

		if (written == -1) {
			/*
			 * Tarball not found.  If check expired flag is set,
			 * these file have already been logged.  Ignore them.
			 */
			percent_junk = 0;
		} else {
			percent_junk = (written - active) * 100LL / written;
		}
		Trace(TR_MISC, "[%s] Check tar file: '%s' junk: %d%%",
		    volName, genSeqNumberFileName(i + seqnum), percent_junk);

		if (percent_junk > obs) {
			/*
			 * Mark drain candidate.
			 */
			seqNumbers.data[i].drain_candidate = B_TRUE;
			if (limitquantity == TRUE) {
				dataquantity -= active;
			}
			drained_space += active;
			num_candidates++;

			Trace(TR_MISC,
			    "[%s] Drain tar active: %lld written: %lld "
			    "dataquantity: %lld total drained: %s",
			    volName, active, written, dataquantity,
			    StrFromFsize(drained_space, 3, NULL, 0));
		}
		i++;	/* Next sequence number */
	}

	Trace(TR_MISC, "[%s] Recycled space during drain: %s,  %ld candidates",
	    volName, StrFromFsize(drained_space, 1, NULL, 0),
	    num_candidates);

	return (drained_space);
}

/*
 * Drain sequence numbers.
 */
static void
drainSeqNumbers(
	DiskVolumeSeqnum_t seqnum,
	char *volName)
{
	int i, j;
	char *fsname;
	sam_id_t id;
	int copy;
	u_longlong_t offset;
	char *pathname;
	int numFiles;
	File_t *file;
	long long space;
	long long capacity;
	long long recycled_space = 0;

	Trace(TR_MISC, "[%s] Drain sequences: %llx-%llx",
	    volName, seqnum, seqnum + seqNumbers.entries - 1);

	for (i = 0; i < seqNumbers.entries; i++) {
		if (seqNumbers.data[i].drain_candidate) {
			Trace(TR_MISC, "[%s] Drain tar: '%s'",
			    volName, genSeqNumberFileName(i + seqnum));
			numRecycledFiles++;

			numFiles = seqNumbers.data[i].num_active_list;
			for (j = 0; j < numFiles; j++) {
				file = & seqNumbers.data[i].active_list[j];
				fsname = file->fsp->fi_mnt_point;
				id = file->id;
				copy = file->copy;
				offset = file->offset;

				/*
				 * Since id_to_path() call is very expensive,
				 * make sure that the call is needed first.
				 */
				if ((traceDebug == B_TRUE) ||
				    (display_draining_vsns)) {
					pathname = id_to_path(fsname, id);
				}

				if (traceDebug == B_TRUE) {
					Trace(TR_DEBUG,
					    "[%s] Rearchive file: '%s' "
					    "copy: %d offset: %llx",
					    volName, pathname, copy + 1,
					    offset/TAR_RECORDSIZE);
				}

				if (ignoreRecycling == B_FALSE) {
					rearchiveCopy(fsname, id, copy);
				}

				if (display_draining_vsns) {
					emit(TO_FILE, 0, 20235, copy+1,
					    pathname, volName);
				}
			}

			/*
			 * The following block of code is for the trace
			 * at the end of this function only.
			 */
			if (traceMisc == B_TRUE) {
				capacity = seqNumbers.data[i].tarfile_size;
				space = seqNumbers.data[i].size_active_files;
				recycled_space += capacity - space;
			}
		}
	}
	Trace(TR_MISC, "[%s] Recycled space during drain: %s",
	    volName, StrFromFsize(recycled_space, 1, NULL, 0));
}


/*
 * There are no files to be retained in the tar file associated with
 * the specified sequence number.  Empty all files in the tar file
 * by removing the file on local or remote host.
 */
static long long
emptyTarFile(
	DiskVolumeSeqnum_t seqnum,
	char *volName)
{
	SamrftStatInfo_t buf;
	char *name;

	/*
	 * Generate tar file name from sequence number.
	 */
	name = genSeqNumberFileName(seqnum);

	if (SamrftStat(rft, name, &buf) < 0) {
		return (0);
	}

	Trace(TR_MISC, "[%s] Unlink tar: '%s'", volName, name);
	unlinkFile(name);

	return (buf.size);
}

static long long
statTarFile(
	DiskVolumeSeqnum_t seqnum)
{
	SamrftStatInfo_t buf;
	char *name;
	long long size = -1;

	(void) DiskVolsGenFileName(seqnum, fullpath, sizeof (fullpath));
	name = gobbleName(diskVolume->DvPath, fullpath);

	if (SamrftStat(rft, name, &buf) >= 0) {
		size = buf.size;
	}
	return (size);
}

/*
 * Unlink file on local or remote host.
 */
static void
unlinkFile(
	char *name)
{
	int rval = 0;
	char *host;


	host = DiskVolsGetHostname(diskVolume);

	if (host) {
		cemit(TO_FILE, 0, 20310, host, name);
	} else {
		cemit(TO_FILE, 0, 20311, name);
	}

	if (ignoreRecycling == B_FALSE) {
		rval = SamrftUnlink(rft, name);
	}

	if (rval == 0) {
		numRecycledFiles++;
	}
}

static char *
genSeqNumberFileName(
	DiskVolumeSeqnum_t seqnum)
{
	char *name;

	(void) DiskVolsGenFileName(seqnum, fullpath, sizeof (fullpath));
	name = gobbleName(diskVolume->DvPath, fullpath);

	return (name);
}

/*
 * Create file or directory name path name.
 * Be careful.  Returns path name in static memory which must
 * be duplicated before calling this function again.
 */
static char *
gobbleName(
	char *path,
	char d_name[1])
{
	static upath_t name;

	strcpy(name, path);
	strcat(name, "/");
	strcat(name, d_name);

	return (name);
}

/*
 * Get disk volume sequence number.
 */
static DiskVolumeSeqnum_t
getDiskVolumeSeqnum(
	char *volName,
	DiskVolumeInfo_t *dv)
{
	struct DiskVolumeSeqnumFile buf;
	int nbytes;
	int rc;
	char *filename;
	DiskVolumeSeqnum_t seqnum = -1;
	size_t size = sizeof (struct DiskVolumeSeqnumFile);

	filename = DISKVOLS_SEQNUM_FILENAME;
	if (DISKVOLS_IS_HONEYCOMB(dv)) {
		filename = volName;
	}
	snprintf(fullpath, sizeof (fullpath), "%s/%s.%s",
	    diskVolume->DvPath, filename, DISKVOLS_SEQNUM_SUFFIX);

	rc = SamrftOpen(rft, fullpath, O_RDONLY, NULL);
	if (rc == 0) {
		rc = SamrftFlock(rft, F_RDLCK);
		if (rc < 0) {
			LibFatal(SamrftFlock, "lock");
		}

		nbytes = SamrftRead(rft, &buf, size);
		if (nbytes == size) {
			seqnum = buf.DsRecycle;
			if (buf.DsMagic == DISKVOLS_SEQNUM_MAGIC_RE) {
				sam_bswap8(&seqnum, 1);
			} else if (buf.DsMagic != DISKVOLS_SEQNUM_MAGIC) {
				LibFatal(getDiskVolumeSeqnum, "magic");
			}
		} else {
			LibFatal(SamrftRead, "size");
		}

		if (SamrftFlock(rft, F_UNLCK) < 0) {
			LibFatal(SamrftFlock, "unlock");
		}

		(void) SamrftClose(rft);
	} else if (rc < 0) {
		Trace(TR_ERR, "Can not open %s, errno %d", fullpath, errno);
	}

	Trace(TR_MISC, "[%s] Got recycling sequence number: %llx",
	    volName, seqnum);

	return (seqnum);
}

/*
 * Update space used in disk volume seqnum file.
 */
static int
updateSpaceUsed(
	char *volName,
	long long removedSpace)
{
	struct DiskVolumeSeqnumFile buf;
	int nbytes;
	int rval;
	off64_t off;
	char *filename;
	DiskVolumeSeqnum_t seqnum = -1;
	size_t size = sizeof (struct DiskVolumeSeqnumFile);
	boolean_t swapped = B_FALSE;

	filename = DISKVOLS_SEQNUM_FILENAME;
	if (DISKVOLS_IS_HONEYCOMB(diskVolume)) {
		filename = volName;
	}
	snprintf(fullpath, sizeof (fullpath), "%s/%s.%s",
	    diskVolume->DvPath, filename, DISKVOLS_SEQNUM_SUFFIX);

	rval = SamrftOpen(rft, fullpath, O_RDWR, NULL);
	if (rval == 0) {
		rval = SamrftFlock(rft, F_RDLCK);
		if (rval < 0) {
			LibFatal(SamrftFlock, "lock");
		}

		nbytes = SamrftRead(rft, &buf, size);
		if (nbytes == size) {
			if (buf.DsMagic == DISKVOLS_SEQNUM_MAGIC_RE) {
				swapped = TRUE;
				sam_bswap8(&buf.DsUsed, 1);
			} else if (buf.DsMagic != DISKVOLS_SEQNUM_MAGIC) {
				LibFatal(updateSpacedUsed, "magic");
			}
		} else {
			LibFatal(SamrftRead, "size");
		}

		Trace(TR_MISC, "[%s] Update used from %lld bytes (%s)",
		    volName, buf.DsUsed, StrFromFsize(buf.DsUsed, 3, NULL, 0));

		if (removedSpace > buf.DsUsed) {
			Trace(TR_MISC, "[%s] Removed space > used", volName);
			buf.DsUsed = 0;
		} else {
			buf.DsUsed -= removedSpace;
		}

		Trace(TR_MISC, "[%s] Update used to %lld bytes (%s)",
		    volName, buf.DsUsed, StrFromFsize(buf.DsUsed, 3, NULL, 0));

		if (swapped) {
			sam_bswap8(&buf.DsUsed, 1);
		}

		/*
		 * Prepare for write, rewind the file.
		 */
		if (SamrftSeek(rft, 0, SEEK_SET, &off) < 0) {
			LibFatal(SamrftSeek, fullpath);
		}

		nbytes = SamrftWrite(rft, &buf, size);
		if (nbytes != size) {
			LibFatal(SamrftWrite, fullpath);
		}

		if (SamrftFlock(rft, F_UNLCK) < 0) {
			LibFatal(SamrftFlock, fullpath);
		}
		(void) SamrftClose(rft);
	}
	return (rval);
}

static void
addActiveList(
	seqEntry_t *buf,
	struct sam_fs_info *fsp,
	union sam_di_ino *inode,
	int copy,
	u_longlong_t offset)
{
	int free;

	if (buf->active_list == NULL ||
	    (buf->num_active_list >= buf->alloc_active_list)) {
		initActiveList(buf);
	}

	free = buf->num_active_list;
	buf->active_list[free].fsp = fsp;
	buf->active_list[free].id = inode->inode.di.id;
	buf->active_list[free].copy = copy;
	buf->active_list[free].offset = offset;
	buf->num_active_list++;
}

static void
initActiveList(
	seqEntry_t *buf)
{
	int size;

	if (buf->active_list == NULL) {
		buf->alloc_active_list = ACTIVE_LIST_CHUNKSIZE;
		size = buf->alloc_active_list * sizeof (File_t);
		SamMalloc(buf->active_list, size);
		memset(buf->active_list, 0, size);
	} else {
		buf->alloc_active_list += ACTIVE_LIST_CHUNKSIZE;
		size = buf->alloc_active_list * sizeof (File_t);
		SamRealloc(buf->active_list, size);
	}
}

static void
rearchiveCopy(
	char *fsname,
	sam_id_t id,
	int copy)
{
	int rc;
	struct sam_ioctl_idscf arg;
	int fd;

	fd = open(fsname, O_RDONLY);
	if (fd < 0) {
		return;
	}

	arg.id = id;
	arg.copy = copy;
	arg.c_flags = AR_rearch;
	arg.flags = AR_rearch;

	rc = ioctl(fd, F_IDSCF, &arg);
	if (rc < 0) {
		char *pathname;

		pathname = id_to_path(fsname, id);
		Trace(TR_ERR, "Cannot ioctl(F_IDSCF) %s:%d", pathname, errno);
	}
	(void) close(fd);
}


/*
 * There are no files to be retained in the honeycomb file associated with
 * the specified sequence number.  Delete the object.
 */
static long long
deleteObject(
	DiskVolumeSeqnum_t seqnum,
	char *volName)
{
	static char *queryBuf = NULL;
	hc_oid oid;
	hcerr_t hcerr;
	hc_query_result_set_t *rset;
	int numOids;
	int finished;
	hc_nvr_t *nvr;
	char **names;
	char **values;
	int count;
	int idx;
	long long objectSize;
	int rval;

	queryBuf = DiskVolsGenMetadataQuery(volName, seqnum, queryBuf);

	Trace(TR_MISC, "HC query buf: '%s'", queryBuf);

	rset = NULL;
	hcerr = hc_query_ez(session, queryBuf, NULL, 0, 1, &rset);
	if (hcerr != HCERR_OK) {
		Trace(TR_MISC, "HC client query failed, hcerr= %d - %s",
		    hcerr, hc_decode_hcerr(hcerr));
		return (0);
	}

	numOids = 0;
	nvr = NULL;

	for (;;) {

		hcerr = hc_qrs_next_ez(rset, &oid, &nvr, &finished);
		if (hcerr != HCERR_OK || finished == 1) {
			Trace(TR_MISC, "HC query complete: hcerr= %d "
			    "finished= %d", hcerr, finished);
			break;
		}
		if (hcerr == HCERR_OK) {
			numOids++;
			Trace(TR_MISC, "HC query complete oid: '%s'", oid);
		}
	}

	(void) hc_qrs_free(rset);

	if (numOids > 1) {
		Trace(TR_MISC, "Unexpected HC query result, "
		    "more than 1 OID (%d) found", numOids);
	}

	if (hcerr != HCERR_OK || numOids == 0 || numOids > 1) {
		return (0);
	}

	hcerr = hc_retrieve_metadata_ez(session, &oid, &nvr);
	if (hcerr != HCERR_OK) {
		Trace(TR_MISC, "HC client retrieve metadata failed, "
		    "hcerr= %d - %s", hcerr, hc_decode_hcerr(hcerr));
		return (0);
	}

	hcerr = hc_nvr_convert_to_string_arrays(nvr, &names, &values, &count);
	if (hcerr != HCERR_OK) {
		Trace(TR_MISC, "HC client convert to string failed, "
		    "hcerr= %d - %s", hcerr, hc_decode_hcerr(hcerr));
		return (0);
	}

	if (nvr != NULL) {
		(void) hc_nvr_free(nvr);
	}

	if (count == 0 || names == NULL || values == NULL) {
		Trace(TR_MISC, "HC client no metadata returned");
		return (0);
	}

	objectSize = 0;
	for (idx = 0; idx < count; idx++) {
		Trace(TR_MISC, "[%d] %s = %s", idx, *names, *values);
		if (strcmp(*names, HONEYCOMB_METADATA_OBJECT_SIZE) == 0) {
			char *p = *values;
			objectSize = strtoll(*values, &p, 0);
			break;
		}
		names++;
		values++;
	}

	Trace(TR_MISC, "[%s] Delete oid: '%s'", volName, oid);

	rval = deleteMetadataRecord(&oid);
	if (rval == -1) {
		objectSize = 0;
	}

	return (objectSize);
}

/*
 * Delete metadata record for a honeycomb object.  When the last
 * metadata record associated with a data object is deleted, the
 * unerlying data object is also deleted.
 */
static int
deleteMetadataRecord(
	hc_oid *oid)
{
	hcerr_t hcerr;

	cemit(TO_FILE, 0, 20311, oid);

	if (ignoreRecycling == B_FALSE) {
		hcerr = hc_delete_ez(session, oid);
		if (hcerr != HCERR_OK) {
			Trace(TR_MISC, "HC client delete failed, "
			    "hcerr= %d - %s", hcerr, hc_decode_hcerr(hcerr));
			return (-1);
		}
	}

	numRecycledFiles++;

	return (0);
}

/*
 * Clear disk volume flag.
 */
static void
clearDiskVolsFlag(
	char *volname,
	int flag)
{
	int rval;
	struct DiskVolsDictionary *dict;

	diskVolume->DvFlags &= ~flag;
	dict = DiskVolsGetHandle(DISKVOLS_VSN_DICT);
	if (dict == NULL) {
		LibFatal(DiskVolsNewHandle, "clearDiskVolsFlag");
	}
	rval = dict->Put(dict, volname, diskVolume);
	if (rval != 0) {
		Trace(TR_MISC, "Disk dictionary clear flag 0x%x failed: dk.%s",
		    rval, volname);
	}
	DiskVolsRelHandle(DISKVOLS_VSN_DICT);
}

/*
 *	Keep lint happy
 */
#if defined(lint)
void
swapdummy(void *buf)
{
	sam_bswap2(buf, 1);
	sam_bswap4(buf, 1);
	sam_bswap8(buf, 1);
}
#endif /* defined(lint) */
