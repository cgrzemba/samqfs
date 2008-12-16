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

#pragma ident	"$Revision: 1.20 $"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "pub/mgmt/error.h"
#include "pub/mgmt/sqm_list.h"
#include "mgmt/util.h"
#include "sam/types.h"
#include "mgmt/config/rpc_secure_cfg.h"

/*
 * This program provides options to add remove and list clients that can
 * manage this SAM-FS/QFS server
 *
 * add <host1 host2 hostN...>: allow host to manage this SAM-FS server.
 *				by adding an entry to the config file
 *
 * remove <host1 host2 hostN...>: remove hostname from the config file
 *
 * list: list all hosts that can mamage this SAM-FS/QFS server
 *
 * Sysadm/user will add/remove/list clients via the samadm
 * samadm is the user of this program
 *
 */
#define	ADD "add"
#define	LIST "list"

#define	USAGE "%s ["ADD" <host1 ...> | remove <host1 ...> |"LIST"]"

int add_clients(sqm_lst_t *lst_client);
int remove_clients(sqm_lst_t *lst_client);
void list_clients();

extern void init_utility_funcs(void);

static int is_valid_name(char *hostname);
static int exists(char *client, sqm_lst_t *lst);

#define	PUT_CLIENTNAMES_IN_LST(argc, argv, lst) \
	{ int i = 0; \
		if (lst == NULL) { \
			lst = lst_create(); \
		} \
		for (i = 2; i < argc; i++) { \
			lst_append(lst, strdup(argv[i])); \
		} \
	}

#define	RM_CLIENT_FROM_CFGLST(client, lst) \
	{ node_t *cur = NULL, *nxt = NULL; \
		cur = lst->head; \
		while (cur != NULL) { \
			nxt = cur->next; \
			if (strcmp((char *)cur->data, client) == 0) { \
				free(cur->data); \
				if (lst_remove(lst, cur) < 0) { \
					return (-1); \
				} \
				remove_flag = B_TRUE; \
				break; \
			} \
			cur = nxt; \
		} \
	}

/*
 * argv[1] : name of operation (ADD, REMOVE, LIST)
 * argv[2] argv[3] ...: clientnames (dotted decimal format or hostnames)
 */
int
main(int argc, char **argv)
{
	int ret = 0;
	sqm_lst_t *lst = NULL;


	/* Init tracing, error and message facilities */
	init_utility_funcs();

	if ((argc < 2) || ((strcmp(argv[1], LIST) != 0) && (argc < 3))) {
		(void) fprintf(stderr, USAGE, argv[0]);
		return (1);
	}

	if (strcmp(argv[1], ADD) == 0) {

		/* TBD: Validate clientnames/ip */

		/* copy the clientnames to a list */
		PUT_CLIENTNAMES_IN_LST(argc, argv, lst);

		/* add clientnames to the config file */
		ret = add_clients(lst);
		lst_free_deep(lst);

	} else if (strcmp(argv[1], "remove") == 0) {

		/* TBD: Validate clientnames/ip */

		/* copy the clientnames in a list */
		PUT_CLIENTNAMES_IN_LST(argc, argv, lst);

		/* remove the clientnames from the config file */
		ret = remove_clients(lst);
		lst_free_deep(lst);

	} else if (strcmp(argv[1], LIST) == 0) {

		list_clients();
	} else {
		(void) fprintf(stderr, USAGE, argv[0]);
		return (1);
	}

	/* return 0 or 1 rather than 0 or -1 */
	ret = (ret < 0) ? 1 : 0;

	return (ret);
}


/*
 * list clients
 *
 * List the hosts that can manage this SAM-FS/QFS server
 * uses fprintf(stdout..) to print to stdout
 *
 */
void
list_clients()
{

	sqm_lst_t *lst_ipaddr = NULL;
	sqm_lst_t *lst_hostname = NULL;
	node_t *node = NULL;

	if (get_clients(&lst_ipaddr, &lst_hostname) < 0) {

		samerrno = SE_CANT_GET_CLIENTS;
		(void) snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), CLNTCFG, "");
		(void) strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);

		(void) fprintf(stderr, "%d:%s", samerrno, samerrmsg);
		return;
	}

	node = lst_ipaddr->head;
	while (node != NULL) {
		(void) fprintf(stdout, "%s\n", (char *)node->data);

		node = node->next;

	}
	lst_free_deep(lst_ipaddr);

	node = lst_hostname->head;
	while (node != NULL) {
		(void) fprintf(stdout, "%s\n", (char *)node->data);

		node = node->next;
	}
	lst_free_deep(lst_hostname);
}


/*
 * add clients
 *
 * Add host entries in the config file so that these hosts can manage
 * this SAM-FS/QFS server
 *
 * input argument	list of client (hostname/ip)
 * return 		0 on success, -1 on err
 *
 */
int
add_clients(
sqm_lst_t *lst_client	/* list of clients that can manage SAM-FS/QFS server */
)
{

	int		cnt_ip		= 0, cnt_host = 0;
	int		fd		= 0;
	size_t		size		= 0;
	clientCfg_t	*clients	= NULL;
	sqm_lst_t		*lst_ip		= NULL, *lst_host = NULL;
	node_t		*node		= NULL;

	if (ISNULL(lst_client)) {
		(void) fprintf(stderr, "%d:%s", samerrno, samerrmsg);
		return (-1);
	}

	/* Get the list of already supported clients */
	if (get_clients(&lst_ip, &lst_host) < 0) {

		/* could not get client ipaddr or client hostnames */
		(void) fprintf(stderr, "%d:%s", samerrno, samerrmsg);
		return (-1);
	}

	/* Calculate number of ips and number of hostnames to be added */
	node_t *nxt;
	node = lst_client->head;
	while (node != NULL) {
		nxt = node->next;

		/* check for validity and network access */
		if (is_valid_name((char *)node->data) == -1) {

			/* samerrmsg and samerrno set in is_valid_name */
			(void) fprintf(stderr, samerrmsg);
			free(node->data);
			lst_remove(lst_client, node);

		} else {
			/* ping to see if the ip address is reachable */
			if (nw_down((char *)node->data) != 0) {
				/* display warning */
				(void) fprintf(stderr,
				    GetCustMsg(SE_RPC_PING_FAILED),
				    (char *)node->data);
			}
			if (is_ipaddr((char *)node->data) == B_TRUE) {
				/* don't add duplicates */
				if (exists((char *)node->data, lst_ip)) {
					free(node->data);
					lst_remove(lst_client, node);

				} else {
					/* needs to be added */
					cnt_ip++;
				}
			} else {
				/* expressed as a hostname */
				if (exists((char *)node->data, lst_host)) {
					free(node->data);
					lst_remove(lst_client, node);
				} else {
					cnt_host++;
				}
			}
		}
		node = nxt;
	}

	if (cnt_ip == 0 && cnt_host == 0) { /* nothing is to be added */
		lst_free_deep(lst_ip);
		lst_free_deep(lst_host);
		return (0);
	}

	/* calculate the number of elements in the dynamic array */
	size = sizeof (clientCfg_t) +
	    ((lst_host->length + lst_ip->length + cnt_ip + cnt_host) > 0 ?
	    (lst_host->length + lst_ip->length + cnt_ip + cnt_host - 1) :
	    0) * sizeof (upath_t);

	if ((clients = (clientCfg_t *)mallocer(size)) == NULL) {
		lst_free_deep(lst_ip);
		lst_free_deep(lst_host);
		return (-1);
	}
	(void) memset(clients, 0, size);

	/* first copy the client ip from the list */
	node = lst_ip->head;
	while (node != NULL) {
		(void) strncpy(clients->addr_arr[clients->num_ip++],
		    (char *)node->data, sizeof (upath_t));

		node = node->next;
	}

	/* copy the newly added clients ip */
	node = lst_client->head;
	while (node != NULL) {

		/* append the client ip to the list of existing clients */
		if (is_ipaddr((char *)node->data) == B_TRUE) {

			(void) strncpy(
			    clients->addr_arr[clients->num_ip++],
			    (char *)node->data, sizeof (upath_t));
		}

		node = node->next;
	}

	/* copy the hostnames from the list */
	node = lst_host->head;
	while (node != NULL) {
		(void) strncpy(
		    clients->addr_arr[clients->num_ip + clients->num_host++],
		    (char *)node->data, sizeof (upath_t));

		node = node->next;
	}

	node = lst_client->head;
	while (node != NULL) {

		if (is_ipaddr((char *)node->data) == B_FALSE) {

			(void) strncpy(clients->addr_arr[
			    clients->num_ip + clients->num_host++],
			    (char *)node->data, sizeof (upath_t));
		}

		node = node->next;
	}
	/* If the config file does not exist, create the file, else trucate */
	if ((fd = open(CLNTCFG, O_RDWR | O_TRUNC | O_CREAT, 0644)) < 0) {

		/* cannot create the file */
		samerrno = SE_CREATE_FILE_FAILED;
		(void) snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), CLNTCFG, "");
		(void) strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);
		(void) fprintf(stderr, "%d:%s", samerrno, samerrmsg);
		free(clients);
		goto err;
	}

	if (write(fd, clients, size) < 0) {
		samerrno = SE_WRITE_MODE_ERROR;
		(void) snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), CLNTCFG, "");
		(void) strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);

		(void) fprintf(stderr, "%d:%s", samerrno, samerrmsg);
		(void) close(fd);
		free(clients);
		goto err;

	}
	(void) close(fd);

	free(clients);
	lst_free_deep(lst_ip);
	lst_free_deep(lst_host);
	return (0);

err:
	lst_free_deep(lst_ip);
	lst_free_deep(lst_host);
	return (-1);
}


/*
 * remove_clients
 *
 * remove clients from the config file. Requests to the SAM-FS/QFS server
 * via the SAM-QFS Manager daemon will be refused.
 *
 * input argument	list of clients
 * return 		0 on success, -1 on err
 *
 */
int
remove_clients(
sqm_lst_t *lst_client	/* list of clients to be removed from cfg file */
)
{

	int		fd		= 0;
	size_t		size		= 0;
	sqm_lst_t		*lst_ip	= NULL, *lst_host = NULL;
	node_t		*node		= NULL;
	boolean_t	remove_flag	= B_FALSE;
	clientCfg_t	*clients	= NULL;

	/* Get the list of clients */
	if (get_clients(&lst_ip, &lst_host) < 0) {

		/* could not get client ipaddr or client hostnames */
		(void) fprintf(stderr, "%d:%s", samerrno, samerrmsg);
		return (-1);
	}

	node = lst_client->head;
	while (node != NULL) {
		/* check if the clientname/ip exists in the lst */
		if (is_ipaddr((char *)node->data) == B_TRUE) {

			RM_CLIENT_FROM_CFGLST((char *)node->data, lst_ip);
		} else {

			RM_CLIENT_FROM_CFGLST((char *)node->data, lst_host);
		}
		node = node->next;

	}

	if (remove_flag == B_TRUE) {

		if (lst_ip->length == 0 && lst_host->length == 0) {

			lst_free(lst_ip);
			lst_free(lst_host);

			/* remove the file */
			return (remove(CLNTCFG));
		}

		size =
		    sizeof (clientCfg_t) +
		    ((lst_ip->length + lst_host->length > 0 ?
		    lst_ip->length + lst_host->length - 1 :
		    0) * sizeof (upath_t));

		if ((clients = mallocer(size)) == NULL) {
			(void) fprintf(stderr, "%d:%s", samerrno, samerrmsg);
			lst_free_deep(lst_ip);
			lst_free_deep(lst_host);
			return (-1);
		}
		(void) memset(clients, 0, size);

		/* copy the client ipaddr from the list */
		node = lst_ip->head;
		while (node != NULL) {
			(void) strncpy(clients->addr_arr[clients->num_ip++],
			    (char *)node->data, sizeof (upath_t));

			node = node->next;
		}

		/* copy the hostnames from the list */
		node = lst_host->head;
		while (node != NULL) {
			(void) strncpy(clients->addr_arr[
			    clients->num_ip + clients->num_host++],
			    (char *)node->data, sizeof (upath_t));

			node = node->next;
		}

		if ((fd = open(CLNTCFG, O_RDWR | O_TRUNC, 0644)) < 0) {

			/* cannot create the file */
			samerrno = SE_CANT_OPEN;
			(void) snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), CLNTCFG, "");
			(void) strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);

			(void) fprintf(stderr, "%d:%s", samerrno, samerrmsg);

			free(clients);
			lst_free_deep(lst_ip);
			lst_free_deep(lst_host);
			return (-1);
		}

		if (write(fd, clients, size) < 0) {
			samerrno = SE_WRITE_MODE_ERROR;
			(void) snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), CLNTCFG, "");
			(void) strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);

			(void) fprintf(stderr, "%d:%s", samerrno, samerrmsg);

			(void) close(fd);
			free(clients);
			lst_free_deep(lst_ip);
			lst_free_deep(lst_host);
			return (-1);
		}

		(void) close(fd);
		free(clients);
	}

	lst_free_deep(lst_ip);
	lst_free_deep(lst_host);
	return (0);
}


/*
 * exists
 *
 * check if client exists in list
 */
int
exists(
char *client,	/* list of clients to be added/removed */
sqm_lst_t *lst	/* list of configured client ips/hostnames */
)
{

	node_t *node = NULL;

	if (ISNULL(client, lst)) {
		return (B_FALSE);
	}

	node = lst->head;

	while (node != NULL) {
		if (strcmp(node->data, client) == 0) {
			return (B_TRUE);
		}
		node = node->next;
	}

	return (B_FALSE);
}

/*
 * is_valid_name
 * from RFC 952
 * "name" should be a text string drawn from the
 * alphabet (A-Z), digits (0-9), minus sign (-), and period (.)
 *
 */
int
is_valid_name(
char *hostname
)
{

	int i = 0;
	char *name = NULL;

	if (ISNULL(hostname)) {
		/* samerr is set */
		return (-1);
	}
	name = hostname;

	for (i = 0; name[i] != '\0'; i++) {

		if (!isalnum(name[i]) &&
		    name[i] != '-' && name[i] != '_' &&
		    name[i] != '.' && name[i] != ':') {

			samerrno = SE_INVALID_HOSTNAME;
			(void) snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), name);
			return (-1);
		}
	}
	return (0);
}

/*
 * is_ipaddr
 *
 * return B_TRUE if address is in the dotted decimal format (e.g. 127.0.0.1)
 * return B_FALSE if address is expressed as an alias or a hostname
 */
boolean_t
is_ipaddr(char *ip)
{
	/*
	 * This might not be the best way to diffrentiate between an IP
	 * address in dotted decimal format or hostname. This does not
	 * detect malformed addresses or incorrect or non-existant addrs
	 */
	if ((in_addr_t)(-1) == inet_addr(ip)) {
		/* IP-address is not of the form a.b.c.d */
		return (B_FALSE);
	}

	return (B_TRUE);
}
