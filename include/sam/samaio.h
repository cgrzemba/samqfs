/*
 * sam/samaio.h - Ioctl(2) file definitions.
 *
 * Contains structures and definitions for IOCTL file commands.
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
 * or https://illumos.org/license/CDDL.
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

#ifndef _SAM_SAMAIO_H
#define	_SAM_SAMAIO_H

#ifdef sun
#pragma ident "$Revision: 1.15 $"
#endif


#include <sys/types.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/dkio.h>

/*
 * /dev names:
 *	/dev/samaioctl	- master control device
 *	/dev/rsamaio	- character devices, named by minor number
 */
#define	SAMAIO_DRIVER_NAME	"samaio"
#define	SAMAIO_CTL_NODE		"ctl"
#define	SAMAIO_CTL_NAME		SAMAIO_DRIVER_NAME SAMAIO_CTL_NODE
#define	SAMAIO_BLOCK_NAME	SAMAIO_DRIVER_NAME
#define	SAMAIO_CHAR_NAME	"r" SAMAIO_DRIVER_NAME

/*
 *
 * Use is:
 *	ld = open("/dev/samaioctl", O_RDWR | O_EXCL);
 *
 * samaio must be opened exclusively. Access is controlled by permissions on
 * the device, which is 644 by default. Write-access is required for ioctls
 * that change state, but only read-access is required for the ioctls that
 * return information.
 *
 * ioctl usage:
 *
 *	ioctl(ld, SAMAIO_ATTACH_DEVICE, &samaio_ioctl);
 *
 * This ioctl is private and only for use by QFS. QFS attaches a raw
 * minor device when a "q" file in lookupname.
 *
 */

typedef struct samaio_ioctl {
	equ_t fs_eq;			/* File system equipment number */
	uint32_t ino;			/* File i-number */
	int32_t gen;			/* File generation number */
	vnode_t *vp;			/* Vnode number */
} samaio_ioctl_t;

#define	SAMAIO_IOC_BASE		(('A' << 16) | ('F' << 8))

#define	SAMAIO_ATTACH_DEVICE	(SAMAIO_IOC_BASE | 0x01)
#define	SAMAIO_DETACH_DEVICE	(SAMAIO_IOC_BASE | 0x02)


/*
 * Only character file types are usable with samaio.
 */
#if defined(_KERNEL)

#define	V_ISSAMAIOABLE(vtype) \
	(vtype == VCHR)

/*
 * This is the uio struct passed to QFS read/write. The buf pointer
 * is after the uio struct.
 */
typedef struct samaio_uio {
	struct uio	uio;		/* uio */
	int		type;		/* type = 1, identifies samaio */
	buf_t		*bp;		/* bp for type == 1 */
	struct iovec iovec[1];		/* iovec */
} samaio_uio_t;

/*
 * types
 */
#define	SAMAIO_CHR_AIO		1
#define	SAMAIO_LIO_DIRECT	2
#define	SAMAIO_LIO_PAGED	3


/*
 * This is the soft state for each minor device.
 */
typedef struct samaio_state {
	struct vnode	*aio_vp;		/* QFS vnode */
	equ_t		aio_fs_eq;		/* QFS equipment number */
	uint32_t	aio_ino;		/* QFS inode number */
	int32_t		aio_gen;		/* QFS gen number */
	int		aio_chr_flags;		/* SAM_AIO_OPEN? */
	int		aio_chr_lyropen;	/* Number of layered opens */
	int		aio_filemode;		/* Open filemode */
} samaio_state_t;

/*
 * aio_chr_flags
 */
#define	SAM_AIO_OPEN	1

#endif

#endif /* _SAM_SAMAIO_H */
