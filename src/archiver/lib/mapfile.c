/*
 * mapfile.c - memory mapped file access functions.
 *
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

#pragma ident "$Revision: 1.9 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <sys/mman.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/sam_malloc.h"

/* Local headers. */
#include "common.h"
#include "threads.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Private data. */
static struct MfEntry {
	void	*MeLoc;
	uint32_t MeLen;
	upath_t	MeName;
} *mapFileTable = NULL;
static pthread_mutex_t mapFileTableMutex = PTHREAD_MUTEX_INITIALIZER;
static int mapFileTableCount = 0;

/* Private functions. */
static int mapFileAlloc(int fd, size_t from, size_t to);
static struct MappedFile *mapFileAttach(char *fileName, uint_t magic, int mode,
		size_t *len);


/*
 * Map a file into memory.
 * mmap() the requested file.
 * RETURN: Pointer to mapped area.  NULL if failed.
 */
void *
ArMapFileAttach(
	char *fileName,		/* Name of file to mmap(). */
	uint_t magic,		/* Magic number for file */
	int mode)		/* O_RDONLY = read only, read/write otherwise */
{
	struct MappedFile *mf;
	size_t	len;

	mf = mapFileAttach(fileName, magic, mode, &len);
	if (mf != NULL) {
		struct MfEntry *me;
		int	i;

		PthreadMutexLock(&mapFileTableMutex);
		for (i = 0; i < mapFileTableCount; i++) {
			if (mapFileTable[i].MeLoc == NULL) {
				break;
			}
		}
		if (i == mapFileTableCount) {
			SamRealloc(mapFileTable, (mapFileTableCount + 1) *
			    sizeof (struct MfEntry));
			mapFileTableCount++;
		}
		me = &mapFileTable[i];
		me->MeLoc = mf;
		me->MeLen = len;
		strncpy(me->MeName, fileName, sizeof (me->MeName));
		Trace(TR_ARDEBUG, "mmap(%s, %p, %d)", me->MeName,
		    (void *)me->MeLoc, me->MeLen);
		PthreadMutexUnlock(&mapFileTableMutex);
		return (mf);
	}
	return (NULL);
}


/*
 * Create a mapped file.
 */
void *
MapFileCreate(
	char *fileName,		/* Name of file to create and mmap(). */
	uint_t magic,		/* Magic number for file */
	size_t size)
{
	struct MappedFile *mf, mfp;
	int	fd;
	int	saveErrno;

	if ((fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0640)) < 0) {
		return (NULL);
	}
	memset(&mfp, 0, sizeof (mfp));
	mfp.MfMagic = magic;
	mfp.MfLen = size;
	mf = NULL;
	if (write(fd, &mfp, sizeof (mfp)) != sizeof (mfp)) {
		goto out;
	}
	if (mapFileAlloc(fd, sizeof (mfp), size) == -1) {
		goto out;
	}
	mf = ArMapFileAttach(fileName, magic, O_RDWR);

out:
	saveErrno = errno;
	(void) close(fd);
	errno = saveErrno;
	return (mf);
}


/*
 * Detach mapped file.
 * RETURN: -1 if failed.
 */
int
ArMapFileDetach(
	void *mf_a)		/* Pointer to mapped file */
{
	struct MappedFile *mf = (struct MappedFile *)mf_a;
	int	i;
	int	ret;

	PthreadMutexLock(&mapFileTableMutex);
	for (i = 0; i < mapFileTableCount; i++) {
		if (mapFileTable[i].MeLoc == mf) {
			break;
		}
	}

	if (i < mapFileTableCount) {
		struct MfEntry *me;

		me = &mapFileTable[i];
		/* man page says: int munmap(void *addr, size_t len); */
		ret = munmap((char *)me->MeLoc, me->MeLen);
		mapFileTable[i].MeLoc = NULL;
		Trace(TR_ARDEBUG, "munmap(%s, %p, %d)", me->MeName,
		    (void *)me->MeLoc, me->MeLen);
	} else {
		Trace(TR_ERR, "Mapped file %p not in table.", (void *)mf);
		ret = -1;
	}
	PthreadMutexUnlock(&mapFileTableMutex);
	return (ret);
}


/*
 * Increase the size of mapped file.
 */
void *
MapFileGrow(
	void *mf_a,
	size_t incr)
{
	struct MappedFile *mf = (struct MappedFile *)mf_a;
	struct MfEntry *me;
	size_t	newLen;
	uint_t magic;
	int	fd;
	int	i;

	PthreadMutexLock(&mapFileTableMutex);
	for (i = 0; i < mapFileTableCount; i++) {
		me = &mapFileTable[i];
		if (me->MeLoc == mf) {
			break;
		}
	}
	if (i >= mapFileTableCount) {
		Trace(TR_ERR, "Mapped file %p not in table.", (void *)mf);
		mf = NULL;
		goto out;
	}
	newLen = me->MeLen + incr;
	mf->MfValid = 0;
	mf->MfLen = newLen;
	magic = mf->MfMagic;
	if (munmap((char *)mf, me->MeLen) == -1) {
		mf = NULL;
		goto out;
	}

	/*
	 * Increase size of file and re-mmap it.
	 */
	mf = NULL;
	if ((fd = open(me->MeName, O_WRONLY)) < 0) {
		goto out;
	}
	if (mapFileAlloc(fd, me->MeLen, newLen) == -1) {
		(void) close(fd);
		goto out;
	}
	if (close(fd) == -1) {
		goto out;
	}

	mf = mapFileAttach(me->MeName, magic, O_RDWR, &newLen);

out:
	if (mf != NULL) {
		me->MeLoc = mf;
		me->MeLen = newLen;
		mf->MfValid = 1;
		Trace(TR_ARDEBUG, "grow(%s, %p, %d, %d)", me->MeName,
		    (void *)me->MeLoc, incr, me->MeLen);
	} else {
		me->MeLoc = NULL;
	}
	PthreadMutexUnlock(&mapFileTableMutex);
	return (mf);
}


/*
 * Rename mapped file.
 */
int
MapFileRename(
	void *mf_a,
	char *newName)		/* New mapped file name */
{
	struct MappedFile *mf = (struct MappedFile *)mf_a;
	int	i;
	int	retval;

	retval = -1;
	PthreadMutexLock(&mapFileTableMutex);
	for (i = 1; i < mapFileTableCount; i++) {
		struct MfEntry *me;

		me = &mapFileTable[i];
		if (me->MeLoc == mf) {
			if ((retval = rename(me->MeName, newName)) == 0) {
				strncpy(me->MeName, newName,
				    sizeof (me->MeName));
			}
			break;
		}
	}
	if (i >= mapFileTableCount) {
		Trace(TR_ERR, "Mapped file %p not in table.", (void *)mf);
	}
	PthreadMutexUnlock(&mapFileTableMutex);
	return (retval);
}



/*
 * Trace the mapped file table.
 */
void
MapFileTrace(void)
{
	FILE	*st;
	int	i;

	Trace(TR_MISC, "Mapped files: %d", mapFileTableCount);
	if ((st = TraceOpen()) == NULL) {
		return;
	}
	for (i = 0; i < mapFileTableCount; i++) {
		struct MfEntry *me;

		me = &mapFileTable[i];
		if (me->MeLoc != NULL) {
			struct MappedFile *mf;

			mf = me->MeLoc;
			fprintf(st, "%d %p %d %s\n", i, (void *)mf,
			    me->MeLen, me->MeName);
		}
	}
	fprintf(st, "\n");
	TraceClose(-1);
}

/* Private functions. */


/*
 * Allocate mapped file.
 * The file must be preallocated by writing it to assure that all pages
 * are available.
 * RETURN: 0 if successful, -1 if error encountered.
 */
static int
mapFileAlloc(
	int fd,
	size_t from,
	size_t to)
{
	static void *blankPage = NULL;
	static size_t pageSize;

	if (blankPage == NULL) {
		pageSize = sysconf(_SC_PAGESIZE);
		SamMalloc(blankPage, pageSize);
		memset(blankPage, 0, pageSize);
	}
	while (from < to) {
		int	n;

		if (pageSize < (to - from)) {
			n = pageSize;
		} else {
			n = to - from;
		}
		if (lseek(fd, from, SEEK_SET) == -1) {
			return (-1);
		}
		if (write(fd, blankPage, n) != n) {
			return (-1);
		}
		from += n;
	}
	return (0);
}


/*
 * Attach a file.
 */
static struct MappedFile *
mapFileAttach(
	char *fileName,
	uint_t magic,
	int mode,
	size_t *len)
{
	struct MappedFile *mf;
	struct stat st;
	void	*mp;
	int	saveErrno;
	int	fd;
	int	prot;

	prot = (O_RDONLY == mode) ? PROT_READ : PROT_READ | PROT_WRITE;
	mode = (O_RDONLY == mode) ? O_RDONLY : O_RDWR;
	if ((fd = open(fileName, mode)) == -1) {
		return (NULL);
	}
	if (fstat(fd, &st) != 0) {
		saveErrno = errno;
		(void) close(fd);
		errno = saveErrno;
		return (NULL);
	}
	mp = mmap(NULL, st.st_size, prot, MAP_SHARED, fd, 0);
	saveErrno = errno;
	(void) close(fd);
	if (mp == MAP_FAILED) {
		errno = saveErrno;
		return (NULL);
	}
	mf = (struct MappedFile *)mp;
	if (st.st_size < sizeof (struct MappedFile)) {
		errno = EINVAL;
	} else if (mf->MfMagic != magic) {
		errno = EBADRQC;
	} else if (st.st_size != mf->MfLen) {
		errno = EINVAL;
	} else {
		*len = st.st_size;
		return (mp);
	}
	return (NULL);
}
