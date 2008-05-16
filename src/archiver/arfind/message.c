/*
 * message.c - Arfind message server.
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

#pragma ident "$Revision: 1.32 $"

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
#include "sam/udscom.h"

/* Local headers. */
#define	ARFIND_PRIVATE
#include "arfind.h"

/* Server functions. */
static void *archreqDone(void *arg, struct UdsMsgHeader *hdr);
static void *changeState(void *arg, struct UdsMsgHeader *hdr);
static void *control(void *arg, struct UdsMsgHeader *hdr);

static void srvrLogit(char *msg);

static struct UdsMsgProcess Table[] = {
	{ control, sizeof (struct AsrControl),
	    sizeof (struct AsrControlRsp) },
	{ archreqDone, sizeof (struct AfrPath),
	    sizeof (struct AfrGeneralRsp) },
	{ changeState, sizeof (struct AfrState),
	    sizeof (struct AfrGeneralRsp) },
};

/* Define the argument buffer to determine size required. */
union argbuf {
	struct AsrControl AsrControl;
	struct AfrPath AfrPath;
	struct AfrState AfrState;
};

static struct UdsServer srvr =
	{ AFSERVER_NAME, AFSERVER_MAGIC, srvrLogit, 0, Table, AFR_MAX,
	sizeof (union argbuf) };

/* Private data. */

static pthread_t thread;

/* Private functions. */
static void stopMessage(void);
static void traceRequest(struct UdsMsgHeader *hdr, struct AfrGeneralRsp *rsp,
		const char *fmt, ...);



/*
 * Thread - Receive arfind service messages.
 */
void *
Message(
	void *arg)
{
	static char pathBuffer[MAXPATHLEN + 4];

	thread = pthread_self();
	snprintf(pathBuffer, sizeof (pathBuffer), AFSERVER_NAME".%s", FsName);
	SamStrdup(srvr.UsServerName, pathBuffer);
	srvr.UsStop = 0;
	ThreadsInitWait(stopMessage, NULL);

	while (Exec < ES_term) {
		if (UdsRecvMsg(&srvr) == -1) {
			if (errno != EINTR) {
				LibFatal(UdsRecvMsg, NULL);
				exit(EXIT_FAILURE);
			}
		}
	}
	Trace(TR_ARDEBUG, "Message() exiting");
	return (arg);
}



/* Server functions. */


/*
 * Archreq done.
 */
static void *
archreqDone(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	struct AfrPath *a = (struct AfrPath *)arg;
	static struct AfrGeneralRsp rsp;

	traceRequest(hdr, &rsp, "archreqDone(%s)", a->AfPath);
	ArchiveRun();
	return (&rsp);
}


/*
 * Change state.
 */
static void *
changeState(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	struct AfrState *a = (struct AfrState *)arg;
	static struct AfrGeneralRsp rsp;

	traceRequest(hdr, &rsp, "changeState(%d)", a->AfState);
	State->AfExec = a->AfState;
	ChangeState(State->AfExec);
	if (Exec == ES_run) {
		ScanfsStartScan();
		ArchiveStartRequests();
	}
	ThreadsWakeup();
	(void) kill(getpid(), SIGALRM);  /* Wakeup signal thread */
	return (&rsp);
}


/*
 * Arfind control.
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
	ThreadsReconfigSync(RC_allow);
	traceRequest(hdr, NULL, "control(%s, %s) %s", a->CtIdent, a->CtValue,
	    controlRsp.CtMsg);
	return (&controlRsp);
}


/* Private functions. */


/*
 * Log a communication error.
 */
static void
srvrLogit(
	char *msg)
{
	Trace(TR_ERR, "%s", msg);
}


/*
 * Stop message thread.
 */
static void
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
	struct AfrGeneralRsp *rsp,
	const char *fmt,		/* printf() style format. */
	...)
{
	char msg[256];

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
