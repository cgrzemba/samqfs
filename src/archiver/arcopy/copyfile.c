/*
 * copyfile.c - Arcopy file copy module.
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


#pragma ident "$Revision: 1.85 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utime.h>
#include <inttypes.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

/* Solaris headers. */
#include <sys/mman.h>
#include <sys/param.h>

/* SAM-FS headers. */
#include "pub/rminfo.h"
#include "pub/sam_errno.h"
#include "pub/stat.h"
#include "sam/types.h"
#include "sam/param.h"
#include "sam/custmsg.h"
#include "sam/lib.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "sam/uioctl.h"
#include "sam/checksum.h"
#include "sam/checksumf.h"
#include "aml/tar.h"
#include "aml/device.h"
#include "aml/sam_rft.h"
#include "sam/fs/dirent.h"

/* Local headers. */
#include "arcopy.h"

/* Private data. */
static pthread_mutex_t bufInuse = PTHREAD_MUTEX_INITIALIZER;
static boolean_t lockBuffer = FALSE;
static fsize_t fileOffset = 0;	/* Offset from beginning of archive file */
static int s_fd;		/* Source file descriptor */

/* I/O buffer. */
static char *bufFirst = NULL;	/* Start of buffer */
static char *bufLast = NULL;	/* End of buffer */
static size_t bufSize;		/* Integral multiple of media block size */

/* Circular buffer controls. */
static ATOM_INT_T bufIn = 0;	/* Offset in buffer of next input */
static ATOM_INT_T bufOut = 0;	/* Offset in buffer of next output */

/*
 * Buffer thread controls.
 * These controls are used only for cond_wait().  cond_wait() switches
 * between threads in a single processor system.
 */
static pthread_mutex_t bufLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t bufRead = PTHREAD_COND_INITIALIZER;
static pthread_cond_t bufWrite = PTHREAD_COND_INITIALIZER;

/* Control flags. */
static boolean_t bufEndArchive = FALSE;
static boolean_t bufEmpty = FALSE;
static boolean_t bufFull = FALSE;
static boolean_t bufWriteEnd = FALSE;

/* Private functions. */
static void beginArchiveFile(void);
static void copyDir(struct sam_disk_inode *dp);
static void copyLink(void);
static void copyRmMedia(struct sam_disk_inode *dp);
static void copyRegular(struct sam_disk_inode *dp);
static void copySegment(struct sam_disk_inode *dp);
static void wakeup(void);
static boolean_t writeBlock(size_t nbytes);
static void writeStop(void);


/*
 * Copy a single file to the archive media.
 * Create the tar record for the file.
 */
void
CopyFile(void)
{
	struct sam_disk_inode dp;
	struct sam_ioctl_idopen idopen;
	struct sam_stat st;

	/*
	 * Open file using inode id.
	 */
	idopen.id	  = File->f->FiId;
	idopen.mtime  = File->f->FiModtime;
	idopen.copy   = ArchiveCopy;
	idopen.flags  = (DirectIo) ? IDO_offline_direct : 0;
	idopen.flags |= (lockBuffer) ? IDO_buf_locked : 0;
	idopen.flags |=
	    (ArchiveSet->AsEflags & AE_directio) ? IDO_direct_io : 0;
	idopen.dp.ptr = &dp;

#if defined(AR_DEBUG)
/*
 * Test error returns on open.
 * Uses files in directory CopyErrors.
 * mtf -s 1M CopyErrors/'dir[1-5]/file[0-9]'
 */
	{
#undef unlink
	upath_t path;

	if (strcmp(File->f->FiName, "CopyErrors/dir2/file3") == 0) {
		/* Test busy. */
		snprintf(path, sizeof (path), "%s/%s", MntPoint,
		    File->f->FiName);
		if (open(path, O_RDWR) == -1) {
			LibFatal(open, path);
		}
	} else if (strcmp(File->f->FiName, "CopyErrors/dir3/file1") == 0) {
		/* Test modified. */
		snprintf(path, sizeof (path), "%s/%s", MntPoint,
		    File->f->FiName);
		if (utime(path, NULL) == -1) {
			LibFatal(utime, path);
		}
	} else if (strcmp(File->f->FiName, "CopyErrors/dir1/file9") == 0) {
		/* Test removed. */
		snprintf(path, sizeof (path), "%s/%s", MntPoint,
		    File->f->FiName);
		if (unlink(path) == -1) {
			LibFatal(unlink, path);
		}
	}
	}
#endif /* defined(AR_DEBUG) */

	SetTimeout(TO_stage);
	if ((s_fd = ioctl(FsFd, F_IDOPENARCH, &idopen)) < 0) {
		ClearTimeout(TO_stage);
		if (errno == ER_FILE_IS_OFFLINE &&
		    (File->f->FiFlags & FI_stagesim)) {
			if ((s_fd = open("/dev/zero", O_RDONLY)) != -1) {
				goto simread;
			}
			errno = ER_FILE_IS_OFFLINE;
		}
		if (errno == ENOENT) {
			Trace(TR_DEBUG, "open(%s): %s", File->f->FiName,
			    StrFromErrno(errno, NULL, 0));
#if defined(AR_DEBUG)
		} else if (errno == EARCHMD) {
			char ts[ISO_STR_FROM_TIME_BUF_SIZE];
			char tso[ISO_STR_FROM_TIME_BUF_SIZE];

			/*
			 * Show the mod times.
			 */
			(void) TimeToIsoStr(File->f->FiModtime, ts);
			(void) TimeToIsoStr(dp.modify_time.tv_sec, tso);
			Trace(TR_ARDEBUG, "%s modified %s %s",
			    File->f->FiName, ts, tso);
#endif /* defined(AR_DEBUG) */
		} else if (errno == ENOARCH || errno == EARCHMD ||
		    errno == EQ_FILE_BUSY) {
			/*
			 * Already archived, modified, or busy aren't serious.
			 */
			Trace(TR_DEBUGERR, "open(%s)", File->f->FiName);
		} else {
			/*
			 * All others are serious.
			 */
			Trace(TR_ERR, "open(%s/%s)", MntPoint, File->f->FiName);
		}
		File->AfFlags |= AF_error;
		File->f->FiFlags |= FI_error;
		File->f->FiErrno = errno;
		return;
	}
	ClearTimeout(TO_stage);
	SparseFile = FALSE;

simread:
	/* Copying file %s */
	PostOprMsg(4308, File->f->FiName);
	PthreadMutexLock(&bufInuse);
	if (fileOffset == 0) {
		beginArchiveFile();
	}
	if (Instance->CiFlags & CI_disk) {
		DkBeginCopyFile();
#if !defined(_NoOSD_)
	} else if (Instance->CiFlags & CI_honeycomb) {
		HcBeginCopyFile();
#endif
	}

	/*
	 * Set the size of the data in the tar file.
	 */
	if (S_ISREQ(dp.mode)) {
		File->AfFileSize = SAM_RM_SIZE(dp.psize.rmfile);
	} else if (S_ISSEGI(&dp)) {
		File->AfFileSize = dp.rm.info.dk.seg.fsize;
	} else {
		File->AfFileSize = dp.rm.size;
	}

	if (File->f->FiStatus & FIF_simread) {
		/*
		 * Simulated file read specified by sam-arfind.
		 * Read from /dev/zero.
		 */
		(void) close(s_fd);
		if ((s_fd = open("/dev/zero", O_RDONLY)) == -1) {
			Trace(TR_DEBUGERR, "open(%s)", "/dev/zero");
			File->AfFlags |= AF_error;
			File->f->FiFlags |= FI_error;
			File->f->FiErrno = errno;
			return;
		}
		File->AfFileSize = File->f->FiFileSize;
	}

	/*
	 * Make and write the tar header record.
	 */
	if (AF_tarheader(File->AfFlags)) {
		st.st_mode	= dp.mode;		/* File mode */
		st.st_uid	= dp.uid;		/* Owner user id */
		st.st_gid	= dp.gid;		/* Owner group id */
		st.st_size	= File->AfFileSize;	/* File size lower */
		st.st_atime	= dp.access_time.tv_sec; /* Access time */
		st.st_mtime	= dp.modify_time.tv_sec; /* Modification time */
		st.st_ctime	= dp.change_time.tv_sec; /* Inode changed */
		if (ZeroOffset) {
			File->AfOffset = fileOffset;
		}
		BuildHeader(File->f->FiName, File->f->FiName_l, &st);
		if (!ZeroOffset) {
			File->AfOffset = fileOffset;
		}
	}

	/*
	 * When file is archived, the access and modify times are reset.
	 * If the modify time changes while the copy is being made,
	 * the copy is stale.
	 */
	File->AfAccessTime = dp.access_time;
	File->AfModifyTime = dp.modify_time;

	/*
	 * copy the file appropriately for its type.
	 */
	if (S_ISLNK(dp.mode)) {
		copyLink();
	} else if (S_ISSEGI(&dp)) {
		copySegment(&dp);
	} else if (S_ISREQ(dp.mode)) {
		copyRmMedia(&dp);
	} else if (S_ISDIR(dp.mode)) {
		copyDir(&dp);
	} else {
		copyRegular(&dp);
	}

	if (AF_tarheader(File->AfFlags)) {
		RoundBuffer(TAR_RECORDSIZE);
	}
	PthreadMutexUnlock(&bufInuse);

	/*
	 * Close the file and update its status.
	 */
	if (close(s_fd) < 0) {
		Trace(TR_DEBUGERR, "close(%s/%s)", MntPoint, File->f->FiName);
		File->AfFlags |= AF_error;
	}

	if (!(File->AfFlags & AF_error)) {
		File->AfFlags |= AF_copied;
	}
	if (File->AfFlags & AF_copied) {
		Instance->CiFilesWritten++;
		(void) time(&File->AfCreateTime);
		Trace(TR_FILES, "Copied %d.%d %s size:%s",
		    File->f->FiId.ino, File->f->FiId.gen, File->f->FiName,
		    StrFromFsize(File->AfFileSize, 1, NULL, 0));
	}
}


/*
 * Reconfigure buffer if necessary.
 */
void
CopyFileReconfig(void)
{
	static int blockCount;
	char	*prevBufFirst;
	size_t	prevBufSize;
	int	blkCnt;
	longlong_t bSize;

	PthreadMutexLock(&bufInuse);
	prevBufFirst = bufFirst;
	prevBufSize = bufSize;
	if (bufFirst == NULL) {
		struct MediaParamsEntry *mp;

		mp = MediaParamsGetEntry(VolsTable->entry[VolCur].Vi.VfMtype);
		if (ArchiveSet->AsFlags & AS_bufsize) {
			blockCount = ArchiveSet->AsBufsize;
		} else {
			if (mp != NULL) {
				blockCount = mp->MpBufsize;
			} else {
				blockCount = 4;
			}
		}
		if (ArchiveSet->AsEflags & AE_lockbuf ||
		    (mp != NULL && (mp->MpFlags & MP_lockbuf))) {
			lockBuffer = TRUE;
		}
	}
	blkCnt = blockCount;
	bSize = (longlong_t)blkCnt * (longlong_t)WriteCount;
	while (bSize >= SIZE_MAX) {
		blkCnt >>= 1;
		bSize = (longlong_t)blkCnt * (longlong_t)WriteCount;
	}
	if (blkCnt < blockCount) {
		Trace(TR_MISC,
		    "bufsize %d too big, adjusted to %d", blockCount, blkCnt);
		blockCount = blkCnt;
	}
	bufSize = blockCount * WriteCount;
	if (bufSize <= prevBufSize) {
		PthreadMutexUnlock(&bufInuse);
		return;
	}

	/*
	 * Initialize buffer.
	 */
	if (lockBuffer) {
		SamValloc(bufFirst, bufSize);
		memset(bufFirst, 0, bufSize);
		if (mlock(bufFirst, bufSize) < 0) {
			LibFatal(mlock, "bufFirst");
		}
	} else {
		SamMalloc(bufFirst, bufSize);
	}

	/* Last address in buffer. */
	bufLast = bufFirst + bufSize;

	if (prevBufFirst != NULL) {
		if (bufOut != bufIn) {
			char	*p;
			int	n;

			/*
			 * Copy data from previous buffer.
			 */
			p = bufFirst;
			n = bufIn - bufOut;
			if (n < 0) {
				n = prevBufSize - bufOut;
				memmove(p, prevBufFirst + bufOut, n);
				p += n;
				bufOut = 0;
			}
			n = bufIn - bufOut;
			if (n > 0) {
				memmove(p, prevBufFirst + bufOut, n);
				p += n;
			}
			bufOut = 0;
			bufIn = Ptrdiff(p, bufFirst);
		}

		/*
		 * Release previous buffer.
		 */
		if (lockBuffer) {
			if (munlock(prevBufFirst, prevBufSize) < 0) {
				LibFatal(mlock, "prevBufFirst");
			}
		}
		SamFree(prevBufFirst);
	}
	ReadCount = bufSize / 2;
	Trace(TR_ARDEBUG,
	    "Block size %d, WriteCount %d, ReadCount %d, blockCount %d%s",
	    BlockSize, WriteCount, ReadCount, blockCount,
	    (lockBuffer) ? "(lock)" : "");
	PthreadMutexUnlock(&bufInuse);
}


/*
 * Thread - Write buffer to archive file.
 * This is a thread function that writes data that is in the circular
 * buffer to the archive file.  The data is written in WriteCount
 * blocks.
 */
void *
WriteBuffer(
	void *arg)
{
	ThreadsInitWait(writeStop, wakeup);

	PthreadMutexLock(&bufLock);
	while (bufFirst == NULL) {
		PthreadCondWait(&bufWrite, &bufLock);
	}
	PthreadMutexUnlock(&bufLock);

	/*
	 * Loop for all archive files.
	 */
	while (Exec == ES_run) {

		/*
		 * Loop for all files in an archive file.
		 */
		for (;;) {
			boolean_t was_full;
			int	l;
			int	n;

			/*
			 * Wait for data in buffer.
			 */
			l = WriteCount;
			PthreadMutexLock(&bufLock);
			for (;;) {
				n = bufIn - bufOut;
				if (n < 0) {
					n += bufSize;
				}
				if (n >= l) {
					/* At least a block available. */
					break;
				}
				if (bufEndArchive) {
					l = n;
					break;
				}
				bufEmpty = TRUE;
				PthreadCondWait(&bufWrite, &bufLock);
				if (Exec == ES_term) {
					PthreadMutexUnlock(&bufLock);
					goto out;
				}
			}
			PthreadMutexUnlock(&bufLock);

			/*
			 * Write any data in the buffer.
			 */
			if (l == 0) {
				break;
			}
			if (!writeBlock(l)) {
				/*
				 * Retry the write.
				 */
				continue;
			}
			VolsTable->entry[VolCur].VlLength += l;
			Instance->CiBytesWritten += l;

			/*
			 * Restart reader if necessary.
			 */
			PthreadMutexLock(&bufLock);
			bufOut += l;
			if (bufOut == bufSize) {
				bufOut = 0;
			}
			was_full = bufFull;
			bufFull = FALSE;
			if (was_full) {
				PthreadCondSignal(&bufRead);
			}
			PthreadMutexUnlock(&bufLock);
		}

		PthreadMutexLock(&bufLock);
		bufEndArchive = FALSE;
		PthreadCondSignal(&bufRead);
		PthreadMutexUnlock(&bufLock);
	}

out:
	if (AfFd >= 0) {
		if (close(AfFd) != 0) {
			IoFatal(close, ArVolName);
		}
		AfFd = -1;
	}

	/*
	 * If disk or honeycomb archiving, set complete flag.
	 */
	if (Instance->CiFlags & CI_disk) {
		DkArchiveDone();
#if !defined(_NoOSD_)
	} else if (Instance->CiFlags & CI_honeycomb) {
		HcArchiveDone();
#endif
	}

	/*
	 * If archiving to a remote host, disconnect from
	 * the rft connection.
	 */
	if (RemoteArchive.enabled) {
		SamrftDisconnect(RemoteArchive.rft);
		RemoteArchive.rft = NULL;
	}

	PthreadMutexLock(&bufLock);
	bufWriteEnd = TRUE;
	PthreadCondSignal(&bufRead);
	PthreadMutexUnlock(&bufLock);

	ThreadsExit();
	/*NOTREACHED*/
	return (arg);
}


/* Public functions. */


/*
 * Advance in.
 * Advance the circular buffer data in index.
 */
void
AdvanceIn(
	int nbytes)	/* Number of bytes to advance */
{
	boolean_t was_empty;

	fileOffset += nbytes;

	/*
	 * Let the write thread know that the buffer is not empty.
	 */
	PthreadMutexLock(&bufLock);
	bufIn += nbytes;
	if (bufIn >= bufSize) {
		bufIn -= bufSize;
	}
	was_empty = bufEmpty;
	bufEmpty = FALSE;
	if (was_empty) {
		PthreadCondSignal(&bufWrite);
	}
	PthreadMutexUnlock(&bufLock);
}


/*
 * Complete the archive file.
 * Round the buffer up to the media block size.
 * Force buffer termination.
 */
void
EndArchiveFile(
	int firstFile)
{
	if (fileOffset == 0) {
		/*
		 * Nothing written.
		 */
#if !defined(_NoOSD_)
		if (Instance->CiFlags & CI_honeycomb) {
			HcEndArchiveFile();
		}
#endif
		fileOffset = 0;
		return;
	}
	if ((Instance->CiFlags & CI_diskInstance) == 0) {
		PthreadMutexLock(&bufInuse);
		RoundBuffer(BlockSize);
		PthreadMutexUnlock(&bufInuse);
	}

	/*
	 * Tell the write thread to end the archive file.
	 */
	PthreadMutexLock(&bufLock);
	bufEndArchive = TRUE;
	PthreadCondSignal(&bufWrite);
	while (bufEndArchive && !bufWriteEnd) {
		PthreadCondWait(&bufRead, &bufLock);
	}
	PthreadMutexUnlock(&bufLock);
	fileOffset = 0;

	if (Instance->CiFlags & CI_disk) {
		DkEndArchiveFile();
#if !defined(_NoOSD_)
	} else if (Instance->CiFlags & CI_honeycomb) {
		HcEndArchiveFile();
#endif
	} else {
		RmEndArchiveFile(firstFile);
	}

	Trace(TR_QUEUE, "Archive file %d, %s bytes written",
	    Instance->CiArchives,
	    StrFromFsize(Instance->CiBytesWritten, 1, NULL, 0));
}


/*
 * Round up the number of bytes in the buffer.
 */
void
RoundBuffer(
	ssize_t nbytes)	/* Number of bytes to be rounded */
{
	size_t	excess;

	excess = fileOffset % nbytes;
	if (excess != 0) {
		ssize_t	rem;

		rem = nbytes - excess;
		while (rem > 0) {
			char	*p;
			size_t	n;

			n = rem;
			p = WaitRoom(n);
			if ((bufIn + n) > bufSize) {
				n = bufSize - bufIn;
			}
			memset(p, 0, n);
			AdvanceIn(n);
			rem -= n;
		}
	}
}


/*
 * Wait for room in the buffer.
 * RETURN: Position in buffer for data input.
 */
char *
WaitRoom(
	ssize_t nbytes)	/* Room required in buffer */
{
	PthreadMutexLock(&bufLock);
	PthreadMutexUnlock(&bufInuse);
	for (;;) {
		ssize_t	n;

		n = bufOut - bufIn;
		if (n <= 0) {
			n += bufSize;
		}
		if ((n - 1) > nbytes) {
			break;
		}
		/*
		 * Not enough room.
		 * Wait for the write thread to make some.
		 */
		bufFull = TRUE;
		PthreadCondWait(&bufRead, &bufLock);
	}
	PthreadMutexLock(&bufInuse);
	PthreadMutexUnlock(&bufLock);
	return (bufFirst + bufIn);
}


/*
 * Write data to the buffer.
 */
void
WriteData(
	void *buf_a,	/* Start of data */
	ssize_t nbytes)	/* Number of bytes to write */
{
	char	*buf;

	buf = (char *)buf_a;

	while (nbytes > 0) {
		char	*p;
		size_t	n;

		/* ReadCount bytes will fit in the buffer. */
		n = nbytes;
		if (n > ReadCount) {
			n = ReadCount;
		}
		p = WaitRoom(n);
		if ((bufIn + n) > bufSize) {
			n = bufSize - bufIn;
		}
		memmove(p, buf, n);
		AdvanceIn(n);
		buf += n;
		nbytes -= n;
	}
}


/*
 * Returns first address in circular i/o buffer.
 */
char *
GetBufFirst(void)
{
	return (bufFirst);
}


/*
 * Returns last address in circular i/o buffer.
 */
char *
GetBufLast(void)
{
	return (bufLast);
}


/*
 * Fatal error related to I/O operation.
 */
void
_IoFatal(
	const char *srcFile,		/* Caller's source file. */
	const int srcLine,		/* Caller's source line. */
	const char *functionName,	/* Name of failing function. */
	const char *functionArg)	/* Argument to function */
{
	SysError(srcFile, srcLine, "%s failed: %s", functionName, functionArg);
	exit(EXIT_FAILURE);
}



/* Private functions. */


/*
 * Begin writing the archive file.
 */
static void
beginArchiveFile(void)
{
	bufIn = bufOut = 0;
	if (Instance->CiFlags & CI_disk) {
		DkBeginArchiveFile();
#if !defined(_NoSTK_)
	} else if (Instance->CiFlags & CI_honeycomb) {
		HcBeginArchiveFile();
#endif
	} else {
		RmBeginArchiveFile();
	}
	if (bufFirst == NULL) {
		PthreadMutexUnlock(&bufInuse);
		CopyFileReconfig();
		PthreadMutexLock(&bufInuse);
		PthreadMutexLock(&bufLock);
		PthreadCondSignal(&bufWrite);
		PthreadMutexUnlock(&bufLock);
	}
}


/*
 * copy the directory data to the archive.
 */
static void
copyDir(
	struct sam_disk_inode *dp)
{
	static char *dirbuf = NULL;	/* Read buffer for directory entries */
	static size_t dirbufSize = 0;
	offset_t	size;

	size = File->AfFileSize;

	/*
	 *  Since we have to validate the directory entries, guarantee that
	 *  they are contiguous by reading them into a separate buffer.
	 *  They must be read in multiples of DIR_BLK.
	 */
	if (dirbuf == NULL) {
		dirbufSize = 4 * DIR_BLK;
		SamMalloc(dirbuf, dirbufSize);
	}
	while (size > 0) {
		char	*pd;
		int	dl;
		int	nd;
		int	n;

		SetTimeout(TO_read);
		n = read(s_fd, dirbuf, dirbufSize);
		ClearTimeout(TO_read);
		if (n <= 0) {
			Trace(TR_ERR, "read(%s/%s) %d",
			    MntPoint, File->f->FiName, n);
			File->AfFlags |= AF_error;
			return;
		}

		/*
		 * Validate directory data.
		 */
		dl = DIR_BLK;
		nd = 0;
		pd = dirbuf;
		while (nd < n) {
			nd += dl;
			if (nd <= n) {
				struct sam_dirval *dvp;

				pd += dl;
				dl = DIR_BLK;
				dvp = (struct sam_dirval *)(void *)
				    (pd - sizeof (struct sam_dirval));
				if (dvp->d_id.ino != dp->id.ino ||
				    dvp->d_id.gen != dp->id.gen ||
				    dvp->d_version != SAM_DIR_VERSION) {
					Trace(TR_ERR, "bad directory(%s/%s)",
					    MntPoint, File->f->FiName);
					File->AfFlags |= AF_error;
				}
			} else  dl = nd - n;
		}
		/* copy directory data */
		WriteData(dirbuf, n);
		size -= n;
	}
}


/*
 * copy the symlink data to the archive.
 * Write linkname as data.
 */
static void
copyLink(void)
{
	extern char linkname[];
	extern int link_l;

	File->AfFileSize = link_l;	/* File size established when */
					/* header written */
	WriteData(linkname, link_l);
}


/*
 * copy the regular file data to the archive.
 */
static void
copyRegular(
	struct sam_disk_inode *dp)
{
	boolean_t doCsum;
	offset_t size;

	if (S_ISSEGS(dp)) {
		File->AfSegNum = dp->rm.info.dk.seg.ord + 1;
	}
	if (dp->status.b.cs_gen) {
		/*
		 * Attribute is set indicating checksum should be generated,
		 * and license allows checksum feature.  Call routine for
		 * initialization.
		 */
		File->AfFlags |= AF_csummed;
		doCsum = TRUE;
		ChecksumInit(dp->cs_algo);
	} else {
		doCsum = FALSE;
	}

	size = File->AfFileSize;

	while (size > 0) {
		boolean_t dataRead;
		char	*p;
		ssize_t	l;
		ssize_t	n;

		l = ReadCount;
		if (size < l) {
			l = size;
		}
		p = WaitRoom(l);
		if ((bufIn + l) > bufSize) {
			l = bufSize - bufIn;
		}
		if (!(File->AfFlags & AF_error)) {
			SetTimeout(TO_read);
			if (!SparseFile) {
				dataRead = TRUE;
				n = read(s_fd, p, l);
			} else {
				dataRead = doCsum;
				n = ReadSparse(s_fd, p, l, &dataRead);
			}
			ClearTimeout(TO_read);
			if (n < l) {
				File->AfFlags |= AF_error;
				if (n == -1) {
					Trace(TR_ERR,
					    "Read(%s/%s) error.",
					    MntPoint, File->f->FiName);
					n = 0;
				} else  {
					if (n < 0) {
						Trace(TR_ERR,
						    "Read(%s/%s) error."
						    "Read %d bytes.",
						    MntPoint, File->f->FiName,
						    n);
						n = 0;
					} else {
						errno = 0;
						Trace(TR_ERR,
						    "Read(%s/%s) expected %d,"
						    "returned %d."
						    "File truncated.",
						    MntPoint, File->f->FiName,
						    l, n);
					}
				}
			}
		} else {
			/*
			 * Fill buffer with zeroes to allow the file to have
			 * the correct length in the tarball.
			 */
			memset(p, 0, l);
			n = l;
		}

		/*
		 * If required, checksum buffer.
		 */
		if (doCsum && n > 0) {
			ChecksumData(p, n);
		}
		if (dataRead) {
			AdvanceIn(n);
			size -= n;
		}
	}

	/*
	 * Check for a complete read.
	 */
	if (!(File->AfFlags & AF_error) && !SparseFile &&
	    !(File->f->FiFlags & FI_stagesim)) {
		char	buf;

		if (read(s_fd, &buf, 1) != 0) {
			File->AfFlags |= AF_error;
			errno = 0;
			Trace(TR_ERR, "Read(%s/%s) beyond original size."
			    "File extended", MntPoint, File->f->FiName);
		}
	}

	/*
	 * Wait for checksum to complete.
	 */
	if (doCsum && File->AfFileSize > 0) {
		ChecksumWait();
	}
}


/*
 * copy the removable media file information to the archive.
 */
static void
copyRmMedia(
	struct sam_disk_inode *dp)
{
	struct sam_resource_file *rfi;
	struct sam_ioctl_idresource sr;
	size_t	size;

	size = File->AfFileSize;
	SamMalloc(rfi, size);
	memset(rfi, 0, size);
	sr.id	= dp->id;
	sr.size = dp->psize.rmfile;
	sr.rp.ptr = rfi;
	if (ioctl(FsFd, F_IDRESOURCE, &sr) < 0) {
		Trace(TR_ERR, "ioctl:F_IDRESOURCE(%s/%s)", MntPoint,
		    File->f->FiName);
		File->AfFlags |= AF_error;
	} else {
		WriteData(rfi, size);
	}
	SamFree(rfi);
}


/*
 * copy the segment data to the archive.
 * Write list of segment ids as data.
 */
static void
copySegment(
	struct sam_disk_inode *dp)
{
	struct sam_ioctl_idseginfo ss;
	size_t	size;
	char *sfi;

	if (dp->status.b.cs_gen) {
		File->AfFlags |= AF_csummed;
	}
	size = File->AfFileSize;
	SamMalloc(sfi, size);
	memset(sfi, 0, size);
	ss.id	   = dp->id;
	ss.size    = size;
	ss.buf.ptr = sfi;
	ss.offset  = 0;
	if (ioctl(FsFd, F_IDSEGINFO, &ss) < 0) {
		Trace(TR_ERR, "ioctl:F_IDSEGMENT(%s/%s)", MntPoint,
		    File->f->FiName);
		File->AfFlags |= AF_error;
	} else {
		WriteData(sfi, size);
	}
	SamFree(sfi);
}


/*
 * Write next block.
 */
static boolean_t	/* TRUE if a successful write */
writeBlock(
	size_t nbytes)
{
	ssize_t	bytesWritten;
	void	*buf;

	buf = bufFirst + bufOut;
	SetTimeout(TO_write);
	if (Instance->CiFlags & CI_disk) {
		bytesWritten = DkWrite(RemoteArchive.rft, buf, nbytes);
#if !defined(_NoOSD_)
	} else if (Instance->CiFlags & CI_honeycomb) {
		bytesWritten = HcWrite(RemoteArchive.rft, buf, nbytes);
#endif
	} else if (RemoteArchive.enabled) {
		bytesWritten = SamrftWrite(RemoteArchive.rft, buf, nbytes);
	} else {
		bytesWritten = RmWrite(AfFd, buf, nbytes);
	}
	ClearTimeout(TO_write);

	if (bytesWritten == nbytes) {
		VolBytesWritten += nbytes;
		return (TRUE);
	}

	if (Instance->CiFlags & CI_disk) {
		DkArchiveDone();
		if (DkWriteError() == 0) {
			return (FALSE);
		}
#if !defined(_NoOSD_)
	} else if (Instance->CiFlags & CI_honeycomb) {
		HcArchiveDone();
		if (HcWriteError() == 0) {
			return (FALSE);
		}
#endif
	} else if (RmWriteError() == 0) {
		return (FALSE);
	}
	exit(EXIT_FAILURE);
/* LINTED function falls off bottom without returning value */
}


/*
 * Wakeup Write().
 */
static void
wakeup(void)
{
	PthreadMutexLock(&bufLock);
	PthreadCondSignal(&bufWrite);
	PthreadMutexUnlock(&bufLock);
}


/*
 * Stop Write().
 */
static void
writeStop(void)
{
	PthreadMutexLock(&bufLock);
	if (bufFirst == NULL) {
		static char notNull;

		/* Needed for stopping */
		bufFirst = &notNull;
	}
	bufEndArchive = TRUE;	/* Make write loop exit */
	PthreadCondSignal(&bufWrite);
	PthreadMutexUnlock(&bufLock);
}
