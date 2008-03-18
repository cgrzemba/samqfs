
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

#pragma ident	"$Revision: 1.9 $"

#include "mgmt/sammgmt.h"

/*
 * catalog_xdr.c
 *
 * This file contains the XDR External Data Representation library functions
 * to represent data structures of aml/catalog.h in a machine-independent form
 */

bool_t
xdr_CatalogEntry(
XDR *xdrs,
struct CatalogEntry *objp)
{

	if (!xdr_uint32_t(xdrs, &objp->CeStatus))
		return (FALSE);
	if (!xdr_mtype_t(xdrs, objp->CeMtype))
		return (FALSE);
	if (!xdr_vsn_t(xdrs, objp->CeVsn))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->CeSlot))
		return (FALSE);
	if (!xdr_uint16_t(xdrs, &objp->CePart))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->CeAccess))
		return (FALSE);
	if (!xdr_uint64_t(xdrs, &objp->CeCapacity))
		return (FALSE);
	if (!xdr_uint64_t(xdrs, &objp->CeSpace))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->CeBlockSize))
		return (FALSE);
	if (!xdr_time32_t(xdrs, &objp->CeLabelTime))
		return (FALSE);
	if (!xdr_time32_t(xdrs, &objp->CeModTime))
		return (FALSE);
	if (!xdr_time32_t(xdrs, &objp->CeMountTime))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->CeBarCode, BARCODE_LEN + 1,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	/*
	 * In XDR, we deal with discriminated unions.
	 * In CatalogEntry structure, we make use of an
	 * anonymous union with no discriminant
	 * since the type is only uint64_t, it is okay
	 * to not have to deal with discriminant
	 * but use ||
	 */
	if (!xdr_uint64_t(xdrs, &objp->m.CePtocFwa) ||
	    !xdr_uint64_t(xdrs, &objp->m.CeLastPos))
		return (FALSE);
	if (!xdr_reserve_option_t(xdrs, &objp->r))
		return (FALSE);
	if (!xdr_uint16_t(xdrs, &objp->CeEq))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->CeMid))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->CeVolInfo, VOLINFO_LEN + 1,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	return (TRUE);
}
