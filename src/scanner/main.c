/*
 * scanner-main.  This is the scanner for the non robotic removable devices.
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#pragma ident "$Revision: 1.32 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <thread.h>
#include <synch.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#define	SCANNER_MAIN
#define	MAIN

#include "sam/types.h"
#include "sam/param.h"
#include "sam/exit.h"
#include "sam/lib.h"
#include "sam/custmsg.h"
#include "aml/shm.h"
#include "aml/scan.h"
#include "sam/defaults.h"
#include "aml/logging.h"
#include "sam/devnm.h"
#include "sam/devinfo.h"
#include "aml/sefvals.h"
#include "aml/robots.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/tapealert.h"
#include "aml/sef.h"

/* Private functions. */
static void kill_off_threads(void);
static void *scan_device(void *vun);
static void allow_media_removal(shm_ptr_tbl_t *);
static void scan_devices(shm_ptr_tbl_t *);
static void *alarm_thread();

void *monitor_msg(void *);

/* some globals */

int fifo_fd, fifo_log_fd;
int thread_pri;	/* priority to run sub-threads at */
pid_t mypid;
char *program_name = "scanner";
char *fifo_path;
shm_alloc_t master_shm, preview_shm;

int
main(
	int argc,
	char **argv)
{
	int what_signal, *tmp, i;
	thread_t me, msg_thread;
	sigset_t signal_set, full_block_set;
	thread_t alarm_thread_id;
	struct sigaction sig_action;
	shm_ptr_tbl_t  *shm_ptr_tbl;
	shm_preview_tbl_t *shm_preview_tbl;

	if (argc != 3) {
		return (EXIT_NORESTART);
	}
	program_name = "scanner";
	CustmsgInit(1, NULL);
	argv++;
	master_shm.shmid = atoi(*argv);
	argv++;
	preview_shm.shmid = atoi(*argv);
	mypid = getpid();

	/*
	 * openlog has not been called yet, failure to attach either shared
	 * memory segment is a very bad problem.  Use exit codes to determine
	 * which failed.
	 */
	if ((master_shm.shared_memory = shmat(master_shm.shmid, (void *) NULL,
	    0774)) == (void *) -1) {
		exit(2);
	}

	if ((preview_shm.shared_memory = shmat(preview_shm.shmid, (void *) NULL,
	    0774)) == (void *) -1) {
		exit(3);
	}
	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
	shm_preview_tbl = (shm_preview_tbl_t *)preview_shm.shared_memory;

	fifo_path = strdup(SHM_REF_ADDR(shm_ptr_tbl->fifo_path));

	/*
	 * In an attempt to keep from losing the alarm signal, the main loop
	 * will run at a higher priority then the actual scanning threads.
	 */
	me = thr_self();
	if (CatalogInit("scanner") == -1) {
		sam_syslog(LOG_ERR, "%s", catgets(catfd, SET, 2364,
		    "Catalog initialization failed!"));
		exit(1);
	}
	/* check if we should log sef data */
	(void) sef_status();

	if (DBG_LVL(SAM_DBG_RBDELAY)) {
		char *sc_mess;
		int ldk = 60;
		sam_syslog(LOG_DEBUG, "Waiting for 60 seconds.");
		while (ldk > 0 && DBG_LVL(SAM_DBG_RBDELAY)) {
			sc_mess = ((shm_ptr_tbl_t *)
			    master_shm.shared_memory)->dis_mes[DIS_MES_CRIT];
			sprintf(sc_mess, "waiting for %d seconds pid %d",
			    ldk, mypid);
			sleep(10);
			ldk -= 10;
		}
		*sc_mess = '\0';
	}
	sigfillset(&full_block_set);	/* used to block all signals */

	sigemptyset(&signal_set);	/* signals to except. */
	sigaddset(&signal_set, SIGINT);	/* during sigwait */
	sigaddset(&signal_set, SIGHUP);
	sigaddset(&signal_set, SIGALRM);


	sig_action.sa_handler = SIG_DFL; /* want to restart system calls */
	sigemptyset(&sig_action.sa_mask);	/* on excepted signals. */
	sig_action.sa_flags = SA_RESTART;

	sigaction(SIGINT, &sig_action, (struct sigaction *)NULL);
	sigaction(SIGHUP, &sig_action, (struct sigaction *)NULL);
	sigaction(SIGALRM, &sig_action, (struct sigaction *)NULL);

	/* The default mode is to block everything */

	thr_sigsetmask(SIG_SETMASK, &full_block_set, (sigset_t *)NULL);
	thr_setconcurrency(10);
	ld_devices(0);		/* load device interfaces */
	if (thr_create(NULL, DF_THR_STK, monitor_msg, (void *) me,
	    (THR_DETACHED | THR_BOUND | THR_NEW_LWP), &msg_thread) &&
	    thr_create(NULL, DF_THR_STK, monitor_msg, (void *) me,
	    THR_DETACHED, &msg_thread)) {
		sam_syslog(LOG_INFO, "Unable to create thread monitor_msg:%m.");
		exit(1);
	}
	sleep(20);
	scan_devices(shm_ptr_tbl);	/* look um over */
	thr_create(NULL, DF_THR_STK, alarm_thread, NULL, THR_BOUND,
	    &alarm_thread_id);
	/* Wait for alarm thread (forever) */
	thr_join(alarm_thread_id, NULL, NULL);
	/* The thread join will never return */
	exit(0x99);
}

/*
 * scan_devices - scan all removable non-family (not robot) devices.
 *
 * Entry -
 *      ptr_tbl - pointer to the shared memory segment.
 */

static void
scan_devices(shm_ptr_tbl_t *ptr_tbl)
{
	dtype_t type;
	dev_ent_t *un;

	un = (dev_ent_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);

	for (; un != NULL;
	    un = (dev_ent_t *)SHM_REF_ADDR(un->next)) {
		/*
		 * only check devices that are not a family set and are
		 * removable
		 */

		type = un->type & DT_CLASS_MASK;
		if (!un->fseq && ((type == DT_TAPE) || (type == DT_OPTICAL))) {
			/*
			 * Dont scan active devices or devices UNAVAIL or OFF
			 * or DOWN
			 */
			if (un->active != 0 || un->state > DEV_IDLE)
				continue;

			/*
			 * if we can't get the lock, we'll just go on and try
			 * the next device.  We'll get this guy the next time
			 */

			/* N.B. Bad indentation to meet cstyle requirements. */
			if (!mutex_trylock(&un->mutex)) {
			if ((un->active == 0) &&
			    (un->state == DEV_IDLE)) {
				message_request_t *message;
				mutex_unlock(&un->mutex);

				message = (message_request_t *)SHM_REF_ADDR(
				    ((shm_ptr_tbl_t *)
				    master_shm.shared_memory)->scan_mess);
				mutex_lock(&message->mutex);
				while (message->mtype != MESS_MT_VOID)
					cond_wait(&message->cond_i,
					    &message->mutex);

				(void) memset(&message->message, 0,
				    sizeof (sam_message_t));

				/* LINTED constant truncated by assignment */
				message->message.magic = MESSAGE_MAGIC;
				message->message.command = MESS_CMD_STATE;
				message->message.param.state_change.flags = 0;
				message->message.param.state_change.eq = un->eq;
				message->message.param.state_change.old_state =
				    DEV_IDLE;
				message->message.param.state_change.state =
				    DEV_OFF;
				message->mtype = MESS_MT_NORMAL;
				cond_signal(&message->cond_r);
				mutex_unlock(&message->mutex);
				continue;
			}
			if (!(un->status.bits &
			    (DVST_REQUESTED | DVST_SCANNING) &&
			    (un->open_count == 0)) &&
			    (!un->status.b.scanned || un->status.b.unload) &&
			    un->active == 0) {
				/*
				 * Set the scanning bit and scan this
				 * device.
				 */
				un->status.bits |= DVST_SCANNING;
				/* Mutex is released in scan_device */
				if (thr_create(NULL, DF_THR_STK, scan_device,
				    (void *) un, (THR_DETACHED | THR_BOUND),
				    &un->scan_tid) &&
				    thr_create(NULL, DF_THR_STK, scan_device,
				    (void *) un, THR_DETACHED, &un->scan_tid)) {
					sam_syslog(LOG_INFO,
					    "Unable to create thread "
					    "scan_device:%m.");
				}
			} else {	/* Check if its needed */
				if (un->status.bits & DVST_LABELED)
					check_preview(un);
				mutex_unlock(&un->mutex);
			}
			}
		}
	}
}

/*
 * allow_media_removal - issue scsi command to allow media removal
 *
 * Entry -
 *      ptr_tbl - pointer to the shared memory segment.
 */

static void
allow_media_removal(shm_ptr_tbl_t *ptr_tbl)
{
	int fd;
	dev_ent_t *un;

	un = (dev_ent_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);
	for (; un != NULL; un = (dev_ent_t *)SHM_REF_ADDR(un->next)) {
		/*
		 * only check devices that are not a family set and are
		 * removable
		 */

		if (!un->fseq && (IS_TAPE(un) || IS_OPTICAL(un))) {
			/*
			 * Dont worry about devices OFF or DOWN.  Note:
			 * Before a device becomes unavailable, it should be
			 * allowed to remove media.
			 */

			if ((un->active != 0) || (un->state > DEV_IDLE))
				continue;

			/*
			 * if we can't get the lock, we'll just go on and try
			 * the next device.
			 */

			if (!mutex_trylock(&un->mutex)) {

				if ((fd = open(un->name, O_RDONLY)) >= 0) {
					INC_OPEN(un);
					if (!mutex_trylock(&un->io_mutex)) {
						if ((un->status.bits &
						    DVST_LABELED) &&
						    (!(IS_OPTICAL(un)))) {
							sef_data(un, fd);
						}
						mutex_unlock(&un->io_mutex);
					}
					TAPEALERT(fd, un);
					scsi_cmd(fd, un, SCMD_DOORLOCK, 0,
					    UNLOCK);
					TAPEALERT(fd, un);
					DEC_OPEN(un);
					close(fd);
				}
				mutex_unlock(&un->mutex);
			}
		}
	}
}


/*
 * scan_device - Scan a device.
 * Scanning bit should be set in the status field.
 * On entry:
 *   vun = The unit table pointer.
 *   Scanning bit set.
 *   Active count is zero
 *   Mutex is locked
 */

static void *
scan_device(
	void *vun)
{
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	dev_ent_t *un = (dev_ent_t *)vun;
	boolean_t unload;
	int err;
	int fd;

	un->slot = 0;
	ce = CatalogGetCeByLoc(un->eq, un->slot, 0, &ced);
	if (ce == NULL) {
		un->slot = CatalogAssignFreeSlot(un->eq);
		if ((ce = CatalogGetCeByLoc(un->eq, un->slot, 0,
		    &ced)) == NULL) {
			sam_syslog(LOG_ERR,
			    "Unable to get catalog entry for eq %d", un->eq);
			err = 1;
			thr_exit(&err);
		}
	}
	un->mid = ce->CeMid;
	un->i.ViPart = ce->CePart;
	un->status.bits &= ~DVST_WRITE_PROTECT;
	if ((fd = open(un->name, O_RDWR)) < 0) {
		if (IS_TAPE(un) && (errno == EACCES)) {
			if ((fd = open(un->name, O_RDONLY)) < 0) {
				/*
				 * If tape fails, then assume no tape mounted
				 * and don't issue error message.
				 */
				if ((errno != EIO) && (errno != EBUSY)) {
					char msg[STR_FROM_ERRNO_BUF_SIZE];

					sam_syslog(LOG_INFO,
					    catgets(catfd, SET, 20003,
					    "Unable to open device(%d):%s."),
					    un->eq,
					    StrFromErrno(errno, msg,
					    sizeof (msg)));
				}
				if (errno != EBUSY) {
					un->status.bits &=
					    ~(DVST_PRESENT | DVST_LABELED);
				}
				un->status.bits &= ~DVST_SCANNING;
				mutex_unlock(&un->mutex); /* release mutex */
				thr_exit(NULL);
			}
			un->status.bits |= DVST_WRITE_PROTECT;
		} else {
			if (DBG_LVL(SAM_DBG_DEBUG)) {
				sam_syslog(LOG_DEBUG, "open error %d\n", errno);
			}
			un->status.bits &= ~DVST_SCANNING;
			mutex_unlock(&un->mutex);	/* release mutex */
			thr_exit(NULL);
		}
	}
	INC_OPEN(un);

	/* see if device has been id'ed yet */
	if (IS_TAPE(un) && (un->scsi_type == '\0')) {
		ident_dev(un, fd);
	}
	/*
	 * Get the rdev file system access.
	 */
	if (!un->st_rdev) {
		struct stat stat_buf;

		if (fstat(fd, &stat_buf)) {
			char msg[STR_FROM_ERRNO_BUF_SIZE];

			sam_syslog(LOG_INFO,
			    catgets(catfd, SET, 20004,
			    "Unable to stat drive(%d):%s"),
			    un->eq, StrFromErrno(errno, msg, sizeof (msg)));
			stat_buf.st_rdev = 0;
		}
		un->st_rdev = stat_buf.st_rdev;
	}
	/*
	 * Clear device messages.
	 */
	*un->dis_mes[DIS_MES_NORM] = '\0';
	*un->dis_mes[DIS_MES_CRIT] = '\0';

	mutex_unlock(&un->mutex);
	unload = un->status.b.unload;
	scan_a_device(un, fd);
	mutex_lock(&un->mutex);
	close(fd);
	DEC_OPEN(un);

	if (un->status.bits & DVST_LABELED) {
		check_preview(un);
	}
	mutex_unlock(&un->mutex);
	if (!unload) {
		if (un->status.b.labeled) {
			boolean_t changed;

			/*
			 * A labeled cartridge is in the drive. Compare with
			 * the catalog entry.
			 */
			if (un->equ_type == DT_FUJITSU_128) {
				/*
				 * Media is labeled and device is Fujitsu
				 * M8100. If media has double length capacity
				 * and previously mounted by the lmf, or
				 * manually mounted and capacity was set to
				 * double length by chmed(1M) command,
				 * un->capacity should be equal to
				 * ce->CeCapacity.
				 */
				if (ce->CeStatus & CES_inuse &&
				    strcmp(sam_mediatoa(un->type),
				    ce->CeMtype) == 0 &&
				    strncmp(un->vsn, ce->CeVsn,
				    sizeof (vsn_t)) == 0 &&
				    un->sector_size == ce->CeBlockSize &&
				    un->label_time == ce->CeLabelTime &&
				    un->capacity != ce->CeCapacity) {
					/*
					 * if (ce->CeCapacity >
					 * (FUJITSU_128_SZ * 2))
					 * ce->CeCapacity = FUJITSU_128_SZ * 2;
					 */
					un->capacity =
					    un->dt.tp.default_capacity =
					    ce->CeCapacity;
				}
			}
			changed = TRUE;
			if (ce->CeStatus & CES_inuse) {
				if (strcmp(sam_mediatoa(un->type),
				    ce->CeMtype) != 0 ||
				    strncmp(un->vsn, ce->CeVsn,
				    sizeof (vsn_t)) != 0 ||
				    un->capacity != ce->CeCapacity ||
				    un->sector_size != ce->CeBlockSize ||
				    un->label_time != ce->CeLabelTime) {
					/*
					 * Volume in drive is different. Any
					 * previous cartridge must have been
					 * unloaded.
					 */
					if (*ce->CeVsn != '\0') {
						un->i.ViFlags = VI_cart;
						un->i.ViSlot = ce->CeSlot;
						un->i.ViEq = ce->CeEq;
						un->i.ViPart = ce->CePart;
						CatalogExport(&un->i);
					}
					changed = TRUE;
				} else if (un->status.b.write_protect !=
				    ((ce->CeStatus & CES_writeprotect) ?
				    1 : 0)) {
					changed = TRUE;
				} else
					changed = FALSE;
			} else
				changed = TRUE;
			if (changed)
				UpdateCatalog(un, 0, CatalogVolumeLoaded);
		} else if (un->equ_type == DT_FUJITSU_128) {
			/*
			 * Media is not labeled and device is Fujitsu M8100,
			 * reset the default media capacity since M8100
			 * doesn't return the media capacity for the
			 * SCMD_MODE_SENSE command so we calculate the space
			 * and capacity using the default_capacity. There is
			 * no way to know the Default capacity unless media
			 * is mounted by library (LT300). M8100 has two
			 * capacity types, normal or double length, and it's
			 * written in a barcode.
			 */
			un->capacity = un->dt.tp.default_capacity =
			    FUJITSU_128_SZ;
		}
	}
	/*
	 * No-one is waiting, but always exit with 0.
	 */
	err = 0;
	thr_exit(&err);
}


static void *
alarm_thread()
{
	int what_signal, alrm_time;
	time_t last_stale = 0, now;
	sigset_t signal_set, full_block_set;
	shm_ptr_tbl_t *shm_ptr_tbl;
	preview_tbl_t *preview_tbl = &((shm_preview_tbl_t *)
	    (preview_shm.shared_memory))->preview_table;
	sam_defaults_t *defaults;

	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
	defaults = GetDefaults();

	sigfillset(&full_block_set);	/* used to block all signals */
	sigemptyset(&signal_set);	/* signals to except. */
	sigaddset(&signal_set, SIGINT);	/* during sigwait */
	sigaddset(&signal_set, SIGHUP);
	sigaddset(&signal_set, SIGALRM);
	thr_sigsetmask(SIG_SETMASK, &full_block_set, (sigset_t *)NULL);
	alrm_time = (SCANNER_CYCLE < 15) ? 15 : SCANNER_CYCLE;
	if (alrm_time > 60)
		alrm_time = 60;

	while (TRUE) {
		alarm(alrm_time);
		what_signal = sigwait(&signal_set);	/* wait for a signal */
		time(&now);
		switch (what_signal) {	/* process the signal */
		case SIGALRM:
			thr_sigsetmask(SIG_SETMASK, &full_block_set,
			    (sigset_t *)NULL);
			if (now > last_stale && preview_tbl->ptbl_count &&
			    defaults->stale_time)
				if (thr_create(NULL, DF_THR_STK,
				    remove_stale_preview, NULL,
				    (THR_DETACHED | THR_BOUND), NULL) &&
				    thr_create(NULL, DF_THR_STK,
				    remove_stale_preview, NULL,
				    THR_DETACHED, NULL)) {
					sam_syslog(LOG_INFO,
					    "Unable to create thread "
					    "remove_stale_preview:%m.");
				} else
					last_stale = now + 60;

			scan_devices(shm_ptr_tbl);	/* look um over */
			break;

		case SIGINT:
			/* FALLTHRU */
		case SIGHUP:
			sam_syslog(LOG_INFO, "scanner shutdown: signal %d",
			    what_signal);
			allow_media_removal(shm_ptr_tbl);
			/* detach master shared memory */
			shmdt(master_shm.shared_memory);
			/* detach preview shared memory */
			shmdt(preview_shm.shared_memory);
			exit(0);

		default:
			break;
		}
	}
}
