/*
 * ----- truncate.c - Process the sam truncate function.
 *
 * Processes the truncate functions supported on the SAM File System.
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

#pragma ident "$Revision: 1.151 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/cmn_err.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/flock.h>
#include <sys/fs_subr.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <vm/pvn.h>


/* ----- SAMFS Includes */

#include <sam/types.h>

#include "macros.h"
#include "inode.h"
#include "mount.h"
#include "quota.h"
#include "ino_ext.h"
#include "rwio.h"
#include "indirect.h"
#include "extern.h"
#include "trace.h"
#include "debug.h"
#include "qfs_log.h"
#include "copy_extents.h"
#include "arfind.h"


static void sam_delete_archive(sam_node_t *ip);
static int sam_reduce_ino(sam_node_t *ip, offset_t length,
	sam_truncate_t tflag);
static int sam_free_xattr_group(sam_node_t *pip, caddr_t base, ssize_t len,
	cred_t *credp);
static int sam_free_xattr_tree(sam_node_t *ip, cred_t *credp);

extern int sam_map_truncate(sam_node_t *ip, offset_t length, cred_t *credp);


/*
 * ----- sam_drop_ino - drop the disk space.
 * Truncate file and then mark the file offline.
 * NOTE. inode_rwl lock set on entry and exit.
 */

int				/* ERRNO if error, 0 if successful. */
sam_drop_ino(sam_node_t *ip, cred_t *credp)
{
	int copies = 0;
	int num_vcopies = 0;
	int error = 0;
	offset_t min_size = 0;
	int copy, mask;

	if (ip->di.rm.size == 0) {
		return (0);
	}
	if (ip->di.status.b.bof_online) {
		if (ip->di.status.b.pextents == 0) {
			min_size = 0;
		} else {
			min_size = sam_partial_size(ip);
		}
	}

	if (ip->di.rm.ui.flags & RM_DATA_VERIFY) {
		int mask = 1;

		for (copy = 0; copy < MAX_ARCHIVE; copy++, mask += mask) {
			if (ip->di.arch_status & mask) {
				copies++;
			}
			if (ip->di.ar_flags[copy] & AR_verified &&
			    !(ip->di.ar_flags[copy] & AR_damaged)) {
				num_vcopies++;
			}
		}
	}
	/*
	 * File must not be staging, have at least one good archive copy,
	 * and have some data in it. All archive copies must be verified
	 * for a file with the verify attribute.
	 */

	for (copy = 0, mask = 1; copy < MAX_ARCHIVE; copy++, mask += mask) {
		if ((ip->di.arch_status & mask) &&
		    ((ip->di.ar_flags[copy] & AR_damaged) == 0)) {
			break;
		}
	}
	if (copy == MAX_ARCHIVE) {
		if (ip->di.status.b.offline) {
			ip->di.status.b.damaged = 1;
			error = EDOM;	/* Offline with no archive copies */
		} else {
			error = 0;
		}
	} else if (ip->flags.b.staging || ip->flags.b.hold_blocks) {
		/* staging or in use by SANergy? */
		error = EBUSY;
	} else if (ip->mp->mt.fi_mflag & MS_RDONLY) {
		error = EROFS;
	} else if ((ip->di.rm.ui.flags & RM_DATA_VERIFY) &&
	    (num_vcopies != copies || copies == 1 ||
	    !ip->di.status.b.archdone)) {
			error = EACCES;
	} else if (ip->di.rm.size > min_size) {
		if ((error = sam_truncate_ino(ip, min_size,
		    SAM_RELEASE, credp)) == 0) {
			if (ip->stage_err == 0) {
				time_t residence_time;

				residence_time = SAM_SECOND();
				ip->di.residence_time = residence_time;
				if (S_ISSEGS(&ip->di)) {
					ip->segment_ip->di.residence_time =
					    residence_time;
					ip->segment_ip->flags.b.changed = 1;
				}
			}
			ip->flags.b.changed = 1;

			if (!ip->di.status.b.offline) {
				(void) sam_quota_foffline(ip->mp, ip, NULL);
			}
			ip->di.status.b.offline = 1;
			if (ip->di.status.b.bof_online == 0) {
				ip->di.status.b.pextents = 0;
			}
			ip->stage_off = 0;
			ip->stage_len = 0;
			ip->real_stage_off = 0;
			ip->real_stage_len = 0;
			if (ip->di.blocks == 0 &&
			    (ip->di.rm.size > SM_OFF(ip->mp, DD))) {
				ip->di.status.b.on_large = 1;
			}
			if (ip->mp->ms.m_fsev_buf) {
				sam_send_event(ip->mp, &ip->di, ev_offline,
				    0, 0, ip->di.residence_time);
			}
		} else if (error == ENOCSI) {	/* Corrupted file */
			ip->di.status.b.damaged = 1;
			ip->flags.b.changed = 1;
		}
	}
	return (error);
}


/*
 * ----- sam_proc_truncate - process file truncate.
 * If length <= filesize, truncate inode by deleting the blocks in the file.
 * If length > filesize, expand the file by allocating blocks.
 * NOTE. inode_rwl lock set on entry and exit.
 * The inode data lock is set on entry.
 */

/* ARGSUSED3 */
int				/* ERRNO if error, 0 if successful. */
sam_proc_truncate(
	sam_node_t *ip,		/* pointer to inode. */
	offset_t length,	/* new length for file. */
	sam_truncate_t tflag,	/* Truncate file or Release file */
	cred_t *credp)		/* credentials pointer. */
{
	offset_t size;
	vnode_t *vp;
	mode_t mode;
	int error = 0;

	/*
	 *	Note: we have to get these values now in case we call
	 *	sam_delete_archive(), which will clear the offline status.
	 */
	offset_t ndkblks = D2QBLKS(ip->di.blocks);
	offset_t ntotblks = TOTBLKS(ip);

	vp = SAM_ITOV(ip);
	size = ip->di.rm.size;
	mode = ip->di.mode;

	TRACE(T_SAM_TRUNCATE, SAM_ITOP(ip), ip->di.id.ino, (sam_tr_t)size,
	    (sam_tr_t)length);
	ASSERT(RW_WRITE_HELD(&ip->inode_rwl));

	/*
	 * Pipes can only be purged.
	 */
	if (S_ISFIFO(mode)) {
		if (tflag != SAM_PURGE) {
			return (0);
		}
		sam_quota_fret(ip->mp, ip);
		return (sam_sync_inode(ip, 0, tflag));
	}

	/*
	 * Specials
	 */
	if (S_ISCHR(mode) || S_ISBLK(mode)) {
		if (tflag == SAM_PURGE) {
			sam_quota_fret(ip->mp, ip);
		}
		return (sam_sync_inode(ip, 0, tflag));
	}

	if (tflag == SAM_PURGE) {
		if (ip->di.ext_id.ino) {	/* Clear inode extensions */
			if (ip->di.ext_attrs & ext_sln) {
				sam_free_inode_ext(ip, S_IFSLN, SAM_ALL_COPIES,
				    &ip->di.ext_id);
				ip->di.ext_attrs &= ~ext_sln;
				bzero((char *)&ip->di.extent[0],
				    SAM_SYMLINK_SIZE);
				ip->di.psize.symlink = 0;
			}
			if (ip->di.ext_attrs & ext_rfa) {
				sam_free_inode_ext(ip, S_IFRFA, SAM_ALL_COPIES,
				    &ip->di.ext_id);
				ip->di.ext_attrs &= ~ext_rfa;
				ip->di.psize.rmfile = 0;
			}
			if (ip->di.ext_attrs & ext_hlp) {
				sam_free_inode_ext(ip, S_IFHLP, SAM_ALL_COPIES,
				    &ip->di.ext_id);
				ip->di.ext_attrs &= ~ext_hlp;
			}
			if (ip->di.ext_attrs & ext_acl) {
				sam_free_inode_ext(ip, S_IFACL, SAM_ALL_COPIES,
				    &ip->di.ext_id);
				ip->di.ext_attrs &= ~ext_acl;
			}
		}
		if (SAM_INODE_HAS_XATTR(ip)) {
			error = sam_free_xattr_tree(ip, credp);
		}
		sam_delete_archive(ip);
		if (!S_ISSEGS(&ip->di)) {
			sam_quota_fret(ip->mp, ip);
		}
	}

	/*
	 * Symlinks truncate only to 0.
	 */
	if (S_ISLNK(mode)) {
		if (length != 0) {
			return (EINVAL);
		}
		if (ip->di.version >= SAM_INODE_VERS_2) {
			if (ip->di.ext_attrs & ext_sln) {
				/*
				 * Symlink stored as inode exts.
				 */
				sam_free_inode_ext(ip, S_IFSLN, SAM_ALL_COPIES,
				    &ip->di.ext_id);
				ip->di.ext_attrs &= ~ext_sln;
				bzero((char *)&ip->di.extent[0],
				    SAM_SYMLINK_SIZE);
				ip->di.psize.symlink = 0;
			}
			return (sam_sync_inode(ip, 0, tflag));
		}
	}

	if (S_ISREG(mode) || S_ISDIR(mode) || S_ISLNK(mode) || S_ISREQ(mode)) {
		if (length != 0) {
			if (length < ip->size) {	/* If shrinking */
				/*
				 * Flush pages so we don't get sparse block
				 * putapage errors for blocks we are removing.
				 */
				if (vn_has_cached_data(vp) != 0) {
					(void) pvn_vplist_dirty(vp,
					    (sam_u_offset_t)length,
					    sam_putapage, (B_INVAL | B_TRUNC),
					    CRED());
					if (ip->koffset >= length) {
						/*
						 * Clear writebehind for pages
						 * just tossed.
						 */
						ip->koffset = 0;
						ip->klength = 0;
					}
				}
				ip->size = length;
			}

			if ((length != size) &&
			    (!S_ISSEGI(&ip->di))) {
				while (((error = sam_map_truncate(ip,
				    length, credp)) != 0) &&
				    IS_SAM_ENOSPC(error)) {
					ip->size = size;
					ip->di.rm.size = size;
					RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
					error = sam_wait_space(ip, error);
					RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
					if (error) {
						break;
					}
				}
			}

			/*
			 * sam_map_block sets file size (ip->di.rm.size) to
			 * length.  Reset for an error or for a release.
			 */
			if (error) {
				ip->size = size;
				ip->di.rm.size = size;
			} else {
				ip->size = length;
				if (tflag == SAM_RELEASE) {
					ip->di.rm.size = size;
				} else {
					ip->di.rm.size = length;
				}
			}
		}

		if (error != 0) {
			goto out;
		}

		if (length <= size) {
			if ((length == 0) && (vn_has_cached_data(vp) != 0)) {
				(void) pvn_vplist_dirty(vp,
				    (sam_u_offset_t)length,
				    sam_putapage, (B_INVAL | B_TRUNC), CRED());
			}
			if (tflag == SAM_PURGE && (ndkblks != 0LL ||
			    ntotblks != 0LL)) {
				sam_quota_bret(ip->mp, ip, ndkblks, ntotblks);
			}
			if (SAM_IS_OBJECT_FILE(ip)) {
				error = sam_truncate_object_file(ip, tflag,
				    size, length);
			} else {
				error = sam_reduce_ino(ip, length, tflag);
			}
			if (error == 0) {
				ip->size = length;  /* Current size */
			}
			ndkblks = ndkblks - D2QBLKS(ip->di.blocks);
			ntotblks = ntotblks - D2QBLKS(ip->di.blocks);
			switch (tflag) {
			case SAM_TRUNCATE:
			case SAM_REDUCE:
				if (ndkblks != 0LL || ntotblks != 0LL) {
					sam_quota_bret(ip->mp, ip, ndkblks,
					    ntotblks);
				}
				break;
			case SAM_RELEASE:
				if (ndkblks != 0) {
					sam_quota_bret(ip->mp, ip, ndkblks,
					    (offset_t)0);
				}
				break;
			case SAM_PURGE:
			default:
				break;
			}
		} else {
			if (SAM_IS_OBJECT_FILE(ip)) {
				error = sam_truncate_object_file(ip, tflag,
				    size, length);
			}
		}
	} else {
		if (length != ip->di.rm.size) {
			error = EINVAL;
		}
	}

	/*
	 * Clear write map cache info and reset allocated size.
	 */
out:
	sam_clear_map_cache(ip);
	ip->cl_allocsz = length;
	return (error);
}


/*
 * ----- sam_reduce_ino - Free extents.
 * Clear the extents to be released and then sync the inode. If
 * successful, move extents to be released to an entry in the release
 * link list and signal the release thread.
 */

static int				/* ERRNO if error, 0 if successful. */
sam_reduce_ino(
	sam_node_t *ip,			/* inode entry */
	offset_t length,		/* new length for file. */
	sam_truncate_t tflag)		/* Truncate file or Release file */
{
	sam_mount_t *mp;
	int i;
	int bt, dt;
	int ileft;
	int kptr[NIEXT + (NIEXT-1)];
	int de;
	int error = 0;

	mp = ip->mp;

	/*
	 * If the file's being used for SAN operations, hold blocks
	 * until we're told it's OK to free them.
	 */
	if (ip->flags.b.hold_blocks) {
		if (tflag != SAM_RELEASE && tflag != SAM_REDUCE) {
			ip->di.rm.size = length;
			if (length > ip->maxalloc) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_reduce_ino: ino %d.%d, "
				    " new length (%lld) > maximum (%lld)",
				    mp->mt.fi_name, ip->di.id.ino,
				    ip->di.id.gen,
				    length, ip->maxalloc);
				ip->maxalloc = length;
			}
		}
		return (0);
	}

	de = -1;
	for (i = 0; i < (NIEXT+2); i++) {
		kptr[i] = -1;
	}
	if ((ip->di.status.b.direct_map == 0) && length != 0) {
		struct sam_get_extent_args arg;
		struct sam_get_extent_rval rval;

		arg.dau = &mp->mi.m_dau[0];
		arg.num_group = mp->mi.m_fs[ip->di.unit].num_group;
		arg.sblk_vers = mp->mi.m_sblk_version;
		arg.status = ip->di.status;
		arg.offset = (offset_t)(length - 1);
		rval.de = &de;
		rval.dtype = &dt;
		rval.btype = &bt;
		rval.ileft = &ileft;
		if ((error = sam_get_extent(&arg, kptr, &rval))) {
			return (error);
		}
	}
	if (de < NDEXT) {
		offset_t size = ip->di.rm.size;
		int segblocks = 0;
		int offline = ip->di.status.b.offline;
		int blocks = ip->di.blocks;
		uchar_t lextent = ip->di.lextent;
		sam_rel_blks_t *rlp;

		rlp = (sam_rel_blks_t *)kmem_zalloc(sizeof (sam_rel_blks_t),
		    KM_SLEEP);
		bcopy((char *)&ip->di.extent[0], (char *)&rlp->e.extent[0],
		    sizeof (sam_extents_t));
		rlp->status = ip->di.status;
		rlp->id = ip->di.id;
		rlp->parent_id = ip->di.parent_id;
		rlp->length = length;
		rlp->imode = ip->di.mode;
		ip->di.blocks = 0;
		if (S_ISSEGS(&ip->di)) {
			segblocks = ip->segment_ip->di.blocks;
			ip->segment_ip->di.blocks -= blocks;
		}

		if (ip->di.status.b.direct_map) {	/* Direct map */
			if (ip->di.extent[1] == 0) {
				kmem_free((char *)rlp, sizeof (*rlp));
				return (0);
			}
			if (length != 0) {
				/* Do not truncate preallocated extent. */
				rlp->e.extent[0] = 0;
				rlp->e.extent[1] = 0;
			} else {
				ip->di.lextent = 0;
				ip->di.extent[0] = 0;
				ip->di.extent[1] = 0;
				ip->di.extent_ord[0] = 0;
				ip->di.blocks = 0;
				ip->di.status.b.direct_map = 0;
			}

		} else {	/* Indirect map */
			for (i = (de + 1); i < NOEXT; i++) {
				ip->di.extent[i] = 0;
				ip->di.extent_ord[i] = 0;
			}
			for (i = 0; i <= de; i++) {
				if (ip->di.extent[i]) {
	/* N.B. Bad indentation here to meet cstyle requirements */
	int32_t blocks;

	dt = ip->di.status.b.meta;
	bt = LG;
	if (i < NSDEXT) {
		bt = ip->di.status.b.on_large ? LG : SM;
	}
	blocks = mp->mi.m_fs[ip->di.unit].num_group == 1 ?
	    mp->mi.m_dau[dt].blocks[bt] :
	    (mp->mi.m_dau[dt].blocks[bt] * mp->mi.m_fs[ip->di.unit].num_group);
	ip->di.blocks += blocks;
	if (S_ISSEGS(&ip->di)) {
		ip->segment_ip->di.blocks += blocks;
	}
				}
			}
		}

		/*
		 * Sync the inode to make sure the blocks are not in the inode
		 * on disk. If error syncing inode, restore blocks and previous
		 * state.
		 */
		if ((error = sam_sync_inode(ip, length, tflag))) {
			if (tflag == SAM_PURGE) {
				kmem_free((char *)rlp, sizeof (*rlp));
				return (EIO);
			}

			bcopy((char *)&rlp->e.extent[0],
			    (char *)&ip->di.extent[0],
			    sizeof (sam_extents_t));
			ip->di.lextent = lextent;
			ip->di.blocks = blocks;
			if (S_ISSEGS(&ip->di)) {
				ip->segment_ip->di.blocks = segblocks;
			}
			ip->di.status.b.offline = offline;
			ip->di.rm.size = size;
			kmem_free((char *)rlp, sizeof (*rlp));
			return (EIO);
		}

		/*
		 * Forward chain the free block request packet and signal
		 * the reclaim thread if it is sleeping.
		 */
		mutex_enter(&mp->mi.m_inode.mutex);
		rlp->next = mp->mi.m_next;
		mp->mi.m_next = rlp;
		mutex_exit(&mp->mi.m_inode.mutex);
		mutex_enter(&mp->mi.m_inode.put_mutex);
		if (mp->mi.m_inode.put_wait) {
			cv_signal(&mp->mi.m_inode.put_cv);
		}
		mutex_exit(&mp->mi.m_inode.put_mutex);

	} else {
		sam_ib_arg_t	args;

		args.cmd = SAM_FREE_BLOCK;
		ASSERT(de < NOEXT);
		for (i = (NOEXT - 1); i >= de; i--) {
			int set = 0;
			int kptr_index;

			if (ip->di.extent[i] == 0) {
				continue;
			}
			if (i == NDEXT) {
				set = 1;	/* First indirect block */
			}
			if (i == de) {
				/*
				 * The current index in the top level indirect
				 * block matches the index where the new offset
				 * resides.  The values in the kptr array
				 * starting at 0 are those returned by
				 * sam_get_extent. Blocks with an index greater
				 * than the value in kptr at each level will be
				 * released. No indirect blocks down this tree
				 * will be release since the new offset is down
				 * this tree.
				 */
				kptr_index = 0;
			} else {
				/*
				 * Deleting all blocks down this indirect tree,
				 * this is indicated by values of -1 in the kptr
				 * array starting at kptr_index.
				 */
				kptr_index = NIEXT - 1;
			}
			error = sam_proc_indirect_block(ip, args, kptr,
			    kptr_index, &ip->di.extent[i],
			    &ip->di.extent_ord[i], (i - NDEXT), &set);
			if (error == ECANCELED) {
				error = 0;
				if (kptr_index <= 0) {
					break;
				}
			}
		}
	}
	return (error);
}


/*
 * ----- sam_sync_inode - Sync inode
 * Based on the tflag,
 *   SAM_TRUNCATE	- truncate the file to length & stale archive copies.
 *   SAM_REDUCE		- reduce extra pre-allocated data blocks (for shared).
 *   SAM_RELEASE		- release the file and make it offline.
 *   SAM_PURGE		- purge and clear the inode.
 * Synchronously write the inode to disk before continuing.
 */

int					/* ERRNO if error, 0 if successful. */
sam_sync_inode(
	sam_node_t *ip,			/* inode entry */
	offset_t length,		/* new length for file. */
	sam_truncate_t tflag)		/* Truncate file or Release file */
{
	enum sam_sync_type sync_type;

	if (length == 0) {
		ip->di.stride = 0;	/* Reset stride for BOF */
	}

	switch (tflag) {

	case SAM_TRUNCATE:
		ip->di.rm.size = length;
		if (S_ISSEGI(&ip->di)) {
			ip->di.rm.info.dk.seg.fsize = 0;
			ip->di.blocks = 0;
		}
		TRANS_INODE(ip->mp, ip);
		sam_mark_ino(ip, (SAM_UPDATED | SAM_CHANGED));
		sync_type = SAM_SYNC_ONE;
		break;

	case SAM_REDUCE:
		sync_type = SAM_SYNC_ONE;
		break;

	case SAM_RELEASE:
		if (!ip->di.status.b.offline) {
			(void) sam_quota_foffline(ip->mp, ip, NULL);
		}
		ip->di.status.b.offline = 1;
		sync_type = SAM_SYNC_ONE;
		break;

	case SAM_PURGE: {
		sam_id_t id;

		id = ip->di.id;
		bzero((char *)&ip->di, sizeof (struct sam_disk_inode));
		ip->di.id.gen = ++id.gen;
		ip->di.id.ino = id.ino;
		sync_type = SAM_SYNC_PURGE;
		}
		break;
	}

	/*
	 * Retry sync at least 2 times.
	 */
	ip->flags.b.changed = 1;
	if (sam_update_inode(ip, sync_type, FALSE)) {
		if (sam_update_inode(ip, SAM_SYNC_ONE, FALSE)) {
			return (EIO);
		}
	}

	/*
	 * Reset free_ino pointer in inode cache. May be reassigned from cache.
	 */
	if (tflag == SAM_PURGE) {
		ip->di.free_ino = 0;
	}
	return (0);
}


/*
 * ----- sam_proc_indirect_block - Free/Find indirect extents.
 * Free - Recusively call this routine until all indirect blocks are freed.
 * Find - Recusively call this routine until one indirect block is found
 * that matches given ord.
 */

int				/* ERRNO if error, 0 if successful. */
sam_proc_indirect_block(
	sam_node_t *ip,		/* inode entry */
	sam_ib_arg_t args,	/* Function arguments -- free or find */
	int kptr[],		/* array of extents that end the file. */
	int ik,			/* kptr index */
	uint32_t *extent_bn,	/* mass storage extent block number */
	uchar_t *extent_ord,	/* mass storage extent ordinal */
	int level,		/* level of indirection */
	int *set)
{
	int i, dt;
	buf_t *bp;
	sam_mount_t *mp;
	sam_daddr_t bn;
	sam_indirect_extent_t *iep;
	uint32_t *ie_addr;
	uchar_t *ie_ord;
	int reset = 0;
	int error = 0;
	int bwrite_indirect = 0;

	mp = ip->mp;
	bn = *extent_bn;
	bn <<= mp->mi.m_bn_shift;
	error = SAM_BREAD(mp, mp->mi.m_fs[*extent_ord].dev, bn, SAM_INDBLKSZ,
	    &bp);
	if (error) {
		return (error);
	}
	bp->b_flags &= ~B_DELWRI;
	iep = (sam_indirect_extent_t *)(void *)bp->b_un.b_addr;
	if ((error = sam_validate_indirect_block(ip, iep, level))) {
		sam_req_ifsck(mp, -1, "sam_proc_indirect_block: validate",
		    &ip->di.id);
		brelse(bp);
		SAMFS_PANIC_INO(mp->mt.fi_name,
		    "Invalid indirect block", ip->di.id.ino);
		return (ENOCSI);
	}
	if ((level == 1) && *set) {
		/*
		 * At a level 1 indirect below a level 2 (triple indirect tree).
		 * Need to clear set but save the fact that it was set so it
		 * can be reset later when the index at this level matches the
		 * one in kptr.
		 */
		reset = 1;
		*set = 0;
	}
	for (i = (DEXT - 1); i >= 0; i--) {
		if (iep->extent[i] == 0) {
			continue;		/* If empty */
		}
		ie_addr = &iep->extent[i];
		ie_ord = &iep->extent_ord[i];
		if (level) {
			if ((ik == 0) && (i <= kptr[ik])) {
				/*
				 * At the top indirect of the tree (ik == 0)
				 * that contains the new offset and the index
				 * matches the value in kptr for this level (i
				 * <= kptr[ik]).
				 */
				*set = 1;
			} else if (reset && (i <= kptr[ik])) {
				/*
				 * At a level 1 below a level 2 (triple indirect
				 * tree) that contains the new offset (reset)
				 * and the index matches the one in kptr for
				 * this level (i <= kptr[ik]).  Restore set so
				 * it will be passed to the level 0.
				 */
				*set = 1;
				reset = 0;
			}
			error = sam_proc_indirect_block(ip, args, kptr,
			    (ik + 1), ie_addr, ie_ord, (level - 1), set);
			if (error == ECANCELED) {
				break;
			}
		} else {
			int32_t blocks;

			if (*set && (i <= kptr[ik])) {
				/*
				 * Done, we have reached the index at level 0
				 * that contains the new offset. This error
				 * is passed up the tree from here and indicates
				 * that all blocks that need to be released have
				 * been released.
				 */
				error = ECANCELED;
				break;
			}
			if (args.cmd == SAM_FREE_BLOCK) {
				/*
				 * Free large data block. If directory, may
				 * be meta block.
				 */
				ip->flags.b.changed = 1;
				sam_free_block(mp, LG, *ie_addr, *ie_ord);
				*ie_addr = 0;
				*ie_ord = 0;
				dt = ip->di.status.b.meta;
				blocks = mp->mi.m_fs[ip->di.unit].num_group ==
				    1 ? LG_BLOCK(mp, dt): (LG_BLOCK(mp, dt) *
				    mp->mi.m_fs[ip->di.unit].num_group);
				ip->di.blocks -= blocks;
				if (S_ISSEGS(&ip->di)) {
					ip->segment_ip->di.blocks -= blocks;
				}
			} else if (args.cmd == SAM_FIND_ORD) {
				if (*ie_ord == args.ord) {
					error = ECANCELED;
					break;
				}
			} else if (args.cmd == SAM_MOVE_ORD) {
				if (*ie_ord == args.ord) {
					sam_bn_t obn, nbn;
					int oord, nord;
					int dt;
					/*
					 * Copy a direct extent data block and
					 * update the indirect that
					 * references it.
					 */
					dt = ip->di.status.b.meta;
					if (args.new_ord >= 0) {
						ip->di.unit = args.new_ord;
					}
					oord = *ie_ord;
					obn = *ie_addr;
					error = sam_allocate_and_copy_extent(ip,
					    LG, dt, obn, oord, NULL,
					    &nbn, &nord, NULL);
					if (error == 0) {
						*ie_addr = nbn;
						*ie_ord = nord;

						sam_free_block(mp, LG,
						    obn, oord);
						bwrite_indirect = 1;
					}
				}
			}
		}
	}
	if (args.cmd == SAM_FREE_BLOCK) {
		if (error != ECANCELED) {
			bp->b_flags |= B_STALE|B_AGE;
			brelse(bp);
			ip->flags.b.changed = 1;
			sam_free_block(mp, LG, *extent_bn, *extent_ord);
			*extent_bn = 0;
			*extent_ord = 0;
		} else {
			bdwrite(bp);
		}
	} else if (args.cmd == SAM_FIND_ORD) {
		if (*extent_ord == args.ord) {
			error = ECANCELED;
		}
		bp->b_flags |= B_STALE|B_AGE;
		brelse(bp);
	} else if (args.cmd == SAM_MOVE_ORD) {
		if (*extent_ord == args.ord) {
			sam_bn_t obn, nbn;
			int oord, nord;
			int dt;
			/*
			 * Copy indirect extent and
			 * update the indirect that references it.
			 */
			if (args.new_ord >= 0) {
				mp->mi.m_inodir->di.unit = args.new_ord;
			}
			obn = *extent_bn;
			oord = *extent_ord;
			dt = mp->mi.m_inodir->di.status.b.meta;
			error = sam_allocate_and_copy_bp(mp->mi.m_inodir,
			    LG, dt, obn, oord, bp, &nbn, &nord, NULL);

			if (error == 0) {
				*extent_bn = nbn;
				*extent_ord = nord;
				sam_free_block(mp, LG, obn, oord);
			}
		}
		if (bwrite_indirect) {
			bwrite(bp);
		} else if (bp) {
			brelse(bp);
		}
	}
	return (error);
}


/*
 * ----- sam_space_ino - Truncate/expand storage space.
 * For cmd F_FREESP, truncate/expand space.
 * l_whence indicates where the relative offset is measured:
 *   0 - the start of the file.
 *   1 - the current position.
 *   2 - the end of the file.
 * l_start is the offset from the position specified in l_whence.
 * l_len is the size to be freed. l_len = 0 frees up to the end of the file.
 */

int				/* ERRNO if error occured, 0 if successful. */
sam_space_ino(
	sam_node_t *ip,		/* inode entry */
	sam_flock_t *flp,	/* Pointer to flock. */
	int flag)		/* FNDELAY or FNONBLOCK flag */
{
	int lock_flag;
	int start;
	int error = 0;
	vnode_t *vp;

	vp = SAM_ITOV(ip);
	TRACE(T_SAM_SPACE, vp, 0, flp->l_start, flp->l_len);
	if (flp->l_len != 0) {
		return (EINVAL);
	}
	if (ip->di.rm.size == flp->l_start) {
		return (0);
	}
	if (MANDLOCK(vp, ip->di.mode)) {
		start = flp->l_start;
		if (ip->di.rm.size < flp->l_start) {	/* Expand case */
			flp->l_start = ip->di.rm.size;
		}
		flp->l_type = F_WRLCK;
		flp->l_sysid = 0;
		flp->l_pid = ttoproc(curthread)->p_pid;
		lock_flag = (flag & (FNDELAY | FNONBLOCK)) ? 0 : SLPFLCK;
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		error = reclock(vp, flp, lock_flag, 0, flp->l_start, 0);
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (error != 0 || flp->l_type != F_UNLCK) {
			return (error ? error : EAGAIN);
		}
		flp->l_start = start;
	}
	return (error);
}


/*
 * ----- sam_delete_archive - free archive inodes & information.
 * Clear removable media information in the inode. Remove any archive inodes
 * and clear the archive status. Clear damaged and offline status bits.
 */
static void			/* ERRNO if error, 0 if successful. */
sam_delete_archive(sam_node_t *ip)
{
	struct sam_perm_inode *permip;
	buf_t *bp;
	int copy;
	sam_id_t id[MAX_ARCHIVE];

	if (sam_read_ino(ip->mp, ip->di.id.ino, &bp, &permip) == 0) {
		if (ip->di.version == SAM_INODE_VERS_1) { /* Previous version */
			sam_perm_inode_v1_t *permip_v1 =
			    (sam_perm_inode_v1_t *)permip;

			for (copy = 0; copy < MAX_ARCHIVE; copy++) {
				id[copy] = permip_v1->aid[copy];
				permip_v1->aid[copy].ino = 0;
				permip_v1->aid[copy].gen = 0;
			}
		}
		bzero((char *)&permip->ar, sizeof (struct sam_arch_inode));
		for (copy = 0; copy < MAX_ARCHIVE; copy++) {
			permip->di.media[copy] = ip->di.media[copy] = 0;
			permip->di.ar_flags[copy] = ip->di.ar_flags[copy] = 0;
		}
		permip->di.arch_status = ip->di.arch_status = 0;
		permip->di.status.b.archdone = ip->di.status.b.archdone = 0;
		permip->di.status.b.damaged = ip->di.status.b.damaged = 0;
		permip->di.status.b.offline = ip->di.status.b.offline = 0;

		if (TRANS_ISTRANS(ip->mp)) {
			TRANS_WRITE_DISK_INODE(ip->mp, bp, permip, ip->di.id);
		} else {
			bdwrite(bp);
		}
		if (ip->di.version >= SAM_INODE_VERS_2) {
			if (ip->di.ext_attrs & ext_mva) {
				sam_free_inode_ext(ip, S_IFMVA, SAM_ALL_COPIES,
				    &ip->di.ext_id);
				ip->di.ext_attrs &= ~ext_mva;
			}
		} else if (ip->di.version == SAM_INODE_VERS_1) {
			/* Prev version */
			for (copy = 0; copy < MAX_ARCHIVE; copy++) {
				if (id[copy].ino) {
					sam_free_inode_ext(ip, S_IFMVA,
					    copy, &id[copy]);
				}
			}
		}
	}
}


/*
 * ----- sam_free_xattr_group - Given a len-sized chunk of entries,
 * call VOP_REMOVE for each one (except . and ..).
 */
static int
sam_free_xattr_group(
	sam_node_t *pip,
	caddr_t base,
	ssize_t len,
	cred_t *credp)
{
	ssize_t offset = 0;
	sam_dirent_t *dp;
	int error = 0;

	while (offset < len) {
		dp = (sam_dirent_t *)(void *)(base + offset);
		if (!IS_DOT_OR_DOTDOT(dp->d_name)) {
#if defined(SOL_511_ABOVE)
			error = VOP_REMOVE(SAM_ITOV(pip),
			    (char *)dp->d_name, credp, NULL, 0);
#else
			error = VOP_REMOVE(SAM_ITOV(pip),
			    (char *)dp->d_name, credp);
#endif
			if (error) {
				break;
			}
		}
		offset += SAM_DIRSIZ(dp);
	}
	return (error);
}


/*
 * ----- sam_free_xattr_tree - We're removing an object with extended
 * attributes.  Walk the xattr directory and remove each attribute.
 */
static int
sam_free_xattr_tree(
	sam_node_t *pip,
	cred_t *credp)
{
	int error;
	sam_node_t *ip;
	uio_t uio;
	iovec_t iov;
	int eof;
	size_t alloc_len;
	caddr_t bsave;

	ASSERT(RW_OWNER_OS(&pip->data_rwl) == curthread);
	ASSERT(RW_OWNER_OS(&pip->inode_rwl) == curthread);
	ASSERT(SAM_INODE_HAS_XATTR(pip));
	ASSERT(!SAM_INODE_IS_XATTR(pip));

	TRACE(T_SAM_XA_FREE, SAM_ITOV(pip), pip->di.id.ino, pip->di.id.gen, 0);

	/*
	 * Get the unnamed xattr directory.
	 */
	error = sam_get_ino(SAM_ITOV(pip)->v_vfsp, IG_EXISTS,
	    &pip->di2.xattr_id, &ip);
	if (error) {
		TRACE(T_SAM_XA_FREE_RET, SAM_ITOV(pip), pip->di.id.ino,
		    pip->di.id.gen, error);
		return (error);
	}

	alloc_len = DIR_BLK;
	bsave = iov.iov_base = (caddr_t)kmem_alloc(alloc_len, KM_SLEEP);
	iov.iov_len = alloc_len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_loffset = 0;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_fmode = FREAD;
	uio.uio_limit = 0;
	uio.uio_resid = alloc_len;

	eof = 0;

	RW_LOCK_OS(&ip->data_rwl, RW_WRITER);
	while (!eof) {
		error = sam_getdents(ip, &uio, credp, &eof, FMT_SAM);
		if (!error) {
			error = sam_free_xattr_group(ip, bsave,
			    alloc_len - uio.uio_resid, credp);
		}
		iov.iov_base = bsave;
		iov.iov_len = alloc_len;
		uio.uio_resid = alloc_len;
	}
	RW_UNLOCK_OS(&ip->data_rwl, RW_WRITER);

	kmem_free(bsave, alloc_len);

	if (!error) {
		pip->di2.xattr_id.ino = 0;
		pip->di2.xattr_id.gen = 0;
		TRANS_INODE(pip->mp, pip);
		sam_mark_ino(pip, (SAM_UPDATED | SAM_CHANGED));

		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		ASSERT(sam_empty_dir(ip) != ENOTEMPTY);
		ip->di.nlink = 0;
		TRANS_INODE(ip->mp, ip);
		sam_mark_ino(ip, (SAM_UPDATED | SAM_CHANGED));
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

		sam_send_to_arfind(ip, AE_remove, 0);
		sam_inactive_ino(ip, credp);
	} else {
		VN_RELE(SAM_ITOV(ip));
	}

	TRACE(T_SAM_XA_FREE_RET, SAM_ITOV(pip), pip->di.id.ino,
	    pip->di.id.gen, error);
	return (error);
}
