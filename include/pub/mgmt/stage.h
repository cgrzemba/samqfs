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
#ifndef _STAGE_H
#define	_STAGE_H

#pragma	ident	"$Revision: 1.30 $"

/*
 * stage.h - SAMFS APIs for staging operations.
 */


#include "sam/types.h"

#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/types.h"
#include "pub/mgmt/directives.h"

#include "aml/stager.h" /* for StagerStateDetail_t */


/*
 *	structs
 */


/* stager_cfg options flags */
#define	ST_LOG_START	0x00000001
#define	ST_LOG_ERROR	0x00000002
#define	ST_LOG_CANCEL	0x00000004
#define	ST_LOG_FINISH	0x00000008
#define	ST_LOG_ALL	0x00000010
#define	ST_DIRECTIO_ON  0x00000020
#define	ST_LOG_DEFAULTS 0x0000000e


typedef struct stream_cfg {
	mtype_t media;		/* dk for now */
	int	drives;		/* number of streams to use */
	fsize_t max_size;	/* max stream size */
	int	max_count;	/* max number of files per stream */
	uint32_t change_flag;
} stream_cfg_t;

/*
 * structure for the stager directives.
 */
typedef struct stager_cfg {
	upath_t		stage_log;
	int		max_active;
	int		max_retries;
	sqm_lst_t	*stage_buf_list;	/* list of buffer_directive_t */
	sqm_lst_t	*stage_drive_list;	/* list of drive_directive_t */
	uint32_t	change_flag;
	uint32_t	options;		/* Added 4.5 */
	stream_cfg_t	*dk_stream;
} stager_cfg_t;


/* change_flag masks for stager_cfg */
#define	ST_stage_log		0x00000001
#define	ST_max_active		0x00000002
#define	ST_max_retries		0x00000004
#define	ST_log_events		0x00000008	/* For all ST_LOG flags */
#define	ST_directio		0x00000010

/* Use StagerStateDetail_t instead of stager_detail_t */

/* Use StagerStateFlags_t instead of stager_state_t */

/*
 * The following structures will be removed once
 * include/aml/stager.h gets updated!
 */
typedef struct active_stager_info {
	StagerStateFlags_t	flags;
	mtype_t			    media;
	int				    eq;
	vsn_t			    vsn;
	umsg_t			    msg;
	StagerStateDetail_t	detail;
} active_stager_info_t;

typedef struct stager_stream {
	boolean_t	active;
	mtype_t		media;
	int			seqnum;
	vsn_t		vsn;
	size_t		count;		/* number of request files in stream */
	time_t		create;		/* time stream req was created */
	time_t		age;		/* elapsed time in seconds */
	umsg_t		msg;
	/* following are copy from active_stager_info */
	StagerStateDetail_t	*current_file; /* current file being staged */
	StagerStateFlags_t	state_flags; /* if active, these will be set */
} stager_stream_t;

typedef struct stager_info {
	upath_t		log_file;	/* log file name */
	umsg_t		msg;
	upath_t		streams_dir;	/* streams directory path */
	upath_t		stage_req;	/* stage requests path */
	sqm_lst_t	*active_stager_info; /* REDUNDANT, NOT REQUIRED */
	sqm_lst_t	*stager_streams;
} stager_info_t;


typedef struct staging_file_info {
	sam_id_t	id;	/* file identification */
	equ_t		fseq;	/* family set equipment number */
	int			copy;	/* stage from this archive copy */
	u_longlong_t	len;
	u_longlong_t	position;
	u_longlong_t	offset;
	mtype_t		media;
	vsn_t		vsn;
	upath_t		filename;
	pid_t		pid;		/* pid of requestor */
	uname_t		user;		/* name of requestor */
} staging_file_info_t;

/*
 *	functions
 */


/*
 * get the stager configuration.
 *	stager_cfg_t **stager_config -	returned stager_cfg_t data,
 *				it must be freed by caller.
 */
int get_stager_cfg(ctx_t *ctx, stager_cfg_t **stager_config);


/*
 * get the drive directives for a library.
 *	const uname_t lib_name -	library name (family set).
 *	drive_directive_t **stage_drive -	returned drive_directive_t data,
 *					it must be freed by caller.
 */
int get_drive_directive(ctx_t *ctx, const uname_t lib_name,
	drive_directive_t **stage_drive);


/*
 * get the buffer directives for a specific media type.
 *	const mtype_t media_type -	media type.
 *	buffer_directive_t **stage_buffer  returned buffer_directive_t data,
 *					it must be freed by caller.
 */
int get_buffer_directive(ctx_t *ctx, const mtype_t media_type,
	buffer_directive_t **stage_buffer);


/*
 * configure the entire staging information.
 *	const stager_cfg_t *stager_config -	structure stager_cfg_t data.
 */
int set_stager_cfg(ctx_t *ctx, const stager_cfg_t *stager_config);


/*
 * Functions to set a drive directive.
 *	drive_directive_t *stage_drive -	drive_directive need to be
 *						set in stager.cmd.
 */
int set_drive_directive(ctx_t *ctx, drive_directive_t *stage_drive);


/*
 * Functions to reset a drive directive.
 * Reset to default and it means that drive_directive will be
 * deleted it from configuration file.
 *
 *	drive_directive_t *stage_drive -  drive directive need to be reset.
 */
int reset_drive_directive(ctx_t *ctx, drive_directive_t *stage_drive);


/*
 * Functions to set a buffer directive.
 *	buffer_directive_t *stage_buffer -	buffer directive need to be set.
 */
int set_buffer_directive(ctx_t *ctx, buffer_directive_t *stage_buffer);


/*
 * Functions to reset a buffer directive.
 * Reset to default and it means that buffer_directive will be
 * deleted it from configuration file.
 *
 *	buffer_directive_t *stage_buffer -  buffer directive need to be reset.
 */
int reset_buffer_directive(ctx_t *ctx, buffer_directive_t *stage_buffer);


/*
 * Function to get the default values of stager. The list of drive
 * directives are for libraries that are under sam's control. The list
 * of buffer directives are for media types that are sam's control in
 * the sam server where this library is instantiated.
 *
 *	stager_cfg_t **stager_config -	default stager_cfg structure data,
 *					it must be freed by caller.
 */
int get_default_stager_cfg(ctx_t *ctx, stager_cfg_t **stager_config);


/*
 * get the default drive directive for staging for a library.
 *	uname_t lib_name -		library name.
 *	drive_directive_t **stage_drive -	default drive_directive_t data,
 *						it must be freed by caller.
 */
int get_default_staging_drive_directive(ctx_t *ctx, uname_t lib_name,
	drive_directive_t **stage_drive);


/*
 * get the default buffer directives for a specific media type
 *
 *	mtype_t media_type -		media type.
 *	buffer_directive_t **stage_buffer -	default buffer_directive_t data,
 *					it must be freed by caller.
 */
int get_default_staging_buffer_directive(ctx_t *ctx, mtype_t media_type,
	buffer_directive_t **stage_buffer);


/*
 * DESCRIPTION:
 *   get stager status information
 * PARAMS:
 *   ctx_t *		IN   - context object
 *   stager_info_t **	OUT  - stager_info_t structure
 * RETURNS:
 *    0 -- success
 *   -1 -- error
 *   -2 w/ empty struct -- stagerd not running
 */
int get_stager_info(ctx_t *ctx, stager_info_t **info);


/*
 * DESCRIPTION:
 *   get total number of staging files
 * PARAMS:
 *   ctx_t *		IN   - context object
 *   size_t *		OUT  - total number of staging files
 * RETURNS:
 *    0 -- success
 *   -1 -- error
 *   -2 -- stagerd not running
 */
int get_total_staging_files(ctx_t *ctx, size_t *total);


/*
 * DESCRIPTION:
 *   get number of staging files in the specified stream
 * PARAMS:
 *   ctx_t *		IN   - context object
 *   stager_stream_t *	IN   - stager stream
 *   size_t *		OUT  - number of staging files in stream
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int get_numof_staging_files_in_stream(ctx_t *ctx,
	stager_stream_t *stream, size_t *num);


/*
 * DESCRIPTION:
 *   get all staging files
 * PARAMS:
 *   ctx_t *		IN   - context object
 *   sqm_lst_t **	OUT  - a list of staging_file_info_t structures
 * RETURNS:
 *    0 -- success
 *   -1 -- error
 *   -2 w/ empty list -- stagerd not running
 */
int get_all_staging_files(ctx_t *ctx, sqm_lst_t **staging_file_infos);


/*
 * DESCRIPTION:
 *   get staging files accoarding to user specified start position
 *   and number of entries
 * PARAMS:
 *   ctx_t *		IN   - context object
 *   int		IN   - start position
 *   int		IN   - number of entries to return
 *   sqm_lst_t **	OUT  - a list of staging_file_info_t structures
 * RETURNS:
 *    0 -- success
 *   -1 -- error
 *   -2 w/ empty list -- stagerd not running
 */
int get_staging_files(ctx_t *ctx, int start, int size,
	sqm_lst_t **staging_file_infos);


typedef enum st_sort_key {
	ST_SORT_BY_FILENAME = 0,
	ST_SORT_BY_UID,
	ST_NO_SORT
} st_sort_key_t;


/*
 * DESCRIPTION:
 *   get all staging files assorted in the specified stream
 * PARAMS:
 *   ctx_t *		IN   - context object
 *   stager_stream_t *	IN   - stager stream
 *   st_sort_key_t	IN   - sort key
 *   boolean_t		IN   - ascending or descending order
 *   sqm_lst_t **	OUT  - a list of staging_file_info_t structures
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int get_all_staging_files_in_stream(ctx_t *ctx, stager_stream_t *stream,
	st_sort_key_t sort_key, boolean_t ascending,
	sqm_lst_t **staging_file_infos);


/*
 * DESCRIPTION:
 *   get staging files assorted in the specified stream according to
 *   user specified start position and number of entries
 * PARAMS:
 *   ctx_t *		IN   - context object
 *   stager_stream_t *	IN   - stager stream
 *   int		IN   - start position
 *   int		IN   - number of entries to return
 *   st_sort_key_t	IN   - sort key
 *   boolean_t		IN   - ascending or descending order
 *   sqm_lst_t **	OUT  - a list of staging_file_info_t structures
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int get_staging_files_in_stream(ctx_t *ctx, stager_stream_t *stream,
	int start, int size, st_sort_key_t sort_key, boolean_t ascending,
	sqm_lst_t **staging_file_infos);


/*
 * DESCRIPTION:
 *   find information about the given file in staging queue
 * PARAMS:
 *   ctx_t *		IN   - context object
 *   upath_t *		IN   - staging file name
 *   vsn_t		IN   - vsn
 *   staging_file_info_t ** OUT  - staging_file_info_t structure
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int find_staging_file(ctx_t *ctx, upath_t fname, vsn_t vsn,
	staging_file_info_t **finfo);


/*
 * DESCRIPTION:
 *   cancel pending requests for the named files,
 *   recursive only applies to directories.
 * PARAMS:
 *   ctx_t *		IN   - context object
 *   sqm_lst_t *	IN   - a list of files or directories
 *   boolean_t		IN   - recursive only applies to dirs
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int cancel_stage(ctx_t *ctx, const sqm_lst_t *file_or_dirs,
	const boolean_t recursive);


/*
 * DESCRIPTION:
 *   cancel stage requests associated with given media.vsn
 * PARAMS:
 *   ctx_t *		IN   - context object
 *   mtype_t *		IN   - media type
 *   vsn_t		IN   - vsn
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int clear_stage_request(ctx_t *ctx, mtype_t media, vsn_t vsn);


#define	ST_OPT_RESET_DEFAULTS	0x0001
#define	ST_OPT_NEVER		0x0002
#define	ST_OPT_ASSOCIATIVE	0x0004
#define	ST_OPT_PARTIAL		0x0008
#define	ST_OPT_RECURSIVE	0x0010
#define	ST_OPT_COPY_1		0x1000
#define	ST_OPT_COPY_2		0x2000
#define	ST_OPT_COPY_3		0x4000
#define	ST_OPT_COPY_4		0x8000

#define	ST_SS_DRIVES		0x0001
#define	ST_SS_MAX_SIZE		0x0002
#define	ST_SS_MAX_COUNT		0x0004

/*
 * API for staging files and setting stager attributes on files and directories
 * in 4.6 and beyond.
 *
 * Flags for setting options
 * ST_OPT_STAGE_NEVER
 * ST_OPT_ASSOCIATIVE_STAGE
 * ST_OPT_STAGE_DEFAULTS
 * ST_OPT_PARTIAL
 * ST_OPT_RECURSIVE
 *
 * Flags to be used in the options mask to indicate copies
 * ST_OPT_COPY_1
 * ST_OPT_COPY_2
 * ST_OPT_COPY_3
 * ST_OPT_COPY_4
 *
 * In order to signal that a file specified as stage -a be set to stage -n
 * the ST_OPT_STAGE_DEFAULTS flag must be set also.
 */
int
stage_files(ctx_t *c, sqm_lst_t *files, int32_t options, char **job_id);

/*
 * stage files
 * Deprecated API. Do not use except with pre 4.6 servers.
 *
 * Used to stage files and directories and to set stage attributes for files
 * and directories.
 *
 * The elements in the lists are matched by index. So the 5th copy entry
 * is used for the fifth filepath entry and the 5th options
 * entry is applied to the staging.
 *
 * In order to signal that a file specified as stage -a be set to stage -n
 * the ST_OPT_STAGE_DEFAULTS flag must be set also.
 *
 * Flags for setting options
 * ST_OPT_STAGE_NEVER
 * ST_OPT_ASSOCIATIVE_STAGE
 * ST_OPT_STAGE_DEFAULTS
 * ST_OPT_PARTIAL
 * ST_OPT_RECURSIVE
 *
 * PARAMS:
 *   sqm_lst_t * copies,	IN - list of ints indicating copy to stage
 *				from. 1-4 or 0 for system picks copy.
 *   sqm_lst_t * filepaths	IN - fully quallified file names
 *   sqm_lst_t * options	IN - bit fields indicating stage options
 *
 * RETURNS:
 *   success -  0	operation successfully issued staging not necessarily
 *			complete.
 *   error   -  -1
 */
int
stage_files_pre46(ctx_t *c, sqm_lst_t *copies, sqm_lst_t *filepaths,
    sqm_lst_t *options);


/*
 * DESCRIPTION:
 *   idle staging
 * PARAMS:
 *   ctx_t *		IN   - context object
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int st_idle(ctx_t *ctx);


/*
 * DESCRIPTION:
 *   start staging
 * PARAMS:
 *   ctx_t *		IN   - context object
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int st_run(ctx_t *ctx);


/*
 * Free methods.
 */
void free_stager_cfg(stager_cfg_t *cfg);
void free_stager_info(stager_info_t *info);
void free_stager_stream(stager_stream_t *stream);
void free_list_of_active_stager_info(sqm_lst_t *info);
void free_list_of_stager_stream(sqm_lst_t *streams);


#endif	/* _STAGE_H */
