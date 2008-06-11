/*
 * inode.h - SAM-FS file system incore inode definitions.
 *
 *	Defines the structure of incore inodes for the SAM file system.
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

#ifndef	_SAM_FS_INODE_H
#define	_SAM_FS_INODE_H

#if !defined(linux)
#pragma ident "$Revision: 1.201 $"
#endif

#ifdef linux

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/vfs.h>
#include <linux/fcntl.h>
#else	/* __KERNEL */
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/fcntl.h>
#endif	/* __KERNEL__ */

#else	/* linux */

#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/t_lock.h>
#include <sys/fcntl.h>
#include <sys/atomic.h>
#include <sys/dnlc.h>
#include <sys/sunddi.h>

#endif	/* linux */

#include "sam/types.h"
#include "sam/param.h"
#include "sam/resource.h"
#include "dirent.h"

#ifndef	linux
#include "sys/thread.h"
#endif	/* linux */

struct sam_mount;			/* forward declaration, in mount.h */
struct sam_node;			/* forward declaration, in this file */

#include "sam/fs/ino.h"
#include "sam/fs/macros.h"
#include "sam/fs/ioblk.h"
#include "sam/quota.h"
#include "sam/fs/acl.h"
#include "sam/fs/share.h"
#include "sam/syscall.h"


/*
 * The number of SAM-QFS inodes and the size of the SAM-QFS inode
 * hash table are generally obtained from elsewhere (/etc/system
 * if set there, if not, then from ncsize (ninodes) or derived
 * from it (nhino)).  If, however, the default values are bogus,
 * we need some default-of-last-resort (DOLR) values to fall back on.
 */
#define	SAM_NINODES_MIN	(1024)		/* Min configurable number of inodes */
#define	SAM_NINODES_DEF	(32*1024)	/* DOLR inode count */
#define	SAM_NINODES_MAX	(4*1024*1024)	/* Max configurable number of inodes */
#define	SAM_NHINO_DEF	(512)		/* DOLR inode hash table size */
#define	SAM_NHINO_MAX	(1024*1024)	/* Max configurable hash size */

#define	SAM_STAGE_SIZE		(64*1024)
#define	SAM_CL_STAGE_SIZE	(2048*1024)

#define	SAM_SEGMENT_SHIFT	(20)

#ifdef sun
#define	SAM_ALL_COPIES		(-1)	/* Copy arg for sam_free_inode_ext */
#define	SAM_IHASH(ino, dev) (uint32_t) \
	(((uint32_t)((ino) + ((dev) & 0x0000ffff))) & (samgt.nhino - 1))
#endif

#ifdef	DEBUG
#define	SAM_VERIFY_INCORE_PAD(ip) \
	{					\
	ASSERT((ip)->pad0 == PAD0_CONTENTS); \
	ASSERT((ip)->pad1 == PAD1_CONTENTS); \
	}
#else	/* DEBUG */
#define	SAM_VERIFY_INCORE_PAD(ip)
#endif	/* DEBUG */

/*
 * ----- File identifier, overlays the fid_t structure in vfs.h
 */

typedef struct {
	union {
		int32_t	fid_pad;	/* Force correct byte alignment */
		struct {
			ushort_t len;	/* Length of data in bytes */
			sam_id_t id;	/* File unique identification */
		} _fid;
	} un;
} sam_fid_t;

typedef struct sam_extents {
	uint32_t extent[NOEXT];		/* Extent inode array */
	uchar_t	 extent_ord[NOEXT];	/* Extent info byte array */
} sam_extents_t;

#ifdef linux
typedef struct sam_indirect_extent_cache {
	struct sam_indirect_extent_cache *nextp;
	int ord;
	unsigned long long blkno;
	long bsize;
	char *bufp;
} sam_iecache_t;
#endif /* linux */


/*
 * ----- sam_rel_blks
 *	Forward link of blocks in mount tbl. Freed by the reclaim thread.
 */
typedef struct sam_rel_blks {
	offset_t length;
	sam_id_t id;
	sam_id_t parent_id;
	struct sam_rel_blks *next;
	int32_t lextent;
	ino_st_t status;
	sam_mode_t imode;
	sam_extents_t e;
} sam_rel_blks_t;


/*
 * ----- sam_inv_inos
 *	Forward link of nfs inodes in mount tbl. Inactivated by the
 *	reclaim thread.
 */
typedef struct sam_inv_inos {
	sam_id_t id;			/* i-number/generation num */
	struct sam_node *ip;		/* sam inode address */
	clock_t	entry_time;		/* entry time into queue */
	struct sam_inv_inos *next;
} sam_inv_inos_t;


/*
 * ----- sam_nchain_t is used to build a chain of inodes
 */

typedef struct sam_nchain {
	struct sam_nchain *forw;
	struct sam_nchain *back;
	struct sam_node *node;
} sam_nchain_t;


/*
 * ----- Hash flags field.  (Also used for free-list flags.)
 */

#define	SAM_HASH_FLAG_UNMOUNT	0x01	/* unmount/failover */
#define	SAM_HASH_FLAG_RMEDIA	0x02	/* start/stop rmedia */
#define	SAM_HASH_FLAG_LEASES	0x04	/* reestablish leases */
#define	SAM_HASH_FLAG_INACTIVATE 0x08	/* deferred inactive */


/*
 * ----- Lease flags field.
 */

#define	SAM_LEASE_FLAG_SERVER 0x01	/* server lease reclaim */
#define	SAM_LEASE_FLAG_CLIENT 0x02	/* client lease reclaim */
#define	SAM_LEASE_FLAG_RESET  0x04	/* client lease reset */

/*
 * -----	Inode file flags field.
 */

union ino_flags {
	struct {
	uint32_t
#if	defined(_BIT_FIELDS_HTOL)
	busy		:1,	/* File is busy (active) */
	free		:1,	/* File is in the free chain */
	hash		:1,	/* File is in the hash chain */
	ap_lease	:1,	/* Append lease has been granted for file */

	staging		:1,	/* File is staging in */
	unloading	:1,	/* Removable media is unloading */
	write_mode	:1,	/* File opened for write */
	inactivate	:1,	/* Defer inactivate if failover */

	purge_pend	:1,	/* Unavailable due to purge pending */
	stale		:1,	/* This is a stale inode entry, don't use it */
	rm_opened	:1,	/* Removable media file is opened */
	stage_n		:1,	/* Direct access - stage never */

	noreclaim	:1,	/* Inode is in use, don't reclaim resources */
	directio	:1,	/* Direct I/O on/off */
	stage_all	:1,	/* Associative stage enabled */
	fill3		:1,

	qwrite		:1,	/* Simultaneous writes - no data WRITER lock */
	nowaitspc	:1,	/* For stage, don't wait for disk space */
	stage_p		:1,	/* Stage in partial */
	stage_pages	:1,	/* File has been staged and has dirty pages */

	archive_w	:1,	/* File is waiting on archiving */
	abr		:1,	/* SAMAIO ABR (direct i/o B_ABRWRITE) set */
	fill5		:1,
	stage_directio	:1,	/* Stager turned on directio */

	arch_direct	:1,	/* Archiver using offline_copy_method=direct */
	hold_blocks	:1,	/* don't de-allocate blocks on truncate */
	positioning	:1,	/* Removable media is positioning */
	valid_mtime	:1,	/* Modify time is valid */

	dirty		:1,	/* File (inode) has dirty pages */
	updated		:1,	/* File (data) has been modified */
	accessed	:1,	/* File has been accessed */
	changed		:1;	/* File (inode) has been changed */

#else /* defined(_BIT_FIELDS_HTOL) */

	changed		:1,	/* File (inode) has been changed */
	accessed	:1,	/* File has been accessed */
	updated		:1,	/* File (data) has been modified */
	dirty		:1,	/* File (inode) has dirty pages */

	valid_mtime	:1,	/* Modify time is valid */
	positioning	:1,	/* Removable media is positioning */
	hold_blocks	:1,	/* don't de-allocate blocks on truncate */
	arch_direct	:1,	/* Archiver using offline_copy_method=direct */

	stage_directio	:1,	/* Stager turned on directio */
	fill5		:1,
	abr		:1,	/* SAMAIO ABR (direct i/o B_ABRWRITE) set */
	archive_w	:1,	/* File is waiting on archiving */

	stage_pages	:1,	/* File has been staged and has dirty pages */
	stage_p		:1,	/* Stage in partial */
	nowaitspc	:1,	/* For stage, don't wait for disk space */
	qwrite		:1,	/* Simultaneous writes, no data WRITER lock */

	fill3		:1,
	stage_all	:1,	/* Associative stage enabled */
	directio	:1,	/* Direct I/O on/off */
	noreclaim	:1,	/* inode in use, don't reclaim resources */

	stage_n		:1,	/* Direct access - stage never */
	rm_opened	:1,	/* Removable media file is opened */
	stale		:1,	/* stale inode entry, don't use it */
	purge_pend	:1,	/* Unavailable due to purge pending */

	inactivate	:1,	/* Defer inactivate if failover */
	write_mode	:1,	/* File opened for write */
	unloading	:1,	/* Removable media is unloading */
	staging		:1,	/* File is staging in */

	ap_lease	:1,	/* Append lease granted for file */
	hash		:1,	/* File is in the hash chain */
	free		:1,	/* File is in the free chain */
	busy		:1;	/* File is busy (active) */
#endif  /* defined(_BIT_FIELDS_HTOL) */
	} b;
	uint_t	bits;			/* File flag bits */
};

#define	SAM_BUSY	0x80000000	/* File is busy (active) */
#define	SAM_FREE	0x40000000	/* File is in the free chain */
#define	SAM_HASH	0x20000000	/* File is in the hash chain */
#define	SAM_AP_LEASE	0x10000000	/* Append lease has been granted */

#define	SAM_STAGING	0x08000000	/* File is staging in */
#define	SAM_UNLOADING	0x04000000	/* Removable media is unloading */
#define	SAM_WRITE_MODE	0x02000000	/* File opened for write */
#define	SAM_INACTIVATE	0x01000000	/* Defer inactivate if failover */

#define	SAM_PURGE_PEND	0x00800000	/* Unavailable due to purge pending */
#define	SAM_STALE	0x00400000	/* Stale inode entry, don't use it */
#define	SAM_RM_OPENED	0x00200000	/* Removable media file is opened */
#define	SAM_STAGE_N	0x00100000	/* Direct access - stage never */

#define	SAM_NORECLAIM	0x00080000	/* Inode in use, don't reclaim space */
#define	SAM_DIRECTIO	0x00040000	/* Direct I/O on/off */
#define	SAM_STAGE_ALL	0x00020000	/* Associative stage enabled */
#define	SAM_FILL3	0x00010000

#define	SAM_QWRITE	0x00008000	/* Simultaneous writes */
#define	SAM_NOWAITSPC	0x00004000	/* Don't wait for disk space */
#define	SAM_STAGE_P	0x00002000	/* Stage in partial */
#define	SAM_STAGE_PAGES	0x00001000	/* File has dirty stage pages */

#define	SAM_ARCHIVE_W	0x00000800	/* File is waiting on archiving */
#define	SAM_ABR		0x00000400	/* SAMAIO ABR (directI/O B_ABRWRITE) */
#define	SAM_FILL5	0x00000200
#define	SAM_STAGE_DIRECTIO 0x00000100	/* Stager turned on Direct I/O */

#define	SAM_ARCH_DIRECT	0x00000080	/* Archiver using stage_n (direct) */
#define	SAM_HOLD_BLOCKS	0x00000040	/* Don't free blocks on truncate */
#define	SAM_POSITIONING	0x00000020	/* Removable media file, positioning */
#define	SAM_VALID_MTIME	0x00000010	/* Modified time is valid */

#define	SAM_DIRTY	0x00000008	/* File (data) has been modified */
#define	SAM_UPDATED	0x00000004	/* File (data) has been modified */
#define	SAM_ACCESSED	0x00000002	/* File has been accessed */
#define	SAM_CHANGED	0x00000001	/* File (inode) has been changed */

#define	SAM_UPDATE_FLAGS (SAM_ACCESSED|SAM_UPDATED|SAM_CHANGED|SAM_DIRTY)
#define	SAM_INHIBIT_UPD (SAM_DIRTY|SAM_STAGING|SAM_STAGE_PAGES|SAM_VALID_MTIME)


/*
 * -----	Hash & Free chain head pointer structure.
 */

typedef struct sam_ihead	{
	struct sam_node	*forw;
	struct sam_node	*back;
} sam_ihead_t;

typedef struct sam_ichain	{
	sam_ihead_t	hash;
	sam_ihead_t	free;
} sam_ichain_t;

/*
 * -----	Incore inode structure.
 *  Most fields are protected by inode_rwl reader/writer lock.
 *
 *  The following fields are protected by a combination of inode_rwl
 *  and fl_mutex.  To modify these, either the inode_rwl must be held
 *  as WRITER, or the inode_rwl must be held as READER and the fl_mutex
 *  must be held.
 *    di.status
 *    di.{access|modify|change|creation|attributes}time
 *    flags
 *    no_opens
 *    rw_count
 *    mm_pages
 *    wmm_pages
 *	  cl_allocsz
 *	  size
 *    cl_closing
 *
 *  The following fields are protected by rm_mutex.
 *	rm_mutex also serializes shared-reader inode updates.
 *	  rm_cv
 *	  rm_wait
 *    stage_n_count
 *
 *  The following fields are protected by write_mutex.
 *    write_cv
 *    cnt_writes
 *    wr_errno
 *    wr_fini
 *    wr_thresh
 *    koffset
 *    klength
 *    pp->p_fsdata (for pages on which a write is in progress)
 *    eoo
 *
 *  The following field is protected by daemon_mutex.
 *    daemon_busy
 *
 *  The following fields are protected by the mount point's m_lease_mutex.
 *  Note that both the m_lease_mutex and ilease_mutex are required to modify
 *  the sr_leases field itself if the inode is linked into the lease chain.
 *    sr_leases
 *    sr_leases.lease_chain
 *    sr_leases.ip
 *    cl_lease_chain
 *    lease_flags
 *
 *  The following fields are protected by the ilease_mutex.
 *  Note that both the m_lease_mutex and ilease_mutex are required to modify
 *  the sr_lease field itself if the inode is linked into the lease chain.
 *    sr_leases (and all fields in its structure except for lease_chain & ip)
 *    cl_leases
 *    cl_saved_leases
 *    cl_short_leases
 *    cl_leasetime
 *    cl_leaseused
 *    cl_leasegen
 *    size_owner
 *    write_owner
 *    lease_cv
 *    getino_waiters
 *    waiting_getino
 *
 *  The following field is protected by the hash chain mutex for this inode.
 *    hash_flags
 *
 *  Partial lock orderings:
 *
 *    data_rwl > inode_rwl > fl_mutex
 *    inode_rwl > ihashlock > v_lock > write_mutex
 *    inode_rwl > ifreelock
 *    inode_rwl > m_lease_mutex > ilease_mutex
 *    inode_rwl > .inodes bp
 */

#ifdef sun
#define	PAD0_CONTENTS (0xfeedfacefeedfaceULL)
#define	PAD1_CONTENTS (0xfacefeedfacefeedULL)

typedef struct sam_node {
	sam_ichain_t	chain; /* Hash & Free chain forw/back, must be first */

	/*
	 * Reserved fields
	 */

	uint64_t		pad0;
	uint64_t		pad1;

	/*
	 * Beginning with S10, the vnode structure is no longer
	 * embedded within the inode but is dynamically allocated.
	 * Hence the pointer.
	 */
	struct vnode	*vnode;

	/*
	 * Start client specific fields.
	 */
	offset_t	cl_stage_size;	/* Valid client stage size */
	offset_t	cl_allocsz;	/* Maximum allocated size */
	offset_t	cl_pend_allocsz; /* Maximum pending allocated size */
	offset_t	cl_alloc_unit;	/* Current allocation unit */
	kmutex_t	cl_apmutex;	/* Append mtx, waits append threads */
	ushort_t	cl_leases;	/* Client leases */
	ushort_t	cl_saved_leases; /* Client leases saved before relinq */
	ushort_t	cl_short_leases; /* Leases marked for early expire */
	uchar_t		cl_hold_blocks;	/* blocks alloc'd & size not set */
	uchar_t		cl_flags;	/* client has marked inode */
	int		cl_locks;	/* Cnt of frlock issued on this file */
	int64_t		cl_leasetime[SAM_MAX_LTYPE];	/* Exp time for lease */
	uint32_t	cl_leaseused[MAX_EXPIRING_LEASES]; /* Ops using lease */
	uint32_t	cl_leasegen[SAM_MAX_LTYPE]; /* Gen # of lease */
	struct sam_nchain cl_lease_chain; /* Leases for this client */
	struct sam_cl_flock *cl_flock;	/* list of outstanding flocks */
	uint32_t	cl_attr_seqno;	/* Clnt seq no for last attr update */
	uint32_t	cl_ino_seqno;	/* Clnt seq no for last inode update */
	uint32_t	cl_ino_gen;	/* Clnt gen no for last inode update */
	uint32_t	cl_srvr_ord;	/* Server ord for last inode update */

	/*
	 * End client specific fields.
	 */

	/*
	 * Start server specific fields.
	 */

	int32_t		size_owner;	/* Ord of clnt controlling size */
	int32_t		write_owner; /* Ord of clnt controlling writes */
	uint32_t	sr_write_seqno;	/* Sequence number for writes */
	struct sam_lease_ino *sr_leases; /* entry in linked lease list. */
	kmutex_t	ilease_mutex;	/* Mutex for inode lease information */

	kthread_id_t	stage_thr;	/* Thread sending stage request */
	pid_t		rm_pid;		/* Pid for Removable media mount */
	pid_t		stage_pid;	/* Pid for stage daemon */
	pid_t		arch_pid[MAX_ARCHIVE];	/* Pids for sam-arcopy procs */
	unsigned short	arch_count;	/* Count of arcopy processes */

	/*
	 * End server specific fields.
	 */

	sam_time_t	updtime;	/* Time of last refresh, shared ino */

	struct sam_mount *mp;		/* Mount table pointer */
	char		copy;		/* Arch copy number to start search */
	char		daemon_busy;	/* Flag for daemon busy */
	dev_t		rdev;		/* Raw device for resource file */
	dev_t		dev;		/* Device for hash chain */
	sam_iomap_t	iom;		/* Last map descriptors */

	ushort_t	seqno;		/* Removable media sequence number */
	int		rm_err;		/* Removable media I/O returned error */
	int		io_count;	/* counter for rm, used in timeout */

	int32_t		mm_pages;	/* Count of memory mapped pages */
	int32_t		wmm_pages;	/* Cnt of mem mapped pages for write */
	caddr_t		stage_seg;	/* Segment offset -- stage write lock */
	sam_size_t	flush_len;	/* Flush write/stage length */

	offset_t	space;		/* Space left, if write rdev optical */
					/* device. */
					/* Section data size, if read optical */
					/* device. */
	offset_t	ra_off;		/* Next read ahead offset */

	offset_t	flush_off;	/* Flush write/stage offset */
	offset_t	koffset;	/* Kluster write offset */

	offset_t	page_off;	/* Last page offset */
	offset_t	real_stage_off;	/* Real stage offset */

	offset_t	real_stage_len;	/* Real stage length */
	caddr_t		lbase;
	caddr_t		rmio_buf;	/* Buffer for rm direct io  */

	sam_size_t	klength;	/* Kluster write length */
	struct sam_node *segment_ip; /* Seg ino pointer - only in base inode */
	uint_t		segment_ord; /* Segment ord - only in base inode */
	int		stage_n_count; /* Explicit stage_n outstanding */
					/*  read count. */
	int64_t		cnt_writes;	/* Cnt of outstanding bytes for write */
	int		wr_errno;	/* Errno for async page writes */
	uint32_t	rw_count; /* Cnt, explicit reads/writes in progress */
	int		no_opens;	/* Cnt of current opens */

	int		rm_wait;	/* Number waiting for stage/rm */
	int		accstm_pnd;	/* Num access time updates deferred */
	int		nfs_ios;	/* NFS I/Os have been processed */
	int		rd_consec;  /* num read consec. auto-qualified I/Os */
	int		wr_consec;  /* num write consec. auto-qualified I/Os */
	int		wr_thresh;  /* Wait cnt, outstanding write threshold */
	int		wr_fini;	/* Num waiting async writes to finish */
	int		seg_held;	/* Seg parent held by segment inode */
	int		getino_waiters;	/* Cnt of threads waiting on ino info */
	boolean_t	waiting_getino;	/* TRUE if NOTIFY_getino outstanding */
	short		stage_err;	/* Stage errno */

	uchar_t		hash_flags;	/* Flags for hash table scan */
	uchar_t		lease_flags;	/* Flags for lease chain scan */

	sam_ic_acl_t	*aclp;		/* Access control list (in core) */
	kmutex_t	listio_mutex;	/* Mutex for listio chain */
	struct sam_listio_call	*listiop; /* chain of outstanding listio */

	kcondvar_t	write_cv;	/* for write */
	kcondvar_t	daemon_cv;	/* for daemon request */
	kcondvar_t	rm_cv;		/* serializing rm file access */
	kcondvar_t	ilease_cv;	/* serializing ino lease */

	kmutex_t	write_mutex;	/* Mutex for write */
	kmutex_t	daemon_mutex;	/* Mutex for daemon request */
	kmutex_t	rm_mutex;	/* Mtx for ino rm_cv, covers rm_wait */
	kmutex_t	fl_mutex;	/* Mtx for ino flags */

	union ino_flags	flags;		/* File flags */
	krwlock_t	inode_rwl;	/* Inode contents lock */
	krwlock_t	data_rwl;	/* Inode data rw lock */
	void		*mt_handle;	/* Generic mount handle for daemon */

	dcanchor_t	i_danchor;	/* Dnlc 2nd Lvl cache hdl (VDIR only) */
	sam_time_t	ednlc_ft;	/* last ednlc failed time (VDIR only) */

	/* quota refs for growing file */
	struct sam_quot	*bquota[SAM_QUOTA_MAX];

	offset_t	stage_n_off;	/* Real stage offset for stage_n */
	offset_t	stage_off;	/* Current stage offset */
	offset_t	stage_len;	/* Current stage length */
	offset_t	stage_size;	/* Size of on-line portion of file */
	int64_t		st_umem_len;	/* Current memory alloc'd for st_buf */
	void		*st_buf;	/* Stage -n buf ptr for mem buffer */
	void		*st_cookie;	/* Stage -n cookie for mem buffer */

	offset_t	maxalloc;	/* Max alloc'd w/ 'hold_blocks' set */
	offset_t	dir_offset;	/* directory offset, if directory */

	offset_t	size;		/* Cur size, incrementing for stage */

	struct sam_disk_inode di;
	struct sam_disk_inode_part2 di2;

	int		cl_closing;	/* Client last close in progress */
	offset_t	zero_end;	/* last offset that was zeroed */
	offset_t	doff;		/* LQFS: inode block offset */
	uchar_t		dord;		/* LQFS: inode ordinal */
} sam_node_t;
#endif /* sun */


#ifdef linux
typedef struct sam_node {
	sam_ichain_t	chain; /* Hash & Free chain forw/back, must be first */

	struct inode	*inode;		/* Linux inode for this sam_node */
	kmutex_t	cl_iec_mutex;	/* Linux indirect extent cache mutex */
	sam_iecache_t	*cl_iecachep;	/* Linux indirect extent cache */

	/*
	 * Start client specific fields.
	 */
	offset_t	cl_allocsz;	/* Maximum allocated size */
	offset_t	cl_pend_allocsz; /* Maximum pending allocated size */
	offset_t	cl_alloc_unit;	/* Current allocation unit */
	kmutex_t	cl_apmutex;	/* Append mtx, waits append threads */
	ushort_t	cl_leases;	/* Client leases */
	ushort_t	cl_saved_leases; /* Clnt leases saved before relinq */
	ushort_t	cl_short_leases; /* Leases marked for early expire */
	uchar_t		cl_flags;	/* Set when client has marked inode */
	int		cl_locks;	/* Cnt of frlock issued on this file */
	int64_t		cl_leasetime[SAM_MAX_LTYPE]; /* Exp time for lease */
	uint32_t	cl_leaseused[MAX_EXPIRING_LEASES]; /* Ops using lease */
	uint32_t	cl_leasegen[SAM_MAX_LTYPE]; /* Gen # of lease */
	struct sam_nchain cl_lease_chain; /* Leases for this client */
	struct sam_cl_flock *cl_flock;	/* list of outstanding flocks */
	uint32_t	cl_attr_seqno;	/* Clnt seq no for last attr update */
	uint32_t	cl_ino_seqno;	/* Clnt seq no for last inode update */
	uint32_t	cl_ino_gen;	/* Clnt gen no for last inode update */
	uint32_t	cl_srvr_ord;	/* Server ord for last inode update */

	/*
	 * End client specific fields.
	 */

	/*
	 * Start server specific fields.
	 */

	int32_t		size_owner;	/* Ord of clnt controlling file size */
	kmutex_t	ilease_mutex;	/* Mtx for ino lease information */

	pid_t		stage_pid;	/* Pid for stage daemon */

	/*
	 * End server specific fields.
	 */

	sam_time_t	updtime;	/* Time of last refresh, shared ino */

	struct sam_mount *mp;		/* Mount table pointer */
	char		copy;		/* Arch copy number to start search */
	dev_t		rdev;		/* Raw device for resource file */
	dev_t		dev;		/* Device for hash chain */
	sam_iomap_t	iom;		/* Last map descriptors */

	int32_t		mm_pages;	/* Count of memory mapped pages */
	int32_t		wmm_pages;	/* Cnt of mem mapped pages for write */
	caddr_t		stage_seg;	/* Seg offset -- stage write lock */

	offset_t	space;		/* Space left, if write rdev optical */
					/* device. */
					/* Section data size, if read optical */
					/* device. */

	offset_t	real_stage_off;	/* Real stage offset */

	offset_t	real_stage_len;	/* Real stage length */
	struct sam_node *segment_ip; /* Seg ino - only in base inode */
	int		stage_n_count; /* Explicit stage_n outstanding */
					/*   read count. */
	int		no_opens;	/* Count of current opens */

	int		rm_wait;	/* Number waiting for stage/rm */
	int		nfs_ios;	/* NFS I/Os have been processed */

	short		stage_err;	/* Stage errno */

	uchar_t		lease_flags;	/* Flags for lease chain scan */

	kcondvar_t	rm_cv;		/* serializing rm file access */
	kmutex_t	rm_mutex;	/* Mtx for ino rm_cv, covers rm_wait */
	kmutex_t	fl_mutex;	/* Mtx for ino flags */

	union ino_flags	flags;		/* File flags */
	krwlock_t	inode_rwl;	/* Inode contents lock */
	krwlock_t	data_rwl;	/* Inode data rw lock */

	offset_t	stage_n_off;	/* Real stage offset for stage_n */
	offset_t	stage_off;	/* Current stage offset */
	offset_t	stage_len;	/* Current stage length */
	offset_t	stage_size;	/* Size of on-line portion of file */

	offset_t	size;		/* Cur size, incrementing for stage */

	struct sam_disk_inode di;
	struct sam_disk_inode_part2 di2;
	int		cl_closing;	/* Client last close in progress */
	offset_t	zero_end;	/* last offset that was zeroed */
} sam_node_t;
#endif /* linux */

enum sam_clear_ino_type {STALE_ARCHIVE, MAKE_ONLINE, PURGE_FILE};
typedef enum {SAM_TRUNCATE, SAM_REDUCE, SAM_RELEASE, SAM_PURGE} sam_truncate_t;

/*
 * ----		sam_operation_t - track open sam I/O operations
 *
 * One of these, allocated on the stack, is pointed to by the
 * thread-specific-data allocated for the FS whenever an operation
 * is pending on that FS.  If operations are nested, then only
 * the first is actually used.  The thread-specific-data is
 * zeroed on the thread's exit from the FS.
 */

struct hold_val {
	unsigned char depth;
	unsigned char flags;
	unsigned short module;
};

typedef union sam_operation {
	void *ptr;
	struct hold_val val;
} sam_operation_t;

/*
 * Flags for sam_operation above.
 * These flags are set in the first call of a thread to sam_open_operation()
 * or its equivalent, and live with the thread until its departure from the
 * samfs module.  This is used to ensure that things like worker threads
 * and NFS threads don't block in sam_freeze_ino(), since this would cause
 * problems.
 */
#define	SAM_FRZI_NOBLOCK	0x01	/* don't block this thread */
#define	SAM_FRZI_DRWLOCK	0x02	/* data_rwl lock held */
#define	SAM_FRZI_ALLOCD		0x04	/* struct dynamically alloc'd */

#define	SAM_OPERATION_INIT { 0 }

#if defined(_KERNEL)

/* Definitions of i-node list characteristics */
#define	SAM_ILIST_DEFAULT	0x00
#define	SAM_ILIST_PREALLOC	0x01

/*
 * -----	Macros for manipulating the inode.
 */

#ifdef linux
#define	SAM_SITOLI(ip)		((struct inode *)(ip)->inode)

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18))
#define	LI_I_PRIVATE	u.generic_ip
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18))
#define	LI_I_PRIVATE	i_private
#endif

#define	SAM_LITOSI(li)		((sam_node_t *)(void *)(li)->LI_I_PRIVATE)

#endif	/* linux */

#ifdef sun
#ifdef DEBUG
vnode_t *sam_itov(sam_node_t *ip);
sam_node_t *sam_vtoi(vnode_t *vp);

#define	SAM_ITOV(ip)		((vnode_t *)sam_itov(ip))
#define	SAM_VTOI(vp)		((sam_node_t *)sam_vtoi(vp))
#define	Sam_ITOV(ip)		((vnode_t *)(ip)->vnode)
#define	Sam_VTOI(vp)		((sam_node_t *)(void *)(vp)->v_data)
#endif	/* DEBUG */
#endif	/* sun */

#ifndef SAM_ITOV
#define	SAM_ITOV(ip)		((vnode_t *)(ip)->vnode)
#endif	/* SAM_ITOV */

#ifndef	SAM_VTOI
#define	SAM_VTOI(vp)		((sam_node_t *)(void *)(vp)->v_data)
#endif	/* SAM_VTOI */


/*
 * -----	sam_hash_ino - Put inode on head of the hashlist.
 *				Assumes lock set.
 */

#define	SAM_HASH_INO(hip, ip) \
{ \
	hip->back->chain.hash.forw = ip; \
	ip->chain.hash.forw = (sam_node_t *)(void *)hip; \
	ip->chain.hash.back = hip->back; \
	hip->back = ip; \
	ip->flags.b.hash = 1; \
}

/*
 * -----	sam_unhash_ino	- Remove inode from the hashlist.
 *				  Assumes lock set.
 */

#define	SAM_UNHASH_INO(ip) \
{ \
	(ip)->chain.hash.back->chain.hash.forw = (ip)->chain.hash.forw;  \
	(ip)->chain.hash.forw->chain.hash.back = (ip)->chain.hash.back;  \
	(ip)->chain.hash.forw = (ip); \
	(ip)->chain.hash.back = (ip); \
	(ip)->flags.b.hash = 0; \
}


/*
 * -----	sam_count_io - Increment i/o count for inode.
 */

#ifdef sun
#define	SAM_USE_ATOMICS 1	/* atomic => better performance */

#if SAM_USE_ATOMICS

#define	SAM_COUNT_IO(ip) atomic_add_32(&ip->rw_count, 1)

#else

#define	SAM_COUNT_IO(ip) \
{ \
	mutex_enter(&ip->fl_mutex); \
	ip->rw_count++; \
	mutex_exit(&ip->fl_mutex); \
}

#endif
#endif /* sun */

/*
 * ----- Get size atomically for VOP_GETATTR()
 */

#if	defined(_LP64)

#define	SAM_GET_ISIZE(resultp, ip) *(resultp) = (ip)->di.rm.size

#else	/* _LP64 */

#if	defined(sparc)

extern long long load_double(long long *);

#define	SAM_GET_ISIZE(resultp, ip) *(resultp) = load_double(&(ip)->di.rm.size)

#else	/* sparc */

#define	SAM_GET_ISIZE(resultp, ip) \
{ \
	RW_LOCK_OS(&(ip)->inode_rwl, RW_READER); \
	*(resultp) = (ip)->di.rm.size; \
	RW_UNLOCK_OS(&(ip)->inode_rwl, RW_READER); \
}

#endif	/* sparc */

#endif	/* _LP64 */

/*
 * ----- sam_uncount_io - Decrement i/o count for inode.
 */

#ifdef sun
#if SAM_USE_ATOMICS

#define	SAM_UNCOUNT_IO(ip) atomic_add_32(&ip->rw_count, -1)

#else

#define	SAM_UNCOUNT_IO(ip) \
{ \
	mutex_enter(&ip->fl_mutex); \
	ip->rw_count--; \
	mutex_exit(&ip->fl_mutex); \
}
#endif
#endif /* sun */

enum sam_sync_type {SAM_SYNC_ALL, SAM_SYNC_ONE, SAM_SYNC_PURGE};
typedef enum {IG_NEW, IG_EXISTS, IG_STALE, IG_SET, IG_DNLCDIR} sam_iget_t;

/*
 * ----- Inode function prototypes.
 */

#ifdef	linux

struct super_block;

int sam_get_ino(struct super_block *lsb, sam_iget_t flag, sam_id_t *fp,
		sam_node_t **ipp);
int sam_delete_ino(struct inode *li);
int sam_read_ino(struct sam_mount *mp, sam_ino_t ino, struct page **bpp,
		struct sam_perm_inode **pip);
struct page *sam_get_buf_header(void);
int sam_ihash(sam_ino_t ino, ulong_t dev);

int  sam_umount_ino(struct super_block *lsb, int forceflag);

#else /* linux */

int  sam_find_ino(vfs_t *vfsp, sam_iget_t flag, sam_id_t *fp,
		sam_node_t **ipp);
void sam_rele_ino(sam_node_t *ip);
void sam_rele_index(sam_node_t *ip);
int  sam_get_ino(vfs_t *vfsp, sam_iget_t flag, sam_id_t *fp, sam_node_t **ipp);
int  sam_put_ino(sam_node_t *ip);
int sam_ihash(sam_ino_t ino, dev_t dev);
void sam_put_freelist_head(sam_node_t *ip);
void sam_put_freelist_tail(sam_node_t *ip);
void sam_remove_freelist(sam_node_t *ip);
void sam_check_chains(struct sam_mount *mp);
int sam_create_ino(sam_node_t *pip, char *cp, struct vattr *vap,
	vcexcl_t ex, int mode, vnode_t **vpp, cred_t *credp, int filemode);
int sam_setattr_ino(sam_node_t *ip, vattr_t *vap, int flags, cred_t *credp);
int  sam_umount_ino(vfs_t *vfsp, int forceflag);
int  sam_delete_ino(vnode_t *vp);
int  sam_read_ino(struct sam_mount *mp, sam_ino_t ino, buf_t **bpp,
	struct sam_perm_inode **pip);
void sam_inactive_ino(sam_node_t *ip, cred_t *credp);
void sam_inactive_stale_ino(sam_node_t *ip, cred_t *credp);
int  sam_drop_ino(sam_node_t *ip, cred_t *credp);

typedef struct sam_ib_arg {
	enum sam_ib_cmd	cmd;
	int		ord;
	int		first_ord;
	int		new_ord;
} sam_ib_arg_t;
int  sam_proc_indirect_block(sam_node_t *ip, sam_ib_arg_t arg, int kptr[],
	int ik, uint32_t *extent_bn, uchar_t *extent_ord, int level, int *set);

int  sam_sync_inode(sam_node_t *ip, offset_t length,
	sam_truncate_t tflag);
int  sam_proc_truncate(sam_node_t *ip, offset_t length, sam_truncate_t tflag,
	cred_t *credp);
int  sam_space_ino(sam_node_t *ip, sam_flock_t *flp, int flag);
void sam_write_inode(struct sam_mount *mp, sam_node_t *ip);
int sam_get_fd(vnode_t *vp, int *fd);
buf_t *sam_get_buf_header(void);
void sam_free_buf_header(sam_uintptr_t *bp);
void sam_free_stage_n_blocks(sam_node_t *ip);
int sam_inactivate_ino(vnode_t *vp, int flag);
void sam_unhash_ino(sam_node_t *ip);
void sam_return_this_ino(sam_node_t *ip, int purge_flag);
int sam_alloc_inode_ext(sam_node_t *bip, mode_t mode, int count,
			sam_id_t *fid);
void sam_free_inode_ext(sam_node_t *bip, mode_t mode, int copy, sam_id_t *fid);
void sam_set_size(sam_node_t *ip);
void sam_destroy_ino(sam_node_t *ip, boolean_t locked);
int sam_trunc_excess_blocks(sam_node_t *ip);
int sam_refresh_shared_reader_ino(sam_node_t *ip, boolean_t writelock,
	cred_t *credp);

#endif	/* linux */

int sam_hold_vnode(sam_node_t *ip, sam_iget_t flag);
void sam_stale_inode(sam_node_t *ip);
int sam_check_cache(sam_id_t *fp, struct sam_mount *mp, sam_iget_t flag,
		sam_node_t *vip, sam_node_t **rip);
void sam_mark_ino(sam_node_t *ip, uint_t flags);
void sam_await_umount_complete(sam_node_t *ip);
int sam_open_operation(sam_node_t *ip);
int sam_open_ino_operation(sam_node_t *ip, krw_t lock_type);
void sam_open_operation_nb(struct sam_mount *mp);
void sam_open_operation_rwl(sam_node_t *ip);
int sam_idle_operation(sam_node_t *ip);
int sam_idle_ino_operation(sam_node_t *ip, krw_t lock_type);
#ifdef sun
int sam_set_operation_nb(struct sam_mount *mp);
void sam_unset_operation_nb(struct sam_mount *mp);
#endif

int sam_issue_object_io(sam_osd_handle_t oh, uint32_t command,
	uint64_t user_obj_id, enum uio_seg seg, char *data, offset_t offset,
	offset_t length);
int sam_create_priv_object_id(sam_osd_handle_t oh, uint64_t user_obj_id);
int sam_create_object_id(struct sam_mount *mp, struct sam_disk_inode *dp);
int sam_remove_object_id(struct sam_mount *mp, uint64_t user_obj_id,
	uchar_t unit);
int sam_truncate_object_file(sam_node_t *ip, sam_truncate_t tflag,
	offset_t size, offset_t length, cred_t *credp);

#ifdef sun
extern int sam_access_ino_ul(void *ip, int mode, cred_t *credp);
#endif
extern int sam_access_ino(sam_node_t *ip, int mode, boolean_t locked,
		cred_t *credp);

extern int sam_cv_wait_sig(kcondvar_t *cvp, kmutex_t *kmp);
extern int sam_cv_wait1sec_sig(kcondvar_t *cvp, kmutex_t *kmp);
extern int sam_check_sig(void);

int sam_verify_root(sam_node_t *ip, struct sam_mount *mp);
void sam_reset_default_options(sam_node_t *ip);
void sam_set_size(sam_node_t *ip);

void sam_create_ino_cache(void);
void sam_delete_ino_cache(void);

void sam_set_directio(sam_node_t *ip, int directio_flag);
void sam_set_abr(sam_node_t *ip);

offset_t sam_partial_size(sam_node_t *ip);
int  sam_clear_file(sam_node_t *ip, offset_t length,
	enum sam_clear_ino_type flag, cred_t *credp);
int  sam_clear_ino(sam_node_t *ip, offset_t length,
	enum sam_clear_ino_type flag, cred_t *credp);
int  sam_truncate_ino(sam_node_t *ip, offset_t length, sam_truncate_t tflag,
	cred_t *credp);

/*
 * ----- Ioctl function prototypes.
 */

int sam_ioctl_util_cmd(sam_node_t *ip, int cmd, int *, int *rvp,
			cred_t *credp);
int sam_ioctl_oper_cmd(sam_node_t *ip, int cmd, int *, int *rvp,
			cred_t *credp);
int sam_ioctl_dkio_cmd(sam_node_t *ip, int cmd, sam_intptr_t, int flag,
	cred_t *credp);
int sam_ioctl_sam_cmd(sam_node_t *ip, int, int *, int flag, int *rvp,
	cred_t *credp);
int sam_ioctl_file_cmd(sam_node_t *ip, int, int *, int flag, int *rvp,
	cred_t *credp);
int sam_proc_lockfs(sam_node_t *ip, int cmd, int *arg, int flag, int *rvp,
	cred_t *credp);


#endif /* _KERNEL */

#endif	/*  _SAM_FS_INODE_H */
