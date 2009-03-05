/*
 * stream.c - stager's stream support
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

#pragma ident "$Revision: 1.58 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <pthread.h>

/* Solaris headers. */

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/shm.h"
#include "pub/stat.h"
#include "aml/device.h"
#include "aml/stager.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "sam/custmsg.h"
#include "sam/exit.h"
#include "aml/stager_defs.h"
#include "sam/lib.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Local headers. */
#include "stager_lib.h"
#include "stager_threads.h"
#include "stager_config.h"
#include "stager_shared.h"
#include "rmedia.h"
#include "stream.h"
#include "copy_defs.h"

#include "stager.h"
#include "stage_reqs.h"

static upath_t fullpath;
static pthread_mutex_t createStreamMutex = PTHREAD_MUTEX_INITIALIZER;

static FileInfo_t **makeSortList(StreamInfo_t *stream);
static void separatePositions(StreamInfo_t *stream, FileInfo_t **sortList);
static void sortStream(StreamInfo_t *stream, FileInfo_t **sortList);
static int comparePosition(const void *p1, const void *p2);
static boolean_t addActiveStream(StreamInfo_t *stream, int id);
static boolean_t appendActiveStream(StreamInfo_t *stream, FileInfo_t *last,
    int id);
static void checkStreamLimits(StreamInfo_t *stream, FileInfo_t *file);

int Seqnum = 1;		/* Stream sequence number */

/*
 * Create new stage stream.  A stream is a
 * group of files to be staged together.
 */
StreamInfo_t *
CreateStream(
	FileInfo_t *file)
{
	int ret;
	StreamInfo_t *stream;
	size_t size;
	pthread_mutexattr_t mattr;
	int copy;

	PthreadMutexLock(&createStreamMutex);
	copy = file->copy;
	size = sizeof (StreamInfo_t);
	SamMalloc(stream, size);
	(void) memset(stream, 0, size);

	stream->create = time(NULL);
	if (GET_FLAG(file->flags, FI_DCACHE_CLOSE)) {
		(void) strncat(stream->vsn, "CLEAN", sizeof ("CLEAN"));
		stream->flags = SR_DCACHE_CLOSE;
	} else {
		(void) strncpy(stream->vsn, file->ar[copy].section.vsn,
		    sizeof (vsn_t));
	}
	stream->media = file->ar[copy].media;
	stream->diskarch = IF_COPY_DISKARCH(file, copy);
	stream->thirdparty = is_third_party(file->ar[copy].media);

	/*
	 * If disk cache already open, multivolume or error retry, on this
	 * file save copy's context in stream.
	 */
	if (GET_FLAG(file->flags, FI_DCACHE)) {
		stream->context = file->context;
	}

	/* Set stream limit parameters for disk archives. */
	if (stream->diskarch == B_TRUE) {
		sam_stager_streams_t *params;

		params = GetCfgStreamParams(stream->media);
		if (params != NULL) {
			stream->sr_params.sr_flags = SP_maxsize | SP_maxcount;
			stream->sr_params.sr_maxCount = params->ss_maxCount;
			stream->sr_params.sr_maxSize = params->ss_maxSize;
		} else {
			stream->sr_params.sr_flags = SP_maxsize;
			stream->sr_params.sr_maxSize = GIGA;
		}
	}

	stream->seqnum = Seqnum;
	stream->first = stream->last = EOS;

	PthreadMutexattrInit(&mattr);
	PthreadMutexattrSetpshared(&mattr, PTHREAD_PROCESS_SHARED);
	PthreadMutexInit(&stream->mutex, &mattr);
	PthreadMutexattrDestroy(&mattr);

	(void) sprintf(fullpath, "%s/%s.%d",
	    SharedInfo->si_streamsDir, stream->vsn, Seqnum);

	ret = WriteMapFile(fullpath, (void *)stream, size);
	if (ret != 0) {
		FatalSyscallError(EXIT_NORESTART, HERE, "WriteMapFile",
		    fullpath);
	}
	SamFree(stream);
	stream = (StreamInfo_t *)MapInFile(fullpath, O_RDWR, NULL);
	if (stream != NULL) {
		Trace(TR_QUEUE, "Create stream: '%s.%d' 0x%x "
		    "media: '%s' context: %d",
		    stream->vsn, Seqnum, (int)stream,
		    sam_mediatoa(stream->media), (int)stream->context);
	} else {
		WarnSyscallError(HERE, "MapInFile", fullpath);
	}
	if (Seqnum >= INT_MAX) {
		Seqnum = 1;
	} else {
		Seqnum++;
	}
	PthreadMutexUnlock(&createStreamMutex);
	return (stream);
}

/*
 * Error stage stream.  Set error for all files in stream.
 */
void
ErrorStream(
	StreamInfo_t *stream,
	int error)
{
	FileInfo_t *file;


	Trace(TR_MISC, "Error stream: '%s' 0x%x error: %d",
	    TrNullString(stream->vsn), (int)stream, error);

	PthreadMutexLock(&stream->mutex);
	SET_FLAG(stream->flags, SR_ERROR);
	PthreadMutexUnlock(&stream->mutex);

	while (stream->first > EOS) {

		PthreadMutexLock(&stream->mutex);
		file = GetFile(stream->first);
		PthreadMutexLock(&file->mutex);
		file->error = error;

		/*
		 * If canceled, send error response to filesystem.
		 * Else verify that another valid copy exists.
		 * If not, an error response must be sent
		 * to the filesystem.
		 */
		if (error == ECANCELED ||
		    GetArcopy(file, file->copy + 1) == file->copy) {
			StageError(file, error);
			SET_FLAG(file->flags, FI_NO_RETRY);
		}

		stream->first = file->next;
		stream->count--;

		if (stream->first == EOS) {
			stream->last = EOS;
		}
		PthreadMutexUnlock(&stream->mutex);
		PthreadMutexUnlock(&file->mutex);

		SetStageDone(file);
	}

	/*
	 * Set active and done flag on stream for delete.
	 */
	PthreadMutexLock(&stream->mutex);
	SET_FLAG(stream->flags, SR_ACTIVE);
	SET_FLAG(stream->flags, SR_DONE);
	CLEAR_FLAG(stream->flags, SR_ERROR);
	PthreadMutexUnlock(&stream->mutex);
}

/*
 * Clear stage stream.
 */
void
ClearStream(
	StreamInfo_t *stream)
{
	FileInfo_t *file;
	boolean_t active;

	while (stream->first > EOS) {

		PthreadMutexLock(&stream->mutex);
		file = GetFile(stream->first);

		PthreadMutexLock(&file->mutex);
		active = GET_FLAG(file->flags, FI_ACTIVE);

		if (active == B_FALSE &&
		    GET_FLAG(file->flags, FI_DCACHE_CLOSE) == 0) {
			SET_FLAG(file->flags, FI_CANCEL);
			StageError(file, ECANCELED);
		}
		stream->first = file->next;
		stream->count--;

		if (stream->first == EOS) {
			stream->last = EOS;
		}

		PthreadMutexUnlock(&stream->mutex);
		PthreadMutexUnlock(&file->mutex);

		if (active == B_FALSE) {
			SetStageDone(file);
		}
	}

	/*
	 * Set active and done flags on stream for delete.
	 */
	SET_FLAG(stream->flags, SR_ACTIVE);
	SET_FLAG(stream->flags, SR_DONE);
}

/*
 * Delete stage stream.
 */
void
DeleteStream(
	StreamInfo_t *stream)
{
	Trace(TR_QUEUE, "Delete stream: '%s.%d' 0x%x",
	    stream->vsn, stream->seqnum, (int)stream);

	(void) sprintf(fullpath, "%s/%s.%d", SharedInfo->si_streamsDir,
	    stream->vsn, stream->seqnum);
	RemoveMapFile(fullpath, stream, sizeof (StreamInfo_t));
}

/*
 * Add stage file request to an existing stream.  Returns TRUE
 * if the request was successfully added.  The existing stream can
 * be active, if this is the case then this request may fail.  Returns
 * FALSE if the request could not be added to the stream.
 */
boolean_t
AddStream(
	StreamInfo_t *stream,
	int id,
	int sort)
{
	boolean_t added;
	ASSERT(stream != NULL);

	added = B_FALSE;

	/* Stream already at capacity. */
	if (GET_FLAG(stream->flags, SR_full)) {
		return (added);
	}

	/*
	 * Add request to inactive stream.  We don't need to lock
	 * the stream to check if active.  This thread is responsible
	 * for scheduling, if its not active already its not going to
	 * change out from underneath us.
	 */
	if (GET_FLAG(stream->flags, SR_ACTIVE) == 0) {
		FileInfo_t *file, *last;

		/*
		 * If adding request and stream have a context assigned,
		 * those should match since request's dcache is opened in
		 * a specific copy process.
		 */
		file = GetFile(id);
		if (GET_FLAG(file->flags, FI_DCACHE) &&
		    (stream->context != NULL &&
		    stream->context != file->context)) {
			return (B_FALSE);
		}

		/*
		 * Lock stream to protect it from the loadExportedVol
		 * thread removing staled files while this thread is
		 * add entries to the stream.
		 */
		PthreadMutexLock(&stream->mutex);
		if (GET_FLAG(stream->flags, SR_ERROR)) {
			PthreadMutexUnlock(&stream->mutex);
			return (B_FALSE);
		}

		if (IS_STREAM_EMPTY(stream)) {
			stream->first = id;
			stream->last = id;
		} else {
			last = GetFile(stream->last);
			last->next  = id;
			stream->last = id;
		}

		last = GetFile(id);
		last->next = EOS;

		/*
		 * If disk cache already open, multivolume or error retry,
		 * on this file save copy's context in stream.
		 */
		if (GET_FLAG(last->flags, FI_DCACHE)) {
			stream->context = last->context;
		}

		added = B_TRUE;
		checkStreamLimits(stream, file);

		/*
		 * If requested, sort inactive stream by position.  If a disk
		 * archive file, sorting requests by position is unnecessary.
		 */
		if (sort == ADD_STREAM_SORT) {
			FileInfo_t **sortList;

			sortList = makeSortList(stream);
			separatePositions(stream, sortList);
		}
		PthreadMutexUnlock(&stream->mutex);

	} else {
		/*
		 * Add stage file request to an active stream.
		 */
		added = addActiveStream(stream, id);
	}

	return (added);
}

/*
 * Cancel request in stage stream.
 */
boolean_t
CancelStream(
	StreamInfo_t *stream,
	int id)
{
	FileInfo_t *curr;
	int curr_id;
	boolean_t found = B_FALSE;
	boolean_t done = B_FALSE;

	FileInfo_t *prev = NULL;
	FileInfo_t *file = GetFile(id);

	curr_id = stream->first;
	while (curr_id > EOS) {

		curr = GetFile(curr_id);

		if (curr->id.ino == file->id.ino &&
		    curr->id.gen == file->id.gen &&
		    curr->fseq == file->fseq) {

			found = B_TRUE;
			/*
			 * Remove file from stream.
			 */
			PthreadMutexLock(&stream->mutex);
			PthreadMutexLock(&file->mutex);
			if (prev != NULL) {
				PthreadMutexLock(&prev->mutex);
			}
			if (GET_FLAG(file->flags, FI_ACTIVE) == 0) {
				if (prev != NULL) {
					prev->next = file->next;
				} else {
					stream->first = file->next;
				}
				SET_FLAG(file->flags, FI_CANCEL);
				if (GET_FLAG(file->flags, FI_DCACHE) == 0) {
					StageError(file, ECANCELED);
				}
				stream->count--;
				if (stream->first == EOS) {
					stream->last = EOS;
					/*
					 * Set flags so stream is deleted.
					 */
					SET_FLAG(stream->flags, SR_ACTIVE);
					SET_FLAG(stream->flags, SR_DONE);
				} else if (file->sort == stream->last) {
					ASSERT(prev != NULL);
					stream->last = prev->sort;
				}
				done = B_TRUE;
			}
			if (prev != NULL) {
				PthreadMutexUnlock(&prev->mutex);
			}
			PthreadMutexUnlock(&file->mutex);
			PthreadMutexUnlock(&stream->mutex);

			/*
			 * Mark file as done.
			 */
			if (done) {
				SetStageDone(file);
			}
			break;
		}

		curr_id = curr->next;
		prev = curr;
	}
	return (found);
}

void
SetStreamActive(
	StreamInfo_t *stream)
{
	SET_FLAG(stream->flags, SR_ACTIVE);
}

void
SetStreamDone(
	StreamInfo_t *stream)
{
	SET_FLAG(stream->flags, SR_DONE);
}

/*
 * Attempt to add a stage file request to an active
 * stream.
 */
static boolean_t
addActiveStream(
	StreamInfo_t *stream,
	int id)
{
	FileInfo_t *file;
	FileInfo_t *first;
	int fid;
	int lid;
	FileInfo_t *last;
	boolean_t added;

	added = B_FALSE;
	file = GetFile(id);

	/* Stream already at capacity. */
	if (GET_FLAG(stream->flags, SR_full)) {
		return (B_FALSE);
	}

	/*
	 * If disk cache already open on this file do not add
	 * it to an active stream.
	 */
	if (GET_FLAG(file->flags, FI_DCACHE)) {
		return (B_FALSE);
	}

	PthreadMutexLock(&stream->mutex);

	/*
	 * Check if stream has already completed.  Don't add a
	 * new stage request to a completed stream.
	 */
	if (GET_FLAG(stream->flags, SR_DONE) ||
	    stream->first == EOS || stream->last == EOS) {
		PthreadMutexUnlock(&stream->mutex);
		return (B_FALSE);
	}

	fid = stream->first;
	first = GetFile(fid);

	lid = stream->last;
	last = GetFile(lid);

	/*
	 * If disk archive its not necessary to sort, append to end
	 * of the active stream.
	 */
	if (stream->diskarch) {
		PthreadMutexLock(&last->mutex);
		added = appendActiveStream(stream, last, id);

		PthreadMutexUnlock(&stream->mutex);
		PthreadMutexUnlock(&last->mutex);
		goto out;
	}

	if (comparePosition(&file, &first) < 0) {
		/*
		 * Add new stage file request as first
		 * entry in stream.
		 */
		PthreadMutexLock(&first->mutex);
		if (GET_FLAG(first->flags, FI_ACTIVE) == 0) {

			file->next = fid;
			stream->first = id;
			checkStreamLimits(stream, file);
			added = B_TRUE;
		}
		PthreadMutexUnlock(&stream->mutex);
		PthreadMutexUnlock(&first->mutex);

	} else if (comparePosition(&file, &last) > 0) {

		PthreadMutexLock(&last->mutex);
		added = appendActiveStream(stream, last, id);

		PthreadMutexUnlock(&stream->mutex);
		PthreadMutexUnlock(&last->mutex);

	} else {

		boolean_t locked;
		FileInfo_t *f1, *f2;
		int id1, id2;

		locked = B_TRUE;		/* stream locked on entry */

		id1 = fid;
		f1 = first;

		id2 = first->next;
		f2 = GetFile(id2);

		/*
		 * Loop over files in stream checking if request can
		 * be inserted between.  We've already checked if it
		 * can be inserted before first or after last.
		 */
		while (id1 != lid) {

			if ((comparePosition(&file, &f1) > 0) &&
			    (comparePosition(&file, &f2) < 0)) {

				PthreadMutexLock(&f1->mutex);
				if (GET_FLAG(f1->flags, FI_DONE) == 0) {

					f1->next = id;
					file->next = id2;
					checkStreamLimits(stream, file);
					added = B_TRUE;

				}
				PthreadMutexUnlock(&stream->mutex);
				PthreadMutexUnlock(&f1->mutex);

				/*
				 * Exiting loop with stream unlocked.
				 */
				locked = B_FALSE;
				break;
			}
			id1 = id2;
			id2 = f2->next;

			f1 = GetFile(id1);
			f2 = GetFile(id2);
		}

		/*
		 * If unable to add file, then stream may be locked and must be
		 * unlocked before exiting.  If the file was added, the stream
		 * is already unlocked from lock chaining with a file entry.
		 */
		if (locked == B_TRUE) {
			PthreadMutexUnlock(&stream->mutex);
		}
	}

out:
	return (added);
}

/*
 * Append stage file request to end of active stream.
 * Stream and last file should be locked on entry to
 * this function.
 */
static boolean_t
appendActiveStream(
	StreamInfo_t *stream,
	FileInfo_t *last,
	int id)
{
	FileInfo_t *file;
	boolean_t added;

	added = B_FALSE;
	file = GetFile(id);
	if (GET_FLAG(last->flags, FI_DONE) == 0) {

		last->next = id;
		stream->last = id;
		file->next = EOS;
		checkStreamLimits(stream, file);
		added = B_TRUE;
	}
	return (added);
}

/*
 * Construct list of stream pointers.  This list is used
 * in performing sorts.  The actual list is private to this
 * module.  Its size will be increased to hold the required list.
 */
static FileInfo_t **
makeSortList(
	StreamInfo_t *stream)
{
	static FileInfo_t **sortList = NULL;
	static size_t alloc = 0;

	int i;
	size_t size;
	FileInfo_t *file;
	int id;

	size = stream->count * sizeof (FileInfo_t *);
	if (size > alloc) {
		/*
		 * Increasing size of sort list, free old stuff.
		 */
		if (alloc > 0 && sortList != NULL) {
			SamFree(sortList);
		}
		alloc = size;
		SamMalloc(sortList, alloc);
	}

	i = 0;
	id = stream->first;
	while (id >= 0) {
		file = GetFile(id);
		sortList[i] = file;
		id = file->next;
		i++;
	}
	return (sortList);
}

/*
 * Create sorted list based on position.
 */
static void
separatePositions(
	StreamInfo_t *stream,
	FileInfo_t **sortList)
{
	qsort(sortList, stream->count, sizeof (FileInfo_t *), comparePosition);

	sortStream(stream, sortList);
}

/*
 * Sort list of streams.
 */
static void
sortStream(
	StreamInfo_t *stream,
	FileInfo_t **sortList)
{
	int i;
	FileInfo_t *file;
	FileInfo_t *last;

	file = sortList[0];
	stream->first = file->sort;

	for (i = 1; i < stream->count; i++) {
		last = file;
		file = sortList[i];
		last->next = file->sort;
	}

	file->next = EOS;
	stream->last = file->sort;
}

/*
 * Compare position ascending.
 */
static int
comparePosition(
	const void *p1,
	const void *p2)
{
	int copy1;
	int copy2;
	ArcopyInfo_t *ar1;
	ArcopyInfo_t *ar2;
	FileInfo_t **f1 = (FileInfo_t **)p1;
	FileInfo_t **f2 = (FileInfo_t **)p2;

	if ((*f1) == (*f2)) {
		return (0);
	}

	if ((*f1) == NULL || (*f2) == NULL) {
		SetErrno = 0;	/* set for trace */
		Trace(TR_ERR, "Compare position error: 0x%x 0x%x",
		    (int)(*f1), (int)(*f2));
		return (1);
	}

	copy1 = (*f1)->copy;
	copy2 = (*f2)->copy;
	ar1 = &((*f1)->ar[copy1]);
	ar2 = &((*f2)->ar[copy2]);

	if (ar1->section.position > ar2->section.position)
		return (1);
	if (ar1->section.position < ar2->section.position)
		return (-1);

	if (ar1->section.offset > ar2->section.offset)
		return (1);
	if (ar1->section.offset < ar2->section.offset)
		return (-1);

	/*
	 * Positions match.  Check if duplicate request.
	 */
	if ((*f1)->id.ino == (*f2)->id.ino &&
	    (*f1)->id.gen == (*f2)->id.gen &&
	    (*f1)->fseq   == (*f2)->fseq &&
	    (*f1)->offset == (*f2)->offset) {

		if ((GET_FLAG((*f1)->flags, FI_DUPLICATE) == 0) &&
		    (GET_FLAG((*f2)->flags, FI_DUPLICATE) == 0)) {

			PthreadMutexLock(&(*f1)->mutex);
			PthreadMutexLock(&(*f2)->mutex);

			if ((GET_FLAG((*f1)->flags, FI_OPENING) == 0) &&
			    (GET_FLAG((*f2)->flags, FI_OPENING) == 0)) {
				Trace(TR_MISC, "Duplicate file inode: "
				    "%d.%d (%d) [%d.%d] (%d)",
				    (*f1)->id.ino, (*f1)->id.gen, (*f1)->fseq,
				    (*f2)->id.ino, (*f2)->id.gen, (*f2)->fseq);
				/*
				 * One of these files may be actively staging.
				 * Don't mark an active file as a duplicate.
				 */
				if (GET_FLAG((*f1)->flags, FI_ACTIVE) == 0) {
					SET_FLAG((*f1)->flags, FI_DUPLICATE);
				} else {
					SET_FLAG((*f2)->flags, FI_DUPLICATE);
				}
			}

			PthreadMutexUnlock(&(*f1)->mutex);
			PthreadMutexUnlock(&(*f2)->mutex);

		}
		return (-1);
	}

	/*
	 * Positions match and not a duplicate. Unusual but can happen
	 * if one of the files is a SAM-FS restored file.
	 */
	Trace(TR_MISC, "File positions matched [%d.%d %d] [%d.%d %d]",
	    (*f1)->id.ino, (*f1)->id.gen, copy1,
	    (*f2)->id.ino, (*f2)->id.gen, copy2);

	return (1);
}

/*
 * Check stream limit parameters.  Mark the stream full if any of the
 * stream limits have been met.
 */
static void
checkStreamLimits(
	StreamInfo_t *stream,
	FileInfo_t *file
)
{
	stream->count++;
	stream->size += file->len;

	if (GET_FLAG(stream->sr_params.sr_flags, SP_maxcount)) {
		if (stream->count >= stream->sr_params.sr_maxCount) {
			/* Max number of files limit hit. */
			SET_FLAG(stream->flags, SR_full);
		}
	}

	if (GET_FLAG(stream->sr_params.sr_flags, SP_maxsize)) {
		if (stream->size >= stream->sr_params.sr_maxSize) {
			/* Max size of stream limit hit. */
			SET_FLAG(stream->flags, SR_full);
		}
	}
}
