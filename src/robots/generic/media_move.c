/*
 *	media_move.c - routines to issue requests to the transport
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

#pragma ident "$Revision: 1.53 $"

/* Using __FILE__ makes duplicate strings */
static char    *_SrcFile = __FILE__;

#include <stdio.h>
#include <thread.h>
#include <synch.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/syslog.h>		/* needed for ETRACE */

#include "sam/types.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/device.h"
#include "aml/dev_log.h"
#include "sam/lib.h"
#include "sam/param.h"
#include "aml/shm.h"
#include "sam/defaults.h"
#include "aml/robots.h"
#include "aml/proto.h"
#include "aml/trace.h"
#include "sam/nl_samfs.h"
#include "generic.h"

/*	function prototypes */
static int
api_load_media(library_t *library, drive_state_t *drive,
		struct CatalogEntry *ce);

static int	api_unload_media(library_t *library, drive_state_t *drive);

static int
check_and_set_occupy(library_t *library, drive_state_t *drive,
			robo_event_t *event, struct CatalogEntry *ce);

static void	exchange_media_failure(library_t *library,
    struct CatalogEntry *ce);

/* globals */
extern shm_alloc_t master_shm, preview_shm;

extern int
api_get_media(library_t *library, drive_state_t *drive,
		robo_event_t *event, struct CatalogEntry *ce);

extern int
generic_get_media(library_t *library, drive_state_t *drive,
		robo_event_t *event, struct CatalogEntry *ce);


/*
 *	exchange_media - do a three move exchange
 *
 * exit -
 *	  0 - ok
 *	  1 - not ok
 *
 */
int
exchange_media(
		library_t *library,	/* pointer to the library */
		const uint_t source,	/* source element address */
		const uint_t dest1,	/* first destination element addr */
		const uint_t dest2,	/* second destination element addr */
		const int invert1,	/* != 0 inverted first */
		const int invert2,	/* != 0 inverted second */
		const move_flags_t flags)
{
	int		err;
	dev_ent_t	*un;
	xport_state_t  *transport;
	robo_event_t   *exchange, *tmp;
	robot_internal_t *params;

	/* Build transport thread request */

	un = library->un;
	exchange = get_free_event(library);
	params = &exchange->request.internal;
	(void) memset(exchange, 0, sizeof (robo_event_t));

	params->command = ROBOT_INTRL_EXCH_MEDIA;
	params->slot = ROBOT_NO_SLOT;
	params->source = source;
	params->destination1 = dest1;
	params->destination2 = dest2;
	params->flags.b.noerror = flags.b.noerror;
	if (invert1)
		params->flags.b.invert1 = TRUE;
	if (invert2)
		params->flags.b.invert2 = TRUE;

	DevLog(DL_DETAIL(5048),
	    source, invert1 ? "invert" : "asis", dest1, dest2,
	    invert2 ? "invert" : "asis");

	exchange->type = EVENT_TYPE_INTERNAL;
	exchange->status.bits = REST_SIGNAL;
	exchange->completion = REQUEST_NOT_COMPLETE;

	transport = library->transports;
	/*
	 * Always post work to the first transport. There is only one
	 * transport thread running.
	 */
	mutex_lock(&exchange->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0)
		transport->first = exchange;
	else {
		LISTEND(transport, tmp);
		append_list(tmp, exchange);
	}
	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);

	/* Wait for the transport to do the exchange */
	while (exchange->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&exchange->condit, &exchange->mutex);
	mutex_unlock(&exchange->mutex);

	err = exchange->completion;
	mutex_destroy(&exchange->mutex);
	cond_destroy(&exchange->condit);
	free(exchange);
	return (err);
}


/*
 *	move_media - use the two stage move
 *
 * exit -
 *	 0 - ok
 *	 1 - not ok
 */
int
move_media(
	library_t *library,	/* library pointer */
	const uint_t trans_ele,	/* transport element address */
	const uint_t source,	/* source element address */
	const uint_t dest,	/* destination element address */
	const int invert,	/* != 0 invert the media */
	const move_flags_t flags)
{
	int		err;
	dev_ent_t	*un;
	xport_state_t  *transport;
	robo_event_t   *exchange, *tmp;
	robot_internal_t *params;

	un = library->un;
	exchange = get_free_event(library);
	params = &exchange->request.internal;
	(void) memset(exchange, 0, sizeof (robo_event_t));

	params->command = ROBOT_INTRL_MOVE_MEDIA;
	params->slot = ROBOT_NO_SLOT;
	params->transport = trans_ele;
	params->source = source;
	params->destination1 = dest;
	params->flags.b.noerror = flags.b.noerror;
	if (invert)
		params->flags.b.invert1 = TRUE;

	DevLog(DL_DETAIL(5049), source, invert ? "invert" : "asis", dest);

	exchange->type = EVENT_TYPE_INTERNAL;
	exchange->completion = REQUEST_NOT_COMPLETE;
	exchange->status.bits = REST_SIGNAL;

	transport = library->transports;

	mutex_lock(&exchange->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0)	/* any existing entries */
		transport->first = exchange;
	else {			/* add this to the end of the list */
		LISTEND(transport, tmp);
		(void) append_list(tmp, exchange);
	}

	transport->active_count++;
	cond_signal(&transport->list_condit);	/* wake up the transport */
	mutex_unlock(&transport->list_mutex);

	/* wait for completion */
	while (exchange->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&exchange->condit, &exchange->mutex);
	mutex_unlock(&exchange->mutex);
	err = exchange->completion;
	mutex_destroy(&exchange->mutex);
	cond_destroy(&exchange->condit);
	free(exchange);
	return (err);
}


/*
 *	flip_and_scan - flip the media in the drive and scan it
 *	 entry -
 *			 mutex on drive_state_t is held
 *			 orig_part is the partition for the side currently
 *			 loaded in the drive.
 */
int
flip_and_scan(
		int orig_part,
		drive_state_t *drive)
{
	dev_ent_t	*un;
	char	   *d_mess = drive->un->dis_mes[DIS_MES_NORM];
	move_flags_t    move_flags;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	struct CatalogEntry ced_flip;
	struct CatalogEntry *ce_flip = &ced_flip;

	un = drive->library->un;
	move_flags.bits = 0;
	(void) spin_drive(drive, SPINDOWN, NOEJECT);

	mutex_lock(&drive->un->mutex);
	close_unit(drive->un, &drive->open_fd);
	drive->un->status.bits = (DVST_PRESENT | DVST_REQUESTED) |
	    (drive->un->status.bits & DVST_CLEANING);
	DevLog(DL_DETAIL(5050));
	mutex_unlock(&drive->un->mutex);

	memccpy(d_mess, catgets(catfd, SET, 9066,
	    "flip media"), '\0', DIS_MES_LEN);

	if (IS_GENERIC_API(drive->library->un->type)) {

#if !defined(SAM_OPEN_SOURCE)
		/* the grau won't do a flip, you need to unload and load */
		int		partition;

		ce = CatalogGetCeByMedia(sam_mediatoa(drive->un->type),
		    drive->un->vsn, &ced);
		if (ce == NULL)
			return (1);

		if (api_unload_media(drive->library, drive) != 0) {
			/* function exits with lock */
			drive->un->mid = ce->CeMid;
			return (1);
		}
		if (ce->CePart == 1)
			partition = 2;
		else if (ce->CePart == 2)
			partition = 1;
		else
			return (1);

		ce_flip = (CatalogGetCeByLoc(ce->CeEq, ce->CeSlot,
		    partition, &ced_flip));
		if (ce_flip == NULL)
			return (1);

		drive->un->mid = ce_flip->CeMid;
		memcpy(drive->un->vsn, ce_flip->CeVsn, sizeof (drive->un->vsn));
		memcpy(drive->bar_code, ce_flip->CeBarCode,
		    sizeof (drive->bar_code));
		memcpy(drive->aci_drive_entry->volser, ce_flip->CeBarCode,
		    sizeof (drive->aci_drive_entry->volser));

		/* mark as loaded */
		if (api_load_media(drive->library, drive, ce) != 0) {
			/* function exits with lock */
			drive->status.b.full = FALSE;
			drive->status.b.bar_code = FALSE;
			drive->aci_drive_entry->volser[0] = '\0';
			return (1);
		}
#endif
	} else if (IS_STORAGE(drive->library, drive->media_element)) {
		if (move_media(drive->library, 0, drive->element,
		    drive->media_element,
		    drive->status.b.d_st_invert, move_flags))
			return (1);

		if (move_media(drive->library, 0, drive->media_element,
		    drive->element,
		    !drive->status.b.d_st_invert, move_flags))
			return (1);
	} else {
		if (move_media(drive->library, 0, drive->element,
		    drive->element, 1,
		    move_flags))
			return (1);
	}

	mutex_lock(&drive->un->mutex);
	/* For the grau, this is handled in the load() function */
	if (drive->library->un->type != DT_GRAUACI)
		drive->status.b.d_st_invert = !drive->status.b.d_st_invert;
	drive->un->status.b.scanning = TRUE;
	mutex_unlock(&drive->un->mutex);

	if (spin_drive(drive, SPINUP, NOEJECT)) {
		/* spin_drive() prints error - downs drive or media */
		if (drive->un->state > DEV_ON)
			return (1);
		return (2);
	}
	/*
	 * scan_a_device must be called with correct partition
	 */
	if (orig_part == 1) {
		drive->un->i.ViPart = 2;
	} else {
		drive->un->i.ViPart = 1;
	}

	/*
	 * Fill in the un with correct information for the newly loaded
	 * volume. This has already been done above for the DAS api before
	 * calling api_load_media and it needs to be done here for scsi
	 * attached libraries before calling scan_a_drive and
	 * CatalogVolumeLoaded.
	 */
	if (!(IS_GENERIC_API(drive->library->un->type))) {
		ce_flip = (CatalogGetCeByLoc(un->eq, drive->un->slot,
		    drive->un->i.ViPart, &ced_flip));
		if (ce_flip == NULL) {
			return (1);
		}
		drive->un->mid = ce_flip->CeMid;
		memcpy(drive->un->vsn, ce_flip->CeVsn, sizeof (drive->un->vsn));
		memcpy(drive->bar_code, ce_flip->CeBarCode,
		    sizeof (drive->bar_code));
	}
	scan_a_device(drive->un, drive->open_fd);
	UpdateCatalog(drive->un, 0, CatalogVolumeLoaded);
	return (0);
}


/*
 *	get_media - get the media mounted.
 *
 * entry   -
 *		   drive->mutex should be held.	 The mutex
 *		   can be released during processing, but will be held
 *		   on return.
 *
 * returns -
 *		   0 - ok
 *		   1 - some sort of error, dispose of event
 *			   Note: The catalog mutex is held on this condition.
 *		   2 - event was requeued.
 *
 *		   in all cases, dev_ent activity count will be incremented.
 */
int
get_media(
	library_t *library,
	drive_state_t *drive,
	robo_event_t *event,	/* may be NULL */
	struct CatalogEntry *ce)
{
	if (IS_GENERIC_API(library->un->type))
		return (api_get_media(library, drive, event, ce));
	else
		return (generic_get_media(library, drive, event, ce));
}


/*
 *	generic_get_media() - generic version (non-API) of get_media()
 */
int
generic_get_media(
		library_t *library,
		drive_state_t *drive,
		robo_event_t *event,	/* may be NULL */
		struct CatalogEntry *ce)
{
	int		other_side;
	int		is_new_flip, err;
	int		status;
	char	   *d_mess = drive->un->dis_mes[DIS_MES_NORM];
	ushort_t	media_element;
	dev_ent_t	*un = drive->un;
	move_flags_t    move_flags;

	mutex_lock(&un->mutex);
	INC_ACTIVE(un);
	mutex_unlock(&un->mutex);

	media_element = ELEMENT_ADDRESS(library, ce->CeSlot);

	if (ce->CePart == 2) {
		is_new_flip = 1;
	} else {
		is_new_flip = 0;
	}

	/* get the flip entry of the media to be mounted. */
	if (library->status.b.two_sided) {
		other_side = (ce->CePart == 1) ? 2 : 1;
	}
	/* is the media already mounted */
	if (drive->status.b.valid && (drive->media_element == media_element)) {
		DevLog(DL_DETAIL(5051), ce->CeSlot);

		/*
		 * Check if the correct side is up
		 */
		if ((library->status.b.two_sided) &&
		    (un->i.ViPart != ce->CePart)) {

			mutex_lock(&un->mutex);
			un->status.b.labeled = FALSE;
			un->status.b.ready = FALSE;
			clear_un_fields(un);
			un->space = ce->CeSpace;
			switch (un->type & DT_CLASS_MASK) {
			case DT_OPTICAL:
				un->dt.od.ptoc_fwa = ce->m.CePtocFwa;
				break;
			case DT_TAPE:
				un->dt.tp.position = 0; /* entry->ptoc_fwa */
				break;
			}
			un->mid = ce->CeMid;
			un->slot = ce->CeSlot;
			mutex_unlock(&un->mutex);

			/*
			 * The requested platter is in the drive, but needs
			 * to be flipped to get the requested side.
			 * flip_and_scan expects to be called with the
			 * partition for the side currently in the drive.
			 */
			if (flip_and_scan(un->i.ViPart, drive)) {
				clear_drive(drive);
				mutex_lock(&un->mutex);
				clear_driver_idle(drive, drive->open_fd);
				close_unit(un, &drive->open_fd);
				un->status.b.requested = FALSE;
				mutex_unlock(&un->mutex);
				return (RET_GET_MEDIA_DISPOSE);
			}
		}
	} else {		/* get the media loaded */

		if (drive->status.b.full) {
			char *MES_9067 = catgets(catfd, SET, 9067,
			    "unloading %s");
			char	   *MES_9068 = catgets(catfd, SET, 9068,
			    "dismount %s/mount %s");
			char	   *mess = (char *)
			    malloc_wait(strlen(MES_9067) +
			    strlen(un->vsn) + 4, 2, 0);

			mutex_lock(&un->mutex);
			un->status.bits |= DVST_UNLOAD;
			mutex_unlock(&un->mutex);
			sprintf(mess, MES_9067, un->vsn);
			memccpy(d_mess, mess, '\0', DIS_MES_LEN);
			free(mess);
			(void) spin_drive(drive, SPINDOWN, NOEJECT);

			mutex_lock(&un->mutex);
			close_unit(un, &drive->open_fd);
			un->status.bits = (DVST_REQUESTED | DVST_PRESENT) |
			    (un->status.bits & DVST_CLEANING);
			clear_un_fields(un);
			mutex_unlock(&un->mutex);

			if (un->open_count)
				DevLog(DL_DEBUG(5196));
			move_flags.bits = 0;

			mess = (char *)malloc_wait(strlen(MES_9068) +
			    strlen(un->vsn) +
			    strlen(ce->CeVsn) + 4, 2, 0);

			sprintf(mess, MES_9068, un->vsn, ce->CeVsn);
			memccpy(d_mess, mess, '\0', DIS_MES_LEN);
			free(mess);

			un->i.ViFlags = VI_cart;
			memmove(un->i.ViMtype, sam_mediatoa(un->type),
			    sizeof (un->i.ViMtype));
			if (*un->i.ViMtype != '\0')
				un->i.ViFlags |= VI_mtype;
			memmove(un->i.ViVsn, un->vsn, sizeof (un->i.ViVsn));
			if (*un->i.ViVsn != '\0')
				un->i.ViFlags |= VI_vsn;
			un->i.ViEq = un->fseq;
			un->i.ViSlot = un->slot;

			if ((err = check_and_set_occupy(library,
			    drive, event, ce))) {
				return (err);
			}
			if ((err = exchange_media(library, media_element,
			    drive->element,
			    drive->media_element, is_new_flip,
			    drive->status.b.d_st_invert, move_flags))) {
				drive->status.b.full = TRUE;
				DevLog(DL_DEBUG(5084));

				/* Determine if occupy bit needs to be set */
				(void) exchange_media_failure(library, ce);

				return (RET_GET_MEDIA_DISPOSE);	/* if error */
			}
			if (CatalogVolumeUnloaded(&un->i, "") == -1) {
				DevLog(DL_SYSERR(5336), un->slot);
			}
		} else {	/* drive is not currently full */

			char	   *MES_9069 = catgets(catfd, SET, 9069,
			    "mounting %s");
			char	   *mess;

			mess = (char *)malloc_wait(strlen(MES_9069)
			    + strlen(ce->CeVsn) + 4, 2, 0);
			move_flags.bits = 0;
			sprintf(mess, MES_9069, ce->CeVsn);
			memccpy(d_mess, mess, '\0', DIS_MES_LEN);
			free(mess);

			if ((err = check_and_set_occupy(library, drive,
			    event, ce))) {
				return (err);
			}
			if ((err = move_media(library, 0, media_element,
			    drive->element,
			    is_new_flip, move_flags))) {
				char	   *MES_9070 =
				    catgets(catfd, SET, 9070,
				    "mount of %s failed");
				char	   *mess = (char *)
				    malloc_wait(strlen(MES_9070) +
				    strlen(ce->CeVsn) + 4, 2, 0);
				if (err == RECOVERED_MEDIA_MOVE ||
				    err == INCOMPATIBLE_MEDIA_MOVE) {
					status = 0;
					status |= CES_occupied;
					if (err == INCOMPATIBLE_MEDIA_MOVE) {
						status |= CES_bad_media;
					}
					(void) CatalogSetFieldByLoc(
					    library->un->eq, ce->CeSlot,
					    ce->CePart, CEF_Status, status, 0);
					if (library->status.b.two_sided) {
						other_side = (ce->CePart == 1)
						    ? 2 : 1;
						(void) CatalogSetFieldByLoc(
						    library->un->eq, ce->CeSlot,
						    other_side, CEF_Status,
						    status, 0);
					}
				}
				if (err == S_ELE_EMPTY) {
					DevLog(DL_ERR(5197), ce->CeVsn,
					    ce->CeSlot);
				}
				sprintf(mess, MES_9070, ce->CeVsn);
				memccpy(d_mess, mess, '\0', DIS_MES_LEN);
				free(mess);
				DevLog(DL_ERR(5054), ce->CeVsn, err);
				return (RET_GET_MEDIA_DISPOSE);	/* if error */
			}
		}

		drive->media_element = media_element;
		drive->status.b.d_st_invert = is_new_flip;
		drive->status.b.full = TRUE;
		drive->status.b.valid = TRUE;

		mutex_lock(&un->mutex);
		memmove(un->vsn, ce->CeVsn, sizeof (un->vsn));
		un->i.ViPart = ce->CePart;
		un->mid = ce->CeMid;
		un->slot = ce->CeSlot;
		if (library->status.b.two_sided) {
			un->flip_mid = (ce->CePart == 1) ? 2 : 1;
		} else {
			un->flip_mid = ROBOT_NO_SLOT;
		}
		un->status.b.labeled = FALSE;
		un->status.b.ready = FALSE;
		mutex_unlock(&un->mutex);
	}

	if (ce->CeStatus & CES_bar_code) {
		memcpy(drive->bar_code, ce->CeBarCode, BARCODE_LEN);
	} else {
		memset(drive->bar_code, 0, BARCODE_LEN);
	}

	mutex_lock(&un->mutex);
	un->space = ce->CeSpace;
	un->status.b.read_only = (ce->CeStatus & CES_read_only);
	un->status.b.bad_media = (ce->CeStatus & CES_bad_media);

	switch (un->type & DT_CLASS_MASK) {
	case DT_OPTICAL:
		un->dt.od.ptoc_fwa = ce->m.CePtocFwa;
		break;

	case DT_TAPE:
		un->dt.tp.position = ce->m.CeLastPos;
		break;
	}

	mutex_unlock(&un->mutex);
	return (RET_GET_MEDIA_SUCCESS);
}


/*
 * GRAU
 */


/*
 * api_load_media - load a drive
 *
 */
static int
api_load_media(
		library_t *library,
		drive_state_t *drive,
		struct CatalogEntry *ce)
{
#if !defined(SAM_OPEN_SOURCE)
	int		local_retry, last_derrno = -1;
	char	   *d_mess = drive->un->dis_mes[DIS_MES_NORM];
	dev_ent_t	*un = drive->un;
	api_errs_t	ret;
	int		d_errno;
	char	   *tag = "load";
	char	   *MES_9069 = catgets(catfd, SET, 9069, "mounting %s");
	char	   *mess = (char *)
	    malloc_wait(strlen(MES_9069) +
	    strlen(ce->CeVsn) + 4, 2, 0);

	sprintf(mess, MES_9069, ce->CeVsn);
	memccpy(d_mess, mess, '\0', DIS_MES_LEN);
	free(mess);

	local_retry = 3;
	ret = API_ERR_TR;
	while (local_retry > 0) {
		if ((aci_load_media(library, drive, ce, &d_errno) == 0)) {
			break;
		} else {

			/* Do not retry if volume in use */
			if (d_errno == EINUSE) {
				local_retry = 0;
				break;
			}
			/* Error return on api call */
			if (d_errno == 0) {
				/*
				 * if call did not happen - error return but
				 * no error
				 */
				local_retry = -1;
				d_errno = EAMUCOMM;
			} else if ((last_derrno == -1) || (last_derrno !=
			    d_errno)) {
				/* Save error if repeated */
				last_derrno = d_errno;
				if (api_valid_error(library->un->type,
				    d_errno, library->un)) {

					if (ce->CeSlot != ROBOT_NO_SLOT) {
						DevLog(DL_DEBUG(6001),
						    ce->CeSlot,
						    tag, d_errno, d_errno,
						    api_return_message(
						    library->un->type,
						    d_errno));
					} else {
						DevLog(DL_DEBUG(6043), tag,
						    d_errno, d_errno,
						    api_return_message(
						    library->un->type,
						    d_errno));
					}

					local_retry = api_return_retry(
					    library->un->type, d_errno);
					ret = api_return_degree(
					    library->un->type, d_errno);

				} else {
					local_retry = -2;
				}
			}
			if (local_retry > 0) {
				/* delay before retrying */
				local_retry--;
				if (local_retry > 0)
					sleep(
					    api_return_sleep(
					    library->un->type,
					    d_errno));

			}
		}
	}

	if ((d_errno != EOK) || (local_retry <= 0)) {
		/* We need to make sure we put out the d_errno message */
		DevLog(DL_ERR(6036), ce->CeVsn);
		if (local_retry == -1) {
			/* The call didn't happen */
			DevLog(DL_ERR(6040), tag);
		} else if (d_errno != EOK) {
			if (api_valid_error(library->un->type,
			    d_errno, library->un)) {
				if (ce->CeSlot != ROBOT_NO_SLOT) {
					DevLog(DL_ERR(6001), ce->CeSlot,
					    tag, d_errno, d_errno,
					    api_return_message(
					    library->un->type, d_errno));
				} else {
					DevLog(DL_ERR(6043), tag,
					    d_errno, d_errno,
					    api_return_message(
					    library->un->type,
					    d_errno));
				}
			}
		}
		if (local_retry == 0) {
			/* retries exceeded */
			DevLog(DL_ERR(6039), tag);
		}
		/* Since the load failed, the tapes are still in their slots */
		if (d_errno == ENOVOLUME) {
			/*
			 * no VSN located at the source of the move -- update
			 * the catalog
			 */
			DevLog(DL_ERR(6037), ce->CeVsn, ce->CeSlot);
		}
		if (ret == API_ERR_DD) {
			return (RET_GET_MEDIA_DOWN_DRIVE);
		} else if (ret == API_ERR_DM) {
			/*
			 * The vsn and entry have to be set to get the tape
			 * and catalog
			 */
			strncpy((char *)un->vsn, ce->CeVsn, sizeof (vsn_t));
			set_bad_media(un);
			/*
			 * Since the load didn't happen don't set bad_media
			 * on the drive
			 */
			un->status.b.bad_media = FALSE;
			memset((char *)un->vsn, 0, sizeof (vsn_t));
			return (RET_GET_MEDIA_RET_ERROR_BAD_MEDIA);
		} else if (ret == API_ERR_DL) {
			down_library(library, SAM_STATE_CHANGE);
			return (RET_GET_MEDIA_RET_ERROR);
		}
		return (RET_GET_MEDIA_DISPOSE);
	}
	if (library->status.b.two_sided) {
		mutex_lock(&drive->un->mutex);
		if (ce->CePart == 2) {
			drive->status.b.d_st_invert = TRUE;
		} else {
			drive->status.b.d_st_invert = FALSE;
		}
		mutex_unlock(&drive->un->mutex);
	}
	mutex_lock(&drive->un->mutex);
	drive->media_element = ELEMENT_ADDRESS(library, ce->CeSlot);
	mutex_unlock(&drive->un->mutex);

	return (RET_GET_MEDIA_SUCCESS);
#endif
}


/*
 * api_unload_media - unload a drive
 *
 *
 */
static int
api_unload_media(
		library_t *library,
		drive_state_t *drive)
{
#if !defined(SAM_OPEN_SOURCE)
	int		local_retry, last_derrno = -1;
	dev_ent_t	*un = drive->un;
	char	   *d_mess = drive->un->dis_mes[DIS_MES_NORM];
	api_errs_t	ret;
	char	   *tag = "unload";
	int		d_errno;
	char	   *mess;
	char	   *MES_9146 = catgets(catfd, SET, 9146, "dismount %s");

	mutex_lock(&un->mutex);
	un->status.bits |= DVST_UNLOAD;
	un->i.ViFlags = VI_cart;
	memmove(un->i.ViMtype, sam_mediatoa(un->type), sizeof (un->i.ViMtype));
	if (*un->i.ViMtype != '\0')
		un->i.ViFlags |= VI_mtype;
	memmove(un->i.ViVsn, un->vsn, sizeof (un->i.ViVsn));
	if (*un->i.ViVsn != '\0')
		un->i.ViFlags |= VI_vsn;
	un->i.ViEq = un->fseq;
	un->i.ViSlot = un->slot;
	un->i.ViPart = 0;
	mutex_unlock(&un->mutex);

	if (un->status.b.ready) {
		sprintf(d_mess, "unloading %s", un->vsn);
		(void) spin_drive(drive, SPINDOWN, NOEJECT);
	}
	mutex_lock(&un->mutex);
	close_unit(un, &drive->open_fd);
	/* Capture existing information about this media */
	un->status.bits = (DVST_REQUESTED | DVST_PRESENT) |
	    (un->status.bits & DVST_CLEANING);
	clear_un_fields(un);
	mutex_unlock(&un->mutex);

	mess = (char *)malloc_wait(strlen(MES_9146) +
	    strlen(un->vsn) + 4, 2, 0);
	sprintf(mess, MES_9146, un->vsn);
	memccpy(d_mess, mess, '\0', DIS_MES_LEN);
	free(mess);

	local_retry = 3;
	ret = API_ERR_TR;
	while (local_retry > 0) {
		if ((aci_dismount_media(library, drive,
		    &d_errno) == 0)) {
			break;
		} else {
			/* Error return on api call */
			if (d_errno == 0) {
				/*
				 * if call did not happen - error return but
				 * no error
				 */
				local_retry = -1;
				d_errno = EAMUCOMM;
			} else if ((last_derrno == -1) ||
			    (last_derrno != d_errno)) {
				/* Save error if repeated */
				last_derrno = d_errno;
					if (api_valid_error(library->un->type,
					    d_errno, library->un)) {

					if (un->slot) {
						DevLog(DL_DEBUG(6001), un->slot,
						    tag, d_errno, d_errno,
						    api_return_message(
						    library->un->type,
						    d_errno));
					} else {
						DevLog(DL_DEBUG(6043),
						    tag, d_errno, d_errno,
						    api_return_message(
						    library->un->type,
						    d_errno));
					}

					local_retry = api_return_retry(
					    library->un->type, d_errno);
					ret = api_return_degree(
					    library->un->type, d_errno);
				} else {
					local_retry = -2;
				}
			}
			if (local_retry > 0) {
				/* delay before retrying */
				local_retry--;
				if (local_retry > 0)
					sleep(api_return_sleep(
					    library->un->type,
					    d_errno));
			}
		}
	}
	if ((d_errno != EOK) || (local_retry <= 0)) {
		/* We need to make sure we put out the d_errno message */
		DevLog(DL_ERR(6033), un->vsn);
		if (local_retry == -1) {
			/* The call didn't happen */
			DevLog(DL_ERR(6040), tag);
		} else if (d_errno != EOK) {
			if (api_valid_error(library->un->type,
			    d_errno, library->un)) {
				if (un->slot != ROBOT_NO_SLOT) {
					DevLog(DL_ERR(6001), un->slot,
					    tag, d_errno, d_errno,
					    api_return_message(
					    library->un->type,
					    d_errno));
				} else {
					DevLog(DL_ERR(6043), tag,
					    d_errno, d_errno,
					    api_return_message(
					    library->un->type,
					    d_errno));
				}
			}
		}
		if (local_retry == 0) {
			/* retries exceeded */
			DevLog(DL_ERR(6039), tag);
		}
		/*
		 * Since the unload didn't succeed, the media is still in the
		 * drive
		 */
		/* Must exit with lock */
		if (ret == API_ERR_DL)
			down_library(library, SAM_STATE_CHANGE);
		else if (ret == API_ERR_DD)
			down_drive(drive, SAM_STATE_CHANGE);
		return (1);
	}
	mutex_lock(&drive->un->mutex);
	/* put the slot number in here */
	drive->media_element = (ushort_t)- 1;
	if (CatalogVolumeUnloaded(&un->i, "") == -1) {
		DevLog(DL_SYSERR(5336), un->slot);
	}
	mutex_unlock(&drive->un->mutex);

	mutex_lock(&un->mutex);
	un->label_time = 0;
	mutex_unlock(&un->mutex);

	return (0);
#endif
}


/*
 *	api_get_media - get the media mounted.
 *
 * entry   -
 *		   drive->mutex should be held.	 This mutex
 *		   can be released during processing, but will be held
 *		   on return.
 *
 *	in all cases, on return, dev_ent activity count
 *  will be incremented.
 */
int
api_get_media(
		library_t *library,
		drive_state_t *drive,
		robo_event_t *event,
		struct CatalogEntry *ce)
{				/* catalog entry of volume to be mounted */
#if !defined(SAM_OPEN_SOURCE)
	ushort_t	media_element;
	dev_ent_t	*un = drive->un;
	int		is_new_flip;
	int		other_side;
	int		status = 0;

	mutex_lock(&un->mutex);
	INC_ACTIVE(un);
	mutex_unlock(&un->mutex);

	media_element = ELEMENT_ADDRESS(library, ce->CeSlot);

	if (ce->CePart == 2) {
		is_new_flip = 1;
	} else {
		is_new_flip = 0;
	}

	/* is the media is already mounted */
	if (drive->status.b.valid && (drive->media_element == media_element)) {
		DevLog(DL_DEBUG(6028), ce->CeVsn);

		/* Make sure it is right side up */
		if ((library->status.b.two_sided) &&
		    (un->i.ViPart != ce->CePart)) {

			mutex_lock(&un->mutex);
			un->status.b.labeled = FALSE;
			un->status.b.ready = FALSE;
			clear_un_fields(un);
			un->space = ce->CeSpace;
			switch (un->type & DT_CLASS_MASK) {
			case DT_OPTICAL:
				un->dt.od.ptoc_fwa = ce->m.CePtocFwa;
				break;

				/* Do NOT restore position from catalog */
			default:
			case DT_TAPE:
				un->dt.tp.position = 0; /* entry->ptoc_fwa */
				break;
			}

			un->slot = ce->CeSlot;
			un->mid = ce->CeMid;
			mutex_unlock(&un->mutex);

			if (flip_and_scan(un->i.ViPart, drive)) {
				clear_drive(drive);
				mutex_lock(&un->mutex);
				clear_driver_idle(drive, drive->open_fd);
				close_unit(un, &drive->open_fd);
				un->status.b.requested = FALSE;
				mutex_unlock(&un->mutex);
				return (RET_GET_MEDIA_DISPOSE);
			}
			un->mid = ce->CeMid;
			memcpy(un->vsn, ce->CeVsn, sizeof (vsn_t));
			drive->open_fd = open_unit(un, un->name, 2);
			un->status.bits |= DVST_OPENED;
			INC_ACTIVE(un);
			un->status.bits |= DVST_SCANNING;
			scan_a_device(un, drive->open_fd);
			UpdateCatalog(un, 0, CatalogVolumeLoaded);

			mutex_lock(&un->mutex);
			close_unit(un, &drive->open_fd);
			DEC_ACTIVE(un);
			mutex_unlock(&un->mutex);
		}
	} else {		/* get the media loaded */
		int		err;

		/*
		 * Make sure the source storage element has media in it. If
		 * the element is empty, external requests will be put back
		 * on the library's work list with the hope that it will be
		 * picked up later. Internal requests will return an error to
		 * the caller. If the in_use flag is not set, then the slot
		 * is "really" empty and the request will be disposed of.
		 */
		DevLog(DL_DEBUG(6029), ce->CeVsn);

		/* If the slot is not occupied: */
		if (!(ce->CeStatus & CES_occupied)) {
			if ((ce->CeStatus & CES_inuse) && event != NULL &&
			    event->type != EVENT_TYPE_INTERNAL &&
			    !event->status.b.dont_reque) {
				/* Put the event back on the list */
				event->next = NULL;
				add_to_end(library, event);
				/* Do not dispose of event */
				return (RET_GET_MEDIA_REQUEUED);
			}
			DevLog(DL_DEBUG(6030), ce->CeVsn);
			return (RET_GET_MEDIA_DISPOSE);
		} else {
			status &= ~CES_occupied;
			(void) CatalogSetFieldByLoc(library->un->eq,
				    ce->CeSlot, ce->CePart,
					CEF_Status, status, CES_occupied);
			if (library->status.b.two_sided) {
				other_side = (ce->CePart == 1) ? 2 : 1;
				(void) CatalogSetFieldByLoc(library->un->eq,
				    ce->CeSlot,
				    other_side, CEF_Status,
				    status, CES_occupied);
			}
		}

		if (drive->status.b.full) {
			if ((err = api_unload_media(library, drive) != 0)) {
				return (err);
			}
			/*
			 * clean up the old entry
			 */
			mutex_lock(&un->mutex);
			un->slot = ROBOT_NO_SLOT;
			un->mid = un->flip_mid = ROBOT_NO_SLOT;
			mutex_unlock(&un->mutex);
			/* clear the drive info just in case the mount fails */
			drive->status.b.full = FALSE;
			drive->status.b.bar_code = FALSE;
			drive->aci_drive_entry->volser[0] = '\0';
		}
		if ((err = api_load_media(library, drive, ce)) != 0) {
			status |= CES_occupied;
			(void) CatalogSetFieldByLoc(library->un->eq, ce->CeSlot,
					ce->CePart, CEF_Status, status, 0);
			return (err);
		}
	}

	mutex_lock(&un->mutex);
	drive->status.b.full = TRUE;
	drive->status.b.valid = TRUE;
	drive->media_element = media_element;
	drive->status.b.d_st_invert = is_new_flip;
	memmove(un->vsn, ce->CeVsn, sizeof (un->vsn));
	un->slot = ce->CeSlot;
	un->i.ViPart = ce->CePart;
	un->mid = ce->CeMid;
	if (library->status.b.two_sided) {
		un->flip_mid = (ce->CePart == 1) ? 2 : 1;
	} else {
		un->flip_mid = ROBOT_NO_SLOT;
	}
	un->status.b.labeled = FALSE;
	un->status.b.ready = FALSE;
	drive->status.b.bar_code = TRUE;
	memcpy(drive->bar_code, ce->CeBarCode, BARCODE_LEN + 1);
	memcpy(drive->aci_drive_entry->volser, ce->CeBarCode, ACI_VOLSER_LEN);
	un->space = ce->CeSpace;

	switch (un->type & DT_CLASS_MASK) {
	case DT_OPTICAL:
		un->dt.od.ptoc_fwa = ce->m.CePtocFwa;
		break;

		/* Do NOT restore position from catalog */
	case DT_TAPE:
		un->dt.tp.position = 0; /* entry->ptoc_fwa */
		break;
	}
	mutex_unlock(&un->mutex);

	return (RET_GET_MEDIA_SUCCESS);
#endif
}


/*
 *	All of these function follow use the following for exit conditions..
 *
 *	   If the aci call was executed then the return value is the
 *	   value of the aci_call(from api_status in the grau_resp_api)
 *	   and d_errno(d_errno in the grau_resp_api) will be
 *	   the value of d_errno.
 *
 *	   The the call was not executed, the the return value is
 *	   errno from where ever it failed and d_errno will be 0.
 *
 */

/*
 *	aci_force_media - unload a drive.
 *
 */
int
aci_force_media(
		library_t *library,
		drive_state_t *drive,
		int *d_errno)
{
#if !defined(SAM_OPEN_SOURCE)
	int		err;
	char	   *d_mess = drive->un->dis_mes[DIS_MES_NORM];
	xport_state_t  *transport;
	robo_event_t   *force;
	aci_information_t *aci_info;
	dev_ent_t	*un = drive->un;

	aci_info = malloc_wait(sizeof (aci_information_t), 2, 0);
	memset(aci_info, 0, sizeof (aci_information_t));
	aci_info->aci_drive_entry = drive->aci_drive_entry;
	aci_info->aci_vol_desc = NULL;

	/* Build transport thread request */

	force = get_free_event(library);
	(void) memset(force, 0, sizeof (robo_event_t));

	force->request.internal.command = ROBOT_INTRL_FORCE_MEDIA;
	force->request.internal.address = (void *) aci_info;

	DevLog(DL_DEBUG(6008), drive->aci_drive_entry->drive_name);

	memccpy(d_mess, "aci_force_media", '\0', DIS_MES_LEN);
	force->type = EVENT_TYPE_INTERNAL;
	force->status.bits = REST_SIGNAL;
	force->completion = REQUEST_NOT_COMPLETE;

	transport = library->transports;
	mutex_lock(&force->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0) {
		ETRACE((LOG_NOTICE, "EV:FrTr:force: %#x.", force));
		transport->first = force;
	} else {
		robo_event_t   *tmp;

		LISTEND(transport, tmp);
		ETRACE((LOG_NOTICE, "EV:ApTr:force: %#x: %#x(%d).", tmp, force,
		    transport->active_count));
		append_list(tmp, force);
	}
	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);

	/* Wait for the transport to do the unload */
	while (force->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&force->condit, &force->mutex);
	mutex_unlock(&force->mutex);

	*d_errno = err = force->completion;
	DevLog(DL_DEBUG(6009), err ? error_handler(err) : "no error");

	/*
	 * No error means the aci command was started and that the response
	 * is in the returned data.
	 */
	if (!err) {
		api_resp_api_t *response =
		    (api_resp_api_t *)&
		    force->request.message.param.start_of_request;

		/* Did the aci call return an error */
		if ((err = response->api_status)) {
			*d_errno = response->d_errno;
			if ((*d_errno >= 0) && (*d_errno < NO_ECODES)) {
				DevLog(DL_DEBUG(6010), err,
				    response->d_errno,
				    grau_messages[*d_errno].mess);
			} else {
				DevLog(DL_DEBUG(6010), err,
				    response->d_errno, "unknown error");
			}
		}
	} else {
		/*
		 * This is bad a call error means the helper and the daemon
		 * are not talking correctly. This should be logged and in
		 * english.
		 */
		DevLog(DL_ERR(6011), error_handler(err));
	}

	free(aci_info);
	mutex_destroy(&force->mutex);
	cond_destroy(&force->condit);
	free(force);
	return (err);
#endif
}


/*
 *	aci_drive_access - set driveaccess.
 *
 */
int
aci_drive_access(
		library_t *library,
		drive_state_t *drive,
		int *d_errno)
{
#if !defined(SAM_OPEN_SOURCE)
	int		err;
	char	   *ent_pnt = "aci_drive_access";
	char	   *d_mess = drive->un->dis_mes[DIS_MES_NORM];
	xport_state_t  *transport;
	robo_event_t   *drv_access;
	aci_information_t *aci_info;
	dev_ent_t	*un = drive->un;

	aci_info = malloc_wait(sizeof (aci_information_t), 2, 0);
	memset(aci_info, 0, sizeof (aci_information_t));
	aci_info->aci_drive_entry = drive->aci_drive_entry;

	/* Build transport thread request */

	drv_access = get_free_event(library);
	(void) memset(drv_access, 0, sizeof (robo_event_t));

	drv_access->request.internal.command = ROBOT_INTRL_DRIVE_ACCESS;
	drv_access->request.internal.address = (void *) aci_info;
	if (DBG_LVL(SAM_DBG_DEBUG))
		memccpy(d_mess, ent_pnt, '\0', DIS_MES_LEN);

	DevLog(DL_DEBUG(6012), drive->aci_drive_entry->drive_name);

	drv_access->type = EVENT_TYPE_INTERNAL;
	drv_access->status.bits = REST_SIGNAL;
	drv_access->completion = REQUEST_NOT_COMPLETE;
	transport = library->transports;
	mutex_lock(&drv_access->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0) {
		ETRACE((LOG_NOTICE, "EV:FrTr:access: %#x.", drv_access));
		transport->first = drv_access;
	} else {
		robo_event_t   *tmp;

		ETRACE((LOG_NOTICE, "EV:ApTr:access: %#x: %#x(%d).",
		    tmp, drv_access,
		    transport->active_count));
		LISTEND(transport, tmp);
		append_list(tmp, drv_access);
	}
	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);

	/* Wait for the transport to do the command */
	while (drv_access->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&drv_access->condit, &drv_access->mutex);
	mutex_unlock(&drv_access->mutex);

	*d_errno = err = drv_access->completion;
	ETRACE((LOG_NOTICE, "EV:RtTr:drive_access: %#x.", drv_access));
	DevLog(DL_DEBUG(6013), err ? error_handler(err) : "no error");

	/*
	 * No error means the aci command was started and that the response
	 * is in the returned data.
	 */
	if (!err) {
		api_resp_api_t *response =
		    (api_resp_api_t *)&
		    drv_access->request.message.param.start_of_request;

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

	mutex_destroy(&drv_access->mutex);
	cond_destroy(&drv_access->condit);
	if (aci_info)
		free(aci_info);
	free(drv_access);
	return (err);
#endif
}


/*
 *	aci_dismount_media - unload a volser
 *
 */
int
aci_dismount_media(
		library_t *library,
		drive_state_t *drive,
		int *d_errno)
{
#if !defined(SAM_OPEN_SOURCE)
	int		err;
	char	   *ent_pnt = "aci_dismount_media";
	char	   *d_mess = drive->un->dis_mes[DIS_MES_NORM];
	xport_state_t  *transport;
	robo_event_t   *dismount;
	aci_information_t *aci_info;
	dev_ent_t	*un = drive->un;

	aci_info = malloc_wait(sizeof (aci_information_t), 2, 0);
	memset(aci_info, 0, sizeof (aci_information_t));
	aci_info->aci_drive_entry = drive->aci_drive_entry;
	aci_info->aci_drive_entry->type = sam2aci_type(drive->un->type);
	aci_info->aci_vol_desc = NULL;

	/* Build transport thread request */

	dismount = get_free_event(library);
	(void) memset(dismount, 0, sizeof (robo_event_t));

	dismount->request.internal.command = ROBOT_INTRL_DISMOUNT_MEDIA;
	dismount->request.internal.address = (void *) aci_info;

	if (DBG_LVL(SAM_DBG_DEBUG))
		memccpy(d_mess, ent_pnt, '\0', DIS_MES_LEN);

	DevLog(DL_DEBUG(6016), drive->aci_drive_entry->drive_name);

	dismount->type = EVENT_TYPE_INTERNAL;
	dismount->status.bits = REST_SIGNAL;
	dismount->completion = REQUEST_NOT_COMPLETE;

	transport = library->transports;
	mutex_lock(&dismount->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0) {
		ETRACE((LOG_NOTICE, "EV:FrTr:dismount: %#x.", dismount));
		transport->first = dismount;
	} else {
		robo_event_t   *tmp;

		LISTEND(transport, tmp);
		ETRACE((LOG_NOTICE, "EV:ApTr:dismount: %#x: %#x(%d).",
		    tmp, dismount,
		    transport->active_count));
		append_list(tmp, dismount);
	}
	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);

	/* Wait for the transport to do the unload */
	while (dismount->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&dismount->condit, &dismount->mutex);
	mutex_unlock(&dismount->mutex);

	*d_errno = err = dismount->completion;
	ETRACE((LOG_NOTICE, "EV:RtTr:dismount: %#x.", dismount));
	DevLog(DL_DEBUG(6017), err ? error_handler(err) : "no error");

	/*
	 * No error means the aci command was started and that the response
	 * is in the returned data.
	 */
	if (!err) {
		api_resp_api_t *response =
		    (api_resp_api_t *)&
		    dismount->request.message.param.start_of_request;

		/* Did the aci call return an error */
		if ((err = response->api_status)) {
			*d_errno = response->d_errno;
			memccpy(d_mess, catgets(catfd, SET, 9194,
			    "api response error"),
			    '\0', DIS_MES_LEN);
			if ((*d_errno >= 0) && (*d_errno < NO_ECODES)) {
				DevLog(DL_DETAIL(6018), err, response->d_errno,
				    grau_messages[*d_errno].mess);
			} else {
				DevLog(DL_DETAIL(6018), err,
				    response->d_errno, "unknown error");
			}

		}
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
		DevLog(DL_DETAIL(6019), error_handler(err));
	}

	free(aci_info);
	mutex_destroy(&dismount->mutex);
	cond_destroy(&dismount->condit);
	free(dismount);
	return (err);
#endif
}


/*
 *	aci_load_media - load media into a drive
 *
 */
int
aci_load_media(
		library_t *library,
		drive_state_t *drive,
		struct CatalogEntry *ce,
		int *d_errno)
{
#if !defined(SAM_OPEN_SOURCE)
	int		err;
	char	   *d_mess = drive->un->dis_mes[DIS_MES_NORM];
	char	   *ent_pnt = "aci_load_media";
	xport_state_t  *transport;
	robo_event_t   *load;
	aci_information_t *aci_info;
	dev_ent_t	*un = drive->un;

	aci_info = malloc_wait(sizeof (aci_information_t), 2, 0);
	memset(aci_info, 0, sizeof (aci_information_t));
	aci_info->aci_drive_entry = drive->aci_drive_entry;
	aci_info->aci_vol_desc = malloc_wait(sizeof (aci_vol_desc_t), 2, 0);
	memset(aci_info->aci_vol_desc, 0, sizeof (aci_vol_desc_t));
	aci_info->aci_vol_desc->type = sam2aci_type(sam_atomedia(ce->CeMtype));
	strncpy(aci_info->aci_vol_desc->volser, ce->CeBarCode,
	    ACI_VOLSER_LEN - 1);

	/* Build transport thread request */

	load = get_free_event(library);
	(void) memset(load, 0, sizeof (robo_event_t));
	load->request.internal.command = ROBOT_INTRL_LOAD_MEDIA;
	load->request.internal.address = (void *) aci_info;

	DevLog(DL_DEBUG(6020), ce->CeVsn, drive->aci_drive_entry->drive_name);

	if (DBG_LVL(SAM_DBG_DEBUG))
		sprintf(d_mess, "%s %s", ent_pnt, ce->CeVsn);
	load->type = EVENT_TYPE_INTERNAL;
	load->status.bits = REST_SIGNAL;
	load->completion = REQUEST_NOT_COMPLETE;

	transport = library->transports;
	mutex_lock(&load->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0) {
		ETRACE((LOG_NOTICE, "EV:FrTr:load: %#x.", load));
		transport->first = load;
	} else {
		robo_event_t   *tmp;

		LISTEND(transport, tmp);
		ETRACE((LOG_NOTICE, "EV:ApTr:load: %#x: %#x(%d).", tmp, load,
		    transport->active_count));
		append_list(tmp, load);
	}
	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);

	/* Wait for the transport to do the unload */
	while (load->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&load->condit, &load->mutex);
	mutex_unlock(&load->mutex);

	*d_errno = err = load->completion;
	ETRACE((LOG_NOTICE, "EV:RtTr:load: %#x.", load));
	DevLog(DL_DEBUG(6021), err ? error_handler(err) : "no error");

	if (!err) {
		api_resp_api_t *response =
		    (api_resp_api_t *)&
		    load->request.message.param.start_of_request;

		/* Did the aci call return an error */
		if ((err = response->api_status)) {
			*d_errno = response->d_errno;
			memccpy(d_mess, catgets(catfd, SET, 9194,
			    "api response error"),
			    '\0', DIS_MES_LEN);
			if ((*d_errno >= 0) && (*d_errno < NO_ECODES)) {
				DevLog(DL_DETAIL(6022), err,
				    response->d_errno,
				    grau_messages[*d_errno].mess);
			} else {
				DevLog(DL_DETAIL(6022), err, response->d_errno,
				    "unknown error");
			}
		}
	} else {
		memccpy(d_mess, catgets(catfd, SET, 9196, "protocol error"),
		    '\0', DIS_MES_LEN);
		*d_errno = 0;
		DevLog(DL_DETAIL(6023), error_handler(err));
	}

	free(aci_info->aci_vol_desc);
	free(aci_info);
	mutex_destroy(&load->mutex);
	cond_destroy(&load->condit);
	free(load);
	return (err);
#endif
}


/*
 *	aci_view_media - view a database entry
 *
 */
int
aci_view_media(
		library_t *library,
		struct CatalogEntry *ce,
		int *fj_tplen,
		int *d_errno)
{
#if !defined(SAM_OPEN_SOURCE)
	int		err;
	char	   *l_mess = library->un->dis_mes[DIS_MES_NORM];
	char	   *ent_pnt = "aci_view_media";
	xport_state_t  *transport;
	robo_event_t   *view;
	aci_vol_desc_t *volume_info;
	dev_ent_t	*un = library->un;

	volume_info = malloc_wait(sizeof (aci_vol_desc_t), 2, 0);
	memset(volume_info, 0, sizeof (aci_vol_desc_t));
	strncpy(volume_info->volser, ce->CeBarCode, ACI_VOLSER_LEN - 1);
	volume_info->type = sam2aci_type(sam_atomedia(ce->CeMtype));

	/* Build transport thread request */

	view = get_free_event(library);
	(void) memset(view, 0, sizeof (robo_event_t));
	view->request.internal.command = ROBOT_INTRL_VIEW_DATABASE;
	view->request.internal.address = (void *) volume_info;

	DevLog(DL_DEBUG(6024), ce->CeBarCode);

	sprintf(l_mess, "%s %s", ent_pnt, ce->CeBarCode);
	view->type = EVENT_TYPE_INTERNAL;
	view->status.bits = REST_SIGNAL;
	view->completion = REQUEST_NOT_COMPLETE;

	transport = library->transports;
	mutex_lock(&view->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0) {
		ETRACE((LOG_NOTICE, "EV:FrTr:view: %#x.", view));
		transport->first = view;
	} else {
		robo_event_t   *tmp;

		LISTEND(transport, tmp);
		ETRACE((LOG_NOTICE, "EV:ApTr:view: %#x: %#x(%d).", tmp, view,
		    transport->active_count));
		append_list(tmp, view);
	}
	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);

	/* Wait for the transport to do the unload */
	while (view->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&view->condit, &view->mutex);
	mutex_unlock(&view->mutex);

	*d_errno = err = view->completion;
	ETRACE((LOG_NOTICE, "EV:RtTr:view: %#x.", view));
	DevLog(DL_DEBUG(6025), err ? error_handler(err) : "no error");

	if (!err) {
		api_resp_api_t *response =
		    (api_resp_api_t *)&
		    view->request.message.param.start_of_request;

		/* Did the aci call return an error */
		if ((err = response->api_status)) {
			*d_errno = response->d_errno;
			memccpy(l_mess, catgets(catfd, SET, 9194,
			    "api response error"),
			    '\0', DIS_MES_LEN);
			if ((*d_errno >= 0) && (*d_errno < NO_ECODES)) {
				DevLog(DL_DETAIL(6026), err, response->d_errno,
				    grau_messages[*d_errno].mess);
			} else {
				DevLog(DL_DETAIL(6026), err,
				    response->d_errno, "unknown error");
			}
		}
	} else {
		memccpy(l_mess, catgets(catfd, SET, 9196, "protocol error"),
		    '\0', DIS_MES_LEN);
		*d_errno = 0;
		DevLog(DL_DETAIL(6027), error_handler(err));
	}

	mutex_destroy(&view->mutex);
	cond_destroy(&view->condit);
	free(view);
	free(volume_info);
	return (err);
#endif
}


/*
 * Get the other side of imported, two-sided optical media
 */
int
aci_getside(
	    library_t *library,
	    char *vsn,
	    char *new_vsn,
	    int *d_errno)
{
#if !defined(SAM_OPEN_SOURCE)
	int		err;
	xport_state_t  *transport;
	robo_event_t   *getsideinfo;
	aci_vol_desc_t *volume_info;
	dev_ent_t	*un = library->un;
	char	   *ent_pnt = "aci_getside";
	char	   *l_mess = library->un->dis_mes[DIS_MES_NORM];

	volume_info = malloc_wait(sizeof (aci_vol_desc_t), 2, 0);
	memset(volume_info, 0, sizeof (aci_vol_desc_t));
	strncpy(volume_info->volser, vsn, ACI_VOLSER_LEN - 1);

	/* Build a transport thread request */
	getsideinfo = get_free_event(library);
	(void) memset(getsideinfo, 0, sizeof (robo_event_t));
	getsideinfo->request.internal.command = ROBOT_INTRL_GET_SIDE_INFO;
	getsideinfo->request.internal.address = (void *) volume_info;

	sprintf(l_mess, "%s %s", ent_pnt, vsn);
	getsideinfo->type = EVENT_TYPE_INTERNAL;
	getsideinfo->status.bits = REST_SIGNAL;
	getsideinfo->completion = REQUEST_NOT_COMPLETE;

	transport = library->transports;
	mutex_lock(&getsideinfo->mutex);
	mutex_lock(&transport->list_mutex);
	if (transport->active_count == 0) {
		ETRACE((LOG_NOTICE, "EV:FrTr:view: %#x.", getsideinfo));
		transport->first = getsideinfo;
	} else {
		robo_event_t   *tmp;

		LISTEND(transport, tmp);
		ETRACE((LOG_NOTICE, "EV:ApTr:getsideinfo: %#x: %#x(%d).", tmp,
		    getsideinfo, transport->active_count));
		append_list(tmp, getsideinfo);
	}

	transport->active_count++;
	cond_signal(&transport->list_condit);
	mutex_unlock(&transport->list_mutex);

	while (getsideinfo->completion == REQUEST_NOT_COMPLETE)
		cond_wait(&getsideinfo->condit, &getsideinfo->mutex);
	mutex_unlock(&getsideinfo->mutex);

	*d_errno = err = getsideinfo->completion;
	ETRACE((LOG_NOTICE, "EV:RtTr:getsideinfo: %#x.", getsideinfo));

	if (!err) {
		api_resp_api_t *response =
		    (api_resp_api_t *)&
		    getsideinfo->request.message.param.start_of_request;

		if ((err = response->api_status)) {
			*d_errno = response->d_errno;
			memccpy(l_mess, catgets(catfd, SET, 9194,
			    "api response error"),
			    '\0', DIS_MES_LEN);
			if ((*d_errno >= 0) && (*d_errno < NO_ECODES)) {
				DevLog(DL_DETAIL(6026), err, response->d_errno,
				    grau_messages[*d_errno].mess);
			} else
				DevLog(DL_DETAIL(6026), err, response->d_errno,
				    "unknown error");
		}
		memcpy(new_vsn, &response->data.vol_des.volser, ACI_VOLSER_LEN);
	} else {
		memccpy(l_mess, catgets(catfd, SET, 9196, "protocol error"),
		    '\0', DIS_MES_LEN);
		*d_errno = 0;
		DevLog(DL_DETAIL(6027), error_handler(err));
	}

	mutex_destroy(&getsideinfo->mutex);
	cond_destroy(&getsideinfo->condit);
	free(getsideinfo);
	free(volume_info);
	return (err);
#endif
}


int
check_and_set_occupy(
			library_t *library,
			drive_state_t *drive,
			robo_event_t *event,
			struct CatalogEntry *ce)
{
	int		status;
	dev_ent_t	*un;
	struct CatalogEntry ced_new;
	struct CatalogEntry *ce_new = &ced_new;

	un = drive->un;

	/*
	 * Make sure the source storage element has media in it. If the
	 * element is empty, external requests will be put back on the
	 * library's work list to be picked up later. Internal requests will
	 * return an error to the caller. If the in_use flag is not set, then
	 * the slot is truly empty and the request will be disposed of.
	 */
	DevLog(DL_DETAIL(5052), ce->CeSlot);

	/*
	 * A copy of the catalog entry has been passed in. Use it to get an
	 * updated copy in case the occupy bit status has been changed by
	 * another thread.
	 */
	ce_new = CatalogGetCeByLoc(ce->CeEq, ce->CeSlot, ce->CePart, &ced_new);
	if (ce_new == NULL) {
		DevLog(DL_DETAIL(5053), ce->CeSlot);
		return (RET_GET_MEDIA_DISPOSE);
	}
	/*
	 * Use the library mutex to prevent more than one thread from
	 * checking and setting the occupy bit simultaneously.
	 */
	mutex_lock(&library->mutex);

	/* If the slot is not occupied: */
	if (!(ce_new->CeStatus & CES_occupied)) {
		if ((ce_new->CeStatus & CES_inuse) && event != NULL &&
		    event->type != EVENT_TYPE_INTERNAL &&
		    !event->status.b.dont_reque) {
			/* put the event back on the list */
			event->next = NULL;
			add_to_end(library, event);
			/* Do not dispose of event */
			mutex_unlock(&library->mutex);
			return (RET_GET_MEDIA_REQUEUED);
		}
		DevLog(DL_DETAIL(5053), ce->CeSlot);
		mutex_unlock(&library->mutex);
		return (RET_GET_MEDIA_DISPOSE);
	} else {
		status = 0;
		status &= ~CES_occupied;
		(void) CatalogSetFieldByLoc(library->un->eq, ce_new->CeSlot,
		    ce_new->CePart, CEF_Status, status, CES_occupied);
		if (library->status.b.two_sided) {
			(void) CatalogSetFieldByLoc(library->un->eq,
			    ce_new->CeSlot,
			    ((ce_new->CePart == 1) ? 2 : 1), CEF_Status,
			    status, CES_occupied);
		}
	}
	mutex_unlock(&library->mutex);
	return (RET_GET_MEDIA_SUCCESS);
}


void
exchange_media_failure(
			library_t *library,
			struct CatalogEntry *ce)
{
	int		status;
	drive_state_t  *drive;
	boolean_t	volume_mounted;

	volume_mounted = FALSE;
	/*
	 * On an exchange_media failure, only set the occupy bit if the
	 * requested volume is not in a drive.
	 */
	for (drive = library->drive; drive != NULL; drive = drive->next) {
		if (strcmp(drive->un->vsn, ce->CeVsn) == 0) {
			volume_mounted = TRUE;
		}
	}

	if (!(volume_mounted)) {
		/*
		 * Set the occupy bit for this slot.
		 */
		status = 0;
		status |= CES_occupied;
		(void) CatalogSetFieldByLoc(library->un->eq, ce->CeSlot,
		    ce->CePart, CEF_Status, status, 0);
		if (library->status.b.two_sided) {
			(void) CatalogSetFieldByLoc(library->un->eq, ce->CeSlot,
			    ((ce->CePart == 1) ? 2 : 1), CEF_Status, status, 0);
		}
	}
}
