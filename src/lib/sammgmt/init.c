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
#pragma ident   "$Revision: 1.35 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */


/*
 * init.c
 * contains functions to check and update the initialization status of the
 * configuration files.
 */

#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/vfstab.h>

#include "sam/lib.h"
#include "mgmt/config/init.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/mgmt.h" /* for initialize */
#include "pub/mgmt/error.h"
#include "pub/mgmt/types.h"
#include "pub/version.h"
#include "mgmt/util.h"
#include "sam/sam_trace.h"


/* file names being used for each of the config files. */
static char *mcf_file = MCF_CFG;
static char *samfs_cmd_file = SAMFS_CFG;
static char *archiver_file = ARCHIVER_CFG;
static char *diskvols_file = DISKVOL_CFG;
static char *stager_file = STAGE_CFG;
static char *releaser_file = RELEASE_CFG;
static char *recycler_file = RECYCLE_CFG;


static int get_time(char *file_name, time_t *time);

/*
 * Get the SAMFS product version number.
 */
char *
get_samfs_version(
ctx_t *ctx /* ARGSUSED */)
{

	return (SAM_VERSION);
}


/*
 * Get the SAMFS management library version number.
 */
char *
get_samfs_lib_version(
ctx_t *ctx /* ARGSUSED */)
{

	return (API_VERSION);
}


/*
 * get the modify time of the named file and return it.
 */
static int
get_time(
char *file_name,	/* file to get time for */
time_t *time)		/* mod time */
{

	struct stat cfg_stat;
	char buf[MAX_MSG_LEN];

	if (stat(file_name, &cfg_stat) != 0) {

		/* if file does not exist no error */
		if (errno == ENOENT) {
			*time = 0;
		} else {
			/* if something else wrong? */
			StrFromErrno(errno, buf, sizeof (buf));
			samerrno = SE_INIT_FAILED;

			/* Init of %s failed:%s */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_INIT_FAILED), file_name, buf);

			Trace(TR_DEBUG, "get time for %s failed: %s",
			    file_name, samerrmsg);

			return (-1);
		}

	} else {
		*time = cfg_stat.st_mtime;
	}

	return (0);

}


/*
 * returns a malloced string containing the host name returned by
 * the function gethostname.
 */
int
get_host_name(
ctx_t *c /* ARGSUSED */,
char **nm)
{
	char hostname[MAXHOSTNAMELEN];

	if (ISNULL(nm)) {
		return (-1);
	}

	gethostname(hostname, MAXHOSTNAMELEN);
	*nm = strdup(hostname);
	return (0);
}


/*
 * Gets file mod time for a single file.  Do not mask multiples in which_cfg.
 * 0 time indicates the file does not exist.
 * any other time is the mod time.
 * -1 return indicates unidentified configuration.
 */
int
get_file_time(int32_t which_cfg, time_t *t) {

	int rval = 0;

	switch (which_cfg) {
		case INIT_MCF: {
			rval = get_time(mcf_file, t);
			break;
		}
		case INIT_SAMFS_CMD: {
			rval = get_time(samfs_cmd_file, t);
			break;
		}
		case INIT_ARCHIVER_CMD: {
			rval = get_time(archiver_file, t);
			break;
		}
		case INIT_DISKVOLS_CONF: {
			rval = get_time(diskvols_file, t);
			break;
		}
		case INIT_STAGER_CMD: {
			rval = get_time(stager_file, t);
			break;
		}
		case INIT_RELEASER_CMD: {
			rval = get_time(releaser_file, t);
			break;
		}
		case INIT_RECYCLER_CMD: {
			rval = get_time(recycler_file, t);
			break;
		}
		case INIT_DEFAULTS_CONF: {
			rval = get_time(DEFAULTS_CFG, t);
			break;
		}
		case INIT_VFSTAB_CFG: {
			rval = get_time(VFSTAB, t);
			break;
		}
		default: {
			char cfgnum[12];
			samerrno = SE_NOT_FOUND;
			sprintf(cfgnum, "%d", which_cfg);
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_NOT_FOUND), cfgnum);
			rval = -1;
			break;
		}
	} /* end switch */

	return (rval);
}

/* globals required for server init */

extern char *program_name = NULL;

/*
 *  Initialize all variables and infrastructure pieces required
 *  by the users of libfsmgmtd.  This function is not thread-safe
 *  and must be called before the program registers as an RPC
 *  service and/or creates any threads.
 */
void
init_utility_funcs(void)
{
	/* if program name isn't set, should probably be a fatal error */
	if (program_name == NULL) {
		program_name = strdup("fsmgmtd");
	}

	/* Enable Tracing */
	TraceInit(program_name, TI_mgmtapi);

	/* initialize the message catalog service */
	CustmsgInit(0, NULL);
}
