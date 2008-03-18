/*
 * devst_string.c  - Return device status string.
 *
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

#pragma ident "$Revision: 1.14 $"


#include <sys/types.h>
#include "pub/lib.h"
#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"

static	char	str[11];	/* Device status string	*/

char *
sam_devstr(uint_t p)
{
	dev_status_t	*dev_st;

	dev_st = (dev_status_t *)&p;

	str[0] = dev_st->scanning	? 's': '-';
	str[0] = dev_st->mounted	? 'm': str[0];
	str[0] = dev_st->maint		? 'M': str[0];
	str[1] = (dev_st->scan_err|dev_st->bad_media) ?
	    'E' : (dev_st->audit	? 'a' : '-');
	str[2] = dev_st->labeled	? 'l': '-';
	str[2] = dev_st->strange	? 'N': str[2];
	str[2] = dev_st->labelling	? 'L': str[2];
	str[3] = dev_st->wait_idle	? 'I': dev_st->attention  ? 'A': '-';
	str[4] = dev_st->unload		? 'U': dev_st->cleaning  ? 'C': '-';
#if defined(DEVICE_STRING)
	str[5] = dev_st->requested	? 'R': dev_st->stripe  ? 'S': '-';
#else
	str[5] = dev_st->requested	? 'R': '-';
#endif
	str[6] = dev_st->wr_lock	? 'w': '-';
	str[7] = dev_st->opened		? 'o': '-';
	str[8] = dev_st->positioning	? 'P': dev_st->stor_full  ? 'F': '-';
	str[9] = dev_st->ready		?
	    (dev_st->write_protect	? 'W':
	    (dev_st->read_only		? 'R': 'r')) :
	    (dev_st->present		? 'p': '-');

	str[10] = '\0';
	return (str);
}
