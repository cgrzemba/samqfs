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

#pragma ident   "$Revision: 1.16 $"

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include "mgmt/config/common.h"
#include "mgmt/log.h"
#include "mgmt/util.h"

#define	SYSLOG_FACILITY LOG_LOCAL6

/*
 * this is currently only used by the client-side utilities. Should
 * the server changeover to use this facility, there will need to be
 * an if-def-ed TRACE_FILENAME
 */
#define	TRACE_FILENAME "/var/log/webconsole/fsmgr.trace_syslog"

#define	TRACE_FILE_MAXSZ (5 * MEGA)
#define	CHK_COUNT 500

#define	BUFSZ 1024

#define	PROCESS_ARGS \
	char msg[BUFSZ];\
	va_list args;\
	va_start(args, fmt);\
	vsnprintf(msg, BUFSZ, fmt, args);\
	va_end(args)

int crtprio = 0;	// tracing is turned off by default
static int count = 0;

static pthread_once_t  _log_once = PTHREAD_ONCE_INIT;

/* function prototypes */
static void _log_init(void);
static void _log(int severity, char *msg);
static void _log_rotate(char *logfile);

void
set_trace_prio(int new_prio) {
	crtprio = new_prio;
}

void
trace_to_syslog(char *fmt, ...) {
	PROCESS_ARGS;
	_log(LOG_DEBUG, msg);
}


void
ptrace_to_syslog(int prio, char *fmt, ...) {
	PROCESS_ARGS;
	if (prio <= crtprio)
		_log(LOG_DEBUG, msg);
}


void
trace_to_stdout(char *fmt, ...) {
	PROCESS_ARGS;
	printf("fsmgmt %s\n", msg);
}


void
ptrace_to_stdout(int prio, char *fmt, ...) {
	PROCESS_ARGS;
	if (prio <= crtprio)
		printf("fsmgmt %s\n", msg);
}


void
slog(const char *fmt, ...) {
	PROCESS_ARGS;
	_log(LOG_INFO, msg);
}


static void
_log_init(void)
{
	openlog(LOG_TAG, LOG_PID, SYSLOG_FACILITY);

}

static void
_log(int severity, char *msg) {
	struct stat64 statbuf;

	/* init, but only once */
	pthread_once(&_log_once, _log_init);

	syslog(severity|SYSLOG_FACILITY, msg);

	count++;
	if (count != CHK_COUNT)
		return;

	count = 0;

	if (-1 == stat64(TRACE_FILENAME, &statbuf)) {
		return;
	}

	if (statbuf.st_size > TRACE_FILE_MAXSZ) {
		/* rotate trace file */
		_log_rotate(TRACE_FILENAME);
	}

}

static void
_log_rotate(char *logfile)
{

	int	maxcopy = 9;		/* keep up to 10 versions */
	int	i;
	char	buf[BUFSZ+1];
	char	buf2[BUFSZ+1];
	char	*lf_fmt = "%s.%d";

	/* expire old versions as required */

	snprintf(buf, sizeof (buf), lf_fmt, logfile, maxcopy);
	if (access(buf, F_OK)) {
		unlink(buf);
	}

	for (i = maxcopy-1; i >= 0; i--) {
		snprintf(buf, sizeof (buf), lf_fmt, logfile, i);
		snprintf(buf2, sizeof (buf2), lf_fmt, logfile, i + 1);
		if (access(buf, F_OK)) {
			rename(buf, buf2);
		}
	}

	/* The .0 slot is always open for us to use */
	snprintf(buf, sizeof (buf), lf_fmt, logfile, 0);
	rename(logfile, buf);

	/* touch the new logfile and tell syslogd that it's changed */
	mknod(logfile, 0644, 0);

	signal_process(0, "syslogd", SIGHUP);
}
