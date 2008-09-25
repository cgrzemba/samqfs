/*
 * rft_defs.h - rft (remote file transfer, was ftp) server definitions.
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

#ifndef RFTD_H
#define	RFTD_H

#pragma ident "$Revision: 1.13 $"

#include <sys/types.h>
#include <sys/socket.h>

#include <stdio.h>
#include <netdb.h>
#include <dirent.h>

#include <assert.h>

#if defined(DEBUG)
#define	ASSERT_NOT_REACHED(f) assert(0);
#else
#define	ASSERT_NOT_REACHED(f) {}
#endif

#define	RFTD_PROGRAM_NAME	"sam-rftd"
#define	COMMAND_FILE_NAME	"rft.cmd"

#define	RM_DIR ".rft"		/* removable media directory name (was .ftp) */
#define	DIR_MODE 0777		/* create mode for daemon's directories */
#define	FILE_MODE 0664		/* create mode for daemon's rm files */

/*
 * Will check for configuration changes whenever this timeout
 * value is reached.
 */
#define	CONFIG_CHECK_TIMEOUT_SECS 10

/*
 * Defined for receiving data from SAM-RFT File Transfer client.
 */
typedef struct Recv {
	void	*buf;			/* buffer to receive data */
	size_t	nbytes;			/* number of bytes to receive */

	/*
	 * Received data may be divided among workers.
	 * The ordering of the requests is established by the
	 * following receive pipeline.
	 */
	struct Recv		*next;
	struct Recv		*pipe;

	pthread_cond_t	done;		/* wait for receive to complete */
	int		done_flag;

	pthread_cond_t	gotit;		/* received data was processed */
	int		gotit_flag;
} Recv_t;

/*
 * Defined for sending data to SAM-RFT File Transfer client.
 */
typedef struct Send {
	void		*buf;		/* buffer for sending data */
	size_t		nbytes;		/* number of bytes to send */

	pthread_cond_t  done;		/* wait for send to complete */
	boolean_t	active_flag;
} Send_t;

/*
 * Worker for data transfer engine.
 */
typedef struct Worker {
	pthread_t	thread_id;
	struct Crew	*crew;

	pthread_mutex_t	mutex;
	pthread_cond_t	request;

	/*
	 * First and last work items when receiving
	 * data from client.
	 */
	Recv_t		*first, *last;

	/*
	 * Work item for sending data to client.
	 */
	Send_t		*item;
	boolean_t	item_ready;

	int		seqnum;			/* sequencing number */
	struct sockaddr_in	addr;
	FILE		*in;
	FILE		*out;

	void		*buf;
} Worker_t;

/*
 * Work crew for data transfer engine.
 */
typedef struct Crew {
	struct Client	*cli;

	size_t		dataportsize;		/* size of data port transfer */
	int		num_dataports;		/* number of data ports */

	Worker_t	*data;

	int		active;
	pthread_mutex_t	mutex;

	/*
	 * Pipeline of requests when receiving
	 * data from client.
	 */
	Recv_t		*head, *tail;

	int		fd;			/* descriptor for user's file */
	char		*filename;		/* user's file name */

	/*
	 * Data port communication.
	 */
	struct sockaddr_in6	addr;
	FILE		*in;
	FILE		*out;
	void		*buf;

	vsn_t		vsn;		/* mounted VSN */
	uint64_t	nbytes_sent;	/* number of bytes sent to client */
	uint64_t	nbytes_received;	/* bytes received from client */
} Crew_t;

/*
 * Client connected to SAM-RFT File Transfer server.
 */
typedef struct Client {

	char	*hostname;

	/*
	 * Control communication port.
	 */
	struct sockaddr_in6 caddr;
	int	fd;
	FILE	*cin;
	FILE	*cout;

	int	disconnect;	/* set if client has requested a disconnect */

	size_t	cmdsize;
	char	*cmdbuf;	/* buffer to hold commands */

	int	num_opendirs;	/* number of open directories */
	DIR	**opendirs;
	char	**openpaths;

	Crew_t	*crew;
} Client_t;

/*
 * Define prototypes in rft.c
 */
void *ShmatSamfs(int mode);

/*
 * Define prototypes in command.c
 */
int GetCommand(Client_t *client);
void DoCommand(Client_t *client);
void SendReply(Client_t *client, const char *fmt, ...);

/*
 * Define prototypes in crew.c
 */
int CreateCrew(Client_t *rft, int streamwidth, size_t streamsize);
int InitDataConnection(Client_t *rft, char *mode, int seqnum, int af,
						struct sockaddr *dataconn);
int OpenFile(Client_t *rft, char *filename, int oflag,
				SamrftCreateAttr_t *creat);
int SendData(Client_t *rft, size_t nbytes);
int ReceiveData(Client_t *rft, fsize_t nbytes);
int SeekFile(Client_t *rft, off64_t setpos, int whence, off64_t *offset);
int FlockFile(Client_t *rft, int type);
int CloseFile(Client_t *rft);
int UnlinkFile(Client_t *rft, char *file_name);
int IsMounted(Client_t *rft, char *mount_point);
int MkDir(Client_t *rft, char *dirname, int mode, int uid, int gid);
int OpenDir(Client_t *rft, char *dirname, int *dirp);
int ReadDir(Client_t *rft, int dirp, SamrftReaddirInfo_t *dir_info);
void CloseDir(Client_t *rft, int dirp);
int RmDir(Client_t *rft, char *dirname);

int LoadVol(Client_t *rft, struct sam_rminfo *rb, int oflag);
int GetVolInfo(Client_t *rft, struct sam_rminfo *getrm, int *eq);
int SeekVol(Client_t *rft, int block);
int UnloadVol(Client_t *rft, struct sam_ioctl_rmunload *unload);
void CleanupCrew(Client_t *rft);

/*
 * Define prototypes in readcmd.c
 */
void ReadCmds();
char *GetCfgLogFile();
int GetCfgBlksize();
int GetCfgTcpWindowsize();

/*
 * Define prototypes in worker.c
 */
void * Worker(void *arg);
void InitWorker(Worker_t *worker, size_t streamsize);
void CleanupWorker(Worker_t *worker);

#endif /* RFTD_H */
