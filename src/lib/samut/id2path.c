/*
 * id2path - convert SAM file id number to a path.
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

#pragma ident "$Revision: 1.18 $"


/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

/* Solaris headers. */
#include <libgen.h>
#include <sys/param.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/param.h"
#include "sam/fs/dirent.h"
#include "sam/lib.h"
#include "sam/fioctl.h"
#include "sam/uioctl.h"
#include "sam/nl_samfs.h"
#include "sam/fs/ino.h"

/* Local headers. */

/* Macros. */
#define	DIRBUF_SIZE 10240

/* Prototypes. */
void *malloc_wait(const size_t, const uint_t, const int);

/* Private functions. */
static int sam_closedir(void);
static int sam_getdents(int dir_fd, char *dirent, int *offset, int dirent_size);
static int id_to_path_(sam_id_t id);

/* Private data. */
static union sam_di_ino inode;
static char fullpath[MAXPATHLEN + 4];
static int fs_fd = -1;
static int n_valid = 0;		/* dirbuf[0..n_valid] are valid info */
static int offset = 0;		/* current offset in directory */
static int dir_fd = -1;		/* the fd on which the directory is open */
static char *dirbuf = NULL;


/*
 * Given an inode id structure (inum,generation number), and the mount
 * point, return a pathname to that inode.  If an error occurs, the
 * "pathname" returned will be the string "Cannot find pathname for ..."
 * as shown below.
 */
char *
id_to_path(
	char *fsname,
	sam_id_t id)
{
	char *msg_buf, *msg1;

	/*
	 * We've been given the mount point name which keeps
	 * us from figuring it out.
	 */
	strcpy(fullpath, fsname);
	fs_fd = open(fsname, O_RDONLY);
	if (fs_fd >= 0) {
		if (id_to_path_(id) > 0) {
			close(fs_fd);
			fs_fd = -1;
			(void) sam_closedir();
			return (&fullpath[0]);
		}
		close(fs_fd);
	}
	fs_fd = -1;
	(void) sam_closedir();
	msg1 = catgets(catfd, SET, 584,
	    "Cannot find pathname for filesystem %s inum/gen %lu/%ld\n");
	msg_buf = malloc_wait(strlen(msg1)+strlen(fsname) + 20, 2, 0);
	sprintf(msg_buf, msg1, fsname, id.ino, id.gen);
	memccpy(fullpath, msg_buf, '\0', MAXPATHLEN);
	free(msg_buf);
	return (&fullpath[0]);
}


/*
 * Recursive helper routine for id_to_path.  Not directly callable.
 */
static int 		/* return value = name buffer length, or zero */
id_to_path_(
	sam_id_t id)
{
	struct sam_ioctl_idstat idstat;
	struct sam_dirent *dirent;
	sam_id_t parent_id;
	int parent_strlen;

	/*
	 * Terminate recursion when we are asked to find inum == 2.
	 * This indicates the top of the sam-fs tree.
	 */
	if (id.ino == 2 || id.ino == 0) {
		return (strlen(fullpath) - 1);
	}

	/* idstat the id, getting the inode in question */

	idstat.id = id;
	idstat.size = sizeof (struct sam_perm_inode);
	idstat.dp.ptr = (void *)&inode;
	if (ioctl(fs_fd, F_IDSTAT, &idstat) < 0) {
		return (0);
	}

	/* Jump back over the segment inode to the base segment inode */

	if (inode.inode.di.status.b.seg_ino) {
		id = inode.inode.di.parent_id;
		idstat.id = id;
		idstat.size = sizeof (struct sam_perm_inode);
		idstat.dp.ptr = (void *)&inode;
		if (ioctl(fs_fd, F_IDSTAT, &idstat) < 0) {
			return (0);
		}
	}

	/* next, let's get that parent's pathname (recursively) */

	parent_id = inode.inode.di.parent_id;
	if ((parent_strlen = id_to_path_(parent_id)) == 0) {
		return (0);
	}
	/*
	 * Open the parent(it should be a directory) and look for an
	 * entry which matches the id we're looking for.
	 */

	if (sam_opendir(fullpath) < 0) {
		return (0);
	}
	while (sam_getdent(&dirent) > 0) {
		if (dirent->d_id.ino == id.ino && dirent->d_id.gen == id.gen) {
			fullpath[parent_strlen + 1] = '/';
			strcpy(&fullpath[parent_strlen + 2],
			    (const char *)dirent->d_name);
			return (parent_strlen +
			    strlen((const char *)dirent->d_name) + 1);
		}
	}
	return (0);
}


static int
sam_getdents(
	int dir_fd,		/* fd on which the directory is open */
	char *dirent,		/* returned directory entry */
	int *offset,		/* current offset into directory */
	int dirent_size)	/* size of dirent */
{
	sam_ioctl_getdents_t getdent;
	int ngot;

	(void) memset(dirent, 0, dirent_size);
/* LINTED pointer cast may result in improper alignment */
	getdent.dir.ptr = (struct sam_dirent *)dirent;
	getdent.size = dirent_size;
	getdent.offset = *offset;

	ngot = ioctl(dir_fd, F_GETDENTS, &getdent);
	*offset = (int)getdent.offset;

	return (ngot);
}


static int
sam_closedir(void)
{
	if (dir_fd >= 0) {
		int stat;

		stat = close(dir_fd);
		dir_fd = -1;
		return (stat);
	}
	return (0);
}

int
sam_opendir(
	char *dir_name)
{
	(void) sam_closedir();	/* in case it's still open */

	dir_fd = open(dir_name, O_RDONLY);
	offset = 0;		/* flag sam_getdents to start at beginning */
	n_valid = 0;		/* flag sam_getdent to read up buffer */

	return (dir_fd);
}


int
sam_getdent(
	struct sam_dirent **dirent)
{
	static struct sam_dirent *current;
	int size;

	if (dirbuf == NULL) {
		n_valid = 0;
		offset = 0;
		dirbuf = (char *)malloc(DIRBUF_SIZE);
		if (dirbuf == NULL) {
			return (-1);
		}
	}
	do {
		if (n_valid == 0) {
			n_valid = sam_getdents(dir_fd, dirbuf, &offset,
			    DIRBUF_SIZE);
			if (n_valid < 0) {
				return (-1);	/* Error */
			} else if (n_valid == 0) {
				return (0);	/* EOF */
			}
/* LINTED pointer cast may result in improper alignment */
			current = (struct sam_dirent *)& dirbuf[0];
		}
		*dirent = current;
		size = SAM_DIRSIZ(*dirent);
/* LINTED pointer cast may result in improper alignment */
		current = (struct sam_dirent *)((char *)current + size);

		if ((char *)current >= &dirbuf[n_valid]) {
			n_valid = 0;
		}
	} while ((*dirent)->d_fmt == 0);
	return (size);			/* valid */
}

/* #define MAIN iff you want this test wrapper compiled. */
#ifdef MAIN
main(int argc, char **argv)
{
	sam_id_t id;
	char *name;

	if (argc != 4) {
		printf("Usage: %s mount_point inumber generation\n");
		exit(1);
	}
	id.ino = atoi(argv[2]);
	id.gen = atoi(argv[3]);

	name = id_to_path(argv[1], id);
	printf("%s\n", name ? name :
	    catgets(catfd, SET, 741, "Could not determine path"));

}
#endif
