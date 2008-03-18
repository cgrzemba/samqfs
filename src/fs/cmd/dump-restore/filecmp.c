/*
 * filecmp.c  - Compare file name strings
 *
 *	Description:
 *		Compare two pathname strings to determine if they match
 *		if one of the strings is a path substring of the other.
 *
 *	On entry:
 *		a, b  The pathname strings to compare.
 *
 *	Returns:
 *		0, if compare failed.
 *		1, if exact match.
 *		2, if a is a subpath of b.
 *		3, if b is a subpath of a.
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


#pragma ident "$Revision: 1.5 $"


#define	dprintf(a, b, c)
/* #define	dprintf(a, b, c) printf(a, b, c) */

int
filecmp(char *a, char *b)
{
	dprintf("filecmp(%s,%s) == ", a, b);

	while (*a != '\0' && *b != '\0') {
		if (*a != *b) {
			dprintf("0\n", 0, 0);
			return (0);
		}
		a++;
		b++;
	}
	if (*a == '\0' && *b == '\0') {
		dprintf("1\n", 0, 0);
		return (1);		/* exact	 */
	}
	if (*a == '\0' && *b == '/') {
		dprintf("2\n", 0, 0);
		return (2);		/* a subpath	 */
	}
	if (*a == '/' && *b == '\0') {
		dprintf("3\n", 0, 0);
		return (3);		/* b subpath	 */
	}
	if ((*a == '\0' && (b[0] == '/' && b[1] == '.' && b[2] == '\0')) ||
	    (*a == '\0' && (a[0] == '/' && a[1] == '.' && a[2] == '\0'))) {
		dprintf("1\n", 0, 0);
		return (1);		/* "exact" */
	}
	dprintf("0\n", 0, 0);
	return (0);			/* no match	 */
}
