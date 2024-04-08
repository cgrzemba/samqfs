/*
 *	parsetabs.c - utility for parsing space delimited files
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

#include <ctype.h>
#include <string.h>
#include <stdio.h>

/*
 *	parsetabs - Parse string into fields.
 *
 *	Description:
 *	    Parsetabs parses a given string into an argument vector
 *	    similar to what is expected by getopt(3). Valid separators
 *	    are space characters defined by isspace(3) and the comma.
 *
 *	On entry:
 *	    string = The string to parse.
 *	    argmax = The maximum number of arguments that can be
 *		     held in the argument vector (argv).
 *
 *	On return:
 *	    argv   = The argument vector.
 *	    argc   = The argument count.
 *
 *	Returns:
 *	    TRUE, if insufficient fields present.
 */
int
parsetabs(
	char	*string,	/* Input string to parse	*/
	char 	*argv[],	/* Argument vector		*/
	int		*argc,		/* Argument count		*/
	int		argmax)		/* Maximum argument count	*/
{
	char		*s;

	*argc = 0;

	for (s = string ; ; s++) {
		if (*s == '\0') {
			return (*argc != argmax);
		}
		if (*s == '\t') {
			continue;
		}

		argv[*argc] = s;
		(*argc)++;

		for (; ; s++) {
			if (*s == '\0') {
				return (*argc != argmax);
			}
			if (*s == '\t') {
				*s = '\0';
				break;
			}
		}
		if (*argc == argmax) {
			return (0);
		}
	}
}
