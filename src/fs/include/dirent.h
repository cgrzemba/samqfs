/*
 *	dirent.h - SAM-FS file system directory entry definition.
 *
 *	Defines the structure of the directory entry in the directory
 *	files.
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

#ifndef	_SAM_FS_DIRENT_H
#define	_SAM_FS_DIRENT_H

#ifdef sun
#pragma ident "$Revision: 1.33 $"
#endif

#ifdef	linux
#ifdef	__KERNEL__
#include <linux/types.h>
#else	/* __KERNEL */
#include <sys/types.h>
#endif	/* __KERNEL__ */
#include "sam/linux_types.h"
#include "sam/types.h"
#else	/* linux */

#include <sys/types.h>
#include <sys/mutex.h>
#include <sys/dnlc.h>
#include "sam/types.h"

#endif	/* linux */

#define	SAM_DIR_VERSION	1		/* First SAM-FS directory layout */

#if defined(MAXNAMLEN)
#undef MAXNAMLEN
#endif
#define	MAXNAMLEN	255			/* Maximum name length */


/* ----- SAM File-system validation directory record entry.  */

struct sam_dirval {
	uint16_t	d_version;	/* Ver number for this dirent layout */
	uint16_t	d_reclen;	/* Length of this record */
	sam_id_t	d_id;		/* i-number/generation */
	sam_time_t	d_time;		/* Time indirect extent modified */
};


/* ----- SAM File-system directory entry.  */

typedef struct sam_dirent {
	uint16_t	d_fmt;		/* File fmt for entry, 0 if empty */
	uint16_t	d_reclen;	/* Length of this record */
	sam_id_t	d_id;		/* Identification - i-num/gen */
	uint16_t	d_namehash;	/* Name hash, or zero */
	uint16_t	d_namlen;	/* Length of string in d_name */
	uchar_t		d_name[4];	/* First char of filename */
					/*   Must be on INT boundry */
} sam_dirent_t;


/* ----- Directory record length given ^dirent */

#define	SAM_DIRSIZ(dp) \
	((sizeof (struct sam_dirent)-NBPW + \
	((dp)->d_namlen + 1) + (NBPW-1)) &  \
			~(NBPW-1))

/* ----- Directory record length given name */

#define	SAM_DIRLEN(nm) \
	((sizeof (struct sam_dirent)-NBPW + (strlen(nm) + 1) + (NBPW-1)) &  \
			~(NBPW-1))

/* ----- Directory record length given name */

#define	SAM_DIRLEN_MIN \
	((sizeof (struct sam_dirent)-NBPW + (2) + (NBPW-1)) &  \
			~(NBPW-1))

/* ----- Return inode number, given hash entry. (Directory Cache) */

#define	SAM_H_TO_INO(h)	\
	((uint32_t)((h) & UINT_MAX))

/* ----- Return space length given hash entry. (Directory Cache) */

#define	SAM_H_TO_LEN(h)	\
	((uint32_t)((h) & UINT_MAX))

/* ----- Return directory offset given hash entry. (Directory Cache) */

#define	SAM_H_TO_OFF(h)	\
	((off_t)((h) >> 32))

/* ----- Return hash entry given directory offset. (Directory Cache) */

#define	SAM_DIR_OFF_TO_H(ino, off)	\
	((uint64_t)(((uint64_t)(off) << 32) | (ino)))

/* ----- Return hash entry given desired dir entry length. (Directory Cache) */

#define	SAM_LEN_TO_H(len, off)	\
	((uint64_t)(((uint64_t)(off) << 32) | (len)))

/* ----- Return non-zero if character string is "." or "..".  */
#define	IS_DOT_OR_DOTDOT(cp) \
	((*(cp) == '.') && ((*((cp) + 1) == '\0') || \
	((*((cp) + 1) == '.') && (*((cp) + 2) == '\0'))))

/* ----- SAM File-system empty directory entry ("." and "..").  */
/* Minimum allocated name length is NBPW (4) */

struct sam_empty_dir {
	struct sam_dirent	dot;	/* "."  */
	struct sam_dirent	dotdot;	/* ".." */
};



/*
 * ----- File system independent defines. These defines depend on
 *		/usr/include/sys/dirent.h not changing.
 */

#define	FS_HDR	(sizeof (ino64_t) + sizeof (off64_t) + sizeof (uint16_t))

#define	FS_DIRSIZ(DP) \
	(((FS_HDR) + ((DP)->d_namlen + 1) + (NBPLW-1)) & ~(NBPLW-1))


/* ------ Format flags for sam_getdents. */

typedef enum {FMT_FS, FMT_SAM} fmt_fs_t;


/* ------ Operation flags for sam_lookup_name, sam_create & sam_remove. */

enum sam_op {SAM_CREATE, SAM_MKDIR, SAM_REMOVE, SAM_RMDIR, SAM_SYMLINK,
			SAM_RESTORE, SAM_FORCE_LOOKUP,
			SAM_LINK, SAM_RENAME_LINK,
			SAM_RENAME_LOOKUPNEW};


/* ------ Sam directory entry returned from lookup. */

enum name_type {
	SAM_NULL,	/* No entry set */
	SAM_ENTRY,	/* A directory entry was returned */
	SAM_EMPTY_SLOT,	/* An empty slot big enough was found */
	SAM_BIG_SLOT	/* A full slot big enough was found */
};

struct sam_name {
	enum sam_op operation;	/* Operation for this entry */
	enum name_type type;	/* Entry type */
	boolean_t need_slot;	/* SAM_CREATE|SAM_MKDIR|SAM_RESTORE|SAM_LINK */
	union {
		/* Set for SAM_ENTRY entry */
		struct {
			offset_t offset;	/* Offset for this entry */
		} entry;
		/* Set for SAM_BIG_SLOT & SAM_EMPTY_SLOT */
		struct {
			offset_t offset;	/* Offset for the empty ent */
			uint16_t reclen;	/* Rec len of the empty ent */
		} slot;
	} data;
	uint64_t slot_handle;	/* Slot handle (Directory Cache) */
	int client_ord;
};

#endif	/* _SAM_FS_DIRENT_H */
