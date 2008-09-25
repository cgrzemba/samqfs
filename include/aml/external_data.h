/*
 * external_data.h
 *
 * Definitions for data manipulation between host machine and externally
 * defined data structures.
 *
 * Multi-byte integers may be stored on external media in standard formats,
 * such as big-endian(most significant byte first) or
 * little-endian(least significant byte first).
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

#ifndef _AML_EXTERNAL_DATA_H
#define	_AML_EXTERNAL_DATA_H

#pragma ident "$Revision: 1.12 $"

/*
 * Multi-byte integer transfers between host and external data structures.
 * The arguments are pointers to the source and destination data items.
 * The items must be the same size.  If not, the transfer will not be correct.
 *
 * Examples:
 * BE32toH(s, d)  Transfer 32 bits from big-endian source 's' to host
 * destination 'd'.
 *
 * HtoLE24(s, d)  Transfer 24 bits from host source 's' to little-endian
 * destination 'd'.
 */

#define	__byte(s, x) (((uint8_t *)(s)) [x])

#define	__copy2bytes(s, d)						\
	(__byte(d, 0) = __byte(s, 0),					\
	    __byte(d, 1) = __byte(s, 1))

#define	__copy3bytes(s, d)						\
	(__byte(d, 0) = __byte(s, 0),					\
		__byte(d, 1) = __byte(s, 1),				\
	    __byte(d, 2) = __byte(s, 2))


#define	__copy4bytes(s, d)						\
	(__byte(d, 0) = __byte(s, 0),					\
	    __byte(d, 1) = __byte(s, 1),				\
	    __byte(d, 2) = __byte(s, 2),				\
	    __byte(d, 3) = __byte(s, 3))

#define	__copy8bytes(s, d)						\
	(__byte(d, 0) = __byte(s, 0),					\
	    __byte(d, 1) = __byte(s, 1),				\
	    __byte(d, 2) = __byte(s, 2),				\
	    __byte(d, 3) = __byte(s, 3),				\
	    __byte(d, 4) = __byte(s, 4),				\
	    __byte(d, 5) = __byte(s, 5),				\
	    __byte(d, 6) = __byte(s, 6),				\
	    __byte(d, 7) = __byte(s, 7))

#define	__copy2bytes_rev(s, d)						\
	(__byte(d, 0) = __byte(s, 1),					\
	    __byte(d, 1) = __byte(s, 0))

#define	__copy3bytes_rev(s, d)						\
	(__byte(d, 0) = __byte(s, 2),					\
	    __byte(d, 1) = __byte(s, 1),				\
	    __byte(d, 2) = __byte(s, 0))

#define	__copy4bytes_rev(s, d)						\
	(__byte(d, 0) = __byte(s, 3),					\
	    __byte(d, 1) = __byte(s, 2),				\
	    __byte(d, 2) = __byte(s, 1),				\
	    __byte(d, 3) = __byte(s, 0))

#define	__copy8bytes_rev(s, d)						\
	(__byte(d, 0) = __byte(s, 7),					\
	    __byte(d, 1) = __byte(s, 6),				\
	    __byte(d, 2) = __byte(s, 5),				\
	    __byte(d, 3) = __byte(s, 4),				\
	    __byte(d, 4) = __byte(s, 3),				\
	    __byte(d, 5) = __byte(s, 2),				\
	    __byte(d, 6) = __byte(s, 1),				\
	    __byte(d, 7) = __byte(s, 0))



#if !defined(EXTERNAL_DATA_FUNCTIONS)

#if	defined(_BIG_ENDIAN)

#define	BE16toH(s, d) __copy2bytes(s, d)
#define	BE24toH(s, d) (__byte(d, 0) = 0, __copy3bytes(s, (uint8_t *)(d) + 1))
#define	BE32toH(s, d) __copy4bytes(s, d)
#define	BE64toH(s, d) __copy8bytes(s, d)

#define	HtoBE16(s, d) __copy2bytes(s, d)
#define	HtoBE24(s, d) __copy3bytes((uint8_t *)(s) + 1, d)
#define	HtoBE32(s, d) __copy4bytes(s, d)
#define	HtoBE64(s, d) __copy8bytes(s, d)

#define	LE16toH(s, d) __copy2bytes_rev(s, d)
#define	LE24toH(s, d) (__byte(d, 0) = 0, \
	__copy3bytes_rev(s, (uint8_t *)(d) + 1))
#define	LE32toH(s, d) __copy4bytes_rev(s, d)
#define	LE64toH(s, d) __copy8bytes_rev(s, d)

#define	HtoLE16(s, d) __copy2bytes_rev(s, d)
#define	HtoLE24(s, d) __copy3bytes_rev((uint8_t *)(s) + 1, d)
#define	HtoLE32(s, d) __copy4bytes_rev(s, d)
#define	HtoLE64(s, d) __copy8bytes_rev(s, d)


#elif defined(_LITTLE_ENDIAN)

#define	BE16toH(s, d) __copy2bytes_rev(s, d)
#define	BE24toH(s, d) (__byte(d, 3) = 0, __copy3bytes_rev(s, d))
#define	BE32toH(s, d) __copy4bytes_rev(s, d)
#define	BE64toH(s, d) __copy8bytes_rev(s, d)

#define	HtoBE16(s, d) __copy2bytes_rev(s, d)
#define	HtoBE24(s, d) __copy3bytes_rev(s, d)
#define	HtoBE32(s, d) __copy4bytes_rev(s, d)
#define	HtoBE64(s, d) __copy8bytes_rev(s, d)

#define	LE16toH(s, d) __copy2bytes(s, d)
#define	LE24toH(s, d) (__byte(d, 3) = 0, __copy3bytes(s, d))
#define	LE32toH(s, d) __copy4bytes(s, d)
#define	LE64toH(s, d) __copy8bytes(s, d)

#define	HtoLE16(s, d) __copy2bytes(s, d)
#define	HtoLE24(s, d) __copy3bytes(s, d)
#define	HtoLE32(s, d) __copy4bytes(s, d)
#define	HtoLE64(s, d) __copy8bytes(s, d)


#else	/* defined(_BIG_ENDIAN) */

#error One of _BIG_ENDIAN or _LITTLE_ENDIAN must be defined

#endif  /* defined(_BIG_ENDIAN) */

#else	/* !defined(EXTERNAL_DATA_FUNCTIONS) */

void BE16toH(uint16_t *s, uint16_t *d);
void BE24toH(uint32_t *s, uint32_t *d);
void BE32toH(uint32_t *s, uint32_t *d);
void BE64toH(uint64_t *s, uint64_t *d);
void HtoBE16(uint16_t *s, uint16_t *d);
void HtoBE24(uint32_t *s, uint32_t *d);
void HtoBE32(uint32_t *s, uint32_t *d);
void HtoBE64(uint64_t *s, uint64_t *d);
void LE16toH(uint16_t *s, uint16_t *d);
void LE24toH(uint32_t *s, uint32_t *d);
void LE32toH(uint32_t *s, uint32_t *d);
void LE64toH(uint64_t *s, uint64_t *d);
void HtoLE16(uint16_t *s, uint16_t *d);
void HtoLE24(uint32_t *s, uint32_t *d);
void HtoLE32(uint32_t *s, uint32_t *d);
void HtoLE64(uint64_t *s, uint64_t *d);

#endif	/* !defined(EXTERNAL_DATA_FUNCTIONS); */

#endif	/* _AML_EXTERNAL_DATA_H */
