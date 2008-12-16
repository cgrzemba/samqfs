/*
 * main.c - main routine for the sony library.
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
#include <thread.h>
#include <synch.h>
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

#define	MAIN

#include "driver/samst_def.h"
#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "sam/devinfo.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "sam/defaults.h"
#include "aml/sef.h"
#include "aml/sefvals.h"
#define	DEV_NM_HERE
#include "sam/devnm.h"
#undef DEV_NM_HERE
#include "aml/logging.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"
#include "sony.h"
#include "aml/proto.h"
#include "aml/trap.h"
#include "sam/lib.h"

#pragma ident "$Revision: 1.26 $"

/* function prototypes */

int sigwait(sigset_t *);
void kill_off_threads(library_t *);

/* some globals */

pid_t mypid;
char *fifo_path;
thread_t threads[SONY_MAIN_THREADS]; /* my main threads */
shm_alloc_t master_shm, preview_shm;
const char *program_name = "sony";
static library_t *library;
extern void common_init(dev_ent_t *un);

static void
fatal_cleanup()
{
	if (library->helper_pid > 0) {
		kill(library->helper_pid, 9);
	}
}


int
main(int argc, char **argv)
{
	int			what_signal, i;
	char			*ent_pnt = "main";
	char			logname[20];
	char			*l_mess, *lc_mess;
	sigset_t		sigwait_set;
	struct sigaction	sig_action;
	dev_ptr_tbl_t		*dev_ptr_tbl;
	shm_ptr_tbl_t		*shm_ptr_tbl;
	sam_defaults_t		*defaults;

	if (argc != 4)
		exit(1);

	initialize_fatal_trap_processing(SOLARIS_THREADS, fatal_cleanup);

	CustmsgInit(1, NULL);

	library = (library_t *)malloc_wait(sizeof (library_t), 2, 0);
	(void) memset(library, 0, sizeof (library_t));

	/*
	 * Crack the arguments
	 */
	argv++;
	master_shm.shmid = atoi(*argv);
	argv++;
	preview_shm.shmid = atoi(*argv);
	argv++;
	library->eq = atoi(*argv);
	mypid = getpid();
	if ((master_shm.shared_memory = shmat(master_shm.shmid, NULL, 0774)) ==
	    (void *)-1)
		exit(2);

	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;

	if ((preview_shm.shared_memory =
	    shmat(preview_shm.shmid, NULL, 0774)) == (void *)-1)
		exit(3);

	fifo_path = strdup(SHM_REF_ADDR(shm_ptr_tbl->fifo_path));

	sprintf(logname, "sony-%d", library->eq);
	defaults = GetDefaults();
	openlog(logname, LOG_PID | LOG_NOWAIT, defaults->log_facility);

	dev_ptr_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->dev_table);

	/* LINTED pointer cast may result in improper alignment */
	library->un = (dev_ent_t *)
	    SHM_REF_ADDR(dev_ptr_tbl->d_ent[library->eq]);
	/* LINTED pointer cast may result in improper alignment */
	library->help_msg = (sony_priv_mess_t *)
	    SHM_REF_ADDR(library->un->dt.rb.private);
	l_mess = library->un->dis_mes[DIS_MES_NORM];
	lc_mess = library->un->dis_mes[DIS_MES_CRIT];

	/* check if we should log sef data */
	(void) sef_status();

	if (DBG_LVL(SAM_DBG_RBDELAY)) {
		int ldk = 60;

		sam_syslog(LOG_DEBUG, "Waiting for 60 seconds.");
		while (ldk > 0 && DBG_LVL(SAM_DBG_RBDELAY)) {
			sprintf(lc_mess, "waiting for %d seconds pid %d",
			    ldk, mypid);
			sleep(10);
			ldk -= 10;
		}
		*lc_mess = '\0';
	}

	mutex_init(&library->mutex, USYNC_THREAD, NULL);

	/*
	 * Hold the lock until initialization is complete
	 */
	mutex_lock(&library->mutex);
	common_init(library->un);
	mutex_init(&library->help_msg->mutex, USYNC_PROCESS, NULL);
	cond_init(&library->help_msg->cond_i, USYNC_PROCESS, NULL);
	cond_init(&library->help_msg->cond_r, USYNC_PROCESS, NULL);
	library->help_msg->mtype = SONY_PRIV_VOID;

	/*
	 * Start the main threads
	 */
	if (thr_create(NULL, DF_THR_STK, monitor_msg, (void *)library,
	    (THR_BOUND | THR_NEW_LWP | THR_DETACHED),
	    &threads[SONY_MSG_THREAD])) {
		sam_syslog(LOG_ERR,
		    "Unable to start thread monitor_msg: %m.\n");
		thr_exit(NULL);
	}

	if (thr_create(NULL, MD_THR_STK, manage_list, (void *)library,
	    (THR_BOUND |THR_NEW_LWP | THR_DETACHED),
	    &threads[SONY_WORK_THREAD])) {
		sam_syslog(LOG_ERR,
		    "Unable to start thread manage_list: %m.\n");
		thr_kill(threads[SONY_MSG_THREAD], SIGINT);
		thr_exit(NULL);
	}

	mutex_lock(&library->un->mutex);
	library->un->dt.rb.process = getpid();
	library->un->status.b.ready = FALSE;
	library->un->status.b.present = FALSE;
	mutex_unlock(&library->un->mutex);

	/*
	 * Initialize the library. This will release the library mutex.
	 */
	memccpy(l_mess, catgets(catfd, SET, 9065, "initializing"),
	    '\0', DIS_MES_LEN);
	if (initialize(library, dev_ptr_tbl))
		thr_exit(NULL);

	/*
	 * Now let the other threads run
	 */
	thr_yield();

	mutex_lock(&library->mutex);
	i = 30;

	{
		char *MES_9155 = catgets(catfd, SET, 9155,
		    "waiting for %d drives to initialize");
		char *mes = (char *)malloc_wait(strlen(MES_9155) + 15, 5, 0);

		while (library->countdown && i-- > 0) {
			sprintf(mes, MES_9155, library->countdown);
			memccpy(l_mess, mes, '\0', DIS_MES_LEN);
			sam_syslog(LOG_INFO, catgets(catfd, SET, 9156,
			    "%s: Waiting for %d drives to initialize."),
			    ent_pnt, library->countdown);
			mutex_unlock(&library->mutex);
			sleep(10);
			mutex_lock(&library->mutex);
		}
		free(mes);
	}

	if (i <= 0)
		sam_syslog(LOG_INFO,
		    catgets(catfd, SET, 9157,
		    "%s: %d drive(s) did not initialize."),
		    ent_pnt, library->countdown);

	mutex_unlock(&library->mutex);
	memccpy(l_mess, "running", '\0', DIS_MES_LEN);
	mutex_lock(&library->un->mutex);
	library->un->status.b.audit = FALSE;
	library->un->status.b.requested = FALSE;
	library->un->status.b.mounted = TRUE;
	library->un->status.b.ready = TRUE;
	mutex_unlock(&library->un->mutex);

	/*
	 * Now that the daemon is fully initialized, the main thread is
	 * just used to monitor the thread state and an indication of shutdown.
	 * This is accomplished using the signals SIGALRM, SIGINT, and SIGTERM.
	 * This is not done with a signal handler, but using the sigwait() call.
	 */
	sigemptyset(&sigwait_set);
	sigaddset(&sigwait_set, SIGINT);
	sigaddset(&sigwait_set, SIGTERM);
	sigaddset(&sigwait_set, SIGALRM);

	/* want to restart system calls */
	sig_action.sa_handler = SIG_DFL;
	sigemptyset(&sig_action.sa_mask);
	sig_action.sa_flags = SA_RESTART;

	sigaction(SIGINT, &sig_action, NULL);
	sigaction(SIGTERM, &sig_action, NULL);
	sigaction(SIGALRM, &sig_action, NULL);
	for (;;) {
		alarm(20);
		what_signal = sigwait(&sigwait_set);
		switch (what_signal) {

			case SIGALRM:
				if ((threads[SONY_MSG_THREAD] ==
				    (thread_t)-1) ||
				    (threads[SONY_WORK_THREAD] ==
				    (thread_t)-1)) {
					/*
					 * If any of the processing threads
					 * have disappeared, log the
					 * fact, and take a core dump
					 */
					sam_syslog(LOG_INFO,
					    "%s: SIGALRM: Thread(s) gone.",
					    ent_pnt);
					abort();
				}
				break;

			/*
			 * For a normal shutdown of the robot daemon:
			 *  1) prevent the alarm from going off during
			 *    shutdown and causing a core dump
			 *  2) log the reason we are shutting down
			 *  3) kill the helper pid if there is one
			 *  4) terminate the connection to the catalog
			 *  5) terminate all of the processing threads
			 */
			case SIGINT:
			case SIGTERM:
				sigdelset(&sigwait_set, SIGALRM);
				sam_syslog(LOG_INFO,
				    "%s: Shutdown by signal %d",
				    ent_pnt, what_signal);
				if (library->helper_pid > 0) {
					kill(library->helper_pid, 9);
				}
				kill_off_threads(library);
				exit(0);
				break;

			default:
				break;
		}
	}
}


void
kill_off_threads(library_t *library)
{
	int i;

	for (i = 0; i < SONY_MAIN_THREADS; i++)
		if (threads[i] > 0)
			if (thr_kill(threads[i], SIGKILL) &&
			    DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_INFO,
				    "kill_off_threads: Unable to kill"
				    " thread %d.\n", i);
}
