/*
 * writecsum.c - write SUNWsamfs_digest attr file
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
#include <fcntl.h>

/* SAM-FS headers. */
#include "pub/stat.h"
#include "sam/types.h"
#include "sam/checksum.h"
#include "sam/checksumf.h"

#define Trace(l, f, ...)

int
writeCsumFile(const char* mntpt, const char* fn, csum_t* csum, int algo)
{
	int dfd;
	int attrfd;
	int cwdfd;
	int afd;
	char abspath[PATH_MAX];
	char compname[PATH_MAX];
	char csumbuf[256] = {'\0'};
	char *csp = csumbuf;
	char *fnpos;

	if (mntpt != NULL) {
		fnpos = strrchr(fn, '/');
		if (fnpos == NULL)
			sprintf(abspath, "%s", mntpt);
		else {
			sprintf(compname, "%s", fnpos+1);
			sprintf(abspath, "%s/%s", mntpt, fn);
		}
	} else {
		sprintf(abspath, "%s", fn);
	}
	if ((cwdfd = open(".", O_RDONLY)) < 0) {
		return (errno);
	}
	dfd = open(abspath, O_RDONLY);
	if (dfd < 0) {
	        Trace(TR_ERR, "failed open: %s, %s", abspath, strerror(errno));
	        return (errno);
	}
	attrfd = openat(dfd, ".", O_RDONLY|O_XATTR);
	close(dfd);
	if (attrfd < 0) {
	        Trace(TR_ERR, "failed open attr dir: %s, %s",
		    abspath, strerror(errno));
	        return (errno);
	}
	if (fchdir(attrfd) == -1) {
		close(attrfd);
	        Trace(TR_ERR, "failed fchdir to attribute: %s, %s",
		    abspath, strerror(errno));
	        return (errno);
	}

	afd = open(FN_CSUM, O_WRONLY | O_CREAT, 0444);
	if (afd < 0) {
	        Trace(TR_ERR, "failed open for writing: %s/%s, %s",
		    abspath, FN_CSUM, strerror(errno));
	        return (errno);
	}
	switch (algo) {
	        case CS_SIMPLE:
	        case CS_MD5:
	                for(int i=0; i<4; i++) {
	                        sprintf(csp,"%08x", csum->csum_val[i]);
	                        csp += 8;
	                }
	                break;
	        case CS_SHA1:
	                for(int i=0; i<(SHA1_DIGEST_LENGTH>>2); i++) {
	                        sprintf(csp,"%08x", csum->csum_val[i]);
	                        csp += 8;
	                }
	                break;
	        case CS_SHA256:
	                for(int i=0; i<(SHA256_DIGEST_LENGTH>>2); i++) {
	                        sprintf(csp,"%08x", csum->csum_val[i]);
	                        csp += 8;
	                }
	                break;
	        default:
			Trace(TR_ERR, "unknown csum algo: %d", checksum->algo);
			return (errno);
	                ;;
	}
	if(dprintf(afd, "%s:%s", csum_algo[algo], csumbuf) < 0) {
	        Trace(TR_ERR, "failed write: %s/%s, %s",
		    abspath, FN_CSUM, strerror(errno));
	        return (errno);
	}

	close(afd);
	fchdir(cwdfd);
	close(attrfd);
	close(cwdfd);
	return (0);
}
