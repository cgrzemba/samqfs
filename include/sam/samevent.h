/*
 * ----- samevent.h - SAM-QFS file system event notification definitions.
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

#ifndef	_SAM_FS_SAMEVENT_H
#define	_SAM_FS_SAMEVENT_H

#include "sam/types.h"

/* File events. */
enum sam_event_num {
	ev_none,
	ev_create,			/* File created */
	ev_change,			/* Attributes changed - uid, gid */
	ev_close,			/* File closed */
	ev_rename,			/* File renamed */
	ev_remove,			/* File removed */
	ev_offline,			/* File changed to offline */
	ev_online,			/* File changed to online */
	ev_archive,			/* Archive copy made */
	ev_modify,			/* Archive copies stale */
	ev_archange,			/* Archive copy changed */
	ev_restore,			/* File was restored */
	ev_umount,			/* File system umount */
	ev_max
};

/* File system daemon status */
enum sam_fsa_daemon_status {
	FSA_STATUS_NONE = 0,		/* Daemon not started */
	FSA_STATUS_RUNNING = 1,		/* Daemon running */
	FSA_STATUS_EXIT = 2,		/* Daemon exited */
	FSA_STATUS_ABORT = 3 		/* Daemon aborted */
};

/* Begin: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack(4)
#endif

/* ----- sam_event_open_arg - SC_event shared memory buffer system call. */

struct sam_event_open_arg {
	uname_t	eo_fsname;		/* File system name */
	uint_t	eo_door_id;		/* Daemon's door */
	int	eo_interval;		/* Periodic upcall interval (seconds) */
	int	eo_bufsize;		/* Maximum bufsize in bytes */
	int	eo_mask;		/* Daemon mask for event */
	SAM_POINTER(void) eo_buffer;	/* returned - addr of shared buffer */
};

/*
 * ----- sam_event_buffer - event buffer.
 * Shared between kernel and daemon.
 */

typedef struct sam_event {
	sam_id_t    ev_id;	/* Inode */
	sam_id_t    ev_pid;	/* Parent inode */
	ushort_t    ev_num;	/* File action event */
	ushort_t    ev_param;	/* Parameter */
	sam_time_t  ev_time;	/* File action event time in seconds */
	uint32_t    ev_seqno;	/* Sequence number */
	ushort_t    ev_param2;	/* Parameter 2 */
	ushort_t    fill;
} sam_event_t;

struct sam_event_buffer {
	int	eb_size;
	short	eb_umount;	/* File system unmount started when non-zero */
	short	eb_dstatus;	/* Daemon status */
	int	eb_buf_full[2];	/* Cnt of buffer overflows (kernel & daemon) */
	int	eb_ev_lost[2];	/* Events lost */
	ATOM_INT_T eb_in;	/* Next input offset */
	ATOM_INT_T eb_out;	/* Next output offset */
	int	eb_limit;	/* End of buffer */
	sam_event_t eb_event[1];
};

/* End: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack()
#endif


#endif /* _SAM_FS_SAMEVENT_H */
