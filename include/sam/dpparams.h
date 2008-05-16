/*
 * dpparams.h
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

#ifndef _SAM_DPPARAMS_H
#define	_SAM_DPPARAMS_H

#define	MAX_BLOCKSIZE 2048
#define	MAX_B_M2 "2048"
#define	MAX_B_SA "2048"
#define	MAX_B_SO "1024"
#define	MAX_B_TI "2048"

#if defined(DEVICE_DEFAULTS)

static struct {
	char	device[5];
	char	*params;
} DeviceParamsVals [] = {

/*
 * Historical note:
 * Some Solaris x86 SCSI HBA drivers only support 32-bit DMA operations.
 * This caused a problem for the st driver on machines with more than
 * 4Gbytes of memory.  Also, some HBA drivers did not support the dma-max
 * option to scsi_ifgetcap().
 * This means that we could only write 64KB per block with page aligned
 * buffers, and less than this with misaligned buffers.  Larger writes were
 * silently divided into multiple blocks, which broke SAM tape positioning.
 * These problems are fixed by bugids 6250131, 6254081, 6274608.
 * Also see bugid 6210716.
 */
	{ "tp", "unload 15" },
	{ "at", "blksize 128, delay 60, unload 5" },
	{ "d3", "blksize 256, position_timeout 135684, unload 7" },
	{ "dt", "blksize 16, unload 5" },
	{ "fd", "blksize 256, unload 15" },
	{ "i7", "blksize 128, unload 15" },
	{ "ib", "blksize 256, unload 15" },
	{ "m2", "blksize "MAX_B_M2", unload 15" },
	{ "li", "blksize 256, delay 60, unload 15" },
	{ "lt", "blksize 128, delay 60, position_timeout 135684, unload 7" },
	{ "sa",
	    "blksize "MAX_B_SA", delay 60, position_timeout 1000,unload 5" },
	{ "se", "blksize 128, position_timeout 135684, unload 15" },
	{ "sf", "blksize 256, position_timeout 10800, unload 7" },
	{ "sg", "blksize 256, position_timeout 3600, unload 7" },
	{ "so", "blksize "MAX_B_SO", unload 7" },
	{ "st", "blksize 128, unload 15" },
	{ "vt", "blksize 128, unload 15" },
	{ "xm", "blksize 128, unload 5" },
	{ "xt", "blksize 16, unload 5" },
	{ "ti",
	    "blksize "MAX_B_TI", delay 60, position_timeout 1000, unload 25" },
	{ "" }
};

#endif  /* defined(DEVICE_DEFAULTS) */

#endif  /* _SAM_DPPARAMS_H */
