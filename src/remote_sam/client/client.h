/*
 *	client.h
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

/*
 * $Revision: 1.16 $
 */

#if !defined(CLIENT_H)
#define	CLIENT_H

#include "aml/remote.h"
#include "aml/robots.h"
#include "aml/historian.h"

/*
 *	Define function prototypes in catalog.c
 */
void request_vsn_list(dev_ent_t *un);
void update_catalog_entries(dev_ent_t *un, rmt_sam_request_t *req);
void mark_catalog_unavail(dev_ent_t *un);

/*
 *	Define function prototypes in connects.c
 */
void *connect_server(void *vun);

/*
 *	Define function prototypes in message.c
 */
void *rc_monitor_msg(void *vun);

#endif /* CLIENT_H */
