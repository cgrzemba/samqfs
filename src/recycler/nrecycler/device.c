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

#pragma ident "$Revision: 1.7 $"

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
#include "aml/catlib.h"

/* Local headers. */
/* #include "recycler.h" */

#if defined(lint)
#include "sam/lint.h"
#undef shmdt
#endif /* defined(lint) */

extern shm_alloc_t master_shm;
extern shm_ptr_tbl_t *shm_ptr_tbl;

/* Device table. */
static struct {
	dev_ent_t	*head;
} deviceTable = { NULL };

static int shmInit(void);

/*
 * Initialize device table.  The recycler will exit if there are fatal
 * errors in devicie initialization.
 */
void
DeviceInit(void)
{
	int high_eq;
	int	entries;

	if (shmInit() != 0) {
		/* SAM-FS shared memory initialization failed */
		SendCustMsg(HERE, 20410);
		exit(EXIT_FATAL);
	}

	deviceTable.head = NULL;
	if (shm_ptr_tbl != NULL && shm_ptr_tbl->valid) {
		deviceTable.head =
		    (struct dev_ent *)SHM_REF_ADDR(shm_ptr_tbl->first_dev);
	}

	if (shm_ptr_tbl == NULL || deviceTable.head == NULL) {
		struct dev_ent *dev;
		struct dev_ent *next;

		entries = read_mcf(NULL, &deviceTable.head, &high_eq);
		if (entries < 0) {
			SendCustMsg(HERE, 19005);
			exit(EXIT_FATAL);
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
}

dev_ent_t *
DeviceGetHead(void)
{
	return (deviceTable.head);
}

char *
DeviceGetFamilyName(
	dev_ent_t *dev)
{
	char *rval;

	rval = "(NONE)";

	if (dev == NULL) {
		return (rval);
	}

	if (*(dev->set) != '\0') {
		rval = dev->set;
	} else if (dev->equ_type == DT_HISTORIAN) {
		rval = "hy";
	}
	return (rval);
}

void
DeviceGetMediaType(
	dev_ent_t *dev,
	mtype_t *mtype)
{
	if ((dev->media & DT_CLASS_MASK) == DT_TAPE) {
		(void) strcpy(*mtype, "tp");
	} else {
		(void) strcpy(*mtype, "od");
	}
}

equ_t
DeviceGetEq(
	dev_ent_t *dev)
{
	return (dev->eq);
}

char *
DeviceGetTypeMnemonic(
	dev_ent_t *dev)
{
	return (device_to_nm(dev->media));
}

/*
 * Initialize SAM-FS shared memory access.
 */
static int
shmInit(void)
{
	/*
	 * Access SAM-FS shared memory segment.
	 */
	if ((master_shm.shmid = shmget(SHM_MASTER_KEY, 0, 0)) < 0) {
		/*
		 * If disk archive recycling the shared memory
		 * segment is not needed.
		 */
		return (0);
	}

	master_shm.shared_memory = shmat(master_shm.shmid, NULL, 0);
	if (master_shm.shared_memory == (void *)-1) {
		Trace(TR_MISC, "Error: unable to attach master shm segment");
		return (-1);
	}

	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
	if (strcmp(shm_ptr_tbl->shm_block.segment_name,
	    SAM_SEGMENT_NAME) != 0) {
		Trace(TR_MISC, "Error: master shared memory name incorrect");
		return (-1);
	}

	if (CatalogInit(program_name) == -1) {
		Trace(TR_MISC, "Catalog initialization failed");
	}

	return (0);
}
