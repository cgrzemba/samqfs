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
/*
 *	releaser.c -  APIs for recycler.h.
 *	It calls functions of cfg_release.c and process
 *	the detailed releaser.cmd operation.
 */
#pragma	ident	"$Revision: 1.26 $"
/* ANSI headers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* POSIX headers. */
#include <fcntl.h>
#include <unistd.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/mount.h"
#include "sam/lib.h"

#include <sys/types.h>
#include <time.h>

/* mgmt API headers. */
#include "mgmt/util.h"
#include "pub/mgmt/error.h"
#include "mgmt/config/common.h"
#include "mgmt/config/releaser.h"
#include "sam/sam_trace.h"
#include "pub/mgmt/process_job.h"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 *	The followings need to be included in sam's
 *	file instead of defining it here
 */
#define	DEFAULT_RL_LOG	""
#define	DEFAULT_RL_WEIGHT_AGE	1.0
#define	DEFAULT_RL_WEIGHT_AGE_TYPE	SIMPLE_AGE_PRIO
#define	DEFAULT_RL_WEIGHT_AGE_ACCESS	1.0
#define	DEFAULT_RL_WEIGHT_AGE_RESIDENCE	1.0
#define	DEFAULT_RL_WEIGHT_AGE_MODIFY	1.0
#define	DEFAULT_RL_WEIGHT_SIZE	1.0
#define	DEFAULT_RL_NO_RELEASE	B_FALSE
#define	DEFAULT_RL_REARCH_NO_RELEASE	B_FALSE
#define	DEFAULT_RL_DISPLAY_ALL_CANDIDATES	B_FALSE
#define	DEFAULT_RL_MIN_RESIDENCE	600
#define	DEFAULT_RL_LIST_SIZE	10000

static char *releaser_file = RELEASE_CFG;
static int dup_rl_fs_directive(const rl_fs_directive_t *in,
    rl_fs_directive_t **out);

static int expand_releaser_options(int32_t options, int32_t partial_sz,
    char *buf, int len);

static int display_release_activity(samrthread_t *ptr, char **result);


#define	RELEASE_FILES_CMD BIN_DIR"/release"


/*
 *	ARGSUSED is used by RPC clients to specify which
 *	connection should be used.
 *	see sammgmt_rpc.h for details.
 *	Also it is used to locate configuration file to dump
 *	configuration file for write functions.
 */


/*
 *	get_default_rl_fs_directive()
 *	Function to get the default release.cmd's
 *	file system prameters.
 */
int
get_default_rl_fs_directive(
ctx_t *ctx,				/* ARGSUSED */
rl_fs_directive_t **rl_fs_directive)	/* must be freed by caller */
{

	Trace(TR_MISC, "get default releaser's file system directive");
	*rl_fs_directive = NULL;
	if (ISNULL(rl_fs_directive)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	allocate memory for *rl_fs_directive and this
	 *	memory must be released in the calling function.
	 */
	*rl_fs_directive = (rl_fs_directive_t *)
	    mallocer(sizeof (rl_fs_directive_t));
	if (*rl_fs_directive == NULL) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	memset(*rl_fs_directive, 0, sizeof (rl_fs_directive_t));
	strlcpy((*rl_fs_directive)->releaser_log, DEFAULT_RL_LOG,
	    sizeof ((*rl_fs_directive)->releaser_log));
	(*rl_fs_directive)->size_priority = DEFAULT_RL_WEIGHT_SIZE;
	(*rl_fs_directive)->type = DEFAULT_RL_WEIGHT_AGE_TYPE;
	(*rl_fs_directive)->age_priority.detailed.access_weight
	    = DEFAULT_RL_WEIGHT_AGE_ACCESS;
	(*rl_fs_directive)->age_priority.detailed.modify_weight
	    = DEFAULT_RL_WEIGHT_AGE_MODIFY;
	(*rl_fs_directive)->age_priority.detailed.residence_weight
	    = DEFAULT_RL_WEIGHT_AGE_RESIDENCE;
	(*rl_fs_directive)->age_priority.simple
	    = DEFAULT_RL_WEIGHT_AGE;
	(*rl_fs_directive)->no_release = DEFAULT_RL_NO_RELEASE;
	(*rl_fs_directive)->rearch_no_release = DEFAULT_RL_REARCH_NO_RELEASE;
	(*rl_fs_directive)->display_all_candidates =
	    DEFAULT_RL_DISPLAY_ALL_CANDIDATES;
	(*rl_fs_directive)->min_residence_age = DEFAULT_RL_MIN_RESIDENCE;
	(*rl_fs_directive)->list_size = DEFAULT_RL_LIST_SIZE;
	Trace(TR_MISC, "get default releaser's file system directive success");
	return (0);
}


/*
 *	get_all_rl_fs_directives()
 *	Function to get all file system directive of releaser.cmd.
 */
int
get_all_rl_fs_directives(
ctx_t *ctx,			/* ARGSUSED */
sqm_lst_t **rl_fs_directives)	/* must be freed by caller */
{
	releaser_cfg_t *releaser;

	Trace(TR_MISC, "get all releaser's file system directives");
	*rl_fs_directives = NULL;
	if (ISNULL(rl_fs_directives)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	read releaser.cmd file to get all
	 *	releaser.cmd information.
	 */
	if (read_releaser_cfg(&releaser) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    releaser_file, samerrmsg);
		return (-1);
	}
	*rl_fs_directives = releaser->rl_fs_dir_list;
	releaser->rl_fs_dir_list = NULL;
	free_releaser_cfg(releaser);
	Trace(TR_MISC, "get all releaser's file system directives success");
	return (0);
}


/*
 *	get_rl_fs_directives()
 *	get the release directives for a file system; use the constant
 *	GLOBAL from types.h as fs_name for the global directive.
 */
int
get_rl_fs_directive(
ctx_t *ctx,				/* ARGSUSED */
const uname_t fs_name,			/* file ssytem name */
rl_fs_directive_t **rl_fs_directives)	/* must be freed by caller */
{
	releaser_cfg_t *releaser;
	rl_fs_directive_t *rl_fsname;
	node_t *node_c;
	int match_flag = 0;

	Trace(TR_MISC, "get releaser's file system directive");
	*rl_fs_directives = NULL;
	if (ISNULL(rl_fs_directives)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	read releaser.cmd file to get all
	 *	releaser.cmd information.
	 */
	if (read_releaser_cfg(&releaser) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    releaser_file, samerrmsg);
		return (-1);
	}
	node_c = (releaser->rl_fs_dir_list)->head;
	while (node_c != NULL) {
		rl_fsname = (rl_fs_directive_t *)node_c ->data;
		if (strcmp(rl_fsname->fs, fs_name) == 0) {
			*rl_fs_directives = rl_fsname;
			node_c ->data = NULL;
			match_flag = 1;
			break;
		}
		node_c = node_c->next;
	}
	free_releaser_cfg(releaser);
	if (match_flag == 0) {
		samerrno = SE_NO_RL_FS_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NO_RL_FS_FOUND), fs_name);
		Trace(TR_ERR, "%s", samerrmsg);
		*rl_fs_directives = NULL;
		return (-1);
	}
	Trace(TR_MISC, "get releaser's file system directive success");
	return (0);
}


/*
 *	set_rl_fs_directives()
 *	Functions to add/modify releaser file policy
 *	for a particular file name.
 */
int
set_rl_fs_directive(
ctx_t *ctx,				/* ARGSUSED */
rl_fs_directive_t *rl_fs_directives)	/* Not freed in this funuction */
{
	releaser_cfg_t *releaser;
	rl_fs_directive_t *rl_fsname;
	node_t *node_c;
	int match_flag = 0;

	Trace(TR_MISC, "set releaser's file system directives");
	if (ISNULL(rl_fs_directives)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	read releaser.cmd file to get all
	 *	releaser.cmd information.
	 */
	if (read_releaser_cfg(&releaser) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    releaser_file, samerrmsg);
		return (-1);
	}
	/*
	 *	Following block traverses rl_fs_dir_list
	 *	list, to find if given file system already
	 *	exist. It it exist, check each filed's bit.
	 *	If the field bit is set, update that field.
	 *	It file system name does not exist, returns error.
	 */
	node_c = (releaser->rl_fs_dir_list)->head;
	while (node_c != NULL) {
		rl_fsname = (rl_fs_directive_t *)node_c ->data;
		if (strcmp(rl_fsname->fs,
		    rl_fs_directives->fs) == 0) {
			match_flag = 1;
			break;
		}
		node_c = node_c->next;
	}
	if (match_flag == 0) {
		free_releaser_cfg(releaser);
		samerrno = SE_RL_FSNAME_NOT_EXIST_IN_MCF;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_RL_FSNAME_NOT_EXIST_IN_MCF),
		    rl_fs_directives->fs);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	free((rl_fs_directive_t *)node_c ->data);
	if (dup_rl_fs_directive(rl_fs_directives, &rl_fsname) != 0) {
		free_releaser_cfg(releaser);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	node_c ->data = rl_fsname;
	/*
	 *	write releaser.cmd with TRUE option
	 *	and it will force to write a new releaser.cmd.
	 */
	if (write_releaser_cfg(releaser, B_TRUE) != 0) {
		/* set nodes data to null so user arg not freed */
		node_c->data = NULL;
		free_releaser_cfg(releaser);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	Trace(TR_FILES, "new rl_fsname %s is modifed to releaser.cmd\n",
	    rl_fs_directives->fs);
	free_releaser_cfg(releaser);
	Trace(TR_MISC, "set releaser's file system directives success");
	return (0);
}


/*
 *	reset_rl_fs_directives()
 *	Functions to remove releaser file policy
 *	for a particular file name.
 */
int
reset_rl_fs_directive(
ctx_t *ctx,				/* ARGSUSED */
rl_fs_directive_t *rl_fs_directives)	/* Not freed in this function */
{
	releaser_cfg_t *releaser;
	rl_fs_directive_t *rl_fsname;
	node_t *node_c;
	int match_flag = 0;

	Trace(TR_MISC, "reset releaser's file system directives");
	if (ISNULL(rl_fs_directives)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	/*
	 *	read releaser.cmd file to get all
	 *	releaser.cmd information.
	 */
	if (read_releaser_cfg(&releaser) != 0) {
		Trace(TR_MISC, "Read of %s failed with error: %s",
		    releaser_file, samerrmsg);
		return (-1);
	}
	/*
	 *	Following block traverses rl_fs_dir_list
	 *	list, to find if given file system already
	 *	exist. It it exist, remove its list from
	 *	rl_fs_dir_list.
	 */
	node_c = (releaser->rl_fs_dir_list)->head;
	while (node_c != NULL) {
		rl_fsname = (rl_fs_directive_t *)node_c ->data;
		if (strcmp(rl_fsname->fs,
		    rl_fs_directives->fs) == 0) {
			if (lst_remove(releaser->rl_fs_dir_list, node_c) != 0) {
				goto error;
			}
			free(rl_fsname);
			match_flag = 1;
			break;
		}
		node_c = node_c->next;
	}
	if (match_flag == 0) {
		samerrno = SE_NO_RL_FS_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NO_RL_FS_FOUND),
		    rl_fs_directives->fs);
		goto error;
	}
	/*
	 *	write releaser.cmd with TRUE option
	 *	and it will force to write a new releaser.cmd.
	 */
	if (write_releaser_cfg(releaser, B_TRUE) != 0) {
		goto error;
	}
	Trace(TR_FILES, "rl_fsname %s is removed from releaser.cmd\n",
	    rl_fs_directives->fs);
	free_releaser_cfg(releaser);
	Trace(TR_MISC, "reset releaser's file system directives success");
	return (0);
error:
	free_releaser_cfg(releaser);
	Trace(TR_ERR, "%s", samerrmsg);
	return (-1);
}


/*
 * duplicate the incoming struct. The resulting out structure
 * is dynamically allocated so it can be freed.
 */
static int
dup_rl_fs_directive(
const rl_fs_directive_t *in,	/* incoming rl_fs_directive_t */
rl_fs_directive_t **out)	/* output rl_fs_directive_t */
{
	Trace(TR_OPRMSG, "duplicate releaser fs directive");
	*out = (rl_fs_directive_t *)mallocer(sizeof (rl_fs_directive_t));
	if (*out == NULL) {
		Trace(TR_OPRMSG, "%s", samerrmsg);
		return (-1);
	}
	memcpy(*out, in, sizeof (rl_fs_directive_t));
	Trace(TR_OPRMSG, "duplicate releaser fs directive success");
	return (0);
}


/*
 * functions to get a fs list which is doing release job.
 * the list is a list of structure release_fs_t;
 */
int
get_releasing_fs_list(
ctx_t *ctx,			/* ARGSUSED */
sqm_lst_t **releasing_fs_list)	/* a list of release_fs_t, */
				/* must be freed by caller */
{
	int numfs;		/* Number of filesystems */
	struct sam_fs_info   *finfo = NULL;
	struct sam_fs_status *fsarray;
	release_fs_t *rel_fs;
	int 	i;

	Trace(TR_MISC, "getting releasing file system");
	if ((numfs = GetFsStatus(&fsarray)) == -1) {
		samerrno = SE_GET_FS_STATUS_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "%s", samerrmsg);
	/*
	 *	returns -2.
	 */
		*releasing_fs_list = lst_create();
		if (*releasing_fs_list == NULL) {
			Trace(TR_ERR, "%s", samerrmsg);
			return (-1);
		}
		return (-2);
	}

	if (finfo != NULL) {
		free(finfo);
	}
	finfo = (struct sam_fs_info *)
	    mallocer(numfs * sizeof (struct sam_fs_info));
	if (finfo == NULL) {
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}


	for (i = 0; i < numfs; i++) {
		struct sam_fs_status *fs;
		struct sam_fs_info *fi;

		fs = fsarray + i;
		fi = finfo + i;
		if (GetFsInfo(fs->fs_name, fi) == -1) {
			samerrno = SE_GET_FS_INFO_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), fs->fs_name);
			Trace(TR_ERR, "%s", samerrmsg);
			goto error;
		}
		if (!(fi->fi_status & FS_MOUNTED)) {
			continue;
		}
	}
	free(fsarray);
	*releasing_fs_list = lst_create();
	if (*releasing_fs_list == NULL) {
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}

	for (i = 0; i < numfs; i++) {
		struct	sam_fs_info *fi;
		int	pct;		/* Disk free space percentage */

		fi = finfo + i;
		if (!(fi->fi_status & FS_MOUNTED)) {
			continue;
		}
		if (fi->fi_status & FS_RELEASING) {
			rel_fs = (release_fs_t *)
			    mallocer(numfs * sizeof (release_fs_t));
			if (rel_fs == NULL) {
				Trace(TR_ERR, "%s", samerrmsg);
				lst_free_deep(*releasing_fs_list);
				*releasing_fs_list = NULL;
				return (-1);
			}
			strlcpy(rel_fs->fi_name, fi->fi_name,
			    sizeof (rel_fs->fi_name));
			pct = llpercent_used(fi->fi_capacity, fi->fi_space);
			rel_fs->used_pct = 100 - pct;
			rel_fs->fi_low = fi->fi_low;
			if (lst_append(*releasing_fs_list, rel_fs) == -1) {
				free(rel_fs);
				lst_free_deep(*releasing_fs_list);
				Trace(TR_ERR, "%s", samerrmsg);
				*releasing_fs_list = NULL;
				return (-1);
			}
		}
	}
	if (finfo) {
		free(finfo);
	}
	Trace(TR_MISC, "finished getting releasing file system");
	return (0);
error:
	if (fsarray) {
		free(fsarray);
	}
	if (finfo) {
		free(finfo);
	}
	return (-1);
}


/*
 * release_files
 * Used to release files and directories and to set releaser
 * attributes for files and directories.
 *
 *
 * Attr Flags
 * RL_OPT_RECURSIVE
 * RL_OPT_NEVER
 * RL_OPT_WHEN_1
 * RL_OPT_PARTIAL
 * RL_OPT_DEFAULTS
 *
 * If any attrubute other than RL_OPT_RECURSIVE is specified the disk space
 * will not be released. Instead the indicated attributes will be set
 * for each file in the file list.
 *
 * The RL_OPT_NEVER & RL_OPT_ALWAYS_WHEN_1 are
 * mutually exclusive.
 *
 * PARAMS:
 *   sqm_lst_t  *files,	 IN - list of fully quallified file names
 *   int32_t options	 IN - bit fields indicate release options (see above).
 *   int32_t *partial_sz IN -
 * RETURNS:
 *   success -  0	operation successfully issued release not necessarily
 *			complete.
 *   error   -  -1
 */
int
release_files(ctx_t *c /* ARGSUSED */, sqm_lst_t *files, int32_t options,
    int32_t partial_sz, char **job_id) {

	char		buf[32];
	char		**command;
	node_t		*n;
	argbuf_t 	*arg;
	size_t		len = MAXPATHLEN * 2;
	boolean_t	found_one = B_FALSE;
	pid_t		pid;
	int		ret;
	int		status;
	FILE		*out;
	FILE		*err;
	exec_cleanup_info_t *cl;
	char release_s_buf[32];
	int arg_cnt;
	int cur_arg = 0;

	if (ISNULL(files, job_id)) {
		Trace(TR_ERR, "release files failed:%s", samerrmsg);
		return (-1);
	}


	/*
	 * Determine how many args to the command and create the command
	 * array. Note that command is malloced because the number of files
	 * is not known prior to execution. The arguments themselves need
	 * not be malloced because the child process will get a copy.
	 * Include space in the command array for:
	 * - the command
	 * - all possible options
	 * - an entry for each file in the list.
	 * - entry for the NULL
	 */
	arg_cnt = 1 + 6 + files->length + 1;
	command = (char **)calloc(arg_cnt, sizeof (char *));
	if (command == NULL) {
		setsamerr(SE_NO_MEM);
		Trace(TR_ERR, "release files failed:%s", samerrmsg);
		return (-1);
	}
	command[cur_arg++] = RELEASE_FILES_CMD;

	*buf = '\0';
	if (partial_sz) {
		snprintf(release_s_buf, sizeof (release_s_buf), "-s%d ",
		    partial_sz);
		command[cur_arg++] = release_s_buf;
	}
	if (options & RL_OPT_NEVER) {
		command[cur_arg++] = "-n";
	}
	if (options & RL_OPT_WHEN_1) {
		command[cur_arg++] = "-a";
	}
	if (options & RL_OPT_PARTIAL) {
		command[cur_arg++] = "-p";
	}
	if (options & RL_OPT_DEFAULTS) {
		command[cur_arg++] = "-d";
	}
	/* Recursive must be specified last */
	if (options & RL_OPT_RECURSIVE) {
		command[cur_arg++] = "-r";
	}

	/* make the argument buffer for the activity */
	arg = (argbuf_t *)mallocer(sizeof (releasebuf_t));
	if (arg == NULL) {
		free(command);
		Trace(TR_ERR, "release files failed:%s", samerrmsg);
		return (-1);
	}

	arg->rl.filepaths = lst_create();
	if (arg->rl.filepaths == NULL) {
		free(command);
		Trace(TR_ERR, "release files failed:%s", samerrmsg);
		return (-1);
	}
	arg->rl.options = options;
	arg->rl.partial_sz = partial_sz;

	/* add the file names to the cmd string and the argument buffer */
	for (n = files->head; n != NULL; n = n->next) {
		if (n->data != NULL) {
			char *cur_file;
			command[cur_arg++] = (char *)n->data;

			found_one = B_TRUE;
			cur_file = copystr(n->data);
			if (cur_file == NULL) {
				free(command);
				free_argbuf(SAMA_RELEASEFILES, arg);
				Trace(TR_ERR, "release files failed:%s",
				    samerrmsg);
				return (-1);
			}
			if (lst_append(arg->rl.filepaths, cur_file) != 0) {
				free(command);
				free(cur_file);
				free_argbuf(SAMA_RELEASEFILES, arg);
				Trace(TR_ERR, "release files failed:%s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	/*
	 * Check that at least one file was found.
	 */
	if (!found_one) {
		free(command);
		free_argbuf(SAMA_RELEASEFILES, arg);
		Trace(TR_ERR, "release files failed:%s", samerrmsg);
		return (-1);
	}

	/* create the activity */
	ret = start_activity(display_release_activity, kill_fork,
	    SAMA_RELEASEFILES, arg, job_id);
	if (ret != 0) {
		free(command);
		free_argbuf(SAMA_RELEASEFILES, arg);
		Trace(TR_ERR, "release files failed:%s", samerrmsg);
		return (-1);
	}

	/*
	 * create the cleanup struct prior to the exec because it is
	 * easier to cleanup here  if the malloc fails.
	 */
	cl = (exec_cleanup_info_t *)mallocer(sizeof (exec_cleanup_info_t));
	if (cl == NULL) {
		free(command);
		lst_free_deep(arg->rl.filepaths);
		end_this_activity(*job_id);
		Trace(TR_ERR, "release files failed, error:%d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	/* exec the process */
	pid = exec_mgmt_cmd(&out, &err, command);
	if (pid < 0) {
		free(command);
		lst_free_deep(arg->rl.filepaths);
		end_this_activity(*job_id);
		Trace(TR_ERR, "release files failed:%s", samerrmsg);
		return (-1);
	}
	free(command);
	set_pid_or_tid(*job_id, pid, 0);


	/* setup struct for call to cleanup */
	strlcpy(cl->func, RELEASE_FILES_CMD, sizeof (cl->func));
	cl->pid = pid;
	strlcpy(cl->job_id, *job_id, MAXNAMELEN);
	cl->streams[0] = out;
	cl->streams[1] = err;


	/* possibly return the results or async notification */
	ret = bounded_activity_wait(&status, 10, *job_id, pid, cl,
	    cleanup_after_exec_get_output);

	if (ret == -1) {
		samerrno = SE_RELEASE_FILES_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_RELEASE_FILES_FAILED));

		free(*job_id);
		*job_id = NULL;
		Trace(TR_ERR, "release files failed:%s", samerrmsg);
		return (-1);
	} else if (ret == 0) {
		/*
		 * job_id was set by start_activity. Clear it now
		 * so that the caller knows the request has been submitted.
		 */
		free(*job_id);
		*job_id = NULL;
		Trace(TR_MISC, "release files completed");
	}

	Trace(TR_MISC, "leaving release files");
	return (0);
}



static int
expand_releaser_options(int32_t options, int32_t partial_sz,
    char *buf, int len) {

	*buf = '\0';
	if (partial_sz) {
		snprintf(buf, len, "-s %d ", partial_sz);
	}
	if (options & RL_OPT_NEVER) {
		strlcat(buf, "-n ", len);
	}
	if (options & RL_OPT_WHEN_1) {
		strlcat(buf, "-a ", len);
	}
	if (options & RL_OPT_PARTIAL) {
		strlcat(buf, "-p ", len);
	}
	if (options & RL_OPT_DEFAULTS) {
		strlcat(buf, "-d ", len);
	}
	/* Recursive must be specified last */
	if (options & RL_OPT_RECURSIVE) {
		strlcat(buf, "-r ", len);
	}

	return (0);
}

static int
display_release_activity(samrthread_t *ptr, char **result) {
	char buf[MAXPATHLEN];
	char details[32];

	expand_releaser_options(ptr->args->rl.options,
	    ptr->args->rl.partial_sz, details, sizeof (details));

	snprintf(buf, sizeof (buf), "activityid=%s,starttime=%ld"
	    ",details=\"%s\",type=%s,pid=%d",
	    ptr->jobid, ptr->start, details, activitytypes[ptr->type],
	    ptr->pid);
	*result = copystr(buf);
	return (0);
}
