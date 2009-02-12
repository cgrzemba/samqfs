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

#pragma ident   "$Revision: 1.4 $"

#include "cmdparse.h"


/*
 * Subcommand function forward declarations.
 */
int add_features_cmd(int argc, char *argv[], cmdOptions_t *, void *callData);
int eq_add_cmd(int argc, char *argv[], cmdOptions_t *, void *callData);
int eq_release_cmd(int argc, char *argv[], cmdOptions_t *, void *callData);
int eq_remove_cmd(int argc, char *argv[], cmdOptions_t *, void *callData);
int eq_alloc_cmd(int argc, char *argv[], cmdOptions_t *, void *callData);
int eq_noalloc_cmd(int argc, char *argv[], cmdOptions_t *, void *callData);
int servicetag_cmd(int argc, char *argv[], cmdOptions_t *, void *callData);

/*
 * Utility routine forward declarations.
 */
boolean_t ask(char *msg, char def);

extern char version_string[];

/*
 * Command names that are supported by this command.
 */
#define	SAMADM	0
#define	CMD_MAX	1

extern char *cmd_names[];
extern int cmd_index;		/* Command index from above tables */
extern char *cmd_name;		/* Command name pointer */
extern char *subcmd_name;	/* Pointer to subcommand name */
