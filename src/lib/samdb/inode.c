/*
 * --	inode.c - mySQL inode functions for SAM-db.
 *
 *	Descripton:
 *	    inode.c is a collecton of routines which manage
 *	    the inode table of samdb
 *
 *	Contents:
 *	    sam_db_inode_new
 *	    sam_db_inode_ftype
 *	    sam_db_inode_insert
 *	    sam_db_inode_select
 *	    sam_db_inode_update
 *	    sam_db_inode_delete
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

#include <pub/stat.h>
#include <sam/sam_trace.h>
#include <sam/sam_db.h>

#define	INO_INSERT	1000
#define	INO_SELECT	1100
#define	INO_UPDATE	1200
#define	INO_DELETE	1300

static void bind_inode(MYSQL_BIND *bind, sam_db_inode_t *inode,
    boolean_t is_result);

/*
 * sam_db_inode_new - Populates the given sam_db_inode_t
 *
 */
int
sam_db_inode_new(
	sam_db_context_t *con,
	sam_id_t id,
	sam_db_inode_t *inode)
{
	struct sam_perm_inode perm;
	if (inode == NULL) {
		Trace(TR_ERR, "Null sam_db_inode_t argument");
		return (-1);
	}

	if (sam_db_id_stat(con, id.ino, id.gen, &perm) < 0) {
		return (-1);
	}

	return (sam_db_inode_new_perm(&perm, inode));
}

/*
 * sam_db_inode_new_perm - Populates the given sam_db_inode_t
 */
int
sam_db_inode_new_perm(sam_perm_inode_t *ip, sam_db_inode_t *inode) {
	if (inode == NULL || ip == NULL) {
		Trace(TR_ERR, "Null argument");
		return (-1);
	}

	memset(inode, 0, sizeof (sam_db_inode_t));
	inode->ino = ip->di.id.ino;
	inode->gen = ip->di.id.gen;
	inode->type = sam_db_inode_ftype(ip);
	inode->size = ip->di.rm.size;
	if (ip->di.status.b.cs_val) {
		snprintf(inode->csum, sizeof (inode->csum), "%08x%08x%08x%08x",
		    ip->csum.csum_val[0],
		    ip->csum.csum_val[1],
		    ip->csum.csum_val[2],
		    ip->csum.csum_val[3]);
	} else {
		*inode->csum = '\0';
	}
	inode->create_time = ip->di.creation_time;
	inode->modify_time = ip->di.modify_time.tv_sec;
	inode->uid = ip->di.uid;
	inode->gid = ip->di.gid;
	inode->online = ip->di.status.b.offline ? 0 : 1;

	return (0);
}

/*
 * sam_db_inode_ftype - Return SAM DB File Type.
 *
 *	Description:
 *	    Converts inode file type to database file type.
 *
 *	On Entry:
 *	    inode	Inode image.
 *
 *	Returns:
 *	    Database file type (see SAM_db_ftype in sam_db.h for types).
 */
sam_db_ftype_t
sam_db_inode_ftype(struct sam_perm_inode *inode)
{
	sam_db_ftype_t ftype; /* File type */

	ftype = FTYPE_OTHER;
	if (S_ISREG(inode->di.mode)) {
		ftype = FTYPE_REG;
	}
	if (S_ISDIR(inode->di.mode)) {
		ftype = FTYPE_DIR;
	}
	if (S_ISSEGS(&inode->di)) {
		ftype = FTYPE_SEGI;
	}
	if (S_ISSEGI(&inode->di)) {
		ftype = FTYPE_SEG;
	}
	if (S_ISLNK(inode->di.mode)) {
		ftype = FTYPE_LINK;
	}

	return (ftype);
}

/*
 * Inserts given inode information into database.
 *
 * Return: 0 on success, -1 on error
 */
int
sam_db_inode_insert(
	sam_db_context_t *con,
	sam_db_inode_t *inode)
{
	MYSQL_BIND bind[10];

	memset(bind, 0, sizeof (bind));
	bind_inode(bind, inode, FALSE);

	if (sam_db_execute_sql(con, bind, NULL, INO_INSERT) == NULL) {
		Trace(TR_ERR, "Error inserting inode %d.%d",
		    inode->ino, inode->gen);
		return (-1);
	}

	return (0);
}

/*
 * Selects inode from database with given inode and gen number.
 *
 * Return: 0 on success, -1 on error
 * Return: inode information copied into result
 */
int
sam_db_inode_select(
	sam_db_context_t *con,
	sam_id_t id,
	sam_db_inode_t *result)
{
	int err;
	MYSQL_STMT *stmt;
	MYSQL_BIND bind[2];
	MYSQL_BIND bind_result[10];

	memset(bind, 0, sizeof (bind));
	memset(bind_result, 0, sizeof (bind_result));

	SAMDB_BIND(bind[0], id.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], id.gen, MYSQL_TYPE_LONG, TRUE);
	bind_inode(bind_result, result, TRUE);

	if ((stmt = sam_db_execute_sql(con, bind,
	    bind_result, INO_SELECT)) == NULL) {
		Trace(TR_ERR, "Error selecting inode %d.%d", id.ino, id.gen);
		return (-1);
	}

	if ((err = mysql_stmt_fetch(stmt)) != 0) {
		if (err == MYSQL_NO_DATA) {
			return (MYSQL_NO_DATA);
		}
		Trace(TR_ERR, "Error fetching inode %d.%d: %s", id.ino, id.gen,
		    mysql_error(con->mysql));
		return (-1);
	}

	if (mysql_stmt_free_result(stmt) != 0) {
		Trace(TR_ERR, "Error freeing result: %s",
		    mysql_error(con->mysql));
	}

	return (0);
}

/*
 * Updates inode in database with information from given sam_db_inode.
 *
 * Return: number affected rows on success, -1 on error
 */
int
sam_db_inode_update(sam_db_context_t *con, sam_db_inode_t *inode)
{
	MYSQL_BIND bind[8];
	memset(bind, 0, sizeof (bind));

	SAMDB_BIND(bind[0], inode->size, MYSQL_TYPE_LONGLONG, FALSE);
	SAMDB_BIND_STR(bind[1], inode->csum);
	SAMDB_BIND(bind[2], inode->modify_time, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[3], inode->uid, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[4], inode->gid, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[5], inode->online, MYSQL_TYPE_TINY, TRUE);
	SAMDB_BIND(bind[6], inode->ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[7], inode->gen, MYSQL_TYPE_LONG, TRUE);

	if (sam_db_execute_sql(con, bind, NULL, INO_UPDATE) == NULL) {
		Trace(TR_ERR, "Error updating inode %d.%d",
		    inode->ino, inode->gen);
		return (-1);
	}

	return (mysql_affected_rows(con->mysql));
}

/*
 * Deletes inode from database with given inode and gen number.
 *
 * Return number affected rows on success, -1 on error
 */
int
sam_db_inode_delete(sam_db_context_t *con, sam_id_t id)
{
	MYSQL_BIND bind[2];
	memset(bind, 0, sizeof (bind));

	SAMDB_BIND(bind[0], id.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], id.gen, MYSQL_TYPE_LONG, TRUE);

	if (sam_db_execute_sql(con, bind, NULL, INO_DELETE) == NULL) {
		Trace(TR_ERR, "Error deleting inode: %s",
		    mysql_error(con->mysql));
		return (-1);
	}

	return (mysql_affected_rows(con->mysql));
}

/*
 * bind_inode - Initializes the bind variable array with inode values
 */
static void
bind_inode(MYSQL_BIND *bind, sam_db_inode_t *inode, boolean_t is_result)
{
	SAMDB_BIND(bind[0], inode->ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], inode->gen, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[2], inode->type, MYSQL_TYPE_TINY, TRUE);
	SAMDB_BIND(bind[3], inode->size, MYSQL_TYPE_LONGLONG, FALSE);
	SAMDB_BIND_STR(bind[4], inode->csum);
	SAMDB_BIND(bind[5], inode->create_time, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[6], inode->modify_time, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[7], inode->uid, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[8], inode->gid, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[9], inode->online, MYSQL_TYPE_TINY, TRUE);

	if (is_result) {
		bind[4].buffer_length = sizeof (inode->csum);
	}
}
