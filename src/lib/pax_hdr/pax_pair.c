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

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers */
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* SAM-FS headers */
#include "sam/sam_malloc.h"
#include "pax_hdr/pax_pair.h"
#include "pax_hdr/pax_util.h"

/* local headers */
#include "pax_macros.h"

/*
 * IMPORTANT:  These MUST align with the values for pax_val_type_t
 * defined in pax_pair.h
 */
static char *pxp_formats[] = {
	"%.0d %s=%n%lld\n",	/* PXP_I64_TYPE */
	"%.0d %s=%n%llu\n",	/* PXP_U64_TYPE */
	"%.0d %s=%n%lld\n",	/* PXP_TIME_TYPE (ipart only!) */
	"%.0d %s=%n%s\n"	/* PXP_STRING_TYPE */
};

/* **************** static function prototypes ******************* */

static pax_pair_t *
_pxp_mkpair(
	char *keyword,
	pax_val_type_t val_type);

static int
_format_line(
	pax_pair_t *pair,
	int change_pair,
	size_t *req_len);

static int
_adjust_line_length(
	size_t len,
	size_t *adj_len);

static int
_get_line_len(
	size_t *len,
	char *line,
	char **next_parse);

/* ***************** exported function definitions ******************* */

pax_pair_t *
pxp_mkpair_i64(
	char *keyword,
	int64_t value)
{
	pax_pair_t *result = _pxp_mkpair(keyword, PXP_I64_TYPE);
	if (result) {
		result->value.i64_val = value;
	}
	return (result);
}

pax_pair_t *
pxp_mkpair_u64(
	char *keyword,
	uint64_t value)
{
	pax_pair_t *result = _pxp_mkpair(keyword, PXP_U64_TYPE);
	if (result) {
		result->value.u64_val = value;
	}
	return (result);
}

pax_pair_t *
pxp_mkpair_time(
	char *keyword,
	pax_time_t value)
{
	pax_pair_t *result = _pxp_mkpair(keyword, PXP_TIME_TYPE);
	if (result) {
		memcpy(&result->value.time_val, &value, sizeof (pax_time_t));
	}
	return (result);
}

pax_pair_t *
pxp_mkpair_string(
	char *keyword,
	char *value)
{
	pax_pair_t *result = _pxp_mkpair(keyword, PXP_STRING_TYPE);
	if (result && value) {
		SamStrdup(result->value.string_val, value);
	}
	return (result);
}

int
pxp_mkpair_line(
	pax_pair_t **pair_result,
	char **buffer,
	size_t *buffer_len)
{
	int status = PX_SUCCESS;
	pax_pair_t *pair = NULL;
	ptrdiff_t key_len;
	char *parse;
	char *next_parse;

	if (!pair_result || !buffer) {
		status = PX_ERROR_INVALID;
		goto out;
	}

	SamMalloc(pair, sizeof (pax_pair_t));
	memset(pair, 0, sizeof (pax_pair_t));

	status = _get_line_len(&pair->line_len, *buffer, NULL);
	TEST_AND_ABORT();

	if (status == PX_SUCCESS_NO_MORE_PAIRS) {
		ABORT();
	}

	if (pair->line_len > *buffer_len) {
		ABORT_ST(PX_ERROR_END_OF_HEADER);
	}

	/* allocate enough memory for the line and a trailing NULL */
	SamMalloc(pair->header_line, pair->line_len + 1);

	memcpy(pair->header_line, *buffer, pair->line_len);

	/* null terminate the line */
	pair->header_line[pair->line_len] = '\0';

	if (pair->header_line[pair->line_len - 1] != '\n') {
		ABORT_ST(PX_ERROR_MALFORMED);
	}

	/* find the beginning of the keyword */
	if (!(parse = strchr(pair->header_line, ' '))) {
		ABORT_ST(PX_ERROR_MALFORMED);
	}
	++parse;

	/* find the end of the keyword and copy it */
	if (!(next_parse = strchr(parse, '='))) {
		ABORT_ST(PX_ERROR_MALFORMED);
	}

	key_len = next_parse - parse;

	if (key_len == 0) {
		ABORT_ST(PX_ERROR_NO_KEY);
	}

	SamMalloc(pair->keyword, key_len + 1);
	memcpy(pair->keyword, parse, key_len);
	pair->keyword[key_len] = '\0';

	/*
	 * position to_string at the begining of the formatted string
	 * representation
	 */
	pair->to_string = next_parse + 1;
out:
	if (status != PX_SUCCESS) {
		if (pair != NULL) {
			if (pair->keyword != NULL) {
				SamFree(pair->keyword);
			}
			if (pair->header_line != NULL) {
				SamFree(pair->header_line);
			}
			SamFree(pair);
		}
		*pair_result = NULL;
	} else {
		*buffer += pair->line_len;
		*buffer_len -= pair->line_len;
		*pair_result = pair;
		pair->val_type = PXP_LINE_TYPE;
	}

	return (status);
}

int
pxp_parse_val_i64(
	pax_pair_t *pair,
	int64_t *value)
{
	int status = PX_SUCCESS;
	int64_t conv;
	char *end = pair->header_line + pair->line_len - 1;

	if (!IS_PXP_PARSED_TYPE(pair->val_type)) {
		ABORT_ST(PX_ERROR_NOT_PARSED);
	}

	if (pair->val_type == PXP_P_I64_TYPE) {
		*value = pair->value.i64_val;
		return (status);
	}

	status = parse_i64(pair->to_string, NULL, end, &conv,
	    PXP_NUMBER_BASE, 0);
	TEST_AND_ABORT();
out:
	if (PXSUCCESS(status)) {
		if (pair->val_type == PXP_P_STRING_TYPE) {
			SamFree(pair->value.string_val);
			pair->value.string_val = NULL;
		}

		pair->value.i64_val = conv;
		pair->val_type = PXP_P_I64_TYPE;
		*value = conv;
	}

	return (status);
}

int
pxp_parse_val_u64(
	pax_pair_t *pair,
	uint64_t *value)
{
	int status = PX_SUCCESS;
	uint64_t conv;
	char *end = pair->header_line + pair->line_len - 1;

	if (!IS_PXP_PARSED_TYPE(pair->val_type)) {
		ABORT_ST(PX_ERROR_NOT_PARSED);
	}

	if (pair->val_type == PXP_P_U64_TYPE) {
		*value = pair->value.u64_val;
		return (status);
	}

	status = parse_u64(pair->to_string, NULL, end, &conv,
	    PXP_NUMBER_BASE, 0);
	TEST_AND_ABORT();
out:
	if (status == PX_SUCCESS) {
		if (pair->val_type == PXP_P_STRING_TYPE) {
			SamFree(pair->value.string_val);
			pair->value.string_val = NULL;
		}

		pair->value.i64_val = conv;
		pair->val_type = PXP_P_U64_TYPE;
		*value = conv;
	}

	return (status);
}

int
pxp_parse_val_time(
	pax_pair_t *pair,
	pax_time_t *value)
{
	int status = PX_SUCCESS;
	char *parse;
	char *next_parse;
	char *end = pair->header_line + pair->line_len - 1;
	int64_t conv_sec = 0;
	_pax_fpart_t fpart;
	uint32_t conv_nsec = 0;

	if (!IS_PXP_PARSED_TYPE(pair->val_type)) {
		ABORT_ST(PX_ERROR_NOT_PARSED);
	}

	if (pair->val_type == PXP_P_TIME_TYPE) {
		*value = pair->value.time_val;
	}

	status = parse_i64(pair->to_string, &next_parse, NULL, &conv_sec,
	    PXP_NUMBER_BASE, ALLOW_TRAILING_CHARS);

	TEST_AND_ABORT();

	if (next_parse == end) {
		ABORT_ST(PX_SUCCESS);
	}

	if (*next_parse++ != '.') {
		ABORT_ST(PX_ERROR_TRAILING_CHARS);
	}

	parse = next_parse;

	status = parse_fpart(parse, &next_parse, end, &fpart,
	    SKIP_OVERPRECISE_DIGITS);
	TEST_AND_ABORT();

	status = fpart_to_ppb(&conv_nsec, &fpart);
	TEST_AND_ABORT();
out:
	if (status == PX_SUCCESS) {
		if (pair->val_type == PXP_P_STRING_TYPE) {
			SamFree(pair->value.string_val);
			pair->value.string_val = NULL;
		}

		pair->value.time_val.sec = conv_sec;
		pair->value.time_val.nsec = conv_nsec;
		pair->val_type = PXP_P_TIME_TYPE;
		*value = pair->value.time_val;
	}
	return (status);
}

int
pxp_parse_val_string(
	pax_pair_t *pair,
	const char **value)
{
	int status = PX_SUCCESS;
	size_t val_len;

	if (!IS_PXP_PARSED_TYPE(pair->val_type)) {
		ABORT_ST(PX_ERROR_NOT_PARSED);
	}

	if (pair->val_type == PXP_P_STRING_TYPE) {
		*value = pair->value.string_val;
		return (status);
	}

	/*
	 * compute the length of the value string, including the \n
	 * and subtract one to account for the additional trailing NULL
	 */
	val_len = pair->header_line + pair->line_len - pair->to_string;

	/*
	 * malloc a buffer to copy into.  This is exactly big enough to
	 * contain the string including the \n.  We'll overwrite the last \n
	 * with a \0 because it isn't part of the original string.  It came
	 * from formatting the header line at some point.
	 */
	SamMalloc(pair->value.string_val, val_len);

	/* copy the string and null terminate it. */
	memcpy(pair->value.string_val, pair->to_string, val_len);
	pair->value.string_val[val_len - 1] = '\0';
	*value = pair->value.string_val;

	pair->val_type = PXP_P_STRING_TYPE;

out:
	if (!PXSUCCESS(status)) {
		SamFree(pair->value.string_val);
		pair->value.string_val = NULL;
	}

	return (status);
}

int
pxp_get_line_len(
	pax_pair_t *pair,
	size_t *line_len)
{
	int status = PX_SUCCESS;
	size_t fmt_len;
	size_t adj_len;

	if (!pair) {
		ABORT_ST(PX_ERROR_INVALID);
	}

	/*
	 * if it's a parsed pair, or if it's a created pair with an existing
	 * formatted header line.
	 */
	if (pair->val_type <= PXP_LINE_TYPE || pair->header_line) {
		*line_len = pair->line_len;
		ABORT_ST(PX_SUCCESS);
	}

	status = _format_line(pair, 0, &fmt_len);
	TEST_AND_ABORT();

	status = _adjust_line_length(fmt_len, &adj_len);
	TEST_AND_ABORT();

	*line_len = adj_len;
out:
	return (status);
}


int
pxp_get_line(
	pax_pair_t *pair,
	char **line,
	size_t *len,
	int force)
{
	int status = PX_SUCCESS;
	size_t fmt_len = 0;
	size_t adj_len = 0;

	if (!pair) {
		return (0);
	}

	if (IS_PXP_PARSED_TYPE(pair->val_type) && pair->header_line) {
		ABORT_ST(PX_SUCCESS);
	}

	if (pair->header_line && !force) {
		ABORT_ST(PX_SUCCESS);
	}

	if (pair->header_line != NULL) {
		SamFree(pair->header_line);
	}
	pair->header_line = NULL;
	pair->to_string = NULL;
	pair->line_len = 0;

	status = _format_line(pair, 0, &fmt_len);
	TEST_AND_ABORT();

	status = _adjust_line_length(fmt_len, &adj_len);
	TEST_AND_ABORT();

	/* Add 1 to account for the final NULL */
	SamMalloc(pair->header_line, adj_len + 1);
	pair->line_len = adj_len;

	status = _format_line(pair, 1, &fmt_len);

out:
	if (status == PX_SUCCESS) {
		*line = pair->header_line;
		*len = pair->line_len;
	}

	return (status);
}

typedef struct type_err_mapping {
	pax_val_type_t val_type;
	int error;
} type_err_mapping_t;

static const int type_error_mappings [] = {
	PX_ERROR_I64_TYPE,
	PX_ERROR_U64_TYPE,
	PX_ERROR_TIME_TYPE,
	PX_ERROR_STRING_TYPE
};

#define	PXP_GET_FN_IMPL(_ctype_, _ptype_, _PTYPE_) \
int \
pxp_get_ ## _ptype_( \
	pax_pair_t *pair, \
	_ctype_ *value) \
{ \
	if (IS_PXP_PARSED_TYPE(pair->val_type)) { \
		return (pxp_parse_val_ ## _ptype_(pair, value)); \
	} else { \
		if (pair->val_type == PXP_ ## _PTYPE_ ## _TYPE) { \
			*value = pair->value._ptype_ ## _val; \
			return (PX_SUCCESS); \
		} else { \
			return (type_error_mappings[pair->val_type]); \
		} \
	} \
}

PXP_GET_FN_IMPL(int64_t, i64, I64)
PXP_GET_FN_IMPL(uint64_t, u64, U64)
PXP_GET_FN_IMPL(pax_time_t, time, TIME)
PXP_GET_FN_IMPL(const char *, string, STRING)

#undef PXP_GET_FN_IMPL

int
pxp_destroy_pairs(
	pax_pair_t *pairs)
{
	int count = 0;
	pax_pair_t *next;

	while (pairs) {
		SamFree(pairs->keyword);
		SamFree(pairs->header_line);

		if (pairs->val_type == PXP_STRING_TYPE) {
			SamFree(pairs->value.string_val);
		}

		next = pairs->next;
		SamFree(pairs);
		pairs = next;
		++count;
	}
	return (count);
}

/* ******************** static function definitions ****************** */

static pax_pair_t *
_pxp_mkpair(
	char *keyword,
	pax_val_type_t val_type)
{
	pax_pair_t *result = NULL;

	if (!keyword) {
		return (result);
	}

	if (val_type < 0 ||
	    (val_type > sizeof (pxp_formats) / sizeof (pxp_formats[0]))) {
		return (result);
	}

	SamMalloc(result, sizeof (pax_pair_t));
	memset(result, 0, sizeof (pax_pair_t));

	SamStrdup(result->keyword, keyword);

	result->val_type = val_type;

	return (result);
}

static int
_format_line(
	pax_pair_t *pair,
	int change_pair,
	size_t *req_len)
{
	int status = PX_SUCCESS;
	char *format = pxp_formats[pair->val_type];
	size_t offset = 0;
	char **st_val = NULL;
	char *buf = NULL;
	int buf_len = 0;
	int ppb_len = 0;

	if (!req_len) {
		return (PX_ERROR_INVALID);
	}

	if (change_pair) {
		buf = pair->header_line;
		/*
		 * buffer is malloced 1 bigger than the line length, we need
		 * to account for that here.
		 */
		buf_len = pair->line_len + 1;
		st_val = &pair->to_string;
	}

	switch (pair->val_type) {
	case PXP_I64_TYPE:
		*req_len = snprintf(buf, buf_len, format, pair->line_len,
		    pair->keyword, &offset, pair->value.i64_val);
		break;
	case PXP_U64_TYPE:
		*req_len = snprintf(buf, buf_len, format, pair->line_len,
		    pair->keyword, &offset, pair->value.u64_val);
		break;
	case PXP_TIME_TYPE:
		/* format up to the ipart ONLY! */
		*req_len = snprintf(buf, buf_len, format, pair->line_len,
		    pair->keyword, &offset, pair->value.time_val.sec);

		if (pair->value.time_val.nsec) {
			if (buf) {
				ppb_len = format_ppb(buf + *req_len - 1,
				    buf_len - *req_len,
				    pair->value.time_val.nsec);
			} else {
				ppb_len = format_ppb(NULL, buf_len - *req_len,
				    pair->value.time_val.nsec);
			}

			if (ppb_len < 0) {
				ABORT_ST(-ppb_len);
			}
			*req_len += ppb_len;

			/* terminate the line with \n\0 */
			if (buf) {
				if (buf_len >= *req_len) {
					buf[*req_len - 1] = '\n';
					buf[*req_len] = '\0';
				}
			}
		}

		break;
	case PXP_STRING_TYPE:
		*req_len = snprintf(buf, buf_len, format, pair->line_len,
		    pair->keyword, &offset, pair->value.string_val);
		break;
	default:
		/* TODO: can't recover at this point.  What should I do? */
		return (PX_ERROR_BAD_TYPE);
	}

	if (st_val) {
		*st_val = buf + offset;
	}

	if (change_pair && *req_len > buf_len) {
		ABORT_ST(PX_ERROR_OVERFLOW);
	}
out:
	return (status);
}

/*
 * Figure out the line length, accounting for the leading line length field.
 * This is a little tricky.  The line length calculated by _format_line is
 * for a line that looks like this:
 * " key=value\n"
 *
 */
static int
_adjust_line_length(
	size_t len, size_t *adj_len)
{
	size_t adjustment = 0;
	size_t exp;
	size_t ten_exp;
	size_t size_max = (size_t)-1;

	if (!adj_len) {
		return (PX_ERROR_INVALID);
	}

	/*
	 * compute ceiling (log10 (len))  This is the number of digits that
	 * are required to represent len in decimal.
	 */
	for (exp = 1, ten_exp = 10; ten_exp < len; ++exp, ten_exp *= 10) {
		/*
		 * len is > the largest ten_exp that is smaller than SIZE_MAX,
		 * but smaller than SIZE_MAX (presumably...).
		 * Thus ceiling (log10 (len)) is exp + 1
		 */
		if (ten_exp > (size_max / 10)) {
			break;
		}
	}

	adjustment = exp;

	if (len >= ten_exp - exp) {
		adjustment += 1;
	}

	/* if len + adjustment > SIZE_MAX, we can't adjust the length. */
	if (size_max - adjustment < len) {
		return (PX_ERROR_RANGE);
	}

	*adj_len = len + adjustment;
	return (PX_SUCCESS);
}

static int
_get_line_len(
	size_t *len,
	char *line,
	char **next_parse)
{
	if (*line == '\0') {
		return (PX_SUCCESS_NO_MORE_PAIRS);
	}

	return (strtosize_t(len, line, next_parse, 10));
}
