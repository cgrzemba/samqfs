/*
 * ----	samdb_updatevsn - Update VSN file residency counts.
 *
 *	Description:
 *	    Print all records for a given list of inodes.
 *
 *	Command Syntax:
 *
 *	    samdb_updatevsn family_set -x expired_time [-a] [-v|-V] vns(s)
 *
 *	    where:
 *
 *		ino(s)		Inode numbers.
 *
 *		family_set	Specifies the family set name.
 *
 *		-a		Update all VSNs.
 *
 *		-x expired_time
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
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "arg.h"
#include "util.h"

#define	DEC_INIT
#include <sam/sam_db.h>

/* Global tables & pointers: */
char *program_name;	/* Program name: used by error */
int exit_status = 0;	/* Exit status */

static int UpdateAll;		/* Update all VSNs */
static time_t Current_Time;	/* Current time */
static time_t Expired_time;	/* Relative expired time */
static char *fs_name;		/* Family set name */

extern int get_vsn(char *, char **, char **);

static void program_help(void);
static int sam_db_update_vsn(int, char *, char *, time_t);

arg_t Arg_Table[] = { /* Argument table	*/
	{ "-a", ARG_TRUE, (char *)&UpdateAll },
	{ "-x", ARG_TIME, (char *)&Expired_time },
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
	time_t expired_time;		/* Dormaent/expired timestamp */
	char *media;			/* Media type */
	char *vsn;			/* VSN */
	int vsn_id;			/* VSN table ordinal */
	int i;

	/* ---	Process arguments: */
	program_name = "samdb_updatevsn";
	UpdateAll = 0;
	Expired_time = 0;
	Current_Time = time(0);

	if (argc < 2) {
		program_help();
	}

	fs_name = *++argv;
	argc--;

	init_trace(NULL, 0);

	sam_db_init_vsn_cache();

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

	if (Expired_time == 0) {
		error(1, 0, "Expired time threshold not"
		    "specified (-e)", 0);
	}

	expired_time = Current_Time - Expired_time;

	if (SAMDB_Verbose) {
		error(0, 0, "Expired timestamp %u", expired_time);
	}
	if (!UpdateAll && argc < 1) {
		error(1, 0, "No VSNs specified.", 0);
	}

	/* ---	Connect to database: */
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

	if (sam_db_load_vsns() < 0) {
		error(0, 0, "Errors loading VSNs from db", 0);
	}

	if (SAMDB_Verbose) {
		error(0, 0, "VSNs loaded from db: %d", n_vsns);
	}

	/* ---	Process VSNs: */
	if (UpdateAll || argc == 0) { /* Update all VSNs */
		for (i = 0; i < n_vsns; i++) {
			(void) sam_db_update_vsn(VSN_Cache[i].vsn_id,
			    VSN_Cache[i].media, VSN_Cache[i].vsn,
			    expired_time);
		}
		sam_db_disconnect();
		exit(exit_status);
	}

	while (argc-- != 0) {
		media = NULL;
		vsn_id = get_vsn(*argv++, &media, &vsn);
		if (vsn_id == 0) {
			error(1, 0, "VSN not found for %s:%s",
			    media == NULL ? "??" : media, vsn);
		}
		if (vsn_id < 0) {
			error(1, 0, "Specifiy media, multiple "
			    "items found for %s", vsn);
		}
		if (SAMDB_Verbose) {
			error(0, 0, "Updating %s:%s, vsn_id=%d",
			    media, vsn, vsn_id);
		}

		(void) sam_db_update_vsn(vsn_id, media, vsn, expired_time);
	}

	sam_db_disconnect();
	exit(exit_status);
	return (0);
}

/*
 * sam_db_update_vsn - Update counters for VSN.
 *
 *	Description:
 *	    sam_db_update_vsn updates the file count/size counters
 *	    for the specified VSN.
 *
 *	On Entry:
 *	    id		VSN identifier.
 *	    media	Media type.
 *	    vsn		VSN.
 *	    expired_time	Dormant/expired threshold.
 *
 *	Returns
 *	    -1 if errors.
 */
static int
sam_db_update_vsn(
	int id, 	/* VSN record ordinal */
	char *media, 	/* Media type */
	char *vsn, 	/* VSN */
	time_t expired_time) /* Dormant/expired threshold */
{
	my_ulonglong rc; 	/* Record count from query */
	unsigned long fc; 	/* Field count */

	MYSQL_RES *res; 	/* mySQL fetch results */
	MYSQL_ROW row;		/* mySQL row result */
	int err = 0; 		/* Error count */
	char *q;
	int i;

	/*
	 * Example:
	 * SELECT count(1),
	 *	sum(sam_archive.size),
	 *	sum(if(NOT sam_inode.deleted AND
	 *		NOT sam_archive.recycled AND
	 *		sam_archive.modify_time=sam_inode.modify_time,1,0)),
	 *	sum(if(NOT sam_inode.deleted AND
	 *		NOT sam_archive.recycled AND
	 *		sam_archive.modify_time=sam_inode.modify_time,
	 *		sam_archive.size,0)),
	 *	sum(if(NOT sam_archive.recycled AND
	 *		sam_archive.modify_time<>sam_inode.modify_time AND
	 *		sam_archive.modify_time>=1035297919,1,0)),
	 *	sum(if(NOT sam_archive.recycled AND
	 *		sam_archive.modify_time<>sam_inode.modify_time AND
	 *		sam_archive.modify_time>=1035297919,
	 *		sam_archive.size,0)),
	 *	sum(if(NOT sam_archive.recycled AND
	 *		sam_archive.modify_time<>sam_inode.modify_time AND
	 *		sam_archive.modify_time<1035297919,1,0)),
	 *		sum(if(NOT sam_archive.recycled AND
	 *		sam_archive.modify_time<>sam_inode.modify_time AND
	 *		sam_archive.modify_time<1035297919,sam_archive.size,0)),
	 *	sum(if(sam_archive.recycled,1,0)),
	 *	sum(if(sam_archive.recycled,sam_archive.size,0))
	 * FROM	sam_archive JOIN sam_inode
	 * ON	sam_inode.ino=sam_archive.ino AND
	 *	sam_inode.gen=sam_archive.gen AND
	 *	vsn_id=530;
	 */

	/* total count/size */
	q = strmov(SAMDB_qbuf, "SELECT count(1), ");
	sprintf(q, "sum(%s.size), ", T_SAM_ARCHIVE);
	q = strend(q);

	/* active count/size */
	sprintf(q, "sum(if(NOT %s.deleted AND NOT %s.recycled AND ",
	    T_SAM_INODE, T_SAM_ARCHIVE);
	q = strend(q);
	sprintf(q, "%s.modify_time=%s.modify_time,1,0)), ",
	    T_SAM_ARCHIVE, T_SAM_INODE);
	q = strend(q);
	sprintf(q, "sum(if(NOT %s.deleted AND NOT %s.recycled AND ",
	    T_SAM_INODE, T_SAM_ARCHIVE);
	q = strend(q);
	sprintf(q, "%s.modify_time=%s.modify_time,%s.size,0)), ",
	    T_SAM_ARCHIVE, T_SAM_INODE, T_SAM_ARCHIVE);
	q = strend(q);

	/* dormant count/size */
	sprintf(q, "sum(if(NOT %s.recycled AND ", T_SAM_ARCHIVE);
	q = strend(q);
	sprintf(q, "%s.modify_time<>%s.modify_time AND ",
	    T_SAM_ARCHIVE, T_SAM_INODE);
	q = strend(q);
	sprintf(q, "%s.modify_time>=%lu,1,0)), ", T_SAM_ARCHIVE, expired_time);
	q = strend(q);
	sprintf(q, "sum(if(NOT %s.recycled AND ", T_SAM_ARCHIVE);
	q = strend(q);
	sprintf(q, "%s.modify_time<>%s.modify_time AND ",
	    T_SAM_ARCHIVE, T_SAM_INODE);
	q = strend(q);
	sprintf(q, "%s.modify_time>=%lu,%s.size,0)), ",
	    T_SAM_ARCHIVE, expired_time, T_SAM_ARCHIVE);
	q = strend(q);

	/* expired count/size */
	sprintf(q, "sum(if(NOT %s.recycled AND ", T_SAM_ARCHIVE);
	q = strend(q);
	sprintf(q, "%s.modify_time<>%s.modify_time AND ",
	    T_SAM_ARCHIVE, T_SAM_INODE);
	q = strend(q);
	sprintf(q, "%s.modify_time<%lu,1,0)), ", T_SAM_ARCHIVE, expired_time);
	q = strend(q);
	sprintf(q, "sum(if(NOT %s.recycled AND ", T_SAM_ARCHIVE);
	q = strend(q);
	sprintf(q, "%s.modify_time<>%s.modify_time AND ",
	    T_SAM_ARCHIVE, T_SAM_INODE);
	q = strend(q);
	sprintf(q, "%s.modify_time<%lu,%s.size,0)), ",
	    T_SAM_ARCHIVE, expired_time, T_SAM_ARCHIVE);
	q = strend(q);

	/* recycled count/size */
	sprintf(q, "sum(if(%s.recycled,1,0)), ", T_SAM_ARCHIVE);
	q = strend(q);
	sprintf(q, "sum(if(%s.recycled,%s.size,0))",
	    T_SAM_ARCHIVE, T_SAM_ARCHIVE);
	q = strend(q);

	q = strmov(q, " FROM ");
	q = strmov(q, T_SAM_ARCHIVE);
	q = strmov(q, " JOIN ");
	q = strmov(q, T_SAM_INODE);
	q = strmov(q, " ON vsn_id='");
	sprintf(q, "%d", id);
	q = strend(q);
	q = strmov(q, "' AND ");
	q = strmov(q, T_SAM_ARCHIVE);
	q = strmov(q, ".ino=");
	q = strmov(q, T_SAM_INODE);
	q = strmov(q, ".ino AND ");
	q = strmov(q, T_SAM_ARCHIVE);
	q = strmov(q, ".gen=");
	q = strmov(q, T_SAM_INODE);
	q = strmov(q, ".gen");
	q = strmov(q, ";");
	*q = '\0';

	if (SAMDB_Debug) {
		fprintf(stderr, "Query: %s\n", SAMDB_qbuf);
	}
	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf, strlen(SAMDB_qbuf))) {
		error(0, 0, "select fail [%s]: %s:%s, %s",
		    T_SAM_VSNS, media, vsn, mysql_error(SAMDB_conn));
		return (-1);
	}

	if ((res = mysql_store_result(SAMDB_conn)) == NULL) {
		error(0, 0, "results fail [%s]: %s:%s, %s",
		    T_SAM_VSNS, media, vsn, mysql_error(SAMDB_conn));
		return (-1);
	}

	rc = mysql_num_rows(res);
	fc = mysql_num_fields(res);

	if (fc != 10) {
		error(0, 0, "results fail [%s]: %s:%s field count"
		    "expected 10, got %d", T_SAM_VSNS, media, vsn, fc);
		return (-1);
	}

	if (rc == 0) {
		error(0, 0, "No archive entries for [%s]: %s:%s",
		    T_SAM_VSNS, media, vsn);
		goto novsns;
	}

	if (rc != 1) {
		error(0, 0, "multiple entries for [%s]: %s:%s",
		    T_SAM_VSNS, media, vsn);
	}

	row = mysql_fetch_row(res);

	if (row == NULL) {
		error(0, 0, "Mising row [%s]: %s:%s", T_SAM_VSNS, media, vsn);
		err++;
		goto novsns;
	}

	for (i = 0; i < 10; i++) {
		if (row[i] == NULL) {
			error(0, 0, "Mising result #%d [%s]: %s:%s",
			    i, T_SAM_VSNS, media, vsn);
			err++;
		}
	}

novsns:
	if (err != 0 || rc == 0) {
		mysql_free_result(res);
		return (err != 0 ? -1 : (int)rc);
	}

	if (SAMDB_Verbose) {
		error(0, 0, "%s:%s total files: %s, total size: %s",
		    media, vsn, row[0], row[1]);
	}

	q = strmov(SAMDB_qbuf, "UPDATE ");
	q = strmov(q, T_SAM_VSNS);
	q = strmov(q, " SET ");
	q = strmov(q, " files_active='");
	q = strmov(q, row[2]);
	q = strmov(q, "', size_active='");
	q = strmov(q, row[3]);
	q = strmov(q, "', files_dorment='");
	q = strmov(q, row[4]);
	q = strmov(q, "', size_dorment='");
	q = strmov(q, row[5]);
	q = strmov(q, "', files_expired='");
	q = strmov(q, row[6]);
	q = strmov(q, "', size_expired='");
	q = strmov(q, row[7]);
	q = strmov(q, "', files_recycled='");
	q = strmov(q, row[8]);
	q = strmov(q, "', size_recycled='");
	q = strmov(q, row[9]);
	sprintf(q, "' WHERE id='%d'", id);
	q = strend(q);
	q = strmov(q, ";");
	*q = '\0';

	mysql_free_result(res);

	if (SAMDB_Debug) {
		fprintf(stderr, "Query: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		error(0, 0, "update fail [%s]: %s:%s, %s",
		    T_SAM_VSNS, media, vsn, mysql_error(SAMDB_conn));
		return (-2);
	}

	rc = mysql_affected_rows(SAMDB_conn);

	if (rc != 1) {
		error(0, 0, "update none [%s]: %s:%s",
		    T_SAM_VSNS, media, vsn);
		return (-2);
	}

	return (0);
}

static void
program_help(void)
{
	printf("Usage:\t%s %s\n", program_name,
	    "fs_name -x expired_time [-a] [-v|V] [vsn(s)]");
	exit(-1);
}
