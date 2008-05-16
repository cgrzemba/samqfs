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

#pragma ident	"$Revision: 1.14 $"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include "pub/mgmt/error.h"
#include "mgmt/util.h"
#include "pub/mgmt/sqm_list.h"
#include "sam/types.h"
#include "mgmt/config/rpc_secure_cfg.h"

/*
 * get_clients
 *
 * Get clients that are allowed to manage this local SAM-FS/QFS server
 * Read from the config file
 * return 0 if success, -1 on err (samerrno and samerrmsg are populated)
 *
 */
int
get_clients(
sqm_lst_t **lst_ipaddr,	/* return - list of client ipaddr */
sqm_lst_t **lst_hostname	/* return - list of client hostname */
)
{

	int fd, i = 0;
	struct stat st;
	clientCfg_t *clients;
	void *mp;
	sqm_lst_t *lst_ip;
	sqm_lst_t *lst_host;

	if (ISNULL(lst_ipaddr, lst_hostname)) {
		return (-1);
	}

	*lst_ipaddr = NULL;
	*lst_hostname = NULL;

	lst_ip = lst_create();
	if (lst_ip == NULL) {
		return (-1);
	}

	lst_host = lst_create();
	if (lst_host == NULL) {
		lst_free(lst_ip);
		return (-1);
	}

	/* file exists, map the client config file */
	if ((fd = open(CLNTCFG, O_RDONLY)) < 0) {
		if (errno == ENOENT) {
			*lst_ipaddr = lst_ip;
			*lst_hostname = lst_host;

			return (0);
		} else {
			samerrno = SE_CANT_OPEN;
			(void) snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), CLNTCFG, "");
			(void) strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);

			(void) fprintf(stderr, "%d:%s", samerrno, samerrmsg);

			lst_free(lst_ip);
			lst_free(lst_host);
			return (-1);
		}
	}

	if (fstat(fd, &st) < 0) {

		samerrno = SE_FSTAT_FAILED;
		(void) snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), CLNTCFG);

		(void) fprintf(stderr, "%d:%s", samerrno, samerrmsg);

		(void) close(fd);
		lst_free(lst_ip);
		lst_free(lst_host);
		return (-1);
	}

	if ((mp = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE,
	    fd, 0)) == MAP_FAILED) {

		samerrno = SE_MMAP_FAILED;
		(void) snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), CLNTCFG, "");
		(void) strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);

		(void) fprintf(stderr, "%d:%s", samerrno, samerrmsg);

		(void) close(fd);
		lst_free(lst_ip);
		lst_free(lst_host);
		return (-1);
	}
	(void) close(fd);

	clients = (clientCfg_t *)mp;
	for (i = 0; i < clients->num_ip; i++) {
		lst_append(lst_ip, strdup(clients->addr_arr[i]));
	}

	for (i = clients->num_ip;
	    i < (clients->num_ip + clients->num_host); i++) {
		lst_append(lst_host, strdup(clients->addr_arr[i]));
	}

	(void) munmap(mp, st.st_size);

	*lst_ipaddr =  lst_ip;
	*lst_hostname = lst_host;

	return (0);
}
