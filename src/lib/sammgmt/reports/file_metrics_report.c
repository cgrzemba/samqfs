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
#pragma ident "$Revision: 1.9 $"

#include <errno.h>
#include <string.h>

#include "pub/mgmt/file_metrics_report.h"
#include "pub/mgmt/fsmdb_api.h"
#include "mgmt/util.h"

/*  Public functions to return File Metric data */

/*
 *  get_file_metrics_report()
 *
 *  Retrieves file metric XML data for a specific file system.
 *
 *  Parameters:
 *
 *	fsname		filesystem name
 *	rptType		What metrics to return - defined in
 *			file_metrics_report.h
 *	start		Date (in seconds) of first snapshot to be
 *			reflected in the report.  Use 0 for no start date.
 *	end		Date (in seconds) of last snapshot to be
 *			returned in the report.  Use 0 for no end date.
 *	outbuf		Report data.  Data will be malloced and must
 *			be freed by the caller.
 *
 * 	If no start or end date, data from all available snapshots will
 *	be returned.  If either are specified, the date the snapshot was
 *	taken must fall between start & end.
 */
int
get_file_metrics_report(
	ctx_t		*c,			/* ARGSUSED */
	char		*fsname,
	fm_rptno_t	rptType,
	time_t		start,
	time_t		end,
	char		**outbuf)			/* OUT */
{
	int	st;

	if (ISNULL(fsname, outbuf)) {
		return (-1);
	}

	if (!(0 < rptType < NUM_FM_RPTS)) {
		samerrno = SE_INVALID_REPORT_REQUESTED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    rptType);
		return (-1);
	}

	st = get_fs_metrics(fsname, rptType, start, end, outbuf);

	if (st != 0) {
		if (st == ENOTCONN) {
			samerrno = SE_FSMDB_CONNECT_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		} else if (st == ENOENT) {
			/* filesystem not in database */
			samerrno = SE_NO_METRICS_AVAILABLE;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		} else {
			samerrno = SE_FILE_METRIC_REPORT_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
			    "");
			strlcat(samerrmsg, strerror(st), MAX_MSG_LEN);
		}
		st = -1;
	}

	return (st);
}
