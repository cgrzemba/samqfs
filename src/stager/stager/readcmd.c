/*
 * readcmd.c - read and parse stager's command file
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

#pragma ident "$Revision: 1.36 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * ANSI C headers.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * POSIX headers.
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * Solaris headers.
 */
#include <libgen.h>

/*
 * SAM-FS headers.
 */
#include "sam/types.h"
#include "sam/names.h"
#include "aml/parsecmd.h"
#include "sam/lib.h"
#include "sam/readcfg.h"
#include "sam/sam_malloc.h"
/* #define NEED_TR_NAMES */
#include "sam/sam_trace.h"
#include "sam/custmsg.h"
#include "aml/stager.h"
#include "aml/stager_defs.h"
#include "sam/custmsg.h"

/*
 * Local headers.
 */
#include "stager_config.h"
#include "rmedia.h"

static void setSimpleParam();
static void setBufsizeParams();
static void setDrivesParams();
static void setLogfileParam();
static void setMaxActiveDefault(void *v);
static int setMaxActiveParam(void *v, char *value, char *buf, int bufsize);
static void setDirectioParam();
static void setfieldErrmsg(int code, char *msg);
static void readcfgErrmsg(char *msg, int lineno, char *line);
static boolean_t verifyFile(char *file);

#include "aml/stager.hc"

/*
 * Stager's command table.
 */
static DirProc_t commands[] = {
	{ "maxactive",		setSimpleParam,			DP_value },
	{ "maxretries",		setSimpleParam,			DP_value },
	{ "logfile",		setLogfileParam,		DP_value },
	{ "bufsize",		setBufsizeParams,		DP_value },
	{ "drives",		setDrivesParams,		DP_value },
	{ "directio",		setDirectioParam,		DP_value },
	{ NULL, NULL }
};

static sam_stager_config_t config;

static char cfgName[TOKEN_SIZE];
static char cfgToken[TOKEN_SIZE];

/*
 * Read stager command file.
 */
void
ReadCmds(void)
{
	upath_t fullpath;

	(void) sprintf(fullpath, "%s/%s", SAM_CONFIG_PATH, COMMAND_FILE_NAME);

	(void) memset(&config, 0, sizeof (config));
	SetFieldDefaults(&config, stagerDefaultConfig);

	(void) ReadCfg(fullpath, commands, cfgName, cfgToken, readcfgErrmsg);
}

/*
 * Get log file name configuration parameter.
 */
char *
GetCfgLogFile(void)
{
	return (config.logfile.name);
}

/*
 * Get log file events configuration parameter.
 */
int
GetCfgLogEvents(void)
{
	return (config.logfile.events);
}

/*
 * Get max active configuration parameter.
 */
long
GetCfgMaxActive(void)
{
	return (config.maxactive);
}

/*
 * Get max retries configuration parameter.
 */
long
GetCfgMaxRetries(void)
{
	return (config.maxretries);
}

/*
 * Get directio directive
 */
int
GetCfgDirectio(void)
{
	return (config.directio);
}

/*
 * Get number of drives configuration parameters.
 */
int
GetCfgNumDrives(void)
{
	return (config.num_drives);
}

/*
 * Get drives configuration parameters.
 */
sam_stager_drives_t *
GetCfgDrives(void)
{
	return (config.drives);
}

/*
 * Set simple, one value, parameter from stager's command file.
 */
static void
setSimpleParam(void)
{
	(void) SetFieldValue(&config, stagerDefaultConfig,
	    cfgName, cfgToken, setfieldErrmsg);
}

/*
 * Set bufsize parameters from stager's command file.
 */
static void
setBufsizeParams(void)
{
	static char *keyword = "bufsize";
	static int64_t minBufsize = 2;
	static int64_t maxBufsize = 8192;

	char *p;
	mtype_t media;

	/*
	 *  Copy media to stager's parameters.
	 */
	if (cfgToken != NULL && *cfgToken != '\0') {
		(void) strncpy(media, cfgToken, sizeof (media));
		config.bufsize.media = sam_atomedia(cfgToken);

		/*
		 * Media value processed.  Next token must be buffer size.
		 */
		(void) ReadCfgGetToken();

		if (*cfgToken != '\0') {
			p = cfgToken;
			config.bufsize.size = strtoll(cfgToken, &p, 0);

			if (config.bufsize.size == 0) {
				ReadCfgError(CustMsg(14101), keyword,
				    cfgToken);
			}

			if (config.bufsize.size < minBufsize &&
			    config.bufsize.size > maxBufsize) {
				ReadCfgError(CustMsg(14102), keyword,
				    minBufsize, maxBufsize);
			}

			if (config.bufsize.size < minBufsize) {
				ReadCfgError(CustMsg(14103), keyword,
				    minBufsize);
			}

			if (config.bufsize.size > maxBufsize) {
				ReadCfgError(CustMsg(14104), keyword,
				    maxBufsize);
			}

			if (ReadCfgGetToken() != 0) {
				if (strcmp(cfgToken, "lock") == 0) {
					config.bufsize.lockbuf = B_TRUE;
				} else {
					ReadCfgError(CustMsg(19034));
				}
			}

			SetMediaParamsBufsize(media, config.bufsize.size,
			    config.bufsize.lockbuf);

		} else {
			ReadCfgError(CustMsg(14008), keyword);
		}

	} else {
		ReadCfgError(CustMsg(14008), keyword);
	}

}

/*
 * Set drives parameters from stager's command file.
 */
static void
setDrivesParams(void)
{
	char *p;
	uname_t robot;
	int count;
	size_t size;
	int idx;

	if (cfgToken != NULL && *cfgToken != '\0') {
		(void) strncpy(robot, cfgToken, sizeof (uname_t));

		/*
		 * Robot name saved.  Next token must be a drive count.
		 */
		(void) ReadCfgGetToken();

		if (*cfgToken != '\0') {
			p = cfgToken;
			count = strtoll(cfgToken, &p, 0);

			config.num_drives++;
			size = config.num_drives * sizeof (sam_stager_drives_t);
			SamRealloc(config.drives, size);

			idx = config.num_drives - 1;
			(void) strcpy(config.drives[idx].robot, robot);
			config.drives[idx].count = count;
		}
	}
}

/*
 * Set logfile parameter from stager's command file.
 */
static void
setLogfileParam(void)
{
	if (cfgToken != NULL && *cfgToken != '\0') {
		if (verifyFile(cfgToken) == B_FALSE) {
			ReadCfgError(CustMsg(19018), "log", cfgToken);
		}

		if (strlen(cfgToken) > sizeof (config.logfile.name)-1) {
			ReadCfgError(CustMsg(19019),
			    sizeof (config.logfile.name)-1);
		}

		(void) strncpy(config.logfile.name, cfgToken,
		    sizeof (config.logfile.name));

		/*
		 * Log file name saved.  Next token must be the events to log.
		 */
		if (ReadCfgGetToken() == 0) {
			config.logfile.events = STAGER_LOGFILE_DEFAULT;

		} else {

			while (cfgToken != NULL && *cfgToken != '\0') {
				if (strcmp(cfgToken, "start") == 0) {
					config.logfile.events |=
					    STAGER_LOGFILE_START;

				} else if (strcmp(cfgToken, "finish") == 0) {
					config.logfile.events |=
					    STAGER_LOGFILE_FINISH;

				} else if (strcmp(cfgToken, "cancel") == 0) {
					config.logfile.events |=
					    STAGER_LOGFILE_CANCEL;

				} else if (strcmp(cfgToken, "error") == 0) {
					config.logfile.events |=
					    STAGER_LOGFILE_ERROR;

				} else if (strcmp(cfgToken, "all") == 0) {
					config.logfile.events =
					    STAGER_LOGFILE_START  |
					    STAGER_LOGFILE_FINISH |
					    STAGER_LOGFILE_CANCEL |
					    STAGER_LOGFILE_ERROR;
				} else {
					ReadCfgError(CustMsg(19028), cfgToken);
				}
				(void) ReadCfgGetToken();
			}
		}
	}
}

/*
 * Set the directio directive
 * on = 1, off = 0
 */
static void
setDirectioParam(void)
{

	if (cfgToken != NULL && *cfgToken != '\0') {
		if (strcmp(cfgToken, "on") == 0) {
			config.directio = 1;
		} else if (strcmp(cfgToken, "off") == 0) {
			config.directio = 0;
		} else {
			ReadCfgError(CustMsg(19038));
		}
	} else {
		ReadCfgError(CustMsg(14008), cfgToken);
	}

}

/*
 * Error handler for SetField function.
 */
static void
setfieldErrmsg(
	int code,
	char *msg)
{
	ReadCfgError(code, msg);
	/* SendCustMsg(HERE, code, msg); */
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
	 * If msg string is NULL its not an error.
	 * ReadCfg function also issues messages before and after
	 * processing the configuration file.  If line is NULL.
	 */
	if (line != NULL) {
		if (msg != NULL) {
			SendCustMsg(HERE, 19014, lineno, line, msg);
		}
	} else if (lineno >= 0) {
		/*
		 * Missing command file is not an error.
		 */
		SendCustMsg(HERE, 19013, msg);
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
	int ret;
	boolean_t rc = B_FALSE;

	/*
	 * Must be a fully qualified path name.
	 */
	if (*file == '/') {
		/*
		 * A file that already exists is good.
		 */
		if (stat(file, &buf) == 0) {
			rc = B_TRUE;

		} else {
			/*
			 * Check if valid directory.
			 */
			SamStrdup(dupfile, file);
			path = dirname(dupfile);
			ret = stat(path, &buf);
			SamFree(dupfile);
			if (ret == 0) {
				rc = B_TRUE;
			}
		}
	}
	return (rc);
}

/* Sets the default max_active based on the system memory size. */
static void
setMaxActiveDefault(void *v)
{
	long pages, pgsize;
	int64_t memsize;
	int maxactive;

	pages = sysconf(_SC_PHYS_PAGES);
	pgsize = sysconf(_SC_PAGESIZE);
	memsize = (int64_t)pages * (int64_t)pgsize;

	/* Number of GB (rounded nearest) * maxactive per GB */
	maxactive = (((memsize>>29)+1)>>1) * STAGER_MAX_ACTIVE_PER_GB;

	/* Make sure we calculated something that makes sense */
	if (maxactive < STAGER_MAX_ACTIVE_PER_GB) {
		maxactive = STAGER_MAX_ACTIVE_PER_GB;
	} else if (maxactive > STAGER_MAX_ACTIVE) {
		maxactive = STAGER_MAX_ACTIVE;
	}

	Trace(TR_PROC, "Setting maxactive default %d", maxactive);
	*(int *)v = maxactive;
}

/* Sets the max_active parameter based on stager.cmd user input */
static int
setMaxActiveParam(void *v, char *value, char *buf, int bufsize)
{
	int64_t	val;
	if (StrToFsize(value, (uint64_t *)&val) != 0) {
		/* Invalid '%s' value ('%s') */
		snprintf(buf, bufsize, GetCustMsg(14101), "maxactive", value);
		return (-1);
	}

	if (val < 1 || val > STAGER_MAX_ACTIVE) {
		/* '%s' value is out of range %lld to %lld */
		snprintf(buf, bufsize, GetCustMsg(14102), "maxactive",
		    (int64_t)1, (int64_t)STAGER_MAX_ACTIVE);
		return (-1);
	}

	*(int *)v = val;
	return (0);
}
