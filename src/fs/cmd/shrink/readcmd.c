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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.2 $"

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

/* Local headers. */
#include "shrink.h"

#include "sam/lint.h"

/*
 * Private functions.
 */
static void cmd_fs(void);
static void cmd_log_file(void);
static void display_all(void);
static void do_not_execute(void);
static void number_of_streams(void);
static void block_size(void);
static void stage_files(void);
static void stage_partial(void);
static void read_config_msg(char *msg, int lineno, char *line);

/*
 * Command table
 */
static DirProc_t dirProcTable[] = {
	{ "fs",				cmd_fs,			DP_value },
	{ "logfile",			cmd_log_file,		DP_value },
	{ "do_not_execute",		do_not_execute,		DP_set },
	{ "display_all_files",		display_all,		DP_set },
	{ "streams",			number_of_streams,	DP_value },
	{ "block_size",			block_size,		DP_value },
	{ "stage_files",		stage_files,		DP_set },
	{ "stage_partial",		stage_partial,		DP_set },
	{ NULL,	NULL }
};

static char dirname[TOKEN_SIZE];
static char token[TOKEN_SIZE];
static boolean_t not_for_this_fs = FALSE;

/*
 * Externals to communicate with main().
 */
extern char *program_name;
extern char *fs_name;		/* File system name */
extern boolean_t Daemon;	/* TRUE if started by sam-fsd */
extern control_t control;	/* Work queue ptrs & thread params */


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
	if (not_for_this_fs)
		return;

	strncpy(control.log_pathname, token, sizeof (control.log_pathname));
	control.log_pathname[sizeof (control.log_pathname)-1] = '\0';
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
display_all(void)
{
	if (not_for_this_fs) {
		return;
	}
	control.display_all_files = TRUE;
}


static void
do_not_execute(void)
{
	if (not_for_this_fs) {
		return;
	}
	control.do_not_execute = TRUE;
}


static void
block_size(void)
{
	char *endptr;
	int block_size;

	if (not_for_this_fs) {
		return;
	}
	block_size = strtol(token, &endptr, 10);
	if (endptr && *endptr != '\0') {
		ReadCfgError(8020, token, fs_name);
		return;
	}
	block_size = 1;
	if (block_size < 1) {
		block_size = 1;
	}
	if (block_size > 16) {
		block_size = 16;
	}
	control.block_size = block_size;
}


static void
number_of_streams(void)
{
	char *endptr;
	int streams;

	if (not_for_this_fs) {
		return;
	}
	streams = strtol(token, &endptr, 10);
	if (endptr && *endptr != '\0') {
		ReadCfgError(8020, token, fs_name);
		return;
	}
	if (streams < 1) {
		streams = 1;
	}
	if (streams > 128) {
		streams = 128;
	}
	control.streams = streams;
}


static void
stage_files(void)
{
	if (not_for_this_fs) {
		return;
	}
	control.stage_files = TRUE;
}


static void
stage_partial(void)
{
	if (not_for_this_fs) {
		return;
	}
	control.stage_partial = TRUE;
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
