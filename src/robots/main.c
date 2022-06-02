/*
 * main.c main routine for robot shepherd process.
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

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#define	ROBOTS_MAIN
#define	MAIN
#define	NEWALARM

#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/param.h"
#include "aml/shm.h"
#include "sam/devnm.h"
#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "robots.h"

#pragma ident "$Revision: 1.21 $"

/* function prototypes */

int	sigwait(sigset_t *);
void	sig_chld(int);

/* some globals */

int	number_robots, got_sigchld = FALSE;
pid_t	mypid;
char	*fifo_path;
shm_alloc_t master_shm, preview_shm;

int
main(int argc, char **argv)
{
	int		what_signal;
	sigset_t	signal_set, full_block_set;
	struct sigaction	sig_action;
	shm_ptr_tbl_t		*shm_ptr_tbl;

	CustmsgInit(1, NULL);
	if (argc != 3)
		exit(1);

	argv++;
	master_shm.shmid = atoi(*argv);
	argv++;
	preview_shm.shmid = atoi(*argv);

	mypid = getpid();
	if ((master_shm.shared_memory = shmat(master_shm.shmid, NULL, 0774)) ==
	    (void *) -1)
		exit(2);

	if ((preview_shm.shared_memory =
	    shmat(preview_shm.shmid, NULL, 0774)) ==
	    (void *) -1)
		exit(3);

	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;

	fifo_path = strdup(SHM_REF_ADDR(shm_ptr_tbl->fifo_path));

	(void) sigfillset(&full_block_set);	/* used to block all signals */

	(void) sigemptyset(&signal_set);	/* signals to except. */
	(void) sigaddset(&signal_set, SIGINT);
	(void) sigaddset(&signal_set, SIGALRM);
	(void) sigaddset(&signal_set, SIGCHLD);

	/* want to restart system calls on excepted signals */
	sig_action.sa_handler = SIG_DFL;
	(void) sigemptyset(&sig_action.sa_mask);
	sig_action.sa_flags = SA_RESTART;

	number_robots = build_pids(shm_ptr_tbl);

	/*
	 * Check if the first pid is a pseudo(SSI) device. If so, then add
	 * sighup to the list of accecpted signals.
	 */
	if (number_robots > 0 && pids->device != NULL) {
		(void) sigdelset(&signal_set, SIGHUP);
		(void) sigaction(SIGHUP, &sig_action, NULL);
	}
	(void) sigaction(SIGINT, &sig_action, NULL);
	(void) sigaction(SIGALRM, &sig_action, NULL);
	sig_action.sa_handler = sig_chld;
	(void) sigaction(SIGCHLD, &sig_action, NULL);

	/* The default mode is to block everything */

	thr_sigsetmask(SIG_SETMASK, &full_block_set, (sigset_t *)NULL);

	if (number_robots > 0)
		start_children(pids);

	for (;;) {
		alarm(20);
		/* wait for a signal */
		what_signal = sigwait(&signal_set);
		/* process the signal */
		switch (what_signal) {
		case SIGALRM:
			if (got_sigchld) {
				/* clean up a dead child */
				reap_child(pids);
				got_sigchld = FALSE;
			}
			if (number_robots > 0)
				check_children(pids);
			break;

		case SIGCHLD:
			reap_child(pids);
			break;

		case SIGINT:
			sam_syslog(LOG_INFO, "robots: shutdown: signal %d",
			    what_signal);
			kill_children(pids);
			exit(0);
			break;

		case SIGHUP:
			sam_syslog(LOG_INFO, "robots: shutdown: signal %d",
			    what_signal);
			kill_children(pids);
			exit(0);
			break;

		default:
#if defined(DEBUG)
			sam_syslog(LOG_DEBUG, "Unknown signal %#x.",
			    what_signal);
#endif
			break;
		}
	}
}

void
sig_chld(int stuff)
{
#if defined(DEBUG)
	sam_syslog(LOG_DEBUG, "sig-chld(func):%s:%d.\n", __FILE__, __LINE__);
#endif
	got_sigchld = TRUE;
}
