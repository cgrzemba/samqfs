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
#ifndef CFG_DISKVOLS_H
#define	CFG_DISKVOLS_H

#pragma ident   "$Revision: 1.14 $"

/*
 * diskvol.h
 * Used to read the disk archive volume configuration and make changes to
 * it. This header contains structures related to the diskvols.conf file.
 */

#include <sys/types.h>

#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/diskvols.h"
#include "pub/mgmt/device.h"


#define	DV_path		0x00000001
#define	DV_host		0x00000002
#define	DV_vol_name	0x00000004

typedef struct diskvols_cfg {
	sqm_lst_t *disk_vol_list;	/* list of disk_vols_t */
	sqm_lst_t *client_list;	/* list of host_t w/names of trusted clients */
	time_t	read_time;
} diskvols_cfg_t;



/*
 * Return one disk_vol_t based on vsn name.
 */
int get_disk_vsn(const diskvols_cfg_t *cfg, const char *disk_vsn,
		disk_vol_t **disk_vsn_path);

/*
 * Returns an error if the disk volume is not complete or has errors.
 */
int check_disk_vol(disk_vol_t *dv);

/*
 * Read the diskvols.conf configuration from the defualt location and
 * build the diskvols_cfg_t structure. Alternately, by specifying a
 * read_location in the ctx argument you can read from a diskvols written
 * at an alternate location.
 */
int read_diskvols_cfg(ctx_t *ctx, diskvols_cfg_t **diskvols);


/*
 * Write the diskvols.conf configuration to the default location or
 * optionally to the location specified in the ctx argument.
 *
 * If force is false and the configuration has been modified since
 * the diskvols_cfg_t struture was read in this function will
 * set errno and return -1
 *
 * If force is true the diskvols configuration will be written without
 * regard to its modification time.
 */
int write_diskvols_cfg(ctx_t *ctx,
    diskvols_cfg_t *diskvols,
    const boolean_t force);

/*
 * dump the configuration to the specified location.  If a file exists at
 * the specified location it will be overwritten.
 */
int dump_diskvols_cfg(const diskvols_cfg_t *diskvols, const char *location);


/*
 * free the diskvols_cfg_t structure and all of its members and submembers.
 */
void free_diskvols_cfg(diskvols_cfg_t *diskvols);

/*
 * function to parse the diskvols.conf file. Should not be called directly.
 * call read instead.
 */
int parse_diskvols_conf(char *file, diskvols_cfg_t **cfg);

/*
 * called internally by the get_fs() API, in order to populate scsiinfo field
 */
int add_scsi_info(au_t *au);

#endif /* CFG_DISKVOLS_H */
