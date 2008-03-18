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

#ifndef _CFG_VFSTAB_H
#define	_CFG_VFSTAB_H

#pragma ident   "$Revision: 1.14 $"

/*
 * vfstab.h
 * Used to read and update the vfstab file.
 * filesystem.h must be included prior to the inclusion of this file.
 */
#include <sys/types.h>
#include "pub/mgmt/filesystem.h"

#define	SAMFS_TYPE	"samfs"
#define	UFS_TYPE	"ufs"
// mount options:
#define	MO_SHARED	"shared"
#define	MO_RO		"ro"
#define	MO_NOSUID	"nosuid"
#define	MO_SEP		","

/*
 * It defines the vfstab file each line's
 * component.
 */
typedef struct vfstab_entry {
	char 		*fs_name;	/* vfs_special */
	char 		*device_to_fsck;
	char 		*mount_point;
	char 		*fs_type;
	int		fsck_pass;
	boolean_t	mount_at_boot;
	char 		*mount_options;
} vfstab_entry_t;

/*
 * update an entry in vfstab.
 * If entry does not exist samerrno will be set to SE_NOT_FOUND.
 */
int update_vfstab_entry(vfstab_entry_t	*entry);

/*
 * add an entry in vfstab.
 * Will fail if an entry with the same fs_name already exists.
 */
int add_vfstab_entry(vfstab_entry_t *entry);

/*
 * delete a single entry at vfstab
 */
int delete_vfstab_entry(char *fs_name);

/*
 * return the entry for the named filesystem if it exists.
 * the return value is allocated dynamically and must be deallocated.
 *
 * return -1 and set samerrno to SE_NOT_FOUND
 */
int get_vfstab_entry(char *fs_name, vfstab_entry_t **ret_val);


/*
 * Function to add vfstab mount options from a vfstab_entry_t
 * structure to a mount_options_t struct. For any mount option found
 * in the vfstab file this function sets the value in the input
 * mount_options struct. If the value is set before coming into this
 * function it will be overwritten. This behavior is to allow this
 * function to be called after the samfs.cmd options have already been
 * set in the structure and have the vfstab options override them.
 */
int
add_mnt_opts_from_vfsent(vfstab_entry_t *vfs_ent, mount_options_t *opts);

/*
 * set_vfstab_options.
 *
 * This function does NOT set all of the options. Instead it favors using
 * the samfs.cmd file.
 *
 * If something has a value in the vfstab it will have its value reset
 * in the vfstab. If a value is being reset to defaults there will be no value
 * in the vfstab after the set_vfstab_options is called. Any value that does
 * not have an entry in the vfstab prior to this call will not after it with
 * the exception of any field that MUST have its value set in the vfstab. This
 * includes shared.
 */
int
set_vfstab_options(char *fs_name, mount_options_t *input);

void
free_vfstab_entry(vfstab_entry_t *vfstab_entry_p);


/*
 * translate and set error codes from sys/vfstab.h into samerrno
 */
void
set_samerrno_for_vfstab_error(char *fs_name, int vfstaberr);


/*
 * get a list containing a vfstab_entry_t for each file system in the
 * vfstab file
 */
int
get_all_vfstab_entries(sqm_lst_t **l);

#endif /* _CFG_VFSTAB_H */
