/*
 * ----- common_subr.c - Common subroutines.
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

#pragma ident "$Revision: 1.19 $"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/sysmacros.h>
#ifdef _KERNEL
#include <sys/systm.h>
#endif /* _KERNEL */
#ifdef linux
#include "sam/linux_types.h"
#endif /* linux */

#include <sam/types.h>
#include <sam/mount.h>
#include "macros.h"
#include "block.h"
#include "sblk.h"
#include "trace.h"
#ifndef _KERNEL
#include "string.h"
#include "stdio.h"
#include "utility.h"
#endif /* _KERNEL */

#ifndef _KERNEL
int d_write(struct devlist *dp, char *buffer, int len, sam_daddr_t sector);
#endif /* _KERNEL */

#ifdef _KERNEL
extern void sam_fs_clear_map(void *mp, struct sam_sblk *sblk, int ord, int len);
#endif /* _KERNEL */

static void sam_clear_maps(sam_caller_t caller, struct sam_sblk *sblk,
	void *mp, int ord, int len, sam_daddr_t blk, int bits);

/*
 *  ----- sam_dir_gennamehash -
 * Given a name string, generate a 16 bit hash value.
 */

ushort_t
sam_dir_gennamehash(int nl, char *np)
{
	ushort_t hash = 0;
	static char *nulls = "\0\0\0";
	register uint_t first, second, temp;
	register short i;

	if (nl <= 2)		/* Don't bother hashing a really short name */
		return (0);
	first = second = 0;
	while (nl > 0) {
		for (i = 0, temp = 0; i < sizeof (uint_t); i++, np++) {
			temp <<= 8;
			temp |= *np;
			if (--nl <= 0)
				np = nulls;
		}
		first ^= temp;
		if (nl > 0) {
			for (i = 0, temp = 0; i < sizeof (uint_t); i++, np++) {
				temp <<= 8;
				temp |= *np;
				if (--nl <= 0)
					np = nulls;
			}
			second ^= temp;
		}
	}
	temp = (((first - 1) * 7) + (first >> 8)) - 1;
	temp = ((temp * 7) + (first >> 16)) - 1;
	temp = ((temp * 7) + (first >> 24)) - 1;
	temp = ((temp * 7) + second) - 1;
	temp = ((temp * 7) + (second >> 8)) - 1;
	temp = ((temp * 7) + (second >> 16)) - 1;
	temp = ((temp * 7) + (second >> 24)) - 1;

	hash = temp & 0xFFFF;
	temp >>= 16;
	hash |= temp & 0xFFFF;
	return (hash);
}


/*
 *  ----- sam_init_sblk_dev -
 */
void
sam_init_sblk_dev(
	struct sam_sblk *sblk,
	sblk_args_t *args)
{
	int dt, mdt;
	struct sam_sbinfo *sbp = &sblk->info.sb;
	struct sam_sbord *sop = &sblk->eq[args->ord].fs;
	struct sam_sbord *somp;

	dt = (args->type == DT_META) ? MM : DD;
	if (sop->dau_next == 0) {
		if (sbp->mm_count == 0) {
			sop->dau_next = SUPERBLK + args->kblocks[dt];
		} else {
			sop->dau_next = SUPERBLK + (sizeof (sam_sblk_t) /
			    SAM_DEV_BSIZE);
		}
		if (SBLK_MAPS_ALIGNED(sbp)) {
			sop->dau_next = roundup(sop->dau_next,
			    args->kblocks[dt]);
		}
	}
	sop->capacity = 0;
	sop->space = 0;
	sop->type = args->type;

	if (sop->num_group) {
		sop->capacity = args->blocks * args->kblocks[dt] *
		    sop->num_group;
		sop->space = sop->capacity;
		sop->dau_size = args->blocks;
		sop->l_allocmap = howmany(sop->dau_size,
		    (SAM_DEV_BSIZE * NBBY));

		if ((sbp->mm_count == 0) ||
		    ((dt == MM) && SBLK_MAPS_ALIGNED(sbp))) {
			/*
			 * ms file system, no metadata devices. Maps on same
			 * device.
			 * ma file system, metadata devices have their own maps.
			 */
			sop->mm_ord = args->ord;
			somp = sop;
			mdt = dt;
		} else {
			/*
			 * ma file system, metadata devices. Maps roundrobin
			 * on meta devices.
			 */
			sop->mm_ord = sbp->mm_ord;
			somp = &sblk->eq[sop->mm_ord].fs;
			mdt = MM;
		}
		sop->allocmap = somp->dau_next;
		somp->dau_next = sop->allocmap + sop->l_allocmap;
		if (SBLK_MAPS_ALIGNED(sbp)) {
			somp->dau_next = roundup(somp->dau_next,
			    args->kblocks[mdt]);
		}

		/*
		 * Advance the metadata ordinal for ma file system.
		 */
		if (sbp->mm_count) {
			for (;;) {
				sbp->mm_ord++;
				if (sbp->mm_ord >= sbp->fs_count) {
					sbp->mm_ord = 0;
				}
				if ((sblk->eq[sbp->mm_ord].fs.type !=
				    DT_META) ||
				    (sblk->eq[sbp->mm_ord].fs.allocmap == 0)) {
					continue;
				}
				break;
			}
		}
	}
}


/*
 * ----- sam_bfmap - Build free maps.
 *
 *	Initialize the small and large free maps on the specified disk.
 */

int				/* ERRNO if error, 0 if successful */
sam_bfmap(
	sam_caller_t caller,	/* Caller - FS, MKFS, or FSCK */
	struct sam_sblk *sblk,	/* Pointer to the superblock */
	int ord,		/* Disk ordinal who owns the map */
#ifdef _KERNEL
	dev_t meta_dev,		/* Meta device */
#else
	struct devlist *dp,	/* Pointer to devlist for device */
#endif /* _KERNEL */
	char *dcp,		/* Pointer to the bit map buffer */
	char *cptr,		/* Pointer to mmap buffer (SAMFSCK only) */
	int bits)		/* Number of bits per log block to allocate */
{
	int ii;
	int *iptr;
	uint_t *wptr;
	int daul;
	int blocks;
	int jj;
	int kk;
	int nn;
	int d_dau;
	int err = FALSE;

	/* build large dau maps */

	d_dau = SAM_DEV_BSIZE * bits;
	iptr = (int *)(void *)dcp;
	for (ii = 0; ii < (d_dau / NBPW); ii++) {
		*iptr++ = -1;
	}
	daul = sblk->eq[ord].fs.l_allocmap;		/* number of blocks */
	blocks = sblk->eq[ord].fs.dau_size * bits;	/* no. of bits */

	for (ii = 0; ii < daul; ii++, cptr += d_dau) {
#ifdef _KERNEL
		buf_t *bp;
		sam_daddr_t block;

		block = (sblk->eq[ord].fs.allocmap + ii) << SAM2SUN_BSHIFT;
		bp = getblk(meta_dev, block, SAM_DEV_BSIZE);
		cptr = (char *)(void *)bp->b_un.b_addr;
		bcopy(dcp, cptr, d_dau);
#else
		if (caller == SAMFSCK_CALLER) {
			memcpy(cptr, dcp, d_dau);
		}
#endif /* _KERNEL */

		if (ii == (daul - 1)) {
			jj = blocks / 32;
			if (caller == SAMFSCK_CALLER) {
				wptr = (uint_t *)(void *)(cptr + (jj * NBPW));
			} else if (caller == SAMFS_CALLER) {
				wptr = (uint_t *)(void *)(cptr + (jj * NBPW));
			} else {
				wptr = (uint_t *)(void *)
				    (dcp + (jj * NBPW));
			}
			kk = blocks & 0x1f;
			if (kk) {
				*wptr = sam_clrbits(*wptr,
				    (31 - kk), (32 - kk));
				jj++;
				wptr++;
			}
			for (nn = jj; nn < (d_dau / NBPW); nn++) {
				*wptr++ = 0;
			}
		}
		blocks -= (d_dau * NBBY);
#ifdef _KERNEL
		bwrite2(bp);
		if ((bp->b_flags & B_ERROR) != 0) {
			err = TRUE;
		}
		brelse(bp);
		if (err) {
			break;
		}
#else
		if (caller == SAMMKFS_CALLER) {
			if (d_write(dp, (char *)dcp, 1,
			    (int)(sblk->eq[ord].fs.allocmap + ii))) {
				err = TRUE;
				break;
			}
		}
#endif /* _KERNEL */
	}
	return (err);
}


/*
 * ----- sam_cablk - Clear allocated blocks.
 *
 *	Clear bit maps for allocated blocks for this ord.
 */

int			/* 0 if successful, 1 if error */
sam_cablk(
	sam_caller_t caller,	/* Caller - FS, MKFS, or FSCK */
	struct sam_sblk *sblk,	/* Pointer to the superblock */
	void *mp,		/* NULL Pointer if cmd, mount pointer if fs */
	int ord,		/* Disk ordinal */
	int bits,		/* No of ord bits to clear */
	int mbits,		/* No of mord bits to clear */
	uint_t dd_kblocks,
	uint_t mm_kblocks,
	int *lenp)
{
	struct sam_sbinfo *sbp = &sblk->info.sb;
	sam_daddr_t blk;	/* Block number */
	int minlen;		/* Length of allocated area prior to round-up */
	int system;		/* Length of allocated area in super block */
	int sblk_size;		/* size of super block  (in 1024 byte blocks) */
	int mm_ord;


	system = sblk->eq[ord].fs.system;
	sblk_size = sizeof (struct sam_sblk) >> SAM_DEV_BSHIFT;
	if (sbp->mm_count == 0) {
		int len2;	/* Length of allocated area for sblk 2 */
		int len3;	/* Length of allocated area for sblk 3 */

		/*
		 * "ms" file system - no meta devices; data & metadata
		 * on same devices
		 */
		len3 = SUPERBLK + dd_kblocks;
		len3 = roundup(len3, dd_kblocks);
		len3 += sblk->eq[ord].fs.l_allocmap;
		len3 = roundup(len3, dd_kblocks);
		if (ord == 0) {
			/* Last super block */
			len3 += dd_kblocks;
			len3 = roundup(len3, dd_kblocks);
		}
		len3 = roundup(len3, dd_kblocks);

		len2 = SUPERBLK + dd_kblocks;
		len2 += sblk->eq[ord].fs.l_allocmap;
		if (ord == 0) {
			/* Last super block */
			len2 += dd_kblocks;
		}
		minlen = len2;
		len2 = roundup(len2, dd_kblocks);

		/*
		 * Sanity check for length of system area for v2 and v2A
		 */
		if (SBLK_MAPS_ALIGNED(sbp)) {
			if ((system != len2) && (system != len3)) {
				*lenp = len3;
				return (1);
			}
			system = system / dd_kblocks;
		} else {		/* Superblock v2 */
			if (system != len2) {	/* Sanity check */
				*lenp = len2;
				if (system < minlen) {
					if (system != OLD_SBSIZE ||
					    dd_kblocks != OLD_SBSIZE) {
						return (1);
					}
				} else {
#ifdef	DEBUG
					if (sblk->eq[ord].fs.system != len2) {
						return (1);
					}
#endif	/* DEBUG */
					sblk->eq[ord].fs.system = len2;
				}
			}
			system = len2 / dd_kblocks;
		}
		sam_clear_maps(caller, sblk, mp, ord, system, 0, bits);

	} else {
		int len;		/* Length of allocated area */
		/*
		 * "ma" file system - meta devices;
		 * data & metadata on different devices
		 */
		if ((sblk->eq[ord].fs.type == DT_META) && (ord == 0)) {
			len = sbp->offset[1];	/* Last super block */
			len += mm_kblocks;
			minlen = len;
			len = roundup(len, mm_kblocks);
			if (system != len) {	/* Sanity check */
				*lenp = len;
				if (system < minlen) {
					if (system != OLD_SBSIZE ||
					    mm_kblocks != OLD_SBSIZE) {
						return (1);
					}
				} else {
#ifdef	DEBUG
					if (sblk->eq[ord].fs.system != len) {
						return (1);
					}
#endif	/* DEBUG */
					sblk->eq[ord].fs.system = len;
				}
			}
			len = len / mm_kblocks;
			sam_clear_maps(caller, sblk, mp, ord, len, 0, bits);

		} else if ((sblk->eq[ord].fs.type == DT_META) && (ord != 0)) {
			len = sblk->eq[ord].fs.system / mm_kblocks;
			sam_clear_maps(caller, sblk, mp, ord, len, 0, bits);

		} else {
			len = SUPERBLK + sblk_size;
			minlen = len;
			len = roundup(len, dd_kblocks);
			if (system != len) {	/* Sanity check */
				*lenp = len;
				if (system < minlen) {
					if (system != OLD_SBSIZE ||
					    dd_kblocks != OLD_SBSIZE) {
						return (1);
					} else {
						/*
						 * use original (3.3.0) sblk len
						 */
						len = system;
					}
				} else {
#ifdef	DEBUG
					if (sblk->eq[ord].fs.system != len) {
						return (1);
					}
#endif	/* DEBUG */
					/* Reserved for system */
					sblk->eq[ord].fs.system = len;
				}
			}
			len = len / dd_kblocks;
			sam_clear_maps(caller, sblk, mp, ord, len, 0, bits);

			/*
			 * The add or grow case where the maps are
			 * allocated after system
			 */
			blk = sblk->eq[ord].fs.allocmap;
			mm_ord = sblk->eq[ord].fs.mm_ord;
			if (blk >= sblk->eq[mm_ord].fs.system) {
				len = sblk->eq[ord].fs.l_allocmap;
				len = howmany(len, mm_kblocks);
				blk = blk / mm_kblocks;
				if (caller != SAMFS_CALLER) {
					sam_clear_maps(caller, sblk, mp,
					    mm_ord, len, blk, mbits);
				}
			}
		}
	}
	return (0);
}


/*
 * ---- sam_clear_maps - Clear free disk blocks in maps.
 */

/* ARGSUSED */
static void
sam_clear_maps(
	sam_caller_t caller,	/* Caller - SAMMKFS or SAMFSCK */
	struct sam_sblk *sblk,	/* Pointer to the superblock */
	void *mp,		/* NULL Pointer if cmd, mount pointer if fs */
	int ord,		/* Disk ordinal to clear maps. */
	int len,		/* Number of allocation units */
	sam_daddr_t blk,	/* Location of first block */
	int bits)		/* No of ord bits to clear */
{
#ifndef _KERNEL
	int cmd_caller;

	if (caller == SAMMKFS_CALLER) {
		cmd_caller = SAMMKFS;
	} else {
		cmd_caller = SAMFSCK;
	}
	cmd_clear_maps(cmd_caller, ord, len, blk, bits);
#else
	sam_fs_clear_map(mp, sblk, ord, len);
#endif /* _KERNEL */
}
