/*
 *	share.h - SAM-QFS shared file system protocol structs.
 *
 *	Type definitions for the SAM-QFS fs client/server communication.
 *
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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
#pragma ident "$Revision: 1.125 $"
#endif


#ifndef	_SAM_FS_SHARE_H
#define	_SAM_FS_SHARE_H

#ifndef linux
#include	<sys/vnode.h>
#include	<sys/acl.h>
#endif	/* linux */

#include	<sam/sys_types.h>
#include	"ino.h"


#define	SAM_SOCKET_NAME	"samsock"

#define	SAM_SOCKET_MAGIC	0x0001020304050607ULL	/* Magic number */

#define	SAM_HDR_LENGTH	STRUCT_RND64(sizeof (sam_san_header_t))

#define	SAM_SHARED_SERVER	1
#define	SAM_SHARED_CLIENT	2

/*
 * ----- Socket Errno.
 *
 * errno indicates disconnection or fatal error
 */
#define	SOCKET_FATAL(e) \
		((e) == EIO || (e) == EPIPE || (e) == ENOTCONN || \
		(e) == EOPNOTSUPP || (e) == EAFNOSUPPORT)


/*
 * ----- SAN major commands.
 */
#define	SAM_CMD_MOUNT		1
#define	SAM_CMD_LEASE		2
#define	SAM_CMD_NAME		3
#define	SAM_CMD_INODE		4
#define	SAM_CMD_BLOCK		5
#define	SAM_CMD_WAIT		6
#define	SAM_CMD_CALLOUT		7
#define	SAM_CMD_NOTIFY		8
#define	SAM_CMD_MAX			9


/*
 * ----- Common structs.
 */

/*
 * Shared file credentials.
 */
typedef struct sam_cred {
	uint32_t cr_ref;		/* reference count */
	uid_t	cr_uid;			/* effective user id */
	gid_t	cr_gid;			/* effective group id */
	uid_t	cr_ruid;		/* real user id */
	gid_t	cr_rgid;		/* real group id */
	uid_t	cr_suid;		/* "saved" user id (from exec) */
	gid_t	cr_sgid;		/* "saved" group id (from exec) */
	uint32_t cr_ngroups;		/* number of groups in cr_groups */
	gid_t	cr_groups[NGROUPS_MAX_DEFAULT];	/* supplementary group list */
} sam_cred_t;

/*
 * Shared file setattr struct.
 */
typedef struct sam_vattr {
	uint64_t	va_rsize;	/* Real file size in bytes */
	uint64_t	va_csize;	/* Current file size in bytes */
	uint64_t	va_stage_size;	/* Valid staged file size in bytes */
	uint64_t	va_nblocks;	/* # of blocks allocated */
	uint64_t	va_rdev;	/* Raw device */
	sam_mode_t	va_mode;	/* File access mode */
	uid_t		va_uid;		/* User id */
	gid_t		va_gid;		/* Group id */
	uint32_t	va_nlink;	/* Number of references to file */
	sam_timestruc_t	va_atime;	/* Time of last access */
	sam_timestruc_t	va_mtime;	/* Time of last modification */
	sam_timestruc_t	va_ctime;	/* Time file ``created'' */
	uint32_t	va_mask;	/* Bit-mask of attributes */
	uint32_t	va_type;	/* Vnode type (for create) */
	uint32_t	va_status;	/* Inode status attributes */
	char		va_pad[4];	/* Force pad */
} sam_vattr_t;

/*
 * Shared file file/record struct.
 */
typedef struct sam_share_flock {
	short	l_type;
	short	l_whence;
	char	l_remote;	/* Used to pass l_has_rmt pseudo-field */
	char	l_pad[3];	/* Force pad */
	int64_t	l_start;
	int64_t	l_len;		/* len == 0 means until end of file */
	int32_t	l_sysid;
	pid_t	l_pid;
} sam_share_flock_t;

/*
 * Directed actions.
 */
#define	SR_STALE_INDIRECT	0x0001	/* Stale indir blocks on client only */
#define	SR_DIRECTIO_ON		0x0002	/* Change I/O to direct */
#define	SR_SYNC_PAGES		0x0004	/* Sync pages */
#define	SR_INVAL_PAGES		0x0008	/* Sync and invalidate pages */
#define	SR_WAIT_LEASE		0x0010	/* Wait on lease expir/cancellation */
#define	SR_WAIT_FRLOCK		0x0020	/* Wait on frlock */
#define	SR_SET_SIZE		0x0040	/* Set size on the server */
#define	SR_NOTIFY_FRLOCK	0x0080	/* Notify frlock waiting on this clnt */
#define	SR_FORCE_SIZE		0x0100	/* Force size to inode size */
#define	SR_ABR_ON		0x0200	/* Set ABR mode for directio writes */

/*
 * Inode change flags.
 */
#define	IC_CHANGED	0x00000001	/* File (inode) has been changed */
#define	IC_ACCESSED	0x00000002	/* File has been accessed */
#define	IC_UPDATED	0x00000004	/* File (data) has been modified */

/*
 * Client shared file attributes. Updated on the server.
 */
typedef struct sam_cl_attr {
	uint32_t seqno;			/* Client attr sequence number */
	ushort_t actions;		/* Client directed action flags */
	char pad1[2];
	uint32_t iflags;		/* Incore inode flag bits */
	char pad[4];			/* Force pad */
	uint64_t real_size;		/* Real file size */
} sam_cl_attr_t;

/*
 * Server shared file attributes. Updated on the client.
 */
typedef struct sam_sr_attr {
	ushort_t actions;	/* Server directed action flags */
	ushort_t stage_err;	/* Stage error */
	uint32_t size_owner;	/* Ordinal of client setting file size */
	uint64_t current_size;	/* Current file size */
	uint64_t stage_size;	/* Stage file size */
	uint64_t alloc_size;	/* Allocated size */
	uint64_t offset;	/* Stale indirect blocks offset */
} sam_sr_attr_t;

/*
 * Server inode sequence struct.
 */
typedef struct sam_ino_instance {
	uint64_t seqno;		/* Inode sequence number */
	uint32_t ino_gen;	/* Unique num that matches this ino instance */
	uint32_t srvr_ord;	/* Server ordinal */
} sam_ino_instance_t;

/*
 * Server inode information record. Set on server and returned to client.
 */
typedef struct sam_ino_record {
	sam_ino_instance_t in;		/* Inode instance */
	sam_sr_attr_t sr_attr;		/* Inode attributes */
	sam_disk_inode_t di;		/* Inode - returned */
	sam_disk_inode_part2_t	di2;	/* Inode extra space - returned */
} sam_ino_record_t;


/*
 * ----- Mount Command
 */
enum MOUNT_operation {
	MOUNT_init	= 1,
	MOUNT_status	= 2,
	MOUNT_failinit	= 3,
	MOUNT_resync	= 4,
	MOUNT_failover	= 5,
	MOUNT_faildone	= 6,
	MOUNT_config	= 7,
	MOUNT_max_op	= 8
};

/*
 * Socket flags definitions for the Set{Server,Client}Socket calls.
 */
#define	SOCK_BYTE_SWAP		0x01
#define	SOCK_CLUSTER_HOST_UP	0x02	/* Active in the cluster */
#define	SOCK_CLUSTER_HOST	0x04	/* SunCluster exists on this host */

typedef struct sam_san_mount {
	sam_id_t id;		/* Place holder for trace messages */
	upath_t hname;		/* Client host name */
	uname_t fs_name;	/* File system name */
	uint32_t sock_flags;	/* Host socket flags */
	int32_t status;		/* Server mount fi_status bits -- returned */
	uint32_t config;	/* Server mount fi_config bits -- returned */
	uint32_t config1;	/* Server mount fi_config1 bits -- returned */
	char pad[4];
} sam_san_mount_t;


/*
 * ----- Lease Command
 */
enum LEASE_operation {
	LEASE_get	 = 1,
	LEASE_remove	 = 2,
	LEASE_reset	 = 3,
	LEASE_relinquish = 4,
	LEASE_max_op	 = 5
};

enum LEASE_type {
	LTYPE_read	= 0,
	LTYPE_write	= 1,
	LTYPE_append	= 2,
	LTYPE_truncate	= 3,
	LTYPE_frlock	= 4,
	LTYPE_stage	= 5,
	LTYPE_open	= 6,
	LTYPE_rmap	= 7,
	LTYPE_wmap	= 8,
	LTYPE_max_op	= 9
};

#define	SAM_MAX_LTYPE	LTYPE_max_op

#define	CL_READ		(1 << LTYPE_read)	/* Read lease */
#define	CL_WRITE	(1 << LTYPE_write)	/* Write lease */
#define	CL_APPEND	(1 << LTYPE_append)	/* Append lease */
#define	CL_TRUNCATE	(1 << LTYPE_truncate)	/* Truncate lease */
#define	CL_FRLOCK	(1 << LTYPE_frlock)	/* File/Record lock */
#define	CL_STAGE	(1 << LTYPE_stage)	/* Stage in process */
#define	CL_OPEN		(1 << LTYPE_open)	/* Outstanding open */
#define	CL_RMAP		(1 << LTYPE_rmap)	/* Read-mapping */
#define	CL_WMAP		(1 << LTYPE_wmap)	/* Write-mapping */

#define	CL_CLOSE	(CL_READ | CL_WRITE | CL_APPEND | CL_TRUNCATE | \
				CL_FRLOCK | CL_OPEN | CL_RMAP | CL_WMAP)

/* XXX - Should stage leases be non-expiring too? */

#define	SAM_NON_EXPIRING_LEASES (CL_TRUNCATE|CL_FRLOCK|CL_OPEN|CL_RMAP|CL_WMAP)
#define	SAM_DATA_MODIFYING_LEASES (CL_WRITE|CL_WMAP|CL_APPEND|CL_STAGE)

enum TRUNC_flag {			/* Flag for LTYPE_truncate */
	TRUNC_truncate	= 0,
	TRUNC_reduce	= 1,
	TRUNC_release	= 2,
	TRUNC_purge	= 3
};

/*
 * Shared volatile file mode bits
 */
typedef struct sam_shvfm_flags {
	uint16_t
#if defined(_BIT_FIELDS_HTOL)
		abr:		1,		/* abr enabled */
		directio:	1,		/* directio enabled */
		unused:		14;
#else /* defined(_BIT_FIELDS_HTOL) */
		unused:		14,
		directio:	1,		/* directio enabled */
		abr:		1;		/* abr enabled */
#endif
} sam_shvfm_flags_t;

typedef union sam_shvfm {
	sam_shvfm_flags_t	b;
	uint16_t		bits;
} sam_shvfm_t;

typedef struct sam_lease_data {
	uint16_t ltype;		/* Lease type (or mask, for relinquish) */
	uchar_t lflag;		/* TRUNC_flag if LTYPE_truncate */
	uchar_t sparse;		/* Allocate blocks for sparse file */
	int32_t filemode;	/* filemode flags, see file.h */
	int64_t offset;		/* Cur file offset, not used for truncate */
	int64_t resid;		/* File request length or truncate length */
	uint64_t alloc_unit;	/* Allocate this amount if expanding file */
	uint64_t zerodau[2];	/* Bit map for allocated DAUs if sparse set */
	int16_t no_daus;	/* num DAUs in zerodau bit map if sparse set */
	sam_shvfm_t shflags;	/* directio/abr bits */
	int32_t cmd;		/* Used only if file lock lease operation */
	char copy;	/* Copy for reset stage lease */
	char pad[3];
	uint32_t interval[MAX_EXPIRING_LEASES]; /* Lease expiration interval */
} sam_lease_data_t;

typedef struct sam_san_lease {
	sam_id_t id;			/* I-number/generation */
	sam_cl_attr_t cl_attr;		/* File attributes */
	sam_lease_data_t data;		/* Lease arguments for the operation */
	sam_share_flock_t flock;	/* File locking record */
	sam_cred_t cred;		/* Credentials */
	uint32_t gen[SAM_MAX_LTYPE];	/* Generation # of lease(s) */
	char	pad[4];			/* Force pad */
} sam_san_lease_t;

typedef struct sam_san_lease2 {
	sam_san_lease_t inp;		/* Lease input parameters */
	uint16_t granted_mask;		/* Leases granted by server */
	char pad[6];			/* Force pad */
	sam_ino_record_t irec;		/* Inode instance record */
} sam_san_lease2_t;


/*
 * ----- Name Command
 */
enum NAME_operation {
	NAME_create		= 1,
	NAME_remove		= 2,
	NAME_mkdir		= 3,
	NAME_rmdir		= 4,
	NAME_link		= 5,
	NAME_rename		= 6,
	NAME_symlink		= 7,
	NAME_acl		= 8,
	NAME_lookup		= 9,
	NAME_max_op		= 10
};

typedef struct sam_name_create {
	sam_vattr_t vattr;		/* Vnode attributes - supplied */
	int32_t	ex;			/* Exclusive create flag. */
	int32_t mode;			/* File mode */
	int32_t flag;			/* Large file create flag */
	char pad[4];			/* Force pad */
} sam_name_create_t;

typedef struct sam_name_remove {
	sam_id_t id;			/* Id of file to be removed */
} sam_name_remove_t;

typedef struct sam_name_link {
	sam_id_t id;
} sam_name_link_t;

typedef struct sam_name_rename {
	sam_id_t new_parent_id;	/* New parent id -- given */
	sam_id_t oid;		/* Old id, may be 0 */
	sam_id_t nid;		/* New id, may be 0 */
	int32_t osize;		/* Number of characters in old component */
	int32_t nsize;		/* Number of characters in new component */
} sam_name_rename_t;

typedef struct sam_name_mkdir {
	sam_vattr_t vattr;		/* Vnode attributes - supplied */
} sam_name_mkdir_t;

typedef struct sam_name_rmdir {
	sam_id_t id;			/* Id of directory to be removed */
} sam_name_rmdir_t;

typedef struct sam_name_symlink {
	sam_vattr_t vattr;	/* Vnode attributes - supplied */
	int32_t comp_size;	/* Number of characters in component */
	int32_t path_size;	/* Number of characters in symlink path */
} sam_name_symlink_t;

typedef struct sam_name_acl {
	int32_t mask;			/* Acl mask */
	int32_t aclcnt;			/* Number of acl entries */
	int32_t dfaclcnt;		/* Number of default acl entries */
	int32_t set;			/* set/get ACLs (1) | get ACLs (0) */
} sam_name_acl_t;

typedef struct sam_name_arg {
	union {
		sam_name_create_t	create;
		sam_name_remove_t	remove;
		sam_name_mkdir_t	mkdir;
		sam_name_rmdir_t	rmdir;
		sam_name_link_t		link;
		sam_name_rename_t	rename;
		sam_name_symlink_t	symlink;
		sam_name_acl_t		acl;
	} p;
} sam_name_arg_t;

typedef struct sam_san_name {
	sam_id_t parent_id;		/* Parent id -- given */
	sam_name_arg_t arg;		/* Name specific args for the op */
	sam_cred_t cred;		/* Credentials */
	char	component[8];		/* Ascii name(s) string */
} sam_san_name_t;

/*
 * The maximum size of this structure probably should be one value.
 * However, linux clients don't handle ACLs, while Solaris clients
 * do.  For now, define per OS.
 * This must be big enough to contain the buffer for the contents of a
 * symlink plus the component name.
 */
#ifdef linux
#define	SAM_MAX_NAME_LEN  \
	(sizeof (sam_san_name2_t) + MAXNAMELEN + QFS_MAXPATHLEN)
#endif
#ifdef sun
#define	SAM_MAX_NAME_LEN  \
	(sizeof (sam_san_name2_t) + 2 * MAX_ACL_ENTRIES * sizeof (aclent_t))
#endif

typedef struct sam_san_name2 {
	sam_id_t parent_id;	/* Parent id -- given */
	sam_id_t new_id;	/* New id -- returned */
	sam_ino_record_t prec;	/* Inode instance record */
	sam_ino_record_t nrec;	/* Inode instance record */
	sam_name_arg_t arg;	/* returned data info */
	char component[8];	/* returned acl (NAME_acl req only) */
} sam_san_name2_t;


/*
 * ----- Inode Command
 */
enum INODE_operation {
	INODE_getino		= 1,
	INODE_fsync_wait	= 2,
	INODE_fsync_nowait	= 3,
	INODE_setabr		= 4,
	INODE_setattr		= 5,
	INODE_stage		= 6,
	INODE_cancel_stage	= 7,
	INODE_samattr		= 8,
	INODE_samarch		= 9,
	INODE_samaid		= 10,
	INODE_putquota		= 11,
	INODE_max_op		= 12
};

typedef struct sam_inode_setattr {
	sam_vattr_t vattr;	/* Vnode attrs, supplied & returned updated */
	uint_t flags;		/* Utime flags */
	char pad[4];		/* Force pad */
} sam_inode_setattr_t;

typedef struct sam_inode_quota {
	int operation;		/* SAM_QOP_{PUT,PUTALL} */
	int type;		/* quota type (admin, group, user) */
	int index;		/* quota index */
	int len;		/* sizeof(sam_dquot_t) */
	sam_dquot_t quota;	/* quota record */
} sam_inode_quota_t;

typedef struct sam_inode_stage {
	uchar_t copy;			/* Stage this copy */
	char pad[7];			/* Force pad */
	int64_t	len;			/* Length to stage, if partial */
} sam_inode_stage_t;

typedef struct sam_inode_samattr {
	int32_t cmd;
	char pad[4];			/* Force pad */
	char ops[128];
} sam_inode_samattr_t;

typedef struct sam_inode_samarch {
	int operation;		/* Unarchive, damage or undamage operation */
	ushort_t flags;		/* Action flags */
	ushort_t media;		/* If not zero, media to affect */
	uchar_t copies;		/* Bit mask for copies to affect */
	uchar_t ncopies;	/* Number of copies */
	uchar_t dcopy;		/* 2nd copy */
	char pad[5];		/* Force pad */
	vsn_t vsn;		/* If not empty, VSN to affect */
} sam_inode_samarch_t;

typedef struct sam_inode_samaid {
	int operation;
	int32_t	aid;
} sam_inode_samaid_t;

typedef struct sam_inode_arg {
	union {
		sam_inode_setattr_t		setattr;
		sam_inode_stage_t		stage;
		sam_inode_samattr_t		samattr;
		sam_inode_samarch_t		samarch;
		sam_inode_samaid_t		samaid;
		sam_inode_quota_t		quota;
	} p;
} sam_inode_arg_t;

typedef struct sam_san_inode {
	sam_id_t id;		/* File id -- given */
	sam_cl_attr_t cl_attr;	/* File attributes */
	sam_inode_arg_t arg;	/* Ino specific arguments for the operation */
	sam_cred_t cred;	/* Credentials */
} sam_san_inode_t;

typedef struct sam_san_inode2 {
	sam_san_inode_t inp;	/* Inode input parameters */
	sam_ino_record_t irec;	/* Inode instance record */
} sam_san_inode2_t;


/*
 * ----- Block Command
 */
enum BLOCK_operation {
	BLOCK_getbuf	= 1,
	BLOCK_fgetbuf	= 2,
	BLOCK_getino	= 3,
	BLOCK_getsblk	= 4,
	BLOCK_vfsstat	= 5,
	BLOCK_wakeup	= 6,
	BLOCK_panic	= 7,
	BLOCK_quota	= 8,
	BLOCK_vfsstat_v2 = 9,
	BLOCK_max_op	= 10
};

typedef struct sam_block_getbuf {
	int ord;			/* File system ordinal */
	uint32_t blkno;			/* Blkno */
	int64_t bsize;			/* Block size */
	int64_t addr;			/* Address */
} sam_block_getbuf_t;

typedef struct sam_block_fgetbuf {
	int64_t offset;		/* File offset */
	int64_t bp;		/* Buffer pointer address passed & returned */
	int32_t len;		/* Block size */
	int32_t ino;		/* Inode owner of page */
} sam_block_fgetbuf_t;

typedef struct sam_block_getino {
	sam_id_t id;			/* Inode/Generation number */
	int64_t bsize;			/* Block size */
	int64_t addr;			/* Address */
} sam_block_getino_t;

typedef struct sam_block_sblk {
	int64_t bsize;			/* Block size */
	int64_t addr;			/* Address */
} sam_block_sblk_t;

typedef struct sam_block_vfsstat {
	int64_t capacity;		/* Capacity */
	int64_t space;			/* Space */
	int64_t mm_capacity;		/* Meta capacity */
	int64_t mm_space;		/* Meta space */
	int64_t fill[8];
} sam_block_vfsstat_t;

typedef struct sam_block_vfsstat_v2 {
	int64_t capacity;		/* Capacity */
	int64_t space;			/* Space */
	int64_t mm_capacity;		/* Meta capacity */
	int64_t mm_space;		/* Meta space */
	short	fs_count;		/* Number of family set members */
	short	mm_count;		/* Number of meta set members */
	int32_t unused;
	int64_t fill[7];
} sam_block_vfsstat_v2_t;

typedef struct sam_block_quota {
	int operation;			/* SAM_QOP_{GET,PUT,PUTALL} */
	int type;			/* admin, group, or user */
	int index;			/* admin ID, GID, or UID */
	int len;			/* sizeof(disk quota record) */
	int64_t buf;			/* quota record buffer address */
} sam_block_quota_t;

typedef struct sam_block_arg {
	union {
		sam_block_getbuf_t		getbuf;
		sam_block_fgetbuf_t		fgetbuf;
		sam_block_getino_t		getino;
		sam_block_sblk_t		sblk;
		sam_block_vfsstat_t		vfsstat;
		sam_block_vfsstat_v2_t		vfsstat_v2;
		sam_block_quota_t		samquota;
	} p;
} sam_block_arg_t;

typedef struct sam_san_block {
	sam_id_t id;			/* File id -- given  (1 if getbuf) */
	sam_block_arg_t arg;
	char	data[4];
	char pad[4];			/* Force pad */
} sam_san_block_t;

#define	SAM_MAX_BLOCK_LEN  \
	((sizeof (sam_san_block_t)) + INO_BLK_SIZE - 4)


/*
 * ----- Shared Client wait message. Used to wait for lease notify.
 */
enum WAIT_operation {
	WAIT_lease				= 1,
	WAIT_max_op				= 2
};

typedef struct sam_san_wait {
	sam_id_t id;			/* File id -- given */
	ushort_t ltype;			/* Lease type for LEASE_get */
} sam_san_wait_t;


/*
 * ----- Shared Server callout to clients.
 */
enum CALLOUT_operation {
	CALLOUT_action				= 1,
	CALLOUT_stage				= 2,
	CALLOUT_acl				= 3,
	CALLOUT_flags				= 4,
	CALLOUT_relinquish_lease		= 5,
	CALLOUT_max_op				= 6
};

typedef struct sam_callout_stage {
	uchar_t copy;			/* Stage this copy */
	uchar_t copy_mask;		/* Stage mask */
	char	pad1[2];		/* Force pad */
	uint32_t flags;			/* Inode flag bits - given */
	int32_t error;			/* Stage errno */
	char pad2[4];			/* Force pad */
} sam_callout_stage_t;

typedef struct sam_callout_relinquish_lease {
	uint16_t lease_mask;		/* Lease mask, for relinquish */
	char	pad[2];
	uint32_t timeo;			/* Lease timeo */
} sam_callout_relinquish_lease_t;

typedef struct sam_callout_arg {
	union {
		sam_callout_stage_t		stage;
		sam_callout_relinquish_lease_t	relinquish_lease;
	} p;
} sam_callout_arg_t;

typedef struct sam_san_callout {
	sam_id_t id;		/* File id -- given */
	sam_callout_arg_t arg;	/* Callout operation specific arguments */
	sam_ino_record_t irec;	/* Inode instance record */
} sam_san_callout_t;


/*
 * ----- Shared Server notify to clients.
 */
enum NOTIFY_operation {
	NOTIFY_lease		= 1,
	NOTIFY_lease_expire	= 2,
	NOTIFY_dnlc		= 3,
	NOTIFY_getino		= 4,
	NOTIFY_panic		= 5,
	NOTIFY_max_op		= 6
};

typedef struct sam_notify_dnlc {
#ifdef linux
	char component[NAME_MAX+1];
#else
	char component[MAXNAMELEN];	/* Component */
#endif	/* linux */
} sam_notify_dnlc_t;

typedef struct sam_notify_lease {
	uint32_t lease_mask;	/* Mask of leases being expired */
	char pad[4];		/* Force pad */
} sam_notify_lease_t;

typedef struct sam_notify_arg {
	union {
		sam_notify_dnlc_t		dnlc;
		sam_notify_lease_t		lease;
	} p;
} sam_notify_arg_t;

typedef struct sam_san_notify {
	sam_id_t id;		/* File id -- given */
	sam_notify_arg_t arg;	/* Notify operation specific arguments */
} sam_san_notify_t;


/*
 * ----- Shared file system client initiated commands.
 */
enum SHARE_flag {
	SHARE_wait_one	= 1,
	SHARE_wait	= 2,
	SHARE_nothr	= 3,
	SHARE_nowait	= 4,
	SHARE_flag_max	= 5
};

/* Begin: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack(4)
#endif

typedef struct sam_san_header {
	int64_t magic;		/* Magic for Shared file system */
	ushort_t unused;
	uchar_t wait_flag;	/* Wait/no wait/no thread */
	uchar_t originator;	/* Server or Client */
	ushort_t command;	/* Command */
	ushort_t operation;	/* Command operation */
	ushort_t length;	/* Length of call message */
	ushort_t out_length;	/* Length of returned call message */
	uint32_t seqno;		/* Seqno - identifies client message */
	uint32_t ack;		/* Message ack - identifies outgoing message */
	int32_t error;		/* Error - returned */
	int32_t client_ord;	/* Ordinal for shared client */
	int32_t server_ord;	/* Ordinal for shared server */
	int32_t hostid;		/* Client host unique id */
	int32_t fsid;		/* File system unique id (fsid + fs_gen) */
	int32_t fsgen;		/* Generation number for this file system */
	char reset_seqno;	/* Flag set by clnt for srvr to reset seqno */
	char pad[3];		/* pad to an 8 byte boundary */
} sam_san_header_t;

/* End: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack()
#endif


typedef struct sam_san_mount_msg {
	struct sam_san_header hdr;
	union {
		uint64_t		fill;
		sam_san_mount_t		mount;
	} call;
} sam_san_mount_msg_t;

typedef struct sam_san_lease_msg {
	struct sam_san_header hdr;
	union {
		uint64_t		fill;
		sam_san_lease_t		lease;
		sam_san_lease2_t	lease2;
	} call;
} sam_san_lease_msg_t;


typedef struct sam_san_name_msg {
	struct sam_san_header hdr;
	union {
		uint64_t		fill;
		sam_san_name_t		name;
		sam_san_name2_t		name2;
		char   i[SAM_MAX_NAME_LEN];
	} call;
} sam_san_name_msg_t;


typedef struct sam_san_inode_msg {
	struct sam_san_header hdr;
	union {
		uint64_t		fill;
		sam_san_inode_t		inode;
		sam_san_inode2_t	inode2;
	} call;
} sam_san_inode_msg_t;


typedef struct sam_san_block_msg {
	struct sam_san_header hdr;
	union {
		uint64_t		fill;
		sam_san_block_t		block;
	} call;
} sam_san_block_msg_t;


typedef struct sam_san_wait_msg {
	struct sam_san_header hdr;
	union {
		uint64_t		fill;
		sam_san_wait_t		wait;
	} call;
} sam_san_wait_msg_t;


typedef struct sam_san_callout_msg {
	struct sam_san_header hdr;
	union {
		uint64_t		fill;
		sam_san_callout_t	callout;
	} call;
} sam_san_callout_msg_t;


typedef struct sam_san_notify_msg {
	struct sam_san_header hdr;
	union {
		uint64_t		fill;
		sam_san_notify_t	notify;
	} call;
} sam_san_notify_msg_t;


typedef struct sam_san_message {
	struct sam_san_header hdr;
	union {
		uint64_t		fill;
		sam_san_mount_t		mount;
		sam_san_lease_t		lease;
		sam_san_lease2_t	lease2;
		sam_san_name_t		name;
		sam_san_name2_t		name2;
		sam_san_inode_t		inode;
		sam_san_inode2_t	inode2;
		sam_san_block_t		block;
		sam_san_wait_t		wait;
		sam_san_callout_t	callout;
		sam_san_notify_t	notify;
		char   i[SAM_MAX_NAME_LEN];
	} call;
} sam_san_message_t;

typedef struct sam_san_max_message {
	struct sam_san_header hdr;
	union {
		uint64_t		fill;
		sam_san_mount_t		mount;
		sam_san_lease_t		lease;
		sam_san_lease2_t	lease2;
		sam_san_name_t		name;
		sam_san_name2_t		name2;
		sam_san_inode_t		inode;
		sam_san_inode2_t	inode2;
		sam_san_block_t		block;
		sam_san_wait_t		wait;
		sam_san_callout_t	callout;
		sam_san_notify_t	notify;
		char   i[SAM_MAX_NAME_LEN];
		char   b[SAM_MAX_BLOCK_LEN];
	} call;
} sam_san_max_message_t;


#endif	/* _SAM_FS_SHARE_H */
