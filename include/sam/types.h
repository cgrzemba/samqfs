/*
 * sam/types.h - SAM-FS types.
 *
 * Description:
 *	Defines SAM-FS types and lots of other things.
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


#ifndef	_SAM_TYPES_H
#define	_SAM_TYPES_H

#ifdef sun
#pragma ident "$Revision: 1.43 $"
#endif

#ifdef linux

#ifndef __KERNEL__
#include <sys/types.h>
#include <netdb.h>
#endif /* __KERNEL__ */

#else /* linux */

#include <sys/types.h>
#include <netdb.h>

#endif /* linux */

#include "sam/sys_types.h"

/*
 * These typedefs are part of an emerging ANSI-C standard for fixed-size
 * integer types.
 * They are available in Solaris 2.6 (and above) in /usr/include/sys/types.h.
 */

#if defined _SYS_INT_TYPES_H
/*
 * (uint8_t, uint32_t and uint64_t are defined in Solaris 2.5
 * /usr/include/sys/synch.h.
 */
#define	_UINT8_T
#define	_UINT16_T
#define	_INT32_T
#define	_UINT32_T
#define	_INT64_T
#define	_UINT64_T
#endif  /* defined _SYS_INTTYPES_H */

#if !defined _UINT8_T
#define	_UINT8_T
typedef unsigned char uint8_t;
#endif  /* !defined _UINT8_T */
#if !defined _UINT16_T
#define	_UINT16_T
typedef unsigned short uint16_t;
#endif  /* !defined uint16_t */
#if !defined _INT32_T
#define	_INT32_T
typedef int int32_t;
#endif  /* !defined _INT32_T */
#if !defined _UINT32_T
#define	_UINT32_T
typedef unsigned int uint32_t;
#endif  /* !defined _UINT32_T */
#if !defined _INT64_T
#define	_INT64_T
typedef long long int64_t;
#endif  /* !defined _INT64_T */
#if !defined _UINT64_T
#define	_UINT64_T
typedef unsigned long long uint64_t;
#endif  /* !defined _UINT64_T */

#if !defined(FALSE)
#define	FALSE   (0)
#endif
#if !defined(TRUE)
#define	TRUE	(1)
#endif

/* The following is for the lawyers */
#define	COPYRIGHT "Copyright (c) 2001 Sun Microsystems, Inc. \
All Rights Reserved. SUN PROPRIETARY/CONFIDENTIAL."

#define	RESELL_LIB "libresel.so"

/* Set errno for MT and non-MT callers. */
/*
 * errno is defined differently for MT and non-MT compiled modules.
 * This macro may be used to properly return errno for both types
 * of modules linked to non-MT compiled library modules.
 * The underlying Solaris errno function is ___errno().
 */
extern int *___errno(void);
#define	SetErrno (*(___errno()))

/* Return the difference between two pointers. */
/*
 * lint produces warning: possible ptrdiff_t overflow when two pointers
 * are subtracted.
 *  Returns 0 if the difference is negative.
 *  The 'lint' version obviously will not do the work.
 */
#if !defined(lint)
#define	Ptrdiff(a, b) ((((char *)(a) - (char *)(b)) > 0) ? \
	((char *)(a) - (char *)(b)) : 0)
#else /* !defined(lint) */
#define	Ptrdiff(a, b) (memcmp((a), (b), 1))
#endif /* !defined(lint) */

/* Round value to be structure aligned. */
/*
 * Structures may have a size that is not a multiple of 64-bits.
 * When these structures are packed in a file that will be memory
 * mapped, the size must be rounded to a 64-bit multiple for SPARC
 * and a 32-bit multiple for AMD.
 */
#if defined(__sparc)
#define	STRUCT_RND(v) ((((v)+sizeof (longlong_t)-1) \
	/ sizeof (longlong_t)) * sizeof (longlong_t))
#elif defined(__i386) || defined(__amd64) || defined(x86_64) || defined(ia64)
#define	STRUCT_RND(v) ((((v)+sizeof (int32_t)-1) \
	/ sizeof (int32_t)) * sizeof (int32_t))
#else
	** ERROR **  unknown architecture
#endif

/*
 * All filesystem OTW structures must be 64-bit aligned,
 * regardless of the architecture for the server and clients.
 */
#define	STRUCT_RND64(v) ((((v)+sizeof (longlong_t)-1) \
	/ sizeof (longlong_t)) * sizeof (longlong_t))

#if defined(O_LARGEFILE)	/* Needed on open for large files */
#define	SAM_O_LARGEFILE O_LARGEFILE
#else
#define	SAM_O_LARGEFILE 0
#endif

#define	SAM_FSTYPE "samfs"	/* st_fstype string for SAM-FS */

#define	ROBOT_NO_SLOT 0xffffffff	/* used to flag no slot. */

/*
 * Define osd_handle (/usr/include/sys/osd.h)
 */
typedef	uint32_t sam_osd_handle_t;

/*
 * When the space left on a tape(in units of 1024) falls below this,
 * the tape will be considered full.
 */
#define	TAPE_IS_FULL 100

/* HASAM running file */

#define	HASAM_TMP_DIR   "/var/run"
#define	HASAM_RUN_FILE  HASAM_TMP_DIR"/hasam_running"

#define	BARCODE_LEN 36		/* length of barcode w/o null by */

typedef	int	ATOM_INT_T;	/* Int with atomic memory reference */

typedef uint64_t fsize_t;	/* Size of a file in bytes */
typedef int64_t sfsize_t;
#define	FSIZE_MAX 0x7fffffffffffffff

typedef char uname_t[32];	/* Device name */
typedef char upath_t[128];	/* Path name */

typedef char mtype_t[5];	/* Media type */
typedef char vsn_t[32];		/* ANSI Volume serial name, 31 chars */
				/* null terminated */
				/* Tape VSNs are 6 chars or less */

typedef char host_t[MAXHOSTNAMELEN];	/* Host name */

typedef uint16_t media_t;	/* Media type */
typedef uint16_t dtype_t;	/* Device type */
typedef uint16_t equ_t;		/* Equipment ordinal */
#define	EQU_MAX 65534		/* Maximum value for equipment ordinal */
				/* (need one reserved for historian) */

/* Common memory mapped file prefix. */
/*
 * If the MfValid field becomes zero, the file should be renewed.
 * MapFileDetach() then MapFileAttach().
 */
typedef struct MappedFile {
	uint32_t MfMagic;	/* Magic number of file */
	uint32_t MfLen;		/* Length of file mapped */
	uint32_t MfValid;	/* File valid if non-zero */
} MappedFile_t;

typedef uint32_t sam_ino_t;

typedef struct sam_id {		/* Inode identification */
	sam_ino_t ino;		/* I-number */
	int32_t gen;		/* Generation number */
} sam_id_t;

typedef struct csum {		/* Data verification value */
	uint32_t csum_val[4];
} csum_t;

typedef int32_t sam_time_t;	/* 32-bit time */

typedef struct sam_timestruc {
	int32_t	tv_sec;
	int32_t	tv_nsec;
} sam_timestruc_t;

typedef struct sam_flagtext {
	uint64_t	flagmask;
	char		*flagtext;
} sam_flagtext_t;

#define	SAM_POINTER(t) union { t *ptr; uint32_t p32; uint64_t p64; }


/* the following used in the SC_fsreleaser system call */
#define	RELEASER_STARTED	1
#define	RELEASER_FINISHED	0

#define	DF_THR_STK	  (0)
#define	SM_THR_STK	  DF_THR_STK
#define	MD_THR_STK	  DF_THR_STK
#define	LG_THR_STK	  DF_THR_STK
#define	HG_THR_STK	  DF_THR_STK

/*
 * Rounding directions
 */
typedef enum sam_round {
	SAM_ROUND_DOWN = 0,
	SAM_ROUND_UP = 1
} sam_round_t;

#endif  /* _SAM_TYPES_H */
