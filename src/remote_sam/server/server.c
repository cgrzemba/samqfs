/*
 * server.c - handle requests from connected clients.
 */
/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#pragma ident "$Revision: 1.21 $"

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
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/sockio.h>
#include <sys/socket.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/shm.h"
#include "sam/defaults.h"
#include "aml/remote.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "sam/sam_trace.h"
#include "sam/custmsg.h"
#include "server.h"

#include <sam/fs/bswap.h>

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Globals */
extern shm_alloc_t master_shm;

/* Function prototypes */
static void process_req(rmt_sam_request_t *reqp, rmt_sam_client_t *client);

/*
 *	Thread to process requests from the client.
 */
void *
serve_client(
	void *vclient)
{
	boolean_t running;
	rmt_sam_request_t *req;
	rmt_sam_client_t *client;
	dev_ent_t *un;
	srvr_clnt_t *srvr_clnt;

	running = B_TRUE;
	client = (rmt_sam_client_t *)vclient;
	un = client->un;
	srvr_clnt = client->srvr_clnt;
	req = (rmt_sam_request_t *)malloc_wait(
	    sizeof (rmt_sam_request_t), 2, 0);

	mutex_lock(&srvr_clnt->sc_mutex);	/* wait for caller to free */
	mutex_unlock(&srvr_clnt->sc_mutex);

	Trace(TR_MISC, "[%s] Waiting on fd %d for requests",
	    TrNullString(client->host_name), client->fd);

	while (running) {
		int dlen;

		dlen = read(client->fd, (char *)req, sizeof (*req));
		if (dlen != sizeof (*req)) {
			if (dlen == 0) {
				/*
				 * Client disconnected.
				 */
				running = B_FALSE;
			} else {
				Trace(TR_MISC, "[%s] Encountered short read %d",
				    TrNullString(client->host_name), dlen);
				reply_message(client, req, RESP_TYPE_UNK,
				    ENODATA);
			}
			continue;
		}
#if defined(__i386) || defined(__amd64)
		if (sam_byte_swap(rmt_sam_request_swap_descriptor,
		    req, sizeof (rmt_sam_request_t))) {
			SysError(HERE,
			    "Byteswap error on request from client %s",
			    TrNullString(client->host_name));
		}
#endif

		if (req->command == RMT_SAM_DISCONNECT) {
			Trace(TR_MISC, "[%s] Received disconnect (DISCONNECT)",
			    TrNullString(client->host_name));
			running = B_FALSE;
		} else {
			process_req(req, client);
		}
	}

	Trace(TR_MISC, "Disconnecting from '%s'",
	    TrNullString(client->host_name));

	mutex_lock(&srvr_clnt->sc_mutex);
	srvr_clnt->flags = 0;
	mutex_unlock(&srvr_clnt->sc_mutex);

	mutex_lock(&un->mutex);
	(void) close(client->fd);
	client->fd = -1;
	DEC_OPEN(un);
	mutex_unlock(&un->mutex);

	free(req);
	thr_exit(NULL);

	/* NOTREACHED */
	return (NULL);
}

void
reply_message(
	rmt_sam_client_t *client,
	rmt_sam_request_t *req,
	rmt_resp_type_t type,
	int err)
{
	int bytes;
	rmt_sam_request_t resp_req;
	rmt_sam_req_resp_t *resp;

	resp = &resp_req.request.req_response;
	memset(&resp_req, 0, sizeof (resp_req));

	resp_req.command = RMT_SAM_REQ_RESP;
	resp_req.version = req->version;
	resp_req.mess_addr = req->mess_addr;
	resp->type = type;
	resp->err = err;

#if defined(__i386) || defined(__amd64)
		if (sam_byte_swap(rmt_sam_request_swap_descriptor,
		    &resp_req, sizeof (rmt_sam_request_t))) {

			SysError(HERE, "Byteswap error on reply to client %s",
			    TrNullString(client->host_name));
		}

		if (sam_byte_swap(rmt_sam_req_resp_swap_descriptor,
		    resp, sizeof (rmt_sam_req_resp_t))) {
			SysError(HERE,
			    "Byteswap error on response to client %s",
			    TrNullString(client->host_name));
		}
#endif
	bytes = write(client->fd, (char *)&resp_req, sizeof (resp_req));

	if (bytes != sizeof (resp_req)) {
		SysError(HERE, "Write failed %d on send message (REQ_RESP)",
		    bytes);
	} else {
		Trace(TR_MISC, "[%s] Sent message (REQ_RESP) type: %d err %d",
		    TrNullString(client->host_name), type, err);
	}
}

/*
 * Process client request.
 */
static void
process_req(
	rmt_sam_request_t *reqp,
	rmt_sam_client_t *client)
{
	switch (reqp->command) {

	/* Respond to a heartbeat */
	case RMT_SAM_HEARTBEAT:
		Trace(TR_MISC, "sending heartbeat response");
		reply_message(client, reqp, RESP_TYPE_HEARTBEAT, 0);
		break;

	/*
	 * Send list of usable VSNs.
	 */
	case RMT_SAM_SEND_VSNS:
		Trace(TR_MISC, "[%s] Received send VSN request (SEND_VSNS)",
		    TrNullString(client->host_name));

		if (!(client->flags.bits & CLIENT_VSN_LIST)) {

			client->flags.bits |= CLIENT_VSN_LIST;

			if (thr_create(NULL, NULL, send_vsn_list,
			    (void *) client,
			    (THR_DETACHED | THR_BOUND | THR_NEW_LWP), NULL)) {

				client->flags.bits &= ~CLIENT_VSN_LIST;
				SysError(HERE,
				    "Unable to create send_vsn_list thread");
			}
		}
		break;

	default:
		Trace(TR_ERR, "[%s] Received unknown command %x",
		    TrNullString(client->host_name), reqp->command);
	}
}
	/* keep lint happy */
#if	defined(lint)
void
swapdummy_s(void *buf)
{
	sam_bswap2(buf, 1);
	sam_bswap4(buf, 1);
	sam_bswap8(buf, 1);
}
#endif /* defined(lint) */
