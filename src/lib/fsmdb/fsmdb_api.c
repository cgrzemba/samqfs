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
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <door.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/mman.h>
#include <libgen.h>
#include <time.h>
#include <note.h>	/* lint notes */
#include "pub/mgmt/fsmdb_api.h"
#include "mgmt/fsmdb_int.h"
#include "mgmt/config/common.h"
#include "mgmt/restore_int.h"
#include "pub/mgmt/sqm_list.h"

/*
 *  API for the FSM File System Database - libfsmdb.so
 */

/* static function prototypes */
static int open_temp_file(void);
static int get_mapped_results(int fd, char **outbuf);
static int call_fsmdb(door_arg_t *arg);

/* globals */


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
 *	ENOTCONN	Server not running
 */
int
import_from_samfsdump(char *fsname, char *snapfile, uint64_t *taskid)
{
	fsmdb_ret_t		ret;
	fsmdb_door_arg_t	arg;
	door_arg_t		d_arg;

	if ((fsname == NULL) || (snapfile == NULL)) {
		return (EINVAL);
	}

	arg.task = IMPORT_SAMFS;

	strlcpy(arg.u.i.fsname, fsname, sizeof (arg.u.i.fsname));
	strlcpy(arg.u.i.snapshot, snapfile, sizeof (arg.u.i.snapshot));

	/* TODO - support taskid's & nowait option */
	if (taskid != NULL) {
		*taskid = 0;
	}

	d_arg.data_ptr = (char *)&arg;
	d_arg.data_size = sizeof (fsmdb_door_arg_t);
	d_arg.desc_ptr = NULL;
	d_arg.desc_num = 0;
	d_arg.rbuf = (char *)&ret;
	d_arg.rsize = sizeof (fsmdb_ret_t);

	if ((call_fsmdb(&d_arg)) != 0) {
		/* couldn't connect. */
		return (ret.status);
	}

	if ((d_arg.data_ptr == NULL) || (d_arg.data_size == 0)) {
		/* something odd happened */
		return (EINVAL);
	}

	return (ret.status);
}

/*
 *  Function:  import_from_mountedfs()
 *
 *  Imports file information by reading through a mounted file system.
 *  Call is asynchronous, client cannot wait until import is complete.
 *
 *  Parameters:
 *	fsname		File System Name - /dev/xxx or SAM/QFS name
 *	mountpt		Filesystem mount point
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
import_from_mountedfs(char *fsname, char *mountpt, uint64_t *taskid)
{
	fsmdb_ret_t		ret;
	fsmdb_door_arg_t	arg;
	door_arg_t		d_arg;

	if ((fsname == NULL) || (mountpt == NULL)) {
		return (EINVAL);
	}

	arg.task = IMPORT_LIVEFS;

	strlcpy(arg.u.i.fsname, fsname, sizeof (arg.u.i.fsname));
	strlcpy(arg.u.i.snapshot, mountpt, sizeof (arg.u.i.snapshot));

	/* TODO - support taskid's & nowait option */
	if (taskid != NULL) {
		*taskid = 0;
	}

	d_arg.data_ptr = (char *)&arg;
	d_arg.data_size = sizeof (fsmdb_door_arg_t);
	d_arg.desc_ptr = NULL;
	d_arg.desc_num = 0;
	d_arg.rbuf = (char *)&ret;
	d_arg.rsize = sizeof (fsmdb_ret_t);

	if ((call_fsmdb(&d_arg)) != 0) {
		/* couldn't connect. */
		return (ret.status);
	}

	if ((d_arg.data_ptr == NULL) || (d_arg.data_size == 0)) {
		/* something odd happened */
		return (EINVAL);
	}

	return (ret.status);
}

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
 *	> 1		Another error
 */
int
check_vsn_inuse(char *vsn)
{
	fsmdb_ret_t		ret;
	fsmdb_door_arg_t	arg;
	door_arg_t		d_arg;

	if (vsn == NULL) {
		return (EINVAL);
	}

	arg.task = CHECK_VSN;

	strlcpy(arg.u.c, vsn, sizeof (arg.u.c));

	d_arg.data_ptr = (char *)&arg;
	d_arg.data_size = sizeof (fsmdb_door_arg_t);
	d_arg.desc_ptr = NULL;
	d_arg.desc_num = 0;
	d_arg.rbuf = (char *)&ret;
	d_arg.rsize = sizeof (fsmdb_ret_t);

	if ((call_fsmdb(&d_arg)) != 0) {
		/* couldn't connect. */
		return (ret.status);
	}

	if ((d_arg.data_ptr == NULL) || (d_arg.data_size == 0)) {
		/* something odd happened */
		return (EINVAL);
	}

	return (ret.status);
}

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
get_used_vsns(
	uint16_t	class,
	uint32_t	*nvsns,
	char		***vsns)
{
NOTE(ARGUNUSED(class))

	fsmdb_ret_t		ret;
	fsmdb_door_arg_t	arg;
	door_desc_t		desc;
	door_arg_t		d_arg;
	int			tmpfd = -1;
	int			st = 0;
	char			*buf = NULL;
	char			**arr;
	size_t			n;

	if ((nvsns == NULL) || (vsns == NULL)) {
		return (EINVAL);
	}

	/* open the temporary file for the XML data */
	tmpfd = open_temp_file();
	if (tmpfd == -1) {
		return (EIO);
	}

	arg.task = GET_ALL_VSN;

	desc.d_data.d_desc.d_descriptor = tmpfd;
	desc.d_attributes = DOOR_DESCRIPTOR;

	d_arg.data_ptr = (char *)&arg;
	d_arg.data_size = sizeof (fsmdb_door_arg_t);
	d_arg.desc_ptr = &desc;
	d_arg.desc_num = 1;
	d_arg.rbuf = (char *)&ret;
	d_arg.rsize = sizeof (fsmdb_ret_t);

	if ((call_fsmdb(&d_arg)) != 0) {
		/* couldn't connect. */
		close(tmpfd);
		return (ret.status);
	}

	if ((d_arg.data_ptr == NULL) || (d_arg.data_size == 0)) {
		/* something odd happened */
		st = EINVAL;
	} else {
		st = ret.status;
	}

	if (st == 0) {
		st = get_mapped_results(tmpfd, &buf);
	}

	if (buf != NULL) {
		arr = malloc(ret.count * sizeof (char *));
		if (arr == NULL) {
			st = ENOMEM;
		} else {
			n = bufsplit(buf, ret.count, arr);
		}
		*vsns = arr;
		*nvsns = n;
	}

	if (tmpfd > 0) {
		close(tmpfd);
	}

	return (st);
}

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
	char ***vsns)
{
	fsmdb_ret_t		ret;
	fsmdb_door_arg_t	arg;
	door_desc_t		desc;
	door_arg_t		d_arg;
	int			tmpfd = -1;
	int			st = 0;
	char			*buf = NULL;
	char			**arr = NULL;
	size_t			n;

	if ((nvsns == NULL) || (vsns == NULL)) {
		return (EINVAL);
	}

	*nvsns = 0;
	*vsns = NULL;

	/* open the temporary file for the XML data */
	tmpfd = open_temp_file();
	if (tmpfd == -1) {
		return (EIO);
	}

	arg.task = GET_SNAPSHOT_VSN;

	strlcpy(arg.u.i.fsname, fsname, sizeof (arg.u.i.fsname));
	strlcpy(arg.u.i.snapshot, snapshot, sizeof (arg.u.i.snapshot));

	desc.d_data.d_desc.d_descriptor = tmpfd;
	desc.d_attributes = DOOR_DESCRIPTOR;

	d_arg.data_ptr = (char *)&arg;
	d_arg.data_size = sizeof (fsmdb_door_arg_t);
	d_arg.desc_ptr = &desc;
	d_arg.desc_num = 1;
	d_arg.rbuf = (char *)&ret;
	d_arg.rsize = sizeof (fsmdb_ret_t);

	if ((call_fsmdb(&d_arg)) != 0) {
		/* couldn't connect. */
		close(tmpfd);
		return (ret.status);
	}

	if ((d_arg.data_ptr == NULL) || (d_arg.data_size == 0)) {
		/* something odd happened */
		st = EINVAL;
	} else {
		st = ret.status;
	}

	if (st == 0) {
		st = get_mapped_results(tmpfd, &buf);
	}

	if (buf != NULL) {
		arr = malloc(ret.count * sizeof (char *));
		if (arr == NULL) {
			st = ENOMEM;
		} else {
			n = bufsplit(buf, ret.count, arr);
		}
		*vsns = arr;
		*nvsns = n;
	}

	if (tmpfd > 0) {
		close(tmpfd);
	}

	return (st);
}

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
 *			(should we change this to either a FILE* or
 *			a pathname?)
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
	char **buffer)
{
	fsmdb_ret_t		ret;
	fsmdb_door_arg_t	arg;
	door_desc_t		desc;
	door_arg_t		d_arg;
	int			tmpfd = -1;
	int			st = 0;
	char			*buf = NULL;

	if ((fsname == NULL) || (buffer == NULL)) {
		return (EINVAL);
	}

	/* open the temporary file for the XML data */
	tmpfd = open_temp_file();
	if (tmpfd == -1) {
		return (EIO);
	}

	arg.task = GET_FS_METRICS;

	strlcpy(arg.u.m.fsname, fsname, sizeof (arg.u.m.fsname));
	arg.u.m.rptType = rptType;
	arg.u.m.start = start;
	arg.u.m.end = end;

	desc.d_data.d_desc.d_descriptor = tmpfd;
	desc.d_attributes = DOOR_DESCRIPTOR;

	d_arg.data_ptr = (char *)&arg;
	d_arg.data_size = sizeof (fsmdb_door_arg_t);
	d_arg.desc_ptr = &desc;
	d_arg.desc_num = 1;
	d_arg.rbuf = (char *)&ret;
	d_arg.rsize = sizeof (fsmdb_ret_t);

	if ((call_fsmdb(&d_arg)) != 0) {
		/* couldn't connect. */
		close(tmpfd);
		return (ret.status);
	}

	if ((d_arg.data_ptr == NULL) || (d_arg.data_size == 0)) {
		/* something odd happened */
		st = EINVAL;
	} else {
		st = ret.status;
	}

	if (st == 0) {
		st = get_mapped_results(tmpfd, &buf);
	}

	if (buf != NULL) {
		*buffer = buf;
	}

	if (tmpfd > 0) {
		close(tmpfd);
	}

	return (st);
}

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
get_fs_snapshots(char *fsname, uint32_t *nsnaps, char ***snapnames)
{
	fsmdb_ret_t		ret;
	fsmdb_door_arg_t	arg;
	door_desc_t		desc;
	door_arg_t		d_arg;
	int			tmpfd = -1;
	int			st = 0;
	char			*buf = NULL;
	char			**arr = NULL;
	size_t			n = 0;

	if ((nsnaps == NULL) || (snapnames == NULL)) {
		return (EINVAL);
	}

	/* open the temporary file for the XML data */
	tmpfd = open_temp_file();
	if (tmpfd == -1) {
		return (EIO);
	}

	arg.task = GET_FS_SNAPSHOTS;
	strlcpy(arg.u.c, fsname, sizeof (arg.u.c));

	desc.d_data.d_desc.d_descriptor = tmpfd;
	desc.d_attributes = DOOR_DESCRIPTOR;

	d_arg.data_ptr = (char *)&arg;
	d_arg.data_size = sizeof (fsmdb_door_arg_t);
	d_arg.desc_ptr = &desc;
	d_arg.desc_num = 1;
	d_arg.rbuf = (char *)&ret;
	d_arg.rsize = sizeof (fsmdb_ret_t);

	if ((call_fsmdb(&d_arg)) != 0) {
		/* couldn't connect. */
		close(tmpfd);
		return (ret.status);
	}

	if ((d_arg.data_ptr == NULL) || (d_arg.data_size == 0)) {
		/* something odd happened */
		st = EINVAL;
	} else {
		st = ret.status;
	}

	if (st == 0) {
		st = get_mapped_results(tmpfd, &buf);
	}

	if (buf != NULL) {
		arr = malloc(ret.count * sizeof (char *));
		if (arr == NULL) {
			st = ENOMEM;
		} else {
			n = bufsplit(buf, ret.count, arr);
		}
	}
	*snapnames = arr;
	*nsnaps = n;

	if (tmpfd > 0) {
		close(tmpfd);
	}

	return (st);
}

/*
 *  To Be Done:
 *	the rest of the RestoreUI interface
 *
 *		- get_file_list(fsname, snap, filter, startat)
 *		- ...
 *
 *	remove snapshot from db
 *
 *	remove metrics from db
 */

/*
 *  Function:  open_temp_file()
 *
 *  Opens a temporary file to store results from the database.
 *
 *  Parameters:
 *	NONE
 *
 *  Return values:
 *
 *	>0	Successful open.  File descriptor returned.
 *	-1	Could not open temporary file.
 */
static int open_temp_file(void)
{
	int	fd = -1;
	char	*newfile;
	char	tmpfile_template[MAXPATHLEN+1];

	mkdirp(TMPFILE_DIR, 0755);

	snprintf(tmpfile_template, sizeof (tmpfile_template),  "%s/%s",
	    TMPFILE_DIR, "fsmdbXXXXXX");

	newfile = mktemp(tmpfile_template);

	if (newfile == NULL) {
		return (-1);
	}

	unlink(newfile);

	/* Open read/write, no world access */
	fd = open(newfile, O_RDWR|O_CREAT|O_EXCL, 0660);

	/*
	 * Unlink the newly created file so it doesn't get
	 * left around if the caller crashes or otherwise
	 * forgets about it.
	 */
	if (fd > 0) {
		unlink(newfile);
	}

	return (fd);
}

/*
 *  get_mapped_results()
 *
 *  function to read back in the data returned by the database server
 */
static int
get_mapped_results(int fd, char **outbuf)
{
	struct stat64		statbuf;
	int			st = 0;
	char			*buf = NULL;
	char			*mp = MAP_FAILED;

	if ((fd == -1) || (outbuf == NULL)) {
		return (-1);
	}

	*outbuf = NULL;

	/* let's get the data back */
	st = fstat64(fd, &statbuf);

	if ((st != 0) || (!S_ISREG(statbuf.st_mode))) {
		return (EINVAL);
	}

	if (statbuf.st_size == 0) {
		return (0);
	}

	buf = malloc(statbuf.st_size + 1);
	if (buf == NULL) {
		st = ENOMEM;
		return (st);
	}

	mp = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (mp == MAP_FAILED) {
		free(buf);
		return (ENOMEM);
	}

	bcopy(mp, buf, statbuf.st_size);
	/* ensure buffer properly terminated */
	buf[statbuf.st_size] = '\0';

	munmap(mp, statbuf.st_size);

	*outbuf = buf;

	return (0);
}

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
free_fsmdb_results(char **result)
{
	if (result == NULL) {
		return;
	}

	if (result[0] != NULL) {
		free(result[0]);
	}

	free(result);
}

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
delete_fsmdb_snapshot(char *fsname, char *snapshot, uint64_t *taskid)
{
	fsmdb_ret_t		ret;
	fsmdb_door_arg_t	arg;
	door_arg_t		d_arg;

	if ((fsname == NULL) || (snapshot == NULL)) {
		return (EINVAL);
	}

	arg.task = DELETE_SNAPSHOT;

	strlcpy(arg.u.i.fsname, fsname, sizeof (arg.u.i.fsname));
	strlcpy(arg.u.i.snapshot, snapshot, sizeof (arg.u.i.snapshot));

	/* TODO - support taskid's & nowait option */
	if (taskid != NULL) {
		*taskid = 0;
	}

	d_arg.data_ptr = (char *)&arg;
	d_arg.data_size = sizeof (fsmdb_door_arg_t);
	d_arg.desc_ptr = NULL;
	d_arg.desc_num = 0;
	d_arg.rbuf = (char *)&ret;
	d_arg.rsize = sizeof (fsmdb_ret_t);

	if ((call_fsmdb(&d_arg)) != 0) {
		/* couldn't connect. */
		return (ret.status);
	}

	if ((d_arg.data_ptr == NULL) || (d_arg.data_size == 0)) {
		/* something odd happened */
		return (EINVAL);
	}

	return (ret.status);
}

/*
 *  Function:  get_snapshot_status()
 *
 *  Returns an array of datastructures describing each snapshot registered
 *  in the database for a given filesystem.
 *
 *  change to get_all_snapshot_status()
 *  new func get_snapshot_status() - takes import_arg_t, returns struct.
 *  change get_fs_snapshots to list_fs_snapshots
 *
 */
int
get_snapshot_status(
	char		*fsname,	/* IN */
	snapspec_t	**snapArray,	/* OUT */
	uint32_t	*count)		/* OUT */
{
	fsmdb_ret_t		ret;
	fsmdb_door_arg_t	arg;
	door_desc_t		desc;
	door_arg_t		d_arg;
	int			tmpfd = -1;
	int			st = 0;
	char			*buf = NULL;

	if ((fsname == NULL) || (snapArray == NULL) || (count == NULL)) {
		return (EINVAL);
	}

	*snapArray = NULL;
	*count = 0;

	/* open the temporary file for the data */
	tmpfd = open_temp_file();
	if (tmpfd == -1) {
		return (EIO);
	}

	arg.task = GET_SNAPSHOT_STATUS;
	strlcpy(arg.u.c, fsname, sizeof (arg.u.c));

	desc.d_data.d_desc.d_descriptor = tmpfd;
	desc.d_attributes = DOOR_DESCRIPTOR;

	d_arg.data_ptr = (char *)&arg;
	d_arg.data_size = sizeof (fsmdb_door_arg_t);
	d_arg.desc_ptr = &desc;
	d_arg.desc_num = 1;
	d_arg.rbuf = (char *)&ret;
	d_arg.rsize = sizeof (fsmdb_ret_t);

	if ((call_fsmdb(&d_arg)) != 0) {
		/* couldn't connect. */
		close(tmpfd);
		return (ret.status);
	}

	if ((d_arg.data_ptr == NULL) || (d_arg.data_size == 0)) {
		/* something odd happened */
		st = EINVAL;
	} else {
		st = ret.status;
	}

	if (st == 0) {
		st = get_mapped_results(tmpfd, &buf);
	}

	if (tmpfd > 0) {
		close(tmpfd);
	}

	if (buf != NULL) {
NOTE(LINTED("pointer cast may result in improper alignment"))
		*snapArray = (snapspec_t *)buf;
		*count = ret.count;
	}

	return (st);
}

/*
 *  Function:  list_snapshot_files()
 *
 *   This function is unusual in that it requires the results list to
 *   be created before calling this function.  This is to avoid linking
 *   with libfsmgmt unless we absolutely have to.
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
	sqm_lst_t		*results)	/* OUT */
{
	fsmdb_ret_t		ret;
	fsmdb_door_arg_t	arg;
	door_desc_t		desc;
	door_arg_t		d_arg;
	int			tmpfd = -1;
	int			st = 0;

	if ((fsname == NULL) || (snapshot == NULL) || (startDir == NULL) ||
	    (results == NULL)) {
		return (EINVAL);
	}

	if (morefiles != NULL) {
		*morefiles = 0;
	}

	/* open the temporary file for the data */
	tmpfd = open_temp_file();
	if (tmpfd == -1) {
		return (EIO);
	}

	arg.task = LIST_SNAPSHOT_FILES;

	memset(&(arg.u.l), 0, sizeof (flist_arg_t));

	strlcpy(arg.u.l.fsname, fsname, sizeof (arg.u.l.fsname));
	strlcpy(arg.u.l.snapshot, snapshot, sizeof (arg.u.l.snapshot));
	strlcpy(arg.u.l.startDir, startDir, sizeof (arg.u.l.startDir));
	if (startFile != NULL) {
		strlcpy(arg.u.l.startFile, startFile,
		    sizeof (arg.u.l.startFile));
	}
	memcpy(&(arg.u.l.restrictions), &restrictions, sizeof (restrict_t));
	arg.u.l.which_details = which_details;
	arg.u.l.howmany = howmany;
	arg.u.l.includeStart = includeStart;

	desc.d_data.d_desc.d_descriptor = tmpfd;
	desc.d_attributes = DOOR_DESCRIPTOR;

	d_arg.data_ptr = (char *)&arg;
	d_arg.data_size = sizeof (fsmdb_door_arg_t);
	d_arg.desc_ptr = &desc;
	d_arg.desc_num = 1;
	d_arg.rbuf = (char *)&ret;
	d_arg.rsize = sizeof (fsmdb_ret_t);

	if ((call_fsmdb(&d_arg)) != 0) {
		/* couldn't connect. */
		close(tmpfd);
		return (ret.status);
	}

	if ((d_arg.data_ptr == NULL) || (d_arg.data_size == 0)) {
		/* something odd happened */
		st = EINVAL;
	} else {
		st = ret.status;
		if (morefiles != NULL) {
			*morefiles = ret.count;
		}
	}

	if (st == 0) {
		XDR	xdrs;
		FILE	*fp;

		fp = fdopen(tmpfd, "r");
		if (fp != NULL) {
			fseek(fp, 0, SEEK_SET);
			xdrstdio_create(&xdrs, fp, XDR_DECODE);
			xdr_filedetails_list(&xdrs, results);
			xdr_destroy(&xdrs);
			fclose(fp);
			tmpfd = -1;
		}
	}

	if (tmpfd > 0) {
		close(tmpfd);
	}

	return (st);
}

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
delete_fs_from_db(char *fsname)
{
	fsmdb_ret_t		ret;
	fsmdb_door_arg_t	arg;
	door_arg_t		d_arg;

	if (fsname == NULL) {
		return (EINVAL);
	}

	arg.task = DELETE_FSDB_FILESYS;

	strlcpy(arg.u.c, fsname, sizeof (arg.u.c));

	d_arg.data_ptr = (char *)&arg;
	d_arg.data_size = sizeof (fsmdb_door_arg_t);
	d_arg.desc_ptr = NULL;
	d_arg.desc_num = 0;
	d_arg.rbuf = (char *)&ret;
	d_arg.rsize = sizeof (fsmdb_ret_t);

	if ((call_fsmdb(&d_arg)) != 0) {
		/* couldn't connect. */
		return (ret.status);
	}

	if ((d_arg.data_ptr == NULL) || (d_arg.data_size == 0)) {
		/* something odd happened */
		return (EINVAL);
	}

	return (ret.status);
}


/*
 *  Function:  call_fsmdb()
 *
 *  Issues the calls to connect to the database server using doors.
 *  If the server is not running, this function will attempt to
 *  start it and retry the connection.
 */
static int
call_fsmdb(door_arg_t *arg)
{
	int		st;
	int		doorfd = -1;
	int		count;
	fsmdb_ret_t	*ret;
	int		saverr;
	timespec_t	sleepfor = {5, 0};	/* 5 seconds */

	if (arg == NULL) {
		return (EINVAL);
	}

NOTE(LINTED("pointer cast may result in improper alignment"))
	ret = (fsmdb_ret_t *)arg->rbuf;
	/* will get overwritten with correct status as appropriate */
	ret->status = ENOTCONN;

	/* try 5 times to get connected, then give up */
	for (count = 0; count < 5; count++) {
		doorfd = open(fsmdbdoor, O_RDWR);
		if (doorfd == -1) {
			if (errno == ENOENT) {
				/* server is not running.  Try to start it */
				pclose(popen(SBIN_DIR"/fsmdb", "w"));
			} else {
				ret->status = errno;
				return (-1);
			}
		}

		/*
		 * try to contact the server - if door_call successful,
		 * status will be set by the server
		 */
		st = door_call(doorfd, arg);
		saverr = errno;

		if (st == 0) {
			/* connected ok, done here */
			break;
		}

		close(doorfd);
		doorfd = -1;

		if (saverr == EBADF) {
			/*
			 * server was not running when we opened
			 * the door file
			 */
			pclose(popen(SBIN_DIR"/fsmdb", "w"));
			/* give the server a chance to start */
			nanosleep(&sleepfor, NULL);
		} else if ((saverr != EAGAIN) && (saverr != EINTR)) {
			/* A non-recoverable error occurred */
			ret->status = saverr;
			return (-1);
		}
	}

	if (doorfd != -1) {
		close(doorfd);
	}

	return (st);
}
