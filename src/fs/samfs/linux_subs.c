/*
 * ----- linux_subs.c - Implement misc. functions not provided by Linux.
 *
 *  Implement functions need for kernel condition variables.
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
 * or https://illumos.org/license/CDDL.
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
#pragma ident "$Revision: 1.19 $"
#endif

#include "sam/osversion.h"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/unistd.h>

#include <linux/fs.h>

#include <asm/system.h>
#include <asm/uaccess.h>

#include "sam/linux_types.h"
#include "macros_linux.h"


void QFS_schedule(void);

/*
 * Wait for a cv_signal() on the specified wait_queue.
 */

static inline void
__cv_wait(wait_queue_head_t *wq, kmutex_t *wl, int state)
{
	DECLARE_WAITQUEUE(wait, current);

	set_current_state(state);

	add_wait_queue_exclusive(wq, &wait);

	mutex_exit(wl);

	QFS_schedule();

	mutex_enter(wl);

	remove_wait_queue(wq, &wait);
	__set_current_state(TASK_RUNNING);
}


void
cv_wait(wait_queue_head_t *wq, kmutex_t *wl)
{
	__cv_wait(wq, wl, TASK_UNINTERRUPTIBLE);
}

/*
 * For daemon threads which don't want to be interpreted by the scheduler
 * as interactive tasks.
 */
void
cv_wait_light(wait_queue_head_t *wq, kmutex_t *wl)
{
	__cv_wait(wq, wl, TASK_INTERRUPTIBLE);
}



/*
 * Wait until wakeup_time for a cv_signal()
 *  on the specified wait_queue
 */
static inline void
__cv_timedwait(wait_queue_head_t *wq, kmutex_t *wl, long wakeup_time,
    int state)
{
	long interval = wakeup_time - *QFS_jiffies;
	DECLARE_WAITQUEUE(wait, current);

	set_current_state(state);

	add_wait_queue_exclusive(wq, &wait);

	if (interval > 0) {
		mutex_exit(wl);
		interval = schedule_timeout(interval);
		mutex_enter(wl);
	}

	__set_current_state(TASK_RUNNING);
	remove_wait_queue(wq, &wait);
}


void
cv_timedwait(wait_queue_head_t *wq, kmutex_t *wl, long wakeup_time)
{
	__cv_timedwait(wq, wl, wakeup_time, TASK_UNINTERRUPTIBLE);
}

/*
 * For daemon threads which don't want to be interpreted by the scheduler
 * as interactive tasks.
 */
void
cv_timedwait_light(wait_queue_head_t *wq, kmutex_t *wl, long wakeup_time)
{
	__cv_timedwait(wq, wl, wakeup_time, TASK_INTERRUPTIBLE);
}


long
cv_timedwait_sig(wait_queue_head_t *wq, kmutex_t *wl, long wakeup_time)
{
	long interval = wakeup_time - *QFS_jiffies;
	long ret = -1;
	DECLARE_WAITQUEUE(wait, current);

	add_wait_queue_exclusive(wq, &wait);

	for (;;) {

		set_current_state(TASK_INTERRUPTIBLE);

		if (interval > 0) {
			mutex_exit(wl);
			interval = schedule_timeout(interval);
			mutex_enter(wl);
		}

		if (signal_pending(current)) {
			/*
			 * Interrupted
			 */
			ret = 0;
			break;
		}
		if (interval == 0) {
			/*
			 * Timedout
			 */
			ret = -1;

		} else {
			/*
			 * Condition signaled
			 */
			ret = interval;
		}
		break;
	}

	__set_current_state(TASK_RUNNING);
	remove_wait_queue(wq, &wait);

	return (ret);
}


long
cv_wait_sig(kcondvar_t *wq, kmutex_t *wl)
{
	long ret = 1;
	DECLARE_WAITQUEUE(wait, current);

	add_wait_queue_exclusive((wait_queue_head_t *)wq, &wait);

	for (;;) {

		set_current_state(TASK_INTERRUPTIBLE);

		mutex_exit(wl);

		QFS_schedule();

		mutex_enter(wl);

		if (signal_pending(current)) {
			/*
			 * Interrupted
			 */
			ret = 0;

		} else {
			/*
			 * Condition signaled
			 */
			ret = 1;
		}
		break;
	}

	__set_current_state(TASK_RUNNING);
	remove_wait_queue((wait_queue_head_t *)wq, &wait);

	return (ret);
}
