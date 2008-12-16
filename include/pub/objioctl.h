/*
 * ----- objioctl.h - Object ioctl definition.
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
#ifndef _OBJIOCTL_H
#define	_OBJIOCTL_H

#pragma ident "$Revision: 1.3 $"

#include <sys/ioccom.h>
/* Begin: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack(4)
#endif

/*
 * Definition to handle 32bit/64bit pointers.
 */
union bufp {
	void *ptr;
	uint32_t p32;
	uint64_t p64;
};

/*
 * Target(FS) level ioctl support routines.
 */
struct obj_ioctl_attach {
	char		fsname[32];	/* File system name */
	uint64_t	handle;		/* Handle to this Target */
};
#define	C_OBJATTACH	1
#define	O_OBJATTACH	_IOWR('o', C_OBJATTACH, struct obj_ioctl_attach)

/*
 * Detach from an OSD Target.
 */
struct obj_ioctl_detach {
	uint64_t	handle;	/* Target Handle */
};
#define	C_OBJDETACH	2
#define	O_OBJDETACH	_IOWR('o', C_OBJDETACH, struct obj_ioctl_detach)

/*
 * OSD Command
 * The CDB is already in Host Native Format.  It is the responsibility of the
 * caller to provide Endianess convesion prior to calling this routine.
 */
struct obj_ioctl_osdcdb {
	uint64_t	handle;		/* Target Handle */
	osd_cdb_t	cdb;		/* OSD CDB */
	union bufp	datain;		/* Data In Buffer e.g. Read Buffer */
	union bufp	dataout;	/* Data Out Buffer e.g. Write Buffer */
	uint64_t	residual;	/* Residual bytes */
};
#define	C_OBJOSDCDB	3
#define	O_OBJOSDCDB	_IOWR('o', C_OBJOSDCDB, struct obj_ioctl_osdcdb)

/* End: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack()
#endif

#endif /* _OBJIOCTL_H */
