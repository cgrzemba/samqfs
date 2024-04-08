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
/*
 *	Shared memory common routines
 *
 */


#pragma ident "$Revision: 1.14 $"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include "aml/shm.h"
#include "pub/sam_errno.h"

/* External global variables declared */

extern shm_alloc_t master_shm, preview_shm;

/*
 *
 * sam_mastershm_attach()
 *
 * common library routine for commands to find, then attach the
 * SAM-FS master shared memory segment.
 *
 * Calling parameters are the permission flags for the shmget()
 * and the shmat() calls.
 *
 * Return value is the pointer to the shared memory segment if
 * successful, and -1 if error.
 *
 */
void *sam_mastershm_attach(
	int shmget_priv,
	int shmat_priv)
{

	int shmid;			/* Shared memory identifier */
	void *shared_memory;		/* Shared memory address */

	shmid = shmget((key_t)SHM_MASTER_KEY, (size_t)0, shmget_priv);

	if (shmid < 0) {
		errno = ER_NO_MASTER_SHM;
		return ((void *) -1);
	}

	shared_memory = (void *)shmat(shmid, (void *)NULL, shmat_priv);

	if ((void *) -1 == shared_memory) {
		errno = ER_NO_MASTER_SHM_ATT;
		return ((void *) -1);
	}

	/* Return shared memory address */

	return (shared_memory);
}
