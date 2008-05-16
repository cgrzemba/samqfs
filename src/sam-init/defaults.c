/*
 * defaults.c - read site defaults file.
 *
 * Solaris 2.x Sun Storage & Archiving Management File System
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

#pragma ident "$Revision: 1.36 $"


/* ANSI C headers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <fcntl.h>
#include <unistd.h>
#include <sys/syslog.h>

/* SAM-FS headers. */
#include "sam/lib.h"
#include "sam/defaults.h"

/* Local headers. */
#include "amld.h"


/*
 * Get defaults values.
 */
void
get_defaults(
	sam_defaults_t *defaults)
{
	sam_defaults_t *df;

	df = GetDefaults();
	memmove(defaults, df, sizeof (sam_defaults_t));

	/* Check if SAM's RPC server is to be started automatically. */
	if (defaults->flags & DF_START_RPC) {
		pids[CHILD_SAMRPC - 1].restart &= ~NO_AUTOSTART;
	}

	/* Do not allow timeout value less than 600 seconds, except for 0. */
	if (defaults->timeout > 0 && defaults->timeout < 600) {
		defaults->timeout = 600;
		sam_syslog(LOG_NOTICE,
		    "timeout configured too low, reset to 600 seconds");
	}
}


/*
 * set_media_to_default - set the media type to the default if needed
 */
void
set_media_to_default(
	media_t *media,
	sam_defaults_t *defaults)
{
	media_t	m_class;

	if (*media != 0) {
		m_class = *media & DT_CLASS_MASK;
		if ((*media & DT_MEDIA_MASK) == 0) {
			if (m_class == DT_TAPE) {
				*media = defaults->tape;
			} else if (m_class == DT_OPTICAL) {
				*media = defaults->optical;
			}
		}
	}
}
