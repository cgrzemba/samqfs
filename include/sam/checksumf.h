/*
 * checksumf.h - checksum function(and array) declarations.
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
#ifndef _SAM_CHECKSUMF_H
#define	_SAM_CHECKSUMF_H

#ifdef sun
#pragma ident "$Revision: 1.18 $"
#endif


typedef void (*csum_func)();

/* The checksum processing functions */
/* A non-null cookie (pointer) indicates an initialization pass. */

/* 0 */
extern void cs_empty(uint64_t *cookie, uchar_t *buf, int len, csum_t *val);

/* 1 */
extern void cs_simple(uint64_t *cookie, uchar_t *buf, int len, csum_t *val);

/* user */
extern void cs_user(uint64_t *cookie, int algo, uchar_t *buf,
	int len, csum_t *val);

/* repair function for cs_simple() */
extern void cs_repair(uchar_t *csum, uint64_t *cookie);

#if defined(DEC_INIT) && !defined(lint)
csum_func csum_fns[CS_FUNCS] = {
	cs_empty,
	cs_simple
};
#else	/* defined(DEC_INIT) */
extern csum_func csum_fns[CS_FUNCS];
#endif	/* defined(DEC_INIT) */

#endif /* _SAM_CHECKSUMF_H */
