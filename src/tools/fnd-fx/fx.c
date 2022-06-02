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

/*
 * This program repairs fields in currently existing, findable-with-a-pathname
 * inodes.  The program must be modified for each set of fields which is to
 * be modified.  The the general format of the input file is:
 *      pathname copy value1 value2 value3...
 * where pathname must be a relative pathname to the directory in which this
 * program was run.  The directory in which this program is run must be a
 * SAM-FS filesystem, and all the pathnames presented must be in that particular
 * filesystem.
 *
 * The flow of this program is very simple:  while there are the correct
 * number of fields read from standard in, translate the pathname into
 * an (inode,generation) pair, get all the inode information as if doing a
 * samfsdump on that file, modify the inode contents according to the
 * values presented, then restore the file as if in a samfsrestore u.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <sys/param.h>

#define DEC_INIT
#include "sam/types.h"
#include "sam/fs/ino.h"
#include "sam/fs/dirent.h"
#include "pub/stat.h"
#include "sam/uioctl.h"
#include "sam/lib.h"

#include "csd.h"

int errno;
char *error_string(int errno);
char *program_name = NULL;

int
main(int argc, char **argv)
{
#ifdef DONT_EVER_DEFINE
	/* This is an extract from ino.h: */
	long		position_u;	/* Pos'n of archive file on media - */
	long		position;	/*   media dependent */
	/* Offset of start of file in archive file - */
	ulong_t		file_offset;
	/*
	 * But I'm almost sure that they really wanted unsigned long for
	 * position's.
	 */
#endif
	char name[MAXPATHLEN];
	int copy;
	int sam_fd;
	long position_u, position;
	struct sam_stat sbuf;
	unsigned long long offset;

	sam_fd = open(".", O_RDONLY);
	if (sam_fd < 0) {
		printf("Cannot open current directory: %s\n",
		    error_string(errno));
		exit(1);
	}

	while (scanf("%s %i %li %li %lli",
	    name, &copy, &position_u, &position, &offset) == 5) {
		/*
		 * I'm assuming that we won't be doing this very often.  More
		 * efficient for a big version of this program might be to do
		 * our own pathname-to-inode conversion.  Maybe there's a
		 * version of sam_stat that can return the whole inode so it
		 * can be passed to sam_restore_a_file, but I'm not sure.
		 * Anyway, this sequence of "sam_lstat" to get the inode and
		 * generation numbers, then using IDSTAT to get the inode
		 * data and VSN information works but is slow.
		 */
		if (sam_lstat(name, &sbuf, sizeof (sbuf)) == 0) {
			struct sam_ioctl_idstat idstat;
			struct sam_perm_inode perm_inode;
			struct sam_perm_inode_v1 *perm_inode_v1;
			struct sam_vsn_section *vsnp = NULL;
			int copy, n_vsns;

			idstat.id.gen = sbuf.gen;
			idstat.id.ino = sbuf.st_ino;
			idstat.size = sizeof (perm_inode);
			idstat.dp.ptr = (void *)&perm_inode;

			if (ioctl(sam_fd, F_IDSTAT, &idstat) < 0) {
				printf("Cannot IDSTAT: %s\n", name);
				exit(1);
			}
			if (!S_ISREG(perm_inode.di.mode)) {
				printf("Skipping non-regular file: %s\n",
				    name);
				exit(1);
			}


	/* N.B. Bad indentation here to meet cstyle requirements */

	/*
	 * Get multivolume vsns for all 4 possible copies if
	 * any exist.
	 */
	n_vsns = 0;
	for (copy = 0; copy < MAX_ARCHIVE; copy++) {
		if (perm_inode.di.version >= SAM_INODE_VERS_2) {
			/* Curr version */
			if (perm_inode.ar.image[copy].n_vsns > 1) {
				n_vsns += perm_inode.ar.image[copy].n_vsns;
			}
		} else if (perm_inode.di.version == SAM_INODE_VERS_1) {
			/* Prev version */
			perm_inode_v1 =
			    (struct sam_perm_inode_v1 *)&perm_inode;
			if (perm_inode_v1->aid[copy].ino != 0) {
				if (perm_inode.ar.image[copy].n_vsns > 1) {
					n_vsns +=
					    perm_inode.ar.image[copy].n_vsns;
				} else {
					perm_inode_v1->aid[copy].ino = 0;
					perm_inode_v1->aid[copy].gen = 0;
				}
			}
		}
	}

	if (n_vsns) {
		struct sam_ioctl_idmva idmva;

		vsnp = (struct sam_vsn_section *)
		    malloc(sizeof (struct sam_vsn_section) *
		    n_vsns);
		idmva.id.gen = sbuf.gen;
		idmva.id.ino = sbuf.st_ino;
		for (copy = 0; copy < MAX_ARCHIVE; copy++) {
			if (perm_inode.di.version >=
			    SAM_INODE_VERS_2) {
				idmva.aid[copy].ino =
				    idmva.aid[copy].gen = 0;
			} else if (perm_inode.di.version ==
			    SAM_INODE_VERS_1) {
				perm_inode_v1 =
				    (struct sam_perm_inode_v1 *)&perm_inode;
				idmva.aid[copy] =
				    perm_inode_v1->aid[copy];
			}
		}
		idmva.size =
		    sizeof (struct sam_vsn_section) * n_vsns;
		idmva.buf.ptr = (void *)vsnp;

		if (ioctl(sam_fd, F_IDMVA, &idmva) < 0) {
			printf("Cannot IDMVA: %s\n", name);
			exit(1);
		}
	}

			/*
			 * Now that we've gotten the entire inode for the
			 * file in question, let's change the fields to the
			 * values we were given in the scanf input.
			 */
			printf("Name %s Copy %d Position %li %li "
			    "Offset %lli\n",
			    name, copy, position_u, position,
			    offset);

			perm_inode.ar.image[copy-1].arch_flags |=
			    SAR_size_block;
			perm_inode.ar.image[copy-1].file_offset = offset;
			perm_inode.ar.image[copy-1].position = position;
			perm_inode.ar.image[copy-1].position_u = position_u;

			/*
			 * This function will re-create the inode, then set
			 * all the fields back to the values we pass.
			 */
			sam_restore_a_file(name, &perm_inode, vsnp,
			    (char *)NULL,
			    (struct sam_resource_file *)NULL);
			if (vsnp)  free(vsnp);
		} else {
			printf("Could not sam_lstat(%s): %s\n", name,
			    error_string(errno));
		}
	}
	return (0);
}

/*
 *  Return the ascii-string representation of the errno.
 */
char *
error_string(int errno)
{
	char *v = (char *)strerror(errno);

	if (v) {
		return (v);
	}
	return ("(NULL)");
}
