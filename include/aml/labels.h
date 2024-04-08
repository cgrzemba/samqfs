/*
 * sam_labels.h - common label processing
 *
 * Description:
 * Contains structs and types common to both tape and optical label
 * processing.
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

#ifndef _AML_LABELS_H
#define	_AML_LABELS_H

#pragma ident "$Revision: 1.12 $"

#define	LEN_TAPE_VSN   6    /* length of tape vsn */
#define	LEN_OPTIC_VSN  31    /* length of optical vsn */

/* flags for label processing */

#define	LABEL_ERASE	0x01		/* erase media during labeling */
#define	LABEL_RELABEL	0x02		/* relabel */
#define	LABEL_SLOT	0x04		/* slot request to robot */
#define	LABEL_BARCODE	(0x08 | 0x04)	/* label with bar code */

typedef struct {
	int	eq;		/* equipment to label, or robot */
	int	block_size;	/* block size for this label */
	uint_t	slot;		/* slot within robot to label */
	uint_t	flags;		/* flag bits */
	int	part;		/* D2 partition id */
	char	*vsn;		/* vsn */
	char	*info;		/* information */
} label_req_t;

typedef struct {
	int	eq;
	int	block_size;
	int	ea;
	uint_t	flags;
	int	Asize;
	int	Bsize;
	int	Acount;
	int	Bcount;
	int	format;
	int	layout;
	char	*vsn;
	char	*info;
} format_req_t;


#endif	/* _AML_LABELS_H */
