
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

#pragma ident	"$Revision: 1.23 $"

#include "mgmt/sammgmt.h"

/*
 * stage_xdr.c
 *
 * This file contains the XDR External Data Representation library functions
 * to represent data structures of stage.h in a machine-independent form
 */
/*
 * ******************
 *  basic structures
 * ******************
 */
bool_t
xdr_stager_cfg_t(
XDR *xdrs,
stager_cfg_t *objp)
{

	if (!xdr_upath_t(xdrs, objp->stage_log))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->max_active))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->max_retries))
		return (FALSE);
	if (!xdr_pointer(xdrs, (char **)&objp->stage_buf_list,
	    sizeof (sqm_lst_t),
	    (xdrproc_t)xdr_buffer_directive_list))
		return (FALSE);
	if (!xdr_pointer(xdrs, (char **)&objp->stage_drive_list,
	    sizeof (sqm_lst_t),
	    (xdrproc_t)xdr_drive_directive_list))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->change_flag))
		return (FALSE);

#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.4.3") <= 0)) {

			return (TRUE); /* versions 1.4.3 and lower */
		}
	}
#endif /* samrpc_client */
	if (!xdr_uint32_t(xdrs, &objp->options))
		return (FALSE);

#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.6.1") <= 0)) {

			return (TRUE); /* versions 1.6.1 and lower */
		}
	}
#endif /* samrpc_client */
	if (!xdr_pointer(xdrs, (char **)&objp->dk_stream,
	    sizeof (stream_cfg_t),
	    (xdrproc_t)xdr_stream_cfg_t))
		return (FALSE);

	return (TRUE);
}

bool_t
xdr_stream_cfg_t(
XDR *xdrs,
stream_cfg_t *objp) {

	if (!xdr_mtype_t(xdrs, objp->media))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->drives))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->max_size))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->max_count))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->change_flag))
		return (FALSE);

	return (TRUE);
}


bool_t
xdr_StagerStateFlags_t(
XDR *xdrs,
StagerStateFlags_t *objp)
{

	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_StagerStateDetail_t(
XDR *xdrs,
StagerStateDetail_t *objp)
{

	if (!xdr_sam_id_t(xdrs, &objp->id))
		return (FALSE);
	if (!xdr_equ_t(xdrs, &objp->fseq))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->copy))
		return (FALSE);
	if (!xdr_u_longlong_t(xdrs, &objp->position))
		return (FALSE);
	if (!xdr_ulong_t(xdrs, &objp->offset))
		return (FALSE);
	if (!xdr_u_longlong_t(xdrs, &objp->len))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->name))
		return (FALSE);

	return (TRUE);
}

bool_t
xdr_active_stager_info_t(
XDR *xdrs,
active_stager_info_t *objp)
{


	if (!xdr_StagerStateFlags_t(xdrs, &objp->flags))
		return (FALSE);
	if (!xdr_mtype_t(xdrs, objp->media))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->eq))
		return (FALSE);
	if (!xdr_vsn_t(xdrs, objp->vsn))
		return (FALSE);
	if (!xdr_umsg_t(xdrs, objp->msg))
		return (FALSE);
	if (!xdr_StagerStateDetail_t(xdrs, &objp->detail))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stager_stream_t(
XDR *xdrs,
stager_stream_t *objp)
{


	if (!xdr_boolean_t(xdrs, &objp->active))
		return (FALSE);
	if (!xdr_mtype_t(xdrs, objp->media))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->seqnum))
		return (FALSE);
	if (!xdr_vsn_t(xdrs, objp->vsn))
		return (FALSE);
	if (!xdr_size_t(xdrs, &objp->count))
		return (FALSE);
	if (!xdr_time_t(xdrs, &objp->create))
		return (FALSE);
	if (!xdr_time_t(xdrs, &objp->age))
		return (FALSE);
	if (!xdr_umsg_t(xdrs, objp->msg))
		return (FALSE);
	if (!xdr_pointer(xdrs, (char **)&objp->current_file,
	    sizeof (StagerStateDetail_t),
	    (xdrproc_t)xdr_StagerStateDetail_t))
		return (FALSE);
#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.5.6") <= 0)) {

			return (TRUE); /* versions 1.5.6 and lower */
		}
	}
#endif /* samrpc_client */
	if (!xdr_StagerStateFlags_t(xdrs, &objp->state_flags))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stager_info_t(
XDR *xdrs,
stager_info_t *objp)
{

	if (!xdr_upath_t(xdrs, objp->log_file))
		return (FALSE);
	if (!xdr_umsg_t(xdrs, objp->msg))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->streams_dir))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->stage_req))
		return (FALSE);
	if (!xdr_pointer(xdrs, (char **)&objp->active_stager_info,
	    sizeof (sqm_lst_t),
	    (xdrproc_t)xdr_active_stager_info_list))
		return (FALSE);
	if (!xdr_pointer(xdrs, (char **)&objp->stager_streams,
	    sizeof (sqm_lst_t),
	    (xdrproc_t)xdr_stager_stream_list))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_staging_file_info_t(
XDR *xdrs,
staging_file_info_t *objp)
{


	if (!xdr_sam_id_t(xdrs, &objp->id))
		return (FALSE);
	if (!xdr_equ_t(xdrs, &objp->fseq))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->copy))
		return (FALSE);
	if (!xdr_u_longlong_t(xdrs, &objp->len))
		return (FALSE);
	if (!xdr_u_longlong_t(xdrs, &objp->position))
		return (FALSE);
	if (!xdr_u_longlong_t(xdrs, &objp->offset))
		return (FALSE);
	if (!xdr_mtype_t(xdrs, objp->media))
		return (FALSE);
	if (!xdr_vsn_t(xdrs, objp->vsn))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->filename))
		return (FALSE);
	if (!xdr_pid_t(xdrs, &objp->pid))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->user))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_st_sort_key_t(
XDR *xdrs,
st_sort_key_t *objp)
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
xdr_set_stager_cfg_arg_t(
XDR *xdrs,
set_stager_cfg_arg_t *objp)
{


	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);
	if (!xdr_pointer(xdrs, (char **)&objp->stager_config,
	    sizeof (stager_cfg_t), (xdrproc_t)xdr_stager_cfg_t))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_drive_directive_arg_t(
XDR *xdrs,
drive_directive_arg_t *objp)
{


	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);
	if (!xdr_pointer(xdrs, (char **)&objp->stage_drive,
	    sizeof (drive_directive_t), (xdrproc_t)xdr_drive_directive_t))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_buffer_directive_arg_t(
XDR *xdrs,
buffer_directive_arg_t *objp)
{


	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);
	if (!xdr_pointer(xdrs, (char **)&objp->stage_buffer,
	    sizeof (buffer_directive_t),
	    (xdrproc_t)xdr_buffer_directive_t))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stager_stream_arg_t(
XDR *xdrs,
stager_stream_arg_t *objp)
{


	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);
	if (!xdr_pointer(xdrs, (char **)&objp->stream,
	    sizeof (stager_stream_t), (xdrproc_t)xdr_stager_stream_t))
		return (FALSE);
	if (!xdr_st_sort_key_t(xdrs, &objp->sort_key))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->ascending))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stager_stream_range_arg_t(
XDR *xdrs,
stager_stream_range_arg_t *objp)
{


	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);
	if (!xdr_pointer(xdrs, (char **)&objp->stream,
	    sizeof (stager_stream_t), (xdrproc_t)xdr_stager_stream_t))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->start))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->size))
		return (FALSE);
	if (!xdr_st_sort_key_t(xdrs, &objp->sort_key))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->ascending))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stage_arg_t(
XDR *xdrs,
stage_arg_t *objp)
{


	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);
	if (!xdr_pointer(xdrs, (char **)&objp->file_or_dirs,
	    sizeof (sqm_lst_t), (xdrproc_t)xdr_string_list))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->recursive))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_clear_stage_request_arg_t(
XDR *xdrs,
clear_stage_request_arg_t *objp)
{


	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);
	if (!xdr_mtype_t(xdrs, objp->media))
		return (FALSE);
	if (!xdr_vsn_t(xdrs, objp->vsn))
		return (FALSE);
	return (TRUE);
}


bool_t
xdr_range_arg_t(
XDR *xdrs,
range_arg_t *objp)
{


	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->start))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->size))
		return (FALSE);
	return (TRUE);
}


bool_t
xdr_staging_file_arg_t(
XDR *xdrs,
staging_file_arg_t *objp)
{


	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->fname))
		return (FALSE);
	/*
	 * To handle strings, xdr_string is to be used.
	 * vsn can be set as NULL by the client, but the XDR (xdr_string)
	 * will translate that to an empty string during XDR_DECODE
	 *
	 * you can use XDR on an empty string ("") but not on a NULL string
	 * search all vsn is indicated by an empty vsn string
	 */

	if (!xdr_string(xdrs, (char **)&objp->vsn, ~0))
			return (FALSE);
	return (TRUE);
}

bool_t
xdr_stream_arg_t(
XDR *xdrs,
stream_arg_t *objp)
{


	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);
	if (!xdr_pointer(xdrs, (char **)&objp->stream,
	    sizeof (stager_stream_t), (xdrproc_t)xdr_stager_stream_t))
		return (FALSE);
	return (TRUE);
}
