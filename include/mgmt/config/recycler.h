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

#ifndef _CFG_RECYCLER_H
#define	_CFG_RECYCLER_H

#pragma	ident	"$Revision: 1.11 $"
/*
 * recycler.h
 * Read the current recycler configuration and
 * configure the behavior of the recycler.
 */

#include "sam/types.h"		/* for upath_t */

#include "pub/mgmt/sqm_list.h"

#define	RC_recycler_log	0x00000001
#define	RC_script	0x00000001


/*
 *	This structure define recycle configuration
 *	file components.
 */
typedef struct recycler_cfg {
	upath_t	recycler_log;
	upath_t script;
	sqm_lst_t	*no_recycle_vsns;	/* all no_recycle vsns */
	sqm_lst_t	*rc_robot_list;		/* all library lists */
	uint32_t change_flag;
	time_t	read_time;

} recycler_cfg_t;


/*
 *	Read recycler configuration from the defualt location
 *	and build the recycler_cfg_t structure.
 */
int read_recycler_cfg(recycler_cfg_t **rc_cfg);


/*
 * Write the recylcer.cmd configuration to the default location.
 *
 * If force is false and the configuration has been modified since
 * the recycler_cfg_t struture was read in this function will
 * set errno and return -1
 *
 * If force is true the recycler configuration will be written without
 * regard to its modification time.
 */
int write_recycler_cfg(recycler_cfg_t *rc_cfg, const boolean_t force);

/*
 * dump the configuration to the specified location.  If a file exists at
 * the specified location it will be overwritten.
 */
int dump_recycler_cfg(const recycler_cfg_t *rc_cfg, const char *location);

/*
 * free	 all members and sub-members of the recycler_cfg_t structure
 */
void free_recycler_cfg(recycler_cfg_t *rc_cfg);

/* from parse_recycler.c */

/*
 * Read command file
 */
int parse_recycler(char *file, recycler_cfg_t **cfg);

#endif /* _CFG_RECYCLER_H */
