/*
 * fs_interface.c common routines to interface back to the file_system
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

#pragma ident "$Revision: 1.25 $"


#include <thread.h>
#include <synch.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "aml/shm.h"
#include "aml/device.h"
#include "aml/fifo.h"
#include "sam/lib.h"
#include "aml/proto.h"
#include "sam/syscall.h"
#define	DEC_INIT
#include "aml/logging.h"
#undef DEC_INIT
#include "sam/nl_samfs.h"
#include "sam/lib.h"

/* some globals */

extern shm_alloc_t master_shm;


void
send_fs_error(sam_handle_t *handle, const int err)
{
	sam_fserror_arg_t gen_error;

	gen_error.handle = *handle;
	gen_error.ret_err = err;

	if (DBG_LVL(SAM_DBG_LOGGING))
		logioctl(SC_fserror, 's', &gen_error);

	if (sam_syscall(SC_fserror, &gen_error, sizeof (sam_fserror_arg_t)) < 0)
		sam_syslog(LOG_CRIT,
		    "File system call failed for %d (FSERROR %d) %s:%d.",
		    handle->fseq, err, __FILE__, __LINE__);
}

/*
 * notify_fs_mount - Start a mount thread for this request. If error notify
 * the file system. Mutex on un should be locked on entry. unit can be a null
 * pointer(in cancel, for example). The caller must insure that there is no
 * other activity on the unit.
 */

void
notify_fs_mount(
	sam_handle_t *handle,		/* fs handle */
	sam_resource_t *resource,	/* resource entry */
	dev_ent_t *un,			/* unit assigned */
	const int errflag)
{
	int	ret_err = errflag;

	if (un != NULL && errflag == 0) {
		if (IS_TAPE(un) || IS_OPTICAL(un)) {
			int		err, retry = 5;
			sam_actmnt_t	*actmnt_req;

			/* check times */
			if (resource->archive.rm_info.valid &&
			    resource->archive.creation_time > 0 &&
			    un->label_time > resource->archive.creation_time) {

				sam_syslog(LOG_WARNING,
				    catgets(catfd, SET, 1244,
				    "Attempted read of re-labeled media %s."),
				    un->vsn);
				ret_err = ESPIPE;
				goto error;
			}
			actmnt_req = (sam_actmnt_t *)
			    malloc_wait(sizeof (sam_actmnt_t), 2, 0);

			actmnt_req->un = un;
			actmnt_req->handle = *handle;
			actmnt_req->resource = *resource;
			actmnt_req->callback = CB_NOTIFY_FS_LOAD;
			INC_ACTIVE(un);
			un->status.b.wr_lock = (resource->access == FWRITE);
			/*
			 * Once the file system has been given the media,
			 * clear the mount time;
			 */
			un->mtime = 0;

			/* CONSTCOND */
			while (TRUE) {
				if (thr_create(NULL, DF_THR_STK, mount_thread,
				    (void *) actmnt_req,
				    (THR_DETACHED | THR_BOUND), NULL) == 0)
					break;

				/* try again as a detached thread */
				if ((err = thr_create(NULL, DF_THR_STK,
				    mount_thread,
				    (void *) actmnt_req, THR_DETACHED,
				    NULL)) == 0)
					break;

				if ((err == EAGAIN) || (err == ENOMEM) &&
				    (retry-- > 0)) {
					sam_syslog(LOG_INFO,
					    "Retry mount_thread:%m.");
					thr_yield();
					(void) sleep(2);
					continue;
				}
				sam_syslog(LOG_INFO,
				"Fatal: Unable to start mount_thread:%m.");

				DEC_ACTIVE(un);
				ret_err = EIO;
				goto error;
			}
			return;	/* If threads started, just return */
		} else if (IS_RSD(un)) {	/* Remote sam device */
			/* un->mutex is still held */
			send_sp_todo(TODO_ADD, un, handle, resource,
			    CB_NOTIFY_FS_LOAD);
			return;
		} else
			ret_err = ENXIO;
	}
error:
	{
		sam_fsmount_arg_t *mount_data;

		if ((resource->access == FWRITE) && un)
			un->status.b.wr_lock = FALSE;

		mount_data = (sam_fsmount_arg_t *)
		    malloc_wait(sizeof (sam_fsmount_arg_t), 2, 0);

		(void) memset(mount_data, 0, sizeof (sam_fsmount_arg_t));

		mount_data->resource.ptr = resource;
		mount_data->handle = *handle;
		mount_data->ret_err = ret_err;

		if (DBG_LVL(SAM_DBG_LOGGING))
			logioctl(SC_fsmount, 's', mount_data);

		/*
		 * send FSMOUNT call with error to the file system.
		 */

		if (sam_syscall(SC_fsmount, mount_data,
		    sizeof (sam_fsmount_arg_t)) < 0) {
			if (errno != ECANCELED)
				sam_syslog(LOG_INFO,
				    "File system call FSMOUNT failed:%m-%s:%d.",
				    __FILE__, __LINE__);
		}
		free(mount_data);
		return;
	}

}

/*
 * notify_fs_unload - notify the file system that an unload has completed
 */

void
notify_fs_unload(sam_handle_t *handle, u_longlong_t position, const int errflag)
{
	char	*ent_pnt = "notify_fs_unload";
	sam_fsunload_arg_t unload_data;

	(void) memset(&unload_data, 0, sizeof (sam_fsunload_arg_t));
	unload_data.handle = *handle;
	unload_data.position = position;
	unload_data.ret_err = errflag;

	if (DBG_LVL(SAM_DBG_LOGGING))
		logioctl(SC_fsunload, 's', &unload_data);

	if (sam_syscall(SC_fsunload, &unload_data,
	    sizeof (sam_fsunload_arg_t)) < 0)
		sam_syslog(LOG_INFO, "%s: System call failed:%m.", ent_pnt);
}

/*
 * notify_fs_invalid_cache - tell file system that we are about to unmount a
 * device and that it should flush/invalidate any buffer cache.
 */

void
notify_fs_invalid_cache(dev_ent_t *un)
{
	sam_fsinval_arg_t inval_cmd;

	inval_cmd.rdev = un->st_rdev;

	if (sam_syscall(SC_fsinval, &inval_cmd, sizeof (sam_fsinval_arg_t)) < 0)
		sam_syslog(LOG_INFO,
		    "file system call fsinval failed:%m- %s:%d.",
		    __FILE__, __LINE__);
}

/*
 * notify_fs_position - notify the file system that a position has completed
 */

void
notify_fs_position(sam_handle_t *handle, u_longlong_t position,
	const int errflag)
{
	sam_position_arg_t response;

	(void) memset(&response, 0, sizeof (sam_position_arg_t));
	response.handle = *handle;
	response.position = position;
	response.ret_err = errflag;

	if (DBG_LVL(SAM_DBG_LOGGING))
		logioctl(SC_position, 's', &response);

	if (sam_syscall(SC_position, &response,
	    sizeof (sam_position_arg_t)) < 0) {
		sam_syslog(LOG_INFO, catgets(catfd, SET, 2976,
		    "File system ioctl (SC_position) failed."));
	}
}

void
logioctl(int type, char system, void *data)
{
	ioctl_log_t    *log_data;

	if (!DBG_LVL(SAM_DBG_LOGGING))
		return;

	if (ioctl_fet == (ioctl_fet_t *)NULL) {
		int	ioctl_fd, file_len;

		if ((ioctl_fd = open(FS_IOCTL_LOG, O_RDWR, 0666)) >= 0) {
			file_len = sizeof (ioctl_fet_t) +
			    (1000 * sizeof (ioctl_log_t));
			if (ftruncate(ioctl_fd, (off_t)file_len) < 0) {
				sam_syslog(LOG_WARNING,
				    "Unable to ftruncate ioctl log - %m");
			} else {
				if ((ioctl_fet = (ioctl_fet_t *)
				    (void *)mmap((void *) NULL, file_len,
				    (PROT_READ | PROT_WRITE), MAP_SHARED,
				    ioctl_fd, (off_t)0)) ==
				    (ioctl_fet_t *)MAP_FAILED) {

					sam_syslog(LOG_WARNING,
					    "Unable to map ioctl log - %m");
					ioctl_fet = (ioctl_fet_t *)NULL;
				}
			}
			(void) close(ioctl_fd);
		} else
			sam_syslog(LOG_WARNING,
			    "Unable to open ioctl log - %m.");
	}
	if (ioctl_fet != (ioctl_fet_t *)NULL) {
		mutex_lock(&ioctl_fet->in_mutex);
		if (ioctl_fet->in + sizeof (ioctl_log_t) > ioctl_fet->limit)
			ioctl_fet->in = ioctl_fet->first;
		/* LINTED pointer cast may result in improper alignment */
		log_data = (ioctl_log_t *)((char *)ioctl_fet + ioctl_fet->in);

		(void) gettimeofday(&log_data->time, (void *) NULL);
		log_data->ioctl_type = type;
		log_data->ioctl_system = system;
		switch (system) {
#if defined(USE_IOCTL_INTERFACE)
		case 'd':
			/* FALLTHROUGH */
		case 'D':
			switch (type) {
			case C_FSMOUNT:
				log_data->ioctl_data.fsmount.handle =
				    ((sam_ioctl_fsmount_t *)data)->handle;

				log_data->ioctl_data.fsmount.mt_handle =
				    ((sam_ioctl_fsmount_t *)data)->mt_handle;

				log_data->ioctl_data.fsmount.ret_err =
				    ((sam_ioctl_fsmount_t *)data)->ret_err;

				log_data->ioctl_data.fsmount.space =
				    ((sam_ioctl_fsmount_t *)data)->space;

				log_data->ioctl_data.fsmount.rdev =
				    ((sam_ioctl_fsmount_t *)data)->rdev;

				log_data->ioctl_data.fsmount.resource =
				    *((sam_ioctl_fsmount_t *)data)->resource;
				break;

			case C_FSUNLOAD:
				log_data->ioctl_data.fsunload =
				    *(sam_ioctl_fsunload_t *)data;
				break;

			case C_FSERROR:
				log_data->ioctl_data.fserror =
				    *(sam_ioctl_error_t *)data;
				break;

			case C_FSSTAGE:
				log_data->ioctl_data.fsstage =
				    *(sam_ioctl_stage_t *)data;
				break;

			case C_FSINVAL:
				log_data->ioctl_data.fsinval =
				    *(sam_ioctl_fsinval_t *)data;
				break;

			case C_FSBEOF:
				log_data->ioctl_data.fsbeof =
				    *(sam_ioctl_fsbeof_t *)data;
				break;

			case C_FSCANCEL:
				log_data->ioctl_data.fserror =
				    *(sam_ioctl_error_t *)data;
				break;

			default:
				break;
			}

			break;
#endif				/* USE_IOCTL_INTERFACE */
		case 'f':
			switch (type) {
			case C_SWRITE:
				log_data->ioctl_data.swrite =
				    *(sam_ioctl_swrite_t *)data;
				break;

			case C_STSIZE:
				log_data->ioctl_data.stsize =
				    *(sam_ioctl_stsize_t *)data;
				break;

			default:
				break;
			}

			break;

		case 's':
			{
				switch (type) {
				case SC_fsmount:
					log_data->ioctl_data.fsmount.handle =
					    ((sam_fsmount_arg_t *)data)->handle;

					log_data->ioctl_data.fsmount.mt_handle =
					    ((sam_fsmount_arg_t *)data)->
					    mt_handle.ptr;

					log_data->ioctl_data.fsmount.ret_err =
					    ((sam_fsmount_arg_t *)data)->
					    ret_err;

					log_data->ioctl_data.fsmount.space =
					    ((sam_fsmount_arg_t *)data)->space;

					log_data->ioctl_data.fsmount.rdev =
					    ((sam_fsmount_arg_t *)data)->rdev;

					log_data->ioctl_data.fsmount.resource =
					    *((sam_fsmount_arg_t *)data)->
					    resource.ptr;

					(void) memcpy(log_data->
					    ioctl_data.fsmount.fifo_name,
					    ((sam_fsmount_arg_t *)data)->
					    fifo_name, 32);
					break;

				case SC_fsunload:
					log_data->ioctl_data.sc_fsunload
					    = *(sam_fsunload_arg_t *)data;
					break;

				case SC_fserror:
					log_data->ioctl_data.sc_fserror
					    = *(sam_fserror_arg_t *)data;
					break;

				case SC_fsstage_err:
				case SC_fsstage:
					log_data->ioctl_data.sc_fsstage
					    = *(sam_fsstage_arg_t *)data;
					break;

				case SC_fsinval:
					log_data->ioctl_data.sc_fsinval
					    = *(sam_fsinval_arg_t *)data;
					break;

				case SC_fscancel:
					log_data->ioctl_data.sc_fserror
					    = *(sam_fserror_arg_t *)data;
					break;

				case SC_fsiocount:
					log_data->ioctl_data.sc_fsiocount
					    = *(sam_fsiocount_arg_t *)data;
					break;

				case SC_position:
					log_data->ioctl_data.sc_position
					    = *(sam_position_arg_t *)data;
					break;

				default:
					break;
				}

			}
			break;

		default:
			break;
		}
		ioctl_fet->in += sizeof (ioctl_log_t);
		mutex_unlock(&ioctl_fet->in_mutex);
	}
}
