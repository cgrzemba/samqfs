/*
 * messages.c - Stager message server.
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

#pragma ident "$Revision: 1.25 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <thread.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/exit.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/udscom.h"
#include "sam/sam_trace.h"
#include "aml/stager.h"
#include "aml/stager_defs.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Local headers. */
#include "stager_lib.h"
#include "copy_defs.h"

#include "stager.h"
#include "schedule.h"
#include "stage_reqs.h"

/* Server functions. */
static void *fsMount(void *arg, struct UdsMsgHeader *hdr);
static void *fsUmount(void *arg, struct UdsMsgHeader *hdr);
static void *stopRm(void *arg, struct UdsMsgHeader *hdr);
static void *logEvent(void *arg, struct UdsMsgHeader *hdr);
static void *control(void *arg, struct UdsMsgHeader *hdr);

static void srvrLogit(char *msg);

static struct UdsMsgProcess Table[] = {
	{ fsMount, sizeof (struct StagerFsname),
		sizeof (struct StagerGeneralRsp) },
	{ fsUmount, sizeof (struct StagerFsname),
		sizeof (struct StagerGeneralRsp) },
	{ stopRm, 0,
		sizeof (struct StagerGeneralRsp) },
	{ logEvent, sizeof (struct StagerLogEvent),
		sizeof (struct StagerGeneralRsp) },
	{ control, sizeof (struct StagerControl),
		sizeof (struct StagerControlRsp) },
};

/* Define the argument buffer to determine size required. */
union argbuf {
	struct StagerFsname fsname;
	struct StagerLogEvent event;
	struct StagerControl control;
};

static struct UdsServer srvr =
	{ STAGER_SERVER_NAME, STAGER_SERVER_MAGIC, srvrLogit, 0, Table, SSR_MAX,
		sizeof (union argbuf) };

/*
 */
pthread_t ReceiveMsgsThreadId;

/* Private functions. */
static void *receiveMsgs(void *arg);
static void traceRequest(struct UdsMsgHeader *hdr,
	struct StagerGeneralRsp *rsp, const char *fmt, ...);


/*
 * Initialize module.
 */
int
InitMessages(void)
{
	int fatal_error;

	fatal_error = pthread_create(&ReceiveMsgsThreadId,
	    NULL, receiveMsgs, NULL);

	return (fatal_error);
}


/*
 * Receive messages thread.
 */
/*ARGSUSED0*/
static void *
receiveMsgs(
	void *arg)
{
	sigset_t sig_set;

	/*
	 * Block all signals.
	 */
	(void) sigfillset(&sig_set);
	(void) thr_sigsetmask(SIG_SETMASK, &sig_set, NULL);

	srvr.UsStop = 0;
	Trace(TR_DEBUG, "Receiving messages");
	for (;;) {
		if (UdsRecvMsg(&srvr) == -1) {
			if (errno != EINTR) {
				LibFatal(UdsRecvMsg, NULL);
				exit(EXIT_FAILURE);
			}
		}
	}
/*LINTED function falls off bottom without returning value */
}


/*
 * Filesystem mounted.
 */
static void *
fsMount(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	struct StagerFsname *a = (struct StagerFsname *)arg;
	static struct StagerGeneralRsp rsp;

	rsp.status = 0;
	traceRequest(hdr, &rsp, "fsMount(%s)", a->fsname);
	MountFileSystem(a->fsname);

	return (&rsp);
}


/*
 * Filesystem unmounted.
 */
static void *
fsUmount(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	struct StagerFsname *a = (struct StagerFsname *)arg;
	static struct StagerGeneralRsp rsp;

	rsp.status = 0;
	traceRequest(hdr, &rsp, "fsUmount(%s)", a->fsname);
	UmountFileSystem(a->fsname);

	return (&rsp);
}


/*
 * Staging event to log file.
 */
static void *
logEvent(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	struct StagerLogEvent *a = (struct StagerLogEvent *)arg;
	static struct StagerGeneralRsp rsp;

	rsp.status = 0;
	traceRequest(hdr, &rsp, "logEvent(%d, %d)", a->type, a->id);
	LogIt(a->type, GetFile(a->id));

	return (&rsp);
}


/*
 * Stop removable media archiving.  Send from sam-amld when
 * a 'samd stop' is issued.
 */
static void *
stopRm(
	/* LINTED argument unused in function */
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct StagerGeneralRsp rsp;

	rsp.status = 0;
	traceRequest(hdr, &rsp, "stopRm");
	ShutdownCopy(SIGTERM);

	return (&rsp);
}

/*
 * Stager control.
 */
static void *
control(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	struct StagerControl *a = (struct StagerControl *)arg;
	static struct StagerControlRsp rsp;

	strncpy(rsp.msg, Control(a->ident, a->value), sizeof (rsp.msg));
	traceRequest(hdr, NULL, "control(%s, %s) %s", a->ident, a->value,
	    rsp.msg);

	return (&rsp);
}

/*
 * Log a communication error.
 */
static void
srvrLogit(
	char *msg)
{
	SetErrno = 0;	/* set for trace */
	Trace(TR_ERR, "%s", msg);
}


/*
 * Trace message request.
 * Returns errno to caller.
 */
static void
traceRequest(
	struct UdsMsgHeader *hdr,
	struct StagerGeneralRsp *rsp,
	const char *fmt,	/* printf() style format. */
	...)
{
	char msg[256];

	if (fmt != NULL) {
		va_list args;

		va_start(args, fmt);
		vsprintf(msg, fmt, args);
		va_end(args);
	} else {
		*msg = '\0';
	}
	TraceName = hdr->UhName;
	TracePid = hdr->UhPid;
	if (rsp != NULL && rsp->status == -1) {
		rsp->error = errno;
		_Trace(TR_err, hdr->UhSrcFile, hdr->UhSrcLine, "%s", msg);
	} else {
		_Trace(TR_debug, hdr->UhSrcFile, hdr->UhSrcLine, "%s", msg);
	}
	TraceName = program_name;
	TracePid = getpid();
}
