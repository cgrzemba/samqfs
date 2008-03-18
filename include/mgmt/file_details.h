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
#ifndef	FILE_DETAILS_H
#define	FILE_DETAILS_H

#pragma ident	"$Revision: 1.9 $"

#include <sys/types.h>
#include "pub/stat.h"	/* sam_stat */
#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"

#define	FL_OFFLINE	0x00000001
#define	FL_PARTIAL	0x00000002
#define	FL_WORM		0x00000004
#define	FL_ARCHDONE	0x00000008
#define	FL_STAGEPEND	0x00000010
#define	FL_SEGMENTED	0x00000020
#define	FL_HASDATA	0x00000040		/* restore only */
#define	FL_EXATTRS	0x00000080		/* file has extended attrs */
#define	FL_DAMAGED	0x00000100
#define	FL_INCONSISTENT	0x00000200
#define	FL_STALE	0x00000400
#define	FL_UNARCHIVED	0x00000800
#define	FL_NOTSAM	0x00001000

typedef struct {
	char		mediaType[4];
	time_t		created;
	uint32_t	flags;
	char	*vsns;			/* malloced */
} fildet_copy_t;

typedef struct {
	uint32_t	created;
	uint32_t	modified;
	uint32_t	accessed;
	uint64_t	size;
	uint32_t	flags;
	uint32_t	segnum;
	fildet_copy_t	copy[4];
} fildet_seg_t;

typedef struct {
	char	*file_name;		/* malloced */
	uint32_t	attrmod;		/* for WORM duration */
	uid_t		user;
	gid_t		group;
	mode_t		prot;
	uint32_t	segCount;
	uint32_t	segSzMB;
	uint32_t	segStageAhead;
	uint32_t	partialSzKB;
	uint16_t	samFileAtts;		/* stage/release/archive atts */
	uint8_t		file_type;
	off64_t		snapOffset;		/* restore only */
	fildet_seg_t	summary;		/* segment index, or whole */
						/* file if not segmented */
	fildet_seg_t *segments;		/* only used if segmented */
} filedetails_t;

/* used by the restore component as well as sam_file_util */
int
collect_file_details_restore(
	char	*fsname,		/* filesystem name */
	char	*snappath,
	char	*usedir,
	sqm_lst_t	*files,
	uint32_t	which_details,
	sqm_lst_t **	status);


void free_file_details(filedetails_t *details);

#endif /* FILE_DETAILS_H */
