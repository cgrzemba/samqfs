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
#pragma ident   "$Revision: 1.20 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */


/*
 * diskvols.c - Read disk volumes configuration file.
 *
 * ISSUES:
 *	a. host names are not checked
 *	b. paths are not checked even if local.
 *	c. No checks for vsn name uniqueness.
 */


/* ANSI C headers. */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* POSIX headers. */
#include <sys/param.h>
#include <fcntl.h>


/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/readcfg.h"
#include "sam/sam_trace.h"
#include "aml/diskvols.h"

/* mgmt headers */
#include "mgmt/config/cfg_diskvols.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/error.h"
#include "parser_utils.h"
#include "mgmt/util.h"


/* Directive processing functions. */
static void dirNoclients(void);
static void dirClients(void);
static void dirEndclients(void);
static void procClients(void);
static void cfgVolume(void);
static void cfgHCVolume(disk_vol_t *dv);

/* Functions new to API */
static void read_cfg_msg(char *msg, int lineno, char *line);
static int init_static_variables(void);


/* Private data. */
/* Directives table */
static DirProc_t dirProcTable[] = {
	{ "endclients",	dirNoclients,	DP_set },
	{ "clients",	dirClients,	DP_set },
	{ NULL,		cfgVolume,	DP_param }
};


/* private parser data */
static char dir_name[TOKEN_SIZE];
static char token[TOKEN_SIZE];
static char *noEnd = NULL;
static boolean_t file_required = B_FALSE;
static diskvols_cfg_t *dv_cfg;
static sqm_lst_t *error_list = NULL;
static boolean_t no_cmd_file = B_FALSE;
static boolean_t empty_cmd_file;
static char open_error[80];


/*
 * Read disk volumes.
 */
int
parse_diskvols_conf(
char *file,		/* path at which to read the diskvols.conf */
diskvols_cfg_t **cfg)	/* malloced return value */
{

	int	errors;
	char	*first_err;

	Trace(TR_OPRMSG, "parsing diskvols file %s", Str(file));

	if (ISNULL(file, cfg)) {
		Trace(TR_OPRMSG, "parsing diskvols file failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (init_static_variables() != 0) {
		Trace(TR_OPRMSG, "parsing diskvols file failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (strcmp(DISKVOL_CFG, file) != 0) {
		file_required = B_TRUE;
	}


	dv_cfg = mallocer(sizeof (diskvols_cfg_t));
	if (dv_cfg == NULL) {
		Trace(TR_OPRMSG, "parsing diskvols file failed: %s",
		    samerrmsg);
		return (-1);
	}

	memset(dv_cfg, 0, sizeof (diskvols_cfg_t));
	dv_cfg->disk_vol_list = lst_create();
	if (dv_cfg->disk_vol_list == NULL) {
		free_diskvols_cfg(dv_cfg);
		*cfg = NULL;
		Trace(TR_OPRMSG, "parsing diskvols file failed: %s",
		    samerrmsg);
		return (-1);
	}

	dv_cfg->client_list = lst_create();
	if (dv_cfg->client_list == NULL) {
		free_diskvols_cfg(dv_cfg);
		*cfg = NULL;
		Trace(TR_OPRMSG, "parsing diskvols file failed: %s",
		    samerrmsg);
		return (-1);

	}
	dv_cfg->read_time = time(0);
	*cfg = dv_cfg;

	errors = ReadCfg(file, dirProcTable, dir_name, token, read_cfg_msg);
	if (errors != 0 && !(errors == -1 && !file_required)) {

		/* The absence of a command file is not an error */
		if (errors == -1) {

			if (no_cmd_file) {
				/*
				 * The absence of a command file
				 * is not an error
				 */
				Trace(TR_OPRMSG,
				    "parsing diskvols, no file present");

				return (0);
			}

			free_diskvols_cfg(*cfg);
			*cfg = NULL;

			/* other access errors are an error */
			samerrno = SE_CFG_OPEN_FAILED;

			/* open failed for %s: %s */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_CFG_OPEN_FAILED), file,
			    open_error);

			Trace(TR_OPRMSG, "parsing diskvols file failed: %s",
			    samerrmsg);
			return (-1);

		} else if (errors == 1 && empty_cmd_file) {
				Trace(TR_OPRMSG,
				    "parsing diskvols, empty file");
				return (0);
		}


		free_diskvols_cfg(*cfg);
		*cfg = NULL;

		if (error_list != NULL && error_list->head != NULL) {
			first_err =
			    ((parsing_error_t *)error_list->head->data)->msg;
		} else {
			first_err = "NULL";
		}

		/* %s contains %d error(s) first:%s */
		samerrno = SE_CONFIG_ERROR;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CONFIG_ERROR), file, errors, first_err);


		Trace(TR_OPRMSG, "parsing diskvols file failed: %s",
		    samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "parsed diskvols file");
	return (0);
}


/*
 * setup static information required for parser run.
 */
static int
init_static_variables(void)
{

	Trace(TR_DEBUG, "initializing static variables");

	noEnd = NULL;
	file_required = B_FALSE;

	if (error_list != NULL) {
		lst_free_deep(error_list);
	}

	error_list = lst_create();
	if (error_list == NULL) {
		Trace(TR_DEBUG, "initializing static variables failed: %s",
		    samerrmsg);

		return (-1);
	}

	no_cmd_file = B_FALSE;
	empty_cmd_file = B_FALSE;
	Trace(TR_DEBUG, "initialized static variables");
	return (0);
}


/*
 * get a list of parsing errors encountered in most recent parser run.
 */
int
get_diskvols_parsing_errors(
sqm_lst_t **l)	/* malloced list of parsing_error_t */
{

	if (ISNULL(l)) {
		Trace(TR_DEBUG, "get_diskvols_parsing_errors failed: %s",
		    samerrmsg);
		return (-1);
	}

	return (dup_parsing_error_list(error_list, l));
}


/*
 * Make disk volume entry.
 */
static void
cfgVolume(void)
{


	char *path;
	disk_vol_t *dv;

	Trace(TR_OPRMSG, "handling disk Volume");

	/*
	 * Check for invalid (too long) volume name.
	 */
	if (strlen(dir_name) > (sizeof (vsn_t) - 1)) {
		ReadCfgError(CustMsg(2881));
	}



	dv = (disk_vol_t *)mallocer(sizeof (disk_vol_t));
	if (dv == NULL) {
		ReadCfgError(samerrno);
	}
	memset(dv, 0, sizeof (disk_vol_t));
	snprintf(dv->vol_name, sizeof (dv->vol_name), dir_name);
	dv->set_flags |= DV_vol_name;

	/*
	 * If this is a honeycomb volume set the honeycomb flag in
	 * the disk_volume_t and call into a separate
	 * honeycomb specific parsing function.
	 */
	if (strcmp(token, HONEYCOMB_RESOURCE_NAME) == 0) {
		dv->status_flags |= DV_STK5800_VOL;
		cfgHCVolume(dv);
	} else {

		/*
		 * If token contains a colon the first part of the token
		 * is a host id. The second half is the path.
		 */
		if ((path = strchr(token, ':')) != NULL) {
			*path++ = '\0';
			snprintf(dv->host, sizeof (dv->host), token);
			dv->set_flags |= DV_host;
		} else {
			*dv->host = '\0';
			path = token;
		}

		if (*path != '/') {
			snprintf(dv->path, sizeof (dv->path), "/%s", path);
			dv->set_flags |= DV_path;
		} else {
			snprintf(dv->path, sizeof (dv->path), path);
			dv->set_flags |= DV_path;
		}
	}

	/*
	 * Explicit check for extra fields removed. Ignore any further
	 * information on the line.
	 */

	if (lst_append(dv_cfg->disk_vol_list, dv) != 0) {
		free(dv);
		ReadCfgError(samerrno);
	}
}

static void
cfgHCVolume(disk_vol_t *dv) {

	/* An IP address or hostname is required. */
	if (ReadCfgGetToken() == 0) {
		ReadCfgError(SE_HC_NEEDS_IP);
	}

	/*
	 * The ip:port or hostname:port are both part of token and
	 * need not be stored separately in the diskvol_t so just copy
	 * them both into the host field.
	 */
	strlcpy(dv->host, token, MAXHOSTNAMELEN);

	/*
	 * The metadir path to the catalog file is not currently supported
	 * in the core. We can add code for that when support is added.
	 */

}

/*
 *	Error if found an 'endclients' but no 'clients' statement.
 */
static void
dirNoclients(void)
{
	ReadCfgError(CustMsg(4447), dir_name + 3);
}


/*
 *	'clients' statement.
 */
static void
dirClients(void)
{

	static DirProc_t table[] = {
		{ "endclients",	dirEndclients,	DP_set   },
		{ NULL,		procClients,	DP_other },
	};
	char *msg;

	Trace(TR_OPRMSG, "handling diskvols clients directive");
	ReadCfgSetTable(table);
	msg = noEnd;
	noEnd = "noclients";
	if (msg != NULL) {
		ReadCfgError(CustMsg(4462), msg);
	}
}


/*
 *	'endclients' statement.
 */
static void
dirEndclients(void)
{

	Trace(TR_OPRMSG, "handling diskvols end_clients directive");
	ReadCfgSetTable(dirProcTable);
	noEnd = NULL;
}


/*
 *	Process clients.
 */
static void
procClients(void)
{

	char *cl;

	Trace(TR_OPRMSG, "processing client %s", Str(dir_name));

	cl = (char *)mallocer(sizeof (host_t));
	if (cl == NULL) {
		ReadCfgError(samerrno);
	}
	snprintf(cl, sizeof (host_t), dir_name);
	if (lst_append(dv_cfg->client_list, cl) != 0) {
		free(cl);
		ReadCfgError(samerrno);
	}

	Trace(TR_OPRMSG, "processed client");
}


/*
 *  Message handler for ReadCfg module.
 */
static void
read_cfg_msg(
char *msg,	/* error message */
int lineno,	/* line number of error */
char *line)	/* text of line in diskvols.conf */
{

	parsing_error_t *err;

	if (line != NULL) {

		if (msg != NULL) {
			/*
			 * Error message.
			 */
			Trace(TR_OPRMSG, "diskvols.conf error %d: %s %s",
			    lineno, line, msg);
			err = (parsing_error_t *)malloc(
			    sizeof (parsing_error_t));
			strlcpy(err->input, line, sizeof (err->input));
			strlcpy(err->msg, msg, sizeof (err->msg));
			err->line_num = lineno;
			err->error_type = ERROR;
			err->severity = 1;
			lst_append(error_list, err);
		}
	} else if (lineno == 0) {
		Trace(TR_OPRMSG, "diskvols.conf file is empty");
		empty_cmd_file = B_TRUE;
	} else if (lineno < 0) {

		/*
		 * Missing command file is not an error.
		 */
		if (errno == ENOENT) {
			no_cmd_file = B_TRUE;
			*open_error = '\0';
		} else {
			no_cmd_file = B_FALSE;
			snprintf(open_error, sizeof (open_error), msg);
			Trace(TR_OPRMSG, "diskvols.conf open error %s",
			    open_error);
		}
	}

}
