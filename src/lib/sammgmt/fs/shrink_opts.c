/*
 * shrink_opts.c - mgmt library code to read and write the command file for
 * sam-shrink.
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

#pragma ident "$Revision: 1.1 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>

/* POSIX headers. */
#include <sys/types.h>

/* Solaris headers. */
#include <sys/param.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/lib.h"
#include "sam/readcfg.h"

#include "sam/sam_trace.h"

/* mgmt headers */
#include "mgmt/util.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/sqm_list.h"


/*
 * Change flag values for shrink options. These allow the caller to
 * distinguish between default and global options that have been
 * copied into the file system options and ones that are explicitly
 * set for the file system.
 */
#define	SO_LOGFILE		0x00000001
#define	SO_BLOCK_SIZE		0x00000002
#define	SO_STREAMS		0x00000004
#define	SO_DO_NOT_EXECUTE	0x00000008
#define	SO_STAGE_FILES		0x00000010
#define	SO_STAGE_PARTIAL	0x00000020
#define	SO_DISPLAY_ALL_FILES   	0x00000040


/*
 * The mgmt lib is going to deal with the shrink options as key value pairs.
 * Include the structures and parsing apparatus here.
 */
typedef struct shrink_opts {
	uname_t fs_name;	/* empty string for the global settings */
	char logfile[MAXPATHLEN];
	int block_size;
	int streams;
	boolean_t do_not_execute;
	boolean_t stage_files;
	boolean_t stage_partial;
	boolean_t display_all_files;
	uint32_t change_flag;
} shrink_opts_t;


/*
 * Private functions.
 */
static void cmd_fs(void);
static void cmd_log_file(void);
static void display_all(void);
static void do_not_execute(void);
static void number_of_streams(void);
static void block_size(void);
static void stage_files(void);
static void stage_partial(void);
static void read_config_msg(char *msg, int lineno, char *line);
static int init_static_variables(void);
static void set_default_shrink_opts(shrink_opts_t *opts);
static int read_shrink_cmd(char *file_name, sqm_lst_t **res);
static int shrink_opts_to_kv(shrink_opts_t *so, char **kv_opts);
static int set_change_flags(shrink_opts_t *opts);
static int update_shrink_cmd(sqm_lst_t *opts);
static int write_shrink_cmd(char *fs_name, sqm_lst_t *opts);
static int write_shrink_opts(FILE *f, shrink_opts_t *so);


/*
 * Command table for parsing the shrink.cmd file
 */
static DirProc_t dirProcTable[] = {
	{ "fs",				cmd_fs,			DP_value },
	{ "logfile",			cmd_log_file,		DP_value },
	{ "do_not_execute",		do_not_execute,		DP_set },
	{ "display_all_files",		display_all,		DP_set },
	{ "streams",			number_of_streams,	DP_value },
	{ "block_size",			block_size,		DP_value },
	{ "stage_files",		stage_files,		DP_set },
	{ "stage_partial",		stage_partial,		DP_set },
	{ NULL,	NULL }
};



/* Macro to produce offset into structure */
#define	shrk_off(name) (offsetof(shrink_opts_t, name))

static parsekv_t shrink_kvtokens[] = {
	{"logfile",		shrk_off(logfile),	parsekv_string_1024},
	{"block_size",		shrk_off(block_size),	parsekv_int},
	{"streams",		shrk_off(streams),	parsekv_int},
	{"do_not_execute",	shrk_off(do_not_execute), parsekv_bool_YN},
	{"stage_files",		shrk_off(stage_files),	parsekv_bool_YN},
	{"stage_partial",	shrk_off(stage_partial), parsekv_bool_YN},
	{"display_all_files",	shrk_off(display_all_files), parsekv_bool_YN},
	{"change_flag",		shrk_off(change_flag),	parsekv_int},
	{"",			0,			NULL}
};



int
get_shrink_options(char *fs_name, char **kv_opts) {
	sqm_lst_t *opts;
	node_t *n;
	shrink_opts_t *globals;

	if (ISNULL(fs_name, kv_opts)) {
		Trace(TR_ERR, "Getting shrink opts failed for %s: %s",
		    Str(fs_name), samerrmsg);
		return (-1);
	}

	*kv_opts = NULL;

	if (read_shrink_cmd(SHRINK_CFG, &opts) != 0) {
		Trace(TR_ERR, "Getting shrink opts failed for %s: %s",
		    Str(fs_name), samerrmsg);
		return (-1);
	}

	/*
	 * find either the specific options for this file system or
	 * the globals which includes the default values.
	 */
	for (n = opts->head; n != NULL; n = n->next) {
		shrink_opts_t *so = (shrink_opts_t *)n->data;
		if (*(so->fs_name) == '\0') {
			/* grab the globals incase they are needed */
			globals = so;
		} else if (strcmp(fs_name, so->fs_name) == 0) {
			if (shrink_opts_to_kv(so, kv_opts) == -1) {
				lst_free_deep(opts);
				Trace(TR_ERR,
				    "Getting shrink opts failed for %s: %s",
				    Str(fs_name), samerrmsg);
				return (-1);
			} else {
				break;
			}
		}
	}

	/* Did not find the fs specific ones so convert the globals */
	if (*kv_opts == NULL && globals != NULL) {
		if (shrink_opts_to_kv(globals, kv_opts) == -1) {
			lst_free_deep(opts);
			Trace(TR_ERR,
			    "Getting shrink opts failed for %s: %s",
			    Str(fs_name), samerrmsg);
			return (-1);
		}
	}

	lst_free_deep(opts);

	if (*kv_opts == NULL) {
		/*
		 * This case is unexpected since globals should always
		 * be present as defaults.
		 */
		setsamerr(SE_GET_SHRINK_OPTS_FAILED);
		Trace(TR_ERR, "Getting shrink opts failed for %s: %s",
		    Str(fs_name), samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "Got shrink opts for %s: %s",
	    Str(fs_name), *kv_opts);

	return (0);
}


/*
 * All options in the input are assumed to be explicit settings for this
 * file system. Passing in an empty string is equivalent to deleting any
 * options present for the fs.
 */
int
set_shrink_options(char *fs_name, char *kv_opts) {
	shrink_opts_t *fs_opts = NULL;
	sqm_lst_t *opts;
	node_t *n;


	if (ISNULL(fs_name, kv_opts)) {
		Trace(TR_ERR, "setting shrink opts failed for %s: %s",
		    Str(fs_name), samerrmsg);
		return (-1);
	}

	if (read_shrink_cmd(SHRINK_CFG, &opts) != 0) {
		Trace(TR_ERR, "setting shrink opts failed for %s: %s",
		    Str(fs_name), samerrmsg);
		return (-1);
	}

	/*
	 * find the options for this file system or insert new ones.
	 */
	for (n = opts->head; n != NULL; n = n->next) {
		shrink_opts_t *so = (shrink_opts_t *)n->data;
		if (*(so->fs_name) != '\0' &&
		    strcmp(so->fs_name, fs_name) == 0) {
			fs_opts = so;
			/*
			 * zero the stuct so existing options will be overridden
			 * by the incoming options. (also supports delete)
			 */
			memset(fs_opts, 0, sizeof (shrink_opts_t));

			/* write the fs_name back into the struct */
			strlcpy(fs_opts->fs_name, fs_name, sizeof (uname_t));

		}
	}

	if (fs_opts == NULL) {
		fs_opts = (shrink_opts_t *)mallocer(sizeof (shrink_opts_t));
		if (fs_opts == NULL) {
			lst_free_deep(opts);
			Trace(TR_ERR, "setting shrink opts failed for %s: %s",
			    Str(fs_name), samerrmsg);
			return (-1);
		}
		if (lst_append(opts, fs_opts) != 0) {
			lst_free_deep(opts);
			Trace(TR_ERR, "setting shrink opts failed for %s: %s",
			    Str(fs_name), samerrmsg);
			return (-1);
		}
		strlcpy(fs_opts->fs_name, fs_name, sizeof (uname_t));
	}

	/* set the values in the fs_opts from the kv_options input */
	if (parse_kv(kv_opts, shrink_kvtokens, fs_opts) != 0) {
		lst_free_deep(opts);
		Trace(TR_ERR, "setting shrink opts failed for %s: %s",
		    Str(fs_name), samerrmsg);
		return (-1);
	}

	/*
	 * Caller is not required to set the change flags for the inputs
	 * so we must do it here. The change flags are only serving the
	 * purpose of helping rewrite the other file system's shrink options
	 */
	if (set_change_flags(fs_opts) != 0) {
		lst_free_deep(opts);
		Trace(TR_ERR, "setting shrink opts failed for %s: %s",
		    Str(fs_name), samerrmsg);
		return (-1);
	}

	if (update_shrink_cmd(opts) != 0) {
		lst_free_deep(opts);
		Trace(TR_ERR, "setting shrink opts failed for %s: %s",
		    Str(fs_name), samerrmsg);
		return (-1);
	}

	lst_free_deep(opts);
	Trace(TR_ERR, "set shrink options for %s",
	    Str(fs_name));
	return (0);
}


static int
shrink_opts_to_kv(shrink_opts_t *so, char **kv_opts) {

	int buf_sz = MAXPATHLEN + 160;
	char buf[buf_sz];
	size_t cur_sz = 0;
	char *end;

	buf[0] = '\0';

	if (*(so->logfile) != '\0') {
		if ((cur_sz = strlcat(buf, "logfile=", buf_sz)) >= buf_sz) {
			goto err;
		}
		if ((cur_sz = strlcat(buf, so->logfile, buf_sz)) >= buf_sz) {
			goto err;
		}

		if ((cur_sz = strlcat(buf, ",", buf_sz)) >= buf_sz) {
			goto err;
		}
	}

	if (so->do_not_execute) {
		if ((cur_sz = strlcat(buf, "do_not_execute=YES,",
		    buf_sz)) >= buf_sz) {
			goto err;
		}
	}
	if (so->stage_files) {
		if ((cur_sz = strlcat(buf, "stage_files=YES,",
		    buf_sz)) >= buf_sz) {
			goto err;
		}
	}
	if (so->stage_partial) {
		if ((cur_sz = strlcat(buf, "stage_partial=YES,",
		    buf_sz)) >= buf_sz) {
			goto err;
		}
	}
	if (so->display_all_files) {
		if ((cur_sz = strlcat(buf, "display_all_files=YES,",
		    buf_sz)) >= buf_sz) {
			goto err;
		}
	}
	if (so->block_size) {
		cur_sz += snprintf(buf + cur_sz, buf_sz - cur_sz,
		    "block_size=%d,", so->block_size);

		if (cur_sz >= buf_sz) {
			goto err;
		}
	}
	if (so->streams) {
		cur_sz += snprintf(buf + cur_sz, buf_sz - cur_sz,
		    "streams=%d,", so->streams);

		if (cur_sz >= buf_sz) {
			goto err;
		}
	}


	/* remove the trailing comma */
	if (*buf != '\0') {
		end = strrchr(buf, ',');
		if (end != NULL) {
			*end = '\0';
		}
	}

	*kv_opts = copystr(buf);
	if (*kv_opts == NULL) {
		goto err;
	}

	return (0);

err:
	Trace(TR_ERR, "converting shrink_opts to kv failed: %s", samerrmsg);
	*kv_opts = NULL;
	return (-1);
}


static int
set_change_flags(shrink_opts_t *opts) {
	if (ISNULL(opts)) {
		return (-1);
	}
	if (*(opts->logfile) != '\0') {
		opts->change_flag |= SO_LOGFILE;
	}
	if (opts->block_size != 0) {
		opts->change_flag |= SO_BLOCK_SIZE;
	}
	if (opts->streams != 0) {
		opts->change_flag |= SO_STREAMS;
	}
	if (opts->do_not_execute) {
		opts->change_flag |= SO_DO_NOT_EXECUTE;
	}
	if (opts->stage_files) {
		opts->change_flag |= SO_STAGE_FILES;
	}
	if (opts->stage_partial) {
		opts->change_flag |= SO_STAGE_PARTIAL;
	}
	if (opts->display_all_files) {
		opts->change_flag |= SO_DISPLAY_ALL_FILES;
	}
	return (0);
}


static int
update_shrink_cmd(sqm_lst_t *opts) {
	char ver_path[MAXPATHLEN+1];
	sqm_lst_t *tst_opts;

	/* write a temp file and read it to verify it */
	if (mk_wc_path(SHRINK_CFG, ver_path, sizeof (ver_path)) != 0) {
		Trace(TR_ERR, "updating shrink.cmd failed: %s", samerrmsg);
		return (-1);
	}
	if (write_shrink_cmd(ver_path, opts) != 0) {
		unlink(ver_path);
		Trace(TR_ERR, "updating shrink.cmd failed: %s", samerrmsg);
		return (-1);
	}
	if (read_shrink_cmd(ver_path, &tst_opts) != 0) {
		unlink(ver_path);
		Trace(TR_ERR, "updating shrink.cmd failed: %s", samerrmsg);
		return (-1);
	}
	lst_free_deep(tst_opts);
	unlink(ver_path);

	/*
	 * backup the cfg prior to writing it. Note this write is actually
	 * conditional see backup_cfg. It will only backup the existing file
	 * if a copy of the current one does not exist in the backup directory.
	 */
	if (backup_cfg(SHRINK_CFG) != 0) {
		/* leave samerrno as set */
		lst_free_deep(tst_opts);
		unlink(ver_path);
		Trace(TR_ERR, "updating shrink.cmd failed: %s", samerrmsg);
		return (-1);
	}

	/* write the actual file */
	if (write_shrink_cmd(SHRINK_CFG, opts) != 0) {
		unlink(ver_path);
		Trace(TR_ERR, "updating shrink.cmd failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * Make a backup of the new file to preserve a copy in case a user
	 * modifies the shrink.cmd file by hand. Trace any errors but do
	 * not return an error as the config has already been updated.
	 */
	if (backup_cfg(SHRINK_CFG) != 0) {
		Trace(TR_ERR, "Post write backup of shrink.cmd failed: %s",
		    samerrmsg);
	}
	return (0);
}

static int
write_shrink_cmd(char *location, sqm_lst_t *opts) {
	time_t the_time;
	char err_buf[80];
	FILE *f = NULL;
	int fd;
	node_t *n;


	if (ISNULL(location, opts)) {
		Trace(TR_OPRMSG, "writing archiver failed: %s",
		    samerrmsg);
		return (-1);
	}
	if ((fd = open(location, O_WRONLY|O_CREAT|O_TRUNC, 0644)) != -1) {
		f = fdopen(fd, "w");
	}
	if (f == NULL) {
		samerrno = errno;

		/* Open failed for %s: %s */
		StrFromErrno(samerrno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), location, err_buf);

		Trace(TR_OPRMSG, "writing shrink.cmd failed: %s",
		    samerrmsg);

		return (-1);
	}

	fprintf(f, "#\n#\tshrink.cmd\n#");
	the_time = time(0);
	fprintf(f, "\n#\tGenerated by config api %s#\n", ctime(&the_time));


	for (n = opts->head; n != NULL; n = n->next) {
		shrink_opts_t *so = (shrink_opts_t *)n->data;

		if (so->change_flag == 0) {
			continue;
		} else {
			write_shrink_opts(f, so);
		}
	}

	fprintf(f, "\n");
	fclose(f);

	Trace(TR_FILES, "wrote archiver %s", location);
	return (0);
}

static int
write_shrink_opts(FILE *f, shrink_opts_t *so) {
	boolean_t global = B_TRUE;
	if (ISNULL(f, so)) {
		return (-1);
	}
	if (so->change_flag == 0) {
		return (0);
	}

	if (*(so->fs_name) != '\0') {
		fprintf(f, "\nfs = %s\n", so->fs_name);
		global = B_FALSE;
	}
	if (*(so->logfile) != '\0' && so->change_flag & SO_LOGFILE) {
		fprintf(f, global ? "logfile = %s\n" : "\tlogfile = %s\n",
		    so->logfile);
	}
	if (so->block_size != 0 && so->change_flag & SO_BLOCK_SIZE) {
		fprintf(f, global ? "block_size = %d\n" : "\tblock_size = %d\n",
		    so->block_size);

	}
	if (so->streams != 0 && so->change_flag & SO_STREAMS) {
		fprintf(f, global ? "streams = %d\n" : "\tstreams = %d\n",
		    so->streams);
	}
	if (so->do_not_execute && so->change_flag & SO_DO_NOT_EXECUTE) {
		fprintf(f, global ? "do_not_execute\n" : "\tdo_not_execute\n");
	}
	if (so->stage_files && so->change_flag & SO_STAGE_FILES) {
		fprintf(f, global ? "stage_files\n" : "\tstage_files\n");
	}
	if (so->stage_partial && so->change_flag & SO_STAGE_PARTIAL) {
		fprintf(f, global ? "stage_partial\n" : "\tstage_partial\n");
	}
	if (so->display_all_files &&
	    so->change_flag & SO_DISPLAY_ALL_FILES) {
		fprintf(f, global ? "display_all_files\n" :
		    "\tdisplay_all_files\n");
	}
	return (0);
}

/*
 * Statics for readcfg
 */
static char dirname[TOKEN_SIZE];
static char token[TOKEN_SIZE];
static sqm_lst_t *shrink_cfg;
static shrink_opts_t *cur_opts;
static shrink_opts_t *global_opts;
static boolean_t no_cmd_file;
static char open_error[80];


/*
 * Read command file and return a list of shrink_opts_t. Note that even
 * thought the get and set shrink options functions deal only with
 * a single fs this function reads and returns data for the whole configuration
 * in order to support rewriting it.
 */
static int
read_shrink_cmd(char *file_name, sqm_lst_t **res)
{

	int errors = 0;


	if (ISNULL(file_name, res)) {
		Trace(TR_ERR, "Reading shrink.cmd failed: %s", samerrmsg);
		return (-1);
	}

	if (init_static_variables() != 0) {
		*res = NULL;
		Trace(TR_ERR, "Reading shrink.cmd failed: %s", samerrmsg);
		return (-1);
	}

	errors = ReadCfg(file_name, dirProcTable, dirname, token,
	    read_config_msg);

	/* errors == -1 indicates a problem opening the file */
	if (errors == -1) {

		if (no_cmd_file) {
			/*
			 * Absent file is not an error. Return the list
			 * containing the default global options
			 */
			*res = shrink_cfg;
			shrink_cfg = NULL;
			Trace(TR_OPRMSG, "leaving read shrink.cmd");
			return (0);
		}

		/*
		 * Other access problems are an error. Free memory, set values
		 * to null and return -1
		 */
		lst_free_deep(shrink_cfg);
		*res = NULL;

		samerrno = SE_CFG_OPEN_FAILED;
		/* Open failed for %s: %s */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), file_name, open_error);

		Trace(TR_OPRMSG, "parsing shrink.cmd %s failed: %s",
		    Str(file_name), samerrmsg);
	} else if (errors > 0) {

		lst_free_deep(shrink_cfg);
		*res = NULL;

		samerrno = SE_CONFIG_ERROR;
		/* shrink.cmd contains %d errors. Manually edit the file ... */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CONFIG_ERROR), file_name,
		    errors);

		Trace(TR_OPRMSG, "parsing shrink.cmd %s failed: %s",
		    Str(file_name), samerrmsg);

		return (-1);
	}

	*res = shrink_cfg;
	shrink_cfg = NULL;
	return (errors);
}


/*
 * create the shrink config.
 */
static int
init_static_variables() {

	shrink_cfg = lst_create();
	if (shrink_cfg == NULL) {
		Trace(TR_ERR, "Reading shrink.cmd failed: %s", samerrmsg);
		return (-1);
	}

	/* Create and initialize the global opts */
	global_opts = (shrink_opts_t *)mallocer(sizeof (shrink_opts_t));
	if (global_opts == NULL) {
		lst_free(shrink_cfg);
		Trace(TR_ERR, "Reading shrink.cmd failed: %s", samerrmsg);
		return (-1);
	}
	set_default_shrink_opts(global_opts);
	cur_opts = global_opts;

	/*
	 * Insert the global opts into the shrink_cfg so that
	 * all allocated options are always in the shrink_cfg.
	 */
	if (lst_append(shrink_cfg, global_opts) != 0) {
		lst_free(shrink_cfg);
		free(global_opts);
		Trace(TR_ERR, "Reading shrink.cmd failed: %s", samerrmsg);
		return (-1);
	}
	no_cmd_file = FALSE;
	return (0);
}


static void
set_default_shrink_opts(shrink_opts_t *opts) {

	opts->block_size = 1;
	opts->streams = 8;
}


/*
 * Handlers for the various keywords.
 */
static void
cmd_log_file(void)
{

	strlcpy(cur_opts->logfile, token, MAXPATHLEN);
	cur_opts->change_flag |= SO_LOGFILE;
}


static void
cmd_fs(void)
{

	/* create new options */
	cur_opts = (shrink_opts_t *)mallocer(sizeof (shrink_opts_t));
	if (cur_opts == NULL) {
		ReadCfgError(samerrno);
	}
	/*
	 * Copy the global properties into the new options but
	 * lear the change_flags so we can tell which ones are overriden
	 * by explicit setting.
	 */
	memcpy(cur_opts, global_opts, sizeof (shrink_opts_t));
	cur_opts->change_flag = 0;

	strlcpy(cur_opts->fs_name, token, sizeof (uname_t));

	/* Append the new options to the list */
	if (lst_append(shrink_cfg, cur_opts) != 0) {
		/* Free cur_opts since it's not in the shrink_cfg yet */
		free(cur_opts);
		cur_opts = NULL;
		ReadCfgError(samerrno);
	}
}


static void
display_all(void)
{
	cur_opts->display_all_files = B_TRUE;
	cur_opts->change_flag |= SO_DISPLAY_ALL_FILES;

}


static void
do_not_execute(void)
{
	cur_opts->do_not_execute = B_TRUE;
	cur_opts->change_flag |= SO_DO_NOT_EXECUTE;
}


static void
block_size(void)
{
	char *endptr;
	long block_size;

	block_size = strtol(token, &endptr, 10);
	if (endptr && *endptr != '\0') {
		ReadCfgError(8020, token, cur_opts->fs_name);
		return;
	}
	if (block_size < 1) {
		block_size = 1;
	}
	if (block_size > 16) {
		block_size = 16;
	}
	cur_opts->block_size = (int)block_size;
	cur_opts->change_flag |= SO_BLOCK_SIZE;
}


static void
number_of_streams(void)
{
	char *endptr;
	long streams;


	streams = strtol(token, &endptr, 10);
	if (endptr && *endptr != '\0') {
		ReadCfgError(8020, token, cur_opts->fs_name);
		return;
	}
	if (streams < 1) {
		streams = 1;
	}
	if (streams > 128) {
		streams = 128;
	}
	cur_opts->streams = (int)streams;
	cur_opts->change_flag |= SO_STREAMS;
}


static void
stage_files(void)
{
	cur_opts->stage_files = B_TRUE;
	cur_opts->change_flag |= SO_STAGE_FILES;
}


static void
stage_partial(void)
{
	cur_opts->stage_partial = B_TRUE;
	cur_opts->change_flag |= SO_STAGE_PARTIAL;
}


/*
 *  Message handler for ReadCfg module.
 */
static void
read_config_msg(char *msg, int lineno, char *line) {
	if (line != NULL) {

		if (msg != NULL) {
			/*
			 * Error message.
			 */
			Trace(TR_ERR, "shrink.cmd has an error at %d: %s %s",
			    lineno, line, msg);
		} else {
			Trace(TR_OPRMSG, "shrink.cmd has an error at %d: %s",
			    lineno, line);
		}

	} else if (lineno >= 0 && msg != NULL) {
			Trace(TR_ERR, "shrink.cmd has an error at %d: %s",
			    lineno, msg);
	} else if (lineno < 0) {
		if (errno == ENOENT) {
			Trace(TR_OPRMSG, "No shrink.cmd file found %s",
			    "using default values");
			no_cmd_file = B_TRUE;
		} else {
			no_cmd_file = B_FALSE;
			Trace(TR_ERR, "file open error:%s", msg);
			snprintf(open_error, sizeof (open_error), msg);
		}
	}
}
