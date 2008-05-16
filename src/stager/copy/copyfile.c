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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.103 $"

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
#include "aml/stager.h"
#include "aml/id_to_path.h"
#include "aml/sam_rft.h"

/* Local headers. */
#include "stager_config.h"
#include "stager_lib.h"
#include "stage_reqs.h"
#include "rmedia.h"
#include "stream.h"
#include "copy_defs.h"
#include "error_retry.h"
#include "filesys.h"
#include "copy.h"
#include "log.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Public data. */
extern CopyInstance_t *Context;
extern int NumOpenFiles;

/*
 * Control information for process.
 */
CopyControl_t *Control = NULL;

/*
 * Active request for process.
 */
CopyRequest_t *Request = NULL;

/*
 * Active stream for process.
 */
StreamInfo_t *Stream = NULL;

/* Private data. */

/*
 * Temporary space for building path name.
 */
static upath_t fullpath;

/*
 */
static char *copyreq_filename;
/*
 * Staged drive number.
 */
static equ_t driveNumber;

/* Private functions. */
static void initControl(int rmfn, media_t media);
static void setMau(offset_t block_size);
static void checkBuffers(char *new_vsn);
static void allocBuffers();
static int advanceInBuffer();
static int advanceOutBuffer();
static void resetOutBuffer();
static void insertBuffer(int idx, int position);
static void setDirtyBuffer(int idx);
static void setErrorBuffer(int idx, int error);
static int isPositionInBuffer(int pos, boolean_t seq);

static int openDiskCache(FileInfo_t *file);
static int writeDiskCache(int fd);
static int verifyTarHeader(char *buffer, char *name,
	u_longlong_t len, boolean_t verifylen);

static void *fileReader(void *arg);
static void rejectRequest(int error, boolean_t clean);
static void removeDcachedFile(StreamInfo_t *stream, int error);
static void setFileError(FileInfo_t *file, int fd, offset_t write_off,
	int error);
static void setVerified(FileInfo_t *file, int copy);

/*
 * Enter loop to copy all files in stream.  This function waits for
 * and services copy request from the scheduler.
 */
void
CopyFiles(void)
{
	pthread_t reader_thread_id;
	int dcache;
	int status;
	FileInfo_t *file;
	int error;
	struct timespec timeout;
	boolean_t reject;

	ASSERT(Context != NULL);

	(void) sprintf(fullpath, "%s/%s/rm%d/%s", SAM_VARIABLE_PATH,
	    STAGER_DIRNAME, Context->rmfn, COPY_FILE_FILENAME);
	SamStrdup(copyreq_filename, fullpath);

	initControl(Context->rmfn, Context->media);

	status = pthread_create(&reader_thread_id, NULL, fileReader,
	    (void *)Control);
	if (status != 0) {
		LibFatal(pthread_create, "fileReader");
	}

	for (;;) {

		timeout.tv_sec = time(NULL) + COPYPROC_TIMEOUT_SECS;
		timeout.tv_nsec = 0;

		status = pthread_mutex_lock(&Context->request_mutex);
		ASSERT(status == 0);

		Context->idle = B_TRUE;

		/*
		 * Wait on timout value or until signaled there is
		 * work to do.
		 */
		while (Context->first == NULL && !IS_SHUTDOWN(Context)) {
			if (getppid() == 1) {
				Trace(TR_ERR, "Detected stager daemon exit");
				SET_FLAG(Context->flags, CI_shutdown);
				break;
			}
			status = pthread_cond_timedwait(&Context->request,
			    &Context->request_mutex, &timeout);

			if (status == ETIMEDOUT) {
				/*
				 * Wait timed out.  If no open disk cache files
				 * set shutdown flag so we exit loop.  If open
				 * files, reset timeout value and wait for work.
				 */
				if (NumOpenFiles <= 0 &&
				    Context->first == NULL) {
					SET_FLAG(Context->flags, CI_shutdown);
					/*
					 * Set context to busy while this
					 * process is exiting and the stager
					 * daemon detects that it exited.
					 */
					Context->idle = 0;
				} else {
					timeout.tv_sec = time(NULL) +
					    COPYPROC_TIMEOUT_SECS;
					timeout.tv_nsec = 0;
				}
			}
		}

		/*
		 * If shutdown requested exit loop.
		 */
		if (IS_SHUTDOWN(Context)) {
			Trace(TR_MISC, "Shutdown context: 0x%x first: 0x%x",
			    (int)Context, (int)Context->first);
			status = pthread_mutex_unlock(&Context->request_mutex);
			break;
		}

		/*
		 *  Received copy request from scheduler.  Initialize
		 *  ourself and then tell scheduler we got it.
		 */
		Request = (CopyRequest_t *)MapInFile(copyreq_filename,
		    O_RDWR, NULL);
		if (Request == NULL) {
			Trace(TR_ERR, "Cannot map in file %s",
			    copyreq_filename);
			(void) pthread_mutex_unlock(&Context->request_mutex);
			if (errno == EMFILE) {
				/*
				 * Open max reached. This should not happen.
				 * Stream will be requeued by parent.
				 */
				(void) pthread_mutex_unlock(
				    &Context->request_mutex);
				LibFatal(MapInFile, "EMFILE");
			}
			(void) pthread_mutex_unlock(&Context->request_mutex);
			continue;
		}

		Context->first = Request->next;
		if (Context->first == NULL)
			Context->last = NULL;

		Trace(TR_MISC, "Got request: '%s.%d' 0x%x",
		    Request->vsn, Request->seqnum, (int)Request);

		(void) sprintf(fullpath, "%s/%s.%d", SharedInfo->streamsDir,
		    Request->vsn, Request->seqnum);
		Stream = (StreamInfo_t *)MapInFile(fullpath, O_RDWR, NULL);
		if (Stream == NULL) {
			Trace(TR_ERR, "Cannot map in file %s", fullpath);
			if (errno == EMFILE) {
				/*
				 * Open max reached. This should not happen.
				 * Stream will be requeued by parent.
				 */
				(void) pthread_mutex_unlock(
				    &Context->request_mutex);
				LibFatal(MapInFile, "EMFILE");
			}
			status = pthread_mutex_unlock(&Context->request_mutex);
			continue;
		}

		Request->got_it_flag = B_TRUE;
		Context->vsn_lib = Stream->vi.lib;
		Context->idle = B_FALSE;

		status = pthread_cond_signal(&Request->got_it);
		ASSERT(status == 0);

		status = pthread_mutex_unlock(&Context->request_mutex);

		/*
		 * If media is unavailable and there are open file descriptors
		 * in the stream send an error response here.
		 * If stream has only pending open file descriptors close file
		 * descriptors.
		 */
		if (GET_FLAG(Stream->flags, SR_UNAVAIL) ||
		    GET_FLAG(Stream->flags, SR_DCACHE_CLOSE)) {
			int id;

			id = Stream->first;
			while (id > EOS) {
				THREAD_LOCK(Stream);
				file = GetFile(id);
				THREAD_LOCK(file);
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
				THREAD_UNLOCK(Stream);
				THREAD_UNLOCK(file);
			}

			if (GET_FLAG(Stream->flags, SR_UNAVAIL)) {
				if (IS_VSN_BADMEDIA(
				    ((VsnInfo_t *)&Stream->vi))) {
					rejectRequest(ENOSPC, B_TRUE);
				} else {
					rejectRequest(ENODEV, B_TRUE);
				}
			} else {
				rejectRequest(0, B_TRUE);
			}
			continue;
		}

		InitStage(Stream->vsn);

		/*
		 * Set loading flag for this stream.
		 */
		THREAD_LOCK(Stream);
		SET_FLAG(Stream->flags, SR_LOADING);
		THREAD_UNLOCK(Stream);

		error = LoadVolume();

		/*
		 * Reject if mount/open failed,
		 */
		if (error) {
			THREAD_LOCK(Stream);
			removeDcachedFile(Stream, error);
			if (error == ENODEV) {
				Stream->context = 0;
				THREAD_UNLOCK(Stream);
				rejectRequest(0, B_TRUE);
				SET_FLAG(Context->flags, CI_shutdown);
			} else {
				THREAD_UNLOCK(Stream);
				SendCustMsg(HERE, 19017, Stream->vsn);
				rejectRequest(error, B_TRUE);
			}
			EndStage();
			continue;
		}

		checkBuffers(Stream->vsn);

		/*
		 * VSN load has completed.
		 */
		THREAD_LOCK(Stream);
		CLEAR_FLAG(Stream->flags, SR_LOADING);
		reject = B_FALSE;
		Context->seqnum = Stream->seqnum;

		/*
		 * Copy all files in stage stream request.  The files have
		 * been ordered to eliminate backward media positioning.
		 */
		while (Stream->first >= 0 &&
		    (GET_FLAG(Stream->flags, SR_ERROR) == 0) &&
		    (GET_FLAG(Stream->flags, SR_CLEAR) == 0) &&
		    (GET_FLAG(Context->flags, CI_failover) == 0) &&
		    (reject == B_FALSE)) {

			/*
			 * Stop staging if parent died.
			 */
			if (getppid() == 1) {
				Trace(TR_ERR, "Detected stager daemon exit");

				Stream->first = -1;
				SET_FLAG(Context->flags, CI_shutdown);
				break;
			}

			file = GetFile(Stream->first);
			file->read = 0;

			THREAD_LOCK(file);
			THREAD_UNLOCK(Stream);

			SET_FLAG(file->flags, FI_ACTIVE);

			THREAD_UNLOCK(file);

			/*
			 * Set file in control structure for Reader thread.
			 */
			Control->file = file;
			file->eq = driveNumber = GetDriveNumber();
			LogIt(LOG_STAGE_START, file);

			/*
			 * Last request was canceled.
			 */
			if (GET_FLAG(Control->flags, CC_cancel)) {
				InvalidateBuffers();
				CLEAR_FLAG(Control->flags, CC_cancel);
			}

			/*
			 * Next archive file.
			 */
			if ((error = NextArchiveFile()) == 0) {
				/*
				 * Prepare filesystem to receive staged file.
				 */
				dcache = openDiskCache(file);
			} else {
				Trace(TR_ERR, "Unable to open disk archive "
				    "copy: %d inode: %d.%d errno: %d",
				    file->copy + 1, file->id.ino, file->id.gen,
				    errno);
				dcache = -1;
				file->error = errno;
				SendErrorResponse(file);
			}

			if (dcache >= 0 && error == 0) {
				/*
				 * Signal reader thread that next file in stream
				 * is ready to be staged.
				 */
				ThreadStatePost(&Control->ready);

				error = writeDiskCache(dcache);
				if (error) {
					SendErrorResponse(file);

					/*
					 * If number of copy errors exceeded,
					 * cancel current file and reject rest
					 * of stages in this stream.
					 */
					if (NumOpenFiles >=
					    MAX_COPY_STREAM_ERRORS) {
						SendCustMsg(HERE, 19017,
						    Stream->vsn);
						if (GET_FLAG(file->flags,
						    FI_DCACHE)) {
							file->error = ECANCELED;
							SET_FLAG(file->flags,
							    FI_NO_RETRY);
							CLEAR_FLAG(file->flags,
							    FI_DCACHE);
							ErrorFile(file);
							Trace(TR_MISC,
							    "Canceled(max "
							    "stream error) "
							    "inode: %d.%d",
							    file->id.ino,
							    file->id.gen);
						}
						reject = B_TRUE;
					}
				}

				ThreadStateWait(&Control->done);

			} else if (error && dcache >= 0) {
				/*
				 * Setup for error handling.
				 */
				setFileError(file, dcache, 0, EIO);
				SendErrorResponse(file);
			}

			EndArchiveFile();

			/*
			 * Remove file from stream before
			 * marking it as done.
			 */
			THREAD_LOCK(Stream);
			Stream->first = file->next;

			/*
			 * Device not available.
			 */
			if (file->error == ENODEV) {
				CLEAR_FLAG(file->flags, FI_DCACHE);
				Trace(TR_ERR, "No device available");

				Stream->first = -1;
				SET_FLAG(Context->flags, CI_shutdown);
			}

			/*
			 * Mark file staging as done.
			 */
			SetStageDone(file);
			Stream->count--;

			if (Stream->first == EOS) {
				Stream->last = EOS;
			}

		}
		if (reject) {
			if (Stream->first > EOS) {
				removeDcachedFile(Stream, ENODEV);
			}
			THREAD_UNLOCK(Stream);
			rejectRequest(ENODEV, B_FALSE);
			THREAD_LOCK(Stream);
		}

		/*
		 * Remove request, no one is waiting on it.
		 */
		RemoveMapFile(copyreq_filename, Request,
		    sizeof (CopyRequest_t));
		Request = NULL;

		/*
		 * Ready to unload.  Mark stream as done.
		 */
		SET_FLAG(Stream->flags, SR_DONE);
		THREAD_UNLOCK(Stream);

		UnloadVolume();

		/*
		 * Unmap pages of memory.  Stream's memory
		 * mapped file is removed in parent.
		 */
		UnMapFile(Stream, sizeof (StreamInfo_t));
		Stream = NULL;

		EndStage();
	}
}

/*
 * Position media.
 */
int
SetPosition(
	int from_pos,
	int to_pos)
{
	FileInfo_t *file;
	int cancel;
	int position = from_pos;

	Trace(TR_DEBUG, "Set position from %d to %d", from_pos, to_pos);

	cancel = GET_FLAG(Control->flags, CC_cancel);
	if (from_pos != to_pos && cancel == 0) {
		/*
		 * Positioning flag used only in status displays.
		 */
		file = Control->file;
		SET_FLAG(file->flags, FI_POSITIONING);

		position = SeekVolume(to_pos);

		CLEAR_FLAG(file->flags, FI_POSITIONING);
	}
	return (position);
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
	reqid_t id;
	FileInfo_t *file, *prev = NULL;

	id = stream->first;
	while (id > EOS) {
		boolean_t removed = B_FALSE;

		file = GetFile(id);
		THREAD_LOCK(file);

		if (prev != NULL) {
			THREAD_LOCK(prev);
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
			THREAD_UNLOCK(prev);
		}
		THREAD_UNLOCK(file);
		id = file->next;
		if (removed) {
			SetStageDone(file);
		} else {
			prev = file;
		}
	}
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
	THREAD_LOCK(Stream);

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

	THREAD_UNLOCK(Stream);

	if (clean) {
		/*
		 * Unmap pages of memory.  Stream's memory
		 * mapped file is removed in parent.
		 */
		UnMapFile(Stream, sizeof (StreamInfo_t));
		Stream = NULL;

		/*
		 * Remove copy request.
		 */
		RemoveMapFile(copyreq_filename, Request,
		    sizeof (CopyRequest_t));
		Request = NULL;

		Context->first = Context->last = NULL;
	}
}

/*
 * The file reader thread is responsible for reading data from the
 * removable media file.
 */
static void *
fileReader(
	/* LINTED argument unused in function */
	void *arg)
{
	int count;
	int numbytes_read;
	int in;
	longlong_t data_to_read;
	int first_pass;
	int cancel;
	int error;
	int position_in_buffer;
	int media_position;	/* current position on physical media */
	int file_position;	/* current position on file being staged */
	ulong_t file_offset;	/* current offset on file being staged */
	longlong_t mau_offset;
	int mau_in_buffer;
	FileInfo_t *file;
	int copy;
	int notarhdr;
	int retry;		/* retry indicator */

	for (;;) {

		/*
		 * Wait for next file to stage.
		 */
		ThreadStateWait(&Control->ready);

		file = Control->file;
		copy = file->copy;
		SetErrno = 0;

		/*
		 * If this is a multivolume stage request the stage length
		 * may be greater than data available on this volume.  Set
		 * data to read based on section length.
		 */
		if (file->len > file->ar[copy].section.length) {
			data_to_read = file->ar[copy].section.length;
		} else {
			data_to_read = (longlong_t)file->len;
		}

		Trace(TR_MISC,
		    "Copy file inode: %d.%d\n\tpos: %llx.%llx len: %lld",
		    file->id.ino, file->id.gen,
		    file->ar[copy].section.position,
		    file->ar[copy].section.offset,
		    data_to_read);

		ASSERT(data_to_read >= 0);

		/*
		 * If zero length file there is no data to read.
		 * Go back to while loop and wait for next file
		 * to stage.
		 */
		if (data_to_read == 0) {
			ThreadStatePost(&Control->done);
			continue;
		}

		/*
		 * Maintain local copy of media's current physical position
		 * and current file position.  These values will not match
		 * if read data is already buffered.  Initially, the current
		 * position is following the mount.  Subsequent positions are
		 * maintained by this thread.
		 */
		media_position = (int)Control->currentPos;

		notarhdr = GET_FLAG(file->flags, FI_NO_TARHDR);

		retry = GET_FLAG(file->flags, FI_RETRY) &&
		    (file->write_off > 0);

		if (retry) {
			file->offset += file->write_off;
			/*
			 * If multivolume set data to read based on
			 * section length.
			 */
			if (file->len > file->ar[copy].section.length) {
				data_to_read = file->len - file->write_off;
				if (data_to_read >
				    file->ar[copy].section.length) {
					data_to_read =
					    file->ar[copy].section.length;
				}
			} else {
				data_to_read = data_to_read - file->write_off;
			}
			notarhdr = 1;
		}

		/*
		 * Calculate new offset and position based on media
		 * allocation unit size (mau).  A media allocation units is
		 * the smallest addressable part on media, ie. block on tape
		 * sector on optical.
		 */

		/*
		 * Calculate mau offset, number of ma units to start of file's
		 * data. Value section.offset is offset in TAR_RECORDSIZE units
		 * to file's data.
		 */
		mau_offset = file->ar[copy].section.offset;
		if (notarhdr == 0) {		/* if tar header to validate */
			mau_offset = mau_offset - 1; /* adjust for tar header */
		}
		mau_offset = (mau_offset * TAR_RECORDSIZE) + file->offset;
		mau_offset = mau_offset / Control->mau;
		Trace(TR_DEBUG, "Mau offset %lld", mau_offset);

		/*
		 * Calculate file position, starting file position in ma units
		 * based on copy's file position and offset.
		 */
		if (Stream->diskarch) {
			file_position = mau_offset;
		} else {
			file_position = (int)file->ar[copy].section.position +
			    mau_offset;
		}
		Trace(TR_DEBUG, "File position %d", file_position);

		/*
		 * Calculate file offset, byte offset to start of file's
		 * data in the file's mau.
		 */
		file_offset =
		    ((file->ar[copy].section.offset * TAR_RECORDSIZE) +
		    file->offset) - (mau_offset * Control->mau);
		Trace(TR_DEBUG, "File offset %d", (int)file_offset);

		mau_in_buffer = Control->mauInBuffer;

		/*
		 * Check if file position already exists in buffer.
		 * If not, position media.
		 */
		in = isPositionInBuffer(file_position, FALSE);

		if (in >= 0) {
			int num_mau;
			/*
			 * Reset file offset and out buffer based on position
			 * already available in buffer.
			 */
			Trace(TR_DEBUG, "Position in buffer: %d %d",
			    in, Control->buffers[in].position);

			position_in_buffer = TRUE;
			numbytes_read = Control->bufSize;

			num_mau = file_position - Control->buffers[in].position;
			if (num_mau > 0) {
				file_offset = file_offset +
				    (num_mau * Control->mau);
				Trace(TR_DEBUG, "File offset %d",
				    (int)file_offset);
				mau_in_buffer = mau_in_buffer - num_mau;
			}
			resetOutBuffer();

		} else {
			/*
			 * Data not available in existing buffer.
			 * Position media.
			 */
			position_in_buffer = FALSE;
			media_position =
			    SetPosition(media_position, file_position);
		}

		first_pass = TRUE;
		cancel = GET_FLAG(Control->flags, CC_cancel);
		error = 0;
		(void) time(&Control->start_time);

		while (data_to_read > 0 && cancel == 0 && error == 0) {

			count = Control->bufSize;

			if (position_in_buffer == FALSE) {
				/*
				 * Get next available buffer for reading a
				 * block of data.
				 */
				in = advanceInBuffer();

				/*
				 * Exit loop if the write thread has requested
				 * a cancel while waiting for a buffer.
				 */
				if (GET_FLAG(Control->flags, CC_cancel)) {
					cancel = 1;
					break;
				}

read_error_retry:
				/*
				 * If positioning request failed.
				 */
				if (media_position < 0) {
					Trace(TR_ERR, "Positioning error "
					    "inode: %d errno: %d",
					    file->id.ino, errno);
					SendCustMsg(HERE, 19030,
					    media_position, driveNumber, errno);
					error = 1;
					setErrorBuffer(in, errno);
					media_position = 0;

					/*
					 * Tell writer thread that a buffer
					 * is available.
					 */
					ThreadSemaPost(&Control->fullBuffers);
					continue;
				}

				Trace(TR_DEBUG, "Start read in: %d count: %d",
				    in, count);

				if (IS_REMOTE_STAGE(Context) ||
				    Control->flags & CC_disk) {
					numbytes_read = SamrftRead(Control->rft,
					    Control->buffers[in].data, count);

				} else if (Control->flags & CC_honeycomb) {
					numbytes_read = HcStageReadArchiveFile(
					    Control->rft,
					    Control->buffers[in].data, count);

				} else {
					numbytes_read = read(Control->rmfd,
					    Control->buffers[in].data, count);
				}

				Trace(TR_DEBUG, "End read in: %d %d",
				    in, numbytes_read);

				Control->buffers[in].count = numbytes_read;
				insertBuffer(in, media_position);

				if (numbytes_read == count ||
				    (numbytes_read > 0 &&
				    numbytes_read >= data_to_read &&
				    errno == 0)) {
					/*
					 * Read okay.
					 */
					media_position = media_position +
					    mau_in_buffer;
					file->read += numbytes_read;

				} else {
					longlong_t total_read;

					/*
					 * Read error.  Check for empty sector
					 * condition on optical.
					 */
					Trace(TR_ERR,
					    "Read failed: request: %d read: %d "
					    "left: %lld errno: %d",
					    count, numbytes_read, data_to_read,
					    errno);

					error = 1;
					/*
					 * Read request not satisfied but errno
					 * not set.
					 */
					if (errno == 0) {
						SetErrno = EIO;
					}

					/*
					 * If first pass, total read size
					 * includes file offset, byte offset
					 * to start of file's data in file's
					 * mau.  If not first pass, file offset
					 * is zero.
					 */
					total_read = data_to_read + file_offset;
					numbytes_read = RetryRead(in,
					    total_read, file_position);

					if (numbytes_read > 0) {
						/*
						 * If retry worked must be at
						 * end of archive set on
						 * optical media.  However the
						 * buffer is not full.  Mark
						 * this buffer so it won't be
						 * used in search if next block
						 * of data is available in
						 * buffer.
						 */
						setDirtyBuffer(in);
						Control->buffers[in].count =
						    numbytes_read;

						error = 0;
						mau_in_buffer = numbytes_read /
						    Control->mau;
						media_position =
						    media_position +
						    mau_in_buffer;
					}

					/*
					 * If read error, check if read should
					 * be retried. If not, set error in
					 * control structure for write thread
					 * to see.  Fall thru to bottom of loop
					 * since still need to tell write thread
					 * that a buffer is available.
					 */
					if (error) {
						/*
						 * Check if error should be
						 * retried.  If yes,
						 * position media and read
						 * same block again.
						 */
						if (IsRetryError(file)) {
							sleep(4);
							error = 0;
							media_position =
							    SetPosition(-1,
							    file_position);
							goto read_error_retry;
						}

						Trace(TR_ERR, "Read error "
						    "inode: %d errno: %d",
						    file->id.ino, errno);
						SendCustMsg(HERE, 19031,
						    driveNumber, errno);

						setErrorBuffer(in, errno);
						/*
						 * Lost position on media.
						 * Reset so next request
						 * will position.
						 */
						media_position = 0;
					}
				}
			}

			/*
			 * If first pass, save file offset for write thread.
			 * Adjust count to ignore tar header size.
			 */
			if (first_pass) {
				Control->offset = file_offset;
				numbytes_read  = numbytes_read - file_offset;
				first_pass = FALSE;
				file_offset = 0;
			}

			/*
			 * Tell writer thread that a buffer is available.
			 */
			ThreadSemaPost(&Control->fullBuffers);

			data_to_read = data_to_read - numbytes_read;
			file_position = file_position + mau_in_buffer;

			/*
			 * If previous block was already in buffer, check
			 * if next block is also available.  If not, position
			 *  media.
			 */
			if (position_in_buffer && data_to_read > 0) {

				mau_in_buffer = Control->mauInBuffer;
				numbytes_read = Control->bufSize;
				in = isPositionInBuffer(file_position, TRUE);
				if (in == -1) {
					/*
					 * Data not available, position media.
					 */
					position_in_buffer = FALSE;
					media_position =
					    SetPosition(media_position,
					    file_position);
					numbytes_read = 0;
				}
			}

			/*
			 * If stage request is canceled, the swrite sycall
			 * in the write thread will fail and cancel flag
			 *  will be set in control structure.
			 */
			cancel = GET_FLAG(Control->flags, CC_cancel);
		}

		/*
		 * Update control's current position.
		 */
		Control->currentPos = media_position;

		ThreadStatePost(&Control->done);
	}

#if defined(lint)
	/* LINTED statement not reached */
	return (NULL);
#endif
}

/*
 * Initialize structure used to control data movement between
 * read and write threads.
 */
static void
initControl(
	int rmfn,
	media_t media)
{
	size_t size;

	/*
	 * Initialize control structure for reader/writer threads.
	 */
	size = sizeof (CopyControl_t);
	SamMalloc(Control, size);
	memset(Control, 0, size);

	ThreadStateInit(&Control->ready);
	ThreadStateInit(&Control->done);
	Control->rmfn = rmfn;
	if (is_disk(media)) {
		if (is_stk5800(media)) {
			Control->flags |= CC_honeycomb;
		} else {
			Control->flags |= CC_disk;
		}
	}

/* return control; */
}

/*
 * Allocate i/o buffers for copy.  Size of each buffers is determined
 * from type of removable media file's block size.
 */
static void
allocBuffers(void)
{
	int block_size;
	offset_t buf_size;
	offset_t alloc_size;
	int i;
	boolean_t lockbuf;
	char *buf_first = NULL;

	block_size = GetBlockSize();
	if (block_size == 0) {
		Trace(TR_ERR, "Block size is zero");
		block_size = 16 *1024;
	}

	buf_size = GetBufferSize(block_size);
	ASSERT(buf_size > 0);

	Control->bufSize = buf_size;

	if (Control->numBuffers == 0) {
		Control->numBuffers = Context->num_buffers;
		SamMalloc(Control->buffers,
		    Control->numBuffers * sizeof (CopyBuffer_t));
		memset(Control->buffers, 0,
		    Control->numBuffers * sizeof (CopyBuffer_t));

	} else {
		/*
		 * Buffers previously allocated.  The media's block size
		 * has changed.  Free existing buffers and allocate new ones
		 * based on the new block size.
		 */
		SamFree(Control->buffers[0].data);
	}

	lockbuf = Context->lockbuf;
	alloc_size = Control->numBuffers * buf_size;

	if (lockbuf == B_TRUE) {
		SamValloc(buf_first, alloc_size);
		if (mlock(buf_first, alloc_size) < 0) {
			WarnSyscallError(HERE, "mlock", "");
			lockbuf = B_FALSE;
		}
	} else {
		SamMalloc(buf_first, alloc_size);
	}
	Control->buffers[0].data = buf_first;

	Trace(TR_MISC, "Block size: %lld buf size: %d %s",
	    buf_size, Control->numBuffers, lockbuf ? "(lock)" : "");

	Control->buffers[0].valid = 0;

	for (i = 1; i < Control->numBuffers; i++) {
		Control->buffers[i].data = Control->buffers[i-1].data +
		    buf_size;
		Control->buffers[i].valid = 0;
	}

	/*
	 * Initialize so advance functions allocate from top.
	 */
	Control->inBuf = Control->outBuf = Control->numBuffers - 1;

	ThreadSemaInit(&Control->fullBuffers, 0);
	ThreadSemaInit(&Control->emptyBuffers, Control->numBuffers);

	/*
	 * Set media allocation unit information.
	 */
	setMau((offset_t)block_size);
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

	/*
	 * If first request for this thread, allocate buffers
	 * in control structure.
	 */
	if (Control->numBuffers == 0) {
		allocBuffers();
		ASSERT(Control->numBuffers > 0);
	}

	/*
	 * If disk archiving, invalidate buffers.
	 */
	if (Control->flags & CC_diskArchiving) {
		InvalidateBuffers();
		Control->currentPos = 0;
		strcpy(Context->vsn, new_vsn);
		return;
	}

	/*
	 * Get current position of removable media file.
	 */
	currentPos = GetPosition();

	Trace(TR_MISC, "Get position %lld block size %d",
	    currentPos, GetBlockSize());

	/*
	 * If we loaded the same VSN as last time this proc was
	 * active, the buffers may be valid and thus data in the buffers
	 * can be reused.  If this is not the same VSN, invalidate
	 * the buffers and save VSN label in the thread's context.
	 */
	if (strcmp(Context->vsn, new_vsn) != 0) {
		offset_t block_size;

		block_size = (offset_t)GetBlockSize();

		/*
		 * New media.  If block size has changed set new mau
		 * information.  If necessary, reallocate buffers based
		 * on the new block size.
		 */
		if (Control->mau != block_size) {

			if (Control->bufSize != GetBufferSize(block_size)) {
				allocBuffers();
			} else {
				setMau((offset_t)block_size);
			}
		} else {
			InvalidateBuffers();
		}
		strcpy(Context->vsn, new_vsn);

	} else if (Control->currentPos != currentPos) {

		/*
		 * The last known media position does not match the
		 * removable media file's position.  Since the position has
		 * changed, buffers must be invalidated.
		 */
		InvalidateBuffers();

	} else {
		/*
		 * Data in buffer is valid and can be reused.
		 */

		Control->fullBuffers.count  = 0;
		Control->emptyBuffers.count = Control->numBuffers;
	}

	/*
	 * Reader thread needs mount position to maintain
	 * position on the media.
	 */
	Control->currentPos = currentPos;
}

/*
 * Invalidate data in i/o buffers.
 */
void
InvalidateBuffers(void)
{
	int i;
	Trace(TR_DEBUG, "Invalidate buffers");
	for (i = 0; i < Control->numBuffers; i++) {
		Control->buffers[i].valid = 0;
		Control->buffers[i].error = 0;
	}
	Control->inBuf = Control->outBuf = Control->numBuffers - 1;
	Control->fullBuffers.count = 0;
	Control->emptyBuffers.count = Control->numBuffers;
}

/*
 * Check if data block is already cached in i/o buffers.
 */
static int
isPositionInBuffer(
	int position,
	boolean_t sequential)
{
	int in = -1;
	int i;
	int fwa;
	int lwa;

	/*
	 *	Loop over buffers and to check if position already in buffer.
	 */
	for (i = 0; i < Control->numBuffers; i++) {
		if (Control->buffers[i].valid == 0) continue;

		fwa = Control->buffers[i].position;
		lwa = (fwa + Control->mauInBuffer) - 1;

		if (position >= fwa && position <= lwa) {
			int nextBuf;

			/*
			 * Position is within buffer.  If this is the
			 * read request (sequential = FALSE), its a winner and
			 * we can reuse the data.  If this is not the first
			 * read for this file, the requested position must be
			 * the first mau in the buffer (should only happen with
			 *  optical).
			 */

			nextBuf = Control->inBuf + 1;
			if (nextBuf == Control->numBuffers) {
				nextBuf = 0;
			}

			if ((sequential == FALSE) ||
			    (sequential == TRUE && i == nextBuf &&
			    position == fwa)) {

				Control->inBuf = i;
				Control->emptyBuffers.count--;
				in = i;

				break;
			}
		}
	}
	return (in);
}

/*
 * Advance in pointer for i/o buffer.  Must wait for
 * space if no empty buffer.
 */
static int
advanceInBuffer(void)
{
	/*
	 * Wait if no empty buffers.
	 */
	ThreadSemaWait(&Control->emptyBuffers);

	if (Control->inBuf == Control->numBuffers - 1)
		Control->inBuf = 0;
	else
		Control->inBuf++;

	return (Control->inBuf);
}

/*
 * Advance out pointer for i/o buffer.  Must wait
 * if no full buffer.
 */
static int
advanceOutBuffer(void)
{
	/*
	 * Wait if no full buffers.
	 */
	ThreadSemaWait(&Control->fullBuffers);

	if (Control->outBuf == Control->numBuffers - 1) {
		Control->outBuf = 0;
	} else {
		Control->outBuf++;
	}

	return (Control->outBuf);
}

/*
 * Reset out pointer for i/o buffer.
 */
static void
resetOutBuffer(void)
{
	if (Control->inBuf == 0) {
		Control->outBuf = Control->numBuffers - 1;
	} else {
		Control->outBuf = Control->inBuf - 1;
	}
}

/*
 * Insert data into buffer.
 */
static void
insertBuffer(
	int idx,
	int position)
{
	Control->buffers[idx].position = position;
	Control->buffers[idx].valid = 1;
	Control->buffers[idx].error = 0;
}

/*
 * Set buffer invalid.
 */
static void
setDirtyBuffer(
	int idx)
{
	Control->buffers[idx].valid = 0;
}

/*
 * Set error in buffer.
 */
static void
setErrorBuffer(
	int idx,
	int error)
{
	Control->buffers[idx].error = error;
	Control->buffers[idx].valid = 0;
}


/*
 * Open disk cache file.
 */
static int
openDiskCache(
	FileInfo_t *file)
{
	sam_fsstage_arg_t arg;

	int fd = -1;

	/*
	 * If saved, restore open disk cache file descriptor.
	 * This occurs when processing multivolume.
	 */
	if (GET_FLAG(file->flags, FI_DCACHE)) {
		ASSERT(file->dcache > 0);
		fd = file->dcache;

		Trace(TR_DEBUG, "Restore disk cache fd: %d", fd);

	} else if (GET_FLAG(file->ar[file->copy].flags, STAGE_COPY_VERIFY)) {
		int retry = 5;
		int err = 0;
		int mpfd;
		char *mount_name;

		mount_name = GetMountPointName(file->fseq);
		if (mount_name == NULL ||
		    (mpfd = open(mount_name, O_RDONLY)) < 0) {
			Trace(TR_ERR, "Unable to determine mount point "
			    "inode: %d.%d fseq: %d mp: \"%s\" errno: %d",
			    file->id.ino, file->id.gen, file->fseq,
			    mount_name == NULL ? "null" : mount_name, errno);
			file->error = ENODEV;
		} else {
			struct sam_ioctl_idopen arg;

			memset(&arg, 0, sizeof (arg));
			arg.id = file->id;

			while (err == 0 && fd == -1 && retry-- > 0) {
				fd = ioctl(mpfd, F_IDOPEN, &arg);
				if (fd == -1) {
					if (retry <= 0) {
						Trace(TR_ERR,
						    "Unable to verify copy %d "
						    "inode: %d.%d errno: %d",
						    file->copy + 1,
						    file->id.ino, file->id.gen,
						    errno);
						file->error = ENODEV;
						fd = -1;
						break;
					}
					sleep(5);
				} else {
					Trace(TR_MISC, "inode: %d.%d opened "
					    "for data verification",
					    file->id.ino, file->id.gen);
					NumOpenFiles++;
				}
			}
			(void) close(mpfd);
		}

	} else {

		memset(&arg.handle, 0, sizeof (sam_handle_t));
		arg.handle.id = file->id;
		arg.handle.fseq = file->fseq;

		arg.handle.stage_off = file->fs.stage_off;
		arg.handle.stage_len = file->len;
		arg.handle.flags.b.stage_wait = file->fs.wait;
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
		THREAD_LOCK(file);
		SET_FLAG(file->flags, FI_OPENING);
		THREAD_UNLOCK(file);

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

				SET_FLAG(Control->flags, CC_cancel);

				/*
				 * Set cancel flag in file, a duplicate request
				 * needs this status.
				 */
				SET_FLAG(file->flags, FI_CANCEL);

			}
		}
		CLEAR_FLAG(file->flags, FI_OPENING);
	}

	return (fd);
}

/*
 * Write data in i/o buffer to disk cache file.
 */
static int
writeDiskCache(
	int fd)
{
	char *offset;
	int count;
	int size_of_request;
	int nbytes_written;
	u_longlong_t reqlen;
	int out;
	offset_t staged_size;
	FileInfo_t *file;
	int copy;
	sam_ioctl_swrite_t stage_write;
	int first_pass;
	int notarhdr;
	boolean_t cancel;
	boolean_t error;
	boolean_t close_diskcache;
	int multivolume;
	/* LINTED variable unused in function */
	time_t end_time;
	/* LINTED variable unused in function */
	int secs;
	boolean_t checksumming;

	file = Control->file;
	copy = file->copy;

	/*
	 * If this is a multivolume stage request the stage length
	 * may be greater than data available on this volume.  Set
	 * data to write based on section length.
	 */
	if (file->len > file->ar[copy].section.length) {
		reqlen = file->ar[copy].section.length;
	} else {
		reqlen = file->len;
	}

	/*
	 * Verify tar header if not a never stage request that
	 * does not start at beginning of file.
	 */
	notarhdr = GET_FLAG(file->flags, FI_NO_TARHDR);

	first_pass = TRUE;
	staged_size = 0;
	cancel = FALSE;
	error = 0;
	checksumming = B_FALSE;

	stage_write.st_flags = 0;
	stage_write.offset = 0;

	if (GET_FLAG(file->flags, FI_DCACHE)) {
		stage_write.offset = file->write_off;
		Trace(TR_DEBUG, "Restore disk cache offset: %lld",
		    stage_write.offset);

		if (GET_FLAG(file->flags, FI_RETRY) && (file->write_off > 0)) {
			/*
			 * If multivolume set data to write based on
			 * section length.
			 */
			if (file->len > file->ar[copy].section.length) {
				reqlen = file->len - file->write_off;
				if (reqlen > file->ar[copy].section.length) {
					reqlen = file->ar[copy].section.length;
				}
			} else {
				reqlen = reqlen - file->write_off;
			}
			notarhdr = 1;
		}
	}

	if (GET_FLAG(file->flags, FI_USE_CSUM)) {
		int dcache;
		checksumming = B_TRUE;
		dcache = GET_FLAG(file->flags, FI_DCACHE) &&
		    file->write_off != 0;
		InitChecksum(file, dcache);
		if (dcache) {
			SetChecksumVal(file->csum_val);
		}
	}

	Trace(TR_DEBUG, "Start write inode: %d len: %lld",
	    file->id.ino, reqlen);

	multivolume = GET_FLAG(file->flags, FI_MULTIVOL);
	while (reqlen > 0 && cancel == FALSE) {

		/*
		 * Get buffer of data from read thread.  This thread
		 * will block until a data buffer is available.
		 */
		out = advanceOutBuffer();

		/*
		 * If data buffer contains read error, mark control
		 * structure in error and break out of write processing.
		 */
		if (Control->buffers[out].error) {
			error = Control->buffers[out].error;

			Trace(TR_MISC, "Found error inode: %d errno: %d "
			    "out: %d", file->id.ino, error, out);

			/*
			 * Signal reader thread that buffer was seen.
			 */
			ThreadSemaPost(&Control->emptyBuffers);

			break;
		}

		/* offset in buffer */
		offset = Control->buffers[out].data;

		/* count of bytes left */
		count =  Control->buffers[out].count;

		/*
		 * If first pass, verify tar header and adjust buffer
		 * over tar header.
		 */
		if (first_pass) {
			ulong_t tar_header;
			ulong_t skip_over;

			/*
			 * Use file offset calculated by read thread.
			 */
			skip_over  = Control->offset;
			offset = offset + skip_over;
			count  = count - skip_over;

			first_pass = FALSE;

			if (notarhdr == 0) {
				boolean_t verifylen;

				tar_header = Control->offset - TAR_RECORDSIZE;
				staged_size = TAR_RECORDSIZE;

				/*
				 * Can't validate request size against tar
				 * header's file size if multivolume, stage
				 * never or partial stage.
				 */
				verifylen = B_TRUE;
				if (GET_FLAG(file->flags, FI_MULTIVOL) ||
				    GET_FLAG(file->flags, FI_STAGE_NEVER) ||
				    GET_FLAG(file->flags, FI_STAGE_PARTIAL)) {
					verifylen = B_FALSE;
				}
				error = verifyTarHeader(
				    Control->buffers[out].data + tar_header,
				    NULL, reqlen, verifylen);
				if (error) {
					char pathBuffer[PATHBUF_SIZE];

					SetErrno = 0;
					SET_FLAG(file->flags, FI_TAR_ERROR);

					Trace(TR_ERR, "Invalid tar header "
					    "inode: %d out: %d",
					    file->id.ino, out);

					GetFileName(file, &pathBuffer[0],
					    PATHBUF_SIZE, NULL);
					SendCustMsg(HERE, 19032,
					    pathBuffer, file->id.ino,
					    file->id.gen, file->copy + 1);

					/*
					 * Cancel stage request, this will be
					 * picked up in the reader thread.
					 */
					Trace(TR_MISC, "Canceled(tar error) "
					    "inode: %d.%d",
					    file->id.ino, file->id.gen);
					SET_FLAG(Control->flags, CC_cancel);

					/*
					 * Signal reader thread that buffer
					 * was seen.
					 */
					ThreadSemaPost(&Control->emptyBuffers);

					break;
				}
			}
		}

		/*
		 * If enabled, trace transfer rate.  Traced variables:
		 * rate:   kilobyte per second average read transfer rate
		 * staged: number of bytes staged
		 * len:    length of file in bytes plus tar record size
		 *
		 *  Start time is set in reader thread after positioning
		 *  has completed.
		 */
#if 0
		(void) time(&end_time);
		secs = end_time - Control->start_time;
		if (secs == 0) secs = 1;
		if (staged_size > KILO) {
			Trace(TR_MISC, "Average transfer rate: %lld\n"
			    "\tstaged: %lld len: %lld",
			    (staged_size/KILO)/secs,
			    staged_size, file->len + TAR_RECORDSIZE);
		}
#endif

		size_of_request = (reqlen > count) ? count : reqlen;

		/*
		 * Check if data to write.  The tar header may be the only
		 * data in this buffer so there is no file data to write.
		 */
		nbytes_written = 0;
		if (size_of_request > 0) {

			stage_write.buf.ptr = offset;
			stage_write.nbyte = size_of_request;

			if (checksumming) {
				Checksum(stage_write.buf.ptr,
				    stage_write.nbyte);
			}

			Trace(TR_DEBUG, "Start write "
			    "fd: %d out: %d nbyte: %d offset: %lld",
			    fd, out, stage_write.nbyte, stage_write.offset);

			if (GET_FLAG(file->ar[copy].flags,
			    STAGE_COPY_VERIFY) == 0) {
				nbytes_written = ioctl(fd, F_SWRITE,
				    &stage_write);

				Trace(TR_DEBUG, "End write out: %d  %d",
				    out, nbytes_written);

				if (nbytes_written != size_of_request) {

					Trace(TR_ERR, "Write error: "
					    "%d fd: %d nbyte: %d offset: %lld",
					    errno, fd, stage_write.nbyte,
					    stage_write.offset);

					/*
					 * Cancel stage request, this will
					 * be picked up in the reader thread.
					 */
					Trace(TR_MISC, "Canceled(write error) "
					    "inode: %d.%d",
					    file->id.ino, file->id.gen);
					SET_FLAG(Control->flags, CC_cancel);

					if (errno == ECANCELED) {
						cancel = TRUE;
						SET_FLAG(file->flags,
						    FI_CANCEL);
					} else {
						error = errno;
						SET_FLAG(file->flags,
						    FI_WRITE_ERROR);
						/*
						 * Signal reader thread that
						 * buffer was seen.
						 */
						ThreadSemaPost(
						    &Control->emptyBuffers);

						break;
					}
				}
			} else {
				nbytes_written = stage_write.nbyte;
			}
		}

		reqlen = reqlen - nbytes_written;
		stage_write.offset = stage_write.offset + nbytes_written;
		staged_size += nbytes_written;
		file->stage_size += nbytes_written;

		if (checksumming && nbytes_written > 0) {
			WaitForChecksum();
		}

		/*
		 * Cancel stage request if shutdown for voluntary failover
		 * has requested, this will be picked up in the reader thread.
		 */
		if (GET_FLAG(Context->flags, CI_failover)) {
			Trace(TR_MISC, "Cancelled for voluntary failover "
			    "inode: %d.%d",
			    file->id.ino, file->id.gen);
			cancel = TRUE;
			SET_FLAG(Control->flags, CC_cancel);
		}

		ThreadSemaPost(&Control->emptyBuffers);
	}

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

	/*
	 * If no error AND cancel, close disk cache file.
	 */
	if (error == 0 && cancel == TRUE) {
		close_diskcache = TRUE;

	/*
	 * If no error AND not multivolume, close disk cache file.
	 */
	} else if (error == 0 && multivolume == 0) {
		close_diskcache = TRUE;

	/*
	 * If no error AND multivolume AND stage_n, close disk cache file.
	 */
	} else if (error == 0 && multivolume != 0 &&
	    GET_FLAG(file->flags, FI_STAGE_NEVER) &&
	    file->stage_size == file->len) {
		close_diskcache = TRUE;

	/*
	 * Else, an error OR more VSNs to stage, save disk cache information.
	 */
	} else {
		/*
		 * If no device available or interrupted system call,
		 * close disk cache.
		 */
		if (error == ENODEV || error == EINTR) {
			close_diskcache = TRUE;
		} else {
			close_diskcache = FALSE;
		}
	}

	/*
	 * Checksumming may have found an error.
	 */
	if (close_diskcache == B_TRUE && checksumming == B_TRUE) {
		/*
		 * Do not check for checksum error if request was
		 * cancelled or an I/O error occurred.
		 */
		if (cancel == B_FALSE && error == 0) {
			error = ChecksumCompare(fd, &file->id);
			SetChecksumDone();
			if (error) {
				close_diskcache = FALSE;
				stage_write.offset = 0;
				if (file->ar[copy].flags & STAGE_COPY_VERIFY) {
					Trace(TR_ERR, "Unable to verify "
					    "copy %d inode: %d.%d errno: %d",
					    file->copy + 1, file->id.ino,
					    file->id.gen, error);
					close_diskcache = TRUE;
				}
			} else {
				if (file->ar[copy].flags & STAGE_COPY_VERIFY) {
					setVerified(file, copy);
					file->ar[copy].flags &=
					    ~STAGE_COPY_VERIFY;
				}
			}
		} else {
			SetChecksumDone();
			if (file->ar[copy].flags & STAGE_COPY_VERIFY) {
				Trace(TR_ERR, "Unable to verify "
				    "copy %d inode: %d.%d errno: %d",
				    file->copy + 1, file->id.ino,
				    file->id.gen, errno);
			}
		}
		checksumming = B_FALSE;
	}

	if (close_diskcache) {
		(void) close(fd);
		if (GET_FLAG(file->flags, FI_DCACHE)) {
			CLEAR_FLAG(file->flags, FI_DCACHE);
		}
		NumOpenFiles--;

	} else {
		setFileError(file, fd, stage_write.offset, error);
		if (checksumming && multivolume != 0) {
			file->csum_val = GetChecksumVal();
			SetChecksumDone();
		}
	}

	return (file->error);
}

/*
 * Verify tar header.
 */
static int
verifyTarHeader(
	char *buffer,
	char *name,
	u_longlong_t reqlen,
	boolean_t verifylen)
{
	struct header *tar_header;
	u_longlong_t tarsize;

	tar_header = (struct header *)buffer;
	if (memcmp(&tar_header->magic, TMAGIC, sizeof (TMAGIC)) != 0) {
		TraceRawData(TR_MISC, buffer, 512);
		return (EIO);
	}

	/*
	 * Validate request length against tar header's file size.
	 */
	if (verifylen) {
		tarsize = llfrom_str(sizeof (tar_header->size),
		    tar_header->size);
		if (tarsize != reqlen) {
			Trace(TR_MISC, "Request length (%lld) does not match "
			    "tar header (%lld) file size", reqlen, tarsize);
			return (EIO);
		}
	}

	/*
	 * Extra sanity check to validate file name matches tar header.
	 */
	if (name) {
		if (strstr(name, tar_header->arch_name) == NULL) {
			ASSERT_NOT_REACHED();
		}
	}
	return (0);
}

/*
 * Set media allocation unit information.  A media allocation unit
 * is defined as the size of the smallest addressable part on a
 * media that can be accessed.  Buffers are a multiple of the
 * ma unit size.
 */
static void
setMau(
	offset_t block_size)
{
	Control->mau = block_size;
	Control->mauInBuffer = Control->bufSize / block_size;

	Trace(TR_MISC, "Set mau: %lld %d",
	    Control->mau, Control->mauInBuffer);
}

/*
 * Set error condition for specified file.
 */
static void
setFileError(
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
		file->context = Context->pid;
		SET_FLAG(file->flags, FI_DCACHE);
		Trace(TR_MISC, "Set file dcache: %d write_off: %lld error: %d",
		    fd, write_off, error);
		Trace(TR_MISC, "\tvsn_cnt: %d context: %d",
		    file->vsn_cnt, (int)file->context);
}

/*
 * Use the SC_archive_copy syscall to set the data verified bit.
 */
void
setVerified(
	FileInfo_t *file,
	int copy)
{
	struct sam_archive_copy_arg arg;
	char pathBuffer[PATHBUF_SIZE];
	int segment_ord;
	int syserr;

	GetFileName(file, &pathBuffer[0], PATHBUF_SIZE, &segment_ord);
	/*
	 * If segment file, remove ordinal from file name.
	 */
	if (segment_ord > 0) {
		char *p;

		p = strrchr(&pathBuffer[0], '/');
		if (p != NULL) {
			*p = '\0';
		}
	}

	memset(&arg, 0, sizeof (arg));
	arg.copies = 1 << copy;
	arg.operation = OP_verified;
	arg.path.ptr = pathBuffer;

	syserr = sam_syscall(SC_archive_copy, &arg, sizeof (arg));
	if (syserr == -1) {
		WarnSyscallError(HERE, "SC_archive_copy", pathBuffer);
	}
}
