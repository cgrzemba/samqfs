/*
 * bswap.h - Do byte reordering of data structures
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

#ifndef _SAM_FS_BSWAP_H
#define	_SAM_FS_BSWAP_H

#ifdef sun
#pragma ident "$Revision: 1.22 $"
#endif

typedef struct swap_descriptor {
	size_t offset;
	size_t elsize;
	size_t count;
} swap_descriptor_t;

int sam_byte_swap(swap_descriptor_t *, void *, size_t);

extern struct swap_descriptor csd_header_swap_descriptor[];
extern struct swap_descriptor csd_filehdr_swap_descriptor[];
extern struct swap_descriptor csd_header_extended_swap_descriptor[];

extern struct swap_descriptor CatalogEntry_swap_descriptor[];
extern struct swap_descriptor rmt_sam_cnt_resp_swap_descriptor[];
extern struct swap_descriptor rmt_sam_connect_swap_descriptor[];
extern struct swap_descriptor rmt_sam_req_resp_swap_descriptor[];
extern struct swap_descriptor rmt_sam_request_swap_descriptor[];
extern struct swap_descriptor rmt_sam_update_vsn_swap_descriptor[];
extern struct swap_descriptor rmt_sam_vsn_entry_swap_descriptor[];

extern struct swap_descriptor sam_san_mount_swap_descriptor[];
extern struct swap_descriptor sam_san_header_swap_descriptor[];
extern struct swap_descriptor sam_san_block_swap_descriptor[];
extern struct swap_descriptor sam_block_getbuf_swap_descriptor[];
extern struct swap_descriptor sam_block_fgetbuf_swap_descriptor[];
extern struct swap_descriptor sam_block_sblk_swap_descriptor[];
extern struct swap_descriptor sam_block_getino_swap_descriptor[];
extern struct swap_descriptor sam_block_quota_swap_descriptor[];
extern struct swap_descriptor sam_block_vfsstat_v2_swap_descriptor[];
extern struct swap_descriptor sam_fsid_swap_descriptor[];
extern struct swap_descriptor sam_san_name_swap_descriptor[];
extern struct swap_descriptor sam_name_mkdir_swap_descriptor[];
extern struct swap_descriptor sam_name_rmdir_swap_descriptor[];
extern struct swap_descriptor sam_name_create_swap_descriptor[];
extern struct swap_descriptor sam_name_remove_swap_descriptor[];
extern struct swap_descriptor sam_name_link_swap_descriptor[];
extern struct swap_descriptor sam_name_rename_swap_descriptor[];
extern struct swap_descriptor sam_name_symlink_swap_descriptor[];
extern struct swap_descriptor sam_name_acl_swap_descriptor[];
extern struct swap_descriptor sam_san_lease_swap_descriptor[];
extern struct swap_descriptor sam_san_inode_swap_descriptor[];
extern struct swap_descriptor sam_inode_samaid_swap_descriptor[];
extern struct swap_descriptor sam_inode_samarch_swap_descriptor[];
extern struct swap_descriptor sam_inode_samattr_swap_descriptor[];
extern struct swap_descriptor sam_inode_setattr_swap_descriptor[];
extern struct swap_descriptor sam_inode_stage_swap_descriptor[];
extern struct swap_descriptor sam_inode_quota_swap_descriptor[];
extern struct swap_descriptor sam_cl_attr_swap_descriptor[];
extern struct swap_descriptor sam_disk_inode_swap_descriptor[];
extern struct swap_descriptor sam_di_osd_swap_descriptor[];
extern struct swap_descriptor sam_perm_inode_swap_descriptor[];
extern struct swap_descriptor sam_perm_inode_v1_swap_descriptor[];
extern struct swap_descriptor sam_inode_ext_hdr_swap_descriptor[];
extern struct swap_descriptor sam_old_resource_swap_descriptor[];
extern struct swap_descriptor sam_old_resource_file_swap_descriptor[];
extern struct swap_descriptor sam_old_rminfo_swap_descriptor[];
extern struct swap_descriptor sam_resource_file_swap_descriptor[];
extern struct swap_descriptor sam_resource_swap_descriptor[];
extern struct swap_descriptor sam_rfa_inode_swap_descriptor[];
extern struct swap_descriptor sam_sln_inode_swap_descriptor[];
extern struct swap_descriptor sam_acl_inode_descriptor[];
extern struct swap_descriptor sam_acl_swap_descriptor[];
extern struct swap_descriptor sam_hlp_inode_descriptor[];
extern struct swap_descriptor sam_mva_inode_descriptor[];
extern struct swap_descriptor sam_osd_ext_inode_descriptor[];
extern struct swap_descriptor sam_vsn_array_swap_descriptor[];
extern struct swap_descriptor sam_vsn_section_swap_descriptor[];


#ifdef _LP64
#define		PTR_32B_ALGND(p)		((((long)p) & 03) == 0)
#define		PTR_64B_ALGND(p)		((((long)p) & 07) == 0)
#else
#define		PTR_32B_ALGND(p)		((((int)p) & 03) == 0)
#define		PTR_64B_ALGND(p)		((((int)p) & 07) == 0)
#endif /* _LP64 */


/*
 * sam_bswapX -- byte swap consecutive data elements
 *
 * Byte swap consecutive data items of size X given
 * their address and a count.
 *
 * Test for alignment, and half-words (32 bits) or words (64 bits)
 * at a time if possible.  We use shift and mask to shuffle things
 * about.
 *
 * Mildly machine dependent.  Assumes that byte pointers' alignment
 * can be determined from their least-significant bits.
 */
static inline void
sam_bswap2(void *buf, size_t count)
{
	uint16_t *op = (uint16_t *)buf;

	while (count) {
		if (count >= 4 && PTR_64B_ALGND(op)) {
			uint64_t v, *vp = (uint64_t *)(void *)op;

			/* 4 for the price of 1 */
			v = *vp;
			/* swap even and odd bytes */
			v = ((v & 0xff00ff00ff00ffLL) << 8)
			    | ((v >> 8) & 0xff00ff00ff00ffLL);
			*vp = v;
			op += 4;
			count -= 4;
		} else if (count >= 2 && PTR_32B_ALGND(op)) {
			uint32_t v, *vp = (uint32_t *)(void *)op;

			/* 2 for the price of 1 */
			v = *vp;
			/* swap even and odd bytes */
			v = ((v & 0xff00ff) << 8) | ((v >> 8) & 0xff00ff);
			*vp = v;
			op += 2;
			count -= 2;
		} else {
			uint16_t v;

			v = *op;
			/* swap bytes */
			v = (v << 8) | (v >> 8);
			*op = v;
			op += 1;
			count -= 1;
		}
	}
}


static inline void
sam_bswap4(void *buf, size_t count)
{
	uint32_t *op = (uint32_t *)buf;

	while (count) {
		if (count >= 2 && PTR_64B_ALGND(op)) {
			uint64_t v, *vp = (uint64_t *)(void *)op;

			/* 2 for the price of 1 */
			v = *vp;
			/* swap 16-bit chunks */
			v = ((v & 0xffff0000ffffLL) << 16)
			    | ((v >> 16) & 0xffff0000ffffLL);
			/* swap bytes */
			v = ((v & 0xff00ff00ff00ffLL) << 8)
			    | ((v >> 8) & 0xff00ff00ff00ffLL);
			*vp = v;
			op += 2;
			count -= 2;
		} else {
			uint32_t v;

			v = *op;
			/* swap 16-bit chunks */
			v = (v << 16) | (v >> 16);
			/* swap bytes */
			v = ((v & 0xff00ff) << 8) | ((v >> 8) & 0xff00ff);
			*op = v;
			op += 1;
			count -= 1;
		}
	}
}


static inline void
sam_bswap8(void *buf, size_t count)
{
	uint64_t *op = (uint64_t *)buf;

	while (count) {
		uint64_t v;

		v = *op;
		/* swap 32-bit chunks */
		v = (v << 32) | (v >> 32);
		/* swap 16-bit chunks */
		v = ((v & 0xffff0000ffffLL) << 16)
		    | ((v >> 16) & 0xffff0000ffffLL);
		/* swap bytes */
		v = ((v & 0xff00ff00ff00ffLL) << 8)
		    | ((v >> 8) & 0xff00ff00ff00ffLL);
		*op = v;
		op += 1;
		count -= 1;
	}
}

#endif	/* _SAM_FS_BSWAP_H */
