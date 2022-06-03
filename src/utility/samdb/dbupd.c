/*
 *	dbupd.c - Continuously updates database from FSA log files.
 *
 *	Monitor's file system activity log files as they
 *	accumulate in the FSA log directory (/var/opt/SUNWsamfs/fsalog/).
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */
#pragma ident "$Revision: 1.2 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <strings.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/vfs.h>

#define DEC_INIT
#include <sam/lib.h>
#include <sam/custmsg.h>
#include <sam/sam_trace.h>
#include <sam/names.h>
#include <sam/samevent.h>
#include <sam/fsalog.h>
#include <sam/sam_db.h>
#include <sam/fs/sblk.h>

#include "util.h"
#include "event_handler.h"

#define	RETRY_MAX 6
#define	RETRY_SLEEP 10
#define	EOF_SLEEP 5

static boolean_t is_daemon = TRUE;
static boolean_t is_shutdown = FALSE;
static char fsa_path[MAXPATHLEN];
static char daemon_path[MAXPATHLEN];
static int num_retry = 0;
static char *fs_name;
static sam_fsa_inv_t *fsa_inv = NULL;
static sam_db_context_t *db_ctx = NULL;

static int dbupd_init(int argc, char **argv);
static int dbupd_connect(void);
static void dbupd_signal(int sig);

int main(
	int argc,	/* Argument count */
	char **argv)	/* Argument vector */
{
	sam_event_t event;
	event_handler_t ev_handler;

	program_name = SAM_DBUPD;
<<<<<<< HEAD
=======

>>>>>>> 47dce836 (build with GCC10, avoid duplicate symbols)
	if (dbupd_init(argc, argv) < 0) {
		return (EXIT_FAILURE);
	}

connect:
	/* Keep trying to connect to database until shutdown or retry limit */
	while (num_retry < RETRY_MAX && !is_shutdown &&
	    dbupd_connect() < 0) {
		num_retry++;
		sleep(RETRY_SLEEP);
	}

	/* Keep trying to process events until retry limit or shutdown */
	while (num_retry < RETRY_MAX && !is_shutdown) {
		int status = sam_fsa_read_event(&fsa_inv, &event);
		if (status < 0) {
			/* Error reading event for fs_name */
			SendCustMsg(HERE, 26003, fs_name);
			num_retry++;
			sleep(RETRY_SLEEP);
			goto connect;
		} else if (status == FSA_EOF) {
			/* Reached the end of events, goto sleep for a while */
			sleep(EOF_SLEEP);
		} else if (IS_DB_INODE(event.ev_id.ino) &&
		    IS_DB_INODE(event.ev_pid.ino)) {
			/* Got next event, get event handler and process */
			ev_handler = get_event_handler(event.ev_num);
			if (ev_handler == NULL) {
				/* Unrecognized event for fsname */
				SendCustMsg(HERE, 26006, event.ev_num, fs_name);
				continue;
			}

			/* Handle event */
			if (ev_handler(db_ctx, &event) < 0) {
				mysql_rollback(db_ctx->mysql);
				/* Event processing failed, running check */
				SendCustMsg(HERE, 26007,
				    get_event_name(event.ev_num),
				    event.ev_id.ino, event.ev_id.gen, fs_name);
				if (check_consistency(db_ctx,
				    &event, TRUE) < 0) {
					mysql_rollback(db_ctx->mysql);
					sam_fsa_rollback(&fsa_inv);
					/* Consistency check failed, retrying */
					SendCustMsg(HERE, 26008);
					num_retry++;
					goto connect;
				} else {
					mysql_commit(db_ctx->mysql);
				}
			} else {
				mysql_commit(db_ctx->mysql);
			}
			num_retry = 0;
		} else {
			/*
			 * Event's inode doesn't belong in database,
			 * run consistency to be sure.
			 */
			(void) check_consistency(db_ctx, &event, TRUE);
		}
	}

	/* Close event log */
	sam_fsa_close_inv(&fsa_inv);

	/* Close database */
	sam_db_disconnect(db_ctx);
	sam_db_context_free(db_ctx);

	if (num_retry >= RETRY_MAX) {
		/* Retry max reached, exiting */
		SendCustMsg(HERE, 26005, RETRY_MAX);
	}

	return (is_shutdown && num_retry < RETRY_MAX ?
	    EXIT_SUCCESS : EXIT_FAILURE);
}

/*
 * Initialize the daemon and database configuration.
 */
static int
dbupd_init(int argc, char **argv)
{
	sam_db_conf_t *db_conf;

	/* Check to make sure fsd started */
	if (strcmp(GetParentName(), SAM_FSD) != 0) {
#ifdef DEBUG
		is_daemon = FALSE;
#else
		fprintf(stderr, "%s may only be started from "SAM_FSD"\n",
		    program_name);
		return (-1);
#endif /* DEBUG */
	}

	/* Process args */
	if (argc < 2) {
		fprintf(stderr, "usage: "SAM_DBUPD" fsname\n");
		return (-1);
	}
	fs_name = argv[1];

	init_trace(is_daemon, TI_dbupd);

	/*
	 * Set up signal handling. Exit on SIGINT and SIGTERM.
	 * sam-fsd sends a SIGHUP on a configure -- ignore it.
	 */
	signal(SIGINT, dbupd_signal);
	signal(SIGTERM, dbupd_signal);
	signal(SIGHUP, SIG_IGN);

	/* Change to daemon directory */
	if (is_daemon) {
		snprintf(daemon_path, MAXPATHLEN, "%s/%s/%s/%s",
		    SAM_VARIABLE_PATH, "fsalogd", fs_name, program_name);
		MakeDir(daemon_path);
		if (chdir(daemon_path)) {
			/* cannot chdir to %s */
			SendCustMsg(HERE, 3038, daemon_path);
			exit(EXIT_FAILURE);
		}
	} else {
		daemon_path[0] = '\0';
	}

	/* Create inventory path */
	snprintf(fsa_path, MAXPATHLEN, "%s/%s",
	    FSA_DEFAULT_LOG_PATH, fs_name);

	/* Get database config and create context */
	db_conf = sam_db_conf_get(SAMDB_ACCESS_FILE, fs_name);
	if (db_conf == NULL) {
		/* Could not find fs_name in SAMDB_ACCESS_FILE */
		SendCustMsg(HERE, 26000, SAMDB_ACCESS_FILE, fs_name);
		return (-1);
	}

	/* Create database context */
	db_ctx = sam_db_context_conf_new(db_conf);
	if (db_ctx == NULL) {
		/* Could not create database context */
		SendCustMsg(HERE, 26001);
		sam_db_conf_free(db_conf);
		return (-1);
	}

	sam_db_conf_free(db_conf);
	return (0);
}

/*
 * Connect to the database and fsalog.
 */
static int
dbupd_connect(void)
{
	/* (Re)connect to fsa log */
	sam_fsa_close_inv(&fsa_inv);
	if (sam_fsa_open_inv(&fsa_inv, fsa_path, fs_name, program_name) < 0) {
		/* Cannot open inventory */
		SendCustMsg(HERE, 26002, fsa_path);
		return (-1);
	}

	/* (Re)connect to database */
	sam_db_disconnect(db_ctx);
	if (sam_db_connect(db_ctx) < 0) {
		SendCustMsg(HERE, 26004, fs_name);
		return (-1);
	}
	mysql_autocommit(db_ctx->mysql, 0);

	return (0);
}

/*
 * Sets the shutdown flag to true.
 */
static void
dbupd_signal(int sig)
{
	Trace(TR_SIG, "Recieved signal %d", sig);
	is_shutdown = TRUE;
}
