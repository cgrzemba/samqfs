/*
 * disk_cache.c - write staged data to disk cache
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

#pragma ident "$Revision: 1.3 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <values.h>
#include <assert.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <pthread.h>

/* Solaris headers. */
#include <sys/shm.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "aml/stager.h"
#include "aml/stager_defs.h"
#include "sam/uioctl.h"
#include "sam/syscall.h"
#include "sam/custmsg.h"
#include "sam/lib.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Local headers. */
#include "stager_config.h"
#include "stager_lib.h"
#include "stager_threads.h"
#include "stager_shared.h"
#include "rmedia.h"
#include "stream.h"
#include "copy_defs.h"
#include "file_defs.h"

#include "copy.h"
#include "circular_io.h"

/* Public data. */
extern CopyInstanceInfo_t *Instance;
extern IoThreadInfo_t *IoThread;
extern int NumOpenFiles;

/* Private data. */
static longlong_t dataToWrite;	/* total bytes to write to disk cache */
static int readErrno;		/* non-zero if read or positioning error */
static boolean_t cancel;	/* true if canceled by file system */

/* Test for more data to write. */
#define	DATA_TO_WRITE() (dataToWrite > 0 && readErrno == 0 && \
    cancel == B_FALSE)

/* Private data. */

/* Private functions. */
static boolean_t ifChecksum(FileInfo_t *file);
static boolean_t ifMultiVolume(FileInfo_t *file);
static boolean_t ifVerify(FileInfo_t *file);
static int openStage(FileInfo_t *file);
static int openVerify(FileInfo_t *file);
static void setVerify(FileInfo_t *file);


/*
 * Open disk cache file.
 */
int
DiskCacheOpen(
	FileInfo_t *file)
{
	int fd;

	/*
	 * If saved, restore open disk cache file descriptor.
	 * This occurs when processing multivolume.
	 */
	if (GET_FLAG(file->flags, FI_DCACHE) && file->dcache > 0) {

		fd = file->dcache;
		Trace(TR_FILES, "Restore disk cache fd: %d", fd);

	} else if (ifVerify(file) == B_TRUE) {

		fd = openVerify(file);
		Trace(TR_FILES, "Verify disk cache fd: %d", fd);

	} else {

		fd = openStage(file);
		Trace(TR_FILES, "Stage disk cache fd: %d", fd);
	}

	return (fd);
}

/*
 * Write data in buffer to disk cache file.
 *
 * This thread is a consumer for the double buffer thread.
 * A conumser must wait until the buffer is not empty, retrieve its
 * data, and then notify the producer that the buffer is not full.
 */
int
DiskCacheWrite(
	FileInfo_t *file,
	int fd)
{
	CircularBuffer_t *writer;
	boolean_t checksum;	/* use checksum enabled on file */
	boolean_t multivolume;	/* staging a multivolume file */
	boolean_t verify;	/* staging a file for data verification */
	sam_ioctl_swrite_t swrite;
	int position;		/* block position for debugging */
	char *out;		/* output buffer pointer */
	int nbytes;
	int nwritten;

/* FIXME comments */
	int copy;
	boolean_t closeDiskcache;
	/* LINTED variable unused in function */
	time_t endTime;
	/* LINTED variable unused in function */
	int secs;

	/* Wait for file to stage. */
	ThreadStateWait(&IoThread->io_writeReady);

	writer = IoThread->io_writer;

	copy = file->copy;

	memset(&swrite, 0, sizeof (sam_ioctl_swrite_t));

	checksum = ifChecksum(file);
	verify = ifVerify(file);
	multivolume = ifMultiVolume(file);

	dataToWrite = IoThread->io_size;

	cancel = B_FALSE;
	readErrno = 0;
	position = 0;		/* block written to disk for file */

	Trace(TR_FILES, "Write disk inode: %d.%d len: %lld",
	    file->id.ino, file->id.gen, dataToWrite);

	while (DATA_TO_WRITE()) {

		/*
		 * Wait until the write buffer is not empty.
		 * Retrieve data at 'out' position.
		 * If archive read thread found an error during a read
		 * from media, positioning failure or tar header validation
		 * the error flag will be set in io thead control structure.
		 */
		out = CircularIoAvail(writer, &nbytes, &readErrno);

		/* If not a full block of data left to write. */
		if (dataToWrite < nbytes) {
			nbytes = dataToWrite;
		}

		Trace(TR_DEBUG,
		    "Write block: %d buf: %d [0x%x] offset: %lld len: %d",
		    position, CircularIoSlot(IoThread->io_writer, out),
		    (int)out, swrite.offset, nbytes);

		swrite.buf.ptr = out;
		swrite.nbyte = nbytes;

		/* Accumulate checksum value. */
		if (checksum == B_TRUE) {
			Checksum(out, nbytes);
		}

		/* Write data block to disk cache. */
		if (verify == B_FALSE) {
			nwritten = ioctl(fd, F_SWRITE, &swrite);

			/* FIXME write fails */
			ASSERT_WAIT_FOR_DBX(nwritten == nbytes);
		}

		/*
		 * Wait for checksum thread to complete on the data block
		 * before allowing the double buffer thread to reuse
		 * the 'out' buffer.
		 */
		if (checksum == B_TRUE && nbytes > 0) {
			ChecksumWait();
		}

		/*
		 * Write complete.  Advance write buffer's 'out'
		 * pointer and notify double buffer thread that the buffer
		 * is not empty.
		 */
		CircularIoAdvanceOut(writer);

		file->stage_size += nbytes;
		swrite.offset += nbytes;
		dataToWrite -= nbytes;
		ASSERT_WAIT_FOR_DBX(dataToWrite >= 0);

		position++;

		Trace(TR_DEBUG, "Write %d bytes left: %lld (%d/%d)",
		    nbytes, dataToWrite, readErrno, cancel);
	}

	Trace(TR_FILES, "Write disk complete inode: %d.%d",
	    file->id.ino, file->id.gen);

	/*
	 * If no error AND cancel request, close disk cache file.
	 * If no error AND no more VSNs,   close disk cache file.
	 * If error OR more VSNs to stage, save disk cache information.
	 */

	/*
	 * There are a number of scenarios under which to determine
	 * whether to close the disk cache file or leave it open for
	 * a future request.
	 *
	 * If no error AND cancel,		close disk cache file.
	 * If no error AND not multivolume,	close disk cache file.
	 * If no error AND multivolume AND stage_n, close disk cache file.
	 * If error OR more VSNs to stage, 	save disk cache information.
	 */

	/* If no error AND cancel, close disk cache file. */
	if (readErrno == 0 && cancel == B_TRUE) {
		closeDiskcache = B_TRUE;

	/* If no error AND not multivolume, close disk cache file. */
	} else if (readErrno == 0 && multivolume == 0) {
		closeDiskcache = B_TRUE;

	/* If no error AND multivolume AND stage_n, close disk cache file. */
	} else if (readErrno == 0 && multivolume != 0 &&
	    GET_FLAG(file->flags, FI_STAGE_NEVER) &&
	    file->stage_size == file->len) {

		closeDiskcache = B_TRUE;

	/* Else, an error OR more VSNs to stage, save disk cache information. */
	} else {
		/*
		 * If no device available or interrupted system call,
		 * close disk cache.
		 */
		if (readErrno == ENODEV || readErrno == EINTR) {
			closeDiskcache = B_TRUE;
		} else {
			closeDiskcache = B_FALSE;
		}
	}

	/*
	 * Checksumming may have found an error.
	 */
	if (closeDiskcache == B_TRUE && checksum == B_TRUE) {
		/*
		 * Do not check for checksum error if request was
		 * cancelled or an I/O error occurred.
		 */
		if (cancel == B_FALSE && readErrno == 0) {

			readErrno = ChecksumCompare(fd, &file->id);

			if (readErrno != 0) {
				closeDiskcache = B_FALSE;
				swrite.offset = 0;

				if (verify == B_TRUE) {
					SetErrno = 0;	/* set for trace */
					Trace(TR_ERR, "Unable to verify "
					    "inode: %d.%d copy: %d errno: %d",
					    file->id.ino, file->id.gen,
					    copy + 1, readErrno);

					closeDiskcache = B_TRUE;
				}
			} else {
				if (verify == B_TRUE) {
					setVerify(file);
				}
			}

		} else {
			if (verify == B_TRUE) {
				SetErrno = 0;	/* set for trace */
				Trace(TR_ERR, "Unable to verify "
				    "inode: %d.%d copy: %d errno: %d",
				    file->id.ino, file->id.gen,
				    copy + 1, errno);
			}
		}
		checksum = B_FALSE;
	}

	if (closeDiskcache == B_TRUE) {
		(void) close(fd);
		CLEAR_FLAG(file->flags, FI_DCACHE);
		NumOpenFiles--;

	} else {
		SetFileError(file, fd, swrite.offset, readErrno);
		if (checksum == B_TRUE && multivolume != 0) {
			file->csum_val = ChecksumGetVal();
		}
	}

	/* Done writing staged file. */
	ThreadStatePost(&IoThread->io_writeDone);

	return (file->error);
}


/*
 * Return true if using checksum on staged file.
 * Initialize checksum for the staged file.
 */
static boolean_t
ifChecksum(
	FileInfo_t *file)
{
	boolean_t retval;
	boolean_t alreadyInitialized;

	retval = B_FALSE;
	if (GET_FLAG(file->flags, FI_USE_CSUM)) {
		retval = B_TRUE;
		alreadyInitialized = GET_FLAG(file->flags, FI_DCACHE) &&
		    file->write_off != 0;

		ChecksumInit(file, alreadyInitialized);

		if (alreadyInitialized == B_TRUE) {
			ChecksumSetVal(file->csum_val);
		}
	}
	return (retval);
}

/*
 * Return true if staging file spanning multiple volumes.
 */
static boolean_t
ifMultiVolume(
	FileInfo_t *file)
{
	boolean_t retval;

	retval = B_FALSE;
	if (GET_FLAG(file->flags, FI_MULTIVOL) != 0) {
		retval = B_TRUE;
	}
	return (retval);
}

/*
 * Return true if staging file for data verification.
 */
static boolean_t
ifVerify(
	FileInfo_t *file)
{
	int copy;
	boolean_t retval;

	retval = B_FALSE;
	copy = file->copy;
	if (GET_FLAG(file->ar[copy].flags, STAGE_COPY_VERIFY) != 0) {
		retval = B_TRUE;
	}
	return (retval);
}

/*
 * Open inode for staging file.
 */
static int
openStage(
	FileInfo_t *file)
{
	int fd;
	sam_fsstage_arg_t arg;

	memset(&arg.handle, 0, sizeof (sam_handle_t));
	arg.handle.id = file->id;
	arg.handle.fseq = file->fseq;

	arg.handle.stage_off = file->fs.stage_off;
	arg.handle.stage_len = file->len;
	arg.handle.flags.b.stage_wait = file->fs.wait;
	arg.directio = file->directio;
	arg.ret_err = 0;

	/*
	 * If this is a duplicate stage request, send an EEXIST
	 * error response back to filesystem.
	 */
	if (GET_FLAG(file->flags, FI_DUPLICATE)) {
		arg.ret_err = EEXIST;
		Trace(TR_MISC, "Duplicate inode: %d.%d",
		    file->id.ino, file->id.gen);
	}

	/*
	 * Stage response syscall may take awhile if trying
	 * to allocate disk space.  Set flag to indicate
	 * this state.
	 */
	PthreadMutexLock(&file->mutex);
	SET_FLAG(file->flags, FI_OPENING);
	PthreadMutexUnlock(&file->mutex);

	fd = sam_syscall(SC_fsstage, &arg, sizeof (sam_fsstage_arg_t));

	/*
	 * If duplicate, filesystem returns zero in response to EEXIST
	 * error.  Set file descriptor to -1 indicating open failure.
	 */
	if (GET_FLAG(file->flags, FI_DUPLICATE)) {
		fd = -1;
	}

	if (fd >= 0) {
		/*
		 * Copy process holds an open file descriptor.
		 * Update open file count so its not timed out during
		 * error recovery or multivolume.
		 */
		NumOpenFiles++;

	} else {
		if (errno == ECANCELED) {
			/*
			 * Cancel stage request from filesystem.
			 * This will be picked up in the reader thread.
			 */
			Trace(TR_MISC,
			    "Canceled(SC_fsstage) inode: %d.%d",
			    file->id.ino, file->id.gen);

			SET_FLAG(IoThread->io_flags, IO_cancel);

			/*
			 * Set cancel flag in file, a duplicate request
			 * needs this status.
			 */
			SET_FLAG(file->flags, FI_CANCEL);

		}
	}
	CLEAR_FLAG(file->flags, FI_OPENING);

	return (fd);
}

/*
 * Open inode for data verification.
 */
static int
openVerify(
	FileInfo_t *file)
{
	int fd;
	int mpfd;
	char *mount_name;
	struct sam_ioctl_idopen arg;

	fd = -1;
	mount_name = GetMountPointName(file->fseq);

	if (mount_name == NULL ||
	    (mpfd = open(mount_name, O_RDONLY)) < 0) {

		SetErrno = 0;	/* set for trace */
		Trace(TR_ERR, "Unable to determine mount point "
		    "inode: %d.%d fseq: %d mp: \"%s\" errno: %d",
		    file->id.ino, file->id.gen, file->fseq,
		    mount_name == NULL ? "null" : mount_name, errno);

		file->error = ENODEV;

	} else {

		memset(&arg, 0, sizeof (arg));
		arg.id = file->id;
		if (file->directio) {
			arg.flags |= IDO_direct_io;
		}

		fd = ioctl(mpfd, F_IDOPEN, &arg);

		if (fd >= 0) {
			/*
			 * Copy process holds an open file descriptor.
			 * Update open file count so its not timed out during
			 * error recovery or multivolume.
			 */
			NumOpenFiles++;
		} else {
			SetErrno = 0;	/* set for trace */
			Trace(TR_ERR, "Idopen failed inode: %d.%d "
			    "copy: %d errno: %d",
			    file->id.ino, file->id.gen,
			    file->copy + 1, errno);

			file->error = ENODEV;
		}
		(void) close(mpfd);
	}

	return (fd);
}

/*
 * Use the SC_archive_copy syscall to set the data verified bit.
 */
static void
setVerify(
	FileInfo_t *file)
{
	struct sam_archive_copy_arg arg;
	char pathBuffer[PATHBUF_SIZE];
	int segment_ord;
	int syserr;
	int copy;

	GetFileName(file, &pathBuffer[0], PATHBUF_SIZE, &segment_ord);

	/* If segmented file remove ordinal from file name. */
	if (segment_ord > 0) {
		char *p;

		p = strrchr(&pathBuffer[0], '/');
		if (p != NULL) {
			*p = '\0';
		}
	}

	copy = file->copy;

	memset(&arg, 0, sizeof (arg));
	arg.copies = 1 << copy;
	arg.operation = OP_verified;
	arg.path.ptr = pathBuffer;

	syserr = sam_syscall(SC_archive_copy, &arg, sizeof (arg));
	if (syserr == 0) {
		/* Success, clear verify flag for file. */
		file->ar[copy].flags &= ~STAGE_COPY_VERIFY;
	} else {
		WarnSyscallError(HERE, "SC_archive_copy", pathBuffer);
	}
}
