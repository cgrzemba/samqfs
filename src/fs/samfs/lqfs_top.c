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
#pragma ident	"$Revision: 1.7 $"
#endif

#include <sys/systm.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/sysmacros.h>
#include <sys/debug.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/taskq.h>
#include <sys/cmn_err.h>
#include <sys/stat.h>
#define	_LQFS_INFRASTRUCTURE
#include <lqfs_common.h>
#undef _LQFS_INFRASTRUCTURE
#ifdef LUFS
#include <sys/fs/qfs_inode.h>
#include <sys/fs/qfs_filio.h>
#include <sys/fs/qfs_log.h>
#include <sys/fs/qfs_bio.h>
#endif /* LUFS */

static void top_issue_sync_tq(void *arg);

/*
 * FILE SYSTEM INTERFACE TO TRANSACTION OPERATIONS (TOP; like VOP)
 */

uint_t topkey; /* tsd transaction key */

/*
 * declare a delta
 */
void
top_delta(
	qfsvfs_t *qfsvfsp,
	offset_t mof,
	uchar_t ord,
	off_t nb,
	delta_t dtyp,
	int (*func)(),
	uint64_t arg)
{
	ml_unit_t		*ul	= LQFS_GET_LOGP(qfsvfsp);
	threadtrans_t		*tp	= tsd_get(topkey);

#ifdef LUFS
	ASSERT((VFS_PTR(qfsvfsp))->vfs_dev == ul->un_dev);
#endif /* LUFS */
	ASSERT(nb);
	ASSERT(((ul->un_debug & (MT_TRANSACT|MT_MATAMAP)) == 0) ||
	    top_delta_debug(ul, mof, ord, nb, dtyp));

	deltamap_add(ul->un_deltamap, mof, ord, nb, dtyp, func, arg, tp);

	ul->un_logmap->mtm_ref = 1; /* for roll thread's heuristic */
	if (tp) {
		tp->any_deltas = 1;
	}
}

/*
 * cancel a delta
 */
void
top_cancel(qfsvfs_t *qfsvfsp, offset_t mof, uchar_t ord, off_t nb, int flags)
{
	ml_unit_t	*ul	= LQFS_GET_LOGP(qfsvfsp);
#ifdef LUFS
	int		metadata = flags & (I_DIR|I_IBLK|I_SHAD|I_QUOTA);
#else
	int		metadata = flags & (S_IFDIR);
	/* QFS doesn't support shadow inodes. */
	/* LQFS_TODO_QUOTAS */
#endif /* LUFS */

#ifdef LUFS
	ASSERT((VFS_PTR(qfsvfsp))->vfs_dev == ul->un_dev);
#endif /* LUFS */
	ASSERT(nb);
	ASSERT(((ul->un_debug & (MT_TRANSACT|MT_MATAMAP)) == 0) ||
	    (!(flags & metadata) ||
	    top_delta_debug(ul, mof, ord, nb, DT_CANCEL)));

	LQFS_MSG(CE_WARN, "top_cancel(): Cancelling all deltas within "
	    "mof 0x%x ord %d nb %d\n", mof, ord, nb);

	if (metadata) {
		deltamap_del(ul->un_deltamap, mof, ord, nb);
	}

	logmap_cancel(ul, mof, ord, nb, metadata);

	/*
	 * needed for the roll thread's heuristic
	 */
	ul->un_logmap->mtm_ref = 1;
}

/*
 * check if this delta has been canceled (metadata -> userdata)
 */
int
top_iscancel(qfsvfs_t *qfsvfsp, offset_t mof, uchar_t ord, off_t nb)
{
	ml_unit_t	*ul	= LQFS_GET_LOGP(qfsvfsp);

#ifdef LUFS
	ASSERT((VFS_PTR(qfsvfsp))->vfs_dev == ul->un_dev);
#endif /* LUFS */
	ASSERT(nb);
	if (logmap_iscancel(ul->un_logmap, mof, ord, nb)) {
		return (1);
	}
	if (ul->un_flags & LDL_ERROR) {
		return (1);
	}
	return (0);
}

/*
 * put device into error state
 */
void
top_seterror(qfsvfs_t *qfsvfsp)
{
	ml_unit_t	*ul	= LQFS_GET_LOGP(qfsvfsp);

#ifdef LUFS
	ASSERT((VFS_PTR(qfsvfsp))->vfs_dev == ul->un_dev);
#endif /* LUFS */
	ldl_seterror(ul, "qfs is forcing a qfs log error");
}

/*
 * issue a empty sync op to help empty the delta/log map or the log
 */
static void
top_issue_sync(void *arg)
{
	qfsvfs_t *qfsvfsp = (qfsvfs_t *)arg;
	ml_unit_t *ul = (ml_unit_t *)LQFS_GET_LOGP(qfsvfsp);
	mt_map_t *mtm = ul->un_logmap;
	int	error = 0;
	threadtrans_t *tp = tsd_get(topkey);

	if ((curthread->t_flag & T_DONTBLOCK) == 0) {
		curthread->t_flag |= T_DONTBLOCK;
	}
	top_begin_sync(qfsvfsp, TOP_COMMIT_ASYNC, 0, &error);
	if (!error) {
		top_end_sync(qfsvfsp, &error, TOP_COMMIT_ASYNC, 0);
	}

	/*
	 * If we are a taskq thread, decrement mtm_taskq_sync_count and
	 * wake up the thread waiting on the mtm_cv if the mtm_taskq_sync_count
	 * hits zero.
	 */

	if (tp && tp->is_trans_tq) {
		mutex_enter(&mtm->mtm_lock);
		mtm->mtm_taskq_sync_count--;
		if (mtm->mtm_taskq_sync_count == 0) {
			cv_signal(&mtm->mtm_cv);
		}
		mutex_exit(&mtm->mtm_lock);
	}
}

/*
 * top_issue_sync_tq ---
 *
 * Wrapper for top_issue_sync when started from taskq_dispatch so this
 * thread can be correctly indentified as a taskq thread.
 */
static void
top_issue_sync_tq(void *arg)
{
	threadtrans_t   *tp;
	qfsvfs_t *qfsvfsp = (qfsvfs_t *)arg;

	tp = tsd_get(topkey);

	ASSERT(tp == NULL);

	if (tp == NULL) {
		tp = kmem_zalloc(sizeof (threadtrans_t), KM_SLEEP);
		tp->is_trans_tq = 1;
		(void) tsd_set(topkey, tp);
	}

	top_issue_sync(qfsvfsp);
}

/*
 * MOBY TRANSACTION ROUTINES
 * begin a moby transaction
 *	sync ops enter until first sync op finishes
 *	async ops enter until last sync op finishes
 * end a moby transaction
 *		outstanding deltas are pushed thru log
 *		log buffer is committed (incore only)
 *		next trans is open to async ops
 *		log buffer is committed on the log
 *		next trans is open to sync ops
 */

/*ARGSUSED*/
void
top_begin_sync(qfsvfs_t *qfsvfsp, top_t topid, ulong_t size, int *error)
{
	ml_unit_t	*ul	= LQFS_GET_LOGP(qfsvfsp);
	mt_map_t	*mtm = ul->un_logmap;
	threadtrans_t	*tp;
	ushort_t	seq;

#ifdef LUFS
	ASSERT((VFS_PTR(qfsvfsp))->vfs_dev == ul->un_dev);
#endif /* LUFS */
	ASSERT(error != NULL);
	ASSERT(*error == 0);

	mutex_enter(&mtm->mtm_lock);
	if (topid == TOP_FSYNC) {
		/*
		 * Error the fsync immediately if this is an nfs thread
		 * and its last transaction has already been committed.
		 * The only transactions outstanding are those
		 * where no commit has even started
		 * (last_async_tid == mtm->mtm_tid)
		 * or those where a commit is in progress
		 * (last_async_tid == mtm->mtm_committid)
		 */
		if (curthread->t_flag & T_DONTPEND) {
			tp = tsd_get(topkey);
			if (tp && (tp->last_async_tid != mtm->mtm_tid) &&
			    (tp->last_async_tid != mtm->mtm_committid)) {
				mutex_exit(&mtm->mtm_lock);
				*error = 1;
				return;
			}
		}

		/*
		 * If there's already other synchronous transactions
		 * and we haven't allowed async ones to start yet
		 * then just wait for the commit to complete.
		 */
		if (((mtm->mtm_closed & (TOP_SYNC | TOP_ASYNC)) ==
		    (TOP_SYNC | TOP_ASYNC)) || mtm->mtm_activesync) {
			seq = mtm->mtm_seq;
			do {
				cv_wait(&mtm->mtm_cv_commit, &mtm->mtm_lock);
			} while (seq == mtm->mtm_seq);
			mutex_exit(&mtm->mtm_lock);
			*error = 1;
			return;
		}
		if (mtm->mtm_closed & TOP_SYNC) {
			/*
			 * We know we're in the window where a thread is
			 * committing a transaction in top_end_sync() and
			 * has allowed async threads to start but hasn't
			 * got the completion on the commit write to
			 * allow sync threads to start.
			 * So wait for that commit completion then retest
			 * for the quick nfs check and if that fails
			 * go on to start a transaction
			 */
			seq = mtm->mtm_seq;
			do {
				cv_wait(&mtm->mtm_cv_commit, &mtm->mtm_lock);
			} while (seq == mtm->mtm_seq);

			/* tp is set above if T_DONTPEND */
			if ((curthread->t_flag & T_DONTPEND) && tp &&
			    (tp->last_async_tid != mtm->mtm_tid) &&
			    (tp->last_async_tid != mtm->mtm_committid)) {
				mutex_exit(&mtm->mtm_lock);
				*error = 1;
				return;
			}
		}
	}
retry:
	mtm->mtm_ref = 1;
	/*
	 * current transaction closed to sync ops; try for next transaction
	 */
	if ((mtm->mtm_closed & TOP_SYNC) && !panicstr) {
		ulong_t		resv;

		/*
		 * We know a commit is in progress, if we are trying to
		 * commit and we haven't allowed async ones to start yet,
		 * then just wait for the commit completion
		 */
		if ((size == TOP_COMMIT_SIZE) &&
		    (((mtm->mtm_closed & (TOP_SYNC | TOP_ASYNC)) ==
		    (TOP_SYNC | TOP_ASYNC)) || (mtm->mtm_activesync))) {
			seq = mtm->mtm_seq;
			do {
				cv_wait(&mtm->mtm_cv_commit, &mtm->mtm_lock);
			} while (seq == mtm->mtm_seq);
			mutex_exit(&mtm->mtm_lock);
			*error = 1;
			return;
		}

		/*
		 * next transaction is full; try for next transaction
		 */
		resv = size + ul->un_resv_wantin + ul->un_resv;
		if (resv > ul->un_maxresv) {
			cv_wait(&mtm->mtm_cv_commit, &mtm->mtm_lock);
			goto retry;
		}
		/*
		 * we are in the next transaction; wait for it to start
		 */
		mtm->mtm_wantin++;
		ul->un_resv_wantin += size;
		/*
		 * The corresponding cv_broadcast wakes up
		 * all threads that have been validated to go into
		 * the next transaction. However, because spurious
		 * cv_wait wakeups are possible we use a sequence
		 * number to check that the commit and cv_broadcast
		 * has really occurred. We couldn't use mtm_tid
		 * because on error that doesn't get incremented.
		 */
		seq = mtm->mtm_seq;
		do {
			cv_wait(&mtm->mtm_cv_commit, &mtm->mtm_lock);
		} while (seq == mtm->mtm_seq);
	} else {
		/*
		 * if the current transaction is full; try the next one
		 */
		if (size && (ul->un_resv && ((size + ul->un_resv) >
		    ul->un_maxresv)) && !panicstr) {
			/*
			 * log is over reserved and no one will unresv the space
			 *	so generate empty sync op to unresv the space
			 */
			if (mtm->mtm_activesync == 0) {
				mutex_exit(&mtm->mtm_lock);
				top_issue_sync(qfsvfsp);
				mutex_enter(&mtm->mtm_lock);
				goto retry;
			}
			cv_wait(&mtm->mtm_cv_commit, &mtm->mtm_lock);
			goto retry;
		}
		/*
		 * we are in the current transaction
		 */
		mtm->mtm_active++;
		mtm->mtm_activesync++;
		ul->un_resv += size;
	}

	ASSERT(mtm->mtm_active > 0);
	ASSERT(mtm->mtm_activesync > 0);
	mutex_exit(&mtm->mtm_lock);

	ASSERT(((ul->un_debug & MT_TRANSACT) == 0) ||
	    top_begin_debug(ul, topid, size));
}

int tryfail_cnt;

int
top_begin_async(qfsvfs_t *qfsvfsp, top_t topid, ulong_t size, int tryasync)
{
	ml_unit_t	*ul	= LQFS_GET_LOGP(qfsvfsp);
	mt_map_t	*mtm	= ul->un_logmap;
	threadtrans_t   *tp;

#ifdef LUFS
	ASSERT((VFS_PTR(qfsvfsp))->vfs_dev == ul->un_dev);
#endif /* LUFS */

	tp = tsd_get(topkey);
	if (tp == NULL) {
		tp = kmem_zalloc(sizeof (threadtrans_t), KM_SLEEP);
		(void) tsd_set(topkey, tp);
	}
	tp->deltas_size = 0;
	tp->any_deltas = 0;

	mutex_enter(&mtm->mtm_lock);
retry:
	mtm->mtm_ref = 1;
	/*
	 * current transaction closed to async ops; try for next transaction
	 */
	if ((mtm->mtm_closed & TOP_ASYNC) && !panicstr) {
		if (tryasync) {
			mutex_exit(&mtm->mtm_lock);
			tryfail_cnt++;
			return (EWOULDBLOCK);
		}
		cv_wait(&mtm->mtm_cv_next, &mtm->mtm_lock);
		goto retry;
	}

	/*
	 * if the current transaction is full; try the next one
	 */
	if (((size + ul->un_resv + ul->un_resv_wantin) > ul->un_maxresv) &&
	    !panicstr) {
		/*
		 * log is overreserved and no one will unresv the space
		 *	so generate empty sync op to unresv the space
		 * We need TOP_SYNC_FORCED because we want to know when
		 * a top_end_sync is completed.
		 * mtm_taskq_sync_count is needed because we want to keep track
		 * of the pending top_issue_sync dispatches so that during
		 * forced umount we can wait for these to complete.
		 * mtm_taskq_sync_count is decremented in top_issue_sync and
		 * can remain set even after top_end_sync completes.
		 * We have a window between the clearing of TOP_SYNC_FORCED
		 * flag and the decrementing of mtm_taskq_sync_count.
		 * If in this window new async transactions start consuming
		 * log space, the log can get overreserved.
		 * Subsequently a new async transaction would fail to generate
		 * an empty sync transaction via the taskq, since it finds
		 * the mtm_taskq_sync_count set. This can cause a hang.
		 * Hence we do not test for mtm_taskq_sync_count being zero.
		 * Instead, the TOP_SYNC_FORCED flag is tested here.
		 */
		if ((mtm->mtm_activesync == 0) &&
		    (!(mtm->mtm_closed & TOP_SYNC_FORCED))) {
			/*
			 * Set flag to stop multiple forced empty
			 * sync transactions. Increment mtm_taskq_sync_count.
			 */
			mtm->mtm_closed |= TOP_SYNC_FORCED;
			mtm->mtm_taskq_sync_count++;
			mutex_exit(&mtm->mtm_lock);
			(void) taskq_dispatch(system_taskq,
			    top_issue_sync_tq, qfsvfsp, TQ_SLEEP);
			if (tryasync) {
				tryfail_cnt++;
				return (EWOULDBLOCK);
			}
			mutex_enter(&mtm->mtm_lock);
			goto retry;
		}
		if (tryasync) {
			mutex_exit(&mtm->mtm_lock);
			tryfail_cnt++;
			return (EWOULDBLOCK);
		}
		cv_wait(&mtm->mtm_cv_next, &mtm->mtm_lock);
		goto retry;
	}
	/*
	 * we are in the current transaction
	 */
	mtm->mtm_active++;
	ul->un_resv += size;

	ASSERT(mtm->mtm_active > 0);
	mutex_exit(&mtm->mtm_lock);

	ASSERT(((ul->un_debug & MT_TRANSACT) == 0) ||
	    top_begin_debug(ul, topid, size));

	return (0);
}

/*ARGSUSED*/
void
top_end_sync(qfsvfs_t *qfsvfsp, int *ep, top_t topid, ulong_t size)
{
	ml_unit_t	*ul	= LQFS_GET_LOGP(qfsvfsp);
	mt_map_t	*mtm	= ul->un_logmap;
	mapentry_t	*cancellist;
	uint32_t	tid;

#ifdef LUFS
	ASSERT((VFS_PTR(qfsvfsp))->vfs_dev == ul->un_dev);
#endif /* LUFS */
	ASSERT(((ul->un_debug & MT_TRANSACT) == 0) ||
	    top_end_debug(ul, mtm, topid, size));

	mutex_enter(&mtm->mtm_lock);
	tid = mtm->mtm_tid;

	mtm->mtm_activesync--;
	mtm->mtm_active--;

	mtm->mtm_ref = 1;

	/*
	 * wait for last syncop to complete
	 */
	if (mtm->mtm_activesync || panicstr) {
		ushort_t seq = mtm->mtm_seq;

		mtm->mtm_closed = TOP_SYNC;

		do {
			cv_wait(&mtm->mtm_cv_commit, &mtm->mtm_lock);
		} while (seq == mtm->mtm_seq);
		mutex_exit(&mtm->mtm_lock);
		goto out;
	}
	/*
	 * last syncop; close current transaction to all ops
	 */
	mtm->mtm_closed = TOP_SYNC|TOP_ASYNC;

	/*
	 * wait for last asyncop to finish
	 */
	while (mtm->mtm_active) {
		cv_wait(&mtm->mtm_cv_eot, &mtm->mtm_lock);
	}

	/*
	 * push dirty metadata thru the log
	 */
	deltamap_push(ul);

	ASSERT(((ul->un_debug & MT_FORCEROLL) == 0) ||
	    top_roll_debug(ul));

	mtm->mtm_tid = tid + 1;	/* can overflow to 0 */

	/*
	 * Empty the cancellist, but save it for logmap_free_cancel
	 */
	mutex_enter(&mtm->mtm_mutex);
	cancellist = mtm->mtm_cancel;
	mtm->mtm_cancel = NULL;
	mutex_exit(&mtm->mtm_mutex);

	/*
	 * allow async ops
	 */
	ASSERT(mtm->mtm_active == 0);
	ul->un_resv = 0; /* unreserve the log space */
	mtm->mtm_closed = TOP_SYNC;
	/*
	 * Hold the un_log_mutex here until we are done writing
	 * the commit record to prevent any more deltas to be written
	 * to the log after we allow async operations.
	 */
	mutex_enter(&ul->un_log_mutex);
	mutex_exit(&mtm->mtm_lock);
	cv_broadcast(&mtm->mtm_cv_next);

	/*
	 * asynchronously write the commit record,
	 */
	logmap_commit(ul, tid);

	/*
	 * wait for outstanding log writes (e.g., commits) to finish
	 */
	ldl_waito(ul);

	/*
	 * Now that we are sure the commit has been written to the log
	 * we can free any canceled deltas.  If we free them before
	 * guaranteeing that the commit was written, we could panic before
	 * the commit, but after an async thread has allocated and written
	 * to canceled freed block.
	 */

	logmap_free_cancel(mtm, &cancellist);
	mutex_exit(&ul->un_log_mutex);

	/*
	 * now, allow all ops
	 */
	mutex_enter(&mtm->mtm_lock);
	mtm->mtm_active += mtm->mtm_wantin;
	ul->un_resv += ul->un_resv_wantin;
	mtm->mtm_activesync = mtm->mtm_wantin;
	mtm->mtm_wantin = 0;
	mtm->mtm_closed = 0;
	ul->un_resv_wantin = 0;
	mtm->mtm_committid = mtm->mtm_tid;
	mtm->mtm_seq++;
	mutex_exit(&mtm->mtm_lock);

	/*
	 * Finish any other synchronous transactions and
	 * start any waiting new synchronous transactions
	 */
	cv_broadcast(&mtm->mtm_cv_commit);

	/*
	 * if the logmap is getting full; roll something
	 */
	if (logmap_need_roll_sync(mtm)) {
		logmap_forceroll_nowait(mtm);
	}

out:
	if (ul->un_flags & LDL_ERROR)
		*ep = EIO;
}

/*ARGSUSED*/
void
top_end_async(qfsvfs_t *qfsvfsp, top_t topid, ulong_t size)
{
	ml_unit_t	*ul	= LQFS_GET_LOGP(qfsvfsp);
	mt_map_t	*mtm	= ul->un_logmap;
	threadtrans_t	*tp	= tsd_get(topkey);
	int		wakeup_needed = 0;

	ASSERT(tp);
#ifdef LUFS
	ASSERT((VFS_PTR(qfsvfsp))->vfs_dev == ul->un_dev);
#endif /* LUFS */

	ASSERT(((ul->un_debug & MT_TRANSACT) == 0) ||
	    top_end_debug(ul, mtm, topid, size));

	mutex_enter(&mtm->mtm_lock);

	if (size > tp->deltas_size) {
		ul->un_resv -= (size - tp->deltas_size);
	}
	if (tp->any_deltas) {
		tp->last_async_tid = mtm->mtm_tid;
	}
	mtm->mtm_ref = 1;

	mtm->mtm_active--;
	if ((mtm->mtm_active == 0) &&
	    (mtm->mtm_closed == (TOP_SYNC|TOP_ASYNC))) {
		wakeup_needed = 1;
	}
	mutex_exit(&mtm->mtm_lock);
	if (wakeup_needed) {
		cv_signal(&mtm->mtm_cv_eot);
	}

	/*
	 * Generate a sync op if the log, logmap, or deltamap are heavily used.
	 * Unless we are possibly holding any VM locks, since if we are holding
	 * any VM locks and we issue a top_end_sync(), we could deadlock.
	 */
	if ((mtm->mtm_activesync == 0) &&
	    !(mtm->mtm_closed & TOP_SYNC) &&
	    (deltamap_need_commit(ul->un_deltamap) ||
	    logmap_need_commit(mtm) ||
	    ldl_need_commit(ul)) &&
	    (topid != TOP_GETPAGE)) {
		top_issue_sync(qfsvfsp);
	}
	/*
	 * roll something from the log if the logmap is too full
	 */
	if (logmap_need_roll_async(mtm)) {
		logmap_forceroll_nowait(mtm);
	}
}

/*
 * Called from roll thread;
 *	buffer set for reading master
 * Returns
 *	0 - success, can continue with next buffer
 *	1 - failure due to logmap deltas being in use
 */
int
top_read_roll(rollbuf_t *rbp, ml_unit_t *ul)
{
	buf_t		*bp	= &rbp->rb_bh;
	offset_t	mof	= ldbtob(bp->b_blkno);
	qfsvfs_t	*qfsvfsp = ul->un_qfsvfs;
	uchar_t		ord;

	/*
	 * get a list of deltas
	 */
	ord = lqfs_find_ord(qfsvfsp, bp);
	if (logmap_list_get_roll(ul->un_logmap, mof, ord, rbp)) {
		/* logmap deltas are in use */
		return (1);
	}

	/*
	 * no deltas were found, nothing to roll
	 */
	if (rbp->rb_age == NULL) {
		bp->b_flags |= B_INVAL;
		return (0);
	}

	/*
	 * If there is one cached roll buffer that cover all the deltas then
	 * we can use that instead of copying to a separate roll buffer.
	 */
	if (rbp->rb_crb) {
		rbp->rb_bh.b_blkno = lbtodb(rbp->rb_crb->c_mof);
		rbp->rb_bh.b_edev = qfsvfsp->mi.m_fs[rbp->rb_crb->c_ord].dev;
		rbp->rb_bh.b_dev = cmpdev(rbp->rb_bh.b_edev);
		return (0);
	}

	/*
	 * Set up the read.
	 * If no read is needed logmap_setup_read() returns 0.
	 */
	if (logmap_setup_read(rbp->rb_age, rbp)) {
		/*
		 * async read the data from master
		 */
		logstats.ls_rreads.value.ui64++;
		bp->b_bcount = MAPBLOCKSIZE;
		if ((bp->b_flags & B_READ) == 0) {
			LQFS_MSG(CE_WARN, "top_read_roll(): bdev_strategy "
			    "writing mof 0x%x edev %ld nb %d\n",
			    ldbtob(bp->b_blkno), bp->b_edev,
			    bp->b_bcount);
		} else {
			LQFS_MSG(CE_WARN, "top_read_roll(): bdev_strategy "
			    "reading mof 0x%x edev %ld nb %d\n",
			    ldbtob(bp->b_blkno), bp->b_edev,
			    bp->b_bcount);
		}
		(void) bdev_strategy(bp);
#ifdef LQFS_TODO_STATS
		lwp_stat_update(LWP_STAT_INBLK, 1);
#endif /* LQFS_TODO_STATS */
	} else {
		sema_v(&bp->b_io); /* mark read as complete */
	}
	return (0);
}

int qfs_crb_enable = 1;

/*
 * move deltas from deltamap into the log
 */
void
top_log(qfsvfs_t *qfsvfsp, char *va, offset_t vamof, uchar_t vaord, off_t nb,
    caddr_t buf, uint32_t bufsz)
{
	ml_unit_t	*ul = LQFS_GET_LOGP(qfsvfsp);
	mapentry_t	*me;
	offset_t	hmof;
	uchar_t		hord;
	uint32_t	hnb, nb1;

	/*
	 * needed for the roll thread's heuristic
	 */
	ul->un_logmap->mtm_ref = 1;

	if (buf && qfs_crb_enable) {
		ASSERT((bufsz & DEV_BMASK) == 0);
		/*
		 * Move any deltas to the logmap. Split requests that
		 * straddle MAPBLOCKSIZE hash boundaries (i.e. summary info).
		 */
		for (hmof = vamof - (va - buf), nb1 = nb; bufsz;
		    bufsz -= hnb, hmof += hnb, buf += hnb, nb1 -= hnb) {
			hnb = MAPBLOCKSIZE - (hmof & MAPBLOCKOFF);
			if (hnb > bufsz) {
				hnb = bufsz;
			}
			me = deltamap_remove(ul->un_deltamap,
			    MAX(hmof, vamof), vaord, MIN(hnb, nb1));
			hord = vaord;
			if (me) {
				logmap_add_buf(ul, va, hmof, hord,
				    me, buf, hnb);
			}
		}
	} else {
		/*
		 * if there are deltas
		 */
		me = deltamap_remove(ul->un_deltamap, vamof, vaord, nb);
		if (me) {
			/*
			 * move to logmap
			 */
			logmap_add(ul, va, vamof, vaord, me);
		}
	}

	ASSERT((ul->un_matamap == NULL) ||
	    matamap_within(ul->un_matamap, vamof, vaord, nb));
}


static void
top_threadtrans_destroy(void *tp)
{
	kmem_free(tp, sizeof (threadtrans_t));
}

void
_init_top(void)
{
	ASSERT(top_init_debug());

	/*
	 * set up the delta layer
	 */
	_init_map();

	/*
	 * Initialise the thread specific data transaction key
	 */
	tsd_create(&topkey, top_threadtrans_destroy);
}
