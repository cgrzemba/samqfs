/*
 * api.c - interfaces to sam rft library
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

#pragma ident "$Revision: 1.24 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

/* SAM-FS headers. */
#include "pub/lib.h"
#include "sam/fioctl.h"
#include "sam/syscall.h"
#include "sam/lib.h"
#include "sam/types.h"
#include "sam/sam_malloc.h"
#include "aml/sam_rft.h"
#include "aml/diskvols.h"
#include "sam/spm.h"

/* Local headers. */
#include "rft_defs.h"

/* Private functions. */
static SamrftImpl_t *connectToServer(char *host, int attempts);
static void disconnectFromServer(SamrftImpl_t *rftd);
static void initCmdPort(SamrftImpl_t *rftd);
static void cleanupCmdPort(SamrftImpl_t *rftd);
static int initDataPorts(SamrftImpl_t *rftd);
static void cleanupDataPorts(SamrftImpl_t *rftd);
static SamrftImpl_t *initRftImpl();
static int traceRftd(SamrftImpl_t *rftd);

static int openLocalFile(SamrftImpl_t *rftd, char *filename, int oflag,
	SamrftCreateAttr_t *creat);

/*
 * Establish connection to remote host.
 */
SamrftImpl_t *
SamrftConnect(
	char *host)
{
	SamrftImpl_t *rftd;

	if (host == NULL || host[0] == '\0' || strcmp(host, "localhost") == 0) {
		rftd = initRftImpl();

	} else {
		Trace(TR_RFT, "Samrft connect host '%s'", host);

		rftd = connectToServer(host, 3);
	}

	if (rftd) {
		Trace(TR_RFT, "Samrft [%d] connect complete", traceRftd(rftd));
		Trace(TR_RFT, "Samrft [%d] flags %x af %d", traceRftd(rftd),
		    rftd->flags, rftd->caddr.cad.sin_family);
	} else {
		Trace(TR_ERR, "Samrft connection to '%s' failed errno: %d",
		    host, errno);
	}

	return (rftd);
}


/*
 * Open a disk file on remote host.
 */
int
SamrftOpen(
	SamrftImpl_t *rftd,
	char *filename,
	int oflag,
	SamrftCreateAttr_t *creat)
{
	int rc;
	int error;

	Trace(TR_RFT, "Samrft [%d] open file %s %d %x",
	    traceRftd(rftd), filename, oflag, creat);

	/*
	 * If first open following connect, initialize data ports.
	 */
	if (rftd->crew == NULL) {
		if (initDataPorts(rftd) < 0) {
			return (-1);
		}
	}

	if (rftd->remotehost) {
		if (creat) {
			SendCommand(rftd, "%s %s %d %d %d %d %d",
			    SAMRFT_CMD_OPEN, filename, oflag,
			    TRUE, creat->mode, creat->uid, creat->gid);
		} else {
			SendCommand(rftd, "%s %s %d %d",
			    SAMRFT_CMD_OPEN, filename, oflag,
			    FALSE);
		}
		if (GetReply(rftd) >= 0) {
			rc = GetOpenReply(rftd, &error);
			if (rc < 0) {
				SetErrno = error;
			}
		}
	} else {
		/*
		 * Open file on local machine.
		 */
		rc = openLocalFile(rftd, filename, oflag, creat);
	}

	return (rc);
}


/*
 * Prepare to store file on remote host.
 */
int
SamrftStore(
	SamrftImpl_t *rftd,
	fsize_t nbytes)
{
	Trace(TR_RFT, "Samrft [%d] store %lld", fileno(rftd->cin), nbytes);

	/*
	 * Store command.  Used by client as indicator to start
	 * reading from the data socket.
	 */
	SendCommand(rftd, "%s %lld", SAMRFT_CMD_STOR, nbytes);

	return (0);
}


/*
 * Write to file on remote host.
 */
size_t
SamrftWrite(
	SamrftImpl_t *rftd,
	void *buf,
	size_t nbytes)
{
	int rc;
	int error;
	size_t bytes_written;

#if 0
	Trace(TR_RFT, "Samrft [%d] write %d", fileno(rftd->cin), nbytes);
#endif

	if (rftd->remotehost) {
		/*
		 * Send command.  Used as information by client to
		 * start reading data sockets.  A reply is not expected until
		 * the data transfer has completed.
		 */
		SendCommand(rftd, "%s %d", SAMRFT_CMD_SEND, nbytes);

		rc = SendData(rftd, buf, nbytes);

		if (GetReply(rftd) >= 0) {
			rc = GetSendReply(rftd, &error);
			if (rc < 0) {
				if (error == 0) {
					/* Short write failure. */
					SetErrno = ENOSPC;
				} else {
					SetErrno = error;
				}
				bytes_written = (long)-1;
			} else {
				bytes_written = nbytes;
			}
		}
	} else {
		/*
		 * Write to local file.
		 */
		bytes_written = write(rftd->fd, buf, nbytes);
		if (bytes_written != nbytes && errno == 0) {
			/* Short write failure. */
			SetErrno = ENOSPC;
		}
	}

#if 0
	Trace(TR_RFT, "Samrft [%d] write complete %d",
	    traceRftd(rftd), bytes_written);
#endif
	return (bytes_written);
}


/*
 * Write to file on remote host.
 */
size_t
SamrftSend(
	SamrftImpl_t *rftd,
	void *buf,
	size_t nbytes)
{
	(void) SendData(rftd, buf, nbytes);
	return (nbytes);
}


/*
 *	Read from file on remote host.
 */
size_t
SamrftRead(
	SamrftImpl_t *rftd,
	void *buf,
	size_t nbytes)
{
	int rc;
	int error;
	size_t bytes_read;

#if 0
	Trace(TR_RFT, "Samrft [%d] read %d", traceRftd(rftd), nbytes);
#endif

	if (rftd->remotehost) {

		SendCommand(rftd, "%s %d", SAMRFT_CMD_RECV, nbytes);

		rc = ReceiveData(rftd, buf, nbytes);

		if (GetReply(rftd) >= 0) {
			rc = GetRecvReply(rftd, &error);
			if (rc < 0) {
				SetErrno = error;
				bytes_read = (long)-1;
			} else {
				bytes_read = nbytes;
			}
		}
	} else {
		/*
		 * Read data from local file.
		 */
		bytes_read = read(rftd->fd, buf, nbytes);
	}

#if 0
	Trace(TR_RFT, "Samrft [%d] read complete %d",
	    traceRftd(rftd), bytes_read);
#endif

	return (bytes_read);
}


/*
 * Close file on remote host.
 */
int
SamrftClose(
	SamrftImpl_t *rftd)
{
	int rc;

#if 0
	Trace(TR_RFT, "Samrft [%d] close file", traceRftd(rftd));
#endif

	if (rftd->remotehost) {

		SendCommand(rftd, "%s", SAMRFT_CMD_CLOSE);
		rc = GetReply(rftd);

	} else {
		close(rftd->fd);
		rftd->fd = -1;
		rc = 0;
	}

	return (rc);
}


/*
 * Close connection to remote host.
 */
void
SamrftDisconnect(
	SamrftImpl_t *rftd)
{
	if (rftd == NULL) {
		return;
	}

	Trace(TR_RFT, "Samrft [%d] disconnect", traceRftd(rftd));

	if (rftd->remotehost) {
		disconnectFromServer(rftd);
	}
	SamFree(rftd);
}

/*
 * Check if mount point on remote host is available.
 */
int
SamrftIsMounted(
	SamrftImpl_t *rftd,
	char *mount_point)
{
	int mounted = 0;

	SendCommand(rftd, "%s %s", SAMRFT_CMD_ISMOUNTED, mount_point);
	if (GetReply(rftd) >= 0) {
		mounted = GetIsMountedReply(rftd);
	}
	return (mounted);
}

/*
 * Get information for file on remote host.
 */
int
SamrftStat(
	SamrftImpl_t *rftd,
	char *filename,
	SamrftStatInfo_t *buf)
{
	int error;
	int rc = -1;

	if (rftd->remotehost) {
		SendCommand(rftd, "%s %s", SAMRFT_CMD_STAT, filename);
		if (GetReply(rftd) >= 0) {
			rc = GetStatReply(rftd, buf, &error);
		}
	} else {
		struct stat64 sb;

		rc = stat64(filename, &sb);
		if (rc == 0) {
			buf->mode = sb.st_mode;
			buf->uid  = sb.st_uid;
			buf->gid  = sb.st_gid;
			buf->size = sb.st_size;
		}
	}
	return (rc);
}

/*
 * Get information for file system on remote host.  Set offline flag
 * to include the size of all SAM-FS offline files in the capacity.
 */
int
SamrftStatvfs(
	SamrftImpl_t *rftd,
	char *path,
	boolean_t offlineFiles,
	struct statvfs64 *buf)
{
	int error;
	int rc = -1;

	if (rftd->remotehost) {
		SendCommand(rftd, "%s %s %d", SAMRFT_CMD_STATVFS,
		    path, offlineFiles);
		if (GetReply(rftd) >= 0) {
			rc = GetStatvfsReply(rftd, buf, &error);
		}
	} else {
		fsize_t offlineFileSize;

		rc = statvfs64(path, buf);
		if (rc == 0 && (strcmp(buf->f_basetype, "samfs") == 0) &&
		    offlineFiles == B_TRUE) {
			offlineFileSize = DiskVolsOfflineFiles(path);
			/*
			 * Adjust capacity to include size of offline files.
			 */
			buf->f_blocks += offlineFileSize / buf->f_frsize;
		}
	}
	return (rc);
}

/*
 * Accumulate space used for selected path.  Space used is calculated
 * by walking the entire path and stat each file.  This method will
 * accumulate space used for online and offline files.
 */
int
SamrftSpaceUsed(
	SamrftImpl_t *rftd,
	char *path,
	fsize_t *used)
{
	int error;
	int rc = -1;

	if (rftd->remotehost) {
		SendCommand(rftd, "%s %s", SAMRFT_CMD_SPACEUSED, path);
		if (GetReply(rftd) >= 0) {
			rc = GetSpaceUsedReply(rftd, used, &error);
		}
	} else {
		*used = DiskVolsAccumSpaceUsed(path);
	}
	return (rc);
}

/*
 * Make directory on remote host.
 */
int
SamrftMkdir(
	SamrftImpl_t *rftd,
	char *dirname,
	SamrftCreateAttr_t *creat)
{
	int rc = 0;
	int error;

	if (rftd->remotehost) {

		SendCommand(rftd, "%s %s %d %d %d", SAMRFT_CMD_MKDIR, dirname,
		    creat->mode, creat->uid, creat->gid);
		if (GetReply(rftd) >= 0) {
			rc = GetMkdirReply(rftd, &error);
			if (rc < 0) {
				SetErrno = error;
			}
		}
	} else {

		if (mkdir(dirname, (creat->mode & S_IAMB)) == 0) {
			/*
			 * If creating a directory on an NFS mount point,
			 * daemon does not have permission to change the
			 * ownership. Ignore chown errors.
			 */
			(void) chown(dirname, creat->uid, creat->gid);
		} else {
			rc = -1;
		}
	}
	return (rc);
}

/*
 * Open directory on remote host.
 */
int
SamrftOpendir(
	SamrftImpl_t *rftd,
	char *dirname,
	int *dirp)
{
	int rc = -1;
	int error;

	SendCommand(rftd, "%s %s", SAMRFT_CMD_OPENDIR, dirname);
	if (GetReply(rftd) >= 0) {
		rc = GetOpendirReply(rftd, dirp, &error);
		if (rc < 0) {
			SetErrno = error;
		}
	}

	return (rc);
}

/*
 * Read directory on remote host.
 */
int
SamrftReaddir(
	SamrftImpl_t *rftd,
	int dirp,
	SamrftReaddirInfo_t *dir_info)
{
	int rc = -1;
	int error;

	SendCommand(rftd, "%s %d", SAMRFT_CMD_READDIR, dirp);
	if (GetReply(rftd) >= 0) {
		rc = GetReaddirReply(rftd, dir_info, &error);
		if (rc < 0) {
			SetErrno = error;
		}
	}

	return (rc);
}

/*
 * Close directory on remote host.
 */
void
SamrftClosedir(
	SamrftImpl_t *rftd,
	int dirp)
{
	SendCommand(rftd, "%s %d", SAMRFT_CMD_CLOSEDIR, dirp);
	(void) GetReply(rftd);
}

/*
 * Unlink file or directory on local or remote host.
 */
int
SamrftUnlink(
	SamrftImpl_t *rftd,
	char *name
)
{
	int rc;

	if (rftd->remotehost) {
		SendCommand(rftd, "%s %s", SAMRFT_CMD_UNLINK, name);
		rc = GetReply(rftd);
	} else {
		rc = unlink(name);
	}

	return (rc);
}

/*
 * Remove directory on remote host.
 */
int
SamrftRmdir(
	SamrftImpl_t *rftd,
	char *dirname)
{
	int rc;

	SendCommand(rftd, "%s %s", SAMRFT_CMD_RMDIR, dirname);
	rc = GetReply(rftd);
	return (rc);
}

/*
 * Move read/write file pointer on remote host.
 */
int
SamrftSeek(
	SamrftImpl_t *rftd,
	off64_t setpos,
	int whence,
	off64_t *offset)
{
	int rc = -1;
	int error;

	if (rftd->remotehost) {
		SendCommand(rftd, "%s %lld %d",
		    SAMRFT_CMD_SEEK, setpos, whence);
		if (GetReply(rftd) >= 0) {
			rc = GetSeekReply(rftd, offset, &error);
			if (rc < 0) {
				SetErrno = error;
			}
		}
	} else {
		*offset = lseek64(rftd->fd, setpos, SEEK_SET);
		if (*offset != -1) {
			rc = 0;
		}
	}
	return (rc);
}

/*
 * Apply or remove exclusive lock on file on remote host.
 */
int
SamrftFlock(
	SamrftImpl_t *rftd,
	int type)
{
	int rc = -1;

	if (rftd->remotehost) {
		SendCommand(rftd, "%s %d", SAMRFT_CMD_FLOCK, type);
		rc = GetReply(rftd);

	} else {
		struct flock fl;

		if (type == F_UNLCK) {
			(void) fsync(rftd->fd);
		}
		memset(&fl, 0, sizeof (struct flock));
		fl.l_type |= type;
		rc = fcntl(rftd->fd, F_SETLKW, &fl);
	}
	return (rc);
}

/*
 * Set archive operations.
 */
int
SamrftArchiveOp(
	SamrftImpl_t *rftd,
	char *path,
	const char *ops)
{
	int rc = -1;
	int error;

	if (rftd->remotehost) {
		SendCommand(rftd, "%s %s %s", SAMRFT_CMD_ARCHIVEOP, path, ops);
		if (GetReply(rftd) >= 0) {
			rc = GetArchiveOpReply(rftd, &error);
			if (rc < 0) {
				SetErrno = error;
			}
		}
	} else {
		struct sam_fileops_arg arg;

		arg.path.ptr = path;
		arg.ops.ptr = ops;
		rc = sam_syscall(SC_archive, &arg, sizeof (arg));
	}
	return (rc);
}


/*
 * Load removable-media volume on remote host.  Parameter attr
 * is a pointer to a sam_rminfo structure describing information
 * about the removable-media to be loaded.
 */
int
SamrftLoadVol(
	SamrftImpl_t *rftd,
	struct sam_rminfo *attr,
	int oflag)
{
	int error;
	int rc = -1;

	Trace(TR_RFT, "Samrft [%d] load volume %s.%s %d",
	    traceRftd(rftd), attr->media, attr->section[0].vsn, oflag);

	/*
	 * If first open following connect, initialize data ports.
	 */
	if (rftd->crew == NULL) {
		if (initDataPorts(rftd) < 0) {
			return (rc);
		}
	}

	SendCommand(rftd, "%s %d %s %s %s %s %s %d", SAMRFT_CMD_LOADVOL,
	    attr->flags, attr->file_id, attr->owner_id, attr->group_id,
	    attr->media, attr->section[0].vsn, oflag);

	if (GetReply(rftd) >= 0) {
		rc = GetLoadVolReply(rftd, &error);
	}
	return (rc);
}

/*
 * Get removable-media information on remote host.
 */
int
SamrftGetVolInfo(
	SamrftImpl_t *rftd,
	struct sam_rminfo *getrm,
	int *eq)
{
	int error;
	int rc = -1;

	Trace(TR_RFT, "Samrft [%d] get volinfo", traceRftd(rftd));

	SendCommand(rftd, "%s", SAMRFT_CMD_GETVOLINFO);

	if (GetReply(rftd) >= 0) {
		rc = GetVolInfoReply(rftd, getrm, eq, &error);
	}
	return (rc);
}

/*
 * Position removable-media on remote host.
 */
int
SamrftSeekVol(
	SamrftImpl_t *rftd,
	int block)
{
	int error;
	int rc = -1;

	Trace(TR_RFT, "Samrft [%d] seek vol %d", traceRftd(rftd), block);

	SendCommand(rftd, "%s %d", SAMRFT_CMD_SEEKVOL, block);

	if (GetReply(rftd) >= 0) {
		rc = GetSeekVolReply(rftd, &error);
	}
	return (rc);
}

/*
 * Unload removable-media volume on remote host.
 */
int
SamrftUnloadVol(
	SamrftImpl_t *rftd,
	struct sam_ioctl_rmunload *unload)
{
	int error;
	int rc = -1;

	Trace(TR_RFT, "Samrft [%d] unload volume %d",
	    traceRftd(rftd), unload->flags);

	SendCommand(rftd, "%s %d", SAMRFT_CMD_UNLOADVOL, unload->flags);

	if (GetReply(rftd) >= 0) {
		rc = GetUnloadVolReply(rftd, &unload->position, &error);
	}
	return (rc);
}

/*
 *
 */
char *
SamrftGetHostByAddr(
	void *addr,
	int  af)
{
	struct hostent *host_ent;
	char *host_name = NULL;
	char buffer[32];
	int h_err = TRUE;
	int size;

	switch (af) {
	case AF_INET:
		size = sizeof (struct in_addr);
		break;
	case AF_INET6:
		size = sizeof (struct in6_addr);
		break;
	}
	host_ent = getipnodebyaddr((char *)addr, size, af, &h_err);
	if (host_ent != NULL) {
		SamStrdup(host_name, host_ent->h_name);
		freehostent(host_ent);
	} else {
		Trace(TR_ERR, "Samrft af %d address %s not found",
		    af, inet_ntop(af, (char *)addr, buffer, 32));
	}
	return (host_name);
}

/*
 * Check if connection to specified remote host name is accessible and
 * path name on the remote host exists.
 */
boolean_t
SamrftIsAccessible(
	char *host,
	char *pathname)
{
	SamrftImpl_t *rftd;
	boolean_t accessible;

	accessible = B_FALSE;

	if (host == NULL || host[0] == '\0' || strcmp(host, "localhost") == 0) {
		rftd = initRftImpl();
	} else {
		rftd = connectToServer(host, 0);
	}

	if (rftd != NULL) {
		SamrftStatInfo_t buf;

		if (SamrftStat(rftd, pathname, &buf) == 0) {
			accessible = B_TRUE;
		}
	}

	if (rftd != NULL && rftd->remotehost == B_TRUE) {
		disconnectFromServer(rftd);
		SamFree(rftd);
	}

	return (accessible);
}

/*
 * Open control connection to SAM's rft server on remote host.
 */
static SamrftImpl_t *
connectToServer(
	char *host,
	int attempts)
{
	static boolean_t infinite = B_TRUE;
	int af;
	int errcode;
	char errbuf[SPM_ERRSTR_MAX];
	int cntl_fd;
	SamrftImpl_t *rft;
	int tos;

	rft = initRftImpl();

	while (infinite) {

		cntl_fd = spm_connect_to_service(SAMRFT_SPM_SERVICE_NAME,
		    host, NULL, &errcode, errbuf);

		if (cntl_fd >= 0) {
			break;
		}

		/*
		 * Connect failed.
		 */
		Trace(TR_RFT, "Connection to host '%s' failed: %d: %s",
		    host, errcode, errbuf);

		attempts--;
		if (attempts < 0) {
			break;
		}

		/*
		 * Wait and try again.
		 */
		sleep(2);
	}

	if (cntl_fd < 0) {
		SamFree(rft);
		rft = NULL;

	} else {
		char *hostname;
		struct  sockaddr_in6 mysock;
		struct  sockaddr_in6 *mysock6;
		struct  sockaddr_in *mysock4;
		struct  sockaddr_in6 peername;
		struct  sockaddr *perp;
		int peername_len;
		int mysock_len;
		int level;

		mysock6 = &mysock;
		mysock4 = (struct sockaddr_in *)&mysock;

		perp = (struct sockaddr *)&peername;
		peername_len = sizeof (struct sockaddr_in6);

		if (getpeername(cntl_fd, perp, &peername_len) < 0) {
			Trace(TR_ERR, "getpeername(%d) failed %d",
			    cntl_fd, errno);
		}

		level = (peername_len == sizeof (struct  sockaddr_in6)) ?
		    IPPROTO_IPV6 : IPPROTO_IP;
		tos = IPTOS_LOWDELAY;
		if (setsockopt(cntl_fd, level, IP_TOS,
		    (char *)&tos, sizeof (int)) < 0) {
			Trace(TR_ERR, "setsockopt(IPTOS_LOWDELAY) failed %d",
			    errno);
		}
		rft->cin  = fdopen(cntl_fd, "r");
		rft->cout = fdopen(cntl_fd, "w");

		rft->remotehost = B_TRUE;

		/*
		 * Initialize the client hostname.  Using cntl_fd, find the
		 * address for the control socket.  We want to use the same
		 * interface for the data connection.
		 */
		mysock_len = sizeof (mysock);
		memset(&mysock, 0, mysock_len);

		(void) getsockname(cntl_fd, (struct  sockaddr *)&mysock,
		    (int *)&mysock_len);

		/*
		 * Save the interface address used to reach the server.
		 * This needs to be used when setting up the data port.
		 * Find the associated hostname for this address.
		 */
		af = mysock.sin6_family;
		if (af == AF_INET6) {
			memcpy(&rft->caddr.cad6, mysock6,
			    sizeof (struct sockaddr_in6));
			rft->flags |= SAMRFT_IPV6;
			hostname = SamrftGetHostByAddr(
			    (char *)&rft->Si_addr6.s6_addr[0], af);
		} else {
			memcpy(&rft->caddr.cad, mysock4,
			    sizeof (struct sockaddr_in));
			hostname = SamrftGetHostByAddr(
			    (char *)&rft->Si_addr.s_addr, af);
		}

		if (hostname) {
			SamMalloc(rft->hostname, (MAXHOSTNAMELEN+1));
			strncpy(rft->hostname, hostname, MAXHOSTNAMELEN);
			free(hostname);
		} else {
			/*
			 * Can't find hostname for this address.
			 */
			Trace(TR_ERR,
			    "SamrftGetHostByAddr() for client failed %d",
			    errno);
			SamFree(rft);
			rft = NULL;
			return (rft);
		}

		initCmdPort(rft);
	}
	return (rft);
}

/*
 * Disconnect from SAM's rft server on remote host.
 */
static void
disconnectFromServer(
	SamrftImpl_t *rftd)
{
	SendCommand(rftd, "%s", SAMRFT_CMD_DISCONN);

	/*
	 * No reply expected.
	 * Free memory, close files.
	 */
	cleanupCmdPort(rftd);
	cleanupDataPorts(rftd);
}

/*
 * Initialize command port for communication to
 * SAM's rft server on remote host.
 */
static void
initCmdPort(
	SamrftImpl_t *rftd)
{
	rftd->cmdsize = SAMRFT_DEFAULT_CMD_BUFSIZE;
	SamMalloc(rftd->cmdbuf, rftd->cmdsize);
}

/*
 * Clean up command port to server.
 */
static void
cleanupCmdPort(
	SamrftImpl_t *rftd)
{
	close(fileno(rftd->cin));
	SamFree(rftd->cmdbuf);
	SamFree(rftd->hostname);
}

/*
 * Initialize data ports.
 */
static int
initDataPorts(
	SamrftImpl_t *rftd)
{
	int rc = 0;
	int num_dataports;
	int dataportsize;
	int tcpwindowsize;

	/* FIXME - get buffer size from server */
	/* FIXME - move CONFIG request to cmdPort initialization */

	if (rftd->remotehost) {
		SendCommand(rftd, "%s %s", SAMRFT_CMD_CONFIG, rftd->hostname);
		rc = GetReply(rftd);
		if (rc >= 0) {
			(void) GetConfigReply(rftd, &num_dataports,
			    &dataportsize, &tcpwindowsize);
			rc = CreateCrew(rftd, num_dataports,
			    dataportsize, tcpwindowsize);
		}
	}
	return (rc);
}

/*
 * Delete rft data ports.
 */
static void
cleanupDataPorts(
	SamrftImpl_t *rftd)
{
	if (rftd->crew) {
		CleanupCrew(rftd->crew);
		SamFree(rftd->crew);
	}
}

/*
 * Alloc and initialize new rft connection structure.
 */
static SamrftImpl_t *
initRftImpl()
{
	SamrftImpl_t *rft;

	SamMalloc(rft, sizeof (SamrftImpl_t));
	memset((char *)rft, 0, sizeof (SamrftImpl_t));

	rft->fd = -1;

	return (rft);
}

/*
 * Open file on local machine.
 */
static int
openLocalFile(
	SamrftImpl_t *rftd,
	char *filename,
	int oflag,
	SamrftCreateAttr_t *creat)
{
	int rc = -1;

	rftd->fd = -1;
	if (creat != NULL) {
		rftd->fd = open64(filename, oflag, creat->mode & S_IAMB);
	} else {
		rftd->fd = open64(filename, oflag);
	}

	if (rftd->fd >= 0) {
		rc = 0;				/* open okay */

		if (creat != NULL) {
			/*
			 * If creating a file on an NFS mount point, daemon
			 * does not have permission to change the ownership.
			 * Ignore chown errors.
			 */
			(void) chown(filename, creat->uid, creat->gid);
		}
	}

	return (rc);
}

/*
 * Return unique identifier for trace rftd connection.
 */
static int
traceRftd(
	SamrftImpl_t *rftd)
{
	int id = -1;

	if (rftd != NULL) {
		if (rftd->remotehost) {
			if (rftd->cin != NULL) {
				id = fileno(rftd->cin);
			}
		} else {
			id = rftd->fd;
		}
	}
	return (id);
}
