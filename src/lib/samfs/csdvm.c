/*
 * csdvm.c - DVM checksum function.
 *
 * This module is now obsolete, but is not deleted so that it can be
 * referenced, if needed, in the future.
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

#pragma ident "$Revision: 1.13 $"


#include <sys/types.h>
#include <string.h>

#include "sam/types.h"
#include "sam/checksum.h"
#define	DEC_INIT
#include "sam/checksumf.h"
#undef DEC_INIT

static void hash(word32 *hashbuf, csum_t *hashval);

void
cs_dvm(u_longlong_t *cookie, uchar_t *buf, int len, csum_t *val)
{
	uchar_t    *hashbuf;
	int    i, nlen, rem;

	if (cookie != NULL) {
		/* initialization */
		for (i = 0; i < 4; i++) {
			val->csum_val[i] = 0;
		}
		return;
	}

	if ((rem = len % CSUM_BYTES) != 0) {
		nlen = len - rem;
	} else {
		nlen = len;
	}

	i = 0;
	hashbuf = buf;
	while (i < nlen) {
/*LINTED pointer cast may result in improper alignment */
		hash((word32 *)hashbuf, val);
		i += CSUM_BYTES;
		hashbuf += CSUM_BYTES;
	}

	if (rem) {
		uchar_t    hb[CSUM_BYTES];

		(void) memset(hb, 0, CSUM_BYTES);
		hashbuf = buf + len - rem;
		i = 0;
		while (i < rem) {
			hb[i] = *hashbuf++;
			i++;
		}
		/* zero pad the hash buffer */
		while (i < CSUM_BYTES)
			hb[i++] = 0;

/*LINTED pointer cast may result in improper alignment */
		hash((word32 *)hb, val);
	}
}

static
void
hash(word32 *hashbuf, csum_t *hashval)
{
	static word32   a[6] = {
		32633,
		65419,
		65469,
		32717,
		65497,
		131009
	};
	word32    u[10];
	int    i;
	csum_t    scratch;


	scratch = *(csum_t *)hashbuf;

	for (i = 0; i < 8; i++) {
		u[0] = a[0] * scratch.csum_val[0];
		u[1] = a[1] + scratch.csum_val[1];
		u[2] = a[2] + scratch.csum_val[2];
		u[3] = a[3] + scratch.csum_val[3];
		u[4] = u[0] ^ u[2];
		u[5] = u[1] ^ u[3];
		u[6] = a[4] * u[4];
		u[7] = u[5] + u[6];
		u[8] = a[5] * u[7];
		u[9] = u[6] + u[8];
		scratch.csum_val[0] = u[0] ^ u[8];
		scratch.csum_val[1] = u[1] ^ u[9];
		scratch.csum_val[2] = u[2] ^ u[8];
		scratch.csum_val[3] = u[3] ^ u[9];
	}
	for (i = 0; i < 4; i++)
		hashval->csum_val[i] += scratch.csum_val[(i + 1) % 4];
}
