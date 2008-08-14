/*
 * --	rename.c - mySQL rename function for SAM-db.
 *
 *	Descripton:
 *	    rename.c resloves path changes in the database.
 */

/*
 *	SAM-QFS_notice_begin
 *
 *	CDDL HEADER START
 *
 *	The contents of this file are subject to the terms of the
 *	Common Development and Distribution License (the "License")
 *	You may not use this file except in compliance with the License.
 *
 *	You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 *	or http://www.opensolaris.org/os/licensing.
 *	See the License for the specific language governing permissions
 *	and limitations under the License.
 *
 *	When distributing Covered Code, include this CDDL HEADER in each
 *	file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 *	If applicable, add the following below this CDDL HEADER, with the
 *	fields enclosed by brackets "[]" replaced with your own identifying
 *	information: Portions Copyright [yyyy] [name of copyright owner]
 *
 *	CDDL HEADER END
 */
/*
 *	Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 *	Use is subject to license terms.
 *
 *	SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.1 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sam/sam_trace.h>
#include <sam/sam_db.h>

/*
 * --	sam_db_rename - Rename directory.
 *
 *	Description:
 *	    Insert path entry into sam_path.
 *
 *	On Entry:
 *	    spath	Source directory path.
 *	    sobj	Source object (directory) name.
 *	    tpath	Target directory path.
 *	    tobj	Target object (directory) name.
 *
 *	Returns:
 *	    Number of entries updated, -1 if error.
 */
my_ulonglong
sam_db_rename(
	char *spath,
	char *sobj,
	char *tpath,
	char *tobj)
{
	my_ulonglong rc; /* Record count from query	*/
	int lp; /* Source string length		*/
	int lo; /* Target string length		*/
	char *q;

	lp = strlen(spath);
	lo = strlen(sobj);
	q = strmov(SAMDB_qbuf, "UPDATE ");
	q = strmov(q, T_SAM_PATH);
	q = strmov(q, " SET path=concat('");
	q += mysql_real_escape_string(SAMDB_conn, q, tpath, strlen(tpath));
	q = strmov(q, "','");
	q += mysql_real_escape_string(SAMDB_conn, q, tobj, strlen(tobj));
	sprintf(q, "','/',substr(path,%d))", lp+lo+1);
	q = strend(q);
	q = strmov(q, " WHERE NOT deleted AND path LIKE '");
	q += mysql_real_escape_string(SAMDB_conn, q, spath, lp);
	q += mysql_real_escape_string(SAMDB_conn, q, sobj, lo);
	q = strmov(q, "/%';");
	*q = '\0';

	if (SAMDB_Debug) {
		Trace(TR_DEBUG, "QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		Trace(TR_ERR, "update fail [%s]: rename s=%s%s, t=%s%s, %s",
		    T_SAM_PATH, spath, sobj, tpath, tobj,
		    mysql_error(SAMDB_conn));
		sam_db_query_error();
		return (-1);
	}

	rc = mysql_affected_rows(SAMDB_conn);

	return (rc);
}
