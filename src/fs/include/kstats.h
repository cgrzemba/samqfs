/*
 *	kstats.h - kernel statistics for SAM-QFS
 *
 *  Defines structures, functions and macros for SAM-FS usage of kstats.
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

#ifdef sun
#pragma ident "$Revision: 1.28 $"
#endif

/* ----- General statistics.  */

struct sam_gen_statistics {
	kstat_named_t version;
	kstat_named_t n_fs_configured;
	kstat_named_t n_fs_mounted;
	kstat_named_t nhino;
	kstat_named_t ninodes;
	kstat_named_t inocount;
	kstat_named_t inofree;
};

extern struct sam_gen_statistics sam_gen_stats;

/* ----- Direct I/O statistics. */

struct sam_dio_statistics {
	kstat_named_t mmap_2_dio;	/* mmap requests converted to dio */
	kstat_named_t dio_2_mmap;	/* dio requests converted to mmap */
};

extern struct sam_dio_statistics sam_dio_stats;

/* ----- Paging statistics. */

struct sam_page_statistics {
	kstat_named_t flush;		/* calls to sam_flush_pages */
	kstat_named_t retry;		/* returns of EDEADLK for VOP retry */
};

extern struct sam_page_statistics sam_page_stats;

/* ----- SAM statistics. */

struct sam_sam_statistics {
	kstat_named_t stage_start;	/* Stages started (sent to daemon) */
	kstat_named_t stage_partial;	/* Stage req satisfied by partial */
	kstat_named_t stage_window;	/* Stage request in stage-n window */
	kstat_named_t stage_errors;	/* Stage completed with error */
	kstat_named_t stage_reissue;	/* Stage reissued due to EREMCHG */
	kstat_named_t stage_cancel;	/* Stage cancelled */
	kstat_named_t archived;		/* Archive copy created */
	kstat_named_t archived_incon;	/* Inconsistent copy created */
	kstat_named_t archived_rele;	/* Released after archiving */
	kstat_named_t noarch_incon;	/* Failure to archive: inconsistent */
	kstat_named_t noarch_already;	/* Failure to archive: already arch'd */
};

extern struct sam_sam_statistics sam_sam_stats;

/* ----- Shared QFS statistics: server & client. */

struct sam_shared_server_statistics {
	kstat_named_t msg_in;		/* Messages received by server */
	kstat_named_t msg_out;		/* Messages sent by server */
	kstat_named_t msg_dup;
	kstat_named_t lease_new;
	kstat_named_t lease_add;
	kstat_named_t lease_wait;
	kstat_named_t lease_grow;
	kstat_named_t failovers;
	kstat_named_t callout_action;
	kstat_named_t callout_relinquish_lease;
	kstat_named_t callout_directio;
	kstat_named_t callout_stage;
	kstat_named_t callout_abr;
	kstat_named_t callout_acl;
	kstat_named_t notify_lease;
	kstat_named_t notify_dnlc;
	kstat_named_t notify_expire;
	kstat_named_t expire_task;	/* Lease expiration task count */
	kstat_named_t expire_task_dup;	/* Redundant starts of expire task */
};

extern struct sam_shared_server_statistics sam_shared_server_stats;

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
	kstat_named_t expired_inuse;	/* NOTIFY_lease_expire expired */
					/*   in-use inode. */
};

extern struct sam_shared_client_statistics sam_shared_client_stats;

/* ----- Thread statistics.  */

struct sam_thread_statistics {
	kstat_named_t block_count;	/* Block thread run count */
	kstat_named_t reclaim_count;	/* Reclaim thread run count */
	kstat_named_t taskq_add;	/* Number of additions to task queue */
	kstat_named_t taskq_add_dup;	/* Number of redundant additions */
	kstat_named_t taskq_add_immed;	/* Num immediate additions */
	kstat_named_t taskq_dispatch;	/* Num tasks executed immediately */
	kstat_named_t taskq_dispatch_fail; /* Num failed dispatches */
	kstat_named_t inactive_count;	/* Inactivate thread run count */
	kstat_named_t max_share_threads; /* Max num active sharefs threads */
};

extern struct sam_thread_statistics sam_thread_stats;

/* ----- DNLC statistics.	*/

struct sam_dnlc_statistics {
	kstat_named_t ednlc_starts;	/* directory dnlc starts */
	kstat_named_t ednlc_purges;	/* directory dnlc purges */
	kstat_named_t ednlc_name_hit;	/* directory dnlc name cache hit */
	kstat_named_t ednlc_name_found;	/* directory dnlc name found */
	kstat_named_t ednlc_name_missed; /* directory dnlc name missed */
	kstat_named_t ednlc_space_found; /* directory dnlc space found */
	kstat_named_t ednlc_space_missed; /* directory dnlc space missed */
	kstat_named_t ednlc_too_big;	/* directory dnlc error DTOOBIG */
	kstat_named_t ednlc_no_mem;	/* directory dnlc error DNOMEM */
	kstat_named_t dnlc_neg_entry;	/* negative dnlc entries made */
	kstat_named_t dnlc_neg_uses;	/* negative dnlc entries used */
};

extern struct sam_dnlc_statistics sam_dnlc_stats;


/*
 * ----- Debugging statistics.  These may be more expensive or only useful
 *	for test purposes.
 */

#ifdef DEBUG

struct sam_debug_statistics {
	kstat_named_t nreads;		/* Number of reads */
	kstat_named_t nwrites;		/* Number of writes */
	kstat_named_t breads;		/* Number of bytes read */
	kstat_named_t bwrites;		/* Number of bytes written */
	kstat_named_t client_dir_putpage; /* putpage on shared clnt dir page */
};

extern struct sam_debug_statistics sam_debug_stats;

#endif

#define	SAM_COUNT(cat, stat)		(sam_##cat##_stats.stat.value.ul++)
#define	SAM_COUNT64(cat, stat)		(sam_##cat##_stats.stat.value.ull++)
#define	SAM_ADD(cat, stat, n)		(sam_##cat##_stats.stat.value.ul += n)
#define	SAM_ADD64(cat, stat, n)		(sam_##cat##_stats.stat.value.ull += n)
#define	SAM_COUNT_LOCKED(cat, stat)	\
			(atomic_add_32(&sam_##cat##_stats.stat.value.ul, 1))
#define	SAM_COUNT64_LOCKED(cat, stat) \
			(atomic_add_64(&sam_##cat##_stats.stat.value.ull, 1))
#define	SAM_ADD_LOCKED(cat, stat, n) \
			(atomic_add_32(&sam_##cat##_stats.stat.value.ul, n))
#define	SAM_ADD64_LOCKED(cat, stat, n) \
			(atomic_add_64(&sam_##cat##_stats.stat.value.ull, n))

#ifdef DEBUG
#define	SAM_DEBUG_COUNT(stat)			SAM_COUNT(debug, stat)
#define	SAM_DEBUG_COUNT64(stat)			SAM_COUNT64(debug, stat)
#define	SAM_DEBUG_ADD(stat, n)			SAM_ADD(debug, stat, n)
#define	SAM_DEBUG_ADD64(stat, n)		SAM_ADD64(debug, stat, n)
#define	SAM_DEBUG_COUNT_LOCKED(stat)	SAM_COUNT_LOCKED(debug, stat)
#define	SAM_DEBUG_COUNT64_LOCKED(stat)	SAM_COUNT64_LOCKED(debug, stat)
#define	SAM_DEBUG_ADD_LOCKED(stat, n)	SAM_ADD_LOCKED(debug, stat, n)
#define	SAM_DEBUG_ADD64_LOCKED(stat, n)	SAM_ADD64_LOCKED(debug, stat, n)
#else
#define	SAM_DEBUG_COUNT(stat)
#define	SAM_DEBUG_COUNT64(stat)
#define	SAM_DEBUG_ADD(stat, n)
#define	SAM_DEBUG_ADD64(stat, n)
#define	SAM_DEBUG_COUNT_LOCKED(stat)
#define	SAM_DEBUG_COUNT64_LOCKED(stat)
#define	SAM_DEBUG_ADD_LOCKED(stat, n)
#define	SAM_DEBUG_ADD64_LOCKED(stat, n)
#endif
