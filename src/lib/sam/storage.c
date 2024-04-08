/*
 * storage.c library interface for file storage info calls
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

#pragma ident "$Revision: 1.14 $"

#include <sys/types.h>
#include <sys/param.h>

#include <errno.h>

/* IBM stuff
#include "samsanergy/fsmdc.h"
#include "samsanergy/fsmdcsam.h"
*/

#include "sam/syscall.h"
#include "sam/lib.h"

/* Macros. */
	/* None. */

/* Types. */
	/* None. */

/* Structures. */
	/* None. */

/* Private data. */
	/* None. */

/* Private functions. */
	/* None. */

/* Public data. */
	/* None. */

/* Function macros. */
	/* None. */

/* Signal catching functions. */
	/* None. */

/*
 * SAM-FS system call.
 */
#define FSLONG long
#define FS64LONG long
#define FSMAPINFO  struct _FSMAPINFO

FSLONG
sam_fd_storage_ops(
	int fd,
	FS64LONG flags,
	FS64LONG flen,
	FS64LONG start,
	FS64LONG slen,
	FSMAPINFO *buf,
	FS64LONG buflen)
{
	struct sam_fd_storage_ops_arg arg;

	arg.fd = fd;
	arg.flags = flags;
	arg.flen = flen;
	arg.start = start;
	arg.slen = slen;
	arg.buf.ptr = buf;
	arg.buflen = buflen;

	return (sam_syscall(SC_store_ops, &arg, sizeof (arg)));
}
