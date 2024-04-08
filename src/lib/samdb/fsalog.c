/*
 * fsalog.c - FSA log file management routines.
 *
 *	fsalog is a collections of routines used to manage and read
 *	file system activity logs.  Each application manages its own
 *	inventory file (*.inv).
 *
 *	Contents:
 *	    sam_fsa_open_inv	- Open inventory to event log path.
 *	    sam_fsa_read_next_event	- Read next event entry from log file.
 *	    sam_fsa_print_inv	- Print log file inventory (for debug).
 *	    sam_fsa_close_inv	- Closes the inventory.
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

#pragma ident "$Revision: 1.4 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <errno.h>
/* ANSI C headers. */
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

#define	NOTNULL_FREE(a)	if ((a) != NULL) SamFree(a)

static char *fstatus_t_names[] = {
	"none",
	"done",
	"partial",
	"error",
	"missing",
	"unknown"
};

static char *inv_build_path(char *, char *, char *);
static int inv_open(char *, int *);
static int inv_update(sam_fsa_inv_t **);
static int inv_add_file(sam_fsa_inv_t **, char *, int);
static int inv_save(sam_fsa_inv_t *);
static void inv_free(sam_fsa_inv_t **);
static int inv_open_log(sam_fsa_inv_t *, int ord);
static int inv_open_next_log(sam_fsa_inv_t **);
static int inv_close_log(sam_fsa_inv_t *, boolean_t);
static boolean_t is_event_logfile(sam_fsa_inv_t *, char *);

/*
 * sam_fsa_open_inv - opens inventory for event log path
 *	inv - address of pointer for inventory to create
 * 	path - absolute path to directory with events logs
 * 	fs_name - family set name for filesystem events
 * 	appname - unique identifier for application using this fsa api
 *
 * precond -
 * 	inv pointer is null
 * 	fs_name is valid family set name
 * 	path points to valid directory with event logs for fs_name
 * postcond -
 * 	inv is allocated and populated with complete event log information
 *	inventory file is created in path with name 'fs_name.appname.inv'
 *
 * Returns 0 on success, -1 on failure
 */
int
sam_fsa_open_inv(
	sam_fsa_inv_t **invp,
	char *path,
	char *fs_name,
	char *appname)
{
	sam_fsa_inv_t *inv;
	char *inv_path;
	int inv_fd;
	int inv_sz;
	int rst = 0;

	if (*invp != NULL) {
		Trace(TR_ERR, "Inventory table already allocated.");
		return (-1);
	}

	inv_path = inv_build_path(path, fs_name, appname);
	inv_fd = inv_open(inv_path, &inv_sz);
	if (inv_fd < 0) {
		Trace(TR_ERR, "Open inventory file failed.");
		return (-1);
	}

	/*
	 * Allocate inventory table.  Initially inv_sz+100 file entries
	 * allocated (one in fsalog_inv_t)
	 */
	SamMalloc(inv, sizeof (sam_fsa_inv_t) + inv_sz +
	    99*sizeof (sam_fsa_log_t));
	memset(inv, 0, sizeof (sam_fsa_inv_t) + inv_sz +
	    99*sizeof (sam_fsa_log_t));

	/* Initialize inventory */
	strncpy(inv->fs_name, fs_name, sizeof (inv->fs_name));
	inv->l_fsn = strlen(fs_name);
	inv->n_logs = inv_sz / sizeof (sam_fsa_log_t);
	inv->n_alloc = inv->n_logs+100;
	inv->c_log = -1;
	inv->fd_log = -1;
	inv->fd_inv = inv_fd;
	SamStrdup(inv->path_fsa, path);
	inv->path_inv = inv_path;

	/* Read inventory file. */
	if (inv_sz > 0) {
		rst = read(inv->fd_inv, &inv->logs[0], inv_sz);
		if (rst < 0) {
			Trace(TR_ERR, "read(%s) failed", inv->path_inv);
			goto error;
		}
		if (rst != inv_sz) {
			Trace(TR_ERR, "read(%s) failed: incomplete read",
			    inv->path_inv);
			goto error;
		}
	}

	if (inv_update(&inv) < 0) {
		goto error;
	}

	*invp = inv;
	return (0);
error:
	inv_free(&inv);
	return (-1);
}

/*
 * sam_fsa_read_event - reads next event from inventory of event logs
 * 	inv - inventory structure created using sam_fsa_open_inv_path
 * 	event - pointer to event to be populated with next event
 *
 * precond -
 * 	inv is valid allocated using sam_fsa_open_inv
 * 	event - non-null pointer
 * postcond -
 * 	inv is updated to reflect position and event log status
 * 	event populated with next event data read from current log
 *
 * Returns
 * 	number of events read, currently only 1.
 * 	if no events are currently available returns FSA_EOF
 * 	if error returns -1
 */
int
sam_fsa_read_event(
	sam_fsa_inv_t **invp, /* Log file inventory table */
	sam_event_t *event) /* SAM file system event buffer */
{
	int err;
	ssize_t rst; /* Read status (read(2)) */

	if ((*invp)->fd_log < 0) {
		err = inv_open_next_log(invp);

		if (err == 1) {
			return (FSA_EOF);
		} else if (err < 0) {
			return (-1);
		}
	}

retry:
	rst = read((*invp)->fd_log, event, sizeof (sam_event_t));

	if (rst == 0) {
		/* end of file, is there a next file? */
		err = inv_open_next_log(invp);

		if (err == 0) {
			goto retry;
		} else if (err == 1) {
			return (FSA_EOF);
		} else {
			Trace(TR_ERR, "error opening next log");
			return (-1);
		}
	}

	if (rst < 0) {
		Trace(TR_ERR, "read(%s) failed", (*invp)->path_log);
		inv_close_log(*invp, FALSE);
		return (-1);
	}

	if (rst != sizeof (sam_event_t)) {
		Trace(TR_ERR, "read(%s) failed: incomplete read",
		    (*invp)->path_log);
		inv_close_log(*invp, FALSE);
		return (-1);
	}

	(*invp)->logs[(*invp)->c_log].offset += sizeof (sam_event_t);
	(*invp)->last_time = event->ev_time;
	(*invp)->logs[(*invp)->c_log].last_time = event->ev_time;

	return (1);
}

/*
 * sam_fsa_rollback - Rolls back the last event that was read so that
 *    the event is read again on the subsequent call to sam_fsa_next_event.
 *
 * precond -
 * 	inv is valid allocated using sam_fsa_open_inv
 *
 * return -
 * 	0 on success, -1 on error
 */
int
sam_fsa_rollback(sam_fsa_inv_t **invp) {
	sam_fsa_inv_t *inv;

	if (invp == NULL || *invp == NULL) {
		Trace(TR_ERR, "rollback failed: null pointer");
		return (-1);
	}

	inv = *invp;

	if (inv->fd_log >= 0 && inv->c_log >= 0) {
		sam_fsa_log_t *entry = &inv->logs[inv->c_log];
		if (entry->offset > 0) {
			entry->offset -= sizeof (sam_event_t);
			if (lseek(inv->fd_log, entry->offset,
			    SEEK_SET) == (off_t)-1) {
				Trace(TR_ERR, "rollback failed: "
				    "seek(%lld) failed",
				    (int64_t)entry->offset);
			}
		}
	} else {
		return (-1);
	}

	return (0);
}

/*
 * sam_fsa_print_inv - Print FSA log file inventory.
 *
 *	sam_fsaprint_inv writes a human readable log file inventory
 * 	to the the specified file.
 */
int
sam_fsa_print_inv(
	sam_fsa_inv_t *inv,	/* Log file inventory table */
	FILE *file)		/* File to print to */
{
	char i_time[60]; /* Init time buffer */
	char l_time[60]; /* Last time buffer */
	int i;
	int status;

	for (i = 0; i < inv->n_logs; i++) {
		status = inv->logs[i].status;
		if (status < 0 || status >= fstat_MAX) {
			status = fstat_MAX;
		}

		strftime(i_time, 60, "%Y.%m.%d-%H:%M:%S",
		    localtime((time_t *)&inv->logs[i].init_time));
		if (inv->logs[i].last_time > 0) {
			strftime(l_time, 60, "%Y.%m.%d-%H:%M:%S",
			    localtime((time_t *)&inv->logs[i].last_time));
		} else {
			strcpy(l_time, "0000.00.00-00:00:00");
		}
		fprintf(file, "%s %s %s %8d %s\n", inv->logs[i].name,
		    i_time, l_time, inv->logs[i].offset,
		    fstatus_t_names[status]);
	}

	return (0);
}

/*
 * sam_fsaclose_inv - close inventory, writing any unsaved status to
 *  inventory file.
 * 	invp - address of inventory pointer created using sam_fsa_open_inv
 *
 * postcond -
 * 	inventory data is saved to disk
 * 	invp is freed and set to NULL
 *
 * returns 0 on success, -1 on failure
 */
int
sam_fsa_close_inv(sam_fsa_inv_t **invp)
{
	int rst = 0;

	if (*invp != NULL) {
		rst = inv_close_log(*invp, FALSE);
		rst |= inv_save(*invp);
		rst |= close((*invp)->fd_inv);
		inv_free(invp);
	}

	return (rst);
}

/*
 * build_invfile_path - builds inventory file path from its components
 * 	Note: result must be freed during sam_fsa_close_inv
 */
static char *
inv_build_path(
	char *path_fsa,
	char *fs_name,
	char *appname)
{
	char *path_inv;
	int len;

	len = strlen(path_fsa) + strlen(fs_name) + strlen(appname) + 10;
	SamMalloc(path_inv, len);
	strcpy(path_inv, path_fsa);
	strcat(path_inv, "/");
	strcat(path_inv, fs_name);
	strcat(path_inv, ".");
	strcat(path_inv, appname);
	strcat(path_inv, ".inv");

	return (path_inv);
}

/*
 * opens inventory file pointed to by inv_path.
 * Returns
 * 	int - file descriptor on inventory file
 * 	num_entries - number of files in inventory file
 */
static int
inv_open(
	char *inv_path,
	int *size_p)
{
	int inv_fd;
	struct stat sb; /* Status buffer (stat(2)) */

	inv_fd = open(inv_path, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|
	    S_IRGRP|S_IROTH);

	if (inv_fd == -1) {
		Trace(TR_ERR, "open(%s) failed", inv_path);
		return (-1);
	}

	if (fstat(inv_fd, &sb) == -1) {
		Trace(TR_ERR, "stat(%s) failed", inv_path);
		goto error;
	}

	if ((sb.st_size % sizeof (sam_fsa_log_t)) != 0) {
		Trace(TR_ERR, "%s: Inventory file size not "
		    "fsalog_file_t increment.", inv_path);
		goto error;
	}

	if (size_p != NULL) {
		*size_p = sb.st_size;
	}

	return (inv_fd);

error:
	if (size_p != NULL) {
		*size_p = 0;
	}

	if (inv_fd >= 0) {
		close(inv_fd);
	}

	return (-1);
}

/* Frees fsa inventory structure and sets pointer to null */
static void
inv_free(sam_fsa_inv_t **invp)
{
	if (*invp != NULL) {
		NOTNULL_FREE((*invp)->path_fsa);
		NOTNULL_FREE((*invp)->path_inv);
		NOTNULL_FREE((*invp)->path_log);
		SamFree(*invp);
		*invp = NULL;
	}
}

/* Updates inventory based on the current state of event log path */
static int
inv_update(sam_fsa_inv_t **invp)
{
	sam_fsa_inv_t *inv;
	int dir_fd = -1;
	DIR *dirp = NULL;
	struct dirent *ent; /* Directory entry */
	int i;
	int rst = 0;
	struct stat sb; /* Status buffer (stat(2)) */
	boolean_t is_changed = FALSE;

	inv = *invp;

	if ((dir_fd = open(inv->path_fsa, O_RDONLY, 0)) < 0) {
		Trace(TR_ERR, "Error opening %s", inv->path_fsa);
		rst = -1;
		goto out;
	}

	if ((dirp = fdopendir(dir_fd)) == NULL) {
		Trace(TR_ERR, "Error opening %s from fd.",
		    inv->path_fsa);
		rst = -1;
		goto out;
	}

	SamMalloc(ent, sizeof (dirent_t) +
	    fpathconf(dir_fd, _PC_NAME_MAX));

	/* Mark inventory (looking for deleted files) */
	for (i = 0; i < inv->n_logs; i++) {
		inv->logs[i].status |= fstat_MARK;
	}

	/* Loop through files in event log directory. */
	while ((ent=readdir(dirp)) != NULL) {
		if (!is_event_logfile(inv, ent->d_name)) {
			continue;
		}

		if (fstatat(dir_fd, ent->d_name, &sb,
		    AT_SYMLINK_NOFOLLOW) == -1) {
			Trace(TR_ERR, "lstat(%s/%s) error",
			    inv->path_fsa, ent->d_name);
			errno = 0;
			continue;
		}

		if (sb.st_size == 0) {
			continue; /* ignore if empty */
		}

		/* Search inventory for file. */
		for (i = 0; i < inv->n_logs; i++) {
			/* If found unmark and continue */
			if (strcmp(ent->d_name,
			    inv->logs[i].name) == 0) {
				inv->logs[i].status &= ~fstat_MARK;
				break;
			}
		}

		/* If file not found add to inventory */
		if (i >= inv->n_logs) {
			/* Newly found log file, add to inventory */
			inv_add_file(invp, ent->d_name, dir_fd);
			/* invp might have been realloc'd */
			inv = *invp;
			is_changed = TRUE;
		}
	}

	/* Change status of deleted files */
	for (i = 0; i < inv->n_logs; i++) {
		if (inv->logs[i].status & fstat_MARK) {
			inv->logs[i].status = fstat_missing;
			is_changed = TRUE;
		}
	}

	if (is_changed) {
		inv_save(*invp);
	}

out:
	NOTNULL_FREE(ent);
	if (dirp != NULL) {
		closedir(dirp);
	}

	return (rst);
}

/*
 *  --	inv_save - Save FSA log file inventory.
 *
 *	inv_save writes the inventory out to the inventory file.
 *	The inventory file should already be opened.
 *
 *	Returns:
 *	    0 if success, if error, returns -1.
 */
static int
inv_save(sam_fsa_inv_t *inv) {
	size_t tot_wr; /* Total bytes written */
	size_t n_wr; /* Number of bytes written */
	int i;

	tot_wr = 0;

	if (inv->fd_inv < 0) {
		Trace(TR_ERR, "Inventory file not open.", 0);
		return (-1);
	}

	if (lseek(inv->fd_inv, (off_t)0, SEEK_SET) == (off_t)-1) {
		Trace(TR_ERR, "seek failed", 0);
		return (-1);
	}

	for (i = 0; i < inv->n_logs; i++) {
		sam_fsa_log_t *cur = &inv->logs[i];
		if (cur->status != fstat_missing) {
			n_wr = write(inv->fd_inv, cur, sizeof (sam_fsa_log_t));
			if (n_wr != sizeof (sam_fsa_log_t)) {
				Trace(TR_ERR, "Inventory save failed!");
				return (-1);
			} else {
				tot_wr += n_wr;
			}
		}
	}

	if (ftruncate(inv->fd_inv, tot_wr) < 0) {
		Trace(TR_ERR, "Inventory truncate failed!");
		return (-1);
	}

	return (0);
}

/* Returns true if name is an event log, false otherwise. */
static boolean_t
is_event_logfile(sam_fsa_inv_t *inv, char *name)
{
	int len;

	/* Process only log files belonging to the family set. */
	if (strncmp(name, inv->fs_name, inv->l_fsn) != 0) {
		return (FALSE);
	}

	if (name[inv->l_fsn] != '.') {
		return (FALSE);
	}

	len = strlen(name);
	if (strcmp(&name[len-4], ".log") != 0) {
		return (FALSE);
	}

	if (strlen(name) > FSA_LOGNAME_MAX) {
		Trace(TR_ERR, "%s: log name too long, > %d characters",
		    name, FSA_LOGNAME_MAX);
		return (FALSE);
	}

	return (TRUE);
}

/*
 * inv_add_file - Adds file to inventory
 * 	invp - address of inventory pointer, might be realloc'd
 * 	name - name of file to add
 * 	dir_fd - directory file descriptor where file is located
 */
static int
inv_add_file(sam_fsa_inv_t **invp, char *name, int dir_fd)
{
	sam_fsa_inv_t *inv;
	int rst;
	int new_fd;
	sam_event_t event;

	new_fd = openat(dir_fd, name, O_RDONLY, 0);
	if (new_fd == -1) {
		Trace(TR_ERR, "open(%s) failed", name);
		errno = 0;
		return (-1);
	}

	inv = *invp;

	/* Read first marker event */
	rst = read(new_fd, &event, sizeof (sam_event_t));
	if (rst < 0) {
		Trace(TR_ERR, "read(%s) failed", name);
	}
	if (rst > 0 && rst != sizeof (sam_event_t)) {
		Trace(TR_ERR, "read(%s) failed: incomplete read", name);
	}
	if (rst != sizeof (sam_event_t)) {
		rst = -1;
		goto out;
	}
	rst = 0;

	if (event.ev_num != ev_none &&
	    event.ev_param != FSA_STATUS_RUNNING) {
		Trace(TR_ERR, "%s: incorrect marker event", name);
		rst = -1;
		goto out;
	}

	/* Realloc if table full */
	if (inv->n_logs == inv->n_alloc) {
		inv->n_alloc += 100;
		SamRealloc(inv, sizeof (sam_fsa_inv_t) +
		    inv->n_alloc*sizeof (sam_fsa_log_t));
		*invp = inv;
	}

	/* Add newly found file to the inventory. */
	memset((char *)&inv->logs[inv->n_logs], 0, sizeof (sam_fsa_log_t));
	strcpy(inv->logs[inv->n_logs].name, name);
	inv->logs[inv->n_logs].init_time = event.ev_time;
	inv->logs[inv->n_logs].status = fstat_none;
	inv->n_logs++;

out:
	if (new_fd > 0) {
		close(new_fd);
	}
	errno = 0;
	return (rst);
}

/*
 *  --	inv_open_next_log - Open next best log file in inventory.
 *
 *	inv_open_next_log searches the inventory for the next best
 *	log file to process.
 *
 *	Returns:
 * 		1 if no next log file
 * 		0 if successful
 * 		-1 other error
 */
static int
inv_open_next_log(sam_fsa_inv_t **invp)
{
	sam_fsa_inv_t *inv;
	int done_ord = -1; /* Ordinal of youngest done log */
	sam_time_t done_time = 0; /* Init time of youngest done log */
	int next_ord = -1; /* Ordinal of oldest not done log */
	sam_time_t next_time = 0; /* Init time of oldest not done log */
	int i;

	if (inv_update(invp) < 0) {
		return (-1);
	}
	inv = *invp;

	/*
	 * Scan inventory table:
	 * 1. Find youngest log file that is completely processed (i.e.  done)
	 * 2. Find next best log file for processing (i.e., oldest not done)
	 */
	for (i = 0; i < inv->n_logs; i++) {
		/* Don't want to pick current log */
		if (i == inv->c_log) {
			continue;
		}

		/* If log already processed record youngest done time */
		if (inv->logs[i].status == fstat_done &&
		    inv->logs[i].init_time > done_time) {
			done_ord = i;
			done_time = inv->logs[i].init_time;
		}

		/* If log still requires processing */
		if (inv->logs[i].status == fstat_none ||
		    inv->logs[i].status == fstat_part) {
			if (inv->logs[i].init_time < next_time ||
			    next_ord == -1) {
				next_ord = i;
				next_time = inv->logs[i].init_time;
			}
		}
	}

	if (next_ord < 0) {
		return (1); /* If no next entry */
	}

	if (inv->c_log >= 0 &&
	    next_time < inv->logs[inv->c_log].init_time) {
		Trace(TR_ERR, "next file [%s] older than current file [%s]",
		    inv->logs[next_ord].name, inv->logs[inv->c_log].name);
	}

	if (inv->last_time != 0 && next_time < inv->last_time) {
		Trace(TR_ERR, "next file [%s] older than last processed event",
		    inv->logs[next_ord].name, inv->logs[inv->c_log].name);
	}

	if (done_ord >= 0 && next_time < done_time) {
		Trace(TR_ERR, "next file [%s] older than"
		    "youngest done file [%s]", inv->logs[next_ord].name,
		    inv->logs[done_ord].name);
	}

	/* Close current log, mark as done */
	if (inv->c_log >= 0) {
		inv_close_log(inv, TRUE);
	}

	inv_open_log(inv, next_ord);

	return (0);
}

/*
 *  --	inv_open_log - Open log file.
 *
 *	Open specified log file from inventory.
 *
 *	On Entry:
 *	    T       = Inventory table.
 *	    ord	    = Ordinal of log file to open.
 *
 *	Returns:
 *	   File descriptor, else -1 if error occurred.
 */
static int
inv_open_log(
	sam_fsa_inv_t *inv, 	/* Log file inventory table	*/
	int ord)		/* Ordinal of log file to open	*/
{
	off_t offset; /* File position (lseek(2)) */

	if (ord < 0 || ord >= inv->n_logs) {
		Trace(TR_ERR, "invalid log file table ordinal:%d", ord);
		return (-1);
	}

	if (inv->fd_log >= 0 || inv->c_log >= 0) {
		Trace(TR_ERR, "log file already open: %s", inv->path_log);
		return (-1);
	}

	SamMalloc(inv->path_log, strlen(inv->path_fsa) + FSA_LOGNAME_MAX + 4);
	strcpy(inv->path_log, inv->path_fsa);
	strcat(inv->path_log, "/");
	strcat(inv->path_log, inv->logs[ord].name);

	inv->fd_log = open(inv->path_log, O_RDONLY, 0);

	if (inv->fd_log == -1) {
		if (errno == ENOENT) {
			Trace(TR_MISC, "file missing(%s)", inv->path_log);
			inv->logs[ord].status = fstat_missing;
			return (-1);
		} else {
			Trace(TR_ERR, "open(%s) failed", inv->path_log);
			return (-1);
		}
	}

	if (inv->logs[ord].status == fstat_part) {
		offset = lseek(inv->fd_log, inv->logs[ord].offset, SEEK_SET);
		if (offset == (off_t)-1) {
			Trace(TR_ERR, "seek failed", 0);
			return (-1);
		}
		if (offset != inv->logs[ord].offset) {
			Trace(TR_ERR, "%s: file positon mismatch in table, "
			    "table:%d, fpos:%d", inv->path_log,
			    inv->logs[ord].offset, offset);
		}
	}

	inv->c_log = ord; /* mark current open log file	*/
	return (0);
}


/*
 * --	inv_close_log - Closes current log file.
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
static int
inv_close_log(
	sam_fsa_inv_t *inv, /* Log file inventory table */
	boolean_t mark_done) /* Mark log file as processed */
{
	off_t offset;	/* File offset (lseek(2)) */
	int rst = 0;	/* Return status */

	/* If no current open log, reset variables and return */
	if (inv->fd_log < 0 || inv->c_log < 0) {
		goto out;
	}

	/* Get current offset */
	offset = lseek(inv->fd_log, (off_t)0, SEEK_CUR);
	if (offset == (off_t)-1) {
		Trace(TR_ERR, "seek failed", 0);
		rst = -1;
	} else {
		sam_fsa_log_t *cur_log = &inv->logs[inv->c_log];
		if (offset != cur_log->offset) {
			Trace(TR_ERR, "%s: file positon mismatch in table,"
			    " table:%d, fpos:%d", inv->path_log,
			    cur_log->offset, offset);
		}
		cur_log->offset = offset;
		if (offset != 0) {
			cur_log->status = fstat_part;
		}
		if (mark_done) {
			cur_log->status = fstat_done;
		}
	}

out:
	if (inv->fd_log >= 0) {
		close(inv->fd_log);
	}

	inv->fd_log = -1;
	NOTNULL_FREE(inv->path_log);
	inv->path_log = NULL;
	inv->c_log = -1;

	return (rst);
}
