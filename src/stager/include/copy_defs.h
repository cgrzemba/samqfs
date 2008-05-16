/*
 * copy_defs.h
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

#pragma ident "$Revision: 1.28 $"

#include "aml/remote.h"
#include "aml/sam_rft.h"

#include "stager_lib.h"
#include "stream.h"

/* Structures. */

/*
 * Copy request.
 * Structure representing a singly-linked list of requests
 * for a copy thread.
 */
typedef struct CopyRequest {

	/*
	 * VSN and sequence number are used to identifier memory mapped
	 * stream file associated with the copy request.
	 */
	vsn_t		vsn;		/* VSN label */
	int		seqnum;		/* sequence number */

	ushort_t	flags;

	struct CopyRequest *next;	/* next request in queue */
	boolean_t	got_it_flag;
	pthread_cond_t	got_it;
} CopyRequest_t;

enum {
	CR_REMOTE =	1 << 0	/* Copy request defined for disk media */
				/*    on remote host */
};

/*
 * Copy context.
 * Structure defining static context of a copy thread.
 */
typedef struct CopyInstance {
	int		lib;		/* unique library table num */
	int		eq;		/* library's equipment number */
	media_t		media;		/* media type */
	int		rmfn;		/* unique drive table number */
	int		drive_eq;	/* drive's equipment number */

	vsn_t		vsn;		/* current or last loaded VSN label */
	int		vsn_lib;	/* library table num where vsn exists */
	int		seqnum;		/* current or last active sequence */
					/*    number */
	int		num_buffers;	/* number of i/o buffers */
	boolean_t	lockbuf;	/* lock buffers */

	boolean_t	created;	/* set if copy thread already created */
	pid_t		pid;		/* pid of running copy process */

	CopyRequest_t	*first;		/* pointer to first request in queue */
	CopyRequest_t	*last;		/* pointer to last request in queue */
	pthread_mutex_t	request_mutex;
	pthread_cond_t	request;

	boolean_t	idle;		/* set if copy thread idle */
	pthread_mutex_t	running_mutex;
	pthread_cond_t	running;

	boolean_t	shutdown;	/* set to shutdown copy proc */
	ushort_t	flags;

	struct in6_addr	host_addr;	/* remote host address */
} CopyInstance_t;

#define	STAGER_COPYPROC_LIST_MAGIC	05501531
#define	STAGER_COPYPROC_LIST_VERSION	60522	/* YMMDD */

/*
 * Copy proc list.
 * Structure representing a list of copy processes/threads to be used
 * by the scheduler.
 */
typedef struct CopyProcList {
	uint32_t	magic;
	uint32_t	version;
	time_t		create;		/* stagereqfile's creation time */
	size_t		size;
	int		entries;	/* number of threads */
	CopyInstance_t	data[1];
} CopyProcList_t;

/*
 * Context flags.
 * FIXME - create, idle, shutdown, open_fd can be flags
 */
enum {
	CI_remote =	1 << 0,	/* Copy process defined for removable media */
				/*    on remote host */
	CI_orphan =	1 << 1,	/* Copy process spawned by dead stagerd */
	CI_addrval =	1 << 2,	/* host_addr is set */
	CI_ipv6	 =	1 << 3,	/* host_addr is an IPv6 address */
	CI_shutdown =	1 << 4,	/* shutdown stager */
	CI_failover =	1 << 5	/* shutdown stager for voluntary failover */
};

#define	IS_REMOTE_STAGE(c)	((c)->flags & CI_remote)
#define	IS_SHUTDOWN(c)		((c)->flags & (CI_shutdown | CI_failover))

/*
 * Copy buffer.
 * Structure defining state and context of data in buffer.
 */
typedef struct CopyBuffer {
	int	valid;		/* set if data in buffer is valid */
	int	error;		/* errno if read/write error for buffer */
	int	position;	/* mau position contained in this buffer */
	int	count;		/* amount of data transferred to buffer */
	char	*data;		/* data in buffer */
} CopyBuffer_t;

typedef struct CopyControl {
	int		rmfn;	/* removable media file name */
	int		rmfd;	/* local removable media file descriptor */
	SamrftImpl_t	*rft;	/* if remote, file transfer descriptor */

	ushort_t	flags;
	time_t		start_time;	/* start time for reading from media */
					/* this does not include positioning */

	/*
	 * A medium allocation unit is defined as the size of the smallest
	 * addressable part on a medium that can be accessed.  On tapes,
	 * this is a block.  On opticial, this is a sector.
	 */
	offset_t	mau;	/* medium allocation unit size in bytes */

	ThreadState_t	ready;	/* archive file ready to stage */
	ThreadState_t	done;	/* stage read complete */

	FileInfo_t	*file;	/* file to stage */
	int		offset;	/* if first pass of staging, contains */
				/* file data offset in TAR_RECORDSIZE */
				/* units.  Calculated by read thread and */
				/* used by write thread. */

	int		currentPos;	/* current position of media */

	int		mauInBuffer;	/* number of ma units in one buffer */
	ThreadSema_t	fullBuffers;	/* count of full buffers */
	ThreadSema_t	emptyBuffers;	/* count of empty buffers */

	int		inBuf;		/* input buffer index */
	int		outBuf;		/* output buffer index */
	int		bufSize;	/* buffer size */
	int		numBuffers;	/* number of buffers */
	CopyBuffer_t	*buffers;

} CopyControl_t;

/*
 * Copy file complete.
 * Structure representing a singly-linked list of stage requests
 * that have completed.
 */
typedef struct CopyComplete {
	int			id;	/* stage file request that is done */
	struct CopyComplete	*next;	/* next request completed */
} CopyComplete_t;

/*
 * Flags defined for CopyControl_t structure.
 */
enum {
	CC_cancel =	1 << 0,		/* cancel staging */
	CC_error =	1 << 1,		/* staging error */
	CC_disk	=	1 << 2,		/* staging from disk */
	CC_honeycomb =	1 << 3		/* staging from STK 5800 disk */
};

#define	CC_diskArchiving (CC_disk | CC_honeycomb)

#endif /* COPY_DEFS_H */
