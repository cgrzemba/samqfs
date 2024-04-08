/*
 * message.c - Archiver message server.
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

#pragma ident "$Revision: 1.63 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/lib.h"
#include "sam/mount.h"
#include "sam/udscom.h"

/* Local headers. */
#include "archiverd.h"
#include "device.h"
#include "volume.h"

/* Server functions. */
static void *catalogChange(void *arg, struct UdsMsgHeader *hdr);
static void *control(void *arg, struct UdsMsgHeader *hdr);
static void *dequeueArchReq(void *arg, struct UdsMsgHeader *hdr);
static void *fsMount(void *arg, struct UdsMsgHeader *hdr);
static void *fsUmount(void *arg, struct UdsMsgHeader *hdr);
static void *queueArchReq(void *arg, struct UdsMsgHeader *hdr);
static void *requestVolume(void *arg, struct UdsMsgHeader *hdr);
static void *rmState(void *arg, struct UdsMsgHeader *hdr);
static void *volStatus(void *arg, struct UdsMsgHeader *hdr);

static void srvrLogit(char *msg);

static struct UdsMsgProcess Table[] = {
	{ control,
	    sizeof (struct AsrControl), sizeof (struct AsrControlRsp) },
	{ catalogChange,
	    0, sizeof (struct AsrGeneralRsp) },
	{ dequeueArchReq,
	    sizeof (struct AsrArchReq), sizeof (struct AsrGeneralRsp) },
	{ fsMount,
	    sizeof (struct AsrFsname), sizeof (struct AsrGeneralRsp) },
	{ fsUmount,
	    sizeof (struct AsrFsname), sizeof (struct AsrGeneralRsp) },
	{ queueArchReq,
	    sizeof (struct AsrArchReq), sizeof (struct AsrGeneralRsp) },
	{ requestVolume,
	    sizeof (struct AsrRequestVolume), sizeof (struct AsrGeneralRsp) },
	{ rmState,
	    sizeof (struct AsrRmState), sizeof (struct AsrGeneralRsp) },
	{ volStatus,
	    sizeof (struct AsrVolStatus), sizeof (struct AsrGeneralRsp) },
};

/* Define the argument buffer to determine size required. */
union argbuf {
	struct AsrArchReq AsrArchReq;
	struct AsrControl AsrControl;
	struct AsrFsname AsrFsname;
	struct AsrRequestVolume AsrRequestVolume;
	struct AsrRmState AsrRmState;
	struct AsrVolStatus AsrVolStatus;
};

static struct UdsServer srvr =
	{ SERVER_NAME, SERVER_MAGIC, srvrLogit, 0, Table, ASR_MAX,
	sizeof (union argbuf) };

static pthread_t thread;

/* Private functions. */
static struct FileSysEntry *findFileSys(char *fsname);
static void traceRequest(struct UdsMsgHeader *hdr, struct AsrGeneralRsp *rsp,
		const char *fmt, ...);
static void stopMessage(void);


/*
 * Thread - Receive archiver service messages.
 */
void *
Message(
	void *arg)
{
	thread = pthread_self();
	srvr.UsStop = 0;
	ThreadsInitWait(stopMessage, NULL);

	while (AdState->AdExec < ES_term) {
		if (UdsRecvMsg(&srvr) == -1) {
			if (errno != EINTR) {
				char msg[80];
				snprintf(msg,80,"%s: %s", srvr.UsServerName,strerror(errno));
				LibFatal(UdsRecvMsg, msg);
				exit(EXIT_FAILURE);
			}
		}
	}
	Trace(TR_ARDEBUG, "Message() exiting");
	return (arg);
}


/*
 * Wait for message ready.
 */
void
MessageReady(void)
{
	int	n;

	for (n = 0; n < 10; n++) {
		if (ArchiverCatalogChange() == 0) {
			return;
		}
		Trace(TR_ARDEBUG, "MessageReady()");
		ThreadsSleep(1);
	}
	LibFatal(message, "Not ready");
}


/*
 * Return an ArchReq to arfind.
 */
void
MessageReturnQueueEntry(
	struct QueueEntry *qe)
{
	static char arname[ARCHREQ_NAME_SIZE];
	struct ArchReq *ar;

	ar = qe->QeAr;
	QueueFree(qe);
	(void) ArchReqName(ar, arname);
	ar->ArState = ARS_done;
	(void) ArfindArchreqDone(ar->ArFsname, arname);
	if (ArchReqDetach(ar) == -1) {
		Trace(TR_ERR, "ArchReqDetach(%s) failed", arname);
	}
}


/* Server functions. */


/*
 * Catalog change.
 */
static void *
catalogChange(
/* LINTED argument unused in function */
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct AsrGeneralRsp rsp;

	rsp.GrStatus = 0;
	traceRequest(hdr, &rsp, "catalogChange");
	ScheduleRun("catalogChange");
	return (&rsp);
}


/*
 * Archiver control.
 */
static void *
control(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	struct AsrControl *a = (struct AsrControl *)arg;
	static struct AsrControlRsp controlRsp;

	ThreadsReconfigSync(RC_wait);
	strncpy(controlRsp.CtMsg, Control(a->CtIdent, a->CtValue),
	    sizeof (controlRsp.CtMsg));
	traceRequest(hdr, NULL, "control(%s, %s) %s", a->CtIdent, a->CtValue,
	    controlRsp.CtMsg);
	return (&controlRsp);
}


/*
 * Dequeue an ArchReq.
 */
static void *
dequeueArchReq(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	struct AsrArchReq *a = (struct AsrArchReq *)arg;
	static struct AsrGeneralRsp rsp;
	static char arname[ARCHREQ_NAME_SIZE];

	snprintf(arname, sizeof (arname), "%s.%s", a->ArFsname, a->ArArname);
	traceRequest(hdr, &rsp, "Dequeue (%s)", arname);
	rsp.GrStatus = -1;
	if (QueueHasArchReq(arname)) {
		if (ScheduleDequeue(arname)) {
			rsp.GrStatus = 0;
			Trace(TR_QUEUE, "Dequeue (%s)", arname);
		} else {
			rsp.GrErrno = EBUSY;
			Trace(TR_QUEUE, "Dequeue (%s) failed", arname);
		}
	} else {
		rsp.GrErrno = EADDRNOTAVAIL;
	}
	return (&rsp);
}


/*
 * Filesystem mounted.
 */
static void *
fsMount(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	struct AsrFsname *a = (struct AsrFsname *)arg;
	static struct AsrGeneralRsp rsp;
	struct FileSysEntry *fs;

	fs = findFileSys(a->ArFsname);
	if (fs == NULL) {
		rsp.GrStatus = -1;
		errno = ENOENT;
	} else {
		rsp.GrStatus = 0;
		FileSysMount(fs);
		(void) kill(getpid(), SIGALRM);	/* Wakeup main loop */
	}
	traceRequest(hdr, &rsp, "fsMount(%s)", a->ArFsname);
	return (&rsp);
}


/*
 * Filesystem unmount requested.
 */
static void *
fsUmount(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	struct AsrFsname *a = (struct AsrFsname *)arg;
	static struct AsrGeneralRsp rsp;
	struct FileSysEntry *fs;


	fs = findFileSys(a->ArFsname);
	if (fs == NULL) {
		rsp.GrStatus = -1;
		errno = ENOENT;
	} else {
		rsp.GrStatus = 0;
		FileSysUmount(fs);
		(void) kill(getpid(), SIGALRM);	/* Wakeup main loop */
	}
	traceRequest(hdr, &rsp, "fsUmount(%s)", a->ArFsname);
	return (&rsp);
}


/*
 * Queue an ArchReq.
 */
static void *
queueArchReq(
	void *arg,
/* LINTED argument unused in function */
	struct UdsMsgHeader *hdr)
{
	struct AsrArchReq *a = (struct AsrArchReq *)arg;
	static char sizebuf[STR_FROM_FSIZE_BUF_SIZE];
	static char arname[ARCHREQ_NAME_SIZE];
	static struct AsrGeneralRsp rsp;
	static struct QueueEntry *qe;
	struct ArchReq *ar;

	rsp.GrStatus = -1;
	rsp.GrErrno = 0;
	snprintf(arname, sizeof (arname), "%s.%s", a->ArFsname, a->ArArname);
	if (QueueHasArchReq(arname)) {
		Trace(TR_MISC, "Already have %s", arname);
		rsp.GrErrno = EADDRINUSE;
		goto out;
	}
	ar = ArchReqAttach(a->ArFsname, a->ArArname, O_RDWR);
	if (ar == NULL) {
		Trace(TR_ERR,
		    "ArchReqAttach(%s.%s) failed", a->ArFsname, a->ArArname);
		rsp.GrErrno = errno;
		goto out;
	}
	if (!ArchReqValid(ar)) {
		rsp.GrErrno = errno;
		(void) ArchReqDetach(ar);
		goto out;
	}
	qe = QueueGetFree();
	ar->ArFlags |= AR_first;
	ar->ArMsgSent = 0;
	ar->ArTimeQueued = time(NULL);
	memset(&ar->ArCpi[0], 0, ar->ArDrives * sizeof (struct ArcopyInstance));
	qe->QeAr = ar;
	strncpy(qe->QeArname, arname, sizeof (qe->QeArname) - 1);
	ComposeEnqueue(qe);
	Trace(TR_QUEUE,
	    "Queue (%s) files: %d space: %s",
	    qe->QeArname, ar->ArFiles,
	    StrFromFsize(ar->ArSpace, 3, sizebuf, sizeof (sizebuf)));
	rsp.GrStatus = 0;

out:
	return (&rsp);
}


/*
 * Request a volume for an arcopy.
 */
static void *
requestVolume(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	struct AsrRequestVolume *a = (struct AsrRequestVolume *)arg;
	static struct AsrGeneralRsp rsp;

	traceRequest(hdr, (struct AsrGeneralRsp *)&rsp, "requestVolume(%s)",
	    a->AsAriname);
	rsp.GrStatus = ScheduleRequestVolume(a->AsAriname, a->AsFileSize);
	return (&rsp);
}


/*
 * Stop removable media archiving.
 */
static void *
rmState(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	struct AsrRmState *a = (struct AsrRmState *)arg;
	static struct AsrGeneralRsp rsp;

	rsp.GrStatus = 0;
	traceRequest(hdr, &rsp, "rmState(%d)", a->state);

	if (a->state != 0) {
		ScheduleSetRmState(EC_amld_start);
		ScheduleRun("rmState(On)");
	} else {
		ScheduleSetRmState(EC_amld_stop);
		ScheduleRun("rmState(Off)");
	}
	return (&rsp);
}


/*
 * Request volume status.
 */
static void *
volStatus(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct AsrGeneralRsp rsp;
	struct AsrVolStatus *a = (struct AsrVolStatus *)arg;

	rsp.GrStatus = ScheduleGetVolStatus(a->mtype, a->vsn);
	traceRequest(hdr, &rsp, "volStatus(%s.%s)", a->mtype, a->vsn);
	return (&rsp);
}


/* Private functions. */


/*
 * Find file system.
 */
static struct FileSysEntry *
findFileSys(
	char *fsname)
{
	int	i;

	ThreadsReconfigSync(RC_wait);
	for (i = 0; i < FileSysTable->count; i++) {
		struct FileSysEntry *fs;

		fs = &FileSysTable->entry[i];
		if (strcmp(fs->FsName, fsname) == 0) {
			return (fs);
		}
	}
	Trace(TR_MISC, "findFileSys(%s) failed", fsname);
	return (NULL);
}


/*
 * Log a communication error.
 */
static void
srvrLogit(
	char *msg)
{
	errno = 0;	/* Allready in message */
	Trace(TR_ERR, "%s", msg);
}


/*
 * Stop Message() thread.
 */
void
stopMessage(void)
{
	srvr.UsStop = 1;
	(void) pthread_kill(thread, SIGUSR1);
}


/*
 * Trace message request.
 * Returns errno to caller.
 */
static void
traceRequest(
	struct UdsMsgHeader *hdr,
	struct AsrGeneralRsp *rsp,
	const char *fmt,		/* printf() style format. */
	...)
{
	char msg[256];

	ThreadsReconfigSync(RC_allow);
	if (fmt != NULL) {
		va_list args;

		va_start(args, fmt);
		vsnprintf(msg, sizeof (msg), fmt, args);
		va_end(args);
	} else {
		*msg = '\0';
	}
	if (rsp != NULL && rsp->GrStatus == -1) {
		rsp->GrErrno = errno;
		_Trace(TR_err, NULL, 0, "From %s[%d] %s:%d %s",
		    hdr->UhName, (int)hdr->UhPid,
		    hdr->UhSrcFile, hdr->UhSrcLine, msg);
	} else {
		_Trace(TR_ardebug, NULL, 0, "From %s[%d] %s:%d %s",
		    hdr->UhName, (int)hdr->UhPid,
		    hdr->UhSrcFile, hdr->UhSrcLine, msg);
	}
}
