/*
 * drive.h
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
 * $Revision: 1.17 $
 */

#ifndef _DRIVE_H
#define	_DRIVE_H

/* function prototypes */

int grau_get_media(library_t *, drive_state_t *, robo_event_t *, int,
    const uint_t);
void grau_clean(drive_state_t *, robo_event_t *);
int get_media(library_t *, drive_state_t *, robo_event_t *,
			struct CatalogEntry *);
void audit(drive_state_t *, const uint_t, const int);
void clean(drive_state_t *, robo_event_t *);
void drive_state_change(drive_state_t *, state_change_t *);
void drive_tapealert_solicit(drive_state_t *, tapealert_request_t *);
void drive_sef_solicit(drive_state_t *, sef_request_t *);
void label_slot(drive_state_t *, robo_event_t *);
void mount(drive_state_t *, robo_event_t *);
void load_unavail(drive_state_t *, robo_event_t *);
void move_list(drive_state_t *, library_t *);
void todo(drive_state_t *, robo_event_t *);


#endif /* _DRIVE_H */
