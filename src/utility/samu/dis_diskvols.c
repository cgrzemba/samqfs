/*
 * ---- dis_disvols.c - Display disk volume dictionary.
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

#pragma ident "$Revision: 1.17 $"

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <db.h>

/* POSIX headers. */
#include <sys/types.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <sys/mman.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/dbfile.h"
#include "sam/udscom.h"
#include "sam/nl_samfs.h"
#include "aml/diskvols.h"

/* Local headers. */
#include "samu.h"

/* Private functions. */

/* Private data. */
static DiskVolsDictionary_t *vsnDict = NULL;
static DiskVolsDictionary_t *cliDict = NULL;
static DiskVolsDictionary_t *hdrDict = NULL;
static int details = 0;

/* Public data. */

boolean
InitDiskvols(
	void)
{
	return (TRUE);
}

/*
 * Disk volumes dictionary display.
 */
void
DisDiskvols(
	void)
{
	int idx;
	DiskVolumeInfo_t *dv;
	char *volname;
	char *cliname;
	DBFile_t *dbfile;
	DB_BTREE_STAT *stat;
	DB *db;
	char pathname[128];
	boolean_t header;
	char space[16];
	char capacity[16];
	char used[16];
	fsize_t spaceUsed;
	char port[16];
	boolean_t honeycomb;
	int rval;
	DiskVolumeVersionVal_t *version;

	hdrDict = DiskVolsNewHandle("samu", DISKVOLS_HDR_DICT,
	    DISKVOLS_RDONLY);
	if (hdrDict == NULL) {
		Mvprintw(ln++, 0, "no header found.");
	} else {
		Mvprintw(ln++, 0, "header");
		rval = hdrDict->GetVersion(hdrDict, &version);
		if (rval == 0) {
			Mvprintw(ln++, 0, "version %d", *version);
		}
		(void) DiskVolsDeleteHandle(DISKVOLS_HDR_DICT);
	}

	Mvprintw(ln++, 0, "%s", " ");
	vsnDict = DiskVolsNewHandle("samu", DISKVOLS_VSN_DICT, 0644);
	if (vsnDict == NULL) {
		Mvprintw(ln++, 0, "no volumes found.");
	} else {
		Mvprintw(ln++, 0, "volumes");
		dbfile = (DBFile_t *)vsnDict->dbfile;
		db = (DB *)dbfile->db;
		(void) db->stat(db, NULL, &stat, 0);
		Mvprintw(ln++, 0, "magic %d version %d nkeys %d ndata %d",
		    stat->bt_magic,
		    stat->bt_version,
		    stat->bt_nkeys,
		    stat->bt_ndata);
		header = B_FALSE;

		idx = 0;
		vsnDict->BeginIterator(vsnDict);
		while (vsnDict->GetIterator(vsnDict, &volname, &dv) == 0) {
			char flags[] = "xxxxxxx";

			honeycomb = B_FALSE;
			/*
			 *	Build path name to disk archive directory.
			 *	Includes host name if path resides on a
			 *	remote server.
			 */
			pathname[0] = '\0';
			if (dv->DvHost[0] != '\0') {
				strcpy(pathname, dv->DvHost);
				strcat(pathname, ":");
			}
			strcat(pathname, dv->DvPath);
			honeycomb = DISKVOLS_IS_HONEYCOMB(dv);

			flags[0] = (dv->DvFlags & DV_labeled)   ? 'l' : '-';
			if (dv->DvFlags & DV_remote) {
				flags[1] = 'r';
			} else {
				flags[1] = '-';
			}
			flags[2] = (dv->DvFlags & DV_unavail)   ? 'U' : '-';
			flags[3] = (dv->DvFlags & DV_read_only) ? 'R' : '-';
			flags[4] = (dv->DvFlags & DV_bad_media) ? 'E' : '-';
			flags[5] = (dv->DvFlags & DV_needs_audit) ? 'A' : '-';
			flags[6] = (dv->DvFlags & DV_archfull)  ? 'F' : '-';

			if (header == B_FALSE) {
				Mvprintw(ln++, 0, "index        space     "
				    "capacity         used flags  volume");
				if (details) {
					Mvprintw(ln++, 0,
					"                                 "
					"                   pathname");
				}
				header = B_TRUE;
			}

			if ((dv->DvFlags & DV_unavail) == 0) {
				(void) DiskVolsGetSpaceUsed(volname, dv, NULL,
				    &spaceUsed);
			}
			if (dv->DvCapacity == 0) {
				/* Not initialized yet. */
				capacity[0] = space[0] = used[0] = '-';
				capacity[1] = space[1] = used[1] = '\0';
				Mvprintw(ln++, 0, "%5d %12s %12s %12s %s %s",
				    idx,
				    space,
				    capacity,
				    used,
				    flags,
				    volname);
			} else {
				Mvprintw(ln++, 0,
				    "%5d %12lld %12lld %12lld %s %s",
				    idx,
				    (honeycomb == B_FALSE) ? dv->DvSpace : 0,
				    (honeycomb == B_FALSE) ? dv->DvCapacity : 0,
				    spaceUsed,
				    flags,
				    volname);
			}
			if (details) {
				if (dv->DvSpace != 0) {
					(void) StrFromFsize(dv->DvSpace, 1,
					    space, sizeof (space));
				} else {
					strncpy(space, "0", sizeof (space));
				}

				if (dv->DvCapacity != 0) {
					(void) StrFromFsize(dv->DvCapacity, 1,
					    capacity, sizeof (capacity));
				} else {
					strncpy(capacity, "0",
					    sizeof (capacity));
				}

				if (spaceUsed != 0) {
					(void) StrFromFsize(spaceUsed, 1,
					    used, sizeof (used));
				} else {
					strncpy(used, "0", sizeof (used));
				}

				if (honeycomb == B_FALSE) {
					Mvprintw(ln++, 0,
					    "%5s %12s %12s %12s %6#x %s",
					    " ",
					    space,
					    capacity,
					    used,
					    dv->DvFlags,
					    pathname);
				} else {
					/* Honeycomb */
					if (dv->DvPort == -1) {
						Mvprintw(ln++, 0,
						    "%5s %12s %12s %12s"
						    " %6#x %s %s",
						    " ",
						    space,
						    capacity,
						    used,
						    dv->DvFlags,
						    HONEYCOMB_DEVNAME,
						    dv->DvAddr);
					} else {
						snprintf(port, sizeof (port),
						    "%d", dv->DvPort);
						Mvprintw(ln++, 0,
						    "%5s %12s %12s %12s"
						    " %6#x %s %s:%s",
						    " ",
						    space,
						    capacity,
						    used,
						    dv->DvFlags,
						    HONEYCOMB_DEVNAME,
						    dv->DvAddr,
						    port);
					}
				}
			}
			idx++;
		}
		vsnDict->EndIterator(vsnDict);
		(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
	}

	Mvprintw(ln++, 0, "%s", " ");
	cliDict = DiskVolsNewHandle("samu", DISKVOLS_CLI_DICT, 0644);
	if (cliDict == NULL) {
		Mvprintw(ln++, 0, "no trusted clients found.");
	} else {
		Mvprintw(ln++, 0, "clients");
		dbfile = (DBFile_t *)cliDict->dbfile;
		db = (DB *)dbfile->db;
		(void) db->stat(db, NULL, &stat, 0);
		Mvprintw(ln++, 0, "magic %d version %d nkeys %d ndata %d",
		    stat->bt_magic,
		    stat->bt_version,
		    stat->bt_nkeys,
		    stat->bt_ndata);

		header = B_FALSE;
		idx = 0;

		cliDict->BeginIterator(cliDict);
		while (cliDict->GetIterator(cliDict, &cliname, &dv) == 0) {
			if (header == B_FALSE) {
				Mvprintw(ln++, 0, "index flags client");
				header = B_TRUE;
			}
			Mvprintw(ln++, 0, "%5d %#5x %s",
			    idx,
			    dv->DvFlags,
			    cliname);
			idx++;
		}
		cliDict->EndIterator(cliDict);
		(void) DiskVolsDeleteHandle(DISKVOLS_CLI_DICT);
	}
}

/*
 *	Keyboard processing.
 */
boolean
KeyDiskvols(
	char key)
{
	switch (key) {

	case KEY_details:
		details ^= 1;
		break;

	default:
		return (FALSE);
	}
	return (TRUE);
}
