/*
 * devstat.c - Get information about a SAMFS device.
 *
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

#pragma ident "$Revision: 1.14 $"


#include <sys/types.h>
#include <sys/shm.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "aml/device.h"
#include "pub/devstat.h"
#include "aml/shm.h"

int
sam_devstat(
	    ushort_t eq,
	    struct sam_devstat *buf,
	    size_t bufsize)
{
	struct dev_ent *dev;
	dev_ptr_tbl_t  *dev_tbl;
	shm_alloc_t	master_shm;
	shm_ptr_tbl_t  *shm_ptr_tbl;
	struct sam_devstat db;

	if (bufsize < sizeof (db)) {
		errno = EINVAL;
		return (-1);
	}

	/*
	 * Access SAM-FS shared memory segment.
	 */
	if ((master_shm.shmid = shmget(SHM_MASTER_KEY, 0, 0)) < 0) {
#ifdef DEBUG
		fprintf(stderr,
		    "Unable to find master shared memory segment.\n");
#endif
		return (-1);
	}
	if ((master_shm.shared_memory =
	    shmat(master_shm.shmid, NULL, 0)) == (void *)-1) {
#ifdef DEBUG
		fprintf(stderr,
		    "Unable to attach master shared memory segment.\n");
#endif
		return (-1);
	}
	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
	if (strcmp(shm_ptr_tbl->shm_block.segment_name,
	    SAM_SEGMENT_NAME) != 0) {
#ifdef DEBUG
		fprintf(stderr,
		    "Master shared memory segment name incorrect.\n");
#endif
		shmdt((char *)master_shm.shared_memory);
		errno = ENOENT;
		return (-1);
	}
	dev_tbl = (dev_ptr_tbl_t *)
	    SHM_REF_ADDR(shm_ptr_tbl->dev_table);
	if ((int)eq > dev_tbl->max_devices) {
		shmdt((char *)master_shm.shared_memory);
		errno = ENODEV;
		return (-1);
	}
	dev = (dev_ent_t *)SHM_REF_ADDR(dev_tbl->d_ent[eq]);
	if (dev == NULL) {
		shmdt((char *)master_shm.shared_memory);
		errno = ENODEV;
		return (-1);
	}
	if (strlen(dev->name) >= 32) {
		errno = E2BIG;
		return (-1);
	}
	db.type = (ushort_t)dev->type;
	db.space = dev->space;
	db.capacity = dev->capacity;
	db.state = dev->state;
	db.status = dev->status.bits;
	strcpy(db.name, dev->name);
	strcpy(db.vsn, dev->vsn);
	(void) memcpy((void *)buf, (void *)&db, bufsize);
	shmdt((char *)master_shm.shared_memory);
	return (0);
}

int
sam_ndevstat(
	    ushort_t eq,
	    struct sam_ndevstat *buf,
	    size_t bufsize)
{
	struct dev_ent *dev;
	dev_ptr_tbl_t  *dev_tbl;
	shm_alloc_t	master_shm;
	shm_ptr_tbl_t  *shm_ptr_tbl;
	struct sam_ndevstat db;

	if (bufsize < sizeof (db)) {
		errno = EINVAL;
		return (-1);
	}

	/*
	 * Access SAM-FS shared memory segment.
	 */
	if ((master_shm.shmid = shmget(SHM_MASTER_KEY, 0, 0)) < 0) {
#ifdef DEBUG
		fprintf(stderr,
		    "Unable to find master shared memory segment.\n");
#endif
		return (-1);
	}
	if ((master_shm.shared_memory =
	    shmat(master_shm.shmid, NULL, 0)) == (void *)-1) {
#ifdef DEBUG
		fprintf(stderr,
		    "Unable to attach master shared memory segment.\n");
#endif
		return (-1);
	}
	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
	if (strcmp(shm_ptr_tbl->shm_block.segment_name,
	    SAM_SEGMENT_NAME) != 0) {
#ifdef DEBUG
		fprintf(stderr,
		    "Master shared memory segment name incorrect.\n");
#endif
		shmdt((char *)master_shm.shared_memory);
		errno = ENOENT;
		return (-1);
	}
	dev_tbl = (dev_ptr_tbl_t *)
	    SHM_REF_ADDR(shm_ptr_tbl->dev_table);
	if ((int)eq > dev_tbl->max_devices) {
		shmdt((char *)master_shm.shared_memory);
		errno = ENODEV;
		return (-1);
	}
	dev = (dev_ent_t *)SHM_REF_ADDR(dev_tbl->d_ent[eq]);
	if (dev == NULL) {
		shmdt((char *)master_shm.shared_memory);
		errno = ENODEV;
		return (-1);
	}
	db.type = (ushort_t)dev->type;
	db.space = dev->space;
	db.capacity = dev->capacity;
	db.state = dev->state;
	db.status = dev->status.bits;
	strcpy(db.name, dev->name);
	strcpy(db.vsn, dev->vsn);
	(void) memcpy((void *)buf, (void *)&db, bufsize);
	shmdt((char *)master_shm.shared_memory);
	return (0);
}
