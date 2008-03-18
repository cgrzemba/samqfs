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

#pragma ident	"$Revision: 1.33 $"

#include "mgmt/sammgmt.h"

/*
 * device_xdr.c
 *
 * This file contains the XDR External Data Representation library functions
 * to represent data structures of device.h in a machine-independent form
 */

/*
 * ******************
 *  basic structures
 * ******************
 */

bool_t
xdr_base_dev_t(
XDR *xdrs,
base_dev_t *objp)
{


	if (!xdr_upath_t(xdrs, objp->name))
		return (FALSE);
	if (!xdr_equ_t(xdrs, &objp->eq))
		return (FALSE);
	if (!xdr_devtype_t(xdrs, objp->equ_type))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->set))
		return (FALSE);
	if (!xdr_equ_t(xdrs, &objp->fseq))
		return (FALSE);
	if (!xdr_dstate_t(xdrs, &objp->state))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->additional_params))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_au_type_t(
XDR *xdrs,
au_type_t *objp)
{


	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_scsi_info_t(
XDR *xdrs,
scsi_info_t *objp)
{

	if (!xdr_vector(xdrs, (char *)objp->vendor, 9,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->prod_id, 17,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->rev_level, 5,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->dev_id, ~0))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_au_t(
XDR *xdrs,
au_t *objp)
{

	if (!xdr_upath_t(xdrs, objp->path))
		return (FALSE);
	if (!xdr_au_type_t(xdrs, &objp->type))
		return (FALSE);
	if (!xdr_dsize_t(xdrs, &objp->capacity))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->fsinfo))
		return (FALSE);
#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {

		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.1") <= 0)) {

			return (TRUE);	/* no more translation required */
		}
	}
#endif /* samrpc_client */
	if (!xdr_string(xdrs, (char **)&objp->raid, ~0))
		return (FALSE);
	XDR_PTR2STRUCT(objp->scsiinfo, scsi_info_t);
	return (TRUE);
}

bool_t
xdr_disk_t(
XDR *xdrs,
disk_t *objp)
{


	if (!xdr_base_dev_t(xdrs, &objp->base_info))
		return (FALSE);
	if (!xdr_au_t(xdrs, &objp->au_info))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->freespace))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_nwlib_type_t(
XDR *xdrs,
nw_lib_type_t *objp)
{

	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_nwlib_req_info_t(
XDR *xdrs,
nwlib_req_info_t *objp)
{
	if (!xdr_uname_t(xdrs, objp->nwlib_name))
		return (FALSE);
	if (!xdr_nwlib_type_t(xdrs, &objp->nw_lib_type))
		return (FALSE);
	if (!xdr_equ_t(xdrs, &objp->eq))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->catalog_loc))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->parameter_file_loc))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_scsi_2_t(
XDR *xdrs,
scsi_2_t *objp)
{
	if (!xdr_int(xdrs, &objp->lun_id))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->target_id))
		return (FALSE);
	return (TRUE);
}


bool_t
xdr_discover_state_t(
XDR *xdrs,
discover_state_t *objp)
{

	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_drive_t(
XDR *xdrs,
drive_t *objp)
{

	if (!xdr_base_dev_t(xdrs, &objp->base_info))
		return (FALSE);
#ifdef SAMRPC_CLIENT
	if ((xdrs->x_public != NULL) &&
	    (strcmp(xdrs->x_public, "1.5.4") <= 0)) {
		if (!xdr_uname_t(xdrs, objp->serial_no))
			return (FALSE);
	} else {
		if (!xdr_vector(xdrs, (char *)&objp->serial_no, 256,
		    sizeof (char), (xdrproc_t)xdr_char))
			return (FALSE);
	}
#endif /* samrpc_client */
#ifdef SAMRPC_SERVER
	if (!xdr_vector(xdrs, (char *)&objp->serial_no, 256,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
#endif /* samrpc_server */

	if (!xdr_uname_t(xdrs, objp->library_name))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)&objp->dev_status, DEV_STATUS_LEN,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->vendor_id))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->product_id))
		return (FALSE);
	if (!xdr_vsn_t(xdrs, objp->loaded_vsn))
		return (FALSE);
	XDR_PTR2LST(objp->alternate_paths_list, string_list);
	if (!xdr_boolean_t(xdrs, &objp->shared))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)&objp->firmware_version, FIRMWARE_LEN,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)&objp->dis_mes, DIS_MES_TYPS,
	    sizeof (dismes_t), (xdrproc_t)xdr_dismes_t))
		return (FALSE);
	if (!xdr_discover_state_t(xdrs, &objp->discover_state))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->scsi_version))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->scsi_path))
		return (FALSE);
	if (!xdr_scsi_2_t(xdrs, &objp->scsi_2_info))
		return (FALSE);
#ifdef SAMRPC_CLIENT
	/*
	 * wwn and id_type are supported in 4.4 and above
	 */
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		/* Client side decoding of results from server */

		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.2") <= 0)) {

			return (TRUE);
		}
	}
#endif /* samrpc_client */

	if (!xdr_uname_t(xdrs, objp->wwn))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->id_type))
		return (FALSE);

#ifdef SAMRPC_CLIENT
	/*
	 * wwn and id_type array are supported in 4.6 and above
	 */
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		/* Client side decoding of results from server */

		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.5.4") <= 0)) {

			return (TRUE);
		}
	}
#endif /* samrpc_client */

	if (!xdr_uname_t(xdrs, objp->lib_serial_no))
		return (FALSE);

	if (!xdr_uname_t(xdrs, objp->wwn_lun))
		return (FALSE);

	if (!xdr_vector(xdrs, (char *)&objp->wwn_id, MAXIMUM_WWN,
	    sizeof (wwn_id_t), (xdrproc_t)xdr_wwn_id_t))
		return (FALSE);

	if (!xdr_vector(xdrs, (char *)&objp->wwn_id_type, MAXIMUM_WWN,
	    sizeof (int), (xdrproc_t)xdr_int))
		return (FALSE);
#ifdef SAMRPC_CLIENT
	/* log_path and log_modtime is supported in 4.6 and above */
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.5.5") <= 0)) {
			return (TRUE);
		}
	}
#endif /* samrpc_client */
	if (!xdr_vector(xdrs, (char *)objp->log_path, MAX_PATH_LENGTH,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	if (!xdr_time_t(xdrs, &objp->log_modtime))
		return (FALSE);
	if (!xdr_time_t(xdrs, &objp->load_idletime))
		return (FALSE);
	if (!xdr_uint64_t(xdrs, &objp->tapealert_flags))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_md_license_t(
XDR *xdrs,
md_license_t *objp)
{

	if (!xdr_mtype_t(xdrs, objp->media_type))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->max_licensed_slots))
		return (FALSE);
#ifdef SAMRPC_CLIENT
	/*
	 * Robot type is only supported in 4.3 and above
	 */
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		/* Client side decoding of results from server */

		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.1") <= 0)) {

			return (TRUE);
		}
	}
#endif /* samrpc_client */
	if (!xdr_ushort(xdrs, &objp->robot_type))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_library_t(
XDR *xdrs,
library_t *objp)
{

	if (!xdr_base_dev_t(xdrs, &objp->base_info))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->serial_no))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->no_of_drives))
		return (FALSE);
	XDR_PTR2LST(objp->drive_list, drive_list);
	XDR_PTR2LST(objp->media_license_list, md_license_list);
	XDR_PTR2LST(objp->alternate_paths_list, string_list);
	if (!xdr_vector(xdrs, (char *)&objp->dev_status, DEV_STATUS_LEN,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)&objp->firmware_version, FIRMWARE_LEN,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->catalog_path))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->vendor_id))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->product_id))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)&objp->dis_mes, DIS_MES_TYPS,
	    sizeof (dismes_t), (xdrproc_t)xdr_dismes_t))
		return (FALSE);
	if (!xdr_discover_state_t(xdrs, &objp->discover_state))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->scsi_version))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->scsi_path))
		return (FALSE);
#ifdef SAMRPC_CLIENT
	/* id_type is supported in 4.4 and above */
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		/* Client side decoding of results from server */

		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.2") <= 0)) {

			return (TRUE);
		}
	}
#endif /* samrpc_client */
	if (!xdr_int(xdrs, &objp->id_type))
		return (FALSE);
#ifdef SAMRPC_CLIENT
	/* stk_param_t and wwn_lun is supported in 4.5 and above */
	/* due to a patch fix in 4.4, 4.4 latest version is 1.3.6 */

	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		/* Client side decoding of results from server */

		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.3.6") <= 0)) {

			return (TRUE);
		}
	}
#endif /* samrpc_client */
	XDR_PTR2STRUCT(objp->storage_tek_parameter, stk_param_t);
	if (!xdr_uname_t(xdrs, objp->wwn_lun))
		return (FALSE);
#ifdef SAMRPC_CLIENT
	/* log_path and log_modtime is supported in 4.6 and above */
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.5.5") <= 0)) {
			return (TRUE);
		}
	}
#endif /* samrpc_client */
	if (!xdr_vector(xdrs, (char *)objp->log_path, MAX_PATH_LENGTH,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	if (!xdr_time_t(xdrs, &objp->log_modtime))
		return (FALSE);

	return (TRUE);
}

bool_t
xdr_import_option_t(
XDR *xdrs,
import_option_t *objp)
{

	if (!xdr_vsn_t(xdrs, objp->vsn))
		return (FALSE);
	if (!xdr_barcode_t(xdrs, objp->barcode))
		return (FALSE);
	if (!xdr_mtype_t(xdrs, objp->mtype))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->audit))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->foreign_tape))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->vol_count))
		return (FALSE);
	if (!xdr_long(xdrs, &objp->pool))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_vsn_sort_key_t(
XDR *xdrs,
vsn_sort_key_t *objp)
{

	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stk_capacity_t(
XDR *xdrs,
stk_capacity_t *objp)
{

	if (!xdr_int(xdrs, &objp->index))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->value))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stk_cap_t(
XDR *xdrs,
stk_cap_t *objp)
{

	if (!xdr_int(xdrs, &objp->acs_num))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->lsm_num))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->cap_num))
		return (FALSE);
	return (TRUE);
}


bool_t
xdr_stk_device_t(
XDR *xdrs,
stk_device_t *objp)
{

	if (!xdr_upath_t(xdrs, objp->pathname))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->acs_num))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->lsm_num))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->panel_num))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->drive_num))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->shared))
		return (FALSE);
	return (TRUE);
}


bool_t
xdr_stk_param_t(
XDR *xdrs,
stk_param_t *objp)
{

	if (!xdr_upath_t(xdrs, objp->param_path))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->access))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->hostname))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->portnum))
		return (FALSE);
	if (!xdr_stk_cap_t(xdrs, &objp->stk_cap))
		return (FALSE);
	XDR_PTR2LST(objp->stk_capacity_list, stk_capacity_list);
	XDR_PTR2LST(objp->stk_device_list, stk_device_list);
	if (!xdr_uname_t(xdrs, objp->ssi_host))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->ssi_inet_portnum))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->csi_hostport))
		return (FALSE);
	return (TRUE);
}


bool_t
xdr_stk_lsm_t(
XDR *xdrs,
stk_lsm_t *objp)
{
	if (!xdr_int(xdrs, &objp->acs_num))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->lsm_num))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->status))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->state))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->free_cells))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stk_cell_t(
XDR *xdrs,
stk_cell_t *objp)
{
	if (!xdr_int(xdrs, &objp->min_row))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->max_row))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->min_column))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->max_column))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stk_pool_t(
XDR *xdrs,
stk_pool_t *objp)
{

	if (!xdr_int(xdrs, &objp->pool_id))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->low_water_mark))
		return (FALSE);
	if (!xdr_long(xdrs, &objp->high_water_mark))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->over_flow))
		return (FALSE);
	return (TRUE);
}


bool_t
xdr_stk_panel_t(
XDR *xdrs,
stk_panel_t *objp)
{

	if (!xdr_int(xdrs, &objp->acs_num))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->lsm_num))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->panel_num))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->type))
		return (FALSE);
	return (TRUE);
}


bool_t
xdr_stk_volume_t(
XDR *xdrs,
stk_volume_t *objp)
{
	if (!xdr_vector(xdrs, (char *)objp->stk_vol, 9,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->acs_num))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->lsm_num))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->panel_num))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->row_id))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->col_id))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->pool_id))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->status, 129,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->media_type, 129,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->volume_type, 129,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	return (TRUE);
}


bool_t
xdr_stk_host_info_t(
XDR *xdrs,
stk_host_info_t *objp)
{

	if (!xdr_uname_t(xdrs, objp->hostname))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->portnum))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->access))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->ssi_host))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->ssi_inet_portnum))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->csi_hostport))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stk_info_type_t(
XDR *xdrs,
stk_info_type_t *objp)
{

	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);
	return (TRUE);
}


bool_t
xdr_stk_phyconf_info_t(
XDR *xdrs,
stk_phyconf_info_t *objp)
{

	XDR_PTR2LST(objp->stk_pool_list, stk_pool_list);
	XDR_PTR2LST(objp->stk_panel_list, stk_panel_list);
	XDR_PTR2LST(objp->stk_lsm_list, stk_lsm_list);
	if (!xdr_stk_cell_t(xdrs, &objp->stk_cell_info))
		return (FALSE);
	return (TRUE);
}

/*
 * *********************
 *  function parameters
 * *********************
 *
 * TI-RPC allows a single parameter to be passed from client to server.
 * If more than one parameter is required, the components can be combined
 * into a structure that is counted as a single element
 * Information passed from server to client is passed as the  function's
 * return value. Information cannot be passed back from server to client
 * through the parameter list
 */

bool_t
xdr_equ_arg_t(
XDR *xdrs,
equ_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_equ_t(xdrs, &objp->eq))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_equ_bool_arg_t(
XDR *xdrs,
equ_bool_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_equ_t(xdrs, &objp->eq))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->bool))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_equ_equ_bool_arg_t(
XDR *xdrs,
equ_equ_bool_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_equ_t(xdrs, &objp->eq1))
		return (FALSE);
	if (!xdr_equ_t(xdrs, &objp->eq2))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->bool))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_au_type_arg_t(
XDR *xdrs,
au_type_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_au_type_t(xdrs, &objp->type))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_discover_media_result_t(
XDR *xdrs,
discover_media_result_t *objp)
{

	XDR_PTR2LST(objp->library_list, library_list);
	XDR_PTR2LST(objp->remaining_drive_list, drive_list);
	return (TRUE);
}

bool_t
xdr_library_arg_t(
XDR *xdrs,
library_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->lib, library_t);
	return (TRUE);
}

bool_t
xdr_library_list_arg_t(
XDR *xdrs,
library_list_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2LST(objp->library_lst, library_list);
	return (TRUE);
}

bool_t
xdr_drive_arg_t(
XDR *xdrs,
drive_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->drive, drive_t);
	return (TRUE);
}

bool_t
xdr_equ_slot_part_bool_arg_t(
XDR *xdrs,
equ_slot_part_bool_arg_t *objp)
{
	XDR_PTR2CTX(objp->ctx);
	if (!xdr_equ_t(xdrs, &objp->eq))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->slot))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->partition))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->bool))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_chmed_arg_t(
XDR *xdrs,
chmed_arg_t *objp)
{
	XDR_PTR2CTX(objp->ctx);
	if (!xdr_equ_t(xdrs, &objp->eq))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->slot))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->set))
		return (FALSE);
	if (!xdr_uint_t(xdrs, &objp->mask))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_import_arg_t(
XDR *xdrs,
import_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_equ_t(xdrs, &objp->eq))
		return (FALSE);
	XDR_PTR2STRUCT(objp->options, import_option_t);
	return (TRUE);
}

bool_t
xdr_import_vsns_arg_t(
XDR *xdrs,
import_vsns_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_equ_t(xdrs, &objp->eq))
		return (FALSE);
	XDR_PTR2STRUCT(objp->options, import_option_t);
	XDR_PTR2LST(objp->vsn_list, string_list);
	return (TRUE);
}

bool_t
xdr_equ_slot_bool_arg_t(
XDR *xdrs,
equ_slot_bool_arg_t *objp)
{
	XDR_PTR2CTX(objp->ctx);
	if (!xdr_equ_t(xdrs, &objp->eq))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->slot))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->bool))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_move_arg_t(
XDR *xdrs,
move_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_equ_t(xdrs, &objp->eq))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->slot))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->dest_slot))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_tplabel_arg_t(
XDR *xdrs,
tplabel_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_equ_t(xdrs, &objp->eq))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->slot))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->partition))
		return (FALSE);
	if (!xdr_vsn_t(xdrs, objp->new_vsn))
		return (FALSE);
	/*
	 * To handle strings, xdr_string is to be used.
	 * vsn can be set as NULL by the client, but the XDR (xdr_string)
	 * will translate that to an empty string during XDR_DECODE
	 *
	 * you can use XDR on an empty string ("") but not on a NULL string
	 * search all vsn is indicated by an empty vsn string
	 */
	if (!xdr_string(xdrs, (char **)&objp->old_vsn, ~0))
		return (FALSE);
	if (!xdr_uint_t(xdrs, &objp->blksize))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->wait))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->erase))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_equ_slot_part_arg_t(
XDR *xdrs,
equ_slot_part_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_equ_t(xdrs, &objp->eq))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->slot))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->partition))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_equ_dstate_arg_t(
XDR *xdrs,
equ_dstate_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_equ_t(xdrs, &objp->eq))
		return (FALSE);
	if (!xdr_dstate_t(xdrs, &objp->state))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_nwlib_req_info_arg_t(
XDR *xdrs,
nwlib_req_info_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->nwlib_req_info, nwlib_req_info_t);
	return (TRUE);
}

bool_t
xdr_string_mtype_arg_t(
XDR *xdrs,
string_mtype_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_string(xdrs, (char **)&objp->str, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->type, ~0))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_equ_sort_arg_t(
XDR *xdrs,
equ_sort_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_equ_t(xdrs, &objp->eq))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->start))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->size))
		return (FALSE);
	if (!xdr_vsn_sort_key_t(xdrs, &objp->sort_key))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->ascending))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_string_sort_arg_t(
XDR *xdrs,
string_sort_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_string(xdrs, (char **)&objp->str, ~0))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->start))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->size))
		return (FALSE);
	if (!xdr_vsn_sort_key_t(xdrs, &objp->sort_key))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->ascending))
		return (FALSE);
	return (TRUE);
}


bool_t
xdr_import_range_arg_t(
XDR *xdrs,
import_range_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_vsn_t(xdrs, objp->begin_vsn))
		return (FALSE);
	if (!xdr_vsn_t(xdrs, objp->end_vsn))
		return (FALSE);
	if (!xdr_equ_t(xdrs, &objp->eq))
		return (FALSE);
	XDR_PTR2STRUCT(objp->options, import_option_t);
	return (TRUE);
}

bool_t
xdr_vsnpool_arg_t(
XDR *xdrs,
vsnpool_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->pool, vsn_pool_t);
	if (!xdr_int(xdrs, &objp->start))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->size))
		return (FALSE);
	if (!xdr_vsn_sort_key_t(xdrs, &objp->sort_key))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->ascending))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_vsnmap_arg_t(
XDR *xdrs,
vsnmap_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->map, vsn_map_t);
	if (!xdr_int(xdrs, &objp->start))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->size))
		return (FALSE);
	if (!xdr_vsn_sort_key_t(xdrs, &objp->sort_key))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->ascending))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stk_display_info_arg_t(
XDR *xdrs,
stk_display_info_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->stk_host_info, stk_host_info_t);
	if (!xdr_stk_info_type_t(xdrs, &objp->type))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->user_time, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->time_type, ~0))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stk_host_list_arg_t(
XDR *xdrs,
stk_host_list_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2LST(objp->stk_host_lst, stk_host_list);
	return (TRUE);
}

bool_t
xdr_stk_host_info_string_arg_t(
XDR *xdrs,
stk_host_info_string_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->stk_host_info, stk_host_info_t);
	if (!xdr_string(xdrs, (char **)&objp->str, ~0))
		return (FALSE);
	return (TRUE);
}
