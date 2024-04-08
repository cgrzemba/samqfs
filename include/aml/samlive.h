/*
 * samlive.h - SAM-FS Live API library functions.
 *
 * Definitions for SAM-FS Live API library functions.
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

#ifndef _AML_SAMLIVE_H
#define	_AML_SAMLIVE_H

#pragma ident "$Revision: 1.12 $"

#include <sys/types.h>



/*
 * SAM Live API function calls
 */


/*
 * Live access to SAMFS data APIs
 */

int samlive_cathandle(const equ_t equ, int *max_count);

int samlive_catlist(int handle, int *, equ_t equ, int count);

int samlive_devhandle(int *max_count);

int samlive_devlist(int handle, dev_ent_t **dev_ptr, int count);

int samlive_devsystemmsgs(int handle, char **msgs);

int samlive_mntreqhandle(int *max_count, int **active_count);

int samlive_mntreqlist(int handle, preview_t **mntreq_ptr, int count);

#endif /* _AML_SAMLIVE_H */
