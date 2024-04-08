/*
 * sam_uioctl.h - Ioctl(2) utility definitions.
 *
 * Contains structures and definitions for IOCTL utility commands.
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

#ifndef _SAM_UIOCTL_H
#define	_SAM_UIOCTL_H

#ifdef sun
#pragma ident "$Revision: 1.35 $"
#endif

#ifdef sun
#include <sys/ioccom.h>
#endif	/* sun */
#include <sam/types.h>
#include <sam/param.h>
#include <sam/fs/ino.h>

/* Begin: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack(4)
#endif


/* Utility operator commands. */

/* Read mount table */
struct sam_ioctl_mount {
	SAM_POINTER(struct sam_mount) mount;	/* Mount image */
};

#define	C_MOUNT 1
#define	F_MOUNT _IOW('U', C_MOUNT, struct sam_ioctl_mount)


/* Read/write inode */
struct sam_ioctl_inode {
	sam_ino_t ino;			/* Inode number */
	int mode;			/* Disk(1)/incore(0) image flag */
	int extent_factor;		/* 1024 or 4096 */
	SAM_POINTER(struct sam_node) ip;	/* Incore inode image */
	SAM_POINTER(struct sam_perm_inode) pip;	/* Permanent inode image */
};

#define	C_RDINO 2
#define	F_RDINO _IOWR('U', C_RDINO, struct sam_ioctl_inode)

#define	C_ZAPINO 3
#define	F_ZAPINO _IOW('U', C_ZAPINO, struct sam_ioctl_inode)


/* Utility archiver commands. */

/* Set archive information */
struct sam_ioctl_setarch {
	sam_id_t id;			/* Inode number & generation number */
	media_t media;			/* Media type */
	/* The following three elements are in arch_status format */
	/* One bit for each copy */
	uchar_t sa_copies_norel;	 /* Copies before automatic release */
					/* is allowed */
	uchar_t sa_copies_rel;		/* Release file after copy is made */
	uchar_t sa_copies_req;		/* Copies required to be made */
	int copy;			/* Archive copy */
	int flags;			/* Flags */
	int error;			/* Error returned when listarch used */
	sam_timestruc_t access_time;	/* Access time of file before copy */
	sam_timestruc_t modify_time;	/* Modification time when file */
					/* was copied */
	csum_t csum;			/* Checksum value */
	sam_archive_info_t ar;
	SAM_POINTER(struct sam_vsn_section) vp;
};

enum SA_flags {
	SA_none = 0,
	SA_csummed = 0x01,		/* Valid checksum value included */
	SA_error = 0x02,		/* Error during listarch */
	SA_max = 0xff
};

#define	C_SETARCH 4
#define	F_SETARCH _IOW('u', C_SETARCH, struct sam_ioctl_setarch)


/* Return base inode stat information */
struct sam_ioctl_idstat {
	sam_id_t id;		/* Inode number & generation number */
	int size;		/* Size of inode image to be returned */
	SAM_POINTER(void) dp;	/* Disk/archive inode */
	sam_time_t time;	/* Time of day returned */
	uint32_t magic;		/* FS magic */
};

#define	C_IDSTAT 5
#define	F_IDSTAT _IOWR('u', C_IDSTAT, struct sam_ioctl_idstat)


/* Open inode */
struct sam_ioctl_idopen {
	sam_id_t id;		/* Inode number & generation number */
	sam_time_t mtime;	/* Modification time when file selected */
				/* Time of day returned for F_IDOPENDIR */
	int copy;		/* Archive copy number */
	int flags;
	SAM_POINTER(void) dp;	/* Disk inode - returned */
};

enum IDO_flags {
	IDO_none = 0,
	IDO_buf_locked = 0x01,	/* Archiver buffers are locked */
	IDO_direct_io = 0x02,	/* Use direct I/O (raw) for reading file */
	IDO_offline_direct = 0x04, /* Use direct access for offline file */
	IDO_max = 0xff
};

#define	C_IDOPEN 6
#define	F_IDOPEN _IOW('u', C_IDOPEN, struct sam_ioctl_idopen)

#define	C_IDOPENARCH 7
#define	F_IDOPENARCH _IOW('u', C_IDOPENARCH, struct sam_ioctl_idopen)


/* Read superblock table */
struct sam_ioctl_sblk {
	SAM_POINTER(struct sam_sblk) sbp;	/* Superblock image */
};

#define	C_SBLK 8
#define	F_SBLK _IOW('U', C_SBLK, struct sam_ioctl_sblk)


/* Read superblock information */
struct sam_ioctl_sbinfo {
	SAM_POINTER(struct sam_sbinfo) sbinfo;	/* Superblock base image */
};

#define	C_SBINFO 9
#define	F_SBINFO _IOW('U', C_SBINFO, struct sam_ioctl_sbinfo)


/* Set (using inode id) an inode's per archive-copy flags */
struct sam_ioctl_idscf {
	sam_id_t id;
	uint_t copy;
	int c_flags;
	int flags;		/* (SA_flags) */
};

#define	C_IDSCF 11
#define	F_IDSCF _IOW('u', C_IDSCF, struct sam_ioctl_idscf)


/* CSD update file times */
struct sam_ioctl_idtime {
	sam_id_t id;		/* Inode number & generation number */
	sam_time_t atime;	/* Access time */
	sam_time_t mtime;	/* Modification time */
	sam_time_t xtime;	/* Creation time */
	sam_time_t ytime;	/* Attribrute change time */
};

#define	C_IDTIME 12
#define	F_IDTIME _IOW('u', C_IDTIME, struct sam_ioctl_idtime)


/* Set archive done status bit */
struct sam_ioctl_archflags {
	sam_id_t id;		/* Inode number & generation number */
	int archdone;		/* 0 if clear, otherwise set it */
	int archnodrop;		/* 0 if clear, otherwise set it */
	int copies_req;		/* copies required in arch_status format */
};

#define	C_ARCHFLAGS 13
#define	F_ARCHFLAGS _IOW('u', C_ARCHFLAGS, struct sam_ioctl_archflags)


/* Open inode for directory reading. */
/*
 * A copy of the disk inode will be returned.
 */

#define	C_IDOPENDIR 14
#define	F_IDOPENDIR _IOWR('u', C_IDOPENDIR, struct sam_ioctl_idopen)


/* Return resource information */
struct sam_ioctl_idresource {
	sam_id_t id;		/* Inode number & generation number */
	int size;		/* Maximum number of vsn sections */
	SAM_POINTER(void) rp;	/* Disk resource or archive vsn information */
};

#define	C_IDRESOURCE 15
#define	F_IDRESOURCE _IOW('u', C_IDRESOURCE, struct sam_ioctl_idresource)


/* Stage file by ID */
struct sam_ioctl_idstage {
	sam_id_t id;		/* Inode number & generation number */
	int copy;		/* Copy number to stage in */
	int flags;		/* Flags */
};

enum IS_flags {
	IS_none = 0,
	IS_wait = 0x01		/* wait for completion */
};

#define	C_IDSTAGE 16
#define	F_IDSTAGE _IOW('u', C_IDSTAGE, struct sam_ioctl_idstage)


/* Return segment file inode information by ID */
struct sam_ioctl_idseginfo {
	sam_id_t id;		/* Inode number & generation number */
	offset_t offset;	/* Offset in file */
	int size;		/* Length of buffer */
	SAM_POINTER(char) buf;	/* Return segment inodes here */
};

#define	C_IDSEGINFO 17
#define	F_IDSEGINFO _IOW('u', C_IDSEGINFO, struct sam_ioctl_idseginfo)


/* Return multivolume archive inode extension information */
struct sam_ioctl_idmva {
	sam_id_t id;		/* Base inode number & generation number */
	sam_id_t aid[MAX_ARCHIVE];	/* Multivolume inode ext ids. */
	int size;			/* Size of vsn information buffer */
	int copy;			/* Copy number (-1 for all) */
	SAM_POINTER(void) buf;	/* Return multivolume archive vsn info */
};

#define	C_IDMVA 18
#define	F_IDMVA _IOW('u', C_IDMVA, struct sam_ioctl_idmva)

/*
 * #######################################################
 * ATTENTION.  The structure and definitions below are
 * shared with the Solaris source pool.  If you change it,
 * a corresponding change is necessary in
 * ~usr/src/cmd/rcm_daemon/common/samfs_rcm.c
 * #######################################################
 */

/* Return device information, for RCM */
struct sam_ioctl_dev {
	uname_t fsname;		/* File system name */
	SAM_POINTER(char) buf;	/* Return device names here */
};

#define	C_DEV 19
#define	F_DEV _IOW('U', C_DEV, struct sam_ioctl_dev)
#define	MAX_DEVS_SAMQFS 1024

/*
 * #######################################################
 * End common Solaris code.
 * #######################################################
 */

/* Panic machine. */
typedef struct sam_ioctl_panic {
	int64_t param1;		/* Information parameter 1 */
	int64_t param2;		/* Information parameter 2 */
} sam_ioctl_panic_t;

#define	C_PANIC 19
#define	F_PANIC _IOW('u', C_PANIC, struct sam_ioctl_panic)


/* Set archive information */
struct sam_ioctl_listsetarch {
	int	lsa_count;
	SAM_POINTER(struct sam_ioctl_setarch) lsa_list;
};

#define	C_LISTSETARCH 21
#define	F_LISTSETARCH _IOW('u', C_LISTSETARCH, struct sam_ioctl_listsetarch)


/* Given ino.gen, Get directory entries returned unformated (FMT_SAM). */

typedef struct {
	sam_id_t id;		/* Directory inode number & generation number */
	SAM_POINTER(struct sam_dirent) dir;	/* Directory buffer */
	int32_t		size;		/* Length of directory buffer */
	sam_timestruc_t	modify_time;	/* Modify time of directory */
	offset_t	offset;		/* Directory file offset-in/out */
	int		eof;		/* EOF found on previous read */
} sam_ioctl_idgetdents_t;

#define	C_IDGETDENTS 22
#define	F_IDGETDENTS _IOWR('u', C_IDGETDENTS, sam_ioctl_idgetdents_t)

/* Multi-VSN request for single archive copy */
#define	C_IDCPMVA 23
#define	F_IDCPMVA _IOW('u', C_IDCPMVA, struct sam_ioctl_idmva)


/* End: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack()
#endif

#endif /* _SAM_UIOCTL_H */
