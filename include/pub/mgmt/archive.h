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
#ifndef	_ARCHIVE_H
#define	_ARCHIVE_H

#pragma ident	"$Revision: 1.41 $"

/*
 * archive.h - SAM-FS APIs for archiving operations.
 */


#include <stddef.h>

#include "pub/mgmt/types.h"
#include "mgmt/util.h"
#include "pub/mgmt/struct_key.h"
#include "pub/mgmt/recycle.h"
#include "pub/mgmt/directives.h"

#include "aml/archiver.h"
#include "aml/archset.h"
#include "aml/archreq.h"

/*
 * Note: The attribute setting fields do not require change flags
 * because there is no way for them to be explicity set to their
 * inverse. The presence of the flag means the attribute was set.
 *
 * The char release/stage fields in ar_set_criteria cannot go away
 * because they must still be supported for backwards compatibility
 * earlier 4.5 and 4.6 versions. They could be eliminated when support
 * for version 4.6 is dropped.
 */
#define	ATTR_RELEASE_DEFAULT	0x00000001
#define	ATTR_RELEASE_NEVER	0x00000002
#define	ATTR_RELEASE_PARTIAL	0x00000004
#define	ATTR_PARTIAL_SIZE	0x00000008
#define	ATTR_RELEASE_ALWAYS	0x00000010
#define	RELEASE_ATTR_SET	0x0000001f

#define	ATTR_STAGE_DEFAULT	0x00000020
#define	ATTR_STAGE_NEVER	0x00000040
#define	ATTR_STAGE_ASSOCIATIVE	0x00000080
#define	STAGE_ATTR_SET		0x000000E0

#define	NEVER_RELEASE	'n'
#define	PARTIAL_RELEASE	'p'
#define	ALWAYS_RELEASE	'a'
#define	SET_DEFAULT_RELEASE	'd'
#define	PARTIAL_SIZE	's'
#define	RELEASE_NOT_DEFINED	'0'

#define	NEVER_STAGE	'n'
#define	ASSOCIATIVE_STAGE	'a'
#define	SET_DEFAULT_STAGE	'd'
#define	STAGE_NOT_DEFINED	'0'

#define	ARCH_DUMP_FILE	"archiver.cmd.dump"

/*
 * In the following enums NOT_SET is not same as none. The reason is
 * NOT_SET is for use in resetting to default values(omit the directive from
 * cmd file. None is a valid explict setting.
 */
typedef enum join_method {
	JOIN_NOT_SET = -1,
	NO_JOIN,
	JOIN_PATH
} join_method_t;


typedef enum offline_copy_method {
	OC_NOT_SET = -1,
	OC_NONE,
	OC_DIRECT,
	OC_STAGEAHEAD,
	OC_STAGEALL
} offline_copy_method_t;


typedef enum sort_method {
	SM_NOT_SET = -1,
	SM_NONE,
	SM_AGE,
	SM_PATH,
	SM_PRIORITY,
	SM_SIZE
} sort_method_t;


typedef enum {
	EM_NOT_SET = 0,
	EM_SCAN,	/* Traditional scan */
	EM_SCANDIRS,	/* Scan only directories */
	EM_SCANINODES,	/* Scan only inodes */
	EM_NOSCAN	/* Continuous archive */
} examine_method_t;


/*
 * This structure is used to define archive set's priority
 * parameter and it will be used inside structure ar_set_copy_params.
 *
 * Valid names include: age, ar_immediate, ar_overflow, ar_loaded,
 * copy1, copy2, copy3, copy4, copies, offline, queuewait,
 * rearchive, reqrelease, size, stage_loaded, stage_overflow
 *
 * Typedef enum needed here.
 */
typedef struct priority {
	uname_t	  priority_name;
	float	  value;
	uint32_t  change_flag;
} priority_t;


/* priority_t change_flag mask */
#define	AR_PR_value 0x00000001;


/*
 * This structure is used to define general archive copy's property.
 */
typedef struct ar_general_copy_cfg {
	int	copy_seq;
	uint_t	ar_age;
} ar_general_copy_cfg_t;


/*
 * This structure is used to define archive set copy's property.
 */
typedef struct ar_set_copy_cfg {
	ar_general_copy_cfg_t ar_copy;
	boolean_t	release;    /* FALSE if not set */
	boolean_t	norelease;  /* FALSE if not set */
	uint_t		un_ar_age;
	uint32_t	change_flag;
} ar_set_copy_cfg_t;


typedef enum regexp_type {
	REGEXP = 0,
	FILE_NAME_CONTAINS,
	PATH_CONTAINS,
	FILE_NAME_STARTS_WITH,
	ENDS_WITH
} regexp_type_t;



/*
 * This structure is used to define archive set's search criteria.
 * if fs_name is set to the constant GLOBAL and the criteria is in the
 * global_directives it applies to all filesystems.
 *
 * fs_name, set_name and path must be set. All other fields are optional.
 */
typedef struct ar_set_criteria {
	uname_t fs_name;	/* the fs name or possibly GLOBAL */
	uname_t set_name;	/* the set name */
	upath_t path;		/* the starting path to apply this criteria */
	fsize_t minsize;	/* size in bytes, fsize_reset if not set */
	fsize_t maxsize;	/* size in bytes, fsize_reset if not set */
	upath_t name;		/* regexp selecting files. */
	uname_t user;		/* not '\0' == -user is set */
	uname_t group;		/* not '\0' == -group is set */
	char	release;	/* not used, see attr flags */
	char	stage;		/* not used, see attr flags */
	int	access;
	int			num_copies; /* the number of arch_copies */
	ar_set_copy_cfg_t	*arch_copy[MAX_COPY];
	uint32_t		change_flag;

	/*
	 * a key used to match criteria passed
	 * into the API with criteria in the
	 * configuration. Not user setable
	 */
	struct_key_t		key;
	boolean_t		nftv;
	uname_t			after; /* added 4.5 */

	/* attr_flags supercede the release and stage fields */
	int32_t			attr_flags; /* see XXX_ATTR defines */
	int32_t			partial_size;
} ar_set_criteria_t;

/*
 * Global and FS Directives Options Flags
 * Added in 4.4 patch 02
 */
#define	SCAN_SQUASH_ON		0x00000001
#define	SETARCHDONE_ON		0x00000002
#define	FS_NOSAM		0x00000004
#define	FS_HAS_EXPLICIT_DEFAULT	0x00000008

/*
 * global directives
 * Global directives can be overridden on a per filesystem basis.
 *
 * The criteria in ar_set_lst are applied after any file system specific
 * criteria for each file system.
 *
 * options field added in 4.4 patch 02
 */
typedef struct ar_global_directive {
	boolean_t	wait;		/* FALSE if not set */
	uint_t		ar_interval;	/* global scanning interval */
	ExamMethod_t	scan_method;	/* scan method */
	sqm_lst_t	*ar_bufs;	/* list of buffer_directive_t */
	sqm_lst_t	*ar_max;	/* list of buffer_directive_t */
	boolean_t	archivemeta;	/* if true, metadata will archive */
	upath_t		notify_script;	/* event notification script */
	upath_t		log_path;
	sqm_lst_t	*ar_drives;	/* list of drive_directive_t */
	sqm_lst_t	*ar_set_lst;	/* global ar_set_criteria list */
	sqm_lst_t	*ar_overflow_lst; /* list of buffer_directive_t */
	uint32_t	change_flag;
	int32_t		options;	/* see options flags above */
	sqm_lst_t	*timeouts;	/* string timeout directives */
	uint_t		bg_interval;
	int		bg_time;	/* hhmm */
} ar_global_directive_t;


/*
 * File system directives
 *
 * ar_set_criteria does not contain the global criteria that might apply to
 * this filesystem.
 */
typedef struct ar_fs_directive {
	uname_t		fs_name;	/* file system name */
	upath_t		log_path;	/* path to log file */
	uint_t		fs_interval;	/* time between archiver operations */

	/*
	 * ordered list of ar_set_criteria_t for this fs
	 */
	sqm_lst_t	*ar_set_criteria;

	boolean_t	wait;		/* if true, archiver won't run */
	ExamMethod_t	scan_method;	/* archiver scan method */
	boolean_t	archivemeta;	/* if true metadata will be archived */
	int		num_copies;	/* number of copies of copies for fs */
	ar_set_copy_cfg_t *fs_copy[MAX_COPY];
	uint32_t	change_flag;
	int32_t		options;	/* see options flags above */
	uint_t		bg_interval;
	int		bg_time;	/* hhmm */
} ar_fs_directive_t;


/*
 * This structure is used to define an archive set copy's parameters.
 */
typedef struct ar_set_copy_params {
	uname_t	ar_set_copy_name; /* set or fs name with .copy_num */
	fsize_t archmax;	/* size in bytes of maximum archive tar file */
	int	bufsize;
	boolean_t buflock;	/* FALSE if not set */
	int	drives;
	fsize_t drivemax;
	fsize_t	drivemin;
	fsize_t	ovflmin;	/* min bytes for archive to 2 volumes */

	vsn_t		disk_volume;	/* not used in 4.4 and beyond */
	boolean_t	fillvsns;	/* FALSE if not set */
	boolean_t	tapenonstop;	/* cannot be used for remote */

	short		reserve;	/* see Reserves in aml/archset.h */
	sqm_lst_t	*priority_lst;	/* list of priority_t */
	boolean_t	unarchage;	/* if true modify age else access */
	join_method_t	join;		/* No longer used as of 5.0 */
	sort_method_t	rsort;
	sort_method_t	sort;
	offline_copy_method_t	offline_copy;
	uint_t		startage;
	int		startcount;
	fsize_t		startsize;
	rc_param_t	recycle;

	/*
	 * DEBUG parameters only have any affect if compiled with DEBUG
	 * if we don't keep them we will strip them out of the archiver.cmd
	 * file Maybe that's OK.
	 */
	int		simdelay;
	boolean_t	tstovfl;

	uint32_t	change_flag;
	boolean_t	directio;	/* 4.2 directio not used if false */
	short		rearch_stage_copy; /* added 4.5, 0 if not specified */
	uint_t		queue_time_limit;	/* added 4.6 */
	fsize_t		fillvsns_min;
} ar_set_copy_params_t;


/*
 * vsn_pool
 */
#define	MAX_VSN_POOL_NAME_LEN 16

typedef struct vsn_pool {
	uname_t	pool_name;	/* name must be <= MAX_VSN_POOL_NAME_LEN */
	mtype_t	media_type;
	sqm_lst_t	*vsn_names;
} vsn_pool_t;


/*
 * vsn and set mapping part
 */
typedef struct vsn_map {
	uname_t	  ar_set_copy_name;
	mtype_t	  media_type;
	sqm_lst_t	  *vsn_names;
	sqm_lst_t	  *vsn_pool_names;
} vsn_map_t;


/*
 * There is one arfind process per filesystem. This structure exists
 * to pair a filesystem name with its ArfindState.
 */
typedef struct ar_find_state {
	uname_t fs_name;		/* the name of the file system */
	struct	ArfindState state;	/* the state of arfind */
} ar_find_state_t;



/*
 * Functions to set up archiving.
 */

/*
 * get the default archive set copy configuration.
 */
int get_default_ar_set_copy_cfg(ctx_t *ctx, ar_set_copy_cfg_t **copy_cfg);


/*
 * get the default archive set criteria.
 */
int get_default_ar_set_criteria(ctx_t *ctx, ar_set_criteria_t **criteria);


/*
 * get the global directive for archiver.
 */
int get_ar_global_directive(ctx_t *ctx, ar_global_directive_t **ar_global);


/*
 * set the global directive for archiver.
 */
int set_ar_global_directive(ctx_t *ctx, ar_global_directive_t *ar_global);


/*
 * get the default global directive for archiver.
 */
int get_default_ar_global_directive(ctx_t *ctx,
	ar_global_directive_t **ar_global);


/*
 * returns a list of the names of all ar_set_criterias currently in the config.
 * This will include no_archive if there are any no_archive criteria present.
 */
int
get_ar_set_criteria_names(ctx_t *ctx, sqm_lst_t **ar_set_crit_names);


/*
 * get a specific archive set. The returned list will include criteria
 * from any file system that includes the set and any global criteria for
 * the set.
 */
int get_ar_set(ctx_t *ctx, const uname_t set_name,
	sqm_lst_t **ar_set_criteria_list);


/*
 * get all the archive set criteria for a specific file
 * system. Returns only the criteria assigned directly to the
 * specified file system. Global criteria are not returned here. To
 * get any global criteria call this function with the filesytem name
 * set to the constant GLOBAL.
 *
 * These two sets of information- global and fs
 * specific criteria are not mixed because they can not be reordered together.
 * The global criteria is always applied after all of file system specific
 * criteria.
 */
int get_ar_set_criteria_list(ctx_t *ctx, const uname_t fs_name,
	sqm_lst_t **ar_set_criteria_list);


/*
 * get all the archive set criteria.
 */
int get_all_ar_set_criteria(ctx_t *ctx, sqm_lst_t **ar_set_criteria_list);


/*
 * get a specific file systems directive for archiving.
 */
int get_ar_fs_directive(ctx_t *ctx, const uname_t fs_name,
	ar_fs_directive_t **ar_fs_directive);


/*
 * get all file systems directives for archiving.
 */
int
get_all_ar_fs_directives(ctx_t *ctx, sqm_lst_t **ar_fs_directive_list);


/*
 * Functions to set and reset file system directive for archiving.
 */
int set_ar_fs_directive(ctx_t *ctx, ar_fs_directive_t *ar_fs_directive);
int reset_ar_fs_directive(ctx_t *ctx, const uname_t fs_name);

/*
 * function to modify an ar_set_criteria_t. The fs_name, set_name and key in
 * the ar_set_criteria are used to identify which criteria to modify.
 * This function will modify the configuration to match the fields of crit
 * that have their change_flag bit set.
 */
int modify_ar_set_criteria(ctx_t *ctx, ar_set_criteria_t *crit);


/*
 * Function to get the default file system directive for archiving.
 */
int get_default_ar_fs_directive(ctx_t *ctx,
	ar_fs_directive_t **ar_fs_directive);


/*
 * get all copy parameters.
 */
int get_all_ar_set_copy_params(ctx_t *ctx, sqm_lst_t **ar_set_copy_params_list);


/*
 * returns a list of the names of all sets for which copy parameters can be
 * defined. The returned list includes the name of each file system and
 * the psuedo set ALLSETS. The performance of this method in terms of time
 * is identical to get_all_ar_set_copy_params.
 */
int get_ar_set_copy_params_names(ctx_t *ctx, sqm_lst_t **ar_set_copy_names);


/*
 * get copy parameters for a specific archive set copy.
 */
int get_ar_set_copy_params(ctx_t *ctx, const uname_t ar_set_copy_name,
	ar_set_copy_params_t **ar_set_copy_parameters);


/*
 * get the copy parameters for all copies defined for this archive set
 */
int get_ar_set_copy_params_for_ar_set(ctx_t *ctx, const char *ar_set_name,
	sqm_lst_t **ar_set_copy_params_list);


/*
 * set_ar_set_copy_params set the parameters. The input copy parameters
 * will replace the exiting params for the copy.
 */
int set_ar_set_copy_params(ctx_t *ctx,
	ar_set_copy_params_t *ar_set_copy_parameters);


/*
 * reset all non-default parameter settings for the named ar_set_copy_params
 */
int reset_ar_set_copy_params(ctx_t *ctx, const uname_t ar_set_copy_name);


/*
 * Function to get the default copy parameters.
 */
int get_default_ar_set_copy_params(ctx_t *ctx,
	ar_set_copy_params_t **parameters);


/*
 * get all the vsn pools.
 */
int get_all_vsn_pools(ctx_t *ctx, sqm_lst_t **vsn_pool_list);


/*
 * get a specific vsn pool.
 */
int get_vsn_pool(ctx_t *ctx, const uname_t pool_name, vsn_pool_t **vsn_pool);


/*
 * sets in_use to true if the named pool is in use. If the pool is in use
 * the function also populates the uname used_by with a string containing
 * the name of an archive copy that uses the pool. There could be many that
 * use it but only one gets returned.
 *
 * in_use and used_by are return parameters.
 */
int is_pool_in_use(ctx_t *ctx, const uname_t pool_name,
	boolean_t *in_use, uname_t used_by);


/*
 * Functions to add, modify or remove a vsn pool.
 */
int add_vsn_pool(ctx_t *ctx, const vsn_pool_t *vsn_pool);
int modify_vsn_pool(ctx_t *ctx, const vsn_pool_t *vsn_pool);
int remove_vsn_pool(ctx_t *ctx, const uname_t pool_name);


/*
 * get all vsn and archive set copy associations.
 */
int get_all_vsn_copy_maps(ctx_t *ctx, sqm_lst_t **vsn_map_list);


/*
 * get a specific vsn and archive set copy association given the copy name.
 */
int get_vsn_copy_map(ctx_t *ctx, const uname_t ar_set_copy_name,
	vsn_map_t **vsn_map);


/*
 * add, modify or remove a vsn and archive set copy association.
 */
int add_vsn_copy_map(ctx_t *ctx, const vsn_map_t *vsn_map);
int modify_vsn_copy_map(ctx_t *ctx, const vsn_map_t *vsn_map);
int remove_vsn_copy_map(ctx_t *ctx, const uname_t ar_set_copy_name);


/*
 * get the default drive directive for archiving for a library.
 */
int get_default_ar_drive_directive(ctx_t *ctx, /* used by rpc */
    uname_t lib_name,
    boolean_t global,
    drive_directive_t **drive);


/*
 * Functions relating to control of archiving
 */


/*
 * Causes the archiver to run for the named file system. This function
 * overrides any wait commands that have previously been applied to the
 * filesystem through the archiver.cmd file.
 */
int ar_run(ctx_t *ctx, /* used by rpc */
    uname_t fs_name);   /* start archiving for fs */


/*
 * Causes the archiver to run for all file systems. This function
 * overrides any wait commands that have previously been applied
 * through the archiver.cmd file.
 */
int ar_run_all(ctx_t *ctx	/* used by rpc */);


/*
 * Causes a soft restart of the archiver.
 */
int ar_rerun_all(ctx_t *ctx	/* used by rpc */);


/*
 * idle archiving for fs. This function stops archiving for the named file
 * system at the next convienent point. e.g. at the end of the current tar file
 */
int ar_idle(ctx_t *ctx, /* used by rpc */
    uname_t fs_name);	/* The fs for which to idle archiving. */


/*
 * idle all archiving. This function stops all archiving at the next convienent
 * point. e.g. at the end of the current tar files
 */
int ar_idle_all(ctx_t *ctx /* used by rpc */);


/*
 * Immediately stop archiving for fs. For a more graceful stop see ar_idle.
 */
int ar_stop(ctx_t *ctx,		/* used by rpc */
    uname_t fs_name); /* stop archiving for fs */


int ar_stop_all(ctx_t *ctx	/* used by rpc */);


/*
 * Interupt the archiver and restart it for all filesystems..
 * This occurs immediately without regard to the state of the archiver.
 * It may terminate media copy operations and therefore waste space on media.
 * This should be done with caution.
 *
 * The capability to restart archiving for a single file system does not exist
 * in samfs.
 */
int ar_restart_all(ctx_t *ctx	/* used by rpc */);


/*
 * Function to get the status of the archiverd daemon.
 * see aml/archiver.h for ArchiverdState.
 */
int
get_archiverd_state(ctx_t *ctx,		/* used by rpc */
	struct ArchiverdState **state); /* return status of archiverd  */


/*
 * get the status of the arfind process for a given file system.
 * GUI note: This contains the status for the archive management page
 */
int get_arfind_state(ctx_t *ctx,	/* used by rpc */
	uname_t fsname,			/* name of fs to get status for */
	ar_find_state_t **state);	/* The state return */


/*
 * get the status of the arfind processes for each archiving file system.
 */
int get_all_arfind_state(ctx_t *ctx,	/* used by rpc */
	sqm_lst_t **find_states);	/* return list of ar_find_state_t */


/*
 * This NON-RPC function will extract the name of the file currently being
 * copied. If a file is not currently being copied this will set file_name
 * to '\0'.
 */
int
get_current_file_name(
    struct ArcopyInstance *copy,	/* Input to get name from */
    upath_t file_name);			/* file currently being copied */


/*
 * Returns a list of arch reqs for the named file system.
 * arch reqs contain information about each of the copy processes for an
 * archive set within a file system.
 */
int get_archreqs(ctx_t *ctx,		/* used by rpc */
	uname_t fsname,			/* name of fs to get archreqs for */
	sqm_lst_t **archreqs);	/* return list of ArchReq structs */


/*
 * Returns a list of all arch reqs.
 * arch reqs contain information about each of the copy processes for an
 * archive set within a file system.
 */
int get_all_archreqs(ctx_t *ctx,		/* used by rpc */
	sqm_lst_t **archreqs);		/* List of all struct ArchReqs */


/*
 * This NON-RPC fuction will extract the stage_vsn and media type from the
 * ArcopyInstance's operator message. If a file is not currently being
 * staged vsn[0], file_name[0] and media_type[0] will be set to '\0'.
 * If a vsn is returned it means that the archiver is waiting on a stage
 * from this vsn.
 */
int
get_stage_vsn(
    struct ArcopyInstance *copy, /* The arcopy from which to get stage vsn */
    upath_t file_name,		/* filename of the file being staged */
    vsn_t vsn_name,		/* vsn being staged from */
    mtype_t media_type);	/* The media type of the vsn */



/*
 * if the named group is a valid unix group, is_valid will be set to true.
 * if any errors are encountered -1 is returned.
 */
int
is_valid_group(
ctx_t *ctx,
uname_t group,		/* the name of the group to check */
boolean_t *is_valid);	/* return value. B_TRUE if group exists */

/*
 * if the named user is a valid user, is_valid will be set to true.
 * if any errors are encountered -1 is returned.
 */
int
is_valid_user(
ctx_t *ctx,
char *user,		/* the name of the user to check */
boolean_t *is_valid);	/* return value. B_TRUE if user exists */


/*
 * activate_archiver_cfg
 *
 * must be called after changes are made to the archiver configuration. It
 * first checks the configuration to make sure it is valid. If the
 * configuration is valid it is made active by signaling sam-fsd.
 *
 * If any configuration errors are encountered a -2 will be returned.
 * In this case sam-fsd is not signaled the configuration will not
 * become active),
 * If the errors are related to policies that do not have vsns defined,
 * err_warn_list is populated by the name of the offending copy.
 *
 * In addition to encountering errors. It is possible that warnings
 * will be encountered. Warnings arise when the configuration will not
 * prevent the archiver from running but the configuration is suspect.
 * If warnings are encountered a -3 will be returned and the
 * configuration will be made active. If the warnings are related to
 * policies that do not have vsn available, err_warn_list will be
 * populated with the names of the copies that have no vsns available.
 *
 * If this function fails to execute properly a -1 is returned. This indicates
 * that there was an error during execution. In this case err_warn_list will
 * not need to be freed by the caller. The configuration has not been
 * activated.
 *
 * returns
 * 0  indicates successful execution, no errors or warnings encountered.
 * -1 indicates an internal error.
 * -2 indicates errors encountered in config.
 * -3 indicates warnings encountered in config.
 */
int
activate_archiver_cfg(ctx_t *ctx, sqm_lst_t **err_warn_list);



#define	AR_OPT_COPY_1		0x00000001
#define	AR_OPT_COPY_2		0x00000002
#define	AR_OPT_COPY_3		0x00000004
#define	AR_OPT_COPY_4		0x00000008
#define	AR_OPT_DEFAULTS		0x00000010
#define	AR_OPT_NEVER		0x00000020
#define	AR_OPT_RECURSIVE	0x00000040
#define	AR_OPT_CONCURRENT	0x00000080
#define	AR_OPT_INCONSISTENT	0x00000100

/*
 * Archive files and directories or set archive options for them.
 *
 * AR_OPT_COPY_X	specify which copy to archive or set options for
 * AR_OPT_DEFAULTS	reset options to default
 * AR_OPT_NEVER		set archive never
 * AR_OPT_RECURSIVE	recursively perform operation
 * AR_OPT_CONCURRENT	enable concurrent archiving for file
 * AR_OPT_INCONSISTENT	enable inconsistent archiving for file
 *
 * If any options other than RECURSIVE or COPY_X are specified the
 * command will set options flags for the files but will not schedule
 * archiving for them.
 *
 * If no AR_OPT_COPY_X flags are specified the command will apply to all
 * archive copies.
 *
 * AR_OPT_INCONSISTENT and AR_OPT_CONCURRENT are advanced options.
 * inconsistent means that an archive copy can be created even if the
 *		file is modified while it is being copied to the media. These
 *		copies can only be used after a samfsrestore.
 *
 * concurrent means that a file can be archived even if opened for
 *		write. This can lead to wasted media.
 *
 * job_id will contain the activity id of the command requesting files
 *		to be archived or will be null if the command
 *		completed. It should be noted however that archiving
 *		is asynchronous and will not necessarily have
 *		completed even if null is returned.
 */
int
archive_files(ctx_t *c, sqm_lst_t *files, int32_t options, char **job_id);


/*
 * get utilization of archive copies sorted by usage
 * This can be used to get copies with low free space
 *
 * Input:
 *	int count	- n copies with top usage
 *
 * Returns a list of formatted strings
 *	name=copy name
 *	type=mediatype
 *	free=freespace in kbytes
 *	usage=%
 */
int get_copy_utilization(ctx_t *ctx, int count, sqm_lst_t **strlst);

/* Structures and Definitions for parsing copy util key=value pairs */
typedef struct arcopy {
	char name[32];		/* archive copy name */
	mtype_t mtype;		/* media type 'li' */
	fsize_t capacity;	/* total capacity */
	fsize_t free;		/* free space */
	int32_t usage;		/* usage % */
} arcopy_t;

/*
 * Free methods.
 */
void free_buffer_directive_list(sqm_lst_t *buffer_directive);
void free_drive_directive_list(sqm_lst_t *drive_directive);
void free_ar_set_criteria_list(sqm_lst_t *ar_set_criteria_list);
void free_ar_set_criteria(ar_set_criteria_t *ar_set_criteria);

void free_ar_global_directive(ar_global_directive_t *ar_global_directive);
void free_ar_fs_directive(ar_fs_directive_t *fs_directive);
void free_ar_fs_directive_list(sqm_lst_t *ar_fs_directive_list);
void free_ar_set_copy_params(ar_set_copy_params_t *set_copy_params);
void free_ar_set_copy_params_list(sqm_lst_t *ar_set_copy_params_list);

void free_vsn_pool(vsn_pool_t *vsn_pool);
void free_vsn_pool_list(sqm_lst_t *vsn_pool_list);
void free_vsn_map(vsn_map_t *vsn);
void free_vsn_map_list(sqm_lst_t *vsn_map_list);

void free_list_of_arfind_status(sqm_lst_t *arfind_status);
void free_list_of_arcopy_status(sqm_lst_t *arcopy_status);


/* flags to use to indicate that a value should be set */

/* ar_set_copy_params flags */
#define	AR_PARAM_archmax	0x00000100
#define	AR_PARAM_bufsize	0x00000200
#define	AR_PARAM_disk_volume	0x00000400
#define	AR_PARAM_drivemin	0x00000800
#define	AR_PARAM_drives		0x00001000
#define	AR_PARAM_fillvsns	0x00002000
#define	AR_PARAM_join		0x00004000
#define	AR_PARAM_buflock	0x00008000
#define	AR_PARAM_offline_copy	0x00010000
#define	AR_PARAM_ovflmin	0x00020000
#define	AR_PARAM_reserve	0x00040000
#define	AR_PARAM_simdelay	0x00080000
#define	AR_PARAM_sort		0x00100000
#define	AR_PARAM_rsort		0x00200000
#define	AR_PARAM_startage	0x00400000
#define	AR_PARAM_startcount	0x00800000
#define	AR_PARAM_startsize	0x01000000
#define	AR_PARAM_tapenonstop	0x02000000
#define	AR_PARAM_tstovfl	0x04000000
#define	AR_PARAM_drivemax	0x08000000
#define	AR_PARAM_unarchage	0x10000000
#define	AR_PARAM_directio	0x20000000
#define	AR_PARAM_rearch_stage_copy 0x40000000
#define	AR_PARAM_queue_time_limit 0x80000000

/* ar_fs_directives flags must match global directive flags */
#define	AR_FS_log_path		0x00000001
#define	AR_FS_fs_interval	0x00000002
#define	AR_FS_wait		0x00000004
#define	AR_FS_scan_method	0x00000008
#define	AR_FS_archivemeta	0x00000010
#define	AR_FS_scan_squash	0x00000040
#define	AR_FS_setarchdone	0x00000080
#define	AR_FS_bg_interval	0x00000100
#define	AR_FS_bg_time		0x00000200

/* flags used to indicate the nature of the fs */
#define	AR_FS_shared_fs		0x10000000
#define	AR_FS_no_archive	0x20000000

/* ar_global_directive_t flags */
#define	AR_GL_log_path		0x00000001
#define	AR_GL_ar_interval	0x00000002
#define	AR_GL_wait		0x00000004
#define	AR_GL_scan_method	0x00000008
#define	AR_GL_archivemeta	0x00000010
#define	AR_GL_notify_script	0x00000020
#define	AR_GL_scan_squash	0x00000040
#define	AR_GL_setarchdone	0x00000080
#define	AR_GL_bg_interval	0x00000100
#define	AR_GL_bg_time		0x00000200

/* ar_set_criteria_t flags */
#define	AR_ST_path		0x00000001
#define	AR_ST_minsize		0x00000002
#define	AR_ST_maxsize		0x00000004
#define	AR_ST_name		0x00000008
#define	AR_ST_user		0x00000010
#define	AR_ST_group		0x00000020
#define	AR_ST_release		0x00000040
#define	AR_ST_stage		0x00000080
#define	AR_ST_access		0x00000100
#define	AR_ST_nftv		0x00000200
#define	AR_ST_after		0x00000400
#define	AR_ST_default_criteria  0x00004000

/* ar_set_copy_cfg_t and ar_general_copy_cfg_t flags */
#define	AR_CP_ar_age		0x00000001
#define	AR_CP_release		0X00000002
#define	AR_CP_norelease		0x00000004
#define	AR_CP_un_ar_age		0x00000008
#define	AR_CP_changed_mask	0x0000000f

#endif	/* _ARCHIVE_H */
