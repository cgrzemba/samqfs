/*
 * fifos.c - read commands from the fifos and act on them.
 *
 * Solaris 2.x Sun Storage & Archiving Management File System
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

#pragma ident "$Revision: 1.52 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <thread.h>
#include <synch.h>
#include <unistd.h>
#include <fcntl.h>
#include <grp.h>
#include <syslog.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/param.h"
#include "aml/shm.h"
#include "sam/lib.h"
#include "sam/names.h"
#include "aml/archiver.h"
#include "aml/device.h"
#include "aml/fifo.h"
#include "aml/logging.h"
#include "aml/odlabels.h"
#include "aml/tplabels.h"
#include "aml/robots.h"
#include "sam/defaults.h"
#include "aml/proto.h"
#include "sam/syscall.h"
#include "sam/nl_samfs.h"
#include "aml/catalog.h"
#include "aml/catlib.h"

/* Local headers. */
#include "amld.h"


/* Private functions. */
static void start_thread_fs(sam_fs_fifo_t **, void *(*)(void *));
static void start_thread_cmd(sam_cmd_fifo_t **, void *(*)(void *));

/* manage_fs_fifo. Watch the file system fifo */

void *
manage_fs_fifo(
	void *param)
{
	fifo_log_t *log_cmd;
	fifo_fet_t *fifo_fet = NULL;
	sam_fs_fifo_t *command;
	sam_defaults_t *defaults;
	sam_resync_arg_t resync_data;
	pid_t	mypid;
	char	*ent_pnt = "manage_fs_fifo";
	char	*s_mess, *sc_mess;
	char	*MES_11016 = catgets(catfd, SET, 11016,
	    "Waiting for file system to resync.");
	int		exit_status, ioctl_fd, file_len;
	int		fifo_log_fd;
	int		err, count;
	int		syscall_err;

	mypid = getpid();
	s_mess =
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->dis_mes[DIS_MES_NORM];
	sc_mess =
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->dis_mes[DIS_MES_CRIT];

	command = (sam_fs_fifo_t *)malloc_wait(sizeof (sam_si_fs_fifo_t), 2, 0);

	while (master_shm.shmid == -1) {
		sleep(5);
	}
	count = 0;
	defaults = GetDefaults();

	/* create and/or truncate the ioctl log file */
	if ((ioctl_fd = open(FS_IOCTL_LOG, O_CREAT | O_TRUNC | O_RDWR, 0660)) >=
	    0) {
		void *mp;

		file_len = sizeof (ioctl_fet_t) + (1000 * sizeof (ioctl_log_t));
		if (ftruncate(ioctl_fd, (off_t)file_len) < 0) {
			if (DBG_LVL(SAM_DBG_DEBUG)) {
				sam_syslog(LOG_ERR,
				    "%s: ftruncate: ioctl log: %m.", ent_pnt);
			}
		} else {
			if ((mp = mmap(NULL, file_len, (PROT_READ | PROT_WRITE),
			    MAP_SHARED, ioctl_fd, 0)) == MAP_FAILED) {
				if (DBG_LVL(SAM_DBG_DEBUG)) {
					sam_syslog(LOG_ERR,
					    "%s: mmap: ioctl log: %m.",
					    ent_pnt);
				}
				ioctl_fet = NULL;
			} else {
				ioctl_fet = (ioctl_fet_t *)mp;
				ioctl_fet->out = ioctl_fet->first =
				    ioctl_fet->in = sizeof (ioctl_fet_t);
				ioctl_fet->limit = file_len;
				(void) mutex_init(&ioctl_fet->in_mutex,
				    USYNC_PROCESS, NULL);
			}
		}
		(void) close(ioctl_fd);
	} else {
		if (DBG_LVL(SAM_DBG_DEBUG)) {
			sam_syslog(LOG_WARNING,
			    "%s: open: ioctl log: %m.", ent_pnt);
		}
	}

	/* create and/or truncate the fifo log file */
	if ((fifo_log_fd = open(FS_FIFO_LOG, O_CREAT | O_TRUNC | O_RDWR,
	    0660)) >= 0) {
		void *mp;

		file_len = sizeof (fifo_fet_t) + (1000 * sizeof (fifo_log_t));
		if (ftruncate(fifo_log_fd, (off_t)file_len) < 0) {
			if (DBG_LVL(SAM_DBG_DEBUG)) {
				sam_syslog(LOG_ERR,
				    "%s: ftruncate: fifo log: %m.", ent_pnt);
			}
		} else {
			if ((mp = mmap(NULL, file_len, PROT_READ | PROT_WRITE,
			    MAP_SHARED, fifo_log_fd, 0)) == MAP_FAILED) {
				if (DBG_LVL(SAM_DBG_DEBUG)) {
					sam_syslog(LOG_ERR,
					    "%s: mmap: fifo log: %m.", ent_pnt);
				}
				fifo_fet = NULL;
			} else {
				fifo_fet = (fifo_fet_t *)mp;
				fifo_fet->out = fifo_fet->first = fifo_fet->in =
				    sizeof (fifo_fet_t);
				fifo_fet->limit = file_len;
			}
		}
		(void) close(fifo_log_fd);
	} else {
		if (DBG_LVL(SAM_DBG_DEBUG)) {
			sam_syslog(LOG_WARNING,
			    "%s: open: fifo log: %m.", ent_pnt);
		}
	}

	resync_data.seq = time(NULL);
	resync_data.sam_amld_pid = mypid;

	if (sam_syscall(SC_amld_resync, &resync_data, sizeof (resync_data)) <
	    0) {
		if (errno == ENOPKG) {
			/*
			 * If the system call failed with ENOPKG,
			 * load the module and try again.
			 */
			sam_syslog(LOG_INFO,
			    "resync failed with ENOPKG; loading modules");
			(void) pclose(
			    popen("/usr/sbin/modload /kernel/drv/samioc", "w"));
			(void) pclose(
			    popen("/usr/sbin/modload /kernel/fs/samfs", "w"));
			if (sam_syscall(SC_amld_resync, &resync_data,
			    sizeof (sam_resync_arg_t)) < 0) {
				sam_syslog(LOG_INFO,
				    "sam-amld: resync syscall failed.\n");
			} else {
				sam_syslog(LOG_INFO,
				    "modules loaded, resync successful");
			}
		} else {
			sam_syslog(LOG_INFO,
			    "sam-amld: resync syscall failed.\n");
		}
	}
	/* LINTED constant in conditional context */
	while (TRUE) {			/* main loop */
		command->magic = 0;
		if ((syscall_err = sam_syscall(SC_amld_call,
		    command, sizeof (sam_fs_fifo_t))) >= 0) {
			count++;
			if (DBG_LVL(SAM_DBG_LOGGING)) {
				if ((fifo_fet->in + sizeof (fifo_log_t)) >
				    fifo_fet->limit)
					fifo_fet->in = fifo_fet->first;

	/* LINTED pointer cast may result in improper alignment */
				log_cmd = (fifo_log_t *)
				    (fifo_fet->in + (char *)fifo_fet);
				(void) gettimeofday(&(log_cmd->time), NULL);
				memmove(&(log_cmd->fifo_cmd), command,
				    sizeof (sam_fs_fifo_t));
				fifo_fet->in += sizeof (fifo_log_t);
			}
			/* Check the magic */
			if (command->magic != FS_FIFO_MAGIC) {
				if (DBG_LVL(SAM_DBG_DEBUG)) {
					sam_syslog(LOG_DEBUG,
					    "%s: Bad magic(%#x).",
					    ent_pnt, command->magic);
				}
				continue;	/* doesn't look right */
			}
			if (command->sam_amld_pid != mypid) {
				memccpy(s_mess, MES_11016, '\0', DIS_MES_LEN);
				sam_syslog(LOG_INFO, MES_11016);
				continue;
			}
			/*
			 * Each command should verify that the parameters
			 * are correct.
			 */

			switch (command->cmd) {
			case FS_FIFO_LOAD:
				start_thread_fs(&command, load_for_fs);
				break;

			case FS_FIFO_UNLOAD:
				start_thread_fs(&command, unload_for_fs);
				break;

			case FS_FIFO_POSITION:
				start_thread_fs(
				    &command, position_rmedia_for_fs);
				break;

			case FS_FIFO_CANCEL:
				start_thread_fs(&command, cancel_for_fs);
				break;

			case FS_FIFO_RESYNC:
				sam_syslog(LOG_INFO, catgets(catfd, SET, 13101,
				    "Resync sam-amld succeeded."));
				*s_mess = '\0';
				break;

			default:
				break;
			}

		} else {		/* error on syscall */
			err = errno;
			if (err == ENOPKG) {
				if (DBG_LVL(SAM_DBG_DEBUG))
					memccpy(s_mess,
					    "syscall module not loaded", '\0',
					    DIS_MES_LEN);
				/*
				 * This sleep should be no more than half the
				 * timeout used in rm.c:sam_fs_fifo_t.
				 */
				sleep(4);
			} else {
				break;	/* break main loop */
			}
		}
	}	/* main while loop */

	/* The system call should never return any error but ENOPKG */
	sam_syslog(LOG_CRIT, "syscall: SC_amld_call returned %#x, %s.",
	    syscall_err, error_handler(err));

	sprintf(sc_mess, "manage_fs_fifo - fatal error %s", error_handler(err));
	exit_status = 1;
	thr_exit(&exit_status);
	/* return (param); */
}


/*
 * manage_cmd_fifo - watch the command fifo
 */
void *
manage_cmd_fifo(
	void *param)
{
	dev_ent_t *device;
	dev_ptr_tbl_t *dev_ptr_tbl;
	sam_cmd_fifo_t *command;
	char	*ent_pnt = "manage_cmd_fifo";
	char	*fifo_path;
	int	count, exit_status, fifo_fd, err;
	int	fifo_log_fd;
	equ_t	cmd_eq;
	exit_FIFO_id	exit_id;

	/*
	 * get the directory path for the fifo, can't use dirname(3g) here
	 * caus' its marked as mt-unsafe in the man pages.
	 */

	fifo_path = SAM_FIFO_PATH "/" CMD_FIFO_NAME;
	unlink(fifo_path);
	err = mkfifo(fifo_path, 0660);

	if (err && (errno != EEXIST)) {
		sam_syslog(LOG_CRIT, "%s: mkfifo:%s:%m.", ent_pnt, fifo_path);
		exit_status = 1;
		thr_exit(&exit_status);
	}
	while (master_shm.shmid == -1) {
		sleep(5);
	}
	command = (sam_cmd_fifo_t *)malloc_wait(sizeof (sam_cmd_fifo_t), 5, 0);

	fifo_log_fd = open("cmd_fifo_log",
	    O_CREAT | O_TRUNC | O_WRONLY | O_SYNC, 0660);
	dev_ptr_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->dev_table);

	while (ent_pnt != NULL) {
		if ((fifo_fd = open(fifo_path, O_RDWR)) < 0) {
			sam_syslog(LOG_CRIT, "%s: open:%s:%m.",
			    ent_pnt, fifo_path);
			exit_status = 1;
			thr_exit(&exit_status);
		}
		while ((count =
		    read(fifo_fd, command, sizeof (sam_cmd_fifo_t))) > 0) {
			if (DBG_LVL(SAM_DBG_LOGGING)) {
				(void) write(fifo_log_fd, command, count);
			}
			/* Command size, magic add cmd offset must match */
			if ((count != sizeof (sam_cmd_fifo_t)) ||
			    (command->magic != CMD_FIFO_MAGIC) ||
			    (((command->cmd & CMD_OFFSET_MASK) !=
			    CMD_OFFSET))) {
				continue;	/* dosen't look right */
			}
			if (command->eq < 0 ||
			    command->eq > dev_ptr_tbl->max_devices) {
				/* equipment not in range */
				continue;
			}
			cmd_eq = command->eq;
			exit_id = command->exit_id;

			/*
			 * Each command should verify that the parameters
			 * are correct.
			 */

			switch (command->cmd) {
			case CMD_FIFO_MOUNT:
				/*
				 * updating the preview table can
				 * take time, so put it on a thread.
				 */
				start_thread_cmd(&command, add_preview_cmd);
				break;

			case CMD_FIFO_CLEAN:
				if (dev_ptr_tbl->d_ent[command->eq] == 0) {
					/* nothing on the eq number */
					continue;
				}
				device = (dev_ent_t *)SHM_REF_ADDR(
				    dev_ptr_tbl->d_ent[command->eq]);

				/* Must be tape and on a robot */
				if (device->fseq > 0 &&
				    device->fseq <=
				    (uint_t)dev_ptr_tbl->max_devices &&
				    IS_TAPE(device)) {
					start_thread_cmd(&command, clean_drive);
				}
				break;

			case CMD_FIFO_MOUNT_S:
				if (dev_ptr_tbl->d_ent[command->eq] == 0) {
					/* nothing on the eq number */
					continue;
				}
				device = (dev_ent_t *)SHM_REF_ADDR(
				    dev_ptr_tbl->d_ent[command->eq]);

				if ((command->slot != ROBOT_NO_SLOT) &&
				    IS_ROBOT(device) &&
				    (device->status.b.ready &&
				    device->status.b.present)) {
					start_thread_cmd(&command, mount_slot);
				}
				break;

			case CMD_FIFO_UNLOAD:
				if (dev_ptr_tbl->d_ent[command->eq] == 0) {
					/* nothing on the eq number */
					continue;
				}
				device = (dev_ent_t *)SHM_REF_ADDR(
				    dev_ptr_tbl->d_ent[command->eq]);

				if (device->fseq) {
					/* if a robot or belongs to a robot */
					start_thread_cmd(
					    &command, robot_unload);
				} else if (!mutex_trylock(&device->mutex)) {
					/* try it here */
					if (device->status.b.ready) {
						device->status.b.unload = TRUE;
					}
					mutex_unlock(&device->mutex);
				} else {
					/* create a thread to do it */
					if (thr_create(NULL, SM_THR_STK,
					    unload_cmd, device, THR_DETACHED,
					    NULL))
					sam_syslog(LOG_ERR,
					    "Unable to create unload_cmd"
					    " thread: %m.");
				}
				break;

			case CMD_FIFO_LABEL:
				if (SlotsUsedUp) {
					char *msg1;

					msg1 = catgets(catfd, SET, 11014,
					    "License prevents labeling more"
					    " media.");
					write_client_exit_string(
					    &command->exit_id, EXIT_FAILED,
					    msg1);
					sam_syslog(LOG_ALERT, msg1);
					continue;
				}
				/*
				 * Common code for both tape and optical
				 * labeling
				 */
				if (dev_ptr_tbl->d_ent[command->eq] == 0) {
					/* nothing on the eq number */
					continue;
				}
				if (command->flags & LABEL_SLOT) {
					/* if for a robot */
					start_thread_cmd(&command, robot_label);
				} else {
					start_thread_cmd(
					    &command, scanner_label);
				}
				break;

			case CMD_FIFO_SET_STATE:
				if (dev_ptr_tbl->d_ent[command->eq] == 0) {
					/* nothing on the eq number */
					continue;
				}
				start_thread_cmd(&command, set_state);
				break;

			case CMD_FIFO_LOAD_U:
				if (dev_ptr_tbl->d_ent[command->eq] == 0) {
					/* nothing on the eq number */
					continue;
				}
				start_thread_cmd(
				    &command, fifo_cmd_load_unavail);
				break;

			case CMD_FIFO_AUDIT:
				if (dev_ptr_tbl->d_ent[command->eq] == 0) {
					/* nothing on the eq number */
					continue;
				}
				start_thread_cmd(
				    &command, fifo_cmd_start_audit);
				break;

			case CMD_FIFO_MOVE:
				if (dev_ptr_tbl->d_ent[command->eq] == 0) {
					/* nothing on the eq number */
					continue;
				}
				start_thread_cmd(&command, move_slots);
				break;

			case CMD_FIFO_REMOVE_V:
				/* FALLTHROUGH */

			case CMD_FIFO_REMOVE_S:
				/* FALLTHROUGH */

			case CMD_FIFO_REMOVE_E:
				{

				/* N.B. Bad indentation here to meet cstyle */
				/* requirements */

				struct CatalogHdr *ch;

				ch = CatalogGetHeader(command->eq);
				if (ch == NULL) {
					sam_syslog(LOG_WARNING,
					    "%s:CMD_FIFO_ADD_VSN"
					    " err:eq%d vsn:%s.",
					    ent_pnt, command->eq, command->vsn);
					break;
				}
				if (ch->ChType == CH_historian) {
					struct VolId vid;

					memset(&vid, 0, sizeof (vid));
					if (command->slot !=
					    (unsigned)ROBOT_NO_SLOT) {
						vid.ViSlot = command->slot;
						vid.ViEq = command->eq;
						vid.ViFlags = VI_cart;
					} else {
						memmove(vid.ViMtype,
						    sam_mediatoa(
						    command->media),
						    sizeof (vid.ViMtype));
						memmove(vid.ViVsn, command->vsn,
						    sizeof (vid.ViVsn));
						vid.ViFlags = VI_logical;
					}
					if (CatalogExport(&vid) == -1) {
						if (errno) {
							err = errno;
						}

						sam_syslog(LOG_WARNING,
						    "%s:CMD_FIFO_REMOVE_VSN"
						    " err:eq%d vsn:%s",
						    ent_pnt, command->eq,
						    command->vsn);
					}
					break;
				}
				if (dev_ptr_tbl->d_ent[command->eq] == 0) {
					/* nothing on the eq number */
					continue;
				}
				start_thread_cmd(
				    &command, fifo_cmd_export_media);
				}
				break;

			case CMD_FIFO_IMPORT:
				if (SlotsUsedUp) {
					sam_syslog(LOG_ALERT,
					    catgets(catfd, SET, 11017,
					    "License prevents importing more"
					    " media."));
					continue;
				}
				if (dev_ptr_tbl->d_ent[command->eq] == 0) {
					/* nothing on the eq number */
					continue;
				}
				start_thread_cmd(
				    &command, fifo_cmd_import_media);
				break;

			case CMD_FIFO_DELETE_P:
				start_thread_cmd(&command, delete_preview_cmd);
				break;

			case CMD_FIFO_ADD_VSN:
				{

				/* N.B. Bad indentation here to meet cstyle */
				/* requirements */

				struct CatalogHdr *ch;

				ch = CatalogGetHeader(command->eq);
				if (ch == NULL) {
					sam_syslog(LOG_WARNING,
					    "%s:CMD_FIFO_ADD_VSN"
					    " err:eq%d vsn:%s.",
					    ent_pnt, command->eq, command->vsn);
					break;
				}
				if (ch->ChType == CH_historian) {
					int op_err = 0;
					uint32_t status = 0;
					struct VolId vid;

					status |= (CES_inuse | CES_occupied);
					vid.ViFlags = VI_cart;
					vid.ViEq = command->eq;
					vid.ViSlot = (short)ROBOT_NO_SLOT;
					memmove(vid.ViMtype,
					    sam_mediatoa(command->media),
					    sizeof (vid.ViMtype));
					if (*command->vsn != '\0') {
						memmove(vid.ViVsn, command->vsn,
						    sizeof (vid.ViVsn));
						status |= CES_labeled;
					}
					if (command->flags & ADDCAT_BARCODE) {
						status |= CES_bar_code;
						op_err = CatalogSlotInit(
						    &vid, status, 0,
						    (char *)command->info, "");
					} else {
						op_err = CatalogSlotInit(
						    &vid, status, 0, "", "");
					}

					if (op_err) {
						err = op_err;

						sam_syslog(LOG_WARNING,
						    "%s:CMD_FIFO_ADD_VSN"
						    " err:eq%d vsn:%s.",
						    ent_pnt, command->eq,
						    command->vsn);
						break;
					}
					break;
				}
				if (dev_ptr_tbl->d_ent[command->eq] == 0) {
					/* nothing on the eq number */
					continue;
				}
				start_thread_cmd(&command, add_catalog_cmd);
				}
				break;

			case CMD_FIFO_TAPEALERT:
				if (dev_ptr_tbl->d_ent[command->eq] == 0) {
					continue; /* nothing on the eq number */
				}
				start_thread_cmd(&command, fifo_cmd_tapealert);
				break;

			case CMD_FIFO_SEF:
				if (dev_ptr_tbl->d_ent[command->eq] == 0) {
					continue; /* nothing on the eq number */
				}
				start_thread_cmd(&command, fifo_cmd_sef);
				break;

			default:
				break;
			}

			/*
			 * If historian, we have to send a response because
			 * there is no historian daemon. Can't use command
			 * because thread has freed it.
			 */
			if (dev_ptr_tbl->d_ent[cmd_eq] == 0) {
				continue;	/* nothing on the eq number */
			}
			device = (dev_ent_t *)
			    SHM_REF_ADDR(dev_ptr_tbl->d_ent[cmd_eq]);
			if (IS_HISTORIAN(device)) {
				write_client_exit_string(&exit_id, err,
				    (char *)NULL);
			}
		}

		if (count == 0) {	/* last writer closed fifo */
			(void) close(fifo_fd);	/* close it here */
			continue;	/* go back and reopen it */
		}
		sam_syslog(LOG_CRIT,
		    "%s:read:CMD FIFO:%s:%m.", ent_pnt, fifo_path);
		exit_status = 1;
		thr_exit(&exit_status);
	}
	return (param);
}


/*
 * start_thread_cmd.
 *
 * start a function on a thread, passing the command.
 * Called thread is responsible for freeing the malloced memory.
 *
 */
static void
start_thread_cmd(
	sam_cmd_fifo_t ** command,
	void *func(void *))
{
	if (thr_create(NULL, SM_THR_STK, func, *command, THR_DETACHED, NULL)) {
		Dl_info sym_info;

		memset(&sym_info, 0, sizeof (Dl_info));
		if (dladdr((void *)func, &sym_info) && sym_info.dli_sname !=
		    NULL) {
			sam_syslog(LOG_INFO, "Unable to create thread %s:%s.",
			    sym_info.dli_sname, StrFromErrno(errno, NULL, 0));
		} else {
			sam_syslog(LOG_INFO,
			    "Unable to create thread %s:%s.", "unknown",
			    StrFromErrno(errno, NULL, 0));
		}
	} else {
		*command = (sam_cmd_fifo_t *)
		    malloc_wait(sizeof (sam_cmd_fifo_t), 4, 0);
	}
}


/*
 * start_thread_fs.
 *
 * start a function on a thread, passing the command.
 * Called thread is responsible for freeing the malloced memory.
 *
 */
void
start_thread_fs(
	sam_fs_fifo_t ** command,
	void *func(void *))
{
	if (thr_create(NULL, DF_THR_STK, func, *command, THR_DETACHED, NULL)) {
		Dl_info sym_info;

		memset(&sym_info, 0, sizeof (Dl_info));
		if (dladdr((void *)func, &sym_info) && sym_info.dli_sname !=
		    NULL) {
			sam_syslog(LOG_INFO, "Unable to create thread %s:%s.",
			    sym_info.dli_sname, StrFromErrno(errno, NULL, 0));
		} else {
			sam_syslog(LOG_INFO,
			    "Unable to create thread %s:%s.", "unknown",
			    StrFromErrno(errno, NULL, 0));
		}
	} else {
		*command = (sam_fs_fifo_t *)
		    malloc_wait(sizeof (sam_si_fs_fifo_t), 2, 0);
	}
}
