/*
 * ----- creclaim.c - Asynchronusly reclaim common sam resources.
 *
 * Inactivates the eligible inactive inodes.
 * Client expires leases for client inodes in a shared file system.
 * Client reestablishes leases for client inodes in a shared file system.
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
#pragma ident "$Revision: 1.98 $"
#endif

#include "sam/osversion.h"

/*
 * ----- UNIX Includes
 */

#ifdef sun
#include <sys/types.h>
#include <sys/param.h>
#include <sys/t_lock.h>
#include <sys/sysmacros.h>
#include <sys/debug.h>
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
#include <sys/taskq.h>
#endif /* sun */

#ifdef linux
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/file.h>
#include <linux/uio.h>
#include <linux/dirent.h>
#include <linux/proc_fs.h>
#include <linux/limits.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#include <linux/buffer_head.h>
#endif

#include <asm/param.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#include <asm/statfs.h>
#endif
#endif /* linux */

/*
 * ----- SAMFS Includes
 */

#include "sam/param.h"
#include "sam/types.h"

#include "macros.h"
#include "sblk.h"
#include "inode.h"
#include "mount.h"
#include "sam_thread.h"

#ifdef sun
#include "extern.h"
#include "kstats.h"
#endif /* sun */

#ifdef linux
#include "clextern.h"
#include "kstats_linux.h"
#endif /* linux */

#include "debug.h"
#include "trace.h"
#include "qfs_log.h"


static void sam_reestablish_lease(sam_node_t *ip);

extern void sam_release_vnodes(sam_schedule_entry_t *entry);
extern int max_vnodes_in_block;


/*
 * ----- sam_inactivate_inodes
 *
 * Inactivate inodes which have been idle for SAM_INACTIVE_DEFER_SECS.
 *
 * This is to prevent thrashing of NFS inodes which are being
 * repeatedly accessed when the inode hash table is full.
 *
 * XXX - Should this be applied to shared client inodes as well?
 */

#ifdef	sun
void
sam_inactivate_inodes(sam_schedule_entry_t *entry)
{
	sam_mount_t *mp;
	struct sam_inv_inos *deact_ptr, *deact_next, *inv_ptr, *inv_prev;
	clock_t now, wait_time;
	sam_schedule_entry_t *vn_rel;
	int taskq_iterations = 0;
	int vnode_count = 0;
	vnode_t **vnode_list = NULL;

	mp = entry->mp;

	TRACE(T_SAM_INVAL, mp, mp->mt.fi_status, mp->mi.m_inval_count, 0);

	SAM_COUNT64(thread, inactive_count);

	mutex_enter(&samgt.schedule_mutex);
	sam_taskq_remove_nocount(entry);

	/*
	 * Locate the inodes to be deactivated.  If we're unmounting, that's all
	 * of them. Otherwise, it's only the ones which are old enough.  Do not
	 * deactivate inodes that are staging.  Otherwise, we lose the staging
	 * bit which results in duplicate stage requests and duplicate
	 * allocation of blocks.  We dequeue these from the mount point before
	 * deactivating.
	 */

	mutex_enter(&mp->mi.m_inode.mutex);

	mutex_enter(&mp->ms.m_fo_vnrele_mutex);
	mp->ms.m_fo_vnrele_count++;
	mutex_exit(&mp->ms.m_fo_vnrele_mutex);

	inv_ptr = mp->mi.m_invalp;
	inv_prev = NULL;
	deact_ptr = NULL;
	deact_next = NULL;
	now = ddi_get_lbolt();
	wait_time = 0;
	if (mp->mi.m_inode.flag == SAM_THR_MOUNT && mp->mi.m_inode.wait == 0) {
		clock_t interval = SAM_INACTIVE_DEFER_SECS * hz;

		mp->mi.m_invalp = NULL;

		/*
		 * The list is ordered "newest ... oldest".
		 */
		while (inv_ptr != NULL) {
			sam_node_t *ip;

			ip = inv_ptr->ip;
			if ((inv_ptr->entry_time + interval) <= now) {
				/*
				 * Build a list of inodes to be deactivated.
				 * Skip the staging inodes.
				 */
				if (ip->flags.b.staging) {
					/*
					 * wait_time is set for the newest
					 * staging entry that has exceeded its
					 * defer inactive interval, if all other
					 * entries have exceeded their interval.
					 */
					if (wait_time == 0) {
						wait_time =
						    inv_ptr->entry_time +
						    interval;
					}
					/*
					 * m_invalp is the first entry on the
					 * defer inactive list.
					 */
					if (mp->mi.m_invalp == NULL) {
						mp->mi.m_invalp = inv_ptr;
					}
					/*
					 * Leave this entry on the defer
					 * inactive list.
					 */
					inv_prev = inv_ptr;
					inv_ptr = inv_ptr->next;
				} else {
					/*
					 * Put this entry at the front of the
					 * deactivate list.  deact_ptr is the
					 * first entry of the deactivate list.
					 * inv_prev is null if there are no
					 * entries currently on the defer
					 * inactive list.
					 */
					if (inv_prev != NULL) {
						inv_prev->next = inv_ptr->next;
					}
					deact_next = deact_ptr;
					deact_ptr = inv_ptr;
					inv_ptr = inv_ptr->next;
					deact_ptr->next = deact_next;
					mp->mi.m_inval_count--;
				}
			} else {
				/*
				 * wait_time is set for the oldest entry that
				 * has not exceeded its defer inactive interval.
				 */
				wait_time = inv_ptr->entry_time + interval;
				/*
				 * m_invalp is the first entry on the defer
				 * inactive list.
				 */
				if (mp->mi.m_invalp == NULL) {
					mp->mi.m_invalp = inv_ptr;
				}
				/*
				 * Leave this entry on the defer inactive list.
				 */
				inv_prev = inv_ptr;
				inv_ptr = inv_ptr->next;
			}
		}
	} else {
		/*
		 * If unmounting, deactivate all of the inodes.
		 */
		deact_ptr = mp->mi.m_invalp;
		mp->mi.m_invalp = NULL;
		mp->mi.m_inval_count = 0;
	}

	mutex_exit(&mp->mi.m_inode.mutex);

	/*
	 * Deactivate the selected inodes.
	 */
	while (deact_ptr != NULL) {
		sam_node_t *ip;
		struct sam_inv_inos *rip;

		ip = deact_ptr->ip;
		ASSERT(deact_ptr->id.ino == ip->di.id.ino &&
		    deact_ptr->id.gen == ip->di.id.gen);

		if (vnode_count == 0) {
			vnode_list = kmem_alloc((max_vnodes_in_block + 1) *
			    sizeof (vnode_t *), KM_SLEEP);
		}

		vnode_list[vnode_count++] = SAM_ITOV(ip);
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
			    (sam_schedule_entry_t *)vn_rel) == 0) {
				/*
				 * Try this loop for awhile.  If we're stuck,
				 * do the work in-line.
				 */

				if (taskq_iterations++ >=
				    MAX_TASKQ_DISPATCH_ATTEMPTS) {
					vn_rel->mp = NULL;
					sam_release_vnodes(vn_rel);

					mutex_enter(&mp->ms.m_fo_vnrele_mutex);
					mp->ms.m_fo_vnrele_count--;
					mutex_exit(&mp->ms.m_fo_vnrele_mutex);

					break;
				}
			}
			vnode_list = NULL;
		}

		rip = deact_ptr;
		deact_ptr = deact_ptr->next;
		kmem_free((char *)rip, sizeof (struct sam_inv_inos));
	}

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

		while (sam_taskq_dispatch(
		    (sam_schedule_entry_t *)vn_rel) == 0) {
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
		 * Notify any waiters that this thread
		 * is done releasing vnodes.
		 */
		cv_broadcast(&mp->ms.m_fo_vnrele_cv);
	}
	mutex_exit(&mp->ms.m_fo_vnrele_mutex);

	if (wait_time != 0) {
		clock_t ticks;

		ticks = wait_time - now;
		if (ticks < SAM_INACTIVE_MIN_TICKS) {
			ticks = SAM_INACTIVE_MIN_TICKS;
		}
		sam_taskq_add(sam_inactivate_inodes, mp, NULL, ticks);
	}

	sam_taskq_uncount(mp);

	TRACE(T_SAM_INVAL_RET, mp, 0, mp->mt.fi_status, mp->mi.m_inval_count);
}
#endif	/* sun */


/*
 * ----- sam_expire_client_leases
 * Process the expired leases of this client.
 */

static void
__sam_expire_client_leases(sam_schedule_entry_t *entry)
{
	sam_mount_t *mp;
	sam_nchain_t *chain;
	clock_t new_wait_time, now;
	int forced_expiration;
	uint32_t saved_leasegen[SAM_MAX_LTYPE];
	boolean_t rerun = FALSE;
	clock_t ticks = (clock_t)0;

	mp = entry->mp;

	TRACE(T_SAM_CL_EXPIRE, mp, mp->mt.fi_status, 0, 0);

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
	    SAM_SCHEDULE_TASK_CLIENT_RECLAIM_RUNNING) {
		mutex_exit(&samgt.schedule_mutex);
		sam_taskq_uncount(mp);
		SAM_COUNT64(shared_client, expire_task_dup);
		TRACE(T_SAM_CL_EXPIRE_RET, mp, 1, mp->mt.fi_status, 0);
		return;
	}
	mp->mi.m_schedule_flags |= SAM_SCHEDULE_TASK_CLIENT_RECLAIM_RUNNING;
	mutex_exit(&samgt.schedule_mutex);

	SAM_COUNT64(shared_client, expire_task);

	/*
	 * If umounting or umounted, expire all the leases now.
	 * Do not expire client leases during failover or server down.
	 */
	new_wait_time = LONG_MAX;
	if (!(mp->mt.fi_status & (FS_LOCK_HARD|FS_FAILOVER|FS_RESYNCING)) &&
	    (mp->mi.m_inode.flag & (SAM_THR_UMOUNT|SAM_THR_UMOUNTING))) {
		now = 0;
		forced_expiration = 1;
	} else {
		now = ddi_get_lbolt();
		forced_expiration = 0;
	}

	/*
	 * Pre-scan the list of inodes with client leases to clear flag.
	 */
	mutex_enter(&mp->mi.m_lease_mutex);

	chain = mp->mi.m_cl_lease_chain.forw;
	while (chain != &mp->mi.m_cl_lease_chain) {
		sam_node_t *ip;

		ip = chain->node;
		chain = chain->forw;

		ip->lease_flags &= ~SAM_LEASE_FLAG_CLIENT;
	}

	/*
	 * Check inodes in the lease linked list. Expire inodes and
	 * potentially inactivate them.
	 */
	chain = mp->mi.m_cl_lease_chain.forw;
	while (chain != &mp->mi.m_cl_lease_chain) {
		sam_node_t *ip;

		/*
		 * Delay expiring client leases during failover or server down.
		 */
		if (mp->mt.fi_status & (FS_LOCK_HARD|FS_FAILOVER|
		    FS_RESYNCING)) {
			new_wait_time = now + (5 * hz);
			break;
		}

		ip = chain->node;
		chain = chain->forw;

		/* Don't process inodes more than once. */

		if (ip->lease_flags & SAM_LEASE_FLAG_CLIENT) {
			continue;
		}

		ip->lease_flags |= SAM_LEASE_FLAG_CLIENT;

		/*
		 * Expire each lease type based on its timeout.  Leases which
		 * don't time out are always extended 30 seconds into the
		 * future.  If we're staging, we automatically extend leases,
		 * as it may take a long time to mount and position the media.
		 *
		 * If we are forcing leases to expire, we don't need to look
		 * at the individual leases.
		 */

		if (forced_expiration) {
			mutex_enter(&ip->ilease_mutex);
			ip->cl_leases = 0;
			ip->cl_short_leases = 0;
			ip->cl_extend_leases = 0;
			sam_client_remove_lease_chain(ip);

			/* Must not hold lease mutex while releasing vnode. */

			mutex_exit(&ip->ilease_mutex);
			mutex_exit(&mp->mi.m_lease_mutex);
			VN_RELE_OS(ip);
			mutex_enter(&mp->mi.m_lease_mutex);
			chain = mp->mi.m_cl_lease_chain.forw;
		} else {
			ushort_t initial_leases;
			ushort_t remaining_leases;
			ushort_t removed_leases;
			ushort_t extend_leases = 0;
			int ltype;
			int lease_mask;
			int duration;
			clock_t min_ltime = LONG_MAX;
			int time_valid = 0;
			boolean_t staging;

			mutex_enter(&ip->ilease_mutex);

			initial_leases = ip->cl_leases;
			remaining_leases = initial_leases;
			staging = ip->flags.b.staging;

			for (ltype = 0; ltype < SAM_MAX_LTYPE; ltype++) {
				lease_mask = (1 << ltype);
				if (initial_leases & lease_mask) {
					/*
					 * Extend non-expiring leases 30 secs.
					 */
					if ((lease_mask &
					    SAM_NON_EXPIRING_LEASES) ||
					    staging) {
						ip->cl_leasetime[ltype] =
						    now + (30 * hz);
					}
					/*
					 * If the lease expired and is not in
					 * use, remove it.  If it is in use,
					 * verify the expiration time.
					 */
					if ((ip->cl_leasetime[ltype] <= now) &&
					    ((ltype >= MAX_EXPIRING_LEASES) ||
					    (ip->cl_leaseused[ltype] == 0))) {
						remaining_leases &= ~lease_mask;
					} else {
						min_ltime = MIN(min_ltime,
						    ip->cl_leasetime[ltype]);
						time_valid = 1;
					}
				}
				saved_leasegen[ltype] = ip->cl_leasegen[ltype];
			}
			removed_leases = (initial_leases & ~remaining_leases);

			/*
			 * If we are expiring the last mmap lease, try to
			 * extend the read or write lease in order to avoid
			 * invalidating the pages.  Otherwise, all the mappers
			 * will have to fault in the pages from disk
			 * (performance hit).
			 */
			if ((remaining_leases & CL_MMAP) &&
			    (removed_leases & (CL_READ|CL_WRITE)) &&
			    !(remaining_leases & (CL_READ|CL_WRITE))) {
				/*
				 * May have expired both, but only extend one.
				 * WRITE has preference over READ since it will
				 * avoid more page invalidations.
				 */
				if (removed_leases & CL_WRITE) {
					ltype = LTYPE_write;
					extend_leases = CL_WRITE;
				} else {
					ltype = LTYPE_read;
					extend_leases = CL_READ;
				}
				/*
				 * Do not extend the lease if one is pending
				 * or there is a request to relinquish it.
				 * Extend the lease by a half interval for now.
				 * It will be extended by a full interval if
				 * the request is successful.  We want to avoid
				 * having the server forcibly expire the lease.
				 */
				if ((ip->cl_extend_leases & extend_leases) ||
				    (ip->cl_short_leases & extend_leases)) {
					extend_leases = 0;
				} else {
					ip->cl_extend_leases |= extend_leases;
					remaining_leases |= extend_leases;
					removed_leases = (initial_leases &
					    ~remaining_leases);
					duration =
					    ip->mp->mt.fi_lease[ltype] / 2;
					ip->cl_leasetime[ltype] =
					    ddi_get_lbolt() + (duration * hz);
					min_ltime = MIN(min_ltime,
					    ip->cl_leasetime[ltype]);
					time_valid = 1;
				}
			}

			if (removed_leases & CL_APPEND) {
				ip->cl_saved_leases =
				    (ip->cl_leases & CL_APPEND);
			}
			ip->cl_leases = remaining_leases;
			ip->cl_short_leases &= ~removed_leases;
			if (remaining_leases == 0) {
				sam_client_remove_lease_chain(ip);
			}

			if (removed_leases || extend_leases) {
				int flags = B_ASYNC;
				int mmap_flush = 0;

				/*
				 * If remaining_leases, increment the vnode
				 * reference count so close doesn't remove
				 * this inode out from under us.
				 * If no remaining_leases, we don't need
				 * to do this since we removed this inode
				 * from the lease chain.
				 * Note that extend_leases will always have
				 * remaining_leases.
				 * We may be both removing and extending
				 * leases here.
				 */
				if (remaining_leases != 0) {
					VN_HOLD_OS(ip);
				}
				mutex_exit(&ip->ilease_mutex);
				mutex_exit(&mp->mi.m_lease_mutex);

				/*
				 * If we are expiring any leases for mmap,
				 * we will have to flush dirty pages.
				 * If we are expiring the last lease for mmap,
				 * we will also have to invalidate dirty pages.
				 * This prevents mmap applications from using
				 * potentially old data.
				 */
				if ((remaining_leases & CL_MMAP) &&
				    (removed_leases & (CL_READ|CL_WRITE))) {
					mmap_flush = 1;
					if (!(remaining_leases &
					    (CL_READ|CL_WRITE))) {
						flags |= B_INVAL;
					}
				}

				if ((removed_leases &
				    SAM_DATA_MODIFYING_LEASES) || mmap_flush) {
#ifdef linux
					struct inode *li = SAM_SITOLI(ip);
#endif /* linux */

#ifdef	sun
					(void) VOP_PUTPAGE_OS(SAM_ITOV(ip), 0,
					    0, flags, kcred, NULL);
					(void) sam_wait_async_io(ip, FALSE);
#endif	/* sun */
#ifdef linux
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
					RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
					rfs_write_inode_now(li, 0);
					if (flags & B_INVAL) {
						invalidate_inode_pages(
						    li->i_mapping);
					}
					RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
#else
					RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
					fsync_inode_data_buffers(li);
					if (flags & B_INVAL) {
						invalidate_inode_pages(li);
					}
					RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
#endif
#endif /* linux */
				}

				/*
				 * If we're expiring an append lease, we must
				 * wait for the server's response because the
				 * size must be updated.  Start a thread who
				 * will wait for the server size update.  The
				 * current lease generation numbers guarantee
				 * that a later append lease will not be
				 * overwritten by this relinquish lease. Stale
				 * indirects or corruption will occur over a
				 * failover -- not staled during failover.
				 */
				if ((removed_leases & CL_APPEND) != 0) {
					TRACE(T_SAM_CL_LEASE_RM, SAM_ITOP(ip),
					    ip->di.id.ino,
					    ip->mp->ms.m_client_ord,
					    removed_leases);
					SAM_COUNT64(shared_client, stale_indir);
					(void) sam_stale_indirect_blocks(ip,
					    ip->di.rm.size);
					sam_start_relinquish_task(ip,
					    removed_leases, saved_leasegen);
				} else if (removed_leases) {
					/*
					 * For expiration of all leases without
					 * the append lease, we do not wait for
					 * a response.
					 */
					TRACE(T_SAM_CL_LEASE_RM, SAM_ITOP(ip),
					    ip->di.id.ino,
					    ip->mp->ms.m_client_ord,
					    removed_leases);
					(void) sam_proc_relinquish_lease(ip,
					    removed_leases, FALSE,
					    saved_leasegen);
				}

				/*
				 * If we're extending a lease, we must wait for
				 * the server's response in order to process
				 * errors.
				 */
				if (extend_leases) {
					TRACE(T_SAM_CL_LEASE_EXT, SAM_ITOP(ip),
					    ip->di.id.ino,
					    ip->mp->ms.m_client_ord,
					    extend_leases);
					sam_start_extend_task(ip,
					    extend_leases);
				}

				/*
				 * Do this unconditionally since either we
				 * removed all the leases and need to remove
				 * the lease hold on the inode, or we didn't
				 * remove all the leases and did the VN_HOLD_OS
				 * above.
				 */
				VN_RELE_OS(ip);

				mutex_enter(&mp->mi.m_lease_mutex);
				chain = mp->mi.m_cl_lease_chain.forw;
			} else {
				mutex_exit(&ip->ilease_mutex);
			}

			/*
			 * Keep track of the lease that will expire next.
			 * We will schedule the expire task accordingly.
			 */
			if (time_valid && (min_ltime < new_wait_time)) {
				new_wait_time = min_ltime;
			}
		}
	}

	mutex_exit(&mp->mi.m_lease_mutex);

	/*
	 * Schedule another run of sam_expire_client_leases unless
	 * there is an additional copy scheduled, or unless there are
	 * no more leases.
	 */

	mutex_enter(&samgt.schedule_mutex);
	mp->mi.m_schedule_flags &= ~SAM_SCHEDULE_TASK_CLIENT_RECLAIM_RUNNING;
	mp->mi.m_cl_leasesch = 0;
	if (new_wait_time != LONG_MAX) {
		rerun = TRUE;
		mp->mi.m_cl_leasesch = 1;
		ticks = new_wait_time - ddi_get_lbolt();
		if (ticks < SAM_EXPIRE_MIN_TICKS) {
			ticks = SAM_EXPIRE_MIN_TICKS;
		}
		if (ticks > SAM_EXPIRE_MAX_TICKS) {
			ticks = SAM_EXPIRE_MAX_TICKS;
		}
		mp->mi.m_cl_leasenext = ticks + ddi_get_lbolt();
	}
	mutex_exit(&samgt.schedule_mutex);

	if (rerun) {
		if (sam_taskq_add_ret(sam_expire_client_leases, mp,
		    NULL, ticks)) {
			mutex_enter(&samgt.schedule_mutex);
			mp->mi.m_cl_leasesch = 0;
			mutex_exit(&samgt.schedule_mutex);
		}
	}

	sam_taskq_uncount(mp);

	TRACE(T_SAM_CL_EXPIRE_RET, mp, 0, mp->mt.fi_status, 0);
}

#ifdef sun
void
sam_expire_client_leases(sam_schedule_entry_t *entry)
{
	sam_mount_t *mp = entry->mp;

	sam_open_operation_nb(mp);

	__sam_expire_client_leases(entry);

	SAM_CLOSE_OPERATION(mp, 0);
}
#endif /* sun */

#ifdef linux
int
sam_expire_client_leases(void *arg)
{
	sam_schedule_entry_t *entry;
	sam_mount_t *mp;
	int error = 0;

	entry = task_begin(arg);
	mp = entry->mp;
	sam_open_operation_nb(mp);

	__sam_expire_client_leases(entry);

	SAM_CLOSE_OPERATION(mp, error);
	return (error);
}
#endif /* linux */


/*
 * Schedule a run of sam_expire_client_leases if there is not one running
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
sam_sched_expire_client_leases(
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
	next = ticks + ddi_get_lbolt();
	if (force || (mp->mi.m_cl_leasesch == 0) ||
	    (next < (mp->mi.m_cl_leasenext - SAM_EXPIRE_MIN_TICKS))) {
		mp->mi.m_cl_leasenext = next;
		mp->mi.m_cl_leasesch = 1;
		run = TRUE;
	}
	mutex_exit(&samgt.schedule_mutex);
	if (run) {
		if (sam_taskq_add_ret(sam_expire_client_leases, mp,
		    NULL, ticks)) {
			mutex_enter(&samgt.schedule_mutex);
			mp->mi.m_cl_leasesch = 0;
			mutex_exit(&samgt.schedule_mutex);
		}
	}
}


typedef struct sam_schedule_relinquish_entry {
	struct sam_schedule_entry	schedule_entry;
	uint32_t			lease_mask;
	uint32_t			leasegen[SAM_MAX_LTYPE];
} sam_schedule_relinquish_entry_t;


/*
 * ----- sam_start_relinquish_task - start a task to relinquish leases.
 * The task waits for the response and then terminates.
 */

void
sam_start_relinquish_task(
	sam_node_t *ip,			/* Pointer to the inode */
	ushort_t removed_leases,	/* Remove leases mask */
	uint32_t *leasegen)		/* Array of lease generation numbers */
{
	sam_mount_t *mp = ip->mp;
	sam_schedule_relinquish_entry_t *ep;
	int i;

	ep = (sam_schedule_relinquish_entry_t *)kmem_alloc(
	    sizeof (sam_schedule_relinquish_entry_t), KM_SLEEP);
	sam_init_schedule_entry(&ep->schedule_entry, mp, sam_relinquish_leases,
	    (void *)ip);
	ep->lease_mask = removed_leases;
	for (i = 0; i < SAM_MAX_LTYPE; i++) {
		ep->leasegen[i] = *leasegen++;
	}
	VN_HOLD_OS(ip);

	if (sam_taskq_dispatch((sam_schedule_entry_t *)ep) == 0) {
		VN_RELE_OS(ip);
		kmem_free(ep, sizeof (sam_schedule_relinquish_entry_t));
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: can't start task to relinquish append lease:"
		    " ino=%d.%d size=%llx",
		    mp->mt.fi_name, ip->di.id.ino, ip->di.id.gen,
		    ip->di.rm.size);
	}
}


/*
 * ----- sam_relinquish_leases - relinquish lease.
 * Relinquish specified leases and wait for the response.
 */

static void
__sam_relinquish_leases(sam_schedule_entry_t *entry)
{
	struct sam_schedule_relinquish_entry *ep;
	sam_mount_t *mp;
	sam_node_t *ip;
	/* LINTED [set but not used] */
	int error;

	mp = entry->mp;
	ip = (sam_node_t *)entry->arg;
	ep = (sam_schedule_relinquish_entry_t *)entry;

	sam_open_operation_nb(mp);

	error = sam_proc_relinquish_lease(ip, ep->lease_mask,
	    TRUE, ep->leasegen);

	SAM_CLOSE_OPERATION(mp, error);

	VN_RELE_OS(ip);
	kmem_free(entry, sizeof (sam_schedule_relinquish_entry_t));
	sam_taskq_uncount(mp);
}

#ifdef sun
void
sam_relinquish_leases(sam_schedule_entry_t *entry)
{
	__sam_relinquish_leases(entry);
}
#endif /* sun */

#ifdef linux
int
sam_relinquish_leases(void *arg)
{
	sam_schedule_entry_t *entry;

	entry = task_begin(arg);
	__sam_relinquish_leases(entry);
	return (0);
}
#endif /* linux */


typedef struct sam_schedule_extend_entry {
	struct sam_schedule_entry	schedule_entry;
	uint32_t			lease_mask;
} sam_schedule_extend_entry_t;


/*
 * ----- sam_start_extend_task - start a task to extend leases.
 * The task waits for the response and then terminates.
 */

void
sam_start_extend_task(
	sam_node_t *ip,			/* Pointer to the inode */
	ushort_t extend_leases)		/* Extend leases mask */
{
	sam_mount_t *mp = ip->mp;
	sam_schedule_extend_entry_t *ep;

	ep = (sam_schedule_extend_entry_t *)kmem_alloc(
	    sizeof (sam_schedule_extend_entry_t), KM_SLEEP);
	sam_init_schedule_entry(&ep->schedule_entry, mp, sam_extend_leases,
	    (void *)ip);
	ep->lease_mask = extend_leases;
	VN_HOLD_OS(ip);

	if (sam_taskq_dispatch((sam_schedule_entry_t *)ep) == 0) {
		VN_RELE_OS(ip);
		kmem_free(ep, sizeof (sam_schedule_extend_entry_t));
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: can't start task to extend lease:"
		    " ino=%d.%d",
		    mp->mt.fi_name, ip->di.id.ino, ip->di.id.gen);
	}
}


/*
 * ----- sam_extend_leases - extend lease.
 * Extend specified leases and wait for the response.
 */

static void
__sam_extend_leases(sam_schedule_entry_t *entry)
{
	struct sam_schedule_extend_entry *ep;
	sam_mount_t *mp;
	sam_node_t *ip;
	/* LINTED [set but not used] */
	int error;

	mp = entry->mp;
	ip = (sam_node_t *)entry->arg;
	ep = (sam_schedule_extend_entry_t *)entry;

	sam_open_operation_nb(mp);

	error = sam_proc_extend_lease(ip, ep->lease_mask);

	SAM_CLOSE_OPERATION(mp, error);

	VN_RELE_OS(ip);
	kmem_free(entry, sizeof (sam_schedule_extend_entry_t));
	sam_taskq_uncount(mp);
}

#ifdef sun
void
sam_extend_leases(sam_schedule_entry_t *entry)
{
	__sam_extend_leases(entry);
}
#endif /* sun */

#ifdef linux
int
sam_extend_leases(void *arg)
{
	sam_schedule_entry_t *entry;

	entry = task_begin(arg);
	__sam_extend_leases(entry);
	return (0);
}
#endif /* linux */


/*
 * ----- sam_reestablish_leases - reestablish the leases on the server.
 * This function resets leases on the server (finishes the
 * thawing phase) and then sets the FS_RESYNCING flag.
 */

static void
__sam_reestablish_leases(sam_schedule_entry_t *entry)
{
	sam_nchain_t *chain;
	int error = 0;
	int server_ord;
	sam_mount_t *mp;
	sam_node_t *ip;

	mp = entry->mp;

	TRACE(T_SAM_CL_REEST, mp, mp->mt.fi_status, mp->mi.m_inode.flag, 0);

	/*
	 * Sanity checks.  The mount point should be in the frozen state,
	 * and we shouldn't already be running this task.
	 */

	mutex_enter(&samgt.schedule_mutex);
	if (!(mp->mt.fi_status & FS_FROZEN)) {
		sam_taskq_remove(entry);
		TRACE(T_SAM_CL_REEST_RET, mp, 1, mp->mt.fi_status,
		    mp->mi.m_inode.flag);
		return;
	}
	if (mp->mi.m_schedule_flags & SAM_SCHEDULE_TASK_THAWING) {
		sam_taskq_remove(entry);
		TRACE(T_SAM_CL_REEST_RET, mp, 2, mp->mt.fi_status,
		    mp->mi.m_inode.flag);
		return;
	}

	/*
	 * XXX - It's better to fix the logic for
	 * when this task gets started.
	 */
	if (SAM_IS_SHARED_SERVER(mp) && (mp->ms.m_max_clients <= 0)) {
		sam_taskq_remove(entry);
		sam_taskq_add(sam_reestablish_leases, mp, NULL, 2);
		TRACE(T_SAM_CL_REEST_RET, mp, 3, mp->mt.fi_status,
		    mp->mi.m_inode.flag);
		return;
	}

	/*
	 * OK.  We're actually going to run now.
	 */

	mp->mi.m_schedule_flags |= SAM_SCHEDULE_TASK_THAWING;
	mutex_exit(&samgt.schedule_mutex);

restart:
	server_ord = mp->ms.m_server_ord;

	/*
	 * Change failover state to thawing. Leases are
	 * reestablished during the thawing state.
	 */
	mp->mt.fi_status &= ~(FS_FREEZING | FS_FROZEN);
	mp->mt.fi_status |= FS_THAWING;
	cmn_err(CE_NOTE,
	    "SAM-QFS: %s: Reset leases,"
	    " then send resync message to new server %s, thawing (%x)",
	    mp->mt.fi_name, mp->mt.fi_server, mp->mt.fi_status);

	/*
	 * Tell Server that failover is beginning and the server should
	 * reset its sequence number. If cannot send message and the server
	 * changed, wake up sam-sharefsd daemon to reestablish the socket to
	 * the new server.
	 */
	{
		sam_san_mount_msg_t *msg;
		int count;

		msg = kmem_zalloc(sizeof (sam_san_mount_msg_t), KM_SLEEP);

		count = 0;
		while (server_ord == mp->ms.m_server_ord &&
		    (error = sam_send_mount_cmd(mp, msg, MOUNT_failinit, 2))) {
			if (++count == 5) {
				cmn_err(CE_NOTE,
				    "SAM-QFS: %s: Cannot send the start "
				    "failover message to new server %s, "
				    "retrying (%x)",
				    mp->mt.fi_name, mp->mt.fi_server,
				    mp->mt.fi_status);
			}
#ifdef sun
			if (sam_server_changed(mp)) {
				sam_wake_sharedaemon(mp, EINTR);
			}
#endif
			delay(hz * 2);
		}
		kmem_free(msg, sizeof (sam_san_mount_msg_t));
	}

	if (server_ord != mp->ms.m_server_ord) {
		goto restart;
	}

	/*
	 * Pre-scan the list of inodes with client leases to clear flag.
	 */
	mutex_enter(&mp->mi.m_lease_mutex);

	chain = mp->mi.m_cl_lease_chain.forw;
	while (chain != &mp->mi.m_cl_lease_chain) {
		ip = chain->node;
		chain = chain->forw;

		ip->lease_flags &= ~SAM_LEASE_FLAG_RESET;
	}

	/*
	 * Scan the list of inodes with leases, resetting leases on the new
	 * server.
	 */

	chain = mp->mi.m_cl_lease_chain.forw;
	while (server_ord == mp->ms.m_server_ord &&
	    chain != &mp->mi.m_cl_lease_chain) {
		ip = chain->node;
		chain = chain->forw;

		/* Don't process inodes more than once. */

		if (ip->lease_flags & SAM_LEASE_FLAG_RESET) {
			continue;
		}

		ip->lease_flags |= SAM_LEASE_FLAG_RESET;

		/*
		 * No leases on system inodes (but quotas are like ordinary
		 * files).
		 */

		if ((ip->di.id.ino == SAM_INO_INO) ||
		    (ip->di.id.ino == SAM_ROOT_INO) ||
		    (ip->di.id.ino == SAM_BLK_INO)) {
			continue;
		}

		/*
		 * If the inode is being deleted, ignore it;
		 * otherwise hold it.
		 */

		if (sam_hold_vnode(ip, IG_EXISTS)) {
			continue;
		}

		/*
		 * Acquire the inode's write lock before re-establishing leases.
		 * Must drop the lease lock first.
		 */

		mutex_exit(&mp->mi.m_lease_mutex);
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);

		sam_reestablish_lease(ip);

		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		VN_RELE_OS(ip);

		/*
		 * Restart scanning the chain at the beginning,
		 * since we dropped the lease lock.
		 */

		mutex_enter(&mp->mi.m_lease_mutex);
		chain = mp->mi.m_cl_lease_chain.forw;
	}

	mutex_exit(&mp->mi.m_lease_mutex);

	if (server_ord != mp->ms.m_server_ord) {
		goto restart;
	}

#ifdef sun
	/*
	 * If we have a deferred inactivate (couldn't push inode pages),
	 * finish the inactivate now.
	 */

	mutex_enter(&samgt.ifreelock);

	for (ip = samgt.ifreehead.free.forw;
	    ip != (sam_node_t *)(void *)&samgt.ifreehead;
	    ip = ip->chain.free.forw) {
		if (ip->mp == mp) {
			ip->hash_flags &= ~SAM_HASH_FLAG_INACTIVATE;
		}
	}

	for (ip = samgt.ifreehead.free.forw;
	    server_ord == mp->ms.m_server_ord &&
	    ip != (sam_node_t *)(void *)&samgt.ifreehead;
	    ip = ip->chain.free.forw) {
		if ((ip->mp == mp) &&
		    (!(ip->hash_flags & SAM_HASH_FLAG_INACTIVATE)) &&
		    (ip->flags.bits & SAM_INACTIVATE)) {
			ip->hash_flags |= SAM_HASH_FLAG_INACTIVATE;
			mutex_exit(&samgt.ifreelock);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			ip->flags.bits &= ~SAM_INACTIVATE;
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			VN_RELE_OS(ip);
			mutex_enter(&samgt.ifreelock);
			ip = samgt.ifreehead.free.forw;	/* restart at top */
		}
	}

	mutex_exit(&samgt.ifreelock);
#endif /* sun */

	if (server_ord != mp->ms.m_server_ord) {
		goto restart;
	}

	/*
	 * If client, send MOUNT_resync message to let the server know that
	 * all the leases have been reset. If the server's returned status
	 * indicates the server is still resyncing, wait. Otherwise, go ahead
	 * and finish the failover.
	 * XXX - need to guarantee server does not finish failover until
	 * all clients have checked in.
	 */
	if (SAM_IS_SHARED_CLIENT(mp)) {
		sam_san_mount_msg_t msg;
		int count;

		count = 0;
		while (server_ord == mp->ms.m_server_ord &&
		    (error = sam_send_mount_cmd(mp, &msg, MOUNT_resync, 2))) {
			if (++count > 90) {
				break;
			}
			delay(hz * 2);
		}

		if (server_ord != mp->ms.m_server_ord) {
			goto restart;
		}

		mutex_enter(&mp->ms.m_waitwr_mutex);
		mp->mt.fi_status |= FS_CLNT_DONE;
		mutex_exit(&mp->ms.m_waitwr_mutex);

		if (error) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: Error %d sending resync to new "
			    "server %s,"
			    " thawing (%x)",
			    mp->mt.fi_name, error, mp->mt.fi_server,
			    mp->mt.fi_status);
			sam_failover_done(mp);
		} else {
			cmn_err(CE_NOTE,
			    "SAM-QFS: %s: New server %s (%x) "
			    "responded to resync message,"
			    " thawing (%x)",
			    mp->mt.fi_name, mp->mt.fi_server,
			    msg.call.mount.status,
			    mp->mt.fi_status);

			/*
			 * If the new server hasn't sent MOUNT_faildone, but the
			 * new server is no longer in failover state, go ahead
			 * and complete failover for this client.
			 */
			mutex_enter(&mp->ms.m_waitwr_mutex);
			if ((msg.call.mount.status &
			    (FS_FAILOVER|FS_RESYNCING)) == 0) {
				mp->mt.fi_status |= FS_SRVR_DONE;
			}
			if (!(mp->mt.fi_status & FS_SRVR_DONE)) {
				mutex_exit(&mp->ms.m_waitwr_mutex);
				cmn_err(CE_NOTE,
				    "SAM-QFS: %s: Waiting for new "
				    "server %s (%x): thawing (%x)",
				    mp->mt.fi_name, mp->mt.fi_server,
				    msg.call.mount.status,
				    mp->mt.fi_status);
			} else {
				mutex_exit(&mp->ms.m_waitwr_mutex);
				sam_failover_done(mp);
			}
		}
	} else {
		mutex_enter(&mp->ms.m_waitwr_mutex);
		mp->mt.fi_status |= FS_CLNT_DONE;
		mp->mt.fi_status &= ~(FS_THAWING);
		mutex_exit(&mp->ms.m_waitwr_mutex);
	}

	/* Let the scheduler know that we're done. */

	mutex_enter(&samgt.schedule_mutex);
	mp->mi.m_schedule_flags &= ~SAM_SCHEDULE_TASK_THAWING;
	sam_taskq_remove(entry);
	TRACE(T_SAM_CL_REEST_RET, mp, 0, mp->mt.fi_status, mp->mi.m_inode.flag);
}

#ifdef sun
void
sam_reestablish_leases(sam_schedule_entry_t *entry)
{
	__sam_reestablish_leases(entry);
}
#endif /* sun */

#ifdef linux
int
sam_reestablish_leases(void *arg)
{
	sam_schedule_entry_t *entry;

	entry = task_begin(arg);
	__sam_reestablish_leases(entry);
	return (0);
}
#endif /* linux */


/*
 * -----	sam_reestablish_lease - reestablish the lease for this inode.
 * This is issued during failover to reestablish leases.
 */

static void
sam_reestablish_lease(sam_node_t *ip)
{
	sam_mount_t *mp = ip->mp;
	int ltype;
	int error;
	ushort_t leases;
	sam_san_lease_msg_t *msg;

	/*
	 * Issue one LEASE_reset for all leases except frlock
	 * (not really a lease).
	 * Then issue a LEASE_reset for the frlock.
	 * XXX - change frlock to INODE_frlock. It is not really a lease.
	 * NOTE: Cannot issue any commands but MOUNT and LEASE_reset while
	 * the server is in the RESYNCING state or these commands will hang.
	 */
	leases = ip->cl_leases;
	if (ip->cl_saved_leases & CL_APPEND) {
		leases |= CL_APPEND;
	}
	if (leases) {
		msg = kmem_alloc(sizeof (*msg), KM_SLEEP);
		error = 0;
		if (leases & ~CL_FRLOCK) {
			ushort_t actions = 0;

			bzero((void *)msg, sizeof (*msg));
			sam_build_header(ip->mp, &msg->hdr, SAM_CMD_LEASE,
			    SHARE_wait, LEASE_reset, sizeof (sam_san_lease_t),
			    sizeof (sam_san_lease2_t));
			msg->call.lease.data.alloc_unit = ip->cl_alloc_unit;
			/* lease mask */
			msg->call.lease.data.ltype = (leases & ~CL_FRLOCK);
			msg->call.lease.data.copy = ip->copy;

			/*
			 * If we are re-establishing an append lease, we were
			 * the owner of the file's size. Set actions to set the
			 * size on the new server. If we're now on the server,
			 * we must ensure that the size is updated.
			 */
			if (leases & CL_APPEND) {
				actions = SR_SET_SIZE;
				if (SAM_IS_SHARED_SERVER(ip->mp)) {
					ip->di.rm.size = ip->size;
					sam_set_size(ip);
					ip->flags.b.changed = 1;
				}
			}
			if ((error = sam_issue_lease_request(ip, msg,
			    SHARE_wait, actions, NULL)) == 0) {
				for (ltype = LTYPE_read;
				    ltype < SAM_MAX_LTYPE; ltype++) {
					if (leases & (1 << ltype)) {
						ip->cl_leasegen[ltype] =
						    msg->call.lease.gen[ltype];
					}
				}
				ip->cl_saved_leases = 0;
			} else {
				if (error == ENOENT) {
					if (ip->cl_leases) {
						sam_client_remove_leases(ip,
						    (leases & ~CL_FRLOCK), 0);
					}
				} else {
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: lease reset "
					    "err=%d: ino=%d.%d lmask=%x,"
					    " allocsz=%llx, size=%llx",
					    mp->mt.fi_name, error,
					    ip->di.id.ino, ip->di.id.gen,
					    (leases & ~CL_FRLOCK),
					    ip->cl_allocsz, ip->di.rm.size);
				}
			}
		}
		if ((error == 0) && (leases & CL_FRLOCK)) {
			sam_cl_flock_t *fptr, *fnext, *fprev;

			bzero((void *)msg, sizeof (*msg));
			sam_build_header(ip->mp, &msg->hdr, SAM_CMD_LEASE,
			    SHARE_wait, LEASE_reset, sizeof (sam_san_lease_t),
			    sizeof (sam_san_lease2_t));
			msg->call.lease.data.alloc_unit = ip->cl_alloc_unit;
			msg->call.lease.data.ltype = CL_FRLOCK;

			fptr = ip->cl_flock;
			fprev = NULL;
			while (fptr != NULL) {
				fnext = fptr->cl_next;

				/*
				 * Reset nfs frlocks only if client is not a
				 * cluster member.  Otherwise, remove frlock and
				 * if none left, remove frlock lease.
				 */
				if (!fptr->nfs_lock ||
				    !SAM_MP_IS_CLUSTERED(mp)) {
					msg->call.lease.data.offset =
					    fptr->offset;
					msg->call.lease.data.filemode =
					    fptr->filemode;
					msg->call.lease.data.cmd = fptr->cmd;
					msg->call.lease.flock = fptr->flock;
					if ((error =
					    sam_issue_lease_request(ip, msg,
					    SHARE_wait, 0, NULL))) {
						cmn_err(CE_WARN,
						    "SAM-QFS: %s: flock "
						    "reset err=%d: ino=%d.%d"
						    " pid=%d cmd=%d "
						    "l_start=%llx l_len=%llx\n",
						    mp->mt.fi_name, error,
						    ip->di.id.ino,
						    ip->di.id.gen,
						    fptr->flock.l_pid,
						    fptr->cmd,
						    (unsigned long long)
						    fptr->flock.l_start,
						    (unsigned long long)
						    fptr->flock.l_len);
					}
					fprev = fptr;
				} else {
					kmem_free(fptr,
					    sizeof (sam_cl_flock_t));
					if (fprev == NULL) {
						ip->cl_flock = fnext;
					} else {
						fprev->cl_next = fnext;
					}
					if ((--ip->cl_locks) < 0) {
						ip->cl_locks = 0;
					}
				}
				fptr = fnext;
			}
			if (ip->cl_flock == NULL) {
				ip->cl_locks = 0;
				sam_client_remove_leases(ip, CL_FRLOCK, 0);
			}
		}
		kmem_free(msg, sizeof (*msg));
	}
}
