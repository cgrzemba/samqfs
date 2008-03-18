/*
 * examlist.h - Arfind examlist definitions.
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

#ifndef EXAMLIST_H
#define	EXAMLIST_H

#pragma ident "$Revision: 1.6 $"

/* Macros. */
#define	EXAMLIST_MAGIC 0530011514	/* Examlist file magic number */
#define	EXAMLIST_VERSION 70219		/* Examlist file version (YMMDD) */

/* The examine list. */
/* A memory mapped file. */

struct ExamList {
	MappedFile_t El;
	int	ElVersion;		/* Version */
	size_t	ElSize;			/* Size of all entries */
	int	ElCount;		/* Number of entries */
	int	ElFree;			/* Number of freed entries */

	struct ExamListEntry {
		sam_id_t XeId;		/* Inode number */
		sam_time_t XeTime;	/* Time to examine inode */
		ushort_t XeFlags;
		ushort_t XeEvent;
	} ElEntry[1];
};

/* Flags. */
#define	XE_free		0x0001

#endif /* EXAMLIST_H */
