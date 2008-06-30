/*
 * ----- objattrops.h - Object attributes definition.
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

#pragma ident "$Revision: 1.1 $"

#ifndef _OBJATTROPS_H
#define	_OBJATTROPS_H

extern int sam_obj_setattr(objnode_t *objnodep, uint32_t pagenum, uint32_t
    attrnum, uint64_t len, void *buf, void *cap, void *sec, cred_t *credp);

extern int sam_obj_getattr(objnode_t *objnodep, uint32_t pagenum, uint32_t
    attrnum, uint64_t len, void *buf, void *cap, void *sec, cred_t *credp);

extern int sam_obj_getattrpage(objnode_t *objnodep, uint32_t pagenum,
    uint64_t len, void *buf, void *cap, void *sec, cred_t *credp);

#endif /* _OBJATTROPS_H */
