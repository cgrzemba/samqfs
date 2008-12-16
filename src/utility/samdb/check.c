/*
 * check.c - check sideband database for consistency with filesystem
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
#pragma ident "$Revision: 1.3 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>

#include <sys/vfs.h>

#include <pub/stat.h>

#include <sam/param.h>
#include <sam/sam_db.h>
#include <sam/sam_malloc.h>
#include <sam/fs/sblk.h>

#include "samdb.h"
#include "util.h"
#include "event_handler.h"

#define	LIST_GROW 10000
#define	CHECK_EQ(a, b, c) if ((a) != (b)) { reason = c; goto error; }
#define	QUIET_PRINT(f, ...) if (!quiet_mode) fprintf(f, __VA_ARGS__)
#define	DIR_ENTRY_SIZE(dp) \
	((sizeof (dir_entry_t)-NBPW + \
	((dp)->name_len + 1) + (NBPW-1)) & ~(NBPW-1))

/* Contains inode data from database to compare inodes against. */
typedef struct check_ino {
	sam_db_inode_t	inode;
	my_bool		copynull[MAX_ARCHIVE];
	uint_t		copytime[MAX_ARCHIVE];
} check_ino_t;

/* Contains directory data from database to compare dirents against. */
typedef struct check_dir {
	uint_t	ino;
	uint_t	gen;
	uint_t	p_ino;
	uint_t	p_gen;
	char	name[MAXNAMELEN];
	ulong_t	name_len;
} check_dir_t;

/* To keep track of entries that should be repaired */
typedef struct repair_entry {
	sam_id_t id;
	sam_id_t pid;
	int	 repair_dir;
} repair_entry_t;

/* To get list of dirents and sort them by id to compare against database */
typedef struct dir_entry {
	sam_id_t id;
	int name_len;
	char name[4];	/* Variable length string */
} dir_entry_t;

/* Inode callback arguments */
typedef struct inode_cb_arg {
	sam_db_context_t *check_con;
	MYSQL_STMT *check_stmt;
	sam_db_context_t *dir_con;
	MYSQL_STMT *dir_stmt;
} inode_cb_arg_t;

static int check_proc_opt(char opt, char *arg);
static MYSQL_STMT *execute_check_query(sam_db_context_t *con);
static MYSQL_STMT *prepare_dir_query(sam_db_context_t *con);
static int inode_callback(sam_perm_inode_t *ip, void *arg);
static int next_check_ino(MYSQL_STMT *stmt);
static int check_inode(sam_perm_inode_t *perm);
static int check_directory(sam_db_context_t *con, MYSQL_STMT *dir_stmt,
    sam_perm_inode_t *perm);
static int add_repair_entry(sam_id_t id, sam_id_t pid, boolean_t repair_dir);
static int add_dirent(sam_id_t pid, sam_dirent_t *dirent, void *arg);
static int cmp_dirent(const void *d1, const void *d2);

static boolean_t scan_only = FALSE;	/* True if no repairs should be done */
static boolean_t fast_scan = FALSE;	/* True if no directory checks */
static boolean_t quiet_mode = FALSE;	/* True if less output preferred */
static check_ino_t cur_check;		/* Current inode info from DB */
static check_dir_t cur_dir_check;	/* Current dirent info from DB */
static boolean_t result_done = FALSE;	/* True if done reading DB results */

static repair_entry_t *repair_list;	/* List of entries to repair */
static int repair_list_size;		/* Number of entries in repair list */
static int repair_list_len;		/* Alloc'd length of repair list */

static dir_entry_t *dir_list_buf;	/* Buffer for dirent list */
static dir_entry_t **dir_list;		/* List of pointers into buffer */
static int dir_list_buf_size;		/* Current buffer size (in bytes) */
static int dir_list_buf_len;		/* Alloc'd length of buffer */
static int dir_list_size;		/* Number of pointers in dir_list */
static int dir_list_len;		/* Alloc'd length of dir_list */

/*
 * Checks the database against the current state of the filesystem.
 *
 * Algorithm description:
 *	The information in the database should match precisely with the
 * 	information in the filesystem.
 *
 * 	1. Execute a query against the database for all inode information
 *	   in order by inode number.
 * 	2. For each entry in .inodes compare against the database.  Since
 *	   both lists are ordered by inode number, read and compare against
 *	   each other.  If there is an mismatch then add the inode to a
 *	   repair list.
 * 	3. For each directory in .inodes compare its entries against the
 *	   sam_file table.  This is done similiar to the overall loop by
 *	   sorting the directory entries by inode number and comparing
 *	   the result against the database.  This approach was taken to
 *	   reduce the number of queries against the database.
 * 	4. After all checks are made, run a consistency check for any inodes
 *	   in the repair list.
 */
int
samdb_check(samdb_args_t *args)
{
	sam_db_context_t dir_con;
	inode_cb_arg_t cb_arg;

	cb_arg.check_con = args->con;
	cb_arg.dir_con = &dir_con;

	/*
	 * Need a second connection for the directory checks.  This is
	 * so we can execute multiple queries simulateously.  Only one
	 * query per connection is allowed.
	 */
	memcpy(&dir_con, args->con, sizeof (sam_db_context_t));
	if (sam_db_connect(args->con) < 0) {
		fprintf(stderr, "Could not connect to %s database.\n",
		    args->fsname);
		goto out;
	}
	if (!fast_scan) {
		/* Second connection for directory checks */
		if (sam_db_connect(&dir_con) < 0) {
			fprintf(stderr, "Could not connect to %s database.\n",
			    args->fsname);
			goto out;
		}

		/* Prepare the directory check query (reused many times) */
		if ((cb_arg.dir_stmt = prepare_dir_query(&dir_con)) == NULL) {
			fprintf(stderr, "Error preparing directory check.");
			goto out;
		}
	}

	/*
	 * Set read and write timeout because checking dirents may take longer
	 * than the default timeout value causing the connection to be
	 * dropped.
	 */
	if (mysql_query(args->con->mysql, "SET net_read_timeout=3600") != 0) {
		fprintf(stderr, "Error setting read timeout.\n");
		goto out;
	}
	if (mysql_query(args->con->mysql, "SET net_write_timeout=3600") != 0) {
		fprintf(stderr, "Error setting read timeout.\n");
		goto out;
	}

	/* Execute the inode check query */
	if ((cb_arg.check_stmt = execute_check_query(args->con)) == NULL) {
		fprintf(stderr, "Error executing check query.\n");
		goto out;
	}

	/* Get the first result */
	if (next_check_ino(cb_arg.check_stmt) < 0) {
		fprintf(stderr, "Error getting check value.\n");
		goto out;
	}

	/* Read the inodes file, check database for each inode read */
	if (read_inodes(args->con->mount_point, inode_callback, &cb_arg) < 0) {
		fprintf(stderr, "Error reading inodes file\n");
		goto out;
	}

	/* Close check statement so we can use connection for repairs */
	if (cb_arg.check_stmt != NULL) {
		mysql_stmt_close(cb_arg.check_stmt);
		cb_arg.check_stmt == NULL;
	}

	printf("%d problems found\n", repair_list_size);
	if (repair_list_size > 0 && !scan_only) {
		int i;
		sam_event_t check_event;
		memset(&check_event, 0, sizeof (check_event));
		printf("Running consistency repairs..");
		for (i = 0; i < repair_list_size; i++) {
			check_event.ev_id = repair_list[i].id;
			check_event.ev_pid = repair_list[i].pid;
			if (check_consistency(cb_arg.check_con, &check_event,
			    repair_list[i].repair_dir) < 0) {
				fprintf(stderr, "Error checking inode %d\n",
				    check_event.ev_id.ino);
				break;
			}
			if (i%1000 == 0) {
				printf(".");
				fflush(stdout);
			}
		}
		printf(" done\n");
	}


out:
	if (cb_arg.check_stmt != NULL) {
		mysql_stmt_close(cb_arg.check_stmt);
	}
	if (!fast_scan && cb_arg.dir_stmt != NULL) {
		mysql_stmt_close(cb_arg.dir_stmt);
	}
	sam_db_disconnect(cb_arg.check_con);
	sam_db_disconnect(cb_arg.dir_con);

	return (0);
}

samdb_opts_t *
samdb_check_getopts(void)
{
	static opt_desc_t opt_desc[] = {
		{"f", "Fast scan, don't check directory namespace."},
		{"s", "Scan only, don't repair database"},
		{"q", "Quiet, only report how many errors"},
		{NULL}
	};
	static samdb_opts_t opts = {
		"fsq",
		check_proc_opt,
		opt_desc
	};
	return (&opts);
}

static int
/*LINTED argument unused in function */
check_proc_opt(char opt, char *arg)
{
	switch (opt) {
	case 'f': fast_scan = TRUE; break;
	case 's': scan_only = TRUE; break;
	case 'q': quiet_mode = TRUE; break;
	}
	return (0);
}

static int
inode_callback(sam_perm_inode_t *ip, void *arg) {
	inode_cb_arg_t *cb_arg = (inode_cb_arg_t *)arg;
	boolean_t retry_inode;

	/* Inodes that shouldn't be checked */
	if (!IS_DB_INODE(ip->di.id.ino) ||
	    !IS_DB_INODE(ip->di.parent_id.ino)) {
		return (0);
	}

retry:
	retry_inode = FALSE;
	if (ip->di.id.ino == cur_check.inode.ino) {
		if (ip->di.id.gen % 2 ||
		    ip->di.id.ino == SAM_ROOT_INO) {
			/*
			 * Check the inode and directory.  Repair reason
			 * is printed in the corresponding check method.
			 */
			if (check_inode(ip) < 0) {
				add_repair_entry(ip->di.id,
				    ip->di.parent_id, FALSE);
			} else if (!fast_scan && S_ISDIR(ip->di.mode)) {
				if (check_directory(cb_arg->dir_con,
				    cb_arg->dir_stmt, ip) < 0) {
					add_repair_entry(ip->di.id,
					    ip->di.parent_id, TRUE);
				}
			}
		} else {
			QUIET_PRINT(stderr, "Inactive inode %d "
			    "found in database.\n",
			    ip->di.id.ino);
			add_repair_entry(ip->di.id,
			    ip->di.parent_id, TRUE);
		}
	} else if (ip->di.id.ino < cur_check.inode.ino) {
		/* Odd gen is active, should be in database */
		if (ip->di.id.gen % 2 ||
		    ip->di.id.ino == SAM_ROOT_INO) {
			QUIET_PRINT(stderr, "Inode %d does "
			    "not exist in database.\n",
			    ip->di.id.ino);
			add_repair_entry(ip->di.id,
			    ip->di.parent_id, TRUE);
		}
		return (0);
	} else { /* ip->di.id.ino > cur_check.inode.ino */
		/* Database is incorrect, parent unknown */
		QUIET_PRINT(stderr, "Erroneous inode %d "
		    "found in database.\n",
		    cur_check.inode.ino);
		sam_id_t id = {cur_check.inode.ino,
		    cur_check.inode.gen};
		sam_id_t pid = {0, 0};
		add_repair_entry(id, pid, TRUE);
		retry_inode = TRUE;
	}

	/* Get next inode to check from database. */
	if (next_check_ino(cb_arg->check_stmt) < 0) {
		fprintf(stderr, "Error getting check value.\n");
		return (-1);
	}

	if (retry_inode) {
		goto retry;
	}

	return (0);
}

/*
 * Executes the query to check the inode information from the database.  This
 * retries all of the inode information from the database in order by inode
 * number.
 */
static MYSQL_STMT *
execute_check_query(sam_db_context_t *con)
{
	char *query = "select f.ino,f.gen,f.type,f.size,f.modify_time,"
	    "f.uid,f.gid,f.online,"
	    "(select a1.create_time from sam_archive a1 where "
	    "a1.ino=f.ino AND a1.gen=f.gen AND copy=0),"
	    "(select a1.create_time from sam_archive a1 where a1.ino=f.ino "
	    "AND a1.gen=f.gen AND copy=1),"
	    "(select a1.create_time from sam_archive a1 where a1.ino=f.ino "
	    "AND a1.gen=f.gen AND copy=2),"
	    "(select a1.create_time from sam_archive a1 where a1.ino=f.ino "
	    "AND a1.gen=f.gen AND copy=3) "
	    "from sam_inode f order by f.ino";
	MYSQL_BIND bind[12];
	MYSQL_STMT *stmt;

	/* Bind results */
	memset(bind, 0, sizeof (bind));
	SAMDB_BIND(bind[0], cur_check.inode.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], cur_check.inode.gen, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[2], cur_check.inode.type, MYSQL_TYPE_TINY, TRUE);
	SAMDB_BIND(bind[3], cur_check.inode.size, MYSQL_TYPE_LONGLONG, TRUE);
	SAMDB_BIND(bind[4], cur_check.inode.modify_time, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[5], cur_check.inode.uid, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[6], cur_check.inode.gid, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[7], cur_check.inode.online, MYSQL_TYPE_TINY, TRUE);
	SAMDB_BIND(bind[8], cur_check.copytime[0], MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[9], cur_check.copytime[1], MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[10], cur_check.copytime[2], MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[11], cur_check.copytime[3], MYSQL_TYPE_LONG, TRUE);

	bind[8].is_null = &cur_check.copynull[0];
	bind[9].is_null = &cur_check.copynull[1];
	bind[10].is_null = &cur_check.copynull[2];
	bind[11].is_null = &cur_check.copynull[3];

	if ((stmt = mysql_stmt_init(con->mysql)) == NULL) {
		fprintf(stderr, "Error initializing statement: %s",
		    mysql_error(con->mysql));
		return (NULL);
	}

	if (mysql_stmt_prepare(stmt, query, strlen(query)) != 0) {
		fprintf(stderr, "Error preparing statement: %s",
		    mysql_error(con->mysql));
		mysql_stmt_close(stmt);
		return (NULL);
	}

	if (mysql_stmt_bind_result(stmt, bind)) {
		fprintf(stderr, "Error binding result: (%d) %s",
		    mysql_stmt_errno(stmt), mysql_stmt_error(stmt));
		mysql_stmt_close(stmt);
		return (NULL);
	}

	if (mysql_stmt_execute(stmt)) {
		fprintf(stderr, "Error executing sql: (%d) %s",
		    mysql_stmt_errno(stmt), mysql_stmt_error(stmt));
		mysql_stmt_close(stmt);
		return (NULL);
	}

	return (stmt);
}

/*
 * Fetch the next result for checking inode.  If no more results then
 * set the current check inode number to max uint and set done.  This will
 * allow any unprocessed inodes to be added to the repair list.
 */
static int
next_check_ino(MYSQL_STMT *stmt)
{
	if (result_done) {
		return (0);
	}

	switch (mysql_stmt_fetch(stmt)) {
	case 0: break;
	case MYSQL_NO_DATA:
		cur_check.inode.ino = 0xFFFFFFFF;
		result_done = TRUE;
		break;
	default:
		fprintf(stderr, "Error getting next inode: %s\n",
		    mysql_stmt_error(stmt));
		return (-1);
	}
	return (0);
}

/*
 * Checks the current database inode against the provided disk inode.
 * Returns 0 if succesful compare, -1 otherwise.
 */
static int
check_inode(sam_perm_inode_t *perm)
{
	int copy;
	char *reason;

	CHECK_EQ(perm->di.id.ino, cur_check.inode.ino, "Inode num");
	CHECK_EQ(perm->di.id.gen, cur_check.inode.gen, "Gen num");
	CHECK_EQ(sam_db_inode_ftype(perm), cur_check.inode.type, "Type");
	CHECK_EQ(perm->di.rm.size, cur_check.inode.size, "Size");
	CHECK_EQ(perm->di.modify_time.tv_sec, cur_check.inode.modify_time,
	    "Modify time");
	CHECK_EQ(perm->di.uid, cur_check.inode.uid, "Uid");
	CHECK_EQ(perm->di.gid, cur_check.inode.gid, "Gid");
	CHECK_EQ(!perm->di.status.b.offline, cur_check.inode.online, "Online");

	for (copy = 0; copy < MAX_ARCHIVE; copy++) {
		if (perm->di.arch_status & (1<<copy)) {
			CHECK_EQ(0, cur_check.copynull[copy], "Copy not null");
			CHECK_EQ(perm->ar.image[copy].creation_time,
			    cur_check.copytime[copy], "Copy create time");
		} else {
			CHECK_EQ(1, cur_check.copynull[copy], "Copy null");
		}
	}

	return (0);

error:
	QUIET_PRINT(stderr, "Inode %d %s mismatch.\n", perm->di.id.ino, reason);
	return (-1);
}

/*
 * Prepares the directory check query.
 *
 * The input and results for this query are in the cur_dir_check variable.
 */
static MYSQL_STMT *
prepare_dir_query(sam_db_context_t *con) {
	MYSQL_STMT *stmt;
	char *query = "select name, ino, gen from sam_file where p_ino=? AND "
	    "p_gen=? order by ino, gen";
	MYSQL_BIND bind[2];
	MYSQL_BIND result[3];
	memset(&bind, 0, sizeof (bind));
	memset(&result, 0, sizeof (result));

	SAMDB_BIND(bind[0], cur_dir_check.p_ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], cur_dir_check.p_gen, MYSQL_TYPE_LONG, TRUE);

	result[0].buffer = cur_dir_check.name;
	result[0].buffer_length = sizeof (cur_dir_check.name);
	result[0].length = &cur_dir_check.name_len;
	SAMDB_BIND(result[1], cur_dir_check.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(result[2], cur_dir_check.gen, MYSQL_TYPE_LONG, TRUE);

	if ((stmt = mysql_stmt_init(con->mysql)) == NULL) {
		fprintf(stderr, "Error initializing statement: %s",
			mysql_error(con->mysql));
		return (NULL);
	}

	if (mysql_stmt_prepare(stmt, query, strlen(query)) != 0) {
		fprintf(stderr, "Error preparing statement: %s",
		    mysql_error(con->mysql));
		mysql_stmt_close(stmt);
		return (NULL);
	}

	if (mysql_stmt_bind_param(stmt, bind)) {
		fprintf(stderr, "Error binding params: (%d) %s",
		    mysql_stmt_errno(stmt), mysql_stmt_error(stmt));
		mysql_stmt_close(stmt);
		return (NULL);
	}

	if (mysql_stmt_bind_result(stmt, result)) {
		fprintf(stderr, "Error binding result: (%d) %s",
		    mysql_stmt_errno(stmt), mysql_stmt_error(stmt));
		mysql_stmt_close(stmt);
		return (NULL);
	}

	return (stmt);
}

/*
 * Checks the directory entries for perm against the current state of the
 * database.
 *
 * Returns 0 on successful compare, -1 otherwise.
 */
static int
check_directory(
	sam_db_context_t *con,
	MYSQL_STMT *dir_stmt,
	sam_perm_inode_t *perm)
{
	sam_db_path_t cur_path;
	sam_db_path_t db_path;
	sam_id_t all_ids = {0, -1};
	char *reason;
	int dir_list_i = 0;
	int err = 0;

	memset(&cur_path, 0, sizeof (sam_db_path_t));
	memset(&db_path, 0, sizeof (sam_db_path_t));
	dir_list_buf_size = 0;
	dir_list_size = 0;

	/* Check path */
	if (sam_db_path_new(con, perm->di.id, -1, &cur_path) < 0) {
		QUIET_PRINT(stderr, "Error getting directory %d Path.\n",
		    perm->di.id.ino);
		return (-1);
	}
	if (sam_db_path_select(con, perm->di.id, &db_path) < 0) {
		QUIET_PRINT(stderr, "%d database path does not exist.\n",
		    perm->di.id.ino);
		return (-1);
	}
	if (strcmp(cur_path.path, db_path.path) != 0) {
		QUIET_PRINT(stderr, "Directory %d Path mismatch.\n",
		    perm->di.id.ino);
		return (-1);
	}

	/* Build dir_list and sort */
	if (sam_db_id_allname(con, perm->di.id, all_ids,
	    add_dirent, NULL) < 0) {
		return (-1);
	}
	qsort(dir_list, dir_list_size, sizeof (dir_entry_t *), cmp_dirent);

	/* Set query parameters and execute */
	cur_dir_check.p_ino = perm->di.id.ino;
	cur_dir_check.p_gen = perm->di.id.gen;
	if (mysql_stmt_execute(dir_stmt)) {
		fprintf(stderr, "Error executing dir_check sql: (%d) %s",
		    mysql_stmt_errno(dir_stmt), mysql_stmt_error(dir_stmt));
		return (-1);
	}

	/* Fetch database results, comparing against sorted dir_list */
	while ((err = mysql_stmt_fetch(dir_stmt)) == 0) {
		dir_entry_t *cur_dir = dir_list[dir_list_i++];
		CHECK_EQ(cur_dir->id.ino, cur_dir_check.ino, "Inode");
		CHECK_EQ(cur_dir->id.gen, cur_dir_check.gen, "Gen");
		CHECK_EQ(cur_dir->name_len, cur_dir_check.name_len, "Name");
		CHECK_EQ(0, strncmp(cur_dir->name, cur_dir_check.name,
		    cur_dir->name_len), "Name");
	}

	if (err != MYSQL_NO_DATA) {
		fprintf(stderr, "Error fetching results: %d %s",
		    err, mysql_stmt_error(dir_stmt));
		reason = "fetch result";
		goto error;
	}

	return (0);
error:
	mysql_stmt_free_result(dir_stmt);
	QUIET_PRINT(stderr, "Directory %d %s mismatch.\n",
	    perm->di.id.ino, reason);
	return (-1);
}

/*
 * Adds a directory entry to dir_list.
 *
 * The optional arg is not used.
 */
static int
/*LINTED argument unused in function */
add_dirent(sam_id_t pid, sam_dirent_t *dirent, void *arg) {
	dir_entry_t *cur_ent;

	/* Ignore . and .. */
	if (dirent->d_name[0] == '.' && (dirent->d_namlen == 1 ||
	    (dirent->d_namlen == 2 && dirent->d_name[1] == '.'))) {
		return (0);
	}

	/* Ignore non database inodes */
	if (!IS_DB_INODE(dirent->d_id.ino)) {
		return (0);
	}

	/* Make sure enough space for entry */
	if (dir_list_buf_size +
	    sizeof (dir_entry_t) + dirent->d_namlen - 1 > dir_list_buf_len) {
		dir_list_buf_len += LIST_GROW *
		    (sizeof (dir_entry_t) + MAXNAMELEN);
		SamRealloc(dir_list_buf, dir_list_buf_len);
	}
	if (dir_list_size + sizeof (dir_entry_t *) > dir_list_len) {
		dir_list_len += LIST_GROW * sizeof (dir_entry_t *);
		SamRealloc(dir_list, dir_list_len);
	}

	/* Calculate current pointer into buffer */
	cur_ent = (dir_entry_t *)((char *)dir_list_buf + dir_list_buf_size);
	cur_ent->id = dirent->d_id;
	cur_ent->name_len = dirent->d_namlen;
	strncpy(cur_ent->name, (char *)dirent->d_name, dirent->d_namlen + 1);

	/* Make sure buf size is word aligned for next entry */
	dir_list_buf_size += DIR_ENTRY_SIZE(cur_ent);
	dir_list[dir_list_size++] = cur_ent;

	return (0);
}

/*
 * Comparison function to sort dir_list
 */
static int
cmp_dirent(const void *v1, const void *v2) {
	dir_entry_t *d1 = *((dir_entry_t **)v1);
	dir_entry_t *d2 = *((dir_entry_t **)v2);

	if (d1->id.ino == d2->id.ino) {
		return (0);
	} else if (d1->id.ino < d2->id.ino) {
		return (-1);
	} else {
		return (1);
	}
}

/*
 * Add an entry to the repair_list.
 */
static int
add_repair_entry(sam_id_t id, sam_id_t pid, boolean_t repair_dir)
{
	if (repair_list_size + 1 >= repair_list_len) {
		repair_list_len += LIST_GROW;
		SamRealloc(repair_list, repair_list_len *
		    sizeof (repair_entry_t));
	}

	repair_list[repair_list_size].id = id;
	repair_list[repair_list_size].pid = pid;
	repair_list[repair_list_size].repair_dir = repair_dir;
	repair_list_size++;

	return (0);
}
