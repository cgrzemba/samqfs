/*
 *	copy_extents.h - SAM-QFS extent copy definitions
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */


#ifndef	_SAM_COPY_EXTENTS_H
#define	_SAM_COPY_EXTENTS_H

#ifdef sun
#pragma ident "$Revision: 1.3 $"
#endif

extern int sam_allocate_and_copy_extent(sam_node_t *ip, int bt, int dt,
	sam_bn_t obn, int oord, buf_t **obpp, sam_bn_t *nbn, int *nord,
    buf_t **nbpp);
extern int sam_allocate_and_copy_bp(sam_node_t *ip, int bt, int dt,
	sam_bn_t obn, int oord, buf_t *obp, sam_bn_t *nbn, int *nord,
    buf_t **nbpp);
extern int sam_allocate_and_copy_directmap(sam_node_t *ip,
    sam_ib_arg_t *ib_args, cred_t *credp);

#endif	/* _SAM_COPY_EXTENTS_H */
