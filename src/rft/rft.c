
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

#pragma ident "$Revision: 1.21 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <nss_dbdefs.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/device.h"
#include "aml/shm.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"
#include "sam/sam_trace.h"
#include "sam/sam_malloc.h"
#include "sam/names.h"
#include "sam/exit.h"
#include "sam/lib.h"
#include "aml/sam_rft.h"
#include "aml/diskvols.h"
#include "sam/readcfg.h"
#include "aml/shm.h"
#include "sam/spm.h"

/* Local headers. */
#include "rft_defs.h"
#include "log.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/*
 * Message catalog descriptor.
 */
nl_catd catfd = NULL;
struct CatalogMap *Catalogs = NULL;

/*
 * Program name.
 */
char *program_name;

/*
 * Set TRUE if started from init.
 */
boolean_t Daemon = TRUE;

/*
 * Path to rft's work directory.
 */
char *WorkDir = NULL;

/*
 * Shutdown rft daemon.
 */
int Shutdown = 0;

/*
 * SAM-FS shared memory segment.
 */
shm_alloc_t		master_shm, preview_shm;
shm_ptr_tbl_t	*shm_ptr_tbl;

/* Private data. */
static upath_t fullpath;	/* temporary space for building names */
static struct DiskVolumes *diskVolumes = NULL;

/*
 * This structure contains all trusted client's host names and
 * IP addresses for validating incoming connections.
 */
typedef char host_buffer_t[NSS_BUFLEN_HOSTS];
typedef struct hostent host_result_t;

static struct {
	int	count;				/* number of clients */
	int	free;				/* free entry for clients */
	host_t	*host;
	struct hostent	*result;
	host_buffer_t	*buffer;
} trustedClients = { 0, 0, NULL, NULL, NULL };

static char dirName[TOKEN_SIZE];
static char token[TOKEN_SIZE];
static boolean_t inMediaSection = B_FALSE;
static struct sigaction sig_action;
static time_t cmdFileTime;
static char cmdFileName[] = SAM_CONFIG_PATH"/"COMMAND_FILE_NAME;

/* Private functions. */
static int waitForClient(int sockfd, struct sockaddr *client_addr);
static void startClientServer(int client_fd, struct sockaddr *client_addr);
static void clientServer(void * arg);
static void setSignalHandling();
static void catchSignals(int signum);
static boolean_t validateClientAddr(struct sockaddr *client_addr);
static void getTrustedClients();
static void getIpNodeByName(char *host, struct hostent **result);
static void addTrustedClient(char *host, char *config_file);
static void allocTrustedClients();
static void deallocTrustedClients();
static void readCmdFile();
static void reconfig();

int
main(
	/* LINTED argument unused in function */
	int argc,
	char *argv[])
{
	int service_fd;
	int client_fd;
	struct sockaddr_in6 client_addr;
	char errbuf[SPM_ERRSTR_MAX];
	int errcode;

	program_name = argv[0];
	CustmsgInit(1, NULL);

	(void) sprintf(fullpath, "%s/%s", SAM_VARIABLE_PATH, SAMRFT_DIRNAME);
	SamStrdup(WorkDir, fullpath);

	/*
	 * Change directory.  This keeps the core file separate.
	 */
	if (chdir(WorkDir) == -1) {
		MakeDir(WorkDir);
		if (chdir(WorkDir) == -1) {
			exit(EXIT_FATAL);
		}
	}

	/*
	 * Set process' signal handling.
	 */
	setSignalHandling();

	/*
	 * Read command file.
	 */
#if 0
	if (argv[1] != NULL && argv[1] != '\0') {
		Daemon = FALSE;
		cmd_filename = argv[1];
	}
#endif
	readCmdFile();

#if 0
	if (Daemon == FALSE) {
		return (EXIT_SUCCESS);
	}
#endif

	/*
	 * Prepare tracing.
	 */
	TraceInit("rft", TI_rft);
	Trace(TR_MISC, "Rft daemon started");

	service_fd = spm_register_service(SAMRFT_SPM_SERVICE_NAME,
	    &errcode, errbuf);
	if (service_fd < 0) {
		SendCustMsg(HERE, 22019, SAMRFT_SPM_SERVICE_NAME,
		    errcode, errbuf);
		exit(1);
	}

	OpenLogFile(GetCfgLogFile());

	Trace(TR_MISC, "'%s' registered fd: %d",
	    SAMRFT_SPM_SERVICE_NAME, service_fd);
	Trace(TR_MISC, "Block size configuration: %d", GetCfgBlksize());
	Trace(TR_MISC, "TCP window size configuration: %d",
	    GetCfgTcpWindowsize());

	/*
	 * Get list of trusted clients from diskvols.conf file.
	 */
	getTrustedClients();

	/*
	 * Start infinite loop waiting for client connections.
	 */
	while (Shutdown == 0) {
		client_fd = waitForClient(service_fd,
		    (struct sockaddr *)&client_addr);
		if (client_fd >= 0) {
			startClientServer(client_fd,
			    (struct sockaddr *)&client_addr);
		}
	}
	SendCustMsg(HERE, 22001, Shutdown);
	Trace(TR_MISC, "Rft daemon exiting");

	return (0);
}

/*
 * Attach SAM-FS shared memory segment.
 */
void *
ShmatSamfs(
	int mode)
{
	shm_ptr_tbl = NULL;
	if ((master_shm.shmid = shmget(SHM_MASTER_KEY, 0, 0)) >= 0) {
		master_shm.shared_memory = shmat(master_shm.shmid, NULL, mode);
		if (master_shm.shared_memory != (void *)-1) {
			shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
			if (strcmp(shm_ptr_tbl->shm_block.segment_name,
			    SAM_SEGMENT_NAME) != 0) {
				errno = ENOCSI;
				shm_ptr_tbl = NULL;
			}
		}
	}
	return (shm_ptr_tbl);
}

/*
 * Wait until a connection is present.
 */
static int
waitForClient(
	int service_fd,
	struct sockaddr *client_addr)
{
	int client_fd;
	int errcode;
	long cliaddrbuf[64];
	char errbuf[SPM_ERRSTR_MAX];
	socklen_t clilen;
	struct sockaddr *cliaddr;

	cliaddr = (struct sockaddr *)cliaddrbuf;
	clilen = sizeof (cliaddrbuf);

	while (Shutdown == 0) {
		client_fd = spm_accept(service_fd, &errcode, errbuf);
		if (client_fd < 0) {
			if (Shutdown == 0) {
				SendCustMsg(HERE, 22020, errcode, errbuf);
			}
			if (getppid() == 1) {
				Shutdown = 1;
			}
			continue;
		}

		if (getpeername(client_fd, cliaddr, &clilen) < 0) {
			SysError(HERE, "getpeername failed");
			(void) close(client_fd);
			continue;
		}

		if (cliaddr->sa_family == AF_INET) {
			struct sockaddr_in *sin;

		/* LINTED pointer cast may result in improper alignment */
			sin = (struct sockaddr_in *)cliaddr;
			memcpy(client_addr, sin, sizeof (struct sockaddr_in));

		} else if (cliaddr->sa_family == AF_INET6) {
		/* LINTED pointer cast may result in improper alignment */
			struct sockaddr_in6  *sin6;

			sin6 = (struct sockaddr_in6 *)cliaddr;
			memcpy(client_addr, sin6, sizeof (struct sockaddr_in6));

		} else {
			SendCustMsg(HERE, 22018, cliaddr->sa_family);
			(void) close(client_fd);
			continue;
		}

		/*
		 * New connection, validate client's IP address.
		 */
		if (client_fd >= 0) {
			if (validateClientAddr(client_addr) == B_FALSE) {
				(void) close(client_fd);
				client_fd = -1;
			}
			break;
		}
	}

	return (client_fd);
}

/*
 * Client connection was accepted.  Fork a child rft daemon
 * to service this client only.  The parent will simply wait for
 * more client connections.
 */
static void
startClientServer(
	int client_fd,
	struct sockaddr *client_addr)
{
	Client_t *rft;
	pid_t pid;

	SamMalloc(rft, sizeof (Client_t));
	(void) memset(rft, 0, sizeof (Client_t));

	rft->fd = client_fd;
	memcpy(&rft->caddr, (struct sockaddr_in6 *)client_addr,
	    sizeof (struct sockaddr_in6));

	Trace(TR_DEBUG, "Client connected on socket: %d", rft->fd);

	pid = fork();
	ASSERT(pid >= 0);

	if (pid == 0) {
		/*
		 * Child.
		 */
		clientServer((void *)rft);
	} else {
		/*
		 * Parent.
		 */
		(void) close(client_fd);
	}
}

/*
 * Child rft daemon for servicing a client connection.
 */
static void
clientServer(
	void * arg)
{
	int level;
	int status;
	Client_t *rft;
	int tos;

	rft = (Client_t *)arg;

	TracePid = getpid();
	Trace(TR_DEBUG, "Rft daemon client started %d", rft->fd);

	status = pthread_detach(pthread_self());
	ASSERT(status == 0);

	rft->cmdsize = SAMRFT_DEFAULT_CMD_BUFSIZE;
	SamMalloc(rft->cmdbuf, rft->cmdsize);
	(void) memset(rft->cmdbuf, 0, rft->cmdsize);

	tos = IPTOS_LOWDELAY;
	level = (rft->caddr.sin6_family == AF_INET6) ?
	    IPPROTO_IPV6 : IPPROTO_IP;
	if (setsockopt(rft->fd, level, IP_TOS,
	    (char *)&tos, sizeof (int)) < 0) {
		Trace(TR_ERR, "setsockopt(IPTOS_LOWDELAY) failed %d", errno);
	}

	rft->cin  = fdopen(rft->fd, "r");
	rft->cout = fdopen(rft->fd, "w");

	while (rft->disconnect == 0) {

		if (GetCommand(rft) < 0) {
			continue;
		}
		Trace(TR_DEBUG, "Received command: '%s'", rft->cmdbuf);

		DoCommand(rft);
	}

	Trace(TR_DEBUG, "Rft daemon exiting");

	exit(0);
}

/*
 * Mask signals to catch or ignore.
 */
static void
setSignalHandling(void)
{
	sig_action.sa_handler = catchSignals;
	sigemptyset(&sig_action.sa_mask);
	sig_action.sa_flags = SA_RESTART;
	(void) sigaction(SIGHUP, &sig_action, NULL);
	(void) sigaction(SIGINT, &sig_action, NULL);
	(void) sigaction(SIGCHLD, &sig_action, NULL);
}

/*
 * Catch signals.
 */
static void
catchSignals(
	int signum)
{
	pid_t pid;
	int stat;
	int childStatus;
	int childSignal;

	switch (signum) {

		case SIGHUP:
			reconfig();
			break;

		case SIGTERM:
		case SIGINT:
			Shutdown = signum;
			break;

		case SIGCHLD:
			while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
				childStatus = (stat >> 8) & 0xf;
				childSignal = stat & 0xff;
				if (childStatus != 0 ||
				    (childSignal != 0 &&
				    (childSignal != SIGTERM &&
				    childSignal != SIGINT))) {

				Trace(TR_ERR,
				    "Child died %d status: %d signal: %d",
				    (int)pid, childStatus, childSignal);
				}
			}
			break;

		default:
			break;
	}
}


/*
 * Validate that a client's IP address belongs to a host
 * we know about.
 */
static boolean_t
validateClientAddr(
	struct sockaddr *client_addr)
{
	int af = client_addr->sa_family;
	int i;
	char *buffer;
	const char *client_nm;
	boolean_t valid = B_FALSE;
	char **p;
	struct hostent *hp;
	struct sockaddr_in *ca4;
	struct sockaddr_in6 *ca6;

	ca4 = (struct sockaddr_in *)client_addr;
	ca6 = (struct sockaddr_in6 *)client_addr;

	SamMalloc(buffer, INET6_ADDRSTRLEN);
	if (af == AF_INET6) {
		client_nm = inet_ntop(af, &ca6->sin6_addr.s6_addr[0],
		    buffer, INET6_ADDRSTRLEN);
	} else {
		client_nm = inet_ntop(af, &ca4->sin_addr,
		    buffer, INET6_ADDRSTRLEN);
	}

	for (i = 0; i < trustedClients.count; i++) {

		hp = &trustedClients.result[i];
		for (p = hp->h_addr_list; *p != 0; p++) {
			switch (hp->h_addrtype) {
			case AF_INET:
				if (memcmp(*p, &ca4->sin_addr.s_addr,
				    sizeof (struct in_addr)) == 0) {
					valid = B_TRUE;
				}
				break;
			case AF_INET6:
				if (memcmp(*p, &ca6->sin6_addr.s6_addr[0],
				    sizeof (struct in6_addr)) == 0) {
					valid = B_TRUE;
				}
				break;
			default:
				Trace(TR_MISC, "Unknown address family  %d",
				    hp->h_addrtype);
				break;
			}
		}
		if (valid) {
			break;
		}
	}

	if (valid) {
		Trace(TR_MISC, "Connection from '%s' authorized", client_nm);
	} else {
		Trace(TR_ERR, "Connection from '%s' not authorized", client_nm);
		/* Sam rft daemon connection from '%s' is not authorized */
		SendCustMsg(HERE, 22000, client_nm);
	}
	free(buffer);
	return (valid);
}

/*
 * Get list of trusted clients from the diskvols.conf and remote server
 * configuration files.  The list is available in a static private
 * variable, trustedClients.
 */
static void dirMedia();
static void dirEndmedia();
static void dirIgnore();
static void notDirective();
static void msgFunc(char *msg, int lineno, char *line);

static void
getTrustedClients(void)
{
	DirProc_t directives[] = {
		{ "media",		dirMedia,	DP_set	},
		{ "endmedia",		dirEndmedia,	DP_set	},
		{ "cache_path",		dirIgnore,	DP_set	},
		{ "min_size",		dirIgnore,	DP_set	},
		{ "max_size",		dirIgnore,	DP_set	},
		{ "cache_size",		dirIgnore,	DP_set	},
		{ "net_blk_size",	dirIgnore,	DP_set	},
		{ "no_cache",		dirIgnore,	DP_set	},
		{ NULL,			notDirective,	DP_other }
	};

	int i;
	struct  DiskVolumeInfo *dv;
	int high_eq;
	dev_ent_t *dev;
	dev_ent_t *head;
	DiskVolsDictionary_t *cliDict;
	char *hostname;

	/*
	 * Map in diskvols configuration for trusted disk archiving clients.
	 */
	cliDict = DiskVolsNewHandle(program_name, DISKVOLS_CLI_DICT,
	    DISKVOLS_RDONLY);
	if (cliDict != NULL) {
		/*
		 * Get number of trusted disk archiving clients
		 * from the diskvols.conf file.
		 */
		(void) cliDict->Numof(cliDict, &trustedClients.count,
		    DV_numof_all);

		/*
		 * Add trusted clients from diskvols.conf file.
		 */
		if (trustedClients.count > 0) {

			allocTrustedClients();

			cliDict->BeginIterator(cliDict);
			while (cliDict->GetIterator(cliDict,
			    &hostname, &dv) == 0) {
				addTrustedClient(hostname, "diskvols.conf");
			}
			cliDict->EndIterator(cliDict);
		}
		(void) DiskVolsDeleteHandle(DISKVOLS_CLI_DICT);
	}

	/*
	 * Add trusted clients from remote server configuration file.
	 */
	(void) read_mcf(NULL, &head, &high_eq);
	for (dev = head; dev != NULL; dev = dev->next) {
		if (dev->equ_type == DT_PSEUDO_SS) {
			int errors;

			errors = ReadCfg(dev->name, directives, dirName,
			    token, msgFunc);
			if (errors == -1) {
				Trace(TR_ERR, "Unable to read "
				    "configuration file '%s'",
				    dev->name);
			}
		}
	}

	for (i = 0; i < trustedClients.count; i++) {
		host_t *host;
		struct hostent *result;

		host = &trustedClients.host[i];
		result = &trustedClients.result[i];

		getIpNodeByName((char *)host, &result);
	}
}

/*
 * Add a trusted client.
 */
void
addTrustedClient(
	char *host,
	char *config_file)
{
	int idx;

	idx = trustedClients.free;

	Trace(TR_MISC, "Add trusted client '%s' [%s]", host, config_file);
	(void) memcpy(&trustedClients.host[idx], host, sizeof (host_t));
	trustedClients.free++;
}

/*
 * Allocate space for trusted clients.
 */
void
allocTrustedClients(void)
{
	size_t size;

	if (trustedClients.host == NULL) {
		size =  trustedClients.count * sizeof (host_t);
		SamMalloc(trustedClients.host, size);
		(void) memset(trustedClients.host, 0, size);

		size = trustedClients.count * sizeof (struct hostent);
		SamMalloc(trustedClients.result, size);
		(void) memset(trustedClients.result, 0, size);

		size = trustedClients.count * sizeof (host_buffer_t);
		SamMalloc(trustedClients.buffer, size);
		(void) memset(trustedClients.buffer, 0, size);

	} else {
		size =  trustedClients.count * sizeof (host_t);
		SamRealloc(trustedClients.host, size);

		size = trustedClients.count * sizeof (struct hostent);
		SamRealloc(trustedClients.result, size);

		size = trustedClients.count * sizeof (host_buffer_t);
		SamRealloc(trustedClients.buffer, size);
	}
}

/*
 * Free space for trusted clients.
 */
static void
deallocTrustedClients(void)
{
	if (trustedClients.count > 0) {
		SamFree(trustedClients.host);
		SamFree(trustedClients.result);
		SamFree(trustedClients.buffer);
		(void) memset(&trustedClients, 0, sizeof (trustedClients));
	}
}

/*
 * For a given host name return its IP address.
 */
static void
getIpNodeByName(
	char *host,
	struct hostent **result)
{
	struct hostent *hp;
	char buffer[INET6_ADDRSTRLEN];
	const char *addr;
	char **p;
	int host_errno;
	boolean_t found = B_FALSE;

	hp = getipnodebyname(host, AF_INET6, 0, &host_errno);
	if (hp != NULL) {
		for (p = hp->h_addr_list; *p != 0; p++) {
			if (**p != 0) {
				addr = inet_ntop(hp->h_addrtype, *p,
				    buffer, INET6_ADDRSTRLEN);
				Trace(TR_MISC, "af %d, length %d addr %X: "
				    "'%s' '%s' '%s'",
				    hp->h_addrtype, hp->h_length,
				    *(int *)*p,
				    addr, buffer, hp->h_name);
				found = B_TRUE;
			}
		}
	}
	if (!found) {
		hp = getipnodebyname(host, AF_INET, 0, &host_errno);
		if (hp != NULL) {
			for (p = hp->h_addr_list; *p != 0; p++) {
				if (**p != 0) {
					addr = inet_ntop(hp->h_addrtype,
					    *p, buffer, INET6_ADDRSTRLEN);
					Trace(TR_MISC,
					    "af %d, length %d addr %X: "
					    "'%s' '%s' '%s'",
					    hp->h_addrtype, hp->h_length,
					    *(int *)*p,
					    addr, buffer, hp->h_name);
					found = B_TRUE;
				}
			}
		}
	}
	if (!found) {
		SendCustMsg(HERE, 22003, host, host_errno);
	} else {
		memcpy(*result, hp, sizeof (struct hostent));
	}
}

static void
dirMedia(void)
{
	inMediaSection = B_TRUE;
}

static void
dirEndmedia(void)
{
	inMediaSection = B_FALSE;
}

static void
dirIgnore(void)
{
}

static void
notDirective(void)
{
	if (inMediaSection == B_FALSE) {
		trustedClients.count++;
		allocTrustedClients();
		addTrustedClient(token, "SAM-Remote");
	}
}

static void
msgFunc(
	/* LINTED argument unused in function */
	char *msg,
	/* LINTED argument unused in function */
	int lineno,
	/* LINTED argument unused in function */
	char *line)
{
}

/*
 * Read rft command file.
 */
static void
readCmdFile(void)
{
	struct stat buf;

	if (stat(cmdFileName, &buf) == -1) {
		/*
		 * Command file not found.
		 */
		cmdFileTime = 0;

	} else {
		cmdFileTime = buf.st_mtim.tv_sec;
	}
	ReadCmds();
}

static void
reconfig(void)
{
	struct stat buf;

	TraceReconfig();
	Trace(TR_MISC, "Reconfigure request received");
	if (stat(cmdFileName, &buf) == -1) {
		/*
		 * Command file not found.
		 */
		buf.st_mtim.tv_sec = 0;
	}

	if (buf.st_mtim.tv_sec != cmdFileTime) {
		Trace(TR_MISC, "Rereading command file");
		readCmdFile();
	}
	deallocTrustedClients();
	getTrustedClients();
}
