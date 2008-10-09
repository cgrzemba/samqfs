/*
 * GPL Notice
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 only.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software Foundation,
 *	Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

/*
 *    SAM-QFS_notice_begin
 *
 *      Solaris 2.x Sun Storage & Archiving Management File System
 *
 *      Copyright (c) 2008 Sun Microsystems, Inc.
 *      All Rights Reserved.
 *
 *      Government Rights Notice
 *      Use, duplication, or disclosure by the U.S. Government is
 *      subject to restrictions set forth in the Sun Microsystems,
 *      Inc. license agreements and as provided in DFARS 227.7202-1(a)
 *      and 227.7202-3(a) (1995), DRAS 252.227-7013(c)(ii) (OCT 1988),
 *      FAR 12.212(a)(1995), FAR 52.227-19, or FAR 52.227-14 (ALT III),
 *      as applicable.  Sun Microsystems, Inc.
 *
 *    SAM-QFS_notice_end
 */

/*
 * Portions copyrighted:
 *  1991, 1992, 1994, 1999, 2002  by Linus Torvalds
 *  1995, 1996 Olaf Kirch <okir@monad.swb.de>
 *  1999 G. Allen Morris III <gam3@acm.org>
 */

/*
 *      For the avoidance of doubt, except that if any license choice other
 *      than GPL or LGPL is available it will apply instead, Sun elects to
 *      use only the General Public License version 2 (GPLv2) at this time
 *      for any software where a choice of GPL license versions is made
 *      available with the language indicating that GPLv2 or any later
 *      version may be used, or where a choice of which version of the GPL
 *      is applied is otherwise unspecified.
 */

#ifdef sun
#pragma ident "$Revision: 1.38 $"
#endif

#include <linux/module.h>
#include <asm/semaphore.h>
#include <linux/dcache.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <asm/string.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
#include <linux/iobuf.h>
#include <linux/locks.h>
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#include <linux/buffer_head.h>
#include <linux/uio.h>
#include <linux/blkdev.h>
#include <linux/writeback.h>
#endif


#include <asm/param.h>
#include <asm/uaccess.h>
#include <asm/system.h>


#if defined(CONFIG_KMOD) && defined(CONFIG_NET)
#include <linux/kmod.h>
#endif

/*
 * I/O completion handler for block_read_full_page() - pages
 * which come unlocked at the end of I/O.
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)) && \
	(LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 9))
/* From SLES9/SP2, fs/buffer.c */
static void
end_buffer_async_read(struct buffer_head *bh, int uptodate)
{
	static spinlock_t page_uptodate_lock = SPIN_LOCK_UNLOCKED;
	unsigned long flags;
	struct buffer_head *tmp;
	struct page *page;
	int page_uptodate = 1;

	BUG_ON(!buffer_async_read(bh));

	page = bh->b_page;
	if (uptodate) {
		set_buffer_uptodate(bh);
	} else {
		clear_buffer_uptodate(bh);
		printk(KERN_CRIT "Buffer I/O error\n");
		SetPageError(page);
	}

	/*
	 * Be _very_ careful from here on. Bad things can happen if
	 * two buffer heads end IO at almost the same time and both
	 * decide that the page is now completely done.
	 */
	spin_lock_irqsave(&page_uptodate_lock, flags);
	clear_buffer_async_read(bh);
	unlock_buffer(bh);
	tmp = bh;
	do {
		if (!buffer_uptodate(tmp))
			page_uptodate = 0;
		if (buffer_async_read(tmp)) {
			BUG_ON(!buffer_locked(tmp));
			goto still_busy;
		}
		tmp = tmp->b_this_page;
	} while (tmp != bh);
	spin_unlock_irqrestore(&page_uptodate_lock, flags);

	/*
	 * If none of the buffers had errors and they are all
	 * uptodate then we can set the page uptodate.
	 */
	if (page_uptodate && !PageError(page))
		SetPageUptodate(page);
	unlock_page(page);
	return;

still_busy:
	spin_unlock_irqrestore(&page_uptodate_lock, flags);
}


/*
 * Generic "read page" function for block devices that have the normal
 * get_block functionality. This is most of the block device filesystems.
 * Reads the page asynchronously --- the unlock_buffer() and
 * mark_buffer_uptodate() functions propagate buffer state into the
 * page struct once IO has completed.
 */
/* From SLES9/SP2, fs/buffer.c:block_read_full_page() */
int
rfs_block_read_full_page(struct page *page, get_block_t *get_block)
{
	struct inode *inode = page->mapping->host;
	sector_t iblock, lblock;
	struct buffer_head *bh, *head, *arr[MAX_BUF_PER_PAGE];
	unsigned int blocksize;
	int nr, i;
	int fully_mapped = 1;
	int err;

	if (!PageLocked(page))
		PAGE_BUG(page);
	if (PageUptodate(page))
		buffer_error();
	blocksize = 1 << inode->i_blkbits;
	if (!page_has_buffers(page))
		create_empty_buffers(page, blocksize, 0);
	head = page_buffers(page);

	iblock = (sector_t)page->index << (PAGE_CACHE_SHIFT - inode->i_blkbits);
	lblock = (i_size_read(inode)+blocksize-1) >> inode->i_blkbits;
	bh = head;
	nr = 0;
	i = 0;

	do {
		if (buffer_uptodate(bh))
			continue;

		if (!buffer_mapped(bh)) {
			fully_mapped = 0;
			if (iblock < lblock) {
				err = get_block(inode, iblock, bh, 0);
				if (err) {
					SetPageError(page);
					unlock_page(page);
					return (err);
				}
			}
			if (!buffer_mapped(bh)) {
				void *kaddr = kmap_atomic(page, KM_USER0);
				memset(kaddr + i * blocksize, 0, blocksize);
				flush_dcache_page(page);
				kunmap_atomic(kaddr, KM_USER0);
				set_buffer_uptodate(bh);
				continue;
			}
			/*
			 * get_block() might have updated the buffer
			 * synchronously
			 */
			if (buffer_uptodate(bh))
				continue;
		}
		arr[nr++] = bh;
	} while (i++, iblock++, (bh = bh->b_this_page) != head);

	if (fully_mapped)
		SetPageMappedToDisk(page);

	if (!nr) {
		/*
		 * All buffers are uptodate - we can set the page uptodate
		 * as well. But not if get_block() returned an error.
		 */
		if (!PageError(page))
			SetPageUptodate(page);
		unlock_page(page);
		return (0);
	}

	/* Stage two: lock the buffers */
	for (i = 0; i < nr; i++) {
		bh = arr[i];
		lock_buffer(bh);
		mark_buffer_async_read(bh);
	}

	/*
	 * Stage 3: start the IO.  Check for uptodateness
	 * inside the buffer lock in case another process reading
	 * the underlying blockdev brought it uptodate (the sct fix).
	 */
	for (i = 0; i < nr; i++) {
		bh = arr[i];
		if (buffer_uptodate(bh))
			end_buffer_async_read(bh, 1);
		else
			submit_bh(READ, bh);
	}
	return (0);
}
#endif /* >=2.6.0 && <2.6.9 */


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 9)
int
rfs_block_read_full_page(struct page *page, get_block_t *get_block)
{
	return (block_read_full_page(page, get_block));
}
#endif /* LINUX_VERSION_CODE >= 2.6.9 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
#ifndef MAX_BUF_PER_PAGE
#define	MAX_BUF_PER_PAGE (PAGE_CACHE_SIZE / 512)
#endif

/* From RHEL3/U5, fs/buffer.c:block_read_full_page(). */
int
rfs_block_read_full_page(struct page *page, get_block_t *get_block)
{
	struct inode *inode = page->mapping->host;
	unsigned long iblock, lblock;
	struct buffer_head *bh, *head, *arr[MAX_BUF_PER_PAGE];
	unsigned int blocksize, blocks;
	int nr, i;
	int err;

	if (!PageLocked(page))
		PAGE_BUG(page);
	blocksize = 1 << inode->i_blkbits;
	if (!page->buffers)
		create_empty_buffers(page, inode->i_dev, blocksize);
	head = page->buffers;

	blocks = PAGE_CACHE_SIZE >> inode->i_blkbits;
	iblock = page->index << (PAGE_CACHE_SHIFT - inode->i_blkbits);
	lblock = (inode->i_size+blocksize-1) >> inode->i_blkbits;
	bh = head;
	nr = 0;
	i = 0;

	do {
		if (buffer_uptodate(bh))
			continue;

		if (!buffer_mapped(bh)) {
			if (iblock < lblock) {
				err = get_block(inode, iblock, bh, 0);
				if (err) {
					UnlockPage(page);
					return (err);
				}
			}
			if (!buffer_mapped(bh)) {
				memset(kmap(page) + i*blocksize, 0, blocksize);
				flush_dcache_page(page);
				kunmap(page);
				set_bit(BH_Uptodate, &bh->b_state);
				continue;
			}
			/*
			 * get_block() might have updated the buffer
			 * synchronously
			 */
			if (buffer_uptodate(bh))
				continue;
		}

		arr[nr] = bh;
		nr++;
	} while (i++, iblock++, (bh = bh->b_this_page) != head);

	if (!nr) {
		/*
		 * all buffers are uptodate - we can set the page
		 * uptodate as well.
		 */
		SetPageUptodate(page);
		UnlockPage(page);
		return (0);
	}

	/* Stage two: lock the buffers */
	for (i = 0; i < nr; i++) {
		struct buffer_head *bh = arr[i];
		lock_buffer(bh);
		set_buffer_async_io(bh);
	}

	/* Stage 3: start the IO */
	for (i = 0; i < nr; i++) {
		struct buffer_head *bh = arr[i];
		if (buffer_uptodate(bh))
			end_buffer_io_async(bh, 1);
		else
			submit_bh(READ, bh);
	}

	wakeup_page_waiters(page);
	return (0);
}
#endif /* < 2.6.0 */


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
/* From RHEL3/U5, fs/buffer.c */
void
unmap_underlying_metadata(struct buffer_head *bh)
{
	struct buffer_head *old_bh;

	old_bh = get_hash_table(bh->b_dev, bh->b_blocknr, bh->b_size);
	if (old_bh) {
		mark_buffer_clean(old_bh);
		wait_on_buffer(old_bh);
		clear_bit(BH_Req, &old_bh->b_state);
		__brelse(old_bh);
	}
}
#endif /* <2.6.0 */


#if defined(RHE_LINUX) && (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
/*
 * Main direct IO helper functions.  The on-disk mapping is constructed
 * by consulting <get_block> on <blocksize>-sized blocks, but the actual
 * IO is performed in <sectsize>-sized chunks, allowing
 * sector-granularity direct IO on regular files.
 *
 * It is up to the caller to ensure that sector-aligned IO is only
 * requested if the device is known to be capable of varyIO.
 */

/* From RHEL3/U5, fs/buffer.c:prepare_direct_IO_iobuf() */
static int
rfs_prepare_direct_IO_iobuf(
	int rw,
	struct inode *inode,
	struct kiobuf *iobuf,
	unsigned long blocknr,
	int blocksize,
	int sectsize,
	int s_offset,
	get_block_t *get_block,
	int *beyond_eof)
{
	int j, retval = 0;
	unsigned long *blocks = iobuf->blocks;
	int length;
	int sectors_per_block, sect_index = 0;
	unsigned long sector;
	int iosize = 0, chunksize, nr_sectors;
	kdev_t first_dev;
	int saved_first_dev = 0;

	*beyond_eof = 0;
	sectors_per_block = blocksize / sectsize;
	length = iobuf->length;
	nr_sectors = length / sectsize;

	/* Ugly, the blocks array size is currently hard-coded in iobuf.c. */
	if (nr_sectors > KIO_MAX_SECTORS)
		BUG();

	/* build the blocklist */
	while (nr_sectors > 0) {
		struct buffer_head bh;

		bh.b_state = 0;
		bh.b_dev = 0;
		bh.b_size = blocksize;
		bh.b_page = NULL;

		chunksize = sectors_per_block - s_offset;
		if (chunksize > nr_sectors)
			chunksize = nr_sectors;

		if (((loff_t)blocknr) * blocksize >= inode->i_size) {
			*beyond_eof = 1;
			if (s_offset != 0 || chunksize != sectors_per_block) {
				/*
				 * We can't safely do direct writes to a
				 * partial filesystem block beyond EOF
				 */
				return (-ENOTBLK);
			}
		}

		/*
		 * Only allow get_block to create new blocks if we are safely
		 * beyond EOF.  O_DIRECT is unsafe inside sparse files.
		 */
		retval = get_block(inode, blocknr, &bh,
		    ((rw != READ) && *beyond_eof));

		if (retval) {
			if (!iosize)
				/* report error to userspace */
				goto out;
			else
				/* do short I/O until 'i' */
				break;
		}

		if (saved_first_dev == 0) {
			first_dev = bh.b_dev;
			saved_first_dev = 1;
		} else if (bh.b_dev != first_dev) {
			/*
			 * This block of IO has crossed a device boundary.
			 */
			break;
		}

		if (rw == READ) {
			if (buffer_new(&bh))
				BUG();
			if (!buffer_mapped(&bh)) {
				/* there was an hole in the filesystem */
				for (j = 0; j < chunksize; j++)
					blocks[sect_index++] = -1UL;
				goto next;
			}
		} else {
			if (buffer_new(&bh))
				unmap_underlying_metadata(&bh);
			if (!buffer_mapped(&bh))
				/*
				 * upper layers need to pass the error on or
				 * fall back to buffered IO.
				 */
				return (-ENOTBLK);
		}
		sector = bh.b_blocknr * sectors_per_block;
		for (j = 0; j < chunksize; j++)
			blocks[sect_index++] = sector + s_offset + j;
	next:
		iosize += chunksize;
		nr_sectors -= chunksize;
		s_offset = 0;
		blocknr++;
	}

	/* patch length to handle short I/O */
	iobuf->length = iosize * sectsize;

out:
	return (retval);
}

/* From RHEL3/U5, fs/buffer.c:generic_direct_sector_IO() */
int
rfs_generic_direct_sector_IO(
	int rw,
	struct inode *inode,
	struct kiobuf *iobuf,
	unsigned long blocknr,
	int blocksize,
	int sectsize,
	int s_offset,
	get_block_t *get_block,
	int drop_locks)
{
	int retval, beyond_eof;
	int old_length = iobuf->length;
	kdev_t real_dev;
	struct buffer_head bh;

	/*
	 * Find the device where this block of IO starts.
	 */
	bh.b_dev = 0;
	bh.b_state = 0;
	bh.b_size = blocksize;
	bh.b_page = NULL;

	retval = get_block(inode, blocknr, &bh, 0);
	if (retval) {
		goto out;
	}
	real_dev = bh.b_dev;

	retval = rfs_prepare_direct_IO_iobuf(rw, inode, iobuf, blocknr,
	    blocksize,
	    sectsize, s_offset, get_block,
	    &beyond_eof);

	if (retval)
		goto out;

	if (drop_locks && !beyond_eof)
		up(&inode->i_sem);
	retval = brw_kiovec(rw, 1, &iobuf, real_dev, iobuf->blocks, sectsize);

	if (drop_locks && !beyond_eof)
		down(&inode->i_sem);

	/* restore orig length */
	iobuf->length = old_length;

out:
	return (retval);
}
#endif /* RHE_LINUX < 2.6.0 */


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
/*
 * Default synchronous end-of-IO handler..  Just mark it up-to-date and
 * unlock the buffer. This is what ll_rw_block uses too.
 */
/* From RHEL3/U5, fs/buffer.c:end_buffer_io_sync() */
void
rfs_end_buffer_io_sync(struct buffer_head *bh, int uptodate)
{
	mark_buffer_uptodate(bh, uptodate);
	unlock_buffer(bh);
	put_bh(bh);
}

/* Based on RHEL3/U5, fs/buffer.c:block_truncate_page() */
int
rfs_do_linux_fbzero(
	struct inode *inode,
	loff_t start,
	unsigned zlen,
	kdev_t dev,
	int ord,
	int qfs_blkno)
{
	struct address_space *mapping = inode->i_mapping;
	unsigned long index;
	struct page *page;
	loff_t offset, pos;
	unsigned long blocksize;
	int error;
	struct buffer_head *bh;

	if (!zlen) {
		return (0);
	}

	index = start >> PAGE_CACHE_SHIFT;
	page = find_or_create_page(mapping, index, GFP_NOFS);
	if (!page) {
		return (-ENOMEM);
	}

	blocksize = inode->i_sb->s_blocksize;

	if (!page->buffers) {
		create_empty_buffers(page, inode->i_dev, blocksize);
	}

	offset = start & (PAGE_CACHE_SIZE-1);

	/*
	 * Find the buffer that contains "offset".
	 */
	bh = page->buffers;
	pos = blocksize;
	while (offset >= pos) {
		bh = bh->b_this_page;
		pos += blocksize;
	}

	error = 0;
	bh->b_end_io = rfs_end_buffer_io_sync;
	bh->b_dev = dev;

	if (!buffer_mapped(bh)) {
		/*
		 * We know what disk block we want, and it must've been
		 * allocated or we wouldn't be here.
		 */
		bh->b_blocknr = qfs_blkno;
		set_bit(BH_Mapped, &bh->b_state);
	}

	/*
	 * If the page is up-to-date, the underlying buffers must be too.
	 */
	if (Page_Uptodate(page)) {
		set_bit(BH_Uptodate, &bh->b_state);
	}

	/*
	 * If we're about to zero an entire block, don't bother reading it in.
	 */
	if (zlen == blocksize) {
		set_bit(BH_Uptodate, &bh->b_state);
	}

	/*
	 * The buffer still isn't uptodate, we need to read before zeroing.
	 */
	if (!buffer_uptodate(bh)) {
		ll_rw_block(READ, 1, &bh);
		wait_on_buffer(bh);
		if (!buffer_uptodate(bh)) {
			error = -EIO;
			goto unlock;
		}
	}

	memset(kmap(page) + offset, 0, zlen);
	flush_dcache_page(page);
	kunmap(page);

	mark_buffer_dirty(bh);
	buffer_insert_inode_data_queue(bh, inode);
	mark_buffer_uptodate(bh, 1);

unlock:
	UnlockPage(page);
	page_cache_release(page);
	return (error);
}

#endif /* linux 2.4 */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)

/* Based on SLES9/SP2, fs/buffer.c:block_truncate_page() */
int
rfs_do_linux_fbzero(
	struct inode *inode,
	loff_t start,
	unsigned zlen,
	struct block_device *dev,
	int ord,
	int qfs_blkno)
{
	struct address_space *mapping = inode->i_mapping;
	unsigned long index;
	struct page *page;
	loff_t offset, pos;
	unsigned long blocksize;
	int error;
	struct buffer_head *bh;
	void *kaddr;

	if (!zlen) {
		return (0);
	}

	index = start >> PAGE_CACHE_SHIFT;
	page = find_or_create_page(mapping, index, GFP_NOFS);
	if (!page) {
		return (-ENOMEM);
	}

	blocksize = inode->i_sb->s_blocksize;

	if (!page_has_buffers(page)) {
		create_empty_buffers(page, blocksize, 0);
	}

	offset = start & (PAGE_CACHE_SIZE-1);

	/*
	 * Find the buffer that contains "offset".
	 */
	bh = page_buffers(page);
	pos = blocksize;
	while (offset >= pos) {
		bh = bh->b_this_page;
		pos += blocksize;
	}

	error = 0;
	bh->b_end_io = end_buffer_read_sync;
	bh->b_bdev = dev;

	if (!buffer_mapped(bh)) {
		/*
		 * We know what disk block we want, and it must've been
		 * allocated or we wouldn't be here.
		 */
		bh->b_blocknr = qfs_blkno;
		set_bit(BH_Mapped, &bh->b_state);
	}

	/*
	 * If the page is up-to-date, the underlying buffers must be too.
	 */
	if (PageUptodate(page)) {
		set_buffer_uptodate(bh);
	}

	/*
	 * If we're about to zero an entire block, don't bother reading it in.
	 */
	if (zlen == blocksize) {
		set_buffer_uptodate(bh);
	}

	/*
	 * The buffer still isn't uptodate, we need to read before zeroing.
	 */
	if (!buffer_uptodate(bh)) {
		ll_rw_block(READ, 1, &bh);
		wait_on_buffer(bh);
		if (!buffer_uptodate(bh)) {
			error = -EIO;
			goto unlock;
		}
	}

	kaddr = kmap_atomic(page, KM_USER0);
	memset(kaddr + offset, 0, zlen);
	flush_dcache_page(page);
	kunmap_atomic(kaddr, KM_USER0);

	mark_buffer_dirty(bh);
	set_buffer_uptodate(bh);

unlock:
	unlock_page(page);
	page_cache_release(page);
	return (error);
}

#endif /* linux 2.6 */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
/* Based on SLES9/SP2, mm/filemap.c:generic_file_write() */
ssize_t
rfs_generic_file_write(struct file *file, const char __user *buf,
			size_t count, loff_t *ppos)
{
	ssize_t ret;
	struct iovec local_iov = { .iov_base = (void __user *)buf,
		.iov_len = count };

	ret = generic_file_write_nolock(file, &local_iov, 1, ppos);

	return (ret);
}
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)) && (defined(RHE_LINUX))
/* Based on RHEL3/U5, mm/filemap.c:generic_file_write() */
ssize_t
rfs_generic_file_write(struct file *file, const char *buf, size_t count,
			loff_t *ppos)
{
	struct inode    *inode = file->f_dentry->d_inode->i_mapping->host;
	ssize_t	err;

	if ((ssize_t)count < 0)
		return (-EINVAL);

	if (!access_ok(VERIFY_READ, buf, count))
		return (-EFAULT);

	if (file->f_flags & O_DIRECT) {
		err = generic_file_write(file, buf, count, ppos);
	} else {
		err = do_generic_file_write(file, buf, count, ppos);
	}
	return (err);
}
#endif


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
/*
 * Function to obtain a well-connected dentry.
 */
/* Based on RHEL3/U5, fs/nfsd/nfsfh.c:nfsd_iget() */
struct dentry *
rfs_d_alloc_anon(struct inode *inode)
{
	struct dentry *result;
	struct list_head *lp;

	spin_lock(&dcache_lock);
	for (lp = inode->i_dentry.next; lp != &inode->i_dentry;
	    lp = lp->next) {
		result = list_entry(lp, struct dentry, d_alias);
		if (! (result->d_flags & DCACHE_NFSD_DISCONNECTED)) {
			dget_locked(result);
			result->d_vfs_flags |= DCACHE_REFERENCED;
			spin_unlock(&dcache_lock);
			iput(inode);
			return (result);
		}
	}

	spin_unlock(&dcache_lock);
	result = d_alloc_root(inode);
	if (result == NULL) {
		iput(inode);
		return (ERR_PTR(-ENOMEM));
	}
	result->d_flags |= DCACHE_NFSD_DISCONNECTED;
	result->d_op = inode->i_sb->s_root->d_op;
	return (result);
}
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
#include <linux/file.h>

/* From RHEL3/U5, net/socket.c:sockfd_lookup() */
struct socket *
rfs_sockfd_lookup(int fd, int *err)
{
	struct file *file;
	struct inode *inode;
	struct socket *sock;

	if (!(file = fget(fd))) {
		*err = -EBADF;
		return (NULL);
	}

	inode = file->f_dentry->d_inode;
	if (!inode->i_sock || !(sock = &inode->u.socket_i)) {
		*err = -ENOTSOCK;
		fput(file);
		return (NULL);
	}

	if (sock->file != file) {
		printk(KERN_ERR "socki_lookup: socket file changed!\n");
		sock->file = file;
	}
	return (sock);
}
#endif
