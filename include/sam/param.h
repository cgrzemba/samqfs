/*
 * sam/param.h - SAM-FS parameter information.
 *
 *  Description:
 *      Parameters for SAM-FS.
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

#ifndef _SAM_PARAM_H
#define	_SAM_PARAM_H

#ifdef sun
#pragma ident "$Revision: 1.60 $"
#endif

#define	L_FSET 252			/* Maximum family set size */

#define	SAM_CONTIG_LUN (128)		/* Default stripe unit for 1 lun */

#define	SAM_DEFAULT_MS_DAU (64)		/* Default DAU for SAMFS */
#define	SAM_DEFAULT_MA_DAU (64)		/* Default DAU for QFS */
					/* wo/stripe groups */
#define	SAM_DEFAULT_SG_DAU (256)	/* Default DAU for QFS */
					/* w/ stripe groups */
#define	SAM_DEFAULT_META_DAU (16)	/* Default DAU for SAMFS */

#define	SAM_MAX_SHARED_HOSTS (65535)	/* max hosts for shared FS */

/*
 * 30 days is default retention period for WORM.
 * Minimum retention period is 1 hour.
 */
#define	DEFAULT_RPERIOD (30 * 24 * 60)
#define	MAX_RPERIOD (2147483648LL)
#define	MIN_RPERIOD 0
#define	V1_RPERIOD (10)

/*
 * The following definitions are for the shared file system.
 */
#define	MAX_EXPIRING_LEASES	(3)		/* Shared rd/wr/append leases */
#define	SAM_DEFAULT_MINALLOCSZ	(8)		/* Default # of miminum DAUs */
#define	SAM_DEFAULT_MAXALLOCSZ	(128)		/* Default # of maximum DAUs */

#define	DEF_LEASE_TIME	(30)
#define	MIN_LEASE_TIME	(15)
#define	MAX_LEASE_TIME	(600)

#define	RD_LEASE		(0)		/* Read lease */
#define	WR_LEASE		(1)		/* Write lease */
#define	AP_LEASE		(2)		/* Append lease */

#define	DEF_META_TIMEO	(3)	/* 3    Default cache invalidate time */
#define	MIN_META_TIMEO	(0)	/* 0    Minimum cache invalidate time */
#define	MAX_META_TIMEO	(60)	/* 60   Maximum cache invalidate time */

/*
 * The following definitions are for write throttle.
 */
#define	SAM_MINWR (256)		/* 256k Low write bytes outstanding. */
#define	SAM_MAXWR (262144)	/* 2g   High write bytes outstanding. */
#define	SAM_DEFWR (-1)		/* -1   Default wr_throttle set to 2% memsize */

#define	SAM_MAX_WR_THROTTLE (524288)	/* Maximum KB write throttle value */


/*
 * The following definitions are the readahead and writebehind for paged I/O.
 */
#define	CNTG_readahead 1
#define	CNTG_writebehind 2

#define	SAM_MINRA (0)		/* Minimum kb read_ahead */
#define	SAM_MAXRA (16777216)	/* Maximum kb read_ahead */
#define	SAM_DEFRA (1024)	/* Default kb read_ahead */

#define	SAM_MINWB (8)		/* Minimum kb write_behind */
#define	SAM_MAXWB (16777216)	/* Maximum kb write_behind */
#define	SAM_DEFWB (512)		/* Default kb write_behind */

/*
 * The following definitions are the defaults for paged to direct I/O
 * switching based on structure (well-formed or not), size and number of
 * consecutive criteria meeting I/Os. A single set of defaults is defined for
 * read and write.
 */
#define	SAM_MINWF_AUTO (256)	/* Min kb to switch a well-formed request */
#define	SAM_MINIF_AUTO (0)	/* Min kb to switch a ill-formed request */
#define	SAM_CONS_AUTO (0)	/* Min consecutive number of requests */
				/* zero consecutive specifies never switch */


#define	SAM_MAXOFF_T	(4294918144)	/* Support 0xffff4000 byte file */

#define	SAM_MINSWINDOW	64		/* Minimum stage window in kilobytes */
#define	SAM_MAXSWINDOW	2097152		/* Maximum stage window in kilobytes */
#define	SAM_DEFSWINDOW	256		/* Default stage window in kilobytes */

#define	SAM_MINPARTIAL	8		/* Minimum partial kilobytes */
#define	SAM_MAXPARTIAL	2097152		/* Maximum partial kilobytes */
#define	SAM_DEFPARTIAL	16		/* Default partial kilobytes */

#define	MAX_ARCHIVE	4		/* Maximum archive copies */

#define	MAX_STAGE_RETRIES_DEF 3		/* Default max stage retries */

#define	SAM_ONE_MEGABYTE (1024*1024)	/* 1 Megabyte */

#ifndef NULL
#define	NULL		0
#endif

#ifndef NBPW
#define	NBPW	sizeof (int)		/* number of bytes per word */
#endif
#ifndef NBPLW
#define	NBPLW	sizeof (off64_t)	/* number of bytes per longlong */
#endif
#ifndef NBBY
#define	NBBY	8			/* number of bits per byte */
#endif

#define	NBPWSHIFT   2			/* byte shift for word */
#define	NBBYSHIFT   3			/* bit shift for byte */
#define	NBWDSHIFT   5			/* bit shift for word */


/* ----- Sam Device Block size */

#define	SAM_DEV_BSIZE	(0x400)	/* SAM device block size is 1024 bytes */
#define	SAM_OSD_BSIZE   (1)	/* SAM object dev blk size is 1 byte */
#define	SAM_DEV_BSHIFT	(10)	/* SAM device byte to block	shift */
#define	SAM_DEV_WSIZE	(0x100)	/* 256 int32 words in SAM device block */

#define	SAM2SUN_BSHIFT	(1)	/* SAM device block shift to convert */
				/* to Solaris DEV_BSIZE sectors. */
#define	SAM_BIT_SHIFT	(13)	/* Log2 Logical bit to block */
#define	SAM_LOG_WMASK	(0x3fc)	/* Logical block word mask */


/* ----- Sam inode shift and block size */

#define	SAM_BLK	(0x1000)	/* Size of SAM fundamental block is 4096 */
#define	SAM_LOG_BLOCK	(4)	/* Number of blocks in fundamental block */
#define	SAM_SHIFT	(12)	/* Log2(SAM_BLK) */
#define	DIF_SAM_SHIFT	(2)


/* ----- Sam fundamental directory block size and shift count */

#define	DIR_BLK		(0x1000)	/* Size of directory block */
#define	DIR_LOG_BLOCK	(4)	/* Number of blocks in directory block */


/* ----- Sam inode block size shift count */

#define	SAM_ISIZE	512		/* Size of sam_ino_t. */
#define	SAM_ISHIFT	9		/* Shift for sam_ino_t size. */
					/* Depends on sizeof(sam_ino_t)= 512 */
#define	INO_BLK_SIZE	(0x4000)	/* ".inodes" buffer cache size */
#define	INO_BLK_FACTOR	(32)		/* ".inodes" buffer size for */
					/* ar and rec */
#define	INO_IN_BLK (INO_BLK_SIZE>>SAM_ISHIFT)	/* Number of inodes in block */
#define	INO_ALLOC_SIZE	(256*1024) /* Number of bytes to allocate in .inodes */


/* ----- Sam disk types */

#define	SAM_MAX_DD	(2)		/* Maximum disk types (data and meta) */
#define	DD		(0)		/* Data device	*/
#define	MM		(1)		/* Meta device	*/


/* ----- Sam disk allocation types */

#define	SAM_MAX_DAU	(2)		/* Maximum disk allocations */
					/*  (small and large) */
#define	SM		(0)		/* Small dau	*/
#define	LG		(1)		/* Large dau	*/

/* ----- Sam quota types (admin, group, user) */
#define	SAM_QUOTA_MAX	(3)

#endif  /* _SAM_PARAM_H */
