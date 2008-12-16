/*
 * trap.c - Fatal trap processing functions.
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

#pragma ident "$Revision: 1.14 $"

/* Feature test switches. */

/* ANSI C headers. */
#include <errno.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>
#include <siginfo.h>

/* Solaris headers. */
#include <sys/syslog.h>
#include <thread.h>
#include <ucontext.h>

/* SAM-FS headers. */
#include "sam/lib.h"
#include "aml/trap.h"

static int fatal_signal[] = {
	SIGILL,
	SIGIOT,
	SIGABRT,
	SIGEMT,
	SIGFPE,
	SIGBUS,
	SIGSEGV,
	0
};

static void (*cleanup_function)() = NULL;

static void
process_fatal_trap(int sig, siginfo_t *info, void *foo)
{
	struct sigaction sig_action;

	if (info) {
		sam_syslog(LOG_INFO, "fatal signal %d, code: %d, address: %#x",
		    sig, info->si_code, info->si_addr);
	} else {
		sam_syslog(LOG_INFO, "fatal signal %d", sig);
	}

	if (cleanup_function) {
		(*cleanup_function)();
	}

	/* Reset the signal to take the default action (a core dump) */
	sig_action.sa_handler = SIG_DFL;
	sigemptyset(&sig_action.sa_mask);
	sig_action.sa_flags = 0;
	sigaction(sig, &sig_action, NULL);

	/* Now for the time warp..... */
	setcontext((ucontext_t *)foo);

	/* Never got here */
}



/*
 * This function sets up the signal handler for processing fatal traps.  For
 * the correct trap processing, this function must be called once from the main
 * thread of a process before any other threads are spawned.  This function
 * sets up a signal handler for fatal traps, sets the signal mask appropriate
 * to multithreading type, and registers the cleanup function.
 *
 * Should any thread want to add other signal handlers, they should only
 * use modify the signal mask using a "how" value of SIG_UNBLOCK.
 *
 * Arguments:
 *   thread_type: The type of multithreading the process utilizes.
 *   cleanup:     A pointer to cleanup routine to be invoked before process
 *                termination and core dump.  A NULL pointer indicates
 *                no cleanup needs to be done.
 * Returns:
 *   0:        Initialization was successful.
 *   non-zero: An errno indicating why initialization failed.
 */
int
initialize_fatal_trap_processing(int thread_type, void (*cleanup)())
{
	int retval = 0;
	int i;
	sigset_t fatal_signal_set;
	struct sigaction fatal_sig_action;


	cleanup_function = cleanup;

	fatal_sig_action.sa_handler = process_fatal_trap;
	sigemptyset(&fatal_sig_action.sa_mask);
	fatal_sig_action.sa_flags = SA_SIGINFO;

	sigfillset(&fatal_signal_set);
	for (i = 0; fatal_signal[i] != 0; i++) {
		sigdelset(&fatal_signal_set, fatal_signal[i]);
		sigaction(fatal_signal[i], &fatal_sig_action, NULL);
	}

	switch (thread_type) {
	case NOT_THREADED:
		retval = sigprocmask(SIG_SETMASK, &fatal_signal_set, NULL);
		break;
	case SOLARIS_THREADS:
		retval = thr_sigsetmask(SIG_SETMASK, &fatal_signal_set, NULL);
		break;
	case POSIX_THREADS:
		retval = pthread_sigmask(SIG_SETMASK, &fatal_signal_set, NULL);
		break;
	default:
		break;
	}
	return (retval);
}
