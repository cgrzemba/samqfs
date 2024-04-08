/*
 * dir_io.c - direct io.
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
 * or https://illumos.org/license/CDDL.
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

#pragma ident "$Revision: 1.36 $"

/* Using __FILE__ makes duplicate strings */
static char    *_SrcFile = __FILE__;

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/file.h>
#include <thread.h>
#include <synch.h>
#include <syslog.h>
#include <assert.h>

#include "aml/shm.h"
#include "sam/defaults.h"
#include "aml/device.h"
#include "sam/fioctl.h"
#include "sam/syscall.h"
#include "aml/logging.h"
#include "sam/nl_samfs.h"
#include "driver/samst_def.h"
#include "aml/dev_log.h"
#include "aml/catlib.h"
#include "aml/catalog.h"
#include "aml/proto.h"
#include "aml/odlabels.h"
#include "aml/sef.h"
#include "sam/lib.h"

/* some globals */
extern shm_alloc_t master_shm, preview_shm;

/* function prototypes */


/*
 * tell the file system to do direct IO for this request.
 */

int
dodirio(sam_io_reader_t *control, sam_actmnt_t *actmnt_req)
{
	int    errflag = 0, newio = 0, oldio = 0;
	int    eq, wrote_tm;
	char    *d_mess;
	time_t    time_now, cancel_time, start_time;
	dev_ent_t    *un = control->un;
	timestruc_t    wait_time;
	sam_act_io_t   *active_io;
	sam_defaults_t *defaults;
	sam_fsmount_arg_t mount_data;
	sam_fsiocount_arg_t iocount_data;
	unsigned int    block_size;
	u_longlong_t    start_position;
	char    *write1_msg, *write2_msg;


	write1_msg = "%#x blocks transferred";
#if defined(DEBUG)
	if (((sam_resource_t *)control->media_info)->access == FWRITE)
		write2_msg = "%uM per/second average write transfer rate";
	else
		write2_msg = "%uM per/second average read transfer rate";
#endif

	defaults = GetDefaults();

	wait_time.tv_nsec = 0;

	iocount_data.handle = actmnt_req->handle;
	mount_data.handle = actmnt_req->handle;
	mount_data.resource.ptr = &actmnt_req->resource;
	mount_data.rdev = actmnt_req->un->st_rdev;
	d_mess = actmnt_req->un->dis_mes[DIS_MES_NORM];

	mount_data.ret_err = 0;
	mutex_lock(&un->io_mutex);
	switch (un->type & DT_CLASS_MASK) {
	case DT_TAPE:
		if (((sam_resource_t *)control->media_info)->access == FWRITE) {
			mutex_lock(&un->mutex);
			un->status.b.wr_lock = TRUE;
			mutex_unlock(&un->mutex);
			/*
			 * any append problem will make the rest of the tape
			 * unusable
			 */
			if ((mount_data.ret_err = tape_append(control->open_fd,
			    un, &actmnt_req->resource)) != 0)
				un->space = 0;

			if (un->capacity != 0 &&
			    actmnt_req->resource.required_size >= un->space) {
				mount_data.ret_err = ENOSPC;
				break;
			}
			actmnt_req->resource.archive.rm_info.bof_written = TRUE;
			actmnt_req->resource.archive.rm_info.file_written
			    = TRUE;
			actmnt_req->resource.archive.rm_info.size = 0;
			actmnt_req->resource.archive.rm_info.file_offset = 0;
			actmnt_req->resource.archive.rm_info.mau
			    = un->sector_size;
		} else {
			mount_data.ret_err = find_tape_file(control->open_fd,
			    un, &actmnt_req->resource);

			/*
			 * On read mau should be set by filesystem. In case
			 * it's not set or set to 1
			 * (dma files with old archive info), set it
			 * to the sector size because filesystem expect some
			 * value
			 */
			if (actmnt_req->resource.archive.rm_info.mau == 0 ||
			    actmnt_req->resource.archive.rm_info.mau == 1)

				actmnt_req->resource.archive.rm_info.mau
				    = un->sector_size;
		}
		block_size = un->sector_size;
		mount_data.space = un->space;
		break;

	case DT_OPTICAL:
		if ((errflag =
		    find_file(control->open_fd, un, &actmnt_req->resource)))
			mount_data.ret_err = errflag;
		else {
			uint64_t reserve_sectors = 12 *
			    SECTORS_FOR_SIZE(un, sizeof (ls_bof1_label_t));

			if (actmnt_req->resource.access == FWRITE) {
				offset_t extra_space = compute_size_from_sectors
				    (un, reserve_sectors);

				if (un->status.b.read_only)
					mount_data.ret_err = EACCES;
				/*
				 * leave enough room for ptoc and 10 blank
				 * sectors
				 */
				else if ((actmnt_req->resource.required_size >
				    (un->space - extra_space)) ||
				    (un->space < extra_space))
					mount_data.ret_err = ENOSPC;
				else if ((mount_data.ret_err =
				    create_bof(control->open_fd,
				    un, &actmnt_req->resource)) == 0) {
					samst_range_t ranges;

					ranges.low_bn = un->dt.od.next_file_fwa;
					/*
					 * Reserve 12 sectors for possible
					 * errors while labeling
					 */
					ranges.high_bn = un->dt.od.ptoc_fwa -
					    reserve_sectors;
					if (ranges.high_bn < ranges.low_bn)
						ranges.high_bn = ranges.low_bn
						    = SAMST_RANGE_NOWRITE;
					DevLog(DL_DETAIL(2055), ranges.low_bn,
					    ranges.high_bn);

					if (ioctl(control->open_fd,
					    SAMSTIOC_RANGE, &ranges))
						DevLog(DL_SYSERR(2056));
				}
			}
			/*
			 * if ((mount_data.space = un->sector_size *
			 * compute_size_from_sectors(un, ((un->dt.od.ptoc_fwa
			 * - un->dt.od.next_file_fwa) - reserve_sectors))) <
			 * 0)
			 */
			if ((mount_data.space = 1024 * compute_size_from_sectors
			    (un, ((un->dt.od.ptoc_fwa -
			    un->dt.od.next_file_fwa) - reserve_sectors))) < 0) {

				mount_data.ret_err = ENOSPC;
				mount_data.space = 0;
			}
			/*
			 * Set mau to the optical sector size for both READ
			 * and WRITE since opticals don't allow any other
			 * allocation units
			 */
			actmnt_req->resource.archive.rm_info.mau
			    = un->sector_size;
		}
		block_size = OD_BS_DEFAULT;
		break;
	}

	actmnt_req->resource.archive.rm_info.media = un->type;

	mutex_unlock(&un->io_mutex);
	mutex_lock(&un->mutex);
	INC_OPEN(un);		/* Increment for the file system */
	if (IS_TAPE(un))
		un->dt.tp.next_read = 0;

	mutex_unlock(&un->mutex);

	if ((active_io = (sam_act_io_t *)SHM_REF_ADDR(un->active_io)) == NULL) {
		DevLog(DL_ERR(1054));
		mount_data.ret_err = EIO;
	}
	DevLog(DL_DETAIL(3287), mount_data.space);
	if (DBG_LVL(SAM_DBG_LOGGING))
		logioctl(SC_fsmount, 's', &mount_data);

	/*
	 * Must setup the active io area before call the system.  The fs may
	 * issue the unload request before this code gets to the wait loop.
	 */
	mutex_lock(&active_io->mutex);
	if (active_io->wait_fs_unload || active_io->active) {
		DevLog(DL_DETAIL(1055));
	}
	active_io->fd_stage = control->open_fd;	/* Pass fd to device thread */
	active_io->handle = actmnt_req->handle;
	active_io->resource = actmnt_req->resource;
	active_io->wait_fs_unload = TRUE;
	active_io->active = TRUE;
	active_io->timeout = FALSE;
	mutex_unlock(&active_io->mutex);

	if (sam_syscall(SC_fsmount, &mount_data, sizeof (sam_fsmount_arg_t))
	    < 0) {
		errflag = errno;
		if (errflag != ECANCELED) {
			DevLog(DL_ERR(1056));
		} else {
			DevLog(DL_DETAIL(1057));
		}
		mount_data.ret_err = errflag;
	}
	if (mount_data.ret_err != 0) {
		errno = mount_data.ret_err;
		DevLog(DL_SYSERR(1058));
		if (actmnt_req->resource.access == FWRITE) {
			if (actmnt_req->resource.archive.rm_info.bof_written &&
			    IS_OPTICAL(un)) {
				mutex_lock(&un->io_mutex);
				(void) create_optic_eof(control->open_fd, un,
				    &actmnt_req->resource);
				mutex_unlock(&un->io_mutex);
			}
		}
		mutex_lock(&active_io->mutex);
		active_io->wait_fs_unload = FALSE;
		active_io->active = FALSE;
		active_io->timeout = FALSE;
		active_io->fd_stage = -1;
		mutex_unlock(&active_io->mutex);
		mutex_lock(&un->mutex);
		if (actmnt_req->resource.access == FWRITE)
			un->status.b.wr_lock = FALSE;
		DEC_UNIT(un);	/* decrement for the file sys */
		DEC_ACTIVE(un);
		mutex_unlock(&un->mutex);
		/* Update the Catalog capacity, ptocfwa or last position */
		UpdateCatalog(un, 0, CatalogMediaClosed);
		return (mount_data.ret_err);
	}
	mutex_lock(&active_io->mutex);
	cancel_time = time(&time_now) + defaults->timeout;
	start_position = 0;
	wrote_tm = 0;
	(void) memccpy(d_mess,
	    catgets(catfd, SET, 2404, "Ready for data transfer"),
	    '\0', DIS_MES_LEN);
	DevLog(DL_DETAIL(1059));

writeon:
	start_time = time_now;
	while (active_io->wait_fs_unload) {
		active_io->block_count = oldio;
		/*
		 * Wake up every 10 seconds to update display info. Need to
		 * add an extra second for code execution time due to where
		 * time_now gets set in relation to its use here.
		 */
		wait_time.tv_sec = time_now + 11;
		errflag = cond_timedwait(&active_io->cond, &active_io->mutex,
		    &wait_time);
		(void) time(&time_now);
		DevLog(DL_DETAIL(1168), errflag, un->state);
		if (errflag != 0 || un->state >= DEV_IDLE) {
			/* If we received the unload, get out of wait */
			if (!active_io->wait_fs_unload) {
				break;
			}
			/*
			 * If not doing timeout, or the real time did not
			 * expire, clear the errflag
			 */
			if (!defaults->timeout ||
			    (errflag == ETIME &&
			    wait_time.tv_sec < cancel_time)) {
				errflag = 0;
			}
			/*
			 * If we have already timed out the file, don't ask
			 * for any more io counts
			 */
			if (!active_io->timeout) {
				if (DBG_LVL(SAM_DBG_LOGGING))
					logioctl(SC_fsiocount, 's',
					    &iocount_data);

				newio = sam_syscall(SC_fsiocount,
				    &iocount_data,
				    sizeof (sam_fsiocount_arg_t));

#if defined(SIM_DIR_IO_TIMEOUT)
				/*
				 * Simulate Direct I/O timeout after 100
				 * i/o's.
				 */
				if (newio >= 100) {
					oldio = newio;
					errflag = ETIME;
					cancel_time = wait_time.tv_sec;
				}
#endif

				if (newio < 0) {
					int err = errno;

					if (err == ECANCELED)
						newio = oldio;
					else
						DevLog(DL_ERR(1060));
				} else {
					char *msg_buf;
#if defined(DEBUG)
					if (DBG_LVL(SAM_DBG_TIME)) {
						msg_buf = malloc_wait(
						    strlen(write2_msg) + 20,
						    2, 0);
						(void) sprintf(msg_buf,
						    write2_msg,
						    (block_size/1024 *
						    newio/1024) / (time_now -
						    start_time));
					} else
#endif
					{
						msg_buf = malloc_wait(
						    strlen(write1_msg) + 20,
						    2, 0);
						(void) sprintf(msg_buf,
						    write1_msg, newio);
					}
					if (newio)
						(void) memccpy(d_mess, msg_buf,
						    '\0', DIS_MES_LEN);
					free(msg_buf);
				}
			} else if (wait_time.tv_sec >= cancel_time) {
				(void) memccpy(d_mess, catgets(catfd, SET, 2903,
				    "Waiting for response to timeout"),
				    '\0', DIS_MES_LEN);
				DevLog(DL_DETAIL(1062));
			}
			if ((oldio >= newio && errflag == ETIME) ||
			    un->state >= DEV_IDLE) {
				int call_err;
				sam_fserror_arg_t err_data;

				err_data.ret_err = ETIME;
				err_data.handle = mount_data.handle;
				active_io->timeout = TRUE;
				DevLog(DL_ERR(1063), err_data.ret_err,
				    oldio, newio);

				/* Must clear lock so sam-amld can get it */
				(void) memccpy(d_mess, catgets(catfd, SET, 906,
				    "Direct I/O timed out"),
				    '\0', DIS_MES_LEN);
				mutex_unlock(&active_io->mutex);
				if (DBG_LVL(SAM_DBG_LOGGING))
					logioctl(SC_fscancel, 's', &err_data);

				if ((call_err = sam_syscall(SC_fscancel,
				    &err_data, sizeof (err_data))) < 0) {
					DevLog(DL_SYSERR(1064));
				}
				mutex_lock(&active_io->mutex);
				if (call_err) {
					cancel_time = time_now +
					    defaults->timeout;
					active_io->timeout = TRUE;
				}
				DevLog(DL_ERR(1065));
				un->active = 0;
			} else if (oldio < newio)
				cancel_time = time_now + defaults->timeout;
		}
		oldio = newio;
	}

	/*
	 * I/O has stopped.
	 */

	/*
	 * Free the active_io mutex because sam_initd is holding it if called
	 * from unload_for_fs.
	 */
	mutex_unlock(&active_io->mutex);
	mutex_lock(&un->mutex);
	DEC_UNIT(un);		/* decrement for the file sys */
	mutex_unlock(&un->mutex);
	mutex_lock(&active_io->mutex);

	if (actmnt_req->resource.access == FWRITE) {
		long long    nbytes;

		nbytes = (long long) block_size *active_io->final_io_count;

		/*
		 * if the size is larger than the real size, just use the
		 * real size
		 */
		if (IS_OPTICAL(un) &&
		    actmnt_req->resource.required_size &&
		    nbytes > actmnt_req->resource.required_size * 1024)
			nbytes = actmnt_req->resource.required_size * 1024;

		DevLog(DL_TIME(1066), nbytes, time(NULL) - start_time);

		mutex_lock(&un->io_mutex);
		if (IS_OPTICAL(un))
			errflag = create_optic_eof(control->open_fd, un,
			    &active_io->resource);
		else {
			int	io_diff;
			uint_t	old_eof_position = un->dt.tp.position;

			active_io->resource.archive.rm_info.filemark = 0;
			if (wrote_tm && !active_io->
			    resource.archive.rm_info.process_wtm) {
				active_io->
				    resource.archive.rm_info.filemark = 1;
			}
			wrote_tm = 0;
			errflag = 0;
			errflag = create_tape_eof(&(control->open_fd), un,
			    &active_io->resource);
			(void) sef_data_sample(control->open_fd, un,
			    SEF_SAMPLE);
			io_diff = (un->dt.tp.position & un->dt.tp.mask) -
			    (old_eof_position & un->dt.tp.mask);
			DevLog(DL_DEBUG(1067), io_diff,
			    active_io->final_io_count);
			if (abs(io_diff - active_io->final_io_count) > 1) {
				DevLog(DL_ERR(1068), old_eof_position,
				    un->dt.tp.position,
				    active_io->final_io_count);
				errflag = EIO;
			}
		}

		mutex_unlock(&un->io_mutex);

		/* Update the catalog */
		if (errflag == 0) {
			UpdateCatalog(un, 0, CatalogMediaClosed);
		} else {
			if (errflag < 0) {
				/* Response should be a valid errno. */
				errflag = EIO;
			}
			eq = (un->fseq ? un->fseq : un->eq);
			(void) CatalogSetFieldByLoc(eq, un->slot, un->i.ViPart,
			    CEF_Status, CES_bad_media, 0);
			DevLog(DL_ALL(10046), errflag);
		}

		/* if already sent by sam-amld */
		if (!active_io->timeout)
			notify_fs_unload(&active_io->handle,
			    start_position, errflag);

		if (active_io->resource.archive.rm_info.process_wtm) {
			/*
			 * Only writing a tape mark, continue writing.
			 */
			start_position = un->dt.tp.position;
			wrote_tm = 1;
			active_io->wait_fs_unload = 1;
			goto writeon;
		}
		mutex_lock(&un->mutex);
		un->status.b.wr_lock = FALSE;
		mutex_unlock(&un->mutex);
	} else {
		long long    nbytes;

		nbytes = (long long) block_size *active_io->final_io_count;
		DevLog(DL_TIME(1069), nbytes, time(NULL) - start_time, errflag);
		assert(errflag==0);
	}

	/* wait_fs_unload must be clear or we would not be here */
	active_io->active = active_io->timeout = active_io->fs_cancel = FALSE;
	mutex_unlock(&active_io->mutex);

	return (errflag);
}
