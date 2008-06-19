/*
 * rmstage.c - support functions specific to staging from
 * removable media archive files
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

#pragma ident "$Revision: 1.34 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <values.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <sys/shm.h>

/* SAM-FS headers. */
#include "pub/lib.h"
#include "sam/types.h"
#include "aml/shm.h"
#include "aml/tar.h"
#include "pub/stat.h"
#include "sam/fioctl.h"
#include "aml/opticals.h"
#include "pub/rminfo.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "sam/custmsg.h"
#include "sam/exit.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "aml/stager_defs.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Local headers. */
#include "stager_config.h"
#include "stager_lib.h"
#include "stager_threads.h"
#include "rmedia.h"
#include "stream.h"
#include "copy_defs.h"

#include "copy.h"
#include "circular_io.h"

/* Public data. */
extern CopyInstanceInfo_t *Instance;
extern IoThreadInfo_t *IoThread;
extern StreamInfo_t *Stream;

/* Private data. */
static char *hostName = NULL;
static struct sam_rminfo rmInfo;
static int rmDriveNumber;

static void initRemoteStage();
static int loadVol(char *file_name, vsn_t vsn, media_t media);
static void createRmFile(char *path);
static void initHostName();
static void getHostAddr();
static void getRmInfo(vsn_t vsn);

#if 0
#define	SIM_ERROR
#define	SIM_POSITION_ERROR
#endif

#ifdef SIM_ERROR
static int simCount = 0;
static int simTrigger = 0;
static int simErrors = 5;
#endif

/*
 * Init removable media file stage.
 */
void
RmInit(void)
{
	memset(&rmInfo, 0, sizeof (struct sam_rminfo));
}

/*
 * Load removable media volume.
 */
int
RmLoadVolume(void)
{
	char rmPath[MAXPATHLEN + 4];
	char *mount_name;
	FileInfo_t *file;
	int error = 0;

	Trace(TR_MISC, "Load rm volume: '%s.%s'",
	    sam_mediatoa(Instance->ci_media), Stream->vsn);

	if (Instance->ci_flags & CI_samremote) {
		initRemoteStage();
	}

	/*
	 * Get a SAM-FS mount point.
	 */
	file = GetFile(Stream->first);

	/*
	 * If no file in stream the request was canceled.
	 * Return a cancel error.  Caller picks up that no files
	 * were in the stream.
	 */
	if (file == NULL) {
		Trace(TR_MISC, "Load rm volume canceled, no files in stream");
		return (ECANCELED);
	}

	mount_name = GetMountPointName(file->fseq);

	sprintf(rmPath, "%s/%s/rm%d", mount_name, RM_DIR, Instance->ci_rmfn);

	error = loadVol(rmPath, Stream->vsn, Instance->ci_media);

	if (error == 0) {
		/*
		 * Get information about removable media file.
		 */
		getRmInfo(Stream->vsn);
	}
	return (error);
}

/*
 * Get removable media file block size.
 */
int
RmGetBlockSize(void)
{
	return (rmInfo.block_size);
}

/*
 * Get removable media file buffer size.
 */
offset_t
RmGetBufferSize(
	int block_size)
{
	offset_t buffer_size = 0;

	if ((Instance->ci_media & DT_CLASS_MASK) == DT_OPTICAL) {
		/*
		 * Optical media reads are blocked by driver.
		 */
		buffer_size = OD_BS_DEFAULT;

	} else {
		/*
		 * Tape media must be read in block size reads.
		 */
		buffer_size = block_size;
	}
	return (buffer_size);
}

/*
 * Get position of removable media file.  NOTE: request only valid
 * following load volume request.
 */
u_longlong_t
RmGetPosition(void)
{
	return (rmInfo.position);
}

/*
 * Get drive number for the mounted VSN.
 */
equ_t
RmGetDriveNumber(void)
{
	return (rmDriveNumber);
}

/*
 * Unload removable media file.
 */
void
RmUnloadVolume(void)
{
	struct sam_ioctl_rmunload rmunload;

	Trace(TR_MISC, "Unload rm volume: '%s'", Stream->vsn);
	rmunload.flags = 0;

	if (Instance->ci_flags & CI_samremote) {
		if (SamrftUnloadVol(IoThread->io_rftHandle, &rmunload) < 0) {
			WarnSyscallError(HERE, "SamrftUnloadVol", "");
		}

		SamrftDisconnect(IoThread->io_rftHandle);
		IoThread->io_rftHandle = NULL;

	} else {

		if (ioctl(IoThread->io_rmFildes, F_UNLOAD, &rmunload) < 0) {
			WarnSyscallError(HERE, "ioctl", "F_UNLOAD");
		}
		if (close(IoThread->io_rmFildes) < 0) {
			WarnSyscallError(HERE, "close", "");
		}
	}
}

/*
 * Seek to block on removable media.
 */
int
RmSeekVolume(
	int to)
{
	int position = -1;

	Trace(TR_MISC, "Start positioning: %d", to);

	if (Instance->ci_flags & CI_samremote) {
		if (SamrftSeekVol(IoThread->io_rftHandle, to) < 0) {
			WarnSyscallError(HERE, "GetrftSeekVol", "");

		} else {
			position = to;
		}

	} else {
		int err;
		sam_ioctl_rmposition_t pos_args;
		int attempts = 2;

		pos_args.setpos = to;

		while (position == -1 && attempts-- > 0) {
			err = ioctl(IoThread->io_rmFildes, F_RMPOSITION,
			    &pos_args);

#ifdef SIM_POSITION_ERROR
			if (simErrors > 0) {
				if (simTrigger == 0) {
					setSimTrigger();
				}
				simCount++;
				if (err >= 0 && simCount >= simTrigger) {
					Trace(TR_DEBUG,
					    "Simulated positioning error");
					err = -1;
					SetErrno = ETIME;

					simErrors--;
					attempts = simCount = simTrigger = 0;
				}
			}
#endif

			if (err >= 0) {
				position = to;

			} else {
				if (errno == ECANCELED) {
					FileInfo_t *file = IoThread->io_file;

					position = 0;

					Trace(TR_MISC, "Canceled(pos error) "
					    "inode: %d.%d",
					    file->id.ino, file->id.gen);

					SET_FLAG(IoThread->io_flags, IO_cancel);

				} else {
					WarnSyscallError(HERE, "ioctl",
					    "F_RMPOSITION");
					/*
					 * Unless the removable media device
					 * times out (ETIME), the device code
					 * will return EIO for all positioning
					 * errors.  This will result in the
					 * copy marked as damaged.
					 */
					sleep(5);
				}
			}
		}
	}

	Trace(TR_DEBUG, "Positioning complete: %d", position);
	return (position);
}

/*
 * Load removable media file.
 */
static int
loadVol(
	char *file_name,
	vsn_t vsn,
	media_t media)
{
	struct sam_rminfo rb;
	char *media_name;
	int oflag;
	int fd = -1;
	int retry = 5;
	int error = 0;

	media_name = sam_mediatoa(media);
	oflag = O_RDONLY | SAM_O_LARGEFILE;

	memset(&rb, 0, SAM_RMINFO_SIZE(1));
	rb.flags = RI_blockio | RI_nopos;
	strncpy(rb.media, media_name, sizeof (rb.media)-1);
	strncpy(rb.file_id, "sam_stage_file", sizeof (rb.file_id));
	strncpy(rb.owner_id, SAM_ARCHIVE_OWNER, sizeof (rb.owner_id));
	strncpy(rb.group_id, SAM_ARCHIVE_GROUP, sizeof (rb.group_id));
	rb.n_vsns = 1;
	memcpy(rb.section[0].vsn, vsn, sizeof (rb.section[0].vsn));

	/*
	 * Request and open the removable media.
	 */
	if (Instance->ci_flags & CI_samremote) {
		SetErrno = 0;
		if (SamrftLoadVol(IoThread->io_rftHandle, &rb, oflag) < 0) {
			if (errno == 0) {
				SetErrno = ETIME;
			}
			WarnSyscallError(HERE, "SamrftLoadVol",
			    rb.section[0].vsn);
			error = errno;
		}

	} else {

		if (sam_request(file_name, &rb,
		    SAM_RMINFO_SIZE(rb.n_vsns)) == -1) {

			createRmFile(file_name);

			if (sam_request(file_name, &rb,
			    SAM_RMINFO_SIZE(rb.n_vsns)) == -1) {
				WarnSyscallError(HERE, "sam_request",
				    file_name);
				error = EIO;
			}
		}

		while (error == 0 && fd == -1 && retry-- > 0) {
			fd = open(file_name, oflag);
			if (fd == -1) {

				Trace(TR_ERR, "Load vsn failed: '%s', "
				    "errno %d", vsn, errno);

				if (errno != ETIME) {
					int saveErrno;
					struct CatalogEntry ced;

					saveErrno = errno;
					SetErrno = ENODEV;

					/*
					 * If load fails check for bad media.
					 */
					if (CatalogInit(program_name) >= 0) {
						if (CatalogGetCeByMedia(
						    sam_mediatoa(media), vsn,
						    &ced) != NULL) {
							if (ced.CeStatus &
							    CES_bad_media) {
								SetErrno =
								    saveErrno;
							}
						}
						CatalogTerm();
					}
					break;
				}
				sleep(5);
			}
		}

		IoThread->io_rmFildes = fd;
		if (fd == -1) {
			if (errno == 0) {
				error = EIO;
			} else {
				error = errno;
			}
		}
	}
	return (error);
}

/*
 * Establish connection to remote host.  If connection fails,
 * the copy process will exit.
 */
static void
initRemoteStage(void)
{
	if (hostName == NULL) {
		initHostName();
	}
	ASSERT(hostName != NULL);

	/*
	 *	Establish connection to remote host.
	 */
	IoThread->io_rftHandle = (void *) SamrftConnect(hostName);
	if (IoThread->io_rftHandle == NULL) {
		LibFatal(SamrftConnect, hostName);
	}
}

/*
 * Create a removable media file.  Should happen rarely, .ie
 * a new file system is mounted, but it seems better to do extra
 * work to create it here versus waiting for the mount to be seen.
 * I think this will be unnecessary when stager is started from fsd.
 */
static void
createRmFile(
	char *path)
{
	static char *delim = "/";
	upath_t token_buf;
	upath_t fullpath;
	char *filename;
	char *dirname;
	char *mountpt;
	struct stat sb;
	int rc;
	int fd;

	filename = strrchr(path, '/');
	filename++;				/* strip leading "/" */

	strcpy(token_buf, path);
	mountpt = strtok(token_buf, delim);

	strcpy(fullpath, delim);
	strcat(fullpath, mountpt);

	dirname = strtok(NULL, delim);
	strcat(fullpath, delim);
	strcat(fullpath, dirname);

	rc = stat(fullpath, &sb);
	if (rc != 0 || S_ISDIR(sb.st_mode) == 0) {
		if (rc == 0) {
			/*
			 * File already exists by this name.
			 */
			FatalSyscallError(EXIT_FATAL, HERE, "createRmFile",
			    fullpath);
		}
		if (mkdir(fullpath, DIR_MODE) != 0) {
			FatalSyscallError(EXIT_NORESTART, HERE, "mkdir",
			    fullpath);
		}
	}

	if (sam_archive(fullpath, "n") < 0) {
		FatalSyscallError(EXIT_NORESTART, HERE, "sam_archive",
		    fullpath);
	}

	if (stat(path, &sb) != 0) {
		fd = open(path, O_CREAT | O_TRUNC | SAM_O_LARGEFILE, FILE_MODE);
		if (fd < 0) {
			FatalSyscallError(EXIT_NORESTART, HERE, "open", path);
		} else {
			(void) close(fd);
		}
	}
}

/*
 * Init host name for remote sam server.
 */
static void
initHostName(void)
{
	int af;

	if ((Instance->ci_flags & CI_addrval) == 0) {
		getHostAddr();
	}
	if (Instance->ci_flags & CI_ipv6) {
		af = AF_INET6;
	} else {
		af = AF_INET;
	}
	hostName = SamrftGetHostByAddr(&Instance->ci_hostAddr, af);
	if (hostName == NULL) {
		FatalSyscallError(EXIT_FATAL, HERE, "SamrftGetHostByAddr", "");
	}
}

/*
 * Get host address for remote sam server.  Kept in the master shared
 * memory segment and used to find server's host name.  The host
 * address is stored in the copy's context so only needs to be looked
 * once.
 */
static void
getHostAddr(void)
{
	dev_ent_t *dev_head;
	dev_ent_t *dev;
	int found_device = FALSE;

	extern shm_alloc_t master_shm;
	extern shm_ptr_tbl_t *shm_ptr_tbl;

	if (shm_ptr_tbl == NULL) {
		if (ShmatSamfs(O_RDWR) == NULL) {
			SendCustMsg(HERE, 19011);
			FatalSyscallError(EXIT_NORESTART, HERE,
			    "ShmatSamfs", "");
		}
	}

	dev_head = (struct dev_ent *)SHM_REF_ADDR(shm_ptr_tbl->first_dev);

	for (dev = dev_head; dev != NULL;
			dev = (struct dev_ent *)SHM_REF_ADDR(dev->next)) {

		if (dev->eq == Instance->ci_eq) {
			ASSERT(dev->equ_type == DT_PSEUDO_SC);
			found_device = TRUE;
			break;
		}
	}

	if (found_device) {
		srvr_clnt_t *server;

		server = (srvr_clnt_t *)SHM_REF_ADDR(dev->dt.sc.server);
		memcpy(&Instance->ci_hostAddr, &server->control_addr,
		    sizeof (in6_addr_t));
		Instance->ci_flags |= CI_addrval;
		if (server->flags && SRVR_CLNT_IPV6) {
			Instance->ci_flags |= CI_ipv6;
		}
	}
}

/*
 * Get information about removable media file.
 */
void
getRmInfo(
	vsn_t vsn)
{
	dev_ent_t *dev_head;
	dev_ent_t *dev;

	extern shm_alloc_t master_shm;
	extern shm_ptr_tbl_t *shm_ptr_tbl;

	if (Instance->ci_flags & CI_samremote) {
		if (SamrftGetVolInfo(IoThread->io_rftHandle, &rmInfo,
		    &rmDriveNumber) < 0) {
			FatalSyscallError(EXIT_FATAL, HERE,
			    "SamrftGetVolInfo", "");
		}

	} else {
		struct sam_ioctl_getrminfo sr;

		sr.bufsize = SAM_RMINFO_SIZE(1);
		sr.buf.ptr = &rmInfo;
		if (ioctl(IoThread->io_rmFildes, F_GETRMINFO, &sr) < 0) {
			FatalSyscallError(EXIT_FATAL, HERE, "ioctl",
			    "F_GETRMINFO");
		}

		/*
		 * Look up drive number for the mounted VSN.
		 */

		rmDriveNumber = 0;

		if (shm_ptr_tbl == NULL) {
			if (ShmatSamfs(O_RDWR) == NULL) {
				SendCustMsg(HERE, 19011);
				FatalSyscallError(EXIT_NORESTART, HERE,
				    "ShmatSamfs", "");
			}
		}

		dev_head =
		    (struct dev_ent *)SHM_REF_ADDR(shm_ptr_tbl->first_dev);

		for (dev = dev_head; dev != NULL;
		    dev = (struct dev_ent *)SHM_REF_ADDR(dev->next)) {

			if (strcmp(dev->vsn, vsn) == 0) {
				rmDriveNumber = dev->eq;
				break;
			}
		}
	}
}

#ifdef SIM_ERROR
int
setSimTrigger(void)
{
	simTrigger = rand();
	while (simTrigger > 100) {
		simTrigger /= 2;
	}
	simTrigger = 1;
}
#endif
