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

#pragma ident	"$Revision: 1.17 $"

#include <stdlib.h>
#include "mgmt/sammgmt.h"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * release_svr.c
 *
 * RPC server wrapper for release api
 */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;


/*
 * samrpc_get_all_rl_fs_directives_1
 * server stub for get_all_rl_fs_directives
 */
samrpc_result_t *
samrpc_get_all_rl_fs_directives_1_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get all releaser directives");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get all releaser directives");
	ret = get_all_rl_fs_directives(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_RL_FS_DIRECTIVE_LIST, lst);

	Trace(TR_DEBUG, "Get all releaser directives return[%d] with [%d] list",
	    ret, (ret == -1) ? ret : lst->length);
	return (&rpc_result);
}


/*
 * samrpc_get_rl_fs_directive_1
 * server stub for get_rl_fs_directive
 */
samrpc_result_t *
samrpc_get_rl_fs_directive_1_svr(
string_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	rl_fs_directive_t *fs_directive;

	Trace(TR_DEBUG, "Get releaser directives for fs[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get releaser directives for fs[%s]",
	    arg->str);
	ret = get_rl_fs_directive(arg->ctx, arg->str, &fs_directive);

	SAMRPC_SET_RESULT(ret, SAM_RL_FS_DIRECTIVE, fs_directive);

	Trace(TR_DEBUG, "Get releaser directives for fs return[%d]", ret);
	return (&rpc_result);
}


/*
 * samrpc_get_default_rl_fs_directive_1
 * server stub for get_default_rl_fs_directive
 */
samrpc_result_t *
samrpc_get_default_rl_fs_directive_1_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	rl_fs_directive_t *fs_directive;

	Trace(TR_DEBUG, "Get default releaser directive");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get default releaser directive");
	ret = get_default_rl_fs_directive(arg->ctx, &fs_directive);

	SAMRPC_SET_RESULT(ret, SAM_RL_FS_DIRECTIVE, fs_directive);

	Trace(TR_DEBUG, "Get default releaser directive return[%d]", ret);
	return (&rpc_result);
}


/*
 * samrpc_set_rl_fs_directive_1
 * server stub for set_rl_fs_directive
 */
samrpc_result_t *
samrpc_set_rl_fs_directive_1_svr(
rl_fs_directive_arg_t *arg,	/* argument to api */
struct svc_req *req		/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Set releaser directive");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to set releaser directive");
	ret = set_rl_fs_directive(arg->ctx, arg->rl_fs_directive);

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

	Trace(TR_DEBUG, "Set releaser directive return[%d]", ret);
	return (&rpc_result);
}


/*
 * samrpc_reset_rl_fs_directive_1
 * server stub for reset_rl_fs_directive
 */
samrpc_result_t *
samrpc_reset_rl_fs_directive_1_svr(
rl_fs_directive_arg_t *arg,	/* argument to api */
struct svc_req *req		/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Reset releaser directive");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to reset releaser directive");
	ret = reset_rl_fs_directive(arg->ctx, arg->rl_fs_directive);

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

	Trace(TR_DEBUG, "Reset releaser directive return[%d]", ret);
	return (&rpc_result);
}


/*
 * samrpc_get_releasing_fs_list_1
 * server stub for get_releasing_fs_list
 */
samrpc_result_t *
samrpc_get_releasing_fs_list_1_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get releasing list");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get releasing list");
	ret = get_releasing_fs_list(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_RL_FS_LIST, lst);

	Trace(TR_DEBUG, "Get releasing list return[%d] with [%d] lists",
	    ret, (ret == -1) ? ret : lst->length);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_release_files_6_svr(
strlst_int32_int32_arg_t *arg,	/* arguments to api */
struct svc_req *req		/* ARGSUSED */
)
{

	int ret = -1;
	char *jobID = NULL;
	Trace(TR_DEBUG, "Release files");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to release files");
	ret = release_files(arg->ctx, arg->strlst, arg->int1,
	    arg->int2, &jobID);

	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, jobID);

	Trace(TR_DEBUG, "Release files %d job:%s", ret, Str(jobID));

	return (&rpc_result);
}
