/*
 * ----- arfind.h - SAM-QFS file system arfind definitions.
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

#ifndef	_SAM_FS_ARFIND_H
#define	_SAM_FS_ARFIND_H

/* File events. */
enum sam_arfind_event {
	AE_none,
	AE_archive,			/* archive immediate */
	AE_change,			/* Attributes changed - uid, gid */
	AE_close,			/* File closed */
	AE_create,			/* File created */
	AE_hwm,				/* High water mark reached */
	AE_modify,			/* File modified */
	AE_rearchive,		/* Rearchived */
	AE_rename,			/* File renamed */
	AE_remove,			/* File removed */
	AE_unarchive,		/* Unarchived */
	AE_MAX
};

#if defined(NEED_ARFIND_EVENT_NAMES)
static char *fileEventNames[] = {
	"none", "archive", "change", "close", "create", "hwm", "modify",
	"rearchive", "rename", "remove", "unarchive", "internal" };
#endif /* defined(NEED_ARFIND_EVENT_NAMES) */

#if defined(_KERNEL)

/* arfind.c function prototypes. */

#ifdef sun
int sam_arfind_call(void *arg, int size, cred_t *credp);
void sam_arfind_hwm(sam_mount_t *mp);
void sam_arfind_init(sam_mount_t *mp);
void sam_arfind_fini(sam_mount_t *mp);
void sam_arfind_umount(sam_mount_t *mp);
void sam_send_to_arfind(sam_node_t *ip, enum sam_arfind_event event, int copy);
#endif /* sun */

#endif /* defined(_KERNEL) */

#if defined(ARFIND_PRIVATE)

#define	ARFIND_EVENT_MAX 1000	/* Maximum arfind_event-s in kernel buffer */

/* Begin: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack(4)
#endif

/* ----- sam_arfind_arg - SC_arfind system call arguments. */

struct sam_arfind_arg {		/* arfind daemon action */
	uname_t AfFsname;		/* Filesystem name */
	int		AfWait;		/* Wait time (seconds) */
	int		AfCount;	/* Num entries returned in buffer */
	int		AfMaxCount;	/* Max number of entries in buffer */
	int		AfOverflow;	/* Count of buffer overflows */
	int		AfUmount;	/* FS unmount started when non-zero */
	SAM_POINTER(void) AfBuffer; /* Buffer for actions */
};


/* ----	arfind_ent - arfind file action entry. */

struct arfind_event {
	sam_id_t AeId;			/* Inode */
	uint8_t	AeEvent;		/* File action event */
	uint8_t	AeCopy;			/* Copy number */
	uint16_t AeUnused;
};

/* End: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack()
#endif

#endif /* defined(ARFIND_PRIVATE) */

#endif /* _SAM_FS_ARFIND_H */
