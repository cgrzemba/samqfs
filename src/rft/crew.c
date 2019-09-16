/*
 * crew.c - work crew for data transfer engine.
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

#pragma ident "$Revision: 1.16 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <thread.h>
#include <pthread.h>
#include <sys/mnttab.h>
#include <utime.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "sam/fioctl.h"
#include "sam/mount.h"
#include "aml/sam_rft.h"
#include "aml/diskvols.h"
#include "sam/lib.h"
#include "sam/lint.h"
#include "pub/lib.h"
#include "aml/shm.h"

/* Local headers. */
#include "rft_defs.h"
#include "log.h"

static int getSocket(char *mode, int af);
static char *getRmfile(char *media, char *volname);
static char *gobbleFilename(char *path, char d_name[1]);

/*
 * Create work crew for file transfer.  A work crew
 * is a set of threads, one for each stream.  Each thread
 * has a data port assigned to it.
 */
int
CreateCrew(
	Client_t *cli,
	int num_dataports,
	size_t dataportsize)
{
	Crew_t *crew;

	SamMalloc(crew, sizeof (Crew_t));
	(void) memset(crew, 0, sizeof (Crew_t));

	cli->crew = crew;
	crew->cli = cli;

	crew->num_dataports = num_dataports ? num_dataports : 1;
	crew->dataportsize = dataportsize;
	SamMalloc(crew->buf, dataportsize);
	Trace(TR_DEBUG, "Data block size set to %d", dataportsize);

	return (0);
}

/*
 * Initialize data connection to client.  Initiate a
 * connection on a data socket.
 */
int
InitDataConnection(
	Client_t *cli,
	char *mode,
	int seqnum,
	int af,
	struct sockaddr *dataconn)
{
	int asize;
	int sockfd;
	int value;
#ifdef ORACLE_SOLARIS
	socklen_t length;
#else
	int length;
#endif
	struct	sockaddr_in *dc4 = (struct sockaddr_in *)dataconn;
	struct	sockaddr_in6 *dc6 = (struct sockaddr_in6 *)dataconn;
	Crew_t *crew = cli->crew;

	sockfd = getSocket(mode, af);
	asize = (af == AF_INET6) ? sizeof (struct sockaddr_in6) :
	    sizeof (struct sockaddr_in);

	Trace(TR_DEBUG, "[t@%d] Connect data socket %d 0x%x 0x%x",
	    pthread_self(), sockfd, dc4->sin_addr.S_un.S_addr,
	    dc4->sin_port);

	if (connect(sockfd, dataconn, asize) < 0) {
		Trace(TR_ERR, "connect failed %d", errno);
		(void) close(sockfd);
		return (-1);
	}

	length = sizeof (value);
	if (getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &value, &length) < 0) {
		Trace(TR_ERR, "getsockopt(SO_RCVBUF) failed %d", errno);
	} else {
		Trace(TR_DEBUG, "Get TCP window (RCVBUF) size= %d", value);
	}

	if (getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &value, &length) < 0) {
		Trace(TR_ERR, "getsockopt(SO_SNDBUF) failed %d", errno);
	} else {
		Trace(TR_DEBUG, "Get TCP window (SNDBUF) size= %d", value);
	}

	if (af == AF_INET6) {
		memcpy(&crew->addr, &dc6, sizeof (struct sockaddr_in6));
	} else {
		memcpy(&crew->addr, &dc4, sizeof (struct sockaddr_in));
	}
	crew->in  = fdopen(sockfd, "r");
	crew->out = fdopen(sockfd, "w");

	Trace(TR_DEBUG, "[t@%d] Connected to data socket %d seqnum: %d",
	    pthread_self(), sockfd, seqnum);

	return (0);
}

/*
 * Client request to open a file.
 */
int
OpenFile(
	Client_t *cli,
	char *filename,
	int oflag,
	SamrftCreateAttr_t *creat)
{
	Crew_t *crew;
	struct stat sb;
	int rc = 0;

	Trace(TR_DEBUG, "[t@%d] Open local %s 0x%x",
	    pthread_self(), filename, oflag);

	crew = cli->crew;
	ASSERT(crew != NULL);

	if ((oflag & (O_WRONLY | O_CREAT | O_TRUNC)) ==
	    (O_WRONLY | O_CREAT | O_TRUNC)) {
		if (stat(filename, &sb) == 0) {
			Trace(TR_DEBUG, "[t@%d] Unlink local %s",
			    pthread_self(), filename);
			(void) unlink(filename);
		}
	}

	crew->fd = -1;
	if (creat != NULL) {
		crew->fd = open64(filename, oflag, creat->mode);
		if (crew->fd > -1) {
			(void) chown(filename, creat->uid, creat->gid);
		}
	} else {
		crew->fd = open64(filename, oflag);
	}

	if (crew->fd >= 0) {
		SamStrdup(crew->filename, filename);
	} else {
		/*
		 *	Open failed, return error.
		 */
		rc = -1;
		crew->filename = NULL;
	}
	return (rc);
}

/*
 * Send data to client.
 */
int
SendData(
	Client_t *cli,
	size_t nbytes)
{
	size_t data_to_send;
	size_t blksize;
	size_t reqsize;
	size_t reqsize_netord;

	int rc;

	Crew_t *crew;
	size_t nbytes_read;
	size_t nbytes_written;

	crew = cli->crew;
	data_to_send = nbytes;
	blksize = crew->dataportsize;

	rc = 0;
	while (data_to_send > 0) {

		reqsize = (data_to_send <= blksize) ? data_to_send : blksize;

		Trace(TR_DEBUG, "Read local %d for %d bytes",
		    crew->fd, reqsize);
		nbytes_read = read(crew->fd, crew->buf, reqsize);

		if ((long)nbytes_read == -1) {
			Trace(TR_ERR,
			    "[t@%d] Read error %d nbytes_read: %d "
			    "reqsize: %d",
			    pthread_self(), errno, nbytes_read, reqsize);
			rc = -1;
			break;
		}

		/*
		 * Send number of bytes ready to write on data socket.
		 * Always send in network (big-endian) order.
		 */
		reqsize_netord = htonl(reqsize);
		nbytes_written = write(fileno(crew->out), &reqsize_netord,
		    sizeof (size_t));
		if ((long)reqsize > 0) {
			Trace(TR_DEBUG, "Write socket %d for %d bytes",
			    fileno(crew->out), reqsize);

			nbytes_written = write(fileno(crew->out), crew->buf,
			    reqsize);

			if (nbytes_written != reqsize) {
				Trace(TR_ERR, "Write error %d %d %d",
				    nbytes_written, reqsize, errno);
				rc = -1;
				break;
			}
		}
		data_to_send -= reqsize;
	}

	/*
	 * Total number of bytes sent to client.
	 */
	crew->nbytes_sent += nbytes;

	return (rc);
}


/*
 * Receive data from client.
 */
int
ReceiveData(
	Client_t *cli,
	fsize_t nbytes)
{
	extern ssize_t readn(int fd, char *ptr, size_t nbytes);
	fsize_t data_to_receive;
	size_t blksize;
	size_t reqsize;
	int rc;

	Crew_t *crew;
	size_t nbytes_written;
	size_t nbytes_read;

	crew = cli->crew;
	data_to_receive = nbytes;
	blksize = crew->dataportsize;

	rc = 0;
	while (data_to_receive > 0) {

		reqsize = (data_to_receive <= blksize) ?
		    data_to_receive : blksize;

		Trace(TR_DEBUG, "Read socket %d for %d bytes",
		    fileno(crew->in), reqsize);
		nbytes_read = readn(fileno(crew->in), crew->buf, reqsize);
		if (nbytes_read != reqsize) {
			Trace(TR_ERR, "Read socket failed expected %d "
			    "got %d %d", reqsize, nbytes_read, errno);
			rc = -1;
			break;
		}

		Trace(TR_DEBUG, "Write local %d for %d bytes",
		    crew->fd, nbytes_read);
		nbytes_written = write(crew->fd, crew->buf, nbytes_read);
		if (nbytes_written != nbytes_read) {
			Trace(TR_ERR, "Write local failed expected %d "
			    "got %d %d", nbytes_read, nbytes_written, errno);
			rc = -1;
			break;
		}
		data_to_receive -= nbytes_read;
	}

	/*
	 * Total number of bytes received from client.
	 */
	crew->nbytes_received += nbytes;

	return (rc);
}

/*
 * Client request to close a file.
 */
int
CloseFile(
	Client_t *cli)
{
	Crew_t *crew;

	crew = cli->crew;
	ASSERT(crew != NULL);

	Trace(TR_DEBUG, "[t@%d] Close local %d", pthread_self(), crew->fd);

	if (crew->fd >= 0) {
		(void) close(crew->fd);

		if (IS_LOG_ENABLED()) {
			LogIt(crew);
		}
	}

	crew->nbytes_sent = 0;
	crew->nbytes_received = 0;
	if (crew->filename) {
		SamFree(crew->filename);
	}

	return (0);
}

/*
 * Client request to seek a file.
 */
int
SeekFile(
	Client_t *cli,
	off64_t setpos,
	int whence,
	off64_t *offset)
{
	int rc = 0;
	Crew_t *crew;

	crew = cli->crew;
	ASSERT(crew != NULL);

	Trace(TR_DEBUG, "[t@%d] Seek local %d %lld", pthread_self(),
	    crew->fd, setpos);

	ASSERT(crew->fd >= 0);
	*offset = lseek64(crew->fd, setpos, whence);
	if (*offset < 0) {
		rc = -1;
	}
	return (rc);
}

/*
 * Client request to flock a file.
 */
int
FlockFile(
	Client_t *cli,
	int type)
{
	int rc = 0;
	Crew_t *crew;
	struct flock fl;

	crew = cli->crew;
	ASSERT(crew != NULL);

	Trace(TR_DEBUG, "[t@%d] Flock local %d %d", pthread_self(),
	    crew->fd, type);

	ASSERT(crew->fd >= 0);
	if (type == F_UNLCK) {
		(void) fsync(crew->fd);
	}
	memset(&fl, 0, sizeof (struct flock));
	fl.l_type |= type;
	rc  = fcntl(crew->fd, F_SETLKW, &fl);

	return (rc);
}

/*
 * Client request to unlink a file.
 */
int
UnlinkFile(
	/* LINTED argument unused in function */
	Client_t *cli,
	char *file_name)
{
	int rc;

	Trace(TR_DEBUG, "[t@%d] Unlink file %s", pthread_self(), file_name);

#undef unlink /* Undefine definition in lint.h */
	rc = unlink(file_name);

	return (rc);
}

/*
 * Client request to check if a file system is mounted.
 */
int
IsMounted(
	/* LINTED argument unused in function */
	Client_t *cli,
	char *mount_point)
{
	FILE *fp;
	struct mnttab mp;
	struct mnttab mpref;
	int mounted = 0;

	Trace(TR_DEBUG, "[t@%d] IsMounted %s",
	    pthread_self(), mount_point);

	fp = fopen(MNTTAB, "r");
	if (fp) {
		(void) memset(&mpref, 0, sizeof (struct mnttab));
		(void) memset(&mp, 0, sizeof (struct mnttab));
		mpref.mnt_mountp = mount_point;

		mounted = getmntany(fp, &mp, &mpref);

		(void) fclose(fp);
	}
	return (mounted >= 0 ? TRUE : FALSE);
}

/*
 * Client request to make a directory.
 */
int
MkDir(
	/* LINTED argument unused in function */
	Client_t *cli,
	char *dirname,
	int mode,
	int uid,
	int gid)
{
	int rc = 0;
	struct stat sb;

	rc = stat(dirname, &sb);
	if (rc != 0 || S_ISDIR(sb.st_mode) == 0) {
		/*
		 *	Name exists but its a file.
		 */
		if (rc == 0) {
			SetErrno = EACCES;
			return (-1);
		}

		rc = mkdir(dirname, mode);
		if (rc == 0) {
			rc = chown(dirname, uid, gid);
		}
	}
	return (rc);
}

/*
 * Client request to open a directory.
 */
int
OpenDir(
	Client_t *cli,
	char *dirname,
	int *dir_idx)
{
	DIR *dirp;
	int rc = -1;

	dirp = opendir(dirname);
	if (dirp != NULL) {
		int idx = cli->num_opendirs;

		if (cli->opendirs == NULL) {
			SamMalloc(cli->opendirs,  sizeof (DIR *));
			SamMalloc(cli->openpaths, sizeof (char *));
		} else {
			SamRealloc(cli->opendirs,
			    sizeof (DIR *) * (cli->num_opendirs + 1));
			SamRealloc(cli->openpaths,
			    sizeof (char *) * (cli->num_opendirs + 1));
		}

		cli->opendirs[idx] = dirp;
		SamStrdup(cli->openpaths[idx], dirname);
		cli->num_opendirs++;

		*dir_idx = idx;
		rc = 0;
	}
	return (rc);
}

/*
 * Client request to read a directory.
 */
int
ReadDir(
	Client_t *cli,
	int dir_idx,
	SamrftReaddirInfo_t *dir_info)
{
	int rc = 0;
	DIR *dirp;
	struct dirent *dp;
	struct stat sb;
	char *name;

	dirp = cli->opendirs[dir_idx];

	while ((dp = readdir(dirp)) != NULL) {
		/* if (skip_file() */
		if ((dp->d_name[0] == '.' && dp->d_name[1] == '\0') ||
		    (dp->d_name[0] == '.' && dp->d_name[1] == '.' &&
		    dp->d_name[2] == '\0')) {
			continue;
		}

		name = gobbleFilename(cli->openpaths[dir_idx], dp->d_name);
		(void) stat(name, &sb);
		dir_info->isdir = S_ISDIR(sb.st_mode);
		break;
	}

	if (dp) {
		dir_info->name = dp->d_name;
	} else {
		dir_info->name = NULL;
		rc = -1;
	}
	return (rc);
}

/*
 * Client request to close a directory.
 */
void
CloseDir(
	Client_t *cli,
	int dir_idx)
{
	DIR *dirp;

	dirp = cli->opendirs[dir_idx];
	(void) closedir(dirp);
}

/*
 * Client request to remove a directory.
 */
int
RmDir(
	/* LINTED argument unused in function */
	Client_t *cli,
	char *dirname)
{
	int rc;

	rc = rmdir(dirname);

	return (rc);
}

/*
 * Client request to load removable-media.
 */
int
LoadVol(
	Client_t *cli,
	struct sam_rminfo *rb,
	int oflag)
{
	int rc = 0;
	char *path;

	path = getRmfile((char *)&rb->media, (char *)&rb->section[0].vsn);
	if (path == NULL) {
		Trace(TR_ERR, "[t@%d] Failed to create rm file",
		    pthread_self());
/* FIXME set errno */
		return (-1);
	}

	Trace(TR_MISC, "[t@%d] Request removable media %s",
	    pthread_self(), path);

	rc = sam_request(path, rb, SAM_RMINFO_SIZE(rb->n_vsns));
	if (rc < 0) {
		Trace(TR_ERR, "[t@%d] Request syscall failed %s %d",
		    pthread_self(), path, errno);
	}

	if (rc >= 0) {
		Crew_t *crew;

		crew = cli->crew;
		ASSERT(crew != NULL);

		crew->fd = open(path, oflag, 0644);
		if (crew->fd >= 0) {
			/*
			 */
			SamStrdup(crew->filename, path);
			strncpy(crew->vsn, (char *)&rb->section[0].vsn,
			    sizeof (crew->vsn));

		} else {
			/*
			 * Open failed, return error.
			 */
			rc = -1;
		}
	}
	return (rc);
}

/*
 * Client request to get removable-media information.
 */
int
GetVolInfo(
	Client_t *cli,
	struct sam_rminfo *getrm,
	int *eq)
{
	extern shm_alloc_t master_shm;
	extern shm_ptr_tbl_t *shm_ptr_tbl;

	int rc = 0;
	struct sam_ioctl_getrminfo sr;
	Crew_t *crew;

	crew = cli->crew;
	ASSERT(crew != NULL);

	sr.bufsize = SAM_RMINFO_SIZE(1);
	sr.buf.ptr = getrm;
	if (ioctl(crew->fd, F_GETRMINFO, &sr) < 0) {
		ASSERT_NOT_REACHED();
	}

	/*
	 * Look up drive number for the mounted VSN.
	 */
	*eq = 0;

	if (shm_ptr_tbl != NULL || ShmatSamfs(O_RDWR) != NULL) {
		dev_ent_t *dev_head;
		dev_ent_t *dev;

		dev_head = (struct dev_ent *)
		    SHM_REF_ADDR(shm_ptr_tbl->first_dev);
		for (dev = dev_head; dev != NULL;
			dev = (struct dev_ent *)SHM_REF_ADDR(dev->next)) {
			if (strcmp(dev->vsn, crew->vsn) == 0) {
				*eq = dev->eq;
				break;
			}
		}
	}

	return (rc);
}

/*
 * Client request to position removable-media.
 */
int
SeekVol(
	Client_t *cli,
	int block)
{
	int rc = 0;
	sam_ioctl_rmposition_t pos_args;
	Crew_t *crew;

	Trace(TR_DEBUG, "[t@%d] Position removable media %d",
	    pthread_self(), block);

	crew = cli->crew;
	ASSERT(crew != NULL);

	pos_args.setpos = block;
	rc = ioctl(crew->fd, F_RMPOSITION, &pos_args);
	if (rc < 0) {
		Trace(TR_ERR, "[t@%d] Position syscall failed %d %d",
		    pthread_self(), crew->fd, errno);
	}

	return (rc);
}

/*
 * Client request to unload removable-media.
 */
int
UnloadVol(
	Client_t *cli,
	struct sam_ioctl_rmunload *unload)
{
	int rc = 0;
	Crew_t *crew;

	crew = cli->crew;

	Trace(TR_MISC, "[t@%d] Unload removable media",
	    pthread_self());

	rc = ioctl(crew->fd, F_UNLOAD, unload);
	if (rc < 0) {
		Trace(TR_ERR, "[t@%d] Unload ioctl failed %d %d",
		    pthread_self(), crew->fd, errno);
	}

	if (close(crew->fd) == -1) {
		Trace(TR_ERR, "[t@%d] Close failed %d %d",
		    pthread_self(), crew->fd, errno);
	}

	if (crew->filename) {
		if (unlink(crew->filename) == -1) {
			Trace(TR_ERR, "[t@%d] Unlink failed %s %d",
			    pthread_self(), crew->filename, errno);
		}
	}
	return (rc);
}

/*
 * Cleanup worker crew. Close sockets for client's work crew.
 */
void
CleanupCrew(
	Client_t *cli)
{
	Crew_t *crew;

	Trace(TR_DEBUG, "[t@%d] Cleanup crew", pthread_self());

	crew = cli->crew;

	if (crew) {
		(void) fclose(crew->in);
		(void) fclose(crew->out);
		SamFree(crew->buf);

/* FIXME - why close cli sockets here ? */
		(void) fclose(cli->cin);
		(void) fclose(cli->cout);
	}
}

/*
 * Create a socket for communication to client.
 */
static int
getSocket(
	/* LINTED argument unused in function */
	char *mode,
	int	af)
{
	int sockfd;
	int on;
	int tcpwindowsize;

	sockfd = socket(af, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket");
		ASSERT_NOT_REACHED();
	}

	/*
	 * Tell kernel its okay to have more than one process
	 * originating from this port.
	 */
	on = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
	    (char *)&on, sizeof (on)) < 0) {
		Trace(TR_ERR, "setsockopt(SO_REUSEADDR) failed %d", errno);
	}

	/*
	 *	Set TCP window size.
	 */
	tcpwindowsize = GetCfgTcpWindowsize();
	Trace(TR_MISC, "Setting TCP window size= %d", tcpwindowsize);
	if (tcpwindowsize > 0) {
		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF,
		    (char *)&tcpwindowsize, sizeof (tcpwindowsize)) < 0) {
			Trace(TR_ERR, "setsockopt(SO_RCVBUF) failed %d", errno);
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF,
		    (char *)&tcpwindowsize, sizeof (tcpwindowsize)) < 0) {
			Trace(TR_ERR, "setsockopt(SO_SNDBUF) failed %d", errno);
		}
	}

	on = IPTOS_THROUGHPUT;
	Trace(TR_DEBUG, "Set TCP throughput option");
	if (setsockopt(sockfd, IPPROTO_IP, IP_TOS,
	    (char *)&on, sizeof (int)) < 0) {
		Trace(TR_ERR, "setsockopt(IPTOS_THROUGHPUT) failed %d", errno);
	}

	return (sockfd);
}

/*
 * Make removable media file.  The .rft directory is created
 * in the root of the filesystem. (replaced .ftp)
 */
static char *
getRmfile(
	char *media,
	char *volname)
{
	static upath_t fullpath;

	int i;
	int rc;
	struct stat sb;
	int num;
	struct sam_fs_status *fsarray;
	struct sam_fs_status fs;
	struct sam_fs_info fi;
	char *mount_point;
	char *path = NULL;

	/*
	 * Get count of configured filesystems.
	 */
	num = GetFsStatus(&fsarray);
	if (num <= 0) {
		Trace(TR_ERR, "[t@%d] No configured SAM-FS filesystems",
		    pthread_self());
		/* SendCustMsg(HERE, 19004); */
		return (NULL);
	}

	mount_point = NULL;
	for (i = 0; i < num; i++) {
		fs = *(fsarray + i);
		if (fs.fs_status & FS_MOUNTED) {
			if (GetFsInfo(fs.fs_name, &fi) == -1) {
				Trace(TR_ERR,
				    "t@[%d] GetFsInfo() failed for %s",
				    pthread_self(), fs.fs_name);
				continue;
			}
			if ((fi.fi_config & MT_SHARED_READER) == 0) {
				mount_point = fs.fs_mnt_point;
				break;
			}
		}
	}

	if (mount_point == NULL || mount_point[0] == '\0') {
		Trace(TR_ERR, "[t@%d] No mounted SAM-FS filesystems",
		    pthread_self());
		/* SendCustMsg(HERE, 19004); */
		return (NULL);
	}

	(void) sprintf(fullpath, "%s/%s", mount_point, RM_DIR);

	/*
	 * Check if directory doesn't exist or something is there
	 * but its not a directory.
	 */
	rc = stat(fullpath, &sb);
	if (rc != 0 || S_ISDIR(sb.st_mode) == 0) {
		if (rc == 0) {
			/* remove existing file */
			(void) unlink(fullpath);
		}

		if (mkdir(fullpath, DIR_MODE) != 0) {
			ASSERT_NOT_REACHED();
		}
	}

	/*
	 *  Set noarch on directory.
	 */
	if (sam_archive(fullpath, "n") < 0) {
		ASSERT_NOT_REACHED();
	}

	path = fullpath + strlen(fullpath);
	(void) sprintf(path, "/%s.%s", media, volname);

	/*
	 * Prepare return value.
	 */
	SamStrdup(path, fullpath);

	if (stat(fullpath, &sb) != 0) {
		int fd;

		fd = open(fullpath, O_CREAT | O_TRUNC | SAM_O_LARGEFILE,
		    FILE_MODE);
		if (fd < 0) {
			SamFree(path);
			path = NULL;
			ASSERT_NOT_REACHED();
		} else {
			(void) close(fd);
		}
	}
	return (path);
}

/*
 * Create path from directory and file name.
 */
char *
gobbleFilename(
	char *path,
	char d_name[1])
{
	static upath_t name;

	strcpy(name, path);
	strcat(name, "/");
	strcat(name, d_name);

	return (name);
}
