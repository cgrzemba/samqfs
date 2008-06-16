/*
 * ----- clmisc.c - Miscellaneous interfaces for the client file system.
 *
 *	Miscellaneous interface modules for the client.
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
#pragma ident "$Revision: 1.251 $"
#endif

#include "sam/osversion.h"

/*
 * ----- UNIX Includes
 */

#ifdef sun
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/t_lock.h>
#include <sys/cred.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/ddi.h>
#include <sys/disp.h>
#include <sys/kmem.h>
#include <sys/stat.h>
#include <sys/dnlc.h>
#include <sys/file.h>
#include <sys/flock.h>
#include <nfs/lm.h>
#include <sys/policy.h>
#endif /* sun */

#ifdef linux
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
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#include <linux/buffer_head.h>
#endif

#include <asm/param.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#endif /* linux */

/*
 * ----- SAMFS Includes
 */

#include <sam/types.h>
#include <sam/syscall.h>
#include <sam/shareops.h>

#ifdef linux
#include "samfs_linux.h"
#endif /* linux */

#include "behavior_tags.h"
#include "bswap.h"
#include "inode.h"
#include "mount.h"
#include "scd.h"
#include "global.h"

#ifdef sun
#include "extern.h"
#endif /* sun */
#ifdef linux
#include "clextern.h"
#endif /* linux */

#include "indirect.h"
#include "rwio.h"
#include "ino_ext.h"

#ifdef sun
#include "debug.h"
#endif /* sun */

#include "trace.h"

#ifdef sun
#include "kstats.h"
static void sam_stop_waiting_frlock(sam_node_t *ip, sam_share_flock_t *flockp);
static int sam_munge_sysid(int original_sysid, int client_ord);
extern int sam_max_shared_hosts;
#endif /* sun */
#ifdef linux
#include "kstats_linux.h"
#endif /* linux */

#include "../lib/bswap.c"


/*
 * ----- sam_stale_indirect_blocks -
 * Stale all the indirect blocks on the client from offset up to EOF.
 * Never stale server indirect blocks. Symlink, direct map, and object
 * regular files do not have indirect blocks.
 */

int				/* ERRNO, else 0 if successful. */
sam_stale_indirect_blocks(
	sam_node_t *ip,		/* Pointer to inode table */
	offset_t offset)	/* Stale indirect buffer offset */
{
	int i;
	int kptr[NIEXT + 1];
	sam_mount_t *mp;
	int bt, dt;
	int ileft;
	int de;
	struct sam_get_extent_args arg;
	struct sam_get_extent_rval rval;
	int error = 0;

	if (SAM_IS_SHARED_SERVER(ip->mp) || ip->di.status.b.direct_map ||
	    S_ISLNK(ip->di.mode) || SAM_IS_OBJECT_FILE(ip)) {
		return (0);
	}

	de = -1;
	for (i = 0; i < (NIEXT+1); i++) {
		kptr[i] = -1;
	}
	mp = ip->mp;
	arg.dau = &mp->mi.m_dau[0];
	arg.num_group = mp->mi.m_fs[ip->di.unit].num_group;
	arg.sblk_vers = mp->mi.m_sblk_version;
	arg.status = ip->di.status;
	arg.offset = offset;
	rval.de = &de;
	rval.dtype = &dt;
	rval.btype = &bt;
	rval.ileft = &ileft;
	if ((error = sam_get_extent(&arg, kptr, &rval))) {
		return (error);
	}
	if (de < NDEXT) {
		de = NDEXT;
	}
	for (i = (NOEXT - 1); i >= de; i--) {
		int set;
		int kptr_index;

		if (ip->di.extent[i] == 0) {
			continue;
		}
		if (i == NDEXT) {
			daddr_t blkno;

			blkno = ip->di.extent[NDEXT];
#ifdef linux
			sam_iecache_ent_clear(ip, ip->di.extent_ord[NDEXT],
			    blkno << ip->mp->mi.m_bn_shift);
#endif /* linux */
#ifdef sun
			blkno <<= (ip->mp->mi.m_bn_shift + SAM2SUN_BSHIFT);
			sam_blkstale(
			    ip->mp->mi.m_fs[ip->di.extent_ord[NDEXT]].dev,
			    blkno);
#endif /* sun */
		} else {
			set = 0;
			kptr_index = i - de;
			if (kptr_index == 0) {
				set = 1;
			}
			error = sam_flush_indirect_block(ip, SAM_STALE_BC,
			    kptr, kptr_index, &ip->di.extent[i],
			    &ip->di.extent_ord[i], (i - (NDEXT + 1)), &set);
			if (error == ECANCELED) {
				/*
				 * Encountered offset -- done
				 */
				error = 0;
				break;
			}
		}
	}
	return (error);
}


/*
 * ----- sam_build_header - build san message header.
 */

void
sam_build_header(sam_mount_t *mp, sam_san_header_t *hdr, ushort_t cmd,
    enum SHARE_flag wait_flag, ushort_t op, ushort_t length,
    ushort_t out_length)
{
	hdr->magic = SAM_SOCKET_MAGIC;
	hdr->unused = 0;
	hdr->wait_flag = (ushort_t)wait_flag;
	hdr->command = cmd;
	hdr->operation = op;
	hdr->length = length;
	hdr->out_length = out_length;
	hdr->error = 0;
	hdr->client_ord = mp->ms.m_client_ord;
	hdr->server_ord = 0;
	hdr->hostid = mp->ms.m_hostid;
	hdr->fsid = mp->mi.m_sblk_fsid;
	hdr->fsgen = mp->mi.m_sblk_fsgen;
	hdr->reset_seqno = 0;
}


/*
 * ---- sam_update_shared_filsys - Get superblock from the server.
 *	For shared client, get updated superblock info for df.
 *	If the superblock has not yet been set or the number of partitions
 *	changed, call sam_update_shared_sblk to get the updated superblock.
 */
int
sam_update_shared_filsys(
	sam_mount_t *mp,		/* The mount table pointer. */
	enum SHARE_flag wait_flag,	/* Wait forever, or wait for a while */
	int flag)			/* SYNC_CLOSE if mounting/umounting */
{
	struct sam_sblk *sblk;
	int error = 0;

	if (flag != 0) {	/* If umounting/failover, don't get sblk */
		return (0);
	}

	/*
	 * If we don't have a valid copy of the superblock, get one.
	 */
	sblk = mp->mi.m_sbp;
	if ((sblk == NULL) || (sblk->info.sb.fs_count != mp->mt.fs_count)) {
		error = sam_update_shared_sblk(mp, wait_flag);
		mp->ms.m_vfs_time = 0;	/* Update sblk now */
	}

	/*
	 * Update existing superblock info.
	 */
	if (error == 0) {
		error = sam_proc_block_vfsstat(mp, wait_flag);
		if (error) {
			return (error);
		}
	}

	sblk = mp->mi.m_sbp;
	if (mp->mi.m_wait_write && (sblk != NULL) &&
	    (sblk->info.sb.space > 0)) {
		/*
		 * Wake up any thread waiting to allocate blocks.
		 */
		mutex_enter(&mp->ms.m_waitwr_mutex);
		if (mp->mi.m_wait_write) {
			cv_broadcast(&mp->ms.m_waitwr_cv);
		}
		mutex_exit(&mp->ms.m_waitwr_mutex);
	}
	return (error);
}


/*
 * ---- sam_update_shared_sblk - Get superblock from the server.
 *	For shared client, get updated superblock for df and partition info.
 *	If the superblock has not been set or the number of partitions
 *	changed, allocate a buffer (pointed to by m_sbp) and fill
 *	with the full super block. This buffer is released during unmount
 *	by sam_cleanup_mount().
 */
int
sam_update_shared_sblk(
	sam_mount_t *mp,		/* The mount table pointer. */
	enum SHARE_flag wait_flag)	/* Wait forever, or wait for a while */
{
#ifdef sun
	buf_t *bp;
#endif /* sun */
#ifdef linux
	char *buf;
#endif /* linux */
	struct sam_sblk *sblk, *old_sblk;
	int32_t sblk_size, old_sblk_size, s;
	int ord;
	int old_fs_count;
	int error = 0;

	/*
	 * If the superblock has not yet been set or the partitions changed,
	 * read the first sector to get the superblock size.
	 */
	sblk = mp->mi.m_sbp;
	old_sblk = sblk;
	sblk_size = mp->mi.m_sblk_size;
	old_sblk_size = sblk_size;

	old_fs_count = 0;
	if ((old_sblk == NULL) ||
	    (mp->mt.fs_count > old_sblk->info.sb.fs_count)) {
		sblk_size = DEV_BSIZE;
		if (old_sblk != NULL) {
			old_fs_count = old_sblk->info.sb.fs_count;
		}
	}

refetch:

#ifdef sun
	if ((error = sam_get_client_sblk(mp, wait_flag, sblk_size, &bp))) {
		return (error);
	}
	sblk = (struct sam_sblk *)(void *)bp->b_un.b_addr;
	s = roundup(sblk->info.sb.sblk_size, DEV_BSIZE);
	if (s != sblk_size) {
		bp->b_flags |= B_STALE | B_AGE;
		brelse(bp);
		sblk_size = s;
		goto refetch;
	}
#endif /* sun */

#ifdef linux
	buf = kmem_zalloc(sblk_size, KM_SLEEP);
	if ((error = sam_get_client_sblk(mp, wait_flag, sblk_size, buf))) {
		kmem_free(buf, sblk_size);
		return (error);
	}
	sblk = (struct sam_sblk *)(void *)buf;
	s = roundup(sblk->info.sb.sblk_size, DEV_BSIZE);
	if (s != sblk_size) {
		kmem_free(buf, sblk_size);
		sblk_size = s;
		goto refetch;
	}

#endif /* linux */

	/*
	 * Don't update superblock if server superblock partition count
	 * is != client partition count. The client must build the mcf
	 * and execute a samd conf
	 */
	if (sblk->info.sb.fs_count != mp->mt.fs_count) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: Server %s has %d partitions, client has %d"
		    " Execute samd buildmcf followed by"
		    " samd conf to update mcf",
		    mp->mt.fi_name, mp->mt.fi_server,
		    sblk->info.sb.fs_count, mp->mt.fs_count);
#ifdef sun
		bp->b_flags |= B_STALE | B_AGE;
		brelse(bp);
#endif /* sun */
#ifdef linux
		kmem_free(buf, sblk_size);
#endif /* linux */
		return (EBUSY);
	}

#ifdef sun
	/*
	 * Make a copy of the superblock, then release the buffer.
	 */
	if (sblk_size > old_sblk_size) {
		sblk = kmem_alloc(sblk_size, KM_SLEEP);
	} else {
		sblk = old_sblk;
		old_sblk = NULL;
	}

	bcopy((void *)bp->b_un.b_addr, sblk, sblk_size);
	bp->b_flags |= B_STALE | B_AGE;
	brelse(bp);
#endif /* sun */

	/*
	 * Update allocation links if sblk increased.
	 */
	if (old_fs_count) {
		int prev_num_grp = -1;
		int dt, dev_type;
		int i, j;

		for (i = old_fs_count; i < sblk->info.sb.fs_count; i++) {
			dt = (mp->mi.m_fs[i].part.pt_type == DT_META) ? MM : DD;
			for (j = 0; j < sblk->info.sb.fs_count; j++) {
				dev_type = (mp->mi.m_fs[j].part.pt_type ==
				    DT_META) ? MM : DD;
				if (dt == dev_type) {
					prev_num_grp = mp->mi.m_fs[j].num_group;
					break;
				}
			}
			(void) sam_build_allocation_links(mp, sblk, i,
			    &prev_num_grp);
		}
	}

	/*
	 * Update state of LUNs in superblock.
	 */
	for (ord = 0; ord < sblk->info.sb.fs_count; ord++) {
		mp->mi.m_fs[ord].part.pt_state = sblk->eq[ord].fs.state;
	}

	/*
	 * Move the new buffer pointer into place. Relies on store atomicity;
	 * it's OK for other threads to see either the old sblk or the new
	 * sblk, but no intermediate states. Note that we set m_sbp only
	 * after setting all related fields.
	 */
	ASSERT(sblk->info.sb.magic != SAM_MAGIC_V1);
	if ((sblk->info.sb.magic == SAM_MAGIC_V2) ||
	    (sblk->info.sb.magic == SAM_MAGIC_V2A)) {
		mp->mi.m_sblk_version = mp->mt.fi_version = SAMFS_SBLKV2;
	}
	mp->mi.m_sblk_fsid = sblk->info.sb.init;
	mp->mi.m_sblk_fsgen = sblk->info.sb.gen;
	mp->mi.m_sblk_size = sblk_size;
	mp->mi.m_sbp = sblk;
	if (old_sblk) {
		kmem_free(old_sblk, old_sblk_size);
	}
	return (0);
}


/*
 * ---- sam_update_shared_ino - Update inode on the server.
 *      If inode accessed/modified/changed, update buffer with incore inode.
 */

int				/* errno if err, 0 if ok. */
sam_update_shared_ino(
	sam_node_t *ip,		/* The inode table pointer. */
	enum sam_sync_type st,	/* SYNC_ALL for delayed write; */
				/* SYNC_ONE for sync ino */
	boolean_t write_lock)
{
	enum INODE_operation op;
	uchar_t iflags;
	int error;

	/*
	 * Lift lock and resolve conflicts in the server.
	 */
	iflags = ip->cl_flags;
	if (st == SAM_SYNC_ALL) {
		op = INODE_fsync_nowait;
	} else {
		op = INODE_fsync_wait;
	}
	if (write_lock) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	}

	error = sam_proc_inode(ip, op, NULL, CRED());

	if (write_lock) {
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	}

	/*
	 * If error, reset inode change flags.
	 * XXX - We do not handle the case -- the INODE_fsync_nowait message
	 * is successfully sent, but did not get processed on the server.
	 */
	if (error) {
		ip->cl_flags |= iflags;
	}
	return (error);
}


/*
 * ----- sam_client_lookup_name - Lookup a directory entry.
 * Given a parent directory inode and component name, search the
 * parent directory for the component name. If found, return inode
 * pointer and inode "held" (v_count incremented).
 */

#ifdef sun
int				/* ERRNO if error, 0 if successful. */
sam_client_lookup_name(
	sam_node_t *pip,	/* parent directory inode */
	char *cp,		/* component name to lookup. */
	int meta_timeo,		/* stale interval time */
	sam_node_t **ipp,	/* pointer pointer to inode (returned). */
	cred_t *credp)		/* credentials. */
{
	sam_ino_record_t *irec;
	int error;
	vnode_t *vp;

	if (S_ISDIR(pip->di.mode) == 0) {	/* Parent must be a directory */
		return (ENOTDIR);
	}
	/* Execute access required to search a directory */
	if ((error = sam_access_ino(pip, S_IEXEC, FALSE, credp))) {
		return (error);
	}
	if (*cp == '\0') {		/* Null name is this directory */
		VN_HOLD(SAM_ITOV(pip));
		*ipp = pip;
		return (0);
	}

	/*
	 * Check DNLC first.
	 */
	error = 0;
	if ((vp = dnlc_lookup(SAM_ITOV(pip), cp))) {
		sam_node_t *ip;

		/*
		 * Negative DNLC entries should only exist on the server.
		 */
		ASSERT(vp != DNLC_NO_VNODE);

		*ipp = SAM_VTOI(vp);	/* dnlc_lookup does VN_HOLD */
		ip = *ipp;
		TRACE(T_SAM_DNLC_LU, SAM_ITOV(pip), (sam_tr_t)vp, ip->di.id.ino,
		    vp->v_count);
		ASSERT(ip->flags.bits & SAM_HASH);

		/*
		 * Verify dnlc cache entry.
		 * If stale, remove from the dnlc cache.
		 */
		if (ip->di.id.ino != SAM_INO_INO) {
			if (!ip->flags.b.purge_pend) {
				error = sam_refresh_client_ino(ip,
				    meta_timeo, credp);
			} else {
				error = ENOENT;
			}
		}
		if (error) {
			dnlc_remove(SAM_ITOV(pip), cp);
			VN_RELE(vp);
		} else {
			return (0);
		}
	}

	/*
	 * Lookup the name OTW.
	 */
	irec = kmem_alloc(sizeof (*irec), KM_SLEEP);
	if ((error = sam_proc_name(pip, NAME_lookup, NULL, 0, cp, NULL,
	    credp, irec)) == 0) {
		sam_node_t *ip;

		if ((error = sam_get_client_ino(irec, pip, cp,
		    &ip, credp)) == 0) {
			*ipp = ip;
		}
	} else {
		dnlc_remove(SAM_ITOV(pip), cp);

	}
	kmem_free(irec, sizeof (*irec));
	return (error);
}
#endif /* sun */

#ifdef linux
int				/* ERRNO if error, 0 if successful. */
sam_client_lookup_name(
	sam_node_t *pip,	/* parent directory inode */
	struct qstr *cp,
	int meta_timeo,		/* stale interval time */
	sam_node_t **ipp,	/* pointer pointer to inode (returned). */
	cred_t *credp)		/* credentials. */
{
	sam_ino_record_t *irec;
	int error;

	if (S_ISDIR(pip->di.mode) == 0) {	/* Parent must be a directory */
		return (ENOTDIR);
	}

	/*
	 * Lookup the name OTW.
	 */
	irec = kmem_alloc(sizeof (*irec), KM_SLEEP);

	if (irec == NULL) {
		return (ENOMEM);
	}

	error = sam_proc_name(pip, NAME_lookup, NULL, 0, cp, NULL, credp, irec);

	if (error == 0) {
		sam_node_t *ip;

		error = sam_get_client_ino(irec, pip, cp, &ip, credp);
		if (error == 0) {
			*ipp = ip;
		}
	}
	kmem_free(irec, sizeof (*irec));
	return (error);
}
#endif /* linux */


#ifdef sun
int
sam_client_get_acl(sam_node_t *ip, cred_t *credp)
{
	sam_name_acl_t acl;
	int error;

	ASSERT(!SAM_IS_SHARED_SERVER(ip->mp));

	bzero(&acl, sizeof (acl));
	acl.set = 0;				/* get ACLs from server */
	acl.mask = VSA_DFACLCNT | VSA_ACLCNT | VSA_DFACL | VSA_ACL;
	acl.aclcnt = MAX_ACL_ENTRIES;
	acl.dfaclcnt = S_ISDIR(ip->di.mode) ? MAX_ACL_ENTRIES : 0;
	error = sam_proc_name(ip, NAME_acl, &acl, sizeof (acl),
	    NULL, NULL, credp, NULL);
	return (error);
}
#endif /* sun */


#ifdef linux
int
sam_client_get_acl(sam_node_t *ip, cred_t *credp)
{
	return (ENOTSUP);
}
#endif /* linux */


/*
 * ----- sam_refresh_client_ino -
 *	Validate shared inode when last update time + meta_timeo
 *	is greater than current time. If an interval is set (mount option,
 *	"meta_timeo"), stale information is permitted.  If validating, read
 *	the ino OTW. If the inode has been modified since the last check,
 *	invalidate pages.
 */

int
sam_refresh_client_ino(
	sam_node_t *ip,		/* Pointer to the inode */
	int meta_timeo,		/* Stale refresh interval */
	cred_t *credp)		/* credentials. */
{
	int error = 0;

	if ((ip->updtime + meta_timeo) > SAM_SECOND()) {
		return (0);
	}
	for (;;) {
		if ((error = sam_proc_inode(ip, INODE_getino, NULL, credp))) {
			if (error == ETIME) {
				if (ip->mp->mt.fi_status &
				    FS_UMOUNT_IN_PROGRESS) {
					return (EBUSY);
				}
				continue;
			}
		}
		break;
	}
	if ((error == 0) && (ip->di.nlink == 0)) {
		error = ENOENT;
	}
#ifdef linux
	if (!error) {
		/*
		 * Update the Linux inode
		 */
		ip->inode->i_mode = ip->di.mode;
		ip->inode->i_nlink = ip->di.nlink;
		ip->inode->i_uid = ip->di.uid;
		ip->inode->i_gid = ip->di.gid;
		if (S_ISLNK(ip->di.mode) && (ip->di.ext_attrs & ext_sln)) {
			rfs_i_size_write(ip->inode, ip->di.psize.symlink);
		} else {
			rfs_i_size_write(ip->inode, ip->di.rm.size);
		}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
		ip->inode->i_atime.tv_sec = ip->di.access_time.tv_sec;
		ip->inode->i_mtime.tv_sec = ip->di.modify_time.tv_sec;
		ip->inode->i_ctime.tv_sec = ip->di.change_time.tv_sec;
#else
		ip->inode->i_atime = ip->di.access_time.tv_sec;
		ip->inode->i_mtime = ip->di.modify_time.tv_sec;
		ip->inode->i_ctime = ip->di.change_time.tv_sec;
#endif
		ip->inode->i_blocks =
		    (u_longlong_t)ip->di.blocks *
		    (u_longlong_t)(SAM_BLK/DEV_BSIZE);
	}
#endif /* linux */
	return (error);
}


/*
 * ----- sam_get_client_ino - Get the current inode.
 *	Check cache for the inode returned from the server. If it in
 *  the incore inode cache, refresh this inode. If it is
 *  stale, unhash this inode and get the right i-number/gen inode.
 */

#ifdef sun
int					/* ERRNO, else 0 if successful. */
sam_get_client_ino(
	sam_ino_record_t *irec,		/* Inode instance info */
	sam_node_t *pip,		/* Pointer to the parent inode */
	char *cp,			/* Pointer to the component name */
	sam_node_t **ipp,		/* the returned inode */
	cred_t *credp)			/* credentials. */
{
	sam_node_t *ip;
	int error;

	error = sam_check_cache(&irec->di.id, pip->mp, IG_EXISTS, NULL, &ip);
	if (error == 0 && ip != NULL) {
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		error = sam_reset_client_ino(ip, 0, irec);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (error) {
			VN_RELE(SAM_ITOV(ip));	/* Decrement vnode count */
		}
	} else {
		/*
		 * If the inode.gen is not in the incore inode cache, check to
		 * see if a stale inode exists. If so, purge it from the dnlc,
		 * remove any leases, and inactivate. Note that this inode will
		 * remain on the hash list if it is still inuse which is
		 * necessary for it to be found during umount.
		 */
		error = sam_stale_hash_ino(&irec->di.id, pip->mp, &ip);
		if (ip == NULL) {
			vnode_t *vp;
			/*
			 * Get the inode.gen and then reset this inode with the
			 * inode info in irec.
			 */
			ip = (sam_node_t *)(void *)irec; /* Redo later */
			vp = SAM_ITOV(pip);
			error = sam_get_ino(vp->v_vfsp, IG_SET, &irec->di.id,
			    &ip);
		}
	}
	if (error == 0) {
		*ipp = ip;
		dnlc_update(SAM_ITOV(pip), cp, SAM_ITOV(ip));
		sam_directed_actions(ip, irec->sr_attr.actions,
		    irec->sr_attr.offset,
		    credp);
	} else {
		dnlc_remove(SAM_ITOV(pip), cp);
	}
	return (error);
}
#endif /* sun */

#ifdef linux
int					/* ERRNO, else 0 if successful. */
sam_get_client_ino(
	sam_ino_record_t *irec,		/* Inode instance info */
	sam_node_t *pip,		/* Pointer to the parent inode */
	struct qstr *cp,		/* Pointer to the component name */
	sam_node_t **ipp,		/* the returned inode */
	cred_t *credp)			/* credentials. */
{
	sam_node_t *ip;
	int error;

	/*
	 * Get the inode.gen and then reset this inode with the
	 * inode info in irec.
	 */
	ip = (sam_node_t *)(void *)irec;	/* Redo later */

	error = sam_get_ino(pip->mp->mi.m_vfsp, IG_SET, &irec->di.id, &ip);

	if (error == 0) {
		*ipp = ip;
		sam_directed_actions(ip,
		    irec->sr_attr.actions, irec->sr_attr.offset, credp);
	}
	return (error);
}
#endif /* linux */


/*
 * ----- sam_stale_hash_ino -  check for stale inode in hash chain.
 *  Check hash chain for matching inode number. If stale inode
 *  found, purge from the dnlc and remove client leases.
 */

#ifdef sun
int				/* ERRNO if error occured, 0 if successful. */
sam_stale_hash_ino(
	sam_id_t *id,		/* file i-number & generation number. */
	sam_mount_t *mp,	/* pointer to mount table. */
	sam_node_t **ipp)	/* Pointer pointer to the returned inode */
{
	sam_node_t *ip;
	int error;

	error = sam_check_cache(id, mp, IG_STALE, NULL, &ip);
	*ipp = ip;
	if (error == EBADR) {
		dnlc_purge_vp(SAM_ITOV(ip));
		if (ip->cl_leases) {
			sam_client_remove_leases(ip, ip->cl_leases, 0);
		}
		VN_RELE(SAM_ITOV(ip));	/* Decrement vnode count */
		*ipp = NULL;
	}
	return (error);
}
#endif /* sun */

/*
 * ----- sam_directed_actions - Execute directed actions.
 * Based on returned actions, switch to direct I/O or stale pages,
 * Note, switching to direct I/O also stales pages. For both
 * cases, indirect blocks may be marked stale.
 */

void
sam_directed_actions(
	sam_node_t *ip,		/* Pointer to inode. */
	ushort_t actions,	/* Directed actions */
	offset_t offset,	/* Stale indirect buffers offset */
	cred_t *credp)		/* credentials. */
{
	int need_lock;
#ifdef linux
	struct inode *li = SAM_SITOLI(ip);
	int error;
#endif /* linux */

	need_lock = RW_OWNER_OS(&ip->inode_rwl) != curthread;

	if (need_lock) {
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	}

	if (actions & (SR_DIRECTIO_ON | SR_ABR_ON)) {
		if (actions & SR_DIRECTIO_ON) {
			SAM_COUNT64(shared_client, dio_switch);
			TRACE(T_SAM_ABR_SDR, SAM_ITOP(ip), ip->di.id.ino,
			    0, actions);
			sam_set_directio(ip, DIRECTIO_ON);
		}
		if (actions & SR_ABR_ON) {
			SAM_COUNT64(shared_client, abr_switch);
			TRACE(T_SAM_ABR_DACT, SAM_ITOP(ip), ip->di.id.ino,
			    0, actions);
			SAM_SET_ABR(ip);
		}
	}

	if ((actions & (SR_INVAL_PAGES|SR_SYNC_PAGES)) &&
	    !ip->stage_seg) {
		int flags;
		boolean_t set = TRUE;

		flags = 0;
		if (S_ISDIR(ip->di.mode)) {
			flags = B_INVAL|B_TRUNC;
			if (SAM_IS_SHARED_SERVER(ip->mp)) {
				set = FALSE;
			}
		} else if (actions & SR_INVAL_PAGES) {
			flags = B_INVAL;
		}
		if (set) {
			if (flags & B_INVAL) {
				SAM_COUNT64(shared_client, inval_pages);
			} else {
				SAM_COUNT64(shared_client, sync_pages);
			}
#ifdef sun
			if (flags == 0) {
				(void) VOP_PUTPAGE_OS(SAM_ITOV(ip), 0, 0,
				    B_ASYNC, credp, NULL);
				(void) sam_wait_async_io(ip, need_lock);
			} else {
				(void) VOP_PUTPAGE_OS(SAM_ITOV(ip), 0, 0,
				    flags, credp, NULL);
			}
#endif /* sun */
#ifdef linux
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
			error = rfs_filemap_write_and_wait(li->i_mapping);
			if (flags & B_INVAL) {
				invalidate_inode_pages(li->i_mapping);
			}
#else
			error = fsync_inode_data_buffers(li);
			if (flags & B_INVAL) {
				invalidate_inode_pages(li);
			}
#endif
#endif /* linux */
		}
	}
	if (actions & SR_STALE_INDIRECT) {
		if (!SAM_IS_SHARED_SERVER(ip->mp)) {
			SAM_COUNT64(shared_client, stale_indir);
			(void) sam_stale_indirect_blocks(ip, offset);
		}
	}

	if (need_lock) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	}
}


/*
 * ----- sam_client_frlock_ino - File and record locking.
 *	Ask the server host to lock the file or record.
 */

int				/* ERRNO if error occured, 0 if successful. */
sam_client_frlock_ino(
	sam_node_t *ip,		/* Pointer to inode. */
	int cmd,		/* Command, cmd = 0 is release all locks */
	sam_flock_t *flp,	/* Pointer to flock */
	int flag,		/* flags, see file.h and flock.h */
	offset_t offset,	/* offset. */
	void *lmfcb,
	cred_t *credp)		/* credentials. */
{
	int error;
	sam_lease_data_t data;
	sam_share_flock_t flock;

	/*
	 * Flush and invalidate pages if releasing the lock,
	 * cmd == 0 means release all locks. Flush pages
	 * if setting the lock.
	 * Note: This code was modified in 4.4 to prevent a
	 * deadlock issue between the inode lock and the page
	 * lock (flushing could cause a write to occur). The
	 * flush with inval forces these pages to be written
	 * prior to lock release.
	 */
	if ((cmd == F_SETLK) || (cmd == F_SETLKW) || (cmd == 0)) {
		if (!ip->stage_seg) {
			if ((cmd == 0) || (flp->l_type == F_UNLCK)) {
				sam_flush_pages(ip, B_INVAL);
			} else {
				sam_flush_pages(ip, 0);
			}
		}
	}
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

	bzero(&flock, sizeof (flock));

	/*
	 * File/Record locking is done on the server.
	 */
	bzero(&data, sizeof (data));
	data.ltype = LTYPE_frlock;
	data.offset = offset;
	data.resid = flp->l_len;
	data.alloc_unit = 0;
	data.filemode = flag & 0xFFFF;	/* Mask off REMOTE bit */

#ifdef sun
	data.cmd = cmd;
	flock.l_type = flp->l_type;
#endif /* sun */

#ifdef linux
	switch (cmd) {
	case 0: /* release all locks */
		data.cmd = 0;
		break;
	case F_GETLK:
	case F_GETLK64:
		data.cmd = 14;
		break;
	case F_SETLK:
	case F_SETLK64:
		data.cmd = 6;
		break;
	case F_SETLKW:
	case F_SETLKW64:
		data.cmd = 7;
		break;
	default:
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		return (EINVAL);
	}

	switch (flp->l_type) {
	case F_RDLCK:
		flock.l_type = 1;
		break;
	case F_WRLCK:
		flock.l_type = 2;
		break;
	case F_UNLCK:
		flock.l_type = 3;
		break;
	default:
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		return (EINVAL);
	}
#endif /* linux */

	flock.l_whence = flp->l_whence;
	flock.l_start = flp->l_start;
	flock.l_len = flp->l_len;

	/*
	 * We have two classes of locks to deal with.
	 *
	 * Local locks are owned by processes on this machine.  For these locks,
	 * we must set the system ID and PID here.  We assign a system ID based
	 * on our ordinal (low numbers starting at 1).  NFS assigns system IDs
	 * starting at $3FFF and working down, so we are unlikely to conflict.
	 *
	 * Remote locks are owned by processes on an NFS client of this machine.
	 * For these locks, we must pass the system ID and PID through.
	 * However,
	 * since the NFS server on this machine assigns system IDs without any
	 * knowledge of NFS servers on other shared clients, it may assign
	 * system
	 * ID numbers which conflict with those servers.
	 *
	 * Ideally, we would solve this conflict by adding a third field or by
	 * extending the system ID field.  Unfortunately we can't do this; it's
	 * fixed at 32 bits.  What's worse, the upper 16 bits are reserved for
	 * the SunCluster PXFS file system, so we only have 16 bits to use.  Of
	 * those 16 bits, we lose 1 because lm_release_blocks() treats the ID
	 * as a signed 16-bit number.  We lose 1 more because the lock manager
	 * uses it internally.  So we have 14 bits, all of which are also used
	 * by NFS in its assignments.
	 *
	 * The best solution would be for the server to keep a map from
	 * <client-ordinal,client-sysid> pairs into <server-sysid> values.
	 * We don't do this, for now at least, because of the complexity.
	 * For now, we just take 7 of the bits and XOR our ordinal,
	 * bit-reversed,
	 * into them.  This does allow false matches, but only if there are more
	 * than 128 NFS clients on a particular QFS client, and there are enough
	 * QFS clients that their ordinal bits overlap with the NFS bits.
	 *
	 * Finally, if we're running in a SunCluster configuration, we don't
	 * want
	 * to use our client ordinal at all, because we want NFS locks to be
	 * associated only with their owner (and we assume that the NFS system
	 * IDs are either maintained coherently by scalable NFS or preserved via
	 * failover without multiple QFS clients being used as NFS servers).
	 */

#ifdef sun
	if (!(flag & F_REMOTELOCK)) {
		flock.l_sysid = ip->mp->ms.m_client_ord;
		flock.l_pid = ttoproc(curthread)->p_pid;
	} else {
		if (SAM_MP_IS_CLUSTERED(ip->mp)) {
			flock.l_sysid = flp->l_sysid;
		} else {
			flock.l_sysid = sam_munge_sysid(flp->l_sysid,
			    ip->mp->ms.m_client_ord);
		}
		flock.l_pid = flp->l_pid;
		if ((cmd == F_SETLK) && (flock.l_type == F_UNLCK)) {
			sam_stop_waiting_frlock(ip, &flock);
		}
	}
#endif /* sun */
#ifdef linux
	flock.l_sysid = ip->mp->ms.m_client_ord;
	flock.l_pid = curthread->pid;
#endif /* linux */

	error = sam_proc_get_lease(ip, &data, &flock, lmfcb, credp);
	if ((cmd == F_GETLK) || (cmd == F_GETLK64)) {
		/*
		 * Replace flock struct. F_GETLK returns info.
		 */
#ifdef sun
		flp->l_type = flock.l_type;
#endif /* sun */
#ifdef linux
		switch (flock.l_type) {
		case 1:
			flp->l_type = F_RDLCK;
			break;
		case 2:
			flp->l_type = F_WRLCK;
			break;
		case 3:
			flp->l_type = F_UNLCK;
			break;
		default:
			cmn_err(CE_WARN,
			    "SAM-QFS: F_GETLK unknown l_type %d\n",
			    flock.l_type);
			break;
		}
#endif /* linux */
		flp->l_whence = flock.l_whence;
		flp->l_start = flock.l_start;
		flp->l_len = flock.l_len;
		flp->l_sysid = flock.l_sysid;
		flp->l_pid = flock.l_pid;
#ifdef sun
	} else if (cmd == F_HASREMOTELOCKS) {
		l_has_rmt(flp) = flock.l_remote;
#endif
	}
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);

	/*
	 * Update frlock list of outstanding locks. This list of locks
	 * is reset by the client after a failover.
	 */
	if ((error == 0) && ((cmd == 0) || (cmd == F_SETLK) ||
	    (cmd == F_SETLKW))) {
		sam_update_frlock(ip, cmd, &flock, flag, offset);
	}

	/*
	 * If there are no locks remaining, remove any potential FRLOCK lease.
	 * XXX - Eventually we should quit treating FRLOCK as a lease at all,
	 * and track locks independently of leases.
	 */
	if (ip->cl_locks == 0) {
		sam_client_remove_leases(ip, CL_FRLOCK, 0);
	}

	TRACE(T_SAM_CL_FRLOCK_RET, SAM_ITOP(ip), flock.l_pid,
	    flock.l_sysid, error);
	return (error);
}


#ifdef sun
/*
 * ----- sam_stop_waiting_frlock -
 *	Check to see if this pid is waiting on a frlock. If so, cancel
 *  this waiting message. Wake up the thread with the error EINTR.
 */

static void
sam_stop_waiting_frlock(sam_node_t *ip, sam_share_flock_t *flockp)
{
	sam_mount_t *mp = ip->mp;
	sam_client_msg_t *clp;
	boolean_t found_waiter = FALSE;

	mutex_enter(&mp->ms.m_cl_mutex);
	clp = mp->ms.m_cl_head;

	/*
	 * Check for a waiting frlock message entry in the client request chain.
	 */
	while (clp != NULL) {
		mutex_enter(&clp->cl_msg_mutex);
		if (((clp->cl_command == SAM_CMD_WAIT) ||
		    (clp->cl_command == SAM_CMD_LEASE)) &&
		    (clp->cl_msg->hdr.operation == LEASE_get)) {
			sam_san_lease_t *lp = &clp->cl_msg->call.lease;
			int cmd;

			cmd = lp->data.cmd;
			if ((lp->data.ltype == LTYPE_frlock) &&
			    ((cmd == F_SETLK) || (cmd == F_SETLKW)) &&
			    (lp->flock.l_pid == flockp->l_pid) &&
			    (lp->flock.l_sysid == flockp->l_sysid)) {

				/*
				 * Stop waiting frlock lease command with EINTR
				 * error.
				 */
				clp->cl_error = EINTR;
				clp->cl_done = TRUE;
				found_waiter = TRUE;
			}
		}
		mutex_exit(&clp->cl_msg_mutex);
		clp = clp->cl_next;
	}
	if (found_waiter && mp->ms.m_cl_server_wait) {
		cv_broadcast(&mp->ms.m_cl_cv);
	}
	mutex_exit(&mp->ms.m_cl_mutex);
}
#endif


/*
 * ----- sam_update_frlock - File and record locking completion.
 *	Update client frlock tables.
 */

void
sam_update_frlock(
	sam_node_t *ip,			/* Pointer to inode. */
	int cmd,			/* Command. */
	sam_share_flock_t *flockp,	/* Pointer to flock */
	int flag,			/* flags, see file.h and flock.h */
	offset_t offset)		/* offset. */
{
	/*
	 * cmd == 0		remove all locks for this pid.
	 * l_type = F_UNLCK	remove all locks for this pid and record.
	 */
	if ((cmd == 0) || (flockp->l_type == F_UNLCK)) {
		sam_cl_flock_t *fptr, *fnext, *fprev;
		boolean_t record_match;

		fptr = ip->cl_flock;
		fprev = NULL;
		while (fptr) {
			fnext = fptr->cl_next;
			record_match = TRUE;
			if (cmd != 0) {
				if ((flockp->l_start != fptr->flock.l_start) ||
				    (flockp->l_len != fptr->flock.l_len)) {
					record_match = FALSE;
				}
			}
			if (record_match &&
			    (flockp->l_pid == fptr->flock.l_pid) &&
			    (flockp->l_sysid == fptr->flock.l_sysid)) {
				kmem_free(fptr, sizeof (sam_cl_flock_t));
				if ((--ip->cl_locks) < 0) {
					ip->cl_locks = 0;
				}
				if (fprev == NULL) {
					ip->cl_flock = fnext;
				} else {
					fprev->cl_next = fnext;
				}
			} else {
				fprev = fptr;
			}
			fptr = fnext;
		}
		if (ip->cl_flock == NULL) {
			ip->cl_locks = 0;
		}

	/*
	 * Add this lock. Check to make sure it does not already exist.
	 */
	} else {
		sam_cl_flock_t *fptr, *fprev;
		boolean_t set_lock;

		fptr = ip->cl_flock;
		fprev = NULL;
		set_lock = TRUE;
		while (fptr) {
			if ((flockp->l_start == fptr->flock.l_start) &&
			    (flockp->l_len == fptr->flock.l_len) &&
			    (flockp->l_pid == fptr->flock.l_pid) &&
			    (flockp->l_sysid == fptr->flock.l_sysid)) {
				set_lock = FALSE;
				break;
			}
			fprev = fptr;
			fptr = fptr->cl_next;
		}
		if (set_lock) {
			fptr = kmem_zalloc(sizeof (sam_cl_flock_t), KM_SLEEP);
			fptr->offset = offset;
			fptr->filemode = flag & 0xFFFF;
			fptr->cmd = cmd;
#ifdef sun
			fptr->nfs_lock = (flag & F_REMOTELOCK) != 0;
#endif /* sun */
			fptr->flock = *flockp;
			if (fprev == NULL) {
				ip->cl_flock = fptr;
			} else {
				fprev->cl_next = fptr;
			}
			ip->cl_locks++;
		}
	}
}


#ifdef sun
/*
 * ----- sam_munge_sysid - Compute a fake system ID based on an NFS client's
 * system ID and our client ordinal.  See comments in sam_client_frlock_ino
 * for details.  We take the client ordinal, bit-reverse it, and XOR it into
 * the NFS system ID to generate a 14-bit fake ID.
 */
static int
sam_munge_sysid(int original_sysid, int client_ord)
{
	int i;
	int reversed_ord;

	/* Really simple bit-reversal algorithm, not speed-critical. */

	reversed_ord = 0;

	for (i = 0; i < 14; i++) {
		reversed_ord |= ((client_ord & (1 << i)) != 0) << (13 - i);
	}

	return (original_sysid ^ reversed_ord);
}
#endif


/*
 * ----- sam_set_cl_attr - Set file attributes in message in client.
 * Executed on the client for a lease or inode command.
 * If not server and force_sync is TRUE, sync pages and set invalidate
 * pages. Pages are not sync'd on the allocate append unless
 * the last update is at least as old as the append lease time.
 * This is to avoid a costly pvn_vplist_dirty dirty page scan.
 * NOTE. Inode WRITER lock set on entry/exit if write_lock TRUE.
 */

void
sam_set_cl_attr(
	sam_node_t *ip,		/* Pointer to inode. */
	ushort_t actions,	/* Client directed actions */
	sam_cl_attr_t *attr,	/* Pointer to attributes in message. */
	boolean_t force_sync,	/* Flag to force inode sync */
	boolean_t write_lock)	/* TRUE if inode WRITERS lock held */
{
#ifdef linux
	struct inode *li = SAM_SITOLI(ip);
#endif /* linux */

	attr->actions = actions;			/* Directed actions */
	/* if caller has or is expiring the append lease */
	if ((ip->cl_leases | ip->cl_saved_leases) & CL_APPEND) {
		attr->actions |= SR_SET_SIZE;
	}
	attr->iflags = ip->cl_flags;	/* Inode change flags */

	if (!SAM_IS_SHARED_SERVER(ip->mp) &&
	    !(ip->mp->mt.fi_status & (FS_FREEZING | FS_FROZEN))) {
		/*
		 * If force_sync with file modified and not stage_n,
		 * sync pages to disk and ask server to set size.
		 */
		if (force_sync && ((ip->flags.bits & SAM_UPDATED) ||
		    (ip->cl_flags & SAM_UPDATED))) {
#ifdef sun
			if (vn_has_cached_data(SAM_ITOV(ip)) != 0) {
				if (!write_lock) {
					RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
				}
				(void) VOP_PUTPAGE_OS(SAM_ITOV(ip), 0, 0,
				    B_ASYNC, kcred, NULL);
				(void) sam_wait_async_io(ip, write_lock);
				if (!write_lock) {
					RW_LOCK_OS(&ip->inode_rwl, RW_READER);
				}
			}
#endif /* sun */
#ifdef linux
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
			/* cstyle */
			{
				if (!write_lock) {
					RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
					RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				}
				rfs_filemap_write_and_wait(li->i_mapping);
				if (!write_lock) {
					RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
					RW_LOCK_OS(&ip->inode_rwl, RW_READER);
				}
			}
#else /* LINUX_VERSION_CODE < 2.6.0 */
			{
				if (!write_lock) {
					RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
					RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				}
				fsync_inode_data_buffers(li);
				if (!write_lock) {
					RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
					RW_LOCK_OS(&ip->inode_rwl, RW_READER);
				}
			}
#endif /* LINUX_VERSION_CODE */
#endif /* linux */
			attr->actions |= SR_INVAL_PAGES;
			ip->cl_flags = 0;
		} else {
			/*
			 * If not syncing pages, don't set size unless our
			 * caller did the sync earlier.
			 */
			if (!(actions & SR_SET_SIZE)) {
				attr->actions &= ~SR_SET_SIZE;
			}
			ip->cl_flags &= ~(SAM_ACCESSED | SAM_CHANGED);
		}
	}
	if (!write_lock) {
		mutex_enter(&ip->fl_mutex);
	}
	if ((attr->seqno = ++ip->cl_attr_seqno) == 0) {
		attr->seqno = ip->cl_attr_seqno = 1;
	}
	attr->real_size = ip->di.rm.size;
	if (!write_lock) {
		mutex_exit(&ip->fl_mutex);
	}
}


/*
 * ----- sam_set_client - Processes the privileged set client system call.
 *	Issued by sam-sharefsd daemon.
 */

int				/* ERRNO if error, 0 if successful. */
sam_set_client(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	struct sam_set_host args;
	sam_mount_t *mp;
	int error = 0;

	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	if ((mp = sam_find_filesystem(args.fs_name)) == NULL) {
		return (ENOENT);
	}

	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EACCES;
		goto out;
	}

	if (!SAM_IS_SHARED_FS(mp)) {
		error = ENOTTY;
		goto out;
	}

	/*
	 * The SetClient call means the sam-sharefsd daemon is initializing.
	 * args.ord is the new server ordinal obtained from the label block.
	 * All clients including the new server execute this call after
	 * terminating & restarting. The updated label block is processed.
	 * args.maxord zero means this is a client and is not the new server.
	 */
	TRACES(T_SAM_SETCLIENT, mp, args.server);
	mutex_enter(&mp->ms.m_waitwr_mutex);
	strncpy(mp->mt.fi_server, args.server, sizeof (mp->mt.fi_server));

	if (args.maxord == 0) {		/* If this is a client */
		mp->mt.fi_status &= ~FS_SERVER;
		mp->mt.fi_status |= FS_CLIENT;
		mp->ms.m_prev_srvr_ord = mp->ms.m_server_ord;
		mp->ms.m_server_tags = args.server_tags;
	}
	TRACE(T_SAM_BEGCLNT, mp, mp->mt.fi_status, args.ord,
	    mp->ms.m_prev_srvr_ord);
	mp->ms.m_server_ord = args.ord;
	if (mp->mt.fi_status & FS_MOUNTING) {
		cmn_err(CE_NOTE,
		"SAM-QFS: %s: New server %s, client is not mounted (%x)",
		    mp->mt.fi_name, mp->mt.fi_server, mp->mt.fi_status);
	} else if (mp->mt.fi_status & FS_MOUNTED) {

		/*
		 * If this is a client, the server has changed, and
		 * no failover is in progress, then set
		 * freezing because this is an involuntary failover.
		 */
		if ((args.maxord == 0) &&
		    mp->ms.m_prev_srvr_ord &&
		    (mp->ms.m_prev_srvr_ord != mp->ms.m_server_ord) &&
		    !(mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING))) {
			mp->mt.fi_status &= ~FS_SRVR_DOWN;
			mp->mt.fi_status |= (FS_FREEZING | FS_LOCK_HARD);
			cmn_err(CE_NOTE,
	"SAM-QFS: %s: Begin involuntary failover, new server %s: freezing (%x)",
			    mp->mt.fi_name, mp->mt.fi_server, mp->mt.fi_status);
		}

		/*
		 * If failing over, the old server and new server have already
		 * changed to resyncing|frozen. Change the other hosts to
		 * frozen now.
		 */
		if ((mp->mt.fi_status & FS_FREEZING) &&
		    !(mp->mt.fi_status & (FS_FROZEN|FS_RESYNCING))) {
			mp->mt.fi_status &= ~FS_FREEZING;
			mp->mt.fi_status |= FS_FROZEN;
			cmn_err(CE_NOTE,
			    "SAM-QFS: %s: Failing over to new "
			    "server %s, frozen (%x)",
			    mp->mt.fi_name, mp->mt.fi_server, mp->mt.fi_status);
		}

		/*
		 * If failing over, start a task to re-establish leases.
		 * This task will start running immediately.
		 */
		cmn_err(CE_NOTE,
		    "SAM-QFS: %s: Set client: new server %s (%x), %d.%d, fl=%x",
		    mp->mt.fi_name, mp->mt.fi_server, mp->mt.fi_status,
		    mp->ms.m_client_ord, mp->ms.m_server_ord,
		    mp->mi.m_schedule_flags & SAM_SCHEDULE_TASK_THAWING);
		if (mp->mt.fi_status & (FS_FREEZING|FS_FROZEN|FS_RESYNCING)) {
			sam_taskq_add(sam_reestablish_leases, mp, NULL, 0);
		}
	}
	TRACE(T_SAM_ENDCLNT, mp, mp->mt.fi_status, args.ord, args.maxord);
	mutex_exit(&mp->ms.m_waitwr_mutex);

out:
	SAM_SYSCALL_DEC(mp, 0);
	return (error);
}


/*
 * ----- sam_byte_swap_header -
 * Byte swap a SAM-QFS OTW message header.
 */
int
sam_byte_swap_header(sam_san_header_t *hdr)
{
	return (sam_byte_swap(sam_san_header_swap_descriptor,
	    (void *)hdr, sizeof (sam_san_header_t)));
}


/*
 * The maximum op values, indexed by command type.
 * If commands are renumbered, this array needs to
 * be updated.  This array should probably go in
 * src/fs/include/share.h with the commands.
 */
/*
 * Place holder for SAM_CMD_WAIT
 * which is not an OTW op
 */
#define	WAIT_max_op	0

static int sam_maxop[] = {
	0,
	MOUNT_max_op,
	LEASE_max_op,
	NAME_max_op,
	INODE_max_op,
	BLOCK_max_op,
	WAIT_max_op,
	CALLOUT_max_op,
	NOTIFY_max_op
};


/*
 * ----- sam_byte_swap_message - Byte swap SAM-QFS OTW messages
 * When called, the header has already been converted to
 * the native format; we rely on its values to determine
 * how to convert the message body.
 *
 * Non-zero return indicates unrecognized message.
 */
static int
__sam_byte_swap_message(sam_san_message_t *msg)
{
	ushort_t command, operation, reply;
	int r = 0;

	command = msg->hdr.command;
	operation = msg->hdr.operation;
	reply = msg->hdr.originator == SAM_SHARED_SERVER;

	if (command == 0 || command >= SAM_CMD_MAX) {
		return (EPROTO);
	}
	if (operation == 0 || operation >= sam_maxop[command]) {
		return (EPROTO);
	}

/*
 * XXX - rh - several secondary byte swap calls use msg->hdr.length
 * XXX - as their length argument.  They should probably be damped
 * XXX - down to the minimum of the expected length and the sent
 * XXX - length (msg->hdr.length - offsetof(call.xxx.data, msg))
 */
	switch (command) {
	case SAM_CMD_MOUNT:
		r = sam_byte_swap(sam_san_mount_swap_descriptor,
		    &msg->call.mount, msg->hdr.length);
		break;

	case SAM_CMD_LEASE:
		if (reply) {
			r = sam_byte_swap(sam_san_lease2_swap_descriptor,
			    &msg->call.lease2, msg->hdr.length);
		} else {
			r = sam_byte_swap(sam_san_lease_swap_descriptor,
			    &msg->call.lease, msg->hdr.length);
		}
		break;

	case SAM_CMD_NAME:
		if (reply) {
			r = sam_byte_swap(sam_san_name2_swap_descriptor,
			    &msg->call.name2, msg->hdr.length);

#ifdef sun
			/*
			 * If this is a reply to a NAME_acl request, we need to
			 * byte swap the ACL header and any ACLs that came back.
			 */
			if (!r && operation == NAME_acl) {
				int acl_offset;

				r = sam_byte_swap(sam_name_acl_swap_descriptor,
				    &msg->call.name2.arg.p.acl,
				    msg->hdr.length);
				for (acl_offset = 0;
				    !r && offsetof(sam_san_name2_t,
				    component[acl_offset]) < msg->hdr.length;
				    acl_offset += sizeof (aclent_t)) {
					(void) sam_byte_swap(
					    sam_acl_swap_descriptor,
					    &msg->call.name2.component[
					    acl_offset],
					    msg->hdr.length -
					    offsetof(sam_san_name2_t,
					    component[acl_offset]));
				}
				if (r) {
					cmn_err(CE_WARN,
					    "SAM-QFS: clmisc: NAME_acl "
					    "byte swap error (1)");
				}
			}
#endif
			/*
			 * If the inode in nrec is a symlink with a short
			 * name, we just incorrectly swapped (part of) the
			 * path.  Undo it.
			 */
			if (S_ISLNK(msg->call.name2.nrec.di.mode)) {
				sam_disk_inode_t *di;

				di = (sam_disk_inode_t *)
				    &msg->call.name2.nrec.di;
				if (di->psize.symlink <= SAM_SYMLINK_SIZE &&
				    di->extent[0]) {
					sam_bswap4((void *)di->extent, NOEXT);
				}
			}
		} else {
			switch (operation) {
			case NAME_create:
				r = sam_byte_swap(
				    sam_name_create_swap_descriptor,
				    &msg->call.name, msg->hdr.length);
				break;

			case NAME_remove:
				r = sam_byte_swap(
				    sam_name_remove_swap_descriptor,
				    &msg->call.name, msg->hdr.length);
				break;

			case NAME_mkdir:
				r = sam_byte_swap(
				    sam_name_mkdir_swap_descriptor,
				    &msg->call.name, msg->hdr.length);
				break;

			case NAME_rmdir:
				r = sam_byte_swap(
				    sam_name_rmdir_swap_descriptor,
				    &msg->call.name, msg->hdr.length);
				break;

			case NAME_link:
				r = sam_byte_swap(
				    sam_name_link_swap_descriptor,
				    &msg->call.name, msg->hdr.length);
				break;

			case NAME_rename:
				r = sam_byte_swap(
				    sam_name_rename_swap_descriptor,
				    &msg->call.name, msg->hdr.length);
				break;

			case NAME_symlink:
				r = sam_byte_swap(
				    sam_name_symlink_swap_descriptor,
				    &msg->call.name, msg->hdr.length);
				break;

			case NAME_acl: {
#ifdef sun
				int acl_offset;
#endif

				r = sam_byte_swap(sam_name_acl_swap_descriptor,
				    &msg->call.name.arg.p.acl, msg->hdr.length);
#ifdef sun
				for (acl_offset = 0;
				    !r && offsetof(sam_san_name_t,
				    component[acl_offset])
				    < msg->hdr.length;
				    acl_offset += sizeof (aclent_t)) {
					(void) sam_byte_swap(
					    sam_acl_swap_descriptor,
					    &msg->call.name.component[
					    acl_offset],
					    msg->hdr.length -
					    offsetof(sam_san_name_t,
					    component[acl_offset]));
				}
				if (r) {
					cmn_err(CE_WARN,
					    "SAM-QFS: clmisc: NAME_acl "
					    "byte swap error (2)");
				}
#endif
				}
				break;

			case NAME_lookup:
				break;

			default:
				r = EPROTO;
			}
		}
		break;

	case SAM_CMD_INODE:
		switch (operation) {
		case INODE_getino:
		case INODE_fsync_wait:
		case INODE_fsync_nowait:
		case INODE_cancel_stage:
		case INODE_setabr:
			r = 0;
			break;

		case INODE_setattr:
			r = sam_byte_swap(sam_inode_setattr_swap_descriptor,
			    &msg->call.inode.arg.p.setattr, msg->hdr.length);
			break;

		case INODE_stage:
			/*
			 * Nothing needs byte swapping in this part of the
			 * message.
			 */
			break;

		case INODE_samattr:
			r = sam_byte_swap(sam_inode_samattr_swap_descriptor,
			    &msg->call.inode.arg.p.samattr, msg->hdr.length);
			break;

		case INODE_samarch:
			r = sam_byte_swap(sam_inode_samarch_swap_descriptor,
			    &msg->call.inode.arg.p.samarch, msg->hdr.length);
			break;

		case INODE_samaid:
			r = sam_byte_swap(sam_inode_samaid_swap_descriptor,
			    &msg->call.inode.arg.p.samaid, msg->hdr.length);
			break;

		case INODE_putquota:
			r = sam_byte_swap(sam_inode_quota_swap_descriptor,
			    &msg->call.inode.arg.p.quota, msg->hdr.length);
			break;

		default:
			r = EPROTO;
		}
		if (r != 0) {
			return (r);
		}
		if (reply) {
			r = sam_byte_swap(sam_ino_record_swap_descriptor,
			    &msg->call.inode2.irec, msg->hdr.length);
			/*
			 * If the inode is a symlink with a short name, we
			 * just incorrectly swapped (part of) the path.  Undo
			 * it.
			 */
			if (S_ISLNK(msg->call.inode2.irec.di.mode)) {
				sam_disk_inode_t *di;

				di = (sam_disk_inode_t *)
				    &msg->call.inode2.irec.di;
				if (di->psize.symlink <= SAM_SYMLINK_SIZE &&
				    di->extent[0]) {
					sam_bswap4((void *)di->extent, NOEXT);
				}
			}
		}
		break;

	case SAM_CMD_BLOCK:
		/*
		 * Byte swap the common sam_san_block part of the
		 *  message
		 */
		r = sam_byte_swap(sam_san_block_swap_descriptor,
		    &msg->call.block, msg->hdr.length);
		if (r) {
			cmn_err(CE_WARN,
			    "SAM-QFS: getino san_block_swap error");
			break;
		}

		switch (operation) {
		case BLOCK_getbuf:
			r = sam_byte_swap(sam_block_getbuf_swap_descriptor,
			    &msg->call.block.arg.p.getbuf,
			    msg->hdr.length);
			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: block_getbuf_swap error");
				break;
			}

			r = sam_byte_swap(sam_indirect_extent_swap_descriptor,
			    msg->call.block.data,
			    msg->hdr.length);

			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: indirect_extent_swap error");
			}

			break;

		case BLOCK_fgetbuf:
			{
				char *rec_start;
				int len = 0;
				struct sam_dirent *sdp;

				r = sam_byte_swap(
				    sam_block_fgetbuf_swap_descriptor,
				    &msg->call.block.arg.p.fgetbuf,
				    msg->hdr.length);
				if (r) {
					cmn_err(CE_WARN,
					    "SAM-QFS: block_fgetbuf swap "
					    "error");
					break;
				}
				rec_start = (char *)msg->call.block.data;
				while (len <=
				    msg->call.block.arg.p.fgetbuf.len) {
					r = sam_byte_swap(
					    sam_dirent_swap_descriptor,
					    rec_start,
					    sizeof (struct sam_dirent));
					if (r) {
						cmn_err(CE_WARN,
						    "SAM-QFS: dirent_swap "
						    "swap error");
						break;
					}

					sdp = (struct sam_dirent *)
					    (void *)rec_start;
					if (sdp->d_reclen == 0) {
						break;
					}

					len += sdp->d_reclen;
					rec_start += sdp->d_reclen;
				}

				/*
				 * Post-Demo:  Need to find the sam_dirval
				 * entry and byteswap it too.
				 */
			}

			break;

		case BLOCK_getino: {
				sam_mode_t mode;

				r = sam_byte_swap(
				    sam_block_getino_swap_descriptor,
				    &msg->call.block.arg.p.getino,
				    msg->hdr.length);
				if (r) {
					cmn_err(CE_WARN,
					    "SAM-QFS: block_getino_swap error");
					break;
				}

				/*
				 * Byte swap the mode so we can figure out if
				 * the inode is a regular or extension inode.
				 */
				bcopy(msg->call.block.data, (char *)&mode,
				    sizeof (mode));
				sam_bswap4((void *)&mode, 1);

				if (S_ISEXT(mode)) {
					struct sam_inode_ext *eip;
					/*
					 * Extension inode
					 */
					mode &= S_IFEXT;
					eip = (struct sam_inode_ext *)
					    (void *)&msg->call.block.data;

					/*
					 * Byte swap the extension inode header
					 */
					r = sam_byte_swap(
					    sam_inode_ext_hdr_swap_descriptor,
					    &eip->hdr, msg->hdr.length);
					if (r) {
						cmn_err(CE_WARN,
						    "SAM-QFS: "
						    "inode_ext_hdr_swap error");
						break;
					}

					switch (mode) {
					case S_IFSLN:
						/*
						 * Symbolic link
						 */
			r = sam_byte_swap(sam_sln_inode_swap_descriptor,
			    &eip->ext.sln,
			    msg->hdr.length);
						if (r) {
							cmn_err(CE_WARN,
							    "SAM-QFS: "
							    "sln_inode_swap "
							    "error");
						}
						break;

					case S_IFRFA:
						/*
						 * Removable media file attr
						 */
			r = sam_byte_swap(sam_rfa_inode_swap_descriptor,
			    &eip->ext.rfa,
			    msg->hdr.length);
						if (r) {
							cmn_err(CE_WARN,
							    "SAM-QFS: "
							    "rfa_inode_swap "
							    "error");
						}
						break;

					case S_IFHLP:
						/*
						 * Hard link parent
						 */
			r = sam_byte_swap(sam_hlp_inode_swap_descriptor,
			    &eip->ext.hlp,
			    msg->hdr.length);
						if (r) {
							cmn_err(CE_WARN,
							    "SAM-QFS: "
							    "hlp_inode_swap "
							    "error");
						}
						break;

					case S_IFACL:
						/*
						 * ACL
						 */
			r = sam_byte_swap(sam_acl_inode_swap_descriptor,
			    &eip->ext.acl,
			    msg->hdr.length);
						if (r) {
							cmn_err(CE_WARN,
							    "SAM-QFS: "
							    "acl_inode_swap "
							    "error");
						}
						break;

					case S_IFMVA:
						/*
						 * Multivolume archive
						 */
			r = sam_byte_swap(sam_mva_inode_swap_descriptor,
			    &eip->ext.mva,
			    msg->hdr.length);
						if (r) {
							cmn_err(CE_WARN,
							    "SAM-QFS: "
							    "mva_inode_swap "
							    "error");
						}
						break;

					default:
						r = -1;
						break;
					}

				} else {
					size_t bsize =
					    msg->call.block.arg.p.getino.bsize;

					/*
					 * Regular inode
					 */
					if (bsize <=
					    sizeof (struct sam_disk_inode)) {
			r = sam_byte_swap(sam_disk_inode_swap_descriptor,
			    msg->call.block.data,
			    bsize);
					} else {
			r = sam_byte_swap(sam_perm_inode_swap_descriptor,
			    msg->call.block.data,
			    bsize);
					}
					if (r) {
						cmn_err(CE_WARN,
						    "SAM-QFS: disk_inode "
						    "swap error");
					}
					/*
					 * But, if a symlink, we may have just
					 * swapped part of the symlink path by
					 * mistake.  Undo it.
					 */
					if (S_ISLNK(mode)) {
						sam_disk_inode_t *di;

					di = (sam_disk_inode_t *)
					    (void *)&msg->call.block.data;

						if (di->psize.symlink <=
						    SAM_SYMLINK_SIZE &&
						    di->extent[0]) {
							sam_bswap4(
							    (void *)di->extent,
							    NOEXT);
						}
					}
				}
			}
			break;

		case BLOCK_getsblk:
			r = sam_byte_swap(sam_block_sblk_swap_descriptor,
			    &msg->call.block.arg.p.sblk, msg->hdr.length);
			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: block_sblk swap error");
				break;
			}

			r = byte_swap_sb((struct sam_sblk *)
			    (void *)msg->call.block.data,
			    msg->hdr.length - offsetof(struct sam_san_block,
			    data) -
			    sizeof (msg->call.block.pad));

			break;

		case BLOCK_vfsstat:
			r = sam_byte_swap(sam_block_vfsstat_swap_descriptor,
			    &msg->call.block.arg.p.vfsstat, msg->hdr.length);

			break;

		case BLOCK_vfsstat_v2:
			r = sam_byte_swap(sam_block_vfsstat_v2_swap_descriptor,
			    &msg->call.block.arg.p.vfsstat_v2, msg->hdr.length);

			break;

		case BLOCK_quota:
			r = sam_byte_swap(sam_block_quota_swap_descriptor,
			    &msg->call.block.arg.p.samquota, msg->hdr.length);
			if (r) {
				break;
			}
			r = sam_byte_swap(sam_dquot_swap_descriptor,
			    &msg->call.block.data, msg->hdr.length);
			break;

		default:
			r = EPROTO;
		}
		break;

	case SAM_CMD_CALLOUT:
		r = sam_byte_swap(sam_id_swap_descriptor,
		    &msg->call.callout.id, msg->hdr.length);
		if (r != 0) {
			return (r);
		}
		switch (operation) {
		case CALLOUT_action:
			break;

		case CALLOUT_stage:
			r = sam_byte_swap(sam_callout_stage_swap_descriptor,
			    &msg->call.callout.arg.p.stage, msg->hdr.length);
			break;

		case CALLOUT_acl:
			break;

		case CALLOUT_flags:
			break;

		case CALLOUT_relinquish_lease:
			r = sam_byte_swap(
			    sam_callout_relinquish_lease_swap_descriptor,
			    &msg->call.callout.arg.p.relinquish_lease,
			    msg->hdr.length);
			break;

		default:
			r = EPROTO;
		}

		if (r != 0) {
			return (r);
		}

		r = sam_byte_swap(sam_ino_record_swap_descriptor,
		    &msg->call.callout.irec, msg->hdr.length);

		break;

	case SAM_CMD_NOTIFY:
		r = sam_byte_swap(sam_san_notify_swap_descriptor,
		    &msg->call.notify, msg->hdr.length);
		if (r != 0) {
			return (r);
		}
		switch (operation) {
		case NOTIFY_lease:
		case NOTIFY_lease_expire:
			r = sam_byte_swap(sam_notify_lease_swap_descriptor,
			    &msg->call.notify.arg.p.lease, msg->hdr.length);
			break;

		case NOTIFY_dnlc:
			r = sam_byte_swap(sam_notify_dnlc_swap_descriptor,
			    &msg->call.notify.arg.p.dnlc, msg->hdr.length);
			break;

		case NOTIFY_getino:
		case NOTIFY_hostoff:
			break;

		default:
			r = EPROTO;
		}
		break;

	default:
		r = EPROTO;
	}
	return (r);
}

#ifdef sun
int
sam_byte_swap_message(mblk_t *mbp)
{
	sam_san_message_t *msg;

	msg = (sam_san_message_t *)(void *)mbp->b_rptr;
	return (__sam_byte_swap_message(msg));
}
#endif

#ifdef linux
int
sam_byte_swap_message(struct sk_buff *mbp)
{
	sam_san_message_t *msg;

	msg = (sam_san_message_t *)(void *)mbp->data;
	return (__sam_byte_swap_message(msg));
}
#endif


/*
 * Placeholder for eventual idle/restart commands to re-host
 * a shared SAM-QFS filesystem.
 */
int			/* ERRNO if error, 0 if successful. */
sam_sys_shareops(void *arg, int size, cred_t *credp, int *rvp)
{
	int error = 0;
	sam_shareops_arg_t args;
	sam_mount_t *mp;
#ifdef METADATA_SERVER
	client_entry_t *clp;
	int ord, r = 0;
#endif

	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	if ((mp = sam_find_filesystem(args.fsname)) == NULL) {
		return (ENOENT);
	}
	SAM_SHARE_DAEMON_HOLD_FS(mp);
	SAM_SYSCALL_DEC(mp, 0);

	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EACCES;
	} else {
		switch (args.op) {
		case SHARE_OP_WAKE_SHAREDAEMON:
			sam_wake_sharedaemon(mp, EINTR);
			break;

		case SHARE_OP_INVOL_FAILOVER:
			/*
			 * Sent by samsharefs command to indicate that the
			 * pending failover is "involuntary" and the failover
			 * can be accelerated if all hosts except the previous
			 * server have checked in.
			 */
			mp->ms.m_involuntary = 1;
			break;

		case SHARE_OP_AWAIT_WAKEUP:
			/*
			 * Called by the FS share daemon to await any events of
			 * that it should know about, e.g., timeout from old
			 * server,
			 * rewrite of hosts file, etc..
			 *
			 * If set, mp->mi.m_shared_event is an errno to be
			 * returned
			 * to sam-sharefsd.  Otherwise, a signal returns EINTR.
			 */
			mutex_enter(&mp->ms.m_shared_lock);
			while (!mp->ms.m_shared_event) {
				if (!cv_wait_sig(&mp->ms.m_shared_cv,
				    &mp->ms.m_shared_lock)) {
					if (!mp->ms.m_shared_event) {
						mp->ms.m_shared_event = EINTR;
					}
				}
			}
			error = mp->ms.m_shared_event;
			mp->ms.m_shared_event = 0;

			mutex_exit(&mp->ms.m_shared_lock);
			TRACE(T_SAM_DAE_WOKE, mp, mp->mt.fi_status, error, 0);
			break;

		case SHARE_OP_AWAIT_FAILDONE:
			/*
			 * Block until the pending failover has completed.
			 */
			error = ENOTSUP;
			break;

#ifdef METADATA_SERVER
		case SHARE_OP_HOST_INOP:
			/*
			 * The client host is DOWN.  Don't wait around for
			 * it to connect.
			 */
			if (args.host <= 0 ||
			    args.host >= sam_max_shared_hosts) {
				error = EINVAL;	/* host # out-of-range */
				break;
			}

			mutex_enter(&mp->ms.m_cl_wrmutex);
			if (!mp->ms.m_clienti) {
				sam_init_client_array(mp);
			}
			clp = sam_get_client_entry(mp, args.host, 1);

			if (clp == NULL) {
				error = EINVAL;
				mutex_exit(&mp->ms.m_cl_wrmutex);
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_sys_shareops client "
				    "entry allocation failed for ordinal %d",
				    mp->mt.fi_name, args.host);
				break;
			}

			clp->cl_flags |= SAM_CLIENT_INOP;
			mutex_exit(&mp->ms.m_cl_wrmutex);

			break;

		case SHARE_OP_CL_MOUNTED:
			/*
			 * If this host isn't the metadata server or isn't
			 * mounted, return error.  Otherwise, return the number
			 * of clients that seem to be mounted.
			 */
			error = EBUSY;
			if ((mp->mt.fi_status & FS_SERVER) == 0) {
				error = EINVAL;
				break;
			}
			if ((mp->mt.fi_status & FS_MOUNTED) == 0) {
				break;
			}
			if (mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING)) {
				break;
			}
			for (ord = 1;
			    ord <= mp->ms.m_max_clients; ord++) {

				clp = sam_get_client_entry(mp, ord, 0);

				if ((clp != NULL) &&
				    (ord != mp->ms.m_client_ord) &&
				    (clp->cl_status & FS_MOUNTED) &&
				    (lbolt - clp->cl_msg_time)/hz <
				    3*SAM_MIN_DELAY) {
					r++;
				}
			}
			error = 0;
			*rvp = r;
			break;
#endif /* METADATA_SERVER */

		default:
			error = EINVAL;
		}
	}
	SAM_SHARE_DAEMON_RELE_FS(mp);
	return (error);
}

#ifdef linux
/*
 * ----- sam_iecache_find - Find the in-core inode indirect extent cache entry
 * identified by ord, blkno, and bsize.  If found, return a pointer to it.
 * Otherwise, return NULL.
 */
sam_iecache_t *
sam_iecache_find(sam_node_t *ip, int ord, unsigned long long blkno,
    long bsize)
{
	sam_iecache_t *iecachep;

	if (ip == NULL) {
		return (NULL);
	}
	for (iecachep = ip->cl_iecachep; iecachep; iecachep = iecachep->nextp) {
		if ((iecachep->ord == ord) && (iecachep->blkno == blkno) &&
		    (iecachep->bsize >= bsize) &&
		    (iecachep->bufp != NULL)) {
			break;
		}
	}
	return (iecachep);
}


/*
 * ----- sam_iecache_update - Update in-core inode indirect extent cache
 * with the entry identified by ord/blkno.  If it doesn't already exist,
 * insert it at the cache list head.
 */
int
sam_iecache_update(sam_node_t *ip, int ord, unsigned long long blkno,
    long bsize, char *bufp)
{
	sam_iecache_t *iecachep;

	if ((ip == NULL) || (ord < 0) || (bsize <= 0)) {
		return (EINVAL);
	}

	mutex_enter(&ip->cl_iec_mutex);

	/* Check if already exists. */
	if ((iecachep = sam_iecache_find(ip, ord, blkno, bsize)) != NULL) {
		/* If size isn't the same, re-allocate */
		if (iecachep->bsize != bsize) {
			kmem_free(iecachep->bufp, iecachep->bsize);
			iecachep->bufp = NULL;
		}
	} else {
		iecachep = kmem_zalloc(sizeof (sam_iecache_t), KM_SLEEP);

		/* Insert new cache entry at head. */
		iecachep->nextp = ip->cl_iecachep;
		ip->cl_iecachep = iecachep;
	}
	if (iecachep->bufp == NULL) {
		iecachep->bufp = kmem_zalloc(bsize, KM_SLEEP);
		iecachep->ord = ord;
		iecachep->bsize = bsize;
		iecachep->blkno = blkno;
	}
	memcpy(iecachep->bufp, bufp, bsize);

	mutex_exit(&ip->cl_iec_mutex);

	return (0);
}


/*
 * ----- sam_iecache_clear - Clear in-core inode indirect extent cache.
 */
int
sam_iecache_clear(sam_node_t *ip)
{
	sam_iecache_t *iecachep;

	if (ip == NULL) {
		return (EINVAL);
	}

	mutex_enter(&ip->cl_iec_mutex);
	iecachep = ip->cl_iecachep;
	ip->cl_iecachep = NULL;
	mutex_exit(&ip->cl_iec_mutex);
	while (iecachep) {
		sam_iecache_t *nextp = iecachep->nextp;

		if (iecachep->bufp) {
			kmem_free(iecachep->bufp, iecachep->bsize);
		}

		kmem_free(iecachep, sizeof (sam_iecache_t));
		iecachep = nextp;
	}

	return (0);
}

/*
 * ----- sam_iecache_ent_clear - Clear in-core inode indirect extent cache
 * entry.
 */
int
sam_iecache_ent_clear(sam_node_t *ip, uchar_t ord, uint32_t blkno)
{
	sam_iecache_t *iecachep;
	sam_iecache_t *prevp = NULL;

	if (ip == NULL) {
		return (EINVAL);
	}

	mutex_enter(&ip->cl_iec_mutex);
	iecachep = ip->cl_iecachep;
	while (iecachep) {
		sam_iecache_t *nextp = iecachep->nextp;

		if ((iecachep->ord == ord) && (iecachep->blkno == blkno)) {
			if (prevp == NULL) {
				ip->cl_iecachep = nextp;
			} else {
				prevp->nextp = nextp;
			}
			if (iecachep->bufp) {
				kmem_free(iecachep->bufp, iecachep->bsize);
			}

			kmem_free(iecachep, sizeof (sam_iecache_t));
			break;
		}
		prevp = iecachep;
		iecachep = nextp;
	}
	mutex_exit(&ip->cl_iec_mutex);

	return (0);
}

#endif /* linux */

/*
 * The following functions are used to help with tracking VFS holds
 * on shared filesystems.  We want to keep a SAM_VFS_HOLD() on the
 * FS for each vnode associated with the filesystem, and we want
 * an extra one for any sam-sharefsd in the kernel (so that we don't
 * free the mount structure until it exits).  But the vfs_t typically
 * doesn't exist for the filesystem until long after sam-sharefsd
 * has started.
 *
 * So there is some rigmarole here.  We set a 'deferred hold' flag
 * on the first share daemon entry.  When an unmount starts, if the
 * deferred hold field is set, a hold is issued and the deferred
 * hold field is cleared.  Thus the mount structure is never freed
 * prior to sam-sharefsd's exit.
 */

/*
 *	---- sam_share_daemon_hold_fs
 *
 * Increment the 'in-use-socket' count for a shared FS.  If
 * this is the first thread entering, save that fact in the
 * mount structure's deferred hold field.
 */
void
sam_share_daemon_hold_fs(sam_mount_t *mp)
{
	mutex_enter(&mp->ms.m_shared_lock);
	TRACE(T_SAM_DAE_HOLD, mp, mp->ms.m_cl_nsocks, mp->ms.m_cl_dfhold, 0);
	if (mp->ms.m_cl_nsocks++ == 0) {
		ASSERT(!mp->ms.m_cl_dfhold);
		mp->ms.m_cl_dfhold = TRUE;
	}
	mutex_exit(&mp->ms.m_shared_lock);
}


/*
 *	---- sam_share_daemon_rele_fs
 *
 * Decrement the 'in-use-socket' count for a shared FS.  If
 * this is the last thread exiting, either clear the deferred
 * hold field (if it's set), or issue the SAM_VFS_RELE for the
 * (non-deferred) hold.
 */
void
sam_share_daemon_rele_fs(sam_mount_t *mp)
{
	boolean_t rele = FALSE;

	mutex_enter(&mp->ms.m_shared_lock);
	TRACE(T_SAM_DAE_RELE, mp, mp->ms.m_cl_nsocks, mp->ms.m_cl_dfhold, 0);
	ASSERT(mp->ms.m_cl_nsocks > 0);
	if (--mp->ms.m_cl_nsocks == 0) {
		rele = !mp->ms.m_cl_dfhold;
		mp->ms.m_cl_dfhold = FALSE;
	}
	mutex_exit(&mp->ms.m_shared_lock);
	if (rele) {
		SAM_VFS_RELE(mp);
	}
}


/*
 *	---- sam_share_daemon_cnvt_fs
 *
 * If there's a deferred hold on the FS, convert it to a real
 * SAM_VFS_HOLD.  Should only be done once we're committed to
 * an unclean, forced unmount.  We mustn't leave a real hold
 * on a cleanly unmounted FS.
 */
void
sam_share_daemon_cnvt_fs(sam_mount_t *mp)
{
	boolean_t hold;

	ASSERT(SAM_IS_SHARED_FS(mp));
	ASSERT(mp->mt.fi_status & FS_MOUNTED);

	mutex_enter(&mp->ms.m_shared_lock);
	TRACE(T_SAM_DAE_CNVT, mp, mp->ms.m_cl_nsocks,
	    (int)mp->ms.m_cl_dfhold, 0);
	hold = mp->ms.m_cl_dfhold;
	mp->ms.m_cl_dfhold = FALSE;
	mutex_exit(&mp->ms.m_shared_lock);
	if (hold) {
		ASSERT(mp->mi.m_vfsp);
		SAM_VFS_HOLD(mp);
	}
}
