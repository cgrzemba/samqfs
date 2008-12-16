/*
 *	pax_hdr.h - definitions for pax_hdr.c
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

#ifndef _PAX_UTIL_H
#define	_PAX_UTIL_H

int strtosize_t(
	size_t *result,
	char *str,
	char **endptr,
	int base);

size_t round_to_block(
	size_t value,
	size_t block_size);

/*
 * Formats an unsigned 32 bit quantity as parts per billion with a leading
 * decimal place and no trailing zeroes.  The string is null terminated
 * Some examples of ppb and the string formatted into buf:
 * ppb = 1 -> buf = ".000000001"
 * ppb = 12345 -> buf = ".000012345"
 * ppb = 93000000 -> buf = ".093"
 *
 * Valid values of ppb are from 0 to
 * 1E9 - 1.  Values of ppb outside of this range will result in
 * -PX_ERROR_IMPROPER_FRACTION being returned.
 * The return on valid input is the length of the buffer needed to format
 * the string.  If this length is greater than buf_len, nothing is formatted
 * into the buffer.  The maximum possible required length is 10: a decimal
 * place, nine digits.  The trailing null is not accounted for in the returned
 * value.  This is consistent with the behavior of snprintf.
 */
int format_ppb(
	char *buf,
	size_t buf_len,
	uint32_t ppb);

/*
 * may not be used for parse_fpart (makes no sense - the fpart should be
 * left aligned to the preceding decimal place, WS indicates a whole new
 * number.)
 */
#define	ALLOW_LEADING_WS 0x0001
#define	ALLOW_TRAILING_CHARS 0x0002

/*
 * may only be used for parse_fpart.  Allows the parse routine to complete
 * successfully if there are more digits in the fractional part than can fit
 * in an int64_t.  (unlikely?)
 */
#define	SKIP_OVERPRECISE_DIGITS 0x0004

int
parse_u64(
	char *parse,
	char **next_parse,
	const char *end,
	uint64_t *value,
	int base,
	uint16_t flags);

int
parse_i64(
	char *parse,
	char **next_parse,
	const char *end,
	int64_t *value,
	int base,
	uint16_t flags);

typedef struct _pax_fpart {
	uint64_t numerator;
	uint64_t denominator;
} _pax_fpart_t;

int
parse_fpart(
	char *parse,
	char **next_parse,
	const char *end,
	_pax_fpart_t *value,
	uint16_t flags);

int
fpart_to_ppb(
	uint32_t *ppb,
	_pax_fpart_t *fpart);

#endif /* _PAX_UTIL_H */
