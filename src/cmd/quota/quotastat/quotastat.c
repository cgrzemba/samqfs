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

/*
 * samquotastat [-agu] <FS>
 *
 * query {admin,group,user} quotas on filesystem <FS>.
 *
 */

#pragma ident "$Revision: 1.15 $"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define DEC_INIT
#include "sam/types.h"
#include "sam/param.h"
#include "sam/quota.h"
#include "sam/lib.h"

#define	streq(x, y)		(strcmp(x, y) == 0)
#define	strsuffix(str, suf)	streq((str)+strlen(str)-strlen(suf), suf)

static void
usage(char *name, int xval)
{
	fprintf(stderr, "Usage:\n\t%s [-agu] fs\n", name);
	exit(xval);
}

int
main(int ac, char *av[])
{
	int i, qfd, qmask = 0, r;
	extern int errno, optind;

	while ((i = getopt(ac, av, "aghu")) != EOF) {
		switch (i) {
		case 'a':
			qmask |= SAM_QFL_ADMIN;
			break;
		case 'g':
			qmask |= SAM_QFL_GROUP;
			break;
		case 'h':
			usage(av[0], 0);			/* No return */
		case 'u':
			qmask |= SAM_QFL_USER;
			break;
		default:
			usage(av[0], 1);
		}
	}

	if (optind != ac-1)
		usage(av[0], 1);

	if ((qfd = open(av[ac-1], O_RDONLY)) < 0) {
		fprintf(stderr,
		    "Can't open '%s' -- %s.\n", av[ac-1], strerror(errno));
		usage(av[0], 10);
	}

	if (qmask == 0)
		qmask = SAM_QFL_ADMIN|SAM_QFL_GROUP|SAM_QFL_USER;

	if (strsuffix(av[0], "stat")) {
		int rmask = 0;

		r = sam_get_quota_stat(qfd, &rmask);
		if (r >= 0) {
			for (i = 0; i < SAM_QUOTA_DEFD; i++) {
				if (qmask & (1 << i)) {
					if (rmask & (1 << i)) {
						printf("%s quota enabled\n",
						    quota_types[i]);
					} else {
						printf("%s quota disabled\n",
						    quota_types[i]);
					}
				}
			}
			exit(!(qmask & rmask)); /* exit(0) if any enabled */
		}
	} else {
		fprintf(stderr, "%s: Invalid program name.\n", av[0]);
		usage(av[0], 10);
	}
	if (r < 0) {
		fprintf(stderr, "%s: Error -- %s\n", av[0], strerror(errno));
	}
	return (r ? 20 : 0);
}
