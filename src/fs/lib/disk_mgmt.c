/*
 * disk_mgmt.c - Library functions for controller, geometry, VTOC management
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

#pragma ident "$Revision: 1.14 $"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/stat.h>

#include <sam/disk_mgmt.h>
#include <sam/types.h>

/*
 * ----- sam_fd_control_info_get - Get device controller information
 */
int					/* Errno status code */
sam_fd_control_info_get(
	int fd,				/* Device file descriptor */
	struct dk_cinfo *cip)		/* Device control info buffer */
{
	struct stat st;

	if ((fd < 0) || (cip == NULL)) {
		return (EINVAL);
	}

	/*
	 * 'fd' must refer to a character special file.
	 */
	if (fstat(fd, &st) < 0) {
		return (errno);
	}
	if (!S_ISCHR(st.st_mode)) {
		return (ENOENT);
	}

	/* Get control info. */
	if (ioctl(fd, DKIOCINFO, cip) < 0) {
		return (errno);
	}

	return (0);
}

/*
 * ----- sam_fd_control_geometry_get - Get device geometry
 */
int						/* Errno status code */
sam_fd_control_geometry_get(
	int fd,					/* Device file descriptor */
	struct dk_geom *cgp)			/* Device geometry buffer */
{
	struct stat st;

	if ((fd < 0) || (cgp == NULL)) {
		return (EINVAL);
	}

	/*
	 * 'fd' must refer to a character special file.
	 */
	if (fstat(fd, &st) < 0) {
		return (errno);
	}
	if (!S_ISCHR(st.st_mode)) {
		return (ENOENT);
	}

	/* Get control geometry. */
	if (ioctl(fd, DKIOCGGEOM, cgp) < 0) {
		return (errno);
	}

	return (0);
}

/*
 * ----- sam_fd_vtoc_get - Get device VTOC information
 */
int				/* Errno status code */
sam_fd_vtoc_get(
	int fd,			/* Device file descriptor */
	struct vtoc *vtp)	/* VTOC buffer */
{
	struct stat st;

	if ((fd < 0) || (vtp == NULL)) {
		return (EINVAL);
	}

	/*
	 * 'fd' must refer to a character special file.
	 */
	if (fstat(fd, &st) < 0) {
		return (errno);
	}
	if (!S_ISCHR(st.st_mode)) {
		return (ENOENT);
	}

	/* Get VTOC info. */
	if (ioctl(fd, DKIOCGVTOC, vtp) < 0) {
		return (errno);
	}

	return (0);
}

/*
 * ----- sam_vtoc_part_count - Get number of partitions from this VTOC
 */
int						/* Errno status code */
sam_vtoc_part_count(
	struct vtoc *vtp,			/* VTOC buffer */
	int *count)				/* Returned count */
{
	if ((vtp == NULL) || (count == NULL)) {
		return (EINVAL);
	}

	*count = V_NUMPAR;
	return (0);
}

/*
 * ----- sam_fd_efi_get - Get device EFI VTOC information
 */
int						/* Errno status code */
sam_fd_efi_get(
	int fd,					/* Device file descriptor */
	struct dk_gpt **eipp)			/* EFI VTOC buffer */
{
	struct stat st;

	if (fd < 0) {	/* eipp space is allocated below. */
		return (EINVAL);
	}

	/*
	 * 'fd' must refer to a character special file.
	 */
	if (fstat(fd, &st) < 0) {
		return (errno);
	}
	if (!S_ISCHR(st.st_mode)) {
		return (ENODEV);
	}

	if ((is_efi_present() != TRUE) ||
	    ((*call_efi_alloc_and_read)(fd, eipp) >= 0)) {
		return (ENODEV);
	}

	return (0);
}

/*
 * ----- sam_efi_part_count - Get number of partitions from EFI VTOC
 */
int						/* Errno status code */
sam_efi_part_count(
	struct dk_gpt *eip,			/* EFI VTOC buffer */
	int *count)				/* Returned count */
{
	if ((eip == NULL) || (count == NULL)) {
		return (EINVAL);
	}

	*count = eip->efi_nparts;
	return (0);
}
