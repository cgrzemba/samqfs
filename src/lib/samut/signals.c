/*
 * signals.c - Signal processing thread.
 * The caller can provide:
 * A list of signals and functions for them.
 * A function to call for a fatal error.
 * The path for the ptrace and core files.
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

#pragma ident "$Revision: 1.12 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#define	_POSIX_PTHREAD_SEMANTICS
#if defined(lint)
#undef __PRAGMA_REDEFINE_EXTNAME
#endif

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/exit.h"
#include "sam/lib.h"
#include "sam/sam_trace.h"
#include "sam/signals.h"

#include "sam/lint.h"
#if defined(lint)
#undef sigemptyset
#undef sigfillset
#undef sigaddset
#undef sigdelset
#undef sigprocmask
#endif /* defined(lint) */

/* Private Data. */

/*
 * Fatal signals.
 * These signals are caught and will produce a ptrace and a core file.
 * If supplied in the arguments, a caller supplied function will be called
 * before the ptrace and core file.
 */
static int fatalSignals[] = {
	SIGABRT,
	SIGBUS,
	SIGEMT,
	SIGFPE,
	SIGILL,
	SIGIOT,
	SIGPIPE,
	SIGSEGV,
	SIGTRAP,
	0
};

static struct SignalsArg *signalsArg;		/* Signals() argument */
static sigset_t sigwaitSet;

/* Private functions. */
static void catchSignals(int signum);
static void fatalSignalHandler(int signum);
static void *signalWaiter(void *arg);


/*
 * Thread -  Wait for signals.
 */
pthread_t
Signals(
	struct SignalsArg *arg)
{
	struct sigaction sig_action;
	sigset_t set;
	pthread_t thread;
	int		i;

	signalsArg = arg;
	/*
	 * Block all signals.
	 */
	if (sigfillset(&set) == -1) {
		LibFatal(sigfillset, "");
	}
	if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
		LibFatal(sigprocmask, "SIG_SETMASK");
	}

	/*
	 * Set signals for sigwait() to receive.
	 */
	memset(&sig_action, 0, sizeof (sig_action));
	sig_action.sa_handler = catchSignals;
	sig_action.sa_flags = SA_RESTART;
	if (sigemptyset(&sig_action.sa_mask) == -1) {
		LibFatal(sigemptyset, "");
	}

	/*
	 * Caller's signals.
	 */
	if (signalsArg->SaSignalFuncs != NULL) {
		struct SignalFunc *sf;

		for (sf = signalsArg->SaSignalFuncs; sf->SfSig != 0; sf++) {
			if (sigaddset(&sig_action.sa_mask, sf->SfSig) == -1) {
				LibFatal(sigaddset, "");
			}
			if (sigaction(sf->SfSig, &sig_action, NULL) == -1) {
				LibFatal(sigaction, "");
			}
		}
	}

	/*
	 * Fatal signals.
	 */
	for (i = 0; fatalSignals[i] != 0; i++) {
		if (sigaddset(&sig_action.sa_mask, fatalSignals[i]) == -1) {
			LibFatal(sigaddset, "");
		}
		if (sigaction(fatalSignals[i], &sig_action, NULL) == -1) {
			LibFatal(sigaction, "");
		}
	}
	sigwaitSet = sig_action.sa_mask;
	if (sigprocmask(SIG_UNBLOCK, &sigwaitSet, NULL) == -1) {
		LibFatal(sigprocmask, "unblock");
	}
	errno = pthread_sigmask(SIG_BLOCK, &sigwaitSet, NULL);
	if (errno != 0) {
		LibFatal(pthread_sigmask, "SIG_BLOCK");
	}

	/*
	 *  Create signal waiter thread.
	 */
	errno = pthread_create(&thread, NULL, signalWaiter, NULL);
	if (errno != 0) {
		LibFatal(pthread_create, "signalWaiter");
	}
	Trace(TR_SIG, "signalwait started [%d]", thread);
	return (thread);
}

/* Private functions */

/*
 * Catch any signal.
 * This function should never be executed.  It is used for the sigaction()
 * handler.
 */
static void
catchSignals(
	int sig)
{
#if defined(DEBUG) | defined(lint)
	TraceSignal(sig);
#endif /* defined(DEBUG) | defined(lint) */
}


/*
 * Fatal signal handler.
 */
void
fatalSignalHandler(
	int sig)
{
	char sigDigits[16];
	char *sigName;

	sigName = strsignal(sig);
	if (sigName == NULL) {
		snprintf(sigDigits, sizeof (sigDigits), "%d", sig);
		sigName = sigDigits;
	}
	Trace(TR_ERR, "Fatal signal %s received", sigName);

	if (signalsArg->SaCoreDir != NULL) {
		char	buf[128];

		(void) putenv("LD_PRELOAD=");

		/*
		 * Stack trace.
		 */
		snprintf(buf, sizeof (buf),
		    "/usr/proc/bin/pstack %d | /bin/tee %s/traceback.%d\n",
		    (int)getpid(), signalsArg->SaCoreDir, (int)getpid());
		(void) pclose(popen(buf, "w"));
		Trace(TR_ERR, "Traceback saved to %s/traceback.%d",
		    signalsArg->SaCoreDir, (int)getpid());

		/*
		 * core file.
		 */
		snprintf(buf, sizeof (buf), "/bin/gcore %d\n", (int)getpid());
		(void) pclose(popen(buf, "w"));
		Trace(TR_ERR, "Core dumped to %s/core.%d",
		    signalsArg->SaCoreDir, (int)getpid());
	}

	if (signalsArg->SaFatal != NULL) {
		signalsArg->SaFatal(sig);
	}

	SendCustMsg(HERE, 14250, program_name, sigName);
	exit(EXIT_FATAL);
}


/*
 * Signal waiter.  A separate thread created to wait for
 * masked signals.  Any masked signaled will be delivered to
 * this thread.
 */
static void*
signalWaiter(
	void *arg)
{
	boolean_t run = TRUE;

	/*
	 * Wait for signals, and call functions to process them.
	 */
	while (run) {
		char *sigName;
		int i;
		int sig;

		if ((sigwait(&sigwaitSet, &sig)) < 0) {
			SysError(HERE, "Sigwait failed");
			continue;
		}
		if (run == FALSE) {
			break;
		}
		sigName = strsignal(sig);
		if (sigName != NULL) {
			Trace(TR_SIG, "Signal %s received", sigName);
		} else {
			Trace(TR_SIG, "Signal %d received", sig);
		}
		if (signalsArg->SaSignalFuncs != NULL) {
			struct SignalFunc *sf;

			sf = signalsArg->SaSignalFuncs;
			while (sf->SfSig != 0 && sf->SfSig != sig) {
				sf++;
			}
			if (sf->SfSig != 0) {
				/*
				 * User defined signal.
				 */
				if (sf->SfFunc != NULL) {
					sf->SfFunc(sig);
				}
				continue;
			}
		}

		for (i = 0; fatalSignals[i] != 0; i++) {
			if (sig == fatalSignals[i]) {
				fatalSignalHandler(sig);
			}
		}
	}
	Trace(TR_SIG, "Signals()[%d] exiting", pthread_self());
	return (arg);
#if defined(lint)
/* Just quiet lint for these - _POSIX_PTHREAD_SEMANTICS thingys */
/*NOTREACHED*/
(void) asctime_r(NULL, NULL);
(void) ctime_r(NULL, NULL);
(void) getlogin_r(NULL, 0);
(void) sigwait(NULL, NULL);
(void) ttyname_r(0, NULL, 0);
#endif /* defined(lint) */
}
