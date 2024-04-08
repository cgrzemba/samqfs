/*
 * previewcmd.c - reads preview.cmd file
 *
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

#pragma ident "$Revision: 1.18 $"

/* ANSI C headers. */
#include <errno.h>
#include <stdlib.h>
#include <values.h>
#include <syslog.h>
#include <string.h>

/* SAM-FS headers. */
#include "aml/types.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/preview.h"
#include "aml/parsecmd.h"
#include "aml/errorlog.h"

/* Local headers. */
#include "amld.h"

/* Private data. */
static prv_fs_ent_t prv_fs_default = {
	0,
	"default",
	PRV_LWM_FACTOR_DEFAULT,
	PRV_LHWM_FACTOR_DEFAULT,
	PRV_HLWM_FACTOR_DEFAULT,
	PRV_HWM_FACTOR_DEFAULT
};
static prv_fs_ent_t *prv_fs_table;

static uname_t fsName;			/* placeholder for fs name */

static float floatRange[] = {-MAXFLOAT + 0.1, MAXFLOAT};	/* any float */
static int fsPostFunc(cmd_ent_t *);

static cmd_ent_t cmdTable[] = {
	/* key	 var	  parse_type    valid	   post_func */
	{"fs", &fsName, PARSE_STRING, NULL, fsPostFunc},
	{"lwm_priority", &prv_fs_default.prv_fswm_factor[FS_LWM],
	PARSE_FLOAT, floatRange, NULL},
	{"lhwm_priority", &prv_fs_default.prv_fswm_factor[FS_LHWM],
	PARSE_FLOAT, floatRange, NULL},
	{"hlwm_priority", &prv_fs_default.prv_fswm_factor[FS_HLWM],
	PARSE_FLOAT, floatRange, NULL},
	{"hwm_priority", &prv_fs_default.prv_fswm_factor[FS_HWM],
	PARSE_FLOAT, floatRange, NULL},
	{"vsn_priority", NULL, PARSE_FLOAT, floatRange, NULL},
	{"age_priority", NULL, PARSE_FLOAT, floatRange, NULL},
	{NULL, NULL, 0, NULL, NULL}
};

enum cmdTableIndex {
	FS = 0,
	LWM_PRIORITY,
	LHWM_PRIORITY,
	HLWM_PRIORITY,
	HWM_PRIORITY,
	VSN_PRIORITY,
	AGE_PRIORITY,
	CMDTABLEINDEX_MAX
};


void
ReadPreviewCmd(
	preview_tbl_t *preview,
	prv_fs_ent_t *preview_fs_table)
{
	void	*vph;
	char	**fsnameValid;
	int		i, j;

	/* Let fsPostFunc know the location of fs table */
	prv_fs_table = preview_fs_table;

	/* Initialize global priorities */
	cmdTable[VSN_PRIORITY].var = &preview->prv_vsn_factor;
	cmdTable[AGE_PRIORITY].var = &preview->prv_age_factor;
	preview->prv_vsn_factor = PRV_VSN_FACTOR_DEFAULT;
	preview->prv_age_factor = PRV_AGE_FACTOR_DEFAULT;

	/* Initialize preview fs table and names table */
	fsnameValid =
	    (char **)malloc_wait(sizeof (char *) * (preview->fs_count + 1),
	    2, 0);
	for (i = 0; i < preview->fs_count; i++) {
		fsnameValid[i] = preview_fs_table[i].prv_fsname;
		for (j = 0; j < FS_WMSTATE_MAX; j++) {
			preview_fs_table[i].prv_fswm_factor[j] = -MAXFLOAT;
		}
	}
	fsnameValid[i] = NULL;
	cmdTable[FS].valid = fsnameValid;

	vph = parse_init(PREVIEW_CMD, SAM_AMLD, TO_SYS, NULL);
	if (vph != NULL) {
		if (parse_cmd_file(vph, cmdTable) != 0) {
			sam_syslog(LOG_ERR,
			    "Failed to parse %s\n", PREVIEW_CMD);
		}
		parse_fini(vph);
	} else {
		/* Don't complain if command file is missing */
		if (errno != ENOENT) {
			sam_syslog(LOG_ERR,
			    "Failed to initialize parsing of %s: %m\n",
			    PREVIEW_CMD);
		}
	}

	/*
	 * For all filesystems which were not set explicetely, set each
	 * factor to default.
	 */
	for (i = 0; i < preview->fs_count; i++) {
		for (j = 0; j < FS_WMSTATE_MAX; j++) {
			if (preview_fs_table[i].prv_fswm_factor[j] ==
			    -MAXFLOAT) {
				preview_fs_table[i].prv_fswm_factor[j] =
				    prv_fs_default.prv_fswm_factor[j];
			}
		}
	}

	free(fsnameValid);
}


static int
fsPostFunc(
	cmd_ent_t *cmd)
{
	prv_fs_ent_t *fs_ent;

	/* Switch priority entries in the table for corresponding fs */
	for (fs_ent = &prv_fs_table[0]; fs_ent != NULL; fs_ent++) {
		if (strcmp(fs_ent->prv_fsname, cmd->var) == 0) {
			cmdTable[LWM_PRIORITY].var =
			    &fs_ent->prv_fswm_factor[FS_LWM];
			cmdTable[LHWM_PRIORITY].var =
			    &fs_ent->prv_fswm_factor[FS_LHWM];
			cmdTable[HLWM_PRIORITY].var =
			    &fs_ent->prv_fswm_factor[FS_HLWM];
			cmdTable[HWM_PRIORITY].var =
			    &fs_ent->prv_fswm_factor[FS_HWM];
			break;
		}
	}
	return (0);
}
