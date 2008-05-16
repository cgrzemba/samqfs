/*
 * sam_label.c - label removable media API
 *
 *	sam_label() causes a new label to be written to the specified removable
 *	media device.
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

#pragma ident "$Revision: 1.19 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */


/* Feature test switches. */
	/* None. */

/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/param.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <libgen.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/device.h"
#include "sam/defaults.h"
#include "aml/fifo.h"
#include "sam/lib.h"
#include "aml/shm.h"
#include "aml/sam_utils.h"
#include "aml/samapi.h"
#include "pub/sam_errno.h"

/* Macros. */

typedef enum media_type {
TAPE,
OPTICAL
}media_type;

/* Private functions. */
static int ConvertToVolId(ushort_t eq, uint_t slot, int part,
	struct VolId *vid);
static int sam_label(struct VolId *vid, media_type med_type, char *new_vsn,
	char *old_vsn, int block_size, char *uinfo, int erase,
	int wait_response);

/* Public data. */
	/* None. */

/* Function macros. */
	/* None. */



/*
 *	sam_tplabel() - API function to write a new label on a tape media
 *
 *	Input parameters --
 *		eq_ord		Equipment ordinal
 *		new_vsn		New volume serial name to be written
 *		old_vsn		Old volume serial name expected on media
 *		slot		If library, slot containing the tape
 *		part		Partition id for D2 tapes
 *		block_size	Block size to be used on the media
 *		erase		Media erased before labeling if nonzero
 *		wait_response	Wait value for label operation to complete
 *				-1	Wait forever
 *				> zero	Wait this many seconds for response
 *
 *	Output parameters --
 *		errno		Set to error number if error encountered
 *
 *	Return value --
 *		0		If successful in labeling the media
 *		-1		If error encountered,
 *				'errno' set to error number
 *
 */

int
sam_tplabel(
	ushort_t eq_ord,	/* Equipment ordinal */
	char *new_vsn,		/* New volume serial name */
	char *old_vsn,		/* Old volume serial name */
	uint_t slot,		/* If library, slot containing the tape */
	int part,		/* Partition id for D2 tapes */
	int block_size,		/* Block size on media */
	int erase,		/* Set nonzero to erase media */
				/* before label is written */
	int wait_response)	/* Nonzero value to wait for response */
{
	int rc = 0;
	struct VolId vid;

	rc = ConvertToVolId(eq_ord, slot, part, &vid);
	if (rc != -1) {
		rc = sam_label(&vid, TAPE, new_vsn, old_vsn, block_size,
		    (char *)NULL, erase, wait_response);
	}
	return (rc);
}


/*
 *	sam_odlabel() - API function to write a new label on an optical disk
 *
 *	Input parameters --
 *		eq_ord		Equipment ordinal
 *		new_vsn		New volume serial name to be written
 *		old_vsn		Old volume serial name expected on media
 *		slot		If library,
 *				slot in library containing the cartridge
 *		side		Side(1 or 2) of cartridge
 *		uinfo		Implementation use string in label
 *		erase		Set nonzero to erase media before label
 *				is written
 *		wait_response	Wait for label operation to complete
 *				-1	Wait forever
 *				> zero	Wait this many seconds for response
 *
 *	Output parameters --
 *		errno		Set to error number if error encountered
 *
 *	Return value --
 *		0		If successful in labeling the media
 *		-1		If error encountered,
 *				'errno' set to error number
 *
 */

int
sam_odlabel(
	ushort_t eq_ord,	/* Equipment ordinal */
	char *new_vsn,		/* New volume serial name */
	char *old_vsn,		/* Old volume serial number */
	uint_t slot,		/* If library, slot containing the cartridge */
	int side,		/* Side (1 or 2) of cartridge */
	char *uinfo,		/* Implementation use string in label */
	int erase,		/* Set nonzero to erase media */
				/* before label is written */
	int wait_response)	/* Set nonzero to wait for labeling */
				/* to complete */
{
	int rc;
	struct VolId vid;

	rc = ConvertToVolId(eq_ord, slot, side, &vid);
	if (rc != -1) {
		rc = sam_label(&vid, OPTICAL, new_vsn, old_vsn, 0, uinfo,
		    erase, wait_response);
	}
	return (rc);
}

static int
ConvertToVolId(
	ushort_t eq,		/* Equipment ordinal */
	uint_t slot,		/* If library, slot containing the cartridge */
	int part,		/* Side (1 or 2) of cartridge */
	struct VolId *vid)	/* Resulting volume id specification */
{
	if (CatalogInit("SamApi") == -1) {
		errno = ER_UNABLE_TO_INIT_CATALOG;
		/* Unable to initialize catalog. */
		return (-1);
	}

	memset(vid, 0, sizeof (struct VolId));

	if ((int)eq < 0 || eq > MAX_DEVICES)  {
		errno = ER_NO_DEVICE_FOUND;
		/* No device found. */
		return (-1);
	}
	vid->ViEq = (uint_t)eq;
	vid->ViFlags |= VI_eq;

	if ((int)slot < 0 || slot > MAX_SLOTS) {
		errno = ER_NOT_VALID_SLOT_NUMBER;
		return (-1);
	}
	vid->ViSlot = (uint_t)slot;
	vid->ViFlags |= VI_slot;

	if (part != 0 && (part <= 0 || part > MAX_PARTITIONS)) {
		errno = ER_NOT_VALID_PARTITION;
		return (-1);
	}

	if (part > 0) {
		vid->ViPart = part;
		vid->ViFlags |= VI_part;
	}
	return (0);
}


/*
 *	sam_label() - Internal function to write a new label
 *		      on a removable media
 *
 *	Input parameters --
 *		eq_ord		Equipment ordinal
 *		med_type	Type of media device
 *		new_vsn		New volume serial number to be written
 *		old_vsn		Old volume serial number expected on media
 *		slot		If a robot, the robot device slot numbe
 *		block_size	Block size to be used on the media
 *		uinfo		Implementation use string in label
 *		erase		Media erased before labeling if nonzero
 *		wait_response	Wait for label operation to complete
 *				-1	Wait forever
 *				> zero	Wait this many seconds for response
 *
 *	Output parameters --
 *		errno		Set to error number if error encountered
 *
 *	Return value --
 *		0		If successful in labeling the media
 *		-1		If error encountered,
 *				'errno' set to error number
 *
 */

static int
sam_label(
	struct VolId *vid,	/* Volume id specifier */
	media_type med_type,	/* Type of media */
	char *new_vsn,		/* New volume serial name */
	char *old_vsn,		/* Old volume serial name */
	int block_size,		/* Block size on media */
	char *uinfo,		/* Information use string in label */
	int erase,		/* Set nonzero to erase media */
	int wait_response)	/* Set nonzero to wait for response */
{
	int new_label = FALSE;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	struct CatalogHdr *ch;
	dev_ent_t *dev;

	operator_t operator;
	sam_cmd_fifo_t cmd_block;
	dtype_t type;
	char *fifo_path = NULL;

	/*
	 *	Verify media type
	 */
	switch (med_type) {

	case OPTICAL:
		type = DT_OPTICAL;
		break;

	case TAPE:
		type = DT_TAPE;
		break;

	default:
		errno = ER_INVALID_MEDIA_TYPE;
		/* Invalid media type specified. */
		return (-1);
	}

	/*
	 *	If no old VSN specified, assume new label
	 *	being put on media.
	 */
	if (old_vsn == NULL) {
		new_label = TRUE;
	}

	if (new_vsn == NULL) {
		errno = ER_VSN_BARCODE_REQUIRED;
		/* VSN required. */
		return (-1);
	}

	/*
	 *	Validate volume id specifier.
	 */
	ce = CatalogCheckVolId(vid, &ced);
	if (ce == NULL) {
		errno = ER_VOLUME_NOT_FOUND;
		/* Volume not found in catalog. */
		return (-1);
	}

	ch = CatalogGetHeader(ce->CeEq);
	if (ch == NULL) {
		errno = ER_VOLUME_NOT_FOUND;
		/* Volume not found in catalog. */
		return (-1);
	}

	if (ch->ChType == CH_historian) {
		errno = ER_HISTORIAN_MEDIA_ONLY;
		/* Cannot label volume in historian. */
		return (-1);
	}

	/*
	 *	Get device entry and FIFO path from
	 *	master shared memory segment.
	 */
	if (sam_get_dev(ce->CeEq, &dev, &fifo_path, &operator) < 0) {
		/* sam_get_dev function set errno. */
		return (-1);
	}

	if (!SAM_OPER_LABEL(operator)) {
		errno = ER_OPERATOR_NOT_PRIV;
		/* Operator not privileged. */
		return (-1);
	}

	switch (type) {

	case DT_TAPE:
		if (block_size != 0) {
			if (block_size != 16 && block_size != 32 &&
			    block_size != 64 && block_size != 128 &&
			    block_size != 256 && block_size != 512 &&
			    block_size != 1024 && block_size != 2048) {

				errno = ER_INVALID_BLOCK_SIZE;
				/* Invalid block size specified. */
				return (-1);
			}
			block_size <<= 10;
		}

		if ((int)strlen(new_vsn) > MAX_TAPE_VSN_LEN) {
			errno = ER_INVALID_VSN_LENGTH;
			/* Invalid length for VSN. */
			return (-1);
		}

		if (is_ansi_tp_label(new_vsn, MAX_TAPE_VSN_LEN) != 1) {
			errno = ER_INVALID_VSN_CHARACTERS;
			/* Invalid characters in VSN. */
			return (-1);
		}

		if (old_vsn != NULL) {
			if ((int)strlen(old_vsn) > MAX_TAPE_VSN_LEN) {
				errno = ER_INVALID_VSN_LENGTH;
				/* Invalid length for VSN. */
				return (-1);
			}

			if (is_ansi_tp_label(old_vsn, MAX_TAPE_VSN_LEN) != 1) {
				errno = ER_INVALID_VSN_CHARACTERS;
				/* Invalid characters in VSN. */
				return (-1);
			}
		}
		break;

	case DT_OPTICAL:
		if ((int)strlen(new_vsn) > MAX_OPTIC_VSN_LEN) {
			errno = ER_INVALID_VSN_LENGTH;
			/* Invalid length for VSN. */
			return (-1);
		}

		if (old_vsn != NULL && (int)strlen(old_vsn) >
		    MAX_OPTIC_VSN_LEN) {
			errno = ER_INVALID_VSN_LENGTH;
			/* Invalid length for VSN. */
			return (-1);
		}

		if ((uinfo != (char *)NULL) &&
		    ((int)strlen(uinfo) > MAX_USER_INFO_LEN)) {
			errno = ER_INVALID_U_INFO_LENGTH;
			/* Invalid user information length. */
			return (-1);
		}
		break;

	default:
		errno = ER_DEVICE_NOT_LABELED;
		/* Device cannot be labeled. */
		return (-1);
	}

	if (new_label) {
		if (*ce->CeVsn != 0) {
			errno = ER_OLD_VSN_NOT_UNK_MEDIA;
			/* Old VSN not matching unknown. */
			return (-1);
		}
	} else {
		if (strcmp(old_vsn, ce->CeVsn) != 0) {
			errno = ER_MEDIA_VSN_NOT_OLD_VSN;
			/* Media VSN not same as old VSN. */
			return (-1);
		}
	}

	/* Issue label command */

	memset(&cmd_block, 0, sizeof (sam_cmd_fifo_t));
	cmd_block.magic = CMD_FIFO_MAGIC;
	cmd_block.cmd = CMD_FIFO_LABEL;
	cmd_block.block_size = block_size;
	cmd_block.flags = 0;
	if (old_vsn != NULL) {
		strcpy(cmd_block.old_vsn, old_vsn);
	}
	strcpy(cmd_block.vsn, new_vsn);
	cmd_block.eq = ce->CeEq;
	cmd_block.media = sam_atomedia(ce->CeMtype);
	if (erase) {
		cmd_block.flags |= CMD_LABEL_ERASE;
	}
	if (new_label == FALSE) {
		cmd_block.flags |= CMD_LABEL_RELABEL;
	}
	if (ch->ChType == CH_library) {
		cmd_block.flags |= CMD_LABEL_SLOT;
	}
	cmd_block.slot = ce->CeSlot;
	cmd_block.part = ce->CePart;
	if (uinfo != NULL) {
		strcpy(cmd_block.info, uinfo);
	}

#if 0
	printf("eq:   %d\n", cmd_block.eq);
	printf("vsn:  %s\n", cmd_block.vsn);
	if (*ce->CeVsn != '\0') {
		printf("was:  %s\n", ce->CeVsn);
	}
	if (uinfo != NULL) {
		printf("info: %s\n", cmd_block.info);
	}
	printf("slot:  %d\n", cmd_block.slot);
#endif

	if (sam_send_cmd(&cmd_block, wait_response, fifo_path) < 0) {
		return (-1);
	}
	return (0);
}
