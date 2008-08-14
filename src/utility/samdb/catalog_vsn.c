/*
 * ----	samdb_catvsn - Generate a list of files archived on VSN.
 *
 *	Description:
 *	    Generate list of files archived to specified VSN.  List
 *	    written to standard output.
 *
 *	Command Syntax:
 *
 *	    samdb_catvsn family_set [-c n] [-m media_type] [-nostore]
 *				[-v|V] vsn(s)
 *
 *	    where:
 *
 *		vsn(s)		Specifies a list of VSNs to catalog.  VSNs are
 *				specified in "mt:vsn" or as "vsn" format,
 *				where "mt" is the media type.  If no media type
 *				is specified then the either the default media
 *				type (-m media_type) is used or data base
 *				is searched for a matching VSN with a
 *				single entry.
 *
 *		family_set	Specifies the family set name.
 *
 *		-c n		Archive copy. List files for this archive copy
 *				only.
 *
 *		-m media_type	Default media type.
 *
 *		-nostore	Instruct mySQL to not use stored results.
 *				This mode is recommened for catalogs with
 *				a large number of entries.  The program
 *				prevents updates to the database in this
 *				mode while the output is being generated.
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

/*
 * Include files:
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define	DEC_INIT
#include <sam/sam_db.h>

#include "arg.h"
#include "util.h"

/*
 * Global tables & pointers:
 */
char *program_name;	/* Program name: used by error	*/
int exit_status = 0;	/* Exit status			*/

int StoreResults;	/* Use mysql_store_result	*/
int Level;		/* Select specific archive copy	*/
char *DefaultMedia;	/* Default media if unspecified	*/
char *fs_name;		/* Family set name		*/

static my_ulonglong sam_db_catalog_vsn(int, int);
static void program_help(void);

arg_t Arg_Table[] = { /* Argument table	*/
	{ "-c", ARG_INT, (char *)&Level },
	{ "-m", ARG_STR, (char *)&DefaultMedia },
	{ "-nostore", ARG_TRUE, (char *)&StoreResults },
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
	int vsn_id;			/* VSN id */
	char *media; 			/* Media type */
	char *vsn; 			/* VSN being queried */
	my_ulonglong rc; 		/* Returned record count */
	int i;

	/* ---	Process arguments: */
	program_name = "samdb_catvsn";
	Level = -1;
	DefaultMedia = NULL;
	StoreResults = 0;

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

	if (argc < 1) {
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

	/* ---	Process VSNs: --- */
	while (argc-- != 0) {
		media = DefaultMedia;
		vsn_id = get_vsn(*argv++, &media, &vsn);
		if (vsn_id == 0) {
			error(1, 0, "VSN not found for %s:%s",
			    media == NULL ? "??" : media, vsn);
		}
		if (vsn_id < 0) {
			error(1, 0, "Specify media, multiple items"
			    " found for %s", vsn);
		}
		if (SAMDB_Verbose) {
			error(0, 0, "Cataloging %s:%s, vsn_id=%d",
			    media, vsn, vsn_id);
		}

		rc = sam_db_catalog_vsn(vsn_id, Level);

		if (SAMDB_Verbose && rc != (my_ulonglong)-1) {
			error(0, 0, "%llu files on %s:%s", rc, media, vsn);
		}
	}

	sam_db_disconnect();

	exit(exit_status);
	return (0);
}

/*
 * sam_db_catalog_vsn - Catalog specified VSN.
 *
 *	Description:
 *	    Generate list of files archived to specified VSN.  List
 *	    written to standard output.
 *
 *	On Entry:
 *	    id		VSN record ordinal from sam_vsns table.
 *	    level	Archive copy. If > 0, list only files for specified
 *			archive copy.
 *
 *	Returns:
 *	    Zero if no errors, else -1 if mySQL query failure.
 */
static my_ulonglong
sam_db_catalog_vsn(
	int id,			/* VSN record ordinal */
	int level)		/* Level number */
{
	my_ulonglong rc;	/* Record count from query */
	unsigned long long rcc;	/* Record count processed */

	MYSQL_RES *res;	/* mySQL fetch results */
	MYSQL_ROW row;	/* mySQL row result */
	char *q;

	/*
	 * 	Example:
	 *	SELECT count(sam_archive.ino), sum(sam_inode.size)
	 *	FROM sam_archive JOIN sam_inode
	 *	ON sam_inode.ino=sam_archive.ino
	 *  AND sam_inode.gen=sam_archive.gen
	 *  AND vsn_id=9;
	 */
	q = strmov(SAMDB_qbuf, "SELECT ");
	q = strmov(q, T_SAM_PATH);
	q = strmov(q, ".path, ");
	q = strmov(q, T_SAM_PATH);
	q = strmov(q, ".obj FROM ");
	q = strmov(q, T_SAM_ARCHIVE);
	q = strmov(q, ", ");
	q = strmov(q, T_SAM_INODE);
	q = strmov(q, ", ");
	q = strmov(q, T_SAM_PATH);
	sprintf(q, " WHERE %s.vsn_id='%d'", T_SAM_ARCHIVE, id);
	q = strend(q);
	if (level >= 0) {
		sprintf(q, " AND %s.copy='%d'", T_SAM_ARCHIVE, level);
	}
	q = strend(q);
	q = strmov(q, " AND ");
	q = strmov(q, T_SAM_ARCHIVE);
	q = strmov(q, ".ino=");
	q = strmov(q, T_SAM_INODE);
	q = strmov(q, ".ino AND ");
	q = strmov(q, T_SAM_ARCHIVE);
	q = strmov(q, ".gen=");
	q = strmov(q, T_SAM_INODE);
	q = strmov(q, ".gen AND ");
	q = strmov(q, T_SAM_ARCHIVE);
	q = strmov(q, ".ino=");
	q = strmov(q, T_SAM_PATH);
	q = strmov(q, ".ino AND ");
	q = strmov(q, T_SAM_ARCHIVE);
	q = strmov(q, ".gen=");
	q = strmov(q, T_SAM_PATH);
	q = strmov(q, ".gen AND NOT ");
	q = strmov(q, T_SAM_ARCHIVE);
	q = strmov(q, ".recycled AND NOT ");
	q = strmov(q, T_SAM_INODE);
	q = strmov(q, ".deleted");
	q = strmov(q, ";");
	*q = '\0';

	if (SAMDB_Debug) {
		fprintf(stderr, "%s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		error(0, 0, "select fail [%s]: %s", T_SAM_VSNS,
		    mysql_error(SAMDB_conn));
		return ((my_ulonglong) -1);
	}

	res = StoreResults ? mysql_store_result(SAMDB_conn) :
	    mysql_use_result(SAMDB_conn);

	if (res == NULL) {
		error(0, 0, "results fail [%s]: %s",
		    T_SAM_VSNS, mysql_error(SAMDB_conn));
		return ((my_ulonglong) -1);
	}

	rc = mysql_num_rows(res);
	rcc = 0;

	for (rcc = 0; (row = mysql_fetch_row(res)) != NULL; rcc++) {
		if (row[0] == NULL || row[1] == NULL) {
			continue;
		}
		printf("%s%s\n", row[0], row[1]);
	}

	if (!StoreResults) {
		rc = mysql_num_rows(res);
	}

	if (rcc != rc) {
		error(0, 0, "missing rows, expected: %d, got: %d",
		    (int)rc, (int)rcc);
	}

	mysql_free_result(res);

	return (rcc);
}

void
program_help(void)
{
	printf("Usage:\t%s %s\n", program_name,
	    "fs_name [-c copy_num] [-m media_type]"
	    " [-nostore] [-v|V] vsn(s)");
	exit(-1);
}
