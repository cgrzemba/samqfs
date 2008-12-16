/*
 * bswap.c - Byte reorder data structures
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

#ifdef sun
#pragma ident "$Revision: 1.21 $"
#endif

/* POSIX headers. */

/* Solaris headers. */
#ifdef sun
#include "sys/vfs.h"
#endif /* sun */

#ifdef linux
#include "sam/linux_types.h"
#endif	/* linux */

/* Socket headers. */

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/param.h"
#include "bswap.h"
#include "sblk.h"
#include "samhost.h"

/* Local headers. */

/* Local data. */
#include "bswap_data.c"


/*
 * sam_byte_swap -- reorder bytes in a data structure
 *
 * Takes three arguments -- a descriptor listing the elements
 * that need reordering, a buffer address, and a length.  The
 * descriptor lists the elements in the buffer (usu. sorted by
 * offset), their sizes, and the number of consecutive elements.
 *
 * Each element is assumed to be naturally aligned -- if not,
 * then this code will break (core dump).  Each element's bytes
 * are reversed; if consecutive elements fit into whole, aligned
 * words (or half-words), we do words (half-words) at a time to
 * speed things.
 *
 * It's also possible that this code will be reworked at some
 * future date to support packing structures as well.  If that
 * happens, the descriptor structure will change substantially.
 *
 * This routine returns 0 on success, < 0 if the buffer is overrun
 * or data elements are non-power-of-2 sized.
 */
int
sam_byte_swap(
	swap_descriptor_t *ep,
	void *buf,
	size_t len)
{
	char *op;

	while (ep->elsize != 0) {
		op = (char *)buf + ep->offset;
		if (ep->offset + (ep->count * ep->elsize) > len) {
			/* object overrun */
			return (-1);
		}
		switch (ep->elsize) {
		case 1:
			break;

		case 2:
			sam_bswap2(op, ep->count);
			break;

		case 4:
			sam_bswap4(op, ep->count);
			break;

		case 8:
			sam_bswap8(op, ep->count);
			break;

		default:
			return (-1);
		}
		ep++;
	}
	return (0);
}


/*
 * Byte-swap the fields in a superblock.  Include the
 * associated sbord structures.  Return error if the
 * sbord count in the superblock is out-of-range.
 *
 * This superblock buffer might not contain all fs_count eq entries,
 * so we must stop swapping at the end of the buffer.
 *
 * XXX If this fails, we restore the superblock data to
 * XXX its original format.  We may not need to do this.
 */
int
byte_swap_sb(
	struct sam_sblk *sblk,
	size_t len)
{
	int i;
	sam_sbinfo_t *sbp = &sblk->info.sb;
	sam_sbord_t *seqp;

	if (len < sizeof (*sbp)) {
		return (-1);
	}

	sam_byte_swap(sam_sbinfo_swap_descriptor, sbp, len);

	if (sblk->info.sb.fs_count < 0 || sblk->info.sb.fs_count > L_FSET) {
		/*
		 * remove the next statement (sam_byte_swap(...)) if
		 * there's no reason to restore the original structure.
		 */
		sam_byte_swap(sam_sbinfo_swap_descriptor, sbp, sizeof (*sbp));
		return (-1);
	}
	len -= sizeof (*sbp);

	for (i = 0; i < sblk->info.sb.fs_count; i++) {
		seqp = &sblk->eq[i].fs;

		sam_byte_swap(sam_sbord_swap_descriptor, seqp, len);
		if (len < sizeof (*seqp)) {
			return (0);
		}
		len -= sizeof (*seqp);
	}

	return (0);
}


/*
 * Byte swap the fields of a shared hosts table.
 * Most of the structure is unstructured characters;
 * only the fixed-width fields in the header need to
 * be swapped.
 */
int
byte_swap_hb(struct sam_host_table_blk *hb)
{
	sam_host_table_t *htp = &hb->info.ht;

	sam_byte_swap(sam_host_table_swap_descriptor, htp, sizeof (*htp));
	return (0);
}


/*
 * Byte-swap the fields in a label block.
 */
int
byte_swap_lb(struct sam_label_blk *lb)
{
	sam_label_t *lbp = &lb->info.lb;

	sam_byte_swap(sam_label_swap_descriptor, lbp, sizeof (*lbp));
	return (0);
}
