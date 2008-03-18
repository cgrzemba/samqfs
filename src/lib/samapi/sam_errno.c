/*
 * sam_errno.c - API cover function for sam error number conversion
 *
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

#pragma ident "$Revision: 1.13 $"

#include "sys/types.h"
#include "sam/types.h"
#include "aml/types.h"
#include "sam/lib.h"
#include "aml/proto.h"

/*	External global variables declared */




/*
 *	sam_errno() - API function to interpret error number from other APIs
 *
 *	Input parameters --
 *		err		Error number received back from an API function
 *
 *	Output parameters --
 *		None
 *
 *	Return value --
 *		String value representing error number
 *
 */

char *
sam_errno(int errnum)
{
	static char buf[STR_FROM_ERRNO_BUF_SIZE];

	return (StrFromErrno(errnum, buf, sizeof (buf)));
}
