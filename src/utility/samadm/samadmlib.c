/*
 * samadmlib - samadm utility routines.
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
 * or https://illumos.org/license/CDDL.
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <unistd.h>

#include "samadm.h"

#pragma ident   "$Revision: 1.2 $"


/*
 * ----- ask - ask the user a y/n question
 *
 * Emit the message provided, and await a y/n response.
 * Allow a default y/n value to be passed, specifying
 * whether y or n is to be returned for a non-y/n answer.
 */
boolean_t
ask(char *msg, char def)
{
	int i, n;
	int defret = (def == 'y' ? B_TRUE : B_FALSE);
	char answ[120];

	printf("%s", msg);
	fflush(stdout);
	fflush(stderr);
	n = read(0, answ, sizeof (answ));
	for (i = 0; i < n; i++) {
		if (answ[i] == ' ' || answ[i] == '\t') {
			continue;
		}
		if (tolower(answ[i]) == 'n') {
			return (B_FALSE);
		}
		if (tolower(answ[i]) == 'y') {
			return (B_TRUE);
		}
		return (defret);
	}
	return (defret);
}
