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

#pragma ident	"$Revision: 1.19 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/sammgmt_rpc.h"

/*
 * recycle_clnt.c
 *
 * RPC client wrapper for recycle api
 */

/*
 * get_rc_log
 * get log file location for recycler from SAM_CONFIG_PATH/recycler.cmd
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
get_rc_log(
ctx_t *ctx,		/* context argument		*/
upath_t rc_log		/* return - recycler log	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get rc log";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(rc_log)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_rc_log, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	strncpy(rc_log, (char *)result.samrpc_result_u.result.result_data,
	    sizeof (upath_t));

	free(result.samrpc_result_u.result.result_data);

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * set_rc_log
 * set the log file location for recycler
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
set_rc_log(
ctx_t *ctx,		/* client communication	*/
const upath_t rc_log	/* log file location	*/
)
{

	int ret_val;
	rc_upath_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:set rc log";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(rc_log)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	strncpy(arg.path, rc_log, sizeof (upath_t));

	SAMRPC_CLNT_CALL(samrpc_set_rc_log, rc_upath_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_default_rc_log
 * get the default log location
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
get_default_rc_log(
ctx_t *ctx,	/* client connection		*/
upath_t rc_log	/* return - releaser log	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get default rc log";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(rc_log)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_default_rc_log, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	strncpy(rc_log, (char *)result.samrpc_result_u.result.result_data,
	    sizeof (upath_t));

	free(result.samrpc_result_u.result.result_data);

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_rc_notify_script
 * get the location of the recycler script from SAM_CONFIG_PATH/recycler.cmd
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
get_rc_notify_script(
ctx_t *ctx,		/* client connection		*/
upath_t rc_script	/* return - recycler script	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get rc notify script";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(rc_script)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_rc_notify_script, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	strncpy(rc_script, (char *)result.samrpc_result_u.result.result_data,
	    sizeof (upath_t));

	free(result.samrpc_result_u.result.result_data);

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * set_rc_notify_script
 * set the location/name of the recycler script in SAM_CONFIG_PATH/recycer.cmd
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
set_rc_notify_script(
ctx_t *ctx,		/* client connection		*/
const upath_t rc_script	/* recycler script location	*/
)
{

	int ret_val;
	rc_upath_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:set rc notify script";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(rc_script)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	strncpy(arg.path, rc_script, sizeof (upath_t));

	SAMRPC_CLNT_CALL(samrpc_set_rc_notify_script, rc_upath_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_default_rc_notify_script
 * get the location of the default recycler script
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
get_default_rc_notify_script(
ctx_t *ctx,		/* client connection			*/
upath_t rc_script	/* return - recycler script location	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get default rc notify script";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(rc_script)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_default_rc_notify_script, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	strncpy(rc_script, (char *)result.samrpc_result_u.result.result_data,
	    sizeof (upath_t));

	free(result.samrpc_result_u.result.result_data);

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_all_no_rc_vsns
 * get the list of all VSNs (as specified originally, i.e., regular
 * expression, explicit vsn name etc.) for which no_recycle directive is set.
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
get_all_no_rc_vsns(
ctx_t *ctx,		/* client connection		*/
sqm_lst_t **no_rc_vsns	/* return - list of VSNS	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all no rc vsns";
	char *err_msg;
	enum clnt_stat stat;

	/* fix the tail */
	node_t *node;
	no_rc_vsns_t *no_vsn;


	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(no_rc_vsns)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_all_no_rc_vsns, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*no_rc_vsns = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*no_rc_vsns));
	if (*no_rc_vsns != NULL) {
		node = (*no_rc_vsns)->head;
		while (node != NULL) {
			no_vsn = (no_rc_vsns_t *)node->data;
			if (no_vsn != NULL) {
				SET_LIST_TAIL(no_vsn->vsn_exp);
			}
		}
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_no_rc_vsns
 * get the no recycle VSN for a particular type of media
 * (as specified originally, i.e., regular expression, explicit vsn name etc.)
 * for which no_recycle directive is set
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
get_no_rc_vsns(
ctx_t *ctx,			/* client connection	*/
const mtype_t media_type,	/* media type		*/
no_rc_vsns_t **no_recycle_vsns	/* return - no recycle vsn */
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get no rc vsns";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(no_recycle_vsns, media_type)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)media_type;

	SAMRPC_CLNT_CALL(samrpc_get_no_rc_vsns, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*no_recycle_vsns = (no_rc_vsns_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	if ((*no_recycle_vsns) != NULL) {
		SET_LIST_TAIL((*no_recycle_vsns)->vsn_exp);
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * add_no_rc_vsns
 * add the no_recycle directive in the SAM_CONFIG_PATH/recycler.cmd for the
 * media type and list of VSNS as given in no_recycle_vsns
 *
 * If no_recycle directive already exists then this function returns an error
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
add_no_rc_vsns(
ctx_t *ctx,			/* client connection		*/
no_rc_vsns_t *no_recycle_vsns	/* media type and vsn list	*/
)
{

	int ret_val;
	no_rc_vsns_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:add no rc vsns";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(no_recycle_vsns)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.no_recycle_vsns = no_recycle_vsns;

	SAMRPC_CLNT_CALL(samrpc_add_no_rc_vsns, no_rc_vsns_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * modify_no_rc_vsns
 * modify the no_recycle directive in the SAM_CONFIG_PATH/recycler.cmd
 * The VSN and the media type (no_recycle_vsns) are supplied as input parameters
 *
 * If the no_recycle directive does not exist, an error is returned
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
modify_no_rc_vsns(
ctx_t *ctx,			/* client connection		*/
no_rc_vsns_t *no_recycle_vsns	/* media type and VSN list	*/)
{

	int ret_val;
	no_rc_vsns_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:modify no rc vsns";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(no_recycle_vsns)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.no_recycle_vsns = no_recycle_vsns;

	SAMRPC_CLNT_CALL(samrpc_modify_no_rc_vsns, no_rc_vsns_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * remove_no_rc_vsns
 * remove the no_recycle directive in the SAM_CONFIG_PATH/recycler.cmd
 * The VSN and the media type (no_recycle_vsns) are supplied as input parameters
 *
 * If the no_recycler directive does not exist or if the no_recycle directive
 * does not exist for that particular media type (supplied as input param),
 * it is an error. However the name of VSNS have no role as input
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
remove_no_rc_vsns(
ctx_t *ctx,			/* client connection	*/
no_rc_vsns_t *no_recycle_vsns	/* media type		*/
)
{

	int ret_val;
	no_rc_vsns_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:remove no rc vsns";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(no_recycle_vsns)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.no_recycle_vsns = no_recycle_vsns;

	SAMRPC_CLNT_CALL(samrpc_remove_no_rc_vsns, no_rc_vsns_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_all_rc_robot_cfg
 * get the parameters that control recycling for every library for which a
 * robot-family-set has been defined in the SAM_CONFIG_PATH/recycler.cmd
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
get_all_rc_robot_cfg(
ctx_t *ctx,		/* client connection			*/
sqm_lst_t **rc_robot_cfg	/* return - list of recycler parameters	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all rc robot cfg";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(rc_robot_cfg)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_all_rc_robot_cfg, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*rc_robot_cfg = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*rc_robot_cfg));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_rc_robot_cfg
 * get the parameters that control recycling for a particular library for which
 * a robot-family-set has been defined in the SAM_CONFIG_PATH/recycler.cmd
 *
 * The library name is given as input parameter
 *
 * If a robot-family-set is not defined for this particular library, it is an
 * error.
 * If multiple directives have been set for a particular library, then the
 * parameters in the first definition are returned. It is not an error
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
get_rc_robot_cfg(
ctx_t *ctx,			/* client connection	*/
const upath_t robot_name,	/* robot name		*/
rc_robot_cfg_t **rc_robot_cfg	/* return - recycler control parameters	*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get rc robot cfg";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(rc_robot_cfg, robot_name)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)robot_name;

	SAMRPC_CLNT_CALL(samrpc_get_rc_robot_cfg, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*rc_robot_cfg = (rc_robot_cfg_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * set_rc_robot_cfg
 * set recycle parameters for a particular library
 *
 * If recycle parameters for this particular library does not exist, it is
 * added to the recycler.cmd file. However if the robot-family-set for the
 * library is to be modified/updated, then the field bits in change_flag has
 * to be set correctly.
 *
 * If the robot-family-set (recycle parameters) exits in releaser.cmd but the
 * change_flag is not set, then the recycle parameters for this library are not
 * changed. This is not an error. If there are multiple robot-family-sets o
 * defined for this particular library, then the first directive definition
 * gets modified
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
set_rc_robot_cfg(
ctx_t *ctx,			/* client connection		*/
rc_robot_cfg_t *rc_robot_cfg	/* library name and parameters	*/
)
{

	int ret_val;
	rc_robot_cfg_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:set rc robot cfg";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(rc_robot_cfg)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.rc_robot_cfg = rc_robot_cfg;

	SAMRPC_CLNT_CALL(samrpc_set_rc_robot_cfg, rc_robot_cfg_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * reset_rc_robot_cfg
 * reset/remove recycle parameters for a particular library from the
 * recycler.cmd file
 *
 * If recycle parameters for this particular library does not exist, it is
 * an error.
 *
 * If there are multiple robot-family-sets defined for this particular library
 * then the first directive definition gets removed
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
reset_rc_robot_cfg(
ctx_t *ctx,			/* client connection		*/
rc_robot_cfg_t *rc_robot_cfg	/* library name given as input	*/
)
{

	int ret_val;
	rc_robot_cfg_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:reset rc robot cfg";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(rc_robot_cfg)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.rc_robot_cfg = rc_robot_cfg;

	SAMRPC_CLNT_CALL(samrpc_reset_rc_robot_cfg, rc_robot_cfg_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);
	ret_val = result.status;
	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_default_rc_params
 * get the default recycle parameters.
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
get_default_rc_params(
ctx_t *ctx,		/* client connection	*/
rc_param_t **rc_params	/* return - default recycler parameters */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get default rc params";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(rc_params)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_default_rc_params, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*rc_params = (rc_param_t *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}
