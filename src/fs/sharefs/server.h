/*
 * server.h - File system server shared daemon common definitions.
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

#ifndef SERVER_H
#define	SERVER_H

#pragma ident "$Revision: 1.24 $"


/*
 * ----- Server Host structs.
 * The server has a linked list of the server clients.
 */
typedef struct client_list_head {
	struct client_list *forw;
	struct client_list *back;
} client_list_head_t;

typedef struct client_list {
	struct client_list_head	chain;
	int sockfd;			/* Srvr socket for this client */
	int hostord;			/* Host's ord/position in hosts file */
	int killThread;			/* if this clnt thr should terminate */
	int flags;			/* byte order flags */
	upath_t hname;			/* Client hostname */
	pthread_t rd_socket;
	uint64_t tags;			/* client behavior tag list */
} client_list_t;

typedef struct server_host_data {
	struct client_list_head	chain;
	pthread_mutex_t server_lock;	/* Server client list mutex */
	pthread_t listen_tid;		/* ServerListenerSocket thread */
} server_host_data_t;


/*
 * Used to cache IP addresses so that we can
 * validate and determine the hostname of incoming
 * IP connects.
 */
typedef struct cache_ent {
	upath_t host;
	struct ip_list {
		struct ip_list *next;
		void *addr;
		int addrlen;
	} *addr;
} cache_ent_t;

/*
 * ----- Public data declaration/initialization macros.
 */
#undef DCL
#undef IVAL
#if defined(DEC_INIT)
#define	DCL
#define	IVAL(v) = v
#else /* defined(DEC_INIT) */
#define	DCL extern
#define	IVAL(v) /* v */
#endif /* defined(DEC_INIT) */


DCL server_host_data_t SrHost;	/* Server Host data struct */
DCL int NumofClients IVAL(0);	/* Number of active shared clients */


/* Public functions. */

void OpenDevs(char *fs);	/* Open root + label devs, and leave open */
void CloseDevs(void);		/* Close root + label devs */

void CloseReaderSockets(void);		/* Close reader threads' sockets */

/*
 * Notify the OS that it's the server
 */
int SetServer(char *fs_name, char *server, int ord, int maxord);

/*
 * Connect a TCP connection to the kernel.
 */
int SetServerSocket(char *fs_name, char *host, int ord, int fd,
		    uint64_t tags, int flags);

/*
 * Notify the kernel about cluster hosts for possible early
 * failover completion.
 */
void SetClusterInfo(char *fs_name);

/*
 * Verify that a client's capabilities are properly matched
 * between the clients' reported info (byte swap, metadata access)
 * and the shared hosts file.  Issue warning messages (only) on
 * failures.
 */
void VerifyClientCap(int hostord, int srvrcap, int byterev);

/*
 * Get shared hosts table (as last read)
 */
int GetHostTab(struct sam_host_table_blk *);

/*
 * DeCommaStr() -- Explode comma-separated string into
 * NULL terminated char *array[]
 */
char **DeCommaStr(char *str, int len);

/*
 * DeCommaStrFree() -- Free list from DeCommaStr
 */
void DeCommaStrFree(char **strlist);

/*
 * Compare kernel's internal FS configuration info
 */
int CmpFsInfo(char *fs, struct sam_mount_info *mip1,
						struct sam_mount_info *mip2);

#endif	/* SERVER_H */
