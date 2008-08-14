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
#pragma ident "$Revision: 1.1 $"

#include <ctype.h>
#include <stdlib.h>

/*
 * ----	atobytes - Convert string to number of bytes.
 *
 *	Description:
 *	    Atobytes converts the specified string to the number of
 *	    bytes.  The string is assumed to be a numeric string
 *	    suffixed by an optional 'k', 'm', or 'g' to designate
 *	    the units kilo, mega, or giga.  If none is specified,
 *	    the units are assumed to be bytes.  See atoi(3) for input
 *	    scan rules.
 *
 *	On entry:
 *	    s	= The string to convert.
 *
 *	Returns:
 *	    The number of bytes.
 */
int atobytes(
	char *s) /* Input string */
{
	unsigned int n;
	char c;

	for (; isspace((int)*s); s++) {
		;
	}

	for (n = 0; ; s++) {
		c = tolower(*s);
		if (!isdigit((int)c)) {
			break;
		}
		n = n*10 + (c-'0');
	}
	switch (c) {
	case 'k':
		n *= 1024;
		break;
	case 'm':
		n *= 1024*1024;
		break;
	case 'g':
		n *= 1024*1024*1024;
		break;
	}
	return (n);
}
