/*
 *	samdb.c - Database utilities
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

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

#define	_DECL
#include "samdb.h"
#undef	_DECL

#include "util.h"

static boolean_t verbose_usage = FALSE;
static int usage_proc_opt(char opt, char *arg);
static void print_usage(samdb_util_t *util);

/*
 * Parses the command line to execute the utilities defined in samdb.h.
 * The command line format is: samdb command fsname opts
 */
int main(
	int argc,
	char **argv)
{
	char *util_name;
	samdb_util_t *util;
	samdb_opts_t *util_opts;
	samdb_args_t util_args;
	sam_db_conf_t *db_conf;
	int errflg = 0;
	char opt;

	init_trace(FALSE, 0);
	memset(&util_args, 0, sizeof (samdb_args_t));

	if (argc < 3) {
		samdb_usage(NULL);
		return (EXIT_FAILURE);
	}

	/* All commands except for help have fsname as their second parameter */
	util_name = argv[1];
	if (strcmp("help", util_name) == 0) {
		argc -= 1;
		argv += 1;
	} else {
		util_args.fsname = argv[2];
		argc -= 2;
		argv += 2;
	}

	/* Find the utility to execute */
	for (util = samdb_utils; util->name != NULL; util++) {
		if (strcmp(util_name, util->name) == 0) {
			break;
		}
	}
	if (util->name == NULL) {
		printf("Command '%s' not found\n", util_name);
		samdb_usage(NULL);
		return (EXIT_FAILURE);
	}

	/* Process opts.  Uses the proc_opt function for each utility */
	util_args.util = util;
	util_opts = util->getopts();
	if (util_opts != NULL) {
		while ((opt = getopt(argc, argv, util_opts->optstr)) != -1) {
			switch (opt) {
			case ':':
				fprintf(stderr,
				    "Option -%c requires an operand\n", optopt);
				errflg++;
				break;
			case '?':
				fprintf(stderr,
				    "Unrecognized option: -%c\n", optopt);
				errflg++;
				break;
			default:
				if (util_opts->proc_opt(opt, optarg) < 0) {
					fprintf(stderr,
					    "Error processing %s option -%c\n",
					    util->name, opt);
					errflg++;
				}
			}
		}
	}
	if (errflg) {
		samdb_usage(&util_args);
		return (EXIT_FAILURE);
	}

	/* Initialize database configuration */
	if (strcmp("help", util_name) != 0) {
		db_conf = sam_db_conf_get(SAMDB_ACCESS_FILE, util_args.fsname);
		if (db_conf == NULL) {
			fprintf(stderr, "No database configured for %s\n",
			    util_args.fsname);
			return (EXIT_FAILURE);
		}
		util_args.con = sam_db_context_conf_new(db_conf);
		sam_db_conf_free(db_conf);
	}

	/* Populate remaining arguments and execute utility */
	util_args.argc = argc - optind;
	util_args.argv = util_args.argc <= 0 ? NULL : &argv[optind];

	if (util->exec(&util_args) < 0) {
		fprintf(stderr, "Error occured executing %s\n", util->name);
		return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}

/*
 * Print the usage for the utility described by args.
 *  If args is NULL a general usage of samdb is printed.
 *
 *  args->util contains the utility to print usage about
 */
int
samdb_usage(samdb_args_t *args) {
	samdb_util_t *util;
	util = (args == NULL) ? NULL : args->util;

	/* Process help arguments if exist */
	if (args != NULL && strcmp("help", util->name) == 0 &&
	    args->argc > 0) {
		for (util = samdb_utils; util->name != NULL; util++) {
			if (strcmp(util->name, *args->argv) == 0) {
				print_usage(util);
				break;
			}
		}
		/* If we couldn't find utility print general usage */
		if (util->name == NULL) {
			samdb_usage(NULL);
		}
	} else if (util == NULL || strcmp("help", util->name) == 0) {
		/* Print general help message */
		fprintf(stderr, "Usage: samdb [command] fsname ...\n");
		fprintf(stderr, "Available commands:\n");
		for (util = samdb_utils; util->name != NULL; util++) {
			fprintf(stderr, "\t%s\t%s\n",
			    util->name, util->desc);
		}
		/* If verbose usage print usage for every utility */
		if (verbose_usage) {
			for (util = samdb_utils; util->name != NULL; util++) {
				fprintf(stderr, "\n");
				print_usage(util);
			}
		}
	} else {
		print_usage(util);
	}

	return (0);
}

samdb_opts_t *
samdb_usage_getopts(void) {
	static opt_desc_t opt_desc[] = {
		{"v", "Print usage for all commands"},
		{NULL}
	};
	static samdb_opts_t usage_opts = {
		"v",
		usage_proc_opt,
		opt_desc
	};

	return (&usage_opts);
}

static int
/*LINTED argument unused in function */
usage_proc_opt(char opt, char *arg) {
	switch (opt) {
	case 'v': verbose_usage = TRUE; break;
	}
	return (0);
}

/*
 * Print a usage message to standard out for the provided utility.
 * Uses the utility and option descriptions provided for each utility.
 */
static void
print_usage(samdb_util_t *util) {
	samdb_opts_t *opts;
	opts = util->getopts();
	fprintf(stderr, "Command: %s\n", util->name);
	fprintf(stderr, "Usage: samdb %s fsname %c%s\n", util->name,
	    opts == NULL || *opts->optstr == '\0' ? ' ' : '-',
	    opts == NULL ? "" : opts->optstr);

	if (opts != NULL) {
		struct opt_desc *od;
		for (od = opts->opt_desc; od->opt != NULL; od++) {
			fprintf(stderr, "\t-%s\t%s\n",
			    od->opt, od->desc);
		}
	}
}

/* Dummy LoadFS to link with libfscmd.a */
/*LINTED argument unused in function */
void LoadFS(void *a) {}
