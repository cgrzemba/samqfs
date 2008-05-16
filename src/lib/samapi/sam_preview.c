/*
 * sam_preview.c - preview priority controls API(internal)
 *
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

#pragma ident "$Revision: 1.10 $"

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>

/* Solaris headers. */
#include <sys/shm.h>
#include <sys/mman.h>

/* SAM-FS headers. */
#include "pub/sam_errno.h"
#include "sam/types.h"
#include "sam/param.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "sam/custmsg.h"
#include "aml/device.h"
#include "sam/exit.h"
#include "aml/fifo.h"
#include "sam/lib.h"
#include "aml/preview.h"
#include "aml/proto.h"
#include "aml/samapi.h"
#include "aml/sam_utils.h"
#include "aml/shm.h"

/* Private functions. */

/* Private data. */

#ifdef MAIN
/*
 *  test program.
 */
int
main(int argc, char **argv)
{
	uint_t		pid;
	float		newpri;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s pid priority", argv[0]);
		exit(-70);
	}

	pid	= strtol(argv[1], NULL, 0);
	newpri = (float)strtod(Argv[2], NULL);

	SamSetPreviewPri(pid, newpri);
}
#endif

/*
 * SamSetPreviewPri - Set the preview priority for a process' load request.
 * Returns 0 if success, -1 if failure
 *
 */
int
SamSetPreviewPri(uint_t	pid, float newpri)
{
	shm_alloc_t preview_shm;	/* Mount preview table */
	preview_tbl_t *Preview_Tbl;	/* Preview table */
	preview_t *p;
	int avail;
	int count;
	int i;

	/*
	 * Access preview display shared memory segment.
	 */
	preview_shm.shmid = shmget(SHM_PREVIEW_KEY, 0, 0);
	preview_shm.shared_memory =
	    shmat(preview_shm.shmid, NULL, NULL); /* r/w */
	if (preview_shm.shared_memory == (void *)-1) {
		return (-1);
	}

	Preview_Tbl =
	    &((shm_preview_tbl_t *)preview_shm.shared_memory)->preview_table;

	avail = Preview_Tbl->avail;
	count = Preview_Tbl->ptbl_count;

	for (i = 0; i < avail && count != 0; i++) {
		p = &Preview_Tbl->p[i];
		if (!p->in_use)  continue;
		if (pid == p->handle.pid) {
			p->priority = newpri;
			break;
		}
		count--;
	}
	shmdt(preview_shm.shared_memory);
	if (count == 0) {
		return (-1);
	}
	return (0);
}
