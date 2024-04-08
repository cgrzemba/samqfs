/*
 *    extent.c - SAM-QFS file system get_extent code.
 *
 *	Common code for filesystem, sammkfs, and samfsck.
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

#ifdef sun
#pragma ident "$Revision: 1.25 $"
#endif


/* ----- Include Files ---- */

#ifdef	sun

#include <sys/types.h>
#include <sys/param.h>
#include <sys/mutex.h>
#include <sys/errno.h>

#endif	/* sun */

#ifdef	linux
#ifdef  __KERNEL__

#include <linux/types.h>
#include <linux/sched.h>
#include <asm/errno.h>

#else	/* !__KERNEL__ */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>

#endif	/* __KERNEL__ */
#endif	/* linux */

#include <sam/types.h>
#include <sam/param.h>

#include "block.h"
#include "ino.h"
#include "trace.h"
#include "indirect.h"
#include "sblk.h"



/*
 * ----- sam_get_extent - Get offset extent location.
 *  Compute where the offset is located in the extent array.
 *  Note: The kptr array elements are initialized as -1 in reclaim.c
 *        as an indicator of whether indirects need to be removed.
 */

int						/* ERRNO, else 0 if success */
sam_get_extent(
	struct sam_get_extent_args *args,	/* arguments used */
	int kptr[],				/* out: Indirect extent locs */
	struct sam_get_extent_rval *rval)	/* arguments returned */
{
	sam_dau_t *dau;		/* Pointer to DAU table */
	int num_group;		/* No of elems in stripe group for this ord */
	ino_st_t status;	/* Ino status */
	offset_t offset;	/* Logical byte offset in file (longlong_t) */
	int extent, dt, bt;
	int ileft = 0;
	offset_t dau_blks, bn, dir_blks, tmp_bn;

	offset = args->offset;
	if (offset < 0)
		return (EFBIG);
	dau			= args->dau;
	num_group	= args->num_group;
	status		= args->status;

	bn = offset >> SAM_DEV_BSHIFT;	/* Convert to units of kilobytes */

	/*
	 * Adjust dau pointer for data disk (dt=DD) or meta disk (dt=MM).
	 * Set number of kilobyte blocks in a large extent and adjust
	 * based on the number of elements in a striped group.
	 */
	dt = status.b.meta;
	dau += dt;
	bt = LG;
	dau_blks = dau->kblocks[LG];
	if (num_group > 1) {
		dau_blks *= num_group;
	}

	/*
	 * Find number of SAM device blocks (kilobytes) in the direct extents.
	 */
	if (dau->dif_shift[LG] && (num_group == 1)) {
		dir_blks = (status.b.on_large ? (NDEXT << dau->dif_shift[LG]) :
		    ((NSDEXT << dau->dif_shift[SM]) +
		    (NLDEXT << dau->dif_shift[LG])));
	} else {
		dir_blks = NDEXT * dau_blks;
	}

	/*
	 * Convert logical offset to block offset. Set number of extents
	 * left in this "like" group.
	 */
	if (bn < dir_blks) {		/* Block is in direct extents */
		if (status.b.on_large) {
			/* No small extents, convert to units of large daus */
			if (dau->dif_shift[LG] && (num_group == 1)) {
				extent = bn >> dau->dif_shift[LG];
			} else {
				extent = bn / dau_blks;
			}
			ileft = NDEXT - extent;
		} else if (bn < (NSDEXT << dau->dif_shift[SM])) {
			/* The offset is in the small extents */
			bt = SM;
			extent = bn >> dau->dif_shift[SM];
			ileft = NSDEXT - extent;
		} else {
			/* The offset is in the large extents */
			extent = ((bn - (NSDEXT << dau->dif_shift[SM])) >>
			    dau->dif_shift[LG]) + NSDEXT;
			ileft = NDEXT - extent;
		}

	} else {
		/*
		 * Block is in indirect extents.  Convert bn to units of large
		 * allocations factoring out the number of SAM device blocks in
		 * the direct extents.
		 */
		if (dau->dif_shift[LG] && (num_group == 1)) {
			bn = (bn - dir_blks) >> dau->dif_shift[LG];
		} else {
			bn = (bn - dir_blks) / dau_blks;
		}
		if (bn >= DEXT) {	/* if in second indirect */
			tmp_bn = (bn >> DEXTSHIFT) - 1LL;
			if (tmp_bn >= DEXT) {	/* if in third indirect */
				kptr[1] = (tmp_bn & (DEXT - 1));
				if (args->sblk_vers < SAMFS_SBLKV2) {
					/*
					 * A bug exists in 3.5.0 and earlier
					 * superblocks which miscalculates
					 * this index by one. Continue this
					 * functionality, if in version 1
					 * superblock.
					 */
					kptr[1]++;
					if (kptr[1] >= DEXT)
						return (EFBIG);
				}
				tmp_bn = (tmp_bn >> DEXTSHIFT);
				kptr[0] = tmp_bn = tmp_bn - 1LL;
				kptr[2] = bn & (DEXT - 1);
				extent = NDEXT + 2;
				if (tmp_bn >= DEXT) {	/* if too big */
					return (EFBIG);
				}
			} else {
				kptr[0] = (int)tmp_bn;
				kptr[1] = bn & (DEXT - 1);
				extent = NDEXT + 1;
			}
		} else {
			kptr[0] = (int)bn;
			extent = NDEXT;
		}
	}
	*(rval->de) = extent;
	*(rval->dtype) = dt;
	*(rval->btype) = bt;
	*(rval->ileft) = ileft;
	return (0);
}
