/*
 * disk_show.h - Definitions to show controller, geometry, VTOC information
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

#ifndef _SAM_DISK_SHOW_H
#define	_SAM_DISK_SHOW_H

#ifdef sun
#pragma ident "$Revision: 1.9 $"
#endif

#ifdef sun
#include <sys/dkio.h>
#include <sys/vtoc.h>
#include <sys/uuid.h>
#include <sys/efi_partition.h>

/* Unrecognized disk type */
#define	DKC_UNKNOWN_STR		"unknown"
#define	DT_UNKNOWN_STR		"unknown"

/* Stripe group string */
#define	DT_STRIPE_GROUP_STR	"g"

/* Declarations of DKIO management library functions */

int
sam_control_info_format(struct dk_cinfo *cip, sam_format_buf_t *bufp);

int
sam_control_geometry_format(struct dk_geom *cgp, sam_format_buf_t *bufp);

int
sam_control_type_to_str(unsigned long ctype, char *strp, int len);

/* Definitions VTOC info display */

/* Unrecognized VTOC partition ID tag or permission */
#define	V_UNKNOWN_STR	"unknown"

/* VTOC permission flag values for display calculations */
#define	V_NONE		0x0
#define	V_RO_UNMNT	(V_RONLY | V_UNMNT)

/* Declare VTOC info display library functions */

int
sam_vtoc_format(struct vtoc *vtp, sam_format_buf_t *bufp);

int
sam_vtoc_partition_format(struct vtoc *vtp, int index, sam_format_buf_t *bufp);

int
sam_vpart_id_to_str(unsigned long id, char *strp, int len);

int
sam_vpart_pflags_to_str(unsigned long pflags, char *strp, int len);

/* Unrecognized EFI VTOC UID */
#define	UUID_UNKNOWN_STR	"unknown"

/* Declare EFI VTOC Management Library Functions */

int
sam_efi_format(struct dk_gpt *eip, sam_format_buf_t *bufp);

int
sam_efi_partition_format(struct dk_gpt *eip, int index, sam_format_buf_t *bufp);

int
sam_uuid_to_str(struct uuid *uup, char *strp, int len);

#endif /* sun */

#endif /* _SAM_DISK_SHOW_H */
