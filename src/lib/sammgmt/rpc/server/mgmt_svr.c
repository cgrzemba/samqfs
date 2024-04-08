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

#pragma ident	"$Revision: 1.38 $"

#include "mgmt/sammgmt.h"
#include <stdlib.h>
#include <unistd.h>
#include "pub/mgmt/mgmt.h"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;

/*
 * mgmt_svr.c
 *
 * RPC server wrapper to get version of lib, initialize etc.
 */

/*
 * samrpc_get_samfs_version_1_svr
 * server stub for get_samfs_version
 *
 * Note that the signature of get_samfs_version and get_samfs_lib_version is
 * different in the sense that the return value is other than int.
 * No samerrmsg or samerrno is available for this from the api
 */
samrpc_result_t *
samrpc_get_samfs_version_1_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	const int ret = 0; /* always success, api does not return an error */
	char *version;

	Trace(TR_DEBUG, "Get SAMFS version");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get SAMFS version");
	version = strdup(get_samfs_version(arg->ctx));

	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, version);

	Trace(TR_DEBUG, "SAMFS version[%s]", version);
	return (&rpc_result);
}


/*
 * samrpc_get_samfs_lib_version_1_svr
 * server stub for get_samfs_version
 *
 * Note that the signature of get_samfs_version and get_samfs_lib_version is
 * different in the sense that the return value is other than int.
 * No samerrmsg or samerrno is available for this from the api
 *
 */
samrpc_result_t *
samrpc_get_samfs_lib_version_1_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	const int ret = 0; /* always success, api does not return an error */
	char *version;

	Trace(TR_DEBUG, "Get SAMFS lib version");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get SAMFS lib version");
	version = strdup(get_samfs_lib_version(arg->ctx));

	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, version);

	Trace(TR_DEBUG, "SAMFS lib version[%s]", version);
	return (&rpc_result);
}


/*
 * samrpc_init_sam_mgmt_1_svr
 * server stub for init_sam_mgmt
 *
 * This function will be called everytime the client gets
 * an exception stating that it is 'Out of sync' with the rpc
 * server or 'sam-mmgt library needs to be re-initialized'
 *
 * 'Out of sync' error indicates that another client is
 * changing configuration information onthe samfs-server.
 *
 * NOTE: As of 4.6 there is no c library init_sam_mgmt call.
 *	All of the required functionality is performed in
 *	this function. This chage is related to the CR
 *	6371323. The goal was to eliminate file modification
 *	time checking. The function is left in place until
 *	the init_sam_mgmt from the client side. This will
 *	be possible once no older clients with mod time
 *	checking are supported.
 */
samrpc_result_t *
samrpc_init_sam_mgmt_1_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	Trace(TR_DEBUG, "Reinitialize with the rpc server and mgmt library");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	/*
	 * synchronize with the sam-mgmt library
	 */
	Trace(TR_DEBUG, "Call native lib to reinit lib");

	/*
	 * Always sync the server and client timestamps if this function is
	 * called. This will cause the client timestamps to be updated with
	 * the server timestamps
	 */
	timestamp_updated = B_TRUE;

	SAMRPC_SET_RESULT(0, SAM_VOID, 0);

	Trace(TR_DEBUG, "Reinitialize library");
	return (&rpc_result);
}


/*
 * samrpc_destroy_process_1_svr
 * server stub for destroy_process
 *
 * samerrmsg or samerrno get propagated from the api
 */
samrpc_result_t *
samrpc_destroy_process_1_svr(
proc_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Destroy process");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to destroy process");
	ret = destroy_process(arg->ctx, arg->pid, arg->ptype);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Destroy process return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_server_info_3_svr(
	ctx_arg_t *arg, /* ARGSUSED */
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = 0;
	char *hostname = NULL;

	hostname = (char *)mallocer(MAXHOSTNAMELEN + 1);
	(void) memset(hostname, 0, MAXHOSTNAMELEN + 1);

	Trace(TR_DEBUG, "Get hostname");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	/*
	 * return the hostname of this server
	 * this is required for shared fs configurations
	 *
	 */
	if (gethostname(hostname, MAXHOSTNAMELEN) != 0) {
		samerrno = 31132;
		(void) strncpy(samerrmsg, GetCustMsg(samerrno), MAX_MSG_LEN);
		free(hostname);
		ret = -1;
	}
	Trace(TR_MISC, "Hostname of server[%s]", hostname);

	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, hostname);

	Trace(TR_DEBUG, "Return hostname[%s]", hostname);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_server_capacities_4_svr(
	ctx_arg_t *arg, /* ARGSUSED */
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = 0;
	char *res = NULL;

	Trace(TR_DEBUG, "Get server capacities entry");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	Trace(TR_DEBUG, "Calling native library to get server capacities");
	ret = get_server_capacities(arg->ctx, &res);

	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, res);

	Trace(TR_DEBUG, "Return capacites[%s]",
	    (res != NULL) ? res : "NULL");
	return (&rpc_result);
}


samrpc_result_t *
samrpc_kill_activity_4_svr(
	string_string_arg_t *arg, /* ARGSUSED */
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = 0;

	Trace(TR_DEBUG, "Kill activity (%s)",
	    (arg->str1 != NULL) ? arg->str1 : "NULL");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	Trace(TR_DEBUG, "Calling native library to kill activity");
	ret = kill_activity(arg->ctx, arg->str1, arg->str2);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_list_activities_4_svr(
	int_string_arg_t *arg, /* ARGSUSED */
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = 0;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "list activity");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	Trace(TR_DEBUG, "Calling native library to list activity");
	ret = list_activities(arg->ctx, arg->i, arg->str, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "List activites return [%d] with [%d] activities",
	    ret, (lst != NULL) ? lst->length : -1);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_system_info_4_svr(
	ctx_arg_t *arg, /* ARGSUSED */
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = 0;
	char *str = NULL;

	Trace(TR_DEBUG, "get server info");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	Trace(TR_DEBUG, "Calling native library to get server info");
	ret = get_system_info(arg->ctx, &str);

	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, str);

	Trace(TR_DEBUG, "Get server info return [%d] with [%s]",
	    ret, (str != NULL) ? str : "NULL");

	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_logntrace_4_svr(
	ctx_arg_t *arg, /* ARGSUSED */
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = 0;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "get log and trace info");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	Trace(TR_DEBUG, "Calling native library to get log and trace info");
	ret = get_logntrace(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "Get log and trace return [%d] with [%d] entries",
	    ret, (lst != NULL) ? lst->length : -1);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_package_info_4_svr(
	string_arg_t *arg, /* ARGSUSED */
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = 0;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "get package info");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	Trace(TR_DEBUG, "Calling native library to get package info");
	ret = get_package_info(arg->ctx, arg->str, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "Get package info return [%d] with [%d] entries",
	    ret, (lst != NULL) ? lst->length : -1);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_configuration_status_4_svr(
	ctx_arg_t *arg, /* ARGSUSED */
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = 0;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "get configuration status");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	Trace(TR_DEBUG, "Calling native library to get config status");
	ret = get_configuration_status(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "Get config status return [%d] with [%d] entries",
	    ret, (lst != NULL) ? lst->length : -1);

	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_architecture_4_svr(
	ctx_arg_t *arg, /* ARGSUSED */
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	char *architecture = NULL;

	Trace(TR_DEBUG, "Get architecture");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	/*
	 * return the architecture of this server
	 * this is required for shared fs configurations
	 *
	 */
	ret = get_architecture(arg->ctx, &architecture);

	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, architecture);

	Trace(TR_DEBUG, "Return architecture[%s]",
	    (architecture != NULL) ? architecture : "NULL");
	return (&rpc_result);
}

samrpc_result_t *
samrpc_list_explorer_outputs_5_svr(
	ctx_arg_t *arg, /* ARGSUSED */
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "list explorer outputs");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	Trace(TR_DEBUG, "Calling native library to list explorer outputs");
	ret = list_explorer_outputs(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "list explorer outputs [%d] with [%d] entries",
	    ret, (lst != NULL) ? lst->length : -1);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_run_sam_explorer_5_svr(
	int_string_arg_t *arg, /* ARGSUSED */
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "list explorer outputs");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	Trace(TR_DEBUG, "Calling native library to run sam explorer");
	ret = run_sam_explorer(arg->ctx, arg->str, arg->i);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "sam explorer returned %d", ret);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_sc_version_5_svr(
	ctx_arg_t *arg, /* ARGSUSED */
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	char *release = NULL;

	Trace(TR_DEBUG, "Get SC release");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	/*
	 * return the architecture of this server
	 * this is required for shared fs configurations
	 *
	 */
	ret = get_sc_version(arg->ctx, &release);

	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, release);

	Trace(TR_DEBUG, "Return release[%s]", Str(release));
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_sc_name_5_svr(
	ctx_arg_t *arg, /* ARGSUSED */
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	char *name = NULL;

	Trace(TR_DEBUG, "Get SC name");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	/*
	 * return the architecture of this server
	 * this is required for shared fs configurations
	 *
	 */
	ret = get_sc_name(arg->ctx, &name);

	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, name);

	Trace(TR_DEBUG, "Return name[%s]", Str(name));
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_sc_nodes_5_svr(
	ctx_arg_t *arg,		/* ARGSUSED */
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *nodes = NULL;

	Trace(TR_DEBUG, "Get SC nodes");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	/* get list of SC nodes */
	ret = get_sc_nodes(arg->ctx, &nodes);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, nodes);
	Trace(TR_DEBUG, "Get SC nodes return [%d] with [%d] entries",
	    ret, (nodes != NULL) ? nodes->length : -1);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_sc_ui_state_5_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Get SC UI state");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	/* no timestamp check required */

	Trace(TR_DEBUG, "Calling native library to get SC UI state");
	ret = get_sc_ui_state(arg->ctx);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Get SC UI state return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_status_processes_6_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get status of processes");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	/* no timestamp check required */

	Trace(TR_DEBUG, "Calling native library to get process status");
	ret = get_status_processes(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "monitor status [%d] with [%d] entries",
	    ret, (lst != NULL) ? lst->length : -1);

	Trace(TR_DEBUG, "Get monitor status return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_component_status_summary_6_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get status summary of all monitored components");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	/* no timestamp check required */

	Trace(TR_DEBUG, "Calling native library to get component status");
	ret = get_component_status_summary(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "component status [%d] with [%d] entries",
	    ret, (lst != NULL) ? lst->length : -1);

	Trace(TR_DEBUG, "Get component status summary return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_config_summary_5_0_svr(
	ctx_arg_t *arg, /* ARGSUSED */
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	char *res = NULL;

	Trace(TR_DEBUG, "Get config summary");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	ret = get_config_summary(arg->ctx, &res);

	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, res);

	Trace(TR_DEBUG, "Return config summary [%s]", Str(res));
	return (&rpc_result);
}
