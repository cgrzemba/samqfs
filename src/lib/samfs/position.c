/*
 * position.c - thread for removable media position requests.
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.22 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

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
extern void notify_fs_position(sam_handle_t *handle, u_longlong_t position,
    const int errflag);
/*
 *  Position removable media file.
 *  Caller has locked un->io_mutex.
 */
void
position_rmedia(dev_ent_t *un
)
{
	sam_act_io_t	*active_io;
	char	*samst_name;
	int	fd;
	uint_t	position;

	int retry = 2;
	int err = 0;
	boolean_t local_open = B_FALSE;

	mutex_lock(&un->mutex);
	active_io = (sam_act_io_t *)SHM_REF_ADDR(un->active_io);
	DevLog(DL_DETAIL(5365), active_io->setpos, active_io->fd_stage);
	mutex_lock(&active_io->mutex);
	mutex_unlock(&un->mutex);

	if (un->status.b.wr_lock) {
		DevLog(DL_SYSERR(3285));
		notify_fs_position(&active_io->handle, 0, -1);
		mutex_unlock(&active_io->mutex);
		return;
	}

	DevLog(DL_DETAIL(5345), un->dt.tp.position, (uint_t)active_io->setpos);

	if (IS_TAPE(un)) {
		if (active_io->fd_stage <  0) {
			/* Device is not already open */
			samst_name = samst_devname(un);
			fd = open_unit(un, samst_name, retry);
			if (fd < 0) {
				DevLog(DL_SYSERR(3033), samst_devname(un));
				notify_fs_position(&active_io->handle, 0, -1);
				mutex_unlock(&active_io->mutex);
				return;
			} else {
				local_open = B_TRUE;
			}
		} else {
			/*
			 * dodirio() has already opened the device,
			 * use the same fd
			 */
			fd = active_io->fd_stage;
		}
		err = position_tape(un, fd, (uint_t)active_io->setpos);
		if (err == 0) {
			(void) read_position(un, fd, &position);
		} else {
			/*
			 * First attempt at positioning failed.
			 * Rewind the tape and try positioning one more time.
			 */
			err = rewind_tape(un, fd);
			if (err) {
				DevLog(DL_SYSERR(3067), 0);
			} else {
				err = position_tape(un, fd,
				    (uint_t)active_io->setpos);
				if (err == 0) {
					(void) read_position(un, fd, &position);
				} else {
					DevLog(DL_SYSERR(3067),
					    (uint_t)active_io->setpos);
				}
			}
		}
		if (local_open) {
			/* Close the fd if we opened it */
			mutex_lock(&un->mutex);
			close_unit(un, &fd);
			mutex_unlock(&un->mutex);
		}
	}

	notify_fs_position(&active_io->handle, 0, err);

	mutex_unlock(&active_io->mutex);
}
