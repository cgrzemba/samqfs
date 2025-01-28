/*
 * tape2.c - support for tapes part two.
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

#pragma ident "$Revision: 1.60 $"

static char *_SrcFile = __FILE__;

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/mtio.h>
#include <sys/scsi/scsi.h>

#include "aml/external_data.h"
#include "aml/device.h"
#include "sam/devinfo.h"
#include "aml/dev_log.h"
#include "aml/tapes.h"
#include "aml/shm.h"
#include "aml/mode_sense.h"
#include "aml/tplabels.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"
#include "aml/tapealert.h"

#include "sam/lint.h"
#include "sam/custmsg.h"
#include "sam/lib.h"

#define	RDCAP_SENSE_LEN  60	/* length of sense for read_capacity */

int    get_9x40_space(dev_ent_t *un, int open_fd);

#define	ABS_POSITION(_p)	((_p) & un->dt.tp.mask)

#define	TIMING

/* globals */

extern shm_alloc_t master_shm, preview_shm;

/*
 * position_tape - set tape to desired location
 *
 * io mutex held on entry and exit.
 */

int
position_tape(dev_ent_t *un, int open_fd, uint_t position)
{
	int	timeout, cleaning;
	char	*d_mess;
	char	*msg_buf, *msg1;
	uint_t	current_pos = 0;
	time_t	start, stop;

	SANITY_CHECK(un != (dev_ent_t *)0);

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
					DevLog(DL_ERR(3146));
					DownDevice(un, SAM_STATE_CHANGE);
					return (-1);
				} else
					IO_table[indx].initialized = TRUE;
			}
			mutex_unlock(&IO_table[indx].mutex);
		}
		if (IO_table[indx].jmp_table.position_tape)
			return (IO_table[indx].jmp_table.position_tape(
			    un, open_fd, position));
	}
	d_mess = un->dis_mes[DIS_MES_NORM];
	if (!read_position(un, open_fd, &current_pos)) {
		if (ABS_POSITION(current_pos) == ABS_POSITION(position)) {
			mutex_lock(&un->mutex);
			un->dt.tp.next_read = position;
			mutex_unlock(&un->mutex);
			return (0);
		}
	}
	timeout = un->dt.tp.position_timeout;
	if (timeout == 0)
		timeout = TP_PT_DEFAULT;
	DevLog(DL_DETAIL(3233), un->dt.tp.position_timeout, un->equ_type);
	mutex_lock(&un->mutex);
	un->dt.tp.next_read = 0;
	un->status.bits |= DVST_POSITION;
	if (ABS_POSITION(position) > ABS_POSITION(current_pos))
		un->status.bits |= DVST_FORWARD;
	else
		un->status.bits &= ~DVST_FORWARD;
	mutex_unlock(&un->mutex);

	msg1 = catgets(catfd, SET, 1941, "Positioning to %#x");
	msg_buf = malloc_wait(strlen(msg1) + 12, 2, 0);
	sprintf(msg_buf, msg1, position);
	memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
	free(msg_buf);
	(void) time(&start);
	if (scsi_cmd(open_fd, un, SCMD_LOCATE, timeout, position)) {
		sam_extended_sense_t *sense = (sam_extended_sense_t *)
		    SHM_REF_ADDR(un->sense);

		DevLog(DL_TIME(3066), time(NULL) - start);
		cleaning = check_cleaning_error(un, open_fd, sense);
		mutex_lock(&un->mutex);
		if (cleaning)
			un->status.b.cleaning = cleaning;
		un->status.bits &= ~DVST_POSITION;
		mutex_unlock(&un->mutex);
		DevLog(DL_ERR(3067), position);
		DevLogCdb(un);
		DevLogSense(un);
		msg1 = catgets(catfd, SET, 1942,
		    "Positioning to %#x-failed, check log");
		msg_buf = malloc_wait(strlen(msg1) + 12, 2, 0);
		sprintf(msg_buf, msg1, position);
		memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
		free(msg_buf);
		TAPEALERT_SKEY(open_fd, un);
		return (EIO);
	}
	*d_mess = '\0';
	(void) time(&stop);
	if ((stop != start && (stop - start > 2)) ||
	    position == un->dt.tp.position) {
		DevLog(DL_TIME(3066), stop - start);
	}
	if (read_position(un, open_fd, &current_pos)) {
		DevLog(DL_ERR(3265), position);
		msg1 = catgets(catfd, SET, 2980,
		    "Read Position near %#x-failed, check log");
		msg_buf = malloc_wait(strlen(msg1) + 12, 2, 0);
		sprintf(msg_buf, msg1, position);
		memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
		free(msg_buf);
		return (EIO);
	}
	if (ABS_POSITION(current_pos) != ABS_POSITION(position)) {
		DevLog(DL_ERR(3266), current_pos, position);
		msg1 = catgets(catfd, SET, 2981,
		    "Locate to %#x-failed, check log");
		msg_buf = malloc_wait(strlen(msg1) + 12, 2, 0);
		sprintf(msg_buf, msg1, position);
		memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
		free(msg_buf);
		return (EIO);
	}
	un->dt.tp.stage_pos = position;
	mutex_lock(&un->mutex);
	un->status.bits &= ~DVST_POSITION;
	un->dt.tp.next_read = position;
	mutex_unlock(&un->mutex);

	msg1 = catgets(catfd, SET, 1937, "Positioned at %#x");
	msg_buf = malloc_wait(strlen(msg1) + 12, 2, 0);
	sprintf(msg_buf, msg1, position);
	memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
	free(msg_buf);
	DevLog(DL_DETAIL(3093), position);
	return (0);
}

/*
 * position_tape_offset - set tape to desired location.
 *
 * position a tape to the location pointed to by the position(from
 * read_position) and a block offset.  Used by drives that cannot just
 * increment the block number from the read position command. These drives
 * are, STK.
 *
 * Used dt.tp.stage_pos as the last position(using locate).
 *
 * io mutex held on entry and exit.
 */

int
position_tape_offset(
	dev_ent_t *un,		/* dev_ent */
	int open_fd,		/* open fd for device */
	uint_t position,	/* position from read_position */
	uint_t offset)		/* offset block */
{
	int	err = 0, skip_count = 0, timeout;
	char	*d_mess;
	char	*msg_buf, *msg1;
	uint_t	current_pos = 0, current_offset = 0;
	time_t	start;
	uchar_t	no_back = FALSE, have_current = FALSE, have_retried = FALSE;

	SANITY_CHECK(un != (dev_ent_t *)0);

	timeout = un->dt.tp.position_timeout;
	if (timeout == 0)
		timeout = TP_PT_DEFAULT;
	DevLog(DL_ALL(3233), un->dt.tp.position_timeout, un->equ_type);

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
					DevLog(DL_ERR(3147));
					DownDevice(un, SAM_STATE_CHANGE);
					return (-1);
				} else
					IO_table[indx].initialized = TRUE;
			}
			mutex_unlock(&IO_table[indx].mutex);
		}
		if (IO_table[indx].jmp_table.position_tape_offset)
			return (
			    IO_table[indx].jmp_table.position_tape_offset(
			    un, open_fd, position, offset));
	}
	d_mess = un->dis_mes[DIS_MES_NORM];
	if (un->dt.tp.stage_pos == 0) {
		if (read_position(un, open_fd, &current_pos)) {
			DevLog(DL_ERR(3191), position);
			return (EIO);
		}
		have_current = TRUE;
	}
	if (((un->dt.tp.stage_pos != 0) ?
	    position != un->dt.tp.stage_pos : position != current_pos)) {
		if ((err = position_tape(un, open_fd, position)))
			return (err);
		current_pos = position;
		no_back = TRUE;
	} else if (!have_current && read_position(un, open_fd, &current_pos)) {
		DevLog(DL_ERR(3192), position);
		return (EIO);
	}
try_again:

	current_offset = (current_pos & un->dt.tp.mask) -
	    (position & un->dt.tp.mask);
	skip_count = offset - current_offset;
	if (skip_count < 0 && no_back) {
		DevLog(DL_DETAIL(3094));
	}
	err = 0;
	if (skip_count != 0) {
		int    resid;

		mutex_lock(&un->mutex);
		un->status.bits |= DVST_POSITION;
		if (skip_count > 0)
			un->status.bits |= DVST_FORWARD;
		else
			un->status.bits &= ~DVST_FORWARD;
		mutex_unlock(&un->mutex);

		(void) time(&start);
		DevLog(DL_DETAIL(3095), skip_count);
		msg1 = catgets(catfd, SET, 1922, "Position with FSR %d");
		msg_buf = malloc_wait(strlen(msg1) + 12, 2, 0);
		sprintf(msg_buf, msg1, skip_count);
		memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
		free(msg_buf);
		err = scsi_cmd(open_fd, un, SCMD_SPACE, timeout, skip_count,
		    0x0, &resid);
		if (err || resid != 0) {
			sam_extended_sense_t *sense = (sam_extended_sense_t *)
			    SHM_REF_ADDR(un->sense);

			DevLog(DL_ERR(3096), sense->es_code, sense->es_add_code,
			    sense->es_qual_code, sense->es_filmk ? 'F' : ' ',
			    sense->es_eom ? 'M' : ' ');
			err = EIO;
			if (!have_retried) {
				DevLog(DL_RETRY(3097));
				(void) scsi_cmd(open_fd, un, SCMD_REZERO_UNIT,
				    3600);
				if (!position_tape(un, open_fd, position)) {
					err = 0;
					current_pos = position;
					have_current = no_back = have_retried
					    = TRUE;
					goto try_again;
				}
			}
			TAPEALERT_SKEY(open_fd, un);
		}
		DevLog(DL_TIME(3098), err ? 'E' : un->status.b.forward ?
		    'F' : 'B', abs(skip_count), time(NULL) - start);
	}
	mutex_lock(&un->mutex);
	if (err)
		un->dt.tp.next_read = 0;
	else {
		mutex_unlock(&un->mutex);
		(void) read_position(un, open_fd, &current_pos);
		mutex_lock(&un->mutex);
		un->dt.tp.next_read = current_pos & un->dt.tp.mask;
		current_offset = (current_pos & un->dt.tp.mask) -
		    (position & un->dt.tp.mask);
		skip_count = offset - current_offset;
		if (skip_count != 0) {
			mutex_unlock(&un->mutex);
			goto try_again;
		}
	}
	un->status.bits &= ~DVST_POSITION;
	mutex_unlock(&un->mutex);
	*d_mess = '\0';
	return (err);
}

/*
 * read_position - return position of tape.
 */

int
read_position(dev_ent_t *un, int open_fd, uint_t *position)
{
	read_position_t pd;


	SANITY_CHECK(un != (dev_ent_t *)0);

	if (IS_TAPE(un) && un->dt.tp.drive_index != SAM_TAPE_DVR_DEFAULT) {
		register int    indx = un->dt.tp.drive_index;
		if (!(IO_table[indx].initialized)) {
			mutex_lock(&IO_table[indx].mutex);
			if (!(IO_table[indx].initialized)) {
				if (load_tape_io_lib(&tape_IO_entries[indx],
				    &(IO_table[indx].jmp_table))) {
					memccpy(un->dis_mes[DIS_MES_CRIT],
					    catgets(catfd, SET, 2162,
					    "Runtime interface not found"),
					    '\0', DIS_MES_LEN);
					DevLog(DL_ERR(3148));
					DownDevice(un, SAM_STATE_CHANGE);
					return (-1);
				} else
					IO_table[indx].initialized = TRUE;
			}
			mutex_unlock(&IO_table[indx].mutex);
		}
		if (IO_table[indx].jmp_table.read_position)
			return (IO_table[indx].jmp_table.read_position(
			    un, open_fd, position));
	}
	if (scsi_cmd(open_fd, un, SCMD_READ_POSITION, 100, &pd, sizeof (pd),
	    (int *)NULL) != sizeof (pd)) {
		sam_extended_sense_t *sense = (sam_extended_sense_t *)
		    SHM_REF_ADDR(un->sense);

		if (check_cleaning_error(un, open_fd, sense)) {
			mutex_lock(&un->mutex);
			un->status.bits |= DVST_CLEANING;
			mutex_unlock(&un->mutex);
		}
		DevLog(DL_ERR(3193));
		DevLogCdb(un);
		DevLogSense(un);
		*position = 0;
		TAPEALERT_SKEY(open_fd, un);
		return (-1);
	}
	if (!pd.BPU) {
		*position = (uint_t)((pd.fbl[0] << 24) | (pd.fbl[1] << 16) |
		    (pd.fbl[2] << 8) | pd.fbl[3]);
		return (0);
	}
	return (-1);		/* Position unknown */
}

/*
 * read_tape_capacity - return total space on a tape in units of 1024.
 *
 * CAUTION - Only call this routine when tape is at, or near, BOT. Many devices
 * only return the amount of space left, from the current position. Calling
 * this routine for one of those devices when not at BOT, will return an
 * amount that is really, really wrong.
 */

uint64_t
read_tape_capacity(dev_ent_t *un, int open_fd)
{
	int		resid;
	uint64_t	return_val = 0;
	uint64_t	tmp_val = (uint64_t) 0;
	uint64_t	tmp_val1;

	SANITY_CHECK(un != (dev_ent_t *)0);

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
					DevLog(DL_ERR(3149));
					DownDevice(un, SAM_STATE_CHANGE);
					return (UINT64_MAX);
				} else
					IO_table[indx].initialized = TRUE;
			}
			mutex_unlock(&IO_table[indx].mutex);
		}
		if (IO_table[indx].jmp_table.read_tape_capacity)
			return (IO_table[indx].jmp_table.read_tape_capacity(
			    un, open_fd));
	}
	switch (un->equ_type) {
	case DT_LINEAR_TAPE:	/* reports in units of 4096 25-28 */
		/* FALLTHROUGH */
	case DT_EXABYTE_TAPE:	/* reports in units of 1024 23-25 */
		/* FALLTHROUGH */
	case DT_EXABYTE_M2_TAPE:	/* reports in units of 32768 23-25 */
		/* FALLTHROUGH */
	case DT_SONYAIT:	/* reports in units of 1024 22-25 */
	case DT_SONYSAIT:	/* reports in units of 1024 22-25 */
		/* FALLTHROUGH */
	case DT_SONYDTF:	/* reports in units of 116868 38-41 */
		/* DTF-2 tape reports in units of 233908 38-41 */
		{
			union {
				uchar_t	dmy[RDCAP_SENSE_LEN];
				struct	scsi_extended_sense sense;
			} s_data;

			memset(s_data.dmy, 0, RDCAP_SENSE_LEN);
			if (scsi_cmd(open_fd, un, SCMD_REQUEST_SENSE, 0,
			    &s_data, RDCAP_SENSE_LEN, &resid) < 0) {
				sam_extended_sense_t *sense =
				    (sam_extended_sense_t *)
				    SHM_REF_ADDR(un->sense);

				if (check_cleaning_error(un, open_fd, sense)) {
					mutex_lock(&un->mutex);
					un->status.bits |= DVST_CLEANING;
					mutex_unlock(&un->mutex);
				}
				DevLog(DL_ERR(3099));
				SendCustMsg(HERE, 9341);
				DevLogCdb(un);
				DevLogSense(un);
				TAPEALERT_SKEY(open_fd, un);
				return (0);
			}
			if (check_cleaning_error(un, open_fd, &s_data.sense)) {
				mutex_lock(&un->mutex);
				un->status.bits |= DVST_CLEANING;
				mutex_unlock(&un->mutex);
			}
			if (un->equ_type == DT_LINEAR_TAPE) {
				/*
				 * The dlt will return a negative capacity if
				 * the tape is in the EOT area.  Since we are
				 * using 64bits shifting a 32 bit negative
				 * number by 2(to get units of 1024) should
				 * result in a number larger than the largest
				 * 32bit unsigned integer.
				 */

				tmp_val = (uint_t)((s_data.dmy[25] << 24) |
				    (s_data.dmy[26] << 16) |
				    (s_data.dmy[27] << 8) |
				    s_data.dmy[28]);
				/* Units of 1024 instead of 4096 */
				tmp_val = (tmp_val << 2);
				DevLog(DL_DETAIL(3213), tmp_val, 0,
				    s_data.dmy[25], s_data.dmy[26],
				    s_data.dmy[27], s_data.dmy[28]);

				/* if eoom bit or if eom area */
				if (s_data.sense.es_eom ||
				    (tmp_val > (long long) UINT_MAX)) {
					tmp_val = 0;
					DevLog(DL_DETAIL(3100));
				}
			} else if (un->equ_type == DT_EXABYTE_TAPE) {
				tmp_val = (uint_t)((s_data.dmy[23] << 16) |
				    (s_data.dmy[24] << 8) |
				    s_data.dmy[25]);
				/*
				 * if end of media, EXABYTE has seen fit to
				 * have the EOM flag mean your at eom or
				 * lbot, Byte 19 bit 0 is another flag that
				 * means lbot.
				 */
				DevLog(DL_DETAIL(3213), tmp_val, 1,
				    s_data.dmy[23], s_data.dmy[24],
				    s_data.dmy[25], 0);
				if (s_data.sense.es_eom &&
				    !(s_data.dmy[19] & 0x01)) {
					tmp_val = 0;
					DevLog(DL_DETAIL(3101));
				}
			} else if (un->equ_type == DT_EXABYTE_M2_TAPE) {
				tmp_val = (uint_t)((s_data.dmy[23] << 16) |
				    (s_data.dmy[24] << 8) | s_data.dmy[25]);
				tmp_val *= 32;
				/*
				 * if end of media, EXABYTE has seen fit to
				 * have the EOM flag mean your at eom or
				 * lbot, Byte 19 bit 0 is another flag that
				 * means lbot.
				 */
				DevLog(DL_DETAIL(3213), tmp_val, 1,
				    s_data.dmy[23], s_data.dmy[24],
				    s_data.dmy[25], 0);
				if (s_data.sense.es_eom &&
				    !(s_data.dmy[19] & 0x01)) {
					tmp_val = 0;
					DevLog(DL_DETAIL(3101));
				}
			} else if (un->equ_type == DT_SONYAIT ||
			    un->equ_type == DT_SONYSAIT) {
				tmp_val = (uint_t)((s_data.dmy[22] << 24) |
				    (s_data.dmy[23] << 16) |
				    (s_data.dmy[24] << 8) | s_data.dmy[25]);
				DevLog(DL_DETAIL(3213), tmp_val, 3,
				    s_data.dmy[22], s_data.dmy[23],
				    s_data.dmy[24], s_data.dmy[25]);
				/*
				 * While AIT supports the EOM flag, there is
				 * no BOT flag.  This means we can't
				 * determine if EOM is at the start or past
				 * EW at the end.
				 */
			} else {
				uint_t physblk;

				if (s_data.dmy[20] == 0x90 ||
				    s_data.dmy[20] == 0x92) {
					/* size of DTF-2 tape physical block */
					physblk = 233908;
				} else {
					/* size of DTF-1 tape physical block */
					physblk = 116868;
				}
				tmp_val = (uint_t)((s_data.dmy[38] << 24) |
				    (s_data.dmy[39] << 16) |
				    (s_data.dmy[40] << 8) | s_data.dmy[41]);

				/* units of 1024 */
				tmp_val = (tmp_val * physblk) / 1024;
				/*
				 * if end of media, SONY has seen fit to have
				 * the EOM flag mean you're at eom or lbot,
				 * Byte 19 bit 0 is another flag that means
				 * lbot.
				 */
				DevLog(DL_DETAIL(3213), tmp_val, 11,
				    s_data.dmy[38], s_data.dmy[39],
				    s_data.dmy[40], s_data.dmy[41]);

				if (s_data.sense.es_eom &&
				    !(s_data.dmy[19] & 0x01)) {
					tmp_val = 0;
					DevLog(DL_DETAIL(3198));
				}
			}

			if (tmp_val > UINT_MAX) {
				DevLog(DL_ERR(3102), tmp_val);
				tmp_val = DEFLT_CAPC(un->equ_type);
			}
			if (tmp_val == 0) {
				DevLog(DL_DETAIL(3103));
			}
			return_val = tmp_val;
		}
		break;

	case DT_DAT:
		{
			uchar_t dmy[50];

			memset(dmy, 0, sizeof (dmy));
			if (scsi_cmd(open_fd, un, SCMD_LOG_SENSE, 0,
			    &dmy, 1, 0x31, 0, 50, &resid) < 0) {
				sam_extended_sense_t *sense =
				    (sam_extended_sense_t *)
				    SHM_REF_ADDR(un->sense);

				if (check_cleaning_error(un, open_fd, sense)) {
					mutex_lock(&un->mutex);
					un->status.bits |= DVST_CLEANING;
					mutex_unlock(&un->mutex);
				}
				DevLog(DL_ERR(3104));
				SendCustMsg(HERE, 9341);
				DevLogCdb(un);
				DevLogSense(un);
				TAPEALERT_SKEY(open_fd, un);
				return (0);
			}
			if ((dmy[0] != 0x31) || (dmy[5] != 0x1)) {
				DevLog(DL_ERR(3105));
				SendCustMsg(HERE, 9342);
				return (0);
			}
			tmp_val = (uint_t)((dmy[8] << 24) | (dmy[9] << 16) |
			    (dmy[10] << 8) | dmy[11]);

			DevLog(DL_DETAIL(3213), tmp_val, 3, dmy[8], dmy[9],
			    dmy[10], dmy[11]);
			if (tmp_val > UINT_MAX) {
				DevLog(DL_ERR(3106), tmp_val);
			}
			return_val = tmp_val;
		}
		break;

	case DT_FUJITSU_128:
		{
			uint_t position;
			register uint_t approx_use;

			return_val = un->dt.tp.default_capacity;
			if (read_position(un, open_fd, &position) < 0) {
				DevLog(DL_ERR(3109));
				SendCustMsg(HERE, 9341);
				return_val = 0;
				break;
			}
			/*
			 * approx_use is the number of blocks written times
			 * the block size in units of 1024.
			 */
			tmp_val = (uint64_t)position *
			    (uint64_t)un->sector_size;
			approx_use = tmp_val / 1024;
			if (approx_use <= un->dt.tp.default_capacity)
				return_val = un->dt.tp.default_capacity
				    - approx_use;
			else
				return_val = 0;
			DevLog(DL_DETAIL(3110), return_val, position,
			    un->dt.tp.default_capacity, approx_use);
		}
		break;

	case DT_3592:
	case DT_3590:
		/* FALLTHROUGH */
	case DT_3570:
		{
			int dmy[10];
			struct PAGE_CODE_HEADER {
				ushort_t p_code;
				/* Contains DU(Disable Update), etc. */
				uchar_t p_ctl_byte;
				uchar_t p_len;
			};

			/*
			 * This just happens to line up on nice integer
			 * boundaries
			 */
			struct log_sense {
				uchar_t	page_code;
				uchar_t	res;
				ushort_t	page_len;
				struct	PAGE_CODE_HEADER E_PAGE;
				uchar_t	capacity[4];
				struct	PAGE_CODE_HEADER F_PAGE;
				uchar_t	fraction;
			} *log_pages = (struct log_sense *)& dmy;

			memset(dmy, 0, sizeof (dmy));
			/*
			 * Get parameter starting from 0x0e, nominal capacity
			 * of volume.
			 */
			if (scsi_cmd(open_fd, un, SCMD_LOG_SENSE, 0, &dmy, 1,
			    0x38, 0x0e, sizeof (dmy), &resid) < 0) {
				sam_extended_sense_t *sense =
				    (sam_extended_sense_t *)
				    SHM_REF_ADDR(un->sense);

				if (check_cleaning_error(un, open_fd, sense)) {
					mutex_lock(&un->mutex);
					un->status.bits |= DVST_CLEANING;
					mutex_unlock(&un->mutex);
				}
				DevLog(DL_ERR(3107));
				SendCustMsg(HERE, 9343);
				DevLogCdb(un);
				DevLogSense(un);
				TAPEALERT_SKEY(open_fd, un);
				return (0);
			}
			tmp_val = (uint_t)((log_pages->capacity[0] << 24) |
			    (log_pages->capacity[1] << 16) |
			    (log_pages->capacity[2] << 8) |
			    log_pages->capacity[3]);
			tmp_val1 = tmp_val;

			if (log_pages->fraction != 0)
				tmp_val = tmp_val -
				    ((tmp_val >> 8) * log_pages->fraction);

			DevLog(DL_DETAIL(3213), tmp_val, log_pages->fraction,
			    log_pages->capacity[0], log_pages->capacity[1],
			    log_pages->capacity[2], log_pages->capacity[3]);
			if (tmp_val > UINT_MAX) {
				DevLog(DL_ERR(3108), tmp_val);
			}
			if (tmp_val > tmp_val1)
				return_val = 0;
			else
				return_val = tmp_val;
		}
		break;

	case DT_IBM3580:
		{
			/*
			 * Remaining tape capacity is determined from Log
			 * Page 31, parameter 1: Main Partition Remaining
			 * Capacity.  The value is in megabytes and assumes
			 * no data compression.
			 */
			uchar_t dmy[50];

			memset(dmy, 0, sizeof (dmy));
			if (scsi_cmd(open_fd, un, SCMD_LOG_SENSE, 0, &dmy,
			    1, 0x31, 0, 50, &resid) < 0) {
				sam_extended_sense_t *sense =
				    (sam_extended_sense_t *)
				    SHM_REF_ADDR(un->sense);

				if (check_cleaning_error(un, open_fd, sense)) {
					mutex_lock(&un->mutex);
					un->status.bits |= DVST_CLEANING;
					mutex_unlock(&un->mutex);
				}
				DevLog(DL_ERR(3104));
				SendCustMsg(HERE, 9341);
				DevLogCdb(un);
				DevLogSense(un);
				TAPEALERT_SKEY(open_fd, un);
				return (0);
			}
			if ((dmy[0] != 0x31) || (dmy[21] != 0x3)) {
				DevLog(DL_ERR(3105));
				SendCustMsg(HERE, 9342);
				return (0);
			}
			tmp_val = (uint64_t)((dmy[24] << 24) | (dmy[25] << 16) |
			    (dmy[26] << 8) | dmy[27]);
			tmp_val *= 1024;

			DevLog(DL_DETAIL(3213), tmp_val, 3, dmy[24], dmy[25],
			    dmy[26], dmy[27]);
			if (tmp_val > UINT64_MAX) {
				DevLog(DL_ERR(3106), tmp_val);
			}
			return_val = tmp_val;
		}
		break;

	case DT_SQUARE_TAPE:
	case DT_9490:
	case DT_D3:
		{
			union {
				struct {
					uint_t
#if defined(_BIT_FIELDS_HTOL)
					sector:8,
					format:2,
					rec_cnt:22;
#else				/* defined(_BIT_FIELDS_HTOL) */
					rec_cnt:22,
					format:2,
					sector:8;
#endif				/* defined(_BIT_FIELDS_HTOL) */
				}    stk;
				uint_t    position;
			}    posit;
			register uint_t file_marks, approx_use;

			return_val = un->dt.tp.default_capacity;
			if (read_position(un, open_fd, &posit.position) < 0) {
				DevLog(DL_ERR(3109));
				SendCustMsg(HERE, 9341);
				return_val = 0;
				break;
			}
			if ((file_marks = posit.stk.sector) >= 254) {
				DevLog(DL_ERR(3216), file_marks, 254);
				return_val = 0;
				break;
			}
			if (un->equ_type == DT_SQUARE_TAPE)
				/* 3480 type use 73k for each tape mark */
				approx_use = 73 * file_marks;
			else
				/*
				 * 9490 and D3 use about 1500 bytes for each
				 * tape mark and 3000 bytes per inter-record
				 * gap.
				 */
				approx_use = (3000 * posit.stk.rec_cnt);

			/*
			 * approx_ use is the number of blocks written times
			 * the block size, in units of 1024.
			 */
			tmp_val = (long long)posit.stk.rec_cnt *
			    (long long)un->sector_size;
			approx_use = tmp_val / 1024;
			if (approx_use <= un->dt.tp.default_capacity)
				return_val = un->dt.tp.default_capacity
				    - approx_use;
			else
				return_val = 0;
			DevLog(DL_DETAIL(3110), file_marks, posit.stk.rec_cnt,
			    un->dt.tp.default_capacity, approx_use);
		}
		break;

	case DT_9840:
	case DT_9940:
	case DT_TITAN:
		{
			if ((return_val = get_9x40_space(un, open_fd)) == 0) {
				return_val = un->dt.tp.default_capacity;
			}
		}
		break;

		/*
		 * The physical tracks in the sense block are only valid
		 * within 30 seconds of tape movement.  If we are not at BOT,
		 * then there is a real good chance that we just came off a
		 * move of some kind.
		 */
	case DT_VIDEO_TAPE:
		{
			read_position_t pd;

			if (scsi_cmd(open_fd, un, SCMD_READ_POSITION, 0, &pd,
			    sizeof (pd), (int *)NULL) != sizeof (pd)) {
				sam_extended_sense_t *sense =
				    (sam_extended_sense_t *)
				    SHM_REF_ADDR(un->sense);

				if (check_cleaning_error(un, open_fd, sense)) {
					mutex_lock(&un->mutex);
					un->status.bits |= DVST_CLEANING;
					mutex_unlock(&un->mutex);
				}
				DevLog(DL_ERR(3111));
				SendCustMsg(HERE, 9341);
				DevLogCdb(un);
				DevLogSense(un);
				TAPEALERT_SKEY(open_fd, un);
				return (0);
			}
			/* if at BOT */
			if (pd.BOP) {
				DevLog(DL_DETAIL(3214), un->dt.tp.medium_type);
				switch (un->dt.tp.medium_type) {
				/* T-160 DDC-343 */
				case 0x0d:
					return_val = VHS343_SIZE;
					break;

				case 0x10:	/* T-180 DDC-367 */
					return_val = VHS367_SIZE;
					break;

				default:
					DevLog(DL_ERR(3112),
					    un->dt.tp.medium_type);
					/* FALLTHROUGH */
				case 0x0a:	/* M-120 DDC-258 */
					return_val = VHS258_SIZE;
					break;
				}
			} else {
				register uint_t tmp_val;
				union {
					uchar_t dmy[96];
					struct scsi_extended_sense sense;
				} s_data;

				memset(s_data.dmy, 0, 96);
				if (scsi_cmd(open_fd, un, SCMD_REQUEST_SENSE,
				    0, &s_data, 96, (int *)NULL) < 0) {
					sam_extended_sense_t *sense =
					    (sam_extended_sense_t *)
					    SHM_REF_ADDR(un->sense);

					if (check_cleaning_error(un, open_fd,
					    sense)) {
						mutex_lock(&un->mutex);
						un->status.bits |=
						    DVST_CLEANING;
						mutex_unlock(&un->mutex);
					}
					DevLog(DL_ERR(3113));
					SendCustMsg(HERE, 9341);
					DevLogCdb(un);
					DevLogSense(un);
					TAPEALERT_SKEY(open_fd, un);
					return (0);
				}
				if (check_cleaning_error(un, open_fd,
				    &s_data)) {
					mutex_lock(&un->mutex);
					un->status.bits |= DVST_CLEANING;
					mutex_unlock(&un->mutex);
				}
				/*
				 * physical tracks (return in request sense)
				 * are 16k long
				 */

				tmp_val = ((uint_t)((s_data.dmy[74] << 16) |
				    (s_data.dmy[75] << 8) |
				    s_data.dmy[76])) << 4;

				DevLog(DL_DETAIL(3213), tmp_val, 5,
				    s_data.dmy[74], s_data.dmy[75],
				    s_data.dmy[76], 0);
				if (tmp_val > UINT_MAX) {
					DevLog(DL_ERR(3114), tmp_val);
				}
				if (tmp_val <= un->capacity)
					return_val = un->capacity - tmp_val;

				DevLog(DL_DETAIL(3215), return_val,
				    un->capacity);

				if (s_data.sense.es_eom) {
					return_val = 0;
					DevLog(DL_DETAIL(3115));
				}
			}
		}
		break;

	default:
		DevLog(DL_ERR(3116));
		SendCustMsg(HERE, 9344);
		return (0);
		/* NOTREACHED */
		break;
	}

	if (return_val < TAPE_IS_FULL)
		return (0);

	return (return_val);
}

/*
 * read_tape_space - return space left on tape in units of 1024.
 *
 * return remaining space if the drive can do that, otherwise fake it by
 * computing (capacity - blocksize*position)
 */

uint64_t
read_tape_space(dev_ent_t *un, int open_fd)
{
	int		resid;
	uint64_t	return_val = 0;
	uint64_t	tmp_val = (uint64_t) 0;
	uint64_t	tmp_val1;

	SANITY_CHECK(un != (dev_ent_t *)0);

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
					DevLog(DL_ERR(3149));
					DownDevice(un, SAM_STATE_CHANGE);
					return (UINT64_MAX);
				} else
					IO_table[indx].initialized = TRUE;
			}
			mutex_unlock(&IO_table[indx].mutex);
		}
		if (IO_table[indx].jmp_table.read_tape_capacity)
			return (IO_table[indx].jmp_table.read_tape_capacity(
			    un, open_fd));
	}
	switch (un->equ_type) {
	case DT_LINEAR_TAPE:	/* reports in units of 4096 25-28 */
		/* FALLTHROUGH */
	case DT_EXABYTE_TAPE:	/* reports in units of 1024 23-25 */
		/* FALLTHROUGH */
	case DT_EXABYTE_M2_TAPE:	/* reports in units of 32768 23-25 */
		/* FALLTHROUGH */
	case DT_SONYAIT:	/* reports in units of 1024 22-25 */
	case DT_SONYSAIT:	/* reports in units of 1024 22-25 */
		/* FALLTHROUGH */
	case DT_SONYDTF:	/* DTF-1 reports in units of 116868 34-37 */
				/* DTF-2 reports in units of 233908 34-37 */
		{
			union {
				uchar_t dmy[RDCAP_SENSE_LEN];
				struct scsi_extended_sense sense;
			} s_data;

			memset(s_data.dmy, 0, RDCAP_SENSE_LEN);
			if (scsi_cmd(open_fd, un, SCMD_REQUEST_SENSE, 0,
			    &s_data, RDCAP_SENSE_LEN, &resid) < 0) {
				sam_extended_sense_t *sense =
				    (sam_extended_sense_t *)
				    SHM_REF_ADDR(un->sense);

				if (check_cleaning_error(un, open_fd, sense)) {
					mutex_lock(&un->mutex);
					un->status.bits |= DVST_CLEANING;
					mutex_unlock(&un->mutex);
				}
				DevLog(DL_ERR(3099));
				SendCustMsg(HERE, 9341);
				DevLogCdb(un);
				DevLogSense(un);
				TAPEALERT_SKEY(open_fd, un);
				return (0);
			}
			if (check_cleaning_error(un, open_fd, &s_data.sense)) {
				mutex_lock(&un->mutex);
				un->status.bits |= DVST_CLEANING;
				mutex_unlock(&un->mutex);
			}
			if (un->equ_type == DT_LINEAR_TAPE) {
				/*
				 * The dlt will return a negative capacity if
				 * the tape is in the EOT area.  Since we are
				 * using 64bits shifting a 32 bit negative
				 * number by 2(to get units of 1024) should
				 * result in a number larger than the largest
				 * 32bit unsigned integer.
				 */

				tmp_val = (uint_t)((s_data.dmy[25] << 24) |
				    (s_data.dmy[26] << 16) |
				    (s_data.dmy[27] << 8) | s_data.dmy[28]);
				/* Units of 1023 instead of 4096 */
				tmp_val = (tmp_val << 2);
				DevLog(DL_DETAIL(3213), tmp_val, 0,
				    s_data.dmy[25], s_data.dmy[26],
				    s_data.dmy[27], s_data.dmy[28]);
				if (s_data.sense.es_eom ||
				    (tmp_val > (long long)UINT_MAX)) {
					tmp_val = 0;
					DevLog(DL_DETAIL(3100));
				}
			} else if (un->equ_type == DT_EXABYTE_TAPE) {
				tmp_val = (uint_t)((s_data.dmy[23] << 16) |
				    (s_data.dmy[24] << 8) | s_data.dmy[25]);
				/*
				 * if end of media, EXABYTE has seen fit to
				 * have the EOM flag mean your at eom or
				 * lbot, Byte 19 bit 0 is another flag that
				 * means lbot.
				 */
				DevLog(DL_DETAIL(3213), tmp_val, 1,
				    s_data.dmy[23], s_data.dmy[24],
				    s_data.dmy[25], 0);
				if (s_data.sense.es_eom &&
				    !(s_data.dmy[19] & 0x01)) {
					tmp_val = 0;
					DevLog(DL_DETAIL(3101));
				}
			} else if (un->equ_type == DT_EXABYTE_M2_TAPE) {
				tmp_val = (uint_t)((s_data.dmy[23] << 16) |
				    (s_data.dmy[24] << 8) | s_data.dmy[25]);
				tmp_val *= 32;
				/*
				 * if end of media, EXABYTE has seen fit to
				 * have the EOM flag mean your at eom or
				 * lbot, Byte 19 bit 0 is another flag that
				 * means lbot.
				 */
				DevLog(DL_DETAIL(3213), tmp_val, 1,
				    s_data.dmy[23], s_data.dmy[24],
				    s_data.dmy[25], 0);
				if (s_data.sense.es_eom &&
				    !(s_data.dmy[19] & 0x01)) {
					tmp_val = 0;
					DevLog(DL_DETAIL(3101));
				}
			} else if (un->equ_type == DT_SONYAIT ||
			    un->equ_type == DT_SONYSAIT) {
				tmp_val = (uint_t)((s_data.dmy[22] << 24) |
				    (s_data.dmy[23] << 16) |
				    (s_data.dmy[24] << 8) | s_data.dmy[25]);
				DevLog(DL_DETAIL(3213), tmp_val, 3,
				    s_data.dmy[22], s_data.dmy[23],
				    s_data.dmy[24], s_data.dmy[25]);
				/*
				 * While AIT supports the EOM flag, there is
				 * no BOT flag.  This means we can't
				 * determine if EOM is at the start or past
				 * EW at the end.
				 */
			/* Sony DTF */
			} else {
				uint_t physblk;

				if (s_data.dmy[20] == 0x90 ||
				    s_data.dmy[20] == 0x92) {
					/* size of DTF-2 tape physical block */
					physblk = 233908;
				} else {
					/* size of DTF-1 tape physical block */
					physblk = 116868;
				}
				tmp_val = ((s_data.dmy[34] << 24) |
				    (s_data.dmy[35] << 16) |
				    (s_data.dmy[36] << 8) | s_data.dmy[37]);

				/* units of 1024 */
				tmp_val = (tmp_val * physblk) / 1024;
				/*
				 * if end of media, SONY has seen fit to have
				 * the EOM flag mean you're at eom or lbot,
				 * Byte 19 bit 0 is another flag that means
				 * lbot.
				 */
				DevLog(DL_DETAIL(3213), tmp_val, 11,
				    s_data.dmy[34], s_data.dmy[35],
				    s_data.dmy[36], s_data.dmy[37]);

				if (s_data.sense.es_eom &&
				    !(s_data.dmy[19] & 0x01)) {
					tmp_val = 0;
					DevLog(DL_DETAIL(3198));
				}
			}

			if (tmp_val > UINT_MAX) {
				DevLog(DL_ERR(3102), tmp_val);
				tmp_val = DEFLT_CAPC(un->equ_type);
			}
			if (tmp_val == 0) {
				DevLog(DL_DETAIL(3103));
			}
			return_val = tmp_val;
		}
		break;

	case DT_DAT:
		{
			uchar_t dmy[50];

			memset(dmy, 0, sizeof (dmy));
			if (scsi_cmd(open_fd, un, SCMD_LOG_SENSE, 0, &dmy, 1,
			    0x31, 0, 50, &resid) < 0) {
				sam_extended_sense_t *sense =
				    (sam_extended_sense_t *)
				    SHM_REF_ADDR(un->sense);

				if (check_cleaning_error(un, open_fd, sense)) {
					mutex_lock(&un->mutex);
					un->status.bits |= DVST_CLEANING;
					mutex_unlock(&un->mutex);
				}
				DevLog(DL_ERR(3104));
				SendCustMsg(HERE, 9341);
				DevLogCdb(un);
				DevLogSense(un);
				TAPEALERT_SKEY(open_fd, un);
				return (0);
			}
			if ((dmy[0] != 0x31) || (dmy[5] != 0x1)) {
				DevLog(DL_ERR(3105));
				SendCustMsg(HERE, 9342);
				return (0);
			}
			tmp_val = (uint_t)((dmy[8] << 24) | (dmy[9] << 16) |
			    (dmy[10] << 8) | dmy[11]);

			DevLog(DL_DETAIL(3213), tmp_val, 3, dmy[8], dmy[9],
			    dmy[10], dmy[11]);
			if (tmp_val > UINT_MAX) {
				DevLog(DL_ERR(3106), tmp_val);
			}
			return_val = tmp_val;
		}
		break;

	case DT_FUJITSU_128:
		{
			uint_t	position;
			register uint_t approx_use;

			return_val = un->dt.tp.default_capacity;
			if (read_position(un, open_fd, &position) < 0) {
				DevLog(DL_ERR(3109));
				SendCustMsg(HERE, 9341);
				return_val = 0;
				break;
			}
			/*
			 * approx_use is the number of blocks written times
			 * the block size in units of 1024.
			 */
			tmp_val = (long long)position *
			    (long long)un->sector_size;
			approx_use = tmp_val / 1024;
			if (approx_use <= un->dt.tp.default_capacity)
				return_val = un->dt.tp.default_capacity
				    - approx_use;
			else
				return_val = 0;
			DevLog(DL_DETAIL(3110), return_val, position,
			    un->dt.tp.default_capacity, approx_use);
		}
		break;

	case DT_3592:
	case DT_3590:
		/* FALLTHROUGH */
	case DT_3570:
		{
			int dmy[10];
			struct PAGE_CODE_HEADER {
				ushort_t p_code;
				/* Contains DU(Disable Update), etc. */
				uchar_t	p_ctl_byte;
				uchar_t	p_len;
			};

			/*
			 * This just happens to line up on nice integer
			 * boundaries
			 */
			struct log_sense {
				uchar_t	page_code;
				uchar_t	res;
				ushort_t	page_len;
				struct PAGE_CODE_HEADER E_PAGE;
				uchar_t	capacity[4];
				struct PAGE_CODE_HEADER F_PAGE;
				uchar_t	fraction;
			} *log_pages = (struct log_sense *)& dmy;

			memset(dmy, 0, sizeof (dmy));
			if (scsi_cmd(open_fd, un, SCMD_LOG_SENSE, 0, &dmy,
			    1, 0x38, 0x0e, sizeof (dmy), &resid) < 0) {
				sam_extended_sense_t *sense =
				    (sam_extended_sense_t *)
				    SHM_REF_ADDR(un->sense);

				if (check_cleaning_error(un, open_fd, sense)) {
					mutex_lock(&un->mutex);
					un->status.bits |= DVST_CLEANING;
					mutex_unlock(&un->mutex);
				}
				DevLog(DL_ERR(3107));
				SendCustMsg(HERE, 9343);
				DevLogCdb(un);
				DevLogSense(un);
				TAPEALERT_SKEY(open_fd, un);
				return (0);
			}
			tmp_val1 = tmp_val = (uint_t)
			    ((log_pages->capacity[0] << 24) |
			    (log_pages->capacity[1] << 16) |
			    (log_pages->capacity[2] << 8) |
			    log_pages->capacity[3]);

			if (log_pages->fraction != 0)
				tmp_val = tmp_val -
				    ((tmp_val >> 8) * log_pages->fraction);

			DevLog(DL_DETAIL(3213), tmp_val, log_pages->fraction,
			    log_pages->capacity[0], log_pages->capacity[1],
			    log_pages->capacity[2], log_pages->capacity[3]);
			if (tmp_val > UINT_MAX) {
				DevLog(DL_ERR(3108), tmp_val);
			}
			if (tmp_val > tmp_val1)
				return_val = 0;
			else
				return_val = tmp_val;
		}
		break;

	case DT_IBM3580:
		{
			/*
			 * Remaining tape capacity is determined from Log
			 * Page 31, parameter 1: Main Partition Remaining
			 * Capacity.  The value is in megabytes and assumes
			 * no data compression.
			 * Warning 2017: this page is deprecated in favour of page 17
			 *
                         * dmy[0] page code
                         * dmy[1] subpage code
                         * dmy[2,3] page code length
                         * dmy[4,5] parameter code
                         * dmy[6] parameter description
                         * dmy[7] parameter length
                         * dmy[8,9] paramter value
                         * dmy[10,11] parameter value
			 *
			 */
			uchar_t dmy[50];

			memset(dmy, 0, sizeof (dmy));
			if (scsi_cmd(open_fd, un, SCMD_LOG_SENSE, 0, &dmy, 1,
			    0x31, 0, 50, &resid) < 0) {
				sam_extended_sense_t *sense =
				    (sam_extended_sense_t *)
				    SHM_REF_ADDR(un->sense);

				if (check_cleaning_error(un, open_fd, sense)) {
					mutex_lock(&un->mutex);
					un->status.bits |= DVST_CLEANING;
					mutex_unlock(&un->mutex);
				}
				DevLog(DL_ERR(3104));
				SendCustMsg(HERE, 9341);
				DevLogCdb(un);
				DevLogSense(un);
				TAPEALERT_SKEY(open_fd, un);
				return (0);
			}
			if ((dmy[0] != 0x31) || (dmy[5] != 0x1)) {
				DevLog(DL_ERR(3105));
				SendCustMsg(HERE, 9342);
				return (0);
			}
			tmp_val = (uint_t)((dmy[8] << 24) | (dmy[9] << 16) |
			    (dmy[10] << 8) | dmy[11]);
			tmp_val *= 1024;

			DevLog(DL_DETAIL(3213), tmp_val, 3, dmy[8],
			    dmy[9], dmy[10], dmy[11]);
			if (tmp_val > UINT64_MAX) {
				DevLog(DL_ERR(3106), tmp_val);
			}
			return_val = tmp_val;
		}
		break;

	case DT_SQUARE_TAPE:
	case DT_9490:
	case DT_D3:
		{
			union {
				struct {
					uint_t
#if defined(_BIT_FIELDS_HTOL)
					sector:8,
					format:2,
					rec_cnt:22;
#else				/* defined(_BIT_FIELDS_HTOL) */
					rec_cnt:22,
					format:2,
					sector:8;
#endif				/* defined(_BIT_FIELDS_HTOL) */
				} stk;
				uint_t    position;
			}    posit;
			register uint_t file_marks, approx_use;

			return_val = un->dt.tp.default_capacity;
			if (read_position(un, open_fd, &posit.position) < 0) {
				DevLog(DL_ERR(3109));
				SendCustMsg(HERE, 9341);
				return_val = 0;
				break;
			}
			if ((file_marks = posit.stk.sector) >= 254) {
				DevLog(DL_ERR(3216), file_marks, 254);
				return_val = 0;
				break;
			}
			if (un->equ_type == DT_SQUARE_TAPE)
				/* 3480 type use 73k for each tape mark */
				approx_use = 73 * file_marks;
			else
				/*
				 * 9490 and D3 use about 1500 bytes for each
				 * tape mark and 3000 bytes per inter-record
				 * gap.
				 */
				approx_use = (3000 * posit.stk.rec_cnt);

			/*
			 * approx_ use is the number of blocks written times
			 * the block size, in units of 1024.
			 */
			tmp_val = (long long)posit.stk.rec_cnt *
			    (long long)un->sector_size;
			approx_use = tmp_val / 1024;
			if (approx_use <= un->dt.tp.default_capacity)
				return_val = un->dt.tp.default_capacity
				    - approx_use;
			else
				return_val = 0;
			DevLog(DL_DETAIL(3110), file_marks, posit.stk.rec_cnt,
			    un->dt.tp.default_capacity, approx_use);
		}
		break;

	case DT_9840:
	case DT_9940:
	case DT_TITAN:
		{
			return_val = get_9x40_space(un, open_fd);
		}
		break;

		/*
		 * The physical tracks in the sense block are only valid
		 * within 30 seconds of tape movement.  If we are not at BOT,
		 * then there is a real good chance that we just came off a
		 * move of some kind.
		 */
	case DT_VIDEO_TAPE:
		{
			read_position_t pd;

			if (scsi_cmd(open_fd, un, SCMD_READ_POSITION, 0, &pd,
			    sizeof (pd), (int *)NULL) != sizeof (pd)) {
				sam_extended_sense_t *sense =
				    (sam_extended_sense_t *)
				    SHM_REF_ADDR(un->sense);

				if (check_cleaning_error(un, open_fd, sense)) {
					mutex_lock(&un->mutex);
					un->status.bits |= DVST_CLEANING;
					mutex_unlock(&un->mutex);
				}
				DevLog(DL_ERR(3111));
				SendCustMsg(HERE, 9341);
				DevLogCdb(un);
				DevLogSense(un);
				TAPEALERT_SKEY(open_fd, un);
				return (0);
			}
			if (pd.BOP) {	/* if at BOT */
				DevLog(DL_DETAIL(3214), un->dt.tp.medium_type);
				switch (un->dt.tp.medium_type) {
				case 0x0d:	/* T-160 DDC-343 */
					return_val = VHS343_SIZE;
					break;

				case 0x10:	/* T-180 DDC-367 */
					return_val = VHS367_SIZE;
					break;

				default:
					DevLog(DL_ERR(3112),
					    un->dt.tp.medium_type);
					/* FALLTHROUGH */
				case 0x0a:	/* M-120 DDC-258 */
					return_val = VHS258_SIZE;
					break;
				}
			} else {
				register uint_t  tmp_val;
				union {
					uchar_t dmy[96];
					struct scsi_extended_sense sense;
				} s_data;

				memset(s_data.dmy, 0, 96);
				if (scsi_cmd(open_fd, un, SCMD_REQUEST_SENSE,
				    0, &s_data, 96, (int *)NULL) < 0) {
					sam_extended_sense_t *sense =
					    (sam_extended_sense_t *)
					    SHM_REF_ADDR(un->sense);

					if (check_cleaning_error(un, open_fd,
					    sense)) {

						mutex_lock(&un->mutex);
						un->status.bits |=
						    DVST_CLEANING;
						mutex_unlock(&un->mutex);
					}
					DevLog(DL_ERR(3113));
					SendCustMsg(HERE, 9341);
					DevLogCdb(un);
					DevLogSense(un);
					TAPEALERT_SKEY(open_fd, un);
					return (0);
				}
				if (check_cleaning_error(un, open_fd,
				    &s_data)) {

					mutex_lock(&un->mutex);
					un->status.bits |= DVST_CLEANING;
					mutex_unlock(&un->mutex);
				}
				/*
				 * physical tracks (return in request sense)
				 * are 16k long
				 */

				tmp_val = ((uint_t)((s_data.dmy[74] << 16) |
				    (s_data.dmy[75] << 8) |
				    s_data.dmy[76])) << 4;

				DevLog(DL_DETAIL(3213), tmp_val, 5,
				    s_data.dmy[74], s_data.dmy[75],
				    s_data.dmy[76], 0);
				if (tmp_val > UINT_MAX) {
					DevLog(DL_ERR(3114), tmp_val);
				}
				if (tmp_val <= un->capacity)
					return_val = un->capacity - tmp_val;

				DevLog(DL_DETAIL(3215), return_val,
				    un->capacity);

				/* if emd pf ,edoa */
				if (s_data.sense.es_eom) {
					return_val = 0;
					DevLog(DL_DETAIL(3115));
				}
			}
		}
		break;

	default:
		DevLog(DL_ERR(3116));
		SendCustMsg(HERE, 9344);
		return (0);
		/* NOTREACHED */
		break;
	}

	if (return_val < TAPE_IS_FULL)
		return (0);

	return (return_val);
}

/*
 * process_tape_labels - Process tape labels.
 *
 * On entry: fd - Open file descriptor un - pointer to device entry mutex and
 * io_mutex held
 */

void
process_tape_labels(int fd, dev_ent_t *un)
{
	int	len1, len2, len3;
	int	retrys = 2;
	char	*d_mess, *dc_mess, *msg_buf, *msg1;
	char    *more_message = " ";
	uint64_t	capacity;
	time_t	start;
	struct mtget	mt_status;
	mode_sense_t	mode_sense;
	vol1_label_t	vol1;
	hdr1_label_t	hdr1;
	hdr2_label_t	hdr2;
	union {
		tape_mode_sense_t *tape_ms;
		video_mode_sense_t *video_ms;
		dlt_mode_sense_t *dlt_ms;
		exab_mode_sense_t *exab_ms;
		stk_mode_sense_t *stk_ms;
		mode_sense_t   *msp;
	} msu;

	struct pg_codes {
		uchar_t	code;
		uchar_t	len;
	} *page_codes;


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
					DevLog(DL_ERR(3150));
					DownDevice(un, SAM_STATE_CHANGE);
					return;
				} else
					IO_table[indx].initialized = TRUE;
			}
			mutex_unlock(&IO_table[indx].mutex);
		}
		if (IO_table[indx].jmp_table.process_tape_labels) {
			IO_table[indx].jmp_table.process_tape_labels(fd, un);
			return;
		}
	}
	d_mess = un->dis_mes[DIS_MES_NORM];
	dc_mess = un->dis_mes[DIS_MES_CRIT];

	/* worm and volsafe media inspection */
	if (tape_properties(un, fd)) {
		return;
	}
	/* tell user about worm and volsafe media */
	if (un->dt.tp.properties & PROPERTY_WORM_MEDIA) {
		DevLog(DL_ALL(3262));
	} else if (un->dt.tp.properties & PROPERTY_VOLSAFE_MEDIA) {
		DevLog(DL_ALL(3263));
	}
	update_block_limits(un, fd);
	msu.msp = (mode_sense_t *)SHM_REF_ADDR(un->mode_sense);

	memset(msu.msp, 0, sizeof (mode_sense_t));
	if ((ioctl(fd, MTIOCGET, (char *)&mt_status) >= 0) &&
	    (mt_status.mt_type == MT_ISDEFAULT)) {
		/* Default type is an error - down the drive and return */
		DevLog(DL_ERR(3117));
		memccpy(dc_mess, catgets(catfd, SET, 872,
		    "Device is type default. Update '/kernel/drv/st.conf'."),
		    '\0', DIS_MES_LEN);
		sam_syslog(LOG_WARNING, catgets(catfd, SET, 2465,
		    "Tape device %d is default type. Update"
		    " '/kernel/drv/st.conf'."),
		    un->eq);
		TAPEALERT_MTS(fd, un, mt_status.mt_erreg);
		DownDevice(un, SAM_STATE_CHANGE);
		return;
	}
	/*
	 * Some tape devices do not support mode sense pages(metrum and
	 * exabyte 82xx).  Attempt a mode sense with the value in the pages
	 * entry of the device, if that fails and is not zero, set it to zero
	 * and try again.  The next time through, it should be set correctly.
	 */

	if ((len1 = scsi_cmd(fd, un, SCMD_MODE_SENSE, 0, &mode_sense,
	    sizeof (mode_sense_t), un->pages, (int *)NULL)) < 0) {
		if (un->pages) {
			un->pages = 0;
			len1 = scsi_cmd(fd, un, SCMD_MODE_SENSE, 0, &mode_sense,
			    sizeof (mode_sense_t), 0, (int *)NULL);
		}
	}
	if (len1 < 0) {
		sam_extended_sense_t *sense = (sam_extended_sense_t *)
		    SHM_REF_ADDR(un->sense);

		if (check_cleaning_error(un, fd, sense))
			un->status.b.cleaning = TRUE;
		DevLog(DL_ERR(3118));
		DevLogCdb(un);
		DevLogSense(un);
		TAPEALERT_SKEY(fd, un);
		un->status.bits |= DVST_SCAN_ERR;
	} else {
		unsigned int    len;
		void    *dummy;

		if (check_cleaning_error(un, fd, NULL))
			un->status.b.cleaning = TRUE;
		if (un->model == M_RSP2150)
			memcpy(msu.msp, &mode_sense,
			    sizeof (video_mode_sense_t));
		else
			memcpy(msu.msp, &mode_sense,
			    sizeof (generic_mode_sense_t));

		/* Find and copy specfic mode sense pages in the correct */
		page_codes = (struct pg_codes *)
		    ((char *)&mode_sense +
		    (mode_sense.u.generic_ms.hdr.blk_desc_len +
		    sizeof (generic_ms_hdr_t)));

		if (un->pages != 0) {	/* If mode sense pages asked for */
			while ((char *)page_codes < ((char *)&mode_sense +
			    mode_sense.u.generic_ms.hdr.sense_data_len)) {
				dummy = NULL;
				len = 0;

				switch (page_codes->code & 0x3f) {
				case 0x00:
					switch (un->model) {
					case M_STK4280:
					case M_STK9490:
					case M_STK9840:
					case M_STK9940:
					case M_STKTITAN:
					case M_STKD3:
						dummy = &msu.stk_ms->pg0;
						len = sizeof (tape_ms_page0_t);
						break;
					}
					break;

				case 0x01:
					switch (un->model) {
					case M_STK4280:
					case M_STK9490:
					case M_STK9840:
					case M_STK9940:
					case M_STKTITAN:
					case M_STKD3:
						dummy = &msu.stk_ms->pg1;
						len = sizeof (tape_ms_page1_t);
						break;

					case M_EXB8505:
					case M_EXBM2:
						dummy = &msu.exab_ms->pg1;
						len = sizeof (exab_ms_page1_t);
						break;

					case M_DLT2000:
					case M_DLT2700:
						dummy = &msu.dlt_ms->pg1;
						len = sizeof (dlt_ms_page1_t);
						break;
					}
					break;

				case 0x02:
					switch (un->model) {
					case M_EXB8505:
					case M_EXBM2:
						dummy = &msu.exab_ms->pg2;
						len = sizeof (exab_ms_page2_t);
						break;

					case M_DLT2000:
					case M_DLT2700:
						dummy = &msu.dlt_ms->pg2;
						len = sizeof (dlt_ms_page2_t);
						break;
					}
					break;

				case 0x0a:
					switch (un->model) {
					case M_EXB8505:
					case M_EXBM2:
						dummy = &msu.exab_ms->pga;
						len = sizeof (exab_ms_pagea_t);
						break;

					case M_DLT2000:
					case M_DLT2700:
						dummy = &msu.dlt_ms->pga;
						len = sizeof (dlt_ms_pagea_t);
						break;
					}
					break;

				case 0x0f:
					switch (un->model) {
					case M_EXB8505:
					case M_EXBM2:
						dummy = &msu.exab_ms->pga;
						len = sizeof (exab_ms_pagea_t);
						break;
					}
					break;

				case 0x10:
					len = sizeof (tape_ms_page10_t);
					switch (un->model) {
					case M_EXB8505:
					case M_EXBM2:
						dummy = &msu.exab_ms->pg10;
						break;

					case M_DLT2000:
					case M_DLT2700:
						dummy = &msu.dlt_ms->pg10;
						break;

					case M_STK4280:
					case M_STK9490:
					case M_STK9840:
					case M_STK9940:
					case M_STKTITAN:
					case M_STKD3:
						dummy = &msu.stk_ms->pg10;
						break;
					}
					break;

				case 0x11:
					switch (un->model) {
					case M_EXB8505:
					case M_EXBM2:
						dummy = &msu.exab_ms->pg11;
						len = sizeof (exab_ms_page11_t);
						break;

					case M_DLT2000:
					case M_DLT2700:
						dummy = &msu.dlt_ms->pg11;
						len = sizeof (dlt_ms_page11_t);
						break;
					}
					break;

				case 0x20:
					switch (un->model) {
					case M_EXB8505:
					case M_EXBM2:
						dummy = &msu.exab_ms->pg11;
						len = sizeof (exab_ms_page11_t);
						break;
					}
					break;

				case 0x21:
					switch (un->model) {
					case M_EXB8505:
					case M_EXBM2:
						dummy = &msu.exab_ms->pg11;
						len = sizeof (exab_ms_page11_t);
						break;
					}
					break;

				case 0x22:
					switch (un->model) {
					case M_EXB8505:
						dummy = &msu.exab_ms->pg11;
						len = sizeof (exab_ms_page11_t);
						break;
					}
					break;

				default:
					break;
				}

				if (dummy != NULL)
					memcpy(dummy, page_codes, len);

				page_codes = (struct pg_codes *)(
				    (char *)page_codes + (page_codes->len + 2));
			}
		}
		un->status.b.write_protect = msu.msp->u.generic_ms.hdr.WP;
		if (un->status.b.write_protect) {
			DevLog(DL_DETAIL(3119));
			more_message = catgets(catfd, SET, 323,
			    ": media is write protected");
		}
		if (un->equ_type == DT_VIDEO_TAPE)
			un->dt.tp.medium_type =
			    msu.video_ms->blk_desc.tape_length;
		else {
			un->dt.tp.medium_type =
			    msu.msp->u.generic_ms.hdr.medium_type;
#ifdef DISABLE_DTF2_CODE
			if (un->equ_type == DT_SONYDTF) {
				/*
				 * DTF-1 / DTF-2 discrimination kludge. DTF-1
				 * media in a DTF-2 drive can only be read.
				 * The problem is the DTF-2 drive does not
				 * set any of the standard write-protect
				 * bits.  The only way to tell what is going
				 * on is to check the cassette type (medium
				 * type).  The following trick works because
				 * of a bug in the DTF-1 drive.  That drive
				 * returns 0 for all cassette types. So....
				 * If we ever ever see a DTF-1 cassette type
				 * (0x82 or 0x83) we must be on a DTF-2 drive
				 * and the media should be write protected.
				 */
				if (un->dt.tp.medium_type == 0x82 ||
				    un->dt.tp.medium_type == 0x83) {
					un->status.b.write_protect = 1;
					DevLog(DL_DETAIL(3239));
					sam_syslog(LOG_INFO,
					    catgets(catfd, SET, 9325,
					    "Drive supports media as"
					    " read-only, setting write"
					    " protect"));
				}
			}
#endif				/* DISABLE_DTF2_CODE */
		}
	}

	/* Must hit this loop with un->mutex held */

	retrys = 3;
	do {
		int	resid;
		int	read_timer;

		sam_extended_sense_t *sense = (sam_extended_sense_t *)
		    SHM_REF_ADDR(un->sense);

		len1 = len2 = len3 = 0;
		un->dt.tp.stage_pos = 0;
		un->sector_size = 0;
		mutex_unlock(&un->mutex);
		DevLog(DL_DETAIL(3120));
		msg1 = catgets(catfd, SET, 1974, "Process labels: Rewinding%s");
		msg_buf = malloc_wait(strlen(msg1) +
		    strlen(more_message) + 4, 2, 0);
		sprintf(msg_buf, msg1, more_message);
		memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
		free(msg_buf);
		memset(sense, 0, sizeof (sam_extended_sense_t));
		(void) time(&start);
		if (scsi_cmd(fd, un, SCMD_REZERO_UNIT, 3600)) {
			sleep(2);
			(void) time(&start);
			if (scsi_cmd(fd, un, SCMD_REZERO_UNIT, 3600)) {
				mutex_lock(&un->mutex);
				un->status.b.scan_err = 1;
				mutex_unlock(&un->mutex);
				DevLog(DL_DETAIL(3121));
				SendCustMsg(HERE, 9337);
				DevLogCdb(un);
				DevLogSense(un);
				TAPEALERT_SKEY(fd, un);
				continue;
			}
		}
		DevLog(DL_TIME(3122), time(NULL) - start);

		/* cant be zero, just rewound it */
		if (!(capacity = read_tape_capacity(un, fd)))
			capacity = un->dt.tp.default_capacity;
		else {
			mutex_lock(&un->mutex);
			un->capacity = capacity;
			if (un->space > capacity) {
				un->space = capacity;
			}
			mutex_unlock(&un->mutex);
		}

		memset(&vol1, 0, 80);
		memset(&hdr1, 0, 80);
		memset(&hdr2, 0, 80);
		DevLog(DL_DETAIL(3123));
		msg1 = catgets(catfd, SET, 1973,
		    "Process labels: Read labels%s");
		msg_buf = malloc_wait(strlen(msg1) +
		    strlen(more_message) + 4, 2, 0);
		sprintf(msg_buf, msg1, more_message);
		memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
		free(msg_buf);

		/*
		 * AIT-3 requires more time to read the label. Time required
		 * has been between 1 minute 20 seconds to 1 minute 40
		 * seconds for labeled tapes and up to 4 minutes for virgin
		 * tapes so use 8 minutes just to be safe.
		 */
		if (un->equ_type == DT_SONYAIT || un->equ_type == DT_SONYSAIT)
			read_timer = 480;
		else
			read_timer = 60;

		len1 = scsi_cmd(fd, un, SCMD_READ, read_timer, &vol1, 80,
		    0, &resid);
		if (len1 > 0) {
			len2 = scsi_cmd(fd, un, SCMD_READ, 60, &hdr1,
			    80, 0, &resid);
			if (len2 > 0) {
				len3 = scsi_cmd(fd, un, SCMD_READ, 60, &hdr2,
				    80, 0, &resid);
			}
		}
		mutex_lock(&un->mutex);
		if (len1 <= 0 || len2 <= 0 || len3 <= 0) {
			sam_extended_sense_t *sense = (sam_extended_sense_t *)
			    SHM_REF_ADDR(un->sense);

			if (sense->es_key == KEY_BLANK_CHECK) {
				/*
				 * Nothing wrong with a blank tape. Log the
				 * fact for support (DL_ERR is used because
				 * that is in the default log setting,
				 * support always wants this logged).
				 */
				DevLog(DL_ERR(3245));
				break;
			}
			DevLogCdb(un);
			DevLogSense(un);
			TAPEALERT_SKEY(fd, un);
			memset(un->vsn, 0, sizeof (vsn_t));
			/*
			 * scan_err will be cleared outside of this "if" if
			 * one of the retries is successful. If we can't read
			 * the label, scan_er will cause the E flag to be set
			 * in UpdateCatalog.
			 */
			un->status.b.scan_err = 0;

			/* Check sony dtf needing recovery */
			if (un->equ_type == DT_SONYDTF &&
			    sense->es_key == KEY_MEDIUM_ERROR &&
			    sense->es_add_code == 0x90 &&
			    sense->es_qual_code == 0x00) {
				sprintf(d_mess, "Recover request: check log");
				syslog(LOG_WARNING, catgets(catfd, SET, 9327,
				    "SONY device %d has requested a"
				    " RECOVER operation."), un->eq);
				syslog(LOG_WARNING, catgets(catfd, SET, 9328,
				    "Data loss may result."));
				syslog(LOG_WARNING, catgets(catfd, SET, 9329,
				    "Contact hardware vendor on how to"
				    " proceed."));
				break;
			}
			if (sense->es_key == KEY_MEDIUM_ERROR ||
			    sense->es_key == KEY_HARDWARE_ERROR) {

				/*
				 * new version of micro code on the 3590 has
				 * a problem with the first thing after
				 * insertion
				 */
				if (un->equ_type == DT_3590 &&
				    sense->es_key == KEY_HARDWARE_ERROR &&
				    sense->es_add_code == 0x44)
					continue;

				un->status.bits |= DVST_SCAN_ERR;
				if ((un->type == DT_D3 || un->type == DT_9490 ||
				    un->type == DT_9840 ||
				    un->type == DT_9940 ||
				    un->type == DT_TITAN) &&
				    sense->es_key == KEY_MEDIUM_ERROR &&
				    sense->es_add_code == 0x30 &&
				    sense->es_qual_code == 0x01) {

					un->space = un->capacity = capacity;
					un->status.b.scan_err = 0;
					break;
				}
				/*
				 * Check for SONY DTF brand-new tape sense
				 * data
				 */
				if (un->type == DT_SONYDTF &&
				    sense->es_key == KEY_MEDIUM_ERROR &&
				    sense->es_add_code == 0x90 &&
				    sense->es_qual_code == 0x02) {

					un->space = un->capacity = capacity;
					un->status.b.scan_err = 0;
					break;
				}
				if (sense->es_key == KEY_MEDIUM_ERROR) {
					sam_syslog(LOG_WARNING,
					    catgets(catfd, SET, 15006,
					    "Media Error"
					    " : Cannot read label for"
					    " eq:slot:partition(%d.%d.%d)"),
					    un->eq, un->slot, un->i.ViPart);

					if (un->slot != ROBOT_NO_SLOT) {
						DevLog(DL_ERR(3199), un->slot);
					} else {
						DevLog(DL_ERR(3249));
					}

					un->status.bits |= DVST_BAD_MEDIA;
					if (check_cleaning_error(un, fd, sense))
						un->status.b.cleaning = TRUE;
					break;	/* break do..while */
				}
				if (sense->es_key == KEY_HARDWARE_ERROR) {
					DownDevice(un, SAM_STATE_CHANGE);
					break;
				}
			}
			continue;
		}
		un->status.b.scan_err = 0;

		/* Check for labels */
		if ((len1 == 80 && (memcmp(&vol1, "VOL1", 4) == 0)) &&
		    ((len2 == 80) && (memcmp(&hdr1, "HDR1", 4) == 0)) &&
		    ((len3 == 80) && (memcmp(&hdr2, "HDR2", 4) == 0)))
			un->status.b.labeled = TRUE;

		memset(un->vsn, 0, sizeof (vsn_t));	/* clean old vsn */
		un->label_time = (time_t)0;
		un->space = un->capacity;

		if (un->status.b.labeled) {	/* copy vsn to device entry */
			char	*c_tmp;
			char	blk_size[10], tim_scr[40];
			unsigned int	block_size = 0, pdu_shift;
			struct tm	local_time;

			/* remove  trailing spaces */
			for (c_tmp = &vol1.volume_serial_number[5];
			    c_tmp != &vol1.volume_serial_number[0] &&
			    *c_tmp == ' '; c_tmp--) {
					*c_tmp = '\0';
			}
			memcpy(&un->vsn, &vol1.volume_serial_number, 6);
			if (memcmp(&hdr2.lsc_uniq.reserved_for_os, "        ", 8)
			    != 0) {
				BE32toH(&hdr2.lsc_uniq.label_time,
				    &un->label_time);
				if (DBG_LVL(SAM_DBG_LBL)) {
					(void) ctime_r(&un->label_time, tim_scr,
					    40);
					tim_scr[24] = '\0';
					sam_syslog(LOG_DEBUG, "(%d:%s) LBL %s",
					    un->eq, un->vsn, tim_scr);
				}
			}
			/* get the block size from the hdr2 record */
			blk_size[8] = '\0';
			memcpy(&blk_size[0], hdr2.block_length_ext, 3);
			memcpy(&blk_size[3], hdr2.block_length, 5);
			block_size = atoi(blk_size);
			if (block_size < TAPE_SECTOR_SIZE ||
			    block_size > un->dt.tp.max_blocksize) {
				sam_syslog(LOG_WARNING,
				    catgets(catfd, SET, 1630,
				    "Media (%d:%s) bad block size %#x"
				    " setting to %d."), un->eq, un->vsn,
				    block_size, TAPE_SECTOR_SIZE);
				block_size = TAPE_SECTOR_SIZE;
			} else if (DBG_LVL(SAM_DBG_LBL)) {
				sam_syslog(LOG_DEBUG,
				    "Blk_size of (%d:%s) is %d.",
				    un->eq, un->vsn, block_size);
			}
			(void) ctime_r(&un->label_time, tim_scr, 40);
			tim_scr[24] = '\0';
			pdu_shift = 1;
			while (!(block_size & (1 << pdu_shift)) ||
			    pdu_shift > 30) {
				pdu_shift++;
			}
			if (block_size ^ (1 << pdu_shift)) {
				sam_syslog(LOG_WARNING,
				    catgets(catfd, SET, 2467,
				    "Tape on (%d:%s) bad block size(not"
				    " power of 2) %#x setting to %d."),
				    un->eq, un->vsn, block_size,
				    TAPE_SECTOR_SIZE);
				block_size = TAPE_SECTOR_SIZE;
			}
			msg1 = catgets(catfd, SET, 1486,
			    "Labeled %s, blocksize %d%s");
			msg_buf = malloc_wait(strlen(msg1) + strlen(tim_scr) +
			    strlen(more_message) + 12, 2, 0);
			sprintf(msg_buf, msg1, tim_scr, block_size,
			    more_message);
			memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
			free(msg_buf);
			un->sector_size = block_size;
			(void) localtime_r(&un->label_time, &local_time);
			(void) strftime(tim_scr, sizeof (tim_scr) - 1,
			    "%Y/%m/%d %H:%M:%S", &local_time);
			DevLog(DL_ALL(3003), un->vsn, tim_scr, block_size);
		}
	} while (!un->status.b.labeled && --retrys > 0);

	if (!un->status.b.labeled) {
		msg1 = catgets(catfd, SET, 2769, "Unlabeled %s");
		msg_buf = malloc_wait(strlen(msg1) + strlen(more_message) + 4,
		    2, 0);
		sprintf(msg_buf, msg1, more_message);
		memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
		free(msg_buf);
	}
}

/*
 * update_block_limits - get block limits from device.
 *
 * entry - un->mutex and un->io_mutex held
 */

void
update_block_limits(dev_ent_t *un, int open_fd)
{
	uchar_t	blklimits[6];

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
					DevLog(DL_ERR(3151));
					DownDevice(un, SAM_STATE_CHANGE);
					return;
				} else
					IO_table[indx].initialized = TRUE;
			}
			mutex_unlock(&IO_table[indx].mutex);
		}
		if (IO_table[indx].jmp_table.update_block_limits) {
			IO_table[indx].jmp_table.update_block_limits(
			    un, open_fd);
			return;
		}
	}
	/* get block limits for this device */
	if (IS_TAPE(un)) {
		if (scsi_cmd(open_fd, un, SCMD_READ_BLKLIM, 0, blklimits,
		    6, NULL) != 6) {
			DevLogCdb(un);
			DevLogSense(un);
			un->dt.tp.max_blocksize = un->dt.tp.default_blocksize;
			if ( errno == EACCES ){
				DevLog(DL_DETAIL(1171), getpid(), SCMD_READ_BLKLIM,
					un->name, open_fd) ;
				OffDevice(un, SAM_STATE_CHANGE);
				SendCustMsg(HERE, 9402);
				/* set drive to state off instead of down */
			} else {
				DevLog(DL_ERR(3124), un->dt.tp.max_blocksize);
				TAPEALERT_SKEY(open_fd, un);
			}
		} else {
			int i = 0;
			uint_t tmp, tmp1;

			tmp = ((blklimits[1] << 16) | (blklimits[2] << 8) |
			    blklimits[3]);
			if (tmp > MAX_TAPE_SECTOR_SIZE)
				tmp = MAX_TAPE_SECTOR_SIZE;

			tmp1 = tmp;
			for (tmp >>= 1; tmp != 0; tmp >>= 1)
				i++;
			un->dt.tp.max_blocksize = (1 << i);
			if (un->dt.tp.default_blocksize >
			    un->dt.tp.max_blocksize)
				un->dt.tp.default_blocksize =
				    un->dt.tp.max_blocksize;
			DevLog(DL_DETAIL(3125),
				    un->dt.tp.max_blocksize, tmp1);
		}
	}
}

int
get_9x40_space(dev_ent_t *un, int open_fd)
{
	int	return_val;
	int	resid;
	uint_t	position;
	uint_t	approx_use;
	uint_t	dmy[20];

	struct PAGE_CODE_HEADER {
		ushort_t	p_code;
		uchar_t
			DU:1,
			DS:1,
			TSD:1,
			ETC:1,
			TMC:2,
				:    1,
			LP:1;
		uchar_t p_len;
	};

	struct log_sense {
		uchar_t	page_code;
		uchar_t	res;
		ushort_t	page_len;
		struct PAGE_CODE_HEADER pch_space_left;
		uchar_t	space[4];
	}    *log_pages = (struct log_sense *)&dmy;


	/*
	 * Old way: Log this for now but don't use it unless the new way
	 * fails.
	 */

	if (read_position(un, open_fd, &position) < 0) {
		DevLog(DL_ERR(3109));
		SendCustMsg(HERE, 9341);
		TAPEALERT_SKEY(open_fd, un);
		return (0);
	}
	return_val = (long long) position *(long long) un->sector_size;
	approx_use = return_val / 1024;
	if (approx_use < un->dt.tp.default_capacity) {
		return_val = un->dt.tp.default_capacity - approx_use;
	} else {
		return_val = 0;
	}
	DevLog(DL_DETAIL(3246),
	    return_val, position, un->dt.tp.default_capacity);

	/*
	 * New way. Only works with 9840 firmware levels 1.24 and above.
	 */
	if (scsi_cmd(open_fd, un, SCMD_LOG_SENSE, 0, log_pages, 1, 0x0c, 0x8000,
		sizeof (struct log_sense), &resid) < 0) {

		DevLog(DL_ERR(3107));
		SendCustMsg(HERE, 9343);
		TAPEALERT_SKEY(open_fd, un);

	/* use the "old way" number, return_val */
	} else {
		return_val = (log_pages->space[0] << 24 |
			log_pages->space[1] << 16 |
			log_pages->space[2] << 8 | log_pages->space[3]) * 4;
		DevLog(DL_DETAIL(3247), return_val);
	}
	return (return_val);
}

int
load_tape_io_lib(tape_IO_entry_t *entries, tape_IO_t *table)
{
	int	i, err = 0;
	char	**lib_name = &entries->lib_name;
	char	**entry_name = lib_name + 1;
	size_t	*entry_pt = (size_t *)table;
	void	*api_handle;

	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "Loading %s.", *lib_name);

	if (strcmp(*lib_name, "libsamfs.so") == 0)
		lib_name = NULL;

	if ((api_handle = dlopen(*lib_name, RTLD_NOW | RTLD_GLOBAL)) == NULL) {
		sam_syslog(LOG_ERR, catgets(catfd, SET, 2483,
		    "The shared object library %s cannot be loaded: %s"),
		    entries->lib_name, dlerror());
		return (1);
	}
	for (i = 0; i < SAM_TAPE_N_ENTRIES; i++, entry_pt++, entry_name++) {
		if (strlen(*entry_name) == 0)
			continue;
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG, "Mapping symbol %s.",
			    *entry_name);
		*entry_pt = (size_t)dlsym(api_handle, *entry_name);
		if (*entry_pt == 0) {
			sam_syslog(LOG_ERR, catgets(catfd, SET, 1049,
			    "Error mapping symbol -%s-:%s."),
			    entry_name, dlerror());
			err = 1;
		}
	}
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "Loading of %s %s.", *lib_name,
		    err ? "failed" : "complete");

	return (err);
}

/*
 * get_media_type - is media eraseable or worm.
 */
uchar_t
get_media_type(dev_ent_t *un)
{
	uchar_t	media_type = MEDIA_RW;	/* regular or VolSafe media */

	if (IS_OPTICAL(un)) {
		/* current op media type */
		media_type = un->dt.od.medium_type;
	} else if (IS_TAPE(un) &&
	    (un->dt.tp.properties & PROPERTY_WORM_MEDIA)) {
		/* worm media processing */
		media_type = MEDIA_WORM;
	}
	return (media_type);
}

/*
 * tape_properties - query drive capablilites and media type.
 */
int
tape_properties(dev_ent_t *un, int fd)
{
#define			len 0xff
	uchar_t		buf[len];
	int		resid;
	sam_extended_sense_t *sense;
	uchar_t		cdb[SAM_CDB_LENGTH];
	uchar_t		worm_capable;
	uchar_t		worm_media;
	uchar_t		encryption_capable;
	properties_t	properties;

	switch (un->type) {
	case DT_9840:		/* STK VolSafe for T9840A, T9840B, T9840C */
	case DT_9940:		/* STK VolSafe for T9940B, ... */
	case DT_TITAN:		/* STK VolSafe and encryption for T1000A */

		/* Initialize drive feature. */
		properties = PROPERTY_VOLSAFE_DRIVE;

		/* Query drive for VolSafe capable. */
		memset(cdb, 0, SAM_CDB_LENGTH);
		cdb[0] = SCMD_INQUIRY;
		cdb[4] = len;	/* buffer length */

		sense = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);
		memset(sense, 0, sizeof (sam_extended_sense_t));

		if (scsi_cmd(fd, un, SCMD_ISSUE_CDB, 30, cdb, 6, buf, len,
		    USCSI_READ, &resid) != 0 || (len - resid) < 56) {
			DevLog(DL_DETAIL(3255));
			un->dt.tp.properties = properties;
			return (0);	/* ignore error */
		}
		worm_capable = (buf[55] & 0x4);	/* VolSafe capable bit */
		encryption_capable = 0;
		if (un->type == DT_TITAN) {
			encryption_capable = (buf[55] & 0x10);
		}
		/* Query drive for VolSafe media. */
		memset(sense, 0, sizeof (sam_extended_sense_t));
		if (scsi_cmd(fd, un, SCMD_REQUEST_SENSE, 0, buf,
		    len, &resid) < 0 || (len - resid) < 25) {
			DevLog(DL_DETAIL(3256));
			un->dt.tp.properties = properties;
			return (0);	/* ignore error */
		}
		worm_media = (buf[24] & 0x2);	/* VolSafe media bit */

		/* Record drive and media type. */
		if (worm_capable) {
			properties |= PROPERTY_VOLSAFE_CAPABLE;	/* volsafe on */
		}
		if (worm_media) {
			properties |= PROPERTY_VOLSAFE_MEDIA;
		}
		if (encryption_capable) {
			properties |= PROPERTY_ENCRYPTION_DRIVE;
		}
		un->dt.tp.properties = properties;
		break;

	case DT_SONYSAIT:	/* Sony SAIT WORM */

		/* Drive may support worm feature. */
		properties = PROPERTY_WORM_DRIVE;

		/* Query drive for WORM capable and media. */
		memset(cdb, 0, SAM_CDB_LENGTH);
		cdb[0] = SCMD_MODE_SENSE;
		cdb[1] = 0x8;	/* disable block descriptors */
		cdb[2] = 0x31;	/* page 0x31 */
		cdb[4] = len;	/* buffer length */

		sense = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);
		memset(sense, 0, sizeof (sam_extended_sense_t));

		if (scsi_cmd(fd, un, SCMD_ISSUE_CDB, 30, cdb, 6, buf, len,
		    USCSI_READ, &resid) != 0 || (len - resid) < 10 ||
		    buf[0] < 9 || buf[3] != 0 || buf[4] != 0x31 ||
		    buf[5] < 4) {

			DevLog(DL_ERR(3257));
			DevLogCdb(un);
			DevLogSense(un);
			SendCustMsg(HERE, 9358);
			TAPEALERT(fd, un);
			DownDevice(un, SAM_STATE_CHANGE);
			memccpy(un->dis_mes[DIS_MES_CRIT],
			    catgets(catfd, SET, 2978,
			    "Unable to determine if WORM or RW media"
			    " loaded."), '\0', DIS_MES_LEN);
			un->dt.tp.properties = properties;
			return (-1);	/* error */
		}
		worm_capable = (buf[9] & 0x01);	/* WORM capable bit */
		worm_media = (buf[8] & 0x40);	/* WORM media bit */

		/* Record drive and media type. */
		if (worm_capable) {
			properties |= PROPERTY_WORM_CAPABLE;	/* worm fw */
		}
		if (worm_media) {
			properties |= PROPERTY_WORM_MEDIA;
		}
		un->dt.tp.properties = properties;
		break;

	case DT_IBM3580:	/* ALL LTOs */

		/* HP LTO-2, LTO-3 and LTO-4 WORM */
		if (memcmp(un->vendor_id, "HP      ", 8) == 0 &&
		    memcmp(un->product_id, "Ultrium 1-SCSI  ", 16) != 0) {

			/* Drive may support worm feature. */
			properties = PROPERTY_WORM_DRIVE;

			sense = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);

			/* Query drive for WORM capable. */
			if (scsi_cmd(fd, un, SCMD_INQUIRY, 0, buf, len, 0, 0,
			    NULL, NULL) < 41) {

				DevLog(DL_ERR(3257));
				DevLogCdb(un);
				DevLogSense(un);
				SendCustMsg(HERE, 9358);
				TAPEALERT(fd, un);
				DownDevice(un, SAM_STATE_CHANGE);
				memccpy(un->dis_mes[DIS_MES_CRIT],
				    catgets(catfd, SET, 2978,
				    "Unable to determine if WORM or RW"
				    " media loaded."),
				    '\0', DIS_MES_LEN);
				un->dt.tp.properties = properties;
				return (-1);	/* error */
			}
			worm_capable = (buf[40] & 0x1);	/* WORM capable bit */
			/* Query drive for WORM media. */
			if (scsi_cmd(fd, un, SCMD_MODE_SENSE, 30, buf, 4,
			    0, NULL) < 2) {
				DevLog(DL_ERR(3257));
				DevLogCdb(un);
				DevLogSense(un);
				SendCustMsg(HERE, 9358);
				TAPEALERT(fd, un);
				DownDevice(un, SAM_STATE_CHANGE);
				memccpy(un->dis_mes[DIS_MES_CRIT],
				    catgets(catfd, SET, 2978,
				    "Unable to determine if WORM or RW"
				    " media loaded."),
				    '\0', DIS_MES_LEN);
				un->dt.tp.properties = properties;
				return (-1);	/* error */
			}
			switch (buf[1]) {	/* WORM media byte */
			case 0x00:
				worm_media = FALSE;	/* read write media */
				break;
			case 0x01:
				worm_media = TRUE;	/* worm media */
				break;
			case 0x80:
				DevLog(DL_ERR(3268));	/* cdrom media */
				SendCustMsg(HERE, 9401);
				DownDevice(un, SAM_STATE_CHANGE);
				memccpy(un->dis_mes[DIS_MES_CRIT],
				    catgets(catfd, SET, 2982,
				    "Drive is in CD-ROM emulation mode."),
				    '\0', DIS_MES_LEN);
				un->dt.tp.properties = properties;
				return (-1);	/* error */
			default:
				DevLog(DL_ERR(3257));
				SendCustMsg(HERE, 9358);
				DownDevice(un, SAM_STATE_CHANGE);
				memccpy(un->dis_mes[DIS_MES_CRIT],
				    catgets(catfd, SET, 2978,
				    "Unable to determine if WORM or RW"
				    " media loaded."), '\0', DIS_MES_LEN);
				un->dt.tp.properties = properties;
				return (-1);	/* error */
			}

			/* Record drive and media type. */
			if (worm_capable) {
				/* volsafe on */
				properties |= PROPERTY_WORM_CAPABLE;
			}
			if (worm_media) {
				properties |= PROPERTY_WORM_MEDIA;
			}
			un->dt.tp.properties = properties;
		}
		if (memcmp(un->vendor_id, "IBM     ", 8) == 0 &&
		    (memcmp(un->product_id, "ULTRIUM-TD3     ", 16) == 0 ||
		    memcmp(un->product_id, "ULT3580-TD3     ", 16) == 0 ||
		    memcmp(un->product_id, "ULTRIUM-TD4     ", 16) == 0 ||
		    memcmp(un->product_id, "ULT3580-TD4     ", 16) == 0 ||
		    memcmp(un->product_id, "ULTRIUM-TD5     ", 16) == 0 ||
		    memcmp(un->product_id, "ULT3580-TD5     ", 16) == 0 ||
		    memcmp(un->product_id, "ULTRIUM-TD6     ", 16) == 0 ||
		    memcmp(un->product_id, "ULT3580-TD6     ", 16) == 0 ||
		    memcmp(un->product_id, "ULTRIUM-TD7     ", 16) == 0 ||
		    memcmp(un->product_id, "ULT3580-TD7     ", 16) == 0 ||
		    memcmp(un->product_id, "ULTRIUM-TD8     ", 16) == 0 ||
		    memcmp(un->product_id, "ULT3580-TD8     ", 16) == 0 ||
		    memcmp(un->product_id, "ULTRIUM-TD9     ", 16) == 0 ||
		    memcmp(un->product_id, "ULT3580-TD9     ", 16) == 0)) {

			/* Checking if the drive supports WORM feature. */

			properties = PROPERTY_WORM_DRIVE;

			sense = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);

			/* Query drive for WORM capable */

			if (scsi_cmd(fd, un, SCMD_INQUIRY, 1, buf, len, 1, 0xb0,
			    NULL, NULL) < 2) {
				if ( errno == EACCES ){
					DevLog(DL_DETAIL(1171), getpid(), SCMD_INQUIRY,
					un->name, fd) ;
					OffDevice(un, SAM_STATE_CHANGE);
					SendCustMsg(HERE, 9402);
					/* set drive to state off */
				} else {
					DevLog(DL_ERR(3257));
					SendCustMsg(HERE, 9358);
					TAPEALERT(fd, un);
					memccpy(un->dis_mes[DIS_MES_CRIT],
						catgets(catfd, SET, 2978,
						"Unable to determine if WORM or RW"
						" media loaded."),
						'\0', DIS_MES_LEN);
				}
				DevLogCdb(un);
				DevLogSense(un);
				un->dt.tp.properties = properties;
				return (-1);	/* error */
			}
			worm_capable = buf[4];


			/* Query drive for WORM media. */

			if (scsi_cmd(fd, un, SCMD_MODE_SENSE, 1, buf, len, 0x3f,
			    &resid) < 83) {
				if ( errno == EACCES ){
					DevLog(DL_DETAIL(1171), getpid(), SCMD_INQUIRY,
					un->name, fd) ;
					OffDevice(un, SAM_STATE_CHANGE);
					SendCustMsg(HERE, 9402);
					/* set drive to state off */
				} else {
					DevLog(DL_ERR(3257));
					SendCustMsg(HERE, 9358);
					TAPEALERT(fd, un);
					memccpy(un->dis_mes[DIS_MES_CRIT],
						catgets(catfd, SET, 2978,
						"Unable to determine if WORM or RW"
						" media loaded."),
						'\0', DIS_MES_LEN);
				}
				DevLogCdb(un);
				DevLogSense(un);
				un->dt.tp.properties = properties;
				return (-1);	/* error */
			}
			switch (buf[1]) {	/* WORM media byte */
			case 0x00:
				worm_media = FALSE;
				break;
			case 0x18:
				worm_media = FALSE;	/* Ultrium 1 data */
				break;
			case 0x28:
				worm_media = FALSE;	/* Ultrium 2 data */
				break;
			case 0x38:
				worm_media = FALSE;	/* Ultrium 3 data */
				break;
			case 0x3c:
				worm_media = TRUE;	/* Ultrium 3 WORM */
				break;
			case 0x48:
				worm_media = FALSE;	/* Ultrium 4 data */
				break;
			case 0x4c:
				worm_media = TRUE;	/* Ultrium 4 WORM */
				break;
			case 0x58:
				worm_media = FALSE;	/* Ultrium 5 data */
				break;
			case 0x5c:
				worm_media = TRUE;	/* Ultrium 5 WORM */
				break;
			case 0x68:
				worm_media = FALSE;	/* Ultrium 6 data */
				break;
			case 0x6c:
				worm_media = TRUE;	/* Ultrium 6 WORM */
				break;
			case 0x78:
				worm_media = FALSE;	/* Ultrium 7 data */
				break;
			case 0x7c:
				worm_media = TRUE;	/* Ultrium 7 WORM */
				break;
			case 0x88:
				worm_media = FALSE;	/* Ultrium 8 data */
				break;
			case 0x8c:
				worm_media = TRUE;	/* Ultrium 8 WORM */
				break;
			case 0x98:
				worm_media = FALSE;	/* Ultrium 9 data */
				break;
			case 0x9c:
				worm_media = TRUE;	/* Ultrium 9 WORM */
				break;
			default:
				DevLog(DL_ERR(3257));
				SendCustMsg(HERE, 9358);
				DownDevice(un, SAM_STATE_CHANGE);
				memccpy(un->dis_mes[DIS_MES_CRIT],
				    catgets(catfd, SET, 2978,
				    "Unable to determine if WORM or RW"
				    " media loaded."), '\0', DIS_MES_LEN);
				un->dt.tp.properties = properties;
				return (-1);	/* error */
			}

			/* Record drive and media type */
			if (worm_capable) {
				properties |= PROPERTY_WORM_CAPABLE;
			}
			if (worm_media) {
				properties |= PROPERTY_WORM_MEDIA;
			}
			un->dt.tp.properties = properties;
		}
		break;

	}

	return (0);
}

/* sizeof = 20
0 struct one_com_des {
    0 uchar_t reserved0 
    1 unsigned char:3 support :3 
    1.3 unsigned char:4 reserved1 :4 
    1.7 unsigned char:1 ctdp :1 
    2 ushort_t cdb_size 
    4 uchar_t [16] usage 
}

sizeof = 12
struct com_timeout_des {
    0 uchar_t descripter length msb
    1 uchar_t descriptor length lsb
    2 uchar_t reserved
    3 uchar_t command specific
    4 uint64_t nominal command processing timeout
    8 uint64_t recommended command timeout
}
*/

int
drive_timeout(dev_ent_t *un, int fd, boolean_t do_lock)
{
#define ONE_COMMAND_NO_SERVICE_DATA_FORMAT 1
#define CTDP_INCL 0x80
#define RSOC_SUPPORTED 3
	const size_t buflen = 22;
	uchar_t cdb[CDB_GROUP5];
	uchar_t buf[buflen];
	int resid;
	int ret;

	if (un->load_timeout != LTO_TUR_TIMEOUT)
		return un->load_timeout;
	memset(cdb, 0, CDB_GROUP5);
	memset(buf, 0, buflen);
        cdb[0] = (uchar_t) SCMD_MAINTENANCE_IN;
        cdb[1] = SSVC_ACTION_GET_SUPPORTED_OPERATIONS;
        cdb[2] = (uchar_t) (ONE_COMMAND_NO_SERVICE_DATA_FORMAT | 0x80); /* RCTD */
        cdb[3] = SCMD_LOAD;
        cdb[9] = (uchar_t) buflen & 0xff; 

	if ((ret=scsi_cmd(fd, un, SCMD_ISSUE_CDB, 60, cdb, CDB_GROUP5, buf, buflen,
	    USCSI_READ, &resid)) != 0) {
		DevLog(DL_ERR(3289), strerror(errno), ret, resid);
		DevLogCdb(un);
		DevLogSense(un);
		DevLog(DL_DETAIL(3288), 0, LTO_TUR_TIMEOUT*10, 0, 0, 0, 0);
		return LTO_TUR_TIMEOUT*10;
	}
	int cdb_len = (buf[2]<<8) + buf[3] +4;
	if ((uchar_t)buf[1] != (CTDP_INCL|RSOC_SUPPORTED)){
		DevLog(DL_DETAIL(3290));
		DevLog(DL_DETAIL(3288), buf[1], LTO_TUR_TIMEOUT, 
		    buf[cdb_len+8], buf[cdb_len+9], buf[cdb_len+10], buf[cdb_len+11]);
		return LTO_TUR_TIMEOUT;
	}
	/* cdb_len buf[2]<<8 + buf[3], descriptor_len = buf[4 + cdb_len]  */
	int timeout = (buf[cdb_len+10]<<8) + buf[cdb_len+11];
	DevLog(DL_DETAIL(3288), buf[1] & 7, timeout,
	    buf[cdb_len+8], buf[cdb_len+9], buf[cdb_len+10], buf[cdb_len+11]);

	if(do_lock)
		mutex_lock(&un->mutex);
	un->load_timeout = timeout;
	if(do_lock)
		mutex_unlock(&un->mutex);

	return timeout;
}
/*
 * tapeclean - determine if tape drive cleaning is needed.
 */
void
tapeclean(dev_ent_t *un, int fd)
{
	int	i;
	int	j;
	/*
	 * page length can be 0x244 for Sony Super AIT
	 */
#define			PAGE_LEN 0x244
	uchar_t	page[PAGE_LEN];
	int	resid;
	int	required = 0;
	int	requested = 0;
	int	expired = 0;
	int	page_len;
	int	param_code;
	int	param_len;
	int	cleaning;


	if ((cleaning = check_cleaning_error(un, fd, NULL))) {
		un->status.b.cleaning = TRUE;
	}
	/* auto-clean feature */
	if ((un->tapeclean & TAPECLEAN_AUTOCLEAN) == 0) {
		/* safely toggle cleaning bit via common request sense */
		if (!cleaning) {
			if (un->status.b.cleaning == TRUE) {
				DevLog(DL_DETAIL(3283));
				un->status.b.cleaning = FALSE;
			}
		}
		return;
	}
	if ((un->tapeclean & TAPECLEAN_LOGSENSE) == 0) {
		return;
	}
	/* look at extended log sense for dirty drive */
	if (memcmp(un->vendor_id, "QUANTUM", 7) == 0 &&
	    memcmp(un->product_id, "SDLT320", 7) == 0) {
		if (scsi_cmd(fd, un, SCMD_LOG_SENSE, 0, page, 1,
		    0x3e, 0, PAGE_LEN, &resid) < 0 || (PAGE_LEN - resid) < 4 ||
		    page[0] != 0x3e) {
			return;
		}
		page_len = (page[2] << 8) | page[3];

		for (i = 4; i < page_len + 4 && i < page_len;
		    i += param_len + 4) {

			param_code = (page[i] << 8) | page[i + 1];
			param_len = page[i + 3];

			if (param_code == 1) {
				required = page[i + 4] & 0x4;
				requested = page[i + 4] & 0x2;
				expired = page[i + 4] & 0x1;
				break;
			}
		}
	} else if (un->equ_type == DT_SONYAIT || un->equ_type == DT_SONYSAIT) {
		if (scsi_cmd(fd, un, SCMD_LOG_SENSE, 0, page, 1,
		    0x33, 0, PAGE_LEN, &resid) < 0 || (PAGE_LEN - resid) < 4 ||
		    page[0] != 0x33) {
			return;
		}
		page_len = (page[2] << 8) | page[3];

		for (i = 4; i < page_len + 4 && i < PAGE_LEN;
		    i += param_len + 4) {

			param_code = (page[i] << 8) | page[i + 1];
			param_len = page[i + 3];

			if ((un->equ_type == DT_SONYAIT && param_code == 5) ||
			    (un->equ_type == DT_SONYSAIT &&
			    param_code == 0xb)) {
				required = page[i + 4] & 0x80;
				break;
			}
		}
	} else {
		if (scsi_cmd(fd, un, SCMD_LOG_SENSE, 0, page, 1,
		    0xc, 0, PAGE_LEN, &resid) < 0 || (PAGE_LEN - resid) < 4 ||
		    page[0] != 0xc) {
			return;
		}
		page_len = (page[2] << 8) | page[3];

		for (i = 4; i < page_len + 4 && i < page_len;
		    i += param_len + 4) {

			param_code = (page[i] << 8) | page[i + 1];
			param_len = page[i + 3];

			if (param_code == 0x100) {
				for (j = 0; j < param_len; j++) {
					required |= page[i + 4 + j];
				}
				break;
			}
		}
	}

	tapeclean_active(un, required, requested, expired, 0);

	/* finally look at TapeAlert */
	TAPEALERT(fd, un);
}

/*
 * tapeclean_inspect - open the drive and search for a dirty drive or expired
 * cleaning media.
 */
static void
tapeclean_inspect(dev_ent_t *un, tapeclean_t option)
{
	int		open_fd;
	boolean_t	unlock_needed = B_FALSE;
	uint32_t	opt;


	/* Only tape drives. */
	if (un->scsi_type != 1) {
		return;
	}
	/* Local file descriptor open. */
	if ((open_fd = open(un->name, O_RDONLY | O_NONBLOCK)) < 0) {
		if (IS_TAPE(un)) {
			char	*open_name;
			if ((open_fd = open((open_name = samst_devname(un)),
			    O_RDONLY | O_NONBLOCK)) < 0) {
				if (open_name != (char *)un->dt.tp.samst_name)
					free(open_name);
				return;
			} else {
				INC_OPEN(un);
				if (open_name != (char *)un->dt.tp.samst_name)
					free(open_name);
			}
		} else {
			DevLog(DL_DEBUG(12014));
			return;
		}
	}
	if (mutex_trylock(&un->io_mutex) == 0) {
		unlock_needed = B_TRUE;
	}
	opt = (uint32_t)option;
	un->tapeclean |= opt;
	tapeclean(un, open_fd);
	un->tapeclean &= ~opt;

	if (unlock_needed == B_TRUE) {
		mutex_unlock(&un->io_mutex);
	}
	(void) close(open_fd);
	DEC_OPEN(un);
}

/*
 * tapeclean_media - during a drive cleaning cycle, is the cleaning media
 * expired?
 */
void
tapeclean_media(dev_ent_t *un)
{
	tapeclean_inspect(un, TAPECLEAN_MEDIA);
}

/*
 * tapeclean_drive - after a drive cleaning cycle, is the drive dirty?
 */
int
tapeclean_drive(dev_ent_t *un)
{
	tapeclean_inspect(un, TAPECLEAN_DRIVE);
	return (un->status.b.cleaning);
}

/*
 * tapeclean_active - set active cleaning flags.
 */
void
tapeclean_active(
	dev_ent_t *un,
	int required,
	int requested,
	int expired,
	int invalid)
{
	if (!IS_TAPE(un))
		return;

	if (required == 0 && requested == 0 && expired == 0 && invalid == 0) {
		return;
	}
	switch (un->tapeclean & TAPECLEAN_MEDIA) {
	case 0:
		/* read write media loaded */
		if ((required || requested) && un->status.b.cleaning == FALSE) {
			if (required) {
				DevLog(DL_ALL(3277));
			} else if (requested) {
				DevLog(DL_ALL(3278));
			}
			un->status.b.cleaning = TRUE;
		}
		break;

	case TAPECLEAN_MEDIA:
		/* cleaning media loaded */
		if ((expired || invalid) && un->status.b.bad_media == FALSE) {
			if (expired) {
				DevLog(DL_ALL(3279));
			} else {
				DevLog(DL_ALL(3282));
			}
			un->status.b.bad_media = TRUE;
		}
		break;
	}
}
