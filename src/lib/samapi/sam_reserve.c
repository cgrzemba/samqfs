/*
 * sam_reserve.c - reserve and unreserve volumes for archiving.
 *
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

#pragma ident "$Revision: 1.14 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <stdlib.h>
#include <string.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "sam/custmsg.h"
#include "sam/lib.h"
#include "aml/samapi.h"


/*
 * SamReserveVolume - Reserve volume for archiving.
 * Returns 0 if success, -1 if failure
 */
int
SamReserveVolume(
	char *volspec,	/* Volume specification eq:slot or mt.vsn */
	time_t rtime,
	char *reserve,	/* Reservation string */
	void (*MsgFunc)(int code, char *msg))
{
	struct CatalogEntry ced;
	struct VolId vid;
	uname_t	asname;
	uname_t	owner;
	uname_t	fsname;
	char	*p;
	int	n;

	/*
	 * Convert specifier to volume identifier.
	 * Validate specifier.
	 */
	if (StrToVolId(volspec, &vid) != 0) {
		LibError(MsgFunc, 0, 18207, volspec);
				/* "Volume specification error %s" */
		return (-1);
	}

	memset(asname, 0, sizeof (asname));
	memset(owner, 0, sizeof (owner));
	memset(fsname, 0, sizeof (fsname));
	p = reserve;
	for (n = 0; *p != '/'; n++) {
		if (*p == '\0' || n >= sizeof (asname)) {
			LibError(MsgFunc, 0, 18214, reserve);
				/* "Volume reserve specification error %s */
			return (-1);
		}
		asname[n] = *p++;
	}
	p++;
	for (n = 0; *p != '/'; n++) {
		if (*p == '\0' || n >= sizeof (owner)) {
			LibError(MsgFunc, 0, 18214, reserve);
				/* "Volume reserve specification error %s */
			return (-1);
		}
		owner[n] = *p++;
	}
	p++;
	for (n = 0; *p != '\0'; n++) {
		if (*p == '/' || n >= sizeof (fsname)) {
			LibError(MsgFunc, 0, 18214, reserve);
				/* "Volume reserve specification error %s */
			return (-1);
		}
		fsname[n] = *p++;
	}

	if (CatalogInit("SamApi") == -1) {
		LibError(MsgFunc, 0, 18211);
				/* "Catalog initialization failed" */
		return (-1);
	}
	if (CatalogCheckSlot(&vid, &ced) == NULL) {
		char vbs[STR_FROM_VOLID_BUF_SIZE];

		LibError(MsgFunc, 0, 18207,
		    StrFromVolId(&vid, vbs, sizeof (vbs)));
				/* "Volume specification error %s" */
		return (-1);
	}
	if (CatalogReserveVolume(&vid, rtime, asname, owner, fsname) == -1) {
		LibError(MsgFunc, 0, 18215);
				/* "Reserve volume failed */
		return (-1);
	}
	return (0);
}


/*
 * SamUnreserveVolume - Unreserve volume for archiving.
 * Returns 0 if success, -1 if failure
 */
int
SamUnreserveVolume(
	char *volspec,	/* Volume specification eq:slot or mt.vsn */
	void (*MsgFunc)(int code, char *msg))
{
	struct CatalogEntry ced;
	struct VolId vid;
	char vbs[STR_FROM_VOLID_BUF_SIZE];

	/*
	 * Convert specifier to volume identifier.
	 * Validate specifier.
	 */
	if (StrToVolId(volspec, &vid) != 0) {
		LibError(MsgFunc, 0, 18207, volspec);
				/* "Volume specification error %s" */
		return (-1);
	}

	if (CatalogInit("SamApi") == -1) {
		LibError(MsgFunc, 0, 18211);
				/* "Catalog initialization failed" */
		return (-1);
	}
	if (CatalogCheckSlot(&vid, &ced) == NULL) {
		LibError(MsgFunc, 0, 18207,
		    StrFromVolId(&vid, vbs, sizeof (vbs)));
				/* "Volume specification error %s" */
		return (-1);
	}
	if (CatalogUnreserveVolume(&vid) == -1) {
		LibError(MsgFunc, 0, 18216);
				/* "Unreserve volume failed */
		return (-1);
	}
	return (0);
}
