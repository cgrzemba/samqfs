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
#pragma ident	"$Revision: 1.8 $"

#include <sys/types.h>
#include <rpc/xdr.h>
#include <stdlib.h>
#include "mgmt/file_details.h"
#include "pub/mgmt/sqm_list.h"

static bool_t
xdr_fildet_seg_t(XDR *xdrs, fildet_seg_t *objp);

static bool_t
xdr_fildet_copy_t(XDR *xdrs, fildet_copy_t *objp);

bool_t
xdr_filedetails_t(XDR *xdrs, filedetails_t *objp)
{
	if (!xdr_wrapstring(xdrs, &objp->file_name))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->attrmod))
		return (FALSE);
	if (!xdr_int32_t(xdrs, (int32_t *)&objp->user))
		return (FALSE);
	if (!xdr_int32_t(xdrs, (int32_t *)&objp->group))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, (uint32_t *)&objp->prot))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->segCount))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->segStageAhead))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->partialSzKB))
		return (FALSE);
	if (!xdr_uint16_t(xdrs, &objp->samFileAtts))
		return (FALSE);
	if (!xdr_uint8_t(xdrs, &objp->file_type))
		return (FALSE);
	if (!xdr_int64_t(xdrs, &objp->snapOffset))
		return (FALSE);
	if (!xdr_fildet_seg_t(xdrs, &objp->summary))
		return (FALSE);
	if (!xdr_array(xdrs, (char **)&objp->segments, &objp->segCount,
	    ~0, sizeof (fildet_seg_t), xdr_fildet_seg_t))
		return (FALSE);

	return (TRUE);
}

static bool_t
xdr_fildet_seg_t(XDR *xdrs, fildet_seg_t *objp)
{
	if (!xdr_uint32_t(xdrs, &objp->created))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->modified))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->accessed))
		return (FALSE);
	if (!xdr_uint64_t(xdrs, &objp->size))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->flags))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->segnum))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)&(objp->copy), 4,
	    sizeof (fildet_copy_t), xdr_fildet_copy_t))
		return (FALSE);

	return (TRUE);
}

static bool_t
xdr_fildet_copy_t(XDR *xdrs, fildet_copy_t *objp)
{

	if (!xdr_vector(xdrs, (char *)&objp->mediaType,
	    sizeof (objp->mediaType), sizeof (char), xdr_char))
		return (FALSE);
	if (!xdr_long(xdrs, &objp->created))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->flags))
		return (FALSE);
	if (!xdr_wrapstring(xdrs, &objp->vsns))
		return (FALSE);

	return (TRUE);
}

bool_t
xdr_filedetails_list(XDR *xdrs, sqm_lst_t *objp)
{
	bool_t		more_data;
	node_t		*next = NULL;
	node_t		*prev = NULL;
	node_t		*node;

	if (objp == NULL) {
		if (xdrs->x_op == XDR_FREE) {
			/* nothing to do */
			return (TRUE);
		} else {
			return (FALSE);
		}
	}

	node = objp->head;

	switch (xdrs->x_op) {
		case XDR_FREE:
			while (node != NULL) {
				next = node->next;

				if (!xdr_pointer(xdrs, (char **)&node->data,
				    sizeof (filedetails_t),
				    xdr_filedetails_t)) {
					return (FALSE);
				}

				free(node);

				node = next;
			}
			/* do I free the list object here? */
			break;
		case XDR_ENCODE:
			more_data = TRUE;
			while (node != NULL) {
				if (!xdr_bool(xdrs, &more_data)) {
					/* won't this leak memory?? */
					return (FALSE);
				}

				if (!xdr_pointer(xdrs, (char **)&node->data,
				    sizeof (filedetails_t),
				    xdr_filedetails_t)) {
					return (FALSE);
				}

				node = node->next;
			}

			/* no more data */
			more_data = FALSE;
			if (!xdr_bool(xdrs, &more_data)) {
				return (FALSE);
			}

			break;
		case XDR_DECODE:
			if (!xdr_bool(xdrs, &more_data))
				return (FALSE);

			while (more_data == TRUE) {
				prev = node;

				node = calloc(1, sizeof (node_t));
				if (node == NULL) {
					return (FALSE);
				}

				node->next = NULL;
				if (prev) {
					prev->next = node;
				}

				if (!xdr_pointer(xdrs, (char **)&node->data,
				    sizeof (filedetails_t),
				    xdr_filedetails_t)) {
					return (FALSE);
				}

				if (objp->head == NULL) {
					objp->head = node;
				}

				objp->length++;

				if (!xdr_bool(xdrs, &more_data))
					return (FALSE);

			}
			objp->tail = node;

			break;
		default:
			break;
	}

	return (TRUE);
}
