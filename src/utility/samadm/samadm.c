/*
 * samadm - main SAM admin command
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <libintl.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <strings.h>

#include "samadm.h"

#pragma ident   "$Revision: 1.2 $"


char version_string[] = "1.0";

/*
 * Command names.
 */
char *cmd_names[CMD_MAX] = {
	"samadm"
};

/*
 * samadm subcommand table.
 */
subCommandProps_t samadm_subcmds[] = {
	/* { option_name, func_handler, options, reqd_options, excl_options, */
	/*  operand_control_mask, long help description }, */
	{ "servicetag", servicetag_cmd, NULL, NULL, NULL,
	    OPERAND_MANDATORY_SINGLE, "Add/delete servicetags"},
	{ "eq-add", eq_add_cmd, NULL, NULL, NULL,
	    OPERAND_MANDATORY_SINGLE, "Add equipment ordinal to QFS"},
	{ "eq-remove", eq_remove_cmd, "r", NULL, NULL,
	    OPERAND_MANDATORY_SINGLE, "Remove equipment ordinal from QFS"},
	{ "eq-alloc", eq_alloc_cmd, NULL, NULL, NULL,
	    OPERAND_MANDATORY_SINGLE, "Set equipment status to alloc"},
	{ "eq-noalloc", eq_noalloc_cmd, NULL, NULL, NULL,
	    OPERAND_MANDATORY_SINGLE, "Set equipment status to noalloc"},
	{ "add-features", add_features_cmd, "a", "a", NULL,
	    OPERAND_MANDATORY_MULTIPLE, "Add features to a QFS file system"},
	{ NULL, 0, NULL, NULL, NULL, 0, NULL}
};

/*
 * samadm option table.
 *	XXX - cmdparse will have to be enhanced soon to allow different
 *	command option help listings for different subcommands.
 */
optionTbl_t samadm_opts[] = {
	{ "releaseeq", no_arg, 'r', "use SAM to release files" },
	{ "adda", no_arg, 'a', "add feature set a" },
	{ NULL, 0, 0, NULL }
};

/*
 * Syntax tables for cmdParse.
 */
synTables_t syn_table[CMD_MAX] = {
	{ version_string, samadm_opts, samadm_subcmds }		/* samadm */
};

int cmd_index;		/* Command index from above tables */
char *cmd_name;		/* Command name pointer */
char *subcmd_name;	/* Pointer to subcommand name */


/*
 * Return basename of command name.
 */
static char *
getExecBasename(
	char	*execFullname)	/* Incoming full invoking path */
{
	char	*lastSlash, *execBasename;

	/* guard against trailing '/'s at end of command */
	for (;;) {
		if ((lastSlash = strrchr(execFullname, '/')) == NULL) {
			execBasename = execFullname;
		} else {
			execBasename = lastSlash + 1;
			if (*execBasename == '\0') {
				*lastSlash = '\0';
				continue;
			}
		}
		break;
	}
	return (execBasename);
}


int
main(
	int	argc,		/* Options arg count */
	char	*argv[])	/* Options arg vector pointer */
{
	int 	ret;
	int	func_ret = 0;

	/*
	 * Get basename of invoking command name.
	 */
	cmd_name = getExecBasename(argv[0]);

	/*
	 * Validate command.
	 */
	for (cmd_index = 0; cmd_index < CMD_MAX; cmd_index++) {
		if (strcmp(cmd_name, cmd_names[cmd_index]) == 0)
			break;
	}
	if (cmd_index == CMD_MAX) {
		fprintf(stderr, "samadm invoked as illegal command name %s\n",
		    cmd_name);
		exit(2);
	}
	subcmd_name = argv[1];

	/*
	 * Call cmdParse and call subcommand functions.
	 */
	ret = cmdParse(argc, argv, syn_table[cmd_index], NULL, &func_ret);
	exit(ret ? ret : func_ret);
}
