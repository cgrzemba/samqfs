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

#pragma ident	"$Revision: 1.44 $"

#include "mgmt/sammgmt.h"
#include <stdlib.h>
#include <dlfcn.h>

#define	STKAPILIB	SAM_DIR"/lib/libstkapi.so"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;


samrpc_result_t *
samrpc_discover_avail_aus_1_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Discover all available allocatable units");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to discover all au");
	ret = discover_avail_aus(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_DEV_AU_LIST, lst);

	Trace(TR_DEBUG, "Discovered available allocatable units return[%d]",
	    ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_discover_aus_1_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Discover all allocatable units");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to discover all au");
	ret = discover_aus(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_DEV_AU_LIST, lst);

	Trace(TR_DEBUG, "Discovered allocatable units return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_discover_avail_aus_by_type_1_svr(
	au_type_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Discover available allocatable units of type[%d]",
	    (int)arg->type);

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to discover available au of type[%d]",
	    (int)arg->type);
	ret = discover_avail_aus_by_type(arg->ctx, arg->type, &lst);

	SAMRPC_SET_RESULT(ret, SAM_DEV_AU_LIST, lst);

	Trace(TR_DEBUG, "Discovered[%d] allocatable units of type[%d]",
	    (ret == -1) ? ret : lst->length, (int)arg->type);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_discover_ha_aus_5_svr(
	strlst_bool_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Discover ha allocatable units");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to discover ha au-s");
	ret = discover_ha_aus(arg->ctx, arg->strlst, arg->bool, &lst);

	SAMRPC_SET_RESULT(ret, SAM_DEV_AU_LIST, lst);

	Trace(TR_DEBUG, "Discovered ha allocatable units return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_discover_media_unused_in_mcf_1_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Discover media not used by SAM");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to discover media not used by SAM");
	ret = discover_media_unused_in_mcf(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_DEV_LIBRARY_LIST, lst);

	Trace(TR_DEBUG, "Discovered libraries[%d] not used by SAM",
	    (lst == NULL) ? -1 : lst->length);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_discover_library_by_path_1_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	library_t *lib;

	Trace(TR_DEBUG, "Discover library by path[%s]", arg->str);

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to discover library by path");
	ret = discover_library_by_path(arg->ctx, &lib, arg->str);

	SAMRPC_SET_RESULT(ret, SAM_DEV_LIBRARY, lib);

	Trace(TR_DEBUG, "Discover library by path return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_discover_standalone_drive_by_path_1_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	drive_t *drive;

	Trace(TR_DEBUG, "Discover standalone drive by path[%s]", arg->str);

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to discover standalone drive by path");
	ret = discover_standalone_drive_by_path(arg->ctx, &drive, arg->str);

	SAMRPC_SET_RESULT(ret, SAM_DEV_DRIVE, drive);

	Trace(TR_DEBUG, "Discover standalone drive by path return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_all_libraries_1_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get all libraries");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get all libraries");
	ret = get_all_libraries(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_DEV_LIBRARY_LIST, lst);

	Trace(TR_DEBUG, "Get all libraries return[%d] with [%d] libraries",
	    ret, (ret == -1) ? ret : lst->length);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_library_by_path_1_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	library_t *lib;

	Trace(TR_DEBUG, "Get library by path[%s]", arg->str);

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get library by path");
	ret = get_library_by_path(arg->ctx, arg->str, &lib);

	SAMRPC_SET_RESULT(ret, SAM_DEV_LIBRARY, lib);

	Trace(TR_DEBUG, "Get library by path return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_library_by_family_set_1_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	library_t *lib;

	Trace(TR_DEBUG, "Get library by family set[%s]", arg->str);

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get library by family set");
	ret = get_library_by_family_set(arg->ctx, arg->str, &lib);

	SAMRPC_SET_RESULT(ret, SAM_DEV_LIBRARY, lib);

	Trace(TR_DEBUG, "Get library by family set return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_library_by_equ_1_svr(
	equ_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	library_t *lib;

	Trace(TR_DEBUG, "Get library by equ[%d]", arg->eq);

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get library by equ[%d]", arg->eq);
	ret = get_library_by_equ(arg->ctx, arg->eq, &lib);

	SAMRPC_SET_RESULT(ret, SAM_DEV_LIBRARY, lib);

	Trace(TR_DEBUG, "Get library by equ[%d] return[%d]", arg->eq, ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_add_library_1_svr(
	library_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Add library");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to add library");
	ret = add_library(arg->ctx, arg->lib);

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

	Trace(TR_DEBUG, "Add library return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_remove_library_1_svr(
	equ_bool_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Remove library");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to remove library");
	ret = remove_library(arg->ctx, arg->eq, arg->bool);

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

	Trace(TR_DEBUG, "Remove library return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_all_standalone_drives_1_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get all standalone drives");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get all standalone drives");
	ret = get_all_standalone_drives(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_DEV_DRIVE_LIST, lst);

	Trace(TR_DEBUG, "Get all standalone drives return[%d] with [%d] drives",
	    ret, (ret == -1) ? ret : lst->length);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_no_of_catalog_entries_1_svr(
	equ_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	int *num_entries;

	Trace(TR_DEBUG, "Get number of catalog entries");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	num_entries = (int *)mallocer(sizeof (int));
	if (num_entries != NULL) {

		Trace(TR_DEBUG, "Call native lib to get no of catalog entries");
		ret = get_no_of_catalog_entries(
		    arg->ctx, arg->eq, num_entries);
	}
	SAMRPC_SET_RESULT(ret, SAM_PTR_INT, num_entries);

	Trace(TR_DEBUG, "Get number of catalog entries return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_rb_auditslot_from_eq_1_svr(
	equ_slot_part_bool_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Audit slot[%d:%d:%d]",
	    arg->eq, arg->slot, arg->partition);

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);
	Trace(TR_DEBUG, "Call native lib to audit slot");
	ret = rb_auditslot_from_eq(
	    arg->ctx,
	    arg->eq,
	    arg->slot,
	    arg->partition,
	    arg->bool);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Audit slot[%d:%d:%d] return[%d]",
	    arg->eq, arg->slot, arg->partition, ret);

	return (&rpc_result);
}

samrpc_result_t *
samrpc_rb_chmed_flags_from_eq_1_svr(
	chmed_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Set or clear catalog flags for [%d:%d]",
	    arg->eq, arg->slot);

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to set/clear library catalog flags");
	ret = rb_chmed_flags_from_eq(
	    arg->ctx,
	    arg->eq,
	    arg->slot,
	    arg->set,
	    arg->mask);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Set or clear catalog flags for [%d:%d] return[%d]",
	    arg->eq, arg->slot, ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_rb_clean_drive_1_svr(
	equ_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Clean drive");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to clean drive");
	ret = rb_clean_drive(arg->ctx, arg->eq);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Clean drive return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_rb_import_1_svr(
	import_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Import cartridges into a library");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);
	Trace(TR_DEBUG, "Call native lib to import cartridges into a library");
	ret = rb_import(arg->ctx, arg->eq, arg->options);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Import cartridges into a library return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_import_all_1_svr(
	import_range_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	int *total_num = NULL;

	Trace(TR_DEBUG, "Import all cartridges into a library");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	total_num = (int *)mallocer(sizeof (int));
	if (total_num != NULL) {

		Trace(TR_DEBUG, "Call native lib to import all cartridges");
		ret = import_all(
		    arg->ctx,
		    arg->begin_vsn,
		    arg->end_vsn,
		    total_num,
		    arg->eq,
		    arg->options);
	}
	SAMRPC_SET_RESULT(ret, SAM_PTR_INT, total_num);

	Trace(TR_DEBUG, "Import all cartridges into a library return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_rb_export_from_eq_1_svr(
	equ_slot_bool_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Export cartridge from robot");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);
	Trace(TR_DEBUG, "Call native lib to export cartridges from robot");
	ret = rb_export_from_eq(
	    arg->ctx,
	    arg->eq,
	    arg->slot,
	    arg->bool);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);
	Trace(TR_DEBUG, "Export cartridges from robot return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_rb_load_from_eq_1_svr(
	equ_slot_part_bool_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Load media into a device");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to load media into a device");
	ret = rb_load_from_eq(
	    arg->ctx,
	    arg->eq,
	    arg->slot,
	    arg->partition,
	    arg->bool);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Load media into a device return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_rb_unload_1_svr(
	equ_bool_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Unload media from device");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to unload media from device");
	ret = rb_unload(arg->ctx, arg->eq, arg->bool);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Unload media from device return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_rb_move_from_eq_1_svr(
	move_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Move a cartridge in a library");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);
	Trace(TR_DEBUG, "Call native lib to move cartridge in a library");
	ret = rb_move_from_eq(
	    arg->ctx,
	    arg->eq,
	    arg->slot,
	    arg->dest_slot);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Move a cartridge in a library return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_rb_tplabel_from_eq_1_svr(
	tplabel_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Label tape");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);
	Trace(TR_DEBUG, "Call native lib to label tape");
	ret = rb_tplabel_from_eq(
	    arg->ctx,
	    arg->eq,
	    arg->slot,
	    arg->partition,
	    arg->new_vsn,
	    arg->old_vsn,
	    arg->blksize,
	    arg->wait,
	    arg->erase);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Label tape return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_rb_unreserve_from_eq_1_svr(
	equ_slot_part_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Unreserve a volume for archiving");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to unreserve a volume for archiving");
	ret = rb_unreserve_from_eq(
	    arg->ctx,
	    arg->eq,
	    arg->slot,
	    arg->partition);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Unreserve a volume for archiving return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_change_state_1_svr(
	equ_dstate_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Change state of media changer or drive");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to change state");
	ret = change_state(
	    arg->ctx,
	    arg->eq,
	    arg->state);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Change state return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_is_vsn_loaded_1_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	drive_t *drive;

	Trace(TR_DEBUG, "Is vsn loaded?");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to check if vsn is loaded");
	ret = is_vsn_loaded(
	    arg->ctx,
	    arg->str,
	    &drive);

	SAMRPC_SET_RESULT(ret, SAM_DEV_DRIVE, drive);

	Trace(TR_DEBUG, "Is vsn loaded return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_total_capacity_of_library_1_svr(
	equ_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	fsize_t *size;

	Trace(TR_DEBUG, "Get total capacity of library");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	size = (fsize_t *)mallocer(sizeof (fsize_t));
	if (size != NULL) {

		Trace(TR_DEBUG, "Call native lib to get total capacity of lib");
		ret = get_total_capacity_of_library(
		    arg->ctx,
		    arg->eq,
		    size);
	}
	SAMRPC_SET_RESULT(ret, SAM_DEV_SPACE, size);

	Trace(TR_DEBUG, "Get total capacity of library return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_free_space_of_library_1_svr(
	equ_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	fsize_t *space;

	Trace(TR_DEBUG, "Get free space of library");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	space = (fsize_t *)mallocer(sizeof (fsize_t));
	if (space != NULL) {

		Trace(TR_DEBUG, "Call native lib to get free space of library");
		ret = get_free_space_of_library(
		    arg->ctx,
		    arg->eq,
		    space);
	}
	SAMRPC_SET_RESULT(ret, SAM_DEV_SPACE, space);

	Trace(TR_DEBUG, "Get free space of library return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_vsn_list_1_svr(
	string_sort_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get a catalog list");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get a catalog list");
	ret = get_vsn_list(
	    arg->ctx,
	    arg->str,
	    arg->start,
	    arg->size,
	    arg->sort_key,
	    arg->ascending,
	    &lst);

	SAMRPC_SET_RESULT(ret, SAM_DEV_CATALOG_ENTRY_LIST, lst);

	Trace(TR_DEBUG, "Get a catalog list return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_tape_label_running_list_1_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get tplabel jobs");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get tplabel jobs");
	ret = get_tape_label_running_list(
	    arg->ctx,
	    &lst);

	SAMRPC_SET_RESULT(ret, SAM_DEV_DRIVE_LIST, lst);

	Trace(TR_DEBUG, "Get tplabel jobs return[%d] with [%d] jobs",
	    ret, (ret == -1) ? ret : lst->length);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_nw_library_1_svr(
	nwlib_req_info_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	library_t *lib;

	Trace(TR_DEBUG, "Get network library");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get network library");
	ret = get_nw_library(
	    arg->ctx,
	    arg->nwlib_req_info,
	    &lib);

	SAMRPC_SET_RESULT(ret, SAM_DEV_LIBRARY, lib);

	Trace(TR_DEBUG, "Get network library return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_all_available_media_type_1_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get all available media types");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get all available media types");
	ret = get_all_available_media_type(
	    arg->ctx,
	    &lst);

	SAMRPC_SET_RESULT(ret, SAM_DEV_STRING_LIST, lst);

	Trace(TR_DEBUG, "Get all available media types return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_properties_of_archive_vsnpool_1_svr(
	string_sort_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	vsnpool_property_t *vsnpool_prop;

	Trace(TR_DEBUG, "Get properties of archive vsnpool");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get properties of archive vsnpool");
	ret = get_properties_of_archive_vsnpool(
	    arg->ctx,
	    arg->str,
	    arg->start,
	    arg->size,
	    arg->sort_key,
	    arg->ascending,
	    &vsnpool_prop);

	SAMRPC_SET_RESULT(ret, SAM_DEV_VSNPOOL_PROPERTY, vsnpool_prop);

	Trace(TR_DEBUG, "Get properties of archive vsnpool return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_available_vsns_1_svr(
	string_sort_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get available vsns");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get available vsns");
	ret = get_available_vsns(
	    arg->ctx,
	    arg->str,
	    arg->start,
	    arg->size,
	    arg->sort_key,
	    arg->ascending,
	    &lst);

	SAMRPC_SET_RESULT(ret, SAM_DEV_CATALOG_ENTRY_LIST, lst);

	Trace(TR_DEBUG, "Get available vsns return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_catalog_entry_1_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get catalog entry");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get catalog entry");
	ret = get_catalog_entry(arg->ctx, arg->str, &lst);

	SAMRPC_SET_RESULT(ret, SAM_DEV_CATALOG_ENTRY_LIST, lst);

	Trace(TR_DEBUG, "Get catalog entry return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_catalog_entry_by_media_type_1_svr(
	string_mtype_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	struct CatalogEntry *cat;

	Trace(TR_DEBUG, "Get catalog entry by media type");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get catalog entry by media type");
	ret = get_catalog_entry_by_media_type(
	    arg->ctx,
	    arg->str,
	    arg->type,
	    &cat);

	SAMRPC_SET_RESULT(ret, SAM_DEV_CATALOG_ENTRY, cat);

	Trace(TR_DEBUG, "Get catalog entry by media type return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_catalog_entry_from_lib_1_svr(
	equ_slot_part_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	struct CatalogEntry *cat;

	Trace(TR_DEBUG, "Get catalog entry from library");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get catalog entry from library");
	ret = get_catalog_entry_from_lib(
	    arg->ctx,
	    arg->eq,
	    arg->slot,
	    arg->partition,
	    &cat);

	SAMRPC_SET_RESULT(ret, SAM_DEV_CATALOG_ENTRY, cat);

	Trace(TR_DEBUG, "Get catalog entry from library return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_all_catalog_entries_1_svr(
	equ_sort_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get all catalog entries");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get all catalog entries");
	ret = get_all_catalog_entries(
	    arg->ctx,
	    arg->eq,
	    arg->start,
	    arg->size,
	    arg->sort_key,
	    arg->ascending,
	    &lst);

	SAMRPC_SET_RESULT(ret, SAM_DEV_CATALOG_ENTRY_LIST, lst);

	Trace(TR_DEBUG, "Get all catalog entries return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_rb_reserve_from_eq_1_svr(
	reserve_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Reserve a volume for archiving");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);
	Trace(TR_DEBUG, "Call native lib to reserve a volume for archiving");
	ret = rb_reserve_from_eq(
	    arg->ctx,
	    arg->eq,
	    arg->slot,
	    arg->partition,
	    arg->reserve_option);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Reserve a volume for archiving return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_default_media_block_size_1_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	int *def_blksize;

	Trace(TR_DEBUG, "Get default media block size");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	def_blksize = (int *)mallocer(sizeof (int));
	if (def_blksize != NULL) {
		Trace(TR_DEBUG, "Call native lib to get def media blk size");
		ret = get_default_media_block_size(arg->ctx, arg->str,
		    def_blksize);
	}
	SAMRPC_SET_RESULT(ret, SAM_PTR_INT, def_blksize);

	Trace(TR_DEBUG, "Get default media block size return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_check_slices_for_overlaps_2_svr(
	string_list_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Check slices for overlap");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to check slices for overlap");
	ret = check_slices_for_overlaps(
	    arg->ctx,
	    arg->strings,
	    &lst);

	SAMRPC_SET_RESULT(ret, SAM_DEV_STRING_LIST, lst);

	Trace(TR_DEBUG, "Check slices for overlap return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_vsn_pool_properties_4_svr(
	vsnpool_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	vsnpool_property_t *vsnpool_prop = NULL;

	Trace(TR_DEBUG, "Get properties of vsnpool");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);
	Trace(TR_DEBUG, "Call native lib to get properties of vsnpool");
	ret = get_vsn_pool_properties(
	    arg->ctx,
	    arg->pool,
	    arg->start,
	    arg->size,
	    arg->sort_key,
	    arg->ascending,
	    &vsnpool_prop);

	if (strcmp(arg->pool->media_type, DISK_MEDIA) == 0 ||
	    strcmp(arg->pool->media_type, STK5800_MEDIA) == 0) {
		SAMRPC_SET_RESULT(ret, SAM_DISK_VSNPOOL_PROPERTY, vsnpool_prop);
	} else {
		SAMRPC_SET_RESULT(ret, SAM_DEV_VSNPOOL_PROPERTY, vsnpool_prop);
	}

	Trace(TR_DEBUG, "Get properties of vsnpool return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_vsn_map_properties_4_svr(
	vsnmap_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	vsnpool_property_t *vsnpool_prop = NULL;

	Trace(TR_DEBUG, "Get properties of vsnmap");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);
	Trace(TR_DEBUG, "Call native lib to get properties of vsnmap");
	ret = get_vsn_map_properties(
	    arg->ctx,
	    arg->map,
	    arg->start,
	    arg->size,
	    arg->sort_key,
	    arg->ascending,
	    &vsnpool_prop);

	if (strcmp(arg->map->media_type, DISK_MEDIA) == 0 ||
	    strcmp(arg->map->media_type, STK5800_MEDIA) == 0) {

		SAMRPC_SET_RESULT(ret, SAM_DISK_VSNPOOL_PROPERTY, vsnpool_prop);
	} else {
		SAMRPC_SET_RESULT(ret, SAM_DEV_VSNPOOL_PROPERTY, vsnpool_prop);
	}
	Trace(TR_DEBUG, "Get properties of vsnpool return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_stk_filter_volume_list_5_svr(
	stk_host_info_string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get stk volume list");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get stk volume list");
	ret = get_stk_filter_volume_list(
	    arg->ctx,
	    arg->stk_host_info,
	    arg->str,
	    &lst);

	SAMRPC_SET_RESULT(ret, SAM_STK_VSN_LIST, lst);

	Trace(TR_DEBUG, "Get stk volume list return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_stk_phyconf_info_5_svr(
	stk_host_info_string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	stk_phyconf_info_t *stk_phyconf_info = NULL;

	Trace(TR_DEBUG, "Get stk phyconf info");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get stk phyconf info");

	ret = get_stk_phyconf_info(arg->ctx, arg->stk_host_info, arg->str,
	    &stk_phyconf_info);

	SAMRPC_SET_RESULT(ret, SAM_STK_PHYCONF_INFO, stk_phyconf_info);
	return (&rpc_result);

}


samrpc_result_t *
samrpc_get_stk_vsn_names_5_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get vsn names imported in stk acsls");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get stk vsn names");
	ret = get_stk_vsn_names(
	    arg->ctx,
	    arg->str,
	    &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "Got vsn names imported in stk acsls");
	return (&rpc_result);
}


samrpc_result_t *
samrpc_discover_stk_5_svr(
	stk_host_list_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Discover stk drives");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	SET_LIST_TAIL(arg->stk_host_lst);

	Trace(TR_DEBUG, "Call native lib to discover stk drives");
	ret = discover_stk(arg->ctx, arg->stk_host_lst, &lst);

	SAMRPC_SET_RESULT(ret, SAM_DEV_LIBRARY_LIST, lst);

	Trace(TR_DEBUG, "Discover stk return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_add_list_libraries_5_svr(
	library_list_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Add list of libraries");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL(arg->library_lst);

	Trace(TR_DEBUG, "Call native lib to list of stk media");
	ret = add_list_libraries(
	    arg->ctx,
	    arg->library_lst);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Add libraries return[%d]", ret);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_import_stk_vsns_5_svr(
	import_vsns_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Import cartridges into stk acsls");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	SET_LIST_TAIL(arg->vsn_list);

	Trace(TR_DEBUG, "Call native lib to list of stk media");
	ret = import_stk_vsns(
	    arg->ctx,
	    arg->eq,
	    arg->options,
	    arg->vsn_list);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Import cartridges return[%d]", ret);

	return (&rpc_result);

}


samrpc_result_t *
samrpc_modify_stkdrive_share_status_5_svr(
	equ_equ_bool_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	Trace(TR_DEBUG, "Modify stk drive share status");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to modify stk drive share status");

	ret = modify_stkdrive_share_status(
	    arg->ctx, arg->eq1, arg->eq2, arg->bool);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);
	return (&rpc_result);

}

samrpc_result_t *
samrpc_get_vsns_6_svr(
	int_uint32_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "get vsns that match a certain set of flags");

	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);
	Trace(TR_DEBUG, "Call native lib to get vsns");
	ret = get_vsns(
	    arg->ctx,
	    arg->i,
	    arg->u_flag,
	    &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "get vsns return[%d]", ret);
	return (&rpc_result);

}
