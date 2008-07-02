
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

#pragma ident	"$Revision: 1.17 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/sammgmt_rpc.h"

/*
 * hosts_clnt.c
 *
 * RPC client wrapper for shared hosts api
 */

/*
 * Return a list of host_info_t from the hosts.<fs_name> file
 *
 */
int
get_host_config(
ctx_t *ctx,			/* client connection		*/
char *fs_name,			/* fs_name to get hosts cfg for */
sqm_lst_t **hosts			/* return - list of host_info_t	*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get host cfg";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_name, hosts)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = fs_name;

	SAMRPC_CLNT_CALL(samrpc_get_host_config, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*hosts = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*hosts));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

int
get_shared_fs_hosts(
ctx_t *ctx,			/* client connection		*/
char *fs_name,			/* fs_name to get hosts cfg for */
int32_t options,
sqm_lst_t **hosts_kv			/* return - list of host_info_t	*/
)
{

	int ret_val;
	int_string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get shared fs hosts";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_name, hosts_kv)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = fs_name;
	arg.i = options;

	SAMRPC_CLNT_CALL(samrpc_get_shared_fs_hosts, int_string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*hosts_kv = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*hosts_kv));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 *
 * Remove a host from the hosts.<fs_name> file for a shared file system
 */
int
remove_host(
ctx_t *ctx,	/* client connection		*/
char *fs_name,	/* name of shared fs from which to remove host */
char *host_name	/* name of host to remove	*/
)
{

	int ret_val;
	string_string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:remove host";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_name, host_name)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str1 = fs_name;
	arg.str2 = host_name;

	SAMRPC_CLNT_CALL(samrpc_remove_host, string_string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 *
 * Set the metadata server for the shared file system
 */
int
change_metadata_server(
ctx_t *ctx,		/* client connection */
uname_t new_server	/* metadata server */
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:change metadata server";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(new_server)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)new_server;

	SAMRPC_CLNT_CALL(samrpc_change_metadata_server, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 *
 * Add a host to the hosts.<fs_name> file for a shared file system
 */
int
add_host(
ctx_t *ctx,	/* client connection		*/
char *fs_name,	/* name of shared fs to which host is to be added */
host_info_t *h	/* host info */
)
{

	int ret_val;
	string_host_info_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:add host";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_name, h)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.fs_name = fs_name;
	arg.h = h;

	SAMRPC_CLNT_CALL(samrpc_add_host, string_host_info_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * discover ip address to detect multiple addresses
 *
 */
int
discover_ip_addresses(
ctx_t *ctx,			/* client connection		*/
sqm_lst_t **ips			/* return - list of ip address str */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:discover ip addresses";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(ips)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_discover_ip_addresses, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*ips = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*ips));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * Set the hosts.<fs_name>.local file to the values passed in.
 */
int
set_advanced_network_cfg(
ctx_t *ctx,		/* client connection */
char *fsname,		/* filesystem */
sqm_lst_t *hosts		/* list of hosts.local key value pairs */
)
{
	int ret_val;
	string_strlst_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:set advanced network cfg";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname, hosts)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = fsname;
	arg.strlst = hosts;

	SAMRPC_CLNT_CALL(samrpc_set_advanced_network_cfg, string_strlst_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * Return a list of key value pairs from the hosts.<fs_name>.local file
 */
int
get_advanced_network_cfg(
ctx_t *ctx,			/* client connection		*/
char *fs_name,			/* fs_name to get hosts cfg for */
sqm_lst_t **hosts			/* return - list of key value pairs */
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get advanced network cfg";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_name, hosts)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = fs_name;

	SAMRPC_CLNT_CALL(samrpc_get_advanced_network_cfg, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*hosts = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*hosts));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * This function returns the name of the mds for the named file system
 * If fs_name is null the name of the metadata server for the first
 * shared file system will be returned.
 */
int get_mds_host(ctx_t *ctx, char *fs_name, char **mds_host) {

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get mds host name";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	/* allow fs_name to be null */
	if (ISNULL(mds_host)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = fs_name;

	SAMRPC_CLNT_CALL(samrpc_get_mds_host, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*mds_host = (char *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);

	return (ret_val);
}

int
set_host_state(
ctx_t *ctx,			/* client connection		*/
char *fs_name,			/* fs_name to set hosts state for */
sqm_lst_t *host_names,
int client_state
)
{

	int ret_val;
	string_strlst_int_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:set host state";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_name, host_names)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = fs_name;
	arg.strlst = host_names;
	arg.int1 = client_state;

	SAMRPC_CLNT_CALL(samrpc_set_host_state, string_strlst_int_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * Function to unmount a shared file system on multiple clients.
 */
int
remove_hosts(
ctx_t *ctx,		/* client connection */
char *fs_name,		/* file system name */
char *hosts[],
int host_count)
{
	int ret_val;
	str_cnt_strarray_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:remove hosts";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_name, hosts)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = fs_name;
	arg.cnt = (unsigned int)host_count;
	arg.array = hosts;

	SAMRPC_CLNT_CALL(samrpc_remove_hosts, str_cnt_strarray_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * Function to unmount a shared file system on multiple clients.
 */
int
add_hosts(
ctx_t *ctx,		/* client connection */
char *fs_name,		/* file system name */
sqm_lst_t *host_infos)
{
	int ret_val;
	string_hostlst_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:add hosts";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_name, host_infos)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);
	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.fs_name = fs_name;
	arg.host_infos = host_infos;

	SAMRPC_CLNT_CALL(samrpc_add_hosts, string_hostlst_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}
