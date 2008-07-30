/*
 * ----- page.c - Process the getpage/putpage functions.
 *
 * Processes the SAM-QFS inode getpage and inode putpage functions.
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

#pragma ident "$Revision: 1.100 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/sysmacros.h>
#include <sys/vnode.h>
#include <sys/fs_subr.h>
#include <sys/proc.h>

#include <vm/hat.h>
#include <vm/page.h>
#include <vm/pvn.h>
#include <vm/as.h>
#include <vm/seg.h>

/* ----- SAMFS Includes */

#include "sam/param.h"
#include "sam/types.h"

#include "inode.h"
#include "mount.h"
#include "extern.h"
#include "debug.h"
#include "trace.h"
#include "kstats.h"
#include "qfs_log.h"
#include "rwio.h"


static int sam_getapage(vnode_t *vp, sam_u_offset_t offset, uint_t length,
	page_t ** pglist, uint_t plsize, struct seg *segp,
	caddr_t addr, enum seg_rw rw, int seq);
static buf_t *sam_pageio(sam_node_t *ip, sam_ioblk_t *iop, page_t *pp,
	offset_t offset, uint_t pg_off, uint_t vn_len, int flags);
static int sam_map(sam_node_t *ip, offset_t offset, struct sam_iohdr *iop);


/*
 * ----- sam_getpages - Get pages for a file.
 *
 * Get pages for a file for use in a memory mapped function.
 *  Return the pages in the pagelist up to page list size.
 */

int				/* ERRNO if error, 0 if successful. */
sam_getpages(
	vnode_t *vp,		/* vnode entry */
	sam_u_offset_t offset,	/* file offset */
	uint_t length,		/* file length provided to mmap. */
	page_t **pglist,	/* page list array (returned) */
	uint_t plsize,		/* max size of pages in page list array. */
	struct seg *segp,	/* page segment. */
	caddr_t addr,		/* page address. */
	enum seg_rw rw,		/* access type for the fault operation. */
	int sparse,
	boolean_t lock_set)
{
	int error;
	sam_node_t *ip = SAM_VTOI(vp);
	page_t *pp;
	page_t **pppl;
	se_t se;
	int seq;
	sam_u_offset_t off;
	sam_u_offset_t size;

	pppl = pglist;
	size = offset + length;
	off = offset;
	se = (rw == S_CREATE || rw == S_OTHER) ? SE_EXCL : SE_SHARED;
	seq = (ip->page_off == offset) && (rw != S_CREATE);

	/*
	 * Readahead is requested if pglist == NULL. Don't readahead
	 * if file is offline.
	 */
	if (pglist == NULL) {
		while (off < size) {
			if (ip->di.status.b.offline) {
				return (0);
			}
			ip->ra_off = off;
			sam_read_ahead(vp, off, off, segp, addr,
			    ip->mp->mt.fi_readahead);
			off += PAGESIZE;
			addr += PAGESIZE;
		}
		ip->page_off = off;
		return (0);
	}

	/*
	 * Search the range (offset, offset+length) looking up pages.
	 * If the page exists, lock it. If it does not exist, get the
	 * page and lock it.
	 */
	while (off < size) {

		if (seq && ((off + ip->mp->mt.fi_readahead) >= ip->ra_off) &&
		    (off <= ip->ra_off) && (ip->ra_off < ip->size) &&
		    !ip->di.status.b.offline && page_exists(vp, off)) {
			sam_read_ahead(vp, off, ip->ra_off, segp, addr,
			    ip->mp->mt.fi_readahead);
		}

		/*
		 * Lookup with nowait if this thread acquired the inode rw_lock
		 * after page faulting (lock_set) since there is a potential
		 * deadlock when more than one thread is trying to access the
		 * same page. If the page is free, page_lookup_nowait will not
		 * get the page; use page_lookup to get the page. If the page is
		 * not free, unlock the range, release the inode rw_lock, delay,
		 * and reissue. XXX - this should be analyzed for a better fix
		 * that does not require a delay.
		 */
		if (!lock_set) {
			pp = page_lookup(vp, off, se);
		} else {
			pp = page_lookup_nowait(vp, off, se);
			if ((pp == NULL) &&
			    ((pp = page_exists(vp, off)) != NULL)) {
				if ((pp != NULL) && PP_ISFREE(pp)) {
					pp = page_lookup(vp, off, se);
				} else {
					/* release locked pages */
					while (pppl > pglist) {
						/* unlock pages */
						page_unlock(*--pppl);
						/* clear page array */
						*pppl = NULL;
					}
					return (EDEADLK);
				}
			}
		}

		if (pp) {	/* Lookup page in cache */
			*pppl = pp;
			off += PAGESIZE;
			addr += PAGESIZE;
			plsize -= PAGESIZE;
			length -= PAGESIZE;
			pppl++;

		} else {
			/* If page does not exist, get the page. */
			if ((error = sam_getapage(vp, off, length, pppl, plsize,
			    segp, addr, rw, seq))) {
				if (off > offset) {
					/* if error & locked pages */
					while (pppl > pglist) {
						/* unlock pages */
						page_unlock(*--pppl);
						/* clear page array */
						*pppl = NULL;
					}
				}
				return (error);
			}
			while (*pppl) {
				off += PAGESIZE;
				addr += PAGESIZE;
				plsize -= PAGESIZE;
				length -= PAGESIZE;
				pppl++;
			}
		}
	}

	/*
	 * If not writing to a sparse file, return pages to the page list
	 * up to plsize if they are in memory.
	 */
	if (pglist && !(sparse && (rw == S_WRITE || rw == S_CREATE))) {
		size = off + plsize;
		while (off < size) {
			if ((pp = page_lookup_nowait(vp, off, SE_SHARED)) ==
			    NULL) {
				break;
			}
			*pppl = pp;
			off += PAGESIZE;
			pppl++;
		}
	}
	if (pglist) {
		*pppl = NULL;		/* Terminate the page list */
	}
	ip->page_off = off;
	return (0);
}


/*
 * ----- sam_getapage - Read or create a single page for a file.
 *
 * Get a single page for a file for use in a memory mapped
 * function. Page does not exist in the paging cache.
 * Either create the page, or read it from disk.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_getapage(
	vnode_t *vp,		/* vnode entry. */
	sam_u_offset_t offset,	/* file offset */
	uint_t length,		/* file length provided to mmap. */
	page_t **pglist,	/* page list array (returned) */
	uint_t plsize,		/* max byte size in page list array */
	struct seg *segp,	/* Pointer to page segment. */
	caddr_t addr,		/* Page address. */
	enum seg_rw rw,		/* Access type for the fault operation. */
	int seq)		/* Sequential access */
{
	buf_t *bp[SAM_MAX_BP];
	sam_iohdr_t iohdr;
	sam_iohdr_t *iop;		/* Pointer to I/O header */
	int error = 0;
	int extra;
	sam_node_t *ip;
	sam_size_t vn_len;
	sam_ssize_t vn_tot;
	sam_u_offset_t vn_off = 0;
	page_t *pp;
	int i;
	sam_u_offset_t off;

	TRACE(T_SAM_GETAPAGE, vp, (sam_tr_t)offset, length, (sam_tr_t)rw);
	ip = SAM_VTOI(vp);
	iop = &iohdr;
	if ((rw != S_CREATE) && (error = sam_map(ip, (offset_t)offset, iop))) {
		return (error);
	}

	if ((rw == S_CREATE) ||
	    (iop->ioblk[0].imap.flags & (M_SPARSE|M_OBJ_EOF))) {
		/* Create the page */
		if ((pp = page_create_va(vp, offset, PAGESIZE, PG_WAIT, segp,
		    addr)) == NULL) {
			cmn_err(SAMFS_DEBUG_PANIC,
			    "SAM-QFS: %s: sam_getapage: cannot create page,"
			    " ip=%p, ino=%d.%d, offset=%llx",
			    ip->mp->mt.fi_name, (void *)ip,
			    ip->di.id.ino, ip->di.id.gen,
			    (offset_t)offset);
			return (ERESTART);		/* from ufs_panic.c */
		}
		if (rw != S_CREATE) {
			pagezero(pp, 0, PAGESIZE);
		}
		vn_len = PAGESIZE;

	} else {			/* Read in the page */
		sam_size_t contig = iop->contig;

		if (iop->ioblk[0].pboff) {
			iop->ioblk[0].blkno += (iop->ioblk[0].pboff >>
			    SAM_DEV_BSHIFT);
			contig -= iop->ioblk[0].pboff;
		}
		if ((seq == 0) || (offset == 0)) {
			contig = MIN(contig, PAGESIZE);
		}
		if (iop->ioblk[0].num_group > 1) {
			contig = MIN(contig,
			    (iop->ioblk[0].bsize-iop->ioblk[0].pboff));
		}
		if (ip->flags.b.staging) {
			/*
			 * If staging, only the data <= stage_size is valid.
			 * Make sure contig is rounded down to a page.
			 */
			if (ip->stage_size > offset) {
				contig = MIN(contig, ip->stage_size - offset);
				contig = contig & PAGEMASK;
				/*
				 * Must have at least one page.
				 */
				ASSERT(contig != 0);
			}
		}
		if ((pp = pvn_read_kluster(vp, offset, segp, addr, &vn_off,
		    &vn_len, offset, contig, 0)) == NULL) {
			/* Page in cache since last page_exists check */
			*pglist = NULL;
			return (0);
		}
		if ((extra = (vn_len & PAGEOFFSET)) != 0) {
			pagezero(pp->p_prev, extra, (PAGESIZE - extra));
		}
		TRACE(T_SAM_GETAPAGE1, vp, vn_off, vn_len, (sam_tr_t)pp);

		ip->ra_off = offset + ((vn_len + PAGESIZE - 1) & PAGEMASK);
		off = offset;
		vn_tot = (sam_ssize_t)vn_len;
		for (i = 0; i < (int)iop->nioblk; i++) {
			uint_t pg_off = (uint_t)(off & (PAGESIZE - 1));
			uint_t len;

			if (!(iop->ioblk[i].imap.flags & M_SPARSE) &&
			    (off < ip->size)) {
				len = MIN(iop->ioblk[i].contig, vn_tot);
				if (iop->ioblk[i].imap.flags & M_OBJECT) {
					bp[i] = sam_pageio_object(ip,
					    &iop->ioblk[i], pp, off, pg_off,
					    len, B_READ);
					if (bp[i] == NULL) {
						if (!(iop->ioblk[i].imap.flags &
						    M_OBJ_EOF)) {
							error = EIO;
						}
					}
				} else {
					bp[i] = sam_pageio(ip,
					    &iop->ioblk[i], pp, off, pg_off,
					    len, B_READ);
					if (bp[i] == NULL) {
						error = EIO;
					}
				}

			} else {
				/*
				 * Unallocated blocks or blocks beyond the end
				 * of the file
				 */
				len = MIN(iop->ioblk[i].bsize, vn_tot);

				/*
				 * If the DAU size is more than the page size,
				 * we split the block into number of pages and
				 * will zero in the pages from that offfset
				 * till the end of DAU.
				 */
				if (len > 0 && pg_off >= 0 &&
				    (pg_off + len <= PAGESIZE)) {
					pagezero(pp, pg_off, len);
				} else {
					int no_pages = len / PAGESIZE;
					int count;
					page_t *savep = pp->p_next;

					pagezero(pp, pg_off,
					    (PAGESIZE - pg_off));

					for (count = 1; count < no_pages;
					    count++) {
						pagezero(savep, 0, PAGESIZE);
						savep = pp->p_next;
					}
				}
				bp[i] = NULL;

			}
			off += len;
			vn_tot -= len;
			if (vn_tot <= 0) {
				break;
			}
		}

		/*
		 * While sync I/O outstanding, readahead if sequential access
		 * and not offline.
		 */
		if (seq && !ip->di.status.b.offline &&
		    (ip->ra_off < ip->size) && (error == 0)) {
			sam_read_ahead(vp, offset, ip->ra_off, segp, addr,
			    ip->mp->mt.fi_readahead);
		}
		for (i = 0; i < (int)iop->nioblk; i++) {
			if (bp[i] != NULL) {
				if (iop->ioblk[i].imap.flags & M_OBJECT) {
					error = sam_pg_object_sync_done(ip,
					    bp[i], "GETPAGE");
				} else {
					int err;

					err = biowait(bp[i]);
					pageio_done(bp[i]);
					if (error == 0) {
						error = err;
					}
				}
			}
		}
		if (error) {
			pvn_read_done(pp, B_ERROR);
			return (error);
		}
	}
	/*
	 * Enter locked pages into the pagelist array.
	 */
	pvn_plist_init(pp, pglist, plsize, offset, vn_len, rw);
	TRACE(T_SAM_GETAPAGE2, vp, (sam_tr_t)* pglist, plsize, (sam_tr_t)pp);
	return (0);
}


/*
 * ----- sam_read_ahead - Initiate a read ahead.
 *
 * Read ahead and do not wait for the I/O.
 */

void
sam_read_ahead(
	vnode_t *vp,			/* vnode entry. */
	sam_u_offset_t offset,		/* file offset */
	sam_u_offset_t vn_off,		/* readahead offset */
	struct seg *segp,		/* Pointer to page segment. */
	caddr_t addr,			/* Page address. */
	long long read_ahead)		/* Read ahead size */
{
	sam_ssize_t contig;
	int extra;
	sam_ioblk_t ioblk;
	sam_ioblk_t *iop;		/* Pointer to I/O entry */
	sam_node_t *ip = SAM_VTOI(vp);
	page_t *pp;
	caddr_t	ra_addr;
	sam_u_offset_t size, length;
	sam_size_t vn_len, vn_tot;
	buf_t *ra_buf;

	ra_addr = addr + (vn_off - offset);
	if (ra_addr >= (segp->s_base + segp->s_size)) {
		return;
	}

	iop = &ioblk;
	length = read_ahead;
	size = (ip->di.rm.size + PAGESIZE - 1) & PAGEMASK;
	if ((vn_off + read_ahead) < size) {
		size = vn_off + read_ahead;
	} else {
		length = size - vn_off;
	}
	while (vn_off < size) {
		if (!S_ISREQ(ip->di.mode)) {
			if (sam_map_block(ip, (offset_t)vn_off,
			    (offset_t)length,
			    (SAM_READ_PUT | SAM_MAP_NOWAIT), iop, CRED())) {
				return;
			}
			if (iop->imap.flags & M_OBJ_EOF) {
				return;
			}
		} else {
			if (ip->rdev == 0) {
				return;
			}
			if (sam_rmmap_block(ip, (offset_t)vn_off,
			    (offset_t)length,
			    SAM_READ_PUT, iop)) {
				return;
			}
		}
		contig = iop->contig;
		if (contig & (PAGESIZE - 1)) {
			contig &= ~(PAGESIZE - 1);
		}
		if (iop->pboff) {
			iop->blkno += (iop->pboff >> SAM_DEV_BSHIFT);
			contig -= iop->pboff;
		}
		if ((iop->count == 0) || (contig <= 0) ||
		    (iop->imap.flags & M_SPARSE)) {
			return;
		}
		vn_tot = contig;
		while ((vn_tot > 0) && (vn_off < size)) {
			if (iop->num_group > 1) {
				contig = MIN(contig, (iop->bsize - iop->pboff));
			}
			if ((pp = pvn_read_kluster(vp, vn_off, segp,
			    ra_addr, &vn_off,
			    &vn_len, vn_off, contig, 1)) == NULL) {
				return;
			}
			if ((extra = (vn_len &  PAGEOFFSET)) > 0) {
				pagezero(pp->p_prev, extra, PAGESIZE - extra);
			}
			ip->ra_off = (vn_off + vn_len + PAGESIZE - 1) &
			    PAGEMASK;
			TRACE(T_SAM_READAHEAD, vp, contig, (sam_tr_t)vn_off,
			    (sam_tr_t)ip->ra_off);

			if (iop->imap.flags & M_OBJECT) {
				ra_buf = sam_pageio_object(ip, iop, pp, vn_off,
				    0, vn_len, (B_READ | B_ASYNC));
			} else {
				ra_buf = sam_pageio(ip, iop, pp, vn_off,
				    0, vn_len, (B_READ | B_ASYNC));
			}
			if (ra_buf == NULL) {
				pvn_read_done(pp, B_ERROR);
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_read_ahead, "
				    "ip=%p, ino=%d.%d,"
				    " pp=%p, vn_off=%llx, len=%llx",
				    ip->mp->mt.fi_name, (void *)ip,
				    ip->di.id.ino,
				    ip->di.id.gen, (void *)pp,
				    (long long)vn_off,
				    (long long)vn_len);
				return;
			}

			vn_off += vn_len;
			ra_addr += vn_len;
			length -= vn_len;
			if (iop->num_group == 1) {
				break;
			}
			vn_tot -= vn_len;
			contig = vn_tot;
			if (vn_len < (iop->bsize - iop->pboff)) {
				iop->blkno += (vn_len >> SAM_DEV_BSHIFT);
				iop->pboff += vn_len;
				if (iop->pboff >= iop->bsize) {
					iop->pboff = 0;
				}
			} else if (SAM_IS_OBJECT_FILE(ip)) {
				iop->obji++;
				if (iop->obji >= iop->num_group) {
					iop->obji = 0;
					iop->imap.blk_off0 += iop->bsize;
				}
				iop->ord = ip->olp->ol[iop->obji].ord;
				iop->blk_off = iop->imap.blk_off0;
				iop->pboff = 0;

			} else {
				if (++iop->ord == (iop->imap.ord0 +
				    iop->num_group)) {
					iop->ord = iop->imap.ord0;
					iop->blkno = iop->imap.blkno0 +
					    (iop->bsize >> SAM_DEV_BSHIFT);
					iop->imap.blkno0 = iop->blkno;
				} else {
					iop->blkno = iop->imap.blkno0;
				}
				iop->pboff = 0;
			}
		}
	}
}


/*
 * ----- sam_putapage - Put a single page from a file.
 *
 * Write out contiguous dirty pages from a file which are used
 * for memory mapped i/o. pp->p_offset specifies the start of the
 * dirty page.
 */

/* ARGSUSED5 */
int				/* ERRNO if error, 0 if successful. */
sam_putapage(
	vnode_t *vp,		/* Pointer to vnode. */
	page_t *pp,		/* page entry. */
	sam_u_offset_t *offset,	/* Pointer to file offset (returned). */
	sam_size_t *length,	/* Pointer to file length (returned). */
	int flags,		/* Flags -- B_INVAL, B_DIRTY, B_FREE, */
				/*   B_DONTNEED, B_FORCE, B_ASYNC. */
	cred_t *credp)		/* Credentials pointer */
{
	buf_t *bp[SAM_MAX_BP];
	sam_iohdr_t ioblk;
	sam_iohdr_t *iop;
	int error = 0;
	sam_node_t *ip;
	sam_size_t vn_len;
	sam_u_offset_t vn_off, lb_off;
	sam_ssize_t vn_tot;
	offset_t off;
	int i;

	ip = SAM_VTOI(vp);

	/*
	 * Set modified time for mapped files which are modified via
	 * stores into the mapped space. Do not change modified time
	 * if staging or stage pages exist.
	 */
	if (!(ip->flags.bits & SAM_INHIBIT_UPD) && !S_ISSEGI(&ip->di) &&
	    (ip->wmm_pages != 0)) {
		boolean_t lock_set = B_FALSE;

		if (RW_OWNER_OS(&ip->inode_rwl) != curthread) {
			/* Enforce the sam_put/getpage_vn locking rule */
			lock_set = B_TRUE;
			mutex_enter(&ip->fl_mutex);
		}
		TRANS_INODE(ip->mp, ip);
		sam_mark_ino(ip, SAM_UPDATED);
		if (lock_set) {
			mutex_exit(&ip->fl_mutex);
		}
	}

	/*
	 * Find offset in vnode for disk block in this page.
	 */
	off = pp->p_offset;
	TRACE(T_SAM_PUTAPAGE1, vp, (sam_tr_t)off, (sam_tr_t)ip->di.rm.size,
	    (sam_tr_t)pp);

	/*
	 * Shared client directory pages are not dirty. The page mod bit
	 * is not set, but VM is still calling putpage.
	 */
	if (SAM_IS_SHARED_CLIENT(ip->mp) && S_ISDIR(ip->di.mode)) {
		SAM_DEBUG_COUNT64(client_dir_putpage);
		if (pp) {
			pvn_write_done(pp, (B_WRITE | flags));
		}
		return (0);
	}

	/*
	 * If the vnode points to a forcibly unmounted FS,
	 * any pages must be dropped on the floor.
	 */
	if (SAM_VP_IS_STALE(vp)) {
		TRACE(T_SAM_STALE_PP, vp, ip->di.id.ino, flags, 0);
		if (pp) {
			/*
			 * We should invalidate the pages as filesystem is
			 * no longer mounted.
			 */
			pvn_write_done(pp, (B_INVAL | flags));
		}
		return (0);
	}

	iop = &ioblk;
	if ((error = sam_map(ip, (offset_t)off, iop))) {
		if (pp) {
			pvn_write_done(pp, (B_ERROR | B_WRITE | flags));
		}
		return (error);
	}

	/*
	 * Putpage must have a disk block mapped for each page.
	 */
	if (iop->ioblk[0].imap.flags & M_SPARSE) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: sam_putapage: sparse block,"
		    " ip=%p, ino=%d.%d, pp=%p, off=%llx",
		    ip->mp->mt.fi_name, (void *)ip, ip->di.id.ino,
		    ip->di.id.gen,
		    (void *)pp, off);
		SAMFS_PANIC_INO(ip->mp->mt.fi_name,
		    "SAM-QFS: sam_putapage: sparse block", ip->di.id.ino);
		if (pp) {
			pvn_write_done(pp, (B_ERROR | B_WRITE | flags));
		}
		return (EIO);
	}

	/*
	 * Find a set of pages that fit in the kluster of blocks.
	 */
	if (SAM_IS_OBJECT_FILE(ip)) {
		lb_off = off;
	} else {
		lb_off = iop->ioblk[0].blk_off;
	}
	if (iop->ioblk[0].num_group > 1) {
		iop->contig = MIN(iop->contig, iop->ioblk[0].bsize);
		iop->ioblk[0].contig = iop->contig;
	}
	pp = pvn_write_kluster(vp, pp, &vn_off, &vn_len, lb_off,
	    iop->contig, flags);
	if (vn_off > lb_off) {
		iop->ioblk[0].blkno += ((vn_off - lb_off) >> SAM_DEV_BSHIFT);
		iop->contig -= (vn_off - lb_off);
	}
	if (vn_len > iop->contig) {
		vn_len = iop->contig;
	}
	if (vn_len == 0) {
		vn_len = PAGESIZE;
	}

	TRACE(T_SAM_PUTAPAGE3, vp, vn_off, vn_len, (sam_tr_t)pp);
	vn_tot = vn_len;
	if (flags & B_ASYNC) {
		mutex_enter(&ip->write_mutex);
		pp->p_fsdata = (int)iop->nioblk;
		mutex_exit(&ip->write_mutex);
	}
	for (i = 0; i < (int)iop->nioblk; i++) {
		uint_t pg_off = off & (PAGESIZE - 1);
		uint_t len = MIN(iop->ioblk[i].contig, vn_tot);

		if (!(iop->ioblk[i].imap.flags & M_SPARSE)) {
			if (iop->ioblk[i].imap.flags & M_OBJECT) {
				bp[i] = sam_pageio_object(ip, &iop->ioblk[i],
				    pp, off, pg_off, len, B_WRITE|flags);
				if (bp[i] == NULL) {
					if (!(iop->ioblk[i].imap.flags &
					    M_OBJ_EOF)) {
						error = EIO;
					}
				}
			} else {
				bp[i] = sam_pageio(ip, &iop->ioblk[i],
				    pp, off, pg_off, len, B_WRITE|flags);
				if (bp[i] == NULL) {
					error = EIO;
				}
			}
		} else {
			bp[i] = NULL;
		}
		off += len;
		vn_tot -= len;
		if (vn_tot <= 0) {
			break;
		}
	}
	if ((flags & B_ASYNC) == 0) {
		/* Wait for I/O to complete if the request is not B_ASYNC. */
		for (i = 0; i < (int)iop->nioblk; i++) {
			if (bp[i] != NULL) {
				if (iop->ioblk[i].imap.flags & M_OBJECT) {
					error = sam_pg_object_sync_done(ip,
					    bp[i], "PUTPAGE");
				} else {
					int err;

					err = biowait(bp[i]);
					pageio_done(bp[i]);
					if (error == 0) {
						error = err;
					}
				}
			}
		}
		pvn_write_done(pp, ((error) ? B_ERROR : 0) | B_WRITE | flags);
	} else {
		/* Check if all buffers were sent  */
		for (i = 0; i < (int)iop->nioblk; i++) {
			if (bp[i] == NULL) {
				mutex_enter(&ip->write_mutex);
				pp->p_fsdata--;
				if ((int)(pp->p_fsdata) == 0) {
					mutex_exit(&ip->write_mutex);
					pvn_write_done(pp, (B_WRITE|flags));
				} else {
					mutex_exit(&ip->write_mutex);
				}
			}
		}
	}
	if (offset) {
		*offset = vn_off;
	}
	if (length) {
		*length = vn_len;
	}
	return (error);
}


/*
 * ----- sam_putpages - Put pages from a SAM-QFS file.
 *
 * Put pages from a file for use in a memory mapped function.
 * If length == 0, put pages from offset to EOF. The typical
 * cases should be length == 0 & offset == 0 (entire vnode page list),
 * length == MAXBSIZE (segmap_release), and
 * length == PAGESIZE (from the pageout daemon or fsflush).
 */

int				/* ERRNO if error, 0 if successful. */
sam_putpages(
	vnode_t *vp,		/* Pointer to vnode. */
	offset_t offset,	/* file offset */
	sam_size_t length,	/* file length provided to mmap. */
	int flags,		/* flags - B_INVAL, B_DIRTY, B_FREE, */
				/*   B_DONTNEED, B_FORCE, B_ASYNC. */
	cred_t *credp)		/* credentials pointer. */
{
	int error = 0;
	sam_node_t *ip;
	page_t *pp;
	sam_size_t vn_len;
	offset_t endoff;
	sam_u_offset_t vn_off;
	boolean_t lock_set = B_FALSE;

	ip = SAM_VTOI(vp);
	if (length == 0) {
		mutex_enter(&ip->write_mutex);
		ip->koffset = 0;
		ip->klength = 0;
		mutex_exit(&ip->write_mutex);
	}

	if (RW_OWNER_OS(&ip->inode_rwl) != curthread) {

		/*
		 * We could deadlock the page daemon if we don't hold the
		 * inode rw lock as a writer.  This can happen when another
		 * thread is handling a mmapped write and the pager starts
		 * to replenish memory, calling this routine in the process.
		 * Explicitly check for the pager and return if we don't
		 * hold the lock as a writer.
		 */
		if (curproc == proc_pageout) {
			return (0);
		}

		/*
		 * Acquire the inode reader lock if lock not already held
		 * by this thread.  Enforce the sam_put/getpage_vn locking rule.
		 */
		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		lock_set = B_TRUE;
	}

	/*
	 * For length == 0, Search entire vnode page list for dirty pages >=
	 * offset. Clear the modify time flag if ALL the pages are flushed
	 * successfully.
	 */
	if (length == 0) {
		error = pvn_vplist_dirty(vp, offset, sam_putapage,
		    flags, credp);
		if ((error == 0) && (offset == 0)) {
			/*
			 * If the pages have been completely flushed without
			 * error, clear the UPDATED flag.  In addition, we set
			 * the DIRTY flag to remember to flush the inode.
			 */
			if (lock_set) {
				mutex_enter(&ip->fl_mutex);
			}
			ip->flags.bits &= ~SAM_VALID_MTIME;
			if (ip->flags.bits & SAM_UPDATED) {
				ip->flags.bits &= ~SAM_UPDATED;
				ip->flags.bits |= SAM_DIRTY;
			}
			if (lock_set) {
				mutex_exit(&ip->fl_mutex);
			}
		}
		goto out;
	}

	/*
	 * Search the range (offset, offset+length) looking for dirty pages.
	 */
	vn_off = offset;
	endoff = offset + length;
	while (vn_off < endoff) {
		/*
		 * The offset provided to page lookup routines must be page
		 * aligned.
		 */
		ASSERT((vn_off & (PAGESIZE-1)) == 0);

		if ((flags & B_INVAL) || ((flags & B_ASYNC) == 0)) {
			pp = page_lookup(vp, vn_off,
			    ((flags & (B_INVAL | B_FREE)) ?
			    SE_EXCL : SE_SHARED));
		} else {
			pp = page_lookup_nowait(vp, vn_off,
			    ((flags & B_FREE) ? SE_EXCL : SE_SHARED));
		}
		if ((pp == NULL) || (pvn_getdirty(pp, flags) == 0)) {
			vn_len = PAGESIZE;
		} else {
			if ((error = sam_putapage(vp,
			    pp, &vn_off, &vn_len, flags, credp))) {
				break;
			}
		}
		vn_off += vn_len;
	}

out:
	if (lock_set) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	}
	return (error);
}


/*
 * ----- sam_pageio - Start block io on a page.
 *
 * Start block io on a page used in memory mapped io.
 */

static buf_t *
sam_pageio(
	sam_node_t *ip,		/* Pointer to inode table */
	sam_ioblk_t *iop,	/* Pointer to I/O entry */
	page_t *pp,		/* Pointer to page list */
	offset_t off,		/* Logical file offset */
	uint_t pg_off,		/* Offset within page */
	uint_t vn_len,		/* Length of transfer */
	int flags)		/* Flags --B_INVAL, B_DIRTY, B_FREE, */
				/*   B_DONTNEED, B_FORCE, B_ASYNC. */
{
	buf_t *bp;
	sam_mount_t *mp;		/* Pointer to mount table */

	mp = ip->mp;
	if (iop->blkno < mp->mi.m_fs[iop->imap.ord0].system) {
		cmn_err(CE_WARN,
			"SAM-QFS: %s: sam_pageio invalid block:"
				" ino=%d.%d, bn=%llx, ord=%d, system=%x",
			mp->mt.fi_name, ip->di.id.ino,
			ip->di.id.gen, (long long)iop->blkno,
			iop->ord, mp->mi.m_fs[iop->imap.ord0].system);
		sam_req_ifsck(mp, iop->ord,
				"sam_pageio: bad block", &ip->di.id);
		return (NULL);
	}
	bp = pageio_setup(pp, vn_len, mp->mi.m_fs[iop->ord].svp, flags);
	bp->b_edev = mp->mi.m_fs[iop->ord].dev;
	bp->b_dev = cmpdev(bp->b_edev);
	bp->b_blkno = iop->blkno << SAM2SUN_BSHIFT;
	bp->b_un.b_addr = (caddr_t)pg_off;
	bp->b_file = SAM_ITOP(ip);
	bp->b_offset = off;
	TRACE(T_SAM_STARTBIO, SAM_ITOV(ip),
	    (ip->rdev ? (sam_tr_t)bp->b_dev : iop->ord),
	    iop->blkno, vn_len);
	if (!(bp->b_flags & B_READ)) {	/* If writing */
		if (SAM_IS_SHARED_CLIENT(ip->mp) && S_ISDIR(ip->di.mode)) {
			/*
			 * Never write directory pages on the client.
			 */
			cmn_err(CE_WARN,
				"SAM-QFS: %s: wr dir page, mod=%d, ip=%p,"
					" ino=%d.%d, bn=%llx, ord=%d",
				mp->mt.fi_name, hat_ismod(pp),
				(void *)ip, ip->di.id.ino,
				ip->di.id.gen, (long long)iop->blkno, iop->ord);
			return (NULL);
		}
		if (bp->b_flags & B_ASYNC) {
			bp->b_iodone = (int (*) ())sam_page_wrdone;
			TRACE(T_SAM_ASYNC_WR, SAM_ITOV(ip), ip->di.id.ino,
			    (sam_tr_t)bp->b_pages, (sam_tr_t)bp);
			mutex_enter(&ip->write_mutex);
			ip->cnt_writes += bp->b_bcount;
			mutex_exit(&ip->write_mutex);
		}
	}
	if (TRANS_ISTRANS(ip->mp)) {
		lqfs_strategy(ip->mp->mi.m_log, bp);
	} else {
		if ((bp->b_flags & B_READ) == 0) {
			LQFS_MSG(CE_WARN, "sam_pageio(): bdev_strategy writing "
				"mof 0x%x edev %ld nb %d\n", bp->b_blkno * 512,
					bp->b_edev, bp->b_bcount);
		} else {
			LQFS_MSG(CE_WARN, "sam_pageio(): bdev_strategy reading "
				"mof 0x%x edev %ld nb %d\n", bp->b_blkno * 512,
					bp->b_edev, bp->b_bcount);
		}
		bdev_strategy(bp);
	}
	return (bp);
}


/*
 * ----- sam_page_wrdone -
 *
 * Process write page finish.
 */

void
sam_page_wrdone(buf_t *bp)
{
	sam_node_t *ip;
	page_t *pp;

	pp = bp->b_pages;
	ip = SAM_VTOI(pp->p_vnode);
	bp->b_iodone = NULL;

	mutex_enter(&ip->write_mutex);
	if (bp->b_flags & B_ERROR) {
		ip->wr_errno = EIO;
	}

	/*
	 * Wake up waiting threads if no I/O or write byte count below
	 * threshold.
	 */
	sam_write_done(ip, bp->b_bcount);

	/*
	 * Check for all block I/O finished in this page. If finished,
	 * call biodone to release buffer and page.
	 */
	pp->p_fsdata--;
	if ((int)(pp->p_fsdata) <= 0) {
		pp->p_fsdata = 0;
		mutex_exit(&ip->write_mutex);
		biodone(bp);
	} else {
		pageio_done(bp);
		mutex_exit(&ip->write_mutex);
	}
}


/*
 * ----- sam_write_done -
 *
 * Check if any threads are waiting on async writes to finish and
 * wake them up if all writes have finished. Also check if any
 * thread is waiting because there are more writes outstanding
 * than the write threshold and wake them up if below threshold.
 */

void
sam_write_done(
	sam_node_t *ip,		/* Pointer to inode */
	uint_t	blength)	/* Write byte count */
{
	ASSERT(MUTEX_HELD(&ip->write_mutex));
	ip->cnt_writes -= blength;
	if ((ip->wr_fini && (ip->cnt_writes <= 0)) ||
	    (ip->wr_thresh && (ip->cnt_writes < ip->mp->mt.fi_wr_throttle))) {
		cv_broadcast(&ip->write_cv);
	}
}


/*
 * ----- sam_flush_pages -
 *
 * Wait for any async I/O pages currently being written to complete.
 * If any v_pages, write pages. Wait for I/O to finish for all
 * flags except B_ASYNC.
 *
 * NOTE: requires inode write lock set (ip->inode_rwl).
 */

int
sam_flush_pages(
	sam_node_t *ip,		/* Pointer to inode */
	int flags)		/* flags - B_INVAL, B_FREE, B_ASYNC, or 0, */
{
	vnode_t *vp = SAM_ITOV(ip);
	int error;

	(void) sam_wait_async_io(ip, TRUE);
	if (vn_has_cached_data(vp) != 0) {
		sam_size_t length;

		length = 0;
		if (ip->flags.b.staging) {
			if (ip->stage_size == 0) {
				return (0);
			}
			length = ip->stage_size;
		}
		ASSERT(RW_OWNER_OS(&ip->inode_rwl) == curthread);
		/*
		 * Synchronously write pages out.
		 *  0	= wait until pages written.
		 *  B_INVAL = wait until pages written & invalidate the pages.
		 *  B_FREE  = wait until pages written & free the pages.
		 *  B_ASYNC = write pages, but do not wait until pages written.
		 */
		if (flags == 0) {
			int err;

			err = VOP_PUTPAGE_OS(vp, 0, length, B_ASYNC, CRED(),
			    NULL);
			error = (sam_wait_async_io(ip, TRUE));
			if (error == 0) {
				error = err;
			}
		} else {
			error = VOP_PUTPAGE_OS(vp, 0, length, flags, CRED(),
			    NULL);
		}
		if (error != 0) {
			TRACE(T_SAM_FLUSHERR1, vp, ip->di.id.ino, vp->v_count,
			    ip->flags.bits);
		}
		ip->klength = ip->koffset = 0;
	} else {
		error = 0;
	}
	if (flags != B_ASYNC) {
		ip->flags.b.stage_pages = 0;
	}

	return (error);
}


/*
 *	----- sam_wait_async_io -
 *
 * Wait for any async I/O pages currently being written to complete.
 * It is not sufficient to check v_pages because the page is assigned
 * to the spec fs during the I/O. The buffer points to vnode owner and this
 * vnode is put back in the page after the I/O finishes. Therefore,
 * write bytes outstanding must be zero to cover this window.
 */

int
sam_wait_async_io(
	sam_node_t *ip,		/* Pointer to inode */
	boolean_t write_lock)	/* Inode WRITERS lock held */
{
	int error;

	/*
	 * Lift inode lock so I/O can complete.
	 */
	if (write_lock) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	}
	SAM_COUNT64(page, flush);
	mutex_enter(&ip->write_mutex);
	ip->wr_fini++;
	while (ip->cnt_writes) {
		cv_wait(&ip->write_cv, &ip->write_mutex);
	}
	if (--ip->wr_fini < 0) {
		ip->wr_fini = 0;
	}
	error = ip->wr_errno;
	ip->wr_errno = 0;
	mutex_exit(&ip->write_mutex);

	if (write_lock) {
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	}
	return (error);
}


/*
 * ----- sam_map -
 *
 * Return disk block or optical block based on ip->rdev for all
 * blocks in the page.
 */

static int			/* Errno if error, 0 if successful */
sam_map(
	sam_node_t *ip,		/* Pointer to inode table */
	offset_t off,		/* File offset */
	sam_iohdr_t *iop)	/* Pointer to ioblk array and header */
{
	int error;
	int i, dt;
	int maxbp;

	if (SAM_IS_OBJECT_FILE(ip)) {
		error = sam_map_osd(ip, off, (offset_t)PAGESIZE, SAM_READ_PUT,
		    &iop->ioblk[0]);
		iop->nioblk = 1;
		iop->contig = iop->ioblk[0].contig;
		return (error);
	}

	dt = ip->di.status.b.meta;
	if ((maxbp = (PAGESIZE >> SM_SHIFT(ip->mp, dt))) == 0) {
		maxbp = 1;
	}
	iop->nioblk = 0;
	iop->contig = 0;
	for (i = 0; i < maxbp; i++) {
		if (!S_ISREQ(ip->di.mode)) {
			error = sam_map_block(ip, off, (offset_t)PAGESIZE,
			    (SAM_READ_PUT | SAM_MAP_NOWAIT),
			    &iop->ioblk[i], CRED());
		} else {
			if (ip->rdev == 0) {
				if ((error = ip->rm_err) == 0) {
					error = ENODEV;
				}
			} else {
				error = sam_rmmap_block(ip, off,
				    (offset_t)PAGESIZE,
				    SAM_READ_PUT, &iop->ioblk[i]);
			}
		}
		if (error) {
			break;
		}
		iop->nioblk++;
		if (iop->ioblk[i].imap.flags & M_SPARSE) {
			int bsize =
			    ip->mp->mi.m_dau[dt].size[iop->ioblk[i].imap.bt];

			off += bsize;
			iop->contig += bsize;
		} else {
			sam_size_t contig;

			if ((iop->ioblk[i].contig - iop->ioblk[i].pboff) >
			    ip->mp->mi.m_maxphys) {
				/*
				 * Over the max allowed, set contig so we
				 * get maxphys after subtracting pboff later.
				 */
				iop->ioblk[i].contig =
				    contig = ip->mp->mi.m_maxphys +
				    iop->ioblk[i].pboff;

			} else {
				/*
				 * This value is OK since contig - pboff will
				 * not be greater than maxphys.
				 */
				contig = iop->ioblk[i].contig;
			}
			off += contig;
			iop->contig += contig;
		}
		if (iop->contig >= PAGESIZE) {
			int ovfl;

			/* Round down last contig seqment to PAGESIZE */
			if ((ovfl = (iop->contig & (PAGESIZE - 1))) != 0) {
				iop->ioblk[i].contig -= ovfl;
				iop->contig -= ovfl;
			}
			break;
		}
	}
	return (error);
}
