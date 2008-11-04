/*
 * ----- event.c - Perform the daemon file event notification.
 *
 *	Perform the file event daemon notification.
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

#pragma ident "$Revision: 1.7 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/ksynch.h>
#include <vm/seg_vn.h>
#include <vm/seg_kp.h>
#include <sys/vmsystm.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/cpuvar.h>
#include <sys/thread.h>
#include <sys/door.h>
#include <sys/policy.h>

/* ----- SAMFS Includes */

#include "sam/types.h"
#include "sam/samevent.h"

#include "inode.h"
#include "mount.h"
#include "extern.h"
#include "trace.h"

#define	EV_BUFFER_SIZE 256	/* buffer size ( * 1024 bytes) */
#define	INTERVAL 10		/* Periodic notify interval (seconds) */


/*
 * ---- sam_event_em - file event management structure.
 *
 * This structure is allocated and the pointer to it is placed in the
 * mounted file system mount instance.
 */
struct sam_event_em {
	kmutex_t	em_mutex;	/* Data mutex */
	kcondvar_t	em_waitcv;	/* Wait while buffer full */
	boolean_t	em_notify;	/* Notify daemon started */
	door_handle_t	em_doorp;	/* Event daemon door handle */
	int		em_interval;	/* Periodic notify interval */
	int		em_limit;	/* End of data area in buffer */
	int		em_lost_events;	/* Number of lost events */
	int		em_size;
	int		em_mask;	/* Upcall event mask */
	uint32_t	em_seqno;	/* Upcall Sequence number */
	struct anon_map	*em_amp;	/* Mapping shared mem event buffer */
	struct sam_event_buffer *em_buffer; /* Shared memory event buffer */

};

static void sam_daemon_notify(sam_schedule_entry_t *);
static void sam_periodic_notify(sam_schedule_entry_t *entry);
static void sam_start_notify(sam_mount_t *mp, struct sam_event_em *em);
static void sam_call_daemon_door(sam_mount_t *mp, struct sam_event_em *em);


/*
 * ----- sam_event_open - connect an event processing daemon.
 * Process system call SC_event_open from daemon.
 * Initialize event management at SC_event_open from daemon.
 *
 */
int				/* ERRNO if error, 0 if successful. */
sam_event_open(
	void *argp,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	struct sam_event_em	*em;
	struct sam_event_buffer	*eb;
	sam_mount_t		*mp;
	struct sam_event_open_arg arg;
	struct as		*as;
	caddr_t			uaddr;
	int			error;

	/*
	 * Read arguments, find the filesystem, and check permission.
	 */
	em = NULL;
	if (size != sizeof (arg) || copyin(argp, &arg, sizeof (arg))) {
		return (EFAULT);
	}
	if ((mp = sam_find_filesystem(arg.eo_fsname)) == NULL) {
		return (ENOENT);
	}
	if (mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING)) {
		error = EAGAIN;
		goto out;
	}
	if ((error = secpolicy_fs_config(credp, mp->mi.m_vfsp))) {
		goto out;
	}
	if (SAM_IS_STANDALONE(mp)) {
		error = EINVAL;
		goto out;
	}


	em = mp->ms.m_fsev_buf;
	if (em == NULL) {
#ifdef DEBUG
		/*
		 * DEBUG supports running fsalogd from comand line.
		 */
		mutex_enter(&samgt.global_mutex);
		error = sam_event_init(mp, arg.eo_bufsize);
		mutex_exit(&samgt.global_mutex);
		if (error) {
			goto out;
		}
#endif
		em = mp->ms.m_fsev_buf;
		if (em == NULL) {
			error = EINVAL;
			goto out;
		}
	}

	mutex_enter(&em->em_mutex);
	if (em->em_doorp != NULL) {
		/*
		 * Another daemon is connected.
		 */
		sam_start_notify(mp, em);
		error = EBUSY;
		goto out;
	}
	em->em_doorp = door_ki_lookup(arg.eo_door_id);
	if (em->em_doorp == NULL) {
		/*
		 * The door is not active.
		 */
		error = ENOTACTIVE;
		goto out;
	}
	em->em_interval = arg.eo_interval;
	em->em_mask = arg.eo_mask;
	error = 0;

	/*
	 * Map event buffer into user space.
	 * See schedctl_map() in "os/schedctl.c".
	 */
	eb = em->em_buffer;
	if (eb == NULL) {
		error = ENODATA;
	}
	as = curproc->p_as;
	as_rangelock(as);
	map_addr(&uaddr, eb->eb_size, (offset_t)(uintptr_t)eb, 1, 0);
	if (uaddr == NULL) {
		as_rangeunlock(as);
		error = ENODATA;
	} else {
		struct segvn_crargs vn_a;

		vn_a.vp = NULL;
		vn_a.cred = NULL;
		vn_a.offset = 0;
		vn_a.type = MAP_SHARED;
		vn_a.prot = vn_a.maxprot = PROT_ALL;
		vn_a.flags = 0;
		vn_a.amp = (struct anon_map *)em->em_amp;
		vn_a.szc = 0;
		vn_a.lgrp_mem_policy_flags = 0;
		error = as_map(as, uaddr, eb->eb_size, segvn_create, &vn_a);
		as_rangeunlock(as);
		if (error == 0) {
			if (curproc->p_model != DATAMODEL_ILP32) {
				arg.eo_buffer.p64 = (uint64_t)uaddr;
			} else {
				arg.eo_buffer.p32 = (uint32_t)uaddr;
			}
			if (copyout(&arg, argp, sizeof (arg)) != 0) {
				error = EFAULT;
			}
		}
	}

out:
	if (em != NULL) {
		mutex_exit(&em->em_mutex);
	}
	if (error == 0) {
		sam_taskq_add(sam_periodic_notify, mp, NULL,
		    em->em_interval * hz);
	}
	SAM_SYSCALL_DEC(mp, 0);
	return (error);
}


/*
 *  ---- sam_event_fini - end event processing.
 */
void
sam_event_fini(
	sam_mount_t *mp)		/* Pointer to mount table */
{
	struct sam_event_em	*em;
	struct sam_event_buffer *eb;
	struct anon_map		*amp;
	caddr_t			kaddr;
	int			i;

	em = mp->ms.m_fsev_buf;
	if (em == NULL) {
		return;
	}

	/*
	 * Delay up to 3 seconds for fsalogd to pick up events.
	 */
	i = 3;
	eb = em->em_buffer;
	while (i-- > 0) {
		if (eb->eb_in == eb->eb_out) {
			break;
		}
		delay(hz);
	}
	amp = em->em_amp;
	kaddr = (caddr_t)(void *)em->em_buffer;

	/*
	 * Free the anonymous memory. See: schedctl_freepage() in
	 * "os/schedctl.c". The kernel mapping of the page is released
	 * and the page is unlocked.
	 */
	ANON_LOCK_ENTER(&amp->a_rwlock, RW_WRITER);
	segkp_release(segkp, kaddr);

	/*
	 * Decrement the refcnt so the anon_map structure will be freed.
	 */
	if (--amp->refcnt == 0) {
		/*
		 * The current process no longer has the page mapped,
		 * so we have to free everything rather than letting
		 * as_free do the work.
		 */
		anon_free(amp->ahp, 0, PAGESIZE);
		ANON_LOCK_EXIT(&amp->a_rwlock);
		anonmap_free(amp);
	} else {
		ANON_LOCK_EXIT(&amp->a_rwlock);
	}
	mutex_destroy(&em->em_mutex);
	cv_destroy(&em->em_waitcv);
	kmem_free(em, sizeof (struct sam_event_em));
	mp->ms.m_fsev_buf = NULL;
}


/*
 *  ---- sam_event_init - initialize event processing.
 */
int
sam_event_init(
	sam_mount_t *mp,		/* Pointer to mount table */
	int	bufsize)		/* Size of event callout table */
{
	struct sam_event_em *em;
	struct sam_event_buffer *eb;
	struct anon_map *amp;
	caddr_t kaddr;
	int size;

	/*
	 * Allocate the control structure if it does not exist.
	 */
	if (mp->ms.m_fsev_buf) {
		return (0);
	}
	em = (struct sam_event_em *)
	    kmem_zalloc(sizeof (struct sam_event_em), KM_SLEEP);

	/*
	 * Allocate shared memory event buffer.
	 * See  schedctl_getpage() in "os/schedctl.c".
	 * Set up anonymous memory struct.  No swap reservation is
	 * needed since the page will be locked into memory.
	 * Allocate the space.
	 */
	if (bufsize == 0) {
		bufsize = EV_BUFFER_SIZE * 1024;
	}
	size = roundup(bufsize, PAGESIZE);
	amp = anonmap_alloc(size, size, ANON_SLEEP);
	kaddr = segkp_get_withanonmap(segkp, size, KPD_LOCKED | KPD_ZERO, amp);
	if (kaddr == NULL) {
		amp->refcnt--;
		anonmap_free(amp);
		kmem_free(em, sizeof (struct sam_event_em));
		cmn_err(CE_WARN, "SAM-QFS: %s: Event buffer "
		    "allocation failed", mp->mt.fi_name);
		return (ENOMEM);
	}

	/*
	 * Initialize the event management structure.
	 */
	mp->ms.m_fsev_buf = em;
	sam_mutex_init(&em->em_mutex, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&em->em_waitcv, NULL, CV_DEFAULT, NULL);
	em->em_notify = FALSE;
	em->em_amp = amp;
	em->em_interval = INTERVAL;
	em->em_limit = ((size - sizeof (struct sam_event_buffer)) /
	    sizeof (struct sam_event)) - 2;
	em->em_lost_events = 0;
	em->em_size = size;
	em->em_buffer = (struct sam_event_buffer *)(void *)kaddr;

	/*
	 * Initialize the event buffer.
	 */
	eb = em->em_buffer;
	eb->eb_buf_full[0] = 0;
	eb->eb_buf_full[1] = 0;
	eb->eb_umount = 0;
	eb->eb_size = em->em_size;
	eb->eb_ev_lost[0] = 0;
	eb->eb_ev_lost[1] = 0;
	eb->eb_in = eb->eb_out = 0;
	eb->eb_limit = em->em_limit;
	return (0);
}


/*
 * ----- sam_send_to_event - send file change event to event daemon.
 * Do not record event unless daemon set given event in the mask. If the
 * daemon requested this event, record the notification entry in the
 * circular buffer.
 */
void
sam_send_event(
	sam_mount_t *mp,		/* Poiner to mount point */
	sam_disk_inode_t *dp,		/* Pointer to disk inode */
	enum sam_event_num event,	/* The file event */
	ushort_t param,			/* Optional parameter */
	sam_time_t time)		/* Event time */
{
	struct sam_event_em	*em;
	struct sam_event_buffer *eb;
	struct sam_event	*ev;
	timespec_t		system_time;
	int			in;

	em = mp->ms.m_fsev_buf;
	if (em == NULL) {
		return;
	}
	if ((em->em_mask & (1 << event)) == 0) {
		return;
	}
	if (SAM_PRIVILEGE_INO(dp->version, dp->id.ino)) {
		return;
	}

	/*
	 * Check buffer.
	 */
	mutex_enter(&em->em_mutex);
	eb = em->em_buffer;
	in = eb->eb_in;
	if (in < 0 || in >= em->em_limit) {
		/*
		 * Daemon has "damaged" buffer.
		 * Really should do something more dramatic....
		 * Unmap the buffer from the daemon, reset the buffer...
		 * as_unmap();
		 */
		em->em_lost_events++;
		mutex_exit(&em->em_mutex);
		cmn_err(CE_WARN, "SAM-QFS: %s: Daemon damaged event buffer",
		    mp->mt.fi_name);
		return;
	}
	ev = &eb->eb_event[in];
	if (++in >= eb->eb_limit) {
		in = 0;
	}
	if (in == eb->eb_out) {
		/*
		 * Buffer full. Wait for room, but keep notifing the deamon.
		 */
		eb->eb_buf_full[0]++;
		while (in == eb->eb_out && em->em_doorp != NULL) {
			sam_start_notify(mp, em);
			(void) cv_timedwait_sig(&em->em_waitcv,
			    &em->em_mutex, lbolt + (1 * hz));
		}
		if (in == eb->eb_out && em->em_doorp == NULL) {
			em->em_lost_events++;
		}
	}

	/*
	 * Add entry to buffer.
	 */
	eb->eb_in = in;
	ev->ev_num = (ushort_t)event;
	if (time == 0) {
		SAM_HRESTIME(&system_time);
		ev->ev_time = system_time.tv_sec;
	} else {
		ev->ev_time = time;
	}
	ev->ev_id = dp->id;
	ev->ev_pid = dp->parent_id;
	ev->ev_param = param;
	if (++em->em_seqno == 0) {
		em->em_seqno = 1;
	}
	ev->ev_seqno = em->em_seqno;

	/*
	 * Check number of entries in buffer.
	 * Wake up the event if over 1 quarter full.
	 */
	if (!em->em_notify) {
		int		n;

		n = eb->eb_in - eb->eb_out;
		if (n < 0) {
			n += eb->eb_limit;
		}
		if (n > (eb->eb_limit / 4)) {
			sam_start_notify(mp, em);
		}
	}
	mutex_exit(&em->em_mutex);
}


/*
 *  ---- sam_event_umount - tell daemon about umount.
 * Check if deamon requested umount event.
 */
void
sam_event_umount(
	sam_mount_t *mp)		/* Pointer to mount table */
{
	struct sam_event_em *em;

	em = mp->ms.m_fsev_buf;
	if (em == NULL) {
		return;
	}
	if (!em->em_buffer->eb_umount) {
		mutex_enter(&em->em_mutex);
		em->em_buffer->eb_umount = 1;
		mutex_exit(&em->em_mutex);
		if ((em->em_mask & ev_umount)) {
			sam_send_event(mp, &mp->mi.m_inodir->di, ev_umount,
			    0, 0);
		}
	}
	/*
	 * Call the daemon door directly because the fsalogd need to get the
	 * umount event.
	 */
	sam_call_daemon_door(mp, em);
}


/*
 *  ---- sam_daemon_notify - tell daemon to process the event buffer.
 */
static void
sam_daemon_notify(
	sam_schedule_entry_t *entry)
{
	struct sam_event_em *em;
	sam_mount_t *mp;

	mp = entry->mp;
	em = mp->ms.m_fsev_buf;
	if (em != NULL) {
		sam_call_daemon_door(mp, em);
	}
	mutex_enter(&samgt.schedule_mutex);
	sam_taskq_remove(entry);
}


/*
 *  ---- sam_call_daemon_door - call daemon door
 */
static void
sam_call_daemon_door(
	sam_mount_t *mp,
	struct sam_event_em *em)
{
	door_handle_t dh;
	int error;

	mutex_enter(&em->em_mutex);
	em->em_buffer->eb_ev_lost[0] = em->em_lost_events;
	dh = em->em_doorp;
	if (dh != NULL) {
		door_arg_t arg;
		int		timeNow;

		mutex_exit(&em->em_mutex);
		timeNow = SAM_SECOND();
		arg.rbuf = NULL;
		arg.rsize = 0;
		arg.data_ptr = (char *)&timeNow;
		arg.data_size = sizeof (timeNow);
		arg.desc_ptr = NULL;
		arg.desc_num = 0;

		error = door_ki_upcall(dh, &arg);
		mutex_enter(&em->em_mutex);
		if (error != 0) {
			switch (error) {
			case EBADF:
				/*
				 * Door was closed.
				 */
				door_ki_rele(dh);
				em->em_doorp = NULL;
				break;
			default:
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: "
				    "Event daemon notify error %d",
				    mp->mt.fi_name, error);
				break;
			}
		}
	}
	em->em_notify = FALSE;
	cv_signal(&em->em_waitcv);
	mutex_exit(&em->em_mutex);
}


/*
 *  ---- sam_periodic_notify - notify daemon periodically.
 */
static void
sam_periodic_notify(
	sam_schedule_entry_t *entry)
{
	struct sam_event_em *em;
	struct sam_event_buffer *eb;
	sam_mount_t *mp;

	mp = entry->mp;
	em = mp->ms.m_fsev_buf;
	if (em == NULL) {
		return;
	}
	mutex_enter(&em->em_mutex);
	eb = em->em_buffer;
	if (!em->em_notify && (eb->eb_in != eb->eb_out)) {
		sam_start_notify(mp, em);
	}
	mutex_exit(&em->em_mutex);
	mutex_enter(&samgt.schedule_mutex);
	sam_taskq_remove(entry);
	sam_taskq_add(sam_periodic_notify, mp, NULL, em->em_interval * hz);
}


/*
 *  ---- sam_start_notify - start (schedule) the daemon notification.
 * em_mutex must have been acquired by caller. Exit sam_start_notify with
 * em_mutex held.
 */
static void
sam_start_notify(
	sam_mount_t *mp,
	struct sam_event_em *em)
{
	if (!em->em_notify) {
		em->em_notify = TRUE;
		mutex_exit(&em->em_mutex);
		sam_taskq_add(sam_daemon_notify, mp, (void *)em, 0);
		mutex_enter(&em->em_mutex);
	}
}
