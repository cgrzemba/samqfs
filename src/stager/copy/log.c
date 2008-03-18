/*
 * log.c - collect file staging events and write to log file
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

#pragma ident "$Revision: 1.10 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <stdio.h>

/* POSIX headers. */

/* SAM-FS headers. */
#include "aml/stager.h"
#include "aml/stager_defs.h"

/* Local headers. */
#include "log_defs.h"
#include "stager_lib.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/*
 * Write staging event to log file.
 */
void
LogIt(
	LogType_t type,
	FileInfo_t *file)
{
	if (IsLogEventEnabled(type)) {
		(void) StagerLogEvent(type, file->sort);
	}
}
