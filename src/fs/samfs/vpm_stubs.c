/*
 * vpm_stubs.c - SAM-FS VPM function stubs for S10.
 *
 *	Defines global definitions for VPM functions to be used
 *	prior to S11.
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

#ifdef sun
#pragma ident "$Revision: 1.1 $"
#endif


#include "sam/osversion.h"


/* ----- UNIX Includes */
#ifdef sun
#include <sys/types.h>
#include <sys/buf.h>
#include <sys/uio.h>

/* ----- SAMFS Includes */

#include "sam/types.h"
#include "macros.h"
#include "inode.h"
#include "mount.h"
#include "extern.h"
#endif


/* Externs for VPM (prior to S11) */

/* ARGSUSED */
extern int
vpm_map_pages(struct vnode *vp, u_offset_t off, size_t len,
    int fetchpage, vmap_t *vml, int nseg, int  *newpage,
    enum seg_rw rw)
{
	return (0);
}

/* ARGSUSED */
extern void
vpm_unmap_pages(vmap_t *vml, enum seg_rw rw)
{
}

/* ARGSUSED */
extern int
vpm_sync_pages(struct vnode *vp, u_offset_t off, size_t len,
    uint_t flags)
{
	return (0);
}

/* ARGSUSED */
extern int
vpm_data_copy(struct vnode *vp, u_offset_t off, size_t len,
    struct uio *uio, int fetchpage, int *newpage, int zerostart,
    enum seg_rw rw)
{
	return (0);
}
