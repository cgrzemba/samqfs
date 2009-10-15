/*
 * ----- sam/reclaim.c - Reclaim SAMFS block free routines.
 *
 * Reclaim freed blocks for this mount point.
 * Server expire leases for client inodes in a shared file system.
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

#pragma ident "$Revision: 1.134 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/t_lock.h>
#include <sys/flock.h>
#include <sys/sysmacros.h>
#include <sys/errno.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/cred.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/ddi.h>
#include <sys/callb.h>
#include <sys/mount.h>
#include <sys/int_limits.h>
#include <sys/stat.h>

/* ----- SAMFS Includes */

#include "sam/param.h"
#include "sam/types.h"

#include "macros.h"
#include "sblk.h"
#include "inode.h"
#include "mount.h"
#include "debug.h"
#include "extern.h"
#include "indirect.h"
#include "trace.h"
#include "kstats.h"
#include "qfs_log.h"


typedef struct sam_callout_entry {
	int32_t ord;
	ushort_t expired_leases;
	boolean_t waiting;
} sam_callout_entry_t;

/*
 *  Number of vnodes in a release block
 */
int	max_vnodes_in_block = 500;

#define	EXTRA_CALLOUTS				8

static void sam_reclaim_thread_exit(sam_mount_t *mp, callb_cpr_t *cprinfo);
static void sam_free_extents(sam_mount_t *mp);
static void sam_prepare_callout_table(sam_callout_entry_t **callouts,
	int *max_callouts, int *num_callouts, struct sam_lease_ino *llp);
static void sam_free_callout_table(sam_callout_entry_t *callouts,
	int max_callouts);
int sam_free_direct_map(sam_mount_t *mp, struct sam_rel_blks *block);
static int sam_free_indirect_map(sam_mount_t *mp, struct sam_rel_blks *block);
static int sam_free_indirect_block(struct sam_mount *mp, struct sam_sblk *sblk,
	struct sam_rel_blks *block, int kptr[], uint32_t *extent_bn,
	uchar_t *extent_ord, int level);
void sam_release_vnodes(sam_schedule_entry_t *entry);
static void sam_release_blocks(sam_mount_t *mp, sam_fb_pool_t *fbp, int bt,
	int active);
static void sam_drain_blocks(sam_mount_t *mp);
static void sam_compact_lease_clients(struct sam_lease_ino *);

#define	SAM_RECLAIM_DEFAULT_SECS 120


/*
 * ----- sam_init_inode - initialize the inode thread.
 */

int					/* ERRNO if error, 0 if successful. */
sam_init_inode(sam_mount_t *mp)
{
	int error = 0;

	mutex_enter(&mp->mi.m_inode.mutex);
	if ((mp->mt.fi_mflag & MS_RDONLY) || SAM_IS_SHARED_CLIENT(mp)) {
		mutex_exit(&mp->mi.m_inode.mutex);
		return (error);
	}
	if (mp->mi.m_inode.state != SAM_THR_EXIST) {
		error = sam_init_thread(&mp->mi.m_inode,
		    sam_reclaim_thread, mp);
	} else {
		mutex_enter(&mp->mi.m_inode.put_mutex);
		cv_signal(&mp->mi.m_inode.put_cv);
		mutex_exit(&mp->mi.m_inode.put_mutex);
	}
	mutex_exit(&mp->mi.m_inode.mutex);
	return (error);
}


/*
 * ----- sam_reclaim_thread - Async thread for this mount point.
 *
 * Processes the free blocks chain.
 */

void
sam_reclaim_thread(sam_mount_t *mp)
{
	callb_cpr_t cprinfo;
	clock_t wait_time;

	mutex_enter(&mp->mi.m_inode.mutex);
	mp->mi.m_inode.state = SAM_THR_EXIST;

	/*
	 * Setup the CPR callback for suspend/resume.
	 */
	CALLB_CPR_INIT(&cprinfo, &mp->mi.m_inode.put_mutex, callb_generic_cpr,
	    "sam_reclaim_thread");

	for (;;) {

		SAM_COUNT64(thread, reclaim_count);

		/*
		 * Release the blocks on the list.
		 */
		if (mp->mi.m_next != NULL) {
			if (!(mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING)) ||
			    (mp->mi.m_inode.flag == SAM_THR_UMOUNTING)) {
				sam_free_extents(mp);
				sam_drain_blocks(mp);
				if (SAM_IS_SHARED_WRITER(mp)) {
					(void) sam_update_sblk(mp, 0, 0, TRUE);
				}
			}
		}

		/*
		 * Notify threads waiting for inode updates.
		 */
		if (mp->mi.m_inode.wait) {
			cv_broadcast(&mp->mi.m_inode.get_cv);
		}

		/*
		 * Wait until all the blocks are released on the list.
		 * Then exit if umounting.
		 */
		if ((mp->mi.m_inode.flag == SAM_THR_UMOUNT) &&
		    (mp->mi.m_next == NULL)) {
			mutex_exit(&mp->mi.m_inode.mutex);
			sam_reclaim_thread_exit(mp, &cprinfo);
		}

		/*
		 * Wait until signaled to get more inodes or free blocks.
		 * Prior to waiting, notify CPR that this thread is suspended.
		 * After waking up, notify CPR that this thread has resumed.
		 */
		mutex_enter(&mp->mi.m_inode.put_mutex);
		mp->mi.m_inode.put_wait = 1;
		mutex_exit(&mp->mi.m_inode.mutex);

		CALLB_CPR_SAFE_BEGIN(&cprinfo);

		/*
		 * Wake up every 2 minutes even if not requested.
		 * XXX - Is this still useful now that we only handle
		 * blocks here?
		 */
		wait_time = lbolt + (SAM_RECLAIM_DEFAULT_SECS * hz);

		(void) cv_timedwait(&mp->mi.m_inode.put_cv,
		    &mp->mi.m_inode.put_mutex,
		    wait_time);

		CALLB_CPR_SAFE_END(&cprinfo, &mp->mi.m_inode.put_mutex);

		mp->mi.m_inode.put_wait = 0;
		mutex_exit(&mp->mi.m_inode.put_mutex);
		/* reacquire the inode lock */
		mutex_enter(&mp->mi.m_inode.mutex);
	}
}


/*
 * ----- sam_reclaim_thread_exit - async thread that terminates.
 *
 * Drain the free inodes out of the inode pool.
 * Kill this thread.
 */

static void
sam_reclaim_thread_exit(sam_mount_t *mp, callb_cpr_t *cprinfo)
{
	/*
	 * Signal anyone waiting. Threads should check state and
	 * exit accordingly.
	 */
	mutex_enter(&mp->mi.m_inode.mutex);
	mp->mi.m_inode.state = SAM_THR_DEAD;	/* This inode thread is dead */
	cv_broadcast(&mp->mi.m_inode.get_cv);
	mutex_exit(&mp->mi.m_inode.mutex);

	/*
	 * Cleanup CPR on exit. This also releases inode.put_mutex.
	 */
	mutex_enter(&mp->mi.m_inode.put_mutex);
	CALLB_CPR_EXIT(cprinfo);
	thread_exit();
}


/*
 * ----- sam_prepare_callout_table
 *
 * Process the expired leases that this server is tracking.
 * If a lease has expired, prepare a callout table to be used
 * to notify appropriate clients.  If there are clients waiting
 * for the lease, set the waiting flag so they can be notified.
 */
static void
sam_prepare_callout_table(sam_callout_entry_t **callouts, int *max_callouts,
    int *num_callouts, struct sam_lease_ino *llp)
{
	int i;

	if ((*callouts == NULL) || (llp->no_clients > *max_callouts)) {
		if (*callouts != NULL) {
			kmem_free(*callouts,
			    *max_callouts * sizeof (sam_callout_entry_t));
		}
		*max_callouts = llp->no_clients + EXTRA_CALLOUTS;
		*callouts = kmem_alloc(
		    *max_callouts * sizeof (sam_callout_entry_t), KM_SLEEP);
	}

	*num_callouts = llp->no_clients;
	for (i = 0; i < llp->no_clients; i++) {
		(*callouts)[i].ord = llp->lease[i].client_ord;
		(*callouts)[i].expired_leases = 0;
		if (llp->lease[i].actions & SR_WAIT_LEASE) {
			(*callouts)[i].waiting = TRUE;
			llp->lease[i].actions &= ~SR_WAIT_LEASE;
		} else {
			(*callouts)[i].waiting = FALSE;
		}
	}
}


/*
 * ----- sam_free_callout_table - If we're done using this callout table
 * free it.
 */
static void
sam_free_callout_table(sam_callout_entry_t *callouts, int max_callouts)
{
	if (callouts != NULL) {
		kmem_free(callouts,
		    max_callouts * sizeof (sam_callout_entry_t));
	}
}


/*
 * Release the vnodes which were deferred by the lease expiration
 * thread.
 */
void
sam_release_vnodes(sam_schedule_entry_t *entry)
{
	struct sam_mount *mp = entry->mp;
	vnode_t **vp;
	int i = 0;
	int do_close_operation;
	sam_node_t *ip;

	vp = entry->arg;

	/*
	 * entry->mp will be NULL when sam_release_vnodes is
	 * called directly which means that sam_open_operation_nb
	 * has already been called.
	 */
	if (mp) {
		sam_open_operation_nb(mp);
		do_close_operation = 1;
	} else {
		ip = SAM_VTOI(vp[0]);
		mp = ip->mp;
		do_close_operation = 0;
	}

	for (i = 0; vp[i] != (vnode_t *)NULL; i++) {

		VN_RELE(vp[i]);

	}

	if (do_close_operation) {
		mutex_enter(&mp->ms.m_fo_vnrele_mutex);
		mp->ms.m_fo_vnrele_count--;
		if (mp->ms.m_fo_vnrele_count == 0) {
			/*
			 * Notify any waiters that this thread
			 * is done releasing vnodes.
			 */
			cv_broadcast(&mp->ms.m_fo_vnrele_cv);
		}
		mutex_exit(&mp->ms.m_fo_vnrele_mutex);
	}

	kmem_free((char *)vp, (sizeof (vnode_t *) *
	    (max_vnodes_in_block + 1)));

	kmem_free(entry, sizeof (sam_schedule_entry_t));

	if (do_close_operation) {
		SAM_CLOSE_OPERATION(mp, 0);
		sam_taskq_uncount(mp);
	}
}


static void
sam_process_lease_times(ushort_t *initial_leases, ushort_t *remaining_leases,
    int *expired, int *time_valid, clock_t *min_ltime, clock_t now,
    boolean_t staging, struct sam_lease_ino *llp, int i)
{
	int ltype;
	int lease_mask;

	for (ltype = 0; ltype < SAM_MAX_LTYPE; ltype++) {
		lease_mask = (1 << ltype);
		if (*initial_leases & lease_mask) {
			if ((lease_mask & SAM_NON_EXPIRING_LEASES) ||
			    staging) {
				llp->lease[i].time[ltype] = now + (30 * hz);
			}
			if (llp->lease[i].time[ltype] <= now) {
				if (llp->no_clients == 1) {
					llp->lease[i].time[ltype] = now +
					    (sam_calculate_lease_timeout(
					    30) * hz);
				} else {
					*remaining_leases &= ~lease_mask;
					*expired = 1;
				}
			} else {
				*min_ltime = MIN(*min_ltime,
				    llp->lease[i].time[ltype]);
				*time_valid = 1;
			}
		}
	}

	/* Non-expiring release (except truncate) for dead cluster clients */
	lease_mask = SAM_NON_EXPIRING_LEASES & ~CL_TRUNCATE;
	if (*remaining_leases & lease_mask) {
		client_entry_t *clp;
		sam_node_t *ip;
		ip = llp->ip;
		clp = sam_get_client_entry(ip->mp, llp->lease[i].client_ord, 0);
		if (clp != NULL && (clp->cl_flags & SAM_CLIENT_SC_DOWN)) {
			/* Clean up frlocks */
			if (*remaining_leases & CL_FRLOCK) {
				cleanlocks(SAM_ITOV(ip), IGN_PID,
				    llp->lease[i].client_ord);
			}
			*remaining_leases &= ~lease_mask;
			/* We don't want callbacks so don't set expired here */
		}
	}
}


/*
 * ----- sam_expire_server_leases
 * Process the expired leases that this server is tracking.
 */

void
sam_expire_server_leases(sam_schedule_entry_t *entry)
{
	sam_mount_t *mp;
	struct sam_lease_ino *llp;
	clock_t new_wait_time, now;
	int forced_expiration = 0;
	sam_callout_entry_t *callouts;
	int num_callouts, max_callouts;
	vnode_t **vnode_list = NULL, *vp = NULL;
	int vnode_count = 0, taskq_iterations = 0;
	sam_schedule_entry_t *vn_rel;
	boolean_t rerun = FALSE;
	clock_t ticks;

	mp = entry->mp;

	TRACE(T_SAM_SR_EXPIRE, mp, mp->mt.fi_status, 0, 0);

	/*
	 * Remove our entry from the scheduling queue, so that any incoming
	 * requests will cause a new task to be scheduled.
	 */

	mutex_enter(&samgt.schedule_mutex);
	sam_taskq_remove_nocount(entry);

	/*
	 * If an instance of this procedure is already running, just exit, after
	 * setting a flag so that the existing instance will restart itself.
	 */

	mutex_enter(&samgt.schedule_mutex);
	if (mp->mi.m_schedule_flags &
	    SAM_SCHEDULE_TASK_SERVER_RECLAIM_RUNNING) {
		mutex_exit(&samgt.schedule_mutex);
		sam_taskq_uncount(mp);
		SAM_COUNT64(shared_server, expire_task_dup);
		TRACE(T_SAM_SR_EXPIRE_RET, mp, 1, mp->mt.fi_status, 0);
		return;
	}

	sam_open_operation_nb(mp);

	mp->mi.m_schedule_flags |= SAM_SCHEDULE_TASK_SERVER_RECLAIM_RUNNING;
	mutex_exit(&samgt.schedule_mutex);

	SAM_COUNT64(shared_server, expire_task);

	callouts = NULL;
	max_callouts = 0;

	/*
	 * If umounting, umounted, or failing over, expire all the leases now.
	 */
	new_wait_time = LONG_MAX;
	if ((mp->mi.m_inode.flag & (SAM_THR_UMOUNT|SAM_THR_UMOUNTING)) ||
	    (mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING))) {
		now = 0;
		forced_expiration = 1;
	} else {
		now = lbolt;
		forced_expiration = 0;
	}

	/*
	 * Check inodes in the lease linked list. Expire inodes and
	 * inactivate them. If staging, don't expire the lease because
	 * it may take a long time to mount & position the media.
	 *
	 * This is tricky because we need to drop the mutex which covers
	 * the list if we find an inode which requires a callout.  In
	 * addition, we need to ensure that the leases for an inode do not
	 * change while we're processing it.
	 */

	mutex_enter(&mp->ms.m_fo_vnrele_mutex);
	mp->ms.m_fo_vnrele_count++;
	mutex_exit(&mp->ms.m_fo_vnrele_mutex);

	mutex_enter(&mp->mi.m_lease_mutex);
	llp = (sam_lease_ino_t *)mp->mi.m_sr_lease_chain.forw;
	while (llp != (sam_lease_ino_t *)(void *)&mp->mi.m_sr_lease_chain) {
		sam_node_t *ip;
		boolean_t staging;
		boolean_t lock_dropped;
		int i;
		boolean_t have_callouts;

		ip = llp->ip;
		staging = ip->flags.b.staging;

		lock_dropped = FALSE;

		/* Don't process inodes more than once. */

		if (ip->lease_flags & SAM_LEASE_FLAG_SERVER) {
			llp = llp->lease_chain.forw;
			continue;
		}

		ip->lease_flags |= SAM_LEASE_FLAG_SERVER;

		have_callouts = FALSE;

		/*
		 * For each client, look at each possible lease type, and
		 * expire it based on its timeout.  Leases which don't time
		 * out are always extended 30 seconds into the future.
		 *
		 * If we are forcing leases to expire, we don't need to look
		 * at the individual leases, but we need to remember if any
		 * clients had the file open.  In this case, we set the
		 * NORECLAIM bit on the inode so that the file won't be
		 * deleted.  The new server will take over and reclaim its
		 * space when the client is done working with it.
		 *
		 * NFS does not generate OPEN leases since there is no explicit
		 * open.  To handle files which are in use by NFS but have a
		 * zero link count (which can happen transiently
		 * during removes),
		 * we preserve these files to pass to the new server.  They
		 * should be deleted by the new server once the lease expires.
		 * There is a slight chance that a client may not re-establish
		 * its leases, in which case an orphan inode will result.
		 */

		mutex_enter(&ip->ilease_mutex);

		if (forced_expiration) {
			for (i = 0; i < llp->no_clients; i++) {
				if (llp->lease[i].leases) {
					mutex_exit(&ip->ilease_mutex);
					mutex_exit(&mp->mi.m_lease_mutex);
					lock_dropped = TRUE;
					RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
					ip->flags.bits |= SAM_NORECLAIM;
					RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
					mutex_enter(&mp->mi.m_lease_mutex);
					mutex_enter(&ip->ilease_mutex);
					/* dropped mutex, must refresh */
					llp = ip->sr_leases;
					break;
				}
			}
		} else {
			int expired;

			for (i = 0; i < llp->no_clients; i++) {
				ushort_t initial_leases, remaining_leases;
				clock_t min_ltime = LONG_MAX;
				int time_valid = 0;

				expired = 0;
				initial_leases = llp->lease[i].leases;
				remaining_leases = initial_leases;

				sam_process_lease_times(&initial_leases,
				    &remaining_leases,
				    &expired, &time_valid,
				    &min_ltime, now,
				    staging, llp, i);

				if (expired) {
					TRACE(T_SAM_LEASE_EXPIRE,
					    SAM_ITOV(ip), ip->di.id.ino,
					    llp->lease[i].client_ord,
					    initial_leases & ~remaining_leases);

					if (!have_callouts) {
						sam_prepare_callout_table(
						    &callouts, &max_callouts,
						    &num_callouts, llp);
						have_callouts = TRUE;
					}

					llp->lease[i].leases = remaining_leases;

					callouts[i].expired_leases =
					    initial_leases & ~remaining_leases;
				}

				if (time_valid && (min_ltime < new_wait_time)) {
					new_wait_time = min_ltime;
				}
			}
		}

		/*
		 * Remove the expired client entries from this inode.  (If we're
		 * forcing expiration, we already know that there won't be any
		 * left, so we don't need to compact the clients.)
		 */
		if (forced_expiration) {
			llp->no_clients = 0;
		} else {
			sam_compact_lease_clients(llp);
		}

		mutex_exit(&ip->ilease_mutex);

		/*
		 * Do not callout to expire leases during failover.
		 */
		if (have_callouts &&
		    !(mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING))) {

			int i;

			mutex_exit(&mp->mi.m_lease_mutex);
			lock_dropped = TRUE;

			RW_LOCK_OS(&ip->inode_rwl, RW_READER);

			for (i = 0; i < num_callouts; i++) {
				if (callouts[i].expired_leases != 0) {
					sam_notify_arg_t notify;

					notify.p.lease.lease_mask =
					    callouts[i].expired_leases;

					SAM_COUNT64(shared_server,
					    notify_expire);
					(void) sam_proc_notify(ip,
					    NOTIFY_lease_expire,
					    callouts[i].ord, &notify, 0);
				}
				if (callouts[i].waiting) {
					SAM_COUNT64(shared_server,
					    notify_lease);
					(void) sam_proc_notify(ip, NOTIFY_lease,
					    callouts[i].ord, NULL, 0);
				}
			}

			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);

			mutex_enter(&mp->mi.m_lease_mutex);
			llp = ip->sr_leases; /* dropped mutex, must refresh */
		}

		/*
		 * If no leases remain, release hold on this inode and free
		 * lease info.
		 */

		mutex_enter(&ip->ilease_mutex);

		if (llp->no_clients == 0) {
			vp = SAM_ITOV(ip);

			llp->lease_chain.back->lease_chain.forw =
			    llp->lease_chain.forw;
			llp->lease_chain.forw->lease_chain.back =
			    llp->lease_chain.back;
			ip->sr_leases = NULL;
			ip->size_owner = 0;
			ip->write_owner = 0;
			ip->cl_hold_blocks = 0;
			ip->lease_flags &= ~SAM_LEASE_FLAG_SERVER;
			mutex_exit(&ip->ilease_mutex);
			mutex_exit(&mp->mi.m_lease_mutex);
			lock_dropped = TRUE;

			kmem_free((char *)llp, (sizeof (sam_lease_ino_t) +
			    sizeof (sam_client_lease_t) *
			    (llp->max_clients - 1)));

			if (vnode_count == 0) {
				vnode_list = kmem_alloc((max_vnodes_in_block +
				    1) *
				    sizeof (vnode_t *), KM_SLEEP);
			}

			vnode_list[vnode_count++] = vp;
			if (vnode_count == max_vnodes_in_block) {
				vnode_list[vnode_count] = NULL;
				vnode_count = 0;
				taskq_iterations = 0;

				vn_rel = (sam_schedule_entry_t *)kmem_alloc(
				    sizeof (sam_schedule_entry_t), KM_SLEEP);
				vn_rel->mp = mp;
				vn_rel->func = sam_release_vnodes;
				vn_rel->arg = (void *)vnode_list;

				mutex_enter(&mp->ms.m_fo_vnrele_mutex);
				mp->ms.m_fo_vnrele_count++;
				mutex_exit(&mp->ms.m_fo_vnrele_mutex);

				while (sam_taskq_dispatch(
				    (sam_schedule_entry_t *)vn_rel)
				    == 0) {
					/*
					 * Try this loop for awhile.  If we're
					 * stuck, do the work in-line.
					 */

					if (taskq_iterations++ >=
					    MAX_TASKQ_DISPATCH_ATTEMPTS) {
						vn_rel->mp = NULL;
						sam_release_vnodes(vn_rel);

						/*
						 * Don't need to check for
						 * zero since this thread
						 * holds a count.
						 */
						mutex_enter(
						    &mp->ms.m_fo_vnrele_mutex);
						mp->ms.m_fo_vnrele_count--;
						mutex_exit(
						    &mp->ms.m_fo_vnrele_mutex);

						break;
					}
				}
				vnode_list = NULL;
			}
			mutex_enter(&mp->mi.m_lease_mutex);
		} else {
			ip->lease_flags &= ~SAM_LEASE_FLAG_SERVER;
			mutex_exit(&ip->ilease_mutex);
		}

		if (lock_dropped) {
			llp = (sam_lease_ino_t *)mp->mi.m_sr_lease_chain.forw;
		} else {
			llp = llp->lease_chain.forw;
		}
	}
	mutex_exit(&mp->mi.m_lease_mutex);

	/*
	 * Check for a partially filled vnode list. If one exists,
	 * dispatch a task to process it.
	 */
	if (vnode_list != NULL) {
		vnode_list[vnode_count] = NULL;
		taskq_iterations = 0;

		vn_rel = (sam_schedule_entry_t *)kmem_alloc(
		    sizeof (sam_schedule_entry_t), KM_SLEEP);
		vn_rel->mp = mp;
		vn_rel->func = sam_release_vnodes;
		vn_rel->arg = (void *)vnode_list;

		mutex_enter(&mp->ms.m_fo_vnrele_mutex);
		mp->ms.m_fo_vnrele_count++;
		mutex_exit(&mp->ms.m_fo_vnrele_mutex);

		while (sam_taskq_dispatch((sam_schedule_entry_t *)vn_rel) ==
		    0) {
			/*
			 * Try this loop for awhile.  If we're stuck,
			 * do the work in-line.
			 */

			if (taskq_iterations++ >= MAX_TASKQ_DISPATCH_ATTEMPTS) {
				vn_rel->mp = NULL;
				sam_release_vnodes(vn_rel);

				mutex_enter(&mp->ms.m_fo_vnrele_mutex);
				mp->ms.m_fo_vnrele_count--;
				mutex_exit(&mp->ms.m_fo_vnrele_mutex);

				break;
			}
		}
	}

	mutex_enter(&mp->ms.m_fo_vnrele_mutex);
	mp->ms.m_fo_vnrele_count--;
	if (mp->ms.m_fo_vnrele_count == 0) {
		/*
		 * Notify any waiters that we are done
		 * releasing vnodes.
		 */
		cv_broadcast(&mp->ms.m_fo_vnrele_cv);
	}
	mutex_exit(&mp->ms.m_fo_vnrele_mutex);

	if (callouts != NULL) {
		sam_free_callout_table(callouts, max_callouts);
	}

	/*
	 * Schedule another run of sam_expire_server_leases unless
	 * there is an additional copy scheduled, or unless there are
	 * no more leases.
	 */

	mutex_enter(&samgt.schedule_mutex);
	mp->mi.m_schedule_flags &= ~SAM_SCHEDULE_TASK_SERVER_RECLAIM_RUNNING;
	mp->mi.m_sr_leasesch = 0;
	if (new_wait_time != LONG_MAX) {
		rerun = TRUE;
		mp->mi.m_sr_leasesch = 1;
		ticks = new_wait_time - lbolt;
		if (ticks < SAM_EXPIRE_MIN_TICKS) {
			ticks = SAM_EXPIRE_MIN_TICKS;
		}
		if (ticks > SAM_EXPIRE_MAX_TICKS) {
			ticks = SAM_EXPIRE_MAX_TICKS;
		}
		mp->mi.m_sr_leasenext = ticks + lbolt;
	}
	mutex_exit(&samgt.schedule_mutex);

	if (rerun) {
		if (sam_taskq_add_ret(sam_expire_server_leases, mp,
		    NULL, ticks)) {
			mutex_enter(&samgt.schedule_mutex);
			mp->mi.m_sr_leasesch = 0;
			mutex_exit(&samgt.schedule_mutex);
		}
	}

	SAM_CLOSE_OPERATION(mp, 0);

	sam_taskq_uncount(mp);

	TRACE(T_SAM_SR_EXPIRE_RET, mp, 0, mp->mt.fi_status, 0);
}

/*
 * ----- sam_clear_server_leases - clear leases for a particular client
 *
 * Removes all leases for the provided client.  It is assumed that the
 * client is in a known clean state (e.g. mounting).
 *
 * No callouts are performed for either the provided client, or clients who
 * may be waiting for the lease.  Clients waiting for a lease will retry.
 * Any frlock lease will have the corresponding locks cleaned.
 * Additional clean-up (e.g. vnode release) will occur in
 * sam_expire_server_leases.
 */
void
sam_clear_server_leases(sam_mount_t *mp, int cl_ord)
{
	sam_node_t *ip;
	struct sam_lease_ino *llp;
	int i;
	int num_cleared = 0;

	TRACE(T_SAM_LEASE_CLEAR, NULL, cl_ord, 0, 0);

	mutex_enter(&mp->mi.m_lease_mutex);
	llp = (sam_lease_ino_t *)mp->mi.m_sr_lease_chain.forw;
	while (llp != NULL &&
	    llp != (sam_lease_ino_t *)(void *)&mp->mi.m_sr_lease_chain) {
		ip = llp->ip;

		mutex_enter(&ip->ilease_mutex);
		for (i = 0; i < llp->no_clients; i++) {
			if (llp->lease[i].client_ord != cl_ord) {
				continue;
			}

			/* Clean-up locks for the client */
			if (flk_sysid_has_locks(llp->lease[i].client_ord,
			    FLK_QUERY_ACTIVE|FLK_QUERY_SLEEPING)) {
				cleanlocks(SAM_ITOV(ip), IGN_PID,
				    llp->lease[i].client_ord);
			}

			if (llp->lease[i].leases != 0 ||
			    llp->lease[i].wt_leases != 0) {
				llp->lease[i].leases = 0;
				llp->lease[i].wt_leases = 0;
				sam_compact_lease_clients(llp);
				num_cleared++;
			}
			break;
		}
		mutex_exit(&ip->ilease_mutex);

		llp = llp->lease_chain.forw;
	}
	mutex_exit(&mp->mi.m_lease_mutex);

	TRACE(T_SAM_LEASE_CLEAR_RET, NULL, cl_ord, num_cleared, 0);
}


/*
 * Schedule a run of sam_expire_server_leases if there is not one running
 * already, or if the next scheduled run is later than this request by
 * at least SAM_EXPIRE_MIN_TICKS.  If the next scheduled run is sooner than
 * this request, sam_expire_client_leases will schedule it at its next run.
 * The SAM_EXPIRE_MIN_TICKS test ensures at least that many ticks between
 * runs.
 *
 * Schedule this run not longer than SAM_EXPIRE_MAX_TICKS from now which
 * gives some periodic behaviour in case a case was missed.
 */

void
sam_sched_expire_server_leases(
	sam_mount_t *mp,		/* Pointer to the mount point */
	clock_t ticks,			/* Schedule tick count from now */
	boolean_t force)		/* Force run at specified time */
{
	boolean_t run = FALSE;
	clock_t next;

	mutex_enter(&samgt.schedule_mutex);
	if ((ticks > SAM_EXPIRE_MAX_TICKS) && !force) {
		ticks = SAM_EXPIRE_MAX_TICKS;
	}
	next = ticks + lbolt;
	if (force || (mp->mi.m_sr_leasesch == 0) ||
	    (next < (mp->mi.m_sr_leasenext - SAM_EXPIRE_MIN_TICKS))) {
		mp->mi.m_sr_leasenext = next;
		mp->mi.m_sr_leasesch = 1;
		run = TRUE;
	}
	mutex_exit(&samgt.schedule_mutex);
	if (run) {
		if (sam_taskq_add_ret(sam_expire_server_leases, mp,
		    NULL, ticks)) {
			mutex_enter(&samgt.schedule_mutex);
			mp->mi.m_sr_leasesch = 0;
			mutex_exit(&samgt.schedule_mutex);
		}
	}
}


/*
 * ----- sam_resync_server
 *
 * If server is resyncing, stay resyncing for maximum lease
 * time to make sure all clients lease times are satisfied.
 * The server can finish the resync phase if all clients have
 * sent the MOUNT_resync message.
 */

void
sam_resync_server(sam_schedule_entry_t *entry)
{
	client_entry_t *clp;
	int diff_time;
	int max_lease;
	int synced;
	int sync_awaited;
	int resyncing;
	int ord;
	sam_mount_t *mp;
	boolean_t failover_done, srvr_connected;

	mp = entry->mp;

	TRACE(T_SAM_SR_RESYNC, mp, mp->mt.fi_status, 0, 0);

	/*
	 * Sanity checks.  The mount point should be resyncing, should not
	 * be thawing, and we shouldn't already be running this task.  Oh,
	 * and we had better be running on the server.
	 *
	 * If FS_FREEZING, FS_FROZEN, or FS_THAWING is set, then the server
	 * has not reestablished its outstanding leases.
	 * Wait until the leases have been reset.
	 */

	mutex_enter(&samgt.schedule_mutex);
	if (mp->mi.m_schedule_flags & SAM_SCHEDULE_TASK_RESYNCING) {
		sam_taskq_remove(entry);
		TRACE(T_SAM_SR_RESYNC_RET, mp, 1, mp->mt.fi_status, 0);
		return;
	}
	if ((mp->mt.fi_status & (FS_FREEZING | FS_FROZEN | FS_THAWING)) ||
	    mp->ms.m_max_clients == 0) {
		sam_taskq_remove(entry);	/* releases schedule mutex */
		sam_taskq_add(sam_resync_server, mp, NULL,
		    SAM_RESYNCING_RETRY_SECS * hz);
		TRACE(T_SAM_SR_RESYNC_RET, mp, 2, mp->mt.fi_status, 0);
		return;
	}
	if (!SAM_IS_SHARED_SERVER(mp)) {
		sam_taskq_remove(entry);
		TRACE(T_SAM_SR_RESYNC_RET, mp, 3, mp->mt.fi_status, 0);
		return;
	}

	mp->mi.m_schedule_flags |= SAM_SCHEDULE_TASK_RESYNCING;
	mutex_exit(&samgt.schedule_mutex);

	/*
	 * If all clients have sent the MOUNT_resync message, then
	 * the server can finish the failover. Otherwise, the server
	 * must wait until the maximum lease time has expired.
	 *
	 * Check for involuntary failover. If all clients except the
	 * previous server have sent the MOUNT_resync message, then
	 * a fast failover can occur.
	 *
	 * We may also have been informed via Sun Cluster
	 * that some hosts are SC_DOWN; we shouldn't allow the
	 * absence of any message(s) from such hosts to delay the
	 * failover.
	 *
	 * . synced is the count of those hosts that have connected
	 *   and resync'ed.  Includes the (local) server itself, so
	 *   it should equal the number of live hosts (when done).
	 *
	 * . sync_awaited is the number of hosts that haven't responded
	 *   or that we haven't gotten sync messages from
	 *
	 * . resyncing is the number of hosts that have responded but
	 *   haven't yet sent sync messages
	 */
	resyncing = sync_awaited = synced = 0;
	srvr_connected = FALSE;
	for (ord = 1;
	    mp->ms.m_clienti != NULL && ord <= mp->ms.m_max_clients; ord++) {

		clp = sam_get_client_entry(mp, ord, 0);

		if (clp == NULL) {
			continue;
		}

		if (clp->cl_sh.sh_fp != NULL && ord == mp->ms.m_client_ord) {
			/*
			 * This is the server, and it has connected
			 */
			srvr_connected = TRUE;
			synced++;
			continue;
		}

		if (!mp->ms.m_involuntary || ord != mp->ms.m_prev_srvr_ord) {
			if ((clp->cl_flags & SAM_CLIENT_SC_DOWN) == 0) {
				/*
				 * Sun Cluster hasn't notified us that this host
				 * is down, so we should try to wait for it.
				 */
				if (clp->cl_sh.sh_fp != NULL &&
				    clp->cl_resync_time >=
				    mp->ms.m_resync_time) {
					synced++;
				} else {
					/*
					 * This is a host we ought to wait for.
					 * If it has connected, but hasn't
					 * completed resync yet, then hold
					 * things up in case it's busy sending
					 * stuff.
					 */
					sync_awaited++;
					if (clp->cl_sh.sh_fp != NULL &&
					    clp->cl_rcv_time >=
					    mp->ms.m_resync_time) {
						int delay = (lbolt -
						    clp->cl_rcv_time) / hz;

	/* N.B. Bad indentation here to meet cstyle requirements */
	if (delay <
	    (3 * SAM_MIN_DELAY)) {
		resyncing++;
		if (clp->cl_flags &
		    SAM_CLIENT_DEAD) {
			clp->cl_flags &= ~SAM_CLIENT_DEAD;
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: client %s responded; "
			    "assuming alive",
			    mp->mt.fi_name, clp->hname);
		}
	} else {
		if ((clp->cl_flags &
		    SAM_CLIENT_DEAD) ==
		    0) {
			clp->cl_flags |= SAM_CLIENT_DEAD;
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: client %s has not "
			    "responded in %d seconds; "
			    "assuming dead",
			    mp->mt.fi_name, clp->hname, delay);
		}
	}
					}
				}
			} else {
				/*
				 * Sun Cluster claimed this host was down at
				 * start of failover.  If it connects, fine, but
				 * don't hold anything up waiting.
				 */
				if (clp->cl_sh.sh_fp != NULL &&
				    clp->cl_resync_time >=
				    mp->ms.m_resync_time) {
					/*
					 * Possibly very bad.  Need to add
					 * flag/check to determine if this was a
					 * fresh mount or a resync.  Fresh mount
					 * = ok, resync = BAD.
					 */
					synced++;
					if (clp->cl_status &
					    (FS_FAILOVER|FS_RESYNCING)) {
						cmn_err(CE_WARN,
						    "SAM-QFS: %s: 'DOWN' "
						    "host %s resynced (%x/%x)",
						    mp->mt.fi_name, clp->hname,
						    mp->mt.fi_status,
						    clp->cl_status);
					}
				}
			}
		} else {
			/*
			 * Involuntary failover, and it's the ex-MDS
			 */
			if (clp->cl_sh.sh_fp != NULL) {
				if (clp->cl_resync_time >=
				    mp->ms.m_resync_time) {
					/*
					 * The ex-MDS has connected.  This is
					 * probably very bad.  Fresh mount = ok,
					 * resync = BAD.
					 */
					synced++;
					if (clp->cl_status &
					    (FS_FAILOVER|FS_RESYNCING)) {
						cmn_err(CE_WARN,
						    "SAM-QFS: %s: Involuntary "
						    "failover: "
						    "ex-MDS %s resynced "
						    "(%x/%x)",
						    mp->mt.fi_name, clp->hname,
						    mp->mt.fi_status,
						    clp->cl_status);
					}
				}
			}
		}
	}

	max_lease = MAX(mp->mt.fi_lease[0], mp->mt.fi_lease[1]);
	max_lease = MAX(max_lease, mp->mt.fi_lease[2]);
	max_lease = MAX(max_lease, DEF_LEASE_TIME);
	diff_time = (lbolt - mp->ms.m_resync_time)/hz;
	failover_done = FALSE;

	if (/* sync_awaited && */ resyncing) {
		cmn_err(CE_NOTE,
		    "SAM-QFS: %s: Failover in progress:"
		    " awaiting resync from %d hosts (1..%d) (%x)",
		    mp->mt.fi_name, resyncing,
		    mp->ms.m_maxord, mp->mt.fi_status);

	} else if (srvr_connected && sync_awaited == 0) {
		failover_done = TRUE;
		cmn_err(CE_NOTE,
		    "SAM-QFS: %s: Failover (%svoluntary):"
		    " %d hosts (1..%d) sent resync (%x)",
		    mp->mt.fi_name,
		    (mp->ms.m_involuntary ? "in" : ""),
		    synced, mp->ms.m_maxord, mp->mt.fi_status);
	} else if (srvr_connected && diff_time >= max_lease) {
		failover_done = TRUE;
		cmn_err(CE_NOTE,
		    "SAM-QFS: %s: Failover (%svoluntary) lease timeout "
		    "(%d sec):"
		    " %d hosts (1..%d) have not sent expected resync (%x)",
		    mp->mt.fi_name, (mp->ms.m_involuntary ? "in" : ""),
		    max_lease, sync_awaited,
		    mp->ms.m_maxord, mp->mt.fi_status);
	}

	if (failover_done) {
		int dones_sent = 0;

		/*
		 * Send faildone message to all the clients.
		 * This causes a SIGHUP to be sent to all daemons.
		 */
		for (ord = 1; ord <= mp->ms.m_max_clients; ord++) {
			int error;

			clp = sam_get_client_entry(mp, ord, 0);

			if (clp == NULL) {
				continue;
			}

			if (clp->cl_sh.sh_fp == NULL) {
				continue;
			}
			if (ord == mp->ms.m_client_ord) {
				/* This is the server */
				dones_sent++;
				mutex_enter(&mp->ms.m_waitwr_mutex);
				mp->mt.fi_status |= FS_SRVR_DONE;
				mutex_exit(&mp->ms.m_waitwr_mutex);
				continue;
			}
			if ((error = sam_shared_failover(mp,
			    MOUNT_faildone, ord)) == 0) {
				dones_sent++;
			} else {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: Error %d sending "
				    "faildone to host %s,"
				    " new server %s, resyncing (%x)",
				    mp->mt.fi_name, error, clp->hname,
				    mp->mt.fi_server,
				    mp->mt.fi_status);
			}
		}
		cmn_err(CE_NOTE,
		    "SAM-QFS: %s: Faildone sent to %d hosts (1..%d),"
		    " resyncing (%x)",
		    mp->mt.fi_name, dones_sent, mp->ms.m_maxord,
		    mp->mt.fi_status);
		sam_failover_done(mp);

	} else {
		mutex_enter(&samgt.schedule_mutex);
		mp->mi.m_schedule_flags &= ~SAM_SCHEDULE_TASK_RESYNCING;
		sam_taskq_remove(entry);	/* releases schedule mutex */
		sam_taskq_add(sam_resync_server, mp, NULL, 2 * hz);
		TRACE(T_SAM_SR_RESYNC_RET, mp, 4, mp->mt.fi_status, 0);
		return;
	}

	mutex_enter(&samgt.schedule_mutex);
	mp->mi.m_schedule_flags &= ~SAM_SCHEDULE_TASK_RESYNCING;
	sam_taskq_remove(entry);

	TRACE(T_SAM_SR_RESYNC_RET, mp, 0, mp->mt.fi_status, 0);
}


/*
 * ----- sam_free_extents - Free indirect extents.
 *
 * Recursively call this routine until all indirect blocks are released.
 */

static void
sam_free_extents(sam_mount_t *mp)
{
	struct sam_rel_blks *block;


	while ((block = mp->mi.m_next) != NULL) {
		mp->mi.m_next = block->next;	/* Update start of list */
		mp->mi.m_inode.busy = 1;
		mutex_exit(&mp->mi.m_inode.mutex);
		if (block->status.b.direct_map) {
			sam_free_direct_map(mp, block);
		} else {
			sam_free_indirect_map(mp, block);
		}
		kmem_free((char *)block, sizeof (struct sam_rel_blks));
		/* reacquire the inode lock */
		mutex_enter(&mp->mi.m_inode.mutex);
		mp->mi.m_inode.busy = 0;
	}
}


/*
 * ----- sam_free_direct_map - Free direct extents.
 *
 * This routine frees direct map blocks.
 */

int				/* ERRNO if error, 0 if successful. */
sam_free_direct_map(
	sam_mount_t *mp,		/* Pointer to the mount table. */
	struct sam_rel_blks *block)	/* release block entry */
{
	int bn;
	int ord;
	int dt;
	offset_t blocks;
	offset_t dau_size;
	offset_t ext_size;
	sam_mode_t metadata = S_ISDIR(block->imode);

	dt = block->status.b.meta;
	bn = block->e.extent[0];
	ord = block->e.extent_ord[0];
	blocks = (offset_t)block->e.extent[1];
	dau_size = (LG_BLK(mp, dt) >> SAM_DEV_BSHIFT);
	ext_size = dau_size >> mp->mi.m_bn_shift;
	while (blocks > 0) {
		if (metadata) {
			LQFS_MSG(CE_WARN, "sam_free_direct_map(): Inode %d: "
			    "Freeing LG block (nb %ld) ext 0x%x (mof 0x%x) "
			    "ord %d\n", block->id.ino,
			    mp->mi.m_dau[MM].size[LG], bn,
			    (bn * mp->mi.m_dau[MM].size[LG]), ord);
			TRANS_CANCEL(mp, (bn * mp->mi.m_dau[MM].size[LG]), ord,
			    mp->mi.m_dau[MM].size[LG], metadata);
		}
		sam_free_block(mp, LG, bn, ord);
		bn += ext_size;
		blocks -= dau_size;
	}
	return (0);
}


/*
 * ----- sam_free_indirect_map - Free indirect extents.
 *
 * This routine frees indirect map blocks.
 */

static int				/* ERRNO if error, 0 if successful. */
sam_free_indirect_map(
	sam_mount_t *mp,		/* Pointer to the mount table. */
	struct sam_rel_blks *block)	/* release block entry */
{
	struct sam_sblk *sblk;
	int i;
	int bn;
	int ord;
	int bt;
	int ileft;
	int kptr[NIEXT];
	int de;
	int error = 0;
	offset_t length;
	sam_mode_t metadata = S_ISDIR(block->imode);

	sblk = mp->mi.m_sbp;
	length = block->length;
	de = 0;
	for (i = 0; i < NIEXT; i++)  kptr[i] = -1;
	if (length != 0) {
		struct sam_get_extent_args arg;
		struct sam_get_extent_rval rval;
		int dt;
		int ord;

		ord = (int)block->e.extent_ord[0];
		arg.dau = &mp->mi.m_dau[0];
		arg.num_group = mp->mi.m_fs[ord].num_group;
		arg.sblk_vers = mp->mi.m_sblk_version;
		arg.status = block->status;
		arg.offset = (offset_t)(length - 1);
		rval.de = &de;
		rval.dtype = &dt;
		rval.btype = &bt;
		rval.ileft = &ileft;
		if ((error = sam_get_extent(&arg, kptr, &rval))) {
			return (error);
		}
	}
	for (i = (NOEXT - 1); i >= 0; i--) {
		if (i < NDEXT) {
			if ((length != 0) && (de == i)) {	/* done */
				break;
			}
			if (block->e.extent[i] == 0)  continue;
			bt = LG;
			if (i < NSDEXT) {
				bt = block->status.b.on_large ?  LG : SM;
			}
			bn = block->e.extent[i];
			ord = block->e.extent_ord[i];
			if (metadata) {
				LQFS_MSG(CE_WARN,
				    "sam_free_indirect_map():"
				    "Inode %d: Freeing direct extent "
				    "block (nb %ld) bn 0x%x "
				    "(mof 0x%x) ord %d\n", block->id.ino,
				    mp->mi.m_dau[MM].size[bt], bn,
				    (bn * mp->mi.m_dau[MM].size[bt]), ord);
				TRANS_CANCEL(mp,
				    (bn * mp->mi.m_dau[MM].size[bt]),
				    ord, mp->mi.m_dau[MM].size[bt], metadata);
			}
			sam_free_block(mp, bt, bn, ord);
			block->e.extent[i] = 0;
			block->e.extent_ord[i] = 0;
		} else {
			if (block->e.extent[i] == 0) {
				if ((length == 0) || (kptr[i - NDEXT] == -1)) {
					continue;
				}
				break;
			}
			error = sam_free_indirect_block(mp, sblk, block, kptr,
			    &block->e.extent[i], &block->e.extent_ord[i],
			    (i - NDEXT));
			if (error == ECANCELED) {
				error = 0;
				break;
			}
		}
	}
	return (error);
}


/*
 * ----- sam_free_indirect_block - Free indirect extents.
 *
 * Recursively call this routine until all indirect blocks are released.
 */

static int				/* ERRNO if error, 0 if successful. */
sam_free_indirect_block(
	struct sam_mount *mp,		/* mount table */
	struct sam_sblk *sblk,		/* Pointer to the superblock */
	struct sam_rel_blks *block,	/* release block entry */
	int kptr[],			/* ary of extents that end the file. */
	uint32_t *extent_bn,		/* mass storage extent block number */
	uchar_t *extent_ord,		/* mass storage extent ordinal */
	int level)			/* level of indirection */
{
	int i;
	buf_t *bp;
	sam_indirect_extent_t *iep;
	uint32_t *ie_addr;
	uchar_t *ie_ord;
	sam_daddr_t bn;
	int error = 0;
	sam_mode_t metadata = S_ISDIR(block->imode);

	bn = *extent_bn;
	bn <<= mp->mi.m_bn_shift;
	error = SAM_BREAD(mp, mp->mi.m_fs[*extent_ord].dev, bn, SAM_INDBLKSZ,
	    &bp);
	if (error == 0) {
		bp->b_flags &= ~B_DELWRI;
		iep = (sam_indirect_extent_t *)(void *)bp->b_un.b_addr;

		/* check for invalid indirect block */
		if (iep->ieno != level) {
			error = ENOCSI;
		} else if ((iep->id.ino != block->id.ino) ||
		    (iep->id.gen != block->id.gen)) {
			error = ENOCSI;
			if (block->status.b.seg_ino) {
				/* Really only true for seg 0 */
				if ((iep->id.ino == block->parent_id.ino) &&
				    (iep->id.gen == block->parent_id.gen)) {
					error = 0;
				}
			}
		}
		if (error) {	/* Invalid indirect block */
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: sam_free_indirect_block: "
			    "invalid indirect block:"
			    " ino=%d.%d.%d val=%d.%d.%d bn=0x%x ord=%d",
			    mp->mt.fi_name, block->id.ino, block->id.gen, level,
			    iep->id.ino, iep->id.gen, iep->ieno,
			    *extent_bn, *extent_ord);
			sam_req_ifsck(mp, *extent_ord,
			    "sam_free_indirect_block: ENOCSI", &iep->id);
			brelse(bp);
			return (ENOCSI);
		}
		for (i = (DEXT - 1); i >= 0; i--) {
			if (kptr[level] == i) {
				error = ECANCELED;
				break;
			}
			ie_addr = &iep->extent[i];
			if (*ie_addr == 0) {	/* If empty */
				continue;
			}
			ie_ord = &iep->extent_ord[i];
			if (level) {
				error = sam_free_indirect_block(mp, sblk,
				    block, kptr, ie_addr,
				    ie_ord, (level - 1));
			} else {
				/*
				 * Free large data block. If directory, may be
				 * meta block.
				 */
				if (metadata) {
					LQFS_MSG(CE_WARN,
					    "sam_free_indirect_block(): "
					    "Inode %d: Freeing LG block "
					    "(nb %ld) ext 0x%x (mof 0x%x) "
					    "ord %d\n", block->id.ino,
					    mp->mi.m_dau[MM].size[LG], *ie_addr,
					    (*ie_addr *
					    mp->mi.m_dau[MM].size[LG]),
					    *ie_ord);
					TRANS_CANCEL(mp,
					    (*ie_addr *
					    mp->mi.m_dau[MM].size[LG]),
					    *ie_ord, mp->mi.m_dau[MM].size[LG],
					    metadata);
				}
				sam_free_block(mp, LG, *ie_addr, *ie_ord);
				*ie_addr = 0;
				*ie_ord = 0;
			}
		}
		if (error != ECANCELED) {
			bp->b_flags |= B_STALE|B_AGE;
			brelse(bp);
			/* Free large meta block */
			if (metadata) {
				LQFS_MSG(CE_WARN, "sam_free_indirect_block(): "
				    "Inode %d: Freeing LG block (nb %ld) "
				    "ext 0x%x (mof 0x%x) ord %d\n",
				    block->id.ino, mp->mi.m_dau[MM].size[LG],
				    *extent_bn, (*extent_bn *
				    mp->mi.m_dau[MM].size[LG]), *extent_ord);
				TRANS_CANCEL(mp, (*extent_bn *
				    mp->mi.m_dau[MM].size[LG]), *extent_ord,
				    mp->mi.m_dau[MM].size[LG], metadata);
			}
			sam_free_block(mp, LG, *extent_bn, *extent_ord);
			*extent_bn = 0;
			*extent_ord = 0;
		} else {
			bdwrite(bp);
		}
	}
	return (error);
}


/*
 * ----- sam_wait_release_blk_list - Wait until released blocks
 * are merged in maps and superblock is updated.
 */

void
sam_wait_release_blk_list(
	struct sam_mount *mp)	/* mount table */
{
	mutex_enter(&mp->mi.m_inode.mutex);
	while (mp->mi.m_next != NULL || mp->mi.m_inode.busy) {
		mutex_enter(&mp->mi.m_inode.put_mutex);
		cv_signal(&mp->mi.m_inode.put_cv);
		mutex_exit(&mp->mi.m_inode.put_mutex);

		mp->mi.m_inode.wait++;
		cv_wait(&mp->mi.m_inode.get_cv, &mp->mi.m_inode.mutex);
		mp->mi.m_inode.wait--;
	}
	mutex_exit(&mp->mi.m_inode.mutex);
}


/*
 * ----- sam_free_block - process free block.
 *
 * Put the block in the release table.  If release table is full, update
 * the file system.
 */

void
sam_free_block(
	sam_mount_t *mp,	/* The mount table pointer. */
	int bt,			/* The block type: small or large. */
	sam_bn_t bn,		/* The block number. */
	int ord)		/* The ordinal in the disk family set. */
{
	int i;
	int dt;			/* The disk type: data or meta. */
	sam_fb_pool_t *fbp;
	int active;

	fbp = &mp->mi.m_fb_pool[bt];
	active = fbp->active;
	ASSERT(active < SAM_NO_FB_ARRAY);
	mutex_enter(&fbp->array[active].fb_mutex);
	i = fbp->array[active].fb_count;
	ASSERT(i < SAM_FB_ARRAY_LEN);
	dt = (mp->mi.m_fs[ord].part.pt_type == DT_META) ? MM : DD;
	if ((mp->mi.m_blk_bn == 0) && (bt != SM) &&
	    (dt == mp->mi.m_inoblk->di.status.b.meta)) {
		/*
		 * Keep in reserve 1 large meta block for the .blocks file.
		 */
		mp->mi.m_blk_bn = bn;
		mp->mi.m_blk_ord = ord;
	} else {
		fbp->array[active].fb_ord[i] = (uchar_t)ord;
		fbp->array[active].fb_bn[i] = bn;
		fbp->array[active].fb_count++;

		/*
		 * Release blocks if array is full
		 */
		if (fbp->array[active].fb_count >= SAM_FB_ARRAY_LEN) {
			sam_release_blocks(mp, fbp, bt, active);
		}
	}
	mutex_exit(&fbp->array[active].fb_mutex);
}


/*
 * ----- sam_drain_blocks - flush all blocks to disk.
 *
 */

static void
sam_drain_blocks(
	sam_mount_t *mp)	/* The mount table pointer. */
{
	int bt;
	sam_fb_pool_t *fbp;
	int active;

	for (bt = 0; bt < SAM_MAX_DAU; bt++) {
		fbp = &mp->mi.m_fb_pool[bt];
		for (active = 0; active < SAM_NO_FB_ARRAY; active++) {
			mutex_enter(&fbp->array[active].fb_mutex);
			sam_release_blocks(mp, fbp, bt, active);
			mutex_exit(&fbp->array[active].fb_mutex);
		}
	}
}


/*
 * ----- sam_release_blocks - process release blocks in free pool
 * Move active to the alternate array for removal use. Process the
 * release blocks SM or LG array. After the array has been released,
 * set the fb_count to zero in the array.
 */

static void
sam_release_blocks(
	sam_mount_t *mp,	/* The mount table pointer. */
	sam_fb_pool_t *fbp,
	int bt,
	int active)
{
	sam_node_t *ip = mp->mi.m_inoblk;
	int j;
	int inoblk_lock	= 0;

	fbp->active = (active + 1) & 1;
	for (j = 0; j < fbp->array[active].fb_count; j++) {
		if (bt == SM) {
			if (inoblk_lock == 0) {
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				inoblk_lock = 1;
			}
			(void) sam_merge_sm_blks(mp->mi.m_inoblk,
			    fbp->array[active].fb_bn[j],
			    fbp->array[active].fb_ord[j]);
		} else {
			(void) sam_free_lg_block(mp,
			    fbp->array[active].fb_bn[j],
			    fbp->array[active].fb_ord[j]);
		}
	}
	fbp->array[active].fb_count = 0;
	if (inoblk_lock == 1) {
		TRANS_INODE(mp, ip);
		ip->flags.b.updated = 1;
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	}
}

/*
 * ----- sam_compact_lease_clients - remove expired clients from inode
 * inode ilease_mutex must be held
 *
 * To minimize copying, we pull entries from the end (skipping
 * expired entries) to fill in any expired entries elsewhere.
 */
static void
sam_compact_lease_clients(struct sam_lease_ino *llp)
{
	int i, j;

	ASSERT(MUTEX_HELD(&llp->ip->ilease_mutex));
	for (i = 0; i <= (llp->no_clients - 1); i++) {
		/*
		 * Lease still active?  Go on to check the next.
		 */
		if ((llp->lease[i].leases != 0) ||
		    (llp->lease[i].wt_leases != 0)) {
			continue;
		}

		/*
		 * Find a non-expired lease towards the end of
		 * the array.
		 */
		for (j = (llp->no_clients - 1); j > i; j--) {
			if ((llp->lease[j].leases == 0) &&
			    (llp->lease[j].wt_leases == 0)) {
				llp->no_clients--;
			} else {
				break;
			}
		}

		/* Account for this expired lease. */
		llp->no_clients--;

		/* If we found no more leases, we're done. */
		if (j == i) {
			break;
		}

		/*
		 * We found a lease, move it to this location.
		 */
		llp->lease[i] = llp->lease[j];
	}
}
