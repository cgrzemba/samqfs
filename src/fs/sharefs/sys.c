/*
 *	sys.c - config support functions for the filesystem.
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

#pragma ident "$Revision: 1.31 $"

static char *_SrcFile = __FILE__;
/* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

/* POSIX headers. */
#include <unistd.h>

/* Solaris headers. */
#include <stropts.h>

/* Socket headers. */
#include <sys/socket.h>
#include <netinet/tcp.h>

/* SAM-FS headers. */
#include "sam/custmsg.h"
#include "sam/lib.h"
#include "sam/quota.h"
#include "sam/syscall.h"
#include "sam/sam_trace.h"


/* Local headers. */
#include "share.h"

/* Library interface (src/lib/samconf/sam_sycall.c) */
extern int sam_syscall(int cmd, void *arg, int size);

/* Private functions  */

/*
 * These functions all increment host ordinals by 1.
 * The hosts file and user space stuff use 0-based host
 * ordinals.  The kernel uses 1-based ordinals, so we
 * increment in here to keep things consistent.
 */


/*
 * Notify file system of the server's identity
 * (and whether or not it's the local host).
 */
int
SetClient(
	char *fs,
	char *server,
	int ord,
	uint64_t server_tags,
	int amserver)
{
	int error = 0;
	struct sam_set_host ss;

	Trace(TR_MISC, "FS %s: Set Client (Server %s/%d) tags=%x.",
	    fs, server, ord + 1, server_tags);

	bzero(&ss, sizeof (ss));
	strncpy(ss.fs_name, fs, sizeof (ss.fs_name));
	strncpy(ss.server, server, sizeof (ss.server));
	ss.ord = ord + 1;
	ss.maxord = amserver;		/* 1 => server; 0 => client */
	ss.server_tags = server_tags;
	if (sam_syscall(SC_set_client, (void *) &ss, sizeof (ss)) < 0) {
		error = errno;
	}
	return (error);
}


/*
 * Connect a TCP connection to the kernel.
 *
 * The connection is used for both reading and writing,
 * but we provide buffers for the kernel to use for reading.
 * The calling thread is used by the kernel for reading.
 * sam_san_max_message_t is the maximum size of a socket message.
 * This user buffer is used in the filesystem for the VOP_READ()
 * because UIO_SYSSPACE is not always supported for VOP_READ()
 * of sockets.
 *
 * Normal return is the errno returned by the rdsock syscall
 * (which always returns -1).  Error return is -1.
 */
static int
setSocket(
	int syscall,
	char *fs,
	char *host,
	int ord,
	int fd,
	uint64_t tags,
	int flags)
{
	struct sam_syscall_rdsock rdsock;
	sam_san_max_message_t *msg = NULL;
	int error = 0;
	int size;
	extern int errno;

#ifdef linux
	size = sizeof (sam_san_max_message_t) * 2;
	if ((msg = (sam_san_max_message_t *)valloc(size)) == NULL) {
		errno = 0;
		SysError(HERE, "%s valloc[setSocket] failed", fs);
		return (-1);
	}
	bzero((char *)msg, size);
#if LOCK_BUFFERS
	if (mlock((char *)msg, size) != 0) {
		SysError(HERE, "%s mlock[setSocket] failed", fs);
		free(msg);
		return (-1);
	}
#endif	/* LOCK_BUFFERS */
#endif	/* linux */

	strncpy(rdsock.fs_name, fs, sizeof (rdsock.fs_name));
	strncpy(rdsock.hname, host, sizeof (rdsock.hname));
	rdsock.hostord = ord;
	rdsock.sockfd = fd;
	rdsock.flags = flags;
	rdsock.msg.ptr = msg;		/* if NULL, use kernel buffers */
	rdsock.tags = tags;

	Trace(TR_MISC, "FS %s: rdsock %s/%d tags=%x (buf=%p).",
	    fs, host, ord, tags, (void *)msg);
	if (sam_syscall(syscall, (void *) &rdsock, sizeof (rdsock)) < 0) {
		error = errno;
	}

#ifdef linux
#if LOCK_BUFFERS
	if (munlock((char *)msg, size) != 0) {
		SysError(HERE, "FS %s: munlock[setSocket] failed", fs);
		free(msg);
		return (-1);
	}
#endif	/* LOCK_BUFFERS */

	free(msg);
#endif	/* linux */
	return (error);
}


/*
 * Connect TCP socket to client kernel.
 */
int
SetClientSocket(
	char *fs,
	char *host,
	int fd,
	int flags)
{
	Trace(TR_MISC, "FS %s: SetClientSocket %s (flags=%x)",
	    fs, host, flags);
	return (setSocket(SC_client_rdsock, fs, host, 0, fd, 0, flags));
}


/*
 * Configure the socket to avoid congestion control.
 */
void
ConfigureSocket(
	int sockfd,
	char *fs)
{
	const int on = 1;
	const int timeout = 100;

	/*
	 * Set TCP_NODELAY option to avoid congestion control.
	 */
	if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &on,
	    sizeof (on)) < 0) {
		SysError(HERE, "FS %s: setsockopt TCP_NODELAY failed, "
		    "continuing", fs);
	}

#ifdef sun
	if (ioctl(sockfd, I_SETCLTIME, &timeout) < 0) {
		SysError(HERE, "FS %s: ioctl I_SETCLTIME failed, "
		    "continuing", fs);
	}
#endif /* sun */
}


#ifdef	METADATA_SERVER
/*
 * Notify file system that we are initiating a voluntary failover
 * (The caller must be the current FS server.)
 */
int
SysFailover(
	char *fs,
	char *server)
{
	int error = 0;
	struct sam_set_host ss;

	Trace(TR_MISC, "FS %s: Failover (Server %s).", fs, server);

	bzero(&ss, sizeof (ss));
	strncpy(ss.fs_name, fs, sizeof (ss.fs_name));
	strncpy(ss.server, server, sizeof (ss.server));
	ss.ord = 0;
	ss.maxord = 0;
	if (sam_syscall(SC_failover, (void *) &ss, sizeof (ss)) < 0) {
		error = errno;
	}
	return (error);
}


/*
 * Notify file system that this host is the server.
 */
int
SetServer(
	char *fs,
	char *server,
	int ord,
	int maxord)
{
	int error = 0;
	struct sam_set_host ss;

	Trace(TR_MISC, "FS %s: Set Server %s; %d/%d.",
	    fs, server, ord + 1, maxord);

	bzero(&ss, sizeof (ss));
	strncpy(ss.fs_name, fs, sizeof (ss.fs_name));
	strncpy(ss.server, server, sizeof (ss.server));
	ss.ord = ord + 1;
	ss.maxord = maxord;
	if (sam_syscall(SC_set_server, (void *) &ss, sizeof (ss)) < 0) {
		error = errno;
	}
	return (error);
}


/*
 * Connect TCP socket to server kernel.
 */
int
SetServerSocket(
	char *fs,
	char *host,
	int ord,
	int fd,
	uint64_t tags,
	int flags)
{
	int ret;

	Trace(TR_MISC, "FS %s: SetServerSocket %s/%d tags=%x (flags=%x)",
	    fs, host, ord+1, tags, flags);
	ret = setSocket(SC_server_rdsock, fs, host, ord+1, fd, tags, flags);
	return (ret);
}
#endif	/* METADATA_SERVER */
