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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <sam/names.h>
#include <sam/spm.h>

#if defined(lint)
#include "sam/lint.h"
#undef sprintf
#undef snprintf
#endif /* defined(lint) */


/*
 * Maximum size of a protocol request
 */
#define	SPM_REQUEST_MAX (SPM_SERVICE_NAME_MAX + 16)


#define	SPM_RESPONSE_MAX (SPM_ERRSTR_MAX + 10)

static int tcp_connect(char *server, char *local, int *error, char *errstr);

int
spm_accept(int service_fd, int *error, char *errstr)
{
	char c;
	struct iovec iov[1];
	struct msghdr msg;
	int passed_fd;
	char response[SPM_ERRSTR_MAX];
	int response_len;
	int retval;

#ifdef linux
	struct cmsghdr *cmsg = NULL;
	char *cmdata = NULL;
	/*
	 * Allocates enough space for a cmsghdr
	 * plus ancillary data, in this case an int.
	 */
	char cmbuf[CMSG_SPACE(sizeof (passed_fd))];

#endif /* linux */

	iov[0].iov_base = (void *)&c;
	iov[0].iov_len = 1;

#ifdef linux

	msg.msg_control = cmbuf;
	msg.msg_controllen = sizeof (cmbuf);

#else
	msg.msg_accrights = (caddr_t)&passed_fd;
	msg.msg_accrightslen = sizeof (int);

#endif /* linux */

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	/* receive the message */
	if ((retval = recvmsg(service_fd, &msg, 0)) == -1) {
		if (errno == EINTR) {
			if (errstr) {
				(void) snprintf(errstr, SPM_ERRSTR_MAX, "%s",
				    strerror(errno));
			}
			*error = ESPM_INTR;
		} else {
			if (errstr) {
				(void) snprintf(errstr, SPM_ERRSTR_MAX,
				    "recvmsg: %s", strerror(errno));
			}
			*error = ESPM_RECVMSG;
		}
		return (-1);
	}

#ifdef linux

	/*
	 * Make sure the received controll message length
	 * is enough for the expected data
	 */
	if (msg.msg_controllen < sizeof (struct cmsghdr) + sizeof (int)) {
		if (errstr) {
			(void) snprintf(errstr, SPM_ERRSTR_MAX,
			    "Descriptor was not passed.");
		}
		*error = ESPM_BADDESC;
		return (-1);

	}

	/*
	 * Get a pointer to the first controll message header
	 * and the data that follows.
	 */
	cmsg	= CMSG_FIRSTHDR(&msg);
	cmdata	= CMSG_DATA(cmsg);

	/*
	 * Check the controll message type and level
	 */
	if ((cmsg->cmsg_type != SOL_SOCKET) ||
	    (cmsg->cmsg_level != SCM_RIGHTS)) {

		if (errstr) {
			(void) snprintf(errstr, SPM_ERRSTR_MAX,
			    "Descriptor was not passed.");
		}
		*error = ESPM_BADDESC;
		return (-1);
	}

	passed_fd = 0;
	/*
	 * Copy the file descripter
	 */
	memcpy((char *)&passed_fd, cmdata, sizeof (int));

#else

	if (msg.msg_accrightslen != sizeof (int) || retval != 1) {
		if (errstr) {
			(void) snprintf(errstr, SPM_ERRSTR_MAX,
			    "Descriptor was not passed.");
		}
		*error = ESPM_BADDESC;
		return (-1);
	}

#endif /* linux */

	/* respond to the "connector" */
	response_len = sprintf(response, "0 Back at you!.\r\n");
	(void) spm_writen(passed_fd, response, response_len);

	return (passed_fd);
}

int
spm_connect_to_service(char *service, char *host, char *interface, int *error,
	char *errstr)
{
	char request_buffer[SPM_REQUEST_MAX];
	int request_size;
	char response_buffer[SPM_RESPONSE_MAX];
	char *response_text;
	int sfd;

	/* Validate service name length */
	if (strlen(service) >= SPM_SERVICE_NAME_MAX) {
		*error = ESPM_SIZE;
		strcpy(errstr, "service name length too long.");
		return (-1);
	}

	/* Connect to the host */
	if ((sfd = tcp_connect(host, interface, error, errstr)) == -1) {
		return (-1);
	}

	/* Send query */
	request_size = snprintf(request_buffer, SPM_REQUEST_MAX,
	    "connect %s\r\n", service);
	(void) spm_writen(sfd, request_buffer, request_size);

	(void) spm_readnvtline(sfd, response_buffer, SPM_RESPONSE_MAX);
	*error = (int)strtol(response_buffer, &response_text, 10);
	if (*error) {
		if (errstr) {
			strncpy(errstr, response_text + 1, SPM_ERRSTR_MAX);
		}
		(void) close(sfd);
		return (-1);
	} else {
		return (sfd);
	}
}

void
spm_free_query_info(struct spm_query_info *info)
{
	struct spm_query_info *entry;
	struct spm_query_info *next_entry;

	entry = info;
	while (entry) {
		next_entry = entry->sqi_next;
		free(entry);
		entry = next_entry;
	}
}

int
spm_query_services(char *host, struct spm_query_info **result,
	int *error, char *errstr)
{
	int fd;
	char *n;
	char *request = "query\r\n";
	char *response;
	int response_len;
	char *response_text;
	char *s;
	struct spm_query_info *sqi;

	/*
	 * Since we do not know how many services are registered or their
	 * string lengths, it is best to assume the worst so allocate a
	 * "really big" buffer.
	 */
	response_len = 8196;
	if ((response = (char *)malloc(response_len)) == NULL) {
		*error = ESPM_NOMEM;
		if (errstr) {
			strcpy(errstr, "No memory");
		}
		return (-1);
	}

	/* Connect to the host */
	if ((fd = tcp_connect(host, NULL, error, errstr)) == -1) {
		free(response);
		return (-1);
	}

	/* Send query */
	(void) spm_writen(fd, request, strlen(request));

	/* Receive response */
	(void) spm_readnvtline(fd, (void *)response, response_len);

	/* Sever the connection to the host */
	(void) close(fd);

	/* Crack the output */
	*error = (int)strtol(response, &response_text, 10);
	if (*error) {
		if (errstr) {
			strncpy(errstr, response_text + 1, SPM_ERRSTR_MAX);
		}
		free(response);
		return (-1);
	} else {
		*result = (struct spm_query_info *)NULL;
		s = response_text + 1;
		while (n = strchr(s, '\n')) {
			*n = '\0';
			if ((sqi = (struct spm_query_info *)
			    malloc(sizeof (struct spm_query_info))) == NULL) {
				*error = ESPM_NOMEM;
				if (errstr) {
					strcpy(errstr, "No memory");
				}
				spm_free_query_info(*result);
				return (-1);
			}
			strcpy(sqi->sqi_service, s);
			sqi->sqi_next = *result;
			*result = sqi;
			s = n + 1;
		}
		return (0);
	}

}

int
spm_register_service(char *service, int *error, char *errstr)
{
	int fd;
	int len;
	char request_buffer[SPM_REQUEST_MAX];
	int request_size;
	char response_buffer[SPM_RESPONSE_MAX];
	char *response_text;
	struct sockaddr_un uaddr;

	/* Validate service name length */
	if (strlen(service) >= SPM_SERVICE_NAME_MAX) {
		*error = ESPM_SIZE;
		strcpy(errstr, "service name length too long.");
		return (-1);
	}

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		if (errstr) {
			(void) snprintf(errstr, SPM_ERRSTR_MAX,
			    "socket: %s", strerror(errno));
		}
		*error = ESPM_SOCKET;
		return (-1);
	}
	memset((void *)&uaddr, 0, sizeof (uaddr));
	uaddr.sun_family = AF_UNIX;
	(void) sprintf(uaddr.sun_path, SPM_UDS_CLIENT_PATH, (int)getpid());
	len = strlen(uaddr.sun_path) + sizeof (uaddr.sun_family);
	unlink(uaddr.sun_path);
	if (bind(fd, (struct sockaddr *)&uaddr, len) < 0) {
		if (errstr) {
			(void) snprintf(errstr, SPM_ERRSTR_MAX,
			    "bind: %s", strerror(errno));
		}
		*error = ESPM_BIND;
		return (-1);
	}

	memset((void *)&uaddr, 0, sizeof (uaddr));
	uaddr.sun_family = AF_UNIX;
	strcpy(uaddr.sun_path, SPM_UDS_DAEMON_PATH);
	len = strlen(uaddr.sun_path) + sizeof (uaddr.sun_family);

	if (connect(fd, (struct sockaddr *)&uaddr, sizeof (uaddr)) < 0) {
		if (errstr) {
			(void) snprintf(errstr, SPM_ERRSTR_MAX,
			    "connect: %s", strerror(errno));
		}
		*error = ESPM_CONNECT;
		return (-1);
	}

	request_size = snprintf(request_buffer, SPM_REQUEST_MAX,
	    "register %s\r\n", service);
	(void) spm_writen(fd, request_buffer, request_size);

	/* Receive response */
	(void) spm_readnvtline(fd, response_buffer, SPM_RESPONSE_MAX);
	*error = (int)strtol(response_buffer, &response_text, 10);
	if (*error) {
		if (errstr) {
			strncpy(errstr, response_text + 1, SPM_ERRSTR_MAX);
		}
		return (-1);
	} else {
		return (fd);
	}
}

int
spm_unregister_service(int service_fd)
{
	(void) close(service_fd);
	return (0);
}

ssize_t
spm_writen(int fd, const void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ((nwritten = write(fd, ptr, nleft)) <= 0) {
			if (errno == EINTR) {
				nwritten = 0;
			} else {
				return (-1);
			}
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return (n);
}

ssize_t
spm_readnvtline(int fd, void *vptr, size_t maxlen)
{
	size_t nleft;
	ssize_t nread;
	size_t totalread;
	char *ptr;

	ptr = vptr;
	nleft = maxlen;
	totalread = 0;

	while (nleft > 0) {
		if ((nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR) {
				continue;
			} else {
				return (-1);
			}
		} else if (nread == 0) {
			return (-1);
		} else {
			if ((ptr[nread - 2] == '\r') &&
			    (ptr[nread - 1] == '\n')) {
				ptr[nread - 2] = '\0';
				return (totalread += nread - 2);
			} else {
				totalread += nread;
				ptr += nread;
				nleft -= nread;
			}
		}
	}
	return (totalread);
}


static int
tcp_connect(char *host, char *interface, int *error, char *errstr)
{
	struct addrinfo hints;
	int retval;
	int sockfd;
	struct addrinfo *srvr;
	struct addrinfo *srvr_save;

	memset((void *)&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((retval = getaddrinfo(host, SPM_PORT_STRING, &hints, &srvr)) != 0) {
		if (errstr) {
			(void) snprintf(errstr, SPM_ERRSTR_MAX,
			    "tcp_connect error for %s: %s",
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

		/*
		 * Bind local interface if specified.
		 * The caller wants to control
		 * which local interface the connection will be made from.
		 */
		if (interface) {
			struct addrinfo *lcl;
			char lcl_name[64];
			struct addrinfo *lcl_save;

			(void) snprintf(lcl_name, 64, "%d", INADDR_ANY);

			memset((void *)&hints, 0, sizeof (hints));
			hints.ai_family = AF_UNSPEC;
			hints.ai_socktype = SOCK_STREAM;
			if ((retval = getaddrinfo(interface, lcl_name,
			    &hints, &lcl)) != 0) {
				if (errstr) {
					(void) snprintf(errstr, SPM_ERRSTR_MAX,
					    "tcp_connect error for %s: %s",
					    interface, gai_strerror(retval));
				}
				freeaddrinfo(srvr_save);
				*error = ESPM_AI;
				return (-1);
			}

			lcl_save = lcl;

			do {
				if (bind(sockfd, lcl->ai_addr,
				    lcl->ai_addrlen) == 0) {
					break;
				}
			} while ((lcl = lcl->ai_next) != NULL);

			freeaddrinfo(lcl_save);

			if (lcl == NULL) {
				if (errstr) {
					(void) snprintf(errstr, SPM_ERRSTR_MAX,
					    "tcp_connect error for %s: %s",
					    interface, strerror(errno));
				}
				freeaddrinfo(srvr_save);
				*error = ESPM_BIND;
				return (-1);
			}
		}

		/* Connect */
		if (connect(sockfd, srvr->ai_addr, srvr->ai_addrlen) == 0) {
			break;
		}

		(void) close(sockfd);
	} while ((srvr = srvr->ai_next) != NULL);

	freeaddrinfo(srvr_save);

	if (srvr == NULL) {
		if (errstr) {
			(void) snprintf(errstr, SPM_ERRSTR_MAX,
			    "tcp_connect error for %s: %s",
			    host, strerror(errno));
		}
		*error = ESPM_CONNECT;
		return (-1);
	} else {
		*error = ESPM_OK;
		return (sockfd);
	}
}
