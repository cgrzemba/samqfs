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

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sam/spm.h>

#define	ACTION_NONE	0
#define	ACTION_INITIALIZE 1
#define	ACTION_SHUTDOWN   2
#define	ACTION_FORK	3

int action;


#if !defined(lint)
void
sig_int(int signo)
{
	int error;
	char errstr[SPM_ERRSTR_MAX];

	printf("Signal %d\n", signo);
	if (signo == SIGUSR1) {
		action = ACTION_INITIALIZE;
	} else	if (signo == SIGUSR2) {
		action = ACTION_FORK;
	} else	if (signo == SIGHUP) {
		action = ACTION_SHUTDOWN;
	} else	if (signo == SIGABRT) {
		spm_error(&error, errstr);
		fprintf(stderr, "SPM terminated unexpectedly: %d: %s\n",
		    error, errstr);
	}
}

int
main(
	/* LINTED argument unused in function */
	int argc,
	/* LINTED argument unused in function */
	char **argv)
{
	struct sigaction act;
	int error;
	char errstr[SPM_ERRSTR_MAX];

	act.sa_handler = sig_int;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (sigaction(SIGUSR1, &act, NULL)) {
		fprintf(stderr, "sigaction: %s", strerror(errno));
	}
	act.sa_handler = sig_int;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (sigaction(SIGUSR2, &act, NULL)) {
		fprintf(stderr, "sigaction: %s", strerror(errno));
	}
	act.sa_handler = sig_int;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (sigaction(SIGHUP, &act, NULL)) {
		fprintf(stderr, "sigaction: %s", strerror(errno));
	}
	act.sa_handler = sig_int;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (sigaction(SIGABRT, &act, NULL)) {
		fprintf(stderr, "sigaction: %s", strerror(errno));
	}

	action = ACTION_NONE;

	/* LINTED constant in conditional context */
	while (1) {
		switch (action) {
		case ACTION_INITIALIZE:
			printf("Initializing SPM\n");
			if (spm_initialize(SIGABRT, &error, errstr)) {
				fprintf(stderr,
				    "spm_initialize failed: %d: %s\n",
				    error, errstr);
			}
			break;
		case ACTION_SHUTDOWN:
			printf("Shutdown SPM\n");
			if (spm_shutdown(&error, errstr)) {
				fprintf(stderr,
				    "spm_shutdown failed: %d: %s\n",
				    error, errstr);
			}
			break;
		case ACTION_FORK:
			printf("Fork\n");
			fork1();
			break;
		}
		action = ACTION_NONE;

		/* Go to sleep forever, or at leasst until an interrupt */
		select(NULL, NULL, NULL, NULL, NULL);
	}
}
#endif
