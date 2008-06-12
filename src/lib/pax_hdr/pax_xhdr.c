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

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers */
#include <stdio.h>
#include <string.h>

/* SAM-FS headers */
#include "pax_hdr/pax_err.h"
#include "pax_hdr/pax_pair.h"
#include "pax_hdr/pax_util.h"
#include "pax_hdr/pax_xhdr.h"
#include "sam/sam_malloc.h"

/* local inludes */
#include "pax_macros.h"

/* **************** static function prototypes ***************** */

static int
_count_pairs(
	pax_pair_t *hdr_list);

static int
_reverse_list(
	pax_pair_t *hdr_list,
	pax_pair_t ***reversed,
	int *count);

/* ***************** exported function definitions ************* */

int
pxh_put_pair(
	pax_pair_t **hdr_list,
	pax_pair_t *pair,
	int count_match)
{
	int count = 0;

	if (!pair || !hdr_list) {
		return (-PX_ERROR_INVALID);
	}

#ifdef PX_THOROUGH_CHECKS
	/*
	 * Several really bad things happen if you do a duplicate insertion:
	 * 1)  Anything that tries to iterate over the list will inf loop
	 * 2)  Except for pxp_destroy_pairs, which will segfault
	 */
	{
		pax_pair_t *ip = *hdr_list;
		while (ip) {
			if (ip == pair) {
				return (-PX_ERROR_DUPLICATE_INSERTION);
			}
			ip = ip->next;
		}
	}
#endif

	/* insert the new pair at the head of the list */
	pair->next = *hdr_list;
	*hdr_list = pair;

	if (count_match) {
		count = pxh_count_matches(pair->next, pair->keyword);
	}
	return (count);
}

pax_pair_t *
pxh_remove_pair(
	pax_pair_t **hdr_list,
	char *keyword,
	int *all_match)
{
	pax_pair_t *matches = NULL;
	pax_pair_t **matches_tail = NULL;
	pax_pair_t *pair = NULL;
	pax_pair_t *prev = NULL;
	int count = 0;

	if (!hdr_list || !keyword) {
		return (NULL);
	}

	pair = *hdr_list;

	while (pair) {
		if (0 == strcmp(keyword, pair->keyword)) {
			/*
			 * assemble the list of matching pairs being removed
			 * from the list.
			 */
			if (!matches) {
				/* first match */
				matches = pair;
			} else {
				/* subsequent matches */
				*matches_tail = pair;
			}
			matches_tail = &pair->next;

			/*
			 * Change the list itself.
			 */
			if (!prev) {
				/*
				 * the pair at the head of the list matched
				 * the keyword.
				 */
				*hdr_list = pair->next;
			} else {
				prev->next = pair->next;
			}

			++count;
			if (!all_match) {
				break;
			}
		} else {
			prev = pair;
		}
		pair = pair->next;
	}

	if (matches_tail && *matches_tail) {
		*matches_tail = NULL;
	}

	if (all_match) {
		*all_match = count;
	}

	return (matches);
}

pax_pair_t **
pxh_get_pair(
	pax_pair_t *hdr_list,
	char *keyword,
	int *all_match)
{
	pax_pair_t **result = NULL;
	int iresult = 0;
	int count = 0;

	if (!keyword) {
		return (result);
	}

	while (hdr_list) {
		if (0 == strcmp(keyword, hdr_list->keyword)) {
			/* first match */
			if (!result) {
				if (all_match) {
					count = pxh_count_matches(hdr_list,
					    keyword);
				} else {
					count = 1;
				}
				SamMalloc(result,
				    count * sizeof (pax_pair_t *));
			}

			result[iresult++] = hdr_list;
			if (!all_match) {
				break;
			}
		}
		hdr_list = hdr_list->next;
	}

	if (all_match) {
		*all_match = count;
	}

	return (result);
}

int
pxh_get_xheader_size(
	pax_pair_t *hdr_list,
	size_t *exact_size,
	size_t *pad_size)
{
	int status = PX_SUCCESS;
	size_t pair_size;

	if (!exact_size || !pad_size) {
		ABORT_ST(PX_ERROR_INVALID);
	}

	*exact_size = 0;
	*pad_size = 0;

	while (hdr_list) {
		status = pxp_get_line_len(hdr_list, &pair_size);
		TEST_AND_ABORT();

		*exact_size += pair_size;
		hdr_list = hdr_list->next;
	}

	/*
	 * padded size is the size rounded up to the nearest whole block.
	 * the number of whole blocks the header requires is size / 512.
	 * if we need a partial block, size / 512 != 0, and we add a whole
	 * block to hold that data and the zero padding.
	 */
	*pad_size = round_to_block (*exact_size, PAX_XHDR_BLK_SIZE);
out:
	return (status);
}

int
pxh_write_xheader(
	pax_pair_t *hdr_list,
	char *buffer,
	size_t buffer_len)
{
	int status = PX_SUCCESS;
	pax_pair_t **rev_pairs = NULL;
	int num_pairs = 0;
	int ipair;
	char *line;
	size_t len;

	_reverse_list(hdr_list, &rev_pairs, &num_pairs);

	for (ipair = 0; ipair < num_pairs; ++ipair) {
		status = pxp_get_line(rev_pairs[ipair], &line, &len, 0);

		if (len > buffer_len) {
			ABORT_ST(PX_ERROR_OVERFLOW);
		}

		memcpy(buffer, line, len);
		buffer += len;
		buffer_len -= len;
	}

	/* zero the rest of the buffer */
	memset(buffer, 0, buffer_len);
out:
	SamFree(rev_pairs);
	return (status);
}

int
pxh_mkpairs(
	pax_pair_t **result_pairs,
	char *buffer,
	size_t buffer_len)
{
	int status = PX_SUCCESS;
	char *end = buffer + buffer_len;
	pax_pair_t *pair;

	if (!result_pairs || !buffer) {
		ABORT_ST(PX_ERROR_INVALID);
	}

	while (buffer < end) {
		status = pxp_mkpair_line(&pair, &buffer, &buffer_len);
		if (status == PX_SUCCESS_NO_MORE_PAIRS) {
			status = PX_SUCCESS;
			break;
		}
		TEST_AND_ABORT();

		pxh_put_pair(result_pairs, pair, 0);

	}
out:
	return (status);
}

int
pxh_count_matches(
	pax_pair_t *hdr_list,
	char *keyword)
{
	int count = 0;

	while (hdr_list) {
		if (keyword) {
			if (0 == strcmp(keyword, hdr_list->keyword)) {
				++count;
			}
		} else {
			++count;
		}
		hdr_list = hdr_list->next;
	}
	return (count);
}

/* **************** static function definitions ***************** */

static int
_count_pairs(
	pax_pair_t *hdr_list)
{
	int count = 0;

	while (hdr_list) {
		++count;
		hdr_list = hdr_list->next;
	}

	return (count);
}

static int
_reverse_list(
	pax_pair_t *hdr_list,
	pax_pair_t ***reversed,
	int *count)
{
	int status = PX_SUCCESS;
	pax_pair_t **rev_pairs = NULL;
	int num_pairs = _count_pairs(hdr_list);
	int ipair;

	SamMalloc(rev_pairs, num_pairs * sizeof (pax_pair_t *));

	ipair = num_pairs - 1;
	while (hdr_list) {
		if (ipair < 0) {
			ABORT_ST(PX_ERROR_INTERNAL);
		}

		rev_pairs[ipair--] = hdr_list;
		hdr_list = hdr_list->next;
	}
	if (ipair != -1) {
		ABORT_ST(PX_ERROR_INTERNAL);
	}
out:
	if (status != PX_SUCCESS) {
		SamFree(rev_pairs);
		rev_pairs = NULL;
	}

	*reversed = rev_pairs;
	*count = num_pairs;
	return (status);
}
