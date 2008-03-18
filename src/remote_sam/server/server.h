/*
 * server.h
 */
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

/*
 * $Revision: 1.15 $
 */

#if !defined(SERVER_H)
#define	SERVER_H

#include "aml/remote.h"
#include "aml/catlib.h"

#define	RMT_SEND_VSN_COUNT 100

typedef struct {
	int count;
	rmt_sam_vsn_entry_t entries[RMT_SEND_VSN_COUNT];
} rmt_entries_found_t;

/*
 * Define function prototypes in main.c
 */
int step(const char *p1, const char *p2);

/*
 * Define function prototypes in connects.c
 */
void *watch_connects(void *vun);

/*
 * Define function prototypes in server.c
 */
void *serve_client(void *vclient);
void reply_message(rmt_sam_client_t *client, rmt_sam_request_t *req,
	rmt_resp_type_t type, int err);

/*
 * Define function prototypes in message.c
 */
void *rs_monitor_msg(void *vun);

/*
 *	Define function prototypes in vsn_list.c
 */
void *send_vsn_list(void *vclient);
int flush_list(rmt_sam_client_t *client, rmt_entries_found_t *found);
int add_vsn_entry(rmt_sam_client_t *client, rmt_entries_found_t *found,
	struct CatalogEntry *ce, uint_t flags);
int run_regex(rmt_vsn_equ_list_t *current_equ, char *string);

#endif	/* SERVER_H */
