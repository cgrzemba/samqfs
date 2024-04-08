/*
 * send_todo.c - routines for sending todo's to various processes.
 *
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
 * or https://illumos.org/license/CDDL.
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

#pragma ident "$Revision: 1.19 $"


#include <string.h>
#include <syslog.h>
#include <errno.h>

#include "aml/shm.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "sam/nl_samfs.h"


extern shm_alloc_t master_shm, preview_shm;

/*
 * send_robot_todo - send a todo request to a robot. Mutex on un should be
 * held when called but will be releases after incrementing the activity
 * count.  If exit with error, mutex is still held.
 *
 * If this is a stage TODO_ADD request and the device is still staging the todo
 * will not be sent and the stage request will be appended to the active
 * stage list.
 */
int				/* errno if error, else 0 */
send_robot_todo(
enum todo_sub sub_cmd,
equ_t robot_equ,	/* robot un */
equ_t device_equ,	/* device un */
enum callback callback,	/* callback function */
sam_handle_t *handle,	/* fs handle */
sam_resource_t *resource,	/* resource record */
void *addr)		/* generic address */
{
	dev_ent_t *device = NULL;
	dev_ptr_tbl_t *dev_ptr_tbl;
	register todo_request_t *todo;
	register message_request_t *message;

	dev_ptr_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->dev_table);

	if (dev_ptr_tbl->d_ent[robot_equ] == 0) {
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG,
			    "Robot device not present:%s:%d.\n",
			    __FILE__, __LINE__);
		send_fs_error(handle, ENXIO);
		return (ENXIO);
	}
	device = (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->d_ent[robot_equ]);
	if (!IS_ROBOT(device)) {
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG, "Not a robotic device:%s:%d.\n",
			    __FILE__, __LINE__);
		send_fs_error(handle, EINVAL);
		return (EINVAL);
	}
	if ((device->state < DEV_IDLE) &&
	    (device->status.b.ready && device->status.b.present)) {
		dev_ent_t *drive;

		drive = (dev_ptr_tbl->d_ent[device_equ] != 0) ?
		    (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->
		    d_ent[device_equ]) : NULL;

		INC_ACTIVE(drive);
		message = (message_request_t *)
		    SHM_REF_ADDR(device->dt.rb.message);
		mutex_unlock(&drive->mutex);
		mutex_lock(&message->mutex);
		while (message->mtype != MESS_MT_VOID)
			cond_wait(&message->cond_i, &message->mutex);

		message->message.magic = (uint_t)MESSAGE_MAGIC;
		message->message.command = MESS_CMD_TODO;
		message->mtype = MESS_MT_NORMAL;
		todo = &(message->message.param.todo_request);
		todo->sub_cmd = sub_cmd;
		todo->eq = device_equ;
		todo->mt_handle = addr;
		todo->callback = callback;
		todo->handle = *handle;
		todo->resource = *resource;
		cond_signal(&message->cond_r);
		mutex_unlock(&message->mutex);
		return (0);
	}
	send_fs_error(handle, EAGAIN);
	return (EAGAIN);
}

/*
 * send_tp_todo - send a todo request to a third party device. Mutex on un
 * should be held when called but will be releases after incrementing the
 * activity count.  If exit with error, mutex is still held.
 *
 */
int	/* 1 if error, else 0 */
send_tp_todo(
enum todo_sub sub_cmd,
dev_ent_t *un,
sam_handle_t *handle,		/* fs handle */
sam_resource_t *resource,	/* resource record */
enum callback callback)
{
	register todo_request_t *todo;
	register message_request_t *message;

	if ((un->state < DEV_IDLE) &&
	    (un->status.b.ready && un->status.b.present)) {

		INC_ACTIVE(un);
		message = (message_request_t *)SHM_REF_ADDR(un->dt.tr.message);
		mutex_unlock(&un->mutex);
		mutex_lock(&message->mutex);
		while (message->mtype != MESS_MT_VOID)
			cond_wait(&message->cond_i, &message->mutex);

		message->message.magic = (uint_t)MESSAGE_MAGIC;
		message->message.command = MESS_CMD_TODO;
		message->mtype = MESS_MT_NORMAL;
		todo = &(message->message.param.todo_request);
		todo->sub_cmd = sub_cmd;
		todo->eq = un->eq;
		todo->mt_handle = NULL;
		todo->callback = callback;
		todo->handle = *handle;
		todo->resource = *resource;
		cond_signal(&message->cond_r);
		mutex_unlock(&message->mutex);
		return (0);
	}
	send_fs_error(handle, EAGAIN);
	return (1);
}

/*
 * send_sp_todo - send a todo request to a remote sam pseudo device. Mutex on
 * un should be held when called but will be releases after incrementing the
 * activity count or notifying the file system of an error.
 */
void
send_sp_todo(
enum todo_sub sub_cmd,
dev_ent_t *un,		/* device */
sam_handle_t *handle,	/* fs handle */
sam_resource_t *resource,	/* resource record */
enum callback callback)	/* callback record */
{
	register todo_request_t *todo;
	register message_request_t *message;

	if ((un->state < DEV_IDLE) &&
	    (un->status.b.ready && un->status.b.present)) {
		if (sub_cmd != TODO_CANCEL)
			INC_ACTIVE(un);
		message = (message_request_t *)SHM_REF_ADDR(un->dt.sp.message);
		mutex_unlock(&un->mutex);
		mutex_lock(&message->mutex);
		while (message->mtype != MESS_MT_VOID)
			cond_wait(&message->cond_i, &message->mutex);

		message->message.magic = (uint_t)MESSAGE_MAGIC;
		message->message.command = MESS_CMD_TODO;
		message->mtype = MESS_MT_NORMAL;
		todo = &(message->message.param.todo_request);
		todo->sub_cmd = sub_cmd;
		todo->eq = un->eq;
		todo->mt_handle = NULL;
		todo->callback = callback;
		todo->handle = *handle;
		todo->resource = *resource;
		cond_signal(&message->cond_r);
		mutex_unlock(&message->mutex);
		return;
	}
	send_fs_error(handle, EAGAIN);
	mutex_unlock(&un->mutex);
}

/*
 * send_scanner_todo - send a todo request to the scanner. Mutex on un should
 * be held when called it will be released if the activity count is
 * incremented.
 *
 * If this is a stage TODO_ADD request and the device is still staging the todo
 * will not be sent and the stage request will be appended to the active
 * stage list.
 */

int	/* errno if error, 0 if OK */
send_scanner_todo(
enum todo_sub sub_cmd,
dev_ent_t *un,
enum callback callback,		/* callback code */
sam_handle_t *handle,		/* handle */
sam_resource_t *resource,	/* resource record */
void *addr)
{
	register todo_request_t *todo;
	register message_request_t *message;


	if ((un->state < DEV_IDLE) &&
	    (un->status.b.ready && un->status.b.present)) {

		INC_ACTIVE(un);
		message = (message_request_t *)SHM_REF_ADDR(
		    ((shm_ptr_tbl_t *)master_shm.shared_memory)->scan_mess);
		mutex_unlock(&un->mutex);
		mutex_lock(&message->mutex);
		while (message->mtype != MESS_MT_VOID)
			cond_wait(&message->cond_i, &message->mutex);

		message->message.magic = (uint_t)MESSAGE_MAGIC;
		message->message.command = MESS_CMD_TODO;
		message->mtype = MESS_MT_NORMAL;
		todo = &(message->message.param.todo_request);
		todo->sub_cmd = sub_cmd;
		todo->eq = un->eq;
		todo->mt_handle = addr;
		todo->callback = callback;
		todo->handle = *handle;
		todo->resource = *resource;
		cond_signal(&message->cond_r);
		mutex_unlock(&message->mutex);
		return (0);
	}
	send_fs_error(handle, EAGAIN);
	return (EAGAIN);
}
