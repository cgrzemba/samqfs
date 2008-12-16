/*
 *	debug.h - debug definitions for SAM-QFS
 *
 *	Defines the debug functions for SAM-QFS.
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
#ifndef	_SAM_FS_DEBUG_H
#define	_SAM_FS_DEBUG_H

#ifdef sun
#pragma ident "$Revision: 1.23 $"
#endif

/*
 * ----- samfs debug.
 *
 *	dcmn_err - use double parens so there appears to be one parameter
 *	to the function; cpp will then substitute the parenthesized list.
 */

#ifdef	DEBUG

extern int panic_on_fs_error;

#define	SAMFS_DEBUG_PANIC	CE_PANIC

#define	SAMFS_CE_DEBUG_PANIC (panic_on_fs_error ? CE_PANIC : CE_WARN)

#define	SAMFS_PANIC_INO(fsname, type_str, ino_num) \
		cmn_err(panic_on_fs_error ? CE_PANIC : CE_WARN, \
			"SAM-QFS: %s: inode 0x%x has a %s error", \
			fsname, ino_num, type_str);

#define	dcmn_err(X)		cmn_err X

#else	/* DEBUG */

#define	SAMFS_DEBUG_PANIC	CE_WARN

#define	SAMFS_CE_DEBUG_PANIC  CE_WARN

#define	SAMFS_PANIC_INO(fsname, type_str, ino_num) \
		cmn_err(CE_WARN, \
			"SAM-QFS: %s: inode 0x%x has a %s error", \
			fsname, ino_num, type_str);

#define	dcmn_err(X)		/* nothing */

#endif	/* DEBUG */

/*
 * ----- samfs debug panic.
 */
#ifdef linux
#define	SAM_ASSERT_PANIC(panic_str)	panic(panic_str)
#endif /* linux */
#ifdef sun
#define	SAM_ASSERT_PANIC(panic_str)	\
		dcmn_err((CE_PANIC, panic_str "LINE=%s", __LINE__))
#endif /* sun */

#endif	/* _SAM_FS_DEBUG_H */
