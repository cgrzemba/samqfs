/*
 * scsi.h - SAM-FS generic scsi interface.
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

#ifndef _AML_SCSI_H
#define	_AML_SCSI_H

#pragma ident "$Revision: 1.17 $"

#include <sys/scsi/generic/commands.h>
#include "aml/scsi_error.h"

#if defined(__cplusplus)
extern    "C" {
#endif

	typedef struct sam_extended_sense {
		uchar_t
#if defined(_BIT_FIELDS_HTOL)
			es_valid:1,	/* sense data is valid */
			es_class:3,	/* Error Class- fixed at 0x7 */
			es_code	:4;	/* Vendor Unique error code */
#else /* defined(_BIT_FIELDS_HTOL) */
			es_code	:4,	/* Vendor Unique error code */
			es_class:3,	/* Error Class- fixed at 0x7 */
			es_valid:1;	/* sense data is valid */
#endif	/* defined(_BIT_FIELDS_HTOL) */
		uchar_t	es_segnum;	/* segment number: for COPY */
					/* cmd only */

		uchar_t
#if defined(_BIT_FIELDS_HTOL)
			es_filmk:1,	/* File Mark Detected */
			es_eom	:1,	/* End of Media */
			es_ili	:1,	/* Incorrect Length Indicator */
				:1,	/* reserved */
			es_key  :4;	/* Sense key (see below) */
#else	/* defined(_BIT_FIELDS_HTOL) */
			es_key	:4,	/* Sense key (see below) */
				:1,	/* reserved */
			es_ili	:1,	/* Incorrect Length Indicator */
			es_eom	:1,	/* End of Media */
			es_filmk:1;	/* File Mark Detected */
#endif	/* defined(_BIT_FIELDS_HTOL) */

		uchar_t	es_info_1;	/* Information byte 1 */
		uchar_t	es_info_2;	/* Information byte 2 */
		uchar_t	es_info_3;	/* Information byte 3 */
		uchar_t	es_info_4;	/* Information byte 4 */
		uchar_t	es_add_len;	/* Number of additional bytes */

		uchar_t	es_cmd_info[4];	/* Command specific information */
		uchar_t	es_add_code;	/* Additional Sense Code */
		uchar_t	es_qual_code;	/* Additional Sense Code Qualifier */
		uchar_t	es_fru_code;	/* Field Replaceable Unit Code */
		uchar_t	es_skey_specific[3];	/* Sense Key Specific info */

		/* The fixed part is 18 bytes long.  Add additionl here */
		uchar_t	es_add_info[32 - 18];	/* additional information */
	} sam_extended_sense_t;


	typedef struct {
		uchar_t
#if defined(_BIT_FIELDS_HTOL)
			es_valid:1,	/* Sense data is valid */
			es_class:3,	/* Error Class- fixed at 0x7 */
			es_code	:4;	/* Vendor Unique error code */
#else	/* defined(_BIT_FIELDS_HTOL) */
			es_code	:4,	/* Vendor Unique error code */
			es_class:3,	/* Error Class- fixed at 0x7 */
			es_valid:1;	/* Sense data is valid */
#endif	/* defined(_BIT_FIELDS_HTOL) */
		uchar_t	es_segnum;	/* Segment number: for COPY */
					/* cmd only */
		uchar_t
#if defined(_BIT_FIELDS_HTOL)
			es_filmk:1,	/* File Mark Detected */
			es_eom	:1,	/* End of Media */
			es_ili	:1,	/* Incorrect Length Indicator */
				:1,	/* Reserved */
			es_key	:4;	/* Sense key (see below) */
#else	/* defined(_BIT_FIELDS_HTOL) */
			es_key	:4,	/* Sense key (see below) */
				:1,	/* Reserved */
			es_ili	:1,	/* Incorrect Length Indicator */
			es_eom	:1,	/* End of Media */
			es_filmk:1;	/* File Mark Detected */
#endif	/* defined(_BIT_FIELDS_HTOL) */
		uchar_t	es_info[4];	/* Information bytes */
		uchar_t	es_add_len;	/* Number of additional bytes */
		uchar_t	es_cmd_info[4];	/* Command information bytes */
		uchar_t	es_asc;		/* Additional Sense Code */
		uchar_t	es_ascq;	/* Additional Sense Code Qualifier */
		uchar_t	es_fru;		/* Field Replaceable Unit Code */
		uchar_t
#if defined(_BIT_FIELDS_HTOL)
			es_fpv	:1,	/* Field pointer valid */
			es_c_d	:1,	/* Command/data */
				:2,	/* Reserved */
			es_bpv	:1,	/* Bit pointer valid */
			es_bitp	:3;	/* Bit pointer */
#else	/* defined(_BIT_FIELDS_HTOL) */
			es_bitp	:3,	/* Bit pointer */
			es_bpv	:1,	/* Bit pointer valid */
				:2,	/* Reserved */
			es_c_d	:1,	/* Command/data */
			es_fpv	:1;	/* Field pointer valid */
#endif	/* defined(_BIT_FIELDS_HTOL) */
		uchar_t	es_fp[2];	/* Field pointer */
		uchar_t	es_add_info[32 - 18];	/* Additional information */
	} sam_newext_sense_t;

	/* Device Capacity. */

	typedef struct {		/* Data fmt for read capacity cmd */
		uchar_t	blk_addr_3;	/* Logical block */
		uchar_t	blk_addr_2;	/* Logical block address */
		uchar_t	blk_addr_1;	/* Logical block address */
		uchar_t	blk_addr_0;	/* Logical block address (LSB) */
		uchar_t	blk_len_3;	/* Block length (MSB) */
		uchar_t	blk_len_2;	/* Block length */
		uchar_t	blk_len_1;	/* Block length */
		uchar_t	blk_len_0;	/* Block length (LSB) */
	} sam_scsi_capacity_t;


#define	SAM_SENSE_LENGTH sizeof (sam_extended_sense_t)
#define	SAM_CDB_LENGTH 12		/* max cdb size */
#define	SAM_SCSI_DEFAULT_TIMEOUT 300	/* default timeout in seconds */

/* unique scsi commands issued through scsi_cmd */

#define	READ	SCMD_READ
#define	WRITE	SCMD_WRITE

/*
 * scsi commands not defined in sys/scsi/generic/commands.h Command
 * that differ from device to device but have the same scsi code have
 * a "sequence" number in the upper halfbyte of the commands listed
 * below.
 */

#define	SCMD_MOVE_MEDIA			0x02
#define	SCMD_INIT_ELEMENT_STATUS	0x07
#define	SCMD_ROTATE_MAILSLOT		0x0c
#define	SCMD_OPEN_CLOSE_MAILSLOT	(0x0100 | 0x0c)
#if !defined(SCMD_LOCATE)
#define	SCMD_LOCATE			0x2b
#endif
#define	SCMD_POSITION_TO_ELEMENT	0x012b
#define	SCMD_READ_POSITION		0x34
#define	SCMD_WRITE_BUFFER		0x3B
#define	SCMD_INIT_ELE_RANGE_37		0x37
#define	SCMD_DENSITY_SUPPORT		0x44
#define	SCMD_LOG_SELECT			0x4C
#define	SCMD_LOG_SENSE			0x4d
#define	SCMD_MOVE_MEDIUM		0xA5
#define	SCMD_EXCHANGE_MEDIUM		0xA6
#define	SCMD_EXTENDED_ERASE		0xAC
#define	SCMD_READ_ELEMENT_STATUS	0xB8
#define	SCMD_INIT_SINGLE_ELEMENT_STATUS	0xC7	/* Plasmon G library */
#define	SCMD_READY_INPORT		0xDE	/* acl240 unique */
#define	SCMD_FORMAT_TRACK		0xE4
#define	SCMD_INIT_ELE_RANGE		0xE7
#if !defined(SCMD_READ_LONG)
#define	SCMD_READ_LONGi			0xE8
#endif
#if !defined(SCMD_WRITE_LONG)
#define	SCMD_WRITE_LONG			0xEA
#endif
#define	SCMD_ISSUE_CDB			0xFF

/* Some handy constants */

#define	SPINDOWN	0
#define	SPINUP		1

#define	NOEJECT		0
#define	EJECT_MEDIA	1

#define	UNLOCK	0
#define	LOCK	1

#define	ROTATE_IN	0		/* LOCK */
#define	ROTATE_OUT	1		/* UNLOCK */

#define	LOCK_MAILBOX(a)		(void) rotate_mailbox(a, ROTATE_IN)
#define	UNLOCK_MAILBOX(a)	(void) rotate_mailbox(a, ROTATE_OUT)

#define	PLASMON_G_OPEN	1
#define	PLASMON_G_CLOSE	0

/* Initial Test Unit Ready retry count */

#define	SPINUP_DLT_TUR_TIMEOUT	600	/* In secs */
#define	INITIAL_TUR_TIMEOUT	180	/* In secs */

#if defined(__cplusplus)
}
#endif

#endif	/* _AML_SCSI_H */
