/*
 *  init.c - initialize the library and all it elements
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

#include <stdio.h>
#include <thread.h>
#include <synch.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
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
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/trace.h"
#include "aml/dev_log.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/tapealert.h"

#include "generic.h"

#pragma ident "$Revision: 1.34 $"

/* Using __FILE__ makes duplicate strings */
static char    *_SrcFile = __FILE__;


/*	function prototypes */
int		g__init_drives(library_t *, dev_ptr_tbl_t *);
void	    start_helper(library_t *);
int	query_drive(library_t *library,
    drive_state_t *drive, int *d_errno);

/*	globals */
extern shm_alloc_t master_shm, preview_shm;
extern void	set_catalog_tape_media(library_t *library);
static char    *api_client, *api_server;
char	   *debug_device_name = NULL;

/*	This is the map for robot type to helper name */
typedef struct {
	dtype_t	 help_type;
	char	   *help_name;
}		helper_map;

helper_map	Helper_map[] = {
	DT_GRAUACI, "sam-grau_helper",
	(dtype_t)0, (char *)0
};


/*
 *	api_initialize - initialize the library
 *
 * exit -
 *	  0 - ok
 *	 !0 - failed
 */
int
api_initialize(
		library_t *library,
		dev_ptr_tbl_t *dev_ptr_tbl)
{
#if !defined(SAM_OPEN_SOURCE)
	int		i, fatal = 0;
	char	   *ent_pnt = "api_initialize";
	char	   *l_mess = library->un->dis_mes[DIS_MES_NORM];
	char	   *lc_mess = library->un->dis_mes[DIS_MES_CRIT];
	char	   *line, *tmp;
	char	   *err_fmt = catgets(catfd, SET, 9176,
	    "%s: Syntax error in aci configuration file line %d.");
	char	   *fat_mess = catgets(catfd, SET, 9178,
	    "fatal error during initialization, see log");
	FILE	   *open_str;
	drive_state_t  *drive;


	/* Process API configuration file */
	if ((open_str = fopen(library->un->name, "r")) == NULL) {
		sam_syslog(LOG_CRIT, catgets(catfd, SET, 9117,
		    "%s: Unable to open"
		    " configuration file(%s): %m."), ent_pnt,
		    library->un->name);
		memccpy(lc_mess, catgets(catfd, SET, 9118,
		    "unable to open configuration file"),
		    '\0', DIS_MES_LEN);
		return (-1);
	}
	i = 0;
	line = malloc_wait(200, 2, 0);
	mutex_lock(&library->un->mutex);

	if (g__init_drives(library, dev_ptr_tbl)) {
		mutex_unlock(&library->un->mutex);
		return (-1);
	}
	library->index = library->drive;

#if defined(DEBUG)
	memccpy(l_mess, "Initializing library", '\0', DIS_MES_LEN);
#endif
	while (fgets(memset(line, 0, 200), 200, open_str) != NULL) {
		i++;
		if (*line == '#' || strlen(line) == 0 || *line == '\n')
			continue;

		if ((tmp = strchr(line, '#')) != NULL)
			memset(tmp, 0, (200 - (tmp - line)));

		if ((tmp = strtok(line, "= \t\n")) == NULL)
			continue;

		if (strcasecmp(tmp, "client") == 0) {
			if ((tmp = strtok(NULL, " =\t\n")) == NULL) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}
			library->api_client = strdup(tmp);
			continue;
		} else if (strcasecmp(tmp, "server") == 0) {
			if ((tmp = strtok(NULL, " =\t\n")) == NULL) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}
			library->api_server = strdup(tmp);
			continue;
		} else if (strcasecmp(tmp, "acidrive") == 0) {
			char	   *rmtname;
			drive_state_t  *drive;

			if ((tmp = strtok(NULL, " =\t\n")) == NULL) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}
			if ((rmtname = strtok(NULL, " =\t\n")) == NULL) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}
			for (drive = library->drive; drive != NULL;
			    drive = drive->next) {
				if (strcmp(drive->un->name, rmtname) == 0) {
					drive->aci_drive_entry =
					    malloc_wait(
					    sizeof (aci_drive_entry_t), 2, 0);
					if ((int)strlen(tmp) > ACI_DRIVE_LEN) {
			/* bad indentation due to cstyle */
			sam_syslog(LOG_INFO, catgets(catfd, SET, 9179,
			    "%s: Drive name(%s) too long line %d."),
			    ent_pnt, tmp, i);
					fatal++;
					} else
					strcpy(
					    drive->aci_drive_entry->drive_name,
					    tmp);
					break;
				}
			}
			if (drive == NULL) {
				sam_syslog(LOG_INFO, catgets(catfd, SET, 9180,
				    "%s: Cannot find drive(%s) line %d."),
				    ent_pnt, rmtname, i);
				fatal++;

			}
			continue;
		} else if (strcasecmp(tmp, "import") == 0) {
			if ((tmp = strtok(NULL, " =\t\n")) == NULL) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}
			library->api_import = strdup(tmp);
			continue;
		} else if (strcasecmp(tmp, "export") == 0) {
			if ((tmp = strtok(NULL, " =\t\n")) == NULL) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}
			library->api_export = strdup(tmp);
			continue;
		} else if (strcasecmp(tmp, "debugtty") == 0) {
			if ((tmp = strtok(NULL, " =\t\n")) == NULL) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}
			debug_device_name = strdup(tmp);
		} else {
			sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
			continue;
		}
	}
	fclose(open_str);

	if (debug_device_name == NULL)
		debug_device_name = strdup("/dev/null");

	/* Make sure all the drives exist */
	for (drive = library->drive; drive != NULL; drive = drive->next) {
		if (drive->aci_drive_entry == NULL) {
			fatal++;
			sam_syslog(LOG_ERR,
			    catgets(catfd, SET, 9181,
			    "%s: No acidrive entry for drive(%s) %d."),
			    ent_pnt, drive->un->name, drive->un->eq);
		}
	}

	if (fatal) {
		memccpy(lc_mess, fat_mess, '\0', DIS_MES_LEN);
		return (-1);
	}
	/* Must have CLIENT name and SERVER name */
	if ((library->api_client == NULL) ||
	    (library->api_server == NULL)) {
		sam_syslog(LOG_ERR,
		    catgets(catfd, SET, 9182,
		    "%s: Client or server not defined."), ent_pnt);
		memccpy(lc_mess, fat_mess, '\0', DIS_MES_LEN);
		return (-1);
	}

	/* Set up environment for Grau */
	if (library->un->equ_type == DT_GRAUACI) {
		sprintf(line, "DAS_CLIENT=%s", library->api_client);
		api_client = strdup(line);
		if (putenv(api_client)) {
			memccpy(lc_mess, fat_mess, '\0', DIS_MES_LEN);
			sam_syslog(LOG_ERR, "%s: putenv: CLIENT: %m", ent_pnt);
			return (-1);
		}
		sam_syslog(LOG_INFO, catgets(catfd, SET, 9183,
		    "%s: Setting API environment %s."), ent_pnt, line);
		sprintf(line, "DAS_SERVER=%s", library->api_server);
		api_server = strdup(line);
		if (putenv(api_server)) {
			memccpy(lc_mess, fat_mess, '\0', DIS_MES_LEN);
			sam_syslog(LOG_ERR, "%s: putenv: SERVER: %m", ent_pnt);
			return (-1);
		}
		sam_syslog(LOG_INFO, catgets(catfd, SET, 9183,
		    "%s: Setting API environment %s."), ent_pnt, line);
	}
	free(line);

	i = 0664;
	library->un->status.b.present = TRUE;
	library->un->status.b.requested = TRUE;
	library->status.b.passthru = 0;

	if (CatalogInit("grau") == -1) {
		sam_syslog(LOG_ERR, "%s",
		    catgets(catfd, SET, 2364,
		    "Catalog initialization failed!"));
		exit(1);
	}
	/* Get the catalog media type from the first active drive */
	set_catalog_tape_media(library);

	if (is_optical(library->un->media))
		library->status.b.two_sided = 1;

	if (!(is_optical(library->un->media)) &&
	    !(is_tape(library->un->media))) {
		sam_syslog(LOG_INFO, catgets(catfd, SET, 9357,
		"Unable to determine media type. Library daemon exiting."));
		exit(1);
	}
	library->status.b.passthru = 0;

	(void) CatalogSetCleaning(library->eq);

	mutex_unlock(&library->un->mutex);

	library->transports = malloc_wait(sizeof (xport_state_t), 5, 0);
	(void) memset(library->transports, 0, sizeof (xport_state_t));
	cond_init(&library->transports->condit, USYNC_PROCESS, NULL);

	mutex_lock(&library->transports->mutex);
	library->transports->library = library;
	library->transports->first = get_free_event(library);
	ETRACE((LOG_NOTICE, "EV:FrTr:init trans:%#x.",
	    library->transports->first));
	(void) memset(library->transports->first, 0, sizeof (robo_event_t));
	library->transports->first->type = EVENT_TYPE_INTERNAL;
	library->transports->first->status.bits = REST_FREEMEM;
	library->transports->first->request.internal.command = ROBOT_INTRL_INIT;
	library->transports->active_count = 1;
	library->helper_pid = -1;
	memccpy(l_mess, catgets(catfd, SET, 9151,
	    "waiting for helper to start"),
	    '\0', DIS_MES_LEN);
	start_helper(library);
#if defined(FUJITSU_SIMULATOR)
	sleep(60);
#endif
	{
		char *MES_9152 = catgets(catfd, SET, 9152,
		    "helper running as pid %d");
		char *mes = (char *)malloc_wait(
		    strlen(MES_9152) + 15, 5, 0);

		sprintf(mes, MES_9152, library->helper_pid);
		memccpy(l_mess, mes, '\0', DIS_MES_LEN);
		free(mes);
	}

	if (thr_create(NULL, MD_THR_STK, &transport_thread,
	    (void *) library->transports,
	    (THR_NEW_LWP | THR_BOUND | THR_DETACHED),
	    &library->transports->thread)) {
	sam_syslog(LOG_ERR,
	    "Unable to create thread transport_thread: %m");
		library->transports->thread = (thread_t)- 1;
	}
	mutex_unlock(&library->transports->mutex);

	/* free the drive threads */
	for (drive = library->drive; drive != NULL; drive = drive->next)
		mutex_unlock(&drive->mutex);

#if defined(DEBUG)
	memccpy(l_mess, "Initialization complete", '\0', DIS_MES_LEN);
#endif
	return (0);
#endif
}


int
g__init_drives(
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
				nxt_drive = malloc_wait(
				    sizeof (drive_state_t), 5, 0);
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
				/* hold the lock until ready */
				mutex_lock(&drive->mutex);
				drive->un->slot = ROBOT_NO_SLOT;
				drive->new_slot = ROBOT_NO_SLOT;
				drive->un->mid = ROBOT_NO_SLOT;
				drive->un->flip_mid = ROBOT_NO_SLOT;
				drive->active_count = 1;
				/*
				 * Cannot take event from the library free
				 * list as it has not been created yet.
				 */
				drive->first = malloc_wait(
				    sizeof (robo_event_t), 5, 0);
				ETRACE((LOG_NOTICE,
				    "EV:FrTr:init drive(%d):%#x.", un->eq,
				    drive->first));
				(void) memset(drive->first, 0,
				    sizeof (robo_event_t));
				drive->first->type = EVENT_TYPE_INTERNAL;
				drive->first->status.bits = REST_FREEMEM;
				drive->first->request.internal.command =
				    ROBOT_INTRL_INIT;

				mutex_lock(&un->mutex);
				if ((un->state < DEV_IDLE) &&
				    (un->st_rdev == 0) && IS_OPTICAL(un)) {
					int		local_open;
					struct stat	stat_buf;

					local_open = open_unit(un, un->name, 1);
					clear_driver_idle(drive, local_open);
					if (fstat(local_open, &stat_buf))
						DevLog(DL_SYSERR(5188),
						    un->name);
					else
						un->st_rdev = stat_buf.st_rdev;

					close_unit(un, &local_open);
				}
				mutex_unlock(&un->mutex);

				if (thr_create(NULL, MD_THR_STK,
				    &drive_thread, (void *) drive,
				    (THR_NEW_LWP | THR_BOUND | THR_DETACHED),
				    &drive->thread)) {
		sam_syslog(LOG_ERR,
		    "Unable to create thread drive_thread: %m.");
					drive->status.b.offline = TRUE;
					drive->thread = (thread_t)- 1;
				}
			}
		}
	return (0);
}


/*
 *	api_re_init_library -
 */
int
api_re_init_library(
		    library_t *library)
{
	return (0);
}


/*
 *	api_init_drive - initialize drive.
 *
 */
void
api_init_drive(
		drive_state_t *drive)
{
#if !defined(SAM_OPEN_SOURCE)
	int		local_retry = -1;
	int		last_derrno = -1;
	int		d_errno;
	char	   *ent_pnt = "api_init_drive";
	char	   *d_mess = drive->un->dis_mes[DIS_MES_NORM];
	char	   *dc_mess = drive->un->dis_mes[DIS_MES_CRIT];
	api_errs_t	ret = API_ERR_OK;

#if defined(FUJITSU_SIMULATOR_XX)
#else
	sleep(4);
	mutex_lock(&drive->mutex);
	mutex_lock(&drive->un->mutex);
	drive->un->scan_tid = thr_self();
#if defined(DEBUG)
	memccpy(d_mess, "setting drive access", '\0', DIS_MES_LEN);
#endif
	while (local_retry != 0) {
		int		d_errno;

		if (aci_drive_access(drive->library, drive, &d_errno)) {
			if (d_errno == 0) {	/* if call did not happen */
				ret = API_ERR_TR;
				local_retry = 0;
			} else if (local_retry < 0 || last_derrno != d_errno) {
				last_derrno = d_errno;
				api_valid_error(drive->library->un->type,
				    d_errno, (dev_ent_t *)0);
				local_retry = api_return_retry(
				    drive->library->un->type,
				    d_errno);
				if (DBG_LVL(SAM_DBG_DEBUG))
					sam_syslog(LOG_DEBUG,
					    "dismount(%d): %s.",
					    drive->un->eq,
					    api_return_message(
					    drive->library->un->type,
					    d_errno));
				ret = api_return_degree(
				    drive->library->un->type, d_errno);
				sleep(api_return_sleep(
				    drive->library->un->type, d_errno));
			}
			if (local_retry > 0) {
				local_retry--;
				continue;
			}
			memccpy(dc_mess, catgets(catfd, SET, 9175,
			    "drive_access denied"),
			    '\0', DIS_MES_LEN);
			sam_syslog(LOG_ERR,
			    catgets(catfd, SET, 9174,
			    "%s: Drive (%d)access denied."),
			    ent_pnt, drive->un->eq);
			drive->un->state = DEV_DOWN;
		}
		break;		/* access worked */
	}
	if (ret == API_ERR_DL)
		down_library(drive->library, SAM_STATE_CHANGE);
	else if (ret == API_ERR_DD)
		down_drive(drive, SAM_STATE_CHANGE);
#endif

	if (drive->un->state < DEV_IDLE) {
		int		local_open;
		int		retry = 3;
		char	   *dev_name;

		drive->un->status.bits |= DVST_REQUESTED;
		if (IS_OPTICAL(drive->un))
			dev_name = &drive->un->name[0];
		else
			dev_name = samst_devname(drive->un);

		drive->un->status.bits |= DVST_READ_ONLY;
		local_open = open_unit(drive->un, dev_name, 1);
		clear_driver_idle(drive, local_open);
		mutex_lock(&drive->un->io_mutex);

		/* assume there might be something in the drive */
		drive->status.b.full = TRUE;
		/*
		 * try to determine if the drive is empty (TUR fails w/ not
		 * ready)
		 */
#if defined(FUJITSU_SIMULATOR_XX)
#else
		while (retry--) {
			if ((scsi_cmd(local_open, drive->un,
			    SCMD_TEST_UNIT_READY, 10) < 0)) {
				TAPEALERT_SKEY(local_open, drive->un);
				if (process_scsi_error(drive->un, NULL, 0) ==
				    ERR_SCSI_NOTREADY) {
					drive->status.b.full = FALSE;
					break;
				}
				sleep(1);
			} else	/* TUR reported media loaded and ready, */
				break; /* unload it */
		}
#endif

		if ((!(IS_OPTICAL(drive->un))) &&
		    (dev_name != (char *)drive->un->dt.tp.samst_name))
			free(dev_name);

		close_unit(drive->un, &local_open);
		mutex_unlock(&drive->un->io_mutex);

		/* assume drive is empty */
		drive->status.b.full = FALSE;
		if (!query_drive(drive->library, drive, &d_errno)) {
			if (drive->aci_drive_entry->volser[0] != '\0') {
				drive->status.b.full = TRUE;
				memcpy(drive->bar_code,
				    drive->aci_drive_entry->volser,
				    ACI_VOLSER_LEN);
			}
		}
		mutex_unlock(&drive->un->mutex);

		if (drive->status.b.full) {
			if (clear_drive(drive))
				down_drive(drive, SAM_STATE_CHANGE);
		}
		mutex_unlock(&drive->mutex);

		mutex_lock(&drive->un->mutex);
		if (drive->open_fd >= 0)
			close_unit(drive->un, &drive->open_fd);

		drive->un->status.bits &= ~(DVST_READ_ONLY | DVST_REQUESTED);
		mutex_unlock(&drive->un->mutex);

	} else {
		drive->un->status.bits &= ~(DVST_READ_ONLY | DVST_REQUESTED);
		mutex_unlock(&drive->mutex);
		mutex_unlock(&drive->un->mutex);
	}

#endif
}


char	   *
set_helper_name(
		dtype_t type)
{
	helper_map	*xx = Helper_map;

	for (xx = Helper_map; xx->help_name != (char *)0; xx++) {
		if (xx->help_type == type) {
			return (xx->help_name);
		}
	}
	return ((char *)0);
}


void
start_helper(
		library_t *library)
{
	pid_t	   pid = -1;
	char	   *ent_pnt = "start_helper";
	char	    path[512], *lc_mess = library->un->dis_mes[DIS_MES_CRIT];

	library->api_helper_name = set_helper_name(library->un->type);
	if (library->api_helper_name == (char *)0)
		library->api_helper_name = "sam-grau_helper";
	sprintf(path, "%s/%s", SAM_EXECUTE_PATH, library->api_helper_name);

	while (pid < 0) {
		int		fd;

		/* Set non-standard files to close on exec. */
		for (fd = STDERR_FILENO + 1; fd < OPEN_MAX; fd++) {
			(void) fcntl(fd, F_SETFD, FD_CLOEXEC);
		}

		if ((pid = fork1()) == 0) {	/* we are the child */
			char	    shmid[12], equ[12];

			setgid(0);	/* clear special group id */

			sprintf(shmid, "%#d", master_shm.shmid);
			sprintf(equ, "%#d", library->eq);
			execl(path, library->api_helper_name, shmid, equ, NULL);
			sprintf(lc_mess, "helper did not start: %s",
			    error_handler(errno));
			_exit(1);
		}
		if (pid < 0)
			sam_syslog(LOG_ERR, "%s: Unable to start %s: %s.",
			    ent_pnt, library->api_helper_name,
			    error_handler(errno));

		sleep(5);
	}

	library->helper_pid = pid;
}


/*
 *	query_drive - get information for a drive.
 * grau version, only needed for initialization.
 */
int
query_drive(
	    library_t *library,
	    drive_state_t *drive,
	    int *d_errno)
{
#if !defined(SAM_OPEN_SOURCE)
	int		err;
	char	   *ent_pnt = "query_drive";
	char	   *d_mess = drive->un->dis_mes[DIS_MES_NORM];
	xport_state_t  *transport;
	robo_event_t   *query;
	aci_information_t *aci_info;
	dev_ent_t	*un = drive->un;

	aci_info = malloc_wait(sizeof (aci_information_t), 2, 0);
	memset(aci_info, 0, sizeof (aci_information_t));
	aci_info->aci_drive_entry = drive->aci_drive_entry;

	/* Build transport thread request */
	query = get_free_event(library);
	(void) memset(query, 0, sizeof (robo_event_t));

	query->request.internal.command = ROBOT_INTRL_QUERY_DRIVE;
	query->request.internal.address = (void *) aci_info;
	if (DBG_LVL(SAM_DBG_DEBUG))
		memccpy(d_mess, ent_pnt, '\0', DIS_MES_LEN);

	DevLog(DL_DEBUG(6012), drive->aci_drive_entry->drive_name);

	query->type = EVENT_TYPE_INTERNAL;
	query->status.bits = REST_SIGNAL;
	query->completion = REQUEST_NOT_COMPLETE;
	transport = library->transports;
	mutex_lock(&query->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0) {
		ETRACE((LOG_NOTICE, "EV:FrTr:query: %#x.", query));
		transport->first = query;
	} else {
		robo_event_t   *tmp;

		ETRACE((LOG_NOTICE, "EV:ApTr:query: %#x(%d).", query,
		    transport->active_count));
		LISTEND(transport, tmp);
		append_list(tmp, query);
	}
	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);

	/* Wait for the transport to do the command */
	while (query->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&query->condit, &query->mutex);
	mutex_unlock(&query->mutex);

	*d_errno = err = query->completion;
	ETRACE((LOG_NOTICE, "EV:RtTr:query_drive: %#x.", query));
	DevLog(DL_DEBUG(6013), err ? error_handler(err) : "no error");

	/*
	 * No error means the aci command was started and that the response
	 * is in the returned data.
	 */
	if (!err) {
	api_resp_api_t *response =
	    (api_resp_api_t *)& query->request.message.param.start_of_request;

		/* Did the aci call return an error */
		if ((err = response->api_status)) {
			*d_errno = response->d_errno;
			memccpy(d_mess, catgets(catfd, SET, 9194,
			    "api response error"),
			    '\0', DIS_MES_LEN);
			if ((*d_errno >= 0) && (*d_errno < NO_ECODES)) {
				DevLog(DL_DETAIL(6014), err, response->d_errno,
				    grau_messages[*d_errno].mess);
			} else {
				DevLog(DL_DETAIL(6014), err,
				    response->d_errno, "unknown error");
			}
		}
		memcpy(drive->aci_drive_entry->volser,
		    &response->data.drive_ent.volser,
		    ACI_VOLSER_LEN);
	} else {
		/*
		 * This is a call error which indicates a problem with
		 * protocol between the transport and the helper. Should
		 * really never happen in the field. Details of the error
		 * have been logged by the helper.
		 */
		memccpy(d_mess, catgets(catfd, SET, 9196, "protocol error"),
		    '\0', DIS_MES_LEN);
		*d_errno = 0;
		DevLog(DL_DETAIL(6015), error_handler(err));
	}

	mutex_destroy(&query->mutex);
	cond_destroy(&query->condit);
	free(query);
	return (err);
#endif
}
