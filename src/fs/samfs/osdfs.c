/*
 * ---- osdfs.c - Object File System Level Operations.
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

#ifdef sun
#pragma ident "$Revision: 1.3 $"
#endif

#include "sam/osversion.h"

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/mutex.h>
#include <sys/flock.h>
#include <sys/debug.h>
#include <sys/kmem.h>
#include <sys/cmn_err.h>

#include "pub/objnops.h"
#include "pub/osdfsops.h"
#include "osdfs.h"

/*
 * ----- samfs_osdfs_alloc - Allocate an osdfs and initialize it with osdfsops.
 * All other fields are not initialized.
 *
 * osdfs_alloc is the external interface to retrieve the set of
 * osdfsopsp function pointers.  When a backing store is externalised
 * on the OSD Target as a LUN, the LUN Driver retrieves osdfsopsp by
 * getting the address of osdfs_alloc via ddi_XXXX.  Using this function
 * the LUN Driver calls to allocate a osdfs_t structure for the backing store.
 * With this osdfs_t structure, the LUN Driver can now call the OSD Target
 * File System to attach and detach the LUN to the Backing Store.
 *
 * The osdfs_t structure works like the vfs_t structure.
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
samfs_osdfs_alloc(
	osdfs_t **osdfspp,
	cred_t *credp)		/* Credentials pointer */
{

	osdfs_t *osdfsp;

	if (!osdfspp) {
		return (EINVAL);
	}

	osdfsp = (struct osdfs *)kmem_zalloc(sizeof (struct osdfs), KM_SLEEP);
	osdfsp->osdfs_op = osdfsopsp;
	*osdfspp = osdfsp;

	return (0);
}

/*
 * ----- samfs_osdfs_free - Free the osdfs structure.
 * This osdfs_t structure is not useable once it has been freed.
 */
/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
samfs_osdfs_free(
	osdfs_t *osdfsp,
	cred_t *credp)		/* Credentials pointer */
{

	if (!osdfsp) {
		return (EINVAL);
	}
	kmem_free(osdfsp, sizeof (struct osdfs));
	return (0);
}
