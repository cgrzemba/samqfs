/*
 * command.c - send and receive rft commands from client
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#pragma ident "$Revision: 1.14 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

/* ANSI C headers. */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

/* SAM-FS headers. */
#include "pub/lib.h"
#include "sam/types.h"
#include "sam/sam_trace.h"
#include "sam/sam_malloc.h"
#include "aml/sam_rft.h"
#include "aml/diskvols.h"

/* Local headers. */
#include "rft_defs.h"

/* Local functions. */
static char *getString(char **lasts);
static int getInteger(char **lasts);
static long long getLongLong(char **lasts);

/*
 * Get command from client.
 */
int
GetCommand(
	Client_t *cli)
{
	char *cs;
	char *msg;

	msg = cli->cmdbuf;

	if (fgets(msg, cli->cmdsize, cli->cin) == NULL) {
		cli->disconnect = 1;
		Trace(TR_MISC, "Empty command, client drop...disconnect");
		return (-1);
	}

	if (msg == NULL || *msg == '\0') {
		return (-1);
	}

	cs = strpbrk(msg, "\n");
	ASSERT(cs != NULL);
	*cs++ = '\0';

	return (0);
}

/*
 * Send command reply to client.
 */
void
SendReply(
	Client_t *cli,
	const char *fmt,
	...)
{
	char *msg;
	va_list args;

	msg = cli->cmdbuf;
	va_start(args, fmt);
	(void) vsprintf(msg, fmt, args);
	va_end(args);

	Trace(TR_DEBUG, "Send reply: '%s'", cli->cmdbuf);

	(void) fputs(cli->cmdbuf, cli->cout);

	(void) fprintf(cli->cout, "\n");
	(void) fflush(cli->cout);

	SetErrno = 0;
}

/*
 * Execute command.
 */
void
DoCommand(
	Client_t *cli)
{
	int rc;
	char *cmd_name;
	char *lasts;
	char *msg;

	rc = 0;
	SetErrno = 0;

	msg = cli->cmdbuf;
	/*
	 * Extract command name.
	 */
/* LINTED improper pointer/integer combination */
	cmd_name = strtok_r(msg, " ", &lasts);

	if (strcmp(cmd_name, SAMRFT_CMD_CONNECT) == 0) {
		SendReply(cli, "%s %d %d", SAMRFT_CMD_CONNECT, 0, 0);

	} else if (strcmp(cmd_name, SAMRFT_CMD_CONFIG) == 0) {
		char *hostname;
		int blksize;
		int tcpwindowsize;

		hostname = getString(&lasts);
		blksize = GetCfgBlksize();
		tcpwindowsize = GetCfgTcpWindowsize();

		SamStrdup(cli->hostname, hostname);
		rc = CreateCrew(cli, 1, blksize);

		SendReply(cli, "%s %d %d %d %d",
		    SAMRFT_CMD_CONFIG, rc, 1, blksize, tcpwindowsize);

	} else if (strcmp(cmd_name, SAMRFT_CMD_OPEN) == 0) {
		char *filename;
		int oflag;
		SamrftCreateAttr_t creat;

		filename = getString(&lasts);
		oflag = getInteger(&lasts);

		if (getInteger(&lasts)) {
			(void) memset((char *)&creat, 0,
			    sizeof (SamrftCreateAttr_t));
			creat.mode = getInteger(&lasts);
			creat.uid = getInteger(&lasts);
			creat.gid = getInteger(&lasts);
			rc = OpenFile(cli, filename, oflag, &creat);
		} else {
			rc = OpenFile(cli, filename, oflag, NULL);
		}

		SendReply(cli, "%s %d %d", SAMRFT_CMD_OPEN, rc, errno);

	} else if (strcmp(cmd_name, SAMRFT_CMD_DPORT) == 0) {
		struct sockaddr_in data;
		int seqnum;
		char *addr = (char *)&data.sin_addr;
		char *port = (char *)&data.sin_port;

		(void) memset((char *)&data, 0, sizeof (struct sockaddr_in));
		data.sin_family = AF_INET;

		seqnum = getInteger(&lasts);

		/*
		 * Addr.
		 */
		addr[0] = getInteger(&lasts);
		addr[1] = getInteger(&lasts);
		addr[2] = getInteger(&lasts);
		addr[3] = getInteger(&lasts);

		/*
		 * Port.
		 */
		port[0] = getInteger(&lasts);
		port[1] = getInteger(&lasts);

		rc = InitDataConnection(cli, "r", seqnum,
		    data.sin_family, (struct sockaddr *)&data);
		SendReply(cli, "%s %d %d", SAMRFT_CMD_DPORT, rc, errno);

	} else if (strcmp(cmd_name, SAMRFT_CMD_DPORT6) == 0) {
		struct sockaddr_in6 data;
		struct sockaddr_in *data4 = (struct sockaddr_in *)&data;
		int i;
		int seqnum;
		char *addr;
		char *port;

		(void) memset((char *)&data, 0, sizeof (struct sockaddr_in6));

		seqnum = getInteger(&lasts);

		/*
		 * Address family.
		 */
		data.sin6_family = getInteger(&lasts);
		if (data.sin6_family == AF_INET6) {
			addr = (char *)&data.sin6_addr;
			port = (char *)&data.sin6_port;
		} else {
			addr = (char *)&data4->sin_addr;
			port = (char *)&data4->sin_port;
		}

		/*
		 * Addr.
		 */
		for (i = 0; i < 16; i++) {
			addr[i] = getInteger(&lasts);
		}

		/*
		 * Port.
		 */
		port[0] = getInteger(&lasts);
		port[1] = getInteger(&lasts);

		rc = InitDataConnection(cli, "r", seqnum,
		    data.sin6_family, (struct sockaddr *)&data);
		SendReply(cli, "%s %d %d", SAMRFT_CMD_DPORT6, rc, errno);

	} else if (strcmp(cmd_name, SAMRFT_CMD_STOR) == 0) {
		fsize_t nbytes;

		nbytes = getLongLong(&lasts);
		rc = ReceiveData(cli, nbytes);

	} else if (strcmp(cmd_name, SAMRFT_CMD_SEND) == 0) {
		size_t nbytes;

		nbytes = getInteger(&lasts);

		rc = ReceiveData(cli, nbytes);
		SendReply(cli, "%s %d %d", SAMRFT_CMD_SEND, rc, errno);

	} else if (strcmp(cmd_name, SAMRFT_CMD_RECV) == 0) {
		size_t nbytes;

		nbytes = getInteger(&lasts);

		/*
		 * Process receive command from client.  Send data from local
		 * file over data sockets to the client process.
		 */
		rc = SendData(cli, nbytes);
		SendReply(cli, "%s %d %d", SAMRFT_CMD_RECV, rc, errno);

	} else if (strcmp(cmd_name, SAMRFT_CMD_SEEK) == 0) {
		off64_t setpos;
		int whence;
		off64_t offset;

		setpos = getLongLong(&lasts);
		whence = getInteger(&lasts);

		rc = SeekFile(cli, setpos, whence, &offset);
		SendReply(cli, "%s %d %d %lld", SAMRFT_CMD_SEEK,
		    rc, errno, offset);

	} else if (strcmp(cmd_name, SAMRFT_CMD_FLOCK) == 0) {
		int type;

		type = getInteger(&lasts);

		rc = FlockFile(cli, type);
		SendReply(cli, "%s %d %d", SAMRFT_CMD_FLOCK, rc, errno);

	} else if (strcmp(cmd_name, SAMRFT_CMD_ARCHIVEOP) == 0) {
		char *path;
		char *ops;

		path = getString(&lasts);
		ops = getString(&lasts);

		rc = sam_archive(path, ops);
		SendReply(cli, "%s %d %d", SAMRFT_CMD_ARCHIVEOP, rc, errno);

	} else if (strcmp(cmd_name, SAMRFT_CMD_CLOSE) == 0) {

		rc = CloseFile(cli);
		SendReply(cli, "%s %d %d", SAMRFT_CMD_CLOSE, rc, errno);

	} else if (strcmp(cmd_name, SAMRFT_CMD_UNLINK) == 0) {
		char *name;

		name = getString(&lasts);

		rc = UnlinkFile(cli, name);
		SendReply(cli, "%s %d %d", SAMRFT_CMD_UNLINK, rc, errno);

	} else if (strcmp(cmd_name, SAMRFT_CMD_DISCONN) == 0) {

		Trace(TR_DEBUG, "RFT disconnect, no reply: '%s'", cli->cmdbuf);
		CleanupCrew(cli);
		cli->disconnect = 1;

		/*
		 * No reply.
		 */

	} else if (strcmp(cmd_name, SAMRFT_CMD_ISMOUNTED) == 0) {
		int mounted;
		char *mount_point;

		mount_point = getString(&lasts);

		mounted = IsMounted(cli, mount_point);
		SendReply(cli, "%s %d", SAMRFT_CMD_ISMOUNTED, mounted);

	} else if (strcmp(cmd_name, SAMRFT_CMD_STAT) == 0) {
		struct stat64 buf;
		char *filename;

		filename = getString(&lasts);

		(void) memset(&buf, 0, sizeof (buf));
		rc = stat64(filename, &buf);
		SendReply(cli, "%s %d %d %d %d %d %lld",
		    SAMRFT_CMD_STAT, rc, errno,
		    buf.st_mode, buf.st_uid, buf.st_gid, buf.st_size);

	} else if (strcmp(cmd_name, SAMRFT_CMD_STATVFS) == 0) {
		struct statvfs64 buf;
		char *mount_point;
		int offlineFiles;
		fsize_t offlineFileSize;

		mount_point = getString(&lasts);
		offlineFiles = getInteger(&lasts);

		(void) memset(&buf, 0, sizeof (buf));
		rc = statvfs64(mount_point, &buf);

		if (rc == 0 && (strcmp(buf.f_basetype, "samfs") == 0) &&
		    offlineFiles == B_TRUE) {
			offlineFileSize = DiskVolsOfflineFiles(mount_point);
			/*
			 * Adjust capacity to include size of offline files.
			 */
			buf.f_blocks += offlineFileSize / buf.f_frsize;
		}

		SendReply(cli, "%s %d %d %lld %lld %ld %s",
		    SAMRFT_CMD_STATVFS, rc, errno,
		    buf.f_bfree, buf.f_blocks, buf.f_frsize, buf.f_basetype);

	} else if (strcmp(cmd_name, SAMRFT_CMD_SPACEUSED) == 0) {
		char *path;
		fsize_t spaceUsed;

		path = getString(&lasts);

		spaceUsed = DiskVolsAccumSpaceUsed(path);

		SendReply(cli, "%s %d %d %lld",
		    SAMRFT_CMD_SPACEUSED, rc, errno, spaceUsed);

	} else if (strcmp(cmd_name, SAMRFT_CMD_MKDIR) == 0) {
		char *dirname;
		int mode, uid, gid;

		dirname = getString(&lasts);
		mode = getInteger(&lasts);
		uid = getInteger(&lasts);
		gid = getInteger(&lasts);

		rc = MkDir(cli, dirname, mode, uid, gid);
		SendReply(cli, "%s %d %d", SAMRFT_CMD_MKDIR, rc, errno);

	} else if (strcmp(cmd_name, SAMRFT_CMD_OPENDIR) == 0) {
		char *dirname;
		int dirp;

		dirname = getString(&lasts);

		rc = OpenDir(cli, dirname, &dirp);
		SendReply(cli, "%s %d %d %d", SAMRFT_CMD_OPENDIR,
		    rc, errno, dirp);

	} else if (strcmp(cmd_name, SAMRFT_CMD_READDIR) == 0) {
		int dirp;
		SamrftReaddirInfo_t dir_info;

		dirp = getInteger(&lasts);

		rc = ReadDir(cli, dirp, &dir_info);
		if (rc == 0) {
			SendReply(cli, "%s %d %d %s %d", SAMRFT_CMD_READDIR,
			    rc, errno, dir_info.name, dir_info.isdir);
		} else {
			SendReply(cli, "%s %d %d", SAMRFT_CMD_READDIR,
			    rc, errno);
		}

	} else if (strcmp(cmd_name, SAMRFT_CMD_CLOSEDIR) == 0) {
		int dirp;

		dirp = getInteger(&lasts);

		CloseDir(cli, dirp);
		SendReply(cli, "%s %d %d", SAMRFT_CMD_CLOSEDIR, 0, 0);

	} else if (strcmp(cmd_name, SAMRFT_CMD_RMDIR) == 0) {
		char *dirname;

		dirname = getString(&lasts);

		rc = RmDir(cli, dirname);
		SendReply(cli, "%s %d %d", SAMRFT_CMD_CLOSEDIR, rc, errno);

	} else if (strcmp(cmd_name, SAMRFT_CMD_LOADVOL) == 0) {
		struct sam_rminfo rb;
		int oflag;

		/*
		 * Create sam_rminfo structure used to create
		 * a removable-media file.
		 */
		(void) memset(&rb, 0, sizeof (rb));

		rb.flags = getInteger(&lasts);

		(void) strncpy(rb.file_id,  getString(&lasts),
		    sizeof (rb.file_id));
		(void) strncpy(rb.owner_id, getString(&lasts),
		    sizeof (rb.owner_id));
		(void) strncpy(rb.group_id, getString(&lasts),
		    sizeof (rb.group_id));

		rb.n_vsns = 1;
		(void) strncpy(rb.media, getString(&lasts), sizeof (rb.media));
		(void) strncpy(rb.section[0].vsn, getString(&lasts),
		    sizeof (rb.section[0].vsn));

		oflag = getInteger(&lasts);

		rc = LoadVol(cli, &rb, oflag);

		SendReply(cli, "%s %d %d", SAMRFT_CMD_LOADVOL, rc, errno);

	} else if (strcmp(cmd_name, SAMRFT_CMD_GETVOLINFO) == 0) {
		struct sam_rminfo getrm;
		int eq;

		rc = GetVolInfo(cli, &getrm, &eq);

		SendReply(cli, "%s %d %d %d %lld %d", SAMRFT_CMD_GETVOLINFO,
		    rc, errno, getrm.block_size, getrm.position, eq);

	} else if (strcmp(cmd_name, SAMRFT_CMD_SEEKVOL) == 0) {
		int block;

		block = getInteger(&lasts);

		rc = SeekVol(cli, block);

		SendReply(cli, "%s %d %d", SAMRFT_CMD_SEEKVOL, rc, errno);

	} else if (strcmp(cmd_name, SAMRFT_CMD_UNLOADVOL) == 0) {
		struct sam_ioctl_rmunload unload;

		/*
		 * Create sam_rmunload structure used to unload
		 * a removable-media file.
		 */
		(void) memset(&unload, 0, sizeof (unload));

		unload.flags = getInteger(&lasts);

		rc = UnloadVol(cli, &unload);

		SendReply(cli, "%s %d %d %lld", SAMRFT_CMD_UNLOADVOL,
		    rc, errno, unload.position);
	} else {
		Trace(TR_ERR, "Unknown RFT command: %s", cmd_name);
	}
}

/*
 * Extract string token from command message.
 */
static char *
getString(
	char **lasts)
{
	char *string;

/* LINTED improper pointer/integer combination */
	string = strtok_r(NULL, " ", lasts);

	return (string);
}

/*
 * Extract integer token from command message.
 */
static int
getInteger(
	char **lasts)
{
	char *str_num;
	int value;

/* LINTED improper pointer/integer combination */
	str_num = strtok_r(NULL, " ", lasts);
	value = strtol(str_num, (char **)NULL, 10);

	return (value);
}

/*
 * Extract long long token from command message.
 */
static long long
getLongLong(
	char **lasts)
{
	char *str_num;
	long long value;

/* LINTED improper pointer/integer combination */
	str_num = strtok_r(NULL, " ", lasts);
	value = strtoll(str_num, (char **)NULL, 10);

	return (value);
}
