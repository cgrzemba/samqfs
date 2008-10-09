/*
 *	qfs linux kernel interface
 *      maps qfs calls to appropriate kernel calls
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

const char *SMI_BCL[] = {
"        Sun Microsystems, Inc. Binary Code License Agreement",
"         ----------------------------------------------------",
"",
"1.  LICENSE TO USE.  Sun grants you a non-exclusive and non-transferable",
"license for the internal use only of the accompanying software and",
"documentation and any error corrections provided by Sun (collectively",
"'Software'), by the number of users and the class of computer hardware for",
"which the corresponding fee has been paid.",
"",
"2.  RESTRICTIONS  Software is confidential and copyrighted. Title to Software",
"and all associated intellectual property rights is retained by Sun and/or its",
"licensors.  Except as specifically authorized in any Supplemental License",
"Terms, you may not make copies of Software, other than a single copy of",
"Software for archival purposes.  Unless enforcement is prohibited by",
"applicable law, you may not modify, decompile, or reverse engineer Software.",
"You acknowledge that Software is not designed, licensed or intended for use",
"in the design, construction, operation or maintenance of any nuclear",
"facility.  Sun disclaims any express or implied warranty of fitness for such",
"uses.  No right, title or interest in or to any trademark, service mark, logo",
"or trade name of Sun or its licensors is granted under this Agreement.",
"",
"3. LIMITED WARRANTY.  Sun warrants to you that for a period of ninety (90)",
"days from the date of purchase, as evidenced by a copy of the receipt, the",
"media on which Software is furnished (if any) will be free of defects in",
"materials and workmanship under normal use.  Except for the foregoing,",
"Software is provided 'AS IS'.  Your exclusive remedy and Sun's entire",
"liability under this limited warranty will be at Sun's option to replace",
"Software media or refund the fee paid for Software.",
"",
"4.  DISCLAIMER OF WARRANTY.  UNLESS SPECIFIED IN THIS AGREEMENT, ALL EXPRESS",
"OR IMPLIED CONDITIONS, REPRESENTATIONS AND WARRANTIES, INCLUDING ANY IMPLIED",
"WARRANTY OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR",
"NON-INFRINGEMENT ARE DISCLAIMED, EXCEPT TO THE EXTENT THAT THESE DISCLAIMERS",
"ARE HELD TO BE LEGALLY INVALID.",
"",
"5.  LIMITATION OF LIABILITY.  TO THE EXTENT NOT PROHIBITED BY LAW, IN NO",
"EVENT WILL SUN OR ITS LICENSORS BE LIABLE FOR ANY LOST REVENUE, PROFIT OR",
"DATA, OR FOR SPECIAL, INDIRECT, CONSEQUENTIAL, INCIDENTAL OR PUNITIVE",
"DAMAGES, HOWEVER CAUSED REGARDLESS OF THE THEORY OF LIABILITY, ARISING OUT OF",
"OR RELATED TO THE USE OF OR INABILITY TO USE SOFTWARE, EVEN IF SUN HAS BEEN",
"ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.  In no event will Sun's liability",
"to you, whether in contract, tort (including negligence), or otherwise,",
"exceed the amount paid by you for Software under this Agreement.  The",
"foregoing limitations will apply even if the above stated warranty fails of",
"its essential purpose.",
"",
"6.  Termination.  This Agreement is effective until terminated.  You may",
"terminate this Agreement at any time by destroying all copies of Software.",
"This Agreement will terminate immediately without notice from Sun if you fail",
"to comply with any provision of this Agreement.  Upon Termination, you must",
"destroy all copies of Software.",
"",
"7.  Export Regulations.  All Software and technical data delivered under this",
"Agreement are subject to US export control laws and may be subject to export",
"or import regulations in other countries.  You agree to comply strictly with",
"all such laws and regulations and acknowledge that you have the",
"responsibility to obtain such licenses to export, re-export, or import as may",
"be required after delivery to you.",
"",
"8.  U.S. Government Restricted Rights.  If Software is being acquired by or",
"on behalf of the U.S. Government or by a U.S. Government prime contractor or",
"subcontractor (at any tier), then the Government's rights in Software and",
"accompanying documentation will be only as set forth in this Agreement; this",
"is in accordance with 48 CFR 227.7201 through 227.7202-4 (for Department of",
"Defense (DOD) acquisitions) and with 48 CFR 2.101 and 12.212 (for non-DOD",
"acquisitions).",
"",
"9.  Governing Law.  Any action related to this Agreement will be governed by",
"California law and controlling U.S. federal law.  No choice of law rules of",
"any jurisdiction will apply.",
"",
"10.  Severability. If any provision of this Agreement is held to be",
"unenforceable, this Agreement will remain in effect with the provision",
"omitted, unless omission would frustrate the intent of the parties, in which",
"case this Agreement will immediately terminate.",
"",
"11.  Integration.  This Agreement is the entire agreement between you and Sun",
"relating to its subject matter.  It supersedes all prior or contemporaneous",
"oral or written communications, proposals, representations and warranties and",
"prevails over any conflicting or additional terms of any quote, order,",
"acknowledgment, or other communication between the parties relating to its",
"subject matter during the term of this Agreement.  No modification of this",
"Agreement will be binding, unless in writing and signed by an authorized",
"representative of each party.",
"",
"For inquiries please contact: Sun Microsystems, Inc.  901 San Antonio Road,",
"Palo Alto, California 94303",
};

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

#if 0
#pragma ident "$Revision: 1.75 $"
#endif

#define	EXPORT_SYMTAB

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/skbuff.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/dcache.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/smp.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0))
#include <linux/file.h>
#include <linux/net.h>
#include <linux/buffer_head.h>
#include <linux/mpage.h>
#include <linux/namei.h>
#include <linux/pagemap.h>
#endif

#if defined(CONFIG_KMOD) && defined(CONFIG_NET)
#include <linux/kmod.h>
#endif

#if (!defined(_LP64) && !defined(__ia64))
#include <bits/wordsize.h>
#if __WORDSIZE == 32
#include "longlong.h"
DWtype __divdi3(DWtype u, DWtype v);
DWtype __moddi3(DWtype u, DWtype v);
#endif
#endif

/* global spin lock to protect QFS_printk */
spinlock_t QFS_printk_lock;

/* variables that need exportage */
unsigned long QFS___HZ;
volatile unsigned long *QFS_jiffies = &jiffies;
struct proc_dir_entry **QFS_proc_root_fs = &proc_root_fs;
unsigned long *QFS_num_physpages = &num_physpages;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
struct timeval *QFS_xtime = (struct timeval *)&xtime;
#else
time_t QFS_xtime(void) {
	struct timespec t;
	t = CURRENT_TIME;
	return (t.tv_sec);
}
#endif
int QFS_smp_num_cpus;
EXPORT_SYMBOL(QFS_smp_num_cpus);

EXPORT_SYMBOL(QFS___HZ);
EXPORT_SYMBOL(QFS_jiffies);
EXPORT_SYMBOL(QFS_proc_root_fs);
EXPORT_SYMBOL(QFS_num_physpages);
EXPORT_SYMBOL(QFS_xtime);
EXPORT_SYMBOL(SMI_BCL);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
extern unsigned int nr_free_pages(void);
extern int rfs_block_read_full_page(struct page *page, get_block_t *get_block);
extern ssize_t rfs_generic_file_write(struct file *file, const char *buf,
					size_t count, loff_t *ppos);
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
struct dentry *rfs_d_alloc_anon(struct inode *);
struct socket *rfs_sockfd_lookup(int fd, int *err);
#endif

#if defined(RHE_LINUX) && (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
extern int rfs_generic_direct_sector_IO(int rw, struct inode *inode,
				struct kiobuf *iobuf,
				unsigned long blocknr, int blocksize,
				int sectsize, int s_offset,
				get_block_t *get_block, int drop_locks);
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
extern int rfs_do_linux_fbzero(struct inode *, loff_t, unsigned,
				kdev_t, int, int);
#else
extern int rfs_do_linux_fbzero(struct inode *, loff_t, unsigned,
				struct block_device *, int, int);
#endif

/* functions */
void FASTCALL(QFS_add_wait_queue_exclusive(wait_queue_head_t *q,
					wait_queue_t *));

void
QFS_add_wait_queue_exclusive(wait_queue_head_t *q, wait_queue_t *wait)
{
	add_wait_queue_exclusive(q, wait);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
void
QFS_add_wait_queue(wait_queue_head_t *q, wait_queue_t *wait)
{
	add_wait_queue(q, wait);
}
#else

void FASTCALL(QFS_add_wait_queue(wait_queue_head_t *q, wait_queue_t *));

void
QFS_add_wait_queue(wait_queue_head_t *q, wait_queue_t *wait)
{
	add_wait_queue(q, wait);
}
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 15))
struct sk_buff *
QFS_rfs_alloc_skb(unsigned int size, int priority)
{
	return (alloc_skb(size, priority));
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15))
struct sk_buff *
QFS_rfs_alloc_skb(unsigned int size, gfp_t priority)
{
	return (alloc_skb(size, priority));
}
#endif

int
QFS_block_prepare_write(struct page *page, unsigned from, unsigned to,
			get_block_t *get_block)
{
	return (block_prepare_write(page, from, to, get_block));
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18))
int
QFS_block_sync_page(struct page *page)
{
	return (block_sync_page(page));
}
EXPORT_SYMBOL(QFS_block_sync_page);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18))
void
QFS_block_sync_page(struct page *page)
{
	block_sync_page(page);
}
EXPORT_SYMBOL(QFS_block_sync_page);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0))
int
QFS_block_write_full_page(struct page *page, get_block_t *get_block,
			struct writeback_control *wbcp)
{
	return (block_write_full_page(page, get_block, wbcp));
}

int
QFS_mpage_readpage(struct page *page, get_block_t get_block)
{
	return (mpage_readpage(page, get_block));
}
EXPORT_SYMBOL(QFS_mpage_readpage);

int
QFS_mpage_readpages(struct address_space *mapping, struct list_head *pages,
		    unsigned nr_pages, get_block_t get_block)
{
	return (mpage_readpages(mapping, pages, nr_pages, get_block));
}
EXPORT_SYMBOL(QFS_mpage_readpages);

int
QFS_mpage_writepages(struct address_space *mapping,
			struct writeback_control *wbc, get_block_t get_block)
{
	return (mpage_writepages(mapping, wbc, get_block));
}
EXPORT_SYMBOL(QFS_mpage_writepages);

#else
int
QFS_block_write_full_page(struct page *page, get_block_t *get_block)
{
	return (block_write_full_page(page, get_block));
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18))
ssize_t
QFS___blockdev_direct_IO(
	int rw,
	struct kiocb *kiocbp,
	struct inode *li,
	struct block_device *bdev,
	const struct iovec *iovp,
	loff_t offset,
	unsigned long ns,
	get_block_t *get_block,
	dio_iodone_t iodone,
	int nsl)
{
	int ret;

	ret = __blockdev_direct_IO(rw, kiocbp, li, bdev, iovp, offset, ns,
	    get_block, iodone, nsl);

	return (ret);
}
EXPORT_SYMBOL(QFS___blockdev_direct_IO);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)) && \
	(LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18))
ssize_t
QFS___blockdev_direct_IO(
	int rw,
	struct kiocb *kiocbp,
	struct inode *li,
	struct block_device *bdev,
	const struct iovec *iovp,
	loff_t offset,
	unsigned long ns,
	get_blocks_t *get_blocks,
	dio_iodone_t iodone,
	int nsl)
{
	int ret;

	ret = __blockdev_direct_IO(rw, kiocbp, li, bdev, iovp, offset, ns,
	    get_blocks, iodone, nsl);

	return (ret);
}
EXPORT_SYMBOL(QFS___blockdev_direct_IO);
#endif

#if defined(RHE_LINUX) && (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
int
QFS_rfs_generic_direct_sector_IO(int rw, struct inode *li,
				struct kiobuf *kiobuf, unsigned long bnr,
				int bsize,
				int ssize, int soffset,
				get_block_t *get_block, int dl)
{
	int ret;

	ret = rfs_generic_direct_sector_IO(rw, li, kiobuf, bnr, bsize, ssize,
	    soffset, get_block, dl);
	return (ret);
}
EXPORT_SYMBOL(QFS_rfs_generic_direct_sector_IO);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0))
struct buffer_head *
QFS_bread(struct block_device *bdev, sector_t block, int size)
{
	return (__bread(bdev, block, size));
}
#else
struct buffer_head *
QFS_bread(kdev_t dev, int block, int size)
{
	return (bread(dev, block, size));
}
#endif

void
QFS___brelse(struct buffer_head *buf)
{
	__brelse(buf);
}

void
QFS_clear_inode(struct inode *inode)
{
	clear_inode(inode);
}

struct proc_dir_entry *
QFS_create_proc_entry(const char *name, mode_t mode,
			struct proc_dir_entry *parent)
{
	return (create_proc_entry(name, mode, parent));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0))
void
QFS_rfs_daemonize(const char *name, ...)
{
	daemonize(name);
}
EXPORT_SYMBOL(QFS_rfs_daemonize);
#else
void
QFS_daemonize(void)
{
	daemonize();
}
EXPORT_SYMBOL(QFS_daemonize);
#endif

struct dentry *
QFS_d_alloc_root(struct inode *root_inode)
{
	return (d_alloc_root(root_inode));
}

void
QFS_d_delete(struct dentry *dentry)
{
	d_delete(dentry);
}

struct dentry *
QFS_d_lookup(struct dentry *parent, struct qstr *name)
{
	return (d_lookup(parent, name));
}

struct dentry *
QFS_d_find_alias(struct inode *inode)
{
	return (d_find_alias(inode));
}

void
QFS_d_instantiate(struct dentry *entry, struct inode *inode)
{
	d_instantiate(entry, inode);
}

void
QFS_do_gettimeofday(struct timeval *tv)
{
	do_gettimeofday(tv);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
int
QFS_rfs_do_linux_fbzero(struct inode *inode, loff_t start,
			unsigned zlen, kdev_t dev, int ord, int qfs_blkno)
{
	return (rfs_do_linux_fbzero(inode, start, zlen, dev, ord, qfs_blkno));
}
#else
int
QFS_rfs_do_linux_fbzero(struct inode *inode, loff_t start,
			unsigned zlen, struct block_device *dev, int ord,
			int qfs_blkno)
{
	return (rfs_do_linux_fbzero(inode, start, zlen, dev, ord, qfs_blkno));
}
#endif
EXPORT_SYMBOL(QFS_rfs_do_linux_fbzero);

unsigned int
QFS_rfs_full_name_hash(const unsigned char *name, unsigned int len)
{
	return (full_name_hash(name, len));
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
struct dentry *
QFS_rfs_d_splice_alias(struct inode *inode, struct dentry *dentry)
{
	return (d_splice_alias(inode, dentry));
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
struct dentry *
QFS_rfs_d_splice_alias(struct inode *inode, struct dentry *dentry)
{
	d_add(dentry, inode);
	return (NULL);
}
#endif
EXPORT_SYMBOL(QFS_rfs_d_splice_alias);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 16))
void
QFS_rfs_down(struct semaphore *sem)
{
	down(sem);
}
EXPORT_SYMBOL(QFS_rfs_down);

void
QFS_rfs_up(struct semaphore *sem)
{
	up(sem);
}
EXPORT_SYMBOL(QFS_rfs_up);

void
QFS_rfs_lock_inode(struct inode *li)
{
	down(&li->i_sem);
}

void
QFS_rfs_unlock_inode(struct inode *li)
{
	up(&li->i_sem);
}
#endif /* < 2.6.16 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 16))
void
QFS_rfs_lock_inode(struct inode *li)
{
	mutex_lock(&li->i_mutex);
}

void
QFS_rfs_unlock_inode(struct inode *li)
{
	mutex_unlock(&li->i_mutex);
}
#endif /* >= 2.6.16 */

void
QFS_rfs_down_read(struct rw_semaphore *sem)
{
	down_read(sem);
}
EXPORT_SYMBOL(QFS_rfs_down_read);

void
QFS_rfs_down_write(struct rw_semaphore *sem)
{
	down_write(sem);
}
EXPORT_SYMBOL(QFS_rfs_down_write);

void
QFS_rfs_up_read(struct rw_semaphore *sem)
{
	up_read(sem);
}
EXPORT_SYMBOL(QFS_rfs_up_read);

void
QFS_rfs_up_write(struct rw_semaphore *sem)
{
	/* inline */
	up_write(sem);
}
EXPORT_SYMBOL(QFS_rfs_up_write);

int
QFS_rfs_down_read_trylock(struct rw_semaphore *sem)
{
	return (down_read_trylock(sem));
}
EXPORT_SYMBOL(QFS_rfs_down_read_trylock);

int
QFS_rfs_down_write_trylock(struct rw_semaphore *sem)
{
	return (down_write_trylock(sem));
}
EXPORT_SYMBOL(QFS_rfs_down_write_trylock);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 16)) && \
	(LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18))
void fastcall
QFS___mutex_init(struct mutex *lock, const char *name)
{
	__mutex_init(lock, name);
}
EXPORT_SYMBOL(QFS___mutex_init);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18))
void
QFS___mutex_init(struct mutex *lock, const char *name,
		struct lock_class_key *key)
{
	__mutex_init(lock, name, key);
}
EXPORT_SYMBOL(QFS___mutex_init);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 16)
void fastcall __sched
QFS_mutex_lock(struct mutex *lock)
{
	mutex_lock(lock);
}
EXPORT_SYMBOL(QFS_mutex_lock);

void fastcall __sched
QFS_mutex_unlock(struct mutex *lock)
{
	mutex_unlock(lock);
}
EXPORT_SYMBOL(QFS_mutex_unlock);

#ifdef CONFIG_DEBUG_MUTEXES
void fastcall
QFS_mutex_destroy(struct mutex *lock)
{
	mutex_destroy(lock);
}
EXPORT_SYMBOL(QFS_mutex_destroy);
#endif
#endif /* >= 2.6.16 */

void
QFS_dput(struct dentry *dentry)
{
	dput(dentry);
}

void
QFS_d_rehash(struct dentry *dentry)
{
	d_rehash(dentry);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#if defined(RHE_LINUX) && (LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 9))
int
QFS_rfs_write_inode_now(struct inode *inode, int sync)
{
	return (write_inode_now_err(inode, sync));
}
#else
int
QFS_rfs_write_inode_now(struct inode *inode, int sync)
{
	return (write_inode_now(inode, sync));
}
#endif
EXPORT_SYMBOL(QFS_rfs_write_inode_now);
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
int
QFS_filemap_fdatasync(struct address_space *mapping)
{
	return (filemap_fdatasync(mapping));
}
EXPORT_SYMBOL(QFS_filemap_fdatasync);
#endif

int
QFS_filemap_fdatawait(struct address_space *mapping)
{
	return (filemap_fdatawait(mapping));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0))
struct page *
QFS_filemap_nopage(struct vm_area_struct *area,
		unsigned long address, int *type)
{
	return (filemap_nopage(area, address, type));
}
#else
struct page *
QFS_filemap_nopage(struct vm_area_struct *area,
		unsigned long address, int unused)
{
	return (filemap_nopage(area, address, unused));
}
#endif

int
QFS_filp_close(struct file *filp, fl_owner_t id)
{
	return (filp_close(filp, id));
}

struct file *
QFS_filp_open(const char *filename, int flags, int mode)
{
	return (filp_open(filename, flags, mode));
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 5, 0))
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 9))
int
QFS_fsync_buffers_list(spinlock_t *lock, struct list_head *list)
{
	return (fsync_buffers_list(lock, list));
}
EXPORT_SYMBOL(QFS_fsync_buffers_list);
#endif
#else

int
QFS_fsync_buffers_list(struct list_head *list)
{
	return (fsync_buffers_list(list));
}
EXPORT_SYMBOL(QFS_fsync_buffers_list);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0))
int
QFS_sync_mapping_buffers(struct address_space *m)
{
	return (sync_mapping_buffers(m));
}
EXPORT_SYMBOL(QFS_sync_mapping_buffers);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
struct dentry *
QFS_rfs_d_alloc_anon(struct inode *li)
{
	return (d_alloc_anon(li));
}
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
struct dentry *
QFS_rfs_d_alloc_anon(struct inode *li)
{
	return (rfs_d_alloc_anon(li));
}
#endif
EXPORT_SYMBOL(QFS_rfs_d_alloc_anon);

int
QFS_generic_commit_write(struct file *file, struct page *page,
			unsigned from, unsigned to)
{
	return (generic_commit_write(file, page, from, to));
}

#if defined(__i386) && (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
unsigned long
QFS___generic_copy_from_user(void *a, const void *b, unsigned long c)
{
	return (__generic_copy_from_user(a, b, c));
}
unsigned long
QFS___generic_copy_to_user(void *a, const void *b, unsigned long c)
{
	return (__generic_copy_to_user(a, b, c));
}
EXPORT_SYMBOL(QFS___generic_copy_from_user);
EXPORT_SYMBOL(QFS___generic_copy_to_user);
#endif

#if defined(__i386) && \
	(LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)) && \
	(LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18))
void
QFS___cond_resched(void)
{
	__cond_resched();
}

void
QFS___might_sleep(char *file, int line, int atomic_depth)
{
	__might_sleep(file, line, atomic_depth);

}

unsigned long
QFS___copy_to_user_ll(void __user *to, const void *from,
			unsigned long n)
{
	return (__copy_to_user_ll(to, from, n));

}

unsigned long
QFS___copy_from_user_ll(void *to, const void __user *from,
			unsigned long n)
{
	return (__copy_from_user_ll(to, from, n));
}

EXPORT_SYMBOL(QFS___cond_resched);
EXPORT_SYMBOL(QFS___might_sleep);
EXPORT_SYMBOL(QFS___copy_to_user_ll);
EXPORT_SYMBOL(QFS___copy_from_user_ll);
#endif


int
QFS_memcmp(const void *cs, const void *ct, __kernel_size_t count)
{
	return (memcmp(cs, ct, count));
}
EXPORT_SYMBOL(QFS_memcmp);


#if defined(__x86_64) || (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18))
unsigned long
QFS_copy_from_user(void *to, const void *from, unsigned len)
{
	return (copy_from_user(to, from, len));
}
unsigned long
QFS_copy_to_user(void *to, const void *from, unsigned len)
{
	return (copy_to_user(to, from, len));
}
EXPORT_SYMBOL(QFS_copy_from_user);
EXPORT_SYMBOL(QFS_copy_to_user);
#endif

#ifdef __ia64
unsigned long
QFS___copy_user(void *pdst, const void *psrc, unsigned long pn)
{
	return (__copy_user(pdst, psrc, pn));
}
EXPORT_SYMBOL(QFS___copy_user);
#endif

loff_t
QFS_generic_file_llseek(struct file *file, loff_t offset, int origin)
{
	return (generic_file_llseek(file, offset, origin));
}

int
QFS_generic_file_mmap(struct file *file, struct vm_area_struct *vma)
{
	return (generic_file_mmap(file, vma));
}

ssize_t
QFS_generic_file_read(struct file *file, char *buf, size_t count,
			loff_t *ppos)
{
	return (generic_file_read(file, buf, count, ppos));
}

ssize_t
QFS_rfs_generic_file_write(struct file *file, const char *buf,
			size_t count, loff_t *ppos)
{
	return (rfs_generic_file_write(file, buf, count, ppos));
}

ssize_t
QFS_generic_read_dir(struct file *file, char *buf, size_t count,
			loff_t *ppos)
{
	return (generic_read_dir(file, buf, count, ppos));
}

void
QFS_rfs_make_bad_inode(struct inode *inode)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
	remove_inode_hash(inode);
#endif
	make_bad_inode(inode);
}
EXPORT_SYMBOL(QFS_rfs_make_bad_inode);

int
QFS_is_bad_inode(struct inode *inode)
{
	return (is_bad_inode(inode));
}
EXPORT_SYMBOL(QFS_is_bad_inode);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0))
int
QFS_invalidate_inodes(struct super_block *sb)
{
	return (invalidate_inodes(sb));
}
EXPORT_SYMBOL(QFS_invalidate_inodes);

struct inode *
QFS_iget5_locked(struct super_block *sb, unsigned long hashval,
		int (*test)(struct inode *, void *),
		int (*set)(struct inode *, void *), void *data)
{
	return (iget5_locked(sb, hashval, test, set, data));
}
EXPORT_SYMBOL(QFS_iget5_locked);

struct inode *
QFS_ilookup5(struct super_block *lsb, unsigned long hv,
		int (*test)(struct inode *, void *), void *data)
{
	return (ilookup5(lsb, hv, test, data));
}
EXPORT_SYMBOL(QFS_ilookup5);

struct super_block *
QFS_sget(struct file_system_type *fst,
	int (*cmp)(struct super_block *, void *),
	int (*fill)(struct super_block *, void *), void *rd)
{
	return (sget(fst, cmp, fill, rd));
}
EXPORT_SYMBOL(QFS_sget);

void
QFS_deactivate_super(struct super_block *lsb)
{
	deactivate_super(lsb);
}
EXPORT_SYMBOL(QFS_deactivate_super);

void
QFS_generic_shutdown_super(struct super_block *lsb)
{
	return (generic_shutdown_super(lsb));
}
EXPORT_SYMBOL(QFS_generic_shutdown_super);

int
QFS_set_anon_super(struct super_block *lsb, void *rdata)
{
	return (set_anon_super(lsb, rdata));
}
EXPORT_SYMBOL(QFS_set_anon_super);

void
QFS_kill_anon_super(struct super_block *lsb)
{
	kill_anon_super(lsb);
}
EXPORT_SYMBOL(QFS_kill_anon_super);

struct block_device *
QFS_open_bdev_excl(const char *path, int flags, void *holder)
{
	return (open_bdev_excl(path, flags, holder));
}
EXPORT_SYMBOL(QFS_open_bdev_excl);

#endif /* 2.6 */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0))
struct inode *
QFS_iget4_locked(struct super_block *sb, unsigned long ino,
		find_inode_t find_actor, void *opaque)
{
	return (iget4_locked(sb, ino, find_actor, opaque));
}
EXPORT_SYMBOL(QFS_iget4_locked);
#endif /* 2.4 */

int
QFS_in_group_p(gid_t id)
{
	return (in_group_p(id));
}

void FASTCALL(QFS_init_rwsem(struct rw_semaphore *sem));

void
QFS_init_rwsem(struct rw_semaphore *sem)
{
	init_rwsem(sem);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18))
void
QFS___init_rwsem(struct rw_semaphore *sem, const char *name,
	struct lock_class_key *key)
{
	__init_rwsem(sem, name, key);
}
EXPORT_SYMBOL(QFS___init_rwsem);
#endif

void
QFS_init_special_inode(struct inode *inode, umode_t mode, int rdev)
{
	init_special_inode(inode, mode, rdev);
}

void
QFS_truncate_inode_pages(struct address_space *mapping, loff_t offset)
{
	truncate_inode_pages(mapping, offset);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0))
unsigned long
QFS_invalidate_inode_pages(struct address_space *m)
{
	return (invalidate_inode_pages(m));
}
#else
void
QFS_invalidate_inode_pages(struct inode *inode)
{
	invalidate_inode_pages(inode);
}
#endif

void
QFS_iput(struct inode *inode)
{
	iput(inode);
}

long
QFS_kernel_thread(int (*fn)(void*), void *arg, unsigned long flags)
{
	return (kernel_thread(fn, arg, flags));
}

void
QFS_kfree(const void *ptr)
{
	kfree(ptr);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 17))
void
QFS___kfree_skb(struct sk_buff *skb)
{
	__kfree_skb(skb);
}
EXPORT_SYMBOL(QFS___kfree_skb);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 17))
void
QFS_kfree_skb(struct sk_buff *skb)
{
	kfree_skb(skb);
}
EXPORT_SYMBOL(QFS_kfree_skb);
#endif

void *
QFS_kmalloc(size_t size, int flags)
{
	void* ptr = NULL;
	ptr = kmalloc(size, flags);
/*	printk(KERN_CRIT "QFS_kmalloc %d %d %p\n", size, flags, ptr); */

	return (ptr);
}

void *
QFS_kmem_cache_alloc(kmem_cache_t *cachep, int flags)
{
	return (kmem_cache_alloc(cachep, flags));
}

kmem_cache_t *
QFS_kmem_cache_create(const char *name, size_t size,
			size_t offset,
			unsigned long flags,
			void (*ctor)(void *, kmem_cache_t *, unsigned long),
			void (*dtor)(void *, kmem_cache_t *, unsigned long))
{
	return (kmem_cache_create(name, size, offset, flags, ctor, dtor));
}

int
QFS_kmem_cache_destroy(kmem_cache_t *cachep)
{
	return (kmem_cache_destroy(cachep));
}

void
QFS_kmem_cache_free(kmem_cache_t *cachep, void *objp)
{
	kmem_cache_free(cachep, objp);
}

void *
QFS_memcpy(void *to, const void *from, __kernel_size_t len)
{
	return (memcpy(to, from, len));
}

#ifdef __x86_64
void *
QFS___memcpy(void *to, const void *from, __kernel_size_t len)
{
	return (__memcpy(to, from, len));
}
EXPORT_SYMBOL(QFS___memcpy);
#endif

void *
QFS_memset(void *ptr, int value, __kernel_size_t len)
{
	return (memset(ptr, value, len));
}
EXPORT_SYMBOL(QFS_memset);

unsigned int
QFS_nr_free_pages(void)
{
	return (nr_free_pages());
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0))
void
QFS___out_of_line_bug(int line)
{
	__out_of_line_bug(line);
}
EXPORT_SYMBOL(QFS___out_of_line_bug);
#endif

void
QFS_panic(char *fmt, ...)
{
	static char buf[1024];
	static char buf1[1024]; /* printk limits to 1024 */
	int i, j, k;
	va_list args;

	memset(buf, 0, sizeof (buf));
	va_start(args, fmt);
	i = vsnprintf(buf, sizeof (buf), fmt, args);
	va_end(args);

	/* this is to handle the case of the message having %'s in the string */
	i = strlen(buf);
	memset(buf1, 0, sizeof (buf1));
	for (j = 0, k = 0; (j < i) && (k < (sizeof (buf1)-1)); j++, k++) {
		buf1[k] = buf[j];
		if (buf[j] == '%') {
			buf1[++k] = '%';
		}
	}
	buf1[k] = 0;
	panic(buf1);
}

void
QFS_path_release(struct nameidata *nd)
{
	path_release(nd);
}

int
QFS_printk(char *fmt, ...)
{
	static char buf[1024];
	static char buf1[1024]; /* printk limits to 1024 */
	int i, j, k;
	va_list args;

	/*	spin_lock(&QFS_printk_lock); */
	memset(buf, 0, sizeof (buf));
	va_start(args, fmt);
	i = vsnprintf(buf, sizeof (buf), fmt, args);
	va_end(args);

	/* this is to handle the case of the message having %'s in the string */
	i = strlen(buf);
	memset(buf1, 0, sizeof (buf1));
	for (j = 0, k = 0; (j < i) && (k < (sizeof (buf1)-1)); j++, k++) {
		buf1[k] = buf[j];
		if (buf[j] == '%') {
			buf1[++k] = '%';
		}
	}
	buf1[k] = 0;
	i = printk(buf1);
	/*	spin_unlock(&QFS_printk_lock); */
	return (i);
}

struct proc_dir_entry *
QFS_proc_mkdir(const char *name, struct proc_dir_entry *parent)
{
	return (proc_mkdir(name, parent));
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0))
struct proc_dir_entry *
QFS_proc_mknod(const char *name, mode_t mode,
		struct proc_dir_entry *parent, kdev_t rdev)
{
	return (proc_mknod(name, mode, parent, rdev));
}
EXPORT_SYMBOL(QFS_proc_mknod);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
int
QFS_redirty_page_for_writepage(struct writeback_control *wbc,
				struct page *page)
{
	return (redirty_page_for_writepage(wbc, page));
}
EXPORT_SYMBOL(QFS_redirty_page_for_writepage);
#endif

int
QFS_register_chrdev(unsigned int major, const char *name,
		    struct file_operations *fops)
{
	int md;
	md = register_chrdev(major, name, fops);
	return (md);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0))
int
QFS_register_blkdev(unsigned int major, const char *name)
{
	int md;
	md = register_blkdev(major, name);
	return (md);
}
#else
int
QFS_register_blkdev(unsigned int major, const char *name,
		    struct block_device_operations *bdops)
{
	int md;
	md = register_blkdev(major, name, bdops);
	return (md);
}
#endif /* KERNEL_MAJOR */
EXPORT_SYMBOL(QFS_register_blkdev);

#if defined(SUSE_LINUX) && (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 5))
int
QFS___register_filesystem(struct file_system_type *fs, int lifo)
{
	return (__register_filesystem(fs, lifo));
}
EXPORT_SYMBOL(QFS___register_filesystem);
#else
int
QFS_register_filesystem(struct file_system_type *fs)
{
	return (register_filesystem(fs));
}
EXPORT_SYMBOL(QFS_register_filesystem);
#endif

void
QFS_remove_proc_entry(const char *name, struct proc_dir_entry *parent)
{
	remove_proc_entry(name, parent);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18))
void
QFS_init_waitqueue_head(wait_queue_head_t *q)
{
	init_waitqueue_head(q);
}
EXPORT_SYMBOL(QFS_init_waitqueue_head);
#endif

void FASTCALL(QFS_remove_wait_queue(wait_queue_head_t *q, wait_queue_t *wait));

void
QFS_remove_wait_queue(wait_queue_head_t *q, wait_queue_t *wait)
{
	remove_wait_queue(q, wait);
}

void
QFS_rfs_d_drop(struct dentry *dentry)
{
	/* inline */
	d_drop(dentry);
}

int
QFS_rfs_block_read_full_page(struct page *page, get_block_t *get_block)
{
	return (rfs_block_read_full_page(page, get_block));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0))
int
QFS_rfs_suser(void)
{
	return (capable(CAP_SYS_ADMIN));
}
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0))
int
QFS_rfs_suser(void)
{
	return (suser());
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0))
void
QFS_rfs_downgrade_write(struct rw_semaphore *sem)
{
	downgrade_write(sem);
}
EXPORT_SYMBOL(QFS_rfs_downgrade_write);
#endif /* KERNEL_MAJOR */

void
QFS_schedule(void)
{
	schedule();
}

signed long FASTCALL(QFS_schedule_timeout(signed long timeout));

signed long
QFS_schedule_timeout(signed long timeout)
{
	return (schedule_timeout(timeout));
}

int
QFS_send_sig(int a, struct task_struct *b, int c)
{
	return (send_sig(a, b, c));
}

void
QFS_skb_over_panic(struct sk_buff *skb, int len, void *here)
{
	skb_over_panic(skb, len, here);
}

int
QFS_snprintf(char *buf, size_t length, const char *fmt, ...)
{
	int	err;
	va_list args;

	va_start(args, fmt);
	err = vsnprintf(buf, length, fmt, args);
	va_end(args);

	return (err);
}

int
QFS_sock_recvmsg(struct socket *sock, struct msghdr *msg, int size,
		int flags)
{
	return (sock_recvmsg(sock, msg, size, flags));
}

int
QFS_sock_sendmsg(struct socket *sock, struct msghdr *msg, int len)
{
	return (sock_sendmsg(sock, msg, len));
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 8))
void
QFS__spin_lock(spinlock_t *lock)
{
	_spin_lock(lock);
}

void
QFS__spin_lock_irq(spinlock_t *lock)
{
	_spin_lock_irq(lock);
}

void
QFS__spin_unlock(spinlock_t *lock)
{
	_spin_unlock(lock);
}

void
QFS__spin_unlock_irq(spinlock_t *lock)
{
	_spin_unlock_irq(lock);
}
EXPORT_SYMBOL(QFS__spin_lock);
EXPORT_SYMBOL(QFS__spin_lock_irq);
EXPORT_SYMBOL(QFS__spin_unlock);
EXPORT_SYMBOL(QFS__spin_unlock_irq);
#endif

int
QFS_sprintf(char *buf, const char *fmt, ...)
{
	int	err;
	va_list args;

	va_start(args, fmt);
	err = vsprintf(buf, fmt, args);
	va_end(args);

	return (err);
}

#ifndef _i386
char *
QFS_strcat(char *dest, const char *src)
{
	return (strcat(dest, src));
}

char *
QFS_strchr(const char *s, int c)
{
	return (strchr(s, c));
}

int
QFS_strcmp(const char *cs, const char *ct)
{
	return (strcmp(cs, ct));
}

char *
QFS_strcpy(char *dest, const char *src)
{
	return (strcpy(dest, src));
}

__kernel_size_t
QFS_strlen(const char *str)
{
	return (strlen(str));
}

int
QFS_strncmp(const char *cs, const char *ct, size_t count)
{
	return (strncmp(cs, ct, count));
}

char *
QFS_strncpy(char *dest, const char *src, __kernel_size_t count)
{
	return (strncpy(dest, src, count));
}
EXPORT_SYMBOL(QFS_strcat);
EXPORT_SYMBOL(QFS_strchr);
EXPORT_SYMBOL(QFS_strcmp);
EXPORT_SYMBOL(QFS_strcpy);
EXPORT_SYMBOL(QFS_strlen);
EXPORT_SYMBOL(QFS_strncmp);
EXPORT_SYMBOL(QFS_strncpy);
#endif

char *
QFS_strstr(const char *cs, const char *ct)
{
	return (strstr(cs, ct));
}


void
QFS_unlock_new_inode(struct inode *inode)
{
	unlock_new_inode(inode);
}
EXPORT_SYMBOL(QFS_unlock_new_inode);

void FASTCALL(QFS_unlock_page(struct page *page));

void
QFS_unlock_page(struct page *page)
{
	unlock_page(page);
}

int
QFS_unregister_chrdev(unsigned int major, char *name)
{
	return (unregister_chrdev(major, name));
}

int
QFS_unregister_blkdev(unsigned int major, char *name)
{
	return (unregister_blkdev(major, name));
}

int
QFS_unregister_filesystem(struct file_system_type *fs)
{
	return (unregister_filesystem(fs));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 16))
int fastcall
QFS___user_walk_fd(int dfd, const char __user *name,
		unsigned flags, struct nameidata *nd)
{
	return (__user_walk_fd(dfd, name, flags, nd));
}
EXPORT_SYMBOL(QFS___user_walk_fd);
#endif

#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)) && \
	(LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 9)))
int FASTCALL(QFS___user_walk(const char *a, unsigned b, struct nameidata *c,
				const char **d));

int
QFS___user_walk(const char *a, unsigned b, struct nameidata *c,
		const char **d)
{
	return (__user_walk(a, b, c, d));
}
EXPORT_SYMBOL(QFS___user_walk);
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)) || \
	(defined(RHE_LINUX) && \
	(LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 9)))
int FASTCALL(QFS___user_walk(const char *a, unsigned b, struct nameidata *c));

int
QFS___user_walk(const char *a, unsigned b, struct nameidata *c)
{
	return (__user_walk(a, b, c));
}
EXPORT_SYMBOL(QFS___user_walk);
#endif

void
QFS_vfree(void *address)
{
	vfree(address);
}

int
QFS_vfs_follow_link(struct nameidata *nd, const char *link)
{
	return (vfs_follow_link(nd, link));
}

void *
QFS_vmalloc(unsigned long size)
{
	return (vmalloc(size));
}

int
QFS_vmtruncate(struct inode *inode, loff_t offset)
{
	return (vmtruncate(inode, offset));
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 8))
void FASTCALL(QFS___wake_up(wait_queue_head_t *q, unsigned int mode, int nr,
			    void *key));

void
QFS___wake_up(wait_queue_head_t *q, unsigned int mode, int nr, void *key)
{
	__wake_up(q, mode, nr, key);
}
#else
void FASTCALL(QFS___wake_up(wait_queue_head_t *q, unsigned int mode, int nr));

void
QFS___wake_up(wait_queue_head_t *q, unsigned int mode, int nr)
{
	__wake_up(q, mode, nr);
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
struct socket *
QFS_rfs_sockfd_lookup(int fd, int *err)
{
	return (sockfd_lookup(fd, err));
}

void
QFS_rfs_sockfd_put(struct socket *sock)
{
	sockfd_put(sock);
}
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
struct socket *
QFS_rfs_sockfd_lookup(int fd, int *err)
{
	return (rfs_sockfd_lookup(fd, err));
}

void
QFS_rfs_sockfd_put(struct socket *sock)
{
	fput((sock)->file);
}
#endif

#ifdef __ia64
int64_t __divdi3(int64_t u, int64_t v);
int64_t __moddi3(int64_t u, int64_t v);
int32_t __divsi3(int32_t, int32_t);
u_int32_t __udivsi3(u_int32_t, u_int32_t);
int32_t __modsi3(int32_t, int32_t);
u_int32_t __umodsi3(u_int32_t, u_int32_t);
#endif


#ifndef __x86_64
int64_t
QFS___divdi3(int64_t u, int64_t v)
{
	return (__divdi3(u, v));
}

int64_t
QFS___moddi3(int64_t u, int64_t v)
{
	return (__moddi3(u, v));
}

EXPORT_SYMBOL(QFS___divdi3);
EXPORT_SYMBOL(QFS___moddi3);
#endif

#ifdef __ia64
int32_t
QFS___divsi3(int32_t u, int32_t v)
{
	return (__divsi3(u, v));
}
u_int32_t
QFS___udivsi3(u_int32_t u, u_int32_t v)
{
	return (__udivsi3(u, v));
}
int32_t
QFS___modsi3(int32_t u, int32_t v)
{
	return (__modsi3(u, v));
}
u_int32_t
QFS___umodsi3(u_int32_t u, u_int32_t v)
{
	return (__umodsi3(u, v));
}
EXPORT_SYMBOL(QFS___divsi3);
EXPORT_SYMBOL(QFS___udivsi3);
EXPORT_SYMBOL(QFS___modsi3);
EXPORT_SYMBOL(QFS___umodsi3);
#endif

EXPORT_SYMBOL(QFS_add_wait_queue_exclusive);
EXPORT_SYMBOL(QFS_add_wait_queue);
EXPORT_SYMBOL(QFS_rfs_alloc_skb);
EXPORT_SYMBOL(QFS_block_prepare_write);
EXPORT_SYMBOL(QFS_rfs_block_read_full_page);
EXPORT_SYMBOL(QFS_block_write_full_page);
EXPORT_SYMBOL(QFS_bread);
EXPORT_SYMBOL(QFS___brelse);
EXPORT_SYMBOL(QFS_clear_inode);
EXPORT_SYMBOL(QFS_create_proc_entry);
EXPORT_SYMBOL(QFS_d_alloc_root);
EXPORT_SYMBOL(QFS_d_delete);
EXPORT_SYMBOL(QFS_d_find_alias);
EXPORT_SYMBOL(QFS_rfs_full_name_hash);
EXPORT_SYMBOL(QFS_d_instantiate);
EXPORT_SYMBOL(QFS_d_lookup);
EXPORT_SYMBOL(QFS_do_gettimeofday);
EXPORT_SYMBOL(QFS_dput);
EXPORT_SYMBOL(QFS_d_rehash);
EXPORT_SYMBOL(QFS_filemap_fdatawait);
EXPORT_SYMBOL(QFS_filemap_nopage);
EXPORT_SYMBOL(QFS_filp_close);
EXPORT_SYMBOL(QFS_filp_open);
EXPORT_SYMBOL(QFS_generic_commit_write);
EXPORT_SYMBOL(QFS_generic_file_llseek);
EXPORT_SYMBOL(QFS_generic_file_mmap);
EXPORT_SYMBOL(QFS_generic_file_read);
EXPORT_SYMBOL(QFS_rfs_generic_file_write);
EXPORT_SYMBOL(QFS_generic_read_dir);
EXPORT_SYMBOL(QFS_in_group_p);
EXPORT_SYMBOL(QFS_init_rwsem);
EXPORT_SYMBOL(QFS_init_special_inode);
EXPORT_SYMBOL(QFS_invalidate_inode_pages);
EXPORT_SYMBOL(QFS_truncate_inode_pages);
EXPORT_SYMBOL(QFS_iput);
EXPORT_SYMBOL(QFS_kernel_thread);
EXPORT_SYMBOL(QFS_kfree);
EXPORT_SYMBOL(QFS_kmalloc);
EXPORT_SYMBOL(QFS_kmem_cache_alloc);
EXPORT_SYMBOL(QFS_kmem_cache_create);
EXPORT_SYMBOL(QFS_kmem_cache_destroy);
EXPORT_SYMBOL(QFS_kmem_cache_free);
EXPORT_SYMBOL(QFS_rfs_lock_inode);
EXPORT_SYMBOL(QFS_rfs_unlock_inode);
EXPORT_SYMBOL(QFS_memcpy);
EXPORT_SYMBOL(QFS_nr_free_pages);
EXPORT_SYMBOL(QFS_panic);
EXPORT_SYMBOL(QFS_path_release);
EXPORT_SYMBOL(QFS_printk);
EXPORT_SYMBOL(QFS_proc_mkdir);
EXPORT_SYMBOL(QFS_register_chrdev);
EXPORT_SYMBOL(QFS_remove_proc_entry);
EXPORT_SYMBOL(QFS_remove_wait_queue);
EXPORT_SYMBOL(QFS_schedule);
EXPORT_SYMBOL(QFS_schedule_timeout);
EXPORT_SYMBOL(QFS_send_sig);
EXPORT_SYMBOL(QFS_skb_over_panic);
EXPORT_SYMBOL(QFS_snprintf);
EXPORT_SYMBOL(QFS_sock_recvmsg);
EXPORT_SYMBOL(QFS_sock_sendmsg);
EXPORT_SYMBOL(QFS_sprintf);
EXPORT_SYMBOL(QFS_strstr);
EXPORT_SYMBOL(QFS_rfs_d_drop);
EXPORT_SYMBOL(QFS_rfs_sockfd_lookup);
EXPORT_SYMBOL(QFS_rfs_sockfd_put);
EXPORT_SYMBOL(QFS_rfs_suser);
EXPORT_SYMBOL(QFS_unlock_page);
EXPORT_SYMBOL(QFS_unregister_chrdev);
EXPORT_SYMBOL(QFS_unregister_blkdev);
EXPORT_SYMBOL(QFS_unregister_filesystem);
EXPORT_SYMBOL(QFS_vfree);
EXPORT_SYMBOL(QFS_vfs_follow_link);
EXPORT_SYMBOL(QFS_vmalloc);
EXPORT_SYMBOL(QFS_vmtruncate);
EXPORT_SYMBOL(QFS___wake_up);


int FASTCALL(QFS_wake_up_process(struct task_struct *tsk));

int
QFS_wake_up_process(struct task_struct *tsk)
{
	return (wake_up_process(tsk));
}
EXPORT_SYMBOL(QFS_wake_up_process);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
void
QFS_generic_fillattr(struct inode *i, struct kstat *k)
{
	generic_fillattr(i, k);
}
EXPORT_SYMBOL(QFS_generic_fillattr);

struct block_device *
QFS_bdget(dev_t d)
{
	return (bdget(d));
}
EXPORT_SYMBOL(QFS_bdget);

void FASTCALL(QFS_finish_wait(wait_queue_head_t *q, wait_queue_t *wait));

void
QFS_finish_wait(wait_queue_head_t *q, wait_queue_t *wait)
{
	finish_wait(q, wait);
}
EXPORT_SYMBOL(QFS_finish_wait);

void FASTCALL(QFS_prepare_to_wait(wait_queue_head_t *q,
				wait_queue_t *wait, int state));

void
QFS_prepare_to_wait(wait_queue_head_t *q, wait_queue_t *wait, int state)
{
	prepare_to_wait(q, wait, state);
}
EXPORT_SYMBOL(QFS_prepare_to_wait);

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 8))
int
QFS_default_wake_function(wait_queue_t *wait, unsigned mode, int sync,
			void *key)
{
	return (default_wake_function(wait, mode, sync, key));
}
EXPORT_SYMBOL(QFS_default_wake_function);

int
QFS_autoremove_wake_function(wait_queue_t *wait, unsigned mode, int sync,
				void *key)
{
	return (autoremove_wake_function(wait, mode, sync, key));
}
EXPORT_SYMBOL(QFS_autoremove_wake_function);

#else
int
QFS_default_wake_function(wait_queue_t *wait, unsigned mode, int sync)
{
	return (default_wake_function(wait, mode, sync));
}
EXPORT_SYMBOL(QFS_default_wake_function);

int
QFS_autoremove_wake_function(wait_queue_t *wait, unsigned mode, int sync)
{
	return (autoremove_wake_function(wait, mode, sync));
}
EXPORT_SYMBOL(QFS_autoremove_wake_function);
#endif

struct proc_dir_entry *
QFS_proc_symlink(const char *name,
    struct proc_dir_entry *parent, char *dest)
{
	return (proc_symlink(name, parent, dest));
}
EXPORT_SYMBOL(QFS_proc_symlink);

void
QFS_close_bdev_excl(struct block_device *d)
{
	close_bdev_excl(d);
}
EXPORT_SYMBOL(QFS_close_bdev_excl);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
int
QFS_rfs_try_module_get(struct module *module)
{
	return (try_module_get(module));
}
EXPORT_SYMBOL(QFS_rfs_try_module_get);

void
QFS_rfs_module_put(struct module *module)
{
	module_put(module);
}
EXPORT_SYMBOL(QFS_rfs_module_put);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
loff_t
QFS_rfs_i_size_read(struct inode *inode)
{
	/* inline */
	return (i_size_read(inode));
}

void
QFS_rfs_i_size_write(struct inode *inode, loff_t i_size)
{
	/* inline */
	i_size_write(inode, i_size);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
loff_t
QFS_rfs_i_size_read(struct inode *inode)
{
	return (inode->i_size);
}

void
QFS_rfs_i_size_write(struct inode *inode, loff_t i_size)
{
	inode->i_size = i_size;
}
#endif /* LINUX_VERSION_CODE < 2.6.0 */

EXPORT_SYMBOL(QFS_rfs_i_size_read);
EXPORT_SYMBOL(QFS_rfs_i_size_write);

void
QFS_wait_event(wait_queue_head_t wq, void *condition)
{
	wait_event(wq, condition);
}
EXPORT_SYMBOL(QFS_wait_event);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 9))
int
QFS_posix_lock_file_wait(struct file *filp, struct file_lock *fl)
{
	return (posix_lock_file_wait(filp, fl));
}
EXPORT_SYMBOL(QFS_posix_lock_file_wait);
int
QFS_flock_lock_file_wait(struct file *filp, struct file_lock *fl)
{
	return (flock_lock_file_wait(filp, fl));
}
EXPORT_SYMBOL(QFS_flock_lock_file_wait);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
ssize_t
QFS_generic_file_write_nolock(struct file *file, const struct iovec *iov,
				unsigned long nr_segs, loff_t *ppos)
{
	return (generic_file_write_nolock(file, iov, nr_segs, ppos));
}
EXPORT_SYMBOL(QFS_generic_file_write_nolock);
#endif


int __init
qfs_ki_init(void)
{
	spin_lock_init(&QFS_printk_lock);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0))
	QFS_smp_num_cpus = smp_num_cpus;
#else
	QFS_smp_num_cpus = num_online_cpus();
#endif
	QFS___HZ = HZ;
	return (0);
}

void __exit
qfs_ki_exit(void)
{
}


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18))
int
QFS_simple_set_mnt(struct vfsmount *mnt, struct super_block *sb)
{
	return (simple_set_mnt(mnt, sb));
}

EXPORT_SYMBOL(QFS_simple_set_mnt);

void
QFS_list_del(struct list_head *entry)
{
	list_del(entry);
}
EXPORT_SYMBOL(QFS_list_del);

void
QFS___list_add(struct list_head *new, struct list_head *prev,
	struct list_head *next)
{
	__list_add(new, prev, next);
}
EXPORT_SYMBOL(QFS___list_add);
#endif

module_init(qfs_ki_init);
module_exit(qfs_ki_exit);
MODULE_LICENSE("GPL");
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0))
MODULE_INFO(supported, "external");
#endif
