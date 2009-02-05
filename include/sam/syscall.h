/*
 * syscall.h - system call definitions.
 *
 * Contains structures and definitions for system call commands.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#ifndef SAM_SYSCALL_H
#define	SAM_SYSCALL_H

#ifdef sun
#pragma ident "$Revision: 1.103 $"
#endif

#include "sam/types.h"
/* #include "samfs/types.h" */

#ifdef METADATA_SERVER
#include "license/license.h"
#endif	/* METADATA_SERVER */


/* Commands. */
typedef enum {
	SC_NULL = 0,
	SC_stat = 1,
	SC_lstat = 2,
	SC_vsn_stat = 3,
	SC_archive = 4,
	SC_release = 5,
	SC_stage = 6,
	SC_ssum = 7,
	SC_archive_copy = 8,
	SC_readrminfo = 9,
	SC_request = 10,
	SC_setfa = 11,
	SC_setcsum = 12,
	SC_cancelstage = 13,
	SC_segment_stat = 14,
	SC_segment_lstat = 15,
	SC_segment = 16,
	SC_quota = 17,		/* takes sam_quota_arg */
	SC_segment_vsn_stat = 18,
	SC_trace_info = 19,
	SC_trace_addr_data = 20, /* samfs_trace */
	SC_trace_tbl_data = 21,	/* an individual cpu's data */
	SC_trace_global = 22,	/* samgt global table */
	SC_trace_tbl_wait = 23,	/* an individual cpu's data (wait for data) */
	SC_projid = 24,		/* takes struct sam_projid_arg */
	SC_USER_MAX = 99,

	SC_FS_MIN = 100,
	SC_fsd = 100,
	SC_setmount = 101,	/* takes struct sam_mount_arg */
#ifdef METADATA_SERVER
	SC_setlicense = 102,	/* takes struct sam_license_arg */
	SC_getlicense = 103,	/* takes struct sam_license_arg */
#endif	/* METADATA_SERVER */
	SC_setfsparam = 104,	/* takes sam_setfsparam_arg */
	SC_setfsconfig = 105,	/* takes sam_setfsconfig */
	SC_getfsstatus = 106,	/* takes sam_get_fsstatus_arg_t */
	SC_getfsinfo = 107,	/* takes sam_get_fsinfo_arg_t */
	SC_getfspart = 108,	/* takes sam_get_fspart_arg_t */
	SC_fssbinfo = 109,	/* takes sam_fssbinfo_arg_t */
	SC_quota_priv = 110,	/* takes sam_quota_arg */
	SC_chaid = 111,		/* takes struct sam_chaid_arg */
	SC_set_server = 112,
	SC_set_client = 113,
	SC_share_mount = 114,
	SC_client_rdsock = 115,	/* Client read socket thread */
	SC_server_rdsock = 116,	/* Server read socket thread */
	SC_sethosts = 117,	/* write /.hosts file */
	SC_gethosts = 118,	/* read /.hosts file */
	SC_shareops = 119,	/* shared FS admin ops */
	SC_getfsinfo_defs = 120, /* takes sam_get_fsinfo_arg_t */
	SC_failover = 121,	/* Start voluntary failover */
	SC_setfspartcmd = 122,	/* takes sam_setfspartcmd arg */
	SC_getfsclistat = 123,	/* takes sam_getfsclistat_arg_t */
	SC_osd_device = 124,	/* takes sam_osd_dev_arg */
	SC_osd_command = 125,	/* takes sam_osd_cmd_arg */
	SC_osd_attr = 126,	/* takes sam_osd_attr_arg */
	SC_fseq_ord = 127,	/* takes sam_fseq_arg */
	SC_onoff_client = 128,	/* takes sam_onoff_client_arg */
	SC_change_features = 129, /* takes sam_change_features_arg */
	SC_FS_MAX = 149,

	SC_SAM_MIN = 150,
	SC_amld_call = 150,	/* takes sam_fs_fifo_t */
	SC_amld_quit = 151,	/* no args */
	SC_amld_resync = 152,	/* takes sam_resync_arg_t */
	SC_amld_stat = 153,	/* takes pointer to int */
	SC_setARSstatus = 154,	/* takes sam_setARS_arg */
	SC_fsmount = 155,	/* takes sam_fsmount_arg_t */
	SC_fsunload = 156,	/* takes sam_fsunload_arg_t */
	SC_position = 157,	/* takes sam_position_arg_t */
	SC_fsiocount = 158,	/* takes sam_fsiocount_arg_t */
	SC_fsinval = 159,	/* takes sam_fsinval_arg_t */
	SC_arfind = 160,	/* takes struct sam_arfind_arg (fs/arfind.h) */
	SC_stageall = 161,
	SC_stager = 162,	/* takes sam_stage_request_t */
	SC_fsstage = 163,	/* takes sam_fsstage_arg_t */
	SC_fsstage_err = 164,	/* takes sam_fsstage_arg_t */
	SC_fscancel = 165,	/* takes sam_fserror_arg_t */
	SC_fserror = 166,	/* takes sam_fserror_arg_t */
	SC_fsdropds = 167,	/* takes sam_fsdropds_arg_t */
	SC_get_san_ids = 168,	/* replacement for SC_get_filemap */
	SC_san_ops = 169,	/* replacement for SC_get_file */
	SC_store_ops = 170,	/* like SC_san_ops, but on an open file */
	SC_event_open = 171,	/* takes sam_event_open_arg (sam/samevent.h) */
	SC_SAM_MAX = 255
} SC_cmd;

/* Argument structures. */

#define	SAM_MAX_OPS_LEN		128

/* Returns from SC_amld_stat */
#define	SAM_AMLD_RUNNING	(0)
#define	SAM_AMLD_SHUT_DOWN	(1)
#define	SAM_AMLD_NOT_RESPONDING	(2)

/* Begin: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack(4)
#endif

struct sam_fileops_arg {
	SAM_POINTER(const char) path;
	SAM_POINTER(const char) ops;	/* File operation letters */
};

struct sam_request_arg {
	SAM_POINTER(const char) path;
	SAM_POINTER(struct sam_rminfo) buf;	/* Buffer for rm info */
	int bufsize;				/* Size of buffer */
};

struct sam_readrminfo_arg {
	SAM_POINTER(const char) path;
	SAM_POINTER(struct sam_rminfo) buf;	/* Buffer for rm info */
	int bufsize;				/* Size of buffer */
};

struct sam_stat_arg {
	SAM_POINTER(char) path;			/* Path name of file */
	SAM_POINTER(struct sam_stat) buf;	/* Buffer for info */
	int bufsize;
};

struct sam_vsn_stat_arg {
	SAM_POINTER(char) path;			/* Path name of file */
	SAM_POINTER(struct sam_section) buf;	/* Buffer for info */
	int bufsize;
	int copy;				/* Archive copy number */
};

struct sam_segment_vsn_stat_arg {
	SAM_POINTER(char) path;			/* Path name of file */
	SAM_POINTER(struct sam_section) buf;	/* Buffer for info */
	int bufsize;
	int copy;				/* Archive copy number */
	int segment;			/* segment index */
};

struct sam_archive_copy_arg {
	SAM_POINTER(const char) path;	/* Path name of file */
	int operation;			/* Unarchive, damage, or undamage */
					/* operation */
	ushort_t flags;			/* Action flags */
	ushort_t media;			/* If not zero, media to affect */
	uchar_t copies;			/* Bit mask for copies to affect */
	uchar_t ncopies;		/* Number of copies */
	uchar_t dcopy;			/* 2nd copy */
	uchar_t pad;
	vsn_t vsn;			/* If not empty, VSN to affect */
};

struct sam_set_csum_arg {
	SAM_POINTER(const char) path;	/* Path name of file */
	csum_t csum;
};

/*
 * Change the project ID of a file.
 */
struct sam_projid_arg {
	SAM_POINTER(const char) path;	/* Path name of file */
	uint32_t projid;		/* New project ID */
	uint32_t follow;		/* Follow if target is symlink */
};

/* Release record of candidates to be released */
struct sam_rel_file {
	sam_id_t id;	/* Unique identification: i-number/generation */
	int blocks;	/* Count of allocated blocks */
	sam_time_t time; /* MIN time accessed, modified, residency */
	int timetype;	/* Type of time: accessed, modified, residency */
	int priority;	/* Priority of entry */
};

typedef struct {			/* Invalidate cache for an rdev */
#if defined(__sparcv9) || defined(__amd64)
	dev32_t rdev;			/* Raw device */
#else /* __sparcv9 || __amd64 */
	dev_t rdev;			/* Raw device */
#endif /* __sparcv9 || __amd64 */
} sam_fsinval_arg_t;

typedef struct {			/* get superblock info */
	equ_t fseq;			/* Family set equipment number */
	SAM_POINTER(struct sam_sbinfo) sbinfo;	/* pointer to sam_sbinfo */
} sam_fssbinfo_arg_t;

struct sam_setARSstatus_arg {	/* set ARS status */
	uname_t as_name;	/* Filesystem name */
	enum ARS {
		ARS_set_a,		/* Set archiving */
		ARS_clr_a,		/* Clear archiving */
		ARS_set_r,		/* Set releasing */
		ARS_clr_r,		/* Clear releasing */
		ARS_set_s,		/* Set staging */
		ARS_clr_s,		/* Clear staging */
		ARS_set_sh,		/* Set shrinking */
		ARS_clr_sh		/* Clear shrinking */
	} as_status;
};

struct sam_setfsparam_arg {	/* set filesystem parameter */
	uname_t sp_fsname;	/* Filesystem name */
	int sp_offset;		/* offset of field in mount parameters */
	uint64_t sp_value;	/* value to set */
};

typedef struct {		/* release disk space */
	equ_t fseq;		/* Family set equipment number */
	sam_id_t id;		/* status */
	uint64_t freeblocks;	/* free space after release in blocks of 1K */
	int shrink;		/* sam-shrink: release partial if allocated */
} sam_fsdropds_arg_t;

struct sam_setfsconfig_arg {	/* set filesystem config parameter */
	uname_t sp_fsname;	/* Filesystem name */
	int sp_offset;		/* offset of field in mount parameters */
	int sp_mask;		/* Mask of fi_config in mount parameters */
	int sp_value;		/* Set for SETFLAG, zero for CLEARFLAG */
};

typedef struct {			/* resync fifo */
	sam_time_t seq;			/* time stamp for uniqueness */
	pid_t sam_amld_pid;		/* sam-amld's pid for validation */
} sam_resync_arg_t;

typedef enum {			/* Operation. */
	OP_NULL = 0,
	OP_unarchive = 1,	/* Unarchive archive copy */
	OP_damage = 2,		/* Damage archive copy */
	OP_undamage = 3,	/* Undamage archive copy */
	OP_rearch = 4,		/* Rearchive archive copy */
	OP_unrearch = 5,	/* Unrearchive archive copy */
	OP_exarchive = 6,	/* Exchange two archive copies */
	OP_verify = 7,		/* File requires verification of all copies */
	OP_verified = 8,	/* Copy has been verified */
	OP_MAX = 8
} OP_operation;

typedef enum {			/* Action flags. */
	SU_NULL = 0,
	SU_online = 1,		/* Stage file before operation */
	SU_force = 2,		/* OK to unarchive last copy (DEBUG only) */
	SU_archive = 4,		/* Rearchive copy after marking damaged */
	SU_meta = 8,		/* Only do meta */
	SU_MAX = 16
} SU_flags;

/* Parameter flags for cmd fifo and syscall. */

typedef enum {			/* Parameter flags. */
	PM_NULL = 0,
	PM_partial = 1,		/* Set Partial */
	PM_interval = 2,	/* Set invalidate cache interval */
	PM_MAX = 3
} PM_param;

struct sam_get_san_ids_arg {
	SAM_POINTER(char) path;
	SAM_POINTER(struct _FSVOLCOOKIE) vc;
	SAM_POINTER(struct _FSFILECOOKIE) fc;
};

struct sam_san_ops_arg {
	SAM_POINTER(struct _FSVOLCOOKIE) vc;
	SAM_POINTER(struct _FSFILECOOKIE) fc;
	uint64_t flags;
	uint64_t flen;
	uint64_t start;
	uint64_t slen;
	SAM_POINTER(struct _FSMAPINFO) buf;
	uint64_t buflen;
};

struct sam_quota_arg {
	uint32_t qcmd;
	uint32_t qflags;
	uint32_t qsize;		/* size of this struct (in) */
	uint32_t qtype;		/* arset, group, or user (in) */
	uint32_t qindex;	/* index of quota record (in/out) */
	uint32_t qfd;		/* fd for quota attributes (in) */
	SAM_POINTER(struct sam_dquot) qp;	/* quota record */
						/* (in (put)/out (get)) */
};

struct sam_fd_storage_ops_arg {
	int fd;
	uint64_t flags;
	uint64_t flen;
	uint64_t start;
	uint64_t slen;
	SAM_POINTER(struct _FSMAPINFO) buf;
	uint64_t buflen;
};

struct sam_get_fsstatus_arg {	/* Get filesystem status */
	int32_t	maxfs;		/* Max number of filesystems to return */
				/* - given */
	int32_t	numfs;		/* Number of configured filesystems */
				/* - returned */
	SAM_POINTER(struct sam_fs_status) fs;	/* Array of fs status */
};

struct sam_get_fsinfo_arg {	/* Get filesystem information */
	uname_t fs_name;	/* Family set name requested */
	SAM_POINTER(struct sam_fs_info) fi;	/* Filesystem information */
};

struct sam_get_fsclistat_arg {	/* Get filesystem client information */
	uname_t fs_name;	/* Family set name requested */
	int32_t	maxcli;		/* Max num of clients to return - given */
	int32_t	numcli;		/* Num of clients for filesystem - returned */
	SAM_POINTER(struct sam_client_info) fc;	/* Filesystem client info */
};

struct sam_get_fspart_arg {	/* Get filesystem partition data */
	uname_t fs_name;	/* Family set name requested */
	int32_t	maxpts;		/* Max num of partitions to return - given */
	int32_t	numpts;		/* Num of partitions in filesystem- returned */
	SAM_POINTER(struct sam_fs_part) pt;	/* Array of partitions */
};

struct sam_mount_arg {		/* Set/Get configured filesystem */
	uname_t fs_name;	/* Family set name requested */
	int32_t	fs_count;	/* Number of devices in the filesystem */
	SAM_POINTER(struct sam_mount_info) mt;	/* Mount table entry */
};

struct sam_set_host {		/* Set server/client */
	uname_t fs_name;	/* Family set name */
	upath_t server;		/* Server */
	uint32_t ord;		/* Server ordinal */
	uint32_t maxord;	/* Number of FS hosts */
	uint64_t server_tags;	/* server's behavior tags */
};

struct sam_share_arg {		/* Shared filesystem mount */
	uname_t fs_name;	/* Family set name */
	upath_t server;		/* Server */
	uint32_t config;	/* Mount flags */
	uint32_t config1;	/* More mount flags */
	uint32_t background;	/* Wait in kernel until socket established */
};


typedef enum {			/* Parameter command flags. */
	DK_CMD_null = 0,
	DK_CMD_noalloc = 1,	/* NoAlloc disk eq */
	DK_CMD_alloc = 2,	/* Alloc disk eq */
	DK_CMD_add = 3,		/* Add disk eq to grow file system */
	DK_CMD_remove = 4,	/* Remove disk eq to shrink file system */
	DK_CMD_release = 5,	/* Release disk eq to evacuate file system */
	DK_CMD_off = 6,		/* Change Noalloc disk to off */
	DK_CMD_max = 6
} DK_CMD_param;

struct sam_setfspartcmd_arg {	/* Set filesystem partition command */
	uname_t fs_name;	/* Family set name requested */
	int32_t eq;		/* Partition equipment number */
	int32_t command;	/* Command */
};


struct sam_syscall_rdsock {
	uname_t fs_name;	/* Family set name requested */
	int sockfd;		/* TCP connection to host/server */
	int hostord;		/* client host ordinal (server only) */
	uint32_t flags;		/* flags (see share.h, SOCK_*) */
	uint64_t tags;		/* client's behavior tags */
	upath_t hname;		/* Client name */
	SAM_POINTER(struct sam_san_max_message) msg; /* Message space */
};


#define	OSD_DEV_OPEN	1
#define	OSD_DEV_CLOSE	2

typedef struct sam_osd_dev_arg {	/* Filesystem osd device args */
	upath_t	osd_name;	/* OSD device name */
	sam_osd_handle_t oh;	/* OSD handle, returned--open/given--other */
	int32_t param;		/* OSD_DEV_OPEN or OSD_DEV_CLOSE */
	int32_t filemode;	/* Filemode for open & close */
} sam_osd_dev_arg_t;

#define	OSD_CMD_CREATE	1
#define	OSD_CMD_WRITE	2
#define	OSD_CMD_READ	3
#define	OSD_CMD_ATTR	4

typedef struct sam_osd_cmd_arg {	/* Filesystem osd command args */
	uname_t fs_name;	/* Mount point name */
	sam_osd_handle_t oh;	/* OSD handle */
	int32_t ord;		/* OSD ordinal, given */
	int32_t command;	/* Command - create, write, read, or get attr */
	int64_t obj_id;		/* User object id */
	int64_t offset;		/* Offset */
	int64_t size;		/* Size */
	SAM_POINTER(char) data; /* Data */
} sam_osd_cmd_arg_t;


#define	SAM_ONOFF_CLIENT_OFF	0
#define	SAM_ONOFF_CLIENT_ON	1
#define	SAM_ONOFF_CLIENT_READ	2

typedef struct sam_onoff_client_arg {	/* On / off client command args */
	uname_t fs_name;	/* Family set name */
	int32_t clord;		/* Client ordinal, [0...n] based */
	int32_t command;	/* Command - off, on, or read only */
	int32_t ret;		/* Old on/off value */
} sam_onoff_client_arg_t;


#define	SAM_CHANGE_FEATURES_ADD_V2A	1

typedef struct sam_change_features_arg {	/* Change features args */
	uname_t fs_name;	/* Family set name */
	int32_t command;	/* Command */
} sam_change_features_arg_t;


enum sam_ib_cmd {SAM_FREE_BLOCK, SAM_FIND_ORD, SAM_MOVE_ORD};

typedef struct {		/* inode on ord? */
	enum sam_ib_cmd cmd;
	equ_t fseq;		/* File system Family set equipment number */
	sam_id_t id;		/* Inode */
	equ_t eq;		/* Device Family set equipment number */
	ushort_t ord;	/* Device Family set ordinal number */
	int on_ord;		/* Set if this inode is on specified eq */
	int new_ord;	/* Device ordinal to move data to if specified */
} sam_fseq_arg_t;

#if defined(_SAM_FS_AMLD_H)

typedef struct sam_stage_arg {
	enum STAGER_cmd {
		STAGER_setpid = 1,	/* Set samgt.stager_pid */
		STAGER_getrequest = 2,	/* Get stage request from fs */
		STAGER_MAX
	} stager_cmd;
	union {
		pid_t pid;		/* stagerd'd pid */
		SAM_POINTER(sam_stage_request_t) request; /* stage request */
	} p;
} sam_stage_arg_t;

typedef struct sam_fsstage_arg {
	sam_handle_t handle;	/* file handle for stage */
	int directio;		/* directio directive */
	int ret_err;		/* error for stage */
} sam_fsstage_arg_t;

typedef struct sam_fsiocount {
	sam_handle_t handle;	/* file handle for io count */
} sam_fsiocount_arg_t;

typedef struct {		/* Mount response */
	sam_handle_t handle;	/* FS handle data */
	SAM_POINTER(struct sam_resource) resource;	/* Resource record */
	offset_t space;		/* Space (bytes) left on media */
#if defined(__sparcv9) || defined(__amd64)
	dev32_t rdev;		/* Raw device */
#else	/* __sparcv9 || __amd64 */
	dev_t rdev;		/* Raw device */
#endif	/* __sparcv9 || __amd64 */
	int fd;			/* File descriptor for opened device */
	SAM_POINTER(void) mt_handle;	/* Generic pointer for daemon */
	char fifo_name[32];
	int ret_err;		/* Errno from mount request */
} sam_fsmount_arg_t;

typedef struct {		/* Eof for buffered io */
	sam_handle_t handle;	/* FS handle data */
	offset_t size;		/* Size of data written */
	int ret_err;		/* Errno */
} sam_fsbeof_arg_t;

typedef struct {		/* Unload response */
	sam_handle_t handle;	/* FS handle data */
	uint64_t position;	/* Position on media (if not 0) */
	int ret_err;		/* Errno */
} sam_fsunload_arg_t;

typedef struct {		/* Error response */
	sam_handle_t handle;	/* FS handle data */
	int access;		/* Access for file */
	int ret_err;		/* Errno */
} sam_fserror_arg_t;


typedef struct {			/* Removable media position response */
	sam_handle_t	handle;		/* FS handle data */
	u_longlong_t	position;	/* Position on media (if not 0) */
	int		ret_err;	/* Errno */
} sam_position_arg_t;

#endif /* defined(_SAM_FS_AMLD_H) */

struct sam_chaid_arg {
	SAM_POINTER(const char) path;	/* Path name of file */
	uint32_t admin_id;		/* new admin ID */
	uint32_t follow;		/* follow if target is symlink */
};

/*
 * File system call daemons (FSCD) - daemons that wait on an
 * event from the filesystem - are listed below. Each daemon has a
 * sam_syscall_daemon structure in the mount table.
 */
enum FSCD_daemons {
	FSCD_dummy   = 0,
	FSCD_MAX
};

/*
 * File system call argument - system call arguments.
 */
typedef struct sam_fscd_arg {
	uname_t fs_name;		/* File system name */
	int32_t fscdi;			/* File system call index */
	int32_t fscd_size;		/* Size of returned command */
	SAM_POINTER(void) fsc;		/* Pointer to command */
} sam_fscd_arg_t;

typedef struct sam_sgethosts_arg {
	uname_t fsname;			/* file system name */
	int32_t newserver;		/* Set if changing server */
	int32_t size;			/* must match length of hosts file */
	SAM_POINTER(const char) hosts;	/* contents of hosts file */
} sam_sgethosts_arg_t;

typedef struct sam_shareops_arg {
	uname_t fsname;		/* file system name */
	int32_t op;
	int32_t	host;
} sam_shareops_arg_t;

/* End: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack()
#endif

#ifdef METADATA_SERVER
struct sam_license_arg {	/* Set/Get license */
	sam_license_t_33 value;	/* License value */
};
#endif	/* METADATA_SERVER */

#endif /* SAM_SYSCALL_H */
