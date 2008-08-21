/*
 * copy_defs.h - common stager copy process definitions
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

#if !defined(COPY_DEFS_H)
#define	COPY_DEFS_H

#pragma ident "$Revision: 1.31 $"

/* Structures. */

/*
 * Copy request.
 * Structure representing a singly-linked list of requests
 * for a copy instance.
 */
typedef struct CopyRequestInfo {
	/*
	 * VSN and sequence number are used to identifier memory mapped
	 * stream file associated with the copy request.
	 */
	vsn_t		cr_vsn;		/* VSN label */
	int		cr_seqnum;	/* sequence number */
	ushort_t	cr_flags;

	struct CopyRequestInfo *cr_next;	/* next request in queue */
	boolean_t	cr_gotItFlag;
	pthread_cond_t	cr_gotIt;
} CopyRequestInfo_t;

/*
 * Copy instance.
 * Structure defining static context of a copy thread.
 */
typedef struct CopyInstanceInfo {
	int		ci_lib;		/* unique library table num */
	int		ci_eq;		/* library's equipment number */
	media_t		ci_media;	/* media type */
	int		ci_rmfn;	/* unique drive table number */
	int		ci_drive;	/* drive's equipment number */

	vsn_t		ci_vsn;		/* current or last loaded VSN label */
	int		ci_vsnLib;	/* library table num where vsn exists */
	int		ci_seqnum;	/* current or last active sequence */
					/*    number */

	int		ci_numBuffers;	/* number of i/o buffers */
	boolean_t	ci_lockbuf;	/* lock buffers */

	boolean_t	ci_created;	/* set if copy thread already created */
	pid_t		ci_pid;		/* pid of running copy process */

	/* Pointer to first request in queue. */
	CopyRequestInfo_t	*ci_first;

	/* Pointer to last request in queue. */
	CopyRequestInfo_t	*ci_last;

	pthread_mutex_t	ci_requestMutex;
	pthread_cond_t	ci_request;

	boolean_t	ci_busy;	/* set if copy process is busy */
	pthread_mutex_t	ci_runningMutex;
	pthread_cond_t	ci_running;

	boolean_t	ci_shutdown;	/* set to shutdown copy proc */
	ushort_t	ci_flags;

	struct in6_addr	ci_hostAddr;	/* remote host address */
} CopyInstanceInfo_t;

#define	COPY_INSTANCE_LIST_MAGIC	05501531
#define	COPY_INSTANCE_LIST_VERSION	80601	/* YMMDD */

/*
 * Copy instance list.
 * Structure representing a list of copy processes/threads to be used
 * by the scheduler.
 */
typedef struct CopyInstanceList {
	uint32_t	cl_magic;
	uint32_t	cl_version;
	time_t		cl_create;	/* stagereqfile's creation time */
	size_t		cl_size;
	int		cl_entries;	/* number of threads */
	CopyInstanceInfo_t	cl_data[1];
} CopyInstanceList_t;

/*
 * Instance flags.
 */
enum {
	CI_samremote =	1 << 0,	/* Copy process defined for removable media */
				/*    on remote host (SAM remote) */
	CI_orphan =	1 << 1,	/* Copy process spawned by dead stagerd */
	CI_addrval =	1 << 2,	/* host address is set */
	CI_ipv6	 =	1 << 3,	/* host address is an IPv6 address */
	CI_shutdown =	1 << 4,	/* shutdown stager */
	CI_failover =	1 << 5	/* shutdown for voluntary failover */
};

/* Check if shutdown has been set for this instance. */
#define	IF_SHUTDOWN(ci)	((ci)->ci_flags & (CI_shutdown | CI_failover))

#endif /* COPY_DEFS_H */
