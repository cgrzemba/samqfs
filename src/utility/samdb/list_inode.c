/*
 *  ----	samdb_listinode - Lookup up inode.
 *
 *	Description:
 *	    Print all records for a given list of inodes.
 *
 *	Command Syntax:
 *
 *	    samdb_listinode family_set [-nolabels] [-v|-V] ino(s)
 *
 *	    where:
 *
 *		family_set	Specifies the family set name.
 *
 *		-nolabels	Do not print field name labels.
 *
 *		-help		Print command syntax summary and exit.
 *
 *		-version	Print program version and exit.
 *
 *		-v		Verbose mode.  Enables verbose messages.
 *
 *		-V		Debug mode.  Enables debug messages, most
 *				notably is the query string sent to the
 *				databsase engine.
 *
 *		ino(s)		Inode numbers.
 *
 *
 *	Environment Variables:
 *
 *	    SAM_FS_NAME		Default family set name.
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

/* Include files: */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <mysql.h>

#include "arg.h"
#include "util.h"

#define	DEC_INIT
#include <sam/sam_db.h>

/* Global tables & pointers: */
char *program_name;	/* Program name: used by error */
int exit_status = 0;	/* Exit status */

int Labels; 	/* Print labels flag */
char *fs_name;	/* Family set name */

extern int print_table(FILE *, char *, unsigned int, unsigned int, int);

static void program_help(void);

arg_t Arg_Table[] = {	/* Argument table */
	{ "-nolabels", ARG_FALSE, (char *)&Labels },
	{ "-help", ARG_PART, (char *)1 },
	{ "-version", ARG_VERS, NULL },
	{ "-v", ARG_TRUE, (char *)&SAMDB_Verbose },
	{ "-V", ARG_TRUE, (char *)&SAMDB_Debug },
	{ NULL, ARG_PART, NULL } };

int
main(
	int argc,	/* Number of arguments */
	char *argv[])	/* Argument pointer list */
{
	unsigned int ino;		/* Inode number for query */
	sam_db_path_t *T_path;
	sam_db_inode_t *T_inode;
	sam_db_archive_t *T_archive;
	sam_db_access_t *dba;		/* DB access information */
	sam_db_connect_t SAM_db;	/* mySQL connect parameters */
	int i;

	/* ---	Process arguments: --- */
	program_name = "samdb_listinode";
	Labels = 1;

	if (argc < 2) {
		program_help();
	}

	fs_name = *++argv;
	argc--;

	init_trace(NULL, 0);

	while ((i = Process_Command_Arguments(Arg_Table, &argc, &argv)) > 0) {
		if (i == 1) {
			program_help();
		}
	}

	if (i < 0) {
		error(1, 0, "Argument errors\n", 0);
	}

	if (SAMDB_Debug) {
		SAMDB_Verbose = 1;
	}

	if ((dba = sam_db_access(SAMDB_ACCESS_FILE, fs_name)) == NULL) {
		error(1, 0, "Family set name [%s] not found", fs_name);
	}

	if (argc < 1) {
		error(1, 0, "No inodes specified.", 0);
	}

	/* ---	Connect to database: --- */
	SAM_db.SAM_host = dba->db_host;
	SAM_db.SAM_user = dba->db_user;
	SAM_db.SAM_pass = dba->db_pass;
	SAM_db.SAM_name = dba->db_name;
	SAM_db.SAM_port = *dba->db_port != '\0' ?
	    atoi(dba->db_port) : SAMDB_DEFAULT_PORT;
	SAM_db.SAM_client_flag = *dba->db_client != '\0' ?
	    atoi(dba->db_client) : SAMDB_CLIENT_FLAG;

	if (sam_db_connect(&SAM_db)) {
		error(1, 0, "cannot connect: %s", mysql_error(SAMDB_conn));
	}

	/* ---	Process inodes: --- */
	while (argc-- != 0) {
		ino = atol(*argv++);
		(void) print_table(stdout, T_SAM_PATH, ino, 0, Labels);
		(void) print_table(stdout, T_SAM_INODE, ino, 0, Labels);
		(void) print_table(stdout, T_SAM_LINK, ino, 0, Labels);
		(void) print_table(stdout, T_SAM_ARCHIVE, ino, 0, Labels);

		i = sam_db_query_path(&T_path, ino, 0);
		i = sam_db_query_inode(&T_inode, ino, 0);
		i = sam_db_query_archive(&T_archive, ino, 0);
	}

	sam_db_disconnect();
	exit(exit_status);
	return (0);
}

static void
program_help(void)
{
	printf("Usage:\t%s %s\n", program_name,
	    "fs_name [-nolabels] [-v|V] inos(s)");
	exit(-1);
}
