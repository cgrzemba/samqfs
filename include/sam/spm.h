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

#ifndef SPM_H
#define	SPM_H

#ifdef sun
#pragma ident "$Revision: 1.16 $"
#endif


#define	SPM_PORT_STRING "sam-qfs"

#define	SPM_ERRSTR_MAX 246
#define	SPM_SERVICE_NAME_MAX 128

/*
 * Paths of Unix Domain Sockets
 */
#define	SPM_UDS_DAEMON_PATH SAM_VARIABLE_PATH "/uds/spmd"
#define	SPM_UDS_CLIENT_PATH SAM_VARIABLE_PATH "/uds/spm.%d"

struct spm_query_info {
	char sqi_service[SPM_SERVICE_NAME_MAX];
	struct spm_query_info *sqi_next;
};

int spm_initialize(int signo, int *error, char *errstr);
int spm_shutdown(int *error, char *errstr);
int spm_error(int *error, char *errstr);


int spm_accept(int service_fd, int *error, char *errstr);
int spm_register_service(char *service, int *error, char *errstr);
int spm_unregister_service(int service_fd);

int spm_connect_to_service(char *service, char *host, char *interface,
    int *error, char *errstr);

int spm_query_services(char *host, struct spm_query_info **result,
    int *error, char *errstr);

void spm_free_query_info(struct spm_query_info *info);

ssize_t spm_writen(int fd, const void *vptr, size_t n);
ssize_t spm_readnvtline(int fd, void *vptr, size_t maxlen);


#define	ESPM_OK		0    /* No error */
#define	ESPM_AI		1    /* getaddrinfo() error */
#define	ESPM_SOCKET	2    /* socket() error */
#define	ESPM_BIND	3    /* bind() error */
#define	ESPM_CONNECT	4    /* connect() error */
#define	ESPM_SIZE	5    /* argument is too big error */
#define	ESPM_NOMEM	6    /* No memory */
#define	ESPM_UNAVAIL	7    /* Service is unavailable */
#define	ESPM_RECVMSG	8    /* Error receiving message */
#define	ESPM_BADDESC	9    /* Descriptor was not passed */
#define	ESPM_SIGNAL	10   /* Error with sigaction */
#define	ESPM_AGAIN	11   /* SPM is already active */
#define	ESPM_PTHREAD	12   /* pthread error */
#define	ESPM_INTR	13   /* interrupted error */
#define	ESPM_POLL	14   /* poll() error */

#endif /* SPM_H */
