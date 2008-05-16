/*
 * readcmd.c - read and parse rft server's command file
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

#pragma ident "$Revision: 1.9 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 *  ANSI C headers.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * POSIX headers.
 */
#include <sys/param.h>

/*
 * SAM-FS headers.
 */
#include "sam/types.h"
#include "sam/names.h"
#include "aml/parsecmd.h"
#include "sam/lib.h"
#include "sam/readcfg.h"
#include "sam/sam_trace.h"
#include "sam/custmsg.h"
#include "aml/sam_rft.h"

/*
 * Local headers.
 */
#include "rft_defs.h"

static void setSimpleParam();
static void setBlksizeParams();
static void setTcpWindowsizeParam();
static void setfieldErrmsg(int code, char *msg);
static void readcfgErrmsg(char *msg, int lineno, char *line);

/*
 * Static declarations
 */
#include "aml/sam_rft.hc"

/*
 * Rft's command table.
 */
static DirProc_t commands[] = {
	{ "blksize",		setBlksizeParams,		DP_value },
	{ "cmd_bufsize",	setSimpleParam,			DP_value },
	{ "logfile",		setSimpleParam,			DP_value },
	{ "tcpwindow",		setTcpWindowsizeParam,	DP_value },
	{ NULL, NULL }
};

static sam_rft_config_t config;

static char cfgName[TOKEN_SIZE];
static char cfgToken[TOKEN_SIZE];

/*
 * Read rft command file.
 */
void
ReadCmds(void)
{
	extern boolean_t Daemon;
	char *fname;
	upath_t fullpath;

	(void) memset(&config, 0, sizeof (config));
	SetFieldDefaults(&config, samrftDefaultConfig);

	(void) sprintf(fullpath, "%s/%s", SAM_CONFIG_PATH, COMMAND_FILE_NAME);
	fname = &fullpath[0];

	(void) ReadCfg(fname, commands, cfgName, cfgToken, readcfgErrmsg);

	/*
	 *	If not started by init, print configuration information.
	 */
	if (Daemon == FALSE) {
		(void) printf("blksize = %d\n", config.blksize);
		(void) printf("tcpwindow = %d\n", config.tcpwindow);
		(void) printf("cmd_bufsize = %d\n", config.cmd_bufsize);
		if (config.logfile != NULL && *config.logfile != '\0') {
			(void) printf("logfile = %s\n", config.logfile);
		}
	} else {
		Trace(TR_MISC, "Block size configuration= %d", config.blksize);
		Trace(TR_MISC, "TCP window size configuration= %d",
		    config.tcpwindow);
	}
}

/*
 * Get log file name configuration parameter.
 */
char *
GetCfgLogFile(void)
{
	return (config.logfile);
}

/*
 * Get blksize configuration parameter.
 */
int
GetCfgBlksize(void)
{
	return (config.blksize);
}

int
GetCfgTcpWindowsize(void)
{
	return (config.tcpwindow);
}

/*
 * Set simple, one value, parameter from rft's command file.
 */
static void
setSimpleParam(void)
{
	(void) SetFieldValue(&config, samrftDefaultConfig,
	    cfgName, cfgToken, setfieldErrmsg);
}

/*
 * Set data blksize parameters from rft daemon's configuration file.
 */
static void
setBlksizeParams(void)
{
	uint64_t value;

	/*
	 * Copy strip size to daemon's parameters.
	 */
	if (cfgToken != NULL && *cfgToken != '\0') {
		if (strcmp(cfgToken, "-") == 0) {
			ReadCfgError(CustMsg(22017), "blksize");
			/* NOTREACHED */
		}
		if (StrToFsize(cfgToken, &value) != 0) {
			ReadCfgError(CustMsg(22017), "blksize");
			/* NOTREACHED */
		}
		if (value > INT_MAX) {
			ReadCfgError(CustMsg(22017), "blksize");
			/* NOTREACHED */
		}
		config.blksize = value;

	} else {
		ReadCfgError(CustMsg(22017), "blksize");
	}
}

static void
setTcpWindowsizeParam(void)
{
	uint64_t value;

	if (StrToFsize(cfgToken, &value) == -1) {
		ReadCfgError(CustMsg(22017), "tcpwindow");
	}
	config.tcpwindow = value;
}

/*
 * Error handler for SetField function.
 */
static void
setfieldErrmsg(
	int code,
	char *msg)
{
	SendCustMsg(HERE, code, msg);
}

/*
 * Error handler for ReadCfg function.
 */
static void
readcfgErrmsg(
	char *msg,
	int lineno,
	char *line)
{
	/*
	 * ReadCfg function sends every line here.
	 * If msg string is NULL, its not an error.
	 */
	if (line != NULL) {
		if (msg != NULL) {
			SendCustMsg(HERE, 22014, lineno, line, msg);
		}
	} else if (lineno > 0) {
		/*
		 * Missing or empty command file is not an error.
		 */
		SendCustMsg(HERE, 22013, msg);
	}
}
