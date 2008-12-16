/*
 *  librobots.h
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

/*
 * $Revision: 1.15 $
 */

#ifndef _SAM_LIBROBOTS_H
#define	_SAM_LIBROBOTS_H
#include "aml/robots.h"

/* Function prototypes for link list management */

robo_event_t *init_list(int);
robo_event_t *append_list(robo_event_t *, robo_event_t *);
robo_event_t *insert_list(robo_event_t *, robo_event_t *);
robo_event_t *unlink_list(robo_event_t *);
robo_event_t *add_active_list(robo_event_t *, robo_event_t *);

/* Macro to set b to the address of the last element on list a */
#define	LISTEND(a, b) for (b = (a)->first; (b)->next != NULL; (b) = (b)->next)

/* common robot routines */

void clear_un_fields(dev_ent_t *);

#endif /* _SAM_LIBROBOTS_H */
