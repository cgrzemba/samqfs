/*
 * robots.c - library code for robot interface
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

#pragma ident "$Revision: 1.16 $"


#include <string.h>

#include "aml/device.h"
#include "aml/shm.h"
#include "aml/message.h"

extern shm_alloc_t master_shm, preview_shm;

/*
 * send a fsumount message to every robot
 */

void
signal_fsumount(equ_t fseq)
{
	dev_ent_t *device = NULL;
	message_request_t *message;

	device = (dev_ent_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);

	for (; device != NULL; device =
	    (dev_ent_t *)SHM_REF_ADDR(device->next)) {

		if (IS_ROBOT(device) && (device->state < DEV_IDLE) &&
		    (device->status.b.ready && device->status.b.present)) {
			message = (message_request_t *)
			    SHM_REF_ADDR(device->dt.rb.message);
			mutex_lock(&message->mutex);
			while (message->mtype != MESS_MT_VOID)
				cond_wait(&message->cond_i, &message->mutex);
			(void) memset(&message->message, 0,
			    sizeof (sam_message_t));
			message->message.magic = (uint_t)MESSAGE_MAGIC;
			message->message.command = MESS_CMD_FSUMOUNT;
			message->mtype = MESS_MT_NORMAL;
			message->message.param.umount_request.fseq = fseq;
			cond_signal(&message->cond_r);	/* wake up the robot */
			mutex_unlock(&message->mutex);
		}
	}
}
