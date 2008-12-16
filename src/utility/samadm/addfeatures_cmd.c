/*
 * samadm add-features command
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

#include "samadm.h"
#include "sam/lib.h"
#include "sam/syscall.h"

#pragma ident "$Revision: 1.2 $"


/*
 * Forward definitions.
 */
static void change_features_error();
static int change_features_v2a(char *fs);


int
add_features_cmd(
	int		argc,		/* Operand arg count */
	char		*argv[],	/* Operand arg vector pointer */
	cmdOptions_t	*options,	/* Pointer to parsed option flags */
	void		*callData)	/* Void data (unused) from main */
{
	cmdOptions_t *opt;
	int i, r = 0;
	sam_change_features_arg_t arg;

	for (opt = options; opt->optval; opt++) {
		switch (options->optval) {
		case 'a':
			/*
			 * -a: Add v5.0 feature set.
			 */
			printf("%s: %s: Warning: adding -a feature set "
			    "yields a\n", cmd_name, subcmd_name);
			printf("\tfile system that cannot be mounted on\n");
			printf("\ta previous version of SAM-QFS, and is not "
			    "reversible.\n");
			if (ask("Type yes to continue? ", 'n') == B_FALSE) {
				return (1);
			}
			for (i = 0; i < argc; i++) {
				char *fs = argv[i];

				if (change_features_v2a(fs)) {
					change_features_error(fs);
					r++;
				} else {
					printf("%s: %s: SAM-QFS v5.0 features "
					    "added successfully to %s.\n",
					    cmd_name, subcmd_name, fs);
				}
			}
			break;
		}
	}
	return (r);
}


/*
 * Do SC_change_features syscall.
 */
static int		/* Nonzero if error */
change_features_v2a(
	char *fs)			/* File system family name */
{
	sam_change_features_arg_t arg;	/* Syscall arg */

	strncpy(arg.fs_name, fs, sizeof (arg.fs_name) - 1);
	arg.command = SAM_CHANGE_FEATURES_ADD_V2A;
	return (sam_syscall(SC_change_features, (void *) &arg, sizeof (arg)));
}


/*
 * Process SC_change_features syscall errors.
 */
static void
change_features_error(
	char	*fs)	/* File system family name */
{
	switch (errno) {

	case EEXIST:
		fprintf(stderr, "%s: %s: cannot add v5.0 features to %s: "
		    "File system already has v5.0 features.\n",
		    cmd_name, subcmd_name, fs);
		break;

	case ENOENT:
		fprintf(stderr, "%s: %s: cannot add v5.0 features to %s: "
		    "File system not found.\n", cmd_name, subcmd_name, fs);
		break;

	case EACCES:
		fprintf(stderr, "%s: %s: cannot add v5.0 features to %s: "
		    "Permission denied.\n", cmd_name, subcmd_name, fs);
		break;

	case EXDEV:
		fprintf(stderr, "%s: %s: cannot add v5.0 features to %s: "
		    "File system not mounted.\n", cmd_name, subcmd_name, fs);
		break;

	case ENOTTY:
		fprintf(stderr, "%s: %s: cannot add v5.0 features to %s: "
		    "Must be done from server.\n", cmd_name, subcmd_name, fs);
		break;

	case ENXIO:
		fprintf(stderr, "%s: %s: cannot add v5.0 features to %s: "
		    "Must be v2 file system.\n", cmd_name, subcmd_name, fs);
		break;

	default:
		fprintf(stderr, "%s: %s: cannot add v5.0 features to %s: "
		    "%s.\n", cmd_name, subcmd_name, fs, strerror(errno));
		break;
	}
}
