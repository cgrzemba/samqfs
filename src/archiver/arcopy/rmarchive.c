/*
 * rmarchive.c - Removable media archival.
 *
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


#pragma ident "$Revision: 1.80 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

/* SAM-FS headers. */
#include "pub/rminfo.h"
#include "pub/stat.h"
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/fioctl.h"
#include "sam/lib.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/device.h"
#include "aml/opticals.h"
#include "aml/remote.h"
#include "aml/sam_rft.h"

/* Local headers. */
#include "arcopy.h"

/* Private data. */
static boolean_t maybeEOM;	/* Possible false end-of-media error */
static struct VolId vid;	/* For the current volume */
static upath_t rmFname;		/* Removable media file name */
static int volStart;		/* Starting volume for archive file */

/* Private functions. */
static void endOfVolume();
static void initRemoteArchive(void);
static void loadVolume(void);
static void reserveVolume(struct VolInfo *vi, struct ArchSet *as,
	char *owner_a, char *fsname_a);
static void setVolumeFull(void);
static void unloadVolume(void);


/*
 * Begin writing archive file.
 * Called at the beginning of an archive file.
 * Loads the removable media file and allocates the copy buffer.
 */
void
RmBeginArchiveFile(void)
{
	volStart = VolCur;
	VolsTable->entry[VolCur].VlLength = 0;
	if (AfFd >= 0) {
		return;
	}
	loadVolume();
	maybeEOM = FALSE;
}


/*
 * Enter removable media information for all files in the just written
 * archive file.  Identify files that have overflowed a volume.
 * Adjust the file offsets for following files.
 */
void
RmEndArchiveFile(
	int firstFile)
{
	fsize_t	offsetAdj;
	struct VolsEntry *ve;
	int	fn;
	int	volNum;

	/*
	 * Clear the empty flag. If the flag is set, the media was relabeled
	 * and this is the first time it has been used since then.
	 */
	if (CatalogSetField(&vid, CEF_Status, 0, CES_empty) == -1) {
		Trace(TR_ERR, "Catalog empty flag unset failed: %s.%s",
		    vid.ViMtype, vid.ViVsn);
	}

	/*
	 * Unload removable media file and get removable media information.
	 */
	unloadVolume();

	/*
	 * Reserve the volume if required.
	 */
	volNum = volStart;
	ve = &VolsTable->entry[volNum];
	if (!(ve->Vi.VfFlags & VF_reserved) &&
	    ArchiveSet->AsReserve != RM_none) {
		reserveVolume(&ve->Vi, ArchiveSet, Instance->CiOwner,
		    ArchReq->ArFsname);
	}

	/*
	 * Enter VSN and position information for all files copied to
	 * archive file.
	 */
	offsetAdj = 0;
	for (fn = firstFile; fn < FilesNumof; fn++) {
		struct ArchiveFile *af;

		af = &FilesTable[fn];
		if (af->AfFlags & AF_first) {
			break;
		}
		if (!(af->AfFlags & AF_copied)) {
			continue;
		}

		af->AfVol = volNum;
		af->AfPosition = ve->VlPosition;
		af->AfOffset -= offsetAdj;
		if ((af->AfOffset + af->AfFileSize) > ve->VlLength) {
			/*
			 * File has overflowed.
			 * Advance past this file.
			 * The offset adjustment for following files is the
			 * length of the overflowed file sections.
			 */
			fsize_t accum;
			int	section;

			af->AfFlags |= AF_overflow;
			accum = 0;
			for (section = 0; /* Terminated inside */; section++) {
				fsize_t	length;
				fsize_t	offset;

				offset = (section == 0) ? af->AfOffset : 0;
				length = ve->VlLength - offset;
				if ((accum + length) > af->AfFileSize) {
					break;
				}
				offsetAdj += ve->VlLength;
				accum += length;
				ve++;
				volNum++;
				section++;
				/*
				 * Reserve the volume if required.
				 */
				if (!(ve->Vi.VfFlags & VF_reserved) &&
				    ArchiveSet->AsReserve != RM_none) {
					reserveVolume(&ve->Vi, ArchiveSet,
					    Instance->CiOwner,
					    ArchReq->ArFsname);
				}
			}
		}
	}
}


/*
 * Initialize RmArchive module.
 */
void
RmInit(void)
{
	struct dev_ent *library;

	snprintf(ArVolName, sizeof (ArVolName), "%s.%s", Instance->CiMtype,
	    Instance->CiVsn);
	snprintf(rmFname, sizeof (rmFname), "%s/%s/rm%d", MntPoint,
	    RM_FILES_DIR, Instance->CiRmFn);

	library = (struct dev_ent *)SHM_REF_ADDR(Instance->CiLibDev);
	if (library->equ_type == DT_PSEUDO_SC) {
		initRemoteArchive();
	} else if (!(Instance->CiFlags & CI_sim)) {
		struct sam_stat sb;

		/*
		 * Assure that the removable media file exists.
		 */
retry:
		if (sam_stat(rmFname, &sb, sizeof (sb)) != 0) {
			int fd;

			if (errno != ENOENT) {
				(void) unlink(rmFname);
			}
			if ((fd = open(rmFname, O_CREAT | O_EXCL, 0600)) < 0) {
				static boolean_t first = TRUE;

				if (errno != ENOENT || !first) {
					LibFatal(open, rmFname);
				}
				first = FALSE;

				/*
				 * Make the archive directory.
				 */
				MakeDir(RM_FILES_DIR);
				goto retry;
			}
			(void) close(fd);
		}
	}

	VolCur = 0;
	VolBytesWritten = 0;
}


/*
 * Write to removable media.
 */
ssize_t
RmWrite(
	int fd,
	void *buf,
	size_t nbytes)
{
	ssize_t	bytesWritten;

#if defined(AR_DEBUG)

	/*
	 * Simulate a write error.
	 * Uses files in directory CopyErrors.
	 * mtf -s 1M CopyErrors/'dir[1-5]/file[0-9]'
	 */
	if (strncmp(File->f->FiName, "CopyErrors/", 11) == 0) {
		if (VolBytesWritten >= 25600000) {
			errno = EIO;
			Trace(TR_DEBUGERR, "ERRsim write(%s)", ArVolName);
			return (-1);
		}
	}
#endif /* defined(AR_DEBUG) */

#if defined(DEBUG)

	/*
	 * Simulated volume overflow.
	 * tstovfl simulates an overflow at ovflmin.
	 */
	if (ArchiveSet->AsDbgFlags & ASDBG_tstovfl) {
		fsize_t ovflmin;

		if (ArchiveSet->AsFlags & AS_ovflmin) {
			ovflmin = ArchiveSet->AsOvflmin;
		} else {
			struct MediaParamsEntry *mp;

			mp = MediaParamsGetEntry(
			    VolsTable->entry[VolCur].Vi.VfMtype);
			if (mp != NULL) {
				ovflmin = mp->MpOvflmin;
			} else {
				ovflmin = 1024 * 1024;
			}
		}
		if (VolBytesWritten > ovflmin) {
			errno = ENOSPC;
			return (-1);
		}
	}

	/*
	 * Simulated archival.
	 */
	if (Instance->CiFlags & CI_sim) {
		struct VolsEntry *ve;
		fsize_t	space;

		if (ArchiveSet->AsDbgFlags & ASDBG_simdelay &&
		    VolBytesWritten % (1024 * 1024) == 0) {
			sleep(ArchiveSet->AsDbgSimDelay);
		}

		/*
		 * Check space in volume.
		 */
		ve = &VolsTable->entry[VolCur];
		space = ve->Vi.VfSpace;
		if (ArchiveSet->AsDbgFlags & ASDBG_simeod) {
			space *= (double)ArchiveSet->AsDbgSimEod / 100;
		}
		if (space < (VolBytesWritten + nbytes)) {
			/*
			 * Overflowed volume.
			 */
			if (CatalogSetField(&vid, CEF_Space, 0, 0) == -1) {
				Trace(TR_MISC, "CatalogSetField(%s.%s) failed",
				    vid.ViMtype, vid.ViVsn);
			}
			errno = ENOSPC;
			return (-1);
		}
	}
#endif /* defined(DEBUG) */

	bytesWritten = write(fd, buf, nbytes);
	Trace(TR_DEBUG, "RmWrite: fd %d buf %#x size %d written %d", fd, buf, nbytes, bytesWritten);
	if (bytesWritten == nbytes) {
		if (maybeEOM) {
			maybeEOM = FALSE;
			Trace(TR_ERR, "Drive %s reported recovered error.",
			    ArVolName);
		}
	} else if ((ArchiveMedia == DT_3570 || ArchiveMedia == DT_3590 ||
	    ArchiveMedia == DT_3592) && !maybeEOM && bytesWritten == 0) {

		/*
		 * An IBM drive reports recovered errors and
		 * mess up EOM processing.  Hold a flag indication
		 * possible EOM and issue the write again.  If it
		 * works and the next write fails we are probably at
		 * eom.
		 */
		bytesWritten = write(fd, buf, nbytes);
		if (bytesWritten == nbytes) {
			maybeEOM = TRUE;
		}
	}
	return (bytesWritten);
}


/*
 * Write error.
 */
int
RmWriteError(void)
{
	if (errno == ENOSPC) {
		endOfVolume();
		return (0);
	}
	if (errno == ETIME) {
		Trace(TR_ERR, "Write timeout: %s", ArVolName);
	} else if (errno == EIO) {
		if (CatalogSetField(&vid, CEF_Status,
		    CES_bad_media, CES_bad_media) == -1) {
			Trace(TR_ERR, "Catalog set bad media failed: %s.%s",
			    vid.ViMtype, vid.ViVsn);
		} else {
			/* Set bad media for %s.%s */
			SendCustMsg(HERE, 4048, vid.ViMtype, vid.ViVsn);
		}
	}
	return (-1);
}


/* Private functions. */


/*
 * Process end of volume.
 * Unload volume and update position.
 * Request an additional volume.
 */
static void
endOfVolume(void)
{
	struct VolsEntry *ve;
	struct sam_ioctl_rmunload rmunload;
	char	save_oprmsg[OPRMSG_SIZE];
	size_t	size;

	ve = &VolsTable->entry[VolCur];
	/* Volume full: %s.%s: %s bytes written */
	SendCustMsg(HERE, 4043, ve->Vi.VfMtype, ve->Vi.VfVsn,
	    CountToA(VolBytesWritten));

	/*
	 * Mark volume full in the catalog.
	 */
	if (!(ArchiveSet->AsDbgFlags & ASDBG_tstovfl)) {
		setVolumeFull();
	}

	/*
	 * Unload the volume and get position.
	 */
	if (!(Instance->CiFlags & CI_sim)) {
		rmunload.flags = UNLOAD_EOX;
		if (RemoteArchive.enabled) {
			if (SamrftUnloadVol(RemoteArchive.rft, &rmunload) < 0) {
				IoFatal(SamrftUnloadVol, "");
			}
		} else {
			if (ioctl(AfFd, F_UNLOAD, &rmunload) < 0) {
				IoFatal(ioctl:F_UNLOAD, ArVolName);
			}
		}
		ve->VlPosition = rmunload.position;
	} else {
		ve->VlPosition = CI_sim;
	}
	if (!RemoteArchive.enabled) {
		if (close(AfFd) != 0) {
			IoFatal(close, ArVolName);
		}
	}
	AfFd = -1;

	/*
	 * This must be the first file in a tarball.
	 */
	if (!(File->f->FiFlags & FI_first)) {
		Trace(TR_ERR, "Media full - not first tarball file: %s.%s",
		    ve->Vi.VfMtype, ve->Vi.VfVsn);
		exit(EXIT_FAILURE);
	}

	/*
	 * Request next volume.
	 */
	memmove(save_oprmsg, Instance->CiOprmsg, sizeof (save_oprmsg));
	Instance->CiFlags |= CI_volreq;
	if (ArchiverRequestVolume(ArName, File->AfFileSize) == -1) {
		Trace(TR_ERR, "Media full: %s.%s",
		    ve->Vi.VfMtype, ve->Vi.VfVsn);
		exit(EXIT_FAILURE);
	}

	/* Waiting for volume %s */
	PostOprMsg(4307, "overflow");
	while (Instance->CiFlags & CI_volreq) {
		ThreadsSleep(1);
	}
	/* Restore message */
	memmove(Instance->CiOprmsg, save_oprmsg, sizeof (Instance->CiOprmsg));

	/*
	 * New volume is returned in the ArchReq.
	 * If volume filled had data written, add new volume to volume list.
	 * Otherwise, just replace the current volume.
	 */
	if (ve->VlLength != 0) {
		VolCur++;
		VolsTable->count++;
		size = sizeof (struct Vols) +
		    ((VolsTable->count - 1) * sizeof (struct VolsEntry));
		SamRealloc(VolsTable, size);
		ve = &VolsTable->entry[VolCur];
	}
	memset(ve, 0, sizeof (struct VolsEntry));
	strncpy(ve->Vi.VfMtype, Instance->CiMtype, sizeof (ve->Vi.VfMtype));
	strncpy(ve->Vi.VfVsn, Instance->CiVsn, sizeof (ve->Vi.VfVsn));
	/* Overflowing to volume %d: %s.%s */
	SendCustMsg(HERE, 4044, VolCur + 1, ve->Vi.VfMtype, ve->Vi.VfVsn);

	/*
	 * Load the volume and return to continue writing.
	 */
	loadVolume();
	ve->VlLength = 0;
	VolBytesWritten = 0;
}


/*
 * Get volume information.
 */
static int
getVolInfo(
	struct VolsEntry *ve)
{
	struct CatalogEntry ced;

	vid.ViFlags = VI_logical;
	strncpy(vid.ViMtype, ve->Vi.VfMtype, sizeof (vid.ViMtype));
	strncpy(vid.ViVsn, ve->Vi.VfVsn, sizeof (vid.ViVsn));
	if (CatalogGetCeByMedia(ve->Vi.VfMtype, ve->Vi.VfVsn, &ced) == NULL) {
		return (-1);
	}
	ve->Vi.VfSlot = ced.CeSlot;
	ve->Vi.VfSpace = ced.CeSpace * 1024;
	if (ced.r.CerTime != 0) {
		ve->Vi.VfFlags |= VF_reserved;
	}
	return (0);
}


/*
 *	Initialize archiving to removable media on a remote host.
 */
static void
initRemoteArchive()
{
	struct dev_ent *library;
	srvr_clnt_t *server;
	int af;

	RemoteArchive.enabled = TRUE;
	library = (struct dev_ent *)SHM_REF_ADDR(Instance->CiLibDev);
	ASSERT(library->equ_type == DT_PSEUDO_SC);
	server = (srvr_clnt_t *)SHM_REF_ADDR(library->dt.sc.server);
	if (server->flags & SRVR_CLNT_IPV6) {
		af = AF_INET6;
	} else {
		af = AF_INET;
	}
	RemoteArchive.host = SamrftGetHostByAddr(&server->control_addr, af);
	if (RemoteArchive.host == NULL) {
		LibFatal(SamrftGetHostByAddr, library->name);
	}

	/*
	 *	Establish connection to remote host.
	 */
	RemoteArchive.rft = (void *)SamrftConnect(RemoteArchive.host);
	if (RemoteArchive.rft == NULL) {
		LibFatal(SamrftConnect, RemoteArchive.host);
	}
	Trace(TR_ARDEBUG, "Connected to %s", RemoteArchive.host);
}


/*
 * Load volume.
 * Request the current volume and open the removable media file.
 * Set BlockSize and WriteCount.
 */
static void
loadVolume(void)
{
	struct VolsEntry *ve;
	struct MediaParamsEntry *mp;
	dev_ent_t *dev;
	struct sam_rminfo rb;
	struct sam_ioctl_getrminfo sr;
	char	save_oprmsg[OPRMSG_SIZE];
	int	oflag;
	int	prevBlockSize;

	if (AfFd >= 0) {
		return;
	}

	ve = &VolsTable->entry[VolCur];
	snprintf(ArVolName, sizeof (ArVolName), "%s.%s", ve->Vi.VfMtype,
	    ve->Vi.VfVsn);

	/*
	 * Get the current information.
	 */
	if (getVolInfo(ve) == -1) {
		LibFatal(getVolInfo, ArVolName);
	}
	if (*TraceFlags & (1 << TR_misc)) {
		FILE *st;

		Trace(TR_MISC, "request %s", ArVolName);
		st = TraceOpen();
		if (st != NULL) {
			fsize_t space;

			fprintf(st, "   ");
			space = ve->Vi.VfSpace;	/* PrintVolInfo adjusts space */
			PrintVolInfo(st, &ve->Vi);
			ve->Vi.VfSpace = space;
			TraceClose(-1);
		}
	}

	/*
	 * Update the operator information.
	 * Save the present message and display a "request" message.
	 */
	memmove(save_oprmsg, Instance->CiOprmsg, sizeof (save_oprmsg));
	/* Waiting for volume %s */
	PostOprMsg(4307, ArVolName);

	/*
	 * Request and open the removable media file.
	 */
	oflag = O_WRONLY | O_CREAT | O_TRUNC | SAM_O_LARGEFILE;
	memset(&rb, 0, SAM_RMINFO_SIZE(1));
	rb.flags = RI_blockio;
	strncpy(rb.file_id, ArchReq->ArAsname, sizeof (rb.file_id));
	strncpy(rb.owner_id, SAM_ARCHIVE_OWNER, sizeof (rb.owner_id));
	strncpy(rb.group_id, SAM_ARCHIVE_GROUP, sizeof (rb.group_id));
	rb.n_vsns = 1;
	strncpy(rb.media, ve->Vi.VfMtype, sizeof (rb.media)-1);
	memmove(rb.section[0].vsn, ve->Vi.VfVsn, sizeof (rb.section[0].vsn));
	prevBlockSize = BlockSize;
	SetTimeout(TO_request);
	if (RemoteArchive.enabled) {
		struct sam_rminfo getrm;

		if (SamrftLoadVol(RemoteArchive.rft, &rb, oflag) < 0) {
			IoFatal(SamrftLoadVol, ArVolName);
		}

		/*
		 * Get the removable media information.
		 */
		if (SamrftGetVolInfo(RemoteArchive.rft, &getrm, &ve->VlEq) <
		    0) {
			IoFatal(SamrftLoadVol, ArVolName);
		}
		BlockSize = getrm.block_size;

	} else if (!(Instance->CiFlags & CI_sim)) {
		if (sam_request(rmFname, &rb, SAM_RMINFO_SIZE(rb.n_vsns)) ==
		    -1) {
			IoFatal(request, ArVolName);
		}

retryOpen:
		if ((AfFd = open(rmFname, oflag, FILE_MODE)) < 0) {
			if (errno == EINTR) {
				if (Exec == ES_run) {
					goto retryOpen;
				}
				ThreadsExit();
				/*NOTREACHED*/
			}
			if (errno == ENOSPC) {
				setVolumeFull();
			}
			IoFatal(open, ArVolName);
		}

		/*
		 * Get the removable media information.
		 */
		sr.bufsize = SAM_RMINFO_SIZE(rb.n_vsns);
		sr.buf.ptr = &rb;
		if (ioctl(AfFd, F_GETRMINFO, &sr) < 0) {
			if (errno != EOVERFLOW) {
				IoFatal(ioctl:F_GETRMINFO, ArVolName);
			}
		}
		BlockSize = rb.block_size;
		if (BlockSize == 0) {
			Trace(TR_MISC, "Block size 0: %s", ArVolName);
			BlockSize = 16 * 1024;
		}
	} else {
		if ((AfFd = open("/dev/null", O_WRONLY)) < 0) {
			LibFatal(open, "/dev/null");
		}
		BlockSize = (VolCur + 1) * 1024 * 1024;
	}
	ClearTimeout(TO_request);

	mp = MediaParamsGetEntry(VolsTable->entry[VolCur].Vi.VfMtype);
	WriteTimeout = mp->MpTimeout;

	ArchiveMedia = sam_atomedia(ve->Vi.VfMtype);
	if ((ArchiveMedia & DT_CLASS_MASK) == DT_OPTICAL) {
		/*
		 * Optical media writes are blocked by the driver.
		 */
		WriteCount = OD_BS_DEFAULT;
	} else {
		/*
		 * Tape media must be written in block size writes.
		 */
		if (BlockSize < (16 * 1024)) {
			BlockSize = 16 * 1024;
		}
		WriteCount = BlockSize;
	}

	/*
	 * Look up drive number.
	 */
	for (dev = (struct dev_ent *)SHM_REF_ADDR(shm_ptr_tbl->first_dev);
	    dev != NULL;
	    dev = (struct dev_ent *)SHM_REF_ADDR(dev->next)) {
		if (dev->type == ArchiveMedia &&
		    strcmp(dev->vsn, ve->Vi.VfVsn) == 0) {
			ve->VlEq = dev->eq;
			break;
		}
	}

	snprintf(ArVolName, sizeof (ArVolName), "eq: %d  %s.%s", ve->VlEq,
	    ve->Vi.VfMtype, ve->Vi.VfVsn);
	/* Restore message */
	memmove(Instance->CiOprmsg, save_oprmsg, sizeof (Instance->CiOprmsg));
	if (prevBlockSize != 0 && prevBlockSize != BlockSize) {
		Trace(TR_MISC, "Block size changed from %d to %d",
		    prevBlockSize, BlockSize);
		if (prevBlockSize < BlockSize) {
			CopyFileReconfig();
		}
	}
}


/*
 * Reserve a volume to an archive set.
 */
static void
reserveVolume(
	struct VolInfo *vi,
	struct ArchSet *as,
	char *owner_a,
	char *fsname_a)
{
	struct VolId vid;
	struct CatalogEntry ced;
	char	*asname;
	char	*owner;
	char	*fsname;

	if (as == NULL || as->AsReserve == 0) {
		return;
	}

	if (as->AsReserve & RM_set) {
		asname = as->AsName;
	} else {
		asname = "";
	}
	if (as->AsReserve & RM_owner) {
		owner = owner_a;
	} else {
		owner = "";
	}
	if (as->AsReserve & RM_fs) {
		fsname = fsname_a;
	} else {
		fsname = "";
	}
	if (*asname == '\0' && *owner == '\0' && *fsname == '\0') {
		Trace(TR_DEBUG, "Empty reservation for %s %s.%s", as->AsName,
		    vi->VfMtype, vi->VfVsn);
		return;
	}

	/*
	 * Check for previous reservation.
	 */
	if (CatalogGetCeByMedia(vi->VfMtype, vi->VfVsn, &ced) == NULL) {
		Trace(TR_MISC, "Archive set %s: Volume %s.%s not found.",
		    asname, vi->VfMtype, vi->VfVsn);
		return;
	}
	if (ced.r.CerTime != 0) {
		Trace(TR_MISC,
		    "Archive set %s: Volume %s.%s already reserved to %s/%s/%s",
		    asname, vi->VfMtype, vi->VfVsn,
		    ced.r.CerAsname, ced.r.CerOwner, ced.r.CerFsname);
		return;
	}

	/*
	 * Reserve volume.
	 */
	strncpy(vid.ViMtype, ced.CeMtype, sizeof (vid.ViMtype));
	strncpy(vid.ViVsn, ced.CeVsn, sizeof (vid.ViVsn));
	vid.ViFlags = VI_logical;
	if (CatalogReserveVolume(&vid, 0, asname, owner, fsname) == -1) {
		Trace(TR_ERR, "ReserveVolume failed(%s.%s)",
		    vid.ViMtype, vid.ViVsn);
		return;
	}
	Trace(TR_MISC, "Volume %s.%s reserved to %s/%s/%s",
	    vi->VfMtype, vi->VfVsn, asname, owner, fsname);
	vi->VfFlags |= VF_reserved;
}



/*
 * Set volume full.
 */
static void
setVolumeFull(void)
{
	if (CatalogSetField(&vid, CEF_Status, CES_archfull, CES_archfull) ==
	    -1) {
		Trace(TR_ERR, "Catalog set full failed: %s.%s", vid.ViMtype,
		    vid.ViVsn);
	}
}


/*
 * Unload removable media file and get removable media information.
 */
static void
unloadVolume(void)
{
	struct VolsEntry *ve;
	struct sam_ioctl_rmunload rmunload;

	ve = &VolsTable->entry[VolCur];
	rmunload.flags = 0;
	if (TapeNonStop) {

		/*
		 * For tape media, only write a tape mark between each archive
		 * file.  This avoids repositioning the tape to write over
		 * the EOF labels.
		 */
		rmunload.flags |= UNLOAD_WTM;
	}

	if (RemoteArchive.enabled) {
		if (SamrftUnloadVol(RemoteArchive.rft, &rmunload) < 0) {
			IoFatal(SamrftUnloadVol, "");
		}
	} else if (!(Instance->CiFlags & CI_sim)) {
		if (ioctl(AfFd, F_UNLOAD, &rmunload) < 0) {
			Trace(TR_ERR, "ioctl rmunload failed: fd=%d, err=%d", AfFd, errno);
			LibFatal(ioctl:F_UNLOAD, ArVolName);
		}
		if (!(rmunload.flags & UNLOAD_WTM)) {
			if (close(AfFd) != 0) {
				IoFatal(close, ArVolName);
			}
			AfFd = -1;
		}
	} else {
		/*
		 * Update the catalog space for the simulated archive.
		 */
		if (CatalogSetField(&vid, CEF_Space,
		    (ve->Vi.VfSpace - (VolBytesWritten + 1023)) / 1024,
		    0) == -1) {
			Trace(TR_MISC, "CatalogSetField(%s.%s) failed",
			    vid.ViMtype, vid.ViVsn);
		}
		rmunload.position = CI_sim;
	}
	ve->VlPosition = rmunload.position;
}
