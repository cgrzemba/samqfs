/*
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

#pragma ident "$Revision: 1.19 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/sam_malloc.h"
#include "aml/sam_rft.h"

/* Local headers. */
#include "rft_defs.h"

static void initCrew(SamrftCrew_t *crew);
static int initDataConnection(SamrftImpl_t *rftd, int tcpwindowsize);

int
CreateCrew(
	SamrftImpl_t *rftd,
	int	num_dataports,
	int dataportsize,
	int tcpwindowsize)
{
	SamrftCrew_t *crew;

	SamMalloc(rftd->crew, sizeof (SamrftCrew_t));
	(void) memset(rftd->crew, 0, sizeof (SamrftCrew_t));
	crew = rftd->crew;
	initCrew(crew);

	crew->num_dataports = num_dataports ? num_dataports : 1;
	crew->dataportsize = dataportsize;
	SamMalloc(crew->buf, dataportsize);
	Trace(TR_RFT, "Samrft Block size configuration= %d", dataportsize);
	Trace(TR_RFT, "Samrft TCP window size configuration= %d",
	    tcpwindowsize);

	return (initDataConnection(rftd, tcpwindowsize));
}

/*
 * Send local data over socket to rft daemon.
 */
size_t
SendData(
	SamrftImpl_t *rftd,
	char *buf,
	size_t nbytes)
{
	size_t data_to_send;
	size_t blksize;
	size_t reqsize;
	size_t nbytes_written;
	SamrftCrew_t *crew;

	crew = rftd->crew;
	data_to_send = nbytes;
	blksize = crew->dataportsize;

	while (data_to_send > 0) {
		reqsize = (data_to_send <= blksize) ? data_to_send : blksize;

		Trace(TR_RFT, "Samrft write socket %d for %d bytes [0x%x]",
		    fileno(crew->out), reqsize, buf);
		nbytes_written = write(fileno(crew->out), buf, reqsize);
		if (nbytes_written != reqsize) {
			Trace(TR_RFT, "Samrft write failed %d", errno);
			break;
		}
		data_to_send -= reqsize;
		buf += reqsize;
	}
	return (nbytes);
}

/*
 * Receive data over socket from daemon on host machine.
 */
size_t
ReceiveData(
	SamrftImpl_t *rftd,
	char *buf,
	size_t nbytes
)
{
	ssize_t data_to_receive;
	ssize_t nbytes_read;
	ssize_t nbytes_expected;
	ssize_t nbytes_exp_netord;
	extern ssize_t readn(int fd, char *ptr, size_t nbytes);

	SamrftCrew_t *crew;

	crew = rftd->crew;
	data_to_receive = nbytes;

	while (data_to_receive > 0) {
		/*
		 * Get number of bytes expected from data socket.
		 * Always send in network (big-endian) order.
		 */
		nbytes_read = readn(fileno(crew->in),
		    (char *)&nbytes_exp_netord, sizeof (size_t));
		if (nbytes_read == 0) {
			/*
			 * Timeout received, archive copy may offline.
			 */
			continue;
		} else if (nbytes_read < 0) {
			Trace(TR_RFT, "Samrft read error %d %d",
			    nbytes_read, sizeof (size_t));
			return ((size_t)-1);
		}
		nbytes_expected = ntohl(nbytes_exp_netord);
		if ((long)nbytes_expected > 0) {
			Trace(TR_RFT,
			    "Samrft read socket %d for %d bytes [0x%x]",
			    fileno(crew->in), nbytes_expected, buf);

			nbytes_read = readn(fileno(crew->in), buf,
			    nbytes_expected);
			if (nbytes_read != nbytes_expected) {
				Trace(TR_RFT, "Samrft read error %d %d %d",
				    nbytes_read, nbytes_expected, errno);
				return ((size_t)-1);
			}
		}

		data_to_receive -= nbytes_read;
		buf += nbytes_read;

	}

	return (nbytes);
}

/*
 * Cleanup work crew.
 */
void
CleanupCrew(
	SamrftCrew_t *crew)
{
	fclose(crew->in);
	fclose(crew->out);
	SamFree(crew->buf);
}

/*
 * Initialize work crew.
 */
static void
initCrew(
	SamrftCrew_t *crew)
{
	int status;

	status = pthread_mutex_init(&crew->mutex, NULL);
	ASSERT(status == 0);

	status = pthread_cond_init(&crew->done, NULL);
	ASSERT(status == 0);
}

/*
 * Initialize data connection to rft server on a
 * remote host.  Initiate connection on a data socket.
 */
static int
initDataConnection(
	SamrftImpl_t *rftd,
	int tcpwindowsize)
{
	int af;
	int level;
	int rc = -1;
	int error = EFAULT;
	int sockfd;
	struct sockaddr_in6 mysock;
	struct sockaddr_in *sa;
	struct sockaddr_in6 *sa6;
	unsigned int sa_len;
	char *port, *addr;
	unsigned int size;
	int data;
	int on;
	SamrftCrew_t *crew;
	int value;
	unsigned int length;
	void *taddr;

	sa = (struct sockaddr_in *)&mysock;
	sa6 = (struct sockaddr_in6 *)&mysock;
	sockfd = socket(rftd->caddr.cad6.sin6_family, SOCK_STREAM, 0);
#if 1
	Trace(TR_RFT, "Samrft init data connection %d", sockfd);
#endif
	ASSERT(sockfd >= 0);

	crew = rftd->crew;
	crew->flags = rftd->flags;
	crew->addr.ad.sin_family = rftd->caddr.cad6.sin6_family;
	/*
	 * Let system pick the port.
	 */
	if (rftd->flags & SAMRFT_IPV6) {
		size = sizeof (struct sockaddr_in6);
		memcpy(&crew->addr, &rftd->caddr, size);
		crew->addr.ad6.sin6_port = 0;
	} else {
		size = sizeof (struct sockaddr_in);
		memcpy(&crew->addr, &rftd->caddr, size);
		crew->addr.ad.sin_port = 0;
	}

	Trace(TR_DEBUG, "Samrft bind size %d af %d",
	    size, (int)crew->addr.ad6.sin6_family);
	if (bind(sockfd, (struct sockaddr *)&crew->addr, size) < 0) {

		Trace(TR_RFT, "Samrft bind failed %d", errno);
		ASSERT_NOT_REACHED();
	}

	if (tcpwindowsize > 0) {
		Trace(TR_RFT, "Samrft Setting TCP window size= %d",
		    tcpwindowsize);

		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF,
		    (char *)&tcpwindowsize, sizeof (tcpwindowsize)) < 0) {
			Trace(TR_ERR, "Samrft setsockopt(SO_RCVBUF) failed %d",
			    errno);
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF,
		    (char *)&tcpwindowsize, sizeof (tcpwindowsize)) < 0) {
			Trace(TR_ERR, "Samrft setsockopt(SO_SNDBUF) failed %d",
			    errno);
		}
	}

	on = IPTOS_THROUGHPUT;
	level = (rftd->caddr.cad6.sin6_family == AF_INET6) ?
	    IPPROTO_IPV6 : IPPROTO_IP;
	Trace(TR_RFT, "Samrft set throughput option level %d", level);
	if (setsockopt(sockfd, level, IP_TOS, (char *)&on, sizeof (int)) < 0) {
		Trace(TR_ERR, "setsockopt(IPTOS_THROUGHPUT) failed %d level %d",
		    errno, level);
	}

	if (listen(sockfd, SOMAXCONN) < 0) {
		perror("listen");
		ASSERT_NOT_REACHED();
	}

	/*
	 * The system picked a port for us on the bind.  Use getsockname
	 * to find the port the system gave us so we can send it to the
	 * rft daemon.
	 */
	sa_len = sizeof (mysock);
	memset(sa6, 0, sa_len);

	if (getsockname(sockfd, (struct sockaddr *)&mysock,
	    &sa_len) < 0) {
		Trace(TR_ERR, "getsockname failed on socket %d, errno %d",
		    sockfd, errno);
	}
	af = mysock.sin6_family;
	if (af == AF_INET6) {
		crew->addr.ad6.sin6_port = sa6->sin6_port;
		addr = (char *)&crew->addr.ad6.sin6_addr.s6_addr[0];
		port = (char *)&crew->addr.ad6.sin6_port;
	} else {
		crew->addr.ad.sin_port = sa->sin_port;
		addr = (char *)&crew->addr.ad.sin_addr;
		port = (char *)&crew->addr.ad.sin_port;
	}

	taddr = (void *)addr;
	Trace(TR_DEBUG, "Samrft send data connection 0x%x 0x%x\n",
	    *(int *)taddr, crew->addr.ad.sin_port);

#define	UC(b)   (((int)b)&0xff)

	SendCommand(rftd,
	    "%s %d %d %d %d %d %d %d %d %d %d "
	    "%d %d %d %d %d %d %d %d %d %d",
	    SAMRFT_CMD_DPORT6, 0, af,
	    UC(addr[0]), UC(addr[1]), UC(addr[2]), UC(addr[3]),
	    UC(addr[4]), UC(addr[5]), UC(addr[6]), UC(addr[7]),
	    UC(addr[8]), UC(addr[9]), UC(addr[10]), UC(addr[11]),
	    UC(addr[12]), UC(addr[13]), UC(addr[14]), UC(addr[15]),
	    UC(port[0]), UC(port[1]));

	if (GetReply(rftd) >= 0) {
		rc = GetDPortReply(rftd, &error);
	}
	if (rc < 0) {
		(void) close(sockfd);
		SetErrno = error;
		return (rc);
	}

	size = sizeof (struct sockaddr_in6);
	data = accept(sockfd, (struct sockaddr *)&crew->addr, &size);
	if (data == -1) {
		perror("accept");
		ASSERT_NOT_REACHED();
	}

	length = sizeof (value);
	if (getsockopt(data, SOL_SOCKET, SO_RCVBUF, &value, &length) < 0) {
		Trace(TR_ERR, "getsockopt(SO_RCVBUF) failed %d", errno);
	} else {
		Trace(TR_RFT, "Samrft Get TCP window (RCVBUF) size= %d", value);
	}

	if (getsockopt(data, SOL_SOCKET, SO_SNDBUF, &value, &length) < 0) {
		Trace(TR_ERR, "getsockopt(SO_SNDBUF) failed %d", errno);
	} else {
		Trace(TR_RFT, "Samrft Get TCP window (SNDBUF) size= %d", value);
	}

	crew->in  = fdopen(data, "r");
	crew->out = fdopen(data, "w");
	(void) close(sockfd);

	return (rc);
}
