
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

#pragma ident	"$Revision: 1.19 $"

#include "mgmt/sammgmt.h"
#include <stdlib.h>

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * stage_svr.c
 *
 * RPC server wrapper for stage api
 */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;

samrpc_result_t *
samrpc_get_stager_cfg_1_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	stager_cfg_t *stager_config;

	Trace(TR_DEBUG, "Get stager configuration");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to to get stager configuration");
	ret = get_stager_cfg(arg->ctx, &stager_config);

	SAMRPC_SET_RESULT(ret, SAM_ST_STAGER_CFG, stager_config);

	Trace(TR_DEBUG, "Get stager configuration return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_drive_directive_1_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	drive_directive_t *stage_drive;

	Trace(TR_DEBUG, "Get stager drive directive");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get stager drive directive");
	ret = get_drive_directive(arg->ctx, arg->str, &stage_drive);

	SAMRPC_SET_RESULT(ret, SAM_ST_DRIVE_DIRECTIVE, stage_drive);

	Trace(TR_DEBUG, "Get stager drive directive return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_buffer_directive_1_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	buffer_directive_t *stage_buffer;

	Trace(TR_DEBUG, "Get stager buffer directive");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get stager buffer directive");
	ret = get_buffer_directive(arg->ctx, arg->str, &stage_buffer);

	SAMRPC_SET_RESULT(ret, SAM_ST_BUFFER_DIRECTIVE, stage_buffer);

	Trace(TR_DEBUG, "Get stager buffer directive return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_set_stager_cfg_1_svr(
	set_stager_cfg_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Set stager configuration");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);
	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	if ((arg != NULL) && (arg->stager_config != NULL)) {
		SET_LIST_TAIL(arg->stager_config->stage_buf_list);
		SET_LIST_TAIL(arg->stager_config->stage_drive_list);
	}

	Trace(TR_DEBUG, "Call native lib to set stager configuration");
	ret = set_stager_cfg(arg->ctx, arg->stager_config);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the client resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Set stager configuration return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_set_drive_directive_1_svr(
	drive_directive_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Set stager drive directive");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to set stager drive directive");
	ret = set_drive_directive(arg->ctx, arg->stage_drive);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the client resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Set stager drive directive return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_reset_drive_directive_1_svr(
	drive_directive_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Reset stager drive directive");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to reset stager drive directive");
	ret = reset_drive_directive(arg->ctx, arg->stage_drive);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the client resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Reset stager drive directive return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_set_buffer_directive_1_svr(
	buffer_directive_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Set stager buffer drive directive");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to set stager buffer directive");
	ret = set_buffer_directive(arg->ctx, arg->stage_buffer);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the client resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Set stager buffer directive return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_reset_buffer_directive_1_svr(
	buffer_directive_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Reset stager buffer directive");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to reset stager buffer directive");
	ret = reset_buffer_directive(arg->ctx, arg->stage_buffer);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the client resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Reset stager buffer directive return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_default_stager_cfg_1_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	stager_cfg_t *stager_config;

	Trace(TR_DEBUG, "Get default stager configuration");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get default stager configuration");
	ret = get_default_stager_cfg(arg->ctx, &stager_config);

	SAMRPC_SET_RESULT(ret, SAM_ST_STAGER_CFG, stager_config);

	Trace(TR_DEBUG, "Get default stager configuration return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_default_staging_drive_directive_1_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	drive_directive_t *stage_drive;

	Trace(TR_DEBUG, "Get default stage drive directive");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get default stage drive directive");
	ret = get_default_staging_drive_directive(
	    arg->ctx,
	    arg->str,
	    &stage_drive);

	SAMRPC_SET_RESULT(ret, SAM_ST_DRIVE_DIRECTIVE, stage_drive);

	Trace(TR_DEBUG, "Get default stage drive directive return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_default_staging_buffer_directive_1_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	buffer_directive_t *stage_buffer;

	Trace(TR_DEBUG, "Get default stage buffer directive");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib get default stage buffer directive");
	ret = get_default_staging_buffer_directive(
	    arg->ctx,
	    arg->str,
	    &stage_buffer);

	SAMRPC_SET_RESULT(ret, SAM_ST_BUFFER_DIRECTIVE, stage_buffer);

	Trace(TR_DEBUG, "Get default stage buffer directive return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_stager_info_1_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	stager_info_t *info = NULL;

	Trace(TR_DEBUG, "Get stager info");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get stager info");
	ret = get_stager_info(arg->ctx, &info);

	SAMRPC_SET_RESULT(ret, SAM_ST_STAGER_INFO, info);

	Trace(TR_DEBUG, "Get stager info return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_all_staging_files_1_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *infos;

	Trace(TR_DEBUG, "Get all staging files");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get all staging files");
	ret = get_all_staging_files(arg->ctx, &infos);

	SAMRPC_SET_RESULT(ret, SAM_ST_STAGING_FILE_INFO_LIST, infos);

	Trace(TR_DEBUG, "Get all staging files return[%d] with [%d] files",
	    ret, (ret == -1) ? ret : infos->length);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_all_staging_files_in_stream_1_svr(
	stager_stream_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get all staging files in stream");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get all staging files in stream");
	ret = get_all_staging_files_in_stream(
	    arg->ctx,
	    arg->stream,
	    arg->sort_key,
	    arg->ascending,
	    &lst);

	SAMRPC_SET_RESULT(ret, SAM_ST_STAGING_FILE_INFO_LIST, lst);

	Trace(TR_DEBUG, "Get all staging files in stream return[%d] [%d] files",
	    ret, (ret == -1) ? ret : lst->length);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_staging_files_in_stream_1_svr(
	stager_stream_range_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get staging files in stream");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get staging files in stream");
	ret = get_staging_files_in_stream(
	    arg->ctx,
	    arg->stream,
	    arg->start,
	    arg->size,
	    arg->sort_key,
	    arg->ascending,
	    &lst);

	SAMRPC_SET_RESULT(ret, SAM_ST_STAGING_FILE_INFO_LIST, lst);

	Trace(TR_DEBUG, "Get staging files in stream return[%d] [%d] files",
	    ret, (ret == -1) ? ret : lst->length);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_cancel_stage_1_svr(
	stage_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Cancel stage");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to cancel stage");
	ret = cancel_stage(arg->ctx, arg->file_or_dirs, arg->recursive);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Cancel stage return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_clear_stage_request_1_svr(
	clear_stage_request_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Clear stage request");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to clear stage request");
	ret = clear_stage_request(arg->ctx, arg->media, arg->vsn);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Clear stage request return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_st_idle_1_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Idle stager");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to idle stager");
	ret = st_idle(arg->ctx);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Idle stager return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_st_run_1_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Run stager");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to run stager");
	ret = st_run(arg->ctx);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Run stager return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_staging_files_1_svr(
	range_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *infos;

	Trace(TR_DEBUG, "Get staging files");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get staging files");
	ret = get_staging_files(arg->ctx, arg->start, arg->size, &infos);

	SAMRPC_SET_RESULT(ret, SAM_ST_STAGING_FILE_INFO_LIST, infos);

	Trace(TR_DEBUG, "Get staging files return[%d] with [%d] files",
	    ret, (ret == -1) ? ret : infos->length);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_find_staging_file_1_svr(
	staging_file_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	staging_file_info_t *info;

	Trace(TR_DEBUG, "Find staging file");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to find staging file");
	ret = find_staging_file(arg->ctx, arg->fname, arg->vsn, &info);

	SAMRPC_SET_RESULT(ret, SAM_ST_STAGING_FILE_INFO, info);

	Trace(TR_DEBUG, "Find staging file return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_total_staging_files_1_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	size_t *total = NULL;

	Trace(TR_DEBUG, "Get total staging files");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	total = (size_t *)mallocer(sizeof (size_t));
	if (total != NULL) {

		Trace(TR_DEBUG, "Call native lib to get total staging files");
		ret = get_total_staging_files(arg->ctx, total);
	}
	SAMRPC_SET_RESULT(ret, SAM_PTR_SIZE, total);

	Trace(TR_DEBUG, "Get total staging files return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_numof_staging_files_in_stream_1_svr(
	stream_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	size_t *num = NULL;

	Trace(TR_DEBUG, "Get number of staging files in stream");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	num = (size_t *)mallocer(sizeof (size_t));
	if (num != NULL) {

		Trace(TR_DEBUG, "Call native lib to get numof staging files");
		ret = get_numof_staging_files_in_stream(
		    arg->ctx, arg->stream, num);
	}
	SAMRPC_SET_RESULT(ret, SAM_PTR_SIZE, num);

	Trace(TR_DEBUG, "Get num staging files return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_stage_files_5_svr(
	strlst_intlst_intlst_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_MISC, "Stage files");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to stage files");
	ret = stage_files_pre46(
	    arg->ctx, arg->intlst1, arg->strlst, arg->intlst2);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_MISC, "Stage files return [%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_stage_files_6_svr(
	strlst_int32_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	char *job_id;
	Trace(TR_MISC, "Stage files");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to stage files");
	ret = stage_files(arg->ctx, arg->strlst, arg->int32, &job_id);

	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, job_id);

	Trace(TR_MISC, "Stage files return [%d] %s", ret, Str(job_id));
	return (&rpc_result);
}
