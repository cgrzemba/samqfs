/*
 *	init.c - initialize the library and all its elements
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

#pragma ident "$Revision: 1.38 $"

static char *_SrcFile = __FILE__;

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
#include "aml/tapes.h"
#include "sam/defaults.h"
#include "aml/robots.h"
#include "ibm3494.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/catalog.h"
#include "aml/catlib.h"


/* function prototypes */
int init_drives(library_t *, dev_ptr_tbl_t *);

/* globals */
extern shm_alloc_t master_shm, preview_shm;
extern void set_catalog_tape_media(library_t *library);


/*
 *	initialize - initialize the library
 *
 * exit -
 *    0 - ok
 *   !0 - failed
 */
int
initialize(
	library_t *library,
	dev_ptr_tbl_t *dev_ptr_tbl)
{
	int	i, fatal = 0;
	char 	mess[DIS_MES_LEN];
	char 	*line, *tmp;
	char *err_fmt = "init(%d): Syntax error in configuration file line %d.";
	FILE 	*open_str;
	drive_state_t 	*drive;

	library->sam_category = NORMAL_CATEGORY;
	if ((open_str = fopen(library->un->name, "r")) == NULL) {
		sam_syslog(LOG_CRIT,
		    "init(%d):Unable to open configuration file: %m.", LIBEQ);
		return (-1);
	}
	i = 0;
	line = malloc_wait(200, 2, 0);
	mutex_lock(&library->un->mutex);

	if (init_drives(library, dev_ptr_tbl)) {
		mutex_unlock(&library->un->mutex);
		return (-1);
	}
	library->index = library->drive;
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
				sam_syslog(LOG_WARNING, err_fmt, LIBEQ, i);
				continue;
			}
			if (strlen(tmp) >
			    (sizeof (vsn_t) + 9 + 17 + sizeof (SAM_CDB_LENGTH)))
				sam_syslog(LOG_WARNING,
				    "name too long - ignored.");
			else
				memcpy(&library->un->vsn, tmp, strlen(tmp) + 1);
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "init(%d): Setting name to %s.", LIBEQ,
				    &library->un->vsn);
			continue;
		} else if (strcasecmp(tmp, "access") == 0) {
			if ((tmp = strtok(NULL, "= \t\n")) == NULL) {
				sam_syslog(LOG_WARNING, err_fmt, LIBEQ, i);
				continue;
			}
			if (strcasecmp(tmp, "private") == 0) {
				library->options.b.shared = FALSE;
				library->un->dt.rb.status.b.shared_access
				    = FALSE;
			} else if (strcasecmp(tmp, "shared") == 0) {
				library->options.b.shared = TRUE;
				library->un->dt.rb.status.b.shared_access
				    = TRUE;
			} else
				sam_syslog(LOG_INFO,
				    "init(%d): Bad option for access (%s)"
				    " line %d.", LIBEQ, tmp, i);
			continue;
		} else if (strcasecmp(tmp, "category") == 0) {
			uint_t l_category;
			if ((tmp = strtok(NULL, "= \t\n")) == NULL) {
				sam_syslog(LOG_WARNING, err_fmt, LIBEQ, i);
				continue;
			}
			l_category = strtol(tmp, NULL, 10);
			if (l_category < 1 || l_category > 0xfeff)
				sam_syslog(LOG_INFO, "init(%d): Bad"
				    " category(%s) line %d, using %#x.",
				    LIBEQ, tmp, i, library->sam_category);
			else
				library->sam_category = l_category;
			continue;
		} else if (*tmp == '/') {
			int drive_id;
			char *rmt_name = tmp;
			char *shared;

			if ((tmp = strtok(NULL, "= \t\n")) == NULL) {
				sam_syslog(LOG_WARNING, err_fmt, LIBEQ, i);
				continue;
			}
			drive_id = strtol(tmp, NULL, 16);

			shared = strtok(NULL, " =\t\n");

			for (drive = library->drive; drive != NULL;
			    drive = drive->next) {
				if (strcmp(drive->un->name, rmt_name) == 0) {
					if (shared != NULL) {
						if (strcasecmp(shared,
						    "shared") == 0) {
							drive->un->flags =
							    DVFG_SHARED;
							sam_syslog(LOG_INFO,
							    "Drive %d is a"
							    " shared drive.",
							    drive->un->eq);
						}
					}
					if (DBG_LVL(SAM_DBG_DEBUG))
						sam_syslog(LOG_DEBUG,
						    "init(%d):setting drive %s"
						    " to id %#8.8x.",
						    LIBEQ, rmt_name, drive_id);
					drive->drive_id = drive_id;
					break;
				}
			}

			if (drive == NULL) {
				sam_syslog(LOG_WARNING,
				    "init(%d): Config error: drive name does"
				    " not match: line %d.", LIBEQ, i);
				fatal++;
			}
			continue;
		} else {
			sam_syslog(LOG_WARNING, err_fmt, LIBEQ, i);
			continue;
		}
	}
	fclose(open_str);

	if (library->un->vsn[0] == '\0') {
		sam_syslog(LOG_INFO,
		    "init(%d)No name defined in configuration file.", LIBEQ);
		fatal++;
	}
	for (drive = library->drive; drive != NULL; drive = drive->next) {
		if (drive->drive_id == 0) {
			drive->un->state = DEV_DOWN;
			drive->status.b.offline = TRUE;
			sam_syslog(LOG_ERR,
			    "init(%d): No entry for drive %s(%d) in"
			    " condifuration file.", LIBEQ, drive->un->name,
			    drive->un->eq);
		}
	}

	if (fatal)
		return (-1);

	sam_syslog(LOG_INFO, "init(%d): Library name is %s.", LIBEQ,
	    library->un->vsn);
	sam_syslog(LOG_INFO, "init(%d): Library access is %s.", LIBEQ,
	    library->options.b.shared ? "shared" : "private");
	sam_syslog(LOG_INFO, "init(%d): Library category is %#x.", LIBEQ,
	    library->sam_category);
	while ((library->open_fd = open_ibmatl(library->un->vsn)) < 0) {
		int loc_err = errno;

		if (loc_err == ENODEV) {
			sprintf(mess,
			    "init(%d): Library %s is not known to lmcpd.",
			    LIBEQ, library->un->vsn);
			sam_syslog(LOG_INFO, mess);
		} else if (loc_err == EIO || loc_err == ECONNREFUSED) {
			sprintf(mess,
			    "init(%d): lmcpd daemon is not running correctly.",
			    LIBEQ);
			sam_syslog(LOG_INFO, mess);
		} else {
			sprintf(mess,
			    "init(%d): Unable to open library:%s", LIBEQ,
			    strerror(errno));
			sam_syslog(LOG_INFO, mess);
			sam_syslog(LOG_INFO, "init(%d): is lmcpd running?",
			    LIBEQ);
		}
		post_dev_mes(library->un, mess, DIS_MES_CRIT);
		sleep(10);
		library->un->dis_mes[DIS_MES_CRIT][0] = '\0';
		continue;
	}

	/* Get the catalog media type from the first active drive */
	set_catalog_tape_media(library);

	if (!(is_optical(library->un->media)) &&
	    !(is_tape(library->un->media))) {
		sam_syslog(LOG_INFO,
		    "Unable to determine media type. Library daemon exiting.");
		exit(1);
	}

	wait_library_ready(library);
	library->un->status.bits |= (DVST_PRESENT | DVST_REQUESTED);

	if (CatalogInit("ibm3494") == -1) {
		sam_syslog(LOG_ERR, "Catalog initialization failed!");
		exit(1);
	}

	library->catalog_fd = -1;
	library->status.b.two_sided = library->un->media == DT_ERASABLE;
	library->status.b.passthru = 0;

	mutex_unlock(&library->un->mutex);

	/* allocate the free list */
	library->free = init_list(ROBO_EVENT_CHUNK);
	library->free_count = ROBO_EVENT_CHUNK;
	mutex_init(&library->free_mutex, USYNC_THREAD, NULL);
	mutex_init(&library->list_mutex, USYNC_THREAD, NULL);
	mutex_init(&library->dlist_mutex, USYNC_THREAD, NULL);
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
	if (thr_create(NULL, MD_THR_STK, &transport_thread,
	    (void *)library->transports,
	    (THR_NEW_LWP | THR_BOUND | THR_DETACHED),
	    &library->transports->thread)) {
		sam_syslog(LOG_CRIT,
		    "init(%d): Unable to start transport thread.", LIBEQ);
		library->transports->thread = (thread_t)- 1;
	}
	mutex_unlock(&library->transports->mutex);
	mutex_unlock(&library->mutex);

	/* free the drive threads */
	for (drive = library->drive; drive != NULL; drive = drive->next)
		mutex_unlock(&drive->mutex);

	post_dev_mes(library->un, "Running.", DIS_MES_NORM);
	return (0);
}


int
init_drives(
	library_t *library,
	dev_ptr_tbl_t *dev_ptr_tbl)
{
	int 		i;
	dev_ent_t 	*un;
	drive_state_t *drive, *nxt_drive;

	drive = NULL;

	/*
	 * For each drive, build the drive state structure,
	 * put the init request on the list and start a thread with a
	 * new lwp.
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
				drive->drive_id = 0;
				/* hold lock until ready */
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
				    (void *)drive,
				    (THR_NEW_LWP | THR_BOUND | THR_DETACHED),
				    &drive->thread)) {
					sam_syslog(LOG_CRIT,
					    "init_drives(%d): Unable to start"
					    " thread: %m.", LIBEQ);
					drive->status.b.offline = TRUE;
					drive->thread = (thread_t)- 1;
				}
			}
		}
	return (0);
}


/*
 *	init_drive - initialize drive.
 */
void
init_drive(
	drive_state_t *drive)
{
	char 	*mess = &drive->un->dis_mes[DIS_MES_NORM][0];
	char 	*c_mess = &drive->un->dis_mes[DIS_MES_CRIT][0];
	library_t *library = drive->library;

	mutex_lock(&drive->mutex);
	mutex_lock(&drive->un->mutex);
	drive->un->scan_tid = thr_self();

	if (drive->un->state < DEV_IDLE) {
		int 	local_open;
		int 	drive_state, drive_status;
		char 	*dev_name;
		req_comp_t err;

		memccpy(mess, "Initializing.", '\0', DIS_MES_LEN);
		*c_mess = '\0';
		drive->un->status.bits |= DVST_REQUESTED;
		drive->status.b.full = FALSE;
		mutex_unlock(&drive->mutex);
		mutex_unlock(&drive->un->mutex);
		err = query_drive(library, drive, &drive_status, &drive_state);
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG,
			    "init_drive(%d): From query drive (%d:%d).",
			    LIBEQ, drive->un->eq, err);

		mutex_lock(&drive->mutex);
		mutex_lock(&drive->un->mutex);
		/*
		 * If the drive is not online or the query failed,
		 * then down the drive.
		 */
		if (err == MC_REQ_DD) {
			memccpy(c_mess,
			    "Query_drive reported error.", '\0', DIS_MES_LEN);
			sam_syslog(LOG_INFO, "init_drive(%d): Drive (%d) down: "
			    "Library reported state.", LIBEQ, drive->un->eq);
			drive->un->state = DEV_DOWN;
			drive->status.b.offline = TRUE;
			drive->un->status.bits &= ~DVST_REQUESTED;
			mutex_unlock(&drive->mutex);
			mutex_unlock(&drive->un->mutex);
			mutex_lock(&library->mutex);
			library->countdown--;
			mutex_unlock(&library->mutex);
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
		    >= 0) {
			drive->status.b.full = TRUE;
			memccpy(mess, "Initializing: Unloading media",
			    '\0', DIS_MES_LEN);
		}
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
				 * For shared drives, only unload the drive if
				 * this volume belongs to this SAM-FS
				 * (i.e., we find this volume in our catalog)
				 */
				if (((ce = CatalogGetEntry(&drive->un->i, &ced))
				    != NULL) &&
				    (!(ce->CeStatus & CES_occupied))) {
					if (clear_drive(drive))
						down_drive(drive,
						    SAM_STATE_CHANGE);
				} else {
					drive->status.b.full = FALSE;
					memccpy(mess, "empty",
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
		*mess = '\0';
	} else {
		drive->un->status.bits &= ~(DVST_READ_ONLY | DVST_REQUESTED);
		mutex_unlock(&drive->mutex);
		mutex_unlock(&drive->un->mutex);
	}

	mutex_lock(&library->mutex);
	library->countdown--;
	mutex_unlock(&library->mutex);
}


/*
 *	re_init_library -
 */
int
re_init_library(
	library_t *library)
{
	return (0);
}


/*
 *	reconcile_catalog is started after all drives have initialized
 */
void *
	reconcile_catalog(
	void *vlibrary)
{
	int 	err, seqno = 0;
	int 	status;
	int	i;
	int	n_entries;
	char 	*mess;
	uint_t 	vol_ser_count = 0x0fffffff;
	library_t		*library = (library_t *)vlibrary;
	IBM_query_info_t	*info;
	IBM_query_info_t	*query_data = NULL;
	struct VolId		vid;
	struct CatalogEntry	*list;
	struct CatalogEntry	ced;
	struct CatalogEntry	*ce;

	mess = library->un->dis_mes[DIS_MES_NORM];

	(void) CatalogSetCleaning(library->eq);

	/* look at all media in the sam_category and update the catalog */
	while (vol_ser_count) {
		int count;
		uint_t i_tmp;
		struct inv_recs *inv_entry;

		err = view_media_category(library, seqno, (void *)&info,
		    library->sam_category);
		if (vol_ser_count == 0x0fffffff) {
			memcpy(&vol_ser_count,
			    &info->data.cat_invent_data.no_vols[0],
			    sizeof (int));
			sprintf(mess, "Found %d volumes in category %#x.",
			    vol_ser_count, library->sam_category);
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "rec_cat(%d): %d volumes in category %#x",
				    LIBEQ, vol_ser_count,
				    library->sam_category);

		}
		memcpy(&i_tmp, &info->data.cat_invent_data.cat_seqno[0],
		    sizeof (i_tmp));
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG, "rec_cat(%d): seq %#x ret %#x.",
			    LIBEQ, seqno, i_tmp);

		seqno = i_tmp;
		inv_entry = &info->data.cat_invent_data.inv_rec[0];
		count = (vol_ser_count > 100) ? 100 : vol_ser_count;
		for (; count && vol_ser_count; count--, inv_entry++,
		    vol_ser_count--) {
			int k;
			char vsn[7];

			vsn[6] = '\0';
			memcpy(&vsn[0], &inv_entry->volser[0], 6);
			for (k = 5; k >= 0; k--) {
				if (vsn[k] != ' ')
					break;
				else
					vsn[k] = '\0';
			}

			if (vsn[0] == '\0' || vsn[0] == ' ') {
				sam_syslog(LOG_INFO,
				    "rec_cat(%d): Found empty vsn, quiting.",
				    LIBEQ);
				vol_ser_count = 0;
				break;
			}

			if ((err = view_media(library, vsn,
			    (void *)&query_data)) != MC_REQ_OK) {
				sam_syslog(LOG_WARNING,
				    "add to catalog failed(%d):%#x.",
				    library->un->eq, err);
				continue;
			} else if (library->un->media == DT_3592 &&
			    (!IS_3592_MEDIA(query_data->
			    data.expand_vol_data.vol_attr1))) {
				sam_syslog(LOG_WARNING, "add to catalog"
				    " failed(%d): %s is not a 3592",
				    library->un->eq, vsn);
				continue;
			} else if (library->un->media == DT_3590 &&
			    (!IS_3590_MEDIA(query_data->
			    data.expand_vol_data.vol_attr1))) {
				sam_syslog(LOG_WARNING, "add to catalog"
				    " failed(%d): %s is not a 3590",
				    library->un->eq, vsn);
				continue;
			}

			if ((ce = CatalogGetCeByBarCode(library->un->eq,
			    sam_mediatoa(library->un->media), vsn, &ced))
			    == NULL) {

				sprintf(mess, "Found %s in category %#x,"
				    " not in catalog.", vsn,
				    library->sam_category);
				sam_syslog(LOG_INFO,
				    "rec_cat(%d): Found %s in category %#x,"
				    " not in catalog",
				    LIBEQ, vsn, library->sam_category);

				status =
				    (CES_inuse | CES_occupied | CES_bar_code);
				vid.ViEq = library->un->eq;
				vid.ViSlot = ROBOT_NO_SLOT;
				memmove(vid.ViMtype,
				    sam_mediatoa(library->un->media),
				    sizeof (vid.ViMtype));
				(void) CatalogSlotInit(&vid, status,
				    (library->status.b.two_sided) ?
				    2 : 0, vsn, "");

				ce = CatalogGetCeByBarCode(library->un->eq,
				    vid.ViMtype, vsn, &ced);
				if (ce == NULL) {
					sam_syslog(LOG_INFO,
					    "rec_cat(%d): Catalog full,"
					    " exporting %s.", LIBEQ, vsn);
					set_media_category(library, vsn,
					    library->sam_category,
					    EJECT_CATEGORY);
				} else
					set_media_category(library, vsn, 0,
					    library->sam_category);
			} else {
				uchar_t vol_status = inv_entry->attr[0];

				if (DBG_LVL(SAM_DBG_DEBUG))
					sam_syslog(LOG_DEBUG,
					    "rec_cat(%d): Found %s slot"
					    " %d-status %#x.", LIBEQ,
					    ce->CeVsn, ce->CeSlot, vol_status);

				if (vol_status & MT_VSLI)
					sam_syslog(LOG_INFO,
					    "rec_cat(%d): %s, Present but"
					    " inaccessible.",
					    LIBEQ, ce->CeVsn);
				if (vol_status & MT_VSMQ)
					sam_syslog(LOG_INFO, "rec_cat(%d): %s,"
					    " Mounting or queued.",
					    LIBEQ, ce->CeVsn);
				if (vol_status & MT_VSEP)
					sam_syslog(LOG_INFO, "rec_cat(%d): %s,"
					    " Ejecting.", LIBEQ, ce->CeVsn);
				if (vol_status & MT_VSM)
					sam_syslog(LOG_INFO, "rec_cat(%d): %s,"
					    " Misplaced.", LIBEQ, ce->CeVsn);
				if (vol_status & MT_VSUU)
					sam_syslog(LOG_INFO,
					    "rec_cat(%d): %s, Unreadable label",
					    LIBEQ, ce->CeVsn);
				if (vol_status & MT_VSMM)
					sam_syslog(LOG_INFO,
					    "rec_cat(%d): %s, Used during"
					    " manual mode.", LIBEQ, ce->CeVsn);
				if (vol_status & MT_VSME) {
					sam_syslog(LOG_INFO, "rec_cat(%d): %s,"
					    " Manually Ejected.",
					    LIBEQ, ce->CeVsn);
					set_media_category(library,
					    ce->CeVsn, MAN_EJECTED_CAT,
					    PURGE_VOL_CATEGORY);
				}

			}

			/*
			 * If there is a catalog entry,
			 * and the entry's capacity is zero, and
			 * not set to zero by the user,
			 * set the capacity to default.
			 */
			if (ce &&
			    !(ce->CeStatus & CES_capacity_set) &&
			    !(ce->CeCapacity)) {
				(void) CatalogSetFieldByLoc(library->un->eq,
				    ce->CeSlot, 0, CEF_Capacity,
				    DEFLT_CAPC(library->un->media),
				    0);
				(void) CatalogSetFieldByLoc(library->un->eq,
				    ce->CeSlot, 0, CEF_Space,
				    DEFLT_CAPC(library->un->media),
				    0);
			}

		}
	}

	/*
	 * Go through the catalog and find any media not
	 * already reconciled (occupied bit is clear).
	 */
	list = CatalogGetEntriesByLibrary(library->eq, &n_entries);
	for (i = 0; i < n_entries; i++) {
		ce = &list[i];

		if ((ce->CeStatus & CES_inuse) &&
		    (!(ce->CeStatus & CES_occupied))) {
			info = NULL;
			err = view_media(library, ce->CeBarCode, (void *)&info);
			if (err != MC_REQ_OK || info == NULL) {
				sam_syslog(LOG_INFO, "rec_cat(%d): %s - %d"
				    " unable to view media.",
				    LIBEQ, ce->CeVsn, i);
			} else {
				ushort_t vol_status, category;

				memcpy(&vol_status, &info->
				    data.expand_vol_data.volume_status[0],
				    sizeof (vol_status));
				memcpy(&category, &info->
				    data.expand_vol_data.cat_assigned[0],
				    sizeof (category));
				if (vol_status & MT_VLI)
					sam_syslog(LOG_INFO,
					    "rec_cat(%d): %s, Present but"
					    " inaccessible.", LIBEQ, ce->CeVsn);
				if (vol_status &
				    (MT_VM | MT_VQM | MT_VPM | MT_VQD | MT_VPD))
					sam_syslog(LOG_INFO, "rec_cat(%d): %s,"
					    " Mounting/Dismounting  or queued.",
					    LIBEQ, ce->CeVsn);
				if (vol_status & (MT_VQE | MT_VPE))
					sam_syslog(LOG_INFO, "rec_cat(%d): %s,"
					" Ejecting.", LIBEQ, ce->CeVsn);
				if (vol_status & MT_VMIS)
					sam_syslog(LOG_INFO, "rec_cat(%d): %s,"
					    " Misplaced.", LIBEQ, ce->CeVsn);
				if (vol_status & MT_VUU)
					sam_syslog(LOG_INFO,
					    "rec_cat(%d): %s, Unreadable label",
					    LIBEQ, ce->CeVsn);
				if (vol_status & MT_VMM)
					sam_syslog(LOG_INFO,
					    "rec_cat(%d): %s, Used during"
					    " manual mode.", LIBEQ, ce->CeVsn);
				if (vol_status & MT_VME)
					sam_syslog(LOG_INFO, "rec_cat(%d): %s,"
					    " Manually Ejected.",
					    LIBEQ, ce->CeVsn);
				if (category == MAN_EJECTED_CAT)
					set_media_category(library,
					    ce->CeVsn, MAN_EJECTED_CAT,
					    PURGE_VOL_CATEGORY);
				else if (category != library->sam_category) {
					sam_syslog(LOG_INFO,
					    "rec_cat(%d): %s, slot %d: wrong"
					    " category %#x: Fixing", LIBEQ,
					    ce->CeVsn, i, category);
					set_media_category(library,
					    ce->CeVsn, category,
					    library->sam_category);
				}
				if (vol_status & (MT_VLI | MT_VQE | MT_VPE |
				    MT_VMIS | MT_VUU | MT_VME)) {
					sam_syslog(LOG_INFO, "rec_cat(%d): %s,"
					    " slot %d: removed from catalog",
					    LIBEQ, ce->CeVsn, i);
					memset(&vid, 0, sizeof (struct VolId));
					CatalogExport(CatalogVolIdFromCe(ce,
					    &vid));
					continue;
				}
				/*
				 * Found media not reconciled with the catalog.
				 * Set occupied status, rest of the
				 * catalog entry for this  media should be okay.
				 */
				status = CES_occupied;
				(void) CatalogSetFieldByLoc(library->un->eq,
				    ce->CeSlot, 0, CEF_Status, status, 0);
			}
		}
	}
	free(list);

	mutex_lock(&library->un->mutex);
	*mess = '\0';
	library->un->status.b.scanning = FALSE;
	mutex_unlock(&library->un->mutex);
	thr_exit(NULL);
}


/*
 * wait_library_ready - return when library state is ok.
 * Does not use the transport thread.
 */
void
wait_library_ready(
	library_t *library)
{
	char 	*stuff;
	char 	*mess = library->un->dis_mes[DIS_MES_NORM];
	char 	*c_mess = library->un->dis_mes[DIS_MES_CRIT];
	ushort_t bad_state = BAD_STATE, old_state = 0;
	int 	old_errno = 0x0fffffff, need_recon = FALSE;
	IBM_query_t query_req;
	IBM_query_info_t *info = &query_req.mtlqret.info;

	while (TRUE) {
		memset(&query_req, 0, sizeof (IBM_query_t));
		query_req.sub_cmd = MT_QLD;
		if (ioctl_ibmatl(library->open_fd, MTIOCLQ, &query_req) == -1) {
			ushort_t cc = query_req.mtlqret.cc;

			old_state = 0;
			if (old_errno != errno) {
				old_errno = errno;

				if (old_errno == EAGAIN) {
					stuff = "Library is unavailable.";
					memccpy(c_mess, stuff, '\0',
					    DIS_MES_LEN);
					sam_syslog(LOG_INFO,
					    "wait_library_ready(%d): %s",
					    LIBEQ, stuff);
				} else {
					if (old_errno)
						sam_syslog(LOG_INFO,
						    "wait_library_ready(%d):"
						    " (MTIOCLQ): %m", LIBEQ);
					if (errno != ENOMEM ||
					    errno != EFAULT && cc != 0) {
						sprintf(c_mess,
						    "Library reports %s error.",
						    (cc > HIGH_CC) ?
						    "Undefined" : cc_codes[cc],
						    cc);
						sam_syslog(LOG_INFO,
						    "wait_library_ready(%d):"
						    " (MTIOCLM): %s(%#x)",
						    LIBEQ, (cc > HIGH_CC) ?
						    "Undefined" : cc_codes[cc],
						    cc);
					}
				}
				sam_syslog(LOG_INFO,
				    "wait_library_ready(%d):Library not ready.",
				    LIBEQ);
			}
		} else {
			int count;
			ushort_t state, cc = query_req.mtlqret.cc;

			memcpy(&state, info->data.library_data.op_state,
			    sizeof (state));
			memcpy(&count, &info->data.library_data.num_cells[0],
			    sizeof (int));
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "Library(%d) reports %d stortage slots.",
				    LIBEQ, count);

			library->stor_count = count;
			if (state != old_state) {

				*c_mess = '\0';
				if (cc)
					sam_syslog(LOG_INFO,
					    "wait_library_ready(%d): (MTIOCLM):"
					    " %s(%#x)",
					    LIBEQ, (cc > HIGH_CC) ?
					    "Undefined" : cc_codes[cc], cc);

				if (state & MT_LIB_AOS) {
					stuff = "Automated Operational State.";
					sam_syslog(LOG_INFO,
					    "wait_library_ready(%d): %s",
					    LIBEQ, stuff);
					memccpy(mess, stuff, '\0', DIS_MES_LEN);
				}
				if (state & MT_LIB_POS) {
					stuff = "Paused Operational State.";
					sam_syslog(LOG_INFO,
					    "wait_library_ready(%d): %s",
					    LIBEQ, stuff);
					memccpy(c_mess, stuff, '\0',
					    DIS_MES_LEN);
				}
				if (state & MT_LIB_MMOS) {
					stuff =
					    "Manual Mode Operational State.";
					sam_syslog(LOG_INFO,
					    "wait_library_ready(%d): %s",
					    LIBEQ, stuff);
					memccpy(c_mess, stuff, '\0',
					    DIS_MES_LEN);
				}
				if (state & MT_LIB_DO) {
					stuff = "Degraded Operation.";
					sam_syslog(LOG_INFO,
					    "wait_library_ready(%d): %s",
					    LIBEQ, stuff);
					memccpy(mess, stuff, '\0', DIS_MES_LEN);
				}
				if (state & MT_LIB_SEIO) {
					stuff =
					    "Safety Enclose Interlock Open.";
					sam_syslog(LOG_INFO,
					    "wait_library_ready(%d): %s",
					    LIBEQ, stuff);
					memccpy(c_mess, stuff, '\0',
					    DIS_MES_LEN);
					need_recon = TRUE;
				}
				if (state & MT_LIB_VSNO) {
					stuff =
					    "Vision system Non-Operational.";
					sam_syslog(LOG_INFO,
					    "wait_library_ready(%d): %s",
					    LIBEQ, stuff);
					memccpy(mess, stuff, '\0', DIS_MES_LEN);
				}
				if (state & MT_LIB_OL) {
					stuff = "Offline.";
					sam_syslog(LOG_INFO,
					    "wait_library_ready(%d): %s",
					    LIBEQ, stuff);
					memccpy(c_mess, stuff, '\0',
					    DIS_MES_LEN);
				}
				if (state & MT_LIB_IR) {
					stuff = "Intervention Required.";
					sam_syslog(LOG_INFO,
					    "wait_library_ready(%d): %s",
					    LIBEQ, stuff);
					memccpy(c_mess, stuff, '\0',
					    DIS_MES_LEN);
				}
				if (state & MT_LIB_CK1) {
					stuff = "Library Manager Check 1"
					    " Condition.";
					sam_syslog(LOG_INFO,
					    "wait_library_ready(%d): %s",
					    LIBEQ, stuff);
					memccpy(mess, stuff, '\0', DIS_MES_LEN);
				}
				if (state & MT_LIB_SCF) {
					stuff = "All Storage Cells Full.";
					sam_syslog(LOG_INFO,
					    "wait_library_ready(%d): %s",
					    LIBEQ, stuff);
					memccpy(mess, stuff, '\0', DIS_MES_LEN);
				}
				if (state & MT_LIB_OCV) {
					stuff = "Out of Cleaner Volumes.";
					sam_syslog(LOG_INFO,
					    "wait_library_ready(%d): %s",
					    LIBEQ, stuff);
					memccpy(c_mess, stuff, '\0',
					    DIS_MES_LEN);
				}
				if (state & MT_LIB_DWD) {
					stuff = "Dual Write Disabled.";
					sam_syslog(LOG_INFO,
					    "wait_library_ready(%d): %s",
					    LIBEQ, stuff);
					memccpy(mess, stuff, '\0', DIS_MES_LEN);
				}
				if (state & MT_LIB_EA) {
					stuff = "Environmental Alert --"
					    " smoke detected.";
					sam_syslog(LOG_INFO,
					    "wait_library_ready(%d): %s",
					    LIBEQ, stuff);
					memccpy(c_mess, stuff, '\0',
					    DIS_MES_LEN);
				}
				if (state & MT_LIB_MMMOS) {
					stuff = "Managed Manual Mode"
					    " Operational State.";
					sam_syslog(LOG_INFO,
					    "wait_library_ready(%d): %s",
					    LIBEQ, stuff);
					memccpy(c_mess, stuff, '\0',
					    DIS_MES_LEN);
				}
			}
			if (!(state & bad_state)) {
				if (need_recon) {
					mutex_lock(&library->un->mutex);
					library->un->status.b.scanning = TRUE;
					mutex_unlock(&library->un->mutex);

					if (thr_create(NULL, DF_THR_STK,
					    reconcile_catalog, (void *)library,
					    (THR_BOUND | THR_DETACHED), NULL)) {
						mutex_lock(&library->un->mutex);
						library->un->status.b.scanning
						    = FALSE;
						mutex_unlock(
						    &library->un->mutex);
					}
				}
				*c_mess = '\0';
				return;
			}
			old_state = state;
			if (old_state == 0)
				sam_syslog(LOG_INFO, "wait_library_ready(%d):"
				" Library not ready.", LIBEQ);
		}
		sleep(10);
	}
}
