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
#pragma ident   "$Revision: 1.33 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */


/*
 * parse_samfs_cmd.c
 * Contains functions to parse the samfs.cmd file and popluate a
 * mount_cfg_t structure and to write a mount_cfg_t structure to a file.
 */

/* ANSI C headers. and Solaris headers. */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>

/* SAM-FS headers. */
#include "sam/mount.h"
#include "sam/readcfg.h"
#include "sam/lib.h"
#include "sam/sam_trace.h"

/* API Headers */

#include "mgmt/util.h"
#include "pub/devstat.h"
#include "pub/mgmt/filesystem.h"
#include "mgmt/config/mount_cfg.h"
#include "pub/mgmt/error.h"
#include "parser_utils.h"
#include "sam/setfield.h"
#include "mgmt/config/mount_options.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/license.h"

/* static data for API */
static mount_cfg_t	*mnt_cfg;
static fs_t		*mp_all, *mp;
static char		*EQUALS = " = ";
static boolean_t	file_required = B_FALSE;
static char		err_msg[256];
static char		dir_name[TOKEN_SIZE];
static char		token[TOKEN_SIZE];
static sqm_lst_t		*error_list = NULL;
static char		*fname;
static boolean_t	out_of_memory = B_FALSE;
static boolean_t	no_cmd_file = B_FALSE;
static char		open_error[80];
static fs_t		error_fs;
static mount_options_t	error_opts;

/* Private functions. */
static void cmdFs(void);
static void cmdValue(void);
static void copy_defaults(void);
static void samfs_msg_func(int code, char *msg);
static void handle_msg(char *msg, int lineno, char *line);
static int build_fs_mnt_opts();

static int init_static_variables();
static int reset_mount_options_change_flag(mount_options_t *mo);
static int write_fs_cmds(FILE *f, fs_t *o);
static int print_field(FILE *f, void *val, struct fieldVals *entry,
	char *print_first, boolean_t *printed);
static int set_dependent_default_values(mount_options_t *mo);
static int set_all_dependent_default_values();

/* Command table */
static DirProc_t dirProcTable[] = {
	{ "fs",	cmdFs,		DP_value},
	{ NULL, cmdValue,	DP_setfield}
};


/*
 * ISSUES:
 * 1. File system and device dependent defaults are not all set correctly
 *    yet.
 */


/*
 * Read file system command file and populate the mount_cfg struct.
 */
int
parse_samfs_cmd(
char *fscfg_name,	/* file to parse */
mount_cfg_t *cfg)	/* struct to populate */
{

	int	errors;

	Trace(TR_FILES, "parsing samfs.cmd (%s)", Str(fscfg_name));


	if (init_static_variables() != 0) {
		Trace(TR_ERR, "parsing samfs.cmd failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (strcmp(fscfg_name, SAMFS_CFG) != 0) {
		file_required = B_TRUE;
	}
	fname = fscfg_name;
	mnt_cfg = cfg;

	if (build_fs_mnt_opts() != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "parsing samfs.cmd failed: %s",
		    samerrmsg);
		return (-1);
	}

	mnt_cfg->read_time = time(0);

	errors = ReadCfg(fscfg_name, dirProcTable, dir_name, token, handle_msg);
	if (errors != 0 && !(errors == -1 && !file_required)) {

		if (errors == -1) {
			if (no_cmd_file) {
				/*
				 * Absence of the default config file
				 * is not an error.
				 */
				Trace(TR_OPRMSG, "parsing samfs.cmd %s",
				    "no file using defaults");
				return (0);

			}

			samerrno = SE_CFG_OPEN_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_CFG_OPEN_FAILED), fscfg_name,
			    open_error);

			Trace(TR_ERR, "parse samfs.cmd failed: %s",
			    samerrmsg);
			return (-1);


		} else if (errors > 0) {
			char *first_err;
			samerrno = SE_CONFIG_ERROR;
			if (error_list != NULL && error_list->head != NULL) {
				first_err = ((parsing_error_t *)
				    error_list->head->data)->msg;
			} else {
				first_err = "NULL";
			}

			/* %s contains %d error(s) first:%s */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_CONFIG_ERROR), fscfg_name,
			    errors, first_err);

			Trace(TR_ERR, "parsing samfs.cmd failed: %s",
			    samerrmsg);

			return (-1);
		}

		if (out_of_memory) {
			setsamerr(SE_NO_MEM);

			Trace(TR_ERR, "parsing samfs.cmd failed: %s",
			    samerrmsg);

			return (-1);
		}

		samerrno = SE_CONFIG_CONTAINED_ERRORS;

		/* Configuration contained %d errors */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CONFIG_CONTAINED_ERRORS), fscfg_name,
		    errors);

		Trace(TR_ERR, "parsing samfs.cmd failed: %s",
		    samerrmsg);

		return (-1);
	}

	/*
	 * So when you are done if current mount options are the
	 * global ones.	 Copy everything over to all of them.
	 */
	if (mp == mp_all) {
		copy_defaults();
	}


	set_all_dependent_default_values();


	if (error_list->length != 0) {
		samerrno = SE_CONFIG_CONTAINED_ERRORS;

		/* Configuration contained errors in aggregate */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CONFIG_CONTAINED_ERRORS), fscfg_name,
		    errors);

		Trace(TR_DEBUG, "parsing samfs.cmd failed: %s",
		    samerrmsg);

		return (-1);
	}


	Trace(TR_DEBUG, "parsing samfs.cmd");
	return (0);
}


/*
 * This function sets the default value of fields that are dependent on
 * other options. It only sets them if they have not been explicitly set.
 * It does not do checking of the interrelated values. It also does not
 * set any change_flags for these fields as these are default values.
 */
static int
set_all_dependent_default_values(void) {
	node_t *n;

	for (n = mnt_cfg->fs_list->head; n != NULL; n = n->next) {
		fs_t *fs = (fs_t *)n->data;

		set_dependent_default_values(fs->mount_options);
		if (get_samfs_type(NULL) == QFS ||
			fs->mount_options->multireader_opts.reader ||
			!fs->mount_options->sam_opts.archive ||
			fs->fi_status & FS_CLIENT) {

			fs->fi_archiving = B_FALSE;
		} else {
			fs->fi_archiving = B_TRUE;
		}
	}
	return (0);
}

/*
 * This function sets the default value of fields that are dependent on
 * other options. It only sets them if they have not been explicitly set.
 * It does not do checking of the interrelated values. It also does not
 * set any change_flags for these fields as these are default values.
 *
 * It must NOT overwrite any explicitly set options.
 */
static int
set_dependent_default_values(mount_options_t *mo) {

	/* if sync_meta is not explicitly set */
	if (!(mo->change_flag & MNT_SYNC_META)) {
		/* if fs is shared sync_meta defaults to 1 else 0 */
		if (mo->sharedfs_opts.shared || mo->multireader_opts.reader ||
			mo->multireader_opts.writer) {

			mo->sync_meta = 1;
		} else {
			mo->sync_meta = 0;
		}
	}

	/* if partial_stage is not explicitly set */
	if (!(mo->sam_opts.change_flag & MNT_PARTIAL_STAGE)) {
		/* set partial_stage to partial */
		mo->sam_opts.partial_stage = mo->sam_opts.partial;
	}

	if (!(mo->readonly & MNT_READONLY)) {
		if (mo->multireader_opts.reader &&
		    (mo->multireader_opts.change_flag &
		    (MNT_READER | MNT_SHARED_READER))) {
			mo->readonly = B_TRUE;
		}
	}
	return (0);
}

/*
 * get a list of parsing errors from the most recent parser run.
 */
int
get_samfs_cmd_parsing_errors(
sqm_lst_t **l)	/* malloced list of parsing_error_t */
{

	return (dup_parsing_error_list(error_list, l));
}


/*
 * initialize static variables needed in each parser run.
 */
static int
init_static_variables(void)
{

	Trace(TR_OPRMSG, "initializing static variables");

	file_required = B_FALSE;
	fname = NULL;
	mnt_cfg = NULL;
	mp_all = mp = NULL;
	*err_msg = '\0';
	*dir_name = '\0';
	*token = '\0';
	no_cmd_file = B_FALSE;
	*open_error = '\0';

	if (error_list != NULL) {
		lst_free_deep(error_list);
	}
	error_list = lst_create();
	if (error_list == NULL) {
		Trace(TR_DEBUG, "initializing static variables failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* create a fs_t for use when unknown filesystems are encountered */
	memset((void *)&error_fs, 0, sizeof (fs_t));
	error_fs.mount_options = &error_opts;
	memset(error_fs.mount_options, 0, sizeof (mount_options_t));

	Trace(TR_OPRMSG, "initialized static variables");
	return (0);
}


/*
 * build a mount options for each fs and set its defaults.
 */
static int
build_fs_mnt_opts(void)
{

	mount_options_t *o;
	fs_t *fs;
	node_t *node;

	Trace(TR_OPRMSG, "building fs mount options");
	if (mnt_cfg->fs_list == NULL) {
		mnt_cfg->fs_list = lst_create();

		if (mnt_cfg->fs_list == NULL) {
			Trace(TR_DEBUG, "building fs mount options failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	if (mnt_cfg->fs_list->length == 0 || strcmp(GLOBAL,
	    ((fs_t *)mnt_cfg->fs_list->head->data)->fi_name) != 0) {

		fs = (fs_t *)mallocer(sizeof (fs_t));
		if (fs == NULL) {
			Trace(TR_DEBUG, "building fs mount options failed: %s",
			    samerrmsg);
			return (-1);
		}
		memset(fs, 0, sizeof (fs_t));

		fs->mount_options = (mount_options_t *)mallocer(
		    sizeof (mount_options_t));

		if (fs->mount_options == NULL) {
			Trace(TR_DEBUG, "building fs mount options failed: %s",
			    samerrmsg);
			return (-1);
		}



		memset(fs->mount_options, 0, sizeof (mount_options_t));
		lst_append(mnt_cfg->fs_list, fs);
		mp_all = fs;
	} else {
		mp_all = (fs_t *)mnt_cfg->fs_list->head->data;
	}


	mp = mp_all;

	/*
	 * set up the defaults.
	 */
	SetFieldDefaults(mp_all->mount_options, cfg_mount_params);
	reset_mount_options_change_flag(mp_all->mount_options);
	strcpy(mp_all->fi_name, GLOBAL);

	/* if the license says QFS only set archive (sam) to false */
	if (get_samfs_type(NULL) == QFS) {
		mp_all->mount_options->sam_opts.archive = B_FALSE;
	}


	for (node = mnt_cfg->fs_list->head; node != NULL;
	    node = node->next) {


		if (strcmp(((fs_t *)node->data)->fi_name, GLOBAL) == 0) {
			continue;
		}

		o = ((fs_t *)node->data)->mount_options;

		if (o == NULL) {
			((fs_t *)node->data)->mount_options =
			    (mount_options_t *)mallocer(
			    sizeof (mount_options_t));

			if (((fs_t *)node->data)->mount_options == NULL) {
				Trace(TR_DEBUG, "%s %s",
				    " building fs mount options failed: ",
				    samerrmsg);

				return (-1);
			}
		}


		memset(o, 0, sizeof (mount_options_t));

	}

	Trace(TR_OPRMSG, "built fs mount options");
	return (0);
}


/*
 * clear all of the change flags in the mount_options and its sub structures.
 */
static int
reset_mount_options_change_flag(mount_options_t *mo)
{

	mo->change_flag = 0;
	mo->io_opts.change_flag = 0;
	mo->sam_opts.change_flag = 0;
	mo->sharedfs_opts.change_flag = 0;
	mo->multireader_opts.change_flag = 0;
	mo->qfs_opts.change_flag = 0;
	mo->post_4_2_opts.change_flag = 0;
	mo->rel_4_6_opts.change_flag = 0;
	mo->rel_5_0_opts.change_flag = 0;
	mo->rel_5_64_opts.change_flag = 0;
	return (0);
}


/*
 * get the default mount options with no dependency on dau, type etc.
 */
int
get_default_mount_opts(mount_options_t **defs)
{

	Trace(TR_OPRMSG, "getting base default mount options");

	*defs = (mount_options_t *)mallocer(sizeof (mount_options_t));
	if (*defs == NULL) {
		Trace(TR_DEBUG, "getting base default mount options failed: %s",
		    samerrmsg);
		return (-1);
	}
	memset(*defs, 0, sizeof (mount_options_t));
	SetFieldDefaults(*defs, cfg_mount_params);

	(*defs)->change_flag = (uint32_t)0;
	(*defs)->io_opts.change_flag = (uint32_t)0;
	(*defs)->sam_opts.change_flag = (uint32_t)0;
	(*defs)->sharedfs_opts.change_flag = (uint32_t)0;
	(*defs)->multireader_opts.change_flag = (uint32_t)0;
	(*defs)->qfs_opts.change_flag = (uint32_t)0;
	(*defs)->post_4_2_opts.change_flag = (uint32_t)0;
	(*defs)->rel_4_6_opts.change_flag = (uint32_t)0;
	(*defs)->rel_5_0_opts.change_flag = (uint32_t)0;
	(*defs)->rel_5_64_opts.change_flag = (uint32_t)0;

	set_dependent_default_values(*defs);

	Trace(TR_OPRMSG, "got base default mount options");
	return (0);
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

	node_t *node;
	boolean_t fs_found = B_FALSE;
	fs_t *tmp;
	Trace(TR_OPRMSG, "processing fs directive for %s", token);

	for (node = mnt_cfg->fs_list->head;
	    node != NULL; node = node->next) {
		if (strcmp(token, ((fs_t *)node->data)->fi_name) == 0) {
			tmp = (fs_t *)node->data;
			fs_found = B_TRUE;
			break;
		}
	}
	if (!fs_found) {
		/*
		 * the fs does not exist in the mcf. In order to continue
		 * to parse, use an error fs as a standin. Also even though
		 * this is an error if the current options up to now were
		 * the globals- copy the global settings into all of the
		 * filesystems.
		 */
		if (mp == mp_all) {
			copy_defaults();
		}

		mp = &error_fs;
		Trace(TR_OPRMSG, "fs directive for non-existant fs %s", token);
		ReadCfgError(0, "Filesystem %s not defined", token);

	}

	/*
	 * if current mp was global- you were doing globals but
	 * you now know you are done getting global values so copy them into
	 * all of the file systems structures.
	 */
	if (mp == mp_all) {
		copy_defaults();
	}
	mp = tmp;
	Trace(TR_OPRMSG, "processed fs directive");
}


/*
 * cmd = value
 */
static void cmdValue(void)
{

	/*
	 * Set cmd (= value).
	 */
	if (SetFieldValue(mp->mount_options, cfg_mount_params, dir_name,
	    token, samfs_msg_func) != 0) {

		if (errno == ENOENT) {
			/* \"%s\" is not a recognized directive name */
			ReadCfgError(14005, dir_name);
		}
		ReadCfgError(0, err_msg);
	}
}


/*
 * Copy defaults. Copy the defaults into the other structures.
 */
static void
copy_defaults(void)
{

	node_t *node;


	for (node = mnt_cfg->fs_list->head; node != NULL; node = node->next) {
		fs_t *cur = node->data;

		if (strcmp(cur->fi_name, GLOBAL) == 0) {
			continue;
		}

		Trace(TR_OPRMSG, "copying defaults from %s to %s",
		    mp_all->fi_name, cur->fi_name);

		/* copy over all of the mount options */
		memcpy(cur->mount_options, mp_all->mount_options,
		    sizeof (mount_options_t));

		/*
		 * reset the change flags because nothing that got copied over
		 * is an explicit set.
		 */
		reset_mount_options_change_flag(cur->mount_options);
	}

	Trace(TR_OPRMSG, "copied default mount options");

}


/*
 * Data field error message function.
 */
static void
samfs_msg_func(
int code	/* ARGSUSED */,
char *msg)	/* error message */
{

	strlcpy(err_msg, msg, sizeof (err_msg)-1);
}


/*
 * Process command file processing message.
 */
static void
handle_msg(
char *msg,	/* error message */
int lineno,	/* line number of error line */
char *line)	/* line in file that contained error */
{

	parsing_error_t *err;
	char		err_buf[80];


	if (line != NULL) {
		/*
		 * error was encountered hile reading the file.
		 */
		if (msg != NULL) {

			/*
			 * Error message.
			 */
			Trace(TR_OPRMSG, "samfs.cmd line %d: %s\n",
			    lineno, line);

			Trace(TR_OPRMSG, "samfs.cmd error %s\n", msg);
			err = (parsing_error_t *)malloc(
			    sizeof (parsing_error_t));
			if (err == NULL) {
				out_of_memory = B_TRUE;
				return;
			}
			strlcpy(err->input, line, sizeof (err->input));
			strlcpy(err->msg, msg, sizeof (err->msg));
			err->line_num = lineno;
			err->error_type = ERROR;
			err->severity = 1;
			lst_append(error_list, err);
		}

	} else if (lineno >= 0) {
		Trace(TR_OPRMSG, "samfs.cmd with null line: %s", msg);
	} else if (lineno < 0) {
		if (errno == ENOENT) {
			no_cmd_file = B_TRUE;
			Trace(TR_OPRMSG, "samfs.cmd no cmd file");
		} else {
			no_cmd_file = B_FALSE;
			snprintf(open_error, sizeof (open_error),
			    StrFromErrno(errno, err_buf,
			    sizeof (err_buf)));

			Trace(TR_OPRMSG, "%s '%s': %s\n",
			    "cannot open samfs.cmd file", fname,
			    open_error);

		}
	}
}


/*
 * function to oversee the actual writing of the samfs cmd file.
 */
int
write_samfs_cmd(
char *location,		/* location at which to write the file */
mount_cfg_t *cfg)	/* cfg to write */
{

	FILE *f = NULL;
	int fd;
	time_t the_time;
	node_t *node;
	char err_buf[256];
	Trace(TR_OPRMSG, "writing samfs.cmd to %s", Str(location));

	if ((fd = open(location, O_WRONLY|O_CREAT|O_TRUNC, 0644)) != -1) {
		f = fdopen(fd, "w");
	}

	if (f == NULL) {
		samerrno = SE_CFG_OPEN_FAILED;
		/* Open failed for %s: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), location, err_buf);

		Trace(TR_DEBUG, "writing samfs.cmd to %s failed: %s",
		    Str(location), samerrmsg);

		return (-1);
	}



	fprintf(f, "#\n#\tsamfs.cmd\n");
	fprintf(f, "#\n");
	the_time = time(0);
	fprintf(f, "#\tGenerated by config api %s#\n", ctime(&the_time));


	for (node = cfg->fs_list->head; node != NULL; node = node->next) {

		if (strcmp(((fs_t *)node->data)->fi_name, GLOBAL) == 0) {
			write_fs_cmds(f, (fs_t *)node->data);
		} else {
			write_fs_cmds(f, (fs_t *)node->data);
		}

	}
	fflush(f);
	fclose(f);

	Trace(TR_OPRMSG, "wrote samfs.cmd");
	return (0);
}


/*
 * only parameters in o that are explicitly set will be written to the file.
 */
static int
write_fs_cmds(
FILE *f,
fs_t *o)	/* file system to write mount options */
{

	struct fieldVals *table = cfg_mount_params;
	boolean_t done_once = B_FALSE;
	char buf[100];
	char *fs_line = NULL;
	uint32_t defbits = 0;

	Trace(TR_OPRMSG, "wrote fs mount options for %s", o->fi_name);

	if (*o->fi_name != '\0' && strcmp(o->fi_name, GLOBAL) != 0) {
		snprintf(buf, sizeof (buf), "#\n#\n%s%s%s\n",
		    "fs", EQUALS, o->fi_name);
		fs_line = buf;
	}


	/* print fields if they are explicitly set */
	while (table->FvName != NULL) {

		if (table->FvType == DEFBITS) {
			defbits = *(uint32_t *)(void *)(((char *)
			    o->mount_options) + table->FvLoc);
		}

		if (is_explicit_set(defbits, table)) {

			print_field(f, o->mount_options, table, fs_line,
			    &done_once);
		}
		table++;
	}

	Trace(TR_OPRMSG, "wrote fs mount options for %s", o->fi_name);
	return (0);
}


/*
 * print val to the file as the type indicated in the fieldVals struct.
 */
static int
print_field(
FILE *f,
void *val,			/* structure containing field to print */
struct fieldVals *entry,	/* entry describing the structure val */
char *print_first,		/* string to print only one time */
boolean_t *printed)		/* true if print_first has been printed */
{

	char *str;
	str = get_cfg_str(val, entry, B_FALSE, NULL, 0, B_FALSE,
	    non_printing_mount_options);
	if (str == NULL) {
		return (0);
	}
	if (!(*printed) && *str != '\0' && print_first != NULL) {
		fprintf(f, print_first);
		*printed = B_TRUE;
	}
	if (*str != '\0') {
		if (print_first != NULL) {
			fprintf(f, "\t%s\n", str);
		} else {
			fprintf(f, "%s\n", str);
		}
	}

	return (0);
}


/*
 * copy all set fields from input to fs_to_file if their change flag is set
 * the resulting fs_to_file will have all fields set that it previously had
 * aswell as all that were set in input.
 */
int
copy_fs_set_fields(
fs_t *fs_to_file,	/* where to merge the options */
fs_t *input)		/* mount options to merge */
{

	struct fieldVals *table = cfg_mount_params;
	mount_options_t save;	/* save the change flags for merge. */
	char buf[100];
	uint32_t defbits;
	uint32_t *savebits;


	Trace(TR_OPRMSG, "copying set mount options");

	/*
	 * save all flag values and set fs_to_file flags to zero so
	 * duplicate checks will pass in setfield
	 */
	save.io_opts.change_flag =
	    fs_to_file->mount_options->io_opts.change_flag;
	fs_to_file->mount_options->io_opts.change_flag = 0;

	save.sam_opts.change_flag =
	    fs_to_file->mount_options->sam_opts.change_flag;
	fs_to_file->mount_options->sam_opts.change_flag = 0;

	save.sharedfs_opts.change_flag =
	    fs_to_file->mount_options->sharedfs_opts.change_flag;
	fs_to_file->mount_options->sharedfs_opts.change_flag = 0;

	save.multireader_opts.change_flag =
	    fs_to_file->mount_options->multireader_opts.change_flag;
	fs_to_file->mount_options->multireader_opts.change_flag = 0;

	save.qfs_opts.change_flag =
	    fs_to_file->mount_options->qfs_opts.change_flag;
	fs_to_file->mount_options->qfs_opts.change_flag = 0;

	save.post_4_2_opts.change_flag =
	    fs_to_file->mount_options->post_4_2_opts.change_flag;
	fs_to_file->mount_options->post_4_2_opts.change_flag = 0;

	save.rel_4_6_opts.change_flag =
	    fs_to_file->mount_options->rel_4_6_opts.change_flag;
	fs_to_file->mount_options->rel_4_6_opts.change_flag = 0;

	save.rel_5_0_opts.change_flag =
	    fs_to_file->mount_options->rel_5_0_opts.change_flag;
	fs_to_file->mount_options->rel_5_0_opts.change_flag = 0;

	save.rel_5_64_opts.change_flag =
	    fs_to_file->mount_options->rel_5_64_opts.change_flag;
	fs_to_file->mount_options->rel_5_64_opts.change_flag = 0;

	save.change_flag = fs_to_file->mount_options->change_flag;
	fs_to_file->mount_options->change_flag = 0;




	/* loop over all Field definitions */
	while (table->FvName != NULL) {

		if (table->FvType == DEFBITS) {
			defbits = *(uint32_t *)(void *)(((char *)
			    input->mount_options) + table->FvLoc);

			/* savebits is needed so things can be unset */
			savebits = (uint32_t *)(void *)(((char *)
			    &save) + table->FvLoc);
			table++;
			continue;
		}



		/*
		 * if the input has its change flag set- set the value
		 * in the fs_to_file
		 */
		if (is_explicit_set(defbits, table) == B_TRUE) {
			int type;
			boolean_t ret = B_FALSE;

			if (is_reset_value(defbits, table, &ret) != 0) {
				Trace(TR_DEBUG, "copy failed for %s:%s",
				    table->FvName, err_msg);
				Trace(TR_DEBUG, "%s%s",
				    "copying set mount options failed: ",
				    samerrmsg);
				return (-1);
			}
			if (ret == B_TRUE) {
				/*
				 * need to clear the flag in the
				 * destination struct and continue.
				 */
				*savebits &= ~(table->FvDefBit);
				table++;
				continue;
			}


			if (NULL == get_cfg_str(input->mount_options, table,
			    B_TRUE, buf, 100, B_FALSE, NULL)) {
				table++;
				continue;
			}
			/*
			 * if no string don't call setfield unless this is for
			 * a flag type. If it is a flag type call setfield if
			 * the value is B_TRUE.
			 */
			type = table->FvType & ~FV_FLAGS;
			if (type == FLAG || type == SETFLAG ||
			    type == CLEARFLAG) {

				/*
				 * If the buffer is empty it means the
				 * flag is not set in the input. This means
				 * it must be not be written to the file.
				 * Accomplish this by clearing the change_flag
				 * for it.
				 */
				if (*buf == '\0') {
					*savebits &= ~(table->FvDefBit);
					table++;
					continue;
				}

				/*
				 * if the buffer contained something the
				 * flag is set. But since this type has no
				 * value set the buf to empty.
				 */
				*buf = '\0';

			} else if (*buf == '\0') {
				table++;
				continue;
			}

			Trace(TR_OPRMSG, "%s", buf);
			if (SetFieldValue(fs_to_file->mount_options,
			    cfg_mount_params, table->FvName, buf,
			    samfs_msg_func) != 0) {

				samerrno = SE_COPY_FAILED;
				/* Copy failed for %s:%s */
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(SE_COPY_FAILED),
				    table->FvName, err_msg);
				Trace(TR_DEBUG, "copy failed for %s:%s",
				    table->FvName, err_msg);
				Trace(TR_DEBUG, "%s%s",
				    "copying set mount options",
				    samerrmsg);
				return (-1);
			}
		}
		table++;
	}


	/* restore the change flags that were saved earlier */
	fs_to_file->mount_options->io_opts.change_flag |=
	    save.io_opts.change_flag;

	fs_to_file->mount_options->sam_opts.change_flag |=
	    save.sam_opts.change_flag;

	fs_to_file->mount_options->sharedfs_opts.change_flag |=
	    save.sharedfs_opts.change_flag;

	fs_to_file->mount_options->multireader_opts.change_flag |=
	    save.multireader_opts.change_flag;

	fs_to_file->mount_options->qfs_opts.change_flag |=
	    save.qfs_opts.change_flag;

	fs_to_file->mount_options->post_4_2_opts.change_flag |=
	    save.post_4_2_opts.change_flag;

	fs_to_file->mount_options->rel_4_6_opts.change_flag |=
	    save.rel_4_6_opts.change_flag;

	fs_to_file->mount_options->rel_5_0_opts.change_flag |=
	    save.rel_5_0_opts.change_flag;

	fs_to_file->mount_options->change_flag |= save.change_flag;

	Trace(TR_OPRMSG, "copied set mount options");
	return (0);
}


/*
 * field diff returns 0 if the two structures contain the same value
 * for the field described by entry. Field diff returns 1 if the
 * values are different for the field;
 */
int
field_diff(
void *st1,			/* struct of type described in entry */
void *st2,			/* struct of type described in entry */
struct fieldVals *entry,	/* entry describing the structure */
void(*samfs_msg_func)(int code, char *msg)) /* error handling function */
{

	void	*v1, *v2;
	int		type;

	Trace(TR_OPRMSG, "field_diff(%s)", entry->FvName);

	v1 = (char *)st1 + entry->FvLoc;
	v2 = (char *)st2 + entry->FvLoc;
	type = entry->FvType & ~FV_FLAGS;

	switch (type) {

	case ENUM:
	case INTERVAL:
	case MUL8:
	case PWR2:
	case INT:
		if (*(int *)v1 == *(int *)v2) {
			goto same;
		}
		break;
	case INT16:
		if (*(int16_t *)v1 == *(int16_t *)v2) {
			goto same;
		}
		break;

	case FSIZE:
	case MULL8:
	case INT64:
		if (*(int64_t *)v1 == *(int64_t *)v2) {
			goto same;
		}
		break;

	case DOUBLE:
		if (*(double *)v1 == *(double *)v2) {
			goto same;
		}
		break;

	case FLOAT:
		if (*(float *)v1 == *(float *)v2) {
			goto same;
		}
		break;

	case SETFLAG:
	case CLEARFLAG:
	case FLAG: {
		struct fieldFlag *vals = (struct fieldFlag *)entry->FvVals;

		if (((*(uint32_t *)v1 ^ *(uint32_t *)v2) & vals->mask) == 0) {
			goto same;
		}
	}
	break;

	case FUNC: {
		struct fieldFunc *vals = (struct fieldFunc *)entry->FvVals;

		if (vals->diff != NULL) {
			if (vals->diff(v1, v2)) {
				goto same;
			}
		}
	}
	break;

	case MEDIA:
		if (*(media_t *)v1 == *(media_t *)v2) {
			goto same;
		}
		break;
	case STRING:
		if (strcmp((char *)v1, (char *)v2) == 0) {
			goto same;
		}
		break;

	case DEFBITS:
	default:
		goto same;
		break;	/* NOTREACHED */
	}

	if (samfs_msg_func != NULL) {
		samfs_msg_func(0, entry->FvName);
	}

	/* They are different */
	Trace(TR_DEBUG, "field_diff(%s) they are different",
	    entry->FvName);

	return (1);

same:
	Trace(TR_OPRMSG, "field_diff(%s) they are the same",
	    entry->FvName);

	return (0);
}
