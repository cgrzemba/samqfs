/*
 * query.c - query a sideband database for a filesystem
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

#include <stdio.h>
#include <strings.h>

#include <sam/sam_malloc.h>
#include <sam/sam_db.h>

#include "samdb.h"

#define	LIST_GROW 1000

typedef enum query_type {
	FILES = 0,
	VSNS,
	VSN_FILES
} query_type_t;

/* For lists of command line terms (vsns, files, inodes) */
typedef struct opt_list {
	int	size;
	int	len;
	char	**val;
} opt_list_t;

static query_type_t query_type;
static boolean_t is_count = FALSE;
static boolean_t is_sorted = FALSE;
static opt_list_t vsn_list;
static opt_list_t file_list;
static opt_list_t inode_list;

static int query_opt_proc(char opt, char *arg);
static void opt_list_add(opt_list_t *list, char *val);
static void opt_list_free(opt_list_t *list);
static int build_query(sam_db_context_t *con);
static char *build_list(sam_db_context_t *con, char *col,
    opt_list_t *list, char *build);
static int execute_query(sam_db_context_t *con);

int samdb_query(samdb_args_t *args) {
	if (sam_db_connect(args->con) < 0) {
		fprintf(stderr, "Can't connect to %s database\n", args->fsname);
		return (-1);
	}
	if (build_query(args->con) < 0) {
		fprintf(stderr, "Failure building query\n");
		goto out;
	}
	if (execute_query(args->con) < 0) {
		fprintf(stderr, "Failure executing query\n");
		goto out;
	}

out:
	sam_db_disconnect(args->con);
	opt_list_free(&vsn_list);
	opt_list_free(&file_list);
	opt_list_free(&inode_list);
	return (0);
}

samdb_opts_t *
samdb_query_getopts(void) {
	static opt_desc_t opt_desc[] = {
		{"t {vsn,file}\n\t", "Type of query result, default is file"},
		{"c", "Only display count of results"},
		{"s", "Sort results"},
		{"v vsn", "Vsn to query"},
		{"f file", "File to query, can use % wildcards"},
		{"i ino", "Inode number to query"},
		{"\b \n\tLike terms are OR'd, unlike terms are AND'd.", ""},
		NULL
	};
	static samdb_opts_t opts = {
		"t:csv:f:i:",
		query_opt_proc,
		opt_desc
	};

	return (&opts);
}

static int
query_opt_proc(char opt, char *arg) {
	switch (opt) {
	case 't':
		if (strcasecmp(arg, "vsn") == 0) {
			query_type = VSNS;
		} else if (strcasecmp(arg, "file") == 0) {
			if (query_type == VSNS) {
				query_type = VSN_FILES;
			} else {
				query_type = FILES;
			}
		} else {
			fprintf(stderr, "Unrecognized query type '%s'", arg);
			return (-1);
		}
		break;
	case 'c':
		is_count = TRUE;
		break;
	case 's':
		is_sorted = TRUE;
		break;
	case 'v':
		opt_list_add(&vsn_list, arg);
		break;
	case 'f':
		opt_list_add(&file_list, arg);
		break;
	case 'i':
		opt_list_add(&inode_list, arg);
		break;
	}

	return (0);
}

/* Add entry to provided list, allocating as necessary */
static void
opt_list_add(opt_list_t *list, char *val)
{
	/* Check available space */
	if ((list->size + 1) * sizeof (char *) > list->len) {
		list->len += LIST_GROW * sizeof (char *);
		SamRealloc(list->val, list->len);
	}

	list->val[list->size++] = val;
}

/* Free memory used by list, resets size and length to 0 */
static void
opt_list_free(opt_list_t *list) {
	if (list->val != NULL) {
		SamFree(list->val);
		list->val = NULL;
		list->size = 0;
		list->len = 0;
	}
}

/*
 * Build the query specified by the user provided options.
 *
 * There are five types of query results that could be produced:
 *  - vsn list or count
 *  - file list or count
 *  - file list grouped by vsn
 *
 * In addition the results can be chosen by the user to be sorted.
 *
 * The user can query based on vsn, file or inode.  The vsn and inode
 * are absolute matches and do not support wildcards.  The file supports
 * wildcards.
 *
 * The file is broken into a path and name portion.  For example /dir1/filea
 * would be broken into path=/dir1/ name=filea.  The path and file can both
 * contain % wildcards.  So /dir%/file% would match all files prefixed with
 * 'file' in all root directories prefixed with 'dir'.
 *
 * Like terms are OR'd together.  So the terms vsn=123 vsn=456 file=a would
 * be grouped as: (vsn=123 OR vsn=456) AND file=a.
 */
static int
build_query(sam_db_context_t *con) {
	int i;
	boolean_t use_archive = vsn_list.size > 0;
	char *build = con->qbuf;

	/* Build the query result list */
	build = strmov(build, "select ");
	switch (query_type) {
	case VSNS:
		if (is_count) {
			build = strmov(build, "count(distinct a.vsn)");
		} else {
			build = strmov(build, "distinct a.vsn");
		}
		use_archive = TRUE;
		break;
	case VSN_FILES:
		if (is_count) {
			query_type = FILES;
		} else {
			build = strmov(build, "a.vsn,");
			use_archive = TRUE;
		}
		/*LINTED fallthrough */
	case FILES:
		if (is_count) {
			build = strmov(build, "count(distinct f.ino)");
		} else {
			build = strmov(build, "concat(p.path, f.name)");
		}
		break;
	}

	/* Optimize no-term file count */
	if (query_type == FILES && is_count && inode_list.size == 0 &&
	    vsn_list.size == 0 && file_list.size == 0) {
		(void) strmov(con->qbuf, "select count(*) from sam_file");
		return (0);
	}

	/* Build which tables we are selecting from */
	build = strmov(build, " from sam_file f LEFT JOIN sam_path p "
	    "ON (p.ino=f.p_ino AND p.gen=f.p_gen) ");
	if (use_archive) {
		build = strmov(build, "LEFT OUTER JOIN sam_archive a "
		    "ON (f.ino=a.ino AND f.gen=a.gen) ");
	}

	/* Build the where list, the 1=1 is to make a no term query work */
	build = strmov(build, "where 1=1 AND ");
	if (vsn_list.size > 0) {
		build = build_list(con, "a.vsn", &vsn_list, build);
		build = strmov(build, " AND ");
	}

	if (inode_list.size > 0) {
		build = build_list(con, "f.ino", &inode_list, build);
		build = strmov(build, " AND ");
	}

	/*
	 * Build the file where clause.  This will be of the form:
	 *  (path LIKE 'path1' AND file LIKE 'file1') OR (...)
	 */
	if (file_list.size > 0) {
		for (i = 0; i < file_list.size; i++) {
			char *name;
			char *path;
			char *sep = strrchr(file_list.val[i], '/');
			if (sep == NULL) {
				name = file_list.val[i];
				*build++ = '(';
			} else {
				/* Make path to just after separator */
				char save = *(sep+1);
				*(sep+1) = '\0';

				path = file_list.val[i];

				/* Don't include mount point in query */
				if (strncmp(path, con->mount_point,
				    strlen(con->mount_point)) == 0) {
					path = path + strlen(con->mount_point);
				}
				build = strmov(build, "(p.path LIKE '");
				build += mysql_real_escape_string(con->mysql,
				    build, path, strlen(path));
				build = strmov(build, "' AND ");

				/* Restore char after sep and set name */
				*(sep+1) = save;
				name = sep + 1;
			}

			build = strmov(build, "f.name LIKE '");
			build += mysql_real_escape_string(con->mysql,
			    build, name, strlen(name));
			build = strmov(build, "') OR ");
		}
		build -= 3; /* Skip past last OR */
		build = strmov(build, " AND ");
	}

	build -= 4; /* Remove last AND */
	*build = '\0';

	/* Order by clause */
	switch (query_type) {
	case VSNS:
		build = strmov(build, "order by a.vsn");
		break;
	case VSN_FILES:
		build = strmov(build, "order by a.vsn");
		if (is_sorted) {
			build = strmov(build, ",p.path,f.name");
		}
		break;
	case FILES:
		if (is_sorted) {
			build = strmov(build, "order by p.path,f.name");
		}
		break;
	}

	return (0);
}

/*
 * Build an IN list where clause from the provided list.
 * This looks like "col IN ('val1', ..., 'valn')"
 */
static char *
build_list(sam_db_context_t *con, char *col, opt_list_t *list, char *build) {
	int i;
	build = strmov(build, col);
	build = strmov(build, " IN (");
	for (i = 0; i < list->size; i++) {
		*build++ = '\'';
		build += mysql_real_escape_string(con->mysql, build,
		    list->val[i], strlen(list->val[i]));
		build = strmov(build, "',");
	}
	*(build-1) = ')';

	return (build);
}

/*
 * Executes the query that was built from build_query.
 * Outputs the result to the standard output.
 */
static int
execute_query(sam_db_context_t *con) {
	MYSQL_RES *result;
	MYSQL_ROW row;
	char prev_vsn[32];
	boolean_t saw_null = FALSE;
	*prev_vsn = '\0';

	if (mysql_real_query(con->mysql, con->qbuf, strlen(con->qbuf))) {
		fprintf(stderr, "%s\n", mysql_error(con->mysql));
		return (-1);
	}

	if ((result = mysql_use_result(con->mysql)) == NULL) {
		fprintf(stderr, "%s\n", mysql_error(con->mysql));
		return (-1);
	}

	/* Output each result according to the type of query executed */
	while ((row = mysql_fetch_row(result)) != NULL) {
		if (is_count) {
			printf("%s\n", row[0]);
		} else {
			switch (query_type) {
			case VSNS:
				if (row[0] != NULL) {
					printf("%s\n", row[0]);
				}
				break;
			case FILES:
				if (row[0] != NULL) {
					printf("%s%s\n", con->mount_point,
					    row[0]);
				}
				break;
			case VSN_FILES:
				if (row[1] != NULL) {
					if (row[0] == NULL) {
						if (!saw_null) {
							printf("No archive "
							    " copies:\n");
							saw_null = TRUE;
						}
					} else if (strcmp(prev_vsn,
					    row[0]) != 0) {
						printf("%s:\n", row[0]);
						strcpy(prev_vsn, row[0]);
					}
					printf("\t%s%s\n", con->mount_point,
					    row[1]);
				}
			}
		}
	}

	mysql_free_result(result);
	return (0);
}
