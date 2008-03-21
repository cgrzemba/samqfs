/*
 * ----- thread.c - Process the init/kill thread functions.
 *
 *	Processes the SAM-QFS init and kill thread functions.
 *  Contains task queue management routines.
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
#pragma ident "$Revision: 1.71 $"
#endif

#include "sam/osversion.h"

/*
 * ----- UNIX Includes
 */

#ifdef sun
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/param.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/cred.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <sys/buf.h>
#include <sys/pathname.h>
#include <sys/file.h>
#include <sys/ddi.h>
#include <sys/mutex.h>
#include <sys/conf.h>
#include <sys/disp.h>
#include <sys/mount.h>
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

#include <asm/param.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#include <asm/statfs.h>
#endif
#endif	/* linux */

/*
 * ----- SAMFS Includes
 */

#include "sam/types.h"
#include "sam/param.h"
#include "sam/mount.h"

#include "sblk.h"
#include "inode.h"
#include "mount.h"
#include "debug.h"
#ifdef sun
#include "extern.h"
#include "trace.h"
#include "kstats.h"
#endif /* sun */
#ifdef linux
#include "clextern.h"
#include "trace.h"
#include "kstats_linux.h"
#endif /* linux */

#ifdef sun
static void sam_taskq_trampoline(void *arg);
#endif /* sun */
static void sam_taskq_uncount_internal(sam_mount_t *mp);

#ifdef linux
extern void QFS_wait_event(wait_queue_head_t wq, void *condition);
#endif

#ifdef sun
/*
 * ----- sam_start_threads - initialize the file system threads.
 * File system must be mounted, mounting.
 */

int					/* ERRNO if error, 0 if successful. */
sam_start_threads(sam_mount_t *mp)
{
	int error;

	mutex_enter(&mp->ms.m_synclock);
	if ((error = sam_init_block(mp))) {
		mutex_exit(&mp->ms.m_synclock);
		return (error);
	}
	if ((error = sam_init_inode(mp))) {
		mutex_exit(&mp->ms.m_synclock);
		return (error);
	}
	mutex_exit(&mp->ms.m_synclock);
	return (0);
}
#endif /* sun */


#ifdef sun
/*
 * ----- sam_stop_threads - kill the file system threads if they exist.
 * File system must be mounted, mounting, or umounting.
 */

void
sam_stop_threads(sam_mount_t *mp)
{
	sam_kill_thread(&mp->mi.m_inode);
	sam_kill_thread(&mp->mi.m_block);
}
#endif /* sun */


/*
 * ----- sam_init_thread - initialize a thread.
 * Create this thread and wait until it is alive.
 * Try 10 times at a 1 second interval to create the thread.
 * tp->mutex is set at entry and exit.
 */

int				/* ERRNO if error, 0 if successful. */
sam_init_thread(
	sam_thr_t *tp,		/* The pointer to the thread table */
#ifdef sun
	void (*fct)(),		/* The pointer to the thread entry point */
#endif /* sun */
#ifdef linux
	int (*fct)(void *),
#endif /* linux */
	void *data)		/* The pointer to the thread data */
{
	int count;
	int ret = 0;
#ifdef linux
	pid_t	pid;
	unsigned long kflags = 0;
#endif /* linux */

	tp->flag = SAM_THR_MOUNT;
	tp->state = SAM_THR_INIT;
	tp->wait = 1;
	count = 0;
	for (;;) {
#ifdef sun
		if ((thread_create(NULL, 0, fct, (caddr_t)data, 0, &p0, TS_RUN,
		    maxclsyspri)) == NULL) {
			delay(hz);
			if (++count > 10) {
				tp->flag = SAM_THR_NULL;
				return (ENOMEM);
			}
		} else {
			break;
		}
#endif /* sun */
#ifdef linux
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
		kflags = CLONE_VM | CLONE_FS | CLONE_FILES;
#endif
		pid = kernel_thread(fct, data, kflags);
		if (pid < 0) {
			delay(hz);
			if (++count > 10) {
				return (-ENOMEM);
			}
		} else {
			ret = pid;
			break;
		}
#endif /* linux */
	}
	while (tp->state != SAM_THR_EXIST) {
		cv_wait(&tp->get_cv, &tp->mutex);
	}
	tp->wait--;
	return (ret);
}


/*
 * -----	sam_kill_thread - terminate a thread.
 * Kill this thread if it is alive and wait until it is dead.
 */

void
sam_kill_thread(sam_thr_t *tp)
{
	mutex_enter(&tp->mutex);
	if (tp->state == SAM_THR_EXIST) {
		tp->flag = SAM_THR_UMOUNT;
		while (tp->state != SAM_THR_DEAD) {
			mutex_enter(&tp->put_mutex);
			cv_signal(&tp->put_cv);
			mutex_exit(&tp->put_mutex);
			cv_wait(&tp->get_cv, &tp->mutex);
		}
		tp->flag = SAM_THR_NULL;
	}
	mutex_exit(&tp->mutex);
}


/*
 * -----	sam_kill_threads - terminate threads.
 * Kill all the threads started by this thread struct.
 * NOTE: tp->mutex set on entry and exit.
 */

void
sam_kill_threads(sam_thr_t *tp)
{
	if (tp->no_threads != 0) {
		tp->flag = SAM_THR_UMOUNT;
		while (tp->no_threads != 0) {
			mutex_enter(&tp->put_mutex);
			cv_broadcast(&tp->put_cv);
			mutex_exit(&tp->put_mutex);

			cv_wait(&tp->get_cv, &tp->mutex);
		}
		tp->flag = SAM_THR_NULL;
	}
}


/*
 * ----- sam_setup_threads - setup threads.
 */

#ifdef sun
void
sam_setup_threads(sam_mount_t *mp)
{
	sam_setup_thread(&mp->mi.m_block);
	sam_setup_thread(&mp->mi.m_inode);
}
#endif /* sun */


#ifdef linux
void
sam_setup_threads(sam_mount_t *mp)
{
	sam_setup_thread(&mp->mi.m_inode);
	sam_setup_thread(&mp->mi.m_statfs);
}
#endif /* linux */


/*
 * ----- sam_setup_thread - setup a thread.
 */

void
sam_setup_thread(sam_thr_t *tp)
{
	sam_mutex_init(&tp->mutex, NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&tp->put_mutex, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&tp->get_cv, NULL, CV_DEFAULT, NULL);
	cv_init(&tp->put_cv, NULL, CV_DEFAULT, NULL);
	mutex_enter(&tp->mutex);
	tp->state = SAM_THR_INIT;
	tp->flag = SAM_THR_NULL;
	tp->queue.items = 0;
	tp->queue.wmark = 0;
	tp->queue.current_item = 0;
	tp->queue.next_item = 0;
	mutex_exit(&tp->mutex);
}


/*
 * Scheduled task management code for SAM-QFS.
 *
 * This module implements a queue of tasks to be run asynchronously at
 * some future time.  A task can also be started immediately.  The
 * schedule normally contains only one entry for each task (but this
 * implementation does produce duplicates in certain circumstances).
 *
 * Current tasks using this module include reclaiming blocks, expiring
 * shared file system leases, and deferred inactivation of inodes.
 */

#define	SAM_TASKQ_RETRY_TICKS 5

/*
 * ----- sam_taskq_init
 * Initialize the SAM task queue.  We use the system's default task queue
 * as our underlying execution engine.  This task queue currently allows
 * at least 64 threads per CPU; we're unlikely to overburden it.
 */

void
sam_taskq_init(void)
{
	sam_mutex_init(&samgt.schedule_mutex, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&samgt.schedule_cv, NULL, CV_DEFAULT, NULL);
	samgt.schedule.forw = samgt.schedule.back =
	    (sam_schedule_entry_t *)(void *)&samgt.schedule;
	samgt.schedule_count = 0;
}


/*
 * ----- sam_taskq_destroy
 * Destroy the SAM task queue.  Don't return until all tasks in progress
 * have completed.  Prevent new tasks from running.
 */

void
sam_taskq_destroy(void)
{
	mutex_enter(&samgt.schedule_mutex);
	samgt.schedule_flags = SAM_SCHEDULE_EXIT;

#ifdef	sun
	/*
	 * Try to cancel all tasks on the queue.  There is an inherent race
	 * condition here, since the tasks may be running and removing their
	 * entries from the queue.  That's OK, as long as we are careful not
	 * to touch potentially-disposed memory.  We can tell whether we did
	 * cancel a task by examining the return value from untimeout().
	 *
	 * Because the function called via timeout() simply starts the task,
	 * which then runs independently, the task may still be running after
	 * a failed attempt to cancel untimeout().  To handle these tasks, we
	 * unlink their entries from the queue, and wait for them to finish
	 * using the schedule_cv condition variable.  Note that we must unlink
	 * their entries from the queue before calling untimeout, to avoid a
	 * race in which the task completes during the call and frees its entry.
	 *
	 * To make things just a little more confusing, we also start some
	 * tasks without using the timeout mechanism.  To allow us to tell
	 * whether those tasks are running, we keep them in the queue as
	 * well.  These tasks are distinguished by a timeout ID of zero.
	 * We just unlink these tasks from the queue when we find them.
	 */

	while (samgt.schedule.forw !=
	    (sam_schedule_entry_t *)(void *)&samgt.schedule) {
		sam_schedule_entry_t *entry;
		timeout_id_t id;
		clock_t remaining;

		entry = samgt.schedule.forw;
		id = entry->id;

		/* Remove task from the queue. */

		entry->queue.back->queue.forw = entry->queue.forw;
		entry->queue.forw->queue.back = entry->queue.back;
		entry->queue.forw = entry->queue.back = entry;

		if (id != 0) {	/* task was started/scheduled via timeout */
			mutex_exit(&samgt.schedule_mutex);

			/*
			 * OK.  We've released the mutex and can safely try to
			 * cancel this task.  untimeout() will block if the task
			 * is running until it completes.  (Astute readers may
			 * note that there is a race with reuse of timeout IDs;
			 * Solaris takes responsibility for managing this
			 * problem.)
			 */

			remaining = untimeout(id);

			/*
			 * Either there was time remaining (0 or more ticks),
			 * and we successfully cancelled the task; or the task
			 * has begun, and will delete its entry once it
			 * completes (which may have happened already, since we
			 * dropped the mutex).
			 */

			if (remaining >= 0) {
				kmem_free(entry, sizeof (sam_schedule_entry_t));
			}

			mutex_enter(&samgt.schedule_mutex);

			if (remaining >= 0) {
				samgt.schedule_count--;
				/*
				 * Note: We don't maintain m_schedule_count
				 * here...
				 */
			}
		}
	}

	/*
	 * OK.  All tasks have been removed from the schedule, and future
	 * tasks cancelled.  We may still have running tasks; wait for
	 * them to complete.
	 */

	while (samgt.schedule_count > 0) {
		cv_wait(&samgt.schedule_cv, &samgt.schedule_mutex);
	}

	/*
	 * All done.
	 */

	mutex_exit(&samgt.schedule_mutex);

#endif	/* sun */
	mutex_destroy(&samgt.schedule_mutex);
	cv_destroy(&samgt.schedule_cv);
}


/*
 * ----- sam_taskq_start_mp
 * Enable scheduled tasks for the specified mount point.
 */

void
sam_taskq_start_mp(sam_mount_t *mp)
{
	if (mp->mi.m_schedule_flags & SAM_SCHEDULE_EXIT) {
		/*
		 * No tasks can be running
		 */
		ASSERT(mp->mi.m_schedule_count == 0);

		mutex_enter(&samgt.schedule_mutex);
		mp->mi.m_schedule_flags &= ~SAM_SCHEDULE_EXIT;
		mutex_exit(&samgt.schedule_mutex);
	}
}


/*
 * ----- sam_taskq_stop_mp
 * Remove any scheduled tasks for the specified mount point.  Wait for
 * tasks in progress to complete.  This is similar to the preceding
 * function, but the implementation is somewhat different, as we need
 * to account for other tasks which may be added to the queue (for
 * other mount points).  See the comments above for some other details.
 */

void
sam_taskq_stop_mp(sam_mount_t *mp)
{
	sam_schedule_entry_t *entry;

	mutex_enter(&samgt.schedule_mutex);
	mp->mi.m_schedule_flags |= SAM_SCHEDULE_EXIT;

	/*
	 * Try to remove all tasks for this mount point from the queue.
	 * Deal with the race condition of tasks trying to remove themselves,
	 * as well as new tasks (for other mount points) being inserted.
	 */

	entry = samgt.schedule.forw;
	while (entry != (sam_schedule_entry_t *)(void *)&samgt.schedule) {
		sam_schedule_entry_t *next;
#ifdef sun
		timeout_id_t id;
		clock_t remaining;
#endif /* sun */
#ifdef linux
		pid_t id;
#endif /* linux */

		/* If the task doesn't belong to this mount point, ignore it. */

		if (entry->mp != mp) {
			entry = entry->queue.forw;
			continue;
		}
#ifdef linux
		if (entry->signaled) {
			entry = entry->queue.forw;
			continue;
		}
#endif /* linux */

		id = entry->id;

#ifdef sun
		/* Remove task from the queue, saving the forward pointer. */

		next = entry->queue.forw;
		entry->queue.back->queue.forw = entry->queue.forw;
		entry->queue.forw->queue.back = entry->queue.back;
		entry->queue.forw = entry->queue.back = entry;

		if (id == 0) {
			entry = next;
		} else {
			mutex_exit(&samgt.schedule_mutex);

			/* Try to cancel this task. */

			remaining = untimeout(id);

			/* If we cancelled it, free its entry. */

			if (remaining >= 0) {
				kmem_free(entry, sizeof (sam_schedule_entry_t));
			}

			mutex_enter(&samgt.schedule_mutex);

			/* If we cancelled it, need to adjust count of tasks. */

			if (remaining >= 0) {
				samgt.schedule_count--;
				mp->mi.m_schedule_count--;
			}

			/*
			 * Restart at the beginning of the queue
			 * since we dropped the mutex.
			 */
			entry = samgt.schedule.forw;
		}
#endif /* sun */
#ifdef linux
		/*
		 * Signal the task to wakeup, run
		 * and clean itself up.
		 */
		next = entry->queue.forw;
		entry->signaled = 1;
		QFS_wait_event(entry->wq, entry->task);
		wake_up_process(entry->task);
		entry = next;
#endif /* linux */
	}

	/* Wait for tasks run directly to complete. */

	while (mp->mi.m_schedule_count > 0) {
		cv_wait(&samgt.schedule_cv, &samgt.schedule_mutex);
	}

	/* All done. */

	mutex_exit(&samgt.schedule_mutex);
}


/*
 * ----- sam_taskq_add
 * Add a task to the schedule, if it isn't already there.  If it's
 * scheduled to run immediately, start it; otherwise, set a timer.
 *
 * Because we can't safely cancel a task scheduled for the future, we
 * go ahead and add the task if the time desired for the new task is
 * earlier than an existing instance of the task.  This does mean we
 * may have duplicates in the schedule.  Tasks which can't tolerate
 * running more than one instance simultaneously must prevent this
 * themselves.
 *
 * In some cases, there are advantages to this approach.  If a task is
 * falling so far behind in its work that the time for a second scheduled
 * instance is reached, we may be able to split the work across two or
 * more threads (potentially even across multiple CPUs).  (Perhaps this
 * indicates that we should be willing to schedule future instances even
 * if there is an existing instance, at least if they're far enough apart
 * in time.)
 */

void
sam_taskq_add(
	sam_task_func_t func,	/* Function to call */
	sam_mount_t *mp,	/* Mount point to associate task with */
	void *arg,		/* Argument for task */
	clock_t ticks)		/* Num ticks to wait before initiating task */
{
	sam_schedule_entry_t *entry;
	int64_t start;
#ifdef linux
	pid_t pid;
	unsigned long kflags = 0;
#endif /* linux */

	if (ticks < 0) {
		ticks = 0;
	}

	start = lbolt + ticks;		/* Start time for this task */

	SAM_COUNT64(thread, taskq_add);

	mutex_enter(&samgt.schedule_mutex);

	/*
	 * If we're shutting down the scheduler, don't add new tasks.
	 */

	if ((samgt.schedule_flags & SAM_SCHEDULE_EXIT) ||
	    ((mp != NULL) && (mp->mi.m_schedule_flags & SAM_SCHEDULE_EXIT))) {
		mutex_exit(&samgt.schedule_mutex);
		return;
	}

	/*
	 * Look for an existing instance of the task.  If we find one, check
	 * whether it's scheduled to start before this new instance.  If so,
	 * just return.  If not, we'll ignore it (leaving it scheduled).
	 *
	 * We could do better here if we kept the list sorted, but I don't
	 * think it's likely to grow very long, so the added complexity may
	 * not be worth it.
	 */

	entry = samgt.schedule.forw;
	while (entry != (sam_schedule_entry_t *)(void *)&samgt.schedule) {
		if ((entry->mp == mp) && (entry->func == func)) {
			if (entry->start <= start) {
				mutex_exit(&samgt.schedule_mutex);
				SAM_COUNT64(thread, taskq_add_dup);
				return;
			}
		}
		entry = entry->queue.forw;
	}

	mutex_exit(&samgt.schedule_mutex);

	/*
	 * OK, we're going to schedule a new instance.  Allocate space for it.
	 * Remember that the task can potentially start running immediately,
	 * and will free its entry when complete; hence we must link the entry
	 * onto the schedule before calling timeout.  Since we can't allow an
	 * entry on the schedule with an invalid id, we must hold the schedule
	 * mutex across the timeout() call.
	 */

	entry = kmem_alloc(sizeof (sam_schedule_entry_t), KM_SLEEP);
	sam_init_schedule_entry(entry, mp, func, arg);
	entry->start = start;

	mutex_enter(&samgt.schedule_mutex);

	samgt.schedule.back->queue.forw = entry;
	entry->queue.forw = (sam_schedule_entry_t *)(void *)&samgt.schedule;
	entry->queue.back = samgt.schedule.back;
	samgt.schedule.back = entry;

#ifdef sun
	if (ticks != 0) {
		entry->id = timeout(sam_taskq_trampoline, entry, ticks);
	} else {
		entry->id = 0;
		(void) taskq_dispatch(system_taskq, (task_func_t *)func, entry,
		    TQ_SLEEP);
		SAM_COUNT64(thread, taskq_add_immed);
	}
#endif /* sun */
#ifdef linux
	entry->ticks = ticks;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	kflags = CLONE_VM | CLONE_FS | CLONE_FILES;
#endif
	pid = kernel_thread(func, entry, kflags);
	ASSERT(pid >= 0);

#endif /* linux */

	samgt.schedule_count++;
	if (mp != NULL) {
		mp->mi.m_schedule_count++;
	}

	mutex_exit(&samgt.schedule_mutex);
}


/*
 * ----- sam_taskq_trampoline
 * This function is called by the timeout mechanism, and schedules a
 * the desired function to be run on the system task queue.
 *
 * We don't call the function directly because we don't want to tie up
 * a callout thread for a potentially long period of time, and because
 * callout threads may run in interrupt context.
 *
 * If we can't dispatch the task to run, we've got a problem.  Luckily
 * we can try again later.
 *
 * Yes, this does add a context switch to the code path.
 */

#ifdef sun
static void
sam_taskq_trampoline(void *arg)
{
	sam_schedule_entry_t *entry;
	taskqid_t id;

	entry = (sam_schedule_entry_t *)arg;

	/* Try to start this task. */

	id = taskq_dispatch(system_taskq, (task_func_t *)entry->func, arg,
	    TQ_NOSLEEP);

	/*
	 * If we fail, we'll schedule another attempt to happen soon.
	 * Update the ID in our schedule entry.  This does introduce
	 * the possibility that we'll fail to cancel a task which we
	 * could have, since the cancellation code may see our (running)
	 * ID and not the new ID.  In this case we'll just wait for the
	 * task to finish before the cancellation procedure returns.
	 */

	if (id == 0) {
		timeout_id_t timer_id;

		timer_id = timeout(sam_taskq_trampoline, entry,
		    SAM_TASKQ_RETRY_TICKS);
		mutex_enter(&samgt.schedule_mutex);
		entry->start = lbolt + SAM_TASKQ_RETRY_TICKS;
		entry->id = timer_id;
		mutex_exit(&samgt.schedule_mutex);
		SAM_COUNT64(thread, taskq_dispatch_fail);
	}
}
#endif /* sun */


/*
 * ----- sam_taskq_remove
 * Remove a task from the queue.  This should be called only by the
 * running task itself.
 *
 * Since we don't schedule a new task if an existing task is running,
 * instead relying on tasks not to exit if they have more work to do,
 * there is a race condition where the existing task might exit without
 * seeing the new work.  The schedule mutex must be used to avoid this
 * problem, and tasks are responsible for acquiring the mutex before
 * calling this routine.  This routine will release the mutex to avoid
 * holding it across the kmem_free call, hence tasks should not attempt
 * to release the mutex.
 */

void
sam_taskq_remove(sam_schedule_entry_t *entry)
{
	ASSERT(MUTEX_HELD(&samgt.schedule_mutex));

	entry->queue.back->queue.forw = entry->queue.forw;
	entry->queue.forw->queue.back = entry->queue.back;

	sam_taskq_uncount_internal(entry->mp);

	mutex_exit(&samgt.schedule_mutex);

	kmem_free(entry, sizeof (sam_schedule_entry_t));
}


/*
 * ----- sam_taskq_remove_nocount
 * Remove a task from the queue, without decrementing the running task
 * count.  This should be used if a task is going to remove itself from
 * the queue before its last use of the file system / mount point.  In
 * this case it must use sam_taskq_uncount before returning.
 */

void
sam_taskq_remove_nocount(sam_schedule_entry_t *entry)
{
	ASSERT(MUTEX_HELD(&samgt.schedule_mutex));

	entry->queue.back->queue.forw = entry->queue.forw;
	entry->queue.forw->queue.back = entry->queue.back;

	mutex_exit(&samgt.schedule_mutex);

	kmem_free(entry, sizeof (sam_schedule_entry_t));
}


/*
 * ----- sam_taskq_uncount
 * Decrement the count of running tasks.  Must be used ONLY after a call
 * to sam_taskq_remove_nocount.
 */

void
sam_taskq_uncount(sam_mount_t *mp)
{
	mutex_enter(&samgt.schedule_mutex);

	sam_taskq_uncount_internal(mp);

	mutex_exit(&samgt.schedule_mutex);
}


/*
 * ----- sam_taskq_uncount_internal
 * Decrement the count of running tasks.  Called from sam_taskq_uncount or
 * sam_taskq_remove.
 */

static void
sam_taskq_uncount_internal(sam_mount_t *mp)
{
	uint32_t flags;

	flags = 0;

	samgt.schedule_count--;
	if (samgt.schedule_count == 0) {
		flags = samgt.schedule_flags;
	}

	if (mp != NULL) {
		mp->mi.m_schedule_count--;
		if (mp->mi.m_schedule_count == 0) {
			flags |= mp->mi.m_schedule_flags;
		}
	}

	if (flags & SAM_SCHEDULE_EXIT) {
		cv_broadcast(&samgt.schedule_cv);
	}
}


/*
 * ----- sam_taskq_dispatch
 * Dispatch a thread. The dispatched thread executes and then terminates.
 */

int
sam_taskq_dispatch(sam_schedule_entry_t *entry)
{
	sam_mount_t *mp;
	int success;
#ifdef linux
	pid_t pid;
	unsigned long kflags = 0;
#endif /* linux */

	SAM_COUNT64(thread, taskq_dispatch);
	mp = entry->mp;

	mutex_enter(&samgt.schedule_mutex);
	samgt.schedule_count++;
	if (mp != NULL) {
		mp->mi.m_schedule_count++;
	}
	mutex_exit(&samgt.schedule_mutex);

#ifdef sun
	success = taskq_dispatch(system_taskq, (task_func_t *)entry->func,
	    (void *)entry, TQ_SLEEP);
#endif /* sun */

#ifdef linux
	entry->ticks = 0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	kflags = CLONE_VM | CLONE_FS | CLONE_FILES;
#endif
	pid = kernel_thread(entry->func, entry, kflags);

	success = pid >= 0;
#endif /* linux */

	if (!success) {
		mutex_enter(&samgt.schedule_mutex);
		sam_taskq_uncount_internal(mp);
		mutex_exit(&samgt.schedule_mutex);
	}

	return (success);
}


#ifdef linux
/*
 * Taskq functions on Linux are actually processes.
 *
 * This routine does various things when starting
 * a taskq function.
 *
 * Disconnects the task from its original parent and
 * makes it a child of init.
 *
 * Does the delay if requested.
 *
 * Changes its name to "taskq".
 *
 */
sam_schedule_entry_t *
task_begin(void *arg)
{
	long timeout;
	sam_schedule_entry_t *entry;

	SAM_DAEMONIZE("taskq");

	entry = (sam_schedule_entry_t *)arg;

	entry->id = current->pid;
	set_current_state(TASK_INTERRUPTIBLE);
	entry->task = current;
	wmb();
	wake_up(&entry->wq);
	timeout = entry->ticks;
	if (timeout > 0) {
		schedule_timeout(timeout);
	}
	set_current_state(TASK_RUNNING);
	return (entry);
}
#endif /* linux */

void
sam_init_schedule_entry(
	sam_schedule_entry_t *ep,	/* Scheduler entry to initialize */
	struct sam_mount *mp,		/* Which mount point */
	sam_task_func_t func,		/* Function to invoke when scheduled */
	void *arg)			/* Arg to func */
{
	ep->mp = mp;
	ep->func = func;
	ep->arg = arg;

#ifdef linux
	ep->ticks = 0;
	ep->signaled = 0;
	ep->id = 0;
	ep->task = NULL;
	init_waitqueue_head(&ep->wq);
#endif /* linux */
}
