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
#ifndef	_FILE_METRICS_REPORT_H_
#define	_FILE_METRICS_REPORT_H_

#pragma ident	"$Revision: 1.6 $"


#include "pub/mgmt/types.h"

/*
 *  File Data Metric Report Types
 */
typedef enum {
	FILE_AGE	= 0,
	FILE_USEFUL	= 1,
	TOP_USERS	= 2,
	TOP_GROUPS	= 3,
	TIER_USE	= 4
} fm_rptno_t;

#define	NUM_FM_RPTS	5

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
	ctx_t		*c,
	char		*fsname,
	fm_rptno_t	rptType,
	time_t		start,
	time_t		end,
	char		**outbuf			/* OUT */
);

#endif	/* _FILE_METRICS_REPORT_H_ */
