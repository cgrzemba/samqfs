/*
 * checksum.c - stager's checksum processing
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#pragma ident "$Revision: 1.17 $"

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

/* Local headers. */
#include "stager_lib.h"
#include "stager_config.h"
#include "stage_reqs.h"
#include "copy_defs.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/*
 * Context for checksum.
 */
typedef struct checksumInfo {
	pthread_t	id;		/* checksum's thread id */
	boolean_t	done;		/* staging on file complete */

	pthread_mutex_t	mutex;

	int		is_avail;	/* data available to checksum */
	pthread_cond_t	avail;
	int		is_complete;	/* checksumming is complete */
	pthread_cond_t	complete;

	uchar_t		algo;		/* checksum algorithm */
	csum_func	func;		/* function */
	csum_t		val;		/* value accumulator */
	u_longlong_t	cookie;
	boolean_t	is_initialized;

	char 		*data;		/* data in buffer to checksum */
	int		num_bytes;	/* number of bytes in buffer */
} checksumInfo_t;

/* Public data. */
extern CopyInstance_t *Context;
extern CopyControl_t *Control;

/* Private data. */
static checksumInfo_t *checksum = NULL;

/* Private functions. */
static void* checksumWorker(void *arg);

/*
 * Initialize checksumming.
 */
void
InitChecksum(
	FileInfo_t *file,
	boolean_t init
)
{
	Trace(TR_MISC, "Checksum file inode: %d", file->id.ino);

	if (checksum == NULL) {
		int status;

		SamMalloc(checksum, sizeof (checksumInfo_t));
		memset(checksum, 0, sizeof (checksumInfo_t));

		status = pthread_mutex_init(&checksum->mutex, NULL);
		ASSERT(status == 0);

		status = pthread_cond_init(&checksum->avail, NULL);
		ASSERT(status == 0);

		status = pthread_cond_init(&checksum->complete, NULL);
		ASSERT(status == 0);
	}

	if (checksum->algo & CS_USER_BIT) {
		checksum->cookie = 0;
	} else {
		/*
		 * The checksum is preset with the space required for the
		 * tar file. This is the file length + 512, rounded up to
		 * the next 512 byte boundary.
		 * If the file name is > 100 characters, we need another tar
		 * header and tar record as well for the long file name.
		 */
		checksum->cookie = file->len + TAR_RECORDSIZE;
		checksum->cookie = roundup(checksum->cookie, TAR_RECORDSIZE);

		if (file->namelen > NAMSIZ) {
			checksum->cookie += file->namelen + TAR_RECORDSIZE;
			checksum->cookie =
			    roundup(checksum->cookie, TAR_RECORDSIZE);
		}
	}
	checksum->algo = file->csum_algo;
	memset(&checksum->val, 0, sizeof (csum_t));
	checksum->is_initialized = init;
	checksum->done = B_FALSE;
	checksum->is_complete = 0;
	checksum->is_avail = 0;

	if (pthread_create(&checksum->id, NULL, checksumWorker, NULL) != 0) {
		LibFatal(pthread_create, NULL);
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
	int status;
	ASSERT(checksum != NULL);

	Trace(TR_DEBUG, "Checksumming data: 0x%x bytes: %d",
	    (int)data, num_bytes);

	status = pthread_mutex_lock(&checksum->mutex);
	ASSERT(status == 0);

	checksum->data = data;
	checksum->num_bytes = num_bytes;
	checksum->is_avail = 1;
	checksum->is_complete = 0;

	status = pthread_cond_signal(&checksum->avail);
	ASSERT(status == 0);

	status = pthread_mutex_unlock(&checksum->mutex);
	ASSERT(status == 0);
}

void
WaitForChecksum(void)
{
	int status;
	ASSERT(checksum != NULL);

	status = pthread_mutex_lock(&checksum->mutex);
	while (checksum->is_complete == 0) {
		status = pthread_cond_wait(&checksum->complete,
		    &checksum->mutex);
		ASSERT(status == 0);
	}
	status = pthread_mutex_unlock(&checksum->mutex);
	ASSERT(status == 0);
}

/*
 * Set flag for worker thread to indicate staging is done.
 */
void
SetChecksumDone(void)
{
	int status;
	ASSERT(checksum != NULL);

	if (checksum != NULL) {
		status = pthread_mutex_lock(&checksum->mutex);
		ASSERT(status == 0);

		checksum->done = B_TRUE;
		checksum->is_avail = 1;

		status = pthread_cond_signal(&checksum->avail);
		ASSERT(status == 0);
		status = pthread_mutex_unlock(&checksum->mutex);
		ASSERT(status == 0);

		(void) pthread_join(checksum->id, NULL);
	}
}

/*
 *
 */
int
ChecksumCompare(
	int fd,
	sam_id_t *id)
{
	struct sam_perm_inode pi;
	struct sam_ioctl_idstat idstat;
	int i;

	int error = 0;
	ASSERT(checksum != NULL);

	idstat.id = *id;
	idstat.size = sizeof (pi);
	idstat.dp.ptr = &pi;
	if (ioctl(fd, F_IDSTAT, &idstat) != 0) {
		error = errno;
	}

	if (error == 0 && checksum != NULL) {
		for (i = 0; i < 4; i++) {
			if (pi.csum.csum_val[i] != checksum->val.csum_val[i]) {
				error = EDVVCMP;
				break;
			}
		}
		if (error == EDVVCMP && pi.di.cs_algo == CS_SIMPLE) {
			error = 0;
			Trace(TR_MISC, "Trying alternate-endian checksum "
			    "inode: %d.%d", id->ino, id->gen);
			Trace(TR_MISC, "Checksum inode: %d.%d "
			    "length %lld cookie %lld",
			    id->ino, id->gen, (u_longlong_t)pi.di.rm.size,
			    checksum->cookie);
			cs_repair((uchar_t *)&pi.csum.csum_val[0],
			    &checksum->cookie);
			for (i = 0; i < 4; i++) {
				if (pi.csum.csum_val[i] !=
				    checksum->val.csum_val[i]) {
					error = EDVVCMP;
					break;
				}
			}
		}
		if (error == EDVVCMP) {
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

	return (error);
}

/*
 * Get checksum value accumulator.
 */
csum_t
GetChecksumVal(void)
{
	return (checksum->val);
}

/*
 * Set checksum value accumulator.
 */
void
SetChecksumVal(
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
	int status;
	ASSERT(checksum != NULL);

	while (checksum->done == B_FALSE) {

		status = pthread_mutex_lock(&checksum->mutex);
		ASSERT(status == 0);

		while (checksum->is_avail == 0) {
			status = pthread_cond_wait(&checksum->avail,
			    &checksum->mutex);
			ASSERT(status == 0);
		}

		status = pthread_mutex_unlock(&checksum->mutex);
		ASSERT(status == 0);

		if (checksum->done) {
			break;
		}

		if (checksum->is_initialized == B_FALSE) {
			if (checksum->algo & CS_USER_BIT) {
				checksum->func = cs_user;
				cs_user(&checksum->cookie, checksum->algo, 0, 0,
				    &checksum->val);
			} else {
				checksum->func = csum[checksum->algo];
				csum[checksum->algo](&checksum->cookie, 0, 0,
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

		status = pthread_mutex_lock(&checksum->mutex);
		ASSERT(status == 0);
		checksum->is_complete = 1;
		checksum->is_avail = 0;
		status = pthread_cond_signal(&checksum->complete);
		ASSERT(status == 0);
		status = pthread_mutex_unlock(&checksum->mutex);
		ASSERT(status == 0);

	}

	return (NULL);
}
