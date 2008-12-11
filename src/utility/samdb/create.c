/*
 * create.c - create a sideband database for a filesystem
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
#pragma ident "$Revision: 1.2 $"

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include <sam/sam_db.h>
#include "samdb.h"

static int process_opt(char opt, char *arg);
static int create_database(sam_db_context_t *con, char *db_name);
static int execute_schema(sam_db_context_t *con, FILE *file);

static char *schema = SAMDB_SCHEMA_FILE;

/*
 * Creates the database.
 *
 * First we must execute the create database command, then we must
 * reconnect to the new database and execute the schema in order to
 * create the tables.
 */
int
samdb_create(samdb_args_t *args) {
	sam_db_context_t *con = args->con;
	char *db_name = con->dbname;
	FILE *schema_file = fopen(schema, "r");
	int err = 0;

	if (schema_file == NULL) {
		fprintf(stderr, "Could not open schema file %s.\n", schema);
		err++;
		goto out;
	}

	/* Connect to mysql to create database */
	con->dbname = NULL;
	if (sam_db_connect(con) < 0) {
		fprintf(stderr, "Could not connect to server %s.\n", con->host);
		err++;
		goto out;
	}
	if (create_database(con, db_name) < 0) {
		fprintf(stderr, "Could not create database.\n");
		err++;
		goto out;
	}
	(void) sam_db_disconnect(con);

	/* Connect to the new database and create tables */
	con->dbname = db_name;
	if (sam_db_connect(con) < 0) {
		fprintf(stderr, "Could not connect to database.\n");
		err++;
		goto out;
	}
	if (execute_schema(con, schema_file) < 0) {
		fprintf(stderr, "Error creating database schema.\n");
	}
out:
	(void) sam_db_disconnect(con);
	if (schema_file != NULL) {
		fclose(schema_file);
	}
	return (err ? -1 : 0);
}

samdb_opts_t *
samdb_create_getopts(void) {
	static opt_desc_t opt_desc[] = {
		{"s schema", "Mysql schema default: "SAMDB_SCHEMA_FILE},
		NULL
	};
	static samdb_opts_t create_opts = {
		"s:",
		process_opt,
		opt_desc
	};
	return (&create_opts);
}

static int
process_opt(char opt, char *arg) {
	switch (opt) {
	case 's':
		schema = arg;
		break;
	default:
		return (-1);
	}

	return (0);
}

/* Create the database db_name using SQL. */
static int
create_database(sam_db_context_t *con, char *db_name) {
	int len = sprintf(con->qbuf, "create database if not exists %s "
	    "character set latin1;", db_name);
	if (mysql_real_query(con->mysql, con->qbuf, len)) {
		fprintf(stderr, "%s\n", mysql_error(con->mysql));
		return (-1);
	}
	return (0);
}

/*
 * Execute the schema DDL from the provided file stream.
 * We read SAMDB_SQL_MAXLEN at a time until the whole file is
 * executed.
 */
static int
execute_schema(sam_db_context_t *con, FILE *file) {
	int offset = 0;
	while (fread(con->qbuf + offset, 1,
	    SAMDB_SQL_MAXLEN - offset, file) > 0) {
		char *start = con->qbuf;
		char *end = strchr(con->qbuf, ';');

		while (end != NULL) {
			offset = end - start;
			if (mysql_real_query(con->mysql, start, offset)) {
				fprintf(stderr, "%s\n",
				    mysql_error(con->mysql));
				return (-1);
			}
			start = end + 1;
			end = strchr(start, ';');
		}
		offset = SAMDB_SQL_MAXLEN - (start - con->qbuf);
		memmove(con->qbuf, start, offset);
	}

	return (0);
}
