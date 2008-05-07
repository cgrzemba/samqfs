/*
 * device_to_nm.c - Convert device type to device mnemonic.
 *
 *  Description:
 *    Convert device type to a two character mnemonic.
 *
 *  On entry:
 *    dt      = The device type.
 *
 *  Returns:
 *    A pointer to the two character device mnemonic.
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
#include <ctype.h>

#include "pub/devstat.h"
#include "sam/types.h"
#include "sam/param.h"
#define	DEC_INIT
#include "sam/devnm.h"

/*
 * Test a character pointer array index for validity.
 * Note that these are null terminated, hence the "-2".
 * Here's a sample from include/sam/devnm.h:
 *  char    *dev_nmps[] = {
 *   "ps", "si", "sc", "ss", "rd", "hy", NULL };
 */

#define	getMn(i, p) get_mn((i), (p), sizeof (p), sizeof (*(p)))

boolean_t
test_index(
	int ind,
	int size,
	int psize)
{
	return ((ind > (size/psize) - 2) ? B_FALSE : B_TRUE);
}

char *
get_mn(
	int i,
	char **p,
	int size,
	int psize)
{
	if (test_index(i, size, psize)) {
		return (p[i]);
	} else {
		return ("??");
	}
}

char *
device_to_nm(
	int dt)
{
	switch (dt & DT_CLASS_MASK) {
	case 0:
	return ("sy");
	case DT_DISK:
		if (is_stripe_group(dt)) {
			if ((dt & DT_MEDIA_MASK) < dev_nmsg_size) {
				return (dev_nmsg[dt & DT_MEDIA_MASK]);
			}
			return ("??");
		} else {
			return (getMn((dt & DT_MEDIA_MASK), dev_nmmd));
		}
	case DT_TARGET:
		if (is_target_group(dt)) {
			if ((dt & DT_MEDIA_MASK) < dev_nmtg_size) {
				return (dev_nmtg[dt & DT_MEDIA_MASK]);
			}
			return ("??");
		} else {
			return ("??");
		}
	case DT_OPTICAL:
		return (getMn((dt & DT_MEDIA_MASK), dev_nmod));
	case DT_TAPE:
		return (getMn((dt & DT_MEDIA_MASK), dev_nmtp));
	case DT_FAMILY_SET:
		return (getMn((dt & DT_MEDIA_MASK), dev_nmfs));
	case DT_ROBOT:
		return (getMn((dt & DT_ROBOT_MASK), dev_nmrb));
	case DT_PSEUDO:
	case DT_PSEUDO | DT_FAMILY_SET:
		return (getMn((dt & DT_MEDIA_MASK), dev_nmps));
	case DT_THIRD_PARTY:
		if (islower(dt & DT_THIRD_MASK))
			return (dev_nmtr[((dt & DT_THIRD_MASK) - 'a') + 10]);
		else if (isdigit(dt & DT_THIRD_MASK))
			return (dev_nmtr[(dt & DT_THIRD_MASK) - '0']);
	default:
		break;
	}
	return ("??");
}

#if defined(TEST)

struct tv {
	int dt;		/* device type from pub/devstat.h */
	char *goodres;	/* correct string to be returned  */
};

struct tv test_val[] = {
	{0x0101, "md"}, {0x0104, "??"}, {0x0180, "g0  "}, {0x0190, "g16 "},
	{0x0201, "vt"}, {0x0211, "li"}, {0x020b, "so"}, {0x18cd, "eb"},
	{0x0502, "wo"}, {0x0801, "ms"}, {0x0802, "ma"},
	{0x1801, "rc"}, {0x184c, "gr"}, {0x1851, "im"}, {0x189a, "pe"},
	{0x1884, "hp"}, {0x1899, "pd"}, {0x18c5, "ml"}, {0x18d8, "ac"},
	{0x18d5, "dm"}, {0x2003, "ss"}, {0x2005, "hy"}, {0x2802, "sc"},
	{0x8032, "z2"}, {0x8066, "zf"}, {0x8071, "zq"}, {0x807b, "??"},
/* next two should be errors, but we don't check upper bits */
	{0x28032, "z2"}, {0x10001, "sy"},
	{0xffff, "??"}, {0xe2345, "??"},
	{0x0, ""}
};

int
main(int argc)
{

	int i = 0;
	char *result;
	char *flag;

	while (test_val[i].dt != 0) {
		result = device_to_nm(test_val[i].dt);
		flag = strcmp(result,
		    test_val[i].goodres) ? "***** ERROR ****" : "OK";
		printf("Device type 0x%x is %s, should be %s %s\n",
		    test_val[i].dt, result, test_val[i].goodres, flag);
		i++;

	}
}
#endif
