/*
 * disk_mgmt.h - Management functions for controller, geometry, VTOC info
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

#ifndef _SAM_FS_DISK_MGMT_H
#define	_SAM_FS_DISK_MGMT_H

#ifdef sun
#pragma ident "$Revision: 1.12 $"
#endif

#ifdef sun
#include <sys/dkio.h>
#include <sys/vtoc.h>
#include <sys/uuid.h>
#include <sys/efi_partition.h>
#include <efilabel.h>

int sam_fd_control_info_get(int fd, struct dk_cinfo *cip);

int sam_fd_control_geometry_get(int fd, struct dk_geom *cgp);

int sam_fd_vtoc_get(int fd, struct vtoc *vtp);

int sam_vtoc_part_count(struct vtoc *, int *);

int sam_fd_efi_get(int fd, struct dk_gpt **eipp);

int sam_efi_part_count(struct dk_gpt *, int *);

#endif /* sun */

#endif /* _SAM_FS_DISK_MGMT_H */
