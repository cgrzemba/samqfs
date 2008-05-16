/*
 * ----- init_linux.c - Process the initialization functions.
 *
 * Defines the samfs module for Linux.
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
#pragma ident "$Revision: 1.34 $"
#endif

#define	SAM_INIT

#define	EXPORT_SYMTAB

#include "sam/osversion.h"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/unistd.h>

#include <linux/proc_fs.h>
#include <linux/fs.h>

#include <asm/system.h>
#include <asm/uaccess.h>

#include "sam/types.h"

#include "inode.h"
#include "samfs_linux.h"
#include "clextern.h"
#include "global.h"
#include "kstats_linux.h"
#include "pub/version.h"
#include "trace.h"


struct sam_shared_client_statistics sam_shared_client_stats;
struct sam_sam_statistics sam_sam_stats;
struct sam_thread_statistics sam_thread_stats;
#ifdef DEBUG
struct sam_debug_statistics sam_debug_stats;
#endif
extern struct proc_dir_entry ** QFS_proc_root_fs;

sam_global_tbl_t samgt;		/* SAM-QFS global parameters */

int sam_tracing = 1;		/* sam_tracing always enabled */
int sam_trace_size = 0;		/* <= 0 means compute a default */

char *sam_zero_block = NULL;	/* a block of zeros */
int sam_zero_block_size = 0;	/* size of a block of zeros */
int panic_on_fs_error = 1;	/* a debug variable */

const char sam_version[] = SAM_BUILD_INFO;
const char sam_build_uname[] = SAM_BUILD_UNAME;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)) && \
	(LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18))
struct super_block *samqfs_read_sblk(struct file_system_type *, int,
					const char *, void *);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18))
int samqfs_read_sblk(struct file_system_type *, int, const char *, void *,
			struct vfsmount *);
#endif

struct proc_dir_entry *proc_fs_samqfs;

static void sam_clean_ino(void);
static void sam_init_ino(void);
static void sam_init_kstats(void);
extern unsigned int nodev_init(void);
extern int nodev_cleanup(void);
extern int samsys_init(void);
extern void samsys_cleanup(void);
extern void sam_sc_daemon_init(void);

void
sam_init_sharefs_rpc_item_cache(void)
{
	samgt.item_cache = kmem_cache_create("sam_rpcitem_cache",
	    sizeof (m_sharefs_item_t), 0, 0, NULL, NULL);
}

void
sam_delete_sharefs_rpc_item_cache(void)
{
	kmem_cache_destroy(samgt.item_cache);
	samgt.item_cache = NULL;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
void samqfs_kill_sblk(struct super_block *);
struct file_system_type samqfs_fs_type = {
	.owner		= THIS_MODULE,
	.name		= "samfs",
	.get_sb		= samqfs_read_sblk,
	.kill_sb	= samqfs_kill_sblk,
	.fs_flags	= FS_BINARY_MOUNTDATA
};
#else
struct super_block *samqfs_read_sblk(struct super_block *, void *, int);
DECLARE_FSTYPE(samqfs_fs_type, "samfs", samqfs_read_sblk, 0);
#endif


static void
sam_init_client_msg_cache(void)
{
	samgt.client_msg_cache =
	    kmem_cache_create("sam_clnt_msg_cache",
	    sizeof (sam_client_msg_t), 0, 0, NULL, NULL);
	ASSERT(samgt.client_msg_cache);
}

static void
sam_delete_client_msg_cache(void)
{
	if (samgt.client_msg_cache) {
		kmem_cache_destroy(samgt.client_msg_cache);
	}
}

/*
 * -----	samqfs_init - Initialize the SAM-QFS filesystem.
 *	Called after the module is loaded.
 *	Allocate the global defaults and lock table.
 */

int				/* ERRNO if error, 0 if successful. */
samqfs_init(struct file_system_type *fstype)
{
	memset((char *)&samgt, 0, sizeof (sam_global_tbl_t));

	sam_mutex_init(&samgt.global_mutex, NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&samgt.buf_mutex, NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&samgt.time_mutex, NULL, MUTEX_DEFAULT, NULL);

	sam_zero_block = (char *)kmem_alloc(SAM_DEV_BSIZE_LINUX, KM_SLEEP);

	if (sam_zero_block == NULL) {
		return (ENOMEM);
	}
	memset(sam_zero_block, 0, SAM_DEV_BSIZE_LINUX);
	sam_zero_block_size = SAM_DEV_BSIZE_LINUX;

	sam_init_ino();
	sam_init_client_msg_cache();
	sam_init_sharefs_rpc_item_cache();

	sam_sc_daemon_init();

#if	SAM_TRACE
	sam_trace_init();
#endif

	sam_init_kstats();
	sam_taskq_init();

	return (0);
}


#define	CLR_LSB(x)	((x) &= ((x)-1))	/* clear least-sig 1-bit in x */
#define	MULTI_BIT(x)	((x)&((x)-1))		/* !0 if > 1 bit set in x */

/*
 * -----	sam_init_ino - Initialize the SAM inode chains.
 *	Initialize the SAM filesystem inode hash chains and locks.
 *	Initialize the SAM filesystem inode free chain pointers.
 *	Initialize the SAM filesystem trace lock.
 */

static void
sam_init_ino(void)
{
	sam_mutex_init(&samgt.ifreelock, NULL, MUTEX_DEFAULT, NULL);
	sam_create_ino_cache();
}


/*
 * -----	sam_clean_ino - Free the SAM inode chains.
 *	Free the SAM filesystem inode hash chains and locks.
 */

static void
sam_clean_ino(void)
{
	sam_delete_ino_cache();
}


void
samqfs_cleanup(void)
{
	sam_mount_t *mp, *next_mp;

	/*
	 * If any filesystems mounting or mounted, return busy.
	 */
	mutex_enter(&samgt.global_mutex);

	/*
	 * Cancel (or wait for) any scheduled tasks.
	 */
	sam_taskq_destroy();

	/*
	 * Terminate all shared file system read socket threads.
	 */
	for (mp = samgt.mp_list; mp != NULL; ) {
		if (SAM_IS_SHARED_FS(mp)) {
			int count = 5;

			mutex_enter(&mp->ms.m_cl_wrmutex);
			while ((mp->ms.m_cl_thread ||
			    mp->ms.m_no_clients) && count--) {
				if (mp->ms.m_cl_thread) {
					send_sig(SIGTERM, mp->ms.m_cl_thread,
					    0);
				}
				mutex_exit(&mp->ms.m_cl_wrmutex);
				/*
				 * Delay for 1 second
				 */
				delay(hz);
				mutex_enter(&mp->ms.m_cl_wrmutex);
			}
			if (mp->ms.m_cl_thread || mp->ms.m_no_clients) {
				mutex_exit(&mp->ms.m_cl_wrmutex);
				mutex_exit(&samgt.global_mutex);
				return;
			}
			sam_clear_sock_fp(&mp->ms.m_cl_sh);
			mutex_exit(&mp->ms.m_cl_wrmutex);
		}
		mp = mp->ms.m_mp_next;
	}
	mutex_exit(&samgt.global_mutex);

	/*
	 * Cancel all call backs and de-allocate all memory.
	 */
	(void) sam_clean_ino();

#if	SAM_TRACE
	(void) sam_trace_fini();
#endif

	sam_delete_sharefs_rpc_item_cache();
	sam_delete_client_msg_cache();

	/*
	 * Free all mount tables.
	 */
	mutex_enter(&samgt.global_mutex);
	for (mp = samgt.mp_list; mp != NULL; ) {
		next_mp = mp->ms.m_mp_next;
		kfree(mp);
		mp = next_mp;
	}
	mutex_exit(&samgt.global_mutex);

	kfree(sam_zero_block);
	sam_zero_block = NULL;
	sam_zero_block_size = 0;
}


#define	SAM_KSTAT64(cat, stat) \
	kstat_named_init(&sam_##cat##_stats.stat, #stat, KSTAT_DATA_UINT64)


static void
kstat_named_init(kstat_named_t *ks, char *name, uchar_t type)
{
	bzero(ks->name, KSTAT_STRLEN);
	strncpy(ks->name, name, KSTAT_STRLEN-1);
	ks->data_type = type;
}

/*
 * ----- sam_init_kstats - Initialize the SAM-QFS kernel statistics structures.
 *
 * We still call it kstats on linux so people recognize we're trying trying
 * to do the same thing.
 */
static void
sam_init_kstats(void)
{
	SAM_KSTAT64(sam, stage_start);
	SAM_KSTAT64(sam, stage_partial);
	SAM_KSTAT64(sam, stage_window);
	SAM_KSTAT64(sam, stage_errors);
	SAM_KSTAT64(sam, stage_reissue);
	SAM_KSTAT64(sam, stage_cancel);

	SAM_KSTAT64(shared_client, msg_in);
	SAM_KSTAT64(shared_client, msg_out);
	SAM_KSTAT64(shared_client, sync_pages);
	SAM_KSTAT64(shared_client, inval_pages);
	SAM_KSTAT64(shared_client, stale_indir);
	SAM_KSTAT64(shared_client, dio_switch);
	SAM_KSTAT64(shared_client, abr_switch);
	SAM_KSTAT64(shared_client, expire_task);
	SAM_KSTAT64(shared_client, expire_task_dup);
	SAM_KSTAT64(shared_client, retransmit_msg);
	SAM_KSTAT64(shared_client, notify_ino);
	SAM_KSTAT64(shared_client, notify_expire);
	SAM_KSTAT64(shared_client, expired_inuse);

	SAM_KSTAT64(thread, taskq_add);
	SAM_KSTAT64(thread, taskq_add_dup);
	SAM_KSTAT64(thread, taskq_dispatch);
	SAM_KSTAT64(thread, max_share_threads);

#ifdef DEBUG
	SAM_KSTAT64(debug, nreads);
	SAM_KSTAT64(debug, nwrites);
	SAM_KSTAT64(debug, statfs_thread);
#endif
}



/*
 * errnos received on the wire are solaris errnos.
 * These routines translate them to/from their Linux equivalents.
 */
#define	SOL_ECANCELED		47
#define	SOL_ENOTSUP		48
#define	SOL_EBADR		51
#define	SOL_EPROTO		71
#define	SOL_EREMCHG		82

int
local_to_solaris_errno(int local_errno)
{
	int solaris_errno = 0;

	switch (local_errno) {
	case EOPNOTSUPP:
		solaris_errno = SOL_ENOTSUP;
		break;
	case ECANCELED:
		solaris_errno = SOL_ECANCELED;
		break;
	case EBADR:
		solaris_errno = SOL_EBADR;
		break;
	case EREMCHG:
		solaris_errno = SOL_EREMCHG;
		break;
	case EPROTO:
		solaris_errno = SOL_EPROTO;
		break;
	default:
		solaris_errno = local_errno;
		break;
	}
	return (solaris_errno);
}

int
solaris_to_local_errno(int solaris_errno)
{
	int local_errno = 0;

	switch (solaris_errno) {
	case SOL_ENOTSUP:
		local_errno = EOPNOTSUPP;
		break;
	case SOL_ECANCELED:
		local_errno = ECANCELED;
		break;
	case SOL_EBADR:
		local_errno = EBADR;
		break;
	case SOL_EREMCHG:
		local_errno = EREMCHG;
		break;
	case SOL_EPROTO:
		local_errno = EPROTO;
		break;
	default:
		local_errno = solaris_errno;
		break;
	}
	return (local_errno);
}


int __init
samqfs_module_init(void)
{
	int ret = 0;

	ret = register_filesystem(&samqfs_fs_type);

	if (ret < 0) {
		return (-EINVAL);
	}

#if CONFIG_PROC_FS
	proc_fs_samqfs = proc_mkdir("samfs", *QFS_proc_root_fs);
#endif

	if ((ret = samqfs_init(&samqfs_fs_type)) < 0) {
		return (ret);
	}

	if ((ret = samsys_init()) < 0) {
		return (ret);
	}

	if ((ret = nodev_init()) < 0) {
		return (ret);
	}

	return (0);
}

void __exit
samqfs_module_exit(void)
{
	(void) nodev_cleanup();

	(void) samsys_cleanup();

#if CONFIG_PROC_FS
	remove_proc_entry("samfs", *QFS_proc_root_fs);
#endif

	unregister_filesystem(&samqfs_fs_type);

	samqfs_cleanup();
}

module_init(samqfs_module_init)
module_exit(samqfs_module_exit)

MODULE_LICENSE("SMI");
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
MODULE_INFO(supported, "external");
MODULE_INFO(depends, "SUNWqfs_ki");
#endif
