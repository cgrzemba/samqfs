/*
 * nm_to_device.c  - Convert device nmemonic to device type.
 *
 *	Description:
 *		Convert two character device nmemonic to device type.
 *
 *	On entry:
 *		nm		= The nmemonic name for the device.
 *
 *	Returns:
 *		The device type or a -1 if the nmemonic is not recognized.
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

#pragma ident "$Revision: 1.14 $"


#ifdef LIBS_64
#define	DEC_INIT
#endif	/* LIBS_64 */

#include <string.h>
#include <sys/types.h>
#include <pub/devstat.h>
#include <sam/types.h>
#include <sam/param.h>

#include "sam/devnm.h"


int
nm_to_device(
	char *nm)	/* Device mnemonic */
{
	int i;

	/* For third party, or in the second character */
	if (*nm == 'z') {
		return (DT_THIRD_PARTY | *(nm + 1));
	}

	for (i = 0; i >= 0; i++) {
		if (dev_nm2dt[i].nm == NULL) {
			return (-1);
		}
		if (strcmp(nm, dev_nm2dt[i].nm) == 0) {
			return (dev_nm2dt[i].dt);
		}
	}
	/* NOTREACHED */
}
