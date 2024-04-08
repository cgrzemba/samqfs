/*
 * intervalstr.c - Interval string conversion functions.
 *
 * An interval is an integer number of seconds.  It may be specified by
 * an integer value followed by the suffixes 's', 'm', 'h', 'd', 'w' and
 * 'y', for seconds, minutes, hours, days, weeks and years.
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

#pragma ident "$Revision: 1.13 $"

/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/lib.h"

#define	MINUTE (60)
#define	HOUR (60 * 60)
#define	DAY (24 * 60 *60)
#define	WEEK (7 * 24 * 60 *60)
#define	YEAR (365 * 24 * 60 *60)


/*
 * Return pointer to interval conversion.
 * Caller provides a buffer for the conversion.  If the caller specifies
 * a NULL buffer, the conversion is returned in a static buffer.
 * Returns start of the string.
 */
char *
StrFromInterval(
	int interval,	/* Value to convert. */
	char *buf,	/* Buffer for the conversion */
	int buf_size)	/* Size of the buffer */
{
	static char our_buf[STR_FROM_INTERVAL_BUF_SIZE];
	char c;
	int v;

	if (buf == NULL) {
		buf = our_buf;
		buf_size = STR_FROM_INTERVAL_BUF_SIZE;
	}
	if (buf_size < STR_FROM_INTERVAL_BUF_SIZE) {
		*buf = '\0';
		return (buf);
	}
	c = '\0';
	v = interval;
	if (v > YEAR && (v % YEAR) == 0) {
		c = 'y';
		v /= YEAR;
	} else if (v > WEEK && (v % WEEK) == 0) {
		c = 'w';
		v /= WEEK;
	} else if (v > DAY && (v % DAY) == 0) {
		c = 'd';
		v /= DAY;
	} else if (v > HOUR && (v % HOUR) == 0) {
		c = 'h';
		v /= HOUR;
	} else if (v > MINUTE && (v % MINUTE) == 0) {
		c = 'm';
		v /= MINUTE;
	}
	snprintf(buf, buf_size, "%d%c", v, c);
	return (buf);
}


/*
 * Parse string to get interval.
 */
int
StrToInterval(
	char *string,
	int *interval)
{
	char	*p;
	int64_t	value;

	errno = 0;
	value = strtoll(string, &p, 0);
	if (errno != 0 || value < 0 || p == string) {
		goto err;
	}
	if (*p == 's') {	/* seconds */
		p++;
	} else if ('m' == *p) {	/* minutes */
		p++;
		value *= MINUTE;
	} else if (*p == 'h') {	/* hours */
		p++;
		value *= HOUR;
	} else if (*p == 'd') {	/* days */
		p++;
		value *= DAY;
	} else if (*p == 'w') {	/* weeks */
		p++;
		value *= WEEK;
	} else if (*p == 'y') {	/* years */
		p++;
		value *= YEAR;
	}
	if (*p != '\0') {
		goto err;
	}
	if (value >= INT_MAX) {
		errno = ERANGE;
		goto err;
	}
	*interval = (int)value;
	return (0);

err:
	if (errno == 0) {
		errno = EINVAL;
	}
	return (-1);
}


#if defined(TEST)

int
main(
	int argc,
	char *argv[])
{
	char buf[STR_FROM_INTERVAL_BUF_SIZE];
	char *tests[] = { "32s", "10h", "5d", "7w", "3y", "18000",
			"13x", "-2", ""};
	int i;
	int interval;

	for (i = 0; *tests[i] != '\0'; i++) {
		char	*p;
		int		interval;

		p = tests[i];
		if (StrToInterval(p, &interval) == -1) {
			fprintf(stderr, "StrToInterval(%s) failed\n", p);
		} else {
			fprintf(stdout, "%s  %d  %s\n", p, interval,
			    StrFromInterval(interval, buf, sizeof (buf)));
		}
	}
	return (EXIT_SUCCESS);
}

#endif /* defined(TEST) */
