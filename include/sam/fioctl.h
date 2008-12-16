/*
 * sam_fioctl.h - Ioctl(2) file definitions.
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

#ifndef _SAM_FIOCTL_H
#define	_SAM_FIOCTL_H

#ifdef sun
#pragma ident "$Revision: 1.32 $"
#endif

#ifndef	linux
#include <sys/ioccom.h>
#endif	/* linux */
#include "sam/types.h"
#include "sam/resource.h"


/* ----- Common file ioctl(2) commands. Ioctl issued on opened file. */

/* Listio read and write command. */

typedef struct sam_listio {
	int64_t		*wait_handle;	/* Set to wait for I/O completion */
	int32_t		mem_list_count;	/* Number entries in the mem arrays */
	int32_t		file_list_count; /* Number entries in the file arrays */
	void		**mem_addr;
	size_t		*mem_count;
	offset_t	*file_off;
	offset_t	*file_len;
} sam_listio_t;

#if defined(_SYSCALL32)
/*
 * ILP32 version of listio, used in sam_proc_listio() to support 32-bit app in
 * LP64 kernel
 */
typedef struct sam_listio32 {
	caddr32_t	wait_handle;	/* Set to wait for I/O completion */
	int32_t		mem_list_count;	/* Number entries in the mem arrays */
	int32_t		file_list_count; /* Number entries in the file arrays */
	caddr32_t	mem_addr;
	caddr32_t	mem_count;
	caddr32_t	file_off;
	caddr32_t	file_len;
} sam_listio32_t;

#endif  /* _SYSCALL32 */

#define	C_LISTIO_WR 3		/* Listio write file access call */
#define	F_LISTIO_WR _IO('f', C_LISTIO_WR)

#define	C_LISTIO_RD 4		/* Listio read file access call */
#define	F_LISTIO_RD _IO('f', C_LISTIO_RD)

#define	C_LISTIO_WAIT 5		/* Wait for Listio file access call */
#define	F_LISTIO_WAIT _IO('f', C_LISTIO_WAIT)


/* Advise ioctl. */

#define	C_ADVISE 6			/* Advise file attributes */
#define	F_ADVISE _IOW('f', C_ADVISE, int *)


/* Lockfs -- uses same ioctl as UFS so application does not change. */

#define	C_FIOLFS 64		/* Lock/Unlock file system */
#ifndef _FIOLFS
#define	_FIOLFS _IO('f', C_FIOLFS)
#endif

#define	C_FIOLFSS 65	/* Get file system lock status */
#ifndef _FIOLFSS
#define	_FIOLFSS _IO('f', C_FIOLFSS)
#endif

/* Directio -- uses same ioctl as UFS so application does not change. */

#ifndef DIRECTIO_OFF
#define	DIRECTIO_OFF 0
#define	DIRECTIO_ON  1
#endif

#define	C_DIRECTIO 76		/* Turn direct I/O on/off */
#ifndef _FIODIRECTIO
#define	_FIODIRECTIO _IO('f', C_DIRECTIO)
#endif

/* Begin: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack(4)
#endif


/* ----- Privileged file ioctl(2) commands. Ioctl issued on opened file. */

/* Get directory entries returned unformated (FMT_SAM). */

typedef struct {
	SAM_POINTER(struct sam_dirent) dir;	/* Directory buffer */
	int32_t		size;		/* Length of directory buffer */
	offset_t	offset;		/* Directory file offset-in/out */
	int		eof;		/* EOF found on previous read */
} sam_ioctl_getdents_t;

#define	C_GETDENTS 1
#define	F_GETDENTS _IOWR('P', C_GETDENTS, sam_ioctl_getdents_t)


/* Stage write issued by stager daemon only. */

#define	ST_locked_buffer 1	/* Stage buffers mlocked */

typedef struct {
	offset_t	offset;		/* Byte offset to start write */
	SAM_POINTER(void) buf;		/* Location of buffer */
	int32_t		nbyte;		/* Number of bytes to write */
	int32_t		st_flags;	/* Stage flags */
} sam_ioctl_swrite_t;

#define	C_SWRITE 2
#define	F_SWRITE _IOW('P', C_SWRITE, sam_ioctl_swrite_t)


/* Update stage file size for mmap stage. Not used. */

typedef struct {
	offset_t size;			/* Current staged in size */
}   sam_ioctl_stsize_t;

#define	C_STSIZE 3
#define	F_STSIZE _IOW('P', C_STSIZE, sam_ioctl_stsize_t)


/* Samfsrestore restore inode */

/*
 * Idrestore restores the saved inode into an already created file.
 * Idrestore also returns the id of a restored file (into the perm.
 * inode id field), if the inode being restored is a directory,
 * hardlink, or a segment index inode.
 */

typedef struct sam_ioctl_idrestore {
	SAM_POINTER(void) dp;	/* Disk/archive inode */
	SAM_POINTER(void) lp;	/* Symbolic link information */
	SAM_POINTER(void) rp;	/* Resource file attr information */
	SAM_POINTER(void) vp;	/* Multivolume vsn information */
}   sam_ioctl_idrestore_t;

#define	C_IDRESTORE 4		/* Samfsrestore restore inode */
#define	F_IDRESTORE _IOW('P', C_IDRESTORE, sam_ioctl_idrestore_t)


/*
 * Unload removable media for archiver.
 * Used for processing the end of an archive file(tar file or tar container).
 * Write labels or tape mark, and return position of the start of the
 * archive file.
 */

typedef struct sam_ioctl_rmunload {
	int flags;			/* Labeling control flags */
	uint64_t position;		/* Position returned */
}   sam_ioctl_rmunload_t;

#define	UNLOAD_EOX 1		/* Write an end of section label */
#define	UNLOAD_WTM 2		/* Write just a tape mark */

#define	C_UNLOAD 5			/* Unload removable media */
#define	F_UNLOAD _IOWR('P', C_UNLOAD, sam_ioctl_rmunload_t)


/* Return removable media info (uses fd). */

typedef struct sam_ioctl_getrminfo {
	SAM_POINTER(void) buf;		/* Rminfo buffer */
	int bufsize;
}   sam_ioctl_getrminfo_t;

#define	C_GETRMINFO 6		/* Return removable media info */
#define	F_GETRMINFO _IOW('P', C_GETRMINFO, sam_ioctl_getrminfo_t)


/* Position removable media file. */

typedef struct sam_ioctl_rmposition {
	u_longlong_t	setpos;
} sam_ioctl_rmposition_t;

#define	C_RMPOSITION  7
#define	F_RMPOSITION  _IOW('P', C_RMPOSITION, sam_ioctl_rmposition_t)


/* Samfsrestore restore file */
/*
 * Samrestore restores the inode and name to the directory referenced.
 * Samfsrestore is only valid for regular file, and special files, when
 * no associated data is being restored.
 */

typedef struct sam_ioctl_samrestore {
	SAM_POINTER(struct sam_dirent) name;	/* Name entry */
	SAM_POINTER(void) lp;		/* Symbolic link information */
	SAM_POINTER(void) dp;		/* Disk/archive inode */
	SAM_POINTER(void) vp;		/* Multivolume vsn information */
} sam_ioctl_samrestore_t;

#define	C_SAMRESTORE	8		/* Create and restore a file */
#define	F_SAMRESTORE	_IOWR('P', C_SAMRESTORE, sam_ioctl_samrestore_t)


/* Flush and invalidate inode pages (uses fd). */

#define	C_FLUSHINVAL	9		/* Flush and inval pages */
#define	F_FLUSHINVAL	 _IO('P', C_FLUSHINVAL)


/* LQFS: Logging protocol */

#define	C_FIOISLOG	72
#ifndef _FIOISLOG
#define	_FIOISLOG	_IO('f', C_FIOISLOG)
#endif /* _FIOISLOG */

#define	C_FIOLOGENABLE	87
#ifndef _FIOLOGENABLE
#define	_FIOLOGENABLE	_IO('f', C_FIOLOGENABLE)
#endif /* _FIOLOGENABLE */

#define	C_FIOLOGDISABLE	88
#ifndef _FIOLOGDISABLE
#define	_FIOLOGDISABLE	_IO('f', C_FIOLOGDISABLE)
#endif /* _FIOLOGDISABLE */

#define	C_FIO_SET_LQFS_DEBUG	93
#ifndef _FIO_SET_LQFS_DEBUG
#define	_FIO_SET_LQFS_DEBUG	_IO('f', C_FIO_SET_LQFS_DEBUG)
#endif /* _FIO_SET_LQFS_DEBUG */

typedef struct fiolog {
	uint_t	nbytes_requested;
	uint_t	nbytes_actual;
	int	error;
} fiolog_t;

#define	FIOLOG_ENONE	0
#define	FIOLOG_ETRANS	1
#define	FIOLOG_EROFS	2
#define	FIOLOG_EULOCK	3
#define	FIOLOG_EWLOCK	4
#define	FIOLOG_ECLEAN	5
#define	FIOLOG_ENOULOCK	6
#define	FIOLOG_ENOTSUP	7
#define	FIOLOG_EPEND	99	/* Journaling state (on/off) is not yet known */


/* End: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack()
#endif

#endif /* _SAM_FIOCTL_H */
