/*
 * mount.h -  Mount information for the SAMFS file system.
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

#ifndef _SAMFS_MOUNT_H
#define	_SAMFS_MOUNT_H

#ifdef sun
#pragma ident "$Revision: 1.130 $"
#endif

#include <sam/types.h>
#include <sam/param.h>
#include <pub/devstat.h>
#include <sam/names.h>
#include <sam/attributes.h>

/* Begin: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack(4)
#endif

/*
 * File system information.
 *
 * Note: Be careful of rearranging the fields in this structure, as
 *       a partial bulk copy is done in src/fs/fsd/readconf.c:copyDefaults.
 *
 * The following fields are protected by the m_waitwr_mutex (in the
 * associated sam_mt_session_t structure):
 *
 *   fi_status
 *   fi_config
 *   fi_config1
 */
struct sam_fs_info {	/* File system information */
	uname_t	fi_name;	/* file system name */
	short	fs_count;	/* number of family set members */
	short	mm_count;	/* number of meta set members */
	uint32_t fi_config;	/* config flags (SETFLAG/CLEARFLAG/FLAG) */
	uint32_t fi_config1;	/* more config flags (mostly derived) */
	equ_t	fi_eq;		/* equipment number */
	dtype_t	fi_type;	/* device type */
	dstate_t fi_state;	/* state - on/ro/idle/off/down */
	int	fi_mflag;	/* vfs mount flags -- see mount.h */
	short	fi_sync_meta;	/* Flag to sync meta */
	short	fi_atime;	/* Flag to update, defer, no update atime */
	short	fi_stripe[SAM_MAX_DD];	/* Stripe width */
	ushort_t fi_high;	/* High water threshold */
	ushort_t fi_low;	/* Low water % threshold for releaser */
	long long fi_wr_throttle; /* High write byte count outstanding */
	long long fi_readahead;	/* Maximum readahead size */
	long long fi_writebehind; /* Maximum writebehind size */
	int64_t	fi_minallocsz;	/* Shared fs min allocate size */
	int64_t	fi_maxallocsz;	/* Shared fs max allocate size */
	int	fi_invalid;	/* Shared reader invalid cache time */
	int	fi_meta_timeo;	/* Shared fs stale cache attributes time */
	int	fi_lease_timeo;	/* Shared fs relinquish lease time */
	int	fi_nstreams;	/* No longer used, remove in future */
	int	fi_min_pool;	/* Minimum pool of sharefs threads */
	int	fi_retry;	/* Shared fs max # of mount retries */
	int	fi_lease[MAX_EXPIRING_LEASES];	/* Shared fs Rd/Wr/Ap */
						/*    lease time */
	int	fi_rd_ino_buf_size;	/* Size of ino buffer read size */
	int	fi_wr_ino_buf_size;	/* Size of ino buffer flush size */
	int	fi_partial;		/* Partial size in kilobytes */
	int	fi_maxpartial;		/* Max Partial size in kilobytes */
	int	fi_partial_stage; /* Partial size to start stage in kbytes */
	int	fi_flush_behind; 	/* Write flush behind in bytes */
	int	fi_stage_flush_behind;	/* Stage flush behind in bytes */
	uint	fi_stage_n_window;	/* Stage -n window size in bytes */
	int	fi_stage_retries; /* max stage retries for cksum stage err */
	int	fi_timeout;	/* Timeout for stage requests in fs */
	int	fi_dio_wr_consec; /* No. of consecutive qualified writes */
	int	fi_dio_wr_form_min;	/* write min. well-formed size */
	int	fi_dio_wr_ill_min;	/* write min. ill-formed size */
	int	fi_dio_rd_consec; /* No. of consecutive qualified reads */
	int	fi_dio_rd_form_min;	/* read min. well-formed size */
	int	fi_dio_rd_ill_min;	/* read min. ill-formed size */
	int	fi_def_retention;	/* Default retention period */
	/* Parameters below set only when mounted */
	int	fi_version;	/* File system version */
	uint32_t fi_status;	/* Status flags */
	upath_t	fi_mnt_point;	/* Full path to mount point */
	upath_t	fi_server;	/* Shared filesystem server hostname */
	fsize_t	fi_capacity;	/* Total bytes in filesystem */
	fsize_t	fi_space;	/* Total free bytes in filesystem */
	uint32_t fi_ext_bsize;	/* Extent block size */
};

/* End: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack()
#endif

/* Flags in fi_status */

#define	FS_MOUNTED	  0x00000001	/* Filesystem is mounted */
#define	FS_MOUNTING	  0x00000002	/* Filesystem is currently mounting */
#define	FS_UMOUNT_IN_PROGRESS 0x00000004 /* Filesystem is currently umounting */

#define	FS_SERVER	  0x00000010	/* Host is now metadata server */
#define	FS_CLIENT	  0x00000020	/* Host is not metadata server */
#define	FS_NODEVS	  0x00000040	/* Host can't be metadata server */
#define	FS_SAM		  0x00000080	/* Metadata server is running SAM */

#define	FS_LOCK_WRITE	  0x00000100	/* Lock write operations */
#define	FS_LOCK_NAME	  0x00000200	/* Lock name operations */
#define	FS_LOCK_RM_NAME	  0x00000400	/* Lock remove name operations */
#define	FS_LOCK_HARD	  0x00000800	/* Lock all operations */

#define	FS_SRVR_DOWN	  0x00001000	/* Server is not responding */
#define	FS_SRVR_BYTEREV	  0x00002000	/* Server has rev byte ordering */

#define	FS_SRVR_DONE	  0x00400000	/* Server finished failover */
#define	FS_CLNT_DONE	  0x00800000	/* Client finished resetting leases */

#define	FS_FREEZING	  0x01000000	/* Host is failing over */
#define	FS_FROZEN	  0x02000000	/* Host is frozen */
#define	FS_THAWING	  0x04000000	/* Host is thawing */
#define	FS_RESYNCING	  0x08000000	/* Server is resyncing */

#define	FS_RELEASING	  0x20000000	/* releasing is active on this fs */
#define	FS_STAGING	  0x40000000	/* staging is active on this fs */
#define	FS_ARCHIVING	  0x80000000	/* archiving is active on this fs */

/* Flags in fi_config */

#define	MT_SHARED_MO	  0x00000001	/* Shared file system - mntopts */
#define	MT_MH_WRITE	  0x00000002	/* Multiple host write access */
#define	MT_SAM_ENABLED	  0x00000004	/* Run archiving/standalone fs */
#define	MT_TRACE	  0x00000008	/* filesystem trace on/off */

#define	MT_QWRITE	  0x00000010	/* Multi-writer access on/off */
#define	MT_DIRECTIO	  0x00000020	/* Directio on */
#define	MT_SOFTWARE_RAID  0x00000040	/* Software raid used in this fs */
#define	MT_SHARED_WRITER  0x00000080	/* Write through meta data */

#define	MT_SHARED_READER  0x00000100	/* Get meta data from disk, no cache */
#define	MT_WORM		  0x00000200	/* WORM enabled */
#define	MT_SYNC_META	  0x00000400	/* Sync meta data */
#define	MT_NFSASYNC	  0x00000800	/* Override nfs sync */

#define	MT_OLD_ARCHIVE_FMT  0x00001000	/* No new style sparse archiving */
#define	MT_QUOTA	  0x00002000	/* Quotas enabled on this fs */
#define	MT_GFSID	  0x00004000	/* sticky, Non-dev-id FSID enabled */
#define	MT_HWM_ARCHIVE	  0x00008000	/* Start archiver if going > HWM */

#define	MT_SHARED_SOFT	  0x00010000	/* Shared fs is soft mounted */
#define	MT_SHARED_BG	  0x00020000	/* Shared fs is in background */
#define	MT_REFRESH_EOF	  0x00040000	/* Multi-reader refresh size at EOF */
#define	MT_ARCHIVE_SCAN	  0x00080000	/* arfind is enabled */

#define	MT_ABR_DATA	  0x00100000	/* ABR permitted on SAMAIO files */
#define	MT_DMR_DATA	  0x00200000	/* DMR permitted on SAMAIO files */
#define	MT_ZERO_DIO_SPARSE 0x00400000	/* Zero sparse files created by dio */
#define	MT_CONSISTENT_ATTR 0x00800000	/* Support consistent attributes */

#define	MT_WORM_LITE	  0x01000000	/* WORM Lite enabled */
#define	MT_WORM_EMUL	  0x02000000	/* WORM Emulation Mode */
#define	MT_EMUL_LITE	  0x04000000	/* WORM Emulation Lite */
#define	MT_CDEVID	  0x08000000	/* Use "made up" global dev for FS */

#define	MT_NOATIME	  0x10000000	/* Noatime option set */

/* Flags in fi_config1 */

#define	MC_SHARED_FS	0x00000001	/* Shared file system - mcf */
#define	MC_SHARED_MOUNTING 0x00000002	/* Mount is requesting socket setup */
#define	MC_MISMATCHED_GROUPS 0x00000008	/* Mismatched groups, cannot strip */

#define	MC_SMALL_DAUS	0x00000010	/* Small daus in this filesystem */
#define	MC_MR_DEVICES	0x00000020	/* mr device type exists in this fs */
#define	MC_MD_DEVICES	0x00000040	/* md device type exists in this fs */
#define	MC_STRIPE_GROUPS 0x00000080	/* Stripe groups exists in this fs */

#define	MC_CLUSTER_MGMT	0x00000100	/* cluster manages MDS switchovers */
#define	MC_CLUSTER_FASTSW 0x00000200	/* accelerate MDS switchover */
#define	MC_LOGGING	0x00000400	/* logging specified */
#define	MC_OBJECT_FS	0x00000800	/* Object storage file system */


/* Collection of fi_status flags which are failover flags */
#define	FS_FAILOVER (FS_FREEZING|FS_FROZEN|FS_THAWING)

/* Collection of fi_status flags which are lockfs flags */
#define	FS_LOCKFS (FS_LOCK_WRITE|FS_LOCK_NAME|FS_LOCK_RM_NAME|FS_LOCK_HARD)

/* Collection of fi_status flags which are shared fs flags */
#define	FS_FSSHARED (FS_SERVER|FS_CLIENT|FS_NODEVS|FS_SAM)

/* Collection of fi_config flags */
#define	MT_ALLWORM_OPTS (MT_WORM|MT_WORM_LITE|MT_WORM_EMUL|MT_EMUL_LITE)
#define	MT_ALLWORM (MT_WORM|MT_WORM_LITE)
#define	MT_ALLEMUL (MT_WORM_EMUL|MT_EMUL_LITE)
#define	MT_STRICT_WORM (MT_WORM|MT_WORM_EMUL)
#define	MT_LITE_WORM (MT_WORM_LITE|MT_EMUL_LITE)

/* WORM Macro's */
#define	WORM(ip) ((ip)->di.status.b.worm_rdonly != 0)
#define	WORM_MT_OPT(mp)	(MT_WORM & (mp)->mt.fi_config)
#define	WORM_LITE_MT_OPT(mp) (MT_WORM_LITE & (mp)->mt.fi_config)
#define	EMUL_MT_OPT(mp) (MT_WORM_EMUL & (mp)->mt.fi_config)
#define	EMUL_LITE_MT_OPT(mp) (MT_EMUL_LITE & (mp)->mt.fi_config)

/* Collection of fi_config1 flags which are being set in fsconfig.c */
#define	MC_FSCONFIG  \
	(MC_SHARED_FS|MC_STRIPE_GROUPS|MC_MR_DEVICES|MC_MD_DEVICES|MC_OBJECT_FS)

/* Begin: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack(4)
#endif

/*
 * Mount structure built by sam-fsd cmd and passed into filesystem.
 * Mount structure returned by the filesystem to the caller.
 */
struct sam_fs_part {
	upath_t		pt_name;	/* device name */
	equ_t		pt_eq;		/* equipment number */
	dtype_t		pt_type;	/* device type */
	dstate_t	pt_state;	/* state- on/ro/idle/off/down/noalloc */
	offset_t	pt_size;	/* size - if ram device */
	fsize_t		pt_capacity;	/* Total bytes in partition */
	fsize_t		pt_space;	/* Total free bytes in partition */
};

typedef struct sam_mount_info {
	struct sam_fs_info params;
	struct sam_fs_part part[L_FSET];
} sam_mount_info_t;

/*
 * Needed by some systems for
 * passing mount system call
 * file system specific data
 */

#define	GENERIC_SAM_MOUNT_INFO	0x01

typedef struct generic_mount_info {
	int type;
	int len;
	void *data;
} generic_mount_info_t;


typedef struct sam_fs_status {
	uname_t		fs_name;	/* file system name */
	equ_t		fs_eq;		/* equipment number */
	uint32_t	fs_status;	/* status from mount table */
	upath_t		fs_mnt_point;	/* Full path to mount point */
} sam_fs_status_t;

typedef struct sam_client_info {
	upath_t		hname;		/* client host name */
	uint32_t	cl_status;	/* mount status flags */
	uint32_t	cl_config;	/* mount config flags */
	uint32_t	cl_config1;	/* more mount config flags */
	uint32_t	cl_flags;	/* client status flags */
	int32_t		cl_nomsg;	/* count of outstanding messages */
	uint32_t	cl_min_seqno;	/* lowest seqno not completed  */
} sam_client_info_t;

/* End: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack()
#endif

#if defined(linux)
#define	CMD_UPDATE_MNTTAB
#endif /* defined(linux) */

#endif /* _SAMFS_MOUNT_H */
