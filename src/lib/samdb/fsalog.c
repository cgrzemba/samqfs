/*
 * fsalog.c - FSA log file management routines.
 *
 *	libfsalog is a collections of routines used to manage and read
 *	file system activity logs.  Each application manages its own
 *	inventory file (*.inv).
 *
 *	Contents:
 *	    FSA_log_path	- Set log path.
 *	    FSA_load_inventory	- Load log file inventory.
 *	    FSA_update_inventory- Update log file inventory.
 *	    FSA_save_inventory	- Save log file inventory.
 *	    FSA_print_inventory	- Print log file inventory.
 *
 *	    FSA_open_log_file	- Open log file.
 *	    FSA_next_log_file	- Advance to next log file.
 *	    FSA_close_log_file	- Close log file.
 *	    FSA_read_next_event	- Read next event entry from log file.
 */

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

#pragma ident "$Revision: 1.1 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

#include <errno.h>		/* ANSI C headers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>	/* POSIX headers. */
#include <dirent.h>
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>

#include "sam/types.h"	/* SAM-FS includes. */
#include "sam/param.h"
#include "sam/samevent.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "sam/fsalog.h"

static int fd_inv = -1; 			/* Inventory file descriptor */
static int fd_log = -1;				/* Log file descriptor */
static char *fsa_path = NULL;		/* FSA directory path */
static char *path_inv = NULL;		/* Inventory file path/name */
static char *path_log = NULL;		/* Log file path/name */
static sam_time_t last_time = 0;	/* Time of last event read */

/*
 *  --	FSA_log_path - Set FSA log path.
 *
 *	FSA_log_path sets the directory path.
 *
 *	On Entry:
 *	    path   = Log directory path.
 */
void FSA_log_path(
	char *path
)
{
	fsa_path = path;
}


/*
 *  --	FSA_load_inventory - Load FSA log file inventory.
 *
 *	FSA_load_inventory reads the inventory file.  Inventory files
 *	are named: fs_name.appl.inv, where fs_name is the family set
 *	name and appl is the application identifier (e.g., samfs1.db.inv).
 *
 *	On Entry:
 *	    fs_name = Family set name.
 *	    appl    = Application identifier.
 *
 *	On Return:
 *	    T       = Inventory table.
 *
 *	Returns:
 *	    Returns number of log files in inventory.  If error, returns -1.
 */
int	FSA_load_inventory(
	fsalog_inv_t **T,	/* Log file inventory table */
	char *fs_name,		/* Family set name */
	char *appl		/* Application name */
)
{
	struct stat	sb;	/* Status buffer (stat(2)) */
	fsalog_inv_t *t;	/* Log file inventory table */
	ssize_t rst;		/* Read status (read(2)) */
	size_t n;
	int l;

	if (*T != NULL) {
		Trace(TR_ERR, "Inventory table already allocated.");
		return (-1);
	}

	if (fd_inv >= 0) {
		Trace(TR_ERR, "Inventory file already open.");
		return (-1);
	}

	if (fsa_path == NULL) {
		fsa_path = SAM_FSA_LOG_PATH;
	}

	l = strlen(fsa_path) + strlen(fs_name) + strlen(appl) + 10;
	SamMalloc(path_inv, l);
	strcpy(path_inv, fsa_path);
	strcat(path_inv, "/");
	strcat(path_inv, fs_name);
	strcat(path_inv, ".");
	strcat(path_inv, appl);
	strcat(path_inv, ".inv");

	fd_inv = open(path_inv, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|
	    S_IRGRP|S_IROTH);

	if (fd_inv == -1) {
		Trace(TR_ERR, "open(%s) failed", path_inv);
		return (-1);
	}

	if (fstat(fd_inv, &sb) == -1) {
		Trace(TR_ERR, "stat(%s) failed", path_inv);
		return (-1);
	}

	n = sb.st_size / sizeof (fsalog_file_t);

	if ((sb.st_size % sizeof (fsalog_file_t)) != 0) {
		Trace(TR_ERR, "%s: Inventory file size not "
		    "flsalog_file_t increment.", path_inv);
	}

	/* Allocate log file inventory table. */
	SamMalloc(t, sizeof (fsalog_inv_t) + (n+100)*sizeof (fsalog_file_t));
	memset(t, 0, sizeof (fsalog_inv_t) + (n+100)*sizeof (fsalog_file_t));
	t->n_logs  = n;
	t->n_alloc = n + 100;
	t->c_log   = -1;
	t->l_fsn   = strlen(fs_name);
	strncpy(t->fs_name, fs_name, 40);
	*T = t;

	/* Read inventory file. */
	if (sb.st_size != 0) {
		rst = read(fd_inv, &t->logs[0], sb.st_size);
		if (rst < 0) {
			Trace(TR_ERR, "read(%s) failed", path_inv);
			return (-1);
		}
		if (rst != sb.st_size) {
			Trace(TR_ERR, "read(%s) failed: incomplete read",
			    path_inv);
			return (-1);
		}
	}

	/* free(path_inv); */
	return (FSA_update_inventory(T));
}


/*
 *  --	FSA_update_inventory - Update FSA log file inventory.
 *
 *	FSA_update_inventory scans the FSA log file directory looking for
 *	log files not currently inventoried.
 *
 *	For a file to become part of the inventory, it most meet the
 *	file nameing convetion of fs_name.*.log (e.g., samfs1.200804181200.log),
 *	have a non-zero length, and have a null event with timestamp as the
 *	initial event.
 *
 *	On Entry:
 *		T		= Address of inventory table pointer.
 *
 *	On Return:
 *		T		= Inventory table (may have moved in memory)
 *
 *	Returns:
 *		Number of new files found,
 *		zero, if no new log files added to the inventory,
 *		and -1 if an error occurred.
 */
int	FSA_update_inventory(
	fsalog_inv_t **T /* Log file inventory table */
)
{
	DIR *dirp;			/* Open directory pointer */
	struct dirent *dirent;		/* Directory entry */
	struct stat sb;			/* Status buffer (lstat(2)) */
	int fd;				/* Log file descriptor */
	fsalog_inv_t *t;		/* Log file inventory table */
	sam_event_t	event;		/* Log file initial event */
	char		*path;		/* Log file full path */
	ssize_t		rst;		/* Read status (read(2)) */
	int		rts	= 0;	/* Return status */
	int		i, l;

	if ((dirp = opendir(fsa_path)) == NULL) {
		Trace(TR_ERR, "opendir(%s)", SAM_FSA_LOG_PATH);
		return (-1);
	}

	t = *T;
	errno = 0;

	SamMalloc(path, strlen(fsa_path) + 64);

	while ((dirent = readdir(dirp)) != NULL) {
		/* Process only log files belonging to the family set. */
		if (strncmp(dirent->d_name, t->fs_name, t->l_fsn) != 0) {
			continue;
		}
		if (dirent->d_name[t->l_fsn] != '.') {
			continue;
		}
		l  = strlen(dirent->d_name);
		if (strcmp(&dirent->d_name[l-4], ".log") != 0) {
			continue;
		}

		if (strlen(dirent->d_name) > 59) {
			Trace(TR_ERR, "%s: file name too long, > 59 characters",
			    path);
			continue;
		}

		strcpy(path, fsa_path);
		strcat(path, "/");
		strcat(path, dirent->d_name);

		if (lstat(path, &sb) == -1) {
			Trace(TR_ERR, "lstat(%s) error", path);
			errno = 0;
			continue;
		}

		if (sb.st_size == 0) {
			continue;	/* ignore while empty	*/
		}

		/* Search inventory for file. */
		for (i = 0; i < t->n_logs; i++) {
			if (strcmp(dirent->d_name, t->logs[i].file_name) == 0) {
				break;
			}
		}

		if (i != t->n_logs) {
			continue;		/* if found	*/
		}

		/* Newly found log file, probe for create time */
		fd = open(path, O_RDONLY, 0);

		if (fd == -1) {
			Trace(TR_ERR, "open(%s) failed", path);
			errno = 0;
			continue;
		}

		rst = read(fd, &event, sizeof (sam_event_t));
		if (rst < 0) {
			Trace(TR_ERR, "read(%s) failed", path);
		}
		if (rst > 0 && rst != sizeof (sam_event_t)) {
			Trace(TR_ERR, "read(%s) failed: incomplete read", path);
		}
		if (rst != sizeof (sam_event_t)) {
			close(fd);
			errno = 0;
			continue;
		}

		if (event.ev_num != ev_none &&
		    event.ev_param != FSA_STATUS_RUNNING) {
			Trace(TR_ERR, "%s: incorrect marker event", path);
			continue;
		}

		/* Add newly found file to the inventory. */
		rst++;

		/* if table full */
		if (t->n_logs == t->n_alloc) {
			t->n_alloc += 100;
			SamRealloc(t, sizeof (fsalog_inv_t) +
			    t->n_alloc*sizeof (fsalog_file_t));
			*T = t;
		}

		memset((char *)&t->logs[t->n_logs], 0, sizeof (fsalog_file_t));
		strcpy(t->logs[t->n_logs].file_name, dirent->d_name);
		t->logs[t->n_logs].init_time = event.ev_time;
		t->logs[t->n_logs].status = fstat_none;
		t->n_logs++;
	}

	SamFree(path);

	if (errno != 0) {
		Trace(TR_ERR, "readdir(%s)", fsa_path);
	}

	if (closedir(dirp) < 0) {
		Trace(TR_ERR, "closedir(%s)", fsa_path);
	}

	return (rts);
}


/*
 *  --	FSA_save_inventory - Save FSA log file inventory.
 *
 *	FSA_save_inventory writes the inventory out to the inventory file.
 *	The inventory file should already be opened.
 *
 *	On Entry:
 *	    T       = Inventory table.
 *
 *	Returns:
 *	    Returns number of log files in inventory.  If error, returns -1.
 */
int	FSA_save_inventory(
	fsalog_inv_t *T,	/* Log file inventory table */
	int close_file		/* Close inventory file flag */
)
{
	ssize_t wst;		/* Write status (write(2)) */
	size_t n;		/* Number of bytes written */

	if (fd_inv < 0) {
		Trace(TR_ERR, "Inventory file not open.", 0);
		return (-1);
	}

	if (lseek(fd_inv, (off_t)0, SEEK_SET) == (off_t)-1) {
		Trace(TR_ERR, "seek failed", 0);
		return (-1);
	}

	n = T->n_logs * sizeof (fsalog_file_t);
	wst = write(fd_inv, T->logs, n);

	if (wst < 0) {
		Trace(TR_ERR, "write failed", 0);
		return (-1);
	}

	if (wst != n) {
		Trace(TR_ERR, "write failed: incomplete write", 0);
		return (-1);
	}

	if (close_file) {
		close(fd_inv);
		fd_inv = -1;
		return (0);
	}

	return (0);
}


/*
 *  --	FSA_print_inventory - Print FSA log file inventory.
 *
 *	FSA_print_inventory writes a list of log file inventory to the
 *	the specified file.
 *
 *	On Entry:
 *	    T       = Inventory table.
 */
int	FSA_print_inventory(
	fsalog_inv_t *T,	/* Log file inventory table */
	FILE *F			/* File to print to */
)
{
	char *fstat[] = {"none", "done", "partial", "error",
	    "missing", "unknown"};
	char i_time[60];	/* Init time buffer */
	char l_time[60];	/* Last time buffer */
	int i, k;

	for (i = 0; i < T->n_logs; i++) {
		k = T->logs[i].status;
		if (k > 3) {
			k = 4;
		}
		strftime(i_time, 60, "%Y.%m.%d-%H:%M:%S",
		    localtime((time_t *)&T->logs[i].init_time));
		if (T->logs[i].last_time > 0) {
			strftime(l_time, 60, "%Y.%m.%d-%H:%M:%S",
			    localtime((time_t *)&T->logs[i].last_time));
		} else {
			strcpy(l_time, "0000.00.00-00:00:00");
		}
		fprintf(F, "%s %s %s %8d %s\n", T->logs[i].file_name,
		    i_time, l_time, T->logs[i].offset, fstat[k]);
	}

	return (0);
}


/*
 *  --	FSA_open_log_file - Open log file.
 *
 *	Open specified log file from inventory.
 *
 *	On Entry:
 *	    T       = Inventory table.
 *	    ord	    = Ordinal of log file to open.
 *	    pos	    = File postion status:
 *		      1 = Postion file to last known offset as noted
 *			in the inventory table.
 *		      0 = Determine file postion based on last known
 *			file status as noted in the inventory table.
 *			If file status "part" (partially processed)
 *			then positon to last known offset, else
 *			set position to start of file.
 *
 *	Returns:
 *	   File descriptor, else -1 if error occurred.
 */
int	FSA_open_log_file(
	fsalog_inv_t *T,	/* Log file inventory table	*/
	int ord,		/* Ordinal of log file to open	*/
	int pos)		/* Position file to last offset	*/
{
	off_t offset;		/* File position (lseek(2)) */

	if (ord < 0 || ord >= T->n_logs) {
		Trace(TR_ERR, "invalid log file table ordinal:%d", ord);
		return (-1);
	}

	if (fd_log >= 0 || T->c_log >= 0) {
		Trace(TR_ERR, "log file already open: %s", path_log);
		return (-1);
	}

	if (fsa_path == NULL) {
		fsa_path = SAM_FSA_LOG_PATH;
	}

	SamMalloc(path_log, strlen(fsa_path) + 64);
	strcpy(path_log, fsa_path);
	strcat(path_log, "/");
	strcat(path_log, T->logs[ord].file_name);

	fd_log = open(path_log, O_RDONLY, 0);

	if (fd_log == -1) {
		Trace(TR_ERR, "open(%s) failed", path_log);
		return (-1);
	}

	if (pos || T->logs[ord].status == fstat_part) {
		offset = lseek(fd_log, T->logs[ord].offset, SEEK_SET);
		if (offset == (off_t)-1) {
			Trace(TR_ERR, "seek failed", 0);
			return (-1);
		}
		if (offset != T->logs[ord].offset) {
			Trace(TR_ERR, "%s: file positon mismatch in table, "
			    "table:%d, fpos:%d", path_log,
			    T->logs[ord].offset, offset);
		}
	}

	T->c_log = ord;		/* mark current open log file	*/
	return (fd_log);
}


/*
 *  --	FSA_next_log_file - Find next best log file in inventory.
 *
 *	FSA_next_log_file searches the inventory for the next best
 *	log file to process.
 *
 *	On Entry:
 *	    T       = Inventory table.
 *	    ignore  = Ignore time oddities. Log file time spans are
 *			check to ensure the next candidate has an
 *			initial time that is younger than the last
 *			recorded event of the last file processed and
 *			is newer than already processed file.
 *
 *	Returns:
 *		File inventory table ordinal of next best log file, else
 *		-1 if no next entry,
 *		-2 if next file older than current file,
 *		-3 if next file older than last processed event,
 *		-4 if next file older than youngest done file".
 */
int	FSA_next_log_file(
	fsalog_inv_t	*T,	/* Log file inventory table	*/
	int	ignore)		/* Ignore time oddities		*/
{
	int done_ord = -1;		/* Ordinal of oldest done log */
	sam_time_t done_time = 0;	/* Init time of oldest done log */
	int next_ord = -1;		/* Ordinal of next not done log */
	sam_time_t next_time = 0;	/* Init time of next not done log */
	int i;

	/*
	 * Scan inventory table:
	 * 1. Find youngest log file that is completely processed (i.e.  done)
	 * 2. Find next best log file for processing (i.e., oldest not done)
	 */
	for (i = 0; i < T->n_logs; i++) {
		if (i == T->c_log) {
			continue;
		}
		if (T->logs[i].status == fstat_done &&
		    T->logs[i].init_time > done_time) {
			done_ord = i;
			done_time = T->logs[i].init_time;
		}
		if (T->logs[i].status == fstat_none ||
		    T->logs[i].status == fstat_part) {
			if (next_ord >= 0) {
				if (T->logs[i].init_time < next_time) {
					next_ord  = i;
					next_time = T->logs[i].init_time;
				}
			} else {
				next_ord  = i;
				next_time = T->logs[i].init_time;
			}
		}
	}

	if (next_ord < 0) {
		return (-1);	/* If no next entry	*/
	}

	if (T->c_log >= 0 && next_time < T->logs[T->c_log].init_time) {
		Trace(TR_ERR, "next file [%s] older than current file [%s]",
		    T->logs[next_ord].file_name, T->logs[T->c_log].file_name);
		if (!ignore) {
			return (-2);
		}
	}

	if (last_time != 0 && next_time < last_time) {
		Trace(TR_ERR, "next file [%s] older than last processed event",
		    T->logs[next_ord].file_name, T->logs[T->c_log].file_name);
		if (!ignore) {
			return (-3);
		}
	}

	if (done_ord < 0) {
		return (next_ord);
	}

	if (next_time < done_time) {
		Trace(TR_ERR, "next file [%s] older than"
		    "youngest done file [%s]", T->logs[next_ord].file_name,
		    T->logs[done_ord].file_name);
		if (!ignore) {
			return (-4);
		}
	}

	return (next_ord);
}


/*
 * --	FSA_close_log_file - Close log file.
 *
 *	Close currently open log file.
 *
 *	On Entry:
 *		T         = Inventory table.
 *		mark_done = If true, set file status as "done".
 *
 *	Returns:
 *		Zero if no error, else -1;
 */
int	FSA_close_log_file(
	fsalog_inv_t *T,	/* Log file inventory table */
	int	mark_done)	/* Mark log file as processed */
{
	off_t offset;	/* File offset (lseek(2)) */
	int rst = 0;	/* Return status */

	offset = lseek(fd_log, (off_t)0, SEEK_CUR);
	if (offset == (off_t)-1) {
		Trace(TR_ERR, "seek failed", 0);
		rst = -1;
	} else {
		if (offset != T->logs[T->c_log].offset) {
			Trace(TR_ERR, "%s: file positon mismatch in table,"
			    " table:%d, fpos:%d", path_log,
			    T->logs[T->c_log].offset, offset);
			rst = -2;
		}
		T->logs[T->c_log].offset = offset;
		if (offset != 0) {
			T->logs[T->c_log].status = fstat_part;
		}
		if (mark_done) {
			T->logs[T->c_log].status = fstat_done;
		}
	}
	close(fd_log);
	SamFree(path_log);
	fd_log = -1;
	path_log = NULL;
	T->c_log = -1;

	return (rst);
}


/*
 * --	FSA_read_next_event - Read next event from log file.
 *
 *	Read next event from the currently open log file.
 *
 *	On Entry:
 *		T       = Inventory table.
 *
 *	On Return:
 *		event   = File system activity event.
 *
 *	Returns:
 *		Zero if no errors, 1 if EOF encountered, -1 if error.
 */
int	FSA_read_next_event(
	fsalog_inv_t *T,	/* Log file inventory table */
	sam_event_t	*event)	/* SAM file system event buffer */
{
	ssize_t rst;		/* Read status (read(2)) */

	if (fd_log < 0) {
		Trace(TR_ERR, "log file not open", 0);
		return (-1);
	}

	rst = read(fd_log, event, sizeof (sam_event_t));

	if (rst == 0) {
		return (1);			/* if at EOF	*/
	}

	if (rst < 0) {
		Trace(TR_ERR, "read(%s) failed", path_log);
		return (-1);
	}

	if (rst != sizeof (sam_event_t)) {
		Trace(TR_ERR, "read(%s) failed: incomplete read", path_log);
		return (-1);
	}

	T->logs[T->c_log].offset += sizeof (sam_event_t);
	T->logs[T->c_log].last_time = last_time = event->ev_time;

	return (0);
}
