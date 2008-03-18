/*
 *  samsizes.c  - Print struct sizes.
 *
 *	Samsizes prints the size of structs used by SAM-FS.
 *
 *	Syntax:
 *		samsizes
 *
 */

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

#pragma ident "$Revision: 1.28 $"


#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#ifdef sun
#include "aml/scan.h"
#endif /* sun */
#include "sam/resource.h"
#include "sam/attributes.h"
#include "aml/fifo.h"
#include "sam/fs/ino.h"
#include "sam/fs/ino_ext.h"
#include "sam/fs/inode.h"
#include "sam/fs/dirent.h"
#include "sam/fs/sblk.h"
#include "sam/fs/amld.h"
#include "sam/fs/share.h"
#include "sam/fs/trace.h"
#include "pub/rminfo.h"
#include "pub/stat.h"
#include "../src/fs/cmd/dump-restore/csd.h"
#include "../src/fs/cmd/dump-restore/old_resource.h"

char		*program_name = "samsizes";		/* Used by error() */
char		*arch;

int
main()
{
	int	i;
	char	hostname[MAXHOSTNAMELEN + 1];	/* This machine's hostname */

#if defined(__sparcv9)
	arch = "sparcv9";
#elif defined(__sparc)
	arch = "sparc";
#elif defined(__amd64)
	arch = "amd64";
#elif defined(x86_64)
	arch = "x86_64";
#elif defined(ia64)
	arch = "ia64";
#elif defined(__i386)
	arch = "i386";
#else
	arch = "unknown";
#endif

	gethostname(hostname, MAXHOSTNAMELEN);
	printf("\nsamsizes\n========\n");
	printf("hostname:     %s\n", hostname);
	printf("architecture: %s\n\n", arch);
	printf("structure\t\t\t\t HEX \t  DEC \n");
	printf("=========\t\t\t\t=====\t =====\n");

	i = sizeof (ino_t);
	printf("ino_t:\t\t\t\t\t%5x\t(%5d)\n",				i, i);
#ifdef sun
	i = sizeof (krwlock_t);
	printf("rwlock_t:\t\t\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (kmutex_t);
	printf("kmutex_t:\t\t\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (kcondvar_t);
	printf("kcondvar_t:\t\t\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (lloff_t);
	printf("lloff_t:\t\t\t\t%5x\t(%5d)\n",				i, i);
	i = sizeof (mutex_t);
	printf("mutex_t:\t\t\t\t%5x\t(%5d)\n",				i, i);
	i = sizeof (diskaddr_t);
	printf("diskaddr_t:\t\t\t\t%5x\t(%5d)\n",			i, i);
#endif /* sun */
	i = sizeof (mode_t);
	printf("mode_t:\t\t\t\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (uid_t);
	printf("uid_t:\t\t\t\t\t%5x\t(%5d)\n",				i, i);
	i = sizeof (gid_t);
	printf("gid_t:\t\t\t\t\t%5x\t(%5d)\n",				i, i);
	i = sizeof (time_t);
	printf("time_t:\t\t\t\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (daddr_t);
	printf("daddr_t:\t\t\t\t%5x\t(%5d)\n",				i, i);

/* ----	stat.h								*/
	i = sizeof (struct sam_stat);
	printf("struct sam_stat:\t\t\t%5x\t(%5d)\n",			i, i);


/* ----	sam_types.h							*/
	i = sizeof (uname_t);
	printf("uname_t:\t\t\t\t%5x\t(%5d)\n",				i, i);
	i = sizeof (media_t);
	printf("media_t:\t\t\t\t%5x\t(%5d)\n",				i, i);
	i = sizeof (vsn_t);
	printf("vsn_t:\t\t\t\t\t%5x\t(%5d)\n",				i, i);
	i = sizeof (offset_t);
	printf("offset_t:\t\t\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (sam_id_t);
	printf("sam_id_t:\t\t\t\t%5x\t(%5d)\n",			i, i);


/* ----	sam_device.h							*/
	i = sizeof (dstate_t);
	printf("dstate_t:\t\t\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (dtype_t);
	printf("dtype_t:\t\t\t\t%5x\t(%5d)\n",				i, i);
	i = sizeof (equ_t);
	printf("equ_t:\t\t\t\t\t%5x\t(%5d)\n",				i, i);
	i = sizeof (dev_ent_t);
	printf("dev_ent_t:\t\t\t\t%5x\t(%5d)\n",			i, i);


/* ----	sam_resource.h							*/
	i = sizeof (sam_vsn_section_t);
	printf("sam_vsn_section_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_archive_t);
	printf("sam_archive_t:\t\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (sam_arch_rminfo_t);
	printf("sam_arch_rminfo_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (struct sam_rminfo);
	printf("struct sam_rminfo:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_resource_t);
	printf("sam_resource_t:\t\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_resource_file_t);
	printf("sam_resource_file_t:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_vsn_section_t);
	printf("sam_vsn_section_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_stage_request_t);
	printf("sam_stage_request_t:\t%5x\t(%5d)\n",		i, i);


/* ----	sam_attributes.h						*/
	i = sizeof (sam_file_attr_t);
	printf("sam_file_attr_t:\t\t%5x\t(%5d)\n",			i, i);

/* ----	sam_fifo.h							*/
	i = sizeof (sam_fs_fifo_t);
	printf("sam_fs_fifo_t:\t\t\t%5x\t(%5d)\n",			i, i);

/* ----	fs/sam_ino.h							*/
	i = sizeof (sam_node_t);
	printf("sam_node_t:\t\t\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (sam_indirect_extent_t);
	printf("sam_indirect_extent_t:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_perm_inode_t);
	printf("sam_perm_inode_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (union sam_di_ino);
	printf("union sam_di_ino:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_disk_inode_t);
	printf("sam_disk_inode_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_arch_inode_t);
	printf("sam_arch_inode_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_archive_info_t);
	printf("sam_archive_info_t:\t\t%5x\t(%5d)\n",		i, i);

/* ----	fs/ino_ext.h							*/
	i = sizeof (sam_inode_ext_hdr_t);
	printf("sam_inode_ext_hdr_t:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (struct sam_mv1_inode);
	printf("struct sam_mv1_inode:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (struct sam_mva_inode);
	printf("struct sam_mva_inode:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (struct sam_sln_inode);
	printf("struct sam_sln_inode:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (struct sam_rfa_inode);
	printf("struct sam_rfa_inode:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (struct sam_rfv_inode);
	printf("struct sam_rfv_inode:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (struct sam_hlp_inode);
	printf("struct sam_hlp_inode:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (struct sam_acl_inode);
	printf("struct sam_acl_inode:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (struct sam_inode_ext);
	printf("struct sam_inode_ext:\t%5x\t(%5d)\n",		i, i);


/* ----	fs/sam_dirent.h							*/
	i = sizeof (sam_dirent_t);
	printf("sam_dirent_t:\t\t\t%5x\t(%5d)\n",			i, i);


/* ----	fs/sam_sblk.h							*/
	i = sizeof (sam_sbinfo_t);
	printf("sam_sbinfo_t:\t\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (sam_sbord_t);
	printf("sam_sbord_t:\t\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (sam_sblk_t);
	printf("sam_sblk_t:\t\t\t\t%5x\t(%5d)\n",			i, i);


/* ----	fs/amld.h							*/
	i = sizeof (samamld_cmd_queue_t);
	printf("samamld_cmd_queue_t:\t%5x\t(%5d)\n",		i, i);

/* ----	fs/share.h							*/
	i = sizeof (sam_cred_t);
	printf("sam_cred_t:\t\t\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (sam_vattr_t);
	printf("sam_vattr_t:\t\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (sam_share_flock_t);
	printf("sam_share_flock_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_cl_attr_t);
	printf("sam_cl_attr_t:\t\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (sam_sr_attr_t);
	printf("sam_sr_attr_t:\t\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (sam_ino_instance_t);
	printf("sam_ino_instance_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_ino_record_t);
	printf("sam_ino_record_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_san_mount_t);
	printf("sam_san_mount_t:\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (sam_shvfm_flags_t);
	printf("sam_shvfm_flags_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_lease_data_t);
	printf("sam_lease_data_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_san_lease_t);
	printf("sam_san_lease_t:\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (sam_san_lease2_t);
	printf("sam_san_lease2_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_name_create_t);
	printf("sam_name_create_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_name_remove_t);
	printf("sam_name_remove_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_name_link_t);
	printf("sam_name_link_t:\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (sam_name_rename_t);
	printf("sam_name_rename_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_name_mkdir_t);
	printf("sam_name_mkdir_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_name_rmdir_t);
	printf("sam_name_rmdir_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_name_symlink_t);
	printf("sam_name_symlink_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_name_acl_t);
	printf("sam_name_acl_t:\t\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_name_arg_t);
	printf("sam_name_arg_t:\t\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_san_name_t);
	printf("sam_san_name_t:\t\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_san_name2_t);
	printf("sam_san_name2_t:\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (sam_inode_setattr_t);
	printf("sam_inode_setattr_t:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_inode_quota_t);
	printf("sam_inode_quota_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_inode_stage_t);
	printf("sam_inode_stage_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_inode_samattr_t);
	printf("sam_inode_samattr_t:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_inode_samarch_t);
	printf("sam_inode_samarch_t:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_inode_samaid_t);
	printf("sam_inode_samaid_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_inode_arg_t);
	printf("sam_inode_arg_t:\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (sam_san_inode_t);
	printf("sam_san_inode_t:\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (sam_san_inode2_t);
	printf("sam_san_inode2_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_block_getbuf_t);
	printf("sam_block_getbuf_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_block_fgetbuf_t);
	printf("sam_block_fgetbuf_t:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_block_getino_t);
	printf("sam_block_getino_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_block_sblk_t);
	printf("sam_block_sblk_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_block_vfsstat_t);
	printf("sam_block_vfsstat_t:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_block_vfsstat_v2_t);
	printf("sam_block_vfsstat_v2_t:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_block_quota_t);
	printf("sam_block_quota_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_block_arg_t);
	printf("sam_block_arg_t:\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (sam_san_block_t);
	printf("sam_san_block_t:\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (sam_san_wait_t);
	printf("sam_san_wait_t:\t\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_callout_stage_t);
	printf("sam_callout_stage_t:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_callout_relinquish_lease_t);
	printf("sam_callout_relq_les_t:\t%5x\t(%5d)\n",	i, i);
	i = sizeof (sam_callout_arg_t);
	printf("sam_callout_arg_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_san_callout_t);
	printf("sam_san_callout_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_notify_dnlc_t);
	printf("sam_notify_dnlc_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_notify_lease_t);
	printf("sam_notify_lease_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_notify_arg_t);
	printf("sam_notify_arg_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_san_notify_t);
	printf("sam_san_notify_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_san_header_t);
	printf("sam_san_header_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_san_mount_msg_t);
	printf("sam_san_mount_msg_t:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_san_lease_msg_t);
	printf("sam_san_lease_msg_t:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_san_name_msg_t);
	printf("sam_san_name_msg_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_san_inode_msg_t);
	printf("sam_san_inode_msg_t:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_san_block_msg_t);
	printf("sam_san_block_msg_t:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_san_wait_msg_t);
	printf("sam_san_wait_msg_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_san_callout_msg_t);
	printf("sam_san_callout_msg_t:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_san_notify_msg_t);
	printf("sam_san_notify_msg_t:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_san_message_t);
	printf("sam_san_message_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_san_max_message_t);
	printf("sam_san_max_message_t:\t%5x\t(%5d)\n",		i, i);

/* ---- fs/trace.h							*/
	i = sizeof (sam_trace_ent_t);
	printf("sam_trace_ent_t:\t\t%5x\t(%5d)\n",			i, i);

/* ---- ../src/utility/csd/csd.h		*/
	i = sizeof (csd_header_t);
	printf("csd_header_t:\t\t\t%5x\t(%5d)\n",			i, i);
	i = sizeof (csd_header_extended_t);
	printf("csd_header_extended_t:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (csd_filehdr_t);
	printf("csd_filehdr_t:\t\t\t%5x\t(%5d)\n", 		i, i);

/* ---- ../src/utility/csd/old_resource.h	*/
	i = sizeof (sam_old_archive_t);
	printf("sam_old_archive_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_old_resource_t);
	printf("sam_old_resource_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_old_resource_file_t);
	printf("sam_old_resource_file_t:%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_old_section_t);
	printf("sam_old_section_t:\t\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_old_arch_rminfo_t);
	printf("sam_old_arch_rminfo_t:\t%5x\t(%5d)\n",		i, i);
	i = sizeof (sam_old_rminfo_t);
	printf("sam_old_rminfo_t:\t\t%5x\t(%5d)\n",		i, i);

	exit(0);
}
