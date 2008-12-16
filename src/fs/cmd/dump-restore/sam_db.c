/*
 * sam_db.c  - Generate load file for SAMdb.
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

#pragma ident "$Revision: 1.4 $"

#include <errno.h>
#include <limits.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/vfs.h>
#include "sam/types.h"
#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "sam/sam_malloc.h"
#include "sam/fs/ino.h"
#include "sam/fs/dirent.h"
#include "sam/fs/sblk.h"
#include "sam/resource.h"
#include "csd_defs.h"

#define	MAX_FILE_LENGTH 500

static char *_SrcFile = __FILE__;

/*
 * Sam_db generates to the specified file a load file that can be used
 * by the SAM database utilities.
 *
 * On entry:
 *	   path = Path/file name of file to list.
 *	   perm_inode = Inode information.
 *	   link = Pointer to symblic link string.
 *	   vsnp = Extended VSN list for multi-VSN archives.
 */
void
sam_db_list(
	char *path,				/* Pathname for file */
	struct sam_perm_inode *perm_inode,	/* Sam inode */
	char *link,				/* Symbolic link */
	sam_vsn_section_t *vsnp)		/* Multi-volume VSN list */
{
	char	obj[MAX_FILE_LENGTH]; /* File name object buffer */
	int		copy;		/* Archive copy			*/
	int		ftype;		/* File type			*/
	int		seq;		/* VSN sequence number		*/
	int		n_vsns;		/* Number VSNs in archive set	*/
	int		vn;		/* VSN index into vsnp		*/
	char		*p, *pp;

	if (DB_FILE == NULL) {
		return;
	}

	if (perm_inode->di.id.ino < SAM_MIN_USER_INO) {
		return;
	}

	ftype = 5;
	if (S_ISREG(perm_inode->di.mode)) {
		ftype = 0;
	}
	if (S_ISDIR(perm_inode->di.mode)) {
		ftype = 1;
	}
	if (S_ISSEGS(&perm_inode->di)) {
		ftype = 2;
	}
	if (S_ISSEGI(&perm_inode->di)) {
		ftype = 3;
	}
	if (S_ISLNK(perm_inode->di.mode)) {
		ftype = 4;
	}

	p = path;
	pp  = strrchr(p, '/');
	if (pp == p && *(p+1) == '\0') {
		return; /* path is / only	*/
	}

	obj[0] = '\0';

	if (S_ISSEGS(&perm_inode->di)) {
		sprintf(obj, "%d", perm_inode->di.rm.info.dk.seg.ord + 1);
	} else if (pp == NULL) {
		/* File is in the root directory with relative path */
		strncpy(obj, p, MAX_FILE_LENGTH);
		p = ".";
	} else {
		strncpy(obj, pp+1, MAX_FILE_LENGTH);
		*pp = '\0';
	}

	/* Print inode line */
	fprintf(DB_FILE, "%u\t%u\t%d\t%llu\t",
	    perm_inode->di.id.ino,
	    perm_inode->di.id.gen,
	    ftype,
	    (unsigned long long) perm_inode->di.rm.size);

	if (perm_inode->di.status.b.cs_val) {
		fprintf(DB_FILE, "%08x%08x%08x%08x\t",
		    perm_inode->csum.csum_val[0],
		    perm_inode->csum.csum_val[1],
		    perm_inode->csum.csum_val[2],
		    perm_inode->csum.csum_val[3]);
	} else {
		fprintf(DB_FILE, "0\t");
	}

	fprintf(DB_FILE, "%u\t%u\t%lu\t%lu\t%c\n",
	    perm_inode->di.creation_time,
	    perm_inode->di.modify_time.tv_sec,
	    perm_inode->di.uid,
	    perm_inode->di.gid,
	    perm_inode->di.status.b.offline ? '0' : '1');

	/* Print file line */
	fprintf(DB_FILE, "%s\t%s\n", p, obj);

	if (pp != NULL) {
		*pp = '/';
	}

	if (ftype == 4) {		/* If symbolic link	*/
		fprintf(DB_FILE, "%s\n", link != NULL?link:"");
	}

	/* Print archive lines */
	for (copy = vn = 0; copy < MAX_ARCHIVE; copy++) {
		int64_t position;
		if (!(perm_inode->di.arch_status & (1<<copy))) {
			continue;
		}

		n_vsns = perm_inode->ar.image[copy].n_vsns;

		for (seq = 0; seq < perm_inode->ar.image[copy].n_vsns; seq++) {
			char stale = perm_inode->di.ar_flags[copy] &
			    AR_stale ? '1' : '0';
			position = n_vsns > 1 ? vsnp[vn].position :
			    (((int64_t)
			    perm_inode->ar.image[copy].position_u) << 32) |
			    perm_inode->ar.image[copy].position;

			fprintf(DB_FILE,
			    "%d\t%d\t%s\t%s\t%llu\t%llu\t%llu\t%u\t%c\n",
			    copy,
			    seq,
			    sam_mediatoa(perm_inode->di.media[copy]),
			    n_vsns > 1 ? vsnp[vn].vsn :
			    perm_inode->ar.image[copy].vsn,
			    position,
			    n_vsns > 1 ? vsnp[vn].offset :
			    (uint64_t)perm_inode->ar.image[copy].file_offset,
			    n_vsns > 1 ? (unsigned long long) vsnp[vn].length :
			    (unsigned long long) perm_inode->di.rm.size,
			    perm_inode->ar.image[copy].creation_time,
			    stale);
			if (n_vsns > 1) {
				vn++;
			}
		}
	}

	fprintf(DB_FILE, "/EOS\n");
}
