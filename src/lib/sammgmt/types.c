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
#pragma ident   "$Revision: 1.20 $"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "pub/mgmt/error.h"

#include "mgmt/util.h"
#include "mgmt/config/common.h"
#include "sam/lib.h"
#include "pub/mgmt/types.h"

/*
 * types.c
 * contains type related functions like string conversions useful
 * to users of the api.
 */

static char *exact_match_fsize(fsize_t v, char *buf, size_t buflen);

#define	DEC_LENGTH 4

/*
 * Returns a string in the largest unit.  If returned value is NOT
 * required to reverse-match back to the original file size, set
 * "exact" to FALSE.
 *
 * The returned char pointer points to the start of the string.
 */
char *
match_fsize(
	fsize_t v,		/* Value to convert */
	int	prec,		/* precision - number of digits after "." */
	boolean_t exact,	/* result must reverse-match input */
	char	*buf,		/* result buffer */
	size_t	buflen		/* size of result buffer */
)
{

	float	f;
	fsize_t size;
	float	szArr[] = {EXA, PETA, TERA, GIGA, MEGA, KILO};
	char	szLtr[] = {'E', 'P', 'T', 'G', 'M', 'K'};
	int	i;


	for (i = 0; i < 6; i++) {
		if (v / szArr[i] < 1.0) {
			continue;
		}

		f = v / szArr[i];

		if (exact) {
			str_to_fsize(buf, &size);
			if (size != v) {
				continue;
			}
		}

		snprintf(buf, buflen, "%*.*f%c", 5 + prec,
		    prec, f, szLtr[i]);

		return (buf);
	}

	/* could not find an acceptable match so it has to become bytes. */
	snprintf(buf, buflen, "%llu", v);
	return (buf);
}


/*
 * Returns a string in the largest unit that was capable of creating
 * an exact match translation.
 *
 * The returned char pointer points to the beginning of the string.
 */
static char *
exact_match_fsize(
	fsize_t v,		/* Value to convert */
	char	*buf,		/* result buffer */
	size_t	buflen		/* size of result buffer */
)
{
	int prec = 3;

	return (match_fsize(v, prec, TRUE, buf, buflen));
}

/*
 * convert a string to fsize. Takes as an argument a string representing a
 * size and converts it to the number of bytes stored in an fsize.
 */
int
str_to_fsize(char *size, fsize_t *fsize) {
	int err = 0;


	if (ISNULL(size)) {
		return (-1);
	}
	err = StrToFsize(size, fsize);
	if (err != 0) {
		samerrno = SE_INVALID_FSIZE;
		/* %s cannot be converted to fsize_t */
		snprintf(samerrmsg, MAX_MSG_LEN,
			GetCustMsg(SE_INVALID_FSIZE), size);
	}

	return (err);
}


/*
 * Takes as input an fsize and generates a string representation of it
 * as a floating point number followed by a size unit. The returned string
 * can have upto 3 digits in the decimal portion but will not contain any
 * trailing zeros. If a precise conversion cannot be made within 3 decimal
 * places, the string will be contain the number of bytes.
 *
 * The upper bound for inputs is 8Exabytes. If a number greater than this is
 * entered 8 Exabytes is returned. The caller is responsible for checking.
 * Based on this limit the buffer that is returned could hold as many as 20
 * decimal digits followed by a NULL terminator.
 *
 * This function returns a pointer to a statically allocated buffer
 * containing the string representation of the fsize. Returns '\0' if
 * error encountered.
 */
char *
fsize_to_str(fsize_t f, char *buf, int bufsize) {


	char *beg = NULL;
	char *end = NULL;
	size_t spaces = 0;
	char unit;

	/* Don't trace. Function is too common. */

	beg = exact_match_fsize(f, buf, bufsize);
	spaces = strspn(beg, " \t");
	beg += spaces;

	/* if this is true then something is wrong. */
	if (strchr(beg, '.') == NULL) {
		return (beg);
	}
	end = beg + strlen(beg) - 1;

	if (isalpha(*(end))) {
		unit = *(end);
		end--;
	} else {
		unit = 'b';
	}
	while (end != beg) {
		/*
		 * strip out trailing zeros.
		 * return when you hit the decimal  or the first nonzero bit
		 * make sure there is a decimal too though!
		 */
		if (*end == '0') {
			*(end + 1) = '\0';
			*end-- = unit;
		} else if (*end == '.') {
			*(end + 1) = '\0';
			*end = unit;
			return (beg);
		} else {
			return (beg);
		}

	}

	return (beg);
}


/*
 * convert a string representation of a time interval
 * including units to an interval
 */
int
str_to_interval(char *str, uint_t *interval) {
	int tmp;
	if (ISNULL(str, interval)) {
		return (-1);
	}

	if (StrToInterval(str, &tmp) == -1) {
		samerrno = SE_INVALID_INTERVAL;
		/* %s cannot be converted to seconds */
		snprintf(samerrmsg, MAX_MSG_LEN,
			GetCustMsg(SE_INVALID_INTERVAL), str);
		return (-1);
	}
	*interval = (uint_t)tmp;
	return (0);
}


/*
 * convert an interval to a string representation including units.
 *
 * THIS FUNCTION IS NOT THREAD SAFE.  DO NOT USE IN A MULTITHREADED
 * APPLICATION.
 */
char *
interval_to_str(uint_t interval) {
	return (StrFromInterval(interval, NULL, 0));
}
