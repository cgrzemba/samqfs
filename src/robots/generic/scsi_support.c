/*
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

#pragma ident "$Revision: 1.39 $"

static char *_SrcFile = __FILE__;

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/mman.h>
#include <sys/scsi/generic/sense.h>

#include "driver/samst_def.h"
#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/dev_log.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "sam/nl_samfs.h"
#include "generic.h"
#include "aml/proto.h"
#include "aml/tapealert.h"

/* Function prototypes */
static int rotate_plasmon_mailbox(library_t *library, const int direction);

/*	Globals */
extern shm_alloc_t master_shm, preview_shm;
extern pid_t mypid;


/*
 *	rotate_mailbox - place mailbox in the desired position.
 *
 *	entry -
 *		 mutex for library->un->mutex(locked)
 */
int
rotate_mailbox(
	library_t *library,
	const int direction) 	/* ROTATE_IN or ROTATE_OUT */
{
	dev_ent_t 	*un;
	int 		err, retry;
	iport_state_t 	*in, *out, *tmp;
	int 		added_more_time = FALSE;
	sam_extended_sense_t *sense = (sam_extended_sense_t *)
	    SHM_REF_ADDR(library->un->sense);

	un = library->un;
	mutex_lock(&library->un->io_mutex);
	switch (library->un->type) {
	case DT_ADIC448:		/* Do not lock these doors */
		/* FALLTHROUGH */
	case DT_ADIC100:
		/* FALLTHROUGH */
	case DT_ADIC1000:
		/* FALLTHROUGH */
	case DT_STK97XX:
		/* FALLTHROUGH */
	case DT_DLT2700:
		/* FALLTHROUGH */
	case DT_3570C:
		/* FALLTHROUGH */
	case DT_SONYDMS:
		/* FALLTHROUGH */
	case DT_GRAUACI:
		/* FALLTHROUGH */
	case DT_ATLP3000:
		/* FALLTHROUGH */
	case DT_EXBX80:
		/* FALLTHROUGH */
	case DT_STKLXX:
		/* FALLTHROUGH */
	case DT_IBM3584:
		/* FALLTHROUGH */
	case DT_HPSLXX:
		/* FALLTHROUGH */
	case DT_FJNMXX:
		/* FALLTHROUGH */
	case DT_SL3000:
		/* FALLTHROUGH */
	case DT_SLPYTHON:
		mutex_unlock(&library->un->io_mutex);
		return (0);

	case DT_HPLIBS:
		/* FALLTHROUGH */
	case DT_PLASMON_D:
	case DT_PLASMON_G:
		rotate_plasmon_mailbox(library, direction);
		break;
	case DT_METD28:
		/* FALLTHROUGH */
	case DT_METD360:
		/* FALLTHROUGH */
	case DT_ACL452:
		/* FALLTHROUGH */
	case DT_ACL2640:
		/* FALLTHROUGH */
	case DT_DOCSTOR:
		/* FALLTHROUGH */
	case DT_EXB210:
		/* FALLTHROUGH */
	case DT_HP_C7200:
		/* FALLTHROUGH */
	case DT_QUAL82xx:
		/* FALLTHROUGH */
	case DT_SPECLOG:
		/* FALLTHROUGH */
	case DT_ATL1500:
		/* FALLTHROUGH */
	case DT_SONYCSM:
	/* FALLTHROUGH */
	case DT_QUANTUMC4:
		/* FALLTHROUGH */
	case DT_ODI_NEO:
		/* FALLTHROUGH */
		{
			retry = 3;
			do {
				TAPEALERT(library->open_fd, library->un);
				memset(sense, 0, sizeof (sam_extended_sense_t));
				if (scsi_cmd(library->open_fd, library->un,
				    SCMD_DOORLOCK, 0, (direction == ROTATE_OUT)
				    ? UNLOCK : LOCK)) {
					TAPEALERT_SKEY(library->open_fd,
					    library->un);
					GENERIC_SCSI_ERROR_PROCESSING(
					    library->un,
					    library->scsi_err_tab, 0,
					    err, added_more_time, retry,
					/* code for DOWN_EQU */
					    down_library(library,
					    SAM_STATE_CHANGE);
						mutex_unlock(&
						    library->un->io_mutex);
						return (-1);
						/* MACRO for cstyle */,
						/* code for ILLREQ */
						    mutex_unlock(&
						    library->un->io_mutex);
						return (-1);
						/* MACRO for cstyle */,
						/* More codes */
						    /* MACRO for cstyle */;
					/* MACRO for cstyle */)
				} else {
					err = 0;
					break;
				}
			} while (--retry > 0);
			TAPEALERT(library->open_fd, library->un);
			if (retry <= 0) {
				/* Log and go on */
				DevLog(DL_ERR(5202));
			}
		}
		break;

	case DT_CYGNET:
		in = out = NULL;
		for (tmp = library->import; tmp != NULL; tmp = tmp->next) {
			if (tmp->status.b.inenab) {
				in = tmp;
				continue;
			} else if (tmp->status.b.exenab) {
				out = tmp;
				continue;
			}
		}
		if ((in == NULL) || (out == NULL)) {
			err = 1;
			break;
		}
		retry = 3;
		do {
			TAPEALERT(library->open_fd, library->un);
			memset(sense, 0, sizeof (sam_extended_sense_t));
			if (direction == ROTATE_OUT)
				err = scsi_cmd(library->open_fd, library->un,
				    SCMD_MOVE_MEDIUM, 0,
				    0, in->element, out->element, 0);
			else
				err = scsi_cmd(library->open_fd, library->un,
				    SCMD_MOVE_MEDIUM, 0,
				    0, out->element, in->element, 0);
			TAPEALERT(library->open_fd, library->un);
			if (err) {
				GENERIC_SCSI_ERROR_PROCESSING(library->un,
				    library->scsi_err_tab, 0,
				    err, added_more_time, retry,
					/* code for DOWN_EQU */
				    down_library(library, SAM_STATE_CHANGE);
					mutex_unlock(&library->un->io_mutex);
					return (-1);
					/* MACRO for cstyle */,
					/* code for ILLREQ */
					    mutex_unlock(
					    &library->un->io_mutex);
					return (-1);
					/* MACRO FOR cstyle */,
					/* More codes */
					    /* MACRO for cstyle */;
						/* MACRO for cstyle */)
			} else {
				err = 0;
				break;
			}
		} while (--retry > 0);
		if (retry <= 0) {
			DevLog(DL_ERR(5203));
		}
		break;

	default:
		DevLog(DL_DETAIL(5085));

		err = 1;
		break;
	}

	mutex_unlock(&library->un->io_mutex);
	return (err);
}


/*
 * Issue SCSI command to Plasmon DVD-RAM library to
 * prevent/allow medium removal from I/E drawer.
 */
int
rotate_plasmon_mailbox(
	library_t *library,
	const int direction)
{
	dev_ent_t 	*un;
	int 		err, retry;
	int 		added_more_time = FALSE;
	sam_extended_sense_t *sense;

	sense = (sam_extended_sense_t *)SHM_REF_ADDR(library->un->sense);
	un = library->un;

	retry = 3;
	do {
		TAPEALERT(library->open_fd, library->un);
		memset(sense, 0, sizeof (sam_extended_sense_t));
		/* For plasmon G, close the door before locking it. */
		if (library->un->equ_type == DT_PLASMON_G &&
		    direction == ROTATE_IN) {
			scsi_cmd(library->open_fd, un, SCMD_OPEN_CLOSE_MAILSLOT,
			    0, PLASMON_G_CLOSE);
		}
		/*
		 * Issue SCSI cmnd to prevent/allow medium removal from
		 * I/E drawer.
		 */
		if (scsi_cmd(library->open_fd, un, SCMD_DOORLOCK, 0,
		    (direction == ROTATE_OUT) ? UNLOCK : LOCK,
		    library->un->equ_type == DT_PLASMON_G ? 0 : 1)) {

			TAPEALERT_SKEY(library->open_fd, library->un);
			GENERIC_SCSI_ERROR_PROCESSING(
			    un, library->scsi_err_tab, 0,
			    err, added_more_time, retry,
						/* DOWN_EQU case statement. */
			    down_library(library, SAM_STATE_CHANGE);
						mutex_unlock(&un->io_mutex);
						return (-1);
						/* MACRO for cstyle */,
						/* ILLREQ case statement. */
						    mutex_unlock(&un->io_mutex);
						return (-1);
						/* MACRO for cstyle */,
						/* Any more code to execute. */
						    /* MACRO for cstyle */;
					/* MACRO for cstyle */);
		} else
			break;		/* success */

	} while (--retry > 0);
	TAPEALERT(library->open_fd, library->un);

	/* If request failed, log error */
	if (retry <= 0) {
		DevLog(DL_ERR(5202));
	}
	return (0);
}


/*
 *	wait_library_ready - wait for unit ready.
 *
 * No mutexs held on entry
 *
 * exit	 -
 *	  when unit ready
 *	  < 0 - fatal error
 *	  > 0 - init_element_status issued
 */
int
wait_library_ready(
	library_t *library)
{
	dev_ent_t 	*un;
	int 		err, retry, ret_code = 0, io_lock = FALSE, loop_cnt = 0;
	char 		*lc_mess;
	sam_extended_sense_t *sense;
	int added_more_time = FALSE;

	un = library->un;
	lc_mess = un->dis_mes[DIS_MES_CRIT];
	sense = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);
	for (;;) {
		if (loop_cnt++ > 5) {
			DevLog(DL_ERR(5205));
			return (-1);
		}
#if 0

		while (un->state != DEV_ON) {
			/* wait for library to go "on" */
			if (io_lock)
				io_lock = mutex_unlock(&un->io_mutex);
			sleep(10);
			wait_on_time -= 10;
		}
#endif
		if	(!io_lock)
				io_lock = !mutex_lock(&un->io_mutex);
		retry = 3;
		do {
			memset(sense, 0, sizeof (sam_extended_sense_t));
			if ((err = scsi_cmd(library->open_fd, un,
			    SCMD_TEST_UNIT_READY, 0)) < 0) {
				TAPEALERT_SKEY(library->open_fd, library->un);
				/* if becoming ready, sleep and try again */
				if ((sense->es_key == KEY_NOT_READY) &&
				    (sense->es_qual_code ==
				    ERR_ASCQ_BECOMMING)) {
					sleep(5);
					if (added_more_time == FALSE) {
						retry += 60;
						added_more_time = TRUE;
					}
					continue;
				}
				if (un->equ_type == DT_3570C &&
				    sense->es_add_code == 0x04 &&
				    (sense->es_qual_code == 0x8d ||
				    sense->es_qual_code == 0x8e)) {
					memccpy(lc_mess,
					    catgets(catfd, SET, 9252,
					    "Must be placed in random mode."),
					    '\0', DIS_MES_LEN);
					DevLog(DL_DETAIL(5131));
					sleep(30);
					*lc_mess = '\0';
					if (added_more_time == FALSE) {
						retry += 60;
						added_more_time = TRUE;
					}
					continue;
				}
				if (err = (int)process_scsi_error(un, NULL,
				    ERR_SCSI_NOLOG)) {
					if ((err == NOT_READY) ||
					    (err == WAIT_READY) ||
					    (err == WAIT_READY_LONG))
						goto proc_err;

					if (err == IGNORE)
						continue;

					memccpy(lc_mess,
					    catgets(catfd, SET, 9071,
					    "error on test unit ready"),
					    '\0', DIS_MES_LEN);
					DevLog(DL_ERR(5129));
					DevLogSense(un);
					if (added_more_time == FALSE) {
						retry += 60;
						added_more_time = TRUE;
					}
					sleep(WAIT_TIME_FOR_READY_LONG);
					*lc_mess = '\0';
					continue;
				}
			} else
				break;
		} while (--retry);

		if (io_lock)
			io_lock = mutex_unlock(&un->io_mutex);

proc_err:
		if ((err == 0) || (err == IGNORE))
			return (ret_code);	/* Everything looks ok */

		if ((err == NOT_READY) && (sense->es_add_code == 0x04))
			switch (sense->es_qual_code) {
			case WAIT_READY_LONG:
				sleep(abs(WAIT_TIME_FOR_READY_LONG -
				    WAIT_TIME_FOR_READY));
			case WAIT_READY:	/* Becoming ready */
				sleep(WAIT_TIME_FOR_READY);
				continue;

			case REZERO:	/* Need rezero */
				if (un->type == DT_METD28 ||
				    un->type == DT_METD360)
					unload_all_drives(library);

				if (!io_lock)
					io_lock = !mutex_lock(&un->io_mutex);
				retry = 3;
				added_more_time = FALSE;
				do {
					switch (un->type) {
					case DT_DOCSTOR:
						err = scsi_cmd(library->open_fd,
						    un, SCMD_REZERO_UNIT, 0);
						break;

					default:
					memset(sense, 0,
					    sizeof (sam_extended_sense_t));
						DevLog(DL_DETAIL(5086));
						err = scsi_cmd(library->open_fd,
						    un,
						    SCMD_INIT_ELEMENT_STATUS,
						    5400);
						ret_code = 1;
						break;
					}

					if (err) {
						TAPEALERT_SKEY(library->open_fd,
						    un);
					}

					if ((err == WAIT_READY) ||
					    (err == WAIT_READY_LONG) ||
					    (err == NOT_READY)) {

						/*
						 * if becoming ready, sleep
						 * and try again
						 */
						if (sense->es_key ==
						    KEY_NOT_READY &&
						    sense->es_qual_code ==
						    ERR_ASCQ_BECOMMING) {
						sleep(
						    WAIT_TIME_FOR_READY);
						if (added_more_time == FALSE) {
							retry += 60;
							added_more_time = TRUE;
						}
						continue;
						}

						DevLog(DL_ERR(5132));
				memccpy(lc_mess,
				    catgets(catfd, SET, 9073,
				    "error on initialize element."),
				    '\0', DIS_MES_LEN);
						DevLogSense(un);
				sleep(WAIT_TIME_FOR_READY_LONG);
						*lc_mess = '\0';
						continue;
					}
					if (err == 0)
						break;

				} while (--retry);
				if (io_lock)
					io_lock = mutex_unlock(&un->io_mutex);
				goto proc_err;

			case ERR_ASCQ_MECH:	/* Mechanical hindrance */
				DevLog(DL_ERR(5130));
				memccpy(lc_mess,
				    catgets(catfd, SET, 9075,
				    "mechanical hindrance"),
				    '\0', DIS_MES_LEN);
				DevLogSense(un);
				sleep(15);
				*lc_mess = '\0';
				continue;
			}
		else
			sleep(5);	/* sleep for a bit */
	}
}


/*
 *	set operator panel to lock or unlock it.
 *
 */
void
set_operator_panel(
	library_t *library,
	const int lock)
{
	/* SpectraLogic code page for locking panel */
	typedef struct {
		robot_ms_hdr_t header;
		uchar_t code;
		uchar_t length;
			uchar_t
#if		defined(_BIT_FIELDS_HTOL)
		:	5,
			Auto:1,
			ChkSum:1,
			EBarCo:1;
#else
			EBarCo:1,
			ChkSum:1,
			Auto:1,
		:	5;
#endif					/* _BIT_FIELDS_HTOL */
		uchar_t queued_unload;
		uchar_t lock_touch_screen;
		uchar_t res0[5];
	}	speclog_pg0_t;

	dev_ent_t *un;
	int len, resid, retry, err, valid = FALSE;
	int added_more_time = FALSE;
	uchar_t *mode_sense_ptr;
	int page_code;
	sam_extended_sense_t *sense;


	un = library->un;
	switch (un->type) {

	case DT_SPECLOG:
		len = sizeof (speclog_pg0_t);
		page_code = 0;
		break;

	case DT_PLASMON_G:
		/*
		 * For plasmon G, close the mailbox door and lock both
		 * the mailbox and magazine.
		 */
		mutex_lock(&library->un->io_mutex);
		scsi_cmd(library->open_fd, un, SCMD_OPEN_CLOSE_MAILSLOT, 0,
		    PLASMON_G_CLOSE);
		mutex_unlock(&library->un->io_mutex);
		LOCK_MAILBOX(library);
		return;
		/* Fall through */
	case DT_PLASMON_D:
		TAPEALERT(library->open_fd, library->un);
		mutex_lock(&library->un->io_mutex);
		scsi_cmd(library->open_fd, library->un, SCMD_DOORLOCK,
		    0, lock, 0);
		mutex_unlock(&library->un->io_mutex);
		TAPEALERT(library->open_fd, library->un);
		return;
	default:
		/* just bail out, its not a library that can lock out */
		return;
	}

	mode_sense_ptr = (uchar_t *)malloc_wait(len, 5, 0);

	mutex_lock(&library->un->io_mutex);
	sense = (sam_extended_sense_t *)SHM_REF_ADDR(library->un->sense);
	retry = 2;
	do {
		(void) memset(sense, 0, sizeof (sam_extended_sense_t));
		if (scsi_cmd(library->open_fd, library->un, SCMD_MODE_SENSE, 30,
		    mode_sense_ptr, len, page_code, &resid) < 0) {
			TAPEALERT_SKEY(library->open_fd, library->un);
			GENERIC_SCSI_ERROR_PROCESSING(un, library->scsi_err_tab,
			    ERR_SCSI_LOG,
			    err, added_more_time, retry,
						/* code for DOWN_EQU */
			    free(mode_sense_ptr);
				mutex_unlock(&library->un->io_mutex);
				down_library(library, SAM_STATE_CHANGE);
						return;
						/* MACRO for cstyle */,
						/* code for ILLREQ */
						    free(mode_sense_ptr);
						mutex_unlock(
						    &library->un->io_mutex);
						return;
						/* MACRO for cstyle */,
						/* More codes */
						    /* MACRO for cstyle */;
					/* MACRO for cstyle */)
		} else {
			valid = TRUE;
			break;
		}
	} while (--retry);

	if (valid) {
		switch (un->type) {
		case DT_SPECLOG:
			((speclog_pg0_t *)mode_sense_ptr)->lock_touch_screen
			    = lock;
			break;
		default:
			break;
		}
		valid = FALSE;
		retry = 2;
		added_more_time = FALSE;
		do {
			memset(sense, 0, sizeof (sam_extended_sense_t));
			if (scsi_cmd(library->open_fd, library->un,
			    SCMD_MODE_SELECT, 0,
			    mode_sense_ptr, len, page_code, &resid) < 0) {
				TAPEALERT_SKEY(library->open_fd, library->un);
				GENERIC_SCSI_ERROR_PROCESSING(un,
				    library->scsi_err_tab,
				    ERR_SCSI_LOG,
				    err, added_more_time, retry,
							/* code for DOWN_EQU */
				    free(mode_sense_ptr);
					mutex_unlock(
					    &library->un->io_mutex);
					down_library(library,
					    SAM_STATE_CHANGE);
							return;
							/* MACRO for cstyle */,
							/* code for ILLREQ */
							    free(
							    mode_sense_ptr);
						mutex_unlock(
						    &library->un->io_mutex);
						return;
						/* MACRO for cstyle */,
						/* More codes */
						    /* MACRO for cstyle */;
						/* MACRO for cstyle */)
			} else {
				valid = TRUE;
				DevLog(DL_DETAIL(5223), lock);
				break;
			}
		} while (--retry);

		if (!valid) {
			DevLog(DL_ERR(5222), page_code);
			DevLogSense(un);
		}

	} else {			/* mode sense failed */
		DevLog(DL_ERR(5221), page_code);
		DevLogSense(un);
	}
	free(mode_sense_ptr);
	mutex_unlock(&library->un->io_mutex);
}


/*
 *	process_res_codes - process read_element_status codes
 *
 * This routine is used for process errors returned in read_element_status.
 *
 */
enum sam_scsi_action
process_res_codes(
	dev_ent_t *un,
	uchar_t sense_key,		/* usually 0x9 (vendor unique) */
	uchar_t add_sense_code,		/* AQC from read_element_status */
	uchar_t add_sense_qual,		/* ASCQ from read_element_status */
	sam_scsi_err_t *scsi_err_tab) 	/* vendor error tab */
{
	sam_scsi_err_t *err_tab;

	if (scsi_err_tab != NULL)
		for (err_tab = scsi_err_tab; err_tab->action != END_OF_LIST;
		    err_tab++)
			if (((sense_key == err_tab->sense_key) ||
			    (err_tab->sense_key == 0xff)) &&
			    ((add_sense_code == err_tab->add_sense) ||
			    (err_tab->add_sense == 0xff)) &&
			    ((add_sense_qual == err_tab->add_sense_qual) ||
			    (err_tab->add_sense_qual == 0xff))) {
				if (err_tab->message_code != 0x00) {
					DevLog(DL_ERR(5133), sense_key,
					    add_sense_code, add_sense_qual,
					    scsi_errcode[
					    err_tab->message_code]);
				}
				return (err_tab->action);
			}
	return (END_OF_LIST);
}
