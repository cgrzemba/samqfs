/*
 * dump.c - dump a sideband database for a filesystem
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

#include <stdio.h>

#include <sys/vfs.h>

#include <sam/sam_db.h>
#include <sam/fs/sblk.h>

#include "samdb.h"
#include "event_handler.h"

static char *dumpfile;
static boolean_t is_abs_paths = FALSE;
static boolean_t is_sorted = FALSE;

static int dump_proc_opt(char opt, char *args);
static int dump_files(sam_db_context_t *con, FILE *file);

int
samdb_dump(samdb_args_t *args) {
	FILE *file;
	if (dumpfile == NULL) {
		file = stdout;
	} else if ((file = fopen(dumpfile, "w")) == NULL) {
		fprintf(stderr, "Could not open %s.", args->fsname);
		return (-1);
	}

	if (sam_db_connect(args->con) < 0) {
		fprintf(stderr, "Can't connect to %s database.", args->fsname);
	}

	if (dump_files(args->con, file) < 0) {
		return (-1);
	}

	sam_db_disconnect(args->con);
	return (0);
}

samdb_opts_t *
samdb_dump_getopts(void) {
	static opt_desc_t opt_desc[] = {
		{"a", "Absolute paths in dump, default is relative."},
		{"f file", "Output file for dump, default standard output."},
		{"s", "Dump file sorted by directory."},
		NULL
	};
	static samdb_opts_t opts = {
		"af:s",
		dump_proc_opt,
		opt_desc
	};
	return (&opts);
}

static int
dump_proc_opt(char opt, char *arg) {
	switch (opt) {
	case 'a': is_abs_paths = TRUE; break;
	case 'f': dumpfile = arg; break;
	case 's': is_sorted = TRUE; break;
	}
	return (0);
}

static int
dump_files(sam_db_context_t *con, FILE *file) {
	MYSQL_RES *result;
	MYSQL_ROW row;

	int len;
	char *query = "select f.ino, f.gen, concat((select p.path from "
	    "sam_path p where p.ino=f.p_ino and p.gen=f.p_gen), "
	    "f.name) from sam_file f %s where f.ino!=%d %s";

	len = sprintf(con->qbuf, query,
	    is_sorted ? "force index (PRIMARY)" : "",
	    SAM_ROOT_INO,
	    is_sorted ? "order by f.p_ino" : "");

	if (mysql_real_query(con->mysql, con->qbuf, len)) {
		fprintf(stderr, "%s\n", mysql_error(con->mysql));
		return (-1);
	}

	if ((result = mysql_use_result(con->mysql)) == NULL) {
		fprintf(stderr, "%s\n", mysql_error(con->mysql));
		return (-1);
	}

	while ((row = mysql_fetch_row(result)) != NULL) {
		fprintf(file, "%s\t%s\t%s%s\n", row[0], row[1],
		    is_abs_paths ? con->mount_point : ".", row[2]);
	}

	mysql_free_result(result);

	return (0);
}
