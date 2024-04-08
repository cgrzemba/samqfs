/*
 * gen_labels.c - generate labels for optic and tape
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

#pragma ident "$Revision: 1.39 $"

static char    *_SrcFile = __FILE__;

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mtio.h>
#include <syslog.h>

#include "sam/types.h"
#include "sam/custmsg.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/device.h"
#include "aml/dev_log.h"
#include "sam/lib.h"
#include "aml/tapes.h"
#include "aml/shm.h"
#include "aml/external_data.h"
#include "aml/odlabels.h"
#include "aml/tplabels.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"
#include "sam/lint.h"
#include "aml/tapealert.h"
#include "aml/robots.h"

/* globals */


extern shm_alloc_t master_shm, preview_shm;

/* Function prototypes */

void init_vol_label(dkpri_label_t *, char *, char *);
void init_par_label(dkpart_label_t *, int, char *);
void ansi_time(dklabel_timestamp_t *);
int  upd_vol_label(dkpri_label_t *, dkpri_label_t *);
int  upd_part_label(dkpart_label_t *, dkpart_label_t *);
int  do_tur(dev_ent_t *un, int open_fd, int retry);


/*
 * wt_labels - thread routine to write labels.
 *
 * Parameter is a pointer to a *_label_req struct. This struct has pointers to
 * all elements.  All of these pointers and the struct itself must have been
 * malloc'ed by the caller before calling wt_labels.  wt_labels will free all
 * memory before returning. Unused parameters can be NULL pointers.  NOTE:
 * wt_labels cannot be used to label devices in a robot.
 */

void *
wt_labels(void *param)
{
	int			eq, open_fd, o_flags;
	int			exit_status = -1;
	char			*d_mess;
	char			*open_name;
	struct CatalogEntry	ced;
	struct CatalogEntry	*ce = &ced;
	dev_ent_t		*un;
	dev_ptr_tbl_t		*dev_ptr_tbl;
	message_request_t	*mes_req = (message_request_t *)param;
	label_request_t		*req;
	label_req_t		request;

	SANITY_CHECK(mes_req != 0);

	if (mes_req == NULL)
		goto thread_exit;

	req = &mes_req->message.param.label_request;

	request.eq = req->slot;
	request.block_size = req->block_size;
	request.slot = 0;
	request.flags = req->flags;
	request.vsn = strlen(req->vsn) ? req->vsn : NULL;
	request.info = strlen(req->info) ? req->info : NULL;


	/* set up pointer to the device table */
	dev_ptr_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->dev_table);

	SANITY_CHECK(request.vsn != (char *)0);

	if (request.vsn == (char *)NULL)
		goto thread_exit;

	eq = request.eq;

	if (dev_ptr_tbl->d_ent[eq] == 0)
		goto thread_exit;

	un = (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->d_ent[eq]);

	SANITY_CHECK(un != (dev_ent_t *)0);

	d_mess = un->dis_mes[DIS_MES_NORM];
	if (un->fseq) {
		/* device is in a robot */
		char	*msg_buf, *msg1;

		msg1 = catgets(catfd, SET, 454,
		    "Attempt to label robotic device (%d).");
		msg_buf = malloc_wait(strlen(msg1) + 12, 2, 0);
		(void) sprintf(msg_buf, msg1, un->eq);
		DevLog(DL_ERR(1036));
		write_client_exit_string(&mes_req->message.exit_id,
		    EXIT_FAILED, msg_buf);
		free(msg_buf);

		goto thread_exit;
	}
	mutex_lock(&un->mutex);

	if (un->active != 0 || (IS_TAPE(un) && un->open_count)) {
		char	*msg_buf, *msg1;

		msg1 = catgets(catfd, SET, 452,
		    "Attempt to label active device (%d).");
		msg_buf = malloc_wait(strlen(msg1) + 12, 2, 0);
		(void) sprintf(msg_buf, msg1, un->eq);
		mutex_unlock(&un->mutex);
		DevLog(DL_ERR(1037));
		write_client_exit_string(&mes_req->message.exit_id,
		    EXIT_FAILED, msg_buf);
		free(msg_buf);
		goto thread_exit;
	}
	INC_ACTIVE(un);

	if (!((un->status.bits & DVST_READY) &&
	    (un->status.bits & DVST_PRESENT)) ||
	    (un->status.bits & (DVST_READ_ONLY | DVST_WRITE_PROTECT))) {
		char	*msg_buf, *msg1;

		msg1 = catgets(catfd, SET, 1820,
		    "Not ready or read-only media in device (%d).");
		msg_buf = malloc_wait(strlen(msg1) + 12, 2, 0);
		(void) sprintf(msg_buf, msg1, eq);
		(void) memccpy(d_mess, catgets(catfd, SET, 596,
		    "cannot label read only media"),
		    '\0', DIS_MES_LEN);
		DEC_ACTIVE(un);
		mutex_unlock(&un->mutex);
		DevLog(DL_ERR(1038));
		write_client_exit_string(&mes_req->message.exit_id,
		    EXIT_FAILED, msg_buf);
		free(msg_buf);
		goto thread_exit;
	}
	if ((un->status.bits & DVST_LABELED) &&
	    !(request.flags & LABEL_RELABEL)) {
		char	*msg_buf, *msg1;

		msg1 = catgets(catfd, SET, 1632,
		    "Media already labeled with %s.");
		msg_buf = malloc_wait(strlen(msg1) +
		    strlen(un->vsn) + 4, 2, 0);
		(void) sprintf(msg_buf, msg1, un->vsn);
		(void) memccpy(d_mess,
		    catgets(catfd, SET, 1631, "media already labeled"),
		    '\0', DIS_MES_LEN);
		DEC_ACTIVE(un);
		mutex_unlock(&un->mutex);
		DevLog(DL_ERR(1039));
		write_client_exit_string(&mes_req->message.exit_id,
		    EXIT_FAILED, msg_buf);
		free(msg_buf);
		goto thread_exit;
	}
	if ((un->status.bits & DVST_LABELED) &&
	    (request.flags & LABEL_RELABEL) &&
	    (get_media_type(un) == MEDIA_WORM)) {
		char	*msg1;

		msg1 = GetCustMsg(9305);
		(void) memccpy(d_mess, msg1, '\0', DIS_MES_LEN);
		SendCustMsg(HERE, 9305);
		DEC_ACTIVE(un);
		mutex_unlock(&un->mutex);
		DevLog(DL_ERR(2098));
		write_client_exit_string(&mes_req->message.exit_id,
		    EXIT_FAILED, msg1);
		goto thread_exit;
	}
	memmove(&un->i.ViMtype, sam_mediatoa(un->type), sizeof (un->i.ViMtype));
	memmove(un->i.ViVsn, un->vsn, sizeof (un->i.ViVsn));
	if (CatalogLabelVolume(&un->i, request.vsn) == -1) {
		char	buf[STR_FROM_ERRNO_BUF_SIZE];

		snprintf(d_mess, DIS_MES_LEN, catgets(catfd, SET, 9323,
		    "Cannot label: %s"),
		    StrFromErrno(errno, buf, sizeof (buf)));
		DEC_ACTIVE(un);
		mutex_unlock(&un->mutex);
		DevLog(DL_ERR(2101), buf);
		write_client_exit_string(&mes_req->message.exit_id,
		    EXIT_FAILED, d_mess);
		goto thread_exit;
	}
	o_flags = (O_RDWR | O_NDELAY);
	open_name = un->name;

	SANITY_CHECK(open_name != (char *)0);
	SANITY_CHECK(open_name[0] != (char)0);

	if ((open_fd = open(open_name, o_flags)) < 0) {
		DevLog(DL_SYSERR(3026), open_name);
		DEC_ACTIVE(un);
		mutex_unlock(&un->mutex);
		write_client_exit_string(&mes_req->message.exit_id,
		    EXIT_FAILED, catgets(catfd, SET, 2671,
		    "Unable to open to write labels."));
		goto thread_exit;
	}
	un->status.bits &= ~DVST_READY;
	INC_OPEN(un);

	mutex_unlock(&un->mutex);
	mutex_lock(&un->io_mutex);

	switch (un->type & DT_CLASS_MASK) {
	case DT_OPTICAL:
		DevLog(DL_ALL(2047), request.vsn);
		exit_status = write_labels(open_fd, un, &request);
		break;

	case DT_TAPE:
		DevLog(DL_ALL(3069), request.vsn);
		exit_status = write_tape_labels(&open_fd, un, &request);
		break;

	default:
		DevLog(DL_ERR(3027), un->type);
		exit_status = -1;
		break;
	}

	if (exit_status) {
		sam_syslog(LOG_ERR,
		    catgets(catfd, SET, 133,
		    "%s(%d): Label of media fails [Old VSN %s, New VSN %s]."),
		    "wt_labels", eq, un->vsn, request.vsn);
	}
	mutex_lock(&un->mutex);
	DEC_OPEN(un);
	DEC_ACTIVE(un);
	un->status.bits |= DVST_READY | DVST_SCANNING;
	if (exit_status) {
		if (exit_status != VOLSAFE_LABEL_ERROR) {
			un->status.bits |= DVST_BAD_MEDIA;
		}
		CatalogLabelFailed(&un->i, request.vsn);
	}
	mutex_unlock(&un->mutex);
	mutex_unlock(&un->io_mutex);

	/*
	 * Temporary "solution" to prevent core dumps until the partition is
	 * passed down the fifo correctly.
	 */
	ce = CatalogGetCeByMedia(un->i.ViMtype, un->i.ViVsn, &ced);
	if (ce != NULL)
		un->i.ViPart = ce->CePart;
	else
		un->i.ViPart = 0;

	scan_a_device(un, open_fd);
	(void) close(open_fd);
	if (!exit_status) {
		UpdateCatalog(un, 0, CatalogLabelComplete);
	}

thread_exit:

	/* ensure a response was sent */
	write_client_exit(&mes_req->message.exit_id, exit_status);

	if (mes_req != NULL)
		free(mes_req);

	thr_exit(&exit_status);
	/* NOTREACHED */
	/* return ((void *) NULL); */
}

/*
 * write_tape_labels io_mutex held on entry.
 * Returns:
 *  0 - Success
 *  1 - Attempt was made to relable volsafe media
 * -1 - Any other label failure
 */

int
write_tape_labels(
	int *open_fd,	/* open file descriptor */
	dev_ent_t *un,	/* device pointer */
	label_req_t *request)
{
	int		flags = 0, ii;
	int		wt_cnt = 0;
	uint64_t	tmp_capacity;
	char		ansi_date[7], scratch[20];
	char		*d_mess;
	char		leading;
	char		*msg_buf, *msg1;
	vsn_t		old_vsn;
	time_t		now;
	struct tm	tm_now;
	struct mtop	mt_oper;
	sam_time_t	sam_time;
	vol1_label_t	vol1;
	hdr1_label_t	hdr1;
	hdr2_label_t	hdr2;
	sam_extended_sense_t	*sense;

	SANITY_CHECK(open_fd != (int *)0);
	SANITY_CHECK(un != (dev_ent_t *)0);
	SANITY_CHECK(request != (label_req_t *)0);

	sense = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);
	d_mess = un->dis_mes[DIS_MES_NORM];

	if (IS_TAPE(un) && un->dt.tp.drive_index != SAM_TAPE_DVR_DEFAULT) {
		register int    indx = un->dt.tp.drive_index;
		if (!(IO_table[indx].initialized)) {
			mutex_lock(&IO_table[indx].mutex);
			if (!(IO_table[indx].initialized)) {
				if (load_tape_io_lib(&tape_IO_entries[indx],
				    &(IO_table[indx].jmp_table))) {
					(void) memccpy(un->
					    dis_mes[DIS_MES_CRIT],
					    catgets(catfd, SET, 2162,
					    "Runtime interface not found"),
					    '\0', DIS_MES_LEN);
					DevLog(DL_ERR(3139));
					DownDevice(un, SAM_STATE_CHANGE);
					return (-1);
				} else
					IO_table[indx].initialized = TRUE;
			}
			mutex_unlock(&IO_table[indx].mutex);
		}
		if (IO_table[indx].jmp_table.write_tape_labels)
			return (IO_table[indx].jmp_table.
			    write_tape_labels(open_fd, un, request));
	}
	if (un->status.bits & DVST_LABELED) {
		(void) memcpy(&old_vsn, un->vsn, sizeof (vsn_t));
		mutex_lock(&un->mutex);
		un->status.bits &= ~DVST_LABELED;

		mutex_unlock(&un->mutex);
	} else
		(void) memset(&old_vsn, 0, sizeof (vsn_t));

	(void) memset(un->vsn, 0, sizeof (vsn_t));
	flags = request->flags;

	(void) memset(&vol1, ' ', 80);
	(void) memset(&hdr1, ' ', 80);
	(void) memset(&hdr2, ' ', 80);

	/* Build vol1 */
	(void) memcpy(&vol1.label_identifier, "VOL1", 4);
	(void) sprintf(scratch, "%-*s",
	    sizeof (vol1.volume_serial_number), request->vsn);
	(void) memcpy(&vol1.volume_serial_number[0], scratch, 6);
	vol1.label_standard_version[0] = '4';
	(void) sprintf(scratch, "SAM-FS 2.0");
	(void) memcpy(&vol1.implementation_identifier[0], scratch,
	    strlen(scratch));

	/* Build hdr1 */
	(void) memcpy(&hdr1.label_identifier, "HDR1", 4);
	(void) memcpy(&hdr1.system_code[0], scratch, strlen(scratch));
	if (request->info != (char *)NULL) {
		(void) sprintf(scratch, "%-*s", sizeof (hdr1.file_identifier),
		    request->info);
		(void) memcpy(&hdr1.file_identifier[0], scratch, 17);
	}
	(void) memcpy(&hdr1.file_section_number[0], "0001", 4);
	(void) memcpy(&hdr1.file_sequence_number[0], "0001", 4);
	(void) memcpy(&hdr1.generation_number[0], "0001", 4);
	(void) memcpy(&hdr1.generation_version_number[0], "00", 2);
	(void) time(&now);
	(void) localtime_r(&now, &tm_now);

	/*
	 * ansi spec calls for a leading blank if the year is < 2000, else a
	 * leading '0' character.  localtime_r returns the year - 1900. The
	 * localtime_r function has returned us a day-of-the-year with jan 1
	 * being 0; ansi calls for 001 for that day, so we must add 1 to that
	 * field.
	 */
	if (tm_now.tm_year >= 100)
		leading = '0';
	else
		leading = ' ';
	sprintf(ansi_date, "%c%02d%03d", leading,
	    tm_now.tm_year % 100, tm_now.tm_yday + 1);
	(void) memcpy(&hdr1.creation_date, &ansi_date, 6);

	/* Build hdr2 */
	(void) memcpy(&hdr2.label_identifier, "HDR2", 4);
	if ((request->block_size == 0) && (un->dt.tp.default_blocksize != 0))
		request->block_size = un->dt.tp.default_blocksize;
	else if (request->block_size < TAPE_SECTOR_SIZE)
		request->block_size = TAPE_SECTOR_SIZE;
	else if (request->block_size > un->dt.tp.max_blocksize)
		request->block_size = un->dt.tp.max_blocksize;

	SANITY_CHECK(request->block_size != 0);

	(void) memset(scratch, ' ', 10);
	(void) sprintf(scratch, "%8d", request->block_size);
	(void) memcpy(&hdr2.block_length[0], &scratch[3], 5);
	(void) memcpy(&hdr2.block_length_ext[0], &scratch[0], 3);
	(void) sprintf(scratch, "%*d", sizeof (hdr2.record_length), 1);
	(void) memcpy(&hdr2.record_length[0], &scratch, 5);
	sam_time = (sam_time_t)time(NULL);
	HtoBE32(&sam_time, &hdr2.lsc_uniq.label_time);

	DevLog(DL_DETAIL(3015));
	(void) memset(sense, 0, sizeof (sam_extended_sense_t));
	if (scsi_cmd(*open_fd, un, SCMD_REZERO_UNIT, 120) < 0 &&
	    scsi_cmd(*open_fd, un, SCMD_REZERO_UNIT, 120) < 0) {
		DevLog(DL_ERR(3016), old_vsn, request->vsn);
		SendCustMsg(HERE, 9338, old_vsn, request->vsn);
		DevLogCdb(un);
		DevLogSense(un);
		TAPEALERT_SKEY(*open_fd, un);
		return (-1);
	}
	if (IS_IBM_COMPATIBLE(un->equ_type))
		assert_compression(*open_fd, un);

	memset(sense, 0, sizeof (sam_extended_sense_t));

	if (flags & LABEL_ERASE) {
		char	tim_scr[40];
		time_t	now;
		int	timeout = 5;

		time(&now);
		ctime_r(&now, tim_scr, 40);
		tim_scr[24] = '\0';
		msg1 = catgets(catfd, SET, 2938,
		    "write label: erase started %s, %d hour time limit");
		msg_buf = malloc_wait(strlen(msg1) +
		    strlen(tim_scr) + 8, 2, 0);
		sprintf(msg_buf, msg1, tim_scr, timeout);
		memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
		free(msg_buf);
		DevLog(DL_DETAIL(3004));
		un->status.bits |= DVST_LABELLING;

		/* Allow 5 hours to erase tape */
		if (scsi_cmd(*open_fd, un, SCMD_ERASE,
		    timeout * 60 * 60, 1) < 0 || sense->es_key != 0)
			goto erase_error;

		(void) memset(sense, 0, sizeof (sam_extended_sense_t));
		(void) memccpy(d_mess,
		    catgets(catfd, SET, 2936, "write label - rewinding"),
		    '\0', DIS_MES_LEN);
		if (scsi_cmd(*open_fd, un, SCMD_REZERO_UNIT, 120) < 0 ||
		    sense->es_key != 0) {
			DevLog(DL_ERR(3019), old_vsn, request->vsn);
			SendCustMsg(HERE, 9338, old_vsn, request->vsn);
			DevLogCdb(un);
			DevLogSense(un);
			TAPEALERT_SKEY(*open_fd, un);
			un->status.bits &= ~DVST_LABELLING;
			return (-1);
		}
	}
	/*
	 * Always short erase unlabeled sony dtfs to force a format of new
	 * tapes
	 */
	else if (un->equ_type == DT_SONYDTF && old_vsn[0] == '\0') {
		char	tim_scr[40];
		time_t	now;
		int	timeout = 1;

		time(&now);
		ctime_r(&now, tim_scr, 40);
		tim_scr[24] = '\0';
		msg1 = catgets(catfd, SET, 2939,
		    "write label: format started %s, %d hour time limit");
		msg_buf = malloc_wait(strlen(msg1) + strlen(tim_scr) +
		    8, 2, 0);
		sprintf(msg_buf, msg1, tim_scr, timeout);
		memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
		free(msg_buf);
		un->status.bits |= DVST_LABELLING;

		if (scsi_cmd(*open_fd, un, SCMD_ERASE,
		    timeout * 60 * 60, 0) < 0 || sense->es_key != 0)
			goto erase_error;

		memset(sense, 0, sizeof (sam_extended_sense_t));
		memccpy(d_mess, catgets(catfd, SET, 2040,
		    "write label: format succeeded: rewinding"),
		    '\0', DIS_MES_LEN);
		if (scsi_cmd(*open_fd, un, SCMD_REZERO_UNIT, 120) < 0 ||
		    sense->es_key != 0) {

			DevLog(DL_ERR(3019), old_vsn, request->vsn);
			SendCustMsg(HERE, 9338, old_vsn, request->vsn);
			DevLogCdb(un);
			DevLogSense(un);
			TAPEALERT_SKEY(*open_fd, un);
			un->status.bits &= ~DVST_LABELLING;
			return (-1);
		}
	}
	for (ii = 2; ; ii--) {
		int	local_ret;
                int tmo = INITIAL_TUR_TIMEOUT;

                if (un->equ_type == DT_IBM3580) {
                        tmo = drive_timeout(un, *open_fd, B_TRUE);
                }

		local_ret = do_tur(un, *open_fd, tmo);
		if (local_ret == 0)
			break;

		if (TUR_ERROR(local_ret) || (ii <= 0)) {
			DevLog(DL_ERR(3083), local_ret, old_vsn, request->vsn);
			return (-1);
		} else if ((local_ret == WAIT_READY) ||
		    (local_ret == WAIT_READY_LONG) ||
		    (local_ret == LONG_WAIT_LOG)) {
			sleep((local_ret == WAIT_READY) ? WAIT_TIME_FOR_READY
			    : (local_ret == WAIT_READY_LONG) ?
			    WAIT_TIME_FOR_READY_LONG :
			    WAIT_TIME_FOR_LONG_WAIT_LOG);
			continue;
		}
		/*
		 * For now if we get a tur failure but no sense key, just log
		 * it
		 * We'll catch any REAL problems on the write ???
		 */
		else {
			DevLog(DL_DETAIL(3083), local_ret, old_vsn,
			    request->vsn);
			break;
		}
	}

	/*
	 * The space and capacity will change and be valid in the code below.
	 * Set the bits now so they don't need to be set in all cases.
	 */
	mutex_lock(&un->mutex);
	un->status.bits |= DVST_LABELLING;
	un->status.bits &= ~DVST_STOR_FULL;
	mutex_unlock(&un->mutex);

	/*
	 * Some drives, notably the stks, have a problem doing a write after
	 * a read on a virgin tape.  The driver seems to remember that there
	 * was a media error.  A close-open sequence clears this condition.
	 */
	(void) close(*open_fd);
	if ((*open_fd = open(un->name, O_RDWR)) < 0) {
		(void) memccpy(d_mess, catgets(catfd, SET, 2680,
		    "unable to reopen for labels"), '\0', DIS_MES_LEN);
		DevLog(DL_SYSERR(3020), un->name, old_vsn, request->vsn);
		return (-1);
	}
	DevLog(DL_ALL(3021));
	(void) memccpy(d_mess, catgets(catfd, SET, 2959, "writing labels"),
	    '\0', DIS_MES_LEN);
	wt_cnt = 0;
	DevLog(DL_DETAIL(3022));
	memccpy(d_mess, catgets(catfd, SET, 2962, "writing labels: vol1"),
	    '\0', DIS_MES_LEN);

	if (write(*open_fd, &vol1, 80) == 80)
		wt_cnt++;

	DevLog(DL_DETAIL(3023));
	memccpy(d_mess, catgets(catfd, SET, 2960, "writing labels: hdr1"),
	    '\0', DIS_MES_LEN);
	if (wt_cnt == 1)	/* First label ok */
		if (write(*open_fd, &hdr1, 80) == 80)
			wt_cnt++;

	DevLog(DL_DETAIL(3024));
	memccpy(d_mess, catgets(catfd, SET, 2961, "writing labels: hdr2"),
	    '\0', DIS_MES_LEN);
	if (wt_cnt == 2)	/* second label ok */
		if (write(*open_fd, &hdr2, 80) == 80)
			wt_cnt++;

	if (wt_cnt != 3) {	/* not all label blocks written */
		struct mtget	mt_s;

		DevLog(DL_ERR(3005), wt_cnt);

		if (ioctl(*open_fd, MTIOCGET, (char *)&mt_s) < 0) {
			DevLog(DL_SYSERR(3006), old_vsn, request->vsn);
		} else {
			DevLog(DL_ERR(3007), mt_s.mt_dsreg, mt_s.mt_erreg,
			    mt_s.mt_resid, mt_s.mt_fileno, mt_s.mt_blkno,
			    mt_s.mt_flags);
		}

		if (mt_s.mt_erreg == KEY_WRITE_PROTECT &&
		    (un->dt.tp.properties & PROPERTY_VOLSAFE) ==
		    PROPERTY_VOLSAFE) {

			DevLog(DL_ALL(3264));
			SendCustMsg(HERE, 9359);
			TAPEALERT(*open_fd, un);
			un->dt.tp.properties |= PROPERTY_VOLSAFE_PERM_LABEL;
			mutex_lock(&un->mutex);
			un->status.bits &= ~DVST_LABELLING;
			mutex_unlock(&un->mutex);
			return (VOLSAFE_LABEL_ERROR);
		}
		DevLog(DL_ERR(3009));
		SendCustMsg(HERE, 9335);
		TAPEALERT_MTS(*open_fd, un, mt_s.mt_erreg);
		mutex_lock(&un->mutex);
		un->space = un->capacity = 0;
		un->status.bits &= ~DVST_LABELLING;
		mutex_unlock(&un->mutex);
		return (-1);
	}
	/* label blocks written successfully */
	if (un->type == DT_VIDEO_TAPE) {
		int	i;
		char	*buffer;

		mt_oper.mt_op = MTWEOF;		/* write file mark */
		mt_oper.mt_count = 1;
		if (ioctl(*open_fd, MTIOCTOP, &mt_oper)) {
			DevLog(DL_SYSERR(3010));
			SendCustMsg(HERE, 9337);
			TAPEALERT(*open_fd, un);
		}
		buffer = (char *)malloc_wait((16 * 1024), 5, 0);
		strcpy(buffer, "Take up some space");
		for (i = 0; i < (10 * 1024 * 1024); i += (16 * 1024))
			write(*open_fd, buffer, (16 * 1024));
		free(buffer);
	}
	/*
	 * The historian needs the vsn, so set it in there over the call to
	 * create_tape_eof.
	 */
	memccpy(un->vsn, request->vsn, '\0', sizeof (vsn_t));
	if (create_tape_eof(open_fd, un, NULL)) {
		DevLog(DL_ERR(3011), old_vsn, request->vsn);
		SendCustMsg(HERE, 9336, old_vsn, request->vsn);
		mutex_lock(&un->mutex);
		memset(un->vsn, 0, sizeof (vsn_t));
		un->space = un->capacity = 0;
		un->status.bits &= ~DVST_LABELLING;
		mutex_unlock(&un->mutex);
		return (-1);
	}
	memset(un->vsn, 0, sizeof (vsn_t));

	mt_oper.mt_op = MTREW;	/* rewind the unit */
	mt_oper.mt_count = 0;
	if (*open_fd >= 0 && ioctl(*open_fd, MTIOCTOP, &mt_oper)) {
		DevLog(DL_SYSERR(3025));
		SendCustMsg(HERE, 9337);
		TAPEALERT(*open_fd, un);
	}
	/*
	 * If labeling on a different drive density(ie: media was originally
	 * on a DLT 4000 and was just relabeled on a DLT7000) the capacity
	 * must be captured now since the drive will switch densities just
	 * before the write operation. Also, new DLT tapes have a capacity of
	 * zero before writing.
	 */
	tmp_capacity = read_tape_capacity(un, *open_fd);

	mutex_lock(&un->mutex);
	un->space = un->capacity = tmp_capacity;
	un->status.bits = (un->status.bits & DVST_CLEANING) | DVST_PRESENT;
	mutex_unlock(&un->mutex);
	return (0);

	/*
	 * The short or long erase failed, or did not complete in the time
	 * allowed. Report an error, attempt to reset the drive, and exit.
	 */
erase_error:
	un->status.bits &= ~DVST_LABELLING;
	DevLog(DL_ERR(3017));
	SendCustMsg(HERE, 9339);
	DevLogCdb(un);
	DevLogSense(un);

	TAPEALERT_SKEY(*open_fd, un);

	memset(sense, 0, sizeof (sam_extended_sense_t));
	if (scsi_reset(*open_fd, un) < 0 || sense->es_key != 0) {
		DevLog(DL_DETAIL(3200));
		DevLogCdb(un);
		DevLogSense(un);
		TAPEALERT_SKEY(*open_fd, un);
		return (-1);
	}
	/* clear unit attention due to reset */
	memset(sense, 0, sizeof (sam_extended_sense_t));
	if (scsi_cmd(*open_fd, un, SCMD_REZERO_UNIT, 120) < 0 ||
	    sense->es_key != 0) {
		DevLog(DL_DETAIL(3201));
		DevLogCdb(un);
		DevLogSense(un);
		TAPEALERT_SKEY(*open_fd, un);
	}
	return (-1);
}

int
format_tape(int *open_fd, dev_ent_t *un, format_req_t *request)
{

	if (IS_TAPE(un) && un->dt.tp.drive_index != SAM_TAPE_DVR_DEFAULT) {
		register int    indx = un->dt.tp.drive_index;
		if (!(IO_table[indx].initialized)) {
			mutex_lock(&IO_table[indx].mutex);
			if (!(IO_table[indx].initialized)) {
				if (load_tape_io_lib(&tape_IO_entries[indx],
				    &(IO_table[indx].jmp_table))) {
					(void) memccpy(
					    un->dis_mes[DIS_MES_CRIT],
					    catgets(catfd, SET, 2162,
					    "Runtime interface not found."),
					    '\0', DIS_MES_LEN);
					DevLog(DL_ERR(3139));
					DownDevice(un, SAM_STATE_CHANGE);
					return (-1);
				} else
					IO_table[indx].initialized = TRUE;
			}
			mutex_unlock(&IO_table[indx].mutex);
		}
		if (IO_table[indx].jmp_table.format_tape)
			return (IO_table[indx].
			    jmp_table.format_tape(open_fd, un, request));
	}
	return (-1);
}

int
get_n_partitions(dev_ent_t *un, int open_fd)
{

	mutex_unlock(&un->mutex);
	if (IS_TAPE(un) && un->dt.tp.drive_index != SAM_TAPE_DVR_DEFAULT) {
		register int    indx = un->dt.tp.drive_index;
		if (!(IO_table[indx].initialized)) {
			mutex_lock(&IO_table[indx].mutex);
			if (!(IO_table[indx].initialized)) {
				if (load_tape_io_lib(&tape_IO_entries[indx],
				    &(IO_table[indx].jmp_table))) {
					(void) memccpy(un->
					    dis_mes[DIS_MES_CRIT],
					    catgets(catfd, SET, 2162,
					    "Runtime interface not found."),
					    '\0', DIS_MES_LEN);
					DevLog(DL_ERR(3139));
					DownDevice(un, SAM_STATE_CHANGE);
					return (-1);
				} else
					IO_table[indx].initialized = TRUE;
			}
			mutex_unlock(&IO_table[indx].mutex);
		}
		if (IO_table[indx].jmp_table.format_tape)
			return (IO_table[indx].
			    jmp_table.get_n_partitions(un, open_fd));
	}
	return (1);
}


/*
 * write_labels - write optic labels. io_mutex held on entry
 */

int
write_labels(int open_fd, dev_ent_t *un, label_req_t *request)
{
	int	flags = 0;
	int	exit_status = -1;
	int	relabel = FALSE;
	vsn_t	old_vsn;
	struct {		/* New Label buffer */
		dkpri_label_t	vol;	/* VOL1 label buffer */
		dkpart_label_t	par;	/* PAR1 label buffer */
	} new_label;

	struct {		/* Old Label buffer */
		dkpri_label_t	vol;	/* VOL1 label buffer */
		dkpart_label_t	par;	/* PAR1 label buffer */
	} old_label;

	SANITY_CHECK(un != (dev_ent_t *)0);
	SANITY_CHECK(request != (label_req_t *)0);

	flags = request->flags;
	if (un->status.bits & DVST_LABELED) {
		relabel = TRUE;
		(void) memcpy(&old_vsn, un->vsn, sizeof (vsn_t));
		mutex_lock(&un->mutex);
		un->status.bits &= ~DVST_LABELED;
		mutex_unlock(&un->mutex);
	} else
		(void) memset(&old_vsn, 0, sizeof (vsn_t));

	/*
	 * At this point we do not know if this medium is truly labeled or if
	 * we are just supporting the label lie. Thus, we "pretend" to
	 * relabel WORM media.
	 */
	/* if relabeling and it is WORM, read the old labels */
	if (relabel && get_media_type(un) == MEDIA_WORM) {
		int	len;
		char	*buffer;

		/* allocated a buffer for reading the labels */
		len = un->sector_size * 2;
		buffer = (char *)malloc_wait(len, 5, 0);

		DevLog(DL_LABEL(2060), 1, un->label_address);
		if (scsi_cmd(open_fd, un, READ, 0, buffer, len,
		    un->label_address, (int *)NULL) != len) {

			DevLog(DL_ERR(2037));
			free(buffer);	/* clean up */
			/*
			 * return error status so place holding label in
			 * catalog is deleted.
			 */
			exit_status = -1;
			TAPEALERT_SKEY(open_fd, un);
			goto exit_label;
		}
		(void) memcpy(&old_label.vol, buffer, sizeof (dkpri_label_t));
		(void) memcpy(&old_label.par, buffer + un->sector_size,
		    sizeof (dkpart_label_t));
		free(buffer);	/* clean up */
	}

	/* Build the new labels */
	init_vol_label(&new_label.vol, request->vsn, request->info);
	init_par_label(&new_label.par, GET_TOTAL_SECTORS(un), request->info);

	/* if relabeling and worm then increment label count */
	if (relabel && get_media_type(un) == MEDIA_WORM) {
		if (upd_vol_label(&old_label.vol, &new_label.vol) ||
		    upd_part_label(&old_label.par, &new_label.par)) {

			DevLog(DL_ERR(2038));
			goto exit_label;
		}
	}
	un->status.bits |= DVST_LABELLING;
	if (!(exit_status = blank_label_optic(open_fd, un, &new_label.vol,
	    &new_label.par, flags, relabel))) {

		mutex_lock(&un->mutex);
		un->status.bits = DVST_PRESENT;
		mutex_unlock(&un->mutex);
	} else {
		DevLog(DL_ERR(2039));
		SendCustMsg(HERE, 9335);
		exit_status = -1;
	}
exit_label:

	un->status.bits &= ~DVST_LABELLING;
	return (exit_status);
}

void
init_vol_label(dkpri_label_t *label, char *vsn, char *uinfo)
{
	uint16_t	v16;
	uint32_t	v32;
	int		i;

	SANITY_CHECK(label != (dkpri_label_t *)0);
	SANITY_CHECK(vsn != (char *)0);

	(void) memset(label, 0, sizeof (dkpri_label_t));

	v16 = PRIMARY_VOL_DES;
	HtoLE16(&v16, &label->descriptor_tag.identifier);
	v16 = 1;
	HtoLE16(&v16, &label->descriptor_tag.version);
	v32 = 1;
	HtoLE32(&v32, &label->descriptor_sequence_number);

	i = strlen(vsn);
	(void) strncpy((char *)label->volume_id, vsn, 32);
	label->volume_id[31] = (i < 33) ? i : 32;

	v16 = 3;
	HtoLE16(&v16, &label->interchange_level);
	HtoLE16(&v16, &label->max_interchange_level);

	v32 = 0xff;
	HtoLE32(&v32, &label->character_set_list);
	HtoLE32(&v32, &label->max_character_set_list);

	label->descriptor_char_set.type = CSET_4;
	label->explanatory_char_set.type = CSET_4;

	ansi_time(&label->recording_date_time);
	if (uinfo != (char *)NULL)
		(void) strncpy((char *)label->implementation_use, uinfo, 64);
}

void
init_par_label(dkpart_label_t *label, int capacity, char *uinfo)
{
	uint16_t	v16;
	uint32_t	v32;

	SANITY_CHECK(label != (dkpart_label_t *)0);

	(void) memset(label, 0, sizeof (dkpart_label_t));
	v16 = PARTITION_DES;
	HtoLE16(&v16, &label->descriptor_tag.identifier);
	v16 = 1;
	HtoLE16(&v16, &label->descriptor_tag.version);
	v32 = 1;
	HtoLE32(&v32, &label->descriptor_sequence_number);
	HtoLE16(&v16, &label->flags);

	(void) strcpy((char *)label->partition_contents.identifier, "*LSC01");

	v32 = 2;		/* WORM media */
	HtoLE32(&v32, &label->access_type);
	v32 = 257;		/* Starting location */
	HtoLE32(&v32, &label->starting_location);
	v32 = capacity - 514;	/* Length */
	HtoLE32(&v32, &label->length);

	if (uinfo != (char *)NULL)
		(void) strncpy((char *)label->implementation_use, uinfo, 128);
}

void
ansi_time(dklabel_timestamp_t *pl)
{
	time_t		clock;
	struct tm	tm;
	ushort_t	my_zone, my_year;
	extern time_t	timezone;

	SANITY_CHECK(pl != (dklabel_timestamp_t *)0);

	clock = time((time_t *)NULL);
	(void) localtime_r(&clock, &tm);
	my_zone = timezone / 60;
	my_year = tm.tm_year + 1900;

	HtoLE16(&my_zone, &pl->timezone);
	HtoLE16(&my_year, &pl->year);

	pl->month = tm.tm_mon + 1;
	pl->day = tm.tm_mday;
	pl->hour = tm.tm_hour;
	pl->minute = tm.tm_min;
	pl->second = tm.tm_sec;
}


int
upd_vol_label(dkpri_label_t *old, dkpri_label_t *new)
{
	uint32_t	seq_num;

	SANITY_CHECK(old != (dkpri_label_t *)0);
	SANITY_CHECK(new != (dkpri_label_t *)0);

	if (vfyansi_label(old, PRIMARY_VOL_DES) != 0)
		return (1);

	LE32toH(&old->descriptor_sequence_number, &seq_num);
	seq_num++;
	HtoLE32(&seq_num, &new->descriptor_sequence_number);
	return (0);
}


int
upd_part_label(dkpart_label_t *old, dkpart_label_t *new)
{
	uint32_t	seq_num;

	SANITY_CHECK(old != (dkpart_label_t *)0);
	SANITY_CHECK(new != (dkpart_label_t *)0);

	if (vfyansi_label(old, PARTITION_DES) != 0)
		return (1);

	LE32toH(&old->descriptor_sequence_number, &seq_num);
	seq_num++;
	HtoLE32(&seq_num, &new->descriptor_sequence_number);
	return (0);
}
