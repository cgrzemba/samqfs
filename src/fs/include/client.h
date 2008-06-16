/*
 *	client.h - client structures for the SUN SAM-QFS shared file system.
 *
 *	Defines the structure of the client table for this instance
 *	of the SAM-QFS filesystem mount. The vfs entry points to
 *	this mount table and this mount table points back at the vfs
 *	entry.
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

#ifndef	_SAM_FS_CLIENT_H
#define	_SAM_FS_CLIENT_H

#ifdef sun
#pragma ident "$Revision: 1.75 $"
#endif

#ifdef	linux
#ifdef	__KERNEL__
#include <linux/file.h>
#else
#include <sys/file.h>
#endif	/* __KERNEL__ */
#else	/* linux */

#include <sys/file.h>

#endif	/* linux */

#define	SAM_FAILOVER_DELAY	(5)
#define	SAM_MIN_DELAY		(15)
#define	SAM_MAX_DELAY		(60)
#define	SAM_MOUNT_TIMEOUT	(30)

#define	SAM_SRVR_INIT_TIMEOUT	(15)

#define	FSPART_INTERVAL		(10)
#ifdef linux
#define	VFSSTAT_INTERVAL	(5)
#endif /* linux */

/*
 * The socket file pointer needs special locking. It is desirable to allow
 * mutiple threads to send and receive messages, while allowing only one
 * thread to modify it, whether to setup a new fp or clear it for some
 * error condition. Thus a simple reference count lock is not adequate.
 * We have chosen a reader-write adaptive lock to protect this socket
 * file. The following structure is used to hold the fp as well as its
 * lock.
 *
 * This reader-writer lock is a long term lock, it can be held across
 * blocking calls e.g. streams related calls. To help threads that need a
 * quick check, another mutex lock is used. We already have this lock -
 * m_cl_wrmutex and cl_wrmutex. So a thread may hold just this lock while
 * reading fp, but must hold both the mutex and the writer lock to modify
 * it. It is suggested that this is used as a part of a larger protected
 * structure, rather than being allocated and freed dynamically. This makes
 * reading trace data easier. If a dynamic allocation/deallocation scheme
 * is implemented, the above mutex lock may be used to protect it.
 *
 * The locking order is : mutex lock -> rdwr lock
 *
 */

typedef struct sam_sock_handle {
#ifdef sun
	file_t	*sh_fp;		/* socket file pointer */
#endif /* sun */
#ifdef linux
	struct socket *sh_fp;
#endif /* linux */
	krwlock_t sh_rwlock;	/* reader-writer lock to protect fp */
	kmutex_t sh_wmutex;	/* Mutex lock to serialize socket writes */
} sam_sock_handle_t;

#define	SAM_SOCK_HANDLE_INIT(sh)	{				\
	(sh)->sh_fp = NULL;						\
	RW_LOCK_INIT_OS(&(sh)->sh_rwlock, NULL, RW_DEFAULT, NULL);	\
	sam_mutex_init(&(sh)->sh_wmutex, NULL, MUTEX_DEFAULT, NULL);	\
}

/*
 * Used by threads that want to send or receive messages on the socket
 */
#define	SAM_SOCK_READER_LOCK(sh) \
	RW_LOCK_OS(&((sh)->sh_rwlock), RW_READER)
#define	SAM_SOCK_READER_UNLOCK(sh) \
	RW_UNLOCK_OS(&((sh)->sh_rwlock), RW_READER)

/*
 * Used by threads that want to modify the socket fp
 */
#define	SAM_SOCK_WRITER_LOCK(sh) \
	RW_LOCK_OS(&((sh)->sh_rwlock), RW_WRITER)
#define	SAM_SOCK_WRITER_UNLOCK(sh) \
	RW_UNLOCK_OS(&((sh)->sh_rwlock), RW_WRITER)


#ifdef METADATA_SERVER
/*
 * ----- sam_msg_array_t
 * Array of outstanding client messages. Each ordinal has a chain
 * of messages; the head and tail indexes are in the client_entry.
 * Ack is used to determine if message is a duplicate (retransmitted).
 */

typedef enum sam_active_e {
	SAM_PROCESSING,
	SAM_WAITFOR_ACK
} sam_active_t;

typedef struct sam_msg_array {
	list_node_t node;	/* Next/Previous pointers */
	int32_t client_ord;	/* Client ordinal, 1..max_clients */
	uint32_t seqno;		/* Message sequence number */
	int32_t error;		/* Error for processed message, active = 2 */
	sam_active_t active;	/* State of message */
	uchar_t command;	/* Command */
	ushort_t operation;	/* Command operation */
	sam_id_t id;		/* Inode id for completed msg, active = 2 */
	sam_ino_record_t irec;	/* Inode instance record */
	uint16_t granted_mask;	/* Leases granted by server */
	uint32_t gen[SAM_MAX_LTYPE]; /* Generation # of lease(s) */
} sam_msg_array_t;

/*
 * ----- client_entry_t
 * Client array for shared data.
 * Client ordinal is place in this table, beginning at 1.
 * Ack is used to determine if message is a duplicate (retransmitted).
 * cl_msgmutex is the per-client lock for the client message chain.
 * m_cl_mutex is the per-mount lock for the client message chain.
 * m_cl_wrmutex is the per-mount lock for the free message chain.
 * cl_msg_mutex (in sam_client_msg) is the per-message-entry lock in the chain.
 *
 * Lock orderings:
 *    cl_msgmutex > m_cl_wrmutex
 *    m_cl_mutex > cl_msg_mutex
 */

/*
 * Client flags
 */
#define	SAM_CLIENT_DEAD	0x01	/* Client assumed dead during failover */
#define	SAM_CLIENT_INOP	0x02	/* Don't hold up failover (client known dead) */
#define	SAM_CLIENT_SOCK_BLOCKED	0x04	/* Writing to client returned EAGAIN */
#define	SAM_CLIENT_OFF_PENDING	0x08	/* Client transitioning to OFF */
#define	SAM_CLIENT_OFF	0x10	/* Client marked OFF in hosts file */

typedef struct client_entry {
	upath_t hname;		/* Client host name */
	int hostid;		/* Host identifier */
	int64_t cl_msg_time;	/* Time message received */
	int64_t cl_rcv_time;	/* Time last lease message received */
	int64_t cl_resync_time;	/* Time resync message received */
	uint32_t cl_sock_flags;	/* Client socket flags */
	uint32_t cl_status;	/* Mount status flags */
	uint32_t cl_config;	/* Mount config flags */
	uint32_t cl_config1;	/* More mount config flags */
	uint32_t cl_flags;	/* Client status flags */
	uint32_t cl_busy_threads; /* Number of busy threads for this client */
	uint64_t cl_tags;	/* Client's behavior tags */
	sam_sock_handle_t cl_sh; /* Server socket file for this client */
	kmutex_t cl_wrmutex;	/* Mutex lock for write socket to client */
	kthread_t *cl_thread;	/* Shared client thread id */
	int32_t cl_nomsg;	/* Count of outstanding messages */
	uint32_t cl_min_seqno;	/* Lowest seqno number not yet completed */
	kmutex_t cl_msgmutex;	/* Mutex lock for this client's msg array */
	short fs_count;		/* Number of family set members */
	short mm_count;		/* Number of meta set members */
	sam_queue_t queue;	/* message queue for this client */
	int cl_offtime;		/* Transition from pending to off state */
} client_entry_t;


/*
 * ----- sam_lease_ino
 * Forward link list of client leases in mount tbl.
 * Expired by the reclaim thread.
 */
typedef struct sam_lease_head	{
	struct sam_lease_ino *forw;
	struct sam_lease_ino *back;
} sam_lease_head_t;

typedef struct sam_client_lease {
	ushort_t	leases;		/* Client leases held on the server */
	ushort_t	wt_leases;	/* Client waiting on leases */
	ushort_t	actions;	/* Server directed actions */
	/* volatile flags shared by srvr & clients */
	sam_shvfm_t	shflags;
	int32_t		client_ord;	/* Client ordinal, 1..max_clients */
	int32_t		wr_seq;		/* Write seq num when lease granted */
	uint32_t	attr_seqno;	/* Last client attr sequence number */
	clock_t		time[SAM_MAX_LTYPE];	/* Exp time for leases */
	uint32_t	gen[SAM_MAX_LTYPE];	/* Generation # for leases */
} sam_client_lease_t;

typedef struct sam_lease_ino {
	struct sam_lease_head lease_chain;
	struct sam_node *ip;	/* Sam inode pointer */
	int32_t no_clients;	/* No. of clients w/ leases/waiting leases */
	int32_t next_ord;	/* Next client ord for CALLOUT_lease */
	int32_t max_clients;	/* Array holds max_clients */
	sam_client_lease_t lease[1];	/* Array of leases */
} sam_lease_ino_t;
#endif /* METADATA_SERVER */

/*
 * ----- sam_cl_flock
 * Forward list of file and record leases held on the client.
 */
typedef struct sam_cl_flock {
	struct sam_cl_flock *cl_next;
	int64_t offset;		/* Frlock file offset */
	int32_t filemode;	/* filemode flags, see file.h */
	int32_t cmd;		/* Command for flock */
	int32_t nfs_lock;	/* Set if nfs lock */
	sam_share_flock_t flock;
} sam_cl_flock_t;

/* ----- Client forward outstanding message chain structure. */

typedef struct sam_client_msg {
	struct sam_client_msg *cl_next;
	struct sam_san_message *cl_msg;	/* Ptr to msg for client name ops */
	kmutex_t	cl_msg_mutex;	/* Mutex - covers this msg entry */
	int		cl_error;	/* Errno ret'd for client operation */
	uint32_t	cl_seqno;	/* Seqno for client outgoing command */
	ushort_t	cl_command;	/* Command */
	ushort_t	cl_operation;	/* Command operation */
	boolean_t	cl_done;	/* This message is done */
} sam_client_msg_t;


#endif  /* _SAM_FS_CLIENT_H */
