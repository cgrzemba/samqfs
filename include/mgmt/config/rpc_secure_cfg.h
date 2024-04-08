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
 * or https://illumos.org/license/CDDL.
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
#ifndef	_RPC_SECURE_CONFIG_H
#define	_RPC_SECURE_CONFIG_H

#pragma ident   "$Revision: 1.12 $"


/*
 * rpc_secure_config.h
 *
 * RPC mgmt daemon should not respond to requests coming from any client
 * in the network. User/sysadm must add the clients that can manage this
 * SAM-FS server to a configuration file. Only if the client (ip/hostname)
 * exists in the configuration file, then access to the SAM-FS server via
 * the rpc mgmt daemon (SAM-QFS Manager daemon is allowed)
 *
 * Used to define secure clients to the rpc mgmt server daemon
 */

#include "sam/types.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/sqm_list.h"

#define	CLNTCFG VAR_DIR"/clntcfg.bin"

typedef struct clientCfg {
	int num_ip;	/* number of client ips */
	int num_host;	/* number of client hostnames */
	upath_t addr_arr[1]; /* length equal to num_ip + num_host */
} clientCfg_t;

/*
 * get_clients
 *
 * get all clients that can manage this SAM-FS/QFS server via the
 * SAM-QFS Manager daemon
 *
 * returns 0 on success (lst_ip and lst_host are populated)
 * returns -1 on error (samerrno and samermsg are populated)
 * If there is no config file, empty list is returned
 *
 */
int get_clients(sqm_lst_t **lst_ip, sqm_lst_t **lst_host);


/*
 * is_ipaddr
 *
 * Checks if the addr is in dotted decimal format or is expressed
 * as a hostname
 *
 * No ip address validation check is done
 * This is solely to distinguish between the user entries in dotted
 * decimal format versus hostname/alias format so that
 * when host2ip translation is required (when matching ip addrs at the
 * server side) unneccessary trnaslation is avaoided
 *
 * input argument char *ip (hostname or ip)
 * returns B_TRUE on success, B_FALSE if no match or err
 */
boolean_t is_ipaddr(char *ip);

#endif /* _RPC_SECURE_CONFIG_H */
