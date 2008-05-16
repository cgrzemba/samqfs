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
 *	recycler.c -  APIs for recycler.h.
 *	It calls functions of cfg_recycle.c and process
 *	the detailed recycler.cmd operation.
 */
#pragma	ident	"$Revision: 1.21 $"
#include <sys/types.h>
#include <time.h>
#include <stdio.h>

#include <string.h>

#include <stdlib.h>
#include "pub/mgmt/types.h"
#include "src/recycler/recycler.h"
#include "mgmt/util.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/recycle.h"
#include "mgmt/config/common.h"
#include "mgmt/config/recycler.h"
#include "sam/sam_trace.h"
#include "parser_utils.h"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */


/*
 *	following default value should be defined somewhere at
 *	sam's source tree.
 */
#define	DEFAULT_RC_LOG	""
#define	DEFAULT_RC_SCRIPT	SCRIPT_DIR"/recycler.sh"
#define	DEFAULT_RC_IGNORE	B_FALSE
#define	DEFAULT_RC_EMAIL_ADDRESS	"root"
#define	DEFAULT_RC_MAIL	B_FALSE
/*
 * following constants come from ../../../src/recycler/recycler.h
 * DEFAULT_ROBOT_HWM	95
 * DEFAULT_DATAQUANTITY	GIGA
 * DEFAULT_MIN_GAIN	50
 * DEFAULT_VSNCOUNT	1
 */

static char *recycler_file = RECYCLE_CFG;
extern char *StrFromFsize(uint64_t size, int prec,
    char *buf, int buf_size);

static int dup_rc_robot_cfg(const rc_robot_cfg_t *in, rc_robot_cfg_t **out);
static int dup_no_rc_vsns(no_rc_vsns_t *in, no_rc_vsns_t **out);


/*
 *	ctx structure is used by RPC clients to specify which
 *	connection should be used.
 *	see sammgmt_rpc.h for details.
 *	Also it is used to locate configuration file to dump
 *	configuration file for write functions.
 */


/*
 *	API get_rc_log(upath_t rc_log)
 *	Get log file location for recycler.
 */
int
get_rc_log(
ctx_t *ctx,	/* ARGSUSED */
upath_t rc_log)	/* recycler log path */
{
	recycler_cfg_t *recycler;
	rc_log[0] = '\0';

	Trace(TR_MISC, "get recycler log file location");
	/*
	 *	read recycler.cmd file to get all
	 *	recycler.cmd information.
	 */
	if (read_recycler_cfg(&recycler) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    recycler_file, samerrmsg);
		return (-1);
	}
	if (recycler->recycler_log != NULL) {
		strlcpy(rc_log, recycler->recycler_log,
		    sizeof (upath_t));
	}
	free_recycler_cfg(recycler);
	Trace(TR_MISC, "get recycler log file location success");
	return (0);
}


/*
 *	API set_rc_log(const upath_t rc_log)
 *	Set log file location for recycler.
 */
int
set_rc_log(
ctx_t *ctx,		/* ARGSUSED */
const upath_t rc_log)	/* recycler log path need to be set */
{
	recycler_cfg_t *recycler;

	Trace(TR_MISC, "set recycler log file location success");
	if (ISNULL(rc_log)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	read recycler.cmd file to get all
	 *	recycler.cmd information.
	 */
	if (read_recycler_cfg(&recycler) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    recycler_file, samerrmsg);
		return (-1);
	}
	if (*rc_log != char_array_reset) {
		strlcpy(recycler->recycler_log, rc_log,
		    sizeof (recycler->recycler_log));
		recycler->change_flag |= RC_recycler_log;

	} else {
		*recycler->recycler_log = char_array_reset;
	}
	/*
	 *	write recycler.cmd with TRUE option
	 *	and it will force to write a new recycler.cmd.
	 */
	if (write_recycler_cfg(recycler, B_TRUE) != 0) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	Trace(TR_FILES, "recycler log %s updated to recycler.cmd\n", rc_log);
	free_recycler_cfg(recycler);
	Trace(TR_MISC, "set recycler log file location success");
	return (0);
}


/*
 *	API get_default_rc_log(upath_t rc_log)
 *	Retrieve the default log file location for recycler.
 */
int
get_default_rc_log(
ctx_t *ctx,	/* ARGSUSED */
upath_t rc_log)	/* default recycler log path */
{

	rc_log[0] = '\0';
	Trace(TR_MISC, "get default recycler log file location");

	strlcpy(rc_log, DEFAULT_RC_LOG,
	    sizeof (upath_t));
	Trace(TR_MISC, "get default recycler log file location success");
	return (0);
}


/*
 *	API get_rc_notify_script(upath_t rc_script)
 *	Get notification scripts for recycler.
 */
int
get_rc_notify_script(
ctx_t *ctx,		/* ARGSUSED */
upath_t rc_script)	/* notify script path */
{
	recycler_cfg_t *recycler;
	rc_script[0] = '\0';

	Trace(TR_MISC, "get recycler notify script location");
	/*
	 *	read recycler.cmd file to get all
	 *	recycler.cmd information.
	 */
	if (read_recycler_cfg(&recycler) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    recycler_file, samerrmsg);
		return (-1);
	}
	if (recycler->script != NULL) {
		strlcpy(rc_script, recycler->script,
		    sizeof (upath_t));
	}
	free_recycler_cfg(recycler);
	Trace(TR_MISC, "get recycler notify script location success");
	return (0);
}


/*
 *	API set_rc_notify_script(const upath_t rc_script)
 *	Set notification scripts for recycler.
 */
int
set_rc_notify_script(
ctx_t *ctx,			/* ARGSUSED */
const upath_t rc_script)	/* notify script path need to be set */
{
	recycler_cfg_t *recycler;

	Trace(TR_MISC, "set recycler notify script location success");
	if (ISNULL(rc_script)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	read recycler.cmd file to get all
	 *	recycler.cmd information.
	 */
	if (read_recycler_cfg(&recycler) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    recycler_file, samerrmsg);
		return (-1);
	}
	if (*rc_script != char_array_reset) {
		strlcpy(recycler->script, rc_script,
		    sizeof (recycler->script));
		recycler->change_flag |= RC_script;
	} else {
		*recycler->script = char_array_reset;
	}
	/*
	 *	write recycler.cmd with TRUE option
	 *	and it will force to write a new recycler.cmd.
	 */
	if (write_recycler_cfg(recycler, B_TRUE) != 0) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	Trace(TR_FILES, "recycler script %s updated to recycler.cmd\n",
	    rc_script);
	free_recycler_cfg(recycler);
	Trace(TR_MISC, "set recycler notify script location success");
	return (0);
}


/*
 *	API get_default_rc_notify_script(upath_t rc_script)
 *	Retrieve the default location of the notification script
 *	for recycler.
 */
int
get_default_rc_notify_script(
ctx_t *ctx,		/* ARGSUSED */
upath_t rc_script)	/* default notify script path */
{

	Trace(TR_MISC, "get default recycler notify script location");
	rc_script[0] = '\0';

	strlcpy(rc_script, DEFAULT_RC_SCRIPT,
	    sizeof (upath_t));
	Trace(TR_MISC, "get default recycler notify script location success");
	return (0);
}


/*
 *	API get_all_no_rc_vsns(sqm_lst_t **no_rc_vsns)
 *	Function to get the list of all VSNs (as specified originally,
 *	i.e., regular expression, explicit vsn name etc.)
 *	for which no recycle has been set.
 */
int
get_all_no_rc_vsns(
ctx_t *ctx,		/* ARGSUSED */
sqm_lst_t **no_rc_vsns)	/* must be freed by caller */
{
	recycler_cfg_t *recycler = NULL;

	Trace(TR_MISC, "get all no recycler VSNs");
	if (ISNULL(no_rc_vsns)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	read recycler.cmd file to get all
	 *	recycler.cmd information.
	 */
	if (read_recycler_cfg(&recycler) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    recycler_file, samerrmsg);
		return (-1);
	}
	*no_rc_vsns = recycler->no_recycle_vsns;
	recycler->no_recycle_vsns = NULL;
	free_recycler_cfg(recycler);
	Trace(TR_MISC, "get all no recycler VSNs success");
	return (0);
}


/*
 *	API get_no_rc_vsns()
 *	Function to get the list of all VSNs (as specified originally,
 *	i.e., regular expression, explicit vsn name etc.)
 *	for which no recycle has been set.
 */
int
get_no_rc_vsns(
ctx_t *ctx,			/* ARGSUSED */
const mtype_t media_type,	/* media type */
no_rc_vsns_t **no_recycle_vsns)	/* must be freed by caller */
{
	recycler_cfg_t *recycler;
	no_rc_vsns_t *no_vsn;
	node_t *node_c;
	int match_flag = 0;

	Trace(TR_MISC, "get no recycler VSNs for media %s", media_type);
	if (ISNULL(no_recycle_vsns)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	read recycler.cmd file to get all
	 *	recycler.cmd information.
	 */
	if (read_recycler_cfg(&recycler) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    recycler_file, samerrmsg);
		return (-1);
	}
	/*
	 *	Following block traverses all no_recycle_vsn
	 *	list, if a match between given media type and
	 *	that list's media type is found, allocate
	 *	memory to *no_recycle_vsns and assign list's
	 *	no_vsn to it.
	 */
	node_c = (recycler->no_recycle_vsns)->head;
	while (node_c != NULL) {
		no_vsn = (no_rc_vsns_t *)node_c ->data;
		if (strcmp(no_vsn->media_type, media_type) == 0) {
			*no_recycle_vsns = no_vsn;
			node_c ->data = NULL;
			match_flag = 1;
			break;
		}
		node_c = node_c->next;
	}
	free_recycler_cfg(recycler);
	if (match_flag == 0) {
		samerrno = SE_NO_NO_RECYCLE_VSN_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NO_NO_RECYCLE_VSN_FOUND),
		    media_type);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	Trace(TR_MISC, "get no recycler VSNs for media %s success", media_type);
	return (0);
}


/*
 *	API get_default_rc_params(rc_param_t **rc_params)
 *	Function to get the default recycle parameters.
 */
int
get_default_rc_params(
ctx_t *ctx,			/* ARGSUSED */
rc_param_t **rc_params)		/* must be freed by caller */
{

	Trace(TR_MISC, "get default recycler params");
	if (ISNULL(rc_params)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	allocate memory for *rc_params and this
	 *	memory must be released in the calling function.
	 */
	*rc_params = (rc_param_t *)
	    mallocer(sizeof (rc_param_t));

	if (*rc_params == NULL) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	memset(*rc_params, 0, sizeof (rc_param_t));
	(*rc_params)->hwm = DEFAULT_ROBOT_HWM;
	(*rc_params)->data_quantity = DEFAULT_DATAQUANTITY;
	(*rc_params)->ignore = DEFAULT_RC_IGNORE;
	strlcpy((*rc_params)->email_addr, DEFAULT_RC_EMAIL_ADDRESS,
	    sizeof ((*rc_params)->email_addr));
	(*rc_params)->mingain = DEFAULT_MIN_GAIN;
	(*rc_params)->vsncount = DEFAULT_VSNCOUNT;
	(*rc_params)->mail = DEFAULT_RC_MAIL;
	Trace(TR_MISC, "get default recycler params success");
	return (0);
}


/*
 *	add_no_rc_vsns()
 *	Functions to add VSNs for which no recycle needs
 *	to be enforced for a particular type of media.
 */
int
add_no_rc_vsns(
ctx_t *ctx,			/* ARGSUSED */
no_rc_vsns_t *no_recycle_vsns)	/* no recycle VSNs */
{
	recycler_cfg_t *recycler;
	no_rc_vsns_t *no_vsn;
	node_t *node_c;
	int match_flag = 0;

	Trace(TR_MISC, "add no recycle vsns to configuration file");
	if (ISNULL(no_recycle_vsns)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	read recycler.cmd file to get all
	 *	recycler.cmd information.
	 */
	if (read_recycler_cfg(&recycler) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    recycler_file, samerrmsg);
		return (-1);
	}
	/*
	 *	Following block traverses all no_recycle_vsn
	 *	list, to find if given media type already
	 *	exists.
	 */
	node_c = (recycler->no_recycle_vsns)->head;
	while (node_c != NULL) {
		no_vsn = (no_rc_vsns_t *)node_c ->data;
		if (strcmp(no_vsn->media_type,
		    no_recycle_vsns->media_type) == 0) {
			match_flag = 1;
			break;
		}
		node_c = node_c->next;
	}
	if (match_flag == 1) {
		free_recycler_cfg(recycler);
		samerrno = SE_NO_RECYCLE_VSN_EXIST;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NO_RECYCLE_VSN_EXIST),
		    no_recycle_vsns->media_type);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	if (dup_no_rc_vsns(no_recycle_vsns, &no_vsn) != 0) {
		free_recycler_cfg(recycler);
		Trace(TR_MISC, "add_no_rc_vsns() %s",
		    samerrmsg);
		return (-1);
	}
	if (lst_append(recycler->no_recycle_vsns, no_vsn) != 0) {
		free_recycler_cfg(recycler);
		free_no_rc_vsns(no_vsn);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	write recycler.cmd with TRUE option
	 *	and it will force to write a new recycler.cmd.
	 */
	if (write_recycler_cfg(recycler, B_TRUE) != 0) {
		free_recycler_cfg(recycler);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	Trace(TR_FILES, "new no_vsn %s added to recycler.cmd\n",
	    no_recycle_vsns->media_type);
	free_recycler_cfg(recycler);
	Trace(TR_MISC, "add no recycle vsns to configuration file success");
	return (0);
}


/*
 *	modify_no_rc_vsns()
 *	Functions to modify VSNs for which no recycle needs
 *	to be enforced for a particular type of media.
 */
int
modify_no_rc_vsns(
ctx_t *ctx,			/* ARGSUSED */
no_rc_vsns_t *no_recycle_vsns)	/* no recycle VSNs */
{
	recycler_cfg_t *recycler;
	no_rc_vsns_t *no_vsn;
	node_t *node_c;
	int match_flag = 0;

	Trace(TR_MISC, "modify no recycle vsns");
	if (ISNULL(no_recycle_vsns)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	read recycler.cmd file to get all
	 *	recycler.cmd information.
	 */
	if (read_recycler_cfg(&recycler) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    recycler_file, samerrmsg);
		return (-1);
	}
	/*
	 *	Following block traverses all no_recycle_vsn
	 *	list, to find if given media type already
	 *	exists. If it exists, update its value.
	 */
	node_c = (recycler->no_recycle_vsns)->head;
	while (node_c != NULL) {
		no_vsn = (no_rc_vsns_t *)node_c ->data;
		if (strcmp(no_vsn->media_type,
		    no_recycle_vsns->media_type) == 0) {
			match_flag = 1;
			break;
		}
		node_c = node_c->next;
	}
	if (match_flag == 0) {
		free_recycler_cfg(recycler);
		samerrno = SE_NO_NO_RECYCLE_VSN_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NO_NO_RECYCLE_VSN_FOUND),
		    no_recycle_vsns->media_type);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	free_no_rc_vsns((no_rc_vsns_t *)node_c ->data);
	if (dup_no_rc_vsns(no_recycle_vsns, &no_vsn) != 0) {
		free_recycler_cfg(recycler);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	node_c ->data = no_vsn;
	/*
	 *	write recycler.cmd with TRUE option
	 *	and it will force to write a new recycler.cmd.
	 */
	if (write_recycler_cfg(recycler, B_TRUE) != 0) {
		free_recycler_cfg(recycler);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	Trace(TR_FILES, "new no_vsn %s modifed to recycler.cmd\n",
	    no_recycle_vsns->media_type);
	free_recycler_cfg(recycler);
	Trace(TR_MISC, "modify no recycle vsns success");
	return (0);
}


/*
 *	remove_no_rc_vsns()
 *	Functions to modify VSNs for which no recycle needs
 *	to be enforced for a particular type of media.
 */
int
remove_no_rc_vsns(
ctx_t *ctx,			/* ARGSUSED */
no_rc_vsns_t *no_recycle_vsns)	/* no recycle VSNs */
{
	recycler_cfg_t *recycler;
	no_rc_vsns_t *no_vsn;
	node_t *node_c;
	int match_flag = 0;

	Trace(TR_MISC, "remove no recycle vsns");
	if (ISNULL(no_recycle_vsns)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	read recycler.cmd file to get all
	 *	recycler.cmd information.
	 */
	if (read_recycler_cfg(&recycler) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    recycler_file, samerrmsg);
		return (-1);
	}
	/*
	 *	Following block traverses all no_recycle_vsn
	 *	list, to find if given media type already
	 *	exists. If it exists, remove it from list.
	 */
	node_c = (recycler->no_recycle_vsns)->head;
	while (node_c != NULL) {
		no_vsn = (no_rc_vsns_t *)node_c ->data;
		if (strcmp(no_vsn->media_type,
		    no_recycle_vsns->media_type) == 0) {
			if (lst_remove(recycler->no_recycle_vsns, node_c)
			    != 0) {
				free_recycler_cfg(recycler);
				Trace(TR_ERR, "%s", samerrmsg);
				return (-1);
			}
			free_no_rc_vsns(no_vsn);
			match_flag = 1;
			break;
		}
		node_c = node_c->next;
	}
	if (match_flag == 0) {
		free_recycler_cfg(recycler);
		samerrno = SE_NO_NO_RECYCLE_VSN_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NO_NO_RECYCLE_VSN_FOUND),
		    no_recycle_vsns->media_type);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	write recycler.cmd with TRUE option
	 *	and it will force to write a new recycler.cmd.
	 */
	if (write_recycler_cfg(recycler, B_TRUE) != 0) {
		free_recycler_cfg(recycler);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	Trace(TR_FILES, "no_vsn %s removed from recycler.cmd\n",
	    no_recycle_vsns->media_type);
	free_recycler_cfg(recycler);
	Trace(TR_MISC, "remove no recycle vsns success");
	return (0);
}


/*
 *	get_all_rc_robot_cfg()
 *	Function to get the recycle parameters for all the libraries.
 */
int
get_all_rc_robot_cfg(
ctx_t *ctx,		/* ARGSUSED */
sqm_lst_t **rc_robot_cfg)	/* It must be freed by caller */
{
	recycler_cfg_t *recycler;

	Trace(TR_MISC, "get all recycle robot configuration");
	if (ISNULL(rc_robot_cfg)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	read recycler.cmd file to get all
	 *	recycler.cmd information.
	 */
	if (read_recycler_cfg(&recycler) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    recycler_file, samerrmsg);
		return (-1);
	}
	*rc_robot_cfg = recycler->rc_robot_list;
	recycler->rc_robot_list = NULL;
	free_recycler_cfg(recycler);
	Trace(TR_MISC, "get all recycle robot configuration success");
	return (0);
}


/*
 *	get_rc_robot_cfg()
 *	Function to get the recycle parameters for a particular library.
 */
int
get_rc_robot_cfg(
ctx_t *ctx,			/* ARGSUSED */
const upath_t robot_name,	/* library name */
rc_robot_cfg_t **rc_robot_cfg)	/* It must be freed by caller */
{
	recycler_cfg_t *recycler;
	rc_robot_cfg_t *rc_robot;
	node_t *node_c;
	int match_flag = 0;

	Trace(TR_MISC, "get recycle robot configuration for %s", robot_name);
	if (ISNULL(rc_robot_cfg)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	*rc_robot_cfg = NULL;
	/*
	 *	read recycler.cmd file to get all
	 *	recycler.cmd information.
	 */
	if (read_recycler_cfg(&recycler) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    recycler_file, samerrmsg);
		return (-1);
	}
	node_c = (recycler->rc_robot_list)->head;
	while (node_c != NULL) {
		rc_robot = (rc_robot_cfg_t *)node_c ->data;
		if (strcmp(rc_robot->robot_name, robot_name) == 0) {
			*rc_robot_cfg = rc_robot;
			node_c ->data = NULL;
			match_flag = 1;
			break;
		}
		node_c = node_c->next;
	}
	if (match_flag == 0) {
		free_recycler_cfg(recycler);
		*rc_robot_cfg = NULL;
		samerrno = SE_NO_ROBOT_CFG_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NO_ROBOT_CFG_FOUND),
		    robot_name);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	free_recycler_cfg(recycler);
	Trace(TR_MISC, "get recycle robot configuration for %s success",
	    robot_name);
	return (0);
}


/*
 *	set_rc_robot_cfg()
 *	Functions to set recycle parameters
 *	for a particular library.
 */
int
set_rc_robot_cfg(
ctx_t *ctx,			/* ARGSUSED */
rc_robot_cfg_t *rc_robot_cfg)	/* rc_robot_cfg need to be set */
{
	recycler_cfg_t *recycler;
	rc_robot_cfg_t *rc_robot;
	node_t *node_c;
	int match_flag = 0;

	Trace(TR_MISC, "set recycle robot configuration");
	if (ISNULL(rc_robot_cfg)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	read recycler.cmd file to get all
	 *	recycler.cmd information.
	 */
	if (read_recycler_cfg(&recycler) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    recycler_file, samerrmsg);
		return (-1);
	}
	/*
	 *	Following block traverses all rc_robot_cfg
	 *	list, to find if given robot name already
	 *	exists. If it doesn't exist, add it to the list.
	 *	If it exists then check structure's
	 *	each field with bit field check. If a structure's
	 *	field bit is set, update that field with given
	 *	structure's field content.
	 */
	node_c = (recycler->rc_robot_list)->head;
	while (node_c != NULL) {
		rc_robot = (rc_robot_cfg_t *)node_c ->data;
		if (strcmp(rc_robot->robot_name,
		    rc_robot_cfg->robot_name) == 0) {
			match_flag = 1;
			break;
		}
		node_c = node_c->next;
	}
	if (dup_rc_robot_cfg(rc_robot_cfg, &rc_robot) != 0) {
		free_recycler_cfg(recycler);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	if (match_flag == 0) {
		if (lst_append(recycler->rc_robot_list, rc_robot) != 0) {
			free_recycler_cfg(recycler);
			free((rc_robot_cfg_t *)rc_robot);
			Trace(TR_ERR, "%s", samerrmsg);
			return (-1);
		}
	} else {
		free((rc_robot_cfg_t *)node_c ->data);
		node_c ->data = rc_robot;
	}
	/*
	 *	write recycler.cmd with TRUE option
	 *	and it will force to write a new recycler.cmd.
	 */
	if (write_recycler_cfg(recycler, B_TRUE) != 0) {
		free_recycler_cfg(recycler);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	Trace(TR_FILES, "new rc_robot %s seted to recycler.cmd\n",
	    rc_robot_cfg->robot_name);
	free_recycler_cfg(recycler);
	Trace(TR_MISC, "set recycle robot configuration success");
	return (0);
}


/*
 *	reset_rc_robot_cfg()
 *	Functions to remove recycle parameters from configuration
 *	file for a particular library.
 */
int
reset_rc_robot_cfg(
ctx_t *ctx,			/* ARGSUSED */
rc_robot_cfg_t *rc_robot_cfg)	/* rc_robot_cfg need to be reset */
{
	recycler_cfg_t *recycler;
	rc_robot_cfg_t *rc_robot;
	node_t *node_c;
	int match_flag = 0;

	Trace(TR_MISC, "reset recycle robot configuration");
	if (ISNULL(rc_robot_cfg)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	read recycler.cmd file to get all
	 *	recycler.cmd information.
	 */
	if (read_recycler_cfg(&recycler) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    recycler_file, samerrmsg);
		return (-1);
	}
	/*
	 *	Following block traverses all rc_robot_cfg
	 *	list, to find if given robot name already
	 *	exists. If it exists, remove it from the list.
	 */
	node_c = (recycler->rc_robot_list)->head;
	while (node_c != NULL) {
		rc_robot = (rc_robot_cfg_t *)node_c ->data;
		if (strcmp(rc_robot->robot_name,
		    rc_robot_cfg->robot_name) == 0) {
			if (lst_remove(recycler->rc_robot_list, node_c) != 0) {
				Trace(TR_ERR, "%s", samerrmsg);
				return (-1);
			}
			free(rc_robot);
			match_flag = 1;
			break;
		}
		node_c = node_c->next;
	}
	if (match_flag == 0) {
		free_recycler_cfg(recycler);
		samerrno = SE_NO_ROBOT_CFG_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NO_ROBOT_CFG_FOUND),
		    rc_robot_cfg->robot_name);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 *	write recycler.cmd with TRUE option
	 *	and it will force to write a new recycler.cmd.
	 */
	if (write_recycler_cfg(recycler, B_TRUE) != 0) {
		free_recycler_cfg(recycler);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	Trace(TR_FILES, "rc_robot %s removed from recycler.cmd\n",
	    rc_robot_cfg->robot_name);
	free_recycler_cfg(recycler);
	Trace(TR_MISC, "reset recycle robot configuration success");
	return (0);
}


/*
 *	support function
 */
static int
dup_no_rc_vsns(
no_rc_vsns_t *in,	/* incoming no_rc_vsns_t */
no_rc_vsns_t **out)	/* output no_rc_vsns_t */
{

	Trace(TR_OPRMSG, "duplicate no recycle vsns");
	*out = (no_rc_vsns_t *)mallocer(sizeof (no_rc_vsns_t));
	if (*out == NULL) {
		Trace(TR_OPRMSG, "%s", samerrmsg);
		return (-1);
	}
	/* copy the data */
	memcpy(*out, in, sizeof (no_rc_vsns_t));
	/* set ptr to null so free can work if errors encountered. */
	(*out)->vsn_exp = NULL;
	(*out)->vsn_exp = lst_create();
	if ((*out)->vsn_exp == NULL) {
		free_no_rc_vsns(*out);
		Trace(TR_OPRMSG, "%s", samerrmsg);
		return (-1);
	}
	if (dup_string_list(in->vsn_exp, &((*out)->vsn_exp)) != 0) {
		free_no_rc_vsns(*out);
		Trace(TR_OPRMSG, "%s", samerrmsg);
		return (-1);
	}
	Trace(TR_OPRMSG, "duplicate no recycle vsns success");
	return (0);
}


/*
 * duplicate the incoming struct. The resulting out structure
 * is dynamically allocated so it can be freed.
 */
static int
dup_rc_robot_cfg(
const rc_robot_cfg_t *in,	/* incoming rc_robot_cfg_t */
rc_robot_cfg_t **out)		/* output rc_robot_cfg_t */
{

	Trace(TR_OPRMSG, "duplicate recycle robot configuration");
	*out = (rc_robot_cfg_t *)mallocer(sizeof (rc_robot_cfg_t));
	if (*out == NULL) {
		Trace(TR_OPRMSG, "%s", samerrmsg);
		return (-1);
	}
	memcpy(*out, in, sizeof (rc_robot_cfg_t));
	Trace(TR_OPRMSG, "duplicate recycle robot configuration success");
	return (0);
}
