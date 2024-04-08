/*
 * sam_load.c - load removable media API
 *
 *	sam_load() causes the specified VSN's removable media to be loaded into
 *	the specified device
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
 * or https://illumos.org/license/CDDL.
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

#pragma ident "$Revision: 1.24 $"


#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stropts.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/shm.h"
#include "sam/defaults.h"
#include "aml/device.h"
#include "aml/fifo.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/samapi.h"
#include "aml/sam_utils.h"
#include "pub/sam_errno.h"

/*
 *	sam_load() - API function to load media
 *
 *	Input parameters --
 *		eq_ord		Equipment ordinal of device to be loaded
 *				if load unavail request,
 *				else, library eq ordinal
 *		vsn		Volume serial number to be loaded
 *		media		Type of media with VSN
 *		ea		Element address in robot to be changed
 *				or ROBOT_NO_SLOT
 *		modifier	Side of optical or partition of D2 tape
 *		wait_response	Wait for command response if nonzero
 *				-1	Wait forever
 *				> zero	Wait this many seconds for response
 *
 *	Output parameters --
 *		errno		Set to error number if error encountered
 *
 *	Return value --
 *		0		If successful in exporting the media
 *		-1		If error encountered,
 *				'errno' set to error number
 *
 */

int
sam_load(
	ushort_t eq_ord,	/* Equipment ordinal */
	char *vsn,		/* New Volume Serial Number */
	char *media,		/* Type of media with VSN */
	int ea,			/* Element Address */
	int modifier,		/* Side of optical or partition of D2 tape */
	int wait_response)	/* Nonzero value to wait for response */
{
	char *fifo_path;	/* Path to FIFO pipe, from shared memory */
	int forever = FALSE;	/* Set TRUE if wait_response is -1 */
	int wait = wait_response;
				/* Time to wait for I/O completion */
	dev_ent_t *device;
	dev_ptr_tbl_t *dev_tbl;
	operator_t operator;	/* Operator data */
	shm_alloc_t master_shm;	/* Master shared memory structure */
	sam_cmd_fifo_t	cmd_block;
	struct CatalogEntry ced;
	struct CatalogEntry *ce;

	/*
	 *	Verify input variables have legitimate settings
	 */
	if ((ROBOT_NO_SLOT == ea) && ((char *)NULL == vsn)) {
		errno = ER_SLOT_OR_VSN_REQUIRED;
		return (-1);
	}

	/*
	 *	If wait for response equals -1,
	 *	set flag to indicate wait forever
	 */

	if (-1 == wait_response) {
		forever = TRUE;
	}

	/*
	 *	Get device entry and FIFO path from master shared memory segment
	 */

	if ((master_shm.shmid = shmget(SHM_MASTER_KEY, 0, 0)) < 0) {
		errno = ER_NO_MASTER_SHM;
		return (-1);
	}

	if ((master_shm.shared_memory = (shm_ptr_tbl_t *)
	    shmat(master_shm.shmid, 0, SHM_RDONLY)) == (shm_ptr_tbl_t *)-1) {
		errno = ER_NO_MASTER_SHM_ATT;
		return (-1);
	}

	fifo_path = (char *)SHM_REF_ADDR(((shm_ptr_tbl_t *)
	    master_shm.shared_memory)->fifo_path);
	dev_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(((shm_ptr_tbl_t *)
	    master_shm.shared_memory)->dev_table);

	SET_SAM_OPER_LEVEL(operator);

	/*
	 *	Check that this is a valid equipment ordinal
	 */

	if ((0 == (int)eq_ord) || ((int)eq_ord > dev_tbl->max_devices) ||
	    ((dev_ent_t *)NULL == dev_tbl->d_ent[eq_ord])) {
		shmdt((char *)master_shm.shared_memory);
		errno = ER_NO_EQUIP_ORDINAL;
		return (-1);
	}

	device = (dev_ent_t *)SHM_REF_ADDR(dev_tbl->d_ent[eq_ord]);

	/*
	 *	Check that this is a valid device
	 */

	if (!(IS_ROBOT(device) || IS_OPTICAL(device) || IS_TAPE(device))) {
		shmdt((char *)master_shm.shared_memory);
		errno = ER_NOT_REMOV_MEDIA_DEVICE;
		return (-1);
	}

	/*
	 *	Try to get catalog entry first by media type and vsn.
	 * 	If unsuccessful, use eq:slot but remember eq_ord could
	 * 	be ordinal of unavailed drive.
	 */
	ce = CatalogGetCeByMedia(media, vsn, &ced);

	if (ce == NULL) {
		if (IS_ROBOT(device)) {
			ce = CatalogGetCeByLoc(eq_ord, ea, modifier, &ced);
		} else {
			ce = CatalogGetCeByLoc(device->fseq,
			    ea, modifier, &ced);
		}
	}

	if (ce == NULL) {
		errno = ER_SLOT_OR_VSN_REQUIRED;

		return (-1);
	}

	/*
	 *	Clear command block area
	 */

	memset(&cmd_block, 0, sizeof (cmd_block));

	/*
	 *	If robot device, check different status
	 */

	if (IS_ROBOT(device)) {
		/*
		 *	Check if operator has privilege to load media
		 */

		if (!SAM_OPER_SLOT(operator)) {
			shmdt((char *)master_shm.shared_memory);
			errno = ER_OPERATOR_NOT_PRIV;
			return (-1);
		}

		if (device->state > DEV_IDLE) {
			shmdt((char *)master_shm.shared_memory);
			errno = ER_DEVICE_OFF_OR_DOWN;
			return (-1);
		}

		cmd_block.cmd = CMD_FIFO_MOUNT_S;
	}
	else
	{
		if ((device->fseq == eq_ord) || (device->fseq == 0)) {
			shmdt((char *)master_shm.shared_memory);
			errno = ER_ROBOT_DEVICE_REQUIRED;
			return (-1);
		}

		if (device->state != DEV_UNAVAIL) {
			shmdt((char *)master_shm.shared_memory);
			errno = ER_DEVICE_NOT_UNAVAILABLE;
			return (-1);
		}

		/*
		 * Make sure this volume is in this drive's library.
		 */
		if (device->fseq != ce->CeEq) {
			errno = ER_VSN_NOT_FOUND_IN_ROBOT;
			return (-1);
		}

		cmd_block.cmd = CMD_FIFO_LOAD_U;
	}


	/*
	 *	If tape device, check that device is available
	 */

	if (IS_TAPE(device)) {
		int open_fd;

		if (((open_fd = open(device->name, O_RDONLY)) < 0) &&
		    (errno == EBUSY)) {
			if (forever || (wait > 0)) {
				while (forever || (wait-- > 0)) {
				/* N.B. bad indentation here to meet */
				/* cstyle requirements */
				if (((open_fd =
				    open(device->name, O_RDONLY)) < 0) &&
				    (errno == EBUSY)) {
					sleep(1);
				}
				else
				{
						break;
				}
				}

				if (!(forever || (wait >= 0))) {
					shmdt((char *)master_shm.shared_memory);
					errno = EBUSY;
					return (-1);
				}
			}
			else
			{
				shmdt((char *)master_shm.shared_memory);
				errno = ER_DEVICE_USE_BY_ANOTHER;
				return (-1);
			}
		}

		if (open_fd >= 0) {
			struct  mtop  tape_op;

			tape_op.mt_op = MTOFFL;
			tape_op.mt_count = 0;
			(void) ioctl(open_fd, MTIOCTOP, &tape_op);
			close(open_fd);
		}
	}

	cmd_block.eq = eq_ord;
	cmd_block.media = sam_atomedia(ce->CeMtype);
	cmd_block.slot = ce->CeSlot;
	cmd_block.part = ce->CePart;

	if (!(ce->CeStatus & CES_inuse) && !(ce->CeStatus & CES_occupied)) {
		shmdt((char *)master_shm.shared_memory);
		errno = ER_SLOT_NOT_OCCUPIED;
		return (-1);
	}

	if (ce->CeStatus & CES_cleaning) {
		shmdt((char *)master_shm.shared_memory);
		errno = ER_SLOT_IS_CLEAN_CARTRIDGE;
		return (-1);
	}

	cmd_block.magic = CMD_FIFO_MAGIC;

	if (vsn != (char *)NULL) {
		strncpy(cmd_block.vsn, vsn, sizeof (vsn_t)-1);
		cmd_block.media = sam_atomedia(ce->CeMtype);
	}

	if (sam_send_cmd(&cmd_block, wait_response, fifo_path) < 0) {
		shmdt((char *)master_shm.shared_memory);
		return (-1);
	}

	shmdt((char *)master_shm.shared_memory);
	return (0);
}
