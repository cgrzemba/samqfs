/*
 * samdb.h - definitions for samdb utilities
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

#ifndef SAMDB_H_
#define	SAMDB_H_

#include <sam/sam_db.h>

typedef struct samdb_util samdb_util_t;
typedef struct samdb_opts samdb_opts_t;
typedef struct samdb_args samdb_args_t;

/* Function prototypes for utility functions */
typedef samdb_opts_t *(*samdb_getopts_f)(void);
typedef int (*samdb_procopt_f)(char opt, char *arg);
typedef int (*samdb_exec_f)(samdb_args_t *args);

/* Description of utility options */
typedef struct opt_desc {
	char *opt;
	char *desc;
} opt_desc_t;

/* Option structure for utilities */
struct samdb_opts {
	char *optstr;	/* getopt string */
	samdb_procopt_f proc_opt;
	opt_desc_t *opt_desc;
};

/* Utility arguments for exec function */
struct samdb_args {
	char *fsname;
	samdb_util_t *util;
	sam_db_context_t *con;
	int argc;
	char **argv;
};

/* Utility list entry */
struct samdb_util {
	char *name;
	char *desc;
	samdb_exec_f exec;
	samdb_getopts_f getopts;
};

/* Usage function */
int samdb_usage(samdb_args_t *args);
samdb_opts_t *samdb_usage_getopts(void);

/* Samdb utilities */
int samdb_check(samdb_args_t *args);
int samdb_create(samdb_args_t *args);
int samdb_dump(samdb_args_t *args);
int samdb_drop(samdb_args_t *args);
int samdb_load(samdb_args_t *args);
int samdb_query(samdb_args_t *args);

samdb_opts_t *samdb_check_getopts(void);
samdb_opts_t *samdb_create_getopts(void);
samdb_opts_t *samdb_dump_getopts(void);
samdb_opts_t *samdb_drop_getopts(void);
samdb_opts_t *samdb_load_getopts(void);
samdb_opts_t *samdb_query_getopts(void);

#ifdef _DECL
/*
 * List of samdb utilities.  samdb.c uses this list to parse command
 * line options and execute the utilities.
 */
samdb_util_t samdb_utils[] = {
	{
		"check",
		"Consistency check database against filesystem",
		samdb_check,
		samdb_check_getopts
	}, {
		"create",
		"Create filesystem database",
		samdb_create,
		samdb_create_getopts
	}, {
		"dump",
		"Create file list for use with samfsdump",
		samdb_dump,
		samdb_dump_getopts
	}, {
		"drop",
		"Drop filesystem database",
		samdb_drop,
		samdb_drop_getopts
	}, {
		"load",
		"Load database from load file",
		samdb_load,
		samdb_load_getopts
	}, {
		"query",
		"Run a query against the database",
		samdb_query,
		samdb_query_getopts
	}, {
		"help",
		"Get help for samdb commands",
		samdb_usage,
		samdb_usage_getopts
	}, {NULL}
};
#else
extern samdb_util_t samdb_utils[];
#endif /* _DECL */

#endif /* SAMDB_H_ */
