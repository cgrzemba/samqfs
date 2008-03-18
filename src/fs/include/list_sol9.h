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

#ifndef	_SYS_LIST_SOL9_H
#define	_SYS_LIST_SOL9_H

#pragma ident	"@(#)list_sol9.h	1.3	04/07/15 SMI"

#include <sys/types.h>

#ifdef	__cplusplus
extern "C" {
#endif

struct list_node {
	struct list_node *list_next;
	struct list_node *list_prev;
};

struct list {
	size_t	list_size;
	size_t	list_offset;
	struct list_node list_head;
};

typedef struct list_node list_node_t;
struct list;

#ifdef _KERNEL
typedef struct list list_t;

void list_create_s9(list_t *, size_t, size_t);
void list_destroy_s9(list_t *);

void list_insert_after_s9(list_t *, void *, void *);
void list_insert_before_s9(list_t *, void *, void *);
void list_insert_head_s9(list_t *, void *);
void list_insert_tail_s9(list_t *, void *);
void list_remove_s9(list_t *, void *);
void list_move_tail_s9(list_t *, list_t *);

void *list_head_s9(list_t *);
void *list_tail_s9(list_t *);
void *list_next_s9(list_t *, void *);
void *list_prev_s9(list_t *, void *);

#endif /* _KERNEL */

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_LIST_SOL9_H */
