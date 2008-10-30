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
#pragma ident	"$Revision: 1.58 $"
static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */


/*
 * parse_archiver.c
 * code to parse the archiver.cmd file and populate archiver_cfg_t structs.
 */


/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* POSIX headers. */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>


/* Solaris headers. */
#include <libgen.h>


/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/lib.h"
#include "sam/readcfg.h"
#include "pub/mgmt/types.h"
#include "mgmt/util.h"
#include "mgmt/private_file_util.h"

/*
 * Local headers. The following header files are required but are not in
 * the include directory. They are being explicitly included from src to
 * distinguish them from files under include that have the same names
 */
#define	NEED_ARCHSET_NAMES
#define	NEED_EXAM_METHOD_NAMES
#define	NEED_FILEPROPS_NAMES

#include "src/archiver/archiver/archiver.h"
#include "src/archiver/include/archset.h"
#include "src/archiver/include/fileprops.h"

/* SAM-FS Headers added for API */

/* api header files */
#include "pub/devstat.h"
#include "pub/mgmt/device.h"
#include "pub/mgmt/sqm_list.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/types.h"
#include "mgmt/config/archiver.h"
#include "pub/mgmt/error.h"
#include "parser_utils.h"
#include "mgmt/config/cfg_diskvols.h"
#include "mgmt/util.h"
#include "pub/mgmt/filesystem.h"
#include "mgmt/config/media.h"

/* Directive functions. */
static void dirArchmax(void);
static void dirArchmeta(void);
static void dirBufsize(void);
static void dirDrives(void);
static void dirEndparams(void);
static void dirEndvsnpools(void);
static void dirEndvsns(void);
static void dirExamine(void);
static void dirFs(void);
static void dirInterval(void);
static void dirLogfile(void);
static void dirScanlist(void);
static void dirNoBegin(void);
static void dirNotify(void);
static void dirOvflmin(void);
static void dirTimeout(void);
static void dirParams(void);
static void dirVsnpools(void);
static void dirVsns(void);
static void dirWait(void);
static void dirSetarchdone(void);
static void dirBackGndInterval(void);
static void dirBackGndTime(void);
static void copyNorelease(void);
static void copyRelease(void);
static void notDirective(void);
static void notGlobalDirective(void);
static void paramsDiskArchive(void);
static void paramsInvalid(void);
static void paramsSetfield(void);
static void paramsFillvsns(void);

static void procParams(void);
static void procVsnpools(void);
static void procVsns(void);
static void propRelease(void);
static void propSetfield(void);
static void propStage(void);

/* Private data. */

/* Global only directives table */
static DirProc_t globalDirectives[] = {
	{ "archmax",		dirArchmax,	DP_value,	4410 },
	{ "bufsize",		dirBufsize,	DP_value,	4410 },
	{ "drives",		dirDrives,	DP_value,	4411 },
	{ "notify",		dirNotify,	DP_value,	4415 },
	{ "ovflmin",		dirOvflmin,	DP_value,	4410 },
	{ "timeout",		dirTimeout,	DP_value,	4410 },
	{ NULL,			notGlobalDirective, DP_other }
};


/* Directives table */
static DirProc_t directives[] = {
	{ "archivemeta",	dirArchmeta,	DP_value,	4410 },
	{ "endparams",		dirNoBegin,	DP_set	 },
	{ "endvsnpools",	dirNoBegin,	DP_set	 },
	{ "endvsns",		dirNoBegin,	DP_set	 },
	{ "examine",		dirExamine,	DP_value,	4537 },
	{ "fs",			dirFs,		DP_value,	4412 },
	{ "interval",		dirInterval,	DP_value,	4413 },
	{ "logfile",		dirLogfile,	DP_value,	4414 },
	{ "params",		dirParams,	DP_set	 },
	{ "vsnpools",		dirVsnpools,	DP_set	 },
	{ "vsns",		dirVsns,	DP_set	 },
	{ "wait",		dirWait,	DP_set	 },
	{ "scanlist_squash",	dirScanlist,	DP_value },
	{ "setarchdone",	dirSetarchdone,	DP_value },
	{ "background_interval", dirBackGndInterval, DP_value,	4548 },
	{ "background_time",	dirBackGndTime,	DP_value,	4549 },
	{ NULL,			notDirective,	DP_other }
};

static char dirName[TOKEN_SIZE];
static char errMsg[256];
static char token[TOKEN_SIZE];
static boolean_t peekedToken = B_FALSE;

static set_name_t asname;	/* Current archive set criteria name */
static boolean_t noGlobal = B_FALSE;	/* Don't allow a global directive */
static boolean_t noDefine = B_FALSE;	/* Don't define an archive set */

static boolean_t defining_metadata_copy;
static char *cmdFname;

static char *noEnd = NULL;	/* Which end statement would be missing */
static int copy;		/* Current copy */
static int errors;


/* API data */
static boolean_t v_allsets	= B_TRUE;
static boolean_t v_allsets_copy = B_TRUE;
static boolean_t p_allsets	= B_TRUE;
static boolean_t p_allsets_copy = B_TRUE;

archiver_cfg_t		*arch_cfg = NULL;
static sqm_lst_t		*fs_list = NULL;
static sqm_lst_t		*lib_list = NULL;
static ar_set_copy_params_t *cur_params = NULL; /* NOTE Mapped to as */
static ar_fs_directive_t *global_props = NULL;	/* NOTE: globalProps */
static ar_fs_directive_t *cur_fs = NULL;	/* NOTE: fpt */
static ar_fs_directive_t *err_fs = NULL;	/* for unknown filesystem. */


/*
 * err_crit is used when the crit being
 * handled has errors
 */
static ar_set_criteria_t *err_crit = NULL;

static ar_set_criteria_t *cur_crit = NULL; /* NOTE fpDef */
static ar_set_copy_cfg_t *cur_copy = NULL;
static struct ArchSet tmp_arch_set; /* for priority checking */
static sqm_lst_t *error_list = NULL;
static char open_error[80];	/* same size as returned from StrFromErrno */
static int mem_err = 0;
static boolean_t no_cmd_file = B_FALSE;
static boolean_t first_pass_other_copy;
static boolean_t first_pass_allsets_copy;

/* For use generating data class names */
static int criteria_index = 0;

static boolean_t parsing_class_file = B_FALSE;
static sqm_lst_t *classes;
/* end data class parsing structs and functions */

/* private api functions */
static boolean_t criteriaMatches(ar_set_criteria_t *, ar_set_criteria_t *);
static ar_fs_directive_t *find_fs(char *name);
static vsn_pool_t *find_vsn_pool(char *poolName);
static vsn_map_t *find_vsn_map(char *map_name);
static int check_vsn_exp(char *string);
static node_t *find_node_str(sqm_lst_t *l, char *str);
static int init_static_variables();
static int init_archiver_cfg(archiver_cfg_t *a);
static int init_fs_entries(archiver_cfg_t *a, sqm_lst_t *l);
static int strcmp_nulls(char *s1, char *s2);
static int set_criteria_keys(archiver_cfg_t *cfg);
static int find_arch_cp_params(char *name);


/* Private functions. */
static void assemble_age(uint_t *age, char *name, int max);
static int asmSetName(void);
static void asmSize(fsize_t *size, char *name);
static void checkRange(char *name, int64_t value, int64_t min, int64_t max);
static void defineCopy(void);
static void defineArchset(char *name);
static int init_copy_params(void);
static void msgFunc(char *msg, int lineno, char *line);
static void msgFuncSetField(int code, char *msg);
static char *normalizePath(char *path);
static void setDefaultParams(int which);
static void setup_global_directives_from_fs();



static int paramsPrioritySet(void *v, char *value, char *buf, int bufsize);
#define	paramsPriorityTostr NULL

static int paramsReserveSet(void *v, char *value, char *buf, int bufsize);
#define	paramsReserveTostr NULL

#include "aml/archset.hc"

static int propGroupSet(void *v, char *value, char *buf, int bufsize);
#define	propGroupTostr NULL

static int propNameSet(void *v, char *value, char *buf, int bufsize);
#define	propNameTostr NULL

static int propUserSet(void *v, char *value, char *buf, int bufsize);
#define	propUserTostr NULL


/*
 * This header must occur after the above methods as they
 * are used in the header
 */
#include "mgmt/config/archiver_fields.h"

/* handy defines */
#define	MAX_MEDIA_LEN	5


/*
 * parse the archiver file and populate the archiver_cfg_t structure cfg
 * if this function fails the archiver_cfg_t struct must be freed by caller.
 */
int
parse_archiver(
char *arch_file,		/* name of the file to parse */
sqm_lst_t *filesystems,   		/* list of fs_t to check against */
sqm_lst_t *libraries,		/* list of library_t to check against */
diskvols_cfg_t *disk_vols	/* ARGSUSED */,
archiver_cfg_t *cfg)		/* malloced return value */
{

	Trace(TR_FILES, "parsing archiver %s", arch_file);
	if (ISNULL(arch_file, filesystems, libraries, cfg)) {
		Trace(TR_OPRMSG, "parsing archiver failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * parser needs to be able to run repeatedly. Reset all globals
	 * to their initial values
	 */
	if (init_static_variables() != 0) {
		Trace(TR_OPRMSG, "parsing archiver failed: %s", samerrmsg);
		return (-1);
	}

	fs_list = filesystems;
	lib_list = libraries;
	cmdFname = arch_file;
	arch_cfg = cfg;

	/* clean out the incoming struct. */
	if (init_archiver_cfg(arch_cfg) != 0) {
		Trace(TR_OPRMSG, "parsing archiver failed: %s",
		    samerrmsg);
		return (-1);
	}


	/* Make copy params for allsets and fs metadata */
	if (init_copy_params() != 0) {
		Trace(TR_OPRMSG, "parsing archiver failed: %s", samerrmsg);
		return (-1);
	}



	if (init_ar_fs_directive(global_props) != 0) {
		Trace(TR_OPRMSG, "parsing archiver failed: %s", samerrmsg);
		return (-1);
	}
	strlcpy((char *)global_props->fs_name, GLOBAL,
	    sizeof (global_props->fs_name));


	if (init_fs_entries(arch_cfg, fs_list) != 0) {
		Trace(TR_OPRMSG, "parsing archiver failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * Begin with  so set global_props to be cur_fs
	 */
	cur_fs = global_props;
	cur_crit = NULL;

	/*
	 * Read the command file.
	 */
	errors = ReadCfg(cmdFname, directives, dirName, token, msgFunc);
	if (errors != 0) {
		if (errors == -1) {

			if (no_cmd_file) {
				/*
				 * The absence of a cmd file is not an error
				 * return 0 and the cfg that was initialized
				 * above. Also set the global wait directive
				 * since this is the new default behavior as
				 * of 5.0.
				 */
				setup_global_directives_from_fs();
				arch_cfg->global_dirs.wait = B_TRUE;
				Trace(TR_OPRMSG, "parsed archiver no file");

				return (0);
			}

			samerrno = SE_CFG_OPEN_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_CFG_OPEN_FAILED), cmdFname,
			    open_error);

			Trace(TR_OPRMSG, "parsing archiver failed: %s",
			    samerrmsg);
			return (-1);

		} else if (mem_err != 0) {
			setsamerr(mem_err);
			Trace(TR_OPRMSG, "parsing archiver failed: %s",
			    samerrmsg);
			return (-1);

		} else if (error_list->length != 0) {
			node_t *j;

			samerrno = SE_CONFIG_CONTAINED_ERRORS;
			Trace(TR_ERR, "Final archiver errors:");
			for (j = error_list->head; j != NULL; j = j->next) {
				parsing_error_t *pe =
				    (parsing_error_t *)j->data;
				if (pe != NULL) {
					Trace(TR_ERR, "line:%s", pe->input);
					Trace(TR_ERR, "error:%s", pe->msg);
				}
			}

			/* %s configuration contained %d errors */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_CONFIG_CONTAINED_ERRORS),
			    arch_file, errors);

			Trace(TR_OPRMSG, "parsing archiver failed: %s",
			    samerrmsg);

			return (-1);

		}
	}
	setup_global_directives_from_fs();

	setDefaultParams(1);
	setDefaultParams(0);


	set_criteria_keys(arch_cfg);
	Trace(TR_OPRMSG, "parsed archiver");
	return (0);
}


static void
setup_global_directives_from_fs(void)
{

	Trace(TR_DEBUG, "setting up global_directives from fs");
	/*
	 * copy global directives into the globals struct
	 * This is done so that during parsing the fs processing code can
	 * handle the global directives too.
	 */
	arch_cfg->global_dirs.wait = global_props->wait;
	arch_cfg->global_dirs.scan_method = global_props->scan_method;
	arch_cfg->global_dirs.archivemeta = global_props->archivemeta;

	if (global_props->fs_interval != 0) {
		arch_cfg->global_dirs.ar_interval = global_props->fs_interval;
	}
	if (*global_props->log_path != '\0') {
		strcpy(arch_cfg->global_dirs.log_path, global_props->log_path);
	}
	if (global_props->ar_set_criteria != NULL) {
		arch_cfg->global_dirs.ar_set_lst =
		    global_props->ar_set_criteria;
		global_props->ar_set_criteria = NULL;
	}
	arch_cfg->global_dirs.change_flag |= global_props->change_flag;
	arch_cfg->global_dirs.options |= global_props->options;
	arch_cfg->global_dirs.bg_interval = global_props->bg_interval;
	arch_cfg->global_dirs.bg_time = global_props->bg_time;

	free_ar_fs_directive(global_props);
	global_props = NULL;

	Trace(TR_DEBUG, "setup global directives from fs");
}


int
get_archiver_parsing_errors(
sqm_lst_t **l)	/* malloced list of parsing_error_t */
{

	if (ISNULL(l)) {
		/* leave samerrno as set */
		return (-1);
	}
	return (dup_parsing_error_list(error_list, l));
}


/*
 * sets up the criteria keys for all of the ar_set_criteria including the
 * no_archive criteria. This could possibly be done when a new criteria is
 * created for the one that was just done. For now it is simpler to do it
 * after they are all done.
 */
static int
set_criteria_keys(
archiver_cfg_t *cfg)
{

	node_t *out;
	node_t *in;
	char *str;
	ar_set_criteria_t *crit;

	Trace(TR_DEBUG, "setting criteria keys");

	if (cfg->global_dirs.ar_set_lst != NULL) {
		for (out = arch_cfg->global_dirs.ar_set_lst->head;
		    out != NULL; out = out->next) {

			/*
			 * This includes the copy config in
			 * the string that gets hashed. It is unclear if
			 * this is really required.
			 */
			crit = (ar_set_criteria_t *)out->data;
			str = criteria_to_str(crit);
			get_key(str, strlen(str), &(crit->key));

		}
	}


	for (out = arch_cfg->ar_fs_p->head; out != NULL; out = out->next) {
		ar_fs_directive_t *fs = (ar_fs_directive_t *)out->data;

		for (in = fs->ar_set_criteria->head; in != NULL;
		    in = in->next) {

			/*
			 * This includes the copy config in
			 * the string that gets hashed. It is unclear if
			 * this is really required.
			 */
			crit = (ar_set_criteria_t *)in->data;
			str = criteria_to_str(crit);
			get_key(str, strlen(str), &(crit->key));

			/*
			 * If the last criteria acts as an explicit default,
			 * set the explicit default flags for the fs and its
			 * last criteria
			 */
			if (in == fs->ar_set_criteria->tail &&
			    fs->archivemeta == B_FALSE &&
			    strcmp(crit->path, ".") == 0 &&
			    crit->change_flag == AR_ST_path) {
				fs->options |= FS_HAS_EXPLICIT_DEFAULT;
				crit->change_flag |= AR_ST_default_criteria;
			}
		}
	}

	Trace(TR_DEBUG, "set criteria keys");
	return (0);
}


/*
 * setup the initial archiver_cfg struct
 */
static int
init_archiver_cfg(
archiver_cfg_t *a)
{

	Trace(TR_DEBUG, "initializing archiver_cfg");

	/* clean out the incoming struct. */
	memset(a, 0, sizeof (archiver_cfg_t));
	a->ar_fs_p = lst_create();
	a->vsn_maps = lst_create();
	a->archcopy_params = lst_create();
	a->vsn_pools = lst_create();

	if (a->ar_fs_p == NULL || a->vsn_maps == NULL ||
	    a->archcopy_params == NULL || a->vsn_pools == NULL) {

		Trace(TR_OPRMSG, "initializing archiver_cfg exit %s",
		    samerrmsg);
		return (-1);

	}
	if (init_global_dirs(&(a->global_dirs), lib_list) != 0) {
		Trace(TR_OPRMSG, "initializing archiver_cfg exit %s",
		    samerrmsg);
		return (-1);
	}
	a->read_time = time(0);

	Trace(TR_DEBUG, "initialized archiver_cfg");
	return (0);
}

/*
 * create default initial global directives.
 */
int
init_global_dirs(
ar_global_directive_t *g,	/* global directives to initialize */
sqm_lst_t *libs)			/* list of library_t structs */
{

	buffer_directive_t *buf;
	sqm_lst_t *mt = NULL;
	node_t *n;

	Trace(TR_DEBUG, "initializing global directives");

	memset(g, 0, sizeof (ar_global_directive_t));
	g->ar_bufs = lst_create();
	g->ar_max = lst_create();
	g->ar_overflow_lst = lst_create();
	g->timeouts = lst_create();

	/*
	 * don't create the ar_set_list it will get the one from the
	 * global fs dir.
	 */

	if (g->ar_bufs == NULL || g->ar_max == NULL ||
	    g->ar_overflow_lst == NULL) {

		goto err;
	}


	if (get_all_available_media_type(NULL, &mt) != 0) {
		goto err;
	}


	for (n = mt->head; n != NULL; n = n->next) {
		char *media_type = (char *)n->data;

		/* setup default buffer list */
		buf = create_buffer_directive(media_type,
		    DEFAULT_AR_BUFSIZE, B_FALSE);

		if (buf == NULL) {
			lst_free_deep(mt);
			goto err;
		}
		if (lst_append(g->ar_bufs, buf) != 0) {
			free(buf);
			lst_free_deep(mt);
			goto err;
		}


		/* setup the default arch_max list */
		buf = create_buffer_directive(media_type, TPARCHMAX, B_FALSE);
		if (buf == NULL) {
			lst_free_deep(mt);
			goto err;
		}
		if (lst_append(g->ar_max, buf) != 0) {
			free(buf);
			lst_free_deep(mt);
			goto err;
		}

		/* setup the default overflow list */
		buf = create_buffer_directive(media_type, fsize_reset, B_FALSE);
		if (buf == NULL) {
			lst_free_deep(mt);
			goto err;
		}
		if (lst_append(g->ar_overflow_lst, buf) != 0) {
			free(buf);
			lst_free_deep(mt);
			goto err;
		}


	}

	/* free the list of media types */
	lst_free_deep(mt);

	g->ar_drives = create_default_ar_drives(libs, B_TRUE);
	if (g->ar_drives == NULL) {
		goto err;
	}

	g->ar_interval = SCAN_INTERVAL;
	g->archivemeta = B_TRUE;
	strlcpy(g->notify_script, DEFAULT_NOTIFY, sizeof (g->notify_script));
	Trace(TR_DEBUG, "initialized global directives");
	return (0);

err:
	lst_free_deep(g->ar_bufs);
	lst_free_deep(g->ar_max);
	lst_free_deep(g->ar_overflow_lst);
	lst_free_deep(g->ar_set_lst);
	lst_free_deep(g->timeouts);
	memset(g, 0, sizeof (ar_global_directive_t));

	Trace(TR_OPRMSG, "initializing global directives failed: %s",
	    samerrmsg);

	return (-1);
}


/*
 * create default drive directives list for global directives or
 * ar_fs_directives.
 */
sqm_lst_t *
create_default_ar_drives(
sqm_lst_t *libraries,	/* list of library_t from get_all_libraries */
boolean_t global)
{

	sqm_lst_t *l;
	node_t *n;
	library_t *lib;
	drive_directive_t *dd;

	Trace(TR_DEBUG, "creating default drives directives");

	/* create the return list */
	l = lst_create();
	if (l == NULL) {
		Trace(TR_OPRMSG,
		    "creating default drives directives failed: %s", samerrmsg);

		return (NULL);
	}

	if (libraries == NULL) {
		return (l);
	}

	/* for each library add a drive directive */
	for (n = libraries->head; n != NULL; n = n->next) {
		lib = (library_t *)n->data;
		if (strcmp(lib->base_info.equ_type, "hy") == 0) {
			continue;
		}
		dd = (drive_directive_t *)mallocer(sizeof (drive_directive_t));
		if (dd == NULL) {
			lst_free_deep(l);
			Trace(TR_OPRMSG, "%s failed: %s",
			    "creating default drives directives", samerrmsg);

			return (NULL);
		}

		memset(dd, 0, sizeof (drive_directive_t));
		strcpy(dd->auto_lib, lib->base_info.set);

		if (global) {
			dd->count = lib->no_of_drives;
		} else {
			dd->count = 1;
		}

		if (lst_append(l, dd) != 0) {
			lst_free_deep(l);
			free(dd);
			Trace(TR_OPRMSG, "%s failed: %s",
			    "creating default drives directives", samerrmsg);
			return (NULL);
		}
	}

	Trace(TR_DEBUG, "created default drives directives");
	return (l);
}


/*
 * create a buffer directive.
 * dosn't set any change flags.
 */
buffer_directive_t *
create_buffer_directive(
mtype_t mt,
fsize_t size,
boolean_t lock)
{

	buffer_directive_t *bd;

	Trace(TR_DEBUG, "creating buffer_directive");

	bd = (buffer_directive_t *)mallocer(sizeof (buffer_directive_t));

	if (bd == NULL) {
		Trace(TR_OPRMSG, "creating buffer_directive failed: %s",
		    samerrmsg);
		return (NULL);
	}

	memset(bd, 0, sizeof (buffer_directive_t));
	strlcpy(bd->media_type, mt, sizeof (devtype_t));
	bd->size = size;
	bd->lock = lock;

	Trace(TR_DEBUG, "created buffer_directive");
	return (bd);
}


/*
 * setup initial default values.
 */
int
init_ar_copy_params(
ar_set_copy_params_t *p)
{
	Trace(TR_DEBUG, "initializing ar_copy_params");

	memset(p, 0, sizeof (ar_set_copy_params_t));


	/*
	 * set all values to their defaults or to a reset value if they
	 * have no default.
	 */
	p->archmax = fsize_reset;	/* defaults to media value */
	p->bufsize = fsize_reset;	/* defaults to media value */
	p->drivemax = fsize_reset;	/* not set */
	p->drivemin = fsize_reset;	/* defaults to archmax */
	p->drives = 1;			/* could be handled by setfield */
	/* p->fillvsns is false */
	/* p->join is none */
	/* p->buflock is false */
	/* p->offline copy is none */
	p->ovflmin = fsize_reset;	/* defaults to media value */
	/* p->reserve = not set */
	/* p->sort is not set */
	p->startage = uint_reset;
	p->startsize = fsize_reset;
	p->startcount = int_reset;
	/* tapenonstop = false */
	/* unarchage = false for access */
	p->recycle.data_quantity = fsize_reset;
	/* p->recycle.ignore = false */
	p->recycle.hwm = int_reset;
	p->recycle.mingain = int_reset;
	p->recycle.vsncount = int_reset;
	p->fillvsns = B_FALSE;
	p->directio = B_TRUE;
	/* rearch_stage_copy is not set by default so leave it = 0 */
	p->queue_time_limit = 24*60*60;

	p->priority_lst = lst_create();
	if (p->priority_lst == NULL) {
		Trace(TR_OPRMSG, "initializing ar_copy_params failed: %s",
		    samerrmsg);

		return (-1);
	}

	Trace(TR_DEBUG, "initialized ar_copy_params");
	return (0);
}


/*
 * init an ar_fs_directive for each fs
 * if this function fails the caller is responsible for freeing the
 * archiver_cfg_t struct
 */
static int
init_fs_entries(
archiver_cfg_t *a,
sqm_lst_t *l)
{
	node_t			*node;
	ar_fs_directive_t	*fsdir;
	fs_t			*fst;
	uname_t			copy_name;

	Trace(TR_DEBUG, "initializing fs directives");

	for (node = l->head; node != NULL; node = node->next) {
		fst = (fs_t *)node->data;

		/*
		 * Include the fs directives for all file systems including
		 * those with nosam set. This allows an archiving configuration
		 * for a file system marked as nosam to be preserved.
		 */
		fsdir = (ar_fs_directive_t *)mallocer(
		    sizeof (ar_fs_directive_t));

		if (fsdir == NULL) {
			Trace(TR_OPRMSG,
			    "initializing fs directives failed: %s", samerrmsg);
			return (-1);
		}
		if (init_ar_fs_directive(fsdir) != 0) {
			free_ar_fs_directive(fsdir);
			Trace(TR_OPRMSG,
			    "initializing fs directives failed: %s", samerrmsg);
			return (-1);
		}

		if (!fst->mount_options->sam_opts.archive) {
			fsdir->options |= FS_NOSAM;
		}

		/* Make the default copy */
		fsdir->archivemeta = B_TRUE;
		snprintf(fsdir->fs_name, sizeof (fsdir->fs_name),
		    fst->fi_name);

		snprintf(copy_name, sizeof (copy_name), "%s.1",
		    fst->fi_name);

		if (lst_append(a->ar_fs_p, fsdir) != 0) {
			free_ar_fs_directive(fsdir);
			Trace(TR_OPRMSG,
			    "initializing fs directives failed: %s", samerrmsg);
			return (-1);
		}

		if (find_arch_cp_params(copy_name) != 0) {
			free_ar_fs_directive(fsdir);
			Trace(TR_OPRMSG,
			    "initializing fs directives failed: %s", samerrmsg);

			return (-1);
		}
	}

	Trace(TR_DEBUG, "initialized fs directives");
	return (0);
}



/*
 * initialize a new criteria.
 */
int
init_criteria(
ar_set_criteria_t *c,
char *criteria_name)
{

	Trace(TR_DEBUG, "initializing criteria");
	memset(c, 0, sizeof (ar_set_criteria_t));
	c->minsize = fsize_reset;
	c->maxsize = fsize_reset;
	c->access = int_reset;
	c->nftv = flag_reset;
	*(c->after) = '\0';


	if (strcmp(criteria_name, NO_ARCHIVE) != 0) {
		/* all non no_archive criteria gets one copy by default */
		c->num_copies = 1;
		if (get_default_copy_cfg(&(c->arch_copy[0])) != 0) {
			Trace(TR_OPRMSG, "initializing criteria failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	Trace(TR_DEBUG, "initialized criteria");
	return (0);
}

int
get_default_copy_cfg(
ar_set_copy_cfg_t **cp)
{

	Trace(TR_DEBUG, "getting default copy cfg");

	*cp = (ar_set_copy_cfg_t *)mallocer(sizeof (ar_set_copy_cfg_t));
	if (*cp == NULL) {
		Trace(TR_OPRMSG, "getting default copy cfg failed: %s",
		    samerrmsg);
		return (-1);
	}
	memset(*cp, 0, sizeof (ar_set_copy_cfg_t));
	(*cp)->ar_copy.copy_seq = 1;
	(*cp)->ar_copy.ar_age = ARCH_AGE;
	(*cp)->un_ar_age = uint_reset;

	Trace(TR_DEBUG, "getting default copy cfg");
	return (0);
}


/*
 * init an ar_fs_directive
 * if this function fails the caller is responsible for freeing the
 * fs.
 */
int
init_ar_fs_directive(
ar_fs_directive_t *fs)
{

	Trace(TR_DEBUG, "initializing ar_fs_directive");

	memset(fs, 0, sizeof (ar_fs_directive_t));
	fs->fs_interval = SCAN_INTERVAL;
	fs->scan_method = EM_noscan;
	fs->archivemeta = B_TRUE;
	/*
	 * scanlist squash and setarchdone default to off so leave the
	 * the options field zeros
	 */

	fs->ar_set_criteria = lst_create();
	if (fs->ar_set_criteria == NULL) {
		Trace(TR_OPRMSG, "initializing ar_fs_directive failed: %s",
		    samerrmsg);
		return (-1);
	}

	fs->num_copies = 1;
	if (get_default_copy_cfg(&(fs->fs_copy[0])) != 0) {
		Trace(TR_OPRMSG, "initializing ar_fs_directive failed: %s",
		    samerrmsg);
		return (-1);
	}

	Trace(TR_DEBUG, "initialized ar_fs_directive");
	return (0);
}


/*
 * reset all statics so parser can run repeatedly.
 */
static int
init_static_variables(void)
{

	Trace(TR_DEBUG, "initializing static variables");
	v_allsets = B_TRUE;
	v_allsets_copy = B_TRUE;
	p_allsets = B_TRUE;
	p_allsets_copy = B_TRUE;
	first_pass_other_copy = B_TRUE;
	first_pass_allsets_copy = B_TRUE;

	noGlobal = B_FALSE;	/* Don't allow a global directive */
	noDefine = B_FALSE;	/* Don't define an archive set */
	cmdFname = ARCHIVER_CMD;
	noEnd = NULL;
	mem_err = 0;
	no_cmd_file = B_FALSE;
	peekedToken = B_FALSE;
	memset(&tmp_arch_set, 0, sizeof (struct ArchSet));


	/* for use when unknown filesystems are encountered */
	if (err_fs != NULL) {
		free_ar_fs_directive(err_fs);
	}
	err_fs = mallocer(sizeof (ar_fs_directive_t));
	if (err_fs == NULL) {
		Trace(TR_OPRMSG, "initializing static variables failed: %s",
		    samerrmsg);
		return (-1);
	}
	init_ar_fs_directive(err_fs);

	if (err_crit != NULL) {
		Trace(TR_OPRMSG, "err_crit non-NULL");
		free_ar_set_criteria(err_crit);
	}

	err_crit = mallocer(sizeof (ar_set_criteria_t));
	if (err_crit == NULL) {
		Trace(TR_OPRMSG, "initializing static variables failed: %s",
		    samerrmsg);
		return (-1);
	}
	if (init_criteria(err_crit, "ERROR_CRIT") != 0) {
		Trace(TR_OPRMSG, "initializing static variables failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (error_list != NULL) {
		lst_free_deep(error_list);
	}
	error_list = lst_create();
	if (error_list == NULL) {
		Trace(TR_OPRMSG, "initializing static variables failed: %s",
		    samerrmsg);

		return (-1);
	}


	/*
	 * initialize the list for putting  fs properties and
	 * create the global entry.
	 */
	if (global_props != NULL) {
		free_ar_fs_directive(global_props);
	}

	global_props =
	    (ar_fs_directive_t *)mallocer(sizeof (ar_fs_directive_t));

	memset(global_props, 0, sizeof (ar_fs_directive_t));

	if (global_props == NULL) {
		Trace(TR_OPRMSG, "parsing archiver failed: %s", samerrmsg);
		return (-1);
	}

	/* Temporary Naming for data classes */
	criteria_index = 0;

	Trace(TR_DEBUG, "initialized static variables");
	return (0);
}


/*
 * Not a directive.
 */
static void
notDirective(void)
{

	if (noGlobal) {
		int	i;
		/*
		 * Check for global directive.
		 */
		for (i = 0; globalDirectives[i].DpName != NULL; i++) {
			if (strcmp(dirName, globalDirectives[i].DpName) == 0) {
				/* '%s' must precede any 'fs =' command */
				ReadCfgError(CustMsg(4439), dirName);
			}
		}
		notGlobalDirective();
	} else {
		ReadCfgLookupDirname(dirName, globalDirectives);
	}
}


/*
 * Not a global directive.
 * Must be an archive set definition, or a copy definition.
 */
static void
notGlobalDirective(void)
{

	if (isalpha(*dirName)) {
		if (ReadCfgGetToken() == 0 || strcmp(token, "=") == 0) {
			/* '%s' is not a valid archiver directive */
			ReadCfgError(CustMsg(4418), dirName);
		}
		/*
		 * An archive set definition.
		 */
		defineArchset(dirName);
	} else if (isdigit(*dirName)) {
		/*
		 * A copy definition.
		 */
		defineCopy();
	} else {
		/* '%s' is not a valid archiver directive */
		ReadCfgError(CustMsg(4418), dirName);
	}
}


/*
 * archmax = media value.
 */
static void
dirArchmax(void)
{

	uint64_t   size = 0;
	char	   tok1[TOKEN_SIZE];
	buffer_directive_t *buf = NULL;
	node_t *n;

	Trace(TR_DEBUG, "handling archmax");

	if (check_media_type(token) == -1) {
		ReadCfgError(CustMsg(4431));
	}

	strcpy(tok1, token);

	if (ReadCfgGetToken() == 0) {
		/* Missing '%s' value */
		ReadCfgError(CustMsg(14008), dirName);
	}
	asmSize(&size, dirName);

	/* check for a default value if found replace it with the new one. */
	for (n = arch_cfg->global_dirs.ar_max->head; n != NULL; n = n->next) {

		buf = (buffer_directive_t *)n->data;
		if (strcmp(tok1, buf->media_type) != 0) {
			buf = NULL;
		} else {
			break;
		}
	}

	if (buf == NULL) {

		buf = (buffer_directive_t *)mallocer(
		    sizeof (buffer_directive_t));

		if (buf == NULL) {
			ReadCfgError(samerrno);
		}
		memset(buf, 0, sizeof (buffer_directive_t));

		if (lst_append(arch_cfg->global_dirs.ar_max, buf) != 0) {
			free(buf);
			ReadCfgError(samerrno);
		}
	}

	strlcpy(buf->media_type, tok1, sizeof (buf->media_type));
	buf->lock = B_FALSE;
	buf->size = size;
	buf->change_flag |= BD_size;
}


static void
dirArchmeta(void)
{

	boolean_t value;
	node_t *n;

	Trace(TR_DEBUG, "handling archivemeta");
	if (strcmp(token, "on") == 0) {
		value = B_TRUE;
	} else if (strcmp(token, "off") == 0) {
		value = B_FALSE;
	} else {
		/* Invalid %s value ('%s') */
		ReadCfgError(CustMsg(4538), "archivemeta", token);
	}

	cur_fs->archivemeta = value;
	cur_fs->change_flag |= AR_FS_archivemeta;


	if (cur_fs == global_props) {
		for (n = arch_cfg->ar_fs_p->head; n != NULL; n = n->next) {
			((ar_fs_directive_t *)n->data)->archivemeta = value;
		}
	}
}


/*
 * bufsize = buffer size value.
 */
static void
dirBufsize(void)
{

	buffer_directive_t *buf;
	buffer_directive_t *tmp;
	char	*p;
	ulong_t	val;
	node_t	*n;


	Trace(TR_DEBUG, "handling bufsize");

	if (check_media_type(token) == -1) {
		ReadCfgError(CustMsg(4431));
	}




	buf = (buffer_directive_t *)mallocer(sizeof (buffer_directive_t));
	if (buf == NULL) {
		ReadCfgError(samerrno);
	}
	memset(buf, 0, sizeof (buffer_directive_t));
	strlcpy(buf->media_type, token, MAX_MEDIA_LEN);

	if (ReadCfgGetToken() == 0) {
		/* Missing '%s' value */
		free(buf);
		ReadCfgError(CustMsg(14008), dirName);
	}

	/*
	 * Set bufsize.
	 */
	errno = 0;
	val = strtoull(token, &p, 0);
	if (errno != 0 || *p != '\0') {
		free(buf);
		/* Invalid '%s' value '%s' */
		ReadCfgError(CustMsg(14101), dirName, token);
	}

	checkRange(dirName, val, 2, 1024);
	buf->size = (fsize_t)val;
	buf->change_flag |= BD_size;

	if (ReadCfgGetToken() != 0) {
		if (strcmp(token, "lock") == 0) {
			buf->lock = B_TRUE;
			buf->change_flag |= BD_lock;
		} else {
			free(buf);

			/* bufsize option must be 'lock' */
			ReadCfgError(CustMsg(4521));
		}
	}
	for (n = arch_cfg->global_dirs.ar_bufs->head; n != NULL; n = n->next) {

		tmp = (buffer_directive_t *)n->data;
		if (strcmp(tmp->media_type, buf->media_type) == 0) {

			free(tmp);

			if (lst_remove(arch_cfg->global_dirs.ar_bufs, n) != 0) {

				free(buf);
				ReadCfgError(samerrno);
			}
			break;
		}
	}

	Trace(TR_DEBUG, "bufsize %s size %llu lock %d flag 0x%04X",
	    buf->media_type, buf->size, buf->lock, buf->change_flag);

	if (lst_append(arch_cfg->global_dirs.ar_bufs, buf) != 0) {
		free(buf);
		ReadCfgError(samerrno);
	}
}


/*
 * drives = library_name count
 * library_name is the family set name of a library from the mcf
 * Family set name is a required field in the mcf for libraries. Any drive
 * that has a dash for family set is assumed to be manually loaded.
 */
static void
dirDrives(void)
{

	char		*p;
	int64_t		drives;
	uname_t		lib_name;
	library_t	*lib = NULL;
	node_t		*n;
	drive_directive_t *drive = NULL;

	Trace(TR_DEBUG, "handling drives");
	strlcpy(lib_name, token, sizeof (lib_name));

	if (ReadCfgGetToken() == 0) {
		/* Missing '%s' value */
		ReadCfgError(CustMsg(14008), dirName);
	}

	/*
	 * check the drive count.
	 * Must be less than count in the library. The number of drives
	 * in the library is the number of entries in the family set
	 * list for the library minus one(the robot entry)
	 */
	p = token;
	errno = 0;
	drives = strtoll(token, &p, 0);
	if (errno != 0 || drives < 0 || p == token) {
		ReadCfgError(CustMsg(4445));
	}

	/*
	 * find Library in the list of libraries and check the number of drives
	 */
	if (find_library_by_family_set(lib_list, lib_name, &lib) == -1) {
		ReadCfgError(CustMsg(4444), token);
	}

	if (drives > lib->no_of_drives) {
		ReadCfgError(CustMsg(4446), lib->no_of_drives);
	}

	/* find the default entry for the drive. */
	for (n = arch_cfg->global_dirs.ar_drives->head;
	    n != NULL; n = n->next) {

		if (strcmp(((drive_directive_t *)n->data)->auto_lib,
		    lib_name) == 0) {

			drive = (drive_directive_t *)n->data;
			break;
		}
	}

	/*
	 * if there was not a default here add the struct now.
	 * fill the fields after addition with non default values.
	 */
	if (drive == NULL) {
		drive = (drive_directive_t *)mallocer(
		    sizeof (drive_directive_t));

		if (drive == NULL) {
			ReadCfgError(samerrno);
		}
		if (lst_append(arch_cfg->global_dirs.ar_drives, drive) != 0) {
			free(drive);
			ReadCfgError(samerrno);
		}
	}

	strlcpy(drive->auto_lib, lib_name, sizeof (uname_t));
	drive->count = drives;
	drive->change_flag |= DD_count;


}


/*
 * examine = method
 */
static void
dirExamine(void)
{

	ExamMethod_t v;
	node_t *n;

	Trace(TR_DEBUG, "handling examine");


	for (v = EM_scan; strcmp(token, ExamMethodNames[v]) != 0; v++) {
		if (v == EM_max) {
			// Invalid % value ('%s')
			ReadCfgError(CustMsg(4538), "examine", token);
		}
	}

	cur_fs->scan_method = v;
	cur_fs->change_flag |= AR_FS_scan_method;

	/*
	 * if the property is being set on global then copy it to
	 * all of the fs directives
	 */
	if (cur_fs == global_props) {
		for (n = arch_cfg->ar_fs_p->head; n != NULL; n = n->next) {
			((ar_fs_directive_t *)n->data)->scan_method = v;
		}
	}
}


/*
 * fs = file_system
 *
 * ISSUES:
 * 1. is it an error for fs = fs1 to show up two times in an archiver.cmd
 * file? If so where is that checking done. readcmd did not do it
 * here.
 */
static void
dirFs(void)
{

	set_name_t	name;
	ar_fs_directive_t *fs;

	Trace(TR_DEBUG, "handling fs directive");


	/*
	 * Look up file system to make sure it exists.
	 */
	fs = find_fs(token);
	if (fs == NULL) {
		/*
		 * here is a wrinkle. To continue parsing without crashing
		 * you have to set cur_fs to a dummy variable. Create an error
		 * fs. That is used to parse into.
		 *
		 */
		if (err_fs == NULL) {

			fs = (ar_fs_directive_t *)
			    mallocer(sizeof (ar_fs_directive_t));

			memset(fs, 0, sizeof (ar_fs_directive_t));

			init_ar_fs_directive(fs);
			strlcpy(err_fs->fs_name, token,
			    sizeof (err_fs->fs_name));
		}

		cur_fs = err_fs;
		defining_metadata_copy = B_TRUE;
		/* Unknown file system */
		ReadCfgError(CustMsg(4449));
	}


	/*
	 * set as current file system.
	 */
	cur_fs = fs;

	strlcpy(asname, token, sizeof (asname)-1);

	defining_metadata_copy = B_TRUE;
	copy = 0;
	snprintf(name, sizeof (name), "%s.%d", token, copy + 1);
	find_arch_cp_params(name);  /* Set the archive set */
	noGlobal = B_TRUE;
}


/*
 * interval = time
 * intervals can be set on a file_system or global basis.
 *
 */
static void
dirInterval(void)
{

	uint_t	v;
	node_t *n;

	Trace(TR_DEBUG, "handling interval");


	assemble_age(&v, "interval", SCAN_INTERVAL_MAX);
	cur_fs->fs_interval = v;
	cur_fs->change_flag |= AR_FS_fs_interval;

	if (cur_fs == global_props) {
		for (n = arch_cfg->ar_fs_p->head; n != NULL; n = n->next) {
			((ar_fs_directive_t *)n->data)->fs_interval = v;
		}
	}
}


/*
 * logfile = file_name
 */
static void
dirLogfile(void)
{

	node_t *n;
	size_t	maxlen = (sizeof (upath_t)) - 1;

	Trace(TR_DEBUG, "handling logfile");

	if (!verify_file(token, B_TRUE)) {
		/* Cannot access %s file %s */
		ReadCfgError(CustMsg(4497), "log", token);
	}

	if (strlen(token) > maxlen) {
		/* Logfile name longer than %d characters */
		ReadCfgError(CustMsg(4536), maxlen);
	}

	/*
	 * Don't need to test to see if currently handling global properties
	 * simply set the log file in cur_fs.
	 */
	strlcpy(cur_fs->log_path, token, sizeof (cur_fs->log_path));
	cur_fs->change_flag |= AR_FS_log_path;

	if (cur_fs == global_props) {
		for (n = arch_cfg->ar_fs_p->head; n != NULL; n = n->next) {
			strcpy(((ar_fs_directive_t *)n->data)->log_path,
			    cur_fs->log_path);
		}
	}
}


/*
 * scanlist_squash = on | off
 */
static void
dirScanlist(void)
{

	node_t *n;
	int32_t flag;

	Trace(TR_DEBUG, "handling scanlist_squash");
	if (strcmp(token, "on") == 0) {
		flag = SCAN_SQUASH_ON;
	} else if (strcmp(token, "off") == 0) {
		flag = 0;
	} else {
		ReadCfgError(SE_INVALID_SCANLIST_VAL, token);
	}

	/*
	 * set the cur fs outside the loop even if it is the global dirs
	 * because inside the change flags do not get set.
	 */
	cur_fs->options &= ~SETARCHDONE_ON;
	cur_fs->options |= flag;
	cur_fs->change_flag |= AR_FS_scan_squash;

	if (cur_fs == global_props) {
		for (n = arch_cfg->ar_fs_p->head; n != NULL; n = n->next) {
			((ar_fs_directive_t *)n->data)->options |= flag;
		}
	}
}

/*
 * scanlist_squash = on | off
 */
static void
dirSetarchdone(void)
{

	node_t *n;
	int32_t flag;

	Trace(TR_DEBUG, "handling setarchdone");
	if (strcmp(token, "on") == 0) {
		flag = SETARCHDONE_ON;
	} else if (strcmp(token, "off") == 0) {
		flag = 0;
	} else {
		ReadCfgError(SE_INVALID_SCANLIST_VAL, token);
	}

	/*
	 * set the cur fs outside the loop even if it is the global dirs
	 * because inside the change flags do not get set.
	 */
	cur_fs->options &= ~SETARCHDONE_ON;
	cur_fs->options |= flag;
	cur_fs->change_flag |= AR_FS_setarchdone;

	if (cur_fs == global_props) {
		for (n = arch_cfg->ar_fs_p->head; n != NULL; n = n->next) {
			((ar_fs_directive_t *)n->data)->options |= flag;
		}
	}
}

/*
 * background_interval = interval
 */
static void
dirBackGndInterval(void)
{
	uint_t	v;
	node_t *n;

	assemble_age(&v, "background_interval", SCAN_INTERVAL_MAX);

	cur_fs->bg_interval = v;
	cur_fs->change_flag |= AR_FS_bg_interval;
	if (cur_fs == global_props) {
		for (n = arch_cfg->ar_fs_p->head; n != NULL; n = n->next) {
			((ar_fs_directive_t *)n->data)->bg_interval = v;
		}
	}
}


/*
 * background_time = hhmm
 */
static void
dirBackGndTime(void)
{
	int	error;
	int	i;
	int	tm[4];
	node_t *n;

	error = strlen(token) != 4;
	for (i = 0; i < 4; i++) {
		error += !isdigit(token[i]);
		tm[i] = token[i] - '0';
	}
	if (error != 0) {
		/* Invalid background time */
		ReadCfgError(CustMsg(4550));
	}
	tm[0] = 10 * tm[0] + tm[1];
	tm[2] = 10 * tm[2] + tm[3];
	if (tm[2] > 59 || tm[0] > 23) {
		/* Invalid background time */
		ReadCfgError(CustMsg(4550));
	}
	cur_fs->bg_time = (tm[0] * 100) + tm[2];
	cur_fs->change_flag |= AR_FS_bg_time;
	if (cur_fs == global_props) {
		for (n = arch_cfg->ar_fs_p->head; n != NULL; n = n->next) {
			((ar_fs_directive_t *)n->data)->bg_time =
			    cur_fs->bg_time;
		}
	}
}


/*
 * No beginning statement.
 * 'params', 'vsnpools', 'vsns'.
 */
static void
dirNoBegin(void)
{

	Trace(TR_OPRMSG, "handling missing beginning statements");
	/* No preceding '%s' statement */
	ReadCfgError(CustMsg(4447), dirName + 3);
}


/*
 * notify = file_name
 * this is the script that will be executed by the archiver when
 * predefined conditions are met.
 */
static void
dirNotify(void)
{

	Trace(TR_DEBUG, "handling notify");
	strlcpy(arch_cfg->global_dirs.notify_script, token,
	    sizeof (arch_cfg->global_dirs.notify_script));

	arch_cfg->global_dirs.change_flag |= AR_GL_notify_script;
}


/*
 * ovflmin = media value.
 */
static void
dirTimeout(void)
{

	char timeout_buf[32] = "";
	char *res;

	/* assemble a timeout kv string and insert it into the list */
	strlcat(timeout_buf, "timeout = ", 32);
	strlcat(timeout_buf, token, 32);
	strlcat(timeout_buf, " ", 32);
	if (ReadCfgGetToken() == 0) {
		/* Missing '%s' value */
		ReadCfgError(CustMsg(14008), dirName);
	}

	strlcat(timeout_buf, token, 32);

	res = strdup(timeout_buf);
	if (res == NULL) {
		ReadCfgError(SE_NO_MEM);
	}

	if (lst_append(arch_cfg->global_dirs.timeouts, res) != 0) {
		free(res);
		ReadCfgError(samerrno);
	}
}


/*
 * ovflmin = media value.
 */
static void
dirOvflmin(void)
{

	char	media_type[MAX_MEDIA_LEN];
	node_t	*n;
	uint64_t   size = 0;
	buffer_directive_t *buf = NULL;

	Trace(TR_DEBUG, "handling ovflmin");

	if (check_media_type(token) == -1) {
		ReadCfgError(CustMsg(4431));
	}

	strlcpy(media_type, token, MAX_MEDIA_LEN);

	if (ReadCfgGetToken() == 0) {
		/* Missing '%s' value */
		ReadCfgError(CustMsg(14008), dirName);
	}

	/* Check the size */
	asmSize(&size, "ovflmin");

	/* check for a default value if found replace it with the new one. */
	for (n = arch_cfg->global_dirs.ar_overflow_lst->head;
	    n != NULL; n = n->next) {

		buf = (buffer_directive_t *)n->data;
		if (strcmp(media_type, buf->media_type) != 0) {
			buf = NULL;
		} else {
			break;
		}
	}

	/* if the buffer was not found create one. */
	if (buf == NULL) {
		buf = (buffer_directive_t *)mallocer(
		    sizeof (buffer_directive_t));
		if (buf == NULL) {
			ReadCfgError(samerrno);
		}
		memset(buf, 0, sizeof (buffer_directive_t));
		strlcpy(buf->media_type, media_type, sizeof (buf->media_type));
		if (lst_append(arch_cfg->global_dirs.ar_overflow_lst,
		    buf) != 0) {

			free(buf);
			ReadCfgError(samerrno);
		}

	}

	/* now setup the directive */
	buf->lock = B_FALSE;
	buf->size = size;
	buf->change_flag |= BD_size;
}


/*
 * params
 */
static void
dirParams(void)
{

	static DirProc_t table[] = {
		{ "endparams", dirEndparams, DP_set },
		{ "vsnpools", dirVsnpools, DP_other },
		{ "vsns", dirVsns, DP_other },
		{ NULL, procParams, DP_other }
	};
	char	*msg;

	Trace(TR_DEBUG, "handling params");

	ReadCfgSetTable(table);
	noDefine = B_TRUE;
	msg = noEnd;
	noEnd = "endparams";
	if (msg != NULL) {
		/* '%s' missing */
		ReadCfgError(CustMsg(4462), msg);
	}
}


/*
 * end
 */
static void
dirEndparams(void)
{

	Trace(TR_DEBUG, "handling endparams");

	ReadCfgSetTable(directives);
	noEnd = NULL;
}


/*
 * Process params definitions.
 * The main body of this method is dedicated to making sure that any allsets
 * parameters are set before any real parameters. And to setting up the default
 * values in the archive sets as they are created.
 *
 * The variables p_allsets and p_allsets_copy are set to true each
 * time the parser is called in the init_static_variables functions.
 */
static void
procParams(void)
{

	/* Archive set parameters table */
	static DirProc_t table[] = {
	    { "-disk_archive", paramsDiskArchive, DP_param,   4522 },
#if !defined(DEBUG) | defined(lint)
	    /* CSTYLED */ { "-simdelay", paramsInvalid, DP_other },
	    /* CSTYLED */ { "-tstovfl", paramsInvalid, DP_other },
#endif /* !defined(DEBUG) | defined(lint) */
	    {"-fillvsns",	paramsFillvsns, DP_other },
	    { NULL,	paramsSetfield, DP_other }
	};


	boolean_t paramsDefined;

	Trace(TR_DEBUG, "handling copy params");


	/* IF IT IS AN ALLSETS */
	if (strcmp(dirName, ALL_SETS) == 0) {
		if (find_arch_cp_params(dirName) == -1) {
			/* Archive set %s not defined */
			ReadCfgError(CustMsg(4436), dirName);
		}
		if (!p_allsets || !p_allsets_copy) {
			/* '%s' parameters must be defined first */
			ReadCfgError(CustMsg(4505), ALL_SETS);
		}
	} else {

		/* Assemble set name. Sets cur_params */
		asmSetName();

		/*
		 * This if else and its internal ifs make sure that
		 * allsets if defined comes before any allsets.x defs
		 * and othersets.x comes after the last allsets.x def
		 */
		if (strncmp(dirName, ALL_SETS, strlen(ALL_SETS)) == 0) {

			/*
			 * allsets.copy paramaters are allowed as long
			 * as they come before anything else.
			 */
			if (!p_allsets_copy) {
				/*
				 * allsetcopy encountered after some non allset
				 * parameters
				 */

				/* '%s' parameters must be defined first */
				ReadCfgError(CustMsg(4505), ALL_SETS);
			}
			if (p_allsets) {
				/*
				 * allsets.copy encountered no more allsets
				 * params are allowed except for copies
				 */
				p_allsets = B_FALSE;
				setDefaultParams(1);
			}
		} else {
			if (p_allsets || p_allsets_copy) {
				p_allsets = B_FALSE;
				p_allsets_copy = B_FALSE;
				setDefaultParams(1);
				setDefaultParams(0);
			}
		}
	}
	paramsDefined = B_FALSE;
	peekedToken = B_FALSE;
	while (peekedToken || ReadCfgGetToken() != 0) {
		if (peekedToken && token[0] == '\0') {
			/* peeked the end of the line */
			break;
		}
		peekedToken = B_FALSE;
		strlcpy(dirName, token, sizeof (dirName)-1);
		ReadCfgLookupDirname(dirName, table);
		paramsDefined = B_TRUE;
	}


	if (!paramsDefined) {
		/* No archive set parameters were defined */
		ReadCfgError(CustMsg(4450));
	}

}


/*
 * vsnpools
 */
static void
dirVsnpools(void)
{

	static DirProc_t table[] = {
		{ "endvsnpools", dirEndvsnpools, DP_set	  },
		{ "params", dirParams, DP_other },
		{ "vsns", dirVsns, DP_other },
		{ NULL, procVsnpools, DP_other }
	};
	char	*msg;

	Trace(TR_DEBUG, "handling vsnpools");

	ReadCfgSetTable(table);
	noDefine = B_TRUE;
	msg = noEnd;
	noEnd = "endvsnpools";
	if (msg != NULL) {
		/* '%s' missing */
		ReadCfgError(CustMsg(4462), msg);
	}
}


/*
 * endvsnpools
 */
static void
dirEndvsnpools(void)
{

	Trace(TR_DEBUG, "handling endvsnpools");

	ReadCfgSetTable(directives);
	noEnd = NULL;
}


/*
 * Process vsnpools directives.
 */
static void
procVsnpools(void)
{

	vsn_pool_t *pool;
	char *str;

	Trace(TR_DEBUG, "handling vsnpools");

	if (strlen(dirName) > MAX_VSN_POOL_NAME_LEN) {
		/* VSN pool name is too long (max %d) */
		ReadCfgError(CustMsg(4488), MAX_VSN_POOL_NAME_LEN);
	}
	if (find_vsn_pool(dirName) != NULL) {
		/* Duplicate VSN pool name %s */
		ReadCfgError(CustMsg(4489), dirName);
	}

	pool = (vsn_pool_t *)mallocer(sizeof (vsn_pool_t));
	if (pool == NULL) {
		ReadCfgError(samerrno);
	}
	memset(pool, 0, sizeof (vsn_pool_t));

	pool->vsn_names = lst_create();
	if (pool->vsn_names == NULL) {
		free_vsn_pool(pool);
		ReadCfgError(samerrno);
	}

	strlcpy(pool->pool_name, token, sizeof (pool->pool_name) - 1);

	if (ReadCfgGetToken() == 0) {
		/* Media specification missing */
		free_vsn_pool(pool);
		ReadCfgError(CustMsg(4416));
	}

	if (check_media_type(token) == -1) {
		free_vsn_pool(pool);
		ReadCfgError(CustMsg(4431));
	}
	strlcpy(pool->media_type, token, sizeof (pool->media_type) - 1);

	/* Assemble at least one VSN. */
	while (ReadCfgGetToken() != 0) {
		if (check_vsn_exp(token) == -1) {
			free_vsn_pool(pool);
			/* Incorrect VSN expression */
			ReadCfgError(CustMsg(4442));
		}
		if (find_node_str(pool->vsn_names, token) == NULL) {
			str = (char *)mallocer(strlen(token) + 1);
			if (str == NULL) {
				free_vsn_pool(pool);
				ReadCfgError(samerrno);
			}
			strcpy(str, token);

			if (lst_append(pool->vsn_names, str) != 0) {
				free(str);
				free_vsn_pool(pool);
				ReadCfgError(samerrno);
			}
		}
	}
	if (pool->vsn_names->length == 0) {
		/* VSN specification missing */
		free_vsn_pool(pool);
		ReadCfgError(CustMsg(4453));
	}

	if (lst_append(arch_cfg->vsn_pools, pool) != 0) {
		free_vsn_pool(pool);
		ReadCfgError(samerrno);
	}
}


/*
 * vsns
 */
static void
dirVsns(void)
{

	static DirProc_t table[] = {
		{ "endvsns", dirEndvsns, DP_set	  },
		{ "params", dirParams, DP_other },
		{ "vsnpools", dirVsns, DP_other },
		{ NULL, procVsns, DP_other }
	};
	char	*msg;

	Trace(TR_DEBUG, "handling vsns");

	ReadCfgSetTable(table);
	noDefine = B_TRUE;
	msg = noEnd;
	noEnd = "endvsns";
	if (msg != NULL) {
		/* '%s' missing */
		ReadCfgError(CustMsg(4462), msg);
	}
}


/*
 * endvsns
 */
static void
dirEndvsns(void)
{

	Trace(TR_DEBUG, "handling endvsns");
	ReadCfgSetTable(directives);
	noEnd = NULL;
}


/*
 * Process vsns.
 *
 * The variables v_allsets and v_allsets_copy are set to true each
 * time the parser is called in the init_static_variables functions.
 */
static void
procVsns(void)
{

	mtype_t	mtype;
	boolean_t made_map = B_FALSE;
	vsn_map_t *cur_vsns;
	char *str;

	Trace(TR_DEBUG, "handling vsns");
	if (strcmp(dirName, ALL_SETS) == 0) {
		if (find_arch_cp_params(dirName) != 0) {
			/* Archive set %s not defined */
			ReadCfgError(CustMsg(4436), dirName);
		}
		if (!v_allsets || !v_allsets_copy) {
			/* '%s' parameters must be defined first */
			ReadCfgError(CustMsg(4505), ALL_SETS);
		}
	} else {
		/*
		 * Assemble set name. sets cur_params and asname
		 * useful for disk archiving and vsn associations respectively.
		 */
		asmSetName();

		/*
		 * This if else and its internal ifs make sure that
		 * allsets, if defined, comes before any allsets.x defs
		 * and othersets.x come after the last allsets.x def
		 */
		if (strncmp(dirName, ALL_SETS, strlen(ALL_SETS)) == 0) {
			/*
			 * allsets.copy
			 */
			if (!v_allsets_copy) {
				/* '%s' parameters must be defined first */
				ReadCfgError(CustMsg(4505), ALL_SETS);
			}
			if (v_allsets) {
				v_allsets = B_FALSE;
			}
		} else {
			if (v_allsets || v_allsets_copy) {
				v_allsets = B_FALSE;
				v_allsets_copy = B_FALSE;
			}
		}
	}

	if (ReadCfgGetToken() == 0) {
		/* Media specification missing */
		ReadCfgError(CustMsg(4416));
	}
	(void) check_media_type(token);	/* Validate media */
	strlcpy(mtype, token, sizeof (mtype));



	/* see if there is a map for this one already. */
	cur_vsns = find_vsn_map(cur_params->ar_set_copy_name);
	if (cur_vsns != NULL) {

		/* Previous VSNs directive for this archive set */
		if (strcmp(cur_vsns->media_type, mtype) != 0) {
			/* Different media previously specified */
			Trace(TR_OPRMSG, "vsn_map existed");
			ReadCfgError(CustMsg(4493));
		}

	} else { // (cur_vsns == NULL) {
		cur_vsns = (vsn_map_t *)mallocer(sizeof (vsn_map_t));
		if (cur_vsns == NULL) {
			ReadCfgError(samerrno);
		}
		memset(cur_vsns, 0, sizeof (vsn_map_t));

		cur_vsns->vsn_names = lst_create();
		cur_vsns->vsn_pool_names = lst_create();

		if (cur_vsns->vsn_names == NULL ||
		    cur_vsns->vsn_pool_names == NULL) {

			free_vsn_map(cur_vsns);
			ReadCfgError(samerrno);
		}

		strlcpy(cur_vsns->media_type, mtype,
		    sizeof (cur_vsns->media_type));

		strlcpy(cur_vsns->ar_set_copy_name,
		    cur_params->ar_set_copy_name,
		    sizeof (cur_vsns->ar_set_copy_name));

		made_map = B_TRUE;
	}

	/* Assemble at least one VSN. */

	while (ReadCfgGetToken() != 0) {
		if (strcmp(token, "-pool") == 0) {
			vsn_pool_t *pool;

			if (ReadCfgGetToken() == 0) {
				/* VSN pool name missing */
				if (made_map) {
					free_vsn_map(cur_vsns);
				}
				ReadCfgError(CustMsg(4490));
			}

			if ((pool = find_vsn_pool(token)) == NULL) {

				if (made_map) {
					free_vsn_map(cur_vsns);
				}
				/* VSN pool %s not defined */
				ReadCfgError(CustMsg(4491), token);
			}

			if (strcmp(pool->media_type, mtype) != 0) {
				if (made_map) {
					free_vsn_map(cur_vsns);
				}
				/* Pool media does not match archive set */
				ReadCfgError(CustMsg(4492));
			}

			/*
			 * The pool existed in the global list so insert its
			 * name for this map if its not already here.
			 */
			if (find_node_str(cur_vsns->vsn_pool_names,
			    token) == NULL) {

				str = (char *)mallocer(strlen(token) + 1);
				if (str == NULL) {
					if (made_map) {
						free_vsn_map(cur_vsns);
					}
					ReadCfgError(samerrno);
				}
				strcpy(str, token);
				if (lst_append(cur_vsns->vsn_pool_names,
				    str) != 0) {

					if (made_map) {
						free_vsn_map(cur_vsns);
					}

					ReadCfgError(samerrno);
				}
			}

		} else {
			if (find_node_str(cur_vsns->vsn_names,
			    token) == NULL) {

				if (check_vsn_exp(token) == -1) {
					if (made_map) {
						free_vsn_map(cur_vsns);
					}

					/* Incorrect VSN expression */
					ReadCfgError(CustMsg(4442));
				}

				str = (char *)mallocer(strlen(token) + 1);
				if (str == NULL) {
					if (made_map) {
						free_vsn_map(cur_vsns);
					}
					ReadCfgError(samerrno);
				}
				strcpy(str, token);
				if (lst_append(cur_vsns->vsn_names,
				    str) != 0) {

					if (made_map) {
						free_vsn_map(cur_vsns);
					}
					ReadCfgError(samerrno);
				}
			}
		}
	}

	if (cur_vsns->vsn_names->length == 0 &&
	    cur_vsns->vsn_pool_names->length == 0) {

		if (made_map) {
			free_vsn_map(cur_vsns);
		}

		/* VSN specification missing */
		ReadCfgError(CustMsg(4453));

	} else if (made_map) {
		/*
		 * only insert the map if it was created during
		 * the current execution of this function
		 */
		if (lst_append(arch_cfg->vsn_maps, cur_vsns) != 0) {
			free_vsn_map(cur_vsns);
			ReadCfgError(samerrno);
		}
	}

}


/*
 * wait
 */
static void
dirWait(void)
{

	node_t *n;
	Trace(TR_DEBUG, "handling wait directive");
	/*
	 * don't check for first read here.  Just record that
	 * wait is in the file.	 The FirstRead check in readcmd prevents
	 * cli execution from restarting the wait.
	 */
	cur_fs->wait = B_TRUE;
	cur_fs->change_flag |= AR_FS_wait;

	if (cur_fs == global_props) {
		for (n = arch_cfg->ar_fs_p->head; n != NULL; n = n->next) {
			((ar_fs_directive_t *)n->data)->wait = cur_fs->wait;
		}
	}

}


/*
 * Copy option -norelease.
 */
static void
copyNorelease(void)
{

	Trace(TR_DEBUG, "handling copy norelease directive");
	cur_copy->norelease = B_TRUE;
	cur_copy->change_flag |= AR_CP_norelease;
}


/*
 * Copy option -release.
 */
static void
copyRelease(void)
{

	Trace(TR_DEBUG, "handling copy release directive");

	cur_copy->release = B_TRUE;
	cur_copy->change_flag |= AR_CP_release;
}


/*
 * Define an archive set.
 * archsetname path [search_criteria1 ...] [file attributes]
 *
 * The body of this function handles archsetname and path.
 * table[] is passed to ReadCfgLookupDirname to handle the remainder of
 * the line.
 *
 * This method is called with name = null to set up FilePropsEntries
 * (ar_set_criteria) when initializing the filesysProps.  We don't need to do
 * that so it should not happen.
 *
 */
static void
defineArchset(char *name)
{

	char	*p;
	ar_set_criteria_t *crit;
	node_t	*node;

	/* File properties table */
	static DirProc_t table[] = {
		{ "-release",	propRelease,	DP_param,	4475 },
		{ "-stage",	propStage,	DP_param,	4476 },
		{ NULL,		propSetfield,	DP_other }
	};
	Trace(TR_DEBUG, "defining Archset(%s)", Str(name));

	defining_metadata_copy = B_FALSE;

	if (name != NULL) {
		/*
		 * Validate archive set name.
		 */
		int	n;

		/* sizeof (set_name_t) from sam/archset.h leave room for .x */
		n = sizeof (set_name_t) - 3;
		if (strlen(name) > n) {
			/* Archive set name is too long (max %d) */
			cur_crit = err_crit;
			ReadCfgError(CustMsg(4454), n);
		}
		p = name;
		while (isalnum(*p) || *p == '_') {
			p++;
		}
		if (*p != '\0') {
			/* Invalid archive set name */
			cur_crit = err_crit;
			ReadCfgError(CustMsg(4432));
		}

		/* Name is valid */
		strlcpy(asname, name, sizeof (asname)-1);

	} else {
		strlcpy(asname, (char *)cur_fs->fs_name, sizeof (asname)-1);
	}

	if (strcmp(asname, NO_ARCHIVE) != 0) {
		set_name_t sname;
		snprintf(sname, sizeof (sname), "%s.1", asname);

		samerrno = 0;
		if (find_arch_cp_params(sname) != 0) {
			if (samerrno != 0) {
				cur_crit = err_crit;
				ReadCfgError(samerrno);
			} else {
				cur_crit = err_crit;
				/* Archive set %s not defined */
				ReadCfgError(CustMsg(4436), dirName);
			}
		}
	}

	/*
	 * if there is any problem creating or inserting the criteria into
	 * the cfg struct free it and insert the err_crit so that parsing
	 * can continue and memory won't leak.
	 * Once the crit has been initialized and inserted into the cfg if
	 * errors are encountered it is not necessary to free it as it will
	 * be freed prior to parse_archivers return thus the actual struct
	 * can be used for further parsing.
	 */
	crit = (ar_set_criteria_t *)mallocer(sizeof (ar_set_criteria_t));
	if (crit == NULL) {
		cur_crit = err_crit;
		ReadCfgError(samerrno);
	}

	if (init_criteria(crit, asname) != 0) {
		cur_crit = err_crit;
		free_ar_set_criteria(crit);
		ReadCfgError(samerrno);
	}

	cur_crit = crit;

	strlcpy(cur_crit->set_name, asname, sizeof (cur_crit->set_name));
	strlcpy((char *)cur_crit->fs_name, (char *)cur_fs->fs_name,
	    sizeof (cur_crit->fs_name));


	/*
	 * Assemble path.
	 */
	if (*token == '\0') {
		cur_crit = err_crit;
		free_ar_set_criteria(crit);

		/* Path missing */
		ReadCfgError(CustMsg(4455));
	}
	if (*token == '/') {
		cur_crit = err_crit;
		free_ar_set_criteria(crit);

		/* Path must be relative */
		ReadCfgError(CustMsg(4456));
	}

	/*
	 * Check the path but preserve the unnormalized path
	 */
	strlcpy(crit->path, token, sizeof (crit->path));
	if (normalizePath(token) == NULL) {
		cur_crit = err_crit;
		free_ar_set_criteria(crit);

		/* Invalid path */

		ReadCfgError(CustMsg(4457));
	}
	crit->change_flag |= AR_ST_path;

	/* handle the rest of the line */
	while (ReadCfgGetToken() != 0) {
		strlcpy(dirName, token, sizeof (dirName)-1);
		ReadCfgLookupDirname(dirName, table);
	}

	/*
	 * Assure that there is no duplicate definition.
	 * Skip the metadata entry.
	 */
	for (node = cur_fs->ar_set_criteria->head;
	    node != NULL; node = node->next) {

		if (criteriaMatches(cur_crit,
		    (ar_set_criteria_t *)node->data)) {
			uname_t crit_name;
			strcpy(crit_name, crit->set_name);

			cur_crit = err_crit;
			free_ar_set_criteria(crit);

			/*
			 * It is not permitted to have duplicate file match
			 * criteria for a file system.
			 */
			ReadCfgError(CustMsg(SE_DUPLICATE_FILE_PROPS));
		}
	}

	if (lst_append(cur_fs->ar_set_criteria, crit) != 0) {
		cur_crit = err_crit;
		free_ar_set_criteria(crit);
		ReadCfgError(samerrno);
	}
}



/*
 * Define copy for the current archive set.
 */
static void
defineCopy(void)
{

	/* Copy Options Table */
	static DirProc_t table[] = {
		{ "-norelease",	copyNorelease,	DP_other },
		{ "-release",	copyRelease,	DP_other },
		{ NULL,		NULL,		CustMsg(4474) }
	};
	set_name_t name;
	uint_t tmp;

	Trace(TR_DEBUG, "defining copy %s", Str(dirName));

	/*
	 * Check for file properties being defined.
	 * Not a "NoArchive" archive set.
	 * Number in range 1 - MAX_ARCHIVE.
	 * Copy not previously defined.
	 */
	if (cur_fs == NULL && !defining_metadata_copy) {
		/*  No file properties defined */
		ReadCfgError(CustMsg(4427));
	}

	if (strcmp(asname, NO_ARCHIVE) == 0) {
		/* Copy not allowed for '%s' */
		ReadCfgError(CustMsg(4428), NO_ARCHIVE);
	}

	if (strlen(dirName) > 1) {
		/* Invalid archive copy number */
		ReadCfgError(CustMsg(4434));
	}
	copy = *dirName - '1';
	if (copy < 0 || copy >= MAX_ARCHIVE) {
		/* Archive copy number must be 1 <= n <= %d */
		ReadCfgError(CustMsg(4435), MAX_ARCHIVE);
	}

	if (defining_metadata_copy) {
		/* if there was a default copy, free it and its copy params */
		if (cur_fs->fs_copy[0] != NULL &&
		    !cur_fs->fs_copy[0]->change_flag) {

			uname_t fs_set_nm;

			free((cur_fs->fs_copy[0]));
			cur_fs->fs_copy[0] = NULL;
			cur_fs->num_copies--; /* will get incremented below */


			snprintf(fs_set_nm, sizeof (uname_t),
			    "%s.1", cur_fs->fs_name);

			/* ignore errors */
			cfg_reset_ar_set_copy_params(arch_cfg,
			    fs_set_nm);
		}

		if (cur_fs->fs_copy[copy] != NULL) {
			/* Duplicate archive copy number */
			ReadCfgError(CustMsg(4429));
		}

		/* create the copy and update num_copies. */
		if (get_default_copy_cfg(&cur_copy) != 0) {
			ReadCfgError(samerrno);
		}

		cur_fs->fs_copy[copy] = cur_copy;
		cur_fs->num_copies++;
		cur_copy->ar_copy.copy_seq = copy + 1;
	} else {

		if (cur_crit == NULL) {
			cur_crit = err_crit;
		}

		/* if there was a default copy free it */
		if (cur_crit->arch_copy[0] != NULL &&
		    !cur_crit->arch_copy[0]->change_flag) {
			uname_t set_nm;

			free((cur_crit->arch_copy[0]));
			cur_crit->arch_copy[0] = NULL;
			cur_crit->num_copies--; /* gets incremented below */


			snprintf(set_nm, sizeof (uname_t),
			    "%s.1", cur_crit->set_name);

			/* ignore errors */
			cfg_reset_ar_set_copy_params(arch_cfg,
			    set_nm);

		}


		if (cur_crit->arch_copy[copy] != NULL) {
			/* Duplicate archive copy number */
			ReadCfgError(CustMsg(4429));
		}

		/* NOW MUST CREATE arch_copy[copy] */
		if (get_default_copy_cfg(&cur_copy) != 0) {
			ReadCfgError(samerrno);
		}
		cur_crit->arch_copy[copy] = cur_copy;
		cur_crit->num_copies++;
		cur_crit->arch_copy[copy]->ar_copy.copy_seq = copy + 1;
	}



	/*
	 * Find or create the copy params.
	 */
	snprintf(name, sizeof (name), "%s.%d", asname, copy + 1);
	(void) find_arch_cp_params(name);
	if (ReadCfgGetToken() == 0) {
		return;
	}

	/*
	 * Process copy options. -release and -norelease
	 */
	while (*token == '-') {
		strlcpy(dirName, token, sizeof (dirName)-1);
		ReadCfgLookupDirname(dirName, table);
		if (ReadCfgGetToken() == 0) {
			return;
		}
	}

	assemble_age(&tmp, "archive age", INT_MAX);
	cur_copy->ar_copy.ar_age = tmp;
	cur_copy->change_flag |= AR_CP_ar_age;
	if (ReadCfgGetToken() == 0) {
		return;
	}

	assemble_age(&tmp, "unarchive age",  INT_MAX);
	cur_copy->un_ar_age = tmp;
	cur_copy->change_flag |= AR_CP_un_ar_age;
	if (ReadCfgGetToken() == 0) {
		return;
	}
	/* Too many age parameters */
	ReadCfgError(CustMsg(4438));
}


/*
 * Process 'group' for SetFieldValue().
 */
static int
propGroupSet(
void *v,
char *value,
char *buf,
int bufsize)
{

	char *name = (char *)v;

	Trace(TR_DEBUG, "handling group directive %s", Str(value));
	if (getgrnam(value) == NULL) {
		/* Group name not in group file */
		snprintf(buf, bufsize, GetCustMsg(4464));
		errno = EINVAL;
		return (-1);
	}

	if (strlen(value) > (sizeof (cur_crit->group) - 1)) {
		snprintf(buf, bufsize, "%s exceeded maximum length of %d",
		    "-group", sizeof (cur_crit->group));

		return (-1);
	}
	strlcpy(name, value, sizeof (cur_crit->group));

	return (0);
}


/*
 * Process 'name' for SetFieldValue().
 * this method implements an interface but doesn't use all of the args.
 */
/* ARGSUSED */
static int
propNameSet(
void *v,
char *value,
char *buf,
int bufsize)
{

	char	*regExp;
	size_t	l;
	char *name = (char *)v;

	Trace(TR_DEBUG, "handling name directive %s", Str(value));

	if ((regExp = regcmp(value, NULL)) == NULL) {
		/* Incorrect file name expression */
		snprintf(buf, bufsize, GetCustMsg(4460));
		errno = EINVAL;
		return (-1);
	}
	free(regExp);
	/*
	 * Save the token string for user list output.
	 */
	l = strlen(value) + 1;
	if (l > sizeof (cur_crit->name)) {
		/* Name longer than %d characters */
		snprintf(buf, bufsize, GetCustMsg(4531),
		    sizeof (cur_crit->name) - 1);
		errno = EINVAL;
		return (-1);
	}


	/*
	 * Name simply be populated here and regexp type set to
	 * REGEXP. If the data class specifies a different regexp type
	 * the name will be modified to match that type when adding
	 * class name to the criteria. This is done to make sure we
	 * get the regexp type set back to how the user
	 * specified it.
	 */
	strlcpy(name, value, sizeof (cur_crit->name));
	return (0);
}


/*
 * Set release attributes for a file.
 */
static void
propRelease(void)
{

	char	*p;
	char	*end_ptr;

	Trace(TR_DEBUG, "handling release directive");

	if (strcmp(asname, NO_ARCHIVE) == 0) {
		free_ar_set_criteria(cur_crit);
		cur_crit = err_crit;

		/* '%s' not allowed for '%s' */
		ReadCfgError(CustMsg(4440), "-release", NO_ARCHIVE);
	}


	p = token;
	while (*p != 0) {
		switch (*p) {
		case SET_DEFAULT_RELEASE:
			cur_crit->attr_flags |= ATTR_RELEASE_DEFAULT;
			break;
		case ALWAYS_RELEASE:
			cur_crit->attr_flags |= ATTR_RELEASE_ALWAYS;
			break;
		case NEVER_RELEASE:
			cur_crit->attr_flags |= ATTR_RELEASE_NEVER;
			break;
		case PARTIAL_RELEASE:
			cur_crit->attr_flags |= ATTR_RELEASE_PARTIAL;
			break;
		case PARTIAL_SIZE:
			cur_crit->attr_flags |= ATTR_PARTIAL_SIZE;
			errno = 0;
			cur_crit->partial_size =
			    (int32_t)strtol((p + 1), &end_ptr, 10);

			if (errno != 0 || ((p+1) == end_ptr)) {
				ReadCfgError(CustMsg(4477), *p);
			}

			/*
			 * p gets incremented after the switch.
			 * So decrement end_ptr here to point to the last
			 * character converted by strtol.
			 */
			p = end_ptr - 1;

			checkRange("partial_size", cur_crit->partial_size,
			    SAM_MINPARTIAL, SAM_MAXPARTIAL);
			break;

		default: {
				free_ar_set_criteria(cur_crit);
				cur_crit = err_crit;
				/* Invalid release attribute %c */
				ReadCfgError(CustMsg(4477), *p);
				break;
			}
		}
		p++;

	} /* End while (*p != 0) */
}


/*
 * Set field for file properties.
 *
 * This method must be called as DP_other.  If not it will fail.
 *
 * Three of the directives that are handled by this method are processed by
 * setfield.c with fieldFunc they are group, name, and user.  The
 * setting of the appropriate fields are handled in
 * propGroupSet, propNameSet and propUserSet in this file.
 *
 * if archiver/include/fileprops.hc changes this method will need to be
 * reviewed.
 */
static void
propSetfield(void)
{
	int keyoffset = 1;

	Trace(TR_DEBUG, "handling properties setfield for %s", Str(dirName));

	if (parsing_class_file) {
		keyoffset = 1;
	}
	if ((!parsing_class_file && dirName[0] != '-') || dirName[1] == '\0') {
		free_ar_set_criteria(cur_crit);
		cur_crit = err_crit;

		/* '%s' is not a valid file property */
		ReadCfgError(CustMsg(4426), dirName);
	}

	/*
	 * Try an empty value.
	 */
	*token = '\0';
	errno = 0;
	if (SetFieldValue(cur_crit, criteria_tbl, dirName+keyoffset, token,
	    msgFuncSetField) != 0) {

		if (errno == ENOENT) {

			free_ar_set_criteria(cur_crit);
			cur_crit = err_crit;

			/* '%s' is not a valid file property */
			ReadCfgError(CustMsg(4426), dirName);
		}
		if (errno != EINVAL) {
			free_ar_set_criteria(cur_crit);
			cur_crit = err_crit;

			ReadCfgError(0, errMsg);
		}

		/*
		 * Parameter requires a value.
		 */
		ReadCfgGetToken();
		if (SetFieldValue(cur_crit, criteria_tbl, dirName+keyoffset,
		    token, msgFuncSetField) != 0) {

			free_ar_set_criteria(cur_crit);
			cur_crit = err_crit;

			ReadCfgError(0, errMsg);
		}
	}
}


/*
 * Set stage attributes for a file.
 */
static void
propStage(void)
{

	char	*p;

	Trace(TR_DEBUG, "handling stage directive");
	if (strcmp(asname, NO_ARCHIVE) == 0) {
		free_ar_set_criteria(cur_crit);
		cur_crit = err_crit;

		/* '%s' not allowed for '%s' */
		ReadCfgError(CustMsg(4440), "-stage", NO_ARCHIVE);
	}
	p = token;
	while (*p != 0) {
		switch (*p) {
		case SET_DEFAULT_STAGE:
			cur_crit->attr_flags |= ATTR_STAGE_DEFAULT;
			break;
		case ASSOCIATIVE_STAGE:
			cur_crit->attr_flags |= ATTR_STAGE_ASSOCIATIVE;
			break;
		case NEVER_STAGE:
			cur_crit->attr_flags |= ATTR_STAGE_NEVER;
			break;
		default: {
				free_ar_set_criteria(cur_crit);
				cur_crit = err_crit;

				/* Invalid stage attribute %c */
				ReadCfgError(CustMsg(4478), *p);
				break;
			}
		}
		p++;
	} /* End while (*p != 0) */
}


/*
 * Process 'user' for SetFieldValue().
 */
static int
propUserSet(
void *v,
char *value,
char *buf,
int bufsize)
{

	char *user = (char *)v;

	Trace(TR_DEBUG, "handling user directive %s", Str(value));
	if (getpwnam(value) == NULL) {
		/* User name not in password file */
		snprintf(buf, bufsize, GetCustMsg(4465));
		errno = EINVAL;
		return (-1);
	}

	if (strlen(value) > (sizeof (cur_crit->user) - 1)) {

		snprintf(buf, bufsize, "%s exceeded maximum length of %d",
		    "-user", sizeof (cur_crit->user));

		return (-1);
	}

	strlcpy(user, value, sizeof (cur_crit->user));
	return (0);
}


/*
 * find the ar_fs_directive for the named fs. If none exists return null.
 */
static ar_fs_directive_t *
find_fs(
char *name)
{

	node_t *node;
	ar_fs_directive_t *fs = NULL;

	for (node = arch_cfg->ar_fs_p->head; node != NULL; node = node->next) {
		fs = (ar_fs_directive_t *)node->data;

		if (strcmp(name, fs->fs_name) == 0) {
			return (fs);
		}
		fs = NULL;
	}
	return (fs);
}




/*
 * Assemble age.
 */
static void
assemble_age(
uint_t *age,
char *name,
int max)
{

	int	interval;

	Trace(TR_DEBUG, "assembling age");

	if (StrToInterval(token, &interval) == -1) {
		/* Invalid '%s' value ('%s') */
		ReadCfgError(CustMsg(14102), name, token);
	}

	checkRange(name, interval, 0, max);
	*age = (uint32_t)interval;
}


/*
 * Assemble set name.
 * Checks the params name and sets the cur_params to the params named
 * dirName.
 */
static int
asmSetName(void)
{

	char	*p;
	int	asn;

	Trace(TR_DEBUG, "assembling set name %s", dirName);
	/*
	 * Set name must begin with letter.
	 */
	if (!isalpha(*dirName) || strlen(dirName) > (sizeof (set_name_t)-1)) {
		/* Invalid archive set name */
		ReadCfgError(CustMsg(4432));
	}

	/*
	 * Set copy.
	 */
	if ((p = strchr(dirName, '.')) == NULL) {
		/* Archive copy number missing */
		ReadCfgError(CustMsg(4433));
	}
	p++;
	if (!isdigit(*p)) {
		/* Invalid archive copy number */
		ReadCfgError(CustMsg(4434));
	}
	copy = *p++ - '1';
	if (copy < 0 || copy >= MAX_ARCHIVE) {
		/* Archive copy number must be 1 <= n <= %d */
		ReadCfgError(CustMsg(4435), MAX_ARCHIVE);
	}

	if (!(*p == '\0' || (*p == 'R' && *(p+1) == '\0'))) {
		/* Invalid archive set name */
		ReadCfgError(CustMsg(4432));
	}
	if (*p == '\0' || *p == 'R') {
		asn = find_arch_cp_params(dirName);
		if (asn == -1) {
			/* Archive set %s not defined */
			ReadCfgError(CustMsg(4436), dirName);
		}
	}


	*(p - 2) = '\0';
	strlcpy(asname, dirName, sizeof (asname)-1);
	*(p - 2) = '.';
	return (asn);
}


/*
 * takes in a string that represents a file size
 * e.g. 10G and converts it to fsize_t
 * Size string is in token.
 */
static void
asmSize(
fsize_t *size,	/* return val */
char *name)	/* for error string */
{

	uint64_t value;

	Trace(TR_DEBUG, "assembling size");
	if (StrToFsize(token, &value) == -1) {
		/* Invalid '%s' value ('%s') */
		ReadCfgError(CustMsg(14101), name, token);
	}
	*size = value;
}

/*
 * check the VSN expression.
 * token = VSN definition string.
 */
static int
check_vsn_exp(
char *string)
{

	char	*RegExp;

	Trace(TR_DEBUG, "checking vsn expression");
	RegExp = regcmp(string, NULL);
	if (RegExp == NULL) {
		return (-1);
	}
	free(RegExp);
	return (0);
}


/*
 * Check range of a value. No return if error.
 */
static void
checkRange(char *name,
int64_t value,
int64_t min,
int64_t max)
{

	if (value < min && value > max) {
		ReadCfgError(CustMsg(14102), name, min, max);
	}
	if (value < min) {
		ReadCfgError(CustMsg(14103), name, min);
	}
	if (value > max) {
		ReadCfgError(CustMsg(14104), name, max);
	}
}



/*
 * Finds the named arch_copy_params_t and sets it to cur_params
 * If the set is not found and noDefine == FALSE
 *	the arch_copy_params will be created.
 * If the arch_copy_param are not found and noDefine is TRUE
 *	-1 is returned.
 *
 *
 * returns 0 if the set was found and or could be created.
 * returns -1 if an error occurs or the set is not found and can not be defined
 *
 */
static int
find_arch_cp_params(
char *name)
{

	size_t size;
	node_t *node;
	ar_set_copy_params_t *p = NULL;

	Trace(TR_DEBUG, "finding ar_set_cp_params %s", name);

	samerrno = 0;
	/* loop over ar_set_copy_params named set params found */
	for (node = arch_cfg->archcopy_params->head;
	    node != NULL; node = node->next) {

		p = (ar_set_copy_params_t *)node->data;
		if (p != NULL && strcmp(p->ar_set_copy_name, name) == 0) {
			cur_params = p;
			return (0);
		}
	}
	if (noDefine && !(*(name + strlen(name) - 1) == 'R')) {
		/*
		 * This means the set was not found and no new sets
		 * should be defined. So the set is missing.
		 */
		return (-1);
	}

	/*
	 * Allocate a new copyparams
	 */
	size = sizeof (ar_set_copy_params_t);
	p = (ar_set_copy_params_t *)mallocer(size);
	if (p == NULL) {
		return (-1);
	}
	if (init_ar_copy_params(p) != 0) {
		free_ar_set_copy_params(p);
		return (-1);
	}

	strlcpy(p->ar_set_copy_name, name, sizeof (p->ar_set_copy_name));

	if (cfg_insert_copy_params(arch_cfg->archcopy_params, p) != 0) {
		free_ar_set_copy_params(p);
		return (-1);
	}

	if (*(name + strlen(name) - 1) == 'R') {
		cur_params = p;
	}

	return (0);
}


/*
 * Find a VSN pool. return null if not found
 */
static vsn_map_t *
find_vsn_map(
char *map_name)
{

	node_t *node;
	vsn_map_t *map;

	Trace(TR_DEBUG, "finding vsn_map %s", map_name);
	for (node = arch_cfg->vsn_maps->head;
	    node != NULL;
	    node = node->next) {

		map = (vsn_map_t *)node->data;
		if (strcmp(map_name, map->ar_set_copy_name) == 0) {
			Trace(TR_DEBUG, "found vsn_map %s",
			    map_name);
			return (map);
		}
	}
	Trace(TR_OPRMSG, "finding vsn_map failed to find %s", map_name);
	return (NULL);
}


/*
 * Find a VSN pool. return null if not found
 */
static vsn_pool_t *
find_vsn_pool(
char *poolName)
{

	node_t *node;
	vsn_pool_t *pool;

	Trace(TR_DEBUG, "finding vsn_pool %s", poolName);
	for (node = arch_cfg->vsn_pools->head;
	    node != NULL;
	    node = node->next) {

		pool = (vsn_pool_t *)node->data;
		if (strcmp(poolName, pool->pool_name) == 0) {
			Trace(TR_DEBUG, "found vsn_pool %s",
			    poolName);
			return (pool);
		}
	}
	Trace(TR_OPRMSG, "failed to find vsn_pool %s", poolName);
	return (NULL);
}


/*
 * make the default copy params for each allset and for each filesystem.
 */
static int
init_copy_params(void)
{

	int	i;
	uname_t name;

	Trace(TR_DEBUG, "initializing copy_params");

	if (find_arch_cp_params(ALL_SETS) != 0) {

		return (-1);

	}
	/*
	 * Define allsets.1 - allsets.4.
	 */
	for (i = 1; i <= MAX_ARCHIVE; i++) {
		snprintf(name, sizeof (name), ALL_SETS".%d", i);
		if (find_arch_cp_params(name) != 0) {
			return (-1);
		}
	}

	return (0);
}

/*
 * this was the function used to set up disk archiving when it was done as
 * a copy parameter. That is no longer a valid mechanism for specifiying disk
 * archiving
 */
static void
paramsDiskArchive(void)
{

	Trace(TR_DEBUG, "handling old format disk archive assignment");
	ReadCfgError(CustMsg(4541));
}


/*
 * Invalid parameter.
 */
static void
paramsInvalid(void)
{

	/* '%s' is not a valid archive set parameter */
	ReadCfgError(CustMsg(4420), dirName);
}


/*
 * Process 'priority' for SetFieldValue().
 * this method implements an interface but doesnt use all of the args.
 */
/* ARGSUSED */
static int
paramsPrioritySet(
void *v,
char *value,
char *buf,
int bufsize)
{

	priority_t *pri;
	char *p;
	double val;

	Trace(TR_DEBUG, "handling priority %s for %s", Str(value),
	    cur_params->ar_set_copy_name);


	strlcpy(dirName, token, sizeof (dirName)-2);
	if (ReadCfgGetToken() == 0) {
		ReadCfgError(CustMsg(4508));
	}

	/* use setfield as a check field here. */
	if (SetFieldValue(&tmp_arch_set, Priorities, dirName, token,
	    msgFuncSetField) != 0) {

		if (errno == ENOENT) {
			/* '%s' is not a valid priority name */
			ReadCfgError(CustMsg(4507), token);
		}
		ReadCfgError(0, errMsg);
	}

	/* Params are valid so insert into cur_params */
	/* error checking done for this already. */
	val = strtod(value, &p);
	pri = (priority_t *)mallocer(sizeof (priority_t));
	if (pri == NULL) {
		ReadCfgError(samerrno);
	}
	strlcpy(pri->priority_name, dirName, sizeof (pri->priority_name));
	pri->value = (float)val;
	pri->change_flag |= AR_PR_value;
	if (lst_append(cur_params->priority_lst, pri) != 0) {
		free(pri);
		ReadCfgError(samerrno);
	}

	return (0);
}


/*
 * Process 'reserve' for SetFieldValue().
 * the format for the line in the archiver.cmd would be
 * set.1 -reserve set -reserve owner
 * or some such.  This function handles one pair of -reserve value
 * at a time.
 * user owner group are mutually exclusive.
 */
static int
paramsReserveSet(
void *v,
char *value,
char *buf,
int bufsize)
{

	short	*val = (short *)v;
	int	i;
	int	reserve;

	Trace(TR_DEBUG, "handling reserve");

	/*
	 * Check if value == any of the names in the EnumTable Reserves.
	 */
	for (i = 0; strcmp(value, Reserves[i].RsName) != 0; i++) {
		if (*Reserves[i].RsName == '\0') {
			/* Invalid %s value ('%s') */
			snprintf(buf, bufsize, GetCustMsg(4538),
			    "reserve", value);
			errno = EINVAL;
			return (-1);
		}
	}

	reserve = Reserves[i].RsValue;
	if (reserve & (RM_dir | RM_user | RM_group)) {
		/*
		 * The "owner" component is mutually exclusive.
		 * Instead of returning an error if a previous setting
		 * is detected, clear it and set it to the new value
		 */
		if (cur_params->reserve & (RM_dir | RM_user | RM_group)) {
			cur_params->reserve &= ~(RM_dir | RM_user | RM_group);
		}
	}

	*val |= reserve;
	return (0);
}


/*
 * Set field for an archive set.
 */
static void
paramsSetfield(void)
{


	if (dirName[0] != '-' || dirName[1] == '\0') {
		/* '%s' is not a valid archive set parameter */
		ReadCfgError(CustMsg(4420), dirName);
	}
	/*
	 * Try an empty value.
	 */
	*token = '\0';

	if (SetFieldValue(cur_params, params_tbl, dirName+1,
	    token, msgFuncSetField) != 0) {

		if (errno == ENOENT) {
			/* '%s' is not a valid archive set parameter */
			ReadCfgError(CustMsg(4420), dirName);
		}
		if (errno != EINVAL) {
			ReadCfgError(0, errMsg);
		}
		/*
		 * Parameter requires a value.
		 */
		ReadCfgGetToken();

		if (SetFieldValue(cur_params, params_tbl, dirName+1,
		    token, msgFuncSetField) != 0) {

			ReadCfgError(0, errMsg);
		}
	}
}

static void
paramsFillvsns(void) {


	/* Try to use the token value if it isn't a parameter */
	if (ReadCfgGetToken() != 0 && token[0] != '-') {
		Trace(TR_DEBUG, "Setting fillvsnsmin, token: %s", token);
		if (SetFieldValue(cur_params, params_tbl, dirName+1,
		    token, msgFuncSetField) != 0) {
			ReadCfgError(0, errMsg);
		}
	} else {
		/* Tell procParams we peeked, but didn't use the token */
		peekedToken = B_TRUE;
	}
	cur_params->fillvsns = B_TRUE;
	cur_params->change_flag |= AR_PARAM_fillvsns;
}

/*
 * Compare two file properties descriptions
 */
static boolean_t
criteriaMatches(
ar_set_criteria_t *cr1,
ar_set_criteria_t *cr2)
{

	Trace(TR_DEBUG, "criteriaMatches %s %s",
	    cr1->set_name, cr2->set_name);

	/*
	 * Compare paths.
	 */
	if (strcmp(cr1->path, cr2->path) != 0) {
		return (B_FALSE);
	}

	/*
	 * Compare file names.
	 */
	if (strcmp_nulls(cr1->name, cr2->name) != 0) {
		return (B_FALSE);
	}

	/*
	 * can only apply each condition if the change_flag is set for
	 * both criteria. However if the changeflag is not set for both
	 * this can still be a match. This is a valid test because there
	 * are no default criteria. We cannot rely on values to not be set
	 * when the flags are not set becuase some of these could be user
	 * passed structures.
	 */
	if (strcmp_nulls(cr1->fs_name, cr2->fs_name) == 0 &&
	    (((cr1->change_flag & cr2->change_flag & AR_ST_user) &&
	    strcmp_nulls(cr1->user, cr2->user) == 0) ||
	    (~cr1->change_flag & ~cr2->change_flag & AR_ST_user)) &&

	    (((cr1->change_flag & cr2->change_flag & AR_ST_group) &&
	    strcmp_nulls(cr1->group, cr2->group) == 0) ||
	    (~cr1->change_flag & ~cr2->change_flag & AR_ST_group)) &&

	    (((cr1->change_flag & cr2->change_flag & AR_ST_minsize) &&
	    cr1->minsize == cr2->minsize) ||
	    (~cr1->change_flag & ~cr2->change_flag & AR_ST_minsize)) &&

	    (((cr1->change_flag & cr2->change_flag & AR_ST_maxsize) &&
	    cr1->maxsize == cr2->maxsize) ||
	    (~cr1->change_flag & ~cr2->change_flag & AR_ST_maxsize)) &&

	    (((cr1->change_flag & cr2->change_flag & AR_ST_access) &&
	    cr1->access == cr2->access) ||
	    (~cr1->change_flag & ~cr2->change_flag & AR_ST_access))) {

		Trace(TR_DEBUG, "criteriaMatches returning true");
		return (B_TRUE);
	}



	Trace(TR_DEBUG, "criteriaMatches returning false");
	return (B_FALSE);

}


/*
 * compares two possibly NULL strings.
 * if s1 is not null but s2 is 1 is returned.
 * if s1 is null and s2 is not -1 is returned.
 * if both are NULL 0 is returned.
 * if neither are null strcmp(s1,s2) is returned.
 *
 */
static int
strcmp_nulls(
char *s1,
char *s2)
{

	if (s1 != NULL) {
		if (s2 == NULL) {
			return (1);
		} else {
			return (strcmp(s1, s2));
		}
	} else if (s2 != NULL) {
		return (-1);
	}
	return (0);
}


/*
 * static void
 * setDefaultParams(int which)
 *
 * Notes on the implementation:
 * This method is actually executed twice. Once for the first
 * instance of allsets.x and once for the first instance of other.x
 * The reason is that allsets params get set for all allset.copies during
 * the first execution.
 *
 * each otherset.copy parameters are set to the allsets vals during the first
 * run for other.x
 *
 * In readcmd.c this method preserves a number of fields in the ArchSet
 * structure that don't need to be here. The reason is that the copy_params
 * structure only contains things related to copy params. Whereas the ArchSet
 * structure that readcmd is operating on contains other things. When this
 * method is executed nothing has been set for the params that are being
 * modified (except their name). This is guaranteed by the static variables
 * first_pass_allsets_copy and first_pass_other_copy. The only exception other
 * than the name is the change_flag which should be preserved at their
 * initial values of zero.
 *
 * which = 1 for allsets.copy
 * which = 0 for all others
 *
 */
static void
setDefaultParams(int which)
{

	ar_set_copy_params_t	*tmp_defs[MAX_COPY + 1];
	node_t	*in, *out;
	int			found = 0;

	if (which == 0) {
		/* static boolean_t first = TRUE; */
		/* first_pass_allsets_copy */

		if (!first_pass_other_copy) {
			return;
		}
		first_pass_other_copy = B_FALSE;
	} else {
		/* first_pass_other_copy */
		/* static boolean_t first = TRUE; */

		if (!first_pass_allsets_copy) {
			return;
		}
		first_pass_allsets_copy = B_FALSE;
	}

	/* find the allsets definitions first. */
	for (out = arch_cfg->archcopy_params->head; out != NULL;
	    out = out->next) {

		ar_set_copy_params_t *cp;
		int num;

		cp = (ar_set_copy_params_t *)out->data;
		if (strcmp(cp->ar_set_copy_name, ALL_SETS) == 0) {
			tmp_defs[0] = cp;
			found++;
		} else if (strstr(cp->ar_set_copy_name, ALL_SETS) ==
		    cp->ar_set_copy_name) {

			num = AS_COPY(cp->ar_set_copy_name) + 1;
			tmp_defs[num] = cp;
			found++;
		}
		if (found > MAX_COPY + 1)
			break;

	}

	for (out = arch_cfg->archcopy_params->head; out != NULL;
	    out = out->next) {

		ar_set_copy_params_t *def;	/* Default Archive Set */
		ar_set_copy_params_t *new;	/* Archive Set to change */
		ar_set_copy_params_t save;

		new = (ar_set_copy_params_t *)out->data;
		save = *new;

		/* select the allsets copy to use as defaults */
		if (which == 0) {
			/* don't modify allsets */
			if (strstr(new->ar_set_copy_name, ALL_SETS) ==
			    new->ar_set_copy_name) {

				continue;
			}
			def = tmp_defs[AS_COPY(new->ar_set_copy_name) + 1];
		} else {

			/* only modify allsets.x */
			if (strstr(new->ar_set_copy_name, ALL_SETS) !=
			    new->ar_set_copy_name) {

				continue;
			}
			def = tmp_defs[0];
		}


		*new = *def;
		lst_free(save.priority_lst);
		new->priority_lst = lst_create();
		for (in = def->priority_lst->head; in != NULL; in = in->next) {
			priority_t *p = (priority_t *)mallocer(
			    sizeof (priority_t));
			memcpy(p, in->data, sizeof (priority_t));
			p->change_flag = 0;
		}

		/*
		 * Restore those fields that are not to be set
		 * to 'allsets' values.
		 */
		memmove(new->ar_set_copy_name, save.ar_set_copy_name,
		    sizeof (new->ar_set_copy_name));

		new->change_flag = save.change_flag;

		/* recycle ignore. defbits */
		new->recycle.change_flag = save.recycle.change_flag;

	}

}


static node_t *
find_node_str(
sqm_lst_t *l,
char *str)
{

	node_t *node;
	char *datastr;

	Trace(TR_DEBUG, "find_node_str %s", str ? str : "NULL");
	for (node = l->head; node != NULL; node = node->next) {
		datastr = (char *)node->data;
		if (strcmp(datastr, str) == 0) {
			Trace(TR_DEBUG, "find_node_str found %s",
			    str);

			return (node);
		}
	}
	Trace(TR_DEBUG, "find_node_str did not find %s", str);
	return (NULL);
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

	parsing_error_t *err;
	char *filtered;
	char err_buf[80];

	if (line != NULL) {
		/*
		 * While reading the file.
		 */
		if (msg != NULL) {

			/*
			 * Error message.
			 */
			Trace(TR_OPRMSG, "error in  %d: %s %s",
			    lineno, line, msg);

			err = (parsing_error_t *)mallocer(
			    sizeof (parsing_error_t));
			if (err == NULL) {
				mem_err = samerrno;
				return;
			}
			strlcpy(err->input, line, sizeof (err->input));
			if (strstr(msg, "Error in line") != NULL) {
				filtered = strpbrk(msg, ":");
				filtered++;
				if (isspace(*filtered)) {
					filtered++;
				}
			} else {
				filtered = msg;
			}
			strlcpy(err->msg, filtered, sizeof (err->msg));
			err->line_num = lineno;
			err->error_type = ERROR;
			err->severity = 1;
			if (lst_append(error_list, err) != 0) {
				mem_err = samerrno;
				return;
			}
		}

	} else if (lineno >= 0) {
		Trace(TR_OPRMSG, "error :%s", msg);
	} else if (lineno < 0) {
		/*
		 * Missing command file is not an error.
		 */
		if (errno == ENOENT) {
			no_cmd_file = B_TRUE;
		} else {
			no_cmd_file = B_FALSE;
			StrFromErrno(errno, err_buf, sizeof (err_buf));
			snprintf(open_error, sizeof (open_error), err_buf);
		}
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		Trace(TR_OPRMSG, "%s '%s': %s\n", GetCustMsg(4400),
		    cmdFname, err_buf);
	}
}


/*
 * Data field error message function.
 */
/* ARGSUSED */
static void
msgFuncSetField(
int code,
char *msg)
{

	strlcpy(errMsg, msg, sizeof (errMsg)-1);

	/*
	 * because of the way the parser works we do not want
	 * to trace messages that say missing value. The parser
	 * tries each copy param without a value first.
	 */
	if (strstr(msg, "Missing") == NULL) {
		Trace(TR_ERR, "setfield error: %s", msg);
	}
}


/*
 * Normalize path.
 * Remove ./ ../ // sequences in a path.
 * returns Start of path.  NULL if invalid, (too many ../'s).
 * Note: path array must be able to hold one more character.
 */
static char *
normalizePath(
char *path)
{

	char *p, *ps, *q;

	Trace(TR_DEBUG, "normalizing path");

	ps = path;
	/* Preserve an absolute path. */
	if (*ps == '/') {
		ps++;
	}
	strncat(ps, "/", sizeof (token)-1);
	p = q = ps;
	while (*p != '\0') {
		char *q1;

		if (*p == '.') {
			if (p[1] == '/') {
				/*
				 * Skip "./".
				 */
				p++;
			} else if (p[1] == '.' && p[2] == '/') {
				/*
				 * "../" Back up over previous component.
				 */
				p += 2;
				if (q <= ps) {
					return (NULL);
				}
				q--;
				while (q > ps && q[-1] != '/') {
					q--;
				}
			}
		}

		/*
		 * Copy a component.
		 */
		q1 = q;
		while (*p != '/') {
			*q++ = *p++;
		}
		if (q1 != q)
			*q++ = *p;

		/*
		 * Skip successive '/'s.
		 */
		while (*p == '/') {
			p++;
		}
	}
	if (q > ps && q[-1] == '/') {
		q--;
	}
	*q = '\0';

	return (path);
}
