/*
 * checksum.c - Checksum processing for archived files.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
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
#include <fcntl.h>

/* SAM-FS headers. */
#include "pub/stat.h"
#include "sam/types.h"
#include "sam/checksum.h"
#define DEC_INIT
#include "sam/checksumf.h"
#undef DEC_INIT

/* Local headers. */
#include "arcopy.h"

/* Local type definitions. */
typedef struct checksumInfo {		/* checksum context */
	pthread_t	id;		/* checksum's thread id */
	boolean_t	done;		/* file archive complete */

	pthread_mutex_t	mutex;

	boolean_t	isAvail;	/* data available to checksum */
	pthread_cond_t	avail;
	boolean_t	isComplete;	/* checksumming is complete */
	pthread_cond_t	complete;

	uchar_t		algo;		/* checksum algorithm */
	csum_func	func;		/* function */

	char		*data;		/* data in buffer to checksum */
	offset_t	numBytes;	/* number of bytes in buffer */
} checksumInfo_t;

/* Private data. */
static checksumInfo_t *checksum = NULL;

/* Private functions. */
static void* checksumWorker(void *arg);

/*
 * Initialize arcopy's checksum thread.
 */
void
ChecksumInit(
	uchar_t algo)
{
	boolean_t startworker;

	Trace(TR_DEBUG, "[%s] Checksum init cookie: %lld",
	    File->f->FiName, File->f->FiSpace);

	if (checksum == NULL) {
		SamMalloc(checksum, sizeof (checksumInfo_t));
		memset(checksum, 0, sizeof (checksumInfo_t));

		PthreadMutexInit(&checksum->mutex, NULL);
		PthreadCondInit(&checksum->avail, NULL);
		PthreadCondInit(&checksum->complete, NULL);

		startworker = B_TRUE;
	} else {
		startworker = B_FALSE;
	}

	checksum->done = B_FALSE;
	checksum->isComplete = B_TRUE;
	checksum->isAvail = B_FALSE;
	checksum->algo = algo;

	if (algo & CS_USER_BIT) {
		u_longlong_t cookie;

		cookie = 0;
		checksum->func = cs_user;
		checksum->func(&cookie, algo, 0, 0, &File->AfCsum);
	} else {
		if (algo > CS_FUNCS ) {
			Trace(TR_ERR, "[%s] Checksum invalid algo: %d",
			    File->f->FiName, algo);
			return;
		}
		checksum->func = csum_fns[algo];
		checksum->func(&File->f->FiSpace, 0, 0, &File->AfCsum);
	}

	if (startworker == B_TRUE) {
		if (pthread_create(&checksum->id, NULL, checksumWorker,
		    NULL) != 0) {
			LibFatal(pthread_create, NULL);
		}
	}
}

/*
 * Checksum data in buffer.
 */
void
ChecksumData(
	char *data,
	ssize_t numBytes
)
{

	PthreadMutexLock(&checksum->mutex);

	while (checksum->isComplete == B_FALSE) {
		PthreadCondWait(&checksum->complete, &checksum->mutex);
	}
	checksum->data = data;
	checksum->numBytes = numBytes;
	checksum->isAvail = B_TRUE;
	checksum->isComplete = B_FALSE;

	PthreadCondSignal(&checksum->avail);
	PthreadMutexUnlock(&checksum->mutex);
}

/*
 * Wait for checksum to complete.
 */
void
ChecksumWait(void)
{
	PthreadMutexLock(&checksum->mutex);
	while (checksum->isComplete == B_FALSE) {
		PthreadCondWait(&checksum->complete, &checksum->mutex);
	}

	Trace(TR_DEBUG, "[%s] Checksum complete: -a %d 0x%.8x%.8x 0x%.8x%.8x",
	    File->f->FiName,
	    checksum->algo,
	    File->AfCsum.csum_val[0], File->AfCsum.csum_val[1],
	    File->AfCsum.csum_val[2], File->AfCsum.csum_val[3]);

	PthreadMutexUnlock(&checksum->mutex);
}

/*
 * Checksum worker thread.
 */
static void*
checksumWorker(
	/* LINTED argument unused in function */
	void *arg)
{
	while (checksum->done == B_FALSE) {

		PthreadMutexLock(&checksum->mutex);
		while (checksum->isAvail == B_FALSE) {
			PthreadCondWait(&checksum->avail, &checksum->mutex);
		}

		Trace(TR_DEBUG, "[%s] Checksumming data: 0x%x bytes: %d",
		    File->f->FiName, (long)checksum->data, checksum->numBytes);

		PthreadMutexUnlock(&checksum->mutex);

		if (checksum->done == B_TRUE) {
			break;
		}

		if (checksum->algo & CS_USER_BIT) {
			checksum->func(0, checksum->algo,
			    (uchar_t *)checksum->data, checksum->numBytes,
			    &File->AfCsum);
		} else {
			checksum->func(0, (uchar_t *)checksum->data,
			    checksum->numBytes, &File->AfCsum);
		}

		PthreadMutexLock(&checksum->mutex);
		checksum->isComplete = B_TRUE;
		checksum->isAvail = B_FALSE;
		PthreadCondSignal(&checksum->complete);
		PthreadMutexUnlock(&checksum->mutex);
	}
	return (NULL);
}

int
writeCsumFile(char* fn, csum_t* csum)
{
	int dfd;
	int dirfd;
	int afd;
	char abspath[PATH_MAX];
	char compname[PATH_MAX];
	char csumbuf[256] = {'\0'};
	char *csp = csumbuf;
	char *fnpos;

	fnpos = strrchr(fn, '/');
	if (fnpos == NULL)
		sprintf(abspath, "%s", MntPoint);
	else {
		sprintf(compname, "%s", fnpos+1);
		sprintf(abspath, "%s/%s", MntPoint, fn);
	}
	dfd = open(abspath, O_RDONLY);
	if (dfd < 0) {
	        Trace(TR_ERR, "failed open: %s, %s", abspath, strerror(errno));
	        return (errno);
	}
	dirfd = openat(dfd, ".", O_RDONLY|O_XATTR);
	close(dfd);
	if (dirfd < 0) {
	        Trace(TR_ERR, "failed open attr dir: %s, %s",
		    abspath, strerror(errno));
	        return (errno);
	}
	if (fchdir(dirfd) == -1) {
		close(dirfd);
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
	switch (checksum->algo) {
	        case CS_SIMPLE:
	                int i;
	                for(i=0; i<4; i++) {
	                        sprintf(csp,"%08x", csum->csum_val[i]);
	                        csp += 8;
	                }
	                break;
	        default:
			Trace(TR_ERR, "unknown csum algo: %d", checksum->algo);
			return (errno);
	                ;;
	}
	if(dprintf(afd, "%s:%s", csum_algo[checksum->algo], csumbuf) < 0) {
	        Trace(TR_ERR, "failed write: %s/%s, %s",
		    abspath, FN_CSUM, strerror(errno));
	        return (errno);
	}

	close(afd);
	close(dirfd);
	return (0);
}
