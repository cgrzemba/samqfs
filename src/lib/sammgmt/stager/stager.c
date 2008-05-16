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
/*
 *	stager.c -  APIs for stager.h.
 *	It calls functions of cfg_stager.c and process
 *	the detailed stager.cmd operation.
 */
#pragma	ident	"$Revision: 1.23 $"
#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "aml/stager.h"
#include "mgmt/util.h"
#include "pub/mgmt/error.h"
#include "mgmt/config/common.h"
#include "mgmt/config/stager.h"
#include "mgmt/config/archiver.h"
#include "pub/mgmt/stage.h"
#include "pub/devstat.h"
#include "pub/mgmt/device.h"
#include "sam/sam_trace.h"
static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */
uint32_t *TraceFlags;

#define	STAGER_DEFAULT_BUFFER_LOCK	B_FALSE
#define	STAGER_DEFAULT_BUFFER_CHANGE_FLAG	0
#define	STAGER_DEFAULT_DRIVE_CHANGE_FLAG	0
/*
 * These constants are defined at aml/stager.h
 * STAGER_DEFAULT_MC_BUFSIZE	4
 * STAGER_DEFAULT_MAX_ACTIVE	1000
 * STAGER_DEFAULT_MAX_RETRIES	3
 */
#define	STAGER_DEFAULT_CHANGE_FLAG	0
#define	STAGER_DEFAULT_LOG	""

static char *stager_file = STAGE_CFG;

/*
 *	ctx structure is used sed by RPC clients to specify which
 *	connection should be used.
 *	see sammgmt_rpc.h for details.
 *	Also it is used to locate configuration file to dump
 *	configuration file for write functions.
 */


/*
 *	API get_stager_cfg()
 *	get the stager configuration.
 */
int
get_stager_cfg(
ctx_t *ctx,			/* ARGSUSED */
stager_cfg_t **stager_config)	/* must be freed by caller */
{
	Trace(TR_MISC, "get stager configuration");
	if (ISNULL(stager_config)) {
		Trace(TR_ERR, "stager:get_stager_cfg: %s", samerrmsg);
		return (-1);
	}
	/*
	 *	read stager.cmd file to get all
	 *	stager.cmd information.
	 */
	if (read_stager_cfg(stager_config) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    stager_file, samerrmsg);
		return (-1);
	}
	Trace(TR_MISC, "get stager configuration success");
	return (0);
}


/*
 *	set_stager_cfg()
 *	configure the entire staging information.
 */
int
set_stager_cfg(
ctx_t *ctx,				/* ARGSUSED */
const stager_cfg_t *stager_config)	/* structure stager_cfg_t */
{

	Trace(TR_MISC, "set stager configuration");
	if (ISNULL(stager_config)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	/*
	 *	write stager.cmd with TRUE option
	 *	and it will force to write a new stager.cmd.
	 */
	if (write_stager_cfg(stager_config, B_TRUE) != 0) {
		Trace(TR_ERR, "write of %s failed with error: %s",
		    stager_file, samerrmsg);
		return (-1);
	}
	Trace(TR_FILES, "stager.cmd has been updated\n");
	if (init_config(NULL) != 0) {
		samerrno = SE_ACTIVATE_STAGER_CFG_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ACTIVATE_STAGER_CFG_FAILED));
		return (-1);
	}

	Trace(TR_MISC, "set stager configuration success");
	return (0);
}


/*
 *	get_drive_directive()
 *	get the drive directives for a library.
 */
int
get_drive_directive(
ctx_t *ctx,				/* ARGSUSED */
const uname_t lib_name,			/* library name (family set) */
drive_directive_t **stage_drive)	/* must be freed by caller */
{
	stager_cfg_t *stager;
	drive_directive_t *drive_directive;
	node_t *node_c;
	int match_flag = 0;

	Trace(TR_MISC, "get stager configuration's drive directive");
	*stage_drive = NULL;
	if (ISNULL(stage_drive)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	read stager.cmd file to get all
	 *	stager.cmd information.
	 */
	if (read_stager_cfg(&stager) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    stager_file, samerrmsg);
		return (-1);
	}
	/*
	 *	traverse stager->stage_drive_list and match
	 *	auto_lib name with given auto_lib name.
	 *	If no match is found, returns error.
	 */
	node_c = (stager->stage_drive_list)->head;
	while (node_c != NULL) {
		drive_directive = (drive_directive_t *)node_c->data;
		if (strcmp(drive_directive->auto_lib, lib_name) == 0) {
			*stage_drive = drive_directive;
			node_c ->data = NULL;
			match_flag = 1;
			break;
		}
		node_c = node_c->next;
	}
	free_stager_cfg(stager);
	if (match_flag == 0) {
		samerrno = SE_NO_DRIVE_DIRECTIVE_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NO_DRIVE_DIRECTIVE_FOUND),
		    lib_name);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	Trace(TR_MISC, "get stager configuration's drive directive success");
	return (0);
}


/*
 *	set_drive_directive()
 *	Functions to modify a drive's directive.
 */
int
set_drive_directive(
ctx_t *ctx,				/* ARGSUSED */
drive_directive_t *stage_drive)		/* drive_directive need to be */
					/* set in stager.cmd */
{
	stager_cfg_t *stager;
	drive_directive_t *drive_directive;
	node_t *node_c;
	int match_flag = 0;

	Trace(TR_MISC, "set stager configuration's drive directive");
	if (ISNULL(stage_drive)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	read stager.cmd file to get all
	 *	stager.cmd information.
	 */
	if (read_stager_cfg(&stager) != 0) {
		Trace(TR_ERR, "Read of %s failed with error %s",
		    stager_file, samerrmsg);
		return (-1);
	}
	/*
	 *	traverse stager->stage_drive_list and match
	 *	auto_lib name with given auto_lib name.
	 *	If a match is not found, add it to configuration
	 *	file; else modify it.
	 */
	node_c = (stager->stage_drive_list)->head;
	while (node_c != NULL) {
		drive_directive = (drive_directive_t *)node_c->data;
		if (strcmp(drive_directive->auto_lib,
		    stage_drive->auto_lib) == 0) {
			if (stage_drive->change_flag & DD_count) {
				if (stage_drive->count != int_reset) {
					drive_directive->count =
					    stage_drive->count;
				} else {
					drive_directive->count = int_reset;
				}
			}
			match_flag = 1;
			break;
		}
		node_c = node_c->next;
	}
	if (match_flag == 0) {
		if (dup_drive_directive(stage_drive, &drive_directive) != 0) {
			free_stager_cfg(stager);
			Trace(TR_ERR, "%s", samerrmsg);
			return (-1);
		}
		if (lst_append(stager->stage_drive_list,
		    drive_directive) != 0) {
			free_stager_cfg(stager);
			free((drive_directive_t *)drive_directive);
			Trace(TR_ERR, "%s", samerrmsg);
			return (-1);
		}
	}
	/*
	 *	write stager.cmd with TRUE option
	 *	and it will force to write a new stager.cmd.
	 */
	if (write_stager_cfg(stager, B_TRUE) != 0) {
		free_stager_cfg(stager);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	Trace(TR_FILES, "new drive_directive %s is modifed to stager.cmd",
	    stage_drive->auto_lib);
	free_stager_cfg(stager);
	if (init_config(NULL) != 0) {
		samerrno = SE_ACTIVATE_STAGER_CFG_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ACTIVATE_STAGER_CFG_FAILED));
		return (-1);
	}
	Trace(TR_MISC, "set stager configuration's drive directive success");
	return (0);
}


/*
 *	reset_drive_directive()
 *	Functions to remove a drive directive.
 */
int
reset_drive_directive(
ctx_t *ctx,			/* ARGSUSED */
drive_directive_t *stage_drive)	/* drive directive need to be reset */
{
	stager_cfg_t *stager;
	drive_directive_t *drive_directive;
	node_t *node_c;
	int match_flag = 0;

	Trace(TR_MISC, "reset stager's drive directive()");
	if (ISNULL(stage_drive)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	read stager.cmd file to get all
	 *	stager.cmd information.
	 */
	if (read_stager_cfg(&stager) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    stager_file, samerrmsg);
		return (-1);
	}
	/*
	 *	traverse stager->stage_drive_list and match
	 *	auto_lib name with given auto_lib name.
	 *	If a match is found, remove it from the list.
	 *	If no match is found, returns error.
	 */
	node_c = (stager->stage_drive_list)->head;
	while (node_c != NULL) {
		drive_directive = (drive_directive_t *)node_c->data;
		if (strcmp(drive_directive->auto_lib,
		    stage_drive->auto_lib) == 0) {
			if (lst_remove(stager->stage_drive_list, node_c) != 0) {
				goto error;
			}
			free(drive_directive);
			match_flag = 1;
			break;
		}
		node_c = node_c->next;
	}
	if (match_flag == 0) {
		samerrno = SE_NO_DRIVE_DIRECTIVE_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NO_DRIVE_DIRECTIVE_FOUND),
		    stage_drive->auto_lib);
		goto error;
	}
	/*
	 *	write stager.cmd with TRUE option
	 *	and it will force to write a new stager.cmd.
	 */
	if (write_stager_cfg(stager, B_TRUE) != 0) {
		goto error;
	}
	Trace(TR_FILES, "drive_directive %s is removed from stager.cmd\n",
	    stage_drive->auto_lib);
	free_stager_cfg(stager);
	if (init_config(NULL) != 0) {
		samerrno = SE_ACTIVATE_STAGER_CFG_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ACTIVATE_STAGER_CFG_FAILED));
		return (-1);
	}
	Trace(TR_MISC, "reset stager's drive directive() success");
	return (0);
error:
	free_stager_cfg(stager);
	Trace(TR_ERR, "%s", samerrmsg);
	return (-1);

}


/*
 *	get_buffer_directive()
 *	Functions to get a buffer's directive.
 */
int
get_buffer_directive(
ctx_t *ctx,				/* ARGSUSED */
const mtype_t media_type,		/* media type */
buffer_directive_t **stage_buffer)	/* must be freed by caller */
{
	stager_cfg_t *stager;
	buffer_directive_t *buffer_directive;
	node_t *node_c;
	int match_flag = 0;

	Trace(TR_MISC, "get stager's buffer directive");
	*stage_buffer = NULL;
	if (ISNULL(stage_buffer)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	read stager.cmd file to get all
	 *	stager.cmd information.
	 */
	if (read_stager_cfg(&stager) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    stager_file, samerrmsg);
		return (-1);
	}
	/*
	 *	traverse stager->stage_buf_list and match
	 *	media type with given media type.
	 *	If a match is found, return that structure.
	 *	If no match is found, returns error.
	 */
	node_c = (stager->stage_buf_list)->head;
	while (node_c != NULL) {
		buffer_directive = (buffer_directive_t *)node_c ->data;
		if (strcmp(buffer_directive->media_type, media_type) == 0) {
			*stage_buffer = buffer_directive;
			node_c ->data = NULL;
			match_flag = 1;
			break;
		}
		node_c = node_c->next;
	}
	free_stager_cfg(stager);
	if (match_flag == 0) {
		samerrno = SE_NO_BUFFER_DIRECTIVE_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NO_BUFFER_DIRECTIVE_FOUND),
		    media_type);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	Trace(TR_MISC, "get stager's buffer directive success");
	return (0);
}


/*
 *	set_buffer_directive()
 *	Functions to modify a buffer's directive in stager.cmd.
 */
int
set_buffer_directive(
ctx_t *ctx,				/* ARGSUSED */
buffer_directive_t *stage_buffer)	/* buffer directive need to be set */
{
	stager_cfg_t *stager;
	buffer_directive_t *buffer_directive;
	node_t *node_c;
	int match_flag = 0;

	Trace(TR_MISC, "set stager's buffer directive");
	if (ISNULL(stage_buffer)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	read stager.cmd file to get all
	 *	stager.cmd information.
	 */
	if (read_stager_cfg(&stager) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    stager_file, samerrmsg);
		return (-1);
	}
	/*
	 *	traverse stager->stage_buf_list and match
	 *	media type with given media type.
	 *	If a match is found, update that structure.
	 *	If no match is found, returns error.
	 */
	node_c = (stager->stage_buf_list)->head;
	while (node_c != NULL) {
		buffer_directive = (buffer_directive_t *)node_c ->data;
		if (strcmp(buffer_directive->media_type,
		    stage_buffer->media_type) == 0) {
			if (stage_buffer->change_flag & BD_size) {
				if (stage_buffer->size != fsize_reset) {
					buffer_directive->size =
					    stage_buffer->size;
				} else {
					buffer_directive->size = fsize_reset;
				}
			}
			if (stage_buffer->change_flag & BD_lock) {
				buffer_directive->lock = stage_buffer->lock;
			}
			match_flag = 1;
			break;
		}
		node_c = node_c->next;
	}
	if (match_flag == 0) {
		if (dup_buffer_directive(stage_buffer,
		    &buffer_directive) != 0) {
			free_stager_cfg(stager);
			Trace(TR_ERR, "%s", samerrmsg);
			return (-1);
		}
		if (lst_append(stager->stage_buf_list, buffer_directive) != 0) {
			free((buffer_directive_t *)buffer_directive);
			goto error;
		}
	}
	/*
	 *	write stager.cmd with TRUE option
	 *	and it will force to write a new stager.cmd.
	 */
	if (write_stager_cfg(stager, B_TRUE) != 0) {
		goto error;
	}
	Trace(TR_FILES, "new buffer_directive %s is modifed to stager.cmd\n",
	    stage_buffer->media_type);
	free_stager_cfg(stager);
	if (init_config(NULL) != 0) {
		samerrno = SE_ACTIVATE_STAGER_CFG_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ACTIVATE_STAGER_CFG_FAILED));
		return (-1);
	}
	Trace(TR_MISC, "set stager's buffer directive success");
	return (0);
error:
	free_stager_cfg(stager);
	Trace(TR_ERR, "%s", samerrmsg);
	return (-1);
}


/*
 *	reset_buffer_directive()
 *	Functions to remove a buffer directive.
 */
int
reset_buffer_directive(
ctx_t *ctx,				/* ARGSUSED */
buffer_directive_t *stage_buffer)	/* buffer directive need to be reset */
{
	stager_cfg_t *stager;
	buffer_directive_t *buffer_directive;
	node_t *node_c;
	int match_flag = 0;

	Trace(TR_MISC, "reset stager's buffer directive");
	if (ISNULL(stage_buffer)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	read stager.cmd file to get all
	 *	stager.cmd information.
	 */
	if (read_stager_cfg(&stager) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    stager_file, samerrmsg);
		return (-1);
	}
	/*
	 *	traverse stager->stage_buf_list and match
	 *	media type with given media type.
	 *	If a match is found, remove that structure.
	 *	If no match is found, returns error.
	 */
	node_c = (stager->stage_buf_list)->head;
	while (node_c != NULL) {
		buffer_directive = (buffer_directive_t *)node_c ->data;
		if (strcmp(buffer_directive->media_type,
		    stage_buffer->media_type) == 0) {
			if (lst_remove(stager->stage_buf_list, node_c) != 0) {
				goto error;
			}
			free(buffer_directive);
			match_flag = 1;
			break;
		}
		node_c = node_c->next;
	}
	if (match_flag == 0) {
		samerrno = SE_NO_BUFFER_DIRECTIVE_FOUND;
		snprintf(samerrmsg,  MAX_MSG_LEN,
		    GetCustMsg(SE_NO_BUFFER_DIRECTIVE_FOUND),
		    stage_buffer->media_type);
		goto error;
	}
	/*
	 *	write stager.cmd with TRUE option
	 *	and it will force to write a new stager.cmd.
	 */
	if (write_stager_cfg(stager, B_TRUE) != 0) {
		goto error;
	}

	Trace(TR_FILES, "buffer_directive %s is removed from stager.cmd\n",
	    stage_buffer->media_type);

	free_stager_cfg(stager);
	if (init_config(NULL) != 0) {
		samerrno = SE_ACTIVATE_STAGER_CFG_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ACTIVATE_STAGER_CFG_FAILED));
		return (-1);
	}
	Trace(TR_MISC, "reset stager's buffer directive success");
	return (0);
error:
	free_stager_cfg(stager);
	Trace(TR_ERR, "%s", samerrmsg);
	return (-1);
}


/*
 *	get_default_staging_drive_directive()
 *	get stager.cmd's default drive directive value.
 */
int
get_default_staging_drive_directive(
ctx_t *ctx,				/* ARGSUSED */
uname_t lib_name,			/* library name */
drive_directive_t **stage_drive)	/* must be freed by caller */
{
	int i;
	library_t *print_lib;

	Trace(TR_MISC, "get stager's default drive directive");
	*stage_drive = NULL;
	if (ISNULL(stage_drive)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	i = get_library_by_family_set(ctx, lib_name, &print_lib);
	if (i != 0) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	allocate memory for *stager_drive and this
	 *	memory must be released in the calling function.
	 */
	*stage_drive = (drive_directive_t *)
	    mallocer(sizeof (drive_directive_t));
	if (*stage_drive == NULL) {
		free_library(print_lib);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	memset(*stage_drive, 0, sizeof (drive_directive_t));
	strlcpy((*stage_drive)->auto_lib, lib_name,
	    sizeof ((*stage_drive)->auto_lib));
	(*stage_drive)->count = print_lib->no_of_drives;
	(*stage_drive)->change_flag = STAGER_DEFAULT_DRIVE_CHANGE_FLAG;
	free_library(print_lib);
	Trace(TR_MISC, "get stager's default drive directive success");
	return (0);
}


/*
 *	get_default_staging_buffer_directive()
 */
int
get_default_staging_buffer_directive(
ctx_t *ctx,				/* ARGSUSED */
mtype_t media_type,			/* media type */
buffer_directive_t **stage_buffer)	/* must be freed by caller */
{

	Trace(TR_MISC, "get stager's default buffer directive()");
	if (ISNULL(stage_buffer)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	allocate memory for *stager_buffer and this
	 *	memory must be released in the calling function.
	 */
	*stage_buffer = (buffer_directive_t *)
	    mallocer(sizeof (buffer_directive_t));
	if (*stage_buffer == NULL) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	memset(*stage_buffer, 0, sizeof (buffer_directive_t));
	strlcpy((*stage_buffer)->media_type, media_type,
	    sizeof ((*stage_buffer)->media_type));
	(*stage_buffer)->size = STAGER_DEFAULT_MC_BUFSIZE;
	(*stage_buffer)->lock = STAGER_DEFAULT_BUFFER_LOCK;
	(*stage_buffer)->change_flag = STAGER_DEFAULT_BUFFER_CHANGE_FLAG;
	Trace(TR_MISC, "get stager's default buffer directive() success");
	return (0);
}


/*
 *	get_default_staging_cfg()
 *	get stager.cmd's default value.
 */
int
get_default_stager_cfg(
ctx_t *ctx,			/* ARGSUSED */
stager_cfg_t **stager_config)	/* must be freed by caller */
{
	int i;
	sqm_lst_t *media_type_list;
	sqm_lst_t *lib_list;
	node_t *node_media, *node_lib;
	char *media_type;
	library_t *print_lib;
	buffer_directive_t *buffer_directive;
	drive_directive_t *drive_directive;

	Trace(TR_MISC, "get stager's default configuration");
	if (ISNULL(stager_config)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	allocate memory for *stager_config and this
	 *	memory must be released in the calling function.
	 */
	*stager_config = (stager_cfg_t *)
	    mallocer(sizeof (stager_cfg_t));
	if (*stager_config == NULL) {
		Trace(TR_ERR, "stager.c:get_default_stager_cfg() %s",
		    samerrmsg);
		return (-1);
	}
	memset(*stager_config, 0, sizeof (stager_cfg_t));

	(*stager_config)->max_active = STAGER_DEFAULT_MAX_ACTIVE;
	(*stager_config)->max_retries = STAGER_DEFAULT_MAX_RETRIES;
	(*stager_config)->change_flag = STAGER_DEFAULT_CHANGE_FLAG;
	strlcpy((*stager_config)->stage_log, STAGER_DEFAULT_LOG,
	    sizeof ((*stager_config)->stage_log));
	(*stager_config)->options |= ST_LOG_DEFAULTS;

	(*stager_config)->stage_buf_list = lst_create();
	if ((*stager_config)->stage_buf_list == NULL) {
		free_stager_cfg(*stager_config);
		*stager_config = NULL;
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	(*stager_config)->stage_drive_list = lst_create();
	if ((*stager_config)->stage_drive_list == NULL) {
		free_stager_cfg(*stager_config);
		*stager_config = NULL;
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	get a list with all available media type.
	 */
	i = get_all_available_media_type(ctx, &media_type_list);
	if (i != 0) {
		free_stager_cfg(*stager_config);
		*stager_config = NULL;
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	traverse the list of media type and assign
	 *	the default values.
	 */
	node_media = media_type_list->head;
	while (node_media != NULL) {
		media_type = (char *)node_media->data;
		buffer_directive = (buffer_directive_t *)
		    mallocer(sizeof (buffer_directive_t));
		if (buffer_directive == NULL) {
			goto error;
		}
		strlcpy(buffer_directive->media_type, media_type,
		    sizeof (buffer_directive->media_type));
		buffer_directive->size = STAGER_DEFAULT_MC_BUFSIZE;
		buffer_directive->lock = STAGER_DEFAULT_BUFFER_LOCK;
		buffer_directive->change_flag =
		    STAGER_DEFAULT_BUFFER_CHANGE_FLAG;
		if (lst_append((*stager_config)->stage_buf_list,
		    buffer_directive) != 0) {
			goto error;
		}
		node_media = node_media->next;
	}
	/*
	 *	get a list with all libraries.
	 */
	i = get_all_libraries(ctx, &lib_list);
	if (i != 0) {
		goto error;
	}
	/*
	 *	traverse the library list and assign each
	 *	structure field a default value.
	 */
	node_lib = lib_list->head;
	while (node_lib != NULL) {
		print_lib = (library_t *)node_lib->data;
		if (strcmp((print_lib->base_info).equ_type, "hy") != 0) {
			drive_directive = (drive_directive_t *)
			    mallocer(sizeof (drive_directive_t));
			if (drive_directive == NULL) {
				free_list_of_libraries(lib_list);
				goto error;
			}

			strlcpy(drive_directive->auto_lib,
			    (print_lib->base_info).set,
			    sizeof (drive_directive->auto_lib));
			drive_directive->count =
			    print_lib->no_of_drives;
			drive_directive->change_flag =
			    STAGER_DEFAULT_DRIVE_CHANGE_FLAG;
			if (lst_append((*stager_config)->stage_drive_list,
			    drive_directive) != 0) {
				free_list_of_libraries(lib_list);
				goto error;
			}
		}
		node_lib = node_lib->next;
	}
	free_list_of_libraries(lib_list);
	lst_free_deep(media_type_list);
	Trace(TR_MISC, "get stager's default configuration success");
	return (0);
error:
	free_stager_cfg(*stager_config);
	lst_free_deep(media_type_list);
	*stager_config = NULL;
	Trace(TR_ERR, "%s", samerrmsg);
	return (-1);
}
