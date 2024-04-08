/*
 * client.c - communicates with filesystem and the shared server host.
 *
 * There is 1 thread:
 *  . ClientRdSocket - Reads from the shared server socket.
 *
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

#pragma ident "$Revision: 1.60 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

/* POSIX headers. */
#include <signal.h>
#include <sys/vfs.h>

/* Solaris headers. */

/* Socket headers. */
#include <arpa/inet.h>
#include <netinet/in.h>

/* SAM-FS headers. */
#include "sam/mount.h"
#include "sam/quota.h"
#include "sam/syscall.h"
#include "sam/sam_trace.h"
#include "sam/exit.h"
#include "sblk.h"
#include "share.h"
#include "samhost.h"
#include "sam/spm.h"

/* Local headers. */
#include "sharefs.h"
#include "config.h"

static char **getLocalAddrs(char *fs, char *server);
static int explodeLine(FILE *fp, char **str, int nstr, char *buf, int buflen);


/*  Private definitions */
#define	ERR_NOADDR	(-2)		/* no address found for the hostname */
#define	ERR_ADDR	(-1)		/* address found for the hostname */

/*
 * The following structure contains a copy of the kernel's
 * info about the FS, plus space for two copies of the
 * on-disk info about the FS's server configuration.
 * We always keep one copy around, and read info into the
 * other for comparison purpposes.
 */
struct shfs_config config;


/*
 * The following is a definition to support binding of the
 * local connection endpoint when connecting to the server.
 * This is useful for things like virtual IP links, where
 * we want both ends of the IP connection bound to a logical
 * host address.  (Failing to bind the local end works fine
 * in most situations, but by default seems to bind the
 * local end to the physical IP address of the local
 * interface chosen.)
 */
struct addrCnxn {
	char *dst;		/* destination IP address (NULL ==> EOL) */
	char *src;		/* local IP address (optional) */
};

/*  Private Functions */
static struct addrCnxn *extractLocalAddrs(char *fs, char *server,
					char **addrs);
static void addrCnxnFree(struct addrCnxn *addr);


/*
 * ----- ClientRdSocket
 *
 * This is the ClientRdSocket thread. This thread is the LWP who reads
 * packets over the socket in the file system from the server host.
 * There is one ClientRdSocket thread.
 * sam_san_max_message_t is the maximum size of a socket message.
 * This user buffer is used in the filesystem for the VOP_READ()
 * because UIO_SYSSPACE is not supported in VOP_READ().
 */
void *
ClientRdSocket(void *c)
{
	host_data_t *cp = (host_data_t *)c;
	int error = 0;
	int errcode;
	char service[SPM_SERVICE_NAME_MAX], errbuf[SPM_ERRSTR_MAX];

	snprintf(service, SPM_SERVICE_NAME_MAX, "sharedfs.%s", cp->fs.fi_name);

	sleep(2);		/* give listener a chance to get set up */

	/*
	 * Create socket connection to the shared server host.
	 */
	cp->sockfd = -1;
	while (!ShutdownDaemon) {
		char **caddrs;
		struct addrCnxn *addrs;
		int i, flags, byteswap, cluster, naddrs = 0;

		if (cp->sockfd != -1) {
			(void) close(cp->sockfd);
			cp->sockfd = -1;
		}
		if ((caddrs = DeCommaStr(cp->serveraddr,
		    sizeof (cp->serveraddr)))
		    == NULL) {
			SysError(HERE, "FS %s: Bad configuration host "
			    "list (%s)",
			    cp->fs.fi_name, cp->serveraddr);
			if (!FsCfgOnly) {
				ShutdownDaemon = TRUE;
				exit(EXIT_NORESTART);
			}
			sleep(10);
			continue;
		}
		addrs = extractLocalAddrs(cp->fs.fi_name, cp->server, caddrs);
		if (addrs == NULL) {
			SysError(HERE, "FS %s: Bad FS and/or local "
			    "configuration (%s)",
			    cp->fs.fi_name, cp->serveraddr);
			if (!FsCfgOnly) {
				ShutdownDaemon = TRUE;
				exit(EXIT_NORESTART);
			}
			DeCommaStrFree(caddrs);
			sleep(10);
			continue;
		}

		if (addrs[0].dst == NULL) {
			Trace(TR_MISC, "FS %s: Client addrs='%s'",
			    cp->fs.fi_name, cp->serveraddr);
			SysError(HERE, "FS %s: no addresses configured for "
			    "server %s; "
			    "check %s/hosts.%s.local",
			    cp->fs.fi_name, cp->server,
			    SAM_CONFIG_PATH, cp->fs.fi_name);
			if (!FsCfgOnly) {
				ShutdownDaemon = TRUE;
				exit(EXIT_NORESTART);
			}
			DeCommaStrFree(caddrs);
			addrCnxnFree(addrs);
			sleep(10);
			continue;
		}

		for (i = 0; addrs[i].dst && !ShutdownDaemon; i++) {
			cp->sockfd = spm_connect_to_service(service,
			    addrs[i].dst, addrs[i].src, &errcode, errbuf);
			if (cp->sockfd >= 0) {
				break;
			}
			Trace(TR_MISC, "FS %s: spm_connect to %s/%s%s%s "
			    "failed: (%d, '%s')",
			    cp->fs.fi_name, cp->server, addrs[i].dst,
			    addrs[i].src ? "@" : "",
			    addrs[i].src ? addrs[i].src : "",
			    errcode, errbuf);

			if (errcode != ESPM_AI && errcode != ESPM_BIND) {
				naddrs++;
			}
		}
		DeCommaStrFree(caddrs);
		addrCnxnFree(addrs);

		if (ShutdownDaemon) {
			break;
		}

		if (cp->sockfd < 0) {
			errno = 0;
			if (!FsCfgOnly && naddrs == 0) {
				ShutdownDaemon = TRUE;
				SysError(HERE, "FS %s: spm_connect failed; "
				    "check host addresses", cp->fs.fi_name);
				exit(EXIT_NORESTART);
			}
			if (!FsCfgOnly) {
				ShutdownDaemon = TRUE;
				if (naddrs == 0) {
					SysError(HERE, "FS %s: spm_connect "
					    "failed; check host addresses",
					    cp->fs.fi_name);
					exit(EXIT_NORESTART);
				} else {
					SysError(HERE, "FS %s: cannot "
					    "connect to server %s",
					    cp->fs.fi_name, cp->server);
					exit(EXIT_FAILURE);
				}
			}
			sleep(5);
			continue;
		}

		if (ShutdownDaemon) {
			break;
		}

		/*
		 * Configure socket's TCP/IP options.
		 * Tell the kernel we're a client.
		 * Then hand the kernel the read socket from the server;
		 * the kernel will use this thread to wait on it.  We
		 * stay in SetClientSocket until a server reconfiguration
		 * (e.g., metadata server switch) or other major event.
		 */
		ConfigureSocket(cp->sockfd, cp->fs.fi_name);

		byteswap = VerifyClientSocket(cp->sockfd, &cp->server_tags);
		if (byteswap < 0) {
			SysError(HERE,
			    "FS %s: Unsuccessful server (%s) message "
			    "exchange (%d)",
			    cp->fs.fi_name, cp->server, byteswap);
			(void) close(cp->sockfd);
			cp->sockfd = -1;
			if (!FsCfgOnly) {
				ShutdownDaemon = TRUE;
			} else {
				sleep(5);
			}
			continue;
		}

		error = SetClient(cp->fs.fi_name, cp->server, cp->serverord,
		    cp->server_tags, ServerHost);
		if (error) {
			SysError(HERE, "FS %s: syscall[SC_set_client] failed",
			    cp->fs.fi_name);
		}

		cluster = IsClusterNodeUp(cp->hname);
		flags = byteswap ? SOCK_BYTE_SWAP : 0;
		flags |= (cluster >= 0) ? SOCK_CLUSTER_HOST : 0;
		flags |= (cluster == 0) ? SOCK_CLUSTER_HOST_UP : 0;

		Conn2Srvr = TRUE;
		error = SetClientSocket(cp->fs.fi_name, cp->hname,
		    cp->sockfd, flags);
		Conn2Srvr = FALSE;

		if (!ShutdownDaemon) {
			Trace(TR_MISC, "FS %s: syscall[SC_client_rdsock] "
			    "returned %d",
			    cp->fs.fi_name, error);
			if (FsCfgOnly) {
				sleep(10);
			} else {
				/*
				 * Disconnect.  Notify the main thread + return
				 */
				ShutdownDaemon = TRUE;
			}
		} else {
			Trace(TR_MISC, "FS %s: Shutting down: ClientRdSocket "
			    "returned (%d)",
			    cp->fs.fi_name, error);
		}
	}
	/*
	 * Notify the main thread that we're going away.
	 */
	if ((errno = pthread_kill(cp->main_tid, SIGHUP)) != 0) {
		SysError(HERE, "FS %s: pthread_kill(main, SIGHUP) failed",
		    cp->fs.fi_name);
	}
	if (cp->sockfd >= 0) {
		(void) close(cp->sockfd);
		cp->sockfd = -1;
	}
	return (c);
}


/*
 * ----- extractLocalAddrs
 *
 * Extract Local Addresses
 *
 * Takes the FS name and server name, and extracts the associated
 * line from /etc/opt/SUNWsamfs/hosts.<fsname>.local, if any.
 * If there is no such line, then a copy of the original list is
 * returned.  If there is such a line, then the input list and
 * the list of host names in the local file are searched, and
 * a list consisting of only names in both lists is returned.
 *
 * W/ regard to ordering, the hosts.<fs>.local has priority.
 */
static struct addrCnxn *
extractLocalAddrs(
	char *fs,
	char *server,
	char **gaddrs)
{
	char **laddrs;
	struct addrCnxn *raddrs;
	int l, g, r;
	int i, j;

	for (g = 0; gaddrs[g] != NULL; g++)
		;

	laddrs = getLocalAddrs(fs, server);
	errno = 0;
	if (laddrs == NULL) {
		/*
		 * No local addresses specified.
		 * Return the shared (global) table entries.
		 */
		raddrs = malloc((g+1) * sizeof (struct addrCnxn));
		if (raddrs == NULL) {
			ShutdownDaemon = TRUE;
			SysError(HERE, "FS %s: can't malloc addrCnxn memory",
			    fs);
			exit(EXIT_FAILURE);
		}
		for (r = 0; gaddrs[r] != NULL; r++) {
			raddrs[r].src = NULL;
			raddrs[r].dst = strdup(gaddrs[r]);
			if (raddrs[r].dst == NULL) {
				ShutdownDaemon = TRUE;
				SysError(HERE, "FS %s: can't malloc string "
				    "memory", fs);
				exit(EXIT_FAILURE);
			}
		}
		raddrs[r].src = NULL;
		raddrs[r].dst = NULL;
		return (raddrs);
	}
	/*
	 * Local addresses specified.
	 * Reconcile the local and shared (global) lists.
	 */
	for (l = 0; laddrs[l] != NULL; l++)
		;
	r = l;
	raddrs = malloc((r+1) * sizeof (struct addrCnxn));
	if (raddrs == NULL) {
		ShutdownDaemon = TRUE;
		SysError(HERE, "FS %s: can't malloc addrCnxn memory", fs);
		exit(EXIT_FAILURE);
	}
	/*
	 * Check each entry on the local list, and see if its
	 * destination address exists in the global list.  If so,
	 * put it in the result list, and put in the source address
	 * if one is specified.  E.g., ash-ge.foo.com@elm-ge.foo.com
	 * in the local addresses file specifies a destination of
	 * ash-ge.foo.com, and a source of elm-ge.foo.com, and
	 * we allow this if ash-ge.foo.com is present in the
	 * shared hosts file.
	 */
	for (i = r = 0; i < l; i++) {
		int globent, globlen;

		globent = -1;
		for (j = 0; j < g; j++) {
			globlen = strlen(gaddrs[j]);
			if (strncasecmp(laddrs[i], gaddrs[j], globlen) == 0 &&
			    (laddrs[i][globlen] == '\0' ||
			    laddrs[i][globlen] == LOCAL_IPADDR_SEP)) {
				globent = j;
				break;
			}
		}
		if (globent >= 0) {
			raddrs[r].dst = strdup(gaddrs[globent]);
			if (raddrs[r].dst == NULL) {
				ShutdownDaemon = TRUE;
				SysError(HERE, "FS %s: can't malloc string "
				    "memory",
				    fs);
				exit(EXIT_FAILURE);
			}
			if (laddrs[i][globlen] == LOCAL_IPADDR_SEP) {
				/*
				 * serverIPinterface@localIPinterface found
				 * in hosts.<fs>.local
				 */
				raddrs[r].src = strdup(&laddrs[i][globlen+1]);
				if (raddrs[r].src == NULL) {
					ShutdownDaemon = TRUE;
					SysError(HERE, "FS %s: can't malloc "
					    "string memory", fs);
					exit(EXIT_FAILURE);
				}
			} else {
				raddrs[r].src = NULL;
			}
			r++;
		}
	}
	raddrs[r].dst = NULL;
	raddrs[r].src = NULL;
	DeCommaStrFree(laddrs);
	return (raddrs);
}


/*
 * ----- dupList
 *
 * Return a copy of a NULL-terminated list of strings.
 */
static char **
dupList(char **p)
{
	int l, i;
	char **r;

	errno = 0;
	for (l = 0; p[l] != NULL; l++)
		;
	r = malloc((l+1) * sizeof (char *));
	if (r == NULL) {
		SysError(HERE, "FS %s: can't malloc stringptr memory",
		    Host.fs.fi_name);
		return (NULL);
	}
	for (i = 0; i < l; i++) {
		r[i] = strdup(p[i]);
		if (r[i] == NULL) {
			SysError(HERE,
			    "FS %s: can't malloc string memory",
			    Host.fs.fi_name);
			return (NULL);
		}
	}
	r[i] = NULL;
	return (r);
}


/*
 * ----- getLocalAddrs
 *
 * Open up the hosts.<fs>.local file, and extract all the
 * aliases for the particular server host.
 */
static char **
getLocalAddrs(
	char *fs,
	char *server)
{
	FILE *fp;
	char **l = NULL;
	char *hosts[128];
	char buf[MAXPATHLEN];

	snprintf(buf, sizeof (buf), "%s/hosts.%s.local", SAM_CONFIG_PATH, fs);
	if ((fp = fopen(buf, "r")) == NULL) {
		return (NULL);
	}
	Trace(TR_MISC, "FS %s: Local config file opened", fs);
	for (;;) {
		if (!explodeLine(fp, hosts, 128, buf, sizeof (buf)))
			break;
		if (hosts[0] && strcasecmp(hosts[0], server) == 0) {
			l = dupList(hosts+1);
		}
	}
	(void) fclose(fp);
	return (l);
}


/*
 * ----- explodeLine
 *
 * Break a line up into an array of strings.
 * Throw out anything after a '#'.
 */
static int
explodeLine(FILE *fp, char **strs, int nstr, char *buf, int buflen)
{
	int c, insp, incom;
	char *bufp, *bufp2, **wp;

	insp = 1;
	incom = 0;
	wp = strs;
	bufp = buf;
	do {
		c = fgetc(fp);
		errno = 0;
		if (c == EOF)
			return (0);
		if (bufp >= &buf[buflen]) {
			SysError(HERE, "explodeLine: hosts.%s.local line "
			    "buffer overflow",
			    Host.fs.fi_name);
			return (0);
		}
		if (wp >= &strs[nstr]) {
			SysError(HERE, "explodeLine: hosts.%s.local word "
			    "buffer overflow",
			    Host.fs.fi_name);
			return (0);
		}
		if (incom)
			continue;
		if (c == '#') {
			incom = 1;
			continue;
		}
		if (c == ' ' || c == '\t' || c == ',' || c == '\n') {
			if (insp == 0)
				*bufp++ = ' ';
			insp = 1;
			continue;
		}
		if (insp) {
			insp = 0;
			*wp++ = bufp;
			*bufp++ = c;
		} else {
			*bufp++ = c;
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


/*
 * ----- DeCommaStr
 *
 * Convert a comma-separated string into
 * a NULL-terminated array of strings.
 */
char **
DeCommaStr(char *str, int len)
{
	int i, j, ncommas = 0;
	char *s = str;
	char *s2, **r;

	for (s = str; (s - str) < len && *s != '\0'; s++) {
		if (*s == ',')
			ncommas++;
	}

	r = malloc((ncommas+2) * sizeof (char *));
	if (r == NULL) {
		SysError(HERE, "FS %s: DeCommaStr() -- no memory",
		    Host.fs.fi_name);
		exit(EXIT_FAILURE);
	}

	s = str;
	for (i = 0; i <= ncommas; i++) {
		j = 0;
		/*
		 * would use strnspn() for this next loop, but it doesn't exist
		 */
		for (s2 = s; (s2 - str) < len && *s2 != '\0' && *s2 != ',';
		    s2++) {
			j++;
		}
		r[i] = malloc(j+1);
		if (r[i] == NULL) {
			SysError(HERE, "FS %s: DeCommaStr -- no memory",
			    Host.fs.fi_name);
			exit(EXIT_FAILURE);
		}
		strncpy(r[i], s, j);
		r[i][j] = 0;
		s = s2 + 1;
	}
	r[i] = NULL;
	return (r);
}


/*
 * ----- DeCommaStrFree
 *
 * Free a list of strings returned by DeCommaStr
 */
void
DeCommaStrFree(char **strlist)
{
	int i;

	if (strlist != NULL) {
		for (i = 0; strlist[i] != NULL; i++) {
			free(strlist[i]);
		}
		free(strlist);
	}
}


/*
 * -----addrCnxnFree
 *
 * Free a list of connection struct pointers
 * and the strings it points to.
 */
static void
addrCnxnFree(struct addrCnxn *addr)
{
	int i;

	if (addr != NULL) {
		for (i = 0; addr[i].dst != NULL; i++) {
			free(addr[i].dst);
			if (addr[i].src != NULL) {
				free(addr[i].src);
			}
		}
		free(addr);
	}
}


/*
 * ----- GetServerInfo
 *
 * int GetServerInfo(char *fs)
 *		Reads the .hosts file and/or the label block of the given
 *		filesystem and returns the name of the server, and its list
 *		of IP names/addresses.  Sets appropriate information into
 *		the Hosts.* structure, including hostname, server name, and
 *		server ordinal.
 *
 * Return:
 *		CFG_FATAL (-2) on error that shouldn't allow restart
 *		CFG_ERROR (-1) on error,
 *		CFG_CLIENT (0) for client-only,
 *		CFG_HOST   (1) for potential server
 * plus the server's name and IP address(es) if obtained.
 */
int
GetServerInfo(char *fs)
{
	struct cfdata *cfp;

	cfp = &config.cp[config.curTab];
	if (cfp->flags & (R_ERRBITS|R_BUSYBITS)) {
		return (CFG_ERROR);
	}

	if ((cfp->flags & R_SBLK_BITS) != 0) {
		struct sam_host_table *ht = &cfp->ht->info.ht;

		if ((cfp->flags & R_SBLK_BITS) != R_SBLK) {
			return (CFG_ERROR);
		}
		if (ht->server == HOSTS_NOSRV) {
			errno = 0;
			SysError(HERE, "FS %s: hosts table declares no "
			    "server", fs);
			return (CFG_ERROR);
		}
		Host.serverord = ht->server;
		strncpy(Host.server, cfp->serverName,
		    sizeof (cfp->serverName));
		strncpy(Host.serveraddr, cfp->serverAddr,
		    sizeof (cfp->serverAddr));
		if (strncasecmp(cfp->serverName,
		    Host.hname, sizeof (cfp->serverName)) != 0) {
			if (cfp->flags & R_LBLK_VERS) {
				/*
				 * not server + bad label
				 */
				return (CFG_ERROR);
			}
			return (CFG_HOST);
		}
		return (CFG_SERVER);
	} else if ((cfp->flags & R_LBLK_BITS) == R_LBLK) {
		/*
		 * No access to the hosts file/root slice; use the label block
		 */
		struct sam_label *lblk = &cfp->lb.info.lb;

		if (cfp->flags & R_LBLK_VERS) {
			/* bad label version; wait for good one */
			return (CFG_ERROR);
		}
		Host.serverord = lblk->serverord;
		strncpy(Host.server, &lblk->server[0], sizeof (lblk->server));
		strncpy(Host.serveraddr, &lblk->serveraddr[0],
		    sizeof (lblk->serveraddr));
		return (CFG_CLIENT);
	}
	Trace(TR_MISC, "FS %s: Label not OK (SERVER)? flags = %x",
	    fs, cfp->flags);
	return (CFG_ERROR);
}
