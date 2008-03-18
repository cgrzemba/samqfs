/*
 * readrminfo.c - Get information about a SAMFS removable media file.
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

#pragma ident "$Revision: 1.14 $"


/* Feature test switches. */
	/* None. */

/* ANSI C headers. */
	/* None. */

/* POSIX headers. */
	/* None. */

/* Solaris headers. */
	/* None. */

/* SAM-FS headers. */
#include "sam/syscall.h"
#include "sam/lib.h"
#include "pub/rminfo.h"

/* Local headers. */
	/* None. */

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


int
sam_readrminfo(const char *path, struct sam_rminfo *buf, size_t bufsize)
{
	struct sam_readrminfo_arg arg;

	arg.path.ptr = path;
	arg.buf.ptr = buf;
	arg.bufsize = bufsize;
	return (sam_syscall(SC_readrminfo, &arg, sizeof (arg)));
}
