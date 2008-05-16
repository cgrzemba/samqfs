/*
 * encrypt.c - encryption and decryption for license keys
 *
 * This file should NEVER be released outside of Sun proper.
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

/*
 * NOTE NOTE NOTE NOTE NOTE NOTE  NOTE NOTE NOTE
 *
 * These files (check_license.c encrypt.c license.c) are intended to
 * be included with the pieces of samfs that need access to the license
 * data.  It is done this way for security reasons.  By including these
 * files, rather than compiling and linking, we can make all the routines
 * static, which makes them much more difficult for a hacker to find.
 *
 */


static ulong_t  crypt_33[8] = { 33469, 37159, 42017, 51257, 60169, 70111,
    80141, 89821 };


static ulong_t F(ulong_t);


static void encrypt(ulong_t *word, ulong_t *id, ulong_t *crypt)
{
	ulong_t x[4];
	x[0] = word[0] ^ crypt[0];
	x[1] = F(x[0]) ^ word[1] ^ crypt[1];
	x[2] = F(x[1]) ^ word[2] ^ crypt[2];
	x[3] = F(x[2]) ^ word[3] ^ crypt[3];
	id[0] = x[1] ^ crypt[4];
	id[1] = x[0] ^ crypt[5];
	id[2] = x[2] ^ crypt[6];
	id[3] = x[3] ^ crypt[7];
}

static void decrypt(ulong_t *id, ulong_t *check, ulong_t *crypt)
{
	ulong_t x[4];
	x[3] = id[3] ^ crypt[7];
	x[2] = id[2] ^ crypt[6];
	x[0] = id[1] ^ crypt[5];
	x[1] = id[0] ^ crypt[4];
	check[3] = F(x[2]) ^ x[3] ^ crypt[3];
	check[2] = F(x[1]) ^ x[2] ^ crypt[2];
	check[1] = F(x[0]) ^ x[1] ^ crypt[1];
	check[0] = x[0] ^ crypt[0];
}

static ulong_t F(ulong_t x)
{
	int i, arr[4];
	ulong_t y, z, mask = 0xff;
	y = x;
	for (i = 0; i < 4; i++) {
	arr[i] = y & mask;
	y >>= 8;
	}

	y = arr[2];
	y <<= 8;
	y ^= arr[0];
	y <<= 8;
	y ^= arr[3];
	y <<= 8;
	y ^= arr[1];
	z = x + y;
	return (z);
}
