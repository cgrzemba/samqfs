/*
 *	sam_list.h - list structures and macros
 *
 *	Makes a unified layer for accessing lists
 *	under solaris10+, solaris9.
 *
 */

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

#ifndef _SAM_LIST_H
#define	_SAM_LIST_H

#ifdef sun
#pragma ident "$Revision: 1.11 $"
#endif

#ifdef sun
#include <sys/list.h>

#ifdef _KERNEL
#define	list_enqueue(list, item)	list_insert_tail((list), (item))

static inline void* list_dequeue(struct list *list) {
	void *item = NULL;

	item = list_head(list);
	if (item != NULL) {
		list_remove(list, item);
	}
	return (item);
}
#endif /* _KERNEL */
#endif /* sun */

#ifdef linux
#ifndef __KERNEL__
struct	list_head {
	void *next, *prev;
};
#else
#include <linux/list.h>
#ifndef list_first
#define	list_first(head)	(((head)->next != (head)) ? \
				(head)->next : (struct list_head *)0)
#endif
#define	list_insert_tail(a, b)	list_add_tail((struct list_head *)(b), (a))
#define	list_create(a, b, c)	INIT_LIST_HEAD((a))
#define	list_head(head)	list_first(head)

#define	list_enqueue(list, item) list_add_tail((struct list_head *)(item), \
						(list))

static inline void* list_dequeue(struct list_head *list) {
	void *item = NULL;

	item = list_first(list);
	if (item != NULL) {
		list_del(item);
	}
	return (item);
}
#endif /* __KERNEL__ */
#endif /* linux */

#endif /* _SAM_LIST_H */
