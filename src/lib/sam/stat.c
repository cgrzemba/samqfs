/*
 * stat.c - Get information about a SAMFS file.
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

#pragma ident "$Revision: 1.20 $"


/* Feature test switches. */
	/* None. */

/* ANSI C headers. */
	/* None. */

/* POSIX headers. */
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

/* Solaris headers. */
	/* None. */

/* SAM-FS headers. */
#include "sam/syscall.h"
#include "sam/lib.h"
#include "pub/stat.h"

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
sam_stat(const char *path, struct sam_stat *buf, size_t bufsize)
{
#ifdef sun
#define	L_STAT stat64
#else
#define	L_STAT stat
#endif
	struct L_STAT stat_buf;
	int err;
	struct sam_stat_arg arg;

	memset(&arg, 0, sizeof (arg));
	arg.path.ptr = (char *)path;
	arg.buf.ptr = buf;
	arg.bufsize = bufsize;
	err = sam_syscall(SC_stat, &arg, sizeof (arg));

	/* if not ours, or samfs not loaded, do regular stat */

	if (err && (errno == ENOSYS || errno == ENOPKG)) {
		memset(buf, 0, sizeof (struct sam_stat));
		err = L_STAT(path, &stat_buf);
		if (!err) {
			buf->st_mode = stat_buf.st_mode;
			buf->st_ino = stat_buf.st_ino;
			buf->st_dev = stat_buf.st_dev;
			buf->st_nlink = stat_buf.st_nlink;
			buf->st_uid = stat_buf.st_uid;
			buf->st_gid = stat_buf.st_gid;
			buf->st_size = stat_buf.st_size;
			buf->st_atime = stat_buf.st_atim.tv_sec;
			buf->st_mtime = stat_buf.st_mtim.tv_sec;
			buf->st_ctime = stat_buf.st_ctim.tv_sec;
			buf->rdev = stat_buf.st_rdev;
			buf->st_blocks = stat_buf.st_blocks;
		}
	}
	return (err);
}
