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
#ifndef _JOB_HISTORY_H
#define	_JOB_HISTORY_H

#pragma ident	"$Revision: 1.11 $"

/*
 * job_history.h - SAM-FS API fault handling API and data structures.
 */

#include <sys/types.h>

#include "sam/types.h"

#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"

#define	MAXLINE 1024+256
#define	ARLOG_FLD_LEN   512
#define	MAX_FS	256
#define	CMD_LEN 512


/* an enum for job types */

typedef enum job_type {
	ARCHIVE,
	RELEASE,
	RECYCLE,
	LOGROT,
	SAMFSDUMP
} job_type_t;

/*
 * Each jobs history info struct has some
 * common fields which are grouped as a
 * header.
 */
typedef struct job_hdr {
	int			jobID;
	int			jobType;	/* Archiver, Releaser etc. */
}job_hdr_t;


/*
 * This could serve as the new structure
 * for a Job Type.
 */
typedef struct job_hist {
	job_hdr_t	job_hdr;   /* The common header */
	time_t		lastRan;	/* when job last ran */
	union {
		uname_t	fsName;
		upath_t	fileName;
	} fn;
} job_hist_t;


/* return job history for a single FS */
int get_jobhist_by_fs(
	ctx_t *ctx,
	uname_t fsname,
	job_type_t job_type,
	job_hist_t **job_hist);

/* return a list of job histories for all FS */
int get_all_jobhist(ctx_t *ctx, job_type_t job_type, sqm_lst_t **job_list);

#endif	/* _JOB_HISTORY_H */
