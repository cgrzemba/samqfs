/*
 *  ----	samdb_listpath - Lookup up path.
 *
 *	Description:
 *	    Print all records for a given file path.
 *
 *	Command Syntax:
 *
 *	    samdb_listpath family_set [-p|e|b] [-nolabels]
 *				[-v|-V] path(s)
 *
 *	    where:
 *
 *		paths(s)	Paths to lookup.  Paths may be spcified
 *				either as full paths or as a path relative
 *				to the mount point.
 *
 *		family_set	Specifies the family set name.
 *
 *		-p		Lookup all records which partially
 *				match the specified path.
 *
 *		-e		Lookup all records which exactly match the
 *				specified path.
 *
 *		-b		Lookup all records which match both the
 *				path and file name for the specified path.
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

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* Include files: */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <mysql.h>

#include <sam/sam_malloc.h>
#define	DEC_INIT
#include <sam/sam_db.h>

#include "arg.h"
#include "util.h"

/* Global tables & pointers: */
char *program_name;	/* Program name: used by error */
int exit_status = 0;	/* Exit status */

int Labels; 	/* Print labels flag */
char *fs_name;	/* Family set name */

static void program_help(void);

arg_t Arg_Table[] = {	/* Argument table */
	{ "-nolabels", ARG_FALSE, (char *)&Labels },
	{ "-p", ARG_PART, (char *)2 },
	{ "-e", ARG_PART, (char *)3 },
	{ "-b", ARG_PART, (char *)4 },
	{ "-help", ARG_PART, (char *)1 },
	{ "-version", ARG_VERS, NULL },
	{ "-v", ARG_TRUE, (char *)&SAMDB_Verbose },
	{ "-V", ARG_TRUE, (char *)&SAMDB_Debug },
	{ NULL, ARG_PART, NULL } };

int main(
	int argc, 	/* Number of arguments */
	char *argv[])	/* Argument pointer list */
{
	sam_db_access_t *dba;	/* DB access information */
	sam_db_connect_t SAM_db; /* mySQL connect parameters */
	char *path; 		/* Path for query */
	char *obj; 		/* Object for query */
	int match = 0; 		/* Match type: partial/exact */
	char mp[128];		/* Mount point */
	int lmp;		/* Mount point string length */
	MYSQL_RES *res;		/* mySQL fetch results */
	MYSQL_ROW row; 		/* mySQL row result */
	MYSQL_FIELD *fields;	/* mySQL field information */
	my_ulonglong rc;	/* Record count from query */
	my_ulonglong rcc;	/* Record count from query */
	unsigned long fc;	/* Field count */
	char *q, *p;
	int i;

	/* ---	Process arguments: --- */
	program_name = "samdb_listpath";
	Labels = 1;

	if (argc < 2) {
		program_help();
	}

	fs_name = *++argv;
	argc--;

	init_trace(NULL, 0);

	while ((i = Process_Command_Arguments(Arg_Table, &argc, &argv)) > 0) {
		switch (i) {
		case 1:
			program_help();
			break;
		case 2:
			match = 0;
			break;
		case 3:
			match = 1;
			break;
		case 4:
			match = 2;
			break;
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
		error(1, 0, "No paths specified.", 0);
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

	strcpy(mp, dba->db_mount);
	lmp = strlen(mp);

	/* Append trailing / */
	if (mp[lmp-1] != '/') {
		mp[lmp++] = '/';
		mp[lmp] = '\0';
	}

	/* ---	Process paths: --- */
	while (argc-- != 0) {
		path = *argv++;
		obj = "";
		if (strncmp(path, mp, lmp) == 0) {
			path += lmp;
		}
		if (match == 2) {
			if ((p = strrchr(path, '/')) != NULL) {
				p = '\0';
				obj = p + 1;
			} else {
				obj = path;
				path = "";
			}
		}

		i = strlen(path);
		path = xstrdup2(path, i+1);
		if (path[i-1] != '/') {
			path[i++] = '/';
			path[i] = '\0';
		}

		if (SAMDB_Verbose) {
			fprintf(stderr, "lookup: path=%s obj=%s\n", path, obj);
		}

		q = strmov(SAMDB_qbuf, "SELECT * FROM ");
		q = strmov(q, T_SAM_PATH);

		/* if -p */
		if (match == 0) {
			q = strmov(q, " WHERE path LIKE '");
			q += mysql_real_escape_string(SAMDB_conn, q,
			    path, strlen(path));
			q = strmov(q, "%%'");
		} else {
			/* if -e or -b */
			q = strmov(q, " WHERE path='");
			q += mysql_real_escape_string(SAMDB_conn, q,
			    path, strlen(path));
			q = strmov(q, "'");
		}
		if (match == 2) {
			/* if -b */
			q = strmov(q, " AND obj='");
			q += mysql_real_escape_string(SAMDB_conn, q,
			    obj, strlen(obj));
			q = strmov(q, "'");
		}
		q = strend(q);
		q = strmov(q, ";");
		*q = '\0';

		if (SAMDB_Debug) {
			fprintf(stderr, "QUERY: %s\n", SAMDB_qbuf);
		}

		if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
		    (unsigned int)(q-SAMDB_qbuf))) {
			error(0, 0, "select fail: %s", mysql_error(SAMDB_conn));
		}

		if ((res = mysql_store_result(SAMDB_conn)) == NULL) {
			error(0, 0, "results fail: %s",
			    mysql_error(SAMDB_conn));
		}

		fc = mysql_num_fields(res);
		rc = mysql_num_rows(res);
		rcc = 0;

		if (rc == 0) {
			goto skip;
		}
		if (Labels) {
			fields = mysql_fetch_fields(res);
		}
		for (rcc = 0; (row = mysql_fetch_row(res)) != NULL; rcc++) {
			for (i = 0; i < fc; i++) {
				if (Labels) {
					printf("%s=", fields[i].name);
				}
				printf("\'%s\'", row[i] ? row[i] : "NULL");
				if (i != fc-1) {
					printf(", ");
				}
			}
			printf("\n");
		}

		if (rcc != rc) {
			error(0, 0, "missing rows, expected: %ld, got: %ld",
			    rc, rcc);
		}
skip:
		mysql_free_result(res);
		SamFree(path);
	}

	sam_db_disconnect();
	return (0);
}

void
program_help(void)
{
	printf("Usage:\t%s %s\n", program_name,
	    "fs_name [-p|-e|-b] [-nolabels] [-v|V] paths(s)");
	exit(-1);
}
