/*
 * ----- vfsops_linux.c
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
#pragma ident "$Revision: 1.69 $"
#endif

#include "sam/osversion.h"

/*
 * ----- UNIX Includes
 */

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/file.h>
#include <linux/uio.h>
#include <linux/dirent.h>
#include <linux/proc_fs.h>
#include <linux/limits.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/pagemap.h>
#include <linux/list.h>
#include <linux/smp_lock.h>
#include <linux/version.h>

#include <asm/param.h>
#include <asm/uaccess.h>
#include <asm/system.h>

/*
 * ----- SAMFS Includes
 */

#include "sam/types.h"
#include "sam/mount.h"

#include "samfs_linux.h"
#include "inode.h"
#include "mount.h"
#include "global.h"
#include "dirent.h"
#include "debug.h"
#include "clextern.h"
#include "trace.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
typedef struct kstatfs qfs_kstatfs_t;
#else
typedef struct statfs qfs_kstatfs_t;
#endif

extern struct proc_dir_entry *proc_fs_samqfs;
int get_qfs_fsid(char *buffer, char **start, off_t offset, int length,
		int *eof, void *data);
extern void sam_dev_cleanup(sam_mount_t *mp);
struct super_operations samqfs_super_ops;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)

extern struct file_system_type samqfs_fs_type;
struct export_operations samqfs_export_ops;

/*
 * ----- samqfs_encode_fh
 * In Linux 2.6 this operation ensures accurate file handle encoding when
 * NFS-serving a file created using QFS.
 *
 */
static int
samqfs_encode_fh(struct dentry *dentry, __u32 *data, int *lenp,
    int connectable)
{
	struct inode *li = dentry->d_inode;
	struct inode *pli;
	int maxlen = *lenp;

	/*
	 * Directories have their parents' info encoded in '..'
	 */
	if (S_ISDIR(li->i_mode)) {
		connectable = 0;
	}

	/*
	 * Do we have enough space to do encoding ?
	 */
	if (maxlen < 2) {
		return (255);
	}

	if (connectable && maxlen < 4) {
		return (255);
	}

	/*
	 * Encode file handle.
	 * Return fhtype.
	 * Valid types are:
	 * 1 : inode no + generation no
	 * 2 : inode no + generation no +
	 *	parent inode no + parent generation no
	 */
	data[0] = li->i_ino;
	data[1] = li->i_generation;

	*lenp = 2;
	if (!connectable) {
		return (1);
	}

	spin_lock(&dentry->d_lock);
	pli = dentry->d_parent->d_inode;
	data[2] = pli->i_ino;
	data[3] = pli->i_generation;
	*lenp = 4;
	spin_unlock(&dentry->d_lock);

	return (2);
}

/*
 * ----- samqfs_decode_fh
 * In Linux 2.6 this operation ensures accurate file handle decoding when
 * NFS-serving a file created using QFS.
 *
 */
static struct dentry *
samqfs_decode_fh(struct super_block *sb, __u32 *fh, int len,
    int fhtype, int (*acceptable)(void *contect, struct dentry *de),
    void *context)
{
	struct dentry *result;
	sam_id_t sid, psid;

	/*
	 * Do we have a stale file handle ?
	 */
	if ((fhtype != 1 && len != 2) && (fhtype != 2 && len != 4)) {
		return (ERR_PTR(-ESTALE));
	}

	/*
	 * Decode information contained in file handle and obtain inode.
	 */
	sid.ino = fh[0];
	sid.gen = fh[1];
	if (fhtype == 2) {
		psid.ino = fh[2];
		psid.gen = fh[3];
	}

	/*
	 * Now to find a well-connected dentry.
	 */
	result = sb->s_export_op->find_exported_dentry(sb, &sid,
	    fhtype == 2 ? &psid : NULL,
	    acceptable, context);
	return (result);
}


/*
 * ----- samqfs_get_dentry
 * In Linux 2.6 this operation obtains the identified inode
 * and returns its corresponding dentry.
 */
static struct dentry *
samqfs_get_dentry(struct super_block *sb, void *vobjp)
{
	__u32 *data = vobjp;
	struct dentry *result;
	sam_node_t *ip;
	sam_id_t sid;
	struct inode *inode;

	sid.ino = data[0];
	sid.gen = data[1];

	if (sam_get_ino(sb, IG_NEW, &sid, &ip)) {
		return (ERR_PTR(-ESTALE));
	}

	/*
	 * Now to find a well-connected dentry.
	 */
	inode = ip->inode;
	result = rfs_d_alloc_anon(inode);
	if (!result) {
		iput(inode);
		return (ERR_PTR(-ENOMEM));
	}

	return (result);
}


/*
 * ----- samqfs_get_parent
 * In Linux 2.6 this operation finds the parent directory
 * for the given child, which is also a directory.
 */
static struct dentry *
samqfs_get_parent(struct dentry *cde)
{
	int error = 0;
	sam_node_t *cip;
	struct dentry *pde = NULL;
	sam_node_t *pip;
	struct inode *pli;
	struct qstr comp;

	cip = SAM_LITOSI(cde->d_inode);

	if ((error = sam_open_operation(cip))) {
		return (ERR_PTR(-error));
	}

	comp.name = "..";
	comp.len = 2;

	error = sam_client_lookup_name(cip, &comp,
	    cip->mp->mt.fi_meta_timeo, 0, &pip, NULL);

	if (error == 0) {
		pli = SAM_SITOLI(pip);

		pde = rfs_d_alloc_anon(pli);

		if (!pde) {
			iput(pli);
			pde = ERR_PTR(-ENOMEM);
		}

	} else {

		pde = ERR_PTR(-error);
	}

	SAM_CLOSE_OPERATION(cip->mp, error);
	return (pde);
}


static int
samqfs_set_super(struct super_block *lsb, void *data)
{
	return (0);
}


static int
samqfs_sync_fs(struct super_block *lsb, int wait)
{
	int error = 0;

	return (error);
}


static int
samqfs_bdev_set(sam_mount_t *mp, int flags)
{
	struct samdent *fs_dent;
	struct block_device *bdev;
	int error = 0;
	int ord;

	mutex_enter(&samgt.global_mutex);
	if (mp != NULL) {
		for (ord = 0; ord < mp->mt.fs_count; ord++) {
			if (mp->mi.m_fs[ord].dt == DD) {
				fs_dent = &mp->mi.m_fs[ord];

				if (mp->mt.fi_status & FS_MOUNTING) {
					/*
					 * Open the block devices during mount.
					 */
					bdev = open_bdev_excl(
					    fs_dent->part.pt_name,
					    flags, &samqfs_fs_type);
					if (IS_ERR(bdev)) {

						error = -PTR_ERR(bdev);
						break;

					} else {

						fs_dent->bdev = bdev;
					}

				} else {
					/*
					 * Close the block devices during
					 * umount.
					 */
					if (fs_dent->bdev) {
						close_bdev_excl(fs_dent->bdev);
						fs_dent->bdev = NULL;
					}
				}
			}
		}
	}
	mutex_exit(&samgt.global_mutex);
	return (error);
}
#endif


/*
 * ----- samqfs_delete_proc_entries -
 * Called by Linux to delete proc entries of a SAM-QFS filesystem
 */
static void
samqfs_delete_proc_entries(sam_mount_t *mp)
{

#ifdef CONFIG_PROC_FS
	if (mp != NULL) {
		if (mp->mi.proc_ent != NULL) {
			struct proc_dir_entry *de;
			struct proc_dir_entry *de_next;

			/*
			 * Remove all nodev devices for "fsname"
			 */
			for (de = mp->mi.proc_ent->subdir; de; de = de_next) {
				de_next = de->next;
				if (strncmp(de->name, "nodev_", 6) == 0) {
					remove_proc_entry(de->name,
					    mp->mi.proc_ent);
				}
			}
			/*
			 * Cleanup /proc/fs/samfs/"fsname"/
			 */
			remove_proc_entry("fsid", mp->mi.proc_ent);
			remove_proc_entry(mp->mt.fi_name, proc_fs_samqfs);
			mp->mi.proc_ent = NULL;
		}
	}
#endif /* CONFIG_PROC_FS */

}


/*
 * ----- samqfs_read_sblk -
 * Called by Linux when mounting a shared QFS file system
 */
static struct super_block *
__samqfs_read_sblk(struct super_block *lsb, int flags, void *rdata)
{
	int i, fs_count;
	int error = 0, err_line = 0;
	sam_id_t id;
	uname_t fsname;
	sam_node_t *rip = NULL;
	generic_mount_info_t *gmip;
	sam_mount_t *mp = NULL;
	sam_mount_info_t *mip = NULL;
	pid_t statfs_pid;

	mutex_enter(&samgt.global_mutex);
	samgt.num_fs_mounting++;
	mutex_exit(&samgt.global_mutex);

	memset((void *)fsname, 0, sizeof (uname_t));

	if (!suser()) {
		err_line = __LINE__;
		error = EPERM;		/* Check for superuser */
		goto done;
	}

	gmip = (generic_mount_info_t *)rdata;

	if (gmip->len != sizeof (sam_mount_info_t)) {
		cmn_err(CE_WARN,
		    "SAM-QFS: samqfs_read_sblk invalid length %d "
		    "expected %lld\n",
		    gmip->len, (long long)sizeof (sam_mount_info_t));
		err_line = __LINE__;
		error = EINVAL;
		goto done;
	}

	mip = (sam_mount_info_t *)kmem_alloc(gmip->len, KM_SLEEP);

	if (mip == NULL) {
		cmn_err(CE_WARN,
		    "SAM-QFS: samqfs_read_sblk kmem_alloc() failed\n");
		err_line = __LINE__;
		error = ENOMEM;
		goto done;
	}

	memset((char *)mip, 0, gmip->len);

	if (copy_from_user((char *)mip, (char *)gmip->data, gmip->len)) {
		cmn_err(CE_WARN,
		    "SAM-QFS: samqfs_read_sblk copy_from_user() failed\n");
		kfree(mip);
		err_line = __LINE__;
		error = EFAULT;
		goto done;
	}

	memcpy(fsname, mip->params.fi_name, sizeof (uname_t));
	cmn_err(CE_NOTE, "SAM-QFS: Initiated mount filesystem: %s", fsname);

	/*
	 * Set the mount table for this mount.
	 */
	fs_count = mip->params.fs_count;
	if (fs_count <= 0) {
		cmn_err(CE_WARN,
		    "SAM-QFS: Mount parameters invalid, fs_count = %x",
		    fs_count);
		err_line = __LINE__;
		error = EINVAL;
		goto done;
	}

	mutex_enter(&samgt.global_mutex);
	for (mp = samgt.mp_list; mp != NULL; mp = mp->ms.m_mp_next) {

		if (strncmp(mp->mt.fi_name,
		    mip->params.fi_name,
		    sizeof (mp->mt.fi_name)) == 0) {

			if (fs_count != mp->mt.fs_count) {
				mutex_exit(&samgt.global_mutex);
				cmn_err(CE_WARN,
				    "SAM-QFS: Mount %s device count %d "
				    "does not match table %d",
				    mip->params.fi_name, fs_count,
				    mp->mt.fs_count);
				mp = NULL;
				err_line = __LINE__;
				error = EINVAL;
				goto done;
			}
			break;
		}
	}
	if (mp == NULL) {
		mutex_exit(&samgt.global_mutex);
		cmn_err(CE_WARN,
		    "SAM-QFS: Cannot find block special device %s",
		    mip->params.fi_name);
		err_line = __LINE__;
		error = EINVAL;
		goto done;
	}

	/*
	 * Is this file system already mounted?  (Or are we trying already?)
	 * Or are we unmounting?
	 */

	mutex_enter(&mp->ms.m_waitwr_mutex);
	if (mp->mt.fi_status & (FS_MOUNTED | FS_MOUNTING |
	    FS_UMOUNT_IN_PROGRESS)) {
		mutex_exit(&mp->ms.m_waitwr_mutex);
		mutex_exit(&samgt.global_mutex);
		mp = NULL;
		err_line = __LINE__;
		error = EBUSY;
		goto done;
	}

	/*
	 * Clear the mount instance mount parameters.
	 */
	mip->params.fi_status = mp->mt.fi_status &
	    (FS_FSSHARED|FS_SRVR_BYTEREV);
	strncpy(mip->params.fi_server, mp->mt.fi_server,
	    sizeof (mip->params.fi_server));
	memset((char *)&mp->mi, 0,
	    (sizeof (sam_mt_instance_t) +
	    (sizeof (struct samdent) * (fs_count-1))));
	mp->mt = mip->params;
	mp->mt.fi_status |= FS_MOUNTING;
	mutex_exit(&mp->ms.m_waitwr_mutex);

	/*
	 * Need to redo this since mi got zero'd
	 */
	sam_setup_threads(mp);

	sam_mutex_init(&mp->mi.m_lease_mutex, NULL, MUTEX_DEFAULT, NULL);
	mp->mi.m_cl_lease_chain.forw = &mp->mi.m_cl_lease_chain;
	mp->mi.m_cl_lease_chain.back = &mp->mi.m_cl_lease_chain;
	mp->mi.m_cl_lease_chain.node = NULL;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	lsb = sget(&samqfs_fs_type, NULL, samqfs_set_super, mp);
	if (IS_ERR(lsb)) {
		mutex_exit(&samgt.global_mutex);
		kfree(mip);
		return (lsb);
	}

	lsb->s_export_op = &samqfs_export_ops;
	lsb->s_flags = flags;
	mp->mi.m_vfsp = lsb;
	lsb->s_fs_info = (void *)mp;
#else
	mp->mi.m_vfsp = lsb;		/* Mounted */
	lsb->u.generic_sbp = (void *)mp;
#endif

	lsb->s_magic = SAM_MAGIC_V2;
	lsb->s_op = &samqfs_super_ops;
	lsb->s_blocksize = SAM_DEV_BSIZE_LINUX;
	lsb->s_blocksize_bits = SAM_DEV_BSHIFT_LINUX;
#ifdef _LP64
	lsb->s_maxbytes = 0x7fffffffffffffff;
#else
	lsb->s_maxbytes = (((u64)SAM_DEV_BSIZE_LINUX << (BITS_PER_LONG-1))-1);
#endif /* _LP64 */

	/*
	 * Check for read-only.
	 */
	if (lsb->s_flags & MS_RDONLY) {
		mp->mt.fi_mflag |= MS_RDONLY;
	}

	mp->mi.m_fsact_buf = NULL;
	mutex_exit(&samgt.global_mutex);

	/*
	 * Get inodes for the block special devices for this filesystem.
	 */

	if ((error = sam_lgetdev(mp, &mip->part[0]))) {
		TRACE(T_SAM_MNT_ERR, lsb, 12, (sam_tr_t)mp, error);
		err_line = __LINE__;
		goto done;
	}

	/*
	 * Read superblock and set up fields, rewrite if not read only.
	 */
	if ((error = sam_mount_fs(mp))) {
		sam_dev_cleanup(mp);
		TRACE(T_SAM_MNT_ERR, lsb, 13, 0, error);
		err_line = __LINE__;
		goto done;
	}

	/*
	 * Close the device opens done by sam_getdev, they
	 * are no longer needed.
	 */
	sam_dev_cleanup(mp);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	error = samqfs_bdev_set(mp, flags);
#endif

	for (i = 0; i < mp->mt.fs_count; i++) {
		if (mp->mi.m_fs[i].dt == DD) {
			lsb->s_dev = mp->mi.m_fs[i].dev;
			break;
		}
	}

	/*
	 * Until linux has a way to set this option
	 * sam_tracing should remain at 1
	 * src/fs/lib/mount.c sets fi_config | MT_TRACE
	 * for DEBUG but this turns it off for non
	 * DEBUG. sam_tracing is initialized to 1 in
	 * init_linux.c
	 *
	 *	sam_tracing = mp->mt.fi_config & MT_TRACE;
	 *
	 */

	/*
	 * Get .inodes, root, & .blocks (hidden) inodes.
	 */

	for (i = SAM_INO_INO; i <= SAM_BLK_INO; i++) {
		id.ino = i;
		id.gen = i;
		error = sam_get_ino(lsb, IG_NEW, &id, &rip);
		if (error) {
			err_line = __LINE__;
			TRACE(T_SAM_MNT_ERRID, lsb, 14, i, error);
			sam_umount_ino(lsb, 0);
			goto done;
		}
		switch (i) {
		case SAM_INO_INO:
			mp->mi.m_inodir = rip;
			mp->mi.m_inodir->flags.bits |= SAM_DIRECTIO;
			break;
		case SAM_ROOT_INO:
			if ((error = sam_verify_root(rip, mp))) {
				err_line = __LINE__;
				goto done;
			}
			mp->mi.m_vn_root = SAM_SITOLI(rip);
			break;
		case SAM_BLK_INO:
			mp->mi.m_inoblk = rip;
			break;
		}
	}

	/*
	 * Get a dentry for the root inode
	 */
	lsb->s_root = d_alloc_root(mp->mi.m_vn_root);

	if (!lsb->s_root) {
		err_line = __LINE__;
		error = ENOENT;
		goto done;
	}

	lsb->s_root->d_op = &samqfs_root_dentry_ops;

	/*
	 * Notify sam-fsd of successful mount.
	 */
	{
		struct sam_fsd_cmd cmd;

		cmd.cmd = FSD_mount;
		memcpy(cmd.args.mount.fs_name, mp->mt.fi_name,
		    sizeof (cmd.args.mount.fs_name));
		cmd.args.mount.init = mp->mi.m_sbp->info.sb.init;
		(void) sam_send_scd_cmd(SCD_fsd, &cmd, sizeof (cmd));
	}
	error = 0;

done:
	mutex_enter(&samgt.global_mutex);
	if (mip != NULL) {
		kfree(mip);
		mip = NULL;
	}
	if (error == 0) {
		mutex_enter(&mp->ms.m_waitwr_mutex);
		mp->mt.fi_status |= FS_MOUNTED;
		samgt.num_fs_mounted++;
		mp->mt.fi_status &= ~FS_MOUNTING;
		mutex_exit(&mp->ms.m_waitwr_mutex);
		sam_send_shared_mount(mp);
		cmn_err(CE_NOTE,
		    "SAM-QFS: Completed mount filesystem: %s", fsname);

		/*
		 * Start sam_statfs_thread
		 */
		mutex_enter(&mp->mi.m_statfs.mutex);
		statfs_pid = sam_init_thread(&mp->mi.m_statfs,
		    sam_statfs_thread, (void *)mp);
		mutex_exit(&mp->mi.m_statfs.mutex);

#ifdef CONFIG_PROC_FS
		if (lsb) {
			/*
			 * Create /proc/fs/samfs/"fsname"/
			 */
			if (mp->mi.proc_ent == NULL) {
				mp->mi.proc_ent = proc_mkdir(mp->mt.fi_name,
				    proc_fs_samqfs);
			}

			if (mp->mi.proc_ent) {
				/*
				 * Add entries into /proc/fs/samfs/"fsname"/
				 */
				(void) create_proc_read_entry("fsid",
				    0, mp->mi.proc_ent, get_qfs_fsid, lsb);
			}
		}
#endif /* CONFIG_PROC_FS */

	} else {
		cmn_err(CE_NOTE,
		    "SAM-QFS: Aborted mount filesystem: %s", fsname);
		if ((mp == NULL) && (error == EBUSY)) {
			/* mount attempt on something already mounted */
			mutex_exit(&samgt.global_mutex);
			return (ERR_PTR(-error));
		}
		/*
		 * Clean it all up
		 */
		samqfs_delete_proc_entries(mp);
		sam_umount_ino(lsb, 0);
		sam_cleanup_mount(mp, NULL, NULL);
		if (mp) {
			mutex_enter(&mp->ms.m_waitwr_mutex);
			mp->mt.fi_status &= ~(FS_MOUNTED|FS_MOUNTING);
			mutex_exit(&mp->ms.m_waitwr_mutex);
			*mp->mt.fi_mnt_point = '\0';
			sam_send_shared_mount(mp);
		}
		TRACE(T_SAM_MNT_ERRLN, NULL, err_line, error, 0);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
		if (mp) {
			mutex_exit(&samgt.global_mutex);
			(void) samqfs_bdev_set(mp, 0);
			mutex_enter(&samgt.global_mutex);
		}
		if (lsb) {
			rfs_up_write(&lsb->s_umount);
			deactivate_super(lsb);
		}
#endif
		lsb = ERR_PTR(-error);
	}
	mutex_exit(&samgt.global_mutex);
	return (lsb);
}


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
struct super_block *
samqfs_read_sblk(struct super_block *sb, void *rdata, int silent)
{
	struct super_block *lsb;

	lsb = __samqfs_read_sblk(sb, silent, rdata);
	if (IS_ERR(lsb)) {
		return (NULL);
	}
	rfs_try_module_get(THIS_MODULE);
	return (lsb);
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)) && \
	(LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18))
struct super_block *
samqfs_read_sblk(struct file_system_type *fst, int flags, const char *devname,
    void *rdata)
{
	struct super_block *lsb;

	ASSERT(fst == &samqfs_fs_type);
	lsb = __samqfs_read_sblk(NULL, flags, rdata);
	if (IS_ERR(lsb)) {
		goto out_err;
	}
	lsb->s_flags |= MS_ACTIVE;
	rfs_try_module_get(THIS_MODULE);
out_err:
	return (lsb);
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18))
int
samqfs_read_sblk(struct file_system_type *fst, int flags, const char *devname,
    void *rdata, struct vfsmount *mnt)
{
	struct super_block *lsb;

	ASSERT(fst == &samqfs_fs_type);
	lsb = __samqfs_read_sblk(NULL, flags, rdata);
	if (IS_ERR(lsb)) {
		return (PTR_ERR(lsb));
	}
	lsb->s_flags |= MS_ACTIVE;
	rfs_try_module_get(THIS_MODULE);
	return (simple_set_mnt(mnt, lsb));
}
#endif


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
void
samqfs_kill_sblk(struct super_block *lsb)
{
	generic_shutdown_super(lsb);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
/*
 * ----- samqfs_read_inode -
 * In Linux 2.4 the NFS exportfs operation checks to see if it's here
 * but never uses it.
 */
static void
samqfs_read_inode(struct inode *lip)
{
	ASSERT(0);
}


/*
 * ----- samqfs_fh_to_dentry
 * In Linux 2.4 this operation ensures accurate file handle decoding when
 * NFS-serving a file created using QFS.
 *
 */
static struct dentry *
samqfs_fh_to_dentry(struct super_block *sb, __u32 *fh, int len, int fhtype,
    int parent)
{
	sam_node_t *ip;
	sam_id_t sid;
	struct inode *inode = NULL;
	struct dentry *result;
	int error;

	/*
	 * Do we have a stale file handle ?
	 */
	if (fhtype != 1 && (parent && fhtype != 2)) {
		return (ERR_PTR(-ESTALE));
	}

	if (len != 2 && (parent && len != 4)) {
		return (ERR_PTR(-ESTALE));
	}

	/*
	 * Decode information contained in file handle and obtain inode.
	 */
	if (parent) {
		sid.ino = fh[2];
		sid.gen = fh[3];
	} else {
		sid.ino = fh[0];
		sid.gen = fh[1];
	}

	if (sam_get_ino(sb, IG_NEW, &sid, &ip)) {
		return (ERR_PTR(-ESTALE));
	}

	/*
	 * Now to find a well-connected dentry.
	 */
	inode = ip->inode;
	result = rfs_d_alloc_anon(inode);
	return (result);
}


/*
 * ----- samqfs_dentry_to_fh
 * In Linux 2.4 this operation ensures accurate file handle encoding when
 * NFS-serving a file created using QFS.
 *
 */
static int
samqfs_dentry_to_fh(struct dentry *dentry, __u32 *fh, int *lenp,
    int need_parent)
{
	struct inode *li = dentry->d_inode;
	struct inode *pli;
	int maxlen = *lenp;

	if (maxlen < 2) {
		return (255);
	}

	/*
	 * Encode file handle.
	 * Return fhtype.
	 * Valid types are:
	 * 1 : 	inode no + generation no
	 * 2 : 	inode no + generation no +
	 *	parent inode no + parent generation no
	 */
	fh[0] = li->i_ino;
	fh[1] = li->i_generation;

	*lenp = 2;
	if (!need_parent) {
		return (1);
	}

	if (need_parent && maxlen < 4) {
		return (255);
	}

	pli = dentry->d_parent->d_inode;
	fh[2] = pli->i_ino;
	fh[3] = pli->i_generation;

	*lenp = 4;

	return (2);
}

#endif /* <2.6.0 */


/*
 * ----- samqfs_write_inode -
 * Called by Linux to write a SAM-QFS inode
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 9) /* RHAS4, but not SLES9 */
int
samqfs_write_inode(struct inode *li, int sync)
{
	return (0);
}
#else
void
samqfs_write_inode(struct inode *li, int sync)
{
}
#endif

/*
 * ----- samqfs_put_inode -
 * Called by Linux during iput().
 */
void
samqfs_put_inode(struct inode *li)
{
}


/*
 * ----- samqfs_delete_inode -
 * Called by Linux when a file or directory is removed
 */
void
samqfs_delete_inode(struct inode *li)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
	truncate_inode_pages(li->i_mapping, 0);
#endif
	clear_inode(li);
}


/*
 * ----- samqfs_put_sblk -
 * Called called by Linux when umounting a SAM-QFS filesystem.
 */
void
samqfs_put_sblk(struct super_block *lsb)
{
	sam_mount_t *mp = NULL;
	struct sam_fsd_cmd cmd;
	uname_t fsname;
	int version;
	int fflag = 0;
	sam_nchain_t *chain;

#if 0
	TRACE(T_SAM_UMOUNT, lsb, (sam_tr_t)mp, (sam_tr_t)mp->mi.m_vn_root,
	    atomic_read(&mp->mi.m_vn_root->i_count));
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	mp = (sam_mount_t *)lsb->s_fs_info;
#else
	mp = (sam_mount_t *)lsb->u.generic_sbp;
#endif

	mutex_enter(&mp->ms.m_waitwr_mutex);

	/*
	 * This gets set when iput is called for the root inode
	 */
	if (!(mp->mt.fi_status & FS_UMOUNT_IN_PROGRESS)) {
		BUG();
	}

	if ((!(mp->mt.fi_status & FS_MOUNTED)) ||
	    (mp->mt.fi_status & FS_MOUNTING)) {
		BUG();
	}

	TRACE(T_SAM_UMNT_RET, lsb, 1, (sam_tr_t)mp, 0);
	version = mp->mt.fi_version;
	memset((void *)fsname, 0, sizeof (uname_t));
	memcpy(fsname, mp->mt.fi_name, sizeof (uname_t));
	cmn_err(CE_NOTE, "SAM-QFS: Initiated unmount filesystem: %s, vers %d",
	    fsname, version);

	mutex_exit(&mp->ms.m_waitwr_mutex);

	TRACE(T_SAM_UMNT_RET, lsb, 2, (sam_tr_t)mp, 0);
	if (sam_unmount_fs(mp, fflag, SAM_UNMOUNT_FS)) {
		BUG();
	}

	/*
	 * Notify sam-fsd of successful umount.
	 */
	cmd.cmd = FSD_umount;
	bcopy(mp->mt.fi_name, cmd.args.umount.fs_name,
	    sizeof (cmd.args.umount.fs_name));
	cmd.args.umount.umounted = 1;
	(void) sam_send_scd_cmd(SCD_fsd, &cmd, sizeof (cmd));

	/*
	 * Kill sam_statfs_thread
	 */
	sam_kill_thread(&mp->mi.m_statfs);


	/*
	 * Close our backing store devices.
	 */
	sam_cleanup_mount(mp, NULL, NULL);

	mutex_enter(&samgt.global_mutex);

	*mp->mt.fi_mnt_point = '\0';

	mp->orig_mt.fi_status = mp->mt.fi_status &
	    (FS_FSSHARED | FS_SRVR_BYTEREV);
	strncpy(mp->orig_mt.fi_server, mp->mt.fi_server,
	    sizeof (mp->orig_mt.fi_server));

	/*
	 * Restore original parameter settings
	 */
	mp->mt = mp->orig_mt;

	mutex_enter(&mp->ms.m_waitwr_mutex);
	mp->mt.fi_status &= ~(FS_MOUNTED | FS_UMOUNT_IN_PROGRESS);
	mutex_exit(&mp->ms.m_waitwr_mutex);

	samgt.num_fs_mounted--;

	mutex_exit(&samgt.global_mutex);

	/* for forced umount probably just want to call sam_send_shared_mount */
	sam_send_shared_mount_blocking(mp);

	/*
	 * Unhash last 3 inodes and inactivate them.
	 */

	if (sam_umount_ino(lsb, 0)) {
		BUG();
	}

	/*
	 * No more TRACEs allowed after sam_umount_ino().
	 */

	cmn_err(CE_NOTE, "SAM-QFS: Completed unmount filesystem: %s, vers %d",
	    fsname, version);

	/* clean up any entries in /proc/fs/samfs/"fsname" */
	samqfs_delete_proc_entries(mp);

	/* clean up any remaining lease, inode references */
	mutex_enter(&mp->mi.m_lease_mutex);
	chain = mp->mi.m_cl_lease_chain.forw;

	while (chain != &mp->mi.m_cl_lease_chain) {
		sam_node_t *ip;

		ip = chain->node;
		chain = chain->forw;

		mutex_enter(&ip->ilease_mutex);
		ip->cl_leases = 0;
		sam_client_remove_lease_chain(ip);

		mutex_exit(&ip->ilease_mutex);
		VN_RELE_OS(ip);
	}
	mutex_exit(&mp->mi.m_lease_mutex);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	(void) samqfs_bdev_set(mp, 0);
#endif
	rfs_module_put(THIS_MODULE);
}


/*
 * ----- samqfs_clear_inode -
 * Called By Linux when Linux is deleting a Linux inode
 */
void
samqfs_clear_inode(struct inode *li)
{
	sam_node_t *ip;

	ip = SAM_LITOSI(li);

	if (ip == NULL) {
		/*
		 * Cleaning up an inode that was never
		 * fully initialized
		 */
		return;
	}

	sam_iecache_clear(ip);

	li->LI_I_PRIVATE = NULL;
	ip->inode = NULL;

	kmem_cache_free(samgt.inode_cache, (void *)ip);

	mutex_enter(&samgt.ifreelock);
	samgt.inocount--;
	mutex_exit(&samgt.ifreelock);
}


/*
 * ----- samqfs_statfs - Return file system status.
 * Return sblk status for this instance of SAM-QFS mount.
 */
static int
__samqfs_statfs(struct super_block *lsb, qfs_kstatfs_t *sp, sam_mount_t *mp)
{
	struct sam_sblk *sblk = NULL;
	int error;

	TRACE(T_SAM_STATVFS, lsb, (sam_tr_t)MAXNAMLEN, 0, 0);

	error = sam_proc_block_vfsstat(mp, SHARE_wait_one);
	if (error) {
		return (-error);
	}

	sblk = mp->mi.m_sbp;
	if (sblk->info.sb.magic != SAM_MAGIC_V2 &&
	    sblk->info.sb.magic != SAM_MAGIC_V2A) {
		cmn_err(CE_WARN, "SAM-FS: Superblock magic invalid <%x>",
		    sblk->info.sb.magic);
		return (-EINVAL);
	}
	memset((void *)sp, 0, sizeof (struct statfs));

	sp->f_type = sblk->info.sb.magic;

	sp->f_bsize = SAM_DEV_BSIZE;		/* file system block size */
	sp->f_blocks = sblk->info.sb.capacity;	/* total # of blocks */
	sp->f_bfree = sblk->info.sb.space;	/* total # of free blocks */
	sp->f_bavail = sblk->info.sb.space; /* blocks avail to non-superuser */
	sp->f_files = -1;		/* total # of file nodes (inodes) */
	sp->f_ffree = -1;		/* total # of free file nodes */
	if (sblk->info.sb.mm_count) {
		offset_t files;
		files = (sblk->info.sb.mm_capacity <<
		    SAM_DEV_BSHIFT) >> SAM_ISHIFT;
		/* total # of file nodes (inodes) */
		sp->f_files = (uint_t)files;
		files = (sblk->info.sb.mm_space <<
		    SAM_DEV_BSHIFT) >> SAM_ISHIFT;
		sp->f_ffree = (uint_t)files;
	}

	/*
	 * maximum file name length
	 */
	sp->f_namelen = MAXNAMLEN;

	return (0);
}


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
static int
samqfs_statfs(struct super_block *lsb, qfs_kstatfs_t *sp)
{
	sam_mount_t *mp;

	mp = (sam_mount_t *)lsb->u.generic_sbp;
	return (__samqfs_statfs(lsb, sp, mp));
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)) && \
	(LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18))
static int
samqfs_statfs(struct super_block *lsb, qfs_kstatfs_t *sp)
{
	sam_mount_t *mp;

	mp = (sam_mount_t *)lsb->s_fs_info;
	return (__samqfs_statfs(lsb, sp, mp));
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18))
static int
samqfs_statfs(struct dentry *dentry, qfs_kstatfs_t *sp)
{
	sam_mount_t *mp;
	struct super_block *lsb = dentry->d_sb;

	mp = (sam_mount_t *)lsb->s_fs_info;
	return (__samqfs_statfs(lsb, sp, mp));
}
#endif


/*
 * ----- get_qfs_fsid - Called from /proc/fs/samfs/"fsname"/fsid.
 * Returns the fsid needed for the NFS exportfs command.
 */
int
get_qfs_fsid(char *buffer, char **start, off_t offset, int length, int *eof,
    void *data)
{
	struct super_block *lsbp;
	int len = 0;

	lsbp = (struct super_block *)data;

	len = sprintf(buffer+len, "fsid=0x%x\n", lsbp->s_dev);
	*eof = 1;

	return (len);
}


/*
 * Operations for the Linux superblock
 */
struct super_operations samqfs_super_ops = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
	read_inode:		samqfs_read_inode,
	fh_to_dentry:	samqfs_fh_to_dentry,
	dentry_to_fh:	samqfs_dentry_to_fh,
#endif
	write_inode:	samqfs_write_inode,
	put_inode:		samqfs_put_inode,
	delete_inode:	samqfs_delete_inode,
	put_super:		samqfs_put_sblk,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	sync_fs:		samqfs_sync_fs,
#endif
	statfs:			samqfs_statfs,
	clear_inode:	samqfs_clear_inode,
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
struct export_operations samqfs_export_ops = {
	encode_fh  : samqfs_encode_fh,
	decode_fh  : samqfs_decode_fh,
	get_parent : samqfs_get_parent,
	get_dentry : samqfs_get_dentry
};
#endif
