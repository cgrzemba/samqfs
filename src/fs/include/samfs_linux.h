/*
 *	samfs_linux.h - file system prototypes for the
 *	SAM file system on Linux.
 *
 *	Defines the functions prototypes for the vfs and vnode
 *	operation of the SAMFS filesystem.
 *
 */

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

#ifdef sun
#pragma ident "$Revision: 1.20 $"
#endif


#ifndef	_SAMFS_LINUX_H
#define	_SAMFS_LINUX_H

#ifdef	__KERNEL__
#include "linux/types.h"
#else
#include <sys/types.h>
#endif	/* __KERNEL__ */


/*
 * ----- samfs file system type.
 */

extern	int	samfs_fstype;


/*
 * ----- samfs fsid.
 */

#define	samfs_fsid	"SAM-QFS: Storage Archiving Mgmt"

struct super_block *samfs_read_sblk(struct super_block *sb, void *rdata, int s);


/*
 * ----- SAM-QFS Linux superblock function prototypes.
 */
#if (KERNEL_MAJOR > 4)
extern struct inode *samqfs_alloc_inode(struct super_block *sb);
extern void samqfs_destroy_inode(struct inode *li);
#endif
extern void samqfs_put_sblk(struct super_block *sblk);

#if (KERNEL_MAJOR > 4)
extern int samqfs_access_vn(struct inode *li, int mask, struct nameidata *ndp);
extern int samqfs_client_getattr(struct vfsmount *mnt, struct dentry *de,
				struct kstat *stat);
#else
extern int samqfs_access_vn(struct inode *li, int mask);
extern int samqfs_client_revalidate(struct dentry *de);
#endif
extern int  samqfs_show_options(struct seq_file *sqf, struct vfsmount *vfsmnt);

extern int samqfs_client_open_vn(struct inode *inode, struct file *filp);
extern int samqfs_client_close_vn(struct inode *li, struct file *fp);
extern int samqfs_client_setattr_vn(struct dentry *dep, struct iattr *ap);
extern int samqfs_client_frlock_vn(struct file *fp, int cmd,
		struct file_lock *fl);
extern int sam_readdir_vn(struct inode *li, cred_t *credp,
		filldir_t filldir, void *d, struct file *fp, int *ret_count);
extern int sam_readlink_vn(struct inode *li, uio_t *uiop, cred_t *credp);


extern struct file_operations samqfs_file_ops;
extern struct inode_operations samqfs_file_inode_ops;
extern struct address_space_operations samqfs_file_aops;

extern struct file_operations samqfs_dir_ops;
extern struct inode_operations samqfs_dir_inode_ops;

extern struct inode_operations samqfs_symlink_inode_ops;

extern struct dentry_operations samqfs_dentry_ops;
extern struct dentry_operations samqfs_root_dentry_ops;

extern ssize_t sam_read_vn(struct file *fp, char *buf, ssize_t size,
			loff_t *offset);
extern int sam_write_vn(struct file *fp, char *buf, size_t count, loff_t *ppos);

extern long cv_wait_sig(kcondvar_t *wq, kmutex_t *wl);

#endif	/* _SAMFS_LINUX_H */
