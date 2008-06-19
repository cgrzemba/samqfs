/*
 * thirdparty.c - provide support for staging files on third party media
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

#pragma ident "$Revision: 1.22 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

/* POSIX headers. */
#include <dlfcn.h>

/* Solaris headers. */

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/lib.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "aml/stager.h"
#include "sam/custmsg.h"
#include "sam/exit.h"
#include "aml/shm.h"
#include "aml/stager_defs.h"

/* Local headers. */
#include "stager_lib.h"
#include "stager_threads.h"
#include "stager_config.h"
#include "copy_defs.h"
#include "file_defs.h"
#include "rmedia.h"

#include "stager.h"
#include "thirdparty.h"
#include "schedule.h"

void *what_device;
extern shm_alloc_t master_shm;
extern shm_ptr_tbl_t *shm_ptr_tbl;

#define	NUM_TOOLKIT_API 3

static int initToolkitList();
static void addThirdPartyRequest(int id);
static void *thirdPartyStage(void *arg);
static void stageFile(ThirdPartyInfo_t *mig, FileInfo_t *file);
static enum StreamPriority findResources(StreamInfo_t *stream);
static void cancelThirdPartyStream(ThirdPartyInfo_t *mig, int id);

static boolean_t infiniteLoop = B_TRUE;

/* Migration toolkit table. */
static struct {
	char			*entryPointNames[NUM_TOOLKIT_API];
	int			entries;
	pthread_mutex_t		mutex;		/* protect access */
	ThirdPartyInfo_t	*data;
} toolkitList = {

	/*
	 * Be careful.  The following api entry point names must
	 * match the definitions as found in the ThirdPartyInfo structure.
	 */
	"usam_mig_initialize",
	"usam_mig_stage_file_req",
	"usam_mig_cancel_stage_req",

	0, PTHREAD_MUTEX_INITIALIZER, NULL };

static struct {
	pthread_mutex_t mutex;	/* protect access */
	pthread_cond_t cond;	/* condition set when thread is running */
	boolean_t running;	/* third party stage thread is running */
} create_thread;

SchedComm_t MigComm = {
	PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, NULL, NULL, 0
};



/*
 * Send stage request to migrator thread.  The MigComm variable is used
 * for communication to the migrator.
 */
int
SendToMigrator(
	int id)
{
	SchedRequest_t *request;

	PthreadMutexLock(&MigComm.mutex);

	SamMalloc(request, sizeof (SchedRequest_t));

	request->id = id;
	request->next = NULL;

	/*
	 * Send new data.
	 */
	if (MigComm.first == NULL) {

		MigComm.first = request;
		MigComm.last = request;

	} else {

		(MigComm.last)->next = request;
		MigComm.last = request;
	}

	PthreadCondSignal(&MigComm.avail);

	PthreadMutexUnlock(&MigComm.mutex);
	return (0);
}

/*
 * Stager's migrator thread.  This thread is responsible for
 * handing out third party media stage file requests.
 */
void *
Migrator(
	void *arg)
{
	SchedComm_t *comm;
	SchedRequest_t *request;
	struct timespec timeout;
	int rval;

	comm = (SchedComm_t *)arg;
	if (comm == NULL) {
		SendCustMsg(HERE, 19009);
		exit(EXIT_NORESTART);
	}

	/*
	 * Start an infinite loop waiting for stage requests to
	 * appear.
	 */
	while (infiniteLoop) {

		timeout.tv_sec = time(NULL) + SCHEDULER_TIMEOUT_SECS;
		timeout.tv_nsec = 0;

		PthreadMutexLock(&comm->mutex);

		/*
		 * Wait on the condition variable for SCHEDULER_TIMEOUT_SECS or
		 * until signaled there is a request available.
		 */
		while (comm->first == NULL) {

			rval = pthread_cond_timedwait(&comm->avail,
			    &comm->mutex, &timeout);
			if (rval == ETIMEDOUT) {
				/*
				 * Wait timed out.  Check if drive state has
				 * changed and other stage requests can now be
				 * scheduled.
				 */
				break;
			}
		}

		/*
		 * Check if new stage request to schedule.
		 */
		while (comm->first != NULL) {

			request = comm->first;

			addThirdPartyRequest(request->id);

			comm->first = request->next;

			SamFree(request);

			if (comm->first == NULL) {
				comm->last = NULL;
			}
		}

		PthreadMutexUnlock(&comm->mutex);

	}
	return (NULL);
}

/*
 * Initialize migration toolkit.
 */
int
InitMigration(
	dev_ent_t *dev)
{
	static int idx;
	void *handle;
	char *entryPointName;
	int i;
	int (**api)();
	char *errmsg;
	ThirdPartyInfo_t *mig = NULL;
	int rc = 0;

	/*
	 * Check if migration toolkit library has already been initialized.
	 */
	if (toolkitList.data != NULL) {
		ThirdPartyInfo_t *data;

		for (i = 0; i < toolkitList.entries; i++) {
			data = &toolkitList.data[i];
			if (data != NULL && data->equ_type == dev->equ_type) {
				return (0);
			}
		}
	}

	if ((handle = dlopen(dev->name, RTLD_NOW)) != NULL) {
		/*
		 * Migration toolkit loaded.
		 */
		SendCustMsg(HERE, 19024, dev->name);

		idx = initToolkitList();

		toolkitList.data[idx].dev = dev;
		toolkitList.data[idx].equ_type = dev->equ_type;
		api = &toolkitList.data[idx].mig_initialize;

		for (i = 0; i < NUM_TOOLKIT_API; i++, api++) {
			entryPointName = toolkitList.entryPointNames[i];
			*api = (int (*)())dlsym(handle, entryPointName);
			if (*api == NULL) {
				/*
				 * Migration toolkit api not found.
				 */
				SendCustMsg(HERE, 19025, entryPointName);
				errmsg = dlerror();
				if (errmsg != NULL) {
					Trace(TR_ERR, "\t%s", errmsg);
				}
				rc = -1;
			}
		}

	} else {
		/*
		 * Migration toolkit not loaded.
		 */
		SendCustMsg(HERE, 19026, dev->name);

		errmsg = dlerror();
		if (errmsg != NULL) {
			Trace(TR_ERR, "\t%s", errmsg);
		}
		rc = -1;
	}

	/*
	 * If migration toolkit library API was found and loaded, invoke
	 * the initialization routine.
	 */
	if (rc == 0) {
		long max_active = GetCfgMaxActive();

		Trace(TR_MISC, "Invoke migration initialization function");

		mig = &toolkitList.data[idx];
		if (mig == NULL || mig->mig_initialize(max_active) != 0) {
			/*
			 * Migration toolkit initialize function failed.
			 */
			SendCustMsg(HERE, 19027, dev->name);
			rc = -1;
		}
	}

	if (rc == 0) {

		mig->stream = NULL;

		/*
		 * Start a thread for each third party media type.  The thread
		 * will monitor the work list for migration stage requests.
		 */
		create_thread.running = B_FALSE;
		PthreadCondInit(&create_thread.cond, NULL);
		PthreadMutexInit(&create_thread.mutex, NULL);

		if (pthread_create(&mig->tid, NULL, thirdPartyStage,
		    (void *)&idx) != 0) {
			WarnSyscallError(HERE, "pthread_create", "");
		}

		PthreadMutexLock(&create_thread.mutex);
		while (create_thread.running == B_FALSE) {
			PthreadCondWait(&create_thread.cond,
			    &create_thread.mutex);
		}
		PthreadMutexUnlock(&create_thread.mutex);

	}

	return (rc);
}

/*
 * Issue stage request for third party media.
 */
static void
stageFile(
	ThirdPartyInfo_t *mig,
	FileInfo_t *file)
{
	int copy;
	media_t media;
	MigFileInfo_t *migfile;

	copy = file->copy;
	media = file->ar[copy].media;

	/*
	 * Build stage request for API function.
	 */
	migfile = (MigFileInfo_t *)malloc(sizeof (MigFileInfo_t));
	if (migfile == NULL) {
		StageError(file, EIO);
		SetStageDone(file);
		return;
	}

	/*
	 * Build request for migration toolkit library.
	 */
	(void) memset(migfile, 0, sizeof (MigFileInfo_t));
	migfile->req.offset = file->offset;
	migfile->req.size = file->len;
	migfile->req.position = file->ar[copy].section.position;
	(void) strncpy(migfile->req.vsn, file->ar[copy].section.vsn,
	    sizeof (vsn_t));
	migfile->req.inode = file->id.ino;
	migfile->req.fseq = file->fseq;
	migfile->req.media_type = media;

	migfile->stream = mig->stream;
	migfile->file = file;
	file->dcache = -1;		/* initialize stage file descriptor */
	what_device = migfile->dev = (void *)mig->dev;

	Trace(TR_MISC, "[t@%d] Third party inode: %ld\n\tlen: %lld "
	    "offset: %lld\n\tmedia: '%s' pos: %llx vsn: '%s'",
	    pthread_self(), migfile->req.inode, migfile->req.size,
	    migfile->req.offset, sam_mediatoa(migfile->req.media_type),
	    migfile->req.position, migfile->req.vsn);

	if (mig->mig_stage_file_req((tp_stage_t *)migfile) != 0) {
		Trace(TR_DEBUG, "[t@%d] Third party stage rejected inode: %d",
		    pthread_self(), (int)migfile->req.inode);
		StageError(file, errno);
	}
}

/*
 * Initialize toolkit list.
 */
static int
initToolkitList(void)
{
	int size;
	int idx;

	idx = toolkitList.entries;
	toolkitList.entries++;
	size = toolkitList.entries * sizeof (ThirdPartyInfo_t);

	if (toolkitList.data == NULL) {
		SamMalloc(toolkitList.data, size);
	} else {
		PthreadMutexLock(&toolkitList.mutex);
		SamRealloc(toolkitList.data, size);
		PthreadMutexUnlock(&toolkitList.mutex);
	}
	return (idx);
}

/*
 * Add request to stream.  The request will be picked up by a thread
 * for this media type.
 */
static void
addThirdPartyRequest(
	int id)
{
	int i;
	int added;
	media_t media;
	FileInfo_t *file;
	ThirdPartyInfo_t *data;
	ThirdPartyInfo_t *mig;

	file = GetFile(id);
	ASSERT(file != NULL);
	if (file == NULL)
		return;

	media = file->ar[file->copy].media;

	Trace(TR_MISC, "Add third party stage request inode: %d.%d media: '%s'",
	    file->id.ino, file->id.gen, sam_mediatoa(media));

	/*
	 * Find migration toolkit library.
	 */
	mig = NULL;
	if (toolkitList.data != NULL) {
		for (i = 0; i < toolkitList.entries; i++) {
			data = &toolkitList.data[i];
			if (data != NULL && data->equ_type == media) {
				mig = data;
				break;
			}
		}
	}

	if (mig == NULL) {
		StageError(file, EIO);
		SetStageDone(file);
	} else {
		if (mig->stream == NULL) {
			mig->stream = CreateStream(file);
			AddWork(mig->stream);
		}
		added = AddStream(mig->stream, id, ADD_STREAM_NOSORT);
		if (added == 0) {
			Trace(TR_MISC, "Add stream failed "
			    "inode: %d.%d media: '%s'",
			    file->id.ino, file->id.gen, sam_mediatoa(media));
		}
	}
}

static void *
thirdPartyStage(
	void *arg)
{
	FileInfo_t *file;
	StreamInfo_t *stream;
	ThirdPartyInfo_t *mig;
	int idx = *(int *)arg;

	mig = &toolkitList.data[idx];
	if (idx < 0 || mig == NULL) {
		Trace(TR_ERR, "Invalid argument to start third party thread");
		return (NULL);
	}

	Trace(TR_MISC, "[t@%d] Third party thread started media: '%s' '%s'",
	    pthread_self(), sam_mediatoa(mig->dev->equ_type),
	    mig->dev->name);

	PthreadMutexLock(&create_thread.mutex);
	create_thread.running = B_TRUE;
	PthreadMutexUnlock(&create_thread.mutex);
	PthreadCondSignal(&create_thread.cond);

	while (infiniteLoop) {
		(void) sleep(5);

		PthreadMutexLock(&toolkitList.mutex);
		mig = &toolkitList.data[idx];
		PthreadMutexUnlock(&toolkitList.mutex);

		if (mig->stream == NULL) continue;

		stream = mig->stream;

		PthreadMutexLock(&stream->mutex);

		stream->priority = findResources(stream);
		if (stream->priority != SP_start) {
			PthreadMutexUnlock(&stream->mutex);
			continue;
		}

		while (stream->first > EOS &&
		    (GET_FLAG(stream->flags, SR_ERROR) == 0) &&
		    (GET_FLAG(stream->flags, SR_CLEAR) == 0)) {

			file = GetFile(stream->first);

			PthreadMutexLock(&file->mutex);
			PthreadMutexUnlock(&stream->mutex);

			SET_FLAG(file->flags, FI_ACTIVE);

			stageFile(mig, file);

			PthreadMutexUnlock(&file->mutex);

			/*
			 * Remove file from stream before marking it as done.
			 */
			PthreadMutexLock(&stream->mutex);
			stream->first = file->next;

			file->next = -1;
			stream->count--;

			if (stream->first == EOS) {
				stream->last = EOS;
			}
		}
		PthreadMutexUnlock(&stream->mutex);
	}
	return (NULL);
}

/*
 * Find resources for specified migration stream.
 */
static enum StreamPriority
findResources(
	/* LINTED argument unused in function */
	StreamInfo_t *stream)
{
	enum StreamPriority priority = SP_noresources;

	/*
	 * Shared memory segment must be available and attached.
	 */
	if (shm_ptr_tbl == NULL) {
		(void) ShmatSamfs(O_RDONLY);
	}
	if (shm_ptr_tbl != NULL) {
		priority = SP_start;
	}
	return (priority);
}

/*
 * Remove request from stream.  The request is in the stream
 * or on the mig code's stage list.
 */
void
CancelThirdPartyRequest(
	int id)
{
	int i;
	int copy;
	media_t media;
	FileInfo_t *file;
	ThirdPartyInfo_t *data;
	ThirdPartyInfo_t *mig;

	file = GetFile(id);
	ASSERT(file != NULL);
	if (file == NULL)
		return;

	copy = file->copy;
	media = file->ar[copy].media;

	Trace(TR_DEBUG, "Request to cancel third party stage inode: %d.%d"
	    " fseq: %d media: '%s'",
	    file->id.ino, file->id.gen, file->fseq, sam_mediatoa(media));

	/*
	 * Find migration toolkit library.
	 */
	mig = NULL;
	if (toolkitList.data != NULL) {
		for (i = 0; i < toolkitList.entries; i++) {
			data = &toolkitList.data[i];
			if (data != NULL && data->equ_type == media) {
				mig = data;
				break;
			}
		}
	}

	if (mig == NULL) {
		Trace(TR_MISC,
		    "Migration toolkit library not initialized media: '%s'",
		    sam_mediatoa(media));
	} else {
		if (mig->stream == NULL) {
			Trace(TR_MISC,
			    "Migration stream not initialized media: '%s'",
			    sam_mediatoa(media));
		} else {
			cancelThirdPartyStream(mig, id);
		}
	}
}

static void
cancelThirdPartyStream(
	ThirdPartyInfo_t *mig,
	int id)
{
	FileInfo_t *curr;
	int curr_id;
	boolean_t done = B_FALSE;
	boolean_t found = B_FALSE;

	StreamInfo_t *stream = mig->stream;
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
			if (GET_FLAG(file->flags, FI_ACTIVE)) {
				tp_stage_t *migreq;

				migreq = (tp_stage_t *)file->migfile;
				if (migreq != NULL) {
					if (mig->mig_cancel_stage_req(
					    migreq) == 0) {
						done = B_TRUE;
						file->migfile = 0;
						free(migreq);
					}
				}
			} else {
				if (prev != NULL) {
					prev->next = file->next;
				} else {
					stream->first = file->next;
				}
				SET_FLAG(file->flags, FI_CANCEL);
				StageError(file, ECANCELED);
				stream->count--;
				if (stream->first == EOS) {
					stream->last = EOS;
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
	if (found == B_FALSE) {
		FileInfo_t *nfile = GetFile(id);

		/*
		 * Requested file is no longer in the stream but may on the
		 * migration request list held by the migration code.
		 */
		Trace(TR_DEBUG, "Third party request "
		    "inode: %d.%d not found in stream: '%s.%s'",
		    file->id.ino, file->id.gen, sam_mediatoa(stream->media),
		    stream->vsn);
		/*
		 * Check file is still on the request list, it may removed
		 * while searching a stream.
		 */
		if (nfile->id.ino == file->id.ino &&
		    nfile->id.gen == file->id.gen &&
		    nfile->fseq == file->fseq) {
			tp_stage_t *migreq;

			migreq = (tp_stage_t *)file->migfile;
			if (migreq != NULL) {
				if (mig->mig_cancel_stage_req(migreq) == 0) {
					file->migfile = 0;
					free(migreq);
					SetStageDone(file);
				}
			}
		}
	}
}
