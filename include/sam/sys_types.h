/*
 *	sam/sys_types.h - SAM-FS system types.
 *
 *	System type definitions for the SAM-FS filesystem and daemons.
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

#ifndef	_SAM_SYS_TYPES_H
#define	_SAM_SYS_TYPES_H

#ifdef sun
#pragma ident "$Revision: 1.25 $"
#endif


#ifdef linux

#include <sam/linux_types.h>
#include <sam/osversion.h>

#ifndef __KERNEL__
#include <sys/fcntl.h>
#include <sam/linux_types.h>	/* struct statvfs */
#include <sys/time.h>
#endif	/* __KERNEL__ */

#else	/* linux */

#include <sam/osversion.h>
#include <sys/fcntl.h>

#include <sys/dirent.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/ksynch.h>
#endif	/* linux */


#ifdef sun
#include <sys/types32.h>

typedef int64_t		sam_tr_t;
typedef size_t		sam_size_t;
typedef ssize_t		sam_ssize_t;
typedef intptr_t	sam_intptr_t;
typedef uintptr_t	sam_uintptr_t;

typedef uint32_t	sam_mode_t;

#endif

#ifdef linux

typedef int64_t		sam_tr_t;
#ifdef _LP64
typedef int64_t		sam_intptr_t;
typedef uint64_t	sam_uintptr_t;
typedef int64_t		sam_ssize_t;
typedef uint64_t 	sam_size_t;
#else
typedef int			sam_intptr_t;
typedef uint_t		sam_uintptr_t;
typedef int			sam_ssize_t;
typedef uint_t		sam_size_t;
#endif
typedef int32_t		time32_t;

typedef uint32_t	sam_mode_t;

struct timeval32 {
	time32_t	tv_sec;		/* seconds */
	int32_t		tv_usec;	/* and microseconds */
};
#endif /* linux */

#define	SAM_MAXOFFSET_T	MAXOFFSET_T

typedef offset_t	sam_offset_t;
typedef u_offset_t	sam_u_offset_t;
typedef uint32_t	sam_bn_t;
typedef u_offset_t	sam_daddr_t;
typedef flock64_t	sam_flock_t;
typedef dirent64_t	sam_dirent64_t;
typedef statvfs64_t	sam_statvfs_t;


#endif	/* _SAM_SYS_TYPES_H */
