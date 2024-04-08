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

#pragma ident	"$Revision: 1.38 $"

#include "mgmt/sammgmt.h"

/*
 * archive_xdr.c
 *
 * This file contains the XDR External Data Representation library functions
 * to represent data structures of archive.h in a machine-independent form
 */
/*
 * ******************
 *  basic structures
 * ******************
 */

bool_t
xdr_join_method_t(
XDR *xdrs,
join_method_t *objp)
{

	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_regexp_type_t(
XDR *xdrs,
regexp_type_t *objp)
{

	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_offline_copy_method_t(
XDR *xdrs,
offline_copy_method_t *objp)
{

	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_sort_method_t(
XDR *xdrs,
sort_method_t *objp)
{

	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_priority_t(
XDR *xdrs,
priority_t *objp)
{

	if (!xdr_uname_t(xdrs, objp->priority_name))
		return (FALSE);
	if (!xdr_float(xdrs, &objp->value))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->change_flag))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_ar_general_copy_cfg_t(
XDR *xdrs,
ar_general_copy_cfg_t *objp)
{

	if (!xdr_int(xdrs, &objp->copy_seq))
		return (FALSE);
	if (!xdr_uint_t(xdrs, &objp->ar_age))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_ar_set_copy_cfg_t(
XDR *xdrs,
ar_set_copy_cfg_t *objp)
{

	if (!xdr_ar_general_copy_cfg_t(xdrs, &objp->ar_copy))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->release))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->norelease))
		return (FALSE);
	if (!xdr_uint_t(xdrs, &objp->un_ar_age))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->change_flag))
		return (FALSE);
	return (TRUE);
}

typedef ar_set_copy_cfg_t *ar_set_copy_cfg_ptr_t;

bool_t
xdr_ar_set_copy_cfg_ptr_t(
XDR *xdrs,
ar_set_copy_cfg_ptr_t *objp)
{

	if (!xdr_pointer(xdrs, (char **)objp,
	    sizeof (ar_set_copy_cfg_t),
	    (xdrproc_t)xdr_ar_set_copy_cfg_t))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_ar_set_criteria_t(
XDR *xdrs,
ar_set_criteria_t *objp)
{

	if (!xdr_uname_t(xdrs, objp->fs_name))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->set_name))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->path))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->minsize))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->maxsize))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->name))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->user))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->group))
		return (FALSE);
	if (!xdr_char(xdrs, &objp->release))
		return (FALSE);
	if (!xdr_char(xdrs, &objp->stage))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->access))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->num_copies))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->arch_copy, MAX_COPY,
	    sizeof (ar_set_copy_cfg_ptr_t),
	    (xdrproc_t)xdr_ar_set_copy_cfg_ptr_t))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->change_flag))
		return (FALSE);
	if (!xdr_struct_key_t(xdrs, objp->key))
		return (FALSE);
#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.3.3") <= 0)) {
			return (TRUE);	/* no more translation required */
		}
	}
#endif /* samrpc_client */
	if (!xdr_boolean_t(xdrs, &objp->nftv))
		return (FALSE);

#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.4.2") <= 0)) {

			return (TRUE); /* versions 1.4.2 or lower */
		}
	}
#endif /* samrpc_client */
	if (!xdr_uname_t(xdrs, objp->after))
		return (FALSE);

#ifdef SAMRPC_CLIENT

	/*
	 * This client block handles servers that have different
	 * data in the struct than what is present from 4.6 patch 04 and
	 * more recent.
	 */
	if ((xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) &&
	    (xdrs->x_public != NULL)) {

		/*
		 * First Case: Server does not have the attr flags and
		 * partial size nor the data class information. This
		 * is for 4.5 servers that have not been patched to
		 * support attr_flags and partial_size.
		 */
		if ((strcmp(xdrs->x_public, "1.4.4") <= 0)) {
			return (TRUE); /* versions 1.4.4 or lower */
		}

		/*
		 * Second Case: Server has the attr flags and partial
		 * size but not the data class information. This is for 4.5
		 * servers that have had patch 07 or greater applied.
		 */
		if ((strcmp(xdrs->x_public, "1.4.5") >= 0) &&
		    (strcmp(xdrs->x_public, "1.5.0") < 0)) {
			if (!xdr_int32_t(xdrs, &objp->attr_flags))
				return (FALSE);
			if (!xdr_int32_t(xdrs, &objp->partial_size))
				return (FALSE);
			return (TRUE);
		}
		/*
		 * Third Case: This is for 4.6 servers running code from
		 * prior to the -04 patch that have data class fields
		 * but do not have attr_flags nor partial_size. It also
		 * is triggered by 5.0 development builds which will not
		 * be released.
		 */
		if (((strcmp(xdrs->x_public, "1.5.0") >= 0) &&
		    (strcmp(xdrs->x_public, "1.5.9") <= 0)) ||
		    ((strcmp(xdrs->x_public, "1.6.0") >= 0) &&
		    (strcmp(xdrs->x_public, "1.6.2") < 0))) {
			return (FALSE);
		}
	}
#endif /* samrpc_client */


	if (!xdr_int32_t(xdrs, &objp->attr_flags))
		return (FALSE);

	if (!xdr_int32_t(xdrs, &objp->partial_size))
		return (FALSE);

	return (TRUE);
}

bool_t
xdr_ar_global_directive_t(
XDR *xdrs,
ar_global_directive_t *objp)
{

	if (!xdr_boolean_t(xdrs, &objp->wait))
		return (FALSE);
	if (!xdr_uint_t(xdrs, &objp->ar_interval))
		return (FALSE);
	if (!xdr_ExamMethod_t(xdrs, &objp->scan_method))
		return (FALSE);
	XDR_PTR2LST(objp->ar_bufs, buffer_directive_list);
	XDR_PTR2LST(objp->ar_max, buffer_directive_list);
	if (!xdr_boolean_t(xdrs, &objp->archivemeta))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->notify_script))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->log_path))
		return (FALSE);
	XDR_PTR2LST(objp->ar_drives, drive_directive_list);
	XDR_PTR2LST(objp->ar_set_lst, ar_set_criteria_list);
	XDR_PTR2LST(objp->ar_overflow_lst, buffer_directive_list);
	if (!xdr_uint32_t(xdrs, &objp->change_flag))
		return (FALSE);

#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.3.6") < 0)) {
			return (TRUE);	/* no more translation required */
		}
	}
#endif /* samrpc_client */

	/*
	 * The options field was added in relase 4.4 patch 02 to
	 * provide support for the scanlist_squash feature
	 */
	if (!xdr_int32_t(xdrs, &objp->options))
		return (FALSE);

#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.5.8") < 0)) {
			return (TRUE);	/* no more translation required */
		}
	}
#endif /* samrpc_client */
	if (!xdr_pointer(xdrs, (char **)&objp->timeouts,
	    sizeof (sqm_lst_t), (xdrproc_t)xdr_string_list)) {
		return (FALSE);
	}

#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.6.1") <= 0)) {
			return (TRUE);	/* no more translation required */
		}
	}
#endif /* samrpc_client */

	if (!xdr_uint_t(xdrs, &objp->bg_interval)) {
		return (FALSE);
	}

	if (!xdr_int(xdrs, &objp->bg_time)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_ar_fs_directive_t(
XDR *xdrs,
ar_fs_directive_t *objp)
{

	if (!xdr_uname_t(xdrs, objp->fs_name))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->log_path))
		return (FALSE);
	if (!xdr_uint_t(xdrs, &objp->fs_interval))
		return (FALSE);
	XDR_PTR2LST(objp->ar_set_criteria, ar_set_criteria_list);
	if (!xdr_boolean_t(xdrs, &objp->wait))
		return (FALSE);
	if (!xdr_ExamMethod_t(xdrs, &objp->scan_method))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->archivemeta))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->num_copies))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->fs_copy, MAX_COPY,
	    sizeof (ar_set_copy_cfg_ptr_t),
	    (xdrproc_t)xdr_ar_set_copy_cfg_ptr_t))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->change_flag))
		return (FALSE);

#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.3.6") < 0)) {
			return (TRUE);	/* no more translation required */
		}
	}
#endif /* samrpc_client */

	/*
	 * The options field was added in relase 4.4 patch 02 to
	 * provide support for the scanlist_squash feature
	 */
	if (!xdr_int32_t(xdrs, &objp->options))
		return (FALSE);



#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.6.1") <= 0)) {
			return (TRUE);	/* no more translation required */
		}
	}
#endif /* samrpc_client */

	if (!xdr_uint_t(xdrs, &objp->bg_interval)) {
		return (FALSE);
	}

	if (!xdr_int(xdrs, &objp->bg_time)) {
		return (FALSE);
	}

	return (TRUE);
}

bool_t
xdr_ar_set_copy_params_t(
XDR *xdrs,
ar_set_copy_params_t *objp)
{

	if (!xdr_uname_t(xdrs, objp->ar_set_copy_name))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->archmax))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->bufsize))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->buflock))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->drives))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->drivemax))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->drivemin))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->ovflmin))
		return (FALSE);
	if (!xdr_vsn_t(xdrs, objp->disk_volume))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->fillvsns))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->tapenonstop))
		return (FALSE);
	if (!xdr_short(xdrs, &objp->reserve))
		return (FALSE);
	XDR_PTR2LST(objp->priority_lst, priority_list);
	if (!xdr_boolean_t(xdrs, &objp->unarchage))
		return (FALSE);
	if (!xdr_join_method_t(xdrs, &objp->join))
		return (FALSE);
	if (!xdr_sort_method_t(xdrs, &objp->rsort))
		return (FALSE);
	if (!xdr_sort_method_t(xdrs, &objp->sort))
		return (FALSE);
	if (!xdr_offline_copy_method_t(xdrs, &objp->offline_copy))
		return (FALSE);
	if (!xdr_uint_t(xdrs, &objp->startage))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->startcount))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->startsize))
		return (FALSE);
	if (!xdr_rc_param_t(xdrs, &objp->recycle))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->simdelay))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->tstovfl))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->change_flag))
		return (FALSE);
	/* Backward compatibility to be done only at the client */
#ifdef SAMRPC_CLIENT
	/*
	 * directio is only used in 4.2, i.e. API_VERSION 1.1
	 *
	 * If the operation is ENCODE (input args) or
	 * decoding of results, version check required
	 *
	 * Perform xdr filtering only if server sent the structure with
	 * the directio field, to do this, check the API_VERSION from the
	 * server, if it is lower than client, then do not do any XDR
	 * translation because the field does not exist
	 *
	 * We do not support older clients and newer servers
	 * We only support newer clients and older servers
	 *
	 */
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {

		/*
		 * Client side decoding of results from server
		 * or encoding input args to server
		 */

		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.1") < 0)) {

			/*
			 * Check if server version and client version
			 * are same, if not do not decode/encode
			 * this might be a new field only existing
			 * on newer servers, not on older servers
			 *
			 * XDR translation ends here...
			 *
			 */
			return (TRUE);	/* no more translation required */
		}
	}

	/*
	 * server side decoding input arguments if SAM-FS/QFS ver >= 4.3
	 * client side encoding of input arguments
	 * server side encoding of results if SAM-FS/QFS ver >= 4.3
	 */
#endif /* samrpc_client */
	if (!xdr_boolean_t(xdrs, &objp->directio))
		return (FALSE);

#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.4.2") <= 0)) {

			return (TRUE); /* versions 1.4.2 or lower */
		}
	}
#endif /* samrpc_client */
	if (!xdr_short(xdrs, &objp->rearch_stage_copy))
		return (FALSE);

#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.5.8") <= 0)) {

			return (TRUE); /* versions 1.5.8 or lower */
		}
	}
#endif /* samrpc_client */

	if (!xdr_uint_t(xdrs, &objp->queue_time_limit))
		return (FALSE);

#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.6.1") <= 0)) {

			return (TRUE); /* versions 1.6.1 or lower */
		}
	}
#endif /* samrpc_client */

	if (!xdr_fsize_t(xdrs, &objp->fillvsns_min))
		return (FALSE);


	return (TRUE);
}

bool_t
xdr_vsn_pool_t(
XDR *xdrs,
vsn_pool_t *objp)
{

	if (!xdr_uname_t(xdrs, objp->pool_name))
		return (FALSE);
	if (!xdr_mtype_t(xdrs, objp->media_type))
		return (FALSE);
	XDR_PTR2LST(objp->vsn_names, string_list);
	return (TRUE);
}

bool_t
xdr_vsn_map_t(
XDR *xdrs,
vsn_map_t *objp)
{

	if (!xdr_uname_t(xdrs, objp->ar_set_copy_name))
		return (FALSE);
	if (!xdr_mtype_t(xdrs, objp->media_type))
		return (FALSE);
	XDR_PTR2LST(objp->vsn_names, string_list);
	XDR_PTR2LST(objp->vsn_pool_names, string_list);
	return (TRUE);
}

bool_t
xdr_ar_find_state_t(
XDR *xdrs,
ar_find_state_t *objp)
{

	if (!xdr_uname_t(xdrs, objp->fs_name))
		return (FALSE);
	if (!xdr_ArfindState(xdrs, &objp->state))
		return (FALSE);
	return (TRUE);
}

/*
 * These are all elements of ArfindState
 * MappedFile_t is defined in sam/types.h
 */
bool_t
xdr_MappedFile_t(
XDR *xdrs,
MappedFile_t *objp)
{

	if (!xdr_uint32_t(xdrs, &objp->MfMagic))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->MfLen))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->MfValid))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_ExamMethod_t(
XDR *xdrs,
ExamMethod_t *objp)
{

	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stats(
XDR *xdrs,
struct stats *objp)
{

	if (!xdr_int(xdrs, &objp->numof))
		return (FALSE);
	if (!xdr_offset_t(xdrs, &objp->size))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_FsStats(
XDR *xdrs,
struct FsStats *objp)
{

	if (!xdr_stats(xdrs, &objp->total))
		return (FALSE);
	if (!xdr_stats(xdrs, &objp->regular))
		return (FALSE);
	if (!xdr_stats(xdrs, &objp->offline))
		return (FALSE);
	if (!xdr_stats(xdrs, &objp->archdone))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->copies, MAX_ARCHIVE,
	    sizeof (struct stats), (xdrproc_t)xdr_stats))
		return (FALSE);
	if (!xdr_stats(xdrs, &objp->dirs))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_ArfindState(
XDR *xdrs,
struct ArfindState *objp)
{

	if (!xdr_MappedFile_t(xdrs, &objp->Af))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->AfVersion))
		return (FALSE);
	if (!xdr_ExamMethod_t(xdrs, &objp->AfExamine))
		return (FALSE);
	if (!xdr_ExecState_t(xdrs, &objp->AfExec))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->AfInterval))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->AfLogFile))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->AfNormalExit))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->AfOprMsg, OPRMSG_SIZE,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	if (!xdr_pid_t(xdrs, &objp->AfPid))
		return (FALSE);
	if (!xdr_sam_time_t(xdrs, &objp->AfSbInit))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->AfSeqNum))
		return (FALSE);
	if (!xdr_FsStats(xdrs, &objp->AfStats))
		return (FALSE);
	if (!xdr_FsStats(xdrs, &objp->AfStatsScan))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_ArchiverdState(
XDR *xdrs,
struct ArchiverdState *objp)
{

	if (!xdr_MappedFile_t(xdrs, &objp->Ad))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->AdVersion))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->AdCount))
		return (FALSE);
	if (!xdr_ExecState_t(xdrs, &objp->AdExec))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->AdInterval))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->AdNotifyFile))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->AdOprMsg, OPRMSG_SIZE,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->AdOprMsg2, OPRMSG_SIZE,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	if (!xdr_pid_t(xdrs, &objp->AdPid))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->AdArchReq, objp->AdCount,
	    sizeof (upath_t), (xdrproc_t)xdr_upath_t))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_set_name_t(
XDR *xdrs,
set_name_t objp)
{

	if (!xdr_vector(xdrs, (char *)objp, 33,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_OwnerName_t(
XDR *xdrs,
OwnerName_t objp)
{

	if (!xdr_vector(xdrs, (char *)objp, 49,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_ArState(
XDR *xdrs,
enum ArState *objp)
{

	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);
	return (TRUE);
}

/* Archiving data. duplicated from ArchReq */
struct ArCopyInstance {
	fsize_t CiBytesWritten; /* Number of bytes written to archive */
	fsize_t CiMinSpace;	/* Space required to archive smallest file */
	fsize_t CiSpace;	/* Space required to archive all files */
	pid_t   CiPid;		/* sam-arcopy process id */
	int	CiArchives;	/* Archive files (tarballs) written */
	int	CiArcopyNum;	/* arcopy number */
	int	CiFiles;	/* Number of files */
	int	CiFilesWritten;	/* Number of files written to archive */
	ushort_t CiFlags;
	OwnerName_t CiOwner;	/* Reserve VSN owner name */
	char    CiOprmsg[OPRMSG_SIZE];

	int	CiAln;		/* Archive Library number */
	int	CiLibDev;	/* Library device entry offset */
	int	CiRmFn;		/* Removable media file number */
	int	CiSlot;		/* Slot number in library */
	mtype_t CiMtype;	/* Media type of volume being written */
	vsn_t	CiVsn;		/* VSN of volume being written */
	fsize_t CiVolSpace;	/* Space available on volume */
};

bool_t
xdr_ArchReq(
XDR *xdrs,
struct ArchReq *objp)
{

	if (!xdr_MappedFile_t(xdrs, &objp->Ar))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->ArVersion))
		return (FALSE);
	if (!xdr_ArState(xdrs, &objp->ArState))
		return (FALSE);
	if (!xdr_set_name_t(xdrs, objp->ArAsname))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->ArFsname))
		return (FALSE);
	if (!xdr_ushort_t(xdrs, &objp->ArStatus))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->ArCount))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->ArDrives))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->ArDrivesUsed))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->ArFileIndex))
		return (FALSE);
	if (!xdr_uint_t(xdrs, &objp->ArSeqnum))
		return (FALSE);
	if (!xdr_size_t(xdrs, &objp->ArSize))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->ArSpace))
		return (FALSE);
	if (!xdr_time_t(xdrs, &objp->ArTime))
		return (FALSE);
	if (!xdr_ushort_t(xdrs, &objp->ArDivides))
		return (FALSE);
	if (!xdr_ushort_t(xdrs, &objp->ArFlags))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->ArFiles))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->ArMinSpace))
		return (FALSE);
	if (!xdr_uint_t(xdrs, &objp->ArMsgSent))
		return (FALSE);
	if (!xdr_float(xdrs, &objp->ArPriority))
		return (FALSE);
	if (!xdr_float(xdrs, &objp->ArSchedPriority))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->ArSelFiles))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->ArSelSpace))
		return (FALSE);
	/* dynamic array */
	if (!xdr_vector(xdrs, (char *)objp->ArCpi, objp->ArDrives,
	    sizeof (struct ArCopyInstance), (xdrproc_t)xdr_ArCopyInstance_t))
		return (FALSE);
	return (TRUE);

}

bool_t
xdr_ArCopyInstance_t(
XDR *xdrs,
struct ArCopyInstance *objp)
{

	if (!xdr_fsize_t(xdrs, &objp->CiBytesWritten))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->CiMinSpace))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->CiSpace))
		return (FALSE);
	if (!xdr_pid_t(xdrs, &objp->CiPid))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->CiArchives))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->CiArcopyNum))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->CiFiles))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->CiFilesWritten))
		return (FALSE);
	if (!xdr_ushort_t(xdrs, &objp->CiFlags))
		return (FALSE);
	if (!xdr_OwnerName_t(xdrs, objp->CiOwner))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->CiOprmsg, OPRMSG_SIZE,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->CiAln))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->CiLibDev))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->CiRmFn))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->CiSlot))
		return (FALSE);
	if (!xdr_mtype_t(xdrs, objp->CiMtype))
		return (FALSE);
	if (!xdr_vsn_t(xdrs, objp->CiVsn))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->CiVolSpace))
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
xdr_ar_global_directive_arg_t(
XDR *xdrs,
ar_global_directive_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->ar_global, ar_global_directive_t);
	return (TRUE);
}

bool_t
xdr_string_arg_t(
XDR *xdrs,
string_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_string(xdrs, (char **)&objp->str, ~0))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_string_bool_arg_t(
XDR *xdrs,
string_bool_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_string(xdrs, (char **)&objp->str, ~0))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->bool))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_ar_fs_directive_arg_t(
XDR *xdrs,
ar_fs_directive_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->fs_directive, ar_fs_directive_t);
	return (TRUE);
}

bool_t
xdr_ar_set_criteria_arg_t(
XDR *xdrs,
ar_set_criteria_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->crit, ar_set_criteria_t);
	return (TRUE);
}

bool_t
xdr_ar_set_copy_params_arg_t(
XDR *xdrs,
ar_set_copy_params_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->copy_params, ar_set_copy_params_t);
	return (TRUE);
}

bool_t
xdr_vsn_pool_arg_t(
XDR *xdrs,
vsn_pool_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->pool, vsn_pool_t);
	return (TRUE);
}

bool_t
xdr_vsn_map_arg_t(
XDR *xdrs,
vsn_map_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->map, vsn_map_t);
	return (TRUE);
}

bool_t
xdr_string_list_arg_t(
XDR *xdrs,
string_list_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2LST(objp->strings, string_list);
	return (TRUE);
}

bool_t
xdr_ExecState_t(
XDR *xdrs,
ExecState_t *objp)
{

	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_buffer_directive_t(
XDR *xdrs,
buffer_directive_t *objp)
{

	if (!xdr_mtype_t(xdrs, objp->media_type))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->size))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->lock))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->change_flag))
	if (!xdr_uint32_t(xdrs, &objp->change_flag))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_drive_directive_t(
XDR *xdrs,
drive_directive_t *objp)
{

	if (!xdr_uname_t(xdrs, objp->auto_lib))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->count))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->change_flag))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_pool_use_result_t(
XDR *xdrs,
pool_use_result_t *objp)
{

	if (!xdr_boolean_t(xdrs, &objp->in_use))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->used_by))
		return (FALSE);
	return (TRUE);
}
