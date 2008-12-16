/*
 * kstats_linux.h - kernel statistics for SAM-QFS on Linux
 *
 *  Defines structures, functions and macros for SAM-QFS usage of kstats.
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
#ifndef _INCLUDE_KSTATS_LINUX_H
#define	_INCLUDE_KSTATS_LINUX_H

#ifdef sun
#pragma ident "$Revision: 1.16 $"
#endif

#ifndef _KERNEL
#error NOT KERNEL
#endif

/*
 * The kstat structure and related defines are modeled after the
 * solaris version.
 */

#define	KSTAT_STRLEN	31	/* 30 chars + NULL; must be 16 * n - 1 */

typedef struct kstat_named {
	char	name[KSTAT_STRLEN];
	uchar_t	data_type;
	union {
		u64		ui64;
		unsigned long	ul;
	} value;
} kstat_named_t;


#define	KSTAT_DATA_UINT64	4
#define	KSTAT_DATA_ULONG	8


/* ----- SAM statistics. */

struct sam_sam_statistics {
	kstat_named_t stage_start;	/* Stages started (sent to daemon) */
	kstat_named_t stage_partial;	/* Stage request satisfied by partial */
	kstat_named_t stage_window;	/* Stage request in stage-n window */
	kstat_named_t stage_errors;	/* Stage completed with error */
	kstat_named_t stage_reissue;	/* Stage reissued due to EREMCHG */
	kstat_named_t stage_cancel;	/* Stage cancelled */
};

extern struct sam_sam_statistics sam_sam_stats;

/* ----- Shared QFS statistics: client. */

struct sam_shared_client_statistics {
	kstat_named_t msg_in;		/* Messages received by client */
	kstat_named_t msg_out;		/* Messages sent by client */
	kstat_named_t sync_pages;	/* Sync-page actions */
	kstat_named_t inval_pages;	/* Invalidate-page actions */
	kstat_named_t stale_indir;	/* Stale-indirect actions */
	kstat_named_t dio_switch;	/* Direct-io switches */
	kstat_named_t abr_switch;	/* ABR-io switches */
	kstat_named_t expire_task;	/* Lease expiration task count */
	kstat_named_t expire_task_dup;	/* Redundant starts of expire task */
	kstat_named_t retransmit_msg;	/* Retransmit messages to server */
	kstat_named_t notify_ino;	/* Cannot find ino -- notify msg */
	kstat_named_t notify_expire;	/* NOTIFY_lease_expire received */
	kstat_named_t expired_inuse;	/* NOTIFY_lease_expire expired lease */
};

extern struct sam_shared_client_statistics sam_shared_client_stats;

/* ----- Thread statistics.  */

struct sam_thread_statistics {
	kstat_named_t taskq_add;	/* Number of additions to task queue */
	kstat_named_t taskq_add_dup;	/* Number of redundant additions */
	kstat_named_t taskq_dispatch;	/* Num tasks immediately executed */
	kstat_named_t max_share_threads; /* Max no. of active sharefs threads */
};

extern struct sam_thread_statistics sam_thread_stats;


/*
 * ----- Debugging statistics.  These may be more expensive or only useful
 *     for test purposes.
 */

#ifdef DEBUG

struct sam_debug_statistics {
	kstat_named_t nreads;			/* Number of reads */
	kstat_named_t nwrites;			/* Number of writes */
	kstat_named_t statfs_thread;		/* Runs of statfs_thread */
};

extern struct sam_debug_statistics sam_debug_stats;

#endif

#define	SAM_COUNT64(cat, stat)	(sam_##cat##_stats.stat.value.ui64++)

#ifdef DEBUG
#define	SAM_DEBUG_COUNT64(stat)		SAM_COUNT64(debug, stat)
#else
#define	SAM_DEBUG_COUNT64(stat)
#endif

#endif /* _INCLUDE_KSTATS_LINUX_H */
