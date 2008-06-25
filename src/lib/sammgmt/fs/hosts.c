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
#pragma ident "	$Revision: 1.43 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* Solaris header files */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>

/* used for IP discovery */
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <alloca.h>

/* API header files */
#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/device.h"
#include "pub/mgmt/filesystem.h"
#include "pub/mgmt/hosts.h"

/* other header files */
#include "mgmt/config/cfg_hosts.h"
#include "mgmt/config/common.h"
#include "mgmt/util.h"
#include "mgmt/control/fscmd.h"
#include "sam/sam_trace.h"
#include "sam/param.h"
#include "sam/shareops.h"
#include "samhost.h"
#include "sam/mount.h"

/* client info headers */
#include "sam/syscall.h"
#include "sam/lib.h"
/*
 *  Client status bit definitions - see src/fs/include/client.h
 */
#define	SAM_CLIENT_DEAD 0x01    /* Client assumed dead during failover */
#define	SAM_CLIENT_INOP 0x02    /* client known dead */
#define	SAM_CLIENT_SOCK_BLOCKED 0x04    /* Writing to client returned EAGAIN */
#define	SAM_CLIENT_OFF_PENDING  0x08	/* Client transitioning to OFF */
#define	SAM_CLIENT_OFF  0x10		/* Client marked OFF in hosts file */

#define	IFCONFIG_CMD "/usr/sbin/ifconfig -a"
#define	CL_STATE_ON_STR "on"
#define	CL_STATE_OFF_STR "off"

static int write_host_line(FILE *f, host_info_t *h);
static int make_hosts_list(char ***charhosts, sqm_lst_t **hosts,
    upath_t server_name, struct sam_get_fsclistat_arg *clnt_stat,
    int kv_options);

static int client_state_to_int(char *state_str);

static int cp_host_fields(host_info_t *dest, host_info_t *src);

static int get_host_kv(char **charhosts, sam_client_info_t *clnt_stat,
    int kv_options, char **kv_string);

static int get_host_struct(char **charhosts, upath_t server_name,
    host_info_t **h);


static int
get_names_for_ip(
char	*ip,	/* IP address as a string */
int	af,	/* address family. AF_INET or AF_INET6 */
sqm_lst_t *lst)	/* add addresses to this list */
{

	struct hostent *hp = NULL;
	char **q;
	int error_num;
	int addr_sz = (af == AF_INET) ? 4 : 16;
	char *addr = (char *)alloca(addr_sz);

	if (NULL == addr)
		return (-1);
	memset(addr, 0, (size_t)addr_sz);
	if (1 != inet_pton(af, ip, (void *)addr)) {
		Trace(TR_ERR, "inet_pton(%d,%s) error: %s",
		    af, Str(ip), strerror(errno));
		return (-1);
	}
	hp = getipnodebyaddr(addr, addr_sz, af, &error_num);
	if (NULL == hp) {
		freehostent(hp);
		Trace(TR_OPRMSG, "no ip node for addr %s. error %d",
		    Str(ip), error_num);
		return (-1);
	}
	if (NULL != hp->h_name)
		lst_append(lst, strdup(hp->h_name));
	if (NULL != (q = hp->h_aliases))
		for (; *q != 0; q++) {
			lst_append(lst, strdup(*q));
		}
	freehostent(hp);
	return (0);
}

/*
 * obtain a list of ip addresses and names under which this host is known
 */
int
discover_ip_addresses(
ctx_t *ctx	/* ARGSUSED */,
sqm_lst_t **addrs)
{
#define	LOOPBACK_IP4 "127.0.0.1"
#define	LOOPBACK_IP6 "::1/128"

	FILE *res_stream;
	char cmd[] = IFCONFIG_CMD
	    /* extract ip address for each adapter */
	    " | /usr/bin/awk '{ if ($1 ~ /^inet.*/) print $1 \" \" $2}'";
	int status;
	pid_t pid;
	char type[10], ip[50];


	Trace(TR_MISC, "discovering ip addrs/names");
	if (ISNULL(addrs)) {
		Trace(TR_ERR, "null argument found (addrs)");
		return (-1);
	}
	*addrs = lst_create();
	if (*addrs == NULL) {
		Trace(TR_ERR, "lst create failed");
		return (-1);
	}

	if (-1 == (pid = exec_get_output(cmd, &res_stream, NULL))) {
		samerrno = SE_CANT_GET_HOST_IPS;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "Could not collect network info using %s",
		    IFCONFIG_CMD);
		return (-1);
	}

	while (EOF != fscanf(res_stream, "%s %s", type, ip)) {
		Trace(TR_DEBUG, "addrtype=%s ip=%s", type, ip);
		if (0 == strcmp(type, "inet")) { // IPv4
			if (0 != strcmp(ip, LOOPBACK_IP4)) {
				/* add this IP to the list */
				if (-1 == lst_append(*addrs, strdup(ip))) {
					Trace(TR_ERR,
					    "cannot create list element (ip)");
					fclose(res_stream);
					return (-1);
				}
				/* add the names corresponding to this IP */
				get_names_for_ip(ip, AF_INET, *addrs);
			}
		} else if (0 == strcmp(type, "inet6")) { // IPv6
			if (0 != strcmp(ip, LOOPBACK_IP6)) {
				/* add this IP to the list */
				ip[strcspn(ip, "/")] = '\0';
				if (-1 == lst_append(*addrs, strdup(ip))) {
					Trace(TR_ERR,
					    "cannot create list element (ip)");
					fclose(res_stream);
					return (-1);
				}
				/* add the names corresponding to this IP */
				get_names_for_ip(ip, AF_INET6, *addrs);
			}
		}
		}
	fclose(res_stream);
	waitpid(pid, &status, 0);
	Trace(TR_MISC, "IP address/name info obtained");
	return (0);
}


/*
 * Remove a host from the hosts.<fs_name> file for a shared file system.
 * If the host being removed is the metadata server, the hosts file will be
 * removed.
 *
 * preconditions:
 * 1. file system must be unmounted on all hosts.
 * 2. This function must be called on a metadata server or a potential metadata
 *    server.
 */
int
remove_host(
ctx_t *ctx,	/* ARGSUSED */
char *fs_name,	/* name of shared file system from which to remove host */
char *host_name) /* name of host to remove */
{

	fs_t *fs;
	int err;

	Trace(TR_MISC, "remove host %s for %s",  Str(host_name),
	    Str(fs_name));


	if (ISNULL(fs_name, host_name)) {
		Trace(TR_ERR, "removing host failed:%s", samerrmsg);
		return (-1);
	}


	/* Note that if -2 is returned by get_fs you can continue. */
	err = get_fs(NULL, fs_name, &fs);
	if (err == -1) {
		Trace(TR_ERR, "removing host failed:%s", samerrmsg);
		return (-1);
	}

	if (!fs->fi_shared_fs) {
		samerrno = SE_CLIENTS_HAVE_NO_HOSTS_FILE;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ONLY_SHARED_HAVE_HOSTS), fs_name);
		free_fs(fs);
		Trace(TR_ERR, "removing host failed:%s", samerrmsg);
		return (-1);
	}

	if (cfg_remove_host(NULL, fs, host_name) != 0) {
		free_fs(fs);
		Trace(TR_ERR, "removing host failed:%s", samerrmsg);
		return (-1);
	}

	free_fs(fs);

	Trace(TR_MISC, "removed host %s for file system %s", Str(host_name),
	    Str(fs_name));

	return (0);
}


/*
 * preconditions: fs_t must have been retrieved from the get_fs or get_all_fs
 * routines.
 */
int
cfg_remove_host(
ctx_t *ctx,
fs_t *fs,
char *host_name)
{

	char *new_server = NULL;
	node_t *n;
	boolean_t found = B_FALSE;
	boolean_t was_server = B_FALSE;
	upath_t hosts_file;

	Trace(TR_DEBUG, "removing host %s from config", host_name);

	/* Check the mount status. */
	if (fs->fi_status & FS_MOUNTED) {
		samerrno = SE_FS_MNTD_CANT_DEL_CLT;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FS_MNTD_CANT_DEL_CLT));
		Trace(TR_ERR, "removing host from config failed: %s",
		    samerrmsg);
		return (-1);

	}

	if (ISNULL(fs->hosts_config)) {

		Trace(TR_ERR, "removing host from config failed: %s",
		    samerrmsg);
		return (-1);
	}


	/* 2. remove the host in question */
	for (n = fs->hosts_config->head; n != NULL; n = n->next) {
		host_info_t *h = (host_info_t *)n->data;
		if (strcasecmp(h->host_name, host_name) == 0) {

			if (lst_remove(fs->hosts_config, n) != 0) {
				Trace(TR_ERR,
				    "removing host from config failed: %s",
				    samerrmsg);

				return (-1);
			}
			found = B_TRUE;
			if (h->current_server) {
				was_server = B_TRUE;
			}
			free_host_info(h);
			break;
		}
	}

	if (!found) {
		samerrno = SE_HOST_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_HOST_NOT_FOUND), Str(host_name));

		Trace(TR_ERR, "removing host from config failed: %s",
		    samerrmsg);

		return (-1);
	}

	/*
	 * if the host being removed is the server, the file system is
	 * being deleted so remove the file and return.
	 */
	snprintf(hosts_file, sizeof (hosts_file), "%s/hosts.%s",
	    CFG_DIR, fs->fi_name);

	if (was_server) {
		/*
		 * ignore any errors. The removal of the file is
		 * not critical
		 */
		unlink(hosts_file);

		return (0);
	}

	/* 3. set the new config */
	if (cfg_set_hosts_config(ctx, fs) != 0) {

		Trace(TR_ERR, "removing host from config failed: %s",
		    samerrmsg);

		return (-1);
	}


	/* activate the new config */
	if (sharefs_cmd(fs->fi_name, new_server, B_TRUE, B_FALSE,
	    B_FALSE) != 0) {
		Trace(TR_ERR, "removing host from config failed: %s",
		    samerrmsg);

		return (-1);
	}


	unlink(hosts_file);

	Trace(TR_DEBUG, "removed host %s from config", host_name);
	return (0);
}




/*
 * Function to add/modify a host in the hosts.<fs_name> file of a shared file
 * system. If the host already exists it will have its fields updated based
 * on in the input host_info_t.
 *
 * This function does not need to be called when creating the file system on
 * a metadata server nor when creating the file system on a potential metadata
 * server. However when creating a shared file system on a client, this
 * function must be called on the metadata server or a potential metadata
 * server to add the new client.
 *
 * preconditions:
 * 1. The function must be called on the metadata server
 * or on a potential metadata server for the file system.
 */
int
add_host(
ctx_t *ctx,
char *fs_name,		/* Name of fs to add host for */
host_info_t *h)		/* host to add */
{

	fs_t *fs;
	int err;
	node_t *n;

	if (ISNULL(fs_name, h)) {
		Trace(TR_ERR, "addinghost failed:%s", samerrmsg);
		return (-1);
	}
	if (ISNULL(h->host_name)) {
		Trace(TR_ERR, "adding host failed:%s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "adding host %s for %s server priority = %d",
	    h->host_name, fs_name, h->server_priority);


	/* Note that if -2 is returned by get_fs you can continue. */
	err = get_fs(NULL, fs_name, &fs);
	if (err == -1) {
		Trace(TR_ERR, "adding host failed:%s", samerrmsg);
		return (-1);
	}

	if (!fs->fi_shared_fs) {
		samerrno = SE_ONLY_SHARED_HAVE_HOSTS;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ONLY_SHARED_HAVE_HOSTS), fs_name);
		free_fs(fs);
		Trace(TR_ERR, "adding host failed:%s", samerrmsg);
		return (-1);
	}

	for (n = fs->hosts_config->head; n != NULL; n = n->next) {
		host_info_t *tmp = (host_info_t *)n->data;

		/* If the host already has an entry update it */
		if (strcasecmp(tmp->host_name, h->host_name) == 0) {


			cp_host_fields(tmp, h);
			if (cfg_set_hosts_config(ctx, fs) != 0) {
				free_fs(fs);
				Trace(TR_ERR, "updating host config failed:%s",
				    samerrmsg);
				return (-1);
			}

			/* Now activate this config change */
			if (sharefs_cmd(fs->fi_name, NULL, B_TRUE,
			    (fs->fi_status & FS_MOUNTED), B_FALSE) != 0) {

				Trace(TR_ERR, "update host for %s failed: %s",
				    fs->fi_name, samerrmsg);
				return (-1);
			}
			free_fs(fs);
			Trace(TR_MISC, "updated existing host");

			return (0);
		}
	}

	if (cfg_add_host(ctx, fs, h) != 0) {
		free_fs(fs);
		Trace(TR_ERR, "adding host failed:%s", samerrmsg);
		return (-1);
	}

	free_fs(fs);

	Trace(TR_MISC, "added host %s for %s",  h->host_name, fs_name);
	return (0);
}


/*
 * Function will not copy the current server field
 * Nor does it remove hosts from dest if they are not present in src.
 */
static int
cp_host_fields(host_info_t *dest, host_info_t *src) {
	node_t *n;

	dest->server_priority = src->server_priority;
	dest->state = src->state;

	/*
	 * Don't allow this function to change the current server
	 * setting. So do not copy current_server.
	 */

	/*
	 * look for IP addresses from src in dest.
	 */
	for (n = src->ip_addresses->head; n != NULL; n = n->next) {
		node_t *found;

		found = lst_search(dest->ip_addresses, n->data,
			(lstsrch_t)strcmp);

		/*
		 * If an address is in src and not in dest add it to
		 * dest.
		 */
		if (found == NULL) {
			char *newip;
			if ((newip = copystr((char *)n->data)) == NULL) {
				return (-1);
			}
			if (lst_append(dest->ip_addresses, newip) != 0) {
				return (-1);
			}
		}
	}
	return (0);
}


/*
 *
 * precondition:
 * 1. file must have been created
 * 2.
 *
 */
int
cfg_add_host(
ctx_t *ctx,	/* ARGSUSED */
fs_t *fs,	/* the file system for which the host is being added */
host_info_t *h) /* The host to add */
{

	FILE *f = NULL;
	int fd;
	upath_t hosts_file;
	upath_t err_buf;
	boolean_t mounted = B_FALSE;


	Trace(TR_MISC, "adding host %s to config for %s",
	    h->host_name, fs->fi_name);


	if (fs->fi_status & FS_MOUNTED) {
		mounted = B_TRUE;
	}


	/* create the local file for this potential metadata server */
	if (sharefs_cmd(fs->fi_name, NULL, B_FALSE,
	    mounted, B_TRUE) != 0) {

		Trace(TR_ERR, "adding host %s to config failed: %s",
		    fs->fi_name, samerrmsg);
		return (-1);
	}

	snprintf(hosts_file, sizeof (hosts_file), "%s/hosts.%s",
	    CFG_DIR, fs->fi_name);

	Trace(TR_MISC, "adding host %s: Writing file %s", fs->fi_name,
	    hosts_file);

	/* open the hosts file for an append */
	if ((fd = open(hosts_file, O_WRONLY|O_APPEND|O_CREAT, 0644)) != -1) {
		f = fdopen(fd, "a");
	}

	if (NULL == f) {
		samerrno = SE_CFG_OPEN_FAILED;

		/* Open failed for %s: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), hosts_file, err_buf);

		Trace(TR_ERR, "adding host %s to config failed: %s",
		    fs->fi_name, samerrmsg);

		return (-1);
	}

	if (write_host_line(f, h) != 0) {

		fclose(f);
		Trace(TR_ERR, "adding host %s to config failed: %s",
		    fs->fi_name, samerrmsg);
		return (-1);
	}

	if (fclose(f) != 0) {
		samerrno = SE_CFG_CLOSE_FAILED;

		/* Close failed for %s: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_CLOSE_FAILED), hosts_file, err_buf);

		Trace(TR_ERR, "adding host %s to config failed: %s",
		    fs->fi_name, samerrmsg);

		return (-1);
	}

	/* Now activate this config change */
	if (sharefs_cmd(fs->fi_name, NULL, B_TRUE,
	    mounted, B_FALSE) != 0) {

		Trace(TR_ERR, "adding host %s to config failed: %s",
		    fs->fi_name, samerrmsg);

		return (-1);
	}

	backup_cfg(hosts_file);

	if (mounted) {
		sleep(1);
		init_config(NULL);
	}

	unlink(hosts_file);

	Trace(TR_MISC, "added host %s config for %s",
	    h->host_name, fs->fi_name);

	return (0);
}


/*
 * Function for internal use to create a hosts configuration file.
 *
 * preconditions:
 * 1. This function is intended for  creating a new hosts
 * file. Any other use is at  callers risk. Specifically the caller is
 * responsible for verifying the  preconditions of any more specific
 * operation that is being achieved through this general function.
 *
 * 2. The fs_t structure must accurately reflect the actual mount state of the
 * file system.
 *
 * Postconditions:
 * 1. The hosts file has been created but sammkfs or samsharefs
 * must be used to cause the reading of this file.
 */
int
cfg_set_hosts_config(
ctx_t *ctx,
fs_t *fs)
{


	upath_t hosts_file;
	FILE *f = NULL;
	int fd;
	node_t *n;
	upath_t err_buf;


	Trace(TR_MISC, "setting hosts config for fs %s", fs->fi_name);


	/* See if it is a dump and set up the path appropriately */
	if (ctx != NULL && *ctx->dump_path != '\0') {
		char *tmp;

		snprintf(hosts_file, sizeof (hosts_file), "hosts.%s.dump",
		    fs->fi_name);

		tmp = assemble_full_path(ctx->dump_path, hosts_file,
		    B_FALSE, NULL);

		strlcpy(hosts_file, tmp, sizeof (hosts_file));
	} else {
		snprintf(hosts_file, sizeof (hosts_file), "%s/hosts.%s",
		    CFG_DIR, fs->fi_name);
	}

	if ((fd = open(hosts_file, O_WRONLY|O_CREAT|O_TRUNC, 0644)) != -1) {
		f = fdopen(fd, "w");
	}

	if (NULL == f) {
		samerrno = SE_CFG_OPEN_FAILED;

		/* Open failed for %s: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), hosts_file, err_buf);

		Trace(TR_ERR, "setting hosts config for fs %s failed: %s",
		    fs->fi_name, samerrmsg);

		return (-1);
	}

	for (n = fs->hosts_config->head; n != NULL; n = n->next) {
		if (write_host_line(f, (host_info_t *)n->data) != 0) {
			fclose(f);
			Trace(TR_ERR, "setting hosts config failed: %s",
			    samerrmsg);
			return (-1);
		}
	}


	if (fclose(f) != 0) {
		samerrno = SE_CFG_CLOSE_FAILED;

		/* Close failed for %s: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_CLOSE_FAILED), hosts_file, err_buf);

		Trace(TR_ERR, "setting hosts config for fs %s failed: %s",
		    fs->fi_name, samerrmsg);

		return (-1);
	}

	/* make a backup of the hosts file but ignore any errors */
	backup_cfg(hosts_file);

	Trace(TR_MISC, "set hosts config for fs %s", fs->fi_name);
	return (0);
}


/*
 * write the hosts line to the file referenced by the FILE pointer.
 */
static int
write_host_line(
FILE *f,
host_info_t *h)
{

	node_t *n;
	upath_t addresses;
	size_t addrlen = sizeof (addresses);
	char *cl_state_str;

	if (ISNULL(f)) {
		Trace(TR_ERR, "Null file pointer in write_host_line");
		return (-1);
	}

	if (h == NULL) {
		Trace(TR_MISC, "host line was null");
		return (0);
	}

	if (h->ip_addresses == NULL || h->ip_addresses->length == 0) {
		samerrno = SE_NO_ADDRESSES_SPECIFIED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NO_ADDRESSES_SPECIFIED));
		return (-1);
	}

	snprintf(addresses, addrlen, (char *)h->ip_addresses->head->data);

	/*
	 * build the address text
	 */
	for (n = h->ip_addresses->head->next; n != NULL; n = n->next) {
		if (n->data == NULL) {
			continue;
		}

		/* only copy the allowed amount. */
		if (strlen((char *)n->data) + strlen(addresses) + 1 >
		    addrlen) {
			break;
		}
		strlcat(addresses, ",", addrlen);
		strlcat(addresses, (char *)n->data, addrlen);
	}

	if (h->state == CL_STATE_ON) {
		cl_state_str = CL_STATE_ON_STR;
	} else if (h->state == CL_STATE_OFF) {
		cl_state_str = CL_STATE_OFF_STR;
	} else {
		samerrno = SE_INVALID_CLIENT_STATE_ARG;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    h->state);
		return (-1);
	}

	/* write the new host */
	if (h->current_server) {
		fprintf(f, "%s\t%s\t%d\t%s\t%s\n", h->host_name, addresses,
		    h->server_priority, cl_state_str, "server");
	} else {
		fprintf(f, "%s\t%s\t%d\t%s\n", h->host_name, addresses,
		    h->server_priority, cl_state_str);
	}

	return (0);
}

/*
 * Return an ordered list of the host_info_t structures. The order will
 * match the order in the hosts.<fs_name> file.
 *
 * Note:
 * 1. Must be called on a metadata server or potential metadata server.
 */
int
get_host_config(
ctx_t *ctx,		/* ARGSUSED */
char *fs_name,		/* name of fs to get hosts config for. */
sqm_lst_t **hosts)		/* Ordered list of host_info_t */
{
	fs_t *fs;

	if (ISNULL(fs_name, hosts)) {
		Trace(TR_ERR, "getting hosts config failed: %s", samerrmsg);
		return (-1);
	}


	Trace(TR_MISC, "getting hosts config for fs %s", fs_name);
	if (get_fs(NULL, fs_name, &fs) == -1) {
		Trace(TR_ERR, "getting hosts config failed: %s", samerrmsg);
		return (-1);
	}

	if (!fs->fi_shared_fs) {
		free_fs(fs);
		samerrno = SE_ONLY_SHARED_HAVE_HOSTS;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ONLY_SHARED_HAVE_HOSTS), fs_name);
		Trace(TR_ERR, "getting hosts config failed: %s", samerrmsg);
		return (-1);
	}

	if (cfg_get_hosts_config(NULL, fs, hosts, 0) != 0) {
		free_fs(fs);
		Trace(TR_ERR, "getting hosts config failed: %s", samerrmsg);
		return (-1);
	}

	free_fs(fs);

	Trace(TR_MISC, "returning hosts config for fs %s", fs_name);
	return (0);
}



int
get_shared_fs_hosts(
ctx_t	*c,		/* ARGSUSED */
char *fs_name,
int32_t options,
sqm_lst_t **hosts)
{

	fs_t *fs;

	if (ISNULL(fs_name, hosts)) {
		Trace(TR_ERR, "getting hosts config failed: %s", samerrmsg);
		return (-1);
	}


	Trace(TR_MISC, "getting hosts config for fs %s", fs_name);
	if (get_fs(NULL, fs_name, &fs) == -1) {
		Trace(TR_ERR, "getting hosts config failed: %s", samerrmsg);
		return (-1);
	}

	if (!fs->fi_shared_fs) {
		free_fs(fs);
		samerrno = SE_ONLY_SHARED_HAVE_HOSTS;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ONLY_SHARED_HAVE_HOSTS), fs_name);
		Trace(TR_ERR, "getting hosts config failed: %s", samerrmsg);
		return (-1);
	}

	if (cfg_get_hosts_config(NULL, fs, hosts, options) != 0) {
		free_fs(fs);
		Trace(TR_ERR, "getting hosts config failed: %s", samerrmsg);
		return (-1);
	}

	free_fs(fs);

	Trace(TR_MISC, "returning hosts config for fs %s", fs_name);
	return (0);
}


int
set_host_state(ctx_t *c, char *fs_name, sqm_lst_t *host_names,
    int client_state) {

	fs_t *fs;
	node_t *n;
	sqm_lst_t *cfg_hosts;
	boolean_t mounted = B_FALSE;
	int err;


	if (ISNULL(fs_name, host_names)) {
		Trace(TR_ERR, "setting client state failed: %s", samerrmsg);
		return (-1);
	}
	if (client_state != CL_STATE_OFF && client_state != CL_STATE_ON) {
		samerrno = SE_INVALID_CLIENT_STATE_ARG;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    client_state);
		Trace(TR_ERR, "setting client state failed: %s", samerrmsg);
		return (-1);
	}

	/* Note that if -2 is returned by get_fs you can continue. */
	err = get_fs(NULL, fs_name, &fs);
	if (err == -1) {
		Trace(TR_ERR, "setting client state failed: %s", samerrmsg);
		return (-1);
	}

	if (!fs->fi_shared_fs) {
		samerrno = SE_ONLY_SHARED_HAVE_HOSTS;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ONLY_SHARED_HAVE_HOSTS), fs_name);
		free_fs(fs);
		Trace(TR_ERR, "setting client state failed: %s", samerrmsg);
		return (-1);
	}
	if (cfg_get_hosts_config(NULL, fs, &cfg_hosts, 0) != 0) {
		free_fs(fs);
		Trace(TR_ERR, "setting client state failed: %s", samerrmsg);
		return (-1);
	}

	fs->hosts_config = cfg_hosts;

	for (n = host_names->head; n != NULL; n = n->next) {
		char *name = (char *)n->data;
		node_t *cfg_node;
		host_info_t *h;

		if (name == NULL || *name == '\0') {
			continue;
		}

		/*
		 * Use lst_search with cmp_str_2_str_ptr because the
		 * host name char pointer is the first field in the
		 * host_info_t structs.
		 */
		cfg_node = lst_search(cfg_hosts, name, cmp_str_2_str_ptr);
		if (cfg_node == NULL) {
			continue;
		}
		h = (host_info_t *)cfg_node->data;
		h->state = client_state;
	}

	/* set the new config */
	if (cfg_set_hosts_config(c, fs) != 0) {
		free_fs(fs);
		Trace(TR_ERR, "Setting client state failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (fs->fi_status & FS_MOUNTED) {
		mounted = B_TRUE;
	}

	/* activate the new config */
	if (sharefs_cmd(fs->fi_name, NULL, B_TRUE, mounted,
	    B_FALSE) != 0) {
		free_fs(fs);
		Trace(TR_ERR, "setting client state failed: %s",
		    samerrmsg);

		return (-1);
	}

	free_fs(fs);
	Trace(TR_ERR, "set client state");

	return (0);
}


/*
 * Internal function to get a hosts list for a shared file system, given
 * the file system structure. This function fetches the information in one
 * of two formats depending on the kv_options. If kv_options != 0 the
 * hosts list will contain key value pair strings. Otherwise it will
 * include host_info_t structures.
 *
 * Preconditions:
 * 1. file system is shared.
 * 2. call is being made on a metadata server or a potential metadata server.
 *    Note however that for calls on a potential metadata server the client
 *    table information will not be available from SC_getfsclistat so there
 *    will be no additional information beyond that available from the
 *    file system's host table.
 */
int
cfg_get_hosts_config(
ctx_t		*ctx,		/* ARGSUSED */
fs_t		*fs,		/* fs to get hosts for */
sqm_lst_t	**hosts,	/* malloced sqm_lst_t of host_info_t for fs */
int32_t		kv_options)	/* what to fetch and include */
{

	struct sam_host_table_blk *host_tbl;
	char	***charhosts = NULL;
	char	*errmsg;
	int	errc = 0;
	char	*devname = NULL;
	int htsize = SAM_LARGE_HOSTS_TABLE_SIZE;


	/* Client Info Table Structs */
	struct sam_get_fsclistat_arg stat_arg;
	struct sam_get_fsclistat_arg *clnt_stat = NULL;
	sam_client_info_t *clnts = NULL;

	Trace(TR_OPRMSG, "getting hosts config for %s", Str(fs->fi_name));

	/* Allocate space to hold the new large host table */
	host_tbl = (struct sam_host_table_blk *)mallocer(htsize);
	if (host_tbl == NULL) {
		Trace(TR_ERR, "getting hosts config failed: %s",
		    samerrmsg);
		return (-1);
	}
	bzero((char *)host_tbl, htsize);


	/* Get the host table from the file system or the raw device. */
	if (!(fs->fi_status & FS_MOUNTED)) {
		/*
		 * file system is not mounted get the information from
		 * the first metadata device.
		 */
		if (strcmp(fs->equ_type, "ma") == 0 &&
		    (fs->meta_data_disk_list == NULL ||
		    fs->meta_data_disk_list->length == 0)) {
			samerrno = SE_UNABLE_TO_READ_RAW_HOSTS;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_UNABLE_TO_READ_RAW_HOSTS),
			    fs->fi_name, "no metadata devices");
			free(host_tbl);
			Trace(TR_ERR, "getting hosts config failed: %s",
			    "no metadata devices");
			return (-1);
		} else {
			disk_t *d;

			/* select a device to read the hosts information from */
			if (strcmp(fs->equ_type, "ma") == 0) {
				d = (disk_t *)
				    fs->meta_data_disk_list->head->data;
			} else {
				/* fs is shared ms, so use a data disk */
				d = (disk_t *)fs->data_disk_list->head->data;
			}
			if (strcmp(d->base_info.name, NODEV_STR) == 0) {

				samerrno = SE_CLIENTS_HAVE_NO_HOSTS_FILE;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(SE_CLIENTS_HAVE_NO_HOSTS_FILE));
				free(host_tbl);
				Trace(TR_ERR,
				    "getting hosts config failed: %s",
				    samerrmsg);

				return (-1);
			}

			dsk2rdsk(d->base_info.name, &devname);

			if (devname == NULL) {
				samerrno = SE_CANNOT_DETERMINE_RAW_SLICE;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(SE_CANNOT_DETERMINE_RAW_SLICE),
				    fs->fi_name);
				free(host_tbl);
				Trace(TR_ERR,
				    "getting hosts config failed: %s",
				    samerrmsg);

				return (-1);
			}
		}

		if (SamGetRawHosts(devname, host_tbl, htsize,
		    &errmsg, &errc) < 0) {

			Trace(TR_OPRMSG, "getting host from raw slice failed");
			/*
			 * This can fail if the mcf does not have
			 * nodevs for the metadata but this host has a
			 * different byte ordering than the host on
			 * which the file system was created. In this case
			 * we do not want to throw an error because
			 * this host can only concievably be a client if
			 * its architecture is different than the architecture
			 * on which the file system was initially created.
			 */
			if (strstr(errmsg, "Foreign") != NULL ||
			    strstr(errmsg, "byte-swapped") != NULL) {

				Trace(TR_OPRMSG, "byte-swapped devs %s",
				    "detected ");
				*hosts = NULL;
				if (devname != NULL) {
					free(devname);
				}
				free(host_tbl);
				return (0);
			}

			samerrno = SE_UNABLE_TO_READ_RAW_HOSTS;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_UNABLE_TO_READ_RAW_HOSTS),
			    fs->fi_name, errmsg);

			Trace(TR_ERR, "getting hosts config failed: %s",
			    samerrmsg);
			Trace(TR_ERR, "tried to read from %s", devname);

			if (devname != NULL) {
				free(devname);
			}
			free(host_tbl);
			return (-1);
		}

	} else {
		/*
		 * The filesystem is mounted, so get the information from
		 * the core
		 */
		if (sam_gethost(fs->fi_name, htsize,
		    (char *)host_tbl) < 0) {

			samerrno = SE_UNABLE_TO_GET_HOSTS_FROM_CORE;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_UNABLE_TO_GET_HOSTS_FROM_CORE),
			    fs->fi_name);

			free(host_tbl);
			Trace(TR_ERR, "getting hosts config failed: %s",
			    samerrmsg);

			return (-1);
		}

		/*
		 * If this host is the current metadata server and
		 * kv_options is non-null get the client info.
		 *
		 * This code is inside the mounted block because...
		 * While the client table will be populated on the mds
		 * if the file system has been mounted since pkgadd or
		 * reboot, it does not appear to contain reliable
		 * useful information except for that which has
		 * already been obtained from the superblock.
		 */
		if (fs->fi_status & FS_SERVER && kv_options) {
			clnts = mallocer(SAM_MAX_SHARED_HOSTS *
			    sizeof (sam_client_info_t));

			if (clnts == NULL) {
				Trace(TR_ERR, "getting hosts config failed:"
				    "%d %s", samerrno, samerrmsg);
				free(host_tbl);
				return (-1);
			}

			strncpy(stat_arg.fs_name, fs->fi_name,
			    sizeof (stat_arg.fs_name));
			stat_arg.maxcli = SAM_MAX_SHARED_HOSTS;
			stat_arg.numcli = 0;
			stat_arg.fc.ptr = clnts;
			if (sam_syscall(SC_getfsclistat, &stat_arg,
			    sizeof (stat_arg)) < 0) {
				free(clnts);
				free(host_tbl);
				Trace(TR_ERR, "SC_getfsclistat failed");
				return (-1);
			}

			/*
			 * Since we now know we have good data- setup
			 * clnt_stat to pass the data to make_hosts_list
			 */
			if (stat_arg.fs_name != '\0') {
				clnt_stat = &stat_arg;
			}
		}
	}



	/*
	 * Convert the information from essentially a char array to an
	 * array of char arrays.
	 */
	if ((charhosts = SamHostsCvt(&host_tbl->info.ht, &errmsg,
	    &errc)) == NULL) {
		samerrno = SE_UNABLE_TO_CONVERT_HOSTS_FILE;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_UNABLE_TO_CONVERT_HOSTS_FILE),
		    fs->fi_name);

		SamHostsFree(charhosts);
		free(host_tbl);
		free(clnts);

		Trace(TR_ERR, "getting hosts config failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (make_hosts_list(charhosts, hosts, fs->fi_server,
	    clnt_stat, kv_options) != 0) {

		SamHostsFree(charhosts);
		free(host_tbl);
		free(clnts);

		Trace(TR_ERR, "getting hosts config failed: %s",
		    samerrmsg);

		return (-1);
	}

	SamHostsFree(charhosts);
	free(host_tbl);
	free(clnts);

	Trace(TR_MISC, "got hosts from config for %s", fs->fi_name);
	return (0);
}


/*
 * from a hosts char *** make a list of host_info_t structs
 */
static int
make_hosts_list(
char		***charhosts,
sqm_lst_t	**hosts,
upath_t		server_name,
struct sam_get_fsclistat_arg *clnt_stat,
int		kv_options)
{

	host_info_t *h = NULL;
	int i;


	*hosts = lst_create();
	if (*hosts == NULL) {
		Trace(TR_ERR, "Making host lists failed: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	for (i = 0; charhosts[i] != NULL; i++) {

		if (kv_options) {
			char *kv_string;
			sam_client_info_t *clnt = NULL;

			if (clnt_stat != NULL && i >= clnt_stat->maxcli) {
				/*
				 * The raw host table information is
				 * out of sync with what the client
				 * info table from the mds.  This
				 * should never happen. If it does it
				 * means that explicit comparisons of
				 * names or ordinals will be required.
				 */
				samerrno = SE_CLNT_TBL_OUT_OF_SYNC;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(samerrno));
				Trace(TR_ERR, "FS reports different number "
				    "of clients than the superblock:%d %s",
				    samerrno, samerrmsg);
				goto err;
			}

			if (clnt_stat != NULL) {
				clnt = &(((sam_client_info_t *)
				    clnt_stat->fc.ptr)[i]);
			}

			if (get_host_kv(charhosts[i], clnt,
			    kv_options, &kv_string) != 0) {
				goto err;
			}

			/*
			 * kv_string can be NULL in non-error
			 * cases. This is true if type of host is
			 * being filtered out due to its role in the
			 * file system.
			 */
			if (kv_string != NULL) {
				if (lst_append(*hosts, kv_string) != 0) {
					goto err;
				}
			}
		} else {
			h = NULL;

			if (get_host_struct(charhosts[i],
			    server_name, &h) != 0) {
				goto err;
			}
			if (lst_append(*hosts, h) != 0) {
				free_host_info(h);
				goto err;
			}
		}
	}

	return (0);

err:
	if (kv_options) {
		lst_free_deep(*hosts);
	} else {
		free_list_of_host_info(*hosts);
	}
	Trace(TR_ERR, "Making hosts lists failed:%d %d", samerrno, samerrmsg);
	return (-1);
}


/*
 * get_host_kv returns a key value string for the host. It also evaluates
 * the kv_options. If the type of host passed in is not requested in the options
 * success is returned and kv_string is set to NULL.
 */
static
int get_host_kv(
char **charhost,
sam_client_info_t *clnt,
int kv_options,
char **kv_string) {

	int priority = 0;
	size_t cur_sz = 0;
	int buf_sz = 2048; /* sufficient for hostname, ips, keys and values */
	char buf[buf_sz];
	char *ip_beg;
	char *c;
	boolean_t status_set = B_FALSE;

	*kv_string = NULL;
	buf[0] = '\0';

	if ((cur_sz = strlcat(buf, "hostname=", buf_sz)) > buf_sz) {
		goto err;
	}

	cur_sz = strlcat(buf, charhost[HOSTS_NAME], buf_sz);
	if (cur_sz > buf_sz) {
		Trace(TR_ERR, "getting host kv failed:%d %s");
		goto err;
	}


	if (*charhost[HOSTS_PRI] == '-') {
		priority = 0;
	} else {
		priority = atoi(charhost[HOSTS_PRI]);
	}

	/*
	 * Determine host type. If that type of host has not been requested
	 * return 0 without creating a kv string.
	 */
	if (priority == 0) {
		/*
		 * A priority of 0 means the host is a client. If clients
		 * have not been requested return
		 */
		if ((kv_options & HOSTS_CLIENTS) == 0) {
			return (0);
		}
		cur_sz = strlcat(buf, ",type=client", buf_sz);
	} else if (charhost[HOSTS_SERVER]) {
		if ((kv_options & HOSTS_MDS) == 0) {
			return (0);
		}

		cur_sz = strlcat(buf, ",type=mds", buf_sz);
		cur_sz = strlcat(buf, ",cur_mds=true", buf_sz);
	} else {
		if ((kv_options & HOSTS_MDS) == 0) {
			return (0);
		}
		cur_sz = strlcat(buf, ",type=pmds", buf_sz);
	}

	if (cur_sz > buf_sz) {
		goto err;
	}
	cur_sz = strlcat(buf, ",ip_addresses=", buf_sz);
	if (cur_sz > buf_sz) {
		goto err;
	}
	ip_beg = buf + cur_sz;
	cur_sz = strlcat(buf, charhost[HOSTS_IP], buf_sz);
	if (cur_sz > buf_sz) {
		goto err;
	}
	if ((buf + cur_sz) == ip_beg) {
		/* Missing ip addresses is an error */
		goto err;
	}
	/* Remove the commas in the ip address string */
	for (c = ip_beg; *c != '\0'; c++) {
		if (*c == ',') {
			*c = ' ';
		}
	}

	/*
	 * If clnt_stat == NULL or status == 0 skip the additional info
	 */
	if (clnt != NULL && clnt->cl_status != 0) {

		/*
		 * For off or unmounted hosts the hname gets cleared.
		 * If it is set, make sure it matches the name from
		 * charhosts.
		 */
		if (*clnt->hname != '\0' && strcmp(clnt->hname,
		    charhost[HOSTS_NAME]) != 0) {

			samerrno = SE_CLNT_TBL_OUT_OF_SYNC;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno));
			Trace(TR_ERR, "Mismatch for %s: %d %s",
			    Str(charhost[HOSTS_NAME]), samerrno, samerrmsg);
			return (-1);
		}

		if (clnt->cl_status & FS_MOUNTED) {
			cur_sz = strlcat(buf, ",mounted=1", buf_sz);
		} else {
			cur_sz = strlcat(buf, ",mounted=0", buf_sz);
		}
		if (cur_sz > buf_sz) {
			goto err;
		}

		if (clnt->cl_flags & SAM_CLIENT_DEAD) {
			cur_sz = strlcat(buf,
			    ",error=assumed_dead", buf_sz);
		}
		if (cur_sz > buf_sz) {
			goto err;
		}

		if (clnt->cl_flags & SAM_CLIENT_INOP) {
			cur_sz = strlcat(buf, ",error=gone",
			    buf_sz);
		}
		if (cur_sz > buf_sz) {
			goto err;
		}

		if (clnt->cl_flags & SAM_CLIENT_SOCK_BLOCKED) {
			cur_sz = strlcat(buf, ",error=blocked",
			    buf_sz);
		}
		if (cur_sz > buf_sz) {
			goto err;
		}

		if (clnt->cl_flags & SAM_CLIENT_OFF_PENDING) {
			status_set = B_TRUE;
			cur_sz = strlcat(buf, ",status=OFFPENDING", buf_sz);
			if (cur_sz > buf_sz) {
				goto err;
			}
		}

		if (kv_options & HOSTS_DETAILS) {
			char *cl_info_beg = buf + cur_sz;
			int added;
			added = snprintf(cl_info_beg, buf_sz - cur_sz,
			    ",status=0x%x,mnt_cfg=0x%x,mnt_cfg1=0x%x,"
			    "flags=0x%x,no_msg=%d,low_msg=%d",
			    clnt->cl_status, clnt->cl_config,
			    clnt->cl_config1, clnt->cl_flags,
			    clnt->cl_nomsg, clnt->cl_min_seqno);
			if ((added + cur_sz) > buf_sz) {
				goto err;
			}
		}
	}


	/*
	 * The charhost can have a number of different values that
	 * map to ON. Normalize those here and return either ON or OFF.
	 */
	if (!status_set) {
		if ((strcasecmp(charhost[3], "on") == 0) ||
		    (strcmp(charhost[3], "0") == 0) ||
		    (strcmp(charhost[3], "-") == 0)) {
			cur_sz = strlcat(buf, ",status=ON", buf_sz);
			if (cur_sz > buf_sz) {
				goto err;
			}
		} else {
			cur_sz = strlcat(buf, ",status=OFF", buf_sz);
			if (cur_sz > buf_sz) {
				goto err;
			}
		}
	}




	*kv_string = copystr(buf);
	if (*kv_string == NULL) {
		Trace(TR_ERR, "get host kv failed: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}
	return (0);

err:
	setsamerr(SE_HOST_KV_TOO_LONG);
	Trace(TR_ERR, "Host KV too long for %s %s", charhost[HOSTS_NAME], buf);
	return (-1);
}

static int
get_host_struct(
char **charhost,
upath_t server_name,
host_info_t **h) {


	*h = mallocer(sizeof (host_info_t));
	if (*h == NULL) {
		Trace(TR_ERR, "making lists of hosts failed: %d %s",
		    samerrno, samerrmsg);
		goto err;
	}

	memset(*h, 0, sizeof (host_info_t));
	(*h)->host_name = strdup(charhost[HOSTS_NAME]);
	if ((*h)->host_name == NULL) {
		Trace(TR_ERR, "making list of hosts failed:%d %s");
		goto err;
	}

	if (*charhost[HOSTS_PRI] == '-') {
		(*h)->server_priority = 0;
	} else {
		(*h)->server_priority = atoi(charhost[HOSTS_PRI]);
	}

	if (charhost[HOSTS_SERVER]) {
		(*h)->current_server = B_TRUE;
		strlcpy(server_name, (*h)->host_name, sizeof (upath_t));
	}

	(*h)->ip_addresses = str2lst(charhost[HOSTS_IP], ", ");
	if ((*h)->ip_addresses == NULL) {
		Trace(TR_ERR, "making host lists failed:%d %s",
		    samerrno, samerrmsg);
		goto err;
	}

	(*h)->state = client_state_to_int(charhost[HOSTS_HOSTONOFF]);
	if ((*h)->state < 0) {
		Trace(TR_ERR, "making host lists failed:%d %s",
		    samerrno, samerrmsg);
		goto err;
	}

	return (0);

err:
	free_host_info(*h);
	*h = NULL;
	return (-1);
}


static int
client_state_to_int(char *state_str) {

	/*
	 * The "-" and "zero" entries are here for backwards compatiblity.
	 */
	if ((strcasecmp(state_str, "on") == 0) ||
	    (strcmp(state_str, "0") == 0) ||
	    (strcmp(state_str, "-") == 0)) {

		return (CL_STATE_ON);

	} else if (strcasecmp(state_str, "off") == 0) {
		return (CL_STATE_OFF);
	} else {
		/*
		 * This state may mean the core has introduced a new
		 * STATE that we don't support yet OR there is an
		 * error in the configuration. Note that from the live
		 * file system there is an "off pending" state but that
		 * never gets written into the file so we don't need to
		 * convert it here.
		 */
		samerrno = SE_INVALID_CLIENT_STATE;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_CLIENT_STATE), Str(state_str));
		Trace(TR_ERR, "Invalid client state %s", Str(state_str));
		return (-1);
	}
}


/*
 * Internal function to create a hosts config. This function creates or
 * updates the hosts config as appropriate depending on wether this
 * host is a metadata server or a potential metadata server.
 *
 * preconditions: if this is a potential metadata server the
 * metadata server must have been created first.
 * If this is for the metadata server no precondition
 * This function should not be called for clients but will return success if
 * it is.
 */
int
create_hosts_config(
ctx_t *ctx,	/* ARGSUSED */
fs_t *fs)	/* The fs for which to create a host config */
{

	boolean_t found = B_FALSE;
	node_t *n;
	sqm_lst_t *cur_hosts = NULL;

	Trace(TR_MISC, "creating hosts config for %s", fs->fi_name);

	if ((fs->fi_status & FS_CLIENT) && (fs->fi_status & FS_NODEVS)) {
		/* Client Branch */
		return (0);
	}


	/*
	 * If the fs is a potential metadata server and this is not a
	 * dump enter the first branch
	 */
	if (fs->fi_status & FS_CLIENT &&
	    !(ctx != NULL && *ctx->dump_path != '\0')) {

		upath_t name;
		int sz;

		/* This branch is for potential metadata servers */
		Trace(TR_MISC, "creating hosts cfg for potential server of %s",
		    fs->fi_name);


		/* get our host name */
		sz = gethostname(name, sizeof (name));
		if (sz > sizeof (name)) {
			Trace(TR_ERR, "creating hosts config for %s failed %s",
			    fs->fi_name, samerrmsg);
			return (-1);
		}


		/*
		 * Check if the host is already present and don't add it if
		 * it is.
		 */
		if (cfg_get_hosts_config(ctx, fs, &cur_hosts, 0) != 0) {
			Trace(TR_ERR, "creating hosts config for %s failed %s",
			    fs->fi_name, samerrmsg);
			return (-1);
		}

		for (n = cur_hosts->head; n != NULL; n = n->next) {
			host_info_t *h = (host_info_t *)n->data;

			if (strcasecmp(h->host_name, name) == 0) {
				/* host is already present */

				free_list_of_host_info(cur_hosts);
				Trace(TR_MISC, "host %s is %s", name,
				    "already present in hosts config");

				return (0);
			}
		}
		free_list_of_host_info(cur_hosts);

		/*
		 * find this hosts name in the input fs_t struct so that
		 * its entry can be added to the hosts file
		 */
		for (n = fs->hosts_config->head; n != NULL; n = n->next) {
			host_info_t *h = (host_info_t *)n->data;

			if (strcasecmp(h->host_name, name) == 0) {
				if (cfg_add_host(NULL, fs, h) != 0) {
					Trace(TR_ERR, "creating hosts%s%s%s%s",
					    " config for ", fs->fi_name,
					    " failed: ", samerrmsg);
					return (-1);
				}
				found = B_TRUE;
				break;
			}
		}

		if (!found) {
			samerrno = SE_HOST_NOT_FOUND;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_HOST_NOT_FOUND), name);

			Trace(TR_ERR, "creating hosts config for %s failed %s",
			    fs->fi_name, samerrmsg);
			return (-1);
		}

	} else {

		/*
		 * This branch is for the metadata server or for a dump
		 * of a potential metadata server. In this branch
		 * we must create the hosts file(possibly in an alternate
		 * location).
		 */

		Trace(TR_MISC, "creating hosts cfg for metadata server for %s",
		    fs->fi_name);

		if (cfg_set_hosts_config(ctx, fs) != 0) {
			Trace(TR_ERR, "creating hosts config for %s failed %s",
			    fs->fi_name, samerrmsg);

			return (-1);
		}

	}


	Trace(TR_MISC, "created hosts config for %s", fs->fi_name);
	return (0);
}


/*
 * explode_local_host_line
 *
 * Break a line up into an array of strings.
 * Throw out anything after a '#'.
 */
static int
explode_local_host_line(FILE *fp, char **strs, int nstr, char *buf, int buflen)
{
	int c, insp, incom;
	char *bufp, *bufp2, **wp;

	insp = 1;
	incom = 0;
	wp = strs;


	if (nstr < 2) {
		Trace(TR_ERR, "explode_local_host_line:nstr < 2");
		return (-1);
	}

	/*
	 * set the first two pointers to NULL so caller can distinquish
	 * between an error and a comment
	 */
	*wp = NULL;
	*(wp+1) = NULL;

	bufp = buf;
	do {
		c = fgetc(fp);

		if (c == EOF) /* endoffile reached */
			return (0);
		if (bufp >= &buf[buflen]) { /* buffer exceeded */
			Trace(TR_ERR, "explode_local_host_line: %s",
			    "buffer overflow");
			return (-1);
		}
		if (wp >= &strs[nstr]) {
			Trace(TR_ERR, "explodeLocalHostLine: %s",
			    "word buffer overflow");
			return (-1);
		}
		if (incom)	/* you are in a comment */
			continue;
		if (c == '#') { /* you have entered a comment  */
			incom = 1;
			continue;
		}
		if (c == ' ' || c == '\t' || c == ',' || c == '\n') {
			if (insp == 0)
				*(bufp++) = ' ';
			insp = 1;
			continue;
		}
		if (insp) {
			insp = 0;
			*(wp++) = bufp;
			*(bufp++) = c;
		} else {
			*(bufp++) = c;
		}
	} while (c != '\n');

	bufp2 = buf;
	while (bufp2 < bufp) {
		if (*bufp2 == ' ')
			*bufp2 = '\0';
		bufp2++;
	}
	*wp = NULL;
	return (1);
}


#define	LOCAL_HOST_KV "hostname=%s, ipaddresses=%s"

/*
 * For the named file system return the hosts.local file's contents.
 * If there is no hosts.local file it is not an error. An empty list
 * list will be returned. If a hosts.<fs>.local file exists,
 * the return list will include a string for each host that includes
 * host name followed by a comma separated list of hostnames/IP
 * addresses.
 */
int
get_advanced_network_cfg(
ctx_t *c	/* ARGSUSED */,
char *fs_name,
sqm_lst_t **ret)
{
	FILE	*fp;
	char	*hosts[128];
	char	buf[MAXPATHLEN];
	char	tmp[MAXPATHLEN];
	char	*res;
	int	rval;
	char	err_buf[80];

	if (ISNULL(fs_name, ret)) {
		Trace(TR_ERR, "FS %s: get adv net cfg failed: %d %s",
		    Str(fs_name), samerrno, samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "get adv net cfg for %s", fs_name);

	snprintf(buf, sizeof (buf), "%s/hosts.%s.local", CFG_DIR,
	    fs_name);

	*ret = lst_create();
	if (*ret == NULL) {
		Trace(TR_ERR, "FS %s: get adv net cfg failed:%d %s",
		    fs_name, samerrno, samerrmsg);
		return (-1);
	}

	if ((fp = fopen(buf, "r")) == NULL) {

		/* The absence of the file is not a bug. */
		if (errno == ENOENT) {
			Trace(TR_MISC, "FS %s:no hosts.fs.local",
			    fs_name);
			return (0);
		}
		samerrno = SE_CFG_OPEN_FAILED;
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), buf, err_buf);

		lst_free(*ret);

		Trace(TR_ERR, "FS %s: get adv net cfg failed:%d %s",
		    fs_name, samerrno, samerrmsg);

		return (-1);
	}
	Trace(TR_MISC, "FS %s: adv net cfg opened", fs_name);


	while ((rval = explode_local_host_line(fp, hosts, 128,
	    buf, sizeof (buf))) == 1) {

		int i;


		/*
		 * if both host[0] and hosts[1] are null then we have just
		 * read in a comment line.
		 */
		if (hosts[0] == NULL && hosts[1] == NULL) {
			continue;
		}

		/*
		 * if one of host[0] or hosts[1] are null then the advanced
		 * network configuration is in error.
		 */
		if (hosts[0] == NULL || hosts[1] == NULL) {
			setsamerr(SE_LOCAL_HOST_READ_FAILED);
			(void) fclose(fp);
			lst_free_deep(*ret);
			*ret = NULL;

			Trace(TR_ERR, "FS %s: get adv net cfg failed %s %s",
			    fs_name, Str(hosts[0]), Str(hosts[1]));

			return (-1);
		}

		snprintf(tmp, sizeof (tmp), LOCAL_HOST_KV, hosts[0], hosts[1]);

		for (i = 2; hosts[i] != NULL; i++) {
			strlcat(tmp, " ", (size_t)sizeof (tmp) - strlen(tmp));
			strlcat(tmp, hosts[i], (size_t)sizeof (tmp) -
			    strlen(tmp));
		}
		res = strdup(tmp);
		if (res == NULL) {
			setsamerr(SE_NO_MEM);

			(void) fclose(fp);
			lst_free_deep(*ret);

			Trace(TR_ERR, "FS %s: get adv net cfg failed %d %s",
			    fs_name, samerrno, samerrmsg);
			return (-1);
		}
		if (lst_append(*ret, res) != 0) {
			(void) fclose(fp);
			lst_free_deep(*ret);
			Trace(TR_ERR, "FS %s: get adv net cfg failed %d %s",
			    fs_name, samerrno, samerrmsg);
			return (-1);
		}
	}

	(void) fclose(fp);

	if (rval == -1) {
		setsamerr(SE_LOCAL_HOST_READ_FAILED);
		lst_free_deep(*ret);
		*ret = NULL;

		Trace(TR_ERR, "FS %s: get adv net cfg failed", fs_name);
		return (-1);
	}


	Trace(TR_MISC, "get adv net cfg for %s returning %d entries",
	    fs_name, (*ret)->length);
	return (0);
}



typedef struct h_ip {
	char host[MAXHOSTNAMELEN];
	char ipaddrs[1024];
} h_ip_t;

static parsekv_t local_host_tokens[] = {
	{"hostname",	offsetof(struct h_ip, host),
		parsekv_string_256},
	{"ipaddresses",	offsetof(struct h_ip, ipaddrs),
		parsekv_string_1024},
	{"",		0,			NULL}
};

#define	LOCAL_HOST_HEADER_FMT "# hosts.%s.local\n# Generated %s" \
	"#\n# Host Name\tHost Interfaces\n# --------- \t---------------\n"



/*
 * Set a hosts.fs.local file for the named file system. The entries
 * in the host_strs list will replace the existing file.
 */
int
set_advanced_network_cfg(
ctx_t *c	/* ARGSUSED */,
char *fs_name,
sqm_lst_t *host_strs) {
	FILE *fp = NULL;
	int fd;
	char buf[MAXPATHLEN];
	time_t the_time;
	h_ip_t hip;
	node_t *n;
	int i;
	char err_buf[80];


	if (ISNULL(fs_name, host_strs)) {
		Trace(TR_ERR, "set adv net cfg failed %d %s for %s",
			samerrno, samerrmsg, Str(fs_name));
		return (-1);
	}

	snprintf(buf, sizeof (buf), "%s/hosts.%s.local", CFG_DIR,
		fs_name);


	/* create a backup of the existing file before the change */
	if (backup_cfg(buf) != 0) {
		Trace(TR_ERR, "setting adv net cfg failed: %s", samerrmsg);
		return (-1);
	}

	if ((fd = open(buf, O_WRONLY|O_CREAT|O_TRUNC, 0644)) != -1) {
		fp = fdopen(fd, "w");
	}
	if (fp == NULL) {
		samerrno = SE_CFG_OPEN_FAILED;
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), buf, err_buf);

		Trace(TR_ERR, "FS %s: get adv net cfg failed:%d %s",
		    fs_name, samerrno, samerrmsg);

		return (-1);
	}
	Trace(TR_MISC, "FS %s: Local config file opened for write", fs_name);

	the_time = time(0);
	fprintf(fp, LOCAL_HOST_HEADER_FMT, fs_name, ctime(&the_time));

	for (n = host_strs->head; n != NULL; n = n->next) {

		*hip.host = '\0';
		*hip.ipaddrs = '\0';
		if (parse_kv((char *)n->data, &local_host_tokens[0],
			(void *)&hip) != 0) {

			setsamerr(SE_SET_ADV_CFG_BAD_INPUT);
			fclose(fp);

			Trace(TR_ERR, "setting adv net cfg failed: %s for %s",
			    samerrmsg, Str((char *)n->data));

			return (-1);
		}


		if (*hip.host != '\0' && *hip.ipaddrs != '\0') {
			/*
			 * replace the whitespace in the ip address string with
			 * commas
			 */
			for (i = 0; i < strlen(hip.ipaddrs); i++) {
				if (hip.ipaddrs[i] == ' ' ||
				    hip.ipaddrs[i] == '\t') {
					hip.ipaddrs[i] = ',';
				}

			}
			fprintf(fp, "%s\t%s\n", hip.host, hip.ipaddrs);
		} else {
			setsamerr(SE_SET_ADV_CFG_BAD_INPUT);
			fclose(fp);
			Trace(TR_ERR, "setting adv net cfg failed: %s for %s",
			    samerrmsg, Str((char *)n->data));
			return (-1);
		}
	}
	(void) fclose(fp);

	/* always backup the new cfg but ignore errors. */
	backup_cfg(buf);

	Trace(TR_MISC, "FS %s: set adv net cfg", fs_name);

	return (0);
}


/*
 * This function returns the name of the mds for the named file system
 * If fs_name is null the name of the metadata server for the first
 * shared file system will be returned.
 */
int get_mds_host(
ctx_t *c	/* ARGSUSED */,
char *fs_name,
char **mds_host)
{

	sqm_lst_t *l;
	node_t *n;

	if (ISNULL(mds_host)) {
		Trace(TR_ERR, "get mds host failed:%s", samerrmsg);
		return (-1);
	}

	if (get_all_fs(NULL, &l) == -1) {
		Trace(TR_ERR, "get mds host failed:%s", samerrmsg);
		return (-1);
	}

	for (n = l->head; n != NULL; n = n->next) {
		fs_t *fs = (fs_t *)n->data;
		if (!fs->fi_shared_fs) {
			continue;
		}
		if (fs_name != NULL && *fs_name != '\0' &&
		    strcmp(fs_name, fs->fi_name) != 0) {
			continue;
		}

		/*
		 * since we know the get_all_fs code always sets the
		 * fi_server field we need only return that.
		 */
		*mds_host = (char *)mallocer(sizeof (upath_t));
		if (*mds_host == NULL) {
			Trace(TR_ERR, "get mds host failed:%s", samerrmsg);
			free_list_of_fs(l);
			return (-1);
		}
		strlcpy(*mds_host, fs->fi_server, sizeof (upath_t));
		free_list_of_fs(l);
		Trace(TR_OPRMSG, "got mds host %s", *mds_host);
		return (0);
	}

	*mds_host = NULL;
	free_list_of_fs(l);
	Trace(TR_OPRMSG, "get mds host: no shared fs found");
	return (0);
}
