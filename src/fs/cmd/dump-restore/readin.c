/*
 *	readin.c - utility for reading input from files
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
 * or https://illumos.org/license/CDDL.
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


#pragma ident "$Revision: 1.2 $"

#include	<stdio.h>

int	readin_ln = 0;		/* Line number count		*/

/*
 *	readin - Read input from file.
 *
 *
 *	Description:
 *	    Readin reads a line from the specified input.
 *
 *	On entry:
 *	    f	= File descriptor to read from.
 *	    s	= The character buffer to return the next line of input
 *		  through.
 *	    n	= The maxium number of characters to read into s.
 *
 *	Returns:
 *	    Zero unless an EOF was encountered, then returns EOF.
 */
int
readin(
	FILE *f,	/* File to read from		*/
	char s[],	/* String buffer		*/
	int n)		/* Number of characters to read	*/
{
	int	i, c;

	s[0] = '\0';

	readin_ln++;

	for (i = 0; i < n; i++) {
		if ((c = getc(f)) == EOF) {
			s[i] = '\0';
			return ((i == 0) ? EOF : 0);
		}
		s[i] = (char)c;
		if (c == '\n') {
			s[i] = '\0';
			return (0);
		}
	}
	while (!(c != '\n' || c != EOF)) {
		c = getc(f);
	}
	return (0);
}
