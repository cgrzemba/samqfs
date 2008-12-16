/*
 * server.c - fsd UDS server.
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

#pragma ident "$Revision: 1.20 $"

static char *_SrcFile = __FILE__;
/* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#ifdef sun
#include <limits.h>
#endif /* sun */
#ifdef linux
#include <linux/limits.h>
#endif /* linux */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/* POSIX headers. */
#include <sys/types.h>
#include <sys/param.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

/* Solaris headers. */
#include <pthread.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/exit.h"
#define	FSD_PRIVATE
#include "aml/fsd.h"
#include "sam/lib.h"
#include "sam/udscom.h"

/* Local headers. */
#include "fsd.h"

/* Server functions. */
static void *notify(void *arg, struct UdsMsgHeader *hdr);

static void srvrLogit(char *msg);

static struct UdsMsgProcess Table[] = {
	{ notify, sizeof (struct FsdNotify), sizeof (struct FsdGeneralRsp) },
};

/* Define the argument buffer to determine size required. */
union argbuf {
	struct FsdNotify FsdNotify;
};

static struct UdsServer srvr =
	{ SERVER_NAME, SERVER_MAGIC, srvrLogit, 0, Table, FSD_MAX,
	sizeof (union argbuf) };

static pthread_t thread;

/* Private functions. */
static void *receiveMsgs(void *arg);
static void traceRequest(struct UdsMsgHeader *hdr, struct FsdGeneralRsp *rsp,
		const char *fmt, ...);


void
ServerInit(void)
{
	if (pthread_create(&thread, NULL, receiveMsgs, NULL) != 0) {
		LibFatal(pthread_create, "receiveMessages");
	}
}


/*
 * Receive messages thread.
 */
/*ARGSUSED0*/
static void *
receiveMsgs(void *arg)
{
	sigset_t sig_set;

	/*
	 * Block all signals.
	 */
	sigfillset(&sig_set);
	thr_sigsetmask(SIG_SETMASK, &sig_set, NULL);

	srvr.UsStop = 0;
	Trace(TR_MISC, "Receiving messages.");
	if (UdsRecvMsg(&srvr) == -1) {
		if (errno != EINTR) {
			LibFatal(UdsRecvMsg, NULL);
			exit(EXIT_FAILURE);
		}
	}
	thr_exit(NULL);
	/* NORETURN like exit() */
/*LINTED function falls off bottom without returning value */
}


/*
 * execute user configurable program
 */
static void *
notify(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct {
		char	*name;
		int		priority;
	} syslogPriorities[] = {
	{ "info", LOG_INFO },
	{ "emerg", LOG_EMERG },
	{ "alert", LOG_ALERT },
	{ "crit", LOG_CRIT },
	{ "err", LOG_ERR },
	{ "warning", LOG_WARNING },
	{ "notice", LOG_NOTICE },
	{ "debug", LOG_DEBUG },
	{ NULL }
	};
	struct FsdNotify *a = (struct FsdNotify *)arg;
	static struct FsdGeneralRsp rsp;

	pid_t	pid;
	char	*argv[7];
	char	pid_str[20];
	char	num_str[20];
	int		i;

	/*
	 * Check the execute file availability.
	 */
	rsp.GrStatus = 0;
	rsp.GrErrno = 0;
	if (access(a->FnFname, X_OK) == -1) {
		Trace(TR_ERR, "From: %s, Cannot access notify file %s",
		    hdr->UhName,
		    a->FnFname);
		Trace(TR_MISC, "  Message %d: %s", a->FnMsgNnum, a->FnMsg);
		rsp.GrStatus = -1;
		rsp.GrErrno = errno;
		return (&rsp);
	}

	/*
	 * Prepare arguments.
	 */
	argv[0] = a->FnFname;
	argv[1] = hdr->UhName;
	snprintf(pid_str, sizeof (pid_str), "%ld", hdr->UhPid);
	argv[2] = pid_str;
	argv[3] = "info";
	for (i = 0; syslogPriorities[i].name != NULL; i++) {
		if (a->FnPriority == syslogPriorities[i].priority) {
			argv[3] = syslogPriorities[i].name;
			break;
		}
	}
	snprintf(num_str, sizeof (num_str), "%d", a->FnMsgNnum);
	argv[4] = num_str;
	argv[5] = a->FnMsg;
	argv[6] = NULL;

	/*
	 * Set non-standard files to close on exec.
	 */
	for (i = STDERR_FILENO + 1; i < OPEN_MAX; i++) {
		(void) fcntl(i, F_SETFD, FD_CLOEXEC);
	}

	if ((pid = fork1()) < 0) {
		LibFatal(fork1, a->FnFname);
	}

	if (pid == 0) {	/* Child. */
		execv(argv[0], argv);
		TracePid = getpid();
		ErrorExitStatus = EXIT_NORESTART;
		LibFatal(exec, argv[0]);
		/*NOTREACHED*/
	}

	errno = 0;
	traceRequest(hdr, &rsp, "Notify(%s, %s, %s, %s, %s, %s)", argv[0],
	    argv[1], argv[2], argv[3], argv[4], argv[5]);
	return (&rsp);
}


						/* Private functions. */



/*
 * Log a communication error.
 */
static void
srvrLogit(char *msg)
{
	Trace(TR_ERR, "%s", msg);
}


/*
 * Trace server request.
 * Returns errno to caller.
 */
static void
traceRequest(
	struct UdsMsgHeader *hdr,
	struct FsdGeneralRsp *rsp,
	const char *fmt,			/* printf() style format. */
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
	TraceName = hdr->UhName;
	TracePid = hdr->UhPid;
	if (rsp != NULL && rsp->GrStatus == -1) {
		rsp->GrErrno = errno;
		_Trace(TR_err, hdr->UhSrcFile, hdr->UhSrcLine, "%s", msg);
	} else {
		_Trace(TR_misc, hdr->UhSrcFile, hdr->UhSrcLine, "%s", msg);
	}
	TraceName = program_name;
	TracePid = getpid();
}
