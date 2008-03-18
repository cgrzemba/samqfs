/*
 * sam_fifo.h - defines and structs for the system call interface.
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#if !defined(_AML_FIFO_H)
#define	_AML_FIFO_H

#pragma ident "$Revision: 1.16 $"

#include "sam/types.h"
#include "pub/devstat.h"
#include "aml/types.h"
#include "sam/mount.h"
#include "sam/resource.h"
#include "aml/exit_fifo.h"

/* cmd commands */

#define	SAM_FIFO_PATH	SAM_AMLD_HOME
#define	CMD_FIFO_NAME	"FIFO_CMD"
#define	CMD_FIFO_MAGIC	0x4c644b6d

#define	CMD_OFFSET	0x0100	/* offset for cmd's */
#define	CMD_OFFSET_MASK	0x0f00
#define	CMD_MASK	0x00ff

/*
 * For flags
 * Label flags are defined in labels.h,
 * audit flags are defined in message.h.  Fix this someday
 */

#define	CMD_LABEL_ERASE		LABEL_ERASE
#define	CMD_LABEL_RELABEL	LABEL_RELABEL
#define	CMD_LABEL_SLOT		LABEL_SLOT
#define	CMD_LABEL_BARCODE	LABEL_BARCODE
#define	CMD_UNLOAD_SLOT    0x04		/* slot request to robot */
#define	CMD_AUDIT_EOD    AUDIT_FIND_EOD


/*
 * Flags for CMD_FIFO_CATALOG - or together all of the bits that you
 * want changed, and store the value into the "flags" field.
 * The LSB of the "value" field in the sam_cmd_fifo_t contains the
 * value that the flags will be set to in the LSB.
 *
 * It is only possible to set the vsn on media marked as strange media
 * and it must be done by slot.
 */
#define	CMD_CATALOG_NEEDS_AUDIT		0x000001
#define	CMD_CATALOG_SLOT_INUSE		0x000002
#define	CMD_CATALOG_LABELED		0x000004
#define	CMD_CATALOG_BAD_MEDIA		0x000008
#define	CMD_CATALOG_SLOT_OCCUPIED	0x000010
#define	CMD_CATALOG_CLEANING		0x000020
#define	CMD_CATALOG_BAR_CODE		0x000040
#define	CMD_CATALOG_WRITE_PROTECT	0x000080
#define	CMD_CATALOG_READ_ONLY		0x000100
#define	CMD_CATALOG_RECYCLE		0x000200
#define	CMD_CATALOG_UNAVAIL		0x000400
#define	CMD_CATALOG_EXPORT_SLOT		0x000800
#define	CMD_CATALOG_VSN			0x001000
#define	CMD_CATALOG_STRANGE		0x002000
#define	CMD_CATALOG_PRIORITY		0x004000

/*
 * Only one of these flags can be set in any call.
 * For these flags, the "value" field contains the value
 * to which the catalog entry will be set.
 */

#define	CMD_CATALOG_CAPACITY	0x100000	/* set capacity */
#define	CMD_CATALOG_SPACE	0x200000	/* set space */
#define	CMD_CATALOG_TIME	0x400000	/* set last-mount time */
#define	CMD_CATALOG_COUNT	0x800000	/* set mount count */


/* Flags for CMD_FIFO_IMPORT */
#define	CMD_IMPORT_AUDIT 0x01		/* audit to eod imported media */
#define	CMD_IMPORT_STRANGE 0x02		/* imported media is not sam */

/* Flags for CMD_ADD_VSN */
#define	CMD_ADD_VSN_AUDIT 0x01		/* audit to eod */
#define	CMD_ADD_VSN_STRANGE 0x02	/* imported media is not sam */

/* Flags for CMD_FIFO_EXPORT */
#define	CMD_EXPORT_ONESTEP 0x01		/* ACSLS one step export */

/* Flags for CMD_FIFO_EXPORT */
#define	CMD_MOUNT_S_MIGKIT 0x01		/* Migration toolkit mount */

/* commands and required fields */

/* mount a vsn - (vsn, media) */
#define	CMD_FIFO_MOUNT    CMD_OFFSET + 1

/* mount a slot - (robots eq, slot) */
#define	CMD_FIFO_MOUNT_S   CMD_OFFSET + 2

/* unload - (eq, optional slot) */
#define	CMD_FIFO_UNLOAD    CMD_OFFSET + 3

/* label - (eq, vsn, others) */
#define	CMD_FIFO_LABEL    CMD_OFFSET + 4

/* set state - (eq, state) */
#define	CMD_FIFO_SET_STATE CMD_OFFSET + 5

/* start audit - (robots eq, slot) */
#define	CMD_FIFO_AUDIT    CMD_OFFSET + 6

/* remove vsn - (robots eq, vsn) */
#define	CMD_FIFO_REMOVE_V  CMD_OFFSET + 7

/* remove slot - (robots eq, slot) */
#define	CMD_FIFO_REMOVE_S  CMD_OFFSET + 8

/* remove eq - (eq drive) */
#define	CMD_FIFO_REMOVE_E  CMD_OFFSET + 9

/* set tapealert state */
#define	CMD_FIFO_TAPEALERT  CMD_OFFSET + 10

/* import media - (robot eq, flags) */
#define	CMD_FIFO_IMPORT    CMD_OFFSET + 11

/* delete preview entry (vsn, media) | slot) */
#define	CMD_FIFO_DELETE_P  CMD_OFFSET + 12

/* clean drive (eq) */
#define	CMD_FIFO_CLEAN    CMD_OFFSET + 13

/* set/clear bits in catalog (vsn, media, flags) */
#define	CMD_FIFO_CATALOG   CMD_OFFSET + 14

/* move media in catalog (eq, slot, d_slot) */
#define	CMD_FIFO_MOVE    CMD_OFFSET + 15

/* add  to catalog (eq, vsn, flags, info, media) */
#define	CMD_FIFO_ADD_VSN   CMD_OFFSET + 16

/* set sef state */
#define	CMD_FIFO_SEF  CMD_OFFSET + 17

/* load on unavail drive - (eq, (slot | vsn, media)) */
#define	CMD_FIFO_LOAD_U    CMD_OFFSET + 18

/*
 * sam_cmd_fifo.  This struct is used by the outside world.
 * The command must be specific enough to know what field(s) to use.
 */

typedef struct {
	uint_t	magic;			/* command start bit sequence */
	int	cmd;			/* command */
	exit_FIFO_id exit_id;		/* exit response id */
	int	eq;			/* equipment */
	int	slot;			/* slot */
	int	part;			/* partition */
	int	d_slot;			/* destination slot */
	int	flags;			/* flags */
	int	media;			/* type of media */
	int	block_size;		/* block size for label - (Rule 1) */
	dstate_t state;			/* state */
	vsn_t	vsn;			/* zero byte terminated vsn 31 bytes */
					/* max (ansi requirement) */
	vsn_t	old_vsn;		/* the former vsn when re-labelling */
	char	info[128];		/* ansi label information */
	int64_t	value;
} sam_cmd_fifo_t;

#endif /* _AML_FIFO_H */
