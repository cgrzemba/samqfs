/*
 * ----- scd.c - Interface for the file system system call daemons.
 *
 * This is the interface module that sits between the file system and
 * global file system system call daemons.
 *
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
#pragma ident "$Revision: 1.55 $"
#endif

#include "sam/osversion.h"

/* ----- UNIX Includes */

#ifdef sun
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/t_lock.h>
#include <sys/cred.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/ddi.h>
#include <sys/disp.h>

/* ----- SAMFS Includes */

#include <sam/types.h>
#include <sam/syscall.h>
#endif /* sun */
#ifdef linux
#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/types.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#include <linux/limits.h>
#endif
#include <sam/linux_types.h>
#endif /* linux */

#include "inode.h"
#include "mount.h"
#include "scd.h"
#include "global.h"
#ifdef sun
#include "extern.h"
#include "debug.h"
#include "trace.h"
#endif /* sun */

static int sam_send_cmd(sam_mount_t *mp, struct sam_syscall_daemon *scd,
	void *cmd, int size, int scdi);
static int sam_wait_cmd(struct sam_syscall_daemon *scd, enum uio_seg segflag,
	void *cmd, int size, int scdi);
static void sam_init_daemon(struct sam_syscall_daemon *scd);
static void sam_finish_daemon(struct sam_syscall_daemon *scd);
static boolean_t sam_is_failover_stage(int scdi, sam_mount_t *mp);

#ifdef linux
extern long cv_wait_sig(kcondvar_t *wq, kmutex_t *wl);
#endif


struct sam_syscall_daemon sam_scd_table[SCD_MAX];
static char *scd_name[] = {"sam-fsd", "sam-stagealld", "sam-stagerd" };

/*
 * ----- sam_send_stage_cmd -
 *
 * send stage command to the global system call daemon.
 */

int				/* ERRNO if error, 0 if successful. */
sam_send_stage_cmd(
	sam_mount_t *mp,	/* Pointer to Mount table */
	enum SCD_daemons scdi,	/* System Call Daemon index */
	void *cmd,		/* Pointer to command */
	int size)		/* Size of command */
{
	struct sam_syscall_daemon *scd = &sam_scd_table[scdi];

	return (sam_send_cmd(mp, scd, cmd, size, (int)scdi));
}


/*
 * ----- sam_send_scd_cmd - send command to the global system call daemon.
 *
 * The file system waits until the one command slot is free. Then
 * the file system copies the command into the global table and
 * signals the possible waiting daemon.
 */

int				/* ERRNO if error, 0 if successful. */
sam_send_scd_cmd(
	enum SCD_daemons scdi,	/* System Call Daemon index */
	void *cmd,		/* Pointer to command */
	int size)		/* Size of command */
{
	struct sam_syscall_daemon *scd = &sam_scd_table[scdi];

	return (sam_send_cmd(NULL, scd, cmd, size, (int)scdi));
}


/*
 * ----- sam_send_cmd - send command to the system call daemon.
 *
 * The file system waits until the one command slot is free. Then
 * the file system copies the command into the global table and
 * signals the possible waiting daemon.
 */

static int				/* ERRNO if error, 0 if successful. */
sam_send_cmd(
	sam_mount_t *mp,		/* Pointer to Mount table */
	struct sam_syscall_daemon *scd,	/* system call daemon record */
	void *cmd,			/* Pointer to command */
	int size,			/* Size of command */
	int scdi)			/* Daemon index */
{
	int ret;
	int error = 0;
	int timeout_cycles = 0;

	if (sam_is_failover_stage(scdi, mp)) {
		return (EREMCHG);	/* Kill this stage request */
	}

	if (!scd->active && (scdi == SCD_fsd)) {
		TRACE(T_SAM_DAE_ACTIVE, mp, scd->active, scdi, 3);
		return (EAGAIN);
	}

	mutex_enter(&scd->mutex);
	scd->put_wait++;
	while (scd->size) {
		if ((ret = sam_cv_wait1sec_sig(&scd->put_cv,
		    &scd->mutex)) <= 0) {
			if (!scd->active && (scdi == SCD_fsd)) {
				TRACE(T_SAM_DAE_ACTIVE, mp, scd->active,
				    scdi, 4);
				error = EAGAIN;
				break;
			} else if (ret == 0) {
				if (sam_is_failover_stage(scdi, mp)) {
					error = EREMCHG;
				} else {
					error = EINTR;	/* If signal received */
				}
				break;
			} else if (scd->timeout &&
			    (++timeout_cycles >= scd->timeout)) {
				error = EAGAIN;	/* If timed out */
				cmn_err(CE_WARN,
				    "SAM-QFS: %s not responding",
				    scd_name[scdi]);
				break;
			} else if (sam_is_failover_stage(scdi, mp)) {
				error = EREMCHG; /* Kill this stage request */
				break;
			}
			continue;
		}
	}
	if (error) {
		scd->put_wait--;
		mutex_exit(&scd->mutex);
		return (error);
	}
	scd->package = 1;
	scd->size = size;
	bcopy(cmd, &scd->cmd, size);
	scd->put_wait--;
	cv_signal(&scd->get_cv);
	mutex_exit(&scd->mutex);
	return (0);
}

/*
 * ----- sam_is_failover_stage -
 *
 * Return true if command is stager and failover is in progress.
 */

static boolean_t
sam_is_failover_stage(
	int scdi,
	sam_mount_t *mp)
{
#ifdef sun
	if (scdi == SCD_stager && mp != NULL && SAM_IS_SHARED_FS(mp) &&
	    (mp->mt.fi_status & FS_FAILOVER)) {
		return (B_TRUE);
	}
#endif /* sun */
	return (B_FALSE);
}

/*
 * ----- sam_wait_scd_cmd - global system call daemon waits for a command.
 *
 * Set command flag and signal the file system. This notifies the
 * file system that this daemon is waiting until the file system signals
 * with a new command. Get the command from the global table. The
 * daemon immediately issues this system call to wait again.
 */

int				/* ERRNO if error, 0 if successful. */
sam_wait_scd_cmd(
	enum SCD_daemons scdi,	/* System Call Daemon index */
	enum uio_seg segflag,	/* Location of buffer */
	void *cmd,		/* Return information for daemon here */
	int size)		/* Size of requested information */
{
	struct sam_syscall_daemon *scd = &sam_scd_table[scdi];

	return (sam_wait_cmd(scd, segflag, cmd, size, (int)scdi));
}


/*
 * ----- sam_wait_cmd - system call daemon waits for filesystem command.
 *
 * Set command flag and signal the filesystem. This notifies the
 * filesystem that this daemon is waiting until the filesystem signals
 * with a new command. Get the command from the global table. The
 * daemon immediately issues this system call to wait again.
 */

static int				/* ERRNO if error, 0 if successful. */
sam_wait_cmd(
	struct sam_syscall_daemon *scd,	/* system call daemon record */
	enum uio_seg segflag,		/* Location of buffer */
	void *cmd,			/* Return info for daemon here */
	int size,			/* Size of requested information */
	int scdi)			/* System Call Daemon index */
{
	int error = 0;

	mutex_enter(&scd->mutex);

	if (size == -1) {
		/*
		 * If daemon is telling us it is quitting.
		 */
		scd->active = 0;
		TRACE(T_SAM_DAE_ACTIVE, NULL, scd->active, scdi, 1);
		mutex_exit(&scd->mutex);
		return (0);
	}
	/*
	 * Mark daemon as active
	 */
	scd->active = 1;
	TRACE(T_SAM_DAE_ACTIVE, NULL, scd->active, scdi, 2);

	while (scd->size == 0) {
		if (cv_wait_sig(&scd->get_cv, &scd->mutex) == 0) {
			error = EINTR;
			mutex_exit(&scd->mutex);
			return (error);
		}
	}
	if ((scd->package == -1) || (scd->size == 0)) {
		error = ENOPKG;
		mutex_exit(&scd->mutex);
		return (error);
	}
	size = MIN(size, scd->size);
	if (size) {
		if (segflag == UIO_USERSPACE) {
			if (copyout(&scd->cmd, cmd, size)) {
				error = EFAULT;
			}
		} else {
			bcopy(&scd->cmd, cmd, size);
		}
	}
	scd->size = 0;

	cv_signal(&scd->put_cv);
	mutex_exit(&scd->mutex);

	return (error);
}


#ifdef	sun

/*
 * ----- sam_clear_cmd - clear daemon command.
 *
 * Clear sam-stagerd's daemon command when failing over.
 * If sam-stagerd was not running on the old server, a stage request
 * was not picked up by the sam-stagerd. We need to clear this pending
 * request in the daemon command queue.
 */

void
sam_clear_cmd(enum SCD_daemons scdi)
{
	struct sam_syscall_daemon *scd = &sam_scd_table[scdi];

	mutex_enter(&scd->mutex);
	cv_broadcast(&scd->put_cv);
	scd->size = 0;
	scd->package = -1;
	cv_signal(&scd->get_cv);
	mutex_exit(&scd->mutex);
}

#endif	/* sun */


/*
 * ----- sam_send_stageall_cmd - send command to the stageall daemon.
 *
 * The filesystem waits until the stageall command slot is free. Then
 * the filesystem copies the stageall id and mp into the global table.
 * Then the filesystem signals the possible waiting daemon.
 */

int					/* ERRNO if error, 0 if successful. */
sam_send_stageall_cmd(sam_node_t *ip)
{
	struct sam_syscall_daemon *scd = &sam_scd_table[SCD_stageall];

	mutex_enter(&scd->mutex);
	scd->put_wait++;
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	while (scd->size) {
		if (sam_cv_wait_sig(&scd->put_cv, &scd->mutex) == 0) {
			scd->put_wait--;
			mutex_exit(&scd->mutex);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			return (EINTR);
		}
	}
	scd->package = 1;
	scd->size = sizeof (struct sam_stageall);
	scd->cmd.stageall.id = ip->di.id;
	scd->cmd.stageall.fseq = ip->mp->mt.fi_eq;
	scd->put_wait--;
	cv_signal(&scd->get_cv);
	mutex_exit(&scd->mutex);
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	return (0);
}


#ifdef sun

/*
 * ----- sam_stageall_wait - wait for an associative staging ip.
 *
 * Wait for associative stage inode pointer. Get parent directory.
 * Change working directory to parent for stageall daemon.
 */

int			/* ERRNO if error, 0 if successful. */
sam_wait_stageall_cmd()
{
	struct sam_stageall sa;
	int error;

	if ((error = sam_wait_scd_cmd(SCD_stageall, UIO_SYSSPACE, &sa,
			sizeof (struct sam_stageall))) == 0) {
		sam_mount_t *mp;
		sam_node_t *aip, *pip;
		vnode_t *vp;

		/*
		 * Get inode.
		 */
		mutex_enter(&samgt.global_mutex);
		mp = samgt.mp_list;
		while (mp != NULL) {
			if (mp->mt.fi_eq == sa.fseq &&
				!(mp->mt.fi_status & FS_UMOUNT_IN_PROGRESS) &&
					(mp->mt.fi_status & FS_MOUNTED)) {
				break;
			}
			mp = mp->ms.m_mp_next;
		}
		mutex_exit(&samgt.global_mutex);
		if (mp == NULL) {
			return (ENOPKG);
		}

		if ((error = sam_get_ino(mp->mi.m_vfsp, IG_EXISTS,
					&sa.id, &aip))) {
			return (error);
		}

		/*
		 * Get parent inode.
		 */
		error = sam_get_ino(aip->mp->mi.m_vfsp, IG_EXISTS,
				    &aip->di.parent_id,
				&pip);
		VN_RELE(SAM_ITOV(aip));
		if (error == 0 && pip) {

			/*
			 * Change to parent directory of staged file.
			 */
			vp = PTOU(curproc)->u_cdir;
			PTOU(curproc)->u_cdir = SAM_ITOV(pip);
			VN_RELE(vp);
		}
	}
	return (error);
}

#endif /* sun */


/*
 * ----- sam_sc_daemon_init - initialize system call daemons.
 */
void
sam_sc_daemon_init(void)
{
	struct sam_syscall_daemon *scd;
	enum SCD_daemons scdi;

	for (scdi = 0; scdi < SCD_MAX; scdi++) {
		scd = &sam_scd_table[scdi];
		sam_init_daemon(scd);
	}
	sam_scd_table[SCD_fsd].timeout = SAM_SCD_TIMEOUT;
}


/*
 * ----- sam_sc_daemon_fini - terminate system call daemons.
 */
void
sam_sc_daemon_fini(void)
{
	struct sam_syscall_daemon *scd;
	enum SCD_daemons scdi;

	for (scdi = 0; scdi < SCD_MAX; scdi++) {
		scd = &sam_scd_table[scdi];
		sam_finish_daemon(scd);
	}
}


/*
 * ----- sam_init_daemon - initialize system call daemons.
 */
static void
sam_init_daemon(struct sam_syscall_daemon *scd)
{
	sam_mutex_init(&scd->mutex, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&scd->get_cv, NULL, CV_DEFAULT, NULL);
	cv_init(&scd->put_cv, NULL, CV_DEFAULT, NULL);
	scd->size = 0;
	scd->put_wait = 0;
	scd->package = -1;
	scd->active = 0;
	scd->timeout = 0;
}


/*
 * ----- sam_finish_daemon - terminate system call daemons.
 */
static void
sam_finish_daemon(struct sam_syscall_daemon *scd)
{
	mutex_enter(&scd->mutex);
	scd->package = -1;
	scd->active = 0;
	cv_signal(&scd->get_cv);
	mutex_exit(&scd->mutex);
}
