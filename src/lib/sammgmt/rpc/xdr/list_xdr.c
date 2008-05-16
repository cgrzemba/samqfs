
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

#pragma ident	"$Revision: 1.32 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/load.h"

/*
 * list_xdr.c
 *
 * This file contains all the XDR functions for list
 */


/*
 * rpc generated lists use recursive mode of traveral of the list
 *
 * recursive-traversal of list causes the C stack to grow linearly
 * with respect to number of nodes in the list
 *
 * So replaced the recursive node traversal with a non-recursive (iterative)
 * traversal. This needs to be done for all the lists which would potentially
 * have 10000 or more entries.
 *
 *
 */

#define	XDR_PTR2NODE(head, node) \
	if (!xdr_pointer(xdrs, (char **)&head, \
		sizeof (node_t), (xdrproc_t)xdr_ ## node)) \
		return (FALSE);

/* if it is the head, dont free it, we did not allocate memory for head  */
#define	XDR_FREE_LIST_RECURSIVE(objp, node_type) \
		node_t *tmp	= NULL; \
		node_t *node	= NULL; \
		node_t *first	= NULL; \
		first = objp; \
		if (first != NULL) { \
			tmp = first->next; \
			while (tmp != NULL) { \
				node = tmp; \
				tmp = node->next; \
				XDR_PTR2STRUCT(node->data, node_type); \
				free(node); \
			} \
		}

#define	XDR_DECODE_LIST_RECURSIVE(objp, node_type) \
		node_t *node	= NULL; \
		node_t *prev	= objp; \
		for (;;) { \
			if (!xdr_bool(xdrs, &more_data)) \
				return (FALSE); \
			if (!more_data) { \
				prev->next = NULL; \
				break; \
			} \
			if (node == NULL) { \
				node = objp; \
			} else { \
				node = (node_t *)malloc(sizeof (node_t)); \
				if (node == NULL) { \
					return (FALSE); \
				} \
				node->data = NULL; \
				node->next = NULL; \
				prev->next = node; \
			} \
			prev = node; \
			XDR_PTR2STRUCT(node->data, node_type); \
		}

#define	XDR_ENCODE_LIST_RECURSIVE(objp, node_type) \
		node_t *node = NULL; \
		node = objp; \
		for (;;) { \
			more_data = node != NULL; \
			if (!xdr_bool(xdrs, &more_data)) \
				return (FALSE); \
			if (!more_data) \
				break; \
			XDR_PTR2STRUCT(node->data, node_type); \
			node = node->next; \
		}


bool_t
xdr_string_list(
XDR *xdrs,
sqm_lst_t *objp)
{
	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	if (!xdr_pointer(xdrs, (char **)&objp->head,
	    sizeof (node_t), (xdrproc_t)xdr_string_node))
		return (FALSE);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);

	return (TRUE);
}

bool_t
xdr_string_node(
XDR *xdrs,
node_t *objp)
{
	bool_t xdr_members();

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		node_t *tmp	= NULL;
		node_t *node	= NULL;

		tmp = objp;

		while (tmp != NULL) {
			node = tmp;
			tmp = node->next;

			if (!xdr_members(xdrs, node))
				return (FALSE);

			if (tmp != objp->next) free(node);
		}
		break;
	}

	case XDR_DECODE: {
		node_t *node	= NULL;
		node_t *prev	= objp;

		for (;;) {

			if (!xdr_bool(xdrs, &more_data))
				return (FALSE);

			if (!more_data) {
				prev->next = NULL;
				break;
			}
			if (node == NULL) {
				node = objp;
			} else {
				node = (node_t *)malloc(sizeof (node_t));
				if (node == NULL) {
					return (FALSE);
				}
				node->data = NULL;
				node->next = NULL;
				prev->next = node;
			}
			prev = node;

			if (!xdr_members(xdrs, node))
				return (FALSE);

		}
		break;
	}

	case XDR_ENCODE: {
		node_t *node = NULL;

		node = objp;

		for (;;) {
			more_data = node != NULL;

			if (!xdr_bool(xdrs, &more_data))
				return (FALSE);

			if (!more_data)
				break;

			if (!xdr_members(xdrs, node))
				return (FALSE);

			node = node->next;
		}
		break;
	}
	default:
		break;
	}

	return (TRUE);

}

bool_t
xdr_members(
XDR *xdrs,
node_t *objp)
{
	if (!xdr_string(xdrs, (char **)&objp->data, ~0))
		return (FALSE);
	return (TRUE);
}

/*
 * *************************
 *  archive.h XDR functions
 * *************************
 */
bool_t
xdr_buffer_directive_node(
XDR *xdrs,
node_t *objp)
{
	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, buffer_directive_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, buffer_directive_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, buffer_directive_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_buffer_directive_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, buffer_directive_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_drive_directive_node(
XDR *xdrs,
node_t *objp)
{
	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, drive_directive_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, drive_directive_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, drive_directive_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_drive_directive_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, drive_directive_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_ar_set_criteria_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, ar_set_criteria_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, ar_set_criteria_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, ar_set_criteria_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_ar_set_criteria_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if (xdrs->x_op == XDR_FREE) {
		if (objp == NULL) {
			return (TRUE); /* nothing to be freed */
		}
	}
	XDR_PTR2NODE(objp->head, ar_set_criteria_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_ar_fs_directive_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, ar_fs_directive_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, ar_fs_directive_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, ar_fs_directive_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_ar_fs_directive_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, ar_fs_directive_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_priority_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, priority_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, priority_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, priority_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_priority_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, priority_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_ar_set_copy_params_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, ar_set_copy_params_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, ar_set_copy_params_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, ar_set_copy_params_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_ar_set_copy_params_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, ar_set_copy_params_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_vsn_pool_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, vsn_pool_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, vsn_pool_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, vsn_pool_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_vsn_pool_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, vsn_pool_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_vsn_map_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, vsn_map_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, vsn_map_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, vsn_map_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_vsn_map_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, vsn_map_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}


bool_t
xdr_ar_set_copy_cfg_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, ar_set_copy_cfg_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, ar_set_copy_cfg_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, ar_set_copy_cfg_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_ar_set_copy_cfg_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, ar_set_copy_cfg_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}


bool_t
xdr_ar_find_state_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, ar_find_state_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, ar_find_state_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, ar_find_state_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_ar_find_state_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, ar_find_state_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_archreq_node(
XDR *xdrs,
node_t *objp)
{
	int arDrives; /* for dynamic array */

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		node_t *tmp	= NULL;
		node_t *node	= NULL;

		tmp = objp;

		while (tmp != NULL) {
			node = tmp;
			tmp = node->next;

			arDrives = ((struct ArchReq *)node->data)->ArDrives;

			if (!xdr_pointer(xdrs, (char **)&node->data,
			    sizeof (struct ArchReq) +
			    ((arDrives - 1) * sizeof (struct ArcopyInstance)),
			    (xdrproc_t)xdr_ArchReq))
				return (FALSE);

			if (tmp != objp->next) free(node);
		}
		break;
	}

	case XDR_DECODE: {
		node_t *node	= NULL;
		node_t *prev	= objp;

		for (;;) {

			if (!xdr_bool(xdrs, &more_data))
				return (FALSE);

			if (!more_data) {
				prev->next = NULL;
				break;
			}
			if (node == NULL) {
				node = objp;
			} else {
				node = (node_t *)malloc(sizeof (node_t));
				if (node == NULL) {
					return (FALSE);
				}
				node->data = NULL;
				node->next = NULL;
				prev->next = node;
			}
			prev = node;

			if (!xdr_int(xdrs, &arDrives))
				return (FALSE);

			if (!xdr_pointer(xdrs, (char **)&node->data,
			    sizeof (struct ArchReq) +
			    ((arDrives - 1) * sizeof (struct ArcopyInstance)),
			    (xdrproc_t)xdr_ArchReq))
				return (FALSE);
		}
		break;
	}

	case XDR_ENCODE: {
		node_t *node = NULL;

		node = objp;

		for (;;) {
			more_data = node != NULL;

			if (!xdr_bool(xdrs, &more_data))
				return (FALSE);

			if (!more_data)
				break;

			arDrives = ((struct ArchReq *)node->data)->ArDrives;

			if (!xdr_int(xdrs, &arDrives))
				return (FALSE);

			if (!xdr_pointer(xdrs, (char **)&node->data,
			    sizeof (struct ArchReq) +
			    ((arDrives - 1) * sizeof (struct ArcopyInstance)),
			    (xdrproc_t)xdr_ArchReq))
				return (FALSE);

			node = node->next;
		}
		break;
	}
	default:
		break;
	}

	return (TRUE);

}

bool_t
xdr_archreq_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, archreq_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

/*
 * ************************
 *  device.h XDR functions
 * ************************
 */
bool_t
xdr_drive_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, drive_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, drive_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, drive_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_drive_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, drive_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_md_license_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, md_license_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, md_license_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, md_license_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_md_license_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, md_license_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_au_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, au_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, au_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, au_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_au_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, au_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_library_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, library_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, library_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, library_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_library_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, library_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

/* do not use macro for this */
bool_t
xdr_catalog_entry_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		node_t *tmp	= NULL;
		node_t *node	= NULL;

		tmp = objp;

		while (tmp != NULL) {
			node = tmp;
			tmp = node->next;

			if (!xdr_pointer(xdrs, (char **)&node->data,
			    sizeof (struct CatalogEntry),
			    (xdrproc_t)xdr_CatalogEntry))
				return (FALSE);

			if (tmp != objp->next) free(node);
		}
		break;
	}

	case XDR_DECODE: {
		node_t *node	= NULL;
		node_t *prev	= objp;

		for (;;) {

			if (!xdr_bool(xdrs, &more_data))
				return (FALSE);

			if (!more_data) {
				prev->next = NULL;
				break;
			}
			if (node == NULL) {
				node = objp;
			} else {
				node = (node_t *)malloc(sizeof (node_t));
				if (node == NULL) {
					return (FALSE);
				}
				node->data = NULL;
				node->next = NULL;
				prev->next = node;
			}
			prev = node;

			if (!xdr_pointer(xdrs, (char **)&node->data,
			    sizeof (struct CatalogEntry),
			    (xdrproc_t)xdr_CatalogEntry))
				return (FALSE);
		}
		break;
	}

	case XDR_ENCODE: {
		node_t *node = NULL;

		node = objp;

		for (;;) {
			more_data = node != NULL;

			if (!xdr_bool(xdrs, &more_data))
				return (FALSE);

			if (!more_data)
				break;

			if (!xdr_pointer(xdrs, (char **)&node->data,
			    sizeof (struct CatalogEntry),
			    (xdrproc_t)xdr_CatalogEntry))
				return (FALSE);

			node = node->next;
		}
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_catalog_entry_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, catalog_entry_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stk_host_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, stk_host_info_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, stk_host_info_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, stk_host_info_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_stk_host_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, stk_host_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stk_panel_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, stk_panel_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, stk_panel_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, stk_panel_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_stk_panel_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, stk_panel_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stk_volume_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, stk_volume_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, stk_volume_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, stk_volume_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_stk_volume_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, stk_volume_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stk_pool_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, stk_pool_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, stk_pool_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, stk_pool_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_stk_pool_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, stk_pool_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stk_lsm_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, stk_lsm_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, stk_lsm_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, stk_lsm_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_stk_lsm_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, stk_lsm_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stk_capacity_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, stk_capacity_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, stk_capacity_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, stk_capacity_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_stk_capacity_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, stk_capacity_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stk_device_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, stk_device_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, stk_device_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, stk_device_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_stk_device_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, stk_device_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

/*
 * ****************************
 *  filesystem.h XDR functions
 * ****************************
 */
bool_t
xdr_disk_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, disk_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, disk_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, disk_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_disk_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, disk_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}
bool_t
xdr_striped_group_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, striped_group_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, striped_group_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, striped_group_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_striped_group_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, striped_group_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_fs_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, fs_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, fs_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, fs_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_fs_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, fs_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_samfsck_info_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, samfsck_info_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, samfsck_info_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, samfsck_info_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_samfsck_info_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, samfsck_info_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_failed_mount_option_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, failed_mount_option_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, failed_mount_option_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, failed_mount_option_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_failed_mount_option_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, failed_mount_option_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}


/*
 * ****************************
 *  load.h XDR functions
 * ****************************
 */
bool_t
xdr_pending_load_info_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, pending_load_info_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, pending_load_info_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, pending_load_info_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_pending_load_info_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, pending_load_info_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}


/*
 * ****************************
 *  license.h XDR functions
 * ****************************
 */


/*
 * ****************************
 *  recycle.h XDR functions
 * ****************************
 */
bool_t
xdr_no_rc_vsns_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, no_rc_vsns_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, no_rc_vsns_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, no_rc_vsns_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_no_rc_vsns_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, no_rc_vsns_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_rc_robot_cfg_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, rc_robot_cfg_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, rc_robot_cfg_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, rc_robot_cfg_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_rc_robot_cfg_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, rc_robot_cfg_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}


/*
 * ****************************
 *  release.h XDR functions
 * ****************************
 */

bool_t
xdr_rl_fs_directive_node(
XDR *xdrs,
node_t *objp)
{

/*
 * Although xdr_reference can be used for pointers, xdr_reference cannot and
 * does not attach any special meaning to null-value pointer
 * xdr_pointer correctly handles NULL pointers
 */

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, rl_fs_directive_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, rl_fs_directive_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, rl_fs_directive_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_rl_fs_directive_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, rl_fs_directive_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);

	return (TRUE);
}


bool_t
xdr_rl_fs_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, release_fs_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, release_fs_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, release_fs_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_rl_fs_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, rl_fs_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);

	return (TRUE);
}

/*
 * ****************************
 *  stage.h XDR functions
 * ****************************
 */

bool_t
xdr_staging_file_info_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, staging_file_info_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, staging_file_info_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, staging_file_info_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_staging_file_info_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, staging_file_info_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_active_stager_info_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, active_stager_info_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, active_stager_info_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, active_stager_info_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_active_stager_info_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, active_stager_info_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_stager_stream_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, stager_stream_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, stager_stream_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, stager_stream_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_stager_stream_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, stager_stream_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

/* faults.h */
bool_t
xdr_fault_attr_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, fault_attr_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, fault_attr_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, fault_attr_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_fault_attr_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, fault_attr_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

/* diskvols.h */
bool_t
xdr_disk_vol_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, disk_vol_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, disk_vol_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, disk_vol_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_disk_vol_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, disk_vol_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

/* notify_summary.h */
bool_t
xdr_notify_summary_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, notf_summary_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, notf_summary_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, notf_summary_t);
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_notify_summary_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, notify_summary_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

/* job_history.h */
bool_t
xdr_job_history_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, job_hist_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, job_hist_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, job_hist_t);
		break;
	}

	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_job_history_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, job_history_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

/* hosts.h */
bool_t
xdr_host_info_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		XDR_FREE_LIST_RECURSIVE(objp, host_info_t);
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, host_info_t);
		break;
	}

	case XDR_ENCODE: {
		XDR_ENCODE_LIST_RECURSIVE(objp, host_info_t);
		break;
	}

	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_host_info_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	XDR_PTR2NODE(objp->head, host_info_node);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_arch_set_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	if (!xdr_pointer(xdrs, (char **)&objp->head,
	    sizeof (node_t), (xdrproc_t)xdr_arch_set_node))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_arch_set_node(
XDR *xdrs,
node_t *objp)
{
	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		node_t *tmp	= NULL;
		node_t *node    = NULL;

		tmp = objp;

		while (tmp != NULL) {
			node = tmp;
			tmp = node->next;

			if (!xdr_pointer(xdrs, (char **)&node->data,
			    sizeof (arch_set_t),
			    (xdrproc_t)xdr_arch_set_t))
				return (FALSE);

			if (tmp != objp->next) free(node);
		}
		break;
	}

	case XDR_DECODE: {
		node_t *node    = NULL;
		node_t *prev    = objp;

		for (;;) {

			if (!xdr_bool(xdrs, &more_data))
				return (FALSE);

			if (!more_data) {
				prev->next = NULL;
				break;
			}
			if (node == NULL) {
				node = objp;
			} else {
				node = (node_t *)malloc(sizeof (node_t));
				if (node == NULL) {
					return (FALSE);
				}
				node->data = NULL;
				node->next = NULL;
				prev->next = node;
			}
			prev = node;

			if (!xdr_pointer(xdrs, (char **)&node->data,
			    sizeof (arch_set_t), (xdrproc_t)xdr_arch_set_t))
				return (FALSE);

		}
		break;
	}

	case XDR_ENCODE: {
		node_t *node = NULL;

		node = objp;

		for (;;) {
			more_data = node != NULL;

			if (!xdr_bool(xdrs, &more_data))
				return (FALSE);

			if (!more_data)
				break;

			if (!xdr_pointer(xdrs, (char **)&node->data,
			    sizeof (arch_set_t), (xdrproc_t)xdr_arch_set_t))
				return (FALSE);

			node = node->next;
		}
		break;
	}
	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_int_node(
XDR *xdrs,
node_t *objp)
{

	bool_t more_data;

	switch (xdrs->x_op) {

	case XDR_FREE: {
		node_t *tmp	= NULL;
		node_t *node	= NULL;

		tmp = objp;

		while (tmp != NULL) {
			node = tmp;
			tmp = node->next;
			if (!xdr_pointer(xdrs, (char **)&node->data,
			    sizeof (int), (xdrproc_t)xdr_int))
				return (FALSE);
			/* if it is the head, dont free it */
			if (tmp != objp->next) free(node);
		}
		break;
	}

	case XDR_DECODE: {
		XDR_DECODE_LIST_RECURSIVE(objp, int);
		break;
	}

	case XDR_ENCODE: {
		node_t *node = NULL;
		node = objp;

		for (;;) {
			more_data = node != NULL;

			if (!xdr_bool(xdrs, &more_data))
				return (FALSE);

			if (!more_data)
				break;

			if (!xdr_pointer(xdrs, (char **)&node->data,
			    sizeof (int), (xdrproc_t)xdr_int))
				return (FALSE);

			node = node->next;
		}
		break;
	}

	default:
		break;
	}

	return (TRUE);
}

bool_t
xdr_int_list(
XDR *xdrs,
sqm_lst_t *objp)
{

	if ((xdrs->x_op == XDR_FREE) && (objp == NULL)) {
		return (TRUE); /* nothing to be freed */
	}
	if (!xdr_pointer(xdrs, (char **)&objp->head,
	    sizeof (node_t), (xdrproc_t)xdr_int_node))
		return (FALSE);

	if (!xdr_int(xdrs, &objp->length))
		return (FALSE);
	return (TRUE);
}
