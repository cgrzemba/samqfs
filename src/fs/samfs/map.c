/*
 * ----- map.c - Process the map functions.
 *
 * Process the SAM-QFS inode map functions.
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
#pragma ident "$Revision: 1.149 $"
#endif

#include "sam/osversion.h"

/* ----- UNIX Includes */

#ifdef sun
#include <sys/types.h>
#include <sys/param.h>
#include <sys/t_lock.h>
#include <sys/sysmacros.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/cred.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/fbuf.h>
#include <sys/rwlock.h>
#include <sys/stat.h>

#include <vm/hat.h>
#include <vm/page.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_map.h>
#endif /* sun */

#ifdef  linux
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/version.h>

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/file.h>
#include <linux/uio.h>
#include <linux/dirent.h>
#include <linux/proc_fs.h>
#include <linux/limits.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/fs.h>

#include <asm/param.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#include <asm/statfs.h>
#endif
#endif  /* linux */


/* ----- SAMFS Includes */

#include "sam/param.h"
#include "sam/types.h"

#include "macros.h"
#include "debug.h"
#include "sblk.h"
#include "inode.h"
#include "mount.h"
#include "syslogerr.h"
#include "indirect.h"
#include "trace.h"
#ifdef sun
#include "extern.h"
#endif /* sun */
#ifdef linux
#include "clextern.h"
#endif /* linux */

static int sam_map_direct(sam_node_t *ip, offset_t offset, offset_t count,
	sam_map_t flag, struct sam_ioblk *iop);
static int sam_map_indirect(sam_node_t *ip, offset_t offset, offset_t count,
	sam_map_t flag, struct sam_ioblk *iop, cred_t *credp);
static int sam_wait_inode(sam_node_t *ip, sam_map_t flag);
static void sam_flush_map(struct sam_mount *mp, map_params_t *mapp);
static int sam_get_block(sam_node_t *ip, offset_t offset, sam_map_t flag,
	sam_ioblk_t *iop, map_params_t *mapp);
static int sam_set_blk_off(sam_node_t *ip, offset_t offset, sam_map_t flag,
	sam_ioblk_t *iop, map_params_t *mapp);
static int sam_clear_append(sam_node_t *ip, offset_t offset, offset_t count,
	sam_map_t flag, sam_ioblk_t *ioblk, map_params_t *mapp);
static void sam_set_ioblk(sam_node_t *ip, offset_t count,
	sam_map_t flag, sam_ioblk_t *iop, map_params_t *mapp);
static int sam_contig(int maxcontig, uint32_t *bnp, uchar_t *eip, int lblocks,
	int ext_to_1k, int ileft);
#ifdef sun
int sam_fbzero(sam_node_t *ip, offset_t offset, int tlen,
	sam_ioblk_t *iop, map_params_t *mapp, struct fbuf **fbp);
#endif /* defined sun */
#ifdef linux
void sam_linux_fbzero(sam_node_t *, offset_t, int);
#endif

#define	rw_lock(l, flag)	if ((flag <= SAM_RD_DIRECT_IO) ||	\
						(flag == SAM_WR_DIRECT_IO)) { \
					RW_LOCK_OS((l), RW_READER);	\
				} else {				\
					RW_LOCK_OS((l), RW_WRITER);	\
				}

#define	rw_unlock(l, flag)	if ((flag <= SAM_RD_DIRECT_IO) ||	\
						(flag == SAM_WR_DIRECT_IO)) { \
					RW_UNLOCK_OS((l), RW_READER);	\
				} else {				\
					RW_UNLOCK_OS((l), RW_WRITER);	\
				}

#ifdef linux
#define	sam_req_ifsck(a, b, c, d)
#endif /* linux */


/*
 * ----- sam_map_block - Map logical block to physical block.
 *
 *  For the given inode and logical byte address: if iop is set, return
 *  the I/O descriptor. If iop is NULL, allocate up to the requested byte
 *  count.
 */

int				/* ERRNO, 0 if successful. */
sam_map_block(
	sam_node_t *ip,		/* Pointer to the inode. */
	offset_t offset,	/* Logical byte offset in file (longlong_t). */
	offset_t count,		/* Requested byte count. */
	sam_map_t flag,		/* SAM_READ if reading. */
				/* SAM_READ_PUT if read with rounded up */
				/*   filesize, return contiguous blocks. */
				/* SAM_RD_DIRECT_IO if read directio */
				/* SAM_WRITE if writing. */
				/* SAM_WR_DIRECT_IO if write directio */
				/*   (READERS lock held ) */
				/* SAM_FORCEFAULT if write and fault */
				/*   for page. */
				/* SAM_WRITE_MMAP if memory mapped write. */
				/* SAM_WRITE_BLOCK if get block */
				/* SAM_WRITE_SPARSE if sparse block */
				/* SAM_ALLOC_BLOCK allocate and don't */
				/*   update size. */
				/* SAM_ALLOC_BITMAP allocate and build bit */
				/*   map (srvr). */
				/* SAM_ALLOC_ZERO allocate and zero. */
	sam_ioblk_t *iop,	/* Ioblk array. */
	cred_t *credp)		/* Creds, for allocating any blocks needed */
{
	int out;
	int end;
	map_params_t params;
	sam_ioblk_t ioblk;
	ushort_t sam_nowait;
	int error;
#ifdef sun
	offset_t prev_filesize = ip->size;
#endif

	sam_nowait = flag & SAM_MAP_NOWAIT;
	flag &= ~SAM_MAP_NOWAIT;

	if (SAM_IS_OBJECT_FILE(ip)) {
		error = sam_map_osd(ip, offset, count, flag, iop);
		return (error);
	}

start:
	ioblk.count = 0;
	ioblk.no_daus = 0;
	if (iop) {		/* ioblk initialization done here */
		iop->count = 0;
		iop->no_daus = 0;
		iop->zerodau[0] = 0;
		iop->zerodau[1] = 0;
	}
	if (ip->di.status.b.direct_map) {
		error = sam_map_direct(ip, offset, count, flag, iop);
		if (error == ENOTCONN || error == EPIPE) {
			if (sam_nowait == 0) {
				if ((error = sam_wait_inode(ip, flag)) == 0) {
					goto start;
				}
			}
		}
		return (error);
	} else if (flag == SAM_READ_PUT) {
		/*
		 * For READ_PUT, search the maps to get an accurate contig
		 * value.
		 */
		error = sam_map_indirect(ip, offset, count, flag, iop, credp);
		if (error == ENOTCONN || error == EPIPE) {
			if (sam_nowait == 0) {
				if ((error = sam_wait_inode(ip, flag)) == 0) {
					goto start;
				}
			}
		}
		return (error);
	}

	/*
	 * Check if requested block is in the incore inode cache map buffer.
	 */
	mutex_enter(&ip->iom.map_mutex);
	out = ip->iom.map_out;
	end = out - 1;
	if (end < 0)  end = SAM_MAX_IOM - 1;

	while (out != end) {
		offset_t diff;

		if ((ip->iom.imap[out].flags == M_VALID) &&
		    (offset >= ip->iom.imap[out].blk_off0) &&
		    ((offset + count) <= (ip->iom.imap[out].blk_off0 +
		    ip->iom.imap[out].contig0))) {

			TRACE(T_SAM_QKMAP, SAM_ITOP(ip), (sam_tr_t)offset,
			    (sam_tr_t)count, (sam_tr_t)flag);
			ioblk.imap = ip->iom.imap[out];
			ip->iom.map_out = out;
			mutex_exit(&ip->iom.map_mutex);

			bzero(&params, sizeof (params));
			ioblk.ord = ioblk.imap.ord0;
			if (sam_set_blk_off(ip, offset, flag, &ioblk,
			    &params) == 0) {
				return (0);
			}

			diff = ioblk.blk_off - ioblk.imap.blk_off0;
			ioblk.imap.blk_off0 = ioblk.blk_off;
			ioblk.contig = ioblk.imap.contig0 -= diff;
			params.size_left = ioblk.contig - ioblk.pboff;
			error = sam_clear_append(ip, offset, count, flag,
			    &ioblk, &params);
			if (error != 0) {
				if (error == ENOTCONN || error == EPIPE) {
					if (sam_nowait == 0) {
						if ((error = sam_wait_inode(ip,
						    flag)) == 0) {
							goto start;
						}
					}
				}
				return (error);
			}

			if (iop) {
				/*
				 * Fill in ioblk if requested.
				 */
				if (ioblk.num_group > 1) {
					diff /= ioblk.num_group;
				}
				ioblk.blkno = ioblk.imap.blkno0 +=
				    diff >> SAM_DEV_BSHIFT;
				sam_set_ioblk(ip, count, flag, &ioblk, &params);
				*iop = ioblk;
				iop->no_daus = 0;
				iop->zerodau[0] = 0;
				iop->zerodau[1] = 0;
			}
#ifdef sun
			if ((offset > prev_filesize) && (flag == SAM_WRITE)) {
				/*
				 * Zero the DAUs between prev_filesize and
				 * offset.
				 */
				sam_zero_sparse_blocks(ip, prev_filesize,
				    offset);
			}
#endif
			return (0);
		}
		if (++out >= SAM_MAX_IOM)  out = 0;
	}
	mutex_exit(&ip->iom.map_mutex);

	error = sam_map_indirect(ip, offset, count, flag, iop, credp);
	if (error == ENOTCONN || error == EPIPE) {
		if (sam_nowait == 0) {
			if ((error = sam_wait_inode(ip, flag)) == 0) {
				goto start;
			}
		}
	}
	return (error);
}


/*
 * ----- sam_wait_inode - Wait for the failover to complete.
 */

static int			/* ERRNO, 0 if successful. */
sam_wait_inode(
	sam_node_t *ip,		/* Pointer to the inode. */
	sam_map_t flag)		/* flag if reading or writing. */
{
	int error;

	rw_unlock(&ip->inode_rwl, flag);
	error = sam_stop_inode(ip);
	rw_lock(&ip->inode_rwl, flag);
	return (error);
}


/*
 * ----- sam_clear_map_cache - Clear the map flags for the cache.
 *
 * Clear write map cache info.
 */

void
sam_clear_map_cache(sam_node_t *ip)
{
	int i;

	if (SAM_IS_OBJECT_FILE(ip)) {
		return;
	}
	mutex_enter(&ip->iom.map_mutex);
	ip->iom.map_in = ip->iom.map_out = 0;
	for (i = 0; i < SAM_MAX_IOM; i++) {
		ip->iom.imap[i].flags = 0;
	}
	mutex_exit(&ip->iom.map_mutex);
}


/*
 * ----- sam_map_direct - Map logical block to physical block.
 *
 *  This function assumes direct blocks.
 *  The extents consists of up to 8 block direct entries.
 *  The entry consists of starting block offset and length
 *  in number of blocks.
 */

static int			/* ERRNO, 0 if successful. */
sam_map_direct(
	sam_node_t *ip,		/* Pointer to the inode. */
	offset_t offset,	/* Logical byte offset in file (longlong_t). */
	offset_t count,		/* Requested byte count. */
	sam_map_t flag,		/* SAM_READ if reading. */
				/* SAM_READ_PUT if read with rounded up */
				/*   filesize, return contiguous blocks. */
				/* SAM_RD_DIRECT_IO if read directio */
				/* SAM_WRITE if writing. */
				/* SAM_WR_DIRECT_IO if write directio */
				/*   (READERS lock held). */
				/* SAM_FORCEFAULT if write and fault for */
				/*   page. */
				/* SAM_WRITE_MMAP if memory mapped write. */
				/* SAM_WRITE_BLOCK if get block */
				/* SAM_WRITE_SPARSE if sparse block */
				/* SAM_ALLOC_BLOCK allocate and don't */
				/*   update size. */
				/* SAM_ALLOC_BITMAP allocate and build bit */
				/*   map (srvr). */
				/* SAM_ALLOC_ZERO allocate and zero. */
	sam_ioblk_t *iop)	/* Ioblk array. */
{
	int dt;
	sam_mount_t *mp;
	uint32_t *bnp, *blp;
	uchar_t *eip;
	uchar_t ioflags = 0;
	int pboff;			/* Offset in block */
	offset_t blk_off;	/* Block offset (always on DAU boundaries) */
	offset_t size, dau_size;
	int num_group, bsize;
	int error = 0;

	ASSERT(!SAM_IS_OBJECT_FILE(ip));

	/*
	 * Build ioblk array given offset and count.
	 */
	mp = ip->mp;
	dt = ip->di.status.b.meta;
	bsize = LG_BLK(mp, dt);
	bnp = &ip->di.extent[0];
	blp = bnp + 1;
	eip = &ip->di.extent_ord[0];
	if ((flag == SAM_READ_PUT) && (*bnp == 0))  ioflags = M_SPARSE;
	num_group = mp->mi.m_fs[ip->di.unit].num_group;
	TRACE(T_SAM_DDMAP, SAM_ITOP(ip), (sam_tr_t)offset,
	    (sam_tr_t)count, flag);

	/*
	 * Allocate if writing and no blocks assigned.
	 */
#ifdef sun
	if (flag >= SAM_WRITE && (*bnp == 0)) {	/* if not allocated */
		sam_prealloc_t pa;
		boolean_t alloc_block = TRUE;

		if (flag == SAM_WR_DIRECT_IO) {
			RW_UPGRADE_OS(&ip->inode_rwl);
			if (*bnp) {
				alloc_block = FALSE;
			}
		}
		if (alloc_block) {
			/*
			 * Setup preallocation request, signal block thread, and
			 * wait for completion. There is no interruption out of
			 * this request.  Future - if user interrupt, must
			 * return blocks.
			 */
			dau_size = bsize;
			if (num_group > 1) {
				dau_size *= num_group;
			}

			pa.length = count;
			pa.count = howmany(pa.length, dau_size);
			pa.dt = (short)dt;
			pa.ord = ip->di.unit;

			if ((error = sam_prealloc_blocks(ip->mp, &pa)) == 0) {
				ip->di.blocks = (pa.count0 * dau_size) >>
				    SAM_SHIFT;
				/*
				 * Logical blocks in 1 member.
				 */
				*blp = pa.count0 * LG_DEV_BLOCK(mp, dt);
				*bnp = pa.first_bn >> ip->mp->mi.m_bn_shift;
				*eip = (uchar_t)pa.ord;
				if (pa.count0 != 0) {
					ip->di.lextent = 1;
				}
			}
		}
		if (flag == SAM_WR_DIRECT_IO) {
			RW_DOWNGRADE_OS(&ip->inode_rwl);
		}
	}
#endif /* sun */

	/*
	 * Assume on_large for direct extents.
	 */
	dau_size = bsize;
	if (num_group > 1)  dau_size *= num_group;
	blk_off = (offset / dau_size) * dau_size;
	pboff = offset % dau_size;

	/*
	 * How much room is left in this dau?
	 */
	size = (offset_t)*blp;
	if (num_group > 1)  size *= num_group;
	size = (size << SAM_DEV_BSHIFT) - (blk_off + (offset_t)pboff);
	if (flag == SAM_READ) {		/* If reading, do not allow past EOF */
		offset_t rem;

		ASSERT(RW_LOCK_HELD(&ip->inode_rwl));
		rem = ip->size - offset;
		if (rem < 0)  rem = 0;
		if (rem < size)  size = rem;
		if (size == 0) {
			TRACE(T_SAM_DDMAP1, SAM_ITOP(ip), (sam_tr_t)offset,
			    (sam_tr_t)ip->size, pboff);
			return (0);
		}
	}

#ifdef sun
	if (flag >= SAM_WRITE) {
		ASSERT(RW_WRITE_HELD(&ip->inode_rwl) ||
		    (flag == SAM_WR_DIRECT_IO));
		/* check for allocated space */
		if (size <= 0) {
			TRACE(T_SAM_DDMAP1, SAM_ITOP(ip), (sam_tr_t)offset,
			    (sam_tr_t)ip->size, pboff);
			sam_syslog(ip, E_PREALLOC_EXCEEDED, ENXIO, 0);
			return (ENXIO);
		}
		if ((offset + count) >= ip->size) {
			if (count < size) {
				size = count;
			}
			if (flag == SAM_WR_DIRECT_IO) {
				mutex_enter(&ip->fl_mutex);
			}
			if ((offset + count) > ip->cl_allocsz) {
				ip->cl_allocsz = offset + size;
			}
			if (flag < SAM_WRITE_SPARSE) {
				/* Increment filesize if no error */
				ip->size = offset + size;
				if (flag != SAM_WRITE &&
				    flag != SAM_WR_DIRECT_IO) {
					if (!ip->flags.b.staging &&
					    !S_ISSEGI(&ip->di)) {
						if (ip->size > ip->di.rm.size) {
							ip->di.rm.size =
							    ip->size;
						}
					}
				}
			}
			if (flag == SAM_WR_DIRECT_IO) {
				mutex_exit(&ip->fl_mutex);
			}
		}
	}
#endif /* sun */
	if (iop) {
		iop->blkno = *bnp;	/* Block at start of direct extent */
		iop->blkno <<= mp->mi.m_bn_shift;
		iop->imap.ord0 = iop->ord = *eip;
		iop->blk_off = blk_off;
		blk_off >>= SAM_DEV_BSHIFT; /* blk_off now in 1024 byte units */
		/* Adjust block and ordinal within group */
		if (num_group > 1) {
			int sg = (pboff / bsize);
			pboff = (pboff % bsize);
			iop->ord += sg;
			blk_off /= num_group;
			iop->blk_off += (sg * bsize);
		}
		iop->blkno += blk_off;
		iop->imap.blkno0 = iop->blkno;
		iop->pboff = pboff;

		iop->imap.flags = ioflags;
		iop->imap.dt = (uchar_t)dt;
		iop->imap.bt = LG;
		iop->dev_bsize = DEV_BSIZE;
		iop->bsize = bsize;
		iop->num_group = num_group;
		iop->count = (size > count) ? count : size;
		if ((flag >= SAM_READ_PUT) && (flag <= SAM_FORCEFAULT)) {
			sam_ssize_t contig;

			contig = (sam_ssize_t)ip->di.extent[1] <<
			    SAM_DEV_BSHIFT;
			if (num_group > 1) contig *= num_group;
			contig -= iop->blk_off;
			if (contig > SAM_DIRECTIO_MAX_CONTIG)
				contig = SAM_DIRECTIO_MAX_CONTIG;
			iop->contig = (sam_size_t)contig;
			TRACE(T_SAM_CONTIG, SAM_ITOP(ip), iop->contig,
			    bsize, iop->blkno);
			if (contig < 0) {	/* contig isn't a valid value */
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_map_direct: contig "
				    "is %lld",
				    mp->mt.fi_name, (long long)contig);
				sam_req_ifsck(mp, iop->ord,
				    "sam_map_direct: bad contig", &ip->di.id);
				error = EIO;
			}
		}
		iop->imap.contig0 = iop->contig;
		TRACE(T_SAM_DDBLK, SAM_ITOP(ip), ip->di.id.ino,
		    iop->blkno, iop->ord);
	}
	return (error);
}


/*
 * ----- sam_map_indirect - Map logical block to physical block.
 *
 *  This function assumes indirect blocks.
 *  The extents consists of x small block direct entries, y large block
 *  direct entries, and i indirect entries. The first indirect entry
 *  points to a large extent of k direct entries. The second indirect
 *  entry points at a large extent of k indirect entries, each
 *  entry pointing to a large extent. The algorithm for the
 *  number of direct entries:
 *
 *  nde(x,y,i,k) = x + y + k + k**2 + k**3 + ... k**i
 */

static int			/* ERRNO, 0 if successful. */
sam_map_indirect(
	sam_node_t *ip,	/* Pointer to the inode. */
	offset_t offset,	/* Logical byte offset in file (longlong_t). */
	offset_t count,		/* Requested byte count. */
	sam_map_t flag,		/* SAM_READ if reading. */
				/* SAM_READ_PUT if read with rounded up */
				/*   filesize, return contiguous blocks. */
				/* SAM_RD_DIRECT_IO if read directio */
				/* SAM_WRITE if writing. */
				/* SAM_WR_DIRECT_IO if write directio */
				/*   (READERS lock held). */
				/* SAM_FORCEFAULT if write and fault */
				/*   for page. */
				/* SAM_WRITE_MMAP if memory mapped write. */
				/* SAM_WRITE_BLOCK if get block */
				/* SAM_WRITE_SPARSE if sparse block */
				/* SAM_ALLOC_BLOCK allocate and don't */
				/*   update size. */
				/* SAM_ALLOC_BITMAP allocate and build */
				/*   bit map (srvr). */
				/* SAM_ALLOC_ZERO allocate and zero. */
	sam_ioblk_t *iop,	/* Ioblk array. */
	cred_t *credp)		/* Credentials for block allocation */
{
	map_params_t params;
	int error = 0;
#ifdef METADATA_SERVER
	offset_t orig_offset = offset;
#endif
	uint64_t zerodau[2];
	ushort_t no_daus = 0;
#ifdef sun
	offset_t prev_filesize = ip->size;
#endif

	ASSERT(!SAM_IS_OBJECT_FILE(ip));

	zerodau[0] = zerodau[1] = 0;
	bzero(&params, sizeof (params));

	/*
	 * Build ioblk array given offset and count.
	 */
	while (count > 0) {
		sam_ioblk_t ioblk;

		/*
		 * A removable media file should NOT get a disk block except for
		 * 1st.
		 */
		if (S_ISREQ(ip->di.mode) && (offset != 0)) {
			return (ENODEV);
		}

		bzero(&ioblk, sizeof (ioblk));

		TRACE(T_SAM_DKMAP, SAM_ITOP(ip), (sam_tr_t)offset,
		    (sam_tr_t)count,
		    flag);

		/*
		 * Given the logical file offset, get the block (params.bnp),
		 * ordinal (param.eip), and block type (ioblk.imap.bt).
		 */
		if ((error = sam_get_block(ip, offset, flag, &ioblk,
		    &params))) {
			sam_flush_map(ip->mp, &params);
			return (error);
		}

		/*
		 * params.ileft is set for the number of "like" blocks remaining
		 * in the inode or indirect block. Thus, we can go immediately
		 * to the next block without re-adjusting the parameters in
		 * sam_get_block.
		 */
		while (count > 0) {
			if (sam_set_blk_off(ip, offset, flag, &ioblk,
			    &params) == 0) {
				sam_flush_map(ip->mp, &params);
				return (0);
			}

			/*
			 * If not allocated and not sparse, allocate the blocks.
			 * The check with sam_round_to_dau is done to make sure
			 * we do not allocate an unnecessary extent beyond eof.
			 */
			if (*params.bnp == 0) {
				if ((flag == SAM_READ_PUT) ||
				    (flag == SAM_RD_DIRECT_IO)) {
					ioblk.imap.flags |= M_SPARSE;
				} else if (SAM_IS_CLIENT_OR_READER(ip->mp)) {
					sam_flush_map(ip->mp, &params);
					return (ELNRNG);
#ifdef METADATA_SERVER
				} else if ((flag != SAM_WRITE_SPARSE) ||
				    (offset < prev_filesize) ||
				    (offset < sam_round_to_dau(ip,
				    prev_filesize, SAM_ROUND_UP))) {

					boolean_t alloc_block = TRUE;


	/* N.B. Bad indentation here to meet cstyle requirements */
	if (flag == SAM_WR_DIRECT_IO) {
		if (params.bp) {
			sam_indirect_extent_t *iep;	/* Indir extent ptr */
			int ord;
			sam_bn_t blkno;
			long bsize;

			ord = params.ord;
			blkno = (params.bp->b_blkno >> SAM2SUN_BSHIFT);
			bsize = params.bp->b_bcount;
			sam_flush_map(ip->mp,
			    &params);
			RW_UPGRADE_OS(&ip->inode_rwl);
			error = sam_sbread(ip, ord, blkno, bsize,
			    &params.bp);
			if (error) {
				RW_DOWNGRADE_OS(&ip->inode_rwl);
				return (error);
			}
			iep = (sam_indirect_extent_t *)
			    (void *)params.bp->b_un.b_addr;
			params.bnp = &iep->extent[params.kptr[params.kk]];
			params.eip =
			    &iep->extent_ord[params.kptr[params.kk]];
			if (*params.bnp) {
				alloc_block = FALSE;
				if (flag == SAM_WR_DIRECT_IO) {
					RW_DOWNGRADE_OS(&ip->inode_rwl);
				}
			}
		} else {
			RW_UPGRADE_OS(&ip->inode_rwl);
		}
	}


					if (alloc_block) {
						error = sam_allocate_block(ip,
						    offset, flag,
						    &ioblk, &params, credp);
						if (flag == SAM_WR_DIRECT_IO) {
							RW_DOWNGRADE_OS(
							    &ip->inode_rwl);
						}
						if (error) {
							sam_flush_map(ip->mp,
							    &params);

							/*
							 * Check for a short
							 * write condition.  If
							 * we are out of blocks
							 * and this isn't the
							 * first iteration allow
							 * previously written
							 * pages to persist.
							 * (ie, There was space
							 * available, not all we
							 * needed).
							 */
							if (IS_SAM_ENOSPC(
							    error) &&
							    (offset !=
							    orig_offset) &&
							    SAM_IS_STANDALONE(
							    ip->mp)) {
								return (0);
							} else {
								return (error);
							}
						}
					}
					if (flag == SAM_ALLOC_BITMAP) {
						int i = 0;
						int ishift = no_daus;

						if (no_daus >= 64) {
							ishift = no_daus - 64;
							i = 1;
						}
						zerodau[i] |= ((uint64_t)1 <<
						    ishift);
					}
#endif
				}
			}
			ioblk.blkno = *params.bnp;
			ioblk.blkno <<= ip->mp->mi.m_bn_shift;
			ioblk.imap.blkno0 = ioblk.blkno;
			ioblk.imap.ord0 = ioblk.ord = *params.eip;
			if (flag == SAM_ALLOC_BITMAP) {
				no_daus++;
			}

			/*
			 * If appending, clear pages so we do not read to write.
			 */
			error = sam_clear_append(ip, offset, count, flag,
			    &ioblk, &params);
			if (error != 0) {
				return (error);
			}

			/*
			 * Fill in ioblk and put in incore cache buffer.
			 */
			sam_set_ioblk(ip, count, flag, &ioblk, &params);

			mutex_enter(&ip->iom.map_mutex);
			ioblk.imap.flags |= M_VALID;
			ip->iom.imap[ip->iom.map_in] = ioblk.imap;
			if (++ip->iom.map_in >= SAM_MAX_IOM) {
				ip->iom.map_in = 0;
			}
			if (iop) {
				if ((flag != SAM_ALLOC_BITMAP) ||
				    (no_daus == MAX_ZERODAUS)) {
					*iop = ioblk;
					iop->no_daus = no_daus;
					iop->zerodau[0] = zerodau[0];
					iop->zerodau[1] = zerodau[1];
					mutex_exit(&ip->iom.map_mutex);

					sam_flush_map(ip->mp, &params);
#ifdef sun
					if ((offset > prev_filesize) &&
					    (flag == SAM_WRITE)) {
						/*
						 * Zero the DAUs between
						 * prev_filesize and offset.
						 */
						sam_zero_sparse_blocks(ip,
						    prev_filesize, offset);
					}
#endif
					return (0);
				}
			}
			mutex_exit(&ip->iom.map_mutex);

			TRACE(T_SAM_DKBLK, SAM_ITOP(ip), ip->di.id.ino,
			    (sam_daddr_t)(*params.bnp) << ip->mp->mi.m_bn_shift,
			    *params.eip);
			params.bnp++;
			params.eip++;
			offset += params.size_left;
			if ((sam_u_offset_t)offset > SAM_MAXOFFSET_T) {
				sam_flush_map(ip->mp, &params);
				return (EFBIG);
			}
			count -= params.size_left;
			if (--params.ileft <= 0) {
				break;
			}
		} /* while (count > 0) */

		sam_flush_map(ip->mp, &params);

	} /* while (count > 0) */
	if (flag == SAM_ALLOC_BITMAP) {
		iop->no_daus = no_daus;
		iop->zerodau[0] = zerodau[0];
		iop->zerodau[1] = zerodau[1];
	}
#ifdef sun
	if ((orig_offset > prev_filesize) && (flag == SAM_WRITE)) {
		/*
		 * Zero the DAUs between prev_filesize and offset.
		 */
		sam_zero_sparse_blocks(ip, prev_filesize, orig_offset);
	}
#endif
	return (0);
}


/*
 * ----- sam_flush_map - finish the map buffer processing.
 *
 * If modified, write out the buffer.
 */

static void
sam_flush_map(struct sam_mount *mp, map_params_t *mapp)
{
#ifdef sun
	if (mapp->bp) {
		if (mapp->modified) {	/* if extent buffer modified */
			if (mapp->force_wr) {
				(void) sam_bwrite_noforcewait_dorelease(mp,
				    mapp->bp);
			} else {
				bdwrite(mapp->bp);
			}
		} else {
			brelse(mapp->bp);
		}
	}
	mapp->bp = NULL;
	mapp->force_wr = 0;
#endif /* sun */
#ifdef linux
	if (mapp->buf) {
		kmem_free(mapp->buf, mapp->bufsize);
		mapp->buf = NULL;
	}
#endif /* linux */
}


/*
 * ----- sam_get_block - get the extent given the logical file offset.
 *
 * Given the logical file offset, get the block (params.bnp),
 * ordinal (param.eip), and block type (ioblk.imap.bt).
 */

static int
sam_get_block(
	sam_node_t *ip,		/* Pointer to the inode. */
	offset_t offset,	/* Logical byte offset in file (longlong_t). */
	sam_map_t flag,		/* Type of op, SAM_READ, SAM_WRITE, etc. */
	sam_ioblk_t *iop,	/* Ioblk array. */
	map_params_t *mapp)	/* Pointer to the map parameters */
{
	sam_mount_t *mp;
	sam_indirect_extent_t *iep;	/* Indirect extent pointer */
	int de;				/* Direct extent ordinal */
	int no_iext;			/* Number of direct indirect extents */
	int bt;				/* Block type, SM or LG */
	int dt;				/* Dev type, DD (data) or MM (meta) */
	int kk;
	int error;
	long idt_dau;
	struct sam_get_extent_args arg;
	struct sam_get_extent_rval rval;

	mp = ip->mp;
	arg.dau = &mp->mi.m_dau[0];
	arg.num_group = mp->mi.m_fs[ip->di.unit].num_group;
	arg.sblk_vers = mp->mi.m_sblk_version;
	arg.status = ip->di.status;
	arg.offset = offset;
	rval.de = &de;
	rval.dtype = &dt;
	rval.btype = &bt;
	rval.ileft = &mapp->ileft;
	if ((error = sam_get_extent(&arg, mapp->kptr, &rval))) {
		return (error);
	}

#ifdef sun
	mapp->bp = NULL;
#endif /* sun */
#ifdef linux
	mapp->buf = NULL;
#endif /* linux */
	mapp->bnp = &ip->di.extent[de];
	mapp->eip = &ip->di.extent_ord[de];
	iop->imap.dt = (uchar_t)dt;
	iop->imap.bt = (uchar_t)bt;

	/*
	 * If offset is not in the indirect blocks, done.
	 * Note, for shared reader, all direct .inodes blocks exist.
	 */
	if (de < NDEXT) {
		return (0);
	}

	/*
	 * Loop through up to 3 direct indirect pointers, beginning
	 * at the first indirect, 2nd indirect, then 3rd indirect.
	 * kptr[0..2] holds the block index into the indirect block.
	 * Allocation the indirect blocks on the meta device.
	 */
	no_iext = de - NDEXT;		/* Number of direct indirect pointers */
	idt_dau = SAM_INDBLKSZ;
	for (kk = 0; kk <= no_iext; kk++) {
		int ord;
		sam_bn_t ext_bn;
		sam_daddr_t bn;

		/*
		 * Client allocates indirect blocks on the server.
		 */
		if (SAM_IS_SHARED_CLIENT(ip->mp) && (*mapp->bnp == 0)) {
			sam_flush_map(ip->mp, mapp);
			if (flag == SAM_READ_PUT) {
				return (0);
			} else {
				return (ELNRNG);
			}
		}

#ifdef METADATA_SERVER
		/*
		 * Allocate indirect block on meta device.
		 */
		if (*mapp->bnp == 0) {
			/* If direct indirect block not allocated */
			if (flag < SAM_WRITE) {
				/*
				 * Don't allocate an indirect block if reading.
				 */
				return (0);
			}

			if ((ip->di.id.ino == SAM_BLK_INO)) {
				if (!mp->mi.m_blk_bn) {
					return (ENOSPC);
				}
				/*
				 * Use reserved block for .blocks file.
				 * Avoid deadlock.
				 */
				ord = mp->mi.m_blk_ord;
				ext_bn = mp->mi.m_blk_bn;
				mp->mi.m_blk_bn = 0;
			} else if ((error = sam_alloc_block(mp->mi.m_inodir,
			    LG, &ext_bn, &ord))) {
				return (error);
			}
			if (flag == SAM_WR_DIRECT_IO) {
				mutex_enter(&ip->fl_mutex);
			}
			ip->flags.b.changed = 1;
			if (flag == SAM_WR_DIRECT_IO) {
				mutex_exit(&ip->fl_mutex);
			}
			*mapp->bnp = ext_bn; /* Store allocated block */
			bn = (sam_daddr_t)ext_bn; /* Store alloc'd blk */
			bn <<= mp->mi.m_bn_shift;
			*mapp->eip = (uchar_t)ord; /* Store alloc'd ord */
			if (mapp->bp) {
				bdwrite(mapp->bp);
			}
			mapp->bp = getblk(mp->mi.m_fs[ord].dev,
			    (bn << SAM2SUN_BSHIFT),
			    idt_dau);
			clrbuf(mapp->bp);
			mapp->modified = 1; /* Buffer (mapp->bp) modified */
			/* First indirect write should be forced */
			mapp->force_wr = 1;
			mapp->level = (no_iext - kk); /* Store level of block */
			mapp->kk = kk;			/* Index into kptr */
			mapp->ord = (uchar_t)ord; /* Ord of block in buffer */

			/*
			 * Store validation record in indirect block.
			 */
			iep = ie_ptr(mapp);
			iep->id = ip->di.id;
			iep->ieno = (no_iext - kk);
			iep->time = SAM_SECOND();
		} else {
#endif /* METADATA_SERVER */
			ext_bn = *mapp->bnp;
			bn = (sam_daddr_t)ext_bn;
			bn <<= mp->mi.m_bn_shift;
			ord = (int)*mapp->eip;
#ifdef sun
			if (mapp->bp) {
				brelse(mapp->bp);
			}
			error = sam_sbread(ip, ord, bn, idt_dau, &mapp->bp);
#endif /* sun */
#ifdef linux
			if (mapp->buf == NULL) {
				mapp->buf = kmem_zalloc(idt_dau, KM_SLEEP);
			}
			mapp->bufsize = idt_dau;
			error = sam_sbread(ip, ord, bn, idt_dau, mapp->buf);
#endif /* linux */
			if (error) {
				return (error);
			}
			mapp->modified = 0; /* Buf ((mapp->bp) not modified */
			mapp->level = (no_iext - kk); /* Store level of block */
			mapp->kk = kk;		/* Index into kptr */
			mapp->ord = (uchar_t)ord; /* Ord of block in buffer */

			/*
			 * Validate indirect block.
			 */
			iep = ie_ptr(mapp);
			error = sam_validate_indirect_block(ip, iep,
			    mapp->level);

			/*
			 * Shared reader may have to refresh buffer cache.
			 */
			if (SAM_IS_SHARED_READER(mp)) {
				mapp->bnp = &iep->extent[mapp->kptr[kk]];
				if (error || (error == 0 && *mapp->bnp == 0)) {
#ifdef sun
					mapp->bp->b_flags |= B_STALE | B_AGE;
					brelse(mapp->bp);
					error = sam_sbread(ip, ord, bn,
					    SAM_INDBLKSZ, &mapp->bp);
#endif /* sun */
#ifdef linux
					error = sam_sbread(ip, ord, bn,
					    SAM_INDBLKSZ, mapp->buf);
#endif /* linux */
					if (error) {
						return (error);
					}
					iep = ie_ptr(mapp);
					error = sam_validate_indirect_block(ip,
					    iep, mapp->level);
				}
			}

			/*
			 * If shared file system, reacquire indirect block.
			 */
			if (SAM_IS_SHARED_CLIENT(mp)) {
				if (error) {
					return (ELNRNG);
				}
				if ((iep->extent[mapp->kptr[kk]] == 0) &&
				    (flag != SAM_READ_PUT)) {
					return (ELNRNG);
				}
			}
			if (error) {
				sam_daddr_t bnx;

				bnx = (*mapp->bnp);
				bnx <<= mp->mi.m_bn_shift;
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_get_block: Invalid "
				    "indirect block,"
				    " ino=%d.%d.%d val=%d.%d.%d bn=0x%llx "
				    "ord=%d, error=%d",
				    mp->mt.fi_name, (int)ip->di.id.ino,
				    ip->di.id.gen,
				    mapp->level, (int)iep->id.ino,
				    iep->id.gen, iep->ieno,
				    bnx, *mapp->eip, error);
				return (error);
			}
#ifdef METADATA_SERVER
		}
#endif /* METADATA_SERVER */
		/*
		 * Store location of extent, location of ordinal, and
		 * "like" extents left in the params record.
		 */
		mapp->bnp = &iep->extent[mapp->kptr[kk]];
		mapp->eip = &iep->extent_ord[mapp->kptr[kk]];
		mapp->ileft = DEXT - mapp->kptr[kk];
	}
	return (0);
}


/*
 * ----- sam_validate_indirect_block - validate the indirect block
 */

int				/* ERRNO, 0 if successful. */
sam_validate_indirect_block(sam_node_t *ip, sam_indirect_extent_t *iep,
    int level)
{
	int error = 0;

	if (iep->ieno != level) {
		sam_req_ifsck(ip->mp, -1,
		    "sam_validate_indirect: level mismatch", &ip->di.id);
		error = ENOCSI;
	} else if ((iep->id.ino != ip->di.id.ino) ||
	    (iep->id.gen != ip->di.id.gen)) {
		error = ENOCSI;
		if (S_ISSEGS(&ip->di) && (ip->di.rm.info.dk.seg.ord == 0)) {
			if ((iep->id.ino == ip->di.parent_id.ino) &&
			    (iep->id.gen == ip->di.parent_id.gen)) {
				error = 0;
			}
		}
		if (error) {
			sam_req_ifsck(ip->mp, -1,
			    "sam_validate_indirect: ino/gen mismatch",
			    &ip->di.id);
		}
	}
	return (error);
}


/*
 * ----- sam_set_blk_off - set the logical block offset.
 *
 * Given the logical file offset, set the block offset (ioblk.blk_off),
 * the physical byte offset in the DAU (ioblk.pboff), and the
 * size left in the DAU (params.size).
 */

static int			/* 0 if reading & at end of file, else 1 */
sam_set_blk_off(
	sam_node_t *ip,		/* Pointer to the inode. */
	offset_t offset,	/* Logical byte offset */
	sam_map_t flag,		/* Type of op, SAM_READ, SAM_WRITE, etc. */
	sam_ioblk_t *iop,	/* Pointer to ioblk array. */
	map_params_t *mapp)	/* Pointer to params. */
{
	sam_mount_t *mp;
	int dt;

	mp = ip->mp;

	/*
	 * A large extent represents the DAU size (mkfs -a parameter).
	 * If num_group > 1, a large extent represents the DAU times num_group
	 * in units of bytes.
	 */
	iop->num_group = mp->mi.m_fs[ip->di.unit].num_group;
	ASSERT(iop->num_group >= 1);
	dt = iop->imap.dt;
	iop->dev_bsize = DEV_BSIZE; /* dynamic in future? */
	iop->bsize = mp->mi.m_dau[dt].size[iop->imap.bt]; /* -a size in bytes */
	mapp->dau_size = iop->bsize;
	mapp->dau_mask = mp->mi.m_dau[dt].mask[iop->imap.bt];
	if (iop->num_group > 1) {
		mapp->dau_size *= iop->num_group;
		mapp->dau_mask = 0;
	}

	if (ip->di.status.b.on_large || (iop->imap.bt == SM)) {
		if (mapp->dau_mask && (iop->num_group == 1)) {
			iop->blk_off = offset & ~mapp->dau_mask;
			iop->pboff = offset & mapp->dau_mask;
		} else {
			iop->blk_off = (offset / mapp->dau_size) *
			    mapp->dau_size;
			iop->pboff = offset % mapp->dau_size;
		}
	} else {
		iop->pboff = (offset - SM_OFF(mp, dt)) & mapp->dau_mask;
		iop->blk_off = (offset - SM_OFF(mp, dt)) & ~mapp->dau_mask;
		iop->blk_off += SM_OFF(mp, dt);
	}

	/*
	 * How much room is left in this dau? Return 0 if reading and the
	 * offset is past the EOF. Otherwise return 1.
	 */
	mapp->size_left = mapp->dau_size - iop->pboff;
	if (flag == SAM_READ) {	/* If reading */
		offset_t rem;

		ASSERT(RW_LOCK_HELD(&ip->inode_rwl));
		rem = ip->size - offset;
		if (rem < 0)  rem = 0;
		if (rem < mapp->size_left) {
			mapp->size_left = rem;
		}
		if (mapp->size_left == 0) {
			TRACE(T_SAM_DKMAP1, SAM_ITOP(ip), (sam_tr_t)offset,
			    (sam_tr_t)ip->size, iop->pboff);
			return (0);
		}
	}
	return (1);
}


/*
 * ----- sam_clear_append - Clear pages when appending.
 *
 *	When returning an error, sam_flush_map has been called.
 */

static int
sam_clear_append(
	sam_node_t *ip,		/* Pointer to the inode. */
	offset_t offset,	/* Logical byte offset in file (longlong_t). */
	offset_t count,		/* Requested byte count. */
	sam_map_t flag,		/* SAM_WRITE or SAM_FORCEFAULT. */
	sam_ioblk_t *iop,
	map_params_t *mapp)
{
	sam_mount_t *mp;
#ifdef sun
	int error;
	sam_u_offset_t seg_size;
#endif

	mp = ip->mp;

	/*
	 * If appending, update file size if not staging and not index segment.
	 */
	mapp->prev_filesize = ip->size;		/* Save current size */

	if (flag >= SAM_WRITE) {
		if (flag == SAM_WR_DIRECT_IO) {
			mapp->size_left = count;
			mutex_enter(&ip->fl_mutex);
		} else if (count < mapp->size_left) {
			ASSERT(RW_WRITE_HELD(&ip->inode_rwl));
			mapp->size_left = count;
		}
		if ((offset + count) > ip->cl_allocsz) {
			ip->cl_allocsz = offset + mapp->size_left;
		}
		if ((offset + count) >= ip->size) {
			if (flag < SAM_WRITE_SPARSE) {
				/* Increment filesize if no error */
				ip->size = offset + mapp->size_left;
				if (flag != SAM_WRITE &&
				    flag != SAM_WR_DIRECT_IO) {
					if (!ip->flags.b.staging &&
					    !S_ISSEGI(&ip->di)) {
						if (ip->size > ip->di.rm.size) {
							ip->di.rm.size =
							    ip->size;
						}
					}
				}
			}
		}
		if (flag == SAM_WR_DIRECT_IO) {
			mutex_exit(&ip->fl_mutex);
		}
	}

#ifdef sun
	/*
	 * Check for a new sparse region between the current file size
	 * and the beginning of the current write operation and zero it.
	 * (DKMAP4)
	 *
	 * Make sure the segment that contains the new EOF is zeroed
	 * from EOF to the end of the segment.
	 * (DKMAP5)
	 *
	 * The code that zeroes the last segment of the file when
	 * appending and flag == SAM_FORCEFAULT is handled in sam_write_io.
	 */
	if ((flag == SAM_WRITE) &&
	    (iop->imap.bt == SM) &&
	    (mapp->size_left > mp->mi.m_dau[iop->imap.dt].seg[SM])) {
		/*
		 * sam_map_block has detected contiguous small DAUs
		 * and set mapp->size_left accordingly. This confuses
		 * the following if statement since .seg here is only 4096.
		 */
		seg_size = mp->mi.m_dau[iop->imap.dt].seg[LG];
	} else {
		seg_size = mp->mi.m_dau[iop->imap.dt].seg[iop->imap.bt];
	}

	if (flag == SAM_WRITE && ((mapp->size_left < seg_size) ||
	    (offset > mapp->prev_filesize))) {
		offset_t dau_size, dau_mask;
		int dt;
		int len;	/* Length to clear */
		sam_offset_t off = offset;

		/*
		 * If the previous file size is < current offset, this means
		 * we have a sparse file. Clear up from the previous
		 * file size to the next DAU boundary. May then
		 * skip one or more DAUS. If Shared filesystem, clear
		 * from last written address up to current offset.
		 */
		dt = iop->imap.dt;
		if (!ip->di.status.b.on_large &&
		    mapp->prev_filesize < SM_OFF(mp, dt)) {
			dau_size = SM_BLK(mp, dt);
			dau_mask = SM_MASK(mp, dt);
		} else {
			if (mp->mi.m_fs[ip->di.unit].num_group > 1) {
				/*
				 * mapp->dau_size is the logical dau of a stripe
				 * group.  We want the dau_size of the stripe
				 * group members.
				 */
				dau_size = LG_BLK(mp, dt);
			} else {
				dau_size = mapp->dau_size;
			}
			dau_mask = mapp->dau_mask;
		}
		if (mapp->prev_filesize < off) {
			if (iop->blk_off > mapp->prev_filesize) {
				int totlen;

				off = mapp->prev_filesize;
				if (dau_mask) {
					if (ip->di.status.b.on_large ||
					    off < SM_OFF(mp, dt)) {
						totlen = dau_size -
						    (off & dau_mask);
					} else {
						totlen = dau_size -
						    ((off - SM_OFF(mp, dt)) &
						    dau_mask);
					}
				} else {
					totlen = dau_size - (off % dau_size);
				}
				if (totlen == dau_size) {
					/* Don't clear - no DAU allocated */
					totlen = 0;
				}
				if (totlen > 0) {
					TRACE(T_SAM_DKMAP4, SAM_ITOP(ip),
					    (sam_tr_t)off, 0, totlen);
					error = sam_fbzero(ip, off, totlen,
					    NULL, mapp, NULL);
					if (error != 0) {
						sam_flush_map(ip->mp, mapp);
						return (error);
					}
				}
				/*
				 * If sparse and buffer offset is NOT in the
				 * same DAU, off is the beginning of next
				 * allocated DAU.
				 */
				off = iop->blk_off;
			} else {
				/*
				 * If sparse but buffer offset is in the same
				 * DAU, off is last file size.
				 */
				off = mapp->prev_filesize;
			}
		} else {
			/*
			 * If not sparse, off is the buffer offset.
			 */
			off = offset;
		}

		/*
		 * Clear from off up to a .seg boundary past offset +
		 * mapp->size_left.
		 */
		seg_size = mp->mi.m_dau[iop->imap.dt].seg[iop->imap.bt];
		len = roundup(offset + mapp->size_left, seg_size) - off;
		if (len > 0) {
			TRACE(T_SAM_DKMAP5, SAM_ITOP(ip), (sam_tr_t)off,
			    len, offset);
			error = sam_fbzero(ip, off, len, iop, mapp, NULL);
			if (error != 0) {
				sam_flush_map(ip->mp, mapp);
				return (error);
			}
		}
	}
#endif /* sun */
	return (0);
}

/*
 * ----- sam_map_fault - Fault for the pages if they do not exist.
 *
 * If clearing pages at relative offset != 0, we must
 * fault for the pages and read them in.  On machines with a 4K PAGESIZE,
 * we might need to fault in multiple pages.
 */

#ifdef sun

int
sam_map_fault(sam_node_t *ip, sam_u_offset_t boff, sam_offset_t total_len,
    map_params_t *mapp)
{
	int error = 0;
	vnode_t *vp = SAM_ITOV(ip);
	page_t *pp = NULL;
	caddr_t base;
	sam_u_offset_t this_len;
	long bsize = 0;
	int ord;
	sam_bn_t blkno;

	do {
		this_len = (total_len > PAGESIZE) ? PAGESIZE : total_len;

		if ((vn_has_cached_data(vp) == 0) ||
		    ((pp = page_lookup(vp, boff, SE_SHARED)) == NULL)) {

			if (!bsize) {
				/*
				 * If we're faulting the page(s) in and we're
				 * holding the indirect block, release it to
				 * avoid a deadlock in the fill, and grab it
				 * back afterwards.
				 */
				if (mapp && mapp->bp) {
					ord = mapp->ord;
					blkno = (mapp->bp->b_blkno >>
					    SAM2SUN_BSHIFT);
					bsize = mapp->bp->b_bcount;
					sam_flush_map(ip->mp, mapp);
				}
			}
			SAM_SET_LEASEFLG(ip->mp);
			base = segmap_getmapflt(segkmap, vp, boff,
			    this_len, 1, S_WRITE);
			segmap_release(segkmap, base, 0);
			SAM_CLEAR_LEASEFLG(ip->mp);
			TRACE(T_SAM_MAPFLT, vp, (sam_tr_t)boff,
			    (sam_tr_t)this_len,
			    (sam_tr_t)vp->v_pages);
		}
		total_len -= this_len;
		boff += this_len;
		if (pp != NULL) {
			page_unlock(pp);
			pp = NULL;
		}
	} while (total_len > 0);

	if (bsize) {
		error = sam_sbread(ip, ord, blkno, bsize, &mapp->bp);
		if (error == 0) {
			sam_indirect_extent_t *iep; /* Indirect extent ptr */

			iep = (sam_indirect_extent_t *)
			    (void *)mapp->bp->b_un.b_addr;
			mapp->bnp = &iep->extent[mapp->kptr[mapp->kk]];
			mapp->eip = &iep->extent_ord[mapp->kptr[mapp->kk]];
		}
	}
	return (error);
}
#endif /* sun */

/*
 * ----- sam_set_ioblk - Set ioblk information.
 *
 * Adjust block and ordinal within striped group.
 * Compute contiguous blocks for this block.
 */

static void
sam_set_ioblk(
	sam_node_t *ip,		/* Pointer to the inode. */
	offset_t count,		/* Requested byte count. */
	sam_map_t flag,		/* SAM_WRITE or SAM_FORCEFAULT. */
	sam_ioblk_t *iop,
	map_params_t *mapp)
{
	sam_mount_t *mp;	/* Pointer to mount table */
	int dt;
	int bt;
	uint_t sgo = 0;		/* Striped group offset */

	mp = ip->mp;
	dt = iop->imap.dt;
	bt = iop->imap.bt;
	iop->imap.blk_off0 = iop->blk_off;
	if (iop->num_group > 1) {
		int sg;

		sg = (iop->pboff / mp->mi.m_dau[dt].size[bt]);
		iop->pboff = iop->pboff % mp->mi.m_dau[dt].size[bt];
		iop->ord += sg;
		sgo = sg * mp->mi.m_dau[dt].size[bt];
		iop->blk_off += (offset_t)sgo;
	}
	iop->count = (mapp->size_left > count) ? count : mapp->size_left;

	/*
	 * Compute contiguous extents if map entry is not in cache.
	 */
	if (mapp->bnp) {
		int maxcont;	/* max contig. extents */
		offset_t contig_bytes;

		/*
		 * maxcont is the maximum number of extents that
		 * sam_contig() should return. These extents are in
		 * units of dau_size (block size * num_group).
		 */
		if (flag == SAM_RD_DIRECT_IO || flag == SAM_WR_DIRECT_IO) {
			/* limit direct I/O at 31 bits of bytes */
			maxcont = SAM_DIRECTIO_MAX_CONTIG / mapp->dau_size;
		} else if (mp->mi.m_dau[dt].shift[bt] &&
		    (iop->num_group == 1)) {
			/* "readahead" extents */
			maxcont = mp->mt.fi_readahead >>
			    mp->mi.m_dau[dt].shift[bt];
		} else {
			/*
			 * "readahead" extents * number of elements in the
			 * striped group.
			 */
			maxcont = iop->num_group *
			    (mp->mt.fi_readahead / mapp->dau_size);
		}
		/*
		 * Contig_bytes is number of contiguous extents in bytes.
		 */
		contig_bytes = (offset_t)sam_contig(maxcont,
		    mapp->bnp, mapp->eip,
		    mp->mi.m_dau[dt].kblocks[bt], mp->mi.m_bn_shift,
		    mapp->ileft) * mapp->dau_size;
		/*
		 * Contig is the number of contiguous bytes in the extents.
		 */
		if (contig_bytes > 0) {
			ASSERT(contig_bytes > (offset_t)sgo);
			contig_bytes -= (offset_t)sgo;
		}
		iop->imap.contig0 = iop->contig = (uint_t)contig_bytes;
		TRACE(T_SAM_CONTIG, SAM_ITOP(ip), iop->contig, mapp->dau_size,
		    iop->blkno);
	}
}


/*
 * ----- sam_contig - Get number of contiguous blocks.
 *
 *  Return the number of contiguous blocks with the same ordinal.
 */

static int		/* length of contiguous byte */
sam_contig(
	int maxcont,	/* Maximum number of contiguous blocks */
	uint32_t *bnp,	/* Direct extent block pointer */
	uchar_t *eip,	/* Ordinal pointer */
	int blocks,	/* dau block size in units of 1024 */
	int ext_to_1k,	/* extent to 1k shift count */
	int ileft)	/* Number of extents left */
{
	int num, count;
	sam_daddr_t cbn, nbn;
	uchar_t ord;

	cbn = *bnp;
	if (cbn == 0) {
		return (0);
	}
	cbn <<= ext_to_1k;		/* Current block */
	ord = *eip;				/* Initial ordinal */
	count = 1;
	num = MIN(ileft, maxcont);
	while (--num > 0) {
		eip++;
		bnp++;
		nbn = *bnp;
		if (nbn != 0) {
			nbn <<= ext_to_1k;
			if (((cbn + (sam_daddr_t)blocks) != nbn) ||
			    (*eip != ord)) {
				break;
			}
		} else {
			break;
		}
		cbn = nbn;
		count++;
	}
	return (count);
}


/*
 *	----- sam_zero_sparse_blocks -
 *
 * Zero all allocated DAUs between off1 and off2.
 *
 * Off1 is rounded up to a DAU boundary and off2
 * is rounded down to a DAU boundary.
 *
 * This routine is called after clear_append() when appending
 * or truncating up creates allocated but uninitialized space.
 */

void
sam_zero_sparse_blocks(
	sam_node_t *ip,		/* Pointer to the inode. */
	offset_t off1,		/* beginning of range to clear */
	offset_t off2)		/* end of range to clear */
{
	int error = 0;
	offset_t start, end;
	offset_t dau_size;
	struct sam_mount *mp = ip->mp;
	short dt = ip->di.status.b.meta;

	if (off2 <= off1) {
		return;
	}

	/*
	 * Round up off1 if not on a DAU boundary.
	 */
	start = sam_round_to_dau(ip, off1, SAM_ROUND_UP);

	/*
	 * Round off2 down if not on a DAU boundary.
	 */
	end = sam_round_to_dau(ip, off2, SAM_ROUND_DOWN);

	if (end <= start) {
		return;
	}

	/*
	 * Loop through all the DAUs between start and end.
	 */
	while (start < end) {
		sam_offset_t current_len;
		sam_ioblk_t hiop;
		map_params_t params;
#ifdef sun
		struct fbuf *fbp;
#endif
		int is_small;

		bzero((char *)&hiop, sizeof (sam_ioblk_t));
		bzero(&params, sizeof (params));

		if (!ip->di.status.b.on_large && start < SM_OFF(mp, dt)) {
			dau_size = SM_BLK(mp, dt);
			is_small = 1;
		} else {
			dau_size = LG_BLK(mp, dt);
			is_small = 0;
		}

		/*
		 * See if there is an allocated block at start.
		 * If there is, zero it.
		 */

		error = sam_get_block(ip, start, SAM_READ, &hiop, &params);

		if (error == 0 && (*params.bnp != 0)) {
			uint_t zlen;

			if (is_small) {
#ifdef sun
				sam_map_fault(ip, start, dau_size, NULL);
				SAM_SET_LEASEFLG(mp);
				fbzero(SAM_ITOV(ip), start, dau_size, &fbp);
				SAM_CLEAR_LEASEFLG(mp);
				fbrelse(fbp, S_WRITE);
#endif
#ifdef linux
				sam_linux_fbzero(ip, start, dau_size);
#endif
			} else {
				zlen =
				    mp->mi.m_dau[
				    hiop.imap.dt].seg[hiop.imap.bt];
				current_len = 0;
				if (zlen > 0) {
					if (zlen > MAXBSIZE) {
						zlen = MAXBSIZE;
					}
					/*
					 * Zero the DAU, MAXBSIZE (8192) at a
					 * time.
					 */
					while (current_len < dau_size) {
#ifdef sun
						SAM_SET_LEASEFLG(mp);
						fbzero(SAM_ITOV(ip),
						    start + current_len,
						    zlen, &fbp);
						SAM_CLEAR_LEASEFLG(mp);
						fbrelse(fbp, S_WRITE);
#endif
#ifdef linux
						sam_linux_fbzero(ip,
						    start + current_len, zlen);
#endif
						current_len += zlen;
					}
				}
			}
		}
		sam_flush_map(ip->mp, &params);
		start += dau_size;
	}
}

#ifdef linux

void
sam_linux_fbzero(
	sam_node_t *ip,		/* Pointer to the inode. */
	offset_t off1,		/* beginning of range to clear */
	int len)		/* length of range to clear */
{
	int error = 0;
	offset_t start, end;
	struct inode *li = SAM_SITOLI(ip);
	unsigned long blocksize = li->i_sb->s_blocksize;
	unsigned char blkbits = li->i_sb->s_blocksize_bits;
	sam_mount_t *mp = ip->mp;

	start = off1;
	end = off1 + len;

	while (start < end) {
		sam_ioblk_t ioblk;
		map_params_t params;

		bzero((char *)&ioblk, sizeof (ioblk));
		bzero((char *)&params, sizeof (params));

		error = sam_get_block(ip, start, SAM_READ, &ioblk, &params);

		if (error == 0 && (*params.bnp != 0)) {
			uint_t zlen;
			int blkno;
			int ord = *params.eip;
			offset_t reloff, rel_boff;
			offset_t dau_start = sam_round_to_dau(ip, start,
			    SAM_ROUND_DOWN);
			int num_group;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
			struct block_device *dev;
#else
			kdev_t dev;
#endif
			num_group = mp->mi.m_fs[ip->di.unit].num_group;
			if (num_group > 1) {
				int sg;

				/*
				 * Set the device ordinal to the correct
				 * member of the striped group.
				 */
				params.dau_size =
				    mp->mi.m_dau[
				    ioblk.imap.dt].size[ioblk.imap.bt] *
				    num_group;
				ioblk.pboff = start % params.dau_size;
				sg = (ioblk.pboff /
				    mp->mi.m_dau[
				    ioblk.imap.dt].size[ioblk.imap.bt]);
				ord += sg;
			}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
			dev = mp->mi.m_fs[ord].bdev;
#else
			dev = mp->mi.m_fs[ord].dev;
#endif

			/*
			 * For Solaris we would use sam_set_blk_off for this
			 * calculation.  But for Linux we know that we already
			 * have the block number in the correct units, so we
			 * skip calling sam_set_blk_off.
			 */
			rel_boff = start - dau_start;
			blkno = (int)*params.bnp + (rel_boff >> blkbits);

			reloff = start & (blocksize-1);

			if (reloff != 0) {
				zlen = blocksize - reloff;
			} else if (len < blocksize) {
				zlen = len;
			} else {
				zlen = blocksize;
			}
			if (zlen > len) {
				zlen = len;
			}

			TRACE(T_SAM_DKMAP6, SAM_ITOP(ip),
			    (sam_tr_t)start, (sam_tr_t)reloff,
			    (sam_tr_t)zlen);

			error = rfs_do_linux_fbzero(SAM_SITOLI(ip), start, zlen,
			    dev, ord, blkno);
			if (error) {
				sam_flush_map(ip->mp, &params);
				break;
			} else {
				if (ip->zero_end < start+zlen) {
					ip->zero_end = start+zlen;
				}
			}
			start += zlen;
		} else {
			sam_flush_map(ip->mp, &params);
			break;
		}
		sam_flush_map(ip->mp, &params);
	}
}
#endif /* linux */

/*
 * ----- sam_round_to_dau - Round up/down to next DAU.
 *
 * Return logical block address of the rounded up/down DAU.
 */

offset_t
sam_round_to_dau(
	sam_node_t *ip,		/* Pointer to the inode. */
	offset_t off,		/* Logical byte offset in file (longlong_t). */
	sam_round_t dir)	/* SAM_ROUND_DOWN or SAM_ROUND_UP */
{
	sam_mount_t *mp = ip->mp;
	short dt = ip->di.status.b.meta;
	offset_t daddr;
	offset_t dau_size;

	/*
	 * Round up given offset if not on a DAU boundary.
	 */
	if (!ip->di.status.b.on_large && off < SM_OFF(mp, dt)) {
		dau_size = SM_BLK(mp, dt);
		daddr = roundup(off, dau_size);
	} else {
		dau_size = LG_BLK(mp, dt);
		if (ip->di.status.b.on_large) {
			daddr = roundup(off, dau_size);
		} else {
			daddr = roundup((off - SM_OFF(mp, dt)), dau_size) +
			    SM_OFF(mp, dt);
		}
	}
	/*
	 * If rounding down and not on DAU boundary, set to next lower DAU
	 * offset.
	 */
	if (dir == SAM_ROUND_DOWN && daddr > off) {
		daddr -= dau_size;
	}
	return (daddr);
}


/*
 * ----- sam_zero_dau - Zero DAU.
 *
 * If the block was allocated, clear pages (entire DAU) beginning at boff.
 */

int
sam_zero_dau(
	sam_node_t *ip,		/* Pointer to the inode. */
	offset_t boff,		/* Logical byte offset in file (longlong_t). */
	offset_t len,		/* Logical byte length (longlong_t). */
	sam_map_t flag,
	ushort_t dt,
	ushort_t bt)
{
	sam_offset_t bend = boff + len;
#ifdef sun
	struct fbuf *fbp;
#endif

	if (flag == SAM_WRITE && bend > ip->size) {
		if (boff >= ip->size) {
			return (0);
		}
		bend = ip->size;
	}
	len = ip->mp->mi.m_dau[dt].seg[bt];
	while (boff < bend) {
#ifdef sun
		if (bt == SM) {
			sam_map_fault(ip, boff, len, NULL);
		}
		SAM_SET_LEASEFLG(ip->mp);
		fbzero(SAM_ITOV(ip), boff, len, &fbp);
		SAM_CLEAR_LEASEFLG(ip->mp);
		fbrelse(fbp, S_WRITE);
#endif
#ifdef linux
		sam_linux_fbzero(ip, boff, len);
#endif
		boff += len;
	}
	return (0);
}


/*
 * ----- sam_clear_append_after_map- Zero sparse blocks
 * when creating sparse regions with uninitialized blocks
 * when using direct IO. The block containing the old file
 * size is zeroed from the old size to the end of the block.
 * The block containing the starting offset is zeroed and
 * the block containing offset + count is zeroed. This last
 * operation is needed if the space beyond the end of the
 * file is mmaped. Any blocks between the old file size
 * and the starting offset are zeroed via sam_zero_sparse_blocks.
 *
 * This code is used for all sparse paths in Linux.
 */
int
sam_clear_append_after_map(sam_node_t *ip, offset_t size, offset_t offset,
    offset_t count)
{
#ifdef sun
	struct fbuf *fbp;
#endif
	offset_t start, offset_start, offset_end;
	offset_t ioend;
	offset_t dau_size;
	int roff, error = 0;
	int totlen;
	ushort_t bt, dt = ip->di.status.b.meta;
	uint_t zlen;
	sam_mount_t *mp = ip->mp;

	TRACE(T_SAM_CADIO, SAM_ITOP(ip), (sam_tr_t)ip->size,
	    (sam_tr_t)offset, (sam_tr_t)size);

	start = size;

	/*
	 * dau_size at start
	 */
	if (!ip->di.status.b.on_large && (start < SM_OFF(mp, dt))) {
		bt = SM;
		dau_size = SM_BLK(mp, dt);
	} else {
		bt = LG;
		dau_size = LG_BLK(mp, dt);
	}

	if ((start <= 0) && (ip->di.extent[0] == 0)) {
		/*
		 * No block at 0
		 */
		start += dau_size;
		if (start > offset + count) {
			return (0);
		}
	}

	if (ip->zero_end > start) {
		start = ip->zero_end;
	}

	/*
	 * dau_size at start
	 */
	if (!ip->di.status.b.on_large && (start < SM_OFF(mp, dt))) {
		bt = SM;
		dau_size = SM_BLK(mp, dt);
	} else {
		bt = LG;
		dau_size = LG_BLK(mp, dt);
	}

	if (offset > size) {
		roff = start - sam_round_to_dau(ip, start, SAM_ROUND_DOWN);
		if (roff > 0) {
			totlen = dau_size - roff;
		} else {
			totlen = 0;
		}

		/*
		 * Creating a hole by starting a write beyond
		 * the end of the file so zero from size to the
		 * end of the dau that contains size.
		 */
		while (totlen) {
			zlen = mp->mi.m_dau[dt].seg[bt] -
			    (roff & (mp->mi.m_dau[dt].seg[bt] - 1));
#ifdef sun
			if ((roff & (mp->mi.m_dau[dt].seg[bt] - 1)) != 0) {
				error = sam_map_fault(ip,
				    start & ~(mp->mi.m_dau[dt].seg[bt] - 1),
				    mp->mi.m_dau[dt].seg[bt], NULL);
				if (error != 0) {
					return (error);
				}
			}
			SAM_SET_LEASEFLG(mp);
			fbzero(SAM_ITOV(ip), start, zlen, &fbp);
			SAM_CLEAR_LEASEFLG(mp);
			fbrelse(fbp, S_WRITE);
#endif
#ifdef linux
			sam_linux_fbzero(ip, start, zlen);
#endif
			start += zlen;
			totlen -= zlen;
			roff = 0;
		}
		ip->zero_end = start;
	}

	/*
	 * Find the start and end of the dau that contains offset.
	 */
	offset_start = sam_round_to_dau(ip, offset, SAM_ROUND_DOWN);
	offset_end = sam_round_to_dau(ip, offset, SAM_ROUND_UP);

	if ((offset_start >= start) && (offset_start < offset)) {
		/*
		 * Zero the dau that contains offset.
		 */
		sam_zero_sparse_blocks(ip, offset_start, offset_end);

		ip->zero_end = offset_end;
	}

	/*
	 * Zero any existing DAUs between the old
	 * size and offset.
	 */
	sam_zero_sparse_blocks(ip, start, offset);

	/*
	 * Zero from the end of the IO (new EOF) to the end of
	 * the DAU that contains the new EOF.
	 */
	ioend = offset + count;
#ifdef sun
	if (ioend > size && ip->zero_end <= ioend) {
		offset_t new_zero_end;
		int len;

		if (!ip->di.status.b.on_large && (ioend < SM_OFF(ip->mp, DD))) {
			new_zero_end = sam_round_to_dau(ip, ioend,
			    SAM_ROUND_UP);
		} else {
			new_zero_end = (ioend + MAXBSIZE - 1) & MAXBMASK;
		}

		len = new_zero_end - ioend;
		if (len > 0) {
			error = sam_fbzero(ip, ioend, len, NULL, NULL, NULL);
		}

		if (error == 0) {
			ip->zero_end = new_zero_end;
		}
	}
#endif

#ifdef linux
	/*
	 * For Linux mmapped files, we can read past EOF as long as we don't
	 * cross a page boundary.  Zero those bytes here.
	 */
	if (ioend > size) {
		offset_t next_page = (ioend + PAGE_SIZE - 1) & PAGE_MASK;

		sam_linux_fbzero(ip, ioend, next_page - ioend);
		ip->zero_end = next_page;

#ifdef __ia64
		/*
		 * The page may contain blocks that are entirely beyond
		 * i_size.  Those blocks will not be written out, so
		 * don't count them as zeroed.
		 */
		{
			struct inode *li = SAM_SITOLI(ip);
			unsigned long blocksize = 1 << li->i_blkbits;
			unsigned long blockmask = ~(blocksize - 1);
			offset_t next_block;

			next_block = (ioend + blocksize - 1) & blockmask;
			ip->zero_end = next_block;
		}
#endif
	}
#endif /* linux */

	return (error);
}


#ifdef linux
void
sam_clear_append_post_write(sam_node_t *ip, offset_t size, offset_t offset,
    offset_t count)
{
	offset_t start;
	offset_t dau_size;
	int roff;
	int totlen;
	ushort_t bt, dt = ip->di.status.b.meta;
	sam_mount_t *mp = ip->mp;
	offset_t isize;
	struct inode *li = SAM_SITOLI(ip);

	TRACE(T_SAM_CADIO, SAM_ITOP(ip), (sam_tr_t)ip->size,
	    (sam_tr_t)offset, (sam_tr_t)size);

	start = size;

	/*
	 * dau_size at start
	 */
	if (!ip->di.status.b.on_large && (start < SM_OFF(mp, dt))) {
		bt = SM;
		dau_size = SM_BLK(mp, dt);
	} else {
		bt = LG;
		dau_size = LG_BLK(mp, dt);
	}

	if ((start <= 0) && (ip->di.extent[0] == 0)) {
		/*
		 * No block at 0
		 */
		start += dau_size;
		if (start > offset + count) {
			return;
		}
	}

	if (ip->zero_end > start) {
		start = ip->zero_end;
	}

	/*
	 * dau_size at start
	 */
	if (!ip->di.status.b.on_large && (start < SM_OFF(mp, dt))) {
		bt = SM;
		dau_size = SM_BLK(mp, dt);
	} else {
		bt = LG;
		dau_size = LG_BLK(mp, dt);
	}

	if (offset > size) {
		roff = start - sam_round_to_dau(ip, start, SAM_ROUND_DOWN);
		if (roff > 0) {
			totlen = dau_size - roff;

			/*
			 * Creating a hole by starting a write beyond
			 * the end of the file so zero from prev size to the
			 * end of the dau that contains size but stopping
			 * prior to offset.  There is already valid data
			 * at offset.
			 */

			if (start + totlen >= offset) {
				/*
				 * start and offset are in the same DAU.
				 */
				roff = start + totlen - offset;
				totlen -= roff;
			}

			if (totlen) {
				sam_linux_fbzero(ip, start, totlen);
				start += totlen;
			}
		}
	}

	if (start < offset) {
		/*
		 * Zero any existing blocks between the last
		 * zeroed DAU and the DAU at offset.
		 */
		sam_zero_sparse_blocks(ip, start, offset);

		/*
		 * Zero bytes in the DAU containing offset, up to offset.
		 */
		start = sam_round_to_dau(ip, offset, SAM_ROUND_DOWN);
		roff = offset - start;
		if (roff > 0) {
			sam_linux_fbzero(ip, start, roff);
		}
	}

	/*
	 * Zero any bytes past i_size, up to the end of the DAU.
	 */
	isize = rfs_i_size_read(li);
	roff = isize - sam_round_to_dau(ip, isize, SAM_ROUND_DOWN);
	if (roff > 0) {
		if (!ip->di.status.b.on_large && (isize < SM_OFF(mp, dt))) {
			bt = SM;
			dau_size = SM_BLK(mp, dt);
		} else {
			bt = LG;
			dau_size = LG_BLK(mp, dt);
		}

		totlen = dau_size - roff;
		if (totlen) {
			/*
			 * If the DAU contains whole blocks that are past
			 * i_size they won't be written. This means we will
			 * have to zero them the next time we read them.
			 */
			offset_t prev_zero_end = ip->zero_end;
			sam_linux_fbzero(ip, isize, totlen);
			ip->zero_end = prev_zero_end;
		}
	}
}
#endif /* linux */


#ifdef sun
/*
 * ----- sam_map_truncate -
 *
 * Change the size of a file to length.
 * Make sure that there is a DAU at the new EOF and
 * that any needed zeroing is done.
 *
 */
int
sam_map_truncate(
	sam_node_t *ip,		/* Pointer to the inode. */
	offset_t length,	/* New file size */
	cred_t *credp)
{
	offset_t size;
	offset_t offset;
	int extra = 0;
	int error = 0;

	size = ip->di.rm.size;

	/*
	 * Offset of the last byte at the new length.
	 */
	offset = length - 1;

	if (length > size) {
		/*
		 * Truncate up.
		 */
		if (SAM_IS_OBJECT_FILE(ip)) {
			error = sam_map_block(ip, length, 0, SAM_WRITE,
			    NULL, credp);
			goto out;
		}

		extra = sam_round_to_dau(ip, offset, SAM_ROUND_UP) - offset;

		if (extra == 0) {
			int dt = ip->di.status.b.meta;
			/*
			 * The offset is at a DAU boundary and the last
			 * byte of the file at the new length is the first
			 * byte of a DAU.
			 */

			/*
			 * Move offset to EOF, needed so sam_clear_append will
			 * zero space from the old EOF to the end of the DAU
			 * that contains the old EOF (DKMAP4);
			 */
			offset++;

			/*
			 * Set extra so DKMAP5 in sam_clear_append zeros only
			 * what needs to be zeroed.
			 */
			if (!ip->di.status.b.on_large &&
			    offset < SM_OFF(ip->mp, dt)) {
				extra = SM_BLK(ip->mp, dt) - 1;
			} else {
				extra = MAXBSIZE - 1;
			}
		}

		error = sam_map_block(ip, (offset_t)offset,
		    (offset_t)extra, SAM_WRITE, NULL, credp);

	} else {
		sam_ioblk_t ioblk;
		map_params_t params;
		/*
		 * Truncate down.
		 */
		if (SAM_IS_OBJECT_FILE(ip)) {
			goto out;
		}

		bzero((char *)&ioblk, sizeof (sam_ioblk_t));
		bzero(&params, sizeof (params));

		error = sam_get_block(ip, offset, SAM_WRITE, &ioblk, &params);

		if (error != 0) {
			sam_flush_map(ip->mp, &params);
			return (error);
		}

		if (*params.bnp == 0) {

			/*
			 * Truncated down into an unallocated region.
			 */
			(void) sam_set_blk_off(ip, offset,
			    SAM_WRITE, &ioblk, &params);

			/*
			 * Allocate a DAU at the end of the file.
			 */
			error = sam_allocate_block(ip, offset, SAM_ALLOC_ZERO,
			    &ioblk, &params, credp);

			sam_flush_map(ip->mp, &params);

		} else {
			offset_t dau_size;
			int len;

			/*
			 * Zero to the end of the DAU that contains the new EOF.
			 * Move offset past the new last byte of the file so it
			 * doesn't get zeroed.
			 */
			sam_flush_map(ip->mp, &params);
			offset++;

			if (ioblk.num_group > 1) {

				dau_size =
				    ip->mp->mi.m_dau[
				    ioblk.imap.dt].size[ioblk.imap.bt];
				ioblk.pboff = offset % dau_size;

			} else {

				(void) sam_set_blk_off(ip, offset,
				    SAM_WRITE, &ioblk, &params);
				dau_size = params.dau_size;
			}
			/*
			 * Don't zero if EOF is on a DAU boundary.
			 */
			if (ioblk.pboff > 0) {
				len = dau_size - ioblk.pboff;
				sam_fbzero(ip, offset, len, &ioblk, NULL, NULL);
			}
		}
	}

out:
	if (error == 0) {
		ip->size = length;
	}
	sam_flush_pages(ip, B_INVAL);

	return (error);
}


/*
 * ----- sam_fbzero -
 *
 * Zero a file from offset for tlen bytes.
 * Offset plus tlen must not go beyond the
 * end of the DAU that contains offset.
 */
int
sam_fbzero(sam_node_t *ip, offset_t offset, int tlen, sam_ioblk_t *iop,
    map_params_t *mapp, struct fbuf **rfbp)
{
	offset_t off;
	int forcefault;
	int bt;
	int dt;
	int roff, len;
	int error = 0;
	sam_mount_t *mp = ip->mp;
	struct fbuf *fbp;
	struct fbuf *last_fbp = NULL;
	int rlen;

	off = offset;
	rlen = tlen;

	if (iop) {
		bt = iop->imap.bt;
		dt = iop->imap.dt;
	} else {
		dt = ip->di.status.b.meta;
		if (!ip->di.status.b.on_large && (offset < SM_OFF(mp, dt))) {
			bt = SM;
		} else {
			bt = LG;
		}
	}

	/*
	 * If offset is on a small-block boundary but not on a
	 * MAXBSIZE boundary, force a fault.
	 */
	if ((bt == SM) &&
	    (((off & (mp->mi.m_dau[dt].seg[SM] - 1)) == 0) &&
	    (off & MAXBOFFSET))) {
		forcefault = 1;
	} else {
		forcefault = 0;
	}

	/*
	 * Clear from offset for length rlen.
	 */
	if ((bt == SM) || ip->di.status.b.on_large) {
		roff = off & (mp->mi.m_dau[dt].seg[bt] - 1);
	} else {
		roff = (off - SM_OFF(mp, dt)) &
		    (mp->mi.m_dau[dt].seg[bt] - 1);
	}
	while (rlen > 0) {

		len = mp->mi.m_dau[dt].seg[bt] - roff;
		if (roff != 0 || forcefault) {
			error = sam_map_fault(ip,
			    off & ~(mp->mi.m_dau[dt].seg[bt] - 1),
			    mp->mi.m_dau[dt].seg[bt], mapp);
			if (error != 0) {
				break;
			}
			forcefault = 0;
		}

		if (len > rlen) {
			len = rlen;
		}
		SAM_SET_LEASEFLG(mp);
		fbzero(SAM_ITOV(ip), off, len, &fbp);
		SAM_CLEAR_LEASEFLG(mp);

		rlen -= len;
		off += len;
		if (off < 0) {	/* if offset rolled into the sign bit */
			error = EFBIG;
			last_fbp = fbp;
			break;
		}

		if (rlen == 0) {
			last_fbp = fbp;
		} else {
			fbrelse(fbp, S_WRITE);
		}
		roff = 0;
	}

	if (rfbp && error == 0) {
		/*
		 * Caller has asked for this pointer
		 * to be returned.
		 */
		*rfbp = last_fbp;
	} else {
		fbrelse(last_fbp, S_WRITE);
	}
	return (error);
}
#endif
