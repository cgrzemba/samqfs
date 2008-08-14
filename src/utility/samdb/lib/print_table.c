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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

#include <sam/sam_db.h>
#include <sam/sam_trace.h>

#include "util.h"

/*
 * --	print_table - Print table contents.
 *
 *	Description:
 *	    Print the contents of a given table for specified inode.
 *	    The generation number can be optionally specified.
 *
 *	On Entry:
 *	    F		File descriptor to print to.
 *	    table	Name of table to print.
 *	    ino		Inode number to query.
 *	    gen		Generation number to optionally query.  If 0, all
 *			inodes matching ino are printed.
 *	    label	Print labels.  If true, print field names.
 *
 *	Returns:
 *	    Number of records found.  -1 if error.
 */
int print_table(
	FILE *F,			/* File descriptor to output to */
	char *table,		/* Table name to print */
	unsigned int ino,	/* Inode number for query */
	unsigned int gen,	/* Generation number for query */
	int label) 			/* Show field labels */
{
	MYSQL_RES *res; 	/* mySQL fetch results */
	MYSQL_ROW row; 		/* mySQL row result */
	MYSQL_FIELD *fields; /* mySQL field information */
	my_ulonglong rc; 	/* Record count from query */
	my_ulonglong rcc; 	/* Record count from query */
	unsigned long fc; 	/* Field count */
	char *q;
	int i;

	q = strmov(SAMDB_qbuf, "SELECT * FROM ");
	q = strmov(q, table);
	q = strmov(q, " WHERE ino=");
	sprintf(q, "%u", ino);
	q = strend(q);
	if (gen != 0) {
		q = strmov(q, " AND gen=");
		sprintf(q, "%u", gen);
		q = strend(q);
	}
	q = strmov(q, ";");
	*q = '\0';

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		Trace(TR_ERR, "select fail: %s", mysql_error(SAMDB_conn));
		return (-1);
	}

	if ((res = mysql_store_result(SAMDB_conn)) == NULL) {
		Trace(TR_ERR, "results fail: %s", mysql_error(SAMDB_conn));
		return (-1);
	}

	rc = mysql_num_rows(res);

	if (rc == 0) {
		goto skip;
	}

	fc = mysql_num_fields(res);
	rcc = 0;

	if (label) {
		fields = mysql_fetch_fields(res);
	}

	for (rcc = 0; (row = mysql_fetch_row(res)) != NULL; rcc++) {
		fprintf(F, "%s:\t", table);
		for (i = 0; i < fc; i++) {
			if (label) {
				fprintf(F, "%s=", fields[i].name);
			}
			fprintf(F, "\'%s\'", row[i] ? row[i] : "NULL");
			if (i != fc-1) {
				fprintf(F, ", ");
			}
		}
		fprintf(F, "\n");
	}

	if (rcc != rc) {
		Trace(TR_ERR, "missing rows, expected: "
		    "%llu, got: %llu", rc, rcc);
	}

skip:
	mysql_free_result(res);

	return ((int)rc);
}
