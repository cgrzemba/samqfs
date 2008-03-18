/*
 * ----- samsys_linux.c - Implement the Linux ioctl interface
 *
 * Implement the ioctl interface for the SAM-QFS system calls.
 *
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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
#pragma ident "$Revision: 1.17 $"
#endif

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/version.h>

#include <asm/system.h>
#include <asm/uaccess.h>

#include "sam/samioc.h"
#include "sam/types.h"
#include "kstats_linux.h"
#include "macros_linux.h"

extern int sam_syscall(int cmd, void *arg, int size, int *rvp);
extern struct proc_dir_entry *proc_fs_samqfs;

extern struct file_system_type samfs_fs_type;
static struct file_operations samsys_fops;

/*
 * Major device number of /proc/fs/samfds/samsys
 */
static int samsys_major;

static int
samsys_ioctl(struct inode *i, struct file *f, unsigned int cmd,
    unsigned long arg)
{
	int rval, ret = 0;
	struct sam_syscall_args scargs;

	if (copy_from_user((char *)&scargs, (char *)arg, sizeof (scargs))) {
		return (-EFAULT);
	}

	switch (cmd) {

	case SAMSYS_IOC_NULL:
		/*
		 * Doesn't do anything
		 */
		break;

	case SAMSYS_IOC_SAM_SYSCALL:
		/*
		 * Do a sam system call
		 */
		rval = 0;
		ret = sam_syscall(scargs.cmd, scargs.buf, scargs.size, &rval);
		break;

	default:
		ret = EINVAL;
	}

	/*
	 * Linux error returns are -errno
	 */
	return (-ret);
}

static ssize_t
samsys_read(struct file *f, char *buf, size_t len, loff_t *offset)
{
	ssize_t ret = -EOPNOTSUPP;

	return (ret);
}

static ssize_t
samsys_write(struct file *f, const char *buf, size_t len, loff_t *offset)
{
	ssize_t ret = -EOPNOTSUPP;

	return (ret);
}

static int
samsys_read_major(char *page, char **start, off_t off, int count, int *eof,
    void *data)
{
	long major;
	long *lptr;

	major = (long)data;
	lptr = (long *)page;
	*lptr = major;
	return (sizeof (major));
}


static unsigned long long
kstat_get(kstat_named_t *ks)
{
	if (ks->data_type == KSTAT_DATA_ULONG) {
		return ((unsigned long long)ks->value.ul);
	}
	if (ks->data_type == KSTAT_DATA_UINT64) {
		return ((unsigned long long)ks->value.ui64);
	}
	return ((unsigned long long)-1);
}

static int
samsys_read_kstats(char *page, char **start, off_t off, int count, int *eof,
    void *data)
{
	int len = 0;

#define	put_stat(ptr, len, count, module, cat, stat)	\
		snprintf(ptr+len, count-len, "%s:0:%s:%s %llu\n", \
			(module), #cat, #stat, \
			kstat_get(&sam_##cat##_stats.stat))


	len += put_stat(page, len, count, "sam-qfs", sam, stage_start);
	len += put_stat(page, len, count, "sam-qfs", sam, stage_partial);
	len += put_stat(page, len, count, "sam-qfs", sam, stage_window);
	len += put_stat(page, len, count, "sam-qfs", sam, stage_errors);
	len += put_stat(page, len, count, "sam-qfs", sam, stage_reissue);
	len += put_stat(page, len, count, "sam-qfs", sam, stage_cancel);

	len += put_stat(page, len, count, "sam-qfs", shared_client, msg_in);
	len += put_stat(page, len, count, "sam-qfs", shared_client, msg_out);
	len += put_stat(page, len, count, "sam-qfs", shared_client, sync_pages);
	len += put_stat(page, len, count, "sam-qfs", shared_client,
	    inval_pages);
	len += put_stat(page, len, count, "sam-qfs", shared_client,
	    stale_indir);
	len += put_stat(page, len, count, "sam-qfs", shared_client, dio_switch);
	len += put_stat(page, len, count, "sam-qfs", shared_client, abr_switch);
	len += put_stat(page, len, count, "sam-qfs", shared_client,
	    expire_task);
	len += put_stat(page, len, count, "sam-qfs", shared_client,
	    expire_task_dup);
	len += put_stat(page, len, count, "sam-qfs", shared_client,
	    retransmit_msg);
	len += put_stat(page, len, count, "sam-qfs", shared_client, notify_ino);
	len += put_stat(page, len, count, "sam-qfs", shared_client,
	    notify_expire);
	len += put_stat(page, len, count, "sam-qfs", shared_client,
	    expired_inuse);

	len += put_stat(page, len, count, "sam-qfs", thread, taskq_add);
	len += put_stat(page, len, count, "sam-qfs", thread, taskq_add_dup);
	len += put_stat(page, len, count, "sam-qfs", thread, taskq_dispatch);
	len += put_stat(page, len, count, "sam-qfs", thread, max_share_threads);

#ifdef DEBUG
	len += put_stat(page, len, count, "sam-qfs", debug, nreads);
	len += put_stat(page, len, count, "sam-qfs", debug, nwrites);
	len += put_stat(page, len, count, "sam-qfs", debug, statfs_thread);
#endif

	ASSERT(len <= (count-1));
	*eof = 1;
	return (len);
}


#ifdef DEBUG
static void
kstat_reset(kstat_named_t *ks)
{
	if (ks->data_type == KSTAT_DATA_ULONG) {
		ks->value.ul = 0;
	}
	if (ks->data_type == KSTAT_DATA_UINT64) {
		ks->value.ui64 = 0;
	}
}

static int
samsys_reset_kstats(struct file *file, const char *buffer,
    unsigned long count, void *data)
{

#define	reset_stat(cat, stat)	kstat_reset(&sam_##cat##_stats.stat)

	reset_stat(sam, stage_start);
	reset_stat(sam, stage_partial);
	reset_stat(sam, stage_window);
	reset_stat(sam, stage_errors);
	reset_stat(sam, stage_reissue);
	reset_stat(sam, stage_cancel);

	reset_stat(shared_client, msg_in);
	reset_stat(shared_client, msg_out);
	reset_stat(shared_client, sync_pages);
	reset_stat(shared_client, inval_pages);
	reset_stat(shared_client, stale_indir);
	reset_stat(shared_client, dio_switch);
	reset_stat(shared_client, abr_switch);
	reset_stat(shared_client, expire_task);
	reset_stat(shared_client, expire_task_dup);
	reset_stat(shared_client, retransmit_msg);
	reset_stat(shared_client, notify_ino);
	reset_stat(shared_client, notify_expire);
	reset_stat(shared_client, expired_inuse);

	reset_stat(thread, taskq_add);
	reset_stat(thread, taskq_add_dup);
	reset_stat(thread, taskq_dispatch);
	reset_stat(thread, max_share_threads);

	reset_stat(debug, nreads);
	reset_stat(debug, nwrites);
	reset_stat(debug, statfs_thread);

	return (count);
}
#endif /* DEBUG */

/*
 * Register a character device and do a mknod in /proc/fs/samfs
 *  for the ioctl() interface
 */
int
samsys_init(void)
{
#if CONFIG_PROC_FS
	struct proc_dir_entry *proc_samsys_major;
	struct proc_dir_entry *proc_samsys_obj;
#endif	/* CONFIG_PROC_FS */
	long major = 0;

	/*
	 * Register samsys character device
	 *  samsys_major is assigned by Linux
	 */
	samsys_major = register_chrdev(0, SAMSYS_CDEV, &samsys_fops);
	major = samsys_major;
#if CONFIG_PROC_FS
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
	/*
	 * Do a "mknod" in /proc/fs/samfs of /proc/fs/samfs/samsys
	 */
	proc_samsys_obj = proc_mknod(SAMSYS_CDEV, S_IFCHR | 0666,
	    proc_fs_samqfs,
	    (kdev_t)MKDEV(samsys_major, 0));
#else
	/*
	 * Don't have proc_mknod() so make a symlink to /dev/samsys.
	 */
	proc_samsys_obj = proc_symlink(SAMSYS_CDEV, proc_fs_samqfs,
	    "/dev/samsys");
#endif
	proc_samsys_major = create_proc_read_entry("major", 0444,
	    proc_fs_samqfs,
	    samsys_read_major, (void *)major);
	create_proc_read_entry("kstats", 0444, proc_fs_samqfs,
	    samsys_read_kstats, NULL);
#ifdef DEBUG
	{
		struct proc_dir_entry *p;
		p = create_proc_entry("reset_kstats", 0200, proc_fs_samqfs);
		if (p) {
			p->write_proc = samsys_reset_kstats;
		}
	}
#endif /* DEBUG */
#endif /* CONFIG_PROC_FS */


	return (0);
}

void
samsys_cleanup(void)
{
	(void) unregister_chrdev(samsys_major, "samsys");

#if CONFIG_PROC_FS
#ifdef DEBUG
	remove_proc_entry("reset_kstats", proc_fs_samqfs);
#endif
	remove_proc_entry("kstats", proc_fs_samqfs);
	remove_proc_entry("major", proc_fs_samqfs);
	remove_proc_entry(SAMSYS_CDEV, proc_fs_samqfs);
#endif /* CONFIG_PROC_FS */
}


static struct file_operations samsys_fops = {
	owner:		NULL,
	llseek:		NULL,
	read:		samsys_read,
	write:		samsys_write,
	readdir:	NULL,
	poll:		NULL,
	ioctl:		samsys_ioctl,
};
