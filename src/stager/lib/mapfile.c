/*
 * mapfile.c - stager's memory mapped file api
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

#pragma ident "$Revision: 1.18 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* POSIX headers. */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
struct tm *localtime_r(const time_t *clock, struct tm *res);
#include <procfs.h>

/* Solaris headers. */
#include <sys/mman.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/udscom.h"
#include "pub/stat.h"
#include "sam/exit.h"
#include "sam/custmsg.h"
#include "sam/sam_trace.h"
#include "aml/stager_defs.h"

/* Local headers. */
#include "stager_config.h"
#include "stager_lib.h"

#if defined(lint)
#include "sam/lint.h"
#undef unlink
#endif /* defined(lint) */

/*
 * Establish mapping between adddress space and file.
 * System call errors are fatal.
 */
void*
MapInFile(
	char *file_name,
	int mode,
	size_t *len)
{
	int prot;
	int fd;
	int err;
	struct stat st;
	void *mp = NULL;
	int retry;

	prot = (O_RDONLY == mode) ? PROT_READ : PROT_READ | PROT_WRITE;

	retry = 3;
	fd = -1;
	while (fd == -1 && retry-- > 0) {
		fd = open(file_name, mode);
		if (fd == -1) {
			Trace(TR_ERR, "MapInFile open failed: '%s', errno %d",
			    file_name, errno);
			sleep(5);
		}
	}

	if (fd == -1) {
		if (len != NULL) {
			*len = 0;
		}
		return (NULL);
	}

	err = fstat(fd, &st);
	if (err != 0) {
		FatalSyscallError(EXIT_NORESTART, HERE, "fstat", file_name);
	}

	retry = 3;
	mp = MAP_FAILED;
	while (mp == MAP_FAILED && retry-- > 0) {
		mp = mmap(NULL, st.st_size, prot, MAP_SHARED, fd, 0);
		if (mp == MAP_FAILED) {
			Trace(TR_ERR, "MapInFile mmap failed: '%s', errno: %d",
			    file_name, errno);
			sleep(5);
		}
	}

	if (mp == MAP_FAILED) {
		FatalSyscallError(EXIT_FATAL, HERE, "mmap", file_name);
	}

	(void) close(fd);

	if (len != NULL) {
		*len = st.st_size;
	}

	return (mp);
}

/*
 * Write map file.
 * System call errors are fatal.
 */
int
WriteMapFile(
	char *file_name,
	void *mp,
	size_t len)
{
	int fd;
	size_t num_written;

	(void) unlink(file_name);

	fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0664);
	if (fd == -1) {
		WarnSyscallError(HERE, "open", file_name);
		return (-1);
	}

	num_written = write(fd, mp, len);
	if (num_written != len) {
		WarnSyscallError(HERE, "write", file_name);
		return (-1);
	}

	(void) close(fd);

	return (0);
}

/*
 * Unmap memory mapped file.
 */
void
UnMapFile(
	void *mp,
	size_t len)
{
	int err;
	err = munmap(mp, len);
	if (err == -1) {
		WarnSyscallError(HERE, "munmap", "");
	}
}

/*
 * Unmap and unlink memory mapped file.
 */
void
RemoveMapFile(
	char *file_name,
	void *mp,
	size_t len)
{
	int err;

	err = munmap(mp, len);
	if (err == -1) {
		WarnSyscallError(HERE, "munmap", "");
	}

	err = unlink(file_name);
	if (err == -1) {
		WarnSyscallError(HERE, "unlink", "");
	}
}
