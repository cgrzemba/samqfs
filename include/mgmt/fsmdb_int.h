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

#ifndef _FSM_DB_PRIVATE_H
#define	_FSM_DB_PRIVATE_H

#include <sys/param.h>
#include <sys/types.h>
#include <rpc/xdr.h>
#include "mgmt/config/common.h"
#include "mgmt/private_file_util.h"	/* for restrict_t */
#include "pub/mgmt/sqm_list.h"	/* for sqm_lst_t */

/*
 *  Private header file for IPC communications with the File System
 *  metrics database.
 */

/*
 * Location of database, door and lock
 */
#define	fsmdbdir	VAR_DIR"/fsmdb"
#define	fsmdbdoordir	"/var/run/samfs"
#define	fsmdbdoor	fsmdbdoordir"/fsmdb_door"
#define	fsmdbdoorlock	fsmdbdoor"_lock"
#define	fsmdbproc	"fsmdb"


/* assign ids and arg types to functions for door call and dispatch */
#define	IMPORT_SAMFS		1
#define	IMPORT_LIVEFS		2
#define	CHECK_VSN		3
#define	GET_ALL_VSN		4
#define	GET_SNAPSHOT_VSN	5
#define	GET_FS_METRICS		6
#define	GET_FS_SNAPSHOTS	7
#define	DELETE_SNAPSHOT		8
#define	GET_SNAPSHOT_STATUS	9
#define	BROWSE_SNAPSHOT_FILES	10	/* details */
#define	LIST_SNAPSHOT_FILES	11	/* simple list */
#define	DELETE_FSDB_FILESYS	12	/* delete all for filesystem */

/*
 * Structure definitions for passing multiple arguments or multi-value
 * returns.
 */
typedef struct {
	char 		fsname[MAXPATHLEN+1];
	char 		snapshot[MAXPATHLEN+1];
} import_arg_t;

typedef struct {
	char  		fsname[MAXPATHLEN+1];
	int32_t		rptType;
	time_t		start;
	time_t		end;
} fs_metric_arg_t;

typedef struct {
	char		fsname[MAXPATHLEN+1];
	char		snapshot[MAXPATHLEN+1];
	char		startDir[MAXPATHLEN+1];
	char		startFile[MAXPATHLEN+1];
	restrict_t	restrictions;
	uint32_t	which_details;
	int32_t		howmany;
	boolean_t	includeStart;	/* include startFile in return */
} flist_arg_t;

typedef struct {
	uint32_t	status;
	uint32_t	count;		/* also used for size returns */
} fsmdb_ret_t;

/*
 * Structures to use when sending arguments through the door
 */
typedef struct {
	uint32_t			task;
	union {
		import_arg_t		i;
		fs_metric_arg_t		m;
		char			c[MAXPATHLEN+1];
		uint16_t		s;
		flist_arg_t		l;
	} 				u;
} fsmdb_door_arg_t;

/* function definitions for code shared between fsmdb and fsmdb_api */

/* translates a list of filedetails_t structures */
bool_t
xdr_filedetails_list(XDR *xdrs, sqm_lst_t *objp);

#endif	/* _FSM_DB_PRIVATE_H */
