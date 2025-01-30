/*
 * device.c - provide access to samfs dev_t entries in shared memory
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

#pragma ident "$Revision: 1.25 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/shm.h>

/* Solaris headers. */

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/shm.h"
#include "sam/nl_samfs.h"
#include "sam/exit.h"
#include "sam/custmsg.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "sam/lib.h"
#include "aml/stager_defs.h"
#if defined(lint)
#include "sam/lint.h"
#undef shmdt
#endif /* defined(lint) */

/* Local headers. */
#include "stager_lib.h"
#include "rmedia.h"

extern shm_alloc_t master_shm;
extern shm_ptr_tbl_t *shm_ptr_tbl;

static boolean_t checkDevices();

/* Device table. */
static struct {
	int		entries;
	dev_ent_t	*head;
} deviceTable = { 0, NULL };

/*
 * Initialize device table.
 */
int
InitDevices(void)
{
	int high_eq;

	sleep(5);
	if (shm_ptr_tbl == NULL) {
		(void) ShmatSamfs(O_RDONLY);
	}

	deviceTable.head = NULL;
	if (shm_ptr_tbl != NULL && shm_ptr_tbl->valid) {
		deviceTable.head =
		    (struct dev_ent *)SHM_REF_ADDR(shm_ptr_tbl->first_dev);
	}

	if (shm_ptr_tbl == NULL || deviceTable.head == NULL) {
		struct dev_ent *dev;
		struct dev_ent *next;

		deviceTable.entries = read_mcf(NULL, &deviceTable.head,
		    &high_eq);
		if (deviceTable.entries < 0) {
			SendCustMsg(HERE, 19005);
			return (EXIT_NORESTART);
		}

		for (dev = deviceTable.head; dev != NULL; dev = next) {
			next = dev->next;
			dev->next =
			    (struct dev_ent *)(void *)SHM_GET_OFFS(next);
		}
		Trace(TR_MISC, "Device table (mcf) initialized");
	} else {
		Trace(TR_MISC, "Device table (shared memory) initialized");
	}

	return (0);
}

/*
 * Get device table.  Returns pointer to head of dev_t entries.
 */
dev_ent_t *
GetDevices(
	boolean_t check)
{
	if (check && checkDevices()) {
		Trace(TR_MISC, "Devices changed");
		(void) InitDevices();
		(void) InitMedia();
	}
	return (deviceTable.head);
}


/*
 * Check availability of devices.
 */
static boolean_t
checkDevices(void)
{
	boolean_t changed = B_FALSE;

	if (shm_ptr_tbl != NULL && shm_ptr_tbl->valid == 0) {
		Trace(TR_DEBUG, "Detach shared memory");
		/*
		 * Detach shared memory.
		 */
		if (shmdt(master_shm.shared_memory) == -1) {
			WarnSyscallError(HERE, "shmdt", "");
		}
		master_shm.shared_memory = NULL;
		shm_ptr_tbl = NULL;

		changed = B_TRUE;

	} else {

		if (shm_ptr_tbl == NULL) {
			(void) ShmatSamfs(O_RDONLY);
			if (shm_ptr_tbl != NULL) {
				if (shm_ptr_tbl->valid &&
				    shm_ptr_tbl->first_dev != 0) {
					changed = B_TRUE;
				} else {
					int rv;

					rv = shmdt(master_shm.shared_memory);
					if (rv == -1) {
						WarnSyscallError(HERE, "shmdt",
						    "");
					}
					master_shm.shared_memory = NULL;
					shm_ptr_tbl = NULL;
				}
			}
		}
	}
	return (changed);
}
