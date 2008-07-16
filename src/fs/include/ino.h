/*
 *	ino.h - SAM-QFS file system disk inode definitions.
 *
 *	Defines the structure of disk inodes for the SAM file system.
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

#ifndef	_SAM_FS_INO_H
#define	_SAM_FS_INO_H

#ifdef sun
#pragma ident "$Revision: 1.64 $"
#endif

#ifdef linux
#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <sys/types.h>
#endif /* __KERNEL__ */
#include <sam/types.h>
#include <sam/param.h>
#include <sam/resource.h>
#include <sam/fs/rmedia.h>

#else /* linux */
#include <sys/types.h>
#include <sam/types.h>
#include <sam/param.h>
#include <sam/resource.h>
#include <sam/fs/rmedia.h>
#endif /* linux */

#ifdef sun
#define	SAM_MIN_INODE_VERSION	1	/* Minimum version of inode layout. */
#define	SAM_INODE_VERS_1	1	/* Prev vers of inode, 3.5 & lower. */
#endif
#define	SAM_INODE_VERS_2	2	/* Prev vers of inode, 4.0-4.6. */
#define	SAM_INODE_VERSION	2	/* Current version of inode layout. */
#define	SAM_MAX_INODE_VERSION	SAM_INODE_VERSION	/* Maximum version. */

#define	SAM_CHECK_INODE_VERSION(x) \
			((x) >= SAM_MIN_INODE_VERSION && \
			(x) <= SAM_MAX_INODE_VERSION)

/* ----- Removable media mode field. */

#define	S_IFREQ 0xe000
#define	S_ISREQ(m) (((m)&0xf000) == 0xe000)

/*
 * Same value as noproject in /etc/project.
 */
#define	SAM_NOPROJECT	2


/* ----- Maximum symlink size that can be held in disk block area */

/*
 *	Note: Even when a symlink fits in the disk inode block extents
 *	area, an extension inode is still allocated and filled in
 *	(version 2 inodes only).
 */

#define	SAM_SYMLINK_SIZE	(NOEXT * sizeof (uint32_t)) + (((NOEXT+3)/4)*4)


/* ----- Segmented file index and segment inode mode and flag fields. */

/*
 * The status.b.seg_file and status.b.seg_ino may have non-zero values when
 * checked against an extent.  Thus, to apply S_ISSEGI and S_ISSEGS to an
 * extent we must first perform the S_ISREG test.
 *
 * The SAM kernel definitions of S_ISSEGI and S_ISSEGS assume that these tests
 * are never applied to extents.
 *
 * The non-kernel definitions of these tests assume that outside the kernel
 * they may be applied to extents.
 */

#if defined(_KERNEL)
#define	S_ISSEGI(di) ((di)->status.b.seg_file)
#define	S_ISSEGS(di) ((di)->status.b.seg_ino)
#else
#define	S_ISSEGI(di) (S_ISREG((di)->mode) && (di)->status.b.seg_file)
#define	S_ISSEGS(di) (S_ISREG((di)->mode) && (di)->status.b.seg_ino)
#endif

/* ----- Archive copy record. 4 copies in the permanent inode */

enum SAR_flags {
	SAR_pax_hdr	= 0x10,	/* pax tar hdr written when archived  */
	SAR_donot_use	= 0x20,	/* this bit was set on files archived */
				/* at 4.0 until 5.0, this bit is not  */
				/* set on files archived prior to 4.0 */
	SAR_diskarch	= 0x40,	/* disk archive  */
	SAR_size_block	= 0x80	/* file_offset is in units of 512 bytes */
};

typedef struct sam_archive_info {
	uchar_t		arch_flags;	/* archiver specific flags */
	char		mau_shift;	/* mau as power of 2 */
	short		n_vsns;		/* Number of sections */
	int32_t		version;	/* Optical disk version number */
	sam_time_t	creation_time;	/* Time archive record was created */
	int32_t		position_u;	/* Pos'n of archive file on media - */
	int32_t		position;	/*   media dependent */
	uint32_t	file_offset;	/* Offset of start of file in archive */
					/*   file - units determined by */
					/*   SAR_size_block */
	vsn_t		vsn;		/* Current vsn */
} sam_archive_info_t;


/* ----- Definitions for direct extents and indirect extents. */

#define	NSDEXT		(8)		/* Small inode direct extents */
#define	NLDEXT		(8)		/* Large inode direct extents */
#define	NDEXT		(NSDEXT+NLDEXT)	/* Number of inode direct extents */
#define	NIEXT		(3)		/* Large indirect extents */
#define	NOEXT		(NDEXT+NIEXT)	/* Number of inode extents */

#define	DEXT		(2048)		/* Direct extents in indirect extent */
#define	DEXTSHIFT	(11)


/* ----- Indirect extent structure. */

typedef struct	sam_indirect_extent {
	uint32_t	extent[DEXT];
	uchar_t		extent_ord[DEXT];
	sam_time_t	time;		/* Time indirect extent modified */
	int32_t		ieno;		/* Indirect extent number */
	sam_id_t	id;		/* Unique id: i-number/generation */
} sam_indirect_extent_t;

#define	SAM_INDBLKSZ	(16 * SAM_DEV_BSIZE)

typedef union sam_indirect_block {
	char				i[SAM_INDBLKSZ];
	struct sam_indirect_extent	indext;
} sam_indirect_block_t;


/* ----- Inode extension type definition. */

enum ext_types {
	ext_sln  = 0x01,	/* Symbolic name inode ext present */
	ext_rfa  = 0x02,	/* Resource file attr inode ext present */
	ext_hlp  = 0x04,	/* Hard link parent inode ext present */
	ext_acl  = 0x08,	/* Access control list inode ext present */
	ext_mva  = 0x10,	/* Multivolume archive inode ext present */
	ext_max  = 0xff
};

/* ----- Inode file status. */
/*
 * The worm_rdonly flag is used in two separate but related contexts.
 * For directories, the flag indicates that the directory is capable of
 *	holding WORM (immutable) files.
 *
 *  When a directory has the worm_rdonly flag set, it has the
 *	following constraints:
 *		- Any mkdir within inherits the worm_rdonly attribute.
 *		- The directory may not be renamed or removed, unless empty
 *		  directory.
 *		- The worm_rdonly bit may not be removed, unless empty
 *		  directory.
 *
 *	When a file other than a directory has the worm_rdonly flag set, it
 *	has the following constraints:
 *		- It cannot be written, nor have attributes changed.
 *		- It cannot be renamed or removed.
 *		- Its path is immutable.
 */

#define	SAM_INHERIT_MASK	0x307206ee
#define	SAM_DIRINHERIT_MASK	0x317206ee	/* for directories */
#define	SAM_ATTR_MASK		0x37f2d7ff

typedef struct	ino_status {
	uint32_t
#if defined(_BIT_FIELDS_HTOL)
		acl		:1,	/* Access control list */
		dfacl		:1,	/* Def ACL also (directories only ) */
		stripe_group:1,		/* Setfa -g attribute set */
		stripe_width:1,		/* Setfa -s attribute set */

		archive_a	:1, /* Bit pos'n, Used by public interface */
		seg_file	:1,	/* Segment file index */
		seg_ino		:1,	/* Segment data inode */
		worm_rdonly	:1,	/* Write once read many */

		worm_timeo	:1,	/* Write once read many timeout set */
		inconsistent_archive:1,	/* archive copy even if inconsistent */
		directio	:1,	/* Directio attribute */
		concurrent_archive:1, /* Archive when opened for write */

		direct_map	:1,	/* Use direct map extents */
		stage_failed:1,		/* Stage failed flag */
		segment		:1,	/* Segment access attribute */
		meta		:1,	/* allocated on meta device(s) */

		offline		:1,	/* File is offline */
		pextents	:1,	/* Partial extents on-line */
		archnodrop	:1,	/* No drop controlled by archiver */
		archdone	:1,	/* all required archiving done */

		on_large	:1,	/* Small extents on large block */
		cs_gen		:1,	/* Generate checksum */
		cs_use		:1,	/* Use checksum */
		cs_val		:1,	/* Valid checksum exists */

		stage_all	:1,	/* Stage all is enabled */
		noarch		:1,	/* No archive is enabled */
		bof_online	:1, /* Beginning of file on-line is enabled */
		damaged		:1,	/* File is damaged */

		direct		:1,	/* File is direct access */
		nodrop		:1,	/* No drop is enabled */
		release		:1,	/* Release after archive */
		remedia		:1;	/* Removable media */
#else /* defined(_BIT_FIELDS_HTOL) */
		remedia		:1,	/* Removable media */
		release		:1,	/* Release after archive */
		nodrop		:1,	/* No drop is enabled */
		direct		:1,	/* File is direct access */

		damaged		:1,	/* File is damaged */
		bof_online	:1, /* Beginning of file on-line is enabled */
		noarch		:1,	/* No archive is enabled */
		stage_all	:1,	/* Stage all is enabled */

		cs_val		:1,	/* Valid checksum exists */
		cs_use		:1,	/* Use checksum */
		cs_gen		:1,	/* Generate checksum */
		on_large	:1,	/* Small extents on large block */

		archdone	:1,	/* all required archiving done */
		archnodrop	:1,	/* No drop controlled by archiver */
		pextents	:1,	/* Partial extents on-line */
		offline		:1,	/* File is offline */

		meta		:1,	/* File is alloc'd on meta device(s) */
		segment		:1,	/* Segment access attribute */
		stage_failed:1,		/* Stage failed flag */
		direct_map	:1,	/* Use direct map extents */

		concurrent_archive:1, /* Archive when opened for write */
		directio	:1,	/* Directio attribute */
		inconsistent_archive:1,	/* arch copy, even if inconsistent */
		worm_timeo	:1,	/* Write once read many timeout set */

		worm_rdonly	:1, /* Write once read many (see comments) */
		seg_ino		:1,	/* Segment data inode */
		seg_file	:1,	/* Segment file index */
		archive_a	:1, /* Bit pos'n, Used by public interface */

		stripe_width:1,		/* Setfa -s attribute set */
		stripe_group:1,		/* Setfa -g attribute set */
		dfacl		:1,	/* Def ACL also (directories only ) */
		acl		:1;	/* Access control list */
#endif  /* defined(_BIT_FIELDS_HTOL) */

} ino_status_t;

typedef union ino_st_t {
	ino_status_t	b;			/* File status flags */
	uint32_t	bits;			/* Status bits */
} ino_st_t;

typedef union sam_psize {
	uint32_t	symlink;	/* Number of chars in symlink */
	uint32_t	rmfile;		/* Removable media size */
	uint32_t	partial;	/* Partial kilobyte size */
	uint32_t	rdev;		/* if BLK or CHR, real device */
} sam_psize_t;

/* Flags definition. */

enum AR_flags {
	AR_stale		= 0x01,		/* Copy is stale */
	AR_rearch		= 0x02,		/* Rearchive file */
	AR_arch_i		= 0x04,		/* Archive immediate */
	AR_verified		= 0x08,		/* Copy has been verified */
	AR_damaged		= 0x10,		/* Copy is damaged */
	AR_unarchived	= 0x20,		/* Copy was unarchived */
	AR_inconsistent	= 0x40,		/* Copy is inconsistent */
	AR_required		= 0x80,		/* Copy is required */
	AR_MAX = 0xff
};

typedef struct sam_di_ext {
	sam_bn_t		extent[NOEXT];		/* Extent ino array */
	uchar_t			extent_ord[NOEXT];	/* Extent ord array */
	uchar_t			stage_ahead;		/* Stage readahead */
} sam_di_ext_t;

#define	SAM_OSD_DIRECT			8	/* No. of stripes in disk ino */
#define	SAM_OSD_EXTENT			48	/* No. of stripes in ext ino */
#define	SAM_MAX_OSD_STRIPE_WIDTH	1024	/* Maximum OSDs in the stripe */

typedef struct sam_di_osd {
	sam_id_t		ext_id;
	uint64_t		obj_id[SAM_OSD_DIRECT];
	uchar_t			ord[SAM_OSD_DIRECT];
} sam_di_osd_t;

typedef union sam_di_ia {
	char			i[((NOEXT * sizeof (int)) +
				    (NOEXT * sizeof (char)) + 1)];
	sam_di_ext_t		ext;
	sam_di_osd_t		osd;
} sam_di_ia_t;

/* ----- Inode structure as it appears on a disk block. */

typedef struct sam_disk_inode {
	sam_mode_t	mode;		/* Mode and type of file */
	int32_t		version;	/* Inode layout version */
	sam_id_t	id;		/* Unique id: i-number/gen */
	sam_id_t	parent_id;	/* Unique parent id: i-num/gen */
	sam_rm_t	rm;		/* Removable file info */
	sam_id_t	ext_id;		/* Inode extension id: i-num/gen */
	uid_t		uid;		/* Owner's user id	*/
	gid_t		gid;		/* Owner's group id */
	int32_t		admin_id;	/* admin ID */
	uint32_t	nlink;		/* Number of links to file */
	ino_st_t	status;		/* Inode status */
	sam_timestruc_t	access_time;	/* Time file last accessed */
	sam_timestruc_t	modify_time;	/* Time file last accessed */
	sam_timestruc_t	change_time;	/* Time file last changed */
	sam_time_t	creation_time;	/* Time inode created */
	sam_time_t	attribute_time;	/* Time attributes last changed */
	uchar_t		unit;		/* Next stripe unit */
	uchar_t		cs_algo;
	uchar_t		arch_status;	/* Archive status */
	uchar_t		lextent;	/* Length of extent array */
	uchar_t		ar_flags[MAX_ARCHIVE];	/* SAM Archive flags */
	char		stripe;		/* Stripe stride width */
	char		stride;		/* Current number of stripes */
	media_t		media[MAX_ARCHIVE]; /* Archive media residency */
	uchar_t		stripe_group;	/* Stripe group unit */
	uchar_t		ext_attrs;	/* Inode extension types present */
	sam_psize_t	psize;		/* dev or symlink/media/partial size */
	int32_t		blocks;		/* Count of allocated blocks */
	sam_time_t	residence_time;	/* Time file changed residence */
	int32_t		free_ino;	/* Next free inode */
	sam_bn_t	extent[NOEXT];	/* Extent inode array */
	uchar_t		extent_ord[NOEXT]; /* Extent info byte array */
	uchar_t		stage_ahead;	/* Stage readahead */
} sam_disk_inode_t;

#define	P2FLAGS_XATTR	0x01
#define	P2FLAGS_WORM_V2	0x02
#define	P2FLAGS_PROJID_VALID	0x04

typedef struct sam_disk_inode_part2 {
	sam_id_t		xattr_id;
	uchar_t			pad1[8];
	projid_t		projid;			/* project ID */
	uchar_t			objtype;		/* object type */
	uchar_t			pad2[2];
	uchar_t			p2flags;
	sam_time_t		rperiod_start_time;		/* WORM */
	uint32_t		rperiod_duration;		/* WORM */
} sam_disk_inode_part2_t;

typedef struct sam_arch_inode {
	sam_archive_info_t image[MAX_ARCHIVE];	/* Up to 4 archive images */
} sam_arch_inode_t;

typedef struct sam_perm_inode {
	sam_disk_inode_t	di;
	csum_t			csum;
	sam_arch_inode_t	ar;
	sam_disk_inode_part2_t	di2;
} sam_perm_inode_t;


/*
 * Deprecated SAM_INODE_VERS_1.
 */
typedef struct sam_perm_inode_v1 {
	sam_disk_inode_t	di;
	csum_t					csum;
	sam_arch_inode_t	ar;
	sam_id_t				aid[MAX_ARCHIVE];
} sam_perm_inode_v1_t;

union sam_di_ino {
	char					i[SAM_ISIZE];
	struct sam_perm_inode	inode;
#ifdef sun
	struct sam_perm_inode_v1	inode_v1;
#endif
};


#endif /* _SAM_FS_INO_H */
