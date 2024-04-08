
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

#pragma ident	"$Revision: 1.17 $"

#include "mgmt/sammgmt.h"
#include <stdlib.h>

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;

/*
 * recycle_svr.c
 *
 * RPC server wrapper for recycle api
 */


samrpc_result_t *
samrpc_get_rc_log_1_svr(
ctx_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	char *location = NULL;
	upath_t log = {0};

	Trace(TR_DEBUG, "Get recycler log");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get recycler log");
	ret = get_rc_log(arg->ctx, log);
	if (ret == 0) {
		location = strdup(log);
		if (ISNULL(location)) {
			ret = -1;
		}
	}

	SAMRPC_SET_RESULT(ret, SAM_RC_STRING, location);

	Trace(TR_DEBUG, "Get recycler log return[%d] log = [%s]",
	    ret, (location == NULL) ? "NULL" : location);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_set_rc_log_1_svr(
rc_upath_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Set recycler log[%s]", arg->path);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to set recycler log");
	ret = set_rc_log(arg->ctx, arg->path);

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

	Trace(TR_DEBUG, "Set recycler log return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_default_rc_log_1_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	char *location = NULL;
	upath_t log = {0};

	Trace(TR_DEBUG, "Get default recycler log");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get default recycler log");
	ret = get_default_rc_log(arg->ctx, log);

	if (ret == 0) {
		location = strdup(log);
		if (ISNULL(location)) {
			ret = -1;
		}
	}
	SAMRPC_SET_RESULT(ret, SAM_RC_STRING, location);

	Trace(TR_DEBUG, "Get default recycler log return[%d] log = [%s]",
	    ret, (location == NULL) ? "NULL" : location);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_rc_notify_script_1_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	char *script = NULL;
	upath_t path = {0};

	Trace(TR_DEBUG, "Get recycler notify script");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get notify script");
	ret = get_rc_notify_script(arg->ctx, path);

	if (ret == 0) {
		script = strdup(path);
		if (ISNULL(script)) {
			ret = -1;
		}
	}
	SAMRPC_SET_RESULT(ret, SAM_RC_STRING, script);

	Trace(TR_DEBUG, "Get recycler notify script return[%d] script = [%s]",
	    ret, (script == NULL) ? "NULL" : script);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_set_rc_notify_script_1_svr(
rc_upath_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Set recycler notify script[%s]", arg->path);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to set recycler notify script");
	ret = set_rc_notify_script(arg->ctx, arg->path);

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

	Trace(TR_DEBUG, "Set recycler notify script return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_default_rc_notify_script_1_svr(
ctx_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	char *script = NULL;
	upath_t path = {0};

	Trace(TR_DEBUG, "Get default recycler notify script");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib get default notify script");
	ret = get_default_rc_notify_script(arg->ctx, path);

	if (ret == 0) {
		script = strdup(path);
		if (ISNULL(script)) {
			ret = -1;
		}
	}
	SAMRPC_SET_RESULT(ret, SAM_RC_STRING, script);

	Trace(TR_DEBUG, "Get default recycler notify script[%s] return[%d]",
	    (script == NULL) ? "NULL" : script, ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_all_no_rc_vsns_1_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get all no recycle vsn list");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get all no recycle vsn list");
	ret = get_all_no_rc_vsns(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_RC_NO_RC_VSNS_LIST, lst);

	Trace(TR_DEBUG, "Get all no recycle vsn list return[%d] with [%d] vsn",
	    ret, (ret == -1) ? ret : lst->length);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_no_rc_vsns_1_svr(
string_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	no_rc_vsns_t *no_rc_vsns;

	Trace(TR_DEBUG, "Get no recycle vsn for media[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get no recycle vsn");
	ret = get_no_rc_vsns(arg->ctx, arg->str, &no_rc_vsns);

	SAMRPC_SET_RESULT(ret, SAM_RC_NO_RC_VSNS, no_rc_vsns);

	Trace(TR_DEBUG, "Get no recycle vsn for media[%s] return[%d]",
	    arg->str, ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_add_no_rc_vsns_1_svr(
no_rc_vsns_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Add no recycle vsn");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	if ((arg != NULL) && (arg->no_recycle_vsns != NULL)) {
		SET_LIST_TAIL(arg->no_recycle_vsns->vsn_exp);
	}

	Trace(TR_DEBUG, "Call native lib to add no recycle vsn");
	ret = add_no_rc_vsns(arg->ctx, arg->no_recycle_vsns);

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

	Trace(TR_DEBUG, "Add no recycle vsn return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_modify_no_rc_vsns_1_svr(
no_rc_vsns_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Modify no recycle vsn");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	if ((arg != NULL) && (arg->no_recycle_vsns != NULL)) {
		SET_LIST_TAIL(arg->no_recycle_vsns->vsn_exp);
	}

	Trace(TR_DEBUG, "Call native lib to modify no recycle vsns");
	ret = modify_no_rc_vsns(arg->ctx, arg->no_recycle_vsns);

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

	Trace(TR_DEBUG, "Modify no recycle vsns return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_remove_no_rc_vsns_1_svr(
no_rc_vsns_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Remove no recycle vsns");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	if ((arg != NULL) && (arg->no_recycle_vsns != NULL)) {
		SET_LIST_TAIL(arg->no_recycle_vsns->vsn_exp);
	}

	Trace(TR_DEBUG, "Call native lib to remove no recycle vsns");
	ret = remove_no_rc_vsns(arg->ctx, arg->no_recycle_vsns);

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

	Trace(TR_DEBUG, "Remove no recycle vsns return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_all_rc_robot_cfg_1_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get recycle parameters for all libs");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib get recycle parameters for all libs");
	ret = get_all_rc_robot_cfg(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_RC_ROBOT_CFG_LIST, lst);

	Trace(TR_DEBUG, "Get recycler parameters return[%d] with [%d] lists",
	    ret, (ret == -1) ? ret : lst->length);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_rc_robot_cfg_1_svr(
string_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	rc_robot_cfg_t *rc_robot_cfg;

	Trace(TR_DEBUG, "Get recycle parameters for library[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get recycle params for %s",
	    arg->str);
	ret = get_rc_robot_cfg(arg->ctx, arg->str, &rc_robot_cfg);

	SAMRPC_SET_RESULT(ret, SAM_RC_ROBOT_CFG, rc_robot_cfg);

	Trace(TR_DEBUG, "Get recycle parameters for library[%s] return[%d]",
	    arg->str, ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_set_rc_robot_cfg_1_svr(
rc_robot_cfg_arg_t *arg,	/* argument to api */
struct svc_req *req		/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Set recycle parameter");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to set recycle parameter");
	ret = set_rc_robot_cfg(arg->ctx, arg->rc_robot_cfg);

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

	Trace(TR_DEBUG, "Set recycle parameter return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_reset_rc_robot_cfg_1_svr(
rc_robot_cfg_arg_t *arg,	/* argument to api */
struct svc_req *req		/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Reset recycle parameters");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to reset recycle parameters");
	ret = reset_rc_robot_cfg(arg->ctx, arg->rc_robot_cfg);

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

	Trace(TR_DEBUG, "Reset recycle parameters return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_default_rc_params_1_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	rc_param_t *rc_params;

	Trace(TR_DEBUG, "Get default recycle parameters");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get default recycle parameters");
	ret = get_default_rc_params(arg->ctx, &rc_params);

	SAMRPC_SET_RESULT(ret, SAM_RC_PARAM, rc_params);

	Trace(TR_DEBUG, "Get default recycle parameters return[%d]", ret);
	return (&rpc_result);
}
