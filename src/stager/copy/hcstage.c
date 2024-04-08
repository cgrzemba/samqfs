/*
 * hcstage.c - functions specific to staging from honeycomb archive files
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

#pragma ident "$Revision: 1.14 $"

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

/* Solaris headers. */
#include <sys/shm.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/shm.h"
#include "pub/stat.h"
#include "sam/fioctl.h"
#include "pub/rminfo.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/diskvols.h"
#include "sam/resource.h"
#include "sam/exit.h"
#include "sam/custmsg.h"
#include "aml/stager.h"
#include "aml/stager_defs.h"
#include "aml/id_to_path.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Honeycomb headers. */
#include "hc.h"
#include "hcclient.h"

/* Local headers. */
#include "stager_config.h"
#include "stager_lib.h"
#include "stager_threads.h"
#include "rmedia.h"
#include "stream.h"
#include "copy_defs.h"

#include "copy.h"
#include "circular_io.h"

/*
 * Context of multi threading Honeycomb API.
 */
typedef struct retrieveObject {
	pthread_t	id;		/* thread id */
	boolean_t	end;		/* set true if terminating thread */

	pthread_mutex_t	requestMutex;	/* mutex for next request */
	boolean_t	next;		/* retrieve oid is available */
	pthread_cond_t	request;

	hc_oid		oid;		/* oid to retrieve */
	hc_long_t	firstbyte;	/* first byte of range to retrieve */
	hc_long_t	lastbyte;	/* last byte of range to retrieve */

	pthread_mutex_t	dataMutex;	/* protect data buffer */
	pthread_cond_t	avail;		/* data available */
	pthread_cond_t	ready;		/* ready for data */
	boolean_t	dataAvail;	/* data present in retrieve buffer */

	char		*data;		/* retrieve buffer */
	long		numBytes;	/* number of bytes in buffer */
	int		error;		/* set if error on honeycomb retrieve */
} retrieveObject_t;

/* Public data */
extern CopyInstanceInfo_t *Instance;
extern IoThreadInfo_t *IoThread;
extern StreamInfo_t *Stream;
extern StagerStateInfo_t *State;

long HcWriteToFile(void *stream, char *buf, long nbytes);

/* Private data */
static hc_session_t *session;		/* Honeycomb session */
static retrieveObject_t *retrieveData;
static char *queryBuf = NULL;

static DiskVolumeInfo_t *diskVolume = NULL;
static DiskVolumeSeqnum_t seqnum;

/* Private functions */
static void initSession();
static void endSession();
static void initAPI();
static void endAPI();
static void *retrieve(void *arg);

/*
 * Init stage from honeycomb data silo.
 */
void
HcInit(void)
{
	DiskVolsDictionary_t *diskvols;
	struct DiskVolumeInfo *dv;
	size_t size;

	diskvols = DiskVolsNewHandle(program_name, DISKVOLS_VSN_DICT,
	    DISKVOLS_RDONLY);
	if (diskvols != NULL) {
		(void) diskvols->Get(diskvols, Stream->vsn, &dv);
		if (dv != NULL) {
			size = STRUCT_RND(sizeof (struct DiskVolumeInfo) +
			    dv->DvPathLen);
			SamMalloc(diskVolume,  size);
			memset(diskVolume, 0,  size);
			memcpy(diskVolume, dv, size);
		} else {
			FatalSyscallError(EXIT_FATAL, HERE, "DiskVols", "get");
		}
		(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
	} else {
		FatalSyscallError(EXIT_NORESTART, HERE, "DiskVols", "open");
	}
	ASSERT(diskVolume != NULL);
}

/*
 * Open honeycomb archive file.
 */
int
HcLoadVolume(void)
{
	Trace(TR_MISC, "Load hc volume: '%s'", Stream->vsn);

	IoThread->io_rftHandle = NULL;

	initSession();		/* initialize honeycomb session */
	initAPI();		/* initialize honeycomb api */

	return (0);
}

/*
 * Open next honeycomb archive file.
 */
int
HcNextArchiveFile(void)
{
	FileInfo_t *file;
	int copy;
	char *vsn;
	hc_oid oid;
	int finished;
	hcerr_t hcerr;
	hc_query_result_set_t *rset;
	int numOids;

	file = IoThread->io_file;
	copy = file->copy;

	Trace(TR_MISC, "HC next archive inode: %d.%d len: %lld offset: %lld",
	    file->id.ino, file->id.gen, file->len, file->offset);

	/*
	 * Find honeycomb OID.
	 */
	vsn = Stream->vsn;
	seqnum = (uint32_t)file->ar[copy].section.position;
	queryBuf = DiskVolsGenMetadataQuery(vsn, seqnum, queryBuf);
	Trace(TR_MISC, "HC query buf: '%s'", queryBuf);

	rset = NULL;
	hcerr = hc_query_ez(session, queryBuf, NULL, 0, 1, &rset);
	if (hcerr != HCERR_OK) {
		Trace(TR_MISC, "HC client query failed, hcerr= %d - %s",
		    hcerr, hc_decode_hcerr(hcerr));
		return (-1);
	}

	numOids = 0;
	for (;;) {
		hcerr = hc_qrs_next_ez(rset, &oid, NULL, &finished);

		if (hcerr != HCERR_OK || finished == 1) {
			Trace(TR_MISC, "HC query complete: "
			    "hcerr= %d finished= %d",
			    hcerr, finished);
			break;
		}
		if (hcerr == HCERR_OK) {
			numOids++;
			Trace(TR_MISC, "HC query complete oid: '%s'", oid);
		}
	}
	(void) hc_qrs_free(rset);

	if (numOids != 1) {
		/*
		 * Unexpected metadata query result, more than 1
		 * (or zero) OIDs found.
		 */
		SendCustMsg(HERE, 19036, numOids);
	}

	if (hcerr != HCERR_OK || numOids != 1) {
		retrieveData->error = EIO;
		retrieveData->dataAvail = B_TRUE;
		return (-1);
	}

	ResetBuffers();
	IoThread->io_position = 0;

	/*
	 * Send retrieve request to worker thread.
	 */
	PthreadMutexLock(&retrieveData->requestMutex);
	strncpy(retrieveData->oid, oid, sizeof (retrieveData->oid));
	Trace(TR_DEBUG, "HC next request complete oid: '%s'",
	    retrieveData->oid);
	retrieveData->firstbyte = file->offset;
	retrieveData->lastbyte = file->offset + file->len;
	retrieveData->next = B_TRUE;
	retrieveData->dataAvail = B_FALSE;	/* no data available yet */
	PthreadCondSignal(&retrieveData->request);
	PthreadMutexUnlock(&retrieveData->requestMutex);

	return (0);
}

/*
 * End of honeycomb archive file.
 */
void
HcEndArchiveFile(void)
{
	Trace(TR_DEBUG, "End HC archive file");
	PthreadMutexLock(&retrieveData->dataMutex);
	while (retrieveData->dataAvail == B_FALSE) {
		PthreadCondWait(&retrieveData->avail, &retrieveData->dataMutex);
	}
	PthreadMutexUnlock(&retrieveData->dataMutex);
	Trace(TR_DEBUG, "End HC archive file complete");
}

/*
 * Read from honeycomb media.
 */
int
HcReadArchiveFile(
	/* LINTED argument unused in function */
	void *rft,
	void *buf,
	size_t nbytes)
{
	static int bytesOverrun = 0;
	ssize_t bytesRead;
	ssize_t totalRead;
	ssize_t dataToRetrieve;
	char *offset;

	Trace(TR_MISC, "Read HC archive file, buf: 0x%x nbytes: %d",
	    (int)buf, nbytes);

	offset = buf;
	dataToRetrieve = nbytes;
	totalRead = 0;

	if (bytesOverrun > 0) {
		/*
		 * There is more data from a previous retrieve.
		 */
		Trace(TR_MISC, "HC overrun %d bytes to 0x%x, left %d",
		    bytesOverrun, (int)offset, dataToRetrieve - bytesOverrun);

		if (dataToRetrieve - bytesOverrun <= 0) {
			ASSERT_NOT_REACHED();
			return (-1);
		}

		memcpy(offset, retrieveData->data, bytesOverrun);

		offset += bytesOverrun;
		dataToRetrieve -= bytesOverrun;
		totalRead += bytesOverrun;

		/*
		 * Buffer overrun complete.  Ready for more data.
		 */
		bytesOverrun = 0;

		PthreadMutexLock(&retrieveData->dataMutex);
		retrieveData->dataAvail = B_FALSE;
		PthreadCondSignal(&retrieveData->ready);
		PthreadMutexUnlock(&retrieveData->dataMutex);
	}

	while (dataToRetrieve > 0) {
		/*
		 * Wait for data available from retrieve thread.
		 */
		PthreadMutexLock(&retrieveData->dataMutex);
		while (retrieveData->dataAvail == B_FALSE) {
			PthreadCondWait(&retrieveData->avail,
			    &retrieveData->dataMutex);
		}

		bytesRead = retrieveData->numBytes;
		if (bytesRead == -1) {
			Trace(TR_DEBUG, "Read HC archive file, %d found",
			    retrieveData->error);
			SetErrno = retrieveData->error;

			PthreadCondSignal(&retrieveData->ready);
			PthreadMutexUnlock(&retrieveData->dataMutex);
			break;
		}

		if (bytesRead > dataToRetrieve) {
			bytesOverrun = bytesRead - dataToRetrieve;

			Trace(TR_MISC, "HC retrieve overrun "
			    "%d bytes read %d, left %d",
			    bytesOverrun, bytesRead, dataToRetrieve);

			bytesRead = dataToRetrieve;
		}

		Trace(TR_MISC, "HC retrieve %d bytes to 0x%x, left %d",
		    bytesRead, (int)offset, dataToRetrieve - bytesRead);

		memcpy(offset, retrieveData->data, bytesRead);

		offset += bytesRead;
		totalRead += bytesRead;
		dataToRetrieve -= bytesRead;

		if (bytesOverrun == 0) {
			/*
			 * Ready for more data from retrieve thread.
			 * Buffer is empty.
			 */
			retrieveData->dataAvail = B_FALSE;
			PthreadCondSignal(&retrieveData->ready);
		} else {
			/*
			 * Buffer overrun. Overrun data will be picked
			 * up on next call.
			 */
			retrieveData->data += bytesRead;
		}
		PthreadMutexUnlock(&retrieveData->dataMutex);
	}

	Trace(TR_MISC, "Read HC archive file complete %d bytes", totalRead);

	return (totalRead);
}

/*
 * Seek to position on honeycomb archive file.
 */
int
HcSeekVolume(
	int to_pos)
{
	/* Nothing to do */
	return (to_pos);
}

/*
 * Get position of honeycomb archive file.
 */
u_longlong_t
HcGetPosition(void)
{
	/* Not available */
	return (-1);
}


/*
 * Close honeycomb archive file.
 */
void
HcUnloadVolume(void)
{
	Trace(TR_MISC, "Unload hc volume: '%s'", Stream->vsn);

	endAPI();
	endSession();

	(void) SamrftClose(IoThread->io_rftHandle);
	SamrftDisconnect(IoThread->io_rftHandle);
	if (diskVolume != NULL) {
		SamFree(diskVolume);
		diskVolume = NULL;
	}
}

/*
 * Establish session to honeycomb data silo.  If connection fails,
 * the copy process will exit.
 */
static void
initSession(void)
{
	char *addr;
	int port;
	hcerr_t hcerr;

	session = NULL;

	IoThread->io_rftHandle = (void *) SamrftConnect("localhost");
	if (IoThread->io_rftHandle == NULL) {
		LibFatal(SamrftConnect, "localhost");
	}

	addr = diskVolume->DvAddr;
	port = diskVolume->DvPort;

	/*
	 *	Initialize global session properties.
	 */
	hcerr = hc_init(malloc, free, realloc);
	if (hcerr != HCERR_OK) {
		SendCustMsg(HERE, 4057, hcerr, hc_decode_hcerr(hcerr));
		exit(EXIT_FAILURE);
	}

	/*
	 * Create honeycomb session.  A copy of the schema will be downloaded.
	 */
	hcerr = hc_session_create_ez(addr, port, &session);
	if (hcerr != HCERR_OK) {
		SendCustMsg(HERE, 4058, addr, (int)port,
		    hcerr, hc_decode_hcerr(hcerr));
		exit(EXIT_FAILURE);
	}

	Trace(TR_MISC, "HC session create addr '%s' port= %d", addr, port);
}

/*
 * Terminate and free honeycomb's global session.
 */
static void
endSession(void)
{
	hc_session_free(session);
	hc_cleanup();
}


/*
 * Initialize honeycomb api infrastructure.  Create worker thread for
 * retrieving data from honeycomb data silo.
 */
static void
initAPI(void)
{
	int rval;

	SamMalloc(retrieveData, sizeof (retrieveObject_t));
	memset(retrieveData, 0, sizeof (retrieveObject_t));

	PthreadMutexInit(&retrieveData->requestMutex, NULL);
	retrieveData->next = B_FALSE;
	PthreadCondInit(&retrieveData->request, NULL);

	PthreadMutexInit(&retrieveData->dataMutex, NULL);
	retrieveData->dataAvail = B_FALSE;
	PthreadCondInit(&retrieveData->avail, NULL);
	PthreadCondInit(&retrieveData->ready, NULL);

	rval = pthread_create(&retrieveData->id,  NULL, retrieve, (void *)NULL);
	if (rval != 0) {
		LibFatal(pthread_create, "retrieve");
	}
}

/*
 * Terminate honeycomb API thread.
 */
static void
endAPI(void)
{
	void *status;

	PthreadMutexLock(&retrieveData->requestMutex);
	retrieveData->end = B_TRUE;
	PthreadCondSignal(&retrieveData->request);
	PthreadMutexUnlock(&retrieveData->requestMutex);

	/*
	 * Wait for final reply from retrieve thread.  Need to make sure
	 * the hc retrieve function has completed.
	 */
	(void) pthread_join(retrieveData->id, &status);
}

static void *
retrieve(
	/* LINTED argument unused in function */
	void *arg)
{
	int rval;
	hcerr_t hcerr;
	hc_oid oid;
	struct timespec timeout;
	hc_long_t firstbyte;
	hc_long_t lastbyte;

	Trace(TR_MISC, "Starting HC retrieve thread");

	for (;;) {
		/*
		 * Wait to recieve oid.
		 */
		timeout.tv_sec = time(NULL) + 2;
		timeout.tv_nsec = 0;

		PthreadMutexLock(&retrieveData->requestMutex);

		while (retrieveData->next == B_FALSE &&
		    retrieveData->end == B_FALSE) {

			rval = pthread_cond_timedwait(&retrieveData->request,
			    &retrieveData->requestMutex, &timeout);

			if (rval == ETIMEDOUT) {
				timeout.tv_sec = time(NULL) + 2;
				timeout.tv_nsec = 0;
			}
		}

		if (retrieveData->end == B_TRUE) {
			PthreadMutexUnlock(&retrieveData->requestMutex);
			break;
		}

		strncpy(oid, retrieveData->oid, sizeof (hc_oid));
		firstbyte = retrieveData->firstbyte;
		lastbyte = retrieveData->lastbyte;
		retrieveData->next = B_FALSE;

		Trace(TR_MISC, "HC retrieve ez oid: '%s' range: %lld:%lld",
		    oid, firstbyte, lastbyte);

		PthreadMutexUnlock(&retrieveData->requestMutex);

		hcerr = hc_range_retrieve_ez(session, &HcWriteToFile,
		    (void *) retrieveData, &oid, firstbyte, lastbyte);
#if 0
		hcerr = hc_retrieve_ez(session, &HcWriteToFile,
		    (void *) retrieveData, &oid);
#endif

		Trace(TR_MISC, "HC retrieve ez complete");

		PthreadMutexLock(&retrieveData->dataMutex);
		/*
		 * Wait for consumer to read data.
		 */
		while (retrieveData->dataAvail == B_TRUE) {
			PthreadCondWait(&retrieveData->ready,
			    &retrieveData->dataMutex);
		}

		/*
		 * Generate a final reply.
		 */
		retrieveData->error = 0;
		if (hcerr != HCERR_OK) {
			Trace(TR_MISC, "HC retrieve ez failed, hcerr= %d - %s",
			    hcerr, hc_decode_hcerr(hcerr));
			retrieveData->error = EIO;
		}

		retrieveData->data = NULL;
		retrieveData->numBytes = -1;
		retrieveData->dataAvail = B_TRUE;	/* data is available */

		PthreadCondSignal(&retrieveData->avail);
		PthreadMutexUnlock(&retrieveData->dataMutex);
	}

	Trace(TR_MISC, "Terminating HC retrieve thread");
	return ((void *)NULL);
}

/*
 * Honeycomb function callback to write the retrieved data.
 */
long
HcWriteToFile(
	/* LINTED argument unused in function */
	void *stream,
	char *buf,
	long nbytes)
{
	/*
	 * API seems to send a number of zero byte reads,
	 * ignore them here.
	 */

	if (nbytes > 0) {

		/*
		 * Buffer is empty, provide new data.
		 */
		PthreadMutexLock(&retrieveData->dataMutex);
		retrieveData->data = buf;
		retrieveData->numBytes = nbytes;
		retrieveData->dataAvail = B_TRUE;	/* data is available */

		Trace(TR_DEBUG, "HC callback %d bytes from 0x%x",
		    (int)nbytes, (int)buf);

		PthreadCondSignal(&retrieveData->avail);

		/*
		 * Important. Must wait for consumer to move data from buffer
		 * before going back to the honeycomb.
		 */
		while (retrieveData->dataAvail == B_TRUE) {
			PthreadCondWait(&retrieveData->ready,
			    &retrieveData->dataMutex);
		}
		PthreadMutexUnlock(&retrieveData->dataMutex);
	}

	return (nbytes);
}
