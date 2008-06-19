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

#ifndef	_SAMMGMT_H_RPCGEN
#define	_SAMMGMT_H_RPCGEN

#pragma ident	"$Revision: 1.114 $"

#include <stdlib.h>
#include <string.h>
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "pub/mgmt/error.h"
#include "pub/mgmt/types.h"
#include "pub/mgmt/process_job.h"
#include "pub/mgmt/archive.h"
#include "pub/mgmt/device.h"
#include "pub/mgmt/archive_sets.h"
#include "mgmt/config/media.h"
#include "pub/mgmt/filesystem.h"
#include "pub/mgmt/release.h"
#include "pub/mgmt/stage.h"
#include "pub/mgmt/faults.h"
#include "pub/mgmt/diskvols.h"
#include "pub/mgmt/notify_summary.h"
#include "pub/mgmt/job_history.h"
#include "pub/mgmt/hosts.h"
#include "pub/mgmt/report.h"
#include "pub/mgmt/task_schedule.h"
#include "pub/mgmt/monitor.h"
#include "pub/mgmt/csn_registration.h"

#ifdef SAMRPC_SERVER
#include "sam/sam_trace.h"		/* for RPC Server Trace */
#endif

/*
 * ct_data :Private data structure
 * copied from /ws/on10-gate/usr/src/lib/libnsl/rpc/clnt_vc.c
 *
 * backward compatilibilty : to fill in the server version in the
 * XDR x_public
 */
#define	MCALL_MSG_SIZE	24
struct ct_data {
	int		ct_fd;		/* connection's fd */
	bool_t		ct_closeit;	/* close it on destroy */
	int		ct_tsdu;	/* size of tsdu */
	int		ct_wait;	/* wait interval in milliseconds */
	bool_t		ct_waitset;	/* wait set by clnt_control? */
	struct netbuf	ct_addr;	/* remote addr */
	struct rpc_err	ct_error;
	char		ct_mcall[MCALL_MSG_SIZE]; /* marshalled callmsg */
	uint_t		ct_mpos;	/* pos after marshal */
	XDR		ct_xdrs;	/* XDR stream */

	/* NON STANDARD INFO - 00-08-31 */
	bool_t		ct_is_oneway; /* True if the current call is oneway. */
	bool_t		ct_is_blocking;
	ushort_t	ct_io_mode;
	ushort_t	ct_blocking_mode;
	uint_t		ct_bufferSize; /* Total size of the buffer. */
	uint_t		ct_bufferPendingSize; /* Size of unsent data. */
	char		*ct_buffer; /* Pointer to the buffer. */
	char		*ct_bufferWritePtr; /* Ptr to the first free byte. */
	char		*ct_bufferReadPtr; /* Ptr to the first byte of data. */
};

/*
 * *******************************
 *  RPC client/server side macros
 * *******************************
 */
#ifdef	SAMRPC_CLIENT
void get_rpc_server(ctx_t *ctx, upath_t svr);

/* client side macro */
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

#define	SAMRPC_CLNT_CALL(rpc_callname, input) \
	struct timeval tm; GET_TIMEOUT(ctx->handle->clnt, tm); \
	if ((stat = (clnt_call(ctx->handle->clnt, rpc_callname, \
		(xdrproc_t)xdr_ ## input, (caddr_t)&arg, \
		(xdrproc_t)xdr_samrpc_result_t, (caddr_t)&result, tm))) \
		!= RPC_SUCCESS) { \
		SET_RPC_ERROR(ctx, func_name, stat); \
	}

#define	CHECK_CLIENT_HANDLE(ctx, func_name) \
	if (!ctx || !ctx->handle || \
		!(ctx->handle->clnt) || \
		!(ctx->handle->svr_name)) { \
		samerrno = SE_RPC_INVALID_CLIENT_HANDLE; \
		(void) snprintf(samerrmsg, MAX_MSG_LEN, \
			"Invalid client handle"); \
		PTRACE(2, "%s invalid client handle", func_name); \
		return (-1); \
	}

/* not used */
#define	CHECK_RPC_ERROR(result, func_name) \
	if (result == (samrpc_result_t *)NULL) { \
		samerrno = SE_RPC_FAILED; \
		err_msg = clnt_sperror(ctx->handle->clnt, \
			ctx->handle->svr_name); \
		(void) snprintf(samerrmsg, MAX_MSG_LEN, err_msg); \
		PTRACE(2, "%s %s", func_name, err_msg); \
		return (-1); \
	}
/* not used */
#define	SET_MEM_ERROR(ctx, func_name) \
	{ \
		samerrno = SE_NO_MEM; \
		(void) snprintf(samerrmsg, MAX_MSG_LEN, \
			"Out of memory [err = %d]", samerrno); \
		PTRACE(2, "%s %s", func_name, samerrmsg); \
		return (-1); \
	}

/*
 * Checking for RPC communication type errors
 * when a clnt_call is made to the server
 * Client may want to handle RPC_TIMEDOUT errors
 * differently from the transport related errors
 *
 * The NETWORK_DOWN and RPC_TIMEDOUT strings will be localized by GUI
 * These are put in temporarily
 */
#define	NETWORK_DOWN_STR "Network is unreachable. Please try after some time"
#define	RPC_TIMEDOUT_STR "Request is taking a long time to complete. Timed out"
#define	RPC_COMM_ERR \
	"There was a problem while trying to communicate with the SAM-FS/QFS " \
	"server. Please retry your request"

/* ctx->handle->svr_name is used to store the architecture */
#define	SET_RPC_ERROR(ctx, func_name, stat) \
	{ upath_t svr = {0}; \
		get_rpc_server(ctx, svr); \
		PTRACE(2, "%s: rpc call to server[%s] failed. stat = %d\n", \
			func_name, svr, stat); \
		if (stat == RPC_TIMEDOUT) { \
			if (nw_down(svr) != 0) { \
				samerrno = SE_NETWORK_DOWN; \
				(void) snprintf(samerrmsg, MAX_MSG_LEN, \
					NETWORK_DOWN_STR); \
			} else { \
				samerrno = SE_RPC_TIMEDOUT; \
				(void) snprintf(samerrmsg, MAX_MSG_LEN, \
					RPC_TIMEDOUT_STR); \
			} \
		} else if (stat == RPC_PROCUNAVAIL) { \
			samerrno = SE_PROC_UNAVAILABLE; \
			err_msg = clnt_sperror(ctx->handle->clnt, svr); \
			(void) snprintf(samerrmsg, MAX_MSG_LEN, err_msg); \
		} else if (stat == RPC_CANTRECV) { \
			samerrno = SE_RPC_FAILED; \
			(void) snprintf(samerrmsg, MAX_MSG_LEN, RPC_COMM_ERR); \
		} else { \
			samerrno = SE_RPC_FAILED; \
			err_msg = clnt_sperror(ctx->handle->clnt, svr); \
			(void) snprintf(samerrmsg, MAX_MSG_LEN, err_msg); \
		} \
		PTRACE(2, "%s %s", func_name, samerrmsg); \
		return (-1); \
	}

/*
 * return code from api will be 0, -1, -2
 *
 * -2 is a warning, suggesting that not all data is correct
 * -1 is error
 */
#define	CHECK_FUNCTION_FAILURE(result, func_name) \
	PTRACE(2, "%s %d", func_name, result.status); \
	switch (result.status) { \
	case -1: \
		samerrno = result.samrpc_result_u.err.errcode; \
		err_msg = result.samrpc_result_u.err.errmsg; \
		(void) snprintf(samerrmsg, MAX_MSG_LEN, err_msg); \
		PTRACE(2, "%s: %s %s", (ctx->handle->svr_name != NULL) ? \
			ctx->handle->svr_name : "NULL", func_name, err_msg); \
		ret_val = result.status; \
		return (ret_val); \
	case -2: \
	case -3: \
		samerrno = result.samrpc_result_u.err.errcode; \
		err_msg = result.samrpc_result_u.err.errmsg; \
		(void) snprintf(samerrmsg, MAX_MSG_LEN, err_msg); \
		PTRACE(2, "%s %s", func_name, err_msg); \
		/* FALLTHRU */ \
	default: \
		if (result.svr_timestamp != SAMRPC_TIMESTAMP_NO_UPDATE) { \
			ctx->handle->timestamp = result.svr_timestamp; \
		} \
	}

#endif

/*
 * Rebuild tail
 *
 */
#define	SET_LIST_TAIL(lst) \
	{ node_t *prev, *cur; \
		if (lst != NULL) { \
			cur = lst->head; \
			prev = lst->head; \
			while (cur != NULL) { \
				prev = cur; \
				cur = cur->next; \
			} \
			lst->tail = prev; \
		} \
	}

/*
 * Set the tail of all lists in fs_t
 */
#define	SET_FS_ARCH_CFG_TAIL(afc) \
	{ node_t *node_s; vsn_map_t *_mp; \
		SET_LIST_TAIL(afc->copies); \
		SET_LIST_TAIL(afc->vsn_maps); \
		if (afc->vsn_maps != NULL) { \
			node_s = afc->vsn_maps->head; \
			while (node_s != NULL) { \
				_mp = (vsn_map_t *)node_s->data; \
				SET_LIST_TAIL(_mp->vsn_names); \
				SET_LIST_TAIL(_mp->vsn_pool_names); \
				node_s = node_s->next; \
			} \
		} \
	}

/*
 * Set the tail of all lists in fs_t
 */
#define	SET_FS_TAIL(fs) \
	{ node_t *node_s; striped_group_t *striped; \
		SET_LIST_TAIL(fs->meta_data_disk_list); \
		SET_LIST_TAIL(fs->data_disk_list); \
		SET_LIST_TAIL(fs->striped_group_list); \
		if (fs->striped_group_list != NULL) { \
			node_s = fs->striped_group_list->head; \
			while (node_s != NULL) { \
				striped = (striped_group_t *)node_s->data; \
				SET_LIST_TAIL(striped->disk_list); \
				node_s = node_s->next; \
			} \
		} \
	}


#ifdef	SAMRPC_SERVER

/* server side macro */
#define	SAMRPC_CHECK_TIMESTAMP(ts) \
	if (ts != server_timestamp) { \
		samerrno = SE_RPC_OUT_OF_SYNC; \
		(void) snprintf(samerrmsg, MAX_MSG_LEN, \
		    "Another user has modified the configuration. "\
		    "If you wish to continue, please retry the operation."); \
		SAMRPC_SET_RESULT(ret, SAM_VOID, 0); \
		return (&rpc_result); \
	}

/* update server timestamp */

#define	SAMRPC_UPDATE_TIMESTAMP(ret) \
	if (ret != -1) { \
		if (server_timestamp < INTMAX_MAX) { \
			++server_timestamp; \
		} else { \
			server_timestamp = 0; \
		} \
		Trace(TR_MISC, "rpc:Tstamp updated [%d]", server_timestamp); \
		timestamp_updated = B_TRUE; \
	}

/*
 * return code from api will be 0, -1, -2
 * -2 is a warning, suggesting that not all data is correct
 * -1 is error
 *
 * server timestamp is sent only if ret is success and if server timestamp
 * has been updated by this client, so that this client and the server
 * can be in sync
 */
#define	SAMRPC_SET_RESULT(ret, res_type, res_data) \
	rpc_result.status = ret; \
	switch (ret) { \
	case -1: \
		rpc_result.samrpc_result_u.err.errcode = samerrno; \
		(void) strcpy(rpc_result.samrpc_result_u.err.errmsg,\
		    samerrmsg);\
		Trace(TR_DEBUG, "[%s]", samerrmsg); \
		break; \
	case -2: \
	case -3: \
		rpc_result.samrpc_result_u.err.errcode = samerrno; \
		(void) strcpy(rpc_result.samrpc_result_u.err.errmsg,\
		    samerrmsg);\
		Trace(TR_DEBUG, "[%s]", samerrmsg); \
		/* FALLTHRU */ \
	default: \
		if (timestamp_updated == B_TRUE) { \
			rpc_result.svr_timestamp = server_timestamp; \
			timestamp_updated = B_FALSE; \
		} else { \
			rpc_result.svr_timestamp = SAMRPC_TIMESTAMP_NO_UPDATE; \
		} \
		rpc_result.samrpc_result_u.result.result_type = res_type; \
		rpc_result.samrpc_result_u.result.result_data = res_data; \
	}

#define	SAMRPC_NOT_YET() \
	{ \
		samerrno = SE_RPC_NOT_YET; \
		(void) snprintf(samerrmsg, MAX_MSG_LEN, \
			"function not implemented yet"); \
		Trace(TR_MISC, "rpc:function not implemented yet"); \
		SAMRPC_SET_RESULT(ret, SAM_VOID, 0); \
		return (&rpc_result); \
	}

#endif


/*
 *  RPC definitions
 */

/*
 * A remote procedure is uniquely identified by a program number, version
 * number and procedure number.
 *
 * The program number identifies a group of related remote procedures, each of
 * which has a unique procedure number. Version numbers enable multiple versions
 * of an RPC protocol to be available simultaneously
 *
 * Program numbers 20000000 - 3fffffff are defined by user for specific
 * customer applications.
 */
#define	SAMMGMTPROG	0x20000001
#define	SAMMGMTVERS	1
extern int sammgmtprog_1_freeresult();


/*
 * RPC result structures
 *
 */

typedef enum samstruct_type {
	SAM_VOID = 0,
	/* mgmt.h */
	SAM_MGMT_STRING,
	/* archive.h */
	SAM_AR_SET_COPY_CFG,
	SAM_AR_SET_CRITERIA,
	SAM_AR_GLOBAL_DIRECTIVE,
	SAM_AR_SET_CRITERIA_LIST,
	SAM_AR_FS_DIRECTIVE,
	SAM_AR_FS_DIRECTIVE_LIST,
	SAM_AR_NAMES_LIST,
	SAM_AR_SET_COPY_PARAMS_LIST,
	SAM_AR_SET_COPY_PARAMS,
	SAM_AR_VSN_POOL_LIST,
	SAM_AR_VSN_POOL,
	SAM_AR_VSN_MAP_LIST,
	SAM_AR_VSN_MAP,
	SAM_AR_DRIVE_DIRECTIVE,
	SAM_AR_BUFFER_DIRECTIVE,
	SAM_AR_ARCHIVERD_STATE,
	SAM_AR_ARFIND_STATE_LIST,
	SAM_AR_ARFIND_STATE,
	SAM_AR_ARCHREQ_LIST,
	SAM_AR_POOL_USED,

	/* device.h */
	SAM_DEV_AU_LIST,
	SAM_DEV_DISCOVER_MEDIA_RESULT,
	SAM_DEV_LIBRARY_LIST,
	SAM_DEV_LIBRARY,
	SAM_DEV_DRIVE_LIST,
	SAM_DEV_DRIVE,
	SAM_DEV_CATALOG_ENTRY_LIST,
	SAM_DEV_CATALOG_ENTRY,
	SAM_DEV_TP_LABEL_LIST,
	SAM_DEV_MEDIA_TYPE_LIST,
	SAM_DEV_VSNPOOL_PROPERTY,
	SAM_DEV_STRING_LIST,
	SAM_DEV_SPACE,

	/* filesystem.h */
	SAM_FS_FS_LIST,
	SAM_FS_STRING_LIST,
	SAM_FS_FS,
	SAM_FS_MOUNT_OPTIONS,
	SAM_FS_FSCK_LIST,
	SAM_FS_FAILED_MOUNT_OPTS_LIST,

	/* license.h */
	SAM_LIC_STRING_LIST,

	/* load.h */
	SAM_LD_FS_LOAD_PRIORITY,
	SAM_LD_FS_LOAD_PRIORITY_LIST,
	SAM_LD_PENDING_LOAD_INFO_LIST,

	/* recycle.h */
	SAM_RC_STRING,
	SAM_RC_NO_RC_VSNS_LIST,
	SAM_RC_NO_RC_VSNS,
	SAM_RC_ROBOT_CFG_LIST,
	SAM_RC_ROBOT_CFG,
	SAM_RC_PARAM,

	/* release.h */
	SAM_RL_FS_DIRECTIVE_LIST,
	SAM_RL_FS_DIRECTIVE,
	SAM_RL_FS_LIST,

	/* stage.h */
	SAM_ST_STAGER_CFG,
	SAM_ST_DRIVE_DIRECTIVE,
	SAM_ST_BUFFER_DIRECTIVE,
	SAM_ST_STAGER_INFO,
	SAM_ST_STAGING_FILE_INFO_LIST,
	SAM_ST_STAGING_FILE_INFO,

	/* faults.h */
	SAM_FAULTS_LIST,
	SAM_FAULTS_SUMMARY,

	/* diskvols.h */
	SAM_DISKVOL,
	SAM_DISKVOL_LIST,
	SAM_CLIENT_LIST,

	/* notify_summary.h */
	SAM_NOTIFY_SUMMARY_LIST,

	/* job history */
	SAM_JOB_HISTORY,
	SAM_JOB_HISTORY_LIST,

	SAM_PTR_BOOL,
	SAM_PTR_INT,
	SAM_PTR_SIZE,

	/*
	 * Backward compatibilty
	 * Please add the result types to the end of this list
	 * for 4.3
	 */
	SAM_LIC_INFO,
	SAM_HOST_INFO_LIST,
	SAM_INT_LIST_RESULT,
	SAM_STRING_LIST,
	SAM_AR_ARCH_SET_LIST,
	SAM_AR_ARCH_SET,
	SAM_INT_LIST,
	SAM_INT_ARRPTRS,
	SAM_DISK_VSNPOOL_PROPERTY,
	SAM_STK_VSN_LIST,
	SAM_STK_LSM_LIST,
	SAM_STK_PANEL_LIST,
	SAM_STK_POOL_LIST,
	SAM_STK_CELL_INFO,
	SAM_STK_PHYCONF_INFO,
	SAM_STK_INFO_LIST,
	SAM_FILE_DETAILS,
	SAM_PUBLIC_KEY
} samstruct_type_t;

typedef struct result_struct {
	samstruct_type_t	result_type;
	void			   *result_data;
} result_struct_t;

/*
 * if the status is -1, only send the err
 * if the status is 0, only send the result
 * for return values of -2 or -3, set both err and result
 */
typedef struct samrpc_result {
	int status;
	int svr_timestamp;
	struct {
		err_struct_t err;
		result_struct_t result;
	} samrpc_result_u;
} samrpc_result_t;


/*
 * ***************************************
 *  the RPC function parameter structures
 * ***************************************
 */

#define	SAMRPC_TIMESTAMP_NO_CHECK	-1
#define	SAMRPC_TIMESTAMP_NO_UPDATE	-1

/* mgmt.h */
typedef struct proc_arg {
	ctx_t *ctx;
	pid_t pid;
	proctype_t ptype;
} proc_arg_t;

/* archive.h */

typedef struct ar_global_directive_arg {
	ctx_t *ctx;
	ar_global_directive_t *ar_global;
} ar_global_directive_arg_t;

typedef struct ctx_arg {
	ctx_t *ctx;
} ctx_arg_t;

typedef struct string_arg {
	ctx_t *ctx;
	char *str;
} string_arg_t;

typedef struct string_string_arg {
	ctx_t *ctx;
	char *str1;
	char *str2;
} string_string_arg_t;

typedef struct string_string_int_arg {
	ctx_t *ctx;
	char *str1;
	char *str2;
	int int1;
} string_string_int_arg_t;

typedef struct string_string_int_int_arg {
	ctx_t *ctx;
	char *str1;
	char *str2;
	int int1;
	int int2;
} string_string_int_int_arg_t;

typedef struct string_string_int_disk_arg {
	ctx_t *ctx;
	char *str1;
	char *str2;
	int int1;
	disk_t *dsk;
} string_string_int_disk_arg_t;

typedef struct string_string_int_group_arg {
	ctx_t *ctx;
	char *str1;
	char *str2;
	int int1;
	striped_group_t *grp;
} string_string_int_group_arg_t;

typedef struct int_int_arg {
	ctx_t *ctx;
	int int1;
	int int2;
} int_int_arg_t;

typedef struct string_sort_arg {
	ctx_t *ctx;
	char *str;
	int start;
	int size;
	vsn_sort_key_t sort_key;
	boolean_t ascending;
} string_sort_arg_t;

typedef struct vsnpool_arg {
	ctx_t *ctx;
	vsn_pool_t *pool;
	int start;
	int size;
	vsn_sort_key_t sort_key;
	boolean_t ascending;
} vsnpool_arg_t;

typedef struct vsnmap_arg {
	ctx_t *ctx;
	vsn_map_t *map;
	int start;
	int size;
	vsn_sort_key_t sort_key;
	boolean_t ascending;
} vsnmap_arg_t;

typedef struct string_uint32_arg {
	ctx_t *ctx;
	char *str;
	uint32_t u_flag;
} string_uint32_arg_t;

typedef struct int_uint32_arg {
	ctx_t *ctx;
	int i;
	uint32_t u_flag;
} int_uint32_arg_t;

typedef struct string_uint32_uint32_arg {
	ctx_t *ctx;
	char *str;
	uint32_t u_1;
	uint32_t u_2;
} string_uint32_uint32_arg_t;

typedef struct equ_sort_arg {
	ctx_t *ctx;
	equ_t eq;
	int start;
	int size;
	vsn_sort_key_t sort_key;
	boolean_t ascending;
} equ_sort_arg_t;

typedef struct string_bool_arg {
	ctx_t *ctx;
	char *str;
	boolean_t bool;
} string_bool_arg_t;

typedef struct string_string_bool_arg {
	ctx_t *ctx;
	char *str1;
	char *str2;
	boolean_t bool;
} string_string_bool_arg_t;

typedef struct ar_fs_directive_arg {
	ctx_t *ctx;
	ar_fs_directive_t *fs_directive;
} ar_fs_directive_arg_t;

typedef struct ar_set_criteria_arg {
	ctx_t *ctx;
	ar_set_criteria_t *crit;
} ar_set_criteria_arg_t;

typedef struct ar_set_copy_params_arg {
	ctx_t *ctx;
	ar_set_copy_params_t *copy_params;
} ar_set_copy_params_arg_t;

typedef struct vsn_pool_arg {
	ctx_t *ctx;
	vsn_pool_t *pool;
} vsn_pool_arg_t;

typedef struct vsn_map_arg {
	ctx_t *ctx;
	vsn_map_t *map;
} vsn_map_arg_t;

typedef struct string_list_arg {
	ctx_t *ctx;
	sqm_lst_t *strings;
} string_list_arg_t;

typedef struct write_temp_config_archive_arg {
	ctx_t *ctx;
	ar_global_directive_t *ar_global;
	sqm_lst_t *ar_fs_directive_list;
	sqm_lst_t *ar_set_copy_params_list;
	sqm_lst_t *vsn_pools_list;
	char *file_location_prefix;
} write_temp_config_archive_arg_t;

typedef struct pool_use_result {
	boolean_t in_use;
	uname_t used_by;
} pool_use_result_t;


typedef struct strlst_int32_arg {
	ctx_t *ctx;
	sqm_lst_t *strlst;
	int32_t int32;
} strlst_int32_arg_t;

/* arch_set.h */

typedef struct arch_set_arg {
	ctx_t *ctx;
	arch_set_t *set;
} arch_set_arg_t;

typedef struct str_critlst_arg {
	ctx_t *ctx;
	char *str;
	sqm_lst_t *critlst; /* list of ar_set_criteria */
} str_critlst_arg_t;

/* device.h */
typedef char dismes_t[DIS_MES_LEN + 1];
typedef char wwn_id_t[WWN_LENGTH + 1];

typedef struct equ_arg {
	ctx_t *ctx;
	equ_t eq;
} equ_arg_t;

typedef struct equ_bool_arg {
	ctx_t *ctx;
	equ_t eq;
	boolean_t bool;
} equ_bool_arg_t;

typedef struct equ_equ_bool_arg {
	ctx_t *ctx;
	equ_t eq1;
	equ_t eq2;
	boolean_t bool;
} equ_equ_bool_arg_t;

typedef struct au_type_arg {
	ctx_t *ctx;
	au_type_t type;
} au_type_arg_t;

typedef struct strlst_bool_arg {
	ctx_t *ctx;
	sqm_lst_t *strlst;
	boolean_t bool;
} strlst_bool_arg_t;

typedef struct discover_media_result {
	sqm_lst_t *library_list;
	sqm_lst_t *remaining_drive_list;
} discover_media_result_t;


/* lst of int* */
typedef struct list_arg {
	ctx_t *ctx;
	sqm_lst_t *lst;
} list_arg_t;

typedef struct library_arg {
	ctx_t *ctx;
	library_t *lib;
} library_arg_t;

typedef struct library_list_arg {
	ctx_t *ctx;
	sqm_lst_t *library_lst;
} library_list_arg_t;

typedef struct drive_arg {
	ctx_t *ctx;
	drive_t *drive;
} drive_arg_t;

typedef struct equ_slot_part_bool_arg {
	ctx_t *ctx;
	equ_t eq;
	int slot;
	int partition;
	boolean_t bool;
} equ_slot_part_bool_arg_t;

typedef struct chmed_arg {
	ctx_t *ctx;
	equ_t eq;
	int slot;
	boolean_t set;
	uint32_t mask;
} chmed_arg_t;

typedef struct import_arg {
	ctx_t *ctx;
	equ_t eq;
	import_option_t *options;
} import_arg_t;

typedef struct import_vsns_arg {
	ctx_t *ctx;
	equ_t eq;
	import_option_t *options;
	sqm_lst_t *vsn_list;
} import_vsns_arg_t;

typedef struct equ_slot_bool_arg {
	ctx_t *ctx;
	equ_t eq;
	int slot;
	boolean_t bool;
} equ_slot_bool_arg_t;

typedef struct move_arg {
	ctx_t *ctx;
	equ_t eq;
	int slot;
	int dest_slot;
} move_arg_t;

typedef struct tplabel_arg {
	ctx_t *ctx;
	equ_t eq;
	int slot;
	int partition;
	vsn_t new_vsn;
	char *old_vsn;
	uint_t blksize;
	boolean_t wait;
	boolean_t erase;
} tplabel_arg_t;

typedef struct equ_slot_part_arg {
	ctx_t *ctx;
	equ_t eq;
	int slot;
	int partition;
} equ_slot_part_arg_t;

typedef struct equ_dstate_arg {
	ctx_t *ctx;
	equ_t eq;
	dstate_t state;
} equ_dstate_arg_t;

typedef struct nwlib_req_info_arg {
	ctx_t *ctx;
	nwlib_req_info_t *nwlib_req_info;
} nwlib_req_info_arg_t;

typedef struct string_mtype_arg {
	ctx_t *ctx;
	char *str;
	char *type;
} string_mtype_arg_t;

typedef struct string_equ_arg {
	ctx_t *ctx;
	char *str;
	equ_t eq;
} string_equ_arg_t;

typedef struct equ_range_arg {
	ctx_t *ctx;
	equ_t eq;
	int start_slot_no;
	int end_slot_no;
	vsn_sort_key_t sort_key;
	boolean_t ascending;
} equ_range_arg_t;

typedef struct reserve_arg {
	ctx_t *ctx;
	equ_t eq;
	int slot;
	int partition;
	reserve_option_t *reserve_option;
} reserve_arg_t;

typedef struct import_range_arg {
	ctx_t *ctx;
	vsn_t begin_vsn;
	vsn_t end_vsn;
	equ_t eq;
	import_option_t *options;
} import_range_arg_t;

typedef struct stk_host_list_arg {
	ctx_t *ctx;
	sqm_lst_t *stk_host_lst;
} stk_host_list_arg_t;

typedef struct stk_host_info_arg {
	ctx_t *ctx;
	stk_host_info_t *stk_host_info;
} stk_host_info_arg_t;

typedef struct stk_display_info_arg {
	ctx_t *ctx;
	stk_host_info_t *stk_host_info;
	stk_info_type_t type;
	char *user_time;
	char *time_type;
} stk_display_info_arg_t;

typedef struct stk_host_info_string_arg {
	ctx_t *ctx;
	stk_host_info_t *stk_host_info;
	char *str;
} stk_host_info_string_arg_t;

/* filesystem.h */
typedef struct create_fs_arg {
	ctx_t *ctx;
	fs_t *fs_info;
	boolean_t modify_vfstab;
} create_fs_arg_t;

typedef struct change_mount_options_arg {
	ctx_t *ctx;
	uname_t fsname;
	mount_options_t *options;
} change_mount_options_arg_t;

typedef struct get_default_mount_options_arg {
	ctx_t *ctx;
	devtype_t fs_type;
	int dau_size;
	boolean_t uses_stripe_groups;
	boolean_t shared;
	boolean_t multi_reader;
} get_default_mount_options_arg_t;

typedef struct fsck_fs_arg {
	ctx_t *ctx;
	uname_t fsname;
	upath_t logfile;
	boolean_t repair;
} fsck_fs_arg_t;

typedef struct mount_fs_arg {
	ctx_t *ctx;
	uname_t fsname;
	sqm_lst_t *mnt_options_lst;
} mount_fs_arg_t;

typedef struct grow_fs_arg {
	ctx_t *ctx;
	fs_t *fs;
	sqm_lst_t *additional_meta_data_disk;
	sqm_lst_t *additional_data_disk;
	sqm_lst_t *additional_striped_group;
} grow_fs_arg_t;

typedef struct create_fs_mount_arg {
	ctx_t *ctx;
	fs_t *fs_info;
	boolean_t mount_at_boot;
	boolean_t create_mnt_point;
	boolean_t mount;
} create_fs_mount_arg_t;

typedef struct create_arch_fs_arg {
	ctx_t *ctx;
	fs_t *fs_info;
	boolean_t mount_at_boot;
	boolean_t create_mnt_point;
	boolean_t mount;
	fs_arch_cfg_t *arch_cfg;
} create_arch_fs_arg_t;

typedef struct int_list_arg {
	ctx_t *ctx;
	int num;
	sqm_lst_t *lst;	/* lst of ints */
} int_list_arg_t;

typedef struct string_int_intlist_arg {
	ctx_t *ctx;
	char *str;
	int num;
	sqm_lst_t *int_lst;	/* lst of ints */
} string_int_intlist_arg_t;

typedef struct int_list_result {
	sqm_lst_t *lst;
	int *first_free;
} int_list_result_t;

typedef struct intlist_arg {
	ctx_t *ctx;
	sqm_lst_t *lst; /* lst of ints */
} intlist_arg_t;

/*
 * int_ptrarr_result_t can be used only to represent an array made of offsets
 * into a malloc()ed blob of data. It cannot be used to pass an array of
 * individually malloc()ed strings.
 */
typedef struct int_ptrarr_result {
	char **pstr;
	uint32_t count;
	char *str; /* only used by the free routine */
} int_ptrarr_result_t;

typedef struct reset_eq_arg {
	ctx_t *ctx;
	char *str;
	sqm_lst_t *lst;
} reset_eq_arg_t;

/* load.h */
typedef struct clear_load_request_arg {
	ctx_t *ctx;
	vsn_t vsn;
	int index;
} clear_load_request_arg_t;

/* recycle.h */
typedef struct no_rc_vsns_arg {
	ctx_t *ctx;
	no_rc_vsns_t *no_recycle_vsns;
} no_rc_vsns_arg_t;

typedef struct rc_robot_cfg_arg {
	ctx_t *ctx;
	rc_robot_cfg_t *rc_robot_cfg;
} rc_robot_cfg_arg_t;

typedef struct rc_upath_arg {
	ctx_t *ctx;
	upath_t path;
} rc_upath_arg_t;


/* release.h */
typedef struct rl_fs_directive_arg {
	ctx_t *ctx;
	rl_fs_directive_t *rl_fs_directive;
} rl_fs_directive_arg_t;

typedef struct strlst_int32_int32_arg {
	ctx_t *ctx;
	sqm_lst_t *strlst;
	int32_t int1;
	int32_t int2;
} strlst_int32_int32_arg_t;

/* stage.h */
typedef struct set_stager_cfg_arg {
	ctx_t *ctx;
	stager_cfg_t *stager_config;
} set_stager_cfg_arg_t;

typedef struct drive_directive_arg {
	ctx_t *ctx;
	drive_directive_t *stage_drive;
} drive_directive_arg_t;

typedef struct buffer_directive_arg {
	ctx_t *ctx;
	buffer_directive_t *stage_buffer;
} buffer_directive_arg_t;

typedef struct stager_stream_arg {
	ctx_t *ctx;
	stager_stream_t *stream;
	st_sort_key_t sort_key;
	boolean_t ascending;
} stager_stream_arg_t;

typedef struct stream_arg {
	ctx_t *ctx;
	stager_stream_t *stream;
} stream_arg_t;

typedef struct stager_stream_range_arg {
	ctx_t *ctx;
	stager_stream_t *stream;
	int start;
	int size;
	st_sort_key_t sort_key;
	boolean_t ascending;
} stager_stream_range_arg_t;

typedef struct stage_arg {
	ctx_t *ctx;
	sqm_lst_t *file_or_dirs;
	boolean_t recursive;
} stage_arg_t;

typedef struct clear_stage_request_arg {
	ctx_t *ctx;
	mtype_t media;
	vsn_t vsn;
} clear_stage_request_arg_t;

typedef struct range_arg {
	ctx_t *ctx;
	int start;
	int size;
} range_arg_t;

typedef struct staging_file_arg {
	ctx_t *ctx;
	upath_t fname;
	char *vsn;
} staging_file_arg_t;

typedef struct write_temp_config_stage_arg {
	ctx_t *ctx;
	stager_cfg_t *stager_cfg;
	char *file_location_prefix;
} write_temp_config_stage_arg_t;

/* faults.h */
typedef struct fault_req_arg {
	ctx_t *ctx;
	flt_req_t fault_req;
} fault_req_arg_t;

/* fault_update_arg is deprecated in API_VERSION 1.5.0 */

typedef struct fault_errorid_arr_arg {
	ctx_t *ctx;
	int num;
	long errorID[DEFAULTS_MAX]; /* max 700 fault can be ack/del at a time */
} fault_errorid_arr_arg_t;

/* diskvols.h */
typedef struct disk_vol_arg {
	ctx_t *ctx;
	disk_vol_t *disk_vol;
} disk_vol_arg_t;

/* notify_summary.h */
typedef struct notify_summary_arg {
	ctx_t *ctx;
	notf_summary_t *notf_summ;
} notify_summary_arg_t;

typedef struct mod_notify_summary_arg {
	ctx_t *ctx;
	upath_t oldemail;
	notf_summary_t *notf_summ;
} mod_notify_summary_arg_t;

typedef struct get_email_addrs_arg {
	ctx_t	*ctx;
	notf_subj_t	subj_wanted;
} get_email_addrs_arg_t;

/* job_history.h */
typedef struct job_type_arg {
	ctx_t *ctx;
	job_type_t job_type;
} job_type_arg_t;

typedef struct job_hist_arg {
	ctx_t *ctx;
	job_type_t job_type;
	uname_t fsname;
} job_hist_arg_t;


typedef struct string_host_info_arg {
	ctx_t *ctx;
	char *fs_name;
	host_info_t *h;
} string_host_info_arg_t;

/* samfsrestore and samfsdump */

typedef struct strlst_intlst_arg {
	ctx_t *ctx;
	sqm_lst_t *intlst;
	sqm_lst_t *strlst;
} strlst_intlst_arg_t;

typedef struct file_restrictions_arg {
	ctx_t *ctx;
	char *fsname;
	char *dumppath;
	int maxentries;
	char *filepath;
	char *restrictions;
} file_restrictions_arg_t;

typedef struct file_restrictions_more_arg {
	ctx_t *ctx;
	char *fsname;
	char *dumppath;
	int maxentries;
	char *dir;
	char *file;
	char *restrictions;
	uint32_t *morefiles;
} file_restrictions_more_arg_t;

typedef struct strlst_arg {
	ctx_t *ctx;
	sqm_lst_t *lst;
} strlst_arg_t;

typedef struct restore_inodes_arg {
	ctx_t *ctx;
	char *fsname;
	char *dumppath;
	sqm_lst_t *filepaths;
	sqm_lst_t *destinations;
	sqm_lst_t *copies;
	replace_t replace;
} restore_inodes_arg_t;

typedef struct string_strlst_arg {
	ctx_t *ctx;
	char *str;
	sqm_lst_t *strlst;
} string_strlst_arg_t;

typedef struct string_string_strlst_arg {
	ctx_t *ctx;
	char *str;
	char *str2;
	sqm_lst_t *strlst;
} string_string_strlst_arg_t;

typedef struct version_details_arg {
	ctx_t *ctx;
	char *fsname;
	char *dumpname;
	char *filepath;
} version_details_arg_t;

typedef struct int_string_arg {
	ctx_t *ctx;
	int i;
	char *str;
} int_string_arg_t;

typedef struct strlst_intlst_intlst_arg {
	ctx_t *ctx;
	sqm_lst_t *intlst1;
	sqm_lst_t *intlst2;
	sqm_lst_t *strlst;
} strlst_intlst_intlst_arg_t;


typedef struct strlst_uint32_arg {
	ctx_t	*ctx;
	sqm_lst_t	*strlst;
	uint32_t u32;
} strlst_uint32_arg_t;

/* report */
typedef struct report_requirement_arg {
	ctx_t *ctx;
	report_requirement_t *req;
} report_requirement_arg_t;

typedef struct file_metric_rpt_arg {
	ctx_t	*ctx;
	char	*str;
	enum_t	which;
	time_t	start;
	time_t	end;
} file_metric_rpt_arg_t;


/* Registration and telemetry args */
typedef struct cns_reg_arg {
	ctx_t *ctx;
	char *kv;
	crypt_str_t *cl_pwd;
	crypt_str_t *proxy_pwd;
	char *hex;
} cns_reg_arg_t;

typedef struct public_key_result {
	char *pub_key_hex;
	crypt_str_t *signature;
} public_key_result_t;


/* file utils */
typedef struct file_details_arg {
	ctx_t *ctx;
	char *fsname;
	char *snap;
	char *dir;
	char *file;
	uint32_t u32;
	int howmany;
	char *restrictions;
	sqm_lst_t *files;
} file_details_arg_t;

typedef struct file_details_result {
	uint32_t more;
	sqm_lst_t	*list;
} file_details_result_t;

/*
 * *****************************
 *  the RPC function prototypes
 * *****************************
 */

#define	samrpc_init	99
extern  enum clnt_stat samrpc_init_1();
extern  int *samrpc_init_1_svr();

/* archive.h - config APIs */
#define	samrpc_get_default_ar_set_copy_cfg	100
extern  samrpc_result_t *samrpc_get_default_ar_set_copy_cfg_1_svr();

#define	samrpc_get_default_ar_set_criteria	101
extern  samrpc_result_t *samrpc_get_default_ar_set_criteria_1_svr();

#define	samrpc_get_ar_global_directive	102
extern  samrpc_result_t *samrpc_get_ar_global_directive_1_svr();

#define	samrpc_set_ar_global_directive	103
extern  samrpc_result_t *samrpc_set_ar_global_directive_1_svr();

#define	samrpc_get_default_ar_global_directive	104
extern  samrpc_result_t *samrpc_get_default_ar_global_directive_1_svr();

#define	samrpc_get_ar_set_criteria_names	105
extern  samrpc_result_t *samrpc_get_ar_set_criteria_names_1_svr();

#define	samrpc_get_ar_set	106
extern  samrpc_result_t *samrpc_get_ar_set_1_svr();

#define	samrpc_get_ar_set_criteria_list	107
extern  samrpc_result_t *samrpc_get_ar_set_criteria_list_1_svr();

#define	samrpc_get_all_ar_set_criteria	108
extern  samrpc_result_t *samrpc_get_all_ar_set_criteria_1_svr();

#define	samrpc_get_ar_fs_directive	109
extern  samrpc_result_t *samrpc_get_ar_fs_directive_1_svr();

#define	samrpc_set_ar_fs_directive	110
extern  samrpc_result_t *samrpc_set_ar_fs_directive_1_svr();

#define	samrpc_reset_ar_fs_directive	111
extern  samrpc_result_t *samrpc_reset_ar_fs_directive_1_svr();

#define	samrpc_modify_ar_set_criteria	112
extern  samrpc_result_t *samrpc_modify_ar_set_criteria_1_svr();

#define	samrpc_get_default_ar_fs_directive	113
extern  samrpc_result_t *samrpc_get_default_ar_fs_directive_1_svr();

#define	samrpc_get_all_ar_set_copy_params	115
extern  samrpc_result_t *samrpc_get_all_ar_set_copy_params_1_svr();

#define	samrpc_get_ar_set_copy_params_names	116
extern  samrpc_result_t *samrpc_get_ar_set_copy_params_names_1_svr();

#define	samrpc_get_ar_set_copy_params	117
extern  samrpc_result_t *samrpc_get_ar_set_copy_params_1_svr();

#define	samrpc_get_ar_set_copy_params_for_ar_set	118
extern  samrpc_result_t *samrpc_get_ar_set_copy_params_for_ar_set_1_svr();

#define	samrpc_set_ar_set_copy_params	119
extern  samrpc_result_t *samrpc_set_ar_set_copy_params_1_svr();

#define	samrpc_reset_ar_set_copy_params	120
extern  samrpc_result_t *samrpc_reset_ar_set_copy_params_1_svr();

#define	samrpc_get_default_ar_set_copy_params	121
extern  samrpc_result_t *samrpc_get_default_ar_set_copy_params_1_svr();

#define	samrpc_get_all_vsn_pools	122
extern  samrpc_result_t *samrpc_get_all_vsn_pools_1_svr();

#define	samrpc_get_vsn_pool	123
extern  samrpc_result_t *samrpc_get_vsn_pool_1_svr();

#define	samrpc_add_vsn_pool	124
extern  samrpc_result_t *samrpc_add_vsn_pool_1_svr();

#define	samrpc_modify_vsn_pool	125
extern  samrpc_result_t *samrpc_modify_vsn_pool_1_svr();

#define	samrpc_remove_vsn_pool	126
extern  samrpc_result_t *samrpc_remove_vsn_pool_1_svr();

#define	samrpc_get_all_vsn_copy_maps	127
extern  samrpc_result_t *samrpc_get_all_vsn_copy_maps_1_svr();

#define	samrpc_get_vsn_copy_map	128
extern  samrpc_result_t *samrpc_get_vsn_copy_map_1_svr();

#define	samrpc_add_vsn_copy_map	129
extern  samrpc_result_t *samrpc_add_vsn_copy_map_1_svr();

#define	samrpc_modify_vsn_copy_map	130
extern  samrpc_result_t *samrpc_modify_vsn_copy_map_1_svr();

#define	samrpc_remove_vsn_copy_map	131
extern  samrpc_result_t *samrpc_remove_vsn_copy_map_1_svr();

#define	samrpc_get_default_ar_drive_directive	132
extern  samrpc_result_t *samrpc_get_default_ar_drive_directive_1_svr();

/* archive.h - control APIs */
#define	samrpc_ar_run	133
extern  samrpc_result_t *samrpc_ar_run_1_svr();

#define	samrpc_ar_run_all	134
extern  samrpc_result_t *samrpc_ar_run_all_1_svr();

#define	samrpc_ar_stop	135
extern  samrpc_result_t *samrpc_ar_stop_1_svr();

#define	samrpc_ar_stop_all	136
extern  samrpc_result_t *samrpc_ar_stop_all_1_svr();

#define	samrpc_ar_idle	137
extern  samrpc_result_t *samrpc_ar_idle_1_svr();

#define	samrpc_ar_idle_all	138
extern  samrpc_result_t *samrpc_ar_idle_all_1_svr();

#define	samrpc_ar_restart_all	139
extern  samrpc_result_t *samrpc_ar_restart_all_1_svr();

#define	samrpc_get_archiverd_state	140
extern  samrpc_result_t *samrpc_get_archiverd_state_1_svr();

#define	samrpc_get_arfind_state	141
extern  samrpc_result_t *samrpc_get_arfind_state_1_svr();

#define	samrpc_get_all_arfind_state	142
extern  samrpc_result_t *samrpc_get_all_arfind_state_1_svr();

#define	samrpc_get_archreqs	143
extern  samrpc_result_t *samrpc_get_archreqs_1_svr();

#define	samrpc_get_all_archreqs	144
extern  samrpc_result_t *samrpc_get_all_archreqs_1_svr();

#define	samrpc_get_all_ar_fs_directives	145
extern  samrpc_result_t *samrpc_get_all_ar_fs_directives_1_svr();

#define	samrpc_activate_archiver_cfg	146
extern  samrpc_result_t *samrpc_activate_archiver_cfg_1_svr();

#define	samrpc_is_pool_in_use	147
extern  samrpc_result_t *samrpc_is_pool_in_use_1_svr();

#define	samrpc_is_valid_group	148
extern  samrpc_result_t *samrpc_is_valid_group_1_svr();

#define	samrpc_get_all_arch_sets	149
extern  samrpc_result_t *samrpc_get_all_arch_sets_3_svr();

#define	samrpc_get_arch_set	150
extern  samrpc_result_t *samrpc_get_arch_set_3_svr();

#define	samrpc_create_arch_set	151
extern  samrpc_result_t *samrpc_create_arch_set_3_svr();

#define	samrpc_modify_arch_set	152
extern  samrpc_result_t *samrpc_modify_arch_set_3_svr();

#define	samrpc_delete_arch_set	153
extern  samrpc_result_t *samrpc_delete_arch_set_3_svr();

#define	samrpc_is_valid_user	154
extern  samrpc_result_t *samrpc_is_valid_user_3_svr();

#define	samrpc_ar_rerun_all	155
extern  samrpc_result_t *samrpc_ar_rerun_all_4_svr();

#define	samrpc_associate_class_with_policy	156
extern  samrpc_result_t *samrpc_associate_class_with_policy_6_svr();

#define	samrpc_delete_data_class	157
extern  samrpc_result_t *samrpc_delete_data_class_6_svr();

#define	samrpc_set_class_order	158
extern  samrpc_result_t *samrpc_set_class_order_6_svr();

#define	samrpc_archive_files	159
extern  samrpc_result_t *samrpc_archive_files_6_svr();

#define	samrpc_get_copy_utilization	160
extern  samrpc_result_t *samrpc_get_copy_utilization_6_svr();


/* device.h - config APIs */
#define	samrpc_discover_avail_aus	200
extern  enum clnt_stat samrpc_discover_avail_aus_1();
extern  samrpc_result_t *samrpc_discover_avail_aus_1_svr();

#define	samrpc_discover_avail_aus_by_type	201
extern  samrpc_result_t *samrpc_discover_avail_aus_by_type_1_svr();

#define	samrpc_get_all_libraries	203
extern  samrpc_result_t *samrpc_get_all_libraries_1_svr();

#define	samrpc_get_library_by_path	204
extern  samrpc_result_t *samrpc_get_library_by_path_1_svr();

#define	samrpc_get_library_by_family_set	205
extern  samrpc_result_t *samrpc_get_library_by_family_set_1_svr();

#define	samrpc_get_library_by_equ	206
extern  samrpc_result_t *samrpc_get_library_by_equ_1_svr();

#define	samrpc_add_library	207
extern  samrpc_result_t *samrpc_add_library_1_svr();

#define	samrpc_remove_library	208
extern  samrpc_result_t *samrpc_remove_library_1_svr();

#define	samrpc_get_all_standalone_drives	209
extern  samrpc_result_t *samrpc_get_all_standalone_drives_1_svr();

#define	samrpc_get_no_of_catalog_entries	214
extern  samrpc_result_t *samrpc_get_no_of_catalog_entries_1_svr();

#define	samrpc_rb_auditslot_from_eq	215
extern  samrpc_result_t *samrpc_rb_auditslot_from_eq_1_svr();

#define	samrpc_rb_chmed_flags_from_eq	216
extern  samrpc_result_t *samrpc_rb_chmed_flags_from_eq_1_svr();

#define	samrpc_rb_clean_drive	217
extern  samrpc_result_t *samrpc_rb_clean_drive_1_svr();

#define	samrpc_rb_import	218
extern  samrpc_result_t *samrpc_rb_import_1_svr();

#define	samrpc_rb_export_from_eq	219
extern  samrpc_result_t *samrpc_rb_export_from_eq_1_svr();

#define	samrpc_rb_load_from_eq	220
extern  samrpc_result_t *samrpc_rb_load_from_eq_1_svr();

#define	samrpc_rb_unload	221
extern  samrpc_result_t *samrpc_rb_unload_1_svr();

#define	samrpc_rb_move_from_eq	222
extern  samrpc_result_t *samrpc_rb_move_from_eq_1_svr();

#define	samrpc_rb_tplabel_from_eq	223
extern  samrpc_result_t *samrpc_rb_tplabel_from_eq_1_svr();

#define	samrpc_rb_unreserve_from_eq	224
extern  samrpc_result_t *samrpc_rb_unreserve_from_eq_1_svr();

#define	samrpc_change_state	225
extern  samrpc_result_t *samrpc_change_state_1_svr();

#define	samrpc_is_vsn_loaded	226
extern  samrpc_result_t *samrpc_is_vsn_loaded_1_svr();

#define	samrpc_get_total_capacity_of_library	229
extern  samrpc_result_t *samrpc_get_total_capacity_of_library_1_svr();

#define	samrpc_get_free_space_of_library	230
extern  samrpc_result_t *samrpc_get_free_space_of_library_1_svr();

#define	samrpc_get_vsn_list	231
extern  samrpc_result_t *samrpc_get_vsn_list_1_svr();

#define	samrpc_get_tape_label_running_list	232
extern  samrpc_result_t *samrpc_get_tape_label_running_list_1_svr();

#define	samrpc_get_nw_library	233
extern  samrpc_result_t *samrpc_get_nw_library_1_svr();

#define	samrpc_get_all_available_media_type	234
extern  samrpc_result_t *samrpc_get_all_available_media_type_1_svr();

#define	samrpc_get_properties_of_archive_vsnpool	235
extern  samrpc_result_t *samrpc_get_properties_of_archive_vsnpool_1_svr();

#define	samrpc_get_available_vsns	236
extern  samrpc_result_t *samrpc_get_available_vsns_1_svr();

#define	samrpc_get_catalog_entry	237
extern  samrpc_result_t *samrpc_get_catalog_entry_1_svr();

#define	samrpc_get_catalog_entry_by_media_type	238
extern  samrpc_result_t *samrpc_get_catalog_entry_by_media_type_1_svr();

#define	samrpc_get_catalog_entry_from_lib	239
extern  samrpc_result_t *samrpc_get_catalog_entry_from_lib_1_svr();

#define	samrpc_get_all_catalog_entries	240
extern  samrpc_result_t *samrpc_get_all_catalog_entries_1_svr();

#define	samrpc_rb_reserve_from_eq	242
extern  samrpc_result_t *samrpc_rb_reserve_from_eq_1_svr();

#define	samrpc_get_default_media_block_size	243
extern  samrpc_result_t *samrpc_get_default_media_block_size_1_svr();

#define	samrpc_discover_media_unused_in_mcf	244
extern  samrpc_result_t *samrpc_discover_media_unused_in_mcf_1_svr();

#define	samrpc_discover_standalone_drive_by_path	245
extern  samrpc_result_t *samrpc_discover_standalone_drive_by_path_1_svr();

#define	samrpc_discover_library_by_path	246
extern  samrpc_result_t *samrpc_discover_library_by_path_1_svr();

/* added in 4.2 */
#define	samrpc_import_all	247
extern  samrpc_result_t *samrpc_import_all_1_svr();

#define	samrpc_check_slices_for_overlaps	248
extern  enum clnt_stat samrpc_check_slices_for_overlaps_2();
extern  samrpc_result_t *samrpc_check_slices_for_overlaps_2_svr();

/* added in 4.3 */
#define	samrpc_discover_aus	250
extern  enum clnt_stat samrpc_discover_aus_1();
extern  samrpc_result_t *samrpc_discover_aus_1_svr();

/* added in 4.4 */
#define	samrpc_get_vsn_pool_properties	251
extern  samrpc_result_t *samrpc_get_vsn_pool_properties_4_svr();

#define	samrpc_get_vsn_map_properties	252
extern  samrpc_result_t *samrpc_get_vsn_map_properties_4_svr();

#define	samrpc_get_stk_filter_volume_list 254
extern  samrpc_result_t *samrpc_get_stk_filter_volume_list_5_svr();

#define	samrpc_import_stk_vsns 258
extern  samrpc_result_t *samrpc_import_stk_vsns_5_svr();

#define	samrpc_add_list_libraries 259
extern  samrpc_result_t *samrpc_add_list_libraries_5_svr();

#define	samrpc_get_stk_vsn_names 261
extern  samrpc_result_t *samrpc_get_stk_vsn_names_5_svr();

#define	samrpc_discover_stk 262
extern  samrpc_result_t *samrpc_discover_stk_5_svr();

#define	samrpc_discover_ha_aus 263
extern  samrpc_result_t *samrpc_discover_ha_aus_5_svr();

#define	samrpc_get_stk_phyconf_info 264
extern  samrpc_result_t *samrpc_get_stk_phyconf_info_5_svr();

#define	samrpc_modify_stkdrive_share_status 265
extern	samrpc_result_t *samrpc_modify_stkdrive_share_status_5_svr();

#define	samrpc_get_vsns 267
extern  samrpc_result_t *samrpc_get_vsns_6_svr();

/* filesystem.h */
#define	samrpc_get_all_fs	300
extern  samrpc_result_t *samrpc_get_all_fs_1_svr();

#define	samrpc_get_fs_names	301
extern  samrpc_result_t *samrpc_get_fs_names_1_svr();

#define	samrpc_get_fs	302
extern  samrpc_result_t *samrpc_get_fs_1_svr();

#define	samrpc_create_fs	303
extern  samrpc_result_t *samrpc_create_fs_1_svr();

#define	samrpc_change_mount_options	304
extern  samrpc_result_t *samrpc_change_mount_options_1_svr();

#define	samrpc_change_live_mount_options	305
extern  samrpc_result_t *samrpc_change_live_mount_options_1_svr();

#define	samrpc_get_default_mount_options	307
extern  samrpc_result_t *samrpc_get_default_mount_options_1_svr();

#define	samrpc_remove_fs	308
extern  samrpc_result_t *samrpc_remove_fs_1_svr();

#define	samrpc_samfsck_fs	309
extern  samrpc_result_t *samrpc_samfsck_fs_1_svr();

#define	samrpc_get_all_samfsck_info	310
extern  samrpc_result_t *samrpc_get_all_samfsck_info_1_svr();

#define	samrpc_mount_fs	311
extern  samrpc_result_t *samrpc_mount_fs_1_svr();

#define	samrpc_umount_fs	312
extern  samrpc_result_t *samrpc_umount_fs_1_svr();

#define	samrpc_grow_fs	313
extern  samrpc_result_t *samrpc_grow_fs_1_svr();

#define	samrpc_create_fs_and_mount	314
extern  samrpc_result_t *samrpc_create_fs_and_mount_1_svr();

#define	samrpc_create_dir	315
extern  samrpc_result_t *samrpc_create_dir_1_svr();

#define	samrpc_get_equipment_ordinals	316
extern  samrpc_result_t *samrpc_get_equipment_ordinals_3_svr();

#define	samrpc_check_equipment_ordinals	317
extern  samrpc_result_t *samrpc_check_equipment_ordinals_3_svr();

#define	samrpc_get_fs_names_all_types	318
extern  samrpc_result_t *samrpc_get_fs_names_all_types_3_svr();

#define	samrpc_reset_equipment_ordinals	319
extern  samrpc_result_t *samrpc_reset_equipment_ordinals_3_svr();

#define	samrpc_get_generic_filesystems	320
extern  samrpc_result_t *samrpc_get_generic_filesystems_4_svr();

#define	samrpc_mount_generic_fs	321
extern  samrpc_result_t *samrpc_mount_generic_fs_4_svr();

#define	samrpc_remove_generic_fs	322
extern  samrpc_result_t *samrpc_remove_generic_fs_4_svr();

#define	samrpc_get_nfs_opts	323
extern  samrpc_result_t *samrpc_get_nfs_opts_4_svr();

#define	samrpc_set_nfs_opts	324
extern  samrpc_result_t *samrpc_set_nfs_opts_4_svr();

#define	samrpc_set_advanced_network_cfg	325
extern  samrpc_result_t *samrpc_set_advanced_network_cfg_5_svr();

#define	samrpc_get_advanced_network_cfg	326
extern  samrpc_result_t *samrpc_get_advanced_network_cfg_5_svr();

/* added for IS pre 4.6 */
#define	samrpc_get_mds_host 327
extern  samrpc_result_t *samrpc_get_mds_host_6_svr();

#define	samrpc_create_arch_fs 328
extern  samrpc_result_t *samrpc_create_arch_fs_6_svr();

#define	samrpc_set_device_state 329
extern samrpc_result_t *samrpc_set_device_state_5_0_svr();

#define	samrpc_shrink_release 330
extern samrpc_result_t *samrpc_shrink_release_5_0_svr();

#define	samrpc_shrink_remove 331
extern samrpc_result_t *samrpc_shrink_remove_5_0_svr();

#define	samrpc_shrink_replace_device 332
extern samrpc_result_t *samrpc_shrink_replace_device_5_0_svr();

#define	samrpc_shrink_replace_group 333
extern samrpc_result_t *samrpc_shrink_replace_group_5_0_svr();


/* license.h */
#define	samrpc_get_license_type		400
extern  samrpc_result_t *samrpc_get_license_type_1_svr();

#define	samrpc_get_expiration_date	401
extern  samrpc_result_t *samrpc_get_expiration_date_1_svr();

#define	samrpc_get_samfs_type	402
extern  samrpc_result_t *samrpc_get_samfs_type_1_svr();

#define	samrpc_get_licensed_media_slots	403
extern  samrpc_result_t *samrpc_get_licensed_media_slots_1_svr();

#define	samrpc_get_licensed_media_types	404
extern  samrpc_result_t *samrpc_get_licensed_media_types_1_svr();

#define	samrpc_get_license_info	405
extern  samrpc_result_t *samrpc_get_license_info_3_svr();


/* load.h */

#define	samrpc_get_pending_load_info	500
extern  samrpc_result_t *samrpc_get_pending_load_info_1_svr();

#define	samrpc_clear_load_request	501
extern  samrpc_result_t *samrpc_clear_load_request_1_svr();

/* recycle.h */
#define	samrpc_get_rc_log	600
extern  samrpc_result_t *samrpc_get_rc_log_1_svr();

#define	samrpc_set_rc_log	601
extern  samrpc_result_t *samrpc_set_rc_log_1_svr();

#define	samrpc_get_default_rc_log	602
extern  samrpc_result_t *samrpc_get_default_rc_log_1_svr();

#define	samrpc_get_rc_notify_script	603
extern  samrpc_result_t *samrpc_get_rc_notify_script_1_svr();

#define	samrpc_set_rc_notify_script	604
extern  samrpc_result_t *samrpc_set_rc_notify_script_1_svr();

#define	samrpc_get_default_rc_notify_script	605
extern  samrpc_result_t *samrpc_get_default_rc_notify_script_1_svr();

#define	samrpc_get_all_no_rc_vsns	606
extern  samrpc_result_t *samrpc_get_all_no_rc_vsns_1_svr();

#define	samrpc_get_no_rc_vsns	607
extern  samrpc_result_t *samrpc_get_no_rc_vsns_1_svr();

#define	samrpc_add_no_rc_vsns	608
extern  samrpc_result_t *samrpc_add_no_rc_vsns_1_svr();

#define	samrpc_modify_no_rc_vsns	609
extern  samrpc_result_t *samrpc_modify_no_rc_vsns_1_svr();

#define	samrpc_remove_no_rc_vsns	610
extern  samrpc_result_t *samrpc_remove_no_rc_vsns_1_svr();

#define	samrpc_get_all_rc_robot_cfg	611
extern  samrpc_result_t *samrpc_get_all_rc_robot_cfg_1_svr();

#define	samrpc_get_rc_robot_cfg	612
extern  samrpc_result_t *samrpc_get_rc_robot_cfg_1_svr();

#define	samrpc_set_rc_robot_cfg	613
extern  samrpc_result_t *samrpc_set_rc_robot_cfg_1_svr();

#define	samrpc_reset_rc_robot_cfg	614
extern  samrpc_result_t *samrpc_reset_rc_robot_cfg_1_svr();

#define	samrpc_get_default_rc_params	615
extern  samrpc_result_t *samrpc_get_default_rc_params_1_svr();


/* release.h */
#define	samrpc_get_all_rl_fs_directives	700
extern  samrpc_result_t *samrpc_get_all_rl_fs_directives_1_svr();

#define	samrpc_get_rl_fs_directive	701
extern  samrpc_result_t *samrpc_get_rl_fs_directive_1_svr();

#define	samrpc_get_default_rl_fs_directive	702
extern  samrpc_result_t *samrpc_get_default_rl_fs_directive_1_svr();

#define	samrpc_set_rl_fs_directive	703
extern  samrpc_result_t *samrpc_set_rl_fs_directive_1_svr();

#define	samrpc_reset_rl_fs_directive	704
extern  samrpc_result_t *samrpc_reset_rl_fs_directive_1_svr();

#define	samrpc_get_releasing_fs_list	705
extern  samrpc_result_t *samrpc_get_releasing_fs_list_1_svr();

#define	samrpc_release_files	706
extern  samrpc_result_t *samrpc_release_files_6_svr();



/* stage.h */
#define	samrpc_get_stager_cfg	800
extern  samrpc_result_t *samrpc_get_stager_cfg_1_svr();

#define	samrpc_get_drive_directive	801
extern  samrpc_result_t *samrpc_get_drive_directive_1_svr();

#define	samrpc_get_buffer_directive	802
extern  samrpc_result_t *samrpc_get_buffer_directive_1_svr();

#define	samrpc_set_stager_cfg	803
extern  samrpc_result_t *samrpc_set_stager_cfg_1_svr();

#define	samrpc_set_drive_directive	804
extern  samrpc_result_t *samrpc_set_drive_directive_1_svr();

#define	samrpc_reset_drive_directive	805
extern  samrpc_result_t *samrpc_reset_drive_directive_1_svr();

#define	samrpc_set_buffer_directive	806
extern  samrpc_result_t *samrpc_set_buffer_directive_1_svr();

#define	samrpc_reset_buffer_directive	807
extern  samrpc_result_t *samrpc_reset_buffer_directive_1_svr();

#define	samrpc_get_default_stager_cfg	808
extern  samrpc_result_t *samrpc_get_default_stager_cfg_1_svr();

#define	samrpc_get_default_staging_drive_directive	809
extern  samrpc_result_t *samrpc_get_default_staging_drive_directive_1_svr();

#define	samrpc_get_default_staging_buffer_directive	810
extern  samrpc_result_t *samrpc_get_default_staging_buffer_directive_1_svr();

#define	samrpc_get_stager_info	811
extern  samrpc_result_t *samrpc_get_stager_info_1_svr();

#define	samrpc_get_all_staging_files	812
extern  samrpc_result_t *samrpc_get_all_staging_files_1_svr();

#define	samrpc_get_all_staging_files_in_stream	813
extern  samrpc_result_t *samrpc_get_all_staging_files_in_stream_1_svr();

#define	samrpc_cancel_stage	814
extern  samrpc_result_t *samrpc_cancel_stage_1_svr();

#define	samrpc_clear_stage_request	815
extern  samrpc_result_t *samrpc_clear_stage_request_1_svr();

#define	samrpc_st_idle	816
extern  samrpc_result_t *samrpc_st_idle_1_svr();

#define	samrpc_st_run	817
extern  samrpc_result_t *samrpc_st_run_1_svr();

#define	samrpc_get_staging_files	818
extern  samrpc_result_t *samrpc_get_staging_files_1_svr();

#define	samrpc_find_staging_file	819
extern  samrpc_result_t *samrpc_find_staging_file_1_svr();

#define	samrpc_get_staging_files_in_stream	820
extern  samrpc_result_t *samrpc_get_staging_files_in_stream_1_svr();

#define	samrpc_get_total_staging_files	821
extern  samrpc_result_t *samrpc_get_total_staging_files_1_svr();

#define	samrpc_get_numof_staging_files_in_stream	822
extern  samrpc_result_t *samrpc_get_numof_staging_files_in_stream_1_svr();

#define	samrpc_stage_files_pre46	823
extern samrpc_result_t *samrpc_stage_files_5_svr();

#define	samrpc_stage_files	824
extern samrpc_result_t *samrpc_stage_files_6_svr();


/* faults.h */

/* samrpc_get_faults	900 is deprecated in API_VERSION 1.5.0 */
#define	samrpc_get_faults	900 /* here for backward compatibility */

/* samrpc_update_fault	901 is deprecated in API_VERSION 1.5.0 */

#define	samrpc_is_faults_gen_status_on	903
extern  samrpc_result_t *samrpc_is_faults_gen_status_on_1_svr();

#define	samrpc_get_faults_by_lib	904
extern  samrpc_result_t *samrpc_get_faults_by_lib_1_svr();

#define	samrpc_get_faults_by_eq		905
extern  samrpc_result_t *samrpc_get_faults_by_eq_1_svr();

#define	samrpc_delete_faults		906
extern  enum clnt_stat samrpc_delete_faults_1();
extern  samrpc_result_t *samrpc_delete_faults_1_svr();

#define	samrpc_ack_faults		907
extern  enum clnt_stat samrpc_ack_faults_1();
extern  samrpc_result_t *samrpc_ack_faults_1_svr();

#define	samrpc_get_fault_summary		908
extern  samrpc_result_t *samrpc_get_fault_summary_1_svr();

#define	samrpc_get_all_faults	909
extern  samrpc_result_t *samrpc_get_all_faults_6_svr();

/* diskvols.h */

#define	samrpc_get_disk_vol	1000
extern  samrpc_result_t *samrpc_get_disk_vol_1_svr();

#define	samrpc_get_all_disk_vols	1001
extern  samrpc_result_t *samrpc_get_all_disk_vols_1_svr();

#define	samrpc_get_all_clients	1002
extern  samrpc_result_t *samrpc_get_all_clients_1_svr();

#define	samrpc_add_disk_vol	1003
extern  samrpc_result_t *samrpc_add_disk_vol_1_svr();

#define	samrpc_remove_disk_vol	1004
extern  samrpc_result_t *samrpc_remove_disk_vol_1_svr();

#define	samrpc_add_client	1005
extern  samrpc_result_t *samrpc_add_client_1_svr();

#define	samrpc_remove_client	1006
extern  samrpc_result_t *samrpc_remove_client_1_svr();

#define	samrpc_set_disk_vol_flags	1007
extern  samrpc_result_t *samrpc_set_disk_vol_flags_4_svr();

/* notify_summary.h */

#define	samrpc_get_notify_summary	1050
extern  samrpc_result_t *samrpc_get_notify_summary_1_svr();

#define	samrpc_del_notify_summary	1051
extern  samrpc_result_t *samrpc_del_notify_summary_1_svr();

#define	samrpc_mod_notify_summary	1052
extern  samrpc_result_t *samrpc_mod_notify_summary_1_svr();

#define	samrpc_add_notify_summary	1053
extern  samrpc_result_t *samrpc_add_notify_summary_1_svr();

#define	samrpc_get_email_addrs_by_subj	1054
extern  samrpc_result_t *samrpc_get_email_addrs_by_subj_5_svr();

/* mgmt.h */
#define	samrpc_get_samfs_version	1100
extern  enum clnt_stat samrpc_get_samfs_version_1();
extern  samrpc_result_t *samrpc_get_samfs_version_1_svr();

#define	samrpc_get_samfs_lib_version	1101
extern  enum clnt_stat samrpc_get_samfs_lib_version_1();
extern  samrpc_result_t *samrpc_get_samfs_lib_version_1_svr();

#define	samrpc_init_sam_mgmt		1102
extern  enum clnt_stat samrpc_init_sam_mgmt_1();
extern  samrpc_result_t *samrpc_init_sam_mgmt_1_svr();

#define	samrpc_destroy_process		1103
extern  samrpc_result_t *samrpc_destroy_process_1_svr();

#define	samrpc_create_file	1104
extern  samrpc_result_t *samrpc_create_file_1_svr();

#define	samrpc_get_server_info	1105
extern  samrpc_result_t *samrpc_get_server_info_3_svr();

#define	samrpc_file_exists	1106
extern  samrpc_result_t *samrpc_file_exists_3_svr();

#define	samrpc_get_server_capacities	1107
extern  samrpc_result_t *samrpc_get_server_capacities_4_svr();

#define	samrpc_get_logntrace	1108
extern  samrpc_result_t *samrpc_get_logntrace_4_svr();

#define	samrpc_get_system_info	1109
extern  samrpc_result_t *samrpc_get_system_info_4_svr();

#define	samrpc_get_package_info	1110
extern  samrpc_result_t *samrpc_get_package_info_4_svr();

#define	samrpc_get_configuration_status	1111
extern  samrpc_result_t *samrpc_get_configuration_status_4_svr();

#define	samrpc_tail 1112
extern  samrpc_result_t *samrpc_tail_4_svr();

#define	samrpc_get_architecture 1113
extern  samrpc_result_t *samrpc_get_architecture_4_svr();

#define	samrpc_list_explorer_outputs 1114
extern  samrpc_result_t *samrpc_list_explorer_outputs_5_svr();

#define	samrpc_run_sam_explorer 1115
extern  samrpc_result_t *samrpc_run_sam_explorer_5_svr();

#define	samrpc_get_sc_version 1120
extern  samrpc_result_t *samrpc_get_sc_version_5_svr();

#define	samrpc_get_sc_name 1121
extern  samrpc_result_t *samrpc_get_sc_name_5_svr();

#define	samrpc_get_sc_nodes 1122
extern  samrpc_result_t *samrpc_get_sc_nodes_5_svr();

#define	samrpc_get_sc_ui_state 1123
extern  samrpc_result_t *samrpc_get_sc_ui_state_5_svr();

#define	samrpc_get_status_processes 1124
extern  samrpc_result_t *samrpc_get_status_processes_6_svr();

#define	samrpc_get_component_status_summary 1125
extern  samrpc_result_t *samrpc_get_component_status_summary_6_svr();

#define	samrpc_get_config_summary 1126
extern  samrpc_result_t *samrpc_get_config_summary_5_0_svr();

/* recyc_sh_wrap.h */

#define	samrpc_get_recycl_sh_action_status	1150
extern samrpc_result_t *samrpc_get_recycl_sh_action_status_1_svr();

#define	samrpc_add_recycle_sh_action_label	1151
extern samrpc_result_t *samrpc_add_recycle_sh_action_label_1_svr();

#define	samrpc_add_recycle_sh_action_export	1152
extern samrpc_result_t *samrpc_add_recycle_sh_action_export_1_svr();

#define	samrpc_del_recycle_sh_action		1153
extern samrpc_result_t *samrpc_del_recycle_sh_action_1_svr();

#define	samrpc_get_jobhist_by_fs		1160
extern samrpc_result_t *samrpc_get_jobhist_by_fs_1_svr();

#define	samrpc_get_all_jobhist			1161
extern samrpc_result_t *samrpc_get_all_jobhist_1_svr();

#define	samrpc_get_host_config			1162
extern samrpc_result_t *samrpc_get_host_config_3_svr();

#define	samrpc_remove_host			1163
extern samrpc_result_t *samrpc_remove_host_3_svr();

#define	samrpc_change_metadata_server	1164
extern samrpc_result_t *samrpc_change_metadata_server_3_svr();

#define	samrpc_add_host			1165
extern samrpc_result_t *samrpc_add_host_3_svr();

#define	samrpc_discover_ip_addresses	1166
extern samrpc_result_t *samrpc_discover_ip_addresses_3_svr();

#define	samrpc_get_shared_fs_hosts	1167
extern samrpc_result_t *samrpc_get_shared_fs_hosts_5_0_svr();

/* samfsrestore and samfsdump */
#define	samrpc_set_csd_params	1169
extern samrpc_result_t *samrpc_set_csd_params_4_svr();

#define	samrpc_get_csd_params	1170
extern samrpc_result_t *samrpc_get_csd_params_4_svr();

#define	samrpc_list_dumps	1171
extern samrpc_result_t *samrpc_list_dumps_4_svr();

/*
 * samrpc_stage_files 1172
 * was not used and its interface was not sufficient for the current
 * needs. It has been replaced by samrpc_stage_files 823
 */

#define	samrpc_list_dir		1173
extern samrpc_result_t *samrpc_list_dir_4_svr();

#define	samrpc_get_file_status	1174
extern samrpc_result_t *samrpc_get_file_status_4_svr();

#define	samrpc_list_versions	1175
extern samrpc_result_t *samrpc_list_versions_4_svr();

#define	samrpc_get_version_details	1176
extern samrpc_result_t *samrpc_get_version_details_4_svr();

#define	samrpc_search_versions	1177
extern samrpc_result_t *samrpc_search_versions_4_svr();

#define	samrpc_restore_inodes	1178
extern samrpc_result_t *samrpc_restore_inodes_4_svr();

#define	samrpc_get_dump_status	1179
extern samrpc_result_t *samrpc_get_dump_status_4_svr();

#define	samrpc_decompress_dump	1180
extern samrpc_result_t *samrpc_decompress_dump_4_svr();

#define	samrpc_take_dump	1181
extern samrpc_result_t *samrpc_take_dump_4_svr();

#define	samrpc_get_search_results	1182
extern samrpc_result_t *samrpc_get_search_results_4_svr();

#define	samrpc_get_file_details		1183
extern samrpc_result_t *samrpc_get_file_details_4_svr();

#define	samrpc_list_activities	1190
extern samrpc_result_t *samrpc_list_activities_4_svr();

#define	samrpc_kill_activity	1191
extern samrpc_result_t *samrpc_kill_activity_4_svr();

#define	samrpc_cleanup_dump	1192
extern samrpc_result_t *samrpc_cleanup_dump_4_svr();

#define	samrpc_delete_dump	1193
extern samrpc_result_t *samrpc_delete_dump_4_svr();

#define	samrpc_get_dump_status_by_dir	1194
extern samrpc_result_t *samrpc_get_dump_status_by_dir_4_svr();

#define	samrpc_list_dumps_by_dir	1195
extern samrpc_result_t *samrpc_list_dumps_by_dir_4_svr();

#define	samrpc_set_snapshot_locked	1196
extern samrpc_result_t *samrpc_set_snapshot_locked_5_svr();

#define	samrpc_clear_snapshot_locked	1197
extern samrpc_result_t *samrpc_clear_snapshot_locked_5_svr();

#define	samrpc_get_indexed_snapshot_directories		1198
extern samrpc_result_t *samrpc_get_indexed_snapshot_directories_6_svr();

#define	samrpc_get_indexed_snapshots	1199
extern samrpc_result_t *samrpc_get_indexed_snapshots_6_svr();

/*
 * file_util.h ids 1300 to 1399
 * many of the functions that were put in file_util.h
 * in 4.5 were previously in other header files. These have not been
 * moved into this section because we cannot change their call ids and
 * moving them would complicate future call id assignment. New functions
 * should however be added here.
 */
#define	samrpc_get_txt_file 1300
extern  samrpc_result_t *samrpc_get_txt_file_5_svr();

#define	samrpc_get_extended_file_details 1301
extern  samrpc_result_t *samrpc_get_extended_file_details_5_svr();

#define	samrpc_get_copy_details 1302
extern  samrpc_result_t *samrpc_get_copy_details_5_svr();

#define	samrpc_list_directory 1303
extern samrpc_result_t *samrpc_list_directory_6_svr();

#define	samrpc_list_and_collect_file_details 1304
extern  samrpc_result_t *samrpc_list_and_collect_file_details_6_svr();

#define	samrpc_collect_file_details 1305
extern  samrpc_result_t *samrpc_collect_file_details_6_svr();

#define	samrpc_delete_files 1306
extern samrpc_result_t *samrpc_delete_files_6_svr();


/* Registration and telemetry functions */
#define	samrpc_cns_get_registration 1350
extern	samrpc_result_t *samrpc_cns_get_registration_6_svr();

#define	samrpc_cns_register 1351
extern	samrpc_result_t *samrpc_cns_register_6_svr();

#define	samrpc_cns_get_public_key 1352
extern	samrpc_result_t *samrpc_cns_get_public_key_6_svr();


/* report */
#define	samrpc_gen_report 1400
extern samrpc_result_t *samrpc_gen_report_6_svr();

#define	samrpc_get_file_metrics_report 1403
extern samrpc_result_t *samrpc_get_file_metrics_report_6_svr();

/* task schedules */
#define	samrpc_get_task_schedules 1410
extern  samrpc_result_t *samrpc_get_task_schedules_6_svr();

#define	samrpc_set_task_schedule 1411
extern  samrpc_result_t *samrpc_set_task_schedule_6_svr();

#define	samrpc_remove_task_schedule 1412
extern  samrpc_result_t *samrpc_remove_task_schedule_6_svr();

#define	samrpc_get_specific_tasks 1413
extern  samrpc_result_t *samrpc_get_specific_tasks_6_svr();


/*
 * *******************
 *  the xdr functions
 * *******************
 */

#define	xdr_uint_t		xdr_u_int
#define	xdr_upri_t		xdr_float
#define	xdr_time32_t	xdr_int32_t
#define	xdr_fsize_t		xdr_uint64_t
#define	xdr_media_t		xdr_uint16_t
#define	xdr_dtype_t		xdr_uint16_t
#define	xdr_equ_t		xdr_uint16_t
#define	xdr_sam_time_t	xdr_int32_t
#define	xdr_sam_ino_t	xdr_uint32_t
#define	xdr_dsize_t		xdr_uint64_t
#define	xdr_offset_t	xdr_int64_t
#define	xdr_pid_t		xdr_long
#define	xdr_uid_t		xdr_long
#define	xdr_time_t		xdr_long
#define	xdr_ushort		xdr_u_short
#define	xdr_ushort_t	xdr_u_short
#define	xdr_ulong_t		xdr_u_long
#define	xdr_size_t		xdr_uint_t
#define	xdr_uchar_t		xdr_u_char


extern bool_t xdr_samstruct_type_t();
extern bool_t xdr_result_struct_t();
extern bool_t xdr_err_struct_t();
extern bool_t xdr_samrpc_result_t();

/* list.h */
extern bool_t xdr_int_node();
extern bool_t xdr_int_list();
extern bool_t xdr_string_node();
extern bool_t xdr_string_list();
extern bool_t xdr_buffer_directive_node();
extern bool_t xdr_buffer_directive_list();
extern bool_t xdr_drive_directive_node();
extern bool_t xdr_drive_directive_list();
extern bool_t xdr_ar_set_criteria_node();
extern bool_t xdr_ar_set_criteria_list();
extern bool_t xdr_ar_fs_directive_node();
extern bool_t xdr_ar_fs_directive_list();
extern bool_t xdr_priority_node();
extern bool_t xdr_priority_list();
extern bool_t xdr_ar_set_copy_params_node();
extern bool_t xdr_ar_set_copy_params_list();
extern bool_t xdr_vsn_pool_node();
extern bool_t xdr_vsn_pool_list();
extern bool_t xdr_vsn_map_node();
extern bool_t xdr_vsn_map_list();
extern bool_t xdr_arfind_status_node();
extern bool_t xdr_arfind_status_list();
extern bool_t xdr_arcopy_status_node();
extern bool_t xdr_arcopy_status_list();
extern bool_t xdr_ar_find_state_node();
extern bool_t xdr_ar_find_state_list();
extern bool_t xdr_archreq_node();
extern bool_t xdr_archreq_list();

extern bool_t xdr_arch_set_list();
extern bool_t xdr_arch_set_node();

extern bool_t xdr_drive_node();
extern bool_t xdr_drive_list();
extern bool_t xdr_md_license_node();
extern bool_t xdr_md_license_list();
extern bool_t xdr_au_node();
extern bool_t xdr_au_list();
extern bool_t xdr_library_node();
extern bool_t xdr_library_list();
extern bool_t xdr_catalog_entry_node();
extern bool_t xdr_catalog_entry_list();
extern bool_t xdr_stk_capacity_list();
extern bool_t xdr_stk_device_list();
extern bool_t xdr_stk_host_list();
extern bool_t xdr_stk_lsm_list();
extern bool_t xdr_stk_pool_list();
extern bool_t xdr_stk_panel_list();
extern bool_t xdr_stk_volume_list();
extern bool_t xdr_stk_info_list();

extern bool_t xdr_disk_node();
extern bool_t xdr_disk_list();
extern bool_t xdr_striped_group_node();
extern bool_t xdr_striped_group_list();
extern bool_t xdr_fs_node();
extern bool_t xdr_fs_list();
extern bool_t xdr_samfsck_info_node();
extern bool_t xdr_samfsck_info_list();
extern bool_t xdr_failed_mount_option_node();
extern bool_t xdr_failed_mount_option_list();
extern bool_t xdr_ar_set_copy_cfg_node();
extern bool_t xdr_ar_set_copy_cfg_list();

extern bool_t xdr_fs_load_priority_node();
extern bool_t xdr_fs_load_priority_list();
extern bool_t xdr_pending_load_info_node();
extern bool_t xdr_pending_load_info_list();

extern bool_t xdr_no_rc_vsns_node();
extern bool_t xdr_no_rc_vsns_list();
extern bool_t xdr_rc_robot_cfg_node();
extern bool_t xdr_rc_robot_cfg_list();

extern bool_t xdr_rl_fs_directive_node();
extern bool_t xdr_rl_fs_directive_list();
extern bool_t xdr_rl_fs_node();
extern bool_t xdr_rl_fs_list();

extern bool_t xdr_staging_file_info_node();
extern bool_t xdr_staging_file_info_list();
extern bool_t xdr_active_stager_info_node();
extern bool_t xdr_active_stager_info_list();
extern bool_t xdr_stager_stream_node();
extern bool_t xdr_stager_stream_list();

extern bool_t xdr_fault_attr_node();
extern bool_t xdr_fault_attr_list();

extern bool_t xdr_disk_vol_node();
extern bool_t xdr_disk_vol_list();

extern bool_t xdr_notify_summary_node();
extern bool_t xdr_notify_summary_list();

extern bool_t xdr_job_history_node();
extern bool_t xdr_job_history_list();

extern bool_t xdr_host_info_node();
extern bool_t xdr_host_info_list();

/* types.h */
extern bool_t xdr_read_mode_t();
extern bool_t xdr_write_mode_t();
extern bool_t xdr_devtype_t();
extern bool_t xdr_ctx_t();
extern bool_t xdr_ctx_arg_t();
extern bool_t xdr_uname_t();
extern bool_t xdr_upath_t();
extern bool_t xdr_mtype_t();
extern bool_t xdr_vsn_t();
extern bool_t xdr_sam_id_t();
extern bool_t xdr_host_t();
extern bool_t xdr_umsg_t();
extern bool_t xdr_barcode_t();
extern bool_t xdr_odlabel_info_t();
extern bool_t xdr_boolean_t();
extern bool_t xdr_proctype_t();

extern bool_t xdr_proc_arg_t();
extern bool_t xdr_strlst_bool_arg_t();

/* devstat.h */
extern bool_t xdr_dstate_t();
/* struct_key.h ??? */
extern bool_t xdr_struct_key_t();

/* archive.h */
extern bool_t xdr_join_method_t();
extern bool_t xdr_offline_copy_method_t();
extern bool_t xdr_sort_method_t();
extern bool_t xdr_ExamMethod_t();
extern bool_t xdr_ExecState_t();
extern bool_t xdr_buffer_directive_t();
extern bool_t xdr_drive_directive_t();
extern bool_t xdr_priority_t();
extern bool_t xdr_ar_general_copy_cfg_t();
extern bool_t xdr_ar_set_copy_cfg_t();
extern bool_t xdr_ar_set_copy_cfg_ptr_t();
extern bool_t xdr_ar_set_criteria_t();
extern bool_t xdr_ar_global_directive_t();
extern bool_t xdr_ar_fs_directive_t();
extern bool_t xdr_ar_set_copy_params_t();
extern bool_t xdr_vsn_pool_t();
extern bool_t xdr_vsn_map_t();
extern bool_t xdr_MappedFile_t();
extern bool_t xdr_stats();
extern bool_t xdr_FsStats();
extern bool_t xdr_ar_find_state_t();
extern bool_t xdr_ArchReq();
extern bool_t xdr_ArfindState();
extern bool_t xdr_ArchiverdState();
extern bool_t xdr_ArCopyInstance_t();
extern bool_t xdr_pool_use_result_t();
extern bool_t xdr_strlst_int32_arg_t();

extern bool_t xdr_ar_global_directive_arg_t();
extern bool_t xdr_string_arg_t();
extern bool_t xdr_string_bool_arg_t();
extern bool_t xdr_ar_fs_directive_arg_t();
extern bool_t xdr_ar_set_criteria_arg_t();
extern bool_t xdr_ar_set_copy_params_arg_t();
extern bool_t xdr_vsn_pool_arg_t();
extern bool_t xdr_vsn_map_arg_t();
extern bool_t xdr_string_list_arg_t();
extern bool_t xdr_write_temp_config_archive_arg_t();
extern bool_t xdr_set_name_t();


/* archive_set.h */
extern bool_t xdr_arch_set_t();
extern bool_t xdr_arch_set_arg_t();
extern bool_t xdr_str_critlst_arg_t();

/* device.h */
extern bool_t xdr_dismes_t();
extern bool_t xdr_wwn_id_t();
extern bool_t xdr_base_dev_t();
extern bool_t xdr_au_type_t();
extern bool_t xdr_au_t();
extern bool_t xdr_disk_t();
extern bool_t xdr_nwlib_type_t();
extern bool_t xdr_nwlib_req_info_t();
extern bool_t xdr_scsi_2_t();
extern bool_t xdr_drive_t();
extern bool_t xdr_md_license_t();
extern bool_t xdr_library_t();
extern bool_t xdr_import_option_t();
extern bool_t xdr_vsnpool_property_t();
extern bool_t xdr_diskvsnpool_property_t();
extern bool_t xdr_reserve_option_t();
extern bool_t xdr_CatalogEntry();
extern bool_t xdr_u_r();
extern bool_t xdr_reserve_info();
extern bool_t xdr_vsn_sort_key_t();
extern bool_t xdr_discover_state_t();
extern bool_t xdr_stk_param_t();
extern bool_t xdr_stk_lsm_t();
extern bool_t xdr_stk_pool_t();
extern bool_t xdr_stk_panel_t();
extern bool_t xdr_stk_volume_t();
extern bool_t xdr_stk_host_info_t();
extern bool_t xdr_stk_cap_t();
extern bool_t xdr_stk_capacity_t();
extern bool_t xdr_stk_device_t();
extern bool_t xdr_stk_cell_t();
extern bool_t xdr_stk_phyconf_info_t();

extern bool_t xdr_equ_arg_t();
extern bool_t xdr_equ_bool_arg_t();
extern bool_t xdr_equ_equ_bool_arg_t();
extern bool_t xdr_au_type_arg_t();
extern bool_t xdr_discover_media_result_t();
extern bool_t xdr_library_arg_t();
extern bool_t xdr_library_list_arg_t();
extern bool_t xdr_drive_arg_t();
extern bool_t xdr_equ_slot_part_bool_arg_t();
extern bool_t xdr_equ_range_arg_t();
extern bool_t xdr_reserve_arg_t();
extern bool_t xdr_chmed_arg_t();
extern bool_t xdr_import_arg_t();
extern bool_t xdr_import_vsns_arg_t();
extern bool_t xdr_import_range_arg_t();
extern bool_t xdr_equ_slot_bool_arg_t();
extern bool_t xdr_move_arg_t();
extern bool_t xdr_tplabel_arg_t();
extern bool_t xdr_equ_slot_part_arg_t();
extern bool_t xdr_equ_dstate_arg_t();
extern bool_t xdr_nwlib_req_info_arg_t();
extern bool_t xdr_string_mtype_arg_t();
extern bool_t xdr_string_sort_arg_t();
extern bool_t xdr_equ_sort_arg_t();
extern bool_t xdr_vsnpool_arg_t();
extern bool_t xdr_vsnmap_arg_t();
extern bool_t xdr_stk_host_info_string_arg_t();
extern bool_t xdr_stk_host_list_arg_t();
extern bool_t xdr_stk_display_info_arg_t();

/* filesystem.h */
extern bool_t xdr_striped_group_t();
extern bool_t xdr_io_mount_options_t();
extern bool_t xdr_sam_mount_options_t();
extern bool_t xdr_sharedfs_mount_options_t();
extern bool_t xdr_multireader_mount_options_t();
extern bool_t xdr_qfs_mount_options_t();
extern bool_t xdr_mount_options_t();
extern bool_t xdr_fs_t();
extern bool_t xdr_samfsck_info_t();
extern bool_t xdr_failed_mount_option_t();

extern bool_t xdr_create_fs_arg_t();
extern bool_t xdr_change_mount_options_arg_t();
extern bool_t xdr_get_default_mount_options_arg_t();
extern bool_t xdr_fsck_fs_arg_t();
extern bool_t xdr_grow_fs_arg_t();
extern bool_t xdr_create_fs_mount_arg_t();
extern bool_t xdr_create_arch_fs_arg_t();
extern bool_t xdr_fs_arch_cfg_t();
extern bool_t xdr_int_list_arg_t();
extern bool_t xdr_string_int_intlist_arg_t();
extern bool_t xdr_intlist_arg_t();
extern bool_t xdr_int_list_result_t();
extern bool_t xdr_reset_eq_arg_t();
extern bool_t xdr_string_string_arg_t();
extern bool_t xdr_string_uint32_arg_t();
extern bool_t xdr_int_uint32_arg_t();
extern bool_t xdr_string_uint32_uint32_arg_t();
extern bool_t xdr_int_int_arg_t();
extern bool_t xdr_string_string_int_arg_t();
extern bool_t xdr_string_string_int_int_arg_t();
extern bool_t xdr_string_string_int_disk_arg_t();
extern bool_t xdr_string_string_int_group_arg_t();


/* license.h */
extern bool_t xdr_license_info_t();

/* load.h */
extern bool_t xdr_pending_load_info_t();

extern bool_t xdr_clear_load_request_arg_t();

/* recycle.h */
extern bool_t xdr_no_rc_vsns_t();
extern bool_t xdr_rc_param_t();
extern bool_t xdr_rc_robot_cfg_t();

extern bool_t xdr_no_rc_vsns_arg_t();
extern bool_t xdr_rc_robot_cfg_arg_t();
extern bool_t xdr_rc_upath_arg_t();

/* release.h */
extern bool_t xdr_age_prio_type();
extern bool_t xdr_rl_age_priority_t();
extern bool_t xdr_rl_fs_directive_t();
extern bool_t xdr_release_fs_t();

extern bool_t xdr_rl_fs_directive_arg_t();
extern bool_t xdr_strlst_int32_int32_arg_t();

/* stage.h */
extern bool_t xdr_stager_cfg_t();
extern bool_t xdr_StagerStateDetail_t();
extern bool_t xdr_StagerStateFlags_t();
extern bool_t xdr_active_stager_info_t();
extern bool_t xdr_stager_stream_t();
extern bool_t xdr_stager_info_t();
extern bool_t xdr_staging_file_info_t();
extern bool_t xdr_st_sort_key_t();

extern bool_t xdr_set_stager_cfg_arg_t();
extern bool_t xdr_drive_directive_arg_t();
extern bool_t xdr_buffer_directive_arg_t();
extern bool_t xdr_stager_stream_arg_t();
extern bool_t xdr_stager_stream_range_arg_t();
extern bool_t xdr_stage_arg_t();
extern bool_t xdr_clear_stage_request_arg_t();
extern bool_t xdr_range_arg_t();
extern bool_t xdr_stream_arg_t();
extern bool_t xdr_staging_file_arg_t();
extern bool_t xdr_strlst_intlst_intlst_arg_t();

/* faults.h */
extern bool_t xdr_fault_state_t();
extern bool_t xdr_fault_sev_t();
extern bool_t xdr_fault_attr_t();
#ifdef	SAMRPC_CLIENT
extern bool_t xdr_fault_req_t();
#endif	/* SAMRPC_CLIENT */
extern bool_t xdr_fault_summary_t();

#ifdef	SAMRPC_CLIENT
extern bool_t xdr_fault_req_arg_t();
#endif	/* SAMRPC_CLIENT */
extern bool_t xdr_fault_updt_arg_t();
extern bool_t xdr_fault_errorid_arr_arg_t();

/* diskvols.h */
extern bool_t xdr_disk_vol_t();

extern bool_t xdr_disk_vol_arg_t();

/* notify_summary.h */
extern bool_t xdr_notf_summary_t();

extern bool_t xdr_notify_summary_arg_t();
extern bool_t xdr_mod_notify_summary_arg_t();
extern bool_t xdr_get_email_addrs_arg_t();

/* job_history.h */
extern bool_t xdr_job_type_t();
extern bool_t xdr_job_hdr_t();
extern bool_t xdr_job_hist_t();

extern bool_t xdr_job_hist_arg_t();
extern bool_t xdr_job_type_arg_t();

/* hosts.h */
extern bool_t xdr_host_info_t();
extern bool_t xdr_string_host_info_arg_t();

/* samfsrestore and samfsdump */
extern bool_t xdr_string_strlst_arg_t();
extern bool_t xdr_file_restrictions_arg_t();
extern bool_t xdr_strlst_arg_t();
extern bool_t xdr_version_details_arg_t();
extern bool_t xdr_restore_inodes_arg_t();
extern bool_t xdr_int_string_arg_t();
extern bool_t xdr_string_string_strlst_arg_t();

extern bool_t xdr_int_ptrarr_result_t();

/* file util */
extern bool_t xdr_strlst_uint32_arg_t();
extern bool_t xdr_file_restrictions_more_arg_t();
extern bool_t xdr_file_details_arg_t();
extern bool_t xdr_file_details_result_t();

/* registration argument functions */
extern bool_t xdr_cns_reg_arg_t();
extern bool_t xdr_crypt_str_t();
extern bool_t xdr_public_key_result_t();

/* report */
extern bool_t xdr_report_requirement_arg_t();
extern bool_t xdr_file_metric_rpt_arg_t();

/* definition of macros for easier readability */

#define	XDR_PTR2LST(name, type) \
	if (!xdr_pointer(xdrs, (char **)&name, \
		sizeof (sqm_lst_t), (xdrproc_t)xdr_ ## type)) \
		return (FALSE);

#define	XDR_PTR2CTX(ctx) \
	if (!xdr_pointer(xdrs, (char **)&ctx, \
		sizeof (ctx_t), \
		(xdrproc_t)xdr_ctx_t)) \
		return (FALSE);

#define	XDR_PTR2STRUCT(name, type) \
	if (!xdr_pointer(xdrs, (char **)&name, \
		sizeof (type), (xdrproc_t)xdr_ ## type)) \
		return (FALSE);

#endif /* !_SAMMGMT_H_RPCGEN */
