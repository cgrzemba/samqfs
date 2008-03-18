#pragma ident "$Revision: 1.21 $"

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

/*
 * sam_syscall.c - Perform SAM-FS system call.
 */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stropts.h>
#include <fcntl.h>

/* POSIX headers. */
#include <sys/types.h>
#include <signal.h>

/* Solaris includes. */
#include <syslog.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/lib.h"
#include "sam/syscall.h"
#include "sam/samioc.h"


/*
 * Perform SAM-FS system call using an ioctl().
 */
int
sam_syscall(
	int number,		/* System call number */
	void *arg,		/* Argument structure */
	int size)		/* Size of argument structure */
{

#ifdef linux
	int fd, cmd, ret = 0;
	struct sam_syscall_args scargs;

	fd = open(SAMSYS_CDEV_NAME, O_RDONLY);

	if (fd < 0) {
		return (-1);
	}

	scargs.cmd	= number;
	scargs.buf	= arg;
	scargs.size	= size;

	ret = ioctl(fd, SAMSYS_IOC_SAM_SYSCALL, (char *)&scargs);

	close(fd);
	return (ret);

#else /* linux */

	/*
	 * No system call number is used. Use an ioctl to communicate with the
	 * file system.
	 */

	static int		syscall_fd = -1;
	struct sam_syscall_args	scargs;

	if (syscall_fd < 0) {

		/*
		 * Use only one file descriptor for all threads in this process.
		 */

		syscall_fd = open(SAMDEV, O_RDONLY);

		if (syscall_fd < 0) {
			return (-1);
		}
	}

	scargs.cmd	= number;
	scargs.buf	= arg;
	scargs.size	= size;

	return (ioctl(syscall_fd, SAMDEV_CMD, (char *)&scargs));

#endif /* linux */

}
