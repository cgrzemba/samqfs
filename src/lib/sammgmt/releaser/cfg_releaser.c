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
#pragma ident   "$Revision: 1.18 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * releaser.c
 * Contains functions that interact with the parsers to read, write and
 * verify the releaser.cmd file and functions to search throught the
 * releaser_cfg_t struct.
 */
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "sam/sam_trace.h"
#include "pub/mgmt/release.h"
#include "mgmt/config/releaser.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/error.h"
#include "pub/devstat.h"
#include "mgmt/util.h"
#include "pub/mgmt/filesystem.h"

/* full path to the releaser.cmd file */
static char *releaser_file = RELEASE_CFG;

/*
 * read the releaser.cmd file and populate the releaser_cfg_t struct.
 */
int
read_releaser_cfg(releaser_cfg_t **cfg) /* cfg must be freed by caller */
{
	sqm_lst_t *fs_list;
	int ret_val;

	Trace(TR_OPRMSG, "reading releaser cfg");

	if (get_all_fs(NULL, &fs_list) == -1) {
		Trace(TR_OPRMSG, "reading releaser cfg failed: %s", samerrmsg);
		return (-1);
	}

	ret_val = parse_releaser_cmd(releaser_file, fs_list, cfg);

	free_list_of_fs(fs_list);

	Trace(TR_OPRMSG, "read releaser cfg");
	return (ret_val);
}


/*
 * get the directive for the named file system.
 */
int
cfg_get_rl_fs_directive(
const releaser_cfg_t *cfg,	/* cfg to search */
const char *fs_name,		/* name of fs to get */
rl_fs_directive_t **fs_release)	/* return val must be freed by caller */
{

	node_t *node;
	rl_fs_directive_t *rd;

	Trace(TR_OPRMSG, "getting releaser fs directive for %s", Str(fs_name));

	if (ISNULL(cfg, fs_name)) {
		Trace(TR_OPRMSG, "getting releaser fs directive failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (cfg->rl_fs_dir_list != NULL) {
	for (node = cfg->rl_fs_dir_list->head;
	    node != NULL; node = node->next) {

		rd = (rl_fs_directive_t *)node->data;
		if (strcmp(rd->fs, fs_name) == 0) {

			*fs_release = (rl_fs_directive_t *)mallocer(
			    sizeof (rl_fs_directive_t));

			if (*fs_release == NULL) {
				Trace(TR_OPRMSG,
				    "getting releaser fs directive failed: %s",
				    samerrmsg);

				return (-1);
			}

			memcpy(*fs_release, rd, sizeof (rl_fs_directive_t));
			Trace(TR_OPRMSG, "got releaser fs directive");
			return (0);
		}
	}
	}

	*fs_release = NULL;
	samerrno = SE_NOT_FOUND;

	/* %s not found */
	snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(SE_NOT_FOUND),
	    fs_name);

	Trace(TR_OPRMSG, "getting releaser fs directive failed: %s",
	    samerrmsg);
	return (-1);
}


/*
 * verify the releaser cfg is valid.
 */
int
verify_releaser_cfg(const releaser_cfg_t *cfg)	/* cfg to verify */
{
	node_t *node;
	sqm_lst_t *filesystems;
	rl_fs_directive_t *rl_fs_dir;

	Trace(TR_DEBUG, "verifying releaser cfg");

	if (ISNULL(cfg)) {
		Trace(TR_OPRMSG, "verifying releaser cfg failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (get_all_fs(NULL, &filesystems) == -1) {
		/*
		 * dont return let the checking try to pass
		 * it could still pass if there are only global
		 * directives
		 */
		filesystems = NULL;
	}

	if (cfg_get_rl_fs_directive(cfg, GLOBAL, &rl_fs_dir) == 0) {
		if (verify_rl_fs_directive(rl_fs_dir, filesystems) != 0) {
			/* leave samerrmsg as set */
			free_list_of_fs(filesystems);
			free(rl_fs_dir);
			Trace(TR_OPRMSG, "verifying releaser cfg failed: %s",
			    samerrmsg);

			return (-1);
		}
		free(rl_fs_dir);
	}

	for (node = cfg->rl_fs_dir_list->head; node != NULL;
	    node = node->next) {

		if (verify_rl_fs_directive((rl_fs_directive_t *)node->data,
		    filesystems) != 0) {

			free_list_of_fs(filesystems);

			/* leave samerrmsg as set */
			Trace(TR_OPRMSG, "verifying releaser cfg failed: %s",
			    samerrmsg);

			return (-1);
		}
	}

	free_list_of_fs(filesystems);
	Trace(TR_OPRMSG, "verified releaser cfg");
	return (0);
}


/*
 * write the releaser_cfg to the default location.
 */
int
write_releaser_cfg(
const releaser_cfg_t *cfg,	/* the config to write out */
const boolean_t force)		/* if true write it even if cant backup */
{

	Trace(TR_OPRMSG, "writing releaser cfg entry");
	if (ISNULL(cfg)) {
		Trace(TR_OPRMSG, "writing releaser cfg failed: %s", samerrmsg);
		return (-1);
	}

	if (verify_releaser_cfg(cfg) != 0) {
		/* Leave samerrno as set */
		Trace(TR_OPRMSG, "writing releaser cfg failed: %s", samerrmsg);
		return (-1);
	}

	/* possibly backup the cfg (see backup_cfg for details) */
	if (backup_cfg(releaser_file) != 0 && !force) {
		/* leave samerrno as set */
		Trace(TR_OPRMSG, "writing releaser cfg failed: %s", samerrmsg);
		return (-1);
	}


	if (write_releaser_cmd(releaser_file, cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_OPRMSG, "writing releaser cfg failed: %s", samerrmsg);
		return (-1);
	} else {
		Trace(TR_FILES, "wrote releaser.cmd file %s", releaser_file);
	}

	/* always backup the new file. */
	backup_cfg(releaser_file);

	Trace(TR_OPRMSG, "wrote releaser cfg");
	return (0);

}


/*
 * write the releaser.cmd file to an alternate location.
 */
int
dump_releaser_cfg(
const releaser_cfg_t *cfg,	/* The config to write out */
const char *location)		/* The location to write at */
{

	Trace(TR_FILES, "dumping releaser cfg to %s entry",
	    location ? location : "NULL");


	if (ISNULL(cfg, location)) {
		Trace(TR_OPRMSG, "dumping releaser cfg failed: %s", samerrmsg);
		return (-1);
	}
	if (strcmp(location, RELEASE_CFG) == 0) {
		samerrno = SE_INVALID_DUMP_LOCATION;

		/* Cannot dump the file to %s */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_DUMP_LOCATION), location);

		Trace(TR_OPRMSG, "dumping releaser cfg failed: %s", samerrmsg);
		return (-1);
	}

	if (write_releaser_cmd(location, cfg) != 0) {
		Trace(TR_OPRMSG, "dumping releaser cfg failed: %s", samerrmsg);
	}

	Trace(TR_OPRMSG, "dumped releaser cfg");
	return (0);
}


/*
 * free the releaser cfg.
 */
void
free_releaser_cfg(releaser_cfg_t *cfg)	/* cfg to free */
{

	Trace(TR_ALLOC, "freeing releaser cfg");
	if (cfg == NULL) {
		return;
	}


	free_list_of_rl_fs_directive(cfg->rl_fs_dir_list);
	free(cfg);
	cfg = NULL;

	Trace(TR_ALLOC, "freed releaser cfg");
}
