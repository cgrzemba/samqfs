/*
 * mapfile.c - memory mapped file access functions.
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.18 $"

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/* POSIX headers. */
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <syslog.h>
#include <sys/mman.h>

/* SAM-FS headers. */
#include "sam/types.h"
#undef DEVICE_DEFAULTS
#include "sam/defaults.h"

#ifdef sun
#include "aml/robots.h"
#else
	/*
	 * Lifted from robots.h
	 */
#define	INITIAL_MOUNT_TIME	30
#endif /* sun */

#include "sam/lib.h"
#include "sam/names.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Private data. */
static struct DefaultsBin *def_bin = NULL;

#include "sam/defaults.hc"
/*
 * Get site defaults.
 * Error exit if file cannot be attached.
 */
sam_defaults_t *
GetDefaults(void)
{
	static struct sam_defaults defaults;

	if (def_bin == NULL || def_bin->Db.MfValid == 0) {

		/*
		 * defaults.bin missing or invalid.
		 */
		if (def_bin != NULL) {
			(void) MapFileDetach(def_bin);
		}
		def_bin = MapFileAttach(DEFAULTS_BIN, DEFAULTS_MAGIC, O_RDONLY);
		if (def_bin == NULL) {
			SetFieldDefaults(&defaults, Defaults);
			return (&defaults);
		}
	}
	return (&def_bin->DbDefaults);
}


/*
 * Get site defaults in read/write mode.
 */
sam_defaults_t *
GetDefaultsRw(void)
{
	if (def_bin != NULL) {
		(void) MapFileDetach(def_bin);
	}
	def_bin = MapFileAttach(DEFAULTS_BIN, DEFAULTS_MAGIC, O_RDWR);
	if (def_bin == NULL) {
		return (NULL);
	}
	return (&def_bin->DbDefaults);
}


/*
 * Map a file into memory.
 * mmap() the requested file.
 * Return a pointer to the mapped area.
 * Return size in a user provided variable.
 * Returns pointer to mapped area.  NULL if failed.
 */
void *
MapFileAttach(
	char *fileName,		/* Name of file to mmap(). */
	uint_t magic,		/* Magic number for file */
	int mode)		/* O_RDONLY = read only, read/write otherwise */
{
	struct stat st;
	struct MappedFile *mf;
	void *mp;
	int saveErrno;
	int fd;
	int prot;

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
		return (mp);
	}
	return (NULL);
}


/*
 * Detach mapped file.
 * Returns -1 if failed.
 */
int
MapFileDetach(
	void *mf_a)		/* Pointer to mapped file */
{
	struct MappedFile *mf = (struct MappedFile *)mf_a;

	/* man page says: int munmap(void *addr, size_t len); */
	return (munmap((char *)mf, mf->MfLen));
}
