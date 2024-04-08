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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sam/spm.h>

#define	DATA_LEN 256

int service_fd = -1;
char *service;

#if !defined(lint)
void
sig_int(
	/* LINTED argument unused in function */
	int signo)
{
	if (service_fd == -1) {
		printf("Service not yet registered!\n");
	} else {
		printf("\nUnregistering service '%s'.\n", service);
		spm_unregister_service(service_fd);
	}
	exit(0);
}


int
main(int argc, char **argv)
{
	struct sigaction act;
	int client_fd;
	char host[128];
	int error;
	char error_buffer[SPM_ERRSTR_MAX];
	unsigned int sa_len;
	struct sockaddr *sa;

	if (argc != 2) {
		fprintf(stderr, "USAGE: spmTestServerS service\n");
		exit(1);
	}

	service = argv[1];

	act.sa_handler = sig_int;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (sigaction(SIGINT, &act, NULL)) {
		fprintf(stderr, "sigaction: %s", strerror(errno));
	}

	sa = (struct sockaddr *)malloc(DATA_LEN);

	if ((service_fd = spm_register_service(service,
	    &error, error_buffer)) < 0) {
		fprintf(stderr, "register failed: %d: %s\n",
		    error, error_buffer);
		exit(1);
	}
	for (;;) {
		if ((client_fd = spm_accept(service_fd,
		    &error, error_buffer)) < 0) {
			fprintf(stderr, "accept failed: %d: %s\n",
			    error, error_buffer);
			break;
		}

		sa_len = DATA_LEN;
		getpeername(client_fd, sa, &sa_len);

		if (sa->sa_family == AF_INET) {
			struct sockaddr_in *sin = (void *)sa;

			if (inet_ntop(AF_INET, &sin->sin_addr,
			    host, sizeof (host)) == NULL) {
				fprintf(stderr, "inet_ntop: %d: %s\n",
				    errno, strerror(errno));
			} else {
				printf("connected to %s\n", host);
			}
		} else if (sa->sa_family == AF_INET6) {
			struct sockaddr_in6 *sin6 = (void *)sa;

			if (inet_ntop(AF_INET6, &sin6->sin6_addr,
			    host, sizeof (host)) == NULL) {
				fprintf(stderr, "inet_ntop: %d: %s\n",
				    errno, strerror(errno));
			} else {
				printf("connected to %s\n", host);
			}
		} else {
			fprintf(stderr, "unknown AF_xxx family: %d\n",
			    sa->sa_family);
		}

		close(client_fd);
	}
	return (0);
}
#endif
