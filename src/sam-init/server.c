/*
 * server.c - start remote-sam server.
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

#pragma ident "$Revision: 1.27 $"

/* ANSI C headers. */
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <thread.h>
#include <synch.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>

/* Solaris headers. */
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/* SAM headers. */
#include "sam/types.h"
#include "aml/device.h"
#include "sam/param.h"
#include "sam/lib.h"
#include "aml/shm.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"
#include "sam/spm.h"
#include <sam/fs/bswap.h>

/* Local headers. */
#include "amld.h"

static char *_SrcFile = __FILE__;  /* Using __FILE__ makes duplicate strings */

/* local structs and function prototypes */
struct connection {
	int fd;
	int addr_len;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
};

static void *process_connection(void *vconnect);
static void rmt_server(void);


/*
 * thread to find the remote sam library and call the rmt_server routine.
 */
void *
rmt_server_thr(
	void *arg)
{
	dev_ent_t *device;

	for (device = (dev_ent_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);
	    device != NULL;
	    device = (dev_ent_t *)SHM_REF_ADDR(device->next)) {
		if (device->equ_type == DT_PSEUDO_SS ||
		    device->equ_type == DT_PSEUDO_SC) {
			break;
		}
	}
	if (device != NULL) {
		rmt_server();	/* should only return on shutdown */
	}
	thr_exit(NULL);
	/* return (arg); */
}


/*
 * rmt_server - called if a remote sam server is defined
 * in the mcf.  This function will sit on the well known port and
 * wait for connection requests.  It will then pass back the port
 * number of the server serving that family.  It is up to that server
 * to validate the client when the connect request arrives.
 */
static void
rmt_server(
	void)
{
	struct servent server_info;
	int errcode;
	int reg_fail = 1;
	int	server_sock;
	char errbuf[SPM_ERRSTR_MAX];
	char service_name[SPM_SERVICE_NAME_MAX];
	long cliaddr_buf[64];
	struct sockaddr *cliaddr = (struct sockaddr *)cliaddr_buf;
	socklen_t cliaddr_len = sizeof (cliaddr_buf);

	memset(&server_info, 0, sizeof (server_info));

	snprintf(service_name, SPM_SERVICE_NAME_MAX, "%s",
	    SAMREMOTE_SPM_SERVICE_NAME);

	while (reg_fail > 0) {
		server_sock =
		    spm_register_service(service_name, &errcode, errbuf);
		if (server_sock < 0) {
			if (reg_fail++ == 1) {
				SendCustMsg(HERE, 22019,
				    service_name, errcode, errbuf);
			}
			Trace(TR_MISC, "'%s' failed to register %d %s",
			    service_name, errcode, errbuf);
			sleep(30);
		} else {
			reg_fail = 0;
		}
	}

	Trace(TR_MISC, "'%s' registered fd: %d", service_name, server_sock);

	/* CONSTCOND */
	while (TRUE) {
		struct connection *cur_conc;

		cur_conc = (struct connection *)
		    malloc_wait(sizeof (struct connection), 2, 0);

		cur_conc->fd = spm_accept(server_sock, &errcode, errbuf);

		if (cur_conc->fd >= 0) {

			if (getpeername(cur_conc->fd, cliaddr, &cliaddr_len) <
			    0) {
				SysError(HERE, "getpeername failed");
				(void) close(cur_conc->fd);
				continue;
			}

			if (cliaddr->sa_family == AF_INET) {
				struct sockaddr_in  *sin =
/* LINTED pointer cast may result in improper alignment */
				    (struct sockaddr_in *)cliaddr;

				cur_conc->addr_len = sizeof (cur_conc->sin);
				memcpy(&cur_conc->sin, sin,
				    sizeof (cur_conc->sin));

			} else if (cliaddr->sa_family == AF_INET6) {
				struct sockaddr_in6  *sin6 =
/* LINTED pointer cast may result in improper alignment */
				    (struct sockaddr_in6 *)cliaddr;

				cur_conc->addr_len = sizeof (cur_conc->sin6);
				memcpy(&cur_conc->sin6, sin6,
				    sizeof (cur_conc->sin6));

			} else {
				SendCustMsg(HERE, 22018, cliaddr->sa_family);
				continue;
			}

			(void) process_connection((void *)cur_conc);

		} else {
			SendCustMsg(HERE, 22020, errcode, errbuf);
			free(cur_conc);
		}
	}
}


static void *
process_connection(
	void *vconnect
)
{
	rmt_sam_request_t request;
	rmt_sam_cnt_resp_t *response;
	struct in_addr in;
	struct in6_addr in6;
	struct hostent h_ent;
	struct hostent *h_entp;
	struct connection *cnt = (struct connection *)vconnect;
	char	*ent_pnt = "process_connection";
	char	*buffer;
	int		data_len = 0, h_err = TRY_AGAIN;
	int		s_req;

	buffer = (char *)malloc_wait(4096, 2, 0);
	s_req = sizeof (request);
	if (cnt->addr_len == sizeof (struct sockaddr_in)) {
		in.s_addr = cnt->sin.sin_addr.s_addr;
	} else {
		memcpy(in6.s6_addr, cnt->sin6.sin6_addr.s6_addr,
		    sizeof (in6.s6_addr));
	}
	while (h_err == TRY_AGAIN && data_len < 5) {
		if (cnt->addr_len == sizeof (struct sockaddr_in)) {
			h_entp =
			    gethostbyaddr_r((char *)&in, sizeof (in), AF_INET,
			    &h_ent, buffer, 4096, &h_err);
		} else {
			h_entp = getipnodebyaddr(&in6, sizeof (in6), AF_INET6,
			    &h_err);
		}
		if (h_entp != NULL) {
			h_err = 0;
		} else {
			data_len++;
			sleep(5);
		}
	}
	if (h_entp != NULL) {
		Trace(TR_MISC, "Process conn fd: %d host: %s",
		    cnt->fd, h_entp->h_name);
	} else {
		Trace(TR_MISC, "Process conn failed fd: %d", cnt->fd);
	}

	response = &(request.request.con_response);

	data_len = read(cnt->fd, &request, s_req);

#if defined(__i386) || defined(__amd64)
	if (sam_byte_swap(rmt_sam_request_swap_descriptor,
	    &request, sizeof (rmt_sam_request_t))) {
		sam_syslog(LOG_INFO, "%s: Byteswap error:connect: %s.", ent_pnt,
		    inet_ntoa(in));
	}
#endif	/* defined(__i386) || defined(__amd64) */

	if (data_len != s_req) {
		sam_syslog(LOG_INFO,
		    "%s: Connect read error %s", ent_pnt, inet_ntoa(in));
		response->err = ENODATA;
	} else if (request.command != RMT_SAM_CONNECT) {
		sam_syslog(LOG_INFO, "%s: Protocol error:connect: %s.", ent_pnt,
		    inet_ntoa(in));
		response->err = EBADR;
	} else if (h_err != 0) {
		sam_syslog(LOG_INFO, "%s: Connect: unknown host(%s)%#x",
		    ent_pnt, inet_ntoa(in), h_err);
		response->err = EACCES;
	} else if (request.version != RMT_SAM_VERSION) {
		sam_syslog(LOG_INFO,
		    "%s: SAM-Remote client (%d) and server (%d) versions"
		    " do not match",
		    ent_pnt, request.version, RMT_SAM_VERSION);
		response->err = EACCES;
	} else {
		dev_ent_t *device;

		for (device = (dev_ent_t *)SHM_REF_ADDR(
		    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);
		    device != NULL;
		    device = (dev_ent_t *)SHM_REF_ADDR(device->next)) {
			if (strcmp(device->set,
			    request.request.connect_req.fset_name) == 0) {
				mutex_lock(&device->mutex);
				if ((response->serv_port =
				    device->dt.ss.serv_port) == 0) {
					response->err = EPIPE;
					if (DBG_LVL(SAM_DBG_DEBUG)) {
						sam_syslog(LOG_DEBUG,
						    "%s: Connect request(%s)%s-"
						    " no server",
						    ent_pnt, request.request.
						    connect_req.fset_name,
						    h_entp->h_name);
					}
				} else {
					response->err = 0;
					if (DBG_LVL(SAM_DBG_DEBUG)) {
						sam_syslog(LOG_DEBUG,
						    "%s: Connect request(%s)%s-"
						    " port %d",
						    ent_pnt, request.request.
						    connect_req.fset_name,
						    h_entp->h_name,
						    response->serv_port);
					}
				}
				mutex_unlock(&device->mutex);
				break;
			}
		}
		if (device == NULL) {
			sam_syslog(LOG_INFO, "%s: Connect req-no match(%s)%s",
			    ent_pnt, request.request.connect_req.fset_name,
			    h_entp->h_name);
			response->err = ENOENT;
		}
	}
#if defined(__i386) || defined(__amd64)
	if (sam_byte_swap(rmt_sam_request_swap_descriptor,
	    &request, sizeof (rmt_sam_request_t))) {
		sam_syslog(LOG_INFO, "%s: Byteswap error:cntreq: %s.", ent_pnt,
		    inet_ntoa(in));
	}
	if (sam_byte_swap(rmt_sam_cnt_resp_swap_descriptor,
	    response, sizeof (rmt_sam_cnt_resp_t))) {
		sam_syslog(LOG_INFO, "%s: Byteswap error:cntresp: %s.", ent_pnt,
		    inet_ntoa(in));
	}
#endif	/* defined(__i386) || defined(__amd64) */

	if (write(cnt->fd, &request, s_req) != s_req) {
		sam_syslog(LOG_INFO, "%s: Write length error.", ent_pnt);
	} else if (DBG_LVL(SAM_DBG_DEBUG)) {
		sam_syslog(LOG_DEBUG, "%s: Sent connect response (error %d)",
		    ent_pnt, response->err);
	}
	(void) close(cnt->fd);
	free(buffer);
	free(vconnect);
#if defined(USE_A_THREAD)
	thr_exit(NULL);
#else
	return (NULL);
#endif
}
	/*	keep lint happy */
#if	defined(lint)
void
swapdummy_s(void *buf)
{
	sam_bswap2(buf, 1);
	sam_bswap4(buf, 1);
	sam_bswap8(buf, 1);
}
#endif /* defined(lint) */
