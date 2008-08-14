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

#include	<sys/types.h>
#include	<strings.h>
#include	<time.h>

static char timeb[60]; /* Time string buffer */

/*
 * 	logtime1 - Format time as: mon dd hh:mm:ss
 *
 *	Descrition:
 *	    Return current time as string in ctime(3) format
 *	    with the day of the week and year removed.
 *
 *	Returns:
 *	    Returns pointer to time string.
 */
char *
logtime1(void)
{
	time_t clock;	/* Current time (binary) */
	char *p; 		/* Pointer to time string */

	clock = time(0l);
	p = ctime(&clock);
	strcpy(timeb, p);
	p = timeb;
	*(p+19) = '\0';
	return (p+4);
}

/*
 * 	logtime2 - Format time as: yyyy/mm/dd hh:mm:ss
 *
 *	Descrition:
 *	    Return current time as string.
 *
 *	Returns:
 *	    Returns pointer to time string.
 */
char *
logtime2(void)
{
	time_t clock; /* Current time (binary)	*/
	struct tm *tmx;

	clock = time(0l);
	tmx = localtime(&clock);
	strftime(timeb, 20, "%Y/%m/%d %H:%M:%S", tmx);
	return (timeb);
}
