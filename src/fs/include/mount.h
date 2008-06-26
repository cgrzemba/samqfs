/*
 *	mount.h - mount table for the SAM file system.
 *
 *	Defines the structure of the mount table for this instance
 *	of the SAMFS filesystem mount. The vfs entry points to
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

#ifndef	_SAM_FS_MOUNT_H
#define	_SAM_FS_MOUNT_H

#ifdef sun
#pragma ident "$Revision: 1.146 $"
#endif

#ifdef sun
#include	<sys/pathname.h>
#include	<sys/thread.h>
#include	<sys/vfs.h>
#endif /* sun */

#include	<sam/types.h>
#include	<sam/syscall.h>
#include	<sam/mount.h>
#include	<sam/syscall.h>

struct sam_mount;

#include	"sam_thread.h"
#include	"block.h"
#include	"sblk.h"
#include	"scd.h"
#include	"client.h"


/*
 * ----- samdent is the logical device (partition) entry for a SAM filesystem.
 *	Ordered by ordinal sequence in sam_build_geometry.
 */

struct samdent {
	struct sam_fs_part part;	/* Equipment entry from mcf */
	kmutex_t	eq_mutex;	/* Mtx, maps for this ord */
	ushort_t
		opened:		1,	/* Device has been opened */
		busy:		1,	/* Device has outstanding block I/O */
		skip_ord:	1,	/* Secondary mem of striped group */
		abr_cap:	1,	/* supports App. Based Recovery */
		dmr_cap:	1,	/* supports Directed Mirror Reads */
		unused:		11;
	short	dt;			/* Device type */
	int	error;			/* Error getting device */
	int	map_empty;		/* No blocks left in device map */
	int	next_ord;		/* Next ordinal for this type */
	int	num_group;		/* Number of members in group */
	int	modified;		/* Number of blocks acquired */
	int	next_dau;		/* Next dau after I/O Outstanding */
	int32_t system;			/* First block for user data */
	uint32_t dev_bsize;		/* Device block size for r/m/w */
	uchar_t	command;		/* Partition allocation command */
	dev_t   dev;			/* Device number for partition */
#ifdef sun
	vnode_t *svp;			/* Vnode for special device */
	sam_osd_handle_t	oh;	/* osd_handle_t */
#endif /* sun */
#ifdef linux
	struct file	*fp;		/* Linux open file pointer */
#if (KERNEL_MAJOR > 4)
	struct block_device	*bdev;
#endif
#endif /* linux */
	struct sam_block	*block[SAM_MAX_DAU];
};

/*
 * item struct for m_sharefs queue
 * list_head and list_node_t are
 * pulled in from sam_thread.h
 */
#ifdef sun
typedef struct m_sharefs_item {
	list_node_t	node;
	mblk_t		*mbp;		/* Shared file system data pointer */
	uint64_t	count;
	clock_t		timestamp;	/* arrival time of packet on srvr */
} m_sharefs_item_t;
#endif

#ifdef linux
typedef struct m_sharefs_item {
	struct list_head	node;
	struct sk_buff		*mbp;
	uint64_t		count;
	uint64_t		timestamp; /* arrival time of packet on srvr */
} m_sharefs_item_t;
#endif

/*
 * -----	Mount table structure.
 * sam_mount_t is the mount table for a SAM filsystem.
 *
 * sam_mt_session is initialized at the SC_setmount system call.
 * sam_mt_instance is initialized at the mount system call.
 *
 *  The following fields are protected by the waitwr_mutex.
 *	  m_wait_write
 *	  m_syscall_cnt
 *	  mi.m_fs_syncing
 *	  mt.fi_status
 *	  mt.fi_config
 *	  mi.m_fs[ord].part.pt_state
 *	  sam_sblk:eq[ord].fs.command
 *	  sam_sblk:eq[ord].fs.sstate
 *
 *  The following fields are protected by the m_sr_ino_mutex.
 *    m_sr_ino_seqno
 *
 *  The following fields are protected by the seqno_mutex.
 *    m_rmseqno
 *
 *  The synclock mutex is held while syncing the file system.
 *
 *  The inode mutex is held while allocating or freeing inodes.
 *    (Note: This is a known bottleneck which should be improved some day.)
 *
 *  Partial lock orderings:
 *
 *    global_mutex > m_waitwr_mutex > m_synclock
 *    global_mutex > m_cl_mutex
 *    m_block.mutex > m_block.put_mutex
 *    m_block.mutex > m_waitwr_mutex
 *    m_cl_init > m_sharefs.mutex
 */
typedef struct sam_mt_session {
	struct sam_mount *m_mp_next; /* Next mount point entry, Null if none */
#ifdef sun
	client_entry_t	**m_clienti;	/* List of shared clnts, server only */
#endif
	struct sam_client_msg *m_cl_head; /* chain of outstanding messages */
	int64_t		m_vfs_time;	/* Time sblk last updated for vfsstat */
	int64_t		m_sblk_failed;	/* Time BLOCK_vfsstat fail, from srvr */
	int64_t		m_sblk_time;	/* Time sblk last updated */
	int		m_cl_server_wait; /* Num waiting for srvr operation */
#ifdef sun
	uint32_t	m_cl_active_ops; /* Number of outstanding operations */
#endif /* sun */
#ifdef linux
#ifdef __KERNEL__
	atomic_t	m_cl_active_ops;
#else
	int		m_cl_active_ops;
#endif

#endif /* linux */
	kmutex_t	m_cl_init;	/* Mtx, thread creation/termination */
	kmutex_t	m_cl_mutex;	/* Mtx, clnt cl_cv, covers cl_flags */
	kcondvar_t	m_cl_cv;	/* for client lease & inode requests */
	int		m_hostid;	/* Host identifier */
	upath_t		m_cl_hname;	/* Client host name */
	sam_sock_handle_t m_cl_sh;	/* Client socket file */
	kmutex_t	m_cl_wrmutex;	/* Mtx lock for write socket to srvr */
	uint32_t	m_cl_sock_flags; /* Sh clnt sock flags, from daemon */
	int		m_client_ord;	/* Shared Client ord, from SetServer */
	int		m_server_ord;	/* Shared Server ord,from SetServer */
	int		m_prev_srvr_ord; /* Sh prev srvr ord, from SetServer */
	int		m_maxord;	/* Max ords, host table, from daemon */
	int		m_involuntary;	/* Involuntary failover in progress */
	uint64_t	m_server_tags;	/* Server behavior tags */
	int64_t		m_resync_time;	/* Failover resync start time */
	int64_t		m_srvr_time;	/* Time of last message from server */
	uint64_t	m_sr_ino_seqno;	/* Ino seq number for Shared Server */
	uint32_t	m_sr_ino_gen;	/* Ino gen number for Shared Server */
	kmutex_t	m_sr_ino_mutex;	/* Mutex lock for shared fs seqno */
#ifdef sun
	kthread_t	*m_cl_thread;	/* Shared client rd_sock thread id */
#endif	/* sun */
#ifdef linux
	struct task_struct *m_cl_thread; /* Shared client rd_sock thread id */
#endif /* linux */
	uint32_t	m_clnt_seqno;	/* Client sequence number */
	int		m_max_clients;	/* Maximum array size currently */
	int		m_no_clients;	/* Current # of clients in shared fs */
	sam_thr_t	m_sharefs;	/* Pointer to shared fs thread info */
	kcondvar_t	m_waitwr_cv;	/* for write waiting */
	kmutex_t	m_waitwr_mutex;	/* Mutex lock for write waiting */
	kmutex_t	m_seqno_mutex;	/* Mutex lock for rm seqno */
	kmutex_t	m_synclock;	/* Sync lock for this mount */
	kmutex_t	m_xmsg_lock;	/* Xmsg lock for this mount */
	kmutex_t	m_inode_mutex;	/* Mutex lock for inode allocation */
	uint32_t	m_syscall_cnt;	/* count of syscalls with this mp */
	boolean_t	m_sysc_dfhold;	/* deferred VFS_HOLD for syscalls? */
	uint32_t	m_cl_nsocks;	/* number of connected sockets */
	boolean_t	m_cl_dfhold;	/* defer VFS_HOLD for sam-sharefsd? */
	kmutex_t	m_shared_lock;	/* coordinate sharedaemon wakeup */
	kcondvar_t	m_shared_cv;	/* cv for shared to sleep on */
	int		m_shared_event;	/* errno to pass to sharefsd */
#ifdef sun
	uint_t		m_tsd_key;	/* thread-specific data key */
	kmutex_t	m_fo_vnrele_mutex;
	kcondvar_t	m_fo_vnrele_cv;
	int			m_fo_vnrele_count;
	struct sam_event_em *m_fsev_buf; /* FS event buf for door callout */
#endif /* sun */
} sam_mt_session_t;

/*
 *
 * The following fields are protected by the m_waitwr_mutex (in the
 * associated sam_mt_session_t structure):
 *
 *   m_fs_syncing
 *
 * The following fields are protected by the m_xmsg_lock mutex:
 *
 *   m_xmsg_time
 *   m_xmsg_state
 *
 * The following fields are protected by the schedule_mutex (in samgt):
 *   m_schedule_flags
 *   m_schedule_count
 *
 */

typedef struct sam_mt_instance {
#ifdef sun
	struct sam_lease_head m_sr_lease_chain; /* list of leases (server) */
#endif
	struct sam_nchain m_cl_lease_chain; /* list of leases (client) */
	void		*m_fsact_buf;	/* FS activity buffer for arfind */
	sam_time_t	m_release_time;	/* Time releaser finished */
	uint64_t	m_umem_size;	/* Mem alloc'd for stage_n umem buf */
	offset_t	m_high_blk_count; /* HWM: Pt at which releaser strtd */
	offset_t	m_low_blk_count; /* LWH: FS low water mark */
#ifdef sun
	sam_time_t	m_xmsg_time;	/* Time of last WM transition msg */
	int		m_xmsg_state;	/* State at last transition message */
	struct vfs	*m_vfsp;	/* Pointer to vfs entry	*/
	struct vnode	*m_vn_root;	/* Vnode of root */
	int		m_maxphys;	/* Maximum I/O request size */
#endif /* sun */
#ifdef linux
	struct super_block *m_vfsp;	/* Pointer to Linux super_block */
	struct inode	*m_vn_root;	/* root inode */
#endif /* linux */
	struct sam_node	*m_inodir;	/* Inode of .inodes file */
	struct sam_node	*m_inoblk;	/* Inode of .blocks (hidden) file */
	struct sam_sblk	*m_sbp;		/* Incore superblock pointer */
	kmutex_t	m_sblk_mutex;	/* Superblock mutex for this mount */
	offset_t	m_prev_space;	/* Last ttl free blocks in family set */
	offset_t	m_prev_mm_space; /* Last total free blocks for mm */
	offset_t	m_inoblk_blocks; /* Num free blocks in .block file */
	int32_t		m_prev_state;	/* Last fsck state (bit 0) */
	int		m_fs_syncing;	/* Set if file system is syncing */
	uint_t		m_sblk_size;	/* Size of incore superblock */
	int		m_sblk_fsid;	/* Superblock time */
	int		m_sblk_fsgen;	/* Superblock generation */
	sam_bn_t	m_sblk_offset[2]; /* Superblock disk offsets */
	int		m_sblk_version;	/* Superblock version (<=3.5.0 == 1) */
	int		m_bn_shift;	/* Superblock extent shift to */
					/* SAM_DEV_BSHIFT (1024 byte block) */
	int		m_min_usr_inum;	/* Minimum user inode number */
	sam_bn_t	m_blk_bn;		/* Block for .blocks file */
	int		m_blk_ord;		/* Ord for .blocks file */
	int		m_no_blocks;	/* Number of times no free blocks */
	int		m_no_inodes;	/* Number of times no free inodes */
	int		m_wait_write;	/* Cnt of threads waiting on ENOSPC */
	int		m_wait_frozen;	/* Count of threads frozen */
	ushort_t	m_rmseqno;	/* Removable media sequence number */
	clock_t		m_fsfull;	/* Time file system last full */
	clock_t		m_fsfullmsg;	/* Time file system full msg issued */
	short		m_dk_start[SAM_MAX_DD]; /* Start for devs for DD/MM */
	short		m_dk_max[SAM_MAX_DD]; /* Count of devices for DD/MM */
	short		m_unit[SAM_MAX_DD]; /* Current unit for Round robin */
	sam_dau_t	m_dau[SAM_MAX_DD]; /* Dau setting, data and meta devs */
	sam_thr_t	m_inode;	/* Ino allocator&reclaim thread info */
#ifdef sun
	sam_thr_t	m_block;	/* Block allocator thread info */
#endif
	clock_t		m_blkth_ran;	/* Time block thread last ran */
	clock_t		m_blkth_alloc;	/* Time block thread found blocks */
	sam_prealloc_t	*m_prealloc;	/* Prealloc forward link request */
	struct sam_rel_blks *m_next;	/* Forward list of release blocks */
	struct sam_inv_inos *m_invalp;	/* Forward list of inos to invalidate */
	kmutex_t	m_lease_mutex;	/* Lease lock for this mount */
	/* <fs>/.quota_[agu] inodes */
	struct sam_node	*m_quota_ip[SAM_QUOTA_MAX];
	kmutex_t	m_quota_hashlock;
	struct sam_quot	**m_quota_hash;
	kmutex_t	m_quota_availlock;
	struct sam_quot	*m_quota_avail;
	uint32_t m_schedule_flags;	/* Flags for task scheduler */
	uint32_t m_schedule_count;	/* Count of scheduled tasks */
	uint32_t m_inval_count;		/* Count of inodes to invalidate */
#ifdef linux
	sam_thr_t	m_statfs;	/* Statfs thread info */
	struct proc_dir_entry *proc_ent; /* /proc entry for this fs */
#endif /* linux */
	struct ml_unit	*m_log;		/* LQFS: FS log info */
	kthread_id_t	m_ul_sbowner;	/* LQFS: SB lock owner */
	uint_t		m_vfs_domatamap; /* LQFS: Metadata map enabled */
	uint_t		m_vfs_nolog_si;	/* LQFS: No summary info */
	clock_t		m_vfs_iotstamp;	/* LQFS: Last I/O timestamp */
#ifdef sun
	sam_fb_pool_t m_fb_pool[SAM_MAX_DAU]; /* SM/LG free blk table */
#endif /* sun */
	struct samdent	m_fs[1];	/* Family set device array */
} sam_mt_instance_t;

typedef struct sam_mount {
	sam_mt_session_t	ms;	/* Initialized at SC_setmount */
	struct sam_fs_info	mt;	/* File system information */
	struct sam_fs_info	orig_mt; /* Original file system information */
	sam_mt_instance_t	mi;	/* Initialized at mount system call */
} sam_mount_t;


#if defined(_KERNEL)

/*
 * ----- macros
 */

#define	SAM_MP_IS_CLUSTERED(mp)				\
	((mp->ms.m_cl_sock_flags &			\
	(SOCK_CLUSTER_HOST_UP|SOCK_CLUSTER_HOST)) ==	\
		(SOCK_CLUSTER_HOST_UP|SOCK_CLUSTER_HOST))

/*
 * ----- mount function prototypes.
 */

int sam_mount_fs(sam_mount_t *mp);
int sam_set_mount(sam_mount_t *mp);
typedef enum {SAM_UNMOUNT_FS, SAM_FAILOVER_OLD_SRVR, SAM_FAILOVER_NEW_SRVR,
	SAM_FAILOVER_POST_PROCESSING} sam_unmount_flag_t;

int sam_build_allocation_links(sam_mount_t *mp, struct sam_sblk *sblk, int i,
	int *num_group_ptr);

#ifdef linux
void sam_cleanup_mount(sam_mount_t *mp, void *pn, cred_t *credp);
int sam_lgetdev(sam_mount_t *mp, struct sam_fs_part *fsp);
#endif

#ifdef sun
int sam_getdev(sam_mount_t *mp, int istart, int filemode, int *npartp,
	cred_t *credp);
int sam_unmount_fs(sam_mount_t *mp, int fflag, sam_unmount_flag_t flag);
int sam_flush_ino(vfs_t *vfsp, sam_unmount_flag_t flag, int fflag);
void sam_cleanup_mount(sam_mount_t *mp, pathname_t *pn, cred_t *credp);
void sam_close_devices(sam_mount_t *mp, int istart, int filemode,
	cred_t *credp);
void sam_report_initial_watermark(sam_mount_t *mp);
void sam_mount_setwm_blocks(sam_mount_t *mp);

int sam_open_osd_device(struct samdent *dp, int filemode, cred_t *credp);
void sam_close_osd_device(sam_osd_handle_t oh, int filemode, cred_t *credp);
int sam_get_osd_fs_attr(sam_mount_t *mp, sam_osd_handle_t oh,
	struct sam_sbord *sop);
#endif


/*
 * ----- sync function prototypes.
 */

int sam_update_filsys(sam_mount_t *mp, int flag);


#endif /* _KERNEL */

/*
 * MDS-mounted QFS file systems with superblock version V2 and above are
 * capable of journaling.
 */
#ifdef sun
#define	LQFS_CAPABLE(mp)	(((mp)->mi.m_sblk_version >= SAMFS_SBLKV2) && \
				    (((mp)->mt.fi_status & FS_CLIENT) == 0))
#else
#define	LQFS_CAPABLE(mp)	(0)
#endif /* sun */

/*
 * Macros to restrict the reference of journaling fields only to file
 * system version(s) that support journaling (V2 and above superblocks).
 * Otherwise, macros resolve to a no-op.  These macros also facilitate
 * keeping the LUFS and LQFS journaling code as similar as possible
 * for maintenance purposes.
 */
#define	LQFS_SET_LOGBNO(mp, val) \
		if (LQFS_CAPABLE((mp))) \
			(mp)->mi.m_sbp->info.sb.logbno = (val)
#define	LQFS_SET_FS_ROLLED(mp, val) \
		if (LQFS_CAPABLE((mp))) \
			(mp)->mi.m_sbp->info.sb.qfs_rolled = (val)
#define	LQFS_SET_LOGORD(mp, val) \
		if (LQFS_CAPABLE((mp))) \
			(mp)->mi.m_sbp->info.sb.logord = (val)
#define	LQFS_SET_FS_CLEAN(mp, val) \
		if (LQFS_CAPABLE((mp))) \
			(mp)->mi.m_sbp->info.sb.qfs_clean = (val)
#define	LQFS_SET_LOGP(mp, val) \
		if (LQFS_CAPABLE((mp))) \
			(mp)->mi.m_log = (val)
#define	LQFS_SET_SBOWNER(mp, val) \
		if (LQFS_CAPABLE((mp))) \
			(mp)->mi.m_ul_sbowner = (val)
#define	LQFS_SET_DOMATAMAP(mp, val) \
		if (LQFS_CAPABLE((mp))) \
			(mp)->mi.m_vfs_domatamap = (val)
#define	LQFS_SET_NOLOG_SI(mp, val) \
		if (LQFS_CAPABLE((mp))) \
			(mp)->mi.m_vfs_nolog_si = (val)
#define	LQFS_SET_IOTSTAMP(mp, val) \
		if (LQFS_CAPABLE((mp))) \
			(mp)->mi.m_vfs_iotstamp = (val)
#define	LQFS_GET_LOGBNO(mp) \
		(LQFS_CAPABLE((mp)) ? (mp)->mi.m_sbp->info.sb.logbno : 0)
#define	LQFS_GET_FS_ROLLED(mp) \
		(LQFS_CAPABLE(mp) ? \
			(mp)->mi.m_sbp->info.sb.qfs_rolled : FS_ALL_ROLLED)
#define	LQFS_GET_LOGORD(mp) \
		(LQFS_CAPABLE(mp) ? (mp)->mi.m_sbp->info.sb.logord : 0)
#define	LQFS_GET_FS_CLEAN(mp) \
		(LQFS_CAPABLE(mp) ? \
			(mp)->mi.m_sbp->info.sb.qfs_clean : FSCLEAN)
#define	LQFS_GET_LOGP(mp) \
		(LQFS_CAPABLE(mp) ? (mp)->mi.m_log : NULL)
#define	LQFS_GET_SBOWNER(mp) \
		(LQFS_CAPABLE(mp) ? (mp)->mi.m_ul_sbowner : 0)
#define	LQFS_GET_DOMATAMAP(mp) \
		(LQFS_CAPABLE(mp) ? (mp)->mi.m_vfs_domatamap : 0)
#define	LQFS_GET_NOLOG_SI(mp) \
		(LQFS_CAPABLE(mp) ? (mp)->mi.m_vfs_nolog_si : 0)
#define	LQFS_GET_IOTSTAMP(mp) \
		(LQFS_CAPABLE(mp) ? (mp)->mi.m_vfs_iotstamp : 0)

#endif  /* _SAM_FS_MOUNT_H */
