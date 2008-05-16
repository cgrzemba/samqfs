/*
 * scan.c - common routines for scanning media
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

#pragma ident "$Revision: 1.32 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <time.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/mtio.h>
#include <errno.h>

#include "driver/samst_def.h"
#include "sam/lib.h"
#include "aml/shm.h"
#include "aml/catalog.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"
#include "aml/dev_log.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/sefvals.h"
#include "aml/tapealert.h"
#include "aml/sef.h"

/* globals */

extern shm_alloc_t master_shm;


/*
 * scan_a_device - routine to scan a device.
 *
 * Scanning bit should be set in the status field.
 *
 *    On entry:
 *    un = The unit table pointer.
 *    Scanning bit set.
 *    Active count is zero
 *    fd - open file descriptor
 */

void
scan_a_device(dev_ent_t *un, int fd)
{
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	int ready = FALSE, no_media = 0;
	char *d_mess = un->dis_mes[DIS_MES_NORM];
	uint_t requested;
	uint_t attn[2];
	dtype_t type = un->type & DT_CLASS_MASK;
	struct mtop mt_op = {0, 0};
	sam_extended_sense_t *sense =
	    (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);
	int 	eq;

	if (un->fseq != 0)
		eq = un->fseq;
	else
		eq = un->eq;
	ce = CatalogGetCeByLoc(eq, un->slot, un->i.ViPart, &ced);
	if (ce == NULL) {
		DevLog(DL_ERR(4021), eq, un->slot, un->i.ViPart);
		return;
	}
	un->i.ViEq   = ce->CeEq;
	un->i.ViSlot = ce->CeSlot;
	un->i.ViPart = ce->CePart;
	un->i.ViFlags = VI_cart;
	DevLog(DL_DETAIL(4022), un->i.ViEq, un->i.ViSlot, un->i.ViPart);
	mutex_lock(&un->io_mutex);

	requested = un->status.bits & DVST_REQUESTED;

	/*
	 * If robot, then we know that its ready, so don't test here.
	 */
	if (un->fseq != 0) {
		DevLog(DL_ALL(1006), un->slot);
		ready = TRUE;
	} else {
		int err;

		err = do_tur(un, fd, INITIAL_TUR_TIMEOUT);

		if (err) {
			if (err == NO_MEDIA) {
				no_media = 1;
			} else if (err == NOT_READY ||
			    err == RETRIES_EXCEEDED || (sense->es_key == 6 &&
			    (sense->es_add_code == 0x28 ||
			    sense->es_add_code == 0x29))) {

				mutex_lock(&un->mutex);
				/* force a scan */
				un->status.b.labeled = 0;
				mutex_unlock(&un->mutex);

			} else if (err == (int)NEEDS_FORMATTING) {
				DevLog(DL_DETAIL(3138));
				memccpy(d_mess, catgets(catfd, SET, 1641,
				    "media needs formatting"),
				    '\0', DIS_MES_LEN);

				mutex_lock(&un->mutex);
				un->dt.tp.status.bits |= DVTP_NEEDFORMAT;
				/* force a scan */
				un->status.bits &= ~DVST_LABELED;
				ready = TRUE;
				/* force read */
				un->status.bits |= DVST_READY;
				mutex_unlock(&un->mutex);

			} else {
				/* not an error normally handled, log it */
				DevLog(DL_ERR(5129));
				DevLogCdb(un);
				DevLogSense(un);
				memccpy(d_mess, catgets(catfd, SET, 9071,
				    "error on test unit ready"),
				    '\0', DIS_MES_LEN);
			}
		} else {
			ready = TRUE;
		}
	}

	if (un->state == DEV_IDLE && un->fseq != 0) {
		TAPEALERT(fd, un);
		(void) scsi_cmd(fd, un, SCMD_DOORLOCK, 0, UNLOCK);
		if (!(IS_OPTICAL(un))) {
			(void) sef_data(un, fd);
		}
		TAPEALERT(fd, un);
		mutex_unlock(&un->io_mutex);

		mutex_lock(&un->mutex);
		DownDevice(un, SAM_STATE_CHANGE);
		un->status.bits = requested | DVST_PRESENT;
		memset(un->vsn, ' ', 32);
		if (type == DT_TAPE)
			un->dt.tp.position = un->dt.tp.next_read = 0;

		mutex_unlock(&un->mutex);

		if (ready) {

			if (type == DT_TAPE) {
				mt_op.mt_op = MTOFFL;
				(void) ioctl(fd, MTIOCTOP, &mt_op);
			} else {
				if (ready &&
				    (un->status.bits & DVST_FS_ACTIVE)) {

					notify_fs_invalid_cache(un);
					mutex_lock(&un->mutex);
					un->status.bits &= ~DVST_FS_ACTIVE;
					mutex_unlock(&un->mutex);
				}
				mutex_lock(&un->io_mutex);
				TAPEALERT(fd, un);
				(void) scsi_cmd(fd, un, SCMD_START_STOP, 0,
				    SPINDOWN, EJECT_MEDIA);
				TAPEALERT(fd, un);
				mutex_unlock(&un->io_mutex);
			}
		}
		DevLog(DL_DETAIL(1033));
		memccpy(d_mess, catgets(catfd, SET, 1647,
		    "media unloaded and door unlocked"), '\0', DIS_MES_LEN);
		return;
	}
	mutex_lock(&un->mutex);
	if (un->fseq == 0) {
		if (type != DT_TAPE) {
			/*
			 * Since open processing does a number of
			 * test_unit_readys, we need to get the unit_attention
			 * flag from the driver.
			 *
			 * This flag is cleared when asked for and basically
			 * tells us if a unit attention has occured since the
			 * last time we asked for the status.
			 * Only valid for optical.
			 */

			(void) ioctl(fd, SAMSTIOC_ATTN, &attn);
			if (attn[0]) {
				/* force a scan */
				un->status.b.labeled = 0;
			}
			if (!ready) {
				un->status.bits = requested | DVST_PRESENT;
				memset(un->vsn, ' ', 32);

				if (!no_media) {
					if (un->slot != ROBOT_NO_SLOT) {
						DevLog(DL_ERR(3032), un->slot);
					} else {
						DevLog(DL_ERR(3253));
					}
				}

				mutex_unlock(&un->mutex);
				mutex_unlock(&un->io_mutex);
				return;
			}
		} else {		/* tapes */
			if (!ready) {
				un->status.bits = requested | DVST_PRESENT;
				memset(un->vsn, ' ', 7);

				if (un->slot != ROBOT_NO_SLOT) {
					DevLog(DL_ERR(3032), un->slot);
				} else {
					DevLog(DL_ERR(3253));
				}

				mutex_unlock(&un->mutex);
				mutex_unlock(&un->io_mutex);
				return;
			}
			/*
			 * If the tape was not labeled last pass,
			 * then check again.
			 */
			if (!un->status.b.unload && un->status.b.labeled) {
				un->status.b.scanning = FALSE;
				un->status.b.scanned = TRUE;
				mutex_unlock(&un->mutex);
				mutex_unlock(&un->io_mutex);
				return;	/* nothing to do */
			}
		}
	}
	/* both mutex and io_mutex held at this time */

	if (!un->status.b.labeled) {	/* if need to scan */
		un->status.b.ready = TRUE;

		switch (type) {
		case DT_TAPE:
			process_tape_labels(fd, un);
			un->space = un->capacity;
			break;

		case DT_OPTICAL:
			if (get_capacity(fd, un)) {
				un->status.bits = requested | DVST_PRESENT;
				memset(un->vsn, ' ', 32);
				if (un->fseq != 0) {
					un->status.b.bad_media = TRUE;
				} else {	/* get rid of it */
					TAPEALERT(fd, un);
					(void) scsi_cmd(fd, un, SCMD_DOORLOCK,
					    0, UNLOCK);
					TAPEALERT(fd, un);
					(void) scsi_cmd(fd, un, SCMD_START_STOP,
					    0, SPINDOWN, EJECT_MEDIA);
					TAPEALERT(fd, un);
					memccpy(d_mess,
					    catgets(catfd, SET, 507,
					    "Bad media unloaded and door"
					    " unlocked"), '\0', DIS_MES_LEN);
				}
				mutex_unlock(&un->mutex);
				mutex_unlock(&un->io_mutex);
				return;
			} else {
				process_optic_labels(fd, un);
			}
			break;

		default:
			break;
		}

		un->mtime = time(0) + un->delay;
	}
	un->status.bits &= ~DVST_SCANNING;
	un->status.bits |= DVST_SCANNED;

	if (un->fseq == 0) { /* Manual drive. */

		/*
		 * If the unload bit is set and there is no activity then
		 * spindown and eject.
		 */
		if (un->status.b.unload && (un->active == 0)) {

			if (!(IS_OPTICAL(un))) {
				(void) sef_data(un, fd);
			}
			un->status.bits = requested | DVST_PRESENT;
			TAPEALERT(fd, un);
			(void) scsi_cmd(fd, un, SCMD_DOORLOCK, 0, UNLOCK);
			if (IS_TAPE(un)) {
				un->dt.tp.position = un->dt.tp.next_read = 0;
			} else {
				if (un->status.bits & DVST_FS_ACTIVE) {
					notify_fs_invalid_cache(un);
					un->status.bits &= ~DVST_FS_ACTIVE;
				}
			}

			memccpy(d_mess,
			    catgets(catfd, SET, 2789, "unloading"),
			    '\0', DIS_MES_LEN);
			TAPEALERT(fd, un);
			(void) scsi_cmd(fd, un, SCMD_START_STOP, 120,
			    SPINDOWN, EJECT_MEDIA);
			TAPEALERT(fd, un);
			memccpy(d_mess, catgets(catfd, SET, 2788,
			    "unloaded and door unlocked"), '\0', DIS_MES_LEN);
			DevLog(DL_DETAIL(1027));
			memmove(un->i.ViMtype, sam_mediatoa(un->type),
			    sizeof (un->i.ViMtype));
			memmove(un->i.ViVsn, un->vsn, sizeof (un->i.ViVsn));
			if (*un->i.ViMtype != '\0')  un->i.ViFlags |= VI_mtype;
			if (*un->i.ViVsn != '\0')  un->i.ViFlags |= VI_vsn;
			memset(un->vsn, ' ', 31);
			if (ce->CeStatus & CES_inuse) {
				(void) CatalogVolumeUnloaded(&un->i, "");
				(void) CatalogExport(&un->i);
			}
		}

		/*
		 * If the device has media, lock the door.
		 */
		else if (un->status.b.ready) {
			TAPEALERT(fd, un);
			if (scsi_cmd(fd, un, SCMD_DOORLOCK, 0, LOCK) != 0) {
				DevLog(DL_ERR(1026));
			}
			TAPEALERT(fd, un);
		}
	}

	mutex_unlock(&un->io_mutex);
	mutex_unlock(&un->mutex);
}
