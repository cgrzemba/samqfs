/*
 * scsi_error.h - SAM-FS generic scsi error interface.
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

#ifndef	_AML_SCSI_ERROR_H
#define	_AML_SCSI_ERROR_H

#pragma ident "$Revision: 1.15 $"

#if defined(__cplusplus)
extern "C" {
#endif

/* Message codes */

#define	MAN_INTER	1	/* Manual intervention reqired */
#define	POWER_ON	2	/* Power on detected */
#define	STOP_BUTTON	3	/* Stop button pressed */
#define	OFFLINE		4	/* Unit is offline */
#define	DOOR_OPEN	5	/* Door is open */
#define	SELF_FAILED	6	/* Self test failed */
#define	STANDBY		7	/* Standby mode */
#define	HARDWARE	8	/* fatal hardware error */
#define	PICKFAIL	9	/* Media pick failed */
#define	WRONG_MODE	10	/* Wrong mode */
#define	MEDIA_RESER	11	/* Media reserved by another */
#define	INCOMPATIBLE	12	/* Incompatible media */
#define	ACCESS_DENIED	13	/* Access to library denied */
#define	DRVSLOT_EMPTY	14	/* Access to library denied */

#if defined(SCSI_ERRCODES)
char *scsi_errcode[] = {
	"",
	"Fatal hardware error, manual intervention required.",
	"Power on/reset.",
	"Stop button pressed.",
	"Unit is offline.",
	"Door is open.",
	"Self test failed.",
	"Unit is in standby.",
	"Fatal hardware error.",
	"Media pick failure.",
	"Changer is not in the correct mode.",
	"Media is reserved by another initiator.",
	"Incompatible media",
	"Access to library denied.",
	"Drive slot empty.",
};
#else
extern char *scsi_errcode[];
#endif

/* Definitions for general scsi error interface */

enum sam_scsi_action {
	WAIT_READY = 1,		/* Wait for ready (5 second sleep) retry */
	WAIT_READY_LONG,	/* Wait for ready (10 second sleep) retry */
	WAIT_INIT,		/* Wait ready then init and update */
				/* element status */
	WAIT_INIT_AUDIT,	/* WAIT_INIT followed by audit */
	IGNORE,			/* Ignore and continue */
	D_ELE_FULL,		/* Destination element full */
	S_ELE_EMPTY,		/* Source element empty */
	NO_MEDIA,		/* Media not present */
	CLEANING_CART,		/* Cleaning cart is in drive */
	BAD_BARCODE,		/* Barcode bad or unreadable */
	LOG_ABOVE_THIS,		/* All actions above this are logged */
	DOWN_EQU,		/* Down equipment */
	DOWN_SUB_EQU,		/* For robot commands where the drive */
				/* needs to be downed */
	CLR_TRANS_RET,		/* Clear transports and retry */
	UPDT_ELEMS_RET,		/* Update elements and retry */
	REZERO,			/* Rezero and retry */
	LOG_IGNORE,		/* Log and retry */
	LONG_WAIT_LOG,		/* Wait a long time (30 second) and retry */
	ILLREQ,			/* Unknown illegal request */
	MARK_UNAVAIL,		/* Make element unavailable */
	INCOMPATIBLE_MEDIA,	/* Incompatible media */
	NOT_READY,		/* Unit is not ready */
	BAD_MEDIA,		/* Media is bad */
	WRITE_PROTECT,
	BLANK_CHECK,
	NEEDS_FORMATTING,	/* Media needs formatting */
	RETRIES_EXCEEDED,
	MARK_EMPTY,		/* Make drive element empty */
	END_OF_LIST
};

typedef struct sam_scsi_err {
	uchar_t		sense_key;
	uchar_t		add_sense;
	uchar_t		add_sense_qual;
	ushort_t		message_code;		/* Message to log */
	enum sam_scsi_action	action;		/* Recovery action */
} sam_scsi_err_t;

/* For process_scsi_error (old form) */

#define	ERR_SCSI_LOG	1	/* Log non fatel errors */
#define	ERR_SCSI_NOLOG	0 	/* Don't log errors */

#define	WAIT_TIME_FOR_READY 5		/* In seconds for WAIT_READY */
#define	WAIT_TIME_FOR_READY_LONG 10	/* In seconds for WAIT_READY_LOG */
#define	WAIT_TIME_FOR_LONG_WAIT_LOG 30	/* In seconds LONG_WAIT_LOG */

/* These are non-retry-able give up errors */
#define	TUR_ERROR(_e)	(((_e) == NO_MEDIA) || \
	((_e) == DOWN_EQU) || \
	((_e) == ILLREQ) || \
	((_e) == INCOMPATIBLE_MEDIA) || \
	((_e) == NOT_READY) || \
	((_e) == BAD_MEDIA) || \
	((_e) == RETRIES_EXCEEDED))

/*
 * GENERIC_SCSI_ERROR_PROCESSING() - general code for most SCSI error processing
 *
 *	_un	(dev_ent_t *)
 *	_errtab			- error processing table to use
 *	_l			- log, !log
 *	_err			- variable where process_scsi_error()
 *				  return value goes
 *	_added_more_time	- variable which defines whether we
 *				  need to add more time on waits
 *	_retry			- retry counter
 *	_downeq			- code to execute on DOWN_EQU
 *	_illreq			- code to execute on ILLREQ
 *	_more_code		- any more code to execute
 */

#define	GENERIC_SCSI_ERROR_PROCESSING(_un, _errtab, _l, _err,		\
	_added_more_time, _retry, _downeq, _illreq, _more_code)	{ 	\
									\
	_err = (int)process_scsi_error(_un, _errtab, _l);		\
	switch ((enum sam_scsi_action)_err) {				\
									\
		case LONG_WAIT_LOG:					\
			sleep(WAIT_TIME_FOR_LONG_WAIT_LOG);		\
			continue;					\
									\
		case WAIT_READY_LONG:					\
			sleep(abs(WAIT_TIME_FOR_READY_LONG - 		\
				WAIT_TIME_FOR_READY));			\
									\
		case WAIT_READY:					\
			if (_added_more_time == FALSE) {		\
				_retry += 60;				\
				_added_more_time = TRUE;		\
			}						\
			sleep(WAIT_TIME_FOR_READY);			\
									\
		default:						\
			break;						\
									\
		case IGNORE:						\
			break;						\
									\
		case ILLREQ:						\
			_illreq						\
									\
		case DOWN_EQU:						\
			_downeq						\
									\
			_more_code					\
	}								\
}

#if 1

/* returned from process_scsi_error */

#define	ERR_SCSI_NOERROR   0x00		/* No error */
#define	ERR_SCSI_RECOVED   0x01		/* Recovered error */
#define	ERR_SCSI_ILLREQ    0x02		/* Illegal request */
#define	ERR_SCSI_SRC_EMPTY 0x03		/* Source element empty */
#define	ERR_SCSI_DEST_FULL 0x04		/* Destination element full */
#define	ERR_SCSI_NO_MEDIA  0x05		/* No media */

#define	ERR_SCSI_ATTNBIT   0x400000	/* Unit attention bit */
#define	ERR_SCSI_MEDIA_I   (0x01 | ERR_SCSI_ATTNBIT)	/* Media imported */
#define	ERR_SCSI_MEDIA_E   (0x02 | ERR_SCSI_ATTNBIT)	/* Media exported */
#define	ERR_SCSI_POWERON   (0x03 | ERR_SCSI_ATTNBIT)	/* Power on */
#define	ERR_SCSI_MICROCODE (0x04 | ERR_SCSI_ATTNBIT)	/* M-code changed */
#define	ERR_SCSI_OPERATOR  (0x05 | ERR_SCSI_ATTNBIT)	/* Operator */
#define	ERR_SCSI_UNKNOWNUA (0x06 | ERR_SCSI_ATTNBIT)	/* Unknown */

#define	ERR_SCSI_FATELERR 0x800000	/* Above this is bad */
#define	ERR_SCSI_NOTREADY (0x01 | ERR_SCSI_FATELERR)	/* Not ready */
#define	ERR_SCSI_HARDWARE (0x02 | ERR_SCSI_FATELERR)	/* Hardware */

#endif

#if defined(__cplusplus)
}
#endif

#endif /* _AML_SCSI_ERROR_H */
