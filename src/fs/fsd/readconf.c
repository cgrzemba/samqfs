/*
 * readconf.c - Read filesystem configuration file.
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

#pragma ident "$Revision: 1.38 $"

/* ANSI C headers. */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Solaris headers. */
#include <sys/mount.h>

/* SAM-FS headers. */
#include "sam/mount.h"
#include "sam/readcfg.h"

/* Local headers. */
#include "fsd.h"


/* Structures. */

/* Private functions. */
static void cmdFs(void);
static void cmdValue(void);
static void copyDefaults(void);
static void msgFunc(int code, char *msg);

/* Command table */
static DirProc_t dirProcTable[] = {
	{ "fs",	cmdFs,		DP_value },
	{ NULL, cmdValue,	DP_setfield }	/* All others use setfield.c */
};

static struct sam_fs_info mp_all, *mp;

static char errMsg[256];
static char dirname[TOKEN_SIZE];
static char token[TOKEN_SIZE];

#include "sam/mount.hc"

/*
 * Read file system command file.
 */
void
ReadFsCmdFile(char *fscfg_name)
{
	char	*fname = SAM_CONFIG_PATH "/samfs.cmd";
	int		errors;

	if (fscfg_name != NULL) {
		fname = fscfg_name;
	}
	mp = &mp_all;
	memset(&mp_all, 0, sizeof (mp_all));
	SetFieldDefaults(&mp_all, MountParams);
	if (QfsOnly) {
		mp->fi_config &= ~MT_SAM_ENABLED;
	}
	errors = ReadCfg(fname, dirProcTable, dirname, token, ConfigFileMsg);
	if (errors != 0 && !(errors == -1 && fscfg_name == NULL)) {
		/* Absence of the default config file is not an error. */
		if (errors > 0) {
			errno = 0;
		}
		/* Problem with file system command file. */
		FatalError(17218);
	}
	if (mp == &mp_all) {
		copyDefaults();
	}
}


/*
 * Processors for the various keywords.
 */


/*
 * fs =
 */
static void
cmdFs(void)
{
	int		n;

	for (n = 0; strcmp(token, FileSysTable[n].params.fi_name) != 0; n++) {
		if (n >= FileSysNumof)
			ReadCfgError(0, "Filesystem %s not defined", token);
	}
	if (mp == &mp_all) {
		copyDefaults();
	}
	mp = &FileSysTable[n].params;
}


/*
 * cmd = value
 */
static void
cmdValue(void)
{
	/*
	 * Set cmd (= value).
	 */
	if (SetFieldValue(mp, MountParams, dirname, token, msgFunc) != 0) {
		if (errno == ENOENT) {
			/* \"%s\" is not a recognized directive name */
			ReadCfgError(14005, dirname);
		}
		ReadCfgError(0, errMsg);
	}
}


/*
 * Copy defaults.
 */
static void
copyDefaults(void)
{
	int		i;

	for (i = 0; i < FileSysNumof; i++) {
		int		offset;
		uint32_t saved_flags;

		FileSysTable[i].params.fi_config = mp_all.fi_config;
		saved_flags = FileSysTable[i].params.fi_config1 & MC_FSCONFIG;
		FileSysTable[i].params.fi_config1 = mp_all.fi_config1;
		FileSysTable[i].params.fi_config1 |= saved_flags;

		/*
		 * Real careful - only replace the changeable parameters.
		 */
		offset = offsetof(struct sam_fs_info, fi_mflag);
		memmove((char *)&FileSysTable[i].params + offset,
		    (char *)&mp_all + offset,
		    sizeof (struct sam_fs_info) - offset);
	}
}


/*
 * Data field error message function.
 */
static void
msgFunc(
/*LINTED argument unused in function */
	int code,
	char *msg)
{
	strncpy(errMsg, msg, sizeof (errMsg)-1);
}
