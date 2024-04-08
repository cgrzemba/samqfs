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

#ifndef	_FSM_FILE_METRICS_H_
#define	_FSM_FILE_METRICS_H_

#pragma ident   "$Revision: 1.13 $"

#include <sys/types.h>
#include "mgmt/fsmdb.h"
#include "pub/mgmt/file_metrics_report.h"

/*
 *  file_metrics.h
 *
 *  Contains datastructures, defines, and public function definitions for
 *  generating audit reports from file information already stored in
 *  the audit databases.  See fsmdb.h for descriptions of the data stored.
 *
 */

/*
 *  For auditing file size aggregations per snapshot view
 *  id is used to represent the size 'bucket' and is kept as
 *	   0	(0)
 *	< 1K	(1)
 *	< 8K	(8)
 *	(...) multiples of 8K
 *
 *  This structure is also used for analyzing space used for
 *  specific users or groups (or even directories).  id is
 *  different for each report type:
 *
 *	top10 Users	uid
 *	top10 Groups	gid
 */
typedef struct {
	uint64_t	id;		/* as in comment above */
	uint64_t	count;		/* number of files in this range */
	uint64_t	total_size;	/* bytes used by this id */
	uint64_t	total_osize;	/* online size used by this id */
} agg_size_t;

/*
 *  For auditing functional age, file age
 *
 *  To calculate percentages for display, count up the totals from
 *  each of the age_aud_t structures and divide by the individual.
 *
 *  total_osize will always equal total_size for non-SAM file systems.
 */
typedef struct {
	int32_t		start;		/* start of age range, in days */
	int32_t		end;		/* top of age range, in days */
					/* -1 means no top end */
	uint64_t	count;		/* number of files represented */
	uint64_t	total_size;	/* bytes consumed by files in range */
	uint64_t	total_osize;	/* online size of files in range */
} file_age_t;

/*
 *  For auditing storage tiers.  Note that multiple copies of the
 *  same file on the same tier will be counted individually.
 *  Also note that storage tier is relative -- we're only basing it
 *  on media type, not on copy number because copy number is variable
 *  depending on archive policies.  A future enhancement might be to
 *  evaluate file usage/tier/archive policy or class.
 *
 *  Expected values for the tiers will be:
 * 	disk cache		reports online size only
 *	disk archive
 *	tape archive
 * 	STK5800 archive
 */
#if _LONG_LONG_ALIGNMENT == 8
#pragma pack(4)
#endif
typedef struct {
	uint64_t	used;		/* bytes used */
	uint32_t	mtype;		/* media type */
} tier_usage_t;
#if _LONG_LONG_ALIGNMENT == 8
#pragma pack()
#endif

/* use TIER_SIZE instead of tier_usage_t to avoid issues with packing & size */
#define	TIER_SIZE 12

/* Public Function Prototypes */

/*
 *  gather_snap_metrics()
 *
 *  For a given snapshot ID, analyzes the file data to determine:
 *
 *	file age distribution	(difference between now and last modified time)
 *	file functional life	(difference between last access & last modified)
 *	top 10 users
 *	top 10 groups
 * 	size used by each archive type and the active disk cache
 *
 *  Called during each snapshot import.  Results are stored
 *  in a separate database for quick retrieval by clients.
 *
 *  Metrics are generated as data is being input.  While this may slow
 *  down the import slightly, it's faster than processing the same data
 *  twice.
 */
int
gather_snap_metrics(
	fs_db_t *fsdb,			/* in */
	fsmsnap_t *snapinfo,		/* in */
	filinfo_t *finfo,		/* in */
	filvar_t *filvar,		/* in */
	void**	rptArg,			/* in/out, opaque */
	int	*media,			/* in */
	void**	rptRes			/* in/out, opaque */
);

/*
 *  finish_snap_metrics()
 *
 *  Post-processes any data, if needed, and stores in the metrics database.
 */
int
finish_snap_metrics(
	fs_db_t *fsdb,			/* in */
	fsmsnap_t *snapinfo,		/* in */
	void**		rptArg,			/* in/out, opaque */
	void**		rptRes			/* in/out, opaque */
);

/*
 *  free_metrics_results()
 *
 *  Post-gathering cleanup
 */
void
free_metrics_results(
	void**		rptArg,			/* in/out, opaque */
	void**		rptRes			/* in/out, opaque */
);

/*
 *  generate_xml_fmrpt()
 *
 *  For each specific type of metric report, generate the XML report output
 *  to be exported to the client.
 */
int generate_xml_fmrpt(fs_entry_t *fsent, FILE *outf, fm_rptno_t which,
	time_t start, time_t end);

#endif /* _FSM_FILE_METRICS_H_ */
