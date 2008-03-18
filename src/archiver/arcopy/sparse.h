/*
 * sparse.h - Sparse file archiving definitions.
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

#ifndef SPARSE_H
#define	SPARSE_H

#pragma ident "$Revision: 1.14 $"

struct Extent {
	fsize_t start;
	fsize_t count;
};

struct FileStoreInfo {
	fsize_t		f_size;		/* official file size */
	fsize_t		f_nbytes;	/* number of storage bytes */
	int		f_nextents;	/* number of extents */
	struct Extent	f_ex[1];	/* more than one if sparse */
};

extern struct FileStoreInfo *Fsip;

#endif /* SPARSE_H */
