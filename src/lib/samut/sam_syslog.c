/*
 * sam_syslog.c - Process syslog messages.
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

/*
 * Provide syslog message facility for SAM processes.
 */

#pragma ident "$Revision: 1.22 $"

/* ANSI C headers. */
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* Solaris headers. */
#include <syslog.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/defaults.h"
#include "sam/lib.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Private data. */
static pthread_mutex_t syslogMutex = PTHREAD_MUTEX_INITIALIZER;


/*
 * Open syslog for sam.
 */
void
SamlogOpen(
	char *ident)
{
	static sam_defaults_t *defaults = NULL;
	static struct DefaultsBin *def_bin = NULL;

	if (defaults == NULL || def_bin->Db.MfValid == 0) {
		defaults = GetDefaults();
		def_bin = (struct DefaultsBin *)(void *) ((char *)defaults -
		    offsetof(struct DefaultsBin, DbDefaults));
	}
	openlog(ident, LOG_PID | LOG_NOWAIT, defaults->log_facility);
}


/*
 * Send a syslog message.
 */
void
sam_syslog(
	int priority,
	const char *fmt_a,
	...)
{
	extern char *program_name;
	va_list args;
	char *m_format;
	char *fmt;
	char msg[256];
	char *ident;
	int saveErrno;

	saveErrno = errno;
	if (program_name == NULL || *program_name == '\0') {
		ident = "SAM-FS";
	} else {
		ident = program_name;
	}

	/*
	 * Check for %m.
	 */
	fmt = (char *)fmt_a;
	m_format = strstr(fmt_a, "%m");
	if (m_format != NULL) {
		/*
		 * Make up a new format string replacing the the first %m
		 * with the 'errno' message.  (More than one %m is most
		 * likely an error.)
		 */
		int		l;

		l = STR_FROM_ERRNO_BUF_SIZE + strlen(fmt_a);
		fmt = malloc(l);
		if (fmt != NULL) {
			char	*f, *fe;
			int		n;

			fe = fmt + l;
			n = Ptrdiff(m_format, fmt_a);	/* Format before %m */
			memmove(fmt, fmt_a, n);
			f = fmt + n;			/* Error string */
			(void) StrFromErrno(saveErrno, f, Ptrdiff(fe, f));
			f += strlen(f);			/* Format after %m */
			strncpy(f, fmt_a + n + 2, Ptrdiff(fe, f));
		} else {
			/*
			 * No space for new format string, use the provided
			 * format string.
			 */
			fmt = (char *)fmt_a;
			m_format = NULL;
		}
	}
	(void) pthread_mutex_lock(&syslogMutex);
	SamlogOpen(ident);
	va_start(args, fmt_a);
	vsnprintf(msg, sizeof (msg), fmt, args);
	va_end(args);
	syslog(priority, "%s", msg);
	closelog();
	(void) pthread_mutex_unlock(&syslogMutex);
	if (m_format != NULL) {
		free(fmt);
	}
	errno = saveErrno;
}

#if defined(TEST)

int
main()
{
	errno = 23;
	sam_syslog(LOG_WARNING, "Check imbedded errno message: %m - %d", errno);
	sam_syslog(LOG_WARNING, "%s", "Check percent - file%fname");
	exit(EXIT_SUCCESS);
}

#endif /* defined(TEST) */
