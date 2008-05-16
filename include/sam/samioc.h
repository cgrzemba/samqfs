#ifndef	_SAMIOC_H
#define	_SAMIOC_H

#ifdef sun
#pragma ident "$Revision: 1.10 $"
#endif

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

/*
 *	Definitions needed for the ioctl() interface that implements
 *	the SAMFS system call.
 */

#ifdef	linux

#ifndef	__KERNEL__
#include <sys/ioctl.h>
#endif	/* __KERNEL__ */

struct sam_syscall_args {
	long cmd;
	void *buf;
	long size;
};

#define	SAMSYS_CDEV		"samsys"
#define	SAMSYS_CDEV_NAME	"/proc/fs/samfs/samsys"
/*
 * The major device number of /proc/fs/samfs/samsys
 * is dynamically assigned when it registers
 */

#define	SAMSYS_IOCTL	's'
#define	SAMSYS_IOC_NULL		_IO(SAMSYS_IOCTL, 0)
#define	SAMSYS_IOC_SAM_SYSCALL	_IO(SAMSYS_IOCTL, 1)

#else	/* linux */

#define	SAMDEV		"/devices/pseudo/samioc@0:syscall"
#define	SAMDEV_CMD	0

struct sam_syscall_args {
	int		cmd;
	void	*buf;
	int		size;
};

#endif	/* linux */

#endif	// _SAMIOC_H
