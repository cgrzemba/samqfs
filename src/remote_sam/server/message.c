/*
 * message.c - process incoming messages.
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

#pragma ident "$Revision: 1.20 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <thread.h>
#include <synch.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/shm.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "aml/trace.h"
#include "sam/nl_samfs.h"
#include "aml/remote.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "sam/sam_trace.h"
#include "server.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Globals */
extern shm_alloc_t master_shm;

/* Function prototypes */
static void rs_server_resp(dev_ent_t *un, sam_message_t **incoming);
static void sig_catch();

/*
 * Thread routine to monitor incoming messages.
 */
void *
rs_monitor_msg(
	void *vun)
{
	sigset_t signal_set;
	dev_ent_t *un;
	struct sigaction sig_action;
	sam_message_t *mess_data = NULL;
	message_request_t *message;
	enum sam_mess_type mtype;

	un = (dev_ent_t *)vun;

	/*
	 * Should have been called with all signals blocked, now
	 * let sigemt be delivered and just exit when it is.
	 */

	sig_action.sa_handler = sig_catch;
	sig_action.sa_flags = 0;
	(void) sigemptyset(&signal_set);
	(void) sigaddset(&signal_set, SIGEMT);
	(void) sigaction(SIGEMT, &sig_action, (struct sigaction *)NULL);
	(void) thr_sigsetmask(SIG_UNBLOCK, &signal_set, NULL);
	message = (message_request_t *)SHM_REF_ADDR(un->dt.ss.message);
	thr_yield();
	sleep(5);

	/* Main loop */
	for (;;) {
		/* Allocate a message area if needed */
		if (mess_data == NULL)
			mess_data =
			    (sam_message_t *)malloc_wait(sizeof (*mess_data),
			    2, 0);

		/* Wait for a message */
		mutex_lock(&message->mutex);
		while (message->mtype == MESS_MT_VOID) {
			(void) cond_wait(&message->cond_r, &message->mutex);
		}

		/* copy the message */
		memcpy(mess_data, &(message->message), sizeof (sam_message_t));
		mtype = message->mtype;

		message->mtype = MESS_MT_VOID;	/* release the message area */
		message->message.exit_id.pid = 0;
		/* and wake up anyone waiting */
		(void) cond_signal(&message->cond_i);
		mutex_unlock(&message->mutex);

		/*
		 * Since most of these are short term (just copying data and
		 * wakeing up a waiting process) we will pass a pointer to the
		 * data pointer.  If this will be a long term thing, rs_server
		 * will set the pointer to null and start a thread.  It is up
		 * to the thread to free the space.
		 */
		if (mtype == MESS_MT_RS_SERVER) {
			rs_server_resp(un, &mess_data);
		} else {
			Trace(TR_MISC, "Unknown server message type %x", mtype);
		}
	}
	/* LINTED Function has no return statement */
}

static void
rs_server_resp(
	dev_ent_t *un,
	sam_message_t **incoming)
{
	sam_message_t *event = *incoming;
	rmt_mess_t *msg = &event->param.rmt_message;

	switch ((rmt_mess_cmd_t)event->command) {

	/* Send message to the client */
	case RMT_MESS_CMD_SEND:
	{
		rmt_mess_send_t *send_msg = &(msg->messages.send);
		srvr_clnt_t *srvr_clnt =
		    (srvr_clnt_t *)SHM_REF_ADDR(un->dt.ss.clients);

		Trace(TR_MISC, "Send command to client (CMD_SEND)");

		if (send_msg->client_index >= RMT_SAM_MAX_CLIENTS ||
		    !((srvr_clnt + send_msg->client_index)->flags &
		    SRVR_CLNT_CONNECTED)) {
			Trace(TR_MISC, "Client (%d) not connected",
			    send_msg->client_index);
		} else {
			rmt_sam_client_t *clnt;

			clnt = (((rmt_sam_client_t *)un->dt.ss.private) +
			    send_msg->client_index);
			Trace(TR_MISC, "Send message %d on fd %d",
			    send_msg->mess.command, clnt->fd);
			send(clnt->fd,
			    (char *)&send_msg->mess,
			    sizeof (rmt_sam_request_t), 0);
		}
	}
	break;

	case RMT_MESS_CATALOG_CHANGE:
	{
		int i;
		rmt_vsn_equ_list_t *next_equ;
		rmt_entries_found_t *found;
		srvr_clnt_t *srvr_clnt;
		rmt_sam_client_t *client;
		rmt_mess_cat_chng_t *change;

		srvr_clnt = (srvr_clnt_t *)SHM_REF_ADDR(un->dt.ss.clients);
		client = (rmt_sam_client_t *)un->dt.ss.private;
		change = &(msg->messages.cat_change);

		Trace(TR_MISC,
		    "[%s] Send catalog change (CATALOG_CHANGE) eq: %d",
		    TrNullString(client->host_name), change->eq);

		found = (rmt_entries_found_t *)malloc_wait(sizeof (*found),
		    2, 0);
		for (i = 0; i < RMT_SAM_MAX_CLIENTS; i++,
		    client++, srvr_clnt++) {
			if ((srvr_clnt->flags & SRVR_CLNT_CONNECTED) &&
			    !(client->flags.bits & CLIENT_VSN_LIST)) {
				found->count = 0;
				/*
				 * Go through each equipment (reg exp)
				 * for this client and check the catalog
				 * entry.
				 */
			/* N.B. Bad indentation to meet cstyle requirements. */
			for (next_equ = client->first_equ;
			    next_equ != NULL && found->count == 0;
			    next_equ = next_equ->next) {
				if ((change->entry.status.bits & CES_NOT_ON) ||
				    change->entry.media != next_equ->media) {
					continue;
				}

				if ((next_equ->un->eq == change->eq) &&
				    ((change->entry.status.bits & CES_MATCH) ==
				    CES_MATCH) &&
				    run_regex(next_equ,
				    (char *)&change->entry.vsn)) {

					struct CatalogEntry ced;
					struct CatalogEntry *ce = &ced;

					(void) CatalogSync();
					ce = CatalogGetCeByMedia(
					    sam_mediatoa(change->entry.media),
					    change->entry.vsn, &ced);

					if (ce != NULL) {
						(void) add_vsn_entry(client,
						    found, ce, change->flags);
					}
				}
			}

			if (found->count != 0) {
				(void) flush_list(client, found);
			}
			}
		}
		free(found);
	}
	break;

	default:
		Trace(TR_MISC, "Unknown command %d", event->command);
	}
}

/*
 * Signal catcher to exit thread.
 */
static void sig_catch(void)
{
	thr_exit((void *) NULL);
}
