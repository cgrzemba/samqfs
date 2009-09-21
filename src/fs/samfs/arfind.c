/*
 * ----- arfind.c - Process the file change notification for arfind.
 *
 *	Process the file change notification for arfind.
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

#pragma ident "$Revision: 1.30 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/ksynch.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/cpuvar.h>
#include <sys/thread.h>
#include <sys/policy.h>

/* ----- SAMFS Includes */

#include "sam/types.h"

#include "inode.h"
#include "mount.h"
#include "extern.h"
#define	ARFIND_PRIVATE
#include "arfind.h"
#undef ARFIND_PRIVATE
#include "trace.h"


/*
 * ---- sam_arfind_buf - arfind file event buffer.
 *
 * This buffer is allocated and the pointer to it is placed in the mounted
 * file system mount instance.
 */
struct sam_arfind_buf {
	kmutex_t ab_bufmutex;	/* Data mutex */
	kcondvar_t ab_waitcv;	/* Waiting for events */
	ATOM_INT_T ab_in;
	ATOM_INT_T ab_out;
	boolean_t ab_hwm_sent;	/* To send AE_hwm once per arfind call */
	int ab_overflow;	/* Buffer overflowed */
	int ab_umount;
	int ab_busy;		/* buffer in use */
	struct arfind_event ab_entry[ARFIND_EVENT_MAX];
};


/*
 * ----- sam_arfind_call - Process the arfind daemon command.
 *
 * arfind daemon request returns a buffer of file system actions.
 */

int					/* ERRNO if error, 0 if successful. */
sam_arfind_call(
	void *arg,			/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	struct sam_arfind_buf *ab;
	sam_mount_t *mp;
	struct sam_arfind_arg af;
	uchar_t	*buf;
	int		error;

	/*
	 * Read arguments, find the filesystem, and check permission.
	 */
	if (size != sizeof (af) || copyin(arg, &af, sizeof (af))) {
		return (EFAULT);
	}
	if ((mp = sam_find_filesystem(af.AfFsname)) == NULL) {
		return (ENOENT);
	}
	error = secpolicy_fs_config(credp, mp->mi.m_vfsp);
	if (error) {
		goto out1;
	}
	/*
	 * Wait for data in buffer.
	 */
	ab = (struct sam_arfind_buf *)(mp->mi.m_fsact_buf);
	if (ab == NULL) {
		error = EINVAL;
		goto out1;
	}

	mutex_enter(&ab->ab_bufmutex);
	ab->ab_busy = 1;
	mutex_exit(&ab->ab_bufmutex);

	error = 0;
	while (error == 0) {
		int		n;
		int		retval;

		if (mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING)) {
			error = EAGAIN;
			goto out;
		}

		/*
		 * Check number of entries in buffer.
		 * Don't wait if over half full;
		 */
		n = ab->ab_in - ab->ab_out;
		if (n < 0) {
			n += ARFIND_EVENT_MAX;
		}
		if (n > (ARFIND_EVENT_MAX / 2)) {
			break;
		}

		mutex_enter(&ab->ab_bufmutex);
		retval = cv_timedwait_sig(&ab->ab_waitcv, &ab->ab_bufmutex,
		    lbolt + (af.AfWait * hz));
		mutex_exit(&ab->ab_bufmutex);
		if (retval == 0) {
			/*
			 * Interrupted.
			 */
			error = EINTR;
		} else if (retval == -1) {
			/*
			 * Timed out.
			 */
			error = ETIME;
		} else {
			/*
			 * We were signaled.
			 */
			break;
		}
	}

	/*
	 * Copy circular buffer to arfind-s buffer.
	 */
	buf = (uchar_t *)af.AfBuffer.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		buf = (uchar_t *)af.AfBuffer.p64;
	}
	af.AfCount = 0;
	mutex_enter(&ab->ab_bufmutex);
	while (af.AfCount < af.AfMaxCount) {
		int		n;

		n = ab->ab_in - ab->ab_out;
		if (n == 0) {
			break;
		}
		if (n < 0) {
			n = ARFIND_EVENT_MAX - ab->ab_out;
		}
		if (n != 0) {
			int		bytes;

			if (n >= af.AfMaxCount - af.AfCount) {
				n = af.AfMaxCount - af.AfCount;
			}
			bytes = n * sizeof (struct arfind_event);
			if (copyout(&ab->ab_entry[ab->ab_out], buf, bytes) !=
			    0) {
				error = EFAULT;
				break;
			}
			buf += bytes;
		}
		ab->ab_out += n;
		if (ab->ab_out >= ARFIND_EVENT_MAX) {
			ab->ab_out = 0;
		}
		af.AfCount += n;
	}
	af.AfOverflow = ab->ab_overflow;
	ab->ab_overflow = 0;
	af.AfUmount = ab->ab_umount;
	ab->ab_umount = 0;
	ab->ab_hwm_sent = FALSE;
	mutex_exit(&ab->ab_bufmutex);

	/*
	 * Return results to arfind.
	 */
	if (copyout(&af, arg, sizeof (af)) != 0) {
		error = EFAULT;
	}

out:
	mutex_enter(&ab->ab_bufmutex);
	ab->ab_busy = 0;
	if (ab->ab_umount == 1) {
		ab->ab_umount = 0;
		af.AfUmount = 1;
		cv_broadcast(&ab->ab_waitcv);
	}
	mutex_exit(&ab->ab_bufmutex);

out1:
	SAM_SYSCALL_DEC(mp, 0);
	return (error);
}


/*
 *  ---- sam_arfind_fini - remove the arfind buffer.
 */

void
sam_arfind_fini(sam_mount_t *mp)
{
	struct sam_arfind_buf *ab;

	ab = (struct sam_arfind_buf *)(mp->mi.m_fsact_buf);
	if (ab != NULL) {
		mutex_enter(&ab->ab_bufmutex);
		while (ab->ab_busy == 1) {
			cv_wait(&ab->ab_waitcv, &ab->ab_bufmutex);
		}
		mutex_exit(&ab->ab_bufmutex);
		mutex_destroy(&ab->ab_bufmutex);
		cv_destroy(&ab->ab_waitcv);
		kmem_free(ab, sizeof (struct sam_arfind_buf));
		mp->mi.m_fsact_buf = NULL;
	}
}


/*
 *  ---- sam_arfind_hwm - tell arfind about high water mark reached.
 */

void
sam_arfind_hwm(sam_mount_t *mp)
{
	struct sam_arfind_buf *ab;

	ab = (struct sam_arfind_buf *)(mp->mi.m_fsact_buf);
	if (ab == NULL) {
		return;
	}

	/*
	 * Only send once per arfind call.
	 */
	if (!ab->ab_hwm_sent) {
		sam_send_to_arfind(mp->mi.m_inodir, AE_hwm, 0);
		mutex_enter(&ab->ab_bufmutex);
		ab->ab_hwm_sent = TRUE;
		mutex_exit(&ab->ab_bufmutex);
	}
}


/*
 *  ---- sam_arfind_init - initialize the arfind buffer.
 */

void
sam_arfind_init(sam_mount_t *mp)
{
	struct sam_arfind_buf *ab;

	mp->mi.m_fsact_buf =
	    kmem_zalloc(sizeof (struct sam_arfind_buf), KM_SLEEP);
	ab = (struct sam_arfind_buf *)mp->mi.m_fsact_buf;
	sam_mutex_init(&ab->ab_bufmutex, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&ab->ab_waitcv, NULL, CV_DEFAULT, NULL);
	ab->ab_in = ab->ab_out = 0;
	ab->ab_overflow = 0;
	ab->ab_busy = 0;
	ab->ab_hwm_sent = FALSE;
}


/*
 * ----- sam_send_to_arfind - send file change event to arfind.
 *	Record the notification entry in the circular buffer.
 */

void
sam_send_to_arfind(
	sam_node_t *ip,			/* Pointer to inode */
	enum sam_arfind_event event,	/* The file event */
	int copy)			/* Archive copy */
{
	struct sam_arfind_buf *ab;
	struct arfind_event *bp;
	sam_mount_t *mp;	/* Pointer to mount table */
	int		in;
	int		n;

	mp = ip->mp;
	ab = (struct sam_arfind_buf *)(mp->mi.m_fsact_buf);
	if (ab == NULL) {
		return;
	}
	mutex_enter(&ab->ab_bufmutex);

	/*
	 * Check buffer.
	 */
	in = ab->ab_in;
	bp = &ab->ab_entry[in];
	if (++in >= ARFIND_EVENT_MAX) {
		in = 0;
	}
	if (in == ab->ab_out) {

		/*
		 * Buffer overflow.
		 * Count number of overflows, and adjust out to skip
		 * oldest entry.
		 */
		ab->ab_overflow++;
		ab->ab_out++;
		if (ab->ab_out >= ARFIND_EVENT_MAX) {
			ab->ab_out = 0;
		}
	}

	/*
	 * Add entry to buffer.
	 */
	ab->ab_in = in;
	bp->AeCopy = (uint8_t)copy;
	bp->AeEvent = (uint8_t)event;
	bp->AeId = ip->di.id;

	/*
	 * Check number of entries in buffer.
	 * Wake up the arfind if over half full;
	 */
	n = ab->ab_in - ab->ab_out;
	if (n < 0) {
		n += ARFIND_EVENT_MAX;
	}
	if (n > (ARFIND_EVENT_MAX / 2)) {
		cv_signal(&ab->ab_waitcv);
	}
	mutex_exit(&ab->ab_bufmutex);
}


/*
 *  ---- sam_arfind_umount - tell arfind about umount.
 */

void
sam_arfind_umount(sam_mount_t *mp)
{
	struct sam_arfind_buf *ab;

	ab = (struct sam_arfind_buf *)(mp->mi.m_fsact_buf);
	if (ab == NULL) {
		return;
	}
	mutex_enter(&ab->ab_bufmutex);
	ab->ab_umount = 1;
	cv_signal(&ab->ab_waitcv);
	mutex_exit(&ab->ab_bufmutex);
}
