/*
 * ----	samdb_dump - Generate a list of files for samfsdump.
 *
 *	Description:
 *	    Generate list of files for samfsdump.  The list is generated
 *	    to co-group files in the same directory.
 *
 *	    This program does not use stored results. While this program
 *	    is executed updates to the database will be prevented.
 *
 *	Command Syntax:
 *
 *	    samdb_dump family_set [-o] [-f file name] [-v|V]
 *
 *	    where:
 *
 *		family_set	Specifies the family set name.
 *
 *		-o		Path only. Do not output inode/generation
 *				numbers with the path.
 *
 *		-f file name	File name to send output to
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
#include <errno.h>
#include <nl_types.h>
#include <sys/types.h>

#define	DEC_INIT
#include <sam/sam_db.h>
#include <sam/nl_samfs.h>

#include "arg.h"
#include "util.h"

/* Global tables & pointers: */
char *program_name; 	/* Program name: used by error */
int exit_status = 0; 	/* Exit status */
char *OutputFile;	/* File to send output to */
FILE *output;		/* File handle for output file */
int PathOnly; 	/* Do not output ino/gen */
char *fs_name;  /* Family set name */

static my_ulonglong sam_db_dumplist(char *);
static void program_help(void);

arg_t Arg_Table[] = {	/* Argument table	*/
	{ "-o", ARG_TRUE, (char *)&PathOnly },
	{ "-f", ARG_STR, (char *)&OutputFile },
	{ "-h", ARG_PART, (char *)1 },
	{ "-version", ARG_VERS, NULL },
	{ "-v", ARG_TRUE, (char *)&SAMDB_Verbose },
	{ "-V", ARG_TRUE, (char *)&SAMDB_Debug },
	{ NULL, ARG_PART, NULL } };

int
main(
	int argc, 	/* Number of arguments */
	char *argv[])	/* Argument pointer list */
{
	sam_db_access_t *dba;		/* DB access information */
	sam_db_connect_t SAM_db; 	/* mySQL connect parameters */
	my_ulonglong rc;		/* Returned record count */
	int i;

	/* ---	Process arguments: --- */
	program_name = "samdb_dump";
	PathOnly = 0;
	OutputFile = NULL;

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

	/* ---	Generate dump list: --- */
	rc = sam_db_dumplist(dba->db_mount);

	if (SAMDB_Verbose && rc != (my_ulonglong)-1) {
		error(0, 0, "%llu files dumped", rc);
	}

	sam_db_disconnect();
	exit(exit_status);
	return (0);
}

/*
 * sam_db_dumplist - Generate file list for samfsdump.
 *
 *	Description:
 *	    Generate list of files for samfsdump.  File list is
 *	    written to OutputFile or to standard out if an output
 *          file is not supplied.
 *
 *	On Entry:
 *	    mp		Mount point.
 *
 *	Returns:
 *	    Number of records outputed if no errors, else -1
 *	    if mySQL query failure.
 */
static my_ulonglong
sam_db_dumplist(
	char *mp)	/* Mount point */
{
	my_ulonglong rc;	/* Record count from query */
	unsigned long long rcc;	/* Record count processed */

	MYSQL_RES *res;	/* mySQL fetch results */
	MYSQL_ROW row;	/* mySQL row result */
	char *q;

	/*
	 * Example:
	 *	SELECT count(sam_archive.ino), sum(sam_inode.size)
	 *	FROM sam_archive JOIN sam_inode
	 *	ON sam_inode.ino=sam_archive.ino
	 *	AND sam_inode.gen=sam_archive.gen
	 *  AND vsn_id=9;
	 */
	q = strmov(SAMDB_qbuf, "SELECT ino, gen, path, obj FROM ");
	q = strmov(q, T_SAM_PATH);
	q = strmov(q, " WHERE NOT deleted;");
	*q = '\0';

	if (SAMDB_Debug) {
		fprintf(stderr, "%s\n", SAMDB_qbuf);
	}

	if ((OutputFile != NULL) && (output = fopen(OutputFile, "w")) == NULL) {
		error(0, errno, "%s", OutputFile);
		error(0, 0, catgets(catfd, SET, 1856,
		    "Open failed on (%s)"), OutputFile);
		OutputFile = NULL;
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		error(0, 0, "select fail [%s]: %s", T_SAM_VSNS,
		    mysql_error(SAMDB_conn));
		return ((my_ulonglong) -1);
	}

	res = mysql_use_result(SAMDB_conn);

	if (res == NULL) {
		error(0, 0, "results fail [%s]: %s",
		    T_SAM_PATH, mysql_error(SAMDB_conn));
		return ((my_ulonglong) -1);
	}

	rcc = 0;

	for (rcc = 0; (row = mysql_fetch_row(res)) != NULL; rcc++) {
		if (row[0] == NULL || row[1] == NULL) {
			continue;
		}
		if (row[2] == NULL || row[3] == NULL) {
			continue;
		}
		if (!PathOnly) {
			if (OutputFile != NULL) {
				fprintf(output, "%s\t%s\t", row[0], row[1]);
			} else {
				printf("%s\t%s\t", row[0], row[1]);
			}
		}
		if (OutputFile != NULL) {
			fprintf(output, "%s/%s%s\n", mp, row[2], row[3]);
		} else {
			printf("%s/%s%s\n", mp, row[2], row[3]);
		}
	}

	rc = mysql_num_rows(res);

	if (rcc != rc) {
		error(0, 0, "missing rows, expected: %d, got: %d",
		    (int)rc, (int)rcc);
	}

	mysql_free_result(res);

	return (rcc);
}

static void program_help(void) {
	printf("Usage:\t%s %s\n", program_name,
	    "fs_name [-f file name] [-v|V]");
	exit(-1);
}
