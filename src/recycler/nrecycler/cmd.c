/*
 * cmd.c - command file processing.
 */
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
#pragma ident "$Revision: 1.5 $"

static char *_SrcFile = __FILE__;

#include "recycler_c.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>

#include "sam/types.h"
#include "sam/names.h"
#include "sam/readcfg.h"
#include "sam/sam_trace.h"
#include "sam/sam_malloc.h"
#include "sam/custmsg.h"
#include "sam/fs/ino.h"
#include "aml/device.h"
#include "aml/diskvols.h"

#include "recycler.h"

/* Recycler's command directive table. */
static void dirNodumps();
static void dirDumps();
static void dirEnddumps();
static void procDumps();
static void setLogfileParam();

static DirProc_t commands[] = {
	{ "endsamfsdumps",	dirNodumps,		DP_set },
	{ "samfsdumps",		dirDumps,		DP_set },
	{ "logfile",		setLogfileParam,	DP_value },
	{ NULL,			NULL }
};

/* Samfsdump file table. */
static struct {
	int count;
	char **path;
} samfsdumpTable = { 0, NULL };

static char cfgName[TOKEN_SIZE];
static char cfgToken[TOKEN_SIZE];
static char *noEnd = NULL;
static char *logfilePath = NULL;
static char *scriptPath = NULL;

static void cfgErrmsg(char *msg, int lineno, char *line);
static boolean_t verifyFile(char *file);

/*
 * Read recycler command file.
 */
void
CmdReadfile(void)
{
	int errors;
	upath_t cmdpath;

	(void) sprintf(cmdpath, "%s/%s", SAM_CONFIG_PATH, RECYCLER_CMD_FILNAME);

	errors = ReadCfg(cmdpath, commands, cfgName, cfgToken, cfgErrmsg);
	if (errors > 0) {
		SendCustMsg(HERE, 2054);
		exit(EXIT_FAILURE);
	}
}

/*
 * Get configured logfile path name.
 */
char *
CmdGetLogfilePath(void)
{
	return (logfilePath);
}

/*
 * Get configured samfs dump path name.
 */
char *
CmdGetDumpPath(
	int idx)
{
	if (idx >= samfsdumpTable.count) {
		return (NULL);
	}

	return (samfsdumpTable.path[idx]);
}

/*
 * Get configured path to recycler's relabel script.
 */
char *
CmdGetScriptPath(void)
{
	if (scriptPath == NULL) {
		char buf[MAXPATHLEN];

		(void) snprintf(buf, sizeof (buf), "%s/%s",
		    SAM_SCRIPT_PATH, RECYCLER_SCRIPT_FILNAME);
		SamStrdup(scriptPath, buf);
	}

	return (scriptPath);
}

/*
 * Set logfile parameter from recycler's command file.
 */
static void
setLogfileParam(void)
{
	if (cfgToken != NULL && *cfgToken != '\0') {
		if (verifyFile(cfgToken) == B_FALSE) {
			ReadCfgError(CustMsg(20407), "log", cfgToken);
		}
	}

	if (logfilePath != NULL) {
		SamFree(logfilePath);
	}
	SamStrdup(logfilePath, cfgToken);
}

/*
 * Error if found an 'endsamfsdumps' but no 'samfsdumps' statement.
 */
static void
dirNodumps(void)
{
	ReadCfgError(CustMsg(20408), cfgName + 3);
}

/*
 * 'samfsdumps' statement.
 */
static void
dirDumps(void)
{
	static DirProc_t sectionCommands[] = {
		{ "endsamfsdumps",	dirEnddumps,	DP_set   },
		{ NULL,			procDumps,	DP_other },
	};

	char *msg;

	ReadCfgSetTable(sectionCommands);
	msg = noEnd;
	noEnd = "nosamfsdumps";
	if (msg != NULL) {
		ReadCfgError(CustMsg(20409), msg);
	}
}

/*
 * 'endsamfsdumps' statement.
 */
static void
dirEnddumps(void)
{
	ReadCfgSetTable(commands);
	noEnd = NULL;
}

/*
 * Process locations for samfsdump files.
 */
static void
procDumps(void)
{
	int idx;

	idx = samfsdumpTable.count;
	if (idx == 0) {
		SamMalloc(samfsdumpTable.path, sizeof (char *));
	} else {
		SamRealloc(samfsdumpTable.path, (idx + 1) * sizeof (char *));
	}
	SamStrdup(samfsdumpTable.path[idx], cfgToken);
	samfsdumpTable.count++;
}

/*
 * Error handler for ReadCfg function.
 */
static void
cfgErrmsg(
	char *msg,
	int lineno,
	char *line)
{
	/*
	 * ReadCfg function sends every line here.
	 * If msg string is NULL its not an error.
	 * ReadCfg function also issues messages before and after
	 * processing the configuration file.  If line is NULL.
	 */
	if (line != NULL) {
		if (msg != NULL) {
			/* Recycler command error. */
			SendCustMsg(HERE, 20411, lineno, line, msg);
		}
	} else if (lineno >= 0) {
		/*
		 * Missing command file is not an error.
		 */
		SendCustMsg(HERE, 20412, msg);
	}
}

/*
 * Verify that a file exists or can be created.
 */
static boolean_t
verifyFile(
	char *file)
{
	char *dupfile;
	char *path;
	struct stat buf;
	int rc;
	boolean_t rval = B_FALSE;

	/*
	 * Must be a fully qualified path name.
	 */
	if (*file == '/') {
		/*
		 * A file that already exists is good.
		 */
		if (stat(file, &buf) == 0) {
			rval = B_TRUE;

		} else {
			/*
			 * Check if valid directory.
			 */
			SamStrdup(dupfile, file);
			path = dirname(dupfile);
			rc = stat(path, &buf);
			SamFree(dupfile);
			if (rc == 0) {
				rval = B_TRUE;
			}
		}
	}
	return (rval);
}
