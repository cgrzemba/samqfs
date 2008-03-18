/*
 * archset.c - Archive set table access functions.
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#pragma ident "$Revision: 1.26 $"

/* static char *_SrcFile = __FILE__; $*Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <sys/param.h>
#include <fcntl.h>

/* SAM-FS headers. */
#include "sam/custmsg.h"
#include "sam/lib.h"

/* Local headers. */
#include "common.h"
#include "archset.h"


/*
 * Find an archive set.
 */
struct ArchSet *
FindArchSet(
	char *name)	/* Archive set name */
{
	struct ArchSet *as;
	int	asn;

	if (ArchSetTable == NULL) {
		return (NULL);
	}
	for (asn = 0, as = ArchSetTable; asn < ArchSetNumof; asn++, as++) {
		if (strcmp(name, as->AsName) == 0) {
			return (as);
		}
	}
	return (NULL);
}


/*
 * Read archive set file.
 */
struct ArchSet *
ArchSetAttach(
	int mode)
{
	static char *fname = ARCHIVER_DIR"/"ARCHIVE_SETS;
	void	*mp;

	if (ArchSetFile != NULL) {
		(void) ArMapFileDetach(ArchSetFile);
	}
	if ((mp = ArMapFileAttach(fname, ARCHSETS_MAGIC, mode)) == NULL) {
		return (NULL);
	}
	ArchSetFile = (struct ArchSetFileHdr *)mp;
	if (ArchSetFile->AsVersion != ARCHSETS_VERSION) {
		errno = EADV;
		return (NULL);
	}
	ArchSetTable = (struct ArchSet *)(void *)((char *)ArchSetFile +
	    ArchSetFile->ArchSetTable);
	ArchSetNumof = ArchSetFile->ArchSetNumof;
	VsnExpTable = (struct VsnExp *)(void *)((char *)ArchSetFile +
	    ArchSetFile->VsnExpTable);
	VsnExpSize = ArchSetFile->VsnExpSize;
	VsnListTable = (struct VsnList *)(void *)((char *)ArchSetFile +
	    ArchSetFile->VsnListTable);
	VsnListSize = ArchSetFile->VsnListSize;
	VsnPoolTable = (struct VsnPool *)(void *)((char *)ArchSetFile +
	    ArchSetFile->VsnPoolTable);
	VsnPoolSize = ArchSetFile->VsnPoolSize;
	MediaParams = (struct MediaParams *)(void *)((char *)ArchSetFile +
	    ArchSetFile->AsMediaParams);
	return (ArchSetTable);
}


/*
 * Return pointer to the MediaParams table entry for a given media.
 * RETURN: Media params entry.
 */
struct MediaParamsEntry *
MediaParamsGetEntry(
	mtype_t mtype)	/* Media type to find */
{
	int   i;

	if (MediaParams == NULL) {
		return (NULL);
	}
	for (i = 0; i < MediaParams->MpCount; i++) {
		struct MediaParamsEntry *mp;

		mp = &MediaParams->MpEntry[i];
		if (strcmp(mtype, mp->MpMtype) == 0) {
			/*
			 * Check for a default media class.
			 */
			if (mp->MpFlags & MP_default) {
				int	j;

				/*
				 * Look up the actual media to use.
				 */
				for (j = 0; j < MediaParams->MpCount; j++) {
					if (MediaParams->MpEntry[j].MpType ==
					    mp->MpType) {
						mp = &MediaParams->MpEntry[j];
						break;
					}
				}
			}
			return (mp);
		}
	}
	return (NULL);
}


/*
 * Get archmax for an Archive Set.
 */
fsize_t
GetArchmax(
	struct ArchSet *as,
	char *mtype)
{
	fsize_t archmax;

	/*
	 * Determine archmax.
	 */
	if (as->AsFlags & AS_archmax) {
		archmax = as->AsArchmax;
	} else {
		struct MediaParamsEntry *mp;

		/*
		 * Get archmax based on media type.
		 */
		if (mtype == NULL) {
			mtype = as->AsMtype;
		}
		mp = MediaParamsGetEntry(mtype);
		if (mp != NULL) {
			archmax = mp->MpArchmax;
		} else {
			archmax = ODARCHMAX;    /* We need something */
		}
	}
	return (archmax);
}
