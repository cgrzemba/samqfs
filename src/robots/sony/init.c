/*
 * init.c - initialize the library and all its elements
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

#pragma ident "$Revision: 1.34 $"

static char    *_SrcFile = __FILE__;

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
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "driver/samst_def.h"
#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/shm.h"
#include "sam/defaults.h"
#include "aml/robots.h"
#include "sam/nl_samfs.h"
#include "sony.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/catalog.h"
#include "aml/catlib.h"


/* function prototypes */
int		init_drives(library_t *, dev_ptr_tbl_t *);
void	    start_helper(library_t *);

/* globals */
extern char    *fifo_path;
static char    *sony_server;
extern shm_alloc_t master_shm, preview_shm;
extern char    *sam_sony_status(int status);
extern void	set_catalog_tape_media(library_t *library);


/*
 * initialize - initialize the library exit - 0 - ok !0 - failed
 */
int
initialize(
	library_t *library,
	dev_ptr_tbl_t *dev_ptr_tbl)
{
	int		i, fatal = 0;
	ulong_t	 tmp_userid;
	char	   *ent_pnt = "initialize";
	char	   *line, *tmp;
	char	   *l_mess = library->un->dis_mes[DIS_MES_NORM];
	char	   *lc_mess = library->un->dis_mes[DIS_MES_NORM];
	char	   *err_fmt = catgets(catfd, SET, 9476,
	    "%s: Syntax error in Sony configuration file line %d.");
	char	   *fat_mess = catgets(catfd, SET, 9478,
	    "Fatal error during initialization, see log");
	FILE	   *open_str;
	drive_state_t  *drive;
	struct CatalogHdr *ch;

	/*
	 * Process Sony configuration file
	 */
	if ((open_str = fopen(library->un->name, "r")) == NULL) {
		sam_syslog(LOG_CRIT, catgets(catfd, SET, 9417,
		    "%s: Unable to open configuration file(%s): %m."),
		    ent_pnt, library->un->name);
		memccpy(lc_mess, catgets(catfd, SET, 9418,
		    "Unable to open configuration file"), '\0', DIS_MES_LEN);
		return (-1);
	}
	if (CatalogInit("sony") == -1) {
		sam_syslog(LOG_ERR, "%s", catgets(catfd, SET, 2364,
		    "Catalog initialization failed!"));
		exit(1);
	}
	i = 0;
	line = malloc_wait(200, 2, 0);
	mutex_lock(&library->un->mutex);

	if (init_drives(library, dev_ptr_tbl)) {
		mutex_unlock(&library->un->mutex);
		return (-1);
	}
	library->index = library->drive;
	library->sony_userid = NULL;

	while (fgets(memset(line, 0, 200), 200, open_str) != NULL) {

		i++;
		if (*line == '#' || strlen(line) == 0 || *line == '\n')
			continue;

		if ((tmp = strchr(line, '#')) != NULL)
			memset(tmp, 0, (200 - (tmp - line)));

		if ((tmp = strtok(line, "= \t\n")) == NULL)
			continue;

		if (strcasecmp(tmp, "userid") == 0) {
			if ((tmp = strtok(NULL, "= \t\n")) == NULL) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}
			library->sony_userid = strdup(tmp);
			tmp_userid = strtoul(library->sony_userid, NULL, 10);
			if (tmp_userid > PSCUSERIDMAX) {
				sam_syslog(LOG_INFO, catgets(catfd, SET, 9419,
				"%s: userid invalid, greater than 0xffff."),
				    ent_pnt);
				fatal++;
			}
			continue;
		} else if ((strcasecmp(tmp, "sonydrive") == 0)) {
			char	*shared;
			char	*rmtname;
			drive_state_t  *drive;

			if ((tmp = strtok(NULL, " =\t\n")) == NULL) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}
			if ((rmtname = strtok(NULL, " =\t\n")) == NULL) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}
			shared = strtok(NULL, " =\t\n");

			for (drive = library->drive; drive != NULL;
			    drive = drive->next) {
				if (strcmp(drive->un->name, rmtname) == 0) {
					drive->PscBinInfo =
					    malloc_wait(sizeof (PscBinInfo_t),
					    2, 0);
					drive->BinNoChar =
					    malloc_wait(PSCDRIVENAMELEN, 2, 0);
					if ((int)strlen(tmp) >
					    PSCDRIVENAMELEN) {
						sam_syslog(LOG_INFO,
						    catgets(catfd, SET, 9479,
						    "%s: Drive name(%s) too"
						    " long line %d."),
						    ent_pnt, tmp, i);
						fatal++;
					} else
						strcpy(drive->BinNoChar, tmp);
					if (shared != NULL) {
						if (strcasecmp(shared, "shared")
						    == 0) {
							drive->un->flags =
							    DVFG_SHARED;
							sam_syslog(LOG_INFO,
							    catgets(catfd, SET,
							    9147, "Drive %d is"
							    " a shared drive."),
							    drive->un->eq);
						}
					}
					break;
				}
			}

			if (drive == NULL) {
				sam_syslog(LOG_INFO, catgets(catfd, SET, 9480,
				    "%s: Cannot find drive(%s) line %d."),
				    ent_pnt, rmtname, i);
				fatal++;
			}
			continue;
		} else if (strcasecmp(tmp, "server") == 0) {
			if ((tmp = strtok(NULL, " =\t\n")) == NULL) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}
			library->sony_servername = strdup(tmp);
			continue;
		} else {
			sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
			continue;
		}
	}
	fclose(open_str);

	/*
	 * Check that a userid was supplied.
	 */
	if (library->sony_userid == NULL) {
		sam_syslog(LOG_ERR, catgets(catfd, SET, 9482,
		    "%s: No userid supplied."), ent_pnt);
		fatal++;
	}
	/*
	 * Check that all the drives exist
	 */
	for (drive = library->drive; drive != NULL; drive = drive->next) {
		if (drive->PscBinInfo == NULL) {
			fatal++;
			sam_syslog(LOG_ERR, catgets(catfd, SET, 9481,
			    "%s: No sonydrive entry for drive(%s) %d."),
			    ent_pnt, drive->un->name, drive->un->eq);
		}
	}

	if (fatal) {
		memccpy(lc_mess, fat_mess, '\0', DIS_MES_LEN);
		return (-1);
	}
	/*
	 * Set up the Sony PSC environment
	 */
	if (library->un->equ_type == DT_SONYPSC) {
		sprintf(line, "PSCSERVERNAME =%s", library->sony_servername);
		sam_syslog(LOG_INFO, catgets(catfd, SET, 9483,
		    "%s: Setting Sony API environment %s."), ent_pnt, line);
		sony_server = strdup(line);
		if (putenv(sony_server)) {
			memccpy(lc_mess, fat_mess, '\0', DIS_MES_LEN);
			sam_syslog(LOG_ERR, "%s: putenv: SERVER: %m", ent_pnt);
			return (-1);
		}
	}
	free(line);

	/*
	 * First active drive determines media type
	 */
	set_catalog_tape_media(library);

	if (!(is_optical(library->un->media)) &&
	    !(is_tape(library->un->media))) {
		sam_syslog(LOG_INFO, catgets(catfd, SET, 9357,
		"Unable to determine media type. Library daemon exiting."));
		exit(1);
	}
	ch = CatalogGetHeader(library->eq);
	library->un->status.bits |= (DVST_PRESENT | DVST_REQUESTED);
	if ((sam_atomedia(ch->ChMediaType) == DT_OPTICAL) ||
	    (sam_atomedia(ch->ChMediaType) == DT_WORM_OPTICAL_12) ||
	    (sam_atomedia(ch->ChMediaType) == DT_WORM_OPTICAL) ||
	    (sam_atomedia(ch->ChMediaType) == DT_ERASABLE) ||
	    (sam_atomedia(ch->ChMediaType) == DT_MULTIFUNCTION))
		library->status.b.two_sided = TRUE;
	else
		library->status.b.two_sided = FALSE;

	library->status.b.passthru = 0;

	(void) CatalogSetCleaning(library->eq);

	mutex_unlock(&library->un->mutex);

	/*
	 * allocate the free list
	 */
	library->free = init_list(ROBO_EVENT_CHUNK);
	library->free_count = ROBO_EVENT_CHUNK;
	mutex_init(&library->free_mutex, USYNC_THREAD, NULL);
	mutex_init(&library->list_mutex, USYNC_THREAD, NULL);
	cond_init(&library->list_condit, USYNC_THREAD, NULL);

	library->transports = malloc_wait(sizeof (xport_state_t), 5, 0);
	(void) memset(library->transports, 0, sizeof (xport_state_t));

	mutex_lock(&library->transports->mutex);
	library->transports->library = library;
	library->transports->first = malloc_wait(sizeof (robo_event_t), 5, 0);
	(void) memset(library->transports->first, 0, sizeof (robo_event_t));
	library->transports->first->type = EVENT_TYPE_INTERNAL;
	library->transports->first->status.bits = REST_FREEMEM;
	library->transports->first->request.internal.command = ROBOT_INTRL_INIT;
	library->transports->active_count = 1;
	library->helper_pid = -1;
	memccpy(l_mess, catgets(catfd, SET, 9451,
	    "waiting for helper to start"), '\0', DIS_MES_LEN);
	start_helper(library);

	{
		char	*MES_9452 = catgets(catfd, SET, 9452,
		    "helper running as pid %d");
		char	*mes = (char *)malloc_wait(strlen(MES_9452) + 15, 5, 0);

		sprintf(mes, MES_9452, library->helper_pid);
		memccpy(l_mess, mes, '\0', DIS_MES_LEN);
		free(mes);
	}

	if (thr_create(NULL, MD_THR_STK, &transport_thread,
	    (void *) library->transports,
	    (THR_NEW_LWP | THR_BOUND | THR_DETACHED),
	    &library->transports->thread)) {
		sam_syslog(LOG_ERR,
		    "Unable to create thread transport_thread: %m.");
		library->transports->thread = (thread_t)- 1;
	}
	mutex_unlock(&library->transports->mutex);
	mutex_unlock(&library->mutex);

	/*
	 * free the drive threads
	 */
	for (drive = library->drive; drive != NULL; drive = drive->next)
		mutex_unlock(&drive->mutex);

	return (0);
}


int
init_drives(
    library_t *library,
    dev_ptr_tbl_t *dev_ptr_tbl)
{
	int		i;
	dev_ent_t	*un;
	drive_state_t  *drive, *nxt_drive;

	drive = NULL;
	/*
	 * For each drive, build the drive state structure, put the init
	 * request on the list and start a thread with a new lwp.
	 */
	for (i = 0; i <= dev_ptr_tbl->max_devices; i++)
		if (dev_ptr_tbl->d_ent[i] != 0) {
			un = (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->d_ent[i]);
			if (un->fseq == library->eq && !IS_ROBOT(un)) {

				nxt_drive = malloc_wait(sizeof (drive_state_t),
				    5, 0);
				nxt_drive->previous = drive;
				if (drive != NULL)
					drive->next = nxt_drive;
				else
					library->drive = nxt_drive;
				drive = nxt_drive;

				(void) memset(drive, 0, sizeof (drive_state_t));
				drive->library = library;
				drive->open_fd = -1;
				drive->un = un;

				mutex_lock(&drive->mutex);
				drive->un->slot = drive->new_slot
				    = ROBOT_NO_SLOT;
				drive->un->mid = drive->un->flip_mid
				    = ROBOT_NO_SLOT;
				library->countdown++;
				drive->active_count = 1;
				drive->first =
				    malloc_wait(sizeof (robo_event_t), 5, 0);
				(void) memset(drive->first, 0,
				    sizeof (robo_event_t));
				drive->first->type = EVENT_TYPE_INTERNAL;
				drive->first->status.bits = REST_FREEMEM;
				drive->first->request.internal.command
				    = ROBOT_INTRL_INIT;

				if (thr_create(NULL, MD_THR_STK, &drive_thread,
				    (void *) drive,
				    (THR_NEW_LWP | THR_BOUND | THR_DETACHED),
				    &drive->thread)) {
					sam_syslog(LOG_ERR,
					    "Unable to create thread"
					    " drive_thread: %m.");
					drive->status.b.offline = TRUE;
					drive->thread = (thread_t)- 1;
				}
			}
		}
	return (0);
}

/*
 * Initialize a drive
 */
void
init_drive(
	drive_state_t *drive)
{
	char	   *ent_pnt = "init_drive";
	char	   *d_mess = drive->un->dis_mes[DIS_MES_NORM];
	char	   *dc_mess = drive->un->dis_mes[DIS_MES_CRIT];

	mutex_lock(&drive->mutex);
	mutex_lock(&drive->un->mutex);
	drive->un->scan_tid = thr_self();

	memccpy(d_mess, catgets(catfd, SET, 9465, "initializing"), '\0',
	    DIS_MES_LEN);
	if (drive->un->state < DEV_IDLE) {
		int	local_open, err, drive_status, drive_state;
		int	down_it = FALSE;
		char   *dev_name;

		drive->un->status.bits |= DVST_REQUESTED;
		drive->status.b.full = FALSE;
		mutex_unlock(&drive->mutex);
		mutex_unlock(&drive->un->mutex);
		err = query_drive(drive->library, drive, &drive_status,
		    &drive_state);
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG, "from query drive(%d:%d).",
			    err, drive_status);

		mutex_lock(&drive->mutex);
		mutex_lock(&drive->un->mutex);
		/*
		 * If the drive is not online or the query failed, then down
		 * the drive
		 */
		if (err) {
			sam_syslog(LOG_INFO, "helper-%s:status:%s",
			    ent_pnt, sam_sony_status(err));
			if (((err & PSC_BINSTS_CASSIN) ||
			    (err & PSC_BINSTS_ADAPTORIN) ||
			    (err & PSC_BINSTS_LOADED)) &&
			    (drive->un->vsn[0] != '\0')) {
				drive->status.b.full = TRUE;
			} else {
				down_it = TRUE;
			}
		}
		if (down_it) {
			memccpy(dc_mess, catgets(catfd, SET, 9453,
			    "drive set off due to"
			    " PSC reported state"), '\0', DIS_MES_LEN);
			sam_syslog(LOG_INFO,
			    catgets(catfd, SET, 9454, "%s: Drive (%d) set down:"
			" PSC reported state."), ent_pnt, drive->un->eq);
			drive->un->state = DEV_DOWN;
			drive->status.b.offline = TRUE;
			drive->un->status.bits &= ~DVST_REQUESTED;
			mutex_unlock(&drive->mutex);
			mutex_unlock(&drive->un->mutex);
			mutex_lock(&drive->library->mutex);
			drive->library->countdown--;
			mutex_unlock(&drive->library->mutex);
			return;
		}
		if (IS_TAPE(drive->un))
			ChangeMode(drive->un->name, SAM_TAPE_MODE);
		if (IS_OPTICAL(drive->un))
			dev_name = &drive->un->name[0];
		else
			dev_name = samst_devname(drive->un);

		drive->un->status.bits |= DVST_READ_ONLY;
		local_open = open_unit(drive->un, dev_name, 1);
		clear_driver_idle(drive, local_open);
		mutex_lock(&drive->un->io_mutex);
		if (scsi_cmd(local_open, drive->un, SCMD_TEST_UNIT_READY, 10)
		    >= 0)
			drive->status.b.full = TRUE;

		close_unit(drive->un, &local_open);
		if (dev_name != &drive->un->dt.tp.samst_name[0])
			free(dev_name);

		mutex_unlock(&drive->un->io_mutex);
		mutex_unlock(&drive->un->mutex);

		if (drive->status.b.full) {
			if (!(drive->un->flags & DVFG_SHARED)) {
				if (clear_drive(drive))
					down_drive(drive, SAM_STATE_CHANGE);
			} else {
				struct CatalogEntry ced;
				struct CatalogEntry *ce = &ced;

				/*
				 * Only unload the drive if this volume
				 * belongs to this SAM-FS (i.e., we find this
				 * volume in our catalog)
				 */
				if (((ce = CatalogGetEntry(&drive->un->i,
				    &ced)) != NULL) &&
				    (!(ce->CeStatus & CES_occupied))) {
					if (clear_drive(drive))
						down_drive(drive,
						    SAM_STATE_CHANGE);
				} else {
					drive->status.b.full = FALSE;
					memccpy(d_mess,
					    catgets(catfd, SET, 9206, "empty"),
					    '\0', DIS_MES_LEN);
				}
			}
		}
		mutex_unlock(&drive->mutex);
		mutex_lock(&drive->un->mutex);
		if (drive->open_fd >= 0)
			close_unit(drive->un, &drive->open_fd);

		drive->un->status.bits &= ~(DVST_READ_ONLY | DVST_REQUESTED);
		mutex_unlock(&drive->un->mutex);
	} else {
		mutex_unlock(&drive->mutex);
		drive->un->status.bits &= ~(DVST_READ_ONLY | DVST_REQUESTED);
		mutex_unlock(&drive->un->mutex);
		*d_mess = '\0';
	}

	mutex_lock(&drive->library->mutex);
	drive->library->countdown--;
	mutex_unlock(&drive->library->mutex);
	if (drive->un->state < DEV_IDLE)
		*dc_mess = *d_mess = '\0';
}

/*
 * re_init_library
 */

int
re_init_library(
	library_t *library)
{
	return (0);
}

void
start_helper(
	library_t *library)
{
	char	   *ent_pnt = "start_helper";
	pid_t	   pid = -1;
	char	    path[512], *lc_mess = library->un->dis_mes[DIS_MES_CRIT];
	int		fd;

	sprintf(path, "%s/sam-sony_helper", SAM_EXECUTE_PATH);
	/*
	 * Set non-standard files to close on exec.
	 */
	for (fd = STDERR_FILENO + 1; fd < OPEN_MAX; fd++) {
		(void) fcntl(fd, F_SETFD, FD_CLOEXEC);
	}
	while (pid < 0) {
		if ((pid = fork1()) == 0) {
			char	    shmid[12], equ[12];

			setgid(0);	/* clear special group id */

			sprintf(shmid, "%#d", master_shm.shmid);
			sprintf(equ, "%#d", library->eq);
			execl(path, "sam-sony_helper", shmid, equ, NULL);
			/*
			 * Can't translate since the fd is closed
			 */
			sprintf(lc_mess, "helper did not start: %s",
			    error_handler(errno));
			_exit(1);
		}
		if (pid < 0)
			sam_syslog(LOG_ERR,
			    "%s: Unable to start sam-sony_helper:%s.",
			    ent_pnt, error_handler(errno));
		sleep(5);
	}

	library->helper_pid = pid;
}
