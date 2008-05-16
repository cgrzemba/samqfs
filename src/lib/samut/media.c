/*
 * media.c - Convert between SAM media code and ASCII.
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

#pragma ident "$Revision: 1.15 $"

#include <string.h>
#include "sam/types.h"
#include "sam/devnm.h"
#include "pub/devstat.h"

/*
 * Return media name from media type.  Empty if not a valid
 * media type.
 */
char *
sam_mediatoa(
	int mt)			/* Media type */
{
	char *media_name;
	dev_nm_t *tmp;

	for (tmp = &dev_nm2mt[0]; tmp->nm != NULL; tmp++)
		if (mt == tmp->dt)
			return (tmp->nm);

	/*
	 * Did not find matching media type.  Check for third party media
	 * types.  If not third party, an empty string is returned.
	 */
	media_name = "";
	if ((mt & DT_CLASS_MASK) == DT_THIRD_PARTY) {
		int n;

		n = mt & DT_MEDIA_MASK;
		if (n >= '0' && n <= '9') {
			media_name = dev_nmtr[n - '0'];
		} else if (n >= 'a' && n <= 'z') {
			media_name = dev_nmtr[(n - 'a') + 10];
		}
	}
	return (media_name);
}


/*
 * Return media type from media name.  Zero (0) if not a
 * valid media name.
 */
int
sam_atomedia(
	char *name)		/* Media name */
{
	dev_nm_t *tmp;

	for (tmp = &dev_nm2mt[0]; tmp->nm != NULL; tmp++)
		if (strcmp(name, tmp->nm) == 0)
			return (tmp->dt);

	return (0);
}
