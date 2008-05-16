/*
 *  samunhold.c
 *
 * Release SAN holds on a SAM-FS filesystem.
 *
 * Note that this can be a dangerous thing to do.
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


#pragma ident "$Revision: 1.15 $"

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>
#include "samsanergy/fsmdc.h"

int
main(int ac, char *av[])
{
	int l;
	FSUNI *u;
	FSLONG r;
	FSVOLCOOKIE vc;
	FSFILECOOKIE fc;

	if (ac != 2) {
		fprintf(stderr, "Usage:\n\t# %s mount_point\n", av[0]);
		exit(1);
	}
	if (getuid()) {
		fprintf(stderr, "%s:  Not super-user.\n", av[0]);
		exit(2);
	}
	l = sizeof (FSUNI) * (strlen(av[1]) + strlen("/.inodes") + 1);
	if ((u = malloc(l)) == NULL) {
		fprintf(stderr, "%s:  Unable to allocate %d bytes of "
		    "memory.\n",
		    av[0], l);
		exit(3);
	}
	/*
	 * concatenate av[0] + "/.inodes" into FSUNI string.
	 */
	{
		char *s = av[1];
		FSUNI *u2 = u;

		while (*s)
			*u2++ = *s++;

		s = "/.inodes";
		while (*u2++ = *s++)
			;
	}
	r = FS_GetCookies(u, &vc, &fc);
	if (r != FS_E_SUCCESS) {
		fprintf(stderr, "%s:  FS_GetCookies('%s%s', &vc, &fc) "
		    "failed: %#x\n",
		    av[0], av[1], "/.inodes", r);
		exit(20);
	}
	r = FS_UnlockMap(&vc, &fc);
	if (r != FS_E_SUCCESS) {
		fprintf(stderr, "%s:  FS_UnlockMap(&vc, &fc) failed: %#x\n",
		    av[0], r);
		exit(21);
	}
	return (0);
}
