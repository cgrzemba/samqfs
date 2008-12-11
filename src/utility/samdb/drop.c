/*
 * drop.c - drop a sideband database for a filesystem
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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "samdb.h"

static int drop_proc_opt(char opt, char *arg);
static int drop_database(sam_db_context_t *con);

int
samdb_drop(samdb_args_t *args) {
	printf("Confirm drop database for %s? (y/N) ", args->fsname);
	if (tolower(getchar()) != 'y') {
		return (0);
	}

	if (sam_db_connect(args->con) < 0) {
		fprintf(stderr, "Can't connect to %s database.", args->fsname);
		return (-1);
	}

	if (drop_database(args->con) < 0) {
		fprintf(stderr, "Could not drop %s database.", args->fsname);
		sam_db_disconnect(args->con);
		return (-1);
	}
	sam_db_disconnect(args->con);

	return (0);
}

samdb_opts_t *
samdb_drop_getopts(void) {
	static opt_desc_t optdesc[] = {
		NULL
	};
	static samdb_opts_t opts = {
		"",
		drop_proc_opt,
		optdesc
	};

	return (&opts);
}

static int
/*LINTED argument unused in function */
drop_proc_opt(char opt, char *arg) {
	return (0);
}

static int
drop_database(sam_db_context_t *con) {
	int len = sprintf(con->qbuf, "drop database if exists %s", con->dbname);
	if (mysql_real_query(con->mysql, con->qbuf, len)) {
		fprintf(stderr, "%s/n", mysql_error(con->mysql));
		return (-1);
	}
	return (0);
}
