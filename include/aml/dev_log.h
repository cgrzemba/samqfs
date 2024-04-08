/*
 * dev_log.h - device log private definitions.
 *
 * Private definitions for the device logging module.
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
 * or https://illumos.org/license/CDDL.
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

#ifndef _AML_DEVLOG_H
#define	_AML_DEVLOG_H

#pragma ident "$Revision: 1.12 $"

#define	DEVLOG_NAME "/var/opt/SUNWsamfs/devlog"

/*
 * The devlog events are used in trace messages.
 * Bits are set in dev_ent_t.log.flags to turn on the message writing.
 */
enum DL_event {			/* Log events */
	DL_detail,		/* Detailed operations */
	DL_err,			/* Operation error */
	DL_label,		/* Label read/write details */
	DL_mig,			/* Migration toolkit */
	DL_msg,			/* Thread/process */
	DL_retry,		/* Operation retry */
	DL_stage_ck,		/* Damaged file checking */
	DL_stage,		/* Staging */
	DL_syserr,		/* System call error */
	DL_date,
	DL_time,		/* Timing */
	DL_module,
	DL__event,		/* Event name */
	DL_debug,		/* Debug messages */
	DL_all,			/* Include all events above */
				/* (this one is always logged) */
	DL_default,
	DL_none,		/* Do not include any events */
	DL_MAX
};

/* The default events. */
#define	DL_def_events ((1 << DL_err) | (1 << DL_retry) | (1 << DL_syserr) | \
	(1 << DL_date))
/* The events for "all". */
#define	DL_all_events ((1 << DL_all) - 1)

#if defined(NEED_DL_NAMES)
static char *DL_names[] = {
	"detail",
	"err",
	"label",
	"mig",
	"msg",
	"retry",
	"stage_ck",
	"stage",
	"syserr",
	"date",
	"time",
	"module",
	"event",
#if defined(DEBUG)	/* Not available in release */
	"debug",
#else
	" ",			/* KEEP the same number as in #if */
#endif /* defined(DEBUG) */

	"all",
	"default",
	"none",
	""
};
#endif /* defined(NEED_DL_NAMES) */

/* Macros to make up arguments for DevLog() */
#define	DL_ALL(n) ((DL_all << 24) | ((n/1000) << 16) | n % 1000), \
	un, _SrcFile, __LINE__

#define	DL_DEBUG(n) ((DL_debug << 24) | ((n/1000) << 16) | (n % 1000)), \
	un, _SrcFile, __LINE__

#define	DL_DETAIL(n) ((DL_detail << 24) | ((n/1000) << 16) | (n % 1000)), \
	un, _SrcFile, __LINE__

#define	DL_ERR(n) ((DL_err << 24) | ((n/1000) << 16) | (n % 1000)), \
	un, _SrcFile, __LINE__

#define	DL_LABEL(n) ((DL_label << 24) | ((n/1000) << 16) | (n % 1000)), \
	un, _SrcFile, __LINE__

#define	DL_MIG ((DL_mig << 24) | ((n/1000) << 16) | (n % 1000)), \
	un, _SrcFile, __LINE__

#define	DL_MSG(n) ((DL_msg << 24) | ((n/1000) << 16) | (n % 1000)), \
	un, _SrcFile, __LINE__

#define	DL_RETRY(n) ((DL_retry << 24) | ((n/1000) << 16) | (n % 1000)), \
	un, _SrcFile, __LINE__

#define	DL_STAGE_CK(n) ((DL_stage_ck << 24) | ((n/1000) << 16) | (n % 1000)), \
	un, _SrcFile, __LINE__

#define	DL_STAGE(n) ((DL_stage << 24) | ((n/1000) << 16) | (n % 1000)), \
	un, _SrcFile, __LINE__

#define	DL_SYSERR(n) ((DL_syserr << 24) | ((n/1000) << 16) | (n % 1000)), \
	un, _SrcFile, __LINE__

#define	DL_TIME(n) ((DL_time << 24) | ((n/1000) << 16) | (n % 1000)), \
	un, _SrcFile, __LINE__

void DevLog(const uint_t MsgCtl, dev_ent_t *un, const char *SrcFile,
	const int SrcLine, ...);
void DevLogCdb(dev_ent_t *un);
void DevLogSense(dev_ent_t *un);
void AttachDevLog(dev_ent_t *un);
void InitDevLog(dev_ent_t *un);

#endif /* _AML_DEVLOG_H */
