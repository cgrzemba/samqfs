/*
 * copyfile.c - copy data from removable media to disk cache
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

#pragma ident "$Revision: 1.110 $"

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
#include "aml/shm.h"
#include "aml/tar.h"
#include "aml/tar_hdr.h"
#include "pub/stat.h"
#include "sam/fioctl.h"
#include "sam/uioctl.h"
#include "pub/rminfo.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/opticals.h"
#include "sam/resource.h"
#include "sam/syscall.h"
#include "sam/exit.h"
#include "sam/custmsg.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "aml/stager.h"
#include "aml/stager_defs.h"
#include "aml/id_to_path.h"
#include "aml/sam_rft.h"
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
extern CopyInstanceList_t *CopyInstanceList;
extern int NumOpenFiles;

/* Communication control for io threads. */
IoThreadInfo_t *IoThread = NULL;

/* Active request for process. */
CopyRequestInfo_t *Request = NULL;

/* Active stream for process. */
StreamInfo_t *Stream = NULL;

/* Check if stream is available. */
#define	STREAM_IS_AVAILABLE() (!(GET_FLAG(Stream->flags, SR_UNAVAIL)) && \
	!(GET_FLAG(Stream->flags, SR_DCACHE_CLOSE)))

/* Check if stream is valid. */
#define	STREAM_IS_VALID(sr) ((Stream->first >= 0) &&	\
	(GET_FLAG(Stream->flags, SR_ERROR) == 0) &&	\
	(GET_FLAG(Stream->flags, SR_CLEAR) == 0) &&	\
	(GET_FLAG(Instance->ci_flags, CI_failover) == 0))

/* Private data. */
/* Temporary space for building path name. */
static upath_t fullpath;

/* Copy request path name.  One file per copy process. */
static char *copyRequestPath;

/* Private functions. */
static void checkBuffers(char *new_vsn);
static void allocBuffers();
static void freeBuffers();

static void copyStream();
static void closeStream();
static void removeDcachedFile(StreamInfo_t *stream, int error);
static boolean_t ifMaxStreamErrors(FileInfo_t *file);
static void rejectRequest(int error, boolean_t clean);

static void initIoThread();
static void setIoThread(FileInfo_t *file);

/*
 * Enter loop to copy all files in stream.  This function waits for
 * and services copy request from the scheduler.
 */
void
CopyFiles(void)
{
	pthread_t tid;
	int rval;
	struct timespec timeout;

	ASSERT(Instance != NULL);

	(void) sprintf(fullpath, "%s/%s/rm%d/%s", SAM_VARIABLE_PATH,
	    STAGER_DIRNAME, Instance->ci_rmfn, COPY_FILE_FILENAME);
	SamStrdup(copyRequestPath, fullpath);

	/* Initialize structure for communcation between io threads. */
	initIoThread();

	/* Start io threads */
	rval = pthread_create(&tid, NULL, ArchiveRead, NULL);
	if (rval != 0) {
		LibFatal(pthread_create, "ArchiveRead");
	}
	rval = pthread_create(&tid, NULL, DoubleBuffer, NULL);
	if (rval != 0) {
		LibFatal(pthread_create, "DoubleBuffer");
	}

	for (;;) {

		timeout.tv_sec = time(NULL) + COPY_INSTANCE_TIMEOUT_SECS;
		timeout.tv_nsec = 0;

		PthreadMutexLock(&Instance->ci_requestMutex);
		Instance->ci_busy = B_FALSE;

		/*
		 * Wait for a request from daemon to stage files.
		 */
		while (Instance->ci_first == NULL && !IF_SHUTDOWN(Instance)) {
			pid_t ppid;

			/*
			 * Exit if parent stager daemon died
			 * or reconfig requested.
			 */
			if (((ppid = getppid()) == 1) ||
			    (CopyInstanceList->cl_reconfig == B_TRUE)) {
				SetErrno = 0;	/* set for trace */
				Trace(TR_ERR,
				    "Detected stager daemon %s",
				    ppid == 1 ? "exit" : "reconfig");
				SET_FLAG(Instance->ci_flags, CI_shutdown);
				break;
			}

			rval = pthread_cond_timedwait(&Instance->ci_request,
			    &Instance->ci_requestMutex, &timeout);

			if (rval == ETIMEDOUT) {
				/*
				 * Wait timed out.  If no open disk cache files
				 * set shutdown flag so we exit loop.  If open
				 * files, reset timeout value and wait for work.
				 */
				if (NumOpenFiles <= 0 &&
				    Instance->ci_first == NULL) {

					SET_FLAG(Instance->ci_flags,
					    CI_shutdown);
					/*
					 * Set instance to 'busy' while this
					 * process is exiting and the stager
					 * daemon detects that it exited.
					 */
					Instance->ci_busy = B_TRUE;
				} else {
					timeout.tv_sec = time(NULL) +
					    COPY_INSTANCE_TIMEOUT_SECS;
					timeout.tv_nsec = 0;
				}
			}
		}

		/* If shutdown requested exit loop. */
		if (IF_SHUTDOWN(Instance)) {
			Trace(TR_PROC, "Shutdown instance: 0x%x first: 0x%x",
			    (int)Instance, (int)Instance->ci_first);
			PthreadMutexUnlock(&Instance->ci_requestMutex);
			break;
		}

		/*
		 * Received a copy request from stager daemon.  Initialize
		 * ourself and notify daemon that we got it.
		 */
		Request = (CopyRequestInfo_t *)MapInFile(copyRequestPath,
		    O_RDWR, NULL);

		if (Request == NULL) {
			Trace(TR_ERR, "Cannot map in file %s", copyRequestPath);
			if (errno == EMFILE) {
				/*
				 * Open max reached. This should not happen.
				 * Stream will be requeued by parent.
				 */
				PthreadMutexUnlock(&Instance->ci_requestMutex);
				LibFatal(MapInFile, "EMFILE");
			}
			continue;
		}

		/* Grab next request from the queue. */
		Instance->ci_first = Request->cr_next;
		if (Instance->ci_first == NULL) {
			Instance->ci_last = NULL;
		}

		Trace(TR_QUEUE, "Got request: '%s.%d' 0x%x",
		    Request->cr_vsn, Request->cr_seqnum, (int)Request);

		/* Generate path name and map in the stream. */
		(void) sprintf(fullpath, "%s/%s.%d",
		    SharedInfo->si_streamsDir,
		    Request->cr_vsn, Request->cr_seqnum);

		Stream = (StreamInfo_t *)MapInFile(fullpath, O_RDWR, NULL);
		if (Stream == NULL) {
			Trace(TR_ERR, "Cannot map in file %s", fullpath);
			if (errno == EMFILE) {
				/*
				 * Open max reached. This should not happen.
				 * Stream will be requeued by parent.
				 */
				PthreadMutexUnlock(&Instance->ci_requestMutex);
				LibFatal(MapInFile, "EMFILE");
			}
			continue;
		}

		Instance->ci_vsnLib = Stream->vi.lib;
		Instance->ci_busy = B_TRUE;

		/* Notify daemon we got the copy request. */
		Request->cr_gotItFlag = B_TRUE;
		PthreadCondSignal(&Request->cr_gotIt);

		PthreadMutexUnlock(&Instance->ci_requestMutex);

		/* Stage all files in stream. */
		if (STREAM_IS_AVAILABLE()) {
			copyStream();

		} else {
			/*
			 * Resources for the stream are not available.
			 * Close open file descriptors.
			 */
			closeStream();
		}
	}
}

/*
 * Invalidate data in i/o buffers.
 */
void
ResetBuffers(void)
{
	Trace(TR_DEBUG, "Reset buffers");

	CircularIoReset(IoThread->io_reader);
	CircularIoReset(IoThread->io_writer);
}

/*
 * Set error condition for specified file.
 */
void
SetFileError(
	FileInfo_t *file,
	int fd,
	offset_t write_off,
	int error)
{
		file->dcache = fd;
		file->write_off = write_off;
		file->error = error;
		if (GET_FLAG(file->flags, FI_MULTIVOL) && error == 0) {
			file->vsn_cnt++;
		}
		file->context = Instance->ci_pid;
		SET_FLAG(file->flags, FI_DCACHE);
		Trace(TR_FILES, "Set file dcache: %d write_off: %lld error: %d",
		    fd, write_off, error);
		Trace(TR_FILES, "\tvsn_cnt: %d context: %d",
		    file->vsn_cnt, (int)file->context);
}

/*
 * Allocate i/o buffers for copy.  Size of each buffers is determined
 * from type of removable media file's block size.
 */
static void
allocBuffers(void)
{
	int blockSize;
	int numBuffers;
	boolean_t lockbuf;

	ASSERT(IoThread->io_numBuffers == 0);

	blockSize = GetBlockSize();
	if (blockSize == 0) {
		SetErrno = 0;		/* set for trace */
		Trace(TR_ERR, "Block size is zero");
		blockSize = 16 *1024;
	}

	lockbuf = Instance->ci_lockbuf;
	numBuffers = Instance->ci_numBuffers;

	IoThread->io_reader = CircularIoConstructor(numBuffers, blockSize,
	    lockbuf);
	IoThread->io_writer = CircularIoConstructor(numBuffers, blockSize,
	    lockbuf);

	Trace(TR_FILES, "Alloc buffers num: %d block size: %d %s",
	    numBuffers, blockSize, lockbuf ? "(lock)" : "");

	/*
	 * Set media allocation unit information.  A media allocation unit
	 * is defined as the size of the smallest addressable part on a
	 * media that can be accessed.  Buffers are a multiple of the
	 * ma unit size.
	 */
	IoThread->io_numBuffers = numBuffers;
	IoThread->io_blockSize = blockSize;
}

/*
 * Free i/o buffers for copy.
 */
static void
freeBuffers(void)
{
	CircularIoDestructor(IoThread->io_reader);
	CircularIoDestructor(IoThread->io_writer);

	IoThread->io_numBuffers = 0;
	IoThread->io_blockSize = 0;
}

/*
 * New media loaded.  Check if data in buffers
 * is validate and can be reused.
 */
static void
checkBuffers(
	char *new_vsn)
{
	u_longlong_t currentPos;

	/* If first request for this thread, allocate buffers. */
	if (IoThread->io_numBuffers == 0) {
		allocBuffers();
		strcpy(Instance->ci_vsn, new_vsn);
		return;
	}

	/* If disk archiving, invalidate buffers. */
	if (IoThread->io_flags & IO_diskArchiving) {
		ResetBuffers();
		IoThread->io_position = 0;
		strcpy(Instance->ci_vsn, new_vsn);
		return;
	}

	/* Get current position of removable media file. */
	currentPos = GetPosition();

	Trace(TR_FILES, "Get position %lld block size %d",
	    currentPos, GetBlockSize());

	/*
	 * If we loaded the same VSN as last time this proc was
	 * active, the buffers may be valid and thus data in the buffers
	 * can be reused.  If this is not the same VSN, invalidate
	 * the buffers and save VSN label in the thread's context.
	 */
	if (strcmp(Instance->ci_vsn, new_vsn) != 0) {
		int blockSize;

		/*
		 * New media.  If block size has changed set new mau
		 * information.  If necessary, reallocate buffers based
		 * on the new block size.
		 */
		blockSize = GetBlockSize();
		if (IoThread->io_blockSize != blockSize) {
			freeBuffers();
			allocBuffers();
		} else {
			ResetBuffers();
		}

		strcpy(Instance->ci_vsn, new_vsn);

	} else if (IoThread->io_position != currentPos) {

		/*
		 * The last known media position does not match the
		 * removable media file's position.  Since the position has
		 * changed, buffers must be invalidated.
		 */
		ResetBuffers();

	}

	/*
	 * Archive read thread needs mount position to maintain
	 * position on the media.
	 */
	IoThread->io_position = currentPos;
}

/*
 * Stage all files in the stream.
 */
static void
copyStream()
{
	int rval;
	FileInfo_t *file;
	int dcache;
	boolean_t reject;

	StageInit(Stream->vsn);

	/* Set loading flag for this stream. */
	PthreadMutexLock(&Stream->mutex);
	SET_FLAG(Stream->flags, SR_LOADING);
	PthreadMutexUnlock(&Stream->mutex);

	rval = LoadVolume();

	/* Reject if mount/open failed. */
	if (rval != 0) {
		PthreadMutexLock(&Stream->mutex);
		removeDcachedFile(Stream, rval);

		if (rval == ENODEV) {
			Stream->context = 0;
			PthreadMutexUnlock(&Stream->mutex);
			rejectRequest(0, B_TRUE);
			SET_FLAG(Instance->ci_flags, CI_shutdown);
		} else {
			PthreadMutexUnlock(&Stream->mutex);
			SendCustMsg(HERE, 19017, Stream->vsn);
			rejectRequest(rval, B_TRUE);
		}

		StageEnd();
		return;
	}

	/* VSN load has completed. */
	checkBuffers(Stream->vsn);

	PthreadMutexLock(&Stream->mutex);
	CLEAR_FLAG(Stream->flags, SR_LOADING);

	Instance->ci_seqnum = Stream->seqnum;
	reject = B_FALSE;

	/*
	 * Copy all files in stage stream request.  The files have
	 * been ordered to eliminate backward media positioning.
	 */

	while (STREAM_IS_VALID() && reject == B_FALSE) {

		/* Stop staging if parent died. */
		if (getppid() == 1) {
			SetErrno = 0;		/* set for trace */
			Trace(TR_ERR, "Detected stager daemon exit");

			Stream->first = EOS;
			SET_FLAG(Instance->ci_flags, CI_shutdown);
			break;
		}

		file = GetFile(Stream->first);

		PthreadMutexLock(&file->mutex);
		PthreadMutexUnlock(&Stream->mutex);

		/*
		 * If the first vsn, clear bytes read count.
		 * And if multivolume and stage -n set, initialize
		 * residual length.
		 */
		if (file->vsn_cnt == 0) {
			file->read = 0;
			if (GET_FLAG(file->flags, FI_MULTIVOL) &&
			    GET_FLAG(file->flags, FI_STAGE_NEVER)) {
				file->residlen = file->len;
			} else {
				file->residlen = 0;
			}
		}

		SET_FLAG(file->flags, FI_ACTIVE);

		PthreadMutexUnlock(&file->mutex);

		/* Set file in io control structure for archive read thread. */
		setIoThread(file);

		/* Log stage start. */
		file->eq = IoThread->io_drive;
		LogIt(LOG_STAGE_START, file);

		/*
		 * Check if last request was canceled.  If the last request
		 * was canceled, invalidate i/o buffers and clear cancel
		 * flag in the control structure.
		 */
		if (GET_FLAG(IoThread->io_flags, IO_cancel)) {
			ResetBuffers();
			CLEAR_FLAG(IoThread->io_flags, IO_cancel);
		}

		/*
		 * Next archive file.  If disk archive, we may be opening
		 * a disk archive tarball.
		 */
		if ((rval = NextArchiveFile()) == 0) {
			/* Prepare filesystem to receive staged file. */
			dcache = DiskCacheOpen(file);

		} else {
			/* Unable to open disk archive.  Error request. */
			SetErrno = 0;	/* set for trace */
			Trace(TR_ERR, "Unable to open disk archive "
			    "copy: %d inode: %d.%d errno: %d",
			    file->copy + 1, file->id.ino, file->id.gen, errno);
			dcache = -1;
			file->error = errno;
			SendErrorResponse(file);
		}

		if (dcache >= 0 && rval == 0) {
			/*
			 * Notify reader thread that next file in stream
			 * is ready to be staged.
			 */
			ThreadStatePost(&IoThread->io_readReady);

			/* Write data to disk cache. */
			rval = DiskCacheWrite(file, dcache);

			if (rval != 0) {
				SendErrorResponse(file);

				/* Check if number of stream errors exceeded. */
				reject = ifMaxStreamErrors(file);
			}

			ThreadStateWait(&IoThread->io_readDone);

		} else if (rval != 0 && dcache >= 0) {
			/* Setup for error handling. */
			SetFileError(file, dcache, 0, EIO);
			SendErrorResponse(file);
		}

		EndArchiveFile();

		/* Remove file from stream before marking it as done. */
		PthreadMutexLock(&Stream->mutex);
		Stream->first = file->next;

		/* Device not available. */
		if (file->error == ENODEV) {
			CLEAR_FLAG(file->flags, FI_DCACHE);
			SetErrno = 0;	/* set for trace */
			Trace(TR_ERR, "No device available");

			Stream->first = EOS;
			SET_FLAG(Instance->ci_flags, CI_shutdown);
		}

		/* Mark file staging as done. */
		SetStageDone(file);
		Stream->count--;

		if (Stream->first == EOS) {
			Stream->last = EOS;
		}

	}

	/* Reject rest of stages in this stream. */
	if (reject == B_TRUE) {
		if (Stream->first > EOS) {
			removeDcachedFile(Stream, ENODEV);
		}
		PthreadMutexUnlock(&Stream->mutex);
		rejectRequest(ENODEV, B_FALSE);
		PthreadMutexLock(&Stream->mutex);
	}

	/* Remove copy request, no one is waiting on it. */
	RemoveMapFile(copyRequestPath, Request, sizeof (CopyRequestInfo_t));
	Request = NULL;

	/* Ready to unload.  Mark stream as done. */
	SET_FLAG(Stream->flags, SR_DONE);
	PthreadMutexUnlock(&Stream->mutex);

	UnloadVolume();

	/*
	 * Unmap pages of memory.  Stream's memory
	 * mapped file is removed in parent.
	 */
	UnMapFile(Stream, sizeof (StreamInfo_t));
	Stream = NULL;

	StageEnd();
}

/*
 * Stream contains open file descriptors that could not be completed.
 * If stream has only pending open file descriptors close file
 * descriptors.
 */
static void
closeStream()
{
	FileInfo_t *file;
	int id;

	id = Stream->first;
	while (id > EOS) {
		PthreadMutexLock(&Stream->mutex);
		file = GetFile(id);
		PthreadMutexLock(&file->mutex);
		if (GET_FLAG(file->flags, FI_DCACHE_CLOSE)) {
			if (close(file->dcache) == -1) {
				WarnSyscallError(HERE,
				    "close", "");
			}
			CLEAR_FLAG(file->flags, FI_DCACHE);
			NumOpenFiles--;

		} else if (GET_FLAG(file->flags, FI_DCACHE)) {
			SendErrorResponse(file);
		}

		id = file->next;
		PthreadMutexUnlock(&Stream->mutex);
		PthreadMutexUnlock(&file->mutex);
	}

	if (GET_FLAG(Stream->flags, SR_UNAVAIL)) {
		rejectRequest(ENODEV, B_TRUE);
	} else {
		rejectRequest(0, B_TRUE);
	}
}

/*
 * If number of stream errors exceeded,
 * cancel current file and reject rest
 * of stages in this stream.
 */
static boolean_t
ifMaxStreamErrors(
	FileInfo_t *file)
{
	boolean_t rval;

	rval = B_FALSE;

	if (NumOpenFiles >= MAX_COPY_STREAM_ERRORS) {
		SendCustMsg(HERE, 19017, Stream->vsn);
		if (GET_FLAG(file->flags, FI_DCACHE)) {
			file->error = ECANCELED;
			SET_FLAG(file->flags, FI_NO_RETRY);
			CLEAR_FLAG(file->flags, FI_DCACHE);
			ErrorFile(file);
			Trace(TR_MISC, "Canceled(max stream error) "
			    "inode: %d.%d", file->id.ino, file->id.gen);
		}
		rval = B_TRUE;
	}
	return (rval);
}

/*
 * Fatal error in copy of archive file.
 * Reject copy request.
 */
static void
rejectRequest(
	int error,
	boolean_t clean)
{
	PthreadMutexLock(&Stream->mutex);

	Stream->pid = 0;

	if (error == 0) {
		if (GET_FLAG(Stream->flags, SR_DCACHE_CLOSE)) {
			SET_FLAG(Stream->flags, SR_CLEAR);
		} else {
			Stream->flags = 0;
			if (Stream->first == EOS) {
				/*
				 * Set active and done flags on stream
				 * for delete.
				 */
				Stream->last = EOS;
				SET_FLAG(Stream->flags, SR_ACTIVE);
				SET_FLAG(Stream->flags, SR_DONE);
			} else {
				Stream->priority = SP_start;
			}
		}
	} else {
		SET_FLAG(Stream->flags, SR_ERROR);
		Stream->error = error;
	}

	PthreadMutexUnlock(&Stream->mutex);

	if (clean) {
		/*
		 * Unmap pages of memory.  Stream's memory
		 * mapped file is removed in parent.
		 */
		UnMapFile(Stream, sizeof (StreamInfo_t));
		Stream = NULL;

		/* Remove copy request. */
		RemoveMapFile(copyRequestPath, Request,
		    sizeof (CopyRequestInfo_t));
		Request = NULL;

		Instance->ci_first = Instance->ci_last = NULL;
	}
}

/*
 * Remove file with dcache opened from stream.
 * Caller must have the stream lock held.
 */
static void
removeDcachedFile(
	StreamInfo_t *stream,
	int error)
{
	int id;
	FileInfo_t *file, *prev = NULL;

	id = stream->first;
	while (id > EOS) {
		boolean_t removed = B_FALSE;

		file = GetFile(id);
		PthreadMutexLock(&file->mutex);

		if (prev != NULL) {
			PthreadMutexLock(&prev->mutex);
		}
		if (GET_FLAG(file->flags, FI_DCACHE)) {
			/*
			 * Remove file from stream.
			 * If multivolume and not section 0, set ECOMM to not
			 * retry from another copy.
			 */
			if (file->ar[file->copy].ext_ord != 0 ||
			    file->se_ord != 0) {
				file->error = ECOMM;
			} else {
				file->error = error;
			}
			SendErrorResponse(file);

			if (prev != NULL) {
				prev->next = file->next;
			} else {
				stream->first = file->next;
			}
			stream->count--;
			if (stream->first == EOS) {
				stream->last = EOS;
			} else if (file->sort == stream->last) {
				ASSERT(prev != NULL);
				stream->last = prev->sort;
			}
			removed = B_TRUE;
		}
		if (prev != NULL) {
			PthreadMutexUnlock(&prev->mutex);
		}
		PthreadMutexUnlock(&file->mutex);
		id = file->next;
		if (removed) {
			SetStageDone(file);
		} else {
			prev = file;
		}
	}
}


/*
 * Initialize stage io thread control structure.  Structure is used
 * to control data movement between io threads.
 */
static void
initIoThread()
{
	size_t size;

	size = sizeof (IoThreadInfo_t);
	SamMalloc(IoThread, size);
	memset(IoThread, 0, size);

	ThreadStateInit(&IoThread->io_readReady);
	ThreadStateInit(&IoThread->io_readDone);

	ThreadStateInit(&IoThread->io_moveReady);
	ThreadStateInit(&IoThread->io_moveDone);

	if (is_disk(Instance->ci_media)) {
		if (is_stk5800(Instance->ci_media)) {
			IoThread->io_flags |= IO_stk5800;
		} else {
			IoThread->io_flags |= IO_disk;
		}
	} else if (Instance->ci_flags & CI_samremote) {
		IoThread->io_flags |= IO_samremote;
	}
}

/*
 * Set stage file data in the i/o thread.
 */
static void
setIoThread(
	FileInfo_t *file)
{
	IoThread->io_file = file;
	IoThread->io_offset = 0;

	/* Set staged drive number. */
	IoThread->io_drive = GetDriveNumber();
}
