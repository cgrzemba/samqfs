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
 * this is compile time tracing. The API implementation is being changed to
 * use runtime tracing instead (see Trace macro defined in sam_trace.h)
 */

#ifndef	_LOG_H
#define	_LOG_H

#pragma ident   "$Revision: 1.19 $"

#include "pub/mgmt/sqm_list.h"

#define	LOG_TAG "fsmgmt" /* tag for messages logged via syslogd */

#ifdef API_DEBUG
void
trace_to_syslog(char *msg, ... /* arguments */);
void
trace_to_stdout(char *msg, ... /* arguments */);

void
ptrace_to_syslog(int prio, char *fmt, ... /* arguments */);
void
ptrace_to_stdout(int prio, char *fmt, ... /* arguments */);

/*
 * TRACE = tracing macro. use is the same as for the trace() function above.
 * to enable tracing, compile with the -DAPI_DEBUG flag.
 * trace messages will go to either stdout (by default) or
 * to syslogd if -DAPI_DEBUG=syslog is specified.
 * the same facility level as in the rest of the SAM-FS code is used.
 *
 * PTRACE is similar to TRACE but it has an additional argument (first arg)
 * - the priority. Compile with -DTRACEPRIO=max_val if you want to trace only
 * messages with priority <= max_val
 */
#if API_DEBUG == syslog
#define	TRACE trace_to_syslog
#define	PTRACE ptrace_to_syslog
#else
#define	TRACE trace_to_stdout
#define	PTRACE ptrace_to_stdout
#endif /* API_DEBUG == syslog */

void
set_trace_prio(int new_prio);

#else
#define	TRACE
#define	PTRACE
#endif

#ifndef Str
#define	Str(s) (s ? (char *)s : "NULL")
#endif

/*
 * log a message via syslog
 */
void
slog(const char *msg, ... /* arguments */);


/*
 * Get the log information, i.e. logname, level, path to file and modtime
 */
int
get_daemontrace_info(sqm_lst_t **lst_trace);
int get_samlog_info(char **info);
int get_samlog_lst(sqm_lst_t **lst);
int get_devlog_info(sqm_lst_t **lst_devlog);
int get_recyclelog_info(char **info);
int get_archivelog_info(sqm_lst_t **lst);
int get_stagelog_info(char **info);
int get_releaselog_info(sqm_lst_t **lst);

#endif /* _LOG_H */
