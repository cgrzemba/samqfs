/*
 *	----- copy_extents.c -
 *
 * Move data off a device ordinal.
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#ifdef sun
#pragma ident "$Revision: 1.3 $"
#endif

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/t_lock.h>
#include <sys/sysmacros.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/cred.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/fbuf.h>
#include <sys/rwlock.h>
#include <sys/stat.h>
#include <sys/conf.h>

#include <vm/hat.h>
#include <vm/page.h>
#include <vm/pvn.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_map.h>


/* ----- SAM-QFS Includes */

#include "sam/param.h"
#include "sam/types.h"

#include "macros.h"
#include "debug.h"
#include "sblk.h"
#include "inode.h"
#include "mount.h"
#include "syslogerr.h"
#include "indirect.h"
#include "trace.h"
#include "extern.h"
#include "rwio.h"
#include "copy_extents.h"
#include "qfs_log.h"

int extent_copyout = 0;

int sam_copy_extents(sam_node_t *ip, offset_t offset, sam_map_t flag,
	sam_ioblk_t *iop, map_params_t *mapp);
extern void sam_flush_map(struct sam_mount *mp, map_params_t *mapp);

extern int sam_free_direct_map(sam_mount_t *mp, struct sam_rel_blks *block);

/*
 * ----- sam_allocate_and_copy_extent -
 *
 * Read old extent (obn.oord), allocate a new extent (nbn.nord)
 * and copy data from old to new.
 */
int
sam_allocate_and_copy_extent(
	sam_node_t *ip,
	int bt,
	int dt,
	sam_bn_t obn,
	int oord,
	buf_t **obpp,
	sam_bn_t *new_ext_bn,
	int *new_ord,
	buf_t **nbpp)
{
	sam_mount_t *mp = ip->mp;
	buf_t *obp = NULL;
	buf_t *nbp = NULL;
	sam_bn_t nbn;
	int nord;
	int error = 0;
	int retried = 0;
	int is_ino_ino = 0;
	int do_stale = 0;
	sam_bn_t sobn;
	long bsize;
	char *tbuf = NULL;
	int rsize, tresid;
	int num_group;
	int xoord, xnord, i;

	if (bt == LG) {
		bsize = LG_BLK(mp, dt);
	} else {
		bsize = SM_BLK(mp, dt);
	}

	rsize = bsize;

	if (ip->di.id.ino == SAM_INO_INO) {
		is_ino_ino++;
		if (mp->mt.fi_rd_ino_buf_size < bsize) {
			rsize = mp->mt.fi_rd_ino_buf_size;
			do_stale++;
		}
	}

	/*
	 * Allocate a new DAU.
	 */
	error = sam_alloc_block(ip, bt, new_ext_bn, new_ord);

	if (error != 0) {
		return (error);
	}

	/*
	 * Convert to 512 byte block number.
	 */
	nbn = *new_ext_bn << (mp->mi.m_bn_shift + SAM2SUN_BSHIFT);
	nord = *new_ord;

	num_group = mp->mi.m_fs[oord].num_group;

	obn <<= mp->mi.m_bn_shift;
	sobn = obn;

	for (i = 1, xnord = nord, xoord = oord;
	    i <= num_group; i++, xnord++, xoord++) {
		tresid = bsize;
		/*
		 * Get a buffer for the new DAU.
		 */
		nbp = getblk(mp->mi.m_fs[xnord].dev, nbn, bsize);

		if (nbp->b_flags & B_ERROR) {
			error = EIO;
			goto out_free;
		}

		tbuf = nbp->b_un.b_addr;

		/*
		 * Read the DAU to be copied.
		 */
		obn = sobn;
again:
		while (tresid > 0) {
			int copysize;

			if (obp) {
				brelse(obp);
				obp = NULL;
			}

			if (tresid < rsize) {
				rsize = tresid;
			}

			error = SAM_BREAD(mp, mp->mi.m_fs[xoord].dev,
			    obn, rsize, &obp);
			if (error != 0) {
				goto out_free;
			}

			if (obp->b_bcount < rsize) {
				if (is_ino_ino) {
					brelse(obp);
					error = EIO;
					goto out_free;
				}

				/*
				 * If this buffer is too small,
				 * stale it and try again.
				 */
				obp->b_flags |= B_STALE|B_AGE;
				brelse(obp);
				if (retried) {
					return (EIO);
				}
				retried++;
				goto again;
			}

			if (obp->b_bcount > rsize) {
				copysize = rsize;
			} else {
				copysize = obp->b_bcount;
			}

			/*
			 * Copy the data from the old buffer into the new one.
			 */
			memcpy(tbuf, obp->b_un.b_addr, copysize);

			tbuf += copysize;
			tresid -= copysize;
			obn += copysize >> SAM_DEV_BSHIFT;
		}

		if (obpp != NULL) {
			/*
			 * Return the old buffer.
			 */
			if (obp->b_bcount < bsize || obp->b_blkno != sobn) {
				obp->b_flags |= B_STALE|B_AGE;
				brelse(obp);

				error = SAM_BREAD(mp, mp->mi.m_fs[oord].dev,
				    sobn, bsize, &obp);
			}

			*obpp = obp;

		} else {
			/*
			 * Release the old buffer.
			 */
			brelse(obp);
			obp = NULL;
		}

		if (nbpp != NULL) {
			/*
			 * Return the new buffer.
			 * It is the caller's responsibility to
			 * write this buffer.
			 */
			*nbpp = nbp;

		} else {
			/*
			 * Write the new buffer.
			 */
			SAM_BWRITE2(mp, nbp);
			if (do_stale || !is_ino_ino) {
				nbp->b_flags |= B_STALE|B_AGE;
			}
			brelse(nbp);
			nbp = NULL;
		}
	}

	return (error);

out_free:

	sam_free_block(mp, bt, *new_ext_bn, *new_ord);
	*new_ext_bn = 0;
	*new_ord = 0;

	return (error);
}


/*
 * ----- sam_allocate_and_copy_bp -
 *
 * Old extent (obn.oord) data is in obp, allocate a new extent (nbn.nord)
 * and copy data from obp to new extent.
 */
int
sam_allocate_and_copy_bp(
	sam_node_t *ip,
	int bt,
	int dt,
	sam_bn_t obn,
	int oord,
	buf_t *obp,
	sam_bn_t *new_ext_bn,
	int *new_ord,
	buf_t **nbpp)
{
	sam_mount_t *mp = ip->mp;
	buf_t *nbp = NULL;
	sam_bn_t nbn;
	int nord;
	int error = 0;
	long bsize;
	char *tbuf = NULL;

	if (bt == LG) {
		bsize = LG_BLK(mp, dt);
	} else {
		bsize = SM_BLK(mp, dt);
	}

	if (obp->b_bcount != bsize) {
		return (EINVAL);
	}

	/*
	 * Allocate a new DAU.
	 */
	error = sam_alloc_block(ip, bt, new_ext_bn, new_ord);

	if (error != 0) {
		return (error);
	}

	/*
	 * Convert to 512 byte block number.
	 */
	nbn = *new_ext_bn << (mp->mi.m_bn_shift + SAM2SUN_BSHIFT);
	nord = *new_ord;

	/*
	 * Get a buffer for the new DAU.
	 */
	nbp = getblk(mp->mi.m_fs[nord].dev, nbn, bsize);

	if (nbp->b_flags & B_ERROR) {
		error = EIO;
		goto out_free;
	}

	tbuf = nbp->b_un.b_addr;

	/*
	 * Copy the data from the old buffer into the new one.
	 */

	memcpy(tbuf, obp->b_un.b_addr, bsize);

	if (nbpp != NULL) {
		/*
		 * Return the new buffer.
		 * It is the caller's responsibility to
		 * write this buffer.
		 */
		*nbpp = nbp;

	} else {
		/*
		 * Write the new buffer.
		 */
		SAM_BWRITE2(mp, nbp);
		brelse(nbp);
	}

	return (error);

out_free:

	sam_free_block(mp, bt, *new_ext_bn, *new_ord);
	/*
	 * Restore the original values.
	 */
	*new_ext_bn = obn;
	*new_ord = oord;
	return (error);
}


/*
 * ----- sam_allocate_and_copy_directmap	-
 *
 * Allocate a new direct map extent (nbn.nord),
 * read old direct map extent and copy data from old to new.
 */
int
sam_allocate_and_copy_directmap(
	sam_node_t *ip,
	sam_ib_arg_t *ib_args,
	cred_t *credp)
{
	sam_mount_t *mp = ip->mp;
	buf_t *obp = NULL;
	buf_t *nbp = NULL;
	sam_bn_t nbn, obn;
	int error = 0;
	int retried = 0;
	int is_ino_ino = 0;
	int do_stale = 0;
	long bsize;
	int dt;
	char *tbuf = NULL;
	int rsize, tresid;
	uchar_t new_ord;
	sam_bn_t new_ext_bn;
	offset_t alloc_length, copy_length;
	sam_bn_t save_bn;
	sam_bn_t save_bn1;
	uchar_t save_ord;
	int32_t save_blocks;
	offset_t save_size;
	sam_rel_blks_t rl;

	ASSERT(!SAM_IS_OBJECT_FILE(ip));

	dt = ip->di.status.b.meta;
	bsize = LG_BLK(mp, dt);

	rsize = bsize;

	/*
	 * Actual space allocated.
	 */
	alloc_length = ip->di.blocks << SAM_SHIFT;

	/*
	 * Amount of data actually in the file.
	 */
	copy_length = ip->di.rm.size;

	if (ip->di.id.ino == SAM_INO_INO) {
		is_ino_ino++;
		if (mp->mt.fi_rd_ino_buf_size < bsize) {
			rsize = mp->mt.fi_rd_ino_buf_size;
			do_stale++;
		}
	}

	/*
	 * Save the existing information.
	 */
	save_size = ip->di.rm.size;
	save_bn = ip->di.extent[0];
	save_bn1 = ip->di.extent[1];
	save_ord = ip->di.extent_ord[0];
	save_blocks = ip->di.blocks;

	/*
	 * Setup the structure that will
	 * be used to free the extent.
	 */
	bcopy((char *)&ip->di.extent[0],
	    (char *)&rl.e.extent[0], sizeof (sam_extents_t));
	rl.status = ip->di.status;
	rl.id = ip->di.id;
	rl.parent_id = ip->di.parent_id;
	rl.length = 0;
	rl.imode = ip->di.mode;

	/*
	 * The inode must not show an existing allocation
	 * or any data before a preallocation.
	 */
	ip->di.rm.size = 0;
	ip->di.extent[0] = 0;
	ip->di.extent[1] = 0;
	ip->di.extent_ord[0] = 0;
	ip->di.blocks = 0;

	/*
	 * Allocate new direct map extent.
	 */
	if (ib_args->new_ord >= 0) {
		ip->di.unit = ib_args->new_ord;
	}
	error = sam_map_block(ip, (offset_t)0, alloc_length,
	    SAM_ALLOC_BLOCK, NULL, credp);

	/*
	 * Restore the original size.
	 */
	ip->di.rm.size = save_size;
	if (error != 0) {
		/*
		 * Restore the original
		 * allocation information.
		 */
		ip->di.extent[0] = save_bn;
		ip->di.extent[1] = save_bn1;
		ip->di.extent_ord[0] = save_ord;
		ip->di.blocks = save_blocks;
		return (error);
	}

	/*
	 * Copy the data to the new direct map allocation.
	 */
	new_ext_bn = ip->di.extent[0];
	new_ord = ip->di.extent_ord[0];

	/*
	 * Convert to 512 byte block number.
	 */
	nbn = new_ext_bn << (mp->mi.m_bn_shift + SAM2SUN_BSHIFT);

	/*
	 * Convert to 1024 byte block number, SAM_BREAD will convert
	 * it to a 512 byte block number.
	 */
	obn = save_bn << mp->mi.m_bn_shift;

	while (copy_length > 0) {
		/*
		 * Copy bsize (DAU) size chunks until
		 * all the existing data is copied.
		 */

		/*
		 * Get a buffer for the new data
		 */
		nbp = getblk(mp->mi.m_fs[new_ord].dev, nbn, bsize);

		if (nbp->b_flags & B_ERROR) {
			error = EIO;
			goto out_restore;
		}

		tbuf = nbp->b_un.b_addr;
		tresid = bsize;

		/*
		 * Read the old data.
		 */
again:
		while (tresid > 0) {
			int copysize;

			if (obp) {
				brelse(obp);
				obp = NULL;
			}

			if (tresid < rsize) {
				rsize = tresid;
			}

			error = SAM_BREAD(mp, mp->mi.m_fs[save_ord].dev,
			    obn, rsize, &obp);
			if (error != 0) {
				goto out_restore;
			}

			if (obp->b_bcount < rsize) {
				if (is_ino_ino) {
					brelse(obp);
					error = EIO;
					goto out_restore;
				}

				/*
				 * If this buffer is too small,
				 * stale it and try again.
				 */
				obp->b_flags |= B_STALE|B_AGE;
				brelse(obp);
				if (retried) {
					error = EIO;
					goto out_restore;
				}
				retried++;
				goto again;
			}

			if (obp->b_bcount > rsize) {
				copysize = rsize;
			} else {
				copysize = obp->b_bcount;
			}

			/*
			 * Copy the data from the old buffer into the new one.
			 */
			memcpy(tbuf, obp->b_un.b_addr, copysize);

			tbuf += copysize;
			tresid -= copysize;
			obn += copysize >> SAM_DEV_BSHIFT;
			nbn += copysize >> (SAM_DEV_BSHIFT - 1);
		}

		/*
		 * Invalidate and release
		 * the old buffer.
		 */
		obp->b_flags |= B_STALE|B_AGE;
		brelse(obp);
		obp = NULL;

		/*
		 * Write the new buffer.
		 */
		SAM_BWRITE2(mp, nbp);
		if (do_stale || !is_ino_ino) {
			nbp->b_flags |= B_STALE|B_AGE;
		}
		brelse(nbp);
		nbp = NULL;

		copy_length -= bsize;
	}

	/*
	 * Free the original allocation.
	 * sam_free_direct_map() always returns 0.
	 */
	(void) sam_free_direct_map(mp, &rl);

	return (error);

out_restore:

	/*
	 * Free the new allocation.
	 * Just need the new extent allocation other
	 * info is the same.
	 */
	bcopy((char *)&ip->di.extent[0],
	    (char *)&rl.e.extent[0], sizeof (sam_extents_t));

	(void) sam_free_direct_map(mp, &rl);

	/*
	 * Restore the original allocation.
	 */
	ip->di.rm.size = save_size;
	ip->di.extent[0] = save_bn;
	ip->di.extent[1] = save_bn1;
	ip->di.extent_ord[0] = save_ord;
	ip->di.blocks = save_blocks;

	return (error);
}
