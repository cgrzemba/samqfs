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


#ifndef _CFG_RELEASER_H
#define	_CFG_RELEASER_H

#pragma	ident	"$Revision: 1.13 $"

/*
 *	releaser.h
 *	Used to read and change the releaser configuration.
 */

#include <sys/types.h>

#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/release.h"

typedef struct releaser_cfg {
	sqm_lst_t *rl_fs_dir_list;
	time_t	read_time;
} releaser_cfg_t;


/*
 *	Given a file system name, get corresponding
 *	file system's releaser structure.
 */
int cfg_get_rl_fs_directive(const releaser_cfg_t *cfg, const char *fs_name,
		rl_fs_directive_t **fs_release);


/*
 * Read releaser configuration from the default location and
 * build the releaser_cfg_t structure.
 */
int read_releaser_cfg(releaser_cfg_t **rel_cfg);


/*
 * Write the releaser.cmd configuration to the default location.
 *
 * If force is false and the configuration has been modified since
 * the releaser_cfg_t structure was read in, this function will
 * set errno and return -1
 *
 * If force is true the releaser configuration will be written without
 * regard to its modification time.
 */
int write_releaser_cfg(const releaser_cfg_t *rel_cfg, const boolean_t force);


/*
 * dump the configuration to the specified location.  If a file exists at
 * the specified location it will be overwritten.
 */
int dump_releaser_cfg(const releaser_cfg_t *rel_cfg, const char *location);


void free_releaser_cfg(releaser_cfg_t *rel_cfg);

/* from parse_releaser.c */

int verify_rl_fs_directive(rl_fs_directive_t *fs, sqm_lst_t *filesystems);

int parse_releaser_cmd(char *file, sqm_lst_t *fs_list, releaser_cfg_t **cfg);

int write_releaser_cmd(const char *location, const releaser_cfg_t *cfg);

#endif /* _CFG_RELEASER_H */
