/*
 * tape.c - support for tapes
 *
 * Programming notes:
 *
 * 1) The "position" returned by the scsi READ_POSITION command is assumed to
 * increase in numberic value as you move down(towards EOT) the tape. If a
 * drive does not do this, then the tape_append function will need to be
 * changed to speical case this drive.
 *
 * 2) The metrum rsp2150 drive(as tested here) can not handle the work of
 * backspacing for the append operation.  This drive is not closed in the
 * standard maner.  Only a TM is writen at the end of a file.  No EOF, no two
 * TMs, no nothing.
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

#pragma ident "$Revision: 1.48 $"

static char *_SrcFile = __FILE__;

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <sys/mtio.h>

#include "aml/device.h"
#include "aml/dev_log.h"
#include "aml/tapes.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/tplabels.h"
#include "aml/historian.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"
#include "aml/table_search.h"
#include "aml/tar.h"
#include "aml/tapealert.h"

#include "sam/lint.h"
#include "sam/custmsg.h"

/* Function prototypes */
int skip_eom(int, dev_ent_t *);
int check_eot(dev_ent_t *un, int *, sam_extended_sense_t *);

/* globals */

extern shm_alloc_t master_shm, preview_shm;

/*
 * create_tape_eof - put down the tape marks and eof label In the case of
 * errors(bus reset, etc) the open_fd will be closed and reopened in the hope
 * that there will be no tape motion that will cause a tape mark to be
 * written.
 *
 * io_mutex is held on entry. un mutex will be aquired is needed.
 */

table_t mt_erreg_table[] = {
	(uint_t)KEY_NO_SENSE, "No sense",
	(uint_t)KEY_RECOVERABLE_ERROR, "Recovered Error",
	(uint_t)KEY_NOT_READY, "Drive Not Ready",
	(uint_t)KEY_MEDIUM_ERROR, "Drive Media Error",
	(uint_t)KEY_HARDWARE_ERROR, "Drive Hardware Error",
	(uint_t)KEY_ILLEGAL_REQUEST, "Illegal SCSI Request",
	(uint_t)KEY_UNIT_ATTENTION, "Drive Unit Attention",
	(uint_t)KEY_WRITE_PROTECT, "Drive Write Protected",
	(uint_t)KEY_BLANK_CHECK, "Media Blank Check",
	(uint_t)KEY_VENDOR_UNIQUE, "Vendor Unique",
	(uint_t)KEY_COPY_ABORTED, "Copy Aborted",
	(uint_t)KEY_ABORTED_COMMAND, "Drive Aborted Command",
	(uint_t)KEY_EQUAL, "Equal",
	(uint_t)KEY_VOLUME_OVERFLOW, "Volume Overflow",
	(uint_t)KEY_MISCOMPARE, "Data Miscompare",
	(uint_t)KEY_RESERVED, "Reserved",
	(uint_t)SUN_KEY_FATAL, "Driver SCSI Handshake Failure",
	(uint_t)SUN_KEY_TIMEOUT, "Driver Command Timeout",
	(uint_t)SUN_KEY_EOF, "Detected End of File",
	(uint_t)SUN_KEY_EOT, "Detected End of Tape",
	(uint_t)SUN_KEY_LENGTH, "Length Error",
	(uint_t)SUN_KEY_BOT, "Detected Beginning of Tape",
	(uint_t)SUN_KEY_WRONGMEDIA, "Wrong Media Type",
	(uint_t)0, ""
};

/*
 * create_tape_eof()
 *
 * Returns :	0 Success 2 cannot open file 1 positioning error <0 others
 */

int
create_tape_eof(
	int *open_fd,	/* open file descriptor */
	dev_ent_t *un,	/* device entry (mutex held) */
	sam_resource_t *resource)	/* resource record */
{
	int	err = 0, return_error = 0, eot, cleaning = 0,
	    writing_labels = FALSE;
	int	tmp_err;
	int	process_eox, process_wtm, wrote_tm;
	uint_t	tmp_position;
	uint64_t space;
	char	eoflb[80];
	char	*d_mess;
	char	*msg_buf, *msg1;
	struct mtget	mt_status;
	sam_extended_sense_t	*sense;

	process_eox = 0;
	process_wtm = 0;
	wrote_tm = 0;
	if (resource != NULL) {
		process_eox = resource->archive.rm_info.process_eox;
		process_wtm = resource->archive.rm_info.process_wtm;
		wrote_tm = resource->archive.rm_info.filemark;
	}
	DevLog(DL_DEBUG(3205), resource, process_eox, process_wtm, wrote_tm);
	if (process_wtm) {
		/* Writing file mark */
		DevLog(DL_DETAIL(3212));
	} else {
		/* Writing EOF/EOV label */
		if (get_media_type(un) == MEDIA_WORM && IS_TAPE(un)) {
			DevLog(DL_DETAIL(3269));
		} else {
			DevLog(DL_DETAIL(3220));
		}
	}
	if (IS_TAPE(un) && un->dt.tp.drive_index != SAM_TAPE_DVR_DEFAULT) {
		register int    indx = un->dt.tp.drive_index;
		if (!(IO_table[indx].initialized)) {
			mutex_lock(&IO_table[indx].mutex);
			if (!(IO_table[indx].initialized)){
				if (load_tape_io_lib(&tape_IO_entries[indx],
				    &(IO_table[indx].jmp_table))) {
					memccpy(un->dis_mes[DIS_MES_CRIT],
					    catgets(catfd, SET, 2162,
					    "Runtime interface not found."),
					    '\0', DIS_MES_LEN);
					DevLog(DL_ERR(3143));
					DownDevice(un, SAM_STATE_CHANGE);
					mutex_unlock(&IO_table[indx].mutex);
					return (-1);
				} else
					IO_table[indx].initialized = TRUE;
			}
			mutex_unlock(&IO_table[indx].mutex);
		}
		if (IO_table[indx].jmp_table.create_tape_eof)
			return (IO_table[indx].jmp_table.create_tape_eof(
			    open_fd, un, resource));
	}
	d_mess = un->dis_mes[DIS_MES_NORM];
	sense = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);

	/*
	 * We have been writing on the media, space will be updated before
	 * before leaving this routine.
	 */
	un->dt.tp.stage_pos = 0;
	memset(eoflb, ' ', 80);
	memcpy(eoflb, "EOF1", 4);
	memset(&mt_status, 0, sizeof (struct mtget));

	/* Capture EOT status */
	eot = (((tmp_err = ioctl(*open_fd, MTIOCGET, (char *)&mt_status))
	    >= 0) && (mt_status.mt_erreg == SUN_KEY_EOT));

	if (tmp_err < 0) {
		if (eot)
			DevLog(DL_DETAIL(3209));
		else {
			DevLog(DL_ERR(3204), errno);
			return_error = -1;
		}
		TAPEALERT_MTS(*open_fd, un, mt_status.mt_erreg);
	}
	if (mt_status.mt_erreg != 0 && mt_status.mt_erreg != SUN_KEY_EOT) {
		DevLog(DL_ERR(3001), mt_status.mt_erreg,
		    table_search((uint_t)mt_status.mt_erreg, mt_erreg_table));
		if (mt_status.mt_erreg == KEY_MEDIUM_ERROR) {
			set_bad_media(un);
		}
	}
	if (eot) {
		DevLog(DL_DETAIL(3002), mt_status.mt_erreg);
		memccpy(d_mess, catgets(catfd, SET, 1010, "EOT detected"),
		    '\0', DIS_MES_LEN);
	}
	tmp_position = 0;
	(void) read_position(un, *open_fd, &tmp_position);
	if (tmp_position == 3)
		writing_labels = TRUE;

	if (mt_status.mt_erreg == KEY_UNIT_ATTENTION || tmp_position == 0) {
		int	retry = 10;
		int	ret;

		memccpy(d_mess, catgets(catfd, SET, 1946,
		    "Possible bus reset"), '\0', DIS_MES_LEN);
		DevLog(DL_ALL(3038));
		mutex_lock(&un->mutex);
		(void) close(*open_fd);	/* take no chances */
		sleep(5);
		while ((*open_fd = open(un->name, O_RDWR)) < 0 && retry-- > 0) {
			memccpy(d_mess, catgets(catfd, SET, 1855,
			    "open failed - retrying"), '\0', DIS_MES_LEN);
			DevLog(DL_RETRY(3039), un->name, retry);
			sleep(10);
		}

		un->status.bits |= (DVST_POSITION | DVST_FORWARD);
		if (*open_fd < 0 || ((ret = skip_eom(*open_fd, un)) < 0)) {
			un->status.bits &= ~(DVST_POSITION | DVST_FORWARD);
			mutex_unlock(&un->mutex);
			DevLog(DL_ERR(3040), *open_fd, ret);
			SendCustMsg(HERE, 9340, *open_fd, ret);
			/* for open failures, down the drive */
			if (*open_fd < 0)
				DownDevice(un, SAM_STATE_CHANGE);
			else
				set_bad_media(un);
			return (2);
		}
		un->status.bits &= ~DVST_POSITION;
		mutex_unlock(&un->mutex);

		/* try position again */
		tmp_position = 0;
		if (read_position(un, *open_fd, &tmp_position) < 0) {
			DevLog(DL_ERR(3041));
			set_bad_media(un);
			return (1);
		}
		msg1 = catgets(catfd, SET, 1938,
		    "positioned to %#x after reset");
		msg_buf = malloc_wait(strlen(msg1) + 12, 2, 0);
		sprintf(msg_buf, msg1, tmp_position);
		memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
		free(msg_buf);
		DevLog(DL_DETAIL(3042), tmp_position);
	}
	if (tmp_position < 3) {
		DevLog(DL_ERR(3043), tmp_position);
		set_bad_media(un);
		memccpy(d_mess,
		    catgets(catfd, SET, 2675, "Unable to recover from error"),
		    '\0', DIS_MES_LEN);
		return (1);
	}
	if (tmp_position == un->dt.tp.position && !wrote_tm) {
		memccpy(d_mess, catgets(catfd, SET, 780,
		    "create_tape_eof - nothing written"), '\0', DIS_MES_LEN);
		DevLog(DL_DEBUG(3045));
		if (writing_labels) {
			/*
			 * This can't happen: un->dt.tp.position should never
			 * point into the label area.
			 */
			DevLog(DL_ERR(3047), tmp_position, tmp_position);
			set_bad_media(un);
			return (1);
		} else {
			return (0);
		}
	}
	if (!wrote_tm) {
		memccpy(d_mess,
		    catgets(catfd, SET, 784, "creating EOF-Tape mark"),
		    '\0', DIS_MES_LEN);
		DevLog(DL_DETAIL(3237), tmp_position);
		if ((err = scsi_cmd(*open_fd, un, SCMD_WRITE_FILE_MARK, 0, 1))
		    != 0) {
			int	eop, save_err = errno;

			cleaning = check_cleaning_error(un, *open_fd, sense);
			eot |= check_eot(un, &eop, sense);
			if (eot)
				DevLog(DL_DETAIL(3208), eop, cleaning);
			else {
				DevLog(DL_ERR(3202), save_err, cleaning);
				DevLogSense(un);
			}
			TAPEALERT_SKEY(*open_fd, un);
		} else {
			/* Flush the device buffer to tape */
			if ((err = scsi_cmd(*open_fd, un, SCMD_WRITE_FILE_MARK,
			    0, 0)) != 0) {
				int	 eop, save_err = errno;
				cleaning = check_cleaning_error(un, *open_fd,
				    sense);
				eot |= check_eot(un, &eop, sense);
				if (eot)
					DevLog(DL_DETAIL(3240), eop, cleaning);
				else {
					DevLog(DL_ERR(3241), save_err,
					    cleaning);
					DevLogSense(un);
				}
				TAPEALERT_SKEY(*open_fd, un);
			}
		}
	}
	if (!eot) {
		eot |= ((space = read_tape_space(un, *open_fd)) == 0);
		err |= (read_position(un, *open_fd, &tmp_position) != 0);
		DevLog(DL_DETAIL(3203), eot, err);
		if (!cleaning)
			return_error = err;
	}
	mutex_lock(&un->mutex);
	if (cleaning)
		un->status.bits |= DVST_CLEANING;
	un->space = (un->capacity > space) ? space : un->capacity;
	un->dt.tp.position = tmp_position;
	if (eot) {
		DevLog(DL_DETAIL(3230), eot);
		un->space = 0;
		un->status.bits |= DVST_STOR_FULL;
		memcpy(eoflb, "EOV1", 4);
	}
	mutex_unlock(&un->mutex);


	/*
	 * If only writing a tape mark (tapenonstop), return here.
	 * If error, log it and mark media as bad.
	 */
	if (process_wtm) {
		if (return_error) {
			/* Unable to write TM */
			DevLog(DL_ERR(3286), un->dt.tp.position);
			set_bad_media(un);
		}
		return (return_error);
	}

	if (process_eox) {
		if (get_media_type(un) == MEDIA_WORM) {
			/* Flush the device buffer to tape */
			if ((err = scsi_cmd(*open_fd, un, SCMD_WRITE_FILE_MARK,
			    0, 0)) != 0) {
				int	eop, save_err = errno;
				cleaning = check_cleaning_error(un, *open_fd,
				    sense);
				eot |= check_eot(un, &eop, sense);
				if (eot) {
					DevLog(DL_ALL(0), "eop=%d cleaning=%d",
					    eop, cleaning);
				} else {
					DevLog(DL_ALL(0),
					    "save_err=%d cleaning=%d",
					    save_err, cleaning);
					DevLogSense(un);
				}
				if (!cleaning) {
					return_error = err;
				}
				TAPEALERT_SKEY(*open_fd, un);
			}
		}
		memcpy(eoflb, "EOV1", 4);
	}
	/*
	 * if err or metrum or worm media then dont write eof and tapemarks.
	 * Quantum claims the read_position problem only happens with
	 * write_tape_marks is issued with more than 1 tape mark, break up
	 * the write_tape_mark into two calls...
	 */
	if (return_error == 0 &&
	    un->equ_type != DT_VIDEO_TAPE && get_media_type(un) != MEDIA_WORM) {
		uint_t	pos;

		err = read_position(un, *open_fd, &pos);
		DevLog(DL_DETAIL(3046), eoflb[2], pos);

		msg1 = catgets(catfd, SET, 2958, "writing EO%c at %#x");
		msg_buf = malloc_wait(strlen(msg1) + 12, 2, 0);
		sprintf(msg_buf, msg1, eoflb[2], un->dt.tp.position);
		memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
		free(msg_buf);
		memcpy(&eoflb[4], &(un->dt.tp.position),
		    sizeof (un->dt.tp.position));
		if ((tmp_err = write(*open_fd, eoflb, 80)) != 80) {
			DevLog(DL_DETAIL(3207), eoflb[2], errno, tmp_err);
			return_error = -1;
			memset(&mt_status, 0, sizeof (struct mtget));
			eot |= (((tmp_err = ioctl(*open_fd, MTIOCGET,
			    (char *)&mt_status)) >= 0) &&
			    (mt_status.mt_erreg == SUN_KEY_EOT));
			if (tmp_err < 0) {
				if (eot) {
					DevLog(DL_DETAIL(3206));
				} else {
					DevLog(DL_ERR(3204), errno);
				}
				TAPEALERT_MTS(*open_fd, un, mt_status.mt_erreg);
			}
			if (mt_status.mt_erreg != 0 &&
			    mt_status.mt_erreg != SUN_KEY_EOT) {
				DevLog(DL_ERR(3001), mt_status.mt_erreg,
				    table_search((uint_t)mt_status.mt_erreg,
				    mt_erreg_table));
				if (mt_status.mt_erreg == KEY_MEDIUM_ERROR) {
					set_bad_media(un);
				}
			}
			if (eot) {

				/* end of tape is not an error, write "EOV" */
				return_error = 0;
				memcpy(eoflb, "EOV1", 4);
				if ((tmp_err = write(*open_fd, eoflb, 80))
				    != 80) {
					/*
					 * Two attempts in a row to write
					 * EOF/EOV failed
					 */
					DevLog(DL_ERR(3228), errno, tmp_err);
					return_error = -1;
					memset(&mt_status, 0,
					    sizeof (struct mtget));
					tmp_err = ioctl(*open_fd, MTIOCGET,
					    (char *)&mt_status);
					if (tmp_err < 0) {
						DevLog(DL_ERR(3204), errno);
					} else {
						DevLog(DL_ERR(3001),
						    mt_status.mt_erreg,
						    table_search(
						    (uint_t)mt_status.mt_erreg,
						    mt_erreg_table));
					}
					TAPEALERT_MTS(*open_fd, un,
					    mt_status.mt_erreg);
				}
			}
		}
		if (return_error == 0 && get_media_type(un) != MEDIA_WORM) {
			(void) read_position(un, *open_fd, &tmp_position);
			memccpy(d_mess, catgets(catfd, SET, 2963,
			    "writing Tape mark-Tape mark"),
			    '\0', DIS_MES_LEN);
			DevLog(DL_DETAIL(3238), EOT_MARKS, tmp_position);
			if ((err = scsi_cmd(*open_fd, un, SCMD_WRITE_FILE_MARK,
			    0, EOT_MARKS)) != 0) {

				int	eop, save_err = errno;
				cleaning = check_cleaning_error(un, *open_fd,
				    sense);
				eot |= check_eot(un, &eop, sense);
				if (eot)
					DevLog(DL_DETAIL(3211), eop, cleaning);
				else {
					DevLog(DL_ERR(3210), save_err,
					    cleaning);
					DevLogSense(un);
					if (!cleaning)
						return_error = err;
				}
				TAPEALERT_SKEY(*open_fd, un);
			} else {
				/* Flush the device buffer to tape */
				if ((err = scsi_cmd(*open_fd, un,
				    SCMD_WRITE_FILE_MARK, 0, 0)) != 0) {
					int	eop, save_err = errno;
					cleaning = check_cleaning_error(un,
					    *open_fd, sense);
					eot |= check_eot(un, &eop, sense);
					if (eot)
						DevLog(DL_DETAIL(3243), eop,
						    cleaning);
					else {
						DevLog(DL_ERR(3242), save_err,
						    cleaning);
						DevLogSense(un);
						if (!cleaning)
							return_error = err;
					}
					TAPEALERT_SKEY(*open_fd, un);
				}
			}
		}
		memset(&mt_status, 0, sizeof (struct mtget));
		eot |= (((tmp_err = ioctl(*open_fd, MTIOCGET,
		    (char *)&mt_status)) >= 0) &&
		    (mt_status.mt_erreg == SUN_KEY_EOT));
		if (tmp_err < 0) {
			if (eot) {
				DevLog(DL_DETAIL(3206));
			} else {
				DevLog(DL_ERR(3204), errno);
			}
		}
		if (eot) {
			/*
			 * We wrote past Logical EOT, the st driver will not
			 * let us read there. We must reposition the tape and
			 * let st know about it
			 */
			memccpy(d_mess, catgets(catfd, SET, 1002,
			    "end of tape detected"), '\0', DIS_MES_LEN);
			DevLog(DL_DEBUG(3048));
			mutex_lock(&un->mutex);
			un->space = 0;
			un->status.bits |= DVST_STOR_FULL;
			mutex_unlock(&un->mutex);
			/*
			 * ignore errors, next access to device
			 * will deal with any
			 */
			(void) rewind_tape(un, *open_fd);
		}
	}
	if (return_error) {
		/* Any kind of error here - set bad_media */
		DevLog(DL_ERR(3049), un->dt.tp.position);
		set_bad_media(un);
	}
	return (return_error);
}

/*
 * tape_append
 *
 * position tape to the correct place for append operation. io_mutex held on
 * entry.  If called with resource == NULL, then we need to skip to the eod
 * of the tape and update the eod/space fields (for auditslot -e).
 *
 * End of tape is Record (R), TM, EOF1 or EOV1, TM, TM, EOD
 */

int
tape_append(
	int open_fd,
	dev_ent_t *un,	/* with mutex i/o mutex held */
	sam_resource_t *resource)
{
	int	err = 0, status, resid, cleaning;
	uint_t	tmp_position;
	uint64_t space;
	char	eoflb[160];
	char	*d_mess;
	char	*msg_buf, *msg1;
	time_t	start;
	struct mtget	mt_status;
	int	tmp_err;
	int	lab_len;

	if (IS_TAPE(un) && un->dt.tp.drive_index != SAM_TAPE_DVR_DEFAULT) {
		register int    indx = un->dt.tp.drive_index;
		if (!(IO_table[indx].initialized)) {
			mutex_lock(&IO_table[indx].mutex);
			if (!(IO_table[indx].initialized)) {
				if (load_tape_io_lib(&tape_IO_entries[indx],
				    &(IO_table[indx].jmp_table))) {
					memccpy(un->dis_mes[DIS_MES_CRIT],
					    catgets(catfd, SET, 2162,
					    "Runtime interface not found."),
					    '\0', DIS_MES_LEN);
					DevLog(DL_ERR(3144));
					DownDevice(un, SAM_STATE_CHANGE);
					mutex_unlock(&IO_table[indx].mutex);
					return (-1);
				} else
					IO_table[indx].initialized = TRUE;
			}
			mutex_unlock(&IO_table[indx].mutex);
		}
		if (IO_table[indx].jmp_table.tape_append)
			return (IO_table[indx].jmp_table.tape_append(
			    open_fd, un, resource));
	}
	d_mess = un->dis_mes[DIS_MES_NORM];

	if (un->equ_type == DT_VIDEO_TAPE || get_media_type(un) == MEDIA_WORM) {
		mutex_lock(&un->mutex);
		un->status.bits |= (DVST_POSITION | DVST_FORWARD);
		mutex_unlock(&un->mutex);
		memccpy(d_mess, catgets(catfd, SET, 2351, "skipping to EOD"),
		    '\0', DIS_MES_LEN);
		(void) time(&start);
		if (scsi_cmd(open_fd, un, SCMD_SPACE, 1800, 0, 0x3, &resid)
		    != 0) {

			DevLog(DL_TIME(3050), time(NULL) - start);
			cleaning = check_cleaning_error(un, open_fd, NULL);
			mutex_lock(&un->mutex);
			if (cleaning)
				un->status.b.cleaning = cleaning;
			un->status.bits &= ~DVST_POSITION;
			mutex_unlock(&un->mutex);
			memccpy(d_mess, catgets(catfd, SET, 2344, "skip to EOD"
			    " failed: attempting recovery-Rewind"),
			    '\0', DIS_MES_LEN);
			(void) time(&start);
			(void) scsi_cmd(open_fd, un, SCMD_REZERO_UNIT, 3600);
			DevLog(DL_TIME(3051), time(NULL) - start);
			mutex_lock(&un->mutex);
			un->status.bits |= DVST_FORWARD;
			mutex_unlock(&un->mutex);
			memccpy(d_mess, catgets(catfd, SET, 2345,
			    "skip to EOD failed: attempting recovery-skip"
			    " EOD"), '\0', DIS_MES_LEN);
			(void) time(&start);
			if (scsi_cmd(open_fd, un, SCMD_SPACE, 1800, 0, 0x3,
			    &resid) != 0) {

				DevLog(DL_TIME(3052), time(NULL) - start);
				memccpy(d_mess, catgets(catfd, SET, 2346,
				    "skip to EOD failed: recovery failed"),
				    '\0', DIS_MES_LEN);
				DevLog(DL_ERR(3053));
				if (resource)
					set_bad_media(un);
				else {
					mutex_lock(&un->mutex);
					un->space = 0;
					un->status.bits |= DVST_STOR_FULL;
					mutex_unlock(&un->mutex);
				}
				TAPEALERT_SKEY(open_fd, un);
				return (1);
			}
			DevLog(DL_TIME(3054), time(NULL) - start);
		} else {
			DevLog(DL_TIME(3055), time(NULL) - start);
		}

		/*
		 * Determine if the tape was terminated by a tapemark.
		 */
		err = skip_block_backward(un, open_fd, 1, &status);
		if (!(err == 0 && status == SUN_KEY_EOF)) {
			/*
			 * Tape does not have a filemark at EOD. Return an
			 * error.
			 */
			(void) read_position(un, open_fd, &tmp_position);
			DevLog(DL_ERR(3160), tmp_position);
			set_bad_media(un);
			return (1);
		}
		/*
		 * Successfully back spaced 1 filemark. The tape was
		 * terminated properly. Reposition the tape at eod for
		 * append.
		 */
		if (scsi_cmd(open_fd, un, SCMD_SPACE, 1800, 0, 0x3, &resid)
		    != 0) {
			DevLog(DL_ERR(3162));
			set_bad_media(un);
			return (1);
		}
		err = read_position(un, open_fd, &tmp_position);
		msg1 = catgets(catfd, SET, 1948, "Position at EOD %#x");
		msg_buf = malloc_wait(strlen(msg1) + 12, 2, 0);
		sprintf(msg_buf, msg1, tmp_position);
		memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
		free(msg_buf);
		DevLog(DL_DEBUG(3056), tmp_position);
		mutex_lock(&un->mutex);
		un->dt.tp.position = tmp_position;
		un->status.bits &= ~DVST_POSITION;
		mutex_unlock(&un->mutex);
		space = read_tape_space(un, open_fd);
		cleaning = check_cleaning_error(un, open_fd, NULL);
		mutex_lock(&un->mutex);
		if (cleaning)
			un->status.b.cleaning = cleaning;
		un->space = (un->capacity > space) ? space : un->capacity;
		if (un->space == 0) {
			un->status.bits |= DVST_STOR_FULL;
			err = 1;
			memccpy(d_mess, catgets(catfd, SET, 1792,
			    "No space left on media"), '\0', DIS_MES_LEN);
			DevLog(DL_ERR(3057));
		} else {
			if (resource)
				resource->archive.rm_info.position =
				    un->dt.tp.position;
		}
		mutex_unlock(&un->mutex);
		return (err);
	}			/* end of metrum code */
	/* Use position from device entry if there */
	if (un->dt.tp.position > 0) {
		msg1 = catgets(catfd, SET, 1941, "positioning to %#x");
		msg_buf = malloc_wait(strlen(msg1) + 12, 2, 0);
		sprintf(msg_buf, msg1, un->dt.tp.position);
		memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
		free(msg_buf);
		err = position_tape(un, open_fd, un->dt.tp.position);
		if (err == 0) {
			memccpy(d_mess, catgets(catfd, SET, 675,
			    "checking for EOF label"), '\0', DIS_MES_LEN);

			/* check to make sure its an EOF or EOV with space  */
			space = read_tape_space(un, open_fd);
			if (((lab_len = read(open_fd, eoflb, 160)) == 80) &&
			    ((memcmp(eoflb, "EOF1", 4) == 0) ||
			    (space > 20 && (memcmp(eoflb, "EOV1", 4) == 0)))) {
				/*
				 * Reposition the tape(bksp record seems to
				 * be the fastest for all tape drives).
				 */
				(void) scsi_cmd(open_fd, un, SCMD_SPACE,
				    2 * 3600, -1, 0, NULL);
				err = read_position(un, open_fd, &tmp_position);
				if (err || (tmp_position & un->dt.tp.mask) !=
				    (un->dt.tp.position & un->dt.tp.mask)) {
					msg1 = catgets(catfd, SET, 1933,
					    "Position error:wanted %#x,"
					    " got %#x");
					msg_buf = malloc_wait(strlen(msg1) + 24,
					    2, 0);
					sprintf(msg_buf, msg1,
					    un->dt.tp.position,
					    tmp_position);
					memccpy(d_mess, msg_buf,
					    '\0', DIS_MES_LEN);
					free(msg_buf);
					DevLog(DL_DETAIL(3058),
					    un->dt.tp.position, tmp_position);
					goto hard_way;
				}
				if (resource) {
					if (IS_IBM_COMPATIBLE(un->equ_type))
						assert_compression(open_fd, un);

					resource->archive.rm_info.position =
					    un->dt.tp.position;
				}
				DevLog(DL_DETAIL(3059), un->dt.tp.position);
				msg1 = catgets(catfd, SET, 1234,
				    "found EOF1 at %#x");
				msg_buf = malloc_wait(strlen(msg1) + 12, 2, 0);
				sprintf(msg_buf, msg1, un->dt.tp.position);
				memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
				free(msg_buf);
				space = read_tape_space(un, open_fd);
				mutex_lock(&un->mutex);
				un->space = (un->capacity > space) ?
				    space : un->capacity;
				mutex_unlock(&un->mutex);
				return (err);
			}
			/* Check to see if its an EOV */
			if ((lab_len == 80) &&
			    (memcmp(eoflb, "EOV1", 4) == 0)) {

				mutex_lock(&un->mutex);
				un->space = 0;
				un->status.bits |= DVST_STOR_FULL;
				mutex_unlock(&un->mutex);
				msg1 = catgets(catfd, SET, 1235,
				    "found EOV1 at %#x");
				msg_buf = malloc_wait(strlen(msg1) + 12, 2, 0);
				sprintf(msg_buf, msg1, un->dt.tp.position);
				memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
				free(msg_buf);
				DevLog(DL_DETAIL(3060), un->dt.tp.position);
				DevLog(DL_ERR(3061));
				return (1);
			} {
				uint_t pos;
				err = read_position(un, open_fd, &pos);
				DevLog(DL_DETAIL(3062), un->dt.tp.position,
				    lab_len, space, err, pos,
				    eoflb[0] & 0xff, eoflb[1] & 0xff,
				    eoflb[2] & 0xff, eoflb[3] & 0xff);
			}
			goto hard_way;
		}
		err = 0;
		DevLog(DL_DETAIL(3063), un->dt.tp.position);
	}
hard_way:
	mutex_lock(&un->mutex);
	un->status.bits |= (DVST_POSITION | DVST_FORWARD);
	mutex_unlock(&un->mutex);

	/* Find the end of tape the hard way */

	DevLog(DL_DETAIL(3070));
	memccpy(d_mess,
	    catgets(catfd, SET, 1205, "finding EOD - skip to EOD"),
	    '\0', DIS_MES_LEN);
	(void) time(&start);
	err = skip_eom(open_fd, un);
	DevLog(DL_TIME(3071), time(NULL) - start);
	mutex_lock(&un->mutex);
	un->status.bits &= ~DVST_POSITION;
	mutex_unlock(&un->mutex);
	if (err < 0) {
		mutex_lock(&un->mutex);
		un->status.bits |= DVST_POSITION;
		un->status.bits &= ~(DVST_FORWARD);
		mutex_unlock(&un->mutex);
		memccpy(d_mess,
		    catgets(catfd, SET, 1204, "finding EOD - rewinding"),
		    '\0', DIS_MES_LEN);
		(void) time(&start);
		(void) scsi_cmd(open_fd, un, SCMD_REZERO_UNIT, 3600);
		DevLog(DL_TIME(3072), time(NULL) - start);
		if (un->dt.tp.position != 0) {
			DevLog(DL_DETAIL(3073));
		} else {
			DevLog(DL_DETAIL(3074));
		}

		memccpy(d_mess, catgets(catfd, SET, 1205,
		    "finding EOD - skip to EOD"), '\0', DIS_MES_LEN);
		mutex_lock(&un->mutex);
		un->status.bits |= (DVST_POSITION | DVST_FORWARD);
		mutex_unlock(&un->mutex);
		(void) time(&start);
		errno = 0;
		err = skip_eom(open_fd, un);
		DevLog(DL_TIME(3075), time(NULL) - start);
		mutex_lock(&un->mutex);
		un->status.bits &= ~(DVST_POSITION | DVST_FORWARD);
		mutex_unlock(&un->mutex);
		if (err < 0) {	/* still an error */
			memccpy(d_mess, catgets(catfd, SET, 2347,
			    "Media Error : skip to EOD returned error"),
			    '\0', DIS_MES_LEN);
			DevLog(DL_ERR(3076));
			if (resource)
				set_bad_media(un);
			memset(&mt_status, 0, sizeof (struct mtget));
			tmp_err = ioctl(open_fd, MTIOCGET, (char *)&mt_status);
			DevLog(DL_ERR(3222), tmp_err, mt_status.mt_dsreg,
			    mt_status.mt_erreg, errno);
			TAPEALERT_MTS(open_fd, un, mt_status.mt_erreg);
			return (1);
		}
	}
	/*
	 * Now, we are at EOD. Veryfy that the tape was properly terminated.
	 * Following the last file on the tape, it should have a filemark, an
	 * EOF/EOV label followed by either 1 or 2 filemarks. Verify that the
	 * tape was terminated properly.
	 */

	tmp_position = (uint_t)-1;
	(void) read_position(un, open_fd, &tmp_position);
	DevLog(DL_DETAIL(3056), tmp_position);

	do {
		err = skip_block_backward(un, open_fd, 1, &status);
		if (!(err == 0 && status == SUN_KEY_EOF)) {
			/*
			 * Tape does not have a filemark at EOD
			 */
			(void) read_position(un, open_fd, &tmp_position);
			DevLog(DL_ERR(3160), tmp_position);
			err = 1;
			break;
		}
		/*
		 * skip back 2 blocks
		 */
		err = skip_block_backward(un, open_fd, 2, &status);
		if (status == SUN_KEY_EOF && err <= 1) {
			/*
			 * These are acceptable results
			 */
			if (err == 1) {
				/*
				 * We hit a filemark. This means that the
				 * tape was terminated with 2 filemarks. We
				 * are sitting in front of the two filemarks.
				 * Back skip 2 blocks to be in front of the
				 * filemarks and label.
				 */
				err = skip_block_backward(un, open_fd, 2,
				    &status);
				if (!(err == 0 && status == SUN_KEY_EOF)) {
					/*
					 * Tape does not have the correct
					 * number of label and filemark.
					 */
					(void) read_position(un, open_fd,
					    &tmp_position);
					DevLog(DL_ERR(3160), tmp_position);
					err = 1;
					break;
				}
			}
		} else {
			/*
			 * Tape does not have the correct number of label and
			 * filemark.
			 */
			(void) read_position(un, open_fd, &tmp_position);
			DevLog(DL_ERR(3160), tmp_position);
			err = 1;
			break;
		}

		/*
		 * We are sitting in front of the filemark preceeding the
		 * label. Skip forward 1 block so we can read the label.
		 */
		(void) read_position(un, open_fd, &tmp_position);
		err = skip_block_forward(un, open_fd, 1, &status);
		if (err) {
			DevLog(DL_ERR(3160), tmp_position);
			err = 1;
			break;
		}
		/*
		 * We are now positioned between the filemark and the label.
		 * Verify that we have an EOF1/EOV1 label
		 */
		tmp_position = (uint_t)-1;
		(void) read_position(un, open_fd, &tmp_position);
		if (!((lab_len = read(open_fd, eoflb, 160)) == 80 &&
		    ((memcmp(eoflb, "EOF1", 4) == 0) ||
		    (memcmp(eoflb, "EOV1", 4) == 0)))) {
			DevLog(DL_ERR(3272), tmp_position);
			err = 1;
			break;
		}
		DevLog(DL_DETAIL(3273), tmp_position);

		/*
		 * We are now positioned after the EOF/EOV label. Backspace
		 * record over the label.
		 */
		err = skip_block_backward(un, open_fd, 1, &status);
		if (err) {
			DevLog(DL_ERR(3223), 1, status, err);
			break;
		}
		/*
		 * We are now positioned at the proper place to start
		 * archiving
		 */
		break;
	/* LINTED end-of-loop code not reached */
	} while (1);

	if (err) {
		set_bad_media(un);
		return (1);
	}
	/*
	 * Set the final position.
	 */
	tmp_position = (uint_t)-1;
	err = read_position(un, open_fd, &tmp_position);
	mutex_lock(&un->mutex);
	un->dt.tp.position = tmp_position;
	mutex_unlock(&un->mutex);
	DevLog(DL_DETAIL(3274), tmp_position);
	if (resource) {
		if (IS_IBM_COMPATIBLE(un->equ_type))
			assert_compression(open_fd, un);
		resource->archive.rm_info.position = un->dt.tp.position;
	}
	/* check the capacity, if no space then just return it here */
	if ((space = read_tape_space(un, open_fd)) == 0) {
		memccpy(d_mess, catgets(catfd, SET, 653,
		    "capacity at EOD returned as 0"), '\0', DIS_MES_LEN);
		mutex_lock(&un->mutex);
		un->space = 0;
		un->status.bits |= DVST_STOR_FULL;
		mutex_unlock(&un->mutex);
		DevLog(DL_DETAIL(3077));
		return (1);
	} else {
		mutex_lock(&un->mutex);
		un->space = (un->capacity > space) ? space : un->capacity;
		mutex_unlock(&un->mutex);
	}

	return (0);

}


int
find_tape_file(
	int open_fd,
	dev_ent_t *un,
	sam_resource_t *resource)
{
	int	err = 0;
	if (IS_TAPE(un) && un->dt.tp.drive_index != SAM_TAPE_DVR_DEFAULT) {
		register int	indx = un->dt.tp.drive_index;
		if (!(IO_table[indx].initialized)) {
			mutex_lock(&IO_table[indx].mutex);
			if (!(IO_table[indx].initialized)) {
				if (load_tape_io_lib(&tape_IO_entries[indx],
				    &(IO_table[indx].jmp_table))) {
					memccpy(un->dis_mes[DIS_MES_CRIT],
					    catgets(catfd, SET, 2162,
					    "Runtime interface not found."),
					    '\0', DIS_MES_LEN);
					DevLog(DL_ERR(3145));
					DownDevice(un, SAM_STATE_CHANGE);
					mutex_unlock(&IO_table[indx].mutex);
					return (-1);
				} else
					IO_table[indx].initialized = TRUE;
			}
			mutex_unlock(&IO_table[indx].mutex);
		}
		if (IO_table[indx].jmp_table.find_tape_file)
			return (IO_table[indx].jmp_table.find_tape_file(
			    open_fd, un, resource));
	}
	if (resource->archive.rm_info.valid) {
		if (resource->archive.rm_info.buffered_io) {
			/* Take into account position and file_offset */
			uint_t position = resource->archive.rm_info.position;
			uint_t file_offset =
			    resource->archive.rm_info.file_offset;
			uint_t pdu = un->sector_size;
			uint_t pdu_shift = 0;
			uint_t start_pda = 0;
			uint_t stk_offset = 0;
			uchar_t is_stk = (uchar_t)0;

			is_stk = (un->equ_type == DT_SQUARE_TAPE ||
			    un->equ_type == DT_9490 ||
			    un->equ_type == DT_9840 ||
			    un->equ_type == DT_9940 ||
			    un->equ_type == DT_TITAN || un->equ_type == DT_D3);

			/*
			 * find the shift count for this sector size
			 * for example, 1024 sector has shift 10
			 * for example, 512 sector has shift 9
			 */
			pdu_shift = 1;
			while ((pdu ^ (1 << pdu_shift)) && pdu_shift < 30) {
				pdu_shift++;
			}

			/* if its not a power of 2, fail */
			if (pdu_shift == 30) {
				DevLog(DL_ERR(1094), pdu);
				return (EINVAL);
			}
			start_pda = position +
			    ((file_offset * TAR_RECORDSIZE) >> pdu_shift);
			if (is_stk)
				stk_offset =
				    ((file_offset * TAR_RECORDSIZE) >>
				    pdu_shift);

			DevLog(DL_DETAIL(3183), position);
			if (is_stk)
				err = position_tape_offset(un, open_fd,
				    position, stk_offset);
			else
				err = position_tape(un, open_fd, start_pda);

		} else {
			/*
			 * Check if load requested with no positioning.
			 */
			if (resource->archive.rm_info.process_wtm) {
				uint_t current_position;

				err = read_position(un, open_fd,
				    &current_position);
				if (err == 0) {
					DevLog(DL_DETAIL(3244),
					    current_position,
					    un->dt.tp.position);
					/*
					 * Set current position in resource
					 * record.  Current position will be
					 * sent back to the file system and
					 * can be retrieved by the GETRMINFO
					 * ioctl.
					 */
					resource->archive.rm_info.position =
					    (current_position & un->dt.tp.mask);
				}
				resource->archive.rm_info.process_wtm = 0;
			} else {
				DevLog(DL_DETAIL(3183),
				    resource->archive.rm_info.position);
				err = position_tape(un, open_fd,
				    resource->archive.rm_info.position);
			}
		}
		return (err);
	} else {
		DevLog(DL_ERR(3184));
		return (EINVAL);
	}
}

int
forwardspace_record(
	dev_ent_t *un,	/* Needed for devlog */
	int fd,
	int count,
	int *sense)
{
	struct mtop	tape_op;
	struct mtget	status;

	*sense = 0;
	tape_op.mt_op = MTFSR;
	tape_op.mt_count = count;
	if (ioctl(fd, MTIOCTOP, &tape_op)) {
		memset(&status, 0, sizeof (struct mtget));
		(void) ioctl(fd, MTIOCGET, &status);
		*sense = status.mt_erreg;
		DevLog(DL_DETAIL(3275), count,
		    status.mt_erreg, status.mt_resid);
		return (abs(status.mt_resid));
	}
	return (0);
}

int
backspace_record(
	dev_ent_t *un, 	/* Needed for devlog */
	int fd,
	int count,
	int *sense)
{
	struct mtop	tape_op;
	struct mtget	status;

	*sense = 0;
	tape_op.mt_op = MTBSR;
	tape_op.mt_count = count;
	if (ioctl(fd, MTIOCTOP, &tape_op)) {
		memset(&status, 0, sizeof (struct mtget));
		(void) ioctl(fd, MTIOCGET, &status);
		*sense = status.mt_erreg;
		DevLog(DL_DETAIL(3223), count,
		    status.mt_erreg, status.mt_resid);
		return (abs(status.mt_resid));
	}
	return (0);
}

int
backspace_file(
	dev_ent_t *un,	/* Needed for devlog */
	int fd,
	int count,
	int *sense)
{
	struct mtop	tape_op;
	struct mtget	status;

	*sense = 0;
	tape_op.mt_op = MTBSF;
	tape_op.mt_count = count;
	if (ioctl(fd, MTIOCTOP, &tape_op)) {
		memset(&status, 0, sizeof (struct mtget));
		(void) ioctl(fd, MTIOCGET, &status);
		*sense = status.mt_erreg;
		DevLog(DL_DETAIL(3270), count,
		    status.mt_erreg, status.mt_resid);
		return (abs(status.mt_resid));
	}
	return (0);
}

int
forwardspace_file(
	dev_ent_t *un,	/* Needed for devlog */
	int fd,
	int count,
	int *sense)
{
	struct mtop	tape_op;
	struct mtget	status;

	*sense = 0;
	tape_op.mt_op = MTFSF;
	tape_op.mt_count = count;
	if (ioctl(fd, MTIOCTOP, &tape_op)) {
		memset(&status, 0, sizeof (struct mtget));
		(void) ioctl(fd, MTIOCGET, &status);
		*sense = status.mt_erreg;
		DevLog(DL_DETAIL(3271), count,
		    status.mt_erreg, status.mt_resid);
		return (abs(status.mt_resid));
	}
	return (0);
}

/*
 * Skip_block_forward - skip tape blocks. Both data records and filemarks are
 * treated as blocks.
 */
int
skip_block_forward(
	dev_ent_t *un,	/* Needed for devlog */
	int fd,
	int count,
	int *sense)
{
	int err;
	int status;
	int resid;

	mutex_lock(&un->mutex);
	un->status.bits |= (DVST_POSITION | DVST_FORWARD);
	mutex_unlock(&un->mutex);
	err = forwardspace_record(un, fd, count, &status);
	resid = err;
	if (err) {
		if (status == SUN_KEY_EOF) {
			err = forwardspace_file(un, fd, 1, sense);
			if (err) {
				mutex_lock(&un->mutex);
				un->status.bits &=
				    ~(DVST_POSITION | DVST_FORWARD);
				mutex_unlock(&un->mutex);
				return (resid);
			} else
				resid--;
		}
	}
	*sense = status;
	mutex_lock(&un->mutex);
	un->status.bits &= ~(DVST_POSITION | DVST_FORWARD);
	mutex_unlock(&un->mutex);
	return (resid);
}

/*
 * Skip_block_backward - skip tape blocks. Both data records and filemarks
 * are treated as blocks. This function behaves like a space record which
 * crosses filemarks.
 */
int
skip_block_backward(
	dev_ent_t *un,	/* Needed for devlog */
	int fd,
	int count,
	int *sense)
{
	int err;
	int status;
	int resid;

	mutex_lock(&un->mutex);
	un->status.bits |= DVST_POSITION;
	un->status.bits &= ~(DVST_FORWARD);
	mutex_unlock(&un->mutex);
	err = backspace_record(un, fd, count, &status);
	resid = err;
	if (err) {
		if (status == SUN_KEY_EOF) {
			err = backspace_file(un, fd, 1, sense);
			if (err) {
				mutex_lock(&un->mutex);
				un->status.bits &=
				    ~(DVST_POSITION | DVST_FORWARD);
				mutex_unlock(&un->mutex);
				return (resid);
			} else
				resid--;
		}
	}
	*sense = status;
	mutex_lock(&un->mutex);
	un->status.bits &= ~(DVST_POSITION | DVST_FORWARD);
	mutex_unlock(&un->mutex);
	return (resid);
}


/*
 * rewind_tape - rewind the tape
 */
int
rewind_tape(
	dev_ent_t *un,
	int open_fd)
{
	time_t	start;
	char	*d_mess = un->dis_mes[DIS_MES_NORM];
	char	vollb[160];

	un->dt.tp.stage_pos = 0;
	memccpy(d_mess, catgets(catfd, SET, 2128, "rewinding"),
	    '\0', DIS_MES_LEN);
	(void) time(&start);
	if (scsi_cmd(open_fd, un, SCMD_REZERO_UNIT, 3600) != 0) {
		DevLog(DL_ERR(3185));
		TAPEALERT_SKEY(open_fd, un);
		return (1);
	}
	DevLog(DL_TIME(3196), time(NULL) - start);
	if (read(open_fd, vollb, 160) == 80 && memcmp(vollb, "VOL1", 4) == 0)
		return (0);
	DevLog(DL_ERR(3224));
	return (1);
}

/*
 * rewind_skipeom - rewind then skip to eom.
 */
int
rewind_skipeom(
	dev_ent_t *un,
	int open_fd)
{
	int	err = 0;
	time_t	start;
	char	*d_mess = un->dis_mes[DIS_MES_NORM];

	un->dt.tp.stage_pos = 0;
	memccpy(d_mess, catgets(catfd, SET, 2128, "rewinding"),
	    '\0', DIS_MES_LEN);
	(void) time(&start);
	if (scsi_cmd(open_fd, un, SCMD_REZERO_UNIT, 3600) != 0) {
		DevLog(DL_ERR(3185));
		TAPEALERT_SKEY(open_fd, un);
		return (1);
	}
	DevLog(DL_TIME(3196), time(NULL) - start);

	memccpy(d_mess, catgets(catfd, SET, 2351, "skipping to  EOD"),
	    '\0', DIS_MES_LEN);
	(void) time(&start);
	err = skip_eom(open_fd, un);
	DevLog(DL_TIME(3186), time(NULL) - start);

	if (err < 0) {
		DevLog(DL_ERR(3187));
		return (1);
	}
	return (0);
}

/*
 * look over scsi sense and see if the "needs cleaning" bit is on return 0 if
 * not set,  !0 if it is set.
 *
 */

int
check_cleaning_error(
	dev_ent_t *un,
	int open_fd,
	void *vold_sense)
{
	int	cleaning = 0, resid = 0;
	uchar_t	*bytes;
	sam_extended_sense_t *local_sense = NULL, *sense = vold_sense;

	if (sense == NULL) {
		local_sense = (sam_extended_sense_t *)malloc_wait(120, 2, 0);
		memset(local_sense, 0, 120);
		(void) scsi_cmd(open_fd, un, SCMD_REQUEST_SENSE,
		    0, local_sense, 120, &resid);
		sense = local_sense;
	}
	bytes = (uchar_t *)sense;

	switch (un->equ_type) {
	case DT_LINEAR_TAPE:
		if (sense->es_add_code == 0x80 &&
		    (sense->es_qual_code == 0x01 ||
		    sense->es_qual_code == 0x02)) {
			cleaning = 1;
		} else if (sense->es_add_code == 0x00 &&
		    sense->es_qual_code == 0x17) {
			/* SuperDLT has a different ASC/ASCQ */
			cleaning = 1;
		}
		break;

	case DT_EXABYTE_TAPE:
	case DT_EXABYTE_M2_TAPE:
	case DT_IBM3580:
		cleaning = *(bytes + 21) & 0x08;
		break;

	case DT_SONYAIT:
	case DT_SONYSAIT:
		cleaning = *(bytes + 26) & 0x08;
		break;

	case DT_VIDEO_TAPE:
		cleaning = *(bytes + 16) & 0x02;
		break;

	case DT_FUJITSU_128:
		/* FALLTHROUGH */
	case DT_3590:
	case DT_3592:
		/* FALLTHROUGH */
	case DT_3570:
		cleaning = sense->es_key == 0x01 &&
		    sense->es_add_code == 0x00 && sense->es_qual_code == 0x17;
		break;

	case DT_SONYDTF:
		cleaning = *(bytes + 21) & 0x01;
		break;

	case DT_9840:
	case DT_9940:
	case DT_TITAN:
		cleaning = (sense->es_key == 0) &&
		    (sense->es_add_code == 0) && (sense->es_qual_code == 0x17);
		break;

	default:
		break;
	}

	if (local_sense)
		free(local_sense);

	if (cleaning)
		DevLog(DL_DETAIL(3188));

	return (cleaning != 0);
}

/*
 * check_eot - check sense data for leot or peot return true for eot, else
 * false.  Sets EOP flag if sense data indicates a check condition(writing
 * past EOP).
 */

int
check_eot(
	dev_ent_t *un,	/* needed for DevLog */
	int *EOP,
	sam_extended_sense_t *sense)
{
	*EOP = FALSE;

	DevLog(DL_DETAIL(3229), sense->es_key, sense->es_eom);

	if (sense->es_key == KEY_NO_SENSE && sense->es_eom)
		return (TRUE);

	if (sense->es_key == KEY_VOLUME_OVERFLOW ||
	    sense->es_key == KEY_MEDIUM_ERROR ||
	    sense->es_key == KEY_HARDWARE_ERROR) {
		*EOP = TRUE;
		return (TRUE);
	}
	return (FALSE);
}

/*
 * assert_compression - Insure that the 3590 and 3570 are running with
 * compression.  un->io_mutex held on entry.
 *
 * Use the 0x10 mode sense page and the block descriptor to set compression on
 * the 3590.
 */

void
assert_compression(
	int open_fd,
	dev_ent_t *un)
{
	int	resid;
	char	ms_35xx[255];	/* The entire mode sense/select area */
	char	*what = "mode sense";
	sam_extended_sense_t *sense;

	sense = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);
	memset(sense, 0, sizeof (sam_extended_sense_t));
	memset(ms_35xx, 0, 255);
	if (scsi_cmd(open_fd, un, SCMD_MODE_SENSE, 5, ms_35xx, 255, 0x10,
	    &resid) >= 0) {
		if (ms_35xx[26] != 0x01) {
			DevLog(DL_DETAIL(3034));
			if (un->equ_type == DT_FUJITSU_128)
				/* must be 8 for Fujitsu */
				ms_35xx[3] = 0x8;
			ms_35xx[0] = 0;
			ms_35xx[4] = 0x0;	/* use default density */
			ms_35xx[26] = 0x01;	/* use compression */
			what = "mode select";
			memset(sense, 0, sizeof (sam_extended_sense_t));
			if (scsi_cmd(open_fd, un, SCMD_MODE_SELECT, 5,
			    ms_35xx, 28, 0, &resid) > 0)
				return;
		} else {
			DevLog(DL_DETAIL(3035));
			return;
		}
	}
	DevLog(DL_ERR(3036), what);
	DevLogCdb(un);
	DevLogSense(un);
	TAPEALERT_SKEY(open_fd, un);
}

/*
 * skip_eom - position the drive to eom.
 *
 * io_mutex should be held on entry.
 *
 * returns 0 if successful !0 if failed
 */

int
skip_eom(
	int open_fd,
	dev_ent_t *un)
{
	struct mtop	tape_op;
	int		ret = 0;

	memset(&tape_op, 0, sizeof (struct mtop));
	tape_op.mt_op = MTEOM;	/* skip to eom */
	ret = ioctl(open_fd, MTIOCTOP, &tape_op);

	/*
	 * Recovery, skip two file marks backwards(towards BOT) then attempt
	 * the skip again.
	 */

	if (ret < 0) {
		DevLog(DL_SYSERR(3037));
		memset(&tape_op, 0, sizeof (struct mtop));
		tape_op.mt_op = MTBSF;
		tape_op.mt_count = 2;
		(void) ioctl(open_fd, MTIOCTOP, &tape_op);

		memset(&tape_op, 0, sizeof (struct mtop));
		tape_op.mt_op = MTEOM;	/* skip to eom */
		ret = ioctl(open_fd, MTIOCTOP, &tape_op);
		if (ret >= 0)
			DevLog(DL_ERR(3234));
		else
			DevLog(DL_SYSERR(3235));
	}
	return (ret);
}
