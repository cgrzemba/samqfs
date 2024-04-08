/*
 * control.c - Stager control.
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

#pragma ident "$Revision: 1.14 $"

/* ANSI C headers. */
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/exit.h"
#include "sam/sam_trace.h"
#include "sam/lib.h"
#include "aml/stager.h"
#include "aml/stager_defs.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Local headers. */
#include "stager_lib.h"
#include "copy_defs.h"
#include "stream.h"

#include "schedule.h"

char *
Control(
	char *ident,
	char *value)
{
	char *lasts;
	char *media;
	char *volume;
	extern boolean_t IdleStager;

	if (strcmp(ident, "exec") == 0) {
		if (strcmp(value, "idle") == 0) {
			IdleStager = B_TRUE;
		} else if (strcmp(value, "run") == 0) {
			IdleStager = B_FALSE;
		}
	} else if (strcmp(ident, "stclear") == 0) {
		media = (char *)strtok_r(value, ".", &lasts);
		volume = (char *)strtok_r(NULL, " ", &lasts);

		ClearScheduler(sam_atomedia(media), volume);
	}
	return ("");
}
