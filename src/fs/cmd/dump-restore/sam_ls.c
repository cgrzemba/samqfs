/*
 * sam_ls.c  - List file information.
 *
 * Sam_ls lists file information in either: a name-only format; a
 * one line format similar to that produced by ls(1); or in
 * a two line format similar to that produced by samls(1).
 *
 * On entry:
 *	   path = Path/file name of file to list.
 *	   st  = Pointer to extended status.
 *	   link = Pointer to symblic link string.
 *	   data = Pointer to resource record for resource files or
 *			  segment access information.
 *
 *	   ls_options = List options mask:
 *			= LS_FILE:	 Print file name only.
 *			  LS_LINE1:  Print 1 line format.
 *			  LS_LINE2:  Print 2 line format.
 *			  LS_INODES: Print inode numbers.
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

#pragma ident "$Revision: 1.9 $"


#include <limits.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include "sam/types.h"
#include "sam/sam_malloc.h"
#include "sam/fs/ino.h"
#include "sam/resource.h"
#include "sam/lib.h"
#include "pub/lib.h"
#include "pub/rminfo.h"
#include "sam_ls.h"
#include <sam/fs/dirent.h>
#include <sam/fs/bswap.h>
#include "sam/nl_samfs.h"
#include "csd_defs.h"
#include <sys/acl.h>


static char *_SrcFile = __FILE__;


void
cs_list(
	int fcount,
	char **flist)
{
	struct	sam_perm_inode perm_inode;
	struct	sam_vsn_section *vsnp;
	csd_fhdr_t file_hdr;
	char	name[MAXPATHLEN];
	char	linkname[MAXPATHLEN];
	int		namelen;
	void	*data;
	int		n_acls = 0;
	int		skipping = 0;
	aclent_t *aclp = NULL;

	while (csd_read_header(&file_hdr) > 0) {
		namelen = file_hdr.namelen;
		csd_read(name, namelen, &perm_inode);
		data = NULL;
		if (S_ISREQ(perm_inode.di.mode)) {
			SamMalloc(data, perm_inode.di.psize.rmfile);
		}

		csd_read_next(&perm_inode, &vsnp, linkname, data,
		    &n_acls, &aclp);

		/*
		 * select subset of inodes to list.  First, weed out by
		 * inode number range, then by path name
		 */

		if (list_by_inode &&
		    !(lower_inode <= perm_inode.di.id.ino &&
		    perm_inode.di.id.ino  <= upper_inode)) {
			goto skip_file;
		}

		if (fcount != 0) {		/* Search the file name list */
			int i;

			for (i = 0; i < fcount; i++) {
				/*
				 * filecmp returns:
				 *	0 for no match,
				 *	1 exact match,
				 *	2 name subpath of flist[i],
				 *	3 flist[i] subpath of name.
				 */
				if (filecmp(name, flist[i]) != 0) {
					break;
				}
			}
			if (i >= fcount) {
				if (S_ISSEGI(&perm_inode.di)) {
					skipping = 1;
					goto skip_seg_file;
				}
				goto skip_file;
			}
		}

		if (S_ISSEGI(&perm_inode.di)) {
			BUMP_STAT(files);
		} else if (S_ISREG(perm_inode.di.mode)) {
			BUMP_STAT(files);
			if (perm_inode.di.arch_status == 0 &&
			    perm_inode.di.rm.size != 0) {
				BUMP_STAT(file_damaged);
			}
		} else if (S_ISLNK(perm_inode.di.mode)) {
			BUMP_STAT(links);
		} else if (S_ISDIR(perm_inode.di.mode)) {
			BUMP_STAT(dirs);
		} else if (S_ISREQ(perm_inode.di.mode)) {
			BUMP_STAT(resources);
		}

		sam_ls(name, &perm_inode, NULL);
		sam_db_list(name, &perm_inode, linkname, vsnp);

		/* If not already offline */
		if (!perm_inode.di.status.b.offline) {
			if (perm_inode.di.arch_status) { /* If archived */
				print_reslog(log_st, name, &perm_inode,
				    "online");
			}
		} else if (perm_inode.di.status.b.pextents) {
			print_reslog(log_st, name, &perm_inode, "partial");
		}

skip_seg_file:

		if (S_ISSEGI(&perm_inode.di)) {
			struct sam_perm_inode seg_inode;
			int i;
			offset_t seg_size;
			int no_seg;

			seg_size =
			    (offset_t)perm_inode.di.rm.info.dk.seg_size *
			    SAM_MIN_SEGMENT_SIZE;
			no_seg = (perm_inode.di.rm.size + seg_size - 1) /
			    seg_size;
			for (i = 0; i < no_seg; i++) {
				struct sam_vsn_section *seg_vsnp;

				readcheck(&seg_inode, sizeof (seg_inode),
				    5002);
				if (swapped) {
					sam_byte_swap(
					    sam_perm_inode_swap_descriptor,
					    &seg_inode, sizeof (seg_inode));
				}
				if (!(SAM_CHECK_INODE_VERSION(
				    seg_inode.di.version))) {
					if (debugging) {
						fprintf(stderr,
						    "sam_ls: seg %d inode "
						    "version %d\n",
						    i, seg_inode.di.version);
					}
					error(0, 0, catgets(catfd, SET, 739,
					    "%s: inode version incorrect - "
					    "skipping"),
					    name);
					dont_process_this_entry = 1;
					continue;
				}
				seg_vsnp = NULL;
				csd_read_mve(&seg_inode, &seg_vsnp);
				if (!skipping) {
					sam_ls(name, &seg_inode, NULL);
					sam_db_list(name, &seg_inode, NULL,
					    seg_vsnp);
				}
				if (seg_vsnp) {
					SamFree(seg_vsnp);
				}
			}
		}
		skipping = 0;

skip_file:
		if (data) {
			SamFree(data);
			data = NULL;
		}
		if (vsnp) {
			SamFree(vsnp);
			vsnp = NULL;
		}
		if (aclp) {
			SamFree(aclp);
			aclp = NULL;
		}
		if (file_hdr.flags & CSD_FH_DATA) {
			skip_embedded_file_data();
		}
	}
}


void
sam_ls(
	char *path,				/* Pathname for file */
	struct sam_perm_inode *perm_inode,	/* Sam inode */
	char *link)				/* Symbolic link */
{
	char	tempbuf[40];
	int		copy;
	offset_t	ii;
	time_t	ls_base_time;	/* Current time */

	if (scan_only) {
		return;
	}

	ls_base_time = time(NULL);	/* Current time */
	if (!(ls_options & (LS_LINE1 | LS_LINE2))) {	/* 0 */
		if (ls_options & LS_INODES) {
			printf("%6u ", perm_inode->di.id.ino);
		}
		printf("%s\n", path);
		return;
	}
	if (ls_options & LS_INODES) {					/* 1 */
		printf("%6u ", perm_inode->di.id.ino);
	}
	printf("%s %3u ", mode_string(perm_inode->di.mode, tempbuf),
	    perm_inode->di.nlink);
	printf("%-8.8s ", getuser(perm_inode->di.uid));
	printf("%-8.8s ", getgroup(perm_inode->di.gid));

	if (S_ISCHR(perm_inode->di.mode) || S_ISBLK(perm_inode->di.mode)) {
		printf("%3u, %3u ", 0, 0);	/* unsupported in sam */
	} else {
		ii = perm_inode->di.rm.size;
		printf("%9llu ", ii);
	}

	printf("%s ", time_string(perm_inode->di.access_time.tv_sec,
	    ls_base_time,
	    tempbuf));

	if (S_ISSEGS(&perm_inode->di)) {
		printf("%s/%d", path, perm_inode->di.rm.info.dk.seg.ord + 1);
	} else {
		printf("%s", path);
	}

	if (S_ISLNK(perm_inode->di.mode) && link != NULL) {
		printf(" -> ");
		if (!(ls_options & LS_LINE2)) {
			printf("%s", link);
		}
	}
	printf("\n");

	if (!(ls_options & LS_LINE2)) {
		return;
	}


	if (ls_options & LS_INODES) {					/* 2 */
		printf("       ");
	}

	printf("%s ", sam_attrtoa(perm_inode->di.status.bits, NULL));
	for (copy = 0; copy < MAX_ARCHIVE; copy++) {
		if (!(perm_inode->di.arch_status & (1<<copy)))  {
			printf("   ");
		} else {
			printf("%2s ",
			    sam_mediatoa(perm_inode->di.media[copy]));
		}
	}

	printf("\n");
}
