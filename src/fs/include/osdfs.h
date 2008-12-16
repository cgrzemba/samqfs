/*
 * ----- osdfs.h - Object file system external definitions.
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
#ifndef _OSDFS_H
#define	_OSDFS_H

#pragma ident "$Revision: 1.3 $"

extern osdfsops_t *osdfsopsp;
extern int samfs_osdfs_alloc(osdfs_t **osdfspp, cred_t *credp);
extern int samfs_osdfs_free(osdfs_t *osdfsp, cred_t *credp);
extern int osdfs_ioctl_register(osdfs_t *osdfsp, char *fsname, cred_t *credp);
extern int osdfs_ioctl_unregister(osdfs_t *osdfsp, int fflag,  cred_t *credp);

#endif /* _OSDFS_H */
