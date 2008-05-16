/*
 * readcmd.c - read commands file.
 * Reads the recycler commands file and constructs the actions.
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
 *
 */

#pragma ident "$Revision: 1.30 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

/* Solaris headers. */
#include <libgen.h>
#include <syslog.h>
#include <sys/shm.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "aml/device.h"
#include "sam/lib.h"
#include "sam/readcfg.h"
#include "aml/robots.h"
#include "sam/lint.h"

/* Local headers. */
#define	ARCHSET_OWNER
#include "recycler.h"

/* Private data. */
static char dirName[TOKEN_SIZE];
static char token[TOKEN_SIZE];
static ROBOT_TABLE *robot;

/* Directive functions. */
static void dirLogfile(void);
static void dirNoRecycle(void);
static void dirRobot(void);
static void dirScript(void);

/* Directives table */
static DirProc_t directives[] = {
	{ "logfile",	dirLogfile,	DP_value },
	{ "no_recycle",	dirNoRecycle,	DP_other },
	{ "script",	dirScript,	DP_value },
	{ NULL,		dirRobot,	DP_other }
};

static char *cmdFname = SAM_CONFIG_PATH "/" RY_CMD;
static int errors;

static char *recycler_script = NULL;

/* Private functions. */
static void msgFunc(char *msg, int lineno, char *line);


/*
 * Read command file
 */
void
Readcmd(void)
{
	/*
	 * Read the command file.
	 */
	errors = ReadCfg(cmdFname, directives, dirName, token, msgFunc);
	if (errors > 0) {
		emit(TO_ALL, LOG_ERR, 2054);
		(void) PostEvent(RY_CLASS, "CmdErr", 2054, LOG_ERR,
		    GetCustMsg(2054), NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);
		exit(EXIT_FAILURE);
	}
}


/*
 * Returns path to script file.
 */
char *
GetScript(void)
{
	if (recycler_script == NULL) {
		char buf[512];

		sprintf(buf, "%s/recycler.sh", SAM_SCRIPT_PATH);
		SamStrdup(recycler_script, buf);
	}
	return (recycler_script);
}


/*
 * Process "logfile = <filename>"
 */
static void
dirLogfile(void)
{
	if (log != NULL) {
		fclose(log);
	}
	log = freopen(token, "a+", stdout);
	if (log == NULL) {
		ReadCfgError(CustMsg(20202), token, errtext);
	}
}


/*
 * Process "script = <filename>"
 */
static void
dirScript(void)
{
	if (recycler_script != NULL) {
		SamFree(recycler_script);
	}
	SamStrdup(recycler_script, token);
}

/*
 * Process "no_recycle <media> [<regexp> ...]
 */
static void
dirNoRecycle(void)
{
	int medium;

	if (ReadCfgGetToken() == 0) {
		return;
	}
	medium = nm_to_device(token);
	while (ReadCfgGetToken() != 0) {
		struct no_recycle *this_one;
		char *RegExp;

		if ((RegExp = regcmp(token, NULL)) == NULL) {
			ReadCfgError(CustMsg(20268), token);
		}
		SamMalloc(this_one, sizeof (struct no_recycle));
		this_one->regexp = RegExp;
		this_one->next = no_recycle;
		this_one->medium = medium;

		no_recycle = this_one;
	}
}

/*
 * Set high water mark for recycling a library
 */
static void
paramsHWM(void)
{
	int high;

	if (sscanf(token, "%d", &high) != 1 || high < 0 || high > 100) {
		/* -hwm value is malformed or out of range. */
		ReadCfgError(CustMsg(20313));
	}
	robot->high = high;
}

/*
 * Set minimum VSN gain for selecting a volume for recycling
 */
static void
paramsMingain(void)
{
	int min;

	if (sscanf(token, "%d", &min) != 1 || min < 0 || min > 100) {
		/* -mingain value is malformed or out of range. */
		ReadCfgError(CustMsg(20314));
	}
	robot->min = min;
}

/*
 * Set maximum data volume for a recycling library
 */
static void
paramsDataquantity(void)
{
	uint64_t dataquantity;

	if (StrToFsize(token, &dataquantity) == -1) {
		ReadCfgError(CustMsg(4441), "-dataquantity");
	}
	robot->dataquantity = dataquantity;
}

/*
 * Set maximum count of volumes to schedule for a recycling library
 */
static void
paramsVsncount(void)
{
	int vsncount;

	if (sscanf(token, "%d", &vsncount) != 1 || vsncount < 0) {
		/* vsncount value is malformed or out of range. */
		ReadCfgError(CustMsg(20318));
	}
	robot->vsncount = vsncount;
}

/*
 * Set recycle-ignore for a library
 */
static void
paramsIgnore(void)
{
	robot->ignore = TRUE;
}

/*
 * Set mail address
 */
static void
paramsMail(void)
{
	char	*mailaddress;

	SamMalloc(mailaddress, strlen(token)+1);
	strcpy(mailaddress, token);
	robot->mailaddress = mailaddress;
	robot->mail = TRUE;
}

/*
 * Process "<robot> <high-water> <min-gain> [<options> ...]"
 *  (old (pre-4.0) syntax)
 */
static void
dirRobot_pre40(void)
{

	/* options to be stored into robot table */
	char	*mailaddress;
	int	min, high;
	int	ignore;
	int	mail;

	/* "high" parameter should be in token when we get here ?? */
	if (sscanf(token, "%d", &high) != 1 || high < 0 || high > 100) {
		/* High-water mark is malformed or out of range. */
		ReadCfgError(CustMsg(20313));
	}
	if (ReadCfgGetToken() == 0) {
		/* Line too short */
		ReadCfgError(CustMsg(20312));
	}
	if (sscanf(token, "%d", &min) != 1 || min < 0 || min > 100) {
		/* Min-gain value is malformed or out of range. */
		ReadCfgError(CustMsg(20314));
	}

	/*
	 * Option processing.
	 */
	ignore = FALSE;
	mail = FALSE;
	mailaddress = NULL;

	while (ReadCfgGetToken() != 0) {
		if (strcmp(token, "ignore") == 0 && ignore == 0) {
			ignore = TRUE;
		} else if (strcmp(token, "mail") == 0 && mail == 0) {
			if (ReadCfgGetToken() != 0) {
				mail = TRUE;
				SamMalloc(mailaddress, strlen(token)+1);
				strcpy(mailaddress, token);
			}
		} else {
			ReadCfgError(CustMsg(20315), token);
		}
	}

	/*
	 * robot (a global variable) should have been set in dirRobot--
	 * before dirRobot_pre40 got called.
	 */
	robot->min = min;
	robot->high = high;
	robot->ignore = ignore;
	robot->mail = mail;
	robot->mailaddress = mailaddress;

}

/*
 * Process robot-family-set line
 */
static void
dirRobot(void)
{
	static DirProc_t table[] = {
		{ "-hwm",	paramsHWM,	DP_param,	4499 },
		{ "-mingain",	paramsMingain,	DP_param,	4501 },
		{ "-dataquantity", paramsDataquantity, DP_param, 4498 },
		{ "-vsncount",	paramsVsncount, DP_param,	4515 },
		{ "-ignore",	paramsIgnore,	DP_other },
		{ "-mail",	paramsMail,	DP_param,	20317 },
		{ NULL,		dirRobot_pre40, DP_other }
	};

	boolean_t noparams = TRUE;	/* were there any params on the line? */
	int i;

	/*
	 * Find the robot mentioned in the recycler.cmd line.
	 */
	for (i = 0; i < ROBOT_count; i++) {

		robot = &ROBOT_table[i];
		if (strcmp(family_name(robot), dirName) == 0) {
			break;
		}
	}
	if (i == ROBOT_count) {
		/* robot not found in mcf "No such library exists in system" */
		ReadCfgError(CustMsg(20316), dirName);
	}
	robot->ignore = FALSE;

	while (ReadCfgGetToken() != 0) {
		noparams = FALSE;
		ReadCfgLookupDirname(token, table);
	} /* end while */

	if (noparams) {
		/* Line too short */
		ReadCfgError(CustMsg(20312));
	}

}

/*
 * Process command file processing message.
 */
static void
msgFunc(
	char *msg,
	int lineno,
	char *line)
{
	if (line != NULL) {
		/*
		 * While reading the file.
		 */
		if (msg != NULL) {
			/*
			 * Error message.
			 */
			emit(TO_ALL, LOG_ERR, 20217, cmdFname, lineno, line);
			emit(TO_SYS|TO_TTY, LOG_ERR, 0, msg);
		}
	}
}
