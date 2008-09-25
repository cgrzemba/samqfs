/*
 * sam_rft.h - Rft interface definitions
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

#ifndef AML_RFT_H
#define	AML_RFT_H

#pragma ident "$Revision: 1.16 $"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <stdio.h>
#include <pthread.h>
#include <dirent.h>

#include "pub/rminfo.h"
#include "sam/types.h"
#include "sam/fioctl.h"

/*
 * Service name for file transfer daemon, registered with spm.
 */
#define	SAMRFT_SPM_SERVICE_NAME	"rft"

/*
 * file transfer daemon's directory in AML_VARIABLE_PATH (was "ftp").
 */
#define	SAMRFT_DIRNAME	"rft"

/*
 * Default value for each data xfer size (1024k).
 */
#define	SAMRFT_DEFAULT_BLKSIZE	1024 * 1024

/*
 * Default trace mask.
 */
#define	SAMRFT_DEFAULT_TRACE_MASK	TR_err | TR_cust

/*
 * Default size for command buffer.
 */
#define	SAMRFT_DEFAULT_CMD_BUFSIZE	10240

/*
 * Maximum path length.
 */
#define	SAMRFT_MAX_PATHLEN	sizeof (upath_t)

/*
 * Command type strings.
 */
#define	SAMRFT_CMD_CONNECT	"connect"
#define	SAMRFT_CMD_CONFIG	"config"
#define	SAMRFT_CMD_DPORT	"dport"
#define	SAMRFT_CMD_DPORT6	"dport6"
#define	SAMRFT_CMD_OPEN		"openfile"
#define	SAMRFT_CMD_SEND		"send"
#define	SAMRFT_CMD_STOR		"stor"
#define	SAMRFT_CMD_RECV		"recv"
#define	SAMRFT_CMD_CLOSE	"closefile"
#define	SAMRFT_CMD_UNLINK	"unlinkfile"
#define	SAMRFT_CMD_DISCONN	"disconn"
#define	SAMRFT_CMD_ISMOUNTED	"ismounted"
#define	SAMRFT_CMD_STAT		"stat"
#define	SAMRFT_CMD_STATVFS	"statvfs"
#define	SAMRFT_CMD_SPACEUSED	"spaceused"
#define	SAMRFT_CMD_MKDIR	"mkdir"
#define	SAMRFT_CMD_OPENDIR	"opendir"
#define	SAMRFT_CMD_READDIR	"readdir"
#define	SAMRFT_CMD_CLOSEDIR	"closedir"
#define	SAMRFT_CMD_RMDIR	"rmdir"
#define	SAMRFT_CMD_SEEK		"seek"
#define	SAMRFT_CMD_FLOCK	"flock"
#define	SAMRFT_CMD_ARCHIVEOP	"archiveop"

#define	SAMRFT_CMD_LOADVOL	"loadvol"
#define	SAMRFT_CMD_GETVOLINFO	"getvolinfo"
#define	SAMRFT_CMD_SEEKVOL	"seekvol"
#define	SAMRFT_CMD_UNLOADVOL	"unloadvol"

/*
 * Rft daemon's trace configuration options.
 */
typedef struct sam_rft_trace {
	upath_t		filename;	/* path to trace file */
	uint32_t	mask;		/* trace mask */
} sam_rft_trace_t;

/*
 * Rft daemon configuration.
 */
typedef struct sam_rft_config {
	int		cmd_bufsize;	/* default command buffer size */
	upath_t		logfile;	/* path to log file */
	sam_rft_trace_t	trace;		/* trace configuration options */
	int		blksize;	/* data block size */
	int		tcpwindow;	/* TCP window size */
} sam_rft_config_t;

/*
 * Defined for receiving data from SAM-RFT server.
 */
typedef struct SamrftRecv {
	void		*buf;		/* buffer to receive data */
	size_t		nbytes;		/* number of bytes to receive */
	pthread_cond_t	done;		/* wait for receive to complete */
	int		active_flag;
} SamrftRecv_t;


/*
 * Defined for sending data to SAM-RFT server.
 */
typedef struct SamrftSend {
	void		*buf;		/* buffer for sending data */
	size_t		nbytes;		/* number of bytes to send */
	struct SamrftSend *next;	/* next request to send */
} SamrftSend_t;

/*
 * Work crew and workers for data transfer engine.
 *
 * A data transfer between server and client can be split among
 * multiple data port.  Each data port is transferred independently
 * by a set of threads, called workers, over separate sockets.
 * Workers are members of a work crew.  A work crew is responsible for
 * handing out i/o requests to the independent worker threads.  Each
 * Samrft client connection to the server results in a crew and
 * associated workers.
 */

/*
 * Worker for data transfer engine.
 */
typedef struct SamrftWorker {
	pthread_t	thread_id;
	struct SamrftCrew *crew;	/* member of this crew */

	int		seqnum;		/* sequencing number */

	/*
	 * Data port communication.
	 */
	FILE		*in;
	FILE		*out;
	int		flags;

	pthread_mutex_t	mutex;
	pthread_cond_t	request;

	/*
	 * First and last work items when sending
	 * data to client.
	 */
	SamrftSend_t	*first, *last;

	/*
	 * Work item for receiving data from client.
	 */
	SamrftRecv_t	*item;
	int		item_ready;
} SamrftWorker_t;

#define	SAMRFT_IPV6	0x0001

/*
 * Work crew for data transfer engine.
 */
typedef struct SamrftCrew {
	size_t		dataportsize;	/* size of data port transfer */
	int		num_dataports;	/* number of data ports */

	/*
	 * Workers for data transfer.
	 */
	SamrftWorker_t	*data;

	int		active;
	pthread_mutex_t	mutex;
	pthread_cond_t	done;

	/*
	 * Pipeline of receive data requests that has
	 * been divided among workers.  The ordering of
	 * data requests is established by this pipeline.
	 */
	SamrftRecv_t	*head, *tail;

	int		fd;		/* descriptor for user's file  */

	/*
	 * Data port communication.
	 */
	FILE		*in;
	FILE		*out;
	union {
		struct sockaddr_in6	ad6;
		struct sockaddr_in	ad;
	} addr;
	int		flags;
	void		*buf;

} SamrftCrew_t;

typedef struct SamrftImpl {

	boolean_t	remotehost;		/* remote data transfer */
	char		*hostname;		/* host name for client */

	/*
	 * Control communication port.
	 */
	FILE		*cin;
	FILE		*cout;
	union {
		struct sockaddr_in6	cad6;
		struct sockaddr_in	cad;
	} caddr;
	int		flags;

	/*
	 * Local data transfer control.
	 */
	int		fd;

	size_t		cmdsize;		/* size of command buffer */
	char		*cmdbuf;		/* command buffer */

	/*
	 * Work crew for data ports.
	 */
	SamrftCrew_t	*crew;

} SamrftImpl_t;

#define	Si_addr  caddr.cad.sin_addr
#define	Si_addr6 caddr.cad6.sin6_addr

/*
 * Attributes for opening a file on remote host.
 */
typedef struct SamrftOpenAttr {
	int		oflag;		/* file access flags */
	uint_t		offset;		/* segmented file offset to */
					/* position on open */
} SamrftOpenAttr_t;

/*
 * Attributes for creating a file or directory on remote host.
 */
typedef struct SamrftCreateAttr {
	uint_t		mode;		/* file mode */
	uid_t		uid;		/* user id of file's owner */
	gid_t		gid;		/* group id of file's group */
} SamrftCreateAttr_t;

/*
 * File/directory information from reading directory on remote host.
 */
typedef struct SamrftReaddirInfo {
	char		*name;
	boolean_t	isdir;
} SamrftReaddirInfo_t;

/*
 * Status information for file on remote host.
 */
typedef struct SamrftStatInfo {
	uint_t		mode;		/* file mode */
	uid_t		uid;		/* user id of file's owner */
	gid_t		gid;		/* group id of file's group */
	long long	size;		/* file size in bytes */
} SamrftStatInfo_t;

char *SamrftGetHostByAddr(void *addr, int af);
boolean_t SamrftIsAccessible(char *host, char *pathname);
SamrftImpl_t *SamrftConnect(char *host);
int SamrftOpen(SamrftImpl_t *rftd, char *filename, int oflag,
	SamrftCreateAttr_t *attr);
int SamrftStore(SamrftImpl_t *rftd, fsize_t nbytes);
size_t SamrftSend(SamrftImpl_t *rftd, void *buf, size_t nbytes);
size_t SamrftWrite(SamrftImpl_t *rftd, void *buf, size_t nbytes);
size_t SamrftRead(SamrftImpl_t *rftd, void *buf, size_t nbytes);
int SamrftClose(SamrftImpl_t *rftd);
int SamrftUnlink(SamrftImpl_t *rftd, char *name);
void SamrftDisconnect(SamrftImpl_t *rftd);
int SamrftIsMounted(SamrftImpl_t *rftd, char *mount_point);
int SamrftStat(SamrftImpl_t *rftd, char *filename, SamrftStatInfo_t *buf);
int SamrftStatvfs(SamrftImpl_t *rftd, char *path,
	boolean_t offlineFiles, struct statvfs64 *buf);
int SamrftSpaceUsed(SamrftImpl_t *rftd, char *path, fsize_t *used);
int SamrftMkdir(SamrftImpl_t *rftd, char *dirname, SamrftCreateAttr_t *attr);
int SamrftOpendir(SamrftImpl_t *rftd, char *dirname, int *dirp);
int SamrftReaddir(SamrftImpl_t *rftd, int dirp, SamrftReaddirInfo_t *dir_info);
void SamrftClosedir(SamrftImpl_t *rftd, int dirp);
int SamrftRmdir(SamrftImpl_t *rftd, char *dirname);
int SamrftSeek(SamrftImpl_t *rftd, off64_t setpos, int whence, off64_t *offset);
int SamrftFlock(SamrftImpl_t *rftd, int type);
int SamrftArchiveOp(SamrftImpl_t *rftd, char *path, const char *ops);

int SamrftLoadVol(SamrftImpl_t *rftd, struct sam_rminfo *attr, int oflag);
int SamrftGetVolInfo(SamrftImpl_t *rftd, struct sam_rminfo *getrm, int *eq);
int SamrftSeekVol(SamrftImpl_t *rftd, int block);
int SamrftUnloadVol(SamrftImpl_t *rftd, struct sam_ioctl_rmunload *unload);

#endif /* AML_RFT_H */
