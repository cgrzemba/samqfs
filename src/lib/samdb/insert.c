/*
 * --	insert.c - mySQL insert functions for SAM-db.
 *
 *	Descripton:
 *	    insert.c is a collecton of routines which issue
 *	    INSERT statements to the corresponding SAM tables.
 *
 *	Contents:
 *	    sam_db_insert_vsn	  - insert SAM vsns table.
 *	    sam_db_insert_path	  - insert SAM path table.
 *	    sam_db_insert_inode	  - insert SAM inode table.
 *	    sam_db_insert_link	  - insert SAM inode table.
 *	    sam_db_insert_archive - insert SAM inode table.
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

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sam/sam_trace.h>
#include <sam/sam_db.h>

/*
 * --	sam_db_insert_vsn - Insert VSN into table.
 *
 *	Description:
 *	    Add given VSN/media type to sam_vsns table.
 *
 *	On Entry:
 *	    media	Media type.
 *	    vsn		VSN.
 *
 *	Returns:
 *	    vsn_id for given entry.  If < 0, then error.
 */
int	sam_db_insert_vsn(
	char *media,	/* Media type */
	char *vsn)		/* VSN */
{
	my_ulonglong rc;	/* Record count from query	*/
	int vsn_id;			/* VSN id ordinals		*/
	char *q;

	q = strmov(SAMDB_qbuf, "INSERT INTO ");
	q = strmov(q, T_SAM_VSNS);
	q = strmov(q, " (media_type, vsn)");
	sprintf(q, " VALUES ( '%s', '%s')", media, vsn);
	q = strend(q);
	q = strmov(q, ";");
	*q = '\0';

	if (SAMDB_Debug) {
		Trace(TR_DEBUG, "QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int) (q-SAMDB_qbuf))) {
		Trace(TR_ERR, "insert fail [%s]: %s:%s",
		    T_SAM_VSNS, media, vsn);
		sam_db_query_error();
		return (-1);
	}

	rc = mysql_affected_rows(SAMDB_conn);

	if (rc != 1) {
		Trace(TR_ERR, "insert rows [%s]: %s:%s, rows=%llu",
		    T_SAM_VSNS, media, vsn, rc);
		return (-1);
	}

	vsn_id = mysql_insert_id(SAMDB_conn);
	return (vsn_id);
}


/*
 * --	sam_db_insert_inode - Insert inode entry into DB.
 *
 *	Description:
 *	    Insert inode entry into sam_inode.
 *
 *	On Entry:
 *	    t	Inode DB structure.
 *
 *	Returns:
 *	    0 if no errors, -1 if error.
 */
int	sam_db_insert_inode(
	sam_db_inode_t	*t
)
{
	my_ulonglong rc;		/* Record count from query */
	char *q;

	q = strmov(SAMDB_qbuf, "INSERT INTO ");
	q = strmov(q, T_SAM_INODE);
	q = strmov(q, " (ino, gen, type, size, create_time,"
	    " modify_time, uid, gid) ");
	sprintf(q, " VALUES ('%u', '%u', '%d', '%llu',"
	    " '%lu', '%lu', '%u', '%u')",
	    t->ino, t->gen, t->type, t->size,
	    t->create_time, t->modify_time, t->uid, t->gid);
	q = strend(q);
	q = strmov(q, ";");
	*q = '\0';

	if (SAMDB_Debug) {
		Trace(TR_DEBUG, "QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		Trace(TR_ERR, "insert fail [%s]: id=%u.%u, %s",
		    T_SAM_INODE, t->ino, t->gen, mysql_error(SAMDB_conn));
		sam_db_query_error();
		return (-1);
	}

	rc = mysql_affected_rows(SAMDB_conn);

	if (rc != 1) {
		Trace(TR_ERR, "insert rows [%s]: id=%u.%u, rows=%llu",
		    T_SAM_INODE, t->ino, t->gen, rc);
		return (-1);
	}

	return (0);
}


/*
 * --	sam_db_insert_path - Insert path entry into DB.
 *
 *	Description:
 *	    Insert path entry into sam_path.
 *
 *	On Entry:
 *	    t	Path DB structure.
 *
 *	Returns:
 *	    0 if no errors, -1 if error.
 */
int	sam_db_insert_path(
	sam_db_path_t	*t
)
{
	my_ulonglong rc;	/* Record count from query	*/
	char *q;

	q = strmov(SAMDB_qbuf, "INSERT INTO ");
	q = strmov(q, T_SAM_PATH);
	q = strmov(q, " (ino, gen, type, path, obj,"
	    " initial_path, initial_obj) ");
	sprintf(q, " VALUES ('%u', '%u', '%d', '",
	    t->ino, t->gen, t->type);
	q  = strend(q);
	q += mysql_real_escape_string(SAMDB_conn, q, t->path, strlen(t->path));
	q  = strmov(q, "', '");
	q += mysql_real_escape_string(SAMDB_conn, q, t->obj, strlen(t->obj));
	q  = strmov(q, "', '");
	q += mysql_real_escape_string(SAMDB_conn, q, t->path, strlen(t->path));
	q  = strmov(q, "', '");
	q += mysql_real_escape_string(SAMDB_conn, q, t->obj, strlen(t->obj));
	q  = strmov(q, "');");
	*q  = '\0';

	if (SAMDB_Debug) {
		Trace(TR_DEBUG, "QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		Trace(TR_ERR, "insert fail [%s]: id=%u.%u, %s",
		    T_SAM_PATH, t->ino, t->gen, mysql_error(SAMDB_conn));
		sam_db_query_error();
		return (-1);
	}

	rc = mysql_affected_rows(SAMDB_conn);

	if (rc != 1) {
		Trace(TR_ERR, "insert rows [%s]: id=%u.%u, rows=%llu",
		    T_SAM_PATH, t->ino, t->gen, rc);
		return (-1);
	}

	return (0);
}


/*
 * --	sam_db_insert_link - Insert link entry into DB.
 *
 *	Description:
 *	    Insert link entry into sam_link.
 *
 *	On Entry:
 *	    t	Link DB structure.
 *
 *	Returns:
 *	    0 if no errors, -1 if error.
 */
int	sam_db_insert_link(
	sam_db_link_t *t
)
{
	my_ulonglong rc;		/* Record count from query	*/
	char *q;

	q = strmov(SAMDB_qbuf, "INSERT INTO ");
	q = strmov(q, T_SAM_LINK);
	q = strmov(q, " (ino, gen, link) ");
	sprintf(q, " VALUES ('%u', '%u', '", t->ino, t->gen);
	q = strend(q);
	q += mysql_real_escape_string(SAMDB_conn, q, t->link, strlen(t->link));
	q = strmov(q, "');");
	*q = '\0';

	if (SAMDB_Debug) {
		Trace(TR_DEBUG, "QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		Trace(TR_ERR, "insert fail [%s]: id=%u.%u, %s",
		    T_SAM_LINK, t->ino, t->gen, mysql_error(SAMDB_conn));
		sam_db_query_error();
		return (-1);
	}

	rc = mysql_affected_rows(SAMDB_conn);

	if (rc != 1) {
		Trace(TR_ERR, "insert rows [%s]: id=%u.%u, rows=%llu",
		    T_SAM_LINK, t->ino, t->gen, rc);
		return (-1);
	}

	return (0);
}


/*
 * --	sam_db_insert_archive - Insert archive entry into DB.
 *
 *	Description:
 *	    Insert archive entry into sam_archive.
 *
 *	On Entry:
 *	    t		Link DB structure.
 *	    modify_time	Modify time from inode.  Used to determine
 *			stale status.
 *
 *	Returns:
 *	    0 if no errors, -1 if error.
 */
int	sam_db_insert_archive(
	sam_db_archive_t	*t,
	time_t		modify_time
)
{
	my_ulonglong rc;	/* Record count from query */
	char *q;

	t->stale = t->modify_time != modify_time;

	q = strmov(SAMDB_qbuf, "INSERT INTO ");
	q = strmov(q, T_SAM_ARCHIVE);
	q = strmov(q, " (ino, gen, copy, seq, vsn_id, size,"
	    " create_time, modify_time, stale) ");
	sprintf(q, " VALUES ('%u', '%u', '%d', '%d', '%u', '%llu',"
	    " '%lu', '%lu', '%d')",	t->ino, t->gen, t->copy, t->seq,
	    t->vsn_id, t->size,	t->create_time, t->modify_time, t->stale);
	q = strend(q);
	q = strmov(q, ";");
	*q = '\0';

	if (SAMDB_Debug) {
		Trace(TR_DEBUG, "QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int) (q-SAMDB_qbuf))) {
		Trace(TR_ERR, "insert fail [%s]: id=%u, %s",
		    T_SAM_ARCHIVE, t->vsn_id, mysql_error(SAMDB_conn));
		sam_db_query_error();
		return (-1);
	}

	rc = mysql_affected_rows(SAMDB_conn);

	if (rc != 1) {
		Trace(TR_ERR, "insert rows [%s]: id=%u, rows=%llu",
		    T_SAM_ARCHIVE, t->vsn_id, rc);
		return (-1);
	}

	return (0);
}
