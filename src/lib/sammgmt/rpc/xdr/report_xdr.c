
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

#pragma ident	"$Revision: 1.10 $"

#include "mgmt/sammgmt.h"

/*
 * report_xdr.c
 *
 * This file contains the XDR External Data Representation library functions
 * to represent data structures of job_history.h in a machine-independent form
 *
 */
bool_t
xdr_report_type_t(
XDR *xdrs,
report_type_t *objp)
{


	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);

	return (TRUE);
}

bool_t
xdr_report_requirement_t(
XDR *xdrs,
report_requirement_t *objp)
{
	XDR_PTR2LST(objp->email_names, string_list);
	if (!xdr_uint32_t(xdrs, &objp->section_flag))
			return (FALSE);
	/* report name is auto-generated */
	/* acsls ports and hostnames are auto-discovered */
	/* a system can have multiple acsls library */

	if (!xdr_report_type_t(xdrs, &objp->report_type))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_report_requirement_arg_t(
XDR *xdrs,
report_requirement_arg_t *objp)
{
	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->req, report_requirement_t);
	return (TRUE);
}

bool_t
xdr_file_metric_rpt_arg_t(
XDR* xdrs,
file_metric_rpt_arg_t *objp)
{
	XDR_PTR2CTX(objp->ctx);
	if (!xdr_string(xdrs, (char **)&objp->str, ~0))
		return (FALSE);
	if (!xdr_enum(xdrs, (enum_t *)&objp->which))
		return (FALSE);
	if (!xdr_time_t(xdrs, &objp->start))
		return (FALSE);
	if (!xdr_time_t(xdrs, &objp->end))
		return (FALSE);
	return (TRUE);
}
