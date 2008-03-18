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
#pragma ident	"$Revision: 1.15 $"

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <stddef.h>
#include "pub/mgmt/error.h"
#include "mgmt/util.h"
#include "pub/mgmt/restore.h"
#include "mgmt/restore_int.h"

#define	BUFFSIZ		MAXPATHLEN



/*
 * Parse_frequency. <Number of seconds since 1970>"P"<Number of seconds>
 * The start time is to allow specifying something like "every 24 hours,
 * at 3:30am". Note that zero time is 1970-january-1, midnight *GMT*
 */

int
parse_frequency(char *ptr, void *bufp)
{
	period_t *frequency;
	char buffer[BUFFSIZ];
	frequency = (period_t *)bufp;

	sscanf(ptr, "%ld%[Pp]%ld", &frequency->start, buffer,
	    &frequency->interval);

	return (0);
}

/*
 * Parse_keepfor:  Expects a string consisting of
 * a number followed by a character denoting the unit,
 * where unit is one of:
 *	D = days
 *	W = weeks
 *	M = months
 *	Y = years
 *
 * A number value of 0 indicates unset.
 *
 */
int
parse_keepfor(char *ptr, void *bufp)
{
	keep_for_t	*retainfor;
	int		len = 0;

	retainfor = (keep_for_t *)bufp;

	if (!ISNULL(ptr)) {
		len = strlen(ptr);
	}

	if (len < 2) {
		return (-1);
	}

	switch (ptr[len-1]) {
		case 'D':
		case 'W':
		case 'M':
		case 'Y':
			retainfor->unit = ptr[len-1];
			break;
		default:
			/* invalid unit */
			retainfor->unit = '\0';
			return (-1);
			break;		/* NOTREACHED */
	}

	/*
	 * note that atoi() will stop converting if it hits a
	 * non-numeric, so it does just what we want here.
	 */
	retainfor->val = atoi(ptr);

	if (retainfor->val < 1) {
		memset(retainfor, 0, sizeof (keep_for_t));
		return (-1);
	}

	return (0);
}

#define	csd_off(name) (offsetof(csdbuf_t, name))


static parsekv_t csdtokens[] = {
	{"location",	csd_off(location),	parsekv_string_1024},
	{"prescript",	csd_off(prescript),	parsekv_string_1024},
	{"postscript",	csd_off(postscript),	parsekv_string_1024},
	{"names",	csd_off(names),		parsekv_string_1024},
	{"compress",	csd_off(compress),	parsekv_string_1024},
	{"logfile",	csd_off(logfile),	parsekv_string_1024},
	{"frequency",	csd_off(frequency),	parse_frequency},
	{"retainfor",	csd_off(retainfor),	parse_keepfor},
	{"name_prefix",	csd_off(nameprefix),	parsekv_string_1024},
	{"autoindex",	csd_off(autoindex),	parsekv_bool},
	{"disabled",	csd_off(disabled),	parsekv_bool},
	{"prescrfatal", csd_off(prescrfatal),	parsekv_bool},
	{"excludedirs",	csd_off(excludedirs),	parsekv_dirs},
	{"retention",	csd_off(retention),	parsekv_string_1024},
	{"",		0,			NULL}
	};


int
parse_csdstr(char *str, csdbuf_t *csdbuf, int csdsize) {
	int rval;

	memset(csdbuf, 0, csdsize);
	csdbuf->prescrfatal = 1;

	rval = parse_kv(str, &csdtokens[0], csdbuf);
	if (rval)
		return (rval);

	if (
		(strlen(csdbuf->location) == 0) ||
		(strlen(csdbuf->names) == 0) ||
		(csdbuf->frequency.interval == 0)) {
		return (samrerr(SE_MISSINGKEY, str));
	}

	if (strcasecmp(csdbuf->compress, "none") == 0) {
		csdbuf->compress[0] = '\0';
	}

	return (rval);
}
