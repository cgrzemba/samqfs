/*
 * connects.c - connect with the server and wait.
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

#pragma ident "$Revision: 1.26 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <thread.h>
#include <synch.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stropts.h>
#include <syslog.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/shm.h"
#include "sam/defaults.h"
#include "aml/remote.h"
#include "client.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "sam/custmsg.h"
#include "sam/sam_trace.h"
#include "sam/spm.h"

#include <sam/fs/bswap.h>

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Globals */
extern shm_alloc_t master_shm;

/* Function prototypes */
static ushort_t server_port(dev_ent_t *un, char *serv_name);
static void check_rmt_request(dev_ent_t *un, rmt_sam_request_t *req);
static int tcpconnect(char *host, int port, int *error, char *errstr);
static ssize_t readn(int fd, char *ptr, size_t nbytes);

static boolean_t infiniteLoop = B_TRUE;

/*
 *	Main thread for the client process.  Connect to the server on the
 *	well known port and find the port to use for this client.  Close and
 *	connect to the server on this port.  Accept requests/commands from
 *	the server.  This thread never exits.
 */
void *
connect_server(
	void *vun)
{
	int srvr_sock;
	ushort_t srvr_port;
	char *buffer, *l_mess;
	dev_ent_t *un = (dev_ent_t *)vun;
	srvr_clnt_t *server = (srvr_clnt_t *)SHM_REF_ADDR(un->dt.sc.server);
	char *serv_name;
	struct in6_addr in6;
	struct hostent *h6_ent;
	int af;
	int h_err;

	l_mess = un->dis_mes[DIS_MES_NORM];
	buffer = (char *)malloc_wait(NI_MAXHOST, 2, 0);

	af = (server->flags & SRVR_CLNT_IPV6) ? AF_INET6 : AF_INET;
	memcpy(&in6.s6_addr, &server->control_addr6, sizeof (struct in6_addr));
	serv_name = buffer;
	if ((h6_ent =
	    getipnodebyaddr(&in6, sizeof (in6), AF_INET6, &h_err)) != NULL) {
		h_err = 0;
		strncpy(serv_name, h6_ent->h_name, NI_MAXHOST);
	} else {
		inet_ntop(af, &in6, serv_name, INET6_ADDRSTRLEN);
	}

	while (infiniteLoop) {
		int err;

		mutex_lock(&un->mutex);
		un->status.bits = 0;
		mutex_unlock(&un->mutex);

		/*
		 * Find out what port to use.
		 */
		srvr_port = server_port(un, serv_name);
		server->port = srvr_port;

		mutex_lock(&un->mutex);
		un->status.b.present = TRUE;
		mutex_unlock(&un->mutex);

		Trace(TR_MISC, "Connecting to server '%s' on port %d",
		    TrNullString(serv_name), srvr_port);
		sprintf(l_mess, "connecting to remote server %s", serv_name);

		/*
		 * Get a socket to the server to send requests on.
		 */
		srvr_sock = tcpconnect(serv_name, srvr_port, &err, NULL);
		if (srvr_sock == -1) {
			SysError(HERE,
			    "Unable to connect to server %s on port %d",
			    serv_name, srvr_port);
			sleep(30);
			continue;
		}

		Trace(TR_MISC, "Server '%s' connected",
		    TrNullString(serv_name));
		sprintf(l_mess, "Remote server %s connected", serv_name);

		mutex_lock(&un->mutex);
		un->open_count = 0;
		un->status.bits = DVST_READY | DVST_PRESENT;
		un->space = srvr_sock;
		mutex_unlock(&un->mutex);

		Trace(TR_MISC, "Requesting VSN list on socket %d port %d",
		    (int)un->space, (int)server->port);
		request_vsn_list(un);

		while (infiniteLoop) {
			int io_len;
			rmt_sam_request_t *req;

			req = malloc_wait(sizeof (*req), 2, 0);
			io_len = readn(srvr_sock, (char *)req,
			    sizeof (rmt_sam_request_t));
			if (io_len == sizeof (*req)) {
#if defined(__i386) || defined(__amd64)
				Trace(TR_DEBUG,
				    "Byte swapping on con %d, port %d",
				    srvr_sock, srvr_port);
				if (sam_byte_swap(
				    rmt_sam_request_swap_descriptor,
				    req, sizeof (rmt_sam_request_t))) {
					SysError(HERE,
					    "Byteswap error on server %s "
					    "request", serv_name);
				}
#endif
				check_rmt_request(un, req);
			} else {
				free(req);
				if (io_len == 0) {
					Trace(TR_MISC,
					    "Connection broken on %d, port %d",
					    srvr_sock, srvr_port);

					(void) close(srvr_sock);

					un->status.bits &= ~DVST_READY;

					Trace(TR_MISC,
					    "Server '%s' has disconnected",
					    TrNullString(serv_name));
					sprintf(l_mess,
					    "server %s has disconnected",
					    serv_name);

					sleep(30);
					break;
				}
				SysError(HERE, "Bad read request");
			}
		}
		mark_catalog_unavail(un);
	}

#ifdef lint
	/* NOTREACHED */
	return (NULL);
#endif
}

/*
 * Connect to the server on the well known port and
 * find out what port this client should use.
 */
static ushort_t
server_port(
	dev_ent_t *un,
	char *serv_name)
{
	int cli_sock;
	ushort_t cont_port;
	rmt_sam_request_t request;
	char *l_mess;
	int nbytes;
	int errcode;
	char errbuf[SPM_ERRSTR_MAX];
	char service_name[SPM_SERVICE_NAME_MAX];
	boolean_t connect_mes = B_TRUE;
	rmt_sam_connect_t *cnt = &request.request.connect_req;
	rmt_sam_cnt_resp_t *response = &request.request.con_response;

	l_mess = un->dis_mes[DIS_MES_NORM];

	memset(&request, 0, sizeof (request));

	cont_port = 0;
	while (cont_port == 0) {

		sprintf(l_mess, "connecting to server %s", serv_name);
		Trace(TR_MISC, "Connecting to server %s",
		    TrNullString(serv_name));

		snprintf(service_name, SPM_SERVICE_NAME_MAX, "%s",
		    SAMREMOTE_SPM_SERVICE_NAME);

		Trace(TR_MISC, "Trying to connect to server %s, service %s",
		    TrNullString(serv_name), TrNullString(service_name));
		sprintf(l_mess, "trying to connect to server %s", serv_name);

		cli_sock = spm_connect_to_service(service_name, serv_name,
		    NULL, &errcode, errbuf);

		if (cli_sock < 0) {
			if (connect_mes == B_TRUE) {
				SendCustMsg(HERE, 22312, serv_name,
				    errcode, errbuf);
			}
			sprintf(l_mess,
			    "connection failed to server %s, retrying",
			    serv_name);

			connect_mes = B_FALSE;
			sleep(30);
			continue;
		}

		connect_mes = B_TRUE;
		/* fset_name is character data, don't have to byteswap */
		strcpy(cnt->fset_name, un->set);

		request.command = RMT_SAM_CONNECT;
		request.version = RMT_SAM_VERSION;
#if defined(__i386) || defined(__amd64)
		Trace(TR_DEBUG, "Byte swapping on conreq %s", serv_name);
		if (sam_byte_swap(rmt_sam_request_swap_descriptor,
		    &request, sizeof (request))) {
			SysError(HERE, "Byteswap error on connect to server %s",
			    serv_name);
		}
#endif
		if (write(cli_sock, &request, sizeof (request)) !=
		    sizeof (request)) {
			SysError(HERE, "Write error on connect to server %s",
			    serv_name);
			(void) close(cli_sock);
			sleep(30);
			continue;
		}
		Trace(TR_MISC, "Sent connect request (SAM_CONNECT)");

		nbytes = readn(cli_sock, (char *)&request, sizeof (request));
		if (nbytes != sizeof (request)) {
			SysError(HERE,
			    "Protocol error during connect (read %d) to %s",
			    nbytes, serv_name);
			(void) close(cli_sock);
			sleep(30);
			continue;
		}
#if defined(__i386) || defined(__amd64)
		Trace(TR_DEBUG, "Byte swapping on conrpl %s", serv_name);
		if (sam_byte_swap(rmt_sam_request_swap_descriptor,
		    &request, sizeof (request))) {
			SysError(HERE, "Byteswap error on con reply, server %s",
			    serv_name);
		}
		/* it must be a connect response */
		if (sam_byte_swap(rmt_sam_cnt_resp_swap_descriptor,
		    response, sizeof (rmt_sam_cnt_resp_t))) {
			SysError(HERE, "Byteswap error on con resp, server %s",
			    serv_name);
		}
#endif

		if (request.command != RMT_SAM_CONNECT) {
			Trace(TR_ERR,
			    "Protocol error during connect (bad reply %d) "
			    "to %s",
			    request.command, TrNullString(serv_name));
			(void) close(cli_sock);
			sleep(30);
			continue;
		}

		if (response->err) {
			Trace(TR_ERR, "Error during connect to %s, %d:%s",
			    TrNullString(serv_name), response->err,
			    TrNullString(strerror(response->err)));
			(void) close(cli_sock);
			sleep(30);
			continue;
		}

		cont_port = response->serv_port;
		if (cont_port == 0) {
			Trace(TR_MISC,
			    "Received server (%s) port 0 with no error",
			    TrNullString(serv_name));
			(void) close(cli_sock);
			sleep(30);
			continue;
		}
		Trace(TR_MISC,
		    "Received connect response from server %s, port %d",
		    TrNullString(serv_name), cont_port);
	}

	(void) close(cli_sock);

	Trace(TR_MISC, "Connection for %s(%d)-%s port %d",
	    TrNullString(un->set), un->eq,
	    TrNullString(serv_name), response->serv_port);

	return (response->serv_port);
}


static int
tcpconnect(
	char *host,
	int port,
	int *error,
	char *errstr)
{
	struct addrinfo hints;
	int retval;
	int sockfd;
	struct addrinfo *srvr;
	struct addrinfo *srvr_save;
	char portstr[12];

	snprintf(portstr, 12, "%d", port);
	memset((void *) &hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((retval = getaddrinfo(host, portstr, &hints, &srvr)) != 0) {
		if (errstr) {
			snprintf(errstr, SPM_ERRSTR_MAX,
			    "tcpconnect error for %s: %s",
			    host, gai_strerror(retval));
		}
		*error = ESPM_AI;
		return (-1);
	}

	srvr_save = srvr;

	do {
		sockfd = socket(srvr->ai_family, srvr->ai_socktype,
		    srvr->ai_protocol);
		if (sockfd < 0) {
			continue;
		}

		if (connect(sockfd, srvr->ai_addr, srvr->ai_addrlen) == 0) {
			break;
		}

		close(sockfd);
	} while ((srvr = srvr->ai_next) != NULL);

	freeaddrinfo(srvr_save);

	if (srvr == NULL) {
		if (errstr) {
			snprintf(errstr, SPM_ERRSTR_MAX,
			    "tcpconnect error for %s: %s",
			    host, strerror(errno));
		}
		*error = ESPM_CONNECT;
		return (-1);
	} else {
		*error = ESPM_OK;
		return (sockfd);
	}
}

static void
check_rmt_request(
	dev_ent_t *un,
	rmt_sam_request_t *req)
{
	switch (req->command) {
	case RMT_SAM_UPDATE_VSN:
		Trace(TR_MISC, "Received update catalog command (UPDATE_VSN)");
#if defined(__i386) || defined(__amd64)
		Trace(TR_DEBUG, "Byte swapping cat update");
		if (sam_byte_swap(rmt_sam_update_vsn_swap_descriptor,
		    &req->request.update_vsn, sizeof (rmt_sam_update_vsn_t))) {
			SysError(HERE, "Byteswap error on catalog update");
		}
#endif
		update_catalog_entries(un, req);
		free(req);
		break;

	case RMT_SAM_REQ_RESP:
		Trace(TR_MISC, "Send response to request (REQ_RESP)");
#if defined(__i386) || defined(__amd64)
		Trace(TR_DEBUG, "Byte swapping req response");
		if (sam_byte_swap(rmt_sam_req_resp_swap_descriptor,
		    &req->request.req_response, sizeof (rmt_sam_req_resp_t))) {
			SysError(HERE, "Byteswap error on req response");
		}
#endif
		if (req->mess_addr != NULL) {

			switch (req->request.req_response.type) {

			case RESP_TYPE_HEARTBEAT:
			{
				Trace(TR_MISC,
				    "Response type RESP_TYPE_HEARTBEAT");
			}
			break;

			case RESP_TYPE_CMD:
			{
				rmt_message_t *message;

				message = (rmt_message_t *)req->mess_addr;
				Trace(TR_MISC, "Response type (TYPE_CMD) "
				    "addr: 0x%x err: 0x%x flags: 0x%x",
				    (int)req->mess_addr,
				    req->request.req_response.err,
				    req->request.req_response.flags);

				mutex_lock(&message->mutex);
				message->complete =
				    req->request.req_response.err;
				(void) cond_signal(&message->cond);
				mutex_unlock(&message->mutex);
			}
			break;

			default:
				Trace(TR_MISC, "Unknown response type (%d) "
				    "addr: 0x%x err: 0x%x flags: 0x%x",
				    req->request.req_response.type,
				    (int)req->mess_addr,
				    req->request.req_response.err,
				    req->request.req_response.flags);
			}
		}
		free(req);
		break;

	default:
		Trace(TR_MISC, "Received bad request: 0x%x", req->command);
		free(req);
	}
}


/*
 * Read from socket.
 */
ssize_t
readn(
	int fd,
	char *ptr,
	size_t nbytes)
{
	size_t nleft;
	size_t nread;
	int nfds;
	int retry = 1;
	fd_set allfds;
	fd_set readfds;
	struct timeval tv;
	sam_defaults_t *defaults;

	defaults = GetDefaults();

	nleft = nbytes;

	tv.tv_sec = defaults->remote_keepalive;
	tv.tv_usec = 0;

	FD_ZERO(&allfds);
	FD_SET(fd, &allfds);

	while (nleft > 0) {

		readfds = allfds;

		nfds = select(fd + 1, &readfds, NULL, NULL, &tv);
		if (nfds < 0) {
			if (errno == EINTR) {
				continue;
			}
			Trace(TR_MISC,
			    "select error, socket not available, errno = %d",
			    errno);
			return (-1);
		} else if (nfds == 0 && defaults->remote_keepalive != 0) {
			rmt_sam_request_t request;

			Trace(TR_MISC, "select timeout");
			/*
			 * Send a heartbeat to see if the
			 * server is still alive. If not,
			 * disconnect and retry connection.
			 */
			request.command = RMT_SAM_HEARTBEAT;
			request.version = RMT_SAM_VERSION;
#if defined(__i386) || defined(__amd64)
			Trace(TR_DEBUG, "Byte swapping heartbeat request");
			if (sam_byte_swap(rmt_sam_request_swap_descriptor,
			    &request, sizeof (request))) {
				SysError(HERE, "Byteswap error on hearbeat");
			}
#endif
			if (write(fd, &request, sizeof (request)) !=
			    sizeof (request)) {
				Trace(TR_MISC, "write of heartbeat failed");
				return (0);
			} else {
				Trace(TR_DEBUG, "write of heartbeat succeeded");
			}
		}

		/*
		 * If data has arrived from server.
		 */
		if (FD_ISSET(fd, &readfds)) {

			Trace(TR_MISC,
			    "reading socket %d for %d bytes (0x%x)",
			    fd, nleft, ptr);

			nread = read(fd, ptr, nleft);

			if ((long)nread <= 0) {
				Trace(TR_MISC, "Read socket %d failed %d",
				    fd, errno);
				return (0);
			}

			nleft -= nread;
			ptr += nread;
		}
	}

	return (nbytes - nleft);
}
	/*	keep lint happy */
#if   defined(lint)
void
swapdummy_c(void *buf)
{
	sam_bswap2(buf, 1);
	sam_bswap4(buf, 1);
	sam_bswap8(buf, 1);
}
#endif /* defined(lint) */
