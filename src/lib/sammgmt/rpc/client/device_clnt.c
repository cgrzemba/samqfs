
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

#pragma ident	"$Revision: 1.38 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/sammgmt_rpc.h"

/*
 * _discover_aus - used internally by discover_avail_aus and discover_aus
 */

static int _discover_aus(
ctx_t *ctx,
sqm_lst_t **au_list,	/* return - list of allocatable unit	*/
boolean_t avail_only	/* decides which API to call */
)
{
	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = (avail_only == B_TRUE) ?
	    "rpc:discover available aus" : "rpc:discover aus";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(au_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;

	stat = avail_only ?
	    samrpc_discover_avail_aus_1(&arg, &result, ctx->handle->clnt) :
	    samrpc_discover_aus_1(&arg, &result, ctx->handle->clnt);

	if (stat != RPC_SUCCESS) {
		SET_RPC_ERROR(ctx, func_name, stat);
	}

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*au_list = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d] and [%d]aus...",
	    func_name, ret_val, (*au_list)->length);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}



/*
 * discover_avail_aus
 *
 * get available AU-s seen by this host.
 */
int
discover_avail_aus(
ctx_t *ctx,		/* client connection			*/
sqm_lst_t **au_list	/* return - list of allocatable unit	*/
)
{
	return (_discover_aus(ctx, au_list, B_TRUE)); // available ones only
}

/*
 * discover_aus
 *
 * get AU-s seen by this host.
 */
int
discover_aus(
ctx_t *ctx,		/* client connection			*/
sqm_lst_t **au_list	/* return - list of allocatable unit	*/
)
{
	return (_discover_aus(ctx, au_list, B_FALSE));
}


/*
 * discover_avail_aus_by_type
 *
 *  get AU-s that have the specified type seen by this host.
 *  filter out those that are known to be in use.
 */
int
discover_avail_aus_by_type(
ctx_t *ctx,		/* client connection			*/
au_type_t type,		/* type - (slice, svm, vxvm)		*/
sqm_lst_t **au_list	/* return - list of allocatable units	*/
)
{

	int ret_val;
	au_type_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:discover available aus by type";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(au_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.type = type;

	SAMRPC_CLNT_CALL(samrpc_discover_avail_aus_by_type, au_type_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*au_list = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d] and [%d]aus...",
	    func_name, ret_val, (*au_list)->length);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * discover_ha_aus
 *
 *  get HA AU-s seen by the specified hosts.
 *  if availOnly is true, then filter out those that are known to be in use.
 */
int
discover_ha_aus(
ctx_t *ctx,		/* client connection			*/
sqm_lst_t *hosts,
boolean_t availOnly,
sqm_lst_t **au_list	/* return - list of allocatable units	*/
)
{

	int ret_val;
	strlst_bool_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:discover ha aus";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(au_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.strlst = hosts;
	arg.bool = availOnly;

	SAMRPC_CLNT_CALL(samrpc_discover_ha_aus, strlst_bool_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*au_list = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d] and [%d]aus...",
	    func_name, ret_val, (*au_list)->length);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * discover_media_unused_in_mcf
 *
 * discover all the direct-attached libraries that sam can potentially
 * control and exclude those which have been already defined in mcf.
 * If nothing is found, return emtpy list (empty does not indicate
 * failure)
 *
 * remaining drive list will be all drive list minus library drive list
 * minus network attached library drive list
 */
int
discover_media_unused_in_mcf(
ctx_t *ctx,			/* client connection		*/
sqm_lst_t **library_list)		/* return - list of library	*/
{
	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:discover media unused in mcf";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(library_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);
	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_discover_media_unused_in_mcf, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*library_list = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * discover_library_by_path
 *
 * given a library_path, we do discovery.  It can be used for
 * the second discovery after finding device busy during the
 * first general discovery
 */
int
discover_library_by_path(
ctx_t *ctx,			/* client connection	*/
library_t **library,		/* return - library	*/
upath_t library_path	/* path	of library	*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:discover library by path";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(library, library_path)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.str = (char *)library_path;

	SAMRPC_CLNT_CALL(samrpc_discover_library_by_path, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*library = (library_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * discover_standalone_drive_by_path
 *
 * given a standalone_drive, we do discovery. It can be
 * used for the second discovery after finding
 * device busy during the first general discovery.
 */
int
discover_standalone_drive_by_path(
ctx_t *ctx,			/* client connection	*/
drive_t **drive,			/* return - drive	*/
upath_t drive_path	/* path of drive	*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:discover standalone drive by path";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(drive, drive_path)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.str = (char *)drive_path;

	SAMRPC_CLNT_CALL(samrpc_discover_standalone_drive_by_path,
	    string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*drive = (drive_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_all_libraries
 *
 * get all the libraries that sam is controlling.
 */
int
get_all_libraries(
ctx_t *ctx,		/* client connection		*/
sqm_lst_t **library_list	/* return - list of libraries	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all libraries";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(library_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_all_libraries, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*library_list = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_library_by_path
 *
 * get the library information given the library path
 */
int
get_library_by_path(
ctx_t *ctx,			/* client connection	*/
const upath_t library_name,	/* path	of library	*/
library_t **library		/* return - library	*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get library by path";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(library_name, library)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.str = (char *)library_name;

	SAMRPC_CLNT_CALL(samrpc_get_library_by_path, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*library = (library_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_library_by_family_set
 *
 * get the library information, given the library family set
 */
int
get_library_by_family_set(
ctx_t *ctx,				/* client connection	*/
const uname_t library_family_set,	/* library family set	*/
library_t **library			/* return - library	*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get library by family set";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(library_family_set, library)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.str = (char *)library_family_set;

	SAMRPC_CLNT_CALL(samrpc_get_library_by_family_set, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*library = (library_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get_library_by_equ
 *
 * get the library information, given the library ordinal number
 */
int
get_library_by_equ(
ctx_t *ctx,		/* client connection	*/
equ_t library_eq,	/* equipment ordinal	*/
library_t **library	/* return - library	*/
)
{

	int ret_val;
	equ_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get library by equ";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(library)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.eq = library_eq;

	SAMRPC_CLNT_CALL(samrpc_get_library_by_equ, equ_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*library = (library_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * add_library
 *
 * add a direct-attached library to sam's control.
 */
int
add_library(
ctx_t *ctx,		/* client connection	*/
library_t *library	/* library		*/
)
{

	int ret_val;
	library_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:add library";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(library)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.lib = library;

	SAMRPC_CLNT_CALL(samrpc_add_library, library_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * remove_library
 *
 * remove a library from sam's control;
 */
int
remove_library(
ctx_t *ctx,		/* client connection	*/
equ_t library_eq,	/* equipment ordinal	*/
boolean_t unload_first	/* unload the library	*/
)
{

	int ret_val;
	equ_bool_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:remove library";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.eq = library_eq;
	arg.bool = unload_first;

	SAMRPC_CLNT_CALL(samrpc_remove_library, equ_bool_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_all_standalone_drives
 *
 * get all the standalone drives.
 */
int
get_all_standalone_drives(
ctx_t *ctx,		/* client connection		*/
sqm_lst_t **drive_list	/* return - list of drive	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all standalone drives";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(drive_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	SAMRPC_CLNT_CALL(samrpc_get_all_standalone_drives, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*drive_list = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_no_of_catalog_entries
 *
 * get the number of catalog entires in a library.
 */
int
get_no_of_catalog_entries(
ctx_t *ctx,			/* client connection		*/
equ_t lib_eq,			/* equipment ordinal of library	*/
int *number_of_catalog_entries	/* return - number of entries	*/
)
{
	int ret_val;
	equ_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get no of catalog entries";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.eq = lib_eq;

	SAMRPC_CLNT_CALL(samrpc_get_no_of_catalog_entries, equ_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);
	ret_val = result.status;
	*number_of_catalog_entries = *((int *)
	    result.samrpc_result_u.result.result_data);

	free(result.samrpc_result_u.result.result_data);

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * rb_auditslot_from_eq
 *
 * audit slot in a robot
 */
int
rb_auditslot_from_eq(
ctx_t *ctx,		/* client connection	*/
const equ_t eq,		/* equipment ordianal	*/
const int slot,		/* slot number		*/
const int partition,	/* partition		*/
boolean_t eod
)
{

	int ret_val;
	equ_slot_part_bool_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:auditslot";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.eq = eq;
	arg.slot = slot;
	arg.partition = partition;
	arg.bool = eod;

	SAMRPC_CLNT_CALL(samrpc_rb_auditslot_from_eq, equ_slot_part_bool_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_catalog_entry
 *
 * get catalog entries when the vsn is given
 */
int
get_catalog_entry(
ctx_t *ctx,			/* client connection		*/
const vsn_t vsn,		/* vsn name */
sqm_lst_t **catalog_entry_list	/* return - list of catalog entries */
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get catalog entry";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(vsn, catalog_entry_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.str = (char *)vsn;

	SAMRPC_CLNT_CALL(samrpc_get_catalog_entry, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*catalog_entry_list = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_catalog_entry_by_media_type
 *
 * get a catalog entry when the vsn and type are given
 */
int
get_catalog_entry_by_media_type(
ctx_t *ctx,				/* client connection */
vsn_t vsn,				/* vsn name */
mtype_t type,				/* media type */
struct CatalogEntry **catalog_entry	/* return - catalog entry */
)
{

	int ret_val;
	string_mtype_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get catalog entry by media type";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(vsn, type, catalog_entry)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.str = (char *)vsn;
	arg.type = (char *)type;

	SAMRPC_CLNT_CALL(samrpc_get_catalog_entry_by_media_type,
	    string_mtype_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*catalog_entry = (struct CatalogEntry *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_catalog_entry_from_lib
 *
 * get a catalog entry when the equipment ordinal, slot and partition
 * of the library is given
 */
int
get_catalog_entry_from_lib(
ctx_t *ctx,				/* client connection	*/
const equ_t library_eq,			/* equipment ordinal	*/
const int slot,				/* slot number		*/
const int partition,			/* partition		*/
struct CatalogEntry **catalog_entry	/* return - catalog entry */
)
{

	int ret_val;
	equ_slot_part_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get catalog entry from lib";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(catalog_entry)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.eq = library_eq;
	arg.slot = slot;
	arg.partition = partition;

	SAMRPC_CLNT_CALL(samrpc_get_catalog_entry_from_lib,
	    equ_slot_part_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*catalog_entry = (struct CatalogEntry *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_all_catalog_entries
 *
 * get all the catalog entries of a library.
 */
int
get_all_catalog_entries(
ctx_t *ctx,			/* client connection	*/
equ_t lib_eq,			/* equipment ordinal	*/
int start,			/* starting index in the list */
int size,		/* num of entries to return, -1: all remaining */
vsn_sort_key_t sort_key,	/* sort key */
boolean_t ascending,		/* ascending order */
sqm_lst_t **catalog_entry_list	/* return - list of catalog entries */
)
{

	int ret_val;
	equ_sort_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all catalog entries";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(catalog_entry_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.eq = lib_eq;
	arg.start = start;
	arg.size = size;
	arg.sort_key = sort_key;
	arg.ascending = ascending;

	SAMRPC_CLNT_CALL(samrpc_get_all_catalog_entries, equ_sort_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*catalog_entry_list = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * rb_chmed_flags_from_eq
 *
 * set or clear the flags for the given volume
 *
 *	The following flags are used.
 *	A	needs audit
 *	C	element address contains cleaning cartridge
 *	E	volume is bad
 *	N	volume is not in SAM format
 *	R	volume is read-only (software flag)
 *	U	volume is unavailable (historian only)
 *	W	volume is physically write-protected
 *	X	slot is an export slot
 *	b	volume has a bar code
 *	c	volume is scheduled for recycling
 *	f	volume found full by archiver
 *	i	element address in use
 *	d	volume has a duplicate vsn
 *	l	volume is labeled
 *	o	slot is occupied
 *	p	high priority volume
 */
int
rb_chmed_flags_from_eq(
ctx_t *ctx,		/* client connection	*/
const equ_t eq,		/* equipment ordinal	*/
const int slot,		/* slot number		*/
boolean_t set,		/* set /unset flag	*/
uint32_t mask		/* mask to be set	*/
)
{

	int ret_val;
	chmed_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:change flags for eq";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.eq = eq;
	arg.slot = slot;
	arg.set = set;
	arg.mask = mask;

	SAMRPC_CLNT_CALL(samrpc_rb_chmed_flags_from_eq, chmed_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * rb_clean_drive
 *
 * Clean drive in media changer
 *
 */
int
rb_clean_drive(
ctx_t *ctx,		/* client connection	*/
equ_t eq		/* equipment ordinal	*/
)
{

	int ret_val;
	equ_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:clean drive";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.eq = eq;

	SAMRPC_CLNT_CALL(samrpc_rb_clean_drive, equ_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * rb_import
 *
 * import cartridges into a library or the historian
 *
 */
int
rb_import(
ctx_t *ctx,			/* client connection	*/
equ_t eq,			/* equipment ordinal	*/
const import_option_t *options	/* import option	*/
)
{

	int ret_val;
	import_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:import media";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.eq = eq;
	arg.options = (import_option_t *)options;

	SAMRPC_CLNT_CALL(samrpc_rb_import, import_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * rb_export_from_eq
 *
 * export a cartridge from a robot
 *
 */
int
rb_export_from_eq(
ctx_t *ctx,		/* client connection	*/
const equ_t eq,		/* equipment ordinal	*/
const int slot,		/* slot number		*/
boolean_t STK_one_step	/* one step ?		*/
)
{

	int ret_val;
	equ_slot_bool_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:export media";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.eq = eq;
	arg.slot = slot;
	arg.bool = STK_one_step;

	SAMRPC_CLNT_CALL(samrpc_rb_export_from_eq, equ_slot_bool_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * rb_unload
 *
 * unload media from a device
 *
 */
int
rb_unload(
ctx_t *ctx,		/* client connection		*/
equ_t eq,		/* equipment ordinal		*/
boolean_t wait		/* wait to complete unload ?	*/
)
{

	int ret_val;
	equ_bool_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:unload media";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.eq = eq;
	arg.bool = wait;

	SAMRPC_CLNT_CALL(samrpc_rb_unload, equ_bool_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * rb_load_from_eq
 *
 * load media into a device
 *
 */
int
rb_load_from_eq(
ctx_t *ctx,		/* client connection		*/
const equ_t eq,		/* equipment ordinal		*/
const int slot,		/* slot number			*/
const int partition,	/* partition			*/
boolean_t wait		/* wait to complete load ? */
)
{

	int ret_val;
	equ_slot_part_bool_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:load media";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.slot = slot;
	arg.eq = eq;
	arg.partition = partition;
	arg.bool = wait;

	SAMRPC_CLNT_CALL(samrpc_rb_load_from_eq, equ_slot_part_bool_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * rb_move_from_eq
 *
 * move a cartridge in a library
 */
int
rb_move_from_eq(
ctx_t *ctx,		/* client connection		*/
const equ_t eq,		/* equipment ordinal		*/
const int slot,		/* from slot number		*/
const int dest_slot	/* to/destination slot number	*/
)
{

	int ret_val;
	move_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:move media";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.slot = slot;
	arg.eq = eq;
	arg.dest_slot = dest_slot;

	SAMRPC_CLNT_CALL(samrpc_rb_move_from_eq, move_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 *	rp_tplabel_from_eq
 *
 *      label tape
 *
 */
int
rb_tplabel_from_eq(
ctx_t *ctx,		/* client connection			*/
const equ_t eq,		/* equipment ordinal			*/
const int slot,		/* slot number				*/
const int partition,	/* partition				*/
vsn_t new_vsn,		/* new VSN for the tape being labeled	*/
vsn_t old_vsn,		/* previously labeled ? old VSN : NULL	*/
uint_t blksize,		/* in units of 1024			*/
boolean_t wait,		/* wait to complete label operation ?	*/
boolean_t erase		/* erase the media before labeling ?	*/
)
{

	int ret_val;
	tplabel_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:Label media";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(new_vsn)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.eq = eq;
	arg.slot = slot;
	arg.partition = partition;
	strcpy(arg.new_vsn, new_vsn);
	arg.old_vsn = (char *)old_vsn;
	arg.blksize = blksize;
	arg.wait = wait;
	arg.erase = erase;

	SAMRPC_CLNT_CALL(samrpc_rb_tplabel_from_eq, tplabel_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * rb_unreserve_from_eq
 *
 * unreserve a volume for archiving
 */
int
rb_unreserve_from_eq(
ctx_t *ctx,		/* client connection */
const equ_t eq,		/* equimpent ordinal */
const int slot,		/* slot number */
const int partition	/* partition */
)
{

	int ret_val;
	equ_slot_part_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:unreserve media";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.eq = eq;
	arg.slot = slot;
	arg.partition = partition;

	SAMRPC_CLNT_CALL(samrpc_rb_unreserve_from_eq, equ_slot_part_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * change_state
 *
 * change to a new state (on, off, unavailable, down)
 */
int
change_state(
ctx_t *ctx,		/* client connection	*/
const equ_t eq,		/* equimpent ordinal	*/
dstate_t new_state	/* new state */
)
{

	int ret_val;
	equ_dstate_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:change state";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.eq = eq;
	arg.state = new_state;

	SAMRPC_CLNT_CALL(samrpc_change_state, equ_dstate_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * is_vsn_loaded
 *
 * If a vsn is loaded, the loaded drive is returned, else an empty drive
 * is returned
 */
int
is_vsn_loaded(
ctx_t *ctx,		/* client connection */
vsn_t vsn,		/* vsn name */
drive_t	**loaded_drive	/* return - if loaded ?		*/
			/* loaded drive : empty drive	*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:is vsn loaded";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(vsn, loaded_drive)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.str = (char *)vsn;

	SAMRPC_CLNT_CALL(samrpc_is_vsn_loaded, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*loaded_drive = (drive_t *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_total_capacity_of_library
 *
 * based on all catalog in the library, get the total capacity
 */
int
get_total_capacity_of_library(
ctx_t *ctx,		/* client connection */
const equ_t eq,		/* equimpent ordinal */
fsize_t *capacity	/* return - total capacity	*/
)
{

	int ret_val;
	equ_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get total capacity of library";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(capacity)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.eq = eq;

	SAMRPC_CLNT_CALL(samrpc_get_total_capacity_of_library, equ_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*capacity = *((fsize_t *)result.samrpc_result_u.result.result_data);

	free(result.samrpc_result_u.result.result_data);

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_free_space_of_library
 *
 * based on all catalog in the library, get the free space available
 */
int
get_free_space_of_library(
ctx_t *ctx,		/* client connection	*/
const equ_t eq,		/* equimpent ordinal	*/
fsize_t *space		/* return - free space	*/
)
{

	int ret_val;
	equ_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get free space of library";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(space)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.eq = eq;

	SAMRPC_CLNT_CALL(samrpc_get_free_space_of_library, equ_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*space = *((fsize_t *)result.samrpc_result_u.result.result_data);

	free(result.samrpc_result_u.result.result_data);

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_tape_label_running_list
 *
 * get a list of drives for which labeling is completed
 */
int
get_tape_label_running_list(
ctx_t *ctx,				/* client connection */
sqm_lst_t **tape_label_running_list	/* return - list of drives */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get tplabel drives";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(tape_label_running_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_tape_label_running_list, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*tape_label_running_list = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_nw_library
 *
 * given network attached library's information: parameter file, eq
 * number, catalog location, library name, network attached library type,
 * get that network attached library's property
 */
int
get_nw_library(
ctx_t *ctx,				/* client connection */
nwlib_req_info_t *nwlib_req_info,	/* nw lib info */
library_t **nw_lib			/* return - lib property */
)
{

	int ret_val;
	nwlib_req_info_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get nw library";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(nwlib_req_info, nw_lib)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.nwlib_req_info = nwlib_req_info;

	SAMRPC_CLNT_CALL(samrpc_get_nw_library, nwlib_req_info_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*nw_lib = (library_t *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_vsn_list
 *
 * given vsn's regular expression, get a list of all matched vsns
 */
int
get_vsn_list(
ctx_t *ctx,			/* client connection		*/
const char *vsn_reg_exp,	/* vsn's regular expr		*/
int start,			/* starting index in the list */
int size,		/* num of entries to return, -1: all remaining */
vsn_sort_key_t sort_key,	/* sort key */
boolean_t ascending,		/* ascending order */
sqm_lst_t **catalog_entry_list	/* return - list of matched vsn	*/
)
{

	int ret_val;
	string_sort_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get vsn list";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(vsn_reg_exp, catalog_entry_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.str = (char *)vsn_reg_exp;
	arg.start = start;
	arg.size = size;
	arg.sort_key = sort_key;
	arg.ascending = ascending;

	SAMRPC_CLNT_CALL(samrpc_get_vsn_list, string_sort_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*catalog_entry_list = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_all_available_media_type
 *
 * checks the live system and returns a list of media types
 */
int
get_all_available_media_type(
ctx_t *ctx,			/* client connection */
sqm_lst_t **media_type_list	/* return - list of media types	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all available media type";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(media_type_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_all_available_media_type, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*media_type_list = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_available_vsns
 *
 * given a archive vsn pool name, get a list of all
 * all vsns inside that vsn pool with free space
 */
int
get_available_vsns(
ctx_t *ctx,			/* client connection */
upath_t archive_vsn_pool_name,	/* pool name */
int start,			/* starting index in the list */
int size,		/* num of entries to return, -1: all remaining */
vsn_sort_key_t sort_key,	/* sort key */
boolean_t ascending,		/* ascending order */
sqm_lst_t **catalog_entry_list	/* return - list of vsn	*/
)
{

	int ret_val;
	string_sort_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get available vsns";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(archive_vsn_pool_name, catalog_entry_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.str = (char *)archive_vsn_pool_name;
	arg.start = start;
	arg.size = size;
	arg.sort_key = sort_key;
	arg.ascending = ascending;

	SAMRPC_CLNT_CALL(samrpc_get_available_vsns, string_sort_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*catalog_entry_list = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_properties_of_archive_vsnpool
 *
 * get the properties of a vsn pool
 */
int
get_properties_of_archive_vsnpool(
ctx_t *ctx,				/* client connection */
const upath_t pool_name,		/* pool name */
int start,				/* starting index in the list */
int size,		/* num of entries to return, -1: all remaining */
vsn_sort_key_t sort_key,		/* sort key */
boolean_t ascending,			/* ascending order */
vsnpool_property_t **vsnpool_prop	/* return - pool property */
)
{

	int ret_val;
	string_sort_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get properties of archive vsnpool";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(pool_name, vsnpool_prop)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.str = (char *)pool_name;
	arg.start = start;
	arg.size = size;
	arg.sort_key = sort_key;
	arg.ascending = ascending;

	SAMRPC_CLNT_CALL(samrpc_get_properties_of_archive_vsnpool,
	    string_sort_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*vsnpool_prop = (vsnpool_property_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * rb_reserve_from_eq
 *
 * Reserve a volume for archiving
 */
int
rb_reserve_from_eq(
ctx_t *ctx,				/* client connection */
const equ_t eq,				/* equipment ordinal */
const int slot,				/* slot number */
const int partition,			/* partition */
const reserve_option_t *reserve_content	/* reserve options */
)
{

	int ret_val;
	reserve_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:reserve media";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(reserve_content)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.eq = eq;
	arg.slot = slot;
	arg.partition = partition;
	arg.reserve_option = (reserve_option_t *)reserve_content;

	SAMRPC_CLNT_CALL(samrpc_rb_reserve_from_eq, reserve_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_default_media_block_size
 *
 * given a media type, get the media's default block size.
 */
int
get_default_media_block_size(
ctx_t *ctx,			/* client connection		*/
mtype_t media_type,		/* media type			*/
int *def_blksize		/* return - default block size	*/
)
{
	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get default media block size";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(media_type, def_blksize)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.str = (char *)media_type;

	SAMRPC_CLNT_CALL(samrpc_get_default_media_block_size, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*def_blksize = *((int *)result.samrpc_result_u.result.result_data);

	free(result.samrpc_result_u.result.result_data);

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * import_all
 *
 * import all VSNs from the beginning vsn to the end vsn
 * All import use the same option
 */
int
import_all(
ctx_t *ctx,			/* client connection		*/
vsn_t begin_vsn,		/* begin vsn */
vsn_t end_vsn,			/* end vsn */
int *total_num,			/* return - total num of vsn's imported */
equ_t eq,			/* equipment ordinal */
import_option_t	*options	/* import options */
)
{
	int ret_val;
	import_range_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:import all";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(begin_vsn, end_vsn, total_num, options)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	strcpy(arg.begin_vsn, begin_vsn);
	strcpy(arg.end_vsn, end_vsn);
	arg.eq = eq;
	arg.options = options;

	SAMRPC_CLNT_CALL(samrpc_import_all, import_range_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*total_num = *((int *)result.samrpc_result_u.result.result_data);

	free(result.samrpc_result_u.result.result_data);

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * check_slices_for_overlaps
 *
 * given a list of slices (/dev/dsk/...), check if the slices
 * overlap.
 * result will include a subset of the input - slices that overlap
 *	with 1/more other slices.
 *	return code:
 *	 0 = no overlaps (result will be an empty list).
 *	-1 = an error occured
 *	>0 = the number of user-specified slices that overlap some other slices
 */
int
check_slices_for_overlaps(
ctx_t *ctx,		/* client connection */
sqm_lst_t *slices,	/* list of slices(strings) to check for overlap */
sqm_lst_t **overlap_slices /* return - list of overlapping slices */
)
{

	int ret_val;
	string_list_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:Check if slices overlap";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(slices, overlap_slices)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.strings = slices;

	SAMRPC_CLNT_CALL(samrpc_check_slices_for_overlaps, string_list_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*overlap_slices = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get_vsn_pool_properties
 *
 * get the properties of a vsn pool
 * If pool->media_type == "dk", then vsnpool_property->catalog_entry_list
 * will contain disk_vol_list, else it would contain list of CatalogEntry
 */
int
get_vsn_pool_properties(
ctx_t *ctx,				/* client connection */
vsn_pool_t *pool,			/* pool name and type */
int start,				/* starting index in the list */
int size,		/* num of entries to return, -1: all remaining */
vsn_sort_key_t sort_key,		/* sort key */
boolean_t ascending,			/* ascending order */
vsnpool_property_t **vsnpool_prop	/* return - pool property */
)
{

	int ret_val;
	vsnpool_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get properties of archive vsnpool";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(pool, vsnpool_prop)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.pool = pool;
	arg.start = start;
	arg.size = size;
	arg.sort_key = sort_key;
	arg.ascending = ascending;

	SAMRPC_CLNT_CALL(samrpc_get_vsn_pool_properties, vsnpool_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*vsnpool_prop = (vsnpool_property_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get_vsn_map_properties
 *
 * get the properties of a vsn map
 * If pool->media_type == "dk", then vsnpool_property->catalog_entry_list
 * will contain disk_vol_list, else it would contain list of CatalogEntry
 */
int
get_vsn_map_properties(
ctx_t *ctx,				/* client connection */
vsn_map_t *map,				/* name and type */
int start,				/* starting index in the list */
int size,		/* num of entries to return, -1: all remaining */
vsn_sort_key_t sort_key,		/* sort key */
boolean_t ascending,			/* ascending order */
vsnpool_property_t **vsnpool_prop	/* return - pool property */
)
{

	int ret_val;
	vsnmap_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get properties of vsnmap";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(map, vsnpool_prop)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.map = map;
	arg.start = start;
	arg.size = size;
	arg.sort_key = sort_key;
	arg.ascending = ascending;

	SAMRPC_CLNT_CALL(samrpc_get_vsn_map_properties, vsnmap_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*vsnpool_prop = (vsnpool_property_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get_stk_vsn_names
 *
 * gets a list of vsn names (list of strings) that are imported to this library
 */
int
get_stk_vsn_names(
ctx_t *ctx,				/* client connection */
devtype_t equ_type,			/* type */
sqm_lst_t **vsn_names			/* return - list of vsn names */
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get vsns that are imported in acsls";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(vsn_names)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.str = strdup(equ_type);

	SAMRPC_CLNT_CALL(samrpc_get_stk_vsn_names, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*vsn_names = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * discover_stk
 *
 *
 */
int
discover_stk(
ctx_t *ctx,				/* client connection */
sqm_lst_t *stk_host_list,			/* list of stk_host_info_t */
sqm_lst_t **stk_library_list		/* return - list of library_t */
)
{

	int ret_val;
	stk_host_list_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:discover stk";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(stk_host_list, stk_library_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.stk_host_lst = stk_host_list;

	SAMRPC_CLNT_CALL(samrpc_discover_stk, stk_host_list_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*stk_library_list = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get_stk_volume_list
 *
 * get all the volume that are imported to acsls and match the criteria
 * specified in the in_str
 *
 * in_str is set of comma-delimited name-value pairs, as below:
 *
 *  Required key values:-
 *   access_date  = access date of vsn (should be set to local date by default)
 *                yyyy-mm-dd or yyyy-mm-dd-yyyy-mm-dd
 *   equ_type           = media type
 *   filter_type        = one of the following:-
 *                      FILTER_BY_SCRATCH_POOL,
 *                      FILTER_BY_VSN_RANGE,
 *                      FILTER_BY_VSN_EXPRESSION,
 *                      FILTER_BY_PHYSICAL_LOCATION,
 *                      NO_FILTER
 *
 *  Optional key values:-
 *  for FILTER_BY_SCRATCH_POOL
 *   scratch_pool_id    = id
 *  for FILTER_BY_VSN_RANGE
 *   start_vsn          = starting vsn number
 *   end_vsn            = ending vsn number
 *  for FILTER_BY_VSN_EXPRESSION
 *   vsn_expression     = pattern
 *  for FILTER_BY_PHYSICAL_LOCATION
 *   lsm                = lsm number
 *   panel              = panel number
 *   start_row          = search for vsn from row
 *   end_row            = search for vsn till row
 *   start_col          = search for vsn from column
 *   end_col            = search for vsn till column
 *
 */
int
get_stk_filter_volume_list(
ctx_t *ctx,			/* client connection */
stk_host_info_t *stk_host_info, /* stk host information */
char *in_str,			/* key-value pairs */
sqm_lst_t **stk_volume_list	/* return - list of stk_volume_t */
)
{

	int ret_val;
	stk_host_info_string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all imported stk vsn";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(stk_volume_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.stk_host_info = stk_host_info;
	arg.str = in_str;

	SAMRPC_CLNT_CALL(samrpc_get_stk_filter_volume_list,
	    stk_host_info_string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*stk_volume_list = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);

	return (ret_val);
}


/*
 * add_list_libraries
 *
 * add a list of library to sam's control.
 */
int
add_list_libraries(
ctx_t *ctx,		/* client connection	*/
sqm_lst_t *library_list	/* list of libraries		*/
)
{

	int ret_val;
	library_list_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:add list of library";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(library_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.library_lst = library_list;

	SAMRPC_CLNT_CALL(samrpc_add_list_libraries, library_list_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * import_stk_vsns
 *
 * import cartridges into the stk library
 *
 */
int
import_stk_vsns(
ctx_t *ctx,			/* client connection	*/
equ_t eq,			/* equipment ordinal	*/
import_option_t *options,	/* import option	*/
sqm_lst_t *vsn_list		/* list of vsns */
)
{

	int ret_val;
	import_vsns_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:import stk vsns";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.eq = eq;
	arg.options = (import_option_t *)options;
	arg.vsn_list = vsn_list;

	SAMRPC_CLNT_CALL(samrpc_import_stk_vsns, import_vsns_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_stk_phyconf_info
 *
 * gets physical configuration (lsm, panel, cell, pool)
 * information for the STK
 */
int
get_stk_phyconf_info(
ctx_t *ctx,				/* client connection */
stk_host_info_t *stk_host_info,		/* stk host info */
devtype_t equ_type,			/* type */
stk_phyconf_info_t **stk_phyconf_info	/* return - stk physical conf info */
)
{

	int ret_val;
	stk_host_info_string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get skt phyconf info";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(stk_phyconf_info, stk_host_info, equ_type)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.str = (char *)equ_type;
	arg.stk_host_info = stk_host_info;

	SAMRPC_CLNT_CALL(samrpc_get_stk_phyconf_info,
	    stk_host_info_string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*stk_phyconf_info = (stk_phyconf_info_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * share/unshare stk drive
 * modifies stk drive's share status
 */
int
modify_stkdrive_share_status(
ctx_t *ctx,				/* client connection */
equ_t lib_eq,		/* library eq number */
equ_t drive_eq,			/* drive eq number */
boolean_t shared	/* should drive be shared */
)
{

	int ret_val;
	equ_equ_bool_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:modify stk drive share status";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.eq1 = lib_eq;
	arg.eq2 = drive_eq;
	arg.bool = shared;

	SAMRPC_CLNT_CALL(samrpc_modify_stkdrive_share_status,
	    equ_equ_bool_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * Get the vsns for that have the given status
 * (one or more of the status bit flags are set)
 * This can be used to check for unusable vsns
 *
 * equ_t lib_eq         - equipment ordinal of library,
 *                      if EQU_MAX, get vsns from all lib
 * uint32_t flags       - status field bit flags
 *                      if 0, default RM_VOL_UNUSABLE_STATUS
 *                      (from src/archiver/include/volume.h)
 *                      CES_needs_audit
 *                      CES_cleaning
 *                      CES_dupvsn
 *                      CES_unavail
 *                      CES_non_sam
 *                      CES_bad_media
 *                      CES_read_only
 *                      CES_writeprotect
 *                      CES_archfull
 *
 * Returns a list of formatted strings
 *      name=vsn
 *      type=mediatype
 *      flags=intValue representing flags that are set
 *
 */
int
get_vsns(
ctx_t *ctx,		/* client connection	*/
const equ_t eq,		/* equipment ordinal	*/
uint32_t flags,		/* flags to match against */
sqm_lst_t **strlst)
{

	int ret_val;
	int_uint32_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get vsns with certain flags set";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;
	arg.i = eq;
	arg.u_flag = flags;

	SAMRPC_CLNT_CALL(samrpc_get_vsns, int_uint32_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*strlst = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...",
	    func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}
