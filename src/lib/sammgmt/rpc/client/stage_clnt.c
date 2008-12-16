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

#pragma ident	"$Revision: 1.23 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/sammgmt_rpc.h"
#include "pub/mgmt/sqm_list.h"

/*
 * stage_clnt.c
 *
 * RPC client wrapper for stage api
 */
static int
make_arg_lists_for_stage_files_pre46(int32_t options, sqm_lst_t **copy_list,
    sqm_lst_t **options_list);


/*
 * get the stager configuration.
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 * and  stager_config is NULL.
 */
int
get_stager_cfg(
ctx_t *ctx,			/* client connection */
stager_cfg_t **stager_config	/* return - stager config */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get stager cfg";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(stager_config)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_stager_cfg, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*stager_config = (stager_cfg_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	if ((*stager_config) != NULL) {
		SET_LIST_TAIL((*stager_config)->stage_buf_list);
		SET_LIST_TAIL((*stager_config)->stage_drive_list);
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get the drive directives for a library.
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 * and  stage_drive is NULL.
 */
int
get_drive_directive(
ctx_t *ctx,			/* client connection */
const uname_t lib_name,		/* library name */
drive_directive_t **stage_drive	/* return - drive directives */
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get drive directive";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(lib_name, stage_drive)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)lib_name;

	SAMRPC_CLNT_CALL(samrpc_get_drive_directive, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*stage_drive = (drive_directive_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get the buffer directives for a specific media type
 */
int
get_buffer_directive(
ctx_t *ctx,				/* client connection */
const mtype_t media_type,		/* media type */
buffer_directive_t **stage_buffer	/* return - stage buffer directives */
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get buffer directive";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(media_type, stage_buffer)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)media_type;

	SAMRPC_CLNT_CALL(samrpc_get_buffer_directive, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*stage_buffer = (buffer_directive_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * configure the entire staging information.
 */
int
set_stager_cfg(
ctx_t *ctx,				/* client connection */
const stager_cfg_t *stager_config	/* stager config */
)
{

	int ret_val;
	set_stager_cfg_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:set stager cfg";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(stager_config)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.stager_config = (stager_cfg_t *)stager_config;

	SAMRPC_CLNT_CALL(samrpc_set_stager_cfg, set_stager_cfg_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * Functions to set or reset a drive directive.
 */
int
set_drive_directive(
ctx_t *ctx,			/* client connection */
drive_directive_t *stage_drive	/* drive directive */
)
{

	int ret_val;
	drive_directive_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:set drive directive";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(stage_drive)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.stage_drive = stage_drive;

	SAMRPC_CLNT_CALL(samrpc_set_drive_directive, drive_directive_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


int
reset_drive_directive(
ctx_t *ctx,			/* client connection */
drive_directive_t *stage_drive	/* drive directive */
)
{

	int ret_val;
	drive_directive_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:reset drive directive";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(stage_drive)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.stage_drive = stage_drive;

	SAMRPC_CLNT_CALL(samrpc_reset_drive_directive, drive_directive_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * Functions to set or reset a buffer directive.
 */
int
set_buffer_directive(
ctx_t *ctx,				/* client connection */
buffer_directive_t *stage_buffer	/* stager buffer directive */
)
{

	int ret_val;
	buffer_directive_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:set buffer directive";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(stage_buffer)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.stage_buffer = stage_buffer;

	SAMRPC_CLNT_CALL(samrpc_set_buffer_directive, buffer_directive_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


int
reset_buffer_directive(
ctx_t *ctx,				/* client connection */
buffer_directive_t *stage_buffer	/* stager buffer directive */
)
{

	int ret_val;
	buffer_directive_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:reset buffer directive";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(stage_buffer)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.stage_buffer = stage_buffer;

	SAMRPC_CLNT_CALL(samrpc_reset_buffer_directive, buffer_directive_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * Function to get the default values of stager. The list of drive
 * directives are for libraries that are under sam's control. The list
 * of buffer directives are for media types that are sam's control in
 * the sam server where this library is instantiated.
 */
int
get_default_stager_cfg(
ctx_t *ctx,			/* client connection */
stager_cfg_t **stager_config	/* return - stager config */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get default stager cfg";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(stager_config)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_default_stager_cfg, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*stager_config = (stager_cfg_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	if ((*stager_config) != NULL) {
		SET_LIST_TAIL((*stager_config)->stage_buf_list);
		SET_LIST_TAIL((*stager_config)->stage_drive_list);
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get the default drive directive for staging for a library.
 */
int
get_default_staging_drive_directive(
ctx_t *ctx,			/* client connection */
uname_t lib_name,		/* library name */
drive_directive_t **stage_drive	/* staging drive directive */
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get default staging drive directive";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(lib_name, stage_drive)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)lib_name;

	SAMRPC_CLNT_CALL(samrpc_get_default_staging_drive_directive,
	    string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*stage_drive = (drive_directive_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get the default buffer directives for a specific media type
 */
int
get_default_staging_buffer_directive(
ctx_t *ctx,				/* client connection */
mtype_t media_type,			/* media type */
buffer_directive_t **stage_buffer	/* return - buffer directive */
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get default staging buffer directive";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(media_type, stage_buffer)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)media_type;

	SAMRPC_CLNT_CALL(samrpc_get_default_staging_buffer_directive,
	    string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*stage_buffer = (buffer_directive_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * staging Status information
 */
int
get_stager_info(
ctx_t *ctx,			/* client connection */
stager_info_t **info		/* return - stager status info */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get stager info";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(info)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_stager_info, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*info = (stager_info_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	if ((*info) != NULL) {
		SET_LIST_TAIL((*info)->active_stager_info);
		SET_LIST_TAIL((*info)->stager_streams);
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * staging files
 */
int
get_all_staging_files(
ctx_t *ctx,			/* client connection */
sqm_lst_t **staging_file_infos	/* return - list of staging files */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all staging files";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(staging_file_infos)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_all_staging_files, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*staging_file_infos = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*staging_file_infos));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get all staging files in stream
 */
int
get_all_staging_files_in_stream(
ctx_t *ctx,			/* client connection */
stager_stream_t *stream,	/* stager stream */
st_sort_key_t sort_key,		/* sort by filename/uid/no sorting */
boolean_t ascending,		/* sort in ascending/descending order */
sqm_lst_t **staging_file_infos	/* return - list of staging files */
)
{

	int ret_val;
	stager_stream_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all staging files in stream";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(stream, staging_file_infos)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.stream = stream;
	arg.sort_key = sort_key;
	arg.ascending = ascending;

	SAMRPC_CLNT_CALL(samrpc_get_all_staging_files_in_stream,
	    stager_stream_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*staging_file_infos = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*staging_file_infos));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get staging files in stream
 */
int
get_staging_files_in_stream(
ctx_t *ctx,			/* client connection */
stager_stream_t *stream,	/* stager stream */
int start,			/* starting point */
int size,			/* number of entries */
st_sort_key_t sort_key,		/* sort by filename/uid/no sorting */
boolean_t ascending,		/* sort in ascending/descending order */
sqm_lst_t **staging_file_infos	/* return - list of staging files */
)
{

	int ret_val;
	stager_stream_range_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get staging files in stream";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(stream, staging_file_infos)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.stream = stream;
	arg.start = start;
	arg.size = size;
	arg.sort_key = sort_key;
	arg.ascending = ascending;

	SAMRPC_CLNT_CALL(samrpc_get_staging_files_in_stream,
	    stager_stream_range_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*staging_file_infos = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*staging_file_infos));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * cancel pending requests for the named files.
 * recursive only applies to directories.
 */
int
cancel_stage(
ctx_t *ctx,			/* client connection */
const sqm_lst_t *file_or_dirs,	/* list of file or directories */
const boolean_t recursive	/* cancel recursively */
)
{

	int ret_val;
	stage_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:cancel stage";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(file_or_dirs)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.file_or_dirs = (sqm_lst_t *)file_or_dirs;
	arg.recursive = recursive;

	SAMRPC_CLNT_CALL(samrpc_cancel_stage, stage_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 */
int
clear_stage_request(
ctx_t *ctx,			/* client connection */
mtype_t media,
vsn_t vsn
)
{

	int ret_val;
	clear_stage_request_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:clear stage request";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(media, vsn)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	strcpy(arg.media, media);
	strcpy(arg.vsn, vsn);

	SAMRPC_CLNT_CALL(samrpc_clear_stage_request, clear_stage_request_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * idle staging
 */
int
st_idle(
ctx_t *ctx	/* client connection */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:st idle";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_st_idle, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * start staging
 */
int
st_run(
ctx_t *ctx	/* client connection */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:st run";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_st_run, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_staging_files
 * user can specify the starting point and number of entries
 */
int
get_staging_files(
ctx_t *ctx,	/* client connection */
int start,	/* start entry */
int size,	/* number of entries */
sqm_lst_t **staging_file_infos	/* return - list of staging_file_infos */
)
{

	int ret_val;
	range_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get staging files";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.start = start;
	arg.size = size;

	SAMRPC_CLNT_CALL(samrpc_get_staging_files, range_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*staging_file_infos = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*staging_file_infos));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * find_staging_file
 * find information about the given file in staging queue
 */
int
find_staging_file(
ctx_t *ctx,	/* client connection */
upath_t fname,	/* file name (absolute path) */
vsn_t vsn,	/* vsn to search for (NULL means search all) */
staging_file_info_t **finfo	/* return - staging_file_info_t */
)
{

	int ret_val;
	staging_file_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:find staging file";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	if (ISNULL(fname)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}
	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	strcpy(arg.fname, fname);
	arg.vsn = (char *)vsn;

	SAMRPC_CLNT_CALL(samrpc_find_staging_file, staging_file_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*finfo = (staging_file_info_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get total staging files
 */
int
get_total_staging_files(
ctx_t *ctx,	/* client connection */
size_t *total	/* return - number of staging files */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get total staging files";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_total_staging_files, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*total = *((size_t *)result.samrpc_result_u.result.result_data);

	free(result.samrpc_result_u.result.result_data);

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get number of staging files in stream
 */
int
get_numof_staging_files_in_stream(
ctx_t *ctx,	/* client connection */
stager_stream_t *stream,	/* stager stream */
size_t *num	/* return - number of staging files */
)
{

	int ret_val;
	stream_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get number of staging files in stream";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.stream = stream;

	SAMRPC_CLNT_CALL(samrpc_get_numof_staging_files_in_stream,
	    stream_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*num = *((size_t *)result.samrpc_result_u.result.result_data);

	free(result.samrpc_result_u.result.result_data);

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * stage_files supports both the 4.6 and pre 4.6 APIs.
 */
int
stage_files(
ctx_t *ctx,		/* client connection */
sqm_lst_t *filepaths,	/* input - list of strings */
int32_t options,
char **job_id)
{
	int ret_val;
	strlst_int32_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:stage files";
	char *err_msg;
	enum clnt_stat stat;
	char *srv_vers;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	srv_vers = get_server_version_from_ctx(ctx);
	if (srv_vers == NULL) {
		setsamerr(SE_STAGE_FAILED_IN_RPC_CLIENT);
		PTRACE(1, "Server version is null in stage files");
		return (-1);
	}

	/* build args and call old function if server is from pre 4.6 */
	if (strcmp(srv_vers, "1.5.7") <= 0) {
		sqm_lst_t *copy_list = NULL;
		sqm_lst_t *options_list = NULL;

		PTRACE(3, "stage_files pre46 branch rpc");
		/*
		 * Make lists for the inputs to match the
		 * old API.
		 */
		if (make_arg_lists_for_stage_files_pre46(options,
		    &copy_list, &options_list) != 0) {
			setsamerr(SE_STAGE_FAILED_IN_RPC_CLIENT);
			PTRACE(1, "making args for stage pre46 failed");
			return (-1);
		}

		ret_val = stage_files_pre46(ctx, copy_list, filepaths,
		    options_list);

		/* free the args created to call the old function */
		lst_free_deep(copy_list);
		lst_free_deep(options_list);


		PTRACE(2, "%s returning with status [%d]...", func_name,
		    ret_val);
		PTRACE(2, "%s exit", func_name);

		/* pre 4.6 no stage files job exists so set id to null */
		*job_id = NULL;

		return (ret_val);
	} else {
		memset((char *)&result, 0, sizeof (result));
		arg.ctx = ctx;
		arg.int32 = options;
		arg.strlst = filepaths;
		SAMRPC_CLNT_CALL(samrpc_stage_files, strlst_int32_arg_t);
	}

	CHECK_FUNCTION_FAILURE(result, func_name);

	*job_id = (char *)result.samrpc_result_u.result.result_data;


	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * stage_files supports both the 4.6 and pre 4.6 APIs.
 */
int
stage_files_pre46(
ctx_t *ctx,
sqm_lst_t *copy_list,
sqm_lst_t *filepaths,
sqm_lst_t *options_list) {
	int ret_val;
	strlst_intlst_intlst_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:stage files pre 46";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	/*
	 * Make lists for the inputs to match the
	 * old API.
	 */
	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.intlst1 = copy_list;
	arg.intlst2 = options_list;
	arg.strlst = filepaths;

	SAMRPC_CLNT_CALL(samrpc_stage_files_pre46,
	    strlst_intlst_intlst_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * This function converts between the 4.6 style stage_files arguments
 * and the pre 4.6 style stage_files arguments.
 * If a copy is not specified *copy_list will be set to NULL.
 * If no options other than a copy are specified *option_list will be
 * set to NULL.
 */
static int
make_arg_lists_for_stage_files_pre46(int32_t options, sqm_lst_t **copy_list,
    sqm_lst_t **options_list) {

	int32_t *dup;

	PTRACE(1, "making args");
	/* If a copy is specified create the list and insert it */
	*copy_list = lst_create();
	PTRACE(1, "created first list");
	if ((options & 0xF000) != 0) {
		PTRACE(1, "options indicate a copy");
		if (*options_list == NULL) {
			return (-1);
		}
		dup = mallocer(sizeof (int32_t));
		if (dup == NULL) {
			return (-1);
		}
		if (options & 0x1000) {
			*dup = 1;
		} else if (options & 0x2000) {
			*dup = 1;
		} else if (options & 0x4000) {
			*dup = 1;
		} else if (options & 0x8000) {
			*dup = 1;
		}
		PTRACE(1, "options indicate a copy %d", *dup);
		if (lst_append(*copy_list, dup) != 0) {
			return (-1);
		}
	}

	/* If any options are set create the list and insert them */
	*options_list = lst_create();
	PTRACE(1, "created 2nd list");
	if (options & 0x0FFF) {
		PTRACE(1, "options indicate options are set %d", options);
		if (*options_list == NULL) {
			return (-1);
		}
		dup = mallocer(sizeof (int32_t));
		if (dup == NULL) {
			return (-1);
		}
		/* exclude the copy number portion */
		*dup = (options & 0x0FFF);
		if (lst_append(*options_list, dup) != 0) {
			return (-1);
		}
	}
	PTRACE(1, "Leaving make args");
	return (0);
}
