/*
 * stage_reqs.c - storage and access to active stage requests
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

#pragma ident "$Revision: 1.89 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/param.h>

/* Solaris headers. */

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/shm.h"
#include "pub/stat.h"
#include "sam/syscall.h"
#include "sam/uioctl.h"
#include "aml/id_to_path.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "aml/stager.h"
#include "sam/lib.h"
#include "sam/custmsg.h"
#include "aml/diskvols.h"
#include "sam/nl_samfs.h"
#include "pub/sam_errno.h"
#include "sam/exit.h"
#include "aml/stager_defs.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Local headers. */
#include "stager_lib.h"
#include "stager_threads.h"
#include "stager_config.h"
#include "stager_shared.h"
#include "copy_defs.h"
#include "stage_done.h"
#include "copy_defs.h"

#include "stager.h"
#include "stage_reqs.h"
#include "schedule.h"
#include "thirdparty.h"

static int addRequest(FileInfo_t *file);
static void markFree(int index);
static int getFree();
static int isFree(int index);
static void createRequestMapFile(char *file_name);
static int getFileExtent(sam_id_t id, int ext_ord, equ_t fseq,
	sam_stage_request_t *req, FileExtentInfo_t *fe);
static void recoverFileExtent();
static void checkFileExtent();
static void deleteFileExtent(sam_id_t id, int ext_ord, equ_t fseq);
static boolean_t createMultiVolume(sam_stage_request_t *req);
static int waitForStageDone();
static FileInfo_t *findNextRequest(FileInfo_t *file);
static void traceFileExtents(int flag, char *srcFile, int srcLine);
static void createFileExtent();
static int recoverRequestList();
static boolean_t valRequest(int id);

static char pathBuffer[PATHBUF_SIZE];

/*
 * External.
 */
extern CopyInstanceList_t *CopyInstanceList;
extern enum StartMode StartMode;

/*
 * This structure contains all stage requests in progress.
 * The request list is implemented as an index file that is mapped to
 * the process's address space.
 */
StageReqs_t StageReqs = {
	0, 0, NULL, -1, -1,
	PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER,
	NULL, NULL
};

/*
 * This structure contains extension information to the stage
 * requests in progress.  As example, extensions are used to hold
 * addition volume sections for multivolume stage requests.  Extensions
 * are being used to reduce memory usage in the FileInfo structure.
 * From 4.6, this structure is written to the disk to allow sam-stagerd
 * to recover the pending requests, and to support HA-SAM.
 */

static struct {
	pthread_mutex_t		mutex;		/* protect access */
	FileExtentHdrInfo_t	*hdr;
	FileExtentInfo_t	*data;
} stageExtents = { PTHREAD_MUTEX_INITIALIZER, NULL, NULL };

/*
 * This structure is used for communicating stage completitions
 * from copy procs so request list space can be freed in a timely
 * fashion.
 */
StageDoneInfo_t *stageDone = NULL;

/*
 * Create an entry for file that is to be staged.
 * BE CAREFUL.  This function returns a pointer to static data.
 * It assumes a copy of the file information will be made by
 * the caller.
 */
FileInfo_t *
CreateFile(
	sam_stage_request_t *req)
{
	int i;
	static FileInfo_t fi;
	pthread_mutexattr_t mattr;

	(void) memset(&fi, 0, sizeof (FileInfo_t));

	fi.id = req->id;
	fi.fseq = req->fseq;

	/*
	 * Set directio directive
	 */
	if (GetCfgDirectio) {
		fi.directio = 1;
	}

	if (req->arcopy[req->copy].flags & STAGE_COPY_ARCHIVED) {
		Trace(TR_MISC, "Request inode: %d.%d "
		    "len: %lld offset: %lld copy: %d vsn: '%s.%s'",
		    req->id.ino, req->id.gen, req->len, req->offset,
		    req->copy + 1, sam_mediatoa(req->arcopy[req->copy].media),
		    req->arcopy[req->copy].section[0].vsn);
	} else {
		Trace(TR_MISC, "Request inode: %d.%d "
		    "len: %lld offset: %lld copy: %d",
		    req->id.ino, req->id.gen, req->len, req->offset,
		    req->copy + 1);
	}

#ifdef DEBUG
	for (i = 0; i < MAX_ARCHIVE; i++) {
		if (req->arcopy[i].flags != 0) {
			int j;
			Trace(TR_DEBUG, "Request archive copy: %d "
			    "media: %d n_vsns %d flags: 0x%x ext_ord %d",
			    i, req->arcopy[i].media, req->arcopy[i].n_vsns,
			    req->arcopy[i].flags, req->arcopy[i].ext_ord);
			for (j = 0; j < SAM_MAX_VSN_SECTIONS; j++) {
				if (*TraceFlags & (1 << TR_debug) &&
				    req->arcopy[i].section[j].vsn[0] != '\0') {
					Trace(TR_DEBUG, "\tpos: %llx.%llx "
					    "len: %lld vsn: '%s'",
					    req->arcopy[i].section[j].position,
					    req->arcopy[i].section[j].offset,
					    req->arcopy[i].section[j].length,
					    req->arcopy[i].section[j].vsn);
				}
			}
		}
	}
#endif

	for (i = 0; i < MAX_ARCHIVE; i++) {
		fi.ar[i].media = req->arcopy[i].media;
		fi.ar[i].n_vsns = req->arcopy[i].n_vsns;
		fi.ar[i].flags = req->arcopy[i].flags;
		fi.ar[i].ext_ord = req->arcopy[i].ext_ord;

		/*
		 * Only one vsn section saved here.
		 */
		fi.ar[i].section.position = req->arcopy[i].section[0].position;
		fi.ar[i].section.offset = req->arcopy[i].section[0].offset;
		fi.ar[i].section.length = req->arcopy[i].section[0].length;
		(void) strcpy(fi.ar[i].section.vsn,
		    req->arcopy[i].section[0].vsn);

		if (req->arcopy[i].flags != 0) {
			Trace(TR_DEBUG, "Request archive copy: %d\n"
			    "\tmedia: %d n_vsns %d flags: 0x%x ext_ord %d\n"
			    "\tpos: %llx.%llx len: %lld vsn: '%s'",
			    i, req->arcopy[i].media, req->arcopy[i].n_vsns,
			    req->arcopy[i].flags, req->arcopy[i].ext_ord,
			    req->arcopy[i].section[0].position,
			    req->arcopy[i].section[0].offset,
			    req->arcopy[i].section[0].length,
			    req->arcopy[i].section[0].vsn);
		}
	}

	fi.copy = req->copy;
	fi.len = req->len;
	fi.offset = req->offset;

	/*
	 * If copy made with pax tar header.  If pax the copy pos/offset
	 * points to tar hdr, not data.
	 */
	if (req->arcopy[fi.copy].flags & STAGE_COPY_PAXHDR) {
		SET_FLAG(fi.flags, FI_PAX_TARHDR);
	}

	/*
	 * If disk/honeycomb archiving, this flag indicates no
	 * tar header was built.
	 */
	if ((req->arcopy[fi.copy].flags & STAGE_COPY_DISKARCH) &&
	    (req->arcopy[fi.copy].media == DT_STK5800)) {
		SET_FLAG(fi.flags, FI_NO_TARHDR);
	}

	/*
	 * If offset is set, this is a stage request that
	 * does not start at beginning of file and thus no
	 * there is no tar header to verify for this request.
	 */
	if (req->offset > 0) {
		SET_FLAG(fi.flags, FI_NO_TARHDR);
	}

	/*
	 * Check if an extended request.  Extended requests are used
	 * for multivolume stages which exceed the maximum number of vsns
	 * per request.
	 */
	if (req->flags & STAGE_EXTENDED) {
		SET_FLAG(fi.flags, FI_EXTENDED);
	}

	/*
	 * Set file's checksum attributes.
	 */
	if (req->flags & STAGE_CSUSE) {
		char *filename;
		SET_FLAG(fi.flags, FI_USE_CSUM);
		fi.csum_algo = req->cs_algo;
		GetFileName(&fi, pathBuffer, PATHBUF_SIZE, NULL);
		fi.namelen = strlen(pathBuffer);
		filename = strrchr(pathBuffer, '/');
		if (filename != NULL) {
			fi.namelen = strlen(filename);
		}
	}

	/*
	 * Set if stage never attribute.
	 */
	if (req->flags & STAGE_NEVER) {
		SET_FLAG(fi.flags, FI_STAGE_NEVER);
	}

	/*
	 * Set if partial stage attribute.
	 */
	if (req->flags & STAGE_PARTIAL) {
		SET_FLAG(fi.flags, FI_STAGE_PARTIAL);
	}

	/*
	 * Set if data verification.
	 */
	if (req->arcopy[fi.copy].flags & STAGE_COPY_VERIFY) {
		SET_FLAG(fi.flags, FI_DATA_VERIFY);
	}

	fi.fs = req->filesys;

	/*
	 * Pid and uid of requestor.
	 */
	fi.pid = req->pid;
	fi.user = req->user;

	/*
	 * Owner's user and group id.
	 */
	fi.owner = req->owner;
	fi.group = req->group;

	fi.dcache = -1;
	fi.sort = fi.next = -1;
	fi.retry = GetCfgMaxRetries();

	/*
	 * Set a magic for validation at recovery after an abnormal
	 * termination.
	 */
	fi.magic = STAGER_REQUEST_MAGIC;

	PthreadMutexattrInit(&mattr);
	PthreadMutexattrSetpshared(&mattr, PTHREAD_PROCESS_SHARED);
	PthreadMutexInit(&(fi.mutex), &mattr);
	PthreadMutexattrDestroy(&mattr);

	return (&fi);
}

/*
 * Create new entry for a staging request and add to list. The request
 * list contains all stage requests in progress.
 */
int
AddFile(
	sam_stage_request_t *req,
	int *status)
{
	FileInfo_t *file;
	int copy;
	int id = -1;

	*status = REQUEST_ERROR;

	/*
	 * Create entry for file to be stage.  Be careful.
	 * CreateFile returns pointer to static data, a copy
	 * must be made.
	 */
	file = CreateFile(req);
	ASSERT(file != NULL);

	/*
	 * Verify that a valid copy exist.
	 */
	if (GetArcopy(file, file->copy) < 0) {

		StageError(file, EINVAL);
		GetFileName(file, &pathBuffer[0], PATHBUF_SIZE, NULL);

		Trace(TR_MISC, "Cannot stage file: all copies are damaged '%s'",
		    pathBuffer);
		SendCustMsg(HERE, 19033, pathBuffer);

		return (-1);
	}

	id = addRequest(file);

	/*
	 * Make sure request was added successfully.  If not, an error
	 * should be returned in status.
	 */
	if (id < 0) {
		*status = REQUEST_LIST_FULL;
		Trace(TR_FILES, "Request list full");

	} else {
		*status = REQUEST_READY;

		/*
		 * If multivolume, create a multivolume request.  If any copy
		 * contains multiple volumes an extension will be created
		 * in createMultiVolume to hold addition volume sections.
		 */
		file = GetFile(id);
		if (createMultiVolume(req) == B_TRUE) {
			SET_FLAG(file->flags, FI_EXTENSION);
			file->se_ord = 0;
		}

		/*
		 * Set archive copy to stage from.
		 */
		copy = GetArcopy(file, file->copy);
		if (copy != file->copy) {

			file->copy = copy;
			Trace(TR_FILES, "Set copy inode: %d.%d "
			    "len: %lld offset: %lld copy: %d vsn: '%s.%s'",
			    file->id.ino, file->id.gen,
			    file->len, file->offset,
			    copy + 1, sam_mediatoa(file->ar[copy].media),
			    file->ar[copy].section.vsn);
		}

		ASSERT(file->copy >= 0 && file->copy < MAX_ARCHIVE);

		/*
		 * Get permanent entry in stage request list.  Set multivolume
		 * flag if the archive copy we are staging from is multivolume.
		 */
		if (file->ar[copy].n_vsns > 1) {
			SET_FLAG(file->flags, FI_MULTIVOL);
		}

		/*
		 * If an extended request for multivolume its not scheduled yet.
		 */
		if (GET_FLAG(file->flags, FI_EXTENDED)) {
			*status = REQUEST_EXTENDED;
		}
	}

	return (id);
}

/*
 * Cancel a stage file request.
 */
void
CancelRequest(
	sam_stage_request_t *req)
{
	int i;
	int copy;
	FileInfo_t *file;

	if (StageReqs.entries > 0) {
		int min_ext = INT_MAX;
		int cid = -1;

		/*
		 * No choice but to scan complete list.  FIXME.
		 */
		for (i = 0; i < StageReqs.alloc; i++) {
			file = &StageReqs.data[i];
			copy = file->copy;

			if (req->id.ino == file->id.ino &&
			    req->fseq == file->fseq &&
			    (GET_FLAG(file->flags, FI_DUPLICATE) == 0)) {

				if (is_third_party(file->ar[copy].media)) {
					CancelThirdPartyRequest(i);
				} else if (GET_FLAG(file->flags, FI_MULTIVOL)) {
					PthreadMutexLock(&file->mutex);
					SET_FLAG(file->flags, FI_CANCEL);
					PthreadMutexUnlock(&file->mutex);
					if (file->ar[copy].ext_ord < min_ext) {
						min_ext =
						    file->ar[copy].ext_ord;
						cid = i;
					}
				} else {
					cid = i;
					break;
				}
			}
		}
		if (cid >= 0) {
			CancelWork(cid);
		}
	}
}

/*
 * Error a stage file request.
 */
void
ErrorRequest(
	sam_stage_request_t *req,
	int error)
{
	FileInfo_t file;

	file.id = req->id;
	file.fseq = req->fseq;

	file.copy = req->copy;
	file.fs = req->filesys;
	file.len =  req->len;

	StageError(&file, error);
}

/*
 * Get pointer to stage file information for a specific
 * request identifier.
 */
FileInfo_t *
GetFile(
	int id)
{
	FileInfo_t *file = NULL;
	if (id >= 0) {
		file = &StageReqs.data[id];
	}
	return (file);
}

/*
 * Check status of stage file requests.  Requests which have
 * successfully completed are removed from the request list.
 *
 * If error, return error status and request identifier to caller.
 *
 * If multivolume, prepare next volume and return multivolume status,
 * return identifier to caller.
 */
int
CheckRequests(
	int *id)
{
	extern FILE *LogFile;

	int err = CHECK_REQUEST_SUCCESS;
	int i;
	int copy;
	int savecopy;
	FileInfo_t *file;

	i = waitForStageDone();

	if (i >= 0) {
		ASSERT(isFree(i) == FALSE);
		file = &StageReqs.data[i];

		if (GET_FLAG(file->flags, FI_DONE)) {

			/*
			 * Generate staging log file entry.
			 */
			if (LogFile != NULL) {
				uint_t flags;
				LogType_t logtype;

				logtype = LOG_STAGE_DONE;
				flags = file->flags;

				if (GET_FLAG(flags, FI_CANCEL)) {
					if (GET_FLAG(flags, FI_WRITE_ERROR) ||
					    GET_FLAG(flags, FI_TAR_ERROR)) {
						logtype = LOG_STAGE_ERROR;
					} else {
						logtype = LOG_STAGE_CANCEL;
					}
				} else if (file->error) {
					logtype = LOG_STAGE_ERROR;
				}
				LogIt(logtype, file);
			}
			savecopy = file->copy;

			/*
			 * If copy proc exhausted retry attempts for an
			 * archive copy, damage copy and check if there is
			 * another copy available to stage from.
			 */
			if (file->error) {
				boolean_t damaged;

				damaged = DamageArcopy(file);
				copy = GetArcopy(file, file->copy + 1);

				/*
				 * If copy was NOT damaged, make sure another
				 * good copy was found.  If only the same copy
				 * is available error the stage request.
				 * If multivolume and not section 0, don't allow
				 * retry from another copy since extent has
				 * removed or type of media may different.
				 */
				if (damaged == B_FALSE && copy == file->copy ||
				    GET_FLAG(file->flags, FI_NO_RETRY) ||
				    file->ar[file->copy].ext_ord != 0 ||
				    file->se_ord != 0) {
					/* another copy not available */
					file->copy = -1;
				} else {
					file->copy = copy;
					Trace(TR_FILES, "%s inode: %d.%d "
					    "error: %d next copy: %d",
					    damaged ? "Damaged" : "Error",
					    file->id.ino, file->id.gen,
					    file->error, file->copy + 1);
				}

				if (file->copy >= 0) {
					/*
					 * Another copy is available to stage
					 * from. Clear flags and reschedule.
					 */
					CLEAR_FLAG(file->flags, FI_ACTIVE);
					CLEAR_FLAG(file->flags, FI_DONE);
					SET_FLAG(file->flags, FI_RETRY);

					file->retry = GetCfgMaxRetries();
					file->error = 0;
					file->stage_size = 0;

					*id = i;
					err = CHECK_REQUEST_SCHED;

					Trace(TR_FILES,
					    "Resched inode: %d.%d "
					    "len: %lld offset: %lld copy: %d "
					    "vsn: '%s.%s'",
					    file->id.ino, file->id.gen,
					    file->len, file->offset,
					    file->copy + 1,
					    sam_mediatoa(
					    file->ar[file->copy].media),
					    file->ar[file->copy].section.vsn);
				}

			} else if (GET_FLAG(file->flags, FI_MULTIVOL) &&
			    GET_FLAG(file->flags, FI_CANCEL) == 0) {

				FileExtentInfo_t extent;
				int se_ord;

				if (GET_FLAG(file->flags, FI_STAGE_NEVER) &&
				    file->stage_size == file->len) {
					goto done;
				}

				/*
				 * Multivolume, prepare for next volume.
				 */
				copy = file->copy;
				if (getFileExtent(
				    file->id, file->ar[copy].ext_ord,
				    file->fseq, NULL, &extent) < 0) {
					SysError(HERE,
					    "Failed to find multivolume extent"
					    " inode: %d.%d ext_ord: %d",
					    file->id.ino, file->id.gen,
					    file->ar[copy].ext_ord);
					exit(EXIT_FAILURE);
				}
				Trace(TR_FILES,
				    "Get file extent: inode: %d.%d "
				    "ext_ord: %d",
				    file->id.ino, file->id.gen,
				    file->ar[copy].ext_ord);

				ASSERT(file->ar[copy].n_vsns >=
				    file->se_ord + 1);
				file->se_ord++;

				if (file->se_ord == SAM_MAX_VSN_SECTIONS) {
					FileInfo_t *next;
					next = findNextRequest(file);
					ASSERT(next != NULL);

					CLEAR_FLAG(file->flags, FI_ACTIVE);
					CLEAR_FLAG(file->flags, FI_MULTIVOL);

					/*
					 * Copy data maintained for an opened
					 * disk cache file to next file
					 * descriptor.
					 */
					next->dcache    = file->dcache;
					next->write_off = file->write_off;
					next->error	= file->error;
					next->vsn_cnt   = file->vsn_cnt;
					next->context   = file->context;
					SET_FLAG(next->flags, FI_DCACHE);

					if (GET_FLAG(file->flags,
					    FI_EXTENSION)) {
						deleteFileExtent(file->id,
						    file->ar[copy].ext_ord,
						    file->fseq);
					}
					DeleteRequest(i);
					i = next->sort;

					file = next;

					/*
					 * If this request is the last VSN
					 * clear the multivolume flag.
					 */
					if (file->vsn_cnt + 1 ==
					    file->ar[copy].n_vsns) {
						CLEAR_FLAG(file->flags,
						    FI_MULTIVOL);
					}
					file->se_ord = 0;
				} else {
					sam_stage_copy_t *se_ar;

					se_ord = file->se_ord;
					se_ar = &extent.fe_ar[copy];

					file->ar[copy].section.position =
					    se_ar->section[se_ord].position;
					file->ar[copy].section.offset =
					    se_ar->section[se_ord].offset;
					file->ar[copy].section.length =
					    se_ar->section[se_ord].length;
					(void) strcpy(
					    file->ar[copy].section.vsn,
					    se_ar->section[se_ord].vsn);

					/*
					 * Clear active and done flags for
					 * next request.
					 */
					CLEAR_FLAG(file->flags, FI_ACTIVE);
					CLEAR_FLAG(file->flags, FI_DONE);

					/*
					 * If this request is the last VSN
					 * clear the multivolume flag.
					 */
					if (file->vsn_cnt + 1 ==
					    file->ar[copy].n_vsns) {
						CLEAR_FLAG(file->flags,
						    FI_MULTIVOL);
					}
				}

				/*
				 * No tar header to validate.
				 */
				SET_FLAG(file->flags, FI_NO_TARHDR);

				*id = i;
				err = CHECK_REQUEST_SCHED;

			} else if (GET_FLAG(file->flags, FI_CANCEL) &&
			    GET_FLAG(file->flags, FI_DCACHE) &&
			    GET_FLAG(file->flags, FI_DCACHE_CLOSE) == 0) {
				/*
				 * Schedule copy process to close dcache.
				 *
				 * This can happen if multivolume stage has
				 * canceled when waiting for the exported
				 *  volume to be loaded with dcache opened.
				 */
				CLEAR_FLAG(file->flags, FI_DONE);
				SET_FLAG(file->flags, FI_DCACHE_CLOSE);

				*id = i;
				err = CHECK_REQUEST_SCHED;
			}

done:
			if (err == CHECK_REQUEST_SUCCESS) {
				Trace(TR_MISC, "Done file inode: %d.%d",
				    file->id.ino, file->id.gen);

				if (GET_FLAG(file->flags, FI_EXTENSION)) {
					FileInfo_t current, *next;

					/*
					 * Delete extents and requests for
					 * extended file.
					 */
					copy = file->copy = savecopy;
					memcpy(&current, file,
					    sizeof (FileInfo_t));

					deleteFileExtent(file->id,
					    file->ar[copy].ext_ord, file->fseq);
					DeleteRequest(i);

					while ((current.ar[copy].n_vsns >
					    (current.ar[copy].ext_ord + 1) *
					    SAM_MAX_VSN_SECTIONS) &&
					    ((next = findNextRequest(&current))
					    != NULL)) {
						deleteFileExtent(next->id,
						    next->ar[0].ext_ord,
						    next->fseq);
						DeleteRequest(next->sort);
						current.ar[copy].ext_ord++;
					}
				} else {
					DeleteRequest(i);
				}
			}
		}
	}
	return (err);
}

/*
 * Part of shutdown, unlink request list's memory mapped file.
 */
void
RemoveStageReqsMapFile(void)
{
	size_t size;

	if (StageReqs.alloc > 0) {
		size = StageReqs.alloc * sizeof (FileInfo_t);
		RemoveMapFile(SharedInfo->si_stageReqsFile,
		    (void *) StageReqs.data, size);

		StageReqs.alloc = 0;
		StageReqs.data = NULL;
	}
}

/*
 * Part of shutdown, unlink stage done memory mapped file.
 */
void
RemoveStageDoneMapFile(void)
{
	RemoveMapFile(SharedInfo->si_stageDoneFile, (void *) stageDone,
	    sizeof (StageDoneInfo_t));
}

/*
 * Mark file staging as active.
 */
void
SetStageActive(
	FileInfo_t *file)
{
	SET_FLAG(file->flags, FI_ACTIVE);
}

/*
 * Mark file staging as done and add to stage done list
 * so request list space can be reclaimed.
 */
void
SetStageDone(
	FileInfo_t *file)
{
	int id;
	FileInfo_t *last;

	PthreadMutexLock(&file->mutex);
	SET_FLAG(file->flags, FI_DONE);
	PthreadMutexUnlock(&file->mutex);

	Trace(TR_DEBUG, "Set stage done: inode: %d.%d fseq: %d",
	    file->id.ino, file->id.gen, file->fseq);

	id = file->sort;
	PthreadMutexLock(&stageDone->sd_mutex);
	if (stageDone->sd_first == -1) {
		stageDone->sd_first = stageDone->sd_last = id;
	} else {
		last = GetFile(stageDone->sd_last);
		last->next = id;
		stageDone->sd_last = id;
	}
	file->next = -1;
	PthreadCondSignal(&stageDone->sd_cond);
	PthreadMutexUnlock(&stageDone->sd_mutex);
}

/*
 * Trace active stage requests.
 */
void
TraceStageReqs(
	int flag,
	char *srcFile,
	int srcLine)
{
	FileInfo_t *file;
	int i;

	_Trace(flag, srcFile, srcLine,
	    "Stage requests entries: %d alloc: %d data: 0x%x",
	    StageReqs.entries, StageReqs.alloc, (int)StageReqs.data);

	if (StageReqs.entries > 0) {
		for (i = 0; i < StageReqs.alloc; i++) {
			if (isFree(i) == 0) {
				file = &StageReqs.data[i];
				_Trace(flag, srcFile, srcLine,
				    "[%d] file: 0x%x inode: %d.%d",
				    i, (int)file, file->id.ino,
				    file->id.gen);
			}
		}
	}
}

/*
 * Add file to request list. The stager's request list contains all
 * stage requests in progress. Since the request list is implemented
 * as an index file that is memory mapped to the process's address
 * space we must copy the file's information to the request list.
 */
static int
addRequest(
	FileInfo_t *file)
{
	int index = -1;
	extern StagerStateInfo_t *State;

	if (StageReqs.data == NULL) {
		(void) InitRequestList();
	}

	index = getFree();

	if (index >= 0) {
		PthreadMutexLock(&StageReqs.entries_mutex);
		StageReqs.entries++;
		PthreadCondSignal(&StageReqs.entries_cond);
		PthreadMutexUnlock(&StageReqs.entries_mutex);

		/*
		 * Maintain own index in the request list.  File sorting is
		 * accomplished with an array of pointers to the files so
		 * after sorting has completed this index is an efficient way
		 * to map a file's pointer to a request list index.
		 */
		file->sort = index;
		memcpy(&StageReqs.data[index], file, sizeof (FileInfo_t));

		if (State != NULL) {
			State->reqEntries = StageReqs.entries;
			State->reqAlloc = StageReqs.alloc;
		}
	}
	return (index);
}

/*
 * Delete file from request list.
 */
void
DeleteRequest(
	int id)
{
	extern StagerStateInfo_t *State;

	if (id >= 0) {
		(void) memset(&StageReqs.data[id], 0, sizeof (FileInfo_t));
		markFree(id);
		PthreadMutexLock(&StageReqs.entries_mutex);
		StageReqs.entries--;
		PthreadMutexUnlock(&StageReqs.entries_mutex);

		if (State != NULL) {
			State->reqEntries = StageReqs.entries;
		}
	}
}

/*
 * Get archive copy.
 */
int
GetArcopy(
	FileInfo_t *file,
	int start)
{
	int copy = -1;
	int i;
	int j;

	i = start; 		/* start search at this copy */

	j = 0;
	while (j < MAX_ARCHIVE) {
		if (i >= MAX_ARCHIVE) {
			i = 0;
		}
		if ((file->ar[i].flags & STAGE_COPY_ARCHIVED) ||
		    (file->ar[i].flags & STAGE_COPY_VERIFY)) {
			if ((file->ar[i].flags &
			    (STAGE_COPY_STALE | STAGE_COPY_DAMAGED)) == 0) {
				copy = i;
				break;
			}
		}
		j++;
		i++;
	}
	return (copy);
}

/*
 * Mark entry in stage request list as free.
 */
static void
markFree(
	int index)
{
	PthreadMutexLock(&StageReqs.free_list_mutex);
	StageReqs.sp++;
	StageReqs.free[StageReqs.sp] = index;
	PthreadMutexUnlock(&StageReqs.free_list_mutex);
}

/*
 * Get a free entry from stage request list.
 */
static int
getFree(void)
{
	int index = -1;

	PthreadMutexLock(&StageReqs.free_list_mutex);
	if (StageReqs.sp >= 0) {
		index = StageReqs.free[StageReqs.sp];
		StageReqs.sp--;
	}
	PthreadMutexUnlock(&StageReqs.free_list_mutex);

	return (index);
}

/*
 * Check if entry in stage request list is free.
 */
static int
isFree(
	int index)
{
	int i;
	int free = 0;

	for (i = 0; i < StageReqs.sp; i++) {
		if (StageReqs.free[i] == index) {
			free = 1;
			break;
		}
	}
	return (free);
}

/*
 *	Initialize request list.
 */
int
InitRequestList()
{
	int i;
	size_t size;
	boolean_t recovered = B_FALSE;
	size_t max_active = GetCfgMaxActive();

	if (StageReqs.data == NULL) {

		/*
		 * Create request list if starting after clean shutdown
		 * or a failover. If restarting after abnormal termination,
		 * try to recover old request list.
		 */
		if (StartMode == SM_cold || StartMode == SM_failover ||
		    recoverRequestList() < 0) {

			/*
			 * Make sure old request list doesn't exist.
			 */
			(void) unlink(SharedInfo->si_stageReqsFile);

			/*
			 *	Recover failed, allocate new list.
			 */
			StageReqs.alloc = max_active;
			size = StageReqs.alloc * sizeof (FileInfo_t);
			size += sizeof (StageReqFileVal_t);

			SamMalloc(StageReqs.data, size);
			(void) memset(StageReqs.data, 0, size);

			StageReqs.val = (StageReqFileVal_t *)(void *)
			    (((char *)StageReqs.data + size) -
			    sizeof (StageReqFileVal_t));
			StageReqs.val->magic = STAGER_REQ_FILE_MAGIC;
			StageReqs.val->version = STAGER_REQ_FILE_VERSION;
			StageReqs.val->alloc = StageReqs.alloc;
			StageReqs.val->size = size;
			StageReqs.val->create = time(NULL);
			Trace(TR_DEBUG,
			    "Malloc StageReqs.data: %0x size: %d val: %0x",
			    (int)StageReqs.data, (int)size, (int)StageReqs.val);
		} else {
			recovered = B_TRUE;
		}

		size = StageReqs.alloc * sizeof (int);
		SamMalloc(StageReqs.free, size);

		/*
		 * Initialize free list indices.  Note the backward loop
		 * index so we start allocating from top of event list.
		 */
		if (recovered == B_FALSE) {
			for (i = StageReqs.alloc-1; i >= 0; i--) {
				markFree(i);
			}
		} else {
			for (i = StageReqs.alloc-1; i >= 0; i--) {
				if ((StageReqs.data[i].id.ino == 0) ||
				    (valRequest(i) == B_FALSE)) {
					(void) memset(&StageReqs.data[i], 0,
					    sizeof (FileInfo_t));
					markFree(i);
				} else {
					FileInfo_t *fi = &StageReqs.data[i];

					StageReqs.entries++;

					Trace(TR_MISC,
					    "Pending request id: %d "
					    "inode: %d.%d flags: %04x",
					    i, fi->id.ino, fi->id.gen,
					    fi->flags);

					if (StageReqs.requeue == -1) {
						fi->next = -1;
					} else {
						fi->next = StageReqs.requeue;
					}
					StageReqs.requeue = i;
				}
			}
			Trace(TR_DEBUG, "# of pending requests: %d",
			    StageReqs.entries);
		}
	} else {
		/*
		 * We'll never hit this pass, whenever InitRequestList called,
		 * StageReqs.data = NULL. But leave it as it was.
		 */

		/*
		 * Increase size of request list if list hasn't already exceeded
		 * maximum number of active stages.
		 */

		if (StageReqs.alloc < max_active) {
			int old = StageReqs.alloc;
			StageReqFileVal_t *oval;

			StageReqs.alloc += STAGE_REQUESTS_CHUNKSIZE;
			if (StageReqs.alloc > max_active)
				StageReqs.alloc = max_active;

			size = StageReqs.alloc * sizeof (FileInfo_t);
			size += sizeof (StageReqFileVal_t);
			StageReqs.data = realloc(StageReqs.data, size);

			oval = (StageReqFileVal_t *)(void *)
			    ((StageReqs.data + StageReqs.val->size) -
			    sizeof (StageReqFileVal_t));
			StageReqs.val = (StageReqFileVal_t *)(void *)
			    ((StageReqs.data + size) -
			    sizeof (StageReqFileVal_t));
			StageReqs.val->size = size;
			ASSERT((int)StageReqs.val > (int)oval);
			memmove(StageReqs.val, oval,
			    sizeof (StageReqFileVal_t));
			size = (size_t)(StageReqs.val - oval);
			(void) memset(oval, 0, size);

			size = StageReqs.alloc * sizeof (int);
			StageReqs.free = realloc(StageReqs.free, size);

			/*
			 * Initialize new free list indices.
			 */
			for (i = StageReqs.alloc-1; i >= old; i--)
				markFree(i);
		}
	}

	if (recovered == B_FALSE) {
		createRequestMapFile(SharedInfo->si_stageReqsFile);
		StageReqs.data =
		    (FileInfo_t *)MapInFile(SharedInfo->si_stageReqsFile,
		    O_RDWR, NULL);
	}

	return (0);
}

/*
 * Create memory mapped file for stage request list.
 */
static void
createRequestMapFile(
	char *file_name)
{
	size_t size;
	size_t num_written;
	int fd;

	unlink(file_name);
	fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0664);
	ASSERT(fd != -1);

	size = StageReqs.alloc * sizeof (FileInfo_t);
	size += sizeof (StageReqFileVal_t);
	num_written = write(fd, StageReqs.data, size);
	ASSERT(num_written == size);

	(void) close(fd);

	SamFree(StageReqs.data);
	StageReqs.data = NULL;
}

/*
 * Check if any of the copies are multivolume.  If so,
 * create a file extension to save the multiple vsn sections.
 */
static boolean_t
createMultiVolume(
	sam_stage_request_t *req)
{
	int i;
	boolean_t multivol = B_FALSE;

	for (i = 0; i < MAX_ARCHIVE; i++) {
		if (req->arcopy[i].n_vsns > 1) {
			multivol = B_TRUE;
			break;
		}
	}

	/*
	 * Found at least one multivolume copy.  Save all vsn
	 * sections in an extent.
	 */
	if (multivol) {
		FileExtentInfo_t extent;

		/*
		 *	Might be a duplicate.  Check if extent already exists.
		 */
		if (getFileExtent(req->id, req->arcopy[0].ext_ord, req->fseq,
		    req, &extent) < 0) {
			SysError(HERE, "Failed to allocate multivolume extent");
			exit(EXIT_FAILURE);
		}
		Trace(TR_FILES, "Add file extent: inode: %d.%d fseq: %d "
		    "count: %d", req->id.ino, req->id.gen, req->fseq,
		    extent.fe_id.ino != 0 ? extent.fe_count : 0);
	}
	return (multivol);
}

/*
 * Get extension for specified file identifier.
 */
static int
getFileExtent(
	sam_id_t id,
	int ext_ord,
	equ_t fseq,
	sam_stage_request_t *req,
	FileExtentInfo_t *raddr)
{
	FileExtentInfo_t *extent = NULL;
	int i;
	int ret = -1;

	PthreadMutexLock(&stageExtents.mutex);
	traceFileExtents(TR_MISC);

	if (stageExtents.hdr == NULL) {
		if (req == NULL) {
			PthreadMutexUnlock(&stageExtents.mutex);
			return (-1);
		}

		createFileExtent();

		stageExtents.hdr = (FileExtentHdrInfo_t *)
		    MapInFile(SharedInfo->si_stageReqExtents, O_RDWR, NULL);
		if (stageExtents.hdr == NULL) {
			Trace(TR_ERR, "Cannot map in file %s",
			    SharedInfo->si_stageReqExtents);
			PthreadMutexUnlock(&stageExtents.mutex);
			return (-1);
		}
		stageExtents.data = (FileExtentInfo_t *)(void *)
		    (stageExtents.hdr + sizeof (FileExtentHdrInfo_t));
	} else {
		if (stageExtents.hdr->fh_entries > 0) {
			FileExtentInfo_t *entry;

			for (i = 0; i < stageExtents.hdr->fh_alloc; i++) {
				entry = &stageExtents.data[i];
				if (entry != NULL &&
				    entry->fe_id.ino == id.ino &&
				    entry->fe_id.gen == id.gen &&
				    entry->fe_fseq == fseq &&
				    entry->fe_extOrd == ext_ord) {
					extent = entry;
					break;
				}
			}
		}
	}
	if (req != NULL) {
		i = 0;
		while (extent == NULL) {
			size_t osz;
			size_t nsz;
			FileExtentHdrInfo_t *nhdr;

			/*
			 * Enter new extent.
			 */
			for (; i < stageExtents.hdr->fh_alloc; i++) {
				int j, k;

				if (stageExtents.data[i].fe_id.ino != 0) {
					continue;
				}

				extent = &stageExtents.data[i];
				extent->fe_id.ino = id.ino;
				extent->fe_id.gen = id.gen;
				extent->fe_fseq	= fseq;
				extent->fe_extOrd = ext_ord;

		/*  N.B. Bad indentation to meet cstyle requirements. */
				for (j = 0; j < MAX_ARCHIVE; j++) {
				for (k = 0; k < SAM_MAX_VSN_SECTIONS; k++) {
					sam_stage_copy_t *ar;

					ar = &extent->fe_ar[j];

					ar->section[k].position =
					    req->arcopy[j].section[k].position;
					ar->section[k].offset =
					    req->arcopy[j].section[k].offset;
					ar->section[k].length =
					    req->arcopy[j].section[k].length;
					strcpy(ar->section[k].vsn,
					    req->arcopy[j].section[k].vsn);

				if (*TraceFlags & (1 << TR_files) &&
				    ar->section[k].vsn[0] != '\0') {
					Trace(TR_FILES, "[%d, %d] pos: "
					    "%llx.%llx len: %lld vsn: '%s'",
					    j, k,
					    ar->section[k].position,
					    ar->section[k].offset,
					    ar->section[k].length,
					    ar->section[k].vsn);
				}
				}
				}
				stageExtents.hdr->fh_entries++;
				break;
			}
			if (extent != NULL) {
				break;	/* exit while (extent == NULL) */
			}

			/*
			 * Increase size of file extension.
			 */

			osz = nsz = sizeof (FileExtentHdrInfo_t);
			osz += sizeof (FileExtentInfo_t) *
			    stageExtents.hdr->fh_alloc;
			nsz += sizeof (FileExtentInfo_t) *
			    (stageExtents.hdr->fh_alloc +
			    STAGE_EXTENTS_CHUNKSIZE);

			SamMalloc(nhdr, nsz);
			(void) memset(nhdr, 0, nsz);
			(void) memcpy(nhdr, stageExtents.hdr, osz);

			(void) RemoveMapFile(SharedInfo->si_stageReqExtents,
			    stageExtents.hdr, osz);

			if (WriteMapFile(SharedInfo->si_stageReqExtents,
			    (void *)nhdr, nsz) != 0) {
				FatalSyscallError(EXIT_NORESTART, HERE,
				    "WriteMapFile",
				    SharedInfo->si_stageReqExtents);
			}

			SamFree(nhdr);
			stageExtents.hdr = (FileExtentHdrInfo_t *)
			    MapInFile(SharedInfo->si_stageReqExtents,
			    O_RDWR, NULL);
			if (stageExtents.hdr == NULL) {
				Trace(TR_ERR, "Cannot map in file %s",
				    SharedInfo->si_stageReqExtents);
				PthreadMutexUnlock(&stageExtents.mutex);
				return (-1);
			}

			stageExtents.data =
			    (FileExtentInfo_t *)(void *)(stageExtents.hdr +
			    sizeof (FileExtentHdrInfo_t));

			stageExtents.hdr->fh_alloc += STAGE_EXTENTS_CHUNKSIZE;
			Trace(TR_DEBUG, "Added %d extensions, current: %d",
			    STAGE_EXTENTS_CHUNKSIZE,
			    stageExtents.hdr->fh_alloc);
		}
		if (extent != NULL) {
			extent->fe_count++;
		}
	}
	if (extent != NULL) {
		ret = 0;
		if (raddr != NULL) {
			memcpy(raddr, extent, sizeof (FileExtentInfo_t));
		}
	}
	PthreadMutexUnlock(&stageExtents.mutex);
	return (ret);
}

/*
 * Create file extension.
 */
static void
createFileExtent(void)
{
	size_t sz;

	sz = sizeof (FileExtentHdrInfo_t);
	sz += sizeof (FileExtentInfo_t) * STAGE_EXTENTS_CHUNKSIZE;
	SamMalloc(stageExtents.hdr, sz);
	(void) memset(stageExtents.hdr, 0, sz);
	stageExtents.hdr->fh_magic   = FILE_EXTENT_MAGIC;
	stageExtents.hdr->fh_version = FILE_EXTENT_VERSION;
	stageExtents.hdr->fh_alloc  = STAGE_EXTENTS_CHUNKSIZE;
	stageExtents.hdr->fh_create = StageReqs.val->create;

	if (WriteMapFile(SharedInfo->si_stageReqExtents,
	    (void *)stageExtents.hdr, sz) != 0) {
		FatalSyscallError(EXIT_NORESTART, HERE, "WriteMapFile",
		    SharedInfo->si_stageReqExtents);
	}

	SamFree(stageExtents.hdr);
	stageExtents.hdr = NULL;
}

/*
 * Delete extension for specified file identifier.
 */
static void
deleteFileExtent(
	sam_id_t id,
	int ext_ord,
	equ_t fseq)
{
	int i;

	if (stageExtents.hdr == NULL || stageExtents.data == NULL) {
		Trace(TR_FILES, "No extentions initialized yet");
		return;
	}

	PthreadMutexLock(&stageExtents.mutex);
	traceFileExtents(TR_MISC);

	if (stageExtents.hdr->fh_entries > 0) {
		FileExtentInfo_t *entry;
		for (i = 0; i < stageExtents.hdr->fh_alloc; i++) {
			entry = &stageExtents.data[i];
			if (entry->fe_id.ino == id.ino &&
			    entry->fe_id.gen == id.gen &&
			    entry->fe_fseq == fseq &&
			    entry->fe_extOrd == ext_ord) {
				Trace(TR_FILES, "Delete file extent: "
				    "inode: %d.%d ext_ord: %d count: %d",
				    entry->fe_id.ino, entry->fe_id.gen,
				    entry->fe_extOrd, entry->fe_count);
				if (--entry->fe_count > 0) {
					break;
				}
				(void) memset(&stageExtents.data[i], 0,
				    sizeof (FileExtentInfo_t));
				stageExtents.hdr->fh_entries--;
				break;
			}
		}
	}
	if (stageExtents.hdr->fh_entries == 0) {
		size_t sz;
		/*
		 * No active extensions, remove map file.
		 */
		sz = sizeof (FileExtentHdrInfo_t);
		sz += sizeof (FileExtentInfo_t) * stageExtents.hdr->fh_alloc;
		RemoveMapFile(SharedInfo->si_stageReqExtents,
		    stageExtents.hdr, sz);
		stageExtents.hdr = NULL;
		stageExtents.data = NULL;
	}
	PthreadMutexUnlock(&stageExtents.mutex);
}

/*
 * Initialize stage done list.
 */
void
InitStageDoneList(void)
{
	int ret;
	size_t size;
	pthread_condattr_t  cattr;
	pthread_mutexattr_t mattr;

	ASSERT(stageDone == NULL);
	size = sizeof (StageDoneInfo_t);
	SamMalloc(stageDone, size);

	PthreadMutexattrInit(&mattr);
	PthreadMutexattrSetpshared(&mattr, PTHREAD_PROCESS_SHARED);
	PthreadMutexInit(&stageDone->sd_mutex, &mattr);
	PthreadMutexattrDestroy(&mattr);

	PthreadCondattrInit(&cattr);
	PthreadCondattrSetpshared(&cattr, PTHREAD_PROCESS_SHARED);
	PthreadCondInit(&stageDone->sd_cond, &cattr);
	PthreadCondattrDestroy(&cattr);

	stageDone->sd_first = stageDone->sd_last = -1;

	ret = WriteMapFile(SharedInfo->si_stageDoneFile,
	    (void *)stageDone, size);
	if (ret != 0) {
		FatalSyscallError(EXIT_NORESTART, HERE, "WriteMapFile",
		    SharedInfo->si_stageDoneFile);
	}
	SamFree(stageDone);
	stageDone = (StageDoneInfo_t *)MapInFile(SharedInfo->si_stageDoneFile,
	    O_RDWR, NULL);

}

static int
waitForStageDone(void)
{
	extern int ShutdownStager;
	int status;
	struct timespec timeout;
	FileInfo_t *file;
	int id = -1;

	timeout.tv_sec = time(NULL) + REQUEST_TIMEOUT_SECS;
	timeout.tv_nsec = 0;

	PthreadMutexLock(&stageDone->sd_mutex);
	while (stageDone->sd_first == -1 && ShutdownStager == 0) {
		status = pthread_cond_timedwait(&stageDone->sd_cond,
		    &stageDone->sd_mutex, &timeout);
		/*
		 * Wait timed out.  Calculate new time out value.
		 */
		if (status == ETIMEDOUT) {
			timeout.tv_sec = time(NULL) + REQUEST_TIMEOUT_SECS;
			timeout.tv_nsec = 0;
		}
	}

	if (ShutdownStager) {
		PthreadMutexUnlock(&stageDone->sd_mutex);
		return (-1);
	}

	id = stageDone->sd_first;
	file = GetFile(id);
	stageDone->sd_first = file->next;

	if (stageDone->sd_first == -1) {
		stageDone->sd_last = -1;
	}

	PthreadMutexUnlock(&stageDone->sd_mutex);

	return (id);
}

/*
 * Notify the file system of a stage error.  File has not been opened
 * so stage syscall response is used to send the error.
 */
void
StageError(
	FileInfo_t *file,
	int error)
{
	int rc;
	sam_fsstage_arg_t arg;

	memset(&arg.handle, 0, sizeof (sam_handle_t));

	arg.handle.id = file->id;
	arg.handle.fseq = file->fseq;

	arg.handle.stage_off = file->fs.stage_off;
	arg.handle.stage_len = file->len;
	arg.handle.flags.b.stage_wait = file->fs.wait;

	arg.ret_err = error;

	Trace(TR_FILES, "Filesys resp inode: %d.%d fseq: %d ret_err: %d",
	    file->id.ino, file->id.gen, file->fseq, arg.ret_err);

	rc = sam_syscall(SC_fsstage, &arg, sizeof (arg));
	if (rc == -1) {
		WarnSyscallError(HERE, "SC_fsstage", "");
	}
}

static FileInfo_t *
findNextRequest(
	FileInfo_t *file)
{
	int copy;
	int i;
	FileInfo_t *entry;
	FileInfo_t *next = NULL;

	copy = file->copy;

	if (StageReqs.entries > 0) {

		for (i = 0; i < StageReqs.alloc; i++) {
			if (isFree(i) == 0) {
				entry = &StageReqs.data[i];
				if (GET_FLAG(entry->flags, FI_EXTENDED) &&
				    file->id.ino == entry->id.ino &&
				    file->id.gen == entry->id.gen &&
				    file->fseq == entry->fseq) {

					if (file->ar[copy].ext_ord + 1 ==
					    entry->ar[copy].ext_ord) {
						next = entry;
						break;
					}
				}
			}
		}
	}
	return (next);
}

static void
traceFileExtents(
	int flag,
	char *srcFile,
	int srcLine)
{
	int i;

	if (stageExtents.hdr == NULL) {
		return;
	}

	_Trace(flag, srcFile, srcLine,
	    "File extents entries: %d alloc: %d data: 0x%x",
	    stageExtents.hdr->fh_entries, stageExtents.hdr->fh_alloc,
	    (int)stageExtents.data);

	if (stageExtents.hdr->fh_entries > 0) {
		FileExtentInfo_t *entry;
		for (i = 0; i < stageExtents.hdr->fh_alloc; i++) {
			entry = &stageExtents.data[i];
			if (entry->fe_id.ino != 0) {
				_Trace(flag, srcFile, srcLine,
				    "[%d] extent: 0x%x inode: %d.%d "
				    "ext_ord: %d", i, (int)entry,
				    entry->fe_id.ino, entry->fe_id.gen,
				    entry->fe_extOrd);
			}
		}
	}
}

/*
 * Recover request list.
 */
static int
recoverRequestList(void)
{
	size_t count;
	size_t size;
	struct stat buf;

	ASSERT(StageReqs.data == NULL);

	if (stat(SharedInfo->si_stageReqsFile, &buf) < 0) {
		Trace(TR_DEBUG, "stat(%s) failed, errno: %d",
		    SharedInfo->si_stageReqsFile, errno);
		return (-1);
	}
	StageReqs.data = (FileInfo_t *)MapInFile(
	    SharedInfo->si_stageReqsFile, O_RDWR, &size);
	if (StageReqs.data == NULL) {
		Trace(TR_ERR, "MapInFile(%s) failed",
		    SharedInfo->si_stageReqsFile);
		return (-1);
	}
	if (size <= sizeof (StageReqFileVal_t)) {
		goto out;
	}
	count = (size - sizeof (StageReqFileVal_t)) / sizeof (FileInfo_t);
	if (count == 0 ||
	    (count * sizeof (FileInfo_t) + sizeof (StageReqFileVal_t)) != size)
		goto out;

	StageReqs.val = (StageReqFileVal_t *)(void *)
	    (((char *)StageReqs.data + size) - sizeof (StageReqFileVal_t));

	if (StageReqs.val->magic != STAGER_REQ_FILE_MAGIC ||
	    StageReqs.val->version != STAGER_REQ_FILE_VERSION ||
	    StageReqs.val->alloc != count ||
	    StageReqs.val->size != size) {

		Trace(TR_DEBUG, "Header error in old StageReqs.data, "
		    "magic: %0x version: %0x alloc: %d size: %d",
		    StageReqs.val->magic, StageReqs.val->version,
		    StageReqs.val->alloc, (int)StageReqs.val->size);
		goto out;
	}
	StageReqs.alloc = count;
	Trace(TR_MISC, "Recovered request list: %s, alloc: %d",
	    SharedInfo->si_stageReqsFile, (int)StageReqs.alloc);
	return (0);

out:
	/*
	 * Remove old request list.
	 */
	Trace(TR_MISC, "Can't recover old request list");
	RemoveMapFile(SharedInfo->si_stageReqsFile, StageReqs.data, size);
	return (-1);
}

/*
 * Separate multivolume request from the requeue link.
 * If multivolume request can't be recovered due to having an
 * open dcache or no extents available, delete request and extent.
 */
void
SeparateMultiVolReq(void)
{
	FileInfo_t *fi;
	int id = StageReqs.requeue;
	int *mid = NULL;
	int i;
	int prev = -1;

	Trace(TR_DEBUG, "StageReqs.entries: %d", StageReqs.entries);

	/*
	 * Separate the multivolume request from the requeue list.
	 */
	SamMalloc(mid, sizeof (int) * StageReqs.entries);
	for (i = 0; i < StageReqs.entries; i++) {
		mid[i] = -1;
	}

	while (id >= 0) {
		int next;

		fi = GetFile(id);
		ASSERT(fi != NULL);

		next = fi->next;

		if (fi->ar[fi->copy].n_vsns > 1) {
			/*
			 * Multivolume request found.
			 */
			Trace(TR_FILES,
			    "Multivolume request file inode: %d.%d "
			    "se_ord: %d ext_ord: %d n_vsns: %d flags: 0x%x",
			    fi->id.ino, fi->id.gen, fi->se_ord,
			    fi->ar[fi->copy].ext_ord,
			    fi->ar[fi->copy].n_vsns, fi->flags);

			i = 0;
			while (mid[i] >= 0) {
				FileInfo_t *cfi;

				cfi = GetFile(mid[i]);
				if (fi->id.ino == cfi->id.ino &&
				    fi->fseq == cfi->fseq) {
					while (cfi->next >= 0) {
						cfi = GetFile(cfi->next);
					}
					cfi->next = id;
					break;
				} else {
					i++;
				}
			}
			if (mid[i] < 0) {
				mid[i] = fi->sort;
			}
			fi->next = -1;

			if (prev == -1) {
				StageReqs.requeue = next;
			} else {
				FileInfo_t *pfi = GetFile(prev);

				ASSERT(pfi != NULL);
				pfi->next = next;
			}
		} else {
			prev = id;
		}
		id = next;
	}

	/*
	 * Examine the multivolume request
	 */
	i = 0;
	if (mid[0] >= 0) {
		recoverFileExtent();
	}
	while (mid[i] >= 0) {
		FileInfo_t *mfi;
		int min_ext;
		boolean_t remove = B_FALSE;

		/*
		 * Request extents has recovered, prepare for requeue.
		 */
		id = mid[i];
		min_ext = INT_MAX;
		while (id >= 0) {
			fi = GetFile(id);
			ASSERT(fi != NULL);
			if (getFileExtent(fi->id, fi->ar[fi->copy].ext_ord,
			    fi->fseq, NULL, NULL) < 0) {
				/*
				 * No extent available for this request.
				 */
				remove = B_TRUE;
			}
			if (fi->ar[fi->copy].ext_ord < min_ext) {
				min_ext = fi->ar[fi->copy].ext_ord;
				mfi = fi;
			}
			Trace(TR_MISC, "Extent for request inode: %d.%d "
			    "%s found", fi->id.ino, fi->id.gen,
			    remove == B_TRUE ? "NOT":"");
			id = fi->next;
		}

		if (GET_FLAG(mfi->flags, FI_ORPHAN)) {
			if (CopyInstanceList == NULL) {
				/*
				 * This happens copy process has exited
				 * before sam-stagerd has restarted and config
				 * has changed.
				 */
				Trace(TR_MISC,
				    "Can't recover multivolume req "
				    "inode: %d.%d "
				    "copyproclist not recovered "
				    "file is orphaned",
				    mfi->id.ino, mfi->id.gen);
				remove = B_TRUE;
			} else {
				/*
				 * Copy process is working on this inode.
				 * Leave copy process to run if staging from
				 * the last vsn.  Otherwise, kill copy process
				 * and delete request.
				 */
				if (mfi->vsn_cnt + 1 ==
				    mfi->ar[mfi->copy].n_vsns) {
					Trace(TR_MISC, "Staging from last vsn "
					    "inode: %d.%d flags: %04x "
					    "vsn_cnt: %d n_vsns: %d",
					    mfi->id.ino, mfi->id.gen,
					    mfi->flags, mfi->vsn_cnt,
					    mfi->ar[mfi->copy].n_vsns);
					remove = B_FALSE;
				} else {
					Trace(TR_MISC, "Staging from the first "
					    "or mid vsn inode: %d.%d "
					    " flags: %04x vsn_cnt: %d "
					    "n_vsns: %d",
					    mfi->id.ino, mfi->id.gen,
					    mfi->flags, mfi->vsn_cnt,
					    mfi->ar[mfi->copy].n_vsns);
					Trace(TR_MISC, "Can't recover "
					    "inode: %d.%d kill copy proc "
					    "pid: %d",
					    mfi->id.ino, mfi->id.gen,
					    (int)mfi->context);
					KillCopyProc(mfi->context, NULL);
					remove = B_TRUE;
				}
			}
		} else if (GET_FLAG(mfi->flags, FI_DCACHE)) {
			Trace(TR_MISC,
			    "Can't recover multivolume request "
			    "inode: %d.%d has a dcache opened",
			    mfi->id.ino, mfi->id.gen);
			remove = B_TRUE;
		} else if (mfi->ar[mfi->copy].ext_ord != 0 ||
		    mfi->se_ord != 0) {
			Trace(TR_MISC, "Can't recover multivolume request "
			    "inode: %d.%d incomplete request, se_ord: %d "
			    "ext_ord: %d",
			    mfi->id.ino, mfi->id.gen,
			    mfi->se_ord, mfi->ar[mfi->copy].ext_ord);
			remove = B_TRUE;
		}

		if (remove == B_FALSE &&
		    IsFileSystemMounted(mfi->fseq) == B_TRUE) {
			/*
			 * Put request back to the requeue link.
			 */
			if (StageReqs.requeue == -1) {
				mfi->next = -1;
			} else {
				mfi->next = StageReqs.requeue;
			}
			StageReqs.requeue = mfi->sort;
		} else {
			/*
			 * Request not recoverable, delete.
			 */
			int nid;
			id = mid[i];

			while (id >= 0) {
				fi = GetFile(id);
				Trace(TR_MISC,
				    "Request removed, inode: %d.%d "
				    "ext_ord: %d se_ord: %d",
				    fi->id.ino, fi->id.gen,
				    fi->ar[fi->copy].ext_ord, fi->se_ord);
				nid = fi->next;
				deleteFileExtent(fi->id,
				    fi->ar[fi->copy].ext_ord, fi->fseq);
				DeleteRequest(id);
				id = nid;
			}
		}
		i++;
	}
	SamFree(mid);
	if (stageExtents.hdr != NULL) {
		checkFileExtent();
	}
}

/*
 * Set stage done to the orphan request.
 */
void
DoneOrphanReq(
	pid_t pid)
{
	int i;
	FileInfo_t *fi;

	for (i = 0; i < StageReqs.alloc; i++) {
		fi = GetFile(i);
		if (GET_FLAG(fi->flags, FI_ORPHAN) != 0 && fi->context == pid) {
			Trace(TR_MISC, "Done orphanreq inode: %d.%d "
			    "pid: %d",
			    fi->id.ino, fi->id.gen, (int)pid);
			SetStageDone(fi);
			break;
		}
	}
	if (i >= StageReqs.alloc) {
		Trace(TR_MISC, "Done orphanReq pid: %d has no pending requests",
		    (int)pid);
	}
}

/*
 * Recover the file extent for the multivolume request.
 */
static void
recoverFileExtent(void)
{
	struct stat buf;
	size_t sz;

	ASSERT(stageExtents.hdr == NULL);

	if (stat(SharedInfo->si_stageReqExtents, &buf) != 0) {
		Trace(TR_MISC, "No request extents available");
		return;
	}

	stageExtents.hdr = (FileExtentHdrInfo_t *)
	    MapInFile(SharedInfo->si_stageReqExtents, O_RDWR, NULL);
	if (stageExtents.hdr == NULL) {
		Trace(TR_ERR, "Cannot map in file %s",
		    SharedInfo->si_stageReqExtents);
		return;
	}

	if (stageExtents.hdr->fh_magic   != FILE_EXTENT_MAGIC ||
	    stageExtents.hdr->fh_version != FILE_EXTENT_VERSION ||
	    stageExtents.hdr->fh_create  != StageReqs.val->create) {

		Trace(TR_ERR, "Corrupted header %s",
		    SharedInfo->si_stageReqExtents);
		RemoveMapFile(SharedInfo->si_stageReqExtents,
		    stageExtents.hdr, buf.st_size);
		stageExtents.hdr = NULL;
		return;
	}

	sz = sizeof (FileExtentHdrInfo_t);
	sz += stageExtents.hdr->fh_alloc * sizeof (FileExtentInfo_t);
	if (sz != buf.st_size) {
		SetErrno = 0;		/* set for trace */
		Trace(TR_ERR, "Corrupted header size %s",
		    SharedInfo->si_stageReqExtents);
		RemoveMapFile(SharedInfo->si_stageReqExtents, stageExtents.hdr,
		    buf.st_size);
		stageExtents.hdr = NULL;
		return;
	}

	stageExtents.data = (FileExtentInfo_t *)(void *)(stageExtents.hdr +
	    sizeof (FileExtentHdrInfo_t));
	Trace(TR_MISC, "Stage request extents recovered");
}

/*
 * Check recovered file extent.
 */
static void
checkFileExtent(void)
{
	int i;

	PthreadMutexLock(&stageExtents.mutex);
	for (i = 0; i < stageExtents.hdr->fh_alloc; i++) {
		int id;
		FileInfo_t *fi;

		if (stageExtents.data[i].fe_id.ino == 0) {
			continue;
		}

		id = StageReqs.requeue;
		while (id >= 0) {
			fi = GetFile(id);
			if (fi->id.ino == stageExtents.data[i].fe_id.ino) {
				break;
			}
			id = fi->next;
		}

		if (id < 0) {
			Trace(TR_MISC, "No request exist for extent "
			    "inode: %d.%d remove",
			    stageExtents.data[i].fe_id.ino,
			    stageExtents.data[i].fe_id.gen);
			(void) memset(&stageExtents.data[i], 0,
			    sizeof (FileExtentInfo_t));
			stageExtents.hdr->fh_entries--;
			ASSERT(stageExtents.hdr->fh_entries >= 0);
		}
	}

	if (stageExtents.hdr->fh_entries == 0) {
		size_t sz;

		/*
		 * No active extensions, remove map file.
		 */
		sz = sizeof (FileExtentHdrInfo_t);
		sz += sizeof (FileExtentInfo_t) * stageExtents.hdr->fh_alloc;
		RemoveMapFile(SharedInfo->si_stageReqExtents,
		    stageExtents.hdr, sz);
		stageExtents.hdr = NULL;
		stageExtents.data = NULL;
	}
	PthreadMutexUnlock(&stageExtents.mutex);
}

/*
 * Validate a request on the recovered request list.
 */
boolean_t
valRequest(
	int id)
{
	FileInfo_t *fi = &StageReqs.data[id];
	int mpfd;
	int fd;
	struct sam_ioctl_idopen arg;
	char *mount_name;

	if ((fi->magic != STAGER_REQUEST_MAGIC) ||
	    (fi->copy < 0) || (fi->copy >= MAX_ARCHIVE) ||
	    (fi->sort != id)) {
		return (B_FALSE);
	}

	mount_name = GetMountPointName(fi->fseq);
	if (mount_name == NULL || ((mpfd = open(mount_name, O_RDONLY)) < 0)) {
		/*
		 * Mount point not found.
		 */
		return (B_FALSE);
	}
	memset(&arg, 0, sizeof (arg));
	arg.id = fi->id;
	if (GetCfgDirectio) {
		arg.flags |= IDO_direct_io;
	}
	if ((fd = ioctl(mpfd, F_IDOPEN, &arg)) < 0) {
		/*
		 * File not found.
		 */
		(void) close(mpfd);
		return (B_FALSE);
	}
	(void) close(mpfd);
	(void) close(fd);

	return (B_TRUE);
}
