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

#ifndef	FSM_CONTROL_H
#define	FSM_CONTROL_H

#pragma ident   "$Revision: 1.14 $"

/*
 * fscmd.h
 *
 * contains non-API file systems-related functions
 */

#include <sys/types.h>

/*
 * create a SAM/SAMQFS/QFS file system.
 * use 0 if you want to use default values for arguments
 */
int
fs_mk(const char *name,
    const int dau,	/* disk allocation unit size (blocks) */
    const int inodes,	/* NOTE: if set now, cannot be increased later */
    const boolean_t prevsblk,	/* previous superblock version */
    const boolean_t shared);	/* create a shared fs */

/*
 * grow an unmounted SAM/SAMQFS/QFS file system.
 * a new device must have been added for the specified file system
 * prior to calling this function.
 * NOTE: this function will likely be moved to a private header file and
 * replaced with a higher level one that will:
 *    - add new device to the fs
 *    - grow the fs
 *    - mount the fs
 *
 */
int
fs_grow(const char *name);


/*
 * perform a samfsck and put output in the identified output file
 */
int
samfsck(const char *fs_name,
    const char *scratch_dir,	/* use /tmp if null */
    const boolean_t repair,	/* if false just reports errors */
    const boolean_t verbose,
    const boolean_t hash,	/* generate directory hash */
    const char *output_file);

/*
 * This method is an extension of the samfsck -R option that changes the file
 * system name from the old name to the new name. This will involve a fair bit
 * of reconfiguration work with changes to the mcf, and /etc/vfstab files.
 */
int
rename_fs(const char *old_name, const char *new_name);




/*
 * preconditions:
 * 1. Cannot set update and write_local_copy at the same time.
 */
int
sharefs_cmd(
const char *fs_name,		/* The name of the file system */
const char *server,		/* if not NULL, will set new server */
const boolean_t update,		/* if true will read in hosts.fsname */
const boolean_t fs_mounted,	/* if false will use info from raw device */
const boolean_t write_local_copy); /* if true will pipe output to hosts file */

#endif	/* FSM_CONTROL_H */
