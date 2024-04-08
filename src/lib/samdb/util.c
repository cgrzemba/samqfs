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
 * or https://illumos.org/license/CDDL.
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

#pragma ident "$Revision: 1.4 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <nl_types.h>
#include <unistd.h>

#include <pub/stat.h>
#include <sam/custmsg.h>
#include <sam/sam_trace.h>
#include <sam/sam_malloc.h>
#include <sam/sam_db.h>
#include <sam/types.h>
#include <sam/lib.h>
#include <sam/uioctl.h>
#include <sam/fs/ino.h>
#include <sam/fs/dirent.h>
#include <sam/sam_db.h>
#include <sam/sam_malloc.h>
#include <sam/sam_trace.h>

#define	SQL_CATSET	1

static int cache_put(sam_db_context_t *con, int sql_id,
    MYSQL_STMT *stmt, MYSQL_STMT **del_stmt);
static MYSQL_STMT *cache_get(sam_db_context_t *con, int sql_id);

/*
 * sam_db_get_sql - Gets SQL statements from samdb catalog.  These statements
 *   are static and predefined.
 *
 *	sql_id - id of the sql statement to get, defined in catalog
 *
 * 	return: a pointer to the sql string, or null if no entry with that id
 */
char *
sam_db_get_sql(int sql_id)
{
	static nl_catd sql_catd = (nl_catd)-1;
	char *sql;

	/* Initialize catalog if needed */
	if ((int)sql_catd == -1) {
		sql_catd = catopen(SAMDB_SQL_CATALOG, NL_CAT_LOCALE);
		if ((int)sql_catd == -1) {
			Trace(TR_ERR, "Error opening sql catalog");
			return (NULL);
		}
	}

	sql = catgets(sql_catd, SQL_CATSET, sql_id, NULL);

	if (sql == NULL) {
		Trace(TR_ERR, "No SQL statement exists with id %d.", sql_id);
	}

	return (sql);
}

/*
 * sam_db_get_stmt - Gets a MYSQL Prepared statement that has
 *   already been prepared.  The statement may have been cached
 *   from a previous execution.
 */
MYSQL_STMT *
sam_db_get_stmt(sam_db_context_t *con, int sql_id)
{
	MYSQL_STMT *stmt, *del_stmt;
	char *sql;

	stmt = cache_get(con, sql_id);
	if (stmt == NULL) {
		if ((sql = sam_db_get_sql(sql_id)) == NULL) {
			Trace(TR_ERR, "Error getting sql %d", sql_id);
			return (NULL);
		}

		if ((stmt = mysql_stmt_init(con->mysql)) == NULL) {
			Trace(TR_ERR, "Error initializing statement: %s",
			    mysql_error(con->mysql));
			return (NULL);
		}

		if ((mysql_stmt_prepare(stmt, sql, strlen(sql))) != 0) {
			Trace(TR_ERR, "Error preparing statement %d: %s",
			    sql_id, mysql_error(con->mysql));
			mysql_stmt_close(stmt);
			return (NULL);
		}

		cache_put(con, sql_id, stmt, &del_stmt);
		if (del_stmt != NULL) {
			if (mysql_stmt_close(del_stmt) != 0) {
				Trace(TR_ERR, "Error closing removed "
				    "statement: %s",
				    mysql_error(con->mysql));
			}
		}
	}

	return (stmt);
}

/*
 * Execute the SQL with given sql_id using the bind and result parameters.
 * To not use either bind or result paramters NULL should be passed in.
 * It is the callers responsibility to fetch the results from the
 * execution.
 *
 * Returns a pointer to the executed statement on success, NULL on failure.
 */
MYSQL_STMT *
sam_db_execute_sql(
	sam_db_context_t *con,
	MYSQL_BIND *bind,
	MYSQL_BIND *result,
	int sql_id)
{
	MYSQL_STMT *stmt;

	if ((stmt = sam_db_get_stmt(con, sql_id)) == NULL) {
		Trace(TR_ERR, "Error getting statment (id%d)", sql_id);
		return (NULL);
	}

	if (bind != NULL) {
		if (mysql_stmt_bind_param(stmt, bind) != 0) {
			Trace(TR_ERR, "Error binding params (id%d): (%d) %s",
			    sql_id, mysql_stmt_errno(stmt),
			    mysql_stmt_error(stmt));
			return (NULL);
		}
	}

	if (result != NULL) {
		if (mysql_stmt_bind_result(stmt, result) != 0) {
			Trace(TR_ERR, "Error binding result (id%d): (%d) %s",
			    sql_id, mysql_stmt_errno(stmt),
			    mysql_stmt_error(stmt));
			return (NULL);
		}
	}

	if (mysql_stmt_execute(stmt) != 0) {
		Trace(TR_ERR, "Error executing sql (id%d): (%d) %s", sql_id,
		    mysql_stmt_errno(stmt), mysql_stmt_error(stmt));
		return (NULL);
	}

	return (stmt);
}

/*
 * Populate the given inode from the filesystem.
 */
int
sam_db_id_stat(
	sam_db_context_t *con,
	unsigned int ino,	/* Inode number for query */
	unsigned int gen, /* Generation number for query */
	struct sam_perm_inode *inode) /* Perminant inode */
{
	struct sam_ioctl_idstat request; /* Stat request */

	request.id.ino = ino;
	request.id.gen = gen;
	request.size = sizeof (struct sam_perm_inode);
	request.dp.ptr = (void *)inode;

	if (ioctl(con->sam_fd, F_IDSTAT, &request) < 0) {
		return (-1);
	}
	return (0);
}

/*
 * 	sam_db_id_name - Find Inode in Directory using F_IDGETDENTS Request.
 *
 *	Description:
 *	    Search the given directory for the specified inode number
 *	    and return entry name. F_IDGETDENTS ioctl(2) request is used.
 *
 *	On Entry:
 *	    con		Database context
 *	    dir		Inode/generation number of directory to search.
 *	    id		Inode/generation number of file to find.
 *	    name	buffer at least MAXNAMELEN+1 length
 *	    hash	hash code to match, -1 if any hash code
 *
 *	Returns:
 *	    int - length of name, -1 if error
 *	    name - entry name copied into buffer, null terminated
 */
int
sam_db_id_name(
	sam_db_context_t *con,
	sam_id_t dir,	/* Inode number of directory */
	sam_id_t id,	/* Inode number to locate */
	int hash,	/* Hash of name to locate, -1 if any */
	char *name)
{
	sam_ioctl_idgetdents_t request;	/* Getdents request */
	char *dirbuf;
	char *endbuf;
	struct sam_dirent *dirp;

	int n;
	int len = -1;

	request.id = dir;
	request.size = 1000 * sizeof (struct sam_dirent);
	SamMalloc(dirbuf, request.size);
	request.dir.ptr = (void *) dirbuf;
	request.offset = 0;
	request.modify_time.tv_sec = 0;
	request.eof = 0;

	while (!request.eof) {
		if ((n = ioctl(con->sam_fd, F_IDGETDENTS, &request)) < 0) {
			if (errno == ESTALE) {
				errno = 0;
				request.offset = 0;
				request.modify_time.tv_sec = 0;
				continue;
			}
			len = -1;
			goto exit;
		}

		endbuf = dirbuf + n;
		dirp = (struct sam_dirent *)((void *)dirbuf);
		while ((char *)dirp < endbuf) {
			if (dirp->d_fmt != 0 &&
			    dirp->d_id.ino == id.ino &&
			    dirp->d_id.gen == id.gen &&
			    (hash < 0 || hash == dirp->d_namehash)) {
				len = strlcpy(name, (char *)dirp->d_name,
				    MAXNAMELEN+1);
				if (len >= MAXNAMELEN+1) {
					len = -1;
				}
				goto exit;
			}

			dirp = (struct sam_dirent *)((void *)((char *)dirp +
			    SAM_DIRSIZ(dirp)));
		}
	}

exit:
	SamFree(dirbuf);
	return (len);
}

/*
 * 	sam_db_id_allname - Find all names in Directory using
 *	    F_IDGETDENTS Request.
 *
 *	Description:
 *	    Search the given directory for the specified inode number
 *	    and return entry name. F_IDGETDENTS ioctl(2) request is used.
 *
 *	On Entry:
 *	    dir		Inode/generation number of directory to search.
 *	    id		Inode/generation number of file to find.
 * 			Id of (0,-1) will match all entries
 *	    dirent_callback Callback function to call for each name found.
 *
 *	Returns:
 *	    0 on success, -1 if error
 */
int
sam_db_id_allname(
	sam_db_context_t *con,
	sam_id_t dir,	/* Inode number of directory */
	sam_id_t id,	/* Inode number to locate */
	sam_db_dirent_cb dirent_callback,
	void *cb_args)
{
	sam_ioctl_idgetdents_t request;	/* Getdents request */
	char *dirbuf;
	char *endbuf;
	struct sam_dirent *dirp;

	int n;
	int ret = 0;

	request.id = dir;
	request.size = 1000 * sizeof (struct sam_dirent);
	SamMalloc(dirbuf, request.size);
	request.dir.ptr = (void *) dirbuf;
	request.offset = 0;
	request.modify_time.tv_sec = 0;
	request.eof = 0;

	while (!request.eof) {
		if ((n = ioctl(con->sam_fd, F_IDGETDENTS, &request)) < 0) {
			if (errno == ESTALE) {
				errno = 0;
				request.offset = 0;
				request.modify_time.tv_sec = 0;
				continue;
			}
			ret = -1;
			goto exit;
		}

		endbuf = dirbuf + n;
		dirp = (struct sam_dirent *)((void *)dirbuf);
		while ((char *)dirp < endbuf) {
			if (dirp->d_fmt != 0 &&
			    id.ino == 0 || (
			    dirp->d_id.ino == id.ino &&
			    dirp->d_id.gen == id.gen)) {
				if (dirent_callback(dir, dirp, cb_args) < 0) {
					ret = -1;
					goto exit;
				}
			}

			dirp = (struct sam_dirent *)((void *)((char *)dirp +
			    SAM_DIRSIZ(dirp)));
		}
	}

exit:
	SamFree(dirbuf);
	return (ret);
}

/*
 * 	sam_db_id_mva - Issue Multivolume Archive Request.
 *
 *	Description:
 *	    Get multivolume archive information.
 *
 *	On Entry:
 *	    inode	Inode image.
 *	    copy	Archive copy.
 *
 *	On Return:
 *	    vsnp	VSN table pointer (dynamicly allocated).
 *			NULL if no VSNs returned.  Caller must free.
 *
 *	Returns:
 *	    Number of VSNs in multivolume archives. -1 if error.
 */
int
sam_db_id_mva(
	sam_db_context_t *con,		/* Database context */
	struct sam_perm_inode *inode,	/* Perminant inode */
	int copy,			/* Archive copy */
	struct sam_vsn_section **vsnp)	/* VSN table */
{
	struct sam_ioctl_idmva request; /* Multi-volume archive request	*/
	int nvsn; /* Number of VSNs */
	int i;

	*vsnp = NULL;
	nvsn = 0;

	if (copy < 0 || copy >= MAX_ARCHIVE) {
		Trace(TR_ERR, "Invalid copy number");
		return (-1);
	}

	if (inode->di.version < SAM_INODE_VERS_2) {
		Trace(TR_ERR, "Unsupported V1 inode found");
		return (-1);
	}

	nvsn = inode->ar.image[copy].n_vsns;
	if (nvsn <= 1) {
		return (0);
	}

	SamMalloc(*vsnp, nvsn * sizeof (struct sam_vsn_section));
	request.id.ino = inode->di.id.ino;
	request.id.gen = inode->di.id.gen;
	request.size = nvsn * sizeof (struct sam_vsn_section);
	request.copy = copy;
	request.buf.ptr = (void *)*vsnp;

	for (i = 0; i < MAX_ARCHIVE; i++) {
		request.aid[i].ino = request.aid[i].gen = 0;
	}

	if (ioctl(con->sam_fd, F_IDCPMVA, &request) < 0) {
		return (-1);
	}
	return (nvsn);
}

/*
 * Puts a mysql statement into the cache with the corresponding id.
 * The cache uses a least-recently-used (LRU) replacment policy.
 *
 * Return:
 * 	int - the cache index that the statement resides
 * 	del_stmt (optional) - address of replaced entry
 */
static int
cache_put(
	sam_db_context_t *con,
	int sql_id,
	MYSQL_STMT *stmt,
	MYSQL_STMT **del_stmt)
{
	int idx;
	int i;
	if (del_stmt != NULL) {
		*del_stmt = NULL;
	}

	/* Find index for adding/replacing */
	if (con->cache_size < SAMDB_CACHE_LEN) {
		idx = con->cache_size++;
	} else {
		int max_i, max_time;
		max_i = 0;
		max_time = con->stmt_cache[0].last_access;

		for (i = 1; i < con->cache_size; i++) {
			if (con->stmt_cache[i].last_access > max_time) {
				max_i = i;
				max_time = con->stmt_cache[i].last_access;
			}
		}

		idx = max_i;
		if (del_stmt != NULL) {
			*del_stmt = con->stmt_cache[idx].stmt;
		}
	}

	/* Age all entries */
	for (i = 0; i < con->cache_size; i++) {
		con->stmt_cache[i].last_access++;
	}

	/* Insert into cache */
	con->stmt_cache[idx].sql_id = sql_id;
	con->stmt_cache[idx].last_access = 0;
	con->stmt_cache[idx].stmt = stmt;

	return (idx);
}

/*
 * cache_get - gets a statement from the cache, if it exists
 *
 * Return - mysql statement corresponding to sql_id, or
 *    null if the statement is not in the cache
 */
static MYSQL_STMT *
cache_get(sam_db_context_t *con, int sql_id)
{
	MYSQL_STMT *stmt = NULL;
	int i;

	/* Loop to find id, and age as we go */
	for (i = 0; i < con->cache_size; i++) {
		if (con->stmt_cache[i].sql_id == sql_id) {
			stmt = con->stmt_cache[i].stmt;
			con->stmt_cache[i].last_access = 0;
		} else {
			con->stmt_cache[i].last_access++;
		}
	}

	return (stmt);
}
