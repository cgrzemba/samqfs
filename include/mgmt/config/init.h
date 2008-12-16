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
#ifndef _CFG_INIT
#define	_CFG_INIT

#pragma ident   "$Revision: 1.16 $"

/*
 * init.h
 * contains function declarations and types used to check and update the
 * initialization status of the configuration files.
 */
#include <sys/types.h>

#define	INIT_MCF		0x00000001
#define	INIT_SAMFS_CMD		0x00000002
#define	INIT_ARCHIVER_CMD	0x00000004
#define	INIT_DISKVOLS_CONF	0x00000008
#define	INIT_STAGER_CMD		0x00000010
#define	INIT_RELEASER_CMD	0x00000020
#define	INIT_RECYCLER_CMD	0x00000040
#define	INIT_DEFAULTS_CONF	0x00000080
#define	INIT_VFSTAB_CFG		0x00000100
#define	INIT_ALL_CFGS		(INIT_MCF|INIT_SAMFS_CMD|INIT_ARCHIVER_CMD|\
				INIT_DISKVOLS_CONF|INIT_STAGER_CMD|\
				INIT_RELEASER_CMD|INIT_RECYCLER_CMD|\
				INIT_DEFAULTS_CONF|INIT_VFSTAB_CFG)


/*
 * Get the mod time for a single file. If the time == 0,
 * the file does not exist. Will return an error if the
 * library has not been initialized or the which_cfg is not
 * recognized.
 */
int
get_file_time(int32_t which_cfg, time_t *t);


#endif /* _CFG_INIT */
