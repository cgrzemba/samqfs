/*
 * ----- samscall.c - Process the sam privileged system calls.
 *
 *	Processes the privileged system calls supported on the SAM File System.
 *	Called by the samioc system call module.
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

#pragma ident "$Revision: 1.53 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <stddef.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/ksynch.h>
#include <sys/file.h>
#include <sys/mode.h>
#include <sys/uio.h>
#include <sys/dirent.h>
#include <sys/proc.h>
#include <sys/disp.h>
#include <sys/policy.h>

/* ----- SAMFS Includes */

#include "sam/types.h"
#include "sam/resource.h"
#include "amld.h"
#include "inode.h"
#include "mount.h"
#include "sam/param.h"
#include "sam/checksum.h"
#include "sam/fioctl.h"
#include "pub/sam_errno.h"
#include "sam/mount.h"
#include "samsanergy/fsmdc.h"
#include "samsanergy/fsmdcsam.h"
#include "sam/syscall.h"
#include "sam/samevent.h"

#include "global.h"
#include "fsdaemon.h"
#include "ino.h"
#include "inode.h"
#include "ino_ext.h"
#include "ioblk.h"
#include "arfind.h"
#include "extern.h"
#include "rwio.h"
#include "syslogerr.h"
#include "debug.h"
#include "trace.h"

extern struct vnodeops samfs_vnodeops;
extern char hw_serial[];

/* External prototypes. */
void sam_sync_amld(sam_resync_arg_t *args);
int sam_get_amld_cmd(char *);
int sam_get_amld_state(int *pid);


/* Private functions. */
static int sam_amld_call(void *arg, int size, cred_t *credp);
static int sam_amld_quit(cred_t *credp);
static int sam_amld_resync(void *arg, int size, cred_t *credp);
static int sam_amld_stat(void *arg, int size, rval_t *rvp);
static int sam_setARSstatus(void *arg, int size, cred_t *credp);
static int sam_mount_call(void *arg, int size, cred_t *credp);
static int sam_unload_call(void *arg, int size, cred_t *credp);
static int sam_position_call(void *arg, int size, cred_t *credp);
static int sam_get_iocount(void *arg, int size, rval_t *rvp);
static int sam_inval_dev_call(void *arg, int size, cred_t *credp);
static int sam_stagealld_call(cred_t *credp);
static int sam_stagerd_call(void *arg, int size, cred_t *credp);
static int sam_stage_call(void *arg, int size, rval_t *rvp, cred_t *credp);
static int sam_stage_err_call(void *arg, int size, cred_t *credp);
static int sam_cancel_call(void *arg, int size, cred_t *credp);
static int sam_error_call(void *arg, int size, cred_t *credp);
static int sam_drop_call(void *arg, int size, cred_t *credp);
static int sam_get_san_ids_call(void *arg, int size, cred_t *credp);
static int sam_san_ops_call(void *arg, int size, cred_t *credp);
static int sam_fd_storage_ops_call(void *arg, int size, cred_t *credp);

static int syscall_stage_response(sam_mount_t *mp, sam_fsstage_arg_t *stage,
	rval_t *rvp, cred_t *credp);
static int syscall_stage_err(sam_mount_t *mp, sam_fsstage_arg_t *stage);
static int syscall_mount_response(sam_mount_t *mp, sam_fsmount_arg_t *mount);
static void syscall_unload_response(sam_mount_t *mp,
	sam_fsunload_arg_t *unload);

static void syscall_error_response(sam_mount_t *mp, sam_fserror_arg_t *error,
	cred_t *credp);
static sam_node_t *syscall_valid_ino(sam_mount_t *mp, sam_handle_t *fhandle);


/*
 * ----- sam_priv_sam_syscall - Process the sam privileged system calls.
 */

int				/* ERRNO if error, 0 if successful. */
sam_priv_sam_syscall(
	int cmd,		/* Command number. */
	void *arg,		/* Pointer to arguments. */
	int size,
	int *irvp)		/* to return a value */
{
	cred_t *credp;
	int error = 0;
	rval_t rv;

	rv.r_val1 = *irvp;
	TRACE(T_SAM_PSAMCALL, NULL, cmd, 0, 0);
	credp = CRED();
	switch (cmd) {

		/*
		 *	sam-amld daemon system call operations.
		 */
		case SC_amld_call:
			error = sam_amld_call(arg, size, credp);
			break;

		case SC_amld_quit:
			error = sam_amld_quit(credp);
			break;

		case SC_amld_resync:
			error = sam_amld_resync(arg, size, credp);
			break;

		case SC_amld_stat:
			error = sam_amld_stat(arg, size, &rv);
			break;

		/*
		 *	Set SAM status.
		 */
		case SC_setARSstatus:
			error = sam_setARSstatus(arg, size, credp);
			break;

		/*
		 *	Daemon removable media options.
		 */
		case SC_fsmount:
			error = sam_mount_call(arg, size, credp);
			break;

		case SC_fsunload:
			error = sam_unload_call(arg, size, credp);
			break;

		case SC_position:
			error = sam_position_call(arg, size, credp);
			break;

		case SC_fsiocount:
			error = sam_get_iocount(arg, size, &rv);
			break;

		case SC_fsinval:
			error = sam_inval_dev_call(arg, size, credp);
			break;

		/*
		 *	Daemon archiver daemon.
		 */
		case SC_arfind:
			error = sam_arfind_call(arg, size, credp);
			break;

		/*
		 *	Event daemon.
		 */
		case SC_event_open:
			error = sam_event_open(arg, size, credp);
			break;

		/*
		 *	Daemon stage options.
		 */
		case SC_stageall:
			error = sam_stagealld_call(credp);
			break;

		case SC_stager:
			error = sam_stagerd_call(arg, size, credp);
			break;

		case SC_fsstage:
			error = sam_stage_call(arg, size, &rv, credp);
			break;

		case SC_fsstage_err:
			error = sam_stage_err_call(arg, size, credp);
			break;

		case SC_fscancel:
			error = sam_cancel_call(arg, size, credp);
			break;

		case SC_fserror:
			error = sam_error_call(arg, size, credp);
			break;

		/*
		 *	Daemon miscellaneous calls.
		 */
		case SC_fsdropds:
			error = sam_drop_call(arg, size, credp);
			break;

		/*
		 *	SANergy operations.
		 */
		case SC_get_san_ids:
			error = sam_get_san_ids_call(arg, size, credp);
			break;

		case SC_san_ops:
			error = sam_san_ops_call(arg, size, credp);
			break;

		/*
		 * For support of sparse archiving
		 */
		case SC_store_ops:
			error = sam_fd_storage_ops_call(arg, size, credp);
			break;

		default:
			error = ENOTTY;
			break;

	}	/* end switch */

	*irvp = rv.r_val1;
	TRACE(T_SAM_PSAMCALL_RET, 0, cmd, error, *irvp);
	return (error);
}


/*
 * ----- sam_amld_call - Process the sam-amld system call.
 * Demon request that returns when filesystem issues a command to sam-amld.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_amld_call(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	if (size != sizeof (sam_fs_fifo_t)) {
		return (EFAULT);
	}
	if (secpolicy_fs_config(credp, NULL)) {
		return (EACCES);
	}
	return (sam_get_amld_cmd(arg));
}


/*
 * ----- sam_amld_quit - Process the sam-amld quit system call.
 */

static int		/* ERRNO if error, 0 if successful. */
sam_amld_quit(cred_t *credp)
{
	if (secpolicy_fs_config(credp, NULL)) {
		return (EACCES);
	}
	sam_shutdown_amld();
	return (0);
}


/*
 * ----- sam_amld_resync - Process the call to resync sam-amld.
 *
 * Resync sam-amld.  When sam-amld initializes it will make this
 * call to the file system to make sure that there aren't any latent
 * commands pending.  This will set samgt.amld_pid.  All commands
 * from the filesystem must have daemon_pid set to the pid
 * of the current copy of sam-amld or they will be discarded.
 */

static int		/* ERRNO if error, 0 if successful. */
sam_amld_resync(
	void *arg,	/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	sam_resync_arg_t  args;

	if (secpolicy_fs_config(credp, NULL)) {
		return (EPERM);
	}

	if ((size != sizeof (args)) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	/*
	 * send the FS_FIFO_RESYNC command to sam-amld.
	 */
	sam_sync_amld(&args);
	return (0);
}


/*
 * ----- sam_amld_stat - Process the sam-amld status system call.
 */

static int		/* ERRNO if error, 0 if successful. */
sam_amld_stat(
	void *arg,	/* Pointer to arguments. */
	int size,
	rval_t *rvp)	/* to return a value */
{
	int samamld_pid;

	rvp->r_val1 = sam_get_amld_state(&samamld_pid);
	if ((size != sizeof (int)) || copyout((caddr_t)&samamld_pid,
	    arg, sizeof (int))) {
		return (EFAULT);
	}
	return (0);
}


/*
 * ----- sam_setARSstatus - Process the end releaser call.
 */

static int		/* ERRNO if error, 0 if successful. */
sam_setARSstatus(
	void *arg,	/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	struct sam_setARSstatus_arg args;
	sam_mount_t  *mp;
	int		error;


	if (size != sizeof (args) || copyin(arg, &args, sizeof (args))) {
		return (EFAULT);
	}
	if ((mp = sam_find_filesystem(args.as_name)) == NULL) {
		return (EINVAL);
	}
	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EACCES;
		mutex_enter(&mp->ms.m_waitwr_mutex);
		goto out;
	}

	/*
	 * Set or clear status flags.
	 */
	mutex_enter(&mp->ms.m_waitwr_mutex);
	error = 0;
	switch (args.as_status) {

	case ARS_set_a:
		if (mp->mt.fi_status & FS_ARCHIVING) {
			error = EBUSY;
		} else {
			mp->mt.fi_status |= FS_ARCHIVING;
		}
		break;

	case ARS_clr_a:
		if (mp->mt.fi_status & FS_ARCHIVING) {
			mp->mt.fi_status &= ~FS_ARCHIVING;
		} else {
			error = ENXIO;
		}
		break;

	case ARS_set_r:
		if (mp->mt.fi_status & FS_RELEASING) {
			error = EBUSY;
		} else {
			mp->mt.fi_status |= FS_RELEASING;
		}
		break;

	case ARS_clr_r:
		if (mp->mt.fi_status & FS_RELEASING) {
			mp->mt.fi_status &= ~FS_RELEASING;
			mp->mi.m_release_time = SAM_SECOND() + 60;
		} else {
			error = ENXIO;
		}
		break;

	case ARS_set_s:
		if (mp->mt.fi_status & FS_STAGING) {
			error = EBUSY;
		} else {
			mp->mt.fi_status |= FS_STAGING;
		}
		break;

	case ARS_clr_s:
		if (mp->mt.fi_status & FS_STAGING) {
			mp->mt.fi_status &= ~FS_STAGING;
		} else {
			error = ENXIO;
		}
		break;

	default:
		error = EINVAL;
		break;
	}

out:
	SAM_SYSCALL_DEC(mp, 1);
	return (error);
}


/*
 * ----- sam_mount_call - Process the daemon fsmount call.
 * Daemon request to announce that media is mounted.
 */

static int		/* ERRNO if error, 0 if successful. */
sam_mount_call(
	void *arg,	/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	sam_fsmount_arg_t  args;
	sam_mount_t  *mp;
	int error;

	/*
	 * Validate and copyin the arguments.
	 */
	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	/*
	 * If the mount point is mounted, process mount request.
	 */
	if ((mp = find_mount_point(args.handle.fseq)) == NULL) {
		return (ECANCELED);
	}

	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EINVAL;
	} else {
		error = syscall_mount_response(mp, &args);
	}

	/*
	 * Decrement syscall count, the vnode count should be incremented by
	 * now.
	 */
	SAM_SYSCALL_DEC(mp, 0);

	return (error);
}


/*
 * ----- sam_unload_call - Process the daemon fsunload call.
 * Daemon response to a unload fifo command
 */

static int			/* ERRNO if error, 0 if successful. */
sam_unload_call(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	sam_fsunload_arg_t  args;
	sam_mount_t  *mp;
	int error = 0;

	/*
	 * Validate and copyin the arguments.
	 */
	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	/*
	 * If the mount point is not found or umounting.
	 */
	if ((mp = find_mount_point(args.handle.fseq)) == NULL) {
		return (ECANCELED);
	}

	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EINVAL;
	} else {
		syscall_unload_response(mp, &args);
	}

	/*
	 * Decrement syscall count, the vnode count should be incremented by
	 * now.
	 */
	SAM_SYSCALL_DEC(mp, 0);
	return (error);
}


/*
 * ----- sam_position_call - Process position response system call.
 * Daemon response to position removable media fifo command.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_position_call(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	sam_position_arg_t  args;
	sam_mount_t  *mp;
	sam_handle_t *fhandle;
	sam_fs_fifo_ctl_t *fifo_ctl;
	sam_node_t *ip;			/* pointer to rm inode */
	int error = 0;

	/* validate and copyin the arguments */
	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	/* If the mount point is not found or umounting */
	if ((mp = find_mount_point(args.handle.fseq)) == NULL) {
		return (ECANCELED);
	}

	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EINVAL;
	} else {
		fhandle = (sam_handle_t *)&args.handle;
		fifo_ctl = (sam_fs_fifo_ctl_t *)fhandle->fifo_cmd.ptr;
		if ((ip = syscall_valid_ino(mp, fhandle))) {
			TRACE(T_SAM_IOCTL_POS, SAM_ITOV(ip), ip->di.id.ino,
			    args.ret_err,
			    ip->rdev);
			/* Is this inode still waiting for the command? */
			mutex_enter(&ip->daemon_mutex);
			if (ip->daemon_busy) {
				if (fhandle->pid == ip->rm_pid) {
					if (fifo_ctl &&
					    fifo_ctl->fifo.magic ==
					    FS_FIFO_MAGIC) {
						fifo_ctl->ret_err =
						    args.ret_err;
						if (args.position != 0) {
			ip->di.rm.info.rm.position = (uint_t)args.position;
						}
					}
					ip->daemon_busy = 0;
					cv_signal(&ip->daemon_cv);
				}
			}
			mutex_exit(&ip->daemon_mutex);
			VN_RELE(SAM_ITOV(ip));
		}
	}

	/*
	 * Decrement syscall count (incremented in find_mount_point()).
	 */
	SAM_SYSCALL_DEC(mp, 0);

	return (error);
}


/*
 * ----- sam_get_iocount - Process the get direct I/O access count.
 * Return io count for direct access io
 */

static int		/* ERRNO if error, 0 if successful. */
sam_get_iocount(
	void *arg,	/* Pointer to arguments. */
	int size,
	rval_t *rvp)	/* to return a value */
{
	sam_fsiocount_arg_t  args;
	sam_mount_t  *mp;
	sam_node_t *ip;				/* pointer to rm inode */
	int error = 0;

	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	if ((mp = find_mount_point(args.handle.fseq)) == NULL) {
		return (ECANCELED);
	}
	if ((ip = syscall_valid_ino(mp, &args.handle)) != NULL) {
		rvp->r_val1 = ip->io_count;
		if (ip->rdev == 0) {
			error = ECANCELED;
		}
		VN_RELE(SAM_ITOV(ip));
	} else {
		error = ECANCELED;
	}

	SAM_SYSCALL_DEC(mp, 0);
	return (error);
}


/*
 * ----- sam_inval_dev_call - Process the invalidate device call.
 * Daemon request to invalidate cache for a devce about to be unloaded.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_inval_dev_call(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	sam_fsinval_arg_t  args;
	dev_t rdev;

	if (secpolicy_fs_config(credp, NULL)) {
		return (EINVAL);
	}

	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	rdev = expldev(args.rdev);
	bflush(rdev);
	binval(rdev);
	return (0);
}


/*
 * ----- sam_stagealld_call - Process the sam stageall system call.
 *	Demon request that returns when an associative stage is required.
 */

static int		/* ERRNO if error, 0 if successful. */
sam_stagealld_call(cred_t *credp)
{
	int error;

	if (secpolicy_fs_config(credp, NULL)) {
		error = EACCES;
	} else {
		error = sam_wait_stageall_cmd();
	}
	return (error);
}


/*
 * ----- sam_stagerd_call - Process the stager daemon system call.
 *
 * Stagerd requests:
 *		STAGER_setpid:      set sam-stagerd's pid into the global table
 *		STAGER_getrequest:  block until a stage is required
 */

static int			/* ERRNO if error, 0 if successful. */
sam_stagerd_call(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	sam_stage_arg_t args;
	void *request;

	if (secpolicy_fs_config(credp, NULL)) {
		return (EACCES);
	}

	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	switch (args.stager_cmd) {
	case STAGER_getrequest:
		request = (void *)args.p.request.p32;
		if (curproc->p_model != DATAMODEL_ILP32) {
			request = (void *)args.p.request.p64;
		}
		return (sam_wait_scd_cmd(SCD_stager, UIO_USERSPACE,
		    request, sizeof (sam_stage_request_t)));

	case STAGER_setpid:
		mutex_enter(&samgt.global_mutex);
		samgt.stagerd_pid = args.p.pid;
		mutex_exit(&samgt.global_mutex);
		return (0);

	default:
		cmn_err(CE_WARN, "SAM-QFS: Unrecognized SC_stager command (%d)",
		    (int)args.stager_cmd);
	}
	return (ENOSYS);
}


/*
 * ----- sam_stage_call - Process the daemon fsstage call.
 *
 * Daemon request to return an fd to use for staging, or to indicate
 * an error in a pending stage request.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_stage_call(
	void *arg,		/* Pointer to arguments. */
	int size,
	rval_t *rvp,		/* to return a value */
	cred_t *credp)
{
	sam_fsstage_arg_t  args;
	sam_mount_t  *mp;
	int error;

	/*
	 * Validate and copyin the arguments.
	 */
	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	/*
	 * If the mount point is not found or umounting.
	 */
	if ((mp = find_mount_point(args.handle.fseq)) == NULL) {
		return (ECANCELED);
	}

	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EINVAL;
	} else {
		error = syscall_stage_response(mp, &args, rvp, credp);
	}

	/*
	 * Decrement syscall count, (incremented in find_mount_point())
	 */
	SAM_SYSCALL_DEC(mp, 0);

	return (error);
}


/*
 * ----- sam_stage_err_call - Process the daemon fsstage_err call.
 * Daemon request to indicate an error during a stage in progress.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_stage_err_call(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	sam_fsstage_arg_t  args;
	sam_mount_t  *mp;
	int error;

	/*
	 * Validate and copyin the arguments.
	 */
	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	/*
	 * If the mount point is not found or umounting.
	 */
	if ((mp = find_mount_point(args.handle.fseq)) == NULL) {
		return (ECANCELED);
	}

	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EINVAL;
	} else {
		error = syscall_stage_err(mp, &args);
	}

	/*
	 * Decrement syscall count
	 */
	SAM_SYSCALL_DEC(mp, 0);
	return (error);
}


/*
 * ----- sam_cancel_call - Process the daemon cancel call.
 * Daemon request to cancel (timeout) a direct io job.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_cancel_call(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	sam_handle_t *fhandle;
	sam_fserror_arg_t args;
	sam_mount_t *mp;
	sam_node_t *ip;			/* pointer to rm inode */
	int error;

	/*
	 * Validate and copyin the arguments.
	 */
	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	/*
	 * If the mount point is mounted, process cancel request.
	 */
	if ((mp = find_mount_point(args.handle.fseq)) == NULL) {
		return (ECANCELED);
	}

	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EINVAL;
		goto cancelerror;
	}

	fhandle = (sam_handle_t *)&args.handle;
	if ((ip = syscall_valid_ino(mp, fhandle)) == NULL) {
		error = ECANCELED;
		goto cancelerror;
	}
	TRACE(T_SAM_DAEMON_CAN, SAM_ITOV(ip), ip->di.id.ino, ip->rdev,
	    args.ret_err);
	RW_LOCK_OS(&ip->data_rwl, RW_WRITER);	/* Wait until I/O done */
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	if (ip->rdev && (fhandle->pid == ip->rm_pid)) {
		ip->rm_err = args.ret_err;
		RW_UNLOCK_OS(&ip->data_rwl, RW_WRITER);
		error = sam_unload_rm(ip, FWRITE, 0, 0, credp);
		if (ip->rm_err == 0) {
			ip->rm_err = error;
		}
	} else {
		cmn_err(CE_NOTE,
		    "SAM-QFS: %s: sam_cancel_call:"
		    " SC_fscancel error: rdev: %d rm_pid: %d fh_pid: %d",
		    mp->mt.fi_name, (int)ip->rdev, ip->rm_pid, fhandle->pid);
		error = ECANCELED;
		RW_UNLOCK_OS(&ip->data_rwl, RW_WRITER);
	}
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	VN_RELE(SAM_ITOV(ip));

	/*
	 * Decrement syscall count, the vnode count should be incremented by
	 * now.
	 */
cancelerror:
	SAM_SYSCALL_DEC(mp, 0);
	return (error);
}


/*
 * ----- sam_error_call - Process the daemon fserror call.
 * Daemon request to report an error
 */

static int			/* ERRNO if error, 0 if successful. */
sam_error_call(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	sam_fserror_arg_t  args;
	sam_mount_t  *mp;
	int error = 0;

	/*
	 * Validate and copyin the arguments.
	 */
	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	/*
	 * If the mount point is not found or umounting.
	 */
	if ((mp = find_mount_point(args.handle.fseq)) == NULL) {
		return (ECANCELED);
	}

	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EINVAL;
	} else {
		syscall_error_response(mp, &args, credp);
	}

	SAM_SYSCALL_DEC(mp, 0);
	return (error);
}


/*
 * ----- sam_drop_call - Process the release file call.
 * drop disk space on file and return current blocks free.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_drop_call(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	sam_fsdropds_arg_t args;
	sam_mount_t *mp;
	sam_node_t *ip = NULL;			/* pointer to rm inode */
	int error;

	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	/*
	 * If the mount point is mounted, process releaser request.
	 */
	if ((mp = find_mount_point(args.fseq)) == NULL) {
		return (ECANCELED);
	}

	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EINVAL;
		goto out;
	}

	if ((error = sam_find_ino(mp->mi.m_vfsp, IG_EXISTS, &args.id, &ip))) {
		goto droperr;
	}
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);

	/*
	 * Don't release file if it is staging, archiving, or release_n set.
	 */
	if (ip->flags.b.staging || ip->arch_count) {
		error = EBUSY;
		dcmn_err((CE_NOTE,
		    "SAM-QFS: %s: Cannot release file:"
		    " %d.%d, flag=%x ac=%d pid=%d.%d.%d.%d",
		    mp->mt.fi_name, ip->di.id.ino, ip->di.id.gen,
		    ip->flags.bits, ip->arch_count, ip->arch_pid[0],
		    ip->arch_pid[1], ip->arch_pid[2], ip->arch_pid[3]));

	} else if ((args.shrink == 0) &&
	    (ip->di.status.b.nodrop || ip->di.status.b.archnodrop ||
	    ip->flags.b.accessed)) {
		error = EINVAL;

	} else {
		int bof_online;

		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		RW_LOCK_OS(&ip->data_rwl, RW_WRITER);
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (args.shrink) {
			bof_online = ip->di.status.b.bof_online;
			ip->di.status.b.bof_online = 0;
		}
		error = sam_drop_ino(ip, credp);
		if (args.shrink) {
			ip->di.status.b.bof_online = bof_online;
		}
		RW_UNLOCK_OS(&ip->data_rwl, RW_WRITER);
	}
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	sam_rele_ino(ip);

	/*
	 * Wait until all the blocks released on the list.
	 */
	mutex_enter(&mp->mi.m_inode.mutex);
	while (mp->mi.m_next != NULL || mp->mi.m_inode.busy) {
		mp->mi.m_inode.wait++;
		if (sam_cv_wait_sig(&mp->mi.m_inode.get_cv,
		    &mp->mi.m_inode.mutex) == 0) {
			mp->mi.m_inode.wait--;
			error = EINTR;
			break;
		}
		mp->mi.m_inode.wait--;
	}
	mutex_exit(&mp->mi.m_inode.mutex);

droperr:
	/*
	 * Return blks now free
	 */
	args.freeblocks = mp->mi.m_sbp->info.sb.space;
	if (size != sizeof (args) ||
	    copyout((caddr_t)&args, arg, sizeof (args))) {
		error = EFAULT;
	}

out:
	SAM_SYSCALL_DEC(mp, 0);
	return (error);
}


/*
 * ----- sam_get_san_ids_call - Process the SANergy API get file system call.
 *
 * Returns a 'volume cookie' and a 'file cookie', which together
 * constitute a handle for the file.  All file ops must be done
 * using this handle to avoid problems with renaming, deletion, etc..
 *
 * (char *path, FSVOLCOOKIE *, FSFILECOOKIE *).
 */

static int			/* ERRNO if error, 0 if successful. */
sam_get_san_ids_call(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	struct sam_get_san_ids_arg args;
	char *a_path;
	FSVOLCOOKIE *a_volcookie;
	FSFILECOOKIE *a_filecookie;

	int sam_gen_file_cookies(char *, FSVOLCOOKIE *, FSFILECOOKIE *);

	if (secpolicy_fs_config(credp, NULL)) {
		return (EPERM);
	}

#ifdef METADATA_SERVER
	if (!samgt.license.license.lic_u.b.shared_san)
		return (ENOTSUP);
#endif	/* METADATA_SERVER */

	if ((size != sizeof (args)) ||
	    copyin(arg, (caddr_t)&args, sizeof (args)))
		return (EFAULT);

	a_path = (char *)args.path.p32;
	a_volcookie = (FSVOLCOOKIE *)args.vc.p32;
	a_filecookie = (FSFILECOOKIE *)args.fc.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		a_path = (char *)args.path.p64;
		a_volcookie = (FSVOLCOOKIE *)args.vc.p64;
		a_filecookie = (FSFILECOOKIE *)args.fc.p64;
	}
	return (sam_gen_file_cookies(a_path, a_volcookie, a_filecookie));
}


/*
 * ----- sam_san_ops_call - Process the SANergy API calls.
 *
 * This includes getting the 'file map' (list of file extents),
 * (pre-)allocation, setting file size, and 'hold'ing the file
 * (to prevent files' blocks from being returned before the SAN
 * software is done writing them).
 *
 * (FSVOLCOOKIE *, FSFILECOOKIE *, flags,
 *  size, start, len, FSMAPINFO *, mapbuflen)
 */

static int			/* ERRNO if error, 0 if successful. */
sam_san_ops_call(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	struct sam_san_ops_arg args;
	FSVOLCOOKIE *a_volcookie;
	FSFILECOOKIE *a_filecookie;
	FSMAPINFO *a_mapbuf;
	offset_t a_flen, a_start, a_slen;
	FS64LONG a_flags, a_mapbuflen;
	int sam_san_ops(FSVOLCOOKIE *, FSFILECOOKIE *, FS64LONG,
	    offset_t, offset_t, offset_t, FSMAPINFO *, FS64LONG);

	if (secpolicy_fs_config(credp, NULL)) {
		return (EPERM);
	}

#ifdef METADATA_SERVER
	if (!samgt.license.license.lic_u.b.shared_san)
		return (ENOTSUP);
#endif	/* METADATA_SERVER */

	if ((size != sizeof (args)) ||
	    copyin(arg, (caddr_t)&args, sizeof (args)))
		return (EFAULT);

	a_volcookie = (FSVOLCOOKIE *)args.vc.p32;
	a_filecookie = (FSFILECOOKIE *)args.fc.p32;
	a_flags = args.flags;
	a_flen = (offset_t)args.flen;
	a_start = (offset_t)args.start;
	a_slen = (offset_t)args.slen;
	a_mapbuf = (FSMAPINFO *)args.buf.p32;
	a_mapbuflen = args.buflen;
	if (curproc->p_model != DATAMODEL_ILP32) {
		a_volcookie = (FSVOLCOOKIE *)args.vc.p64;
		a_filecookie = (FSFILECOOKIE *)args.fc.p64;
		a_mapbuf = (FSMAPINFO *)args.buf.p64;
	}
	return sam_san_ops(a_volcookie, a_filecookie, a_flags,
	    a_flen, a_start, a_slen, a_mapbuf, a_mapbuflen);
}


/*
 * ----- sam_fd_storage_ops_call - Process the SANergy API calls.
 *
 * This call is much similar to the SAN file ops above, but
 * takes a file descriptor instead of cookies, and doesn't
 * need to support any file modification or locking
 * functions.
 */
static int			/* ERRNO if error, 0 if successful. */
sam_fd_storage_ops_call(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	struct sam_fd_storage_ops_arg args;
	int a_fd;
	FSMAPINFO *a_mapbuf;
	offset_t a_flen, a_start, a_slen;
	FS64LONG a_flags, a_mapbuflen;
	int sam_fd_storage_ops(int, FS64LONG, offset_t, offset_t, offset_t,
	    FSMAPINFO *, FS64LONG);

	if (secpolicy_fs_config(credp, NULL)) {
		return (EPERM);
	}

	if ((size != sizeof (args)) ||
	    copyin(arg, (caddr_t)&args, sizeof (args)))
		return (EFAULT);

	a_fd = args.fd;
	a_flags = args.flags;
	a_flen = (offset_t)args.flen;
	a_start = (offset_t)args.start;
	a_slen = (offset_t)args.slen;
	a_mapbuf = (FSMAPINFO *)args.buf.p32;
	a_mapbuflen = args.buflen;
	if (curproc->p_model != DATAMODEL_ILP32) {
		a_mapbuf = (FSMAPINFO *)args.buf.p64;
	}
	return sam_fd_storage_ops(a_fd, a_flags, a_flen, a_start, a_slen,
	    a_mapbuf, a_mapbuflen);
}


/*
 * ----- syscall_stage_response - process stage response.
 *
 * Returns -1 for error with errno set, else returns a file descriptor
 * to use for stage daemon.  The stage close will occur here when an
 * fd of -1 and ECANCELED is returned.
 * Signal thread (if any) waiting on this inode.
 */

static int				/* ERRNO if error, 0 if ok */
syscall_stage_response(
	sam_mount_t *mp,		/* pointer to mount entry. */
	sam_fsstage_arg_t *stage,	/* pointer to syscall stage response */
	rval_t *rvp,			/* returned value pointer */
	cred_t *credp)			/* credentials pointer. */
{
	sam_handle_t *fhandle;
	sam_node_t *ip;
	vnode_t *vp;
	int	error;
	int opened = 0;

	rvp->r_val1 = 0;

	fhandle = (sam_handle_t *)&stage->handle;

	/* Is this inode still waiting for the stage? */
	if ((error = sam_find_ino(mp->mi.m_vfsp, IG_EXISTS,
	    &fhandle->id, &ip))) {
		return (ECANCELED);
	}
	vp = SAM_ITOV(ip);
	if (stage->ret_err == EEXIST) {
		/*
		 * Outstanding stage request already exists, wake up anyone
		 * waiting
		 */
		mutex_enter(&ip->rm_mutex);
		if (ip->rm_wait)  cv_broadcast(&ip->rm_cv);
		mutex_exit(&ip->rm_mutex);

		sam_rele_ino(ip);
		return (0);
	}
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	if (ip->stage_pid > 0) {
		/*
		 * Another stage is in progress. This can happen if the incore
		 * inode is released after a nowait stage for copy 1, then stage
		 * for copy 2.
		 */
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		sam_rele_ino(ip);
		return (ECANCELED);
	}
	ip->flags.b.staging = 1;	/* Reset for acquired incore inode */
	ip->copy = 0;
	ip->stage_off = fhandle->stage_off;
	ip->stage_len = fhandle->stage_len;
	if (stage->ret_err == 0) {
		if ((!ip->di.status.b.offline &&
		    !(ip->di.rm.ui.flags & RM_DATA_VERIFY)) ||
		    !ip->di.arch_status ||
		    ip->stage_err) {
			error = ECANCELED;
			TRACE(T_SAM_IOCTL_STCAN, vp,
			    fhandle->id.ino, ip->stage_err,
			    ip->flags.bits);
		} else {
			offset_t cur_size = ip->size;

			error = 0;
			if (!fhandle->flags.b.stage_wait) {
				/*
				 * Allocate file now for nowait stage.
				 *
				 * Assert
				 * permissions based on the uid of the handle
				 * structure, not the stager.  Root can force
				 * staging, but users have to live with quotas.
				 */
				if (ip->di.blocks == 0) {
					error =
					    sam_set_unit(ip->mp, &(ip->di));
				}
				if (error == 0) {
					error = sam_map_block(ip, ip->stage_off,
					    ip->stage_len,
					    SAM_WRITE_BLOCK, NULL, credp);
					ip->size = cur_size;
				}
			}

			if (error == 0) {
				/*
				 * Simulate the open because we don't
				 * have the path.
				 */
				if ((error = sam_get_fd(vp,
				    &rvp->r_val1)) == 0) {
					if (vp->v_type == VREG) {
						atomic_add_32(&vp->v_rdcnt, 1);
						atomic_add_32(&vp->v_wrcnt, 1);
					}
					ip->stage_pid = SAM_CUR_PID;
					ip->no_opens++;
					opened = 1;
					if (stage->directio) {
						sam_set_directio(ip,
						    DIRECTIO_ON);
					}
				}
			}
			if (error) {
				ip->stage_err = (short)error;
				TRACE(T_SAM_IOCTL_SERROR, vp, fhandle->id.ino,
				    (sam_tr_t)ip->size, error);
				error = ECANCELED;
			} else {
				TRACE(T_SAM_IOCTL_STAGE, vp, fhandle->id.ino,
				    (sam_tr_t)ip->stage_off,
				    (uint_t)ip->stage_len);
			}
		}

		/*
		 * Issue close to clean up the inode. Stage daemon won't get an
		 * fd.
		 */
		if (error) {
			(void) sam_close_stage(ip, credp);
			/* tell stage daemon a close not needed */
			rvp->r_val1 = -1;
		}
	} else {
		/*
		 * Set stage_err from stagerd..
		 */
		ip->stage_err = stage->ret_err;
		TRACE(T_SAM_IOCTL_STERR, vp, fhandle->id.ino, fhandle->id.gen,
		    stage->ret_err);

		/*
		 * Issue close to clean up the inode. Daemon knows not to close.
		 */
		(void) sam_close_stage(ip, credp);
		error = 0;
	}
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	if (opened) {
		VN_RELE(SAM_ITOV(ip));
	} else {
		sam_rele_ino(ip);
	}
	return (error);
}


/*
 * ----- syscall_stage_err - process stage error.
 */

static int				/* ERRNO if error, 0 if ok */
syscall_stage_err(
	sam_mount_t *mp,		/* pointer to mount entry. */
	sam_fsstage_arg_t *stage)	/* pointer to syscall stage response */
{
	sam_handle_t *fhandle;
	sam_node_t *ip;
	vnode_t *vp;
	int error;

	fhandle = (sam_handle_t *)&stage->handle;

	/* Is this inode still waiting for the stage? */
	if ((error = sam_find_ino(mp->mi.m_vfsp, IG_EXISTS,
	    &fhandle->id, &ip))) {
		return (ECANCELED);
	}
	vp = SAM_ITOV(ip);
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	ip->flags.b.staging = 1;	/* Reset for acquired incore inode */
	ip->copy = 0;
	error = 0;
	ip->stage_err = stage->ret_err;
	TRACE(T_SAM_IOCTL_STERR, vp, fhandle->id.ino, fhandle->id.gen,
	    stage->ret_err);

	/*
	 * No need to close in this case, the stage daemon will be closing
	 * the fd real soon.
	 */
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	sam_rele_ino(ip);
	return (error);
}


/*
 * ----- syscall_mount_response - process mount response.
 *
 * Return -1 if error with errno set, else return fd for mount daemon's
 * use.  Signal thread (if any) that the mount is ready.
 */

static int				/* ERRNO if error, 0 if ok */
syscall_mount_response(
	sam_mount_t *mp,		/* pointer to mount entry. */
	sam_fsmount_arg_t *mount)	/* pointer to daemon mount response */
{
	sam_handle_t *fhandle;
	sam_node_t *ip;			/* pointer to rm inode */
	vnode_t	*vp;
	int	error = 0;
	sam_fs_fifo_ctl_t *fifo_ctl;
	dev_t rdev;

	fhandle = (sam_handle_t *)&mount->handle;
	fifo_ctl = (sam_fs_fifo_ctl_t *)fhandle->fifo_cmd.ptr;
	if ((ip = syscall_valid_ino(mp, fhandle)) == NULL) {
			return (ECANCELED);
	}
	vp = SAM_ITOV(ip);
	mutex_enter(&ip->daemon_mutex);

	/*
	 * Is inode still waiting for the mount?
	 */
	if (ip->daemon_busy && ip->seqno == fhandle->seqno) {
		if (fifo_ctl && (fifo_ctl->fifo.magic == FS_FIFO_MAGIC)) {
			fifo_ctl->ret_err = mount->ret_err;
			rdev = expldev(mount->rdev);
			if ((mount->ret_err == 0) && rdev) {
				offset_t size;
				sam_resource_t *resource;
				offset_t section_size = 0;

				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				/*
				 * Get the rm_info record. Preserve the file
				 * size, since it includes the size of all VSNs.
				 * Use size from rm_info record when reading
				 * existing optical disk file (rm_info not
				 * valid).
				 */
				size = ip->di.rm.size;

				resource =
				    (sam_resource_t *)mount->resource.p32;
				if (curproc->p_model != DATAMODEL_ILP32) {
					resource =
					    (sam_resource_t *)
					    mount->resource.p64;
				}
				if (copyin((char *)&resource->archive.rm_info,
				    (char *)&ip->di.rm,
				    sizeof (sam_arch_rminfo_t))) {
					error = EFAULT;
				} else {
					/*
					 * If the bof was written, or the
					 * rm_info was not valid, get all the
					 * resource record.
					 */
					if ((ip->di.rm.ui.flags &
					    RM_BOF_WRITTEN) ||
					    !(ip->di.rm.ui.flags & RM_VALID)) {
						sam_archive_t *ap;

			ap = &fifo_ctl->fifo.param.fs_load.resource.archive;
						copyin(
						    (char *)&resource->archive,
						    (char *)ap,
						    sizeof (sam_archive_t));
						ap->rm_info.valid = 1;
					}
					/*
					 * Preserve size of file for volume
					 * overflow. Do not change size when
					 * reading existing optical disk file.
					 */
					if ((ip->di.rm.ui.flags &
					    RM_BOF_WRITTEN) ||
					    (ip->di.rm.ui.flags & RM_VALID)) {
						section_size = ip->di.rm.size;
						ip->di.rm.size = size;
					}
					/* Removable info valid */
					ip->di.rm.ui.flags |= RM_VALID;

					/*
					 * Set "space" from mount arg for writes
					 * and tape.  Set from section size of
					 * the file when reading from optical
					 * disk.
					 */
					ip->space = mount->space;
					if ((ip->di.rm.media &
					    DT_CLASS_MASK) == DT_OPTICAL) {
						char access;

						copyin(
						    (char *)&resource->access,
						    (char *)&access,
						    sizeof (char));
						if (access == FREAD) {
							ip->space =
							    section_size;
						}
					}
					ip->rdev = rdev;
					ip->mt_handle =
					    (void *)mount->mt_handle.p32;
					if (curproc->p_model !=
					    DATAMODEL_ILP32) {
				ip->mt_handle = (void *)mount->mt_handle.p64;
					}
					ip->io_count = 0;
					TRACE(T_SAM_IOCTL_MNT, vp,
					    ip->di.id.ino, (sam_tr_t)size,
					    ip->rdev);

					if (!(ip->di.rm.ui.flags &
					    RM_STRANGER) &&
					    (ip->di.rm.info.rm.mau == 0)) {
						error = EINVAL;
						fifo_ctl->ret_err = EINVAL;
					}

					if (!(ip->di.rm.ui.flags &
					    RM_BOF_WRITTEN)) {
						if ((ip->di.rm.media &
						    DT_CLASS_MASK) ==
						    DT_OPTICAL) {


				ip->stage_off =	ip->di.rm.info.rm.position *
				    ip->di.rm.info.rm.mau +
				    ip->di.rm.info.rm.file_offset * 512;


						}
					}
				}
				ip->size = ip->di.rm.size;
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			} else {
				if (mount->ret_err == 0) {
					/* No error with rdev == 0 */
					fifo_ctl->ret_err = ENODEV;
				}
				TRACE(T_SAM_IOCTL_EMNT, vp,
				    ip->di.id.ino, 1, mount->ret_err);
			}
		} else {
			if (mount->ret_err == 0)  error = ECANCELED;
			TRACE(T_SAM_IOCTL_EMNT, vp,
			    ip->di.id.ino, 2, mount->ret_err);
		}
		/* Signal 1 thread waiting on mount. */
		ip->daemon_busy = 0;
		cv_signal(&ip->daemon_cv);
	} else {
		if (mount->ret_err == 0)  error = ECANCELED;
		TRACE(T_SAM_IOCTL_EMNT, vp, ip->di.id.ino, 3, mount->ret_err);
	}
	mutex_exit(&ip->daemon_mutex);
	VN_RELE(vp);
	return (error);
}


/*
 * ----- syscall_unload_response - A device is about to unload.
 * Return error in fifo command. Signal thread waiting on this inode.
 */

static void
syscall_unload_response(
	sam_mount_t *mp,		/* pointer to mount entry. */
	sam_fsunload_arg_t *unload)	/* pointer to daemon unload response */
{
	sam_handle_t *fhandle;
	sam_fs_fifo_ctl_t *fifo_ctl;
	sam_node_t *ip;			/* pointer to rm inode */

	fhandle = (sam_handle_t *)&unload->handle;
	fifo_ctl = (sam_fs_fifo_ctl_t *)fhandle->fifo_cmd.ptr;
	if ((ip = syscall_valid_ino(mp, fhandle))) {
		TRACE(T_SAM_IOCTL_UL, SAM_ITOV(ip), ip->di.id.ino,
		    unload->ret_err,
		    ip->rdev);

		/*
		 * Is this inode still waiting for the unload command? Is this
		 * the same pid?
		 */
		mutex_enter(&ip->daemon_mutex);
		if (ip->daemon_busy) {
			if (fhandle->pid == ip->rm_pid) {
				if (fifo_ctl && fifo_ctl->fifo.magic ==
				    FS_FIFO_MAGIC) {
					fifo_ctl->ret_err = unload->ret_err;
					if (unload->position != 0) {
						ip->di.rm.info.rm.position =
						    unload->position;
					}
				}
				ip->daemon_busy = 0;
				cv_signal(&ip->daemon_cv);
			} else {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: syscall_unload_response "
				    "error: "
				    " rdev: %d rm_pid: %d fh_pid: %d",
				    mp->mt.fi_name, (int)ip->rdev, ip->rm_pid,
				    fhandle->pid);
			}
		}
		mutex_exit(&ip->daemon_mutex);
		VN_RELE(SAM_ITOV(ip));
	}
}


/*
 * ----- syscall_error_response - response for general error.
 * Response for general error.  Signal thread waiting on this inode.
 */

static void
syscall_error_response(
	sam_mount_t *mp,		/* pointer to mount entry. */
	sam_fserror_arg_t *error,	/* pointer to daemon error response */
	cred_t *credp)			/* credentials pointer. */
{
	sam_handle_t *fhandle;
	sam_fs_fifo_ctl_t *fifo_ctl;
	sam_node_t *ip;			/* pointer to rm inode */

	fhandle = (sam_handle_t *)&error->handle;
	fifo_ctl = (sam_fs_fifo_ctl_t *)fhandle->fifo_cmd.ptr;
	if ((ip = syscall_valid_ino(mp, fhandle))) {
		TRACE(T_SAM_IOCTL_ERR, SAM_ITOV(ip), ip->di.id.ino,
		    error->ret_err,
		    ip->rdev);
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		mutex_enter(&ip->daemon_mutex);
		if (ip->daemon_busy) {	/* ino waiting for mount? */
			if (fhandle->pid == ip->rm_pid) {
				if (fifo_ctl && (fifo_ctl->fifo.magic ==
				    FS_FIFO_MAGIC)) {
					fifo_ctl->ret_err = error->ret_err;
				}
				ip->daemon_busy = 0;
				cv_signal(&ip->daemon_cv);
			} else {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: syscall_error_response error:"
				    " rdev: %d rm_pid: %d fh_pid: %d",
				    mp->mt.fi_name, (int)ip->rdev, ip->rm_pid,
				    fhandle->pid);
			}
		}
		mutex_exit(&ip->daemon_mutex);

		if (ip->flags.b.staging) {		/* ino is staging? */
			ip->stage_err = error->ret_err;
			(void) sam_close_stage(ip, credp);
		}
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		VN_RELE(SAM_ITOV(ip));
	}
}


/*
 * ----- syscall_valid_ino - verify returned ino is valid.
 *
 *	Called to verify the inode returned in the file handle
 *	is in the active hash chain. Verify file has not been removed
 *	and is another file by checking generation number.
 */

static sam_node_t *
syscall_valid_ino(
	sam_mount_t *mp,	/* pointer to mount entry. */
	sam_handle_t *fhandle)	/* file handle pointer. */
{
	sam_node_t *ip;		/* pointer to rm inode */

	/*
	 * Check to see if inode is in hash chain.
	 */
	if (mp->mt.fi_eq != fhandle->fseq) {
		ip = NULL;
	} else {
		(void) sam_check_cache(&fhandle->id, mp, IG_EXISTS, NULL, &ip);
	}
	return (ip);
}
