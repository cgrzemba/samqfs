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
#pragma ident   "$Revision: 1.25 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */


/*
 * cfg_recycler.c
 * Contains functions that interact with the parsers to read, write and
 * verify recycler.cmd files and helper functions to search through the
 * recycler_cfg_t struct.
 */

#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <libgen.h>
#include <string.h>
#include <fcntl.h>

#include "sam/sam_trace.h"

#include "mgmt/config/recycler.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/recycle.h"
#include "pub/mgmt/error.h"
#include "pub/devstat.h"
#include "pub/mgmt/device.h"

#include "parser_utils.h"
#include "mgmt/util.h"
#include "mgmt/private_file_util.h"

/* full path to the recycler.cmd file */
static char *recycler_file = RECYCLE_CFG;


extern char *StrFromFsize(uint64_t size, int prec,
	char *buf, int buf_size);

/* private funtions */
static int write_recycler_cmd(const char *location, const recycler_cfg_t *cfg);
static void write_no_rc_vsn(FILE *f, no_rc_vsns_t *no_rc);
static void write_robot(FILE *f, rc_robot_cfg_t *rb);
static int verify_no_rc_vsns(no_rc_vsns_t *no_rc);
static int verify_rc_robot_cfg(rc_robot_cfg_t *rb, sqm_lst_t *libs);


/*
 * read the recycler cfg.
 */
int
read_recycler_cfg(
recycler_cfg_t **cfg)	/* malloced return */
{

	int ret_val;

	Trace(TR_OPRMSG, "reading recycler cfg %s", recycler_file);

	if (ISNULL(cfg)) {
		Trace(TR_OPRMSG, "reading recycler cfg %s", samerrmsg);
		return (-1);
	}

	ret_val = parse_recycler(recycler_file, cfg);

	Trace(TR_OPRMSG, "read recycler cfg");
	return (ret_val);
}


/*
 * verify that the recycler_cfg_t is valid
 */
int
verify_recycler_cfg(
const recycler_cfg_t *cfg)	/* cfg to verify */
{

	node_t *node;
	sqm_lst_t *libs = NULL;

	Trace(TR_OPRMSG, "verifying recycler cfg");

	if (ISNULL(cfg)) {
		Trace(TR_OPRMSG, "verifying recycler cfg failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (get_all_libraries(NULL, &libs) == -1) {
		/*
		 * don't return, let the checking try to pass
		 * it could still pass if there are no robot directives.
		 */
		libs = NULL;
	}

	if (*cfg->recycler_log != char_array_reset &&
	    cfg->change_flag & RC_recycler_log) {

		if (verify_file(cfg->recycler_log, B_TRUE) == B_FALSE) {
			samerrno = SE_LOGFILE_CANNOT_BE_CREATED;
			free_list_of_libraries(libs);

			/* Logfile %s can not be created */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_LOGFILE_CANNOT_BE_CREATED),
			    cfg->recycler_log);
			Trace(TR_OPRMSG, "verifying recycler cfg failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	if (*cfg->script != char_array_reset &&
	    cfg->change_flag & RC_script) {
		if (verify_file(cfg->script, B_FALSE) == B_FALSE) {
			free_list_of_libraries(libs);
			samerrno = SE_SCRIPT_DOES_NOT_EXIST;

			/* Script %s does not exist. */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_SCRIPT_DOES_NOT_EXIST),
			    cfg->script);

			Trace(TR_OPRMSG, "verifying recycler cfg failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	for (node = cfg->no_recycle_vsns->head; node != NULL;
	    node = node->next) {

		if (verify_no_rc_vsns((no_rc_vsns_t *)node->data) != 0) {

			free_list_of_libraries(libs);
			/* leave samerrmsg as set */
			Trace(TR_OPRMSG, "verifying recycler cfg failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	for (node = cfg->rc_robot_list->head; node != NULL;
	    node = node->next) {

		if (verify_rc_robot_cfg((rc_robot_cfg_t *)node->data,
		    libs) != 0) {

			free_list_of_libraries(libs);
			Trace(TR_OPRMSG, "verifying recycler cfg failed: %s",
			    samerrmsg);

			/* leave samerrmsg as set */
			return (-1);
		}
	}
	free_list_of_libraries(libs);
	Trace(TR_OPRMSG, "verified recycler cfg");

	return (0);
}


/*
 * function to verify that the no_rc_vsns is correct
 */
static int
verify_no_rc_vsns(
no_rc_vsns_t *no_rc)
{

	node_t *n;

	Trace(TR_DEBUG, "verifying no_rc_vsns");
	if (ISNULL(no_rc)) {
		Trace(TR_DEBUG, "verifying no_rc_vsns failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (*no_rc->media_type != char_array_reset) {
		if (check_media_type(no_rc->media_type) < 0) {
			samerrno = SE_INVALID_MEDIA_TYPE;

			/* %s is not a valid media type. */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_INVALID_MEDIA_TYPE),
			    no_rc->media_type);

			Trace(TR_DEBUG, "verifying no_rc_vsns failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	for (n = no_rc->vsn_exp->head; n != NULL; n = n->next) {
		if (regcmp((char *)n->data, NULL) == NULL) {
			samerrno = SE_MALFORMED_REGEXP;

			/* Malformed regular expression %s */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_MALFORMED_REGEXP),
			    (char *)n->data);

			Trace(TR_DEBUG, "verifying no_rc_vsns failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	Trace(TR_DEBUG, "verified no_rc_vsns");
	return (0);
}


/*
 * verify that the robot cfg is valid
 */
static int
verify_rc_robot_cfg(
rc_robot_cfg_t *rb,	/* robot to check */
sqm_lst_t *libs)		/* list of library_t to check against */
{

	Trace(TR_DEBUG, "verifying rc_robot_cfg");

	if (libs == NULL) {
		samerrno = SE_UNABLE_TO_CHECK_ROBOT_WITH_MCF;

		/* Unable to check robot against mcf */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_UNABLE_TO_CHECK_ROBOT_WITH_MCF));

		Trace(TR_DEBUG, "verifying rc_robot_cfg failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (rb->rc_params.hwm != int_reset &&
	    rb->rc_params.change_flag & RC_hwm) {
		if (rb->rc_params.hwm < 0 || rb->rc_params.hwm > 100) {

			samerrno = SE_VALUE_OUT_OF_RANGE;
			/* %s %d is out range %d %d */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_VALUE_OUT_OF_RANGE), "hwm",
			    rb->rc_params.hwm, 0, 100);

			Trace(TR_DEBUG, "verifying rc_robot_cfg failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	if (rb->rc_params.mingain != int_reset &&
	    rb->rc_params.change_flag & RC_mingain) {

		if (rb->rc_params.mingain < 0 || rb->rc_params.mingain > 100) {
			samerrno = SE_VALUE_OUT_OF_RANGE;

			/* %s %d is out range %d %d */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_VALUE_OUT_OF_RANGE), "mingain",
			    rb->rc_params.mingain, 0, 100);

			Trace(TR_DEBUG, "verifying rc_robot_cfg failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	if (rb->rc_params.data_quantity != fsize_reset &&
	    rb->rc_params.change_flag & RC_data_quantity) {
		if (rb->rc_params.data_quantity > FSIZE_MAX) {
			samerrno = SE_VALUE_OUT_OF_RANGE;

			/* "%s %d is out range %d %d" */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_VALUE_OUT_OF_RANGE), "mingain",
			    rb->rc_params.mingain, 0, FSIZE_MAX);

			Trace(TR_DEBUG, "verifying rc_robot_cfg failed: %s",
			    samerrmsg);
			return (-1);
		}
	}


	if (*rb->rc_params.email_addr != char_array_reset &&
	    rb->rc_params.change_flag & RC_email_addr) {
		if (has_spaces(rb->rc_params.email_addr, "email_addr") != 0) {
			Trace(TR_DEBUG, "verifying rc_robot_cfg failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	if (rb->rc_params.vsncount != int_reset &&
	    rb->rc_params.change_flag & RC_vsncount) {

		if (rb->rc_params.vsncount < 0) {
			samerrno = SE_INVALID_VALUE;

			/* %s %d is invalid */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_INVALID_VALUE),
			    "vsncount", rb->rc_params.vsncount, 0);

			Trace(TR_DEBUG, "verifying rc_robot_cfg failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	Trace(TR_DEBUG, "verified rc_robot_cfg");
	return (0);
}


/*
 * verify that the recycler_cfg is correct and write it to the
 * configuration location.
 */
int
write_recycler_cfg(
recycler_cfg_t *cfg,	/* cfg to write */
const boolean_t force)	/* if true cfg is written even if backup fails */
{

	Trace(TR_OPRMSG, "writing recycler cfg");

	if (ISNULL(cfg)) {
		Trace(TR_OPRMSG, "writing recycler cfg failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (verify_recycler_cfg(cfg) != 0) {
		/* Leave samerrno as set */
		Trace(TR_OPRMSG, "writing recycler cfg failed: %s",
		    samerrmsg);

		return (-1);
	}

	/* possibly backup the cfg (see backup_cfg for details) */
	if (backup_cfg(recycler_file) != 0 && !force) {
		/* leave samerrno as set */
		Trace(TR_OPRMSG, "writing recycler cfg failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (write_recycler_cmd(recycler_file, cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_OPRMSG, "writing recycler cfg failed: %s",
		    samerrmsg);

		return (-1);
	} else {
		Trace(TR_FILES, "wrote recycler.cmd file %s", recycler_file);
	}

	/* always backup the new file. */
	backup_cfg(recycler_file);

	Trace(TR_OPRMSG, "wrote recycler cfg");
	return (0);
}


/*
 * write the recycler.cmd file to an alternate location without running
 * verify.
 */
int
dump_recycler_cfg(
const recycler_cfg_t *cfg,	/* cfg to write */
const char *location)		/* location at which to write the cfg */
{

	int err = 0;

	Trace(TR_OPRMSG, "dumping recycler cfg %s", Str(location));

	if (ISNULL(cfg, location)) {
		Trace(TR_OPRMSG, "dumping recycler cfg failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (strcmp(location, RECYCLE_CFG) == 0) {
		samerrno = SE_INVALID_DUMP_LOCATION;

		/*  Cannot dump the file to %s */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_DUMP_LOCATION), location);

		Trace(TR_OPRMSG, "dumping recycler cfg failed: %s",
		    samerrmsg);

		return (-1);
	}
	err = write_recycler_cmd(location, cfg);

	Trace(TR_OPRMSG, "dumped recycler cfg");

	return (err);
}


/*
 * write the cmd file to the specified location.
 */
static int
write_recycler_cmd(
const char *location,		/* location at which to write the cfg */
const recycler_cfg_t *cfg)	/* cfg to write */
{

	FILE *f = NULL;
	int fd;
	time_t the_time;
	node_t *node;
	char err_buf[256];
	errno = 0;

	Trace(TR_DEBUG, "writing recycler.cmd");

	if (ISNULL(location, cfg)) {
		Trace(TR_DEBUG, "writing recycler.cmd failed: %s",
		    samerrmsg);
		return (-1);
	}

	if ((fd = open(location, O_WRONLY|O_CREAT|O_TRUNC, 0644)) != -1) {
		f = fdopen(fd, "w");
	}
	if (f == NULL) {
		samerrno = SE_CFG_OPEN_FAILED;

		/* Open failed for %s: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), location, err_buf);

		Trace(TR_DEBUG, "writing recycler.cmd failed: %s",
		    samerrmsg);

		return (-1);
	}



	fprintf(f, "#\n#\trecycler.cmd\n");
	fprintf(f, "#\n");
	the_time = time(0);
	fprintf(f, "#  Generated by by config api %s#\n", ctime(&the_time));


	if (*cfg->recycler_log != char_array_reset &&
	    cfg->change_flag & RC_recycler_log) {
		fprintf(f, "logfile = %s", cfg->recycler_log);
	}

	if (*cfg->script != char_array_reset && cfg->change_flag & RC_script) {
		fprintf(f, "\nscript = %s", cfg->script);
	}

	if (cfg->no_recycle_vsns != NULL) {
		for (node = cfg->no_recycle_vsns->head; node != NULL;
		    node = node->next) {
			write_no_rc_vsn(f, (no_rc_vsns_t *)node->data);
		}
	}

	if (cfg->rc_robot_list != NULL) {
		for (node = cfg->rc_robot_list->head; node != NULL;
		    node = node->next) {
			write_robot(f, (rc_robot_cfg_t *)node->data);
		}
	}


	fprintf(f, "\n");
	if (fclose(f) != 0) {
		char err_buf[80];
		samerrno = SE_CFG_CLOSE_FAILED;

		/* Close failed for %s: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_CLOSE_FAILED), location, err_buf);

		Trace(TR_DEBUG, "writing recycler.cmd failed: %s",
		    samerrmsg);

		return (-1);
	}

	Trace(TR_DEBUG, "wrote recycler.cmd");
	return (0);

}


/*
 * internal function that actually writes the no_rc_vsn entry
 * no_rc can not be null.
 */
static void
write_no_rc_vsn(
FILE *f,
no_rc_vsns_t *no_rc)	/* no_rc_vsns to write */
{

	node_t *n;

	Trace(TR_DEBUG, "writing no_rc_vsn");

	if (no_rc->vsn_exp == NULL || no_rc->vsn_exp->length == 0) {
		return;
	}

	fprintf(f, "\nno_recycle %s", no_rc->media_type);

	for (n = no_rc->vsn_exp->head; n != NULL; n = n->next) {
		fprintf(f, " %s", (char *)n->data);
	}

	Trace(TR_DEBUG, "wrote no_rc_vsn");

}


/*
 * internal function that actually writes the rc_robot_cfg_t entry
 */
static void
write_robot(
FILE *f,
rc_robot_cfg_t *rb)	/* robot to write */
{

	boolean_t rb_printed = B_FALSE;
	upath_t name;
	char fsizestr[FSIZE_STR_LEN];

	Trace(TR_DEBUG, "writing rc_robot");

	snprintf(name, sizeof (name), "\n%s", rb->robot_name);
	if (rb->rc_params.data_quantity != fsize_reset &&
	    rb->rc_params.change_flag & RC_data_quantity) {

		write_once(f, name, &rb_printed);
		fprintf(f, " -dataquantity %s", fsize_to_str(
		    rb->rc_params.data_quantity, fsizestr,
		    FSIZE_STR_LEN));
	}

	if (rb->rc_params.hwm != int_reset &&
	    rb->rc_params.change_flag & RC_hwm) {

		write_once(f, name, &rb_printed);
		fprintf(f, " -hwm %d", rb->rc_params.hwm);
	}

	if (rb->rc_params.ignore != flag_reset &&
	    rb->rc_params.change_flag & RC_ignore) {

		write_once(f, name, &rb_printed);
		fprintf(f, " -ignore");
	}

	if (*(rb->rc_params.email_addr) != '\0' &&
	    rb->rc_params.change_flag & RC_email_addr) {

		write_once(f, name, &rb_printed);
		fprintf(f, " -mail %s", rb->rc_params.email_addr);
	}

	if (rb->rc_params.mingain != int_reset &&
	    rb->rc_params.change_flag & RC_mingain) {

		write_once(f, name, &rb_printed);
		fprintf(f, " -mingain %d", rb->rc_params.mingain);
	}

	if (rb->rc_params.vsncount != int_reset &&
	    rb->rc_params.change_flag & RC_vsncount) {

		write_once(f, name, &rb_printed);
		fprintf(f, " -vsncount %d", rb->rc_params.vsncount);
	}

	Trace(TR_DEBUG, "wrote rc_robot");
}
