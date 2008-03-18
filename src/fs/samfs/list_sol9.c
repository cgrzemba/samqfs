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

#include <unistd.h>
#include <sys/debug.h>
#include "sam/osversion.h"

/* this file is empty for solaris 10 and up */
#ifndef SOL_510_ABOVE

#pragma ident   "@(#)list_sol9.c     1.4     04/07/15 SMI"

/*
 * Generic doubly-linked list implementation
 */

#include "list_sol9.h"

#define	list_d2l_s9(a, obj) ((list_node_t *)(((char *)obj) + (a)->list_offset))
#define	list_object_s9(a, node) ((void *)(((char *)node) - (a)->list_offset))
#define	list_empty_s9(a) ((a)->list_head.list_next == &(a)->list_head)

#define	list_insert_after_node_s9(list, node, object) {    \
	list_node_t *lnew = list_d2l_s9(list, object);     \
	lnew->list_prev = node;                         \
	lnew->list_next = node->list_next;              \
	node->list_next->list_prev = lnew;              \
	node->list_next = lnew;                         \
}

#define	list_insert_before_node_s9(list, node, object) {   \
	list_node_t *lnew = list_d2l_s9(list, object);     \
	lnew->list_next = node;                         \
	lnew->list_prev = node->list_prev;              \
	node->list_prev->list_next = lnew;              \
	node->list_prev = lnew;                         \
}

void
list_create_s9(list_t *list, size_t size, size_t offset)
{
	ASSERT(list);
	ASSERT(size > 0);
	ASSERT(size >= offset + sizeof (list_node_t));

	list->list_size = size;
	list->list_offset = offset;
	list->list_head.list_next =
	    list->list_head.list_prev = &list->list_head;
}

void
list_destroy_s9(list_t *list)
{
	list_node_t *node = &list->list_head;

	ASSERT(list);
	ASSERT(list->list_head.list_next == node);
	ASSERT(list->list_head.list_prev == node);

	node->list_next = node->list_prev = NULL;
}

void
list_insert_after_s9(list_t *list, void *object, void *nobject)
{
	list_node_t *lold = list_d2l_s9(list, object);
	list_insert_after_node_s9(list, lold, nobject);
}

void
list_insert_before_s9(list_t *list, void *object, void *nobject)
{
	list_node_t *lold = list_d2l_s9(list, object);
	list_insert_before_node_s9(list, lold, nobject)
}

void
list_insert_head_s9(list_t *list, void *object)
{
	list_node_t *lold = &list->list_head;
	list_insert_after_node_s9(list, lold, object);
}

void
list_insert_tail_s9(list_t *list, void *object)
{
	list_node_t *lold = &list->list_head;
	list_insert_before_node_s9(list, lold, object);
}

void
list_remove_s9(list_t *list, void *object)
{
	list_node_t *lold = list_d2l_s9(list, object);
	ASSERT(!list_empty_s9(list));
	lold->list_prev->list_next = lold->list_next;
	lold->list_next->list_prev = lold->list_prev;
	lold->list_next = lold->list_prev = NULL;
}

void *
list_head_s9(list_t *list)
{
	if (list_empty_s9(list))
		return (NULL);
	return (list_object_s9(list, list->list_head.list_next));
}

void *
list_tail_s9(list_t *list)
{
	if (list_empty_s9(list))
		return (NULL);
	return (list_object_s9(list, list->list_head.list_prev));
}

void *
list_next_s9(list_t *list, void *object)
{
	list_node_t *node = list_d2l_s9(list, object);

	if (node->list_next != &list->list_head)
		return (list_object_s9(list, node->list_next));

	return (NULL);
}

void *
list_prev_s9(list_t *list, void *object)
{
	list_node_t *node = list_d2l_s9(list, object);

	if (node->list_prev != &list->list_head)
		return (list_object_s9(list, node->list_prev));

	return (NULL);
}

/*
 *  Insert src list after dst list. Empty src list thereafter.
 */
void
list_move_tail_s9(list_t *dst, list_t *src)
{
	list_node_t *dstnode = &dst->list_head;
	list_node_t *srcnode = &src->list_head;

	ASSERT(dst->list_size == src->list_size);
	ASSERT(dst->list_offset == src->list_offset);

	if (list_empty_s9(src))
		return;

	dstnode->list_prev->list_next = srcnode->list_next;
	srcnode->list_next->list_prev = dstnode->list_prev;
	dstnode->list_prev = srcnode->list_prev;
	srcnode->list_prev->list_next = dstnode;

	/* empty src list */
	srcnode->list_next = srcnode->list_prev = srcnode;
}

#endif
