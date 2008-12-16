/*
 * sam/linux_types.h - SAM-FS system types for Linux.
 *
 * System type definitions for the SAM-FS filesystem and daemons.
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

#ifndef	_SAM_LINUX_TYPES_H
#define	_SAM_LINUX_TYPES_H

#ifdef sun
#pragma ident "$Revision: 1.29 $"
#endif

#ifndef __KERNEL__

#ifdef SUSE_LINUX
#ifdef _LP64
#define	BITS_PER_LONG	64
#else
#define	BITS_PER_LONG   32
#endif
#endif /* SUSE_LINUX */

#if defined(_LP64) && !defined(x86_64) && !defined(__ia64)
typedef unsigned long u64;
typedef unsigned int u32;
typedef signed long s64;
#endif /* _LP64 */

#include <stdint.h>
#include <time.h>
#include <byteswap.h>
#include <sys/param.h>

#define	SAM_LIB_GNU

#endif /* __KERNEL__ */

#if (KERNEL_MAJOR < 5)
#ifndef	O_LARGEFILE
#define	O_LARGEFILE 0
#endif
#endif

#define	_SYS_INT_TYPES_H

/*
 * lseek64 is a better match for llseek
 */
#ifndef	__KERNEL__
#define	llseek	lseek64
#endif

/*
 * Access mode bits
 */
#define	S_IAMB	0x1FF

/*
 * Linux doesn't have SIGEMT
 */
#define	SIGEMT	SIGIOT

/*
 * Linux defines NGROUPS_MAX to 32, but Solaris Server uses 16.
 */
#define	NGROUPS_MAX_DEFAULT	16

/*
 * Max size of a file name
 * MUST match the size of component in sam_san_name_msg_t
 */
#define	MAXNAMELEN	256

/*
 * Max length of a pathname.  Solaris's limit is lower than Linux's
 * so use the Solaris limit.
 */
#define	QFS_MAXPATHLEN	1024

#define	FSTYPSZ	16

typedef enum { B_FALSE, B_TRUE } boolean_t;

typedef unsigned long ulong_t;
typedef unsigned long long u_longlong_t;
typedef long long longlong_t;
typedef unsigned short ushort_t;
typedef unsigned char uchar_t;
typedef unsigned int uint_t;
typedef loff_t	offset_t;
#ifdef	__KERNEL__
typedef unsigned long long u_offset_t;
#else
typedef __u_quad_t	u_offset_t;
#endif	/* __KERNEL__ */
#ifndef _LARGEFILE_SOURCE
typedef offset_t	off64_t;
typedef ino_t	ino64_t;
typedef ulong_t	fsblkcnt64_t;
typedef ulong_t fsfilcnt64_t;
#endif
typedef ulong_t fsblkcnt32_t;
typedef ulong_t fsfilcnt32_t;
typedef unsigned long *kthread_id_t;
typedef unsigned int dev32_t;
typedef	unsigned int projid_t;

/*
 * Time expressed as a 64-bit nanosecond counter.
 */
typedef	longlong_t	hrtime_t;

typedef struct flock flock_t;

/*
 * These structures are passed from user land to
 * the kernel in other structures so these
 * these definitions need to be duplicated here
 * since including the kernel include files
 * is not always possible or desirable.
 */
#ifndef __KERNEL__
typedef struct { } kmutex_t;
typedef struct { } kcondvar_t;
typedef struct { } krwlock_t;
#endif

typedef struct linux_flock64 {
	short	l_type;
	short	l_whence;
	off64_t	l_start;
	off64_t	l_len;		/* len == 0 means until end of file */
	int	l_sysid;
	pid_t	l_pid;
	long	l_pad[4];	/* reserve area */
} flock64_t;

#ifdef __KERNEL__
#include <sam/linux_ktypes.h>
#endif /* __KERNEL__ */

#ifndef __KERNEL__
#ifndef _LARGEFILE_SOURCE
typedef struct dirent64 {
	ino64_t		d_ino;		/* "inode number" of entry */
	off64_t		d_off;		/* offset of disk directory entry */
	unsigned short	d_reclen;	/* length of this record */
	char		d_name[1];	/* name of file */
} dirent64_t;
#else
typedef struct dirent64 dirent64_t;
#endif
#endif /* __KERNEL__ */

typedef struct statvfs {
	unsigned long	f_bsize;	/* fundamental file system block size */
	unsigned long	f_frsize;	/* fragment size */
	fsblkcnt32_t	f_blocks;	/* total blocks of f_frsize on fs */
	fsblkcnt32_t	f_bfree;	/* total free blocks of f_frsize */
	fsblkcnt32_t	f_bavail;	/* free blocks avail to non-superuser */
	fsfilcnt32_t	f_files;	/* total file nodes (inodes) */
	fsfilcnt32_t	f_ffree;	/* total free file nodes */
	fsfilcnt32_t	f_favail;	/* free nodes avail to non-superuser */
	unsigned long	f_fsid;		/* file system id (dev for now) */
	char		f_basetype[FSTYPSZ];	/* target fs type name, */
						/* null-terminated */
	unsigned long	f_flag;		/* bit-mask of flags */
	unsigned long	f_namemax;	/* maximum file name length */
	char		f_fstr[32];	/* filesystem-specific string */
#if !defined(_LP64)
	unsigned long	f_filler[16];	/* reserved for future expansion */
#endif
} statvfs_t;

typedef struct statvfs64 {
	unsigned long	f_bsize;	/* preferred file system block size */
	unsigned long	f_frsize;	/* fundamental file system block size */
	fsblkcnt64_t	f_blocks;	/* total blocks of f_frsize */
	fsblkcnt64_t	f_bfree;	/* total free blocks of f_frsize */
	fsblkcnt64_t	f_bavail;	/* free blocks avail to non-superuser */
	fsfilcnt64_t	f_files;	/* total # of file nodes (inodes) */
	fsfilcnt64_t	f_ffree;	/* total # of free file nodes */
	fsfilcnt64_t	f_favail;	/* free nodes avail to non-superuser */
	unsigned long	f_fsid;		/* file system id (dev for now) */
	char		f_basetype[FSTYPSZ];	/* target fs type name, */
						/* null-terminated */
	unsigned long	f_flag;		/* bit-mask of flags */
	unsigned long	f_namemax;	/* maximum file name length */
	char		f_fstr[32];	/* filesystem-specific string */
	unsigned long	f_filler[16];	/* reserved for future expansion */
} statvfs64_t;

#define	MAXBSIZE	8192

#define	LEN_DKL_VVOL	8
#define	NDKMAP		8
#define	V_NUMPAR	NDKMAP
#define	LEN_DKL_ASCII	128

#ifndef	__KERNEL__
struct partition {
	ushort_t p_tag;		/* ID tag of partition */
	ushort_t p_flag;	/* permision flags */
	daddr_t p_start;	/* start sector no of partition */
	long    p_size;		/* # of blocks in partition */
};

struct vtoc {
	unsigned long	v_bootinfo[3];	/* info needed by mboot (unsupported) */
	unsigned long	v_sanity;	/* to verify vtoc sanity */
	unsigned long	v_version;	/* layout version */
	char	v_volume[LEN_DKL_VVOL];	/* volume name */
	ushort_t	v_sectorsz;	/* sector size in bytes */
	ushort_t	v_nparts;	/* number of partitions */
	unsigned long	v_reserved[10];	/* free space */
	struct partition v_part[V_NUMPAR]; /* partition headers */
	time_t	timestamp[V_NUMPAR];	/* partition timestamp (unsupported) */
	char	v_asciilabel[LEN_DKL_ASCII];	/* for compatibility */
};
#endif	/* __KERNEL__ */

#define	DK_DEVLEN	16		/* device name max length, including */
					/* unit # & NULL (ie - "xyc1") */
/*
 * Used for controller info
 */
struct dk_cinfo {
	char	dki_cname[DK_DEVLEN];	/* controller name (no unit #) */
	ushort_t dki_ctype;		/* controller type */
	ushort_t dki_flags;		/* flags */
	ushort_t dki_cnum;		/* controller number */
	uint_t	dki_addr;		/* controller address */
	uint_t	dki_space;		/* controller bus type */
	uint_t	dki_prio;		/* interrupt priority */
	uint_t	dki_vec;		/* interrupt vector */
	char	dki_dname[DK_DEVLEN];	/* drive name (no unit #) */
	uint_t	dki_unit;		/* unit number */
	uint_t	dki_slave;		/* slave number */
	ushort_t dki_partition;		/* partition number */
	ushort_t dki_maxtransfer;	/* max. transfer size in DEV_BSIZE */
};

typedef struct timespec timestruc_t;	/* definition per SVr4 */

#endif	/* _SAM_LINUX_TYPES_H */
