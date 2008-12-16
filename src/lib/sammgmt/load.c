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
#pragma ident   "$Revision: 1.19 $"

static char *_SrcFile = __FILE__;  /* Using __FILE__ makes duplicate strings */

/*
 * load.c contains control side implementation of load.h.
 */

#include <strings.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <pwd.h>
#include <unistd.h>
#include <fcntl.h>

#include "aml/shm.h"
#include "aml/fifo.h"
#include "sam/lib.h"
#include "sam/sam_trace.h"

#include "mgmt/util.h"

#include "pub/mgmt/error.h"
#include "pub/mgmt/load.h"


static int find_preview(preview_tbl_t *, char *vsn);
static int issue_fifo_cmd(sam_cmd_fifo_t *, int cmd);


/*
 * Function to get information about all pending load information.
 */
int
get_pending_load_info(
ctx_t *ctx,						/* ARGSUSED */
sqm_lst_t **pending_load_infos)	/* OUT - list of pending_load_info_t */
{
	shm_alloc_t preview_shm;
	shm_preview_tbl_t *shm_preview_tbl;
	preview_tbl_t *preview_tbl;
	preview_t *p;
	pending_load_info_t *info = NULL;
	sqm_lst_t *load_infos = NULL;
	struct passwd *pw = NULL;
	time_t now;
	int avail;		/* available entry count */
	int count;		/* active entry count */
	int i;

	Trace(TR_MISC, "getting pending load info");

	if (ISNULL(pending_load_infos)) {
		Trace(TR_OPRMSG, "null argument found");
		goto err;
	}

	load_infos = lst_create();
	if (load_infos == NULL) {
		Trace(TR_OPRMSG, "out of memory");
		goto err;
	}

	Trace(TR_OPRMSG, "attaching to shared memory");

	preview_shm.shmid = shmget(SHM_PREVIEW_KEY, 0, 0);
	if (preview_shm.shmid < 0) {
		samerrno = SE_PREVIEW_SHM_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_OPRMSG,
		    "unable to find preview shared memory segment");
		*pending_load_infos = load_infos;
		Trace(TR_MISC, "got 0 pending load info");
		return (-2);
	}

	preview_shm.shared_memory = shmat(preview_shm.shmid, NULL, SHM_RDONLY);
	if (preview_shm.shared_memory == (void *)-1) {
		samerrno = SE_PREVIEW_SHM_ATTACH_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_OPRMSG,
		    "unable to attach preview shared memory segment");
		goto err;
	}

	shm_preview_tbl = (shm_preview_tbl_t *)preview_shm.shared_memory;
	preview_tbl = &shm_preview_tbl->preview_table;

	Trace(TR_OPRMSG, "creating pending load info list");

	avail = preview_tbl->avail;
	count = preview_tbl->ptbl_count;

	for (i = 0; i < avail && count != 0; i++) {
		p = &preview_tbl->p[i];

		if (!p->in_use)
			continue;

		count--;

		info = (pending_load_info_t *)
		    mallocer(sizeof (pending_load_info_t));
		if (info == NULL) {
			Trace(TR_OPRMSG, "out of memory");
			goto err;
		}

		info->id = i;

		info->flags = 0;
		if (p->busy)		info->flags |= LD_BUSY;
		if (p->in_use)		info->flags |= LD_IN_USE;
		if (p->p_error)		info->flags |= LD_P_ERROR;
		if (p->write)		info->flags |= LD_WRITE;
		if (p->fs_req)		info->flags |= LD_FS_REQ;
		if (p->block_io)	info->flags |= LD_BLOCK_IO;
		if (p->stage)		info->flags |= LD_STAGE;
		if (p->flip_side)	info->flags |= LD_FLIP_SIDE;

		info->count = p->count;

		info->robot_equ = p->robot_equ;

		info->remote_equ = p->remote_equ;

		snprintf(info->media, sizeof (info->media),
		    sam_mediatoa(p->resource.archive.rm_info.media));

		info->slot = p->slot;

		info->ptime = p->ptime;

		now = time(NULL);
		info->age = now - p->ptime;

		info->priority = p->priority;

		info->pid = p->handle.pid;

		pw = getpwuid(getuid());

		if (pw != NULL) {
			snprintf(info->user, sizeof (info->user), pw->pw_name);
		} else {
			snprintf(info->user, sizeof (info->user),
			    "Unknown user");
		}

		snprintf(info->vsn, sizeof (vsn_t), p->resource.archive.vsn);

		if (lst_append(load_infos, info) == -1) {
			Trace(TR_OPRMSG, "lst append failed");
			goto err;
		}
	}

	shmdt(preview_shm.shared_memory);
	*pending_load_infos = load_infos;

	Trace(TR_OPRMSG, "returned pending load info list of size [%d]",
	    load_infos->length);
	Trace(TR_MISC, "got pending load info");
	return (0);

err:
	if (preview_shm.shared_memory != (void *)-1) {
		shmdt(preview_shm.shared_memory);
	}
	if (load_infos)
		free_list_of_pending_load_info(load_infos);
	if (info)
		free_pending_load_info(info);

	Trace(TR_ERR, "get pending load info failed: %s", samerrmsg);
	return (-1);
}


/*
 * Cancel a load request.
 */
int
clear_load_request(
ctx_t *ctx,	/* ARGSUSED */
vsn_t vsn,	/* IN - vsn */
int index)	/* IN - (optional) index in preview queue or -1 */
{
	shm_alloc_t preview_shm;
	shm_preview_tbl_t *shm_preview_tbl;
	preview_tbl_t *preview_tbl;
	sam_cmd_fifo_t cmd_block;
	int count;		/* active entry count */

	Trace(TR_MISC, "clearing load request");

	if (ISNULL(vsn)) {
		Trace(TR_OPRMSG, "null argument found");
		goto err;
	}

	Trace(TR_OPRMSG, "attaching to shared memory");

	preview_shm.shmid = shmget(SHM_PREVIEW_KEY, 0, 0);
	if (preview_shm.shmid < 0) {
		samerrno = SE_PREVIEW_SHM_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_OPRMSG,
		    "unable to find preview shared memory segment");
		goto err;
	}

	preview_shm.shared_memory = shmat(preview_shm.shmid, NULL, SHM_RDONLY);
	if (preview_shm.shared_memory == (void *)-1) {
		samerrno = SE_PREVIEW_SHM_ATTACH_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno));
		Trace(TR_OPRMSG,
		    "unable to attach preview shared memory segment");
		goto err;
	}

	shm_preview_tbl = (shm_preview_tbl_t *)preview_shm.shared_memory;
	preview_tbl = &shm_preview_tbl->preview_table;

	Trace(TR_OPRMSG, "locating vsn");

	count = preview_tbl->ptbl_count;

	if (index >= 0) {
		if (index > count) {
			samerrno = SE_INDEX_NOT_IN_PREVIEW;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), index);
			Trace(TR_OPRMSG,
			    "index %d not in preview queue", index);
			goto err;
		}
		if (strcmp(preview_tbl->p[index].resource.archive.vsn, vsn)) {
			samerrno = SE_VSN_INDEX_INCONSISTENT;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno));
			Trace(TR_OPRMSG, "index and vsn inconsistent");
			goto err;
		}
	} else {
		index = find_preview(preview_tbl, (char *)vsn);
		if (index < 0) {
			samerrno = SE_VSN_NOT_FOUND;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno));
			Trace(TR_OPRMSG, "vsn %s not found", vsn);
			goto err;
		}
	}

	Trace(TR_OPRMSG, "issuing fifo command");

	memset(&cmd_block, 0, sizeof (cmd_block));
	cmd_block.slot = index;

	if (issue_fifo_cmd(&cmd_block, CMD_FIFO_DELETE_P) == -1) {
		Trace(TR_OPRMSG, "fifo command failed");
		goto err;
	}

	shmdt(preview_shm.shared_memory);

	Trace(TR_OPRMSG, "clear load request command executed");
	Trace(TR_MISC, "load request cleared");
	return (0);

err:
	if (preview_shm.shared_memory != (void *)-1) {
		shmdt(preview_shm.shared_memory);
	}

	Trace(TR_ERR, "clear load request failed: %s", samerrmsg);
	return (-1);
}


/*
 * **************************
 *  Private helper functions
 * **************************
 */

/*
 * find a preview entry.
 */
static int					/* RETURN - index of found */
find_preview(
preview_tbl_t *preview_tbl,	/* IN - preview table */
char *vsn)					/* IN - vsn */
{
	preview_t *ent;
	int i, count, match;

	match = 0;
	count = preview_tbl->ptbl_count;
	for (i = 0; i < preview_tbl->avail && count != 0; i++) {
		ent = &preview_tbl->p[i];
		if (!ent->in_use)  continue;

		count--;

		if (strcmp(vsn, ent->resource.archive.vsn) == 0) {
			match++;
			break;
		}
	}

	if (match)
		return (i);

	return (-1);
}


#define	FIFO_path	SAM_FIFO_PATH"/"CMD_FIFO_NAME

/*
 * put a command into cmd fifo pipe
 */
static int
issue_fifo_cmd(
sam_cmd_fifo_t *cmd_block,	/* IN - cmd fifo */
int cmd)					/* IN - cmd */
{
	int fifo_fd;	/* file descriptor for FIFO */

	fifo_fd = open(FIFO_path, O_WRONLY);
	if (fifo_fd < 0) {
		samerrno = SE_CMD_FIFO_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno));
		return (-1);
	}

	cmd_block->magic = CMD_FIFO_MAGIC;
	cmd_block->cmd = cmd;

	write(fifo_fd, cmd_block, sizeof (sam_cmd_fifo_t));
	close(fifo_fd);

	return (0);
}
