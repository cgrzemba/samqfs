/*
 * sam_defaults.h - shared memory struct with samfs defaults.
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

#ifndef _SAM_DEFAULTS_H
#define	_SAM_DEFAULTS_H

#ifdef sun
#pragma ident "$Revision: 1.61 $"
#endif

#include "sam/types.h"
#include "aml/types.h"

#define	DEFAULTS_MAGIC 004050601
#define	DEFAULTS_BIN  SAM_VARIABLE_PATH "/defaults.bin"

/*
 * For processing debug, use the sam_debug_t in the main shared memeoy
 * segment, this is just to build and store the default.
 */

typedef struct sam_defaults {
	media_t		optical;	/* Default optical disk type */
	media_t		tape;		/* Default tape type */
	operator_t	operator;	/* Operator settings */
	sam_debug_t	debug;		/* Default debug mode */
	int		timeout;	/* Timeout for direct access */
	int		previews;	/* Max number of preview entries */
	int		stages;		/* Max number of stage requests */
	int		tapemode;	/* Reset value for tape drives mode */
	int		log_facility;	/* Syslog facility */
	int		stale_time;	/* Stale preview time */
	int		idle_unload;	/* Idle time to force media unload */
	int		shared_unload;	/* Idle time for shared drives */
	int		remote_keepalive; /* SAMremote keepalive interval */
	int		avail_timeout;	/* Drive avail timeout in seconds */
	boolean_t	samstorade;	/* StorADE API */
	boolean_t	archive_copy_retention;	/* Archive copy retention */
	uint32_t	flags;
} sam_defaults_t;

/* Flags definitions. */
#define	DF_LABEL_BARCODE	0x00001	/* Labels = barcodes */
#define	DF_RELEASE_OVERLAP	0x00002	/* can releaser/archiver overlap */
#define	DF_BARCODE_LOW		0x00004	/* label from low end of barcode */
#define	DF_EXPORT_UNAVAIL	0x00008	/* exported media is unavailable */
#define	DF_ATTENDED		0x00010	/* attended = 1, unattended = 0 */
#define	DF_START_RPC		0x00020	/* Start RPC daemon */
#define	DF_ALERTS		0x00040	/* generate alerts : consumed by */
					/* SNMP traps, faults logging in GUI */
#define	DF_NEW_RECYCLER		0x00080 /* New recycler enabled - yes or no */
#define	DF_LEGACY_ARCH_FORMAT	0x00100 /* Use the legacy archive format */
#define	DF_PAX_ARCH_FORMAT	0x00200 /* use the new pax arch format */
#define	DF_ALIGN_SCSI_CMDBUF	0x00400 /* align scsi command buffer */

/* Device parameters. */
struct DeviceParams {
	int		count;
	struct DpEntry {
		char	DpName[3];	/* Device name */
		media_t	DpType;		/* Device type */
		uint_t	DpBlock_size;	/* Default blocksize to use */
		int	DpDelay_time;	/* Delay time for dismount/wait */
		uint_t	DpPosition_timeout;	/* Position timeout to use */
		uint_t	DpUnload_time;		/* Delay between spindown */
						/* and unload */
	} DpTable[1];
};

/* defaults.bin definition. */
struct DefaultsBin {
	MappedFile_t		Db;
	sam_defaults_t		DbDefaults;
	struct DeviceParams	DbDeviceParams;
};

/* Public functions. */
sam_defaults_t *GetDefaults(void);
sam_defaults_t *GetDefaultsRw(void);


#endif /* _SAM_DEFAULTS_H */
