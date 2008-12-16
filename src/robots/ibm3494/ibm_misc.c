/*
 * ibm_misc.c - misc routines for the ibm
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

#pragma ident "$Revision: 1.17 $"

#include <string.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/shm.h"
#include "aml/historian.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "ibm3494.h"

/* some globals */
extern shm_alloc_t master_shm, preview_shm;


/*
 * is_storage - is the specified element a storage address
 *
 */
int
is_storage(
	library_t *library,
	const uint_t element)
{
	return (TRUE);
}


/*
 * slot_number - given the element address, return the slot number
 *
 */
uint_t
slot_number(
	library_t *library,
	const uint_t element)
{
	return (element);
}


/*
 * element_address - given the slot number, return the element address
 *
 */
uint_t
element_address(
	library_t *library,
	const uint_t slot)
{
	return (slot);
}


/*
 * schedule_export - put an export request in the libraries message
 * queue.
 */
void
schedule_export(
	library_t *library,
	uint_t slot)
{
	register dev_ent_t  		*un = library->un;
	register message_request_t 	*message;
	register export_request_t 	*request;

	message = (message_request_t *)SHM_REF_ADDR(un->dt.rb.message);
	request = &message->message.param.export_request;

	mutex_lock(&message->mutex);

	while (message->mtype != MESS_MT_VOID)
		cond_wait(&message->cond_i, &message->mutex);

	(void) memset(&message->message, 0, sizeof (sam_message_t));
	/*LINTED constant truncated by assignment */
	message->message.magic = MESSAGE_MAGIC;
	message->message.command = MESS_CMD_EXPORT;
	request->eq = un->eq;
	request->flags = EXPORT_BY_SLOT;
	request->slot = slot;
	message->mtype = MESS_MT_NORMAL;
	cond_signal(&message->cond_r);

	mutex_unlock(&message->mutex);
}
