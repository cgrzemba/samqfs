/*
 *  utility.h - common definitions for sammkfs and samfsck.
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

#ifndef SAM_UTILITY_H
#define	SAM_UTILITY_H

#ifdef sun
#pragma ident "$Revision: 1.41 $"
#endif

/* Declaration/initialization macros. */
#undef CON_DCL
#undef DCL
#undef IVAL
#if defined(MAIN)
#define	CON_DCL
#define	DCL
#define	IVAL(v) = v
#else /* defined(MAIN) */
#define	CON_DCL extern const
#define	DCL extern
#define	IVAL(v) /* v */
#endif /* defined(MAIN) */

#if defined(_SAM_FS_BLOCK_H)
#define	SAMFS	0
#define	SAMMKFS	1
#define	SAMFSCK	2

struct devlist {
	upath_t eq_name;	/* Equipment device names */
	offset_t size;		/* Device size in bytes */
	offset_t blocks;	/* SAM logical blocks per device */
	offset_t iblk;		/* Current allocation map sector */
	uint64_t oh;		/* Object device handle */
	int fd;			/* Drive file descriptors table */
	int filemode;		/* Open filemode flags */
	off_t off;		/* Mmap off addr, only for sam{fsck,bcheck} */
	off_t length;		/* Mmap length - only for samfsck, sambcheck */
	char *mm;		/* Mmap area - only for samfsck, sambcheck */
	equ_t eq;		/* Filesystem equipment numbers */
	dtype_t type;		/* Device type */
	dstate_t state;		/* state- on/ro/idle/off/down/noalloc */
	ushort_t num_group;	/* Number of members in group */
};

typedef struct d_list {
	struct devlist	device[1];
} d_list_t;

typedef struct sam_dau_info {
	sam_dau_t	m_dau[SAM_MAX_DD];
} sam_dau_info_t;
typedef struct sam_dau_tbl {
	sam_dau_info_t	mi;
} sam_dau_tbl_t;

DCL char *program_name;			/* Program name: used by error */
DCL uname_t	fs_name;		/* Filesystem name */
DCL time_t fstime;			/* File system initialize time */
DCL struct d_list *devp;		/* Points to device list */
DCL struct sam_sblk sblock;		/* Superblock buffer */
DCL sam_dau_tbl_t	dau_tbl;	/* Table for dau parameters */
DCL sam_dau_tbl_t *mp IVAL(&dau_tbl);
DCL char *dcp;				/* Large block (DD) temp space */
DCL char *first_sm_blk;			/* First block of initial .inodes */
DCL int mm_count;			/* Cnt of meta devices in filesystem */
DCL int mm_ord;				/* Ordinal of meta device in fs */
DCL char *dir_blk;			/* Directory block */
DCL char *ibufp;			/* Large block (dd) temp space */

DCL char *daubuf;
DCL sam_daddr_t dausector IVAL(0);
DCL int dauord IVAL(-1);

DCL sam_daddr_t ibufsector IVAL(0);
DCL int ibuford IVAL(-1);

DCL int *bio_buffer;

DCL	int	verbose IVAL(0);

extern void sam_set_dau(sam_dau_t *dau, int lg_kblocks, int sm_kblocks);

int chk_devices(char *fs_name, int oflags, struct sam_mount_info *mp);
void close_devices(struct sam_mount_info *mp);
void get_mem(int exit_status);
int bfmap(int caller, int ord, int bits);
int cablk(int caller, int ord, int bits);
sam_offset_t getdau(int ord, int num, sam_daddr_t blk, int flag);
void writedau();
int write_sblk(struct sam_sblk *sbp, struct d_list *devp);
int sam_cmd_get_extent(struct sam_disk_inode *dp, offset_t offset, int sb_ver,
	int *de, int *dtype, int *btype, int kptr[], int *ileft);
void d_cache_init(int entries);
void d_cache_printstats(void);
int d_read(struct devlist *dp, char *buffer, int len, sam_daddr_t sector);
int d_write(struct devlist *dp, char *buffer, int len, sam_daddr_t sector);

int get_chunk(int fd, int eq, int *block, int *segment);
void update_sblk_to_40(struct sam_sblk *sbp, int fs_count, int mm_count);
int sam_bfmap(sam_caller_t caller, struct sam_sblk *sblk, int ord,
	struct devlist *dp, char *dcp, char *cptr, int bits);
int sam_cablk(sam_caller_t caller, struct sam_sblk *sblk, void *vp, int ord,
	int bits, int mbits, uint_t dd_kblocks, uint_t mm_kblocks, int *lenp);
void cmd_clear_maps(int caller, int ord, int len, sam_daddr_t blk, int bits);

#endif /* defined(_SAM_FS_BLOCK_H) */

int get_blk_device(struct sam_fs_part *fsp, int oflags);
int check_mnttab(char *fs_name);
void ChkFs(void);
char *MountCheckParams(struct sam_fs_info *mp);

#endif /* SAM_UTILITY_H */
