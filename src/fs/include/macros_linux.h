/*
 *	macros_linux.h - SAM-QFS macros.
 *
 *	Description:
 *		define SAM-QFS macros for Linux.
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

#ifndef	_SAM_MACROS_LINUX_H
#define	_SAM_MACROS_LINUX_H

#ifdef sun
#pragma ident "$Revision: 1.64 $"
#endif

#ifdef __KERNEL__
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 16))
#include "linux/mutex.h"
#endif

#ifdef ASSERT
#undef	ASSERT
#endif

#ifdef DEBUG
static inline void
assert_failed(char *a, char *f, int l)
{
	printk("QFS Assertion failed (%s) %s, line %d\n", a, f, l);
	BUG();
}

#define	ASSERT(EX) ((EX) ? ((void)0) : assert_failed(#EX, __FILE__, __LINE__))
#else
#define	ASSERT(EX)	do { } while (0)
#endif


int local_to_solaris_errno(int local_errno);
int solaris_to_local_errno(int solaris_errno);


/*
 * ----- read/write locks
 */

extern void rfs_down_write(struct rw_semaphore *sem);
extern void rfs_up_write(struct rw_semaphore *sem);
extern void rfs_down_read(struct rw_semaphore *sem);
extern void rfs_up_read(struct rw_semaphore *sem);
struct inode;
struct dentry *rfs_d_alloc_anon(struct inode *);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
extern int rfs_down_read_trylock(struct rw_semaphore *sem);
extern int rfs_down_write_trylock(struct rw_semaphore *sem);
extern void rfs_downgrade_write(struct rw_semaphore *sem);
struct block_device;
struct inode;
extern int  rfs_do_linux_fbzero(struct inode *, offset_t, unsigned,
				struct block_device *bdev, int, int);
#endif /* LINUX_VERSION_CODE >= 2.6.0 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
extern int  rfs_do_linux_fbzero(struct inode *, offset_t, unsigned, kdev_t,
				int, int);
#endif /* LINUX_VERSION_CODE < 2.6.0 */

struct inode;
void rfs_lock_inode(struct inode *);
void rfs_unlock_inode(struct inode *);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 16))
void rfs_down(struct semaphore *sem);
void rfs_up(struct semaphore *sem);

#define	sam_mutex_init(sem, a2, a3, a4)	init_MUTEX(sem)
#define	mutex_enter(sem)		rfs_down(sem)
#define	mutex_exit(sem)			rfs_up(sem)

#define	mutex_destroy(x)		do { } while (0)
#endif /* < 2.6.16 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 16))
#define	sam_mutex_init(sem, a2, a3, a4)	mutex_init(sem)
#define	mutex_enter(sem)		mutex_lock(sem)
#define	mutex_exit(sem)			mutex_unlock(sem)
#endif /* >= 2.6.16 */

#define	sam_open_mutex_operation(ip, sem)	mutex_enter(sem)

typedef enum {
	RW_OPEN,
	RW_WRITER,
	RW_READER
} krw_t;

#define	RW_LOCK_INIT_OS(lock, a2, a3, a4)	rw_lock_init_linux((lock))
#define	RW_LOCK_DESTROY_OS(rwl)

#define	RW_LOCK_OS(lock, state)		rw_lock_linux((lock), (state))
#define	RW_UNLOCK_OS(lock, state)	rw_unlock_linux((lock), (state))
#define	RW_UNLOCK_CURRENT_OS(lock) rw_unlock_linux((lock), (lock)->rw_state)
#define	RW_OWNER_OS(rwl)			rw_owner_linux(rwl)
#define	RW_TRYUPGRADE_OS(rwl)		(0)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
#define	RW_DOWNGRADE_OS(rwl)		rw_downgrade_linux(rwl)
#else
#define	RW_DOWNGRADE_OS(rwl)			\
	do {					\
		RW_UNLOCK_OS(rwl, RW_WRITER);	\
		RW_LOCK_OS(rwl, RW_READER);	\
	} while (0)
#endif
#define	RW_WRITE_HELD_OS(rwl)		rw_write_held_linux(rwl)
#define	RW_TRYENTER_OS(rwl, state)	rw_trylock_linux(rwl, state)

#define	sam_rwdlock_ino(ip, ls, f)	RW_LOCK_OS(&ip->data_rwl, ls)

#ifdef DEBUG
#define	MUTEX_HELD(sem)		(1)
#define	RW_WRITE_HELD(lock)	RW_WRITE_HELD_OS(lock)
#define	RW_LOCK_HELD(lock)	(1)
#endif /* DEBUG */

/*
 * ----- Macros to upgrade READERS lock to WRITERS lock.
 */
#define	RW_UPGRADE_OS(rwl)	if (!RW_TRYUPGRADE_OS(rwl)) {	\
				RW_UNLOCK_OS(rwl, RW_READER);	\
				RW_LOCK_OS(rwl, RW_WRITER);	\
			}


/*
 * ----- rw_lock_init_linux - Initialize the contents of a krwlock_t
 *  which allows tracking of the current lock owner (process) and lock
 *  state (RW_READER, RW_WRITER).  The lock state is meaningful only
 *  when the lock owner is not NULL.
 */
static inline void
rw_lock_init_linux(krwlock_t *lock)
{
	lock->rw_owner = NULL;
	lock->rw_state = RW_OPEN;
	init_rwsem(&lock->rw_sem);
}


/*
 * ----- rw_lock_linux - Wait to get a lock of specified state (RW_READER,
 *  RW_WRITER).  After getting the lock, save the new lock state and owner.
 */
static inline void
rw_lock_linux(krwlock_t *lock, krw_t state)
{
	switch (state) {
	case RW_READER:
		rfs_down_read(&lock->rw_sem);
		ASSERT(!lock->rw_owner);
		break;
	case RW_WRITER:
		rfs_down_write(&lock->rw_sem);
		lock->rw_owner = current;
		break;
	case RW_OPEN:
		ASSERT(0);
		break;
	}
	ASSERT(lock->rw_state != RW_WRITER);
	lock->rw_state = state;
}


/*
 * ----- rw_unlock_linux - Wait to release a lock of the specified type
 *  (RW_READER, RW_WRITER).  If there aren't other active locks, set
 *  the saved owner to NULL.
 *
 */
static inline void
rw_unlock_linux(krwlock_t *lock, krw_t state)
{
	/*
	 * Must have an active lock of the specified state
	 */
	if (lock->rw_state != state) {
		panic("Trying to release lock bad state");
	}

	switch (state) {
	case RW_READER:
		ASSERT(lock->rw_owner == NULL);
		rfs_up_read(&lock->rw_sem);
		break;
	case RW_WRITER:
		if (lock->rw_owner != current) {
			panic("Trying to release unowned write lock");
		}
		lock->rw_owner = NULL;
		lock->rw_state = RW_OPEN;
		rfs_up_write(&lock->rw_sem);
		break;
	case RW_OPEN:
		ASSERT(0);
		break;
	}
}


/*
 * ----- rw_trylock_linux - Attempt to get a lock of specified state (RW_READER,
 *  RW_WRITER). Return 1 if successful, 0 if not.
 */
static inline int
rw_trylock_linux(krwlock_t *lock, krw_t state)
{
	switch (state) {
	case RW_READER:
		if (rfs_down_read_trylock(&lock->rw_sem) == 0) {
			return (0);
		}
		ASSERT(!lock->rw_owner);
		break;
	case RW_WRITER:
		if (rfs_down_write_trylock(&lock->rw_sem) == 0) {
			return (0);
		}
		lock->rw_owner = current;
		ASSERT(lock->rw_state != RW_WRITER);
		break;
	case RW_OPEN:
		ASSERT(0);
		break;
	}
	lock->rw_state = state;
	return (1);
}


/*
 * ----- rw_owner_linux - Return the task that holds the lock.
 * This is only valid if the lock is a write lock. Returns
 * a pointer to the task_struct for a write lock, otherwise NULL.
 */
static inline struct task_struct *
rw_owner_linux(krwlock_t *lock)
{
	return (lock->rw_owner);
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
/*
 * ----- rw_downgrade_linux - Downgrade lock from a write lock
 * to a read lock.
 */

static inline void
rw_downgrade_linux(krwlock_t *lock)
{
	if (lock->rw_state != RW_WRITER) {
		panic("Trying to downgrade an RW_READER lock");
	}
	if (lock->rw_owner != current) {
		panic("Trying to release unowned write lock");
	}
	lock->rw_owner = NULL;
	lock->rw_state = RW_READER;
	rfs_downgrade_write(&lock->rw_sem);
}
#endif /* 2.6 kernel */

/*
 * ----- rw_write_held_linux - Check if the current task holds a write
 * lock. Return 1 if yes, 0 if no.
 */
static inline int
rw_write_held_linux(krwlock_t *lock)
{
	return ((lock->rw_state == RW_WRITER) && (lock->rw_owner == current));
}


/*
 * ----- General interface to current time in seconds.
 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
extern struct timeval *QFS_xtime;
#define	SAM_SECOND()	(QFS_xtime->tv_sec)
#else
extern time_t QFS_xtime(void);
#define	SAM_SECOND() (QFS_xtime())
#endif

/*
 * ----- Detach thread from parent.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)

void rfs_daemonize(const char *, ...);
#define	SAM_DAEMONIZE(str)	rfs_daemonize(str)

#else /* < 2.6.0 */

#define	SAM_DAEMONIZE(str)					\
do {								\
	int comm_length, name_length;				\
								\
	daemonize();						\
	sigfillset(&current->blocked);				\
								\
	name_length = strlen(str);				\
	comm_length = sizeof (current->comm) - 1;		\
								\
	if (name_length <= comm_length) {			\
		comm_length = name_length;			\
	}							\
	current->comm[comm_length] = 0;				\
	memcpy(current->comm, str, comm_length);		\
} while (0)

#endif /* LINUX_VERSION_CODE >= 2.6.0 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
#define	rfs_try_module_get(x)	MOD_INC_USE_COUNT
#define	rfs_module_put(x)	MOD_DEC_USE_COUNT
#endif

#endif   /* __KERNEL__ */

/*
 * ----- General interface to hrestime (timespec_t).
 */
#define	SAM_HRESTIME(a) do_gettimeofday((a))


/*
 * Check if the mandatory locking bits are set.
 */
#define	MANDMODE_OS(imode)	(((imode) & (S_ISGID|(S_IXUSR>>3))) == S_ISGID)
#define	MANDLOCK_OS(ip, imode)	(S_ISREG(ip->di.mode) && MANDMODE_OS(imode))


/*
 * Check if the calling thread is one of the sam_sharefs_threads
 */
extern char *sharefs_thread_name;

#define	IS_SHAREFS_THREAD_OS \
	(strncmp(current->comm, sharefs_thread_name, \
		strlen(current->comm)) == 0)

/*
 * ----- General test if caller is NFS.
 */
extern char *nfsd_thread_name;

#define	SAM_THREAD_IS_NFS()	\
	(strncmp(current->comm, nfsd_thread_name, strlen(current->comm)) == 0)


/*
 * ----- Increment/decrement FS mount point reference counts.
 * (One reference per in-core FS inode.)  No-op under Linux.
 */
#define	SAM_VFS_HOLD(mp)
#define	SAM_VFS_RELE(mp)

/*
 * Track the number of rdsock threads in the FS.
 * Under linux this is mostly a no-op.  While non-zero,
 * we shouldn't destroy the base mount structure.
 */
#define	SAM_SHARE_DAEMON_HOLD_FS(mp) (mp)->ms.m_cl_nsocks++
#define	SAM_SHARE_DAEMON_RELE_FS(mp) (mp)->ms.m_cl_nsocks--
#define	SAM_SHARE_DAEMON_CNVT_FS(mp)

/*
 * ----- Decrement operation activity count.
 */

#define	SAM_INC_OPERATION(mp)  atomic_inc(&mp->ms.m_cl_active_ops)
#define	SAM_DEC_OPERATION(mp)  atomic_dec(&mp->ms.m_cl_active_ops)

#define	SAM_CLOSE_OPERATION(mp, error) {			\
	SAM_DEC_OPERATION(mp);					\
	SAM_NFS_JUKEBOX_ERROR(error);				\
	if (error > 0) {					\
		error = -error;					\
	}							\
}

/*
 * Any NFS access is assumed to be v3 since v2 is not supported
 * and we can't tell the difference.
 */
#define	SAM_THREAD_IS_NFSv3() (SAM_THREAD_IS_NFS())

/*
 * Returning ENOMEM is a kludge to get the Linux NFS
 * server to drop the request while a file is staged or
 * failover completes.
 */
#define	SAM_NFS_JUKEBOX_ERROR(error) {				\
	if ((error == EAGAIN) && (SAM_THREAD_IS_NFS())) {	\
		error = ENOMEM;					\
	}							\
}

#define	THREAD_KILL_OS(t)	send_sig(SIGKILL, t, 0);

/*
 * ----- Macros to get address of indirect extent
 */
#define	ie_ptr(mapp)	(sam_indirect_extent_t *)(void *)mapp->buf


/*
 * ----- Trace macros for vnode
 */
#define	SAM_ITOP(ip)	SAM_SITOLI(ip)


/*
 * ----- Current process id
 */
#define	SAM_CUR_PID		(current->pid)


/*
 * ----- Hold and Release inode macros.
 */
#define	VN_HOLD_OS(ip)		atomic_inc(&(SAM_SITOLI(ip)->i_count))
#define	VN_RELE_OS(ip)		iput(SAM_SITOLI(ip))


/*
 * ----- Macros to use instead of ifdefs in places of common code between OSes
 */

#define	SAM_SET_ABR(a)		do {} while (0)
#define	SAM_SET_LEASEFLG(a)	do {} while (0)
#define	SAM_CLEAR_LEASEFLG(a)	do {} while (0)

#define	sam_start_stop_rmedia(a, b)	do { } while (0)
#define	sam_mount_setwm_blocks(a)	do { } while (0)
#define	sam_send_to_arfind(a, b, c)	do { } while (0)
#define	sam_send_event(a, b, c, d, e, f)	do { } while (0)

#define	sam_get_segment_ino(a, b, c)		(ENOTSUP)
#define	sam_quota_fonline(a, b, c)		(0)
#define	sam_priv_sam_syscall(a, b, c, d)	(ENOTSUP)
#define	sam_quota_foffline(a, b, c)		(0)
#define	sam_req_fsck(a, b, c)			do { } while (0)
#define	sam_open_ino_operation(a, b)		sam_open_operation(a)
#define	sam_unset_operation_nb(a)		do { } while (0)
#define	sam_map_osd(a, b, c, d, e)		(ENOTSUP)
#define	sam_set_end_of_obj(a, b, c)		(0)


/*
 * ----- Credential macros.
 */
#define	crgetuid(credp)		(credp->cr_uid)


/*
 * ----- Security Policy macros.
 */
#define	secpolicy_fs_config(credp, vfsp)	(suser(credp) ? 0 : EPERM)

/*
 * ----- Project ID.
 */
#define	sam_set_projid(a, b, c)				(ENOSYS)

#endif /* _SAM_MACROS_LINUX_H */
