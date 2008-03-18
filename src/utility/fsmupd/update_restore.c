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
#pragma ident	"$Revision: 1.5 $"

#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include "pub/mgmt/restore.h"
#include "mgmt/restore_int.h"
#include "mgmt/util.h"
#include "pub/mgmt/task_schedule.h"

/*
 *  Utility function to update snapshot schedules from csd.cmd
 *  to the new task schedule format.
 */
int
csd_to_task(void)
{
	int		st;
	FILE		*fp = NULL;
	int		len = sizeof (csdbuf_t) + MAXPATHLEN + 1;
	char		buf[len];
	char		*bufp;
	char		*namep;
	csdbuf_t	csd;
	snapsched_t	sched;

	fp = fopen(CSDFILENAME, "r");
	if (fp == NULL) {
		if (errno == ENOENT) {
			/* nothing to do */
			return (0);
		}
		return (-1);
	}

	st = backup_cfg(CSDFILENAME);
	if (st != 0) {
		/* what should happen on failure? */
		return (st);
	}

	while ((fgets(buf, len, fp)) != NULL) {
		bufp = buf;

		while (isspace(*bufp)) {
			bufp++;
		}

		if ((*bufp == '#') || (*bufp == '\0')) {
			continue;
		}

		/* first part of the csd entry is the fsname */
		namep = bufp;

		/* find the end of the name and terminate */
		while (!(isspace(*bufp))) {
			bufp++;
		}

		*bufp = '\0';
		bufp++;

		while (isspace(*bufp)) {
			bufp++;
		}

		st = parse_csdstr(bufp, &csd, sizeof (csd));
		if (st != 0) {
			/* bad entry, skip it */
			st = 0;
			continue;
		}

		/* translate to new structure */
		(void) memset(&sched, 0, sizeof (snapsched_t));
		(void) strlcpy(sched.id.fsname, namep,
		    sizeof (sched.id.fsname));
		(void) strlcpy(sched.location, csd.location,
		    sizeof (sched.location));
		(void) strlcpy(sched.namefmt, csd.names,
		    sizeof (sched.namefmt));
		(void) strlcpy(sched.prescript, csd.prescript,
		    sizeof (sched.prescript));
		(void) strlcpy(sched.postscript, csd.postscript,
		    sizeof (sched.postscript));
		(void) strlcpy(sched.logfile, csd.logfile,
		    sizeof (sched.logfile));
		sched.autoindex = csd.autoindex;
		sched.disabled = csd.disabled;
		sched.prescriptFatal = csd.prescrfatal;

		(void) memcpy(&(sched.excludeDirs), &(csd.excludedirs),
		    sizeof (sched.excludeDirs));

		if (strncmp(csd.compress, "gzip", 4) == 0) {
			sched.compress = 1;
		} else if (strcmp(csd.compress, "compress") == 0) {
			sched.compress = 2;
		}

		(void) cftime(sched.starttime, "%Y%m%d%H%M",
		    &(csd.frequency.start));

		(void) snprintf(sched.periodicity, sizeof (sched.periodicity),
		    "%ds", csd.frequency.interval);

		if (csd.retainfor.val != 0) {
			if (csd.retainfor.unit == 'D') {
				csd.retainfor.unit = 'd';
			} else if (csd.retainfor.unit == 'W') {
				csd.retainfor.unit = 'w';
			} else if (csd.retainfor.unit == 'Y') {
				csd.retainfor.unit = 'y';
			}
			(void) snprintf(sched.duration, sizeof (sched.duration),
			    "%d%c", csd.retainfor.val, csd.retainfor.unit);
		}

		/* translate back into a string */
		st = snapsched_to_string(&sched, buf, len);

		if (st != 0) {
			/* yuck, should probably warn someone */
			continue;
		}

		st = set_task_schedule(NULL, buf);
		if (st != 0) {
			break;
		}
	}

	/* if all is well, remove the old csd.cmd file */
	if (st == 0) {
		(void) unlink(CSDFILENAME);
	}

	return (st);
}
