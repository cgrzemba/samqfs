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

#ifndef _OBJNODE
#define _OBJNODE

#include "inode.h"

typedef struct objnode {
	uint32_t obj_busy;
} objnode_t;

#if !defined(_NoOSD_)
void sam_objnode_alloc(sam_node_t * ip);
void sam_objnode_init(sam_node_t * ip);
void sam_objnode_free(sam_node_t * ip);
#else
#define sam_objnode_alloc(ip)
#define sam_objnode_init(ip)
#define sam_objnode_free(ip)
#endif /* _NoOSD_ */

#endif
