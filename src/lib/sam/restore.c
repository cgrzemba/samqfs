/*
 * restore.c
 *
 * sam_mig_create_file - Third party API to create a file in the
 * SAM-FS file system.
 *
 * sam_restore_file - API to restore a file into the SAM-FS file system.
 * sam_restore_copy - API to restore an archive copy for an existing file
 * in the SAM-FS file system.
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

#pragma ident "$Revision: 1.25 $"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "sam/fioctl.h"
#include "sam/uioctl.h"
#include "sam/lib.h"
#include "sam/fs/ino.h"
#include "sam/fs/ino_ext.h"
#include "pub/mig.h"
#include "pub/stat.h"
#include "pub/devstat.h"

#define	SAM_MIGRATE_FILE	1
#define	SAM_RESTORE_FILE	2

static int sam_create_file(char *path, int type, struct sam_stat *s_buf,
	size_t bufsize);

/*
 * sam_mig_create_file
 * Third party API to create a file in the SAM-FS file system.
 */
int
sam_mig_create_file(char *path, struct sam_stat *s_buf)
{
	return (sam_create_file(path, SAM_MIGRATE_FILE, s_buf,
	    sizeof (struct sam_stat)));
}


/*
 * sam_restore_file
 * User API to restore a file and one or more copies in the SAM-FS file system.
 */
int
sam_restore_file(const char *path, struct sam_stat *buf, size_t bufsize)
{
	if (getuid() != 0) {
		errno = EPERM;
		return (-1);
	}
	return (sam_create_file((char *)path, SAM_RESTORE_FILE, buf, bufsize));
}


/*
 * sam_create_file
 * Common code to create file in a SAM-FS filesystem.
 */
static int
sam_create_file(char *path, int type, struct sam_stat *s_buf, size_t bufsize)
{
	int copy;
	int fd;
	struct sam_perm_inode  inode;
	struct sam_ioctl_idrestore idrestore;

	if (bufsize < sizeof (struct sam_stat)) {
		errno = EINVAL;
		return (-6);
	}

	memset(&idrestore, 0, sizeof (idrestore));
	memset(&inode, 0, sizeof (inode));
	inode.di.version = 1;
	inode.di.mode = ((s_buf->st_mode & S_IAMB) | S_IFREG);
	inode.di.uid = s_buf->st_uid;
	inode.di.gid = s_buf->st_gid;
	inode.di.rm.size = s_buf->st_size;
	inode.di.access_time.tv_sec = s_buf->st_atime;
	inode.di.modify_time.tv_sec = s_buf->st_mtime;
	inode.di.change_time.tv_sec = s_buf->st_ctime;
	inode.di.creation_time = (s_buf->creation_time == 0) ?
	    time(NULL) : s_buf->creation_time;
	inode.di.attribute_time = time(NULL);
	inode.di.residence_time = time(NULL);

	/* Fill in archive information for each copy if any */
	for (copy = 0;  copy < MAX_ARCHIVE; copy++) {
		struct sam_copy_s *stat_ar_copy;
		sam_archive_info_t  *ar;

		stat_ar_copy = &s_buf->copy[copy];
		ar = &inode.ar.image[copy];

		/* Verify valid media type */
		if (type == SAM_MIGRATE_FILE) {
			if (stat_ar_copy->media[3] == '\0') {
				continue;
			}
			if (!(islower(stat_ar_copy->media[2]) ||
			    isdigit(stat_ar_copy->media[2]))) {
				errno = EINVAL;
				return (-2);
			}
			if (!(islower(stat_ar_copy->media[3]) ||
			    isdigit(stat_ar_copy->media[3]))) {
				errno = EINVAL;
				return (-2);
			}

			inode.di.media[copy] =
			    (DT_THIRD_PARTY | (stat_ar_copy->media[3] & 0xff));

		} else if (type == SAM_RESTORE_FILE) {
			int media;

			if (stat_ar_copy->vsn[0] == '\0') {
				continue;
			}
			ar->arch_flags |= SAR_size_block;
			media = nm_to_device(stat_ar_copy->media);
			if (media <= 0) {
				errno = EINVAL;
				return (-2);
			}
			inode.di.media[copy] = (media_t)media;
			if (media == DT_DISK) {
				ar->arch_flags |= SAR_diskarch;
			}
		}
		ar->file_offset = stat_ar_copy->offset;
		ar->n_vsns = 1;
		ar->creation_time =
		    (stat_ar_copy->creation_time == 0) ? time(NULL) :
		    stat_ar_copy->creation_time;
		ar->position_u = stat_ar_copy->position >> 32;
		ar->position = stat_ar_copy->position & 0xffffffff;
		if ((stat_ar_copy->vsn[0] == '\0') ||
		    strlen(stat_ar_copy->vsn) > (sizeof (vsn_t) -1)) {
			errno = EINVAL;
			return (-3);
		}

		memccpy(ar->vsn, stat_ar_copy->vsn, '\0', sizeof (vsn_t) - 1);
		inode.di.arch_status |= (1 << copy);
	}

	inode.di.status.b.archdone = TRUE;
	inode.di.status.b.offline = TRUE;

	if ((fd = open(path, O_CREAT | O_EXCL, s_buf->st_mode)) < 0) {
		return (-5);
	}
	idrestore.dp.ptr = &inode;
	if (ioctl(fd, F_IDRESTORE, &idrestore) < 0) {
		(void) close(fd);
		(void) unlink(path);
		return (-6);
	}
	(void) close(fd);
	return (0);
}


/*
 * sam_restore_copy
 * User API to restore a copy for an existing file in the SAM-FS file system.
 */
int
sam_restore_copy(const char *path, int copy, struct sam_stat *s_buf,
	size_t bufsize, struct sam_section *vbuf, size_t vbufsize)
{
	int fd;
	int media;
	int n_vsns;
	struct sam_stat statbuf;
	struct sam_copy_s *ar;
	struct sam_ioctl_setarch sa;

	if (getuid() != 0) {
		errno = EPERM;
		return (-1);
	}
	if (copy >= MAX_ARCHIVE) {
		errno = EINVAL;
		return (-2);
	}
	if (bufsize < sizeof (struct sam_stat)) {
		errno = EINVAL;
		return (-10);
	}
	ar = &s_buf->copy[copy];
	if (ar->vsn[0] == '\0') {
		errno = EINVAL;
		return (-3);
	}
	if (sam_stat(path, &statbuf, sizeof (struct sam_stat)) < 0) {
		return (-4);
	}

	if ((statbuf.st_uid != s_buf->st_uid) ||
	    (statbuf.st_gid != s_buf->st_gid)) {
		return (-6);
	}

	if (statbuf.copy[copy].vsn[0] != '\0') {
		errno = EINVAL;
		return (-7);
	}

	memset(&sa, 0, sizeof (struct sam_ioctl_setarch));
	if (vbuf == NULL) {
		n_vsns = 1;
	} else {
		n_vsns = ar->n_vsns;
		sa.vp.ptr = (struct sam_vsn_section *)vbuf;
		if (vbufsize == 0 ||
		    (vbufsize / sizeof (struct sam_section) != n_vsns) ||
			(vbufsize % sizeof (struct sam_section) != 0)) {
			errno = EINVAL;
			return (-8);
		}
	}

	sa.id.ino = statbuf.st_ino;
	sa.id.gen = statbuf.gen;
	media = nm_to_device(ar->media);
	if (media <= 0) {
		errno = EINVAL;
		return (-9);
	}
	sa.media = (media_t)media;

	if ((fd = open(path, O_RDWR | SAM_O_LARGEFILE, statbuf.st_mode)) < 0) {
		return (-5);
	}

	sa.copy = copy;
	sa.access_time.tv_sec = statbuf.st_atime;
	sa.modify_time.tv_sec = statbuf.st_mtime;
	sa.ar.n_vsns  = n_vsns;
	sa.ar.version = 0;
	sa.ar.creation_time = ar->creation_time;
	sa.ar.position_u = ar->position >> 32;
	sa.ar.position = ar->position & 0xffffffff;
	sa.ar.file_offset = ar->offset;
	sa.ar.arch_flags |= SAR_size_block;
	if (media == DT_DISK) {
		sa.ar.arch_flags |= SAR_diskarch;
	}
	memcpy(sa.ar.vsn, ar->vsn, sizeof (vsn_t));
	if (ioctl(fd, F_SETARCH, &sa) < 0) {
		(void) close(fd);
		return (-10);
	}
	(void) close(fd);
	return (0);
}
