/*
 * message.c - process incoming messages
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

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

#include <thread.h>
#include <synch.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/shm.h"
#include "sam/defaults.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "aml/historian.h"
#include "aml/remote.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"
#include "sam/lib.h"
#include "sam/sam_trace.h"
#include "client.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Globals */
extern shm_alloc_t master_shm;

/* Function prototypes */
static void process_client_message(dev_ent_t *un, message_request_t *request);

/*
 * Thread routine to monitor messages.
 */
void *
rc_monitor_msg(
	void *vun)
{
	sigset_t signal_set;
	dev_ent_t *un = (dev_ent_t *)vun;
	message_request_t *message;

	(void) sigfillset(&signal_set);	/* Block all signals */
	(void) thr_sigsetmask(SIG_SETMASK, &signal_set, (sigset_t *)NULL);
	(void) sigemptyset(&signal_set);
	(void) sigaddset(&signal_set, SIGEMT);	/* Allow SIGEMT */
	(void) thr_sigsetmask(SIG_UNBLOCK, &signal_set, NULL);
	message = (message_request_t *)SHM_REF_ADDR(un->dt.sc.message);
	Trace(TR_PROC, "Monitor messages thread started");

	/* Main loop */
	mutex_lock(&message->mutex);
	for (;;) {
		while (message->mtype == MESS_MT_VOID) {
			(void) cond_wait(&message->cond_r, &message->mutex);
		}

		process_client_message(un, message);
		message->mtype = MESS_MT_VOID;
		message->message.exit_id.pid = 0;
		/* wake up anyone waiting */
		(void) cond_signal(&message->cond_i);
	}
	/* LINTED Function has no return statement */
}

/* ARGSUSED */
static void
process_client_message(
	dev_ent_t *un,
	message_request_t *request)
{
	switch (request->message.command) {

	case MESS_CMD_PREVIEW:
		/*
		 * The MESS_CMD_PREVIEW hasn't been used since 3.5.0
		 * Just ignore it.
		 */
		break;

	default:
		Trace(TR_MISC, "Unknown message received 0x%x",
		    request->message.command);
		break;
	}
}
