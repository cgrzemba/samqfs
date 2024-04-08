/*
 * id_to_path.c
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

#pragma ident "$Revision: 1.17 $"


#undef MAIN

#include <sys/param.h>
#include <sam/uioctl.h>
#include <sam/fioctl.h>
#include <sam/fs/ino.h>
#include <sam/types.h>
#include "sam/nl_samfs.h"
#include <sam/fs/dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include "sam/lint.h"

static int SAM_fd = -1;
extern int errno;

int sam_closedir(void);
int	sam_opendir(char *);
int id_to_path_(sam_id_t);
int sam_getdents(int, char *, int *, int);
int sam_getdent(struct sam_dirent **);

static union sam_di_ino inode;
char namebuf[MAXPATHLEN];

/*
 *  Given an inode id structure (inum,generation number), and the mount
 *  point, return a pathname to that inode.  If an error occurs, the
 *  "pathname" returned will be the string "Cannot find pathname for ..."
 *  as shown below.
 */
char *
id_to_path(
	char *fsname,
	sam_id_t id)
{
	/*
	 * We've handily been given the mount point
	 * name which keeps us from figuring it out.
	 */
	strcpy(namebuf, fsname);

	SAM_fd = open(fsname, O_RDONLY);
	if (SAM_fd >= 0) {
		if (id_to_path_(id) > 0) {
			close(SAM_fd);
			SAM_fd = -1;
			sam_closedir();
			return (&namebuf[0]);
		}
		close(SAM_fd);
	}
	SAM_fd = -1;
	sam_closedir();
	sprintf(namebuf, catgets(catfd, SET, 584,
	    "Cannot find pathname for filesystem %s inum/gen %lu/%ld\n"),
	    fsname, id.ino, id.gen);
	return (&namebuf[0]);
}

struct sam_ioctl_idstat idstat;
static sam_id_t parent_id;
struct sam_dirent *dirent;

/*
 * Recursive helper routine for id_to_path.  Not directly callable.
 */
static int
id_to_path_(
	sam_id_t id)
{
	int parent_strlen;
	int fd;

	/*
	 * Terminate recursion when we are asked to find inum == 2.
	 * This indicates the top of the sam-fs tree.
	 */
	if (id.ino == 2 || id.ino == 0) {
		return (strlen(namebuf) - 1);
	}
	/* idstat the id, getting the inode in question */

	idstat.id = id;
	idstat.size = sizeof (struct sam_perm_inode);
	idstat.dp = (void *)&inode;
	if (ioctl(SAM_fd, F_IDSTAT, &idstat) < 0) {
		return (0);
	}
	/* next, let's get that parent's pathname (recursively) */

	parent_id = inode.inode.di.parent_id;
	if ((parent_strlen = id_to_path_(parent_id)) == 0) {
		return (0);
	}

	/*
	 * now, open the parent (it should be a directory), and look for an
	 * entry which matches the id we're looking for
	 */
	if ((fd = sam_opendir(namebuf)) < 0) {
		return (0);
	}
	while (sam_getdent(&dirent) > 0) {
		if (dirent->d_id.ino == id.ino &&
		    dirent->d_id.gen == id.gen) {
			namebuf[parent_strlen + 1] = '/';
			strcpy(&namebuf[parent_strlen + 2], dirent->d_name);
			return (parent_strlen + strlen(dirent->d_name) + 1);
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

	memset(dirent, 0, dirent_size);
	getdent.dir = (struct sam_dirent *)dirent;
	getdent.size = dirent_size;
	getdent.offset = *offset;

	ngot = ioctl(dir_fd, F_GETDENTS, &getdent);
	*offset = getdent.offset;

	return (ngot);
}


static n_valid = 0;		/* dirbuf[0..n_valid] are valid info */
static offset = 0;		/* current offset in directory */
static int dir_fd = -1;		/* the fd on which the directory is open */

#define	DIRBUF_SIZE 10240
static char *dirbuf = NULL;
static char *saved_dir_name;

static int
sam_closedir(void)
{
	if (dir_fd >= 0) {
		int stat = close(dir_fd);
		dir_fd = -1;
		return (stat);
	}
	return (0);
}

static int
sam_opendir(
	char *dir_name)
{
	sam_closedir();			/* in case it's still open */

	dir_fd = open(dir_name, O_RDONLY);
	offset = 0;		/* flag sam_getdents to start at beginning */
	n_valid = 0;		/* flag sam_getdent to read up buffer */

	saved_dir_name = dir_name;
	return (dir_fd);
}

static struct sam_dirent *current;

static int
sam_getdent(
	struct sam_dirent ** dirent)
{
	int size;

	if (dirbuf == NULL) {
		n_valid = 0;
		offset = 0;
		SamMalloc(dirbuf, DIRBUF_SIZE);
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
			current = (struct sam_dirent *)& dirbuf[0];
		}
		*dirent = current;
		size = SAM_DIRSIZ(*dirent);
		current = (struct sam_dirent *)((char *)current + size);

		if ((char *)current >= &dirbuf[n_valid]) {
			n_valid = 0;
		}
	} while ((*dirent)->d_fmt == 0);
	return (size);			/* valid */

}

/*	#define	MAIN iff you want this test wrapper compiled. */
#ifdef MAIN
main(int argc, char **argv)
{
	sam_id_t id;
	char *name;
	CustmsgInit(0, NULL);
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
