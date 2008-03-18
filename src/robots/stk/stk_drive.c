/*
 *	stk_drive.c - stk_drive unique routines.
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

#pragma ident "$Revision: 1.39 $"

static char *_SrcFile = __FILE__;

#include <synch.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/dev_log.h"
#include "sam/lib.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/odlabels.h"
#include "aml/tplabels.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "sam/defaults.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"
#include "stk.h"
#include "aml/trace.h"
#include "../common/drive.h"
#include "driver/samst_def.h"


/*	some globals */
extern shm_alloc_t master_shm, preview_shm;

extern void LogApiStatus(char *entry_ptr, int error);

/*
 *	audit - start auditing
 */
void
audit(
	drive_state_t *drive,		/* drive state pointer */
	const uint_t slot,		/* slot to audit */
	const int audit_eod)
{
	int 		err;
	char 		*ent_pnt = "audit";
	dev_ent_t 	*un = drive->un;
	robo_event_t 	robo_event;
	sam_defaults_t 	*defaults;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;

	defaults = GetDefaults();

	if (slot == ROBOT_NO_SLOT) {
		sam_syslog(LOG_ERR, catgets(catfd, SET, 9158,
		    "Audit not supported on stk."));
		return;
	}

	/* Set up dummy event for get_media, force no_requeue */
	memset(&robo_event, 0, sizeof (robo_event_t));
	robo_event.status.b.dont_reque = TRUE;

	ce = CatalogGetCeByLoc(drive->library->un->eq, slot, 0, &ced);
	if (ce == NULL) {
		sam_syslog(LOG_INFO, catgets(catfd, SET, 9090,
		    "%s: Slot (%d) is out of range."), ent_pnt, slot);
		return;
	}
	mutex_lock(&drive->mutex);
	if (drive->status.b.full) {
		un->status.b.requested = TRUE;
		if (clear_drive(drive)) {
			mutex_unlock(&drive->mutex);
			return;
		}
		if (drive->open_fd >= 0) {
			mutex_lock(&un->mutex);
			close_unit(un, &drive->open_fd);
			mutex_unlock(&un->mutex);
		}
	}
	mutex_unlock(&drive->mutex);
	mutex_lock(&un->mutex);
	un->status.b.requested = TRUE;
	un->status.b.labeled = FALSE;
	un->status.b.ready = FALSE;
	mutex_unlock(&un->mutex);

	mutex_lock(&drive->mutex);

	/* Can not audit empty slot or not_sam media */
	if ((ce->CeStatus & CES_occupied) && !(ce->CeStatus & CES_non_sam)) {
		if ((err = get_media(drive->library, drive, &robo_event, ce))
		    != RET_GET_MEDIA_SUCCESS) {

			mutex_lock(&un->mutex);
			un->status.b.requested = FALSE;
			if (IS_GET_MEDIA_FATAL_ERROR(err)) {
				if (err == RET_GET_MEDIA_DOWN_DRIVE) {
					un->state = DEV_DOWN;
					sam_syslog(LOG_INFO,
					    catgets(catfd, SET, 9159,
					    "%s:Setting device (%d) down"
					    " due to ACL error."),
					    ent_pnt, un->eq);
					err = -err;
				}
			}

			DEC_ACTIVE(un);
			mutex_unlock(&un->mutex);	/* if error */
			mutex_unlock(&drive->mutex);
			return;
		}

		mutex_lock(&un->mutex);
		un->status.b.scanning = TRUE;
		mutex_unlock(&un->mutex);
		while (spin_drive(drive, SPINUP, NOEJECT)) {
			if (un->state > DEV_ON) {
				mutex_lock(&un->mutex);
				un->status.b.scanning = FALSE;
				mutex_unlock(&un->mutex);
				return;
			}
			sleep(2);
		}

		un->i.ViPart = ce->CePart;
		scan_a_device(un, drive->open_fd);

		mutex_lock(&un->mutex);

		if (!un->status.b.labeled &&
		    (ce->CeStatus & CES_bar_code) &&
		    (defaults->flags & DF_LABEL_BARCODE) &&
		    IS_TAPE(un)) {

			vsn_from_barcode(un->vsn, ce->CeBarCode, defaults, 6);
			un->status.b.labeled = TRUE;
			un->space = un->capacity;
		}

		if (audit_eod && un->status.b.labeled) {
			mutex_unlock(&un->mutex);
			mutex_lock(&un->io_mutex);
			tape_append(drive->open_fd, un, NULL);
			mutex_unlock(&un->io_mutex);
			mutex_lock(&un->mutex);
		}

		UpdateCatalog(un, 0, CatalogVolumeLoaded);
		close_unit(un, &drive->open_fd);
		DEC_ACTIVE(un);
		un->status.b.requested = TRUE;
		mutex_unlock(&un->mutex);
	}

	mutex_unlock(&drive->mutex);
	mutex_lock(&un->mutex);
	un->status.b.requested = FALSE;
	mutex_unlock(&un->mutex);
	mutex_lock(&drive->library->un->mutex);
	drive->library->un->status.b.mounted = TRUE;
	mutex_unlock(&drive->library->un->mutex);
}


/*
 *	query_drive - get information for a drive.
 * exit -
 *	 status of the query.  If > STATUS_LAST, then helper returned error.
 */
int
query_drive(
	library_t *library,	/* library */
	drive_state_t *drive,	/* drive */
	int *drive_status,	/* status from reply */
	int *drive_state)
{
	int 			err;
	stk_information_t 	*stk_info;
	robo_event_t 		*query, *tmp;
	xport_state_t 		*transport;

	stk_info = malloc_wait(sizeof (stk_information_t), 2, 0);
	memset(stk_info, 0, sizeof (stk_information_t));
	stk_info->drive_id = drive->drive_id;

	/* Build transport thread request */
	query = get_free_event(library);
	(void) memset(query, 0, sizeof (robo_event_t));
	query->request.internal.command = ROBOT_INTRL_QUERY_DRIVE;
	query->request.internal.address = (void *)stk_info;

	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG, "stk_query_drive: %d,%d,%d,%d.",
		    DRIVE_LOC(drive->drive_id));

	query->type = EVENT_TYPE_INTERNAL;
	query->status.bits = REST_SIGNAL;
	query->completion = REQUEST_NOT_COMPLETE;

	transport = library->transports;

	mutex_lock(&query->mutex);
	mutex_lock(&transport->list_mutex);

	if (transport->active_count == 0)
		transport->first = query;
	else {
		LISTEND(transport, tmp);
		append_list(tmp, query);
	}

	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);

	/* Wait for the transport to do the unload */
	while (query->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&query->condit, &query->mutex);
	mutex_unlock(&query->mutex);

	*drive_state = STATE_FIRST;
	*drive_status = STATUS_PROCESS_FAILURE;
	err = query->completion;
	if (err == STATUS_SUCCESS) {
		stk_resp_acs_t *acs_resp =
		    (stk_resp_acs_t *)&
		    query->request.message.param.start_of_request;
		SAM_ACS_QUERY_DRV_RESPONSE *qr = &acs_resp->data.q_drive;

		/* Get acs_response status */
		err = acs_resp->hdr.acs_status;
		if (err == STATUS_SUCCESS) {
			/* Get status of the query command */
			err = qr->query_drv_status;
			if (err == STATUS_SUCCESS) {
				char *state_msg;

				/* dig out the status of the drive itself */
				*drive_status = qr->drv_status[0].status;
				*drive_state = qr->drv_status[0].state;
				if (DBG_LVL(SAM_DBG_DEBUG)) {
					state_msg = (*drive_state ==
					    STATE_ONLINE ? "ONLINE" :
					    *drive_state == STATE_OFFLINE ?
					    "OFFLINE" : "DIAG");
					sam_syslog(LOG_DEBUG,
					    "query drive (%d,%d,%d,%d)"
					    " returned:%s:%s",
					    DRIVE_LOC(drive->drive_id),
					    sam_acs_status(err), state_msg);
				}
				if (strlen((void *)&qr->drv_status[0].
				    vol_id.external_label[0]) != 0) {
					if (DBG_LVL(SAM_DBG_DEBUG))
						sam_syslog(LOG_DEBUG,
						"query drive (%d,%d,%d,%d) has"
						" volume %s", DRIVE_LOC(
						    drive->drive_id), qr->
						    drv_status[0].vol_id.
						    external_label);

					memcpy(drive->un->vsn,
					    (void *)&qr->drv_status[0].
					    vol_id.external_label[0], 7);
					memcpy(drive->bar_code,
					    drive->un->vsn, 7);
					memmove(drive->un->i.ViVsn,
					    drive->un->vsn,
					    sizeof (drive->un->i.ViVsn));
					memmove(drive->un->i.ViMtype,
					    sam_mediatoa(drive->un->
					    equ_type),
					    sizeof (drive->un->i.ViMtype));
					drive->un->i.ViFlags = VI_logical;
				} else if (DBG_LVL(SAM_DBG_DEBUG))
					sam_syslog(LOG_DEBUG,
					    "query drive (%d,%d,%d,%d) has"
					    " no volume", DRIVE_LOC(
					    drive->drive_id));
			} else
				sam_syslog(LOG_ERR,
				    "query drive status:%s",
				    sam_acs_status(err));
		} else
			sam_syslog(LOG_ERR,
			    "query drive response status:%s",
			    sam_acs_status(err));
	} else {
		if (err > STATUS_LAST)
			sam_syslog(LOG_INFO,
			    "helper-query_drive status(errno):%s.",
			    strerror(err - STATUS_LAST));
		else
			sam_syslog(LOG_INFO, "helper-query_drive status:%s",
			    sam_acs_status(err));
	}

	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "Return from query_drive (%#x).", err);

	mutex_destroy(&query->mutex);
	cond_destroy(&query->condit);
	free(query);
	free(stk_info);
	return (err);
}


drive_state_t *
find_empty_drive(
	drive_state_t *drive)
{
	library_t	*library;
	drive_state_t	*empty_drive;

	empty_drive = NULL;
	library = drive->library;
	if (library->status.b.passthru) {
		empty_drive = query_mnt_status(drive);
		return (empty_drive);
	} else {
		empty_drive = query_all_drives(drive);
		return (empty_drive);
	}
}


/*
 * query_all_drives - gets information for MAX_ID drives, returns empty drive.
 */
drive_state_t *
query_all_drives(
	drive_state_t *drive)
{
	int			i, err;
	drive_state_t		*local_drive, *save_drive, *start_drive;
	stk_information_t 	*stk_info;
	robo_event_t		*query_all, *tmp;
	xport_state_t		*transport;
	library_t		*library;

	library = drive->library;
	local_drive = start_drive = drive;
	save_drive = NULL;
	stk_info = malloc_wait(sizeof (stk_information_t), 2, 0);
	memset(stk_info, 0, sizeof (stk_information_t));

	/* Build transport thread request */
	query_all = get_free_event(library);
	(void) memset(query_all, 0, sizeof (robo_event_t));
	query_all->request.internal.command = ROBOT_INTRL_QUERY_ALL_DRIVES;
	query_all->request.internal.address = (void *)stk_info;

	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG,
		    "stk_query_all_drives(%d)", library->un->eq);

	query_all->type = EVENT_TYPE_INTERNAL;
	query_all->status.bits = REST_SIGNAL;
	query_all->completion = REQUEST_NOT_COMPLETE;

	transport = library->transports;

	mutex_lock(&query_all->mutex);
	mutex_lock(&transport->list_mutex);

	if (transport->active_count == 0) {
		transport->first = query_all;
	} else {
		LISTEND(transport, tmp);
		append_list(tmp, query_all);
	}

	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);

	/* Wait for the transport to do the unload */
	while (query_all->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&query_all->condit, &query_all->mutex);
	mutex_unlock(&query_all->mutex);

	err = query_all->completion;

	if (err != STATUS_SUCCESS) {
		if (err > STATUS_LAST) {
			sam_syslog(LOG_INFO,
			    "helper-query_drive status(errno):%s.",
			    strerror(err - STATUS_LAST));
		} else {
			sam_syslog(LOG_INFO, "helper-query_drive status:%s",
			    sam_acs_status(err));
		}
		goto out;

	} else {

		stk_resp_acs_t	*acs_resp =
		    (stk_resp_acs_t *)
		    &query_all->request.message.param.start_of_request;
		SAM_ACS_QUERY_DRV_RESPONSE *qr = &acs_resp->data.q_drive;

		/* Get acs_response status */
		err = acs_resp->hdr.acs_status;

		if (err != STATUS_SUCCESS) {
			sam_syslog(LOG_ERR, "query drive response status:%s",
			    sam_acs_status(err));
			goto out;
		}

		/* Get status of the query command */
		err = qr->query_drv_status;
		if (err == STATUS_SUCCESS) {
		/*
		 * N.B. Note the inaccurate code indentation below.
		 */

			/*
			 * Start at the next drive to use and search
			 * for an empty drive returned from the query all.
			 */
		for (;;) {

		for (i = 0; i < qr->count; i++) {

			if ((strlen((void *)&qr->drv_status[i].vol_id) == 0) &&
			    (qr->drv_status[i].status
			    == STATUS_DRIVE_AVAILABLE) &&
			    DRIVE_ID_MATCH(local_drive->drive_id,
			    qr->drv_status[i].drive_id)) {

				/*
				 * It's empty, see if already requested.
				 */
				if (!mutex_trylock(&local_drive->mutex)) {

					/*
					 * N.B. Inaccurate indentation follows
					 */

				if (!local_drive->status.b.full &&
				    !local_drive->active_count &&
				    mutex_trylock(&local_drive->
				    un->mutex)) {

					if (!(local_drive->un->status.bits &
					    (DVST_REQUESTED | DVST_CLEANING)) &&
					    !local_drive->un->
					    status.b.shared_reqd &&
					    !local_drive->un->
					    status.b.requested &&
					    (local_drive->un->state
					    < DEV_IDLE)) {

						local_drive->un->
						    status.b.shared_reqd =
						    TRUE;
						save_drive = local_drive;
						goto out;
					}

					mutex_unlock(&local_drive->un->mutex);
					}

					mutex_unlock(&local_drive->mutex);
					}
				}
			}

			if ((local_drive = local_drive->next) == NULL) {
				local_drive = library->drive;
			}

			/* Have we looked at all drives? */
			if (local_drive == start_drive) {
				break;
			}

			}

		} else {
			sam_syslog(LOG_ERR, "query drive all status:%s",
			    sam_acs_status(err));
		}
	}

out:

	if (save_drive != NULL) {

		mutex_lock(&library->mutex);
		library->index = save_drive;
		mutex_unlock(&library->mutex);

		sam_syslog(LOG_DEBUG,
		    "query_all_drives found empty drive(%d,%d,%d,%d)",
		    DRIVE_LOC(save_drive->drive_id));

	} else {
		sam_syslog(LOG_DEBUG,
		    "query_all_drives found no empty drive");
	}

	mutex_destroy(&query_all->mutex);
	cond_destroy(&query_all->condit);
	free(query_all);
	free(stk_info);
	return (save_drive);
}


/*
 * query_mount_status - returns a list of drives ordered by proximity to VOLID
 */
drive_state_t *
query_mnt_status(
	drive_state_t *drive)
{
	int			i, err;
	char			*ent_pnt = "query_mnt_status";
	drive_state_t		*tmp_drive;
	robo_event_t		*query_mnt, *tmp;
	xport_state_t		*transport;
	library_t		*library;
	VOLID			*vol_id;

	library = drive->library;
	if (*library->un->i.ViVsn == '\0') {
		sam_syslog(LOG_ERR,
		    "No volume available for query_mnt_status.");
		return (NULL);
	}

	tmp_drive = library->drive;

	vol_id = malloc_wait(sizeof (VOLID), 2, 0);
	strncpy((void *)vol_id, library->un->i.ViVsn, sizeof (VOLID));

	/* Build transport thread request */
	query_mnt = get_free_event(library);
	(void) memset(query_mnt, 0, sizeof (robo_event_t));
	query_mnt->request.internal.command = ROBOT_INTRL_QUERY_MNT_STATUS;
	query_mnt->request.internal.address = (void *)vol_id;

	if (DBG_LVL(SAM_DBG_TMOVE)) {
		sam_syslog(LOG_DEBUG, "%s: %s.", ent_pnt, vol_id);
	}

	query_mnt->type = EVENT_TYPE_INTERNAL;
	query_mnt->status.bits = REST_SIGNAL;
	query_mnt->completion = REQUEST_NOT_COMPLETE;

	transport = library->transports;

	mutex_lock(&query_mnt->mutex);
	mutex_lock(&transport->list_mutex);

	if (transport->active_count == 0) {
		transport->first = query_mnt;
	} else {
		LISTEND(transport, tmp);
		append_list(tmp, query_mnt);
	}

	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);

	/* Wait for the transport to do the unload */
	while (query_mnt->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&query_mnt->condit, &query_mnt->mutex);
	mutex_unlock(&query_mnt->mutex);

	err = query_mnt->completion;

	if (err == STATUS_SUCCESS) {
		stk_resp_acs_t *acs_resp =
		    (stk_resp_acs_t *)
		    &query_mnt->request.message.param.start_of_request;
		SAM_ACS_QUERY_MNT_RESPONSE *qr = &acs_resp->data.q_mount;

		/* Get acs_response status */
		err = acs_resp->hdr.acs_status;
		/*
		 * N.B. Inaccurate code indentation follows.
		 * Done in order to meet cstyle requirements and
		 * not make any logic changes.
		 */
		if (err == STATUS_SUCCESS) {

			/* Get status of the query command */
		err = qr->query_mnt_stat;

		if (err == STATUS_SUCCESS) {
				/*
				 * for loop searches through a list of
				 * drives ordered by proximity to volume.
				 * Find the fist empty drive and exit loop.
				 */
		for (i = 0; i < qr->mnt_status[0].drive_count; i++) {

			if (qr->mnt_status[0].drive_status[i].status ==
			    STATUS_DRIVE_AVAILABLE) {
				tmp_drive = library->drive;
				while (tmp_drive != NULL) {

				if (DRIVE_ID_MATCH(tmp_drive->drive_id,
				    qr->mnt_status[0].
				    drive_status[i].drive_id)) {
					if (!mutex_trylock(&tmp_drive->mutex)) {
						if (!tmp_drive->status.b.full &&
						    !tmp_drive->active_count &&
						    !mutex_trylock(
						    &tmp_drive->un->mutex)) {

						if (!(tmp_drive->un->
						    status.bits &
						    (DVST_REQUESTED |
						    DVST_CLEANING)) &&
						    !tmp_drive->
						    un->status.b.shared_reqd &&
						    !tmp_drive->un->
						    status.b.requested &&
						    (tmp_drive->un->state <
						    DEV_IDLE)) {
							tmp_drive->un->
							    status.b.shared_reqd
							    = TRUE;
							goto out;
						}
						mutex_unlock(
						    &tmp_drive->un->mutex);
						}
						mutex_unlock(&tmp_drive->mutex);
					}
				}
				tmp_drive = tmp_drive->next;

				} /* end of while tmp_drive != NULL */
			}
		} /* end of for loop */

		} else {
			sam_syslog(LOG_ERR, "query mount status:%s",
			    sam_acs_status(err));
		}

		} else {
			sam_syslog(LOG_ERR, "query mount response status:%s",
			    sam_acs_status(err));
		}
	} else {
		if (err > STATUS_LAST) {
			sam_syslog(LOG_INFO,
			    "helper-query_mount status(errno):%s.",
			    strerror(err - STATUS_LAST));
		} else {
			sam_syslog(LOG_INFO, "helper-query_mount status:%s",
			    sam_acs_status(err));
		}
	}

out:
	if (tmp_drive != NULL) {
		sam_syslog(LOG_DEBUG,
		    "query_mount_status found empty drive(%d,%d,%d,%d)",
		    DRIVE_LOC(tmp_drive->drive_id));
	} else {
		sam_syslog(LOG_DEBUG,
		    "query_mnt_status found no empty drive");
	}
	mutex_destroy(&query_mnt->mutex);
	cond_destroy(&query_mnt->condit);
	free(query_mnt);
	free(vol_id);
	return (tmp_drive);
}


/*
 *	search_drives - search drives looking for a drive with slot
 * exit -
 *	 drive_state_t * to drive with the slot (mutex locked)
 *	 drive_state_t *NULL if not found.
 *
 */
drive_state_t *
search_drives(
	library_t *library,
	uint_t slot)
{
	drive_state_t 	*drive;

	for (drive = library->drive; drive != NULL; drive = drive->next) {
		if (drive->un == NULL)
			continue;

		if (drive->status.b.full && (drive->un->slot == slot)) {
			mutex_lock(&drive->mutex);
			mutex_lock(&drive->un->mutex);

			if (drive->status.b.full && (drive->un->slot == slot))
				return (drive);

			mutex_unlock(&drive->un->mutex);
			mutex_unlock(&drive->mutex);
		}
	}
	return (NULL);
}

/*
 *	robot_autoclean - robot cleans the drive.
 */
boolean_t
library_autoclean(
	void)
{
	char *ptr;


	return (B_FALSE);
}

/*
 *	clean - clean the drive.
 */
void
clean(
	drive_state_t *drive,
	robo_event_t *event)
{
	int 		err, retry, stk_error;
	dev_ent_t 	*un = drive->un;
	library_t 	*library = drive->library;
	struct CatalogEntry 	ced;
	struct CatalogEntry 	*ce = &ced;
	uint32_t	access_count, status = 0;
	char		*d_mess;
	char 		*l_mess;


	d_mess = drive->un->dis_mes[DIS_MES_NORM];
	l_mess = library->un->dis_mes[DIS_MES_NORM];

	if (library_autoclean() == B_TRUE) {
		DevLog(DL_ALL(8003));
		mutex_lock(&drive->un->mutex);
		drive->un->status.bits &= ~(DVST_REQUESTED | DVST_CLEANING);
		mutex_unlock(&drive->un->mutex);
		disp_of_event(library, event, 0);
		return;
	}

	mutex_lock(&drive->mutex);

	if (clear_drive(drive)) {
		mutex_lock(&drive->un->mutex);
		drive->un->status.bits |= DVST_CLEANING;
		mutex_unlock(&drive->un->mutex);
		mutex_unlock(&drive->mutex);
		disp_of_event(library, event, ENOENT);
		return;
	}

	mutex_lock(&drive->un->mutex);
	drive->un->status.bits |= (DVST_REQUESTED | DVST_CLEANING);

	if (drive->un->open_count) {
		clear_driver_idle(drive, drive->open_fd);
		close_unit(drive->un, &drive->open_fd);
		DEC_OPEN(drive->un);
	}

	mutex_unlock(&drive->un->mutex);
	mutex_unlock(&drive->mutex);

	memccpy(d_mess, catgets(catfd, SET, 9025, "needs cleaning"),
	    '\0', DIS_MES_LEN);

	ce = CatalogGetCleaningVolume(library->un->eq, &ced);

	if (ce == NULL) {
		memccpy(l_mess,
		    catgets(catfd, SET, 9026,
		    "no cleaning cartridge available"),
		    '\0', DIS_MES_LEN);

		DevLog(DL_ERR(5141));
		SendCustMsg(HERE, 9347);
		mutex_lock(&drive->mutex);
		down_drive(drive, SAM_STATE_CHANGE);
		mutex_lock(&drive->un->mutex);
		drive->un->status.bits &= ~DVST_REQUESTED;
		mutex_unlock(&drive->un->mutex);
		disp_of_event(library, event, EAGAIN);
		mutex_unlock(&drive->mutex);
		return;
	}

	DevLog(DL_ALL(0), "Cleaning cart barcode %s", ce->CeBarCode);
	status &= ~CES_occupied;
	(void) CatalogSetFieldByLoc(ce->CeEq, ce->CeSlot, ce->CePart,
	    CEF_Status, status, CES_occupied);

	mutex_lock(&drive->mutex);

	if (load_media(library, drive, ce, &stk_error)) {
		DevLog(DL_ERR(6036), ce->CeBarCode);
		memccpy(drive->un->dis_mes[DIS_MES_CRIT],
		    catgets(catfd, SET, 9029,
		    "unable to load cleaning cartridge, move failed"),
		    '\0', DIS_MES_LEN);
		down_drive(drive, SAM_STATE_CHANGE);
		mutex_unlock(&drive->mutex);
		disp_of_event(library, event, EIO);
		return;
	}

	mutex_unlock(&drive->mutex);

	/* Log successful mount of cleaning tape */
	DevLog(DL_ALL(10042), drive->un->eq);
	tapeclean_media(drive->un);

	access_count = ce->CeAccess;
	access_count--;
	(void) CatalogSetFieldByLoc(ce->CeEq, ce->CeSlot, ce->CePart,
	    CEF_Access, access_count, 0);
	(void) CatalogSetFieldByLoc(ce->CeEq, ce->CeSlot, ce->CePart,
	    CEF_MountTime, time(NULL), 0);

	retry = 7;
	do {
		memccpy(d_mess,
		    catgets(catfd, SET, 9030, "wait for cleaning cycle"),
		    '\0', DIS_MES_LEN);
		sleep(3 * 60);		/* wait 3 minutes */

		tapeclean_media(drive->un);
		mutex_lock(&drive->mutex);
		err = dismount_media(library, drive, &stk_error);
		if (err) {
			DevLog(DL_DETAIL(8002), retry-1);
		}
		mutex_unlock(&drive->mutex);

	} while (err != 0 && retry-- != 0);

	tapeclean_media(drive->un);

	if (err != 0) {
		DevLog(DL_ERR(5080));
		memccpy(drive->un->dis_mes[DIS_MES_CRIT],
		    catgets(catfd, SET, 9032,
		    "unable to unload cleaning cartridge"),
		    '\0', DIS_MES_LEN);
		mutex_lock(&drive->mutex);
		drive->status.b.cln_inprog = FALSE;
		down_drive(drive, SAM_STATE_CHANGE);
		mutex_unlock(&drive->mutex);
		disp_of_event(library, event, EIO);
		return;
	}

	status = CES_occupied;
	if (drive->un->status.b.bad_media) {
		/* cleaning media marked in catalog as bad */
		status |= CES_bad_media;

		/* reset bad media flag */
		drive->un->status.b.bad_media = 0;
	}
	(void) CatalogSetFieldByLoc(library->un->eq, ce->CeSlot, 0,
	    CEF_Status, status, 0);

	DevLog(DL_ALL(5334), access_count);

	if ((status & CES_bad_media) == 0) {
		memccpy(d_mess,
		    catgets(catfd, SET, 9034, "drive has been cleaned"),
		    '\0', DIS_MES_LEN);
	}
	mutex_lock(&drive->mutex);
	drive->status.b.cln_inprog = FALSE;
	mutex_lock(&drive->un->mutex);
	if ((status & CES_bad_media) == 0) {
		/* drive was cleaned */
		drive->un->status.bits &= ~(DVST_CLEANING | DVST_REQUESTED);
	} else {
		/* drive was not cleaned, it needs to still be cleaned */
		drive->un->status.bits &= ~(DVST_REQUESTED);
	}
	mutex_unlock(&drive->un->mutex);
	mutex_unlock(&drive->mutex);
	if (ce->CeAccess == 0 || (status & CES_bad_media)) {
		char	*MES_9035 = catgets(catfd, SET, 9035,
		    "cleaning cartridge in slot %d has expired");
		char	*mess = (char *)
		    malloc_wait(strlen(MES_9035) + 15, 2, 0);

		sprintf(mess, MES_9035, ce->CeSlot);
		memccpy(l_mess, mess, '\0', DIS_MES_LEN);
		free(mess);
		DevLog(DL_ERR(5144), ce->CeSlot);
	} else if (tapeclean_drive(drive->un)) {
		memccpy(l_mess,
		    catgets(catfd, SET, 2983, "clean failed"),
		    '\0', DIS_MES_LEN);
		DevLog(DL_ERR(5364));
		mutex_lock(&drive->mutex);
		down_drive(drive, SAM_STATE_CHANGE);
		mutex_unlock(&drive->mutex);
		disp_of_event(library, event, EIO);
		return;
	}
	disp_of_event(library, event, 0);
}

/*
 * This is ACSLS and passthru. Interrogate the ACSLS
 * database to find the resident lsm for this volume.
 * then compare the resident lsm with the lsm
 * the drive is in. Return TRUE if vsn and drive are
 * in the same lsm.
 */
boolean_t
drive_is_local(
	library_t *library,
	drive_state_t *drive)
{
	boolean_t 	drive_is_local;
	int 		lsm;
	int 		err;
	VOLID 		*vol_id;
	char 		*ent_pnt = "select_passthru_drive";
	xport_state_t 	*transport;
	robo_event_t 	*query, *tmp;

	drive_is_local = TRUE;

	/* If the volid is not filled in, quit now. */
	if (*library->un->i.ViVsn == '\0') {
		sam_syslog(LOG_ERR,
		    "No volume information available for view media.");
		return (drive_is_local);
	}

	/* Perform a query volume to discover the local library. */
	vol_id = malloc_wait(sizeof (VOLID), 2, 0);
	strncpy((void *)vol_id, library->un->i.ViVsn, sizeof (VOLID));
	query = get_free_event(library);
	(void) memset(query, 0, sizeof (robo_event_t));
	query->request.internal.command = ROBOT_INTRL_VIEW_DATABASE;
	query->request.internal.address = (void *)vol_id;

	if (DBG_LVL(SAM_DBG_TMOVE))
		sam_syslog(LOG_DEBUG, "%s: %s.", ent_pnt, vol_id);

	query->type = EVENT_TYPE_INTERNAL;
	query->status.bits = REST_SIGNAL;
	query->completion = REQUEST_NOT_COMPLETE;

	transport = library->transports;
	mutex_lock(&query->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0)
		transport->first = query;
	else {
		LISTEND(transport, tmp);
		append_list(tmp, query);
	}

	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);

	/* Now wait for the transport to do the request */
	while (query->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&query->condit, &query->mutex);

	mutex_unlock(&query->mutex);

	if ((err = query->completion) == STATUS_SUCCESS) {
		stk_resp_acs_t *acs_resp =
		    (stk_resp_acs_t *)&
		    query->request.message.param.start_of_request;
		SAM_ACS_QUERY_VOL_RESPONSE *qr = &acs_resp->data.query;

		/* Check the api_status before proceeding. */
		if (acs_resp->hdr.api_status != STATUS_SUCCESS) {
			LogApiStatus(ent_pnt, acs_resp->hdr.api_status);
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "Return from %s (%#x).", ent_pnt, err);
			goto out;
		}

		/* Now dig out the lsm id for this volume. */

		/* This is the acs_response status */
		err = acs_resp->hdr.acs_status;
		if (err == STATUS_SUCCESS) {
			err = qr->query_vol_status;
			if (err == STATUS_SUCCESS) {
				lsm = qr->vol_status[0].
				    location.cell_id.panel_id.lsm_id.lsm;
				if (lsm != drive->drive_id.panel_id.lsm_id.lsm)
					drive_is_local = FALSE;
			} else {
				sam_syslog(LOG_INFO, "%s status:%s", ent_pnt,
				    sam_acs_status(err));
			}
		} else {
			sam_syslog(LOG_INFO, "helper-%s status:%s",
			    ent_pnt, sam_acs_status(err));
		}
	} else {
		if (err > STATUS_LAST) {
			err = err - STATUS_LAST;
			sam_syslog(LOG_INFO,
			    "helper-%s status(errno):%m.", ent_pnt);
		} else {
			sam_syslog(LOG_INFO, "helper-%s status:%s", ent_pnt,
			    sam_acs_status(err));
		}
	}

out:
	mutex_destroy(&query->mutex);
	cond_destroy(&query->condit);
	free(query);
	free(vol_id);
	return (drive_is_local);
}
