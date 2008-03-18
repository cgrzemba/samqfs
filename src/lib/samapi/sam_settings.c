/*
 * sam_settings.c - get default settings and system messages API
 *
 *	sam_settings() will retrieve the contents of the defaults structure and
 *	the two system message arrays and return the structure address
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

#pragma ident "$Revision: 1.20 $"


#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/shm.h>

#include "sam/defaults.h"
#include "sam/lib.h"
#include "sam/param.h"
#include "aml/shm.h"
#include "aml/samapi.h"
#include "pub/sam_errno.h"

/*
 *	sam_settings() - API function to obtain the default settings
 *
 *	Input parameters --
 *		None
 *
 *	Output parameters --
 *		defaults	Pointer to structure containing defaults
 *				and system messages
 *		errno		Set to error number if error encountered
 *
 *	Return value --
 *		0		If successful in obtaining the defaults
 *		-1		If error encountered,
 *				'errno' set to error number
 *
 */

int
sam_settings(struct sam_def_values *defaults, int size)
{
	int		i;		/* Temporary index */

	shm_ptr_tbl_t	*shm_mstr;	/* Master shared memory segment */
	sam_defaults_t	*shm_defaults;	/* Defaults table in shared memory */


	/*
	 *	Verify input
	 */

	if (size < sizeof (struct sam_def_values)) {
		errno = ER_STRUCTURE_TOO_SMALL;
		return (-1);
	}

	/*
	 *	Access defaults table in shared memory segment
	 */

	if ((shm_mstr = (shm_ptr_tbl_t *)sam_mastershm_attach(0, SHM_RDONLY))
	    == (void *) -1) {
		return (-1);
	}

	shm_defaults = GetDefaults();

	/*
	 *	Transfer fields from shared memory to reply structure
	 */

	defaults->optical = shm_defaults->optical;
	defaults->tape = shm_defaults->tape;
	defaults->operator = shm_defaults->operator;
	defaults->debug = shm_defaults->debug;
	defaults->timeout = shm_defaults->timeout;
	defaults->previews = shm_defaults->previews;
	defaults->stages = shm_defaults->stages;
	defaults->tapemode = shm_defaults->tapemode;
	defaults->log_facility = shm_defaults->log_facility;
	defaults->stale_time = shm_defaults->stale_time;
	defaults->idle_unload = shm_defaults->idle_unload;
	defaults->shared_unload = shm_defaults->shared_unload;
	defaults->flags = shm_defaults->flags;

	/*
	 *	Transfer system messages to reply structure
	 */

	for (i = 0; i < DIS_MES_TYPS; i++) {
		(void) strncpy(defaults->dis_mes[i], shm_mstr->dis_mes[i],
		    DIS_MES_LEN);
	}

	shmdt((char *)shm_mstr);
	return (0);
}
