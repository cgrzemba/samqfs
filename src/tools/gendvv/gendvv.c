/*
 *	gendvv.c - generate data verification values for files.
 *
 *	gendvv generates the SUN data verification value for the given files.
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

#ifdef __STDC__
#pragma ident "$Revision: 1.16 $"
#endif /* __STDC__ */


/*
 * Feature test switches:
 *	IBMPC  - Define PC compilation otherwise solaris.
 *	PRDATA - Print input data and packed data.
 */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if !defined(IBMPC)
/* Solaris headers. */
#include <libgen.h>
#endif /* IBMPC */

/* SAM-FS headers. */
	/* None. */

/* Local headers. */
	/* None. */

/* Macros. */
#define	BUFSIZE 16384

/* Types. */
#if !defined(IBMPC)
typedef unsigned int word32;
#else
typedef long word32;
#endif /* IBMPC */

typedef struct {
	word32 v[4];
} dvv_t;

/* Structures. */
	/* None. */

/* Private data. */
static uchar_t *buf;
static char *program_name = NULL;
static int nodvv = 0;
static int prdvv = 0;

/* Private functions. */
#ifdef __STDC__
static void dofile(char *fname);
static void dvm(uchar_t *bp, int len, dvv_t *dvv);
static void hash(word32 *hashbuf, dvv_t *dvv);
#else
static void dofile();
static void dvm();
static void hash();
static char *strerror();
#endif

/* Public data. */
	/* None. */

/* Function macros. */
	/* None. */

/* Signal catching functions. */
	/* None. */


int
#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
	int argc;
	char *argv[];
#endif
{
	char optchar;
	int n;

#if !defined(IBMPC)
	program_name = basename(argv[0]);
	optchar = '-';
#else
	program_name = "gendvv";
	optchar = '/';
#endif /* IBMPC */

	n = 1;
	while (n < argc && *argv[n] == optchar) {
		char ch = *(argv[n] + 1);

		if (ch == 'n')  nodvv = 1;
		else if (ch == 'p')  prdvv = 1;
		else {
			n = argc;
			break;
		}
		n++;
	}

	if (n == argc) {
		fprintf(stderr, "usage: %s [%cn] [%cp] file ...\n",
			program_name,
				optchar, optchar);
		fprintf(stderr,
			"  n = do not generate dvv (use for timings)\n");
		fprintf(stderr, "  p = print each intermediate dvv\n");
		exit(2);
	}

	/* malloc the buffer space */
	if ((buf = (uchar_t *)malloc(BUFSIZE)) == NULL) {
		fprintf(stderr, "malloc failed, %d\n", BUFSIZE);
		exit(1);
	}
	while (n < argc) {
		dofile(argv[n++]);
	}
	return (0);
}


/*
 *	Process a file.
 *	Read the file and compute its data verification value.
 *	Print the file name and value to standard out.
 */
void
#ifdef __STDC__
dofile(char *fname)
#else
dofile(fname)
	char *fname;
#endif
{
	struct stat st;
	word32 len[4];
	dvv_t dvv;
	int fd, n;

#if !defined(IBMPC)
	if ((lstat(fname, &st)) < 0) {
#else
	if ((stat(fname, &st)) < 0) {
#endif /* IBMPC */
		fprintf(stderr, "stat(%s): %s\n", fname, strerror(errno));
		return;
	}
#if !defined(IBMPC)
	if (!S_ISREG(st.st_mode)) {
		return;
	}
#endif /* IBMPC */
	if ((fd = open(fname, O_RDONLY)) < 0) {
		fprintf(stderr, "open(%s): %s\n", fname, strerror(errno));
		return;
	}
	memset(&dvv, 0, sizeof (dvv));	/* Clear dvv. */

#ifdef INC_LENGTH
	/*
	 * Compute the initial data verification value using the length of the
	 * file as words 1 and 3 of data.
	 */
	len[0] = len[2] = 0;
#if !defined(IBMPC)
	len[1] = len[3] = st.st_size;
#else
	len[1] = len[3] = st.size;
#endif /* IBMPC */
	hash(len, &dvv);
#endif /* INC_LENGTH */

	while ((n = read(fd, buf, BUFSIZE)) != 0) {
		if (n < 0) {
			fprintf(stderr, "read(%s): %s\n", fname,
				strerror(errno));
			close(fd);
			return;
		}
		if (!nodvv) {
			dvm(buf, n, &dvv);
		}
	}
	for (n = 0; n < 4; n++) {
		printf("%08X", dvv.v[n]);
	}
	printf(" %s\n", fname);
	close(fd);
}


/*
 *	Accumulate data verification value.
 */
static void
#ifdef __STDC__
dvm(
	uchar_t *bp,		/* Data */
	int len,		/* Number of bytes of data */
	dvv_t *dvv)		/* Data verification value */
#else
dvm(bp, len, dvv)
	uchar_t *bp;
	int len;
	dvv_t *dvv;
#endif
{
	int l;
	int l1 = len - 16;

	for (l = 0; l < len; l += 16) {
		word32 dw[4];
		uchar_t cf[16];
		int n;

		if (l > l1) {	/* Process remainder of buffer */
			l1 = len - l;
			n = 0;
			while (n < l1)  cf[n++] = *bp++;
			while (n < 16)  cf[n++] = 0;
			bp = cf;
		}
#ifdef PRDATA
		/* Print the input data. */
		{ uchar_t *b = bp;

		printf("%d", l);
		for (n = 0; n < 4; n++) {
			printf(" %02X%02X%02X%02X", *b, *(b+1), *(b+2),
				*(b+3));
			b += 4;
		}
		}
#endif
#if !defined(IBMPC)
		memmove(&dw, bp, 16);
		bp += 16;
#else
		/* Pack PC (Intel) 32-bit integer. */
		for (n = 0; n < 4; n++) {
			dw[n] = (word32)(*bp++)	|
					(word32)(*bp++) <<  8 |
					(word32)(*bp++) << 24 |
					(word32)(*bp++) << 16;
		}
#endif /* IBMPC */
		hash(dw, dvv);
	}
}


/*
 *	Compute data verification value from 4 32-bit words.
 *	Note:  The 4 32-bit words are altered.
 */
static void
#ifdef __STDC__
hash(
	word32 *dw,		/* 4 32-bit words */
	dvv_t *dvv)		/* Data verification value */
#else
hash(dw, dvv)
	word32 *dw;
	dvv_t *dvv;
#endif
{
	static word32 a[6] = {
		32633,
		65419,
		65469,
		32717,
		65497,
		131009
	};
	word32 u[10];
	int n;

#ifdef PRDATA
		/* Print the 4 32-bit words. */
		printf(" :");
		for (n = 0; n < 4; n++) {
			printf(" %08lX", dw[n]);
		}
		printf("\n");
#endif
	for (n = 0; n < 8; n++) {
		u[0] = a[0] * dw[0];
		u[1] = a[1] + dw[1];
		u[2] = a[2] + dw[2];
		u[3] = a[3] + dw[3];
		u[4] = u[0] ^ u[2];
		u[5] = u[1] ^ u[3];
		u[6] = a[4] * u[4];
		u[7] = u[5] + u[6];
		u[8] = a[5] * u[7];
		u[9] = u[6] + u[8];
		dw[0] = u[0] ^ u[8];
		dw[1] = u[1] ^ u[9];
		dw[2] = u[2] ^ u[8];
		dw[3] = u[3] ^ u[9];
	}
	dvv->v[0] += dw[0];
	dvv->v[1] += dw[1];
	dvv->v[2] += dw[2];
	dvv->v[3] += dw[3];

	if (prdvv) {
		static int count = 1;

		for (n = 0; n < 4; n++) {
			printf("%08X", dvv->v[n]);
		}
		printf(" %d\n", count++);
	}
}
