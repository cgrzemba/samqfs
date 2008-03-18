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

#ifndef _CFG_FS_H
#define	_CFG_FS_H

#pragma ident   "$Revision: 1.16 $"

#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/filesystem.h"
#include "mgmt/config/master_config.h"

/*
 * cfg_fs.h --  internal functions for file system configuration. These
 * functions get information only from the config files. It is not expected
 * that the GUI will ever call these directly.
 */


/*
 * get_all_fs returns both the mounted and unmounted
 * file systems.
 */
int cfg_get_all_fs(sqm_lst_t **fs_list);

/*
 * return the names of all configured filesystems
 */
int cfg_get_fs_names(sqm_lst_t **fs_names);

/*
 * get the information about a specific file system.
 */
int cfg_get_fs(uname_t fsname, fs_t **fs);


/*
 * private function to create an fs struct given a list of devices and a name.
 */
int create_fs_struct(char *fs_name, sqm_lst_t *devs, fs_t **fs);


/*
 * check_config_with_fsd()
 * can be used to verify the mcf, samfs.cmd, diskvols.conf
 * files. It returns a list of the errors printed to stderr by
 * the sam-fsd command.
 *
 * If this function executes error free and no errors are detected in the
 * file being checked 0 is returned. The list parsing_errs will be NULL.
 *
 *
 * If this function executes error free but encounters errors in the cfg file
 * being checked the status of sam-fsd is returned and the parsing_errs list is
 * populated with strings describing the errors in the cmd file. The caller
 * needs to call lst_free_deep for parsing_errs.
 *
 * If this function experiences internal errors -1 is returned and samerrno
 * and samerrmsg are set. The list parsing_errs will be NULL.
 *
 * There is an ordering adhered to by the underlying function.
 * The order files are checked is defaults.conf mcf diskvols.conf samfs.cmd
 *
 * If errors are encountered in the defaults.conf no more files are processed.
 * If errors are encountered in the mcf no more files are processed.
 * If defaults.conf and mcf are error free the remaining
 * files both get checked. This means that you can get errors that pertain
 * to both the diskvols.conf and samfs.cmd files at the same time. But if you
 * get defaults.conf errors or mcf errors that it is all you will get.
 *
 * Parsing errors may be for any of the files. Including ones with NULL input
 * arguments.
 */
int
check_config_with_fsd(char *mcf_location, char *diskvols_location,
    char *samfs_location, char *defaults_location, sqm_lst_t **parsing_errs);



/*
 * given a list of fs_t structures return the named fs struct
 */
int
find_fs(sqm_lst_t *fs_list, char *fs_name, fs_t **fs);


/*
 * given a file system structure that is for a shared file system that has
 * actual metadata devices, populate the fi_status field with FS_CLIENT or
 * FS_SERVER depending on the host config.
 */
int set_shared_fs_status_flags(fs_t *f);


/*
 * This function will read the super block entries on each device in the
 * shared file system.
 * This function will check:
 *	The superblock is for the same fs as the fs_t
 *	that all devices are present for the filesystem.
 *	all devices have the right equipment ordinal in the fs_t
 *	that the device types are right in the fs_t
 *	that the mcf file has the devices in the right order.
 *
 *	preconditions: file system must have been created on some node
 *	for this function to work.
 *
 * There are a set of checks that check the sblk vs the mcf passed in.
 *
 * There are a set of checks that are performed between the super
 * blocks from each device to make sure they are all in agreement.
 * This includes data count, mm_count, fs eq, family set name, time
 * superblock initialized
 */
int
shared_fs_check(ctx_t *ctx, char *fs_name, mcf_cfg_t *mcf);

#endif /* _CFG_FS_H */
