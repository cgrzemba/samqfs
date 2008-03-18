/*
 * stage.c - interfaces to stage api for migration toolkit
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

#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "aml/types.h"
#include "aml/shm.h"

#include "sam/types.h"
#include "aml/stager_defs.h"
#include "sam/fs/amld.h"
#include "sam/syscall.h"
#include "sam/fioctl.h"
#include "sam/lib.h"
#include "sam/sam_trace.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

extern shm_alloc_t master_shm;

/*
 * Declare external functions from stager daemon.
 */
extern void SetStageActive(FileInfo_t *file);
extern void SetStageDone(FileInfo_t *file);
extern void LogStageStart(FileInfo_t *file);
extern boolean_t DamageArcopy(FileInfo_t *file);

static char *libname = "sam_mig";

static void errorFile(MigFileInfo_t *migfile, int error);

/*
 * This function is called when the foreign data migration program
 * is ready to start staging the data for a stage request.  Open
 * the disk cache file.
 */
int
sam_mig_stage_file(
	tp_stage_t *stage_req)
{
	MigFileInfo_t *migfile;
	FileInfo_t *file;
	sam_fsstage_arg_t arg;
	int rc;

	migfile = (MigFileInfo_t *)stage_req;
	if (migfile == NULL) {
		return (-1);
	}
	file = migfile->file;
	if (file == NULL) {
		return (-1);
	}

	if (DBG_LVL(SAM_DBG_MIGKIT)) {
		sam_syslog(LOG_DEBUG, "%s [t@%d] Start staging inode: %d",
		    libname, pthread_self(), migfile->req.inode);
	}

	SetStageActive(file);
	LogStageStart(file);

	memset(&arg.handle, 0, sizeof (sam_handle_t));
	arg.handle.id = file->id;
	arg.handle.fseq = migfile->req.fseq;

	arg.handle.stage_off = file->fs.stage_off;
	arg.handle.stage_len = migfile->req.size;
	arg.handle.flags.b.stage_wait = file->fs.wait;
	arg.ret_err = 0;

	rc = 0;
	file->dcache = sam_syscall(SC_fsstage, &arg,
	    sizeof (sam_fsstage_arg_t));
	if (file->dcache < 0) {
		rc = -1;
		if (errno == ECANCELED) {
			sam_syslog(LOG_DEBUG,
			    "%s [t@%d] Stage request canceled by filesystem",
			    libname, pthread_self());
		} else {
			sam_syslog(LOG_INFO,
			    "%s [t@%d] System call (SC_fsstage) failed "
			    "errno: %d", libname, pthread_self(), errno);
		}
	}

	return (rc);
}

/*
 * Passes data from the foreign data migration program to the file
 * filesystem for the associated stage request.
 */
int
sam_mig_stage_write(
	tp_stage_t *stage_req,
	char *buffer,
	int size,
	offset_t offset)
{
	MigFileInfo_t *migfile;
	FileInfo_t *file;
	sam_ioctl_swrite_t stage_write;
	int nbytes_written;

	migfile = (MigFileInfo_t *)stage_req;
	file = migfile->file;

	stage_write.buf.ptr = buffer;
	stage_write.nbyte = size;
	stage_write.offset = offset;

	nbytes_written = ioctl(file->dcache, F_SWRITE, &stage_write);

	if (nbytes_written != size) {
		if (nbytes_written == 0) {
			nbytes_written = -1;
			SetErrno = ECANCELED;
		}
		if (errno == ECANCELED) {
			sam_syslog(LOG_DEBUG,
			    "%s [t@%d] Stage request canceled by filesystem",
			    libname, pthread_self());
		} else {
			sam_syslog(LOG_INFO,
			    "%s [t@%d] System call (F_SWRITE) failed errno: %d",
			    libname, pthread_self(), errno);
		}
	}
	return (nbytes_written);
}

/*
 * This function is called when the foreign data migration program
 * has finished the stage or detected an error after sam_mig_stage_file
 * has been called without error.
 */
int
sam_mig_stage_end(
	tp_stage_t *stage_req,
	int error)
{
	MigFileInfo_t *migfile;
	FileInfo_t *file;

	migfile = (MigFileInfo_t *)stage_req;
	file = migfile->file;

	if (DBG_LVL(SAM_DBG_MIGKIT)) {
		sam_syslog(LOG_DEBUG,
		    "%s [t@%d] End staging inode: %d error: %d",
		    libname, pthread_self(), migfile->req.inode, error);
	}

	if (file->dcache >= 0) {
		if (error != 0) {
			errorFile(migfile, error);
		}
		close(file->dcache);
	} else {
		errorFile(migfile, error);
	}
	file->migfile = 0;
	free(migfile);
	SetStageDone(file);

	return (0);
}

/*
 * This function is used to pass an error to the file system for
 * an associated stage request.
 */
int
sam_mig_stage_error(
	tp_stage_t *stage_req,
	int error)
{
	MigFileInfo_t *migfile;
	FileInfo_t *file;

	migfile = (MigFileInfo_t *)stage_req;
	file = migfile->file;

	if (DBG_LVL(SAM_DBG_MIGKIT)) {
		sam_syslog(LOG_DEBUG,
		    "%s [t@%d] Error staging inode: %d error: %d",
		    libname, pthread_self(), migfile->req.inode, error);
	}

	if (error != 0) {
		errorFile(migfile, error);
	}

	file->migfile = 0;
	free(migfile);
	SetStageDone(file);

	return (0);
}

static void
errorFile(
	MigFileInfo_t *migfile,
	int error)
{
	sam_fsstage_arg_t arg;
	FileInfo_t *file;
	int rc;

	file = migfile->file;

	memset(&arg.handle, 0, sizeof (sam_handle_t));

	arg.handle.id = file->id;
	arg.handle.fseq = migfile->req.fseq;

	arg.handle.stage_off = file->fs.stage_off;
	arg.handle.stage_len = migfile->req.size;
	arg.handle.flags.b.stage_wait = file->fs.wait;

	arg.ret_err = error;

	if (file->dcache >= 0) {
		rc = sam_syscall(SC_fsstage_err, &arg, sizeof (arg));
		if (rc < 0) {
			sam_syslog(LOG_INFO,
			    "%s [t@%d] System call (SC_fsstage_err) failed "
			    "errno: %d", libname, pthread_self(), errno);
		}
		close(file->dcache);

	} else {
		rc = sam_syscall(SC_fsstage, &arg, sizeof (arg));
		if (rc < 0) {
			sam_syslog(LOG_INFO,
			    "%s [t@%d] System call (SC_fsstage) failed "
			    "errno: %d", libname, pthread_self(), errno);
		}
	}

	if (error) {
		file->error = error;
		DamageArcopy(file);
	}
}
