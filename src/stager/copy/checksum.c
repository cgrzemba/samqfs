/*
 * checksum.c - stager's checksum processing
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

#pragma ident "$Revision: 1.20 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <sys/param.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>

/* Solaris headers. */
#include <sys/sysmacros.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "sam/uioctl.h"
#include "pub/sam_errno.h"
#include "sam/checksum.h"
#define	DEC_INIT
#include "sam/checksumf.h"
#undef DEC_INIT
#include "sam/custmsg.h"
#include "aml/tar.h"
#include "aml/stager_defs.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Local headers. */
#include "stager_lib.h"
#include "stager_threads.h"
#include "stager_config.h"
#include "copy_defs.h"

#include "copy.h"

/*
 * Context for checksum.
 */
typedef struct checksumInfo {
	pthread_t	id;		/* checksum's thread id */
	boolean_t	done;		/* staging on file complete */

	pthread_mutex_t	mutex;

	boolean_t	is_avail;	/* data available to checksum */
	pthread_cond_t	avail;
	boolean_t	is_complete;	/* checksumming is complete */
	pthread_cond_t	complete;

	uchar_t		algo;		/* checksum algorithm */
	csum_func	func;		/* function */
	csum_t		val;		/* value accumulator */
	u_longlong_t	seed;
	boolean_t	is_initialized;

	char 		*data;		/* data in buffer to checksum */
	int		num_bytes;	/* number of bytes in buffer */
} checksumInfo_t;

/* Public data. */
extern CopyInstanceInfo_t *Instance;

/* Private data. */
static checksumInfo_t *checksum = NULL;

/* Private functions. */
static void* checksumWorker(void *arg);

/*
 * Initialize checksumming.
 */
void
ChecksumInit(
	FileInfo_t *file,
	boolean_t alreadyInitialized
)
{
	boolean_t startWorker;

	if (checksum == NULL) {
		SamMalloc(checksum, sizeof (checksumInfo_t));
		memset(checksum, 0, sizeof (checksumInfo_t));

		PthreadMutexInit(&checksum->mutex, NULL);
		PthreadCondInit(&checksum->avail, NULL);
		PthreadCondInit(&checksum->complete, NULL);

		startWorker = B_TRUE;
	} else {
		startWorker = B_FALSE;
	}

	if (checksum->algo & CS_USER_BIT) {
		checksum->seed = 0;
	} else {
		/*
		 * The checksum is preset with the space required for the
		 * tar file. This is the file length + 512, rounded up to
		 * the next 512 byte boundary.
		 * If the file name is > 100 characters, we need another tar
		 * header and tar record as well for the long file name.
		 */
		checksum->seed = file->len + TAR_RECORDSIZE;
		checksum->seed = roundup(checksum->seed, TAR_RECORDSIZE);

		if (file->namelen > NAMSIZ) {
			checksum->seed += file->namelen + TAR_RECORDSIZE;
			checksum->seed =
			    roundup(checksum->seed, TAR_RECORDSIZE);
		}
	}

	Trace(TR_FILES, "Checksum init inode: %d.%d algo: %d seed: %lld",
	    file->id.ino, file->id.gen, file->csum_algo & ~CS_USER_BIT,
	    checksum->seed);

	checksum->algo = file->csum_algo;
	memset(&checksum->val, 0, sizeof (csum_t));
	checksum->is_initialized = alreadyInitialized;
	checksum->done = B_FALSE;
	checksum->is_complete = B_FALSE;
	checksum->is_avail = B_FALSE;

	if (startWorker == B_TRUE) {
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
Checksum(
	char *data,
	int num_bytes)
{
	ASSERT(checksum != NULL);

	Trace(TR_DEBUG, "Checksumming data: 0x%x bytes: %d",
	    (int)data, num_bytes);

	PthreadMutexLock(&checksum->mutex);

	checksum->data = data;
	checksum->num_bytes = num_bytes;
	checksum->is_avail = B_TRUE;
	checksum->is_complete = B_FALSE;

	PthreadCondSignal(&checksum->avail);

	PthreadMutexUnlock(&checksum->mutex);
}

/*
 * Wait for checksum thread to complete accumulation of the checksum
 * value for a data block.
 */
void
ChecksumWait(void)
{
	ASSERT(checksum != NULL);

	PthreadMutexLock(&checksum->mutex);
	while (checksum->is_complete == B_FALSE) {
		PthreadCondWait(&checksum->complete, &checksum->mutex);
	}
	PthreadMutexUnlock(&checksum->mutex);
}

/*
 * Done staging file.  Compare accumulated checksum value on stage
 * against value generated during archive.  Upon successful completion
 * return a zero.  Otherwise, this function returns errno.
 */
int
ChecksumCompare(
	int fd,
	sam_id_t *id)
{
	struct sam_perm_inode pi;
	struct sam_ioctl_idstat idstat;
	int i;
	int retval;

	ASSERT(checksum != NULL);

	retval = 0;

	idstat.id = *id;
	idstat.size = sizeof (pi);
	idstat.dp.ptr = &pi;
	if (ioctl(fd, F_IDSTAT, &idstat) != 0) {
		retval = errno;
	}

	if (retval == 0 && checksum != NULL) {
		for (i = 0; i < 4; i++) {
			if (pi.csum.csum_val[i] != checksum->val.csum_val[i]) {
				retval = EDVVCMP;
				break;
			}
		}

		/* Checksum failed.  Try endian repair calculation. */
		if (retval == EDVVCMP && pi.di.cs_algo == CS_SIMPLE) {
			retval = 0;
			Trace(TR_MISC, "Trying alternate-endian checksum "
			    "inode: %d.%d", id->ino, id->gen);
			Trace(TR_FILES, "Checksum inode: %d.%d "
			    "length: %lld cookie: %lld",
			    id->ino, id->gen, (u_longlong_t)pi.di.rm.size,
			    checksum->seed);
			cs_repair((uchar_t *)&pi.csum.csum_val[0],
			    &checksum->seed);
			for (i = 0; i < 4; i++) {
				if (pi.csum.csum_val[i] !=
				    checksum->val.csum_val[i]) {
					retval = EDVVCMP;
					break;
				}
			}
		}

		if (retval == EDVVCMP) {
			SetErrno = 0;		/* set for trace */
			Trace(TR_ERR, "Checksum error inode: %d.%d",
			    id->ino, id->gen);
			Trace(TR_ERR, "Checksum value: %.8x %.8x %.8x %.8x",
			    pi.csum.csum_val[0], pi.csum.csum_val[1],
			    pi.csum.csum_val[2], pi.csum.csum_val[3]);
			Trace(TR_ERR, "Checksum calc: %.8x %.8x %.8x %.8x",
			    checksum->val.csum_val[0],
			    checksum->val.csum_val[1],
			    checksum->val.csum_val[2],
			    checksum->val.csum_val[3]);
		}
	}

	return (retval);
}

/*
 * Get checksum value accumulator.
 */
csum_t
ChecksumGetVal(void)
{
	return (checksum->val);
}

/*
 * Set checksum value accumulator.
 */
void
ChecksumSetVal(
	csum_t val)
{
	checksum->val = val;
}

/*
 * Checksum work thread.
 */
static void*
checksumWorker(
	/* LINTED argument unused in function */
	void *arg)
{
	ASSERT(checksum != NULL);

	while (checksum->done == B_FALSE) {

		PthreadMutexLock(&checksum->mutex);

		while (checksum->is_avail == B_FALSE) {
			PthreadCondWait(&checksum->avail, &checksum->mutex);
		}

		PthreadMutexUnlock(&checksum->mutex);

		if (checksum->done == B_TRUE) {
			break;
		}

		if (checksum->is_initialized == B_FALSE) {
			if (checksum->algo & CS_USER_BIT) {
				checksum->func = cs_user;
				cs_user(&checksum->seed, checksum->algo, 0, 0,
				    &checksum->val);
			} else {
				checksum->func = csum[checksum->algo];
				csum[checksum->algo](&checksum->seed, 0, 0,
				    &checksum->val);
			}
			checksum->is_initialized = B_TRUE;
		}

		if (checksum->algo & CS_USER_BIT) {
			checksum->func(0, checksum->algo, checksum->data,
			    checksum->num_bytes, &checksum->val);
		} else {
			checksum->func(0, checksum->data, checksum->num_bytes,
			    &checksum->val);
		}

		PthreadMutexLock(&checksum->mutex);
		checksum->is_complete = B_TRUE;
		checksum->is_avail = B_FALSE;
		PthreadCondSignal(&checksum->complete);
		PthreadMutexUnlock(&checksum->mutex);
	}

	return (NULL);
}
