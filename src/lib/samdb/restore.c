/*
 * --	restore.c - mySQL restore function for SAM-db.
 *
 *	Descripton:
 *	    restore.c resolves inode/generation number changes
 *	    in the database.  This occurs typically after a
 *	    samfsdump/restore operation.
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

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sam/sam_malloc.h>
#include <sam/sam_trace.h>
#include <sam/sam_db.h>

/*
 * --	sam_db_restore - Restore Database Entries.
 *
 *	Description:
 *	    sam_db_restore remaps inode/generation number for the
 *	    specified path/object.  This is done by searching the
 *	    path table to get the inode/generation number pre-restore
 *	    and then update each table to is newly assigned
 *	    inode/generation number.
 *
 *	On Entry:
 *	    ino		New inode number.
 *	    gen		New generation number.
 *	    type	File type.
 *	    path	Entry path name.
 *	    obj		Entry object name.
 *
 *	Returns:
 *	    Zero if no errors, -1 if error.
 */
my_ulonglong
sam_db_restore(
	unsigned int ino,	/* New inode number */
	unsigned int gen,	/* New generation number */
	sam_db_ftype type,	/* File type */
	char *path, 		/* Entry path name */
	char *obj)		/* Entry object name */
{
	my_ulonglong rc;	/* Record count from query */
	my_ulonglong rcc = 0;	/* Updated record count */
	sam_db_path_t *T_path; 	/* Path table */
	my_ulonglong n_path; 	/* Number of path entries */
	sam_db_path_t *p;

	n_path = sam_db_query_by_path(&T_path, path, obj);

	if (n_path == 0) {
		Trace(TR_MISC, "no path entries found for %s%s", path, obj);
		return ((my_ulonglong)-1);
	}

	if (n_path > 1) {
		Trace(TR_ERR, "multiple path entries found for %s%s",
		    path, obj);
	}

	p = T_path;

	if (p->type != type) {
		Trace(TR_ERR, "type mismatch for %s%s, expected %d, got %d",
		    path, obj, type, p->type);
	}

	rc = sam_db_restore_table(T_SAM_INODE, ino, gen, p->ino, p->gen);
	if (rc > 1) {
		Trace(TR_ERR, "multiple inode entries found for %s%s",
		    path, obj);
	}
	if (rc > 0) {
		rcc += rc;
	}

	rc = sam_db_restore_table(T_SAM_LINK, ino, gen, p->ino, p->gen);
	if (rc > 1) {
		Trace(TR_ERR, "multiple link entries found for %s%s",
		    path, obj);
	}
	if (rc > 0) {
		rcc += rc;
	}

	rc = sam_db_restore_table(T_SAM_ARCHIVE, ino, gen, p->ino, p->gen);
	if (rc > 0) {
		rcc += rc;
	}

	rc = sam_db_restore_table(T_SAM_PATH, ino, gen, p->ino, p->gen);
	if (rc > 0) {
		rcc += rc;
	}

	return (rcc);
}

/*
 * sam_db_restore_table - Remap Inode/Generation for Specified  Table.
 *
 *	Description:
 *	    Update specified table remaping inode/generation numbers.
 *
 *	On Entry:
 *	    table	Table name.
 *	    n_ino/n_gen	New inode/generation numbers.
 *	    o_ino/o_gen	Old inode/generation numbers.
 *
 *	Returns:
 *	     Number of records updated, -1 if error.
 */
my_ulonglong
sam_db_restore_table(
	char *table,		/* Table name to update	*/
	unsigned int n_ino, /* New inode number */
	unsigned int n_gen, /* New generation number */
	unsigned int o_ino,	/* Old inode number */
	unsigned int o_gen)	/* Old generation number */
{
	char *q;

	q = strmov(SAMDB_qbuf, "UPDATE ");
	q = strmov(q, table);
	sprintf(q, "' SET ino=%u, gen=%u", n_ino, n_gen);
	q = strend(q);
	sprintf(q, "' WHERE ino=%u AND gen=%u", o_ino, o_gen);
	q = strend(q);
	q = strmov(q, ";");
	*q = '\0';

	if (SAMDB_Verbose) {
		printf("QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		Trace(TR_ERR, "update fail[%s] ino=%u.%u: %s",
		    table, n_ino, n_gen, mysql_error(SAMDB_conn));
		sam_db_query_error();
		return ((my_ulonglong)-1);
	}

	return (mysql_affected_rows(SAMDB_conn));
}

/*
 * --	sam_db_query_by_path - mySQL query for sam path table.
 *
 *	Description:
 *	    Return the contents of the path table for the specified path/object.
 *
 *	On Entry:
 *	    ino		Inode number to query.
 *	    gen		Generation number to optionally query.  If 0, all
 *			inodes matching ino are printed.
 *
 *	On Return:
 *	    t		Pointer to table of found records (dynamicly
 *			allocated).
 *
 *	Returns:
 *	    Number of records found.  -1 if error.
 */
my_ulonglong
sam_db_query_by_path(
	sam_db_path_t **t,	/* Table buffer */
	char *path,		/* Path name */
	char *obj)		/* Object name */
{
	MYSQL_RES *res;		/* mySQL fetch results */
	MYSQL_ROW row;		/* mySQL row result */
	my_ulonglong rc;	/* Record count from query */
	my_ulonglong rcc; 	/* Record count from query */
	char *q;
	sam_db_path_t *v;

	q = strmov(SAMDB_qbuf, "SELECT ino, gen, type FROM ");
	q = strmov(q, T_SAM_PATH);
	q = strmov(q, " WHERE NOT deleted AND path='");
	q += mysql_real_escape_string(SAMDB_conn, q, path, strlen(path));
	q = strmov(q, "' AND obj='");
	q += mysql_real_escape_string(SAMDB_conn, q, obj, strlen(obj));
	q = strmov(q, "';");
	*q = '\0';

	if (SAMDB_Debug) {
		Trace(TR_DEBUG, "QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		Trace(TR_ERR, "select fail [%s]: %s%s, %s",
		    T_SAM_PATH, path, obj, mysql_error(SAMDB_conn));
		sam_db_query_error();
		return ((my_ulonglong)-1);
	}

	if ((res = mysql_store_result(SAMDB_conn)) == NULL) {
		Trace(TR_ERR, "results fail [%s]: %s%s, %s",
		    T_SAM_PATH, path, obj, mysql_error(SAMDB_conn));
		return ((my_ulonglong)-1);
	}

	rc = mysql_num_rows(res);

	if (rc == 0) {
		rcc = 0;
		goto out;
	}

	SamMalloc(*t, (int)rc * sizeof (sam_db_path_t));
	memset((char *)*t, 0, (int)rc * sizeof (sam_db_path_t));

	for (rcc = 0; (row = mysql_fetch_row(res)) != NULL; rcc++) {
		/* overflow	*/
		if (rcc == rc) {
			Trace(TR_ERR, "excess rows, expected: %llu", rc);
			break;
		}

		v = (*t) + rcc;
		v->ino = strtoul(row[0], (char **)NULL, 10);
		v->gen = strtoul(row[1], (char **)NULL, 10);
		v->type = strtol(row[2], (char **)NULL, 10);
	}

	if (rcc != rc) {
		Trace(TR_ERR, "missing rows, expected: %llu, got: %llu",
		    rc, rcc);
	}

out:
	mysql_free_result(res);
	return (rcc);
}
