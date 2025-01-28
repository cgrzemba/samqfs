/*
 * ----- utility/samfm_client.c - StorADE API.
 *
 * Request information from SAM-FS for StorADE.
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include "sam/spm.h"
#include "samfm.h"

static char *vstrapp(char *str, char *fmt, va_list args);
static char *strapp(char *str, char *fmt, ...);
static int comm(int fd, int timeout, char *send, char **recv);

/*
 * ----- Append string with formatted data.
 * Returns NULL if error, new string if successful.
 */
static char *
strapp(char *str, char *fmt, ...)
{
	va_list args;
	char	*ptr;

	if (fmt == NULL) {
		return (NULL);
	}

	va_start(args, fmt);

	ptr = vstrapp(str, fmt, args);

	va_end(args);

	return (ptr); /* string appended */
}

/*
 * ----- Append variable argument list to a string with formatted data.
 * Returns NULL if error, new string if successful.
 */
static char *
vstrapp(
	char *str,
	char *fmt,
	va_list args)
{

/* N.B. Bad indentation here to meet cstyle requirements */
#define	TMP_LEN 10

	int	count;
	int	offset;
	char	*ptr;
	char	tmp[TMP_LEN];

	if (fmt == NULL) {
		return (NULL);
	}

	if ((count = vsnprintf(tmp, TMP_LEN, fmt, args)) == -1) {
		return (NULL);
	}

	if (str == NULL) {
		if ((ptr = (char *)malloc(count+1)) == NULL) {
			return (NULL);
		}
		ptr[0] = '\0';
		offset = 0;
	} else {
		offset = strlen(str);
		if ((ptr = (char *)realloc(str, strlen(str)+count+1)) == NULL) {
			return (NULL);
		}
	}

	(void) vsprintf(ptr+offset, fmt, args);

	return (ptr);
}

/*
 * ----- Communicate with robot daemon.
 */
static int /* Non-zero if error, 0 if successful. */
comm(
	int fd,
	int timeout,
	char *send,
	char **recv)
{
	char	*ptr;
	int	bytes;
	char	buf[100];
	struct pollfd fds[1];

	bytes = strlen(send)+1;
	if (write(fd, send, bytes) != bytes) {
		return (SAMFM_ERROR_WRITE);
	}

	fds[0].fd = fd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	if (poll(fds, 1, timeout) != 1 && fds[0].revents != POLLIN) {
		free(*recv);
		return (SAMFM_ERROR_POLL);
	}

	*recv = NULL;
	do {
		if ((bytes = read(fd, buf, 100)) <= 0) {
			free(*recv);
			*recv = NULL;
			return (SAMFM_ERROR_READ);
		}

		if ((ptr = strapp(*recv, "%s", buf)) == NULL) {
			free(*recv);
			*recv = NULL;
			return (SAMFM_ERROR_MEMORY);
		}
		*recv = ptr;

	} while (bytes == 100);

	return (0);
}

/*
 * ----- StorADE API.
 */
int /* Non-zero if error, 0 if successful. */
samfm(
	char *host,
	int timeout,
	char *send,
	char **recv)
{
	int	fd;
	int	error;
	char	ebuf[SPM_ERRSTR_MAX];

	if (host == NULL || send == NULL) {
		return (SAMFM_ERROR_ARGS);
	}

	if ((fd = spm_connect_to_service("samfm", host, 0,
	    &error, ebuf)) == -1) {
		return (SAMFM_ERROR_CONNECT);
	}

	error = comm(fd, timeout, send, recv);

	(void) close(fd);
	return (error);
}
