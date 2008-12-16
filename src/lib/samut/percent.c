/*
 * percent.c - Compute device usage percentage.
 *
 * int percent_used(uint capacity, uint space) int llpercent_used(uint64_t
 * capacity, uint64_t space)
 *
 *
 * Description: Computes percent usage as an integer 0-100.
 *
 * Returns: For completely empty media, return  0. For 0-1% full media, return
 * 1. For 1-99% full media, return percentage rounded up:
 * ((capacity-space)/capacity)*100 + 0.5 For 99-100% full media, return
 * 99. For completely full media, return 100.
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

#pragma ident "$Revision: 1.16 $"


#include <sys/types.h>

#include "sam/types.h"


int
percent_used(uint_t capacity, uint_t space)
{
	double p;

	if (capacity == 0)
		return (0);

	p = (((double)capacity - (double)space) / (double)capacity) * 100.0;

	if ((p > 0.0) && (p < 1.0))
		p = 1.0;
	else if ((p > 99.0) && (p < 100.0))
		p = 99.0;
	else
		p += 0.5;

	return ((int)p);
}

int
llpercent_used(u_longlong_t capacity, u_longlong_t space)
{
	double p;

	if (capacity == 0LL)
		return (0);

	p = (((double)capacity - (double)space) / (double)capacity) * 100.0;

	if ((p > 0.0) && (p < 1.0))
		p = 1.0;
	else if ((p > 99.0) && (p < 100.0))
		p = 99.0;
	else
		p += 0.5;

	return ((int)p);
}

#ifdef TEST
#define	L(x) ((u_longlong_t)x)
int
P_U(u_longlong_t space, u_longlong_t capacity)
{
	double p;

	p = (((double)capacity - (double)space) / (double)capacity) * 100.0;

	if ((p > 0.0) && (p < 1.0))
		p = 1.0;
	else if ((p > 99.0) && (p < 100.0))
		p = 99.0;
	else
		p += 0.5;

	return ((int)p);
}

#define	TestIt(s, c) c, s, percent_used(L(c), L(s)), P_U(L(s), L(c))
#define	llTestIt(s, c) c, s, llpercent_used(L(c), L(s)), P_U(L(s), L(c))
main(int argc, char **argv)
{
	printf("percent_used(%d, %d) = %d, was %d\n", TestIt(0, 577));
	printf("percent_used(%d, %d) = %d, was %d\n", TestIt(1, 577));
	printf("percent_used(%d, %d) = %d, was %d\n", TestIt(288, 577));
	printf("percent_used(%d, %d) = %d, was %d\n", TestIt(292, 577));
	printf("percent_used(%d, %d) = %d, was %d\n", TestIt(576, 577));
	printf("percent_used(%d, %d) = %d, was %d\n", TestIt(577, 577));
	printf("percent_used(%d, %d) = %d, was %d\n", TestIt(0, 39428707));
	printf("llpercent_used(%lld, %lld) = %d, was %d\n",
	    llTestIt(17179868184L L, 17179869184L L));
	printf("llpercent_used(%lld, %lld) = %d, was %d\n",
	    llTestIt(1104L L, 17179869184L L));
	printf("llpercent_used(%lld, %lld) = %d, was %d\n",
	    llTestIt(0L L, 0L L));
}
#endif
