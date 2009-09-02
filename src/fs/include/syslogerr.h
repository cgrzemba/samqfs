/*
 *	syslogerr.h - syslog error definitions for SAM-QFS
 *
 *	Defines the syslog error messages for SAM-QFS.
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

#ifndef	_SAM_FS_SYSLOGERR_H
#define	_SAM_FS_SYSLOGERR_H

#ifdef sun
#pragma ident "$Revision: 1.18 $"
#endif

struct syslog_errors {
	char *message;
	int priority;
};

enum syslog_numbers {
	E_NULL,
	E_COPY_DAMAGED,
	E_BAD_STAGE,
	E_PREALLOC_EXCEEDED,
	E_NOSPACE,
	E_FSWM,
	E_SAM_MAX
};

enum syslog_space_actions {
	E_WAITING = 0,
	E_INPROGRESS,
	E_NOSPC
};

#ifdef	_KERNEL

void sam_syslog(sam_node_t *ip, enum syslog_numbers slerror, int error,
	int param);

#else

/* Used in sam-fsd.c */

/* message format "%s inode %d.%d - message...[%d]...message[: %m]" */
static struct syslog_errors	syslog_errors[] =  {
	"%s inode %d.%d - Undefined message %d: %m", LOG_INFO,
	"%s inode %d.%d - Archive copy %d marked damaged: %m", LOG_INFO,
	"%s inode %d.%d - Illegal stage count = %d: %m", LOG_INFO,
	"%s inode %d.%d - I/O was attempted beyond the preallocated space",
				LOG_INFO,
	"%s File system full - %s", LOG_INFO,
	"Unidentified message", LOG_INFO
};

static char *syslog_nospace_reasons[] = {
	"waiting",
	"EINPROGRESS",
	"ENOSPC"
};

#endif	/* _KERNEL */


#endif	/* _SAM_FS_SYSLOGERR_H */
