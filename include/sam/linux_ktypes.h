/*
 * sam/linux_ktypes.h - SAM-FS system types for Linux kernel modules.
 *
 * System type definitions for the SAM-FS filesystem and daemons.
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


#ifndef	_SAM_LINUX_KTYPES_H
#define	_SAM_LINUX_KTYPES_H

#ifdef sun
#pragma ident "$Revision: 1.52 $"
#endif

#ifdef	__KERNEL__

#include "linux/spinlock.h"
#include "linux/slab.h"
#include "linux/version.h"
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 16))
#include "linux/mutex.h"
#endif
#include "linux/sched.h"

/*
 * Bits to shift to get SAM_DEV_BSIZE_LINUX
 * block offset from byte offset.
 */
#define	SAM_DEV_BSHIFT_LINUX	12

/*
 * File block size
 */
#define	SAM_DEV_BSIZE_LINUX	(1 << SAM_DEV_BSHIFT_LINUX)  /* 4096 */

/*
 * Bits to shift to convert SAM_DEV_BSIZE block
 * offset to SAM_DEV_BSIZE_LINUX block offset
 */
#define	SAM_DEV_BLOCKS_BSHIFT_TO_LINUX	2

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18))
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))
#endif
#define	howmany(x, y)	(((x)+((y)-1))/(y))

#define	bzero(p, l)	memset((p), 0, (l))
#define	bcopy(s, d, l)	memcpy((d), (s), (l))

#define	SYNC_ATTR	0x01		/* sync attributes only */
#define	SYNC_CLOSE	0x02		/* close open file */
#define	SYNC_ALL	0x04		/* force to sync all fs */

#ifndef DIRECTIO_OFF
#define	DIRECTIO_OFF 0
#define	DIRECTIO_ON  1
#endif

#define	B_INVAL	0x010000	/* does not contain valid info  */
#define	B_TRUNC	0x100000	/* truncate page without I/O */
#define	B_ASYNC	0x000400	/* don't wait for I/O completion */

/*
 * Owner permission bits
 */
#define	S_IREAD		S_IRUSR
#define	S_IWRITE	S_IWUSR
#define	S_IEXEC		S_IXUSR

/*
 * Kernel mode flags used by the SAM-QFS protocol
 */
#define	FREAD	0x01
#define	FWRITE	0x02
#define	FAPPEND	0x08


/*
 * vnode types.  VNON means no type.  These values are unrelated to
 * values in on-disk inodes.
 */
#ifndef _LINUX_VNODE_I
#define	_LINUX_VNODE_I
typedef enum vtype {
	VNON	= 0,
	VREG	= 1,
	VDIR	= 2,
	VBLK	= 3,
	VCHR	= 4,
	VLNK	= 5,
	VFIFO	= 6,
	VDOOR	= 7,
	VPROC	= 8,
	VSOCK	= 9,
	VBAD	= 10
} vtype_t;
#endif

/*
 * Attributes of interest to the caller of setattr or getattr.
 */
#define	QFS_AT_TYPE	0x0001
#define	QFS_AT_MODE	0x0002
#define	QFS_AT_UID	0x0004
#define	QFS_AT_GID	0x0008
#define	QFS_AT_FSID	0x0010
#define	QFS_AT_NODEID	0x0020
#define	QFS_AT_NLINK	0x0040
#define	QFS_AT_SIZE	0x0080
#define	QFS_AT_ATIME	0x0100
#define	QFS_AT_MTIME	0x0200
#define	QFS_AT_CTIME	0x0400
#define	QFS_AT_RDEV	0x0800
#define	QFS_AT_BLKSIZE	0x1000
#define	QFS_AT_NBLOCKS	0x2000
#define	QFS_AT_VCODE	0x4000

#define	DEV_BSIZE	512

#define	bcmp	memcmp

#ifndef MAX
#define	MAX(a, b)	((a) < (b) ? (b) : (a))
#endif
#ifndef MIN
#define	MIN(a, b)	((a) < (b) ? (a) : (b))
#endif

/*
 * The current Linux task
 */
#define	curthread	current

static inline void delay(long timeout)
{
	do {
		set_current_state(TASK_INTERRUPTIBLE);
		timeout = schedule_timeout(timeout);
	} while (timeout);
	__set_current_state(TASK_RUNNING);
}

extern unsigned long *QFS_jiffies;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
typedef int *qfs_nopage_arg3_t;
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
typedef int qfs_nopage_arg3_t;
#endif

#define	hz	HZ
#define	lbolt	(*QFS_jiffies)

#ifndef ENOTSUP
#define	ENOTSUP	ENOSYS
#endif

typedef	enum uio_seg { UIO_USERSPACE, UIO_SYSSPACE, UIO_USERISPACE } uio_seg_t;
typedef	enum uio_rw { UIO_READ, UIO_WRITE } uio_rw_t;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 16))
typedef struct semaphore kmutex_t;
#endif /* < 2.6.16 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 16))
typedef struct mutex kmutex_t;
#endif /* >= 2.6.16 */

typedef wait_queue_head_t kcondvar_t;

typedef struct krwlock {
	struct rw_semaphore rw_sem;
	struct task_struct *rw_owner;
	int rw_state;
} krwlock_t;

/*
 * cv_init, cv_signal, cv_broadcast, and cv_wait
 *  work something like condition variables using
 *  Linux waitqueues.
 *
 *  Waiters (cv_wait()) set WQ_FLAG_EXCLUSIVE so wake_up()
 *  only wakes up one process and wake_up_all()
 *  wakes up all waiters.
 */
#define	cv_init(cv, a2, a3, a4)	init_waitqueue_head(cv)
#define	cv_signal(cv)		wake_up(cv)
#define	cv_broadcast(cv)	wake_up_all(cv)

void cv_wait(wait_queue_head_t *wq, kmutex_t *wl);
void cv_wait_light(wait_queue_head_t *wq, kmutex_t *wl);
void cv_timedwait(wait_queue_head_t *wq, kmutex_t *wl, long timeout);
void cv_timedwait_light(wait_queue_head_t *wq, kmutex_t *wl, long timeout);
long cv_timedwait_sig(wait_queue_head_t *wq, kmutex_t *wl, long timeout);

#define	cv_destroy(x)	do { } while (0)


typedef void	*timeout_id_t;  /* opaque handle from timeout(9F) */

/*
 * Misc definitions that Linux doesn't have
 */

#define	INT32_MAX	(2147483647)
#define	MICROSEC	1000000

typedef struct timespec	timespec_t;
typedef struct dirent64 dirent64_t;
typedef struct iovec iovec_t;

#define	MAXOFFSET_T	0x7fffffffffffffffLL

typedef struct uio {
	iovec_t		*uio_iov;	/* pointer to array of iovecs */
	int		uio_iovcnt;	/* number of iovecs */
	offset_t	uio_loffset;	/* file offset */
	int		uio_segflg;	/* address space (kernel or user) */
	short		uio_fmode;	/* file mode flags */
	offset_t	uio_limit;	/* u-limit (maximum byte offset) */
	ssize_t		uio_resid;	/* residual count */
} uio_t;

/*
 * User credentials.  The size of the cr_groups[] array is configurable
 * but is the same (ngroups_max) for all cred structures; cr_ngroups
 * records the number of elements currently in use, not the array size.
 */

typedef struct cred {
	uint_t	cr_ref;			/* reference count */
	uid_t	cr_uid;			/* effective user id */
	gid_t	cr_gid;			/* effective group id */
	uid_t	cr_ruid;		/* real user id */
	gid_t	cr_rgid;		/* real group id */
	uid_t	cr_suid;		/* "saved" user id (from exec) */
	gid_t	cr_sgid;		/* "saved" group id (from exec) */
	uint_t	cr_ngroups;		/* number of groups in cr_groups */
	gid_t	cr_groups[1];		/* supplementary group list */
} cred_t;

#define	CRED()	NULL


#define	CE_WARN	KERN_WARNING
#define	CE_NOTE	KERN_NOTICE
#define	CE_PANIC KERN_CRIT

#define	cmn_err(level, format, arg...)	do { \
	printk(level format, ## arg); \
	printk("\n"); \
} while (0);

#define	uprintf(format, arg...)	printk(KERN_INFO format, ## arg);


/*
 * Redefine memory allocation functions
 * for Linux
 */
#define	KM_SLEEP	GFP_KERNEL
#define	KM_NOSLEEP	GFP_ATOMIC

extern void *QFS_kmalloc(size_t size, int flags);
#define	kmem_alloc(s, f)	QFS_kmalloc(s, f)
#define	kmem_free(p, s)		kfree(p)

static inline void *kmem_zalloc(size_t size, int flag)
{
	void *m;

	m = QFS_kmalloc(size, flag);
	if (m) {
		memset(m, 0, size);
	}

	return ((void *)m);
}

typedef struct fbuf {
	caddr_t fb_addr;
	uint_t  fb_count;
	uint_t	fb_bsize;
#if !defined(_LP64)
	uint_t	fb_pad;
#endif
} fbuf_t;

#define	fbrelease(fbp)	kfree((char *)fbp)

#define	PAGESIZE	8192

#define	MAXBOFFSET	(MAXBSIZE - 1)
#define	MAXBMASK	(~MAXBOFFSET)

#ifndef	ECANCELED
#define	ECANCELED	47	/* Operation canceled */
#endif

#define	suser(credp)		rfs_suser()
#define	copyout(a, b, c)	copy_to_user(b, a, c)
#define	copyin(a, b, c)		copy_from_user(b, a, c)


/*
 * Linux doesn't bother to
 * define these on _LP64
 */
#ifdef _LP64
#define	F_GETLK64	12
#define	F_SETLK64	13
#define	F_SETLKW64	14
#endif

#endif	/* __KERNEL__ */

#endif	/* _SAM_LINUX_KTYPES_H */
