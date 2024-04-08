/*
 * ----- sam/getdents.c - Process the SAM-QFS readdir function.
 *
 *	Processes the standard readdir vnode call and the ioctl getdents
 *	(F_GETDENTS) extended read directory call.
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

#pragma ident "$Revision: 1.44 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/dirent.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/fs_subr.h>
#include <sys/flock.h>
#include <sys/fbuf.h>
#include <vm/hat.h>

/* ----- SAMFS Includes */

#include "sam/param.h"
#include "sam/types.h"

#include "ino.h"
#include "inode.h"
#include "mount.h"
#include "dirent.h"
#include "extern.h"
#include "debug.h"

/*
 * ----- sam_getdents - Return a SAM directory.
 *	Read the directory and return the entries in the file system
 *	independent format if format is FMT_FS.	Return the entries in
 *	the sam_dirent format if format is FMT_SAM.
 *	Inode data_rwl RW_READER lock is set on entry.
 */

int				/* ERRNO if error, 0 if successful. */
sam_getdents(
	sam_node_t *ip,		/* directory inode to be returned. */
	uio_t *uiop,		/* user I/O vector array. */
	cred_t *credp,		/* credentials pointer */
	int	*eofp,		/* if not NULL, return eof flag . */
	fmt_fs_t format)	/* format directory entry flag. */
{
	struct fbuf *fbp;
	caddr_t	cp;
	offset_t offset;
	int	cp_size;
	int	error;
	int	length;
	int	out;
	int	rem;

	error = 0;
	offset = uiop->uio_loffset; /* Return directory entries at offset. */
	if (offset >= ip->di.rm.size) {
		if (eofp) {
			*eofp = 1;
		}
		return (0);
	}

	length = uiop->uio_iov->iov_len;	/* Length of user buffer */
	if (offset & (NBPW-1) || length <= 0) {
		return (EINVAL);
	}

	/*
	 * Allocate a directory block sized local buffer for formatting entries.
	 */
	cp_size = DIR_BLK;
	cp = (caddr_t)kmem_alloc(cp_size, KM_SLEEP);
	fbp = NULL;
	out = 0;
	ASSERT(uiop->uio_iovcnt == 1);
	if (ip->di.status.b.offline || ip->flags.b.stage_pages) {
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (ip->di.status.b.offline || ip->flags.b.stage_pages) {
			error = sam_clear_ino(ip, ip->di.rm.size,
			    MAKE_ONLINE, credp);
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			if (error) {
				goto done;
			}
		} else {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
	}

	/*
	 * Read directory and return formatted entries to user.
	 */
	rem = (cp_size < length) ? cp_size : length;
	while (offset < ip->di.rm.size) {
		int bsize;
		int	in;

		/*
		 * Read and validate directory block.
		 */
		bsize = SAM_FBSIZE(offset);
		if (bsize > DIR_BLK) {
			bsize = bsize - DIR_BLK;
		}
		if ((error = fbread(SAM_ITOV(ip), offset, bsize,
		    S_OTHER, &fbp))) {
			goto done;
		}

		error = sam_validate_dir_blk(ip, offset, bsize, &fbp);
		if (error) {
			if (error == EDOM) {
				sam_req_ifsck(ip->mp, -1,
				    "sam_getdents: EDOM",
				    &ip->di.id);
				if (!SAM_IS_SHARED_FS(ip->mp)) {
					SAMFS_PANIC_INO(ip->mp->mt.fi_name,
					    "Bad directory validation",
					    ip->di.id.ino);
				}
			}
			goto done;
		}

		/*
		 * Copy valid directory entries to local buffer.
		 */
		in = 0;
		while (in < bsize) {
			struct sam_dirent *dp;
			int	desize;
			int	reclen;

			dp = (struct sam_dirent *)(void *)(fbp->fb_addr + in);
			/* Following code duplicated in lookup.c */
			reclen = (int)dp->d_reclen;
			if (reclen <= 0 || (reclen & (NBPW-1)) ||
			    (in + reclen) > bsize) {
				error = EINVAL;		/* invalid entry */
				goto done;
			}
			if (dp->d_fmt <= SAM_DIR_VERSION) {
				/* Empty/validation entry */
				in += reclen;
				offset += reclen;
				continue;
			}
			if (dp->d_id.ino == 0 || dp->d_namlen == 0 ||
			    dp->d_namlen > reclen) {
				sam_req_ifsck(ip->mp, -1,
				    "sam_getdents: EBADSLT", &ip->di.id);
				/* Directory inode entry invalid */
				error = EBADSLT;
				goto done;
			}

			desize = (format == FMT_FS) ?
			    FS_DIRSIZ(dp) : SAM_DIRSIZ(dp);
			if (out + desize >= rem) {
				/*
				 * Entry will overflow local buffer.
				 * Copy local buffer to user.
				 */
				if (out == 0) {
					goto done;
				}
				if ((error = uiomove(cp, out, UIO_READ,
				    uiop)) != 0) {
					out = 0;
					goto done;
				}
				length -= out;
				out = 0;
				rem = (cp_size < length) ? cp_size : length;
				if (desize >= rem) {
					goto done;
				}
			}

			/*
			 * Copy entry to local buffer.
			 */
			offset += reclen;
			if (format == FMT_FS) {
				sam_dirent64_t *fs_dp;

				/*
				 * Convert to FS independent format.
				 */
				fs_dp = (sam_dirent64_t *)(void *)(cp + out);
				fs_dp->d_ino = dp->d_id.ino;
				fs_dp->d_off = offset;
				fs_dp->d_reclen = (ushort_t)desize;
				strncpy(fs_dp->d_name, (const char *)dp->d_name,
				    (dp->d_namlen+1));
			} else {
				bcopy((char *)dp, cp + out, desize);
			}
			out += desize;
			in += reclen;
		}
		fbrelse(fbp, S_OTHER);
		fbp = NULL;
	}

done:
	if (fbp) {
		fbrelse(fbp, S_OTHER);
	}
	if (out > 0 && length > 0) {
		/*
		 * Copy remainder of local buffer to user.
		 */
		if (out > length) {
			out = length;
		}
		error = uiomove(cp, out, UIO_READ, uiop);
	}
	uiop->uio_loffset = offset;
	if (eofp != NULL && error == 0) {
		*eofp = (offset >= (int)ip->di.rm.size);
	}
	kmem_free((void *)cp, cp_size);
	return (error);
}


/*
 * ----- sam_check_zerodirblk -
 * At 3.3.1-25 through 3.3.1-34 and 3.5.0-xx through 3.5.0-xy, a problem
 * in create code could have caused a zero block at the end of a directory,
 * without a validation record. sam_check_zerodirblk checks for this
 * condition.
 */

int				/* 1 returned, if a zero directory block */
sam_check_zerodirblk(sam_node_t *pip, char *cp, sam_u_offset_t offset)
{
	ulong_t *word;
	int i;

	if (offset == (pip->di.rm.size - DIR_BLK)) {
		word = (ulong_t *)(void *)cp;
		for (i = 0; i < (DIR_BLK/sizeof (ulong_t)); i++, word++) {
			if (*word != 0) {
				return (0);
			}
		}
		return (1);
	}
	return (0);
}
