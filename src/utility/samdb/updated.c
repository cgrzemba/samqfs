/*
 *	updated.c - Continuously updates database from FSA log files.
 *
 *	Monitor's file system activity log files as they
 *	accumulate in the FSA log directory (/var/opt/SUNWsamfs/fsalog/).
 */

/*
 *	SAM-QFS_notice_begin
 *
 *	CDDL HEADER START
 *
 *	The contents of this file are subject to the terms of the
 *	Common Development and Distribution License (the "License")
 *	You may not use this file except in compliance with the License.
 *
 *	You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 *	or http://www.opensolaris.org/os/licensing.
 *	See the License for the specific language governing permissions
 *	and limitations under the License.
 *
 *	When distributing Covered Code, include this CDDL HEADER in each
 *	file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 *	If applicable, add the following below this CDDL HEADER, with the
 *	fields enclosed by brackets "[]" replaced with your own identifying
 *	information: Portions Copyright [yyyy] [name of copyright owner]
 *
 *	CDDL HEADER END
 */

/*
 *	Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 *	Use is subject to license terms.
 *
 *	SAM-QFS_notice_end
 */
#pragma ident "$Revision: 1.1 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <errno.h>			/* ANSI C includes */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>			/* POSIX includes */
#include <fcntl.h>
#include <unistd.h>

#include <sam/custmsg.h>		/* SAMFS includes */
#include <sam/mount.h>
#include <sam/lib.h>
#include <sam/names.h>
#include <sam/samevent.h>
#include <sam/fsalog.h>
#include <sam/sam_malloc.h>
#include <sam/sam_trace.h>

#define	DEC_INIT
#include <sam/sam_db.h>

/* Local Includes */
#include "arg.h"
#include "util.h"

sam_db_connect_t SAM_db; /* mySQL connect parameters */
int IdleCount; 		/* Idle loop count before exit */
char *FSA_Path; 	/* FSA directory path */
char *fs_name; 		/* Family set name */
char *fs_true_name; 	/* Family set name */
char *appl_name; 	/* Application name */
char *program_name;	/* Program name */
int Daemon = 0;		/* Running as sam-fsd child */
sig_atomic_t is_shutdown;	/* Shutdown flag set by shutdown_signal */

static fsalog_inv_t *T; /* Log file inventory table */

/* Local extern functions (get rid of these with local headers?) */
extern void Update(sam_id_t, sam_id_t, int, time_t, char *);
extern int Rename(sam_id_t, sam_id_t, int, char *);
extern int print_table(FILE *, char *, unsigned int, unsigned int, int);

static void shutdown_signal(int sig);	/* Shutdown signal handler */
static void init_signal(void);
static void print_event(FILE *, sam_event_t *);
static void program_help(void);

typedef struct { /* FSA event name table entry */
	char *label;	/* Event label string */
	short pino;	/* Parent inode used flag */
	short p;	/* Parameter field used flag */
} fsa_event_t;

static fsa_event_t fsa_event_names[] = {
	{ "none", 0, 0 },
	{ "create", 1, 0 },
	{ "change", 1, 0 },
	{ "close", 1, 0 },
	{ "rename", 1, 0 },
	{ "remove", 1, 0 },
	{ "offline", 0, 0 },
	{ "online", 0, 0 },
	{ "archive", 0, 1 },
	{ "modify", 0, 0 },
	{ "archchange", 0, 1 },
	{ "umount", 0, 1 },
	{ "unknown", 1, 1 } };

arg_t Arg_Table[] = {	/* Argument table */
/*
 * These two arguments are not currently used.
 * Intended to enable samdb_update to update multiple
 * databases for a single filesystem.
 *  { "-appl", ARG_STR, (char *)&appl_name },
 *	{ "-ffs", ARG_STR, (char *)&fs_true_name },
 */
	{ "-help", ARG_PART, (char *)1 },
	{ "-version", ARG_VERS, NULL },
	{ "-i", ARG_INT, (char *)&IdleCount },
	{ "-l", ARG_STR, (char *)&FSA_Path },
	{ "-v", ARG_TRUE, (char *)&SAMDB_Verbose },
	{ "-V", ARG_TRUE, (char *)&SAMDB_Debug },
	{ NULL, ARG_PART, NULL } };

int main(
	int argc,	/* Argument count */
	char **argv)	/* Argument vector */
{
	int ret = 0;	/* Return status */
	int eof; 	/* EOF status/count */
	int upstat; 	/* Inventory update status */
	sam_event_t ev = {0, 0}; /* Current event */
	int ord;	/* Ordinal of current log file */
	int next; 	/* Ordinal of next best file */
	int rst; 	/* Read status */
	unsigned int ino;
	unsigned int gen;
	sam_db_access_t *dba; /* DB access information	*/
	int n_vsns; 	/* Number of VSNs in cache */
	char *mount_point;
	struct sam_fs_info mnt_info;	/* File system mount table */
	int i;

	program_name = SAM_DBUPD;
	appl_name = program_name;
	fs_true_name = NULL;
	IdleCount = -1;
	FSA_Path = NULL;

	Daemon = strcmp(GetParentName(), SAM_FSD) == 0;

	init_trace(Daemon, TI_dbupd);
	init_signal();

#ifndef DEBUG
	if (!Daemon) {
		Trace(TR_ERR, SAM_DBUPD" process not started by sam-fsd");
		exit(EXIT_FAILURE);
	}
#endif

	if (argc < 2) {
		program_help();
	}

	fs_name = *++argv;
	argc--;

	sam_db_init_vsn_cache();

	while ((i = Process_Command_Arguments(Arg_Table, &argc, &argv)) > 0) {
		if (i == 1) {
			program_help();
		}
	}

	if (i < 0) {
		Trace(TR_ERR, "Argument errors");
		exit(EXIT_FAILURE);
	}

	if (SAMDB_Debug) {
		SAMDB_Verbose = TRUE;
	}

	if (fs_true_name == NULL) {
		fs_true_name = fs_name;
	}

	if (Daemon) {
		char fileName[256];

		sprintf(fileName, "%s/%s/%s/%s", SAM_VARIABLE_PATH, "fsalogd",
		    fs_name, program_name);
		MakeDir(fileName);
		if (chdir(fileName)) {
			/* cannot chdir to %s */
			SendCustMsg(HERE, 3038, fileName);
			exit(EXIT_FAILURE);
		}
	}

	if (FSA_Path == NULL) {
		SamMalloc(FSA_Path, strlen(SAM_FSA_LOG_PATH)+
		    strlen(fs_true_name)+2);
		strcpy(FSA_Path, SAM_FSA_LOG_PATH);
		strcat(FSA_Path, "/");
		strcat(FSA_Path, fs_true_name);
	}

	FSA_log_path(FSA_Path);

	if ((dba = sam_db_access(SAMDB_ACCESS_FILE, fs_name)) == NULL) {
		Trace(TR_ERR, "family set name '%s' not found in %s",
		    fs_name, SAMDB_ACCESS_FILE);
		exit(EXIT_FAILURE);
	}

	Trace(TR_MISC, "%s started %s", program_name, fs_name);

	SAM_db.SAM_host = dba->db_host;
	SAM_db.SAM_user = dba->db_user;
	SAM_db.SAM_pass = dba->db_pass;
	SAM_db.SAM_name = dba->db_name;
	SAM_db.SAM_port = *dba->db_port != '\0' ?
	    atoi(dba->db_port) : SAMDB_DEFAULT_PORT;
	SAM_db.SAM_client_flag = *dba->db_client != '\0' ?
	    atoi(dba->db_client) : SAMDB_CLIENT_FLAG;
	mount_point = dba->db_mount;

	/*
	 * Check if name is a mount point or family set name, and mounted.
	 */
	if ((GetFsInfo(fs_true_name, &mnt_info)) == -1) {
		/* Filesystem \"%s\" not found. */
		SendCustMsg(HERE, 620, fs_true_name);
		exit(EXIT_FAILURE);
	}
	if ((mnt_info.fi_status & FS_MOUNTED) == 0)  {
		/* Filesystem \"%s\" not mounted. */
		SendCustMsg(HERE, 13754, fs_true_name);
		exit(EXIT_FAILURE);
	}

	/* Connect to the database */
	if (sam_db_connect(&SAM_db)) {
		Trace(TR_ERR, "Connect fail: %s", mysql_error(SAMDB_conn));
		exit(EXIT_FAILURE);
	}

	if ((n_vsns = sam_db_load_vsns()) < 0) {
		Trace(TR_ERR, "Errors loading VSNs from db");
		ret = EXIT_FAILURE;
		goto out;
	}

	if (SAMDB_Verbose) {
		Trace(TR_MISC, "VSNs loaded from db: %d", n_vsns);
		Trace(TR_MISC, "Family set: %s", fs_name);
	}

	/* Initialize rename */
	(void) Rename(ev.ev_id, ev.ev_pid, -1, mount_point);

	if (FSA_load_inventory(&T, fs_true_name, appl_name) < 0) {
		Trace(TR_ERR, "Error loading inventory");
		ret = EXIT_FAILURE;
		goto out;
	}

	if ((upstat = FSA_update_inventory(&T)) < 0) {
		Trace(TR_ERR, "Error updating inventory");
		ret = EXIT_FAILURE;
		goto out;
	}

	if (upstat != 0) {
		if (FSA_save_inventory(T, 0) < 0) {
			Trace(TR_ERR, "error saving inventory");
			ret = EXIT_FAILURE;
			goto out;
		}
	}

	if (SAMDB_Verbose) {
		FSA_print_inventory(T, stdout);
	}

	ord = FSA_next_log_file(T, 0);

	if (ord == -1) {
		Trace(TR_ERR, "No next log file");
		ret = EXIT_FAILURE;
		goto out;
	}
	if (ord < 0) {
		Trace(TR_ERR, "Error finding next log file: %d", ord);
		ret = EXIT_FAILURE;
		goto out;
	}

advance: /* Advance to next log file */
	if (FSA_open_log_file(T, ord, 0) < 0) {
		Trace(TR_ERR, "Error opening log file");
		ret = EXIT_FAILURE;
		goto out;
	}
	if (SAMDB_Verbose) {
		Trace(TR_MISC, "%s\n", T->logs[ord].file_name);
	}

restart: /* Restart scan */

	if ((SAMDB_fd = open(dba->db_mount, O_RDONLY)) < 0) {
		Trace(TR_ERR, "Can't open mount point: %s", dba->db_mount);
	}
	for (; ; ) {
		if (is_shutdown) {
			rst = 0;
			break;
		}
		if ((rst = FSA_read_next_event(T, &ev)) != 0) {
			break;
		}
		eof = 0;
		if (SAMDB_Verbose) {
			print_event(stdout, &ev);
		}
		if (ev.ev_num == ev_none) {
			continue;
		}
		ino = ev.ev_id.ino;
		gen = ev.ev_id.gen;

		if (SAMDB_Debug) {
			printf("DB before:\n");
			(void) print_table(stdout, T_SAM_INODE, ino, gen, 0);
			(void) print_table(stdout, T_SAM_PATH, ino, gen, 0);
			(void) print_table(stdout, T_SAM_LINK, ino, gen, 0);
			(void) print_table(stdout, T_SAM_ARCHIVE, ino, gen, 0);
		}

		i = 0;
		if (ev.ev_num == ev_rename) {
			i = Rename(ev.ev_id, ev.ev_pid,
			    ev.ev_param, mount_point);

			/* If part one of two, don't do update below. */
			if (ev.ev_param == 1) {
				i = -1;
			}
		}

		if (i >= 0) {
			Update(ev.ev_id, ev.ev_pid, 0, ev.ev_time, mount_point);
		}

		if (SAMDB_Debug) {
			printf("DB after:\n");
			(void) print_table(stdout, T_SAM_INODE, ino, gen, 0);
			(void) print_table(stdout, T_SAM_PATH, ino, gen, 0);
			(void) print_table(stdout, T_SAM_LINK, ino, gen, 0);
			(void) print_table(stdout, T_SAM_ARCHIVE, ino, gen, 0);
		}
	}
	close(SAMDB_fd);

	if (rst == 1) { /* if EOF */
		eof++;
next_log:
		next = FSA_next_log_file(T, 0);
		if (next < 0) {
			if (next == -1) {
				if (SAMDB_Debug) {
				Trace(TR_DEBUG, "Going to sleep on EOF");
				}
				if (is_shutdown) {
					goto out;
				} else {
					sleep(60);
				}
			} else {
				Trace(TR_ERR, "Error finding next log file");
				ret = EXIT_FAILURE;
				goto out;
			}
		}
		if (next >= 0) {
			if (FSA_close_log_file(T, 1) < 0) { /* mark done */
				Trace(TR_ERR, "Error closing logfile");
				ret = EXIT_FAILURE;
				goto out;
			}
			if (FSA_save_inventory(T, 0) < 0) {
				Trace(TR_ERR, "Error saving inventory");
				ret = EXIT_FAILURE;
				goto out;
			}
			ord = next;
			if (is_shutdown) {
				goto out;
			} else {
				goto advance;
			}
		}
		if (!is_shutdown && eof % 2 == 1) {
			goto restart;
		}
		if ((upstat = FSA_update_inventory(&T)) < 0) {
			Trace(TR_ERR, "Error updating inventory");
			ret = EXIT_FAILURE;
			goto out;
		}
		if (!is_shutdown && upstat != 0) {
			goto next_log;
		}

		/* new files added	*/
		if (!is_shutdown) {
			if (SAMDB_Verbose) {
				Trace(TR_MISC, "Going to sleep "
				"after inventory update");
			}
			sleep(60);
			if (eof < IdleCount || IdleCount < 0) {
				goto restart;
			}
		}
	}

out:
	if (rst < 0) {
		Trace(TR_ERR, "Error reading log file");
	}

	if (!is_shutdown && SAMDB_Verbose) {
		Trace(TR_MISC, "Tired of waiting");
	}

	if (FSA_close_log_file(T, 0) < 0) { /* mark partial	*/
		Trace(TR_ERR, "Error closing logfile");
	}

	if (FSA_save_inventory(T, 1) < 0) {
		Trace(TR_ERR, "Error saving inventory");
	}

	sam_db_disconnect();
	return (ret);
}

static void
print_event(
	FILE *F, 		/* File descriptor to print to */
	sam_event_t *ev) 	/* Event to print */
{
	int event; 	/* Event number */
	char tb[60]; 	/* Time buffer */

	event = ev->ev_num;

	/* if  ( event == ev_none ) return; */
	if (event > ev_max) {
		event = ev_max;
	}

	strftime(tb, 60, "%Y.%m.%d-%H:%M:%S",
	    localtime((time_t *)&ev->ev_time));

	fprintf(F, "EVENT: %s %d.%d %s", tb, ev->ev_id.ino, ev->ev_id.gen,
	    fsa_event_names[event].label);

	if (fsa_event_names[event].pino) {
		fprintf(F, " pi=%d.%d", ev->ev_pid.ino, ev->ev_pid.gen);
	}
	if (fsa_event_names[event].p) {
		fprintf(F, " p=%d", ev->ev_param);
	}
	fprintf(F, "\n");
}

static void
init_signal(void)
{
	/*
	 * Set up signal handling. Exit on SIGINT and SIGTERM.
	 * sam-fsd sends a SIGHUP on a configure -- ignore it.
	 */
	signal(SIGINT, shutdown_signal);
	signal(SIGTERM, shutdown_signal);
	signal(SIGHUP, SIG_IGN);
}

/*
 * Sets the shutdown flag to true.
 */
static void
shutdown_signal(int sig)
{
	Trace(TR_SIG, "Recieved signal %d", sig);
	is_shutdown = TRUE;
}

static void
program_help(void)
{
	printf("Usage:\t%s %s\n", program_name, "fs_name [-i count] "
	    "[-l path] [-v|V]");
	exit(-1);
}
