/*
 *  segment.h - segment structs.
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

#ifndef	_SAM_FS_SEGMENT_H
#define	_SAM_FS_SEGMENT_H

#ifdef sun
#pragma ident "$Revision: 1.18 $"
#endif

/*
 * ----- Segment Call Back type
 */
enum CALLBACK_type {
	CALLBACK_clear		= 1,
	CALLBACK_syscall	= 2,
	CALLBACK_inactivate	= 3,
	CALLBACK_wait_rm	= 4,
	CALLBACK_stat_vsn	= 5,
	CALLBACK_max		= 6
};

typedef struct sam_callback_clear {
	offset_t	length;
#ifdef	linux
	int		cflag;
#else	/* linux */
	enum sam_clear_ino_type cflag;
#endif	/* linux */
	cred_t		*credp;
} sam_callback_clear_t;

typedef struct sam_callback_syscall {
#ifdef	linux
	int	(*func)(struct inode *vp, int flag, void *args, cred_t *credp);
#else	/* linux */
	int	(*func)(vnode_t *vp, int flag, void *args, cred_t *credp);
#endif	/* linux */
	int			cmd;
	void		*args;
	cred_t		*credp;
} sam_callback_syscall_t;

typedef struct sam_callback_inactivate {
	int			flag;
} sam_callback_inactivate_t;

typedef struct sam_callback_wait_rm {
	int			archive_w;
} sam_callback_wait_rm_t;

typedef struct sam_callback_sam_vsn_stat {
	int			copy;
	int			bufsize;
	void		*buf;
} sam_callback_sam_vsn_stat_t;

typedef struct sam_callback_segment {
	union {
		sam_callback_clear_t		clear;
		sam_callback_syscall_t		syscall;
		sam_callback_inactivate_t	inactivate;
		sam_callback_wait_rm_t		wait_rm;
		sam_callback_sam_vsn_stat_t	sam_vsn_stat;
	} p;
} sam_callback_segment_t;

#endif /* _SAM_FS_SEGMENT_H */
