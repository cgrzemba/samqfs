/*
 * sharefs.h - Filesystem client shared daemon common definitions.
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

#ifndef SHAREFS_H
#define	SHAREFS_H

#pragma ident "$Revision: 1.48 $"

#include "sam/custmsg.h"
#include "sam/sam_trace.h"
#include "sam/osversion.h"

#define	SHAREFS_DIRNAME	"sharefsd"

#define	LOCAL_IPADDR_SEP		'@'

/*
 * ----- Client Host structs.
 */

typedef struct host_data {
	struct sam_fs_info fs;		/* Mount point fs info */
	upath_t hname;			/* This hostname */
	upath_t server;			/* Server hostname */
	int serverord;			/* server ordinal */
	int maxord;			/* Number of FS hosts */
	upath_t serveraddr;		/* server addresses */
	pthread_t listen_tid;		/* ServerListenerSocket thread */
	pthread_t sock_tid;		/* ClientReadSocket thread id */
	pthread_t main_tid;		/* Main thread id */
	int sockfd;			/* Client socket file descriptor */
	uint64_t server_tags;		/* server's behavior tags */
} host_data_t;


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


DCL boolean_t ShutdownDaemon IVAL(FALSE);
DCL boolean_t ServerHost IVAL(FALSE);
DCL boolean_t Conn2Srvr IVAL(FALSE);
DCL host_data_t Host;		/* Client Host data struct */
DCL boolean_t FsCfgOnly IVAL(FALSE);


/* Public functions. */
boolean_t ValidateFs(char *fs_name, struct sam_fs_info *fi);
int GetServerInfo(char *fs_name);
int Dsk2Rdsk(char *dsk, char *rdsk);

void OpenDevs(char *fs);	/* Open root + label devs, and leave open */
void CloseDevs(void);		/* Close root + label devs */

/*
 * Check FS partitions, superblock, hosts file, hosts.<fs>.local
 * or any available subset to see if any configuration information
 * has changed.  Update the internal copies if any has.
 */
void DoUpdate(char *fs);

/*
 * Notify the OS that it's a client.
 */
int SetClient(char *fs_name, char *server, int ord, uint64_t tags,
    int amserver);

/*
 * Set socket options as appropriate for our connections.
 */
void ConfigureSocket(int sockfd, char *fs);

/*
 * Verify that the TCP/IP connection is connected to a suitable target.
 */
int VerifyClientSocket(int sockfd, uint64_t *tags);
int VerifyServerSocket(int sockfd, char *client, int *srvrcap, int *byterev,
		uint64_t *tags);

/*
 * Connect a TCP connection to the kernel.
 */
int SetClientSocket(char *fs_name, char *host, int fd, int flags);

/*
 * Cluster functions.
 */
int IsClusterNodeUp(char *node);	/* 0 yes, 1 no, < 0 not cluster */

/*
 * Initiate voluntary failover
 */
int SysFailover(char *fs, char *server);

int GetHostTab(struct sam_host_table_blk *);	/* get host table */
char **DeCommaStr(char *str, int len);	/* explode comma-separated string  */
					/* into NULL-term'd char *array[] */
void DeCommaStrFree(char **);		/* free list from DeCommaStr  */

/*
 * Return codes from GetServerInfo
 */
#define	CFG_SERVER	(2)	/* server */
#define	CFG_HOST	(1)	/* srvr someday, client for now */
#define	CFG_CLIENT	(0)	/* definitely a client */
#define	CFG_ERROR	(-1)	/* problem */
#define	CFG_FATAL	(-2)	/* definitely a prob; exit w/o restart */

#endif /* SHAREFS_H */
