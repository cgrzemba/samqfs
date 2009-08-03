/*
 * ----- rwio.c - Process the explicit read/write functions.
 *
 * Processes the SAM-QFS inode read/pread write/pwrite functions.
 * Processes the SAM-QFS raw read/write to disk and removable media.
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
#pragma ident "$Revision: 1.181 $"
#endif

#include "sam/osversion.h"


/* ----- UNIX Includes */
#ifdef sun
#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/sysmacros.h>
#include <sys/vnode.h>
#include <sys/fs_subr.h>
#include <sys/cpuvar.h>
#include <sys/systm.h>
#include <sys/atomic.h>

#include <vm/hat.h>
#include <vm/page.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_map.h>
#include <sys/policy.h>
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

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#include <linux/buffer_head.h>
#endif

#include <asm/param.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#endif  /* linux */

/* ----- SAMFS Includes */

#include "sam/types.h"
#ifdef sun
#include "sam/samaio.h"
#endif /* sun */

#include "macros.h"
#include "inode.h"
#include "mount.h"
#ifdef sun
#include "listio.h"
#include "extern.h"
#endif /* sun */
#ifdef linux
#include "clextern.h"
#endif  /* linux */
#include "rwio.h"
#include "debug.h"
#include "trace.h"
#ifdef sun
#include "qfs_log.h"
#endif /* sun */


#ifdef sun
static offset_t sam_dk_issue_direct_io(sam_node_t *ip, buf_descriptor_t *bdp,
	sam_ioblk_t *iop, enum uio_rw rw, uio_t *uiop, iov_descriptor_t *iovp);
static void sam_dk_direct_done(buf_t *bp);
static int sam_dk_fini_direct_io(sam_node_t *ip, enum uio_rw rw,
	uio_t *uiop, buf_descriptor_t *bdp);
static void sam_dk_wait_direct_io(sam_node_t *ip, buf_descriptor_t *bdp);
int sam_get_sparse_blocks(sam_node_t *ip, enum SPARSE_type flag, uio_t *uiop,
	int ioflag, cred_t *credp);
static int sam_lio_move(void *p, size_t n, enum uio_rw rw, struct uio *uio);
int sam_clear_append_after_map(sam_node_t *ip, offset_t size, offset_t offset,
	offset_t count);

#define	UIOMOVE(p, n, rw, uio, is_lio) \
	(is_lio ? sam_lio_move(p, n, rw, uio) : uiomove(p, n, rw, uio))

#pragma rarely_called(sam_get_sparse_blocks)

extern ulong_t lotsfree, pages_before_pager;


#endif /* sun */
#ifdef linux
int sam_get_sparse_blocks(sam_node_t *ip, enum SPARSE_type flag, uio_t *uiop,
	int ioflag, cred_t *credp);
ssize_t rfs_generic_file_write(struct file *file, const char *buf,
				size_t count, loff_t *ppos);
#endif /* linux */


/*
 * ----- sam_read_io - Read a SAM-QFS file.
 *
 * Map the file offset to the correct page boundary.
 * Move the data from the file data pages to the caller's buffer.
 */

#ifdef sun
int				/* ERRNO if error, 0 if successful. */
sam_read_io(
	vnode_t *vp,		/* Pointer to vnode */
	uio_t *uiop,		/* Pointer to user I/O vector array. */
	int ioflag)		/* Open flags */
{
	struct sam_listio_call *callp;
	int seg_flags, free_page;
	sam_u_offset_t offset, reloff;
	sam_ssize_t nbytes;	/* Num bytes to move from this block */
	caddr_t base;		/* Kernel address of mapped in block */
	int error;
	sam_node_t *ip;
	sam_u_offset_t buff_off;
	int is_lio = (ioflag == FASYNC &&
	    ((struct samaio_uio *)uiop)->type == SAMAIO_LIO_PAGED);


	ip = SAM_VTOI(vp);

start:

	if (is_lio) {
		struct samaio_uio *auiop = (struct samaio_uio *)uiop;

		callp = (struct sam_listio_call *)auiop->bp;
		while (callp->listio.file_len[0] == 0) {
			if (--callp->listio.file_list_count == 0) {
				return (0);
			}
			callp->listio.file_len++;
			callp->listio.file_off++;
		}
		uiop->uio_loffset = callp->listio.file_off[0];
		uiop->uio_resid = callp->listio.file_len[0];
	}

	for (;;) {

		/*
		 * A maximum MAXBSIZE (8k) number of bytes are transferred each
		 * pass.  It would be good if Solaris let MAXBSIZE be a bigger
		 * variable so more than 2 4k or 1 8k contiguous pages could be
		 * acquired.
		 */
		offset = uiop->uio_loffset;
		if ((offset_t)offset > ip->size) {
			return (0);
		}
		if (offset > SAM_MAXOFFSET_T) {
			return (EFBIG);
		}
		buff_off = offset & (offset_t)MAXBMASK;
		reloff = offset & ((offset_t)MAXBSIZE - 1);
		nbytes = MIN(((offset_t)MAXBSIZE - reloff), uiop->uio_resid);
		if (nbytes > (ip->size - offset)) {
			nbytes = ip->size - offset;
		}

		TRACE(T_SAM_READIO1, vp, (sam_tr_t)offset,
		    (sam_tr_t)reloff, nbytes);
		if (nbytes <= 0) {
			return (0);
		}
		if (ip->page_off == (offset & (sam_u_offset_t)PAGEMASK)) {
			free_page = 1;
		} else {
			free_page = 0;
		}

		/*
		 * For a removable media file, increment io_count. This
		 * is the activity counter for the daemon timeout.
		 */
		if (S_ISREQ(ip->di.mode)) {
			if (ip->rdev) {
				ip->io_count++;
			} else {
				if ((error = ip->rm_err) == 0) {
					error = ENODEV;
				}
				break;	/* If daemon cancel */
			}
		}
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);

		/*
		 * Map the file offset..nbytes and lock it. This may result in a
		 * call to getpage.
		 */
		seg_flags = 0;
		ASSERT(nbytes > 0);
		if (sam_vpm_enable) {

			SAM_SET_LEASEFLG(ip);
			error = vpm_data_copy(vp, offset, nbytes, uiop, 1, NULL,
			    0, S_READ);
			SAM_CLEAR_LEASEFLG(ip);

		} else {
			SAM_SET_LEASEFLG(ip);
			base = segmap_getmapflt(segkmap, vp, offset, nbytes,
			    1, S_READ);

			/*
			 * Move the data from the locked pages into the
			 * user's buffer
			 */
			error = UIOMOVE((base + reloff), nbytes, UIO_READ, uiop,
			    is_lio);
			SAM_CLEAR_LEASEFLG(ip);
		}
		if (error == 0) {
			if (((reloff + nbytes) == MAXBSIZE) && free_page &&
			    (freemem < (lotsfree + pages_before_pager))) {
				seg_flags = SM_FREE | SM_ASYNC | SM_DONTNEED;
			}
			if (sam_vpm_enable) {
				error = vpm_sync_pages(vp, buff_off, MAXBSIZE,
				    seg_flags);
			} else {
				error = segmap_release(segkmap, base,
				    seg_flags);
			}
		} else {
			if (sam_vpm_enable) {
				(void) vpm_sync_pages(vp, buff_off,
				    MAXBSIZE, 0);
			} else {
				(void) segmap_release(segkmap, base, 0);
			}
		}
		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		TRACE(T_SAM_READIO2, vp, (sam_tr_t)base,
		    (sam_tr_t)uiop->uio_resid,
		    seg_flags);
		if (ip->stage_err) {
			/*
			 * Return ip->stage_err if not failing over.
			 */
			if ((samgt.stagerd_pid >= 0) ||
			    !SAM_IS_SHARED_SERVER(ip->mp) ||
			    (ip->stage_err != ECOMM)) {
				error = ip->stage_err;
			}
		}

		if (error || (uiop->uio_resid <= 0)) {
			break;
		}
	}

	if (error == 0 && is_lio && --callp->listio.file_list_count) {
		callp->listio.file_off++;
		callp->listio.file_len++;
		goto start;
	}

	return (error);
}


/*
 * ----- sam_read_stage_n_io - Read a SAM-QFS file.
 *
 * Map the file offset to the correct page boundary.
 * Move the data from the stage_n data pages to the caller's buffer.
 */

int				/* ERRNO if error, 0 if successful. */
sam_read_stage_n_io(
	vnode_t *vp,		/* Pointer to vnode */
	uio_t *uiop)		/* Pointer to user I/O vector array. */
{
	sam_u_offset_t offset, reloff, s_off, s_len;
	sam_ssize_t nbytes;	/* Num bytes to move from this block */
	caddr_t base;		/* Kernel address of mapped in block */
	int error;
	sam_node_t *ip;

	ip = SAM_VTOI(vp);
	offset = uiop->uio_loffset;
	if ((offset_t)offset > ip->size) {
		return (0);
	}
	if (offset > SAM_MAXOFFSET_T) {
		return (EFBIG);
	}

	reloff = offset & ((sam_u_offset_t)PAGESIZE - 1);
	nbytes = uiop->uio_resid;
	if (nbytes > (ip->size - offset)) {
		nbytes = ip->size - offset;
	}
	/*
	 * Stage -n buffer is reset to the real values on close.
	 */
	if (ip->stage_len) {
		s_off = ip->stage_off;
		s_len = ip->stage_len;
	} else {
		s_off = ip->real_stage_off;
		s_len = ip->real_stage_len;
	}
	if ((offset < s_off) || (offset + nbytes) > (s_off + s_len)) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: stage -n: ino=%d.%d f=%x off=%llx len=%lld"
		    " soff=%lld slen=%lld",
		    ip->mp->mt.fi_name, ip->di.id.ino,
		    ip->di.id.gen, ip->flags.bits,
		    (offset_t)offset, (offset_t)nbytes, s_off, s_len);
		return (EIO);
	}

	TRACE(T_SAM_READ_N1, vp, (sam_tr_t)offset, (sam_tr_t)reloff, nbytes);
	if (nbytes <= 0) {
		return (0);
	}
	base = (caddr_t)ip->st_buf + ((offset - s_off) & PAGEMASK);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);

	/*
	 * Move the data from the stage_n memory pages into the user's buffer.
	 */
	ASSERT(nbytes > 0);
	SAM_SET_LEASEFLG(ip);
	error = uiomove((base + reloff), nbytes, UIO_READ, uiop);
	SAM_CLEAR_LEASEFLG(ip);

	RW_LOCK_OS(&ip->inode_rwl, RW_READER);
	if (ip->stage_err) {
		error = ip->stage_err;
	}
	TRACE(T_SAM_READ_N2, vp, (sam_tr_t)base,
	    (sam_tr_t)uiop->uio_resid, error);
	return (error);
}


/*
 * Fault in the pages of the first n bytes specified by the uio structure.
 * 1 byte in each page is touched and the uio struct is unmodified. Any
 * error will terminate the process as this is only a best attempt to get
 * the pages resident.
 *
 * This is copied from uio_prefaultpages().
 */
void
sam_uio_prefaultpages(ssize_t n, struct uio *uio)
{
	struct iovec *iov;
	ulong_t cnt, incr;
	caddr_t p;
	uint8_t tmp;
	int iovcnt;

	iov = uio->uio_iov;
	iovcnt = uio->uio_iovcnt;

	while ((n > 0) && (iovcnt > 0)) {
		cnt = MIN(iov->iov_len, n);
		if (cnt == 0) {
			/* empty iov entry */
			iov++;
			iovcnt--;
			continue;
		}
		n -= cnt;
		/*
		 * touch each page in this segment.
		 */
		p = iov->iov_base;
		while (cnt) {
			switch (uio->uio_segflg) {
			case UIO_USERSPACE:
			case UIO_USERISPACE:
				if (fuword8(p, &tmp))
					return;
				break;
			case UIO_SYSSPACE:
				if (kcopy(p, &tmp, 1))
					return;
				break;
			}
			incr = MIN(cnt, PAGESIZE);
			p += incr;
			cnt -= incr;
		}
		/*
		 * touch the last byte in case it straddles a page.
		 */
		p--;
		switch (uio->uio_segflg) {
		case UIO_USERSPACE:
		case UIO_USERISPACE:
			if (fuword8(p, &tmp))
				return;
			break;
		case UIO_SYSSPACE:
			if (kcopy(p, &tmp, 1))
				return;
			break;
		}
		iov++;
		iovcnt--;
	}
}


/*
 * ----- sam_write_io - Write a SAM-QFS file.
 *
 * Map the file offset to the correct page boundary.
 * Move the data from the caller's buffer to the file data pages.
 */

int				/* ERRNO if error, 0 if successful. */
sam_write_io(
	vnode_t *vp,		/* Pointer to vnode */
	uio_t *uiop,		/* Pointer to user I/O vector array. */
	int ioflag,		/* Open flags */
	cred_t *credp)		/* credentials pointer */
{
	struct sam_listio_call *callp;
	int pglock;
	boolean_t appending;
	int error, forcefault, seg_flags;
	sam_size_t nbytes;
	sam_u_offset_t offset, reloff, allocsz, orig_offset, segsz;
	sam_u_offset_t buff_off;
	caddr_t base;			/* Kernel address of mapped in block */
	sam_map_t type;
	sam_node_t *ip;
	int is_lio = (ioflag == FASYNC &&
	    ((struct samaio_uio *)uiop)->type == SAMAIO_LIO_PAGED);


	ip = SAM_VTOI(vp);

start:

	if (is_lio) {
		struct samaio_uio *auiop = (struct samaio_uio *)uiop;

		callp = (struct sam_listio_call *)auiop->bp;
		while (callp->listio.file_len[0] == 0) {
			if (--callp->listio.file_list_count == 0) {
				return (0);
			}
			callp->listio.file_len++;
			callp->listio.file_off++;
		}
		uiop->uio_loffset = callp->listio.file_off[0];
		uiop->uio_resid = callp->listio.file_len[0];
	}

	allocsz = uiop->uio_resid;
	orig_offset = uiop->uio_loffset;

	if (ip->di.rm.info.dk.allocahead) {
		if ((uiop->uio_loffset + uiop->uio_resid) > ip->cl_allocsz) {
			allocsz = (ip->di.rm.info.dk.allocahead *
			    SAM_ONE_MEGABYTE);
		}
	}
	if (SAM_IS_SHARED_CLIENT(ip->mp) && !SAM_IS_OBJECT_FILE(ip)) {
		if ((error = sam_map_block(ip, uiop->uio_loffset,
		    (offset_t)allocsz, SAM_ALLOC_BLOCK, NULL, credp))) {
			if ((error == ELNRNG) && S_ISREG(ip->di.mode)) {
				error = sam_get_sparse_blocks(ip,
				    SPARSE_zeroall, uiop, ioflag, credp);
			}
			if (error) {
				return (error);
			}
		}
	}
	for (;;) {

		/*
		 * A maximum MAXBSIZE (8k) number of bytes are transferred each
		 * pass.  It would be good if Solaris let MAXBSIZE be a bigger
		 * variable so more than 2 4k or 1 8k contiguous pages could be
		 * acquired.
		 */
		offset = uiop->uio_loffset;
		reloff = offset & ((offset_t)MAXBSIZE - 1);
		buff_off = offset & (offset_t)MAXBMASK;
		nbytes = MIN(((offset_t)MAXBSIZE - reloff), uiop->uio_resid);

		TRACE(T_SAM_WRITEIO1, vp, (sam_tr_t)offset,
		    (sam_tr_t)reloff, nbytes);
		if ((appending = (offset + (offset_t)nbytes) > ip->size) != 0) {
			ip->flush_len += nbytes;
			if (ip->flush_off == 0) {
				ip->flush_off = offset & PAGEMASK;
			}
		} else {
			ip->flush_len = 0;
			ip->flush_off = 0;
		}

		/*
		 * If appending at the beginning of a segment, rewriting a full
		 * segment, or creating a segment past the last segment
		 * (sparse), create page(s) in memory to avoid read before
		 * write.
		 */
		if (appending || nbytes == MAXBSIZE) {
			if (ip->di.status.b.on_large ||
			    offset >= SM_OFF(ip->mp, ip->di.status.b.meta)) {
				segsz = (sam_u_offset_t)MAXBSIZE;
			} else {
				segsz =
				    ip->mp->mi.m_dau[
				    ip->di.status.b.meta].seg[SM];
			}
			if (offset > ((ip->size + segsz - 1) & ~(segsz - 1))) {
				forcefault = 0;
			} else {
				forcefault = (reloff != 0);
			}
		} else {
			forcefault = 1;
		}
		type = forcefault ? SAM_FORCEFAULT : SAM_WRITE;

		/*
		 * Allocate blocks to cover the offset..nbytes.
		 */
		if (!S_ISREQ(ip->di.mode)) {
			int32_t no_blocks = ip->di.blocks;

			if ((error = sam_map_block(ip, uiop->uio_loffset,
			    (offset_t)nbytes, type, NULL, credp))) {

				/*
				 * Unless this is a QFS filesystem and we've
				 * successfully allocated/mapped some storage,
				 * return the error.  Otherwise, do what I/O we
				 * can (and return a short write).
				 */
				if (!IS_SAM_ENOSPC(error)) {
					return (error); /* "real" error */
				}
				if (!SAM_IS_STANDALONE(ip->mp)) {
					return (error);
				}
				if (uiop->uio_loffset == orig_offset) {
					return (error);
				}
				error = 0;	/* QFS && short write */
				break;
			}
#ifdef METADATA_SERVER
			if ((ioflag & (FSYNC|FDSYNC)) &&
			    (ip->di.blocks != no_blocks) &&
			    !SAM_FORCE_NFS_ASYNC(ip->mp)) {
				error = sam_flush_extents(ip);
			}
#endif
		} else {

			/*
			 * For a removable media file, increment io_count. This
			 * is the activity counter for the daemon timeout.
			 */
			if (ip->rdev) {
				ip->io_count++;
			} else {
				if ((error = ip->rm_err) == 0) {
					error = ENODEV;
				}
				return (error);	/* If daemon cancel */
			}
			if ((error = sam_rmmap_block(ip, uiop->uio_loffset,
			    (offset_t)nbytes, type, NULL))) {
				return (error);
			}
		}

		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

		/*
		 * Map the file offset..nbytes and lock it. This may result in a
		 * call to getpage.
		 */
		pglock = 0;
		seg_flags = 0;
		ASSERT(nbytes > 0);

		/*
		 * Touch the page and fault it in if it is not in core
		 * before segmap_getmapflt or vpm_data_copy can lock it.
		 * This is to avoid a deadlock if the buffer is mapped
		 * to the same file through mmap which we want to
		 * write.
		 */
		SAM_SET_LEASEFLG(ip);
		sam_uio_prefaultpages(nbytes, uiop);
		SAM_CLEAR_LEASEFLG(ip);

		if (sam_vpm_enable) {

			SAM_SET_LEASEFLG(ip);
			error = vpm_data_copy(vp, offset, nbytes, uiop,
			    forcefault, NULL, 0, S_WRITE);
			SAM_CLEAR_LEASEFLG(ip);
		} else {
			SAM_SET_LEASEFLG(ip);
			base = segmap_getmapflt(segkmap, vp, offset, nbytes,
			    forcefault, S_WRITE);

			if (forcefault == 0) {
				pglock = segmap_pagecreate(segkmap, base,
				    nbytes, 0);
			}

			/*
			 * Move the data from the user's buffer into the
			 * locked pages.
			 */
			error = UIOMOVE((base + reloff), nbytes, UIO_WRITE,
			    uiop, is_lio);
			SAM_CLEAR_LEASEFLG(ip);
		}

		if (error) {
			seg_flags = SM_INVAL;	/* Invalidate pages */
		} else if ((ioflag & (FSYNC|FDSYNC)) &&
		    !SAM_FORCE_NFS_ASYNC(ip->mp)) {
			/* Write through to disk (sync pages) */
			seg_flags = SM_WRITE;
		} else if ((reloff + nbytes) == MAXBSIZE && !is_lio) {
			/* Flush out full blocks asynchronously if not listio */
			seg_flags = SM_WRITE | SM_ASYNC | SM_DONTNEED;
		}
		if (sam_vpm_enable) {
			(void) vpm_sync_pages(vp, buff_off, MAXBSIZE,
			    seg_flags);
		} else {
			if (pglock) {
				segmap_pageunlock(segkmap, base, nbytes,
				    S_WRITE);
			}
			(void) segmap_release(segkmap, base, seg_flags);
		}

		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		TRACE(T_SAM_WRITEIO2, vp, (sam_tr_t)base,
		    (sam_tr_t)uiop->uio_resid,
		    (sam_tr_t)reloff);

		if (error || (uiop->uio_resid <= 0)) {
			break;
		}
	}

	/*
	 * Zero from the new EOF to a .seg boundary.
	 * This operation is normally done by sam_clear_append at
	 * DKMAP5, but only if type == SAM_WRITE.
	 */
	if (type == SAM_FORCEFAULT && appending && error == 0) {
		struct fbuf *fbp;
		int zlen;

		if (!ip->di.status.b.on_large &&
		    ip->size < SM_OFF(ip->mp, ip->di.status.b.meta)) {

			zlen = roundup(ip->size,
			    ip->mp->mi.m_dau[ip->di.status.b.meta].seg[SM]) -
			    ip->size;

			if (zlen > 0) {
				SAM_SET_LEASEFLG(ip);
				fbzero(SAM_ITOV(ip), ip->size, zlen, &fbp);
				SAM_CLEAR_LEASEFLG(ip);
				fbrelse(fbp, S_WRITE);
			}
		}
	}

	if (error == 0 && is_lio && --callp->listio.file_list_count) {
		callp->listio.file_off++;
		callp->listio.file_len++;
		goto start;
	}

	/*
	 * If successful write, set times and size. If error, truncate
	 * file to return blocks allocated.
	 */
	if (error == 0) {
		if (ip->flags.b.staging == 0) {
			if (ip->size > ip->di.rm.size) {
				ip->di.rm.size = ip->size;
			}
		}
		TRANS_INODE(ip->mp, ip);
		sam_mark_ino(ip, (SAM_UPDATED | SAM_CHANGED));
		/* If not superuser, clear setuid/setgid if execute bits set */
		if ((ip->di.mode & (S_ISUID | S_ISGID)) &&
		    (ip->di.mode & (S_IXUSR | S_IXGRP | S_IXOTH)) &&
		    (secpolicy_vnode_setid_retain(credp,
		    (ip->di.mode & S_ISUID) && (ip->di.uid == 0)))) {
			ip->di.mode &= ~(S_ISUID | S_ISGID);
		}
	} else {
		if (is_lio) {
			/*
			 * Update listio cursor to reflect transfer thus far.
			 */
			callp->listio.file_off[0] = uiop->uio_loffset;
			callp->listio.file_len[0] = uiop->uio_resid;
		}
		if (ip->di.rm.size < ip->size) {
			(void) sam_truncate_ino(ip, ip->di.rm.size,
			    SAM_TRUNCATE, credp);
		}
	}

	/*
	 * Flush behind to make sequentially written pages clean.
	 */
	if ((error == 0) && appending && ip->mp->mt.fi_flush_behind &&
	    (ip->flush_len > ip->mp->mt.fi_flush_behind)) {
		(void) sam_flush_behind(ip, credp);
	}

	return (error);
}


/*
 * ----- sam_flush_behind - flush behind write/stage pages
 *
 * Asynchronously write behind pages which are dirty.  This helps
 * Solaris VM keep pages clean.
 */

void
sam_flush_behind(
	sam_node_t *ip,		/* Pointer to vnode */
	cred_t *credp)		/* credentials pointer */
{
	ip->flush_len &= ~((sam_size_t)(MAXBSIZE - 1));
	(void) VOP_PUTPAGE_OS(SAM_ITOV(ip), ip->flush_off, ip->flush_len,
	    B_WRITE | B_ASYNC, credp, NULL);
	ip->flush_off += (offset_t)ip->flush_len;
	ip->flush_len = 0;
}


/*
 * ----- sam_dk_direct_io - Read/Write a SAM-QFS disk file using raw I/O.
 *
 * WRITE: Move the data from the caller's buffer to the device using
 * the cache buffers or the device driver.
 * READ: Move the data from the device to the caller's buffer using
 * the cache buffers or the device driver.
 */

int				/* ERRNO if error, 0 if successful. */
sam_dk_direct_io(
	sam_node_t *ip,		/* Pointer to inode */
	enum uio_rw rw,		/* UIO_READ or UIO_WRITE */
	int ioflag,		/* file I/O flags (/usr/include/sys/file.h). */
	uio_t *uiop,		/* Pointer to user I/O vector array. */
	cred_t *credp)		/* credentials pointer. */
{
	sam_ioblk_t ioblk;
	buf_descriptor_t bd_desc;
	iov_descriptor_t *iovp;
	buf_descriptor_t *bdp;
	int unallocated_read = 0;
	sam_map_t map_flag;
	enum seg_rw lock_type;
	vnode_t *vp;
	size_t len, len_delta;
	krw_t rw_type;	/* type of hold on ip->inode_rwl on entry */
	boolean_t ilock_held;
	offset_t undelegated_bytes;
	int i;
	int error = 0, bdp_error;

	vp = SAM_ITOV(ip);
	ilock_held = TRUE;
	if ((ioflag & FASYNC) || (uiop->uio_iovcnt > 1)) {
		bdp = (buf_descriptor_t *)kmem_alloc(sizeof (buf_descriptor_t) +
		    ((uiop->uio_iovcnt - 1) * sizeof (iov_descriptor_t)),
		    KM_SLEEP);
	} else {
		bdp = &bd_desc;
	}
	if (ioflag & FASYNC) {
		ASSERT(ip->flags.b.directio);
		bdp->aiouiop = (struct samaio_uio *)uiop;
	} else {
		bdp->aiouiop = NULL;
	}
	if (uiop->uio_segflg != UIO_SYSSPACE) {	/* User address space? */
		bdp->procp = ttoproc(curthread);
		bdp->asp = bdp->procp->p_as;
	} else {
		bdp->procp = NULL;
		bdp->asp = &kas;
		TRACE(T_SAM_DIOKERN, SAM_ITOV(ip), (sam_tr_t)uiop,
		    vp->v_rdev, (sam_tr_t)bdp);
	}
	sam_mutex_init(&bdp->bdp_mutex, NULL, MUTEX_DEFAULT, NULL);
	sema_init(&bdp->io_sema, 0, NULL, SEMA_DEFAULT, NULL);
	bdp->ip = ip;
	undelegated_bytes = uiop->uio_resid;
	bdp->remaining_length = uiop->uio_resid;
	bdp->resid = uiop->uio_resid;
	bdp->write_bytes_outstanding = 0;
	bdp->iov_count = uiop->uio_iovcnt;
	iovp = &bdp->iov[0];
	for (i = 0; i < uiop->uio_iovcnt; iovp++, i++) {
		iovp->pagelock_failed = TRUE;
	}
	iovp = &bdp->iov[0];
	bdp->error = -1;	/* negative value indicates no I/O issued yet */
	bdp->io_count = 0;
	bdp->rem = 0;
	SAM_BUF_LINK_INIT(&bdp->dio_link);

	if ((sam_u_offset_t)uiop->uio_loffset > SAM_MAXOFFSET_T) {
		error = EFBIG;
		goto done;
	}

	if (rw == UIO_READ) {
		offset_t rem = MAX(ip->size - uiop->uio_loffset, 0);

		map_flag = SAM_RD_DIRECT_IO;
		rw_type = RW_READER;
		if ((offset_t)uiop->uio_resid > rem) { /* If reading past EOF */
			bdp->rem = (offset_t)uiop->uio_resid - rem;
			uiop->uio_resid = (ssize_t)rem;
		}
		if (ip->di.id.ino == SAM_INO_INO) {
			if ((uiop->uio_resid & (ip->mp->mt.fi_rd_ino_buf_size -
			    1)) ||
			    (uiop->uio_loffset &
			    (ip->mp->mt.fi_rd_ino_buf_size - 1))) {
				error = EINVAL;
				goto done;
			}
		}
		if (rem <= 0) {
			error = 0;
			goto done;
		}

	} else {			/* For write, allocate blocks */
		offset_t allocsz;

		map_flag = SAM_WR_DIRECT_IO;
		rw_type = RW_WRITER;
		offset_t prev_filesize;
		if (ip->di.id.ino == SAM_INO_INO) {
			error = EINVAL;
			goto done;
		}
		prev_filesize = ip->size;
		allocsz = uiop->uio_resid;
		if (ip->di.rm.info.dk.allocahead) {
			if ((uiop->uio_loffset + uiop->uio_resid) >
			    ip->cl_allocsz) {
				allocsz = (ip->di.rm.info.dk.allocahead *
				    SAM_ONE_MEGABYTE);
			}
		}
		if (allocsz) {
			if (SAM_IS_SHARED_CLIENT(ip->mp)) {
				if ((error = sam_map_block(ip,
				    uiop->uio_loffset,
				    (offset_t)uiop->uio_resid,
				    SAM_WRITE_BLOCK, NULL, credp))) {
					if ((error == ELNRNG) &&
					    S_ISREG(ip->di.mode)) {
						enum SPARSE_type
						    zero_sparse_flag;

						if (ip->mp->mt.fi_config &
						    MT_ZERO_DIO_SPARSE) {
							zero_sparse_flag =
							    SPARSE_directio;
						} else {
							zero_sparse_flag =
							    SPARSE_nozero;
						}
						error = sam_get_sparse_blocks(
						    ip, zero_sparse_flag,
						    uiop, FWRITE, credp);
					}
					if (error) {
						goto done;
					}
				}
			} else {
				sam_map_t mflag;

				if ((ip->mp->mt.fi_config &
				    MT_ZERO_DIO_SPARSE) &&
				    (uiop->uio_loffset < ip->size)) {
					/*
					 * Zero all newly allocated blocks.
					 */
					mflag = SAM_ALLOC_ZERO;
				} else {
					/*
					 * Doesn't zero any new blocks.
					 */
					mflag = SAM_WRITE_BLOCK;
				}

				error = sam_map_block(ip, uiop->uio_loffset,
				    (offset_t)uiop->uio_resid, mflag, NULL,
				    credp);

				if (!error && (mflag == SAM_ALLOC_ZERO) &&
				    (ip->size > prev_filesize)) {
					/*
					 * Blocks beyond the old file size have
					 * been allocated and zeroed. Set
					 * ip->zero_end so they don't get zeroed
					 * again.
					 */
					ip->zero_end = sam_round_to_dau(ip,
					    ip->size, SAM_ROUND_UP);
				}
				if (error) {
					goto done;
				}
			}
		}

		/*
		 * MT_ZERO_DIO_SPARSE flag says to zero newly created
		 * sparse regions created via direct IO including
		 * up to a MAXBSIZE boundary beyond EOF. Only do this
		 * when not staging or when the last write occurs
		 * when staging.
		 */
		if (((uiop->uio_loffset + uiop->uio_resid) > prev_filesize) &&
		    (ip->mp->mt.fi_config & MT_ZERO_DIO_SPARSE) &&
		    (!ip->flags.b.staging || (ip->size >= ip->di.rm.size))) {
			/*
			 * Zero from size to the end of the dau that
			 * contains prev_filesize, zero the dau that contains
			 * uiop->uio_loffset and zero the dau that contains
			 * uiop->uio_loffset + uiop->uio_resid.
			 */
			error = sam_clear_append_after_map(ip, prev_filesize,
			    uiop->uio_loffset, uiop->uio_resid);
		}
	}

	if (vn_has_cached_data(vp)) {		/* if pages exist */
		offset_t offset;

		/*
		 * If pages exist, flush and invalidate them as they may
		 * conflict with the direct I/O.  We used to flush and
		 * invalidate all pages of the file but the flush could be paced
		 * by a paged I/O process.  Instead just flush the pages we
		 * need.
		 */
		if (rw == UIO_READ) {
			/*
			 * Drop the inode_rwl READER, sam_putpages will
			 * re-acquire.  We still hold ip->data_rwl, RW_READER.
			 */
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			ilock_held = FALSE;
		}
		offset = uiop->uio_loffset;
		len = uiop->uio_resid;
		/*
		 * Make sure offset is on a page boundary.
		 */
		if ((offset & (offset_t)(PAGESIZE - 1)) != 0) {
			len_delta = offset & (offset_t)(PAGESIZE - 1);
			offset &= ~((offset_t)(PAGESIZE - 1));
			len += len_delta;
		}
		TRACE(T_SAM_DIOPAGES, vp, offset, len, len_delta);
		(void) VOP_PUTPAGE_OS(vp, offset, len, B_INVAL, credp, NULL);
	}

	lock_type = rw == UIO_READ ? S_WRITE : S_READ;

	if (ilock_held) {
		RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
		ilock_held = FALSE;
	}

	while (uiop->uio_iovcnt > 0) {
		iovec_t *iov = uiop->uio_iov;
		boolean_t lock_pages;
		offset_t vblen;		/* iovp->blength */

		/*
		 * Use of as_pagelock for direct I/O is required for even
		 * mlock'ed memory.
		 */
		iovp->base = iov->iov_base;
		iovp->blength = vblen = iov->iov_len;
		iovp->pagelock_failed = FALSE;
		if (rw == UIO_WRITE) {
			mutex_enter(&ip->write_mutex);
			ip->cnt_writes += vblen;
			mutex_exit(&ip->write_mutex);
			bdp->write_bytes_outstanding += vblen;
		}
		/*
		 * If ASYNC, aphysio has already done an as_pagelock. The pplist
		 * is in the aio buffer.
		 */
		lock_pages = TRUE;
		if (ioflag & FASYNC) {
			if (bdp->aiouiop->type == SAMAIO_CHR_AIO) {
				iovp->pplist = bdp->aiouiop->bp->b_shadow;
				lock_pages = FALSE;
			}
		}
		if (lock_pages) {
			if ((error = as_pagelock(bdp->asp,
			    &(iovp->pplist), iovp->base, vblen, lock_type))) {
				iovp->base = 0;
				iovp->blength = vblen = 0;
				iovp->pplist = NULL;
				iovp->pagelock_failed = TRUE;
				if (SAM_IS_OBJECT_FILE(ip)) {
					goto done;
				}
			}
		}

		while (iov->iov_len > 0) {
			bdp->length = iov->iov_len;
			if (bdp->length > uiop->uio_resid) {
				bdp->length = uiop->uio_resid;
			}

			RW_LOCK_OS(&ip->inode_rwl, RW_READER);
			/*
			 * Check if staging has been terminated. If so, exit
			 * immediately.
			 */
			if ((ip->stage_pid > 0) &&
			    (ip->stage_pid == SAM_CUR_PID) &&
			    !ip->flags.b.staging) {
				error = ECANCELED;
				if (ip->stage_err == 0) {
					ip->stage_err = ECANCELED;
				}
				RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
				goto done;
			}
			if ((error = sam_map_block(ip, uiop->uio_loffset,
			    (offset_t)bdp->length, map_flag, &ioblk, credp))) {

				if ((error == ELNRNG) && (rw == UIO_READ)) {
					/*
					 * unallocated read
					 */
					error = 0;
					ioblk.count = bdp->length;
					ioblk.blkno = 0;
					ioblk.ord = 0;
					unallocated_read = 1;
					RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
					goto unallocated_read;

				} else {

					RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
					goto done;
				}
			}
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);

			if (ioblk.count == 0) {
				goto done;
			}
			if (ioblk.blkno < ip->mp->mi.m_fs[ioblk.ord].system) {
				if (rw == UIO_WRITE || ioblk.blkno > 0) {
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: direct %s i/o: "
					    "bn=%llx, ord=%d,"
					    " ino=%d.%d, sys=%x, off=%lld",
					    ip->mp->mt.fi_name,
					    rw == UIO_WRITE ? "write" : "read",
					    (long long)ioblk.blkno,
					    (int)ioblk.ord,
					    ip->di.id.ino, ip->di.id.gen,
					    ip->mp->mi.m_fs[ioblk.ord].system,
					    uiop->uio_loffset);
					sam_req_ifsck(ip->mp, ioblk.ord,
					    "sam_dk_direct_io: bad block",
					    &ip->di.id);
					error = EDOM;
					goto done;
				} else
					unallocated_read = 1;
			}
unallocated_read:
			if (unallocated_read) {
				/* Move zeros into the user buffer */
				offset_t dau_size, dau_end;
				short dt = ip->di.status.b.meta;

				TRACE(T_SAM_DIOUAR, SAM_ITOV(ip),
				    (sam_tr_t)uiop->uio_loffset,
				    (sam_tr_t)uiop->uio_resid,
				    (sam_tr_t)ip->size);
				if (!ip->di.status.b.on_large &&
				    uiop->uio_loffset < SM_OFF(ip->mp, dt)) {
					dau_size = SM_BLK(ip->mp, dt);
					dau_end = roundup(uiop->uio_loffset,
					    dau_size);
				} else {
					dau_size = LG_BLK(ip->mp, dt);
					if (ip->di.status.b.on_large) {
						dau_end =
						    roundup(uiop->uio_loffset,
						    dau_size);
					} else {
						dau_end = roundup(
						    (uiop->uio_loffset -
						    SM_OFF(ip->mp, dt)),
						    dau_size) +
						    SM_OFF(ip->mp, dt);
					}
				}
				if (dau_end > uiop->uio_loffset) {
					/*
					 * Not on a block boundary.  The lesser
					 * of uiop->uio_resid and the remaining
					 * bytes in the current (unallocated)
					 * block, the next block may exist.
					 */
					ioblk.count = MIN((dau_end -
					    uiop->uio_loffset),
					    uiop->uio_resid);

				} else {
					/*
					 * On a block boundary.  No more than
					 * the size of the current (unallocated)
					 * block, the next block may exist.
					 */
					ioblk.count = MIN(dau_size,
					    uiop->uio_resid);
				}
				/*
				 * We only have sam_zero_block_size zeros.
				 */
				ioblk.count = MIN(ioblk.count,
				    sam_zero_block_size);
				ASSERT(ioblk.count > 0);
				SAM_SET_LEASEFLG(ip);
				error = uiomove(sam_zero_block, ioblk.count,
				    UIO_READ, uiop);
				SAM_CLEAR_LEASEFLG(ip);
				if (error) {
					goto done;
				}
				unallocated_read = 0;
			} else if ((ioblk.dev_bsize != SAM_OSD_BSIZE) &&
			    ((ioblk.pboff & (DEV_BSIZE - 1)) ||
			    (ioblk.count < DEV_BSIZE) ||
			    iovp->pagelock_failed)) {
				buf_t *bp;
				sam_daddr_t blkno;	/* Block # on device */

				ASSERT(!(SAM_IS_OBJECT_FILE(ip)));
				/*
				 * Block offset not sector multiple, count <
				 * sector multiple, buffer not on short
				 * boundary, or memory lockdown failed, must
				 * read-modify-write sector.
				 */
				if (!(ioflag & FASYNC)) {
					/*
					 * If not asynch I/O, must wait for
					 * previous I/O to complete to preserve
					 * POSIX semantics.
					 */
					sam_dk_wait_direct_io(ip, bdp);
				}
				blkno = (ioblk.blkno << SAM2SUN_BSHIFT) +
				    (sam_daddr_t)(ioblk.pboff / DEV_BSIZE);
				if ((error = sam_bread_db(ip->mp,
				    ip->mp->mi.m_fs[ioblk.ord].dev,
				    blkno, DEV_BSIZE, &bp)) != 0) {
					goto done;
				}
				ioblk.pboff = (ioblk.pboff & (DEV_BSIZE - 1));
				ioblk.count = MIN(ioblk.count,
				    (DEV_BSIZE - ioblk.pboff));
				ASSERT(ioblk.count > 0);

				SAM_SET_LEASEFLG(ip);
				error = uiomove((bp->b_un.b_addr + ioblk.pboff),
				    ioblk.count, rw, uiop);
				SAM_CLEAR_LEASEFLG(ip);
				if (rw == UIO_WRITE && error == 0) {
					error = SAM_BWRITE2(ip->mp, bp);
				}
				bp->b_flags |= B_STALE | B_AGE;
				brelse(bp);
				if (error) {
					goto done;
				}
			} else {
				offset_t start_len, iov_len;
				/*
				 * Block offset is sector multiple, count is a
				 * sector multiple, and buffer is on short
				 * boundary, issue direct async I/O.
				 */
				start_len = iov->iov_len;
				iov_len = sam_dk_issue_direct_io(ip, bdp,
				    &ioblk, rw, uiop, iovp);
				undelegated_bytes -= (start_len - iov_len);
				if (undelegated_bytes == 0) {
					/*
					 * In an asynchronous i/o operation,
					 * bdp, uiop, and iov may be freed by
					 * completion of issued i/o.  This
					 * 'goto' prevents them from being
					 * accessed by the tail of the loop.
					 */
					goto done;
				}
			}
		}
		uiop->uio_iov++;
		uiop->uio_iovcnt--;
		iovp++;
	}

done:
	/*
	 * We may be holding the inode_rwl for the error path.
	 * We will not be holding the inode_rwl for the normal path.
	 */
	if (ilock_held) {
		RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
	}
	if (ioflag & FASYNC) {
		if (undelegated_bytes) {
			if (error) {
				mutex_enter(&bdp->bdp_mutex);
				if (bdp->error <= 0) {
					/* Don't overwrite any errors */
					/* reported by actual I/O. */
					bdp->error = error;
				}
				mutex_exit(&bdp->bdp_mutex);
			}
			sam_dk_aio_direct_done(ip, bdp, undelegated_bytes);
		}
		RW_LOCK_OS(&ip->inode_rwl, rw_type);
		return (0);
	}

	sam_dk_wait_direct_io(ip, bdp);
	bdp_error = sam_dk_fini_direct_io(ip, rw, uiop, bdp);
	if (bdp->iov_count > 1) {
		int iov_count = bdp->iov_count;

		kmem_free(bdp, sizeof (buf_descriptor_t) +
		    ((iov_count - 1) * sizeof (iov_descriptor_t)));
	}
	RW_LOCK_OS(&ip->inode_rwl, rw_type);
	/* I/O errors dominate mainline */
	return (bdp_error ? bdp_error : error);
}


/*
 * ----- sam_dk_fini_direct_io - finish direct I/O.
 */

static int
sam_dk_fini_direct_io(
	sam_node_t *ip,		/* Pointer to inode */
	enum uio_rw rw,
	uio_t *uiop,		/* Pointer to user I/O vector array. */
	buf_descriptor_t *bdp)
{
	/*
	 * Unlock the address space, and toss the iov list, if used.
	 * For asynchronous i/o, as_pageunlock is done in aio_done, not here.
	 */
	if (!(bdp->aiouiop && bdp->aiouiop->type == SAMAIO_CHR_AIO)) {
		int i;
		iov_descriptor_t *iovp;
		enum seg_rw lock_type;

		lock_type = (rw == UIO_READ) ? S_WRITE : S_READ;

		for (i = 0, iovp = bdp->iov; i < bdp->iov_count; i++, iovp++) {
			if (!iovp->pagelock_failed) {
				as_pageunlock(bdp->asp, iovp->pplist,
				    iovp->base,
				    iovp->blength, lock_type);
			}
		}
	}

	if (bdp->write_bytes_outstanding) {
		/*
		 * If outstanding write bytes fall below threshold, wake up
		 * writers.
		 */
		mutex_enter(&ip->write_mutex);
		sam_write_done(ip, bdp->write_bytes_outstanding);
		mutex_exit(&ip->write_mutex);
	}

	/*
	 * If bdp->error < 0, we didn't get as far as issuing an I/O.
	 * In this case, don't try to update file size, but for reads,
	 * set caller's residual to the number of bytes which we
	 * didn't transfer.
	 */

	if (rw == UIO_WRITE) {
		if (bdp->error > 0) {
			uiop->uio_resid = bdp->resid;
			if (ip->di.rm.size < ip->size) {
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				(void) sam_truncate_ino(ip, ip->di.rm.size,
				    SAM_TRUNCATE, CRED());
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			}
		} else if (bdp->error == 0) {
			/* XXX could return partial success. */
			uiop->uio_resid = 0;
			if (ip->flags.b.staging == 0) {
				if (ip->size > ip->di.rm.size) {
					RW_LOCK_OS(&ip->inode_rwl, RW_READER);
					mutex_enter(&ip->fl_mutex);
					ip->di.rm.size = ip->size;
					mutex_exit(&ip->fl_mutex);
					RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
				}
			}
		}
	} else {	/* UIO_READ case */
		if (bdp->error > 0) {
			uiop->uio_resid = bdp->resid;
		} else {
			uiop->uio_resid = bdp->rem;
		}
	}
	sema_destroy(&bdp->io_sema);
	mutex_destroy(&bdp->bdp_mutex);

	return ((bdp->error >= 0) ? bdp->error : 0);
}


/*
 * ----- sam_dk_wait_direct_io - wait for async direct I/O.
 *
 * The use of bdp_mutex mutex here serves a non-obvious purpose.  While it
 * is not necessary for synchronization of accesses to bdp->io_count or
 * other fields, it prevents this routine from returning (and freeing the
 * bdp structure) before the last thread which decrements bdp->io_count has
 * completed its last access to bdp.
 */

static void
sam_dk_wait_direct_io(sam_node_t *ip, buf_descriptor_t *bdp)
{
	struct cpu *cpup;

	/*
	 * Wait until I/O done.
	 */
	cpup = CPU;
	mutex_enter(&bdp->bdp_mutex);
	while (bdp->io_count) {
		TRACE(T_SAM_DIOWAIT, SAM_ITOV(ip), bdp->error, bdp->io_count,
		    (sam_tr_t)bdp);
		mutex_exit(&bdp->bdp_mutex);
		atomic_add_64(&cpup->cpu_stats.sys.iowait, 1);
		if (panicstr) {
			while (bdp->io_count) {
				drv_usecwait(10);
			}
		} else {
			sema_p(&bdp->io_sema);	/* Wait until all I/Os done */
		}
		atomic_add_64(&cpup->cpu_stats.sys.iowait, -1);
		mutex_enter(&bdp->bdp_mutex);
	}
	mutex_exit(&bdp->bdp_mutex);
}


/*
 * ----- sam_dk_issue_direct_io - issue async direct I/O.
 */

static offset_t
sam_dk_issue_direct_io(
	sam_node_t *ip,		/* Pointer to inode */
	buf_descriptor_t *bdp,
	sam_ioblk_t *iop,
	enum uio_rw rw,
	uio_t *uiop,		/* Pointer to user I/O vector array. */
	iov_descriptor_t *iovp)
{
	buf_t *bp;
	offset_t contig, iop_contig;
	uint32_t int_contig;
	int bflags = B_PHYS | B_BUSY | B_ASYNC;
	offset_t blkno;			/* block # on device (union) */
	iovec_t *iov = uiop->uio_iov;
	sam_buf_t *sbp;
	offset_t saved_iov_len = iov->iov_len;

	iop->contig -= (sam_size_t)iop->pboff;
	/*
	 * Adjust space to requested I/O size (uiop->uio_resid).
	 */
	if (iop->contig > (sam_size_t)uiop->uio_resid) {
		iop->contig = (sam_size_t)(uiop->uio_resid &
		    ~((ssize_t)(iop->dev_bsize - 1)));
	}

	if (rw == UIO_WRITE) {
		bflags |= B_WRITE;
		if (ip->flags.b.abr && S_ISREG(ip->di.mode)) {
			bflags |= B_ABRWRITE;
		}
	} else {
		bflags |= B_READ;
	}

	if (bdp->error < 0) {
		bdp->error = 0;
	}

	for (;;) {
		offset_t saved_uio_resid;

		bp = sam_get_buf_header();
		sbp = (sam_buf_t *)bp;

		contig = iop->contig;
		if ((bdp->length & ~((offset_t)
		    (iop->dev_bsize - 1))) < contig) {
			contig = (bdp->length &
			    ~((offset_t)(iop->dev_bsize - 1)));
		}
		if (iop->num_group > 1) {
			if ((offset_t)(iop->bsize - iop->pboff) < contig) {
				contig = (offset_t)(iop->bsize - iop->pboff);
			}
		}
		bp->b_proc = bdp->procp;
		bp->b_edev = ip->mp->mi.m_fs[iop->ord].dev;
		bp->b_dev = cmpdev(bp->b_edev);
		blkno = (iop->blkno << SAM2SUN_BSHIFT) +
		    (offset_t)(iop->pboff / iop->dev_bsize);
		bp->b_lblkno = blkno;
		bp->b_un.b_addr = (caddr_t)iov->iov_base;
		bp->b_bcount = (size_t)contig;
		bp->b_flags = bflags;
		bp->b_file = SAM_ITOP(ip);
		bp->b_offset = uiop->uio_loffset;

		/*
		 * If as_pagelock provided us with a pagelist (an array of
		 * all pages in the I/O range, correct to the start of this
		 * bufs pages, and indicate the list is present.
		 */
		if (iovp->pplist != NULL) {
			bp->b_flags |= B_SHADOW;
			bp->b_shadow = iovp->pplist +
			    btop((uintptr_t)iov->iov_base -
			    ((uintptr_t)iovp->base & PAGEMASK));
		}
		if (bp->b_bcount > ip->mp->mi.m_maxphys) {
			bp->b_bcount = ip->mp->mi.m_maxphys;
		}

		/*
		 * Reset contig since minphys() can reduce bp->b_bcount to the
		 * value suitable for the driver.
		 */
		contig = (size_t)bp->b_bcount;
		int_contig = (uint32_t)contig;
		ASSERT((contig - (offset_t)int_contig) == 0);
		bp->b_bufsize = (size_t)contig;
		bp->b_vp = (vnode_t *)bdp;
		ASSERT(bp->b_bcount > 0);
		bp->b_iodone = (int (*) ()) sam_dk_direct_done;

		mutex_enter(&bdp->bdp_mutex);
		bdp->io_count++;
		SAM_BUF_ENQ(&bdp->dio_link, &sbp->link);
		mutex_exit(&bdp->bdp_mutex);

		iov->iov_base += contig;
		iov->iov_len -= int_contig;
		bdp->length = iov->iov_len;
		uiop->uio_loffset += contig;
		uiop->uio_resid -= int_contig;
		iop_contig = iop->contig - contig;
		iop->contig = (size_t)iop_contig;

		saved_uio_resid = uiop->uio_resid;
		saved_iov_len = iov->iov_len;

		if (SAM_IS_OBJECT_FILE(ip)) {
			TRACE((rw == UIO_WRITE ? T_SAM_DIOWROBJ_ST :
			    T_SAM_DIORDOBJ_ST), SAM_ITOV(ip), bdp->io_count,
			    iop->blk_off+iop->pboff,
			    (offset_t)iop->obji<<32|contig);
			(void) sam_issue_direct_object_io(ip, rw, bp, iop,
			    contig);
		} else {
			TRACE(rw == UIO_WRITE ? T_SAM_DIOWRBLK_ST :
			    T_SAM_DIORDBLK_ST, SAM_ITOV(ip),
			    blkno >> SAM2SUN_BSHIFT, uiop->uio_loffset, contig);
			if ((bp->b_flags & B_READ) == 0) {
				LQFS_MSG(CE_WARN, "sam_dk_issue_direct_io(): "
				    "bdev_strategy writing mof 0x%x edev %ld "
				    "nb %d\n", bp->b_blkno * 512, bp->b_edev,
				    bp->b_bcount);
			} else {
				LQFS_MSG(CE_WARN, "sam_dk_issue_direct_io(): "
				    "bdev_strategy reading mof 0x%x edev %ld "
				    "nb %d\n", bp->b_blkno * 512, bp->b_edev,
				    bp->b_bcount);
			}
			(void) bdev_strategy(bp);
		}

		/*
		 * For the asynchronous I/O case, once bdev_strategy has been
		 * called for the last segment, we must not access any
		 * dynamically allocated data structures for this I/O (bdp, uio,
		 * iov) as they may have been freed by the completing I/O
		 * thread.
		 */

		if (iop->num_group == 1) {
			break;
		}

		if (contig < (offset_t)(iop->bsize - iop->pboff)) {
			iop->pboff += contig;
			TRACE(T_SAM_GROUPIO1, SAM_ITOV(ip), iop->ord,
			    (sam_tr_t)saved_uio_resid, (uint_t)iop->contig);
		} else if (SAM_IS_OBJECT_FILE(ip)) {
			iop->obji++;
			if (iop->obji >= iop->num_group) {
				iop->obji = 0;
				iop->imap.blk_off0 += iop->bsize;
			}
			iop->ord = ip->olp->ol[iop->obji].ord;
			iop->blk_off = iop->imap.blk_off0;
			iop->pboff = 0;
			TRACE(T_SAM_GROUPIO2, SAM_ITOV(ip), iop->ord,
			    (sam_tr_t)saved_uio_resid, (sam_tr_t)iop->contig);
		} else {
			if (++iop->ord == (iop->imap.ord0 + iop->num_group)) {
				iop->ord = iop->imap.ord0;
				iop->blkno = iop->imap.blkno0 +
				    (sam_daddr_t)(iop->bsize >> SAM_DEV_BSHIFT);
				iop->imap.blkno0 = iop->blkno;
			} else {
				iop->blkno = iop->imap.blkno0;
			}
			iop->pboff = 0;
			TRACE(T_SAM_GROUPIO2, SAM_ITOV(ip), iop->ord,
			    (sam_tr_t)saved_uio_resid, (sam_tr_t)iop->contig);
		}
		if ((saved_uio_resid < (ssize_t)iop->dev_bsize) ||
		    (saved_iov_len < iop->dev_bsize) || (iop->contig <= 0)) {
			break;		/* If done */
		}
	}
	return (saved_iov_len);
}


/*
 * ----- sam_dk_direct_done - process I/O completion.
 *
 * This is the interrupt routine. Called when each async I/O completes.
 */

static void
sam_dk_direct_done(buf_t *bp)
{
	sam_node_t *ip;
	buf_descriptor_t *bdp;
	sam_buf_t  *sbp = (sam_buf_t *)bp;
	offset_t count;
	boolean_t async;

	bdp = (buf_descriptor_t *)(void *)bp->b_vp;
	ip = bdp->ip;
	async = (bdp->aiouiop != NULL);
	bp->b_vp = NULL;
	bp->b_iodone = NULL;
	count = bp->b_bcount;

	if (bp->b_flags & B_REMAPPED) {
		bp_mapout(bp);
	}

	mutex_enter(&bdp->bdp_mutex);
	SAM_BUF_DEQ(&sbp->link);
	if (!async) {
		sema_v(&bdp->io_sema);		/* Increment completed I/Os */
	}
	bdp->io_count--;		/* Decrement number of issued I/O */
	TRACE((bp->b_flags & B_READ ? T_SAM_DIORDBLK_COMP :
	    T_SAM_DIOWRBLK_COMP), SAM_ITOV(ip),
	    (sam_tr_t)(bp->b_lblkno >> SAM2SUN_BSHIFT),
	    (sam_tr_t)bdp->io_count, (sam_tr_t)bdp);
	if ((bp->b_flags & B_ERROR)) {
		bdp->error = EIO;
	}
	mutex_exit(&bdp->bdp_mutex);

	bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS|B_SHADOW);
	sam_free_buf_header((sam_uintptr_t *)bp);

	if (async) {
		sam_dk_aio_direct_done(ip, bdp, count);
	}
}


/*
 * ----- sam_dk_aio_direct_done - process AIO I/O completion.
 *
 * Called when each async AIO I/O completes.
 */

void
sam_dk_aio_direct_done(sam_node_t *ip, buf_descriptor_t *bdp, offset_t count)
{
	/*
	 * Atomically subtract the number of bytes completed by this I/O from
	 * the total size of the I/O operation.  If we just finished the last
	 * portion of the I/O, clean up and notify the AIO subsystem.
	 */
	if (atomic_add_64_nv((uint64_t *)&bdp->remaining_length, -count) == 0) {
		struct buf *bp;
		enum uio_rw rw;
		int iov_count = bdp->iov_count;
		int error;

		bp = bdp->aiouiop->bp;
		rw = (bp->b_flags & B_READ) ? UIO_READ : UIO_WRITE;
		if ((error = sam_dk_fini_direct_io(ip, rw, &bdp->aiouiop->uio,
		    bdp))) {
			bioerror(bp, error);
			TRACE(T_SAM_AIOERR, SAM_ITOV(ip),
			    (SAM_ITOV(ip))->v_rdev,
			    bdp->aiouiop->uio.uio_resid, (sam_tr_t)bdp);
		} else {
			bp->b_resid = bdp->aiouiop->uio.uio_resid;
			TRACE(T_SAM_AIODONE, SAM_ITOV(ip),
			    (SAM_ITOV(ip))->v_rdev,
			    bp->b_resid, (sam_tr_t)bdp);
		}

		/*
		 * Free the kmem parameters only after the main thread is done
		 * and all the I/O has completed. Either can finish first and
		 * can only free after both events are done.
		 */
		kmem_free(bdp->aiouiop, sizeof (struct samaio_uio) +
		    (sizeof (struct iovec) * (iov_count - 1)));
		kmem_free(bdp, sizeof (buf_descriptor_t) +
		    ((iov_count - 1) * sizeof (iov_descriptor_t)));
		biodone(bp);
		return;
	}
}


/*
 * ----- sam_get_buf_header - return buffer pointer.
 *
 * If no buffer pointer in buffer pointer pool, allocate one.
 * These buffers are never to be released to the common OS pool
 * i.e. via brelse().
 *
 * So the following restrictions apply
 *	- if B_ASYNC is set, b_iodone must be set and this
 *	  iodone routine must call sam_free_buf_header() to release it.
 *	- if B_ASYNC is not set, the caller must call sam_free_buf_header()
 *	  to release it.
 */

buf_t *
sam_get_buf_header(void)
{
	sam_uintptr_t **entry = (sam_uintptr_t **)&samgt.buf_freelist;
	sam_uintptr_t *ibp;
	buf_t *bp;

	mutex_enter(&samgt.buf_mutex);
	if ((ibp = *entry) == NULL) {
		ibp = kmem_alloc(sizeof (sam_buf_t), KM_SLEEP);
		bp = (buf_t *)(void *)ibp;
	} else {
		*entry = *(void **) ibp;
		bp = (buf_t *)(void *)ibp;
		/* Destroy semaphores from previous use */
		sema_destroy(&bp->b_sem);
		sema_destroy(&bp->b_io);
	}
	mutex_exit(&samgt.buf_mutex);

	/* Zero buffer header and initialize semaphores */
	bzero((caddr_t)bp, sizeof (sam_buf_t));
	sema_init(&bp->b_sem, 0, NULL, SEMA_DEFAULT, NULL);
	sema_init(&bp->b_io, 0, NULL, SEMA_DEFAULT, NULL);
	return (bp);
}


/*
 * ----- sam_free_buf_header - return buffer pointer to pool.
 *
 * Must be the last operation done on the buffer. It is safe
 * to kmem_free() these buf headers, should we redesign it later
 * to use our own kmem cache to respond better to memory pressure.
 */

void
sam_free_buf_header(sam_uintptr_t *ibp)
{
	sam_uintptr_t **entry = (sam_uintptr_t **)&samgt.buf_freelist;

	mutex_enter(&samgt.buf_mutex);
	*(void **) ibp = *entry;
	*entry = ibp;
	mutex_exit(&samgt.buf_mutex);
}
#endif /* sun */


/*
 * ----- sam_get_sparse_blocks - allocate blocks for sparse file.
 *
 * Get block by setting up a write lease with sparse flag set.
 */

int				/* ERRNO if error, 0 if successful. */
sam_get_sparse_blocks(
	sam_node_t *ip,		/* Pointer to inode */
	enum SPARSE_type flag,
	uio_t *uiop,		/* Pointer to user I/O vector array. */
	int ioflag,		/* Open flags */
	cred_t *credp)		/* credentials pointer */
{
	sam_lease_data_t data;
	offset_t diff;
	int error;

	bzero(&data, sizeof (data));
	RW_UNLOCK_CURRENT_OS(&ip->inode_rwl);
	data.offset = uiop->uio_loffset;
	if (flag == SPARSE_directio) {
		data.resid = uiop->uio_resid;
	} else {
		data.resid = MAX(uiop->uio_resid, ip->mp->mt.fi_maxallocsz);
	}
	diff = ip->size - data.offset;
	if ((diff > 0) && (diff < data.resid)) {
		data.resid = diff;
	}

	data.ltype = LTYPE_write;
	data.sparse = (uchar_t)flag;
	data.filemode = ioflag;
	data.alloc_unit = ip->mp->mt.fi_maxallocsz;

	/*
	 * Allocate at the requested offset, request size + maxallocsz.
	 * Begin allocation at the beginning of a DAU and page align
	 * the buffer because of small allocation (4K) md devices.
	 */
	for (;;) {
		error = sam_get_zerodaus(ip, &data, credp);
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (error == 0) {
			error = sam_process_zerodaus(ip, &data, credp);
			if (error == 0) {
				diff = ((uiop->uio_loffset + uiop->uio_resid) -
				    (data.offset + data.resid));
				if (diff <= 0) {
					break;
				}
				data.offset += data.resid;
				data.resid = diff;
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				continue;
			}
			if (error == ELNRNG) {
				cmn_err(CE_WARN,
			"SAM-QFS: %s: rwio %s ELNRNG error: ino %d.%d, "
			"off %llx len %llx",
				    ip->mp->mt.fi_name,
				    (flag == 2) ? "page" : "direct",
				    ip->di.id.ino, ip->di.id.gen,
				    uiop->uio_loffset,
				    (long long)uiop->uio_resid);
			}
		}
		break;
	}
	return (error);
}


/*
 * ----- sam_get_zerodaus - get the zerodau bit array
 *
 * Ask the server to return a bit map of daus that are allocated.
 */

int				/* ERRNO if error, 0 if successful. */
sam_get_zerodaus(
	sam_node_t *ip,		/* Pointer to inode */
	sam_lease_data_t *dp,	/* Pointer to the data lease struct */
	cred_t *credp)		/* credentials pointer */
{
	offset_t off1, off2;
	offset_t dau_size;
	ushort_t dt;
	int error;

	dp->lflag = 0;
	dp->cmd = 0;
	dp->shflags.b.directio = ip->flags.b.directio;
	dp->shflags.b.abr = ip->flags.b.abr;
	dp->no_daus = 0;
	dp->zerodau[0] = 0;
	dp->zerodau[1] = 0;
	if ((dp->sparse == SPARSE_zeroall) || (dp->sparse == SPARSE_directio)) {
		/*
		 * Round down offset to the closest DAU, MAXBSIZE aligned.
		 * Round up the minimum of filesize, MAX_ZERODAUS, length.
		 */
		off1 = sam_round_to_dau(ip, dp->offset, SAM_ROUND_DOWN);
		off1 = off1 & MAXBMASK;

		dt = ip->di.status.b.meta;
		if (!ip->di.status.b.on_large && (off1 < SM_OFF(ip->mp, dt))) {
			dau_size = SM_BLK(ip->mp, dt);
		} else {
			dau_size = LG_BLK(ip->mp, dt);
		}
#ifdef sun
		off2 = sam_round_to_dau(ip, dp->offset + dp->resid,
		    SAM_ROUND_UP);
		off2 = MIN(off1 + (dau_size * MAX_ZERODAUS), off2);
		off2 = (off2 + MAXBSIZE - 1) & MAXBMASK;
#endif

#ifdef linux
		off2 = sam_round_to_dau(ip, dp->offset + dp->resid,
		    SAM_ROUND_UP);
		off2 = PAGE_ALIGN(off2);
		off2 = MIN(off1 + (dau_size * MAX_ZERODAUS), off2);
#endif

		dp->offset = off1;
		dp->resid = off2 - off1;
	}
	error = sam_proc_get_lease(ip, dp, NULL, NULL, SHARE_wait, credp);
	return (error);
}



/*
 * ----- sam_process_zerodaus - process the zerodau bit array
 *
 * The server returned a bit map of daus that were allocated. The
 * client must clear these blocks.
 */

int				/* ERRNO if error, 0 if successful. */
sam_process_zerodaus(
	sam_node_t *ip,		/* Pointer to inode */
	sam_lease_data_t *dp,	/* Pointer to the data lease struct */
	cred_t *credp)		/* credentials pointer */
{
	offset_t boff;
	offset_t boff_last = 0;
	ushort_t bt, dt;
	ushort_t bt_last = 0;
	offset_t dau_size;
	offset_t dau_size_last = 0;
	int i, j, ishift;
	int error;
	int did_first = 0;
	int num_group;

	num_group = ip->mp->mi.m_fs[ip->di.unit].num_group;

	boff = dp->offset;
	error = sam_map_block(ip, boff, dp->resid, SAM_ALLOC_BLOCK,
	    NULL, credp);
	if ((error == 0) && ((dp->sparse == SPARSE_zeroall) ||
	    (dp->sparse == SPARSE_directio))) {
		TRACE(T_SAM_ZERODAU, SAM_ITOP(ip), dp->offset,
		    dp->resid, dp->no_daus);

		/*
		 * Zero the DAUs between boff and boff + dp->resid which were
		 * allocated.
		 */
		dt = ip->di.status.b.meta;
		boff = dp->offset;

		/*
		 * If this is a stripe group each bit in zerodau[]
		 * represents num_group * dau_size.
		 */
		if (num_group > 1) {
			offset_t daddr;
			offset_t sg_dau;

			/*
			 * Need to round boff back to the beginning of the
			 * stripe group so we don't miss dau(s) in the
			 * stripe group before boff.
			 */
			sg_dau = LG_BLK(ip->mp, dt) * num_group;

			daddr = roundup(boff, sg_dau);
			if (daddr > boff) {
				boff = daddr - sg_dau;
			}
		}
		for (i = 0, j = 0, ishift = 0; i < dp->no_daus; i++) {
			if (i == 64) {
				ishift = 0;
				j++;
			}
			if (!ip->di.status.b.on_large &&
			    (boff < SM_OFF(ip->mp, dt))) {
				bt = SM;
				dau_size = SM_BLK(ip->mp, dt);
			} else {
				bt = LG;
				dau_size = LG_BLK(ip->mp, dt) * num_group;
			}
			if (dp->zerodau[j] & ((uint64_t)1 << ishift)) {
				/*
				 * Zero the DAUS in the list.
				 * If dp->sparse == SPARSE_zeroall,
				 *   zero them all.
				 * If dp->sparse == SPARSE_directio,
				 *   zero first and last.
				 */
				if (!did_first ||
				    (dp->sparse == SPARSE_zeroall)) {

					did_first = 1;
					sam_zero_dau(ip, boff, dau_size,
					    SAM_FORCEFAULT, dt, bt);

				} else {
					/*
					 * Save these for later since this may
					 * be the last DAU in the list.
					 */
					dau_size_last = dau_size;
					boff_last = boff;
					bt_last = bt;
				}
			}
			boff += dau_size;
			ishift++;
		}
		if (dau_size_last > 0) {
			/*
			 * Zero the last DAU in the list.
			 */
			sam_zero_dau(ip, boff_last, dau_size_last,
			    SAM_FORCEFAULT,
			    dt, bt_last);
		}
	}
	return (error);
}


#ifdef sun
/*
 * ----- sam_lio_move -
 * uiomove replacement for sam_lio interface.
 */
static int			/* errno or 0 */
sam_lio_move(
	void	*p,		/* buffer address */
	size_t	n,		/* number of bytes */
	enum uio_rw rw,		/* UIO_READ or UIO_WRITE */
	struct uio *uio)	/* I/O parameters */
{
	ulong_t cnt;
	int error;
	struct sam_listio_call *callp;
	struct samaio_uio *auiop;


	auiop = (struct samaio_uio *)uio;
	callp = (struct sam_listio_call *)auiop->bp;
	while (n && uio->uio_resid) {
		cnt = MIN(n, callp->listio.mem_count[0]);
		if (cnt == 0) {
			callp->listio.mem_count++;
			callp->listio.mem_addr++;
			if (--callp->listio.mem_list_count == 0) {
				return (EINVAL);
			}
			continue;
		}
		if (rw == UIO_READ) {
			error = copyout(p, callp->listio.mem_addr[0], cnt);
		} else {
			error = copyin(callp->listio.mem_addr[0], p, cnt);
		}
		if (error) {
			return (EFAULT);
		}
		callp->listio.mem_addr[0] =
		    (caddr_t)callp->listio.mem_addr[0] + cnt;
		callp->listio.mem_count[0] -= cnt;
		uio->uio_resid -= cnt;
		uio->uio_loffset += cnt;
		p = (caddr_t)p + cnt;
		n -= cnt;
	}
	return (0);
}
#endif /* sun */


#ifdef linux
/*
 * ----- samqfs_handle_mmap_offline -
 * Called from samqfs_get_block to handle offline files during mmap read/write.
 */
static int
samqfs_handle_mmap_offline(sam_node_t *ip, offset_t offset, int flag)
{
	int			error = 0;
	cred_t		*credp;
	sam_cred_t	sam_cred;

	sam_set_cred(NULL, &sam_cred);
	credp = (cred_t *)&sam_cred;

	if (flag) {
		/*
		 * Writing -
		 *
		 * If offline or stage pages exist for the inode, clear inode
		 * file/segment for writing by making file/segment online.
		 */
		if (ip->di.status.b.offline || ip->flags.b.stage_pages) {
			error = sam_clear_ino(ip, ip->di.rm.size,
			    MAKE_ONLINE, credp);
		}
	} else {
		/*
		 * Reading -
		 * If offline, return when enough of the file is online for the
		 * request.
		 */
		if (ip->di.status.b.offline) {
			uio_t	uio;
			int		stage_n_set = 0;

			uio.uio_loffset = offset;
			uio.uio_resid = SAM_DEV_BSIZE;
			error = sam_read_proc_offline(ip, ip, &uio, credp,
			    &stage_n_set);
		}
	}
	return (error);
}

/*
 * ----- samqfs_get_block -
 * Called by Linux to map a logical SAM_DEV_BSIZE block
 * at a SAM_DEV_BSIZE block offset to a real device and block number.
 */
static int
samqfs_get_block(struct inode *li, long iblk, struct buffer_head *bh,
    int flag, krw_t rw_type)
{
	sam_node_t *ip;
	sam_ioblk_t ioblk;
	offset_t offset;
	sam_daddr_t blkno;
	sam_map_t type;
	int error;

	ip = SAM_LITOSI(li);

	/*
	 * sam_map_block() wants the offset in bytes.
	 */
	offset = (offset_t)iblk * SAM_DEV_BSIZE_LINUX;

	if (flag && !RW_WRITE_HELD_OS(&ip->inode_rwl)) {
		panic("samqfs_get_block: no write lock!");
	}

	if (ip->di.status.b.offline || ip->flags.b.stage_pages) {
		error = samqfs_handle_mmap_offline(ip, offset, flag);
		if (error) {
			return (-error);
		}
	}

	type = (flag == 0) ? (SAM_READ_PUT | SAM_MAP_NOWAIT) : SAM_WRITE;

	while ((error = sam_map_block(ip, offset, SAM_DEV_BSIZE_LINUX,
	    type, &ioblk, CRED())) != 0 && IS_SAM_ENOSPC(error)) {
		RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
		error = sam_wait_space(ip, error);
		RW_LOCK_OS(&ip->inode_rwl, rw_type);
		if (error) {
			break;
		}
	}

	if (error == 0) {
		if (ioblk.blkno == 0) {
			/*
			 * This is an unallocated DAU.
			 * Return without setting BH_mapped
			 * causes Linux to provide a zeroed page.
			 */
			return (0);
		}

		/*
		 * ioblk.blkno is the block number
		 *		of the start of a DAU.
		 *
		 * ioblk.pboff is the offset in bytes from the
		 *		start of the DAU to the wanted offset.
		 *
		 * ioblk.count is number of available bytes.
		 *
		 * ioblk.ord is the device ordinal.
		 */

		blkno = (ioblk.blkno >> SAM_DEV_BLOCKS_BSHIFT_TO_LINUX) +
		    (ioblk.pboff >> SAM_DEV_BSHIFT_LINUX);

		bh->b_blocknr = blkno;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
		bh->b_bdev = ip->mp->mi.m_fs[ioblk.ord].bdev;
#else
		bh->b_dev = ip->mp->mi.m_fs[ioblk.ord].dev;
#endif
		set_bit(BH_Mapped, &bh->b_state);

		if (ioblk.imap.flags & M_SPARSE) {
			return (0);
		}
	}

	return (-error);
}


int
samqfs_get_block_reader(
	struct inode *li,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	sector_t iblk,
#else
	long iblk,
#endif
	struct buffer_head *bh,
	int flag)
{
	return (samqfs_get_block(li, iblk, bh, flag, RW_READER));
}


int
samqfs_get_block_writer(
	struct inode *li,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	sector_t iblk,
#else
	long iblk,
#endif
	struct buffer_head *bh,
	int flag)
{
	return (samqfs_get_block(li, iblk, bh, flag, RW_WRITER));
}


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
int
samqfs_get_blocks_reader(struct inode *li, sector_t iblk,
    unsigned long maxblocks, struct buffer_head *bh, int flag)
{
	int rv;

	rv = samqfs_get_block_reader(li, iblk, bh, flag);

	if (rv == 0) {
		bh->b_size = (1 << li->i_blkbits);
	}

	return (rv);
}


int
samqfs_get_blocks_writer(struct inode *li, sector_t iblk,
    unsigned long maxblocks, struct buffer_head *bh, int flag)
{
	int rv;

	rv = samqfs_get_block_writer(li, iblk, bh, flag);

	if (rv == 0) {
		bh->b_size = (1 << li->i_blkbits);
	}

	return (rv);
}
#endif

/*
 * ----- sam_flush_pages -
 * Flush any dirty inode data buffers.
 */
int
sam_flush_pages(
	sam_node_t *ip,		/* Pointer to inode */
	int flags)		/* flags - B_INVAL, B_FREE, B_ASYNC, or 0, */
{
	struct inode *li;
	int error = 0;

	if (ip->flags.b.staging) {
		if (ip->stage_size == 0) {
			return (0);
		}
	}

	li = SAM_SITOLI(ip);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))

	error = sync_mapping_buffers(li->i_mapping);

	if (flags == B_INVAL) {
		invalidate_inode_pages(li->i_mapping);
	}

#else
	if (!list_empty(&li->i_dirty_data_buffers)) {

		error = fsync_buffers_list(&li->i_dirty_data_buffers);

		if (flags == B_INVAL) {
			invalidate_inode_pages(li);
		}

		if (error != 0) {
			TRACE(T_SAM_FLUSHERR3, li, ip->di.id.ino,
			    atomic_read(&li->i_count), ip->flags.bits);
		}
	}
#endif

	if (flags != B_ASYNC) {
		ip->flags.b.stage_pages = 0;
	}

	return (error);
}


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
/*
 * Linux 2.6 direct I/O will happily allow an I/O to straddle a device
 * boundary, stuffing a bio with blocks from both pages.
 * Chop up the I/O by device before sending it down.
 *
 * We also chop up the I/O when it covers multiple disk blocks and those
 * blocks are not contiguous on disk.
 */

static ssize_t
samqfs_directio_file_IO(struct file *file, char __user *buf, size_t count,
    loff_t *ppos, int rw)
{
	ssize_t ret = 0;
	struct iovec local_iov;
	struct inode *li = file->f_dentry->d_inode;
	unsigned long blocksize = 1 << li->i_blkbits;
	struct block_device *dev = NULL;
	offset_t start, end;
	size_t this_len;
	char __user *bufp;
	struct buffer_head map_bh;
	long block_in_file;
	sam_daddr_t expect_blocknr = 0;

	start = *ppos;
	end = *ppos + count;
	bufp = (char __user *)buf;
	this_len = 0;

	while (start + this_len < end) {

		block_in_file = (start + this_len) >> li->i_blkbits;
		ret = samqfs_get_block(li, block_in_file, &map_bh, 1,
		    RW_WRITER);

		if ((rw == 0) && (ret == -ELNRNG)) {
			/*
			 * Direct I/O read from hole.  map_bh is unmapped.
			 */
			ret = 0;
		}

		if (ret < 0) {
			break;
		}

		if (dev == NULL) {
			dev = map_bh.b_bdev;
		}
		if (expect_blocknr == 0) {
			expect_blocknr = map_bh.b_blocknr;
		}
		if ((dev == map_bh.b_bdev) &&
		    (expect_blocknr == map_bh.b_blocknr)) {
			this_len = MIN(this_len + blocksize, count);
		}

		if ((start + this_len == end) || (dev != map_bh.b_bdev) ||
		    (map_bh.b_blocknr != expect_blocknr)) {

			if (rw) {
				local_iov.iov_base = (void __user *)bufp;
				local_iov.iov_len = this_len;
				ret = generic_file_write_nolock(file,
				    &local_iov,
				    1, ppos);
			} else {
				ret = generic_file_read(file, bufp,
				    this_len, ppos);
			}

			break;
		}

		expect_blocknr++;
	}

	return (ret);
}


ssize_t
samqfs_generic_file_write(struct file *file, const char __user *buf,
    size_t count, loff_t *ppos)
{
	ssize_t ret;

	if (file->f_flags & O_DIRECT) {
		ret = samqfs_directio_file_IO(file, (char __user *)buf,
		    count, ppos, 1);
	} else {
		ret = rfs_generic_file_write(file, buf, count, ppos);
	}
	return (ret);
}


ssize_t
samqfs_generic_file_read(struct file *file, char __user *buf, size_t count,
    loff_t *ppos)
{
	ssize_t ret;

	if (file->f_flags & O_DIRECT) {
		ret = samqfs_directio_file_IO(file, buf, count, ppos, 0);
	} else {
		ret = generic_file_read(file, buf, count, ppos);
	}
	return (ret);
}
#endif /* >= 2.6 */

#endif /* linux */
