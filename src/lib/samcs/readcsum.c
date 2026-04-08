/*
 * readcsum.c - read SUNWsamfs_digest file
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
 * Copyright 2026 Carsten Grzemba
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.4 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <pthread.h>
#include <unistd.h>
#include <limits.h>

/* Solaris headers. */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sha1.h>
#include <sys/sha2.h>
#include <fcntl.h>

/* SAM-FS headers. */
#include "pub/stat.h"
#include "sam/types.h"
#include "sam/checksum.h"
#define DEC_INIT
#include "sam/checksumf.h"
#undef DEC_INIT

#ifdef TEST
#define Trace(l, f, ...) printf(stderr, f, ...)
#else
#define	Trace(l, f, ...)
#endif

int
csumstr2qw(char *csumbuf, int csum_size, csum_t *csum, int algo)
{
	char csalgo[8];
	char *csp = csumbuf;

	strncpy(csalgo, csp, strlen(csum_algo[algo]));
	if (strncmp(csalgo, csum_algo[algo],
	    strlen(csum_algo[algo])) != 0) {
		Trace(TR_ERR, "csum algo missmatch: %s/%s, %s", fn,
		    csum_algo[algo], csalgo);
		return (EINVAL);
	}
	csp = strchr(csumbuf, ':');
	csp++;
        for(int i=0; i<csum_size; i++) {
		sscanf(csp,"%08x", &csum->csum_val[i]);
		csp += 8;
	}
	return (0);
}

int
readCsumFile(const char* fn, csum_t* csum, int algo)
{
	int dfd;
	int cwdfd;
	int attrfd;
	int afd;
	char csumbuf[256] = {'\0'};
	char *fnpos;

	if ((cwdfd = open(".", O_RDONLY)) < 0) {
	        Trace(TR_ERR, "failed get current directory: %s", strerror(errno));
		return (errno);
	}
	dfd = open(fn, O_RDONLY);
	if (dfd < 0) {
	        Trace(TR_ERR, "failed open: %s, %s", fn, strerror(errno));
	        return (errno);
	}
	attrfd = openat(dfd, ".", O_RDONLY|O_XATTR);
	close(dfd);
	if (attrfd < 0) {
	        Trace(TR_ERR, "failed open attr dir: %s, %s",
	            fn, strerror(errno));
	        return (errno);
	}
	if (fchdir(attrfd) == -1) {
	        close(attrfd);
	        Trace(TR_ERR, "failed fchdir to attribute: %s, %s",
	            fn, strerror(errno));
	        return (errno);
	}

	afd = open(FN_CSUM, O_RDONLY);
	if (afd < 0) {
	        Trace(TR_ERR, "failed open for writing: %s/%s, %s",
	            fn, FN_CSUM, strerror(errno));
	        return (errno);
	}
	if(read(afd, csumbuf, 256) <= 0) {
	        Trace(TR_ERR, "failed read: %s/%s, %s",
	            fn, FN_CSUM, strerror(errno));
	        return (errno);
	}
	memset(csum->csum_val, 0, sizeof(csum_t));
	switch (algo) {
		case CS_SIMPLE:
		case CS_MD5:
			(void) csumstr2qw(csumbuf, 4, csum, algo);
			break;
		case CS_SHA1:
			(void) csumstr2qw(csumbuf, SHA1_DIGEST_LENGTH>>2, csum, algo);
	                break;
		case CS_SHA256:
			(void) csumstr2qw(csumbuf, SHA256_DIGEST_LENGTH>>2, csum, algo);
	                break;
		case CS_SHA384:
			(void) csumstr2qw(csumbuf, SHA384_DIGEST_LENGTH>>2, csum, algo);
	                break;
		case CS_SHA512:
			(void) csumstr2qw(csumbuf, SHA512_DIGEST_LENGTH>>2, csum, algo);
	                break;
	        default:
	                Trace(TR_ERR, "unknown csum algo: %d", algo);
	                return (errno);
	                ;;
	}
out:
	close(afd);
	(void) fchdir(cwdfd);
	close(attrfd);
	close(cwdfd);
	return (0);
}
