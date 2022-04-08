/*
 * ----- sam/cvnops.c - Process the common vnode functions.
 *
 * Processes the common vnode functions supported on the QFS File System.
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

#pragma ident "$Revision: 1.156 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/dirent.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/fs_subr.h>
#include <sys/flock.h>
#include <sys/systm.h>
#include <sys/ioccom.h>
#include <sys/vmsystm.h>
#include <sys/file.h>
#include <sys/dkio.h>
#include <sys/rwlock.h>
#include <sys/share.h>
#include <sys/unistd.h>
#include <nfs/nfs.h>
#include <vm/as.h>
#include <vm/seg_vn.h>
#include <sys/policy.h>

/* ----- SAMFS Includes */

#include "sam/param.h"
#include "sam/types.h"
#include "sam/fioctl.h"
#include "sam/samaio.h"
#include "sam/samevent.h"

#include "samfs.h"
#include "ino.h"
#include "macros.h"
#include "inode.h"
#include "mount.h"
#include "arfind.h"
#include "rwio.h"
#include "extern.h"
#include "listio.h"
#include "debug.h"
#include "trace.h"
#include "kstats.h"
#include "qfs_log.h"

static int sam_proc_listio(sam_node_t *ip, int mode, int *arg, int flag,
	int *rvp, cred_t *credp);
static int sam_process_listio_call(sam_node_t *ip, int mode,
	struct samaio_uio *auio, cred_t *credp);
static int sam_direct_listio(sam_node_t *ip, int mode,
	struct sam_listio_call *callp, cred_t *credp);
void sam_listio_done(struct buf *bp);
static int sam_wait_listio(sam_node_t *ip, void *arg, pid_t pid);
static void sam_free_listio(struct sam_listio_call *callp);

void sam_rwlock_common(vnode_t *vp, int w);
extern int sam_freeze_ino(sam_mount_t *mp, sam_node_t *ip, int force_freeze);

extern int qfs_fiologenable(vnode_t *vp, fiolog_t *ufl, cred_t *cr, int flags);
extern int qfs_fiologdisable(vnode_t *vp, fiolog_t *ufl, cred_t *cr, int flags);
extern int qfs_fioislog(vnode_t *vp, uint32_t *islog, cred_t *cr, int flags);
extern uint_t lqfs_debug;

/*
 * MDB switch to control paged i/o on object files.
 */
boolean_t sam_object_directio_required = FALSE;

#define	SAM_DIRECTIO_REQUIRED(ip) \
	((SAM_IS_OBJECT_FILE(ip) && sam_object_directio_required))

/*
 * ----- sam_close_vn - Close a SAM-QFS file.
 *
 * Closes a SAM-QFS file.
 */

#define	GET_WRITE_LOCK(ip, write_lock)					\
		if (!write_lock) {					\
			if (!rw_tryupgrade(&(ip)->inode_rwl)) {		\
				RW_UNLOCK_OS(&(ip)->inode_rwl, RW_READER); \
				RW_LOCK_OS(&(ip)->inode_rwl, RW_WRITER); \
			}						\
			write_lock = TRUE;				\
		}

int				/* ERRNO if error, 0 if successful. */
sam_close_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* pointer to vnode. */
	int filemode,		/* open file mode. */
	int count,		/* ref count for the file ptr being closed. */
	offset_t offset,	/* offset pointer. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* pointer to vnode. */
	int filemode,		/* open file mode. */
	int count,		/* ref count for the file ptr being closed. */
	offset_t offset,	/* offset pointer. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	sam_node_t *ip;
	int error = 0;
	int write_mode;
	boolean_t write_lock;
	boolean_t last_close;
	boolean_t arch_close;

	if (SAM_VP_IS_STALE(vp)) {
#if defined(SOL_511_ABOVE)
		return (sam_close_stale_vn(vp, filemode, count, offset,
		    credp, ct));
#else
		return (sam_close_stale_vn(vp, filemode, count, offset,
		    credp));
#endif
	}
	ip = SAM_VTOI(vp);
	TRACE(T_SAM_CLOSE, vp, ip->di.id.ino, ip->flags.bits, (sam_tr_t)ip->rdev);

	RW_LOCK_OS(&ip->inode_rwl, RW_READER);
	write_lock = FALSE;
	arch_close = FALSE;

	write_mode = ip->flags.b.write_mode;

	/*
	 * Process termination of stage when the stage daemon closes the file.
	 * Clear archiver flags reset attributes when sam-arcopy process
	 * closes the file.
	 */
#ifdef METADATA_SERVER
	if ((ip->stage_pid > 0) && (ip->stage_pid == SAM_CUR_PID)) {
		GET_WRITE_LOCK(ip, write_lock);
		error = sam_close_stage(ip, credp);
	} else if (ip->arch_count) {
		int i;

		GET_WRITE_LOCK(ip, write_lock);
		for (i = 0; i < MAX_ARCHIVE; i++) {
			if (ip->arch_pid[i] == curproc->p_ppid) {
				arch_close = TRUE;
				ip->arch_count--;
				ip->arch_pid[i] = 0;
				break;
			}
		}
	}
#endif

	if (ip->rm_pid && (ip->rm_pid == curproc->p_pid) &&
	    ip->flags.b.rm_opened) {
#ifdef METADATA_SERVER
		GET_WRITE_LOCK(ip, write_lock);
		error = sam_close_rm(ip, filemode, credp);
#else
		error = EINVAL;
#endif
	} else if ((count <= 1) && write_mode && ip->klength) {
		/*
		 * Flush kluster from last write behind if last reference close.
		 */
		GET_WRITE_LOCK(ip, write_lock);
		(void) VOP_PUTPAGE_OS(vp, ip->koffset, ip->klength,
		    (B_ASYNC | B_FREE), credp, NULL);
	}

	/*
	 * Decrement count of opens.
	 */
	last_close = FALSE;
	if (count <= 1) {
		mutex_enter(&ip->fl_mutex);
		if (--ip->no_opens < 0) {
			ip->no_opens = 0;
		}
		last_close = (ip->no_opens == 0);
		mutex_exit(&ip->fl_mutex);
	}

	if (last_close) {
		GET_WRITE_LOCK(ip, write_lock);
		ip->flags.b.write_mode = 0; /* File not opened for write */

		if (!SAM_IS_SHARED_FS(ip->mp) ||
		    !SAM_IS_SHARED_SERVER(ip->mp)) {
			TRACE(T_SAM_ABR_CLR, SAM_ITOP(ip), ip->di.id.ino,
			    -1, ip->no_opens);
			ip->flags.b.abr = 0;
		}

		/*
		 * At last close, switch to default I/O type and switch to
		 * default qwrite setting.
		 */
		sam_reset_default_options(ip);

		/*
		 * At last close, remove all outstanding listio.
		 */
		if (ip->listiop) {
			int err;

			err = sam_remove_listio(ip);
			if (!error) {
				error = err;
			}
		}

		/*
		 * On last close, if stage_n, free stage_n blocks and umem
		 * buffer pages.  Note, stage_n_count may be set if NFS is also
		 * accessing the stage_n buffer. Do not free buffer in this
		 * case. This will cause this buffer to be held until the file
		 * is inactivated.
		 */
		if (ip->flags.b.stage_n && (ip->stage_n_count == 0)) {
			sam_free_stage_n_blocks(ip);
		}

		/*
		 * On last archiver close, reset stage_n flags in case it was
		 * changed by the archiver.
		 */
		if (arch_close) {
			ip->flags.b.arch_direct = 0;
			ip->flags.b.stage_n = ip->di.status.b.direct;
			ip->flags.b.stage_all = ip->di.status.b.stage_all;
		}
	}

	/*
	 * For object files on last close of a file operation that changes
	 * allocation, update allocated block count.
	 */
	if (SAM_IS_OBJECT_FILE(ip) && last_close &&
	    (filemode & (FWRITE|FAPPEND|FTRUNC))) {
		sam_osd_update_blocks(ip, 0);
	}

	if (write_lock) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	} else {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	}

	if (!SAM_IS_SHARED_FS(ip->mp)) {
		cleanlocks(vp, ttoproc(curthread)->p_pid, 0);
	}

	cleanshares(vp, ttoproc(curthread)->p_pid);
	if (last_close && write_mode &&
	    (ip->mp->mt.fi_config & MT_SHARED_WRITER)) {
#if defined(SOL_511_ABOVE)
		(void) sam_fsync_vn(vp, FSYNC, credp, ct);
#else
		(void) sam_fsync_vn(vp, FSYNC, credp);
#endif
	}
	/*
	 * Notify arfind and event daemon of close.
	 */
	if (last_close && !arch_close &&
	    !S_ISDIR(ip->di.mode) && ip->di.id.ino != SAM_INO_INO) {
		sam_send_to_arfind(ip, AE_close, 0);
		if (ip->mp->ms.m_fsev_buf &&
		    (filemode & (FWRITE|FAPPEND|FCREAT|FTRUNC))) {
			sam_send_event(ip->mp, &ip->di, ev_close, filemode, 0,
			    ip->di.modify_time.tv_sec);
		}
	}

	/*
	 * If segment inode held segment index, must inactive index (seg_held).
	 *
	 * XXX Is this really correct?  This will release the segment index
	 * XXX when any user of the inode performs a close, not just the one
	 * XXX who originally caused it to be held via sam_find_ino.
	 */
	if (S_ISSEGS(&ip->di) && ip->seg_held) {
		sam_rele_index(ip);
	}
	TRACE(T_SAM_CLOSE_RET, vp, (sam_tr_t)ip->rdev, ip->flags.bits, error);
	return (error);
}

#undef GET_WRITE_LOCK


/*
 * ----- sam_read_vn - Read a SAM-QFS file.
 *
 * Read a SAM-QFS file. Parameters are in user I/O vector array.
 */

int				/* ERRNO if error, 0 if successful. */
sam_read_vn(
	vnode_t *vp,		/* pointer to vnode. */
	uio_t *uiop,		/* user I/O vector array. */
	int ioflag,		/* file I/O flags (/usr/include/sys/file.h). */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct)	/* caller context pointer. */
{
	sam_node_t		*ip, *bip;
	int				stage_n_set = 0;
	int				error = 0;
	int				sam_blocks;
	boolean_t		use_direct_io;
	segment_descriptor_t seg;

	ip = SAM_VTOI(vp);
	bip = ip;
	TRACE(T_SAM_READ, vp, ip->di.id.ino, (sam_tr_t)uiop->uio_loffset,
	    uiop->uio_resid);
	SAM_DEBUG_COUNT64(nreads);

	RW_LOCK_OS(&ip->inode_rwl, RW_READER);
	if (ip->di.status.b.damaged) {
		error = ENODATA;		/* best fit errno */
		goto out;
	}
	/*
	 * Removable media file must be opened. NFS access is not supported.
	 */
	if (S_ISREQ(ip->di.mode) && !ip->flags.b.rm_opened) {
		error = EINVAL;
		goto out;
	}

	if (MANDLOCK(vp, ip->di.mode)) {
		sam_u_offset_t off = uiop->uio_loffset;
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		error = chklock(vp, FREAD, off, uiop->uio_resid,
		    uiop->uio_fmode, ct);
		if (error) {
			return (error);
		}
		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
	}
	if (uiop->uio_resid == 0) {
		goto out;
	}
	SAM_DEBUG_ADD64(breads, uiop->uio_resid);

	/*
	 * Check for index segment access file. If one, must process in
	 * segments.
	 */
	if (S_ISSEGI(&bip->di)) {
		if ((error = sam_setup_segment(bip, uiop, UIO_READ,
		    &seg, credp))) {
			goto out;
		}

		/*
		 * If segment file, may have to loop to issue for all segments
		 */
segment_file:
		if ((uiop->uio_loffset >= bip->di.rm.size) ||
		    (error = sam_get_segment(bip, uiop, UIO_READ, &seg, &ip,
		    credp))) {
			goto out;
		}
		if (ip->flags.b.stage_n) {
			ip->no_opens++;
		}
	}

	if (SAM_THREAD_IS_NFS()) {
		ip->nfs_ios++;
	}

	/*
	 * If offline, return when enough of the file is online for the request.
	 */
	if (ip->di.status.b.offline) {
		if (SAM_THREAD_IS_NFS() &&
		    !S_ISSEGI(&bip->di) && ip->flags.b.stage_n) {
			ip->no_opens++;
		}
		if (uiop->uio_loffset >= ip->di.rm.size) {
			goto out;
		}
		if ((error = sam_read_proc_offline(ip, bip, uiop, credp,
		    &stage_n_set))) {
			if (S_ISSEGI(&bip->di)) {
				sam_release_segment(bip, uiop,
				    UIO_READ, &seg, ip);
			}
			if (!SAM_IS_SHARED_FS(ip->mp)) {
				/* EMPTY */
				SAM_NFS_JUKEBOX_ERROR(error);
			}
			goto out;
		}
	}

#ifdef METADATA_SERVER
	/*
	 * If shared reader reading past EOF, refresh inode.
	 */
	if ((ip->mp->mt.fi_config & MT_REFRESH_EOF) &&
	    SAM_IS_SHARED_READER(ip->mp)) {
		if ((uiop->uio_loffset + uiop->uio_resid) > ip->size) {
			if (ip->di.id.ino != SAM_INO_INO) {
				if ((error = sam_refresh_shared_reader_ino(ip,
				    FALSE, credp))) {
					goto out;
				}
			}
		}
	}
#endif

	SAM_COUNT_IO(ip);

	/*
	 * Determine whether to use direct i/o.
	 *
	 * Tapes and optical devices always use direct i/o.
	 *
	 * For normal files, we use direct i/o if it is both preferred and
	 * allowed.
	 *
	 * We prefer direct i/o if:
	 *   (a) the file's directio flag is set; or
	 *   (b) (1) a sufficient number of read operations have been issued;
	 *  and
	 *   (b) (2) the file is not staging.
	 *
	 * We allow direct i/o if:
	 *   (a) the file is not memory-mapped; and
	 *   (b) no pages are presently in use by the stager; and
	 *   (c) the i/o request is for an integral number of device blocks; and
	 *   (d) the i/o buffer is correctly aligned.
	 *
	 * (Currently we allow direct i/o to proceed if the first buffer in a
	 * uiovec is aligned, even if further buffers are not.  We emulate this
	 * using a page of memory for temporary storage.  I'm not sure if this
	 * is a good idea.)
	 */

	use_direct_io = ((ip->flags.bits & SAM_DIRECTIO) != 0) ||
	    SAM_DIRECTIO_REQUIRED(ip);

	if (vp->v_type == VREG) {
		if (use_direct_io) {
			if (!SAM_DIRECTIO_REQUIRED(ip) &&
			    (!SAM_DIRECTIO_ALLOWED(ip) ||
			    (((uint64_t)uiop->uio_iov->iov_base & 1) != 0) ||
			    (uiop->uio_resid < DEV_BSIZE))) {
				SAM_COUNT64(dio, dio_2_mmap);
				use_direct_io = FALSE;
			}
		} else {
			if (SAM_DIRECTIO_ALLOWED(ip) &&
			    !ip->flags.b.staging &&
			    (((uint64_t)uiop->uio_iov->iov_base & 1) == 0) &&
			    (ip->mp->mt.fi_dio_rd_consec > 0)) {
				boolean_t can_attempt_dio = TRUE;

				sam_blocks = howmany(uiop->uio_resid,
				    SAM_DEV_BSIZE);
				/* If ill-formed */
				if ((uiop->uio_loffset &
				    (u_offset_t)(DEV_BSIZE - 1)) != 0) {
					if ((ip->mp->mt.fi_dio_rd_ill_min ==
					    0) ||
					    (sam_blocks <
					    ip->mp->mt.fi_dio_rd_ill_min)) {
						ip->rd_consec = 0;
						can_attempt_dio = FALSE;
					}
				} else {
					if (sam_blocks <
					    ip->mp->mt.fi_dio_rd_form_min) {
						ip->rd_consec = 0;
						can_attempt_dio = FALSE;
					}
				}
				if (can_attempt_dio) {
					if (++(ip->rd_consec) >=
					    ip->mp->mt.fi_dio_rd_consec) {
						SAM_COUNT64(dio, mmap_2_dio);
						use_direct_io = TRUE;
					}
				}
			}
		}
	}

	if (ip->flags.b.stage_n && ip->st_buf) {
		error = sam_read_stage_n_io(SAM_ITOV(ip), uiop);
	} else if (use_direct_io || (vp->v_type == VCHR)) {
		if (S_ISREQ(ip->di.mode)) {
			/* Tape and Optical direct I/O */
			error = sam_rm_direct_io(ip, UIO_READ, uiop, credp);
		} else {
			/* Disk direct I/O */
			error = sam_dk_direct_io(ip, UIO_READ, ioflag, uiop,
			    credp);
		}
	} else {
		ASSERT(!S_ISREQ(ip->di.mode));
		error = sam_read_io(SAM_ITOV(ip), uiop, ioflag);
	}

	if (error == 0 && !(ip->mp->mt.fi_mflag & MS_RDONLY)) {
		mutex_enter(&ip->fl_mutex);
		TRANS_INODE(ip->mp, ip);
		sam_mark_ino(ip, (SAM_ACCESSED));
		mutex_exit(&ip->fl_mutex);
	}
	SAM_UNCOUNT_IO(ip);

	if (S_ISSEGI(&bip->di)) {
		sam_release_segment(bip, uiop, UIO_READ, &seg, ip);
		if ((error == 0) && (uiop->uio_resid > 0)) {
			if (stage_n_set) {
				sam_stage_n_io_done(ip);
				stage_n_set = 0;
			}
			if (ip->flags.b.stage_n) {
				if (--ip->no_opens < 0) {
					ip->no_opens = 0;
				}
			}
			goto segment_file;
		}
	}
out:
	if (stage_n_set) {
		sam_stage_n_io_done(ip);
	}
	if (S_ISSEGI(&bip->di)) {
		if (ip->flags.b.stage_n) {
			if (--ip->no_opens < 0) {
				ip->no_opens = 0;
			}
		}
		ip = bip;
	} else if (SAM_THREAD_IS_NFS() && ip->di.status.b.offline &&
	    ip->flags.b.stage_n) {
		if (--ip->no_opens < 0) {
			ip->no_opens = 0;
		}
	}
	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	TRACE(T_SAM_READ_RET, vp, ip->di.id.ino, uiop->uio_resid, error);
	return (error);
}


/*
 * ----- sam_write_vn - Write a SAM-QFS file.
 *
 * Write a SAM-QFS file. Parameters are in user I/O vector array.
 */

int				/* ERRNO if error, 0 if successful. */
sam_write_vn(
	vnode_t *vp,		/* pointer to vnode. */
	uio_t *uiop,		/* user I/O vector array. */
	int ioflag,		/* file I/O flags (/usr/include/sys/file.h). */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct)	/* caller context pointer. */
{
	sam_node_t	*ip, *bip;
	int			error = 0;
	int			sam_blocks;
	segment_descriptor_t seg;
	int			segment_index = 0;
	int			rwl_mode;
	boolean_t	use_direct_io;
	boolean_t	partial_write = FALSE;
	ssize_t		returned_res = 0;
	rlim64_t	limit = uiop->uio_llimit;
	int		resv;

	ip = SAM_VTOI(vp);
	TRACE(T_SAM_WRITE, vp, ip->di.id.ino, (sam_tr_t)uiop->uio_loffset,
	    uiop->uio_resid);
	if (SAM_PRIVILEGE_INO(ip->di.version, ip->di.id.ino)) {
		return (EPERM);
	}
	SAM_DEBUG_COUNT64(nwrites);

	/*
	 * Default write throttle is 1/4 physmem. Stops runaway programs.
	 */
	if (ip->mp->mt.fi_wr_throttle &&
	    ip->cnt_writes > ip->mp->mt.fi_wr_throttle) {
		mutex_enter(&ip->write_mutex);
		ip->wr_thresh++;
		while (ip->cnt_writes > ip->mp->mt.fi_wr_throttle) {
			cv_wait(&ip->write_cv, &ip->write_mutex);
		}
		if (--ip->wr_thresh < 0) {
			ip->wr_thresh = 0;
		}
		mutex_exit(&ip->write_mutex);
	}

	/*
	 * If new file and request is >= one large DAU, set on large DAU flag
	 */
	if (!ip->di.status.b.on_large &&
	    (ip->di.blocks == 0) &&
	    (uiop->uio_loffset == 0) &&
	    (uiop->uio_resid >= LG_BLK(ip->mp, ip->di.status.b.meta))) {
		ip->di.status.b.on_large = 1;
	}

	/*
	 * Start a synchronous or an asynchronous LQFS transaction (depending
	 * on ioflag)
	 */
	resv = INODESIZE;	/* Use resv for future data journaling ? */
	if (ioflag & (FSYNC|FDSYNC)) {
		int terr = 0;

		TRANS_BEGIN_SYNC(ip->mp, TOP_WRITE_SYNC, resv, terr);
		ASSERT(!terr);
	} else {
		TRANS_BEGIN_ASYNC(ip->mp, TOP_WRITE, resv);
	}

	/*
	 * For listio, data_rwl is held in READ mode.
	 * Set rwl_mode = 0 (READ) for listio, else 1 (WRITE).
	 */
	rwl_mode = (ioflag != FASYNC ||
	    ((struct samaio_uio *)uiop)->type == SAMAIO_CHR_AIO);

	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);

	/*
	 * Removable media file must be opened. NFS access is not supported.
	 */
	if (S_ISREQ(ip->di.mode) && !ip->flags.b.rm_opened) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		error = EINVAL;		/* If removable media file */
		goto out2;
	}
	if ((ioflag & FAPPEND) && S_ISREG(ip->di.mode)) {
		uiop->uio_loffset = ip->di.rm.size;	/* Append at EOF */
	}

	if (MANDLOCK(vp, ip->di.mode)) {
		sam_u_offset_t off = uiop->uio_loffset;

		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		error = chklock(vp, FWRITE, off, uiop->uio_resid,
		    uiop->uio_fmode, ct);
		if (error) {
			goto out2;
		}
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	}
	if (uiop->uio_resid == 0) {
		error = 0;
		goto out;
	}
	if (limit == RLIM64_INFINITY || limit > SAM_MAXOFFSET_T) {
		limit = SAM_MAXOFFSET_T;
	}
	if (uiop->uio_loffset > SAM_MAXOFFSET_T ||
	    uiop->uio_loffset >= limit) {
		error = EFBIG;
		goto out;
	}
	if (uiop->uio_loffset + uiop->uio_resid > limit) {
		returned_res = (uiop->uio_loffset + uiop->uio_resid) - limit;
		uiop->uio_resid = limit - uiop->uio_loffset;
		partial_write = TRUE;
	}
	SAM_DEBUG_ADD64(bwrites, uiop->uio_resid);

	/*
	 * Do not create index segment (seg_file), until file exceeds segment
	 * size.
	 */
	bip = ip;
	if (bip->di.status.b.segment) {
		if (S_ISSEGI(&bip->di) ||
		    ((uiop->uio_loffset + uiop->uio_resid) >
		    SAM_SEGSIZE(bip->di.rm.info.dk.seg_size))) {
			if ((error = sam_setup_segment(bip, uiop,
			    UIO_WRITE, &seg, credp))) {
				goto out;
			}
			segment_index = 1;

			/*
			 * If segment file, may have to loop to issue for all
			 * segments
			 */
segment_file:
			if ((error = sam_get_segment(bip, uiop, UIO_WRITE,
			    &seg, &ip, credp))) {
				ip = bip;
				goto out;
			}
		}
	}

	if (SAM_THREAD_IS_NFS()) {
		ip->nfs_ios++;
	}

	/*
	 * If offline or stage pages exist for the inode, clear inode
	 * file/segment for writing by making file/segment online.
	 */
	if (ip->di.status.b.offline || ip->flags.b.stage_pages) {
		if (vp->v_type != VCHR) {
			RW_UNLOCK_CURRENT_OS(&ip->data_rwl);
		}
		error = sam_clear_ino(ip, ip->di.rm.size, MAKE_ONLINE, credp);
		if (vp->v_type != VCHR) {
			krw_t rw_type;
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			if (rwl_mode) {
				rw_type = RW_WRITER;
			} else {
				rw_type = RW_READER;
			}
			sam_rwdlock_ino(ip, rw_type, 1);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
		if (error) {
			if (S_ISSEGI(&bip->di)) {
				sam_release_segment(bip, uiop, UIO_WRITE,
				    &seg, ip);
				ip = bip;
			}
			goto out;
		}
	}

	SAM_COUNT_IO(ip);

	/*
	 * See comments in sam_read_vn() for direct i/o rules.
	 */

	use_direct_io = ((ip->flags.bits & SAM_DIRECTIO) != 0) ||
	    SAM_DIRECTIO_REQUIRED(ip);

	if (vp->v_type == VREG) {
		if (use_direct_io) {
			if (!SAM_DIRECTIO_REQUIRED(ip) &&
			    (!SAM_DIRECTIO_ALLOWED(ip) ||
			    (((uint64_t)uiop->uio_iov->iov_base & 1) != 0) ||
			    (uiop->uio_resid < DEV_BSIZE))) {
				SAM_COUNT64(dio, dio_2_mmap);
				use_direct_io = FALSE;
			}
		} else {
			if (SAM_DIRECTIO_ALLOWED(ip) &&
			    (((uint64_t)uiop->uio_iov->iov_base & 1) == 0) &&
			    (ip->mp->mt.fi_dio_wr_consec > 0)) {
				boolean_t can_attempt_dio = TRUE;

				sam_blocks = howmany(uiop->uio_resid,
				    SAM_DEV_BSIZE);
				/* If ill-formed */
				if ((uiop->uio_loffset &
				    (u_offset_t)(DEV_BSIZE - 1)) != 0) {
					if ((ip->mp->mt.fi_dio_wr_ill_min ==
					    0) ||
					    (sam_blocks <
					    ip->mp->mt.fi_dio_wr_ill_min)) {
						ip->wr_consec = 0;
						can_attempt_dio = FALSE;
					}
				} else {
					if (sam_blocks <
					    ip->mp->mt.fi_dio_wr_form_min) {
						ip->wr_consec = 0;
						can_attempt_dio = FALSE;
					}
				}
				if (can_attempt_dio) {
					if (++(ip->wr_consec) >=
					    ip->mp->mt.fi_dio_wr_consec) {
						SAM_COUNT64(dio, mmap_2_dio);
						use_direct_io = TRUE;
					}
				}
			}
		}
	}

	if (use_direct_io || (vp->v_type == VCHR)) {
		if (S_ISREQ(ip->di.mode)) {
			/* Tape and Optical direct I/O */
			error = sam_rm_direct_io(ip, UIO_WRITE, uiop, credp);
		} else {
			/* Disk direct I/O */
			while (((error = sam_dk_direct_io(
			    ip, UIO_WRITE, ioflag, uiop, credp)) != 0) &&
			    IS_SAM_ENOSPC(error)) {
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				if (vp->v_type != VCHR) {
					RW_UNLOCK_CURRENT_OS(&ip->data_rwl);
				}
				/*
				 * Close the LQFS transaction.
				 */
				if (ioflag & (FSYNC|FDSYNC)) {
					int terr = 0;

					TRANS_END_SYNC(ip->mp, terr,
					    TOP_WRITE_SYNC, resv);
					if (error == 0) {
						error = terr;
					}
				} else {
					TRANS_END_ASYNC(ip->mp, TOP_WRITE,
					    resv);
				}
				error = sam_wait_space(ip, error);
				/*
				 * Re-open the LQFS transaction.
				 */
				if (ioflag & (FSYNC|FDSYNC)) {
					int terr = 0;

					TRANS_BEGIN_SYNC(ip->mp, TOP_WRITE_SYNC,
					    resv, terr);
					ASSERT(!terr);
				} else {
					TRANS_BEGIN_ASYNC(ip->mp, TOP_WRITE,
					    resv);
				}
				if (vp->v_type != VCHR) {
					sam_rwlock_common(SAM_ITOV(ip),
					    rwl_mode);
				}
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				if (error) {
					break;
				}
			}
		}
	} else {
		int iovcnt = uiop->uio_iovcnt;

		ASSERT(!S_ISREQ(ip->di.mode));
		while (((error = sam_write_io(
		    SAM_ITOV(ip), uiop, ioflag, credp)) != 0) &&
		    IS_SAM_ENOSPC(error)) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			RW_UNLOCK_CURRENT_OS(&ip->data_rwl);
			/*
			 * Close the LQFS transaction.
			 */
			if (ioflag & (FSYNC|FDSYNC)) {
				int terr = 0;

				TRANS_END_SYNC(ip->mp, terr, TOP_WRITE_SYNC,
				    resv);
				if (error == 0) {
					error = terr;
				}
			} else {
				TRANS_END_ASYNC(ip->mp, TOP_WRITE, resv);
			}
			error = sam_wait_space(ip, error);
			/*
			 * Re-open the LQFS transaction.
			 */
			if (ioflag & (FSYNC|FDSYNC)) {
				int terr = 0;

				TRANS_BEGIN_SYNC(ip->mp, TOP_WRITE_SYNC, resv,
				    terr);
				ASSERT(!terr);
			} else {
				TRANS_BEGIN_ASYNC(ip->mp, TOP_WRITE, resv);
			}
			sam_rwlock_common(SAM_ITOV(ip), rwl_mode);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			if (error) {
				break;
			}
		}
		if (ioflag == FASYNC &&
		    ((struct samaio_uio *)uiop)->type ==
		    SAMAIO_LIO_DIRECT) {
			/*
			 * Direct list I/O got handled as paged.  Signal
			 * completion.
			 */
			struct samaio_uio *aio = (struct samaio_uio *)uiop;
			buf_t *bp = aio->bp;

			sam_listio_done(bp);
			kmem_free(aio, sizeof (struct samaio_uio) +
			    sizeof (struct iovec) * (iovcnt - 1));
		}
	}

	SAM_UNCOUNT_IO(ip);

	if (error == 0) {
		TRANS_INODE(ip->mp, ip);
		sam_mark_ino(ip, (SAM_UPDATED | SAM_CHANGED));
	}

	if (segment_index) {
		sam_release_segment(bip, uiop, UIO_WRITE, &seg, ip);
		if (error || (uiop->uio_resid <= 0)) {
			ip = bip;
		} else {
			goto segment_file;
		}
	}
out:
	TRANS_INODE(ip->mp, ip);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

	if (partial_write && !(ioflag & FASYNC)) {
		uiop->uio_resid += returned_res;
	}
out2:
	/*
	 * Close the LQFS transaction.
	 */
	if (ioflag & (FSYNC|FDSYNC)) {
		int terr = 0;

		TRANS_END_SYNC(ip->mp, terr, TOP_WRITE_SYNC, resv);
		if (error == 0) {
			error = terr;
		}
	} else {
		TRANS_END_ASYNC(ip->mp, TOP_WRITE, resv);
	}
	TRACE(T_SAM_WRITE_RET, vp, ip->di.id.ino,
	    (ioflag & FASYNC) ? -1 : uiop->uio_resid, error);
	return (error);
}


/*
 * ----- sam_ioctl_vn - Ioctl call.
 *
 * Ioctl miscellaneous call.
 */

/* ARGSUSED6 */
int				/* ERRNO if error, 0 if successful. */
sam_ioctl_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* pointer to vnode. */
	int cmd,		/* command number. */
	sam_intptr_t arg,	/* argument. */
	int flag,		/* file descriptor open instance flag. */
	cred_t *credp,		/* credentials pointer. */
	int *rvp,		/* return value pointer. */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* pointer to vnode. */
	int cmd,		/* command number. */
	sam_intptr_t arg,	/* argument. */
	int flag,		/* file descriptor open instance flag. */
	cred_t *credp,		/* credentials pointer. */
	int *rvp		/* return value pointer. */
#endif
)
{
	offset_t raddr[32];
	int *p = (int *)raddr;
	int error = 0;
	sam_node_t *ip;
	char type;

	type = (cmd >> 8) & 0xff;
	ip = SAM_VTOI(vp);

	switch (cmd & 0xf0000000) {
	case IOC_VOID:
		raddr[0] = arg;
		break;
	case IOC_IN:
	case IOC_INOUT:
		if (copyin((caddr_t)arg, (caddr_t)p,
		    (cmd >> 16) & IOCPARM_MASK)) {
			return (EFAULT);
		}
		break;
	default:
		break;
	}

	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

	TRACE(T_SAM_IOCTL, (sam_tr_t)vp, (sam_tr_t)ip->rdev, cmd, (sam_tr_t)p);
	switch (type) {

	case 'u':		/* Miscellaneous archiver utility commands. */

		error = sam_ioctl_util_cmd(ip, (cmd & 0xff), p, rvp, credp);
		break;

	case 'U':		/* Miscellaneous operator utility commands. */

		error = sam_ioctl_oper_cmd(ip, (cmd & 0xff), p, rvp, credp);
		break;

	case 'P':		/* SAM File privileged commands. */

		error = sam_ioctl_sam_cmd(ip, (cmd & 0xff), p, flag,
		    rvp, credp);
		break;

	case 'f':		/* File commands. */

		error = sam_ioctl_file_cmd(ip, (cmd & 0xff), p, flag,
		    rvp, credp);
		break;

#ifdef DKIOCGETVOLCAP
	case ((DKIOC) >> 8): /* {GET,SET}VOLCAP commands (ABR/DMR emulation) */
		error = sam_ioctl_dkio_cmd(ip, (cmd & 0xffff), arg,
		    flag, credp);
		break;
#endif

	case 'o':		/* QFS OBJECT ioctl commands */
		error = sam_obj_ioctl_cmd(ip, (cmd & 0xff), p, rvp, credp);
		break;

	default:
		error = ENOTTY;
		break;
	}

	SAM_CLOSE_OPERATION(ip->mp, error);

	switch (cmd & 0xf0000000) {
	case IOC_OUT:
	case IOC_INOUT:
		if (copyout((caddr_t)p,
		    (caddr_t)arg, (cmd >> 16) & IOCPARM_MASK)) {
			if (error == 0) {
				error = EFAULT;
			}
		}
		break;
	default:
		break;
	}
	TRACE(T_SAM_IOCTL_RET, vp, (sam_tr_t)cmd, error, 0);
	return (error);
}


/*
 * For NAS compatibility the retention period is stored as the
 * modified time plus the duration which is stored in the attribute_time
 * (in minutes). If the attribute time is zero meaning the period is
 * permanent, set the access time to INT_MAX (the NAS value for
 * permanent retention).
 */
static void
sam_trans_access(
	vattr_t *vap,   /* vattr pointer. */
	sam_node_t *ip) /* inode pointer. */
{
	sam_timestruc_t access_time;

	if (ip->di2.rperiod_duration == 0) {
		access_time.tv_sec = INT_MAX;
		access_time.tv_nsec = 0;
	} else {
		access_time.tv_sec = ip->di2.rperiod_start_time;
		access_time.tv_sec += ip->di2.rperiod_duration * 60;
		access_time.tv_nsec = 0;
	}
	SAM_TIMESPEC32_TO_TIMESPEC(&vap->va_atime, &access_time);
}


/*
 * ----- sam_getattr_vn - Get attributes call.
 *
 * Get attributes for a SAM-QFS file.
 */

/* ARGSUSED2 */
int				/* ERRNO if error, 0 if successful. */
sam_getattr_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* pointer to vnode. */
	vattr_t *vap,		/* vattr pointer. */
	int flag,		/* flag. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* pointer to vnode. */
	vattr_t *vap,		/* vattr pointer. */
	int flag,		/* flag. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	sam_node_t *ip;
	int have_write_lock = 0;

	ip = SAM_VTOI(vp);
	if (RW_OWNER_OS(&ip->inode_rwl) == curthread) {
		/*
		 * There are paths via sam_get_special_file
		 * that get here with inode_rwl held as RW_WRITER.
		 */
		have_write_lock = 1;
	}
#ifdef METADATA_SERVER
	if (SAM_IS_SHARED_READER(ip->mp) && SAM_THREAD_IS_NFS()) {
		int error;

		if (ip->di.id.ino != SAM_INO_INO) {
			if (!have_write_lock) {
				RW_LOCK_OS(&ip->inode_rwl, RW_READER);
			}
			error = sam_refresh_shared_reader_ino(ip, FALSE, credp);
			if (!have_write_lock) {
				RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			}
			if (error) {
				return (error);
			}
		}
	}
#endif
	/*
	 * Solaris 10 FCS NFSv4 no longer uses this AT_SIZE optimization.
	 * lseek(fd, ..., SEEK_END) still uses it, tho'.
	 */
	if (vap->va_mask == AT_SIZE) {
		if (!have_write_lock) {
			RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		}
		vap->va_size = ip->di.rm.size;
		if (!have_write_lock) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		}
		return (0);
	}
	if (!have_write_lock) {
		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
	}
	if (S_ISREQ(ip->di.mode) && ip->di.rm.size == MAXOFFSET_T) {
		vap->va_size = 0;
	} else if (S_ISLNK(ip->di.mode) && (ip->di.ext_attrs & ext_sln)) {
		vap->va_size = ip->di.psize.symlink;
	} else {
		vap->va_size = ip->di.rm.size;
	}
	vap->va_type = vp->v_type;		/* Vnode type (for create) */
	vap->va_mode = ip->di.mode & MODEMASK;
	vap->va_uid = ip->di.uid;		/* Owner user id */
	vap->va_gid = ip->di.gid;		/* Owner group id */
	vap->va_fsid = ip->mp->mi.m_vfsp->vfs_dev;
	vap->va_nodeid = ip->di.id.ino;		/* Node id */
	vap->va_nlink = ip->di.nlink;		/* num references to file */
	/*
	 * Return inode times: last access, last modification, & inode changed.
	 * If the WORM bit is set on a file then this is a request to translate
	 * the access time.  The retention period stored in the modify and
	 * attribute time fields are used to generate the access time.
	 */
	if (ip->di.status.b.worm_rdonly && !S_ISDIR(ip->di.mode) &&
	    (ip->di.version >= SAM_INODE_VERS_2)) {
		sam_trans_access(vap, ip);
	} else {
		SAM_TIMESPEC32_TO_TIMESPEC(&vap->va_atime, &ip->di.access_time);
	}
	SAM_TIMESPEC32_TO_TIMESPEC(&vap->va_mtime, &ip->di.modify_time);
	SAM_TIMESPEC32_TO_TIMESPEC(&vap->va_ctime, &ip->di.change_time);
	if ((vp->v_type == VCHR) || (vp->v_type == VBLK)) {
		vap->va_rdev = vp->v_rdev;
	} else {
		vap->va_rdev = ip->rdev;	/* Device the file represents */
	}
	vap->va_blksize = LG_BLK(ip->mp, DD);	/* Block size in bytes */
	/* # of DEV_BSIZE (512-byte) blocks allocated */
	vap->va_nblocks = (u_longlong_t)ip->di.blocks *
	    (u_longlong_t)(SAM_BLK/DEV_BSIZE);

#if defined(AT_SEQ)
	vap->va_seq = 0;			/* Sequence number */
#else
	vap->va_vcode = 0;			/* Cache coherency inode code */
#endif
	if (!have_write_lock) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	}
	return (0);
}


/*
 * ----- sam_access_vn - Get access permissions.
 * Get access permissions for a SAM-QFS file.
 */

/* ARGSUSED4 */
int				/* ERRNO if error, 0 if successful. */
sam_access_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* pointer to vnode. */
	int mode,		/* mode. */
	int flags,		/* flags (not used). */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* pointer to vnode. */
	int mode,		/* mode. */
	int flags,		/* flags (not used). */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	int error;
	sam_node_t *ip;

	TRACE(T_SAM_ACCESS, vp, mode, flags, 0);
	ip = SAM_VTOI(vp);
	error = sam_access_ino(ip, mode, FALSE, credp);
	TRACE(T_SAM_ACCESS_RET, vp, ip->di.mode, error, ip->di.id.ino);
	return (error);
}


/*
 * ----- sam_readdir_vn - Return a SAM-QFS directory.
 *
 * Read the directory and return the entries in the
 * file system independent format. NOTE, inode data RWLOCK held.
 */

/* ARGSUSED4 */
int				/* ERRNO if error, 0 if successful. */
sam_readdir_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* directory vnode to be returned. */
	uio_t *uiop,		/* user I/O vector array. */
	cred_t *credp,		/* credentials pointer */
	int *eofp,		/* If not NULL, return eof flag . */
	caller_context_t *ct,	/* caller context pointer. */
	int flag		/* File flags. */
#else
	vnode_t *vp,		/* directory vnode to be returned. */
	uio_t *uiop,		/* user I/O vector array. */
	cred_t *credp,		/* credentials pointer */
	int *eofp		/* If not NULL, return eof flag . */
#endif
)
{
	sam_node_t *ip;
	int error;
	offset_t offset = uiop->uio_offset;

	TRACE(T_SAM_READDIR, vp, (sam_tr_t)uiop->uio_offset,
	    (uint_t)uiop->uio_iov->iov_len, (uint_t)credp);
	ip = SAM_VTOI(vp);
#ifdef METADATA_SERVER
	if (SAM_IS_SHARED_READER(ip->mp)) {
		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		error = sam_refresh_shared_reader_ino(ip, FALSE, credp);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		if (error) {
			return (error);
		}
	}
#endif

	/*
	 * Quit if we're at EOF or link count is zero (posix).
	 */
	if ((offset >= ip->di.rm.size) || ((int)ip->di.nlink <= 0)) {
		if (eofp) {
			*eofp = 1;
		}
		return (0);
	}
	if ((error = sam_getdents(ip, uiop, credp, eofp, FMT_FS)) == 0) {
		if (!(ip->mp->mt.fi_mflag & MS_RDONLY)) {
			RW_LOCK_OS(&ip->inode_rwl, RW_READER);
			mutex_enter(&ip->fl_mutex);
			TRANS_INODE(ip->mp, ip);
			sam_mark_ino(ip, (SAM_ACCESSED));
			mutex_exit(&ip->fl_mutex);
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		}
		if (SAM_THREAD_IS_NFS()) {
			ip->nfs_ios++;
		}
	}
	TRACE(T_SAM_READDIR_RET, vp, (sam_tr_t)uiop->uio_offset, error, 0);
	return (error);
}


/*
 * ----- sam_readlink_vn - Read a SAM-QFS symbolic link.
 *
 * Return the symbolic link name.
 */

/* ARGSUSED3 */
int				/* ERRNO if error, 0 if successful. */
sam_readlink_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* pointer to vnode. */
	uio_t *uiop,		/* User I/O vector array. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* pointer to vnode. */
	uio_t *uiop,		/* User I/O vector array. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	int error;
	sam_node_t *ip;

	ip = SAM_VTOI(vp);
	TRACE(T_SAM_READLINK, vp, ip->di.id.ino,
	    (sam_tr_t)uiop->uio_offset, (sam_tr_t)uiop->uio_resid);

	if (ip->di.version >= SAM_INODE_VERS_2) {
		if ((ip->di.ext_attrs & ext_sln) == 0) {
			dcmn_err((CE_WARN, "SAM-QFS: %s: symlink has "
			    "zero ext_sln flag",
			    ip->mp->mt.fi_name));
		}
		ASSERT(ip->di.ext_attrs & ext_sln);

		/* Retrieve symlink name from inode extension */
		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		error = sam_get_symlink(ip, uiop);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);

#ifdef METADATA_SERVER
	} else if (ip->di.version == SAM_INODE_VERS_1) {
		error = sam_get_old_symlink(ip, uiop, credp);
	} else {
		sam_req_ifsck(ip->mp, -1,
		    "sam_readlink_vn: version", &ip->di.id);
		error = ENOCSI;
#else
	} else {
		error = EINVAL;
#endif
	}

	TRACE(T_SAM_READLINK_RET, vp, ip->di.id.ino,
	    (sam_tr_t)uiop->uio_offset, (sam_tr_t)uiop->uio_resid);
	return (error);
}


/*
 * ----- sam_fsync_vn - Flush this vn to disk.
 *
 * Synchronize the vnode's in-memory state on disk.
 */

/* ARGSUSED2 */
int				/* ERRNO if error, 0 if successful. */
sam_fsync_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* pointer to vnode. */
	int filemode,		/* open filemode. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* pointer to vnode. */
	int filemode,		/* open filemode. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	sam_node_t *ip;
	int error = 0;

	ip = SAM_VTOI(vp);
	TRACE(T_SAM_FSYNC, vp, filemode, 0, 0);

	TRANS_BEGIN_ASYNC(ip->mp, TOP_FSYNC, TOP_SYNCIP_SIZE);

	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);

	/*
	 * If file is offline, the file is not modified, therefore complete
	 * fsync.
	 */
	if (ip->di.status.b.offline) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		TRANS_END_ASYNC(ip->mp, TOP_FSYNC, TOP_SYNCIP_SIZE);
		return (0);
	}

	/*
	 * File data integrity while writing or reading.
	 */
	if (filemode & (FRSYNC|FDSYNC|FSYNC)) {
		error = sam_flush_pages(ip, 0);
	}
	/*
	 * Inode integrity while writing or reading.
	 */
	if (filemode & FSYNC) {
		/*
		 * Cannot use inode_rwl to serialize .inodes; it's acquired in
		 * sam_read_ino.
		 */
		if (ip->di.id.ino == SAM_INO_INO) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			mutex_enter(&ip->mp->ms.m_inode_mutex);
		}
		(void) sam_update_inode(ip, SAM_SYNC_ONE, FALSE);
		if (ip->di.id.ino == SAM_INO_INO) {
			mutex_exit(&ip->mp->ms.m_inode_mutex);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
	}
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	TRANS_END_ASYNC(ip->mp, TOP_FSYNC, TOP_SYNCIP_SIZE);
	if (error == 0) {
		TRANS_BEGIN_SYNC(ip->mp, TOP_FSYNC, TOP_COMMIT_SIZE, error);
		if (error) {
			error = 0;	/* Commit wasn't needed */
			goto out;
		}
		TRANS_END_SYNC(ip->mp, error, TOP_FSYNC, TOP_COMMIT_SIZE);
	}
out:
	TRACE(T_SAM_FSYNC_RET, vp, error, 0, 0);
	return (error);
}


/*
 * ----- sam_inactive_vn - Inode is inactive, deallocate it.
 *
 * The vnode is inactive, write out the inode.
 */

/* ARGSUSED2 */
void
sam_inactive_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* pointer to vnode. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* pointer to vnode. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	sam_node_t *ip;

	ip = SAM_VTOI(vp);
	TRACE(T_SAM_INACTIVE, vp, vp->v_count, ip->flags.bits, ip->di.id.ino);
	sam_inactive_ino(ip, credp);
}


/*
 * ----- sam_inactive_stale_vn - Inode is inactive and stale (attached to
 * a forcibly unmounted FS).  Deallocate it.
 */

/* ARGSUSED2 */
void
sam_inactive_stale_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* pointer to vnode. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* pointer to vnode. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	sam_node_t *ip = SAM_VTOI(vp);

	TRACE(T_SAM_INACT_STALE, vp, vp->v_count,
	    ip->flags.bits, ip->di.id.ino);
	sam_inactive_stale_ino(ip, credp);
}


/*
 * ----- sam_fid_vn - Return file identifier.
 *
 * Return file identifier given vnode. Used by nfs.
 */

/* ARGSUSED2 */
int				/* ERRNO if error occured, 0 if successful. */
sam_fid_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* pointer to vnode. */
	fid_t *fidp,		/* pointer to file id (returned). */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* pointer to vnode. */
	fid_t *fidp		/* pointer to file id (returned). */
#endif
)
{
	sam_node_t *ip;
	sam_fid_t *sam_fidp = (sam_fid_t *)fidp;

	ip = SAM_VTOI(vp);

	if (fidp->fid_len < (sizeof (sam_fid_t) - sizeof (ushort_t))) {
		fidp->fid_len = sizeof (sam_fid_t) - sizeof (ushort_t);
		return (ENOSPC);
	}
	TRACE(T_SAM_FID, vp, ip->di.id.ino, ip->di.uid, ip->di.gid);
	bzero((char *)sam_fidp, sizeof (sam_fid_t));
	sam_fidp->fid_len = sizeof (sam_fid_t) - sizeof (ushort_t);
	sam_fidp->un._fid.id = ip->di.id;
	TRACE(T_SAM_FID_RET, vp, sam_fidp->un._fid.id.ino,
	    sam_fidp->un._fid.id.gen, 0);
	return (0);
}


/*
 * ----- sam_rwlock_common - Set r/w lock for vnode.
 *
 * Does the actual locking for sam_rwlock_vn.
 * Is called directly in some cases to prevent nested calls to
 * sam_open_operation_nb.
 */
void
sam_rwlock_common(
	vnode_t *vp,		/* Pointer to vnode. */
	int w)			/* Lock flag. */
{
	sam_node_t *ip;

	ip = SAM_VTOI(vp);
	if (w) {
		if (SAM_THREAD_IS_NFS() || ((ip->flags.b.qwrite == 0) &&
		    !(ip->mp->mt.fi_config & MT_QWRITE))) {
			TRACE(T_SAM_RWLOCK, vp, 1, 1, 0);
			RW_LOCK_OS(&ip->data_rwl, RW_WRITER);
		} else {
			TRACE(T_SAM_RWLOCK, vp, 1, 0, 0);
			RW_LOCK_OS(&ip->data_rwl, RW_READER);
		}
		/*
		 * If shared and SAN is holding a lock,
		 * flush+invalidate any pages hanging around.
		 */
		if (ip->flags.b.hold_blocks) {
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			sam_flush_pages(ip, B_INVAL);
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
	} else {
		TRACE(T_SAM_RWLOCK, vp, 0, 0, 0);
		RW_LOCK_OS(&ip->data_rwl, RW_READER);
	}
	TRACE(T_SAM_RWLOCK_RET, vp, w, 0, 0);
}


/*
 * ----- sam_rwlock_vn - Set r/w lock for vnode.
 *
 * Set writer or reader data lock for vnode depending on w flag.
 * If mount parameter qwrite is on or inode flags qwrite is on,
 * the writer lock is not set to allow simultaneous reads & writes
 * to the file.
 */

/* ARGSUSED2 */
void
sam_rwlock_vn(
	vnode_t *vp,		/* Pointer to vnode. */
	int w,			/* Lock flag. */
	caller_context_t *ct)	/* Caller context pointer. */
{
	sam_node_t *ip;

	ip = SAM_VTOI(vp);

	if (S_ISDIR(ip->di.mode)) {
		sam_open_operation_rwl(ip);
	}

	sam_rwlock_common(vp, w);
}


/*
 * ----- sam_rwdlock_ino - Set r/w data lock for vnode.
 *
 * Used instead of sam_rwlock_ino(&ip->data_rwl, rw_type).
 * Allows a thread to sleep waiting for the data_rwl lock
 * and not hold up failover.
 *
 */
void
sam_rwdlock_ino(
	sam_node_t *ip,	/* Pointer to vnode. */
	krw_t rw_type,	/* Lock type. */
	int vop_eq)		/* Need VOP_RWLOCK behaviour */
{
	sam_mount_t *mp;
	krw_t use_rw_type = rw_type;

	mp = ip->mp;

	if (vop_eq && (rw_type == RW_WRITER)) {
		if (!SAM_THREAD_IS_NFS() && (ip->flags.b.qwrite ||
		    (ip->mp->mt.fi_config & MT_QWRITE))) {

			use_rw_type = RW_READER;
		}
	}

	if (SAM_IS_SHARED_CLIENT(mp)) {
#ifdef DEBUG
		sam_operation_t ep;
		ep.ptr = tsd_get(mp->ms.m_tsd_key);
		ASSERT(ep.ptr != NULL);
#endif
		/*
		 * An existing tsd entry means we were called
		 * from within QFS which should have an active operation.
		 */
		ASSERT(mp->ms.m_cl_active_ops > 0);

		TRACE(T_SAM_RWDLOCK, SAM_ITOV(ip), rw_type, use_rw_type, 0);

		if (rw_tryenter(&ip->data_rwl, use_rw_type) == 0) {

			SAM_DEC_OPERATION(mp);
			RW_LOCK_OS(&ip->data_rwl, use_rw_type);

			/*
			 * Before incrementing m_cl_active_ops
			 * check for failover. Failover may already
			 * be proceeding with m_cl_active_ops
			 * of zero.
			 */
			while (mp->mt.fi_status &
			    (FS_LOCK_HARD | FS_UMOUNT_IN_PROGRESS)) {

				if (sam_is_fsflush()) {
					break;
				}
				/*
				 * If failover started while we acquired
				 * the lock freeze here.
				 */
				(void) sam_freeze_ino(mp, ip, 1);
			}
			SAM_INC_OPERATION(mp);
		}
		TRACE(T_SAM_RWDLOCK_RET, SAM_ITOV(ip), rw_type, use_rw_type, 0);

	} else {

		RW_LOCK_OS(&ip->data_rwl, use_rw_type);

	}

	/*
	 * If shared and SAN is holding a lock,
	 * flush+invalidate any pages hanging around.
	 */
	if (vop_eq && (rw_type == RW_WRITER) &&
	    ip->flags.b.hold_blocks) {

		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		sam_flush_pages(ip, B_INVAL);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	}
}

/*
 * ----- sam_rwunlock_vn - Clear r/w lock for vnode.
 *
 * Clear r/w lock for vnode.
 */

/* ARGSUSED2 */
void
sam_rwunlock_vn(
	vnode_t *vp,		/* Pointer to vnode. */
	int w,			/* Lock flag. */
	caller_context_t *ct)	/* Caller context pointer. */
{
	sam_node_t *ip;

	TRACE(T_SAM_UNRWLOCK, vp, w, 0, 0);
	ip = SAM_VTOI(vp);
	/*
	 * If shared and SAN is holding a lock,
	 * flush+invalidate any pages hanging around.
	 */
	if (ip->flags.b.hold_blocks) {
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		sam_flush_pages(ip, B_INVAL);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	}
	RW_UNLOCK_CURRENT_OS(&ip->data_rwl);

	if (S_ISDIR(ip->di.mode)) {
		SAM_CLOSE_OPERATION(ip->mp, 0);
	}

	TRACE(T_SAM_UNRWLOCK_RET, vp, w, 0, 0);
}


/*
 * ----- sam_seek_vn - Seek to vnode offset.
 *
 * Seek to vnode offset. Disallow seek on tape direct access I/O.
 */

/* ARGSUSED1 */
int				/* EINVAL if out of bounds, 0 if successful. */
sam_seek_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* Pointer to vnode. */
	offset_t ooff,		/* Old offset. */
	offset_t *noffp,	/* Pointer to new offset */
	caller_context_t *ct	/* Caller context pointer. */
#else
	vnode_t *vp,		/* Pointer to vnode. */
	offset_t ooff,		/* Old offset. */
	offset_t *noffp		/* Pointer to new offset */
#endif
)
{
	sam_node_t *ip;
	int error = 0;

	ip = SAM_VTOI(vp);
	if (S_ISREQ(ip->di.mode)) {
		error = EINVAL;
	} else if ((*noffp < 0) || (*noffp > SAM_MAXOFFSET_T)) {
		error = EINVAL;
	}
	return (error);
}


/*
 * ----- sam_getpage_vn - Get pages for a SAM-QFS file.
 *
 * Get pages for a file for use in a memory mapped function.
 * Return the page list with all the pages for the file from
 * offset to (offset + length).
 */

/* ARGSUSED10 */
int				/* ERRNO if error, 0 if successful. */
sam_getpage_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* vnode entry */
	offset_t offset,	/* file offset */
	sam_size_t length,	/* file length provided to mmap */
	uint_t *protp,		/* requested protection <sys/mman.h> */
	page_t **pglist,	/* page list array (returned) */
	sam_size_t plsize,	/* max byte size in page list array */
	struct seg *segp,	/* page segment */
	caddr_t addr,		/* page address */
	enum seg_rw rw,		/* access type for the fault operation */
	cred_t *credp,		/* credentials */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* vnode entry */
	offset_t offset,	/* file offset */
	sam_size_t length,	/* file length provided to mmap */
	uint_t *protp,		/* requested protection <sys/mman.h> */
	page_t **pglist,	/* page list array (returned) */
	sam_size_t plsize,	/* max byte size in page list array */
	struct seg *segp,	/* page segment */
	caddr_t addr,		/* page address */
	enum seg_rw rw,		/* access type for the fault operation */
	cred_t *credp		/* credentials */
#endif
)
{
	int error = 0;
	sam_node_t *ip;
	boolean_t lock_set = B_FALSE;
	offset_t size;
	int appending;
	int blocks, sparse, bt, dt;
	boolean_t map_blocks = B_TRUE;
	int non_shared;

	TRACE(T_SAM_GETPAGE, vp, (sam_tr_t)offset, length, rw);
	if (vp->v_flag & VNOMAP) {
		return (ENOSYS);
	}
	ip = SAM_VTOI(vp);
	non_shared = !SAM_IS_SHARED_FS(ip->mp);
	if (protp != NULL) {
		*protp = PROT_ALL;
	}

	/*
	 * Acquire the inode READER lock if WRITER lock not already held.
	 */
retry:
	if (RW_OWNER_OS(&ip->inode_rwl) != curthread) {
		/* Enforce the sam_put/getpage_vn locking rule */
		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		lock_set = B_TRUE;
	}

	if (non_shared && (error = sam_open_ino_operation(ip,
	    lock_set ? RW_READER : RW_WRITER))) {
		if (lock_set) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		}
		TRACE(T_SAM_GETPAGE_RET, vp, error, (sam_tr_t)pglist, 0);
		return (error);
	}

	/*
	 * If file is offline, begin stage. Return when offset+length is online.
	 * If stage_n file is always offline & window is already staged by read.
	 */
	size = ip->size;
	if (S_ISSEGI(&ip->di)) {
		size = ip->di.rm.info.dk.seg.fsize;
	}
	if (ip->di.status.b.offline) {
		int ret;

		if ((error = sam_idle_ino_operation(ip,
		    lock_set ? RW_READER : RW_WRITER))) {
			goto out;
		}
		if ((error = sam_proc_offline(ip, offset,
		    (offset_t)length, &size,
		    credp, &ret))) {
			goto out;
		} else if (ret == EAGAIN && !SAM_THREAD_IS_NFS()) {
			/* trigger re-try from fault code */
			SAM_COUNT64(page, retry);
			error = EDEADLK;
			goto out;
		}
	}

	/*
	 * Check for appending past the EOF.
	 */
	appending = (offset + (offset_t)length) >
	    ((size + PAGEOFFSET) & (~((offset_t)PAGEOFFSET)));

	if (appending && (!sam_vpm_enable || (segp != segkmap))) {
		error = EFAULT;
		goto out;
	}

	/*
	 * Check for sparse file. Blocks are in units of 4 kilobytes.
	 */
	dt = ip->di.status.b.meta;
	if (ip->di.status.b.on_large || (ip->size > SM_OFF(ip->mp, dt))) {
		bt = LG;
	} else {
		bt = SM;
	}
	if (ip->mp->mi.m_fs[ip->di.unit].num_group == 1) {
		blocks = (ip->size + (ip->mp->mi.m_dau[dt].size[bt] - 1)) >>
		    SAM_SHIFT;
	} else {
		blocks = (ip->size + ((ip->mp->mi.m_dau[dt].size[bt] *
		    ip->mp->mi.m_fs[ip->di.unit].num_group) - 1)) >> SAM_SHIFT;
	}
	sparse = (blocks != ip->di.blocks);

	/*
	 * If memory mapping to a sparse file, allocate blocks.
	 * SAM_WRITE_SPARSE causes pages to be created in memory.
	 * If we hit the deadlock condition in sam_getpages and we're
	 * in the retry loop, do not re-map blocks.
	 */
	if (map_blocks && ((rw == S_WRITE) || (rw == S_CREATE)) && sparse) {
		if (lock_set && (rw_tryupgrade(&ip->inode_rwl) == 0)) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
		if (!S_ISREQ(ip->di.mode)) {
			while (((error = sam_map_block(ip, offset,
			    (offset_t)length,
			    (SAM_WRITE_SPARSE | SAM_MAP_NOWAIT),
			    NULL, credp)) != 0) &&
			    IS_SAM_ENOSPC(error)) {
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				error = sam_wait_space(ip, error);
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				if (error) {
					break;
				}
			}
			if (error == ELNRNG) {
				if (!ip->di.status.b.on_large &&
				    (offset < SM_OFF(ip->mp, dt))) {
					error = 0;
				}
			}
		} else {
			if (ip->rdev == 0) {
				if ((error = ip->rm_err) == 0) {
					error = ENODEV;
				}
			} else {
				error = sam_rmmap_block(ip, offset,
				    (offset_t)length,
				    SAM_WRITE_SPARSE, NULL);
			}
		}
		if (error) {
			goto out;
		}
		if (lock_set) {
			rw_downgrade(&ip->inode_rwl);
		}
	}

	/*
	 * Comment stolen from UFS:
	 *
	 * We remove PROT_WRITE in cases when the file has holes
	 * because we don't want to call bmap_read() to check each
	 * page if it is backed with a disk block.
	 */
	if (protp && sparse && (rw != S_WRITE) && (rw != S_CREATE)) {
		*protp &= ~PROT_WRITE;
	}

	/*
	 * There is a potential deadlock condition when more than one
	 * thread is trying to access the same page.  This occurs when
	 * more than one thread race for the same page after page
	 * faulting.  If the deadlock can occur, drop the inode rw lock
	 * and retry the getpage.  See the comment in sam_getpages.
	 */
	error = sam_getpages(vp, (sam_offset_t)offset, length, pglist, plsize,
	    segp, addr, rw, sparse, lock_set);

	if ((error == EDEADLK) && lock_set) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		error = 0;
		map_blocks = B_FALSE;
		delay(1);
		if (non_shared) {
			SAM_CLOSE_OPERATION(ip->mp, error);
		}
		goto retry;
	}

	/*
	 * Update inode accessed and modified time for mmap I/O.
	 * rw_count == 0 indicates an mmap request.
	 *
	 * Note: There is a race condition here; a page fault could occur while
	 * we are processing a normal read or write.  This is generally
	 * harmless,
	 * but it means that we could miss a page fault which will modify
	 * the file
	 * during a read operation.  Since we will mark the file as updated once
	 * putpage is called, this isn't too serious.
	 *
	 * If lock_set is true, we have a reader lock on the inode, and must use
	 * a mutex to protect the flags.  If lock_set is false, our caller has a
	 * writer lock on the inode, and we don't need to grab the mutex.
	 *
	 * Staging a file will also have rw_count == 0.
	 * Calling sam_mark_ino in this case can cause the mod time to change
	 * and valid archives to be staled.
	 * If the the file is staging, don't call sam_mark_ino.
	 *
	 */
	if (ip->rw_count == 0 && !ip->flags.b.staging) {
		uint_t flags;

		flags = 0;
		if (rw == S_WRITE) {
			flags = SAM_UPDATED;
		} else if (rw != S_OTHER) {
			flags = SAM_ACCESSED;
		}
		if (flags && !(ip->mp->mt.fi_mflag & MS_RDONLY) &&
		    !S_ISSEGI(&ip->di)) {
			if (lock_set) {
				mutex_enter(&ip->fl_mutex);
			}
			TRANS_INODE(ip->mp, ip);
			sam_mark_ino(ip, flags);
			if (lock_set) {
				mutex_exit(&ip->fl_mutex);
			}
		}
	}
out:
	if (lock_set) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	}
	if (non_shared) {
		SAM_CLOSE_OPERATION(ip->mp, error);
	}
	TRACE(T_SAM_GETPAGE_RET, vp, error, (sam_tr_t)pglist, 0);
	return (error);
}


/*
 * ----- sam_putpage_vn - Put a single page from a SAM-QFS file.
 *
 * Put pages from a file for use in a memory mapped function.
 * If length == 0, put pages from offset to EOF. The typical
 * cases should be length == 0 & offset == 0 (entire vnode page list),
 * length == MAXBSIZE (segmap_release), and
 * length == PAGESIZE (from the pageout daemon or fsflush).
 */

/* ARGSUSED5 */
int				/* ERRNO if error, 0 if successful. */
sam_putpage_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* Pointer to vnode. */
	offset_t offset,	/* file offset */
	sam_size_t length,	/* file length provided to mmap. */
	int flags,		/* flags - B_INVAL, B_DIRTY, B_FREE, */
				/* B_DONTNEED, B_FORCE, B_ASYNC. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* Pointer to vnode. */
	offset_t offset,	/* file offset */
	sam_size_t length,	/* file length provided to mmap. */
	int flags,		/* flags - B_INVAL, B_DIRTY, B_FREE, */
				/* B_DONTNEED, B_FORCE, B_ASYNC. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	int error = 0;
	sam_node_t *ip;

	if (vp->v_flag & VNOMAP) {
		return (ENOSYS);
	}
	if (vn_has_cached_data(vp) == 0) {
		return (0);
	}

	/*
	 * If file system is under Sun Cluster management, and we are
	 * panicking, do not sync.  Sun Cluster will have fenced our
	 * disks and (if a client) our socket to server will be closed,
	 * and we may get stuck forever without a dump.
	 */
	ip = SAM_VTOI(vp);
	if (panicstr && (ip->mp->mt.fi_config1 & MC_CLUSTER_MGMT)) {
		return (0);
	}

	/*
	 * If the vnode is stale, we would need to invalidate all
	 * the pages of the vnode. Incoming request is changed to
	 * address all the pages of the vnode. B_ASYNC flag is cleared
	 * to prevent unnecessary extra processing as the vnode is stale.
	 */
	if (SAM_VP_IS_STALE(vp)) {
		offset = 0;
		length = 0;
		flags &= ~(B_ASYNC);

		/*
		 * Pages exist because we were not able to write to the OSN.
		 * Invalidate them because we still won't be able to flush them.
		 */
		if (SAM_IS_OBJECT_FILE(ip)) {
			dcmn_err((CE_WARN, "SAM-QFS: %s: STALE INODE %d.%d "
			    "iflags=%x, flags=%x, %x",
			    ip->mp->mt.fi_name, ip->di.id.ino, ip->di.id.gen,
			    ip->flags.bits, flags, B_INVAL));
			if (ip->flags.b.hash == 0) {
				flags = B_INVAL;
			}
		}
	}

	if (S_ISREQ(ip->di.mode)) {
		cmn_err(SAMFS_DEBUG_PANIC,
		    "SAM-QFS: %s: "
		    "sam_putpage: removable media file, ip=%p, ino=%d.%d",
		    ip->mp->mt.fi_name, (void *)ip, ip->di.id.ino,
		    ip->di.id.gen);
		return (ENODEV);
	}

	/*
	 * Make sure offset is on a page boundary.
	 */
	if ((offset & (offset_t)(PAGESIZE - 1)) != 0) {
		sam_size_t len_delta;

		len_delta = offset & (offset_t)(PAGESIZE - 1);
		offset &= ~((offset_t)(PAGESIZE - 1));
		length += len_delta;
	}

	TRACE(T_SAM_PUTPAGE, vp, (sam_tr_t)offset, length, flags);

	/*
	 * If failing over, defer file page flushing for NFS and fsflush as
	 * both will retry the operation in the future.
	 * Note, allow truncated pages and directory pages.
	 */
	if (SAM_IS_SHARED_CLIENT(ip->mp) &&
	    ((ip->mp->mt.fi_status & (FS_FREEZING | FS_FROZEN)) ||
	    ((ip->mp->mt.fi_status & FS_LOCK_HARD) &&
	    !(ip->mp->mt.fi_status & FS_THAWING)))) {
		if (!((flags & B_TRUNC) || S_ISDIR(ip->di.mode))) {
			if (SAM_THREAD_IS_NFS()) {
				curthread->t_flag |= T_WOULDBLOCK;
				error = EAGAIN;
				SAM_NFS_JUKEBOX_ERROR(error);
				return (error);
			}
			if (sam_is_fsflush()) {
				return (EIO);
			}
		}
	}

	if (vp->v_count == 0 && !S_ISSEGS(&ip->di)) {
		/* If vnode is inactive */
		cmn_err(SAMFS_DEBUG_PANIC,
		    "SAM-QFS: %s: sam_putpage:"
		    " inode inactive, ip=%p, ino=%d.%d, bits=%x\n",
		    ip->mp->mt.fi_name, (void *)ip, ip->di.id.ino,
		    ip->di.id.gen,
		    ip->flags.bits);
		return (EIO);
	}

	/*
	 * For async I/O, use writebehind buffering. This is done to write
	 * larger requests and to also minimize the overhead of
	 * read/modify/write.
	 */
	if (length && (flags & B_ASYNC) &&
	    ((flags & ~(B_ASYNC|B_DONTNEED|B_FREE)) == 0)) {

		mutex_enter(&ip->write_mutex);

		/*
		 * Start new writebehind buffer if one does not already exist
		 * and if software raid, offset is aligned.
		 */
		if (ip->klength == 0) {
			if (!(ip->mp->mt.fi_config & MT_SOFTWARE_RAID) ||
			    ((offset % ip->mp->mt.fi_writebehind) == 0)) {
				ip->klength = length;
				ip->koffset = offset;
				mutex_exit(&ip->write_mutex);
				goto out;
			}

		/*
		 * If write behind length is less than the writebehind size
		 * and the next offset is sequential to write behind buffer,
		 * increment the writebehind buffer.
		 */
		} else if ((ip->klength < ip->mp->mt.fi_writebehind) &&
		    (ip->koffset + (offset_t)ip->klength) == offset) {
			ip->klength += length;
			mutex_exit(&ip->write_mutex);
			goto out;

		/*
		 * Flush old write behind buffer and start a new one if
		 * software raid and offset is aligned, and another one
		 * has not already been started in the meantime.
		 */
		} else {
			offset_t koffset;
			sam_size_t klength;

			koffset = ip->koffset;
			klength = ip->klength;
			ip->koffset = 0;
			ip->klength = 0;
			mutex_exit(&ip->write_mutex);

			error = sam_putpages(vp, koffset, klength,
			    flags, credp);

			mutex_enter(&ip->write_mutex);
			if (ip->klength == 0) {
				if (!(ip->mp->mt.fi_config &
				    MT_SOFTWARE_RAID) ||
				    ((offset %
				    ip->mp->mt.fi_writebehind) == 0)) {
					ip->koffset = offset;
					ip->klength = length;
					mutex_exit(&ip->write_mutex);
					goto out;
				}
			}
		}
		mutex_exit(&ip->write_mutex);
	}
	error = sam_putpages(vp, offset, length, flags, credp);

out:
	TRACE(T_SAM_PUTPAGE_RET, vp, ip->di.id.ino, error, ip->di.mode);
	return (error);
}


/*
 * ----- sam_map_vn - Set up a vnode so its pages can be memory mapped.
 *
 * Set up a vnode so its pages can be memory mapped.
 */

/* ARGSUSED9 */
int				/* ERRNO if error, 0 if successful. */
sam_map_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* pointer to vnode. */
	offset_t offset,	/* file offset. */
	struct as *asp,		/* pointer to address segment. */
	char **addrpp,		/* pointer pointer to address. */
	sam_size_t length,	/* file length provided to mmap. */
	uchar_t prot,		/* requested protection from <sys/mman.h>. */
	uchar_t maxprot,	/* max requested prot from <sys/mman.h>. */
	uint_t flags,		/* flags. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* pointer to vnode. */
	offset_t offset,	/* file offset. */
	struct as *asp,		/* pointer to address segment. */
	char **addrpp,		/* pointer pointer to address. */
	sam_size_t length,	/* file length provided to mmap. */
	uchar_t prot,		/* requested protection from <sys/mman.h>. */
	uchar_t maxprot,	/* max requested prot from <sys/mman.h>. */
	uint_t flags,		/* flags. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	struct segvn_crargs vn_seg;
	int error;
	sam_node_t *ip;
	sam_u_offset_t off = offset;
	boolean_t is_write;

	TRACE(T_SAM_MAP, vp, (sam_tr_t)offset, length, prot);
	ip = SAM_VTOI(vp);
	if (ip->di.id.ino == SAM_INO_INO) {	/* If .inodes file */
		return (EPERM);
	}
	if (vp->v_flag & VNOMAP) {	/* File cannot be memory mapped */
		return (ENOSYS);
	}
	if ((offset < 0) || ((offset + length) > SAM_MAXOFFSET_T) ||
	    ((offset + length) < 0)) {
		return (EINVAL);
	}
	/*
	 * If file is not regular, file is a segment, or locked, cannot mmap
	 * file.
	 */
	if (vp->v_type != VREG || ip->di.status.b.segment) {
		return (ENODEV);
	}
	if (vn_has_flocks(vp) && MANDLOCK(vp, ip->di.mode)) {
		return (EAGAIN);
	}

	/*
	 * Mmap not supported for removable media files or stage_n
	 * (direct access).
	 */
	if (S_ISREQ(ip->di.mode)) {
		return (EINVAL);
	}
	if (ip->di.status.b.direct) {
		return (EINVAL);
	}

	/*
	 * If mmap write and the file is offline, stage in the entire file.
	 * (Only for shared mappings; treat private mappings as read-only.)
	 */
	is_write = (prot & PROT_WRITE) && ((flags & MAP_TYPE) == MAP_SHARED);

	if (is_write) {
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (ip->di.status.b.offline || ip->flags.b.stage_pages) {
			error = sam_clear_ino(ip, ip->di.rm.size,
			    MAKE_ONLINE, credp);
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			if (error) {
				return (error);
			}
		} else {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
	}
	as_rangelock(asp);			/* Lock address space */

	if ((flags & MAP_FIXED) == 0) {
#if (KERNEL_MINOR >= 4)
		map_addr(addrpp, length, off, flags);
#else
		map_addr(addrpp, length, off, 1, flags);
#endif
		if (*addrpp == NULL) {
			as_rangeunlock(asp);
			TRACE(T_SAM_MAP_RET, vp, ENOMEM, 0, 0);
			return (ENOMEM);
		}
	} else {
		/* Unmap any previous mappings */
		as_unmap(asp, *addrpp, length);
	}

	/* Create vnode mapped segment. */
	bzero((char *)&vn_seg, sizeof (vn_seg));
	vn_seg.vp = vp;
	vn_seg.cred = credp;
	vn_seg.offset = off;
	vn_seg.type = flags & MAP_TYPE;		/* Type of sharing done */
	vn_seg.prot = prot;
	vn_seg.maxprot = maxprot;
	vn_seg.flags = flags & ~MAP_TYPE;
	vn_seg.amp = NULL;

	error = as_map(asp, *addrpp, length, segvn_create, (caddr_t)& vn_seg);

	as_rangeunlock(asp);			/* Unlock address space */

	TRACE(T_SAM_MAP_RET, vp, error, 0, 0);
	return (error);
}


/*
 * ----- sam_pathconf_vn - POSIX pathconf support.
 *
 * Get configurable pathname variables. See fpathconf(2).
 */

/* ARGSUSED4 */
int			/* ERRNO if error, 0 if successful. */
sam_pathconf_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* pointer to vnode. */
	int cmd_a,		/* type of value requested. */
	ulong_t *valp,		/* returned value pointer. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* pointer to vnode. */
	int cmd_a,		/* type of value requested. */
	ulong_t *valp,		/* returned value pointer. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	sam_node_t	*ip = SAM_VTOI(vp);

	*valp = 0;
	/*
	 * Special case any file system specific pathconf() call.
	 */
	switch (cmd_a) {
	case _PC_FILESIZEBITS:
		/*
		 * Return bits of significance for _PC_FILESIZEBITS.
		 *
		 * The number of bits (ie the largest offset) which can
		 * be generated is dependent on the types of device
		 * (ie mr,md,gxxx) and the dau size.  The actual
		 * number of bits of significance for a file offset
		 * can be calculated as 10 (normalize to Kbytes) +
		 * the number of bits in the direct extents + the
		 * number of bits in the indirect extents (3 levels
		 * of indirection at 2K per level). The end result
		 * is 43 + the number of bits to represent the direct
		 * extents (this is variable depending on device
		 * type and dau size). The calculation below accounts
		 * for importing a 4.0 fs into 4.1.  The "ext_bshift"
		 * represents the number of bits in a block. Subtracting
		 * this from the smallest value (minus a little) yields
		 * 36 as a base.
		 */
		*valp = 36 + ip->mp->mi.m_sbp->info.sb.ext_bshift;
		return (0);
	case _PC_ACL_ENABLED:
		*valp = _ACL_ACLENT_ENABLED;
		return (0);
	case _PC_XATTR_EXISTS:
		/*
		 * Does this vnode have any extended attributes?
		 */
		if (vp->v_vfsp->vfs_flag & VFS_XATTR) {
			sam_node_t *ip;
			sam_node_t *pip = SAM_VTOI(vp);
			int error;

			if (pip->mp->mt.fi_config1 & MC_NOXATTR) {
				*valp = 0;
				return (0);
			}
			ip = NULL;
			RW_LOCK_OS(&pip->inode_rwl, RW_READER);
			error = sam_xattr_getattrdir(vp, &ip,
			    LOOKUP_XATTR, credp);
			RW_UNLOCK_OS(&pip->inode_rwl, RW_READER);
			if (error == 0 && ip != NULL) {
				/*
				 * Check empty directory only if MDS. Otherwise,
				 * return extended attributes exists on shared
				 * client even if there is an empty hidden
				 * extended attributes directory.
				 */
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				error = sam_empty_dir(ip);
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				if (error == 0) {
					*valp = 0;
				} else if (error == ENOTEMPTY) {
					*valp = 1;
					error = 0;
				}
				VN_RELE(SAM_ITOV(ip));
				return (error);
			} else if (error == ENOENT) {
				*valp = 0;
				return (0);
			}
		} else {
			*valp = 0;
			return (0);
		}
		/* NOTREACHED */
		break;
	}

	return (FS_PATHCONF_OS(vp, cmd_a, valp, credp, NULL));
}


/*
 * ----- sam_ioctl_file_cmd - ioctl file command.
 *
 *	Called when the user issues an file "f" ioctl command for
 *	an opened file on the SAM-QFS file system.
 */

int				/* ERRNO if error, 0 if successful. */
sam_ioctl_file_cmd(
	sam_node_t *ip,		/* Pointer to inode. */
	int	cmd,		/* Command number. */
	int	*arg,		/* Pointer to arguments. */
	int flag,		/* Datamodel */
	int	*rvp,		/* Returned value pointer */
	cred_t *credp)		/* Credentials pointer. */
{
	int	error = 0;

	switch (cmd) {

	case C_LISTIO_WR:		/* Listio write file access */
		error = sam_proc_listio(ip, FWRITE, arg, flag, rvp, credp);
		break;

	case C_LISTIO_RD:		/* Listio read file access */
		error = sam_proc_listio(ip, FREAD, arg, flag, rvp, credp);
		break;

	case C_LISTIO_WAIT:		/* Listio wait file access completion */
		error = sam_wait_listio(ip, *(caddr_t *)arg, curproc->p_pid);
		break;

	case C_ADVISE: {		/* Advise file operating attributes */
		char *op;
		int i = 0;

		op = (char *)(void *)arg;
		sam_rwdlock_ino(ip, RW_READER, 0);
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		while (*op != '\0' && error == 0 && ++i <= 4) {
			switch (*op++) {
			case 'b':		/* Buffered I/O */
				sam_set_directio(ip, DIRECTIO_OFF);
				break;
			case 'd':		/* Return to defaults */
				sam_reset_default_options(ip);
				break;
			case 'r':		/* Direct (raw) I/O */
				sam_set_directio(ip, DIRECTIO_ON);
				break;
			case 'p': /* DOES NOTHING Superuser locked buffers */
				/* This implementation was removed as */
				/* unneeded. */
				break;
			case 'w':
				/*
				 * Simultaneous writes - no WRITERS data lock
				 */
				ip->flags.b.qwrite = 1;
				break;
			default:
				error = EINVAL;
				break;
			}
		}
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		RW_UNLOCK_OS(&ip->data_rwl, RW_READER);
		}
		break;

	case C_FIOLFSS:			/* Get fs Locking status */
	case C_FIOLFS:			/* Lock fs on/off */
		error = sam_proc_lockfs(ip, cmd, arg, flag, rvp, credp);
		break;

	case C_DIRECTIO: {		/* Turn direct I/O on/off */
		uint64_t *flag;

		flag = (uint64_t *)(void *)arg;
		sam_rwdlock_ino(ip, RW_READER, 0);
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		sam_set_directio(ip, (int)*flag);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		RW_UNLOCK_OS(&ip->data_rwl, RW_READER);
		}
		break;

	case C_FIOLOGENABLE:
		if (secpolicy_fs_config(credp, ip->mp->mi.m_vfsp) != 0) {
			return (EPERM);
		}
		error = qfs_fiologenable(SAM_ITOV(ip), (void *)arg,
		    credp, flag);
		break;

	case C_FIOLOGDISABLE:
		if (secpolicy_fs_config(credp, ip->mp->mi.m_vfsp) != 0) {
			return (EPERM);
		}
		error = qfs_fiologdisable(SAM_ITOV(ip), (void *)arg,
		    credp, flag);
		break;

	case C_FIOISLOG:
		return (qfs_fioislog(SAM_ITOV(ip), (void *)arg, credp, flag));

	case C_FIO_SET_LQFS_DEBUG:
		if (secpolicy_fs_config(credp, ip->mp->mi.m_vfsp) != 0) {
			return (EPERM);
		}
		if (ddi_copyin(*((caddr_t *)arg), &lqfs_debug, sizeof (uint_t),
		    flag)) {
			return (EFAULT);
		}
		error = 0;
		break;

	default:
		error = ENOTTY;
		break;

	}
	return (error);
}


/*
 * ----- sam_proc_listio - ioctl listio command.
 *
 *	Called when the user issues an file read/write listio ioctl command for
 *	an opened file on the SAM-QFS file system.
 */

/* ARGSUSED4 */
static int			/* ERRNO if error, 0 if successful. */
sam_proc_listio(
	sam_node_t *ip,		/* Pointer to inode. */
	int	mode,		/* FWRITE or FREAD */
	int	*arg,		/* Pointer to arguments. */
	int flag,		/* Datamodel */
	int	*rvp,		/* Returned value pointer */
	cred_t *credp)		/* Credentials pointer.	*/
{
	struct sam_listio_call *callp, *niop;
	int error = 0;


	callp = (struct sam_listio_call *)kmem_zalloc(
	    sizeof (struct sam_listio_call), KM_SLEEP);
	sema_init(&callp->cmd_sema, 0, NULL, SEMA_DEFAULT, NULL);
	sam_mutex_init(&callp->lio_mutex, NULL, MUTEX_DEFAULT, NULL);

	if ((flag & DATAMODEL_MASK) == DATAMODEL_LP64) {
		if (copyin(*((caddr_t *)arg), &callp->listio,
				sizeof (struct sam_listio))) {
			sam_free_listio(callp);
			return (EFAULT);
		}

		if (!callp->listio.mem_list_count ||
		    !callp->listio.file_list_count) {
			sam_free_listio(callp);
			return (EINVAL);
		}
		callp->mem_cnt = callp->listio.mem_list_count;
		callp->file_cnt = callp->listio.file_list_count;

		/*
		 * Read in the memory arrays.
		 */
		callp->mem_addr = (caddr_t *)kmem_alloc(
		    (sizeof (caddr_t) * callp->listio.mem_list_count),
		    KM_SLEEP);
		if (copyin((caddr_t)callp->listio.mem_addr, callp->mem_addr,
		    (sizeof (caddr_t) * callp->listio.mem_list_count))) {
			sam_free_listio(callp);
			return (EFAULT);
		}
		callp->mem_count = (size_t *)kmem_alloc(
		    (sizeof (size_t) * callp->listio.mem_list_count),
		    KM_SLEEP);
		if (copyin((caddr_t)callp->listio.mem_count, callp->mem_count,
		    (sizeof (size_t) * callp->listio.mem_list_count))) {
			sam_free_listio(callp);
			return (EFAULT);
		}
	} else {
		struct sam_listio32 listio32;
		caddr32_t *mem_addr32 = NULL;
		uint32_t *mem_count32 = NULL;
		int err = 0;

		if (copyin(*((caddr_t *)arg),
				&listio32, sizeof (struct sam_listio32))) {
			sam_free_listio(callp);
			return (EFAULT);
		}

		callp->listio.wait_handle = (void *)listio32.wait_handle;
		callp->listio.mem_list_count = listio32.mem_list_count;
		callp->listio.mem_addr = (void **)listio32.mem_addr;
		callp->listio.mem_count = (size_t *)listio32.mem_count;
		callp->listio.file_list_count = listio32.file_list_count;
		callp->listio.file_off = (offset_t *)listio32.file_off;
		callp->listio.file_len = (offset_t *)listio32.file_len;
		callp->mem_cnt = callp->listio.mem_list_count;
		callp->file_cnt = callp->listio.file_list_count;

		if (!callp->listio.mem_list_count ||
		    !callp->listio.file_list_count) {
			sam_free_listio(callp);
			return (EINVAL);
		}

		/*
		 * Read in the memory arrays.
		 */
		do {
			int i;

			mem_addr32 = (caddr32_t *)kmem_alloc(
			    (sizeof (caddr32_t) * callp->listio.mem_list_count),
			    KM_SLEEP);
			if (copyin((caddr_t)listio32.mem_addr, mem_addr32,
			    (sizeof (caddr32_t) * listio32.mem_list_count))) {
				err = EFAULT;
				break;
			}
			callp->mem_addr = (caddr_t *)kmem_alloc(
			    (sizeof (caddr_t) * callp->listio.mem_list_count),
			    KM_SLEEP);

			mem_count32 = (uint32_t *)kmem_alloc(
			    (sizeof (uint32_t) * callp->listio.mem_list_count),
			    KM_SLEEP);
			if (copyin((caddr_t)listio32.mem_count, mem_count32,
			    (sizeof (uint32_t) * listio32.mem_list_count))) {
				err = EFAULT;
				break;
			}
			callp->mem_count = (size_t *)kmem_alloc(
			    (sizeof (size_t) * callp->listio.mem_list_count),
			    KM_SLEEP);

			for (i = 0; i < callp->listio.mem_list_count; i++) {
				callp->mem_addr[i] = (caddr_t)mem_addr32[i];
				callp->mem_count[i] = mem_count32[i];
			}
		} while (0) /*CONSTCOND*/;
		kmem_free(mem_addr32,
		    (sizeof (caddr32_t) * listio32.mem_list_count));
		if (mem_count32) {
			kmem_free(mem_count32,
			    (sizeof (uint32_t) * listio32.mem_list_count));
		}
		if (err) {
			sam_free_listio(callp);
			return (err);
		}

	}

	/*
	 * Read in the file arrays.
	 */
	callp->file_off = (offset_t *)kmem_alloc(
	    (sizeof (offset_t) * callp->listio.file_list_count),
	    KM_SLEEP);
	if (copyin((caddr_t)callp->listio.file_off, callp->file_off,
	    (sizeof (offset_t)*callp->listio.file_list_count))) {
		sam_free_listio(callp);
		return (EFAULT);
	}
	callp->file_len = (offset_t *)kmem_alloc(
	    (sizeof (offset_t) * callp->listio.file_list_count),
	    KM_SLEEP);
	if (copyin((caddr_t)callp->listio.file_len, callp->file_len,
	    (sizeof (offset_t)*callp->listio.file_list_count))) {
		sam_free_listio(callp);
		return (EFAULT);
	}

	/*
	 * Put logic here to coalesce the mem list & file list into adjacent
	 * disk addresses. Logic for sieving and listio.
	 * May decide to do direct or paged based on the list.
	 */

	if (((ip->flags.bits & SAM_DIRECTIO) != 0) ||
	    (SAM_ITOV(ip)->v_type == VCHR)) {
		/*
		 * Forward chain call in the incore inode.
		 * Rework this and put 16 requests in one allocation.
		 */
		mutex_enter(&ip->listio_mutex);
		niop = ip->listiop;
		if (niop == NULL) {
			ip->listiop = callp;
		} else {
			while (niop->nextp != NULL) {
				niop = niop->nextp;
			}
			niop->nextp = callp;
		}
		callp->handle = callp->listio.wait_handle;
		callp->pid = curproc->p_pid;
		if (!callp->handle)
			callp->handle = &callp;
		mutex_exit(&ip->listio_mutex);

		error = sam_direct_listio(ip, mode, callp, credp);

		if (!callp->listio.wait_handle) {
			error = sam_wait_listio(ip, callp->handle, callp->pid);
		}
	} else {
		struct samaio_uio auio;

		/* Initialize callp->listio to act as cursor */
		callp->listio.mem_addr = (void **)callp->mem_addr;
		callp->listio.mem_count = callp->mem_count;
		callp->listio.file_off = callp->file_off;
		callp->listio.file_len = callp->file_len;

		auio.type = SAMAIO_LIO_PAGED;
		auio.bp = (struct buf *)(void *)callp;
		auio.uio.uio_iov = &(auio.iovec[0]);
		auio.uio.uio_segflg = UIO_USERSPACE;
		auio.uio.uio_llimit = SAM_MAXOFFSET_T;
		auio.uio.uio_loffset = callp->file_off[0];
		auio.iovec[0].iov_base = (char *)callp->listio.mem_addr[0];
		auio.iovec[0].iov_len = callp->listio.mem_count[0];
		auio.uio.uio_resid = callp->listio.file_len[0];

		error = sam_process_listio_call(ip, mode, &auio, credp);
		if (callp->listio.wait_handle && error != EFAULT) {
			/*
			 * Currently, paged listio is always synchronous.
			 * Put completed request in chain.
			 */
			mutex_enter(&ip->listio_mutex);
			niop = ip->listiop;
			if (niop == NULL) {
				ip->listiop = callp;
			} else {
				while (niop->nextp != NULL) {
					niop = niop->nextp;
				}
				niop->nextp = callp;
			}
			callp->handle = callp->listio.wait_handle;
			callp->pid = curproc->p_pid;
			callp->io_count = 0;
			callp->error = error;
			error = 0;
			mutex_exit(&ip->listio_mutex);
		} else {
			sam_free_listio(callp);
		}
	}
	return (error);
}


/*
 * ----- sam_process_listio_call - issue read or write call
 *
 */

static int				/* ERRNO if error, 0 if successful. */
sam_process_listio_call(
	sam_node_t *ip,			/* Pointer to inode. */
	int mode,			/* Read/Write */
	struct samaio_uio *aiop,	/* samaio struct for type 3 */
	cred_t *credp)			/* credentials pointer. */
{
	int error;

	sam_rwdlock_ino(ip, RW_READER, 0);
	if (mode == FREAD) {
		aiop->uio.uio_fmode = FREAD;
		error = VOP_READ_OS(SAM_ITOV(ip), (struct uio *)aiop,
		    FASYNC, credp, NULL);
	} else {
		aiop->uio.uio_fmode = FWRITE;
		error = VOP_WRITE_OS(SAM_ITOV(ip), (struct uio *)aiop,
		    FASYNC, credp, NULL);
	}
	RW_UNLOCK_OS(&ip->data_rwl, RW_READER);
	return (error);
}


/*
 * ----- sam_direct_listio - direct listio command.
 *
 */

static int				/* ERRNO if error, 0 if successful. */
sam_direct_listio(
	sam_node_t *ip,			/* Pointer to inode. */
	int mode,			/* Read/Write */
	struct sam_listio_call *callp,	/* Pointer to call */
	cred_t *credp)			/* credentials pointer. */
{
	buf_t *bp;
	int rw;
	int fi, mi;
	struct samaio_uio *aiop;
	int error = 0;

	/*
	 * Build each request with the memory iovec.
	 */
	rw = (mode == FWRITE) ? B_WRITE : B_READ;
	fi = 0;
	mi = 0;
	for (;;) {
		size_t count;
		size_t totcount, length;
		int j, m;
		int iovcnt;

		bp = sam_get_buf_header();
		bp->b_vp = (vnode_t *)(void *)callp;
		bp->b_flags = B_PHYS | B_BUSY | B_ASYNC | rw;
		bp->b_iodone = (int (*)()) sam_listio_done;
		bp->b_flags |= rw;
		bp->b_proc = curproc;

		/*
		 * Allocate samaio_uio struct for I/O. Freed by
		 * sam_dk_aio_direct_done.
		 */
		iovcnt = 0;
		length = callp->file_len[fi];
		m = mi;
		for (;;) {
			count = callp->mem_count[m];
			iovcnt++;
			if (count >= length) {
				break;
			}
			m++;
			length -= count;
		}
		aiop = (struct samaio_uio *)kmem_zalloc(
		    sizeof (struct samaio_uio) +
		    (sizeof (struct iovec) * (iovcnt - 1)), KM_SLEEP);
		aiop->type = SAMAIO_LIO_DIRECT;
		aiop->bp = bp;

		aiop->uio.uio_iov = &(aiop->iovec[0]);
		aiop->uio.uio_segflg = UIO_USERSPACE;
		aiop->uio.uio_llimit = SAM_MAXOFFSET_T;
		aiop->uio.uio_loffset = callp->file_off[fi];

		/*
		 * Build the uio iovec array.
		 */
		totcount = 0;
		j = 0;
		length = callp->file_len[fi];
		for (;;) {
			count = callp->mem_count[mi];
			if (count > length) {
				count = length;
			}
			if (count <= length) {
				aiop->iovec[j].iov_base =
				    (char *)callp->mem_addr[mi];
				aiop->iovec[j].iov_len = count;
				totcount += count;
				callp->mem_addr[mi] += count;
				callp->mem_count[mi] -= count;
				if (callp->mem_count[mi] == 0) {
					mi++;
				}
				callp->file_off[fi] += count;
				callp->file_len[fi] -= count;
				j++;
				if (callp->file_len[fi] == 0) {
					fi++;
					break;
				}
				if (j == iovcnt) {
					break;
				}
				continue;
			}
			break;
		}
		aiop->uio.uio_resid = totcount;
		aiop->uio.uio_iovcnt = j;

		mutex_enter(&callp->lio_mutex);
		callp->io_count++;
		mutex_exit(&callp->lio_mutex);

		error = sam_process_listio_call(ip, mode, aiop, credp);
		if (error) {
			bioerror(bp, error);
			sam_listio_done(bp);
			break;
		}
		if ((fi < callp->listio.file_list_count) &&
		    (mi < callp->listio.mem_list_count)) {
			continue;
		}
		break;
	}
	return (error);
}


/*
 * ----- sam_listio_done - Completion of I/O for direct listio
 *
 *	Called when the I/O completes for a file read/write listio.
 */

void
sam_listio_done(struct buf *bp)
{
	struct sam_listio_call *callp;
	int io_count;

	callp = (struct sam_listio_call *)bp->b_vp;
	mutex_enter(&callp->lio_mutex);
	callp->io_count--;
	io_count = callp->io_count;
	if (callp->error == 0) {
		callp->error = bp->b_error;
	}
	mutex_exit(&callp->lio_mutex);
	sam_free_buf_header((sam_uintptr_t *)bp);
	if (io_count == 0) {
		/*
		 * Wake up the user.
		 */
		sema_v(&callp->cmd_sema);
	}
}


/*
 * ----- sam_wait_listio - ioctl listio wait command.
 *
 *	Called when the user issues an wait for listio ioctl command for
 *	an opened file on the SAM-QFS file system.
 *  Called if the user requested to wait for I/O.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_wait_listio(
	sam_node_t *ip,		/* Pointer to inode. */
	void *hdl,		/* Pointer to wait handle. */
	pid_t pid)		/* Initiator pid. */
{
	struct sam_listio_call *callp, *prev_callp;
	int error = EINVAL;

	mutex_enter(&ip->listio_mutex);
	if (ip->listiop == NULL) {
		mutex_exit(&ip->listio_mutex);
		return (EINVAL);		/* No I/O Outstanding */
	}

	/*
	 * Find I/O request in chain by matching user argument address.
	 * Wait until I/O has completed.
	 */

search_again:

	callp = ip->listiop;
	prev_callp = NULL;
	while (callp) {
		if (callp->handle == hdl && callp->pid == pid) {
			mutex_enter(&callp->lio_mutex);
			/*
			 * Error if two callers try to wait on the same I/O.
			 */
			if (callp->waiting) {
				mutex_exit(&callp->lio_mutex);
				mutex_exit(&ip->listio_mutex);
				return (EINVAL);
			}
			error = callp->error;
			if (callp->io_count) {
				callp->waiting = 1;
				mutex_exit(&callp->lio_mutex);
				mutex_exit(&ip->listio_mutex);
				sema_p(&callp->cmd_sema);
				mutex_enter(&ip->listio_mutex);
				/*
				 * Chain may have changed while we were asleep.
				 */
				callp->waiting = 0;
				goto search_again;
			}

			mutex_exit(&callp->lio_mutex);

			/*
			 * Remove this entry from the listio incore inode chain.
			 */
			if (prev_callp == NULL) {
				ip->listiop = callp->nextp;
			} else {
				prev_callp->nextp = callp->nextp;
			}

			sam_free_listio(callp);
			break;
		}
		prev_callp = callp;
		callp = callp->nextp;
	}
	mutex_exit(&ip->listio_mutex);

	if (callp == NULL) {
		return (EINVAL);		/* No I/O Outstanding */
	}

	return (error);
}


/*
 * ----- sam_remove_listio - remove the listio chain
 *
 *	Called when the user issues the last close or when the inode is
 *  inactivated.
 */

int				/* ERRNO if error, 0 if successful. */
sam_remove_listio(sam_node_t *ip)
{
	struct sam_listio_call *callp, *ncallp;
	int error = 0;

search_again:

	mutex_enter(&ip->listio_mutex);
	callp = ip->listiop;
	if (callp == NULL) {
		mutex_exit(&ip->listio_mutex);
		return (0);
	}

	while (callp) {
		if (callp->io_count) {
			/*
			 * Wait until I/O has completed.
			 */
			void *handle = callp->handle;
			pid_t pid = callp->pid;
			int err;

			/*
			 * Update chain to eliminate previously freed elements.
			 */
			ip->listiop = callp;
			mutex_exit(&ip->listio_mutex);
			err = sam_wait_listio(ip, handle, pid);
			if (!error) {
				error = err;
			}
			goto search_again;
		}
		ncallp = callp->nextp;
		sam_free_listio(callp);
		callp = ncallp;
	}
	ip->listiop = NULL;
	mutex_exit(&ip->listio_mutex);
	return (error);
}


/*
 * ----- sam_free_listio - free the call arguments
 *
 */

static void
sam_free_listio(struct sam_listio_call *callp)
{
	ASSERT(!callp->waiting);
	ASSERT(!callp->io_count);
	mutex_destroy(&callp->lio_mutex);
	sema_destroy(&callp->cmd_sema);
	if (callp->mem_addr) {
		kmem_free(callp->mem_addr, sizeof (caddr_t)*callp->mem_cnt);
	}
	if (callp->mem_count) {
		kmem_free(callp->mem_count, sizeof (size_t)*callp->mem_cnt);
	}
	if (callp->file_off) {
		kmem_free(callp->file_off, sizeof (offset_t)*callp->file_cnt);
	}
	if (callp->file_len) {
		kmem_free(callp->file_len, sizeof (offset_t)*callp->file_cnt);
	}
	kmem_free(callp, sizeof (struct sam_listio_call));
}
