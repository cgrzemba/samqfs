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
#pragma ident   "$Revision: 1.14 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * key.c
 * provides an implemenation of the interface in mgmt/struct_key.h
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "pub/mgmt/struct_key.h"
#include "pub/mgmt/archive.h"
#include "mgmt/util.h"
#include "sam/sam_trace.h"

#include <md5.h>

/*
 * return a key calculated on the specified number of bytes starting
 * at the void pointer. This function generates a key based on the
 * entire range from s to s + num_bytes. If that range is a structure
 * with regions that may not contain data this function may behave differently
 * from one call to the next. For instance if there is 20 character array that
 * only contains a 3 character string. The entire array will be used for key
 * generation. If on a subsequent call the 3 character string is the same
 * but any of the remaining characters is different the key will be different.
 */
int
get_key(
void *s,		/* begining of keyed region */
uint_t num_bytes,	/* number of bytes to get key for */
struct_key_t *key)	/* populated with the key on return */
{

	unsigned char mk[16];

	if (ISNULL(key, s)) {
		return (-1);
	}

	md5_calc(mk, s, num_bytes);
	memcpy(key, mk, 16);

	return (0);
}


/*
 * compare two keys return true if they match.
 */
boolean_t
keys_match(
struct_key_t k1,
struct_key_t k2)
{

	Trace(TR_DEBUG, "checking keys entry");

	if (memcmp(k1, k2, sizeof (*k1)) == 0) {
		Trace(TR_DEBUG, "keys match");
		return (B_TRUE);
	} else {
		Trace(TR_DEBUG, "keys do not match");
		return (B_FALSE);
	}
}
