/*
 *	pax_pair.h - definitions for the pax_pair structure which forms the
 * basis of the pax_hdr library by providing input/output, parsing, etc of
 * individual fields in an extended pax header.
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

#ifndef _PAX_PAIR_H
#define	_PAX_PAIR_H

#include <sys/types.h>

#include <pax_hdr/pax_err.h>

/*
 * IMPORTANT:  The non-negative values MUST align with the values for
 * pxp_formats defined in pax_pair.c
 */
typedef enum pax_val_type {
	PXP_P_STRING_TYPE = -5,
	PXP_P_U64_TYPE = -4,
	PXP_P_I64_TYPE = -3,
	PXP_P_TIME_TYPE = -2,
	PXP_LINE_TYPE = -1,
	PXP_I64_TYPE = 0,
	PXP_U64_TYPE,
	PXP_TIME_TYPE,
	PXP_STRING_TYPE
} pax_val_type_t;

#define	IS_PXP_PARSED_TYPE(_type_) (_type_ < 0)

#define	PXP_NUMBER_BASE 10

typedef struct pax_time {
	/* whole seconds */
	int64_t sec;
	/* nanoseconds */
	uint32_t nsec;
} pax_time_t;

typedef struct pax_pair {
	char *keyword;
	size_t line_len;
	char *header_line;

	/*
	 * points into the header line at the beginning of the string
	 * representation of the data in the pair.
	 */
	char *to_string;

	pax_val_type_t val_type;
	union {
		int64_t i64_val;
		uint64_t u64_val;
		pax_time_t time_val;
		char *string_val;
	} value;

	struct pax_pair *next;
} pax_pair_t;

/*
 * All of these create a new pair from data
 */
pax_pair_t *
pxp_mkpair_i64(
	char *keyword,
	int64_t value);

pax_pair_t *
pxp_mkpair_u64(
	char *keyword,
	uint64_t value);

pax_pair_t *
pxp_mkpair_time(
	char *keyword,
	pax_time_t value);

pax_pair_t *
pxp_mkpair_string(
	char *keyword,
	char *value);

/*
 * Makes a new pair from a formatted line
 */
int
pxp_mkpair_line(
	pax_pair_t **result,
	char **buffer,
	size_t *buffer_len);

/*
 * All of these attempt to parse a value from a pair that was created with
 * pxp_mkpair_line.  They attempt to return a sensible error if the value
 * cannot be parsed as requested.
 */
int
pxp_parse_val_i64(
	pax_pair_t *pair,
	int64_t *value);

int
pxp_parse_val_u64(
	pax_pair_t *pair,
	uint64_t *value);
int
pxp_parse_val_time(
	pax_pair_t *pair,
	pax_time_t *value);

int
pxp_parse_val_string(
	pax_pair_t *pair,
	const char **value);

/*
 * returns the length of the header line for a pair.  If it's a parsed pair
 * returns the length of the existing line.  If it's a non-parsed pair (i.e.
 * created by one of the typed mkpair functions) returns the length of the
 * line as it would be/is formatted.
 * Does not leave a formatted line in the pair if it does not already have one.
 */
int
pxp_get_line_len(
	pax_pair_t *pair,
	size_t *len);

/*
 * Allocates a buffer of the appropriate size and formats the data in the
 * pair into the line as it would be represented in the extended header.
 *
 * Returns a pointer to the member of the structure, which should not be
 * modified.  Will not re-format a line if it does not need to be.
 */
int
pxp_get_line(
	pax_pair_t *pair,
	char **line,
	size_t *len,
	int force);

int pxp_get_i64(
	pax_pair_t *pair,
	int64_t *value);

int pxp_get_u64(
	pax_pair_t *pair,
	uint64_t *value);

int pxp_get_time(
	pax_pair_t *pair,
	pax_time_t *value);

int pxp_get_string(
	pax_pair_t *pair,
	const char **value);

/*
 * Destroys all of the pairs in the list pointed to by pairs by traversing
 * the next pointers in each pair.
 */
int
pxp_destroy_pairs(
	pax_pair_t *pairs);

#endif /* _PAX_PAIR_H */
