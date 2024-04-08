/*
 * server.c - listens for client connections on a well defined port.
 *
 * There are 1 listener thread:
 *  . ServerListenerSocket - Listens for new client connections.
 *
 * The port is defined in /etc/services. For file system named sharefs,
 * an example line is:
 *
 * samsock.sharefs1	7105/tcp  # SAM sharefs1 file system socket server
 *
 * There is 1 thread per connected client:
 *  . serverRdSocket - Reads from the shared server socket.
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

#pragma ident "$Revision: 1.64 $"

static char *_SrcFile = __FILE__;
/* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

/* POSIX headers. */
#include <signal.h>
#include <sys/vfs.h>
#include <pthread.h>

/* Solaris headers. */

/* Socket headers. */
#include <arpa/inet.h>

/* SAM-FS headers. */
#include "sam/mount.h"
#include "sam/quota.h"
#include "sam/sam_malloc.h"
#include "sam/spm.h"
#include "sam/fs/share.h"
#include "samhost.h"

/* Local headers. */
#include "samhost.h"

#include "sharefs.h"
#include "server.h"


#define		beq(p1, p2, n)		(bcmp((p1), (p2), (n)) == 0)


/*  Private Functions */
static void *serverRdSocket(void *c);
static int tcpGetAddr(char *name, struct ip_list **ipp);
static void tcpAccept(host_data_t *sp, int svc_fd);
static int createClient(host_data_t *sp, client_list_t *cp, char *hname);
static void putClient(client_list_t *sp);
static void removeClient(client_list_t *sp);
static void shutdownReaders(void);
static int ipSockAddrCmp(void *ipp, int iplen, void *clip, int clilen);
static int samHostsValidateIpAddr(void *cliaddr, int clilen, char *hostname,
				int ipaddrfun(char *, struct ip_list **));


int service_fd = -1;


/*
 * ----- ServerListenerSocket
 *
 * Create a listening socket for the clients mounting this file system.
 */
void *
ServerListenerSocket(void *s)
{
	int	errcode;
	host_data_t *sp = (host_data_t *)s;
	char service_name[SPM_SERVICE_NAME_MAX], errbuf[SPM_ERRSTR_MAX];

	/*
	 * Initialize forward backward chains for client forw/back linked list.
	 */
	SrHost.chain.forw = SrHost.chain.back =
	    (client_list_t *)(void *) &SrHost.chain;
	pthread_mutex_init(&SrHost.server_lock, NULL);

	snprintf(service_name, SPM_SERVICE_NAME_MAX, "sharedfs.%s",
	    sp->fs.fi_name);

	while (!ShutdownDaemon) {
		if ((service_fd =
		    spm_register_service(service_name, &errcode,
		    errbuf)) < 0) {
			Trace(TR_MISC,
			    "FS %s: spm_register_service('%s') failed "
			    "(%d '%s').\n",
			    sp->fs.fi_name, service_name, errcode, errbuf);
		} else {
			tcpAccept(sp, service_fd);
			shutdownReaders();
			(void) spm_unregister_service(service_fd);
			service_fd = -1;
		}
		if (FsCfgOnly) {
			sleep(5);
		} else {
			ShutdownDaemon = TRUE;
		}
	}

	Trace(TR_MISC,
	    "FS %s: ServerListenerSocket thread exiting", sp->fs.fi_name);
	if ((errno = pthread_kill(sp->main_tid, SIGHUP)) != 0) {
		SysError(HERE, "FS %s: pthread_kill(main, SIGHUP) failed",
		    sp->fs.fi_name);
		exit(EXIT_FAILURE);
	}
	return (s);
}


/*
 * ----- tcpGetAddr
 *
 * Return a list of host IP addresses for name.
 */
static int
tcpGetAddr(
	char *name,
	struct ip_list **ipp)
{
	struct ip_list *current;
	struct hostent *hp;
	char **pptr;
	int err;

	*ipp = NULL;
	/*
	 * Get IPv6 addresses for the host...
	 */
	if ((hp = getipnodebyname(name, AF_INET6, 0, &err)) != NULL) {
		for (pptr = hp->h_addr_list; pptr && *pptr; pptr++) {
			current = malloc(sizeof (*current));
			if (current == NULL) {
				errno = 0;
				SysError(HERE, "tcpGetAddr:  Cannot allocate "
				    "memory.");
				exit(EXIT_FAILURE);
			}
			bzero((char *)current, sizeof (*current));
			current->next = *ipp;
			current->addr = malloc(hp->h_length);
			if (current->addr == NULL) {
				errno = 0;
				SysError(HERE, "tcpGetAddr:  Cannot allocate "
				    "memory.");
				exit(EXIT_FAILURE);
			}
			bcopy((char *)*pptr, (char *)current->addr,
			    hp->h_length);
			current->addrlen = hp->h_length;
			*ipp = current;
		}
		freehostent(hp);
	}

	/*
	 * ...and get IPv4 addresses for the host.
	 */
	if ((hp = getipnodebyname(name, AF_INET, 0, &err)) != NULL) {
		for (pptr = hp->h_addr_list; pptr && *pptr; pptr++) {
			current = malloc(sizeof (*current));
			if (current == NULL) {
				errno = 0;
				SysError(HERE, "tcpGetAddr:  Cannot allocate "
				    "memory.");
				exit(EXIT_FAILURE);
			}
			bzero((char *)current, sizeof (*current));
			current->next = *ipp;
			current->addr = malloc(hp->h_length);
			if (current->addr == NULL) {
				errno = 0;
				SysError(HERE, "tcpGetAddr:  Cannot allocate "
				    "memory.");
				exit(EXIT_FAILURE);
			}
			bcopy((char *)*pptr, (char *)current->addr,
			    hp->h_length);
			current->addrlen = hp->h_length;
			*ipp = current;
		}
		freehostent(hp);
	}
	if (*ipp == NULL) {
		errno = 0;
		SysError(HERE, "No Host Address found for '%s'", name);
		return (0);
	}
	return (1);
}


/*
 * ----- tcpAccept
 *
 * Loop waiting on an accept from client hosts.
 */
static void
tcpAccept(
	host_data_t *sp,
	int svc_fd)
{

	/*
	 * Wait for communication from the client(s) (accept).
	 * Start a read socket thread for each unique client.
	 */
	while (!ShutdownDaemon) {
		long cliaddrbuf[64];
		struct sockaddr *cliaddr = (struct sockaddr *)cliaddrbuf;
		client_list_t *cp;
		socklen_t clilen = sizeof (cliaddrbuf);
		int sockfd, hostord, errcode, ret;
		int byterev, srvrcap;
		char errbuf[SPM_ERRSTR_MAX];
		upath_t hname;
		uint64_t client_tags;

		if ((sockfd = spm_accept(svc_fd, &errcode, errbuf)) < 0) {
			/* generally fatal */
			if (!ShutdownDaemon) {
				SysError(HERE, "FS %s:  spm_accept failed "
				    "(%d, '%s')",
				    sp->fs.fi_name, errcode, errbuf);
				if (FsCfgOnly) {
					sleep(2);
				} else {
					ShutdownDaemon = TRUE;
				}
			}
			continue;
		}
		if (getpeername(sockfd, cliaddr, &clilen) < 0) {
			SysError(HERE, "FS %s: getpeername failed: %s",
			    sp->fs.fi_name, strerror(errno));
			(void) close(sockfd);
			continue;
		}

		/*
		 * Validate the client.
		 */
		if ((hostord = samHostsValidateIpAddr(
		    (void *) cliaddr, clilen, hname,
		    tcpGetAddr)) < 0) {
			errno = 0;
			if (inet_ntop(
			    ((struct sockaddr_in *)cliaddr)->sin_family,
			    &((struct sockaddr_in *)cliaddr)->sin_addr,
			    hname, sizeof (hname)) != NULL) {
				SysError(HERE,
				    "FS %s: connect attempt "
				    "from unauthorized host: %s ",
				    sp->fs.fi_name, hname);
			} else {
				SysError(HERE,
				    "FS %s: connect attempt "
				    "from unknown host address",
				    sp->fs.fi_name);
			}
			(void) close(sockfd);
			continue;
		}

		ConfigureSocket(sockfd, sp->fs.fi_name);

		ret = VerifyServerSocket(sockfd, hname, &srvrcap,
		    &byterev,
		    &client_tags);
		if (ret < 0) {
			errno = 0;
			SysError(HERE, "FS %s: Bad client (%s) message "
			    "exchange",
			    Host.fs.fi_name, hname);
			(void) close(sockfd);
			continue;
		}

		VerifyClientCap(hostord, srvrcap, byterev);

		SamMalloc(cp, sizeof (client_list_t));
		memset(cp, 0, sizeof (client_list_t));
		cp->sockfd = sockfd;
		cp->hostord = hostord;
		cp->flags = byterev;
		cp->tags = client_tags;
		if (createClient(sp, cp, hname)) {
			break;
		}
	}
}


/*
 * ----- serverRdSocket
 *
 * This is the serverRdSocket thread. This thread is the LWP that reads
 * file system packets over the socket from the connected client host.
 * There is one serverRdSocket thread per client host (the server itself
 * also counts as a client).
 */
void *
serverRdSocket(void *c)
{
	int flags, cluster, called;
	client_list_t *cp = (client_list_t *)c;


	cluster = IsClusterNodeUp(cp->hname);
	flags = cp->flags ? SOCK_BYTE_SWAP : 0;
	flags |= (cluster >= 0) ? SOCK_CLUSTER_HOST : 0;
	flags |= (cluster == 0) ? SOCK_CLUSTER_HOST_UP : 0;

	called = errno = 0;
	while (!ShutdownDaemon && !cp->killThread) {
		/*
		 * Connect the client TCP connection to the (server) kernel.
		 */
		called = 1;
		errno = SetServerSocket(Host.fs.fi_name, cp->hname,
		    cp->hostord, cp->sockfd, cp->tags, flags);
		if (errno != EINTR) {
			break;
		}
		Trace(TR_MISC, "FS %s: SetServerSocket %s: EINTR",
		    Host.fs.fi_name, cp->hname);
	}

	if (ShutdownDaemon || cp->killThread) {
		if (called) {
			Trace(TR_MISC, "FS %s: %s: SetServerSocket %s "
			    "returned (%d)",
			    Host.fs.fi_name,
			    ShutdownDaemon ? "Shutdown" : "ThrKill",
			    cp->hname, errno);
		} else {
			Trace(TR_MISC, "FS %s: Shutdown prior to "
			    "SetServerSocket call",
			    Host.fs.fi_name);
		}
	} else {
		SysError(HERE, "FS %s: SetServerSocket %s failed",
		    Host.fs.fi_name, cp->hname);
	}

	/*
	 * Disconnect.  Notify the main thread so it can take action.
	 */
	if (!cp->killThread &&
	    (errno = pthread_kill(Host.main_tid, SIGHUP)) != 0) {
		SysError(HERE, "FS %s: pthread_kill(main, SIGHUP) failed",
		    Host.fs.fi_name);
	}
	removeClient(cp);
	return (c);
}


/*
 *	Put client on forw/back client list.
 */
static int
createClient(
	host_data_t *sp,
	client_list_t *cp,
	char *hname)
{
	pthread_attr_t attr;
	int error = 0;

	strcpy(cp->hname, hname);
	putClient(cp);		/* Add this client to the chain */

	/*
	 * Create a socket reader thread per connected client.
	 */
	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	if ((error = pthread_create(&cp->rd_socket, &attr, serverRdSocket,
	    (void *)cp)) != 0) {
		errno = error;
		SysError(HERE,
		    "FS %s: pthread_create[serverRdSocket] failed, client=%s",
		    sp->fs.fi_name, cp->hname);
		removeClient(cp);
	}
	pthread_attr_destroy(&attr);
	return (error);
}


/*
 * CloseReaderSockets()
 *
 * Close reader threads' sockets.  This is called by the main thread;
 * the listener thread attempts separately to signal and reap them.
 * This is a "just in case" where the reader thread can't exit because
 * the socket is still open, can't close in the kernel because it's
 * full, and a worker thread is trying to send something, holding up
 * the works.
 */
void
CloseReaderSockets(void)
{
	int fd;
	client_list_t *lp;

	if ((errno = pthread_mutex_lock(&SrHost.server_lock))) {
		LibFatal(pthread_mutex_lock, "");
	}
	for (lp = (client_list_t *)SrHost.chain.forw;
	    lp != (client_list_t *)(void *)&SrHost.chain;
	    lp = lp->chain.forw) {
		lp->killThread = 1;
		if ((fd = lp->sockfd) != -1) {
			lp->sockfd = -1;
			if (close(fd) < 0) {
				SysError(HERE, "FS %s: Host %s/%d: close(%d) "
				    "failed",
				    Host.fs.fi_name, lp->hname, lp->hostord,
				    fd);
			}
		}
	}
	pthread_mutex_unlock(&SrHost.server_lock);
}


/*
 *	Put client on forw/back client list.
 */
static void
putClient(client_list_t *cp)
{
	client_list_t *lp;
	void *statusp;

	if ((errno = pthread_mutex_lock(&SrHost.server_lock))) {
		LibFatal(pthread_mutex_lock, "");
	}

	/*
	 * Check for host already in client list.
	 */
	lp = (client_list_t *)SrHost.chain.forw;
	while (lp != (client_list_t *)(void *)&SrHost.chain) {
		if (strcasecmp(lp->hname, cp->hname) == 0) {
			int fd;

			/*
			 * Previous connect for this client, close the
			 * connection and kill the thread.
			 */
			lp->killThread = 1;
			if ((fd = lp->sockfd) != -1) {
				lp->sockfd = -1;
				(void) close(fd);
			}
			pthread_mutex_unlock(&SrHost.server_lock);
			if ((errno = pthread_kill(lp->rd_socket, SIGEMT))) {
				SysError(HERE, "pthread_kill[serverRdSocket] "
				    "failed");
			}
			if ((errno = pthread_join(lp->rd_socket, &statusp))) {
				SysError(HERE, "pthread_join[serverRdSocket] "
				    "failed");
			}
			if ((errno =
			    pthread_mutex_lock(&SrHost.server_lock))) {
				LibFatal(pthread_mutex_lock, "");
			}
			break;
		}
		lp = lp->chain.forw;
	}
	SrHost.chain.forw->chain.back = cp;
	cp->chain.back = (client_list_t *)(void *)&SrHost.chain;
	cp->chain.forw = SrHost.chain.forw;
	SrHost.chain.forw = cp;
	NumofClients++;
	pthread_mutex_unlock(&SrHost.server_lock);
}


/*
 *	Remove client from forw/back client list.
 */
static void
removeClient(client_list_t *cp)
{
	int fd;

	if ((errno = pthread_mutex_lock(&SrHost.server_lock))) {
		LibFatal(pthread_mutex_lock, "");
	}
	if ((fd = cp->sockfd) != -1) {
		cp->sockfd = -1;
		(void) close(fd);
	}
	cp->chain.back->chain.forw = cp->chain.forw;
	cp->chain.forw->chain.back = cp->chain.back;
	cp->chain.forw = cp;
	cp->chain.back = cp;
	if (--NumofClients < 0)  {
		NumofClients = 0;
		SrHost.chain.forw = SrHost.chain.back =
		    (client_list_t *)(void *) &SrHost.chain;
	}
	pthread_mutex_unlock(&SrHost.server_lock);
	SamFree(cp);
}


/*
 *	Shutdown server reader threads (serverRdSocket) on the client list.
 *  Send a EMT signal.
 */
static void
shutdownReaders(void)
{
	client_list_t *cp;
	void *statusp;

	if ((errno = pthread_mutex_lock(&SrHost.server_lock))) {
		LibFatal(pthread_mutex_lock, "");
	}

	/*
	 * Check for host already in client list.
	 */
	cp = (client_list_t *)SrHost.chain.forw;
	while (cp != (client_list_t *)(void *) &SrHost.chain) {
		pthread_mutex_unlock(&SrHost.server_lock);
		if ((errno = pthread_kill(cp->rd_socket, SIGEMT))) {
			SysError(HERE, "pthread_kill[serverRdSocket] failed");
		}
		if ((errno = pthread_join(cp->rd_socket, &statusp))) {
			SysError(HERE, "pthread_join[serverRdSocket] failed");
		}
		if ((errno = pthread_mutex_lock(&SrHost.server_lock))) {
			LibFatal(pthread_mutex_lock, "");
		}
		cp = (client_list_t *)SrHost.chain.forw;
	}
	pthread_mutex_unlock(&SrHost.server_lock);
}


/*
 * ipSockAddrCmp -- Highly non-portable IP address comparison
 *
 * Compares the socket address data returned by a getpeername() call
 * on the accept()'ed socket, and compares it with an address obtained
 * by calling getipnodebyname() on an entry from the hosts file.
 *
 * ipp/iplen are the actual address and size of the candidate IP
 * address, and clip/clilen the (sockaddr_in *) or (sockaddr_in6 *)
 * of the connecting socket and its length.
 */
static int
ipSockAddrCmp(void *ipp, int iplen, void *clip, int clilen)
{
	struct sockaddr_in  *s4ip = (struct sockaddr_in  *)clip;
	struct sockaddr_in6 *s6ip = (struct sockaddr_in6 *)clip;
	in_addr_t x;

	if (iplen == sizeof (in_addr_t)) {
		/*
		 * ipp points to IPv4 address.
		 */
		switch (s4ip->sin_family) {
		case AF_INET:
			/*
			 * clip also points to socket w/ IPv4 INET connection
			 */
			return (beq(&s4ip->sin_addr, ipp, iplen));

		case AF_INET6:
			/*
			 * clip points to socket w/ IPv6 INET connection.
			 * If it's not a V4 mapped address, reject it,
			 * otherwise compare it to ipp.
			 */
			if (!IN6_IS_ADDR_V4MAPPED(&s6ip->sin6_addr)) {
				return (0);
			}
			IN6_V4MAPPED_TO_IPADDR(&s6ip->sin6_addr, x);
			return (beq((char *)&x, ipp, iplen));
		}
	}
	if (iplen == sizeof (in6_addr_t)) {
		/*
		 * ipp points to IPv6 address.
		 */
		switch (s6ip->sin6_family) {
		case AF_INET:
			/*
			 * clip points to a socket w/ IPv4 connection.
			 * Convert ipp to a IPv4 address (reject if we
			 * can't) and compare.
			 */
			if (!IN6_IS_ADDR_V4MAPPED((struct in6_addr *)ipp)) {
				return (0);
			}
			IN6_V4MAPPED_TO_IPADDR((struct in6_addr *)ipp, x);
			return (beq(&s4ip->sin_addr, &x, sizeof (in_addr_t)));

		case AF_INET6:
			return (beq(&s6ip->sin6_addr, ipp, iplen));
		}
	}
	errno = 0;
	SysError(HERE,
	    "FS %s: sam-sharefsd/ipSockAddrCmp:  unknown addrlen (%d/%d)",
	    Host.fs.fi_name, iplen, clilen);
	return (0);
}


/*
 * Determine if the socket address belongs to a host we
 * should know about.  If not, return -1.  If so, return
 * its ordinal (line #) from the hosts file and copy out
 * the host name.
 *
 * Get access to the hosts table and build a cached copy
 * of all the host addresses we've looked at, so that we
 * don't do this repeatedly.
 *
 */

static struct cache_ent *cacheAddrs = NULL;

static int
samHostsValidateIpAddr(
	void *cliaddr,
	int clilen,
	char *hostname,
	int (*ipaddrfun)(char *, struct ip_list **))
{
	struct sam_host_table_blk *htab;
	struct sam_host_table *ht;
	char **IPaddrs;
	int i, j;

	htab = (struct sam_host_table_blk *)malloc(SAM_LARGE_HOSTS_TABLE_SIZE);
	ht = (struct sam_host_table *)htab;

	if (GetHostTab(htab) < 0) {
		errno = 0;
		free(htab);
		SysError(HERE, "samHostsValidateIpAddr: GetExpHostTab failed");
		return (-1);
	}

	if (cacheAddrs == NULL) {
		cacheAddrs = (struct cache_ent *)
		    malloc(ht->count * sizeof (*cacheAddrs));
		if (cacheAddrs == NULL) {
			errno = 0;
			free(htab);
			SysError(HERE, "cannot alloc space for IP addr "
			    "cache\n");
			return (-1);
		}
		bzero((char *)cacheAddrs, ht->count *
		    sizeof (struct cache_ent));
	}

	for (i = 0; i < ht->count; i++) {
		/*
		 * Compute and set up cacheAddrs[i] if it isn't already present
		 */
		if (cacheAddrs[i].host[0] == '\0') {
			struct ip_list *first = NULL, *last = NULL, *next;
			upath_t srv, addr;

			if (!GetSharedHostInfo(ht, i, srv, addr)) {
				errno = 0;
				free(htab);
				SysError(HERE, "FS %s: get host info for "
				    "host %d failed\n",
				    Host.fs.fi_name, i);
				return (-1);
			}
			strncpy((char *)&cacheAddrs[i].host, srv,
			    sizeof (cacheAddrs[i].host));
			IPaddrs = DeCommaStr(addr, sizeof (addr));
			for (j = 0; IPaddrs[j]; j++) {
				if ((*ipaddrfun)(IPaddrs[j], &next)) {
					if (first == NULL) {
						first = next;
					}
					if (last == NULL) {
						last = next;
					} else {
						if (last->next != NULL) {
							errno = 0;
							SysError(HERE,
							    "List "
							    "programming "
							    "error! "
							    "first=%p, "
							    "last=%p, "
							    "next=%p",
							    (void *)first,
							    (void *)last,
							    (void *)next);
							exit(EXIT_FAILURE);
						}
						last->next = next;
					}
					while (last->next != NULL) {
						last = last->next;
					}
				}
			}
			cacheAddrs[i].addr = first;
			DeCommaStrFree(IPaddrs);
		}
		/*
		 * cacheAddrs[i] is present, see if cliaddr matches
		 */
		if (cacheAddrs[i].addr) {
			struct ip_list *ipp;

			for (ipp = cacheAddrs[i].addr; ipp; ipp = ipp->next) {
				if (ipSockAddrCmp(ipp->addr, ipp->addrlen,
				    cliaddr, clilen)) {
					char addrstr[128];

					strncpy(hostname, cacheAddrs[i].host,
					    sizeof (cacheAddrs[i].host));
					switch (((struct sockaddr_in *)
					    cliaddr)->sin_family) {
					case AF_INET:
						if (inet_ntop(
						    ((struct sockaddr_in *)
						    cliaddr)->sin_family,
						    &((struct sockaddr_in *)
						    cliaddr)->sin_addr,
						    addrstr,
						    sizeof (addrstr)) != NULL) {
							Trace(TR_MISC, "FS %s:"
							    "Connect from "
							    "%s, %s",
							    Host.fs.fi_name,
							    hostname,
							    addrstr);
							free(htab);
							return (i);
						}
						break;
					case AF_INET6:
						if (inet_ntop(
						    ((struct sockaddr_in6 *)
						    cliaddr)->sin6_family,
						    &((struct sockaddr_in6 *)
						    cliaddr)->sin6_addr,
						    addrstr,
						    sizeof (addrstr)) != NULL) {
							Trace(TR_MISC, "FS %s:"
							    "Connect from %s"
							    ", %s",
							    Host.fs.fi_name,
							    hostname,
							    addrstr);
							free(htab);
							return (i);
						}
						break;
					default:
						errno = 0;
						SysError(HERE,
						    "FS %s: Connect from "
						    "unknown family type",
						    Host.fs.fi_name);
					}
					errno = 0;
					free(htab);
					SysError(HERE,
					    "FS %s: Connect from host %s, "
					    "address unprintable",
					    Host.fs.fi_name, hostname);
					return (i);
				}
			}
		}
	}
	free(htab);
	return (-1);
}
