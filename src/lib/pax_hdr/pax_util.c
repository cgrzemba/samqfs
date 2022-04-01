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

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>

#include "pax_hdr/pax_err.h"
#include "pax_hdr/pax_util.h"
#include "pax_macros.h"

int
strtosize_t(
	size_t *result,
	char *str,
	char **end_ptr,
	int base)
{
	unsigned long long max;
	char *end = NULL;

#if SIZE_MAX == ULONG_MAX
	*result = strtoul(str, &end, base);
	max = ULONG_MAX;
#elif SIZE_MAX == ULONGLONG_MAX
	*result = strtoull(str, &end, base);
	max = ULONGLONG_MAX;
#else
#error could not figure out the sizeof (size_t)
#endif
	if (*result == max) {
		if (errno == ERANGE) {
			return (PX_ERROR_RANGE);
		}
	}

	if (*result == 0) {
		/*
		 * strtoul[l] is not required to set errno if no conversion
		 * is performed, we can also check by seeing if it ended
		 * its conversion where it started it.
		 */
		if (str == end || errno == EINVAL) {
			return (PX_ERROR_INVALID);
		}
	}

	if (end_ptr) {
		*end_ptr = end;
	}

	return (PX_SUCCESS);
}

size_t
round_to_block(
	size_t value,
	size_t block_size)
{
	return ((value / block_size) + (value % block_size ? 1 : 0))
	    * block_size;
}

int
format_ppb(
	char *buf,
	size_t buf_len,
	uint32_t ppb)
{
	/* can format the decimal point and 9 digits */
	size_t req_len = 10;
	int zeros;
	uint32_t divisor = 1;
	/* the non-zero digits in ppb */
	uint32_t digits;
	/* number of digits formatted by snprintf */
	int fmt_digits;
	int i;

	if (ppb >= 1000000000) {
		return (-PX_ERROR_IMPROPER_FRACTION);
	}

	if (ppb == 0) {
		return (0);
	}

	/* divide by ever larger powers of 10 until get a remainder */
	while (!(ppb % (divisor *= 10))) { --req_len; }

	/*
	 * one for the decimal point, one for the digit that tripped
	 * out of the above loop
	 */
	zeros = req_len - 2;

	/*
	 * divisor is a power of 10 larger than the lowest order non-zero
	 * digit, knock it back down to get the correct answer, because we
	 * multiply by 10 again before we check.
	 */
	divisor /= 10;
	digits = ppb / divisor;

	/* divide by powers of 10 until we get 0) */
	while (ppb / (divisor *= 10)) { --zeros; }

	if (buf) {
		if (req_len > buf_len) {
			return (req_len);
		}

		*(buf++) = '.';

		for (i = 0; i < zeros; ++i) {
			*(buf++) = '0';
		}
		fmt_digits = snprintf(buf, buf_len - zeros, "%d", digits);

		/* should never happen! */
		if (fmt_digits + zeros > buf_len) {
			return (-PX_ERROR_INTERNAL);
		}
	}
	return (req_len);
}

int
parse_u64(
	char *parse,
	char **next_parse,
	const char *end,
	uint64_t *value,
	int base,
	uint16_t flags)
{
	int status = PX_SUCCESS;
	uint64_t conv;
	int64_t tconv;
	char *end_conv;

	if (!parse || !value) {
		return (PX_ERROR_INVALID);
	}

	--parse;

	while (isspace(*(++parse))) {
		if (! (flags & ALLOW_LEADING_WS)) {
			ABORT_ST(PX_ERROR_LEADING_CHARS);
		}
	}

	/*
	 * Check for negative numbers - strtoull will return the unsigned
	 * representation of - (abs (string)) if string represents a
	 * negative number.  We can ignore errno afterwards:
	 * 1)  ERANGE indicates either large negative number (returns
	 * LLONG_MIN which is < 0) or large postive number (returns
	 * LLONG_MAX, which could indicate LLONG_MAX < number <= ULLONG_MAX)
	 * 2)  EINVAL will be caught below.
	 */
	errno = 0;
	tconv = strtoll(parse, NULL, base);

	if (tconv < 0) {
		ABORT_ST(PX_ERROR_RANGE);
	}

	errno = 0;
	conv = strtoull(parse, &end_conv, base);

	if (conv == 0 && errno == EINVAL) {
		ABORT_ST(PX_ERROR_INVALID);
	}

	if ((conv == 0 || conv == ULLONG_MAX) && errno == ERANGE) {
		ABORT_ST(PX_ERROR_RANGE);
	}

	if (end && end_conv > end) {
		ABORT_ST(PX_ERROR_MALFORMED);
	}

	if (end && !(flags & ALLOW_TRAILING_CHARS)) {
		if (end_conv != end) {
			ABORT_ST(PX_ERROR_TRAILING_CHARS);
		}
	}
out:
	if (PXSUCCESS(status)) {
		*value = conv;
		if (next_parse) {
			*next_parse = end_conv;
		}
	}

	return (status);
}

int
parse_i64(
	char *parse,
	char **next_parse,
	const char *end,
	int64_t *value,
	int base,
	uint16_t flags)
{
	int status = PX_SUCCESS;
	int64_t conv;
	char *end_conv;

	if (!parse || !value) {
		return (PX_ERROR_INVALID);
	}

	--parse;

	while (isspace(*(++parse))) {
		if (! (flags & ALLOW_LEADING_WS)) {
			ABORT_ST(PX_ERROR_LEADING_CHARS);
		}
	}

	errno = 0;
	conv = strtoll(parse, &end_conv, base);

	if (conv == 0 && errno == EINVAL) {
		ABORT_ST(PX_ERROR_INVALID);
	}

	if ((conv == LLONG_MAX || conv == LLONG_MIN) && errno == ERANGE) {
		ABORT_ST(PX_ERROR_RANGE);
	}

	if (end && end_conv > end) {
		ABORT_ST(PX_ERROR_MALFORMED);
	}

	if (end && !(flags & ALLOW_TRAILING_CHARS)) {
		if (end_conv != end) {
			ABORT_ST(PX_ERROR_TRAILING_CHARS);
		}
	}
out:
	if (PXSUCCESS(status)) {
		*value = conv;
		if (next_parse) {
			*next_parse = end_conv;
		}
	}

	return (status);
}

int
parse_fpart(
	char *parse,
	char **next_parse,
	const char *end,
	_pax_fpart_t *value,
	uint16_t flags)
{
	int status = PX_SUCCESS;
	int too_precise = 0;
	int64_t num = 0;
	int64_t den = 1;
	char *end_conv;

	if (!parse || !value || (flags & ALLOW_LEADING_WS)) {
		ABORT_ST(PX_ERROR_INVALID);
	}

	while (parse <= end && isdigit (*parse)) {
		if (too_precise) {
			continue;
		}

		num *= 10;
		den *= 10;

		num += *parse - '0';

		if (den > INT64_MAX / 10) {
			if (flags & SKIP_OVERPRECISE_DIGITS) {
				too_precise = 1;
			} else {
				ABORT_ST(PX_ERROR_FPART_TOO_PRECISE);
			}
		}
		++parse;
	}

	end_conv = parse;

	if (end && end_conv > end) {
		ABORT_ST(PX_ERROR_MALFORMED);
	}

	if (end && !(flags & ALLOW_TRAILING_CHARS)) {
		if (end_conv != end) {
			ABORT_ST(PX_ERROR_TRAILING_CHARS);
		}
	}

	if (next_parse) {
		*next_parse = end_conv;
	}
out:

	if (status == PX_SUCCESS) {
		value->numerator = num;
		value->denominator = den;
	}

	return (status);
}

int
fpart_to_ppb(
	uint32_t *ppb,
	_pax_fpart_t *fpart)
{
	int status = PX_SUCCESS;
	uint64_t result;

	if (!ppb || !fpart) {
		ABORT_ST(PX_ERROR_INVALID);
	}

	if (fpart->denominator == 0) {
		ABORT_ST(PX_ERROR_DIV_ZERO);
	}

	if (fpart->numerator >= fpart->denominator) {
		ABORT_ST(PX_ERROR_IMPROPER_FRACTION);
	}

	if (fpart->denominator < 1000000000) {
		result = (1000000000 / fpart->denominator) * fpart->numerator;
	} else {
		result = fpart->numerator / (fpart->denominator / 1000000000);
	}

	if (result >= 1000000000) {
		ABORT_ST(PX_ERROR_INVALID);
	}

	*ppb = (uint32_t)result;
out:
	return (status);
}
