/*
 *	scd.h - SAM-QFS system call daemons.
 *
 *	Type definitions for the SAM-QFS global system call daemons.
 *	Type definitions for the SAM-QFS file system call daemons.
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
#ifndef	_SAM_FS_SCD_H
#define	_SAM_FS_SCD_H

#ifdef sun
#pragma ident "$Revision: 1.32 $"
#endif

#include	<sam/sys_types.h>
#include	<sam/resource.h>
#include	"fsdaemon.h"


#define	SAM_SCD_TIMEOUT 5			/* Global system call daemons */

/* Begin: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack(4)
#endif


/*
 * ----- Stageall system call daemon (sam-stagealld).
 */
struct sam_stageall {
	sam_id_t	id;			/* i-number/gen */
	equ_t		fseq;		/* filesystem equipment */
};

/* End: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack()
#endif


/*
 * ----- System call daemons (SCD) - daemons that wait on an event
 * from any filesystem - are listed below.  Each daemon has a
 * sam_syscall_daemon structure in the static sam_scd_table table.
 */


enum SCD_daemons {
	SCD_fsd		= 0,
	SCD_stageall	= 1,
	SCD_stager	= 2,
	SCD_MAX
};

struct sam_syscall_daemon {
	kmutex_t mutex;		/* Mutex covering this struct */
	kcondvar_t get_cv;	/* CV - waiting to get data */
	kcondvar_t put_cv;	/* CV - waiting to put data */
	short put_wait;		/* Condition flag */
	short package;		/* Package flag */
	short active;		/* Daemon active flag */
	short scd_pad;		/* Pad to 64-bit boundary */
	int size;		/* Size of command */
	int timeout;		/* Ret EAGAIN if no daemon in timeout seconds */
	union {
		struct sam_fsd_cmd fsd;
		struct sam_stageall stageall;
		sam_stage_request_t	stager_arg;
	} cmd;
};

#endif	/* _SAM_FS_SCD_H */
