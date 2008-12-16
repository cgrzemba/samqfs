/*
 * sam_live API
 *  - provides live access to the samfs data(devices, catalogs, and mount
 *    requests)
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

#pragma ident "$Revision: 1.20 $"



#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "aml/shm.h"
#include "sam/types.h"
#include "aml/device.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/samapi.h"
#include "pub/sam_errno.h"

/*
 * TODO:
 * - need to do mount request code(see src/gui/preview/preview.c for code)
 * - need to create public template(code uses internal structures still)
 * - need to add more error checking, clean up messages
 * - need to package as APIs(man pages, header file, etc)
 * - need to provide code example
 * - need internationalize?
 */

/*	Private functions */

/*	Open an entry in device segment list */
static int open_dev_segment();

/*	Open an entry in mount request segment list */
static int open_mntreq_segment();

#define	MEM_OFFSET(x) (((x) == NULL? NULL : ((char *)mem->address + (int)x)))

typedef struct {
	int handle;
	char *address;
} memory_segment_t;

#define	MAX_DEV_SEGMENTS	10
static memory_segment_t dev_segments[MAX_DEV_SEGMENTS] = {
	-1, NULL, -1, NULL, -1, NULL, -1, NULL, -1, NULL, -1,
	NULL, -1, NULL, -1, NULL, -1, NULL, -1, NULL
};

#define	MAX_MNTREQ_SEGMENTS	10
static memory_segment_t mntreq_segments[MAX_MNTREQ_SEGMENTS] = {
	-1, NULL, -1, NULL, -1, NULL, -1, NULL, -1, NULL, -1,
	NULL, -1, NULL, -1, NULL, -1, NULL, -1, NULL
};

static int open_dev_segment()
{
	int i;

	for (i = 0; i < MAX_DEV_SEGMENTS; i++) {
		if (dev_segments[i].handle < 0)
			return (i);
	}

	errno = ER_NO_LIVE_DEVICE_SEGS;
	return (-1);
}


/*
 *	Live SAMFS data API functions
 */
int
samlive_devhandle(int *max_count)
{
	shm_ptr_tbl_t   *shm_ptr_tbl;   /* SM device pointer table */
	dev_ent_t	*p;		/* Device table entry  */
	dev_ent_t	*Dev_Head;
	memory_segment_t *mem;
	int cnt = 0;
	int handle;


	if ((handle = open_dev_segment()) < 0)
		return (handle);

	mem = &dev_segments[handle];

	/* access master/device shared memory segment */
	mem->handle = shmget(SHM_MASTER_KEY, 0, 0);

	if (mem->handle < 0) {
		errno = ER_NO_MASTER_SHM;
		return (-1);
	}

	mem->address = shmat(mem->handle, (void *)NULL, SHM_RDONLY);

	if (mem->address == (void *)-1) {
		errno = ER_NO_MASTER_SHM_ATT;
		return (-1);
	}

	/* LINTED pointer cast may result in improper alignment */
	shm_ptr_tbl = (shm_ptr_tbl_t *)mem->address;

	/* LINTED pointer cast may result in improper alignment */
	Dev_Head = (dev_ent_t *)MEM_OFFSET(shm_ptr_tbl->first_dev);

	/* parse dev entries */
	/* LINTED pointer cast may result in improper alignment */
	for (p = Dev_Head; p; p = (dev_ent_t *)MEM_OFFSET(p->next)) {
#ifdef SAMDEBUG
		if (IS_DISK(p))
			printf("%d (%d) Disk: %s\n", p->eq, p->fseq, p->name);
		else if (IS_OPTICAL(p))
			printf("%d (%d) Optical: %s\n",
			    p->eq, p->fseq, p->name);
		else if (IS_TAPE(p))
			printf("%d (%d) Tape: %s\n", p->eq, p->fseq, p->name);
		else if (IS_ROBOT(p))
			printf("%d (%d) Robot: %s\n", p->eq, p->fseq, p->name);
		else if (IS_FS(p))
			printf("%d (%d) FS: %s\n", p->eq, p->fseq, p->name);
#endif /* SAMDEBUG */
		cnt++;
	}

	*max_count = cnt;
	return (handle);
}



int
samlive_devlist(int handle, dev_ent_t **dev_ptr, int count)
{
	shm_ptr_tbl_t	*shm_ptr_tbl;   /* SM device pointer table */
	dev_ent_t	*p;		/* Device table entry */
	dev_ent_t	*Dev_Head;
	memory_segment_t *mem;
	int				i = 0;

	if ((handle < 0) || (dev_segments[handle].handle < 0)) {
		errno = ER_NO_LIVE_DEVICE_HANDLE;
		return (-1);
	}

	mem = &dev_segments[handle];

	/* LINTED pointer cast may result in improper alignment */
	shm_ptr_tbl = (shm_ptr_tbl_t *)mem->address;

	/* LINTED pointer cast may result in improper alignment */
	Dev_Head = (dev_ent_t *)MEM_OFFSET(shm_ptr_tbl->first_dev);

	/* parse dev entries */
	for (p = Dev_Head; p && (i < count);
		/* LINTED pointer cast may result in improper alignment */
	    p = (dev_ent_t *)MEM_OFFSET(p->next)) {
		dev_ptr[i++] = p;
	}

	return (i);
}



int
samlive_devsystemmsgs(int handle, char **msgs)
{
	memory_segment_t *mem;
	shm_ptr_tbl_t   *shm_ptr_tbl;   /* SM device pointer table */
	int i;

	if (handle < 0 || dev_segments[handle].handle < 0) {
		errno = ER_NO_LIVE_DEVICE_HANDLE;
		return (-1);
	}

	mem = &dev_segments[handle];

	/* LINTED pointer cast may result in improper alignment */
	shm_ptr_tbl = (shm_ptr_tbl_t *)mem->address;

	for (i = 0; i < DIS_MES_TYPS; i++) {
		msgs[i] = shm_ptr_tbl->dis_mes[i];
	}

	return (i);
}

int
samlive_cathandle(const equ_t equ, int *max_count)
{
	int Catalog_Length;
	struct CatalogEntry *list;

	list = CatalogGetEntriesByLibrary(equ, &Catalog_Length);
	free(list);

	*max_count = Catalog_Length;
	return (0);
}


int
/* LINTED argument unused in function */
samlive_catlist(int handle, int *ilist, equ_t equ, int count)
{
	int		i, n_entries;
	struct CatalogEntry *list;

	if ((handle < 0)) {
		errno = ER_NO_LIVE_CATALOG_HANDLE;
		return (-1);
	}

	list = CatalogGetEntriesByLibrary(equ, &n_entries);

	for (i = 0; i < n_entries; i++) {
		struct CatalogEntry *ce;

		ce = &list[i];
		ilist[i] = ce->CeMid;
	}

	free(list);

	return (n_entries);
}


int
samlive_mntreqhandle(int *max_count, int **active_count)
{
	preview_tbl_t *Preview_Tbl; /* SM preview table			 */
	memory_segment_t *mem;
	int handle;

	if ((handle = open_mntreq_segment()) < 0)
		return (handle);

	mem = &mntreq_segments[handle];

	/* access master/device shared memory segment */
	mem->handle = shmget(SHM_PREVIEW_KEY, 0, 0);

	if (mem->handle < 0) {
#ifdef	SAMDEBUG
		perror("Unable to find mount request shared memory segment");
#endif	/* SAMDEBUG */

		errno = ER_NO_PREVIEW_SHM;
		return (-1);
	}

	mem->address = shmat(mem->handle, (void *)NULL, SHM_RDONLY);

	if (mem->address == (void *)-1) {
#ifdef	SAMDEBUG
		perror("Unable to attach mount request shared memory segment");
#endif	/* SAMDEBUG */

		errno = ER_NO_PREVIEW_SHM_ATT;
		return (-1);
	}

	/* LINTED pointer cast may result in improper alignment */
	Preview_Tbl = &((shm_preview_tbl_t *)mem->address)->preview_table;

	*active_count = &Preview_Tbl->ptbl_count;
	*max_count = Preview_Tbl->avail;
	return (handle);
}


int
samlive_mntreqlist(int handle, preview_t **mntreq_ptr, int count)
{
	preview_tbl_t *Preview_Tbl; /* SM preview table */
	memory_segment_t *mem;
	int i = 0;

	if ((handle < 0) || (mntreq_segments[handle].handle < 0)) {
		errno = ER_NO_LIVE_MNTREQ_HANDLE;
		return (-1);
	}

	mem = &mntreq_segments[handle];

	/* LINTED pointer cast may result in improper alignment */
	Preview_Tbl = &((shm_preview_tbl_t *)mem->address)->preview_table;

	for (i = 0; (i < Preview_Tbl->avail) && (i < count); i++) {
		mntreq_ptr[i] = &Preview_Tbl->p[i];
	}

	return (i);
}


static int open_mntreq_segment()
{
	int i;

	for (i = 0; i < MAX_MNTREQ_SEGMENTS; i++) {
		if (mntreq_segments[i].handle < 0) {
			return (i);
		}
	}

	errno = ER_NO_LIVE_MNTREQ_SEGS;
	return (-1);
}
