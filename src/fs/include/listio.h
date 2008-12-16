/*
 *	listio.h - SAM-FS ioctl listio call.
 *
 *	Type definitions for the SAM-FS ioctl listio.
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
#ifndef	_SAM_FS_LISTIO_H
#define	_SAM_FS_LISTIO_H

#ifdef sun
#pragma ident "$Revision: 1.14 $"
#endif

#include	<sam/fioctl.h>

/*
 * ----- Listio list of outstanding calls. This call exists until the I/O
 * completes. Chained through the incore inode.
 */

struct sam_listio_call {
	struct sam_listio_call *nextp;		/* Next call if one */
	kmutex_t		lio_mutex;	/* Mtx for this struct */
	struct sam_listio	listio;		/* User's arguments */
	int		mem_cnt;	/* Memory array size */
	int		file_cnt;	/* File array size */
	caddr_t		*mem_addr;	/* Memory array: addresses */
	size_t		*mem_count;	/*		: counts */
	offset_t	*file_off;	/* File array: offsets */
	offset_t	*file_len;	/*	   : lengths */
	void		*handle;	/* Wait handle */
	pid_t		pid;		/* Caller pid */
	int32_t		io_count;	/* Outstanding count of I/Os */
	int32_t		error;		/* Error */
	boolean_t	waiting;	/* TRUE if process is waiting */
	ksema_t		cmd_sema;	/* Held until cmd completes */
};


#endif	/* _SAM_FS_LISTIO_H */
