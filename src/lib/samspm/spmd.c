/*
 * spmd.c - Source for libsamspmd
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

#pragma ident "$Revision: 1.14 $"


/*
 * System Headers
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sam/names.h>
#include <sam/spm.h>

#if defined(lint)
#include "sam/lint.h"
#undef sprintf
#endif /* defined(lint) */


/*
 * Definitions
 */
#define	SPM_REQUEST_MAX		SPM_SERVICE_NAME_MAX + 16
#define	SPM_PATH_MAX		108

typedef struct service_table_entry {
	int fd;
	int poll_index;
	char path[SPM_PATH_MAX];
	char service[SPM_SERVICE_NAME_MAX];
	struct service_table_entry *next;
	struct service_table_entry *previous;
} ste_t;

static enum {UDS, IPV6, IPV4, SERVICES};
/*
 * External Globals
 */


/*
 * Public Globals
 */


/*
 * Private Globals
 */
static pthread_mutex_t spm_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t spm_cv = PTHREAD_COND_INITIALIZER;
static pthread_t spm_tid;
static enum {inactive, initializing, active, terminating} spm_state = inactive;
static int spm_errno = ESPM_OK;
static char spm_errstr[SPM_ERRSTR_MAX];
static ste_t *spm_st_head;
static ste_t *spm_st_tail;
static struct pollfd *spm_poll;
static int spm_poll_maxindex;
static int spm_signo;


static ste_t *free_ste(ste_t *entry);
static void release_all_resources();




/*
 * Private Functions
 */

static void
atfork_child()
{
	/*
	 * Since we lost the SPM thread on the fork but still have resources,
	 * we are sort of in the process of terminating.  Set the state
	 * accordingly.
	 */
	spm_state = terminating;

	release_all_resources();
	(void) pthread_mutex_unlock(&spm_mutex);
}

static void
atfork_parent()
{
	(void) pthread_mutex_unlock(&spm_mutex);
}

static void
atfork_prepare()
{
	(void) pthread_mutex_lock(&spm_mutex);
}

/*ARGSUSED0*/
static void
cleanup_on_pthread_termination(void *arg)
{
	(void) pthread_mutex_lock(&spm_mutex);
	release_all_resources();
	(void) pthread_mutex_unlock(&spm_mutex);
}

static void
do_connect(int fd, char *service)
{
	char c;
	struct iovec iov[1];
	struct msghdr msg;
	char response[SPM_ERRSTR_MAX];
	int response_len;
	int retval;
	ste_t *ste;

	/*
	 * Scan through services table for specifed service.
	 */
	for (ste = spm_st_head; ste; ) {
		if (strncmp(ste->service,
		    service, sizeof (ste->service)) == 0) {
			/*
			 * Found the service, push the descriptor to
			 * the service provider.
			 */
			iov[0].iov_base = (void *)&c;
			iov[0].iov_len = 1;

			msg.msg_accrights = (caddr_t)&fd;
			msg.msg_accrightslen = sizeof (int);
			msg.msg_name = NULL;
			msg.msg_namelen = 0;
			msg.msg_iov = iov;
			msg.msg_iovlen = 1;

			/* send the message */
			while ((retval = sendmsg(ste->fd, &msg, 0)) == -1) {
				if (errno != EINTR) {
					break;
				}
			}
			if (retval == 1) {
				break;
			}

			/*
			 * If we got here, it's because we are unable to
			 * communicate with the service provider.  The safest
			 * thing to do is to assume the service provider died.
			 * Remove the entry from the services table (free_ste
			 * will also release all resources associated with the
			 * entry).  Continue the table scan in case another
			 * entity is providing the same service.
			 */
			ste = free_ste(ste);
		} else {
			ste = ste->next;
		}
	}

	if (ste == NULL) {
		/*
		 * We were unable to find the service or communicate with the
		 * service provider.  Respond to the caller.
		 */
		response_len = sprintf(response,
		    "7 Service is unavailable\r\n");
		(void) spm_writen(fd, response, response_len);
	}

	(void) close(fd);
}

static void
do_query(int fd)
{
	char err_response[SPM_ERRSTR_MAX];
	char *p;
	char *response;
	int response_len = 0;
	ste_t *ste;

	/*
	 * First we need to determine how big a response buffer is needed.
	 * Loop through the services table and add up the string lengths of
	 * each service.
	 */
	for (ste = spm_st_head; ste; ste = ste->next) {
		response_len += strlen(ste->service) + 1;
	}

	/*
	 * Allocate a response buffer.  Pad it to allow for the error
	 * number and NVT trailer.
	 */
	if ((response = (char *)malloc(response_len + 10)) == NULL) {
		response_len = sprintf(err_response, "6 no memory\r\n");
		(void) spm_writen(fd, err_response, response_len);
		(void) close(fd);
		return;
	}

	/*
	 * Now that we have a buffer, once again loop through the services
	 * table and fill in the response.
	 */
	p = response;
	p += sprintf(p, "0 ");
	for (ste = spm_st_head; ste; ste = ste->next) {
		p += sprintf(p, "%s\n", ste->service);
	}
	(void) sprintf(p, "\r\n");

	/* Respond to the caller. */
	(void) spm_writen(fd, response, strlen(response));
	free(response);
	(void) close(fd);
}

static void
do_register(int fd, char *service, char *path)
{
	int i;
	char response[SPM_ERRSTR_MAX];
	int response_len;
	ste_t *ste;

	if (strlen(service) >= SPM_SERVICE_NAME_MAX ||
	    strlen(path) >= SPM_PATH_MAX) {
		response_len = sprintf(response, "5 argument is too big\r\n");
		(void) spm_writen(fd, response, response_len);
		(void) close(fd);
		return;
	}

	/* Allocate a service table entry */
	if ((ste = (ste_t *)malloc(sizeof (ste_t))) == NULL) {
		response_len = sprintf(response, "6 no memory\r\n");
		(void) spm_writen(fd, response, response_len);
		(void) close(fd);
		return;
	}

	/* Initialize it */
	memset((void *)ste, 0, sizeof (ste_t));
	ste->fd = fd;
	strncpy(ste->path, path, sizeof (ste->path));
	strncpy(ste->service, service, sizeof (ste->service));

	/* Append it to the table */
	if (spm_st_tail) {
		ste->previous = spm_st_tail;
		spm_st_tail->next = ste;
		spm_st_tail = ste;
	} else {
		spm_st_head = ste;
		spm_st_tail = ste;
	}

	/* Update the poll() data structure */
	for (i = SERVICES; i < OPEN_MAX; i++) {
		if (spm_poll[i].fd == -1) {
			ste->poll_index = i;
			spm_poll[i].fd = fd;
			if (i == spm_poll_maxindex) {
				spm_poll_maxindex++;
			}
			break;
		}
	}

	/* Respond to caller */
	response_len = sprintf(response, "0 ok\r\n");
	(void) spm_writen(fd, response, response_len);
}


static ste_t *
free_ste(ste_t *entry)
{
	ste_t *next;
	ste_t *previous;

	(void) close(entry->fd);
	unlink(entry->path);

	spm_poll[entry->poll_index].fd = -1;

	if ((entry->poll_index + 1) == spm_poll_maxindex) {
		spm_poll_maxindex--;
	}

	next = entry->next;
	previous = entry->previous;

	if (next) {
		next->previous = previous;
	} else {
		spm_st_tail = previous;
	}

	if (previous) {
		previous->next = next;
	} else {
		spm_st_head = next;
	}

	free(entry);

	return (next);
}

static int
initialize_internet_port(int *error, char *errstr)
{
	struct addrinfo hints;
	struct pollfd *pfd;
	int reuseaddr_flag = 1;
	int retval;
	struct addrinfo *srvr;
	struct addrinfo *srvr_save;

	memset((void *)&hints, 0, sizeof (hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((retval = getaddrinfo(NULL, SPM_PORT_STRING, &hints, &srvr)) != 0) {
		if (errstr) {
			snprintf(errstr, SPM_ERRSTR_MAX, "error: %s",
			    gai_strerror(retval));
		}
		*error = ESPM_AI;
		return (-1);
	}

	srvr_save = srvr;

	do {
		if (srvr->ai_family == PF_INET6) {
			pfd = &spm_poll[IPV6];
		} else if (srvr->ai_family == PF_INET) {
			pfd = &spm_poll[IPV4];
		} else {
			if (errstr) {
				snprintf(errstr, SPM_ERRSTR_MAX,
				    "Unsupported address family: %d",
				    srvr->ai_family);
			}
			*error = ESPM_AI;
			freeaddrinfo(srvr_save);
			return (-1);
		}
		if (pfd->fd != -1) {
			if (errstr) {
				snprintf(errstr, SPM_ERRSTR_MAX,
				    "Duplicate address family: %d",
				    srvr->ai_family);
			}
			*error = ESPM_AI;
			freeaddrinfo(srvr_save);
			return (-1);
		}

		if ((pfd->fd = socket(srvr->ai_family,
		    srvr->ai_socktype, srvr->ai_protocol)) < 0) {
			if (errstr) {
				snprintf(errstr, SPM_ERRSTR_MAX,
				    "socket[%d]: %s",
				    srvr->ai_family, strerror(errno));
			}
			*error = ESPM_SOCKET;
			freeaddrinfo(srvr_save);
			return (-1);
		}

		/* set socket option */
		if (setsockopt(pfd->fd, SOL_SOCKET, SO_REUSEADDR,
		    (void *)&reuseaddr_flag, sizeof (reuseaddr_flag)) < 0) {
			if (errstr) {
				snprintf(errstr, SPM_ERRSTR_MAX,
				    "setsockopt[%d]: %s",
				    srvr->ai_family, strerror(errno));
			}
			*error = ESPM_SOCKET;
			freeaddrinfo(srvr_save);
			return (-1);
		}
		/* Bind local */
		if (bind(pfd->fd, srvr->ai_addr, srvr->ai_addrlen) < 0) {
			if (errstr) {
				snprintf(errstr, SPM_ERRSTR_MAX,
				    "bind[%d]: %s",
				    srvr->ai_family, strerror(errno));
			}
			*error = ESPM_BIND;
			freeaddrinfo(srvr_save);
			return (-1);
		}

		listen(pfd->fd, 5);

	} while ((srvr = srvr->ai_next) != NULL);

	freeaddrinfo(srvr_save);

	return (0);
}

static int
initialize_unix_domain_port(int *error, char *errstr)
{
	int len;
	struct sockaddr_un uaddr;

	if ((spm_poll[UDS].fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
		if (errstr) {
			snprintf(errstr, SPM_ERRSTR_MAX, "socket[%d]: %s",
			    AF_UNIX, strerror(errno));
		}
		*error = ESPM_SOCKET;
		return (-1);
	}
	memset((void *)&uaddr, 0, sizeof (uaddr));
	uaddr.sun_family = PF_UNIX;
	strncpy(uaddr.sun_path, SPM_UDS_DAEMON_PATH, sizeof (uaddr.sun_path));
	len = strlen(uaddr.sun_path) + sizeof (uaddr.sun_family);
	unlink(uaddr.sun_path);
	if (bind(spm_poll[UDS].fd, (struct sockaddr *)&uaddr, len) < 0) {
		if (errstr) {
			snprintf(errstr, SPM_ERRSTR_MAX, "bind[%d]: %s",
			    AF_UNIX, strerror(errno));
		}
		*error = ESPM_BIND;
		return (-1);
	}

	listen(spm_poll[UDS].fd, 5);

	return (0);
}

static void
process_inet_request(int server_fd)
{
	int client_fd;
	char line_buffer[SPM_REQUEST_MAX];
	char *request_arg;

	if ((client_fd = accept(server_fd, NULL, NULL)) < 0) {
		return;
	}

	memset((void *)line_buffer, 0, SPM_REQUEST_MAX);
	if (spm_readnvtline(client_fd, line_buffer, SPM_REQUEST_MAX) <= 0) {
		(void) close(client_fd);
		return;
	}

	/* Split the line */
	if (request_arg = strchr(line_buffer, ' ')) {
		*request_arg = '\0';
		request_arg++;
	}
	if (strcmp(line_buffer, "query") == 0) {
		do_query(client_fd);
	} else if (strcmp(line_buffer, "connect") == 0 && request_arg != NULL) {
		do_connect(client_fd, request_arg);
	} else {
		char *response = "10 Unknown request\r\n";
		(void) spm_writen(client_fd, response, strlen(response));
		(void) close(client_fd);
	}
}

static void
process_unix_request(int uds_fd)
{
	int client_fd;
	char line_buffer[SPM_REQUEST_MAX];
	char *request_arg;
	char response[SPM_ERRSTR_MAX];
	int response_len;
	struct sockaddr_un uaddr;
	socklen_t uaddr_len;

	uaddr_len = sizeof (uaddr);
	if ((client_fd = accept(uds_fd,
	    (struct sockaddr *)&uaddr, &uaddr_len)) < 0) {
		return;
	}

	(void) spm_readnvtline(client_fd, line_buffer, SPM_REQUEST_MAX);

	/* Split the line */
	if (request_arg = strchr(line_buffer, ' ')) {
		*request_arg = '\0';
		request_arg++;
	}
	if (strcmp(line_buffer, "register") == 0 && request_arg != NULL) {
		do_register(client_fd, request_arg, uaddr.sun_path);
	} else {
		response_len = sprintf(response, "10 Unknown request\r\n");
		(void) spm_writen(client_fd, response, response_len);
		(void) close(client_fd);
	}
}

static void
release_all_resources()
{
	int signo = 0;
	ste_t *ste;

	/* Determine if we need to throw a signal */
	if (spm_signo && (spm_state == active)) {
		signo = spm_signo;
	}

	if (spm_poll) {
		/* close uds fd */
		if (spm_poll[UDS].fd != -1) {
			(void) close(spm_poll[UDS].fd);
		}

		/* close inet IPV6 fd */
		if (spm_poll[IPV6].fd != -1) {
			(void) close(spm_poll[IPV6].fd);
		}

		/* close inet IPV4 fd */
		if (spm_poll[IPV4].fd != -1) {
			(void) close(spm_poll[IPV4].fd);
		}

		/* close uds fd */
		if (spm_poll[UDS].fd != -1) {
			(void) close(spm_poll[UDS].fd);
		}

		/*
		 * loop through services table, closing fds
		 * and freeing entries
		 */
		for (ste = spm_st_head; ste; ) {
			ste = free_ste(ste);
		}

		/* Free the poll() data structure */
		free((void *)spm_poll);
	}

	/* Clear the signal */
	spm_signo = 0;

	/* Clear initialized flag */
	spm_state = inactive;

	/* Throw a signal if indicated */
	if (signo) {
		kill(getpid(), signo);
	}
}

static void *
spm(void *arg)
{
	int count;
	int i;
	ste_t *ste;

	(void) pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	(void) pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	(void) pthread_mutex_lock(&spm_mutex);

	/* Prepare for the inevitable... */
	pthread_cleanup_push(cleanup_on_pthread_termination, (void *)0);

	/* Set the termination signal */
	spm_signo = (int)arg;

	/* Set up the data structure for poll */
	spm_poll = (struct pollfd *)malloc(OPEN_MAX * sizeof (struct pollfd));
	for (i = 0; i < OPEN_MAX; i++) {
		spm_poll[i].fd = -1;
		spm_poll[i].events = POLLRDNORM;
	}
	spm_poll_maxindex = SERVICES;

	/* Set up the internet port. */
	if (initialize_internet_port(&spm_errno, spm_errstr) == -1) {
		(void) pthread_cond_signal(&spm_cv);
		(void) pthread_mutex_unlock(&spm_mutex);
		pthread_exit((void *)0);
	}

	/*
	 * Set up the UDS pipe
	 */
	if (initialize_unix_domain_port(&spm_errno, spm_errstr) == -1) {
		(void) pthread_cond_signal(&spm_cv);
		(void) pthread_mutex_unlock(&spm_mutex);
		pthread_exit((void *)0);
	}

	/*
	 * Initialization is complete.
	 */
	spm_state = active;
	(void) pthread_cond_signal(&spm_cv);

	for (;;) {
		(void) pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

		if (spm_state == terminating) {
			(void) pthread_mutex_unlock(&spm_mutex);
			pthread_exit((void *)0);
		}

		(void) pthread_mutex_unlock(&spm_mutex);

		count = poll(spm_poll, spm_poll_maxindex, -1);
		if (count == -1) {
			if (errno == EINTR) {
				continue;
			}
			snprintf(spm_errstr, SPM_ERRSTR_MAX, "poll(): %s",
			    strerror(errno));
			spm_errno = ESPM_POLL;
			pthread_exit((void *)0);
		}

		(void) pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		(void) pthread_mutex_lock(&spm_mutex);

		for (i = 0; count && (i < spm_poll_maxindex); i++) {
			if (spm_poll[i].revents) {
				switch (i) {
				case UDS:
					process_unix_request(spm_poll[i].fd);
					break;
				case IPV6:
				case IPV4:
					process_inet_request(spm_poll[i].fd);
					break;
				default:  /* services */
					ste = spm_st_head;
					while (ste) {
						if (ste->fd == spm_poll[i].fd) {
							ste = free_ste(ste);
						} else {
							ste = ste->next;
						}
					}
					break;
				}
				spm_poll[i].revents = 0;
				count--;
			}
		}

	}

	/*NOTREACHED*/
#pragma error_messages(off, E_STATEMENT_NOT_REACHED)
	pthread_cleanup_pop(0);
	return ((void *)0);
#pragma error_messages(default, E_STATEMENT_NOT_REACHED)
}


/*
 * Public Functions
 */
int
spm_error(int *error, char *errstr)
{
	if (spm_errno) {
		if (errstr) {
			strncpy(errstr, spm_errstr, SPM_ERRSTR_MAX);
		}
		*error = spm_errno;

		/* Clear the global error */
		spm_errno = ESPM_OK;
		spm_errstr[0] = '\0';
	} else {
		spm_errno = ESPM_OK;
		spm_errstr[0] = '\0';
	}
	return (0);
}



int
spm_initialize(int signo, int *error, char *errstr)
{
	struct sigaction act;
	struct sigaction oact;

	(void) pthread_mutex_lock(&spm_mutex);

	if (spm_state != inactive) {
		(void) pthread_mutex_unlock(&spm_mutex);
		if (errstr) {
			snprintf(errstr, SPM_ERRSTR_MAX,
			    "SPM is already active.");
		}
		*error = ESPM_AGAIN;
		return (-1);
	}

	/* Set the state */
	spm_state = initializing;

	/*
	 * Check how this process handles SIGPIPE.  If the handler is any other
	 * than SIG_DFL leave it alone.  Otherwise set it to SIG_IGN.  This is
	 * done to prevent I/O to half-closed sockets from terminating the
	 * process. If the process already has a handler, it is assumed that
	 * the process will "do the right thing".
	 */
	if (sigaction(SIGPIPE, NULL, &oact)) {
		if (errstr) {
			snprintf(errstr, SPM_ERRSTR_MAX, "sigaction: %s",
			    strerror(errno));
		}
		*error = ESPM_SIGNAL;
		(void) pthread_mutex_unlock(&spm_mutex);
		return (-1);
	}
	if (oact.sa_handler == SIG_DFL) {
		act.sa_handler = SIG_IGN;
		sigemptyset(&act.sa_mask);
		act.sa_flags = 0;
		if (sigaction(SIGPIPE, &act, NULL)) {
			if (errstr) {
				snprintf(errstr, SPM_ERRSTR_MAX,
				    "sigaction: %s", strerror(errno));
			}
			*error = ESPM_SIGNAL;
			(void) pthread_mutex_unlock(&spm_mutex);
			return (-1);
		}
	}

	/* create spm thread */
	if (pthread_create(&spm_tid, NULL, spm, (void *)signo) != 0) {
		if (errstr) {
			snprintf(errstr, SPM_ERRSTR_MAX, "pthread: %s",
			    strerror(errno));
		}
		*error = ESPM_PTHREAD;
		(void) pthread_mutex_unlock(&spm_mutex);
		return (-1);
	}

	/* Prepare for the possibility the host process may fork */
	if (pthread_atfork(atfork_prepare, atfork_parent, atfork_child) != 0) {
		if (errstr) {
			snprintf(errstr, SPM_ERRSTR_MAX, "pthread_atfork: %s",
			    strerror(errno));
		}
		*error = ESPM_PTHREAD;
		(void) pthread_mutex_unlock(&spm_mutex);
		return (-1);
	}

	/* wait for initialization to complete */
	while (spm_state == initializing && spm_errno == 0) {
		(void) pthread_cond_wait(&spm_cv, &spm_mutex);
	}

	/* return initialization status */
	(void) pthread_mutex_unlock(&spm_mutex);
	if (spm_errno) {
		if (errstr) {
			strncpy(errstr, spm_errstr, SPM_ERRSTR_MAX);
		}
		*error = spm_errno;

		/* Clear the global error */
		spm_errno = ESPM_OK;
		spm_errstr[0] = '\0';
		return (-1);
	} else {
		return (0);
	}
}

int
spm_shutdown(int *error, char *errstr)
{
	(void) pthread_mutex_lock(&spm_mutex);

	if (spm_state != active) {
		(void) pthread_mutex_unlock(&spm_mutex);
		if (errstr) {
			snprintf(errstr, SPM_ERRSTR_MAX, "SPM is not active.");
		}
		*error = ESPM_AGAIN;
		return (-1);
	}

	/* Indicate a pending shutdown */
	spm_state = terminating;

	(void) pthread_mutex_unlock(&spm_mutex);

	/* signal spm thread */
	if (pthread_cancel(spm_tid) != 0) {
		if (errstr) {
			snprintf(errstr, SPM_ERRSTR_MAX, "cancel: %s",
			    strerror(errno));
		}
		*error = ESPM_PTHREAD;
		return (-1);
	}

	/* wait for death */
	if (pthread_join(spm_tid, NULL) != 0) {
		if (errstr) {
			snprintf(errstr, SPM_ERRSTR_MAX, "join: %s",
			    strerror(errno));
		}
		*error = ESPM_PTHREAD;
		return (-1);
	}

	/* return status */
	*error = ESPM_OK;
	return (0);
}
