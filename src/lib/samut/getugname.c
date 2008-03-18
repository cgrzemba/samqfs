/*
 * getugname.c - Find username/groupname from uid/gid
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

#pragma ident "$Revision: 1.8 $"

/* ANSI C headers. */
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

/* SAM headers */
#include "sam/sam_malloc.h"

static char *_SrcFile = __FILE__;

union ugid {
	uid_t u;
	gid_t g;
};

struct idelement {
	struct idelement *next;
	char *name;
	union ugid id;
};

static struct idelement *uids = NULL;
static struct idelement *gids = NULL;

/*
 * Map a uid to a user name.  If none, then generate the ASCII
 * numeric string.  Either way, save the result on the uid list.
 */
char *
getuser(uid_t uid)
{
	struct idelement *ide;
	struct passwd *pwent;
	char strbuf[32];

	for (ide = uids; ide != NULL; ide = ide->next) {
		if (ide->id.u == uid) {		/* already seen? */
			return (ide->name);
		}
	}

	/*
	 * Allocate and fill new entry
	 */
	SamMalloc(ide, sizeof (struct idelement));
	ide->id.u = uid;
	pwent = getpwuid(uid);
	if (pwent == NULL) {
		snprintf(strbuf, sizeof (strbuf), "%lu", (unsigned long)uid);
		strbuf[sizeof (strbuf)-1] = '\0';
		SamStrdup(ide->name, strbuf);
	} else {
		SamStrdup(ide->name, pwent->pw_name);
	}

	/*
	 * Prepend to list; return string.
	 */
	ide->next = uids;
	uids = ide;

	return (ide->name);
}


/*
 * Map a gid to a group name.  If none, then generate the ASCII
 * numeric string.  Either way, save the result on the gid list.
 */
char *
getgroup(gid_t gid)
{
	struct idelement *ide;
	struct group *grent;
	char strbuf[32];

	for (ide = gids; ide != NULL; ide = ide->next) {
		if (ide->id.g == gid) {		/* already seen? */
			return (ide->name);
		}
	}

	/*
	 * Allocate and fill new entry
	 */
	SamMalloc(ide, sizeof (struct idelement));
	ide->id.g = gid;
	grent = getgrgid(gid);
	if (grent == NULL) {
		snprintf(strbuf, sizeof (strbuf), "%lu", (unsigned long)gid);
		strbuf[sizeof (strbuf)-1] = '\0';
		SamStrdup(ide->name, strbuf);
	} else {
		SamStrdup(ide->name, grent->gr_name);
	}

	/*
	 * Prepend to list; return string.
	 */
	ide->next = gids;
	gids = ide;

	return (ide->name);
}
