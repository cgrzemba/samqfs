/*
 * schedule.c - schedule stage requests on available resources
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

#pragma ident "$Revision: 1.97 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* POSIX headers. */
#include <sys/types.h>
#include <sys/param.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/syslog.h>
#include <pthread.h>

/* Solaris headers. */

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/shm.h"
#include "sam/lib.h"
#include "pub/rminfo.h"
#include "sam/fioctl.h"
#include "sam/nl_samfs.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "aml/fsd.h"
#include "sam/custmsg.h"
#include "sam/exit.h"
#include "aml/stager.h"
#include "aml/stager_defs.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Local headers. */
#include "stager_lib.h"
#include "stager_config.h"
#include "stager_shared.h"
#include "stager_threads.h"
#include "copy_defs.h"
#include "file_defs.h"
#include "rmedia.h"
#include "stream.h"

#include "stager.h"
#include "schedule.h"
#include "thirdparty.h"

static upath_t fullpath;
static boolean_t infiniteLoop = B_TRUE;

static void scheduleWork();
static void startWork(boolean_t copy_assigned);
static int insertWork(int id);
static void checkWork();
static void requeueWork(pid_t pid);
static void updateWorkState();
static void updateActiveState(
	StagerStateInfo_t *update, StreamInfo_t *stream, int j);
static void updateStreamState(
	StagerStateInfo_t *update, StreamInfo_t *stream, int i);
static boolean_t ifWorkAvail();
static void checkCopies();
static int recoverCopyInstanceList(int numDrives);

static enum StreamPriority findResources(StreamInfo_t *stream);

static int startCopy(StreamInfo_t *stream);
static CopyInstanceInfo_t *findCopy(int library, media_t media);
static CopyInstanceInfo_t *findCopyByPid(pid_t pid);
static boolean_t isCopyIdle(VsnInfo_t *vi);
static boolean_t isCopyBusy(VsnInfo_t *vi, u_longlong_t position);
static boolean_t isDriveAvailForCopy(int library);
static void initCopyInstanceList(boolean_t recover);
static void startCopyProcess(CopyInstanceInfo_t *instance);
/* LINTED static unused */
static void waitCopyProcesses();
static boolean_t sendLoadNotify(StreamInfo_t *stream, equ_t fseq);
static void *loadExportedVol(void *arg);
static void cancelLoadExpVol(StreamInfo_t *stream);
static void setLdCancelSig();
static void catchLdCancelSig(int sig);
static void restoreCopyProc(CopyInstanceInfo_t *copy);
static boolean_t isStagingSuspended(media_t	media);

/*
 * Work queue.
 * Structure representing a singly-linked list of
 * stage stream requests.  A stage stream represents a
 * group of files to be staged together.
 */
static struct {
	size_t		entries;		/* number of entries in list */
	StreamInfo_t	*first;			/* first stream in list */
	StreamInfo_t	*last;			/* last stream in list */
} workQueue = { 0, NULL, NULL };

CopyInstanceList_t *CopyInstanceList = NULL;
extern StageReqs_t StageReqs;
extern int Seqnum;
extern int ShutdownStager;

SchedComm_t SchedComm;		/* communication to scheduling thread */

int OrphanProcs = 0;
/*
 * Send stage request to scheduler thread.  The SchedComm variable is used
 * for communication to the scheduler.
 */
int
SendToScheduler(
	int id)
{
	SchedRequest_t *request;

	PthreadMutexLock(&SchedComm.mutex);

	SamMalloc(request, sizeof (SchedRequest_t));

	request->id = id;
	request->next = NULL;

	/*
	 * Send new data.
	 */
	if (SchedComm.first == NULL) {

		SchedComm.first = request;
		SchedComm.last = request;

	} else {

		(SchedComm.last)->next = request;
		SchedComm.last = request;
	}

	PthreadCondSignal(&SchedComm.avail);

	PthreadMutexUnlock(&SchedComm.mutex);

	return (0);
}

/*
 * Stager's scheduling thread.  This thread is responsible for
 * scheduling stage file requests on available drives.
 */
void*
Scheduler(
	void *arg)
{
	SchedComm_t *comm;
	SchedRequest_t *request;
	FileInfo_t *file;
	struct timespec timeout;
	int rval;
	int id;
	extern StagerStateInfo_t *State;
	int copy;

	comm = (SchedComm_t *)arg;
	ASSERT(comm != NULL);

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
			id = request->id;
			file = GetFile(id);

			ASSERT(file != NULL);
			if (file == NULL) continue;
			copy = file->copy;

			/*
			 * If third party media send request to migrator
			 * to handle.  This decision could be made sooner
			 * but trying to minimize impact on the normal path.
			 */
			if (is_third_party(file->ar[copy].media)) {
				if (FoundThirdParty()) {
					(void) SendToMigrator(id);
				} else {
					StageError(file, ENODEV);
					SetStageDone(file);
				}
			} else {

				/*
				 * Check if new request can be added to an
				 * existing stream.  If not, add it to the
				 * composition list.  Before adding request
				 * Before adding request check if cancel has
				 * been seen and not closing pending file
				 * descriptors. If canceled and not closing
				 * pending file descriptors, mark as done
				 * and don't add to stream or composition list.
				 */
				if (GET_FLAG(file->flags, FI_CANCEL) == 0 ||
				    GET_FLAG(file->flags, FI_DCACHE_CLOSE)) {
					if (insertWork(id) == 0) {
						AddCompose(id);
					}
				} else {
					/*
					 * Cancel seen.  Mark request as done
					 * so its removed from the stager's
					 * request list.
					 */
					SET_FLAG(file->flags, FI_DONE);
				}
			}

			comm->first = request->next;

			SamFree(request);

			if (comm->first == NULL) {
				comm->last = NULL;
			}
		}

		PthreadMutexUnlock(&comm->mutex);

		if (OrphanProcs != 0) {
			checkCopies();
		}

		if (ifWorkAvail()) {
			/*
			 * If drives available, start scheduling requests.
			 */
			if (IfDrivesAvail()) {

				/*
				 * Compose stage file requests into streams.
				 * Add streams to work queue.
				 */
				Compose();

				/*
				 * Schedule stage streams from work queue.
				 */
				scheduleWork();

				/*
				 * Start stage streams from work queue.
				 * Start a stream with a file dcache opened
				 * first, then others.
				 */
				startWork(B_TRUE);

				startWork(B_FALSE);

			} else {

				/*
				 * Post 'Waiting for resources' operator
				 * message.
				 */
				PostOprMsg(State->errmsg, 19211);
			}
		}

		/*
		 * Update memory mapped file with current state of
		 * staging.  The data is used for external display.
		 */
		updateWorkState();

		/*
		 * Check if any streams are done staging and can
		 * be removed from the work queue.  Staging errors
		 * are handled on a per file basis in the main thread.
		 */
		checkWork();

	}
	return (NULL);
}

/*
 * Responsible for the scheduling of stage streams off
 * work queue.
 */
static void
scheduleWork(void)
{
	StreamInfo_t *stream;
	int i;

	/*
	 * TODO
	 * Sort work queue based on staging priority.
	 * Ordering based on stage priority of all files in stream.
	 */

	/*
	 * Make an assignment of VSNs so a scheduling priority
	 * can be determined.
	 */
	stream = workQueue.first;
	for (i = 0; i < workQueue.entries; i++) {

		if ((GET_FLAG(stream->flags, SR_ACTIVE) == 0) &&
		    (GET_FLAG(stream->flags, SR_WAIT) == 0) &&
		    (GET_FLAG(stream->flags, SR_WAITDONE) == 0) &&
		    (GET_FLAG(stream->flags, SR_CLEAR) == 0) &&
		    (stream->thirdparty == B_FALSE)) {

			PthreadMutexLock(&stream->mutex);
			stream->priority = findResources(stream);
			PthreadMutexUnlock(&stream->mutex);
		}
		stream = stream->next;
	}

	/*
	 * TODO
	 * Sort work queue based on scheduling priority.
	 */

}

/*
 * Give streams to copy procs.
 */
static void
startWork(
	boolean_t copy_assigned)
{
	StreamInfo_t *stream;
	int i;

	if (ShutdownStager != 0) {
		return;
	}

	stream = workQueue.first;
	for (i = 0; i < workQueue.entries; i++) {

		if (((copy_assigned == B_TRUE) && (stream->context == NULL)) ||
		    ((copy_assigned == B_FALSE) && (stream->context != NULL))) {
			stream = stream->next;
			continue;
		}

		if ((GET_FLAG(stream->flags, SR_ACTIVE) == 0) &&
		    (stream->thirdparty == B_FALSE)) {

			switch (stream->priority)  {
				case SP_start:
					(void) startCopy(stream);
					break;
			}

		}

		if ((GET_FLAG(stream->flags, SR_ERROR)) &&
		    (stream->thirdparty == B_FALSE)) {

			(void) ErrorStream(stream,
			    stream->error == 0 ? ESRCH : stream->error);
		}

		if ((GET_FLAG(stream->flags, SR_CLEAR)) &&
		    (stream->thirdparty == B_FALSE)) {

			ClearStream(stream);
		}

		if (GET_FLAG(stream->flags, SR_WAITDONE)) {
			if (pthread_join(stream->ldtid, NULL) != 0) {
				WarnSyscallError(HERE,
				    "pthread_join[LoadExportedVol]", "");
			}
			stream->ldtid = 0;
			CLEAR_FLAG(stream->flags, SR_WAITDONE);
		}

		stream = stream->next;
	}
}

/*
 * Add a stream to scheduler's work queue.
 */
void
AddWork(
	StreamInfo_t *stream)
{
	if (workQueue.first == NULL) {
		workQueue.first = workQueue.last = stream;
		stream->next = NULL;
	} else {
		workQueue.last->next = stream;
		workQueue.last = stream;
	}
	workQueue.entries++;
}

/*
 * Cancel pending or active stage request for specified file.
 */
void
CancelWork(
	int id)
{
	int i;
	FileInfo_t *file;
	StreamInfo_t *stream;
	int copy;
	boolean_t found;

	file = GetFile(id);
	copy = file->copy;
	Trace(TR_MISC, "Request to cancel inode: %d.%d (%d)",
	    file->id.ino, file->id.gen, file->fseq);

	stream = workQueue.first;
	for (i = 0; i < workQueue.entries; i++) {
		if (file->ar[copy].media == stream->media &&
		    strcmp(file->ar[copy].section.vsn, stream->vsn) == 0) {

			found = CancelStream(stream, id);
			if (found) {
				break;
			}
		}
		stream = stream->next;
	}
}

/*
 * Stage daemon is exiting.  Send a response for all requests
 * in the work queue.
 */
void
ShutdownWork(void)
{
	StreamInfo_t *stream;
	FileInfo_t *file;
	int i;

	Trace(TR_MISC, "Shutdown work");

	stream = workQueue.first;
	for (i = 0; i < workQueue.entries; i++) {

		PthreadMutexLock(&stream->mutex);
		SET_FLAG(stream->flags, SR_ERROR);
		PthreadMutexUnlock(&stream->mutex);

		while (stream->first > EOS) {
			PthreadMutexLock(&stream->mutex);
			file = GetFile(stream->first);

			PthreadMutexLock(&file->mutex);
			StageError(file, ENODEV);
			stream->first = file->next;
			stream->count--;

			if (stream->first == EOS) {
				stream->last = EOS;
			}
			PthreadMutexUnlock(&stream->mutex);
			PthreadMutexUnlock(&file->mutex);

			SetStageDone(file);
		}

		stream = stream->next;
	}
}

/*
 * Trace work queue.
 */
void
TraceWorkQueue(
	int flag,
	char *srcFile,
	int srcLine)
{
	StreamInfo_t *stream;
	int i;

	_Trace(flag, srcFile, srcLine, "Work queue entries: %d",
	    workQueue.entries);

	stream = workQueue.first;
	for (i = 0; i < workQueue.entries; i++) {
		_Trace(flag, srcFile, srcLine, "%s.%s: stream: 0x%x",
		    sam_mediatoa(stream->media), stream->vsn, (int)stream);
		stream = stream->next;
	}
}

/*
 * Get status of work queue for external display.  Update memory
 * mapped file with current state of staging.  Copy state of
 * active streams in the work queue.
 */
static void
updateWorkState(void)
{
	extern StagerStateInfo_t *State;
	extern boolean_t IdleStager;

	static int last_active_state = 0;
	static boolean_t last_idle_state = B_FALSE;
	StreamInfo_t *stream;
	int i, j, k;
	StagerStateInfo_t update;

	/*
	 * No work and the active state has not changed.
	 */
	if (workQueue.entries == 0 &&
	    last_active_state == 0 &&
	    IdleStager == last_idle_state &&
	    OrphanProcs <= 0) {

		return;
	}
	if (State == NULL)
		return;

	/*
	 * Save current number of work queue entries.
	 */
	last_active_state = workQueue.entries;
	(void) memset(update.active, 0, sizeof (update.active));

	if (workQueue.entries == 0 && OrphanProcs <= 0) {
		if (IdleStager) {
			PostOprMsg(State->errmsg, 4300, ":strun");
		} else {
			ClearOprMsg(State->errmsg);
		}
		memcpy(State->active,  update.active,  sizeof (State->active));
		memcpy(State->streams, update.streams, sizeof (State->streams));
		last_idle_state = IdleStager;
		return;
	}

	(void) memset(update.streams, 0, sizeof (update.streams));
	j = k = 0;
	/*
	 * Update state of orphan copy proc's stream.
	 */
	if (OrphanProcs > 0) {
		struct stat buf;

		for (i = 0; i < CopyInstanceList->cl_entries; i++) {
			CopyInstanceInfo_t *ci;

			ci = &CopyInstanceList->cl_data[i];
			if (j >= STAGER_DISPLAY_ACTIVE) {
				break;
			}
			if (GET_FLAG(ci->ci_flags, CI_orphan) == 0 ||
			    ci->ci_pid == 0) {
				continue;
			}
			sprintf(fullpath, "%s/%s.%d", SharedInfo->si_streamsDir,
			    ci->ci_vsn, ci->ci_seqnum);
			if (stat(fullpath, &buf) < 0) {
				Trace(TR_DEBUG, "Stream %s for orphan copy "
				    "pid: %ld not found.",
				    fullpath, ci->ci_pid);
			} else {
				stream = (StreamInfo_t *)MapInFile(fullpath,
				    O_RDONLY, NULL);

				if (stream != NULL) {
					if (k < STAGER_DISPLAY_STREAMS) {
						updateStreamState(&update,
						    stream, k++);
					}
					if (!(GET_FLAG(stream->flags,
					    (SR_ACTIVE|SR_DONE)) ==
					    (SR_ACTIVE|SR_DONE))) {
						updateActiveState(&update,
						    stream, j++);
					}
					UnMapFile(stream,
					    sizeof (StreamInfo_t));
				}
			}
		}
	}

	/*
	 * Update state of streams in the work queue.
	 */
	if (workQueue.entries > 0) {
		stream = workQueue.first;
		for (i = 0; i < workQueue.entries; i++) {

			if (j >= STAGER_DISPLAY_ACTIVE || stream == NULL) {
				break;
			}
			if (!(GET_FLAG(stream->flags, (SR_ACTIVE|SR_DONE)) ==
			    (SR_ACTIVE|SR_DONE))) {
				updateActiveState(&update, stream, j++);
			}
			stream = stream->next;
		}

		stream = workQueue.first;
		for (i = 0; i < workQueue.entries; i++, k++) {

			if (k >= STAGER_DISPLAY_STREAMS || stream == NULL) {
				break;
			}
			updateStreamState(&update, stream, k);
			stream = stream->next;
		}
	}

	memcpy(State->active,  update.active,  sizeof (State->active));
	memcpy(State->streams, update.streams, sizeof (State->streams));
}

/*
 * Update copy state of active streams.
 */
static void
updateActiveState(
	StagerStateInfo_t *update,
	StreamInfo_t *stream,
	int j)
{
	int  id;
	FileInfo_t *file;
	int msgnum;
	int copy;

	if (GET_FLAG(stream->flags, SR_ACTIVE)) {

		if (GET_FLAG(stream->flags, SR_DONE)) {
			return;
		}

		(void) strcpy(update->active[j].media,
		    sam_mediatoa(stream->vi.media));
		update->active[j].eq = GetEqOrdinal(stream->vi.drive);
		(void) strcpy(update->active[j].vsn, stream->vi.vsn);

		if (GET_FLAG(stream->flags, SR_LOADING)) {
			update->active[j].flags = STAGER_STATE_LOADING;

			/* Post 'Loading VSN' operator message */
			PostOprMsg(update->active[j].oprmsg, 19200,
			    stream->vi.vsn);
		} else {
			id = stream->first;
			if (id == EOS) {
				update->active[j].flags = STAGER_STATE_DONE;

				/* Post 'Unloading VSN' operator message */
				PostOprMsg(update->active[j].oprmsg, 19201,
				    stream->vi.vsn);
			} else {
				file = GetFile(id);
				if (GET_FLAG(file->flags, FI_POSITIONING)) {
					update->active[j].flags =
					    STAGER_STATE_POSITIONING;

					/* 'Positioning for file' message */
					msgnum = 19203;
				} else {
					update->active[j].flags =
					    STAGER_STATE_COPY;

					/* 'Copied for file' message */
					msgnum = 19205;
				}
				copy = file->copy;

				update->active[j].detail.pid = stream->pid;
				update->active[j].detail.id = file->id;
				update->active[j].detail.fseq = file->fseq;
				update->active[j].detail.copy = file->copy;
				update->active[j].detail.len = file->len;

				update->active[j].detail.position =
				    file->ar[copy].section.position;
				update->active[j].detail.offset =
				    file->ar[copy].section.offset;

				GetFileName(file, update->active[j].detail.name,
				    sizeof (update->active[j].detail.name),
				    NULL);

				if (msgnum == 19205) {
					char read[16];
					char len[16];

					/*
					 * Data is read in media allocation
					 * units. This may cause the file's
					 * 'read' value to exceed the file's
					 * length. Adjust for display purposes.
					 */
					if (file->read > file->len) {
						file->read = file->len;
					}
					(void) StrFromFsize(file->read, 1,
					    read, sizeof (read));
					(void) StrFromFsize(file->len,  1,
					    len, sizeof (len));

					PostOprMsg(update->active[j].oprmsg,
					    msgnum, read, len,
					    update->active[j].detail.name);
				} else {
					PostOprMsg(update->active[j].oprmsg,
					    msgnum,
					    update->active[j].detail.name);
				}
			}
		}
	} else if (GET_FLAG(stream->flags, SR_WAIT)) {
		(void) strcpy(update->active[j].media,
		    sam_mediatoa(stream->media));
		(void) strcpy(update->active[j].vsn, stream->vsn);

		update->active[j].flags = STAGER_STATE_WAIT;

		/* Post 'Waiting for VSN to be loaded or imported' message */
		PostOprMsg(update->active[j].oprmsg, 19204, stream->vsn);
	} else {
		(void) strcpy(update->active[j].media,
		    sam_mediatoa(stream->media));
		(void) strcpy(update->active[j].vsn, stream->vsn);

		update->active[j].flags = STAGER_STATE_NORESOURCES;

		/* Post 'Resources not available' message */
		PostOprMsg(update->active[j].oprmsg, 19213, stream->vsn);
	}
}

/*
 * Update stream status.
 */
static void
updateStreamState(
	StagerStateInfo_t *update,
	StreamInfo_t *stream,
	int i)
{
	update->streams[i].seqnum = stream->seqnum;
	update->streams[i].media = stream->media;
	(void) strcpy(update->streams[i].vsn, stream->vsn);
	if (stream->flags & SR_ACTIVE) {
		update->streams[i].active = B_TRUE;
	}

	switch (stream->priority) {
		case SP_noresources:
			PostOprMsg(update->streams[i].oprmsg, 19208);
			break;
		case SP_nofilesys:
			PostOprMsg(update->streams[i].oprmsg, 19209);
			break;
		case SP_busy:
			PostOprMsg(update->streams[i].oprmsg, 19210);
			break;
		case SP_start:
			PostOprMsg(update->streams[i].oprmsg, 19211);
			break;
		default:
			strcpy(update->streams[i].oprmsg, "");
	}
}

/*
 * Check if a new stage file request can be inserted in an
 * existing stream on the work queue.
 */
static int
insertWork(
	int id)
{
	int i;
	FileInfo_t *file;
	StreamInfo_t *stream;
	int added = 0;
	int copy;

	file = GetFile(id);
	copy = file->copy;

	/*
	 * TODO
	 * First pass. Walk over work queue to check if the new request
	 * can be added to an active stream.  We may miss adding request
	 * to an active stream if there is a matching inactive stream
	 * found first.
	 */
	stream = workQueue.first;
	for (i = 0; i < workQueue.entries; i++) {
		if (GET_FLAG(file->flags, FI_DCACHE_CLOSE)) {
			if (GET_FLAG(stream->flags, SR_DCACHE_CLOSE) &&
			    file->context == stream->context) {

				added = AddStream(stream, id,
				    ADD_STREAM_NOSORT);

				if (added)	/* done checking if added */
					break;	/*    to the stream */
			}
		} else if ((strcmp(file->ar[copy].section.vsn,
		    stream->vsn) == 0) &&
		    (file->ar[copy].media == stream->media) &&
		    !(IF_COPY_DISKARCH(file, copy))) {

			added = AddStream(stream, id, ADD_STREAM_SORT);

			if (added)	/* done checking if added */
				break;	/*    to the stream */
		}
		stream = stream->next;
	}
	return (added);
}

/*
 */
static boolean_t
ifWorkAvail(void)
{
	boolean_t avail = B_FALSE;

	if (workQueue.entries > 0 ||
	    GetNumComposeEntries() > 0) {

		avail = B_TRUE;
	}

	return (avail);
}

/*
 * Shutdown all copy processes/threads.  Wait for processes
 * to exit before returning to caller.
 */
void
ShutdownCopy(
	int stopSignal)
{
	int i;
	pid_t pid;

	if (CopyInstanceList == NULL) {
		return;
	}

	Trace(TR_MISC, "ShutdownCopy: sig: %d", stopSignal);

	for (i = 0; i < CopyInstanceList->cl_entries; i++) {
		pid = CopyInstanceList->cl_data[i].ci_pid;
		if (pid != 0) {
			(void) kill(pid, stopSignal);
			Trace(TR_MISC, "Sent signal %d to copy[%d]",
			    stopSignal, (int)pid);
		}
	}

	/*
	 * Shutdown copy process for failover if SIGUSR1.
	 */
	if (stopSignal == SIGUSR1) {
		int procs;

		/*
		 * Wait for copy procs to exit.
		 */
		procs = 1;
		while (procs != 0) {
			procs = 0;
			for (i = 0; i < CopyInstanceList->cl_entries; i++) {
				pid = CopyInstanceList->cl_data[i].ci_pid;

				if (pid != 0) {
					int rv;
					char rmfn[10];
					CopyInstanceInfo_t *ci;

					ci = &CopyInstanceList->cl_data[i];
					sprintf(rmfn, "%d", ci->ci_rmfn);

					rv = FindProc(COPY_PROGRAM_NAME, rmfn);
					if (rv > 0) {
						procs++;
						Trace(TR_DEBUG,
						    "Copy: %d pid: %d busy.",
						    i, (int)ci->ci_pid);
						sleep(1);
						break;
					}
				}
			}
		}
	}
}

/*
 * Remove copy proc file.
 */
void
RemoveCopyProcMapFile(void)
{
	if (CopyInstanceList == NULL) {
		return;
	}
	RemoveMapFile(SharedInfo->si_copyInstancesFile,
	    (void *) CopyInstanceList, CopyInstanceList->cl_size);
	CopyInstanceList = NULL;
}

/*
 * Copy process has exited.
 */
void
CopyProcExit(
	int sig)
{
	pid_t pid;
	int stat;
	CopyInstanceInfo_t *instance;
	int exitstatus;
	int signum;
	boolean_t restart = B_TRUE;

	SetErrno = 0;	/* clear for trace */
	Trace(TR_ERR, "Copy process exit signal: %d", sig);
	if (sig != SIGCLD)
		return;

	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
		exitstatus = WEXITSTATUS(stat);
		signum = WTERMSIG(stat);
		if (exitstatus != 0 ||
		    (signum != 0 && (signum != SIGTERM && signum != SIGINT))) {
			Trace(TR_ERR, "Copy ERR exit "
			    "pid: %d stat: %04x exit: %d sig: %d",
			    (int)pid, stat, exitstatus, signum);
			if (exitstatus == EXIT_NORESTART ||
			    ShutdownStager == SIGUSR1) {
				Trace(TR_ERR, "Copy process not restarted");
				restart = B_FALSE;
			}
		} else {
			Trace(TR_MISC, "Copy normal exit pid: %d stat: %04x",
			    (int)pid, stat);

			/*
			 * The copy process is unable to catch the shutdown
			 * signal, SIGINT, if in syscall to open removable
			 * media.  This prevents the copy process from properly
			 * cleaning up an active stream.  Make sure a stream
			 * in this state is rescheduled here.
			 */
			requeueWork(pid);
		}

		/*
		 * Find copy proc struct for exited child process.
		 * Enable copy struct so process can be restarted.
		 */
		instance = findCopyByPid(pid);

		if (instance != NULL) {
			boolean_t shutdown;

			shutdown = IF_SHUTDOWN(instance);
			/*
			 * The scheduler thread may be accessing this copy's
			 * instance. Lock it before changing the state.
			 */
			PthreadMutexLock(&instance->ci_requestMutex);
			if (restart) {
				instance->ci_created = B_FALSE;
				instance->ci_busy = B_FALSE;
				instance->ci_pid = 0;
				CLEAR_FLAG(instance->ci_flags,
				    (CI_shutdown | CI_failover));
				instance->ci_first = instance->ci_last = NULL;
			} else {
				instance->ci_busy = B_TRUE;
			}
			PthreadMutexUnlock(&instance->ci_requestMutex);

			if (shutdown == B_FALSE) {
				/*
				 * Copy proc abnormally terminated.  Find
				 * streams in work queue assigned to terminated
				 *  process and requeue them.
				 */
				requeueWork(pid);
				restoreCopyProc(instance);
			}
		}
	}
}

/*
 * Responsible for the checking if any stage streams have completed
 * and removing them from the work queue.  Errors are handling at
 * the file level.
 */
static void
checkWork(void)
{
	StreamInfo_t *stream;
	StreamInfo_t *next;
	StreamInfo_t *prev;

	prev = NULL;
	stream = workQueue.first;
	while (stream != NULL) {

		if (GET_FLAG(stream->flags, SR_ACTIVE) &&
		    GET_FLAG(stream->flags, SR_DONE)) {

			/*
			 * If load exported volume is pending, cancel load.
			 */
			if (GET_FLAG(stream->flags, SR_WAIT)) {
				cancelLoadExpVol(stream);
			}

			workQueue.entries--;
			if (prev == NULL) {
				workQueue.first = stream->next;
				if (workQueue.first == NULL)
					workQueue.last = NULL;
			} else {
				prev->next = stream->next;
			}
			next = stream->next;

			DeleteStream(stream);

			stream = next;

		}

		/*
		 * Check to make sure we didn't just remove
		 * the last entry in the work queue.
		 */
		if (stream) {
			prev = stream;
			stream = stream->next;
		} else {
			workQueue.last = prev;
		}
	}
}

/*
 * A copy process terminated abnormally.  Find streams which were scheduled
 * to run on the terminated process and requeue them.
 */
static void
requeueWork(
	pid_t pid)
{
	int i;
	StreamInfo_t  *stream;

	stream = workQueue.first;
	for (i = 0; i < workQueue.entries; i++) {

		if (stream->pid == pid) {
			/*
			 * Found stream which was active in terminated process.
			 * If not already done, requeue stream.
			 */
			if (GET_FLAG(stream->flags, SR_DONE) == 0) {
				int id;
				FileInfo_t *prev = NULL;

				id = stream->first;
				while (id > EOS) {
					FileInfo_t *file;
					boolean_t removed = B_FALSE;

					PthreadMutexLock(&stream->mutex);
					file = GetFile(id);

					PthreadMutexLock(&file->mutex);
					if (prev != NULL) {
						PthreadMutexLock(&prev->mutex);
					}

					/*
					 * If file descritor opened
					 * remove a request.
					 */
					if (GET_FLAG(file->flags, FI_DCACHE)) {
						file->dcache = 0;
						file->error = ECANCELED;
						file->context = 0;
						SET_FLAG(file->flags,
						    FI_NO_RETRY);
						CLEAR_FLAG(file->flags,
						    FI_DCACHE);
						if (prev != NULL) {
							prev->next =
							    file->next;
						} else {
							stream->first =
							    file->next;
						}
						stream->count--;
						if (stream->first == EOS) {
							stream->last = EOS;
							/*
							 * Set flags so stream
							 * is deleted.
							 */
							SET_FLAG(stream->flags,
							    SR_ACTIVE);
							SET_FLAG(stream->flags,
							    SR_DONE);
						} else if (file->sort ==
						    stream->last) {
							ASSERT(prev != NULL);
							stream->last =
							    prev->sort;
						}
						removed = B_TRUE;
					}
					if (prev != NULL) {
						PthreadMutexUnlock(
						    &prev->mutex);
					}
					PthreadMutexUnlock(&file->mutex);
					PthreadMutexUnlock(&stream->mutex);

					id = file->next;
					if (removed) {
						SetStageDone(file);
					} else {
						prev = file;
					}
				}
				if (GET_FLAG(stream->flags, SR_DONE) == 0) {
					Trace(TR_MISC, "Requeue stream: "
					    "'%s.%d' 0x%x",
					    stream->vsn, stream->seqnum,
					    (int)stream);
					stream->flags = 0;
					stream->pid = 0;
					stream->context = NULL;
					stream->priority = SP_start;
				}
			}
		}
		stream = stream->next;
	}
}

/*
 * Find resources and set priority for specified stream.
 * CAREFUL.  Stream is locked on entry.
 */
static enum StreamPriority
findResources(
	StreamInfo_t *stream)
{
	FileInfo_t *file;
	int copy;
	boolean_t attended;
	VsnInfo_t *vi;
	enum StreamPriority priority = SP_noresources;

	if (stream->first == EOS) {
		return (priority);
	}

	if (GET_FLAG(stream->flags, SR_DCACHE_CLOSE)) {
		return (SP_start);
	}

	/*
	 * Find VSN info for stage file's VSN.  The
	 * first file's VSN is used since its the same as all
	 * other files in the stream.
	 */
	file = GetFile(stream->first);
	copy = file->copy;

	if (IsFileSystemMounted(file->fseq) == FALSE) {
		return (SP_nofilesys);
	}

	if (isStagingSuspended(file->ar[copy].media)) {
		return (SP_idle);
	}

	vi = FindVsn(file->ar[copy].section.vsn, file->ar[copy].media);

	if (vi == NULL || IsLibraryOff(vi->lib)) {
		if (vi == NULL) {
			priority = SP_noresources;

			/*
			 * Check availability of catalog.  If the catalog is
			 * available and volume is not found, issue an error
			 * for this request.
			 */
			if (IsCatalogAvail()) {
				/*
				 * Volume not found.
				 */
				if (GET_FLAG(file->flags, FI_DCACHE)) {
					/*
					 * Found opened file.  An error
					 * response for the file must be
					 * generated in the associated copy
					 * process. Set media unavailable in
					 * stream for copy.
					 */
					SET_FLAG(stream->flags, SR_UNAVAIL);
					priority = SP_start;
				} else {
					stream->error = ENODEV;
					SET_FLAG(stream->flags, SR_ERROR);
				}
				SendCustMsg(HERE, 19020,
				    sam_mediatoa(file->ar[copy].media),
				    file->ar[copy].section.vsn);
			}
		} else {
			/*
			 * Library not available.
			 */
			stream->error = ENODEV;
			SET_FLAG(stream->flags, SR_ERROR);

			SendCustMsg(HERE, 19022,
			    sam_mediatoa(file->ar[copy].media),
			    file->ar[copy].section.vsn);
		}
		return (priority);

	} else {
		memcpy(&stream->vi, vi, sizeof (stream->vi));
		stream->lib = vi->lib;
	}

	/*
	 * Check availability of media.
	 */
	if (IsVsnAvail(&stream->vi, &attended) == B_FALSE) {
		priority = SP_noresources;

		if (attended == B_FALSE) {
			Trace(TR_ERR, "Volume '%s.%s' not available",
			    sam_mediatoa(file->ar[copy].media),
			    file->ar[copy].section.vsn);

			if (GET_FLAG(file->flags, FI_DCACHE)) {
				/*
				 * Found opened file.  An error response for the
				 * for the file must be generated in the
				 * associated copy process. Set media
				 * unavailable in stream for copy.
				 */
				SET_FLAG(stream->flags, SR_UNAVAIL);
				priority = SP_start;
			} else {
				PthreadMutexUnlock(&stream->mutex);
				ErrorStream(stream, ENODEV);
				PthreadMutexLock(&stream->mutex);
			}

		} else {
			if (GET_FLAG(stream->flags, SR_WAIT) == 0) {
				if (sendLoadNotify(stream, file->fseq)) {
					SET_FLAG(stream->flags, SR_WAIT);
				}
			}
		}
		return (priority);
	}

	/*
	 * Check bad media.
	 */
	if (IS_VSN_BADMEDIA(vi)) {
		priority = SP_noresources;

		SetErrno = ENOSPC;	/* Set errno for trace */
		Trace(TR_ERR, "Volume '%s.%s' is bad media",
		    sam_mediatoa(file->ar[copy].media),
		    file->ar[copy].section.vsn);

		if (GET_FLAG(file->flags, FI_DCACHE)) {
			SET_FLAG(stream->flags, SR_UNAVAIL);
			priority = SP_start;
		} else {
			PthreadMutexUnlock(&stream->mutex);
			ErrorStream(stream, ENOSPC);
			PthreadMutexLock(&stream->mutex);
		}
		return (priority);
	}

	CLEAR_FLAG(stream->flags, SR_WAIT);
	ASSERT(strcmp(stream->vi.vsn, stream->vsn) == 0);

	/*
	 * Check availability of stager's copy resources.
	 * Set resource to busy if copy already working on this VSN or
	 * if there are no idle copy available for library.
	 */
	if (isCopyBusy(&stream->vi, file->ar[copy].section.position)) {
		priority = SP_busy;

	} else if (isCopyIdle(&stream->vi) == B_FALSE) {
		priority = SP_noresources;

	}

	/*
	 * FindVsn call above checked if requested VSN is loaded.  If so,
	 * this is a good candidate to schedule now.
	 */
	if (priority != SP_busy) {

		if (IS_VSN_LOADED(vi)) {
			priority = SP_start;

		} else {

			/*
			 * Check if drives are available in the library
			 * holding the VSN.
			 */
			if (GetNumLibraryDrives(stream->vi.lib) > 0)
				priority = SP_start;

			/*
			 * Check if there's a drive free to use.
			 */
			if (IsLibraryDriveFree(stream->vi.lib) == FALSE) {
				priority = SP_noresources;
			}
		}
	}
	return (priority);
}

/*
 * Read to copy, send stream to copy proc.
 */
static int
startCopy(
	StreamInfo_t *stream)
{
	extern char *WorkDir;
	int ret;
	VsnInfo_t *vi;
	FileInfo_t *file;
	CopyRequestInfo_t *request;
	pthread_condattr_t cattr;
	CopyInstanceInfo_t *instance = NULL;

	vi = &stream->vi;
	file = GetFile(stream->first);		/* first file in stream */
	ASSERT(file != NULL);

	/*
	 * If no drive available for copy, set stream priority and return.
	 */
	if (isDriveAvailForCopy(vi->lib) != B_TRUE) {
		stream->priority = SP_busy;
		return (0);
	}

	/*
	 * If disk cache already open, ie. multivolume or error retry,
	 * find the copy process with the already opened file descriptor.
	 */
	if (stream->context != 0) {
		instance = findCopyByPid(stream->context);
		if (instance == NULL &&
		    GET_FLAG(stream->flags, SR_DCACHE_CLOSE)) {
			/*
			 * Copy process has exited, no need to close file
			 * descriptors but clear the stream.
			 */
			Trace(TR_MISC, "Find copy failed "
			    "pid: %d stream '%s.%d'",
			    (int)stream->context, stream->vsn, stream->seqnum);
			SET_FLAG(stream->flags, SR_CLEAR);
			return (0);
		}
		/*
		 * Make sure copy process is not already busy.
		 */
		if (instance != NULL && instance->ci_busy == B_TRUE) {
			Trace(TR_MISC, "Copy process %d busy",
			    (int)stream->context);
			instance = NULL;
		}
		if (instance == NULL) {
			Trace(TR_MISC, "Find copy failed "
			    "pid: %d stream '%s.%d'",
			    (int)stream->context, stream->vsn, stream->seqnum);
		}

	} else {
		instance = findCopy(vi->lib, vi->media);
	}

	/*
	 * Unable to find a copy thread.  This means multiple
	 * streams could be scheduled on the thread and this stream lost
	 * the race.  Set stream priority and return.
	 */
	if (instance == NULL) {
		stream->priority = SP_busy;
		return (0);
	}

	Trace(TR_MISC, "Found copy-rm%d instance: 0x%x media: '%s'",
	    instance->ci_rmfn, (int)instance, sam_mediatoa(instance->ci_media));

	PthreadMutexLock(&instance->ci_requestMutex);

	/*
	 * Copy process timed out.  Reschedule this request.
	 */
	if (IF_SHUTDOWN(instance) || instance->ci_created == 0) {
		stream->priority = SP_start;
		Trace(TR_MISC, "\ttimed out");
		PthreadMutexUnlock(&instance->ci_requestMutex);
		return (0);
	}

	/*
	 * Create and initialize a request structure.
	 */
	SamMalloc(request, sizeof (CopyRequestInfo_t));
	memset(request, 0, sizeof (CopyRequestInfo_t));

	request->cr_next = NULL;
	request->cr_gotItFlag = B_FALSE;

	PthreadCondattrInit(&cattr);
	PthreadCondattrSetpshared(&cattr, PTHREAD_PROCESS_SHARED);
	PthreadCondInit(&request->cr_gotIt, &cattr);
	PthreadCondattrDestroy(&cattr);

	(void) sprintf(fullpath, "%s/rm%d/%s", WorkDir,
	    instance->ci_rmfn, COPY_FILE_FILENAME);
	ret = WriteMapFile(fullpath, (void *)request,
	    sizeof (CopyRequestInfo_t));
	if (ret != 0) {
		upath_t dirpath;

		/*
		 * Make a directory if it doesn't already exist.
		 */
		(void) sprintf(dirpath, "%s/rm%d", WorkDir, instance->ci_rmfn);
		MakeDirectory(dirpath);

		ret = WriteMapFile(fullpath, (void *)request,
		    sizeof (CopyRequestInfo_t));
		if (ret != 0) {
			FatalSyscallError(EXIT_NORESTART, HERE,
			    "WriteMapFile", fullpath);
		}
	}
	SamFree(request);
	request = (CopyRequestInfo_t *)MapInFile(fullpath, O_RDWR, NULL);
	if (request == NULL) {
		Trace(TR_ERR, "Cannot map in file %s", fullpath);
		stream->priority = SP_busy;
		return (0);
	}

	Trace(TR_MISC, "Schedule copy-rm%d: '%s.%d' 0x%x",
	    instance->ci_rmfn, stream->vsn, stream->seqnum, (int)request);

	if (instance->ci_first == NULL) {
		instance->ci_first = request;
		instance->ci_last = request;
	} else {
		(instance->ci_last)->cr_next = request;
		instance->ci_last = request;
	}

	/*
	 * Set process id of copy proc in stream.  In the case of
	 * abnormal copy proc termination this allows us to find streams
	 * that were active.
	 */
	stream->pid = instance->ci_pid;

	/*
	 * Set active flag for this stream.  This prevents the scheduler
	 * from treating this as an inactive stream before the copy thread
	 * has a change to pick it up.
	 */
	PthreadMutexLock(&stream->mutex);
	SET_FLAG(stream->flags, SR_ACTIVE);
	PthreadMutexUnlock(&stream->mutex);

	(void) strncpy(request->cr_vsn, stream->vsn, sizeof (request->cr_vsn));
	request->cr_seqnum = stream->seqnum;

	/*
	 * Tell copy server that a request is available.  This call
	 * unblocks the server thread waiting on the condition variable.
	 */
	PthreadCondSignal(&instance->ci_request);

	/*
	 * Request issued.  Copy server will free request
	 * structure.  Wait here for copy server to pick up request.
	 * Otherwise we may try to schedule another stream on this
	 * copy server since it will look idle.
	 */
	while (request->cr_gotItFlag == B_FALSE) {
		PthreadCondWait(&request->cr_gotIt, &instance->ci_requestMutex);
	}
	PthreadMutexUnlock(&instance->ci_requestMutex);

	/*
	 * Unmap pages of memory.  Request's memory
	 * mapped file is removed in child.
	 */
	UnMapFile(request, sizeof (CopyRequestInfo_t));

	return (0);
}

/*
 * Find copy proc for specified library and media type.
 * If a copy proc has not yet been created it gets started here.
 */
static CopyInstanceInfo_t *
findCopy(
	int library,
	media_t	media)
{
	int i;
	int num_drives;
	int retry;
	int trylock;

	CopyInstanceInfo_t *instance = NULL;

	num_drives = GetNumLibraryDrives(library);
	/*
	 * Attempt to find idle thread for this media type.
	 */
	if (CopyInstanceList == NULL) {
		return (NULL);
	}

	for (i = 0; i < CopyInstanceList->cl_entries; i++) {
		if (CopyInstanceList->cl_data[i].ci_lib == library) {
			num_drives--;
			if (CopyInstanceList->cl_data[i].ci_busy == B_FALSE &&
			    num_drives >= 0) {
				instance = &CopyInstanceList->cl_data[i];
				break;
			}
		}
	}

	/*
	 * If there is no existing thread (instance == NULL), check here
	 * if we can create a new one.  CopyInstanceList has an entry
	 * for all drives allowed (not just available) so also check
	 * to make sure we do not oversubscribe the copy threads and
	 * number of drives available for the library.
	 */
	if (instance == NULL) {
		num_drives = GetNumLibraryDrives(library);
		for (i = 0; i < CopyInstanceList->cl_entries; i++) {
			CopyInstanceInfo_t *ci;

			ci = &CopyInstanceList->cl_data[i];
			if (ci->ci_lib == library) {
				num_drives--;
				if (ci->ci_created == B_FALSE &&
				    num_drives >= 0) {
					instance = ci;
					break;
				}
			}
		}
	}

	/*
	 * If unable to find or create a copy thread.  This means
	 * multiple streams could be scheduled on the thread and this
	 * stream lost the race.  Return NULL instance.
	 */
	if (instance == NULL)
		return (NULL);

	/*
	 * Lock the request mutex.  Use trylock function so if the mutex is
	 * currently locked, we get an immediate return.  This may occur
	 * if a copy process died with the mutext locked.
	 */
	retry = 3;
	trylock = EBUSY;
	while (trylock != 0) {
		trylock = pthread_mutex_trylock(&instance->ci_requestMutex);
		if (trylock != 0) {
			Trace(TR_MISC, "Request mutex locked %d "
			    "instance: 0x%x, errno: %d",
			    trylock, (int)instance, errno);
			if (--retry > 0) {
				sleep(5);
			} else {
				pthread_mutexattr_t mattr;
				pthread_condattr_t cattr;

				/*
				 * Mutex is currently locked but no way to
				 * unlock the mutex.  May need to replace the
				 * pthread_mutex_trylock() with
				 * pthread_mutexattr_setrobust_np().
				 */

				Trace(TR_MISC, "Reinitialize mutex lock "
				    "instance: 0x%x", (int)instance);

				/*
				 * Reinitialize mutex.
				 */
				PthreadMutexattrInit(&mattr);
				PthreadMutexattrSetpshared(&mattr,
				    PTHREAD_PROCESS_SHARED);
				PthreadCondattrInit(&cattr);
				PthreadCondattrSetpshared(&cattr,
				    PTHREAD_PROCESS_SHARED);

				PthreadMutexInit(&instance->ci_requestMutex,
				    &mattr);
				PthreadCondInit(&instance->ci_request, &cattr);
				PthreadMutexInit(&instance->ci_runningMutex,
				    &mattr);
				PthreadCondInit(&instance->ci_running, &cattr);

				PthreadMutexattrDestroy(&mattr);
				PthreadCondattrDestroy(&cattr);

				PthreadMutexLock(&instance->ci_requestMutex);

				break;
			}
		}
	}

	/*
	 * If copy process/thread not created it gets started here.
	 */
	if (instance->ci_created == 0) {
		boolean_t lockbuf;

		instance->ci_media = media;
		instance->ci_numBuffers = GetMediaParamsBufsize(media,
		    &lockbuf);
		instance->ci_lockbuf = lockbuf;

		/*
		 * Start copy process for removable media drive.
		 */
		startCopyProcess(instance);
	}

	PthreadMutexUnlock(&instance->ci_requestMutex);

	return (instance);
}

/*
 * Find an existing copy proc by pid.  This shouldn't happen,
 * but return NULL if copy proc does not exist.
 */
static CopyInstanceInfo_t *
findCopyByPid(
	pid_t pid)
{
	int i;
	CopyInstanceInfo_t *instance = NULL;

	if (CopyInstanceList == NULL) {
		return (instance);
	}

	for (i = 0; i < CopyInstanceList->cl_entries; i++) {
		if (CopyInstanceList->cl_data[i].ci_created &&
		    CopyInstanceList->cl_data[i].ci_pid == pid) {
			instance = &CopyInstanceList->cl_data[i];
			break;
		}
	}
	Trace(TR_MISC, "Find copy: 0x%x by pid: %d", (int)instance, (int)pid);
	return (instance);
}

/*
 * Check if there is an idle copy proc for specified library
 * and media type.
 */
static boolean_t
isCopyIdle(
	VsnInfo_t *vi)
{
	boolean_t avail;
	int i;
	int library;
	int num_avail_drives;

	avail = B_FALSE;

	/*
	 * Attempt to find idle thread for specified library.
	 * The CopyInstanceList structure has an entry for all drives
	 * allowed (not just available) * so also check to make sure not
	 * to oversubscribe the copy threads and number of drives
	 * available for the library.
	 */
	library = vi->lib;
	num_avail_drives = GetNumLibraryDrives(library);

	for (i = 0; i < CopyInstanceList->cl_entries; i++) {
		CopyInstanceInfo_t *ci;

		ci = &CopyInstanceList->cl_data[i];
		if (ci->ci_lib == library) {
			num_avail_drives--;
			if (ci->ci_busy == B_FALSE && num_avail_drives >= 0) {

				avail = B_TRUE;
				break;
			}
		}
	}
	return (avail);
}

/*
 * Check if the copy proc for specified library and media type
 * is busy.
 */
static boolean_t
isCopyBusy(
	VsnInfo_t *vi,
	u_longlong_t position)
{
	boolean_t busy;
	int i;
	int library;

	busy = B_FALSE;
	initCopyInstanceList(B_FALSE);

/*
 * FIXME - detect if trying to schedule flip side of two-sided media
 * and other side is already in use(vi->ele_addr)
 */

	library = vi->lib;
	for (i = 0; i < CopyInstanceList->cl_entries; i++) {
		CopyInstanceInfo_t *ci;

		ci = &CopyInstanceList->cl_data[i];
		if (ci->ci_busy == B_TRUE &&
		    ci->ci_lib == library &&
		    strcmp(ci->ci_vsn, vi->vsn) == 0 &&
		    ci->ci_dkPosition == position) {

			busy = B_TRUE;
			break;
		}
	}
	return (busy);
}

/*
 * Check if drive for specified library and media type is available
 * for copy proc. Copy proc may have a different type of media when
 * retrying from the next available copy.
 */
static boolean_t
isDriveAvailForCopy(
	int library)
{
	int i;
	int num_drives;

	num_drives = GetNumLibraryDrives(library);
	for (i = 0; i < CopyInstanceList->cl_entries; i++) {
		if (CopyInstanceList->cl_data[i].ci_vsnLib == library &&
		    CopyInstanceList->cl_data[i].ci_busy == B_FALSE &&
		    CopyInstanceList->cl_data[i].ci_created == B_TRUE) {
			if (--num_drives <= 0) {
				return (B_FALSE);
			}
		}
	}
	return (B_TRUE);
}

/*
 * Initialize copy proc list.  The CopyInstanceList has an entry for all
 * drives allowed (not just available).
 */
static void
initCopyInstanceList(
	boolean_t recover)
{
	struct sigaction sig_action;
	int i, j;
	size_t size;
	int numLibraries;
	LibraryInfo_t *libraries;
	LibraryInfo_t *entry;
	int numDrives = 0;
	int ret;
	pthread_mutexattr_t mattr;
	pthread_condattr_t cattr;

	if (CopyInstanceList != NULL) {
		return;
	}
	numDrives = GetNumAllDrives();

	if (numDrives == 0) {
		return;
	}

	if (recover == B_TRUE) {
		if (recoverCopyInstanceList(numDrives) < 0) {
			return;
		} else {
			ASSERT(CopyInstanceList != NULL);
			SharedInfo->si_numCopyInstanceInfos = numDrives;
			Trace(TR_MISC, "Recovered %d copy procs", numDrives);

			memset(&sig_action, 0, sizeof (sig_action));
			sig_action.sa_handler = CopyProcExit;
			sigemptyset(&sig_action.sa_mask);
			sig_action.sa_flags = SA_RESTART;
			(void) sigaction(SIGCLD, &sig_action, NULL);
			return;
		}
	}

	Trace(TR_MISC, "Initialize %d copy procs", numDrives);

	size = sizeof (CopyInstanceList_t);
	size += (numDrives - 1) * sizeof (CopyInstanceInfo_t);
	SamMalloc(CopyInstanceList, size);
	(void) memset(CopyInstanceList, 0, size);

	CopyInstanceList->cl_magic	= COPY_INSTANCE_LIST_MAGIC;
	CopyInstanceList->cl_version    = COPY_INSTANCE_LIST_VERSION;
	CopyInstanceList->cl_create	= StageReqs.val->create;
	CopyInstanceList->cl_entries	= numDrives;
	CopyInstanceList->cl_size	= size;
	SharedInfo->si_numCopyInstanceInfos = numDrives;

	PthreadMutexattrInit(&mattr);
	PthreadMutexattrSetpshared(&mattr, PTHREAD_PROCESS_SHARED);
	PthreadCondattrInit(&cattr);
	PthreadCondattrSetpshared(&cattr, PTHREAD_PROCESS_SHARED);

	/*
	 * Assign copy proc to each allowed drive in all
	 * libraries.
	 */
	j = 0;
	libraries = GetLibraries(&numLibraries);
	for (i = 0; i < numLibraries; i++) {
		boolean_t is_remote = FALSE;

		entry = &libraries[i];
		if (IS_LIB_HISTORIAN(entry) || IS_LIB_THIRDPARTY(entry)) {
			continue;
		}

		numDrives = entry->li_numAllowedDrives;

		/*
		 * Mark each copy proc if accessing a remote library.
		 */
		if (IS_LIB_REMOTE(entry)) {
			is_remote = TRUE;
		}
		while (numDrives-- > 0) {
			CopyInstanceInfo_t *ci;

			ci = &CopyInstanceList->cl_data[j];

			PthreadMutexInit(&ci->ci_requestMutex, &mattr);
			PthreadCondInit(&ci->ci_request, &cattr);

			PthreadMutexInit(&ci->ci_runningMutex, &mattr);
			PthreadCondInit(&ci->ci_running, &cattr);

			ci->ci_lib = i;
			ci->ci_eq = entry->li_eq;
			ci->ci_busy = B_TRUE;

			ci->ci_rmfn = j;
			ci->ci_drive = GetEqOrdinal(j);

			/*
			 * Set flag if drive associated with this copy
			 * process resides on a remote host.
			 */
			if (is_remote) {
				SET_FLAG(ci->ci_flags, CI_samremote);
			}
			j++;
		}
	}

	ret = WriteMapFile(SharedInfo->si_copyInstancesFile,
	    (void *)CopyInstanceList, size);
	if (ret != 0) {
		FatalSyscallError(EXIT_NORESTART, HERE, "WriteMapFile",
		    SharedInfo->si_copyInstancesFile);
	}
	SamFree(CopyInstanceList);
	CopyInstanceList = (CopyInstanceList_t *)
	    MapInFile(SharedInfo->si_copyInstancesFile, O_RDWR, NULL);

	PthreadMutexattrDestroy(&mattr);
	PthreadCondattrDestroy(&cattr);

	memset(&sig_action, 0, sizeof (sig_action));
	sig_action.sa_handler = CopyProcExit;
	sigemptyset(&sig_action.sa_mask);
	sig_action.sa_flags = SA_RESTART;
	(void) sigaction(SIGCLD, &sig_action, NULL);
}

/*
 * Start a copy process for removable media file.
 */
static void
startCopyProcess(
	CopyInstanceInfo_t *instance)
{
	extern char *CopyExecPath;
	pid_t pid;
	int i;

	Trace(TR_MISC, "Starting copy-rm%d media: '%s'",
	    instance->ci_rmfn, sam_mediatoa(instance->ci_media));

	/*
	 * Set non-standard files to close on exec.
	 */
	for (i = STDERR_FILENO + 1; i < OPEN_MAX; i++) {
		(void) fcntl(i, F_SETFD, FD_CLOEXEC);
	}

	pid = fork1();
	if (pid < 0) {
		SysError(HERE, "fork1");
		instance->ci_pid = 0;
		instance->ci_created = B_FALSE;
		return;
	}

	if (pid == 0) {			/* child */

		char rm[16];

		sprintf(rm, "%d", instance->ci_rmfn);
		execl(CopyExecPath, COPY_PROGRAM_NAME, rm, NULL);

		Trace(TR_ERR, "Exec '%s' failed %d", CopyExecPath, errno);
		instance->ci_pid = 0;
		instance->ci_created = B_FALSE;
		return;
	}

	/*
	 * Parent.
	 */
	instance->ci_pid = pid;
	instance->ci_created = B_TRUE;
}

/*
 * Part of shutdown, wait for all copy processes to exit.
 */
static void
waitCopyProcesses(void)
{
	int i;

	Trace(TR_MISC, "Waiting for copy procs to shutdown");
	for (i = 0; i < CopyInstanceList->cl_entries; i++) {
		CopyInstanceInfo_t *ci;

		ci = &CopyInstanceList->cl_data[i];
		if (ci->ci_pid != 0) {
			int signum = SIGKILL;

			sleep(2);

			/*
			 * If not yet shutdown, force it.
			 */
			if (ci->ci_pid != 0) {
				(void) kill(ci->ci_pid, signum);
				Trace(TR_MISC, "Sent signal %d to copy[%d]",
				    signum, (int)ci->ci_pid);
			}
		}
	}
}

/*
 * Send load notify message.
 */
static boolean_t
sendLoadNotify(
	StreamInfo_t *stream,
	equ_t fseq)
{
	LoadExported_t *rmload;
	char *mount_name;

	mount_name = GetMountPointName(fseq);
	if (mount_name == NULL) {
		Trace(TR_ERR, "Load exported volume error fseq: %d", fseq);
		return (B_FALSE);
	}

	SamMalloc(rmload, sizeof (LoadExported_t));
	memset(rmload, 0, sizeof (LoadExported_t));

	rmload->mountPoint = mount_name;
	rmload->stream = stream;
	stream->ldarg = (void *) rmload;

	if (pthread_create(&stream->ldtid, NULL, loadExportedVol,
	    (void *)rmload) != 0) {
		WarnSyscallError(HERE, "pthread_create", "");
	}

	return (B_TRUE);
}

/*
 * Load exported volume thread.
 */
static void *
loadExportedVol(
	void *arg)
{
	int fd;
	StreamInfo_t *stream;
	struct sam_rminfo rb;
	struct stat sb;
	char *media_name;
	struct sam_ioctl_rmunload rmunload;
	char *rmPath;
	boolean_t no_errors = B_TRUE;

	LoadExported_t *rmload = (LoadExported_t *)arg;
	stream = rmload->stream;

	Trace(TR_MISC, "Load exported volume: '%s'", stream->vsn);

	SamMalloc(stream->rmPath, MAXPATHLEN + 4);
	rmPath = stream->rmPath;
	rmPath[0] = '\0';
	stream->ldfd = -1;
	setLdCancelSig();

	sprintf(rmPath, "%s/%s/%s", rmload->mountPoint, RM_DIR, stream->vsn);
	media_name = sam_mediatoa(stream->media);

	memset(&rb, 0, SAM_RMINFO_SIZE(1));
	rb.flags = RI_blockio | RI_nopos;
	strncpy(rb.media, media_name, sizeof (rb.media)-1);
	strncpy(rb.file_id, "sam_stage_file", sizeof (rb.file_id));
	strncpy(rb.owner_id, SAM_ARCHIVE_OWNER, sizeof (rb.owner_id));
	strncpy(rb.group_id, SAM_ARCHIVE_GROUP, sizeof (rb.group_id));
	rb.n_vsns = 1;
	memcpy(rb.section[0].vsn, stream->vsn, sizeof (rb.section[0].vsn));

	if (stat(rmPath, &sb) != 0) {
		fd = open(rmPath, O_CREAT | O_TRUNC | SAM_O_LARGEFILE,
		    FILE_MODE);
		if (fd < 0) {
			WarnSyscallError(HERE, "open", rmPath);
			no_errors = B_FALSE;
		} else {
			(void) close(fd);
		}
	}

	/*
	 * Request and open the removable media.
	 */

	if (no_errors) {
		if (sam_request(rmPath, &rb,
		    SAM_RMINFO_SIZE(rb.n_vsns)) == -1) {
			WarnSyscallError(HERE, "sam_request", rmPath);
			no_errors = B_FALSE;
		}
	}

	if (no_errors) {
		fd = open(rmPath, O_RDONLY | SAM_O_LARGEFILE);
		if (fd == -1) {
			no_errors = B_FALSE;
		} else {
			stream->ldfd = fd;
		}
	}

	if (no_errors == B_FALSE) {
		(void) ErrorStream(stream, ENODEV);

	} else {

		Trace(TR_MISC, "Unload exported volume: '%s'", stream->vsn);

		rmunload.flags = 0;

		if (ioctl(fd, F_UNLOAD, &rmunload) < 0) {
			WarnSyscallError(HERE, "ioctl", "F_UNLOAD");
		}
		if (close(fd) < 0) {
			WarnSyscallError(HERE, "close", "");
		}
	}

	(void) unlink(rmPath);
	SamFree(rmload);
	SamFree(stream->rmPath);
	stream->rmPath = NULL;
	stream->ldarg = 0;
	CLEAR_FLAG(stream->flags, SR_WAIT);
	SET_FLAG(stream->flags, SR_WAITDONE);

	return (NULL);
}

/*
 * Restore copy process state from abnormal termination.
 */
static void
restoreCopyProc(
	CopyInstanceInfo_t *instance
)
{
	extern char *WorkDir;
	upath_t req_file;
	CopyRequestInfo_t *request;

	/*
	 * Check if request was seen by copy process.  If not, tell
	 * the scheduler we got it so it doesn't wait forever.
	 */
	(void) sprintf(req_file, "%s/rm%d/%s", WorkDir, instance->ci_rmfn,
	    COPY_FILE_FILENAME);
	request = (CopyRequestInfo_t *)MapInFile(req_file, O_RDWR, NULL);
	if (request != NULL) {
		PthreadMutexLock(&instance->ci_requestMutex);
		request->cr_gotItFlag = B_TRUE;
		PthreadCondSignal(&request->cr_gotIt);
		PthreadMutexUnlock(&instance->ci_requestMutex);
		RemoveMapFile(req_file, request, sizeof (CopyRequestInfo_t));
	}
}

/*
 *	Clear active or pending requests for requested media type
 *	and vsn from work queue.
 */
void
ClearScheduler(
	media_t media,
	char *volume)
{
	int i;
	StreamInfo_t *stream;
	boolean_t found = B_FALSE;

	stream = workQueue.first;
	for (i = 0; i < workQueue.entries; i++) {
		if (media == stream->media &&
		    strcmp(volume, stream->vsn) == 0) {
			found = B_TRUE;
			break;
		}
		stream = stream->next;
	}

	if (found) {
		Trace(TR_MISC, "Clear requests: '%s.%s 0x%x",
		    sam_mediatoa(stream->media), stream->vsn, (int)stream);

		PthreadMutexLock(&stream->mutex);
		SET_FLAG(stream->flags, SR_CLEAR);
		PthreadMutexUnlock(&stream->mutex);
	}
}

/*
 * Check if staging has been suspended by an operator.
 */
static boolean_t
isStagingSuspended(
	/* LINTED argument unused in function */
	media_t	media)
{
	return (B_FALSE);
}

/*
 * Cancel load exported volume.
 */
static void
cancelLoadExpVol(
	StreamInfo_t *stream)
{
	PthreadMutexLock(&stream->mutex);
	if (GET_FLAG(stream->flags, SR_WAIT)) {
		if (stream->ldtid != 0) {
			(void) pthread_kill(stream->ldtid, SIGUSR1);
			if (pthread_join(stream->ldtid, NULL) != 0) {
				WarnSyscallError(HERE,
				    "pthread_join[LoadExportedVol]", "");
			}
			CLEAR_FLAG(stream->flags, SR_WAITDONE);
		}
		CLEAR_FLAG(stream->flags, SR_WAIT);
	}
	PthreadMutexUnlock(&stream->mutex);
}

/*
 * Unblock SIGUSR1 to allow cancel load exported volume.
 */
static void
setLdCancelSig(void)
{
	struct sigaction act;

	(void) memset(&act, 0, sizeof (act));
	act.sa_handler = catchLdCancelSig;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGUSR1, &act, NULL) == -1) {
		WarnSyscallError(HERE, "sigaction(SIGUSR1)", "");
	}

	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGUSR1);
	errno = pthread_sigmask(SIG_UNBLOCK, &act.sa_mask, NULL);
	if (errno != 0) {
		WarnSyscallError(HERE, "pthread_sigmask(SIG_UNBLOCK)", "");
	}
}

/*
 * Catch load cancel signal.
 */
static void
catchLdCancelSig(
	/* LINTED argument unused in function */
	int sig)
{
	StreamInfo_t *stream;
	struct stat sb;
	pthread_t tid = pthread_self();

	/*
	 * Target stream is locked by signal sender.
	 */
	stream = workQueue.first;
	while (stream != NULL) {
		if (GET_FLAG(stream->flags, SR_WAIT) &&
		    (stream->ldtid != 0 && stream->ldtid == tid)) {
			if (stream->rmPath != NULL) {
				if (stream->rmPath[0] != '\0') {
					if (stat(stream->rmPath, &sb) == 0) {
						if (stream->ldfd >= 0) {
							(void) close(
							    stream->ldfd);
						}
						(void) unlink(stream->rmPath);
					}
				}
				SamFree(stream->rmPath);
				stream->rmPath = NULL;
			}
			SamFree(stream->ldarg);
			stream->ldarg = NULL;
			stream->ldtid = 0;
			break;
		}
		stream = stream->next;
	}
	thr_exit(NULL);
}

/*
 * Check pending copy procs folked by previous sam-stagerd.
 */
static void
checkCopies(void)
{
	int i;
	struct stat buf;
	StreamInfo_t *stream;

	if (CopyInstanceList == NULL) {
		return;
	}
	Trace(TR_DEBUG, "Pending copies: entries:%d OrphanProcs:%d",
	    CopyInstanceList->cl_entries, OrphanProcs);

	OrphanProcs = 0;
	for (i = 0; i < CopyInstanceList->cl_entries; i++) {
		CopyInstanceInfo_t *cc = &CopyInstanceList->cl_data[i];

		Trace(TR_DEBUG, "i: %d flags: %04x shutdown: %d",
		    i, cc->ci_flags, cc->ci_shutdown);
		if (GET_FLAG(cc->ci_flags, CI_orphan) == 0) {
			continue;
		}

		if (IF_SHUTDOWN(cc)) {
			pthread_mutexattr_t mattr;
			pthread_condattr_t cattr;

			Trace(TR_MISC, "Orphan copy process pid: %d exited",
			    (int)cc->ci_pid);

			DoneOrphanReq(cc->ci_pid);
			sprintf(fullpath, "%s/%s.%d", SharedInfo->si_streamsDir,
			    cc->ci_vsn, cc->ci_seqnum);
			if (stat(fullpath, &buf) < 0) {
				Trace(TR_DEBUG, "Stream %s for orphan copy "
				    "pid: %d not found.",
				    fullpath, (int)cc->ci_pid);
			} else {
				stream = (StreamInfo_t *)MapInFile(fullpath,
				    O_RDONLY, NULL);
				if (stream != NULL) {
					Trace(TR_DEBUG,
					    "Delete stream: '%s'", fullpath);
					DeleteStream(stream);
				}
			}

			cc->ci_created	= B_FALSE;
			cc->ci_busy	= B_TRUE;
			cc->ci_pid		= 0;
			CLEAR_FLAG(cc->ci_flags, (CI_shutdown | CI_failover));
			cc->ci_first = cc->ci_last = NULL;
			CLEAR_FLAG(cc->ci_flags, CI_orphan);

			PthreadMutexattrInit(&mattr);
			PthreadMutexattrSetpshared(&mattr,
			    PTHREAD_PROCESS_SHARED);
			PthreadCondattrInit(&cattr);
			PthreadCondattrSetpshared(&cattr,
			    PTHREAD_PROCESS_SHARED);

			PthreadMutexInit(&cc->ci_requestMutex, &mattr);
			PthreadCondInit(&cc->ci_request, &cattr);
			PthreadMutexInit(&cc->ci_runningMutex, &mattr);
			PthreadCondInit(&cc->ci_running, &cattr);

			PthreadMutexattrDestroy(&mattr);
			PthreadCondattrDestroy(&cattr);
		} else {
			OrphanProcs++;
		}
	}
}

/*
 * Check orphan copy processes folked by previous sam-stagerd.
 */
void
CheckCopyProcs(void)
{
	int i;
	pid_t pid;
	char rmfn[10];

	initCopyInstanceList(B_TRUE);

	if (CopyInstanceList == NULL) {
		/*
		 * Failed to recover the CopyInstanceList. If there is a pending
		 * request on the request list, kill all "sam-stagerd_copy" to
		 * avoid copy process to accidentally update the request list.
		 * It happens if copy process has an active request and stagerd
		 * failed to recover the CopyInstanceList, stagerd removes a
		 * request from the request list in requeueRequests(), and
		 * copy process also set the FI_DONE in the SetStageDone() when
		 * it detected daemon (parent) exit.
		 */
		if (StageReqs.entries > 0) {
			int numDrives = 0;
			/*
			 * Kill copy procs. 1....(GetNumAllDrives() + 10)
			 *
			 * We really don't know how many copy procs have
			 * configured previously. Assume, current + 10.
			 */
			numDrives = GetNumAllDrives();
			numDrives += 10;
			for (i = 0; i < numDrives; i++) {

				sprintf(rmfn, "%d", i);
				if ((pid = FindProc("sam-stagerd_copy", rmfn))
				    > 0) {
					Trace(TR_MISC, "copy-%d pid: %d "
					    "still running, kill", i, (int)pid);
					kill(pid, SIGKILL);
				}
			}
		}
		return;
	}

	/*
	 * CopyInstanceList has successfully recovered.
	 */

	Trace(TR_DEBUG, "Recovered %d copy procs",
	    CopyInstanceList->cl_entries);
	Trace(TR_DEBUG, "     %8s lib media rmfn creat idl shtdwn flgs %6s "
	    "seq", "pid", "vsn");

	for (i = 0; i < CopyInstanceList->cl_entries; i++) {
		CopyInstanceInfo_t *cc;
		int id;
		int retry;
		int trylock;
		upath_t stream;
		StreamInfo_t *cs;
		FileInfo_t *fi = NULL;

		cc = &CopyInstanceList->cl_data[i];
		Trace(TR_DEBUG,
		    "[%2d] %8d %3d %5s %4d %5d %3d %6d %04x %6s %4d",
		    i, (int)cc->ci_pid, cc->ci_lib,
		    sam_mediatoa(cc->ci_media), cc->ci_rmfn,
		    cc->ci_created, cc->ci_busy, cc->ci_shutdown,
		    cc->ci_flags, cc->ci_vsn, cc->ci_seqnum);
		if (cc->ci_pid <= 0) {
			continue;
		}

		SET_FLAG(cc->ci_flags, CI_orphan);
		sprintf(rmfn, "%d", cc->ci_rmfn);
		if ((pid = FindProc("sam-stagerd_copy", rmfn)) != cc->ci_pid) {
			/*
			 * Copy process is no longer running, make sure
			 * shutdown is set, to force checkCopies() to clean up
			 * this entry.
			 */
			SET_FLAG(cc->ci_flags, CI_shutdown);
		} else {
			/*
			 * Copy process is still running.
			 */
			Trace(TR_MISC, "Orphan copy-%d pid: %d still running",
			    cc->ci_rmfn, (int)pid);
			if (StageReqs.entries == 0) {
				/*
				 * There is no pending requests on request list.
				 * kill copy process.
				 */
				Trace(TR_MISC, "No pending requests, "
				    "kill copy-%d pid:%d",
				    cc->ci_rmfn, (int)pid);

				KillCopyProc(pid, cc);
				continue;
			}
		}

		sprintf(stream, "%s/%s.%d", SharedInfo->si_streamsDir,
		    cc->ci_vsn, cc->ci_seqnum);
		Trace(TR_DEBUG, "attaching stream \"%s\"", stream);
		cs = (StreamInfo_t *)MapInFile(stream, O_RDWR, NULL);
		if (cs == NULL) {
			continue;
		}

		Trace(TR_DEBUG, "stream: '%s.%d' count: %d create: 0x%x "
		    "lib: %d media: '%s' instance: %d pid: %d flags:% 04x "
		    "first: %d error: %d",
		    cs->vsn, cs->seqnum, cs->count, (int)cs->create, cs->lib,
		    sam_mediatoa(cs->media), (int)cs->context, (int)cs->pid,
		    cs->flags, cs->first, cs->error);

		if (Seqnum <= cs->seqnum) {
			if (cs->seqnum >= INT_MAX) {
				Seqnum = 1;
			} else {
				Seqnum = cs->seqnum + 1;
			}
			Trace(TR_MISC, "seqnum updated %d", Seqnum);
		}
		/*
		 * Check if the stream mutex is locked by a copy process
		 * or previous sam-stagerd. Need to kill a copy process if
		 * lock is held since no way to unlock the mutex.
		 */
		retry = 3;
		trylock = EBUSY;
		while (trylock != 0) {
			trylock = pthread_mutex_trylock(&cs->mutex);
			if (trylock != 0) {
				Trace(TR_MISC, "Stream mutex locked %d "
				    "pid: %d, errno: %d",
				    trylock, (int)cs->pid, errno);
				if (--retry > 0) {
					sleep(5);
				} else {
					/*
					 * Stream mutex is locked.
					 * Kill copy proc.
					 */
					if (pid == cc->ci_pid) {
						KillCopyProc(pid, cc);
					}
					break;
				}
			}
		}
		if (trylock == 0) {
			PthreadMutexUnlock(&cs->mutex);
		}

		id = cs->first;
		if (trylock != 0 || id < 0) {
			continue;
		}

		/*
		 * Copy proc is still working on this request.
		 */
		fi = GetFile(id);
		if (fi != NULL) {
			/*
			 * Check if the file mutex is locked by a copy process
			 * or previous sam-stagerd. Need to kill a copy process
			 * if lock is held since no way to unlock the mutex.
			 */
			retry = 3;
			trylock = EBUSY;
			while (trylock != 0) {
				trylock = pthread_mutex_trylock(&fi->mutex);
				if (trylock != 0) {
					Trace(TR_MISC, "File mutex locked %d "
					    "inode: %d.%d, errno: %d",
					    trylock, fi->id.ino, fi->id.gen,
					    errno);
					if (--retry > 0) {
						sleep(5);
					} else {
						/*
						 * File mutex is locked.
						 * Kill copy proc.
						 */
						if (pid == cc->ci_pid) {
							KillCopyProc(pid, cc);
						}
						break;
					}
				}
			}
			if (trylock == 0) {

				PthreadMutexUnlock(&fi->mutex);

				/*
				 * FI_ORPHAN and fi->context are also set in
				 * SetStageDone() by copy proc. But we still
				 * need to set here since if copy proc is
				 * still running, those two are not set yet.
				 * In such a case, we need to indicate this
				 * request should not requeued by setting
				 * fi->flags and fi->context.
				 */
				SET_FLAG(fi->flags, FI_ORPHAN);
				fi->context  = cc->ci_pid;

				Trace(TR_MISC, "Orphan inode %d.%d fseq: %d "
				    "flags: %04x",
				    fi->id.ino, fi->id.gen, fi->fseq,
				    fi->flags);
			}
		}
		UnMapFile(cs, sizeof (StreamInfo_t));
	}
}

/*
 * Recover copyCopyInstanceList cretaed by the previous sam-stagerd.
 */
static int
recoverCopyInstanceList(
	int numDrives)
{
	struct stat buf;
	size_t size;
	int i;
	int j;
	int k;
	int numlibs = 0;
	int xnumlibs = 0;
	int numLibraries;
	int drvcnt;
	LibraryInfo_t *libraries;
	LibraryInfo_t *entry;
	boolean_t valid = B_TRUE;

	if (stat(SharedInfo->si_copyInstancesFile, &buf) != 0) {
		Trace(TR_ERR, "No such %s, errno: %d",
		    SharedInfo->si_copyInstancesFile, errno);
		return (-1);
	}

	CopyInstanceList =
	    (CopyInstanceList_t *)MapInFile(SharedInfo->si_copyInstancesFile,
	    O_RDWR, NULL);
	if (CopyInstanceList == NULL) {
		Trace(TR_ERR, "Cannot map in file %s",
		    SharedInfo->si_copyInstancesFile);
		return (-1);
	}

	size = sizeof (CopyInstanceList_t);
	size += (numDrives - 1) * sizeof (CopyInstanceInfo_t);

	if (CopyInstanceList->cl_magic != COPY_INSTANCE_LIST_MAGIC	||
	    CopyInstanceList->cl_version != COPY_INSTANCE_LIST_VERSION	||
	    CopyInstanceList->cl_create != StageReqs.val->create	||
	    CopyInstanceList->cl_entries != numDrives			||
	    CopyInstanceList->cl_size != size) {

		Trace(TR_MISC, "Invalid %s found, remove.",
		    SharedInfo->si_copyInstancesFile);
		RemoveMapFile(SharedInfo->si_copyInstancesFile,
		    CopyInstanceList, buf.st_size);
		CopyInstanceList = NULL;
		return (-1);
	}

	/*
	 * Mapped in CopyInstanceList has a valid header, then validate the
	 * library config against current library table.
	 */

	j = -1;
	for (i = 0; i < CopyInstanceList->cl_entries; i++) {
		if (CopyInstanceList->cl_data[i].ci_lib != j) {
			j = CopyInstanceList->cl_data[i].ci_lib;
			xnumlibs++;		/* count old library */
		}
	}

	libraries = GetLibraries(&numLibraries);
	for (i = 0; i < numLibraries; i++) {
		entry = &libraries[i];
		if (!IS_LIB_HISTORIAN(entry) && !IS_LIB_THIRDPARTY(entry)) {
			numlibs++;		/* count current library */
		}
	}

	if (numlibs != xnumlibs) {
		Trace(TR_MISC, "Previous %s had different library count: %d "
		    "current: %d, remove",
		    SharedInfo->si_copyInstancesFile, xnumlibs, numlibs);
		RemoveMapFile(SharedInfo->si_copyInstancesFile,
		    CopyInstanceList, buf.st_size);
		CopyInstanceList = NULL;
		return (-1);
	}

	/*
	 * Validate the drive config against current drive table.
	 */
	for (i = 0; i < numLibraries; i++) {
		boolean_t is_remote = FALSE;
		entry = &libraries[i];

		if (IS_LIB_HISTORIAN(entry) || IS_LIB_THIRDPARTY(entry)) {
			continue;
		}

		if (IS_LIB_REMOTE(entry)) {
			is_remote = TRUE;
		}

		drvcnt = 0;

		for (j = 0; j < CopyInstanceList->cl_entries; j++) {
			CopyInstanceInfo_t *ci;
			ci = &CopyInstanceList->cl_data[j];

			if (entry->li_eq != ci->ci_eq) {
				continue;
			}

			if (is_remote &&
			    (GET_FLAG(ci->ci_flags, CI_samremote) == 0)) {
				continue;
			}

			if (IS_LIB_DISK(entry)) {
				drvcnt++;
				Trace(TR_DEBUG, "Disk library found "
				    "drvcnt: %d num_drives: %d",
				    drvcnt, entry->li_numDrives);
			} else {
				for (k = 0; k < numDrives; k++) {
					if (ci->ci_drive == GetEqOrdinal(k)) {
						drvcnt++;
						Trace(TR_DEBUG,
						    "Library found eq: %d "
						    "drvcnt: %d num_drives: %d",
						    ci->ci_drive, drvcnt,
						    entry->li_numDrives);
						break;
					}
				}
			}
		}
		if (drvcnt != entry->li_numDrives) {
			valid = B_FALSE;
			break;
		}
	}
	if (valid != B_TRUE) {
		Trace(TR_MISC, "Previous %s had different drive count: %d "
		    " for library eq: %d, remove",
		    SharedInfo->si_copyInstancesFile, drvcnt, entry->li_eq);
		RemoveMapFile(SharedInfo->si_copyInstancesFile,
		    CopyInstanceList, buf.st_size);
		CopyInstanceList = NULL;
		return (-1);
	}

	Trace(TR_MISC, "%s recovered, create: 0x%x entries: %d",
	    SharedInfo->si_copyInstancesFile, (int)CopyInstanceList->cl_create,
	    CopyInstanceList->cl_entries);

	return (0);
}

/*
 * Kill a copy process and clear the CopyInstanceInfo_t.
 */
void
KillCopyProc(
	pid_t pid,
	CopyInstanceInfo_t *cc)
{
	pthread_mutexattr_t mattr;
	pthread_condattr_t cattr;

	if (pid <= 0) {
		return;
	}
	if (cc == NULL) {
		int i;

		for (i = 0; i < CopyInstanceList->cl_entries; i++) {
			if (CopyInstanceList->cl_data[i].ci_pid == pid) {
				cc = &CopyInstanceList->cl_data[i];
				break;
			}
		}
		if (cc == NULL) {
			Trace(TR_ERR, "No CopyInstanceList for pid: %d found",
			    (int)pid);
			return;
		}
	}

	kill(pid, SIGKILL);

	cc->ci_created = B_FALSE;
	cc->ci_busy = B_TRUE;
	cc->ci_pid = 0;
	cc->ci_first = 0;
	cc->ci_last = 0;
	CLEAR_FLAG(cc->ci_flags, (CI_orphan | CI_shutdown | CI_failover));

	/*
	 * Reinitialize mutex and condition.
	 */
	PthreadMutexattrInit(&mattr);
	PthreadMutexattrSetpshared(&mattr, PTHREAD_PROCESS_SHARED);
	PthreadCondattrInit(&cattr);
	PthreadCondattrSetpshared(&cattr, PTHREAD_PROCESS_SHARED);

	PthreadMutexInit(&cc->ci_requestMutex, &mattr);
	PthreadCondInit(&cc->ci_request, &cattr);
	PthreadMutexInit(&cc->ci_runningMutex, &mattr);
	PthreadCondInit(&cc->ci_running, &cattr);

	PthreadMutexattrDestroy(&mattr);
	PthreadCondattrDestroy(&cattr);
}
