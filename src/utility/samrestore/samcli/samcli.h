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

#ifndef _SAMCLI_SAMCLI_H
#define	_SAMCLI_SAMCLI_H

#pragma ident	"$Revision: 1.14 $"

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/restore.h"
#include "pub/mgmt/file_util.h"
#include "pub/mgmt/stage.h"

/* Command list */

typedef enum v_command {
	v_null,
	v_setcsd,
	v_getcsd,
	v_listdumps,
	v_readdumps,
	v_showdumps,
	v_restore,
	v_takedump,
	v_stagefiles,
	v_listdir,
	v_listver,
	v_getfilestatus,
	v_getfiledetails,
	v_getverdetails,
	v_getdumpstatus,
	v_decompressdump,
	v_search,
} v_command_t;

/* Parser entry structure */
typedef char *strptr;

typedef struct arg {
	int (*routine)(char *);	/* Routine to call to parse value */
	strptr name[3];		/* Possible switch names */
	char *desc;		/* Help text message */
} arg_t;

/* Main parser entry point */
int aparse(int argc, char *argv[], arg_t *argt[]);

/* Results of parsing */
extern arg_t *cli_args[];	/* Pointer to our parsing arg tree */
extern sqm_lst_t *pathlist;
extern sqm_lst_t *destlist;
extern sqm_lst_t *csdlist;
extern sqm_lst_t *filstructlist;
extern sqm_lst_t *dmplist;
extern sqm_lst_t *restlist;
extern sqm_lst_t *cpylist;		/* Integer list */
extern sqm_lst_t *entrylist;	/* Integer list */
extern v_command_t command;
extern int no_action;

#endif /* _SAMCLI_SAMCLI_H */
