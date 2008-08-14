/*
 *  --	query.c - mySQL query functions for SAM-db.
 *
 *	Descripton:
 *	    query.c is a collecton of routines which issue
 *	    SELECT queries to the corresponding SAM tables.
 *	    In general selection is by inode and generaton number.
 *
 *	Contents:
 *	    sam_db_query_path	- query SAM path table.
 *	    sam_db_query_inode	- query SAM inode table.
 *	    sam_db_query_link	- query SAM inode table.
 *	    sam_db_query_archive- query SAM inode table.
 *	    sam_db_row_count	- query row count.
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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sam/sam_malloc.h>
#include <sam/sam_trace.h>
#include <sam/sam_db.h>

/*
 * --	sam_db_query_path - mySQL query for sam path table.
 *
 *	Description:
 *	    Return the contents of the path table for the specified inode.
 *	    The generation number can be optionally specified.
 *
 *	On Entry:
 *	    ino		Inode number to query.
 *	    gen		Generation number to optionally query.  If 0, all
 *			inodes matching ino are printed.
 *
 *	On Return:
 *	    t		Pointer to table of found records.
 *
 *	Returns:
 *	    Number of records found.  -1 if error.
 */
my_ulonglong sam_db_query_path(
	sam_db_path_t **t,		/* Table buffer	*/
	unsigned long ino,	/* Inode number for query */
	unsigned long gen)	/* Generation number for query */
{
	MYSQL_RES *res;		/* mySQL fetch results */
	MYSQL_ROW row; 		/* mySQL row result */
	unsigned long *rowlens; /* mySQL row result strlen()'s */
	my_ulonglong rc; 	/* Record count from query */
	my_ulonglong rcc; 	/* Record count from query */
	char *q;
	sam_db_path_t *v;

	q = strmov(SAMDB_qbuf, "SELECT ino, gen, type, path, obj, initial_path,"
	    " initial_obj, deleted, delete_time FROM ");
	q = strmov(q, T_SAM_PATH);
	sprintf(q, " WHERE ino=%u", ino);
	q = strend(q);
	if (gen != 0) {
		sprintf(q, " AND gen=%u", gen);
		q = strend(q);
	}
	q = strmov(q, ";");
	*q = '\0';

	if (SAMDB_Debug) {
		Trace(TR_DEBUG, "QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		Trace(TR_ERR, "select fail [%s]: id=%u.%u, %s",
		    T_SAM_PATH, ino, gen, mysql_error(SAMDB_conn));
		sam_db_query_error();
		return ((my_ulonglong)-1);
	}

	if ((res = mysql_store_result(SAMDB_conn)) == NULL) {
		Trace(TR_ERR, "results fail [%s]: id=%u.%u, %s",
		    T_SAM_PATH, ino, gen, mysql_error(SAMDB_conn));
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
		/* overflow */
		if (rcc == rc) {
			Trace(TR_ERR, "excess rows, expected: %llu", rc);
			break;
		}

		v = (*t) + rcc;
		rowlens = mysql_fetch_lengths(res);
		v->ino = strtoul(row[0], (char **)NULL, 10);
		v->gen = strtoul(row[1], (char **)NULL, 10);
		v->type = strtol(row[2], (char **)NULL, 10);

		/*
		 * The following SamStrdup could be replaced by a "strndup"
		 * call that specifies the length of the string to avoid an
		 * extra strlen call.  For sake of debugging memory leaks
		 * we are foregoing this optimization for now.
		 */
		if (row[3] != NULL) {
			SamStrdup(v->path, row[3]);
		}

		if (row[4] != NULL) {
			SamStrdup(v->obj, row[4]);
		}

		if (row[5] != NULL) {
			SamStrdup(v->initial_path, row[5]);
		}

		if (row[6] != NULL) {
			SamStrdup(v->initial_obj, row[6]);
		}

		v->deleted = strtoul(row[7], (char **)NULL, 10);
		v->delete_time = (time_t)strtoul(row[8], (char **)NULL, 10);
	}

	if (rcc != rc) {
		Trace(TR_ERR, "missing rows, expected: %llu, got: %llu",
		    rc, rcc);
	}

out:
	mysql_free_result(res);
	return (rcc);
}

/*
 * --	sam_db_query_link - mySQL query for sam link table.
 *
 *	Description:
 *	    Return the contents of the link table for the specified inode.
 *	    The generation number can be optionally specified.
 *
 *	On Entry:
 *	    ino		Inode number to query.
 *	    gen		Generation number to optionally query.  If 0, all
 *			inodes matching ino are printed.
 *
 *	On Return:
 *	    t		Pointer to table of found records.
 *
 *	Returns:
 *	    Number of records found.  -1 if error.
 */
my_ulonglong sam_db_query_link(
	sam_db_link_t **t,		/* Table buffer	*/
	unsigned long ino,	/* Inode number for query */
	unsigned long gen)	/* Generation number for query */
{
	MYSQL_RES *res;		/* mySQL fetch results */
	MYSQL_ROW row; 		/* mySQL row result */
	unsigned long *rowlens; /* mySQL row result strlen()'s */
	my_ulonglong rc; 		/* Record count from query */
	my_ulonglong rcc;		/* Record count from query */
	char *q;
	sam_db_link_t *v;

	q = strmov(SAMDB_qbuf, "SELECT ino, gen, link FROM ");
	q = strmov(q, T_SAM_LINK);
	sprintf(q, " WHERE ino=%u", ino);
	q = strend(q);
	if (gen != 0) {
		sprintf(q, " AND gen=%u", gen);
		q = strend(q);
	}
	q = strmov(q, ";");
	*q = '\0';

	if (SAMDB_Debug) {
		Trace(TR_DEBUG, "QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		Trace(TR_ERR, "select fail [%s]: id=%u.%u, %s",
		    T_SAM_LINK, ino, gen, mysql_error(SAMDB_conn));
		sam_db_query_error();
		return ((my_ulonglong)-1);
	}

	if ((res = mysql_store_result(SAMDB_conn)) == NULL) {
		Trace(TR_ERR, "results fail [%s]: id=%u.%u, %s",
		    T_SAM_LINK, ino, gen, mysql_error(SAMDB_conn));
		return ((my_ulonglong)-1);
	}

	rc = mysql_num_rows(res);

	if (rc == 0) {
		rcc = 0;
		goto out;
	}

	SamMalloc(*t, (int)rc * sizeof (sam_db_link_t));
	memset((char *)*t, 0, (int)rc * sizeof (sam_db_link_t));

	for (rcc = 0; (row = mysql_fetch_row(res)) != NULL; rcc++) {
		if (rcc == rc) { /* overflow	*/
			Trace(TR_ERR, "excess rows, expected: %llu", rc);
			break;
		}

		v = (*t) + rcc;
		rowlens = mysql_fetch_lengths(res);
		v->ino = strtoul(row[0], (char **)NULL, 10);
		v->gen = strtoul(row[1], (char **)NULL, 10);

		if (row[3] != NULL) {
			SamStrdup(v->link, row[3]);
		}
	}

	if (rcc != rc) {
		Trace(TR_ERR, "missing rows, expected: %llu, got: %llu",
		    rc, rcc);
	}

out:
	mysql_free_result(res);
	return (rcc);
}

/*
 * --	sam_db_query_inode - mySQL query for sam inode table.
 *
 *	Description:
 *	    Return the contents of the inode table for the specified inode.
 *	    The generation number can be optionally specified.
 *
 *	On Entry:
 *	    ino		Inode number to query.
 *	    gen		Generation number to optionally query.  If 0, all
 *			inodes matching ino are printed.
 *
 *	On Return:
 *	    t		Pointer to table of found records.
 *
 *	Returns:
 *	    Number of records found.  -1 if error.
 */
my_ulonglong
sam_db_query_inode(
	sam_db_inode_t **t,	/* Table buffer */
	unsigned long ino,	/* Inode number for query */
	unsigned long gen)	/* Generation number for query */
{
	MYSQL_RES *res;		/* mySQL fetch results */
	MYSQL_ROW row;		/* mySQL row result */
	my_ulonglong rc;	/* Record count from query */
	my_ulonglong rcc;	/* Record count from query */
	char *q;
	sam_db_inode_t *v;

	q = strmov(SAMDB_qbuf, "SELECT ino, gen, type, size, create_time,"
	    " modify_time, deleted, delete_time, uid, gid FROM ");
	q = strmov(q, T_SAM_INODE);
	sprintf(q, " WHERE ino=%u", ino);
	q = strend(q);
	if (gen != 0) {
		sprintf(q, " AND gen=%u", gen);
		q = strend(q);
	}
	q = strmov(q, ";");
	*q = '\0';

	if (SAMDB_Debug) {
		Trace(TR_DEBUG, "QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		Trace(TR_ERR, "select fail [%s]: id=%u.%u, %s",
		    T_SAM_INODE, ino, gen, mysql_error(SAMDB_conn));
		sam_db_query_error();
		return ((my_ulonglong)-1);
	}

	if ((res = mysql_store_result(SAMDB_conn)) == NULL) {
		Trace(TR_ERR, "results fail [%s]: id=%u.%u, %s",
		    T_SAM_INODE, ino, gen, mysql_error(SAMDB_conn));
		return ((my_ulonglong)-1);
	}

	rc = mysql_num_rows(res);

	if (rc == 0) {
		rcc = 0;
		goto out;
	}

	SamMalloc(*t, (int)rc * sizeof (sam_db_inode_t));
	memset((char *)*t, 0, (int)rc * sizeof (sam_db_inode_t));

	for (rcc = 0; (row = mysql_fetch_row(res)) != NULL; rcc++) {
		/* overflow */
		if (rcc == rc) {
			Trace(TR_ERR, "excess rows, expected: %llu", rc);
			break;
		}

		v = (*t) + rcc;
		v->ino = strtoul(row[0], (char **)NULL, 10);
		v->gen = strtoul(row[1], (char **)NULL, 10);
		v->type = strtol(row[2], (char **)NULL, 10);
		v->size = strtoull(row[3], (char **)NULL, 10);
		v->create_time = (time_t)strtoul(row[4], (char **)NULL, 10);
		v->modify_time = (time_t)strtoul(row[5], (char **)NULL, 10);
		v->deleted = strtoul(row[6], (char **)NULL, 10);
		v->delete_time = (time_t)strtoul(row[7], (char **)NULL, 10);
		v->uid = (uid_t)strtoul(row[8], (char **)NULL, 10);
		v->gid = (gid_t)strtoul(row[9], (char **)NULL, 10);
	}

	if (rcc != rc) {
		Trace(TR_ERR, "missing rows, expected: %llu, got: %llu",
		    rc, rcc);
	}

out:
	mysql_free_result(res);
	return (rcc);
}

/*
 * --	sam_db_query_archive - mySQL query for sam archive table.
 *
 *	Description:
 *	    Return the contents of the archive table for the specified inode.
 *	    The generation number can be optionally specified.
 *
 *	On Entry:
 *	    ino		Inode number to query.
 *	    gen		Generation number to optionally query.  If 0, all
 *			inodes matching ino are printed.
 *
 *	On Return:
 *	    t		Pointer to table of found records.
 *
 *	Returns:
 *	    Number of records found.  -1 if error.
 */
my_ulonglong
sam_db_query_archive(
	sam_db_archive_t **t,	/* Table buffer */
	unsigned long ino,	/* Inode number for query */
	unsigned long gen) 	/* Generation number for query */
{
	MYSQL_RES *res;		/* mySQL fetch results */
	MYSQL_ROW row; 		/* mySQL row result */
	my_ulonglong rc; 	/* Record count from query */
	my_ulonglong rcc;	/* Record count from query */
	char *q;
	sam_db_archive_t *v;

	q = strmov(SAMDB_qbuf, "SELECT ino, gen, copy, seq, vsn_id, size,"
	    " create_time, modify_time, stale, recycled,"
	    " recycle_time FROM ");
	q = strmov(q, T_SAM_ARCHIVE);
	sprintf(q, " WHERE ino=%u", ino);
	q = strend(q);
	if (gen != 0) {
		sprintf(q, " AND gen=%u", gen);
		q = strend(q);
	}
	q = strmov(q, ";");
	*q = '\0';

	if (SAMDB_Debug) {
		Trace(TR_DEBUG, "QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int) (q-SAMDB_qbuf))) {
		Trace(TR_ERR, "select fail [%s]: id=%u.%u, %s",
		    T_SAM_ARCHIVE, ino, gen, mysql_error(SAMDB_conn));
		sam_db_query_error();
		return ((my_ulonglong)-1);
	}

	if ((res = mysql_store_result(SAMDB_conn)) == NULL) {
		Trace(TR_ERR, "results fail [%s]: id=%u.%u, %s",
		    T_SAM_ARCHIVE, ino, gen, mysql_error(SAMDB_conn));
		return ((my_ulonglong)-1);
	}

	rc = mysql_num_rows(res);

	if (rc == 0) {
		rcc = 0;
		goto out;
	}

	SamMalloc(*t, (int)rc * sizeof (sam_db_archive_t));
	memset((char *)*t, 0, (int)rc * sizeof (sam_db_archive_t));

	for (rcc = 0; (row = mysql_fetch_row(res)) != NULL; rcc++) {
		/* overflow */
		if (rcc == rc) {
			Trace(TR_ERR, "excess rows, expected: %llu", rc);
			break;
		}

		v = (*t) + rcc;
		v->ino = strtoul(row[0], (char **)NULL, 10);
		v->gen = strtoul(row[1], (char **)NULL, 10);
		v->copy = (unsigned short)strtol(row[2], (char **)NULL, 10);
		v->seq = (unsigned short)strtol(row[3], (char **)NULL, 10);
		v->vsn_id = strtol(row[4], (char **)NULL, 10);
		v->size = (off64_t)strtoull(row[5], (char **)NULL, 10);
		v->create_time = (time_t)strtoul(row[6], (char **)NULL, 10);
		v->modify_time = (time_t)strtoul(row[7], (char **)NULL, 10);
		v->stale = strtoul(row[8], (char **)NULL, 10);
		v->recycled = strtoul(row[9], (char **)NULL, 10);
		v->recycle_time = (time_t)strtoul(row[10], (char **)NULL, 10);
	}

	if (rcc != rc) {
		Trace(TR_ERR, "missing rows, expected: %llu, got: %llu",
		    rc, rcc);
	}

out:
	mysql_free_result(res);
	return (rcc);
}

/*
 * --	sam_db_row_count - mySQL query for row_count.
 *
 *	Description:
 *	    Return the contents of the ROW_COUNT() function.
 *
 *	Returns:
 *	    Row count.  -1 if error.
 */
my_ulonglong
sam_db_row_count(void)
{
	MYSQL_RES *res;		/* mySQL fetch results */
	MYSQL_ROW row; 		/* mySQL row result */
	my_ulonglong rc;	/* Record count from query */
	my_ulonglong count; /* Record count from query */
	char *q;

	q = strmov(SAMDB_qbuf, "SELECT ROW_COUNT();");
	*q = '\0';

	if (SAMDB_Debug) {
		Trace(TR_DEBUG, "QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		Trace(TR_ERR, "select fail [ROW_COUNT]: %s",
		    mysql_error(SAMDB_conn));
		sam_db_query_error();
		return ((my_ulonglong)-1);
	}

	if ((res = mysql_store_result(SAMDB_conn)) == NULL) {
		Trace(TR_ERR, "results fail [ROW_COUNT]: %s",
		    mysql_error(SAMDB_conn));
		return ((my_ulonglong)-1);
	}

	rc = mysql_num_rows(res);

	if (rc == 0) {
		count = 0;
		goto out;
	}

	if (rc != 1) {
		Trace(TR_ERR, "ROW_COUNT: excess rows, expected: 1, got: %llu",
		    rc);
	}

	row = mysql_fetch_row(res);
	count = strtoull(row[0], (char **)NULL, 10);

out:
	mysql_free_result(res);
	return (count);
}
