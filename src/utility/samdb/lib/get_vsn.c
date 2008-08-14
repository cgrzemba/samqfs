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
#pragma ident "$Revision: 1.1 $"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define	SAM_DB_NO_MYSQL
#include <sam/sam_db.h>

#include "util.h"


/*
 * get_vsn - Get media type/VSN from string.
 *
 *	Description:
 *	    Extract media type/VSN from string in format of "mt:vsn" or
 *	    "mt/vsn".  Media type is optionally specified in string
 *	    (i.e, no "mt:") in which case function either use the default
 *	    media specified or if no default, then will match only the
 *	    VSN for VSNs with a singular entry.
 *
 *	    get_vsn requires that the VSN_Cache be loaded (see vsn_cache.c)
 *
 *	On Entry:
 *	    s		String in the format "mt:vsn" or "mt/vsn"
 *	    media	Default media type.  NULL specified no default.
 *
 *	On Return:
 *	    media	Media type of specified VSN.
 *	    vsn		VSN.
 *
 *	Returns:
 *	    vsn_id from sam_vsns table entry.  Zero if media type/VSN
 *	    not found.  -1 if multiple entries found for VSN.
 */
int	get_vsn(
	char *s,		/* Media/VSN string */
	char **media,	/* Media type */
	char **vsn)		/* VSN */

{
	char *p, *m, *v;
	int i, k;

	m = NULL;
	v = s;

	if ((p = strchr(s, ':')) != NULL) {
	    m = s;
	    v = p+1;
	    *p = '\0';
	}

	if (m == NULL && (p = strchr(s, '/')) != NULL) {
	    m = s;
	    v = p+1;
	    *p = '\0';
	}

	if (m == NULL) {
		m = *media;		/* Set default media */
	}

	*media = m;
	*vsn   = v;

	if (m != NULL) {
	    return (sam_db_find_vsn(0, m, v));
	}

	/* Search cache	*/
	k = -1;
	for (i = 0; i < n_vsns; i++) {
		if (strcasecmp(VSN_Cache[i].vsn, v) != 0) {
			continue;
		}
		/* if multiple vsns */
		if (k >= 0) {
			return (-1);
		}
		k = i;
	}

	if (k < 0) {
		return (0);	/* not found */
	}

	*media = VSN_Cache[k].media;
	*vsn = VSN_Cache[k].vsn;
	return (VSN_Cache[k].vsn_id);
}
