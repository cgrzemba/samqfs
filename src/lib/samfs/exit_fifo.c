/*
 * exit_fifo.c - routines to define and send/receive exit status on a FIFO.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.18 $"


#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <syslog.h>

#include "aml/device.h"
#include "aml/exit_fifo.h"
#include "aml/proto.h"
#include "sam/lib.h"

#define	PATHLEN 128


char *exit_completed = "completed";
char *exit_unknown = "unknown error";


void
set_exit_id(int seqnum, exit_FIFO_id *id)
{
	if (id == NULL)
		return;

	id->pid = getpid();
	id->thr_id = thr_self();
	id->seqnum = seqnum;
}


/*
 * create_exit_FIFO
 * - creates a named pipe
 */
int
create_exit_FIFO(exit_FIFO_id *id)
{
	char pathname[PATHLEN];

	get_exit_fifo_path(pathname, id);
	return (mkfifo(pathname, 0666));
}


/*
 * unlink_exit_FIFO
 * - removes a named pipe
 */
int
unlink_exit_FIFO(exit_FIFO_id *id)
{
	char pathname[PATHLEN];

	get_exit_fifo_path(pathname, id);
	return (unlink(pathname));
}


/*
 * write_client_exit_string(id, code, string)
 * - writes exit code and string to client
 * - if string is NULL, will send errno string or such
 * - marks the id as "closed" by senting pid to 0
 */
void
write_client_exit_string(exit_FIFO_id *id, int exit_code, char *exit_string)
{
	int fd;
	exit_FIFO_header exit_header;
	char pathname[PATHLEN];
	int retry = 3;


	if (id->pid == 0)
		return;

	if (exit_string == NULL) {
		if (!exit_code)
			exit_string = exit_completed;
		else
		{
			exit_string = error_handler(exit_code);
			if (!exit_string)
				exit_string = exit_unknown;
		}
	}

	get_exit_fifo_path(pathname, id);

	/*
	 * in some cases such as chmed, control could come here before the
	 *  read_exit_fifo function is executed. Hence, the exit fifo pathname
	 *  file may not yet be opened. If the open fails here, delay and try
	 *  again.
	 */

	while (retry-- > 0) {
		if ((fd = open(pathname, O_WRONLY|O_NONBLOCK)) >= 0)
			break;
		if (errno == ENXIO)
			(void) sleep(1);	/* pause and retry */
		else
			break;
	}

	if (fd < 0) {
		sam_syslog(LOG_INFO,
		    "write_client_exit_string: code = %d,"
		    " path = %s errno = %d\n",
		    exit_code, pathname, errno);
		return; /* couldn't open the pipe to send the exit, bail out */
	}

	exit_header.code = exit_code;
	exit_header.length = strlen(exit_string)+1;

	write(fd, &exit_header, sizeof (exit_header));
	write(fd, exit_string, exit_header.length);

	(void) close(fd);

	id->pid = 0;
}

static sigjmp_buf env;

static void
read_exit_signal(int sig)
{
	/* return to the jump point with the signal */
	siglongjmp(env, sig);
}


/*
 * read_server_exit_string(exit_FIFO_id, code, data, length, sec)
 * - exit_FIFO_id is the id passed on the command block
 * - code is the address to return an int exit code
 * - data is a pointer to storage to hold the exit string
 * - length is the length of the exit data passed in
 * - sec is a timeout value(-1 means monitor signals, but no timeout)
 * Returns:
 * > 0 the length of the data read
 * < 0 an exit occurred, check errno
 *    EINTR if interrupted
 *    EBUSY if it timed out
 */
int
read_server_exit_string(exit_FIFO_id *id, int *exit_code, char *exit_data,
    int exit_length, int timeout)
{
	int fd = -1, return_length = -1, read_len;
	exit_FIFO_header exit_header;
	char pathname[PATHLEN];
	int return_signal = 0;

	if (timeout && thr_main() < 0) {
		/* watch for alarms and user interrupts */
		(void) sigset(SIGALRM, read_exit_signal);
		(void) sigset(SIGINT, read_exit_signal);
		/* set an alarm for timeout */
		if (timeout > 0)
			(void) alarm(timeout);
	}

	/*
	 * if ...
	 *  we're running threaded
	 *  or there was no timeout specified
	 *  or no signals received yet
	 *  then read the data
	 */
	if (thr_main() >= 0 || timeout == 0 ||
	    (return_signal = sigsetjmp(env, 0)) == 0) {

		get_exit_fifo_path(pathname, id);
		if ((fd = open(pathname, O_RDONLY)) < 0)
			goto read_exit;

		if (read(fd, &exit_header,
		    sizeof (exit_header)) == sizeof (exit_header)) {

			*exit_code = exit_header.code;
			read_len = exit_length < exit_header.length ?
			    exit_length : exit_header.length;
			if (read(fd, exit_data, read_len) == read_len)
				return_length = read_len;
		}
	} else { /* some signal interrupted the read process */
		switch (return_signal) {
		case SIGINT:
			errno = EINTR;
			break;
		case SIGALRM:
			errno = ETIME;
			break;
		}
	}

read_exit:
	if (timeout && thr_main() < 0) {
		if (timeout > 0)
			(void) alarm(0);
		(void) sigset(SIGALRM, SIG_DFL);
		(void) sigset(SIGINT, SIG_DFL);
	}
	if (fd > 0)
		(void) close(fd);
	return (return_length);
}


int
timeout_factor(equ_t equ_type)
{
	switch (equ_type) {
	default:
		if (is_tape(equ_type))
			return (600);    /* ten minutes */
		else if (is_optical(equ_type))
			return (300);    /* five minutes */
		return (3600);    /* one hour */
	}
}
