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
#pragma ident   "$Revision: 1.22 $"
static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */


/*
 * parse_releaser.c - Read command file for releaser.
 */

/* ANSI C headers. */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/sam_trace.h"
#include "sam/lib.h"
#include "sam/readcfg.h"

/* API Headers */
#include "pub/mgmt/release.h"
#include "mgmt/config/releaser.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/devstat.h"
#include "pub/mgmt/device.h"
#include "pub/mgmt/error.h"
#include "parser_utils.h"
#include "mgmt/util.h"
#include "pub/mgmt/filesystem.h"
#include "mgmt/config/cfg_fs.h"
#include "mgmt/config/common.h"
#include "mgmt/private_file_util.h"


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
static int init_static_variables(void);
static int copy_releaser_defaults(void);
static int build_rel_cfg(void);
static int write_rl_fs_directive(FILE *f, rl_fs_directive_t *fs);


/* Command table */
static DirProc_t dirProcTable[] = {
	{ "fs", CmdFs, DP_value },
	{ "logfile", CmdLogfile, DP_value },
	{ "list_size", List_size, DP_value },
	{ "min_residence_age", MinResidenceAge, DP_value },
	{ "weight_age", Weight_age, DP_value },
	{ "weight_age_access", Weight_age_access, DP_value },
	{ "weight_age_modify", Weight_age_modify, DP_value },
	{ "weight_age_residence", Weight_age_residence, DP_value },
	{ "weight_size", Weight_size, DP_value },
	{ "no_release", No_release, DP_set },
	{ "rearch_no_release", Rearch_no_release, DP_set },
	{ "display_all_candidates", Display_all, DP_set },
	{ "debug_partial", Debug_partial, DP_set },
	{ NULL, NULL }
};


/* Private Data */
static rl_fs_directive_t *cur_rl, *glob_rl;
static releaser_cfg_t *rel_cfg;
static sqm_lst_t *fs_list;
static sqm_lst_t *error_list = NULL;
static boolean_t no_cmd_file = B_FALSE;
static char open_error[80];
static char dir_name[TOKEN_SIZE];
static char token[TOKEN_SIZE];
static rl_fs_directive_t *err_fs;	/* for unknown filesystems */

/*
 * Read command file. If errors are encounterd *cfg is set to null and -1 is
 * returned.
 */
int
parse_releaser_cmd(
char		*file,	/* the file to read */
sqm_lst_t		*filesystems,	/* list of fs to compare against */
releaser_cfg_t	**cfg)	/* return val, must be freed by caller */
{

	int		errors;

	Trace(TR_OPRMSG, "parsing releaser.cmd %s", Str(file));

	if (init_static_variables() != 0) {
		Trace(TR_OPRMSG, "parsing releaser.cmd %s failed: %s",
		    Str(file), samerrmsg);
		return (-1);
	}


	fs_list = filesystems;

	if (build_rel_cfg() != 0) {
		Trace(TR_OPRMSG, "parsing releaser.cmd %s failed: %s",
		    Str(file), samerrmsg);
		return (-1);
	}

	*cfg = rel_cfg;

	errors = ReadCfg(file, dirProcTable, dir_name, token, readcfgMsg);
	if (errors == -1) {
		if (no_cmd_file) {
			/*
			 * The absence of a command file
			 * is not an error
			 */
			Trace(TR_OPRMSG, "parsed releaser.cmd %s", Str(file));
			return (0);
		}
		/* other access errors are an error */
		free_releaser_cfg(*cfg);
		samerrno = SE_CFG_OPEN_FAILED;
		/* Open failed for %s: %s */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), file, open_error);

		Trace(TR_OPRMSG, "parsing releaser.cmd %s failed: %s",
		    Str(file), samerrmsg);

		return (-1);

	} else if (errors > 0) {
		char *first_err;
		free_releaser_cfg(*cfg);
		samerrno = SE_CONFIG_ERROR;
		if (error_list != NULL && error_list->head != NULL) {
			first_err =
			    ((parsing_error_t *)error_list->head->data)->msg;
		} else {
			first_err = "NULL";
		}

		/* %s contains %d error(s) first:%s */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CONFIG_ERROR), file,
		    errors, first_err);

		Trace(TR_OPRMSG, "parsing releaser.cmd %s failed: %s",
		    Str(file), samerrmsg);

		return (-1);
	}

	/* if true it means no fs specific directives were encountered. */
	if (cur_rl == glob_rl) {
		copy_releaser_defaults();
	}
	Trace(TR_OPRMSG, "parsed releaser.cmd %s", Str(file));
	return (0);
}


/*
 * initialize all static data to allow repeated calls to parse_releaser to
 * have the same starting position.
 */
static int
init_static_variables(void)
{

	Trace(TR_DEBUG, "initializing static variables");

	if (error_list != NULL) {
		lst_free_deep(error_list);
	}
	error_list = lst_create();
	if (error_list == NULL) {
		Trace(TR_DEBUG, "initializing static variables failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (err_fs != NULL) {
		free(err_fs);
		err_fs = NULL;
	}

	if (get_default_rl_fs_directive(NULL, &err_fs) != 0) {
		Trace(TR_DEBUG, "initializing static variables failed: %s",
		    samerrmsg);
		return (-1);
	}
	strcpy(err_fs->fs, "error fs");

	no_cmd_file = B_FALSE;
	*open_error = '\0';

	Trace(TR_DEBUG, "initialized static variables");
	return (0);

}


/*
 * function that would be used to copy the defaults and globals.
 * This function must be called prior to setting values in any of the
 * individual filesystems but after global directives are done being processed.
 */
static int
copy_releaser_defaults(void)
{

	node_t *n;
	rl_fs_directive_t *fs;
	uname_t tmp_name;
	Trace(TR_DEBUG, "copying releaser defaults");

	for (n = rel_cfg->rl_fs_dir_list->head; n != NULL; n = n->next) {
		fs = (rl_fs_directive_t *)n->data;

		/* preserve the name but copy everything else */
		strlcpy(tmp_name, fs->fs, sizeof (tmp_name));
		memcpy(fs, glob_rl, sizeof (rl_fs_directive_t));
		strlcpy(fs->fs, tmp_name, sizeof (tmp_name));
		if (strcmp(fs->fs, GLOBAL) != 0) {
			fs->change_flag = 0;
		}
	}
	Trace(TR_DEBUG, "copied releaser defaults");
	return (0);
}


/*
 * build releaser cfg in preparation for parsing.
 */
static int
build_rel_cfg(void)
{

	rl_fs_directive_t *rd;
	node_t *node;

	Trace(TR_DEBUG, "building releaser cfg");

	rel_cfg = (releaser_cfg_t *)mallocer(sizeof (releaser_cfg_t));
	if (rel_cfg == NULL) {
		Trace(TR_DEBUG, "building releaser cfg failed: %s",
		    samerrmsg);
		return (-1);
	}
	memset(rel_cfg, 0, sizeof (releaser_cfg_t));


	rel_cfg->rl_fs_dir_list = lst_create();
	if (rel_cfg->rl_fs_dir_list == NULL) {
		free_releaser_cfg(rel_cfg);
		Trace(TR_DEBUG, "building releaser cfg failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* init the releaser cfg */
	if (get_default_rl_fs_directive(NULL, &glob_rl) != 0) {
		free_releaser_cfg(rel_cfg);
		Trace(TR_DEBUG, "building releaser cfg failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* init the global_directives and insert into fs list */
	cur_rl = glob_rl;
	strcpy(glob_rl->fs, GLOBAL);
	if (lst_append(rel_cfg->rl_fs_dir_list, glob_rl) != 0) {
		free_releaser_cfg(rel_cfg);
		free(glob_rl);
		Trace(TR_DEBUG, "building releaser cfg failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* create a releaser params for each fs */
	for (node = fs_list->head; node != NULL; node = node->next) {
		char *name = ((fs_t *)node->data)->fi_name;

		if (get_default_rl_fs_directive(NULL, &rd) != 0) {
			free_releaser_cfg(rel_cfg);
			Trace(TR_DEBUG, "building releaser cfg failed: %s",
			    samerrmsg);
			return (-1);
		}

		strlcpy(rd->fs, name, sizeof (rd->fs));

		if (lst_append(rel_cfg->rl_fs_dir_list, rd) != 0) {
			free_releaser_cfg(rel_cfg);
			free(rd);
			Trace(TR_DEBUG, "building releaser cfg failed: %s",
			    samerrmsg);
			return (-1);
		}

	}

	rel_cfg->read_time = time(0);

	Trace(TR_DEBUG, "built releaser cfg");
	return (0);
}


/*
 * returns the parsing errors encountered in the most recent parsing.
 */
int
get_releaser_parsing_errors(
sqm_lst_t **l)	/* lst of parsing_error_t */
{

	Trace(TR_DEBUG, "getting releaser parsing errors");
	if (dup_parsing_error_list(error_list, l) != 0) {
		Trace(TR_DEBUG, "getting releaser parsing errors failed: %s",
		    samerrmsg);
		return (-1);
	}
	Trace(TR_DEBUG, "got releaser parsing errors");
	return (0);
}


/*
 * at least 10 but no more than 2,147,483,648.
 */
static void
List_size(void)
{

	char *endptr;
	int tmp;
	Trace(TR_DEBUG, "setting list_size");
	tmp = strtol(token, &endptr, 10);
	if (endptr && *endptr != '\0')  {
		/* Error converting "%s" to "%s" */
		ReadCfgError(8020, token, dir_name);
	}

	cur_rl->change_flag |= RL_list_size;
	cur_rl->list_size = tmp;
	Trace(TR_DEBUG, "set list size");
}


/*
 * check and set minresidenceage
 */
static void
MinResidenceAge(void)
{

	char *endptr;
	long tmp;
	Trace(TR_DEBUG, "setting min_residence_age");
	tmp = strtol(token, &endptr, 10);
	if (endptr && *endptr != '\0')  {
		/* Error converting "%s" to "%s" */
		ReadCfgError(8020, token, dir_name);
	}

	cur_rl->change_flag |= RL_min_residence_age;
	cur_rl->min_residence_age = tmp;
	Trace(TR_DEBUG, "set min_residence_age");
}


/*
 * check and set weightage
 */
static void
Weight_age(void) {

	Trace(TR_DEBUG, "setting weight_age");

	if (cur_rl->type == DETAILED_AGE_PRIO &&
	    cur_rl->change_flag & RL_type) {
		mixed_weights_used();
	}

	cur_rl->change_flag |= (RL_type | RL_simple);
	cur_rl->type = SIMPLE_AGE_PRIO;
	cur_rl->age_priority.simple = getfloat();
	Trace(TR_DEBUG, "set weight_age");
}


/*
 * check and set weight size
 */
static void
Weight_size(void)
{

	Trace(TR_DEBUG, "setting weight_size");

	/* errors are checked in get float */
	cur_rl->change_flag |= RL_size_priority;
	cur_rl->size_priority = getfloat();
	Trace(TR_DEBUG, "set weight_size");
}


/*
 * check and set weight age access
 */
static void
Weight_age_access(void)
{

	Trace(TR_DEBUG, "setting weight_age_access type = %d",
	    cur_rl->type);

	if (cur_rl->type == SIMPLE_AGE_PRIO && cur_rl->change_flag & RL_type) {
		mixed_weights_used();
	}

	cur_rl->change_flag |= (RL_type | RL_access_weight);
	cur_rl->type = DETAILED_AGE_PRIO;
	cur_rl->age_priority.detailed.access_weight = getfloat();

	Trace(TR_DEBUG, "set weight_age_access");
}


/*
 * check and set weight age modify
 */
static void
Weight_age_modify(void)
{

	Trace(TR_DEBUG, "setting weight_age_modify type = %d",
	    cur_rl->type);

	if (cur_rl->type == SIMPLE_AGE_PRIO && cur_rl->change_flag & RL_type) {
		mixed_weights_used();
	}

	cur_rl->change_flag |= (RL_type | RL_modify_weight);
	cur_rl->type = DETAILED_AGE_PRIO;
	cur_rl->age_priority.detailed.modify_weight = getfloat();
	Trace(TR_DEBUG, "set weight_age_modify");
}


/*
 * check and set weight age residence
 */
static void
Weight_age_residence(void)
{

	Trace(TR_DEBUG, "setting weight_age_residence type = %d",
	    cur_rl->type);

	if (cur_rl->type == SIMPLE_AGE_PRIO && cur_rl->change_flag & RL_type) {
		mixed_weights_used();
	}

	cur_rl->change_flag |= (RL_type | RL_residence_weight);
	cur_rl->type = DETAILED_AGE_PRIO;
	cur_rl->age_priority.detailed.residence_weight = getfloat();
	Trace(TR_DEBUG, "set weight_age_residence()");
}


/*
 * check and set log file
 */
static void
CmdLogfile(void)
{

	Trace(TR_DEBUG, "setting log_file");

	if (token != NULL && *token != '\0') {

		if (strlen(token) > sizeof (cur_rl->releaser_log)-1) {
			ReadCfgError(CustMsg(19019),
			    sizeof (cur_rl->releaser_log)-1);
		}

		if (verify_file(token, B_TRUE) == B_FALSE) {
			ReadCfgError(CustMsg(19018), "log", token);
		}


		cur_rl->change_flag |= RL_releaser_log;
		(void) strlcpy(cur_rl->releaser_log, token,
		    sizeof (cur_rl->releaser_log));
	}
	Trace(TR_DEBUG, "set log_file");
}


/*
 * set the current fs to token.
 */
static void
CmdFs(void)
{

	node_t *node;
	boolean_t fs_found = B_FALSE;
	rl_fs_directive_t *tmp;

	Trace(TR_DEBUG, "setting current fs %s", token);

	for (node = rel_cfg->rl_fs_dir_list->head;
	    node != NULL; node = node->next) {
		if (strcmp(token,
		    ((rl_fs_directive_t *)node->data)->fs) == 0) {

			tmp = (rl_fs_directive_t *)node->data;
			fs_found = B_TRUE;
		}
	}

	if (!fs_found) {
		cur_rl = err_fs;
		ReadCfgError(0, "Filesystem %s not defined", token);
	}

	if (cur_rl == glob_rl) {
		copy_releaser_defaults();
	}
	cur_rl = tmp;

	Trace(TR_DEBUG, "set current fs");
}


/*
 * check and set display all.
 */
static void
Display_all(void)
{

	Trace(TR_DEBUG, "setting display_all");
	cur_rl->change_flag |= RL_display_all_candidates;
	cur_rl->display_all_candidates = B_TRUE;
	Trace(TR_DEBUG, "set display_all");
}


/*
 * check and set no release.
 */
static void
No_release(void)
{

	Trace(TR_DEBUG, "setting no_release");
	cur_rl->change_flag |= RL_no_release;
	cur_rl->no_release = B_TRUE;
	Trace(TR_DEBUG, "set no_release");
}


/*
 * check and set rearch no release.
 */
static void
Rearch_no_release(void)
{

	Trace(TR_DEBUG, "setting rearch_no_release");
	cur_rl->change_flag |= RL_rearch_no_release;
	cur_rl->rearch_no_release = B_TRUE;
	Trace(TR_DEBUG, "set rearch_no_release");
}


/*
 * check and set no debug partial.
 */
static void
Debug_partial(void)
{

	Trace(TR_DEBUG, "setting debug_partial");
	cur_rl->change_flag |= RL_debug_partial;
	cur_rl->debug_partial = B_TRUE;
	Trace(TR_DEBUG, "set debug_partial");
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
		ReadCfgError(8020, token, dir_name);
	}
	if (value > 1.0 || value < 0.0) {
		/* value "%f" for "%s" must be >= 0.0 and <= 1.0  */
		ReadCfgError(8021, value, dir_name);
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
readcfgMsg(
char *msg,	/* the err msg */
int lineno,	/* the line number */
char *line)	/* the input line */
{

	parsing_error_t *err;


	if (line != NULL) {
		if (msg != NULL) {
			/*
			 * Error message.
			 */
			Trace(TR_OPRMSG, "error in releaser.cmd line %d: %s",
			    lineno, line);
			Trace(TR_OPRMSG, "error: %s", msg);
			err = (parsing_error_t *)mallocer(
			    sizeof (parsing_error_t));
			strlcpy(err->input, line, sizeof (err->input));
			strlcpy(err->msg, msg, sizeof (err->msg));
			err->line_num = lineno;
			err->error_type = ERROR;
			err->severity = 1;
			lst_append(error_list, err);
		}
	} else if (lineno >= 0) {
		Trace(TR_DEBUG, "releaser.cmd error:%s", msg);
	} else if (lineno < 0) {
		if (errno == ENOENT) {
			Trace(TR_OPRMSG, "No releaser.cmd file found %s",
			    "using default values");
			no_cmd_file = B_TRUE;
		} else {
			no_cmd_file = B_FALSE;
			Trace(TR_OPRMSG, "file open error:%s", msg);
			snprintf(open_error, sizeof (open_error), msg);
		}
	}
}


/*
 * write releaser.cmd file to the specified location.
 */
int
write_releaser_cmd(
const char *location,		/* where to write */
const releaser_cfg_t *cfg)	/* cfg to write */
{

	FILE *f = NULL;
	int fd;
	time_t the_time;
	node_t *node;
	rl_fs_directive_t *rl_fs_dir;

	Trace(TR_DEBUG, "write_releaser_cmd %s ", Str(location));
	if (ISNULL(location, cfg)) {
		Trace(TR_DEBUG, "write_releaser_cmd %s failed: %s",
		    Str(location), samerrmsg);
		return (-1);
	}

	if ((fd = open(location, O_WRONLY|O_CREAT|O_TRUNC, 0644)) != -1) {
		f = fdopen(fd, "w");
	}

	if (f == NULL) {
		char err_msg[STR_FROM_ERRNO_BUF_SIZE];
		samerrno = SE_CFG_OPEN_FAILED;
		/* Open failed for %s: %s */

		StrFromErrno(errno, err_msg, sizeof (err_msg));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), location, err_msg);

		Trace(TR_DEBUG, "write_releaser_cmd() failed: %s",
		    samerrmsg);

		return (-1);
	}

	fprintf(f, "#\n#\treleaser.cmd\n");
	fprintf(f, "#\n");
	the_time = time(0);
	fprintf(f, "#\tGenerated by config api %s#\n", ctime(&the_time));

	/* get the globals and print them first */
	if (cfg_get_rl_fs_directive(cfg, GLOBAL, &rl_fs_dir) == 0) {
		write_rl_fs_directive(f, rl_fs_dir);
		free(rl_fs_dir);
	}

	for (node = cfg->rl_fs_dir_list->head; node != NULL;
	    node = node->next) {
		if (strcmp(((rl_fs_directive_t *)node->data)->fs,
		    GLOBAL) != 0) {

			write_rl_fs_directive(f,
			    (rl_fs_directive_t *)node->data);
		}
	}

	fprintf(f, "\n");
	fflush(f);
	if (fclose(f) != 0) {
		char err_buf[80];
		samerrno = SE_CFG_CLOSE_FAILED;
		/* Close failed for %s: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_CLOSE_FAILED), location, err_buf);

		Trace(TR_DEBUG, "write_releaser_cmd() failed: %s",
		    samerrmsg);
		return (-1);
	}


	Trace(TR_DEBUG, "write_releaser_cmd returning 0");
	return (0);
}


/*
 * write rl_fs_directive to file.
 */
static int
write_rl_fs_directive(
FILE *f,
rl_fs_directive_t *fs)
{

	upath_t buf;
	char *print_first = NULL;
	boolean_t done_once = B_FALSE;
	Trace(TR_DEBUG, "write_rl_fs_directive %s %u", fs->fs,
	    fs->change_flag);

	/* make the fs line. Keep it till needed. */
	if (strcmp(fs->fs, GLOBAL) != 0) {
		snprintf(buf, sizeof (buf), "#\n#\nfs = %s\n", fs->fs);
		print_first = buf;
	}

	if (*fs->releaser_log != char_array_reset &&
	    fs->change_flag & RL_releaser_log) {

		write_once(f, print_first, &done_once);
		fprintf(f, "%s = %s\n", done_once ? "\tlogfile" : "logfile",
		    fs->releaser_log);
	}

	if (fs->size_priority != float_reset &&
	    fs->change_flag & RL_size_priority) {
		write_once(f, print_first, &done_once);
		fprintf(f, "%s = %f\n", done_once ? "\tweight_size" :
		    "weight_size", fs->size_priority);
	}

	if (fs->type == SIMPLE_AGE_PRIO &&
	    fs->age_priority.simple != float_reset &&
	    fs->change_flag & RL_simple) {

		write_once(f, print_first, &done_once);
		fprintf(f, "%s = %f\n", done_once ? "\tweight_age" :
		    "weight_age", fs->age_priority.simple);
	}


	if (fs->type == DETAILED_AGE_PRIO &&
	    fs->age_priority.detailed.access_weight != float_reset &&
	    fs->change_flag & RL_access_weight) {

		write_once(f, print_first, &done_once);
		fprintf(f, "%s = %f\n", done_once ? "\tweight_age_access" :
		    "weight_age_access",
		    fs->age_priority.detailed.access_weight);
	}

	if (fs->type == DETAILED_AGE_PRIO &&
	    fs->age_priority.detailed.modify_weight != float_reset &&
	    fs->change_flag & RL_modify_weight) {

		write_once(f, print_first, &done_once);
		fprintf(f, "%s = %f\n", done_once ? "\tweight_age_modify" :
		    "weight_age_modify",
		    fs->age_priority.detailed.modify_weight);
	}

	if (fs->type == DETAILED_AGE_PRIO &&
	    fs->age_priority.detailed.residence_weight != float_reset &&
	    fs->change_flag & RL_residence_weight) {

		write_once(f, print_first, &done_once);
		fprintf(f, "%s = %f\n", done_once ? "\tweight_age_residence" :
		    "weight_age_residence",
		    fs->age_priority.detailed.residence_weight);
	}

	if (fs->no_release && fs->change_flag & RL_no_release) {
		write_once(f, print_first, &done_once);
		fprintf(f, "%s\n", done_once ? "\tno_release" : "no_release");
	}

	if (fs->rearch_no_release &&
	    fs->change_flag & RL_rearch_no_release) {

		write_once(f, print_first, &done_once);
		fprintf(f, "%s\n", done_once ? "\trearch_no_release" :
		    "rearch_no_release");
	}

	if (fs->display_all_candidates &&
	    fs->change_flag & RL_display_all_candidates) {

		write_once(f, print_first, &done_once);
		fprintf(f, "%s\n", done_once ? "\tdisplay_all_candidates" :
		    "display_all_candidates");
	}

	if (fs->debug_partial && fs->change_flag & RL_debug_partial) {
		write_once(f, print_first, &done_once);
		fprintf(f, "%s\n", done_once ? "\tdebug_partial" :
		    "debug_partial");
	}

	if (fs->list_size != int_reset && fs->change_flag & RL_list_size) {

		write_once(f, print_first, &done_once);
		fprintf(f, "%s = %d\n", done_once ? "\tlist_size" :
		    "list_size", fs->list_size);
	}

	if (fs->min_residence_age != long_reset &&
	    fs->change_flag & RL_min_residence_age) {

		write_once(f, print_first, &done_once);
		fprintf(f, "%s = %ld\n", done_once ? "\tmin_residence_age" :
		    "min_residence_age", fs->min_residence_age);
	}

	Trace(TR_DEBUG, "write_rl_fs_directive()");
	return (0);
}


/*
 * will succeed even if fs list is NULL if there are only global directives.
 */
int
verify_rl_fs_directive(
rl_fs_directive_t *fsdir,	/* fs to verify */
sqm_lst_t *filesystems)		/* list of fs_t to check against. */
{

	fs_t *fst = NULL;

	Trace(TR_DEBUG, "verify_rl_fs_directive()");
	if (ISNULL(fsdir)) {
		Trace(TR_DEBUG, "verify_rl_fs_directive() failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (strcmp(fsdir->fs, GLOBAL) != 0) {
		/* check with to ensure the fs exists */

		if (filesystems == NULL) {
			samerrno = SE_UNABLE_TO_CHECK_FS_WITH_MCF;
			/* Could not check file systems with master config */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_UNABLE_TO_CHECK_FS_WITH_MCF));

			Trace(TR_DEBUG, "verify_rl_fs_directive()%s %s",
			    " failed:", samerrmsg);

			return (-1);
		}

		if (find_fs(filesystems, fsdir->fs, &fst) != 0) {
			Trace(TR_DEBUG, "verify_rl_fs_directive() %s",
			    "failed: -1");
			return (-1);
		}
	}


	if (*fsdir->releaser_log != char_array_reset &&
	    fsdir->change_flag & RL_releaser_log) {

		if (verify_file(fsdir->releaser_log, B_TRUE) == 0) {
			samerrno = SE_LOGFILE_CANNOT_BE_CREATED;
			/* Logfile %s can not be created */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_LOGFILE_CANNOT_BE_CREATED),
			    fsdir->releaser_log);

			Trace(TR_DEBUG, "verify_rl_fs_directive()%s%s",
			    " failed: ", samerrmsg);

			return (-1);
		}
	}

	if (fsdir->size_priority != float_reset &&
	    fsdir->change_flag & RL_size_priority) {

		if (fsdir->size_priority > 1.0 || fsdir->size_priority < 0.0) {
			samerrno = SE_FLOAT_OUT_OF_RANGE;
			/* value "%f" for "%s" must be >= 0.0 and <= 1.0  */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_FLOAT_OUT_OF_RANGE),
			    fsdir->size_priority, "weight_size", 0.0, 1.0);

			Trace(TR_DEBUG, "verify_rl_fs_directive()%s%s",
			    " failed: ", samerrmsg);
			return (-1);
		}
	}



	if (fsdir->type != SIMPLE_AGE_PRIO &&
	    fsdir->type != DETAILED_AGE_PRIO && fsdir->type != NOT_SET &&
	    fsdir->change_flag & RL_type) {


		samerrno = SE_INVALID_AGE_PRIORITY_TYPE;

		/* Invalid age priority type %d */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_AGE_PRIORITY_TYPE), fsdir->type);

		Trace(TR_DEBUG, "verify_rl_fs_directive() failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (fsdir->type == SIMPLE_AGE_PRIO &&
	    fsdir->age_priority.simple != float_reset &&
	    fsdir->change_flag & RL_simple) {

		if (fsdir->age_priority.simple > 1.0 ||
		    fsdir->age_priority.simple < 0.0) {

			samerrno = SE_FLOAT_OUT_OF_RANGE;
			/* value "%f" for "%s" must be >= %f and <= %f  */

			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_FLOAT_OUT_OF_RANGE),
			    fsdir->age_priority.simple, "weight_age",
			    0.0, 1.0);

			Trace(TR_DEBUG, "verify_rl_fs_directive()%s%s",
			    " failed: ", samerrmsg);
			return (-1);
		}
	}


	if (fsdir->type == DETAILED_AGE_PRIO &&
	    fsdir->age_priority.detailed.access_weight != float_reset &&
	    fsdir->change_flag & RL_access_weight) {

		if (fsdir->age_priority.detailed.access_weight > 1.0 ||
		    fsdir->age_priority.detailed.access_weight < 0.0) {


			samerrno = SE_FLOAT_OUT_OF_RANGE;
			/* value "%f" for "%s" must be >= 0.0 and <= 1.0  */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_FLOAT_OUT_OF_RANGE),
			    fsdir->age_priority.detailed.access_weight,
			    "weight_age_access", 0.0, 1.0);

			Trace(TR_DEBUG, "verify_rl_fs_directive()%s%s",
			    " failed: ", samerrmsg);

			return (-1);
		}
	}


	if (fsdir->type == DETAILED_AGE_PRIO &&
	    fsdir->age_priority.detailed.modify_weight != float_reset &&
	    fsdir->change_flag & RL_modify_weight) {

		if (fsdir->age_priority.detailed.modify_weight > 1.0 ||
		    fsdir->age_priority.detailed.modify_weight < 0.0) {

			samerrno = SE_FLOAT_OUT_OF_RANGE;
			/* value "%f" for "%s" must be >= 0.0 and <= 1.0  */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_FLOAT_OUT_OF_RANGE),
			    fsdir->age_priority.detailed.modify_weight,
			    "weight_age_modify", 0.0, 1.0);

			Trace(TR_DEBUG, "verify_rl_fs_directive()%s%s",
			    " failed: ", samerrmsg);

			return (-1);
		}
	}


	if (fsdir->type == DETAILED_AGE_PRIO &&
	    fsdir->age_priority.detailed.residence_weight != float_reset &&
	    fsdir->change_flag & RL_residence_weight) {

		if (fsdir->age_priority.detailed.residence_weight > 1.0 ||
		    fsdir->age_priority.detailed.residence_weight < 0.0) {


			samerrno = SE_FLOAT_OUT_OF_RANGE;
			/* value "%f" for "%s" must be >= 0.0 and <= 1.0  */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_FLOAT_OUT_OF_RANGE),
			    fsdir->age_priority.detailed.residence_weight,
			    "weight_age_residence", 0.0, 1.0);

			Trace(TR_DEBUG, "verify_rl_fs_directive()%s%s",
			    " failed: ", samerrmsg);
			return (-1);
		}
	}

	if (fsdir->list_size != int_reset &&
	    fsdir->change_flag & RL_list_size) {

		if (fsdir->list_size < 10 || fsdir->list_size > INT32_MAX) {

			samerrno = SE_LIST_SIZE_OUT_OF_RANGE;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_LIST_SIZE_OUT_OF_RANGE),
			    fsdir->list_size, 10, INT32_MAX);

			Trace(TR_DEBUG, "verify_rl_fs_directive()%s%s",
			    " failed: ", samerrmsg);

			return (-1);
		}
	}


	if (fsdir->min_residence_age != long_reset &&
	    fsdir->change_flag & RL_min_residence_age) {

		if (fsdir->min_residence_age < 0) {

			samerrno = SE_MIN_RESIDENCE_AGE_TOO_LOW;

			/* min_residence_age %d must be > 0 */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_MIN_RESIDENCE_AGE_TOO_LOW),
			    fsdir->min_residence_age);

			Trace(TR_DEBUG, "verify_rl_fs_directive()%s%s",
			    " failed: ", samerrmsg);

			return (-1);
		}
	}


	Trace(TR_DEBUG, "verify_rl_fs_directive()");
	return (0);
}
