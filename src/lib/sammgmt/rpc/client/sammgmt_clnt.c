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

#pragma ident	"$Revision: 1.75 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/sammgmt_rpc.h"
#include "pub/mgmt/mgmt.h"

#include <stdio.h>
#include <stdlib.h>			/* getenv, exit */

/*
 * this returns the mgmt API version used by client-side RPC.
 * It must match the API version implemented on the SAM-FS/QFS server.
 * (see mgmt.h)
 */
char *
samrpc_version() {
	return (API_VERSION);
}

/*
 * samrpc_create_clnt()
 *
 * This function creates a samrpc_client_t structure which holds the
 * CLIENT handle, the server name and the server timestamp.
 *
 * returns a valid samrpc_client_t on success, if the server program has not
 * been registered, NULL is returned. samerrno and samerrmsg describe the error
 */
samrpc_client_t *
samrpc_create_clnt(
const char *rpc_server	/* rpc server name */
)
{
	return (samrpc_create_clnt_timed(rpc_server, DEF_TIMEOUT_SEC));
}

/*
 * samrpc_create_clnt_timed()
 *
 * This causes the timeout value to be set to tv_sec for all requests
 * coming from this client, unless overridden by samrpc_set_timeout.
 *
 * A timeout value is expected as an argument along with the server name
 * This function creates a samrpc_client_t structure which holds the
 * CLIENT handle, the server name and the server timestamp.
 *
 * returns a valid samrpc_client_t on success, if the server program has not
 * been registered, NULL is returned. samerrno and samerrmsg describe the error
 */
samrpc_client_t *
samrpc_create_clnt_timed(
const char *rpc_server,	/* rpc server name */
time_t tv_sec		/* timeout in seconds */
)
{
	samrpc_client_t *samrpc_client = NULL;
	CLIENT *client = NULL;
	char *func_name = "rpc:create client connection with specified timeout";
	char *err_msg;
	int timestamp;
	int dummy = 0;
	enum clnt_stat rpc_stat;

	/* for backward compatibilty */
	struct ct_data *ct = NULL;
	XDR *xdrs = NULL;
	ctx_t ctx;
	char *version;

	struct timeval tm;
	tm.tv_sec = tv_sec;
	tm.tv_usec = 0;

	PTRACE(2, "%s entry", func_name);

	samrpc_client = (samrpc_client_t *)mallocer(sizeof (samrpc_client_t));
	if (!samrpc_client) {
		PTRACE(2, "%s out of memory...", func_name);
		goto err;
	}

	memset(samrpc_client, 0, sizeof (samrpc_client_t));
	samrpc_client->svr_name = strdup(rpc_server);

	if (!samrpc_client->svr_name) {
		samerrno = SE_NO_MEM;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    "Out of memory [err = %d]", samerrno);
		PTRACE(2, "%s out of memory...", func_name);
		goto err;
	}

	PTRACE(3, "%s creating client handle with rpc server %s...",
	    func_name, rpc_server);
	/*
	 * Create a CLIENT handle (that is used to describe a unique client/
	 * server connection) for the specified server host, program and
	 * version number. select transport as 'tcp' as procedures might
	 * require to return more than 8k bytes of encoded data
	 *
	 * If the server program has been registered, then a CLIENT handle is
	 * returned. If the query of the named server's portmap does not
	 * reveal a match in program number, then NULL is returned
	 */

	client = clnt_create_timed(rpc_server, SAMMGMTPROG,
	    SAMMGMTVERS, "tcp", &tm);
	if (client == (CLIENT *)NULL) {
		samerrno = SE_RPC_CANNOT_CREATE_CLIENT;
		err_msg = clnt_spcreateerror(rpc_server);
		snprintf(samerrmsg, MAX_MSG_LEN, err_msg);
		PTRACE(2, "%s %s", func_name, err_msg);
		goto err;
	}

	PTRACE(3, "%s initializing client handle %d with rpc server %s...",
	    func_name, client, rpc_server);

	/* set the timeout value for this client to tv_sec */
	samrpc_client->clnt = client;
	samrpc_set_timeout(samrpc_client, tv_sec);

	/*
	 * Initialize the sam-mgmt server to keep track of changes made to the
	 * config files by other process
	 *
	 * All client stubs take 3 arguments, 1) the argument to the
	 * api, 2) the the pointer to the result from the
	 * server program and 3) is the CLIENT handle
	 * since in this case, we don't have any argument to api, we just pass
	 * in a dummy argument
	 */
	rpc_stat = samrpc_init_1(&dummy, &timestamp, client);

	if (rpc_stat != RPC_SUCCESS) {
		samerrno = SE_RPC_CANNOT_CREATE_CLIENT;
		err_msg = clnt_sperror(client, rpc_server);
		snprintf(samerrmsg, MAX_MSG_LEN, err_msg);
		PTRACE(2, "%s %s", func_name, err_msg);
		goto err;
	}

	/*
	 * The client uses samrpc_client_t structure to identify the
	 * connection between the client and the server.
	 * In addition to the client handle, this structure also holds the
	 * server name and server timestamp
	 */
	samrpc_client->clnt = client;
	samrpc_client->timestamp = timestamp;

	/*
	 * For backward compatibilty, store the server version in the
	 * CLIENT structure so that it could be used while
	 * deserializing
	 */
	ctx.handle = samrpc_client;
	ctx.dump_path[0] = '\0';
	ctx.read_location[0] = '\0';
	ctx.user_id[0] = '\0';

	/* LINTED - alignment (32bit) OK */
	ct = (struct ct_data *)client->cl_private;
	xdrs = &(ct->ct_xdrs);
	xdrs->x_public = NULL;

	if ((version = get_samfs_lib_version(&ctx)) == NULL) {
		PTRACE(2, "Could not get server version");
		goto err;
	}

	xdrs->x_public = strdup(version); /* server version */

	/*
	 * Instead of saving the server name, the svr_name is used to
	 * store the architecture of the server
	 */
	if (strcmp(version, "1.3.1") >= 0) {
		free(samrpc_client->svr_name);
		samrpc_client->svr_name = NULL;

		if (get_architecture(&ctx, &(samrpc_client->svr_name)) == -1) {
			PTRACE(2, "Could not get architecture");
			goto err;
		}

	}
	PTRACE(2, "Client server name/ architecture [%s]",
	    samrpc_client->svr_name != NULL ?
	    samrpc_client->svr_name : "NULL");

	return (samrpc_client);

err:
	if (client)
		clnt_destroy(client);
	if (samrpc_client) {
		if (samrpc_client->svr_name)
			free(samrpc_client->svr_name);
		free(samrpc_client);
	}
	return ((samrpc_client_t *)NULL);
}

/*
 * samrpc_destroy_clnt()
 *
 * Memory allocated to the samrpc_client_t structure is freed. This should be
 * called when the client is done with the server connection
 * This function deallocates memory associated with the CLIENT handle. If the
 * RPC routines were used to open the socket, the socket is neatly closed
 *
 * returns 0 on success, -1 on failure
 */
int
samrpc_destroy_clnt(
samrpc_client_t *rpc_client	/* CLIENT handle and other connection info */
)
{
	char *func_name = "rpc:destroy client connection";

	PTRACE(2, "%s entry", func_name);

	if (!rpc_client) {
		samerrno = SE_RPC_INVALID_CLIENT_HANDLE;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    "Invalid client communication information");
		PTRACE(2, "%s invalid client communication information",
		    func_name);
		return (-1);
	}

	if (rpc_client->clnt)
		clnt_destroy(rpc_client->clnt);
	if (rpc_client->svr_name)
		free(rpc_client->svr_name);
	free(rpc_client);

	PTRACE(2, "%s exit", func_name);
	return (0);
}

/*
 * samrpc_get_timeout
 *
 * get the current default timeout for the client
 */
int
samrpc_get_timeout(
samrpc_client_t *rpc_client,	/* client */
time_t *tv_sec		/* return - timeout */
)
{
	struct timeval tm;

	if (clnt_control(
	    rpc_client->clnt, CLGET_TIMEOUT, (char *)&tm) == TRUE) {

		*tv_sec = tm.tv_sec;
		return (0);
	} else {
		*tv_sec = -1;
		return (-1);
	}

}

/*
 * samrpc_set_timeout
 *
 * Change the default time-out
 *
 * Any value set by clnt_control changes the default time-out
 * and causes timeout values specified by clnt_call() to be
 * ignored
 */
int
samrpc_set_timeout(
samrpc_client_t *rpc_client,	/* client */
time_t tv_sec			/* input - timeout value in seconds */
)
{
	struct timeval tm;
	tm.tv_sec = tv_sec;
	tm.tv_usec = 0;

	if (clnt_control(
	    rpc_client->clnt, CLSET_TIMEOUT, (char *)&tm) == TRUE) {

		return (0);
	} else {
		return (-1);
	}
}


/*
 * TIMEOUT
 *
 * The default timeout is set to 65 seconds.
 * TIMEOUT can be changed using samrpc_set_timeout()
 *
 * Any value set by the samrpc_set_timeout() causes the default
 * time-out for that client to be overridden
 *
 */
#define	GET_TIMEOUT(clnt, tm) \
	tm.tv_sec = DEF_TIMEOUT_SEC; \
	tm.tv_usec = 0; \
	clnt_control(clnt, CLGET_TIMEOUT, (char *)&tm);

/*
 * RPC client side stubs
 *
 * We use MT-safe client stub to process incoming client requests concurrently.
 * This is necessary because the Web based GUI might need to talk to different
 * servers concurrently
 *
 * The client handle is passed to the stub routine that calls the remote
 * procedure. A pointer to both the arguments and the results needs to be
 * passed in to preserve re-entrancy.
 * If the api takes more than one argument, then arguments must be passed in a
 * struct.
 * The value returned by the stub function indicates whether this call is a
 * success or a failure. The stub returns RPC_SUCCESS if the call is successful.
 *
 */
enum clnt_stat
samrpc_init_1(argp, clnt_res, clnt)
	void *argp;
	int *clnt_res;
	CLIENT *clnt;
{

	struct timeval tm;
	GET_TIMEOUT(clnt, tm);

	return (clnt_call(clnt, samrpc_init,
	    (xdrproc_t)xdr_int, (caddr_t)argp,
	    (xdrproc_t)xdr_int, (caddr_t)clnt_res, tm));
}

/*
 * mgmt.h
 */
enum clnt_stat
samrpc_get_samfs_version_1(argp, clnt_res, clnt)
	void *argp;
	samrpc_result_t *clnt_res;
	CLIENT *clnt;
{

	struct timeval tm;
	GET_TIMEOUT(clnt, tm);

	return (clnt_call(clnt, samrpc_get_samfs_version,
	    (xdrproc_t)xdr_ctx_arg_t, (caddr_t)argp,
	    (xdrproc_t)xdr_samrpc_result_t, (caddr_t)clnt_res, tm));
}

enum clnt_stat
samrpc_get_samfs_lib_version_1(argp, clnt_res, clnt)
	void *argp;
	samrpc_result_t *clnt_res;
	CLIENT *clnt;
{

	struct timeval tm;
	GET_TIMEOUT(clnt, tm);

	return (clnt_call(clnt, samrpc_get_samfs_lib_version,
	    (xdrproc_t)xdr_ctx_arg_t, (caddr_t)argp,
	    (xdrproc_t)xdr_samrpc_result_t, (caddr_t)clnt_res, tm));
}

enum clnt_stat
samrpc_init_sam_mgmt_1(argp, clnt_res, clnt)
	void *argp;
	samrpc_result_t *clnt_res;
	CLIENT *clnt;
{

	struct timeval tm;
	GET_TIMEOUT(clnt, tm);

	return (clnt_call(clnt, samrpc_init_sam_mgmt,
	    (xdrproc_t)xdr_ctx_arg_t, (caddr_t)argp,
	    (xdrproc_t)xdr_samrpc_result_t, (caddr_t)clnt_res, tm));
}

enum clnt_stat
samrpc_delete_faults_1(argp, clnt_res, clnt)
	fault_errorid_arr_arg_t *argp;
	samrpc_result_t *clnt_res;
	CLIENT *clnt;
{

	struct timeval tm;
	GET_TIMEOUT(clnt, tm);

	return (clnt_call(clnt, samrpc_delete_faults,
	    (xdrproc_t)xdr_fault_errorid_arr_arg_t, (caddr_t)argp,
	    (xdrproc_t)xdr_samrpc_result_t, (caddr_t)clnt_res, tm));
}

enum clnt_stat
samrpc_ack_faults_1(argp, clnt_res, clnt)
	fault_errorid_arr_arg_t *argp;
	samrpc_result_t *clnt_res;
	CLIENT *clnt;
{

	struct timeval tm;
	GET_TIMEOUT(clnt, tm);

	return (clnt_call(clnt, samrpc_ack_faults,
	    (xdrproc_t)xdr_fault_errorid_arr_arg_t, (caddr_t)argp,
	    (xdrproc_t)xdr_samrpc_result_t, (caddr_t)clnt_res, tm));
}

/*
 * device.h - config APIs
 */
enum clnt_stat
samrpc_discover_avail_aus_1(argp, clnt_res, clnt)
	void *argp;
	samrpc_result_t *clnt_res;
	CLIENT *clnt;
{

	struct timeval tm;
	GET_TIMEOUT(clnt, tm);
	/*
	 * If we have to print the server name to which the request is made
	 *
	 *	struct ct_data *ct = NULL; struct sockaddr_in *in;
	 *	ct = (struct ct_data *)clnt->cl_private;
	 *	in = (struct sockaddr_in *)(ct->ct_addr).buf;
	 *	PTRACE(3, "Request to server [%s]", inet_ntoa(in->sin_addr));
	 */
	return (clnt_call(clnt, samrpc_discover_avail_aus,
	    (xdrproc_t)xdr_ctx_arg_t, (caddr_t)argp,
	    (xdrproc_t)xdr_samrpc_result_t, (caddr_t)clnt_res, tm));
}

enum clnt_stat
samrpc_discover_aus_1(argp, clnt_res, clnt)
	void *argp;
	samrpc_result_t *clnt_res;
	CLIENT *clnt;
{

	struct timeval tm;
	GET_TIMEOUT(clnt, tm);
	/*
	 * If we have to print the server name to which the request is made
	 *
	 *	struct ct_data *ct = NULL; struct sockaddr_in *in;
	 *	ct = (struct ct_data *)clnt->cl_private;
	 *	in = (struct sockaddr_in *)(ct->ct_addr).buf;
	 *	PTRACE(3, "Request to server [%s]", inet_ntoa(in->sin_addr));
	 */
	return (clnt_call(clnt, samrpc_discover_aus,
	    (xdrproc_t)xdr_ctx_arg_t, (caddr_t)argp,
	    (xdrproc_t)xdr_samrpc_result_t, (caddr_t)clnt_res, tm));
}

/*
 * used by client side macros defined in sammgmt.h
 * The following code is put in a function to avoid lint compiler warnings
 * The input arguments are expected to be non-null and valid. Those checks
 * are made in the calling function
 */
void
get_rpc_server(
ctx_t *ctx,
upath_t svr
)
{
	struct ct_data *ct = NULL;
	struct sockaddr_in *in;
	/* LINTED - alignment (32bit) OK */
	ct = (struct ct_data *)ctx->handle->clnt->cl_private;
	/* LINTED - alignment (32bit) OK */
	in = (struct sockaddr_in *)(ct->ct_addr).buf;
	strlcpy(svr, inet_ntoa(in->sin_addr), sizeof (upath_t));
}


/*
 * This function is intended for use only on the client
 */
char *
get_server_version_from_ctx(ctx_t *c) {
	char *ret_val = NULL;
	struct ct_data *ct;
	XDR *xdrs;
	if (c == NULL || c->handle == NULL || c->handle->clnt == NULL) {
		return (NULL);
	}

	/* LINTED - alignment (32bit) OK */
	ct = (struct ct_data *)c->handle->clnt->cl_private;

	if (ct == NULL) {
		return (NULL);
	}
	xdrs = &(ct->ct_xdrs);
	if (xdrs && xdrs->x_public) {
		ret_val = strdup(xdrs->x_public);
	}
	return (ret_val);
}
