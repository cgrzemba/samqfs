/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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
#pragma ident   "$Revision: 1.19 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */
/*
 * cfg_stager.c
 * Contains functions that interact with the parsers to read, write and
 * verify stager.cmd files and helper functions to search through the
 * stager_cfg_t struct.
 */

#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

#include "sam/sam_trace.h"
#include "aml/stager.h"
#include "mgmt/config/stager.h"
#include "pub/mgmt/stage.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/error.h"
#include "pub/devstat.h"
#include "pub/mgmt/device.h"
#include "mgmt/util.h"

/* full path of the stager.cmd */
static char *stager_file = STAGE_CFG;

/*
 * read the stager.cmd file.
 */
int
read_stager_cfg(stager_cfg_t **cfg)	/* malloced return value */
{

	sqm_lst_t *l;
	int err;

	Trace(TR_OPRMSG, "read stager.cmd");
	if (ISNULL(cfg)) {
		Trace(TR_OPRMSG, "read stager.cmd failed: %s", samerrmsg);
		return (-1);
	}

	if (get_all_libraries(NULL, &l) == -1) {
		Trace(TR_OPRMSG, "read stager.cmd failed: %s", samerrmsg);
		return (-1);
	}

	err = parse_stager_cmd(stager_file, l, cfg);

	free_list_of_libraries(l);
	Trace(TR_OPRMSG, "read stager.cmd");
	return (err);
}


/*
 * verify that the stager config is valid.
 */
int
verify_stager_cfg(const stager_cfg_t *cfg)	/* cfg to verify */
{

	node_t *node;

	Trace(TR_OPRMSG, "verify stager cfg entry");
	if (ISNULL(cfg)) {
		Trace(TR_OPRMSG, "verify stager cfg failed: %s", samerrmsg);
		return (-1);
	}

	/* check basics */
	if (cfg->max_retries != int_reset &&
	    cfg->change_flag & ST_max_retries &&
	    check_maxretries(cfg) != 0) {

		Trace(TR_OPRMSG, "verify stager cfg failed: %s", samerrmsg);
		return (-1);
	}

	if (cfg->max_active != int_reset &&
	    cfg->change_flag & ST_max_active &&
	    check_maxactive(cfg) != 0) {

		Trace(TR_OPRMSG, "verify stager cfg failed: %s", samerrmsg);
		return (-1);
	}
	if (*cfg->stage_log != char_array_reset &&
	    cfg->change_flag & ST_stage_log &&
	    check_logfile(cfg->stage_log) != 0) {

		Trace(TR_OPRMSG, "verify stager cfg failed: %s", samerrmsg);
		return (-1);
	}

	/* check drives */
	if (cfg->stage_drive_list != NULL) {
		sqm_lst_t *libs;

		if (get_all_libraries(NULL, &libs) == -1) {
			Trace(TR_OPRMSG, "verify stager cfg failed: %s",
			    samerrmsg);
			return (-1);
		}

		for (node = cfg->stage_drive_list->head;
		    node != NULL; node = node->next) {
			if (check_stage_drives((drive_directive_t *)node->data,
			    libs) != 0) {

				free_list_of_libraries(libs);
				Trace(TR_OPRMSG,
				    "verify stager cfg failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
		free_list_of_libraries(libs);
	}

	/* check buffers */
	if (cfg->stage_buf_list != NULL) {
		for (node = cfg->stage_buf_list->head;
		    node != NULL; node = node->next) {
			if (check_bufsize(
			    (buffer_directive_t *)node->data) != 0) {
				Trace(TR_OPRMSG,
				    "verify stager cfg failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	Trace(TR_OPRMSG, "stager cfg is valid");
	return (0);
}


/*
 * write the stager configuration to the default location.
 */
int
write_stager_cfg(
const stager_cfg_t *cfg, /* cfg to write */
const boolean_t force)	/* if true cfg will be written even if backup fails */
{

	Trace(TR_OPRMSG, "write stager cfg");

	if (ISNULL(cfg)) {
		Trace(TR_OPRMSG, "write stager cfg failed: %s", samerrmsg);
		return (-1);
	}

	if (verify_stager_cfg(cfg) != 0) {
		/* Leave samerrno as set */
		Trace(TR_OPRMSG, "write stager cfg failed: %s", samerrmsg);
		return (-1);
	}

	/* possibly backup the cfg (see backup_cfg for details) */
	if (backup_cfg(stager_file) != 0 && !force) {
		/* leave samerrno as set */
		Trace(TR_OPRMSG, "write stager cfg failed: %s", samerrmsg);
		return (-1);
	}

	if (write_stager_cmd(stager_file, cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_OPRMSG, "write stager cfg failed: %s", samerrmsg);
		return (-1);
	} else {
		Trace(TR_FILES, "wrote stager file %s", stager_file);
	}

	/* always backup the new file. */
	backup_cfg(stager_file);

	Trace(TR_OPRMSG, "wrote stager cfg");
	return (0);
}


/*
 * write the stager config to an alternate location and skip verification.
 */
int
dump_stager_cfg(
const stager_cfg_t *cfg,	/* cfg to dump */
const char *location)		/* location at which to write the file */
{

	int err = 0;

	Trace(TR_FILES, "dump stager cfg to %s",
	    location ? location : "NULL");

	if (ISNULL(cfg, location)) {
		Trace(TR_OPRMSG, "dump stager cfg failed: %s", samerrmsg);
		return (-1);
	}
	if (strcmp(location, STAGE_CFG) == 0) {
		samerrno = SE_INVALID_DUMP_LOCATION;

		/* Cannot dump the file to %s */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_DUMP_LOCATION), location);

		Trace(TR_OPRMSG, "dump stager cfg failed: %s", samerrmsg);
		return (-1);
	}

	err = write_stager_cmd(location, cfg);

	Trace(TR_OPRMSG, "dumped stager cfg");

	return (err);
}
