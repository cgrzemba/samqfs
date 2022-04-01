/*
 *	main.c - main routine for the scsi libraries.
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

#pragma ident "$Revision: 1.39 $"

static char *_SrcFile = __FILE__;

#include <stdio.h>
#include <thread.h>
#include <synch.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define	MAIN

#include "driver/samst_def.h"
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/lib.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/dev_log.h"
#include "sam/devinfo.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "aml/sef.h"
#include "aml/sefvals.h"
#include "sam/devnm.h"
#include "aml/logging.h"
#include "sam/nl_samfs.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "generic.h"
#include "element.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/trap.h"
#include "aml/tapealert.h"

/*	function prototypes */
int sigwait(sigset_t *);
void kill_off_threads(library_t *);
void check_slot_audits(library_t *);
void start_library_audit(library_t *);


/*	some globals */
pid_t mypid;
char *fifo_path;
char *program_name;
thread_t threads[GENERIC_MAIN_THREADS];	/* my main threads */
shm_alloc_t master_shm, preview_shm;
mutex_t lock_time_mutex;			/* for mktime */
static library_t *library;
extern void common_init(dev_ent_t *un);
extern void set_operator_panel(library_t *library, const int lock);


static void
fatal_cleanup()
{
	if (library->helper_pid > 0) {
		kill(library->helper_pid, 9);
	}
}

/* argv[4]: master_shmid, preview_shmid, library_eq */
int
main(
	int argc,
	char **argv)
{
	int 	what_signal;
	char 	*ent_pnt = "main";
	char 	logname[20];
	char 	*lc_mess;
	sigset_t sigwait_set;
	struct sigaction sig_action;
	dev_ptr_tbl_t *dev_ptr_tbl;
	shm_ptr_tbl_t *shm_ptr_tbl;

	initialize_fatal_trap_processing(SOLARIS_THREADS, fatal_cleanup);

	program_name = "generic";
	library = (library_t *)malloc_wait(sizeof (library_t), 2, 0);
	(void) memset(library, 0, sizeof (library_t));
	(void) memset(&lock_time_mutex, 0, sizeof (mutex_t));
	if (argc != 4)
		exit(1);

	/* Crack arguments (such as they are) e.g.: 4 5 20 */
	argv++;
	master_shm.shmid = atoi(*argv);
	argv++;
	preview_shm.shmid = atoi(*argv);
	argv++;
	library->eq = atoi(*argv);

	sprintf(logname, "genu-%d", library->eq);
	program_name = logname;
	open("/dev/null", O_RDONLY);	/* stdin */
	open("/dev/null", O_RDONLY);	/* stdout */
	open("/dev/null", O_RDONLY);	/* stderr */
	CustmsgInit(1, NULL);
	mypid = getpid();
	if ((master_shm.shared_memory =
	    shmat(master_shm.shmid, NULL, 0774)) == (void *)-1)
		exit(2);

	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
	if ((preview_shm.shared_memory =
	    shmat(preview_shm.shmid, NULL, 0774)) == (void *)-1)
		exit(3);

	fifo_path = strdup(SHM_REF_ADDR(shm_ptr_tbl->fifo_path));

	dev_ptr_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->dev_table);

	/* LINTED pointer cast may result in improper alignment */
	library->un = (dev_ent_t *)SHM_REF_ADDR(
	    dev_ptr_tbl->d_ent[library->eq]);

	library->ele_dest_len = ELEMENT_DESCRIPTOR_LENGTH;
	lc_mess = library->un->dis_mes[DIS_MES_CRIT];


	if (IS_GENERIC_API(library->un->type)) {

		if (sizeof (api_resp_api_t) > sizeof (sam_message_t)) {
			sprintf(lc_mess,
			    "FATAL: API response(%d) larger than message(%d)"
			    " area.",
			    sizeof (api_resp_api_t), sizeof (sam_message_t));
			sam_syslog(LOG_CRIT,
			    "FATAL: API response(%d) larger than message(%d)"
			    " area.",
			    sizeof (api_resp_api_t), sizeof (sam_message_t));
			exit(4);
		}
	}

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

	mutex_init(&library->mutex, USYNC_THREAD, 0);

	/* grab the lock and hold it until initialization is complete */
	mutex_lock(&library->mutex);
	common_init(library->un);

	/* allocate the free list */
	library->free = init_list(ROBO_EVENT_CHUNK);
	library->free_count = ROBO_EVENT_CHUNK;
	mutex_init(&library->free_mutex, USYNC_THREAD, NULL);
	mutex_init(&library->list_mutex, USYNC_THREAD, NULL);
	cond_init(&library->list_condit, USYNC_THREAD, NULL);
	if (IS_GENERIC_API(library->un->type)) {
		library->help_msg = (api_priv_mess_t *)
		    SHM_REF_ADDR(library->un->dt.rb.private);
		mutex_init(&library->help_msg->mutex, USYNC_PROCESS, NULL);
		cond_init(&library->help_msg->cond_i, USYNC_PROCESS, NULL);
		cond_init(&library->help_msg->cond_r, USYNC_PROCESS, NULL);
		library->help_msg->mtype = API_PRIV_VOID;
	}

	/* start the main threads */

	/*
	 * monitor_msg does not wait for the main mutex.
	 * Since initialize elements may wich to talk to the historian,
	 * we must be able to process messages.
	 */
	if (thr_create(NULL, MD_THR_STK, monitor_msg, (void *)library,
	    (THR_BOUND | THR_NEW_LWP | THR_DETACHED),
	    &threads[GENERIC_MSG_THREAD])) {
		sam_syslog(LOG_CRIT,
		    "Unable to create thread monitor_msg: %m.\n");
		exit(2);
	}
	if (thr_create(NULL, (LG_THR_STK), manage_list, (void *)library,
	    (THR_BOUND | THR_NEW_LWP | THR_DETACHED),
	    &threads[GENERIC_WORK_THREAD])) {
		sam_syslog(LOG_INFO, "Unable to create thread manage_list:%m.");
		thr_kill(threads[GENERIC_MSG_THREAD], SIGINT);
		exit(2);
	}
	mutex_lock(&library->un->mutex);
	library->un->dt.rb.process = getpid();
	library->un->status.b.ready = FALSE;
	library->un->status.b.present = FALSE;
	mutex_unlock(&library->un->mutex);

	/* Initialize the library. */
	if (initialize(library, dev_ptr_tbl))
		exit(1);

	mutex_unlock(&library->mutex);	/* release the mutex and stand back */
	thr_yield();			/* let the other threads run */
	if (!library->un->status.b.audit)
		check_slot_audits(library);	/* check for slot audits */
	else {
		mutex_lock(&library->un->mutex);
		library->un->status.b.audit = FALSE;
		mutex_unlock(&library->un->mutex);
		/* Don't audit a tape libraries */
		if (!IS_TAPELIB(library->un))
			start_library_audit(library);
	}

	mutex_lock(&library->un->mutex);
	library->un->status.b.mounted = TRUE;
	library->un->status.b.requested = FALSE;
	if (!library->un->status.b.audit)
		library->un->status.b.ready = TRUE;
	mutex_unlock(&library->un->mutex);

	/*
	 * Now that the daemon is fully initialized, the main thread
	 * is just used to monitor the thread state and an
	 * indication of shutdown.	This is accomplished using the signals
	 * SIGALRM, SIGINT, and SIGTERM.  This is not
	 * done with a signal handler, but using the sigwait() call.
	 */
	sigemptyset(&sigwait_set);
	sigaddset(&sigwait_set, SIGINT);
	sigaddset(&sigwait_set, SIGTERM);
	sigaddset(&sigwait_set, SIGALRM);

	sig_action.sa_handler = SIG_DFL;
	sigemptyset(&sig_action.sa_mask);
	sig_action.sa_flags = SA_RESTART;

	sigaction(SIGINT, &sig_action, NULL);
	sigaction(SIGTERM, &sig_action, NULL);
	sigaction(SIGALRM, &sig_action, NULL);
	for (;;) {
		alarm(20);
		what_signal = sigwait(&sigwait_set);	/* wait for a signal */
		switch (what_signal) {		/* process the signal */
		case SIGALRM:
			if ((threads[GENERIC_MSG_THREAD] == (thread_t)- 1) ||
			    (threads[GENERIC_WORK_THREAD] == (thread_t)- 1)) {
				/*
				 * If any of the processing threads have
				 * disappeared,
				 * log the fact, and take a core dump.
				 */
				sam_syslog(LOG_INFO,
				    "%s: SIGALRM: Thread(s) gone.", ent_pnt);
				abort();
			}
			break;

			/*
			 * For a normal shutdown of the robot daemon:
			 * 1) prevent the alarm from going off during
			 *    shutdown and causing a core dump
			 * 2) log the reason we are shutting down
			 * 3) kill the helper pid if there is one
			 * 4) terminate the connection to the catalog
			 * 5) terminate all of the processing threads
			 */
		case SIGINT:
		case SIGTERM:
			sigdelset(&sigwait_set, SIGALRM);
			sam_syslog(LOG_INFO,
			    "%s: Shutdown by signal %d", ent_pnt, what_signal);
			if (library->helper_pid > 0)
				kill(library->helper_pid, 9);
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
	dev_ent_t 	*un;
	int 		i;

	un = library->un;
	for (i = 0; i < GENERIC_MAIN_THREADS; i++) {
		if (threads[i] > 0 && thr_kill(threads[i], SIGEMT)) {
			DevLog(DL_SYSERR(5081), i);
		}
	}

	mutex_lock(&library->un->mutex);
	if (library->helper_pid == (pid_t)- 1) {
		/*
		 * We are shuting down,
		 * attempt to allow media removal from the robot.
		 */
		TAPEALERT(library->open_fd, library->un);
		scsi_cmd(library->open_fd, library->un, SCMD_DOORLOCK, 0,
		    UNLOCK, 1);
		TAPEALERT(library->open_fd, library->un);
		set_operator_panel(library, UNLOCK);
		mutex_unlock(&library->un->mutex);
	}
}


/*
 * check_slot_audits - go through the library and audit any slot with
 * the audit bit set.
 */
void
check_slot_audits(
	library_t *library)
{
	int 	ns;
	struct CatalogEntry ced;
	struct CatalogEntry ced2;
	struct CatalogEntry *ce = NULL;
	struct CatalogEntry *ce2 = NULL;
	struct VolId vid;

	memset(&vid, 0, sizeof (struct VolId));
	vid.ViFlags = VI_cart;
	vid.ViEq = library->eq;
	memmove(vid.ViMtype, sam_mediatoa(library->un->media),
	    sizeof (vid.ViMtype));

	for (ns = 0; ns <= library->range.storage_count; ns++) {

		vid.ViSlot = ns;
		if (library->status.b.two_sided) {
			vid.ViPart = 1;
		} else {
			vid.ViPart = 0;
		}

		ce = CatalogGetEntry(&vid, &ced);
		if (library->status.b.two_sided) {
			vid.ViPart = 2;
			ce2 = CatalogGetEntry(&vid, &ced2);
		}
		if ((ce != NULL && (ce->CeStatus & CES_inuse) &&
		    (ce->CeStatus & CES_needs_audit)) ||
		    (ce2 != NULL && (ce2->CeStatus & CES_inuse) &&
		    (ce2->CeStatus & CES_needs_audit))) {
			robo_event_t *new_event;

			new_event = get_free_event(library);
			(void) memset(new_event, 0, sizeof (robo_event_t));
			new_event->request.message.param.audit_request.slot =
			    ns;
			new_event->request.message.command = MESS_CMD_AUDIT;
			new_event->request.message.magic =
			    (uint_t)MESSAGE_MAGIC;
			new_event->type = EVENT_TYPE_MESS;
			new_event->status.bits = REST_FREEMEM;
			add_to_end(library, new_event);
		}
	}

	mutex_lock(&library->list_mutex);
	cond_signal(&library->list_condit);
	mutex_unlock(&library->list_mutex);
}


/*
 *	start_library_audit - put audit event on the worklist.
 */
void
start_library_audit(
	library_t *library)
{
	robo_event_t *new_event;

	new_event = get_free_event(library);
	(void) memset(new_event, 0, sizeof (robo_event_t));
	new_event->type = EVENT_TYPE_MESS;
	new_event->status.bits = REST_FREEMEM;

	new_event->request.message.command = MESS_CMD_AUDIT;
	/* LINTED constant truncated by assignment */
	new_event->request.message.magic = (uint_t)MESSAGE_MAGIC;
	new_event->request.message.param.audit_request.slot = ROBOT_NO_SLOT_L;

	mutex_lock(&library->list_mutex);
	if (library->active_count == 0)	/* any existing stuff */
		library->first = new_event;
	else
		add_active_list(library->first, new_event);

	library->active_count++;
	cond_signal(&library->list_condit);
	mutex_unlock(&library->list_mutex);
}
