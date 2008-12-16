/*
 * ----- sam/amld.c - Process the automatic media library daemon
 *                   communication functions.
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

#pragma ident "$Revision: 1.25 $"

/*
 *	This is the interface module that sits between the file system and
 *	sam-amld.  It encapsulates the logic that moves commands from the
 *	file system to sam-amld.  The logic that responds to commands sent
 *	to sam-amld is (for now) outside of this module.  This module
 *	does not read or modify any data outside of this module, nor does
 *	it allow anything outside of this module to read or modify data
 *	internal to this module (with the exception of system services, such
 *	as mutex's, cv's, and lbolt).
 *
 *	Externally callable routines are as follows:
 *
 *	sam_sync_amld(args)
 *		This routine sends the SYNC and INIT commands to sam-amld.
 *		This is to resync sam-amld with the FS when sam-amld is
 *		restarted.
 *
 *	sam_put_amld_cmd(sam_fs_fifo_t *cmd, block_flag)
 *		This routine is the external interface for the FS to put a
 *		command to sam-amld.
 *
 *	sam_send_amld_cmd(sam_fs_fifo_t *cmd, block_flag)
 *		This routine is the external interface for the FS to pass
 *		commands to sam-amld.
 *
 *	sam_get_amld_cmd()
 *		This routine is called from the syscall interface to copy
 *		a command to sam-amld's memory space.
 *
 *	sam_shutdown_amld()
 *		This routine is called from the syscall interface to shutdown
 *		the sam-amld daemon activity.
 *
 *	sam_get_amld_state()
 *		This routine is called from the syscall interface to get the
 *		daemon state.
 *
 *	sam_amld_init()
 *		This routine is called from fs/init.c to initialize the
 *		the interface data structures.
 *
 *	sam_amld_fini()
 *		This routine is called from fs/init.c to remove the
 *		the interface data structures.
 *
 *	See below for the internal data structures and routines.
 */

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/t_lock.h>
#include <sys/cred.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/ddi.h>
#include <sys/disp.h>
#include <sys/kmem.h>

/* ----- SAMFS Includes */

#include "sam/types.h"
#include "sam/resource.h"
#include "sam/syscall.h"

#include "inode.h"
#include "mount.h"
#include "global.h"
#include "extern.h"
#include "debug.h"
#include "amld.h"
#include "trace.h"


/*
 *	Function prototypes for internal functions.
 */

static samamld_cmd_queue_t *add_to_samamld_cmd_queue(sam_fs_fifo_t *, int);
static int samamld_timed_out();
static void clear_samamld_cmd_queue();
static samamld_cmd_queue_t *remove_from_queue(samamld_cmd_queue_t *);
static void put_on_free_list(samamld_cmd_queue_t *);
static samamld_cmd_queue_t *get_free_cmd_buffer(int);
static int put_cmd_on_queue(sam_fs_fifo_t *, int);
static void unlink_from_queue(samamld_cmd_queue_t *);
static void clear_amld();


/*
 *	This is the master table to hold control structure for the interface.
 *	See include/sam/fs/amld.h for details.
 */

samamld_cmd_table_t *si_cmd_table = NULL;


/*
 *	Sent the sam-amld daemon not responding message.
 */

boolean_t sam_amld_down_msg_sent = B_FALSE;


/*
 * ----- sam_sync_amld - initialization/sync for sam-amld.
 *
 * This routine is called when sam-amld starts, before sam-amld checks for any
 * other cmds.
 * The order of logic is:
 *	reset the sam-amld pid in the header.
 *	set the lock flag to keep the FS out of the cmd queue.
 *	flush everything out of the cmd queue.
 *	send a RESYNC to sam-amld.
 *	for every mounted file system:
 *	send an INIT to sam-amld.
 *	clear the lockout flag
 *	broadcast a signal to every thread that is waiting on the lockout cv.
 */

void
sam_sync_amld(sam_resync_arg_t *args)
{
	sam_fs_fifo_t fifo;
	samamld_cmd_queue_hdr_t *hdr = &si_cmd_table->samamld_cmd_queue_hdr;
	int error;

	if (args) {
		/*
		 * Set minimum directio filesize, sam-amld pid and time stamp.
		 * max_stages will be set after the stages are restarted.
		 */
		mutex_enter(&samgt.global_mutex);
		samgt.amld_pid = args->sam_amld_pid;
		mutex_exit(&samgt.global_mutex);
		fifo.param.fs_resync.seq = args->seq;  /* time stamp */
	}

	mutex_enter(&hdr->cmd_lockout_mutex);
	hdr->cmd_lock_flag = 1;
	mutex_exit(&hdr->cmd_lockout_mutex);
	si_cmd_table->samamld_cmd_queue_hdr.cmd_queue_timeout = lbolt;

	clear_samamld_cmd_queue();

	fifo.cmd = FS_FIFO_RESYNC;
	fifo.magic = FS_FIFO_MAGIC;
	if (error = sam_put_amld_cmd(&fifo, SAM_NOBLOCK)) {
		cmn_err(CE_WARN,
		    "SAM-QFS: failed to notify sam-amld of resync, "
		    "error = %d", error);
	}

	mutex_enter(&hdr->cmd_lockout_mutex);
	hdr->cmd_lock_flag = 0;
	cv_broadcast(&hdr->cmd_lockout_cv);
	mutex_exit(&hdr->cmd_lockout_mutex);

	(void) sam_start_stop_rmedia(NULL, SAM_START_DAEMON);

	sam_amld_down_msg_sent = B_FALSE;
}


/*
 * ----- sam_put_amld_cmd - puts a command on the samamld queue.
 *
 * This routine puts a cmd on the cmd queue for sam-amld to pick up.
 * put the cmd on the cmd queue
 */

int
sam_put_amld_cmd(sam_fs_fifo_t *cmd, enum sam_block_flag blk_flag)
{
	return (put_cmd_on_queue(cmd, blk_flag));
}


/*
 * ----- sam_send_amld_cmd - puts a command on the samamld queue.
 *
 * This routine puts a cmd on the cmd queue for sam-amld to pick up.
 * If the sam-amld pid hasn't been set, or, if it has been reset to
 * -1 because of a sam-amld timeout, return EAGAIN.
 * if the cmd queue is locked, wait on the lockout cv for it to be unlocked.
 * If sam-amld has timed out, set the sam-amld pid to -1, flush the cmd
 *	queue and return EAGAIN.
 * put the cmd on the cmd queue
 */

int
sam_send_amld_cmd(sam_fs_fifo_t *cmd, enum sam_block_flag blk_flag)
{
	samamld_cmd_queue_hdr_t *hdr = &si_cmd_table->samamld_cmd_queue_hdr;
	int error = 0;

retry:
	if (samgt.amld_pid > 0) {
		if (blk_flag == SAM_NOBLOCK && hdr->cmd_lock_flag) {
			cmn_err(CE_WARN,
			    "SAM-QFS: sam_send_amld_cmd: EAGAIN "
			    "samgt.amld_pid > 0");
			return (EAGAIN);
		}

		mutex_enter(&hdr->cmd_lockout_mutex);
		while (hdr->cmd_lock_flag) {
			if (sam_cv_wait_sig(&hdr->cmd_lockout_cv,
			    &hdr->cmd_lockout_mutex) == 0) {
				mutex_exit(&hdr->cmd_lockout_mutex);
				cmn_err(CE_WARN,
				    "SAM-QFS: sam_send_amld_cmd: EINTR "
				    "samgt.amld_pid > 0");
				return (EINTR);
			}
		}
		mutex_exit(&hdr->cmd_lockout_mutex);

		if (samamld_timed_out()) {
			cmn_err(CE_WARN,
			    "SAM-QFS: sam-amld daemon timed out "
			    "(sam_send_amld_cmd)");
			/* Do not call sam_shutdown_amld here. */
			(void) clear_amld();
			/* Delay for 10 seconds to avoid tight loop. */
			delay(hz*10);
			goto retry;
		}
		if ((error = put_cmd_on_queue(cmd, blk_flag)) == ECANCELED) {
			cmn_err(CE_WARN,
			    "SAM-QFS: sam_send_amld_cmd: "
			    "ECANCELED samgt.amld_pid > 0");
			/* Delay for 10 seconds to avoid tight loop. */
			delay(hz*10);
			goto retry;
		} else {
			if (error != 0) {
				cmn_err(CE_WARN,
				    "SAM-QFS: sam_send_amld_cmd: "
				    "error: %d samgt.amld_pid > 0",
				    error);
			}
			return (error);
		}
	} else if (samgt.amld_pid < 0) {
		if (!sam_amld_down_msg_sent) {
			sam_amld_down_msg_sent = B_TRUE;
			cmn_err(CE_WARN,
			    "SAM-QFS: sam-amld daemon not responding");
		}
		if (blk_flag == SAM_BLOCK_DAEMON_ACTIVE) {
			cmn_err(CE_WARN,
			    "SAM-QFS: sam_send_amld_cmd: EIO "
			    "samgt.amld_pid < 0");
			return (EIO);
		}
		if (blk_flag == SAM_NOBLOCK) {
			cmn_err(CE_WARN,
			    "SAM-QFS: sam_send_amld_cmd: EAGAIN "
			    "samgt.amld_pid < 0");
			return (EAGAIN);
		}

		mutex_enter(&hdr->cmd_lockout_mutex);
		while (hdr->cmd_lock_flag) {
			if (sam_cv_wait_sig(&hdr->cmd_lockout_cv,
			    &hdr->cmd_lockout_mutex) == 0) {
				mutex_exit(&hdr->cmd_lockout_mutex);
				cmn_err(CE_WARN,
				    "SAM-QFS: sam_send_amld_cmd: "
				    "EINTR samgt.amld_pid < 0");
				return (EINTR);
			}
		}
		mutex_exit(&hdr->cmd_lockout_mutex);
		delay(hz*10);  /* Delay for 10 seconds to avoid tight loop. */
		goto retry;
	} else {
		cmn_err(CE_WARN,
		    "SAM-QFS: sam_send_amld_cmd: EAGAIN samgt.amld_pid = 0");
		return (EAGAIN);
	}
}


/*
 * ----- sam_get_amld_cmd - gets a command from the samamld queue.
 *
 * This routine is called from the syscall interface to take a cmd off
 * the cmd queue and copy it out to sam-amld's memory space.
 * If there are no cmd's in the queue, it waits on cmd_queue_cv for
 * the FS to put a cmd on the queue.  When it has a cmd, it signals
 * that cmds cmd_cv, to unblock the thread that might be waiting for
 * sam-amld to pick up that cmd.
 *
 * On entry, it zeros the timeout value, which tells the FS that sam-amld
 * is waiting for a cmd.  On exit, it sets the current time into the
 * timeout value, so that the FS can determine how long sam-amld has
 * been away.  see also samamld_timed_out()
 */

int
sam_get_amld_cmd(char *arg)
{
	samamld_cmd_queue_t *qcmd;
	int error;
	clock_t timeout;

	samamld_cmd_queue_hdr_t *hdr = &si_cmd_table->samamld_cmd_queue_hdr;

	if (samamld_timed_out()) {
		cmn_err(CE_WARN,
		    "SAM-QFS: sam-amld daemon timed out (sam_get_amld_cmd)");
		sam_sync_amld(NULL);
	}

	mutex_enter(&hdr->cmd_queue_mutex);
	hdr->cmd_queue_timeout = lbolt;

	while ((qcmd = remove_from_queue(hdr->front)) == NULL) {
		hdr->cmd_queue_timeout = lbolt;
		/*
		 * Wait for a command to appear on the queue for 8 minutes,
		 * then try again.  This is mostly to refresh the command queue
		 * timeout value so the FS knows if sam-amld has been killed or
		 * just plain died.
		 */
		timeout = lbolt + (hz << 9);
		if ((cv_timedwait_sig(&hdr->cmd_queue_cv,
		    &hdr->cmd_queue_mutex, timeout)) == 0) {
				mutex_exit(&hdr->cmd_queue_mutex);
				return (EINTR);
		}
	}

	error = copyout((char *)&qcmd->cmd, (char *)arg,
	    sizeof (sam_fs_fifo_t));

	mutex_enter(&qcmd->cmd_mutex);
	qcmd->qcmd_wait = 0;
	cv_signal(&qcmd->cmd_cv);

	hdr->cmd_queue_timeout = lbolt;
	/*
	 * Only put the command back on the free list if it was a non-blocking
	 * command.  Otherwise, the initiating thread will put it back.
	 */
	if (qcmd->blk_flag == SAM_NOBLOCK) {
		mutex_exit(&qcmd->cmd_mutex);
		put_on_free_list(qcmd);
	} else {
		mutex_exit(&qcmd->cmd_mutex);
	}

	mutex_exit(&hdr->cmd_queue_mutex);
	return (error);
}


/*
 * ----- sam_shutdown_amld - shutdown samamld.
 *
 * shut down the sam-amld daemon activity.
 * requires sam-amld to be functioning properly.
 * called from the sam-amld thread; caller cannot
 * use this function if the inode rwlock is set.
 */

void
sam_shutdown_amld()
{
	(void) sam_start_stop_rmedia(NULL, SAM_STOP_DAEMON);
	clear_amld();
}


/*
 * ----- sam_get_amld_state - get samamld state.
 *
 * report the status of sam-amld
 */

int
sam_get_amld_state(int *pid)
{
	if ((*pid = samgt.amld_pid) == -1) {
		return (SAM_AMLD_SHUT_DOWN);
	}

	if (samamld_timed_out()) {
		cmn_err(CE_WARN,
		    "SAM-QFS: sam-amld daemon timed out (sam_get_amld_state)");
		return (SAM_AMLD_NOT_RESPONDING);
	}

	return (SAM_AMLD_RUNNING);
}


/*
 * ----- sam_amld_init - initialization.
 *
 * This routine initializes the cmd internal structures.
 * It allocates space for the cmd_table and the cmd buffer list.
 * It sets up the cmd buffer free list.
 * It initializes the mutex's and cv's
 */

void
sam_amld_init()
{
	int i;
	samamld_cmd_queue_t *p;

	si_cmd_table = (samamld_cmd_table_t *)
	    kmem_zalloc(sizeof (samamld_cmd_table_t), KM_SLEEP);
	samgt.amld_pid = 0;

	sam_amld_down_msg_sent = B_FALSE;

	si_cmd_table->cmd_buffers = (samamld_cmd_queue_t *)
	    kmem_zalloc(sizeof (samamld_cmd_queue_t) * SAM_AMLD_CMD_BUF,
	    KM_SLEEP);
	si_cmd_table->samamld_cmd_queue_hdr.front = NULL;
	si_cmd_table->samamld_cmd_queue_hdr.end = NULL;
	si_cmd_table->samamld_cmd_queue_hdr.cmd_queue_timeout = 0;
	si_cmd_table->samamld_cmd_queue_hdr.cmd_lock_flag = 1;
	sam_mutex_init(&si_cmd_table->samamld_cmd_queue_hdr.cmd_queue_mutex,
	    NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&si_cmd_table->queue_mutex,
	    NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&si_cmd_table->samamld_cmd_queue_hdr.cmd_lockout_mutex,
	    NULL, MUTEX_DEFAULT, NULL);
	cv_init(&si_cmd_table->samamld_cmd_queue_hdr.cmd_queue_cv,
	    NULL, CV_DEFAULT, NULL);
	cv_init(&si_cmd_table->samamld_cmd_queue_hdr.cmd_lockout_cv,
	    NULL, CV_DEFAULT, NULL);
	cv_init(&si_cmd_table->free_list_cond,
	    NULL, CV_DEFAULT, NULL);

	si_cmd_table->cmd_buffer_free_list = &(si_cmd_table->cmd_buffers[0]);
	si_cmd_table->cmd_buffer_free_list_count = SAM_AMLD_CMD_BUF;
	p = si_cmd_table->cmd_buffer_free_list;
	/* This loop starts at one, because the first buffer is put in the */
	/* free list above */
	for (i = 1; i <= SAM_AMLD_CMD_BUF; i++) {
		if (i < SAM_AMLD_CMD_BUF) {
			p->forward = &(si_cmd_table->cmd_buffers[i]);
		} else {
			p->forward = NULL;
		}
		cv_init(&p->cmd_cv, NULL, CV_DEFAULT, NULL);
		sam_mutex_init(&p->cmd_mutex, NULL, MUTEX_DEFAULT, NULL);
		if (p->forward) {
			p = p->forward;
			p->forward = NULL;
		}
	}
}


/*
 * ----- sam_amld_fini
 *
 * This routine frees the cmd internal structures.
 * It frees space for the cmd_table and the cmd buffer list.
 */

void
sam_amld_fini()
{
	kmem_free((void *)si_cmd_table->cmd_buffers,
	    sizeof (samamld_cmd_queue_t) * SAM_AMLD_CMD_BUF);
	kmem_free((void *)si_cmd_table, sizeof (samamld_cmd_table_t));
}


/*
 *	Internal (private) functions.
 *
 *	put_cmd_on_queue(cmd)
 *		put the cmd on the command queue for sam-amld to pick up.
 *
 *	samamld_timed_out()
 *		Boolean function to determine if sam-amld has been away
 *		too long.
 *
 *	clear_samamld_cmd_queue()
 *		Routine that flushes all of the commands in the command queue.
 *		This occurs when sam-amld resyncs and when sam-amld has
 *		timed out.
 *		Any commands that are being waited for cause a signal to be
 *		sent
 *		to the waiting thread with an error of ECANCEL.
 *
 *	add_to_samamld_cmd_queue(cmd, blk_flag)
 *		Add cmd to the command queue
 *
 *	remove_from_queue(cmd)
 *		remove the specified command from the front of the command
 *		queue.
 *
 *	unlink_from_queue(cmd)
 *		unlink the specified command from wherever it is in the command
 *		queue.  This is done for commands that have been interrupted by
 *		a signal and have not yet been picked up by sam-amld.
 *
 *	put_on_free_list(cmd, blk_flag)
 *		put the specified cmd on the free list for re-use.
 *
 *	get_free_cmd_buffer(blk_flag)
 *		Get a cmd buffer from the free list.
 *
 *	clear_amld()
 *		Shut down the sam-amld command queue.
 *
 */


/*
 * This routine puts a cmd on the cmd queue.
 * First, add the cmd to the cmd queue.
 * If the block flag is set to SAM_BLOCK, wait on the cmd cv for sam-amld to
 *	pick up the cmd.
 * If the wait is interrupted due to a signal, take the cmd off the queue
 *	and return EINTR.
 */

static int
put_cmd_on_queue(sam_fs_fifo_t *cmd, int blk_flag)
{
	int error;
	samamld_cmd_queue_t *qcmd;
	samamld_cmd_queue_hdr_t *hdr = &si_cmd_table->samamld_cmd_queue_hdr;

	mutex_enter(&hdr->cmd_queue_mutex);
	cmd->sam_amld_pid = samgt.amld_pid;
	if (cmd->magic != FS_FIFO_MAGIC) {
		cmn_err(SAMFS_DEBUG_PANIC,
		    "SAM-QFS: bad cmd fifo magic (%x)", cmd->magic);
	}

	qcmd = add_to_samamld_cmd_queue(cmd, blk_flag);

	if (qcmd) {
		mutex_enter(&qcmd->cmd_mutex);
		qcmd->qcmd_error = 0;
		qcmd->qcmd_wait = 1;
		mutex_exit(&qcmd->cmd_mutex);
	}
	mutex_exit(&hdr->cmd_queue_mutex);

	if (qcmd == NULL) {
		cmn_err(CE_WARN,
		    "SAM-QFS: put_cmd_on_queue: EAGAIN samgt.amld_pid = %d",
		    samgt.amld_pid);
		return (EAGAIN);
	} else {
		if (blk_flag == SAM_BLOCK ||
		    blk_flag == SAM_BLOCK_DAEMON_ACTIVE) {
			mutex_enter(&qcmd->cmd_mutex);
			while (qcmd->qcmd_wait) {
				if (sam_cv_wait_sig(&qcmd->cmd_cv,
				    &qcmd->cmd_mutex) == 0) {
					mutex_exit(&qcmd->cmd_mutex);
					unlink_from_queue(qcmd);
					return (EINTR);
				}
			}
			error = qcmd->qcmd_error;
			mutex_exit(&qcmd->cmd_mutex);
			/*
			 * This is a blocking cmd.  That means that this
			 * thread has to
			 * put the cmd buffer back on the free list.
			 */
			mutex_enter(&hdr->cmd_queue_mutex);
			put_on_free_list(qcmd);
			mutex_exit(&hdr->cmd_queue_mutex);
			return (error);
		}
	}
	return (0);
}

/*
 * Check to see if the sam-amld daemon has timed out.
 * If the timeout value is more than 1024 seconds ago (~17 min.),
 * then we have to believe that sam-amld is never coming back, so
 * return that sam-amld has timed out.
 */

static int
samamld_timed_out()
{
	clock_t now = lbolt;
	clock_t last = si_cmd_table->samamld_cmd_queue_hdr.cmd_queue_timeout;

	if ((last + (hz << 10)) < now) {
		return (1);
	}
	return (0);
}


/*
 * flush the cmd queue.
 * Walk down the cmd queue, removing the cmd at the front of the
 * queue and putting it on the free list.  For every cmd, set
 * the error value to ECANCELED and signal the waiting thread.
 */

static void
clear_samamld_cmd_queue()
{
	samamld_cmd_queue_t *cmd;
	samamld_cmd_queue_hdr_t *hdr = &si_cmd_table->samamld_cmd_queue_hdr;

	mutex_enter(&hdr->cmd_queue_mutex);
	cmd = hdr->front;
	while (cmd) {
		mutex_enter(&cmd->cmd_mutex);
		cmd->qcmd_error = ECANCELED;
		cmd->qcmd_wait = 0;
		mutex_exit(&cmd->cmd_mutex);
		(void) remove_from_queue(cmd);

		mutex_enter(&cmd->cmd_mutex);
		cv_signal(&cmd->cmd_cv);
		if (cmd->blk_flag == SAM_NOBLOCK) {
			mutex_exit(&cmd->cmd_mutex);
			put_on_free_list(cmd);
		} else {
			mutex_exit(&cmd->cmd_mutex);
		}
		cmd = hdr->front;
	}
	mutex_exit(&hdr->cmd_queue_mutex);
}

/*
 * Add a cmd to the end (tail) of the cmd queue.
 * Get a free buffer. If there isn't one, return NULL.
 * Copy the cmd into the buffer.
 * Link it onto the queue.
 * Signal the sam-amld thread that there is a cmd on the queue.
 *
 * Mutex hdr->cmd_queue_mutex is set on entry.
 *
 * The in-use cmd entries are maintained in a doubly linked list (non-circular)
 * in conjunction with a head (front) and tail (end) pointer.
 * The in-use cmd entries contain pointers set up in the following manner:
 *     cmd->forward always points towards the head of the cmd queue.
 *     cmd->back    always points towards the tail of the cmd queue.
 * The free   cmd entries are maintained in a singly linked list (non-circular)
 * in conjunction with a head (cmd_buffer_free_list) pointer.
 * The free   cmd entries contain pointers set up in the following manner:
 *     cmd->forward always points towards the tail of the cmd queue.
 *     cmd->back    is left unchanged because it is not used.
 */

static samamld_cmd_queue_t *
add_to_samamld_cmd_queue(sam_fs_fifo_t *c, int blk_flag)
{
	samamld_cmd_queue_t *cmd;
	samamld_cmd_queue_hdr_t *hdr = &si_cmd_table->samamld_cmd_queue_hdr;

	cmd = get_free_cmd_buffer(blk_flag);
	if (cmd) {
		cmd->cmd = *c;
		if (hdr->end) {
			hdr->end->back = cmd;
		}
		cmd->forward = hdr->end;
		hdr->end = cmd;
		cmd->back = NULL;
		if (hdr->front == NULL) {
			hdr->front = cmd;
		}
		cmd->blk_flag = blk_flag;
		cv_signal(&hdr->cmd_queue_cv);
		return (cmd);
	}
	return (NULL);
}

/*
 * Delete a cmd from the front (head) of the cmd queue.
 * Unlink it from the queue.
 *
 * Mutex hdr->cmd_queue_mutex is set on entry.
 *
 * The in-use cmd entries are maintained in a doubly linked list (non-circular)
 * in conjunction with a head (front) and tail (end) pointer.
 * The in-use cmd entries contain pointers set up in the following manner:
 *     cmd->forward always points towards the head of the cmd queue.
 *     cmd->back    always points towards the tail of the cmd queue.
 * The free   cmd entries are maintained in a singly linked list (non-circular)
 * in conjunction with a head (cmd_buffer_free_list) pointer.
 * The free   cmd entries contain pointers set up in the following manner:
 *     cmd->forward always points towards the tail of the cmd queue.
 *     cmd->back    is left unchanged because it is not used.
 */

static samamld_cmd_queue_t *
remove_from_queue(samamld_cmd_queue_t *cmd)
{
	samamld_cmd_queue_hdr_t *hdr = &si_cmd_table->samamld_cmd_queue_hdr;

	if (cmd) {
		if (cmd == hdr->front) {
			if (cmd->back) {
				hdr->front = cmd->back;
				hdr->front->forward = NULL;
			} else {
				hdr->front = NULL;
			}
		}
		if (cmd == hdr->end) {
			hdr->end = NULL;
		}
	}
	return (cmd);
}

/*
 * Delete a cmd from an arbitrary point in the queue.
 * Just your standard remove-from-doubly-linked-list.
 *
 * Mutex hdr->cmd_queue_mutex is set within this routine.
 *
 * The in-use cmd entries are maintained in a doubly linked list (non-circular)
 * in conjunction with a head (front) and tail (end) pointer.
 * The in-use cmd entries contain pointers set up in the following manner:
 *     cmd->forward always points towards the head of the cmd queue.
 *     cmd->back    always points towards the tail of the cmd queue.
 * The free   cmd entries are maintained in a singly linked list (non-circular)
 * in conjunction with a head (cmd_buffer_free_list) pointer.
 * The free   cmd entries contain pointers set up in the following manner:
 *     cmd->forward always points towards the tail of the cmd queue.
 *     cmd->back    is left unchanged because it is not used.
 */

static void
unlink_from_queue(samamld_cmd_queue_t *cmd)
{
	samamld_cmd_queue_hdr_t *hdr = &si_cmd_table->samamld_cmd_queue_hdr;
	samamld_cmd_queue_t *p;

	mutex_enter(&hdr->cmd_queue_mutex);
	if (cmd) {
		p = hdr->front;
		while (p && p != cmd) {
			p = p->back;
		}
		if (p == NULL) {
			/* it wasn't on the queue */
			/* This routine is called by the FS thread. */
			/* The FS thread is responsible for putting */
			/* blocking cmds back on the free list */
			if (cmd->blk_flag == SAM_BLOCK ||
			    cmd->blk_flag == SAM_BLOCK_DAEMON_ACTIVE) {
				cmn_err(CE_WARN,
				"SAM-QFS: unlink_from_queue: cmd not "
				"found: freed blk_flag: %d",
				    cmd->blk_flag);
				put_on_free_list(cmd);
			} else {
				cmn_err(CE_WARN,
			"SAM-QFS: unlink_from_queue: cmd not found: not "
			"freed blk_flag: %d",
				    cmd->blk_flag);
			}
			mutex_exit(&hdr->cmd_queue_mutex);
			return;
		}
		/* This cmd happens to be at the head of the cmd queue, */
		/* so just call remove_from_queue(). */
		if (cmd == hdr->front) {
			cmn_err(CE_NOTE,
			    "SAM-QFS: unlink_from_queue: remove from front");
			remove_from_queue(cmd);
			put_on_free_list(cmd);
			mutex_exit(&hdr->cmd_queue_mutex);
			return;
		}
		if (cmd == hdr->end) {
			cmn_err(CE_NOTE,
			    "SAM-QFS: unlink_from_queue: remove from end");
			hdr->end = cmd->forward;
		} else {
			cmn_err(CE_NOTE,
			    "SAM-QFS: unlink_from_queue: remove from middle");
		}
		if (cmd->forward) {
			cmd->forward->back = cmd->back;
		}
		if (cmd->back) {
			cmd->back->forward = cmd->forward;
		}
		put_on_free_list(cmd);
	}
	mutex_exit(&hdr->cmd_queue_mutex);
}

/*
 * Put a cmd on the front (head) of the free list.
 * Signal that a cmd buffer is available.
 *
 * Mutex hdr->cmd_queue_mutex is set on entry.
 * Mutex si_cmd_table->queue_mutex is set within this routine.
 *
 * The in-use cmd entries are maintained in a doubly linked list (non-circular)
 * in conjunction with a head (front) and tail (end) pointer.
 * The in-use cmd entries contain pointers set up in the following manner:
 *     cmd->forward always points towards the head of the cmd queue.
 *     cmd->back    always points towards the tail of the cmd queue.
 * The free   cmd entries are maintained in a singly linked list (non-circular)
 * in conjunction with a head (cmd_buffer_free_list) pointer.
 * The free   cmd entries contain pointers set up in the following manner:
 *     cmd->forward always points towards the tail of the cmd queue.
 *     cmd->back    is left unchanged because it is not used.
 */

static void
put_on_free_list(samamld_cmd_queue_t *cmd)
{

	mutex_enter(&si_cmd_table->queue_mutex);

	if (si_cmd_table->cmd_buffer_free_list) {
		cmd->forward = si_cmd_table->cmd_buffer_free_list;
	} else {
		/* The free list is currently empty. */
		cmd->forward = NULL;
	}

	si_cmd_table->cmd_buffer_free_list = cmd;
	si_cmd_table->cmd_buffer_free_list_count =
	    si_cmd_table->cmd_buffer_free_list_count + 1;

	if (si_cmd_table->cmd_buffer_free_list == NULL) {
		cmn_err(CE_WARN,
"SAM-QFS: put_on_free_list: cmd_buffer_free_list NULL: "
"cmd_buffer_free_list_count = %d",
		    si_cmd_table->cmd_buffer_free_list_count);
	}
	cv_signal(&si_cmd_table->free_list_cond);
	mutex_exit(&si_cmd_table->queue_mutex);
}

/*
 * Get a cmd off the front (head) of the free list.
 * If the wait is interrupted due to a signal, return NULL.
 *
 * Mutex hdr->cmd_queue_mutex is set on entry.
 * Mutex si_cmd_table->queue_mutex is set within this routine.
 *
 * The in-use cmd entries are maintained in a doubly linked list (non-circular)
 * in conjunction with a head (front) and tail (end) pointer.
 * The in-use cmd entries contain pointers set up in the following manner:
 *     cmd->forward always points towards the head of the cmd queue.
 *     cmd->back    always points towards the tail of the cmd queue.
 * The free   cmd entries are maintained in a singly linked list (non-circular)
 * in conjunction with a head (cmd_buffer_free_list) pointer.
 * The free   cmd entries contain pointers set up in the following manner:
 *     cmd->forward always points towards the tail of the cmd queue.
 *     cmd->back    is left unchanged because it is not used.
 */

static samamld_cmd_queue_t *
get_free_cmd_buffer(int blk_flag)
{
	samamld_cmd_queue_t *cmd = NULL;

	/* must release this lock so that the sam-amld thread can */
	/* have access to the queue */
	mutex_exit(&si_cmd_table->samamld_cmd_queue_hdr.cmd_queue_mutex);
retry:
	mutex_enter(&si_cmd_table->queue_mutex);
	if (si_cmd_table->cmd_buffer_free_list) {

		cmd = si_cmd_table->cmd_buffer_free_list;
		si_cmd_table->cmd_buffer_free_list = cmd->forward;
		si_cmd_table->cmd_buffer_free_list_count =
		    si_cmd_table->cmd_buffer_free_list_count - 1;

		if ((si_cmd_table->cmd_buffer_free_list == NULL) &&
		    (si_cmd_table->cmd_buffer_free_list_count > 0)) {
			cmn_err(SAMFS_DEBUG_PANIC,
			    "SAM-QFS: sam-amld cmd free list NULL, "
			    "blk_flag=%d count=%d cmd=%p fwd=%p",
			    blk_flag, si_cmd_table->cmd_buffer_free_list_count,
			    (void *)cmd, (void *)cmd->forward);
		}
	} else if (blk_flag != SAM_NOBLOCK) {
		/* wait for a free buffer to show up */
		while (si_cmd_table->cmd_buffer_free_list == NULL) {
			if (cv_wait_sig(&si_cmd_table->free_list_cond,
			    &si_cmd_table->queue_mutex) == 0) {
				mutex_exit(&si_cmd_table->queue_mutex);
				mutex_enter(&si_cmd_table->
				    samamld_cmd_queue_hdr.cmd_queue_mutex);
				return (NULL);
			}
		}
		mutex_exit(&si_cmd_table->queue_mutex);
		goto retry;
	}
	mutex_exit(&si_cmd_table->queue_mutex);
	mutex_enter(&si_cmd_table->samamld_cmd_queue_hdr.cmd_queue_mutex);
	return (cmd);
}

/*
 * Shut down the sam-amld command queue.
 */

static void
clear_amld()
{
	samamld_cmd_queue_hdr_t *hdr = &si_cmd_table->samamld_cmd_queue_hdr;

	mutex_enter(&hdr->cmd_lockout_mutex);
	hdr->cmd_lock_flag = 1;
	mutex_exit(&hdr->cmd_lockout_mutex);

	mutex_enter(&samgt.global_mutex);
	samgt.amld_pid = -1;
	mutex_exit(&samgt.global_mutex);

	clear_samamld_cmd_queue();

	mutex_enter(&hdr->cmd_lockout_mutex);
	hdr->cmd_lock_flag = 0;
	cv_broadcast(&hdr->cmd_lockout_cv);
	mutex_exit(&hdr->cmd_lockout_mutex);

	sam_amld_down_msg_sent = B_FALSE;
}
