/*
 * main.c - main routine for the ibm3494 library.
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
#include "sam/nl_samfs.h"
#include "aml/sef.h"
#include "aml/sefvals.h"
#include "sam/devnm.h"
#include "sam/custmsg.h"
#include "aml/logging.h"
#define	CC_CODES
#include "ibm3494.h"
#undef CC_CODES
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/trap.h"

#pragma ident "$Revision: 1.27 $"

/* function prototypes */
int sigwait(sigset_t *);
void kill_off_threads(library_t *);
int virtual_shared_check(library_t *library);

/* some globals */
pid_t mypid;
char *fifo_path;
thread_t threads[IBM_MAIN_THREADS]; /* my main threads */
shm_alloc_t master_shm, preview_shm;
const char *program_name = "ibm3494";
int initialized = 0;
extern void common_init(dev_ent_t *un);
extern int using_supported_tape_driver(library_t *library);

/*
 * Struct needed to verify that virtual libraries are shared
 */
struct lib	{
    vsn_t	vsn;
    int		shared;
    int		category;
};

#define	NUM_LIB_INCR	10

int
main(
	int argc,
	char **argv)
{
	int		what_signal, i;
	char		*l_mess, *lc_mess;
	char		logname[20];
	sigset_t	sigwait_set;
	library_t	*library;
	struct sigaction	sig_action;
	dev_ptr_tbl_t		*dev_ptr_tbl;
	shm_ptr_tbl_t		*shm_ptr_tbl;


	if (argc != 4) {
		fprintf(stderr, "Usage: ibm3494 mshmid pshmid equip\n");
		exit(1);
	}

	initialize_fatal_trap_processing(SOLARIS_THREADS, NULL);

	CustmsgInit(1, NULL);

	library = (library_t *)malloc_wait(sizeof (library_t), 2, 0);
	(void) memset(library, 0, sizeof (library_t));

	argv++;
	master_shm.shmid = atoi(*argv);
	argv++;
	preview_shm.shmid = atoi(*argv);
	argv++;
	library->eq = atoi(*argv);
	sprintf(logname, "ibm3494-%d", library->eq);
	program_name = logname;
	mypid = getpid();
	if ((master_shm.shared_memory =
	    shmat(master_shm.shmid, NULL, 0774)) ==
	    (void *)-1)
		exit(1);

	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;

	if ((preview_shm.shared_memory =
	    shmat(preview_shm.shmid, NULL, 0774)) ==
	    (void *)-1)
		exit(2);

	fifo_path = strdup(SHM_REF_ADDR(shm_ptr_tbl->fifo_path));

	dev_ptr_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(((shm_ptr_tbl_t *)
	    master_shm.shared_memory)->dev_table);

	/* LINTED pointer cast may result in improper alignment */
	library->un = (dev_ent_t *)SHM_REF_ADDR(
	    dev_ptr_tbl->d_ent[library->eq]);
	l_mess = library->un->dis_mes[DIS_MES_NORM];
	lc_mess = library->un->dis_mes[DIS_MES_CRIT];

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

	sleep(2);

	mutex_init(&library->mutex, USYNC_THREAD, NULL);

	/* grab the lock and hold it until initialization is complete */
	mutex_lock(&library->mutex);
	common_init(library->un);
	/* start the main threads */
	if (thr_create(NULL, DF_THR_STK, monitor_msg, (void *)library,
	    (THR_BOUND | THR_NEW_LWP | THR_DETACHED),
	    &threads[IBM_MSG_THREAD])) {
		sam_syslog(LOG_CRIT, "Unable to start message thread: %m.\n");
		thr_exit(NULL);
	}

	if (thr_create(NULL, MD_THR_STK, manage_list, (void *)library,
	    (THR_BOUND |THR_NEW_LWP | THR_DETACHED),
	    &threads[IBM_WORK_THREAD])) {
		sam_syslog(LOG_CRIT, "Unable to start worklist thread: %m.\n");
		thr_kill(threads[IBM_MSG_THREAD], SIGINT);
		thr_exit(NULL);
	}

	/*
	 * Have to suspend the delay_resp thread until the library is
	 * initialized. Otherwise the shared flag is not set properly and
	 * the category of imported volumes will not be set correctly. Have
	 * to call thr_continue to get delay_resp running after the library
	 * is initialized.
	 */

	if (thr_create(NULL, MD_THR_STK, delay_resp, (void *)library,
	    (THR_BOUND | THR_DETACHED | THR_NEW_LWP |
	    THR_SUSPENDED), &threads[IBM_DELAY_THREAD])) {
		sam_syslog(LOG_CRIT, "Unable to start worklist thread: %m.\n");
		thr_kill(threads[IBM_MSG_THREAD], SIGINT);
		thr_kill(threads[IBM_WORK_THREAD], SIGINT);
		thr_exit(NULL);
	}

	mutex_lock(&library->un->mutex);
	library->un->dt.rb.process = getpid();
	library->un->status.b.ready = FALSE;
	library->un->status.b.present = FALSE;
	library->un->status.b.scanning = TRUE;
	mutex_unlock(&library->un->mutex);


	/* Initialize the library. Initialize will release the library mutex. */
	if (initialize(library, dev_ptr_tbl))
		thr_exit(NULL);

	initialized = 1;

	/*
	 * verify the drives are using the tape driver from solaris driver
	 * and
	 * If there are more than one virtual library, then all virtual
	 * libraries must be in shared mode and have different categories.
	 */
	if (using_supported_tape_driver(library) ||
	    virtual_shared_check(library)) {
		thr_kill(threads[IBM_WORK_THREAD], SIGINT);
		thr_kill(threads[IBM_DELAY_THREAD], SIGINT);
		sleep(3);
		thr_kill(threads[IBM_MSG_THREAD], SIGINT);
		exit(0);
	}

	/* Let delay_resp thread run */
	thr_continue(threads[IBM_DELAY_THREAD]);

	thr_yield();			  /* let the other threads run */

	mutex_lock(&library->mutex);
	i = 20;
	while (library->countdown && i-- > 0) {
		sprintf(l_mess, "Waiting for %d drive%s to initialize.",
		    library->countdown, library->countdown > 1 ? "s" : "");
		sam_syslog(LOG_INFO, "Waiting for %d drive%s to initialize.",
		    library->countdown, library->countdown > 1 ? "s" : "");
		mutex_unlock(&library->mutex);

		sleep(10);

		mutex_lock(&library->mutex);
	}

	if (i <= 0)
		sam_syslog(LOG_INFO,
		    "(%d) drive(s) did not initialize.", library->countdown);

	mutex_unlock(&library->mutex);

	mutex_lock(&library->un->mutex);
	library->un->status.b.audit = FALSE;
	library->un->status.b.requested = FALSE;
	library->un->status.b.mounted = TRUE;
	library->un->status.b.ready = TRUE;
	mutex_unlock(&library->un->mutex);

	memccpy(l_mess, "Reconcile catalog.", '\0', DIS_MES_LEN);

	if (thr_create(NULL, DF_THR_STK, reconcile_catalog,
	    (void *)library, (THR_BOUND | THR_DETACHED), NULL)) {
		sam_syslog(LOG_CRIT,
		    "Unable to start reconcile_catalog thread: %m.\n");
		mutex_lock(&library->un->mutex);
		library->un->status.b.scanning = FALSE;
		mutex_unlock(&library->un->mutex);
	}

	/*
	 * Now that the daemon is fully initialized,
	 * the main thread is just used
	 * to monitor the thread state and an indication of shutdown.  This is
	 * accomplished using the signals SIGALRM, SIGINT, and SIGTERM.  This is
	 * not done with a signal handler, but using the sigwait() call.
	 */
	sigemptyset(&sigwait_set);
	sigaddset(&sigwait_set, SIGINT);
	sigaddset(&sigwait_set, SIGTERM);
	sigaddset(&sigwait_set, SIGALRM);

	sig_action.sa_handler = SIG_DFL; /* want to restart system calls */
	sigemptyset(&sig_action.sa_mask);
	sig_action.sa_flags = SA_RESTART;

	sigaction(SIGINT, &sig_action, NULL);
	sigaction(SIGTERM, &sig_action, NULL);
	sigaction(SIGALRM, &sig_action, NULL);

	for (;;) {
		alarm(20);
		what_signal = sigwait(&sigwait_set); 	/* wait for a signal */

		switch (what_signal) {		/* process the signal */
		case SIGALRM:
			if ((threads[IBM_MSG_THREAD] == (thread_t)-1) ||
			    (threads[IBM_WORK_THREAD] == (thread_t)-1) ||
			    (threads[IBM_DELAY_THREAD] == (thread_t)-1)) {
				/*
				 * If any of the processing threads have
				 * disappeared, log the
				 * fact, and take a core dump
				 */
				sam_syslog(LOG_INFO, "Exit: Thread(s) gone.\n");
				abort();
			}
			break;

		/*
		 * For a normal shutdown of the robot daemon:
		 *  1) prevent the alarm from going off during shutdown and
		 * causing a core dump
		 *  2) log the reason we are shutting down
		 *  3) terminate all of the processing threads
		 *  4) terminate the connection to the catalog
		 */
		case SIGINT:
		case SIGTERM:
			sigdelset(&sigwait_set, SIGALRM);
			sam_syslog(LOG_INFO, "Shutdown by signal %d",
			    what_signal);
			kill_off_threads(library);
			exit(0);
			break;

		default:
			break;
		}
	}
}


void
kill_off_threads(
	library_t *library)
{
	int	i;

	for (i = 0; i < IBM_MAIN_THREADS; i++)
		if (threads[i] > 0)
			if (thr_kill(threads[i], SIGEMT))
				sam_syslog(LOG_INFO,
				    "Unable to kill thread.\n");
}

int
virtual_shared_read_config(
	struct lib *lib,
	dev_ent_t *device)
{
	FILE	*open_str;
	char	line[300];
	char	*tmp;
	int		i;

	if ((open_str = fopen(device->name, "r")) == NULL) {
	sam_syslog(LOG_CRIT,
	    "Unable to open configuration file: %s.", device->name);
	return (-1);
	}

	lib->category = NORMAL_CATEGORY;
	while (fgets(memset(line, 0, 200), 200, open_str) != NULL) {
		i++;
		if (*line == '#' || strlen(line) == 0 || *line == '\n')
			continue;

		if ((tmp = strchr(line, '#')) != NULL)
			memset(tmp, 0, (200 - (tmp - line)));

		if ((tmp = strtok(line, "= \t\n")) == NULL)
			continue;

		if (strcasecmp(tmp, "name") == 0) {
			if ((tmp = strtok(NULL, "= \t\n")) == NULL) {
				continue;
			}
			if (strlen(tmp) <= sizeof (vsn_t)) {
				memcpy(&lib->vsn, tmp, strlen(tmp) + 1);
			}
			continue;

		} else if (strcasecmp(tmp, "access") == 0) {
			if ((tmp = strtok(NULL, "= \t\n")) == NULL) {
				continue;
			}
			if (strcasecmp(tmp, "private") == 0) {
				lib->shared = FALSE;
			} else if (strcasecmp(tmp, "shared") == 0) {
				lib->shared = TRUE;
			} else {
				fclose(open_str);
				return (-1);
			}
			continue;

		} else if (strcasecmp(tmp, "category") == 0) {
			uint_t l_category;

			if ((tmp = strtok(NULL, "= \t\n")) == NULL) {
				continue;
			}
			l_category = strtol(tmp, NULL, 10);
			if (l_category < 1 || l_category > 0xfeff) {
				fclose(open_str);
				return (-1);
			} else
				lib->category = l_category;
			continue;

		} else if (*tmp == '/') { /* start of an absolute path name */
			continue;
		}
	}
	fclose(open_str);

	if (lib->vsn[0] == '\0') {
		return (-1);
	}
	return (0);
}


int
virtual_shared_verify(
	library_t *library,
	struct lib *libs,
	int *num_libs,
	dev_ent_t *device)
{
	int		i, rc;
	struct lib	*lib = libs + *num_libs;

	/*
	 * Check the config file if it is different from my own.
	 */
	if (strcmp(library->un->name, device->name)) {
		if (virtual_shared_read_config(lib, device)) {
			return (-1);
		}

		if (strcmp(lib->vsn, library->un->vsn)) {
			/* Not interested if different server */
			return (0);
		}

		if (!lib->shared) {
			/* Must be shared */
			sam_syslog(LOG_ERR,
			    "All virtual 3494 libraries must be shared.");
			return (-1);
		}

		for (i = 0; i < *num_libs; i++) {
			/* Must have different category */
			if (lib->category == libs[i].category) {
				sam_syslog(LOG_ERR,
				    "All virtual 3494 libraries must have "
				    "different categories.");
				return (-1);
			}
		}

		(*num_libs)++;
	}
	return (0);
}


int
virtual_shared_check(
	library_t *library)
{
	dev_ent_t	*device;
	struct lib	*libs = NULL;
	int		num_libs_allocated = 0;
	int		num_libs = 0;
	struct lib	*newlibs;
	int		size;

	for (device = (dev_ent_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);
	    device != NULL;
	    device = (dev_ent_t *)SHM_REF_ADDR(device->next)) {
		if (device->type == DT_IBMATL) {
			/*
			 * Allocate more libs if no libs left
			 */
			if (num_libs_allocated == num_libs) {
				size = (num_libs_allocated + NUM_LIB_INCR) *
				    sizeof (struct lib);
				newlibs = (struct lib *)malloc_wait(size, 2, 0);
				memset(newlibs, 0, size);
				if (libs) {
					memmove(newlibs, libs,
					    num_libs_allocated *
					    sizeof (struct lib));
					free(libs);
				}
				libs = newlibs;
				num_libs_allocated += NUM_LIB_INCR;
				/*
				 * Set up my own lib
				 */
				if (num_libs == 0) {
					strcpy(libs[0].vsn, library->un->vsn);
					libs[0].category =
					    library->sam_category;
					libs[0].shared =
					    library->options.b.shared;
					num_libs = 1;
				}
			}
			if (virtual_shared_verify(library, libs, &num_libs,
			    device)) {
				if (libs) {
					free(libs);
				}
				return (-1);
			}
		}
	}
	if (libs) {
		free(libs);
	}
	if (num_libs > 1) {
		if (! library->options.b.shared) {
			sam_syslog(LOG_ERR,
			    "All virtual 3494 libraries must be shared.");
			return (-1);
		}
	}
	return (0);
}
