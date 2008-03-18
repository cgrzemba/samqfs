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
#ifndef STRUCT_KEY_H
#define	STRUCT_KEY_H

#pragma ident   "$Revision: 1.9 $"


/*
 * struct_key provides interfaces for getting and checking a key that is
 * generated based on the contents of a structure.
 */

#include <sys/types.h>

typedef unsigned char	struct_key_t[16];

/*
 * return a key calculated on the specified number of bytes starting
 * at the void pointer. NOTE: This function keys over the entire range of bits
 * from the pointer to num_bytes.
 */
int get_key(void *, uint_t num_bytes, struct_key_t *key);

/*
 * compare two keys return true if they match.
 */
boolean_t keys_match(struct_key_t k1, struct_key_t k2);

#endif	/* STRUCT_KEY_H */
