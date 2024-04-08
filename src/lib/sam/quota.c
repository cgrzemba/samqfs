/*
 * quota.c library interface for Quota API calls
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

#pragma ident "$Revision: 1.16 $"

/* Feature test switches. */
	/* None. */

/* ANSI C headers. */
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>

/* Solaris headers. */
#include <errno.h>

/* SAM-FS headers. */
#include <sam/types.h>
#include <sam/param.h>
#define	__QUOTA_DEFS
#include "sam/quota.h"
#include "sam/syscall.h"
#include "sam/lib.h"

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

int
sam_get_quota_entry_by_fd(int fd, int type, int *index, struct sam_dquot *dq)
{
	int err;
	struct sam_quota_arg arg;

	memset(&arg, 0, sizeof (arg));

	arg.qcmd = SAM_QOP_GET;
	arg.qflags = 0;
	arg.qsize = sizeof (struct sam_quota_arg);
	arg.qtype = type;
	arg.qindex = 0;
	arg.qfd = fd;
	arg.qp.ptr = dq;

	err = sam_syscall(SC_quota, &arg, sizeof (arg));
	if (!err && index != NULL) {
		*index = arg.qindex;
	}
	return (err);
}

int
sam_get_quota_entry_by_index(int fd, int type, int index, struct sam_dquot *dq)
{
	struct sam_quota_arg arg;

	memset(&arg, 0, sizeof (arg));

	arg.qcmd = SAM_QOP_GET;
	arg.qflags = SAM_QFL_INDEX;
	arg.qsize = sizeof (struct sam_quota_arg);
	arg.qtype = type;
	arg.qindex = index;
	arg.qfd = fd;
	arg.qp.ptr = dq;

	return (sam_syscall(SC_quota, &arg, sizeof (arg)));
}

int
sam_put_quota_entry(int fd, int type, int index, struct sam_dquot *dq)
{
	struct sam_quota_arg arg;

	memset(&arg, 0, sizeof (arg));

	arg.qcmd = SAM_QOP_PUT;
	arg.qflags = SAM_QFL_INDEX;
	arg.qsize = sizeof (struct sam_quota_arg);
	arg.qtype = type;
	arg.qindex = index;
	arg.qfd = fd;
	arg.qp.ptr = dq;

	return (sam_syscall(SC_quota_priv, &arg, sizeof (arg)));
}


int
sam_putall_quota_entry(int fd, int type, int index, struct sam_dquot *dq)
{
	struct sam_quota_arg arg;

	memset(&arg, 0, sizeof (arg));

	arg.qcmd = SAM_QOP_PUTALL;
	arg.qflags = SAM_QFL_INDEX;
	arg.qsize = sizeof (struct sam_quota_arg);
	arg.qtype = type;
	arg.qindex = index;
	arg.qfd = fd;
	arg.qp.ptr = dq;

	return (sam_syscall(SC_quota_priv, &arg, sizeof (arg)));
}

int
sam_get_quota_stat(int fd, int *mask)
{
	int r;
	struct sam_quota_arg arg;

	arg.qcmd = SAM_QOP_QSTAT;
	arg.qflags = 0;
	arg.qsize = sizeof (struct sam_quota_arg);
	arg.qtype = 0;
	arg.qindex = 0;
	arg.qfd = fd;
	arg.qp.ptr = NULL;

	r = sam_syscall(SC_quota, &arg, sizeof (arg));
	if (r < 0)
		return (r);
	if (mask)
		*mask = arg.qflags;
	return (0);
}


/*
 * Change file's admin-ID
 */
int
sam_chaid(const char *path, int aid)
{
	struct sam_chaid_arg arg;

	arg.path.ptr = path;
	arg.admin_id = aid;
	arg.follow = 1;
	return (sam_syscall(SC_chaid, &arg, sizeof (arg)));
}

/*
 * Same thing, but if the target is a symbolic link,
 * don't follow it.
 */
int
sam_lchaid(const char *path, int aid)
{
	struct sam_chaid_arg arg;

	arg.path.ptr = path;
	arg.admin_id = aid;
	arg.follow = 0;
	return (sam_syscall(SC_chaid, &arg, sizeof (arg)));
}
