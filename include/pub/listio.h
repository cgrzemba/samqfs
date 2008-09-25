/*
 * listio.h - QFS listio definitions.
 *
 * Defines the QFS listio input structure and functions.
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

#ifndef	QFS_LISTIO_H
#define	QFS_LISTIO_H

#ifdef sun
#pragma ident "$Revision: 1.11 $"
#endif

#include <sys/types.h>

#ifdef  __cplusplus
extern "C" {
#endif


typedef int64_t qfs_lio_handle_t;
int qfs_lio_init(qfs_lio_handle_t *hdl);
int qfs_lio_write(int fd,
		int mem_list_count, void **mem_addr, size_t *mem_count,
		int file_list_count, offset_t *file_off, offset_t *file_len,
		qfs_lio_handle_t *hdl);
int qfs_lio_read(int fd,
		int mem_list_count, void **mem_addr, size_t *mem_count,
		int file_list_count, offset_t *file_off, offset_t *file_len,
		qfs_lio_handle_t *hdl);
int qfs_lio_wait(qfs_lio_handle_t *hdl);

#ifdef  __cplusplus
}
#endif

#endif /* QFS_LISTIO_H */
