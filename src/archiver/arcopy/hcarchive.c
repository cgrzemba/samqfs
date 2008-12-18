/*
 * hcarchive.c - Honeycomb media archival.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */


#pragma ident "$Revision: 1.23 $"

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
#include "sam/lib.h"
#include "aml/device.h"
#include "aml/diskvols.h"
#include "aml/opticals.h"
#include "aml/sam_rft.h"
#include <sam/fs/bswap.h>

/* Honeycomb headers. */
#include "hc.h"
#include "hcclient.h"

/* Local headers. */
#include "arcopy.h"

/* Context for multi threading Honeycomb API. */
struct storeObject {
	pthread_t	id;		/* thread id */
	boolean_t	end;		/* set true if terminating thread */

	pthread_mutex_t requestMutex;	/* mutex for next request */
	boolean_t	next;		/* store oid is available */
	pthread_cond_t	request;

	DiskVolumeSeqnum_t seqnum;	/* unique volume seqnum for data */
					/* object */

	pthread_mutex_t	dataMutex;	/* data mutex */
	boolean_t	isAvail;	/* data from disk cache is available */
	pthread_cond_t	avail;
	boolean_t	isComplete;	/* store to honeycomb is complete */
	pthread_cond_t	complete;

	pthread_mutex_t	storeMutex;	/* store mutex */
	boolean_t	isStoreboth;	/* hc store both api is active */
	pthread_cond_t	storeboth;

	char		*data;		/* store buffer */
	char		*offset;	/* data buffer offset */
	long		numBytes;	/* number of bytes in buffer to store */
	int		error;		/* set if error on honeycomb store */
};

/* Private data. */
static struct DiskVolumeInfo *diskVolume;
static upath_t seqnumFileName;		/* sequence number file name */
static hc_session_t *session;		/* Honeycomb session */
static struct storeObject *storeData;
static char *metaBuf = NULL;
static char *queryBuf = NULL;

/* Honeycomb metadata information for sequence number record. */
static struct {
	boolean_t	exists;		/* set true if seqnum metadata */
					/* record exists */
	boolean_t	dupcheck;	/* set true if checked for dup */
					/* metadata entry */
} seqnumMetadata = {
	B_FALSE, B_FALSE
};

/* Private functions. */
static void createSamMeta(hc_nvr_t *nvr, DiskVolumeSeqnum_t seqnum);
static void createSeqnumMeta();
/* LINTED static unused */
static void endAPI(void);
/* LINTED static unused */
static void endSession(void);
/* LINTED static unused */
static void errorMsg(char *op);
static boolean_t ifMetaExists(char *volname, DiskVolumeSeqnum_t seqnum);
static void initAPI(void);
static void initSession(void);
static void querySeqnumMeta(char *volname);
static void *store(void *arg);
static void outOfSpace(void);

/*
 * Honeycomb archiving to the sequence number is complete.
 */
void
HcArchiveDone(void)
{
	/*
	 * Update disk volume seqnum file.
	 */
	Trace(TR_DEBUG, "Hc archive done");
	DkUpdateVolumeSeqnum(diskVolume->DvPath, seqnumFileName);
}


/*
 * Begin writing honeycomb archive file.  Called at the beginning of an
 * archive file.
 */
void
HcBeginArchiveFile(void)
{
	DiskVolumeSeqnum_t seqnum;
	struct VolsEntry *ve;
	int	rval;

	Trace(TR_DEBUG, "Begin HC archive file '%s'", File->f->FiName);

	/*
	 * Set block size and buffer size.
	 */
	BlockSize = HONEYCOMB_DEFAULT_BLOCKSIZE;
	WriteCount = HONEYCOMB_DEFAULT_BLOCKSIZE;

	ve = &VolsTable->entry[VolCur];

	/*
	 * Before generating seed value, make sure the disk
	 * volume seqnum path exists.
	 *
	 * If the seqnum metadata record exists on the honeycomb and
	 * the seqnum file was removed, DkGetVolumeSeqnum will not
	 * generate a new seqnum and archiving will not continue for
	 * the honeycomb disk volume.
	 */
	if (seqnumMetadata.exists == B_FALSE) {
		rval = DiskVolsLabel(ve->Vi.VfVsn, diskVolume, NULL);
		if (rval != 0) {
			SendCustMsg(HERE, 4050, ve->Vi.VfVsn, GetCustMsg(4053));
			exit(EXIT_FAILURE);
		}
		/*
		 * Create seqnum metadata record on honeycomb.
		 */
		createSeqnumMeta();
	}

	seqnum = DkGetVolumeSeqnum(diskVolume->DvPath, seqnumFileName);

	/*
	 * Check for fatal error on disk volume seqnum file.  If an error
	 * occured, set the error flag on honeycomb volume.
	 */
	if (seqnum < 0 || ifMetaExists(ve->Vi.VfVsn, seqnum) == B_TRUE) {
		SendCustMsg(HERE, 4060, seqnumFileName);
		DkSetDiskVolsFlag(diskVolume, DV_bad_media);
		exit(EXIT_FAILURE);
	}

	/*
	 * Enter media information for archreq.
	 */
	VolsTable->entry[VolCur].VlPosition = seqnum;

	/*
	 * Send store request to worker thread.
	 */
	PthreadMutexLock(&storeData->requestMutex);
	storeData->seqnum = seqnum;
	storeData->next = B_TRUE;
	PthreadCondSignal(&storeData->request);
	PthreadMutexUnlock(&storeData->requestMutex);

	/*
	 * Wait for hc_store_both_ez to start.
	 */
	PthreadMutexLock(&storeData->storeMutex);
	while (storeData->isStoreboth == B_FALSE) {
		PthreadCondWait(&storeData->storeboth,
		    &storeData->storeMutex);
	}
	PthreadMutexUnlock(&storeData->storeMutex);
}


/*
 * Begin copying a file.
 * Enter volume and position for file being copied.
 */
void
HcBeginCopyFile(void)
{
	Trace(TR_DEBUG, "Begin HC copy file '%s'", File->f->FiName);
	File->AfVol = VolCur;
	File->AfPosition = VolsTable->entry[VolCur].VlPosition;
	File->AfOffset = 0;
}


/*
 * End honeycomb archive file.  The Honeycomb store callback
 * will be terminated.
 */
void
HcEndArchiveFile(void)
{
	Trace(TR_MISC, "[%s] End HC archive file", File->f->FiName);
	/*
	 * Terminate Honeycomb store callback.
	 *
	 * If idopen failed, ex. file changed, the store callback was not
	 * started so stopping the callback is not necessary.
	 */
	PthreadMutexLock(&storeData->storeMutex);
	if (storeData->isStoreboth == B_FALSE) {
		PthreadMutexUnlock(&storeData->storeMutex);
		return;
	}

	(void) HcWrite(NULL, NULL, 0);

	while (storeData->isStoreboth == B_TRUE) {
		PthreadCondWait(&storeData->storeboth, &storeData->storeMutex);
	}
	PthreadMutexUnlock(&storeData->storeMutex);
}


/*
 * Callback function for creating seqnum metadata record on honeycomb.
 */
long
HcCreateSeqnumMeta(
	/* LINTED argument unused in function */
	void *stream,
	/* LINTED argument unused in function */
	char *buf,
	/* LINTED argument unused in function */
	long nbytes)
{
	return (0);
}


/*
 * Initialize honeycomb archiving and map in disk volume dictionary.
 */
void
HcInit(void)
{
	extern char *program_name;
	struct MediaParamsEntry *mp;
	struct VolsEntry *ve;
	struct DiskVolsDictionary *dict;
	struct DiskVolumeInfo *dv;
	size_t size;
	char *mtype;

	Trace(TR_DEBUG, "Init HC archive file");

	mtype = sam_mediatoa(DT_STK5800);
	mp = MediaParamsGetEntry(mtype);
	WriteTimeout = mp->MpTimeout;

	ve = &VolsTable->entry[VolCur];
	diskVolume = NULL;

	dict = DiskVolsNewHandle(program_name, DISKVOLS_VSN_DICT, 0);
	if (dict == NULL) {
		LibFatal(DiskVolsNewHandle, "InitHcArchive");
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
		LibFatal(dict->Get, "InitHcArchive");
	}

	snprintf(ArVolName, sizeof (ArVolName), "%s.%s", mtype, ve->Vi.VfVsn);

	initSession();		/* initialize honeycomb session */
	initAPI();		/* initialize honeycomb api */

	snprintf(seqnumFileName, sizeof (seqnumFileName), "%s.%s",
	    ve->Vi.VfVsn, DISKVOLS_SEQNUM_SUFFIX);
	VolBytesWritten = 0;

	/*
	 * Query honeycomb for seqnum metadata string.  If the volume's
	 * sequence number metadata exists then the seqnum file has been
	 * created and must exist for archiving to continue.  Intended to
	 * be called only once.
	 */
	querySeqnumMeta(ve->Vi.VfVsn);

#if defined(lint)
	sam_bswap2(diskVolume, 1);
	sam_bswap4(diskVolume, 1);
	sam_bswap8(diskVolume, 1);
#endif /* defined(lint) */
}


/*
 * Honeycomb function callback for source of data to be stored.
 */
long
HcReadFromFile(
	/* LINTED argument unused in function */
	void *stream,
	char *buf,
	long nbytes)
{
	static boolean_t complete = B_FALSE;
	static char *offset = NULL;

	long bytesToStore;

	PthreadMutexLock(&storeData->dataMutex);

	if (complete == B_TRUE) {
		/*
		 * Send store complete.
		 */
		offset = NULL;
		complete = B_FALSE;

		storeData->isComplete = B_TRUE;
		storeData->isAvail = B_FALSE;

		PthreadCondSignal(&storeData->complete);
	}

	/*
	 * Check if need a new data buffer to store.
	 */
	if (offset == NULL) {
		Trace(TR_DEBUG, "[%s] store new buffer nbytes= %ld",
		    (char *)File->f->FiName, nbytes);

		/* Wait for data from HcWrite */

		while (storeData->isAvail == B_FALSE) {
			PthreadCondWait(&storeData->avail,
			    &storeData->dataMutex);
		}
		offset = storeData->data;

	}

	bytesToStore = storeData->numBytes;

	if (bytesToStore > 0) {
		bytesToStore = (bytesToStore > nbytes) ? nbytes : bytesToStore;
		memcpy(buf, offset, bytesToStore);

		storeData->numBytes -= bytesToStore;
	}

	if (storeData->numBytes == 0) {

		Trace(TR_DEBUG, "[%s] store complete bytesToStore= %ld",
		    (char *)File->f->FiName, bytesToStore);

		/*
		 * Completed write of data in this buffer.
		 */
		complete = B_TRUE;
	} else {
		/*
		 * There is more data in this buffer to store.
		 */
		offset += bytesToStore;
	}

	if (bytesToStore == 0) {
		/*
		 * Final response.  Not coming back.
		 */
		offset = NULL;
		complete = B_FALSE;
		Trace(TR_DEBUG, "[%s] store terminate bytesToStore= %ld",
		    (char *)File->f->FiName, bytesToStore);
	} else {
		Trace(TR_DEBUG,
		    "[%s] store exit bytesToStore= %ld offset= 0x%x",
		    (char *)File->f->FiName, bytesToStore, (int)offset);
	}

	PthreadMutexUnlock(&storeData->dataMutex);

	return (bytesToStore);
}


/*
 * Write to honeycomb media.
 */
int
HcWrite(
	/* LINTED argument unused in function */
	void *rft,
	void *buf,
	size_t nbytes)
{
	size_t bytesWritten;

	Trace(TR_DEBUG, "[%s] HC write nbytes= %d",
	    File->f->FiName, nbytes);

	PthreadMutexLock(&storeData->dataMutex);

	storeData->data = buf;
	storeData->numBytes = nbytes;

	storeData->isAvail = B_TRUE;
	storeData->isComplete = B_FALSE;

	PthreadCondSignal(&storeData->avail);

	/*
	 * Wait for store thread to complete write.
	 */
	while (storeData->isComplete == B_FALSE) {
		PthreadCondWait(&storeData->complete,
		    &storeData->dataMutex);
	}

	bytesWritten = nbytes;
	Trace(TR_DEBUG, "[%s] HC write complete %d",
	    File->f->FiName, bytesWritten);

	PthreadMutexUnlock(&storeData->dataMutex);

	return (bytesWritten);
}


/*
 * Write error.
 */
int
HcWriteError(void)
{
	struct VolsEntry *ve;

	ve = &VolsTable->entry[VolCur];

	Trace(TR_ERR, "Hc write error: %s (%s) errno= %d",
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


static void
createSeqnumMeta()
{
	hcerr_t hcerr;
	hc_nvr_t *nvr;
	hc_system_record_t systemRecord;
	struct VolsEntry *ve;

	ve = &VolsTable->entry[VolCur];

	if (metaBuf == NULL) {
		SamMalloc(metaBuf, HONEYCOMB_METADATA_QUERYLEN);
	}

	/*
	 * Add metadata record, 'hcvol1:hcvol1.seqnum', to mark creation
	 * of the seqnum file.  If the seqnum file DOES NOT exist and this
	 * metadata record DOES exist, archiving will not continue.
	 */
	snprintf(metaBuf, HONEYCOMB_METADATA_QUERYLEN,
	    "%s:%s", ve->Vi.VfVsn, seqnumFileName);

	hcerr = hc_nvr_create(session, HONEYCOMB_NUM_METADATA, &nvr);
	if (hcerr != HCERR_OK) {
		LibFatal(hc_nvr_create, "");
	}

	hcerr = hc_nvr_add_from_string(nvr, HONEYCOMB_METADATA_ARCHID,
	    (hc_string_t)metaBuf);
	if (hcerr != HCERR_OK) {
		LibFatal(hc_nvr_add_from_string, metaBuf);
	}
	Trace(TR_MISC, "[%s] HC store both archive id= '%s'",
	    seqnumFileName, metaBuf);

	hcerr = hc_store_both_ez(session, &HcCreateSeqnumMeta, NULL,
	    nvr, &systemRecord);

	if (hcerr != HCERR_OK) {
		SendCustMsg(HERE, 4059, diskVolume->DvAddr, diskVolume->DvPort,
		    hcerr, hc_decode_hcerr(hcerr));
		exit(EXIT_FAILURE);
	}

	hcerr = hc_nvr_free(nvr);
	if (hcerr != HCERR_OK) {
		LibFatal(hc_nvr_free, "");
	}

	seqnumMetadata.exists = B_TRUE;

	Trace(TR_DEBUG, "[%s] HC store both complete", seqnumFileName);
	Trace(TR_DEBUG, "\toid= '%s'", systemRecord.oid);
}

/*
 * Terminate honeycomb API thread.
 */
static void
endAPI(void)
{
	PthreadMutexLock(&storeData->requestMutex);
	storeData->end = B_TRUE;
	storeData->next = B_TRUE;
	PthreadCondSignal(&storeData->request);
	PthreadMutexUnlock(&storeData->requestMutex);
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
 * Return true if metadata record for this volume's seqnum already exists.
 */
static boolean_t
ifMetaExists(
	char *volname,
	DiskVolumeSeqnum_t seqnum)
{
	hc_oid oid;
	int finished;
	hcerr_t hcerr;
	hc_query_result_set_t *rset;
	boolean_t duplicate;

	/*
	 * The query is expensive.  Only check for duplicate once.
	 */
	if (seqnumMetadata.dupcheck == B_TRUE) {
		return (B_FALSE);
	}

	duplicate = B_FALSE;
	seqnumMetadata.dupcheck = B_TRUE;

	queryBuf = DiskVolsGenMetadataQuery(volname, seqnum, queryBuf);

	Trace(TR_MISC, "HC query buf: '%s'", queryBuf);

	rset = NULL;
	hcerr = hc_query_ez(session, queryBuf, NULL, 0, 100, &rset);
	if (hcerr != HCERR_OK) {
		Trace(TR_MISC, "HC client query failed, hcerr= %d - %s",
		    hcerr, hc_decode_hcerr(hcerr));
		exit(EXIT_FAILURE);
	}

	for (;;) {
		hcerr = hc_qrs_next_ez(rset, &oid, NULL, &finished);
		if (hcerr != HCERR_OK || finished == 1) {
			break;
		}
		if (hcerr == HCERR_OK) {
			/*
			 * Found seqnum metadata record.
			 */
			duplicate = B_TRUE;
		}
	}
	(void) hc_qrs_free(rset);

	return (duplicate);
}


/*
 * Initialize honeycomb API infrastructure.  Create worker thread for
 * retrieving data from honeycomb data silo.
 */
static void
initAPI(void)
{
	int rval;

	SamMalloc(storeData, sizeof (struct storeObject));
	memset(storeData, 0, sizeof (struct storeObject));

	PthreadMutexInit(&storeData->dataMutex, NULL);
	PthreadCondInit(&storeData->avail, NULL);
	PthreadCondInit(&storeData->complete, NULL);
	PthreadMutexInit(&storeData->storeMutex, NULL);
	PthreadCondInit(&storeData->storeboth, NULL);

	storeData->isComplete = B_FALSE;
	storeData->isAvail    = B_FALSE;
	storeData->isStoreboth = B_FALSE;

	rval = pthread_create(&storeData->id,  NULL, store, (void *)NULL);
	if (rval != 0) {
		LibFatal(pthread_create, "store");
	}
}


/*
 * Establish session to honeycomb data silo.  If connection fails,
 * arcopy will exit.
 */
static void
initSession(void)
{
	char *addr;
	int port;
	hcerr_t hcerr;

	session = NULL;

	RemoteArchive.rft = (void *)SamrftConnect("localhost");
	if (RemoteArchive.rft == NULL) {
		LibFatal(SamrftConnect, RemoteArchive.host);
	}
	RemoteArchive.enabled = TRUE;

	addr = diskVolume->DvAddr;
	port = diskVolume->DvPort;

	/*
	 * Initialize global session properties.
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

	Trace(TR_MISC, "HC session created addr '%s' port= %d",
	    addr, (int)port);
}


/*
 * Query honeycomb for seqnum metadata string.  If the volume's
 * sequence number metadata exists then the seqnum file has been
 * created and must exist for archiving to continue.  Intended to
 * be called only once.
 */
static void
querySeqnumMeta(
	char *volname)
{
	hc_oid oid;
	int finished;
	hcerr_t hcerr;
	hc_query_result_set_t *rset;

	/*
	 * Initialize local static structure.
	 */
	seqnumMetadata.exists = B_FALSE;
	seqnumMetadata.dupcheck = B_TRUE;	/* ignore duplicate check */

	if (queryBuf == NULL) {
		SamMalloc(queryBuf, HONEYCOMB_METADATA_QUERYLEN);
	}

	/*
	 * Generate query string for seqnum metadata record.  For example,
	 * 'archiveId = hcvol1:hcvol1.seqnum'.
	 */
	snprintf(queryBuf, HONEYCOMB_METADATA_QUERYLEN, "%s = '%s:%s'",
	    HONEYCOMB_METADATA_ARCHID, volname, seqnumFileName);

	Trace(TR_MISC, "HC query buf: '%s'", queryBuf);

	rset = NULL;
	hcerr = hc_query_ez(session, queryBuf, NULL, 0, 100, &rset);
	if (hcerr != HCERR_OK) {
		Trace(TR_MISC, "HC client query failed, hcerr= %d - %s",
		    hcerr, hc_decode_hcerr(hcerr));
		exit(EXIT_FAILURE);
	}

	for (;;) {
		hcerr = hc_qrs_next_ez(rset, &oid, NULL, &finished);
		if (hcerr != HCERR_OK || finished == 1) {
			break;
		}
		if (hcerr == HCERR_OK) {
			/*
			 * Found seqnum metadata record.
			 */
			seqnumMetadata.exists = B_TRUE;
		}
	}

	(void) hc_qrs_free(rset);
}


/*
 * Thread for retrieving data from honeycomb data silo.
 */
static void *
store(
	/* LINTED argument unused in function */
	void *arg)
{
	hcerr_t hcerr;
	hc_system_record_t systemRecord;
	hc_nvr_t *nvr;
	DiskVolumeSeqnum_t seqnum;
	struct timespec timeout;
	int rval;

	Trace(TR_DEBUG, "Starting HC store thread");

	for (;;) {
		/*
		 * Wait to receive data.
		 */
		timeout.tv_sec = time(NULL) + 2;
		timeout.tv_nsec = 0;

		PthreadMutexLock(&storeData->requestMutex);

		while (storeData->next == B_FALSE &&
		    storeData->end == B_FALSE) {
			rval = pthread_cond_timedwait(&storeData->request,
			    &storeData->requestMutex, &timeout);
			if (rval == ETIMEDOUT) {
				timeout.tv_sec = time(NULL) + 2;
				timeout.tv_nsec = 0;
			}
		}

		if (storeData->end == B_TRUE) {
			PthreadMutexUnlock(&storeData->requestMutex);
			break;
		}

		seqnum = storeData->seqnum;
		storeData->next = B_FALSE;

		PthreadMutexLock(&storeData->storeMutex);
		storeData->isStoreboth = B_TRUE;
		PthreadCondSignal(&storeData->storeboth);
		PthreadMutexUnlock(&storeData->storeMutex);

		Trace(TR_MISC, "[%s] Create HC metadata",
		    File->f->FiName);

		PthreadMutexUnlock(&storeData->requestMutex);

		/*
		 * Create enough slots for all metadata entries.
		 * No error is returned if this value is incorrect.
		 */
		hcerr = hc_nvr_create(session, HONEYCOMB_NUM_METADATA, &nvr);
		if (hcerr != HCERR_OK) {
			LibFatal(hc_nvr_create, "");
		}

		createSamMeta(nvr, seqnum);

		hcerr = hc_store_both_ez(session, &HcReadFromFile,
		    (void *) storeData, nvr, &systemRecord);

		if (hcerr != HCERR_OK) {
			SendCustMsg(HERE, 4059, diskVolume->DvAddr,
			    diskVolume->DvPort, hcerr, hc_decode_hcerr(hcerr));
			HcArchiveDone();
			exit(EXIT_FAILURE);
		}

		hcerr = hc_nvr_free(nvr);
		if (hcerr != HCERR_OK) {
			LibFatal(hc_nvr_free, "");
		}

		Trace(TR_MISC, "[%s] HC store both complete seqnum= %llx",
		    File->f->FiName, seqnum);
		Trace(TR_MISC, "\toid= '%s'", systemRecord.oid);

		PthreadMutexLock(&storeData->dataMutex);
		storeData->isComplete = B_TRUE;
		storeData->isAvail = B_FALSE;
		PthreadCondSignal(&storeData->complete);
		PthreadMutexUnlock(&storeData->dataMutex);

		PthreadMutexLock(&storeData->storeMutex);
		storeData->isStoreboth = B_FALSE;
		PthreadCondSignal(&storeData->storeboth);
		PthreadMutexUnlock(&storeData->storeMutex);
	}

	Trace(TR_DEBUG, "Terminating HC store thread");
	return ((void *)NULL);
}

/*
 * Create SAM specific metadata.
 */
static void
createSamMeta(
hc_nvr_t *nvr,
DiskVolumeSeqnum_t seqnum
)
{
	static char fn[HONEYCOMB_METADATA_STRINGLEN];
	char ts[ISO_STR_FROM_TIME_BUF_SIZE];
	struct VolsEntry *ve;
	char *filename;
	hcerr_t hcerr;

	ve = &VolsTable->entry[VolCur];

	/*
	 * Create unique id for the archive's metadata, 'vsn:seqnum'.
	 * For example, "hcvol1:2710".
	 */
	metaBuf = DiskVolsGenMetadataArchiveId(ve->Vi.VfVsn, seqnum, metaBuf);

	/* Add archive id. */
	hcerr = hc_nvr_add_from_string(nvr, HONEYCOMB_METADATA_ARCHID,
	    (hc_string_t)metaBuf);
	if (hcerr != HCERR_OK) {
		LibFatal(hc_nvr_add_from_string, metaBuf);
	}
	Trace(TR_DEBUG, "HC metadata %s='%s'", HONEYCOMB_METADATA_ARCHID,
	    metaBuf);

	/* Add file name. */
	if (strlen(File->f->FiName) > HONEYCOMB_METADATA_STRINGLEN-1) {
		int tailLength;

		filename = File->f->FiName;
		memset(fn, 0, HONEYCOMB_METADATA_STRINGLEN);
		tailLength = strlen(File->f->FiName) -
		    (HONEYCOMB_METADATA_STRINGLEN-1);
		memmove(fn, filename + tailLength,
		    HONEYCOMB_METADATA_STRINGLEN-1);

		filename = fn;
		Trace(TR_DEBUG, "\tTruncating file name to '%s' (%d)",
		    filename, strlen(filename));
	} else {
		filename = File->f->FiName;
	}
	hcerr = hc_nvr_add_from_string(nvr, HONEYCOMB_METADATA_FILENAME,
	    (hc_string_t)filename);
	if (hcerr != HCERR_OK) {
		LibFatal(hc_nvr_add_from_string, File->f->FiName);
	}
	Trace(TR_DEBUG, "\t%s='%s'", HONEYCOMB_METADATA_FILENAME, filename);

	/* Add mod time. */
	hcerr = hc_nvr_add_time(nvr, HONEYCOMB_METADATA_MODTIME,
	    File->f->FiModtime);
	if (hcerr != HCERR_OK) {
		LibFatal(hc_nvr_add_from_string, HONEYCOMB_METADATA_MODTIME);
	}
	(void) TimeToIsoStr(File->f->FiModtime, ts);
	Trace(TR_DEBUG, "\t%s='%s'", HONEYCOMB_METADATA_MODTIME, ts);
}


/*
 * Process out of disk space.
 */
static void
outOfSpace(void)
{
	struct VolsEntry *ve;

	ve = &VolsTable->entry[VolCur];
	/* Volume full: %s.%s: %s bytes written */
	SendCustMsg(HERE, 4043, ve->Vi.VfMtype, ve->Vi.VfVsn,
	    CountToA(VolBytesWritten));
}


/*  keep lint happy */
#if defined(lint)
void
hcswapdummy(
	void *buf)
{
	sam_bswap2(buf, 1);
	sam_bswap4(buf, 1);
	sam_bswap8(buf, 1);
}
#endif /* defined(lint) */
