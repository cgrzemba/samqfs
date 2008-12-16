/*
 * ----- iget_linux.c - Process the iget/iput functions.
 *
 *	Processes the SAM-QFS inode get and inode put functions.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#ifdef sun
#pragma ident "$Revision: 1.55 $"
#endif

#include "sam/osversion.h"

#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/version.h>

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/file.h>
#include <linux/uio.h>
#include <linux/dirent.h>
#include <linux/proc_fs.h>
#include <linux/limits.h>
#include <linux/net.h>
#include <linux/socket.h>
#include <linux/skbuff.h>
#include <net/sock.h>

#include <asm/param.h>
#include <asm/uaccess.h>

/*
 * ----- SAMFS Includes
 */

#include "sam/types.h"
#include "sam/fioctl.h"

#include "samfs_linux.h"
#include "inode.h"
#include "mount.h"
#include "ino_ext.h"
#include "macros.h"
#include "ioblk.h"
#include "clextern.h"
#include "debug.h"
#include "syslogerr.h"
#include "trace.h"
#include "bswap.h"


static void sam_clear_incore_inode(sam_node_t *ip);
static void sam_release_ino(sam_node_t *ip);
void sam_inode_constructor(void *buf, kmem_cache_t *cachep,
						unsigned long cflags);
void sam_inode_destructor(void *buf, kmem_cache_t *cachep,
						unsigned long dflags);
int sam_read_disk_ino(sam_mount_t *mp, sam_id_t *fp, char *buf);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
static int sam_find_ino_actor26(struct inode *li, void *p);
#else
static int sam_find_ino_actor24(struct inode *li, unsigned long ino, void *p);
#endif
int sam_get_client_block(sam_mount_t *mp, sam_node_t *ip,
	enum BLOCK_operation op, enum SHARE_flag wait_flag,
	void *args, char *buf);
void rfs_make_bad_inode(struct inode *inode);

#ifndef SLAB_RECLAIM_ACCOUNT
#define	SLAB_RECLAIM_ACCOUNT 0
#endif
#ifndef SLAB_MEM_SPREAD
#define	SLAB_MEM_SPREAD 0
#endif

/*
 * ----- samqfs_setup_inode -
 * Use the data in sam_node_t to fill in
 * the Linux inode, and link them together.
 */
int
samqfs_setup_inode(struct inode *li, sam_node_t *ip)
{
	struct sam_disk_inode *dip;
	sam_mount_t *mp = NULL;
	struct super_block *lsb = NULL;
	int error = 0;

	mp = ip->mp;

	if (!mp) {
		return (EINVAL);
	}
	lsb = mp->mi.m_vfsp;

	dip = &ip->di;

	li->LI_I_PRIVATE = (void *)ip;
	ip->inode = li;

	li->i_mode = dip->mode;
	li->i_nlink = dip->nlink;
	li->i_uid = dip->uid;
	li->i_gid = dip->gid;
	li->i_rdev = 0;
	if (S_ISLNK(ip->di.mode) && (ip->di.ext_attrs & ext_sln)) {
		rfs_i_size_write(li, ip->di.psize.symlink);
	} else {
		rfs_i_size_write(li, dip->rm.size);
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	li->i_atime.tv_sec	= dip->access_time.tv_sec;
	li->i_atime.tv_nsec	= dip->access_time.tv_nsec;
	li->i_mtime.tv_sec	= dip->modify_time.tv_sec;
	li->i_mtime.tv_nsec = dip->modify_time.tv_nsec;
	li->i_ctime.tv_sec	= dip->change_time.tv_sec;
	li->i_ctime.tv_nsec = dip->change_time.tv_nsec;
#else
	li->i_atime = dip->access_time.tv_sec;
	li->i_mtime = dip->modify_time.tv_sec;
	li->i_ctime = dip->change_time.tv_sec;
	li->i_generation = dip->id.gen;
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18))
	li->i_blksize = LG_BLK(ip->mp, DD);
#endif

	li->i_blocks =
	    (u_longlong_t)dip->blocks * (u_longlong_t)(SAM_BLK/DEV_BSIZE);

	if (S_ISREG(li->i_mode)) {

		li->i_op = &samqfs_file_inode_ops;
		li->i_fop = &samqfs_file_ops;
		li->i_data.a_ops = &samqfs_file_aops;

	} else if (S_ISDIR(li->i_mode)) {

		li->i_op = &samqfs_dir_inode_ops;
		li->i_fop = &samqfs_dir_ops;

	} else if (S_ISLNK(li->i_mode)) {

		li->i_op = &samqfs_symlink_inode_ops;

	} else {

		init_special_inode(li, li->i_mode, ip->rdev);
	}

	/*
	 * Backwards support for residency time prior to 3.3.0-28
	 */
	if (ip->di.residence_time == 0) {
		ip->di.residence_time = ip->di.creation_time;
		ip->flags.b.changed = 1;
	}

	/*
	 * Backwards support prior to 3.2, 3.3.1, etc.
	 *
	 * Clear previous settings. Stageall and partial not
	 *  allowed with direct.
	 */
	if (ip->di.status.b.direct) {
		ip->di.status.b.stage_all = 0;
	}

	if (ip->di.status.b.bof_online) {
		if (ip->di.psize.partial == 0) {
			/*
			 * Backwards support for partial
			 * attribute prior to 3.2.
			 */
			if (ip->di.status.b.on_large) {
				ip->di.psize.partial = 16;
			} else {
				ip->di.psize.partial = 8;
			}
			if (ip->di.status.b.offline) {
				if (ip->di.extent[0] == 0) {
					ip->di.status.b.pextents = 0;
				} else {
					ip->di.status.b.pextents = 1;
				}
			}
		} else {
			/*
			 * Check for a 3.3.1 condition where the pextents flag
			 * was set, yet no data blocks existed.
			 */
			if (S_ISREG(ip->di.mode)) {
				if ((ip->di.status.b.pextents) &&
				    (ip->di.blocks == 0) &&
				    (ip->di.rm.size > 0)) {
					ip->di.status.b.pextents = 0;
				}
			}
		}
	}

	/*
	 * Only a regular file can be a segmented file, thus if a file is
	 * not a regular file, then explicitly clear the segmented file
	 * and segmented inode bit flags for the file.
	 */

	if (!S_ISREG(ip->di.mode)) {
		ip->di.status.b.seg_file = 0;
		ip->di.status.b.seg_ino  = 0;
	}

	/*
	 * Regular and directory files must be on disk.
	 */

	if (S_ISREG(ip->di.mode) || S_ISDIR(ip->di.mode)) {
		ip->di.rm.media = DT_DISK;
	}

	/* End of backward support */

	sam_set_size(ip);
	ip->cl_allocsz = ip->size;
	if (ip->cl_allocsz < (ip->di.blocks * SAM_BLK)) {
		ip->cl_allocsz = (ip->di.blocks * SAM_BLK);
	}

	/*
	 * Set directio if mount directio or file directio set.
	 */
	sam_reset_default_options(ip);
	if (S_ISREQ(ip->di.mode)) {
		if (ip->mp->mt.fi_version == SAMFS_SBLKV2) {
			ip->di.status.b.remedia = 0;
		}
	}
	ip->flags.b.stage_n = ip->di.status.b.direct;
	ip->flags.b.stage_all = ip->di.status.b.stage_all;

	return (error);
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
static int
sam_set_ino_actor(struct inode *li, void *p)
{
	sam_id_t *fp = (sam_id_t *)p;

	li->i_ino = fp->ino;
	li->i_generation = fp->gen;
	return (0);
}
#endif

/*
 * ----- sam_get_ino - Get/lock an inode.
 *	Given a file id (i-number/generation number), find it and
 *  return it held. First check the inode active hash cache.
 *  If there, return the inode locked. If not there either get a
 *  free inode if the inode limit has been reached or allocate a
 *  new inode if the inode limit has not been reached.
 *  flag is set:
 *   IG_NEW		- get new inode.
 *   IG_EXISTS	- get existing inode for given id, id must match.
 *   IG_STALE	- get existing inode, or if stale, get stale inode, id.ino
 *		  must match, but generation may be less than id.gen.
 *   IG_SET	- create a new inode and initialize it using the record passed
 *				  in via ipp.
 */

int				/* ERRNO if error, 0 if successful--ipp set. */
sam_get_ino(
	struct super_block *sb,	/* sb pointer for this instance of mount. */
	sam_iget_t flag,	/* flag - get new, existing, or stale inode */
	sam_id_t *fp,		/* file i-number & generation number. */
	sam_node_t **ipp)	/* pointer to pointer to inode (returned). */
{
	int error;
	sam_mount_t *mp;
	struct inode *li = NULL;
	sam_node_t *ip = NULL;
	sam_node_t *fip = NULL;
	sam_ino_record_t *irec;
	struct sam_perm_inode *permip;
	unsigned long cflags = 0;
	unsigned long hash;

	ASSERT((flag == IG_SET) || (flag == IG_NEW));
	irec = (flag == IG_SET) ? ((sam_ino_record_t *)*ipp) : NULL;
	*ipp = NULL;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	mp = (sam_mount_t *)(void *)sb->s_fs_info;
#else
	mp = (sam_mount_t *)(void *)sb->u.generic_sbp;
#endif
	TRACE(T_SAM_FIND, sb, fp->ino, fp->gen, 0);

	if (fp->ino == 0) {
		return (ENOENT);
	}
	if (mp->mt.fi_status & FS_UMOUNT_IN_PROGRESS) {
		TRACE(T_SAM_UMNT_IGET, NULL, mp->mt.fi_status,
		    (sam_tr_t)mp, EBUSY);
		return (EBUSY);
	}

	hash = (unsigned long)fp->ino;
	ASSERT(fp->gen > 0);

	/*
	 * We can race with threads creating new inodes.  If we lose
	 * and the winner gets back here and marks it bad (jumps to out_bad)
	 * then we have try again.
	 * On 2.4 kernels we can also race with threads in sam_check_cache;
	 * if they win and the inode is new they'll mark it as bad.  So,
	 * we'd have to try again.
	 */
lookup_inode:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	li = iget5_locked(sb, hash, sam_find_ino_actor26,
	    sam_set_ino_actor, fp);
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
	li = iget4_locked(sb, hash, sam_find_ino_actor24, fp);
#endif
	if (is_bad_inode(li)) {
		iput(li);
		goto lookup_inode;
	}

	/*
	 * Refresh an existing inode.
	 */
	if (!(li->i_state & I_NEW)) {
		fip = SAM_LITOSI(li);
		if (flag == IG_SET) {
			RW_LOCK_OS(&fip->inode_rwl, RW_WRITER);
			error = sam_reset_client_ino(fip, 0, irec);
			ASSERT(!error);
			RW_UNLOCK_OS(&fip->inode_rwl, RW_WRITER);
		}
		*ipp = fip;
		goto found;
	}
	/*
	 * We have a new inode to populate.
	 */
	ASSERT(li->i_state & I_NEW);

	/*
	 * Allocate a sam node
	 */

	ip = (sam_node_t *)kmem_cache_alloc(samgt.inode_cache, KM_SLEEP);
	sam_inode_constructor((void *)ip, samgt.inode_cache, cflags);

	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	ip->flags.bits = 0;
	ip->flags.b.busy = 1;	/* Inode is active (SAM_BUSY) */
	mutex_enter(&samgt.ifreelock);
	samgt.inocount++;
	mutex_exit(&samgt.ifreelock);

	ip->mp = mp;
	ip->dev = mp->mi.m_fs[0].dev;
	ip->di.id.ino = 0;
	sam_clear_incore_inode(ip);

	if (flag == IG_SET) {
		ip->di.mode = 0;
		error = sam_reset_client_ino(ip, 0, irec);
		ASSERT(!error);
	} else {
		char *buf;

		buf = kmem_alloc(sizeof (struct sam_perm_inode), KM_SLEEP);

		error = sam_read_disk_ino(mp, fp, buf);

		if (error) {
			kfree(buf);
			goto out_bad;
		}
		permip = (struct sam_perm_inode *)buf;

		/*
		 *  Move disk image to incore inode
		 */

		ip->di = permip->di;
		ip->di2 = permip->di2;

		kfree(buf);
	}


	/*
	 * Hook up Linux inode and sam_node
	 */
	samqfs_setup_inode(li, ip);
	unlock_new_inode(li);

	/*
	 * Using the new inode
	 */
	*ipp = ip;
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

found:
	ASSERT(fp->ino == (*ipp)->di.id.ino);
	ASSERT(fp->gen == (*ipp)->di.id.gen);
	TRACE(T_SAM_FIND1, li, (*ipp)->di.id.ino, (*ipp)->di.id.gen,
	    atomic_read(&li->i_count));
	return (0);

out_bad:
	ASSERT(li->i_state & I_NEW);
	rfs_make_bad_inode(li);
	unlock_new_inode(li);

	TRACE(T_SAM_FIND3, NULL, ip->di.mode, ip->di.id.ino,
	    ip->di.id.gen);
	if (error == EBADR) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		*ipp = ip;
		TRACE(T_SAM_FIND4, NULL, error, fp->ino, fp->gen);
		return (error);
	}

	/*
	 * We created an inode placeholder but then couldn't read the inode.
	 */
	ip->flags.b.stale = 1;
	TRACE(T_SAM_FIND5, NULL, error, fp->ino, fp->gen);
	sam_release_ino(ip);
	return (error);
}


/*
 * ----- sam_clear_incore_inode - Clear incore flags.
 */

static void
sam_clear_incore_inode(sam_node_t *ip)
{
	int i;

	time_t curr_time;

	curr_time = SAM_SECOND();

	/* XXX - eventually only initialize client/server fields if shared? */

	/*
	 * Pointer to the Linux inode
	 */
	ip->inode = NULL;

	/*
	 * Client fields.
	 */
	ip->cl_allocsz = 0;
	ip->cl_pend_allocsz = 0;
	ip->cl_alloc_unit = ip->mp->mt.fi_minallocsz;
	ip->cl_leases = 0;
	ip->cl_saved_leases = 0;
	ip->cl_short_leases = 0;
	ip->cl_flags = 0;
	ip->cl_locks = 0;
	ip->no_opens = 0;
	ip->cl_closing = 0;
	ip->cl_flock = NULL;
	ip->cl_attr_seqno = 0;
	ip->cl_ino_seqno = 0;
	ip->cl_ino_gen = 0;
	ip->cl_srvr_ord = 0;
	for (i = 0; i < MAX_EXPIRING_LEASES; i++) {
		ip->cl_leaseused[i] = 0;
	}
	ip->cl_lease_chain.forw = ip->cl_lease_chain.back = NULL;
	ip->cl_lease_chain.node = ip;

	/*
	 * Server fields.
	 */
	ip->size_owner = 0;
	ip->stage_pid = 0;

	ip->updtime = curr_time;

	ip->copy = 0;
	ip->rdev = 0;
	ip->mm_pages = 0;
	ip->wmm_pages = 0;
	ip->space = 0;
	ip->real_stage_off = 0;
	ip->real_stage_len = 0;
	ip->stage_n_count = 0;
	ip->segment_ip = NULL;
	ip->rm_wait = 0;
	ip->nfs_ios = 0;
	ip->stage_err = 0;
	ip->flags.bits &= (SAM_BUSY|SAM_FREE|SAM_HASH);
	ip->stage_n_off = 0;
	ip->stage_off = 0;
	ip->stage_len = 0;
	ip->stage_size = 0;
	ip->zero_end = 0;
	sam_clear_map_cache(ip);
	ip->cl_iecachep = NULL;
}


/*
 * ----- sam_verify_root - Verify the inode in cache exists
 *	and seems to be a root of a sam file system.
 */

int				/* 0 if valid */
sam_verify_root(sam_node_t *ip, sam_mount_t *mp)
{
	int	error = 1;	/* assume some error */
	if (S_ISDIR(ip->di.mode)) {
		if (ip->di.id.ino != SAM_ROOT_INO) {
			cmn_err(CE_WARN,
			    "SAM-QFS: root inode not SAM_ROOT_INO on eq %d",
			    mp->mi.m_fs[0].part.pt_eq);
		}
		if (ip->di.parent_id.ino != ip->di.id.ino ||
		    ip->di.parent_id.gen != ip->di.id.gen) {
			cmn_err(CE_WARN,
			    "SAM-QFS: root inode parent not self on eq %d",
			    mp->mi.m_fs[0].part.pt_eq);
		}
		/* let 'em off with a warning */
		error = 0;
	} else {
		cmn_err(CE_WARN,
		    "SAM-QFS: root inode on eq %d is not a directory",
		    mp->mi.m_fs[0].part.pt_eq);
		error = ENOTDIR;
	}
	return (error);
}


/*
 * ----- sam_check_cache -
 * Get an inode if it's in the Linux cache.
 */

int				/* ERRNO, else 0 if inode pointer returned */
sam_check_cache(
	sam_id_t *fp,		/* file i-number & generation number. */
	sam_mount_t *mp,	/* pointer to mount table. */
	sam_iget_t flag,	/* flag - get new, existing, or stale inode */
	sam_node_t *vip,	/* pointer to hash lock. */
	sam_node_t **rip)	/* NULL if not found, pointer if found. */
{
	struct super_block *lsb;
	struct inode *li;
	sam_node_t *ip;
	unsigned long hash;

	*rip = NULL;
	if (flag != IG_EXISTS) {
		return (EINVAL);
	}

	lsb = mp->mi.m_vfsp;

	hash = (unsigned long)fp->ino;
	ASSERT(fp->gen > 0);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
	/*
	 * Find an existing inode.  We don't want a new inode, so junk it
	 * if that happens.  We can race with other threads looking for
	 * this inode, too, and the winner might have marked the inode
	 * as bad so we have to deal with that.
	 */
	li = iget4_locked(lsb, hash, sam_find_ino_actor24, fp);
	ASSERT(li);
	if (li->i_state & I_NEW) {
		rfs_make_bad_inode(li);
		unlock_new_inode(li);
	}
	if (is_bad_inode(li)) {
		iput(li);
		return (ENOENT);
	}
#else
	/*
	 * This looks for an existing inode only.
	 */
	li = ilookup5(lsb, hash, sam_find_ino_actor26, (void *)fp);
	if (li == NULL) {
		return (ENOENT);
	}
#endif

	ip = SAM_LITOSI(li);
	ASSERT(ip);
	*rip = ip;
	return (0);
}


/*
 * ----- sam_release_ino - Release inode.
 *	Release newly acquired inode. inode_rwl WRITER lock is set.
 */

static void
sam_release_ino(sam_node_t *ip)
{
	struct inode *li;

	ASSERT(RW_WRITE_HELD(&ip->inode_rwl));

	li = SAM_SITOLI(ip);
	if (!li) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		kmem_cache_free(samgt.inode_cache, (void *)ip);
		mutex_enter(&samgt.ifreelock);
		samgt.inocount--;
		mutex_exit(&samgt.ifreelock);
		return;
	}
	iput(li);
}


/*
 * ----- sam_read_disk_ino - Read inode and return buffer pointer.
 * Try for 1 second to get inode. If server is not responding,
 * return with ETIME errno.
 */

int				/* ERRNO if error, 0 if successful. */
sam_read_disk_ino(
	sam_mount_t *mp,	/* pointer to the mount table. */
	sam_id_t *fp,		/* file i-number & generation number. */
	char *buf)		/* pointer pointer to inode data (returned). */
{
	int error;
	sam_block_getino_t getino;

	if (!SAM_IS_SHARED_CLIENT(mp)) {
		return (ENOSYS);
	}

	if (mp->mt.fi_status & FS_LOCK_HARD) {
		return (ETIME);
	}

	getino.id = *fp;
	getino.bsize = sizeof (struct sam_perm_inode);
	getino.addr = 0;
	memcpy((void *)&getino.addr, (void *)&buf, sizeof (char *));

	error = sam_get_client_block(mp, NULL, BLOCK_getino,
	    SHARE_wait_one, &getino, buf);

	if (error) {
		TRACE(T_SAM_BREAD_ERR,
		    NULL, error, 0, ((daddr_t)0x80000000 | fp->ino));
	}
	return (error);
}


/*
 * ----- sam_reset_default_options - reset to default mount options.
 * Reset directio to the setfa -D or mount forcedirectio setting.
 * Reset qwrite to the qwrite mount setting.
 * NOTE: inode_rwl WRITERS lock set on entry and exit.
 */

void
sam_reset_default_options(sam_node_t *ip)
{
	if (ip->di.id.ino == SAM_INO_INO) {
		return;
	}
	if (S_ISREG(ip->di.mode)) {
		int directio_flag;

		directio_flag =
		    (ip->di.status.b.directio ||
		    (ip->mp->mt.fi_config & MT_DIRECTIO)) ?
		    DIRECTIO_ON : DIRECTIO_OFF;
		sam_set_directio(ip, directio_flag);
		ip->flags.b.qwrite = (ip->mp->mt.fi_config & MT_QWRITE) ? 1 : 0;
	}
}


/*
 * ----- sam_set_directio - set directio
 *
 * NOTE: inode_rwl WRITERS lock set on entry and exit.
 */

void				/* ERRNO, else 0 if successful. */
sam_set_directio(
	sam_node_t *ip,		/* Pointer to inode. */
	int directio_flag)	/* DIRECTIO_ON or DIRECTIO_OFF */
{
	ASSERT(RW_WRITE_HELD(&ip->inode_rwl));

	/*
	 * No directio for now
	 */
	directio_flag = DIRECTIO_OFF;

	if (S_ISREG(ip->di.mode)) {
		if (ip->flags.b.directio != directio_flag) {
			ip->flags.b.directio = directio_flag;
			if (directio_flag == DIRECTIO_ON) {
				/*
				 * Flush and invalidate pages to set directio.
				 * Cannot flush pages now if memory mapped.
				 */
				if (!ip->mm_pages) {
					sam_flush_pages(ip, B_INVAL);
				}
			}
		}
	}
}

/*
 * ----- sam_hold_vnode -
 * Increment existing inode count
 */

int				/* ERRNO, else 0 if successful. */
sam_hold_vnode(
	sam_node_t *ip,		/* Pointer to inode. */
	sam_iget_t flag)	/* flag - get new, existing, or stale inode */
{
	int error = 0;

	atomic_inc(&(SAM_SITOLI(ip)->i_count));
	return (error);
}


/*
 * ----- sam_set_size -
 * Set the real size based on offline/online. Set partial if
 * partial extents exist.
 */

void
sam_set_size(sam_node_t *ip)
{
	if (ip->di.status.b.offline == 0) {
		ip->size = ip->di.rm.size;
		if (S_ISSEGI(&ip->di)) {
			ip->size = ip->di.rm.info.dk.seg.fsize;
		}
	} else if (!ip->flags.b.staging) {
		ip->size = 0;
		if (ip->di.status.b.bof_online) {
			if (ip->di.status.b.pextents) { /* Partial online */
				ip->size = sam_partial_size(ip);
			}
		}
	}
}


/*
 * ----- sam_create_ino_cache - creates the SAM-QFS inode cache.
 * Called from sam_init_ino in init_linux.c
 */

void
sam_create_ino_cache(void)
{
	samgt.inode_cache = kmem_cache_create("sam_inode_cache",
	    sizeof (sam_node_t), 0,
	    SLAB_HWCACHE_ALIGN|SLAB_MEM_SPREAD|SLAB_RECLAIM_ACCOUNT,
	    NULL,
	    NULL);
}

/*
 * ----- sam_delete_ino_cache - deletes the SAM-QFS inode cache.
 * Destroys all free inodes and deletes the sam inode cache.
 * Called from sam_clean_ino in init.c
 */

void
sam_delete_ino_cache(void)
{
	kmem_cache_destroy(samgt.inode_cache);
	samgt.inode_cache = NULL;
}


/*
 * ----- sam_inode_constructor - initializes a new SAM-QFS inode.
 */

/* ARGSUSED1 */
void
sam_inode_constructor(
	void *buf,			/* inode to be constructed */
	kmem_cache_t *cachep,
	unsigned long notused)
{
	sam_node_t *ip = (sam_node_t *)buf;

	memset((char *)ip, 0, sizeof (sam_node_t));

	RW_LOCK_INIT_OS(&ip->inode_rwl, NULL, RW_DEFAULT, NULL);

	ip->flags.b.busy = 1;	/* Inode is active (SAM_BUSY) */

	RW_LOCK_INIT_OS(&ip->data_rwl, NULL, RW_DEFAULT, NULL);

	sam_mutex_init(&ip->rm_mutex, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&ip->rm_cv, NULL, CV_DEFAULT, NULL);
	sam_mutex_init(&ip->fl_mutex, NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&ip->iom.map_mutex, NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&ip->ilease_mutex, NULL, MUTEX_DEFAULT, NULL);

	/*
	 * Client fields.
	 */
	sam_mutex_init(&ip->cl_apmutex, NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&ip->cl_iec_mutex, NULL, MUTEX_DEFAULT, NULL);
}


/*
 * ----- sam_inode_destructor - clears a deleted SAM-QFS inode.
 */

/* ARGSUSED1 */
void
sam_inode_destructor(void *buf, kmem_cache_t *cachep, unsigned long dflags)
{
	sam_node_t *ip = (sam_node_t *)buf;
	struct inode *li;

	li = SAM_SITOLI(ip);

	RW_LOCK_DESTROY_OS(&ip->inode_rwl);
	RW_LOCK_DESTROY_OS(&ip->data_rwl);
	mutex_destroy(&ip->daemon_mutex);
	cv_destroy(&ip->daemon_cv);
	mutex_destroy(&ip->rm_mutex);
	cv_destroy(&ip->rm_cv);
	mutex_destroy(&ip->write_mutex);
	cv_destroy(&ip->write_cv);
	mutex_destroy(&ip->fl_mutex);
	mutex_destroy(&ip->cl_apmutex);
}


/*
 * ----- sam_find_ino_actor -
 * Called by Linux from find_inode when a Linux inode is
 * found with a matching super_block and inode number.
 *
 * If there is a race to create a new inode then the loser will
 * get here with an inode that is I_NEW|I_LOCK and has no
 * sam_node.  We need to call that a match, and the loser will
 * sit in wait_on_inode() with this inode until the winner completes the
 * initialization and unlocks it.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
static int
sam_find_ino_actor24(struct inode *li, unsigned long ino, void *p)
{
	sam_id_t *fp = (sam_id_t *)p;

	if (is_bad_inode(li)) {
		return (0);
	}
	if (li->i_state & (I_CLEAR|I_FREEING)) {
		return (0);
	}

	ASSERT(ino == fp->ino);
	if (li->i_state & I_NEW) {
		/*
		 * Another thread beat us to create a new inode, but
		 * they haven't finished initializing it yet.
		 */
		return (1);
	}

	if (li->i_generation == fp->gen) {
		return (1);
	}

	return (0);
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
static int
sam_find_ino_actor26(struct inode *li, void *p)
{
	sam_id_t *fp = (sam_id_t *)p;

	if (is_bad_inode(li)) {
		return (0);
	}
	if (li->i_state & (I_CLEAR|I_FREEING)) {
		return (0);
	}

	if ((li->i_ino == fp->ino) &&
	    (li->i_generation == fp->gen)) {
		return (1);
	}

	return (0);
}
#endif


/*
 * ----- sam_wait_space - wait until disk space is available.
 * No space (ENOSPC), if not a shared client, or not waiting on reclaim thread,
 *    issue "File system full" message,
 * if file is on QFS meta device, return ENOSPC and issue META ENOSPC message.
 * if shared QFS (no SAM) or standalone FS data device, return ENOSPC
 * if NFS and not standalone, return EINPROGRESS
 * otherwise hold thread until space is available.
 */

int				/* ERRNO if error occured, 0 if successful. */
sam_wait_space(
	sam_node_t *ip,		/* The inode table entry pointer. */
	int error)		/* SAM ENOSPC error */
{
	char *err_str = "waiting";
	int ret_error = 0;
	enum syslog_space_actions action = E_WAITING;
	sam_mount_t *mp;	/* The mount table pointer. */

	mp = ip->mp;

	if (ip->di.status.b.meta || IS_SAM_META_ENOSPC(error)) {
		/* if meta inode, META ENOSPC */
		ret_error = ENOSPC;
		err_str = "META ENOSPC";
		action = E_NOSPC;

	} else if ((SAM_IS_SHARED_CLIENT(mp) && !(SAM_IS_SHARED_SAM_FS(mp))) ||
	    (!(SAM_IS_SHARED_CLIENT(mp)) && SAM_IS_STANDALONE(mp))) {
		ret_error = ENOSPC;
		err_str = "ENOSPC";
		action = E_NOSPC;
	}

	/*
	 * Issue "full" message at most once every 5 minutes.
	 * No message issued if waiting on reclaim thread, nor
	 * if a shared client.
	 */
	error = ret_error;
	TRACE(T_SAM_WAIT_FSPACE1, mp->mi.m_vfsp, (sam_tr_t)mp, error,
	    mp->mi.m_fsfullmsg);

	if (error == 0) {
		mutex_enter(&mp->ms.m_waitwr_mutex);
		mp->mi.m_wait_write++;
		if (sam_cv_wait_sig(&mp->ms.m_waitwr_cv,
		    &mp->ms.m_waitwr_mutex) == 0) {
			error = EINTR;
		}
		mp->mi.m_wait_write--;
		if (mp->mt.fi_status & FS_FAILOVER) {
			mutex_exit(&mp->ms.m_waitwr_mutex);
			error = sam_stop_inode(ip);
		} else {
			mutex_exit(&mp->ms.m_waitwr_mutex);
		}
	}
	TRACE(T_SAM_WAIT_FSPACE2, mp->mi.m_vfsp, (sam_tr_t)mp, error,
	    mp->mi.m_fsfullmsg);
	return (error);
}

/*
 * ----- sam_get_symlink - Retrieve a SAM-QFS symbolic link name.
 * Return the symbolic link name from inode extension(s).
 * Caller must have the inode read lock held.  This lock is not released.
 */
int
sam_get_symlink(
	sam_node_t *bip,	/* Pointer to base inode */
	struct uio *uiop)	/* User I/O vector array. */
{
	int error = 0;
	char *bp;
	sam_id_t eid;
	struct sam_inode_ext *eip;
	int ino_chars;
	int n_chars;
	char *slp = uiop->uio_iov->iov_base;
	int buf_len = uiop->uio_iov->iov_len;

	if ((bp = kmem_alloc(sizeof (struct sam_perm_inode), KM_SLEEP)) ==
	    NULL) {
		return (ENOMEM);
	}

	n_chars = 0;
	eid = bip->di.ext_id;
	while (eid.ino) {
		if ((error = sam_read_disk_ino(bip->mp, &eid, bp)) != 0) {
			break;
		}
		/*
		 * For extent i-nodes, we need to undo the byte-swapping that
		 * assumed a regular i-node.
		 */
		eip = (struct sam_inode_ext *)bp;
		if (EXT_HDR_ERR(eip, eid, bip)) {
			error = ENOCSI;
			break;
		}
		if (S_ISSLN(eip->hdr.mode)) {
			ino_chars = eip->ext.sln.n_chars;
			if ((ino_chars == 0) ||
			    (ino_chars > MAX_SLN_CHARS_IN_INO) ||
			    ((n_chars + ino_chars) > bip->di.psize.symlink)) {
				error = ENOCSI;
				break;
			}
			/* Don't exceed supplied buffer length. */
			if ((n_chars + ino_chars) > buf_len) {
				error = EFAULT;
				break;
			}
			if (uiop->uio_segflg == UIO_USERSPACE) {
				if (copy_to_user(slp, eip->ext.sln.chars,
				    ino_chars)) {
					error = EFAULT;
					break;
				}
			} else {
				memcpy(slp, eip->ext.sln.chars, ino_chars);
			}
			n_chars += ino_chars;
			slp += ino_chars;
		}
		eid = eip->hdr.next_id;
	}
	if (error == 0) {
		if (n_chars != bip->di.psize.symlink) {
			error = ENOCSI;
		}
	}
	kfree(bp);

	TRACE(T_SAM_GET_SLN_EXT, SAM_SITOLI(bip), (int)bip->di.id.ino, n_chars,
	    error);
	if (error == 0) {
		return (-n_chars);
	}
	return (error);
}
