/*
 *	ioblk.h - SAM-FS file system map definitions.
 *
 *	Defines the structure of the I/O entry from the map function.
 *	Used in read/write I/O.
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
#ifndef	_SAM_FS_IOBLK_H
#define	_SAM_FS_IOBLK_H

#ifdef sun
#pragma ident "$Revision: 1.34 $"
#endif

#ifdef linux
#ifdef __KERNEL__
#include	<linux/types.h>
#include	<linux/param.h>
#include	<linux/uio.h>
#else	/* __KERNEL */
#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/uio.h>
#endif	/* __KERNEL__ */
#include	"sam/param.h"
#else	/* linux */

#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/uio.h>
#include	<sys/buf.h>
#include	"sam/param.h"

#endif	/* linux */

#include	"inode.h"


/*
 * ABR (application based recovery) flag.  Private between SVM
 * and SAM-QFS.  If not defined in base Solaris, define it here
 * for now.
 * Setting this on writes to mirrored volumes allows faster
 * writes (no DRL logging required), but the application must
 * be prepared to guarantee mirror consistency after uncompleted
 * writes (possibly by using DMR (directed mirror reads) to
 * verify completion) and/or rewriting to ensure consistency.
 */
#ifndef B_ABRWRITE
#define		B_ABRWRITE	0x4000000
#endif	/* B_ABRWRITE */

/*
 * Sparse flags.
 *
 * SPARSE_none: not sparse
 * SPARSE_nozero: don't zero allocated DAUs
 * SPARSE_zeroall: zero all allocated DAUs
 * SPARSE_directio: zero first and last allocated DAUs, directio only
 * SPARSE_noalloc: don't allocate, directio with MT_ZERO_DIO_SPARSE only
 */
enum SPARSE_type {
	SPARSE_none	= 0,
	SPARSE_nozero	= 1,
	SPARSE_zeroall	= 2,
	SPARSE_directio	= 3,
	SPARSE_noalloc	= 4,
};

typedef enum {SAM_READ = 0, SAM_READ_PUT, SAM_RD_DIRECT_IO, SAM_WRITE,
			SAM_WR_DIRECT_IO, SAM_FORCEFAULT, SAM_WRITE_MMAP,
			SAM_WRITE_BLOCK, SAM_WRITE_SPARSE, SAM_ALLOC_BLOCK,
			SAM_ALLOC_BITMAP,
			SAM_ALLOC_ZERO
} sam_map_t;

#define	SAM_MAP_NOWAIT	(0x1000) /* Mask to return on failover error */
#define	SAM_MAX_BP	(2)	/* Max blocks per page - (PAGESIZE/SM_BLK) */

#define	SAM_MAX_IOM	(4)	/* Maximum map entries per inode */

/*
 * SAM_DIRECTIO_MAX_CONTIG is the maximum number of contiguous
 * bytes a sam_map_block() call should ever return for direct I/O.
 */
#define	SAM_DIRECTIO_MAX_CONTIG 0x40000000


/* ----- Struct for map.c descriptor */

typedef struct map_params {
	offset_t dau_size; /* Size of DAU. Striped group = num_group * bsize */
	offset_t dau_mask;	/* mask of DAU, 0 if none */
	offset_t size_left;	/* Size left in DAU from offset */
	offset_t prev_filesize;	/* Size of file prior to extending it */
#ifdef	sun
	buf_t *bp;		/* Buffer pointer */
#endif	/* sun */
#ifdef	linux
	char *buf;
	int bufsize;
#endif	/* linux */
	uint32_t *bnp;		/* position of current extent */
	uchar_t *eip;		/* position of current extent ord */
	uchar_t ord;		/* Ordinal of bn in buffer */
	int	ileft;		/* Num "like" contiguous extents left */
	int	modified;	/* Buf has been modified, must write it */
	int	force_wr;	/* Force the buffer out, when writing */
	int level;		/* Level of indirect block */
	int kk;			/* Index into kptr */
	int kptr[NIEXT];	/* Position in indirect blocks */
} map_params_t;


/* -----	Maximum number of zero daus. */

#define	MAX_ZERODAUS	128

/* -----	I/O map flags field. */

#define	M_VALID		0x80		/* Valid entry */
#define	M_SPARSE	0x01		/* Sparse block -- create page */
#define	M_OBJECT	0x02		/* Object ioblk */
#define	M_OBJ_EOF	0x04		/* Object read past EOF--create page */

typedef struct sam_cache_ioblk {
	offset_t	blk_off0; /* Offset, beginning of stripe grp dau */
	offset_t	contig0;	/* Contiguous kluster size */
	sam_daddr_t	blkno0;		/* First block offset in file	*/
	uchar_t		ord0;		/* First group device ordinal */
	uchar_t		dt;		/* Device type - data or meta */
	uchar_t		bt;		/* Block type small or large */
	uchar_t		flags;		/* Flags */
} sam_cache_ioblk_t;

typedef struct sam_ioblk {
	sam_cache_ioblk_t imap;		/* Inode cache map entry */
	uint64_t	zerodau[2];	/* Bit map for allocated DAUs */
	offset_t	count;		/* Byte count in block */
	offset_t	blk_off;	/* Offset at beginning of disk dau */
	offset_t	contig;		/* Contiguous kluster size */
	sam_daddr_t	blkno;		/* Current block offset in file	*/
	int		pboff;		/* Physical byte offset	*/
	int		num_group;	/* Num elements in the striped group */
	int		bsize;		/* Block size */
	int		dev_bsize;	/* Device r/m/w size */
	ushort_t	no_daus;	/* Num daus returned for sparse */
	uchar_t		ord;		/* Current device ordinal */
	uchar_t		obji;		/* Current object array index */
} sam_ioblk_t;


typedef struct sam_iohdr {
	offset_t  contig;			/* Contiguous kluster size */
	uchar_t	nioblk;				/* Number of ioblks */
	sam_ioblk_t ioblk[SAM_MAX_BP];
} sam_iohdr_t;


typedef struct sam_iomap {
	int map_in;			/* In pointer */
	int map_out;		/* Last valid pointer */
	kmutex_t map_mutex;	/* Mutex to cover iomap */
	sam_cache_ioblk_t imap[SAM_MAX_IOM];
} sam_iomap_t;


#endif /* _SAM_FS_IOBLK_H */
