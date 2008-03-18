/*
 *  setdau.c - common function for kernel and commands mkfs and fsck.
 *
 *		Routines used in both kernel and non-kernel portions of
 *		SAM-FS.
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

#ifdef sun
#pragma ident "$Revision: 1.17 $"
#endif

/*
 * ----- Include Files ----
 */

#ifdef	linux

#ifdef	__KERNEL__
#include <linux/types.h>
#include <linux/sched.h>
#else
#include <sys/types.h>
#include <sys/param.h>
#endif	/* __KERNEL__ */

#else	/* linux */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/mutex.h>

#endif	/* linux */

#include <sam/types.h>
#include <sam/param.h>

#include "block.h"
#include "ino.h"

/*
 * ----- sam_set_dau - Set the disk dau parameters.
 *
 */

void
sam_set_dau(
	sam_dau_t *dau,
	int	sm_kblocks,
	int	lg_kblocks)
{
	int bt, j, shift, value;

	dau->kblocks[SM] = sm_kblocks;
	dau->kblocks[LG] = lg_kblocks;
	for (bt = 0; bt < SAM_MAX_DAU; bt++) {
		dau->size[bt]  = dau->kblocks[bt] * SAM_DEV_BSIZE;
		dau->wsize[bt] = dau->kblocks[bt] * SAM_DEV_WSIZE;
		shift = SAM_DEV_BSHIFT;
		value = SAM_DEV_BSIZE;
		for (;;) {
			if (value > dau->size[bt]) {
				shift = 0;
				break;
			}
			if (value == dau->size[bt])  break;
			shift++;
			value <<= 1;
		}
		if (shift) {
			dau->shift[bt] = shift;
			dau->dif_shift[bt] = dau->shift[bt] - SAM_DEV_BSHIFT;
			dau->mask[bt] = dau->size[bt] - 1;
		}
		dau->blocks[bt] = dau->kblocks[bt] / SAM_LOG_BLOCK;
		dau->seg[bt] = dau->size[bt];
		if (bt == LG) dau->seg[bt] = MAXBSIZE;
	}
	if (dau->kblocks[SM]) {
		dau->sm_blkcount = dau->kblocks[LG] / dau->kblocks[SM];
	} else {
		dau->sm_blkcount = 0;
	}
	dau->sm_bits = 0;
	for (j = 0; j < dau->sm_blkcount; j++) {
		dau->sm_bits = (dau->sm_bits << 1) | 1;
	}
	dau->sm_bmask = dau->kblocks[LG] - 1;
	dau->sm_off = NSDEXT * dau->size[SM];

#if defined(PRINT_INFO)
#if defined(_KERNEL)
	cmn_err(CE_NOTE, "--------------\n");
	cmn_err(CE_NOTE, "KBLOCK = (%d,%d)\n",
	    dau->kblocks[SM], dau->kblocks[LG]);
	cmn_err(CE_NOTE, "BLOCK = (%d,%d)\n",
	    dau->blocks[SM], dau->blocks[LG]);
	cmn_err(CE_NOTE, "BLK = (%d,%d)\n",
	    dau->size[SM], dau->size[LG]);
	cmn_err(CE_NOTE, "WBLK = (%d,%d)\n",
	    dau->wsize[SM], dau->wsize[LG]);
	cmn_err(CE_NOTE, "SHIFT = (%d,%d)\n", dau->shift[SM], dau->shift[LG]);
	cmn_err(CE_NOTE, "DIF_SHIFT = (%d,%d)\n",
	    dau->dif_shift[SM], dau->dif_shift[LG]);
	cmn_err(CE_NOTE, "SEG = (%d,%d)\n",
	    dau->seg[SM], dau->seg[LG]);
	cmn_err(CE_NOTE, "MASK = (%x,%x)\n", dau->mask[SM], dau->mask[LG]);
	cmn_err(CE_NOTE, "SM_BMASK = %x\n", dau->sm_bmask);
	cmn_err(CE_NOTE, "SM_BITS = %x\n", dau->sm_bits);
	cmn_err(CE_NOTE, "SM_OFF = %x\n", dau->sm_off);
#else /* _KERNEL */
	printf("-----------\n");
	printf("KBLOCK = (%d,%d)\n", dau->kblocks[SM], dau->kblocks[LG]);
	printf("BLOCK = (%d,%d)\n", dau->blocks[SM], dau->blocks[LG]);
	printf("BLK = (%d,%d)\n", dau->size[SM], dau->size[LG]);
	printf("WBLK = (%d,%d)\n", dau->wsize[SM], dau->wsize[LG]);
	printf("SHIFT = (%d,%d)\n", dau->shift[SM], dau->shift[LG]);
	printf("DIF_SHIFT = (%d,%d)\n", dau->dif_shift[SM],
	    dau->dif_shift[LG]);
	printf("MASK = (%llx,%llx)\n", dau->mask[SM], dau->mask[LG]);
	printf("SM_BMASK = %x\n", dau->sm_bmask);
	printf("SM_BITS = %x\n", dau->sm_bits);
	printf("SM_OFF = %x\n", dau->sm_off);
	printf("SM_BLKCNT = %x\n", dau->sm_blkcount);
#endif /* _KERNEL */
#endif /* PRINT_INFO */
}
