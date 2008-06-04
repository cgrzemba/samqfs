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

#pragma ident	"$Revision: 1.34 $"

#include "pub/mgmt/types.h"
#include "pub/devstat.h"
#include "mgmt/sammgmt.h"
#include "pub/mgmt/csn_registration.h"
/*
 * types_xdr.c
 *
 * This file contains the XDR External Data Representation library functions
 * to represent data structures of types.h in a machine-independent form
 */
bool_t
xdr_dismes_t(
XDR *xdrs,
dismes_t objp)
{


	if (!xdr_vector(xdrs, (char *)objp, DIS_MES_LEN + 1,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_wwn_id_t(
XDR *xdrs,
wwn_id_t objp)
{


	if (!xdr_vector(xdrs, (char *)objp, WWN_LENGTH + 1,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_devtype_t(
XDR *xdrs,
devtype_t objp)
{


	if (!xdr_vector(xdrs, (char *)objp, MAX_DEVTYPE_LENGTH,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_samrpc_client_t(
XDR *xdrs,
samrpc_client_t *objp)
{

	if (!xdr_int(xdrs, &objp->timestamp))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_ctx_arg_t(
XDR *xdrs,
ctx_arg_t *objp)
{
	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_ctx_t(
XDR *xdrs,
ctx_t *objp)
{

	if (!xdr_upath_t(xdrs, (char *)objp->dump_path))
		return (FALSE);
	if (!xdr_upath_t(xdrs, (char *)objp->read_location))
		return (FALSE);
	if (!xdr_pointer(xdrs, (char **)&objp->handle,
	    sizeof (samrpc_client_t), (xdrproc_t)xdr_samrpc_client_t))
		return (FALSE);
#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.5.2") <= 0)) {

			return (TRUE); /* versions 1.5.2 or lower */
		}
	}
#endif /* samrpc_client */
	if (!xdr_uname_t(xdrs, objp->user_id))
		return (FALSE);

	return (TRUE);
}

bool_t
xdr_uname_t(
XDR *xdrs,
uname_t objp)
{

	if (!xdr_vector(xdrs, (char *)objp, MAX_NAME_LENGTH,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);

	return (TRUE);
}

bool_t
xdr_upath_t(
XDR *xdrs,
upath_t objp)
{

	if (!xdr_vector(xdrs, (char *)objp, MAX_PATH_LENGTH,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_mtype_t(
XDR *xdrs,
mtype_t objp)
{


	if (!xdr_vector(xdrs, (char *)objp, MAX_MTYPE_LENGTH,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_vsn_t(
XDR *xdrs,
vsn_t objp)
{


	if (!xdr_vector(xdrs, (char *)objp, MAX_VSN_LENGTH,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_sam_id_t(
XDR *xdrs,
sam_id_t *objp)
{


	if (!xdr_sam_ino_t(xdrs, &objp->ino))
		return (FALSE);
	if (!xdr_int32_t(xdrs, &objp->gen))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_host_t(
XDR *xdrs,
host_t objp)
{


	if (!xdr_vector(xdrs, (char *)objp, MAXHOSTNAMELEN,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_umsg_t(
XDR *xdrs,
umsg_t objp)
{


	if (!xdr_vector(xdrs, (char *)objp, OPRMSG_SIZE,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_barcode_t(
XDR *xdrs,
barcode_t objp)
{


	if (!xdr_vector(xdrs, (char *)objp, BARCODE_LEN,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_boolean_t(
XDR *xdrs,
boolean_t *objp)
{


	if (!xdr_bool(xdrs, (bool_t *)objp))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_dstate_t(
XDR *xdrs,
dstate_t *objp)
{


	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_struct_key_t(
XDR *xdrs,
struct_key_t objp)
{


	if (!xdr_vector(xdrs, (char *)objp, 16,
	    sizeof (uchar_t), (xdrproc_t)xdr_u_char))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_proctype_t(
XDR *xdrs,
proctype_t *objp)
{


	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_proc_arg_t(
XDR *xdrs,
proc_arg_t *objp)
{

	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);
	if (!xdr_pid_t(xdrs, &objp->pid))
		return (FALSE);
	if (!xdr_proctype_t(xdrs, &objp->ptype))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_string_uint32_arg_t(
XDR *xdrs,
string_uint32_arg_t *objp)
{

	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->str, ~0))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->u_flag))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_int_uint32_arg_t(
XDR *xdrs,
int_uint32_arg_t *objp)
{

	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->i))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->u_flag))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_string_uint32_uint32_arg_t(
XDR *xdrs,
string_uint32_uint32_arg_t *objp)
{

	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->str, ~0))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->u_1))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->u_2))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_int_string_arg_t(
XDR *xdrs,
int_string_arg_t *objp)
{

	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->i))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->str, ~0))
		return (FALSE);
	return (TRUE);
}


bool_t
xdr_strlst_intlst_intlst_arg_t(
XDR *xdrs,
strlst_intlst_intlst_arg_t *objp)
{

	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);
	if (!xdr_pointer(xdrs, (char **)&objp->intlst1,
	    sizeof (sqm_lst_t), (xdrproc_t)xdr_int_list))
		return (FALSE);
	if (!xdr_pointer(xdrs, (char **)&objp->intlst2,
	    sizeof (sqm_lst_t), (xdrproc_t)xdr_int_list))
		return (FALSE);
	if (!xdr_pointer(xdrs, (char **)&objp->strlst,
	    sizeof (sqm_lst_t), (xdrproc_t)xdr_string_list))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_strlst_uint32_arg_t(
XDR *xdrs,
strlst_uint32_arg_t *objp)
{

	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);
	if (!xdr_pointer(xdrs, (char **)&objp->strlst,
	    sizeof (sqm_lst_t), (xdrproc_t)xdr_string_list))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->u32))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_strlst_bool_arg_t(
XDR *xdrs,
strlst_bool_arg_t *objp)
{

	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);
	if (!xdr_pointer(xdrs, (char **)&objp->strlst,
	    sizeof (sqm_lst_t), (xdrproc_t)xdr_string_list))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->bool))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_file_details_arg_t(
XDR *xdrs,
file_details_arg_t *objp)
{
	XDR_PTR2CTX(objp->ctx);
	if (!xdr_string(xdrs, (char **)&objp->fsname, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->snap, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->dir, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->file, ~0))
		return (FALSE);
	if (!xdr_u_int(xdrs, &objp->u32))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->howmany))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->restrictions, ~0))
		return (FALSE);
	if (!xdr_pointer(xdrs, (char **)&objp->files,
	    sizeof (sqm_lst_t), (xdrproc_t)xdr_string_list))
		return (FALSE);


	return (TRUE);
}

bool_t
xdr_file_details_result_t(
XDR *xdrs,
file_details_result_t *objp)
{
	if (!xdr_uint32_t(xdrs, &objp->more))
		return (FALSE);
	XDR_PTR2LST(objp->list, string_list);

	return (TRUE);
}


bool_t
xdr_strlst_int32_arg_t(XDR *xdrs, strlst_int32_arg_t *objp) {
	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);

	if (!xdr_pointer(xdrs, (char **)&objp->strlst,
		sizeof (sqm_lst_t), (xdrproc_t)xdr_string_list))
		return (FALSE);

	if (!xdr_int32_t(xdrs, &objp->int32))
		return (FALSE);


	return (TRUE);
}

bool_t
xdr_strlst_int32_int32_arg_t(XDR *xdrs, strlst_int32_int32_arg_t *objp) {
	if (!xdr_pointer(xdrs, (char **)&objp->ctx,
	    sizeof (ctx_t), (xdrproc_t)xdr_ctx_t))
		return (FALSE);

	if (!xdr_pointer(xdrs, (char **)&objp->strlst,
	    sizeof (sqm_lst_t), (xdrproc_t)xdr_string_list))
		return (FALSE);

	if (!xdr_int32_t(xdrs, &objp->int1))
		return (FALSE);


	if (!xdr_int32_t(xdrs, &objp->int2))
		return (FALSE);


	return (TRUE);
}

bool_t
xdr_crypt_str_t(XDR *xdrs, crypt_str_t *objp) {

	if (!xdr_int(xdrs, &objp->str_len))
		return (FALSE);

	if (objp->str == NULL) {
		objp->str = (unsigned char *)mallocer(objp->str_len);
		if (objp->str == NULL) {
			return (FALSE);
		}
	}

	if (!xdr_array(xdrs, (char **)&objp->str, (uint_t *)&objp->str_len,
	    objp->str_len, sizeof (uchar_t), (xdrproc_t)xdr_u_char))
		return (FALSE);

	return (TRUE);
}

bool_t
xdr_cns_reg_arg_t(XDR *xdrs, cns_reg_arg_t *objp) {

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_string(xdrs, (char **)&objp->kv, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->hex, ~0))
		return (FALSE);
	XDR_PTR2STRUCT(objp->cl_pwd, crypt_str_t);
	XDR_PTR2STRUCT(objp->proxy_pwd, crypt_str_t);
	return (TRUE);
}

bool_t
xdr_public_key_result_t(XDR *xdrs, public_key_result_t *objp) {

	if (!xdr_string(xdrs, (char **)&objp->pub_key_hex, ~0))
		return (FALSE);

	XDR_PTR2STRUCT(objp->signature, crypt_str_t);

	return (TRUE);
}


bool_t
xdr_string_int_intlist_arg_t(
XDR *xdrs,
string_int_intlist_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_string(xdrs, (char **)&objp->str, ~0))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->num))
		return (FALSE);
	XDR_PTR2LST(objp->int_lst, int_list);
	return (TRUE);
}
