/*
 * dkarchive.c - Disk media archival.
 *
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


#pragma ident "$Revision: 1.58 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utime.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>

/* SAM-FS headers. */
#include "pub/stat.h"
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/fioctl.h"
#include "sam/sam_trace.h"
#include "aml/device.h"
#include "aml/diskvols.h"
#include "aml/opticals.h"
#include "aml/sam_rft.h"
#include "sam/lib.h"
#include <sam/fs/bswap.h>

/* Local headers. */
#include "arcopy.h"
#define	BUFFER_USER

#define	TARFILE_MODE 0600	/* create mode for disk tar files */

/* Private data. */
static struct DiskVolumeInfo *diskVolume;
static upath_t seqnumFileName;	/* sequence number file name */
static upath_t tarFileName;
static upath_t tarFilePath;
static struct stat dvPathStat;

/* Private functions. */
static void initRemoteArchive(char *host_name);
static DiskVolumeSeqnum_t openTarFilePath();
static int mkDirPath(char *path, struct stat *sb);
static int mkDir(char *dirname, struct stat *sb);
static void errorMsg(char *op);
static void outOfSpace();


/*
 * Disk archiving to the sequence number is complete.
 */
void
DkArchiveDone(void)
{
	/*
	 * Update disk volume seqnum file.
	 */
	DkUpdateVolumeSeqnum(diskVolume->DvPath, seqnumFileName);
}


/*
 * Begin writing disk archive file.  Called at the beginning of an
 * archive file.
 */
void
DkBeginArchiveFile(void)
{
	DiskVolumeSeqnum_t seqnum;
	struct VolsEntry *ve;
	int	rc;

	Trace(TR_ARDEBUG, "Begin dk archive file");

	/*
	 * Set block size and buffer size.
	 */
	BlockSize = OD_SS_DEFAULT;
	WriteCount = OD_BS_DEFAULT;

	/*
	 * Before generating seed value, make sure the disk
	 * volume path exists.
	 */
	ve = &VolsTable->entry[VolCur];
	rc = DiskVolsLabel(ve->Vi.VfVsn, diskVolume, RemoteArchive.rft);

	if (rc != 0) {
		if (rc == DISKVOLS_PATH_UNAVAIL) {
			/*
			 * Path name of the disk archive directory is not
			 * available.  Seqnum file has been created (labeled)
			 * but does not exist.
			 */
			SendCustMsg(HERE, 4050, ve->Vi.VfVsn, GetCustMsg(4051));

		} else if (rc == DISKVOLS_PATH_ENOENT) {
			/*
			 * Path name of the disk archive directory
			 * does not exist.
			 */
			SendCustMsg(HERE, 4050, ve->Vi.VfVsn, GetCustMsg(4052));

		} else {
			/*
			 * Failed to open/create disk archive directory.
			 */
			SendCustMsg(HERE, 4050, ve->Vi.VfVsn, GetCustMsg(4053));
		}
		exit(EXIT_FAILURE);
	}

	seqnum = openTarFilePath();

	/*
	 * Enter media information for archreq.
	 */
	VolsTable->entry[VolCur].VlPosition = seqnum;
}


/*
 * Begin copying a file.
 * Enter volume and position for file being copied.
 */
void
DkBeginCopyFile(void)
{
	File->AfVol = VolCur;
	File->AfPosition = VolsTable->entry[VolCur].VlPosition;
	File->AfOffset = 0;
}


/*
 * End disk archive file.
 */
void
DkEndArchiveFile(void)
{
	int	rc;

	/*
	 * Close file on remote host.  Connection remains
	 * open and active.
	 */
	rc = SamrftClose(RemoteArchive.rft);
	if (rc == -1) {
		IoFatal(SamrftClose, RemoteArchive.host);
	}
}


/*
 * Get disk volume sequence number.
 */
DiskVolumeSeqnum_t
DkGetVolumeSeqnum(
	char *path,
	char *fname)
{
	struct DiskVolumeSeqnumFile buf;
	DiskVolumeSeqnum_t	DsVal;
	boolean_t swapped = B_FALSE;
	off64_t off;
	size_t size = sizeof (struct DiskVolumeSeqnumFile);
	int	nbytes;
	int	rc;

	snprintf(ScrPath, sizeof (ScrPath), "%s/%s", path, fname);

	rc = SamrftOpen(RemoteArchive.rft, ScrPath, O_RDWR, NULL);
	if (rc != 0) {
		Trace(TR_ERR, "Disk volume seqnum file %s, open failed %d",
		    ScrPath, errno);
		return (-1);
	}

	if (SamrftFlock(RemoteArchive.rft, F_WRLCK) < 0) {
		Trace(TR_ERR, "Disk volume seqnum file %s, flock failed %d",
		    ScrPath, errno);
		return (-1);
	}

	nbytes = SamrftRead(RemoteArchive.rft, &buf, size);

	if (nbytes != size) {
		Trace(TR_ERR, "Disk volume seqnum file %s, read failed %d",
		    ScrPath, errno);
		return (-1);
	}

	DsVal = buf.DsVal;
	if (buf.DsMagic == DISKVOLS_SEQNUM_MAGIC_RE) {
		swapped = B_TRUE;
		sam_bswap8(&DsVal, 1);
	} else if (buf.DsMagic != DISKVOLS_SEQNUM_MAGIC) {
		Trace(TR_ERR, "Disk volume seqnum file %s,"
		    " magic number invalid", ScrPath);
		return (-1);
	}
	DsVal++;
	if (DsVal > DISKVOLS_SEQNUM_MAX_VALUE) {
		Trace(TR_ERR, "Disk volume seqnum file %s, overflow", ScrPath);
		return (-1);
	}
	buf.DsVal = DsVal;
	if (swapped) {
		sam_bswap8(&buf.DsVal, 1);
	}

	/*
	 * Prepare for write, rewind the file.
	 */
	if (SamrftSeek(RemoteArchive.rft, 0, SEEK_SET, &off) < 0) {
		Trace(TR_ERR, "Disk volume seqnum file %s, seek failed %d",
		    ScrPath, errno);
		return (-1);
	}

	nbytes = SamrftWrite(RemoteArchive.rft, &buf, size);

	if (nbytes != size) {
		Trace(TR_ERR, "Disk volume seqnum file %s, write failed %d",
		    ScrPath, errno);
		return (-1);
	}

	if (SamrftFlock(RemoteArchive.rft, F_UNLCK) < 0) {
		Trace(TR_ERR, "Disk volume seqnum file %s, flock failed %d",
		    ScrPath, errno);
		return (-1);
	}
	(void) SamrftClose(RemoteArchive.rft);

	return (DsVal);
}


/*
 * Initialize disk archive module.
 * Initialize disk archiving options and map in
 * disk volume table.
 */
void
DkInit(void)
{
	extern char *program_name;
	struct MediaParamsEntry *mp;
	char	*host_name;
	struct VolsEntry *ve;
	struct DiskVolsDictionary *dict;
	struct DiskVolumeInfo *dv;
	size_t size;
	char *mtype;

	Trace(TR_ARDEBUG, "Init dk archive file");

	mtype = sam_mediatoa(DT_DISK);
	mp = MediaParamsGetEntry(mtype);
	WriteTimeout = mp->MpTimeout;
	ve = &VolsTable->entry[VolCur];
	diskVolume = NULL;

	dict = DiskVolsNewHandle(program_name, DISKVOLS_VSN_DICT,
	    DISKVOLS_RDONLY);
	if (dict == NULL) {
		LibFatal(DiskVolsNewHandle, "InitDkArchive");
	}

	(void) dict->Get(dict, ve->Vi.VfVsn, &dv);
	if (dv != NULL) {
		size =
		    STRUCT_RND(sizeof (struct DiskVolumeInfo) + dv->DvPathLen);
		SamMalloc(diskVolume,  size);
		memset(diskVolume, 0,  size);
		memcpy(diskVolume, dv, size);
	}
	(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
	if (dv == NULL) {
		LibFatal(dict->Get, "InitDkArchive");
	}

	snprintf(ArVolName, sizeof (ArVolName), "%s.%s", mtype, ve->Vi.VfVsn);

	host_name = DiskVolsGetHostname(diskVolume);
	initRemoteArchive(host_name);

	snprintf(seqnumFileName, sizeof (seqnumFileName), "%s.%s",
	    DISKVOLS_SEQNUM_FILENAME, DISKVOLS_SEQNUM_SUFFIX);

	VolBytesWritten = 0;
#if defined(lint)
	sam_bswap2(diskVolume, 1);
	sam_bswap4(diskVolume, 1);
#endif /* defined(lint) */
}


/*
 * Set disk volume flag.
 */
void
DkSetDiskVolsFlag(
	struct DiskVolumeInfo *dv,
	int flag)
{
	int ret;
	struct DiskVolsDictionary *dict;
	struct VolsEntry *ve;
	extern char *program_name;

	ve = &VolsTable->entry[VolCur];
	dv->DvFlags |= flag;
	dict = DiskVolsNewHandle(program_name, DISKVOLS_VSN_DICT, 0);
	if (dict == NULL) {
		LibFatal(DiskVolsNewHandle, "DkSetDiskVolsFlag");
	}
	ret = dict->Put(dict, ve->Vi.VfVsn, dv);
	if (ret != 0) {
		Trace(TR_ERR, "Disk dictionary set flag 0x%x failed: %s.%s",
		    ret, ve->Vi.VfMtype, ve->Vi.VfVsn);
	} else {
		SendCustMsg(HERE, 4048, ve->Vi.VfMtype, ve->Vi.VfVsn);
	}
	(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
}


/*
 * Update disk volume seqnum file.
 * Increment disk volume sequence number for recycling and update space
 * used for volume.
 */
void
DkUpdateVolumeSeqnum(
	char *path,
	char *fname)
{
	struct DiskVolumeSeqnumFile buf;
	off64_t	off;
	size_t	size = sizeof (struct DiskVolumeSeqnumFile);
	int	nbytes;
	int	rc;
	struct VolsEntry *ve;

	ve = &VolsTable->entry[VolCur];

	Trace(TR_DEBUG, "Dk archive '%s' done, wrote %lld bytes (%s)",
	    ve->Vi.VfVsn, VolBytesWritten,
	    StrFromFsize(VolBytesWritten, 3, NULL, 0));

	snprintf(ScrPath, sizeof (ScrPath), "%s/%s", path, fname);

	rc = SamrftOpen(RemoteArchive.rft, ScrPath, O_RDWR, NULL);
	if (rc != 0) {
		errorMsg("open");
		IoFatal(open, ScrPath);
	}

	if (SamrftFlock(RemoteArchive.rft, F_WRLCK) < 0) {
		IoFatal(SamrftFlock, "lock");
	}

	nbytes = SamrftRead(RemoteArchive.rft, &buf, size);

	if (nbytes != size) {
		LibFatal(read, ScrPath);
	}

	/*
	 * since we don't actually use any values here we can
	 * save a byte swap.  If in the future we modify or
	 * use any values from the file, we'll have to swap
	 * and then reswap before updating.
	 */
	if ((buf.DsMagic != DISKVOLS_SEQNUM_MAGIC) &&
	    (buf.DsMagic != DISKVOLS_SEQNUM_MAGIC_RE)) {
		Trace(TR_ERR, "Disk volume seqnum file %s,"
		    " magic number invalid", ScrPath);
		return;
	}

	Trace(TR_DEBUG,
	    "Disk volume '%s' update used from %lld bytes (%s) recycle: %lld",
	    ve->Vi.VfVsn, buf.DsUsed, StrFromFsize(buf.DsUsed, 3, NULL, 0),
	    buf.DsRecycle);

	buf.DsRecycle = buf.DsVal;
	buf.DsUsed += VolBytesWritten;

	Trace(TR_DEBUG,
	    "Disk volume '%s' update used to %lld bytes (%s) recycle: %lld",
	    ve->Vi.VfVsn, buf.DsUsed, StrFromFsize(buf.DsUsed, 3, NULL, 0),
	    buf.DsRecycle);

	/*
	 * Prepare for write, rewind the file.
	 */
	if (SamrftSeek(RemoteArchive.rft, 0, SEEK_SET, &off) < 0) {
		IoFatal(SamrftSeek, ScrPath);
	}

	nbytes = SamrftWrite(RemoteArchive.rft, &buf, size);

	if (nbytes != size) {
		IoFatal(write, ScrPath);
	}

	if (SamrftFlock(RemoteArchive.rft, F_UNLCK) < 0) {
		LibFatal(SamrftFlock, "unlock");
	}
	(void) SamrftClose(RemoteArchive.rft);
}


/*
 * Write to disk media.
 */
int
DkWrite(
	void *rft,
	void *buf,
	size_t nbytes)
{
	ssize_t bytesWritten;

#if defined(AR_DEBUG)

	/*
	 * Simulate a write error.
	 * Uses file in directory CopyErrors.
	 * mtf -s 1M CopyErrors/'dir[1-5]/file[0-9]'
	 */
	if (strncmp(File->f->FiName, "CopyErrors/", 11) == 0) {
		if (VolBytesWritten >= 25600000) {
			errno = EIO;
			Trace(TR_DEBUGERR, "ERRsim write(%s)", ArVolName);
			return (-1);
		}
	}
#endif /* defined(AR_DEBUG) */

	bytesWritten = SamrftWrite((SamrftImpl_t *)rft, buf, nbytes);

	return (bytesWritten);
}


/*
 * Write error.
 */
int
DkWriteError(void)
{
	struct VolsEntry *ve;

	ve = &VolsTable->entry[VolCur];

	Trace(TR_ERR, "Dk write error: %s (%s) errno= %d",
	    ve->Vi.VfVsn, diskVolume->DvPath, errno);

	if (errno == ENOSPC) {
		outOfSpace();
		return (0);
	} else if (errno == ETIME) {
		Trace(TR_ERR, "Write timeout: %s", diskVolume->DvPath);
	} else if (errno == EIO) {
		DkSetDiskVolsFlag(diskVolume, DV_bad_media);
	}
	return (-1);
}


/*
 * Initialize a remote disk archive.  Note: The Samrft library
 * routines handle whether disk archive is local or remote.  Arcopy
 * does not care.
 */
static void
initRemoteArchive(
	char *host_name)
{
	RemoteArchive.enabled = TRUE;
	RemoteArchive.host = host_name;

	/*
	 * Establish connection to remote host.
	 */
	RemoteArchive.rft = (void *)SamrftConnect(RemoteArchive.host);
	if (RemoteArchive.rft == NULL) {
		LibFatal(SamrftConnect, RemoteArchive.host);
	}
}


/*
 * Generate next sequence number and open tar file on local or remote host.
 */
static DiskVolumeSeqnum_t
openTarFilePath(void)
{
	DiskVolumeSeqnum_t seqnum;
	SamrftCreateAttr_t creat;
	int	oflag;
	int	rc;

	seqnum = DkGetVolumeSeqnum(diskVolume->DvPath, seqnumFileName);

	/*
	 * Check for fatal error on disk volume seqnum file.  If an error
	 * occured, set the error flag on disk volume.
	 */
	if (seqnum < 0) {
		DkSetDiskVolsFlag(diskVolume, DV_bad_media);
		LibFatal(DkGetVolumeSeqnum, "openTarFilePath");
	}

	/*
	 * Generate tar file name.
	 */
	(void) DiskVolsGenFileName(seqnum, tarFileName, sizeof (tarFileName));

	/*
	 * Set tar file path.
	 */
	snprintf(tarFilePath, sizeof (tarFilePath), "%s/%s",
	    diskVolume->DvPath, tarFileName);

	Trace(TR_DEBUG, "request %s (0x%llx)", tarFilePath, seqnum);

	/*
	 * Set O_CREAT and O_EXCL so the open will fail if the file exists.
	 */
	oflag = O_WRONLY | O_CREAT | O_EXCL | O_LARGEFILE;

	/*
	 * File creation attributes.
	 */
	memset(&creat, 0, sizeof (SamrftCreateAttr_t));
	creat.mode = TARFILE_MODE;
	creat.uid  = dvPathStat.st_uid;
	creat.gid  = dvPathStat.st_gid;

	/*
	 * Open disk archive tar file on remote host.
	 */
	rc = SamrftOpen(RemoteArchive.rft, tarFilePath, oflag, &creat);
	if (rc < 0) {
		/*
		 * Open failed.  Make sure directory path
		 * exists and try again.
		 */
		if (mkDirPath(tarFilePath, &dvPathStat) < 0) {
			LibFatal(mkdir, tarFilePath);
		}
		rc = SamrftOpen(RemoteArchive.rft, tarFilePath, oflag, &creat);
		if (rc < 0) {
			LibFatal(SamrftOpen, tarFilePath);
		}
	}
	return (seqnum);
}


/*
 * Make directory path.
 */
static int
mkDirPath(
	char *path,
	struct stat *sb)
{
	size_t	len;

	/*
	 * Back up through components until mkdir succeeds.
	 */
	len = strlen(path);
	for (;;) {
		char	*p;

		p = strrchr(path, '/');
		if (p == NULL || p == path) {
			return (-1);
		}
		*p = '\0';
		if (mkDir(path, sb) == 0) {
			break;
		}
		if (errno != ENOENT) {
			return (-1);
		}
	}

	/*
	 * Move forward through components to the original name.
	 */
	path[strlen(path)] = '/';
	while (strlen(path) < len) {
		if (mkDir(path, sb) != 0) {
			return (-1);
		}
		path[strlen(path)] = '/';
	}
	return (0);
}


/*
 * Make directory.
 */
static int
mkDir(
	char *dirname,
	struct stat *sb)
{
	int	rc;

	SamrftCreateAttr_t creat;

	memset(&creat, 0, sizeof (SamrftCreateAttr_t));
	creat.mode = sb->st_mode & S_IAMB;
	creat.uid = sb->st_uid;
	creat.gid = sb->st_gid;

	rc = SamrftMkdir(RemoteArchive.rft, dirname, &creat);

	/*
	 * Not an error if path already exists.
	 */
	if (rc != 0 && errno == EEXIST) {
		rc = 0;
	}
	return (rc);
}


/*
 * Process error message.
 */
static void
errorMsg(
	char *op)
{
	char	*host_name;

	host_name = DiskVolsGetHostname(diskVolume);
	if (host_name == NULL) {
		host_name = "";
	}

	PostOprMsg(4539, op, host_name, ScrPath, errno);
	ThreadsSleep(2);
}


/*
 * Process out of disk space.
 */
static void
outOfSpace(void)
{
	struct VolsEntry *ve;

	DkSetDiskVolsFlag(diskVolume, DV_archfull);

	ve = &VolsTable->entry[VolCur];
	/* Volume full: %s.%s: %s bytes written */
	SendCustMsg(HERE, 4043, ve->Vi.VfMtype, ve->Vi.VfVsn,
	    CountToA(VolBytesWritten));
}


/*	keep lint happy */
#if defined(lint)
void
swapdummy(
	void *buf)
{
	sam_bswap2(buf, 1);
	sam_bswap4(buf, 1);
	sam_bswap8(buf, 1);
}
#endif /* defined(lint) */
