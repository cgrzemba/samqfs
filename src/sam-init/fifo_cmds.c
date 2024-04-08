/*
 * fifo_cmd.c - thread routines for processing commands.
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

#pragma ident "$Revision: 1.35 $"

/* ANSI C headers. */
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <thread.h>
#include <synch.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/param.h"
#include "sam/lib.h"
#include "aml/shm.h"
#include "aml/fifo.h"
#include "sam/defaults.h"
#include "aml/message.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"
#include "sam/syscall.h"

/* Local headers. */
#include "amld.h"

#if defined(lint)
#undef mutex_lock
#endif /* defined(lint) */

/* Macro to delete training blanks in string a of length l */
#define	DTB(a, l) { uchar_t *t; for (t = ((a)+((l)-1)); \
	(t >= (a))&&(((*t) == 0x20)||((*t) == 0x00)); t--) (*t) = 0x00; }

/* DEV_ENT - given an equipment ordinal, return the dev_ent */
#define	DEV_ENT(a) ((dev_ent_t *)SHM_REF_ADDR(((dev_ptr_tbl_t *)SHM_REF_ADDR( \
	((shm_ptr_tbl_t *)master_shm.shared_memory)->dev_table))->d_ent[(a)]))


/*
 * unload_cmd.  Thread version of unload command.
 * This is used in the main loop cannot get the mutex.  Instead of waiting
 * around, it called this routine.
 */
void *
unload_cmd(
	void *vdev)
{
	dev_ent_t *device = (dev_ent_t *)vdev;

	if (mutex_lock(&device->mutex) != 0) {
		if (device->status.b.ready) {
			device->status.b.unload = TRUE;
		}
		mutex_unlock(&device->mutex);
	}
	thr_exit(NULL);
/* LINTED Function has no return statement */
}


/*
 * tell robot to start label process.  Runs as a thread.
 * Programming note:  The parameter passed in was malloc'ed, be sure
 * to free it before thr_exit.
 */
void *
robot_label(
	void *vcmd)
{
	sam_cmd_fifo_t *command = (sam_cmd_fifo_t *)vcmd;
	sam_defaults_t *defaults;
	dev_ent_t *device;
	message_request_t *message;

	/* equipment was verified in the caller */
	device = DEV_ENT(command->eq);

	message = (message_request_t *)SHM_REF_ADDR(device->dt.rb.message);
	defaults = GetDefaults();

	set_media_to_default((media_t *)& command->media, defaults);

	if (IS_ROBOT(device) &&
	    (device->status.bits & (DVST_READY | DVST_PRESENT))) {
		DTB((uchar_t *)command->vsn, sizeof (vsn_t));
		(void) mutex_lock(&message->mutex);
		while (message->mtype != MESS_MT_VOID) {
			cond_wait(&message->cond_i, &message->mutex);
		}

		memset(&message->message, 0, sizeof (sam_message_t));
		message->message.magic = MESSAGE_MAGIC;
		message->message.exit_id = command->exit_id;
		message->message.command = MESS_CMD_LABEL;
		message->message.param.label_request.flags = command->flags;
		message->message.param.label_request.slot = command->slot;
		message->message.param.label_request.part = command->part;
		message->message.param.label_request.media = command->media;
		message->message.param.label_request.block_size =
		    command->block_size;
		memmove(&(message->message.param.label_request.vsn),
		    &(command->vsn), sizeof (vsn_t));
		memmove(&(message->message.param.label_request.old_vsn),
		    &(command->old_vsn), sizeof (vsn_t));
		memmove(&(message->message.param.label_request.info),
		    &(command->info), 127);

		message->mtype = MESS_MT_NORMAL;
		cond_signal(&message->cond_r);	/* wake up robot */
		mutex_unlock(&message->mutex);
	} else {
		char *msg2;
		char *msg1 = catgets(catfd, SET, 20008,
		    "Slot label only valid on active robots (eq %d)");
		/* allow some room for the equipment number to expand */
		int		len;

		msg2 = (char *)malloc_wait((len = (strlen(msg1) + 20)), 4, 0);
		memset(msg2, 0, len);
		sprintf(msg2, msg1, device->eq);
		write_client_exit_string(&command->exit_id, EXIT_FAILED, msg2);
		sam_syslog(LOG_INFO, msg2);
		free(msg2);
	}
	free(command);			/* free the command buffer */
	thr_exit(NULL);
/* LINTED Function has no return statement */
}


/*
 * tell scanner to start label process.  Runs as a thread.
 *
 * Programming note:  The parameter passed in was malloc'ed, be sure
 * to free it before thr_exit.
 */
void *
scanner_label(
	void *vcmd)
{
	sam_cmd_fifo_t *command = (sam_cmd_fifo_t *)vcmd;
	sam_defaults_t *defaults;
	dev_ent_t *device;
	message_request_t *message;

	/* equipment was verified in the caller */
	device = DEV_ENT(command->eq);
	message = (message_request_t *)SHM_REF_ADDR
	    (((shm_ptr_tbl_t *)master_shm.shared_memory)->scan_mess);
	defaults = GetDefaults();

	set_media_to_default((media_t *)& command->media, defaults);

	if (!(device->status.bits & DVST_SCANNING) &&
	    (device->status.bits & (DVST_READY | DVST_PRESENT))) {
		DTB((uchar_t *)command->vsn, sizeof (vsn_t));

		(void) mutex_lock(&message->mutex);
		while (message->mtype != MESS_MT_VOID)
			cond_wait(&message->cond_i, &message->mutex);

		(void) memset(&message->message, 0, sizeof (sam_message_t));
		message->message.magic = MESSAGE_MAGIC;
		message->message.exit_id = command->exit_id;
		message->message.command = MESS_CMD_LABEL;
		message->message.param.label_request.flags = command->flags;
		message->message.param.label_request.slot = command->eq;
		message->message.param.label_request.part = command->part;
		message->message.param.label_request.media = command->media;
		message->message.param.label_request.block_size =
		    command->block_size;

		memmove(&(message->message.param.label_request.vsn),
		    &(command->vsn), sizeof (vsn_t));
		memmove(&(message->message.param.label_request.old_vsn),
		    &(command->old_vsn), sizeof (vsn_t));
		memmove(&(message->message.param.label_request.info),
		    &(command->info), 127);
		message->mtype = MESS_MT_NORMAL;
		cond_signal(&message->cond_r);	/* wake up robot */
		mutex_unlock(&message->mutex);
	} else {
		char *msg1, *msg2;
		int		 len;

		msg1 = catgets(catfd, SET, 20009,
		    "Device not ready for labeling (eq %d)");
		/* allow some room for the equipment number to expand */
		msg2 = (char *)malloc_wait((len = (strlen(msg1) + 20)), 4, 0);
		memset(msg2, 0, len);
		sprintf(msg2, msg1, device->eq);
		write_client_exit_string(&command->exit_id, EXIT_FAILED, msg2);
		sam_syslog(LOG_WARNING, msg2);
		free(msg2);
	}

	free(command);			/* free the command buffer */
	thr_exit(NULL);
/* LINTED Function has no return statement */
}


/*
 * tell robot to unload library or drive.  Runs as a thread.
 *
 * Programming note:  The parameter passed in was malloc'ed, be sure
 * to free it before thr_exit.
 */
void *
robot_unload(
	void *vcmd)
{
	sam_cmd_fifo_t *command = (sam_cmd_fifo_t *)vcmd;
	dev_ent_t *device;

	/* all commands are sent to the robot, get robots device entry */
	device = DEV_ENT(DEV_ENT(command->eq)->fseq);

	if (IS_ROBOT(device) &&
	    (device->status.b.ready && device->status.b.present)) {
		message_request_t *message;

		message =
		    (message_request_t *)SHM_REF_ADDR(device->dt.rb.message);
		(void) mutex_lock(&message->mutex);
		while (message->mtype != MESS_MT_VOID) {
			cond_wait(&message->cond_i, &message->mutex);
		}
		memset(&message->message, 0, sizeof (sam_message_t));
		message->message.magic = MESSAGE_MAGIC;
		message->message.command = MESS_CMD_UNLOAD;
		message->message.param.unload_request.flags = command->flags;
		if ((command->flags & CMD_UNLOAD_SLOT) &&
		    (command->slot != ROBOT_NO_SLOT)) {
			message->message.param.unload_request.slot =
			    command->slot;
		} else {
			message->message.param.unload_request.slot =
			    (uint_t)ROBOT_NO_SLOT;
			message->message.param.unload_request.flags &=
			    ~CMD_UNLOAD_SLOT;
		}

		message->message.exit_id = command->exit_id;
		message->message.param.unload_request.eq = command->eq;
		message->mtype = MESS_MT_NORMAL;
		cond_signal(&message->cond_r);
		mutex_unlock(&message->mutex);
	}
	free(command);			/* free the command buffer */
	thr_exit(NULL);
/* LINTED Function has no return statement */
}


/*
 * tell robot to mount media by slot.  Runs as a thread.
 *
 * Programming note:  The parameter passed in was malloc'ed, be sure
 * to free it before thr_exit.
 */
void *
mount_slot(
	void *vcmd)
{
	sam_cmd_fifo_t *command = (sam_cmd_fifo_t *)vcmd;
	message_request_t *message;

	/* valid equipment was verified in the caller */
	message = (message_request_t *)SHM_REF_ADDR(
	    (DEV_ENT(command->eq))->dt.rb.message);

	(void) mutex_lock(&message->mutex);
	while (message->mtype != MESS_MT_VOID) {
		cond_wait(&message->cond_i, &message->mutex);
	}
	(void) memset(&message->message, 0, sizeof (sam_message_t));
	message->message.magic = MESSAGE_MAGIC;
	message->message.command = MESS_CMD_MOUNT;
	message->message.exit_id = command->exit_id;
	message->message.param.mount_request.flags = command->flags;
	message->message.param.mount_request.slot = command->slot;
	message->message.param.mount_request.part = command->part;
	message->mtype = MESS_MT_NORMAL;
	cond_signal(&message->cond_r);
	mutex_unlock(&message->mutex);
	free(command);			/* free the command buffer */
	thr_exit(NULL);
/* LINTED Function has no return statement */
}


/*
 * tell robot to clean a drive.  Runs as a thread.
 *
 * Programming note:  The parameter passed in was malloc'ed, be sure
 * to free it before thr_exit.
 */
void *
clean_drive(
	void *vcmd)
{
	sam_cmd_fifo_t *command = (sam_cmd_fifo_t *)vcmd;
	dev_ent_t *device;

	device = DEV_ENT(DEV_ENT(command->eq)->fseq);

	if (IS_ROBOT(device) && device->state < DEV_IDLE) {
		message_request_t *message;

		message =
		    (message_request_t *)SHM_REF_ADDR(device->dt.rb.message);
		(void) mutex_lock(&message->mutex);
		while (message->mtype != MESS_MT_VOID) {
			cond_wait(&message->cond_i, &message->mutex);
		}
		memset(&message->message, 0, sizeof (sam_message_t));
		message->message.magic = MESSAGE_MAGIC;
		message->message.command = MESS_CMD_CLEAN;
		message->message.param.clean_request.eq = command->eq;
		message->mtype = MESS_MT_NORMAL;
		cond_signal(&message->cond_r);
		mutex_unlock(&message->mutex);
	}
	free(command);			/* free the command buffer */
	thr_exit(NULL);
/* LINTED Function has no return statement */
}


/*
 * set new state for a device.
 *
 * Programming note:  The parameter passed in was malloc'ed, be sure
 * to free it before thr_exit.
 */
void *
set_state(
	void *vcmd)
{
	int old_state;
	sam_cmd_fifo_t *command = (sam_cmd_fifo_t *)vcmd;
	dev_ent_t *device, *robot = NULL;
	message_request_t *message = NULL;

	/* equipment was verified in the caller */

	device = DEV_ENT(command->eq);
	if (device->fseq) {
		robot = DEV_ENT(device->fseq);
	}
	if (command->state >= DEV_ON && command->state <= DEV_DOWN &&
	    command->state != DEV_NOALLOC) {
		old_state = device->state;
		if (robot != NULL) {
			device = robot;
			if (IS_ROBOT(device) && device->status.b.present) {
				message = (message_request_t *)SHM_REF_ADDR(
				    device->dt.rb.message);
			}
		} else if (IS_OPTICAL(device) || IS_TAPE(device)) {
			/* send to scanner */
			message = (message_request_t *)SHM_REF_ADDR(
			    ((shm_ptr_tbl_t *)master_shm.shared_memory)->
			    scan_mess);
		}
		if (message != NULL) {
			(void) mutex_lock(&message->mutex);
			while (message->mtype != MESS_MT_VOID) {
				cond_wait(&message->cond_i, &message->mutex);
			}
			memset(&message->message, 0, sizeof (sam_message_t));
			message->message.magic = MESSAGE_MAGIC;
			message->message.command = MESS_CMD_STATE;
			message->message.exit_id = command->exit_id;
			message->message.param.state_change.flags =
			    command->flags;
			message->message.param.state_change.eq = command->eq;
			message->message.param.state_change.old_state =
			    old_state;
			message->message.param.state_change.state =
			    command->state;
			message->mtype = MESS_MT_NORMAL;
			cond_signal(&message->cond_r);
			mutex_unlock(&message->mutex);
		}
	}
	free(command);			/* free the command buffer */
	thr_exit(NULL);
/* LINTED Function has no return statement */
}


/*
 * start an audit of a library or slot in library
 *
 * Programming note:  The parameter passed in was malloc'ed, be sure
 * to free it before thr_exit.
 */
void *
fifo_cmd_start_audit(
	void *vcmd)
{
	sam_cmd_fifo_t *command = (sam_cmd_fifo_t *)vcmd;
	dev_ent_t *device;

	/* equipment was verified in the caller */
	device = DEV_ENT(command->eq);

	/* Must be a member of a family set or an entire robot */
	if (device->fseq) {
		message_request_t *message;

		device = DEV_ENT(device->fseq);
		message =
		    (message_request_t *)SHM_REF_ADDR(device->dt.rb.message);
		if (IS_ROBOT(device) &&
		    (device->status.b.ready && device->status.b.present)) {
			(void) mutex_lock(&message->mutex);
			while (message->mtype != MESS_MT_VOID) {
				cond_wait(&message->cond_i, &message->mutex);
			}
			memset(&message->message, 0, sizeof (sam_message_t));
			message->message.magic = MESSAGE_MAGIC;
			message->message.command = MESS_CMD_AUDIT;
			message->message.exit_id = command->exit_id;
			message->message.param.audit_request.flags =
			    command->flags;
			message->message.param.audit_request.eq = command->eq;
			message->message.param.audit_request.slot =
			    command->slot;
			message->mtype = MESS_MT_NORMAL;
			cond_signal(&message->cond_r);
			mutex_unlock(&message->mutex);
		}
	}
	free(command);			/* free the command buffer */
	thr_exit(NULL);
/* LINTED Function has no return statement */
}


/*
 * export media from a library
 *
 * Programming note:  The parameter passed in was malloc'ed, be sure
 * to free it before thr_exit.
 */
void *
fifo_cmd_export_media(
	void *vcmd)
{
	sam_cmd_fifo_t *command = (sam_cmd_fifo_t *)vcmd;
	sam_defaults_t *defaults;
	export_request_t *request;
	dev_ent_t *device;

	/* equipment was verified in the caller */
	device = DEV_ENT(command->eq);

	defaults = GetDefaults();

	/* Must be a member of a family set or an entire robot */
	if (device->fseq) {
		message_request_t *message;

		/* The command is always sent to the robot */
		device = DEV_ENT(device->fseq);
		message =
		    (message_request_t *)SHM_REF_ADDR(device->dt.rb.message);
		request = &message->message.param.export_request;

		if (IS_ROBOT(device) &&
		    (device->status.b.ready && device->status.b.present)) {
			boolean_t issue = TRUE;

			(void) mutex_lock(&message->mutex);
			while (message->mtype != MESS_MT_VOID) {
				cond_wait(&message->cond_i, &message->mutex);
			}
			memset(&message->message, 0, sizeof (sam_message_t));
			message->message.magic = MESSAGE_MAGIC;
			message->message.command = MESS_CMD_EXPORT;
			message->message.exit_id = command->exit_id;
			request->flags = command->flags;
			request->eq = command->eq;
			request->flags &= ~EXPORT_FLAG_MASK;
			switch (command->cmd) {
			case CMD_FIFO_REMOVE_V:
				memmove(&request->vsn, &command->vsn, 32);
				set_media_to_default(
				    (media_t *)&command->media, defaults);
				request->media = command->media;
				request->slot = (uint_t)ROBOT_NO_SLOT;
				request->flags |= EXPORT_BY_VSN;
				break;

			case CMD_FIFO_REMOVE_S:
				request->slot = command->slot;
				request->flags |= EXPORT_BY_SLOT;
				break;

			case CMD_FIFO_REMOVE_E:
				request->eq = command->eq;
				request->slot = (uint_t)ROBOT_NO_SLOT;
				request->flags |= EXPORT_BY_EQ;
				break;

			default:
				sam_syslog(LOG_ERR,
				    "fifo_cmd_export_media: unknown switch"
				    "(%#x): %s:%s\n",
				    command->cmd, __FILE__, __LINE__);
				issue = FALSE;
				break;
			}

			if (issue) {
				message->mtype = MESS_MT_NORMAL;
			} else {
				message->mtype = MESS_MT_VOID;
				message->message.exit_id.pid = 0;
			}

			cond_signal(&message->cond_r);
			mutex_unlock(&message->mutex);
		}
	}
	free(command);			/* free the command buffer */
	thr_exit(NULL);
/* LINTED Function has no return statement */
}

/*
 * perform tapealert action for robot or tape drive
 *
 * Programming note:  The parameter passed in was malloc'ed, be sure
 * to free it before thr_exit.
 */
void *
fifo_cmd_tapealert(
	void *vcmd)
{
	sam_cmd_fifo_t *command = (sam_cmd_fifo_t *)vcmd;
	dev_ent_t *device, *robot = NULL;
	message_request_t *message = NULL;

	/* equipment was verified in the caller */

	device = DEV_ENT(command->eq);
	if (device->fseq) {
		robot = DEV_ENT(device->fseq);
	}
	if (robot != NULL) {
		device = robot;
		if (IS_ROBOT(device) && device->status.b.present) {
			/* send to robot */
			message = (message_request_t *)SHM_REF_ADDR(
			    device->dt.rb.message);
		}
	} else if (IS_TAPE(device)) {
		/* send to scanner */
		message = (message_request_t *)SHM_REF_ADDR(
		    ((shm_ptr_tbl_t *)master_shm.shared_memory)->scan_mess);
	}
	if (message != NULL) {
		(void) mutex_lock(&message->mutex);
		while (message->mtype != MESS_MT_VOID) {
			cond_wait(&message->cond_i, &message->mutex);
		}
		memset(&message->message, 0, sizeof (sam_message_t));
		message->message.magic = MESSAGE_MAGIC;
		message->message.command = MESS_CMD_TAPEALERT;
		message->message.exit_id = command->exit_id;
		message->message.param.tapealert_request.flags = command->flags;
		message->message.param.tapealert_request.eq = command->eq;
		message->mtype = MESS_MT_NORMAL;
		cond_signal(&message->cond_r);
		mutex_unlock(&message->mutex);
	}
	free(command);			/* free the command buffer */
	thr_exit(NULL);
/* LINTED Function has no return statement */
}

/*
 * perform tapealert action for robot or tape drive
 *
 * Programming note:  The parameter passed in was malloc'ed, be sure
 * to free it before thr_exit.
 */
void *
fifo_cmd_sef(
	void *vcmd)
{
	sam_cmd_fifo_t *command = (sam_cmd_fifo_t *)vcmd;
	dev_ent_t *device, *robot = NULL;
	message_request_t *message = NULL;

	/* equipment was verified in the caller */

	device = DEV_ENT(command->eq);
	if (device->fseq) {
		robot = DEV_ENT(device->fseq);
	}
	if (robot != NULL) {
		device = robot;
		if (IS_ROBOT(device) && device->status.b.present) {
			/* send to robot */
			message = (message_request_t *)SHM_REF_ADDR(
			    device->dt.rb.message);
		}
	} else if (IS_TAPE(device)) {
		/* send to scanner */
		message = (message_request_t *)SHM_REF_ADDR(
		    ((shm_ptr_tbl_t *)master_shm.shared_memory)->scan_mess);
	}
	if (message != NULL) {
		(void) mutex_lock(&message->mutex);
		while (message->mtype != MESS_MT_VOID) {
			cond_wait(&message->cond_i, &message->mutex);
		}
		memset(&message->message, 0, sizeof (sam_message_t));
		message->message.magic = MESSAGE_MAGIC;
		message->message.command = MESS_CMD_SEF;
		message->message.exit_id = command->exit_id;
		message->message.param.sef_request.flags = command->flags;
		message->message.param.sef_request.eq = command->eq;
		message->message.param.sef_request.interval =
		    (int)command->value;
		message->mtype = MESS_MT_NORMAL;
		cond_signal(&message->cond_r);
		mutex_unlock(&message->mutex);
	}
	free(command);			/* free the command buffer */
	thr_exit(NULL);
/* LINTED Function has no return statement */
}

/*
 * import media into a library
 *
 * Programming note:  The parameter passed in was malloc'ed, be sure
 * to free it before thr_exit.
 */
void *
fifo_cmd_import_media(
	void *vcmd)
{
	sam_cmd_fifo_t *command = (sam_cmd_fifo_t *)vcmd;
	import_request_t *request;
	dev_ent_t *device;

	/* equipment was verified in the caller */
	device = DEV_ENT(command->eq);
	/* Must be a member of a family set or an entire robot */
	if (device->fseq) {
		message_request_t *message;

		device = DEV_ENT(device->fseq);
		message =
		    (message_request_t *)SHM_REF_ADDR(device->dt.rb.message);
		request = &message->message.param.import_request;

		if (IS_ROBOT(device) &&
		    (device->status.b.ready && device->status.b.present)) {
			(void) mutex_lock(&message->mutex);
			while (message->mtype != MESS_MT_VOID) {
				cond_wait(&message->cond_i, &message->mutex);
			}
			memset(&message->message, 0, sizeof (sam_message_t));
			message->message.magic = MESSAGE_MAGIC;
			message->message.command = MESS_CMD_IMPORT;
			message->message.exit_id = command->exit_id;
			request->flags = command->flags;
			message->mtype = MESS_MT_NORMAL;
			cond_signal(&message->cond_r);
			mutex_unlock(&message->mutex);
		}
	}
	free(command);			/* free the command buffer */
	thr_exit(NULL);
/* LINTED Function has no return statement */
}


/*
 * add catalog command
 *
 * Programming note:  The parameter passed in was malloc'ed, be sure
 * to free it before thr_exit.
 */
void *
add_catalog_cmd(
	void *vcmd)
{
	sam_cmd_fifo_t *command = (sam_cmd_fifo_t *)vcmd;
	dev_ent_t *device;
	message_request_t *message;

	if (command->vsn[0] == '\0' && !(command->flags & ADDCAT_BARCODE)) {
		sam_syslog(LOG_INFO,
		    catgets(catfd, SET, 11008,
		    "Add catalog, vsn not supplied."));
	} else {
		device = DEV_ENT(DEV_ENT(command->eq)->fseq);
		if ((IS_GENERIC_API(device->type) ||
		    device->type == DT_STKAPI ||
		    device->type == DT_SONYPSC ||
		    device->type == DT_IBMATL) ||
		    device->type == DT_HISTORIAN &&
		    (device->state < DEV_IDLE) &&
		    (device->status.b.ready && device->status.b.present)) {
			message = (message_request_t *)
			    SHM_REF_ADDR(device->dt.rb.message);
			(void) mutex_lock(&message->mutex);
			while (message->mtype != MESS_MT_VOID) {
				cond_wait(&message->cond_i, &message->mutex);
			}
			memset(&message->message, 0, sizeof (sam_message_t));
			message->message.magic = MESSAGE_MAGIC;
			message->message.command = MESS_CMD_ADD;
			message->message.exit_id = command->exit_id;
			message->mtype = MESS_MT_NORMAL;
			message->message.param.addcat_request.media =
			    command->media;
			message->message.param.addcat_request.flags =
			    command->flags;
			memmove(&message->message.param.addcat_request.vsn[0],
			    &command->vsn[0], sizeof (vsn_t));
			if (command->flags & ADDCAT_BARCODE) {
				memccpy(&message->
				    message.param.addcat_request.bar_code[0],
				    &command->info[0], '\0', BARCODE_LEN);
			}
			cond_signal(&message->cond_r);
			mutex_unlock(&message->mutex);
		}
	}

	free(command);			/* free the command buffer */
	thr_exit(NULL);
/* LINTED Function has no return statement */
}


/*
 * tell robot to move media between two slots.  Runs as a thread.
 *
 * Programming note:  The parameter passed in was malloc'ed, be sure
 * to free it before thr_exit.
 */
void *
move_slots(
	void *vcmd)
{
	sam_cmd_fifo_t *command = (sam_cmd_fifo_t *)vcmd;
	dev_ent_t *device;

	/* all commands are sent to the robot, get robots device entry */
	device = DEV_ENT(DEV_ENT(command->eq)->fseq);

	if (IS_ROBOT(device) && (device->status.b.ready &&
	    device->status.b.present)) {
		message_request_t *message;

		message =
		    (message_request_t *)SHM_REF_ADDR(device->dt.rb.message);
		(void) mutex_lock(&message->mutex);
		while (message->mtype != MESS_MT_VOID) {
			cond_wait(&message->cond_i, &message->mutex);
		}
		memset(&message->message, 0, sizeof (sam_message_t));
		message->message.magic = MESSAGE_MAGIC;
		message->message.command = MESS_CMD_MOVE;
		message->message.exit_id = command->exit_id;
		message->message.param.cmdmove_request.flags = command->flags;
		message->message.param.cmdmove_request.slot = command->slot;
		message->message.param.cmdmove_request.d_slot = command->d_slot;
		message->mtype = MESS_MT_NORMAL;
		cond_signal(&message->cond_r);
		mutex_unlock(&message->mutex);
	}
	free(command);			/* free the command buffer */
	thr_exit(NULL);
/* LINTED Function has no return statement */
}


/*
 * tell robot to load media on unavail drive.  Runs as a thread.
 *
 * Programming note:  The parameter passed in was malloc'ed, be sure
 * to free it before thr_exit.
 */
void *
fifo_cmd_load_unavail(
	void *vcmd)
{
	sam_cmd_fifo_t *command = (sam_cmd_fifo_t *)vcmd;
	dev_ent_t *device;

	/* all commands are sent to the robot, get robots device entry */
	device = DEV_ENT(DEV_ENT(command->eq)->fseq);

	if (IS_ROBOT(device) &&
	    (device->status.b.ready && device->status.b.present)) {
		message_request_t *message;
		load_u_request_t *load_req;

		message =
		    (message_request_t *)SHM_REF_ADDR(device->dt.rb.message);
		load_req = &message->message.param.load_u_request;

		(void) mutex_lock(&message->mutex);
		while (message->mtype != MESS_MT_VOID) {
			cond_wait(&message->cond_i, &message->mutex);
		}
		memset(&message->message, 0, sizeof (sam_message_t));
		message->message.magic = MESSAGE_MAGIC;
		message->message.command = MESS_CMD_LOAD_UNAVAIL;
		message->message.exit_id = command->exit_id;
		load_req->slot = command->slot;
		load_req->part = command->part;
		load_req->flags = command->flags;
		load_req->media = command->media;
		load_req->eq = command->eq;
		memmove(&(load_req->vsn), &(command->vsn), sizeof (vsn_t));
		message->mtype = MESS_MT_NORMAL;
		cond_signal(&message->cond_r);
		mutex_unlock(&message->mutex);
	}
	free(command);			/* free the command buffer */
	thr_exit(NULL);
/* LINTED Function has no return statement */
}
