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

#ifndef	_RESTORE_INT_H_
#define	_RESTORE_INT_H_

#pragma ident	"$Revision: 1.5 $"

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <zlib.h>

/*
 * These states represent the state of a snapshot in the database.
 * As locked and compressed states are derived from the actual
 * samfsdump file, they are not part of this enum.
 */
typedef enum {
	UNINDEXED	= 0,
	INDEXED		= 1,
	METRICS		= 2,
	DAMAGED		= 3,
	UKNOWN		= 4
} snap_state_t;

typedef struct {
	gzFile		fildmp;		/* gzFile struct ptr for snapshot */
	FILE		*logfil;	/* open Log file */
	int		csdversion;	/* samfsdump format version */
	boolean_t	byteswapped;	/* If snapshot is other-endian */
	time_t		snaptime;	/* Date snapshot was taken */
	uint64_t	numEntries;	/* Number of entries in snapshot */
	snap_state_t	snapState;	/* Current state of snapshot */
	char		snapname[MAXPATHLEN + 1];
} snapspec_t;

/* structures to hold snapshot schedule parameters */
typedef struct {
	char		fsname[58];
	char		startDir[MAXPATHLEN];
} snapsched_id_t;

typedef struct {
	snapsched_id_t	id;
	char		location[MAXPATHLEN];
	char		namefmt[MAXPATHLEN];
	char		prescript[MAXPATHLEN];
	char		postscript[MAXPATHLEN];
	char		logfile[MAXPATHLEN];
	char		starttime[16];
	char		periodicity[16];
	char		duration[16];
	char		excludeDirs[10][MAXPATHLEN];
	int32_t		compress;	/* 0 = none, 1 = gzip, 2 = compress */
	boolean_t	autoindex;
	boolean_t	disabled;
	boolean_t	prescriptFatal;
} snapsched_t;

/* function to turn a snapsched_t into a string */
int snapsched_to_string(snapsched_t *sched, char *buf, size_t buflen);

/* translate key/value pairs to snapsched_t */
int parse_snapsched(char *str, snapsched_t *sched, int len);
int parsekv_dirs(char *ptr, void * bufp);


#endif	/* _RESTORE_INT_H_ */
