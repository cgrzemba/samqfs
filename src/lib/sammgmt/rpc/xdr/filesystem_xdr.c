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

#pragma ident	"$Revision: 1.30 $"

#include "mgmt/sammgmt.h"

/*
 * filesystem_xdr.c
 *
 * This file contains the XDR External Data Representation library functions
 * to represent data structures of filesystem.h in a machine-independent form
 */

/*
 * ******************
 *  basic structures
 * ******************
 */

bool_t
xdr_striped_group_t(
XDR *xdrs,
striped_group_t *objp)
{

	if (!xdr_devtype_t(xdrs, objp->name))
		return (FALSE);
	XDR_PTR2LST(objp->disk_list, disk_list);
	return (TRUE);
}

bool_t
xdr_io_mount_options_t(
XDR *xdrs,
io_mount_options_t *objp)
{

	if (!xdr_int(xdrs, &objp->dio_rd_consec))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->dio_rd_form_min))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->dio_rd_ill_min))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->dio_wr_consec))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->dio_wr_form_min))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->dio_wr_ill_min))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->forcedirectio))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->sw_raid))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->flush_behind))
		return (FALSE);
	if (!xdr_int64_t(xdrs, &objp->readahead))
		return (FALSE);
	if (!xdr_int64_t(xdrs, &objp->writebehind))
		return (FALSE);
	if (!xdr_int64_t(xdrs, &objp->wr_throttle))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->forcenfsasync))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->change_flag))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_sam_mount_options_t(
XDR *xdrs,
sam_mount_options_t *objp)
{

	if (!xdr_int16_t(xdrs, &objp->high))
		return (FALSE);
	if (!xdr_int16_t(xdrs, &objp->low))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->partial))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->maxpartial))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->partial_stage))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->stage_n_window))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->stage_retries))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->stage_flush_behind))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->hwm_archive))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->archive))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->arscan))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->oldarchive))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->change_flag))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_sharedfs_mount_options_t(
XDR *xdrs,
sharedfs_mount_options_t *objp)
{

	if (!xdr_boolean_t(xdrs, &objp->shared))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->bg))
		return (FALSE);
	if (!xdr_int16_t(xdrs, &objp->retry))
		return (FALSE);
	if (!xdr_int64_t(xdrs, &objp->minallocsz))
		return (FALSE);
	if (!xdr_int64_t(xdrs, &objp->maxallocsz))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->rdlease))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->wrlease))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->aplease))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->mh_write))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->nstreams))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->meta_timeo))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->lease_timeo))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->soft))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->change_flag))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_multireader_mount_options_t(
XDR *xdrs,
multireader_mount_options_t *objp)
{

	if (!xdr_boolean_t(xdrs, &objp->writer))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->reader))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->invalid))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->refresh_at_eof))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->change_flag))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_qfs_mount_options_t(
XDR *xdrs,
qfs_mount_options_t *objp)
{

	if (!xdr_boolean_t(xdrs, &objp->qwrite))
		return (FALSE);
	if (!xdr_uint16_t(xdrs, &objp->mm_stripe))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->change_flag))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_post_4_2_options_t(
XDR *xdrs,
post_4_2_options_t *objp)
{

	if (!xdr_uint32_t(xdrs, &objp->change_flag))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->def_retention))
		return (FALSE);
#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.3.3") <= 0)) {

			return (TRUE); /* versions 1.3.3 or lower */
		}
	}
#endif /* samrpc_client */
	if (!xdr_boolean_t(xdrs, &objp->abr))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->dmr))
		return (FALSE);
#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.3.4") <= 0)) {

			return (TRUE); /* versions 1.3.4 or lower */
		}
	}
#endif /* samrpc_client */
	/* Sparse file direct i/o zeroing for QFS file system added in 1.3.5 */
	if (!xdr_boolean_t(xdrs, &objp->dio_szero))
		return (FALSE);

#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.4.1") <= 0)) {

			return (TRUE); /* versions 1.4.1 or lower */
		}
	}
#endif /* samrpc_client */
	/* cattr/nocattr support for QFS shared file system added in 1.4.2 */
	if (!xdr_boolean_t(xdrs, &objp->cattr))
		return (FALSE);


	return (TRUE);
}

bool_t
xdr_rel_4_6_options_t(
XDR *xdrs,
rel_4_6_options_t *objp)
{

	if (!xdr_uint32_t(xdrs, &objp->change_flag))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->worm_emul))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->worm_lite))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->emul_lite))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->cdevid))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->clustermgmt))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->clusterfastsw))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->noatime))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->noatime))
		return (FALSE);
	if (!xdr_int16_t(xdrs, &objp->atime))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->min_pool))
		return (FALSE);

	return (TRUE);
}

bool_t
xdr_mount_options_t(
XDR *xdrs,
mount_options_t *objp)
{

	if (!xdr_boolean_t(xdrs, &objp->no_mnttab_entry))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->global))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->overlay))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->readonly))
		return (FALSE);
	if (!xdr_int16_t(xdrs, &objp->sync_meta))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->no_suid))
		return (FALSE);
	if (!xdr_int16_t(xdrs, &objp->stripe))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->trace))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->quota))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->rd_ino_buf_size))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->wr_ino_buf_size))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->worm_capable))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->gfsid))
		return (FALSE);
	if (!xdr_io_mount_options_t(xdrs, &objp->io_opts))
		return (FALSE);
	if (!xdr_sam_mount_options_t(xdrs, &objp->sam_opts))
		return (FALSE);
	if (!xdr_sharedfs_mount_options_t(xdrs, &objp->sharedfs_opts))
		return (FALSE);
	if (!xdr_multireader_mount_options_t(xdrs, &objp->multireader_opts))
		return (FALSE);
	if (!xdr_qfs_mount_options_t(xdrs, &objp->qfs_opts))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->change_flag))
		return (FALSE);
#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.1") <= 0)) {

			return (TRUE); /* versions 1.1 or lower */
		}
	}
#endif /* samrpc_client */
	if (!xdr_post_4_2_options_t(xdrs, &objp->post_4_2_opts))
		return (FALSE);

#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.5.7") <= 0)) {

			return (TRUE); /* versions 1.5.7 or lower */
		}
	}
#endif /* samrpc_client */
	if (!xdr_rel_4_6_options_t(xdrs, &objp->rel_4_6_opts))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_fs_t(
XDR *xdrs,
fs_t *objp)
{

	if (!xdr_uname_t(xdrs, objp->fi_name))
		return (FALSE);
	if (!xdr_short(xdrs, &objp->fs_count))
		return (FALSE);
	if (!xdr_short(xdrs, &objp->mm_count))
		return (FALSE);
	if (!xdr_equ_t(xdrs, &objp->fi_eq))
		return (FALSE);
	if (!xdr_devtype_t(xdrs, objp->equ_type))
		return (FALSE);
	if (!xdr_ushort(xdrs, &objp->dau))
		return (FALSE);
	if (!xdr_time_t(xdrs, &objp->ctime))
		return (FALSE);
	if (!xdr_dstate_t(xdrs, &objp->fi_state))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->fi_version))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->fi_status))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->fi_mnt_point))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->fi_server))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->fi_capacity))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->fi_space))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->fi_archiving))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->fi_shared_fs))
		return (FALSE);
	XDR_PTR2STRUCT(objp->mount_options, mount_options_t);
	XDR_PTR2LST(objp->meta_data_disk_list, disk_list);
	XDR_PTR2LST(objp->data_disk_list, disk_list);
	XDR_PTR2LST(objp->striped_group_list, striped_group_list);
#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.1") <= 0)) {

			return (TRUE); /* versions 1.1 or lower */
		}
	}
#endif /* samrpc_client */
	XDR_PTR2LST(objp->hosts_config, host_info_list);
#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.3.2") <= 0)) {

			return (TRUE); /* versions 1.3.2 or lower */
		}
	}
#endif /* samrpc_client */
	if (!xdr_uname_t(xdrs, objp->nfs))
		return (FALSE);

	return (TRUE);
}

bool_t
xdr_samfsck_info_t(
XDR *xdrs,
samfsck_info_t *objp)
{
	if (!xdr_char(xdrs, &objp->state))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->user))
		return (FALSE);
	if (!xdr_pid_t(xdrs, &objp->pid))
		return (FALSE);
	if (!xdr_pid_t(xdrs, &objp->ppid))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->pri))
		return (FALSE);
	if (!xdr_size_t(xdrs, &objp->size))
		return (FALSE);
	if (!xdr_time_t(xdrs, &objp->stime))
		return (FALSE);
	if (!xdr_time_t(xdrs, &objp->time))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->cmd, 255,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->fsname))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->repair))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_failed_mount_option_t(
XDR *xdrs,
failed_mount_option_t *objp)
{
	if (!xdr_enum(xdrs, (enum_t *)objp))
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
xdr_create_fs_arg_t(
XDR *xdrs,
create_fs_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->fs_info, fs_t);
	if (!xdr_boolean_t(xdrs, &objp->modify_vfstab))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_change_mount_options_arg_t(
XDR *xdrs,
change_mount_options_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_uname_t(xdrs, objp->fsname))
		return (FALSE);
	XDR_PTR2STRUCT(objp->options, mount_options_t);
	return (TRUE);
}

bool_t
xdr_get_default_mount_options_arg_t(
XDR *xdrs,
get_default_mount_options_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_devtype_t(xdrs, objp->fs_type))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->dau_size))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->uses_stripe_groups))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->shared))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->multi_reader))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_fsck_fs_arg_t(
XDR *xdrs,
fsck_fs_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_uname_t(xdrs, objp->fsname))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->logfile))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->repair))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_grow_fs_arg_t(
XDR *xdrs,
grow_fs_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->fs, fs_t);
	XDR_PTR2LST(objp->additional_meta_data_disk, disk_list);
	XDR_PTR2LST(objp->additional_data_disk, disk_list);
	XDR_PTR2LST(objp->additional_striped_group, striped_group_list);
	return (TRUE);
}

bool_t
xdr_create_fs_mount_arg_t(
XDR *xdrs,
create_fs_mount_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->fs_info, fs_t);
	if (!xdr_boolean_t(xdrs, &objp->mount_at_boot))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->create_mnt_point))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->mount))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_create_arch_fs_arg_t(
XDR *xdrs,
create_arch_fs_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->fs_info, fs_t);
	if (!xdr_boolean_t(xdrs, &objp->mount_at_boot))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->create_mnt_point))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->mount))
		return (FALSE);
	XDR_PTR2STRUCT(objp->arch_cfg, fs_arch_cfg_t);
	return (TRUE);
}

bool_t
xdr_fs_arch_cfg_t(
XDR *xdrs,
fs_arch_cfg_t *objp)
{

	if (!xdr_uname_t(xdrs, objp->set_name))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->log_path))
		return (FALSE);
	XDR_PTR2LST(objp->vsn_maps, vsn_map_list);
	XDR_PTR2LST(objp->copies, ar_set_copy_cfg_list);


	return (TRUE);
}

bool_t
xdr_int_list_arg_t(
XDR *xdrs,
int_list_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_int(xdrs, &objp->num))
		return (FALSE);
	XDR_PTR2LST(objp->lst, int_list);
	return (TRUE);
}

bool_t
xdr_intlist_arg_t(
XDR *xdrs,
intlist_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2LST(objp->lst, int_list);
	return (TRUE);
}

bool_t
xdr_reset_eq_arg_t(
XDR *xdrs,
reset_eq_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_string(xdrs, (char **)&objp->str, ~0))
		return (FALSE);
	XDR_PTR2LST(objp->lst, int_list);
	return (TRUE);
}

bool_t
xdr_int_list_result_t(
XDR *xdrs,
int_list_result_t *objp)
{

	XDR_PTR2LST(objp->lst, int_list);
	XDR_PTR2STRUCT(objp->first_free, int);
	return (TRUE);
}

bool_t
xdr_str_cnt_strarray_mntopts_t(
XDR *xdrs,
str_cnt_strarray_mntopts_t *objp)
{
	XDR_PTR2CTX(objp->ctx);
	if (!xdr_string(xdrs, (char **)&objp->str, ~0))
		return (FALSE);
	if (!xdr_u_int(xdrs, &objp->cnt))
		return (FALSE);
	if (!xdr_charstararray(xdrs, (char ***)&objp->array, &objp->cnt, ~0))
		return (FALSE);
	XDR_PTR2STRUCT(objp->mo, mount_options_t);


	return (TRUE);
}

bool_t
xdr_add_storage_node_arg_t(
XDR *xdrs,
add_storage_node_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_string(xdrs, (char **)&objp->fs_name, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->node_name, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->node_ip, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->node_data, ~0))
		return (FALSE);

	XDR_PTR2STRUCT(objp->fs, fs_t);
	return (TRUE);
}
