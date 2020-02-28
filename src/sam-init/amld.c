/*
 * sam-amld - initialize the SAMFS automated media library daemons.
 *
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <fcntl.h>
#include <synch.h>
#include <thread.h>
#include <time.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <link.h>

#define	DEC_INIT
#define	MAIN

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/param.h"
#include "sam/nl_samfs.h"
#include "sam/lib.h"
#include "sam/defaults.h"
#include "aml/shm.h"
#include "aml/logging.h"
#include "aml/tapes.h"
#include "aml/archiver.h"
#include "aml/proto.h"
#include "aml/stager.h"
#include "aml/dev_log.h"

/* Local headers. */
#include "amld.h"

#define	TIMER_SECS 10

/* some globals */

/* External function prototypes. */
extern void *samfm_service(void *arg);

/* Private data. */
static sam_defaults_t *df;
static thread_t fifo_threads[NUM_FIFOS];
static const char *xmllib[] = {	/* Gnome XML library search paths */
	"/usr/lib/libxml2.so.2",
	"/opt/SUNWstade/libxml2/lib/libxml2.so.2",
	"libxml2.so.2",
	NULL
};
static void *xmlhandle = NULL;	/* Gnome XML library handle */
static thread_t samfm_thread;	/* StorADE API thread */

/* Private functions. */
static void identify_devices(void);
static void sig_child(int);
static void shut_down_FS_cmd_queue(void);
static void start_sam_remote();
static void check_datapath(int);

char *program_name;


int
main(
/* ARGSUSED0 */
	int argc,
	char **argv)
{
	sigset_t signal_set, full_block_set;
	shm_ptr_tbl_t *shm_ptr_tbl;
	struct sigaction sig_action;
	char	*sc_mess;
	int shutdown_amld = 0;
	int	i;

	program_name = SAM_AMLD;
	master_shm.shmid = -1;
	master_shm.shared_memory = NULL;

	if (strcmp(GetParentName(), SAM_FSD) != 0) {
		/* sam-amld may be started only by sam-fsd */
		fprintf(stderr, "%s\n", GetCustMsg(11007));
#if !defined(DEBUG)
		exit(EXIT_FAILURE);
#endif
	}
	CustmsgInit(1, NULL);
	TraceInit(program_name, TI_aml);

	/*
	 * All sam-amld files are releative to AML_DIR.
	 * Also, sam-amld's core files will be found here.
	 */
	if (chdir(AML_DIR) == -1) {
		LibFatal(chdir, AML_DIR);
	}

	/*
	 * Set group id for all created directories and files.
	 */
	df = GetDefaults();
	if (setgid(df->operator.gid) == -1) {
		SysError(HERE, "setgid(%ld)", df->operator.gid);
	}
	(void) umask(0);

	/*
	 * Set the number of open file descriptors to the limit, This is
	 * inherited by children so everyone started by sam-amld will have
	 * this limit.
	 */
	{
		struct rlimit rlimits;

		if (getrlimit(RLIMIT_NOFILE, &rlimits) != -1) {
			rlimits.rlim_cur = rlimits.rlim_max;
			if (setrlimit(RLIMIT_NOFILE, &rlimits) < 0) {
				Trace(TR_ERR,
				    "setrlimit(%d) failed",
				    (int)rlimits.rlim_max);
			}
		} else {
			Trace(TR_ERR, "getrlimit() failed");
		}
	}

	Trace(TR_MISC, "Automated media library daemon started.");
	sigfillset(&full_block_set);	/* used to block all signals */

	sigemptyset(&signal_set);	/* signals to except. */
	sigaddset(&signal_set, SIGINT);
	sigaddset(&signal_set, SIGHUP);
	sigaddset(&signal_set, SIGALRM);
	sigaddset(&signal_set, SIGCHLD);

	sig_action.sa_handler = SIG_DFL;	/* want to restart system */
						/* calls. */
	sigemptyset(&sig_action.sa_mask);	/* on excepted signals. */
	sig_action.sa_flags = SA_RESTART;

	(void) sigaction(SIGINT, &sig_action, NULL);
	(void) sigaction(SIGHUP, &sig_action, NULL);
	(void) sigaction(SIGALRM, &sig_action, NULL);

	sig_action.sa_handler = sig_child;	/* needed for Solaris 2.5 */
	(void) sigaction(SIGCHLD, &sig_action, NULL);

	/* The default mode is to block everything */

	thr_sigsetmask(SIG_SETMASK, &full_block_set, NULL);

	alloc_shm_seg();		/* establish shared memory */
	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
	DeviceParams = (struct DeviceParams *)(void *)
	    ((char *)df + sizeof (sam_defaults_t));
	shm_ptr_tbl->debug = df->debug;
	shm_ptr_tbl->sam_amld = getpid();
	shm_ptr_tbl->valid = 1;

	sam_syslog(LOG_INFO, "sam-amld started.");
	sc_mess = ((shm_ptr_tbl_t *)
	    master_shm.shared_memory)->dis_mes[DIS_MES_CRIT];
	if (sem_init(&shm_ptr_tbl->notify_sem, 1, 1) != 0) {
		sam_syslog(LOG_ERR, "sem_init failed: %d", errno);
	}

	/*
	 * Make important directories.
	 * Start the catalog server.
	 * Identify devices.
	 *
	 * Start the catalog server before identify_devices() since
	 * run_mt_cmd() called by identify_devices() may take a while to
	 * complete. The hasam agent hasam_start() requires the catalog
	 * server is running soon after sam-amld has started.
	 * This requirement is only applied to the hasam configuration.
	 */
	MakeDir(DEVLOG_NAME);

	if (DBG_LVL(SAM_DBG_DEBUG)) {
		sam_syslog(LOG_DEBUG, "Starting %s.", CATSERVER_CMD);
	}
	start_a_process(&pids[CHILD_CATSERVER - 1], NULL);

	identify_devices();

	start_children(pids);
	start_sam_remote();

	if (thr_create(NULL, DF_THR_STK, manage_fs_fifo, NULL,
	    (THR_DETACHED | THR_BOUND), &fifo_threads[SAM_FS_FIFO_THREAD])) {
		sam_syslog(LOG_ALERT,
		    "Unable to create thread manage_fs_fifo:%m.");
		exit(2);
	}
	if (thr_create(NULL, DF_THR_STK, manage_cmd_fifo, NULL,
	    (THR_DETACHED | THR_BOUND), &fifo_threads[SAM_CMD_FIFO_THREAD])) {
		sam_syslog(LOG_ALERT,
		    "Unable to create thread manage_cmd_fifo:%m. ");
		exit(2);
	}

	if (df->samstorade != B_FALSE) {
		/*
		 * Dynamic linking of Gnome XML shared library.
		 */
		xmlhandle = NULL;
		for (i = 0; xmlhandle == NULL && xmllib[i] != NULL; i++) {
			xmlhandle = dlopen(xmllib[i], RTLD_LAZY);
		}

		if (xmlhandle) {
			if (thr_create(NULL, DF_THR_STK, samfm_service,
			    xmlhandle, (THR_DETACHED | THR_BOUND),
			    &samfm_thread)) {
				sam_syslog(LOG_ALERT,
				    "Unable to create thread"
				    " samfm_service:%m. ");
				exit(2);
			}
		} else {
			sam_syslog(LOG_INFO,
			    "Sun StorADE API unavailable, libxml.so.2 not"
			    " found.");
		}
	} else {
		sam_syslog(LOG_INFO,
		    "Sun StorADE API in defaults.conf is off.");
	}

	(void) ArchiverRmState(1);
	while (shutdown_amld == 0) {
		int		what_signal;

		alarm(TIMER_SECS);	/* restart timer */
		what_signal = sigwait(&signal_set);	/* wait for a signal */
		switch (what_signal) {	/* process the signal */
		case SIGALRM:
			check_children(pids);
			check_datapath(TIMER_SECS);
			break;

		case SIGCHLD:
			reap_child(pids);	/* clean up a dead child */
			break;

		case SIGINT:
			memccpy(sc_mess,
			    catgets(catfd, SET, 11001,
			    "sam-amld has shutdown, shared memory invalid"),
			    '\0', DIS_MES_LEN);
			shutdown_amld = SIGINT;
			break;

		case SIGHUP:
			TraceReconfig();
			break;

		default:
			break;
		}
	}

	/*
	 * Cleanup for sam-amld shutdown.
	 */
	sam_syslog(LOG_INFO, "sam-amld shutdown by signal %d", shutdown_amld);
	(void) ArchiverRmState(0);
	(void) StagerStopRm();
	kill_off_threads();	/* shut down my threads */
	shut_down_FS_cmd_queue();
	kill_children(pids);	/* stop the children */
	rel_shm_seg();	/* release shared memory */
	return (EXIT_SUCCESS);
}


static void
sig_child(
/* ARGSUSED0 */
	int sig)
{
}


void
kill_off_threads()
{
	int		i;

	for (i = 0; i < NUM_FIFOS; i++)
		thr_kill(fifo_threads[i], SIGINT);
	if (xmlhandle) {
		thr_kill(samfm_thread, SIGINT);
		(void) dlclose(xmlhandle);
	}
}


void
shut_down_FS_cmd_queue(void)
{
	(void) sam_syscall(SC_amld_quit, NULL, 0);
}

static void
check_datapath(int secs)
{
	dev_ent_t *un;
#define	SEF_MASK (SEF_ENABLED|SEF_SUPPORTED)
	static int total_secs = 0;

	total_secs += secs;
	if (total_secs < FIVE_MINS_IN_SECS) {
		return;
	}

	for (un = (dev_ent_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);
	    un != NULL; un = (dev_ent_t *)SHM_REF_ADDR(un->next)) {

		/* sef log sense polling */
		if (un->scsi_type == 1 && un->state == DEV_ON &&
		    (un->sef_sample.state & SEF_MASK) == SEF_MASK &&
		    un->sef_sample.interval > SEF_INTERVAL_ONCE) {
			un->sef_sample.counter += total_secs;
			if (un->sef_sample.counter >= un->sef_sample.interval) {
				un->sef_sample.counter = 0;
				un->sef_sample.state |= SEF_POLL;
			}
		}

		/* TODO: device watchdog */
	}

	total_secs = 0;
}

/*
 * run_mt_cmd - Wrapper to run mt command on the node.
 *
 * Return: Status of command execution
 */
int
run_mt_cmd(
	char *drvpath,
	char *mtcmd)
{
	char cmd[MAX_MSGBUF_SIZE];
	FILE *ptr;

	(void) snprintf(cmd, MAX_MSGBUF_SIZE,
	    "/usr/bin/mt -f %s %s > /dev/null 2>&1", drvpath, mtcmd);

	sam_syslog(LOG_DEBUG, "run_mt_cmd %s", cmd);
	Trace(TR_MISC, "run_mt_cmd %s", cmd);

	if ((ptr = popen(cmd, "w")) == NULL) {
		sam_syslog(LOG_ERR, "run_mt_cmd: popen failed");
		return (-1);
	}

	if (pclose(ptr) == -1) {
		sam_syslog(LOG_ERR, "run_mt_cmd: pclose failed");
		return (-1);
	}

	return (0);
}

/*
 * identify_devices - identify devices.
 */
static void
identify_devices()
{
	dev_ent_t *device;
	int rc;
	struct stat buf;
	boolean_t hasam_running = FALSE;

	if (stat(HASAM_RUN_FILE, &buf) == 0) {
		hasam_running = TRUE;
	}

	for (device = (dev_ent_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);
	    device != NULL; device = (dev_ent_t *)SHM_REF_ADDR(device->next)) {

		if (IS_RSC(device) || IS_RSD(device) || IS_RSS(device)) {
			InitDevLog(device);
			continue;
		}
		if (is_scsi(device) || IS_ROBOT(device)) {
			int		i;

			InitDevLog(device);
			/* device must be ON, RO or UNAVAIL */
			if (device->state != DEV_UNAVAIL &&
			    device->state > DEV_RO) {
				continue;
			}
			mutex_lock(&device->mutex);	/* aquire lock */
			if (is_scsi(device) || (device->type == DT_ROBOT)) {
				if ((hasam_running) && (IS_TAPE(device))) {
					rc = run_mt_cmd(device->name,
					    "forcereserve");
					if (rc != 0) {
						sam_syslog(LOG_ERR,
						    "Error mt forcereserve on"
						    " drive %s failed",
						    device->name);
					}
				}
				ident_dev(device, -1);	/* id the device */
			}
			if (IS_ROBOT(device)) {
				device->dt.rb.status.b.export_unavail =
				    (df->flags & DF_EXPORT_UNAVAIL) ?
				    TRUE : FALSE;
			}

			/*
			 * Set delay/unload time if not already set.
			 */
			for (i = 0; i < DeviceParams->count; i++) {
				struct DpEntry *dp;

				dp = DeviceParams->DpTable + i;
				if (device->type != dp->DpType) {
					continue;
				}
				if (device->delay == 0) {
					device->delay = dp->DpDelay_time;
				}
				if (device->unload_delay == 0) {
					device->unload_delay =
					    dp->DpUnload_time;
				}
				if (IS_TAPE(device) &&
				    device->dt.tp.default_blocksize == 0) {
					device->dt.tp.default_blocksize =
					    dp->DpBlock_size;
				}
				if (IS_TAPE(device) &&
				    device->dt.tp.position_timeout == 0) {
					device->dt.tp.position_timeout =
					    dp->DpPosition_timeout;
				}
				break;
			}
			mutex_unlock(&device->mutex);
		}
	}
}


static void
start_sam_remote()
{
	dev_ent_t *device;
	boolean_t startit;

	/*
	 * Don't start this thread until the license manager has a chance
	 * to check the license and delete any remote sam devices if the
	 * license doesn't allow remote sam.  We know that the license
	 * manager has done this because it posts to child_sema after it's
	 * done.
	 */
	startit = FALSE;
	for (device = (dev_ent_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);
	    device != NULL; device = (dev_ent_t *)SHM_REF_ADDR(device->next)) {
		if (device->equ_type == DT_PSEUDO_SC ||
		    device->equ_type == DT_PSEUDO_SS) {
			startit = TRUE;
		}
	}
	if (startit) {
		if (thr_create(NULL, DF_THR_STK, rmt_server_thr, NULL,
		    (THR_DETACHED | THR_BOUND), NULL)) {
			sam_syslog(LOG_ALERT,
			    "Unable to create thread rmt_server_thr:%m.");
			exit(2);
		}
	}
}
