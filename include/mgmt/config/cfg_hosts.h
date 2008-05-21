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

#ifndef _CFG_HOSTS_H
#define	_CFG_HOSTS_H

#pragma ident "	$Revision: 1.13 $	"

#include "pub/mgmt/types.h"
#include "pub/mgmt/filesystem.h"
#include "pub/mgmt/hosts.h"

/*
 * Function for internal use to create a hosts configuration file.
 *
 * preconditions:
 * 1. This function is intended for  creating a new hosts
 * file. Any other use is at  callers risk. Specifically the caller is
 * responsible for verifying the  preconditions of any more specific
 * operation that is being achieved through this general function.
 *
 * 2. The fs_t structure must accurately reflect the actual mount state of the
 * file system.
 *
 * Postconditions:
 * 1. The hosts file has been created but sammkfs must be used to
 * cause the reading of this file.
 */
int
cfg_set_hosts_config(
ctx_t *ctx,		/* context argument for use by RPC */
fs_t *fs);		/* file system containing a hosts_config to be added */


/*
 * Internal function to add a host to the host config of a file system.
 *
 * preconditions and rules:
 * 1. file must have been created
 * 2. If adding a host that has current server set to true the field
 * will be ignored.
 */
int
cfg_add_host(
ctx_t *ctx,		/* context argument for RPC use */
fs_t *fs,		/* the file system for which the host is being added */
host_info_t *h);	/* The host to add */


/*
 * Internal function to get a hosts list for a shared file system, given
 * the file system structure. If kv_options is 0 host_info_t structs will
 * be returned. Otherwise key value pairs will be returned.
 *
 * Preconditions:
 * 1. file system is shared.
 * 2. call is being made on a metadata server or a potential metadata server.
 */
int
cfg_get_hosts_config(
ctx_t *ctx,		/* context argument for RPC use */
fs_t *fs,		/* fs to get hosts for */
sqm_lst_t **hosts,	/* list of host_info_t or kv strings see kv_options */
int32_t kv_options);	/* If != 0 kv strings will be returned */


/*
 * Internal function to remove a host from the hosts.<fs_name> file.
 *
 * precoditions:
 * 1. fs_t must have been retrieved from the get_fs or get_all_fs routines.
 */
int
cfg_remove_host(
ctx_t *ctx,		/* context argument for RPC use */
fs_t *fs,		/* fs to remove host for */
char *host_name);	/* The name of the host to remove */


/*
 * Internal function to create a hosts config. This function creates or
 * updates the hosts config as appropriate depending on wether this
 * host is a metadata server or a potential metadata server.
 *
 * preconditions:
 * 1. if this is a potential metadata server the metadata server must
 * have been created first.
 * 2. This function will return success without doing anything if it is
 * called for a client host.
 */
int
create_hosts_config(
ctx_t *ctx,	/* context for RPC use */
fs_t *fs);	/* The fs for which to create a host config */


#endif /* _CFG_HOSTS_H */
