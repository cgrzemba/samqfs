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
#ifndef _SAMMGMT_RPC_H_
#define	_SAMMGMT_RPC_H_

#pragma ident	"$Revision: 1.16 $"


#include "pub/mgmt/types.h"
#include "mgmt/log.h"		/* for PTRACE in all consumers */

#define	DEF_TIMEOUT_SEC	65

/*
 * this returns the mgmt API version used by client-side RPC.
 * It must match the API version implemented on the SAM-FS/QFS server.
 * (see mgmt.h)
 */
char *samrpc_version();

samrpc_client_t *samrpc_create_clnt_timed(
    const char *rpc_server, time_t tv_sec);

samrpc_client_t *samrpc_create_clnt(const char *rpc_server);

int samrpc_destroy_clnt(samrpc_client_t *rpc_client);
int samrpc_set_timeout(samrpc_client_t *rpc_client, time_t tv_sec);
int samrpc_get_timeout(samrpc_client_t *rpc_client, time_t *tv_sec);
int get_server_info(ctx_t *ctx, char hostname[MAXHOSTNAMELEN + 1]);

/*
 * On the client side extract the server's API version from an
 * initialized context argument.
 */
char *get_server_version_from_ctx(ctx_t *c);

#endif	/* _SAMMGMT_RPC_H_ */
