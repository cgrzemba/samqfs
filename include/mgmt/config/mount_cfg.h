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
#ifndef	_MOUNT_CFG_H
#define	_MOUNT_CFG_H

#pragma ident   "$Revision: 1.18 $"

/*
 * mount_cfg.h
 * Used to read and configure default mount parameters for SAM and QFS file
 * systems. Parameters specified here can be overriden by passing
 * parameters to the mount command.
 *
 * The structures in this header map to the samfs.cmd configuration file.
 *
 */

#include <sys/types.h>
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/filesystem.h"

/*
 * Structure for the entire mount paramters config.
 *
 * inodes is the max number of inodes in all file systems. If it is set
 * to anything less than ncsize(from solaris name cache it will default to
 * ncsize)
 */
typedef struct mount_cfg {
	int	inodes;
	sqm_lst_t	*fs_list;
	time_t	read_time;
} mount_cfg_t;


/*
 * Given a file system name, get corresponding
 * file system's mount option (name value pair).
 */
int get_fs_mount_opts(const mount_cfg_t *cfg, const char *fs_name,
    mount_options_t **params);


/*
 * changes the mount options for the file system input.
 * if ctx is non-NULL and contains a non-empty dump_path this will dump
 * the mount options to the location specified. Additionally if the above
 * are true and for_create is true and the filesystem is not found in the
 * existing config it will not be considered an error and the mount options
 * will be dumped to a samfs.cmd file in the directory indicated in ctx.
 */
int cfg_change_mount_options(ctx_t *ctx, fs_t *input, boolean_t for_create);


/*
 * returns a mount_opts_structure that contains all of the default values.
 * This method may need to change to get a mount_cfg_t structure that
 * gives the defaults for each file  system.  The reason is that
 * some of the default values are dependent on fs type and device mix.
 *
 */
int get_default_mount_opts(mount_options_t **defs);


/*
 * This is equivalent of deleting the options for the named filesystem
 */
int reset_default_mount_options(ctx_t *ctx, mount_cfg_t *mnt_cfg,
	uname_t fs_name);

/*
 * Read samfs.cmd configuration and build the mount_cfg_t structure.
 * It is legal for there to be no configuration for the mount parameters.
 * if this is the case success will be returned and *cfg will contain
 * initialized mount_opts for the global and one for each file system.
 *
 * For each filesystem all options will have values.
 * If there is no file system value for an option the global one will be
 * set.  If there is no global setting the default will be set.
 * If a value is explicitly set for the option the corresponding bit
 * in the change flag will be set.
 *
 */
int read_mount_cfg(mount_cfg_t **cfg);



/*
 * pass in a list of fs_t structures for all configured file systems.
 * the structures will be populated with mount options when returned.
 * This function behaves exactly like read_mount_cfg except that it does
 * not read in the mcf to get information about existing file systems.
 * it simply uses those in the passed in structure.
 */
int populate_mount_cfg(mount_cfg_t *cfg);


/*
 * Write the samfs.cmd configuration to the default location.
 *
 * If force is false and the configuration has been modified since
 * the mount_cfg_t struture was read in, this function will
 * set errno and return -1
 *
 * If force is true the samfs.cmd configuration will be written without
 * regard to its modification time.
 */
int write_mount_cfg(ctx_t *ctx, mount_cfg_t *cfg, const boolean_t force,
	boolean_t for_create);


/*
 * Dump the configuration to the specified location.  If a file exists at
 * the specified location it will be overwritten.
 */
int dump_mount_cfg(mount_cfg_t *cfg, char *location);


/*
 * verify that the mount_cfg_t passed in is legal.
 */
int verify_mount_cfg(ctx_t *ctx, mount_cfg_t *cfg, boolean_t for_create);


/*
 * copy all set fields from input to fs_to_file.
 */
int copy_fs_set_fields(fs_t *fs_to_file, fs_t *input);


void free_mount_cfg(mount_cfg_t *cfg);

/* from parse_samfs_cmd.c */

int parse_samfs_cmd(char *fscfg_name, mount_cfg_t *cfg);
int write_samfs_cmd(char *location, mount_cfg_t *cfg);

#endif /* _MOUNT_CFG_H */
