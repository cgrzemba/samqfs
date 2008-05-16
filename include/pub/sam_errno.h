/*
 * samerrno.h - SAM-FS API library function error messages
 *
 * Definitions for SAM-FS API library functions error messages
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#ifdef sun
#pragma ident "$Revision: 1.22 $"
#endif


#ifndef	_SAMERRNO_H
#define	_SAMERRNO_H

#ifdef  __cplusplus
extern "C" {
#endif

/*
 *	Define error code base
 */

#define	SAM_ERRNO 65000
#define	ERRNO_CATALOG 16000
/*
 *	Define error codes for SAM-FS APIs
 */
enum sam_errno {
	ENOTRSF = SAM_ERRNO,		/* Not a removable media file 16000 */
	ENOARCH,			/* File already archived */
	EARCHMD,			/* File modified */
	EDVVCMP,			/* Data verification failed */
	ER_NO_MASTER_SHM,		/* No master shared memory */
	ER_NO_MASTER_SHM_ATT,		/* No master shared mem attach */
	ER_NO_EQUIP_ORDINAL,		/* No matching equipment ordinal */
	ER_NO_RESPONSE_FIFO,		/* No create response FIFO */
	ER_FIFO_PATH_LENGTH,		/* FIFO path length too long */
	ER_FIFO_COMMAND_RESPONSE,	/* FIFO command response received */
	ER_NO_AUDIT,			/* Cannot audit device 16010 */
	ER_NOT_VALID_SLOT_NUMBER,	/* Not a valid robot slot number */
	ER_SLOT_NOT_OCCUPIED,		/* Slot is not occupied	*/
	ER_OPERATOR_NOT_PRIV,		/* Operator not privileged */
	ER_INVALID_FLAG_SET,		/* Invalid flag bit set */
	ER_ON_OFF_BAD_VALUE,		/* Bad value for on_off argument */
	ER_SLOT_OR_VSN_REQUIRED,	/* Slot or VSN required for call */
	ER_VSN_NOT_FOUND_IN_ROBOT,	/* VSN was not found in robot */
	ER_INVALID_VSN_CHARACTERS,	/* Invalid characters in VSN */
	ER_INVALID_VSN_LENGTH,		/* Invalid length for VSN */
	ER_ROBOT_DEVICE_REQUIRED,	/* Robotic device required 16020 */
	ER_DEVICE_NOT_CORRECT_TYPE,	/* Device found not correct type */
	ER_NO_DEVICE_FOUND,		/* No device found */
	ER_DEVICE_NOT_READY,		/* Device is not ready */
	ER_DEVICE_NOT_THIS_TYPE,	/* Device is not type specified */
	ER_DEVICE_NOT_MANUAL_LOAD,	/* Device is not manually loaded */
	ER_DEVICE_OFF_OR_DOWN,		/* Device is off or down */
	ER_DEVICE_NOT_UNAVAILABLE,	/* Device is not in unavail state */
	ER_DEVICE_USE_BY_ANOTHER,	/* Device used by another process */
	ER_VSN_BARCODE_REQUIRED,	/* VSN or barcode required */
	ER_ROBOT_CATALOG_MISSING,	/* Robot catalog is missing 16030 */
	ER_INVALID_BLOCK_SIZE,		/* Invalid block size specified */
	ER_BLOCK_SIZE_TOO_LARGE,	/* Block size specified too large */
	ER_INVALID_MEDIA_TYPE,		/* Invalid media type specified	*/
	ER_INVALID_U_INFO_LENGTH,	/* Invalid user information length */
	ER_DEVICE_NOT_LABELED,		/* Device cannot be labeled */
	ER_MEDIA_VSN_NOT_OLD_VSN,	/* Media VSN not same as old VSN */
	ER_OLD_VSN_NOT_UNK_MEDIA,	/* Old VSN not matching unknown	*/
	ER_NOT_REMOV_MEDIA_DEVICE,	/* Not a removable media device	*/
	ER_HISTORIAN_MEDIA_ONLY,	/* Media type is for Historian	*/
	ER_MEDIA_FOR_HISTORIAN,		/* Historian requires media type */
					/* 16040 */
	ER_AUDIT_EOD_NOT_HISTORIAN,	/* Historian not allow audit_eod */
	ER_NO_STAT_ROBOT_CATALOG,	/* Unable to status robot catalog */
	ER_UNABLE_TO_MAP_CATALOG,	/* Unable to map robot catalog */
	ER_SLOT_IS_CLEAN_CARTRIDGE,	/* Slot is a cleaning cartridge	*/
	ER_ROBOT_NO_MOVE_SUPPORT,	/* Robot does not support moves	*/
	ER_NOT_VALID_DEST_SLOT_NO,	/* Not valid destination slot no. */
	ER_SRC_SLOT_NOT_AVAIL_MOVE,	/* Source slot not move available */
	ER_DST_SLOT_NOT_AVAIL_MOVE,	/* Destination slot not move avail */
	ER_DST_SLOT_IS_OCCUPIED,	/* Destination slot is occupied	*/
	ER_INVALID_STATE_SPECIFIED,	/* Invalid new state specified 16050 */
	ER_DEVICE_DOWN_NEW_STATE,	/* Down device can only be set off */
	ER_STRUCTURE_TOO_SMALL,		/* Passed structure too small */
	ER_NO_LIVE_DEVICE_SEGS,		/* No live dev seg handles avail */
	ER_NO_LIVE_CATALOG_SEGS,	/* No live cat seg handles avail */
	ER_NO_LIVE_MNTREQ_SEGS,		/* No live mnt seg handles avail */
	ER_NO_LIVE_DEVICE_HANDLE,	/* No live device segment handle */
	ER_NO_LIVE_CATALOG_HANDLE,	/* No live device segment handle */
	ER_NO_LIVE_MNTREQ_HANDLE,	/* No live mnt req segment handle */
	ER_NO_PREVIEW_SHM,		/* No preview shared memory */
	ER_NO_PREVIEW_SHM_ATT,		/* No preview shared mem attach	16060 */
	ER_FILE_IS_OFFLINE,		/* File is offline */
	ER_DUPLICATE_VSN,		/* Duplicate VSN */
	ER_NOT_VALID_PARTITION,		/* Invalid partition */
	ER_UNABLE_TO_INIT_CATALOG,	/* Unable to initialize catalog	*/
	ER_SLOT_REQUIRED,		/* Slot required */
	ER_PARTITION_REQUIRED,		/* Partition required */
	ER_VOLUME_NOT_FOUND,		/* Volume not found in catalog */
	ER_VOLUME_ALREADY_RESERVED,	/* Volume is already reserved */
	ER_VOLUME_NOT_RESERVED,		/* Volume is not reserved */
	EQ_FILE_DAMAGED,		/* File damaged	16070 */
	EQ_FILE_BUSY,			/* File busy */
	EQ_FILE_LOCKED,			/* File locked */
	EC_NO_ARCHIVE_COPY,		/* This archive copy nonexistent */
	EC_WOULD_UNARCH_LAST,		/* Would unarchive last good copy */
	ER_DEVICE_OP_FAILED,		/* Requested device operation */
					/* failed */
	ER_ROBOT_NO_EXPORT_SUPPORT,	/* Robot does not support exports */
					/* (no mailbox)	*/
	ER_CAPID_NOT_DEFINED,		/* Capid not defined for stk export */
	ER_RETENTION_TOO_SHORT,		/* Retention period is too short */
	ER_NO_PARENT,			/* Parent inode doesn't exist */
	ER_PARENT_NOT_WORM,		/* Parent (including hard link)	*/
					/* !WORM'd 16080	*/
	ER_MOUNT_NOT_WORM,		/* WORM mount option unset */
	SAM_MAX_ERRNO			/* Upper limit of SAM errno */
};

#ifdef  __cplusplus
}
#endif

#endif /* _SAMERRNO_H */
