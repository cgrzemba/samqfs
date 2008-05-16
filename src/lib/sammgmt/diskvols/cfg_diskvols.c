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
 * diskvols.c
 * contains functions to read, write and search the diskvols configuration.
 */

#include <sys/stat.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "mgmt/config/cfg_diskvols.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/error.h"
#include "parser_utils.h"
#include "mgmt/util.h"
#include "aml/diskvols.h"
#include "sam/sam_trace.h"


/* full path to the diskvols.conf file */
static char *diskvols_file = DISKVOL_CFG;


/* private functions */
static int write_diskvols_conf(const char *location,
	const diskvols_cfg_t *cfg);

static int write_disk_vol(FILE *f, disk_vol_t *dv);


/*
 * read the diskvols.conf file.
 */
int
read_diskvols_cfg(
ctx_t *ctx,		/* optional alternate diskvols file to read */
diskvols_cfg_t **cfg)	/* malloced return value */
{

	char *location = NULL;
	int err = 0;

	Trace(TR_OPRMSG, "reading diskvols cfg");

	if (ISNULL(cfg)) {
		Trace(TR_OPRMSG, "reading diskvols cfg failed: %s", samerrmsg);
		return (-1);
	}

	if (ctx != NULL && *ctx->read_location != NULL) {
		location = assemble_full_path(ctx->read_location,
		    DISKVOLS_DUMP_FILE, B_FALSE, NULL);
	} else {
		location = diskvols_file;
	}


	err = parse_diskvols_conf(location, cfg);

	Trace(TR_OPRMSG, "read diskvols cfg");
	return (err);
}


/*
 * Return one disk_vol_t based on vsn name.
 */
int
get_disk_vsn(
const diskvols_cfg_t *cfg,	/* cfg to search */
const char *vsn_name,		/* name of disk_vol to get */
disk_vol_t **disk_vol)		/* malloced return value */
{

	node_t *node;
	disk_vol_t *dv;

	Trace(TR_OPRMSG, "getting diskvol %s", Str(vsn_name));

	if (ISNULL(cfg, vsn_name)) {
		Trace(TR_OPRMSG, "getting diskvol failed: %s", samerrmsg);
		return (-1);
	}

	if (cfg->disk_vol_list != NULL) {
	for (node = cfg->disk_vol_list->head;
	    node != NULL; node = node->next) {

		dv = (disk_vol_t *)node->data;
		if (strcmp(dv->vol_name, vsn_name) == 0) {

			*disk_vol = (disk_vol_t *)mallocer(
			    sizeof (disk_vol_t));

			if (*disk_vol == NULL) {
				Trace(TR_OPRMSG, "getting diskvol failed: %s",
				    samerrmsg);
				return (-1);
			}

			memcpy(*disk_vol, dv, sizeof (disk_vol_t));
			Trace(TR_OPRMSG, "got diskvol");
			return (0);
		}
	}
	}

	*disk_vol = NULL;
	samerrno = SE_NOT_FOUND;
	/* %s not found */
	snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(SE_NOT_FOUND),
	    vsn_name);

	Trace(TR_OPRMSG, "getting diskvol failed: %s", samerrmsg);
	return (-1);

}


/*
 * verify that the diskvols_cfg_t is valid.
 */
int
verify_diskvols_cfg(
const diskvols_cfg_t *cfg) /* cfg to verify */
{

	node_t *node;

	Trace(TR_OPRMSG, "verifying diskvols cfg");

	if (ISNULL(cfg)) {
		Trace(TR_OPRMSG, "verifying diskvols cfg failed: %s",
		    samerrmsg);
		return (-1);
	}
	if (cfg->disk_vol_list != NULL) {
		for (node = cfg->disk_vol_list->head;
		    node != NULL; node = node->next) {
			if (check_disk_vol((disk_vol_t *)node->data) != 0) {
				Trace(TR_OPRMSG,
				    "verifying diskvols cfg %s: %s",
				    "failed", samerrmsg);
				return (-1);
			}
		}
	}
	Trace(TR_OPRMSG, "verifying diskvols cfg");
	return (0);
}


/*
 * verify a single volume.
 */
int
check_disk_vol(
disk_vol_t *dv)	/* disk_vol to check */
{

	Trace(TR_DEBUG, "checking diskvol");
	if (ISNULL(dv)) {
		Trace(TR_DEBUG, "checking diskvol failed: %s", samerrmsg);
		return (-1);
	}
	if (dv->status_flags & DV_STK5800_VOL) {
		if (*(dv->host) == '\0') {
			setsamerr(SE_HC_NEEDS_IP);
			Trace(TR_DEBUG, "checking diskvol failed: %s",
			    samerrmsg);
			return (-1);
		}
	}
	if (*(dv->vol_name) == '\0') {
		setsamerr(SE_DV_NEEDS_NAME);
		Trace(TR_DEBUG, "checking diskvol failed: %s",
		    samerrmsg);
		return (-1);
	}


	if (has_spaces(dv->vol_name, "vol_name") != 0) {
		Trace(TR_DEBUG, "checking diskvol failed: %s", samerrmsg);
		return (-1);
	}
	if (has_spaces(dv->path, "path") != 0) {
		Trace(TR_DEBUG, "checking diskvol failed: %s", samerrmsg);
		return (-1);
	}
	if (has_spaces(dv->host, "host") != 0) {
		Trace(TR_DEBUG, "checking diskvol failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_DEBUG, "checked diskvol");
	return (0);
}


/*
 * write the diskvols.conf file.
 */
int
write_diskvols_cfg(
ctx_t *ctx,
diskvols_cfg_t *cfg,	/* cfg to write */
const boolean_t force)	/* if true cfg will be written even if backup fails */
{

	boolean_t dump = B_FALSE;
	char *dmp_loc = NULL;

	Trace(TR_FILES, "writing diskvols cfg to %s", diskvols_file);

	if (ISNULL(cfg)) {
		Trace(TR_ERR, "writing diskvols cfg failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (verify_diskvols_cfg(cfg) != 0) {

		/* Leave samerrno as set */
		Trace(TR_ERR, "writing diskvols cfg failed: %s",
		    samerrmsg);

		return (-1);
	}

	dump = ctx != NULL && *ctx->dump_path != '\0';

	if (!dump) {
		/* possibly backup the cfg (see backup_cfg for details) */
		if (backup_cfg(diskvols_file) != 0 && !force) {
			/* leave samerrno as set */
			Trace(TR_ERR, "writing diskvols cfg failed: %s",
			    samerrmsg);

			return (-1);
		}
	}
	if (dump) {

		dmp_loc = assemble_full_path(ctx->dump_path,
		    DISKVOLS_DUMP_FILE, B_FALSE, NULL);

		if (dump_diskvols_cfg(cfg, dmp_loc) != 0) {
			/* leave samerrno as set */
			Trace(TR_ERR, "writing diskvols to %s failed: %s",
			    Str(ctx->dump_path), samerrmsg);

			return (-1);
		}

		Trace(TR_FILES, "wrote diskvols cfg to %s",
		    Str(dmp_loc));
		return (0);

	} else if (write_diskvols_conf(diskvols_file, cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "writing diskvols cfg failed: %s",
		    samerrmsg);

		return (-1);
	} else {
		Trace(TR_FILES, "wrote diskvols.conf file %s", diskvols_file);
	}

	/* always backup the new file. */
	backup_cfg(diskvols_file);

	Trace(TR_FILES, "wrote diskvols cfg");
	return (0);

}


/*
 * dump the diskvols_cfg_t to an alternate location.
 */
int
dump_diskvols_cfg(
const diskvols_cfg_t *cfg,	/* cfg to dump */
const char *location)		/* full path at which to dump the cfg */
{

	int err;

	Trace(TR_FILES, "dumping diskvols cfg to %s", Str(location));

	if (ISNULL(cfg, location)) {
		Trace(TR_ERR, "dumping diskvols cfg  failed: %s", samerrmsg);
		return (-1);
	}

	if (strcmp(location, DISKVOL_CFG) == 0) {
		samerrno = SE_INVALID_DUMP_LOCATION;

		/* Cannot dump the file to %s */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_DUMP_LOCATION), location);

		Trace(TR_ERR, "dumping diskvols cfg  failed: %s", samerrmsg);
		return (-1);
	}

	err = write_diskvols_conf(location, cfg);

	Trace(TR_OPRMSG, "dumped diskvols cfg %d", err);
	return (err);


}


/*
 * internal function that does the writing.
 */
static int
write_diskvols_conf(
const char *location,		/* location at which to write the cfg */
const diskvols_cfg_t *cfg)	/* cfg to write */
{

	FILE *f = NULL;
	time_t the_time;
	node_t *node;
	char err_buf[256];
	int fd;


	if ((fd = open(location, O_WRONLY|O_CREAT|O_TRUNC, 0644)) != -1) {
		f = fdopen(fd, "w");
	}

	if (f == NULL) {
		samerrno = SE_CFG_OPEN_FAILED;
		/* Open failed for %s: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), location, err_buf);

		Trace(TR_DEBUG, "write diskvols conf failed: %s",
		    samerrmsg);

		return (-1);
	}



	fprintf(f, "#\n#\tdiskvols.conf\n");
	fprintf(f, "#\n");
	the_time = time(0);
	fprintf(f, "#  Generated by libsammgmt api %s#\n", ctime(&the_time));
	fprintf(f, "#  VSN Name [Host Name:]Path\n#");
	if (cfg->disk_vol_list != NULL) {
		for (node = cfg->disk_vol_list->head; node != NULL;
		    node = node->next) {
			write_disk_vol(f, (disk_vol_t *)node->data);
		}
	}

	if (cfg->client_list != NULL && cfg->client_list->length != 0) {
		fprintf(f, "\n#\n#\nclients");
		for (node = cfg->client_list->head; node != NULL;
		    node = node->next) {
			fprintf(f, "\n%s", (char *)(node->data));
		}
		fprintf(f, "\nendclients");
	}

	fprintf(f, "\n");
	if (fclose(f) != 0) {
		samerrno = SE_CFG_CLOSE_FAILED;

		/* Close failed for %s: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_CLOSE_FAILED), location, err_buf);

		Trace(TR_DEBUG, "write diskvols.conf failed: %s",
		    samerrmsg);

		return (-1);
	}

	Trace(TR_DEBUG, "wrote diskvols.conf");
	return (0);
}


/*
 * internal function to write a single diskvol.
 */
static int
write_disk_vol(
FILE *f,	/* file to write to */
disk_vol_t *dv)	/* disk_vol_t to write */
{

	if (dv->status_flags & DV_STK5800_VOL) {
		fprintf(f, "\n%s %s %s", dv->vol_name, HONEYCOMB_RESOURCE_NAME,
		    dv->host);
	} else if (*dv->host == '\0') {
		fprintf(f, "\n%s %s", dv->vol_name, dv->path);
	} else {
		fprintf(f, "\n%s %s:%s", dv->vol_name, dv->host, dv->path);
	}
	return (0);
}
