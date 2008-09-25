/*
 * sam_host.h - Shared file system host definitions.
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

#ifndef SAM_HOST_H
#define	SAM_HOST_H

#ifdef sun
#pragma ident "$Revision: 1.30 $"
#endif

#include "sam/types.h"


#define	SAM_HOSTS_TABLE_SIZE	(16*(SAM_DEV_BSIZE))
#define	SAM_LARGE_HOSTS_TABLE_SIZE	(2*1024*(SAM_DEV_BSIZE))

/*
 * Number of client host entries in each chunk
 * of the incore client host table.
 */
#define	SAM_INCORE_HOSTS_TABLE_INC	256

/*
 * Number of chunks of the incore client host table.
 */
#define	SAM_INCORE_HOSTS_INDEX_SIZE	256

#define	SAM_HOSTS_VERSION1	1		/* obs. fixed format table */
#define	SAM_HOSTS_VERSION2	2		/* obs. pre-release version */
#define	SAM_HOSTS_VERSION3	3		/* split server -> pend+srv */
#define	SAM_HOSTS_VERSION4	4		/* add prevsrv for cluster */

#define	SAM_HOSTS_VERSION_MIN	3
#define	SAM_HOSTS_VERSION_MAX	4

#define	SAM_HOSTS_COOKIE	0x48735462	/* 'HsTb' (V5+) */

/*
 * Shared host binary file definition.
 */
typedef struct sam_host_table {
	uint32_t	cookie;		/* For ident and byte order */
	uint32_t	unused1;	/* must be 0 */
	uint32_t	unused2;	/* must be 0 */
	uint32_t	version;	/* 4 */
	uint32_t	gen;		/* generation #; never reused */
	uint16_t	prevsrv;	/* previous server index */
	uint16_t	count;		/* host count */
	uint32_t	length;		/* byte count of this structure */
	uint16_t	pendsrv;	/* pending host server index */
	uint16_t	server;		/* host server index */
	char		ent[1];		/* total struct size to 16k/2048k */
} sam_host_table_t;

typedef struct sam_host_table_blk {
	union {
		struct sam_host_table ht;
		char i[SAM_HOSTS_TABLE_SIZE];
	} info;
} sam_host_table_blk_t;

/*
 * Fields in the hosts file
 */
#define	HOSTS_NAME	0	/* name of host */
#define	HOSTS_IP	1	/* IP addresses of host (comma separated) */
#define	HOSTS_PRI	2	/* host's server priority */
#define	HOSTS_HOSTONOFF	3	/* client/host on or off field */
#define	HOSTS_SERVER	4	/* optional "server" declaration */
#define	HOSTS_FIELDMAX	5	/* field count */

/*
 * Special values in server, pendsrv, prevsrv fields
 */
#define	HOSTS_NOSRV	((uint16_t)65535)	/* -1 */
	/* Involuntary failover for prevsrv field */
	/* No server specified for server or pendsrv fields */

/*
 * External Functions.
 */

/* Generate exploded hosts file from text hosts file */
char ***SamReadHosts(char *hostsfile);

/* Generate exploded hosts file from binary hosts structure */
char ***SamHostsCvt(struct sam_host_table *, char **errmsg, int *errc);

/* Free exploded hosts file */
void SamHostsFree(char ***);

/* Convert exploded hosts file to binary hosts structure */
int SamStoreHosts(struct sam_host_table *, int, char ***, int);

/*
 * Get/Put the hosts file from/to the given slice 0 raw device name
 */
int SamGetRawHosts(char *dev, struct sam_host_table_blk *ht,
		int htbufsize, char **errmsg, int *errc);
int SamPutRawHosts(char *dev, struct sam_host_table_blk *ht,
		int htbufsize, char **errmsg, int *errc);

/*
 * Extract the host name from a host table and an index
 */
int SamGetSharedHostInfo(struct sam_host_table *host, int hostno, upath_t name,
		upath_t addr, char *serverpri, char *onoff);
int SamGetSharedHostName(struct sam_host_table *host, int hostno, upath_t name);
int GetSharedHostInfo(struct sam_host_table *host, int hostno, upath_t name,
		upath_t addr);

/* Get hosts file given file descriptor */
int sam_fd_host_table_get(int fd, struct sam_host_table *htp);
int sam_fd_host_table_get2(int fd, int htsize, struct sam_host_table *htp);

#endif /* SAM_HOST_H */
