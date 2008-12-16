/*
 * cssimple.c - Simple checksum function.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#include "sam/types.h"
#include <sam/fs/bswap.h>

#pragma ident "$Revision: 1.16 $"


void
cs_simple(u_longlong_t *cookie, uchar_t *buf, int len, csum_t *val)
{
	int i;
	char *p, *q;

	if (cookie != NULL) {
		/* initialization */
		for (i = 0; i < 4; i++) {
			val->csum_val[i] = 0;
		}

		p = (char *)val;
		for (i = 0; i < 4; i++) {
			p[i+4] = (char)*((char *)cookie+i);
		}

		for (i = 0; i < 4; i++) {
			p[i + 12] = (char)*((char *)cookie+i);
		}
		return;
	}

	p = (char *)val;
	while (len >= 16) {
		for (i = 0; i < 16; i++) {
			p[i] = (p[i] + *(buf+i)) & 0xff;
		}
		len -= 16;
		buf += 16;
	}

	if (len != 0) {

		char local[16];

		for (i = 0; i < len; i++) {
			local[i] = *(buf+i);
		}
		for (i = len; i < 16; i++) {
			local[i] = 0;
		}
		q = &local[0];
		for (i = 0; i < 16; i++) {
			p[i] = (p[i] + *(q + i)) & 0xff;
		}
	}
}

/*
 * Unfortunately, we define the checksum as an array of 4 uint_t's
 * and byte-swap it when restoring on the opposite endian architecture.
 * Also, the cookie initialization in cs_simple isn't endian neutral.
 * This routine corrects the checksum to match what was generated.
 */
void
cs_repair(
	uchar_t *csum,
	u_longlong_t *cookie)
{
	int i;
	char local[16];

	for (i = 0; i < 16; i++) {
		local[i] = csum[i];
	}
	/*
	 * Rearrange cookie (file space)
	 */
	for (i = 0; i < 4; i++) {
		local[i+4] -= (char)*((char *)cookie+i+4);
		local[i+12] -= (char)*((char *)cookie+i+4);
	}
	sam_bswap4(local, 4);
	for (i = 0; i < 4; i++) {
		local[i+4] += (char)*((char *)cookie+i);
		local[i+12] += (char)*((char *)cookie+i);
	}
	for (i = 0; i < 16; i++) {
		csum[i] = local[i];
	}
}
