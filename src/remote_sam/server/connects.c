/*
 * connects.c - create socket and wait for initial connections
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

static char *_SrcFile = __FILE__;  /* Using __FILE__ makes duplicate strings */
#pragma ident "$Revision: 1.20 $"

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
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>

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

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Globals */
extern shm_alloc_t master_shm;

/* Locals */
static	boolean_t infiniteLoop = B_TRUE;
static	srvr_clnt_t *next_clnt;

static	boolean_t validate_host(struct hostent *h_ent, srvr_clnt_t *srvr_clnt);

/* Macros */
#define	s6_addr32	_S6_un._S6_u32

#ifdef _BIG_ENDIAN
#define	V6_MAPPED_INADDR(a)	(((a).s6_addr32[2] == 0xffffU || \
				(a).s6_addr32[2] == 0) &&               \
				(a).s6_addr32[1] == 0 &&                \
				(a).s6_addr32[0] == 0)

#else
#define	V6_MAPPED_INADDR(a)	(((a).s6_addr32[2] == 0xffff0000U ||     \
				(a).s6_addr32[2] == 0) &&               \
				(a).s6_addr32[1] == 0 &&                \
				(a).s6_addr32[0] == 0)
#endif /* _BIG_ENDIAN */

/*
 * Thread that accepts (or rejects) connections from clients.
 */
void *
watch_connects(
	void *vun)
{
	int one = 1;
	int server_sock;
	int retval;
	int port;
	boolean_t connected = B_FALSE;
	char	portstr[12];
	char *buffer;
	struct addrinfo hints;
	struct addrinfo *srvr;
	struct addrinfo *srvr_save;
	dev_ent_t *un;
	srvr_clnt_t *srvr_clnt;
	rmt_sam_client_t *clnt;
	socklen_t	addrlen;

	un = (dev_ent_t *)vun;
	srvr_clnt = (srvr_clnt_t *)SHM_REF_ADDR(un->dt.ss.clients);
	clnt = (rmt_sam_client_t *)un->dt.ss.private;

	Trace(TR_PROC, "Watch connects thread started");
	buffer = (char *)malloc_wait(1024, 2, 0);

	memset((void *)&hints, 0, sizeof (hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	for (port = RMT_SAM_PORTS + un->dt.ss.ordinal;
	    port < RMT_SAM_PORTS + 1000; port += 10) {
		snprintf(portstr, 12, "%d", port);
		if ((retval = getaddrinfo(NULL, portstr, &hints, &srvr)) != 0) {
			SysError(HERE, "getaddrinfo failed %s",
			    gai_strerror(retval));
			thr_exit(NULL);
		}
		srvr_save = srvr;

		do {
			Trace(TR_PROC,
			    "Watch connects: af %d, type %d proto %d port %d",
			    srvr->ai_family, srvr->ai_socktype,
			    srvr->ai_protocol, port);
			server_sock = socket(srvr->ai_family,
			    srvr->ai_socktype, srvr->ai_protocol);
			if (server_sock < 0) continue;

			if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR,
			    (char *)&one, sizeof (int))) {
				SysError(HERE, "Set socket options failed");
			}
			if (bind(server_sock, srvr->ai_addr,
			    srvr->ai_addrlen) == 0) {
				connected = B_TRUE;
				break;
			}
			close(server_sock);
		} while ((srvr = srvr->ai_next) != NULL);
		if (connected) break;
		freeaddrinfo(srvr_save);
	}

	if (srvr == NULL) {
		SysError(HERE, "Create socket or bind failed");
		thr_exit(NULL);
	}

	addrlen = srvr->ai_addrlen;
	freeaddrinfo(srvr_save);

	Trace(TR_PROC, "Listening on port %s", portstr);

	if (listen(server_sock, RMT_SAM_MAX_CLIENTS)) {
		SysError(HERE, "listen failed");
		thr_exit(NULL);
	}

	mutex_lock(&un->mutex);
	un->dt.ss.serv_port = port;
	un->open_count = 0;
	un->status.bits = DVST_READY | DVST_PRESENT;
	mutex_unlock(&un->mutex);

	/* Loop forever accepting connections */
	while (infiniteLoop) {
		boolean_t valid = B_FALSE;
		int fd;
		const char *cliaddr;
		socklen_t sock_addr_len = addrlen;
		rmt_sam_client_t *n_clnt;
		struct sockaddr_in6 client_sockaddr;
		struct sockaddr_in *sin;
		struct sockaddr_in6 *sin6;
		struct sockaddr *ssin;

		sin = (struct sockaddr_in *)&client_sockaddr;
		sin6 = &client_sockaddr;
		ssin = (struct sockaddr *)&client_sockaddr;

		memset(&client_sockaddr, 0, sizeof (struct sockaddr_in6));

		fd = accept(server_sock, ssin, &sock_addr_len);

		if (fd < 0) {
			/* Accept failed, log and release resources */
			SysError(HERE, "accept connection on a socket failed");
			continue;
		}
		/* New connection, validate the client */
		int h_err = TRUE;
		int aport;
		struct hostent *h_ent;

		if (ssin->sa_family == AF_INET6) {
			cliaddr = inet_ntop(AF_INET6,
			    &sin6->sin6_addr.s6_addr[0],
			    buffer, 1024);
			aport = ntohs(sin6->sin6_port);
			h_ent = getipnodebyaddr(
			    &sin6->sin6_addr.s6_addr[0],
			    sock_addr_len, AF_INET6, &h_err);
			if (h_ent == NULL) {
				h_err = 0;
				h_ent = getipnodebyaddr(
				    &sin6->sin6_addr.s6_addr[12], 4,
				    AF_INET, &h_err);
				Trace(TR_MISC,
				    "[%s] Lookup v6 as v4 %d",
				    cliaddr, h_err);
			}
		} else {
			cliaddr = inet_ntop(AF_INET,
			    &sin->sin_addr.s_addr, buffer, 1024);
			aport = ntohs(sin->sin_port);
			h_ent = getipnodebyaddr(&sin->sin_addr.s_addr,
			    sock_addr_len, AF_INET, &h_err);
		}
		Trace(TR_MISC,
		    "Accept on socket, af %d, addr %s client port %d",
		    ssin->sa_family, cliaddr, aport);
		if (h_ent != NULL) {
			h_err = 0;
		} else {
			Trace(TR_MISC, "[%s] Unknown host error %d",
			    cliaddr, h_err);
			SysError(HERE, "unknown host");
			freehostent(h_ent);
			thr_exit(NULL);
		}

		Trace(TR_MISC, "[%s] Connection requested",
		    TrNullString(h_ent->h_name));

		mutex_lock(&un->mutex);
		if (un->open_count >= RMT_SAM_MAX_CLIENTS) {
			SendCustMsg(HERE, 22302, h_ent->h_name);
			(void) close(fd);
			fd = -1;
			mutex_unlock(&un->mutex);
			continue;
		}
		valid = validate_host(h_ent, srvr_clnt);

		/*
		 * Not found in list of clients.
		 * Connection not authorized.
		 */
		if (!valid) {
			(void) close(fd);
			fd = -1;
			freehostent(h_ent);
			mutex_unlock(&un->mutex);
			continue;
		}

		INC_OPEN(un);
		mutex_unlock(&un->mutex);

		next_clnt->port = aport;
		n_clnt = clnt + next_clnt->index;
		n_clnt->fd = fd;
		n_clnt->srvr_clnt = next_clnt;
		if (ssin->sa_family == AF_INET6) {
			memcpy(&n_clnt->client_addr6.s6_addr[0],
			    &sin6->sin6_addr.s6_addr[0],
			    sizeof (struct in6_addr));
		} else {
			memcpy(&n_clnt->client_addr.s_addr,
			    &sin->sin_addr.s_addr,
			    sizeof (struct in_addr));
		}
		n_clnt->host_name = strdup(h_ent->h_name);

		if (thr_create(NULL, DF_THR_STK, serve_client,
		    (void *) (n_clnt), (THR_DETACHED | THR_BOUND), NULL)) {
			SysError(HERE, "Thread creation failed: %s",
			    h_ent->h_name);
			(void) close(fd);
			mutex_unlock(&next_clnt->sc_mutex);

			mutex_lock(&un->mutex);
			DEC_OPEN(un);
			mutex_unlock(&un->mutex);
		} else {
			next_clnt->flags |= SRVR_CLNT_CONNECTED;
			mutex_unlock(&next_clnt->sc_mutex);
			Trace(TR_MISC,
			    "[%s] Accepted connection",
			    TrNullString(h_ent->h_name));
		}
	}
	/* LINTED Function has no return statement */
}

static boolean_t
validate_host(
	struct hostent *h_ent,
	srvr_clnt_t *srvr_clnt)
{
	boolean_t found = B_FALSE;
	char **p;
	int adr_temp;
	int i;

	/* Validate this host  */
	for (next_clnt = srvr_clnt, i = 0;
	    i < RMT_SAM_MAX_CLIENTS; next_clnt++, i++) {
		if ((next_clnt->flags & SRVR_CLNT_PRESENT) == 0) {
			break;
		}

		mutex_lock(&next_clnt->sc_mutex);
		for (p = h_ent->h_addr_list; *p != 0; p++) {
			boolean_t ismapped;
			boolean_t isv6 = B_FALSE;
			char *adr = *p;
			char *adr_v4 = &adr[0];
			uchar_t *addr;
			uchar_t *srvaddr;
			char *atype;
			struct in6_addr *addr6 = (struct in6_addr *)adr;
			int addrsize;

			atype = "IPv4";
			if (h_ent->h_addrtype == AF_INET6) {
				ismapped = V6_MAPPED_INADDR(*addr6);
				if (ismapped) {
					adr_v4 = &adr[12];
					atype = "IPv4comp";
				} else {
					isv6 = B_TRUE;
					atype = "IPv6";
				}
			}

			if (isv6 && (next_clnt->flags & SRVR_CLNT_IPV6) != 0) {
				adr_temp = *(int *)&adr[12];
				addr = (uchar_t *)&adr[0];
				srvaddr = &next_clnt->control_addr6.s6_addr[0];
				addrsize = sizeof (struct in6_addr);
			} else {
				adr_temp = *(int *)adr_v4;
				addr = (uchar_t *)adr_v4;
				srvaddr = (uchar_t *)
				    &next_clnt->control_addr.s_addr;
				addrsize = sizeof (struct in_addr);
			}

			if (memcmp(addr, srvaddr, addrsize) == 0) {
				if (next_clnt->flags & SRVR_CLNT_CONNECTED) {
					mutex_unlock(&next_clnt->sc_mutex);
					Trace(TR_MISC, "%s already connected",
					    TrNullString(h_ent->h_name));
					return (B_FALSE);
				} else {
					Trace(TR_MISC, "%s authorized %s",
					    TrNullString(h_ent->h_name), atype);
					found = B_TRUE;
				}
				break;	/* break for address list */
			} else {
				Trace(TR_MISC, "No match %s", atype);
			}
		}	/* end for p */
		mutex_unlock(&next_clnt->sc_mutex);

		if (found) break;	/* break for client list */
	}	/* end for next_clnt */
	if (!found) {
		SendCustMsg(HERE, 22301, h_ent->h_name, adr_temp);
	}
	return (found);
}
