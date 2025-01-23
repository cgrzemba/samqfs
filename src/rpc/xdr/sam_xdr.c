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
 * Copyright (c) 2010, 2016, Oracle and/or its affiliates. All rights reserved.
 *
 *    SAM-QFS_notice_end
 */


#include <sys/types.h>
#include <sys/time.h>
#include "stat.h"
#include "samrpc.h"

/*
 * This horrible kludge is needed to get a clean compile on Linux platforms.
 * For some reason the Linux library developers decided to change the parameter
 * types to a number of the XDR functions from the "Sun standard" to their
 * own types.  Their types are equivalent but cause compiler warnings.  Most
 * of them can be simply casted away; the following cannot.
 */

#if defined(sun)
typedef u_longlong_t rpc_u_longlong_t;
#elif defined(linux)
typedef u_quad_t rpc_u_longlong_t;
#else
#error Undefined platform
#endif

bool_t
xdr_filecmd(
	register XDR *xdrs,
	filecmd *objp)
{
	if (!xdr_string(xdrs, &objp->filename, MAXPATHLEN))
		return (FALSE);
	if (!xdr_string(xdrs, &objp->options, MAX_OPTS))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_statcmd(
	register XDR *xdrs,
	statcmd *objp)
{
	if (!xdr_string(xdrs, &objp->filename, MAXPATHLEN))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->size))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_sam_st(
	register XDR *xdrs,
	sam_st *objp)
{
	if (!xdr_int(xdrs, &objp->result))
		return (FALSE);
	if (!xdr_samstat_t(xdrs, &objp->s))
		return (FALSE);
	return (TRUE);
}


bool_t
xdr_samcopy(
	register XDR *xdrs,
	samcopy *objp)
{
	if (!xdr_ushort_t(xdrs, &objp->flags))
		return (FALSE);
	if (!xdr_short(xdrs, &objp->n_vsns))
		return (FALSE);
	if (!xdr_u_int(xdrs, (uint_t*)&objp->creation_time))
		return (FALSE);
	if (!xdr_uint64_t(xdrs, &objp->position))
		return (FALSE);
	if (!xdr_u_int(xdrs, &objp->offset))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->media, 4,
	    sizeof (char), (xdrproc_t)xdr_char))
			return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->vsn, MAX_VSN,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	return (TRUE);
}


bool_t
xdr_samstat_t(
	register XDR *xdrs,
	samstat_t *objp)
{
	if (!xdr_u_int(xdrs, (uint_t *)&objp->st_mode))
		return (FALSE);
	if (!xdr_u_int(xdrs, (uint_t *)&objp->st_ino))
		return (FALSE);
	if (!xdr_u_int(xdrs, (uint_t *)&objp->st_dev))
		return (FALSE);
	if (!xdr_u_int(xdrs, (uint_t *)&objp->st_nlink))
		return (FALSE);
	if (!xdr_u_int(xdrs, (uint_t *)&objp->st_uid))
		return (FALSE);
	if (!xdr_u_int(xdrs, (uint_t *)&objp->st_gid))
		return (FALSE);
	if (!xdr_uint64_t(xdrs, &objp->st_size))
		return (FALSE);
	if (!xdr_int(xdrs, (int *)&objp->st_atime))
		return (FALSE);
	if (!xdr_int(xdrs, (int *)&objp->st_mtime))
		return (FALSE);
	if (!xdr_int(xdrs, (int *)&objp->st_ctime))
		return (FALSE);
	if (!xdr_int(xdrs, (int *)&objp->attribute_time))
		return (FALSE);
	if (!xdr_int(xdrs, (int *)&objp->creation_time))
		return (FALSE);
	if (!xdr_int(xdrs, (int *)&objp->residence_time))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->copy, MAX_ARCHIVE,
	    sizeof (samcopy), (xdrproc_t)xdr_samcopy))
		return (FALSE);
	if (!xdr_uint_t(xdrs, &objp->old_attr))
		return (FALSE);
	if (!xdr_uint64_t(xdrs, &objp->attr))
		return (FALSE);
	if (!xdr_u_char(xdrs, &objp->cs_algo))
		return (FALSE);
	if (!xdr_u_char(xdrs, &objp->flags))
		return (FALSE);
	if (!xdr_u_char(xdrs, &objp->stripe_width))
		return (FALSE);
	if (!xdr_u_char(xdrs, &objp->stripe_group))
		return (FALSE);
	if (!xdr_u_int(xdrs, (uint_t *)&objp->gen))
		return (FALSE);
	if (!xdr_u_int(xdrs, (uint_t *)&objp->partial_size))
		return (FALSE);
	if (!xdr_u_int(xdrs, (uint_t *)&objp->rdev))
		return (FALSE);
	if (!xdr_uint64_t(xdrs, &objp->st_blocks))
		return (FALSE);
	if (!xdr_u_int(xdrs, (uint_t *)&objp->segment_size))
		return (FALSE);
	if (!xdr_u_int(xdrs, (uint_t *)&objp->segment_number))
		return (FALSE);
	if (!xdr_uint_t(xdrs, &objp->stage_ahead))
		return (FALSE);
	return (TRUE);
}
