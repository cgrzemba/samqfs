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
#pragma ident   "$Revision: 1.24 $"
static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * parse_recycler.c
 *
 * Reads the recycler commands file and populates the recycler_cfg_t.
 */


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


/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/sam_trace.h"
#include "aml/device.h"
#include "sam/lib.h"
#include "sam/readcfg.h"
#include "aml/robots.h"


#define	ARCHSET_OWNER


/* API headers */
#include "mgmt/config/recycler.h"
#include "mgmt/config/common.h"
#include "pub/devstat.h"
#include "pub/mgmt/device.h"
#include "pub/mgmt/error.h"
#include "parser_utils.h"
#include "mgmt/util.h"
#include "mgmt/private_file_util.h"
#include "pub/mgmt/recycle.h"

/* Private data. */
static char dirName[TOKEN_SIZE];
static char token[TOKEN_SIZE];
static char open_error[80];
static boolean_t no_cmd_file;

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


/* private parser state */
static int		errors;
static sqm_lst_t		*error_list;
static recycler_cfg_t	*rc_cfg;
static rc_robot_cfg_t	*cur_rb;
static char		*cmd_file_name;

/* Private functions. */
static void msgFunc(char *msg, int lineno, char *line);
static int init_rc_cfg(void);
static int init_static_variables(void);
static void init_rc_robot(rc_robot_cfg_t *rb);


/*
 * Read command file
 */
int
parse_recycler(
char *file,		/* file to parse */
recycler_cfg_t **cfg)	/* malloced return */
{

	Trace(TR_OPRMSG, "parsing recycler cfg %s", Str(file));

	if (ISNULL(file, cfg)) {
		Trace(TR_OPRMSG, "parsing recycler cfg failed: %s", samerrmsg);
		return (-1);
	}
	cmd_file_name = file;

	if (init_static_variables() != 0) {
		return (-1);
	}


	if (init_rc_cfg() != 0) {
		Trace(TR_OPRMSG, "parsing recycler cfg failed: %s", samerrmsg);
		return (-1);
	}
	*cfg = rc_cfg;

	/*
	 * Read the command file.
	 */
	errors = ReadCfg(file, directives, dirName, token, msgFunc);

	if (errors == -1) {
		if (no_cmd_file) {
			/*
			 * The absence of a cmd file is not an error
			 * return 0 and the cfg that was initialized
			 * above
			 */
			Trace(TR_OPRMSG, "parsed recycler cfg");

			return (0);
		}

		free_recycler_cfg(*cfg);
		*cfg = NULL;

		samerrno = SE_CFG_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), cmd_file_name,
		    open_error);

		Trace(TR_OPRMSG, "parsing recycler cfg failed: %s", samerrmsg);

		return (-1);
	} else if (errors > 0) {
		char *first_err;
		samerrno = SE_CONFIG_ERROR;
		if (error_list != NULL && error_list->head != NULL) {
			first_err =
			    ((parsing_error_t *)error_list->head->data)->msg;
		} else {
			first_err = "NULL";
		}
		free_recycler_cfg(*cfg);
		*cfg = NULL;

		samerrno = SE_CONFIG_CONTAINED_ERRORS;
		/* %s contains %d error(s) first:%s */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CONFIG_ERROR), file, errors, first_err);

		Trace(TR_OPRMSG, "parsing recycler cfg failed: %s",
		    samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "parsed recycler cfg");
	return (0);
}


/*
 * get list of parsing errors from most recent run of parser.
 */
int
get_recycler_parsing_errors(sqm_lst_t **l) /* list of parsing_error_t */
{

	Trace(TR_DEBUG, "getting recycler parsing errors");
	if (ISNULL(l)) {
		Trace(TR_DEBUG, "getting recycler parsing errors() failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (dup_parsing_error_list(error_list, l) != 0) {
		Trace(TR_DEBUG, "getting recycler parsing errors() failed: %s",
		    samerrmsg);
		return (-1);
	}

	Trace(TR_DEBUG, "got recycler parsing errors");
	return (0);
}


/*
 * create the recycler structure with defualt entries.
 */
static int
init_rc_cfg(void)
{

	node_t *n;
	sqm_lst_t *l;
	rc_robot_cfg_t *rc_rob;
	rc_param_t *def_params;
	library_t *lib;

	Trace(TR_DEBUG, "initializing recycler cfg");

	rc_cfg = (recycler_cfg_t *)mallocer(sizeof (recycler_cfg_t));
	if (rc_cfg == NULL) {
		Trace(TR_DEBUG, "initializing recycler cfg failed: %s",
		    samerrmsg);
		return (-1);
	}

	memset(rc_cfg, 0, sizeof (recycler_cfg_t));
	if (get_default_rc_notify_script(NULL, rc_cfg->script) != 0) {
		free_recycler_cfg(rc_cfg);
		Trace(TR_DEBUG, "initializing recycler cfg failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (get_default_rc_log(NULL, rc_cfg->recycler_log) != 0) {
		free_recycler_cfg(rc_cfg);
		Trace(TR_DEBUG, "initializing recycler cfg failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* there are no default no_recycle_vsns, but create the list */
	rc_cfg->no_recycle_vsns = lst_create();
	if (rc_cfg->no_recycle_vsns == NULL) {
		free_recycler_cfg(rc_cfg);
		Trace(TR_DEBUG, "initializing recycler cfg failed: %s",
		    samerrmsg);
		return (-1);
	}


	rc_cfg->rc_robot_list = lst_create();
	if (rc_cfg->rc_robot_list == NULL) {
		free_recycler_cfg(rc_cfg);
		Trace(TR_DEBUG, "initializing recycler cfg failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* get the libs so a list default params can be built for them. */
	if (get_all_libraries(NULL, &l) == -1) {
		free_recycler_cfg(rc_cfg);
		return (-1);
	}

	if (get_default_rc_params(NULL, &def_params) != 0) {
		free_recycler_cfg(rc_cfg);
		free_list_of_libraries(l);
		Trace(TR_DEBUG, "initializing recycler cfg failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* there is a defualt entry for each robot present. Make it here */
	for (n = l->head; n != NULL; n = n->next) {
		lib = (library_t *)n->data;
		Trace(TR_DEBUG, "encountered robot %s", lib->base_info.set);

		/* don't create a rc_robot_cfg for historian */
		if (strcmp((lib->base_info).equ_type, "hy") == 0) {
			continue;
		}

		rc_rob = (rc_robot_cfg_t *)mallocer(sizeof (rc_robot_cfg_t));
		if (rc_rob == NULL) {
			free_recycler_cfg(rc_cfg);
			free(rc_rob);
			free(def_params);
			free_list_of_libraries(l);
			Trace(TR_DEBUG, "initializing recycler cfg failed: %s",
			    samerrmsg);
			return (-1);
		}
		init_rc_robot(rc_rob);

		memcpy(&(rc_rob->rc_params), def_params,
		    sizeof (rc_param_t));

		strlcpy(rc_rob->robot_name, lib->base_info.set,
		    sizeof (rc_rob->robot_name));

		if (lst_append(rc_cfg->rc_robot_list, rc_rob) != 0) {
			free_recycler_cfg(rc_cfg);
			free(def_params);
			free(rc_rob);
			free_list_of_libraries(l);
			Trace(TR_DEBUG, "initializing recycler cfg failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	free_list_of_libraries(l);
	free(def_params);
	rc_cfg->read_time = time(0);
	Trace(TR_DEBUG, "initialized recycler cfg");

	return (0);
}


/*
 * initialize variables that control the parsers activities.
 */
static int
init_static_variables(void)
{

	Trace(TR_DEBUG, "initializing static variables");

	cur_rb = NULL;
	rc_cfg = NULL;
	errors = 0;
	no_cmd_file = B_FALSE;

	if (error_list != NULL) {
		lst_free_deep(error_list);
	}
	error_list = lst_create();
	if (error_list == NULL) {
		Trace(TR_DEBUG, "initializing static variables() failed: %s",
		    samerrmsg);
		return (-1);
	}

	Trace(TR_DEBUG, "initialized static variables()");
	return (0);
}


/*
 * initialize a non-null rc_robot_cfg_t structure
 *
 */
static void
init_rc_robot(rc_robot_cfg_t *rb)
{

	Trace(TR_DEBUG, "initializing rc_robot");

	memset(rb, 0, sizeof (rc_robot_cfg_t));
	rb->rc_params.hwm = int_reset;
	rb->rc_params.data_quantity = fsize_reset;
	rb->rc_params.mingain = int_reset;
	rb->rc_params.vsncount = int_reset;
	Trace(TR_DEBUG, "initialized rc_robot");
}


/*
 * Process "logfile = <filename>"
 */
static void
dirLogfile(void)
{

	Trace(TR_DEBUG, "setting log");

	if (verify_file(token, B_TRUE) == B_FALSE) {
		ReadCfgError(CustMsg(19018), "log", token);
	}
	if (strlen(token) > sizeof (rc_cfg->recycler_log)-1) {
		ReadCfgError(CustMsg(19019),
		    sizeof (rc_cfg->recycler_log)-1);
	}

	(void) strlcpy(rc_cfg->recycler_log, token,
	    sizeof (rc_cfg->recycler_log));

	rc_cfg->change_flag |= RC_recycler_log;
	Trace(TR_DEBUG, "setting log");
}


/*
 * Process "script = <filename>"
 */
static void
dirScript(void)
{

	Trace(TR_DEBUG, "setting script");
	if (strlen(token) > sizeof (rc_cfg->script)-1) {
		ReadCfgError(CustMsg(19019),
		    sizeof (rc_cfg->script)-1);
	}

	strlcpy(rc_cfg->script, token, sizeof (rc_cfg->script));
	rc_cfg->change_flag |= RC_script;
	Trace(TR_DEBUG, "setting script");
}

/*
 * Process "no_recycle <media> [<regexp> ...]
 */
static void
dirNoRecycle(void)
{

	no_rc_vsns_t *no_rc;

	Trace(TR_DEBUG, "setting no_recycle");
	if (ReadCfgGetToken() == 0) {
		Trace(TR_DEBUG, "setting no_recycle: no token");
		return;
	}

	if (check_media_type(token) < 0) {
		ReadCfgError(4431);
	}

	no_rc = (no_rc_vsns_t *)mallocer(sizeof (no_rc_vsns_t));
	if (no_rc == NULL) {
		ReadCfgError(samerrno);
	}

	no_rc->vsn_exp = lst_create();
	if (no_rc->vsn_exp == NULL) {
		free(no_rc);
		ReadCfgError(samerrno);
	}

	strlcpy(no_rc->media_type, token, sizeof (no_rc->media_type));
	while (ReadCfgGetToken() != 0) {
		char *exp;
		if ((exp = regcmp(token, NULL)) == NULL) {
			free_no_rc_vsns(no_rc);
			ReadCfgError(CustMsg(20268), token);
		}
		free(exp);

		exp = (char *)mallocer(strlen(token) + 1);
		if (exp == NULL) {
			free_no_rc_vsns(no_rc);
			ReadCfgError(samerrno);
		}

		strcpy(exp, token);
		if (lst_append(no_rc->vsn_exp, exp) != 0) {
			free(exp);
			free_no_rc_vsns(no_rc);
			ReadCfgError(samerrno);
		}
	}

	if (lst_append(rc_cfg->no_recycle_vsns, no_rc) != 0) {
		free_no_rc_vsns(no_rc);
		ReadCfgError(samerrno);
	}

	Trace(TR_DEBUG, "set no_recycle");
}


/*
 * Set high water mark for recycling a library
 */
static void
paramsHWM(void)
{

	int		high;

	Trace(TR_DEBUG, "setting hwm");

	if (sscanf(token, "%d", &high) != 1 || high < 0 || high > 100) {
		Trace(TR_DEBUG, "high = %d", high);

		/* -hwm value is malformed or out of range. */
		ReadCfgError(CustMsg(20313));
	}
	cur_rb->rc_params.hwm = high;
	cur_rb->rc_params.change_flag |= RC_hwm;
	Trace(TR_DEBUG, "set hwm");
}


/*
 * Set minimum VSN gain for selecting a volume for recycling
 */
static void
paramsMingain(void)
{

	int		min;

	Trace(TR_DEBUG, "setting mingain");
	if (sscanf(token, "%d", &min) != 1 || min < 0 || min > 100) {
		/* -mingain value is malformed or out of range. */
		ReadCfgError(CustMsg(20314));
	}
	cur_rb->rc_params.mingain = min;
	cur_rb->rc_params.change_flag |= RC_mingain;
	Trace(TR_DEBUG, "set mingain");
}


/*
 * Set maximum data volume for a recycling library
 */
static void
paramsDataquantity(void)
{

	uint64_t	dataquantity;

	Trace(TR_DEBUG, "setting dataquantity");
	if (StrToFsize(token, &dataquantity) == -1) {
		ReadCfgError(CustMsg(4441), "-dataquantity");
	}
	cur_rb->rc_params.data_quantity = dataquantity;
	cur_rb->rc_params.change_flag |= RC_data_quantity;
	Trace(TR_DEBUG, "set dataquantity");
}


/*
 * Set maximum count of volumes to schedule for a recycling library
 */
static void
paramsVsncount(void)
{

	int		vsncount;

	Trace(TR_DEBUG, "setting vsncount");
	if (sscanf(token, "%d", &vsncount) != 1 || vsncount < 0) {
		/* vsncount value is malformed or out of range. */
		ReadCfgError(CustMsg(20318));
	}
	cur_rb->rc_params.vsncount = vsncount;
	cur_rb->rc_params.change_flag |= RC_vsncount;
	Trace(TR_DEBUG, "set vsncount");
}


/*
 * Set recycle-ignore for a library
 */
static void
paramsIgnore(void)
{

	Trace(TR_DEBUG, "setting ignore");
	cur_rb->rc_params.ignore = TRUE;
	cur_rb->rc_params.change_flag |= RC_ignore;
	Trace(TR_DEBUG, "set ignore");
}


/*
 * Set mail address
 */
static void
paramsMail(void)
{

	Trace(TR_DEBUG, "setting mail");
	strlcpy(cur_rb->rc_params.email_addr, token,
	    sizeof (cur_rb->rc_params.email_addr));
	cur_rb->rc_params.change_flag |= RC_email_addr;
	Trace(TR_DEBUG, "set mail");
}


/*
 * Process "<robot> <high-water> <min-gain> [<options> ...]"
 *  (old (pre-4.0) syntax)
 */
static void
dirRobot_pre40(void)
{

	int high;
	int min;

	Trace(TR_OPRMSG, "handling pre4.0 robot %s", token);
	/* "high" parameter should be in token when we get here ?? */
	if (sscanf(token, "%d", &high) != 1 || high < 0 || high > 100) {
		/* High-water mark is malformed or out of range. */
		ReadCfgError(CustMsg(20313));
	}
	cur_rb->rc_params.hwm = high;
	cur_rb->rc_params.change_flag |= RC_hwm;

	if (ReadCfgGetToken() == 0) {
		/* Line too short */
		ReadCfgError(CustMsg(20312));
	}
	if (sscanf(token, "%d", &min) != 1 || min < 0 || min > 100) {
		/* Min-gain value is malformed or out of range. */
		ReadCfgError(CustMsg(20314));
	}
	cur_rb->rc_params.mingain = min;
	cur_rb->rc_params.change_flag |= RC_mingain;

	/*
	 * Option processing.
	 */
	while (ReadCfgGetToken() != 0) {
		if (strcmp(token, "ignore") == 0 &&
		    cur_rb->rc_params.ignore  == 0) {

			cur_rb->rc_params.ignore = TRUE;
			cur_rb->rc_params.change_flag |= RC_ignore;

		} else if (strcmp(token, "mail") == 0 &&
		    cur_rb->rc_params.mail == 0) {

			if (ReadCfgGetToken() != 0) {
				strlcpy(cur_rb->rc_params.email_addr, token,
				    sizeof (cur_rb->rc_params.email_addr));
				cur_rb->rc_params.change_flag |=
				    (RC_mail | RC_email_addr);
			}

		} else {
			ReadCfgError(CustMsg(20315), token);
		}
	}
}


/*
 * Process robot-family-set line
 */
static void
dirRobot(void)
{

	boolean_t found = B_FALSE;
	node_t *n;

	static DirProc_t table[] = {
		{ "-hwm",	paramsHWM,	DP_param, 4499 },
		{ "-mingain",	paramsMingain, DP_param, 4501 },
		{ "-dataquantity", paramsDataquantity, DP_param, 4498 },
		{ "-vsncount", paramsVsncount, DP_param, 4515 },
		{ "-ignore", paramsIgnore, DP_other },
		{ "-mail", paramsMail, DP_param, 20317 },
		{ NULL, dirRobot_pre40, DP_other }
	};

	/* were there any params on the line? */
	boolean_t	noparams = TRUE;

	/*
	 * Find the robot mentioned in the recycler.cmd line.
	 */
	for (n = rc_cfg->rc_robot_list->head; n != NULL; n = n->next) {
		cur_rb = (rc_robot_cfg_t *)n->data;
		if (strcmp(cur_rb->robot_name, dirName) == 0) {
			found = B_TRUE;
			break;
		}
	}

	if (!found) {
		/*
		 * robot not found in mcf "No such library ...
		 * exists in system"
		 */
		ReadCfgError(CustMsg(20316), dirName);
	}

	while (ReadCfgGetToken() != 0) {
		noparams = FALSE;
		ReadCfgLookupDirname(token, table);
	} /* end while */

	if (noparams) {
		ReadCfgError(CustMsg(20312));
	}
}


/*
 * Process command file processing message.
 */
static void
msgFunc(
char *msg,	/* error message */
int lineno,	/* line number where error was encountered */
char *line)	/* input line (if any) that contained error */
{
	char err_buf[80];

	if (line != NULL) {
		/*
		 * While reading the file.
		 */
		if (msg != NULL) {
			parsing_error_t *err;
			/*
			 * Error message.
			 */
			Trace(TR_OPRMSG, "\t%d: %s\n", lineno, line);
			Trace(TR_OPRMSG, "\t*** %s\n", msg);
			err = (parsing_error_t *)mallocer(
			    sizeof (parsing_error_t));
			strlcpy(err->input, line, sizeof (err->input));
			strlcpy(err->msg, msg, sizeof (err->msg));
			err->line_num = lineno;
			err->error_type = ERROR;
			err->severity = 1;
			lst_append(error_list, err);
		}
	} else if (lineno < 0) {

		if (errno == ENOENT) {
			no_cmd_file = B_TRUE;
		} else {
			no_cmd_file = B_FALSE;
			snprintf(open_error, sizeof (open_error),
			    StrFromErrno(errno, err_buf,
			    sizeof (err_buf)));
		}
		Trace(TR_OPRMSG, "cannot open %s '%s':%s\n",
		    cmd_file_name, GetCustMsg(4400),
		    StrFromErrno(errno, err_buf, sizeof (err_buf)));

	}
}
