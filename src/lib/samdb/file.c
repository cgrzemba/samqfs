/*
 * --	file.c - mySQL file functions for SAM-db.
 *
 *	Descripton:
 *	    file.c is a collecton of routines which manage
 *	    the file table of samdb
 *
 *	Contents:
 *	    sam_db_file_new
 *	    sam_db_file_new_byname
 *	    sam_db_file_insert
 *	    sam_db_file_select
 *	    sam_db_file_count_byhash
 *	    sam_db_file_update
 *	    sam_db_file_update_byhash
 *	    sam_db_file_delete
 *	    sam_db_file_delete_byhash
 *	    sam_db_file_delete_bypid
 *	    sam_db_file_delete_byid
 *	    sam_db_file_delete_bypidid
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.2 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <pub/stat.h>
#include <sam/sam_trace.h>
#include <sam/sam_db.h>

#define	FILE_INSERT	3000
#define	FILE_SELECT	3100
#define	FILE_CNTHASH	3101
#define	FILE_UPDATE	3200
#define	FILE_UPDHASH	3201
#define	FILE_DELETE	3300
#define	FILE_DELHASH	3301
#define	FILE_DELPID	3302
#define	FILE_DELID	3303
#define	FILE_DELPIDID	3304

static void bind_file(MYSQL_BIND *bind, sam_db_file_t *file,
    boolean_t is_result);

/*
 * sam_db_file_new - Populates the given sam_db_file_t
 */
int
sam_db_file_new(
	sam_db_context_t *con,
	sam_id_t id,
	sam_id_t parent_id,
	int namehash,
	sam_db_file_t *file)
{
	if (file == NULL) {
		Trace(TR_ERR, "sam_db_file_new null file argument");
		return (-1);
	}

	memset(file, 0, sizeof (sam_db_file_t));
	file->p_ino = parent_id.ino;
	file->p_gen = parent_id.gen;
	file->ino = id.ino;
	file->gen = id.gen;
	file->name_hash = namehash;

	if (sam_db_id_name(con, parent_id, id, namehash, file->name) <= 0) {
		Trace(TR_ERR, "error getting name for %d.%d",
		    file->ino, file->gen);
		return (-1);
	}

	return (0);
}

/*
 * sam_db_file_new_byname - Populates the given sam_db_file_t
 */
int
sam_db_file_new_byname(
	sam_id_t id,
	sam_id_t parent_id,
	int namehash,
	char *name,
	sam_db_file_t *file)
{
	if (file == NULL) {
		Trace(TR_ERR, "sam_db_file_new null file argument");
		return (-1);
	}

	file->p_ino = parent_id.ino;
	file->p_gen = parent_id.gen;
	file->name_hash = namehash;
	file->ino = id.ino;
	file->gen = id.gen;
	(void) strlcpy(file->name, name, MAXNAMELEN+1);

	return (0);
}

/*
 * Inserts given file information into database.
 *
 * Return: 0 on success, -1 on error
 */
int
sam_db_file_insert(
	sam_db_context_t *con,
	sam_db_file_t *file)
{
	MYSQL_BIND bind[6];

	memset(bind, 0, sizeof (bind));
	bind_file(bind, file, FALSE);

	if (sam_db_execute_sql(con, bind, NULL, FILE_INSERT) == NULL) {
		Trace(TR_ERR, "Error inserting file %d.%d",
		    file->ino, file->gen);
		return (-1);
	}

	return (0);
}

/*
 * Selects file from database with given parent id and name.
 *
 * Return: 0 on success, -1 on error
 * Return: file information copied into result
 */
int
sam_db_file_select(
	sam_db_context_t *con,
	sam_id_t parent_id,
	char *name,
	sam_db_file_t *result)
{
	int err;
	MYSQL_STMT *stmt;
	MYSQL_BIND bind[3];
	MYSQL_BIND bind_result[6];

	memset(bind, 0, sizeof (bind));
	memset(bind_result, 0, sizeof (bind_result));

	SAMDB_BIND(bind[0], parent_id.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], parent_id.gen, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND_STR(bind[2], name);

	bind_file(bind_result, result, TRUE);

	if ((stmt = sam_db_execute_sql(con, bind,
	    bind_result, FILE_SELECT)) == NULL) {
		Trace(TR_ERR, "Error selecting file %d.%d %s",
		    parent_id.ino, parent_id.gen, name);
		return (-1);
	}

	if ((err = mysql_stmt_fetch(stmt)) != 0) {
		if (err == MYSQL_NO_DATA) {
			return (MYSQL_NO_DATA);
		}
		Trace(TR_ERR, "Error fetching file %d.%d %s: %s",
		    parent_id.ino, parent_id.gen, name,
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
 * Counts files with given parent id, id and name hash.
 *
 * Return: count on success, -1 on error
 */
int
sam_db_file_count_byhash(
	sam_db_context_t *con,
	sam_id_t parent_id,
	sam_id_t id,
	int namehash)
{
	int count;
	MYSQL_STMT *stmt;
	MYSQL_BIND bind[5];
	MYSQL_BIND bind_result[1];

	memset(bind, 0, sizeof (bind));
	memset(bind_result, 0, sizeof (bind_result));

	SAMDB_BIND(bind[0], parent_id.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], parent_id.gen, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[2], namehash, MYSQL_TYPE_LONG, FALSE);
	SAMDB_BIND(bind[3], id.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[4], id.gen, MYSQL_TYPE_LONG, TRUE);

	SAMDB_BIND(bind_result[0], count, MYSQL_TYPE_LONG, FALSE);

	if ((stmt = sam_db_execute_sql(con, bind,
	    bind_result, FILE_CNTHASH)) == NULL) {
		Trace(TR_ERR, "Error selecting count %d.%d %d.%d %d",
		    parent_id.ino, parent_id.gen, id.ino, id.gen, namehash);
		return (-1);
	}

	if (mysql_stmt_fetch(stmt) != 0) {
		Trace(TR_ERR, "Error fetching count %d.%d %d.%d %d: %s",
		    parent_id.ino, parent_id.gen, id.ino, id.gen, namehash,
		    mysql_error(con->mysql));
		return (-1);
	}

	if (mysql_stmt_free_result(stmt) != 0) {
		Trace(TR_ERR, "Error freeing result: %s",
		    mysql_error(con->mysql));
	}

	return (count);
}

/*
 * Updates file in database with information from given sam_db_file.
 * Return: number affected rows on success, -1 on error
 */
int
sam_db_file_update(
	sam_db_context_t *con,
	sam_id_t old_pid,
	char *oldname,
	sam_db_file_t *file)
{
	MYSQL_BIND bind[7];
	memset(bind, 0, sizeof (bind));

	SAMDB_BIND(bind[0], file->p_ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], file->p_gen, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND_STR(bind[2], file->name);
	SAMDB_BIND(bind[3], file->name_hash, MYSQL_TYPE_SHORT, TRUE);
	SAMDB_BIND(bind[4], old_pid.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[5], old_pid.gen, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND_STR(bind[6], oldname);

	if (sam_db_execute_sql(con, bind, NULL, FILE_UPDATE) == NULL) {
		Trace(TR_ERR, "Error updating file %d.%d",
		    file->ino, file->gen);
		return (-1);
	}

	return (mysql_affected_rows(con->mysql));
}

/*
 * Updates file in database with information from given sam_db_file.
 * Uses the hash value of the name to identity the row to update. To
 * use this function without collisions sam_db_file_countbyhash
 * should equal 1.
 *
 * Return: number affected rows on success, -1 on error
 */
int
sam_db_file_update_byhash(
	sam_db_context_t *con,
	sam_id_t old_pid,
	int namehash,
	sam_db_file_t *file)
{
	MYSQL_BIND bind[9];
	memset(bind, 0, sizeof (bind));

	SAMDB_BIND(bind[0], file->p_ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], file->p_gen, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND_STR(bind[2], file->name);
	SAMDB_BIND(bind[3], file->name_hash, MYSQL_TYPE_SHORT, TRUE);
	SAMDB_BIND(bind[4], old_pid.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[5], old_pid.gen, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[6], namehash, MYSQL_TYPE_LONG, FALSE);
	SAMDB_BIND(bind[7], file->ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[8], file->gen, MYSQL_TYPE_LONG, TRUE);

	if (sam_db_execute_sql(con, bind, NULL, FILE_UPDHASH) == NULL) {
		Trace(TR_ERR, "Error updating file %d.%d %d.%d %d: %s",
		    old_pid.ino, old_pid.gen, file->ino, file->gen, namehash,
		    mysql_error(con->mysql));
		return (-1);
	}

	return (mysql_affected_rows(con->mysql));
}

/*
 * Deletes file from database with given parent, id and name.
 *
 * Return number affected rows on success, -1 on error
 */
int
sam_db_file_delete(sam_db_context_t *con, sam_id_t parent_id, char *name)
{
	MYSQL_BIND bind[3];
	memset(bind, 0, sizeof (bind));

	SAMDB_BIND(bind[0], parent_id.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], parent_id.gen, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND_STR(bind[2], name);

	if (sam_db_execute_sql(con, bind, NULL, FILE_DELETE) == NULL) {
		Trace(TR_ERR, "Error deleting file: %s",
		    mysql_error(con->mysql));
		return (-1);
	}

	return (mysql_affected_rows(con->mysql));
}

/*
 * Deletes file from database with given parent and file id, with name hash.
 *
 * Return number affected rows on success, -1 on error
 */
int
sam_db_file_delete_byhash(
	sam_db_context_t *con,
	sam_id_t pid,
	sam_id_t id,
	int namehash)
{
	MYSQL_BIND bind[5];
	memset(bind, 0, sizeof (bind));

	SAMDB_BIND(bind[0], pid.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], pid.gen, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[2], namehash, MYSQL_TYPE_LONG, FALSE);
	SAMDB_BIND(bind[3], id.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[4], id.gen, MYSQL_TYPE_LONG, TRUE);

	if (sam_db_execute_sql(con, bind, NULL, FILE_DELHASH) == NULL) {
		Trace(TR_ERR, "Error deleting file: %s",
		    mysql_error(con->mysql));
		return (-1);
	}

	return (mysql_affected_rows(con->mysql));
}

/*
 * Deletes files from database with given parent.
 *
 * Return number affected rows on success, -1 on error
 */
int
sam_db_file_delete_bypid(
	sam_db_context_t *con,
	sam_id_t pid)
{
	MYSQL_BIND bind[2];
	memset(bind, 0, sizeof (bind));

	SAMDB_BIND(bind[0], pid.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], pid.gen, MYSQL_TYPE_LONG, TRUE);

	if (sam_db_execute_sql(con, bind, NULL, FILE_DELPID) == NULL) {
		Trace(TR_ERR, "Error deleting files: %s",
		    mysql_error(con->mysql));
		return (-1);
	}

	return (mysql_affected_rows(con->mysql));
}

/*
 * Deletes file from database with given file id.
 *
 * Return number affected rows on success, -1 on error
 */
int
sam_db_file_delete_byid(
	sam_db_context_t *con,
	sam_id_t id)
{
	MYSQL_BIND bind[2];
	memset(bind, 0, sizeof (bind));

	SAMDB_BIND(bind[0], id.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], id.gen, MYSQL_TYPE_LONG, TRUE);

	if (sam_db_execute_sql(con, bind, NULL, FILE_DELID) == NULL) {
		Trace(TR_ERR, "Error deleting files: %s",
		    mysql_error(con->mysql));
		return (-1);
	}

	return (mysql_affected_rows(con->mysql));
}

/*
 * Deletes file from database with given parent and file id.
 *
 * Return number affected rows on success, -1 on error
 */
int
sam_db_file_delete_bypidid(
	sam_db_context_t *con,
	sam_id_t pid,
	sam_id_t id)
{
	MYSQL_BIND bind[4];
	memset(bind, 0, sizeof (bind));

	SAMDB_BIND(bind[0], pid.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], pid.gen, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[2], id.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[3], id.gen, MYSQL_TYPE_LONG, TRUE);

	if (sam_db_execute_sql(con, bind, NULL, FILE_DELPIDID) == NULL) {
		Trace(TR_ERR, "Error deleting files: %s",
		    mysql_error(con->mysql));
		return (-1);
	}

	return (mysql_affected_rows(con->mysql));
}

/*
 * bind_file - Initializes the bind variable array with file values
 */
static void
bind_file(MYSQL_BIND *bind, sam_db_file_t *file, boolean_t is_result)
{
	SAMDB_BIND(bind[0], file->p_ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], file->p_gen, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[2], file->name_hash, MYSQL_TYPE_SHORT, TRUE);
	SAMDB_BIND_STR(bind[3], file->name);
	if (is_result) {
		bind[3].buffer_length = sizeof (file->name);
	}
	SAMDB_BIND(bind[4], file->ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[5], file->gen, MYSQL_TYPE_LONG, TRUE);
}
