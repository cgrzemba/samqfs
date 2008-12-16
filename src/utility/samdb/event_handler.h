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
#ifndef EVENT_HANDLER_H_
#define	EVENT_HANDLER_H_

#include <sam/samevent.h>
#include <sam/sam_db.h>

/* Event handler function type, Return 0 on success, -1 on failure */
typedef int (*event_handler_t)(sam_db_context_t *, sam_event_t *);

#define	IS_DB_INODE(ino) ((ino) >= SAM_MIN_USER_INO || (ino) == SAM_ROOT_INO)

event_handler_t get_event_handler(int ev_num);
char *get_event_name(int ev_num);
int check_consistency(sam_db_context_t *, sam_event_t *, boolean_t repair_dir);

#endif /* EVENT_HANDLER_H_ */
