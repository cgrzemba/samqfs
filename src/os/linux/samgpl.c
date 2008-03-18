/*
 *  GPL Notice
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 only.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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

#ifdef sun
#pragma ident "$Revision: 1.24 $"
#endif

#define	EXPORT_SYMTAB

#include <linux/module.h>
#include <linux/mm.h>
#include <linux/smp_lock.h>
#include <linux/socket.h>
#include <linux/file.h>
#include <linux/net.h>
#include <linux/interrupt.h>
#include <linux/netdevice.h>
#include <linux/proc_fs.h>
#include <linux/wanrouter.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/cache.h>
#include <linux/highmem.h>
#include <linux/version.h>

#if defined(CONFIG_KMOD) && defined(CONFIG_NET)
#include <linux/kmod.h>
#endif

#include <asm/system.h>
#include <asm/uaccess.h>

#include <net/sock.h>
#include <net/scm.h>
#include <linux/netfilter.h>

#if (!defined(_LP64) && !defined(__ia64))
#include <bits/wordsize.h>
#if __WORDSIZE == 32
#include "longlong.h"
extern DWtype __divdi3(DWtype u, DWtype v);
extern DWtype __moddi3(DWtype u, DWtype v);
EXPORT_SYMBOL(__divdi3);
EXPORT_SYMBOL(__moddi3);
#endif
#endif

struct tm;
extern int rfs_block_read_full_page(struct page *page, get_block_t *get_block);

EXPORT_SYMBOL(rfs_block_read_full_page);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
struct dentry *rfs_d_alloc_anon(struct inode *);
EXPORT_SYMBOL(rfs_d_alloc_anon);
struct socket *rfs_sockfd_lookup(int fd, int *err);
EXPORT_SYMBOL(rfs_sockfd_lookup);
#if defined(RHE_LINUX)
extern int rfs_generic_direct_sector_IO(int rw, struct inode *inode,
				struct kiobuf *iobuf,
				unsigned long blocknr, int blocksize,
				int sectsize, int s_offset,
				get_block_t *get_block, int drop_locks);
EXPORT_SYMBOL(rfs_generic_direct_sector_IO);
#else
extern int rfs_generic_direct_IO(int rw, struct inode *inode,
				struct kiobuf *iobuf, unsigned long blocknr,
				int blocksize,
				get_block_t *get_block);
EXPORT_SYMBOL(rfs_generic_direct_IO);
#endif /* RHE_LINUX */
#endif /* KERNEL_MAJOR */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
extern int rfs_filemap_write_and_wait(struct address_space *mapping);
EXPORT_SYMBOL(rfs_filemap_write_and_wait);
extern ssize_t rfs_generic_file_write(struct file *file, const char *buf,
					size_t count, loff_t *ppos);
EXPORT_SYMBOL(rfs_generic_file_write);
extern int rfs_do_linux_fbzero(struct inode *, loff_t, unsigned,
				struct block_device *, int, int);
#else
extern ssize_t rfs_generic_file_write(struct file *file, const char *buf,
					size_t count, loff_t *ppos);
EXPORT_SYMBOL(rfs_generic_file_write);
extern int rfs_do_linux_fbzero(struct inode *, loff_t, unsigned, kdev_t,
				int, int);
#endif /* KERNEL_MAJOR */

EXPORT_SYMBOL(rfs_do_linux_fbzero);

int __init
samgpl_init(void)
{
	return (0);
}

void __exit
samgpl_exit(void)
{
}

module_init(samgpl_init);
module_exit(samgpl_exit);
MODULE_LICENSE("GPL");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
MODULE_INFO(supported, "external");
#endif
