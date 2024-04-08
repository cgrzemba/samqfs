/*
 *  sam_restore.c
 *
 *  Work routines for csd restore.
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

#pragma ident "$Revision: 1.20 $"


#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sam/fs/ino.h>
#include <errno.h>
#include <string.h>
#include <pub/rminfo.h>
#include <sam/lib.h>
#include <sam/resource.h>
#include <sam/fioctl.h>
#include <sam/uioctl.h>
#include <sam/types.h>
#include "csd.h"
#include <sam/fs/sblk.h>
#include "sam/nl_samfs.h"

#define	catgets(a, b, c, d) d


/*
 * ----- sam_restore_a_file -  CSD restore.
 *	Restore a single file to the filesystem from a csd image.
 */


void
sam_restore_a_file(
	char			*path,		/* path name of the file */
	struct sam_perm_inode	*perm_inode,
	struct sam_vsn_section	*vsns,
	char			*link,		/* symlink resolution */
	struct sam_resource_file *resource)	/* resource file info */
{
	struct sam_ioctl_idrestore idrestore = { NULL, NULL, NULL, NULL };
	char *basename = strrchr(path, '/');
	int entity_fd;
	int copy;

	/*	don't do much for "..".  */
	if (basename != NULL) {
		if ((strcmp(basename, "/..") == 0)) {
			return;
		} else if (strcmp(basename, "/.") == 0) {
			return;
		}
	}

	if (1) {  /* was "replace" */
		if (S_ISDIR(perm_inode->di.mode)) {
			rmdir(path);
		} else {
			unlink(path);
		}
	}

	/*
	 * Create the subject entity.  It might be one of a number of
	 * different things, each of which must be created slightly
	 * differently.
	 */
	if (S_ISREG(perm_inode->di.mode)) {
		if ((entity_fd = open(path, O_CREAT | O_EXCL,
		    perm_inode->di.mode & 07777)) < 0) {
			BUMP_STAT(errors);
			error(0, errno, catgets(catfd, SET, 215,
			    "%s: Cannot creat()"),
			    path);
			return;
		}
		BUMP_STAT(files);

	} else if (S_ISDIR(perm_inode->di.mode)) {
		if (mkdir(path, perm_inode->di.mode & 07777) < 0) {
			BUMP_STAT(errors);
			error(0, errno, catgets(catfd, SET, 222,
			    "%s: Cannot mkdir()"),
			    path);
			return;
		}
		if ((entity_fd = open(path, O_RDONLY)) < 0) {
			BUMP_STAT(errors);
			error(0, errno, catgets(catfd, SET, 5010,
			    "%s: Cannot open() following mkdir()"),
			    path);
			return;
		}
		BUMP_STAT(dirs);
	} else if (S_ISREQ(perm_inode->di.mode)) {
		if (resource->resource.revision != SAM_RESOURCE_REVISION) {
			BUMP_STAT(errors);
			error(0, 0, catgets(catfd, SET, 5014,
			    "%s: Unsupported revision for removable "
			    "media info"),
			    path);
			return;
		}
		if ((entity_fd = open(path,
		    O_WRONLY | O_CREAT | O_EXCL | O_TRUNC,
		    perm_inode->di.mode & 07777)) < 0) {
			BUMP_STAT(errors);
			error(0, errno,
			    catgets(catfd, SET, 215, "%s: Cannot creat()"),
			    path);
			return;
		}
		idrestore.rp.ptr = (void *)resource;
		BUMP_STAT(resources);
	} else if (S_ISLNK(perm_inode->di.mode)) {
		if ((entity_fd = open(path,
		    O_WRONLY | O_CREAT | O_EXCL | O_TRUNC,
		    perm_inode->di.mode & 07777)) < 0) {
			BUMP_STAT(errors);
			error(0, errno,
			    catgets(catfd, SET, 215, "%s: Cannot creat()"),
			    path);
			return;
		}
		idrestore.lp.ptr = (void *)link;
		BUMP_STAT(links);
	}

	/* Notify user of previously or newly damaged files */
	if (perm_inode->di.status.b.damaged) {
		BUMP_STAT(file_damaged);
		error(0, 0,
		    catgets(catfd, SET, 241,
		    "%s: File was already damaged prior to dump"),
		    path);
	} else {
		if (S_ISREG(perm_inode->di.mode) &&
		    perm_inode->di.arch_status == 0 &&
		    perm_inode->di.rm.size != 0) {
			int copy;
			uint_t *arp = (uint_t *)&perm_inode->di.ar_flags[0];

			/* These stale entries should not be rearchived */
			*arp &= ~((AR_rearch<<24)|(AR_rearch<<16)|
			    (AR_rearch<<8)|AR_rearch);
			/*
			 * If inode has any stale copies, file is still
			 * marked damaged.
			 */
			for (copy = 0; copy < MAX_ARCHIVE; copy++) {
				if ((perm_inode->di.ar_flags[copy] &
				    AR_stale) &&
				    (perm_inode->ar.image[copy].vsn[0] != 0)) {
					perm_inode->di.ar_flags[copy] &=
					    ~AR_rearch;
					error(0, 0, catgets(catfd, SET, 5017,
					    "%s: File is now damaged, "
					    "stale archive copy exists."),
					    path);
					break;
				}
			}
			if (copy >= MAX_ARCHIVE) {
				error(0, 0,
				    catgets(catfd, SET, 240,
				    "%s: File is now damaged."), path);
			}
			BUMP_STAT(file_damaged);
			perm_inode->di.status.b.damaged = 1;
		}
	}

	/*
	 * Pass the old inode to the filesystem; let it copy into the new
	 * inode that part of it which makes sense.
	 */
	idrestore.dp.ptr = perm_inode;
	/* Previous version */
	if (perm_inode->di.version == SAM_INODE_VERS_1) {
		sam_perm_inode_v1_t *perm_inode_v1 =
		    (sam_perm_inode_v1_t *)perm_inode;

		for (copy = 0; copy < MAX_ARCHIVE; copy++) {
			if (perm_inode_v1->aid[copy].ino) {
				idrestore.vp.ptr = (void *)vsns;
				break;
			}
		}
	}
	if (ioctl(entity_fd, F_IDRESTORE, &idrestore) < 0) {
		error(0, errno, "%s: ioctl(F_IDRESTORE)", path);
		BUMP_STAT(errors);
	}
	if (close(entity_fd) < 0) {
		error(0, errno, "%s: close() ", path);
		BUMP_STAT(errors);
	}

}
