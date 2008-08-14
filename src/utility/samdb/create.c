/*
 * ----	samdb_create - Create SAM Database.
 *
 *
 *	Description:
 *	    Create database.
 *
 *	    Unless specified, schema file read from
 *	    /etc/opt/SUNWsamfs/samdb.schema
 *
 *	    Access to mySQL is determined by the parameters specified in
 *	    /etc/opt/SUNWsamfs/samdb.conf(4).  The name of the database
 *	    to create is also specified through this file.
 *
 *	Command Syntax:
 *
 *	    samdb_create family_set [-s schema_file] [-v|-V]
 *
 *	    where:
 *
 *		family_set	Specifies the family set name.
 *
 *		-s schema_file	Specifies the schema file to use.  The
 *				default file is /etc/opt/SUNWsamfs/samdb.schema.
 *				The schema file contains a series of
 *				CREATE TABLE commands.
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
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errmsg.h>

#include <sam/sam_malloc.h>
#define	DEC_INIT
#include <sam/sam_db.h>

#include "arg.h"
#include "util.h"

/* Global tables & pointers: */
char *program_name;		/* Program name: used by error */
static int exit_status = 0;	/* Exit status */

static Buffer in;		/* Input buffer */

static char *fs_name;		/* Family set name */
static char *schema_file;	/* Schema file name */

static void Process_Schema_Input(FILE *);
static int issue_query(char *, int);
static void program_help(void);
static int clean_string(char *);
static int sam_db_create_database(char *);

arg_t Arg_Table[] = { /* Argument table	*/
	{ "-s", ARG_STR, (char *)&schema_file },
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
	FILE *F;
	sam_db_access_t	*dba;		/* DB access information */
	sam_db_connect_t SAM_db;	/* mySQL connect parameters */
	int i;

	/* ---	Process arguments: --- */
	program_name = "samdb_create";
	schema_file = SAMDB_SCHEMA_FILE;
	in.bufl = L_IN_BUFFER;
	SamMalloc(in.buf, L_IN_BUFFER);

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

	SAM_db.SAM_host	= dba->db_host;
	SAM_db.SAM_user	= dba->db_user;
	SAM_db.SAM_pass	= dba->db_pass;
	SAM_db.SAM_name	= NULL;
	SAM_db.SAM_port	= *dba->db_port != '\0' ?
	    atoi(dba->db_port) : SAMDB_DEFAULT_PORT;
	SAM_db.SAM_client_flag = *dba->db_client != '\0' ?
	    atoi(dba->db_client) : SAMDB_CLIENT_FLAG;

	if (sam_db_connect(&SAM_db)) {
		error(1, 0, "cannot connect: %s", mysql_error(SAMDB_conn));
	}

	if ((F = fopen64(schema_file, "r")) == NULL) {
		error(1, errno, "Can't open (%s)", schema_file);
	}

	if (sam_db_create_database(dba->db_name) < 0) {
		error(1, 0, "Cannot create database [%s]", dba->db_name);
	}

	sam_db_disconnect();

	SAM_db.SAM_name	= dba->db_name;

	if (sam_db_connect(&SAM_db)) {
		error(1, 0, "cannot connect: %s", mysql_error(SAMDB_conn));
	}

	Process_Schema_Input(F);
	fclose(F);

	sam_db_disconnect();
	exit(exit_status);
	return (0);
}


/*
 *	Process_Schema_Input - Process Schema File.
 */
static void
Process_Schema_Input(
	FILE *F)	/* Input file descriptor */
{
	int k, l;
	char *p, *q;

	q = SAMDB_qbuf;
	l = 0;

	for (;;) {
		if (readin(F, &in) == EOF) {
			break;
		}
		if (*in.buf == ';' || *in.buf == '#') {
			continue;
		}

		for (p = in.buf; *p != '\0'; p++) {
			if (!isspace(*p)) {
				break;
			}
		}
		/* If blank line */
		if (*p == '\0') {
			continue;
		}

		k = strlen(in.buf);
		if (l + k + 1 > SAMDB_QBUF_LEN) {
			error(1, 0, "Input exceeds buffer length (%d)",
			    SAMDB_QBUF_LEN);
		}

		q = strmov(q, in.buf);
		q = strmov(q, " ");
		l += k+1;
		*q = '\0';

		/* If not complete	*/
		if (strrchr(in.buf, ';') == NULL) {
			continue;
		}

		l = clean_string(SAMDB_qbuf);

		if (l != strlen(SAMDB_qbuf)) {
			error(0, 0, "length error %d!=%d", l,
			    strlen(SAMDB_qbuf));
		}

		(void) issue_query(SAMDB_qbuf, l);
		q = SAMDB_qbuf;
		l = 0;
	}
}


/*
 * 	clean_string - Remove excess white space from string.
 *
 *	Description:
 *	    Remove excess white space from string.  Change all
 *	    space characters to ' '.
 *
 *	On Entry:
 *	    string	String to clean.
 *
 *	Returns:
 *	    Lenght of resulting string.
 */
static int
clean_string(
	char *string) /* Input string */
{
	char *s, *p;

	/* Skip leading */
	for (s = p = string; *s != '\0'; s++) {
		if (!isspace(*s)) {
			break;
		}
	}

	for (; *s != '\0'; s++) {
		if (isspace(*s)) {
			*s = ' ';
			if (p == string || *(p-1) == ' ') {
				continue;
			}
		}
		*p++ = *s;
	}
	*p = '\0';
	if (p != string && *(p-1) == ' ') {
		*--p = '\0';
	}

	return ((int)(p-string));
}


/*
 * issue_query - Issue Query.
 *
 *	Entry:
 *	    query	Query to issue;
 *	    qlen	Query string length.
 *
 *	Returns:
 *	    -1 if errors, otherwise creation status.
 */
static int
issue_query(
	char *query,	/* Query string */
	int qlen)	/* Query string length */
{
	if (SAMDB_Debug) {
		fprintf(stderr, "QUERY: %s\n", query);
	}

	if (mysql_real_query(SAMDB_conn, query, qlen)) {
		error(0, 0, "query fail: %s", mysql_error(SAMDB_conn));
		sam_db_query_error();
		return (-1);
	}

	return (mysql_affected_rows(SAMDB_conn));
}


void
program_help(void)
{
	printf("Usage:\t%s %s\n", program_name,
	    "fs_name [-s file] [-v|V]");
	exit(-1);
}


/*
 * sam_db_create_database - Create SAM Database.
 *
 *	Description:
 *	    Initial creation of SAM database.  See MYsql CREATE DATABASE
 *	    for further information.
 *
 *	Entry:
 *	    name	Database name.
 *
 *	Returns:
 *	    -1 if errors, otherwise creation status.
 */
static int
sam_db_create_database(
	char *name)		/* Database name */
{
	char *q;

	q = strmov(SAMDB_qbuf, "CREATE DATABASE IF NOT EXISTS ");
	q = strmov(q, name);
	q = strmov(q, " CHARACTER SET latin1;");
	*q = '\0';

	if (SAMDB_Debug) {
		fprintf(stderr, "QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		error(0, 0, "create fail [%s]: %s", name,
		    mysql_error(SAMDB_conn));
		sam_db_query_error();
		return (-1);
	}

	return (mysql_affected_rows(SAMDB_conn));
}
