/*
 * catalog.c - catalog routines.
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.27 $"

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include "sam/types.h"
#include "aml/types.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/shm.h"

extern shm_alloc_t master_shm;

void
_UpdateCatalog(const char *SrcFile, const int SrcLine, dev_ent_t *un,
    uint32_t status_bits, int (*func)(const char *SrcFile,
    const int SrcLine, struct CatalogEntry *))
{
	struct CatalogEntry ce;
	int		ret;

	/* NOTE:  sometimes this happens... */
	if (un->mid == ROBOT_NO_SLOT) {
		sam_syslog(LOG_INFO,
		    "UpdateCatalog called with invalid mid value");
		return;
	}

	memset(&ce, 0, sizeof (ce));
	/* NOTE: un->media is zero */
	memmove(ce.CeMtype, sam_mediatoa(un->type), sizeof (ce.CeMtype));
	if (!(un->status.b.scan_err)) {
		memcpy(ce.CeVsn, un->vsn, sizeof (ce.CeVsn));
	}
	if (un->fseq != 0)  ce.CeEq = un->fseq;
	else  ce.CeEq = un->eq;
	ce.CeSlot = un->slot;
	ce.CePart = un->i.ViPart;

	ce.CeCapacity = un->capacity;
	if (un->space > un->capacity) {
		ce.CeSpace = un->capacity;
	} else {
		ce.CeSpace = un->space;
	}

	ce.CeBlockSize = un->sector_size;
	ce.CeLabelTime = un->label_time;
	ce.CeStatus = status_bits;

	if ((un->status.b.bad_media) | (un->status.b.scan_err)) {
		ce.CeStatus |= CES_bad_media;
	}

	if (un->status.b.write_protect) {
		ce.CeStatus |= CES_writeprotect;
	}

	if (!IS_OPTICAL(un)) {
		ce.m.CeLastPos = un->dt.tp.position;
	} else {
		ce.m.CePtocFwa = un->dt.od.ptoc_fwa;
	}

	ce.CeMid = un->mid;

	ret = func(SrcFile, SrcLine, &ce);
	if (ret != 0) {
		char	buf[STR_FROM_ERRNO_BUF_SIZE];
		int	SaveErrno;

		SaveErrno = errno;
		sam_syslog(LOG_INFO,
		    "UpdateCatalog called from %s:%d - catserver error: %s",
		    SrcFile, SrcLine,
		    StrFromErrno(SaveErrno, buf, sizeof (buf)));
		errno = SaveErrno;

	}
}
