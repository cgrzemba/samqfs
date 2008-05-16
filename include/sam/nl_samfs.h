/*
 * nl_samfs.h - Native Language SAMFS Include.
 *
 *  Description:
 *    Include file for SAMFS internationalization.
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

#ifdef sun
#pragma ident "$Revision: 1.18 $"
#endif


#if !defined(_SAM_NL_SAMFS_H)
#define	_SAM_NL_SAMFS_H

#include <locale.h>
#include <nl_types.h>
#include <limits.h>
#include "sam/names.h"

#define	NL_CAT_NAME "SUNWsamfs"
#define	DEFCAT "/usr/lib/locale/C/LC_MESSAGES/"

#define	SET 1

nl_catd catfd;

#endif  /* _SAM_NL_SAMFS_H */
