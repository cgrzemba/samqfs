/*
 * readcmd.c - Read command file for releaser.
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

#pragma ident "$Revision: 1.20 $"

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
#include "releaser.h"

#include "sam/lint.h"

/* Defines */

/* Structures. */

/* Private functions. */
static void CmdFs(void);
static void CmdLogfile(void);
static void Debug_partial(void);
static void Display_all(void);
static void MinResidenceAge(void);
static void No_release(void);
static void Rearch_no_release(void);
static void List_size(void);
static void Weight_age(void);
static void Weight_age_access(void);
static void Weight_age_modify(void);
static void Weight_age_residence(void);
static void Weight_size(void);
static float getfloat(void);
static void mixed_weights_used(void);
static void readcfgMsg(char *msg, int lineno, char *line);

/* Command table */
static DirProc_t dirProcTable[] = {
	{ "fs",				CmdFs,			DP_value },
	{ "logfile",			CmdLogfile,		DP_value },
	{ "list_size",			List_size,		DP_value },
	{ "min_residence_age",		MinResidenceAge,	DP_value },
	{ "weight_age",			Weight_age,		DP_value },
	{ "weight_age_access",		Weight_age_access,	DP_value },
	{ "weight_age_modify",		Weight_age_modify,	DP_value },
	{ "weight_age_residence",	Weight_age_residence,	DP_value },
	{ "weight_size",		Weight_size,		DP_value },
	{ "no_release",			No_release,		DP_set },
	{ "rearch_no_release",		Rearch_no_release,	DP_set },
	{ "display_all_candidates",	Display_all,		DP_set },
	{ "debug_partial",		Debug_partial,		DP_set },
	{ NULL,	NULL }
};

static char dirname[TOKEN_SIZE];
static char token[TOKEN_SIZE];

static boolean_t not_for_this_fs = FALSE;

/*  Stuff to communicate with main(). */
extern char *program_name;
extern char *fs_name;	/* File system name */
extern char log_pathname[MAXPATHLEN];
extern float weight_age;
extern float weight_age_access;
extern float weight_age_modify;
extern float weight_age_residence;
extern float weight_size;
extern int debug_partial;
extern int display_all_candidates;
extern int list_size;
extern int min_residence_age;
extern int release;
extern int rearch_release;
extern int use_one_age_weight;
extern int use_three_age_weights;


/*
 * Read command file.
 */
int
read_cmd_file(char *FileName)
{
	int		errors;

	errors = ReadCfg(FileName, dirProcTable, dirname, token, readcfgMsg);
	if (errors == -1) {
		errors = 0;
	}
	return (errors);
}


/*
 * Handlers for the various keywords.
 */

static void
List_size(void)
{
	char *endptr;

	if (not_for_this_fs)
		return;

	list_size = strtol(token, &endptr, 10);
	if (endptr && *endptr != '\0')  {
		/* Error converting "%s" to "%s" */
		ReadCfgError(8020, token, dirname);
	}
}

static void
MinResidenceAge(void)
{
	char *endptr;

	if (not_for_this_fs)
		return;

	min_residence_age = strtol(token, &endptr, 10);
	if (endptr && *endptr != '\0')  {
		/* Error converting "%s" to "%s" */
		ReadCfgError(8020, token, dirname);
	}
}


static void
Weight_age(void)
{
	if (not_for_this_fs)
		return;

	if (use_three_age_weights) mixed_weights_used();
	use_one_age_weight = TRUE;
	weight_age = getfloat();
}


static void
Weight_size(void)
{
	if (not_for_this_fs)
		return;

	weight_size = getfloat();
}


static void
Weight_age_access(void)
{
	if (not_for_this_fs)
		return;

	if (use_one_age_weight) mixed_weights_used();
	use_three_age_weights = TRUE;
	weight_age_access = getfloat();
}


static void
Weight_age_modify(void)
{
	if (not_for_this_fs)
		return;

	if (use_one_age_weight) mixed_weights_used();
	use_three_age_weights = TRUE;
	weight_age_modify = getfloat();
}


static void
Weight_age_residence(void)
{
	if (not_for_this_fs)
		return;

	if (use_one_age_weight) mixed_weights_used();
	use_three_age_weights = TRUE;
	weight_age_residence = getfloat();
}


static void
CmdLogfile(void)
{
	if (not_for_this_fs)
		return;

	strncpy(log_pathname, token, sizeof (log_pathname));
	log_pathname[sizeof (log_pathname)-1] = '\0';
}


static void
CmdFs(void)
{
	if (strcmp(fs_name, token) == 0) {
		not_for_this_fs = FALSE;
	} else {
		not_for_this_fs = TRUE;
	}
}


static void
Display_all(void)
{
	if (not_for_this_fs)
		return;

	display_all_candidates = TRUE;
}


static void
No_release(void)
{
	if (not_for_this_fs)
		return;

	release = FALSE;
}

static void
Rearch_no_release(void)
{
	if (not_for_this_fs)
		return;

	rearch_release = FALSE;
}


static void
Debug_partial(void)
{
	if (not_for_this_fs)
		return;

	debug_partial = TRUE;
}


/*
 *  See if the current token is a valid floating point value, and if it's
 *  between 1.0 and 0.0, inclusive.  Return the value if valid, or emit
 *  an error message if not.
 */
static float
getfloat(void)
{
	char	*p;
	double	value;

	errno = 0;
	value = strtod(token, &p);
	if (*p != '\0' || errno == ERANGE || errno == EINVAL) {
		/* Error converting \"%s\" to \"%s\" */
		ReadCfgError(8020, token, dirname);
	}
	if (value > 1.0 || value < 0.0) {
		/* value "%f" for "%s" must be >= 0.0 and <= 1.0  */
		ReadCfgError(8021, value, dirname);
	}
	return (value);
}


/*
 *  Emit an error message when the command file mixes
 *  weight_age and any of weight_age_access, _modify, or _residence
 */
static void
mixed_weights_used(void)
{
	/*
	 * "Cannot mix weight_age and any of weight_age_access,"
	 * "weight_age_modify, or weight_age_residence"
	 */
	ReadCfgError(8022);
}


/*
 *  Message handler for ReadCfg module.
 */
static void
readcfgMsg(char *msg, int lineno, char *line)
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
