/*
 * ----	samdb_listvsn - List vsns.
 *
 *	Description:
 *	    Print all VSNs.
 *
 *	Command Syntax:
 *
 *	    samdb_listvsn family_set [-m media_type] [-v|-V]
 *
 *	    where:
 *
 *		family_set	Specifies the family set name.
 *
 *		-m media_type	List only VSNs for this media type.
 *
 *		-help		Print command syntax summary and exit.
 *
 *		-version	Print program version and exit.
 *
 *		-v		Verbose mode.
 *
 *		-V		Verbose mode.
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

/*	Include files:	*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <mysql.h>

#define	DEC_INIT
#include <sam/sam_db.h>

#include "arg.h"
#include "util.h"

/* Global tables & pointers:  */
char *program_name;	/* Program name: used by error */
int exit_status = 0;	/* Exit status */

char *media;	/* Media type */
char *fs_name;	/* Family set name */

static void program_help(void);
static int sam_db_list_vsns(char *);

arg_t Arg_Table[] = {	/* Argument table	*/
	{ "-m", ARG_STR, (char *)&media },
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
	sam_db_access_t *dba;		/* DB access information */
	sam_db_connect_t SAM_db;	/* mySQL connect parameters */
	int i;

	/* ---	Process arguments: --- */
	program_name = "samdb_listvsn";
	media = NULL;

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
		error(1, 0, "family set name [%s] not found", fs_name);
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

	exit_status = sam_db_list_vsns(media);
	sam_db_disconnect();

	exit(exit_status);
	return (0);
}

int
sam_db_list_vsns(
	char *media)	/* Media type */
{
	my_ulonglong rc;	/* Record count from query */
	unsigned long long rcc;	/* Record count processed */
	unsigned long fc;	/* Field count */

	MYSQL_RES *res;	/* mySQL fetch results */
	MYSQL_ROW row;	/* mySQL row result */
	char *q;
	int err;	/* Error count	*/

	err = 0;

	q = strmov(SAMDB_qbuf, "SELECT id, media_type, vsn FROM ");
	q = strmov(q, T_SAM_VSNS);
	if (media != NULL) {
		q = strmov(q, " WHERE media_type='");
		q = strmov(q, media);
		q = strmov(q, "'");
	}
	q = strmov(q, " ORDER BY media_type, vsn");
	q = strmov(q, ";");
	*q = '\0';

	if (SAMDB_Debug) {
		fprintf(stderr, "%s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		error(0, 0, "select fail [%s]: %s", T_SAM_VSNS,
		    mysql_error(SAMDB_conn));
		return (-1);
	}

	if ((res = mysql_store_result(SAMDB_conn)) == NULL) {
		error(0, 0, "results fail [%s]: %s",
		    T_SAM_VSNS, mysql_error(SAMDB_conn));
		return (-1);
	}

	rc = mysql_num_rows(res);
	fc = mysql_num_fields(res);
	rcc = 0;

	if (fc != 3) {
		error(0, 0, "results fail [%s]: field count expected 3, got %d",
		    T_SAM_VSNS, fc);
		return (-1);
	}

	if (rc == 0) {
		goto novsns;
	}

	for (rcc = 0; (row = mysql_fetch_row(res)) != NULL; rcc++) {
		if (row[0] == NULL) {
			error(0, 0, "Mising id [%s] row=%ld", T_SAM_VSNS, rcc);
			err++;
			continue;
		}
		if (row[1] == NULL) {
			error(0, 0, "Mising media_type [%s] row=%ld",
			    T_SAM_VSNS, rcc);
			err++;
			continue;
		}
		if (row[2] == NULL) {
			error(0, 0, "Mising vsn [%s] row=%ld", T_SAM_VSNS, rcc);
			err++;
			continue;
		}

		printf("%s:%s\t%s\n", row[1], row[2], row[0]);
	}

novsns:
	mysql_free_result(res);

	if (rcc != rc) {
		error(0, 0, "missing rows, expected: %ld, got: %ld", rc, rcc);
	}

	if (err != 0) {
		error(0, 0, "rows with errors: %d", err);
	}

	return (err != 0 ? -2 : 0);
}

void
program_help(void)
{
	printf("Usage:\t%s %s\n", program_name,
	    "fs_name [-m media_type] [-v|V]");
	exit(-1);
}
