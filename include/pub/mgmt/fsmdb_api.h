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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */
#ifndef	_FSMDB_API_H_
#define	_FSMDB_API_H_

#pragma ident	"$Revision: 1.13 $"


/*
 *  API for the FSM File System Database - libfsmdb.so
 */

#include <sys/types.h>
#include "mgmt/restore_int.h"
#include "pub/mgmt/sqm_list.h"
#include "mgmt/private_file_util.h"

/*
 *  Function:  import_from_samfsdump()
 *
 *  Imports file information from a samfsdump file and stores for
 *  later retrieval and processing.  Call is asynchronous, client
 *  cannot wait until import is complete.
 *
 *  Parameters:
 *	fsname		File System Name - /dev/xxx or SAM/QFS name
 *	snapfile	Path to the samfsdump file to be imported
 *	taskid		To check status of long-running jobs.  If NULL,
 *			no task id returned.
 *
 *  Return values:
 *	0		Success
 *	ENOENT		snapfile could not be found
 *	EACCES		snapfile could not be read
 *	EINVAL		Either fsname or snapfile was not specified
 *	ENODEV		File system name is invalid
 *	EALREADY	Request already in progress
 */
int
import_from_samfsdump(char *fsname, char *snapfile, uint64_t *taskid);

/*
 *  Function:  import_from_mountedfs()
 *
 *  Imports file information by reading through a mounted file system.
 *  Call is asynchronous, client cannot wait until import is complete.
 *
 *  Parameters:
 *	fsname		File System Name - /dev/xxx or SAM/QFS name
 *      mountpt         Filesystem mount point
 *	taskid		To check status of long-running jobs.  If NULL,
 *			no task id returned.
 *
 *  Return values:
 *	0		Success
 *	EACCES		File system is not mounted or could not be read
 *	EINVAL		fsname was not specified
 *	ENODEV		File system name is invalid
 */
int
import_from_mountedfs(char *fsname, char *mountpt, uint64_t *taskid);

/*
 *  Function:  check_vsn_inuse()
 *
 *  Checks if there are any references to a VSN in the database.
 *
 *  Parameters:
 *	vsn		VSN name
 *
 *  Return values:
 *	0		Not in use
 *	1		In use
 */
int
check_vsn_inuse(char *vsn);

/*
 *  Function:  get_used_vsns()
 *
 *  Returns an array of VSNs with references in the database.
 *
 *  Parameters:
 *	class		Media type class (DT_TAPE, DT_DISK, etc.) as defined
 *			in pub/devstat.h
 *	nvsns		Number of VSNs in array
 *	vsns		Array of VSN names.  If NULL, only the count will
 *			be returned in nvsns.
 *
 *  IMPORTANT:  Array must be freed with free_fsmdb_result()
 *
 *  Return values:
 *	0		Success
 *	ENOMEM		Not enough memory
 *	EINVAL		Either class was invalid or nvsns was NULL.
 */
int
get_used_vsns(uint16_t class, uint32_t *nvsns, char ***vsns);

/*
 *  Function:  get_vsns_in_snapshot()
 *
 *  Returns an array of VSNs that are referenced by a specific snapshot.
 *
 *  Parameters:
 *	fsname		SAM filesystem name
 *	snapshot	Snapshot name.  If not a fully-qualified path,
 *			must be relative to the directory specified in
 *			the File System Manager Snapshot Schedule.
 *	nvsns		Number of VSNs in array
 *	vsns		Array of VSN names.
 *
 *  IMPORTANT:  Array must be freed with free_fsmdb_result()
 *
 *  Return Values:
 *	0		Success
 *	ENOMEM		Not enough memory
 *	ENODEV		Filesystem name is invalid
 *	ENOENT		Snapshot does not exist
 *	EINVAL		nvsns or vsns is NULL
 */
int
get_vsns_in_snapshot(char *fsname, char *snapshot, uint32_t *nvsns,
	char ***vsns);

/*
 *  Function:  get_fs_metrics()
 *
 *  Returns XML file system metrics reports.
 *
 *  Parameters:
 *	fsname		Filesystem name.  If NULL, metrics will
 *			be returned for all filesystems for which
 *			metrics are being kept.
 *	rptType		Type of metric report.  If -1, all reports
 *			will be returned.
 *	start		Start date in seconds from the epoch.  If 0,
 *			the date range will start from the earliest
 *			available snapshot.
 *	end		End date in seconds from the epoch.  If 0,
 *			the date range will end with the last available
 *			snapshot.
 *	buffer		buffer for report data.
 *
 *	To get all metrics for all dates:
 *		get_fs_metrics(NULL, -1, 0, 0, buffer);
 *
 *  Return Values:
 *	0		Success
 *	ENOMEM		Not enough memory
 *	ENODEV		Filesystem name is invalid.
 *	EINVAL		Buffer is NULL
 */
int
get_fs_metrics(char *fsname, int32_t rptType, time_t start, time_t end,
	char **buffer);

/*
 *  Function:  get_fs_snapshots()
 *
 *  Returns a list of snapshots in the database for a given file system.
 *
 *  Parameters:
 *	fsname		Filesystem name.
 *	nsnaps		Number of snapshots in DB for fsname
 *	snapnames	Array of snapshot names
 *
 *  IMPORTANT:  Array must be freed with free_fsmdb_result()
 *
 *  Return Values:
 *	0		Success
 *	ENOMEM		Not enough memory
 *	ENODEV		Filesystem name is invalid
 *	EINVAL		fsname or snapnames or nsnaps is NULL.
 */
int
get_fs_snapshots(char *fsname, uint32_t *nsnaps, char ***snapnames);

/*
 *  Function:  free_fsmdb_results()
 *
 *  Frees array results allocated by fsmdb_api functions
 *
 *  Parameters:
 *	results		Result returned by fsmdb_api function
 *
 *  Return values:
 *
 *	none
 *
 */
void
free_fsmdb_results(char **result);

/*
 *  Function:  delete_fsmdb_snapshot()
 *
 *  Imports file information from a samfsdump file and stores for
 *  later retrieval and processing.  Call is asynchronous, client
 *  cannot wait until import is complete.
 *
 *  Parameters:
 *	fsname		File System Name - /dev/xxx or SAM/QFS name
 *	snapshot	Name of the snapshot to be deleted
 *	taskid		To check status of long-running jobs.  If NULL,
 *			no task id returned.
 *
 *  Return values:
 *	0		Success
 *	EINVAL		Either fsname or snapshot was not specified
 *	ENODEV		File system name is invalid
 *	EALREADY	Request already in progress
 *	ENOTCONN	Server not running
 */
int
delete_fsmdb_snapshot(char *fsname, char *snapshot, uint64_t *taskid);

/*
 *  Function:  get_snapshot_status()
 *
 *  Returns an array of datastructures describing each snapshot registered
 *  in the database for a given filesystem.
 *
 *  Parameters:
 *	fsname		File System Name
 * 	snapArray	Output - array of snapspec_t structures
 *	count		Output - length of returned array.
 *
 *	0		Success
 *	EINVAL		fsname, snapArray or count was not specified
 *	ENOTCONN	Server not running
 */
int
get_snapshot_status(char *fsname, snapspec_t **snapArray, uint32_t *count);

/*
 *  Function:  list_snapshot_files()
 *
 *  Returns a list of filedetails_t structures.  The results list must
 *  be created by the caller.
 */
int
list_snapshot_files(
	char		*fsname,
	char		*snapshot,
	char		*startDir,
	char		*startFile,
	restrict_t	restrictions,
	uint32_t	which_details,
	int32_t		howmany,
	boolean_t	includeStart,	/* include startFile in return */
	uint32_t	*morefiles,	/* OUT */
	sqm_lst_t	*results);	/* OUT */

/*
 *  Function:  delete_fs_from_db()
 *
 *  This function removes the database files associated with a given
 *  filesystem.  Data cannot be recovered.
 *
 *  Parameters:
 *	fsname		File System Name - /dev/xxx or SAM/QFS name
 *
 *  Return values:
 *	0		Success
 *	EINVAL		fsname was not specified
 *	ENOTCONN	Server not running
 */
int
delete_fs_from_db(char *fsname);


/*
 *  To Be Done:
 *
 *	remove metrics from db
 */

#endif	/* _FSMDB_API_H_ */
