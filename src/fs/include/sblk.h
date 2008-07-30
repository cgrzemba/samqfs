/*
 * sblk.h - SAM-QFS file system label and superblock.
 *
 *	Label and Superblock for the SAM-QFS file system.
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

#ifndef	_SAM_FS_SBLK_H
#define	_SAM_FS_SBLK_H

#ifdef sun
#pragma ident "$Revision: 1.73 $"
#endif

typedef enum {SAMFS_CALLER, SAMMKFS_CALLER, SAMFSCK_CALLER} sam_caller_t;

/*
 * Length definitions.
 */
#define	L_LABEL		1024	/* Max length of label block information */
#define	L_SBINFO	256	/* Max length of superblock information */
#define	L_SBORD		64	/* Max length of superblock ordinal */
#define	OLD_SBSIZE	16	/* Superblk size (in 1024 blks) w/ 200 ords */

/*
 * Anchor locations.
 */
#define	LABELBLK	((sam_bn_t)(1))	/* Location of the label block */
#define	SUPERBLK	((sam_bn_t)(2))	/* Location of the super block */

/*
 * Special Reserved Inode Numbers for Block QFS.
 */
#define	SAM_INO_INO		(1)	/* I-num for .inodes file */
#define	SAM_ROOT_INO		(2)	/* I-num for root */
#define	SAM_BLK_INO		(3)	/* I-num for SM block map (no name) */
#define	SAM_HOST_INO		(4)	/* I-num for shared hosts (no name) */
#define	SAM_ARCH_INO		(5)	/* I-num for .archive directory */
#define	SAM_LOSTFOUND_INO	(6)	/* I-num for lost+found directory */
#define	SAM_STAGE_INO		(7)	/* I-num for .stage directory */
#define	SAM_SHFLOCK_INO		(8)	/* I-num reserved, but not used */

#define	SAM_MAX_MKFS_INO	(8)	/* Max block mkfs I-num */

/*
 * Special Reserved Inode Numbers for Object QFS - mat FS only.
 */
#define	SAM_OBJ_OBJCTL_INO	(26)	/* I-num for .objects file */
#define	SAM_OBJ_ORPHANS_INO	(27)	/* I-num for orphan list */
#define	SAM_OBJ_LBLK_INO	(28)	/* I-num for MDS label block */
#define	SAM_OBJ_SBLK_INO	(29)	/* I-num for MDS superblock */
#define	SAM_OBJ_PAR_INO		(30)	/* I-num for MDS default partition */
#define	SAM_OBJ_PAR0_INO	(31)	/* I-num for object partition 0 */
#define	SAM_OBJ_ROOT_INO	(32)	/* I-num for object root */

#define	SAM_OBJ_MIN_MKFS_INO	(26)	/* Min object mkfs I-num */
#define	SAM_OBJ_MAX_MKFS_INO	(32)	/* Max object mkfs I-num */

/*
 * Special Object IDs. Used to access special inodes in the "mat" file system.
 */
#define	SAM_OBJ_ORPHANS_ID	(((uint64_t)SAM_OBJ_ORPHANS_INO << 32) \
	| SAM_OBJ_ORPHANS_INO)

#define	SAM_OBJ_LBLK_ID (((uint64_t)SAM_OBJ_LBLK_INO << 32) | SAM_OBJ_LBLK_INO)
#define	SAM_OBJ_SBLK_ID	(((uint64_t)SAM_OBJ_SBLK_INO << 32) | SAM_OBJ_SBLK_INO)
#define	SAM_OBJ_PAR_ID	(((uint64_t)SAM_OBJ_PAR_INO << 32) | SAM_OBJ_PAR_INO)

/*
 * Minimum/Maximum inode number definitions.
 */
#define	SAM_MAX_PRIV_INO	(1024)	/* Max privileged I-num */
#define	SAM_MIN_USER_INO	(1025)	/* Min user inode I-num */
#define	SAM_MAX_PRIV_INO_VERS_1 (5)	/* Max privileged vers 1 I-num */
#define	SAM_MIN_USER_INO_VERS_1	(9)	/* Min user inode vers 1 I-num */

#define	SAM_PRIVILEGE_INO(vers, ino)	((((ino <= SAM_MAX_PRIV_INO) && \
	(ino != SAM_LOSTFOUND_INO) && (vers != SAMFS_SBLKV1)) || \
	(ino <= SAM_MAX_PRIV_INO_VERS_1)) &&	\
	(!((ino >= SAM_OBJ_OBJCTL_INO) && (ino <= SAM_OBJ_ROOT_INO))))

/*
 * Solaris has a 1Tb limit on device size in 32 bit kernels, and
 * in certain levels of Solaris. SAM-QFS currently has a 16 Tb limit
 * when using 4k block addressing.
 */
#define	SAM_1TB_LIMIT	(0x10000000000LL)
#define	SAM_16TB_LIMIT	(16LL*SAM_1TB_LIMIT)

/*
 * Superblock magic number definitions.
 */
#define	SAM_MAGIC_V1	0xfd187e20	/* Magic num for fs (3.5.0) */
#define	SAM_MAGIC_V2	0x76657232	/* Magic num for fs (4.0&5.0) */
#define	SAM_MAGIC_V2A	0x76653241	/* Magic num for fs (after add) */

/*
 * Reverse-endian superblock magic number definitions.
 */
#define	SAM_MAGIC_V1_RE	0x207e18fd	/* reverse endian V1 */
#define	SAM_MAGIC_V2_RE	0x32726576	/* reverse endian V2 */
#define	SAM_MAGIC_V2A_RE 0x41326576	/* reverse endian V2A */

#define	SAM_MAGIC	SAM_MAGIC_V2

/*
 * Superblock version definitions.
 */
#define	SAMFS_SBLKV1		1		/* Superblock version 1 */
#define	SAMFS_SBLKV2		2		/* Superblock version 2 */
#define	SAMFS_SBLK		SAMFS_SBLKV2
#define	SAMFS_SBLK_UNKNOWN	0		/* Unknown sblk version */

/*
 * Filesystem name definitions.
 */
#define	SAMFS_SB_NAME_STR	"SBLK"
#define	SAMFS_SB_NAME_LEN	4

#ifdef	BYTE_SWAP
typedef	struct	sam_fsid {
		int val[2];			/* file system id type */
} sam_fsid_t;
#else
typedef	fsid_t	sam_fsid_t;
#endif	/* BYTE_SWAP */

typedef struct sam_sbinfo {
	char		name[4];	/* 0: Identifier name: "SBLK" */
	uint32_t	magic;		/* 4: Magic number for file system */
	sam_time_t	init;		/* Time super block initialized */
	int32_t		ord;		/* Family set ord for this partition */
	uname_t		fs_name;	/* Family set name */
	sam_time_t	time;		/* Last super block update */
	int32_t		state;		/* fsck me (bit 0) */
	offset_t	inodes;		/* Start of inode file */
	offset_t	offset[2];	/* Superblock disk block offset */
	offset_t	capacity;	/* Total blocks in family set */
	offset_t	space;		/* Total free blocks in family set */
	ushort_t	dau_blks[2];	/* Data blocks per SM/LG dau */
	int32_t		sblk_size;	/* Sizeof superblk (incl partitions) */
	int32_t		fs_count;	/* Size of family set */
	short		da_count;	/* Size of data family set members */
	short		mm_count;	/* Size of meta family set members */
	offset_t	mm_capacity;	/* Ttl blks in meta data family set */
	offset_t	mm_space;	/* Ttl free blks, meta data */
					/*   family set. */
	ushort_t	meta_blks;	/* Blocks in meta dau, 3.5 and below */
	equ_t		eq;		/* FS equipment number */
	ushort_t	mm_blks[2];	/* Meta blks per SM/LB dau, */
					/*   3.5.1 & above. */
	offset_t	hosts;		/* Start of hosts file, 0 == shared */
	sam_fsid_t	fs_id;		/* unique, static FSID */
	ushort_t	ext_bshift;	/* basis of an extent shift */
					/* In 3.5.0 and below, SAM_DEV_BSHIFT */
					/* In 4.0, SAM_SHIFT */
	ushort_t	min_usr_inum;	/* Minimum user inode number */
	int32_t		gen;		/* Gen num for this file system */
	sam_time_t	repaired;	/* Last time fsck completed */
	ushort_t	opt_mask_ver;	/* Option mask version */
	ushort_t	hosts_ord;	/* dev ordinal of the hosts file */
	int32_t		opt_mask;	/* Option mask */
	dtype_t		fi_type;	/* Family set type, 5.0 and above */
	ushort_t	mm_ord;		/* Last mm ord for data device */
	u_longlong_t	logbno;		/* LQFS: log blkno */
	uint32_t	qfs_rolled;	/* LQFS: log fully rolled */
	ushort_t	logord;		/* LQFS: log ordinal */
	char		qfs_clean;	/* LQFS: detailed FS state flags */
	char		pad1;		/* Unused: Pad to 8-byte boundary */
	uint32_t	opt_features;	/* Opt features - backward compatible */
	uchar_t		pad2[4];	/* Unused: Pad to 8-byte boundary */
	uchar_t		fill2[56];	/* Unused: MAX size of sam_sbinfo_t */
} sam_sbinfo_t;

/*
 * LQFS: sam_sbinfo_t qfs_clean values.
 */
#define	FSACTIVE	((char)0)
#define	FSCLEAN		((char)0x1)
#define	FSSTABLE	((char)0x2)
#define	FSBAD		((char)0xff)	/* Mounted !FSCLEAN and !FSSTABLE */
#define	FSSUSPEND	((char)0xfe)	/* Temporarily suspended */
#define	FSLOG		((char)0xfd)	/* Logging FS */
#define	FSFIX		((char)0xfc)	/* Being repaired while mounted */

/*
 * LQFS: sam_sbinfo_t qfs_rolled values.
 */
#define	FS_PRE_FLAG	0
#define	FS_ALL_ROLLED	1
#define	FS_NEED_ROLL	3

/*
 * Request FSCK bits.  They come in two flavors -- generic
 * and specific.  The former are set whenever something that
 * is not traceable to a particular slice indicates that
 * something in the FS is amiss.  The latter is set iff
 * something in the FS indicates that there's a problem
 * with this particular slice.
 *
 * At this time, we'd like to track this info as much as
 * possible, since we don't necessarily know if the
 * specific information will be valuable, but would like
 * to make use of it if it is.  So we set the bits
 * separately for now, and fsck clears them all when
 * it runs.  This on the theory that someday it may
 * be possible to run fsck on individual slices.
 */
#define	SB_FSCK_GEN	0x1		/* Request FSCK */
#define	SB_FSCK_SP	0x2		/* Request FSCK of specific slice(s) */
#define	SB_FSCK_ALL	(SB_FSCK_SP|SB_FSCK_GEN)
#define	SB_FSCK_NONE	0x0		/* No FSCK needed */


/*
 * Option features: Used to display enabled features. These features are
 * optional and are backward compatible.
 */
#define	SBLK_FV1_MAPS_ALIGNED	0x00000001	/* FS has aligned maps */

#define	SBLK_MAPS_ALIGNED(sb)	((sb)->opt_features & SBLK_FV1_MAPS_ALIGNED)

/*
 * Superblock option mask version/inode and mask.
 *
 * Used for preventing damage to a file system by prohibiting its
 * mounting on versions of SAM-QFS which don't recognize the particular
 * options.
 */
#define	SBLK_OPT_VER1		0x1		/* Option mask version */

#define	SBLK_OPTV1_WORM		0x00000001	/* FS has WORM files */
#define	SBLK_OPTV1_SPARSE	0x00000002	/* FS has sparse files */
#define	SBLK_OPTV1_WORM_LITE	0x00000004	/* WORM lite */
#define	SBLK_OPTV1_WORM_EMUL	0x00000008	/* Emulation Mode */

#define	SBLK_OPTV1_EMUL_LITE	0x00000010	/* Emulation lite */
#define	SBLK_OPTV1_LG_HOSTS	0x00000040	/* Large host table */
#define	SBLK_OPTV1_CONV_WORMV2	0x00000080	/* Convert to WORM V2 */


/*
 * Superblock WORM macros.
 */
#define	SBLK_WORM(sb)		(((sb)->opt_mask & SBLK_OPTV1_WORM) != 0)
#define	SBLK_WORM_EMUL(sb)	(((sb)->opt_mask & SBLK_OPTV1_WORM_EMUL) != 0)
#define	SBLK_WORM_LITE(sb)	(((sb)->opt_mask & SBLK_OPTV1_WORM_LITE) != 0)
#define	SBLK_EMUL_LITE(sb)	(((sb)->opt_mask & SBLK_OPTV1_EMUL_LITE) != 0)

#define	SBLK_OPTV1_ALL_WORM (SBLK_OPTV1_WORM|SBLK_OPTV1_WORM_LITE|	\
					SBLK_OPTV1_WORM_EMUL|		\
					SBLK_OPTV1_EMUL_LITE)
#define	SBLK_OPTV1_STRICT_WORM	(SBLK_OPTV1_WORM|			\
					SBLK_OPTV1_WORM_EMUL)
#define	SBLK_OPTV1_LITE_WORM	(SBLK_OPTV1_WORM_LITE|			\
					SBLK_OPTV1_EMUL_LITE)
#define	SBLK_ALL_OPTV1	(SBLK_OPTV1_ALL_WORM|SBLK_OPTV1_SPARSE|	\
	SBLK_OPTV1_LG_HOSTS|SBLK_OPTV1_CONV_WORMV2)

#define	SBLK_UPDATE(sb, ip)						\
		((((sb)->opt_mask & SBLK_OPTV1_ALL_WORM) == 0) ||	\
			(((sb)->opt_mask & SBLK_OPTV1_WORM_LITE) &&	\
			((ip)->mp->mt.fi_config & MT_WORM)) ||		\
			(((sb)->opt_mask & SBLK_OPTV1_EMUL_LITE) &&	\
			((ip)->mp->mt.fi_config & MT_WORM_EMUL)))

/*
 * Superblock device entry.
 */
typedef struct sam_sbord {
	ushort_t	ord;		/* Equipment family set ordinal */
	equ_t		eq;		/* Equipment number for entry */
	uchar_t		state;		/* State -- on, off, down, noalloc */
	uchar_t		fsck_stat;	/* Fsck status */
	ushort_t	num_group;	/* Num members in striped group */
	offset_t	capacity;	/* No. of blocks on partition */
	offset_t	space;		/* No. of free blocks on partition */
	offset_t	allocmap;	/* Start of large allocation map */
	int32_t		fill2;
	int32_t		l_allocmap;	/* Number of blocks in large dau map */
	offset_t	dau_next;	/* Next free large dau number */
	offset_t	dau_size;	/* Total number of large daus */
	int32_t		system;		/* Total num blocks for system space */
	ushort_t	mm_ord;		/* Meta data ordinal */
	dtype_t		type;		/* device type */
} sam_sbord_t;

typedef struct sam_sblk {
	union {
		struct sam_sbinfo	sb;
		char	i[L_SBINFO];
	} info;
	union {
		struct sam_sbord	fs; /* Ord ary, dynamically assigned */
		char	i[L_SBORD];
	} eq[L_FSET];
} sam_sblk_t;

/*
 * Label block definition.
 */
#define	SAM_LABEL_MAGIC	0x53484162	/* Magic number for label block */

#define	SAM_LABEL_VERSION	6	/* label version # for sam-sharefsd */

typedef struct sam_label {
	char		name[4];	/* 0: Identifier name: "LBLK" */
	uint32_t	magic;		/* 4: Magic number for file system */
	sam_time_t	init;		/* time fs's superblock init'd */
	sam_time_t	update;		/* Time label block last touched */
	uint32_t	gen;		/* generation number from host file */
	uint32_t	version;	/* vers #; private to sam-sharefsd */
	uint32_t	serverord;	/* Server ordinal */
	upath_t		server;		/* Server name */
	upath_t		serveraddr;	/* Server IP name/address */
} sam_label_t;

typedef struct sam_label_blk {
	union {
		struct sam_label	lb;
		char	i[L_LABEL];
	} info;
} sam_label_blk_t;

/*
 * Superblock function prototypes.
 */
typedef struct sblk_args {
	offset_t	blocks;
	uint_t		kblocks[SAM_MAX_DD];
	int			ord;
	int			type;
} sblk_args_t;

void sam_init_sblk_dev(struct sam_sblk *sblk, sblk_args_t *args);

#endif	/* _SAM_FS_SBLK_H */
