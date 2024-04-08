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

#pragma ident	"$Revision: 1.40 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/license.h"

/*
 * sammgmt_xdr.c
 *
 * External data representation (XDR) is an architecture-independent
 * specification for representing data. It resolves the differences in data
 * byte ordering, data type size, representation, and alignment between
 * different architectures. Applications that use XDR can exchange data across
 * heterogeneous hardware systems.
 *
 * Information passed from server to client is passed as the server function's
 * return value. Since the information cannot be passed back form server to
 * client through parameter list the rpc server program puts the informtion in
 * the samrpc_result_t structure, appropriately filling the  result_struct_t if
 * the function succeeds or an err_struct_t if the function fails
 *
 * The XDR library supports discriminated unions. All server functions return
 * a discriminated union samrpc_result_t  to the client.  The union is used
 * to discriminate between successful and unsuccesful return values.
 *
 * The result_struct_t has an enum and the actual result. The enum value is to
 * pass the result in the correct format to the client function
 *
 *
 * To handle strings, xdr_string is conventionally used
 * (To handle fixed length arrays, I have used xdr_vector)
 *
 * The drawback with XDR handling of strings is that NULL strings cannot be
 * passed.for e.g. in the tp_label api (rp_tplabel_from_eq), the old_vsn can be
 * a null argument according to the api, but when the client sets the old_vsn
 * to null, the XDR (xdr_string) conversion translates the NULL to a null
 * terminated string (\0) empty string dueing XDR_DECODE
 * In other words the null pointer is not preserved accross XDR conversions.
 * Empty (null terminated strings) are converted correctly.
 */

bool_t
xdr_samstruct_type_t(
XDR *xdrs,
samstruct_type_t *objp)
{

	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_result_struct_t(
XDR *xdrs,
result_struct_t *objp)
{

	int adCount;	/* num of AdArchReq in ArchiverdState */

	if ((xdrs->x_op == XDR_FREE) && (objp->result_data == NULL)) {
		return (TRUE);
	}
	if (!xdr_samstruct_type_t(xdrs, &objp->result_type))
		return (FALSE);


	switch (objp->result_type) {
		case SAM_MGMT_STRING:
			if (!xdr_string(xdrs, (char **)&objp->result_data, ~0))
				return (FALSE);
			break;

		case SAM_AR_SET_COPY_CFG:
			XDR_PTR2STRUCT(objp->result_data, ar_set_copy_cfg_t);
			break;

		case SAM_AR_SET_CRITERIA:
			XDR_PTR2STRUCT(objp->result_data, ar_set_criteria_t);
			break;

		case SAM_AR_GLOBAL_DIRECTIVE:
			XDR_PTR2STRUCT(objp->result_data,
			    ar_global_directive_t);
			break;

		case SAM_AR_SET_CRITERIA_LIST:
			XDR_PTR2LST(objp->result_data, ar_set_criteria_list);
			break;

		case SAM_AR_FS_DIRECTIVE:
			XDR_PTR2STRUCT(objp->result_data, ar_fs_directive_t);
			break;

		case SAM_AR_FS_DIRECTIVE_LIST:
			XDR_PTR2LST(objp->result_data, ar_fs_directive_list);
			break;

		case SAM_AR_NAMES_LIST:
			XDR_PTR2LST(objp->result_data, string_list);
			break;

		case SAM_AR_SET_COPY_PARAMS_LIST:
			XDR_PTR2LST(objp->result_data, ar_set_copy_params_list);
			break;

		case SAM_AR_SET_COPY_PARAMS:
			XDR_PTR2STRUCT(objp->result_data, ar_set_copy_params_t);
			break;

		case SAM_AR_VSN_POOL_LIST:
			XDR_PTR2LST(objp->result_data, vsn_pool_list);
			break;

		case SAM_AR_VSN_POOL:
			XDR_PTR2STRUCT(objp->result_data, vsn_pool_t);
			break;

		case SAM_AR_VSN_MAP_LIST:
			XDR_PTR2LST(objp->result_data, vsn_map_list);
			break;

		case SAM_AR_VSN_MAP:
			XDR_PTR2STRUCT(objp->result_data, vsn_map_t);
			break;

		case SAM_AR_DRIVE_DIRECTIVE:
			XDR_PTR2STRUCT(objp->result_data, drive_directive_t);
			break;

		case SAM_AR_BUFFER_DIRECTIVE:
			XDR_PTR2STRUCT(objp->result_data, buffer_directive_t);
			break;

		case SAM_AR_ARCHIVERD_STATE:

			/* handle the dynamic array */
			if (xdrs->x_op == XDR_ENCODE) {
				adCount = ((struct ArchiverdState *)
				    objp->result_data)->AdCount;

			}

			if (!xdr_int(xdrs, &adCount))
				return (FALSE);

			if (!xdr_pointer(xdrs,
			    (char **)&objp->result_data,
			    sizeof (struct ArchiverdState) +
			    (adCount - 1) * 128,
			    (xdrproc_t)xdr_ArchiverdState))
				return (FALSE);
			break;

		case SAM_AR_ARFIND_STATE_LIST:
			XDR_PTR2LST(objp->result_data, ar_find_state_list);
			break;

		case SAM_AR_ARFIND_STATE:
			XDR_PTR2STRUCT(objp->result_data, ar_find_state_t);
			break;

		case SAM_AR_ARCHREQ_LIST:
			XDR_PTR2LST(objp->result_data, archreq_list);
			break;

		case SAM_AR_POOL_USED:
			XDR_PTR2STRUCT(objp->result_data, pool_use_result_t);
			break;

		case SAM_AR_ARCH_SET_LIST:
			XDR_PTR2LST(objp->result_data, arch_set_list);
			break;

		case SAM_AR_ARCH_SET:
			XDR_PTR2STRUCT(objp->result_data, arch_set_t);
			break;

		case SAM_DEV_AU_LIST:
			XDR_PTR2LST(objp->result_data, au_list);
			break;

		case SAM_DEV_DISCOVER_MEDIA_RESULT:
			XDR_PTR2STRUCT(objp->result_data,
			    discover_media_result_t);
			break;

		case SAM_DEV_LIBRARY_LIST:
			XDR_PTR2LST(objp->result_data, library_list);
			break;

		case SAM_DEV_LIBRARY:
			XDR_PTR2STRUCT(objp->result_data, library_t);
			break;

		case SAM_DEV_DRIVE_LIST:
			XDR_PTR2LST(objp->result_data, drive_list);
			break;
		case SAM_DEV_VSNPOOL_PROPERTY:
			XDR_PTR2STRUCT(objp->result_data, vsnpool_property_t);
			break;
		case SAM_DEV_DRIVE:
			XDR_PTR2STRUCT(objp->result_data, drive_t);
			break;
		case SAM_DEV_CATALOG_ENTRY:
			if (!xdr_pointer(xdrs, (char **)&objp->result_data,
			    sizeof (struct CatalogEntry),
			    (xdrproc_t)xdr_CatalogEntry))
				return (FALSE);
			break;
		case SAM_DEV_CATALOG_ENTRY_LIST:
			XDR_PTR2LST(objp->result_data, catalog_entry_list);
			break;
		case SAM_DEV_STRING_LIST:
			XDR_PTR2LST(objp->result_data, string_list);
			break;
		case SAM_DEV_SPACE:
			XDR_PTR2STRUCT(objp->result_data, fsize_t);
			break;
		case SAM_FS_FS_LIST:
			XDR_PTR2LST(objp->result_data, fs_list);
			break;

		case SAM_FS_FS:
			XDR_PTR2STRUCT(objp->result_data, fs_t);
			break;

		case SAM_FS_STRING_LIST:
			XDR_PTR2LST(objp->result_data, string_list);
			break;

		case SAM_FS_MOUNT_OPTIONS:
			XDR_PTR2STRUCT(objp->result_data, mount_options_t);
			break;
		case SAM_FS_FSCK_LIST:
			XDR_PTR2LST(objp->result_data, samfsck_info_list);
			break;

		case SAM_FS_FAILED_MOUNT_OPTS_LIST:
			XDR_PTR2LST(objp->result_data,
			    failed_mount_option_list);
			break;

		case SAM_LIC_STRING_LIST:
			XDR_PTR2LST(objp->result_data, string_list);
			break;

		case SAM_LD_PENDING_LOAD_INFO_LIST:
			XDR_PTR2LST(objp->result_data, pending_load_info_list);
			break;

		case SAM_RC_STRING:
			if (!xdr_string(xdrs, (char **)&objp->result_data, ~0))
				return (FALSE);

			break;

		case SAM_RC_NO_RC_VSNS_LIST:
			XDR_PTR2LST(objp->result_data, no_rc_vsns_list);
			break;

		case SAM_RC_NO_RC_VSNS:
			XDR_PTR2STRUCT(objp->result_data, no_rc_vsns_t);
			break;

		case SAM_RC_ROBOT_CFG_LIST:
			XDR_PTR2LST(objp->result_data, rc_robot_cfg_list);
			break;

		case SAM_RC_ROBOT_CFG:
			XDR_PTR2STRUCT(objp->result_data, rc_robot_cfg_t);
			break;

		case SAM_RC_PARAM:
			XDR_PTR2STRUCT(objp->result_data, rc_param_t);
			break;

		case SAM_RL_FS_DIRECTIVE_LIST:
			XDR_PTR2LST(objp->result_data, rl_fs_directive_list);
			break;

		case SAM_RL_FS_DIRECTIVE:
			XDR_PTR2STRUCT(objp->result_data, rl_fs_directive_t);
			break;

		case SAM_RL_FS_LIST:
			XDR_PTR2LST(objp->result_data, rl_fs_list);
			break;

		case SAM_PTR_SIZE:
			XDR_PTR2STRUCT(objp->result_data, size_t);
			break;

		case SAM_PTR_INT:
			XDR_PTR2STRUCT(objp->result_data, int);
			break;

		case SAM_ST_STAGER_CFG:
			XDR_PTR2STRUCT(objp->result_data, stager_cfg_t);
			break;

		case SAM_ST_DRIVE_DIRECTIVE:
			XDR_PTR2STRUCT(objp->result_data, drive_directive_t);
			break;

		case SAM_ST_BUFFER_DIRECTIVE:
			XDR_PTR2STRUCT(objp->result_data, buffer_directive_t);
			break;

		case SAM_ST_STAGER_INFO:
			XDR_PTR2STRUCT(objp->result_data, stager_info_t);
			break;

		case SAM_ST_STAGING_FILE_INFO_LIST:
			XDR_PTR2LST(objp->result_data, staging_file_info_list);
			break;

		case SAM_ST_STAGING_FILE_INFO:
			XDR_PTR2STRUCT(objp->result_data, staging_file_info_t);
			break;

		case SAM_FAULTS_LIST:
			XDR_PTR2LST(objp->result_data, fault_attr_list);
			break;

		case SAM_FAULTS_SUMMARY:
			XDR_PTR2STRUCT(objp->result_data, fault_summary_t);
			break;

		case SAM_PTR_BOOL:
			XDR_PTR2STRUCT(objp->result_data, boolean_t);
			break;

		case SAM_DISKVOL:
			XDR_PTR2STRUCT(objp->result_data, disk_vol_t);
			break;

		case SAM_DISKVOL_LIST:
			XDR_PTR2LST(objp->result_data, disk_vol_list);
			break;

		case SAM_CLIENT_LIST:
			XDR_PTR2LST(objp->result_data, string_list);
			break;

		/* notify_summary.h */

		case SAM_NOTIFY_SUMMARY_LIST:
			XDR_PTR2LST(objp->result_data, notify_summary_list);
			break;

		case SAM_JOB_HISTORY:
			XDR_PTR2STRUCT(objp->result_data, job_hist_t);
			break;

		case SAM_JOB_HISTORY_LIST:
			XDR_PTR2LST(objp->result_data, job_history_list);
			break;

		case SAM_LIC_INFO:
			XDR_PTR2STRUCT(objp->result_data, license_info_t);
			break;

		case SAM_HOST_INFO_LIST:
			XDR_PTR2LST(objp->result_data, host_info_list);
			break;

		case SAM_STRING_LIST:
			XDR_PTR2LST(objp->result_data, string_list);
			break;

		case SAM_INT_LIST_RESULT:
			XDR_PTR2STRUCT(objp->result_data, int_list_result_t);
			break;

		case SAM_INT_LIST:
			XDR_PTR2LST(objp->result_data, int_list);
			break;

		case SAM_INT_ARRPTRS:
			XDR_PTR2STRUCT(objp->result_data, int_ptrarr_result_t);
			break;

		case SAM_DISK_VSNPOOL_PROPERTY:
			if (!xdr_pointer(xdrs, (char **)&objp->result_data,
			    sizeof (vsnpool_property_t),
			    (xdrproc_t)xdr_diskvsnpool_property_t))
				return (FALSE);
			break;

		case SAM_STK_LSM_LIST:
			XDR_PTR2LST(objp->result_data, stk_lsm_list);
			break;

		case SAM_STK_POOL_LIST:
			XDR_PTR2LST(objp->result_data, stk_pool_list);
			break;

		case SAM_STK_PANEL_LIST:
			XDR_PTR2LST(objp->result_data, stk_panel_list);
			break;

		case SAM_STK_VSN_LIST:
			XDR_PTR2LST(objp->result_data, stk_volume_list);
			break;

		case SAM_STK_CELL_INFO:
			XDR_PTR2STRUCT(objp->result_data, stk_cell_t);
			break;

		case SAM_STK_PHYCONF_INFO:
			XDR_PTR2STRUCT(objp->result_data, stk_phyconf_info_t);
			break;

		case SAM_FILE_DETAILS:
			XDR_PTR2STRUCT(objp->result_data,
			    file_details_result_t);
			break;

		case SAM_PUBLIC_KEY:
			XDR_PTR2STRUCT(objp->result_data, public_key_result_t);
			break;
		default:
			/* do nothing */
			break;
	}

	return (TRUE);
}

bool_t
xdr_err_struct_t(
XDR *xdrs,
err_struct_t *objp)
{

	if (!xdr_int(xdrs, &objp->errcode))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->errmsg, MAX_MSG_LEN,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);

	return (TRUE);
}

bool_t
xdr_samrpc_result_t(
XDR *xdrs,
samrpc_result_t *objp)
{

	if (!xdr_int(xdrs, &objp->status))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->svr_timestamp))
		return (FALSE);

	switch (objp->status) {
		case -1:
			if (!xdr_err_struct_t(xdrs, &objp->samrpc_result_u.err))
				return (FALSE);
			break;

		case -2:
		case -3:
			if (!xdr_err_struct_t(xdrs, &objp->samrpc_result_u.err))
				return (FALSE);
			/* FALLTHRU */
		default:
			if (!xdr_result_struct_t(xdrs,
			    &objp->samrpc_result_u.result))
				return (FALSE);
			break;
	}

	return (TRUE);
}

bool_t
xdr_int_ptrarr_result_t(
XDR *xdrs,
int_ptrarr_result_t *objp)
{

	char **res;
	bool_t more_data;

	switch (xdrs->x_op) {
		case XDR_DECODE: {
			uint32_t count = 0; int k = 0;
			char *node = NULL;
			if (!xdr_uint32_t(xdrs, &objp->count))
				return (FALSE);

			count = objp->count;
			res = (char **)calloc(count + 1, sizeof (char *));

			for (k = 0; k < count; k++) {
				res[k] = NULL;
				node = NULL;

				if (!xdr_bool(xdrs, &more_data))
					return (FALSE);
				if (!more_data) {
					objp->pstr = (char **)res;
					break;
				}
				if (!xdr_string(xdrs, (char **)&node, ~0))
					return (FALSE);
				res[k] = node;
			}
			res[count] = NULL;
			objp->pstr = res;
			break;
		}

		case XDR_ENCODE: {
			res = (char **)objp->pstr;
			char *node = NULL;
			if (res != NULL) {
				node = *res;
			}
			/* first send the array count */
			if (!xdr_uint32_t(xdrs, &objp->count))
				return (FALSE);

			for (;;) {
				more_data = node != NULL;
				if (!xdr_bool(xdrs, &more_data))
					return (FALSE);
				if (!more_data)
					break;
				if (!xdr_string(xdrs, (char **)&node, ~0))
					return (FALSE);
				node = *++res;
			}
			break;
		}
		case XDR_FREE: {
			if (objp != NULL) {
				if (objp->str != NULL) {
					free(objp->str);
				}
				if (objp->pstr != NULL) {
					free(objp->pstr);
				}
				free(objp);
			}
			break;
		}
		default:
			break;
	}
	return (TRUE);

}
