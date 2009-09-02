/*
 *	fsdaemon.h - SAM-QFS file system call daemon.
 *
 *	Type definitions for the SAM-QFS file system call daemon.
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

#ifndef	_SAM_FS_FSDAEMON_H
#define	_SAM_FS_FSDAEMON_H

#ifdef sun
#pragma ident "$Revision: 1.29 $"
#endif

/*
 * ----- File System system call daemon (sam-fsd).
 */
enum FSD_cmd {
	FSD_null = 0,
	FSD_sharedmn,		/* Argument is sam_fsd_mount */
	FSD_mount,		/* Argument is sam_fsd_mount */
	FSD_releaser,		/* Argument is sam_fsd_releaser */
	FSD_syslog,		/* Argument is sam_fsd_syslog */
	FSD_restart,		/* Argument is sam_fsd_restart */
	FSD_preumount,		/* Argument is sam_fsd_umount */
	FSD_umount,		/* Argument is sam_fsd_umount */
	FSD_stalefs,		/* Argument is sam_fsd_umount */
	FSD_stop_sam,		/* Argument is sam_fsd_mount */
	FSD_shrink,		/* Argument is sam_fsd_shrink */
	FSD_cmd_MAX
};

/* Begin: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack(4)
#endif

struct sam_fsd_archiver {
	uname_t		fs_name;	/* File system name */
};

struct sam_fsd_mount {
	uname_t		fs_name;	/* File system name */
	sam_time_t	init;		/* Time superblock iniitalized */
	boolean_t	start_samdb;	/* Start fsalogd */
};

struct sam_fsd_releaser {
	uname_t fs_name;
};

struct sam_fsd_syslog {
	upath_t		mnt_point;	/* Mount point */
	sam_id_t	id;		/* Inode id - i-number and generation */
	equ_t		eq;		/* equipment number */
	int		slerror;	/* errmsg, include/sam/fs/syslogerr.h */
	int		error;		/* errno */
	int		param;		/* optional integer parameter */
};

struct sam_fsd_umount {
	uname_t		fs_name;	/* File system name */
	int		umounted;	/* 1 if umount succeeded, 0 if not */
};

struct sam_fsd_restart {
	uname_t		fs_name;	/* File system name */
};

struct sam_fsd_shrink {
	uname_t		fs_name;	/* File system name */
	int		command;	/* DK_CMD_remove or DK_CMD_release */
	int		eq;		/* Equipment to remove/release */
};

struct sam_fsd_cmd {
	int32_t cmd;
	union {
		struct sam_fsd_archiver archiver;
		struct sam_fsd_mount mount;
		struct sam_fsd_releaser releaser;
		struct sam_fsd_syslog syslog;
		struct sam_fsd_umount umount;
		struct sam_fsd_restart restart;
		struct sam_fsd_shrink shrink;
	} args;
};

/* End: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack()
#endif

#endif	/* _SAM_FS_FSDAEMON_H */
