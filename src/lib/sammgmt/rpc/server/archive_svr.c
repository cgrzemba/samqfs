
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

#pragma ident	"$Revision: 1.20 $"

#include "mgmt/sammgmt.h"
#include <stdlib.h>

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * archive_svr.c
 *
 * RPC server wrapper for archive api
 */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;

/*
 * *************
 *  config APIs
 * *************
 */

samrpc_result_t *
samrpc_get_default_ar_set_copy_cfg_1_svr(
ctx_arg_t *arg,		/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	ar_set_copy_cfg_t *copy_cfg;

	Trace(TR_DEBUG, "Get default archive set copy configuration");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib get default archive set copy cfg");
	ret = get_default_ar_set_copy_cfg(arg->ctx, &copy_cfg);

	SAMRPC_SET_RESULT(ret, SAM_AR_SET_COPY_CFG, copy_cfg);

	Trace(TR_DEBUG, "Get default archive set copy configuration return[%d]",
	    ret);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_default_ar_set_criteria_1_svr(
ctx_arg_t *arg,		/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	ar_set_criteria_t *criteria;

	Trace(TR_DEBUG, "Get default archive set criteria");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get default archive set criteria");
	ret = get_default_ar_set_criteria(arg->ctx, &criteria);

	SAMRPC_SET_RESULT(ret, SAM_AR_SET_CRITERIA, criteria);

	Trace(TR_DEBUG, "Get default archive set criteria return[%d]", ret);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_ar_global_directive_1_svr(
ctx_arg_t *arg,		/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	ar_global_directive_t *ar_global;

	Trace(TR_DEBUG, "Get archiver global directives");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get archiver global directive");
	ret = get_ar_global_directive(arg->ctx, &ar_global);

	SAMRPC_SET_RESULT(ret, SAM_AR_GLOBAL_DIRECTIVE, ar_global);

	Trace(TR_DEBUG, "Get archiver global directive return[%d]", ret);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_set_ar_global_directive_1_svr(
ar_global_directive_arg_t *arg,		/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Set archiver global directive");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	/*
	 * The tail of the list is not passed via XDR
	 * Set the tail of the lists (ar_bufs, ar_max, ar_drives,
	 * ar_set_lst and ar_overflow_lst)
	 */
	if ((arg != NULL) && (arg->ar_global != NULL)) {
		SET_LIST_TAIL(arg->ar_global->ar_bufs);
		SET_LIST_TAIL(arg->ar_global->ar_max);
		SET_LIST_TAIL(arg->ar_global->ar_drives);
		SET_LIST_TAIL(arg->ar_global->ar_set_lst);
		SET_LIST_TAIL(arg->ar_global->ar_overflow_lst);
	}

	Trace(TR_DEBUG, "Call native lib to set archiver global directive");
	ret = set_ar_global_directive(arg->ctx, arg->ar_global);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Set archiver global directive return[%d]", ret);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_default_ar_global_directive_1_svr(
ctx_arg_t *arg,		/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	ar_global_directive_t *ar_global;

	Trace(TR_DEBUG, "Get default archiver global directive");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get default global directive");
	ret = get_default_ar_global_directive(arg->ctx, &ar_global);

	SAMRPC_SET_RESULT(ret, SAM_AR_GLOBAL_DIRECTIVE, ar_global);

	Trace(TR_DEBUG, "Get default archiver global directive return[%d]",
	    ret);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_ar_set_criteria_names_1_svr(
ctx_arg_t *arg,		/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get archive set names");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get archive set names");
	ret = get_ar_set_criteria_names(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_AR_NAMES_LIST, lst);

	Trace(TR_DEBUG, "Get archive set names return[%d]", ret);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_ar_set_1_svr(
string_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get archive set[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get archive set");
	ret = get_ar_set(arg->ctx, arg->str, &lst);

	SAMRPC_SET_RESULT(ret, SAM_AR_SET_CRITERIA_LIST, lst);

	Trace(TR_DEBUG, "Get archive set return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_ar_set_criteria_list_1_svr(
string_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get archive set criteria for fs[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get archive set criteria");
	ret = get_ar_set_criteria_list(arg->ctx, arg->str, &lst);

	SAMRPC_SET_RESULT(ret, SAM_AR_SET_CRITERIA_LIST, lst);

	Trace(TR_DEBUG, "Get archive set crit for fs[%s] return[%d] [%d] lists",
	    arg->str, ret, (ret == -1) ? ret : lst->length);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_all_ar_set_criteria_1_svr(
ctx_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get all archive set criteria");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get all archive set criteria");
	ret = get_all_ar_set_criteria(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_AR_SET_CRITERIA_LIST, lst);

	Trace(TR_DEBUG, "Get all archive set criteria return[%d] [%d] lists",
	    ret, (ret == -1) ? ret : lst->length);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_ar_fs_directive_1_svr(
string_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	ar_fs_directive_t *fs_directive;

	Trace(TR_DEBUG, "Get file system directives for archiving");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get file system directives");
	ret = get_ar_fs_directive(arg->ctx, arg->str, &fs_directive);

	SAMRPC_SET_RESULT(ret, SAM_AR_FS_DIRECTIVE, fs_directive);

	Trace(TR_DEBUG, "Get file system directives for archiving return[%d]",
	    ret);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_all_ar_fs_directives_1_svr(
ctx_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get all file systems directives");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get all file systems directives");
	ret = get_all_ar_fs_directives(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_AR_FS_DIRECTIVE_LIST, lst);

	Trace(TR_DEBUG, "Get all file systems directives return[%d]", ret);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_set_ar_fs_directive_1_svr(
ar_fs_directive_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Set file system directive");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	/*
	 * The tail of the list is not passed via XDR
	 * Set the tail of the ar_set_criteria list
	 */
	if ((arg != NULL) && (arg->fs_directive != NULL)) {
		SET_LIST_TAIL(arg->fs_directive->ar_set_criteria);
	}

	Trace(TR_DEBUG, "Call native lib to set file system directive");
	ret = set_ar_fs_directive(arg->ctx, arg->fs_directive);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);


	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Set file system directive return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_reset_ar_fs_directive_1_svr(
string_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Reset archive file system directive");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to reset file system directive");
	ret = reset_ar_fs_directive(arg->ctx, arg->str);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Reset archive file system directive return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_modify_ar_set_criteria_1_svr(
ar_set_criteria_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Modify archive set criteria");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to modify archive set criteria");
	ret = modify_ar_set_criteria(arg->ctx, arg->crit);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Modify archive set criteria return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_default_ar_fs_directive_1_svr(
ctx_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	ar_fs_directive_t *fs_directive;

	Trace(TR_DEBUG, "Get default file system directive");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get default file system directive");
	ret = get_default_ar_fs_directive(arg->ctx, &fs_directive);

	SAMRPC_SET_RESULT(ret, SAM_AR_FS_DIRECTIVE, fs_directive);

	Trace(TR_DEBUG, "Get default file system directive return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_all_ar_set_copy_params_1_svr(
ctx_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get all archive set copy parameters");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get all archive set copy params");
	ret = get_all_ar_set_copy_params(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_AR_SET_COPY_PARAMS_LIST, lst);

	Trace(TR_DEBUG, "Get all archive set copy parameters return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_ar_set_copy_params_names_1_svr(
ctx_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *ar_set_copy_names;

	Trace(TR_DEBUG, "Get archive set parameter names");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get archive set parameter names");
	ret = get_ar_set_copy_params_names(arg->ctx, &ar_set_copy_names);

	SAMRPC_SET_RESULT(ret, SAM_AR_NAMES_LIST, ar_set_copy_names);

	Trace(TR_DEBUG, "Get archive set parameter names return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_ar_set_copy_params_1_svr(
string_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	ar_set_copy_params_t *copy_parameters;

	Trace(TR_DEBUG, "Get archive set copy parameters");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get archive set copy parameters");
	ret = get_ar_set_copy_params(arg->ctx, arg->str, &copy_parameters);

	SAMRPC_SET_RESULT(ret, SAM_AR_SET_COPY_PARAMS, copy_parameters);

	Trace(TR_DEBUG, "Get archive set copy parameters return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_ar_set_copy_params_for_ar_set_1_svr(
string_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get archive set copy parameters for set[%s]",
	    arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get archive set copy parameters");
	ret = get_ar_set_copy_params_for_ar_set(arg->ctx, arg->str, &lst);

	SAMRPC_SET_RESULT(ret, SAM_AR_SET_COPY_PARAMS_LIST, lst);

	Trace(TR_DEBUG, "Get archive set copy parameters return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_set_ar_set_copy_params_1_svr(
ar_set_copy_params_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Set archive set copy parameters");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	/*
	 * The tail of the list is not passed via XDR
	 * Set the tail of the priority_t list
	 */
	if ((arg != NULL) && (arg->copy_params != NULL)) {
		SET_LIST_TAIL(arg->copy_params->priority_lst);
	}

	Trace(TR_DEBUG, "Call native lib to set archive set copy parameters");
	ret = set_ar_set_copy_params(arg->ctx, arg->copy_params);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Set archive set copy parameters return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_reset_ar_set_copy_params_1_svr(
string_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Reset archive set copy parameters");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to reset archive set copy parameters");
	ret = reset_ar_set_copy_params(arg->ctx, arg->str);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Reset archive set copy parameters return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_default_ar_set_copy_params_1_svr(
ctx_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	ar_set_copy_params_t *parameters;

	Trace(TR_DEBUG, "Get default archive set copy parameters");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib get default archive set copy params");
	ret = get_default_ar_set_copy_params(arg->ctx, &parameters);

	SAMRPC_SET_RESULT(ret, SAM_AR_SET_COPY_PARAMS, parameters);

	Trace(TR_DEBUG, "Get default archive set copy parameters return[%d]",
	    ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_all_vsn_pools_1_svr(
ctx_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get all vsn pools");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get all vsn pools");
	ret = get_all_vsn_pools(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_AR_VSN_POOL_LIST, lst);

	Trace(TR_DEBUG, "Get all vsn pools return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_vsn_pool_1_svr(
string_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	vsn_pool_t *vsn_pool;

	Trace(TR_DEBUG, "Get vsn pool[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get vsn pool[%s]", arg->str);
	ret = get_vsn_pool(arg->ctx, arg->str, &vsn_pool);

	SAMRPC_SET_RESULT(ret, SAM_AR_VSN_POOL, vsn_pool);

	Trace(TR_DEBUG, "Get vsn pool[%s] return[%d]", arg->str, ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_add_vsn_pool_1_svr(
vsn_pool_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Add vsn pool");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	/*
	 * The tail of the list is not passed via XDR
	 * Set the tail of list of vsn_names
	 */
	if ((arg != NULL) && (arg->pool != NULL)) {
		SET_LIST_TAIL(arg->pool->vsn_names);
	}

	Trace(TR_DEBUG, "Call native lib to add vsn pool");
	ret = add_vsn_pool(arg->ctx, arg->pool);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Add vsn pool return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_modify_vsn_pool_1_svr(
vsn_pool_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Modify vsn pool");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	/*
	 * The tail of the list is not passed via XDR
	 * Set the tail of list of vsn_names
	 */
	if ((arg != NULL) && (arg->pool != NULL)) {
		SET_LIST_TAIL(arg->pool->vsn_names);
	}

	Trace(TR_DEBUG, "Call native lib to modify vsn pool");
	ret = modify_vsn_pool(arg->ctx, arg->pool);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Modify vsn pool return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_remove_vsn_pool_1_svr(
string_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Remove vsn pool");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to remove vsn pool");
	ret = remove_vsn_pool(arg->ctx, arg->str);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Remove vsn pool return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_all_vsn_copy_maps_1_svr(
ctx_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get all vsn copy maps");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get all vsn copy maps");
	ret = get_all_vsn_copy_maps(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_AR_VSN_MAP_LIST, lst);

	Trace(TR_DEBUG, "Get all vsn copy maps return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_vsn_copy_map_1_svr(
string_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	vsn_map_t *vsn_map;

	Trace(TR_DEBUG, "Get vsn copy map");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get vsn copy map");
	ret = get_vsn_copy_map(arg->ctx, arg->str, &vsn_map);

	SAMRPC_SET_RESULT(ret, SAM_AR_VSN_MAP, vsn_map);

	Trace(TR_DEBUG, "Get vsn copy map return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_add_vsn_copy_map_1_svr(
vsn_map_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Add vsn copy map");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	/*
	 * The tail of the list is not passed via XDR
	 * Set the tail of list of vsn_names
	 */
	if ((arg != NULL) && (arg->map != NULL)) {
		SET_LIST_TAIL(arg->map->vsn_names);
		SET_LIST_TAIL(arg->map->vsn_pool_names);
	}

	Trace(TR_DEBUG, "Call native lib to add vsn copy map");
	ret = add_vsn_copy_map(arg->ctx, arg->map);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Add vsn copy map return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_modify_vsn_copy_map_1_svr(
vsn_map_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Modify vsn copy map");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	/*
	 * The tail of the list is not passed via XDR
	 * Set the tail of list of vsn_names and vsn_pool_names
	 */
	if ((arg != NULL) && (arg->map != NULL)) {
		SET_LIST_TAIL(arg->map->vsn_names);
		SET_LIST_TAIL(arg->map->vsn_pool_names);
	}

	Trace(TR_DEBUG, "Call native lib to modify vsn copy map");
	ret = modify_vsn_copy_map(arg->ctx, arg->map);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Modify vsn copy map return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_remove_vsn_copy_map_1_svr(
string_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Remove vsn copy map");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to remove vsn copy map");
	ret = remove_vsn_copy_map(arg->ctx, arg->str);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Remove vsn copy map return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_default_ar_drive_directive_1_svr(
string_bool_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	drive_directive_t *drive;

	Trace(TR_DEBUG, "Get default archive drive directive");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib get default archive drive directive");
	ret = get_default_ar_drive_directive(arg->ctx, arg->str, arg->bool,
	    &drive);

	SAMRPC_SET_RESULT(ret, SAM_AR_DRIVE_DIRECTIVE, drive);

	Trace(TR_DEBUG, "Get default archive drive directive return[%d]", ret);
	return (&rpc_result);
}


/*
 * **************
 *  control APIs
 * **************
 */

samrpc_result_t *
samrpc_ar_run_1_svr(
string_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Run archiver on fs[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to run archiver");
	ret = ar_run(arg->ctx, arg->str);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Run archiver return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_ar_run_all_1_svr(
ctx_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Run archiver on all fs");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to run archiver on all fs");
	ret = ar_run_all(arg->ctx);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Run archiver on all fs return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_ar_stop_1_svr(
string_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Stop archiver on fs[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to stop archiver");
	ret = ar_stop(arg->ctx, arg->str);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Stop archiver return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_ar_stop_all_1_svr(
ctx_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Stop all archiving");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to stop all archiving");
	ret = ar_stop_all(arg->ctx);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Stop all archiving return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_ar_idle_1_svr(
string_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Idle archiver");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to idle archiver");
	ret = ar_idle(arg->ctx, arg->str);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Idle archiver return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_ar_idle_all_1_svr(
ctx_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Idle archiving on all fs");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to idle archiving on all fs");
	ret = ar_idle_all(arg->ctx);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Idle archiving on all fs return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_ar_restart_all_1_svr(
ctx_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Restart archiving");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to restart archiving");
	ret = ar_restart_all(arg->ctx);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Restart archiving return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_archiverd_state_1_svr(
ctx_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	struct ArchiverdState *status;

	Trace(TR_DEBUG, "Get sam-archiverd state");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get state of sam-archiverd");
	ret = get_archiverd_state(arg->ctx, &status);

	SAMRPC_SET_RESULT(ret, SAM_AR_ARCHIVERD_STATE, status);

	Trace(TR_DEBUG, "Get sam-archiverd state return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_all_arfind_state_1_svr(
ctx_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get all arfind state");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get all arfind state");
	ret = get_all_arfind_state(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_AR_ARFIND_STATE_LIST, lst);

	Trace(TR_DEBUG, "Get all arfind state return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_arfind_state_1_svr(
string_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{


	int ret = -1;
	struct ar_find_state *status;

	Trace(TR_DEBUG, "Get arfind state");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get arfind state");
	ret = get_arfind_state(arg->ctx, arg->str, &status);

	SAMRPC_SET_RESULT(ret, SAM_AR_ARFIND_STATE, status);

	Trace(TR_DEBUG, "Get arfind state return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_archreqs_1_svr(
string_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get archive requests");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get archive requests");
	ret = get_archreqs(arg->ctx, arg->str, &lst);

	SAMRPC_SET_RESULT(ret, SAM_AR_ARCHREQ_LIST, lst);

	Trace(TR_DEBUG, "Get archive requests return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_all_archreqs_1_svr(
ctx_arg_t *arg,		/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get all archive requests");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get all archive requests");
	ret = get_all_archreqs(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_AR_ARCHREQ_LIST, lst);

	Trace(TR_DEBUG, "Get all archive requests return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_activate_archiver_cfg_1_svr(
ctx_arg_t *arg,		/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Activate archiver config");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to activate archiver config");
	ret = activate_archiver_cfg(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_AR_NAMES_LIST, lst);

	Trace(TR_DEBUG, "Activate acrhiver config return[%d] with [%d] errors",
	    ret, (ret == -1) ? ret : lst->length);

	return (&rpc_result);
}

samrpc_result_t *
samrpc_archive_files_6_svr(
strlst_int32_arg_t *arg,	/* arguments to api */
struct svc_req *req		/* ARGSUSED */
)
{

	int ret = -1;
	char *jobID = NULL;
	Trace(TR_DEBUG, "archive files");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to archive files");
	ret = archive_files(arg->ctx, arg->strlst, arg->int32, &jobID);

	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, jobID);

	Trace(TR_DEBUG, "Archive files %d job:%s",
	    ret, Str(jobID));

	return (&rpc_result);
}

samrpc_result_t *
samrpc_is_pool_in_use_1_svr(
string_arg_t *arg,		/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	pool_use_result_t *res = NULL;

	Trace(TR_DEBUG, "Is pool is use");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	res = (pool_use_result_t *)mallocer(sizeof (pool_use_result_t));
	if (res != NULL) {

		Trace(TR_DEBUG, "Call native lib to check if pool is in use");
		ret = is_pool_in_use(arg->ctx, arg->str, &res->in_use,
		    res->used_by);
	}
	SAMRPC_SET_RESULT(ret, SAM_AR_POOL_USED, res);

	Trace(TR_DEBUG, "pool in use return[%d]", ret);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_is_valid_group_1_svr(
string_arg_t *arg,		/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	boolean_t *is_valid = NULL;

	Trace(TR_DEBUG, "Is valid group");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	is_valid = (boolean_t *)mallocer(sizeof (boolean_t));
	if (is_valid != NULL) {

		Trace(TR_DEBUG, "Call native lib to check if group is valid");
		ret = is_valid_group(arg->ctx, arg->str, is_valid);
	}
	SAMRPC_SET_RESULT(ret, SAM_PTR_BOOL, is_valid);

	Trace(TR_DEBUG, "valid group return[%d]", ret);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_is_valid_user_3_svr(
string_arg_t *arg,		/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	boolean_t *is_valid = NULL;

	Trace(TR_DEBUG, "Is valid user");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	is_valid = (boolean_t *)mallocer(sizeof (boolean_t));
	if (is_valid != NULL) {

		Trace(TR_DEBUG, "Call native lib to check if user is valid");
		ret = is_valid_user(arg->ctx, arg->str, is_valid);
	}
	SAMRPC_SET_RESULT(ret, SAM_PTR_BOOL, is_valid);

	Trace(TR_DEBUG, "valid user return[%d]", ret);

	return (&rpc_result);
}

samrpc_result_t *
samrpc_ar_rerun_all_4_svr(
ctx_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Rerun archiver on all fs");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to run archiver on all fs");
	ret = ar_rerun_all(arg->ctx);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Rerun archiver on all fs return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_copy_utilization_6_svr(
int_int_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "get copy utilization");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get copy utilization");
	ret = get_copy_utilization(arg->ctx, arg->int1, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "get copy utilization return[%d]", ret);
	return (&rpc_result);
}
