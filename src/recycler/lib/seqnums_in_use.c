/*
 * seqnums_in_use.c
 *
 * Definition of functions which serve for work with disk archive
 * sequence numbers that are in use by currently running arcopy processes.
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

#pragma ident "$Revision$"

/* System headers related to this file only */
#include <dirent.h>

/* Common recycler headers */
#include "recycler_c.h"

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/archset.h"
#include "aml/archiver.h"
#include "aml/archreq.h"
#include "aml/diskvols.h"
#include "sam/sam_trace.h"
#include "sam/sam_malloc.h"
#include "sam/lib.h"



/* Library related headers */
#include "recycler_lib.h"

static char *_SrcFile = __FILE__;

/*
 * Returns the array of sequence numbers in use.
 * The array is allocated in this function. Caller is responsible for freeing
 * the array when no longer needed.
 */
SeqNumsInUse_t *
GetSeqNumsInUse(
	char *vsn,
	char *fsname,
	SeqNumsInUse_t *inuse
)
{
	int count = 0;
	struct dirent *dirent;
	DIR *dirp;
	struct ArchReq *ar;
	upath_t	fullpath;

	snprintf(fullpath, sizeof (fullpath), ARCHIVER_DIR"/%s/"ARCHREQ_DIR,
	    fsname);

	if ((dirp = opendir(fullpath)) == NULL) {
		Trace(TR_ERR, "Can not open %s directory", fullpath);
		return (NULL);
	}

	while ((dirent = readdir(dirp)) != NULL) {
		if (*dirent->d_name == '.') {
			continue;
		}

		ar = ArchReqAttach(fsname, dirent->d_name, O_RDONLY);
		if (ar == NULL) {
			Trace(TR_ERR, "Can't attach %s, errno %d",
			    dirent->d_name, errno);
			return (NULL);
		}

		if (ar->ArState == ARS_archive && ar->ArFlags &
		    AR_diskArchReq) {
			for (int i = 0; i < ar->ArDrives; i++) {
				struct ArcopyInstance *ci;
				int ssize = sizeof (DiskVolumeSeqnum_t);
				int sinuse = sizeof (SeqNumsInUse_t);

				ci = &ar->ArCpi[i];
				if ((strcmp(ci->CiVsn, vsn) == 0) &&
				    ci->CiSeqNum != -1) {
					if (inuse == NULL) {
						SamMalloc(inuse, sinuse);
						inuse->count = 1;
						SamMalloc(inuse->seqnums,
						    ssize);
					} else {
						int cnt = inuse->count++;
						SamRealloc(inuse->seqnums,
						    cnt * ssize);
					}

					inuse->seqnums[inuse->count - 1] =
					    ci->CiSeqNum;
				}

			}

		}

		(void) MapFileDetach(ar);
	}

	return (inuse);
}


/*
 * Returns TRUE if the given sequence number is currenty in use by arcopy,
 * returns FALSE otherwise.
 */
boolean_t
IsSeqNumInUse(
	DiskVolumeSeqnum_t seqnum,
	SeqNumsInUse_t *seqNumsInUse
)
{
	if (seqNumsInUse != NULL) {
		for (int i = 0; i < seqNumsInUse->count; i++) {
			if (seqNumsInUse->seqnums[i] == seqnum) {
				return (B_TRUE);
			}
		}
	}

	return (B_FALSE);
}
