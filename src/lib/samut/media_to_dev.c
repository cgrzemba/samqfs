/*
 * media_to_device.c - Convert media type to device type.
 *
 *	Description:
 *	    Media_to_devaice converts a media type to device type.
 *
 *	On entry:
 *	    nm   The nmemonic name for the media.
 *
 *	Returns:
 *	    The device type or a -1 if the nmemonic is not recognized.
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

#pragma ident "$Revision: 1.15 $"

#include <sys/types.h>
#include <string.h>
#include <ctype.h>

#include "pub/devstat.h"
#include "sam/types.h"
#include "sam/param.h"
#include "sam/devnm.h"
#include "sam/lib.h"


/* Media alias table: */

dev_nm_t dev_alias[] = {
	{"tape",	1, DT_TAPE		},
	{"video",	0, DT_VIDEO_TAPE	},
	{"exabyte",	0, DT_EXABYTE_TAPE	},
	{"8mm",		0, DT_EXABYTE_TAPE	},
	{"ait",		0, DT_SONYAIT		},
	{"sait",	0, DT_SONYSAIT		},
	{"dtf",		0, DT_SONYDTF		},
	{"dlt",		0, DT_LINEAR_TAPE	},
	{"optical",	1, DT_OPTICAL		},
	{"worm12",	0, DT_WORM_OPTICAL_12	},
	{"worm",	0, DT_WORM_OPTICAL	},
	{"none",	1, 0			},
	{NULL,		0, 0			}
};

int
media_to_device(
	char *nm)		/* media nmemonic string	*/

{
	int	i;

	for (i = 0; dev_nm2mt[i].nm != NULL; i++) {
		if (strcmp(nm, dev_nm2mt[i].nm) == 0) {
			return (dev_nm2mt[i].dt);
		}
	}

	for (i = 0; dev_alias[i].nm != NULL; i++) {
		if (strcmp(nm, dev_alias[i].nm) == 0) {
			return (dev_alias[i].dt);
		}
	}

	return (-1);
}
