/*
 * fsizestr.c - File size string conversion functions.
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

#pragma ident "$Revision: 1.18 $"

/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/lib.h"


#define	KILO (float)(1024LL)
#define	MEGA (float)(1024LL * 1024)
#define	GIGA (float)(1024LL * 1024 * 1024)
#define	TERA (float)(1024LL * 1024 * 1024 * 1024)
#define	PETA (float)(1024LL * 1024 * 1024 * 1024 * 1024)
#define	EXA  (float)(1024LL * 1024 * 1024 * 1024 * 1024 * 1024)


/*
 * Return pointer to 3.xm size conversion.
 * Caller provides a buffer for the conversion.  If the caller specifies
 * a NULL buffer, the conversion is returned in a static buffer.
 * Returns start of the string.
 */
char *
StrFromFsize(
	fsize_t v,	/* Value to convert. */
	int prec,	/* Precision 1 - 3 */
	char *buf,	/* Buffer for the conversion */
	int buf_size)	/* Size of the buffer */
{
	static char our_buf[STR_FROM_FSIZE_BUF_SIZE];
	char c;
	float round;
	float f;

	if (buf == NULL) {
		buf = our_buf;
		buf_size = STR_FROM_FSIZE_BUF_SIZE;
	}
	if (buf_size < STR_FROM_FSIZE_BUF_SIZE) {
		*buf = '\0';
		return (buf);
	}
	c = '\0';
	switch (prec) {
	case 0:	round = 0.512;		break;
	case 1:	round = 0.0512;		break;
	case 2:	round = 0.00512;	break;
	case 3:	round = 0.000512;	break;
	default:
		*buf = '\0';
		return (buf);
	}
	f = (float)v;
	if (f >= EXA - (PETA * round)) {
		c = 'E';
		f /= EXA;
	} else if (f >= PETA - (TERA * round)) {
		c = 'P';
		f /= PETA;
	} else if (f >= TERA - (GIGA * round)) {
		c = 'T';
		f /= TERA;
	} else if (f >= GIGA - (MEGA * round)) {
		c = 'G';
		f /= GIGA;
	} else if (f >= MEGA - (KILO * round)) {
		c = 'M';
		f /= MEGA;
	} else if (f >= KILO - round) {
		c = 'k';
		f /= KILO;
	} else {
		snprintf(buf, buf_size, "%4.0f%*s", f, 2 + prec, "");
		return (buf);
	}
	snprintf(buf, buf_size, "%*.*f%c", 5 + prec, prec, f, c);
	return (buf);
}


/*
 * Parse string to get file size.
 */
int
StrToFsize(
	char *string,
	uint64_t *size)
{
	char	*p;
	int64_t value;
	double	conv;
	double	frac;

	*size = 0;
	if (*string == '0' && (*(string+1) == 'x' || *(string+1) == 'X' ||
	    isdigit(*(string+1)))) {
		/*
		 * Do hex/octal.
		 */
		errno = 0;
		value = strtoll(string, &p, 0);
		if (*p == '\0' && errno == 0 && value >= 0) {
			*size = value;
			return (0);
		}
		goto err;
	}
	errno = 0;
	conv = strtod(string, &p);
	if (errno != 0 || p == string || conv < 0) {
		goto err;
	}
	value = (int64_t)conv;
	frac = conv - (double)value;
	if (*p == 'b') {
		p++;
		if (frac > .09) {
			goto err;
		}
	} else if (*p == 'k') {
		p++;
		value *= KILO;
		frac  *= KILO;
	} else if (*p == 'M') {
		p++;
		value *= MEGA;
		frac  *= MEGA;
	} else if (*p == 'G') {
		p++;
		value *= GIGA;
		frac  *= GIGA;
	} else if (*p == 'T') {
		p++;
		value *= TERA;
		frac  *= TERA;
	} else if (*p == 'P') {
		p++;
		value *= PETA;
		frac  *= PETA;
	} else if (*p == 'E') {
		p++;
		value *= EXA;
		frac  *= EXA;
	} else if (frac > .09) {
		goto err;
	}
	if (*p != '\0') {
		goto err;
	}
	*size = (uint64_t)(value + frac);
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
	char *errors[] =
		{ "-1", "-0x1", "-01", "-0.1k", "0x0.1k",
			"1.1", "100.1", "0666.1",
			"0666k", "0x444.1",	"0x444k", "00k", "00x0",
			"08", "008", 0 };
	char *hex_octal[] =
		{ "0x1", "01", "0xffffffff", "0777777", 0 };

	char	buf[STR_FROM_FSIZE_BUF_SIZE];
	char	*p;
	uint64_t value;
	uint64_t size;
	int n;
	int ret;
	int s;

	p = StrFromFsize(1234, 1, NULL, 0);
	if (p == NULL) {
		p = "NULL";
	}
	printf(" buf =  NULL '%s'\n", p);
	p = StrFromFsize(0, 1, buf, 0);
	if (p == NULL) {
		p = "NULL";
	}
	printf(" bufsize = 0 '%s'\n", p);
	printf(" prec    = 0 '%s'\n", StrFromFsize(0, 0, buf, sizeof (buf)));
	printf(" %3d %d  %s\n", 0, 0, StrFromFsize(0, 1,  buf, sizeof (buf)));
	printf(" %3d %d  %s\n", 0, 0, StrFromFsize(0xffffffffffffffff, 1,
	    buf, sizeof (buf)));
	size = 0;
	printf(" prec = 1 %9lld %s|\n", size,
	    StrFromFsize(size, 1, buf, sizeof (buf)));
	printf(" prec = 2 %9lld %s|\n", size,
	    StrFromFsize(size, 2, buf, sizeof (buf)));
	printf(" prec = 3 %9lld %s|\n", size,
	    StrFromFsize(size, 3, buf, sizeof (buf)));
	size = 636528 * 1024;
	size = 1726;
	printf(" prec = 1 %9lld %s|\n", size,
	    StrFromFsize(size, 1, buf, sizeof (buf)));
	printf(" prec = 2 %9lld %s|\n", size,
	    StrFromFsize(size, 2, buf, sizeof (buf)));
	printf(" prec = 3 %9lld %s|\n", size,
	    StrFromFsize(size, 3, buf, sizeof (buf)));
	size = 636528 * 1024;
	printf(" prec = 1 %9lld %s|\n", size,
	    StrFromFsize(size, 1, buf, sizeof (buf)));

	value = 1;
	for (s = 0; s < 6; s++) {
		for (n = 1; n < 1000; n *= 10) {
			printf(" %3d %d  %s", n, s,
			    StrFromFsize(value * n, 1, buf, sizeof (buf)));
			p = strtok(buf, " ");
			ret = StrToFsize(p, &size);
			printf(" %22llu  %22llu %d\n", value * n, size, ret);
			if (ret == -1) {
				fprintf(stderr, "StrToFsize(%s) failed\n", p);
			}
		}
		value *= 1024;
	}
	for (n = 1; n < 16; n++) {
		printf(" %3d %d  %s", n, s,
		    StrFromFsize(value * n, 1, buf, sizeof (buf)));
			p = strtok(buf, " ");
			ret = StrToFsize(p, &size);
			printf(" %22llu  %22llu %d\n", value * n, size, ret);
			if (ret == -1) {
				fprintf(stderr, "StrToFsize(%s) failed\n", p);
			}
	}
	value = 512;
	for (s = 1; s < 6; s++) {
		for (n = 10; n < 1000; n *= 10) {
			printf(" %3d %d  %s", n, s,
			    StrFromFsize(value * n, 1, buf, sizeof (buf)));
			p = strtok(buf, " ");
			ret = StrToFsize(p, &size);
			printf(" %22llu  %22llu %d", value * n, size, ret);
			printf(" %s\n",
			    StrFromFsize(size, 1, buf, sizeof (buf)));
			if (ret == -1) {
				fprintf(stderr, "StrToFsize(%s) failed\n", p);
			}
		}
		value *= 1024;
	}

	/*
	 * Fractional inputs.
	 */
	value = 512;
	for (s = 0; s < 10; s++) {
		if (s == 0) {
			strcpy(buf, "   0.5k");
		} else {
			(void) StrFromFsize(value, 1, buf, sizeof (buf));
		}
		printf(" %3d %s", s, buf);
		p = strtok(buf, " ");
		ret = StrToFsize(p, &size);
		printf(" %22llu  %22llu %d", value, size, ret);
		printf(" %s\n", StrFromFsize(size, 1, buf, sizeof (buf)));
		if (ret == -1) {
			fprintf(stderr, "StrToFsize(%s) failed\n", p);
		}
		value += 1024;
	}

	printf("Hex and Octal\n");

	for (n = 0; hex_octal[n] != 0; n++) {
		ret = StrToFsize(hex_octal[n], &size);
		printf("%10s %10llu 0x%08llx  0%010llo  %d\n",
		    hex_octal[n], size, size, size, ret);
		if (ret == -1) {
			fprintf(stderr, "StrToFsize(%s) failed\n",
			    hex_octal[n]);
		}
	}

	printf("Following should be errors.\n");
	for (n = 0; errors[n] != 0; n++) {
		ret = StrToFsize(errors[n], &size);
		printf("%10s %10llu 0x%08llx  0%010llo  %d\n",
		    errors[n], size, size, size, ret);
		if (ret == 0) {
			fprintf(stderr, "Error case StrToFsize(%s) failed\n",
			    errors[n]);
		}
	}
	return (EXIT_SUCCESS);
}

#endif /* defined(TEST) */
