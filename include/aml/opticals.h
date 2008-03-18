/*
 * opticals.h - defines and stuff for optical processing
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

#if !defined(_AMLOPTICALS_H)
#define	_AMLOPTICALS_H

#pragma ident "$Revision: 1.11 $"

/* Define the default optical sector size */
#define	OD_SS_DEFAULT 1024

/* Define the optical block size */
#define	OD_BS_DEFAULT (1024 * 1024)

/* Define the min and max sector sizes */
/* Note: Code and drive can't handle smaller than 512 sector size */
#define	MIN_OPTICAL_SECTOR_SIZE    512		/* Do NOT change */
#define	MAX_OPTICAL_SECTOR_SIZE    16384

/* Defines for optical position constants */
/* Note: Do NOT change these -- ANSI standard */
#define	OD_ANCHOR_POS 256		/* optical anchor block position */
#define	OD_SPACE_UNAVAIL (OD_ANCHOR_POS*2)	/* anchor space is not usable */

#endif /* !defined(_AMLOPTICALS_H) */
