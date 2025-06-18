/*
 *	sam_getdent.c - csd create/restore a csd dump file.
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


#pragma ident "$Revision: 1.7 $"


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/dirent.h>
#include <strings.h>
#include <pub/stat.h>
#include <sam/uioctl.h>
#include <unistd.h>
#include <sam/lib.h>
#include <sam/fioctl.h>
#include <sam/fs/dirent.h>
#include <sam/fs/ino.h>
#include "sam/nl_samfs.h"
#include "csd_defs.h"

extern boolean debugging;

#ifndef SAM_MIN_USER_INO
#define SAM_MIN_USER_INO 1025
#endif

static void dump_dirent(int i, struct sam_dirent dirent);


/*
 * ----- sam_getdents
 *
 * Use the ioctl to get a set of directory entries
 */
static int
sam_getdents(
	int dir_fd,		/* fd on which the directory is open */
	char *dirent,		/* returned directory entry */
	int *offset,		/* current offset into directory */
	int dirent_size)	/* size of dirent */
{
	sam_ioctl_getdents_t getdent;
	int ngot;

	getdent.dir.ptr = (struct sam_dirent *)(void *)dirent;
	getdent.size = dirent_size;
	getdent.offset = *offset;

	ngot = ioctl(dir_fd, F_GETDENTS, &getdent);
	*offset = getdent.offset;

	return (ngot);
}


static int n_valid = 0;		/* dirbuf[0..n_valid] are valid info */
static int offset = 0;		/* current offset in directory */
static int dir_fd = -1;		/* the fd on which the directory is open */
static int is_xattr = 0;

#define	DIRBUF_SIZE 10240
static char *dirbuf = NULL;
static char *saved_dir_name;

int
samattropen(char *dir_name)
{
	if (dir_fd >= 0) {
		(void) close(dir_fd);
		is_xattr = 0;
	}

	dir_fd = attropen(dir_name, ".", O_RDONLY);
	offset = 0;		/* flag sam_getdents to start at beginning */
	n_valid = 0;		/* flag sam_getdent to read up buffer */
	is_xattr = 1;

	saved_dir_name = dir_name;
	return (dir_fd);
}

int
samopendir(char *dir_name)
{
	if (dir_fd >= 0) {
		(void) close(dir_fd);
		is_xattr = 0;
	}

	/* no need for SAM_O_LARGEFILE here, directories always < 2GB */
	dir_fd = open(dir_name, O_RDONLY);
	offset = 0;		/* flag sam_getdents to start at beginning */
	n_valid = 0;		/* flag sam_getdent to read up buffer */

	saved_dir_name = dir_name;
	return (dir_fd);
}

static struct sam_dirent *current;

/*
 * ----- sam_getdent
 *
 * Using buffers, get the next entry from the currently open directory
 */

/* for xattr directory use generic getdents, there is no ioctl for getdents */
static int
sam_getdents_gen(struct sam_dirent *dirent) {
	int size;
	struct dirent *direntbuf;

	direntbuf = (struct dirent *)malloc(DIRBUF_SIZE);
	if (direntbuf == NULL) {
		BUMP_STAT(errors);
		BUMP_STAT(errors_dir);
		error(0, errno,
		    catgets(catfd, SET, 605,
		    "Cannot malloc space for reading directory %s"),
		    saved_dir_name);
	}
	bzero((void*)dirent, DIRBUF_SIZE);
	size = getdents(dir_fd, direntbuf, DIRBUF_SIZE);
	if (size > 0) {
		struct dirent *dp = direntbuf;
		struct sam_dirent *sdp = dirent;
		uint16_t namlen;
		int n_valid = 0;

		while(dp->d_reclen > 0 && dp < direntbuf+size) {
			if (dp->d_ino >= SAM_MIN_USER_INO) { /* skip system inodes */
				sdp->d_fmt = 0x4000;
				namlen = strlen(dp->d_name);
				sdp->d_namlen = namlen;
				sdp->d_reclen = SAM_DIRSIZ(sdp);
				n_valid += sdp->d_reclen;
				sdp->d_id.ino = dp->d_ino;
				bcopy(dp->d_name, sdp->d_name, namlen+1);
			}
			sdp = (struct sam_dirent *)(((char*) sdp) + sdp->d_reclen);
			dp = (struct dirent *)(((char*) dp) + dp->d_reclen);
		}
		free(direntbuf);
		return (n_valid);
	} else if (size == 0) {
		free(direntbuf);
		return (0);
	}
	free(direntbuf);
	BUMP_STAT(errors);
	BUMP_STAT(errors_dir);
	error(0, errno,
	    catgets(catfd, SET, 226,
	    "%s: Cannot read xattr directory"),
	    saved_dir_name);
	return (-1);	/* Error */
}

int
sam_getdent(struct sam_dirent **dirent)
{
	int size;


	if (dirbuf == NULL) {
		n_valid = 0;
		offset = 0;
		dirbuf = (char *)malloc(DIRBUF_SIZE);
		bzero(dirbuf, DIRBUF_SIZE);
		if (dirbuf == NULL) {
			BUMP_STAT(errors);
			BUMP_STAT(errors_dir);
			error(0, errno,
			    catgets(catfd, SET, 605,
			    "Cannot malloc space for reading directory %s"),
			    saved_dir_name);
		}
	}
	do {
		if (n_valid == 0) {
			if (is_xattr)
				n_valid = sam_getdents_gen((struct sam_dirent *)(void *)
				    &dirbuf[0]);
			else
				n_valid = sam_getdents(dir_fd, dirbuf, &offset,
				    DIRBUF_SIZE);
			if (n_valid < 0) {
				BUMP_STAT(errors);
				BUMP_STAT(errors_dir);
				error(0, errno,
				    catgets(catfd, SET, 226,
				    "%s: Cannot read directory"),
				    saved_dir_name);
				return (-1);	/* Error */
			} else if (n_valid == 0) {
				return (0);	/* EOF */
			}
			current = (struct sam_dirent *)(void *)&dirbuf[0];
		}
		*dirent = current;

		if (debugging) {
			dump_dirent(Ptrdiff(current, &dirbuf[0]), **dirent);
		}

		size = SAM_DIRSIZ(*dirent);
		current = (struct sam_dirent *)(void *)((char *)current +
		    size);

		if ((char *)current >= &dirbuf[n_valid]) {
			n_valid = 0;
		}
	} while ((*dirent)->d_fmt == 0);

	return (size);			/* valid */
}


/*
 * ----- dump_dirent
 *
 * a debugging function
 */
static void
dump_dirent(
	int i,
	struct sam_dirent dirent)
{
	fprintf(stderr, "(%d) fmt %o reclen %d ino %d gen %d namlen %d\n", i,
	    dirent.d_fmt, dirent.d_reclen, dirent.d_id.ino, dirent.d_id.gen,
	    dirent.d_namlen);
}
