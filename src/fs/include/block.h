/*
 *	block.h - block structs for the SAM file system.
 *
 *	Defines the structure of the block tables for this instance
 *	of the SAM filesystem mount. The block tables are in the
 *	mount table.
 *  Defines the struct for the hidden small block file, inode 3.
 *  Defines the preallocation struct used during a preallocation
 *  request.
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

#ifdef sun
#pragma ident "$Revision: 1.25 $"
#endif


#ifndef	_SAM_FS_BLOCK_H
#define	_SAM_FS_BLOCK_H


/*
 * ----- sam_block is the block structure: SM and LG block tables.
 *
 * Maximum number of block buffers (SM, LG). Located in the
 * device section (struct samdent) of the mount table.
 */

#define	L_SAM_BLK	(256)	/* Max number of blocks in circular queue */

#define	BLOCK_COUNT(in, out, limit)		\
	(int)((in - out) < 0 ? (limit - out) + in : in - out)

struct sam_block {
	volatile int in;		/* In pointer */
	volatile int out;		/* Out pointer */
	volatile int fill;		/* Beginning in ptr at fill time */
	int			limit;	/* Size of circular buffer */
	short		ord;		/* Current partition ordinal */
	short		block_count;	/* Num blocks in circular buffer */
	short		requested;	/* Count of requested blocks */
	char		unused;
	char		dt;		/* Device type - data or meta */
	struct sam_mount *mp;		/* Pointer back to mount table */
	kmutex_t	md_mutex;
	sam_bn_t	bn[1];
};


#ifdef sun
/*
 * ----- sam_freeblk_pool holds 2 arrays of the SM and LG blocks released
 * since the last SYNC. Note, there are 2 lists, one active for I/O.
 *
 * Located in the mount table.
 */

#define	SAM_NO_FB_ARRAY		2		/* Max no of arrays for LG/SM */
#define	SAM_FB_ARRAY_LEN	3274	/* Max no of blocks in freeblk array */

typedef struct sam_freeblk {
	kmutex_t	fb_mutex;	/* Mutex for release block */
	int		fb_count;	/* Count of blocks to be released */
	int		fb_busy;
	sam_bn_t	fb_bn[SAM_FB_ARRAY_LEN];
	uchar_t		fb_ord[SAM_FB_ARRAY_LEN];
} sam_freeblk_t;

typedef struct sam_fb_pool {
	int		active;		/* Current filling active array */
	sam_freeblk_t	array[SAM_NO_FB_ARRAY];
} sam_fb_pool_t;

#endif /* sun */


/*
 * ----- sam_inoblk_t defines the structure of the free small block file.
 *
 * The free small block file is inode 3. It is hidden.
 */

typedef struct sam_inoblk {
	sam_bn_t  bn;		/* Large block address of first small block */
	ushort_t  ord;		/* Partition ordinal */
	ushort_t  free;		/* Bit mask for free blocks */
} sam_inoblk_t;


/*
 * ----- sam_prealloc_t holds pre-allocation information
 */

typedef struct sam_prealloc {
	offset_t length;		/* Preallocation byte length */
	int count0;			/* Count of (daus / stripe_count) */
	int count;			/* Countdown of (daus/stripe_count) */
	struct sam_prealloc *next;	/* Next prealocation request */
	int wait;			/* Set if waiting for preallocation */
	int error;			/* Returned error */
	int first_bn;			/* Starting block number */
	int first_dau;			/* Starting dau number */
	short set;			/* Clear if searching, set if setting */
	short dt;			/* Device type - data or meta */
	short ord;			/* Requested ordinal */
} sam_prealloc_t;


/*
 * ----- sam_dau_t holds the dau variables.
 *
 * Located in the mount table.
 */

typedef struct sam_dau {
	sam_u_offset_t mask[SAM_MAX_DAU];	/* Mask for dau */
	sam_u_offset_t seg[SAM_MAX_DAU];	/* Sm dau size/MAXBSIZE (8k) */
	sam_u_offset_t sm_bmask;		/* Mask for small/large block */
	uint_t		size[SAM_MAX_DAU];	/* Dau size in bytes */
	uint_t		wsize[SAM_MAX_DAU];	/* Dau size in words */
	uint_t		shift[SAM_MAX_DAU];	/* Shift for dau */
	uint_t		dif_shift[SAM_MAX_DAU];	/* Shift for dau */
	uint_t		blocks[SAM_MAX_DAU]; /* Dau size in logical blks (4k) */
	uint_t		kblocks[SAM_MAX_DAU];	/* Dau size in kb blks (1k) */
	uint_t		sm_blkcount;	/* Num sm dau blocks in large Dau */
	uint_t		sm_bits;		/* Bits, sm daus in lg dau */
	uint_t		sm_off;		/* (NSDEXT) sm direct daus off */
} sam_dau_t;


/* ----- Sam byte & word small and large block sizes. */

#define	SM_BLK(mp, dt)		mp->mi.m_dau[dt].size[SM]
#define	LG_BLK(mp, dt)		mp->mi.m_dau[dt].size[LG]

#define	SM_WBLK(mp, dt)		mp->mi.m_dau[dt].wsize[SM]
#define	LG_WBLK(mp, dt)		mp->mi.m_dau[dt].wsize[LG]


/* ----- Sam shifts and dif shift. */

#define	SM_SHIFT(mp, dt)		mp->mi.m_dau[dt].shift[SM]
#define	LG_SHIFT(mp, dt)		mp->mi.m_dau[dt].shift[LG]
#define	DIF_SHIFT(mp, dt) (mp->mi.m_dau[dt].shift[LG]-mp->dau[dt].shift[SM])

#define	DIF_SM_SHIFT(mp, dt)	mp->mi.m_dau[dt].dif_shift[SM]
#define	DIF_LG_SHIFT(mp, dt)	mp->mi.m_dau[dt].dif_shift[LG]


/* ----- Sam shifts and masks */

#define	SM_MASK(mp, dt)	mp->mi.m_dau[dt].mask[SM]	/* Small buffer mask */
#define	LG_MASK(mp, dt)	mp->mi.m_dau[dt].mask[LG]	/* Large buffer mask */


#define	SM_BMASK(mp, dt) mp->mi.m_dau[dt].sm_bmask	/* Small block mask */
#define	SM_BITS(mp, dt)	mp->mi.m_dau[dt].sm_bits /* Bits, sm daus in lg dau */
#define	SM_OFF(mp, dt)	mp->mi.m_dau[dt].sm_off	/* SM direct (NSDEXT) offset */
#define	SM_BLKCNT(mp, dt) mp->mi.m_dau[dt].sm_blkcount /* # SM blks in LG blk */


/*
 * ----- SAM logical block sizes in units of 4096 bytes.
 *
 *		4096 bytes is the SAM logical block size.
 *		Number of SAM logical blocks in the SM/LG dau.
 */

#define	SM_BLOCK(mp, dt)	mp->mi.m_dau[dt].blocks[SM]
#define	LG_BLOCK(mp, dt)	mp->mi.m_dau[dt].blocks[LG]


/*
 * ----- SAM device block sizes in units of kilobytes.
 *
 *		1024 bytes is the SAM device block.
 *		Number of SAM device blocks in the SM/LG dau.
 */

#define	SM_DEV_BLOCK(mp, dt)  mp->mi.m_dau[dt].kblocks[SM]
#define	LG_DEV_BLOCK(mp, dt)  mp->mi.m_dau[dt].kblocks[LG]


#endif  /* _SAM_FS_BLOCK_H */
