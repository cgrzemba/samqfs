/*
 * sam_attrtoa.c - Convert SAM-FS attributes to ASCII.
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

#pragma ident "$Revision: 1.12 $"


/* POSIX headers. */
#include <sys/types.h>

/* SAM-FS headers. */
#include "lib.h"
#include "stat.h"


/*
 * Return attributes in ASCII from attribute field.
 * String returned.
 */
char *
sam_attrtoa(
	int attr,	/* Attributes field */
	char *string)	/* If not NULL, place string here */
{
	static char s[24];

	if (string == NULL)  string = s;
	string[0] = (SS_ISOFFLINE(attr)) ? 'O' : '-';
	if (SS_ISDAMAGED(attr))  string[0] = 'E';

	/* archive */
	string[1] = (SS_ISARCHIVE_N(attr)) ? 'n' : '-';
	string[2] = (SS_ISARCHIVE_A(attr)) ? 'a' : '-';
	string[3] = (SS_ISARCHIVE_R(attr)) ? 'r' : '-';

	/* release */
	string[4] = (SS_ISRELEASE_N(attr)) ? 'n' : '-';
	string[5] = (SS_ISRELEASE_A(attr)) ? 'a' : '-';
	string[6] = (SS_ISRELEASE_P(attr)) ? 'p' : '-';

	/* stage */
	string[7] = (SS_ISSTAGE_N(attr)) ? 'n' : '-';
	string[8] = (SS_ISSTAGE_A(attr)) ? 'a' : '-';
	string[9] = '-';

	string[10] = ' ';

	/* checksum & worm attributes */
	string[11] = (SS_ISCSGEN(attr)) ? 'g' : '-';
	string[12] = (SS_ISCSUSE(attr)) ? 'u' : '-';
	string[13] = (SS_ISCSVAL(attr)) ? 'v' : '-';
	string[14] = (SS_ISWORM(attr)) ? 'w' : '-';
	string[15] = (SS_ISREADONLY(attr)) ? 'R' : '-';
	string[16] = ' ';

	/* segment attributes */
	string[17] = (SS_ISSEGMENT_A(attr)) ? 's' : '-';
	if (SS_ISSEGMENT_F(attr))  {
		string[18] =  'I';
	} else if (SS_ISSEGMENT_S(attr))  {
		string[18] =  'S';
	} else string[18] = '-';
		string[19] = '\0';

	return (string);
}
