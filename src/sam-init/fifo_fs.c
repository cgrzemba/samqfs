/*
 * fifo_fs.c - thread routines for processing file system commands.
 *
 * Solaris 2.x Sun Storage & Archiving Management File System
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

#pragma ident "$Revision: 1.37 $"

/* ANSI C headers. */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* POSIX headers. */
#include <thread.h>
#include <synch.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/vfs.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/param.h"
#include "sam/lib.h"
#include "aml/shm.h"
#include "aml/archiver.h"
#include "aml/device.h"
#include "aml/fifo.h"
#include "sam/uioctl.h"
#include "aml/proto.h"
#include "aml/preview.h"
#include "sam/resource.h"
#include "sam/nl_samfs.h"
#include "sam/syscall.h"
#include "sam/fs/sblk.h"

/* Local headers. */
#include "amld.h"


/*
 * load_for_fs - process a load request for the removable media file.
 *
 * Load the media into the drive and return the removable media information
 * to the file system.
 */
void *
load_for_fs(
	void *vcmd)
{
	fs_load_t *command;
	sam_fs_fifo_t *fifo_cmd = (sam_fs_fifo_t *)vcmd;
	int	err;

	command = &fifo_cmd->param.fs_load;

	if (LicenseExpired || SlotsUsedUp) {
		send_fs_error(&fifo_cmd->handle, ENOTSUP);
	} else if ((err = add_preview_fs(&fifo_cmd->handle, &command->resource,
	    PRV_FS_PRIO_DEFAULT, CB_NOTIFY_FS_LOAD))) {
		send_fs_error(&fifo_cmd->handle, err);
	}
	free(vcmd);			/* always free the command buffer */
	thr_exit(NULL);
/* LINTED Function has no return statement */
}


/*
 *  position_rmedia_for_fs - position request for removable media file.
 */
void *
position_rmedia_for_fs(
	void *vcmd)
{
	sam_fs_fifo_t *fifo_cmd = (sam_fs_fifo_t *)vcmd;
	fs_position_t *command;
	dev_ent_t *device;
	int		err;

	command = &fifo_cmd->param.fs_position;

	if (LicenseExpired || SlotsUsedUp) {
		send_fs_error(&fifo_cmd->handle, ENOTSUP);
	} else {
#if 0
		err = add_preview_fs(&fifo_cmd->handle, &command->resource,
		    PRV_FS_PRIO_DEFAULT, CB_POSITION_RMEDIA);
#endif

		for (device = (dev_ent_t *)SHM_REF_ADDR(
		    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);
		    device != NULL;
		    device = (dev_ent_t *)SHM_REF_ADDR(device->next)) {
			if (device->st_rdev == command->rdev) {
				break;
			}
		}

		if (device) {
			sam_act_io_t *active_io;

			mutex_lock(&device->mutex);
			active_io =
			    (sam_act_io_t *)SHM_REF_ADDR(device->active_io);
			mutex_lock(&active_io->mutex);
			active_io->setpos = command->setpos;
			active_io->handle.fifo_cmd = fifo_cmd->handle.fifo_cmd;
			mutex_unlock(&active_io->mutex);
			mutex_unlock(&device->mutex);

			if (device->fseq == 0) {
				err = send_scanner_todo(TODO_ADD, device,
				    CB_POSITION_RMEDIA, &fifo_cmd->handle,
				    &command->resource, NULL);
			} else {
				err = send_robot_todo(TODO_ADD, device->fseq,
				    device->eq, CB_POSITION_RMEDIA,
				    &fifo_cmd->handle, &command->resource,
				    NULL);
			}
			if (err) {
				mutex_unlock(&device->mutex);
			}
		} else {
			err = EIO;
		}

		if (err)
			send_fs_error(&fifo_cmd->handle, err);
	}

	free(vcmd);			/* always free the command buffer */
	thr_exit(NULL);
/* LINTED Function has no return statement */
}


/*
 * unload_for_fs - unload processing
 *
 * Send todo request to the device.
 */
void *
unload_for_fs(void *vcmd)
{
	dev_ent_t *device;
	fs_unload_t *command;
	sam_fs_fifo_t *fifo_cmd = (sam_fs_fifo_t *)vcmd;

	if (LicenseExpired || SlotsUsedUp) {
		send_fs_error(&fifo_cmd->handle, ENOTSUP);
	} else {
		command = &fifo_cmd->param.fs_unload;

		/* find the device */

		for (device = (dev_ent_t *)SHM_REF_ADDR(((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);
		    device != NULL;
		    device = (dev_ent_t *)SHM_REF_ADDR(device->next)) {
			if (device->st_rdev == command->rdev) {
				sam_syslog(LOG_DEBUG,
			    "unload_for_fs: found rdev (%#x) %#x %x", command->rdev, device->st_rdev, device);
				break;
			}
		}

		if (device == NULL) {
			sam_syslog(LOG_ERR,
			    "unload_for_fs: bad rdev (%#x)", command->rdev);
			send_fs_error(&fifo_cmd->handle, EIO);
		} else {
			/*
			 * unload_media - signal the thread doing io that an
			 * unload request has arrived.
			 */
			sam_act_io_t *active_io;

			mutex_lock(&device->mutex);
			active_io =
			    (sam_act_io_t *)SHM_REF_ADDR(device->active_io);
			mutex_lock(&active_io->mutex);
			active_io->resource = command->resource;
			active_io->final_io_count = command->io_count;
			active_io->wait_fs_unload = FALSE;

			/*
			 * If io timed out, then we must notify the file
			 * system because the device is hung up waiting for
			 * the umount.
			 */
			if (active_io->timeout &&
			    command->resource.archive.rm_info.bof_written) {
				notify_fs_unload(&fifo_cmd->handle,
				    (u_longlong_t)0, 0);
			}

			/*
			 * Make the fifo_cmd the same in the shared memory
			 * segment if the inodes match.
			 */
			if (active_io->handle.id.ino ==
			    fifo_cmd->handle.id.ino) {
				active_io->handle.fifo_cmd =
				    fifo_cmd->handle.fifo_cmd;
			}
			cond_signal(&active_io->cond);
			mutex_unlock(&active_io->mutex);
			mutex_unlock(&device->mutex);

#if defined(SEND_UNLOAD_TOTO)
			if (device->fseq == 0) {
				err = send_scanner_todo(TODO_UNLOAD, device,
				    CB_NOTIFY_FS_ULOAD, &fifo_cmd->handle,
				    &command->resource, command->mt_handle);
			} else {
				err = send_robot_todo(TODO_UNLOAD, device->fseq,
				    device->eq, CB_NOTIFY_FS_ULOAD,
				    &fifo_cmd->handle, &command->resource,
				    command->mt_handle);
			}
			if (err) {
				mutex_unlock(&device->mutex);
			}
#endif
		}
	}

	free(vcmd);			/* always free the command buffer */
	thr_exit(NULL);
/* LINTED Function has no return statement */
}


/*
 * cancel_for_fs - cancel a file system request
 *
 * Invoked by the file system when a user action(typically an interrupt)
 * terminates a request.
 */
void *
cancel_for_fs(void *vcmd)
{
	sam_fs_fifo_t *command = (sam_fs_fifo_t *)vcmd;

	switch (command->param.fs_cancel.cmd) {
	case FS_FIFO_LOAD:
		remove_preview_handle(command, CB_NOTIFY_FS_LOAD);
		break;

	case FS_FIFO_UNLOAD:
		break;

	case FS_FIFO_CANCEL:
		break;

	}
	free(vcmd);			/* always free the command buffer */
	thr_exit(NULL);
/* LINTED Function has no return statement */
}
