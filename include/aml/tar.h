/*
 * tar.h -  SAM-QFS tar constants
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

#ifndef _SAMFS_TAR_H
#define	_SAMFS_TAR_H

#ifdef sun
#pragma ident "$Revision: 1.8 $"
#endif

/*
 * A "block" is a big chunk of stuff that we do I/O on.
 * A "record" is a piece of info that we care about.
 * Typically many "record"s fit into a "block".
 */
#define	TAR_RECORDSIZE 512
#define	NAMSIZ  100
#define	TUNMLEN  32
#define	TGNMLEN  32
#define	SPARSE_EXT_HDR  21
#define	SPARSE_IN_HDR 4

/* The checksum field is filled with this while the checksum is computed. */
#define	CHKBLANKS "        "	/* 8 blanks, no null */

/* The magic field is filled with this if uname and gname are valid. */
#define	TMAGIC  "ustar  "		/* 7 chars and a null */

/* The linkflag defines the type of file. */
#define	LF_OLDNORMAL '\0'		/* Normal disk file, Unix compat */
#define	LF_NORMAL	'0'		/* Normal disk file */
#define	LF_LINK		'1'		/* Link to previously dumped file */
#define	LF_SYMLINK	'2'		/* Symbolic link */
#define	LF_CHR		'3'		/* Character special file */
#define	LF_BLK		'4'		/* Block special file */
#define	LF_DIR		'5'		/* Directory */
#define	LF_FIFO		'6'		/* FIFO special file */
#define	LF_CONTIG	'7'		/* Contiguous file */
/* Further link types may be defined later. */

/*
 * Note that the standards committee allows only capital A through
 * capital Z for user-defined expansion.  This means that defining something
 * as, say '8' is a *bad* idea.
 */
#define	LF_DUMPDIR 'D'	/* This is a dir entry that contains the names of */
			/* files that were in the dir at the time the */
			/* dump was made */
#define	LF_LONGLINK 'K'	/* Identifies the NEXT file on the tape as having */
			/* a long linkname */
#define	LF_LONGNAME 'L'	/* Identifies the NEXT file on the tape as having */
			/* a long name. */
#define	LF_MULTIVOL 'M'	/* This is the continuation of a file that began */
			/* on another volume */
#define	LF_NAMES 'N'	/* For storing filenames that didn't fit in 100 */
			/* characters */
#define	LF_PARTIAL 'P'	/* Partial data portion of a normal disk file */
#define	LF_SPARSE 'S'	/* This is for sparse files */
#define	LF_VOLHDR 'V'	/* This file is a tape/volume header */
			/* Ignore it on extraction */
#define	LF_DIRHDR 'Y'	/* This is a SAM-FS directory header - skip it */

#endif /* _SAMFS_TAR_H */
