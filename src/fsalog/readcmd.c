/*
 * readcmd.c - Read and parse command file for sam-shrink.
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

#pragma ident "$Revision: 1.3 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>

/* Solaris headers. */
#include <sys/param.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/lib.h"
#include "sam/readcfg.h"
#include "sam/names.h"
#include "sam/sam_malloc.h"

#include "sam/lint.h"

					/* Utlity functions.		*/
/*
 * Private functions.
 */
static void cmd_fs(void);
static void cmd_log_file(void);
static void cmd_log_rollover(void);
static void cmd_log_expire(void);
static void cmd_event_interval(void);
static void cmd_event_buffer(void);
static void cmd_open_retry(void);
static void read_config_msg(char *msg, int lineno, char *line);

/*
 * Command table
 */
static DirProc_t dirProcTable[] = {
	{ "fs",				cmd_fs,			DP_value },
	{ "log_path",			cmd_log_file,		DP_value },
	{ "log_rollover_interval",	cmd_log_rollover,	DP_value },
	{ "log_expire",			cmd_log_expire,		DP_value },
	{ "event_interval",		cmd_event_interval,	DP_value },
	{ "event_buffer_size",		cmd_event_buffer,	DP_value },
	{ "event_open_retry",		cmd_open_retry,		DP_value },
	{ NULL,	NULL }
};

static char	dirname[TOKEN_SIZE];
static char	token[TOKEN_SIZE];
static boolean_t not_for_this_fs = FALSE;

/*
 * Externals to communicate with main().
 */
extern char	*program_name;
extern char	*fs_name;		/* File system name		*/
extern int	Daemon;			/* TRUE if started by sam-fsd	*/
extern	char	*Logfile_Path;		/* Logfile path			*/
extern	int	Log_Rollover;		/* Log rollover interval	*/
extern	int	Log_Expire;		/* Log inactivity expiration	*/
extern	int	Event_Interval;		/* Event interval time		*/
extern	int	Event_Buffer_Size;	/* Event buffer size		*/
extern	int	Event_Open_Retry;	/* Open syscall retry count	*/


/*
 * Read command file.
 */
int
read_cmd_file(char *FileName)
{
	int		errors;

	errors = ReadCfg(FileName, dirProcTable, dirname, token,
	    read_config_msg);
	if (errors == -1) {
		errors = 0;
	}
	return (errors);
}


/*
 * Handlers for the various keywords.
 */
static void
cmd_log_file(void)
{
	int		l;

	if (not_for_this_fs)
		return;

	l = strlen(token);
	SamMalloc(Logfile_Path, l+2);

	strcpy(Logfile_Path, token);
	if (*(Logfile_Path+l-1) == '/')
		*(Logfile_Path+l-1) = '\0';
}


static void
cmd_fs(void)
{
	if (strcmp(fs_name, token) == 0) {
		not_for_this_fs = FALSE;
	} else {
		not_for_this_fs = TRUE;
	}
}


static void
cmd_log_rollover(void)
{
	char		*endptr;
	int		intv;

	if (not_for_this_fs) {
		return;
	}
	intv = strtol(token, &endptr, 10);
	if (endptr && *endptr != '\0') {
		ReadCfgError(8020, token, fs_name);
		return;
	}

	Log_Rollover = intv;
}

static void
cmd_log_expire(void)
{
	char		*endptr;
	int		intv;

	if (not_for_this_fs) {
		return;
	}
	intv = strtol(token, &endptr, 10);
	if (endptr && *endptr != '\0') {
		ReadCfgError(8020, token, fs_name);
		return;
	}

	Log_Expire = intv;
}

static void
cmd_event_interval(void)
{
	char		*endptr;
	int		intv;

	if (not_for_this_fs) {
		return;
	}
	intv = strtol(token, &endptr, 10);
	if (endptr && *endptr != '\0') {
		ReadCfgError(8020, token, fs_name);
		return;
	}
	Event_Interval = intv;
}


static void
cmd_event_buffer(void)
{
	char		*endptr;
	int		size;

	if (not_for_this_fs) {
		return;
	}
	size = strtol(token, &endptr, 10);
	if (endptr && *endptr != '\0') {
		ReadCfgError(8020, token, fs_name);
		return;
	}
	Event_Buffer_Size = size;
}


static void
cmd_open_retry(void)
{
	char		*endptr;
	int		intv;

	if (not_for_this_fs) {
		return;
	}
	intv = strtol(token, &endptr, 10);
	if (endptr && *endptr != '\0') {
		ReadCfgError(8020, token, fs_name);
		return;
	}
	Event_Open_Retry = intv;
}


/*
 *  Message handler for ReadCfg module.
 */
static void
read_config_msg(
	char *msg,
	int lineno,
	char *line)
{
	if (line != NULL) {
		static boolean_t line_printed = FALSE;

		if (msg != NULL) {
			if (!line_printed) {
				printf("%d: %s\n", lineno, line);
			}
			fprintf(stderr, " *** %s\n", msg);
		} else {
			/*
			 * Informational messages.
			 */
			if (lineno == 0) {
				printf("%s\n", line);
			} else {
				printf("%d: %s\n", lineno, line);
				line_printed = TRUE;
				return;
			}
		}
		line_printed = FALSE;
	} else if (lineno >= 0) {
		/* A missing command file is not an error */
		if (Daemon) {
			SendCustMsg(HERE, 8023, msg);
		} else {
			fprintf(stderr, "%s\n", msg);
		}
	}
}
