/*
 * build_pids.c - build the pids area for robot shephard.
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

#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/shm.h"
#include "aml/device.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "robots.h"

#pragma ident "$Revision: 1.19 $"

/* some globals */

extern shm_alloc_t master_shm, preview_shm;

int
build_pids(shm_ptr_tbl_t *ptr_tbl)
{
	int    i, j, count, stk_found = 0;
	char   *ent_pnt = "build_pids";
	robots_t	*robot;
	dev_ent_t	*device_entry;
	rob_child_pids_t  *pid;

	device_entry = (dev_ent_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);

	/* Find number of devices that need drivers. */
	for (count = 0; device_entry != NULL;
	    device_entry = (dev_ent_t *)SHM_REF_ADDR(device_entry->next)) {
		if (IS_ROBOT(device_entry) ||
		    device_entry->type == DT_PSEUDO_SC ||
		    device_entry->type == DT_PSEUDO_SS) {
			count++;
			/* stks need the ssi running */
			if (!stk_found && device_entry->type == DT_STKAPI) {
				stk_found = TRUE;
				count++;
			}
		}
	}

	if (count) {
		count++;
		pids = (rob_child_pids_t *)
		    malloc_wait((count * sizeof (rob_child_pids_t)), 2, 0);
		memset(pids, 0, (count * sizeof (rob_child_pids_t)));
		device_entry = (dev_ent_t *)SHM_REF_ADDR(
		    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);

		pid = pids;	/* the pseudo device is first pid */
		if (stk_found)	/* need to have an ssi running */
			for (j = 0, robot = robots; j < NUMBER_OF_ROBOT_TYPES;
			    j++, robot++)
				if (robot->type == DT_PSEUDO_SSI) {
					pid->oldstate = DEV_ON;
					pid->who = robot;
					pid->eq = 0;
					pid->device = NULL;
					pid++;
					break;
				}

		for (; device_entry != NULL; device_entry = (dev_ent_t *)
		    SHM_REF_ADDR(device_entry->next))
			if (IS_ROBOT(device_entry) ||
			    device_entry->type == DT_PSEUDO_SC ||
			    device_entry->type == DT_PSEUDO_SS) {
				pid->who = NULL;
				/* Is it driven by generic scsi II driver */
				if ((device_entry->type & DT_SCSI_R) ==
				    DT_SCSI_R) {
					pid->oldstate = device_entry->state;
					pid->who = &generic_robot;
					pid->eq = device_entry->eq;
					pid->device = device_entry;
				} else {
					for (j = 0, robot = robots;
					    j < NUMBER_OF_ROBOT_TYPES;
					    j++, robot++)
						if (robot->type ==
						    device_entry->type) {
							pid->oldstate =
							    device_entry->state;
							pid->who = robot;
							pid->eq =
							    device_entry->eq;
							pid->device =
							    device_entry;
							break;
						}
				}
				if (pid->who == NULL && DBG_LVL(SAM_DBG_DEBUG))
					/*
					 * To get this far with an unknown
					 * robot type
					 * means things failed during shared
					 * memory creation.
					 */
					if (device_entry->type !=
					    DT_HISTORIAN) {
						sam_syslog(LOG_DEBUG,
						    "%s:(%d): unknown"
						    " robot type(%d).", ent_pnt,
						    device_entry->eq,
						    device_entry->type);
					}
				pid++;
			}
	} else {
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG, "No robotic devices found.");
		return (0);
	}
	return (count - 1);
}
