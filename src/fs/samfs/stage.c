/*
 * ----- stage.c - Process the sam stage functions.
 *
 *	Processes the stage functions supported on the SAM File System.
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
#pragma ident "$Revision: 1.132 $"
#endif

#include "sam/osversion.h"

/* ----- UNIX Includes */

#ifdef sun
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/t_lock.h>
#include <sys/cred.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/ddi.h>
#include <sys/disp.h>
#include <sys/param.h>
#include <sys/mount.h>
#endif /* sun */

#ifdef  linux
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/version.h>

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/file.h>
#include <linux/uio.h>
#include <linux/dirent.h>
#include <linux/proc_fs.h>
#include <linux/limits.h>
#include <linux/skbuff.h>
#include <linux/ip.h>

#include <asm/param.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#endif /* linux */


/* ----- SAMFS Includes */

#include "sam/types.h"
#include "sam/resource.h"
#include "pub/sam_errno.h"
#include <pub/stat.h>

#include "inode.h"
#include "mount.h"
#include "ino_ext.h"
#ifdef sun
#include "extern.h"
#include "macros_solaris.h"
#endif /* sun */
#ifdef linux
#include "clextern.h"
#include "macros_linux.h"
#endif /* linux */
#include "arfind.h"
#include "fsdaemon.h"
#include "rwio.h"
#include "debug.h"
#include "syslogerr.h"
#include "trace.h"
#ifdef sun
#include "kstats.h"
#endif /* sun */
#ifdef linux
#include "kstats_linux.h"
#endif /* linux */


/*
 * ----- sam_read_proc_offline - process offline file.
 *
 *	Called to check the status of a file. If the file is offline and
 *	not staging, serialize the stage so only one stage is done.
 *  NOTE. inode_rwl readers lock set on entry and exit.
 */

int				/* ERRNO if error, 0 if successful. */
sam_read_proc_offline(
	sam_node_t *ip,		/* Pointer to inode. */
	sam_node_t *bip,	/* Pointer to index segment inode (if one). */
	uio_t *uiop,		/* user I/O vector array. */
	cred_t *credp,		/* Credentials pointer */
	int *stage_n_set)
{
	krw_t rw_type;
	offset_t window, offset;
	int error = 0;

	*stage_n_set = 0;
	rw_type = RW_READER;

	/*
	 * For stage_n, must hold writers lock when checking window.
	 * stage_n_count is incremented during the I/O.
	 */
	if (ip->flags.b.stage_n) {
		if ((RW_TRYUPGRADE_OS(&ip->inode_rwl) == 0)) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
		rw_type = RW_WRITER;
		offset = uiop->uio_loffset;
		window = (offset + (offset_t)uiop->uio_resid);
		if ((window != 0) && (offset >= ip->real_stage_off) &&
		    (window <= (ip->real_stage_off + ip->real_stage_len))) {
				/*
				 * If the requested amount is in the stage_n
				 * section and it is a wait stage request, do
				 * not stage the window.
				 */
			mutex_enter(&ip->rm_mutex);
			ip->stage_n_count++;
			mutex_exit(&ip->rm_mutex);
			*stage_n_set = 1;
		}

	/*
	 * For segment access, spin stages ahead by stage_ahead.
	 */
	} else if (S_ISSEGS(&ip->di) && bip->di.stage_ahead) {
		sam_node_t *sip;
		offset_t segment_size, fileoff;
		int first = ip->di.rm.info.dk.seg.ord;
		int ord;

		segment_size = SAM_SEGSIZE(bip->di.rm.info.dk.seg_size);
		for (ord = first; ord < first + bip->di.stage_ahead + 1;
		    ord++) {
			fileoff = ord * segment_size;
			if (fileoff >= bip->di.rm.size) { /* If past EOF */
				break;
			}
			if (ord != first) {
				RW_LOCK_OS(&bip->inode_rwl, RW_READER);
				if ((error = sam_get_segment_ino(bip, ord,
				    &sip)) != 0) {
					break;
				}
				RW_UNLOCK_OS(&bip->inode_rwl, RW_READER);
				/*
				 * If inode was read from disk, pointer to the
				 * index segment must be initialized.
				 */
				if (sip->segment_ip == NULL) {
					sip->segment_ip = bip;
				}
				RW_LOCK_OS(&sip->inode_rwl, RW_READER);
			} else {
				sip = ip;
			}
			(void) sam_proc_offline(sip, (offset_t)0,
			    (offset_t)0, NULL, credp,
			    NULL);
			if (ord != first) {
				RW_UNLOCK_OS(&sip->inode_rwl, RW_READER);
				VN_RELE_OS(sip);
			}
		}
	}
	if (*stage_n_set == 0) {
		if (SAM_THREAD_IS_NFS()) {
			if ((error = sam_proc_offline(ip, uiop->uio_loffset,
			    (offset_t)uiop->uio_resid, NULL, credp,
			    NULL)) != 0) {
				goto ret_error;
			}
		} else {
			/*
			 * A stage with nowait may already be in progress for
			 * the requested file, in which case an ENOSPC error
			 * could get returned.  We really want to wait for the
			 * file on read, so reissue the stage with wait if
			 * ENOSPC is returned.
			 */
			mutex_enter(&ip->fl_mutex);
			/* wait for disk space on ENOSPC */
			ip->flags.b.nowaitspc = 0;
			mutex_exit(&ip->fl_mutex);
			while ((error = sam_proc_offline(ip, uiop->uio_loffset,
			    (offset_t)uiop->uio_resid, NULL, credp,
			    NULL)) == ENOSPC) {
				mutex_enter(&ip->fl_mutex);
				ip->flags.b.nowaitspc = 0;
				mutex_exit(&ip->fl_mutex);
			}
			if (error)
				goto ret_error;
		}
		if (ip->flags.b.stage_n) {
			mutex_enter(&ip->rm_mutex);
			ip->stage_n_count++;
			mutex_exit(&ip->rm_mutex);
			*stage_n_set = 1;
		}
	}
	error = 0;

ret_error:
	if (rw_type == RW_WRITER) {
		RW_DOWNGRADE_OS(&ip->inode_rwl);
	}
	return (error);
}


#ifdef sun
/*
 * ----- sam_stage_n_io_done - process I/O completion for stage_n file.
 *
 * Decrement number of outstanding I/O's for the current stage_n window.
 * If any threads waiting on getting a window, wake them up.
 */

void
sam_stage_n_io_done(sam_node_t *ip)
{
	mutex_enter(&ip->rm_mutex);
	ip->stage_n_count--;
	if (ip->rm_wait)  cv_broadcast(&ip->rm_cv);
	mutex_exit(&ip->rm_mutex);
}

/*
 * Switch to allow/disallow blocking while waiting on a stage.
 * sam_getpage will request 'dontblock' so it can signal VM
 * to drop locks and try again instead of blocking while
 * holding the as_lock.  This avoids processes backing up on
 * this lock but introduces the overhead of the retry.
 * This switch is for sites preferring blocked processes over
 * the overhead of the retrys.
 */
int	sam_dontblock = 1;
#endif /* sun */

/*
 * ----- sam_proc_offline - process offline file.
 *
 *	Called to check the status of a file. If the file is offline and
 *	not staging, serialize the stage so only one stage is done.
 *  NOTE. inode_rwl lock set on entry and exit.
 */

int				/* ERRNO if error, 0 if successful. */
sam_proc_offline(
	sam_node_t *ip,		/* Pointer to vnode. */
	offset_t offset,	/* file offset */
	offset_t length,	/* requested length */
	offset_t *size,		/* Size of file--returned if not NULL */
	cred_t *credp,		/* Credentials pointer */
	int *dontblock)		/* in:non-null selects non-blocking, */
				/*   out: EAGAIN or zero */
{
	int error;
	krw_t rw_type;
	offset_t window;

	error = 0;
#ifdef	sun
	if (dontblock != NULL) {
		*dontblock = 0;
	}
#endif
	/*
	 * Cannot stage read-only filesystem, or read-only shared client
	 * without SAM on the metadata server.
	 */
	if (ip->mp->mt.fi_mflag & MS_RDONLY) {
		if (SAM_IS_SHARED_CLIENT(ip->mp)) {
			if (!SAM_IS_SHARED_SAM_FS(ip->mp)) {
				return (EROFS);
			}
		} else {
			return (EROFS);
		}
	}
	if (size != NULL) {
		*size = ip->di.rm.size;
	}

	/* Must have inode_rwl WRITER lock */
	if (RW_OWNER_OS(&ip->inode_rwl) != curthread) {
		rw_type = RW_READER;
		if ((RW_TRYUPGRADE_OS(&ip->inode_rwl) == 0)) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
	} else {
		rw_type = RW_WRITER;
	}
	if (!ip->di.status.b.offline) {
		goto out;	/* If already online */
	} else if (S_ISLNK(ip->di.mode) &&
		(ip->di.version >= SAM_INODE_VERS_2)) {
		/* no offline data for v2 symlinks */
		ip->di.status.b.offline = 0;
		goto out;
	}
	if ((error = sam_quota_fonline(ip->mp, ip, credp)) != 0) {
		goto out;
	}

	/*
	 * Process file stage.
	 */
stage_file:
	if (ip->di.status.b.cs_use && ip->di.status.b.cs_val &&
			ip->flags.b.nowaitspc == 0) {
		length = ip->di.rm.size;
	}
	window = (offset + (offset_t)length);
	if (window > ip->di.rm.size) {
		window = ip->di.rm.size;
	}
	if (ip->flags.b.staging == 0) {		/* If NOT staging */
		if ((ip->stage_pid > 0) && ip->stage_err == ECANCELED) {
			error = ECANCELED;
			goto out;
		}
		ip->stage_size = 0;
		ip->stage_err = 0;
		if (ip->flags.b.stage_p) {	/* If stage in partial */
			ip->stage_off = 0;
			ip->stage_len = sam_partial_size(ip);
		} else if ((window != 0) && ((window <= ip->size) &&
				(window <= ip->mp->mt.fi_partial_stage) &&
				(ip->di.status.b.pextents))) {
			/*
			 * If the requested amount is in the partial on-line
			 * section and it is a wait stage request, do not stage
			 * the file.
			 */
			SAM_COUNT64(sam, stage_partial);
			goto out;
		} else if (ip->flags.b.stage_n && !S_ISSEGI(&ip->di)) {
			/* If direct access -- stage never */
			if ((window != 0) && (offset >= ip->real_stage_off) &&
				(window <= (ip->real_stage_off +
				ip->real_stage_len))) {
				/*
				 * If the requested amount is in the stage_n
				 * window and it is a wait stage request, do not
				 * stage the window.
				 */
				SAM_COUNT64(sam, stage_window);
				goto out;
			}
			/*
			 * Wait if buffer is still busy. Then check for staging
			 * already started because multiple process can exit
			 * while loop.
			 */
			while (ip->stage_n_count) {
				if ((error = sam_wait_rm(ip,
							SAM_STAGING)) != 0) {
					goto out;
				}
			}
			if (ip->flags.b.staging) {
				goto stage_file;
			}
			ip->real_stage_off = 0;
			ip->real_stage_len = 0;
			ip->stage_off = offset;
			ip->stage_len = length;
		} else {
			ip->stage_off = 0;
			ip->stage_len = ip->di.rm.size;
			if (S_ISREQ(ip->di.mode)) {
				ip->stage_len = (offset_t)ip->di.psize.rmfile;
			} else if (S_ISSEGI(&ip->di)) {
				ip->stage_len = ip->di.rm.info.dk.seg.fsize;
			}
		}
		ip->flags.b.staging = 1;
		ip->di.status.b.stage_failed = 0;
#ifdef sun
		ip->stage_thr = curthread;
		ip->flush_off = 0;		/* Flush behind stage offset */
		ip->flush_len = 0;		/* Flush behind stage length */
#endif

		SAM_COUNT64(sam, stage_start);
		/*
		 * If Shared client, issue stage on the server.
		 */
		if (SAM_IS_SHARED_CLIENT(ip->mp)) {
			sam_inode_stage_t stage;

			ip->flags.b.stage_n = 0;
			stage.copy = ip->copy;
			stage.len = ip->flags.b.stage_p ? ip->stage_len : 0;
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			error = sam_proc_inode(ip, INODE_stage, &stage, credp);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		} else {
#ifdef METADATA_SERVER
			if (SAM_IS_SHARED_SERVER(ip->mp)) {
				/*
				 * If Shared server, setup stage & get stage
				 * lease.
				 */
				sam_setup_stage(ip, credp);
			}
			error = sam_stagerd_file(ip, length, credp);
#else
			error = EINVAL;
#endif
#ifdef	sun
			/*
			 * If shared fs & error is EREMCHG, freeze stage except
			 * if the stage is on the shared file system thread.
			 * Note, stage_err is set to EREMCHG when the daemon is
			 * stopped.
			 */
			if (SAM_IS_SHARED_FS(ip->mp) &&
					((error == EREMCHG) ||
					(ip->stage_err == EREMCHG))) {
				if (IS_SHAREFS_THREAD_OS) {
					ip->stage_err = EREMCHG;
					error = 0;
					goto out;
				}

				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				error = sam_idle_operation(ip);
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				if (error == 0) {
					ip->flags.b.staging = 0;
					ip->stage_thr = 0;
					/*
					 * Return EREMCHG if thread is
					 * non-blocking and a failover is
					 * in progress.
					 */
					if (ip->mp->mt.fi_status &
					    (FS_LOCK_HARD |
					    FS_UMOUNT_IN_PROGRESS)) {
						ip->stage_err = EREMCHG;
						error = EREMCHG;
						goto out;
					}
					SAM_COUNT64(sam, stage_reissue);
					goto stage_file;
				}
			}
#endif	/* sun */
		}

		if (error) {
			SAM_COUNT64(sam, stage_errors);
			ip->stage_err = (short)error;
			ip->flags.b.staging = 0;
			ip->di.status.b.stage_failed = 1;
			ip->flags.b.stage_n = ip->di.status.b.direct;
			ip->flags.b.stage_all = ip->di.status.b.stage_all;
			mutex_enter(&ip->rm_mutex);
			if (ip->rm_wait) {
				cv_broadcast(&ip->rm_cv);
			}
			mutex_exit(&ip->rm_mutex);
			goto out;
		}
	}
	/*
	 * Stage daemon has responded to stage request if stage_pid != 0.
	 * sam_proc_offline can be called by the stage daemon from sam_getpage.
	 */
	if (ip->flags.b.staging && (ip->stage_pid > 0) &&
				(ip->stage_pid == SAM_CUR_PID)) {
		if (size != NULL) {
			*size = ip->di.rm.size;
		}
	} else {
		/*
		 * While staging for read behind stage, wait for window to
		 * stage.
		 */
		while (ip->flags.b.staging && window) {

			if (ip->flags.b.stage_n) {
				if ((offset >= ip->stage_n_off) &&
				    (window <= ip->stage_size)) {
					break;
				}
			} else if (window <= sam_partial_size(ip) &&
						(ip->di.status.b.pextents)) {
				/*
				 * If the requested amount is in the partial
				 * on-line section, go complete the request.
				 */
				goto out;
			} else {
				/*
				 * Enough has staged to handle the current
				 * request, go complete the request.
				 */
				if (window <= ip->stage_size) {
					break;
				}
			}
#ifdef sun
			if (dontblock != NULL && sam_dontblock) {
				if (sam_check_sig()) {
					error = EINTR;
				} else {
					*dontblock = EAGAIN;
				}
				goto out;
			}
#endif /* sun */
			if ((error = sam_wait_rm(ip, SAM_STAGING)) != 0) {
				if ((error == EINTR) && (ip->rm_wait == 0)) {
					/*
					 * Cancel stage if SIGINT or SIGKILL
					 * signal.
					 */
#ifdef sun
					if (((ttolwp(curthread))->lwp_cursig ==
						SIGINT) ||
					    ((ttolwp(curthread))->lwp_cursig ==
					    SIGKILL)) {
						(void) sam_cancel_stage(ip,
							EINTR, credp);
					}
#endif /* sun */
#ifdef linux
					if (signal_pending(current)) {
#if defined(RHE_LINUX) || (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
						spin_lock_irq(
						    &current->sighand->siglock);
#else
						spin_lock_irq(
						    &current->sig->siglock);
#endif /* RHE_LINUX */
						if (sigismember(
						    &current->pending.signal,
						    SIGINT) ||
						    sigismember(
						    &current->pending.signal,
						    SIGSTOP)) {
						(void) sam_cancel_stage(ip,
							EINTR, credp);
						}
#if defined(RHE_LINUX) || (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
						spin_unlock_irq(
						    &current->sighand->siglock);
#else
						spin_unlock_irq(
						    &current->sig->siglock);
#endif /* RHE_LINUX */
					}
#endif /* linux */
				}
				/*
				 * sam_wait_rm will return EAGAIN if NFS and not
				 * readily available
				 */
				goto out;
			}
			if (ip->stage_err == EREMCHG) {
				if (SAM_IS_SHARED_FS(ip->mp) &&
				    !IS_SHAREFS_THREAD_OS &&
				    (ip->mp->mt.fi_status &
					(FS_LOCK_HARD |
					FS_UMOUNT_IN_PROGRESS))) {
					/* Issue stage again */
					ip->flags.b.staging = 0;
					error = EREMCHG;
					goto out;
				}
				break;	/* If daemon down */
			}
		}

		/*
		 * If no daemon present, reissue stage.
		 */
		if (ip->stage_err == EREMCHG) {
			TRACE(T_SAM_STAGE_DAEMON, SAM_ITOP(ip), samgt.amld_pid,
					(sam_tr_t)ip->di.rm.size,
					(sam_tr_t)ip->size);
			ip->flags.b.staging = 0;	/* Issue stage again */
			if (IS_SHAREFS_THREAD_OS) {
				goto out;
			}
			SAM_COUNT64(sam, stage_reissue);
			goto stage_file;
		}

		if (ip->flags.b.stage_n && !ip->stage_err && window) {
			if (ip->flags.b.staging) {
				if ((offset < ip->stage_n_off) ||
				    (window > ip->stage_size)) {
					goto stage_file;
				}
			} else {
				if ((offset < ip->real_stage_off) || (window >
				    (ip->real_stage_off +
				    ip->real_stage_len)))  {
					goto stage_file;
				}
			}
		}
		if (size != NULL) {
			*size = ip->flags.b.staging ? ip->stage_size : ip->size;
		}
		if ((error = ip->stage_err) != 0) {
			TRACE(T_SAM_STAGE_ERR, SAM_ITOP(ip), error,
					(sam_tr_t)ip->flags.bits,
					(sam_tr_t)ip->size);
			if (!ip->flags.b.staging) {
				SAM_COUNT64(sam, stage_errors);
				ip->di.status.b.stage_failed = 1;
			}
		}
	}
out:
	if (ip->di.status.b.offline) {
		if (sam_quota_foffline(ip->mp, ip, credp)) {
			cmn_err(CE_WARN,
		"sam_proc_offline:  couldn't return file quota "
		"status to offline\n");
		}
	}
#ifdef	sun
	if (ip->stage_thr == curthread) {
		ip->stage_thr = 0;
	}
#endif
	if (rw_type == RW_READER) {
		RW_DOWNGRADE_OS(&ip->inode_rwl);
	}
	return (error);
}


/*
 * ----- sam_cancel_stage - cancel the stage
 *
 *	For a given inode, cancel the stage. If request is still in the
 *  preview, the FS_FIFO_CANCEL will remove it. Otherwise the next
 *  SWRITE will fail because stage_err is set.
 */

int					/* ERRNO if error, 0 if successful. */
sam_cancel_stage(
	sam_node_t *ip,			/* pointer to inode. */
	int err,			/* Type of cancel error */
	cred_t *credp)			/* credentials pointer */
{
	int error = 0;

	ip->stage_err = (short)err;	/* If staging operation cancelled */
	ip->flags.b.staging = 0;
	if (ip->stage_seg) {
		sam_release_seg(ip);
	}
	sam_set_size(ip);
	TRACE(T_SAM_FIFO_CANST, SAM_ITOP(ip), ip->di.id.ino,
	    ip->di.rm.size, err);
	SAM_COUNT64(sam, stage_cancel);

	if (SAM_IS_SHARED_CLIENT(ip->mp)) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		error = sam_proc_inode(ip, INODE_cancel_stage, NULL, credp);
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		mutex_enter(&ip->rm_mutex);
		if (ip->rm_wait == 0) { /* If no one is waiting on stage */
			ip->flags.b.staging = 0;
		}
		mutex_exit(&ip->rm_mutex);

	} else {
		sam_stage_request_t *req;

		req = kmem_zalloc(sizeof (*req), KM_SLEEP);

		req->id = ip->di.id;
		req->fseq = ip->mp->mt.fi_eq;
		req->flags |= STAGE_CANCEL;

		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		error = sam_send_stage_cmd(ip->mp, SCD_stager,
		    req, sizeof (*req));
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		kmem_free(req, sizeof (*req));
	}

	return (error);
}


/*
 * ----- sam_syslog - issue syslog message to sam-fsd.
 */

void
sam_syslog(
	sam_node_t *ip,			/* pointer to inode. */
	enum syslog_numbers slerror,	/* error message number */
	int error,			/* errno */
	int param)			/* optional parameter */
{
	struct sam_fsd_cmd cmd;
	sam_mount_t *mp;

	mp = ip->mp;
	cmd.args.syslog.id = ip->di.id;
	bcopy(mp->mt.fi_mnt_point, cmd.args.syslog.mnt_point,
	    sizeof (cmd.args.syslog.mnt_point));
	cmd.args.syslog.slerror = slerror;
	cmd.args.syslog.error = error;
	cmd.args.syslog.param = param;
	cmd.cmd = FSD_syslog;
	(void) sam_send_scd_cmd(SCD_fsd, &cmd, sizeof (cmd));
}


/*
 * ----- sam_wait_archive - Wait for file/segment to be archived.
 *
 * NOTE. inode_rwl WRITER lock set on entry and exit.
 */

int				/* ERRNO if error, 0 if successful. */
sam_wait_archive(
	sam_node_t *ip,		/* Pointer to inode. */
	int archive_w)		/* copies to wait for; 0x10 - any; */
				/*   0x20 - all required */
{
	int error = 0;

	if (archive_w == 0) {
		return (EINVAL);
	}
	ip->flags.bits |= SAM_ARCHIVE_W;
	for (;;) {
		if ((archive_w  & 0x10) && ip->di.arch_status != 0) {
			break;
		}
		if (archive_w & 0x20) {
			int mask;
			int n;
			int req;

			/*
			 * Check for all required copies made.
			 */
			req = ip->di.status.b.archdone;
			for (n = 0, mask = 1; n < MAX_ARCHIVE;
				n++, mask += mask) {
				if (ip->di.ar_flags[n] & AR_required) {
					req++;
					if (!(ip->di.arch_status & mask)) {
						break;
					}
				}
			}
			if (n >= MAX_ARCHIVE && req != 0) {
				/*
				 * All required copies made or archdone set.
				 */
				break;
			}
			if (req == 0) {
				/*
				 * Copy status not known - ask arfind to set it.
				 */
				sam_send_to_arfind(ip, AE_archive, 0);
			}
		}
		if (archive_w  & ip->di.arch_status) {
			break;
		}
		if ((error = sam_wait_rm(ip, SAM_ARCHIVE_W))) {
			break;
		}
	}

	ip->flags.bits &= ~SAM_ARCHIVE_W;
	return (error);
}


/*
 * ----- sam_wait_rm - Wait for signal from daemon.
 *
 * Wait for signal from daemon. If nfs, return right away.
 * NOTE. inode_rwl WRITER lock set on entry and exit.
 *
 * flag:
 *	SAM_STAGING if waiting for stage,
 *	SAM_RM_BUSY if waiting for removable media
 *	SAM_ARCHIVE_W if waiting for one archive copy
 *	SAM_RM_OPENED if waiting for removable media file open
 */

int				/* ERRNO if error, 0 if successful. */
sam_wait_rm(sam_node_t *ip, int flag)
{
	int error, ret;

	mutex_enter(&ip->rm_mutex);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	ip->rm_wait++;
	if (flag == SAM_STAGING) {
#ifdef	sun
		if (ip->stage_thr == curthread) {
			ip->stage_thr = 0;
		}
#endif
		TRACE(T_SAM_STAGE_WAIT, SAM_ITOP(ip), ip->rm_wait,
		    ip->flags.bits,
		    (uint_t)ip->stage_size);
	} else if (flag == SAM_ARCHIVE_W) {
		TRACE(T_SAM_ARCH_WAIT, SAM_ITOP(ip), ip->di.id.ino,
		    ip->flags.bits,
		    ip->rm_wait);
	} else {
		TRACE(T_SAM_RM_WAIT, SAM_ITOP(ip), ip->di.id.ino,
		    ip->flags.bits,
		    ip->rm_wait);
	}
	error = 0;
	if (ip->stage_err == EREMCHG) {
		error = EREMCHG;
	} else if (!SAM_THREAD_IS_NFS()) {		/* if not NFS access */
		if ((ret = sam_cv_wait_sig(&ip->rm_cv, &ip->rm_mutex)) == 0) {
			error = EINTR;
		}
	} else {		/* If NFS access */
		if ((ret = sam_cv_wait1sec_sig(&ip->rm_cv,
		    &ip->rm_mutex)) <= 0) {
			if (ret == 0) {		/* If signal received */
				error = EINTR;
			} else {			/* If timed out */
#ifdef sun
				curthread->t_flag |= T_WOULDBLOCK;
#endif /* sun */
				error = EAGAIN;
			}
		}
	}

	if (flag == SAM_STAGING) {
		TRACE(T_SAM_STAGE_GO, SAM_ITOP(ip),
		    ip->di.id.ino, ip->flags.bits,
		    error);
	} else if (flag == SAM_ARCHIVE_W) {
		TRACE(T_SAM_ARCH_GO, SAM_ITOP(ip),
		    ip->di.id.ino, ip->flags.bits,
		    error);
	} else {
		TRACE(T_SAM_RM_GO, SAM_ITOP(ip), ip->di.id.ino, ip->flags.bits,
		    error);
	}
	ip->rm_wait--;
	mutex_exit(&ip->rm_mutex);

	/*
	 * If shared fs, stage_err is set to EREMCHG. Freeze stage
	 * except if the stage is on the shared file system thread.
	 */
	if (SAM_IS_SHARED_FS(ip->mp)) {
		if ((ip->stage_err == EREMCHG) &&
		    ((flag == SAM_STAGING) || (flag == SAM_ARCHIVE_W))) {
			if (!IS_SHAREFS_THREAD_OS) {
				error = sam_idle_operation(ip);
			}
		}
	}
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	return (error);
}


#ifdef sun
/*
 * ----- sam_start_stop_rmedia -
 *
 * If starting daemon, find inodes waiting on staging & reissue.
 * If stopping daemon, find inodes writing to rm and issue unload.
 */

void
sam_start_stop_rmedia(sam_mount_t *mp, sam_dstate_t flag)
{
	sam_ihead_t	*hip;	/* pointer to the hash pointer */
	sam_node_t *ip;
	sam_node_t *nip;
	int	ihash;
	kmutex_t *ihp;

	/*
	 * Pre-cycle through hash chains, clearing rmedia-scanned flag.
	 */
	hip = (sam_ihead_t *)&samgt.ihashhead[0];
	ASSERT(hip != NULL);
	for (ihash = 0; ihash < samgt.nhino; ihash++, hip++) {
		ihp = &samgt.ihashlock[ihash];
		mutex_enter(ihp);
		for (ip = hip->forw;
		    (ip != (sam_node_t *)(void *)hip);
		    ip = ip->chain.hash.forw) {
			if ((mp == NULL) || ((mp != NULL) && (mp == ip->mp))) {
				ip->hash_flags &= ~SAM_HASH_FLAG_RMEDIA;
			}
		}
		mutex_exit(ihp);
	}

	/*
	 * Cycle through hash chains.
	 */
	hip = (sam_ihead_t *)&samgt.ihashhead[0];
	ASSERT(hip != NULL);
	for (ihash = 0; ihash < samgt.nhino; ihash++, hip++) {
		ihp = &samgt.ihashlock[ihash];
		mutex_enter(ihp);
		for (ip = hip->forw;
		    (ip != (sam_node_t *)(void *)hip); ip = nip) {
			nip = ip->chain.hash.forw;
			if ((ip->di.id.ino == SAM_INO_INO) ||
			    (ip->di.id.ino == SAM_ROOT_INO) ||
			    (ip->di.id.ino == SAM_BLK_INO)) {
				continue;
			}
			if ((mp != NULL) && (mp != ip->mp)) {
				continue;
			}
			if (ip->hash_flags & SAM_HASH_FLAG_RMEDIA) {
				/* Already seen? */
				continue;
			}
			ip->hash_flags |= SAM_HASH_FLAG_RMEDIA;

			if ((sam_hold_vnode(ip, IG_EXISTS))) {
				continue;
			}
			if (RW_TRYENTER_OS(&ip->inode_rwl, RW_WRITER) == 0) {
				/*
				 * Could not get writers lock, block until lock
				 * gotten
				 */
				mutex_exit(ihp);
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			} else {
				mutex_exit(ihp);
			}

			switch (flag) {
			case SAM_START_DAEMON:
				/*
				 * Reissue stage for processes that are waiting.
				 * Clear stage flag for stages which have no
				 * process waiting.
				 */
				if (ip->flags.b.staging) {
					mutex_enter(&ip->rm_mutex);
					if (ip->rm_wait) {
						TRACE(T_SAM_RMRESUME,
						    SAM_ITOP(ip), ip->di.id.ino,
						    (SAM_ITOV(ip))->v_count,
						    ip->flags.bits);
						ip->stage_err = EREMCHG;
						cv_broadcast(&ip->rm_cv);
					} else if (ip->stage_thr == 0) {
						ip->flags.b.staging = 0;
					}
					mutex_exit(&ip->rm_mutex);
				}
				break;

			case SAM_STOP_DAEMON: {
				boolean_t wake_rm_waiter = B_FALSE;
				boolean_t notify_client = B_FALSE;

#ifdef METADATA_SERVER
				/*
				 * Issue unload. This causes the next write to
				 * the removable media file to fail with ENODEV
				 * errno.
				 */
				if (ip->flags.b.rm_opened) {
					int error;

					TRACE(T_SAM_RM_STOP, SAM_ITOP(ip),
					    ip->di.id.ino,
					    (SAM_ITOV(ip))->v_count,
					    ip->flags.bits);
					error = sam_unload_rm(ip, FWRITE, 0,
					    0, CRED());
					if (ip->rm_err == 0) {
						ip->rm_err = error;
					}
				}
				if (ip->daemon_busy) {
					mutex_enter(&ip->daemon_mutex);
					if (ip->daemon_busy) {
						TRACE(T_SAM_DAEMON_STOP,
						    SAM_ITOP(ip), ip->di.id.ino,
						    (SAM_ITOV(ip))->v_count,
						    ip->flags.bits);
						ip->rm_err = EINTR;
						ip->daemon_busy = 0;
						cv_signal(&ip->daemon_cv);
					}
					mutex_exit(&ip->daemon_mutex);
				}
#endif /* METADATA_SERVER */
				/*
				 * Wake up rm_cv waiters if stage is
				 * interrupted, or stager has shut down for
				 * failover and wasn't called by sam-amld but by
				 * sam_failover_old_server().
				 */
				mutex_enter(&ip->rm_mutex);
				if (ip->flags.b.staging) {
					sam_mount_t *mp = ip->mp;

					TRACE(T_SAM_STAGE_STOP, SAM_ITOP(ip),
					    ip->di.id.ino,
					    (SAM_ITOV(ip))->v_count,
					    ip->flags.bits);

					if (SAM_IS_SHARED_SERVER(mp) &&
					    (samgt.stagerd_pid < 0)) {
						/*
						 * If sam-amld is exiting for
						 * failover. Don't wake up
						 * threads waiting on the
						 * rm_mutex. Those threads will
						 * be signaled after FS_FAILOVER
						 * flag is set.
						 */
						if (SAM_CUR_PID ==
						    samgt.amld_pid) {
							mutex_exit(&ip->
							    rm_mutex);
							break;
						} else if (mp->mt.fi_status &
						    FS_FAILOVER) {
							notify_client = B_TRUE;
						}
					}
					if (ip->rm_wait) {
						wake_rm_waiter = B_TRUE;
					}
					ip->stage_err = EREMCHG;
				} else if ((ip->stage_err == ENOTACTIVE) &&
				    (samgt.stagerd_pid < 0) &&
				    (SAM_CUR_PID != samgt.amld_pid)) {
					/*
					 * Stage i/o was inprogress on this
					 * inode when voluntary failover was
					 * initiated.
					 */
					TRACE(T_SAM_STAGE_STOP, SAM_ITOP(ip),
					    ip->di.id.ino,
					    (SAM_ITOV(ip))->v_count,
					    ip->flags.bits);
					ip->stage_err = EREMCHG;
					if (SAM_IS_SHARED_SERVER(ip->mp)) {
						notify_client = B_TRUE;
					}
					if (ip->rm_wait) {
						wake_rm_waiter = B_TRUE;
					}
				}
				if (notify_client) {
					sam_notify_staging_clients(ip);
				}
				if (wake_rm_waiter) {
					cv_broadcast(&ip->rm_cv);
				}
				mutex_exit(&ip->rm_mutex);
				}
				break;

			}
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			VN_RELE_OS(ip);
			mutex_enter(ihp);
			nip = hip->forw;
		}
		mutex_exit(ihp);
	}
}
#endif /* sun */
