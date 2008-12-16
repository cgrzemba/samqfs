/*
 *  command.c -
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.18 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/sam_malloc.h"
#include "aml/sam_rft.h"

/* Local headers. */
#include "rft_defs.h"

#define	RFT_BUFSIZ	10240	/* FIXME - configuration */
				/* careful - multiple threads */

static boolean_t verifyReply(char *expected, char *got);
static char *getString(char **lasts);
static int getInteger(char **lasts);
static long long getLongLong(char **lasts);

void
SendCommand(
	SamrftImpl_t *rftd,
	const char *fmt,
	...)
{
	char *msg;
	va_list args;

	msg = rftd->cmdbuf;
	va_start(args, fmt);
	vsprintf(msg, fmt, args);
	va_end(args);

	fputs(rftd->cmdbuf, rftd->cout);

	fprintf(rftd->cout, "\n");
	fflush(rftd->cout);
}

/*
 * Wait for reply from rft server.  Reply is received
 * into a command buffer.
 */
int
GetReply(
	SamrftImpl_t *rftd)
{
	char *cs;
	char *msg;
	int n;
	size_t nleft;

	ASSERT(rftd->cmdbuf != NULL);
	ASSERT(rftd->cmdsize > 0);

	msg = rftd->cmdbuf;
	nleft = rftd->cmdsize;


	while (nleft > 0) {

		n = recv(fileno(rftd->cin), msg, rftd->cmdsize, 0);
		if (n <= 0) {
			Trace(TR_RFT, "Samrft [%d] reply failed %d",
			    fileno(rftd->cin), errno);
			return (-1);
		}

		nleft -= n;
		msg += n;

		cs = strpbrk(rftd->cmdbuf, "\n");
		if (cs != NULL) {
			*cs++ = '\0';
			break;
		}
	}

	return (0);
}

/*
 * Get reply from rft server for connection
 * to remote host.
 */
int
GetConnectReply(
	SamrftImpl_t *rftd,
	int *error)
{
	int rc;
	char *lasts;
	char *cmd_name;
	char *str_num;
	char *msg;

	msg = rftd->cmdbuf;

	/*
	 * Verify command name.
	 */
	cmd_name =  (char *)strtok_r(msg, " ", &lasts);
	if (verifyReply(SAMRFT_CMD_CONNECT, cmd_name) == B_FALSE) {
		*error = EFAULT;
		return (-1);
	}

	rc = getInteger(&lasts);

	str_num = (char *)strtok_r(NULL, " ", &lasts);
	*error = strtol(str_num, (char **)NULL, 10);

	return (rc);
}

/*
 * Get reply from rft server for initializing data connection
 * on remote host.
 */
int
GetDPortReply(
	SamrftImpl_t *rftd,
	int *error)
{
	int rc;
	char *lasts;
	char *msg;
	char *cmd_name;

	msg = rftd->cmdbuf;

	/*
	 * Command name.
	 */
	cmd_name = (char *)strtok_r(msg, " ", &lasts);
	if (verifyReply(SAMRFT_CMD_DPORT6, cmd_name) == B_FALSE) {
		*error = EFAULT;
		return (-1);
	}

	rc = getInteger(&lasts);
	*error = getInteger(&lasts);

	return (rc);
}

/*
 * Get reply from rft server for opening a file
 * on remote host.
 */
int
GetOpenReply(
	SamrftImpl_t *rftd,
	int *error)
{
	int rc;
	char *lasts;
	char *msg;
	char *str_num;
	char *cmd_name;

	msg = rftd->cmdbuf;

	/*
	 * Command name.
	 */
	cmd_name = (char *)strtok_r(msg, " ", &lasts);
	if (verifyReply(SAMRFT_CMD_OPEN, cmd_name) == B_FALSE) {
		*error = EFAULT;
		return (-1);
	}

	str_num = (char *)strtok_r(NULL, " ", &lasts);
	rc = strtol(str_num, (char **)NULL, 10);

	str_num = (char *)strtok_r(NULL, " ", &lasts);
	*error = strtol(str_num, (char **)NULL, 10);

	return (rc);
}

/*
 * Get reply from rft server for receiving data from file
 * on remote host.
 */
int
GetRecvReply(
	SamrftImpl_t *rftd,
	int *error)
{
	int rc;
	char *lasts;
	char *msg;
	char *str_num;
	char *cmd_name;

	msg = rftd->cmdbuf;

	/*
	 * Command name.
	 */
	cmd_name = (char *)strtok_r(msg, " ", &lasts);
	if (verifyReply(SAMRFT_CMD_RECV, cmd_name) == B_FALSE) {
		*error = EFAULT;
		return (-1);
	}

	str_num = (char *)strtok_r(NULL, " ", &lasts);
	rc = strtol(str_num, (char **)NULL, 10);

	str_num = (char *)strtok_r(NULL, " ", &lasts);
	*error = strtol(str_num, (char **)NULL, 10);

	return (rc);
}

/*
 * Get reply from rft server for sending data to file
 * on remote host.
 */
int
GetSendReply(
	SamrftImpl_t *rftd,
	int *error)
{
	int rc;
	char *lasts;
	char *msg;
	char *str_num;
	char *cmd_name;

	msg = rftd->cmdbuf;

	/*
	 * Command name.
	 */
	cmd_name = (char *)strtok_r(msg, " ", &lasts);
	if (verifyReply(SAMRFT_CMD_SEND, cmd_name) == B_FALSE) {
		*error = EFAULT;
		return (-1);
	}

	str_num = (char *)strtok_r(NULL, " ", &lasts);
	rc = strtol(str_num, (char **)NULL, 10);

	str_num = (char *)strtok_r(NULL, " ", &lasts);
	*error = strtol(str_num, (char **)NULL, 10);

	return (rc);
}

/*
 * Get reply from rft server for storing a file
 * on remote host.
 */
int
GetStorReply(
	SamrftImpl_t *rftd,
	int *error)
{
	int rc;
	char *lasts;
	char *msg;
	char *str_num;
	char *cmd_name;

	msg = rftd->cmdbuf;

	/*
	 * Command name.
	 */

	cmd_name = (char *)strtok_r(msg, " ", &lasts);
	if (verifyReply(SAMRFT_CMD_STOR, cmd_name) == B_FALSE) {
		*error = EFAULT;
		return (-1);
	}

	str_num = (char *)strtok_r(NULL, " ", &lasts);
	rc = strtol(str_num, (char **)NULL, 10);

	str_num = (char *)strtok_r(NULL, " ", &lasts);
	*error = strtol(str_num, (char **)NULL, 10);

	return (rc);
}


/*
 * Get reply from rft server for configuration
 * from remote host.
 */
int
GetConfigReply(
	SamrftImpl_t *rftd,
	int *num_dataports,
	int *dataportsize,
	int *tcpwindowsize)
{
	int rc;
	char *lasts;
	char *cmd_name;
	char *msg;

	msg = rftd->cmdbuf;

	/*
	 * Verify command name.
	 */
	cmd_name =  (char *)strtok_r(msg, " ", &lasts);
	if (verifyReply(SAMRFT_CMD_CONFIG, cmd_name) == B_FALSE) {
		return (-1);
	}

	rc = getInteger(&lasts);
	*num_dataports = getInteger(&lasts);
	*dataportsize = getInteger(&lasts);
	*tcpwindowsize = getInteger(&lasts);

	return (rc);
}

/*
 * Get reply from rft server for check if mount point
 * on remote host is available.
 */
int
GetIsMountedReply(
	SamrftImpl_t *rftd)
{
	int mounted;
	char *lasts;
	char *msg;
	char *str_num;
	char *cmd_name;

	mounted = 0;
	msg = rftd->cmdbuf;

	/*
	 * Command name.
	 */
	cmd_name = (char *)strtok_r(msg, " ", &lasts);
	if (verifyReply(SAMRFT_CMD_ISMOUNTED, cmd_name) == B_FALSE) {
		return (-1);
	}

	str_num = (char *)strtok_r(NULL, " ", &lasts);
	mounted = strtol(str_num, (char **)NULL, 10);

	return (mounted);
}


/*
 * Get reply from rft server for information for file
 * on remote host.
 */
int
GetStatReply(
	SamrftImpl_t *rftd,
	SamrftStatInfo_t *buf,
	int *error)
{
	int rc;
	char *lasts;
	char *msg;
	char *cmd_name;

	memset(buf, 0, sizeof (SamrftStatInfo_t));

	msg = rftd->cmdbuf;

	/*
	 * Command name.
	 */
	cmd_name = (char *)strtok_r(msg, " ", &lasts);
	if (verifyReply(SAMRFT_CMD_STAT, cmd_name) == B_FALSE) {
		*error = EFAULT;
		return (-1);
	}

	rc = getInteger(&lasts);
	*error = getInteger(&lasts);

	if (rc == 0) {
		buf->mode = getInteger(&lasts);
		buf->uid  = getInteger(&lasts);
		buf->gid  = getInteger(&lasts);
		buf->size = getLongLong(&lasts);
	}
	return (rc);
}


/*
 * Get reply from rft server for information for file system
 * on remote host.
 */
int
GetStatvfsReply(
	SamrftImpl_t *rftd,
	struct statvfs64 *buf,
	int *error)
{
	int rc;
	char *lasts;
	char *msg;
	char *basetype;
	char *cmd_name;

	memset(buf, 0, sizeof (struct statvfs64));

	msg = rftd->cmdbuf;

	/*
	 * Command name.
	 */
	cmd_name = (char *)strtok_r(msg, " ", &lasts);
	if (verifyReply(SAMRFT_CMD_STATVFS, cmd_name) == B_FALSE) {
		*error = EFAULT;
		return (-1);
	}

	rc = getInteger(&lasts);
	*error = getInteger(&lasts);

	if (rc == 0) {
		buf->f_bfree = getLongLong(&lasts);
		buf->f_blocks = getLongLong(&lasts);
		buf->f_frsize = getInteger(&lasts);
		basetype = getString(&lasts);
		strcpy(buf->f_basetype, basetype);
	}
	return (rc);
}


/*
 * Get reply from rft server for space used for path on remote host.
 */
int
GetSpaceUsedReply(
	SamrftImpl_t *rftd,
	fsize_t *used,
	int *error)
{
	int rc;
	char *lasts;
	char *msg;
	char *cmd_name;

	*used = 0;

	msg = rftd->cmdbuf;

	/*
	 * Command name.
	 */
	cmd_name = (char *)strtok_r(msg, " ", &lasts);
	if (verifyReply(SAMRFT_CMD_SPACEUSED, cmd_name) == B_FALSE) {
		*error = EFAULT;
		return (-1);
	}

	rc = getInteger(&lasts);
	*error = getInteger(&lasts);

	if (rc == 0) {
		*used = getLongLong(&lasts);
	}
	return (rc);
}


/*
 * Get reply from rft server for opening a directory on remote host.
 */
int
GetOpendirReply(
	SamrftImpl_t *rftd,
	int *dirp,
	int *error)
{
	int rc;
	char *lasts;
	char *msg;
	char *cmd_name;

	msg = rftd->cmdbuf;

	/*
	 * Command name.
	 */
	cmd_name = (char *)strtok_r(msg, " ", &lasts);
	if (verifyReply(SAMRFT_CMD_OPENDIR, cmd_name) == B_FALSE) {
		*error = EFAULT;
		return (-1);
	}

	rc = getInteger(&lasts);
	*error = getInteger(&lasts);
	*dirp = getInteger(&lasts);

	return (rc);
}


/*
 * Get reply from rft server for seek a file on remote host.
 */
int
GetSeekReply(
	SamrftImpl_t *rftd,
	off64_t *offset,
	int *error)
{
	int rc;
	char *lasts;
	char *msg;
	char *cmd_name;

	msg = rftd->cmdbuf;

	/*
	 * Command name.
	 */
	cmd_name = (char *)strtok_r(msg, " ", &lasts);
	if (verifyReply(SAMRFT_CMD_SEEK, cmd_name) == B_FALSE) {
		*error = EFAULT;
		return (-1);
	}

	rc = getInteger(&lasts);
	*error = getInteger(&lasts);

	if (rc == 0) {
		*offset = getLongLong(&lasts);
	}

	return (rc);
}


/*
 * Get reply from rft server for reading a directory on remote host.
 */
int
GetReaddirReply(
	SamrftImpl_t *rftd,
	SamrftReaddirInfo_t *dir_info,
	int *error)
{
	int rc;
	char *lasts;
	char *msg;
	char *cmd_name;
	char *name;

	msg = rftd->cmdbuf;

	/*
	 * Command name.
	 */
	cmd_name = (char *)strtok_r(msg, " ", &lasts);
	if (verifyReply(SAMRFT_CMD_READDIR, cmd_name) == B_FALSE) {
		*error = EFAULT;
		return (-1);
	}

	rc = getInteger(&lasts);
	*error = getInteger(&lasts);

	if (rc == 0) {
		name = getString(&lasts);
		SamStrdup(dir_info->name, name);
		dir_info->isdir = getInteger(&lasts);
	}

	return (rc);
}


/*
 * Get reply from rft server for making a directory on remote host.
 */
int
GetMkdirReply(
	SamrftImpl_t *rftd,
	int *error)
{
	int rc;
	char *lasts;
	char *msg;
	char *cmd_name;

	msg = rftd->cmdbuf;

	/*
	 * Command name.
	 */
	cmd_name = (char *)strtok_r(msg, " ", &lasts);
	if (verifyReply(SAMRFT_CMD_MKDIR, cmd_name) == B_FALSE) {
		*error = EFAULT;
		return (-1);
	}

	rc = getInteger(&lasts);
	*error = getInteger(&lasts);

	return (rc);
}

/*
 * Get reply from rft server for archive operations on remote host.
 */
int
GetArchiveOpReply(
	SamrftImpl_t *rftd,
	int *error)
{
	int rc;
	char *lasts;
	char *msg;
	char *cmd_name;

	msg = rftd->cmdbuf;

	/*
	 * Command name.
	 */
	cmd_name = (char *)strtok_r(msg, " ", &lasts);
	if (verifyReply(SAMRFT_CMD_ARCHIVEOP, cmd_name) == B_FALSE) {
		*error = EFAULT;
		return (-1);
	}

	rc = getInteger(&lasts);
	*error = getInteger(&lasts);

	return (rc);
}


/*
 * Get reply from rft server for loading removable media volume
 * on remote host.
 */
int
GetLoadVolReply(
	SamrftImpl_t *rftd,
	int *error)
{
	int rc;
	char *lasts;
	char *msg;
	char *cmd_name;

	msg = rftd->cmdbuf;

	/*
	 * Command name.
	 */
	cmd_name = (char *)strtok_r(msg, " ", &lasts);
	if (verifyReply(SAMRFT_CMD_LOADVOL, cmd_name) == B_FALSE) {
		*error = EFAULT;
		return (-1);
	}

	rc = getInteger(&lasts);
	*error = getInteger(&lasts);

	return (rc);
}


/*
 * Get reply from rft server for get removable media volume information
 * on remote host.
 */
int
GetVolInfoReply(
	SamrftImpl_t *rftd,
	struct sam_rminfo *getrm,
	int *eq,
	int *error)
{
	int rc;
	char *lasts;
	char *msg;
	char *cmd_name;

	msg = rftd->cmdbuf;

	/*
	 *	Command name.
	 */
	cmd_name = (char *)strtok_r(msg, " ", &lasts);
	if (verifyReply(SAMRFT_CMD_GETVOLINFO, cmd_name) == B_FALSE) {
		*error = EFAULT;
		return (-1);
	}

	rc = getInteger(&lasts);
	*error = getInteger(&lasts);

	memset(getrm, 0, sizeof (struct sam_rminfo));
	getrm->block_size = getInteger(&lasts);
	getrm->position   = getInteger(&lasts);

	*eq = getInteger(&lasts);

	return (rc);
}


/*
 * Get reply from rft server for positioning removable media volume
 * on remote host.
 */
int
GetSeekVolReply(
	SamrftImpl_t *rftd,
	int *error)
{
	int rc;
	char *lasts;
	char *msg;
	char *cmd_name;

	msg = rftd->cmdbuf;

	/*
	 * Command name.
	 */
	cmd_name = (char *)strtok_r(msg, " ", &lasts);
	if (verifyReply(SAMRFT_CMD_SEEKVOL, cmd_name) == B_FALSE) {
		*error = EFAULT;
		return (-1);
	}

	rc = getInteger(&lasts);
	*error = getInteger(&lasts);

	return (rc);
}


/*
 * Get reply from rft server for unloading removable media volume
 * on remote host.
 */
int
GetUnloadVolReply(
	SamrftImpl_t *rftd,
	uint64_t *position,
	int *error)
{
	int rc;
	char *lasts;
	char *msg;
	char *cmd_name;

	msg = rftd->cmdbuf;

	/*
	 * Command name.
	 */
	cmd_name = (char *)strtok_r(msg, " ", &lasts);
	if (verifyReply(SAMRFT_CMD_UNLOADVOL, cmd_name) == B_FALSE) {
		*error = EFAULT;
		return (-1);
	}

	rc = getInteger(&lasts);
	*error = getInteger(&lasts);
	*position = getInteger(&lasts);

	return (rc);
}

/*
 * Verify rft server reply is the one expected.
 */
static boolean_t
verifyReply(
	char *expected,
	char *got)
{
	boolean_t rc = B_TRUE;

	if ((got == NULL) || (strcmp(expected, got) != 0)) {
		Trace(TR_ERR, "Samrft invalid reply");
		rc = B_FALSE;
	}
	return (rc);
}

/*
 * Extract string token from command message.
 */
static char *
getString(
	char **lasts)
{
	char *string;

	string = (char *)strtok_r(NULL, " ", lasts);

	return (string);
}

/*
 * Extract integer token from command messages.
 */
static int
getInteger(
	char **lasts)
{
	char *str_num;
	int value;

	str_num = (char *)strtok_r(NULL, " ", lasts);
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

	str_num = (char *)strtok_r(NULL, " ", lasts);
	value = strtoll(str_num, (char **)NULL, 10);

	return (value);
}
