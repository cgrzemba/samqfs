/*
 * mount.c - thread for optical mount requests(non/stage).
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

#pragma ident "$Revision: 1.18 $"

static char    *_SrcFile = __FILE__;

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <sys/file.h>
#include <stdio.h>
#include <thread.h>
#include <synch.h>

#include "aml/shm.h"
#include "aml/proto.h"
#include "sam/fioctl.h"
#include "aml/logging.h"
#include "sam/nl_samfs.h"
#include "aml/dev_log.h"

/* some globals */


extern shm_alloc_t master_shm, preview_shm;

/*
 * mount_thread - thread mount requests.
 *
 * Handle mount requests after the media has been found on a device.
 */

void *
mount_thread(void *vparam)
{
	dev_ent_t	*un;
	int		retry = 5;
	char		*d_mess, *dc_mess;
	sam_actmnt_t	*actmnt_req = (sam_actmnt_t *)vparam;
	sam_io_reader_t	*control;
	sam_act_io_t	*active_io;
	int		rd_only;
	int		error;

	un = actmnt_req->un;
	rd_only = un->status.bits & (DVST_READ_ONLY | DVST_WRITE_PROTECT);
	control = (sam_io_reader_t *)malloc_wait(sizeof (sam_io_reader_t),
	    2, 0);
	(void) memset(control, 0, sizeof (sam_io_reader_t));

	d_mess = un->dis_mes[DIS_MES_NORM];
	dc_mess = un->dis_mes[DIS_MES_CRIT];
	if (!(IS_OPTICAL(un) || IS_TAPE(un))) {
		DevLog(DL_ERR(1045), un->equ_type);
		notify_fs_mount(&actmnt_req->handle, &actmnt_req->resource,
		    un, ENXIO);
		mutex_lock(&un->mutex);
		if (actmnt_req->resource.access == FWRITE)
			un->status.b.wr_lock = FALSE;
		DEC_ACTIVE(un);
		mutex_unlock(&un->mutex);
		free(control);
		free(actmnt_req);
		thr_exit((void *) NULL);
	}
	if (actmnt_req->callback != CB_NOTIFY_FS_LOAD) {
		DevLog(DL_DEBUG(1046));
		notify_fs_mount(&actmnt_req->handle, &actmnt_req->resource,
		    un, ENXIO);
		mutex_lock(&un->mutex);
		if (actmnt_req->resource.access == FWRITE)
			un->status.b.wr_lock = FALSE;
		DEC_ACTIVE(un);
		mutex_unlock(&un->mutex);
		free(control);
		free(actmnt_req);
		thr_exit((void *) NULL);
	}
	DevLog(DL_DETAIL(1047), un->vsn);
	mutex_lock(&un->mutex);
	while ((control->open_fd = open(un->name, rd_only ? O_RDONLY :
	    O_RDWR)) < 0 && (--retry > 0)) {
		if (errno == EACCES && !rd_only) {
			/* try opening with read-only access */
			un->status.bits |= DVST_WRITE_PROTECT;
			rd_only = 1;
		}
		DevLog(DL_RETRY(1048), un->name);
		mutex_unlock(&un->mutex);

		thr_yield();
		(void) sleep(2);
		mutex_lock(&un->mutex);
	}

	if (control->open_fd < 0 || un->vsn[0] == '\0') {
		DevLog(DL_SYSERR(1049), un->name);
		mutex_unlock(&un->mutex);
		notify_fs_mount(&actmnt_req->handle, &actmnt_req->resource,
		    un, errno);
		mutex_lock(&un->mutex);
		if (actmnt_req->resource.access == FWRITE)
			un->status.b.wr_lock = FALSE;
		DEC_ACTIVE(un);
		mutex_unlock(&un->mutex);
		free(control);
		free(actmnt_req);
		thr_exit((void *) NULL);
	}
	*dc_mess = '\0';
	INC_OPEN(un);
	DevLog(DL_DETAIL(1050), un->name, control->open_fd, un->open_count);

	mutex_unlock(&un->mutex);
	control->un = un;
	control->media_info = &actmnt_req->resource;

	error = dodirio(control, actmnt_req);

	mutex_lock(&un->mutex);
	DEC_OPEN(un);

	if ((active_io = (sam_act_io_t *)SHM_REF_ADDR(un->active_io)) != NULL) {
		if (active_io->fd_stage == control->open_fd) {
			active_io->fd_stage = -1;
		}
	}
	retry = 6;
	while (control->open_fd >= 0 && close(control->open_fd) < 0 &&
	    --retry > 0) {

		DevLog(DL_RETRY(1051), control->open_fd);
		(void) sleep(10);
	}

	if (retry <= 0)
		DevLog(DL_SYSERR(1052), control->open_fd);

	if (error == ETIME) {
		DevLog(DL_SYSERR(1065));
		mutex_unlock(&un->mutex);
		notify_fs_mount(&actmnt_req->handle, &actmnt_req->resource,
		    un, errno);
		mutex_lock(&un->mutex);
		if (actmnt_req->resource.access == FWRITE) {
			un->status.b.wr_lock = FALSE;
		}
		DEC_ACTIVE(un);
		mutex_unlock(&un->mutex);
		free(control);
		free(actmnt_req);
		thr_exit((void *) NULL);
	}
	*d_mess = '\0';
	un->mtime = time(NULL);	/* update last used time */
	DEC_ACTIVE(un);
	DevLog(DL_DETAIL(1053), control->open_fd, un->open_count);

	check_preview(un);	/* another? */
	mutex_unlock(&un->mutex);
	free(control);
	free(actmnt_req);
	thr_exit((void *) NULL);
	/* NOTREACHED */
	/* return ((void *) NULL); */
}

/*
 * unload_media - signal the thread doing io that an unload request has
 * arrived.
 */

void
unload_media(dev_ent_t *un, todo_request_t *request)
{
	sam_act_io_t   *active_io;

	mutex_lock(&un->mutex);
	active_io = (sam_act_io_t *)SHM_REF_ADDR(un->active_io);
	mutex_lock(&active_io->mutex);
	active_io->resource = request->resource;
	active_io->wait_fs_unload = active_io->fs_cancel = FALSE;
	active_io->timeout = FALSE;
	active_io->fd_stage = -1;
	active_io->active = FALSE;
	cond_signal(&active_io->cond);
	mutex_unlock(&un->mutex);
	mutex_unlock(&active_io->mutex);
}
