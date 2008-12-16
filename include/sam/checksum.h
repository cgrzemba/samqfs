/*
 * checksum.h - checksum definitions.
 *
 *	Contains structures and definitions for checksum implementation.
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

#ifndef _SAM_CHECKSUM_H
#define	_SAM_CHECKSUM_H

#ifdef sun
#pragma ident "$Revision: 1.16 $"
#endif



#define	CSUM_BYTES	16	/* number of bytes processed at one time */

#define	CS_USER		128
#define	CS_USER_BIT	0x80


/*
 * The below constant declarations must correspond with the ordering of the
 * functions in the csum array, declared in checksumf.h.
 */
#define	CS_NONE		0
#define	CS_SIMPLE	1

#define	CS_FUNCS	2	/* number of SAM-defined checksum functions */

#endif /* _SAM_CHECKSUM_H */
