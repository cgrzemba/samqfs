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

#pragma ident	"$Revision: 1.36 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/sammgmt_rpc.h"
/*
 * mgmt_clnt.c
 *
 * RPC client wrapper for getting version of mgmt lib
 */

/*
 * get the SAMFS product version number
 */
char *
get_samfs_version(
ctx_t *ctx	/* client connection */
)
{

	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get samfs version";
	char *err_msg;
	char *version;

	PTRACE(2, "%s entry", func_name);

	if (!ctx ||
	    !ctx->handle ||
	    !(ctx->handle->clnt) ||
	    !(ctx->handle->svr_name)) {
		return (NULL);
	}
	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	if (samrpc_get_samfs_version_1(&arg, &result,
	    ctx->handle->clnt) != RPC_SUCCESS) {
		samerrno = SE_RPC_FAILED;
		err_msg = clnt_sperror(ctx->handle->clnt,
		    ctx->handle->svr_name);
		snprintf(samerrmsg, MAX_MSG_LEN, err_msg);
		return (NULL);
	}

	if (result.status < 0) {
		samerrno = result.samrpc_result_u.err.errcode;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    result.samrpc_result_u.err.errmsg);
	}

	version = (char *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s exit", func_name);
	return (version);
}


/*
 * Get the SAMFS management library version number
 */
char *
get_samfs_lib_version(
ctx_t *ctx	/* client connection */
)
{

	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get samfs lib version";
	char *err_msg;
	char *version;

	PTRACE(2, "%s entry", func_name);

	if ((ctx == NULL) ||
	    (ctx->handle == NULL) ||
	    (ctx->handle->clnt == NULL) ||
	    (ctx->handle->svr_name == NULL)) {
		return (NULL);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	if (samrpc_get_samfs_lib_version_1(&arg, &result,
	    ctx->handle->clnt) != RPC_SUCCESS) {
		samerrno = SE_RPC_FAILED;
		err_msg = clnt_sperror(ctx->handle->clnt,
		    ctx->handle->svr_name);
		snprintf(samerrmsg, MAX_MSG_LEN, err_msg);
		return (NULL);
	}

	if (result.status < 0) {
		samerrno = result.samrpc_result_u.err.errcode;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    result.samrpc_result_u.err.errmsg);
	}
	version = (char *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s exit", func_name);
	return (version);
}

/*
 * Function to re-initialize the library
 *
 * If the configuration files have been modified since the library was
 * initialized, then this function gets called
 *
 */
int
init_sam_mgmt(
ctx_t *ctx	/* client connection */
)
{
	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:Reinitialize the mgmt library";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	if ((stat = samrpc_init_sam_mgmt_1(&arg, &result,
	    ctx->handle->clnt)) != RPC_SUCCESS) {
		SET_RPC_ERROR(ctx, func_name, stat);
	}

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * Cancel a SAM-FS job by killing the process specified
 */
int
destroy_process(
ctx_t *ctx,		/* client connection */
pid_t pid,		/* process pid */
proctype_t ptype	/* process type */
)
{
	int ret_val;
	proc_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:destroy process";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.pid = pid;
	arg.ptype = ptype;

	SAMRPC_CLNT_CALL(samrpc_destroy_process, proc_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * Get the hostname of the server
 *
 */
int
get_server_info(
ctx_t *ctx,					/* client connection */
char hostname[MAXHOSTNAMELEN+ 1]		/* hostname */
)
{
	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get hostname of server";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(hostname)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_server_info, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	strncpy(hostname, (char *)result.samrpc_result_u.result.result_data,
	    MAXHOSTNAMELEN);

	free(result.samrpc_result_u.result.result_data);

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * Get the filesystem and media capacities as CSV key value pairs
 *
 * PARAMS:
 *   ctx_t *    IN   - context object
 *   char **    OUT  - malloced CSV key value pairs of the following format
 *	MountedFS		= <number of mounted file systems>
 *	DiskCache		= <diskcache of mounted SAM-FS/QFS
 *					file systems in kilobytes>
 *	AvailableDiskCache	= <available disk cache in kilobytes>
 *	LibCount		= <number of libraries>
 *	MediaCapacity		= <capacity of library in kilobytes>
 *	AvailableMediaCapacity	= <available capacity in kilobytes>
 *	SlotCount		= <number of configured slots>
 *
 * RETURNS:
 *   success -  0
 *   error   -  -1
 *
 */
int
get_server_capacities(
ctx_t *ctx,	/* client connection */
char **res	/* CSV key value pairs */
)
{
	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get server capacities";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(res)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_server_capacities, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*res = (char *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * List activities on this server
 * PARAMS
 *   ctx_t *    IN   - context object
 *   int	IN   - maximum entries requested
 *   char *	IN   - restrictions if any
 *   sqm_lst_t **  OUT  - malloced list of strings
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int
list_activities(
ctx_t *ctx,	/* client connection */
int maxentries,
char *restrictions,
sqm_lst_t **activities	/* return list of strings describing activities */
)
{
	int ret_val;
	int_string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:list activities on server";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(activities)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.i = maxentries;
	arg.str = restrictions;

	SAMRPC_CLNT_CALL(samrpc_list_activities, int_string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*activities = (sqm_lst_t *)result.samrpc_result_u.result.result_data;
	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*activities));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * kill activity %s on this server
 * PARAMS
 *   ctx_t *    IN   - context object
 *   char *	IN   - activity id/desc
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int
kill_activity(
ctx_t *ctx,	/* client connection */
char *activityid,
char *type
)
{
	int ret_val;
	string_string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:kill activity";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(activityid)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str1 = activityid;
	arg.str2 = type;

	SAMRPC_CLNT_CALL(samrpc_kill_activity, string_string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * Get system info
 * PARAMS
 *   ctx_t *    IN   - context object
 *   char ** 	OUT  - malloced string containing sysinfo
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int
get_system_info(
ctx_t *ctx,	/* client connection */
char **res	/* return string */
)
{
	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get system info";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(res)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_system_info, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*res = (char *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * Get log and trace info
 * PARAMS
 *   ctx_t *    IN   - context object
 *   sqm_lst_t **OUT - list of formatted strings containing log/trace info
 *
 * format:
 * Name=name,
 * Type=log/trace,
 * State=on/off,
 * Path=filename,
 * Flags=flags,
 * Size=size,
 * Modtime=last modified time (num of seconds since 1970)
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int
get_logntrace(
ctx_t *ctx,	/* client connection */
sqm_lst_t **lst
)
{
	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get log and trace info";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(lst)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_logntrace, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*lst = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*lst));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * Get package info
 * DESCRIPTION:
 *	Get information for the packages of interest:
 *	SUNWsamfsu SUNWsamfsr SUNWqfsu SUNWqfsr SUNWstade SUNWsamqfsuiu
 *	Possibly lockhart packages too.
 *
 *
 * PARAMS
 *   ctx_t *    IN   - context object
 *   char *	IN   - pkgname
 *   sqm_lst_t **	OUT  - list of name-value pairs describing the package
 * format:
 *  PKGINST = SUNWsamfsu
 *     NAME = Sun SAM-FS and Sun SAM-QFS software Solaris 9 (usr)
 * CATEGORY = system
 *     ARCH = sparc
 *  VERSION = 4.3.multidisk REV=debug REV=5.9.2005.01.18
 *   VENDOR = Sun Microsystems Inc.
 *     DESC = Storage and Archive Manager File System
 *   PSTAMP = ns-east-6420050118171403
 * INSTDATE = Jan 20 2005 13:58
 *  HOTLINE = Please contact your local service provider
 *   STATUS = completely installed
 *
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int
get_package_info(
ctx_t *ctx,	/* client connection */
char *pkgs,
sqm_lst_t **lst
)
{
	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get package info";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(lst)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = pkgs;

	SAMRPC_CLNT_CALL(samrpc_get_package_info, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*lst = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*lst));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * Get configuration status
 * PARAMS
 *   ctx_t *    IN   - context object
 *   sqm_lst_t **	OUT  - list of name-value pairs describing the status
 *
 * format:
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int
get_configuration_status(
ctx_t *ctx,	/* client connection */
sqm_lst_t **lst
)
{
	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get configuration status";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(lst)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_configuration_status, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*lst = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*lst));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * Get the architecture of the server
 *
 */
int
get_architecture(
ctx_t *ctx,					/* client connection */
char **architecture		/* architecture */
)
{
	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get architecture of server";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	if ((ctx == NULL) ||
	    (ctx->handle == NULL) ||
	    (ctx->handle->clnt == NULL)) {
		return (-1);
	}
	if (ISNULL(architecture)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_architecture, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*architecture =  (char *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d] [%s]...", func_name, ret_val,
	    (*architecture != NULL) ? *architecture : "NULL");

	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * Get a list of sam explorer outputs
 * PARAMS
 *   ctx_t *    IN   - context object
 *   sqm_lst_t **	OUT  - list of name-value pairs giving the location
 *			and file characteristics of existing sam reports
 *
 * format:
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int
list_explorer_outputs(
ctx_t *ctx,	/* client connection */
sqm_lst_t **lst
)
{
	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:list explorer outputs";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(lst)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_list_explorer_outputs, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*lst = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*lst));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


int
run_sam_explorer(
ctx_t *ctx,
char *location,
int log_lines) {

	int ret_val;
	int_string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:run sam explorer";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)location;
	arg.i = log_lines;

	SAMRPC_CLNT_CALL(samrpc_run_sam_explorer, int_string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * Get SC version
 */
int
get_sc_version(
ctx_t *ctx,		/* client connection */
char **release		/* SC release */
)
{
	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get SC release";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(release)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_sc_version, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*release =  (char *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d] [%s]...",
	    func_name, ret_val, (*release != NULL) ? *release : "NULL");

	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * Get SC name
 */
int
get_sc_name(
ctx_t *ctx,		/* client connection */
char **name		/* SC name */
)
{
	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get SC name";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(name)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_sc_name, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*name =  (char *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d] [%s]...",
	    func_name, ret_val, Str(name));

	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * Get configuration status
 * PARAMS
 *   ctx_t *    IN   - context object
 *   sqm_lst_t **	OUT  - list of name-value pairs describing each node
 *
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int
get_sc_nodes(
ctx_t *ctx,	/* client connection */
sqm_lst_t **lst
)
{
	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get node status";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(lst)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_sc_nodes, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*lst = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*lst));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

int
get_sc_ui_state(
ctx_t *ctx	/* client connection */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get sc ui state";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_sc_ui_state, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning sc ui state [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

int
get_status_processes(
ctx_t *ctx,	/* client connection */
sqm_lst_t **lst
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get status of daemons";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_status_processes, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*lst = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*lst));

	PTRACE(2, "%s returning processes status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

int
get_component_status_summary(
ctx_t *ctx,			/* client connection */
sqm_lst_t **status_lst		/* return list of integers giving status */
)
{
	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get summary of status";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_component_status_summary, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*status_lst = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}
