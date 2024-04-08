/*
 * samadm add-eq command
 * samadm remove-eq command
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

#include "samadm.h"
#define DEC_INIT
#include "sam/syscall.h"
#include <sam/mount.h>
#include "sam/lib.h"

#pragma ident "$Revision: 1.4 $"


/*
 * Check if eq is a mounted disk partition device.
 */
static char *
CheckFSPartByEq(
	char *eqarg,			/* Equipment ordinal to check */
	struct sam_fs_info *fi)		/* File system info for above eq */
{
	int eq;
	char *p;
	char *retval = NULL;
	static char ret[80] = "";

	errno = 0;
	eq = strtol(eqarg, &p, 0);
	if (errno || eq == 0) {
		snprintf(ret, sizeof (ret), "Illegal equipment number %d.", eq);
	} else {
		if (GetFsInfoByPartEq(eq, fi) < 0) {
			snprintf(ret, sizeof (ret), "Disk eq %d not"
			    " in a family set.", eq);
		} else {
			if ((fi->fi_status & FS_MOUNTED) == 0)  {
				snprintf(ret, sizeof (ret), "Filesystem %s"
				    " is not mounted.", fi->fi_name);
			}
		}
	}
	return (ret[0] ? ret : NULL);
}


/*
 * Common code for add/remove/alloc/noalloc.
 */
static int
cmd_eq(
	int	command,	/* Command to send to SetFsPartCmd */
	char	*c,		/* Command string for errors */
	char	*new_eq)	/* Equipment to be added/removed */
{
	char *errstr;
	struct sam_fs_info fi;

	errstr = CheckFSPartByEq(new_eq, &fi);
	if (errstr) {
		fprintf(stderr, "%s: %s: cannot %s eq %s - %s\n", cmd_name,
		    subcmd_name, c, new_eq, errstr);
		return (1);
	}
	if (SetFsPartCmd(fi.fi_name, new_eq, command) != 0) {
		fprintf(stderr, "%s: %s: cannot %s eq %s - ", cmd_name,
		    subcmd_name, c, new_eq);
		perror("");
		return (1);
	}
	printf("%s: %s: eq %s %s started successfully.\n"
	    "Monitor 'samcmd m' display for completion and "
	    "'/var/adm/messages' for errors.\n", cmd_name,
	    subcmd_name, c, new_eq);
	return (0);
}


/*
 * eq_add command.
 */
int
eq_add_cmd(
	int		argc,		/* Operand arg count */
	char		*argv[],	/* Operand arg vector pointer */
	cmdOptions_t	*options,	/* Pointer to parsed option flags */
	void		*callData)	/* Void data (unused) from main */
{
	char *eqp = argv[0];

	return (cmd_eq(DK_CMD_add, "add", eqp));
}


/*
 * eq_remove command.
 */
int
eq_remove_cmd(int argc, char *argv[], cmdOptions_t *options, void *callData)
{
	char *eqp = argv[0];

	return (cmd_eq(DK_CMD_remove, "remove", eqp));
}


/*
 * eq_release command.
 */
int
eq_release_cmd(int argc, char *argv[], cmdOptions_t *options, void *callData)
{
	char *eqp = argv[0];

	return (cmd_eq(DK_CMD_release, "release", eqp));
}


/*
 * eq_alloc command.
 */
int
eq_alloc_cmd(
	int		argc,		/* Operand arg count */
	char		*argv[],	/* Operand arg vector pointer */
	cmdOptions_t	*options,	/* Pointer to parsed option flags */
	void		*callData)	/* Void data (unused) from main */
{
	char *eqp = argv[0];

	return (cmd_eq(DK_CMD_alloc, "alloc", eqp));
}



/*
 * eq_noalloc command.
 */
int
eq_noalloc_cmd(
	int		argc,		/* Operand arg count */
	char		*argv[],	/* Operand arg vector pointer */
	cmdOptions_t	*options,	/* Pointer to parsed option flags */
	void		*callData)	/* Void data (unused) from main */
{
	char *eqp = argv[0];

	return (cmd_eq(DK_CMD_noalloc, "noalloc", eqp));
}
