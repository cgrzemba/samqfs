/*
 * --	path.c - mySQL path functions for SAM-db.
 *
 *	Descripton:
 *	    path.c is a collecton of routines which manage
 *	    the path table of samdb
 *
 *	Contents:
 *	    sam_db_path_new
 *	    sam_db_path_insert
 *	    sam_db_path_select
 *	    sam_db_path_update
 *	    sam_db_path_update_subdir
 *	    sam_db_path_delete
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
#include <sys/param.h>
#include <sys/vfs.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pub/stat.h>
#include <sam/types.h>
#include <sam/lib.h>
#include <sam/uioctl.h>
#include <sam/sam_db.h>
#include <sam/sam_malloc.h>
#include <sam/sam_trace.h>
#include <sam/sam_db.h>
#include <sam/fs/sblk.h>

#define	PATH_INSERT	2000
#define	PATH_SELECT	2100
#define	PATH_UPDATE	2200
#define	PATH_UPDSUBDIR	2201
#define	PATH_DELETE	2300

static void bind_path(MYSQL_BIND *bind, sam_db_path_t *path,
    boolean_t is_result);
static int build_path(sam_db_context_t *con, sam_perm_inode_t *ip,
    int namehash, char *path);

/*
 * sam_db_path_new - Populates provided sam_db_path.  Can only be used
 * for directories and symbolic links.
 */
int
sam_db_path_new(
	sam_db_context_t *con,
	sam_id_t id,
	int namehash,
	sam_db_path_t *path)
{
	sam_perm_inode_t perm;
	sam_db_ftype_t ftype;

	if (path == NULL) {
		Trace(TR_ERR, "sam_db_path_new null path argument");
		return (-1);
	}

	if (sam_db_id_stat(con, id.ino, id.gen, &perm) < 0) {
		Trace(TR_ERR, "Could not stat %d.%d", id.ino, id.gen);
		return (-1);
	}

	ftype = sam_db_inode_ftype(&perm);
	if (!(ftype == FTYPE_DIR || ftype == FTYPE_LINK)) {
		Trace(TR_ERR, "path inode must be a directory or link",
		    id.ino, id.gen);
		return (-1);
	}

	memset(path, 0, sizeof (sam_db_path_t));
	path->ino = id.ino;
	path->gen = id.gen;

	if (build_path(con, &perm, namehash, path->path) < 0) {
		Trace(TR_ERR, "could not build path for %d.%d",
		    id.ino, id.gen);
		return (-1);
	}

	if (ftype == FTYPE_LINK) {
		/* Need to build link path to use readlink(2) */
		strcpy(con->qbuf, con->mount_point);
		strcat(con->qbuf, path->path);
		memset(path->path, 0, MAXPATHLEN);
		if (readlink(con->qbuf, path->path, MAXPATHLEN) < 0) {
			Trace(TR_ERR, "Could not read link %s", con->qbuf);
			return (-1);
		}
	}

	return (0);
}

/*
 * sam_db_path_insert - Inserts the provided path into the database.
 *   Returns: 0 if success, -1 if error
 */
int
sam_db_path_insert(
	sam_db_context_t *con,
	sam_db_path_t *path)
{
	MYSQL_BIND bind[3];

	memset(bind, 0, sizeof (bind));
	bind_path(bind, path, FALSE);

	if (sam_db_execute_sql(con, bind, NULL, PATH_INSERT) == NULL) {
		Trace(TR_ERR, "Error inserting path %d.%d",
		    path->ino, path->gen);
		return (-1);
	}

	return (0);
}

/*
 * Select the path from the database with the provided inode and gen number.
 *
 * Returns: 0 on success, -1 on error, MYSQL_NO_DATA for not found
 */
int
sam_db_path_select(
	sam_db_context_t *con, sam_id_t id, sam_db_path_t *result)
{
	int err;
	MYSQL_STMT *stmt;
	MYSQL_BIND bind[2];
	MYSQL_BIND bind_result[3];

	memset(bind, 0, sizeof (bind));
	memset(bind_result, 0, sizeof (bind_result));

	SAMDB_BIND(bind[0], id.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], id.gen, MYSQL_TYPE_LONG, TRUE);
	bind_path(bind_result, result, TRUE);

	if ((stmt = sam_db_execute_sql(con, bind, bind_result,
	    PATH_SELECT)) == NULL) {
		Trace(TR_ERR, "Error selecting path %d.%d", id.ino, id.gen);
		return (-1);
	}

	if ((err = mysql_stmt_fetch(stmt)) != 0) {
		if (err == MYSQL_NO_DATA) {
			return (MYSQL_NO_DATA);
		}
		Trace(TR_ERR, "Error fetching path %d.%d: (%d) %s",
		    id.ino, id.gen, err, mysql_stmt_error(stmt));
		return (-1);
	}

	if (mysql_stmt_free_result(stmt) != 0) {
		Trace(TR_ERR, "Error freeing result: %s",
		    mysql_error(con->mysql));
	}

	return (0);
}

/*
 * sam_db_path_update - Updates path with provided data.
 * Returns: affected rows on success, -1 on error
 */
int
sam_db_path_update(sam_db_context_t *con, sam_db_path_t *path)
{
	MYSQL_BIND bind[3];
	memset(bind, 0, sizeof (bind));

	SAMDB_BIND_STR(bind[0], path->path);
	SAMDB_BIND(bind[1], path->ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[2], path->gen, MYSQL_TYPE_LONG, TRUE);

	if (sam_db_execute_sql(con, bind, NULL, PATH_UPDATE) == NULL) {
		Trace(TR_ERR, "Error updating path %d.%d",
		    path->ino, path->gen);
		return (-1);
	}

	return (mysql_affected_rows(con->mysql));
}

/*
 * sam_db_path_update_subdir - Updates paths that have the provided
 * oldpath as a prefix.
 * Returns: affected rows on success, -1 on error
 */
int
sam_db_path_update_subdir(
	sam_db_context_t *con,
	char *oldpath,
	sam_db_path_t *path)
{
	char prefix[MAXPATHLEN+2];
	int len;
	MYSQL_BIND bind[3];
	memset(bind, 0, sizeof (bind));

	/* Make prefix 'oldpath/%' so that LIKE term matches */
	len = strlcpy(prefix, oldpath, MAXPATHLEN);
	prefix[len++] = '%';
	prefix[len] = '\0';

	SAMDB_BIND_STR(bind[0], path->path);
	SAMDB_BIND_STR(bind[1], oldpath);
	SAMDB_BIND_STR(bind[2], prefix);

	if (sam_db_execute_sql(con, bind, NULL, PATH_UPDSUBDIR) == NULL) {
		Trace(TR_ERR, "Error updating path subdirs %d.%d",
		    path->ino, path->gen);
		return (-1);
	}

	return (mysql_affected_rows(con->mysql));
}

/*
 * sam_db_path_delete - Deletes path from database with inode, gen number.
 * Returns: affected rows on success, -1 on failure
 */
int
sam_db_path_delete(sam_db_context_t *con, sam_id_t id)
{
	MYSQL_BIND bind[2];
	memset(bind, 0, sizeof (bind));

	SAMDB_BIND(bind[0], id.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], id.gen, MYSQL_TYPE_LONG, TRUE);

	if (sam_db_execute_sql(con, bind, NULL, PATH_DELETE) == NULL) {
		Trace(TR_ERR, "Error deleting path: %s",
		    mysql_error(con->mysql));
		return (-1);
	}

	return (mysql_affected_rows(con->mysql));
}

/*
 * bind_path - Initializes the bind variable array with path values
 */
static void
bind_path(MYSQL_BIND *bind, sam_db_path_t *path, boolean_t is_result)
{
	SAMDB_BIND(bind[0], path->ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], path->gen, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND_STR(bind[2], path->path);
	if (is_result) {
		bind[2].buffer_length = sizeof (path->path);
	}
}

/*
 * build_path - Builds the path using the provided sam_perm_inode.
 *  path must be at least MAXPATHLEN size
 *  Directories have a trailing slash.
 *  Returns length of path, or -1 if error.
 */
static int
build_path(
	sam_db_context_t *con,
	sam_perm_inode_t *ip,
	int namehash,
	char *path)
{
	sam_perm_inode_t perm;
	int len;
	int name_len;
	char name[MAXNAMELEN];
	char *start;

	memcpy(&perm, ip, sizeof (sam_perm_inode_t));

	start = path + MAXPATHLEN;
	*start = '\0';
	len = 1;

	if (perm.di.id.ino != SAM_ROOT_INO) {
		do {
			name_len = sam_db_id_name(con, perm.di.parent_id,
			    perm.di.id, len == 1 ? namehash : -1, name);

			if (name_len <= 0) {
				Trace(TR_ERR, "%d.%d failed getting name",
				    ip->di.id.ino, ip->di.id.gen);
				return (-1);
			}

			if (len + name_len > MAXPATHLEN) {
				Trace(TR_ERR, "%d.%d path too long",
				    ip->di.id.ino, ip->di.id.gen);
				return (-1);
			}

			if (strlcpy(start-name_len-1, name, name_len+1) >
			    name_len+1) {
				Trace(TR_ERR, "unexpected name length %d.%d",
				    ip->di.id.ino, ip->di.id.gen);
				return (-1);
			}

			*(start-1) = '/';
			start -= name_len + 1;
			len += name_len + 1;

			sam_db_id_stat(con, perm.di.parent_id.ino,
			    perm.di.parent_id.gen, &perm);
		} while (perm.di.id.ino != SAM_ROOT_INO);
	}

	*--start = '/';
	len++;

	/* Remove trailing slash if not a directory */
	if (sam_db_inode_ftype(ip) != FTYPE_DIR) {
		len--;
		start[len-1] = '\0';
	}

	memmove(path, start, len);

	return (len);
}
